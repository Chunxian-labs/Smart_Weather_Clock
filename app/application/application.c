#include "application.h"
#include "esp32.h"
#include "weather.h"
#include "wifi.h"
#include "ui.h"
#include "st7789.h"
#include "aht20.h"
#include "bsp.h"
#include "cpu_tick.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#define MILLISECONDS(x)   			(x)
#define SECONDS(x)        			(MILLISECONDS(x)*1000)
#define MINUTES(x)        			(SECONDS(x)*60)
#define HOURS(x)          			(MINUTES(x)*60)
#define DAYS(x)           			(HOURS(x)*24)
#define TIME_SYNC_INTERVAL			DAYS(1)
#define WIFI_UPDATE_INTERVAL		SECONDS(5)
#define TIME_UPDATE_INTERVAL		SECONDS(1)
#define INDOOR_UNPDATE_INTERVAL		SECONDS(3)
#define OUTDOOR_UNPDATE_INTERVAL	MINUTES(1)
#define WEATHER_URL "https://api.seniverse.com/v3/weather/now.json?key=SZWlQoqIpQd5ekJQI&location=beijing&language=en&unit=c" 
static volatile uint32_t time_sync_delay;
static volatile uint32_t wifi_update_delay;
static volatile uint32_t time_update_delay;
static volatile uint32_t indoor_update_delay;
static volatile uint32_t outdoor_update_delay;
void application_init(void)
{  
	bsp_init();
    ST7789_Init();
    ST7789_Init_display();
	AHT20_Init();
    welcome_page_display();
	wifi_init();
	wifi_page_display();
	wifi_connect();
    main_page_display_init();
    cpu_tick_set_callback(cpu_tick_callback_handler);  
}
void cpu_tick_callback_handler(void)
{
	if(time_sync_delay > 0)
	{
		time_sync_delay--;
	}
	if(wifi_update_delay > 0)
	{
		wifi_update_delay--;
	}
	if(time_update_delay > 0)
	{
		time_update_delay--;
	}
	if(indoor_update_delay > 0)
	{
		indoor_update_delay--;
	}
	if(outdoor_update_delay > 0)
	{
		outdoor_update_delay--;
	}
}
static void time_sync(void)
{
	if(time_sync_delay > 0)
	{
		return;
	}
	time_sync_delay = TIME_SYNC_INTERVAL;
	esp_date_time_t esp_date = {0};
	if(!ESP_Get_RealTime(&esp_date))
	{
		printf("[SNTP]ESP_Get_RealTime failed\r\n");
		time_sync_delay = SECONDS(5);
		return;
	}
	if(esp_date.year < 2000)
	{
		printf("[SNTP]Invalid year\r\n");
		time_sync_delay = SECONDS(5);
		return;
	}
	printf("[SNTP]\n %d-%d-%d %d:%d:%d %s\r\n",
		esp_date.year,esp_date.month,esp_date.day,esp_date.hour,esp_date.minute,esp_date.second,
		esp_date.weekday==1?"Monday":
		esp_date.weekday==2?"Tuesday":
		esp_date.weekday==3?"Wednesday":
		esp_date.weekday==4?"Thursday":
		esp_date.weekday==5?"Friday":
		esp_date.weekday==6?"Saturday":
		esp_date.weekday==7?"Sunday":
		"Unknown");
	rtc_time_t rtc_date = {0};
	rtc_date.year = esp_date.year;
	rtc_date.month = esp_date.month;
	rtc_date.day = esp_date.day;
	rtc_date.hour = esp_date.hour;
	rtc_date.minute = esp_date.minute;
	rtc_date.second = esp_date.second;
	rtc_date.weekday = esp_date.weekday;
	RTC_Set_Time(&rtc_date);

	time_update_delay = 10;

}
static void wifi_update(void)
{	
	static ESP32_WIFI_Info_T last_info = {0};
	if(wifi_update_delay > 0)
	{
		return;
	}
	wifi_update_delay = WIFI_UPDATE_INTERVAL;
	ESP32_WIFI_Info_T current_info = {0};
	if(!ESP_WiFi_Info_Get(&current_info))//解析WiFi连接状态和AP信息，ESP32返回信息，存储到current_info结构体中
	{
		printf("[WIFI]ESP_WiFi_Info_Get failed\r\n");
		return;
	}
	if(current_info.connection_state == last_info.connection_state)
	{
		return;
	}
	if(current_info.connection_state)
	{
		main_page_wifi_refresh(true);
		printf("[WIFI]WiFi connected to %s\r\n",current_info.ssid);
		printf("[WIFI]\nSSID: %s, BSSID: %s, Channel: %d, RSSI: %d\r\n",
			current_info.ssid,current_info.bssid,current_info.channel,current_info.rssi);
	}
	else
	{
		main_page_wifi_refresh(false);
		printf("[WIFI]WiFi disconnected from %s\r\n",last_info.ssid);
	}
	last_info = current_info;
}

