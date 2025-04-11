// system_data.h
#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <stdint.h> // For uint8_t, uint16_t, etc.

// Firmware Version Global Variable
#define fw_version 0.19

// Define the struct
typedef struct system_data
{
    uint8_t hold_btn, sp_btn, rcl_btn;
    uint16_t set_freq;
    uint8_t led_state;
    int8_t VI_measure_mode;
    float rms_voltage, rms_AFC_volt, rms_current, rms_AFC_current;
    float voltage_phase, current_phase, VI_phase;
    float capacitance, inductance, resistance;
    int8_t unit_capacitance, unit_inductance, unit_resistance, unit_esr;
    float impedance, esr, tan_delta, QF;
    uint8_t uart_all_print_DSO, LCR_Mode;
    uint8_t adc_measure_status; // 1 -> Start Voltage, 2 -> Start Current, 3 -> Both are Ready
} system_data;

// Declare the global variable
extern system_data process_data;
#define DMA_ADC_data_length 128 // n=6; Selected Length is 2^n
#define ADC_SAMPLE_RATE 64      // 32 samples per cycle
int16_t adc_Current_data[DMA_ADC_data_length];
int16_t adc_Volt_data[DMA_ADC_data_length];
int16_t AFC_adc_Current_data[DMA_ADC_data_length];
int16_t AFC_adc_Volt_data[DMA_ADC_data_length];

int16_t adc_Current_data_ZC[DMA_ADC_data_length];
int16_t adc_Volt_data_ZC[DMA_ADC_data_length];
int16_t AFC_adc_Current_data_ZC[DMA_ADC_data_length];
int16_t AFC_adc_Volt_data_ZC[DMA_ADC_data_length];

#endif // SYSTEM_DATA_H
