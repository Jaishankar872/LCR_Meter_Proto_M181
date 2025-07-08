
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 * Developed by: Jaishankar M
 * Credits: OneOfEleven
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
#include "crc.h" // For UART

// TIM_HandleTypeDef htim2;
// TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

void system_setup();
void system_loop();
void SystemClock_Config(void);

static void DMA_Init_UART();
static void setup_UART(void);
void zero_padding_value();

// Private Variable Declaration
// Refer the file name "system_data.h"

system_data process_data; // From file name "system_data.h"

// [Temporary] For UART

#pragma pack(push, 1)
typedef struct {
	union {
		uint32_t marker;
		uint8_t  marker_b[sizeof(uint32_t)];
	};
	union {
		uint16_t crc;
		uint8_t  crc_b[sizeof(uint16_t)];
	};
	union {
		float   data[DMA_ADC_DATA_LENGTH * 8];
		uint8_t data_b[sizeof(float) * DMA_ADC_DATA_LENGTH * 8];
	};
} t_packet;
#pragma pack(pop)
t_packet              tx_packet;

int _write(int file, char *ptr, int len)
{
  // HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 1000);
  
  // start sending using DMA mode
  const uint32_t tick = HAL_GetTick();
  // wait for a maximum of 100ms for the send to start
		while (HAL_BUSY == HAL_UART_Transmit_DMA(&huart1, (uint8_t *)ptr, len) && (HAL_GetTick() - tick) < 100)
			__WFI();    // wait until next interrupt occurs

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
  DMA_Init_UART();
  setup_UART();

  // Display Initialization
  ssd1306_display_sofwire_Init();

  // Bootup Screen Time - 4 Seconds
  bootup_screen(1); // Wait for 1 Seconds
  // Zero-Padding
  zero_padding_value(); // Wait for +1 Seconds for Bootup Screen
  HAL_Delay(2000);      // Wait for +2 Seconds for Bootup Screen
  clear_full_display();

  // Default: Set-> Update -> Display ** Must required
  // Set
  process_data.set_freq = 1000;        // Default frequency
  process_data.uart_all_print_DSO = 1; // Default Mode
  process_data.LCR_Mode = 2;           // Default Mode - Capacitance
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
    // process_data_via_DSP(&process_data);

    // Update the display
    screen1_home_print(process_data);
    if (process_data.uart_all_print_DSO)
    {
      #ifdef MATLAB_serial_enable
      const int _print_delay = 5; // Milli Seconds
      // printf("Via DMA interrupt Callback function\n");
      for (int i = 0; i < DMA_ADC_data_length; i++)
      {
        printf("%d,", i + 1);
        for (int col = 0; col < 8; col++)
        {
          if (col != 7)
            printf("%.1f,", adc_raw_data[col][i]);
          else
            printf("%.1f\r\n", adc_raw_data[col][i]);
        }
        HAL_Delay(_print_delay);
      }
      #else
      // send as binary packet

      tx_packet.marker = PACKET_MARKER;
      memcpy(tx_packet.data, &adc_data, sizeof(adc_data));
      tx_packet.crc = CRC16_block(0, tx_packet.data, sizeof(tx_packet.data));

        #if 0
        // start sending the packet (wait here for upto 200ms until it does start)
        const uint32_t tick = HAL_GetTick();
        while (HAL_BUSY == HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&tx_packet, sizeof(tx_packet)) && (HAL_GetTick() - tick) < 200)
          __WFI();    // wait until next interrupt occurs
       #else
        // don't hang around waiting for packet to start
        HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&tx_packet, sizeof(tx_packet));
        #endif
      #endif
    }
    // Restart the Data capture
    process_data_via_DSP(&process_data);
    ADC_recapture_data();
    HAL_Delay(800); // Pause for a moment
  }
  else
  {
    // Update the display
    screen1_home_print(process_data);
  }
}

void zero_padding_value()
{
  if (0)
  {
    int16_t _adc_avg_data[2] = {0, 0};
    int16_t _length = 10, _each_delay = 60;

    manual_ctrl_DAC(127);
    HAL_Delay(_each_delay);
    manual_read_ADC(); // Just call, skip inital buffer [Revisit Again....]
    HAL_Delay(_each_delay);

    for (int n = 0; n < _length; n++)
    {
      manual_read_ADC();
      HAL_Delay(_each_delay);

      _adc_avg_data[0] += zero_pad_adc_PA[0];
      _adc_avg_data[1] += zero_pad_adc_PA[1];
    }
    zero_pad_adc_PA[0] = _adc_avg_data[0] / _length;
    zero_pad_adc_PA[1] = _adc_avg_data[1] / _length;

    printf("Average \r\n %d - %.3f, %d - %.3f\r\n", zero_pad_adc_PA[0], adc_volt_convert(zero_pad_adc_PA[0]),
           zero_pad_adc_PA[1], adc_volt_convert(zero_pad_adc_PA[1]));
    release_manual_read_ADC();
    release_manual_ctrl_DAC();
  }
  else
  {
    zero_pad_adc_PA[0] = 2275; // V/I offset value
    zero_pad_adc_PA[1] = 1861; // AFC Pin offset value
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
  huart1.Init.BaudRate = UART_BAUDRATE;
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

static void DMA_Init_UART()
{

    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA1_Channel4_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0); // PreemptPriority = 5, SubPriority = 0
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn); // 6. UART DMA
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
