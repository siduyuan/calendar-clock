/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
#include "rtc.h"

/* USER CODE BEGIN 0 */
extern volatile uint32_t rtc_total_seconds;     /* main.c 定义：开机累计秒数（闹钟回调自增） */
static volatile uint32_t next_alarm_target = 0; /* 下一次闹钟目标值（回调自增并直接写 ALR） */

/* 最小写入：把 target 写入 ALR。不读 CNT、不清 ALRF（由 HAL 中断服务在回调后统一清）。
   关键点：F1 的 RTC 寄存器受写保护，写入前必须置 CRL.CNF 进入配置模式，否则被静默忽略。 */
static void RTC_WriteAlarm(RTC_HandleTypeDef *hrtc, uint32_t target)
{
  RTC_TypeDef *rtc = hrtc->Instance;

  while ((rtc->CRL & RTC_CRL_RTOFF) == 0U)   /* 等上次写完成 */
  {
  }
  rtc->CRL |= RTC_CRL_CNF;                    /* 配置模式：可写 */
  rtc->ALRH = (uint16_t)(target >> 16U);
  rtc->ALRL = (uint16_t)(target & 0xFFFFU);
  rtc->CRL &= ~(uint32_t)RTC_CRL_CNF;         /* 退出配置模式，恢复计数 */
  while ((rtc->CRL & RTC_CRL_RTOFF) == 0U)
  {
  }
}

/* 一次性配置：使能闹钟中断 + EXTI17 唤醒线 + NVIC + 装载首次闹钟。
   启动时影子寄存器仍与 RTC 核心同步，允许此处读一次 CNT 作为初始基准（非运行期路径）。 */
void RTC_Alarm_Config(RTC_HandleTypeDef *hrtc)
{
  RTC_TypeDef *rtc = hrtc->Instance;

  __HAL_RCC_RTC_ENABLE();
  __HAL_RTC_ALARM_ENABLE_IT(hrtc, RTC_IT_ALRA);

  /* 手动使能 EXTI Line 17（HAL/CubeMX 不会自动配置，唤醒 STOP 的关键） */
  EXTI->IMR  |= (1UL << 17);
  EXTI->RTSR |= (1UL << 17);
  EXTI->PR    = (1UL << 17);

  HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

  /* 初始基准：当前秒 + 1 */
  next_alarm_target = (((uint32_t)rtc->CNTH << 16U) | (uint32_t)rtc->CNTL) + 1U;
  RTC_WriteAlarm(hrtc, next_alarm_target);
}
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */
  /* 关闭闹钟输出到 PC13/TAMPER(BKP_RTCCR.ASOE)，避免绿灯被闹钟强亮。
     放在 USER CODE 区，CubeMX 重生成 rtc.c(OutPut 仍为 ALARM)也不会还原 */
  BKP->RTCCR &= ~(uint32_t)BKP_RTCCR_ASOE;
  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();

    /* RTC interrupt Init */
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();

    /* RTC interrupt Deinit */
    HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
  * @brief  RTC 闹钟 A 事件回调（1 秒心跳，每秒触发一次）
  * @note   累计开机秒数并装载下一秒闹钟；进入 STOP 模式时同样由它经 EXTI17 唤醒。
  */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  rtc_total_seconds++;              /* 累计秒数（volatile，主循环差值驱动日历） */
  next_alarm_target += 1U;          /* 下一次目标在软件里自增（不读影子寄存器，免疫 STOP 陈旧值） */
  RTC_WriteAlarm(hrtc, next_alarm_target);  /* 直接写硬件；闹钟刚触发，写时序安全 */
}
/* USER CODE END 1 */
