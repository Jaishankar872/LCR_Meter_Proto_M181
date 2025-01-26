/**
 * @file DAC_sine_wave_gen.h
 * @brief DAC Sine Wave Generator
 * This file contains the function definition for DAC Sine Wave Generator
 * Via Timer 1 Interrupt
 */

#ifndef DAC_SINE_WAVE_GEN_H
#define DAC_SINE_WAVE_GEN_H

// Include Header Files
#include "stm32f1xx_hal.h"

// Timer
extern TIM_HandleTypeDef htim1;
uint16_t _timer1_prescaler = 2;

// DAC Sine data generated
int sine_data[100];
uint16_t _no_of_sample_per_sine = 100;
uint16_t DAC_resolution = 256; // 2^8=256;
int _pos_sine_data = 0;

// Function Declaration
void sine_wave_setup();
void generate_sine_wave_data();
void DAC_pinMode_B0_B7(uint8_t _pinmode0);
void DAC_analogWrite_B0_B7(uint8_t _dat1);
void set_sine_wave_frequency(uint16_t _set_frequency);
void timer1_setup(void);
extern void Error_Handler(void);
#endif
// End