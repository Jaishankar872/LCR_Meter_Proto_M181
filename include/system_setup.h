#include "Arduino.h"
#include "DAC_sine_wave_gen.h"
#include <HardwareSerial.h>

// UART
HardwareSerial Serial_debug(PA_10, PA_9); //(RX, TX)

#define ADC_pin PA0
#define AFC_pin PA1

#define LED_pin PA5
#define GS_pin PA6
#define VI_pin PA7

#define BTN1_HOLD PB13
#define BTN2_SP PB14
#define BTN3_RCL PB15

bool ledstate = 1;

uint8_t hex8 = 0x01;

void int_system_setup();
void regular_task_loop();

void int_system_setup()
{
    // Input pin Setup
    pinMode(ADC_pin, INPUT);
    pinMode(AFC_pin, INPUT);
    // Three input switch
    pinMode(BTN1_HOLD, INPUT);
    pinMode(BTN2_SP, INPUT);
    pinMode(BTN3_RCL, INPUT);

    // Output pin Setup
    DAC_pinMode_B0_B7(0x2); // Output push-pull, Max speed 2 MHz.
    pinMode(LED_pin, OUTPUT);
    pinMode(GS_pin, OUTPUT);
    pinMode(VI_pin, OUTPUT);

    //UART
    Serial_debug.begin(115200);
}

void regular_task_loop()
{
    digitalWrite(LED_pin, ledstate);
    // GPIOC->ODR = (GPIOC->ODR & 0xFFFFDFFF) | (ledstate << 13);
    //  Multiple GPIO controlling
    DAC_sine_wave(hex8);
    hex8 = hex8 << 1;
    if (hex8 == 0)
        hex8 = 0x05;
    delay(800);
    ledstate = !ledstate;
    Serial_debug.println("Hello World!");
}