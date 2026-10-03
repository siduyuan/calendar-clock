#include "button.h"


static uint8_t Get_Add_Button_State(void)
{
    if (HAL_GPIO_ReadPin(BUTTON_PORT, ADD_PIN) == DOWN_STATE)
    {
        osDelay(20); // 消抖确认
        if (HAL_GPIO_ReadPin(BUTTON_PORT, ADD_PIN) == DOWN_STATE)
        	return 1;
    }
    return 0;
}

static uint8_t Get_Sw_Button_State(void)
{
    if (HAL_GPIO_ReadPin(BUTTON_PORT, SW_PIN) == DOWN_STATE)
    {
        osDelay(40); // 消抖确认
        if (HAL_GPIO_ReadPin(BUTTON_PORT, SW_PIN) == DOWN_STATE)
        	return 1;
    }
    return 0;
}

static uint8_t Get_Sub_Button_State(void)
{
    if (HAL_GPIO_ReadPin(BUTTON_PORT, SUB_PIN) == DOWN_STATE)
    {
        osDelay(20); // 消抖确认
        if (HAL_GPIO_ReadPin(BUTTON_PORT, SUB_PIN) == DOWN_STATE)
        	return 1;
    }
    return 0;
}


/**
 * @brief 获取三个按钮状态
 * @return 取值[-1,2],分别表示无状态，减小，模式切换，增加
 */
uint8_t Get_Button_State(void)
{
	if(Get_Add_Button_State()==1)return 2;
	if(Get_Sw_Button_State()==1)return 1;
	if(Get_Sub_Button_State()==1)return 0;
	return 255;
}
