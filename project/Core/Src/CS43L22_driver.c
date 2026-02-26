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
#include <stdio.h>

#define CODEC_I2C_ADDR 0x94

static I2C_HandleTypeDef *hi2c;
static I2S_HandleTypeDef *hi2s;

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define MASTER_MAX_VOLUME_DB 12
#define MASTER_MIN_VOLUME_DB -102

#define OUTPUT_DEVICE_HEADPHONE   0xAF
#define OUTPUT_DEVICE_OFF         0xFF

// CS43l22 Registers
// from https://github.com/STMicroelectronics/stm32-cs43l22
#define   CS43L22_REG_ID                  0x01
#define   CS43L22_REG_POWER_CTL1          0x02
#define   CS43L22_REG_POWER_CTL2          0x04
#define   CS43L22_REG_CLOCKING_CTL        0x05
#define   CS43L22_REG_INTERFACE_CTL1      0x06
#define   CS43L22_REG_INTERFACE_CTL2      0x07
#define   CS43L22_REG_PASSTHR_A_SELECT    0x08
#define   CS43L22_REG_PASSTHR_B_SELECT    0x09
#define   CS43L22_REG_ANALOG_ZC_SR_SETT   0x0A
#define   CS43L22_REG_PASSTHR_GANG_CTL    0x0C
#define   CS43L22_REG_PLAYBACK_CTL1       0x0D
#define   CS43L22_REG_MISC_CTL            0x0E
#define   CS43L22_REG_PLAYBACK_CTL2       0x0F
#define   CS43L22_REG_PASSTHR_A_VOL       0x14
#define   CS43L22_REG_PASSTHR_B_VOL       0x15
#define   CS43L22_REG_PCMA_VOL            0x1A
#define   CS43L22_REG_PCMB_VOL            0x1B
#define   CS43L22_REG_BEEP_FREQ_ON_TIME   0x1C
#define   CS43L22_REG_BEEP_VOL_OFF_TIME   0x1D
#define   CS43L22_REG_BEEP_TONE_CFG       0x1E
#define   CS43L22_REG_TONE_CTL            0x1F
#define   CS43L22_REG_MASTER_A_VOL        0x20
#define   CS43L22_REG_MASTER_B_VOL        0x21
#define   CS43L22_REG_HEADPHONE_A_VOL     0x22
#define   CS43L22_REG_HEADPHONE_B_VOL     0x23
#define   CS43L22_REG_SPEAKER_A_VOL       0x24
#define   CS43L22_REG_SPEAKER_B_VOL       0x25
#define   CS43L22_REG_CH_MIXER_SWAP       0x26
#define   CS43L22_REG_LIMIT_CTL1          0x27
#define   CS43L22_REG_LIMIT_CTL2          0x28
#define   CS43L22_REG_LIMIT_ATTACK_RATE   0x29
#define   CS43L22_REG_OVF_CLK_STATUS      0x2E
#define   CS43L22_REG_BATT_COMPENSATION   0x2F
#define   CS43L22_REG_VP_BATTERY_LEVEL    0x30
#define   CS43L22_REG_SPEAKER_STATUS      0x31
#define   CS43L22_REG_TEMPMONITOR_CTL     0x32
#define   CS43L22_REG_THERMAL_FOLDBACK    0x33
#define   CS43L22_REG_CHARGE_PUMP_FREQ    0x34


static uint8_t i2s_stream_state = I2S_AUDIO_STOPPED;

typedef enum {
	SEND_2ND_HALF_FILL_1ST = 1,
	SEND_1ST_HALF_FILL_2ND = 2
} BuffStatus;
BuffStatus buffStatus = SEND_1ST_HALF_FILL_2ND;


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

int audio_set_hp_volume_db(int16_t db_256) {
  db_256 = MIN(db_256, HP_MAX_VOLUME_DB << 8);
  db_256 = MAX(db_256, HP_MIN_VOLUME_DB << 8);

#ifdef DEBUG
  printf("setting vol: %d, %d.%d db\n", db_256, db_256/256, db_256/128*5);
#endif

  // CS43L22 has a 0.5db resolution
  uint8_t value = db_256 >> 7;
  uint8_t success = 0;
  success += codec_i2c_write_reg(CS43L22_REG_HEADPHONE_A_VOL, value);
  success += codec_i2c_write_reg(CS43L22_REG_HEADPHONE_B_VOL, value);

  // if there was any error it will be non zero
  return success != 0;
}

int audio_set_hp_mute(uint8_t mute) {
	  uint8_t value = mute > 0 ? 0b11000000 : 0b00000000;
	  uint8_t success = 0;
	  success += codec_i2c_write_reg(CS43L22_REG_PLAYBACK_CTL2, value);

	  // if there was any error it will be non zero
	  return success != 0;
}

