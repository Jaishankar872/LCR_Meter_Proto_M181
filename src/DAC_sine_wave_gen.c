/**
 * @file DAC_sine_wave_gen.c
 * @brief DAC Sine Wave Generator
 * This file contains the function definition for DAC Sine Wave Generator
 * Via Timer 1 Interrupt
 *
 * @author Jaishankar M
 * Credits: OneOfEleven
 */

#include "DAC_sine_wave_gen.h"
#include <math.h>

// Include Header Files
#include "stm32f1xx_hal_tim.h"

// Timer
TIM_HandleTypeDef htim1;

// Private Variables

// DAC Config
const uint16_t DAC_resolution = 256;            // 2^8=256; - Fixed
float DAC_sine_wave_amplitude = 1.0;            // 0.0 = 0%, 1.0 = 100%, -1.0 = 100% phase inverted
volatile unsigned int DAC_sine_table_index = 0; // Has current index value

#define DAC_sine_table_SIZE (DMA_ADC_DATA_LENGTH / no_of_sine_wave_cycle_per_data)
uint8_t DAC_sine_table[DAC_sine_table_SIZE] = {0}; // matched to the ADC sampling

// Private Function Declaration
void generate_sine_wave_data(const float amplitude);
void DAC_pinMode_B0_B7(uint8_t _pinmode0);
void DAC_analogWrite_B0_B7(uint8_t _dat1);
extern void Error_Handler(void);

/**
 * Timer 1 Interrupt
 * To Generator Sine Wave via digital pin PB0 to PB7.
 */

void sine_wave_setup()
{
    DAC_pinMode_B0_B7(0x2);       // Set as output mode(0x2 Hex)
    generate_sine_wave_data(1.0); // Calling Sine data generator
}

void generate_sine_wave_data(const float amplitude)
{
    // fill the look-up buffer with one complete sine cycle

    // value will be limited from -1.0 to +1.0
    // 0.0 = 0%, 1.0 = 100%, -1.0 = 100% phase inverted
    DAC_sine_wave_amplitude = (amplitude < -1.0f) ? -1.0f : (amplitude > 1.0f) ? 1.0f
                                                                               : amplitude;

    const float _scale = (DAC_resolution - 1) * DAC_sine_wave_amplitude * 0.5f;
    const float _phase_step = (float)(2.0 * M_PI) / DAC_sine_table_SIZE;

    for (unsigned int i = 0; i < DAC_sine_table_SIZE; i++)
        DAC_sine_table[i] = (uint8_t)floorf(((1.0f + sinf(_phase_step * i)) * _scale) + 0.5f); // raised sine
}

void DAC_pinMode_B0_B7(uint8_t _pinmode0)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // Enable clock for GPIOB
    // This frees PB3 and PB4 for general-purpose I/O
    AFIO->MAPR &= ~(AFIO_MAPR_SWJ_CFG); // Clear the SWJ_CFG bits
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1;  // Set SWJ_CFG to disable JTAG but keep SWD

    GPIOB->CRL &= ~(0xFFFFFFFF); // Clear configuration for PB0-PB7
    uint32_t _data0 = 0x0;
    for (int c = 0; c < 8; c++)
        _data0 |= (_pinmode0 << (c * 4));
    GPIOB->CRL |= _data0; // Set PB0-PB7 Mode
                          //----
}

void On_Timer3_Interrupt()
{
    unsigned int _index = DAC_sine_table_index;
    DAC_analogWrite_B0_B7(DAC_sine_table[_index]);

    // next index value
    DAC_sine_table_index = (++_index >= DAC_sine_table_SIZE) ? 0 : _index;
}

void DAC_analogWrite_B0_B7(uint8_t _dat1)
{
    GPIOB->ODR = (GPIOB->ODR & 0xFFFFFF00) | (_dat1);
}

void manual_ctrl_DAC(uint8_t _dac_output) // DAC Supports input from [0 to 255] only
{
    HAL_TIM_Base_Stop_IT(&htim1); // Stop Timer 1 Interrupt
    DAC_analogWrite_B0_B7(_dac_output);
}

void release_manual_ctrl_DAC()
{
    // HAL_TIM_Base_Start_IT(&htim1); // Start Timer 1 Interrupt
}
