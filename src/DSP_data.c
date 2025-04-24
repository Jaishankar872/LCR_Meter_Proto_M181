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
#define PI 3.14159f

// Private Variable Declaration
int8_t ref0_data[DMA_ADC_data_length];
int8_t ref90_data[DMA_ADC_data_length];

int16_t max_adc_Volt = 0, min_adc_Volt = 4096;
int16_t max_adc_Volt_AFC = 0, min_adc_Volt_AFC = 4096;
int16_t max_adc_Current = 0, min_adc_Current = 4096;
int16_t max_adc_Current_AFC = 0, min_adc_Current_AFC = 4096;

int16_t _phase_offset_array_index = 0;

// Private Function Declaration
void generate_ref_signal(int _length);
void calculate_signal_amplitude(system_data *_adc_data1);
float adc_volt_convert(int16_t raw_adc);
int16_t low_pass_filter_calc(int16_t input, int16_t prev_output);
float phase_value_calculation(int16_t _in_array[], int16_t _start_l, int16_t _length);
float LCR_calculation(uint8_t _mode, uint16_t _freq, float _impedance, float _phase);
int8_t unit_conversion(float *value);

// Function Definition
void setup_DSP_parameter()
{
    generate_ref_signal(DMA_ADC_data_length);
}

void process_data_via_DSP(system_data *_adc_data)
{
    calculate_signal_amplitude(_adc_data);
    _adc_data->impedance = _adc_data->rms_voltage / _adc_data->rms_current;

    _phase_offset_array_index = 0;
    for (int j = 1; j < 8; j++)
    {
        for (int i = 1; i < DMA_ADC_data_length; i++)
        {
            // Low pass filter
            adc_raw_data[j][i] = low_pass_filter_calc(adc_raw_data[j][i], adc_raw_data[j][i - 1]);
        }
    }
    _adc_data->voltage_phase = phase_value_calculation(adc_raw_data[(amp_gain_sel * 4)], _phase_offset_array_index, DMA_ADC_data_length) - phase_value_calculation(adc_raw_data[(amp_gain_sel * 4) + 2], _phase_offset_array_index, DMA_ADC_data_length);                    // Phase calculation for voltage
    _adc_data->current_phase = fabsf(phase_value_calculation(adc_raw_data[(amp_gain_sel * 4) + 2], _phase_offset_array_index, DMA_ADC_data_length) - phase_value_calculation(adc_raw_data[(amp_gain_sel * 4) + 3], _phase_offset_array_index, DMA_ADC_data_length)); // Phase calculation for current
    _adc_data->current_phase -= 180;

    _adc_data->VI_phase = fabsf(fabsf(_adc_data->voltage_phase) - fabsf(_adc_data->current_phase)); // Phase calculation for voltage & current

    _adc_data->esr = LCR_calculation(3, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase);       // ESR
    _adc_data->tan_delta = LCR_calculation(4, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase); // Tan Delta calculation

    if (_adc_data->LCR_Mode == 1)
        _adc_data->inductance = LCR_calculation(_adc_data->LCR_Mode, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase); // Inductance
    else if (_adc_data->LCR_Mode == 2)
        _adc_data->capacitance = LCR_calculation(_adc_data->LCR_Mode, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase); // Capacitance
    else if (_adc_data->LCR_Mode == 3)
    {
        _adc_data->resistance = LCR_calculation(_adc_data->LCR_Mode, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase); // ESR
        _adc_data->esr = 0;
        _adc_data->tan_delta = 0;
    }
    // Unit conversion for capacitance, inductance, and resistance
    _adc_data->unit_capacitance = unit_conversion(&_adc_data->capacitance);
    _adc_data->unit_inductance = unit_conversion(&_adc_data->inductance);
    _adc_data->unit_resistance = unit_conversion(&_adc_data->resistance);
    _adc_data->unit_esr = unit_conversion(&_adc_data->esr);
}

int16_t low_pass_filter_calc(int16_t input, int16_t prev_output)
{
    const float ALPHA = 0.15f; // Use 'const' for fixed values
    return (int16_t)((ALPHA * input) + ((1.0f - ALPHA) * prev_output));
}

