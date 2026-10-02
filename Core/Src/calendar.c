#include "calendar.h"
#include "string.h"

Calendar_TypeDef calendar;
//待办：旋钮逻辑，按钮消抖--2025.12.3  03:41
#define default_time 2005,8,21,0,0,0
#define nurmal 1970
#define delta_ganzhi_day 17
#define delta_ganzhi_hour 24
#define delta_ganzhi_year 46
#define delta_weekday 4


// 月份天数表（非闰年）
static const uint8_t month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

// 天干地支表（简化版）
static const char *tiangan[] = {"甲","乙","丙","丁","戊","己","庚","辛","壬","癸"};
static const char *dizhi[] = {"子","丑","寅","卯","辰","巳","午","未","申","酉","戌","亥"};

//农历生肖名称
static const char *lunar_year_names[] = {"鼠","牛","虎","兔","龙","蛇","马","羊","猴","鸡","狗","猪"};

// 农历月份名称
static const char *lunar_month_names[] = {"正","二","三","四","五","六","七","八","九","十","冬","腊"};


// 农历日期名称
static const char *lunar_day_names[] = {
    "初一","初二","初三","初四","初五","初六","初七","初八","初九","初十",
    "十一","十二","十三","十四","十五","十六","十七","十八","十九","二十",
    "廿一","廿二","廿三","廿四","廿五","廿六","廿七","廿八","廿九","三十"
};

static const uint8_t lunar_jie_date[72][12] = {//起始小寒，1月，丑月
	  //丑寅卯辰 巳午未申 酉戌亥子
		{5,4,6,5,6,6,7,8,8,8,7,7},{6,4,6,5,6,6,7,8,8,9,8,7},//1970
		{6,4,6,5,6,6,8,8,8,9,8,8},{6,5,5,5,5,5,7,7,7,8,7,7},//1972
		{5,4,6,5,5,6,7,8,8,8,7,7},{6,4,6,5,6,6,7,8,8,9,8,7},//1974
		{6,4,6,5,6,6,8,8,8,9,8,8},{6,5,5,4,5,5,7,7,7,8,7,7},//1976
		{5,4,6,5,5,6,7,7,8,8,7,7},{6,4,6,5,6,6,7,8,8,8,8,7},//1978
		{6,4,6,5,6,6,8,8,8,9,8,8},{6,5,5,4,5,5,7,7,7,8,7,7},//1980
		{5,4,6,5,5,6,7,7,8,8,7,7},{6,4,6,5,6,6,7,8,8,8,8,7},//1982
		{6,4,6,5,6,6,8,8,8,9,8,8},{6,4,5,4,5,5,7,7,7,8,7,7},//1984
		{5,4,5,5,5,6,7,7,8,8,7,7},{5,4,6,5,6,6,7,8,8,8,8,7},//1986
		{6,4,6,5,6,6,8,8,8,9,8,7},{6,4,5,4,5,5,7,7,7,8,7,7},//1988
		{5,4,5,5,5,6,7,7,8,8,7,7},{5,4,6,5,6,6,7,8,8,8,8,7},//1990
		{6,4,6,5,6,6,7,8,8,9,8,7},{6,4,5,4,5,5,7,7,7,8,7,7},//1992
		{5,4,5,5,5,6,7,7,7,8,7,7},{5,4,6,5,6,6,7,8,8,8,7,7},//1994
		{6,4,6,5,6,6,7,8,8,9,8,7},{6,4,5,4,5,5,7,7,7,8,7,7},//1996
		{5,4,5,5,5,5,7,7,7,8,7,7},{5,4,6,5,6,6,7,8,8,8,7,7},//1998
		{6,4,6,5,6,6,7,8,8,9,8,7},{6,4,5,4,5,5,7,7,7,8,7,7},//2000
		{5,4,5,5,5,5,7,7,7,8,7,7},{5,4,6,5,6,6,7,8,8,8,7,7},//2002
		{6,4,6,5,6,6,7,8,8,9,8,7},{6,4,5,4,5,5,7,7,7,8,7,7},//2004
		{5,4,5,5,5,5,7,7,7,8,7,7},{5,4,6,5,5,6,7,7,8,8,7,7},//2006
		{6,4,6,5,6,6,7,8,8,9,8,7},{6,4,5,4,5,5,7,7,7,8,7,7},//2008
		{5,4,5,4,5,5,7,7,7,8,7,7},{5,4,6,5,5,6,7,7,8,8,7,7},//2010
		{6,4,6,5,6,6,7,8,8,8,8,7},{6,4,5,4,5,5,7,7,7,8,7,7},//2012
		{5,4,5,4,5,5,7,7,7,8,7,7},{5,4,6,5,5,6,7,7,8,8,7,7},//2014
		{6,4,6,5,6,6,7,8,8,8,8,7},{6,4,5,4,5,5,7,7,7,8,7,7},//2016
		{5,3,5,4,5,5,7,7,7,8,7,7},{5,4,5,5,5,6,7,7,8,8,7,7},//2018
		{5,4,6,5,6,6,7,8,8,8,8,7},{6,4,5,4,5,5,6,7,7,8,7,7},//2020
		{5,3,5,4,5,5,7,7,7,8,7,7},{5,4,5,5,5,6,7,7,7,8,7,7},//2022
		{5,4,6,5,6,6,7,8,8,8,8,7},{6,4,5,4,5,5,6,7,7,8,7,6},//2024
		{5,3,5,4,5,5,7,7,7,8,7,7},{5,4,5,5,5,5,7,7,7,8,7,7},//2026
		{5,4,6,5,6,6,7,8,8,8,7,7},{6,4,5,4,5,5,6,7,7,8,7,6},//2028
		{5,3,5,4,5,5,7,7,7,8,7,7},{5,4,5,5,5,5,7,7,7,8,7,7},//2030
		{5,4,6,5,6,6,7,8,8,8,7,7},{6,4,5,4,5,5,6,7,7,8,7,6},//2032
		{5,3,5,4,5,5,7,7,7,8,7,7},{5,4,5,5,5,5,7,7,7,8,7,7},//2034
		{5,4,6,5,5,6,7,7,8,8,7,7},{6,4,5,4,5,5,6,7,7,8,7,6},//2036
		{5,3,5,4,5,5,7,7,7,8,7,7},{5,4,5,5,5,5,7,7,7,8,7,7},//2038
		{5,4,6,5,5,6,7,7,8,8,7,7},{6,4,5,4,5,5,6,7,7,8,7,6}//2040
};

