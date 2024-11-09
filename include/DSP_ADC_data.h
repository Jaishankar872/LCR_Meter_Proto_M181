/*
Code is for SSD1306 128x64 display
*/
#ifndef DSP_ADC_DATA_H
#define DSP_ADC_DATA_H

#include "Arduino.h"

/*----------------------------------------------------------------------*/

// Function Delcaration
float adc_volt_convert(int raw_adc);
uint16_t copy_raw_data_input(int16_t _source_data[], int16_t *_target_data, uint8_t _max_data_size);
float calculate_voltage(int16_t *_data_in, int16_t _time_gap, uint8_t _max_data_size);

/*----------------------------------------------------------------------*/

// Function Definition

float adc_volt_convert(int raw_adc)
{
    int adc_res = 4096;
    float adc_ref = 3.3;
    float volt_reading1 = 0;
    volt_reading1 = (float)adc_ref * raw_adc;
    volt_reading1 /= adc_res;
    return volt_reading1;
}

float calculate_voltage(int16_t *_data_in, int16_t _time_gap, uint8_t _max_data_size)
{
    int16_t _max_value_t = 0;
    int16_t _min_value_t = 4500;
    int16_t _pk_pk_raw_value = 0;

    for (int i = 0; i < _max_data_size; i++)
    {
        if (_data_in[i] > _max_value_t)
            _max_value_t = _data_in[i];
        if (_data_in[i] < _min_value_t)
            _min_value_t = _data_in[i];
    }
    _pk_pk_raw_value = (_max_value_t - _min_value_t);
    float _pk_pk_voltage = adc_volt_convert(_pk_pk_raw_value);

    return _pk_pk_voltage;
}

uint16_t copy_raw_data_input(int16_t _source_data[], int16_t *_target_data, uint8_t _max_data_size)
{
    /*
    Error Code Means
    0 => No Error
    1 => Some value are zeros & -ve Value.
    2 => Checking missing somewhere in code
    */
    uint8_t _error_code = 2;

    // Check and copy the data
    for (int i = 0; i < _max_data_size; i++)
    {
        if (_source_data[i] <= 0)
        {
            _error_code = 1;
            // break; //Comment as of Now to skip the error logic
        }
        else
            _target_data[i] = _source_data[i];

        // At the last loop run (or Before exiting)
        if (i == (_max_data_size - 1))
            _error_code = 0;
    }

    return _error_code;
}

#endif