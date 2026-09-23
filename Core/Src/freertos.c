/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "driver_led.h"
#include "driver_usart.h"
#include "driver_dht11.h"
#include "driver_modbus_reg.h" 
#include "usart.h" 

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


/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myTask02 */
osThreadId_t myTask02Handle;
const osThreadAttr_t myTask02_attributes = {
  .name = "myTask02",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myTask03 */
osThreadId_t myTask03Handle;
const osThreadAttr_t myTask03_attributes = {
  .name = "myTask03",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myQueue01 */
osMessageQueueId_t myQueue01Handle;
const osMessageQueueAttr_t myQueue01_attributes = {
  .name = "myQueue01"
};
/* Definitions for modbusQueue */
osMessageQueueId_t modbusQueueHandle;
const osMessageQueueAttr_t modbusQueue_attributes = {
  .name = "modbusQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void Modbus_Task(void *argument); 

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask02(void *argument);
void StartTask03(void *argument);

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

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of myQueue01 */
  myQueue01Handle = osMessageQueueNew (16, sizeof(uint16_t), &myQueue01_attributes);

  /* creation of modbusQueue */
  modbusQueueHandle = osMessageQueueNew (16, 16, &modbusQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
	 Dht11_QueueInit();
	 
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of myTask02 */
  myTask02Handle = osThreadNew(StartTask02, NULL, &myTask02_attributes);

  /* creation of myTask03 */
  myTask03Handle = osThreadNew(StartTask03, NULL, &myTask03_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
	//osThreadNew(Led_BlinkTask, NULL, NULL);
	
	//Modbus_TaskHandle = osThreadNew(Modbus_Task,   NULL, &Modbus_Task_attributes); 

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */

  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
     //HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);   /* ??PA0 */
    osDelay(1);
		//Led_Test();
		//Usart_Test();
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the myTask02 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
  /* Infinite loop */
  for(;;)
  {
    Dht11_Task(argument);  
		//osDelay(1);
    //Led_Test();
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the myTask03 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void *argument)
{
  /* USER CODE BEGIN StartTask03 */
  /* Infinite loop */
	
  for(;;)
  {
    Dht11_ProcessTask(argument); 
		//osDelay(1000);
  }
  /* USER CODE END StartTask03 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

#include "cmsis_os2.h"
#include "usart.h"
#include <string.h>

extern osMessageQueueId_t modbusQueueHandle;
extern UART_HandleTypeDef huart2;


/* CRC16 Modbus */
static uint16_t Modbus_CRC16(uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= buf[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

/* Modbus?? */
void Modbus_Parse(uint8_t *buf, uint8_t len)
{
	//HAL_UART_Transmit(&huart2, (uint8_t*)"P", 1, 100);
	 //HAL_UART_Transmit(&huart2, (uint8_t*)"P\r\n", 3, 100);
    if (len < 8) return;
    if (buf[0] != 0x01) return;

    uint16_t crc_calc = Modbus_CRC16(buf, len - 2);
    uint16_t crc_recv = buf[len - 2] | (buf[len - 1] << 8);
    if (crc_calc != crc_recv) return;

    if (buf[1] == 0x03)
    {
        uint16_t start_addr = (buf[2] << 8) | buf[3];
        uint16_t reg_count  = (buf[4] << 8) | buf[5];

        uint8_t resp[32];
        uint8_t idx = 0;
        resp[idx++] = 0x01;
        resp[idx++] = 0x03;
        resp[idx++] = reg_count * 2;

        for (uint16_t i = 0; i < reg_count; i++)
        {
            uint16_t val = ModbusReg_GetHolding(start_addr + i);
            resp[idx++] = (val >> 8) & 0xFF;
            resp[idx++] = val & 0xFF;
        }

        uint16_t crc = Modbus_CRC16(resp, idx);
        resp[idx++] = crc & 0xFF;
        resp[idx++] = (crc >> 8) & 0xFF;

        HAL_UART_Transmit(&huart2, resp, idx, 100);
    }
}

/* Modbus?? */
void Modbus_Poll(void)
{
    uint8_t frame[16];

    if (osMessageQueueGet(modbusQueueHandle, frame, NULL, 0) == osOK)
    {
        Modbus_Parse(frame, 8);
    }
}

/* USER CODE END Application */

