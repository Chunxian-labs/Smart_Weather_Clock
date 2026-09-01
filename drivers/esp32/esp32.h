#ifndef __ESP32_H_
#define __ESP32_H_
#include <stdint.h>
#include <stdbool.h>
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0])) //这个是用于获取数组的元素个数的宏定义
#define ESP_AT_DEBUG   0 //是否开启调试模式
//该枚举为非显示枚举，值默认从0开始递增
typedef enum
{
    AT_ACK_NONE,
    AT_ACK_OK,
    AT_ACK_ERROR,
    AT_ACK_BUSY,
    AT_ACK_READY,
}AT_ACK_T;      //该枚举定义了AT命令的应答状态
// 结构体：把【枚举状态码】和【对应的应答字符串】绑定在一起
typedef struct 
{
    AT_ACK_T ack;  //ack为枚举类型，用于存储AT命令的应答状态，等于AT_ACK_T中的一个值
    const char *string;   /* data */
}AT_ACK_MATCH_T;
typedef struct 
{
    char ssid[64]; //wifi名称
    char bssid[18];  //MAC地址
    int channel;//信道
    int rssi;   //信号强度
    bool connection_state;
}ESP32_WIFI_Info_T;
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
}esp_date_time_t;

bool ESP32_AT_Init(void);
bool ESP_AT_WAIT_READY(uint32_t timeout);
bool ESP_Send_CMD(const char *ptr,uint32_t timeout);
const char *ESP_Get_Response(void);
bool ESP_Wifi_Init(void);
bool ESP_Wifi_Connect(const char *ssid,const char *password,const char *mac);
bool ESP_WiFi_Info_Get(ESP32_WIFI_Info_T *info);
bool WiFi_Conneted(void);
bool ESP_Get_RealTime(esp_date_time_t *data_time);
bool ESP_SNTP_Init(void);
const char *ESP_HTTP_Get(const char *url);
#endif