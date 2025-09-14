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
typedef struct
{
    uint16_t traget_freq;
    uint16_t sample_freq;
    uint16_t N_len;
    float coeff;
    float sin_w;
    float cos_w;
} settings_goertzel;

typedef struct
{
    float re;
    float im;
    float amp;
    float phase;
} calc_goertzel;

settings_goertzel LCR_setup = {0};
calc_goertzel LCR_calc_data[8];

// int8_t ref0_data[DMA_ADC_DATA_LENGTH];
// int8_t ref90_data[DMA_ADC_DATA_LENGTH];

int16_t max_adc_Volt = 0, min_adc_Volt = 4096;
int16_t max_adc_Volt_AFC = 0, min_adc_Volt_AFC = 4096;
int16_t max_adc_Current = 0, min_adc_Current = 4096;
int16_t max_adc_Current_AFC = 0, min_adc_Current_AFC = 4096;

int16_t _phase_offset_array_index = 0;

// Private Function Declaration
void generate_ref_signal(int _length);
void calculate_signal_amplitude(system_data *_adc_data1);
float adc_volt_convert(int16_t raw_adc);

// Goertzel
void filter_init(settings_goertzel *g_buffer, uint16_t freq_value, uint32_t _sample_freq);
void goertzel_process(settings_goertzel *g_buffer, int16_t *data, calc_goertzel *g_result);

int16_t low_pass_filter_calc(int16_t input, int16_t prev_output);
float phase_value_calculation(int16_t _in_array[], int16_t _start_l, int16_t _length);
float LCR_calculation(uint8_t _mode, uint16_t _freq, float _impedance, float _phase);
int8_t unit_conversion(float *value);

// Function Definition
void setup_DSP_parameter()
{
    // generate_ref_signal(DMA_ADC_DATA_LENGTH);
}

void process_data_via_DSP(system_data *_adc_data)
{
    // Fetch the Settings
    LCR_setup.traget_freq = _adc_data->set_freq;
    LCR_setup.sample_freq = ((uint32_t)DMA_ADC_DATA_LENGTH / no_of_sine_wave_cycle_per_data) * 1000UL;
    LCR_setup.N_len = DMA_ADC_DATA_LENGTH;

    // Initate the filter
    filter_init(&LCR_setup, LCR_setup.traget_freq, LCR_setup.sample_freq);

    // Process the data
    // For Amplitude--------------------------------
    calculate_signal_amplitude(_adc_data);
    _adc_data->impedance = _adc_data->rms_voltage / _adc_data->rms_current;

    //Applying the correction Factor - Manually
    if (volt_gain_sel)
        _adc_data->impedance /= 101;
    if (amp_gain_sel)
        _adc_data->impedance *= (101*1.017);
    else
        _adc_data->impedance *= 0.874;
    
    // For Phase------------------------------------
    const int _channel_count = 8;
    for (int col = 0; col < _channel_count; col++)
        goertzel_process(&LCR_setup, adc_raw_data[col], &LCR_calc_data[col]);

    // 2. Phase
    float _phase_diff_temp = 0;
    // Voltage
    _phase_diff_temp = fabsf(LCR_calc_data[volt_gain_sel * 4].phase - LCR_calc_data[volt_gain_sel * 4 + 1].phase);
    _adc_data->voltage_phase = _phase_diff_temp;
    // Current
    _phase_diff_temp = fabsf(LCR_calc_data[(amp_gain_sel * 4) + 3].phase - LCR_calc_data[(amp_gain_sel * 4) + 2].phase);
    _adc_data->current_phase = _phase_diff_temp;
    _adc_data->current_phase -= 180;
    // Phase calculation for voltage & current
    _adc_data->VI_phase = fabsf(fabsf(_adc_data->voltage_phase) - fabsf(_adc_data->current_phase));

    _adc_data->esr = LCR_calculation(3, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase);       // ESR
    _adc_data->tan_delta = LCR_calculation(4, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase); // Tan Delta calculation

    if (_adc_data->LCR_Mode == 1)
        _adc_data->inductance = LCR_calculation(_adc_data->LCR_Mode, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase); // Inductance
    else if (_adc_data->LCR_Mode == 2)
        _adc_data->capacitance = LCR_calculation(_adc_data->LCR_Mode, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase); // Capacitance
    else if (_adc_data->LCR_Mode == 3)
    {
        _adc_data->resistance = _adc_data->impedance; // LCR_calculation(_adc_data->LCR_Mode, _adc_data->set_freq, _adc_data->impedance, _adc_data->VI_phase); // ESR
        _adc_data->esr = 0;
        _adc_data->tan_delta = 0;
    }
    // Unit conversion for capacitance, inductance, and resistance
    _adc_data->unit_capacitance = unit_conversion(&_adc_data->capacitance);
    _adc_data->unit_inductance = unit_conversion(&_adc_data->inductance);
    _adc_data->unit_resistance = unit_conversion(&_adc_data->resistance);
    _adc_data->unit_esr = unit_conversion(&_adc_data->esr);
}

