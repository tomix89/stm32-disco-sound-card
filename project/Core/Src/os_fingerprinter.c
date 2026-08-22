/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Jerzy Kasenberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <string.h>

//#include "bsp/board_api.h"
//#include "common_types.h"
//#include "tusb.h"
//#include "usb_descriptors.h"
//#include "usb_handler.h"

//#include "main.h"
//#include "CS43L22_driver.h"

#include "class/audio/audio.h"
#include "os_fingerprinter.h"

#define LOG_SIZE_TGT    10
#define pack_data(x,y,z) ( (((uint8_t)(x))<<16) | (((uint8_t)(y))<<8) | ((uint8_t)(z)) )


static uint32_t usbTransactions[LOG_SIZE_TGT];
static uint8_t curr_action_id = 0;

static uint8_t isAndroid = 0;

static const uint32_t androidTransactions[LOG_SIZE_TGT] = {
		pack_data(USB_ACTION_EP, AUDIO10_EP_CTRL_SAMPLING_FREQ, AUDIO10_CS_REQ_SET_CUR),
		pack_data(USB_ACTION_EP, AUDIO10_EP_CTRL_SAMPLING_FREQ, AUDIO10_CS_REQ_GET_CUR),

		pack_data(USB_ACTION_ENTT, AUDIO10_FU_CTRL_VOLUME, AUDIO10_CS_REQ_GET_MAX),
		pack_data(USB_ACTION_ENTT, AUDIO10_FU_CTRL_VOLUME, AUDIO10_CS_REQ_GET_MIN),
		pack_data(USB_ACTION_ENTT, AUDIO10_FU_CTRL_VOLUME, AUDIO10_CS_REQ_GET_RES),
		pack_data(USB_ACTION_ENTT, AUDIO10_FU_CTRL_VOLUME, AUDIO10_CS_REQ_GET_RES),

		pack_data(USB_ACTION_ENTT, AUDIO10_FU_CTRL_VOLUME, AUDIO10_CS_REQ_GET_CUR),
		pack_data(USB_ACTION_ENTT, AUDIO10_FU_CTRL_VOLUME, AUDIO10_CS_REQ_SET_CUR),
		pack_data(USB_ACTION_ENTT, AUDIO10_FU_CTRL_VOLUME, AUDIO10_CS_REQ_GET_CUR),
		pack_data(USB_ACTION_ENTT, AUDIO10_FU_CTRL_VOLUME, AUDIO10_CS_REQ_SET_CUR),
};



void evaluate_log(void) {
	isAndroid = (memcmp(androidTransactions, usbTransactions, LOG_SIZE_TGT*sizeof(uint32_t)) != 0);
}

void log_usb_command(UsbActionType_t type, uint8_t feature, uint8_t request) {

	if (curr_action_id >= LOG_SIZE_TGT) {
		return;
	}

	printf("----->  fingerprint: %u, %u, %u\r\n", type, feature, request);

	usbTransactions[curr_action_id] = pack_data(type, feature, request);
	curr_action_id++;

	if (curr_action_id >= LOG_SIZE_TGT) {
		evaluate_log();
		// do a callback if android -> set max volume on DAC
		// set a bool flag for the UI
	}
}
