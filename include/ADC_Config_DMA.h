/**
 * @file ADC_Config_DMA.h
 * @brief ADC Configuration with DMA
 * This file contains the function definition for ADC Configuration with DMA
 *
 * Developed by: Jaishankar M
 */
#ifndef ADC_CONFIG_DMA_H
#define ADC_CONFIG_DMA_H

// Include Header Files
#include "stm32f1xx_hal.h"

// Public Variable Declaration
#ifndef DMA_ADC_data_length
#define DMA_ADC_data_length 50
#endif
extern uint8_t adc_PA0_data_ready_flag;
extern int16_t adc_pa0_DMA_data[DMA_ADC_data_length];

// Public Function Declaration
void setup_ADC_with_DMA();
void set_ADC_Measure_window(uint16_t _measure_frequency);
void Start_ADC_Conversion();

#endif // End of ADC_CONFIG_DMA_H