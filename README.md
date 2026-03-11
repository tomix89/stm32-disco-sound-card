# stm32-disco-sound-card
An UAC1 implementation for the STM32F411-DISCO board with **tone control**!\
What i wanted to achive a long time for now to have a ligh weight USB DAC for my headphones which does not need any special driver and have hardware physical tone contol in the digital domain.\
For this the STM32F411-DISCO board which i had for years seemed to be a really good candidate with it's CS43L22 audio headphone DAC.
The project aims for a generic 48kHz 24bit stereo implementation.

This project heavily relyes on the tiny USB project:\
https://docs.tinyusb.org/en/latest \
https://github.com/hathach/tinyusb \
Big thank you for the developers! It is a lot better and in depth impelemntation then the middle ware from STM itself.

The project is made by the STM Cube MX, and programmed in their STM32CubeIDE 2.0.0

There is a hand written tool to calculate the best (exact) configuration for the I2S PLL, as STM cube seemed to not support this
tools\i2s-clock-calc.py

Current state of the prototype:
<table>
  <tr>
    <td>
      <img src="images/UI1.jpg">
    </td>
    <td>
      <img src="images/UI2.jpg">
    </td>
  </tr>
  <tr>
    <td colspan="2" align="center">
      <img src="images/board.jpg" width="50%">
    </td>
  </tr>
</table>
