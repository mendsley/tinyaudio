/*-
 * Copyright 2011-2013 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "TINYAUDIO/tinyaudio.h"

#include <stdint.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <semaphore.h>

namespace tinyaudio {

static const int c_nsamples = 2048;
static int g_sample_rate;
static samples_callback g_callback;
static pthread_t g_thread;
static snd_pcm_t* g_handle;
static bool g_running;
static const int c_nlasterror = 128;
static char g_lasterror[c_nlasterror];

static bool alsa_init()
{
	int err;

	if (0 > (err = snd_pcm_open(&g_handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0))) {
		snprintf(g_lasterror, c_nlasterror, "failed to open alsa device: %d", err);
		return false;
	}
	
	snd_pcm_hw_params_t* hwparams;
	if (0 > (err = snd_pcm_hw_params_malloc(&hwparams))) {
		snprintf(g_lasterror, c_nlasterror, "failed to alloc hw params: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_hw_params_any(g_handle, hwparams))) {
		snd_pcm_hw_params_free(hwparams);
		snprintf(g_lasterror, c_nlasterror, "failed to initialize hwparams: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_hw_params_set_access(g_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED))) {
		snd_pcm_hw_params_free(hwparams);
		snprintf(g_lasterror, c_nlasterror, "failed to set hwparams access: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_hw_params_set_format(g_handle, hwparams, SND_PCM_FORMAT_S16_LE))) {
		snd_pcm_hw_params_free(hwparams);
		snprintf(g_lasterror, c_nlasterror, "failed to set hwparams format: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_hw_params_set_rate(g_handle, hwparams, g_sample_rate, 0))) {
		snd_pcm_hw_params_free(hwparams);
		snprintf(g_lasterror, c_nlasterror, "failed to set hwparams rate: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_hw_params_set_channels(g_handle, hwparams, 2))) {
		snd_pcm_hw_params_free(hwparams);
		snprintf(g_lasterror, c_nlasterror, "failed to set hwparams channels: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_hw_params(g_handle, hwparams))) {
		snd_pcm_hw_params_free(hwparams);
		snprintf(g_lasterror, c_nlasterror, "failed to set device hwparams: %d", err);
		return false;
	}

	snd_pcm_hw_params_free(hwparams);

	snd_pcm_sw_params_t* swparams;
	if (0 > (err = snd_pcm_sw_params_malloc(&swparams))) {
		snprintf(g_lasterror, c_nlasterror, "failed to alloc swparams: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_sw_params_current(g_handle, swparams))) {
		snd_pcm_sw_params_free(swparams);
		snprintf(g_lasterror, c_nlasterror, "failed to get current swparams: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_sw_params_set_avail_min(g_handle, swparams, 4096))) {
		snd_pcm_sw_params_free(swparams);
		snprintf(g_lasterror, c_nlasterror, "failed to set swparams avail min: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_sw_params_set_start_threshold(g_handle, swparams, 0U))) {
		snd_pcm_sw_params_free(swparams);
		snprintf(g_lasterror, c_nlasterror, "failed to set swparams start threshold: %d", err);
		return false;
	}

	if (0 > (err = snd_pcm_sw_params(g_handle, swparams))) {
		snd_pcm_sw_params_free(swparams);
		snprintf(g_lasterror, c_nlasterror, "failed to set device swparams: %d", err);
		return false;
	}

	snd_pcm_sw_params_free(swparams);

	if (0 > (err = snd_pcm_prepare(g_handle))) {
		snprintf(g_lasterror, c_nlasterror, "failed to prepare device for playback: %d", err);
		return false;
	}

	return true;
}

static void* alsa_thread(void* context)
{
	sem_t* init = (sem_t*)context;

	if (!alsa_init()) {
		
		if (g_handle) {
			snd_pcm_close(g_handle);
			g_handle = 0;
		}
	}

	sem_post(init);
	if (!g_handle)
		return 0;

	int err;
	short samples[c_nsamples * 2];
	snd_pcm_t* pcm = g_handle;
	g_running = true;
	while (g_running) {

		if (0 > (err = snd_pcm_wait(pcm, 1000)))
			break;

		const int frames = snd_pcm_avail_update(pcm);
		if (frames == -EPIPE)
			snd_pcm_prepare(pcm);
		else if (frames < 0)
			break;

		g_callback(samples, c_nsamples);
		if (0 > (err = snd_pcm_writei(pcm, samples, c_nsamples)))
			snd_pcm_prepare(pcm);
	}

	snd_pcm_close(pcm);
	g_handle = 0;
	return 0;
}

bool init(int sample_rate, samples_callback callback)
{
	g_sample_rate = sample_rate;
	g_callback = callback;

	sem_t init;
	sem_init(&init, 0, 0);
	pthread_create(&g_thread, NULL, &alsa_thread, &init);
	sem_wait(&init);
	sem_destroy(&init);

	if (!g_handle)
		return false;
	return true;
}

void release()
{
	g_running = false;
	pthread_join(g_thread, NULL);
}

const char* last_error()
{
	return g_lasterror;
}

}
