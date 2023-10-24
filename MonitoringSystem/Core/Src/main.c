#include "main.h"
#include "fatfs.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "projdefs.h"
#include "lcd.h"
#include "keypad.h"
#include "soil.h"
#include "string.h"
#include "stm32f4xx_hal_adc.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);

// SDCARD
FATFS fs;          // file system
FIL fil;           // file
FRESULT fresult;   // to store the result
char buffer[1024]; // to store the data

UINT br, bw; // file read / write count

// Capacity related variables
FATFS *pfs;
DWORD fre_clust;
uint32_t total, free_space;

// SDCARD

ADC_HandleTypeDef hadc;
SPI_HandleTypeDef hspi1;

// Threads
TaskHandle_t lcd_task;
TaskHandle_t logger_task;
TaskHandle_t keypad_task;
TaskHandle_t soil_task;
TaskHandle_t alarm_task;

// Queue
QueueHandle_t lcd_queue = NULL;

// Structure
lcd_event_t lcd_message;
Keyestudio_SoilSensor_t soil;

// The soil moisture limit used to trigger an alarm
uint8_t soil_limit_value = 0;

// Menu Control
uint8_t SoilControl = 1;
uint8_t AlarmControl = 0;
volatile uint8_t LoggingControl = 0;

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
	if(pin == GPIO_PIN_0)
	{
		lcd_event_t button_msg;
		button_msg.row = 2;
		button_msg.col = 0;

		// 1. First button press should enable data logging
		if(LoggingControl == 0)
		{
			// Enable data logging
			LoggingControl = 1;
			sprintf(button_msg.message, "Start Logging");
			xQueueSendFromISR(lcd_queue, &button_msg, (BaseType_t *)3);
		}
		// 2. Second button press should disable data logging
		else if(LoggingControl == 1)
		{
			// Disable data logging
			LoggingControl = 0;
			sprintf(button_msg.message, "Stop Logging");
			xQueueSendFromISR(lcd_queue, &button_msg, (BaseType_t *)3);
		}
	}

	else
	{
		__NOP();
	}
}

void lcd_thread(void *arg)
{
	// Create the queue to hold all lcd messages
	lcd_queue = xQueueCreate(10, sizeof(lcd_event_t));

	// Lcd message
	lcd_event_t msg;

	// Initialization and welcoming message
	Lcd_init_4bit();
	Lcd_clear_4bit();
	Lcd_string_xy(1, 3, "Monitoring");
	Lcd_string_xy(2, 5, "System");
	vTaskDelay(pdMS_TO_TICKS(1000));

	for(;;)
	{
		if(xQueueReceive(lcd_queue, &msg, 0) == pdPASS)
		{
			Lcd_clear_4bit();
			vTaskDelay(pdMS_TO_TICKS(100));
			Lcd_string_xy(msg.row, msg.col, msg.message);
			vTaskDelay(pdMS_TO_TICKS(100));
		}
	}
}

