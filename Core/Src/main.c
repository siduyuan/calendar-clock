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
#include <oled.h>
#include <font.h>
#include "button.h"
#include "calendar.h"
#include "timestamp_flash.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 时间分阶局部刷新区域参数（font16x16 的 ASCII 子字库 afont24x12：每字12px宽、24px高）
#define TIME_REF_PAGE   2  // 局部刷新起始页（0起始，按当前设定2-4页）
#define TIME_REF_PAGES  3  // 局部刷新页数
#define TIME_SEC_COL    92 // 秒数字起始列（SS 两字 → 列92-115）
#define TIME_SEC_LEN    24 // 秒区域列长度
#define TIME_MS_COL     56 // 分+秒区域起始列（MM:SS 五字 → 列56-115）
#define TIME_MS_LEN     60 // 分+秒区域列长度

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
uint32_t last_key_rtc_seconds = 0;  // 最近一次按键事件对应的累计秒数（控制STOP进入，主循环逻辑，非按键逻辑）
extern TIM_HandleTypeDef htim4;  // 系统滴答由 TIM4 维护，STOP 前后需手动管理
static uint8_t last_draw_hour = 0xFF;   // 最近一次全量刷新的小时（整点全量刷新判定）
static uint8_t last_draw_minute = 0xFF; // 最近一次刷新（含局部）的分钟（整分局部刷新判定）
static uint8_t last_ts_hour = 0xFF;     // 最近一次写入备份区的小时（每小时写一次时间戳判定）
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SystemClock_ResetPLL(void);  // STOP 唤醒后轻量恢复，不重配 RTC（用户代码，防被重生成清掉）
void Display_FullRefresh(void);   // 全量重绘整帧并整屏刷新（开机/整点/按键改动时调用）
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

  TS_Init();// 初始化时间戳存储库
  Calendar_Init();
  OLED_Init();
  RTC_Alarm_Config(&hrtc);  /* 一次性配置闹钟中断+EXTI17并以闹钟启动 1 秒心跳 */
  Display_FullRefresh();    /* 开机整帧绘制+整屏刷新，建立基础画布 */
  last_draw_hour = calendar.hour;
  last_draw_minute = calendar.minute;
  last_ts_hour = calendar.hour;  // 备份小时基准：当前小时不再写，下个整点才写


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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

    // 1. 判断按键事件（EXTI 中断置位 key_event，主循环消抖处理后清零）
    if (key_event)
    {
      last_key_rtc_seconds = rtc_total_seconds;  // 记录最近按键时刻，控制 STOP 进入条件

      HAL_Delay(20);  // 消抖

      if ((key_event & KEY_ADD) && (HAL_GPIO_ReadPin(BUTTON_PORT, ADD_PIN) == DOWN_STATE))
      {
        if (mod != 0) {
          Calendar_ChangeTime(mod, 1);
          key_changed = 1;
        }
      }
      if ((key_event & KEY_SW) && (HAL_GPIO_ReadPin(BUTTON_PORT, SW_PIN) == DOWN_STATE))
      {
        mod++;
        if(mod > 6) mod = 0;
        key_changed = 1;
      }
      if ((key_event & KEY_SUB) && (HAL_GPIO_ReadPin(BUTTON_PORT, SUB_PIN) == DOWN_STATE))
      {
        if (mod != 0) {
          Calendar_ChangeTime(mod, -1);
          key_changed = 1;
        }
      }

      key_event = 0;  // 清除按键事件
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
  
      // 刷新屏幕（分阶局部覆盖+局部刷新：每秒仅覆盖秒区域，每分覆盖分+秒区域，整点/按键全量）
      if(OLEDV) {
        if(key_changed || (calendar.hour != last_draw_hour)) {
          // ===== 全量：重建基础画布并整屏刷新（开机/整点/按键改动） =====
          Display_FullRefresh();
          last_draw_hour = calendar.hour;
          last_draw_minute = calendar.minute;
        }
        else if(calendar.minute != last_draw_minute) {
          // ===== 整分：覆盖分+秒区域（MM:SS，含中间冒号）并局部刷新 =====
          OLED_PrintString(TIME_MS_COL, 14, Calendar_Gettimestr() + 3, &font16x16, OLED_COLOR_NORMAL);
          OLED_ShowFrameRegion(TIME_MS_COL, TIME_REF_PAGE, TIME_MS_LEN, TIME_REF_PAGES);
          last_draw_minute = calendar.minute;
        }
        else {
          // ===== 每秒：覆盖秒区域（SS）并局部刷新 =====
          OLED_PrintString(TIME_SEC_COL, 14, Calendar_Gettimestr() + 6, &font16x16, OLED_COLOR_NORMAL);
          OLED_ShowFrameRegion(TIME_SEC_COL, TIME_REF_PAGE, TIME_SEC_LEN, TIME_REF_PAGES);
        }
      }
     
      // 清除按键标志
      key_changed = 0;
    }

    // 4. 每小时写一次时间戳到备份区：小时变更即写，掉电后用于恢复时间/估算供电时长。
    //    写满后库内部自动整页擦除重来，正常使用无需理会。放在 STOP 决策之前执行。
    if (calendar.hour != last_ts_hour)
    {
      last_ts_hour = calendar.hour;  // 无论写成败都推进基准，避免每秒重试
      TS_Write(calendar.timestamp);
    }

    // 距上次按键中断不足 5 秒时保持运行（短延时快速响应连续按键），超过才进入 STOP
    if ((rtc_total_seconds - last_key_rtc_seconds) >= 5U)
    {
      // 降功耗：挂起系统滴答后进入 STOP，由 RTC 闹钟中断(EXTI17)每秒唤醒
      // 1. 收尾：屏蔽并清除 TIM4 滴答中断，避免残留更新事件让 WFI 立即返回无法真正入睡
      __HAL_TIM_DISABLE_IT(&htim4, TIM_IT_UPDATE);
      __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
      HAL_NVIC_ClearPendingIRQ(TIM4_IRQn);

      // 2. 进入 STOP：主稳压器工作（无额外唤醒延时）；唤醒后系统时钟默认为 HSI(8MHz)
      HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

      // 3. STOP 唤醒后恢复 HSE+PLL 72MHz（不重配 RTC，避免闹钟心跳掉拍）
      SystemClock_ResetPLL();

      // 4. 恢复系统滴答（TIM4），保证后续 HAL_Delay 消抖等正常
      __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
      HAL_NVIC_ClearPendingIRQ(TIM4_IRQn);
      __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
    }
    else
    {
      // 距上次按键不足 5 秒：保持短延时运行，保证一组连续按键操作不被打断
      HAL_Delay(10);
    }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief  STOP 唤醒后的轻量时钟恢复：仅恢复 HSE+PLL 72MHz、CPU/总线分频与 FLASH 等待。
  * @note   刻意不调用完整 SystemClock_Config()，避免每次唤醒重配 RTC 时钟源(LSI)——那会
  *         扰动 RTC 计数/闹钟对齐，导致 1 秒心跳掉拍成约 2 秒。唤醒后系统时钟默认已是 HSI。
  *         放到 USER CODE 区，避免 CubeMX 重新生成时被清除。
  * @retval None
  */
void SystemClock_ResetPLL(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* 重新使能 HSE 并回到 HSE+PLL(×9) 72MHz */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* CPU/AHB/APB 分频与 FLASH 等待；HAL_RCC_ClockConfig 内部会重建 TIM4 滴答 */
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

/**
  * @brief  全量重绘整帧并整屏刷新，用于建立/重建基础画布
  * @note   在开机初始化、整点切换、按键改动（含OLED重新上电）时调用；
  *         平时每秒/每分仅做局部覆盖与局部刷新，其余区域保持基础画布不变
  * @retval None
  */
void Display_FullRefresh(void)
{
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
}

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
