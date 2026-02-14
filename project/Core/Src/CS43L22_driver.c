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

#include "CS43L22_driver.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "tusb.h"

#define CODEC_I2C_ADDR 0x94

static I2C_HandleTypeDef *hi2c;
static I2S_HandleTypeDef *hi2s;

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define MASTER_MAX_VOLUME_DB 12
#define MASTER_MIN_VOLUME_DB -102

#define OUTPUT_DEVICE_HEADPHONE   0xAF
#define OUTPUT_DEVICE_OFF         0xFF

static uint8_t i2s_stream_state = I2S_AUDIO_STOPPED;

typedef enum {
	SEND_2ND_HALF_FILL_1ST = 1,
	SEND_1ST_HALF_FILL_2ND = 2
} BuffStatus;
BuffStatus buffStatus = SEND_1ST_HALF_FILL_2ND;

#define SAMPLING_RATE	48000
// data comes each 1ms -> at 48KHz it will be 48 samples per channel
#define SAMP_PER_CHANNEL	   48
#define SAMP_ALL_CHANNELS      2 * SAMP_PER_CHANNEL // we have 2 channels
#define TOTAL_AUDIO_SAMPLES    2 * SAMP_ALL_CHANNELS  // we have circular double buffering
// Note: this is uint_16 sample size! byte size is 2x

// this is the actual DMA buffer
uint16_t i2s_audio_buffer[TOTAL_AUDIO_SAMPLES];

static uint8_t isFirst = 1;

//-------------------------------------------------------------------------------------------------------
//---------------------------- low level I2C access -----------------------------------------------------
//-------------------------------------------------------------------------------------------------------

// this is fairly quick but still blocking - ToDo make it unblocking
HAL_StatusTypeDef codec_i2c_write_reg(uint8_t addr, uint8_t val) {
	return HAL_I2C_Master_Transmit(hi2c, CODEC_I2C_ADDR,
			(uint8_t[]) { addr, val }, 2, 1000);
}

// this is blocking for even more
HAL_StatusTypeDef codec_i2c_read_reg(uint8_t addr, uint8_t *val) {
	HAL_StatusTypeDef result = HAL_I2C_Master_Transmit(hi2c, CODEC_I2C_ADDR,
			(uint8_t[] ) { addr }, 1, 1000);
	if (result != HAL_OK) {
		return 1;
	}

	return HAL_I2C_Master_Receive(hi2c, CODEC_I2C_ADDR, val, 1, 1000);
}


//-------------------------------------------------------------------------------------------------------
//---------------------------- middle level control -----------------------------------------------------
//-------------------------------------------------------------------------------------------------------

int audio_init(void *i2c, void *i2s) {
	uint8_t success = 0;
	hi2c = i2c;
	hi2s = i2s;

	HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(100);

	// keep codec powered OFF
	success += codec_i2c_write_reg(CS43L22_REG_POWER_CTL1, 0x01);

	// set output device
	success += codec_i2c_write_reg(CS43L22_REG_POWER_CTL2, OUTPUT_DEVICE_HEADPHONE);

	// clock configuration: auto detection
	success += codec_i2c_write_reg(CS43L22_REG_CLOCKING_CTL, 0x80);

	// set the slave mode and the audio standard
	uint8_t data = 0;
	data |= 0b00000011; // 16bit mode
	data |= 0b00000100; // I2S format
	success += codec_i2c_write_reg(CS43L22_REG_INTERFACE_CTL1, data);

	// register settings are loaded, re-apply power
	success += codec_i2c_write_reg(CS43L22_REG_POWER_CTL1, 0x9E);

	// if there was any error it will be non zero
	return success != 0;
}

int audio_set_master_volume_db(int8_t db) {
  db = MIN(db, MASTER_MAX_VOLUME_DB);
  db = MAX(db, MASTER_MIN_VOLUME_DB);

  // CS43L22 has a 0.5db resolution
  uint8_t value = 2*db;
  uint8_t success = 0;
  success += codec_i2c_write_reg(CS43L22_REG_MASTER_A_VOL, value);
  success += codec_i2c_write_reg(CS43L22_REG_MASTER_B_VOL, value);

  // if there was any error it will be non zero
  	return success != 0;
}

//-------------------------------------------------------------------------------------------------------
//---------------------------- high level control -----------------------------------------------------
//-------------------------------------------------------------------------------------------------------

void audio_play() {
	i2s_stream_state = I2S_AUDIO_STREAMING;

	if (isFirst) {
		isFirst = 0;
		HAL_I2S_Transmit_DMA(hi2s, i2s_audio_buffer, TOTAL_AUDIO_SAMPLES);
	}
	audio_set_master_volume_db(-10);
}

void audio_stop() {
	i2s_stream_state = I2S_AUDIO_STOPPED;

	audio_set_master_volume_db(MASTER_MIN_VOLUME_DB);

	// we cant's stop the DMA immediately, because CS43L22
	// needs MCLK and LRCLK to set the volume/mute (it has some ramping)
	// check 4.10 Recommended Power-Down Sequence of the CS43L22 data sheet
	// for now it states that a fully powered peripheral consumes 25mW
	// HAL_I2S_DMAStop(hi2s);
}

inline I2sAudioState get_audio_state() {
	return i2s_stream_state;
}

//-------------------------------------------------------------------------------------------------------
//---------------------------- I2S DMA callbacks -----------------------------------------------------
//-------------------------------------------------------------------------------------------------------

void loadMore() {
    // add new stuff when available
	const uint16_t bytesToRead = SAMP_ALL_CHANNELS * 2;
    const uint16_t I2S_BUFF_OFFS = buffStatus == SEND_2ND_HALF_FILL_1ST ? 0 : SAMP_ALL_CHANNELS;

	// this reads in bytes
	tud_audio_read(&i2s_audio_buffer[I2S_BUFF_OFFS], bytesToRead);
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
	buffStatus = SEND_2ND_HALF_FILL_1ST;
    loadMore();
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
	buffStatus = SEND_1ST_HALF_FILL_2ND;
    loadMore();
}


