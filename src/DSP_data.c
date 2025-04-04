/**
 * @file DSP_data.c
 * @brief Process the data from ADC
 * This file contains the function definition for Process the data from ADC
 *
 * @author Jaishankar M
 */

#include "DSP_data.h"

// Private Includes
#include <math.h>

// Private Variable Declaration
int8_t ref0_data[DMA_ADC_data_length];
int8_t ref90_data[DMA_ADC_data_length];

int16_t max_adc_Volt = 0, min_adc_Volt = 4096;
int16_t max_adc_Volt_AFC = 0, min_adc_Volt_AFC = 4096;
int16_t max_adc_Current = 0, min_adc_Current = 4096;
int16_t max_adc_Current_AFC = 0, min_adc_Current_AFC = 4096;

// Private Function Declaration
void generate_ref_signal(int _length);
void calculate_peak_value(system_data *_adc_data1);
float adc_volt_convert(int16_t raw_adc);
int16_t low_pass_filter_calc(int16_t input, int16_t prev_output);
float phase_value_calculation(int16_t _in_array[DMA_ADC_data_length], int16_t _length);

// Function Definition
void setup_DSP_parameter()
{
    generate_ref_signal(DMA_ADC_data_length);
}

void process_data_via_DSP(system_data *_adc_data)
{
    calculate_peak_value(_adc_data);

    for (int i = 1; i < DMA_ADC_data_length; i++)
    {
        // Low pass filter for voltage
        adc_Volt_data[i] = low_pass_filter_calc(adc_Volt_data[i], adc_Volt_data[i - 1]);
        // Low pass filter for AFC voltage
        AFC_adc_Volt_data[i] = low_pass_filter_calc(AFC_adc_Volt_data[i], AFC_adc_Volt_data[i - 1]);
        // Low pass filter for current
        adc_Current_data[i] = low_pass_filter_calc(adc_Current_data[i], adc_Current_data[i - 1]);
        // Low pass filter for AFC current
        AFC_adc_Current_data[i] = low_pass_filter_calc(AFC_adc_Current_data[i], AFC_adc_Current_data[i - 1]);
    }
    _adc_data->voltage_phase = phase_value_calculation(adc_Volt_data, DMA_ADC_data_length) - phase_value_calculation(AFC_adc_Volt_data, DMA_ADC_data_length);              // Phase calculation for voltage
    _adc_data->current_phase = fabsf(phase_value_calculation(adc_Current_data, DMA_ADC_data_length) - phase_value_calculation(AFC_adc_Current_data, DMA_ADC_data_length)); // Phase calculation for current

    _adc_data->current_phase -= 180;

    _adc_data->VI_phase = fabs(_adc_data->voltage_phase) - fabs(_adc_data->current_phase); // Phase calculation for voltage & current
}

int16_t low_pass_filter_calc(int16_t input, int16_t prev_output)
{
    const float ALPHA = 0.15f; // Use 'const' for fixed values
    return (int16_t)((ALPHA * input) + ((1.0f - ALPHA) * prev_output));
}

void calculate_peak_value(system_data *_adc_data1) // Pointer used to edit in struct value
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

    _adc_data1->pk_pk_voltage = adc_volt_convert(max_adc_Volt - min_adc_Volt);
    _adc_data1->pk_pk_AFC_volt = adc_volt_convert(max_adc_Volt_AFC - min_adc_Volt_AFC);
    _adc_data1->pk_pk_current = adc_volt_convert(max_adc_Current - min_adc_Current);
    _adc_data1->pk_pk_AFC_current = adc_volt_convert(max_adc_Current_AFC - min_adc_Current_AFC);
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

void generate_ref_signal(int _length)
{
    // Assume two complete cycles, each divided into 4 quadrants
    // Total quadrants = 8; each quadrant length:

    int per_len_quadrant = _length / 8;
    for (int i = 0; i < 8; i++)
    {
        // Use (i % 4) to cycle through quadrants for one complete cycle
        int quadrant = i % 4;
        int start_index = i * per_len_quadrant;
        for (int j = 0; j < per_len_quadrant; j++)
        {
            switch (quadrant)
            {
            case 0: // 0°-90°: ref0 = +1, ref90 = +1
                ref0_data[start_index + j] = 1;
                ref90_data[start_index + j] = 1;
                break;
            case 1: // 90°-180°: ref0 = +1, ref90 = -1
                ref0_data[start_index + j] = 1;
                ref90_data[start_index + j] = -1;
                break;
            case 2: // 180°-270°: ref0 = -1, ref90 = -1
                ref0_data[start_index + j] = -1;
                ref90_data[start_index + j] = -1;
                break;
            case 3: // 270°-360°: ref0 = -1, ref90 = +1
                ref0_data[start_index + j] = -1;
                ref90_data[start_index + j] = 1;
                break;
            }
        }
    }
}

float phase_value_calculation(int16_t _in_array[], int16_t _length)
{
    float I_sum = 0.0, Q_sum = 0.0;

    for (int i = 0; i < _length; i++)
    {
        // Multiply signal by 0° and 90° square wave references
        I_sum += _in_array[i] * ref0_data[i];
        Q_sum += _in_array[i] * ref90_data[i];
    }

    // Normalize by sample count and scale for square-wave demodulation (2/pi)
    float I_avg = (I_sum / _length) * (3.14159 / 2.0);
    float Q_avg = (Q_sum / _length) * (3.14159 / 2.0);

    // Calculate phase angle in degrees
    float phase = atan2f(Q_avg, I_avg) * (180.0 / 3.14159);

    return phase;
}
