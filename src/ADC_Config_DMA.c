/**
 * @file ADC_Config_DMA.c
 * @brief ADC Configuration with DMA
 * This file contains the function definition for ADC Configuration with DMA
 *
 * Core Functionality:
 * Timer3 >-> Trigger[TRGO] >-> ADC1 -> DMA1
 *       -> Memory >-> Interrupt[Full] >-> Create a Copy of the Data
 *
 * @author Jaishankar M
 */
#include "ADC_Config_DMA.h"

// Include Header Files - Private
// #include "stm32f1xx_hal_dma.h"
// #include "stm32f1xx_hal_adc.h"
// #include "stm32f1xx_hal_tim.h"

// Private Variable Declaration
ADC_HandleTypeDef hadc1, hadc2;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim2, htim3;

#define GS_Pin GPIO_PIN_6
#define GS_pin_GPIO_Port GPIOA
#define VI_Pin GPIO_PIN_7
#define VI_pin_GPIO_Port GPIOA
#define HIGH 1
#define LOW 0

// Private Variable Declaration
uint16_t raw_adc_DMA_data[DMA_ADC_data_length * 2];
volatile uint8_t adc_read_complete_flag_DMA = 0;

uint8_t measure_volt_flag = 0, measure_current_flag = 0;
uint8_t GS_pin_state = 1, VI_pin_state = 0; // VI_measure_mode = 0;
volatile uint8_t _VI_measure_mode = 1, auto_switch_VI_measure_mode = 1;

// Private Function Declaration
void Timer3_Init_ADC();
void DMA_Init_ADC();
void separate_ADC_CH_from_DMA();
void Start_ADC_Conversion();

void GPIO_Init_VI_GS_Pin();
void Timer2_Init_VI_switch(void);
void Start_Timer_VI_switch();
void Stop_Timer_VI_switch();
void set_measure_mode(int8_t _mode1);

void ADC_Init_PA0_PA1();
extern void Error_Handler(void);

// Function Definition
void setup_ADC_with_DMA()
{
    // Initiate ADC with Timer 3
    DMA_Init_ADC();
    ADC_Init_PA0_PA1();
    Timer3_Init_ADC();

    // Initiate VI Pin with Timer 2
    GPIO_Init_VI_GS_Pin();
    Timer2_Init_VI_switch();
    Start_Timer_VI_switch();

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

    // Start the Timer3
    HAL_TIM_Base_Start(&htim3);
}

void ADC_Init_PA0_PA1()
{
    /**
     * 1. Setting up Timer 3 to Output TRGO
     * 2. Setting up ADC1 to Trigger on Timer 3 TRGO
     * 3. From the ADC1 Conversion Complete Interrupt I will store the ADC data
     * 4. Start the ADC1
     * */
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 2; // For Two Channel
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_0; // PA0 Pin
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_1; // PA1 Pin
    sConfig.Rank = ADC_REGULAR_RANK_2;
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
    // HAL_ADC_Start_DMA(&hadc1, (uint32_t *)raw_adc_DMA_data, DMA_ADC_data_length);

    // Rest of the Config remains mentioned as
    // void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) placed at stm32f1xx_hal_msp.c file
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
        separate_ADC_CH_from_DMA();
    }

    // Now will take over by DMA
}

void separate_ADC_CH_from_DMA()
{
    if (measure_volt_flag && !measure_current_flag)
    {
        for (int i = 0; i < DMA_ADC_data_length * 2; i++)
        {
            if (i % 2 == 0)
                adc_Volt_data[i / 2] = raw_adc_DMA_data[i];
            else if (i % 2 == 1)
                AFC_adc_Volt_data[i / 2] = raw_adc_DMA_data[i];
        }
    }
    else if (!measure_volt_flag && measure_current_flag)
    {
        for (int i = 0; i < DMA_ADC_data_length * 2; i++)
        {
            if (i % 2 == 0)
                adc_Current_data[i / 2] = raw_adc_DMA_data[i];
            else if (i % 2 == 1)
                AFC_adc_Current_data[i / 2] = raw_adc_DMA_data[i];
        }
        adc_read_complete_flag_DMA = 1; // Only after the Current Completion
    }
    // adc_read_complete_flag_DMA = 1;
}

uint8_t ADC_Data_Ready()
{
    if (adc_read_complete_flag_DMA)
    {
        adc_read_complete_flag_DMA = 0;
        return 1;
    }
    else
        return 0;
}

