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

#define CODEC_I2C_ADDR 0x94

static I2C_HandleTypeDef *hi2c;

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define MASTER_MAX_VOLUME_DB 12
#define MASTER_MIN_VOLUME_DB -102

#define OUTPUT_DEVICE_HEADPHONE   0xAF

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
//---------------------------- low level I2C access -----------------------------------------------------
//-------------------------------------------------------------------------------------------------------

int audio_init(void *i2c) {
	uint8_t success = 0;
	hi2c = i2c;

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

  uint8_t value = 2*db;
  uint8_t success = 0;
  success += codec_i2c_write_reg(CS43L22_REG_MASTER_A_VOL, value);
  success += codec_i2c_write_reg(CS43L22_REG_MASTER_B_VOL, value);

  // if there was any error it will be non zero
  	return success != 0;
}

int audio_set_master_volume_pct(uint8_t percent) {
  percent = MIN(percent, 100);

  int8_t db = MASTER_MIN_VOLUME_DB + (percent * MASTER_MAX_VOLUME_DB) / 100;
  return audio_set_master_volume_db(db) != 0;
}
