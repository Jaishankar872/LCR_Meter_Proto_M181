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
bool ledstate = 1;
uint8_t hex8 = 0x01;

#define SCREEN_ADDRESS 0x3C

#define ADC_pin PA0
#define AFC_pin PA1

#define LED_pin PA5
#define GS_pin PA6
#define VI_pin PA7

#define BTN1_HOLD PB13
#define BTN2_SP PB14
#define BTN3_RCL PB15

//Function Declaration
void int_system_setup();
void regular_task_loop();

/*----------------------------------------------------------------------*/

// Function Definition
void int_system_setup()
{
    pinMode(ADC_pin, INPUT);
    pinMode(AFC_pin, INPUT);
    pinMode(BTN1_HOLD, INPUT);
    pinMode(BTN2_SP, INPUT);
    pinMode(BTN3_RCL, INPUT);

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
    digitalWrite(LED_pin, ledstate);
    DAC_sine_wave((100 + (ledstate * 900)));
    hex8 = hex8 << 1;
    if (hex8 == 0)
        hex8 = 0x05;
    delay(1800);
    ledstate = !ledstate;
    // Serial_debug.print(SystemCoreClock);
    Serial_debug.println("Hello World!");
}

#endif
