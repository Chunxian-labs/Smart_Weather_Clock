#ifndef __WEATHER_H__
#define __WEATHER_H__
#include <stdint.h>
#include <stdbool.h>
typedef struct
{
    char city[32];
    char location[128];
    char weather[16];
    int weather_code;
    float temperature;
}weather_info_t;

bool parse_weather_responce(const char *responce,weather_info_t *weather_info);
#endif