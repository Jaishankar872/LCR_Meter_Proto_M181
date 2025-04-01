/**
 * @file DSP_data.c
 * @brief Process the data from ADC
 * This file contains the function definition for Process the data from ADC
 *
 * @author Jaishankar M
 */

#include "DSP_data.h"

// Private Includes


// Private Variable Declaration
int16_t max_adc_Volt = 0, min_adc_Volt = 4096;
int16_t max_adc_Volt_AFC = 0, min_adc_Volt_AFC = 4096;
int16_t max_adc_Current = 0, min_adc_Current = 4096;
int16_t max_adc_Current_AFC = 0, min_adc_Current_AFC = 4096;

// Private Function Declaration
void separate_adc_max_value();
float adc_volt_convert(int16_t raw_adc);


// Function Definition
void calculate_peak_value(system_data *_adc_data) // Pointer used to edit in struct value
{
        separate_adc_max_value();
        int16_t _pk_pk;
        _pk_pk = max_adc_Volt - min_adc_Volt;
        _adc_data->pk_pk_voltage = adc_volt_convert(_pk_pk);
        _pk_pk = max_adc_Volt_AFC - min_adc_Volt_AFC;
        _adc_data->pk_pk_AFC_volt = adc_volt_convert(_pk_pk);
        _pk_pk = max_adc_Current - min_adc_Current;
        _adc_data->pk_pk_current = adc_volt_convert(_pk_pk);
        _pk_pk = max_adc_Current_AFC - min_adc_Current_AFC;
        _adc_data->pk_pk_AFC_current = adc_volt_convert(_pk_pk);
}

void separate_adc_max_value()
{
    // Reset the value before get into the loop
    // Voltage Value Reset
    max_adc_Volt = 0;
    min_adc_Volt = 4096;
    max_adc_Volt_AFC = 0;
    min_adc_Volt_AFC = 4096;

    // Current Value Reset
    max_adc_Current = 0;
    min_adc_Current = 4096;
    max_adc_Current_AFC = 0;
    min_adc_Current_AFC = 4096;

    for (int i = 0; i < DMA_ADC_data_length; i++)
    {
        // Voltage Value Reset
        // Maximum & Minimum calculation
        if (adc_Volt_data[i] > max_adc_Volt)
            max_adc_Volt = adc_Volt_data[i];
        if (adc_Volt_data[i] < min_adc_Volt)
            min_adc_Volt = adc_Volt_data[i];
        // Maximum & Minimum calculation - AFC
        if (AFC_adc_Volt_data[i] > max_adc_Volt_AFC)
            max_adc_Volt_AFC = AFC_adc_Volt_data[i];
        if (AFC_adc_Volt_data[i] < min_adc_Volt_AFC)
            min_adc_Volt_AFC = AFC_adc_Volt_data[i];

        // Current Value Reset
        // Maximum & Minimum calculation
        if (adc_Current_data[i] > max_adc_Current)
            max_adc_Current = adc_Current_data[i];
        if (adc_Current_data[i] < min_adc_Current)
            min_adc_Current = adc_Current_data[i];
        // Maximum & Minimum calculation -AFC
        if (AFC_adc_Current_data[i] > max_adc_Current_AFC)
            max_adc_Current_AFC = AFC_adc_Current_data[i];
        if (AFC_adc_Current_data[i] < min_adc_Current_AFC)
            min_adc_Current_AFC = AFC_adc_Current_data[i];
    }
}

float adc_volt_convert(int16_t raw_adc)
{
    int16_t adc_res = 4096;
    float adc_ref = 3.3;
    float volt_reading1 = 0;
    volt_reading1 = (float)adc_ref * raw_adc;
    volt_reading1 /= adc_res;
    return volt_reading1;
}
