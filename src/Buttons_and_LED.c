/**
 * @file Buttons_and_LED.h
 * @brief Button and LED Configuation
 * Config 3 button with external interrupt enabled and LED Ctrl
 *
 * @author Jaishankar M
 */

#include "Buttons_and_LED.h"

// Private Includes

// Private Variable Declaration
#define BTN1_HOLD_Pin GPIO_PIN_13
#define BTN1_HOLD_GPIO_Port GPIOB
#define BTN2_SP_Pin GPIO_PIN_14
#define BTN2_SP_GPIO_Port GPIOB
#define BTN3_RCL_Pin GPIO_PIN_15
#define BTN3_RCL_GPIO_Port GPIOB

#define LED_Pin GPIO_PIN_5
#define LED_pin_GPIO_Port GPIOA

volatile uint8_t _btn1_hold_flag = 0, _btn2_sp_flag = 0, _btn3_rcl_flag = 0;

// Private Function Declaration

void setup_buttons_and_LED()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pins : BTN1_HOLD_Pin BTN2_SP_Pin BTN3_RCL_Pin */
    GPIO_InitStruct.Pin = BTN1_HOLD_Pin | BTN2_SP_Pin | BTN3_RCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING; // Mode for Interrupt
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pins : LED_Pin GS_Pin VI_Pin */
    GPIO_InitStruct.Pin = LED_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    // Rest of interrupt config mentioned at
    // void EXTI15_10_IRQHandler(void) placed at stm32f1xx_it.c file
    // Don't add anything in the above function
    // Need to use HAL_GPIO_EXTI_Callback(GPIO_Pin);
}

// For the below check the delcare of HAL_GPIO_EXTI_IRQHandler
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN1_HOLD_Pin)
        _btn1_hold_flag = 1;
    if (GPIO_Pin == BTN2_SP_Pin)
        _btn2_sp_flag = 1;
    if (GPIO_Pin == BTN3_RCL_Pin)
        _btn3_rcl_flag = 1;
}

uint8_t get_buttons_status(uint8_t _pin_number)
{
    /**
     * _pin_number = 1 => Button 1 HOLD
     * _pin_number = 2 => Button 2 S/P
     * _pin_number = 3 => Button 2 RCL
     */
    uint8_t _return_status = 0;
    if (_pin_number == 1)
    {
        if (_btn1_hold_flag)
        {
            _return_status = 1;
            _btn1_hold_flag = 0; // Clear flag After reading
        }
        else
            _return_status = 0;
    }
    else if (_pin_number == 2)
    {
        if (_btn2_sp_flag)
        {
            _return_status = 1;
            _btn2_sp_flag = 0; // Clear flag After reading
        }
        else
            _return_status = 0;
    }
    else if (_pin_number == 3)
    {
        if (_btn3_rcl_flag)
        {
            _return_status = 1;
            _btn3_rcl_flag = 0; // Clear flag After reading
        }
        else
            _return_status = 0;
    }
    else
    {
        _return_status = 0;
    }

    return _return_status;
}

void LED_control(uint8_t _led_state)
{
    if (_led_state)
        LED_pin_GPIO_Port->BSRR = LED_Pin; // Set PA5 High
    else
        LED_pin_GPIO_Port->BRR = LED_Pin; // Set PA5 Low
}

uint8_t on_button_event(system_data *_control) // Pointer used to edit send value
{
    uint8_t _change_happen = 0;

    if (get_buttons_status(1)) // Button 1 HOLD
    {
        // _control the frequency
        uint16_t _frequency_array[4] = {100, 500, 800, 1000};

        if (_frequency_array[0] == _control->set_freq)
            _control->set_freq = _frequency_array[1];
        else if (_frequency_array[1] == _control->set_freq)
            _control->set_freq = _frequency_array[2];
        else if (_frequency_array[2] == _control->set_freq)
            _control->set_freq = _frequency_array[3];
        else if (_frequency_array[3] == _control->set_freq)
            _control->set_freq = _frequency_array[0];
        else
            _control->set_freq = _frequency_array[3];

        _change_happen = 1;
    }

    if (get_buttons_status(2)) // Button 2 S/P
    {
        // Yet to add
        _control->uart_all_print_DSO = !_control->uart_all_print_DSO;
        _change_happen = 1;
    }

    if (get_buttons_status(3)) // Button 3 RCL
    {
        // Yet to add
        _control->led_state = !_control->led_state;
        LED_control(_control->led_state);
        _change_happen = 1;
    }

    return _change_happen;
}