/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "gpio.h"

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

/* USER CODE BEGIN PV */
uint8_t OLEDV = 0;
uint8_t mod = 0;
uint8_t key_changed = 0;  // 按键改动标志
volatile uint32_t rtc_total_seconds = 0;  // 开机累计秒数
uint32_t last_rtc_seconds = 0;  // 上次RTC秒数
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C1_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  Calendar_Init();
  OLED_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
/*
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET ) {
      if(OLEDV == 0){
        HAL_Delay(20);
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET){
          OLED_Init();
          OLEDV = 1;
          key_changed = 1;  // 初始化后强制立即刷新一次
        }
      }
    } 
    else if(OLEDV == 1){
    OLEDV = 0;
    }
*/
    // 1. 判断按键事件
    uint8_t state = Get_Button_State();
    switch(state)
    {
      case 0: // 减小
        if(mod != 0) {
          Calendar_ChangeTime(mod, -1);
          key_changed = 1;
        }
        break;
      case 1: // 模式切换
        mod++;
        if(mod > 6) mod = 0;
        key_changed = 1;
        break;
      case 2: // 增加
        if(mod != 0) {
          Calendar_ChangeTime(mod, 1);
          key_changed = 1;
        }
        break;
    }
    
    // 2. 计算秒数差值和判断按键改动标志，刷新屏幕
    uint32_t elapsed_seconds = rtc_total_seconds - last_rtc_seconds;
    
    // 3. 如果超过1秒或按键改动，刷新屏幕
    if(elapsed_seconds >= 1 || key_changed)
    {
      // 更新时间
      if(elapsed_seconds >= 1) {
        for(uint32_t i = 0; i < elapsed_seconds; i++) {
          Calendar_Increment();
        }
        last_rtc_seconds = rtc_total_seconds;
      }
  
      // 刷新屏幕
      //if(OLEDV) {//临时测试。
        OLED_NewFrame();
        OLED_PrintString(2, 0, Calendar_Getdatestr(), &font15x15, OLED_COLOR_NORMAL); // 日期显示
        OLED_PrintString(100, 0, Calendar_Getweek_daystr(), &font15x15, OLED_COLOR_NORMAL); // 星期显示
        OLED_PrintString(20, 14, Calendar_Gettimestr(), &font16x16, OLED_COLOR_NORMAL); // 时间显示
        
        if(Calendar_Get_lunar_moth_leap() != 0) {
          OLED_PrintString(2, 20, "闰", &font15x15, OLED_COLOR_NORMAL); // 闰字显示
        }
        OLED_PrintString(2, 38, Calendar_GetlunardateStr(), &font15x15, OLED_COLOR_NORMAL); // 农历显示
        OLED_PrintString(2, 53, Calendar_GetFourPillarStr(), &font12x12, OLED_COLOR_NORMAL); // 干支显示
        
        // 如果处于模式调整状态，绘制选择框
        if(mod) {
          switch(mod) {
            case 1: OLED_DrawRectangle(1, 0, 32, 16, OLED_COLOR_NORMAL); break;
            case 2: OLED_DrawRectangle(48, 0, 17, 16, OLED_COLOR_NORMAL); break;
            case 3: OLED_DrawRectangle(79, 0, 17, 16, OLED_COLOR_NORMAL); break;
            case 4: OLED_DrawRectangle(18, 16, 27, 20, OLED_COLOR_NORMAL); break;
            case 5: OLED_DrawRectangle(54, 16, 27, 20, OLED_COLOR_NORMAL); break;
            case 6: OLED_DrawRectangle(90, 16, 27, 20, OLED_COLOR_NORMAL); break;
          }
        }
        
        OLED_ShowFrame();
      //}
      
      // 清除按键标志
      key_changed = 0;
    }

    // 短暂延时，降低CPU占用
    HAL_Delay(10);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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

// RTC 秒中断回调函数（F1 系列回调名为 RTCEventCallback）
void HAL_RTCEx_RTCEventCallback(RTC_HandleTypeDef *hrtc)
{
  rtc_total_seconds++;
}