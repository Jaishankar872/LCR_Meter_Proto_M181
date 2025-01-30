
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 * Developed by: Jaishankar M
 ******************************************************************************
 */

#include "main.h"
#include "string.h"
#include "stdio.h"
#include "m181_display_softwire.h"
#include "DAC_sine_wave_gen.h"
#include "ADC_Config_DMA.h"
#include "Buttons_and_LED.h"

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

void setup();
void SystemClock_Config(void);
// static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

// static void MX_TIM2_Init(void);
// static void MX_TIM3_Init(void);

// Private Variable Declaration
#define DMA_ADC_data_length 50
int16_t adc_Current_data[DMA_ADC_data_length];
int16_t adc_Volt_data[DMA_ADC_data_length];
int16_t AFC_adc_Current_data[DMA_ADC_data_length];
int16_t AFC_adc_Volt_data[DMA_ADC_data_length];

// [Temp]Redirect printf to UART
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 1000);
  return len;
}

int main(void)
{

  setup();
  while (1)
  {
    if (ADC_Data_Ready() == 1)
    {

      const int _print_delay = 20; // Milli Seconds
      // printf("Via DMA interrupt Callback function\n");
      for (int i = 0; i < DMA_ADC_data_length; i++)
      {
        printf("%d,%d,%d,%d,%d\n", i + 1, adc_Volt_data[i], AFC_adc_Volt_data[i], adc_Current_data[i], AFC_adc_Current_data[i]);
        HAL_Delay(_print_delay);
      }
      // GPIOA->ODR ^= GPIO_PIN_5; // Toggle LED
    }
  }
}

void setup()
{
  // Syatem & HAL Initialization
  HAL_Init();
  SystemClock_Config();

  // Sine Wave Generator Initialization
  sine_wave_setup();

  // ADC Initialization
  setup_ADC_with_DMA();

  // GPIO Initialization
  setup_buttons_and_LED();
  // MX_GPIO_Init();

  // UART Initialization
  MX_USART1_UART_Init();

  // Display Initialization
  ssd1306_display_sofwire_Init();

  // Set DAC Frequency
  u_int16_t _freq = 800;
  set_sine_wave_frequency(_freq);
  print_home_screen(_freq);
  set_ADC_Measure_window(_freq);
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * Enable DMA controller clock
 */
// static void MX_DMA_Init(void)
// {

//   /* DMA controller clock enable */
//   __HAL_RCC_DMA1_CLK_ENABLE();

//   /* DMA interrupt init */
//   /* DMA1_Channel1_IRQn interrupt configuration */
//   HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
//   HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
//   /* DMA1_Channel4_IRQn interrupt configuration */
//   HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
//   HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
// }

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
