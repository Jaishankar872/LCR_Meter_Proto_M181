/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f1xx_hal.h"

void Error_Handler(void);

// Function Declaration for
// DAC Sine Wave Generator "DAC_sine_wave_gen.h"
extern void sine_wave_setup();
extern void set_sine_wave_frequency(uint16_t _set_frequency);

#define ADC_pin_Pin GPIO_PIN_0
#define ADC_pin_GPIO_Port GPIOA
#define AFC_pin_Pin GPIO_PIN_1
#define AFC_pin_GPIO_Port GPIOA

#define I2C_SDA_Pin GPIO_PIN_3
#define I2C_SDA_GPIO_Port GPIOA
#define I2C_SCL_Pin GPIO_PIN_4
#define I2C_SCL_GPIO_Port GPIOA

#define LED_pin_Pin GPIO_PIN_5
#define LED_pin_GPIO_Port GPIOA

#define GS_pin_Pin GPIO_PIN_6
#define GS_pin_GPIO_Port GPIOA
#define VI_pin_Pin GPIO_PIN_7
#define VI_pin_GPIO_Port GPIOA

#define BTN1_HOLD_Pin GPIO_PIN_13
#define BTN1_HOLD_GPIO_Port GPIOB
#define BTN2_SP_Pin GPIO_PIN_14
#define BTN2_SP_GPIO_Port GPIOB
#define BTN3_RCL_Pin GPIO_PIN_15
#define BTN3_RCL_GPIO_Port GPIOB

#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
