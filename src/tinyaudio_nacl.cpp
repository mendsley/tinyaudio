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
#include <limits.h>
#include <ppapi/c/pp_instance.h>
#include <ppapi/c/pp_resource.h>
#include <ppapi/c/ppb_audio.h>
#include <ppapi/c/ppb_audio_config.h>

namespace tinyaudio {

static const int c_nsamples = 2048;
static PP_Instance g_ppInstance;
static const PPB_Audio* g_ppbAudio;
static const PPB_AudioConfig* g_ppbAudioConfig;
static samples_callback g_callback;
static const char* g_lasterror = "";
static float scratch[c_nsamples * 2];

#ifdef PPB_AUDIO_CONFIG_INTERFACE_1_1
static void nacl_stream_callback(void* sample_buffer, uint32_t buffer_size_in_bytes, PP_TimeDelta /*latency*/, void* /*context*/)
#else
static void nacl_stream_callback(void* sample_buffer, uint32_t buffer_size_in_bytes, void* /*context*/)
#endif
{
	if (g_callback) {
		const int nsamples = buffer_size_in_bytes / (2 * sizeof(short));
#if TINYAUDIO_FLOAT_BUS
		g_callback(scratch, nsamples);
		for (int ii = 0; ii < nsamples; ++ii) {
			int32_t sample = (int32_t)((float)0x8000 * scratch[ii]);
			if (sample > SHRT_MAX) sample = SHRT_MAX;
			if (sample < SHRT_MIN) sample = SHRT_MIN;
			((int16_t*)sample_buffer)[ii] = (int16_t)sample;
		}
#else
		g_callback((short*)sample_buffer, nsamples);
#endif
	}
}

void set_nacl_interfaces(PP_Instance instance, const PPB_Audio* audio, const PPB_AudioConfig* audio_config)
{
	g_ppInstance = instance;
	g_ppbAudio = audio;
	g_ppbAudioConfig = audio_config;
}

bool init(int sample_rate, samples_callback callback)
{
	if (g_ppInstance == 0) {
		g_lasterror = "No PP_Instance set. Use ser_nacl_interfaces";
		return false;
	} else if (g_ppbAudio == NULL) {
		g_lasterror = "No PPB_Audio interface set. Use ser_nacl_interfaces";
		return false;
	} else if (g_ppbAudioConfig == NULL) {
		g_lasterror = "No PPB_AudioConfig interface set. Use ser_nacl_interfaces";
		return false;
	}

	if (sample_rate != 44100) {
		g_lasterror = "tinyaudio only supports 44100 for NaCl";
		return false;
	}

	// make sure NaCl isn't doing weird things to our sample buffer
	const uint32_t nsamples =
#ifdef PPB_AUDIO_CONFIG_INTERFACE_1_1
		g_ppbAudioConfig->RecommendSampleFrameCount(g_ppInstance, PP_AUDIOSAMPLERATE_44100, c_nsamples);
#else
		g_ppbAudioConfig->RecommendSampleFrameCount(PP_AUDIOSAMPLERATE_44100, c_nsamples);
#endif

	PP_Resource resource = g_ppbAudioConfig->CreateStereo16Bit(g_ppInstance, PP_AUDIOSAMPLERATE_44100, nsamples);
	if (!resource) {
		g_lasterror = "failed to create a stereo 16bit audio config";
		return false;
	}

	PP_Resource stream = g_ppbAudio->Create(g_ppInstance, resource, nacl_stream_callback, 0);
	if (!stream) {
		g_lasterror = "failed to create the audio stream";
		return false;
	}

	g_callback = callback;
	g_lasterror = "";

	g_ppbAudio->StartPlayback(stream);
	return true;
}

void release()
{
}

const char* last_error()
{
	return g_lasterror;
}

}
