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

#define DUAL_ADC_SIM_OFF // Dual Mode for ADC1 and ADC2 is Disabled
// Clean the Code Before Build the code

// Private Variable Declaration
#ifdef DUAL_ADC_SIM_OFF
uint16_t raw_adc_DMA_data[DMA_ADC_data_length];
uint16_t raw_adc2_data[DMA_ADC_data_length];
uint16_t adc2_buffer_counter = 0;
#else
uint16_t raw_adc_DMA_data[DMA_ADC_data_length * 2];
#endif
volatile uint8_t adc_read_complete_flag_DMA = 0;

uint8_t measure_volt_flag = 0, measure_current_flag = 0;
uint8_t GS_pin_state = 1, VI_pin_state = 0; // VI_measure_mode = 0;
volatile uint8_t _VI_measure_mode = 1, auto_switch_VI_measure_mode = 1;

int16_t max_adc_Volt = 0, min_adc_Volt = 4096;
int16_t max_adc_Volt_AFC = 0, min_adc_Volt_AFC = 4096;
int16_t max_adc_Current = 0, min_adc_Current = 4096;
int16_t max_adc_Current_AFC = 0, min_adc_Current_AFC = 4096;

// Private Function Declaration
void Timer3_Init_ADC();
void DMA_Init_ADC();
void separate_ADC_CH_from_DMA();
void Start_ADC_Conversion();
void Stop_ADC_Conversion();

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

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_0; // PA0 Pin
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    
    //---------------------------ADC1 Config Completed----------------------------------//

    hadc2.Instance = ADC2;
    /* Same configuration as ADC master, with continuous mode and external      */
    /* trigger disabled since ADC master is triggering the ADC slave            */
    /* conversions                                                              */
    hadc2.Init = hadc1.Init;
    if (HAL_ADC_Init(&hadc2) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_1; // PA1 Pin
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;

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
    // HAL_ADC_Start_DMA(&hadc1, (uint32_t *)raw_adc_DMA_data, DMA_ADC_data_length);

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

    /* DMA interrupt init */
    /* DMA1_Channel1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0); // 1. PreemptPriority = 0, SubPriority = 0
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);         // 1. ADC Data

    /* DMA1_Channel4_IRQn interrupt configuration */
    // HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0); // PreemptPriority = 5, SubPriority = 0
    // HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn); // 6. UART DMA
}

void set_ADC_Measure_window(uint16_t _measure_frequency)
{
    float window_time_us = 0;
    uint32_t timer3_period = 0; // 32 bit is needed -to handle number Big number on Calculation
    uint8_t timer3_prescaler = 8;
    uint8_t adc_sample_rate = 32;

    float APB1_Timer_clock_set_Time_nS = 15.625; // 10^3/64MHz = 15.625nS;(Not 72MHz)
    float After_timer3_prescaler_time_nS = APB1_Timer_clock_set_Time_nS * timer3_prescaler;

    if (_measure_frequency > 0)
    {
        window_time_us = 1000000 / _measure_frequency; // Microseconds
        window_time_us /= adc_sample_rate;

        timer3_period = window_time_us * 1000;           // Convert into uS into nS
        timer3_period /= After_timer3_prescaler_time_nS; // Timer frequency
        timer3_period -= 1; // Counter starts with 0

        htim3.Init.Prescaler = timer3_prescaler;
        htim3.Init.Period = timer3_period;
        // Rest of Config remains same as MX_TIM3_Init() function
        if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
        {
            Error_Handler();
        }
    }

    // After the Windows Reset the following flag
    adc_read_complete_flag_DMA = 0;     // Re-Capture the Reading
    _VI_measure_mode = 1;               // Reset VI Switch Position
    set_measure_mode(_VI_measure_mode); // GPIO State
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
        Stop_ADC_Conversion();
        separate_ADC_CH_from_DMA();
        adc2_buffer_counter = 0;
    }
#ifdef DUAL_ADC_SIM_OFF
    if (hadc->Instance == ADC2)
    {
        // Only for Test Purpose - ADC Sample Rate via DSO
        // GPIOA->ODR ^= GPIO_PIN_5; // Toggle PA5 - LED Pin
        //--------------
        raw_adc2_data[adc2_buffer_counter] = HAL_ADC_GetValue(hadc);
        adc2_buffer_counter++;
        if (adc2_buffer_counter >= DMA_ADC_data_length)
            adc2_buffer_counter = 0;
    }
#endif

    // Now will take over by DMA
}

