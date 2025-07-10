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
 * Credits: OneOfEleven
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

// Dual ADC Reference: https://github.com/STMicroelectronics/STM32CubeF1/blob/master/Projects/STM3210C_EVAL/Examples/ADC/ADC_DualModeInterleaved/Src/main.c
// Uncomment any one not both
// Clean the Code Before Build the code

// Private Variable Declaration
int32_t raw_adc_DMA_data[DMA_ADC_DATA_LENGTH];
volatile uint8_t adc_read_complete_flag_DMA = 0;

uint8_t measure_mode_flag = 0;
volatile uint8_t _VI_measure_mode = 0;

// Private Function Declaration
void Timer3_Init_ADC();
void DMA_Init_ADC();
void separate_ADC_CH_from_DMA();
void Start_ADC_Conversion();
void Stop_ADC_Conversion();
// void separate_adc_max_value();

void GPIO_Init_VI_GS_Pin();
void Timer2_Init_VI_switch(void);
void Start_Timer_VI_switch();
void Stop_Timer_VI_switch();
void set_measure_mode(uint8_t _mode1);

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

    set_ADC_Measure_window(1000); // 1 KHz
    Start_Timer_VI_switch();
}

// Manual Control for Zero padding
// void manual_read_ADC()
// {
//     // Note: Don't call this function directly,
//     // setup_ADC_with_DMA() need to call atleast once
//     // HAL_TIM_Base_Stop(&htim3); // Start the Timer3

//     HAL_GPIO_WritePin(VI_pin_GPIO_Port, VI_Pin, LOW);
//     HAL_GPIO_WritePin(GS_pin_GPIO_Port, GS_Pin, HIGH);
//     _manual_read_ADC_ = 1;
//     Start_ADC_Conversion();
// }

// void release_manual_read_ADC()
// {
//     // Release by Windows Reset the following flag
//     _manual_read_ADC_ = 0;              // Release to normal mode
//     adc_read_complete_flag_DMA = 0;     // Re-Capture the Reading
// }

void Timer3_Init_ADC()
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    const uint32_t timer3_rate_Hz = (DMA_ADC_DATA_LENGTH / no_of_sine_wave_cycle_per_data) * 1000; // Default - 1kHz

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 4;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = (((HAL_RCC_GetHCLKFreq() / (htim3.Init.Prescaler + 1)) + (timer3_rate_Hz / 2)) / timer3_rate_Hz) - 1;
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

    // ************************************************
    // setup the master ADC
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE; // Only One Channel is used
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1; // For only one Channel
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    //  Configure the ADC multi-mode
    ADC_MultiModeTypeDef multimode = {0};
    multimode.Mode = ADC_DUALMODE_REGSIMULT;
    if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
    {
        Error_Handler();
    }

    // ADC1 and ADC2 Settings
    sConfig.Rank = ADC_REGULAR_RANK_1;
    //	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLES_5;     //   1.5 + 12.5 =  14 cycles, ADC clk = 12MHz, 1.2us sample time, max 857kHz sample rate
    //	sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;     //   7.5 + 12.5 =  20 cycles, ADC clk = 12MHz, 1.7us sample time, max 600kHz sample rate
    //	sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;    //  13.5 + 12.5 =  26 cycles, ADC clk = 12MHz, 2.2us sample time, max 461kHz sample rate
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5; //  28.5 + 12.5 =  41 cycles, ADC clk = 12MHz, 3.4us sample time, max 292kHz sample rate
    //	sConfig.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;    //  41.5 + 12.5 =  54 cycles, ADC clk = 12MHz, 4.5us sample time, max 222kHz sample rate
    //	sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;    //  55.5 + 12.5 =  68 cycles, ADC clk = 12MHz, 5.7us sample time, max 176kHz sample rate
    // sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;    //  71.5 + 12.5 =  84 cycles, ADC clk = 12MHz, 7.0us sample time, max 142kHz sample rate
    //	sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;   // 239.5 + 12.5 = 252 cycles, ADC clk = 12MHz, 21us sample time, max 47.6kHz sample rate

    // Configure Regular Channel
    sConfig.Channel = ADC_CHANNEL_0; // PA0 Pin

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // ************************************************
    // setup the slave ADC

    hadc2.Instance = ADC2;
    /* Same configuration as ADC master, with continuous mode and external      */
    /* trigger disabled since ADC master is triggering the ADC slave            */
    /* conversions                                                              */
    hadc2.Init = hadc1.Init;
    hadc2.Init.ContinuousConvMode = ENABLE; // **Very Important (Not Mentioned in any Document)
    hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;

    if (HAL_ADC_Init(&hadc2) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_1; // PA1 Pin
    // sConfig.Rank = ADC_REGULAR_RANK_1;
    // sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;

    if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    //---------------------------ADC2 Config Completed----------------------------------//
    // Rest of the Config remains mentioned as
    // void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)

    // HAL_ADC_Start_IT(&hadc1); // Start the ADC conversion with Interrupt
    // Disable the ADC Interrupt at stm32f1xx_hal_msp.c file

    // Start the ADC with DMA
    // HAL_ADC_Start_DMA(&hadc1, (uint32_t *)raw_adc_DMA_data, DMA_ADC_DATA_LENGTH);

    // Rest of the Config remains mentioned as
    // void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) placed at stm32f1xx_hal_msp.c file

    /* Run the ADC calibration */
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADCEx_Calibration_Start(&hadc2) != HAL_OK)
    {
        Error_Handler();
    }
}

