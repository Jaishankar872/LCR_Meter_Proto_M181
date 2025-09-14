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
    uint16_t target_freq;
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

// Global array to store RMS values for all 8 ADC channels
// Index mapping: [0-1]: Voltage channels, [2-3]: AFC Voltage, [4-5]: Current channels, [6-7]: AFC Current
float rms_value_all_ch[8];

int16_t _phase_offset_array_index = 0;

// Private Function Declaration
void generate_ref_signal(int _length);
float calculate_rms_amplitude(int16_t *data_in, int16_t _dat_len);
float adc_volt_convert(int16_t raw_adc);

// Goertzel
void filter_init(settings_goertzel *g_buffer, uint16_t freq_value, uint32_t _sample_freq);
void goertzel_process(settings_goertzel *g_buffer, int16_t *data, calc_goertzel *g_result);

int16_t low_pass_filter_calc(int16_t input, int16_t prev_output);
// float phase_value_calculation(int16_t _in_array[], int16_t _start_l, int16_t _length);
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
    LCR_setup.target_freq = _adc_data->set_freq;
    LCR_setup.sample_freq = ((uint32_t)DMA_ADC_DATA_LENGTH / no_of_sine_wave_cycle_per_data) * 1000UL;
    LCR_setup.N_len = DMA_ADC_DATA_LENGTH;

    // Initate the filter
    filter_init(&LCR_setup, LCR_setup.target_freq, LCR_setup.sample_freq);

    // Process the data
    const int _channel_count = 8;
    for (int col = 0; col < _channel_count; col++)
    {
        // For Phase------------------------------------
        goertzel_process(&LCR_setup, adc_raw_data[col], &LCR_calc_data[col]);
        // For Amplitude--------------------------------
        rms_value_all_ch[col] = calculate_rms_amplitude(adc_raw_data[col], DMA_ADC_DATA_LENGTH);
        // rms_value_all_ch[col] = LCR_calc_data[col].amp;
    }

    // Automatic Gain Selection
    // 1. Set Default as Gain B - Minimum
    volt_gain_sel = voltage_low_gain_mode;
    amp_gain_sel = current_low_gain_mode;

    // 2. Check the gain value
    int16_t _threshold = 50;
    if (rms_value_all_ch[voltage_low_gain_mode * 2] < _threshold)
        volt_gain_sel = voltage_high_gain_mode;
    if (rms_value_all_ch[current_low_gain_mode * 2] < _threshold)
        amp_gain_sel = current_high_gain_mode;

    _adc_data->rms_voltage = adc_volt_convert((float)rms_value_all_ch[volt_gain_sel * 2]);
    _adc_data->rms_AFC_volt = adc_volt_convert((float)rms_value_all_ch[(volt_gain_sel * 2) + 1]);
    _adc_data->rms_current = adc_volt_convert((float)rms_value_all_ch[(amp_gain_sel * 2)]);
    _adc_data->rms_AFC_current = adc_volt_convert((float)rms_value_all_ch[(amp_gain_sel * 2) + 1]);

    // Impedance Calculated
    _adc_data->impedance = _adc_data->rms_voltage / _adc_data->rms_current;

    // Applying the correction Factor - Manually
    float Amplifier_gain = 101.0f * 1.017f;
    // Voltage
    if (volt_gain_sel == voltage_low_gain_mode)
        _adc_data->impedance /= Amplifier_gain;
    // Current Comp
    if (amp_gain_sel == current_low_gain_mode)
        _adc_data->impedance *= Amplifier_gain;
    // else
    //     _adc_data->impedance *= 0.874;

    // 2. Phase calculations
    float v_phase = LCR_calc_data[volt_gain_sel * 2].phase;
    float v_ref_phase = LCR_calc_data[(volt_gain_sel * 2) + 1].phase;
    float i_phase = LCR_calc_data[amp_gain_sel * 2].phase;
    float i_ref_phase = LCR_calc_data[(amp_gain_sel * 2) + 1].phase;

    // Calculate relative phases (preserving sign)
    _adc_data->voltage_phase = fmodf(v_phase - v_ref_phase + 360.0f, 360.0f);
    _adc_data->current_phase = fmodf(i_phase - i_ref_phase + 360.0f, 360.0f);
    _adc_data->current_phase -= 180.0f; // Subtract the Trans-impedance Amplifier phase

    // Calculate V-I phase difference (preserving sign)
    float phase_diff = _adc_data->voltage_phase - _adc_data->current_phase;
    if (phase_diff < 0) // To remove Negative sign
        phase_diff *= -1;
    _adc_data->VI_phase = phase_diff;

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
    g_buffer->target_freq = freq_value;
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

float calculate_rms_amplitude(int16_t *data_in, int16_t _dat_len)
{
    double mean = 0.0;
    double sum_sq = 0.0;
    int n = _dat_len;

    // First pass: Calculate mean (DC offset)
    double sum = 0.0;
    for (int i = 0; i < n; i++)
    {
        sum += data_in[i];
    }
    mean = sum / n;

    // Second pass: Calculate RMS with DC offset removed
    for (int i = 0; i < n; i++)
    {
        double ac_value = data_in[i] - mean;
        sum_sq += ac_value * ac_value;
    }

    // Calculate true RMS value
    return (float)sqrt(sum_sq / n);
}

#define ADC_REF_VOLTAGE 3.3f
#define ADC_RESOLUTION  4096U  // 12-bit ADC

float adc_volt_convert(int16_t raw_adc)
{
    // Guard against division by zero (should never happen with constant)
    if (ADC_RESOLUTION == 0) {
        return 0.0f;  // Return safe value
    }
    
    // Cast to float before multiplication to prevent integer overflow
    // Then divide by resolution to get voltage
    return (ADC_REF_VOLTAGE * (float)raw_adc) / (float)ADC_RESOLUTION;
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