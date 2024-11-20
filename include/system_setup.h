#ifndef SYSTEM_SETUP_H
#define SYSTEM_SETUP_H

#include "Arduino.h"
#include "DAC_sine_wave_gen.h"
#include "ADC_Config.h"
#include "DSP_ADC_data.h"
#include <HardwareSerial.h>

// UART
HardwareSerial Serial_debug(PA_10, PA_9); //(RX, TX)

// Timer for VI Mode Switch
HardwareTimer timer2_VI_switch(TIM2); // Initialize timer TIM1
uint32_t _timer2_prescaler = 72000;
// int8_t _timer2_int_counter = 0;

// List of Variable to pass from System setup
struct system_data
{
    bool hold_btn, sp_btn, rcl_btn; // Tell the 3 button status(via polling)
    uint16_t set_freq;
    bool led_state;
    int8_t VI_measure_mode;
    float pk_pk_voltage, pk_pk_current, pk_pk_AFC;
};
system_data back_end_data;

// Define and initialize global variables
float _fw0_version = 0.1;
bool ledstate = 1;
int test_frequency = 1000;
volatile bool _btn1_hold_flag = 0, _btn2_sp_flag = 0, _btn3_rcl_flag = 0;
bool GS_pin_state = 1, VI_pin_state = 0; // VI_measure_mode = 0;
volatile int8_t _VI_measure_mode = 1;
bool auto_switch_VI_measure_mode = 1;

volatile bool volt_data_record_ready = 0, current_data_record_ready = 0;
unsigned long _timer3_record = 0, _adc_sample_time_gap = 0;

#define _sample_size 80
int volt_sample_count = 0, current_sample_count = 0;
bool measure_volt_flag = 0, measure_current_flag = 0;
int16_t volt_raw_data[_sample_size], current_raw_data[_sample_size];

int AFC_sample_count_v = 0;
int16_t AFC_raw_data_v[_sample_size];
int AFC_sample_count_i = 0;
int16_t AFC_raw_data_i[_sample_size];

#define SCREEN_ADDRESS 0x3C

#define ADC_pin PA0
#define AFC_pin PA1

#define LED_pin PA5
#define GS_pin PA6
#define VI_pin PA7

#define BTN1_HOLD PB13
#define BTN2_SP PB14
#define BTN3_RCL PB15

void int_system_setup(float _fw_ver);
void regular_task_loop();
void btn1_hold_update();
void btn2_sp_update();
void btn3_rcl_update();
void on_button_press_event();
void set_measure_mode(int8_t _mode1);

void store_VI_data(int16_t _sdata);
volatile bool adc_capture_data_enable = 1;

void store_AFC_data(int16_t _sdata);
volatile bool AFC_adc_capture_data_enable = 1;

void timer2_setup_VI();
void OnTimer2Interrupt();
bool process_adc_data(int16_t *_volt_a, int16_t *_current_a, int16_t *_AFC_a_v, int16_t *_AFC_a, uint8_t _max_data_size);

// Variable Declaration - For DSP
int8_t _data_copy_check = 0;
int16_t volt_adc_data[_sample_size], current_adc_data[_sample_size];
int16_t AFC_adc_data_v[_sample_size], AFC_adc_data_i[_sample_size];
bool adc_data_process_going = 0;
bool _dsp_serial_print0 = 0;

/*----------------------------------------------------------------------*/

// Function Definition
void int_system_setup(float _fw_ver)
{
    _fw0_version = _fw_ver;
    // ADC Input pin
    pinMode(ADC_pin, INPUT_ANALOG);
    pinMode(AFC_pin, INPUT_ANALOG); // As per the testing JYETech FW.

    // Button
    pinMode(BTN1_HOLD, INPUT_PULLUP);
    pinMode(BTN2_SP, INPUT_PULLUP);
    pinMode(BTN3_RCL, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BTN1_HOLD), btn1_hold_update, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN2_SP), btn2_sp_update, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN3_RCL), btn3_rcl_update, FALLING);

    DAC_pinMode_B0_B7(0x2);
    generate_sine_wave_data(); // Calling Sine data generator
    pinMode(LED_pin, OUTPUT);
    pinMode(GS_pin, OUTPUT);
    pinMode(VI_pin, OUTPUT);

    digitalWrite(LED_pin, ledstate);
    digitalWrite(GS_pin, GS_pin_state);
    digitalWrite(VI_pin, VI_pin_state);

    Serial_debug.begin(115200);

    timer1_setup();
    set_measure_mode(0);           // Set measure as Voltage
    DAC_sine_wave(test_frequency); // Set default Frequency
    back_end_data.set_freq = test_frequency;

    // (void)analogRead(ADC_pin);     // use one analogRead to setup pin correctly in analog mode
    setup_ADC1_PA1();
    setup_ADC2_PA0(); // kick off timer to generate TRGO events and setup ADC
    int _time_us1 = timer3_set_interval(test_frequency);

    timer2_setup_VI();

    // One Time Serial print
    if (_time_us1 != -1)
    {
        Serial_debug.println("Timer 3 Setup Overflow");
        Serial_debug.print(_time_us1);
        Serial_debug.println(" uS");
    }
    else
        Serial_debug.println("Wrong Timer3 Value, pass correct value");
}

