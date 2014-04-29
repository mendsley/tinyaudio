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
#include <stdio.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

namespace tinyaudio {

struct AndroidPlayer {
	samples_callback callback;
	int currentBuffer;

	static const int c_nbuffers = 2;
	static const int c_nsamples = 2048;
	sample_type buffers[c_nbuffers][c_nsamples * 2];
#if TINYAUDIO_FLOAT_BUS
	int16_t scratch[c_nsamples * 2];
#endif
};

static AndroidPlayer g_player = {0};

static const int c_nlasterror = 128;
static char g_lasterror[c_nlasterror];

static void SLAPIENTRY audio_callback(SLAndroidSimpleBufferQueueItf bq, void* context) {

	AndroidPlayer* p = (AndroidPlayer*)context;

	sample_type* buffer = p->buffers[p->currentBuffer];

	p->callback(buffer, p->c_nsamples);

#if TINYAUDIO_FLOAT_BUS
	// convert from float to int16_t
	for (int ii = 0; ii < p->c_nsamples*2; ++ii) {
		p->scratch[ii] = (int16_t)(0x8000 * buffer[ii]);
	}

	const int16_t* s16buffer = p->scratch;
#else
	const int16_t* s16buffer = buffer;
#endif

	(*bq)->Enqueue(bq, s16buffer, sizeof(int16_t) * 2 * p->c_nsamples);

	p->currentBuffer = (p->currentBuffer + 1 ) % p->c_nbuffers;
}

bool init(int sample_rate, samples_callback callback) {

	g_lasterror[0] = 0;
	g_player.callback = callback;

	SLmilliHertz samplerate;
	switch (sample_rate) {
	case 8000: samplerate = SL_SAMPLINGRATE_8; break;
	case 11025: samplerate = SL_SAMPLINGRATE_11_025; break;
	case 12000: samplerate = SL_SAMPLINGRATE_12; break;
	case 16000: samplerate = SL_SAMPLINGRATE_16; break;
	case 22050: samplerate = SL_SAMPLINGRATE_22_05; break;
	case 24000: samplerate = SL_SAMPLINGRATE_24; break;
	case 32000: samplerate = SL_SAMPLINGRATE_32; break;
	case 44100: samplerate = SL_SAMPLINGRATE_44_1; break;
	case 48000: samplerate = SL_SAMPLINGRATE_48; break;
	case 64000: samplerate = SL_SAMPLINGRATE_64; break;
	case 88200: samplerate = SL_SAMPLINGRATE_88_2; break;
	case 96000: samplerate = SL_SAMPLINGRATE_96; break;
	case 192000: samplerate = SL_SAMPLINGRATE_192; break;
	default:
		snprintf(g_lasterror, c_nlasterror, "Unsupported sample rate %d", sample_rate);
		return false;
	}

	const SLEngineOption engineOpts[] = {
		{SL_ENGINEOPTION_THREADSAFE}, {SL_BOOLEAN_FALSE},
	};

	SLObjectItf iface;
	SLresult res = slCreateEngine(&iface, 1, engineOpts, 0, NULL, NULL);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to create opensl engine: %d", res);
		return false;
	}

	res = (*iface)->Realize(iface, SL_BOOLEAN_FALSE);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to realize engine: %d", res);
		return false;
	}

	SLEngineItf engine;
	res = (*iface)->GetInterface(iface, SL_IID_ENGINE, &engine);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to get engine interface: %d", res);
		return false;
	}

	SLObjectItf outputmix;
	res = (*engine)->CreateOutputMix(engine, &outputmix, 0, NULL, NULL);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to create output mix: %d", res);
		return false;
	}

	res = (*outputmix)->Realize(outputmix, SL_BOOLEAN_FALSE);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to realize output mix: %d", res);
		return false;
	}

	SLDataLocator_AndroidSimpleBufferQueue bufferQueueDesc = {
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
		2,
	};
	SLDataFormat_PCM format = {
		SL_DATAFORMAT_PCM,
		2,
		samplerate,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
		SL_BYTEORDER_LITTLEENDIAN,
	};
	SLDataSource source = {
		&bufferQueueDesc,
		&format,
	};

	SLDataLocator_OutputMix output = {
		SL_DATALOCATOR_OUTPUTMIX,
		outputmix,
	};
	SLDataSink sink = {
		&output,
		NULL,
	};

	static const SLInterfaceID playerIfaces[] = {
		SL_IID_BUFFERQUEUE,
	};
	static const SLboolean playerIfaceReqs[] = {
		SL_BOOLEAN_TRUE,
	};
	SLObjectItf player;
	res = (*engine)->CreateAudioPlayer(engine, &player, &source, &sink, 1, playerIfaces, playerIfaceReqs);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to create audio player: %d", res);
		return false;
	}

	res = (*player)->Realize(player, SL_BOOLEAN_FALSE);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to realize audio player: %d", res);
		return false;
	}

	SLPlayItf play;
	res = (*player)->GetInterface(player, SL_IID_PLAY, &play);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to get SL_IID_PLAY interface: %d", res);
		return false;
	}

	SLAndroidSimpleBufferQueueItf bufferQueue;
	res = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &bufferQueue);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to get the SL_IID_BUFFERQUEUE interface: %d", res);
		return false;
	}

	res = (*bufferQueue)->RegisterCallback(bufferQueue, audio_callback, &g_player);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to register bufferqueue callback: %d", res);
		return false;
	}

	res = (*play)->SetPlayState(play, SL_PLAYSTATE_PLAYING);
	if (res != SL_RESULT_SUCCESS) {
		snprintf(g_lasterror, c_nlasterror, "Failed to start playing SL_IID_PLAY interface: %d", res);
		return false;
	}

	for (int ii = 0; ii < g_player.c_nbuffers; ++ii) {
		audio_callback(bufferQueue, &g_player);
	}

	return true;
}

void release() {
}

const char* last_error() {
	return g_lasterror;
}

}
