/**
 * @file ADC_Config_DMA.c
 * @brief ADC Configuration with DMA
 * This file contains the function definition for ADC Configuration with DMA
 *
 * Core Functionality:
 * Timer3 >-> Trigger[TRGO] >-> ADC1 -> DMA1
 *       -> Memory >-> Interrupt[Full] >-> Create a Copy of the Data
 *
 * Developed by: Jaishankar M
 */
#include "ADC_Config_DMA.h"

// Include Header Files - Private
// #include "stm32f1xx_hal_dma.h"
// #include "stm32f1xx_hal_adc.h"
// #include "stm32f1xx_hal_tim.h"

// Private Variable Declaration
ADC_HandleTypeDef hadc1, hadc2;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim3;

// Private Variable Declaration
uint16_t raw_adc_DMA_data[DMA_ADC_data_length * 2];
volatile uint8_t adc_read_complete_flag_DMA = 0;

// Private Function Declaration
void Timer3_Init_ADC();
void DMA_Init_ADC();
void separate_ADC_CH_from_DMA();

void ADC_Init_PA0();
void ADC_Init_PA1();
extern void Error_Handler(void);

// Function Definition
void setup_ADC_with_DMA()
{
    DMA_Init_ADC();
    ADC_Init_PA0();
    Timer3_Init_ADC();

    // ADC_Init_PA1(); // To be implemented
    HAL_TIM_Base_Start(&htim3);

    set_ADC_Measure_window(1000); // 1 KHz
}

void Timer3_Init_ADC()
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim3.Instance = TIM3; // Timer 3 ** Missed this line development face
    htim3.Init.Prescaler = 8;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 270; // Default Set for 1kHZ Measurement, 20% as Extra Buffer
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // Rest of the Config remains mentioned as
    // void void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base)
}

void ADC_Init_PA0()
{
    /**
     * 1. Setting up Timer 3 to Output TRGO
     * 2. Setting up ADC1 to Trigger on Timer 3 TRGO
     * 3. From the ADC1 Conversion Complete Interrupt I will store the ADC data
     * 4. Start the ADC1
     * */
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    // Call the HAL_ADC_MspInit() function in the HAL_ADC_Init() function

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // Rest of the Config remains mentioned as
    // void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)

    // HAL_ADC_Start_IT(&hadc1); // Start the ADC conversion with Interrupt
    // Disable the ADC Interrupt at stm32f1xx_hal_msp.c file

    // Start the ADC with DMA
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)raw_adc_DMA_data, DMA_ADC_data_length);

    // Rest of the Config remains mentioned as
    // void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) placed at stm32f1xx_hal_msp.c file

}
void ADC_Init_PA1()
{
}

void DMA_Init_ADC()
{

    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Channel1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0); // ADC Data
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);


}

void set_ADC_Measure_window(uint16_t _measure_frequency)
{
    float window_time_us = 0;
    uint32_t timer3_period = 0; // 32 bit is needed -to handle number Big number on Calculation
    uint8_t timer3_prescaler = 8;
    uint8_t adc_sample_rate = 40;

    float APB1_Timer_clock_set_Time_nS = 13.889; // 10^9/APB1_Timer_clock_set = 13.889nS;
    float After_timer3_prescaler_time_nS = APB1_Timer_clock_set_Time_nS * timer3_prescaler;

    if (_measure_frequency > 0)
    {
        window_time_us = 1000000 / _measure_frequency; // Microseconds
        window_time_us /= adc_sample_rate;
        window_time_us *= 1.2; // 20% as Extra Buffer

        timer3_period = window_time_us * 1000;           // Convert into uS into nS
        timer3_period /= After_timer3_prescaler_time_nS; // Timer frequency

        htim3.Init.Prescaler = timer3_prescaler;
        htim3.Init.Period = timer3_period; // 1 KHz
        // Rest of Config remains same as MX_TIM3_Init() function
        if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
        {
            Error_Handler();
        }
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    /* For HAL Driver with transfer by DMA:
      ◦ Activate the ADC peripheral and start conversions using function HAL_ADC_Start_DMA()
      ◦ Wait for ADC conversion completion by call of function HAL_ADC_ConvCpltCallback() or HAL_ADC_ConvHalfCpltCallback() (these functions must be implemented in user program)
      ◦ Conversion results are automatically transferred by DMA into destination variable address.
      ◦ Stop conversion and disable the ADC peripheral using function HAL_ADC_Stop_DMA()
      */

    if (hadc->Instance == ADC1)
    {
        HAL_ADC_Stop_DMA(&hadc1);
        GPIOA->BRR = GPIO_PIN_5; // Set PA5 Low
        separate_ADC_CH_from_DMA();
        adc_read_complete_flag_DMA = 1;
    }

    // Now will take over by DMA
}

void separate_ADC_CH_from_DMA()
{
    for (int i = 0; i < DMA_ADC_data_length; i++)
        adc_pa0_DMA_data[i] = raw_adc_DMA_data[i];
    adc_PA0_data_ready_flag = 1;
}

void Start_ADC_Conversion(){
    // Restart the ADC
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)raw_adc_DMA_data, DMA_ADC_data_length);
    GPIOA->BSRR = GPIO_PIN_5; // Set PA5 High
}