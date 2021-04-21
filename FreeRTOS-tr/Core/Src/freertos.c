/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "printf.h"
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
/* USER CODE BEGIN Variables */
typedef struct
{
	uint32_t Value;
	uint32_t Sender;
}Message_t;
/* USER CODE END Variables */
/* Definitions for ProducerTask */
osThreadId_t ProducerTaskHandle;
const osThreadAttr_t ProducerTask_attributes = {
  .name = "ProducerTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal7,
};
/* Definitions for CostumerTask */
osThreadId_t CostumerTaskHandle;
const osThreadAttr_t CostumerTask_attributes = {
  .name = "CostumerTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for BinarySem01 */
osSemaphoreId_t BinarySem01Handle;
const osSemaphoreAttr_t BinarySem01_attributes = {
  .name = "BinarySem01"
};
/* Definitions for IRQSemaphore */
osSemaphoreId_t IRQSemaphoreHandle;
const osSemaphoreAttr_t IRQSemaphore_attributes = {
  .name = "IRQSemaphore"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartProducerTask(void *argument);
void StartCostumerTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of BinarySem01 */
  BinarySem01Handle = osSemaphoreNew(1, 1, &BinarySem01_attributes);

  /* creation of IRQSemaphore */
  IRQSemaphoreHandle = osSemaphoreNew(1, 1, &IRQSemaphore_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of ProducerTask */
  ProducerTaskHandle = osThreadNew(StartProducerTask, NULL, &ProducerTask_attributes);

  /* creation of CostumerTask */
  CostumerTaskHandle = osThreadNew(StartCostumerTask, NULL, &CostumerTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartProducerTask */
/**
  * @brief  Function implementing the ProducerTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartProducerTask */
void StartProducerTask(void *argument)
{
  /* USER CODE BEGIN StartProducerTask */

  /* Infinite loop */
  for(;;)
  {
	  printf("In Task Producer\n\r");

	  printf("Wait for semaphore\n\r");
	 if(osOK== osSemaphoreAcquire(BinarySem01Handle, osWaitForever))
	 {
		 printf("Semaphore Producer Taken\n\r");
	 }
		  printf("Producer Task Exit\n\r");
	  	 // osDelay(1000);
  }
  /* USER CODE END StartProducerTask */
}

/* USER CODE BEGIN Header_StartCostumerTask */
/**
* @brief Function implementing the CostumerTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCostumerTask */
void StartCostumerTask(void *argument)
{
  /* USER CODE BEGIN StartCostumerTask */
  /* Infinite loop */
  for(;;)
  {
	  printf("Enter Cost Task\n\r");

	/*  if(osOK == osSemaphoreRelease(BinarySem01Handle))
	  {
		  printf("BinSem01 Release\n\r");
	  }*/
	  printf("Exit Cost Task\n\r");

     osDelay(1000);
  }
  /* USER CODE END StartCostumerTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void _putchar(char character)
{
	//osSemaphoreAcquire(PrintSemaphoreHandle, osWaitForever);
	HAL_UART_Transmit(&huart2, (uint8_t*)&character, 1, 100);
	//osSemaphoreRelease(PrintSemaphoreHandle);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

	if(B1_Pin==GPIO_Pin)
	{
		osSemaphoreRelease(BinarySem01Handle);
	}
}


/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
