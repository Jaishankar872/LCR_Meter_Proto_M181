
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
#include "system_data.h"
#include "m181_display_softwire.h"
#include "DAC_sine_wave_gen.h"
#include "ADC_Config_DMA.h"
#include "Buttons_and_LED.h"
#include "DSP_data.h"

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

void system_setup();
void system_loop();
void SystemClock_Config(void);
static void setup_UART(void);

// Private Variable Declaration
// Refer the file name "system_data.h"

system_data process_data; // From file name "system_data.h"

// [Temp]Redirect printf to UART
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 1000);
  return len;
}

int main(void)
{

  system_setup();
  while (1)
  {
    system_loop();
  }
}

void system_setup()
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

  // UART Initialization
  setup_UART();

  // Display Initialization
  ssd1306_display_sofwire_Init();

  // Default: Set-> Update -> Display ** Must required
  // Set
  process_data.set_freq = 1000;        // Default frequency
  process_data.uart_all_print_DSO = 1; // Default Mode
  process_data.LCR_Mode = 3;           // Default Mode - Capacitance
  // Update
  set_sine_wave_frequency(process_data.set_freq);
  set_ADC_Measure_window(process_data.set_freq);
  // Display
  screen1_home_print(process_data);
  setup_DSP_parameter();
}

void system_loop()
{
  // Read the control parameter & if Changes happen then
  if (on_button_event(&process_data))
  {

    // Set the based on the data
    set_sine_wave_frequency(process_data.set_freq);
    set_ADC_Measure_window(process_data.set_freq);

    // Update the display
    screen1_home_print(process_data);
  }

  process_data.adc_measure_status = get_measure_status();
  if (process_data.adc_measure_status == 4)
  {
    // First Process the data
    // ->Display value
    process_data_via_DSP(&process_data);

    // Update the display
    screen1_home_print(process_data);
    if (process_data.uart_all_print_DSO)
    {
      const int _print_delay = 20; // Milli Seconds
      // printf("Via DMA interrupt Callback function\n");
      for (int i = 0; i < DMA_ADC_data_length; i++)
      {
        printf("%d,%d,%d,%d,%d,%d,%d,%d,%d\n", i + 1, adc_Volt_data[i], AFC_adc_Volt_data[i], adc_Current_data[i], AFC_adc_Current_data[i],
               adc_Volt_data_ZC[i], AFC_adc_Volt_data_ZC[i], adc_Current_data_ZC[i], AFC_adc_Current_data_ZC[i]);
        HAL_Delay(_print_delay);
      }
    }
    // Restart the Data capture
    ADC_recapture_data();
    HAL_Delay(800); // Pause for a moment
  }
  else
  {
    // Update the display
    screen1_home_print(process_data);
  }
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

static void setup_UART(void)
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