void regular_task_loop()
{
    on_button_press_event();

    // Storing the data in passing variables
    back_end_data.VI_measure_mode = _VI_measure_mode; // Optional
    if (_VI_measure_mode == 1 || !auto_switch_VI_measure_mode)
    {
        adc_capture_data_enable = 0; // STOP: ADC Capturing the data
        process_adc_data(volt_raw_data, current_raw_data, AFC_raw_data_v, AFC_raw_data_i, _sample_size);
        adc_capture_data_enable = 1;
    }
    // delay(1);
    // Serial_debug.print(SystemCoreClock);
}

/*----------------------------------------------------------------------*/
// Addition function
void on_button_press_event()
{
    if (_btn1_hold_flag || _btn2_sp_flag) // || _btn3_rcl_flag)
    {
        Serial_debug.println("-----");
        Serial_debug.print("FW Version: V");
        Serial_debug.println(_fw0_version);
        Serial_debug.println(" ");
    }
    if (_btn1_hold_flag)
    {
        _btn1_hold_flag = 0;
        ledstate = !ledstate;
        digitalWrite(LED_pin, ledstate);
        digitalWrite(GS_pin, GS_pin_state);

        // Serial Output
        Serial_debug.println("Pressed Button 1");
        if (ledstate)
            Serial_debug.println("LED ON");
        else
            Serial_debug.println("LED OFF");
        Serial_debug.println("--");
        back_end_data.led_state = ledstate;
        auto_switch_VI_measure_mode = ledstate;
    }
    if (_btn2_sp_flag)
    {
        _btn2_sp_flag = 0;
        if (test_frequency == 100)
            test_frequency = 500;
        else if (test_frequency == 500)
            test_frequency = 1000;
        else if (test_frequency == 1000)
            test_frequency = 100;
        // test_frequency = 1000;

        DAC_sine_wave(test_frequency);
        timer3_set_interval(test_frequency);
        back_end_data.set_freq = test_frequency;

        // Serial Output
        Serial_debug.println("Pressed Button 2");
        Serial_debug.print("Test Frequency: ");
        Serial_debug.print(test_frequency);
        Serial_debug.println(" Hz");
        Serial_debug.println("--");
    }
    if (_btn3_rcl_flag)
    {
        _btn3_rcl_flag = 0;
        Serial_debug.println(" ");
        Serial_debug.print("Current ADC Record Status -> ");
        if (adc_capture_data_enable)
            Serial_debug.println("ENABLED");
        else
            Serial_debug.println("Disabled");
        Serial_debug.print("ADC sample time gap uS = ");
        Serial_debug.println(_adc_sample_time_gap);

        // for (int _c1 = 0; _c1 < _sample_size; _c1++)
        //     Serial_debug.println(volt_raw_data[_c1]);
        // Serial_debug.println("Voltage DATA");
        // for (int _c1 = 0; _c1 < _sample_size; _c1++)
        //     Serial_debug.println(current_raw_data[_c1]);
        // Serial_debug.println("Current DATA");
        // DSP processing function
        Serial_debug.println("DSP Print Enable one time");
        if (!auto_switch_VI_measure_mode)
        {
            if (_VI_measure_mode == 3)
                _VI_measure_mode = 6;
            else if (_VI_measure_mode == 6)
                _VI_measure_mode = 3;
            else
                _VI_measure_mode = 3;
        }
        Serial_debug.print("VI_measure_mode -> ");
        if (auto_switch_VI_measure_mode)
            Serial_debug.print("AUTO -> ");
        else
            Serial_debug.print("Manual -> ");
        Serial_debug.println(_VI_measure_mode);
        _dsp_serial_print0 = 1;
    }
}

