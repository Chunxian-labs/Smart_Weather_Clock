#include "esp32.h"
#include "uart.h"
#include "stddef.h"
#include "cpu_tick.h"
#include <string.h>
#include <stdio.h>
/*
USART2_RX  PA3
USART2_TX  PA2
*/
// static const 常量数组，查找表(Lookup Table)，用于存储AT命令的应答状态码和对应的应答字符串
static const AT_ACK_MATCH_T at_ack_matches[] = 
{
    {AT_ACK_NONE,""},             //第一个结构体成员
    {AT_ACK_OK,"OK\r\n"},         //第二个结构体成员
    {AT_ACK_ERROR,"ERROR\r\n"},   //第三个结构体成员
    {AT_ACK_BUSY,"busy p…\r\n"},  //第四个结构体成员
    {AT_ACK_READY,"ready\r\n"},   //第五个结构体成员
};
static char rxbuff[1024];//缓冲数组，用于接收ESP32的应答字符串

//@brief 主控芯片通过USART2向ESP32发送数据
static bool ESP_AT_USART_Write(const char *data)
{
    while(data && *data)
    //data是指针变量，存的是内存地址；data == NULL 代表指针无效，没有指向任何内存地址，该方式可以进行空指针检查，*data代表数据，以ascii码形式呈现
    //只有当*data != '\0'时，才会进入循环，否则会跳出循环,'\0'表示字符串结束,即真正意义上的0
    {
        if(!uart2_write_byte((uint8_t)*data++)) return false;
    }
    if(!uart2_write_byte('\r')) return false;
    if(!uart2_write_byte('\n')) return false;
    return true;
}
//@brief 等待ESP32的引导完成
static bool ESP_AT_WAIT_BOOT(uint32_t timeout)
{
    for(uint32_t t=0;t<timeout;t+=100)
    {
        if(ESP_Send_CMD("AT",100))
        {
            return true;
        }
    }
    return false;
}
//@brief 初始化ESP32的AT端口
bool ESP32_AT_Init(void)
{
    if(!ESP_AT_WAIT_BOOT(3000)) return false;
    if(!ESP_Send_CMD("AT",100)) return false;
    if(!ESP_Send_CMD("AT+RESTORE",2000)) return false;
    if(!ESP_AT_WAIT_READY(5000)) return false;
    return true;
}

//@brief 匹配ESP32的应答字符串，返回对应的枚举状态码
static AT_ACK_T match_internal_ack(const char *str)
{
    for (uint32_t i=0;i<ARRAY_SIZE(at_ack_matches);i++)
    {
        if(strcmp(str,at_ack_matches[i].string) == 0)
        {
            return at_ack_matches[i].ack;//如果匹配上了对应的应答字符串，就返回对应的枚举状态码
        }
    }
    return AT_ACK_NONE;
    //如果遍历过程中匹配上了对应的应答字符串，就返回对应的枚举状态码
    //如果没有匹配上，就返回AT_ACK_NONE，表示未知状态
}
//@brief 等待ESP32的应答，返回对应的枚举状态码
static AT_ACK_T ESP_AT_USART_Wait_Response(uint32_t timeout)
{
    uint32_t rxlen = 0;
    const char *line = rxbuff;
    rxbuff[0] = '\0';
    uint64_t start = cpu_tick_get_ms();//获取当前时间，毫秒级的超时时间计算
    while(rxlen<sizeof(rxbuff)-1)
    {
        uint64_t elapsed = cpu_tick_get_ms()-start;//计算当前时间到开始时间的毫秒数，用于超时判断
        if(elapsed >= timeout)
        {
            return AT_ACK_NONE;
        }
        uint32_t remaining = timeout - (uint32_t)elapsed;
        //用于ESP回复数据的逻辑判断
        // rxbuff[rxlen++] = USART_ReceiveData(USART2);
        if(!uart2_read_byte((uint8_t *)&rxbuff[rxlen],remaining))
        {
            return AT_ACK_NONE;
        }
        rxlen++;
        if(rxbuff[rxlen-1] == '\n')
        {
            rxbuff[rxlen] = '\0';
            AT_ACK_T ack = match_internal_ack(line);
            if(ack != AT_ACK_NONE)
            {
                return ack;
            }
            line = rxbuff+rxlen;
        }
    }
    return AT_ACK_NONE;
    //如果超时时间到了，还没有匹配上对应的应答字符串，就返回AT_ACK_NONE，表示超时
    //如果没有超时，但是没有匹配上枚举状态码，就返回AT_ACK_NONE，表示未知状态
    //如果匹配上了枚举状态码，就返回对应的枚举状态码
}
bool ESP_AT_WAIT_READY(uint32_t timeout)
{
    if(ESP_AT_USART_Wait_Response(timeout) == AT_ACK_READY)
    {
        return true;
    }
    return false;
}
bool ESP_Send_CMD(const char *ptr,uint32_t timeout)
{
#if ESP_AT_DEBUG
    printf("Command :%s\r\n",ptr);
#endif
    uart2_rx_clear();
    if(!ESP_AT_USART_Write(ptr)) return false;
    AT_ACK_T ack = ESP_AT_USART_Wait_Response(timeout);
#if ESP_AT_DEBUG
    printf("Response :\r\n%s",rxbuff);
#endif
    if(ack == AT_ACK_OK)
    {
        return true;
    }
    return false;
}

