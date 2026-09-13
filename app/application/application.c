#include "application.h"
#include "esp32.h"
#include "weather.h"
#include "wifi.h"
#include "ui.h"
#include "st7789.h"
#include "aht20.h"
#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
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
#define INDOOR_UPDATE_INTERVAL		SECONDS(3)
#define OUTDOOR_UPDATE_INTERVAL		MINUTES(1)
// 网络任务处理的事件
#define NET_EVT_OUTDOOR_FETCH	    (1UL << 0)
#define NET_EVT_WIFI_QUERY			(1UL << 1)
#define NET_EVT_TIME_SYNC			(1UL << 2)
#define NET_EVT_ALL					(NET_EVT_OUTDOOR_FETCH | NET_EVT_WIFI_QUERY | NET_EVT_TIME_SYNC)
// 应用/UI 任务处理的事件
#define APP_EVT_TIME_UPDATE         (1UL << 0)
#define APP_EVT_INDOOR_UPDATE	  	(1UL << 1)
#define APP_EVT_OUTDOOR_UPDATE		(1UL << 2)
#define APP_EVT_WIFI_UPDATE			(1UL << 3)
#define APP_EVT_TIME_SYNC			(1UL << 4)

#define APP_EVT_WEATHER_READY		(1UL << 5)
#define APP_EVT_WIFI_READY			(1UL << 6)
#define APP_EVT_TIME_SYNC_READY		(1UL << 7)
#define APP_EVT_ALL_EVENTS			(APP_EVT_TIME_UPDATE |\
									APP_EVT_INDOOR_UPDATE |\
									APP_EVT_OUTDOOR_UPDATE |\
									APP_EVT_WIFI_UPDATE |\
									APP_EVT_TIME_SYNC |\
									APP_EVT_WEATHER_READY |\
									APP_EVT_WIFI_READY |\
									APP_EVT_TIME_SYNC_READY)	
