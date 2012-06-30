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

#include <stdint.h>
#include <string.h>
#include <ppapi/c/ppb.h>
#include <ppapi/c/ppb_audio.h>
#include <ppapi/c/ppb_audio_config.h>
#include <ppapi/c/ppp.h>
#include <ppapi/c/ppp_instance.h>
#include <ppapi/c/pp_errors.h>
#include <ppapi/c/pp_module.h>
#include <TINYAUDIO/tinyaudio.h>
#include <TINYAUDIO/tinyaudio_nacl.h>

extern bool start_sample();

template<typename T>
static const T* lookup_interface(PPB_GetInterface get_browser, const char* name)
{
	return (const T*)(get_browser(name));
}

static const PPB_Audio* g_audio;
static const PPB_AudioConfig* g_audioconfig;

static PP_Bool naclInstanceDidCreate(PP_Instance instance, uint32_t /*argc*/, const char* /*argn*/[], const char* /*argv*/[])
{
	if (!tinyaudio::set_nacl_interfaces(instance, g_audio, g_audioconfig))
		return PP_FALSE;

	if (!start_sample())
		return PP_FALSE;

	return PP_TRUE;
}

static void naclInstanceDidDestroy(PP_Instance /*instance*/)
{
}

static void naclInstanceDidChangeView(PP_Instance /*instance*/, PP_Resource /*view*/)
{
}

static void naclInstanceDidChangeFocus(PP_Instance /*instance*/, PP_Bool /*focus*/)
{
}

static PP_Bool naclInstanceHandleDocumentLoad(PP_Instance /*instance*/, PP_Resource /*url_loader*/)
{
	return PP_FALSE;
}

PP_EXPORT int32_t PPP_InitializeModule(PP_Module /*module_id*/, PPB_GetInterface get_browser)
{
	g_audio = lookup_interface<PPB_Audio>(get_browser, PPB_AUDIO_INTERFACE);
	g_audioconfig = lookup_interface<PPB_AudioConfig>(get_browser, PPB_AUDIO_CONFIG_INTERFACE);

	if (!g_audio || !g_audioconfig)
		return PP_ERROR_NOINTERFACE;

	return PP_OK;
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name)
{
	if (0 == strcmp(interface_name, PPP_INSTANCE_INTERFACE))
	{
		static PPP_Instance interface =
		{
			&naclInstanceDidCreate,
			&naclInstanceDidDestroy,
			&naclInstanceDidChangeView,
			&naclInstanceDidChangeFocus,
			&naclInstanceHandleDocumentLoad,
		};

		return &interface;
	}

	return NULL;
}

PP_EXPORT void PPP_ShutdownModule()
{
}
