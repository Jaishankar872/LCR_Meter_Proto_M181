#ifndef DAC_SINE_WAVE_GEN_H
#define DAC_SINE_WAVE_GEN_H

#include "Arduino.h"

void DAC_analogWrite_B0_B7(uint8_t _dat1);
void DAC_pinMode_B0_B7(uint8_t _pinmode0);
void DAC_sine_wave(uint8_t _frequency0);

#endif
//End