void separate_ADC_CH_from_DMA()
{
    if (measure_volt_flag && !measure_current_flag)
    {
        // Reset the value before get into the loop
        max_adc_Volt = 0;
        min_adc_Volt = 4096;
        max_adc_Volt_AFC = 0;
        min_adc_Volt_AFC = 4096;
        for (int i = 0; i < DMA_ADC_data_length; i++)
        {
            adc_Volt_data[i] = raw_adc_DMA_data[i];  // PA0
            AFC_adc_Volt_data[i] = raw_adc2_data[i]; // PA1

            // Maximum & Minimum calculation
            if (adc_Volt_data[i] > max_adc_Volt)
                max_adc_Volt = adc_Volt_data[i];
            if (adc_Volt_data[i] < min_adc_Volt)
                min_adc_Volt = adc_Volt_data[i];
            // Maximum & Minimum calculation - AFC
            if (AFC_adc_Volt_data[i] > max_adc_Volt_AFC)
                max_adc_Volt_AFC = AFC_adc_Volt_data[i];
            if (AFC_adc_Volt_data[i] < min_adc_Volt_AFC)
                min_adc_Volt_AFC = AFC_adc_Volt_data[i];
        }
        adc_read_complete_flag_DMA = 3; // Voltage data Ready & Current data start Capturing
    }
    else if (!measure_volt_flag && measure_current_flag)
    {
        // Reset the value before get into the loop
        max_adc_Current = 0;
        min_adc_Current = 4096;
        max_adc_Current_AFC = 0;
        min_adc_Current_AFC = 4096;
        for (int i = 0; i < DMA_ADC_data_length; i++)
        {
            adc_Current_data[i] = raw_adc_DMA_data[i];  // PA0
            AFC_adc_Current_data[i] = raw_adc2_data[i]; // PA1

            // Maximum & Minimum calculation
            if (adc_Current_data[i] > max_adc_Current)
                max_adc_Current = adc_Current_data[i];
            if (adc_Current_data[i] < min_adc_Current)
                min_adc_Current = adc_Current_data[i];
            // Maximum & Minimum calculation -AFC
            if (AFC_adc_Current_data[i] > max_adc_Current_AFC)
                max_adc_Current_AFC = AFC_adc_Current_data[i];
            if (AFC_adc_Current_data[i] < min_adc_Current_AFC)
                min_adc_Current_AFC = AFC_adc_Current_data[i];
        }
        adc_read_complete_flag_DMA = 4; // Both are voltage and current data Ready
    }
}

uint8_t ADC_Data_Ready()
{
    if (adc_read_complete_flag_DMA == 3)
    {
        adc_read_complete_flag_DMA = 0;
        return 3;
    }
    else
        return adc_read_complete_flag_DMA;
}

void Start_ADC_Conversion()
{
    // Restart the ADC
#ifdef DUAL_ADC_SIM_OFF
    // Enable ADC slave
    HAL_ADC_Start_IT(&hadc2);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)raw_adc_DMA_data, DMA_ADC_data_length);
#else
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)raw_adc_DMA_data, DMA_ADC_data_length * 2);

#endif
}

void Stop_ADC_Conversion()
{
#ifdef DUAL_ADC_SIM_OFF
    // Enable ADC slave
    HAL_ADC_Stop_IT(&hadc2);
    HAL_ADC_Stop_DMA(&hadc1);
#else
    // Stop the ADC
    HAL_ADC_Stop_DMA(&hadc1);
#endif
}

float adc_volt_convert(int16_t raw_adc)
{
    int16_t adc_res = 4096;
    float adc_ref = 3.3;
    float volt_reading1 = 0;
    volt_reading1 = (float)adc_ref * raw_adc;
    volt_reading1 /= adc_res;
    return volt_reading1;
}

void get_adc_reading(system_data *_adc_data) // Pointer used to edit in struct value
{
    int16_t _pk_pk;
    _pk_pk = max_adc_Volt - min_adc_Volt;
    _adc_data->pk_pk_voltage = adc_volt_convert(_pk_pk);
    _pk_pk = max_adc_Volt_AFC - min_adc_Volt_AFC;
    _adc_data->pk_pk_AFC_volt = adc_volt_convert(_pk_pk);
    _pk_pk = max_adc_Current - min_adc_Current;
    _adc_data->pk_pk_current = adc_volt_convert(_pk_pk);
    _pk_pk = max_adc_Current_AFC - min_adc_Current_AFC;
    _adc_data->pk_pk_AFC_current = adc_volt_convert(_pk_pk);
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
            adc_read_complete_flag_DMA = 1; // Voltage data start Capturing
            measure_volt_flag = 1;          // Start Measurement
            measure_current_flag = 0;
            Start_ADC_Conversion();
        }
        else if (_mode1 == 5)
        {
            // 6. Measure Current
            adc_read_complete_flag_DMA = 2; // Current data start Capturing
            measure_volt_flag = 0;
            measure_current_flag = 1; // Start Measurement
            Start_ADC_Conversion();
        }
    }
    else if (auto_switch_VI_measure_mode)
    {
        if (measure_volt_flag && !measure_current_flag)
        {
            Start_ADC_Conversion(); // Measure Voltage
        }
        if (!measure_volt_flag && measure_current_flag)
        {
            Start_ADC_Conversion(); // Measure Current
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