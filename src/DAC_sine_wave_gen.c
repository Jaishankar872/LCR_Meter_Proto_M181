/**
 * @file DAC_sine_wave_gen.c
 * @brief DAC Sine Wave Generator
 * This file contains the function definition for DAC Sine Wave Generator
 * Via Timer 1 Interrupt
 */

#include "DAC_sine_wave_gen.h"
#include <math.h>

// Include Header Files
#include "stm32f1xx_hal_tim.h"

// Timer
TIM_HandleTypeDef htim1;
uint16_t _timer1_prescaler = 2;

// Private Variables
int sine_data[100];
uint16_t _no_of_sample_per_sine = 100;
uint16_t DAC_resolution = 256; // 2^8=256;
int _pos_sine_data = 0;

// Private Function Declaration
void generate_sine_wave_data();
void DAC_pinMode_B0_B7(uint8_t _pinmode0);
void DAC_analogWrite_B0_B7(uint8_t _dat1);
void set_sine_wave_frequency(uint16_t _set_frequency);
void timer1_setup(void);
extern void Error_Handler(void);

/**
 * Timer 1 Interrupt
 * To Generator Sine Wave via digital pin PB0 to PB7.
 * Timer Specs
 *      - Prescaler: 2
 *      - Interval: (1/frequency)*(1/100)
 */

void sine_wave_setup()
{
    DAC_pinMode_B0_B7(0x2);
    generate_sine_wave_data(); // Calling Sine data generator
    timer1_setup();
    set_sine_wave_frequency(1000); // Set Frequency
}

void generate_sine_wave_data()
{
    int _max_DAC_value = DAC_resolution - 1;
    float _step_value_k = (2 * 3.141 * 1000) / _no_of_sample_per_sine; // x1000

    for (int i = 0; i < _no_of_sample_per_sine; i++)
    {
        float _temp_cal_value = (float)(i * _step_value_k) / 1000;
        sine_data[i] = (_max_DAC_value / 2) + (_max_DAC_value / 2 * sin(_temp_cal_value)); // calculating the sin value at each instance
    }
}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
void timer1_setup(void)
{

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = _timer1_prescaler;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 359; // Default Set for 1kHz Measurement
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

    uint32_t window_time_us = 0;
    uint32_t timer1_period = 0;

    float APB1_Timer_clock_set_Time_nS = 20.9;//48MHz = 20.9nS; //[Not Working] 72MHz = 13.889nS;
    float After_timer1_prescaler_time_nS = APB1_Timer_clock_set_Time_nS * _timer1_prescaler;

    if (_set_frequency > 0)
    {
        window_time_us = 1000000 / _set_frequency; // Microseconds
        window_time_us /= _no_of_sample_per_sine;

        timer1_period = window_time_us * 1000;           // Convert into nS into uS
        timer1_period /= After_timer1_prescaler_time_nS; // Timer frequency

        htim1.Init.Prescaler = _timer1_prescaler;
        htim1.Init.Period = timer1_period;
        /** [Calculation] For Timer Clock 48MHz, Prescaler = 2, Sample per Cycle = 100
         * 1000Hz => Period = 240 -1 = 239
         * 500Hz => Period = 480 -1 = 479
         * 100Hz => Period = 2400 -1 = 2399
         */
        // Rest of Config remains same as MX_TIM3_Init() function
        if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
        {
            Error_Handler();
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        DAC_analogWrite_B0_B7(sine_data[_pos_sine_data]);
        if (_pos_sine_data >= (_no_of_sample_per_sine - 1))
            _pos_sine_data = 0;
        else
            _pos_sine_data++;
    }
}

void DAC_analogWrite_B0_B7(uint8_t _dat1)
{
    GPIOB->ODR = (GPIOB->ODR & 0xFFFFFF00) | (_dat1);
}
