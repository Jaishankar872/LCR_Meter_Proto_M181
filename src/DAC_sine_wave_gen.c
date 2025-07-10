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
void set_sine_wave_frequency(uint16_t _set_frequency);
void Timer1_Init_DAC(void);
extern void Error_Handler(void);

/**
 * Timer 1 Interrupt
 * To Generator Sine Wave via digital pin PB0 to PB7.
 */

void sine_wave_setup()
{
    DAC_pinMode_B0_B7(0x2);       // Set as output mode(0x2 Hex)
    generate_sine_wave_data(1.0); // Calling Sine data generator
    Timer1_Init_DAC();
    set_sine_wave_frequency(1000); // Set Frequency
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

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
void Timer1_Init_DAC(void)
{

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    const uint32_t timer1_rate_Hz = DAC_sine_table_SIZE * 1000; // Default - 1kHz

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 2;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = (((HAL_RCC_GetHCLKFreq() / (htim1.Init.Prescaler + 1)) + (timer1_rate_Hz / 2)) / timer1_rate_Hz) - 1; // Default
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // Enable Timer 1 Interrupt
    HAL_TIM_Base_Start_IT(&htim1);
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

void set_sine_wave_frequency(uint16_t _set_frequency)
{
    const uint32_t timer1_rate_Hz = DAC_sine_table_SIZE * _set_frequency; // Default - 1kHz
    uint32_t timer1_period = (((HAL_RCC_GetHCLKFreq() / (htim1.Init.Prescaler + 1)) + (timer1_rate_Hz / 2)) / timer1_rate_Hz) - 1;

    if (_set_frequency > 0) // check point to ensure only +ve value only
    {
        htim1.Init.Period = timer1_period;
        // Rest of Config remains same as MX_TIM3_Init() function
        if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
        {
            Error_Handler();
        }
    }

    // Default Attenuation
    if (_set_frequency == 100)
        generate_sine_wave_data(0.7); // set_DAC_out_factor(30);
    else if (_set_frequency == 500)
        generate_sine_wave_data(0.85); // set_DAC_out_factor(15);
    else if (_set_frequency == 1000)
        generate_sine_wave_data(1.0); // set_DAC_out_factor(0);
}

void On_Timer1_Interrupt()
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
    HAL_TIM_Base_Start_IT(&htim1); // Start Timer 1 Interrupt
}