void filter_init(settings_goertzel *g_buffer, uint16_t freq_value, uint32_t _sample_freq)
{
    // 1) Compute the bin index k
    const uint32_t k_constant = (uint32_t)(floorf(((float)(DMA_ADC_DATA_LENGTH * freq_value) / _sample_freq) + 0.5f));

    // 2) Compute normalized angular frequency ω = 2πk/N
    const float w_freq = (2.0f * PI * (float)k_constant) / DMA_ADC_DATA_LENGTH;

    // 3) Precompute constants
    const float wr = cosf(w_freq);
    const float wi = sinf(w_freq);
    g_buffer->traget_freq = freq_value;
    g_buffer->coeff = wr * 2.0f;
    g_buffer->cos_w = wr;
    g_buffer->sin_w = wi;
}

void goertzel_process(settings_goertzel *g_buffer, int16_t *data, calc_goertzel *g_result)
{
    uint16_t N_length = g_buffer->N_len;
    // 5) Main recurrence
    float s_prev = 0, s_prev2 = 0, s;
    for (int n = 0; n < N_length; n++)
    {
        s = ((float)data[n]) + g_buffer->coeff * s_prev - s_prev2;
        s_prev2 = s_prev;
        s_prev = s;
    }

    // 6) Compute real & imag components
    float real = s_prev - s_prev2 * g_buffer->cos_w;
    float imag = s_prev2 * g_buffer->sin_w;

    // [**Doubt] correct the output sample amplitude
    const float scale = 1; // 2.0f / N_length;
    g_result->re = real * scale;
    g_result->im = imag * scale;

    g_result->amp = sqrtf((g_result->re * g_result->re) + (g_result->im * g_result->im));
    g_result->phase = atan2f(g_result->im, g_result->re) * (180.0f / PI);
}

void calculate_signal_amplitude(system_data *_adc_data1)
{
    // double sum_sq_adc_Volt = 0.0, sum_sq_AFC_adc_Volt = 0.0;
    // double sum_sq_adc_Current = 0.0, sum_sq_AFC_adc_Current = 0.0;
    double sum_sq_adc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    double rms_val_adc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    // double amp_val_adc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int n = DMA_ADC_DATA_LENGTH;

    // Accumulate squared deviations
    for (int _row = 0; _row < 8; _row++)
    {
        for (int _col = 0; _col < n; _col++)
        {
            double _val_sq = 0;
            if (_row % 2 == 0)
                _val_sq = adc_raw_data[_row][_col];
            else
                _val_sq = adc_raw_data[_row][_col];

            sum_sq_adc[_row] += _val_sq * _val_sq;
            adc_raw_data[_row][_col] = _val_sq;
        }
        // Calculate RMS
        rms_val_adc[_row] = sqrt(sum_sq_adc[_row] / n);
        // Peak amplitude = RMS * sqrt(2)
        // amp_val_adc[_row] = rms_val_adc[_row] * sqrt(2.0);
    }

    // Automatic Gain Selection
    // 1. Set Default as Gain A
    volt_gain_sel = 1;
    amp_gain_sel = 1;

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

// void generate_ref_signal(int _length)
// {
//     // Assume two complete cycles, each divided into 4 quadrants
//     // Total quadrants = 8; each quadrant length:

//     int per_len_quadrant = _length / 8;
//     for (int i = 0; i < 8; i++)
//     {
//         // Use (i % 4) to cycle through quadrants for one complete cycle
//         int quadrant = i % 4;
//         int start_index = i * per_len_quadrant;
//         for (int j = 0; j < per_len_quadrant; j++)
//         {
//             switch (quadrant)
//             {
//             case 0: // 0°-90°: ref0 = +1, ref90 = +1
//                 ref0_data[start_index + j] = 1;
//                 ref90_data[start_index + j] = 1;
//                 break;
//             case 1: // 90°-180°: ref0 = +1, ref90 = -1
//                 ref0_data[start_index + j] = 1;
//                 ref90_data[start_index + j] = -1;
//                 break;
//             case 2: // 180°-270°: ref0 = -1, ref90 = -1
//                 ref0_data[start_index + j] = -1;
//                 ref90_data[start_index + j] = -1;
//                 break;
//             case 3: // 270°-360°: ref0 = -1, ref90 = +1
//                 ref0_data[start_index + j] = -1;
//                 ref90_data[start_index + j] = 1;
//                 break;
//             }
//         }
//     }
// }

// float phase_value_calculation(int16_t _in_array[], int16_t _start_l, int16_t _length)
// {
//     float I_sum = 0.0f, Q_sum = 0.0f;
//     int sample_count = _length - _start_l;

//     for (int i = _start_l; i < _length; i++)
//     {
//         // Multiply signal by 0° and 90° square wave references
//         I_sum += _in_array[i] * ref0_data[i];
//         Q_sum += _in_array[i] * ref90_data[i];
//     }

//     // Normalize by sample count and scale for square-wave demodulation (2/pi)
//     float I_avg = (I_sum / sample_count) * (3.14159f / 2.0f);
//     float Q_avg = (Q_sum / sample_count) * (3.14159f / 2.0f);

//     // Calculate phase angle in degrees using atan2f
//     float phase = atan2f(Q_avg, I_avg) * (180.0f / 3.14159f);
//     return phase;
// }

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