void btn1_hold_update()
{
    _btn1_hold_flag = 1;
}
void btn2_sp_update()
{
    _btn2_sp_flag = 1;
}
void btn3_rcl_update()
{
    _btn3_rcl_flag = 1;
}

// VI Measure mode controlled by Timer 2 Interrupt
void set_measure_mode(int8_t _mode1)
{
    /*
    +----------------------+
    <1> - Discharge for Voltage Measure
    <2> - Set Voltage Mode
    <3> - Measure Voltage ****
    <4> - Discharge for Current Measure
    <5> - Set Current Mode
    <6> - Measure Current ****
    |---------------------------|
    | Discharge | Set | Measure |
    |---------------------------|
    */
    if (!auto_switch_VI_measure_mode)
        _mode1 -= 1; // To set control pins

    switch (_mode1)
    {
    case 1:
        // 1. Discharge for Voltage Measure
        VI_pin_state = LOW;
        GS_pin_state = LOW;
        break;
    case 2:
        // 2. Set Voltage Mode
        VI_pin_state = LOW;
        GS_pin_state = HIGH;
        break;
    case 3:
        // 3. Measure Voltage
        measure_volt_flag = 1; // Start Measurement
        measure_current_flag = 0;
        break;
    case 4:
        // 4. Discharge for Current Measure
        VI_pin_state = HIGH;
        GS_pin_state = LOW;
        break;
    case 5:
        // 5.Set Current Mode
        VI_pin_state = HIGH;
        GS_pin_state = HIGH;
        break;
    case 6:
        // 6. Measure Current
        measure_volt_flag = 0;
        measure_current_flag = 1; // Start Measurement
        break;
    default:
        // Add Fail safe step
        // Do Nothing -> Stop the ADC data capturing volt/current data
        measure_volt_flag = 0;
        measure_current_flag = 0;
        break;
    }

    if (_mode1 % 3 != 0)
    {
        // Stop Measurement
        measure_volt_flag = 0;
        measure_current_flag = 0;
        // Set the Mode
        digitalWrite(VI_pin, VI_pin_state);
        digitalWrite(GS_pin, GS_pin_state);
    }
    if (!auto_switch_VI_measure_mode)
    {
        if (_mode1 == 2)
        {
            // 3. Measure Voltage
            measure_volt_flag = 1; // Start Measurement
            measure_current_flag = 0;
        }
        else if (_mode1 == 5)
        {
            // 6. Measure Current
            measure_volt_flag = 0;
            measure_current_flag = 1; // Start Measurement
        }
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        int16_t adc_pa1 = HAL_ADC_GetValue(hadc);
        if (AFC_adc_capture_data_enable)
            store_AFC_data(adc_pa1); // Store Voltage data
    }
    if (hadc->Instance == ADC2)
    {
        int16_t adc_pa0 = HAL_ADC_GetValue(hadc);
        if (adc_capture_data_enable)
            if (measure_volt_flag || measure_current_flag)
                store_VI_data(adc_pa0); // Store Voltage data
    }
}

void store_VI_data(int16_t _sdata)
{
    // Circle or Ring Buffer to store data Volt, Current Measure data
    // NOTE: Here only memory pointing variable is used.
    _adc_sample_time_gap += (micros() - _timer3_record);
    _adc_sample_time_gap /= 2;
    _timer3_record = micros();
    if (measure_volt_flag)
    {
        volt_raw_data[volt_sample_count] = _sdata;
        if (volt_sample_count < _sample_size)
            volt_sample_count++;
        if (volt_sample_count >= _sample_size)
            volt_sample_count = 0;
    }
    else if (measure_current_flag)
    {
        current_raw_data[current_sample_count] = _sdata;
        if (current_sample_count < _sample_size)
            current_sample_count++;
        if (current_sample_count >= _sample_size)
            current_sample_count = 0;
    }
}

void store_AFC_data(int16_t _sdata)
{
    if (measure_volt_flag)
    {
        AFC_raw_data_v[AFC_sample_count_v] = _sdata;
        if (AFC_sample_count_v < _sample_size)
            AFC_sample_count_v++;
        if (AFC_sample_count_v >= _sample_size)
            AFC_sample_count_v = 0;
    }
    else if (measure_current_flag)
    {
        AFC_raw_data_i[AFC_sample_count_i] = _sdata;
        if (AFC_sample_count_i < _sample_size)
            AFC_sample_count_i++;
        if (AFC_sample_count_i >= _sample_size)
            AFC_sample_count_i = 0;
    }
}

