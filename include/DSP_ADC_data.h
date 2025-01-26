/*
To Process the captured ADC Raw Data
*/
#ifndef DSP_ADC_DATA_H
#define DSP_ADC_DATA_H

#include "Arduino.h"
#include <CMSIS_DSP.h>
/*
Reference for FFT
1. CMSIS DSP - https://github.com/stm32duino/Arduino_Core_STM32/wiki/API#cmsis-dsp
2. Phil's Lab - https://www.youtube.com/watch?v=d1KvgOwWvkM
*/

/*----------------------------------------------------------------------*/

// Variable Delcaration

arm_rfft_fast_instance_f32 fft_handler;

#define FFT_BUFFER_SIZE 128 // Size should power of 2 like 2,4,8,16,32,64,
float fft_in_buffer[FFT_BUFFER_SIZE], fft_out_buffer[FFT_BUFFER_SIZE];
u_int8_t fft_flag_complete = 0;

/*----------------------------------------------------------------------*/

// Function Delcaration
float adc_volt_convert(int raw_adc);
uint16_t copy_raw_data_input(int16_t _source_data[], int16_t *_target_data, uint8_t _max_data_size);
float calculate_voltage(int16_t *_data_in, int16_t _time_gap, uint8_t _max_data_size);
void setup_fft_function();
void process_fft_data_manually(int16_t *_input_data, float *_output_data, u_int16_t _buffer_size);

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

void setup_fft_function()
{
    arm_rfft_fast_init_f32(&fft_handler, FFT_BUFFER_SIZE);
}

void process_fft_data_manually(int16_t *_input_data, float *_output_data, u_int16_t _buffer_size)
{
    // Pre Process the data
    static int16_t fftIndex = 0;
    for (int n = 0; n < _buffer_size; n++)
    {
        // Convert into float value
        float _input_float_data = (float)_input_data[n];

        // Fill FFT buffer with data
        fft_in_buffer[fftIndex] = _input_float_data;
        fftIndex++;
        // Note: Buffer size of FFT value and Input value not required to be same.
    }
    if (1) // if (fftIndex >= _buffer_size)
    {
        // Perform FFT
        arm_rfft_fast_f32(&fft_handler, fft_in_buffer, fft_out_buffer, 0);

        // Set the FFT Complete flag variable
        fft_flag_complete = 1;

        // Reset the FFT Buffer Index
        fftIndex = 0;
    }

    // Post-process the FFT output data
    for (int n = 0; n < _buffer_size; n++) {
        // Convert FFT output to int16_t
        _output_data[n] = fft_out_buffer[n];
    }
    fftIndex = 0;
}
#endif