const char *ESP_Get_Response(void)
{
    return rxbuff;
}

bool ESP_Wifi_Init(void)
{
    return ESP_Send_CMD("AT+CWMODE=1",2000);
}
//@brief 连接WiFi网络
/*
ssid：WiFi名称字符串指针，传入 WiFi 热点名字，例如 "MyHomeWiFi"
password：WiFi密码字符串指针，例如 "12345678"
mac：可选，指定要连接 AP 的 BSSID（MAC 地址），可以传NULL表示不指定 MAC，只按 ssid 连接
*/
bool ESP_Wifi_Connect(const char *ssid,const char *password,const char *mac)
{
    //保护：WiFi 名、密码指针不能是空指针，为空直接返回连接失败
    if(ssid==NULL || password==NULL)
    {
        return false;
    }
    char cmd[128];//cmd[128]：局部栈上字符缓冲区，用来拼装AT指令文本。
    int len = snprintf(cmd,sizeof(cmd),"AT+CWJAP=\"%s\",\"%s\"",ssid,password);
    if(mac != NULL)
    {
        snprintf(cmd+len,sizeof(cmd)-len,",\"%s\"",mac);
    }
    return ESP_Send_CMD(cmd,5000);
}
static bool parse_cwstate_responce(const char *responce,ESP32_WIFI_Info_T *wifi_info)
{   
    // AT+CWSTATE?
    // +CWSTATE:2,"iPhone 17Pro"
    // OK
    int wifi_state;
    responce = strstr(responce,"+CWSTATE:");
    //如果找到指针直接指向"+CWSTATE:"
    //strcmp**完整字符串相等判断**0 相等，非 0 不等
    //strstr**查找是否包含某段子串**找到返回指针，NULL 没找到
    if(responce == NULL)
    return false;
    /*
    1. **第一个参数**：内存里的字符串缓冲区（`char*`，必须以`\0`结尾），数据源。
    2. **第二个参数**：匹配 / 解析规则（格式字符串）。
    3. **后面若干参数**：传**变量的地址**，解析出来的数据会写到这些地址对应的变量。
    */
    if(sscanf(responce,"%*[^:]:%d,\"%63[^\"]\"", &wifi_state, wifi_info->ssid) != 2)
    return false;

    wifi_info->connection_state = (wifi_state==2);
    return true;
}
static bool parse_cwjap_responce(const char *responce,ESP32_WIFI_Info_T *wifi_info)
{
// AT+CWJAP?
// +CWJAP:"iPhone 17Pro","ba:21:89:6f:b8:c4",2,-45,0,1,3,0,1
// OK
    responce = strstr(responce,"+CWJAP:");
    if(responce == NULL)
    return false;
    if(sscanf(responce,"%*[^:]:\"%63[^\"]\",\"%18[^\"]\",%d,%d", 
        wifi_info->ssid, 
        wifi_info->bssid,
        &wifi_info->channel,
        &wifi_info->rssi) != 4)
    return false;

    return true;
}
bool ESP_WiFi_Info_Get(ESP32_WIFI_Info_T *info)
{
    if(!ESP_Send_CMD("AT+CWSTATE?",2000))//查询WiFi连接状态
        return false;
    if(!parse_cwstate_responce(ESP_Get_Response(),info))//解析WiFi连接状态
        return false;
    if(info == NULL)
        return false;
    if(info->connection_state == true)
    {
        if(!ESP_Send_CMD("AT+CWJAP?",2000))//查询与 ESP32-C3 Station 连接的AP信息
            return false;
        if(!parse_cwjap_responce(ESP_Get_Response(),info))//解析AP信息
            return false;
    }
    return true;
}
bool WiFi_Conneted(void)
{
    ESP32_WIFI_Info_T info;
    if(ESP_WiFi_Info_Get(&info))
    {
        return info.connection_state;
    }
    return false;
}
bool ESP_SNTP_Init(void)
{
    if(!ESP_Send_CMD("AT+CIPSNTPCFG=1,8",2000))
        return false;
    return true;
}
static uint8_t month_str_to_num(const char *month)
{
    const char *month_str[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    for(uint8_t i = 0;i<12;i++)
    {
        if(strcmp(month,month_str[i]) == 0)
        {
            return i+1;
        }
    }
    return 0;
}
static uint8_t weekday_str_to_num(const char *weekday)
{
    const char *weekday_str[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
    for(uint8_t i = 0;i<7;i++)
    {
        if(strcmp(weekday,weekday_str[i]) == 0)
        {
            return i+1;
        }
    }
    return 0;
}
static bool parse_CIPSNTPCFG_responce(const char *responce,esp_date_time_t *data_time)
{
    // AT+CIPSNTPTIME?
    // +CIPSNTPTIME:Sun Jul 27 14:07:19 2025
    // OK
    responce = strstr(responce,"+CIPSNTPTIME:");
    if(responce == NULL)
        return false;
    char weekday_str[4] = {0};
    char month_str[4] = {0};
    // 注意：newlib-nano 的 sscanf 对 %hhu/%hu 支持不全，这里统一用 %d 解析成 int，再强转回小类型
    int day = 0, hour = 0, minute = 0, second = 0, year = 0;
    if(sscanf(responce,"+CIPSNTPTIME:%3s %3s %d %d:%d:%d %d",
        weekday_str,month_str,&day,&hour,&minute,&second,&year) != 7)
        return false;
    data_time->day     = (uint8_t)day;
    data_time->hour    = (uint8_t)hour;
    data_time->minute  = (uint8_t)minute;
    data_time->second  = (uint8_t)second;
    data_time->year    = (uint16_t)year;
    data_time->weekday = weekday_str_to_num(weekday_str);
    data_time->month   = month_str_to_num(month_str);
    return true;
}
bool ESP_Get_RealTime(esp_date_time_t *data_time)
{
    if(data_time == NULL)
        return false;
    if(!ESP_Send_CMD("AT+CIPSNTPTIME?",2000))
        return false;
    if(!parse_CIPSNTPCFG_responce(ESP_Get_Response(),data_time))
        return false;
    return true;
}
const char *ESP_HTTP_Get(const char *url)
{
    char *txbuff = rxbuff;
    snprintf(txbuff,sizeof(rxbuff),"AT+HTTPCLIENT=2,1,\"%s\",,,2",url);
    bool ret = ESP_Send_CMD(txbuff,5000);
    return ret ? ESP_Get_Response() : NULL;
}