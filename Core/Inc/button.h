
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

uint8_t Get_Button_State(void);





#ifdef __cplusplus
}
#endif


#endif /* INC_BUTTON_H_ */
