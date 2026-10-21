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
volatile uint32_t rtc_total_seconds = 0;  /* 开机累计秒数（闹钟回调自增，main.c 经 extern 消费） */
static volatile uint32_t last_alarm_value = 0;  /* 上次写入 ALR 的值；回调读 CNT 后防御比较用（正常路径不触发） */

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

/* 一次性配置：使能闹钟中断 + EXTI17 唤醒线 + 装载首次闹钟。
   RTC 时钟使能与 NVIC 优先级已由 HAL_RTC_MspInit（MX_RTC_Init 内部）完成，此处不重复。
   启动时影子寄存器仍与 RTC 核心同步，允许此处读一次 CNT 作为初始基准（非运行期路径）。 */
void RTC_Alarm_Config(RTC_HandleTypeDef *hrtc)
{
  RTC_TypeDef *rtc = hrtc->Instance;

  __HAL_RTC_ALARM_ENABLE_IT(hrtc, RTC_IT_ALRA);

  /* 手动使能 EXTI Line 17（HAL/CubeMX 不会自动配置，唤醒 STOP 的关键） */
  EXTI->IMR  |= (1UL << 17);
  EXTI->RTSR |= (1UL << 17);
  EXTI->PR    = (1UL << 17);

  /* 初始基准：当前秒 + 1（启动期读一次 CNT，影子寄存器刚完成同步，非运行期路径） */
  last_alarm_value = (((uint32_t)rtc->CNTH << 16U) | (uint32_t)rtc->CNTL) + 1U;
  RTC_WriteAlarm(hrtc, last_alarm_value);
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
  * @note   设计：闹钟触发后先等影子寄存器同步（清 RSF → 轮询置位，1~2 RTCCLK ≈ 30~61µs，
  *         LSI 32768Hz），此时 CNT 必已越过触发边界（= 上次闹钟值 + 1），读 CNT 直接写
  *         ALR = CNT，无需 +1。ALRF/EXTI17 由 HAL_RTC_AlarmIRQHandler 在回调返回后统一清，
  *         回调内不清标志。进入 STOP 模式时同样由本回调经 EXTI17 唤醒。
  */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  RTC_TypeDef *rtc = hrtc->Instance;
  uint16_t high1, high2, low;
  uint32_t cnt;

  /* 1. 影子寄存器同步（STOP 唤醒后首读 CNT 必须先做，避免陈旧值）。
        与 HAL_RTC_WaitForSynchro 同手法：清 RSF → 轮询 CRL.RSF 置位 */
  CLEAR_BIT(rtc->CRL, RTC_FLAG_RSF);
  while ((rtc->CRL & RTC_FLAG_RSF) == RESET) {}

  /* 2. 双读法取 CNT：CNTH/CNTL 各 16 位，读低再读高可能跨位回卷，需二次核对高位 */
  do {
    high1 = rtc->CNTH;
    low   = rtc->CNTL;
    high2 = rtc->CNTH;
  } while (high1 != high2);
  cnt = ((uint32_t)high1 << 16U) | low;

  /* 3. 防御：正常同步后 CNT 必 == 上次闹钟值 + 1；若读到 <= 上次值（边界竞态、同步未越过），
        强制前推一位，避免写回旧值导致闹钟永不触发（心跳打停）。正常路径不触发。 */
  if (cnt <= last_alarm_value) {
    cnt = (uint32_t)last_alarm_value + 1U;
  }
  last_alarm_value = cnt;

  /* 4. 写 ALR = CNT（复用 RTC_WriteAlarm：等 RTOFF → CNF → 写 ALRH/ALRL → 退 CNF） */
  RTC_WriteAlarm(hrtc, cnt);

  /* 5. 累计开机秒数（volatile，主循环差值驱动日历） */
  rtc_total_seconds++;
}
/* USER CODE END 1 */
