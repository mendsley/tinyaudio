/*-
 * Copyright 2011-2012 Matthew Endsley
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
#include <ppapi/c/pp_instance.h>
#include <ppapi/c/pp_resource.h>
#include <ppapi/c/ppb_audio.h>
#include <ppapi/c/ppb_audio_config.h>

namespace tinyaudio {

static const int c_nsamples = 2048;
static PP_Resource g_stream;
static samples_callback g_callback;
static const char* g_lasterror = "";

static void nacl_stream_callback(void* sample_buffer, uint32_t buffer_size_in_bytes, void* /*context*/)
{
	if (g_callback)
		g_callback((short*)sample_buffer, buffer_size_in_bytes / (2 * sizeof(short)));
}

bool set_nacl_interfaces(PP_Instance instance, const PPB_Audio* audio, const PPB_AudioConfig* audio_config)
{
	// make sure NaCl isn't doing weird things to our sample buffer
	const uint32_t nsamples =
#ifdef PPB_AUDIO_CONFIG_INTERFACE_1_1
		audio_config->RecommendSampleFrameCount(instance, PP_AUDIOSAMPLERATE_44100, c_nsamples);
#else
		audio_config->RecommendSampleFrameCount(PP_AUDIOSAMPLERATE_44100, c_nsamples);
#endif

	PP_Resource resource = audio_config->CreateStereo16Bit(instance, PP_AUDIOSAMPLERATE_44100, nsamples);
	if (!resource) {
		g_lasterror = "failed to create a stereo 16bit audio config";
		return false;
	}

	g_stream = audio->Create(instance, resource, nacl_stream_callback, 0);
	if (!g_stream) {
		g_lasterror = "failed to create the audio stream";
		return false;
	}

	audio->StartPlayback(g_stream);
	return true;
}

bool init(int sample_rate, samples_callback callback)
{
	if (sample_rate != 44100) {
		g_lasterror = "tinyaudio only supports 44100 for NaCl";
		return false;
	}
	if (!g_stream)
		return false;

	g_callback = callback;
	g_lasterror = "";
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
