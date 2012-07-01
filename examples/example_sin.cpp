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

#include <math.h>
#include <TINYAUDIO/tinyaudio.h>

static int sample_rate = 44100;
static const float frequency = 440.0f;
static const float two_pi = 6.283185307f;
static const float angle_delta = two_pi * frequency / sample_rate;

static float angle = 0.0f;

// Very simple callback that generates a 440hz sine wave
static void generate_samples(short* samples, int nsamples)
{
	for (; nsamples; --nsamples, samples += 2, angle += angle_delta) {

		// wrap angle to [0.0, 2pi]
		if (angle > two_pi)
			angle -= two_pi;

		const float sin_angle = sinf(angle);
		const short clipped_sample = (short)(sin_angle * 0x7FFF);

		samples[0] = clipped_sample; // left channel
		samples[1] = clipped_sample; // right channel
	}
}

bool start_sample()
{
	if (!tinyaudio::init(sample_rate, &generate_samples))
		return false;

	return true;
}
