/**
 Copyright (c) 2026 tomix89

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to use,
 copy, modify, and distribute the Software for non-commercial purposes only,
 subject to the following conditions:

 1. Attribution: All copies or substantial portions of the Software must
 retain this copyright notice and the original author information.

 2. Open-Source Requirement: Any modified versions of the Software must be
 distributed under this same license and made publicly available in source
 form.

 3. Non-Commercial Use: The Software may not be used for commercial purposes
 without explicit written permission from the author.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "audio_controls.h"
#include <stdio.h> // sprintf()

#define DIVISOR 	10
// these all are x10 the real world value so we can have 0.5
#define TONE_MAX 	120  // +12.0dB
#define TONE_MIN   -105  // -10.5dB
#define TONE_STEP    15  //   1.5dB

#define TONE_FREQ_CNT	4

int16_t control_value[AUDIO_CONTROL_CNT];

const char *const bass_freqs[TONE_FREQ_CNT] = { " 50Hz", "100Hz", "200Hz", "250Hz" };

const char *const treb_freqs[TONE_FREQ_CNT] = { " 5kHz", " 7kHz", "10kHz", "15kHz" };

char string_buffer[8]; // for the audio values as string

void audio_increase(AudioControl control) {
	switch (control) {
	case AUDIO_CONTROL_BASS:
	case AUDIO_CONTROL_TREB:
		control_value[control] += TONE_STEP;
		if (control_value[control] > TONE_MAX) {
			control_value[control] = TONE_MAX;
		}
		break;

	case AUDIO_CONTROL_BASS_FREQ:
	case AUDIO_CONTROL_TREB_FREQ:
		control_value[control] += 1;
		if (control_value[control] >= TONE_FREQ_CNT) {
			control_value[control] = TONE_FREQ_CNT - 1;
		}
		break;
	}
}

void audio_decrease(AudioControl control) {
	switch (control) {
	case AUDIO_CONTROL_BASS:
	case AUDIO_CONTROL_TREB:
		control_value[control] -= TONE_STEP;
		if (control_value[control] < TONE_MIN) {
			control_value[control] = TONE_MIN;
		}
		break;

	case AUDIO_CONTROL_BASS_FREQ:
	case AUDIO_CONTROL_TREB_FREQ:
		control_value[control] -= 1;
		if (control_value[control] < 0) {
			control_value[control] = 0;
		}
		break;
	}
}

// format value to dB
static inline void format_db(int16_t current_value, char** ptr) {
	int16_t front =  current_value / DIVISOR;
	int8_t back = abs(current_value - front * DIVISOR);

	uint8_t pos = 0;
	// we don't have a minus sign, so pad
	if (front >= 0) {
		string_buffer[pos++] = ' ';
	}
	// 1 digit number, so pad
	if (abs(front) < 10) {
		string_buffer[pos++] = ' ';
	}

	sprintf(&string_buffer[pos], "%d.%ddB", front, back);
	*ptr = string_buffer;
}

void get_audio_value_str(AudioControl control, char** ptr) {
	int16_t current_value = control_value[control];

	switch (control) {
	case AUDIO_CONTROL_BASS:
		format_db(current_value, ptr);
		break;

	case AUDIO_CONTROL_TREB:
		format_db(current_value, ptr);
		break;

	case AUDIO_CONTROL_BASS_FREQ:
		*ptr = bass_freqs[current_value];
		break;

	case AUDIO_CONTROL_TREB_FREQ:
		*ptr = treb_freqs[current_value];
		break;

	}
}