void timer2_setup_VI()
{
    timer2_VI_switch.setPrescaleFactor(_timer2_prescaler); // 72MHZ/72000 = 10kHz
    timer2_VI_switch.setOverflow(2500);                    // Trigger every 250mS
    timer2_VI_switch.attachInterrupt(OnTimer2Interrupt);
    timer2_VI_switch.refresh();
    timer2_VI_switch.resume();
}

void OnTimer2Interrupt()
{
    // _timer2_int_counter++;
    // if (_VI_measure_mode % 3 != 0)
    // {
    //     c
    //     _timer2_int_counter = 0;
    // }
    // else if (_timer2_int_counter >= 2)
    // {
    //     _VI_measure_mode++;
    //     _timer2_int_counter = 0;
    // }
    if (auto_switch_VI_measure_mode)
    {
        _VI_measure_mode++;
        if (_VI_measure_mode == 7)
            _VI_measure_mode = 1;
    }
    set_measure_mode(_VI_measure_mode);
}

bool process_adc_data(int16_t *_volt_a, int16_t *_current_a, int16_t *_AFC_a_v, int16_t *_AFC_a_i, uint8_t _max_data_size)
{
    uint8_t error_voltage_adc_data = 0, error_current_adc_data = 0, error_AFC_adc_data_i = 0;
    int8_t _size_of_array = _max_data_size;

    error_voltage_adc_data = copy_raw_data_input(_volt_a, volt_adc_data, _size_of_array);
    error_current_adc_data = copy_raw_data_input(_current_a, current_adc_data, _size_of_array);
    error_AFC_adc_data_i = copy_raw_data_input(_AFC_a_v, AFC_adc_data_v, _size_of_array) || copy_raw_data_input(_AFC_a_i, AFC_adc_data_i, _size_of_array);
    // if (!error_voltage_adc_data && !error_current_adc_data)
    //     _data_copy_check = 1;
    // else
    //     _data_copy_check = 0;
    _data_copy_check = 1;

    if (_dsp_serial_print0)
    {
        Serial_debug.println("1. DATA Vaildation & Copy");
        Serial_debug.print("1.1 Voltage, Return: ");
        Serial_debug.println(error_voltage_adc_data);
        Serial_debug.print("1.2 Current, Return: ");
        Serial_debug.println(error_current_adc_data);
    }

    if (_dsp_serial_print0 && 1)
    {
        Serial_debug.println("---*");
        for (int _c1 = 0; _c1 < _sample_size; _c1++)
            Serial_debug.println(volt_adc_data[_c1]);
        Serial_debug.println("Voltage DATA");
        for (int _c1 = 0; _c1 < _sample_size; _c1++)
            Serial_debug.println(current_adc_data[_c1]);
        Serial_debug.println("Current DATA");
        // AFC Data
        Serial_debug.println("---*");
        for (int _c1 = 0; _c1 < _sample_size; _c1++)
            Serial_debug.println(AFC_adc_data_v[_c1]);
        Serial_debug.println("AFC DATA Volt");
        for (int _c1 = 0; _c1 < _sample_size; _c1++)
            Serial_debug.println(AFC_adc_data_i[_c1]);
        Serial_debug.println("AFC DATA Current");
    }

    if (_data_copy_check)
    {
        back_end_data.pk_pk_voltage = calculate_voltage(volt_adc_data, _adc_sample_time_gap, _size_of_array);
        back_end_data.pk_pk_current = calculate_voltage(current_adc_data, _adc_sample_time_gap, _size_of_array);
        back_end_data.pk_pk_AFC = calculate_voltage(AFC_adc_data_i, _adc_sample_time_gap, _size_of_array);

        if (_dsp_serial_print0)
        {
            Serial_debug.println("2. Measure Amplitude & Frequecy");
            Serial_debug.print("2.1 Voltage=");
            Serial_debug.print(back_end_data.pk_pk_voltage);
            Serial_debug.println(" V");
            Serial_debug.print("2.2 Current=");
            Serial_debug.print(back_end_data.pk_pk_current);
            Serial_debug.println(" V");
            Serial_debug.print("2.3 AFC=");
            Serial_debug.print(back_end_data.pk_pk_AFC);
            Serial_debug.println(" V");
            Serial_debug.println("2.4 Frequency(Hz)");
            Serial_debug.println(back_end_data.set_freq);
            Serial_debug.println("--");
            Serial_debug.println("--");
        }
    }
    else if (_dsp_serial_print0)
        Serial_debug.println("2. Error found in Copied Data");

    _dsp_serial_print0 = 0; // Reset the Serial print flag
    return 0;
}

#endif