void DMA_Init_ADC()
{
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    // ADC - DMA1_Channel1_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0); // 1. PreemptPriority = 0, SubPriority = 0
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);         // 1. ADC Data
}

void set_ADC_Measure_window(uint16_t _measure_frequency)
{
    const uint32_t timer3_rate_Hz = (DMA_ADC_DATA_LENGTH / no_of_sine_wave_cycle_per_data) * _measure_frequency;
    uint32_t timer3_period = (((HAL_RCC_GetHCLKFreq() / (htim3.Init.Prescaler + 1)) + (timer3_rate_Hz / 2)) / timer3_rate_Hz) - 1;

    if (_measure_frequency > 0) // check point to ensure only +ve value only
    {
        htim3.Init.Period = timer3_period;
        // Rest of Config remains same as Timer3_Init_ADC() function
        if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
        {
            Error_Handler();
        }
    }

    // After the Windows Reset the following flag
    adc_read_complete_flag_DMA = 0; // Re-Capture the Reading
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
        separate_ADC_CH_from_DMA();
}

void separate_ADC_CH_from_DMA()
{
    int16_t PA0_data_temp = 0, PA1_data_temp = 0;

    for (int i = 0; i < DMA_ADC_DATA_LENGTH; i++)
    {
        PA0_data_temp = (int16_t)(raw_adc_DMA_data[i] & 0xFFFF); // Extract PA0 data
        PA1_data_temp = (int16_t)(raw_adc_DMA_data[i] >> 16);    // Extract PA1 data

        // Remove offset value
        PA0_data_temp -= zero_pad_adc_PA[0];
        PA1_data_temp -= zero_pad_adc_PA[1];

        adc_raw_data[((measure_mode_flag * 2) - 2)][i] = PA0_data_temp;
        adc_raw_data[((measure_mode_flag * 2) - 1)][i] = PA1_data_temp;

        adc_data[((measure_mode_flag * 2) - 2)][i] = adc_volt_convert(PA0_data_temp);
        adc_data[((measure_mode_flag * 2) - 1)][i] = adc_volt_convert(PA1_data_temp);
    }

    // Transfer the Status After Completing
    adc_read_complete_flag_DMA = measure_mode_flag;
}

uint8_t get_measure_status()
{
    return adc_read_complete_flag_DMA;
}

void Start_ADC_Conversion()
{
    // Restart the ADC
    HAL_ADC_Start(&hadc2); // Start ADC2 First
    HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t *)raw_adc_DMA_data, DMA_ADC_DATA_LENGTH);
}

void Stop_ADC_Conversion()
{
    // Stop the ADC
    HAL_ADC_Stop(&hadc2); // Start ADC2 First
    HAL_ADCEx_MultiModeStop_DMA(&hadc1);
}

uint8_t ADC_recapture_data() // Pointer used to edit in struct value
{
    if (get_measure_status() == 4)
    {
        adc_read_complete_flag_DMA = 0; // Reset Flag After Copying
        _VI_measure_mode = 0; // Reset VI Switch Position
        return 1;                       // Recapture started
    }
    else
        return 0; // Invaild request to Recapture
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
    htim2.Init.Period = 1250; // Every 125mSecs
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
void set_measure_mode(uint8_t _a_mode1)
{
    uint8_t _mode1 = _a_mode1 / 2;
    if (_a_mode1 % 2 == 0)
    {
        if (_mode1 >= 0 && _mode1 <= 3)
        {
            HAL_GPIO_WritePin(VI_pin_GPIO_Port, VI_Pin, (_mode1 & 2) ? HIGH : LOW); // 2nd bit
            HAL_GPIO_WritePin(GS_pin_GPIO_Port, GS_Pin, (_mode1 & 1) ? HIGH : LOW); // 1st bit
        }
    }
    else
    {
        measure_mode_flag = _mode1 + 1;
        Start_ADC_Conversion(); // Trigger the Measurement
    }
}

void On_Timer2_Interrupt()
{
    if (_VI_measure_mode <= 7)
    {
        set_measure_mode(_VI_measure_mode);
        _VI_measure_mode++;
    }
}