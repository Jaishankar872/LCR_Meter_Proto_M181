#ifndef SYSTEM_SETUP_H
#define SYSTEM_SETUP_H

#include "Arduino.h"
#include "DAC_sine_wave_gen.h"
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
    float pk_pk_voltage, pk_pk_current;
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

// ADC Related function
TIM_HandleTypeDef htim3;
ADC_HandleTypeDef hadc1;

volatile bool adc_capture_data_enable = 1;
uint16_t timer3_prescaler = 8;
uint8_t adc_sample_rate = 40;
volatile bool volt_data_record_ready = 0, current_data_record_ready = 0;
unsigned long _timer3_record = 0, _adc_sample_time_gap = 0;

const unsigned int adc_sample_interval_pa0 = 100; // MicroSeconds
volatile bool print_if_adc_read = 0;

const int _sample_size = 80;
int volt_sample_count = 0, current_sample_count = 0;
bool measure_volt_flag = 0, measure_current_flag = 0;
int16_t volt_raw_data[80], current_raw_data[80];

#define SCREEN_ADDRESS 0x3C

#define ADC_pin PA0
#define AFC_pin PA1

#define LED_pin PA5
#define GS_pin PA6
#define VI_pin PA7

#define BTN1_HOLD PB13
#define BTN2_SP PB14
#define BTN3_RCL PB15

// Function Declaration
void int_system_setup(float _fw_ver);
void regular_task_loop();
void btn1_hold_update();
void btn2_sp_update();
void btn3_rcl_update();
void on_button_press_event();
void set_measure_mode(int8_t _mode1);

// ADC Related function
void store_VI_data(int16_t _sdata);
void timer3_set_interval(int16_t _measure_freq, int _n_samples);
void setup_timer3_ADC_PA0();

void timer2_setup_VI();
void OnTimer2Interrupt();
bool process_adc_data(int16_t *_volt_a, int16_t *_current_a, uint8_t _max_data_size);

// Variable Declaration - For DSP
int8_t _data_copy_check = 0;
int16_t volt_adc_data[80], current_adc_data[80];
bool adc_data_process_going = 0;
bool _dsp_serial_print0 = 0;

/*----------------------------------------------------------------------*/

// Function Definition
void int_system_setup(float _fw_ver)
{
    _fw0_version = _fw_ver;
    // ADC Input pin
    pinMode(ADC_pin, INPUT_ANALOG);

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
    pinMode(AFC_pin, INPUT); // As per the testing JYETech FW.

    digitalWrite(LED_pin, ledstate);
    // digitalWrite(AFC_pin, LOW); // As per the testing JYETech FW.
    digitalWrite(GS_pin, GS_pin_state);
    digitalWrite(VI_pin, VI_pin_state);

    Serial_debug.begin(115200);

    timer1_setup();
    set_measure_mode(0);           // Set measure as Voltage
    DAC_sine_wave(test_frequency); // Set default Frequency
    back_end_data.set_freq = test_frequency;

    // (void)analogRead(ADC_pin);     // use one analogRead to setup pin correctly in analog mode
    setup_timer3_ADC_PA0(); // kick off timer to generate TRGO events and setup ADC
    timer3_set_interval(test_frequency, adc_sample_rate);
    HAL_ADC_Start_IT(&hadc1); // Start the ADC conversion

    timer2_setup_VI();
}

