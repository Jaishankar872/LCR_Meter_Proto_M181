// system_data.h
#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <stdint.h> // For uint8_t, uint16_t, etc.

// Firmware Version Global Variable
#define fw_version 0.15

// Define the struct
typedef struct system_data
{
    uint8_t hold_btn, sp_btn, rcl_btn;
    uint16_t set_freq;
    uint8_t led_state;
    int8_t VI_measure_mode;
    float pk_pk_voltage, pk_pk_AFC_volt, pk_pk_current, pk_pk_AFC_current;
    uint8_t uart_all_print_DSO;
    uint8_t adc_measure_status; // 1 -> Start Voltage, 2 -> Start Current, 3 -> Both are Ready
} system_data;

// Declare the global variable
extern system_data process_data;
#define DMA_ADC_data_length 64 // n=6; Selected Length is 2^n
int16_t adc_Current_data[DMA_ADC_data_length];
int16_t adc_Volt_data[DMA_ADC_data_length];
int16_t AFC_adc_Current_data[DMA_ADC_data_length];
int16_t AFC_adc_Volt_data[DMA_ADC_data_length];

int16_t adc_Current_data_ZC[DMA_ADC_data_length];
int16_t adc_Volt_data_ZC[DMA_ADC_data_length];
int16_t AFC_adc_Current_data_ZC[DMA_ADC_data_length];
int16_t AFC_adc_Volt_data_ZC[DMA_ADC_data_length];

#endif // SYSTEM_DATA_H
