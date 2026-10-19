#include "button.h"

/* EXTI 按键事件标志：bit0=ADD(PB14) bit1=SW(PB13) bit2=SUB(PB12) */
volatile uint8_t key_event = 0;

/**
  * @brief  EXTI 中断回调（PB12/13/14 共享 EXTI15_10，下降沿触发）
  * @note   HAL 在调用本回调前已自动清除 EXTI 挂起位，这里只需记录是哪个键
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == ADD_PIN)      key_event |= KEY_ADD;
  else if (GPIO_Pin == SW_PIN)  key_event |= KEY_SW;
  else if (GPIO_Pin == SUB_PIN) key_event |= KEY_SUB;
}
