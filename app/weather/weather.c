#include "weather.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
bool parse_weather_responce(const char *responce,weather_info_t *weather_info)
{
    responce = strstr(responce,"\"results\":");
    if(responce == NULL)
        return false;
    const char *location_responce = strstr(responce,"\"location\":");
    if(location_responce == NULL)
        return false;
    const char *location_name_responce = strstr(location_responce,"\"name\":");
    if(location_name_responce != NULL)
    {
        sscanf(location_name_responce,"\"name\": \"%31[^\"]\"",weather_info->city);
    }
    const char *location_path_responce = strstr(location_responce,"\"path\":");
    if(location_path_responce != NULL)
    {
        sscanf(location_path_responce,"\"path\":\" %127[^\"]\"",weather_info->location);
    }
    const char *now_responce = strstr(responce,"\"now\":");
    if(now_responce == NULL)
        return false;
    const char *now_text_responce = strstr(now_responce,"\"text\":");
    if(now_text_responce != NULL)
    {
        sscanf(now_text_responce,"\"text\": \"%15[^\"]\"",weather_info->weather);
    }
    const char *now_code_responce = strstr(now_responce,"\"code\":");
    if(now_code_responce != NULL)
    {
        sscanf(now_code_responce,"\"code\": \"%d\"",&weather_info->weather_code);
    }
    const char *now_temperature_responce = strstr(now_responce,"\"temperature\":");
    char temperature_str[16] = {0};
    if(now_temperature_responce != NULL)
    {
        if(sscanf(now_temperature_responce,"\"temperature\": \"%15[^\"]\"",temperature_str) == 1)
        {
            weather_info->temperature = atof(temperature_str);
            //atof 是 ASCII to Float 的缩写，用于将字符串转换为浮点数
        }
    }
    
    return true;
}