/**
 * @file ADC_Config_DMA.h
 * @brief ADC Configuration with DMA
 * This file contains the function definition for ADC Configuration with DMA
 *
 * @author Jaishankar M
 */
#ifndef ADC_CONFIG_DMA_H
#define ADC_CONFIG_DMA_H

// Include Header Files
#include "stm32f1xx_hal.h"
#include "system_data.h"

// Public Variable Declaration
#ifndef DMA_ADC_DATA_LENGTH
#define DMA_ADC_DATA_LENGTH 50
#endif

// Public Variable Declaration

// Public Function Declaration
void setup_ADC_with_DMA();
void set_ADC_Measure_window(uint16_t _measure_frequency);
float adc_volt_convert(int16_t raw_adc);
// void manual_read_ADC();
// void release_manual_read_ADC();

#endif // End of ADC_CONFIG_DMA_H