void Start_ADC_Conversion()
{
    // Restart the ADC
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)raw_adc_DMA_data, DMA_ADC_data_length * 2);
}

void GPIO_Init_VI_GS_Pin()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */

    /*Configure GPIO pins : LED_pin_Pin GS_Pin VI_Pin */
    GPIO_InitStruct.Pin = GS_Pin | VI_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Set to Voltage Measurement Mode
    // HAL_GPIO_WritePin(VI_pin_GPIO_Port, VI_Pin, 0);
    // HAL_GPIO_WritePin(GS_pin_GPIO_Port, GS_Pin, 1);
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
void Timer2_Init_VI_switch(void)
{

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 7200;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 6250; // 12500; // 25000;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void Start_Timer_VI_switch()
{
    // Start the Timer 2 with Interrupt
    HAL_TIM_Base_Start_IT(&htim2);
}

void Stop_Timer_VI_switch()
{
    // Start the Timer 2 with Interrupt
    HAL_TIM_Base_Stop_IT(&htim2);
}

// VI Measure mode controlled by Timer 2 Interrupt
void set_measure_mode(int8_t _mode1)
{
    /*
    +----------------------+
    <1> - Discharge for Voltage Measure
    <2> - Set Voltage Mode
    <3> - Measure Voltage ****
    <4> - Discharge for Current Measure
    <5> - Set Current Mode
    <6> - Measure Current ****
    |---------------------------|
    | Discharge | Set | Measure |
    |---------------------------|
    */
    if (!auto_switch_VI_measure_mode)
        _mode1 -= 1; // To set control pins

    switch (_mode1)
    {
    case 1:
        // 1. Discharge for Voltage Measure
        VI_pin_state = LOW;
        GS_pin_state = LOW;
        break;
    case 2:
        // 2. Set Voltage Mode
        VI_pin_state = LOW;
        GS_pin_state = HIGH;
        break;
    case 3:
        // 3. Measure Voltage
        measure_volt_flag = 1; // Start Measurement
        measure_current_flag = 0;
        break;
    case 4:
        // 4. Discharge for Current Measure
        VI_pin_state = HIGH;
        GS_pin_state = LOW;
        break;
    case 5:
        // 5.Set Current Mode
        VI_pin_state = HIGH;
        GS_pin_state = HIGH;
        break;
    case 6:
        // 6. Measure Current
        measure_volt_flag = 0;
        measure_current_flag = 1; // Start Measurement
        break;
    default:
        // Add Fail safe step
        // Do Nothing -> Stop the ADC data capturing volt/current data
        measure_volt_flag = 0;
        measure_current_flag = 0;
        break;
    }

    if (_mode1 % 3 != 0)
    {
        // Stop Measurement
        measure_volt_flag = 0;
        measure_current_flag = 0;
        // Set the Mode
        HAL_GPIO_WritePin(VI_pin_GPIO_Port, VI_Pin, VI_pin_state);
        HAL_GPIO_WritePin(GS_pin_GPIO_Port, GS_Pin, GS_pin_state);
    }
    if (!auto_switch_VI_measure_mode)
    {
        if (_mode1 == 2)
        {
            // 3. Measure Voltage
            measure_volt_flag = 1; // Start Measurement
            measure_current_flag = 0;
            Start_ADC_Conversion();
        }
        else if (_mode1 == 5)
        {
            // 6. Measure Current
            measure_volt_flag = 0;
            measure_current_flag = 1; // Start Measurement
            Start_ADC_Conversion();
        }
    }
    else if (auto_switch_VI_measure_mode)
    {
        if (measure_volt_flag && !measure_current_flag)
        {
            GPIOA->BSRR = GPIO_PIN_5; // Set PA5 High
            Start_ADC_Conversion();   // Measure Voltage
        }
        if (!measure_volt_flag && measure_current_flag)
        {
            GPIOA->BRR = GPIO_PIN_5; // Set PA5 LOW
            Start_ADC_Conversion();  // Measure Current
        }
    }
}

void On_Timer2_Interrupt()
{
    if (auto_switch_VI_measure_mode)
    {
        _VI_measure_mode++;
        if (_VI_measure_mode == 7)
            _VI_measure_mode = 1;
    }
    set_measure_mode(_VI_measure_mode);
}