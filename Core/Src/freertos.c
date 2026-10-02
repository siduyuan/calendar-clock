/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <string.h>
#include <oled.h>
#include <font.h>
#include "button.h"
#include "calendar.h"
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
uint8_t mod=0;
/* USER CODE END Variables */
/* Definitions for TimeTask */
osThreadId_t TimeTaskHandle;
const osThreadAttr_t TimeTask_attributes = {
  .name = "TimeTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for OLEDTask */
osThreadId_t OLEDTaskHandle;
const osThreadAttr_t OLEDTask_attributes = {
  .name = "OLEDTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for ButtonTask */
osThreadId_t ButtonTaskHandle;
const osThreadAttr_t ButtonTask_attributes = {
  .name = "ButtonTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

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

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of TimeTask */
  TimeTaskHandle = osThreadNew(StartDefaultTask, NULL, &TimeTask_attributes);

  /* creation of OLEDTask */
  OLEDTaskHandle = osThreadNew(StartTask02, NULL, &OLEDTask_attributes);

  /* creation of ButtonTask */
  ButtonTaskHandle = osThreadNew(StartTask03, NULL, &ButtonTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
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
	    osDelay(1000);
	 Calendar_Increment();
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the OLEDTask thread.
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

	  OLED_NewFrame();
	  OLED_PrintString(2, 0, Calendar_Getdatestr(), &font15x15, OLED_COLOR_NORMAL);//日期显示
	  OLED_PrintString(100, 0, Calendar_Getweek_daystr(), &font15x15, OLED_COLOR_NORMAL);//星期显示
	  OLED_PrintString(20, 14, Calendar_Gettimestr(), &font16x16, OLED_COLOR_NORMAL);//时间显示
	  if(Calendar_Get_lunar_moth_leap()!=0)
	  {
		  OLED_PrintString(2, 20, "闰", &font15x15, OLED_COLOR_NORMAL);//闰字显示
	  }
	  OLED_PrintString(2, 38, Calendar_GetlunardateStr(), &font15x15, OLED_COLOR_NORMAL);//农历显示
	  OLED_PrintString(2, 53, Calendar_GetFourPillarStr(), &font12x12, OLED_COLOR_NORMAL);//干支显示

	  if(mod)
	  {
		  switch(mod)
		  {
		  case 1:OLED_DrawRectangle(1,0, 32, 16, OLED_COLOR_NORMAL);break;
		  case 2:OLED_DrawRectangle(48,0, 17, 16, OLED_COLOR_NORMAL);break;
		  case 3:OLED_DrawRectangle(79,0, 17, 16, OLED_COLOR_NORMAL);break;
		  case 4:OLED_DrawRectangle(18,16, 27, 20, OLED_COLOR_NORMAL);break;
		  case 5:OLED_DrawRectangle(54,16, 27, 20, OLED_COLOR_NORMAL);break;
		  case 6:OLED_DrawRectangle(90,16, 27, 20, OLED_COLOR_NORMAL);break;
		  }
	  }

	  OLED_ShowFrame();
	  osDelay(250);
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the ButtonTask thread.
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
	  uint8_t state=Get_Button_State();
	  switch(state)
	  {
	  case 0:if(mod!=0)Calendar_ChangeTime(mod,-1);break;
	  case 1:mod++;if(mod>6)mod-=7;break;
	  case 2:if(mod!=0)Calendar_ChangeTime(mod,1);break;
	  }
	  osDelay(100);
  }
  /* USER CODE END StartTask03 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

