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
#include "CS43L22_driver.h"
#include <stdio.h> // sprintf()
#include <stdlib.h> // abs()

#define DIVISOR 	10
// these all are x10 (DIVISOR) of the real world value so we can have 0.5 and still somewhat human readable
#define TONE_MAX 	120  // +12.0dB
#define TONE_MIN   -105  // -10.5dB
#define TONE_STEP    15  //   1.5dB


// the -102dB range is too much, on half the slider there is almost no sound at all
// so do not go that low
#define SYSTEM_MAX_VOLUME_DB	  0
#define SYSTEM_MIN_VOLUME_DB	-80

// when negative then we decrease L channel
// when positive then we decrease R channel
#define BLNC_MAX         400  //  40.0dB
#define BLNC_MIN        -400  // -40.0dB
#define BLNC_BASE_STEP     5  //   0.5dB

#define TONE_FREQ_CNT	4

int16_t control_value[AUDIO_CONTROL_CNT];

const char *const bass_freqs[TONE_FREQ_CNT] = { " 50Hz", "100Hz", "200Hz", "250Hz" };

const char *const treb_freqs[TONE_FREQ_CNT] = { " 5kHz", " 7kHz", "10kHz", "15kHz" };

char string_buffer[8]; // for the audio values as string

// internal DIVISOR offseted value comes in
static inline uint8_t convert_to_tone_gain(int16_t value) {
	// the values are: 0b0000 -> +12.0dB
	//                        ...
	//                 0b0111 ->  +1.5dB
	//                 0b1000 ->   0.0dB
	//                 0b1001 ->  -1.5dB
	//                        ...
	//                 0b1111 -> -10.5dB

	return (TONE_MAX - value) / 15;
}

static void send_volume_with_blnc(void) {
	int16_t vol_L = control_value[AUDIO_CONTROL_VOLUME];
	int16_t vol_R = control_value[AUDIO_CONTROL_VOLUME];
	int16_t blnc = control_value[AUDIO_CONTROL_BALANCE];

	if (blnc < 0) {
		vol_R -= -blnc;
	} else if (blnc > 0) {
		vol_L -= blnc;
	}

	// scale it to 0.5dB step format for CS43L22
	CS43L22_set_hp_volume_db(
			vol_L / (DIVISOR / 2),
			vol_R / (DIVISOR / 2));
}

static void update_audio_codec(AudioControl control) {
	switch (control) {
	case AUDIO_CONTROL_MUTE:
		CS43L22_set_hp_mute(control_value[AUDIO_CONTROL_MUTE]);
		break;

	case AUDIO_CONTROL_BASS:
	case AUDIO_CONTROL_TREB:
		CS43L22_set_bass_treb_gain(
				convert_to_tone_gain(control_value[AUDIO_CONTROL_BASS]),
				convert_to_tone_gain(control_value[AUDIO_CONTROL_TREB]));
		break;

	case AUDIO_CONTROL_BASS_FREQ:
	case AUDIO_CONTROL_TREB_FREQ:
		CS43L22_set_bass_treb_freq(control_value[AUDIO_CONTROL_BASS_FREQ],
				control_value[AUDIO_CONTROL_TREB_FREQ]);
		break;

	case AUDIO_CONTROL_VOLUME:
	case AUDIO_CONTROL_BALANCE:
		send_volume_with_blnc();
		break;
	}
}

_Static_assert(USB_MIN_VOLUME_PCT == 0, "Can't work with negative percent");
// converts from the USB pct mapped custom values to internal dB with DIVISOR offset
static inline int16_t vol_usb_pct_to_db_div(int16_t volume_pct) {
	// we already have a nice range 0 to USB_MAX_VOLUME_PCT
	// and use this to scale from SYSTEM_MIN_VOLUME_DB to SYSTEM_MAX_VOLUME_DB
	int32_t db_div = SYSTEM_MIN_VOLUME_DB * DIVISOR + ((int32_t)volume_pct * (SYSTEM_MAX_VOLUME_DB - SYSTEM_MIN_VOLUME_DB) / (USB_MAX_VOLUME_PCT / DIVISOR));
	return (int16_t)db_div;
}

