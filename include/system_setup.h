#ifndef SYSTEM_SETUP_H
#define SYSTEM_SETUP_H

#include "Arduino.h"
#include "DAC_sine_wave_gen.h"
#include <HardwareSerial.h>
#include <SoftWire.h>

// UART
HardwareSerial Serial_debug(PA_10, PA_9); //(RX, TX)
// I2C
SoftWire Wire2(PA3, PA4); // SDA = PA3, SCL = PA4

// Define and initialize global variables
float _fw0_version = 0.1;
bool ledstate = 1;
int test_frequency = 100;
volatile bool btn1_hold_flag = 0, btn2_sp_flag = 0, btn3_rcl_flag = 0;

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
    pinMode(AFC_pin, INPUT_ANALOG);

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

    Serial_debug.begin(115200);

    Wire2.begin();
    delay(1000);

    Serial_debug.print("Calling the Slave at 0x");
    Serial_debug.println(SCREEN_ADDRESS, HEX);
    Wire2.beginTransmission(SCREEN_ADDRESS);
    byte error1 = Wire2.endTransmission();
    if (error1 == 0)
        Serial_debug.println("Success!");
    else
    {
        Serial_debug.print("Failed with error code: ");
        Serial_debug.println(error1);
    }

    timer1_setup();
    DAC_sine_wave(50); // Frequency - 50Hz
}

void regular_task_loop()
{
    on_button_press_event();

    delay(10);
    // Serial_debug.print(SystemCoreClock);
}

void on_button_press_event()
{
    if (btn1_hold_flag || btn2_sp_flag || btn3_rcl_flag)
    {
        Serial_debug.println("-----");
        Serial_debug.print("FW Version: V");
        Serial_debug.println(_fw0_version);
        Serial_debug.println(" ");
    }
    if (btn1_hold_flag)
    {
        btn1_hold_flag = 0;
        ledstate = !ledstate;
        digitalWrite(LED_pin, !ledstate);

        // Serial Output
        Serial_debug.println("Pressed Button 1");
        if (!ledstate)
            Serial_debug.println("LED ON");
        else
            Serial_debug.println("LED OFF");
        Serial_debug.println("--");
    }
    if (btn2_sp_flag)
    {
        btn2_sp_flag = 0;
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
    if (btn3_rcl_flag)
    {
        btn3_rcl_flag = 0;

        // Serial Output
        Serial_debug.println("Pressed Button 3");
        Serial_debug.println("--");
    }
}

void btn1_hold_update()
{
    btn1_hold_flag = 1;
}
void btn2_sp_update()
{
    btn2_sp_flag = 1;
}
void btn3_rcl_update()
{
    btn3_rcl_flag = 1;
}

#endif
