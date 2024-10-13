#include "DAC_sine_wave_gen.h"

void DAC_pinMode_B0_B7(uint8_t _pinmode0)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // Enable clock for GPIOB
  // This frees PB3 and PB4 for general-purpose I/O
  AFIO->MAPR &= ~(AFIO_MAPR_SWJ_CFG); // Clear the SWJ_CFG bits
  AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1;  // Set SWJ_CFG to disable JTAG but keep SWD

  GPIOB->CRL &= ~(0xFFFFFFFF); // Clear configuration for PB0-PB7
  uint32_t _data0 = 0x0;
  for (int c = 0; c < 8; c++)
    _data0 |= (_pinmode0 << (c * 4));
  GPIOB->CRL |= _data0; // Set PB0-PB7 Mode
  //----
}

void DAC_analogWrite_B0_B7(uint8_t _dat1)
{
  GPIOB->ODR = (GPIOB->ODR & 0xFFFFFF00) | (_dat1);
}

void DAC_sine_wave(uint8_t _frequency0)
{
  DAC_analogWrite_B0_B7(_frequency0);
}