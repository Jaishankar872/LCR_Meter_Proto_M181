/**
 * @file DAC_sine_wave_gen.h
 * @brief DAC Sine Wave Generator
 * This file contains the function definition for DAC Sine Wave Generator
 * Via Timer 1 Interrupt
 * 
 * @author Jaishankar M
 */

#ifndef DAC_SINE_WAVE_GEN_H
#define DAC_SINE_WAVE_GEN_H

// Include Header Files
#include "stm32f1xx_hal.h"
#include "system_data.h"

// Public Function Declaration
void sine_wave_setup();
void On_Timer3_Interrupt();
void manual_ctrl_DAC(uint8_t _dac_output);
void release_manual_ctrl_DAC();

#endif
// End