int audio_set_bass_treb_gain(uint8_t bass, uint8_t treb) {
	uint8_t value = (treb & 0x0F)<<4 | (bass & 0x0F);
	uint8_t success = 0;
	success += codec_i2c_write_reg(CS43L22_REG_TONE_CTL, value);

	// if there was any error it will be non zero
	return success != 0;
}


int audio_set_bass_treb_freq(uint8_t bass_id, uint8_t treb_id) {
	uint8_t value = 0x01; // beep off, beep mix disabled, tone control on
	value |= (treb_id & 0x03)<<3 | (bass_id & 0x03)<<1;
	uint8_t success = 0;
	success += codec_i2c_write_reg(CS43L22_REG_BEEP_TONE_CFG, value);

	// if there was any error it will be non zero
	return success != 0;
}

//--------------------------------------------------------------------
//----------------- only for used internally in CS43L22_driver -------
//--------------------------------------------------------------------

int audio_set_master_volume_db(int16_t db_256) {
  db_256 = MIN(db_256, MASTER_MAX_VOLUME_DB << 8);
  db_256 = MAX(db_256, MASTER_MIN_VOLUME_DB << 8);

  // CS43L22 has a 0.5db resolution
  uint8_t value = db_256 >> 7;
  uint8_t success = 0;
  success += codec_i2c_write_reg(CS43L22_REG_MASTER_A_VOL, value);
  success += codec_i2c_write_reg(CS43L22_REG_MASTER_B_VOL, value);

  // if there was any error it will be non zero
  return success != 0;
}

// do not use master mute as it has also the analog gain in the same register
// will be hard to do write only
int audio_set_pcm_mute(uint8_t mute) {
	  uint8_t value = mute > 0 ? 0b10000000 : 0b00000000;
	  uint8_t success = 0;
	  success += codec_i2c_write_reg(CS43L22_REG_PCMA_VOL, value);
	  success += codec_i2c_write_reg(CS43L22_REG_PCMA_VOL, value);

	  // if there was any error it will be non zero
	  return success != 0;
}

//-------------------------------------------------------------------------------------------------------
//---------------------------- high level control -----------------------------------------------------
//-------------------------------------------------------------------------------------------------------

void audio_play() {
	i2s_stream_state = I2S_AUDIO_STREAMING;
	HAL_GPIO_WritePin(LED_Orange_GPIO_Port, LED_Orange_Pin, GPIO_PIN_SET);

	if (isFirst) {
		isFirst = 0;
		HAL_I2S_Transmit_DMA(hi2s, i2s_audio_buffer, TOTAL_AUDIO_SAMPLES);
	}
	audio_set_pcm_mute(0);
}

void audio_stop() {
	i2s_stream_state = I2S_AUDIO_STOPPED;
	HAL_GPIO_WritePin(LED_Orange_GPIO_Port , LED_Orange_Pin, GPIO_PIN_RESET);

	audio_set_pcm_mute(1);

	// 1. Mute the DAC's and PWM outputs
	// 2. Disable soft ramp and zero cross volume transitions.
	// 3. Set the "Power Ctl 1" register (0x02) to 0x9F.
	// 4. Wait at least 100 μs.
	// 5. MCLK may be removed at this time.

	// we cant's stop the DMA immediately, because CS43L22
	// needs MCLK and LRCLK to set the volume/mute (it has some ramping)
	// check 4.10 Recommended Power-Down Sequence of the CS43L22 data sheet
	// for now it states that a fully powered peripheral consumes 25mW
	// HAL_I2S_DMAStop(hi2s);

	 memset(i2s_audio_buffer, 0, TOTAL_AUDIO_SAMPLES*2); // *2 because memset() sets in bytes
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

    // since we are not stopping the I2S it will deplete the USB FIFO fully
    // but additionally it will also slow the refill significantly
    // so take samples out of the USB FIFO only when really playing
    if (i2s_stream_state == I2S_AUDIO_STOPPED) {
    	return;
    }

	// this reads in bytes
	tud_audio_read(&i2s_audio_buffer[I2S_BUFF_OFFS], bytesToRead);


	{
		// really basic signal level detection
		static uint8_t pwm_counter = 0;
		static int32_t average = 0;
		int16_t max = 0;
		for (int i=0; i<SAMP_ALL_CHANNELS; ++i) {
			max = MAX((int16_t)i2s_audio_buffer[I2S_BUFF_OFFS+i], max);
		}
		average = (9 * average + max) / 10;

		// Increase counter and wrap at 10
		pwm_counter++;
		if (pwm_counter >= 10) {
			pwm_counter = 0;
		}

		HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, average/1200 > pwm_counter ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
	buffStatus = SEND_2ND_HALF_FILL_1ST;
    loadMore();
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
	buffStatus = SEND_1ST_HALF_FILL_2ND;
    loadMore();
}


