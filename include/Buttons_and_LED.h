/**
 * @file Buttons_and_LED.h
 * @brief Button and LED Configuation
 * Config 3 button with external interrupt enabled and LED Ctrl
 * 
 * @author Jaishankar M
 */

#include "stm32f1xx_hal.h"

// Public Variable Declaration


// Public Function Declaration
void setup_buttons_and_LED();
uint8_t get_buttons_status(uint8_t _pin_number);
void LED_control(uint8_t _led_state);