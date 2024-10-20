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
int test_frequency = 100;
volatile bool _btn1_hold_flag = 0, _btn2_sp_flag = 0, _btn3_rcl_flag = 0;
bool GS_pin_state = 1, VI_pin_state = 0;// VI_measure_mode = 0;

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
    DAC_sine_wave(50); // Frequency - 50Hz
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
    if (_btn1_hold_flag || _btn2_sp_flag || _btn3_rcl_flag)
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
            test_frequency = 512;
        else if (test_frequency == 512)
            test_frequency = 1000;
        else if (test_frequency == 1000)
            test_frequency = 100;
        DAC_sine_wave(test_frequency);

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
        // Serial Output
        Serial_debug.println("Pressed Button 3");
       back_end_data.VI_measure_mode = !back_end_data.VI_measure_mode;
        if (back_end_data.VI_measure_mode)
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

#endif