// scale from internal dB DIVISOR to USB percent
static inline int16_t vol_db_div_to_usb_pct(int16_t volume_db_div) {
	 int32_t pct = USB_MAX_VOLUME_PCT * ((int32_t)volume_db_div - SYSTEM_MIN_VOLUME_DB * DIVISOR) / ((SYSTEM_MAX_VOLUME_DB - SYSTEM_MIN_VOLUME_DB) * DIVISOR);
	 return (int16_t)pct;
}

void audio_set_volume_usb_pct(int16_t volume_pct) {
	// volume is special. HW supports -102dB +12dB/0dB range, but on USB level we map it to 0-100% range with 0.5% step
	// internally we convert it to the dB with DIVISOR
	control_value[AUDIO_CONTROL_VOLUME] = vol_usb_pct_to_db_div(volume_pct);
	printf("volume is: %d\n", control_value[AUDIO_CONTROL_VOLUME]);
	update_audio_codec(AUDIO_CONTROL_VOLUME);
}

int16_t audio_get_volume_usb_pct(void) {
	return vol_db_div_to_usb_pct(control_value[AUDIO_CONTROL_VOLUME]);
}

void audio_set_mute(int8_t mute) {
	control_value[AUDIO_CONTROL_MUTE] = mute;
	update_audio_codec(AUDIO_CONTROL_MUTE);
}

int8_t audio_get_mute(void) {
	return control_value[AUDIO_CONTROL_MUTE];
}

static int16_t get_blnc_step() {
	int16_t curr_blnc = abs(control_value[AUDIO_CONTROL_BALANCE]);
	if (curr_blnc < 10 * DIVISOR) {
		return 	BLNC_BASE_STEP;
	} else if (curr_blnc < 20 * DIVISOR) {
		return BLNC_BASE_STEP * 2;
	} else {
		return BLNC_BASE_STEP * 4;
	}
}

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

	case AUDIO_CONTROL_BALANCE:
		control_value[control] += get_blnc_step();
		if (control_value[control] > BLNC_MAX) {
			control_value[control] = BLNC_MAX;
		}
		break;

	default:
		return;
	}

	update_audio_codec(control);
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

	case AUDIO_CONTROL_BALANCE:
		control_value[control] -= get_blnc_step();
		if (control_value[control] < BLNC_MIN) {
			control_value[control] = BLNC_MIN;
		}
		break;

	default:
		return;
	}

	update_audio_codec(control);
}

void audio_init() {
	control_value[AUDIO_CONTROL_VOLUME] = SYSTEM_MAX_VOLUME_DB * DIVISOR;
	control_value[AUDIO_CONTROL_BASS] = 1.5 * 4 * DIVISOR; // 1.5 step x 4 = +6dB
	control_value[AUDIO_CONTROL_TREB] = 1.5 * 2 * DIVISOR; // 1.5 step x 2 = +3dB
	control_value[AUDIO_CONTROL_BASS_FREQ] = 1; // 100Hz


	update_audio_codec(AUDIO_CONTROL_VOLUME);
	// both BASS and TREB is in a same register so they both will be updated
	update_audio_codec(AUDIO_CONTROL_BASS);
	update_audio_codec(AUDIO_CONTROL_BASS_FREQ);
}

//------------------------------------------------------------------------------
//-------------------------- string formatters for UI --------------------------
//------------------------------------------------------------------------------

// format value to dB
static inline void format_db(int16_t current_value, char **ptr) {
	int16_t front = current_value / DIVISOR;
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

inline int16_t get_audio_value(AudioControl control) {
	return control_value[control];
}

void get_audio_value_str(AudioControl control, char **ptr) {
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

	case AUDIO_CONTROL_BALANCE:
		// this is always a negative
		format_db(-abs(current_value), ptr);
		break;

	default:
		*ptr = 0;
	}
}