void regular_task_loop()
{
    on_button_press_event();

    // Storing the data in passing variables
    back_end_data.VI_measure_mode = _VI_measure_mode; // Optional
    if (_VI_measure_mode == 1 || !auto_switch_VI_measure_mode)
    {
        adc_capture_data_enable = 0; // STOP: ADC Capturing the data
        process_adc_data(volt_raw_data, current_raw_data, _sample_size);
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
        timer3_set_interval(test_frequency, adc_sample_rate);
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
            if (_VI_measure_mode == 3)
                _VI_measure_mode = 6;
            else if (_VI_measure_mode == 6)
                _VI_measure_mode = 3;
            else
                _VI_measure_mode = 3;
        Serial_debug.print("VI_measure_mode -> ");
        if (auto_switch_VI_measure_mode)
            Serial_debug.print("AUTO -> ");
        else
            Serial_debug.print("Manunal -> ");
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

// ADC Related function
void timer3_set_interval(int16_t _measure_freq, int _n_samples)
{
    if (_measure_freq > 0)
    {
        int _time_us = 1000000 / (_measure_freq * _n_samples); // Microseconds
        uint32_t _temp2_ovf = CPU_FREQ / timer3_prescaler;     // Timer frequency
        _temp2_ovf *= _time_us;

        _temp2_ovf /= 1000; // uS -> mS
        _temp2_ovf /= 1000; // mS -> Seconds

        htim3.Instance = TIM3;
        htim3.Init.Prescaler = timer3_prescaler - 1; //
        htim3.Init.Period = _temp2_ovf - 1;          // Trigger TRGO
        htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
        htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

        TIM_ClockConfigTypeDef sClockSourceConfig = {0};
        sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
        HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig);

        TIM_MasterConfigTypeDef masterConfig = {0};
        masterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // Trigger on update
        masterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

        HAL_TIM_Base_Init(&htim3);
        HAL_TIMEx_MasterConfigSynchronization(&htim3, &masterConfig);
        HAL_TIM_Base_Start(&htim3);

        if (1)
        {
            Serial_debug.println("Timer 3 Setup Overflow");
            Serial_debug.print(_measure_freq);
            Serial_debug.println(" Hz");

            Serial_debug.print(_n_samples);
            Serial_debug.println(" Samples");

            Serial_debug.print(_time_us);
            Serial_debug.println(" uS");

            Serial_debug.print(_temp2_ovf);
            Serial_debug.println(" OVerflow");
        }
    }
    else
    {
        Serial_debug.println("Wrong Timer3 Value, pass correct value");
    }
}

// Interrupt service routine for the timer
void setup_timer3_ADC_PA0()
{
    /*
      1. Setting up Timer 3 to Output TRGO
      2. Setting up ADC to Trigger on Timer 3 TRGO
      3. From the ADC Conversion Complete Interrupt I will store the ADC data
      4. Start the ADC
   */
    // Setup ADC to Trigger on Timer 3 TRGO
    __HAL_RCC_ADC1_CLK_ENABLE();
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO; // Triggered by Timer 3 TRGO
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef adcChannelConfig = {0};
    adcChannelConfig.Channel = ADC_CHANNEL_0; // Selected pin PA0
    adcChannelConfig.Rank = ADC_REGULAR_RANK_1;
    adcChannelConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &adcChannelConfig);
    //--------------------------------------------------------------------------------------------
    // Setting up Timer 3 to Output TRGO
    __HAL_RCC_TIM3_CLK_ENABLE();

    timer3_set_interval(100, 10);

    //--------------------------------------------------------------------------------------------
    // Enable interrupt for ADC1_2
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
    //--------------------------------------------------------------------------------------------
}

// interrupt handler for ADC
extern "C" void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        int16_t adc_pa0 = HAL_ADC_GetValue(hadc);
        if (adc_capture_data_enable)
            if (measure_volt_flag || measure_current_flag)
                store_VI_data(adc_pa0); // Store Voltage data
    }
}

void store_VI_data(int16_t _sdata)
{
    _adc_sample_time_gap += (micros() - _timer3_record);
    _adc_sample_time_gap /= 2;
    _timer3_record = micros();
    if (measure_volt_flag)
    {
        if (volt_sample_count < _sample_size)
        {
            volt_raw_data[volt_sample_count] = _sdata;
            volt_sample_count++;
        }
        else
        {
            volt_sample_count = 0;
            // for (int _c = 0; _c < (_sample_size - 1); _c++)
            //     volt_raw_data[_c] = volt_raw_data[_c + 1];
            // volt_raw_data[_sample_size - 1] = _sdata;
        }
    }
    else if (measure_current_flag)
    {
        if (current_sample_count < _sample_size)
        {
            current_raw_data[current_sample_count] = _sdata;
            current_sample_count++;
        }
        else
        {
            current_sample_count = 0;
            // for (int _c = 0; _c < (_sample_size - 1); _c++)
            //     current_raw_data[_c] = current_raw_data[_c + 1];
            // current_raw_data[_sample_size - 1] = _sdata;
        }
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

bool process_adc_data(int16_t *_volt_a, int16_t *_current_a, uint8_t _max_data_size)
{
    uint8_t error_voltage_adc_data = 0, error_current_adc_data = 0;
    int8_t _size_of_array = _max_data_size;

    error_voltage_adc_data = copy_raw_data_input(_volt_a, volt_adc_data, _size_of_array);
    error_current_adc_data = copy_raw_data_input(_current_a, current_adc_data, _size_of_array);
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
    }

    if (_data_copy_check)
    {
        back_end_data.pk_pk_voltage = calculate_voltage(volt_adc_data, _adc_sample_time_gap, _size_of_array);
        back_end_data.pk_pk_current = calculate_voltage(current_adc_data, _adc_sample_time_gap, _size_of_array);

        if (_dsp_serial_print0)
        {
            Serial_debug.println("2. Measure Amplitude & Frequecy");
            Serial_debug.print("2.1 Voltage=");
            Serial_debug.print(back_end_data.pk_pk_voltage);
            Serial_debug.println(" V");
            Serial_debug.print("2.2 Current=");
            Serial_debug.print(back_end_data.pk_pk_current);
            Serial_debug.println(" V");
        }
    }
    else if (_dsp_serial_print0)
        Serial_debug.println("2. Error found in Copied Data");

    _dsp_serial_print0 = 0; // Reset the Serial print flag
    return 0;
}

#endif