//农历月份起始日期表：月份，日期.....（13个月26个格，如没闰月，最后一月为0）+闰月月份
static const uint8_t lunar_month_day[71][27] = {
		{2,17,3,18,4,17,5,16,6,15,7,14,8,13,9,12,10,11,11,10,12,9,1,8,0,0,0},//农历1969
		{2,6,3,8,4,6,5,5,6,4,7,3,8,2,9,1,9,30,10,30,11,29,12,28,0,0,0},//农历1970
		{1,27,2,25,3,27,4,25,5,24,6,23,7,22,8,21,9,19,10,19,11,18,12,18,1,16,5},//农历1971
		{2,15,3,15,4,14,5,13,6,11,7,11,8,9,9,8,10,7,11,6,12,6,1,4,0,0,0},//农历1972
		{2,3,3,5,4,3,5,3,6,1,6,30,7,30,8,28,9,26,10,26,11,25,12,24,0,0,0},//农历1973
		{1,23,2,22,3,24,4,22,5,22,6,20,7,19,8,18,9,16,10,15,11,14,12,14,1,12,4},//农历1974
		{2,11,3,13,4,12,5,11,6,10,7,9,8,7,9,6,10,5,11,3,12,3,1,1,0,0,0},//农历1975
		{1,31,3,1,3,31,4,29,5,29,6,27,7,27,8,25,9,24,10,23,11,21,12,21,1,19,8},//农历1976
		{2,18,3,20,4,18,5,18,6,17,7,16,8,15,9,13,10,13,11,11,12,11,1,9,0,0,0},//农历1977
		{2,7,3,9,4,7,5,7,6,6,7,5,8,4,9,3,10,2,11,1,11,30,12,30,0,0,0},//农历1978
		{1,28,2,27,3,28,4,26,5,26,6,24,7,24,8,23,9,21,10,21,11,20,12,19,1,18,6},//农历1979
		{2,16,3,17,4,15,5,14,6,13,7,12,8,11,9,9,10,9,11,8,12,7,1,6,0,0,0},//农历1980
		{2,5,3,6,4,5,5,4,6,2,7,2,7,31,8,29,9,28,10,28,11,26,12,26,0,0,0},//农历1981
		{1,25,2,24,3,25,4,24,5,23,6,21,7,21,8,19,9,17,10,17,11,15,12,15,1,14,4},//农历1982
		{2,13,3,15,4,13,5,13,6,11,7,10,8,9,9,7,10,6,11,5,12,4,1,3,0,0,0},//农历1983
		{2,2,3,3,4,1,5,1,5,31,6,29,7,28,8,27,9,25,10,24,11,23,12,22,1,21,10},//农历1984
		{2,20,3,21,4,20,5,20,6,18,7,18,8,16,9,15,10,14,11,12,12,12,1,10,0,0,0},//农历1985
		{2,9,3,10,4,9,5,9,6,7,7,7,8,6,9,4,10,4,11,2,12,2,12,31,0,0,0},//农历1986
		{1,29,2,28,3,29,4,28,5,27,6,26,7,26,8,24,9,23,10,23,11,21,12,21,1,19,6},//农历1987
		{2,17,3,18,4,16,5,16,6,14,7,14,8,12,9,11,10,11,11,9,12,9,1,8,0,0,0},//农历1988
		{2,6,3,8,4,6,5,5,6,4,7,3,8,2,8,31,9,30,10,29,11,28,12,28,0,0,0},//农历1989
		{1,27,2,25,3,27,4,25,5,24,6,23,7,22,8,20,9,19,10,18,11,17,12,17,1,16,5},//农历1990
		{2,15,3,16,4,15,5,14,6,12,7,12,8,10,9,8,10,8,11,6,12,6,1,5,0,0,0},//农历1991
		{2,4,3,4,4,3,5,3,6,1,6,30,7,30,8,28,9,26,10,26,11,24,12,24,0,0,0},//农历1992
		{1,23,2,21,3,23,4,22,5,21,6,20,7,19,8,18,9,16,10,15,11,14,12,13,1,12,3},//农历1993
		{2,10,3,12,4,11,5,11,6,9,7,9,8,7,9,6,10,5,11,3,12,3,1,1,0,0,0},//农历1994
		{1,31,3,1,3,31,4,30,5,29,6,28,7,27,8,26,9,25,10,24,11,22,12,22,1,20,8},//农历1995
		{2,19,3,19,4,18,5,17,6,16,7,16,8,14,9,13,10,12,11,11,12,11,1,9,0,0,0},//农历1996
		{2,7,3,9,4,7,5,7,6,5,7,5,8,3,9,2,10,2,10,31,11,30,12,30,0,0,0},//农历1997
		{1,28,2,27,3,28,4,26,5,26,6,24,7,23,8,22,9,21,10,20,11,19,12,19,1,17,5},//农历1998
		{2,16,3,18,4,16,5,15,6,14,7,13,8,11,9,10,10,9,11,8,12,8,1,7,0,0,0},//农历1999
		{2,5,3,6,4,5,5,4,6,2,7,2,7,31,8,29,9,28,10,27,11,26,12,26,0,0,0},//农历2000
		{1,24,2,23,3,25,4,23,5,23,6,21,7,21,8,19,9,17,10,17,11,15,12,15,1,13,4},//农历2001
		{2,12,3,14,4,13,5,12,6,11,7,10,8,9,9,7,10,6,11,5,12,4,1,3,0,0,0},//农历2002
		{2,1,3,3,4,2,5,1,5,31,6,30,7,29,8,28,9,26,10,25,11,24,12,23,0,0,0},//农历2003
		{1,22,2,20,3,21,4,19,5,19,6,18,7,17,8,16,9,14,10,14,11,12,12,12,1,10,2},//农历2004
		{2,9,3,10,4,9,5,8,6,7,7,6,8,5,9,4,10,3,11,2,12,1,12,31,0,0,0},//农历2005
		{1,29,2,28,3,29,4,28,5,27,6,26,7,25,8,24,9,22,10,22,11,21,12,20,1,19,7},//农历2006
		{2,18,3,19,4,17,5,17,6,15,7,14,8,13,9,11,10,11,11,10,12,10,1,8,0,0,0},//农历2007
		{2,7,3,8,4,6,5,5,6,4,7,3,8,1,8,31,9,29,10,29,11,28,12,27,0,0,0},//农历2008
		{1,26,2,25,3,27,4,25,5,24,6,23,7,22,8,20,9,19,10,18,11,17,12,16,1,15,5},//农历2009
		{2,14,3,16,4,14,5,14,6,12,7,12,8,10,9,8,10,8,11,6,12,6,1,4,0,0,0},//农历2010
		{2,3,3,5,4,3,5,3,6,2,7,1,7,31,8,29,9,27,10,27,11,25,12,25,0,0,0},//农历2011
		{1,23,2,22,3,22,4,21,5,21,6,19,7,19,8,17,9,16,10,15,11,14,12,13,1,12,4},//农历2012
		{2,10,3,12,4,10,5,10,6,8,7,8,8,7,9,5,10,5,11,3,12,3,1,1,0,0,0},//农历2013
		{1,31,3,1,3,31,4,29,5,29,6,27,7,27,8,25,9,24,10,24,11,22,12,22,1,20,9},//农历2014
		{2,19,3,20,4,19,5,18,6,16,7,16,8,14,9,13,10,13,11,12,12,11,1,10,0,0,0},//农历2015
		{2,8,3,9,4,7,5,7,6,5,7,4,8,3,9,1,10,1,10,31,11,29,12,29,0,0,0},//农历2016
		{1,28,2,26,3,28,4,26,5,26,6,24,7,23,8,22,9,20,10,20,11,18,12,18,1,17,6},//农历2017
		{2,16,3,17,4,16,5,15,6,14,7,13,8,11,9,10,10,9,11,8,12,7,1,6,0,0,0},//农历2018
		{2,5,3,7,4,5,5,5,6,3,7,3,8,1,8,30,9,29,10,28,11,26,12,26,0,0,0},//农历2019
		{1,25,2,23,3,24,4,23,5,23,6,21,7,21,8,19,9,17,10,17,11,15,12,15,1,13,4},//农历2020
		{2,12,3,13,4,12,5,12,6,10,7,10,8,8,9,7,10,6,11,5,12,4,1,3,0,0,0},//农历2021
		{2,1,3,3,4,1,5,1,5,30,6,29,7,29,8,27,9,26,10,25,11,24,12,23,0,0,0},//农历2022
		{1,22,2,20,3,22,4,20,5,19,6,18,7,18,8,16,9,15,10,15,11,13,12,13,1,11,2},//农历2023
		{2,10,3,10,4,9,5,8,6,6,7,6,8,4,9,3,10,3,11,1,12,1,12,31,0,0,0},//农历2024
		{1,29,2,28,3,29,4,28,5,27,6,25,7,25,8,23,9,22,10,21,11,20,12,20,1,19,6},//农历2025
		{2,17,3,19,4,17,5,17,6,15,7,14,8,13,9,11,10,10,11,9,12,9,1,8,0,0,0},//农历2026
		{2,6,3,8,4,7,5,6,6,5,7,4,8,2,9,1,9,30,10,29,11,28,12,28,0,0,0},//农历2027
		{1,26,2,25,3,26,4,25,5,24,6,23,7,22,8,20,9,19,10,18,11,16,12,16,1,15,5},//农历2028
		{2,13,3,15,4,14,5,13,6,12,7,11,8,10,9,8,10,8,11,6,12,5,1,4,0,0,0},//农历2029
		{2,3,3,4,4,3,5,2,6,1,7,1,7,30,8,29,9,27,10,27,11,25,12,25,0,0,0},//农历2030
		{1,23,2,21,3,23,4,22,5,21,6,20,7,19,8,18,9,17,10,16,11,15,12,14,1,13,3},//农历2031
		{2,11,3,12,4,10,5,9,6,8,7,7,8,6,9,5,10,4,11,3,12,3,1,1,0,0,0},//农历2032
		{1,31,3,1,3,31,4,29,5,28,6,27,7,26,8,25,9,23,10,23,11,22,12,22,1,20,11},//农历2033
		{2,19,3,20,4,19,5,18,6,16,7,16,8,14,9,13,10,12,11,11,12,11,1,9,0,0,0},//农历2034
		{2,8,3,10,4,8,5,8,6,6,7,5,8,4,9,2,10,1,10,31,11,30,12,29,0,0,0},//农历2035
		{1,28,2,27,3,28,4,26,5,26,6,24,7,23,8,22,9,20,10,19,11,18,12,17,1,16,6},//农历2036
		{2,15,3,17,4,16,5,15,6,14,7,13,8,11,9,10,10,9,11,7,12,7,1,5,0,0,0},//农历2037
		{2,4,3,6,4,5,5,4,6,3,7,2,8,1,8,30,9,29,10,28,11,26,12,26,0,0,0},//农历2038
		{1,24,2,23,3,25,4,23,5,23,6,22,7,21,8,20,9,18,10,18,11,16,12,16,1,14,5}//农历2039
};

