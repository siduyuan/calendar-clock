
#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include "main.h"

// 引脚定义
#define BUTTON_PORT GPIOB
#define ADD_PIN GPIO_PIN_14
#define SW_PIN GPIO_PIN_13
#define SUB_PIN GPIO_PIN_12

//电平定义
#define DOWN_STATE 0
#define UP_STATE 1

//按键事件位定义（EXTI 中断回调置位）
#define KEY_ADD  0x01
#define KEY_SW   0x02
#define KEY_SUB  0x04

//EXTI 中断置位的按键事件标志（ISR 中修改，必须 volatile）
extern volatile uint8_t key_event;





#ifdef __cplusplus
}
#endif


#endif /* INC_BUTTON_H_ */
