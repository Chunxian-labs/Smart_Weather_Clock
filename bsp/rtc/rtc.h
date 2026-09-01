#ifndef __RTC_H_
#define __RTC_H_
#include <stdint.h>
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
}rtc_time_t;

void RTC_Config(void);
void RTC_Set_Time(const rtc_time_t *time);
void RTC_Get_Time(rtc_time_t *time);
#endif