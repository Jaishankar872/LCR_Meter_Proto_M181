#ifndef DAC_SINE_WAVE_GEN_H
#define DAC_SINE_WAVE_GEN_H

#include "Arduino.h"
#define CPU_FREQ 72000000 // 48 MHz clock **Need to check
#include <HardwareTimer.h>

// Timer
HardwareTimer timer1_DAC(TIM1); // Initialize timer TIM1
uint16_t _timer1_prescaler = 2;

// DAC Sine data generated
int sine_data[300];
uint16_t _no_of_sample_per_sine = 200;
uint8_t DAC_resolution_bit = 8;
int _pos_sine_data = 0;

//Function Declaration
void generate_sine_wave_data();
void DAC_pinMode_B0_B7(uint8_t _pinmode0);
void DAC_analogWrite_B0_B7(uint8_t _dat1);
void timer1_setup();
void OnTimer1Interrupt();

/*----------------------------------------------------------------------*/

// Function Definition
void generate_sine_wave_data()
{
  int _max_DAC_value = pow(2, DAC_resolution_bit) - 1;
  float _step_value_k = (2 * 3.141 * 1000) / _no_of_sample_per_sine; // x1000

  for (int i = 0; i < _no_of_sample_per_sine; i++)
  {
    float _temp_cal_value = (float)(i * _step_value_k) / 1000;
    sine_data[i] = (_max_DAC_value / 2) + (_max_DAC_value / 2 * sin(_temp_cal_value)); // calculating the sin value at each instance
  }
}

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

void DAC_sine_wave(int _frequency0)
{
  /*
  This Calculation method is not accurate only in mid value 
  Test Result: Set-> 512Hz, Measured -> 555Hz(100Hz & 1KHz are okay)
  NOTE: As of Now, this is enough to continue planned task.
  */
  int time_us = 1000000 / (_frequency0 * _no_of_sample_per_sine); // Microseconds
  uint32_t _temp1_ovf = CPU_FREQ / _timer1_prescaler; // For one seconds time
  _temp1_ovf *= time_us;

  _temp1_ovf /= 1000; // uS -> mS
  _temp1_ovf /= 1000; // mS -> Seconds

  timer1_DAC.setOverflow(_temp1_ovf);
  // timer1_DAC.refresh();
  // timer1_DAC.resume();
}

void timer1_setup()
{
  timer1_DAC.setPrescaleFactor(_timer1_prescaler);
  DAC_sine_wave(100);//timer1_DAC.setOverflow(32761);
  timer1_DAC.attachInterrupt(OnTimer1Interrupt);
  timer1_DAC.refresh();
  timer1_DAC.resume();
}

void OnTimer1Interrupt()
{
  DAC_analogWrite_B0_B7(sine_data[_pos_sine_data]);
  if (_pos_sine_data >= (_no_of_sample_per_sine - 1))
    _pos_sine_data = 0;
  else
    _pos_sine_data++;
}
#endif
// End