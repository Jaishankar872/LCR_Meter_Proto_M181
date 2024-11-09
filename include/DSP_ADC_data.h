/*
Code is for SSD1306 128x64 display
*/
#ifndef DSP_ADC_DATA_H
#define DSP_ADC_DATA_H

#include "Arduino.h"

/*----------------------------------------------------------------------*/

// Function Delcaration
float adc_volt_convert(int raw_adc);
bool process_adc_data(int16_t _volt_a[], int16_t _current_a[], uint8_t _max_data_size);
uint16_t copy_raw_data_input(int16_t _volt_a[], int16_t _current_a[], uint8_t _max_data_size);
uint8_t filter_low_pass(int16_t *_data_in, uint8_t _max_data_size);
uint16_t remove_data_offset(int16_t *_data_in, uint8_t _max_data_size);
float measure_rms_value(int16_t _data_in[], unsigned int _time_gap, uint8_t _max_data_size);


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

float measure_rms_value(int16_t _data_in[], unsigned int _time_gap, uint8_t _max_data_size)
{
    // Calculate one cycle of sine data [start and end point]
    int8_t _start_point_adc = 9;
    int8_t _end_point_adc = _start_point_adc;

    // Find the end of one cycle sine
    bool _calculation_process = 0; // 1 - Automatic, 0 - Maunal
    if (_calculation_process)
    {
        const int8_t _DELTA_limit = 90;
        for (uint8_t c1 = _start_point_adc + 15; c1 < _max_data_size; c1++)
        {
            int16_t _temp_delta1 = _data_in[_start_point_adc] - _data_in[c1];
            if (_temp_delta1 < 0)
                _temp_delta1 = -_temp_delta1; // Convert to positive
            // if (1)
            // {
            //     Serial_debug.print("Index:");
            //     Serial_debug.print(c1);
            //     Serial_debug.print(", ");
            //     Serial_debug.print("Delta1:");
            //     Serial_debug.print(_temp_delta1);
            //     Serial_debug.print(", ");
            //     Serial_debug.print("ADC Data:");
            //     Serial_debug.println(_data_in[c1]);
            // }
            if (_temp_delta1 <= _DELTA_limit)
            {
                _end_point_adc = c1;
                break;
            }
        }
    }
    else
    {
        // Serial_debug.println("Maunal calc");
        _end_point_adc = _start_point_adc + 40;
    }

    int8_t value_count1 = _end_point_adc - _start_point_adc;
    float _frequency = value_count1 * _time_gap;
    _frequency = 1000 / _frequency;
    _frequency *= 1000;

    //----------------------------------------------------------------------------
    float rms_value1 = 0;

    for (int i = _start_point_adc; i <= _end_point_adc; i++)
    {
        int rms_ac_temp = _data_in[i]; // - _peak_value;
        if (rms_ac_temp < 0)           // Convert -Ve into +Ve value
            rms_ac_temp *= -1;

        float rms_ac_volt_temp = adc_volt_convert(rms_ac_temp);
        rms_value1 += (rms_ac_volt_temp * rms_ac_volt_temp);
    }
    rms_value1 = rms_value1 / value_count1;
    rms_value1 = sqrt(rms_value1);

    return rms_value1;
}

uint16_t remove_data_offset(int16_t *_data_in, uint8_t _max_data_size)
{
    int16_t _max_value_t = 0;
    int16_t _min_value_t = 4500;
    int16_t _offset = 0;

    for (int i = 0; i < _max_data_size; i++)
    {
        if (_data_in[i] > _max_value_t)
            _max_value_t = _data_in[i];
        if (_data_in[i] < _min_value_t)
            _min_value_t = _data_in[i];
    }
    _offset = (_max_value_t - _min_value_t) / 2;
    _offset += _min_value_t;

    // Serial_debug.print("offest value=");
    // Serial_debug.println(_offset);
    for (int i = 0; i < _max_data_size; i++)
    {
        _data_in[i] -= _offset;
        // Serial_debug.println(_data_in[i]);
    }
    // Serial_debug.println("offest removed");

    return (_offset);
}

uint8_t filter_low_pass(int16_t *_data_in, uint8_t _max_data_size)
{
    // Alpha value
    const float filter_constant = 0.5;
    const float complement_constant = 1.0 - filter_constant;

    // Serial_debug.println(volt_adc_data[0]);
    // Formula
    for (int i = 1; i < _max_data_size; i++)
    {
        _data_in[i] = (filter_constant * _data_in[i]) + (complement_constant * _data_in[i - 1]);
        // Serial_debug.println(_data_in[i]);
    }
    // Serial_debug.println("Low pass output");
    return 0;
}

uint16_t copy_raw_data_input(int16_t _source_data[], int16_t *_target_data, uint8_t _max_data_size)
{
    /*
    Error Code Means
    0 => No Error
    1 => Some value are zeros
    2 => Checking missing somewhere in code
    */
    uint8_t _error_code = 2;

    // Check and copy the data
    for (int i = 0; i < _max_data_size; i++)
    {
        if (_source_data[i] == 0)
        {
            _error_code = 1;
            break;
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