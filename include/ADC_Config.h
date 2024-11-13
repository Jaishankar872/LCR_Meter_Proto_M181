/*
Setup and Config the ADC pin trigger
*/
#ifndef ADC_CONFIG_H
#define ADC_CONFIG_H

#include "Arduino.h"

// Variables
TIM_HandleTypeDef htim3;
ADC_HandleTypeDef hadc1_PA1, hadc2_PA0;

uint16_t timer3_prescaler = 8;
uint8_t adc_sample_rate = 40;

const unsigned int adc_sample_interval_pa0 = 100; // MicroSeconds
bool ADC_interrupt_one_time_config = 1;
bool timer3_one_time_config = 1;

// Function Declaration
int timer3_set_interval(int16_t _measure_freq, int _n_samples);
void setup_ADC1_PA1();
void setup_ADC2_PA0();

/*----------------------------------------------------------------------*/

// Function Definition

// Setup Timer3 ---> ADC2
int timer3_set_interval(int16_t _measure_freq, int _n_samples)
{
    int _time_us = 0;
    if (_measure_freq > 0)
    {
        _time_us = 1000000 / (_measure_freq * _n_samples); // Microseconds
        uint32_t _temp2_ovf = CPU_FREQ / timer3_prescaler; // Timer frequency
        _temp2_ovf *= _time_us;

        _temp2_ovf /= 1000; // uS -> mS
        _temp2_ovf /= 1000; // mS -> Seconds

        if (timer3_one_time_config)
            __HAL_RCC_TIM3_CLK_ENABLE();

        htim3.Instance = TIM3;
        htim3.Init.Prescaler = timer3_prescaler - 1; //
        htim3.Init.Period = _temp2_ovf - 1;          // Trigger TRGO
        htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
        htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

        TIM_ClockConfigTypeDef sClockSourceConfig = {0};
        sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
        HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig);

        TIM_MasterConfigTypeDef masterConfig = {0};
        masterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // Trigger on update
        masterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

        // if (timer3_one_time_config)
        HAL_TIM_Base_Init(&htim3);
        HAL_TIMEx_MasterConfigSynchronization(&htim3, &masterConfig);
        HAL_TIM_Base_Start(&htim3);

        timer3_one_time_config = 0; // Reset flag
    }
    else
    {
        return -1;
    }
    return _time_us;
}

// Setup ADC1 -> PA1 pin
void setup_ADC1_PA1()
{
    // ADC1 used to meaursing the GA pin (Voltage/Current).
    /*
      1. Setting up Timer 3 to Output TRGO
      2. Setting up ADC1 to Trigger on Timer 3 TRGO
      3. From the ADC1 Conversion Complete Interrupt I will store the ADC data
      4. Start the ADC1
   */
    // Setup ADC to Trigger on Timer 3 TRGO
    __HAL_RCC_ADC1_CLK_ENABLE();
    hadc1_PA1.Instance = ADC1;
    hadc1_PA1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1_PA1.Init.ContinuousConvMode = DISABLE;
    hadc1_PA1.Init.DiscontinuousConvMode = DISABLE;
    hadc1_PA1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO; // Triggered by Timer 3 TRGO
    hadc1_PA1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1_PA1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1_PA1);

    ADC_ChannelConfTypeDef adcChannelConfig = {0};
    adcChannelConfig.Channel = ADC_CHANNEL_1; // Selected pin PA1
    adcChannelConfig.Rank = ADC_REGULAR_RANK_1;
    adcChannelConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1_PA1, &adcChannelConfig);
    //--------------------------------------------------------------------------------------------

    // Enable interrupt for ADC1_2

    if (ADC_interrupt_one_time_config)
    {
        HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
        ADC_interrupt_one_time_config = 0;
    }

    HAL_ADC_Start_IT(&hadc1_PA1); // Start the ADC conversion
}

// Setup ADC2 -> PA0 pin
void setup_ADC2_PA0()
{
    // ADC2 used to meaursing the GA pin (Voltage/Current).
    /*
      1. Setting up Timer 3 to Output TRGO
      2. Setting up ADC2 to Trigger on Timer 3 TRGO
      3. From the ADC2 Conversion Complete Interrupt I will store the ADC data
      4. Start the ADC2
   */
    // Setup ADC to Trigger on Timer 3 TRGO
    __HAL_RCC_ADC2_CLK_ENABLE();
    hadc2_PA0.Instance = ADC2;
    hadc2_PA0.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc2_PA0.Init.ContinuousConvMode = DISABLE;
    hadc2_PA0.Init.DiscontinuousConvMode = DISABLE;
    hadc2_PA0.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO; // Triggered by Timer 3 TRGO
    hadc2_PA0.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc2_PA0.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc2_PA0);

    ADC_ChannelConfTypeDef adcChannelConfig = {0};
    adcChannelConfig.Channel = ADC_CHANNEL_0; // Selected pin PA0
    adcChannelConfig.Rank = ADC_REGULAR_RANK_1;
    adcChannelConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc2_PA0, &adcChannelConfig);
    //--------------------------------------------------------------------------------------------

    // Enable interrupt for ADC1_2
    if (ADC_interrupt_one_time_config)
    {
        HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
        ADC_interrupt_one_time_config = 0;
    }
    //--------------------------------------------------------------------------------------------
    HAL_ADC_Start_IT(&hadc2_PA0); // Start the ADC conversion
}

// interrupt handler for ADC
// Check the function -> void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
extern "C" void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1_PA1);
    HAL_ADC_IRQHandler(&hadc2_PA0);
}

#endif