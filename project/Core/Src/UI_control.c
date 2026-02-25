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

#include "UI_control.h"
#include "main.h"
#include "ssd1306.h"
#include "audio_controls.h"
#include <stdio.h> // printf()
#include <stdbool.h>

SSD1306_t SSD1306_Disp;

typedef enum {
	BTN_LEFT = 0, BTN_MIDDLE, BTN_RIGHT, BTN_COUNT
} Button;

// Timing thresholds (in ms)
#define PRESS_THRESHOLD  5
#define HOLD_START       350
#define HOLD_REPEAT      120

typedef struct {
	uint16_t press_cntr; // count up, how long the button is pressed
	uint16_t hold_cntr; // count down to next hold event
	bool pressed_event_sent;

} ButtonState;
static ButtonState btn_state[BTN_COUNT];

typedef enum {
	PAGE_BASS = 0, PAGE_TREBLE, PAGE_BASS_FREQ, PAGE_TREBLE_FREQ, PAGE_CNT
} UiPage;
static UiPage active_page = PAGE_BASS;

// UiPage has to be in sync with "audio_controls.h" AudioControl
// other ways it needs mapping between the 2
_Static_assert((int)PAGE_BASS == (int)AUDIO_CONTROL_BASS, "UiPage must be in sync with AudioControl");
_Static_assert((int)PAGE_TREBLE == (int)AUDIO_CONTROL_TREB, "UiPage must be in sync with AudioControl");
_Static_assert((int)PAGE_BASS_FREQ == (int)AUDIO_CONTROL_BASS_FREQ, "UiPage must be in sync with AudioControl");
_Static_assert((int)PAGE_TREBLE_FREQ == (int)AUDIO_CONTROL_TREB_FREQ, "UiPage must be in sync with AudioControl");

// when a page is not selected the BTN_LEFT / BTN_RIGHT goes to the next Page
// when a page is selected the value can be changed with BTN_LEFT / BTN_RIGHT
bool is_page_selected = false;

static void key_pressed(Button btn);
static void key_hold(Button btn);

static inline void print_us() {
	uint32_t ticks_per_us = SystemCoreClock / 1000000;
	uint32_t elapsed_us = (SysTick->LOAD - SysTick->VAL) / ticks_per_us;

	uint32_t curr_ms = HAL_GetTick();
	printf("%lu.%04lu ", curr_ms, elapsed_us);
}

// Call this every 1 ms
static void update_button(GPIO_TypeDef *port, uint16_t pin, uint8_t btn_id) {
	// buttons are on pull up -> active low
	bool is_down = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET);

	if (is_down) {
		if (!btn_state[btn_id].pressed_event_sent) {
			btn_state[btn_id].press_cntr++;

			// Trigger "pressed" event once after PRESS_THRESHOLD
			if (btn_state[btn_id].press_cntr >= PRESS_THRESHOLD) {
				key_pressed(btn_id);
				btn_state[btn_id].pressed_event_sent = true;
				btn_state[btn_id].hold_cntr = HOLD_START; // start count down
			}
		} else {
			// Handle hold events
			btn_state[btn_id].hold_cntr--;

			// First hold event after HOLD_START
			if (btn_state[btn_id].hold_cntr == 0) {
				key_hold(btn_id);
				btn_state[btn_id].hold_cntr = HOLD_REPEAT; // restart for repeat
			}
		}
	} else {
		// Button released -> reset state
		btn_state[btn_id].press_cntr = 0;
		btn_state[btn_id].hold_cntr = 0;
		btn_state[btn_id].pressed_event_sent = false;
	}
}

