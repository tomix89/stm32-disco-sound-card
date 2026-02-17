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
#pragma once

#include <stdint.h>

typedef enum {
  I2S_AUDIO_STOPPED = 0,
  I2S_AUDIO_STREAMING = 1,
}I2sAudioState;


#define HP_MAX_VOLUME_DB        0
#define HP_MIN_VOLUME_DB       -102
#define HP_VOLUME_RESOLUTION_DB 0.5

int audio_init(void *i2c, void *i2s);

void audio_play();
void audio_stop();
I2sAudioState get_audio_state();

// set volume has a 1/256th of a db resolution
// e.g. +10db => 2560
//      -25db => -6400
int audio_set_hp_volume_db(int16_t db_256);
int audio_set_hp_mute(uint8_t mute);

