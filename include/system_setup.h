#ifndef SYSTEM_SETUP_H
#define SYSTEM_SETUP_H

#include "Arduino.h"
#include "DAC_sine_wave_gen.h"
#include <HardwareSerial.h>

// UART
HardwareSerial Serial_debug(PA_10, PA_9); //(RX, TX)

// List of Variable to pass from System setup
struct system_data
{
    bool hold_btn, sp_btn, rcl_btn; // Tell the 3 button status(via polling)
    uint16_t set_freq;
    bool led_state;
    bool VI_measure_mode;
};
system_data back_end_data;

// Define and initialize global variables
float _fw0_version = 0.1;
bool ledstate = 1;
int test_frequency = 1000;
volatile bool _btn1_hold_flag = 0, _btn2_sp_flag = 0, _btn3_rcl_flag = 0;
bool GS_pin_state = 1, VI_pin_state = 0; // VI_measure_mode = 0;

// ADC Related function
TIM_HandleTypeDef htim3;
ADC_HandleTypeDef hadc1;

uint16_t timer3_prescaler = 72;
uint8_t adc_sample_rate = 20;
volatile bool adcChanged = 0;
unsigned long _timer3_record = 0, _adc_sample_time_gap = 0;

const unsigned int adc_sample_interval_pa0 = 100; // MicroSeconds
volatile bool print_if_adc_read = 0;

const int _sample_size = 40;
int volt_sample_count = 0, current_sample_count = 0;
int16_t volt_raw_data[40], current_raw_data[40];

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
void set_measure_mode(bool _mode1);

// ADC Related function
void store_VI_data(int16_t _sdata, bool _mode0);
void timer3_set_interval(int16_t _measure_freq, int _n_samples);
void setup_timer3_ADC_PA0();

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
    pinMode(AFC_pin, OUTPUT);
    digitalWrite(AFC_pin, HIGH);
    digitalWrite(GS_pin, GS_pin_state);
    digitalWrite(VI_pin, VI_pin_state);

    Serial_debug.begin(115200);

    timer1_setup();
    set_measure_mode(0);           // Set measure as Voltage
    DAC_sine_wave(test_frequency); // Set default Frequency

    // (void)analogRead(ADC_pin);     // use one analogRead to setup pin correctly in analog mode
    setup_timer3_ADC_PA0(); // kick off timer to generate TRGO events and setup ADC
    timer3_set_interval(test_frequency, adc_sample_rate);
    HAL_ADC_Start_IT(&hadc1); // Start the ADC conversion
    // timer2_time_gap(back_end_data.set_freq,10); // uSecs
}

void regular_task_loop()
{
    on_button_press_event();

    // Storing the data in passing variables
    back_end_data.led_state = !ledstate;
    back_end_data.set_freq = test_frequency;

    delay(10);
    // Serial_debug.print(SystemCoreClock);
}

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
        GS_pin_state = ledstate;
        digitalWrite(GS_pin, GS_pin_state);

        // Serial Output
        Serial_debug.println("Pressed Button 1");
        if (ledstate)
            Serial_debug.println("LED ON 1-10");
        else
            Serial_debug.println("LED OFF 1-1");
        Serial_debug.println("--");
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
        Serial_debug.print("ADC sample time gap = ");
        Serial_debug.println(_adc_sample_time_gap);
        for (int _c1 = 0; _c1 < _sample_size; _c1++)
            Serial_debug.println(volt_raw_data[_c1]);
    }
    if (0) //_btn3_rcl_flag) Disabled
    {
        _btn3_rcl_flag = 0;
        // Serial Output
        Serial_debug.println("Pressed Button 3");
        // back_end_data.VI_measure_mode = !back_end_data.VI_measure_mode;
        // set_measure_mode(back_end_data.VI_measure_mode);
        Serial_debug.println("--");
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
void set_measure_mode(bool _mode1)
{
    // 0 - Voltage Mode
    // 1 - Current mode
    if (!_mode1)
    {
        Serial_debug.println("Mode: Voltage, Att: 1-10");
        VI_pin_state = LOW;
        GS_pin_state = HIGH;
    }
    else
    {
        Serial_debug.println("Mode: Current, Att: 1-1");
        VI_pin_state = HIGH;
        GS_pin_state = LOW;
    }
    digitalWrite(VI_pin, VI_pin_state);
    digitalWrite(GS_pin, GS_pin_state);
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

/* interrupt handler for ADC */
extern "C" void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        int16_t adc_pa0 = HAL_ADC_GetValue(hadc);
        store_VI_data(adc_pa0, 0); // Store Voltage data
        adcChanged = 1;
    }
}

void store_VI_data(int16_t _sdata, bool _mode0)
{
    if (!_mode0)
    {
        _adc_sample_time_gap = micros() - _timer3_record;
        _timer3_record = micros();
        if (volt_sample_count < _sample_size)
        {
            volt_raw_data[volt_sample_count] = _sdata;
            volt_sample_count++;
        }
        else
        {
            // volt_sample_count = 0;
            for (int _c = 0; _c < (_sample_size - 1); _c++)
                volt_raw_data[_c] = volt_raw_data[_c + 1];
            volt_raw_data[_sample_size - 1] = _sdata;
        }
    }
}

#endif
