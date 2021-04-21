/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "image.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void drawLines()
{
  for (int16_t i = 0; i < ssd1306_GetWidth(); i += 4)
  {
    ssd1306_DrawLine(0, 0, i, ssd1306_GetHeight() - 1);
    ssd1306_UpdateScreen();
    HAL_Delay(10);
  }
  for (int16_t i = 0; i < ssd1306_GetHeight(); i += 4)
  {
    ssd1306_DrawLine(0, 0, ssd1306_GetWidth() - 1, i);
    ssd1306_UpdateScreen();
    HAL_Delay(10);
  }
  HAL_Delay(250);

  ssd1306_Clear();
  for (int16_t i = 0; i < ssd1306_GetWidth(); i += 4)
  {
   ssd1306_DrawLine(0, ssd1306_GetHeight() - 1, i, 0);
   ssd1306_UpdateScreen();
   HAL_Delay(10);
  }
  for (int16_t i = ssd1306_GetHeight() - 1; i >= 0; i -= 4)
  {
   ssd1306_DrawLine(0, ssd1306_GetHeight() - 1, ssd1306_GetWidth() - 1, i);
   ssd1306_UpdateScreen();
   HAL_Delay(10);
  }
  HAL_Delay(250);
  ssd1306_Clear();
  for (int16_t i = ssd1306_GetWidth() - 1; i >= 0; i -= 4)
  {
    ssd1306_DrawLine(ssd1306_GetWidth() - 1, ssd1306_GetHeight() - 1, i, 0);
    ssd1306_UpdateScreen();
    HAL_Delay(10);
  }
  for (int16_t i = ssd1306_GetHeight() - 1; i >= 0; i -= 4)
  {
    ssd1306_DrawLine(ssd1306_GetWidth() - 1, ssd1306_GetHeight() - 1, 0, i);
    ssd1306_UpdateScreen();
    HAL_Delay(10);
  }
  HAL_Delay(250);
  ssd1306_Clear();
  for (int16_t i = 0; i < ssd1306_GetHeight(); i += 4)
  {
    ssd1306_DrawLine(ssd1306_GetWidth() - 1, 0, 0, i);
    ssd1306_UpdateScreen();
    HAL_Delay(10);
  }
  for (int16_t i = 0; i < ssd1306_GetWidth(); i += 4)
  {
    ssd1306_DrawLine(ssd1306_GetWidth() - 1, 0, i, ssd1306_GetHeight() - 1);
    ssd1306_UpdateScreen();
    HAL_Delay(10);
  }
  HAL_Delay(250);
}

// Adapted from Adafruit_SSD1306
void drawRect(void)
{
  for (int16_t i = 0; i < ssd1306_GetHeight() / 2; i += 2)
  {
    ssd1306_DrawRect(i, i, ssd1306_GetWidth() - 2 * i, ssd1306_GetHeight() - 2 * i);
    ssd1306_UpdateScreen();
    HAL_Delay(10);
  }
}

// Adapted from Adafruit_SSD1306
void fillRect(void) {
  uint8_t color = 1;
  for (int16_t i = 0; i < ssd1306_GetHeight() / 2; i += 3)
  {
    ssd1306_SetColor((color % 2 == 0) ? Black : White); // alternate colors
    ssd1306_FillRect(i, i, ssd1306_GetWidth() - i * 2, ssd1306_GetHeight() - i * 2);
    ssd1306_UpdateScreen();
    HAL_Delay(10);
    color++;
  }
  // Reset back to WHITE
  ssd1306_SetColor(White);
}

// Adapted from Adafruit_SSD1306
void drawCircle(void)
{
  for (int16_t i = 0; i < ssd1306_GetHeight(); i += 2)
  {
    ssd1306_DrawCircle(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, i);
    ssd1306_UpdateScreen();
    HAL_Delay(10);
  }
  HAL_Delay(1000);
  ssd1306_Clear();

  // This will draw the part of the circel in quadrant 1
  // Quadrants are numberd like this:
  //   0010 | 0001
  //  ------|-----
  //   0100 | 1000
  //
  ssd1306_DrawCircleQuads(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, ssd1306_GetHeight() / 4, 0b00000001);
  ssd1306_UpdateScreen();
  HAL_Delay(200);
  ssd1306_DrawCircleQuads(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, ssd1306_GetHeight() / 4, 0b00000011);
  ssd1306_UpdateScreen();
  HAL_Delay(200);
  ssd1306_DrawCircleQuads(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, ssd1306_GetHeight() / 4, 0b00000111);
  ssd1306_UpdateScreen();
  HAL_Delay(200);
  ssd1306_DrawCircleQuads(ssd1306_GetWidth() / 2, ssd1306_GetHeight() / 2, ssd1306_GetHeight() / 4, 0b00001111);
  ssd1306_UpdateScreen();
}

void drawProgressBarDemo(int counter)
{
 char str[128];
  // draw the progress bar
  ssd1306_DrawProgressBar(0, 32, 120, 10, counter);

  // draw the percentage as String
  ssd1306_SetCursor(64, 15);
  sprintf(str, "%i%%", counter);
  ssd1306_WriteString(str, Font_7x10);
  ssd1306_UpdateScreen();
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  ssd1306_Init();
   ssd1306_FlipScreenVertically();
   ssd1306_Clear();
   ssd1306_SetColor(White);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  drawLines();
	  HAL_Delay(1000);
	  ssd1306_Clear();

	  drawRect();
	  HAL_Delay(1000);
	  ssd1306_Clear();

	  fillRect();
	  HAL_Delay(1000);
	  ssd1306_Clear();

	  drawCircle();
	  HAL_Delay(1000);
	  ssd1306_Clear();

	  for(int i = 0; i < 100; i++)
	  {
	    drawProgressBarDemo(i);
	    HAL_Delay(25);
	    ssd1306_Clear();
	  }

	  ssd1306_DrawRect(0, 0, ssd1306_GetWidth(), ssd1306_GetHeight());
	  ssd1306_SetCursor(8, 20);
	  ssd1306_WriteString("SSD1306", Font_16x26);
	  ssd1306_UpdateScreen();
	  HAL_Delay(2000);
	  ssd1306_Clear();
	  ssd1306_DrawBitmap(0, 0, 128, 64, stm32fan);
	  ssd1306_UpdateScreen();
	  HAL_Delay(2000);
	  ssd1306_InvertDisplay();
	  HAL_Delay(2000);
	  ssd1306_NormalDisplay();
	  ssd1306_Clear();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
