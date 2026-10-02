#ifndef __CALENDAR_H
#define __CALENDAR_H

#include "stdint.h"

// 时间结构体
typedef struct {
    uint32_t timestamp;
    uint32_t lunar_month_start_timestamp;// 时间戳（1970年1月1日以来的秒数）
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t week_day;    // 星期几（0-6，0=周日）
    uint8_t refresh_flag;

    uint8_t lunar_day;//存储农历日期：初几十几廿几
    uint8_t lunar_month;//存储农历月份,正月腊月...
    uint8_t lunar_moth_leap;//存储农历该月是否闰月，如果是则存储月份，不是为0
    uint8_t lunar_Heavenly_year;      // 农历年天干
    uint8_t lunar_Heavenly_month;     // 农历月天干
    uint8_t lunar_Heavenly_day;       // 农历日天干
    uint8_t lunar_Heavenly_hour;      // 农历时天干
    uint8_t lunar_Earthly_year;       // 农历年地支
    uint8_t lunar_Earthly_month;      // 农历月地支
    uint8_t lunar_Earthly_day;        // 农历日地支
    uint8_t lunar_Earthly_hour;       // 农历时地支

    char date_str[15];   // 年月日字符串：xxxx年xx月xx日
    char time_str[12];   // 时分秒字符串：xx:xx:xx
    char week_day_str[7];//星期字符串
    char lunar_date_str[25]; // 农历日期字符串
    char lunar_Pillar_str[25]; // 四柱字符串

    char timestamp_str[25];
} Calendar_TypeDef;

// 函数声明
void Calendar_Init(void);
void Calendar_SetTime(uint16_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t min, uint8_t s);
void Calendar_ChangeTime(uint8_t unit, int8_t direction);
void Calendar_Increment(void);
void Calendar_Refresh(void);
void calculate_HeavenlyandEarthly(uint32_t timestamp,uint16_t year, uint8_t month, uint8_t day);

char* Calendar_Getdatestr(void);
char* Calendar_Gettimestr(void);
char* Calendar_Getweek_daystr(void);
char* Calendar_GetFourPillarStr(void);
uint8_t Calendar_Get_lunar_moth_leap(void);
char* Calendar_GetlunardateStr(void);
char* Calendar_Gettimestampstr(void);

extern Calendar_TypeDef calendar;

#endif