static void show_page(UiPage page) {
	const uint8_t BACK_CLR =
			is_page_selected ? SSD1306_PX_CLR_WHITE : SSD1306_PX_CLR_BLACK;
	const uint8_t FONT_CLR =
			is_page_selected ? SSD1306_PX_CLR_BLACK : SSD1306_PX_CLR_WHITE;
	char *string_ptr = 0; // for the audio strings

	SSD1306_Fill(BACK_CLR);

	switch (page) {
	case PAGE_BASS:
		SSD1306_GotoXY(1, 0);
		SSD1306_Puts("Bass", &Font_11x18, FONT_CLR);
		SSD1306_GotoXY(0, 20);
		SSD1306_Puts(" gain", &Font_7x10, FONT_CLR);

		SSD1306_GotoXY(50, 7);
		get_audio_value_str(AUDIO_CONTROL_BASS, &string_ptr);
		SSD1306_Puts(string_ptr, &Font_11x18, FONT_CLR);
		break;

	case PAGE_TREBLE:
		SSD1306_GotoXY(1, 0);
		SSD1306_Puts("Treb", &Font_11x18, FONT_CLR);
		SSD1306_GotoXY(0, 20);
		SSD1306_Puts(" gain", &Font_7x10, FONT_CLR);

		SSD1306_GotoXY(50, 7);
		get_audio_value_str(AUDIO_CONTROL_TREB, &string_ptr);
		SSD1306_Puts(string_ptr, &Font_11x18, FONT_CLR);
		break;

	case PAGE_BASS_FREQ:
		SSD1306_GotoXY(1, 0);
		SSD1306_Puts("Bass", &Font_11x18, FONT_CLR);
		SSD1306_GotoXY(0, 20);
		SSD1306_Puts(" freq", &Font_7x10, FONT_CLR);

		SSD1306_GotoXY(70, 7);
		get_audio_value_str(AUDIO_CONTROL_BASS_FREQ, &string_ptr);
		SSD1306_Puts(string_ptr, &Font_11x18, FONT_CLR);
		break;

	case PAGE_TREBLE_FREQ:
		SSD1306_GotoXY(1, 0);
		SSD1306_Puts("Treb", &Font_11x18, FONT_CLR);
		SSD1306_GotoXY(0, 20);
		SSD1306_Puts(" freq", &Font_7x10, FONT_CLR);

		SSD1306_GotoXY(70, 7);
		get_audio_value_str(AUDIO_CONTROL_TREB_FREQ, &string_ptr);
		SSD1306_Puts(string_ptr, &Font_11x18, FONT_CLR);
		break;
	}

	if (SSD1306_UpdateScreen() == SSD1306_SPI_ERROR) {
		/* Program enters here only when HAL_SPI_Transmit_DMA function call fails */
		Error_Handler();
	};
}

static void key_pressed(Button btn) {

	if (is_page_selected) {
		if (btn == BTN_RIGHT) {
			audio_increase(active_page);
		} else if (btn == BTN_LEFT) {
			audio_decrease(active_page);
		}

	} else {
		if (btn == BTN_RIGHT) {
			active_page++;
			if (active_page >= PAGE_CNT) {
				active_page = 0;
			}
		} else if (btn == BTN_LEFT) {
			active_page--;
			if (active_page >= PAGE_CNT) {
				active_page = PAGE_CNT - 1;
			}
		}
	}

	if (btn == BTN_MIDDLE) {
		is_page_selected = !is_page_selected;
	}

	show_page(active_page);
}

static void key_hold(Button btn) {
	if (is_page_selected) {
		if (btn == BTN_RIGHT) {
			audio_increase(active_page);
		} else if (btn == BTN_LEFT) {
			audio_decrease(active_page);
		}

		if (btn != BTN_MIDDLE) {
			show_page(active_page);
		}
	}
}

void ui_task(void) {
	static uint32_t last_ms = 0;
	uint32_t curr_ms = HAL_GetTick();
	if (last_ms == curr_ms)
		return; // not enough time
	last_ms = curr_ms;

	update_button(BTN_USR_L_GPIO_Port, BTN_USR_L_Pin, BTN_LEFT);
	update_button(BTN_USR_M_GPIO_Port, BTN_USR_M_Pin, BTN_MIDDLE);
	update_button(BTN_USR_R_GPIO_Port, BTN_USR_R_Pin, BTN_RIGHT);
}