void calculate_signal_amplitude(system_data *_adc_data1)
{
    // double sum_sq_adc_Volt = 0.0, sum_sq_AFC_adc_Volt = 0.0;
    // double sum_sq_adc_Current = 0.0, sum_sq_AFC_adc_Current = 0.0;
    double sum_sq_adc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    double rms_val_adc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    // double amp_val_adc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int n = DMA_ADC_data_length;

    double offset_PA0 = 2275;     // zero_pad_adc_PA[0]
    double offset_PA1_AFC = 1861; // zero_pad_adc_PA[1]

    // Remove DC offset and accumulate squared deviations
    for (int _row = 0; _row < 8; _row++)
    {
        for (int _col = 0; _col < n; _col++)
        {
            double _val_sq = 0;
            if (_row % 2 == 0)
                _val_sq = adc_raw_data[_row][_col] - offset_PA0;
            else
                _val_sq = adc_raw_data[_row][_col] - offset_PA1_AFC;

            sum_sq_adc[_row] += _val_sq * _val_sq;
        }
        // Calculate RMS
        rms_val_adc[_row] = sqrt(sum_sq_adc[_row] / n);
        // Peak amplitude = RMS * sqrt(2)
        // amp_val_adc[_row] = rms_val_adc[_row] * sqrt(2.0);
    }

    // Automatic Gain Selection
    // 1. Set Default as Gain A
    volt_gain_sel = 0;
    amp_gain_sel = 0;

    // 2. Check the gain value
    int16_t _threshold = 50;
    if (rms_val_adc[volt_gain_sel * 4] < _threshold)
        volt_gain_sel = 1;
    if (rms_val_adc[(amp_gain_sel * 4) + 2] < _threshold)
        amp_gain_sel = 1;

    // Update system_data struct with amplitude after conversion
    _adc_data1->rms_voltage = adc_volt_convert((float)rms_val_adc[volt_gain_sel * 4]);
    _adc_data1->rms_AFC_volt = adc_volt_convert((float)rms_val_adc[volt_gain_sel * 4 + 1]);
    _adc_data1->rms_current = adc_volt_convert((float)rms_val_adc[(amp_gain_sel * 4) + 2]);
    _adc_data1->rms_AFC_current = adc_volt_convert((float)rms_val_adc[(amp_gain_sel * 4) + 3]);
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

float phase_value_calculation(int16_t _in_array[], int16_t _start_l, int16_t _length)
{
    float I_sum = 0.0f, Q_sum = 0.0f;
    int sample_count = _length - _start_l;

    for (int i = _start_l; i < _length; i++)
    {
        // Multiply signal by 0° and 90° square wave references
        I_sum += _in_array[i] * ref0_data[i];
        Q_sum += _in_array[i] * ref90_data[i];
    }

    // Normalize by sample count and scale for square-wave demodulation (2/pi)
    float I_avg = (I_sum / sample_count) * (3.14159f / 2.0f);
    float Q_avg = (Q_sum / sample_count) * (3.14159f / 2.0f);

    // Calculate phase angle in degrees using atan2f
    float phase = atan2f(Q_avg, I_avg) * (180.0f / 3.14159f);
    return phase;
}

float LCR_calculation(uint8_t _mode, uint16_t _freq, float _impedance, float _phase)
{
    // Convert phase to radians
    float phase_rad = _phase * (PI / 180.0f);
    // Compute the reactive component (absolute value)
    float reactance = _impedance * fabsf(sinf(phase_rad));
    if (reactance < 1e-6f)
        return 0.0f;
    // Angular frequency in rad/s
    float omega = 2.0f * PI * _freq;

    if (_mode == 1)
    { // Inductance: L = X / ω, convert to microHenries
        float inductance = reactance / omega;
        return inductance * 1e6f;
    }
    else if (_mode == 2)
    { // Capacitance: C = 1/(ωX), convert to nanoFarads
        float capacitance = 1.0f / (omega * reactance);
        if (amp_gain_sel)
            return capacitance * 1e7f;
        else
            return capacitance * 1e9f;
    }
    else if (_mode == 3)
    { // ESR: real part of the impedance
        return _impedance * fabsf(cosf(phase_rad));
    }
    else if (_mode == 4)
    { // Tan Delta calculation
        return (_impedance * fabsf(cosf(phase_rad))) / reactance;
    }
    return 0.0f;
}

int8_t unit_conversion(float *value)
{
    // No conversion needed if the value is between 1.0 and 999.9
    if (*value >= 1.0f && *value <= 999.9f)
    {
        return 0; // No unit conversion
    }
    // Conversion for values larger than 999.9
    else if (*value > 999.9f)
    {
        if (*value < 1e6f)
        {
            *value /= 1000.0f; // Convert to kilo
            return 1;          // Unit: Kilo
        }
        else // *value is at least 1e6
        {
            *value /= 1e6f; // Convert to mega
            return 2;       // Unit: Mega
        }
    }
    // Conversion for values lower than 1
    else // *value < 1.0f
    {
        *value *= 1000.0f; // Convert to milli
        return -1;         // Unit: Milli
    }
}