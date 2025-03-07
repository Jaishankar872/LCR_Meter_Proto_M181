/**
 * @file Buttons_and_LED.h
 * @brief Button and LED Configuation
 * Config 3 button with external interrupt enabled and LED Ctrl
 * 
 * @author Jaishankar M
 */

#include "stm32f1xx_hal.h"
#include "system_data.h"

// Public Variable Declaration


// Public Function Declaration
void On_EXTI15_10_Interrupt(uint16_t GPIO_Pin);//Only for Interrupt callback
void setup_buttons_and_LED();
uint8_t on_button_event(system_data* _control);
void LED_control(uint8_t _led_state);