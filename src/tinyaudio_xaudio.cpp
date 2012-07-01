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
#if !defined(_CRT_SECURE_NO_WARNINGS)
#	define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <XAudio2.h>

namespace tinyaudio {

static const int c_nsamples = 2048;
static const int c_npackets = 2;

struct XAudioMixer : IXAudio2VoiceCallback
{
	samples_callback m_callback;
	IXAudio2SourceVoice* m_voice;
	short m_packets[c_npackets][c_nsamples * 2];

	void fill_buffer(void* buffer)
	{
		short* sample_data = (short*)buffer;
		m_callback(sample_data, c_nsamples);

		XAUDIO2_BUFFER packet = {0};
		packet.AudioBytes = sizeof(short) * c_nsamples * 2;
		packet.pAudioData = (const BYTE*)sample_data;
		packet.pContext = sample_data;
		m_voice->SubmitSourceBuffer(&packet, NULL);
	}

	void seed_buffers()
	{
		for (int ii = 0; ii < c_npackets; ++ii)
			fill_buffer(m_packets[ii]);
	}

	virtual ~XAudioMixer() {}
	virtual void CALLBACK OnBufferEnd(void* context)
	{
		fill_buffer(context);
	}
	virtual void CALLBACK OnBufferStart(void*) {}
	virtual void CALLBACK OnLoopEnd(void*) {}
	virtual void CALLBACK OnStreamEnd() {}
	virtual void CALLBACK OnVoiceError(void*, HRESULT) {}
	virtual void CALLBACK OnVoiceProcessingPassEnd() {}
	virtual void CALLBACK OnVoiceProcessingPassStart(UINT32) {}
};

static IXAudio2* g_device;
static IXAudio2MasteringVoice* g_master;
static XAudioMixer g_mixer;

static const int c_nlasterror = 128;
static char g_lasterror[c_nlasterror + 1];

#if !defined(_XBOX)
// XAudio 2.7 (June 2010 SDK)
EXTERN_C const GUID DECLSPEC_SELECTANY CLSID_XAudio2 = { 0x5a508685, 0xa254, 0x4fba, {0x9b, 0x82, 0x9a, 0x24, 0xb0, 0x03, 0x06, 0xaf} };
EXTERN_C const GUID DECLSPEC_SELECTANY IID_IXAudio2 = { 0x8bcf1f58, 0x9fe7, 0x4583, {0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb} };

static const char* c_xaudio_module_system = "XAudio2_7.dll";
static const char* c_xaudio_module_distro = ".\\XAudio2_dist.dll";
#endif

bool init(int sample_rate, samples_callback callback)
{
	HRESULT hr;
#if !defined(_XBOX)
	IClassFactory* factory = NULL;

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
	{
		_snprintf(g_lasterror, c_nlasterror, "failed to initialize COM: 0x%08X", hr);
		goto error;
	}

	HMODULE audiodll = LoadLibraryA(c_xaudio_module_system);
	if (!audiodll)
		audiodll = LoadLibraryA(c_xaudio_module_distro);

	if (!audiodll)
	{
		_snprintf(g_lasterror, c_nlasterror, "failed to load the XAudio dll: 0x%08X", GetLastError());
		goto error;
	}

	typedef HRESULT (__stdcall *dll_get_class_object_func)(const IID&, const IID&, LPVOID*);
	dll_get_class_object_func dllgetclassobj = (dll_get_class_object_func)GetProcAddress(audiodll, "DllGetClassObject");
	if (!dllgetclassobj)
	{
		_snprintf(g_lasterror, c_nlasterror, "failed to xaudio's DllGetClassObject: 0x%08X", GetLastError());
		goto error;
	}

	hr = dllgetclassobj(CLSID_XAudio2, IID_IClassFactory, (void**)&factory);
	if (FAILED(hr))
	{
		_snprintf(g_lasterror, c_nlasterror, "failed to create the xaudio class loader: 0x%08X", hr);
		goto error;
	}

	hr = factory->CreateInstance(NULL, IID_IXAudio2, (void**)&g_device);
	if (FAILED(hr))
	{
		_snprintf(g_lasterror, c_nlasterror, "failed to create the xaudio device: 0x%08X", hr);
		goto error;
	}

	factory->Release();
	factory = NULL;

	hr = g_device->Initialize(0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		_snprintf(g_lasterror, c_nlasterror, "failed to initialize the xaudio device: 0x%08X", hr);
		goto error;
	}
#else
	hr = XAudio2Create(&g_device, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		_snprintf(g_lasterror, c_nlasterror, "failed to startup xaudio: 0x%08X", hr);
		goto error;
	}
#endif

	static const UINT32 nchannels = 2;
	hr = g_device->CreateMasteringVoice(&g_master, nchannels, sample_rate, 0, 0, NULL);
	if (FAILED(hr))
	{
		_snprintf(g_lasterror, c_nlasterror, "failed to create the xaudio mastering voice: 0x%08X", hr);
		goto error;
	}

	{
		WAVEFORMATEX mixing_format = {0};
		mixing_format.nChannels = nchannels;
		mixing_format.wBitsPerSample = 16;
		mixing_format.nSamplesPerSec = sample_rate;
		mixing_format.nBlockAlign = sizeof(short) * mixing_format.nChannels;
		mixing_format.nAvgBytesPerSec = mixing_format.nSamplesPerSec * mixing_format.nBlockAlign;
		mixing_format.wFormatTag = WAVE_FORMAT_PCM;

		static const UINT32 mixing_flags = 0;
		hr = g_device->CreateSourceVoice(&g_mixer.m_voice, &mixing_format, mixing_flags, 1.0f, &g_mixer, NULL, NULL);
		if (FAILED(hr))
		{
			_snprintf(g_lasterror, c_nlasterror, "failed to create the xaudio source voice: 0x%08X", hr);
			goto error;
		}
	}

	g_mixer.m_voice->Discontinuity();
	g_mixer.m_voice->Start();

	g_mixer.m_callback = callback;
	g_mixer.seed_buffers();

	g_lasterror[0] = 0;
	return true;

error:
	tinyaudio::release();

#if !defined(_XBOX)
	if (factory)
		factory->Release();
#endif

	return false;
}

void release()
{
	if (g_mixer.m_voice)
	{
		g_mixer.m_voice->Stop();
		g_mixer.m_voice->DestroyVoice();
		g_mixer.m_voice = NULL;
	}

	if (g_master)
	{
		g_master->DestroyVoice();
		g_master = NULL;
	}

	if (g_device)
	{
		g_device->Release();
		g_device = NULL;
	}

#if !defined(_XBOX)
	HMODULE audiodll = GetModuleHandleA(c_xaudio_module_distro);
	if (audiodll)
		FreeLibrary(audiodll);

	audiodll = GetModuleHandleA(c_xaudio_module_system);
	if (audiodll)
		FreeLibrary(audiodll);
#endif
}

const char* last_error()
{
	return g_lasterror;
}

}
