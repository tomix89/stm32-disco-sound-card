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

SSD1306_t SSD1306_Disp;

void ui_task(void) {
	    static int xPos = 5;
	    static uint32_t start_ms = 0;

	    uint32_t curr_ms = HAL_GetTick();
	    if (start_ms == curr_ms) {
	    	return; // not enough time passed
	    }
	    start_ms = curr_ms;

	    /* Only write to buffer when not in transmission */
	    if (SSD1306_Disp.state == SSD1306_STATE_READY)
	    {
	      /* These blocking calls will write data to buffer */
	      SSD1306_Fill_ToRight(xPos - 1, SSD1306_PX_CLR_BLACK);

	      SSD1306_GotoXY(xPos, 5);
	      SSD1306_Puts("Hello", &Font_7x10, SSD1306_PX_CLR_WHITE);
	      SSD1306_GotoXY(xPos, 21);
	      SSD1306_Puts("World", &Font_7x10, SSD1306_PX_CLR_WHITE);
	    }

	    /* Update the ssd1306 display in non-blocking mode -> should return SSD1306_STATE_READY if successful */
	    if (SSD1306_UpdateScreen() == SSD1306_SPI_ERROR)
	    {
	      /* Program enters here only when HAL_SPI_Transmit_DMA function call fails */
	      Error_Handler();
	    };

	    xPos++;
	    if (xPos == 80)
	    {
	      xPos = 5;
	    }

}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  /* Set the SSD1306 state to ready */
  SSD1306_Disp.state = SSD1306_STATE_READY;
}