#define WEATHER_URL "https://api.seniverse.com/v3/weather/now.json?key=SZWlQoqIpQd5ekJQI&location=beijing&language=en&unit=c" 
static TaskHandle_t application_task_handle = NULL;
static TaskHandle_t network_task_handle = NULL;
static TimerHandle_t time_update_timer = NULL;
static TimerHandle_t indoor_update_timer = NULL;
static TimerHandle_t outdoor_update_timer = NULL;
static TimerHandle_t wifi_update_timer = NULL;
static TimerHandle_t time_sync_timer = NULL;
static QueueHandle_t weather_result_queue = NULL;
static QueueHandle_t wifi_result_queue = NULL;
static QueueHandle_t time_sync_result_queue = NULL;
static void application_init(void);
static void time_update(void);
static void indoor_update(void);
static bool outdoor_fetch(weather_info_t *weather_info);
static void outdoor_display_update(const weather_info_t *weather_info);
static bool wifi_fetch(ESP32_WIFI_Info_T *info);
static void wifi_display_update(const ESP32_WIFI_Info_T *info);
static bool time_fetch(esp_date_time_t *esp_date);
static void time_apply(const esp_date_time_t *esp_date);
static void time_update_timer_callback(TimerHandle_t xTimer)
{
	(void)xTimer;
	xTaskNotify(application_task_handle, APP_EVT_TIME_UPDATE, eSetBits);
}
static void indoor_update_timer_callback(TimerHandle_t xTimer)
{
	(void)xTimer;
	xTaskNotify(application_task_handle, APP_EVT_INDOOR_UPDATE, eSetBits);
}
static void outdoor_update_timer_callback(TimerHandle_t xTimer)
{
	(void)xTimer;
	xTaskNotify(application_task_handle, APP_EVT_OUTDOOR_UPDATE, eSetBits);
}
static void wifi_update_timer_callback(TimerHandle_t xTimer)
{
	(void)xTimer;
	xTaskNotify(application_task_handle, APP_EVT_WIFI_UPDATE, eSetBits);
}
static void time_sync_timer_callback(TimerHandle_t xTimer)
{
	(void)xTimer;
	xTaskNotify(application_task_handle, APP_EVT_TIME_SYNC, eSetBits);
}
static void network_task(void *argument)
{
	uint32_t event;
	(void)argument;
	while(1)
	{
		xTaskNotifyWait(0,NET_EVT_ALL,&event,portMAX_DELAY);
		if(event & NET_EVT_OUTDOOR_FETCH)
		{
			weather_info_t weather_info = {0};
			if(outdoor_fetch(&weather_info))
			{
				xQueueOverwrite(weather_result_queue, &weather_info);
				xTaskNotify(application_task_handle, APP_EVT_WEATHER_READY, eSetBits);
			}
		}
		if(event & NET_EVT_WIFI_QUERY)
		{
			ESP32_WIFI_Info_T wifi_info = {0};
			if(wifi_fetch(&wifi_info))
			{
				xQueueOverwrite(wifi_result_queue, &wifi_info);
				xTaskNotify(application_task_handle, APP_EVT_WIFI_READY, eSetBits);
			}
		}
		if(event & NET_EVT_TIME_SYNC)
		{
			esp_date_time_t esp_date = {0};
			if(time_fetch(&esp_date))
			{
				xQueueOverwrite(time_sync_result_queue, &esp_date);
				xTaskNotify(application_task_handle, APP_EVT_TIME_SYNC_READY, eSetBits);
			}
		}
	}
}
static void application_task(void *argument)
{
	uint32_t event;
	(void)argument;
	application_init();
	xTaskNotify(network_task_handle, NET_EVT_TIME_SYNC, eSetBits);
	xTaskNotify(application_task_handle,APP_EVT_OUTDOOR_UPDATE,eSetBits);
	while(1)
	{
		xTaskNotifyWait(0,APP_EVT_ALL_EVENTS,&event,portMAX_DELAY);
		if(event & APP_EVT_TIME_UPDATE)
		{
			time_update();
		}
		if(event & APP_EVT_INDOOR_UPDATE)
		{
			indoor_update();
		}
		if(event & APP_EVT_OUTDOOR_UPDATE)
		{
			xTaskNotify(network_task_handle, NET_EVT_OUTDOOR_FETCH, eSetBits);
		}
		if(event & APP_EVT_WEATHER_READY)
		{
			weather_info_t weather_info = {0};
			if(xQueueReceive(weather_result_queue, &weather_info, 0)== pdPASS)
			{
				outdoor_display_update(&weather_info);
			}
		}
		
		if(event & APP_EVT_WIFI_UPDATE)
		{
			xTaskNotify(network_task_handle, NET_EVT_WIFI_QUERY, eSetBits);
		}
		if(event & APP_EVT_WIFI_READY)
		{
			ESP32_WIFI_Info_T wifi_info = {0};
			if(xQueueReceive(wifi_result_queue, &wifi_info, 0)== pdPASS)
			{
				wifi_display_update(&wifi_info);
			}
		}
		if(event & APP_EVT_TIME_SYNC)
		{
			xTaskNotify(network_task_handle, NET_EVT_TIME_SYNC, eSetBits);
		}
		if(event & APP_EVT_TIME_SYNC_READY)
		{
			esp_date_time_t esp_date = {0};
			if(xQueueReceive(time_sync_result_queue, &esp_date, 0)== pdPASS)
			{
				time_apply(&esp_date);
			}
		}
	}
}
void application_start(void)
{
	BaseType_t result;
	weather_result_queue = xQueueCreate(1, sizeof(weather_info_t));
	configASSERT(weather_result_queue != NULL);
	wifi_result_queue = xQueueCreate(1, sizeof(ESP32_WIFI_Info_T));
	configASSERT(wifi_result_queue != NULL);
	time_sync_result_queue = xQueueCreate(1, sizeof(esp_date_time_t));
	configASSERT(time_sync_result_queue != NULL);

	result = xTaskCreate(application_task,"application_task",1024,NULL,1,&application_task_handle);
	configASSERT(result == pdPASS);
	result = xTaskCreate(network_task,"network_task",1024,NULL,1,&network_task_handle);
	configASSERT(result == pdPASS);

	time_update_timer = xTimerCreate("time_update", pdMS_TO_TICKS(TIME_UPDATE_INTERVAL), pdTRUE, NULL, time_update_timer_callback);
	configASSERT(time_update_timer != NULL);
	result = xTimerStart(time_update_timer, 0);
	configASSERT(result == pdPASS);
	indoor_update_timer = xTimerCreate("indoor_update", pdMS_TO_TICKS(INDOOR_UPDATE_INTERVAL), pdTRUE, NULL, indoor_update_timer_callback);
	configASSERT(indoor_update_timer != NULL);
	result = xTimerStart(indoor_update_timer, 0);
	configASSERT(result == pdPASS);
	outdoor_update_timer = xTimerCreate("outdoor_update", pdMS_TO_TICKS(OUTDOOR_UPDATE_INTERVAL), pdTRUE, NULL, outdoor_update_timer_callback);
	configASSERT(outdoor_update_timer != NULL);
	result = xTimerStart(outdoor_update_timer, 0);
	configASSERT(result == pdPASS);
	wifi_update_timer = xTimerCreate("wifi_update", pdMS_TO_TICKS(WIFI_UPDATE_INTERVAL), pdTRUE, NULL, wifi_update_timer_callback);
	configASSERT(wifi_update_timer != NULL);
	result = xTimerStart(wifi_update_timer, 0);
	configASSERT(result == pdPASS);
	time_sync_timer = xTimerCreate("time_sync", pdMS_TO_TICKS(TIME_SYNC_INTERVAL), pdTRUE, NULL, time_sync_timer_callback);
	configASSERT(time_sync_timer != NULL);
	result = xTimerStart(time_sync_timer, 0);
	configASSERT(result == pdPASS);

}
static void application_init(void)
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
}
static void time_update(void)
{
	static rtc_time_t last_time = {0};
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
/* 只访问网络、获取并解析天气；不刷新屏幕。 */
static bool outdoor_fetch(weather_info_t *weather_info)
{
	if(weather_info == NULL)
	{
		return false;
	}
	const char *weather_responce = ESP_HTTP_Get(WEATHER_URL);
	if(weather_responce == NULL)
	{
		printf("[WEATHER]http error\r\n");
		return false;
	}
	if(!parse_weather_responce(weather_responce,weather_info))
	{
		printf("[WEATHER]parse_weather_responce failed\r\n");
		return false;
	}
	printf("[WEATHER]\n %s, %s, %.2f\r\n",
		weather_info->city,weather_info->weather,weather_info->temperature);
	return true;
}
/* 只操作 UI；不访问 ESP32、不执行 HTTP。 */
static void outdoor_display_update(const weather_info_t *weather_info)
{
	static weather_info_t last_weather_info = {0};
	if(weather_info == NULL)
	{
		return;
	}
	if(last_weather_info.weather_code != weather_info->weather_code ||
		last_weather_info.temperature != weather_info->temperature)
	{
		main_page_outdoor_refresh(weather_info->temperature,
									weather_info->weather_code);
		last_weather_info = *weather_info;
	}
}
/* 只访问 ESP32；不刷新屏幕。 */
static bool wifi_fetch(ESP32_WIFI_Info_T *wifi_info)
{
	if(wifi_info == NULL)
	{
		return false;
	}
	if(!ESP_WiFi_Info_Get(wifi_info))
	{
		printf("[WIFI]ESP_WiFi_Info_Get failed\r\n");
		return false;
	}
	return true;
}
/* 只刷新 UI；不访问 ESP32。 */
static void wifi_display_update(const ESP32_WIFI_Info_T *wifi_info)
{
	static ESP32_WIFI_Info_T last_wifi_info = {0};
	if(wifi_info == NULL)
	{
		return;
	}
	if(wifi_info->connection_state == last_wifi_info.connection_state)
	{
		return;
	}
	if(wifi_info->connection_state)
	{
		main_page_wifi_refresh(true);
		printf("[WIFI]WiFi connected to %s\r\n",wifi_info->ssid);
		printf("[WIFI]\nSSID: %s, BSSID: %s, Channel: %d, RSSI: %d\r\n",
			wifi_info->ssid,wifi_info->bssid,wifi_info->channel,wifi_info->rssi);
	}
	else
	{
		main_page_wifi_refresh(false);
		printf("[WIFI]WiFi disconnected from %s\r\n",last_wifi_info.ssid);
	}
	last_wifi_info = *wifi_info;
}
/* 只访问 ESP32；以后由 network_task 调用。 */
static bool time_fetch(esp_date_time_t *esp_date)
{
	if(esp_date == NULL)
	{
		return false;
	}
	if(!ESP_Get_RealTime(esp_date))
	{
		printf("[TIME]ESP_Time_Get failed\r\n");
		return false;
	}
	return true;
}
/* 只操作 RTC；以后由 application_task 调用。 */
static void time_apply(const esp_date_time_t *esp_date)
{
	if(esp_date == NULL)
	{
		return;
	}
	if(esp_date->year < 2020)
	{
		return;
	}
	printf("[SNTP]\n %d-%d-%d %d:%d:%d %s\r\n",
		esp_date->year,esp_date->month,esp_date->day,esp_date->hour,esp_date->minute,esp_date->second,
		esp_date->weekday==1?"Monday":
		esp_date->weekday==2?"Tuesday":
		esp_date->weekday==3?"Wednesday":
		esp_date->weekday==4?"Thursday":
		esp_date->weekday==5?"Friday":
		esp_date->weekday==6?"Saturday":
		esp_date->weekday==7?"Sunday":
		"Unknown");
    rtc_time_t rtc_date = {0};

    rtc_date.year = esp_date->year;
    rtc_date.month = esp_date->month;
    rtc_date.day = esp_date->day;
    rtc_date.hour = esp_date->hour;
    rtc_date.minute = esp_date->minute;
    rtc_date.second = esp_date->second;
    rtc_date.weekday = esp_date->weekday;

    RTC_Set_Time(&rtc_date);
}