void soil_thread(void *arg)
{
	// Clock enable
	__HAL_RCC_ADC1_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	// soil_msg
	lcd_event_t soil_msg;

	//GPIO Initialization - PA2
	GPIO_InitTypeDef adc_pin;
	adc_pin.Pin = GPIO_PIN_2;
	adc_pin.Mode = GPIO_MODE_ANALOG;
	adc_pin.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	adc_pin.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &adc_pin);

	// ADC initialization
	ADC_ChannelConfTypeDef sConfig = {0};
	HAL_ADC_MspInit(&hadc);

	hadc.Instance = ADC1;
	hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.ScanConvMode = DISABLE;
	hadc.Init.ContinuousConvMode = ENABLE;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.NbrOfConversion = 1;
	hadc.Init.DMAContinuousRequests = DISABLE;
	hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
	if(HAL_ADC_Init(&hadc) != HAL_OK)
	{
		Error_Handler();
	}

	sConfig.Channel = ADC_CHANNEL_2;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}

	// Initialize the soil sensor
	SoilMoistureSensor_Init(&soil);

	// Start the ADC
	HAL_ADC_Start(&hadc);

	for(;;)
	{
		if(SoilControl == 1)
		{
			char buffer[16] = {0};
			soil.Dryness = 0.0f;
			// Calculate the soil moisture
			soil.RawValue = HAL_ADC_GetValue(&hadc);;
			SoilMoistureSensor_GetValue(&soil);
			// Display the results
			soil_msg.row = 1;
			soil_msg.col = 0;
			// Check limit breach
			if(soil.Dryness < soil_limit_value)
			{
				sprintf(soil_msg.message, "Water Low < %d", soil_limit_value);
				Lcd_string_xy(1, 0, soil_msg.message);
				sprintf(buffer, "Water level: %d", soil.Dryness);
				Lcd_string_xy(2, 0, buffer);
				// Activate the alarm
				AlarmControl = 1;
			}
			else
			{
				sprintf(soil_msg.message, "R:%d S:%d", soil.RawValue, soil.Dryness);
				xQueueSend(lcd_queue, &soil_msg, 1000);
				AlarmControl = 0;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void logger_thread(void *arg)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	// Configure GPIO pin Output Level
	HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

	// Configure GPIO pin : SPI1_CS_Pin
	GPIO_InitStruct.Pin = SPI1_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

	// Not mounted the first time
	static uint8_t mount_status = 0;

	for(;;)
	{
		// Enabled or Disabled by pressing the button
		if(LoggingControl == 1)
		{
			char soil_data_buffer[255] = {0};

			// Mount only once
			if(mount_status == 0)
			{
				fresult = f_mount(&fs, "", 0);
				if(fresult != FR_OK)
				{
					  // Mount failed
				}
				else
				{
					// Mount successful
				}
				mount_status = 1;
				// Acquire total + free space
				f_getfree("", &fre_clust, &pfs);
				total = (uint32_t)( (pfs->n_fatent - 2) * pfs->csize * 0.5 );
				free_space = (uint32_t)(fre_clust * pfs->csize * 0.5);
				// WARNING !!!!!! We need to stop writing if no free space is available
			}

			// 3. Opening the file or creating it if it doesn't exist
			fresult = f_open(&fil, "soil.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);

			// 4. Write the data to storage
			sprintf(soil_data_buffer, "Soil moisture %d\n", soil.Dryness);
			fresult = f_puts(soil_data_buffer, &fil);

            // Closing the file
			fresult = f_close(&fil);
		}
		else
		{
			// Unmount
			if(mount_status == 1)
			{
				fresult = f_mount(NULL, "", 1);
				if(fresult == FR_OK)
				{
					// Successful unmount
				}
				mount_status = 0;
			}
			HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void keypad_thread(void *arg)
{
	char key = 0;
	lcd_event_t keypad_msg;
	keypad_init();
	for(;;)
	{
		key = keypad_scan();
		if(key != 0)
		{
			// ************* Menu control *************** //

			// Use case 1: Set the limit for soil moisture
			if(key == 'A')
			{
				// Deactivate all functionality
				SoilControl = 0;
				AlarmControl = 0;

				char temp = 0;
				char f_digit = 0;
				char s_digit = 0;

				// Message to LCD: Enter the soil limit
				sprintf(keypad_msg.message, "Enter soil limit");
				keypad_msg.row = 1;
				keypad_msg.col = 0;
				xQueueSend(lcd_queue, &keypad_msg, 1000);

				// 1st press
				temp = keypad_wait_for_key(250);
				// Cancel
				if(temp == '*')
				{
					// Re-enable control
					SoilControl = 1;
				}
				// If its a digit
				else if(keypad_is_digit(temp) == true)
				{
					// Acquire the 1st key
					f_digit = temp;
					// Message to LCD: Display the 1st digit
					sprintf(keypad_msg.message, "%c", f_digit);
					Lcd_string_xy(2, 0, keypad_msg.message);
					// Clear temp
					temp = 0;
				}

				// 2nd press skip
				if(SoilControl != 1)
				{
					temp = keypad_wait_for_key(250);
				}

				// Cancel
				if(temp == '*')
				{
					// Re-enable control
					SoilControl = 1;
				}
				// if a digit
				else if(keypad_is_digit(temp) == true)
				{
					// Acquire the 2nd key
					s_digit = temp;
					// Message to LCD: Display the 1st digit
					sprintf(keypad_msg.message, "%c", s_digit);
					Lcd_string_xy(2, 1, keypad_msg.message);
					// Clear temp
					temp = 0;
				}

				// 3rd press skip
				if(SoilControl != 1)
				{
					temp = keypad_wait_for_key(250);
				}

				if(temp == '*' || temp == '#')
				{
					// Cancel
					if(temp == '*')
					{
						// Re-enable control
						SoilControl = 1;
					}
					else if(temp == '#')
					{
						// Convert to integer, store, and display the limit to the user
						soil_limit_value = (f_digit-'0')*10;
						soil_limit_value += (s_digit-'0');

						// Message to LCD: "Limit set: xx"
						Lcd_clear_4bit();
						sprintf(keypad_msg.message, "Limit set: %d", soil_limit_value);
						Lcd_string_xy(1, 0, keypad_msg.message);
						// Re-enable control level
						SoilControl = 1;
					}
				}
				else
				{
					// rescan for a new key
					temp = keypad_wait_for_key(250);
					while(keypad_confirm_or_cancel(temp) == KEY_TYPE_NONE)
					{
						temp = keypad_wait_for_key(250);
					}
					// Cancel
					if(temp == '*')
					{
						// Re-enable control
						SoilControl = 1;
					}
					// Confirm
					else if(temp == '#')
					{
						// Convert to integer, store, and display the limit to the user
						soil_limit_value = (f_digit-'0')*10;
						soil_limit_value += (s_digit-'0');

						// Message to LCD: "Limit set: xx"
						Lcd_clear_4bit();
						sprintf(keypad_msg.message, "Limit set: %d", soil_limit_value);
						Lcd_string_xy(1, 0, keypad_msg.message);
						// Re-enable control level
						SoilControl = 1;
					}
				}

				// Clear the key
				key = 0;
			}
			// Use case 2: Start or Stop recording data to the memory card
			// Note: Not completed, because the button press on the board is used
			//       for this until extra wires are obtained
			else if(key == 'B')
			{
				lcd_event_t store_msg;
				store_msg.row = 2;
				store_msg.col = 0;

				// 1. First button press should enable data logging
				if(LoggingControl == 0)
				{
					// Enable data logging
					LoggingControl = 1;
					sprintf(store_msg.message, "Start Logging");
					xQueueSend(lcd_queue, &store_msg, 1000);
				}
				// 2. Second button press should disable data logging
				else if(LoggingControl == 1)
				{
					// Disable data logging
					LoggingControl = 0;
					sprintf(store_msg.message, "Stop Logging");
					xQueueSend(lcd_queue, &store_msg, 1000);
				}
				// Reset the key value
				key = 0;
			}
			// Use case 3: Deactivate the alarm
			else if(key == 'C')
			{
				// Deactivate all
				AlarmControl = 0;
				soil_limit_value = 0;
				// Reset the value
				key = 0;
			}
			// Use case 4: Display the soil limit that was set
			else if(key == 'D')
			{
				Lcd_clear_4bit();
				sprintf(keypad_msg.message, "Limit set: %d", soil_limit_value);
				Lcd_string_xy(1, 0, keypad_msg.message);
				// Reset the key value
				key = 0;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void alarm_thread(void *arg)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
	for(;;)
	{
		if(AlarmControl == 1)
		{
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
			vTaskDelay(pdMS_TO_TICKS(1000));
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
		else
		{
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_FATFS_Init();
  MX_SPI1_Init();

  xTaskCreate(lcd_thread, "Lcd task", 128, NULL, 2, &lcd_task);
  xTaskCreate(keypad_thread, "Keypad task", 128, NULL, 2, &keypad_task);
  xTaskCreate(soil_thread, "Soil moisture task", 128, NULL, 2, &soil_task);
  xTaskCreate(alarm_thread, "Alarm task", 128, NULL, 2, &alarm_task);
  // Note: Pending -> Time stamp
  xTaskCreate(logger_thread, "Logging task", 1024, NULL, 2, &logger_task);

  vTaskStartScheduler();

  while (1)
  {
	  // Should never enter here
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  // Button interrupt setup
  GPIO_InitTypeDef button = {0};
  button.Pin = GPIO_PIN_0;
  button.Mode = GPIO_MODE_IT_FALLING;
  button.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &button);
  // Button as an interrupt
  // Important not to have it set between 0 - 4. Only 5 and above otherwise it
  // will get stuck in port.c
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 5);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */


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

#ifdef  USE_FULL_ASSERT
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