static void time_update(void)
{
	static rtc_time_t last_time = {0};
	if(time_update_delay > 0)
	{
		return;
	}
	time_update_delay = TIME_UPDATE_INTERVAL;

	rtc_time_t current_time = {0};
	RTC_Get_Time(&current_time);
	if(current_time.year < 2020)
	{
		// RTC 尚未同步到真实时间，不刷新，避免显示复位默认的 2000/01/01
		return;
	}
	if(last_time.hour != current_time.hour ||
		last_time.minute != current_time.minute ||
		last_time.second != current_time.second)
	{
		main_page_RTC_timerefresh(&current_time);
	}
	if(last_time.year != current_time.year ||
		last_time.month != current_time.month ||
		last_time.day != current_time.day || 
		last_time.weekday != current_time.weekday)
	{
		main_page_RTC_daterefresh(&current_time);
	}
	last_time = current_time;
}
static void indoor_update(void)
{
	static float last_temperature,last_humidity;
	if(indoor_update_delay > 0)
	{
		return;
	}
	indoor_update_delay = INDOOR_UNPDATE_INTERVAL;

	if(!AHT20_Measurement_start())
	{
		printf("[AHT20]AHT20 measurement failed\r\n");
		return;
	}
	if(!AHT20_Measurement_Delay())
	{
		printf("[AHT20]AHT20 wait for measurement failed\r\n");
		return;
	}
	float temperature = 0.00f,humidity = 0.00f;
	if(!AHT20_GetMeasuredData(&temperature,&humidity))
	{
		printf("[AHT20]AHT20_Read failed\r\n");
		return;
	}
	if(last_temperature != temperature ||
		last_humidity != humidity)
	{
		main_page_indoor_refresh(temperature,humidity);
		last_temperature = temperature;
		last_humidity = humidity;
	}
	printf("[AHT20]Temperature: %.2f, Humidity: %.2f\r\n",temperature,humidity);
}
static void outdoor_update(void)
{
	static weather_info_t last_weather_info = {0};
	if(outdoor_update_delay > 0)
	{
		return;
	}
	outdoor_update_delay = OUTDOOR_UNPDATE_INTERVAL;
	weather_info_t current_weather_info = {0};
	const char *weather_responce = ESP_HTTP_Get(WEATHER_URL);
	if(weather_responce == NULL)
	{
		printf("[WEATHER]http error\r\n");
		return;
	}
	if(!parse_weather_responce(weather_responce,&current_weather_info))
	{
		printf("[WEATHER]parse_weather_responce failed\r\n");
		return;
	}
	if(last_weather_info.weather_code != current_weather_info.weather_code ||
		last_weather_info.temperature != current_weather_info.temperature)
		{
			main_page_outdoor_refresh(current_weather_info.temperature,
									current_weather_info.weather_code);
		}
	printf("[WEATHER]\n %s, %s, %.2f\r\n",
		current_weather_info.city,current_weather_info.weather,current_weather_info.temperature);
}
void application_run(void)
{
    time_sync();
    wifi_update();
    time_update();
    indoor_update();
    outdoor_update();
}