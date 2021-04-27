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
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rc522.h"
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
uint8_t 	k;
uint8_t 	i;
uint8_t 	j;
uint8_t 	b;
uint8_t 	q;
uint8_t 	en;
uint8_t 	ok;
uint8_t 	comand;
uint8_t		text1[63] = "STM32F103 Mifare RC522 RFID Card reader 13.56 MHz for KEIL HAL\r";
uint8_t		text2[9] = "Card ID: ";
uint8_t		end[1] = "\r";
uint8_t		txBuffer[18] = "Card ID: 00000000\r";
uint8_t 	retstr[10];
uint8_t 	rxBuffer[8];
uint8_t		lastID[4];
uint8_t		memID[8] = "9C55A1B5";
uint8_t		str[MFRC522_MAX_LEN];																						// MFRC522_MAX_LEN = 16
//uint32_t 	num;
//uint8_t*	txNum = (unsigned char*)&num;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

// RC522
uint8_t MFRC522_Check(uint8_t* id);
uint8_t MFRC522_Compare(uint8_t* CardID, uint8_t* CompareID);
void MFRC522_WriteRegister(uint8_t addr, uint8_t val);
uint8_t MFRC522_ReadRegister(uint8_t addr);
void MFRC522_SetBitMask(uint8_t reg, uint8_t mask);
void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask);
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t* TagType);
uint8_t MFRC522_ToCard(uint8_t command, uint8_t* sendData, uint8_t sendLen, uint8_t* backData, uint16_t* backLen);
uint8_t MFRC522_Anticoll(uint8_t* serNum);
void MFRC522_CalulateCRC(uint8_t* pIndata, uint8_t len, uint8_t* pOutData);
uint8_t MFRC522_SelectTag(uint8_t* serNum);
uint8_t MFRC522_Auth(uint8_t authMode, uint8_t BlockAddr, uint8_t* Sectorkey, uint8_t* serNum);
uint8_t MFRC522_Read(uint8_t blockAddr, uint8_t* recvData);
uint8_t MFRC522_Write(uint8_t blockAddr, uint8_t* writeData);
void MFRC522_Init(void);
void MFRC522_Reset(void);
void MFRC522_AntennaOn(void);
void MFRC522_AntennaOff(void);
void MFRC522_Halt(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// string hex to char number
uint8_t hex_to_char(uint8_t data) {
	uint8_t number;

	if (rxBuffer[data] < 58) number = (rxBuffer[data]-48)*16; else number = (rxBuffer[data]-55)*16;
	data++;
	if (rxBuffer[data] < 58) number = number+(rxBuffer[data]-48); else number = number+(rxBuffer[data]-55);
	return number;
}

// char number to string hex (FF) (Only big letters!)
void char_to_hex(uint8_t data) {
	uint8_t digits[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	if (data < 16) {
		retstr[0] = '0';
		retstr[1] = digits[data];
	} else {
		retstr[0] = digits[(data & 0xF0)>>4];
		retstr[1] = digits[(data & 0x0F)];
	}
}

// Number to string (16 bit)
void StrTo() {
	uint32_t	number;
	uint32_t 	ret;

	number = 0;
	ret = 0;
	ret = retstr[0]-0x30;
	number = ret*1000000;
	ret = retstr[1]-0x30;
	number = number+ret*100000;
	ret = retstr[2]-0x30;
	number = number+ret*10000;
	ret = retstr[3]-0x30;
	number = number+ret*1000;
	ret = retstr[4]-0x30;
	number = number+ret*100;
	ret = retstr[5]-0x30;
	number = number+ret*10;
	ret = retstr[6]-0x30;
	number = number+ret;
}

// String to number (32 bit)
void ToStr(uint32_t number) {
	uint32_t i;

	i = number/1000000000;
	number = number-i*1000000000;
	retstr[0] = i+0x30;

	i=number/100000000;
	number=number-i*100000000;
	retstr[1] = i+0x30;

	i = number/10000000;
	number = number-i*10000000;
	retstr[2] = i+0x30;

	i = number/1000000;
	number = number-i*1000000;
	retstr[3] = i+0x30;

	i = number/100000;
	number = number-i*100000;
	retstr[4] = i+0x30;

	i = number/10000;
	number = number-i*10000;
	retstr[5] = i+0x30;

	i = number/1000;
	number = number-i*1000;
	retstr[6] = i+0x30;

	i = number/100;
	number = number-i*100;
	retstr[7] = i+0x30;

	i = number/10;
	number = number-i*10;
	retstr[8] = i+0x30;

	retstr[9] = number+0x30;
}

void led(uint8_t n) {
	for (uint8_t i=0; i<n; i++) {
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);			          // LED1 ON
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);				          // LED1 OFF
		HAL_Delay(100);
	}
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
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);			         				// LED2 ON
  	HAL_Delay(10);
  	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);				     				// LED2 OFF
  	HAL_Delay(50);

  	led(1);
  	MFRC522_Init();
  	led(1);
  	HAL_UART_Transmit(&huart1, text1, 63, 100);
  	//CDC_Transmit_FS(text1, 63);
  	led(1);
  	HAL_UART_Receive_IT(&huart1,(uint8_t*)rxBuffer, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		if (!MFRC522_Request(PICC_REQIDL, str)) {
		/*	if (!MFRC522_Anticoll(str)) {
				j = 0;
				q = 0;
				b = 9;
				en = 1;

				for (i=0; i<4; i++) if (lastID[i] != str[i]) j = 1;								// Repeat test

				if (j && en) {
					q = 0;
					en = 0;
					for (i=0; i<4; i++) lastID[i] = str[i];

					for (i=0; i<4; i++) {
						char_to_hex(str[i]);
						txBuffer[b] = retstr[0];
						b++;
						txBuffer[b] = retstr[1];
						b++;
					}

					//HAL_UART_Transmit(&huart1, txBuffer, 18, 100);

					ok = 1;
					for (i=0; i<8; i++) if (txBuffer[9+i] != memID[i]) ok = 0;
					//led(1);
				}*/
			MFRC522_Read(1, str);
				HAL_UART_Transmit(&huart1, txBuffer, 18, 100);
				led(1);
			//}
		}

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