// 检查闰年
static uint8_t is_leap_year(uint16_t year) {
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

// 获取月份天数
static uint8_t get_month_days(uint16_t year, uint8_t month) {
    if(month == 2 && is_leap_year(year))
        return 29;
    return month_days[month-1];
}

//时间转时间戳（简化版）
static uint32_t date_to_timestamp(uint16_t year, uint8_t month, uint8_t day,uint8_t hour, uint8_t minute, uint8_t second)
{
    uint32_t days = 0;
    uint16_t y;
    uint8_t m;

    // 计算1970年到指定年份的天数
    for(y = 1970; y < year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }

    // 计算月份天数
    for(m = 1; m < month; m++) {
        days += get_month_days(year, m);
    }

    // 加上日期
    days += day - 1;

    // 转换为秒数
    return days * 86400UL + hour * 3600UL + minute * 60UL + second;
}


static int compare_same_day(uint16_t y1,uint8_t m1,uint8_t d1,uint16_t y2,uint8_t m2,uint8_t d2)
{
	if(y1<y2)return -1;
	if(y1>y2)return 1;
	if(m1<m2)return -1;
	if(m1>m2)return 1;
	if(d1<d2)return -1;
	if(d1>d2)return 1;
	else return 0;
}

static uint8_t get_lunar_month(uint16_t y, uint8_t m, uint8_t d, uint8_t* leap)
{
    uint16_t y_index = y - 1969;
    uint16_t y_yang=y;
    uint8_t m_index=0;
    uint8_t m_count = 0;

    *leap = 0;

    // 检查是否在农历新年之前
    if(compare_same_day(y,m, d, y_yang,lunar_month_day[y_index][0], lunar_month_day[y_index][1]) == -1)
    {
        y_index--;
        y_yang--;
    }
    m_index = 2;
	calendar.lunar_month_start_timestamp=date_to_timestamp(y_yang,lunar_month_day[y_index][0], lunar_month_day[y_index][1],0,0,0);
    while(m_index<26)
    {
    	if (lunar_month_day[y_index][m_index] == 0)break;
    	if((y_index+1969!=y)&&lunar_month_day[y_index][m_index]==1)
		{
    		y_yang+=1;
		}
    	if(compare_same_day(y,m, d, y_yang,lunar_month_day[y_index][m_index], lunar_month_day[y_index][m_index+1]) == -1)
    	{
    		return m_count;
    	}
    	else
    	{
    		calendar.lunar_month_start_timestamp=date_to_timestamp(y_yang,lunar_month_day[y_index][m_index], lunar_month_day[y_index][m_index+1],0,0,0);
    	}
    	if((m_index/2)!=lunar_month_day[y_index][26] || lunar_month_day[y_index][26]==0)
    	{
    		*leap = 0;
    		m_count++;
    	}
    	else *leap=lunar_month_day[y_index][26];
    	m_index+=2;
    }
    return 11;  // 默认返回最后一个月份
}

static uint8_t get_lunar_day(uint16_t y, uint8_t m, uint8_t d, uint8_t lm,uint8_t leap)
{
	uint32_t delta=calendar.timestamp-calendar.lunar_month_start_timestamp;
	return delta/86400;
	//return 0;
}




// 时间戳转时间
static void timestamp_to_date(uint32_t timestamp, uint16_t *year, uint8_t *month, uint8_t *day,
                             uint8_t *hour, uint8_t *minute, uint8_t *second) {
    uint32_t days = timestamp / 86400UL;
    uint32_t seconds_in_day = timestamp % 86400UL;

    *hour = seconds_in_day / 3600;
    *minute = (seconds_in_day % 3600) / 60;
    *second = seconds_in_day % 60;

    // 计算年份
    *year = 1970;
    while(days >= (is_leap_year(*year) ? 366 : 365)) {
        days -= is_leap_year(*year) ? 366 : 365;
        (*year)++;
    }

    // 计算月份
    *month = 1;
    while(days >= get_month_days(*year, *month)) {
        days -= get_month_days(*year, *month);
        (*month)++;
    }

    // 计算日期
    *day = days + 1;
}

// 计算星期（简化版 - 使用1970-1-1是周四的基准）
static uint8_t calculate_weekday(uint32_t timestamp) {
    return (timestamp / 86400UL + delta_weekday) % 7; // 1970-1-1是周四
}

void calculate_HeavenlyandEarthly(uint32_t timestamp,uint16_t year, uint8_t month, uint8_t day)
    {
    	//乙酉 甲申 丁丑 庚子
    	//计算日干支
    	uint16_t D=calendar.timestamp/86400;
        uint8_t day_index = (delta_ganzhi_day + D) % 60;
        calendar.lunar_Heavenly_day=day_index%10;
        calendar.lunar_Earthly_day=day_index%12;

        //计算时干支
        uint32_t H = (calendar.timestamp+3600)/7200;// 7200秒 = 2小时（一个时辰）
        uint8_t hour_index = (delta_ganzhi_hour + H) % 60;
        calendar.lunar_Heavenly_hour = hour_index % 10;
        calendar.lunar_Earthly_hour = hour_index % 12;

        //计算年月干支
        uint8_t y_index=calendar.year-1969;
        uint16_t year_diff = calendar.year-nurmal+delta_ganzhi_year;
        if(compare_same_day(year,month,day,year,2,lunar_jie_date[y_index][1])==-1)
        {
        	year_diff-=1;//如果在立春之前，沿用上一年的
        }
        uint8_t m_index=calendar.month-1;
        calendar.lunar_Earthly_month = m_index+1;
        if(compare_same_day(year,month,day,year,month,lunar_jie_date[y_index][m_index])==-1)
        {
        	calendar.lunar_Earthly_month--;
        }
        if(calendar.lunar_Earthly_month<0)calendar.lunar_Earthly_month+=12;
        if(calendar.lunar_Earthly_month==12)calendar.lunar_Earthly_month-=12;
        //calendar.lunar_Heavenly_month = ((year_diff % 5)*2+calendar.lunar_Earthly_month)%10;//处理不了子月0的情况，子月偏移量应该是12而非0
        //月天干序号=(2×年干序号+(月支序号+10)mod12+2)mod10   AI给的
        calendar.lunar_Heavenly_month =(2*year_diff+(calendar.lunar_Earthly_month+10)%12 + 2)%10;
        //自己改正
        //uint8_t ear_mon=calendar.lunar_Earthly_month;
        //if(ear_mon==0)ear_mon+=12;
        //calendar.lunar_Heavenly_month = ((year_diff % 5)*2+ear_mon)%10;
        if(compare_same_day(year,month,day,year,2,lunar_jie_date[y_index][1])==-1)
        {
        	year_diff+=1;//取消立春的影响
        }
        if(compare_same_day(year,month,day,year,lunar_month_day[y_index][0],lunar_month_day[y_index][1])==-1)
        {
        	year_diff-=1;//如果在春节前，沿用上一年
        }
        calendar.lunar_Heavenly_year = year_diff % 10;
        calendar.lunar_Earthly_year = year_diff % 12;

        calendar.lunar_month = get_lunar_month(year,month,day,&calendar.lunar_moth_leap);

        //计算日期初几
        calendar.lunar_day=get_lunar_day(year,month,day,calendar.lunar_month,calendar.lunar_moth_leap);
		//calendar.lunar_day=5;
    }

// 初始化日历
void Calendar_Init(void) {
    // 设置默认时间：2005年8月21日0时0分0秒
    Calendar_SetTime(default_time);
    //乙酉 甲申 丁丑 庚子
}

// 设置时间
void Calendar_SetTime(uint16_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t min, uint8_t s) {
    calendar.timestamp = date_to_timestamp(y, m, d, h, min, s);
    calendar.refresh_flag=1;
    Calendar_Refresh();

}

// 时间改变函数
void Calendar_ChangeTime(uint8_t unit, int8_t direction) {
    uint32_t delta = 0;

    switch(unit) {
        case 1: // 年
            delta = direction * 31536000UL; // 365天
            break;
        case 2: // 月
            delta = direction * 2592000UL; // 30天
            break;
        case 3: // 日
            delta = direction * 86400UL;
            break;
        case 4: // 时
            delta = direction * 3600UL;
            break;
        case 5: // 分
            delta = direction * 60UL;
            break;
        case 6: // 秒
            delta = direction;
            break;
    }

    calendar.timestamp += delta;
    calendar.refresh_flag=1;
    Calendar_Refresh();
}

// 时间自增函数（每秒调用）
void Calendar_Increment(void) {
    calendar.timestamp++;
    Calendar_Refresh();
}

// 刷新所有时间变量
void Calendar_Refresh(void) {
    // 更新时间变量
    timestamp_to_date(calendar.timestamp, &calendar.year, &calendar.month, &calendar.day,
                     &calendar.hour, &calendar.minute, &calendar.second);
    if(calendar.refresh_flag==0 && calendar.minute!=0)
    	return;//判断是否需要强制刷新或者处于整时间段，如果不处于就不刷新，低功耗节省计算资源
    calendar.refresh_flag=0;
    // 计算星期
    calendar.week_day = calculate_weekday(calendar.timestamp);


    calculate_HeavenlyandEarthly(calendar.timestamp,calendar.year,calendar.month,calendar.day);

}

char* Calendar_Getdatestr(void)
{
	// 格式化日期字符串：xxxx年xx月xx
    snprintf(calendar.date_str, sizeof(calendar.date_str),
             "%04d年%02d月%02d",
             calendar.year, calendar.month, calendar.day);
	//return date_str;   // 年月日字符串：xxxx年xx月xx日
    return calendar.date_str;
}

char* Calendar_Gettimestr(void)
{
    // 格式化时间字符串：xx:xx:xx（24小时制）
    snprintf(calendar.time_str, sizeof(calendar.time_str),
             "%02d:%02d:%02d",
             calendar.hour, calendar.minute, calendar.second);
	return calendar.time_str;   // 时分秒字符串：xx:xx:xx
}

char* Calendar_Getweek_daystr(void)
{
	switch(calendar.week_day)
	{
	case 0:strcpy(calendar.week_day_str, "周日");break;
	case 1:strcpy(calendar.week_day_str, "周一");break;
	case 2:strcpy(calendar.week_day_str, "周二");break;
	case 3:strcpy(calendar.week_day_str, "周三");break;
	case 4:strcpy(calendar.week_day_str, "周四");break;
	case 5:strcpy(calendar.week_day_str, "周五");break;
	case 6:strcpy(calendar.week_day_str, "周六");break;
	}
	return calendar.week_day_str;
}

char* Calendar_GetFourPillarStr(void)
{
	sprintf(calendar.lunar_Pillar_str, "%s%s %s%s %s%s %s%s",
			tiangan[calendar.lunar_Heavenly_year]  , dizhi[calendar.lunar_Earthly_year]  ,
			tiangan[calendar.lunar_Heavenly_month] , dizhi[calendar.lunar_Earthly_month] ,
			tiangan[calendar.lunar_Heavenly_day]   , dizhi[calendar.lunar_Earthly_day]   ,
			tiangan[calendar.lunar_Heavenly_hour]  , dizhi[calendar.lunar_Earthly_hour]  );
    return calendar.lunar_Pillar_str;
}

uint8_t Calendar_Get_lunar_moth_leap(void)
{
	return calendar.lunar_moth_leap;
}

char* Calendar_GetlunardateStr(void)
{
	sprintf(calendar.lunar_date_str, "%s%s%s年%s月%s",
			tiangan[calendar.lunar_Heavenly_year]  ,
			dizhi[calendar.lunar_Earthly_year]  ,
			lunar_year_names[calendar.lunar_Earthly_year],
			lunar_month_names[calendar.lunar_month],
			lunar_day_names[calendar.lunar_day]);

	return calendar.lunar_date_str;
}


char* Calendar_Gettimestampstr(void)
{
	sprintf(calendar.timestamp_str,"%u",calendar.lunar_month_start_timestamp/86400);
	return calendar.timestamp_str;
}

