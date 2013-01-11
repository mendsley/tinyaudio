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

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <pulse/simple.h>
#include <semaphore.h>

namespace tinyaudio {

static int g_sample_rate;
static samples_callback g_callback;
static pthread_t g_thread;
static pa_simple* g_pulse;
static const char* g_appname = "tinyaudio app";
static bool g_running;
static const int c_nsamples = 2048;
static const int c_nlasterror = 128;
static char g_lasterror[c_nlasterror];

static void* pulse_thread(void* context)
{
	sem_t* init = (sem_t*)context;

	pa_sample_spec ss;
	ss.format = PA_SAMPLE_S16LE;
	ss.channels = 2;
	ss.rate = g_sample_rate;

	int err;
	g_pulse = pa_simple_new(NULL, g_appname, PA_STREAM_PLAYBACK, NULL, g_appname, &ss, NULL, NULL, &err);
	if (!g_pulse) {
		snprintf(g_lasterror, c_nlasterror, "failed to connect to pulse server: %d", err);
	}

	sem_post(init);
	if (!g_pulse)
		return 0;

	pa_simple* s = g_pulse;
	short samples[c_nsamples*2];

	g_running = true;
	while (g_running) {
		g_callback(samples, c_nsamples);
		if (0 > pa_simple_write(s, samples, sizeof(samples), NULL))
			break;
	}

	pa_simple_flush(s, NULL);
	pa_simple_free(s);
	g_pulse = 0;
	return 0;
}

void set_pulse_application_name(const char* name)
{
	g_appname = name;
}

bool init(int sample_rate, samples_callback callback)
{
	g_sample_rate = sample_rate;
	g_callback = callback;

	sem_t init;
	sem_init(&init, 0, 0);
	pthread_create(&g_thread, NULL, &pulse_thread, &init);
	sem_wait(&init);
	sem_destroy(&init);

	if (!g_pulse)
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
