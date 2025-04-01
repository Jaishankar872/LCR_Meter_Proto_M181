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
#ifndef DMA_ADC_data_length
#define DMA_ADC_data_length 50
#endif
extern int16_t adc_Current_data[DMA_ADC_data_length];
extern int16_t adc_Volt_data[DMA_ADC_data_length];
extern int16_t AFC_adc_Current_data[DMA_ADC_data_length];
extern int16_t AFC_adc_Volt_data[DMA_ADC_data_length];

// Public Variable Declaration

// Public Function Declaration
void setup_ADC_with_DMA();
void set_ADC_Measure_window(uint16_t _measure_frequency);
uint8_t get_measure_status();
float adc_volt_convert(int16_t raw_adc);
uint8_t ADC_recapture_data();
void On_Timer2_Interrupt();

#endif // End of ADC_CONFIG_DMA_H