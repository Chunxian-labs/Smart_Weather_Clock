#ifndef __UI_H_
#define __UI_H_
#include "rtc.h"
void welcome_page_display(void);
void error_page_display(void);
void wifi_page_display(void);
void main_page_display_init(void);
void main_page_indoor_refresh(float temperature,float humidity);
void main_page_outdoor_refresh(float temperature,const int weather_code);
void main_page_wifi_refresh(bool state);
void main_page_RTC_timerefresh(rtc_time_t *time);
void main_page_RTC_daterefresh(rtc_time_t *time);
#endif