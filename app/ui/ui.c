#include "image.h"
#include "st7789.h"
#include "font.h"
#include "ui.h"
#include <stddef.h>
#include <stdio.h>
static uint16_t bg_color_black = fill_color(0,0,0);

static uint16_t bg_color_time = fill_color(248,248,248);
static uint16_t bg_color_indoor = fill_color(136,217,234);
static uint16_t bg_color_outdoor = fill_color(253,238,195);
void welcome_page_display(void)
{
    ST7789_fill_color(0,0,ST7789_WIDTH-1,ST7789_HEIGHT-1,bg_color_black);
    ST7789_Draw_image(0,0,&image_welcome_page);
}
void error_page_display(void)
{
    ST7789_fill_color(0,0,ST7789_WIDTH-1,ST7789_HEIGHT-1,bg_color_black);
    ST7789_Draw_image(52,95,&image_error_page_pic);
    ST7789_write_string(35,240,"WiFi Error",fill_color(255,0,0), fill_color(255,255,255),&font32_maple_bold);
}

void wifi_page_display(void)
{
    ST7789_fill_color(0,0,ST7789_WIDTH-1,ST7789_HEIGHT-1,bg_color_black);
    ST7789_write_string(60,36,"Welcome!",fill_color(0,255,255), bg_color_black,&font32_maple_bold);
    ST7789_Draw_image(52,95,&image_wifi);
    ST7789_write_string(84,220,"WiFi",fill_color(0,255,255), bg_color_black,&font32_maple_bold);
    ST7789_write_string(32,253,"Connecting...",fill_color(255,255,0), bg_color_black,&font32_maple_bold);
}

void main_page_display_init(void)
{
    ST7789_fill_color(0,0,ST7789_WIDTH-1,ST7789_HEIGHT-1,bg_color_black);
    do
    {
        ST7789_fill_color(15,15,224,154,bg_color_time);//填充上半部分底色
        ST7789_fill_color(15,165,114,304,bg_color_indoor);//填充左下部分底色
        ST7789_fill_color(125,165,224,304,bg_color_outdoor);//填充右下部分底色
    } while (0);
    
    do//WiFi信息显示
    {
        ST7789_Draw_image(23,20,&gImage_wifi_pic);//显示wifi图标
        ST7789_write_string(100,20,"[iPhone 17 Pro]",fill_color(0,0,0), bg_color_time,&font16_maple);//WiFi名称显示
    }while(0);

    do//时间部分显示
    {
        ST7789_write_string(25,50,"--:--",fill_color(0,0,0), bg_color_time,&font76_maple_extrabold);//时间显示
        ST7789_write_string(35,125,"----/--/-- ---",fill_color(0,0,0), bg_color_time,&font24_maple_bold);//日期显示
    }while(0);
    
    do//室内环境显示
    {
        
        ST7789_write_string(17,167,"Indoor",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//室内显示
        ST7789_write_string(17,207,"T:",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//温度显示
        ST7789_write_string(41,207,"--.--",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//温度显示
        ST7789_write_string(101,207,"C",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//温度显示

        ST7789_write_string(17,247,"H:",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//湿度显示
        ST7789_write_string(41,247,"--.--",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//湿度显示
        ST7789_write_string(101,247,"%",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//湿度显示
    } while (0);
    
    do//室外环境显示
    {
        
        ST7789_write_string(127,167,"Outdoor",fill_color(0,0,0), bg_color_outdoor,&font24_maple_bold);//室外显示
        ST7789_write_string(127,207,"T:",fill_color(0,0,0), bg_color_outdoor,&font24_maple_bold);//温度显示
        ST7789_write_string(151,207,"--.--",fill_color(0,0,0), bg_color_outdoor,&font24_maple_bold);//温度显示
        ST7789_write_string(211,207,"C",fill_color(0,0,0), bg_color_outdoor,&font24_maple_bold);//温度显示

        ST7789_Draw_image(139,239,&gImage_thermometer_pic);//显示温度计图标
        ST7789_Draw_image(166,240,&gImage_sunny_pic);//显示天气图标
    }while(0);

}
void main_page_wifi_refresh(bool state)
{
    if(state == true)
    {
        ST7789_Draw_image(23,20,&gImage_wifi_pic);//显示wifi图标
        ST7789_write_string(68,20,"    [iPhone 17 Pro]",fill_color(0,0,0), bg_color_time,&font16_maple);//WiFi名称显示
    }
    else
    {
        ST7789_write_string(23,20,"  ",fill_color(0,0,0), bg_color_time,&font24_maple_bold);//WiFi名称显示
        ST7789_write_string(68,20,"[WIFI Disconnected]",fill_color(0,0,0), bg_color_time,&font16_maple);//WiFi名称显示
    }
}
void main_page_RTC_timerefresh(rtc_time_t *time)
{
    char time_str[20];
    snprintf(time_str,sizeof(time_str),"%02u%c%02u",time->hour,(time->second%2==0)?':':' ',time->minute);
    ST7789_write_string(25,50,time_str,fill_color(0,0,0), bg_color_time,&font76_maple_extrabold);//时间显示

}
void main_page_RTC_daterefresh(rtc_time_t *time)
{
    static const char *weekday_str[8] = {"","Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
    uint8_t weekday = (time->weekday>=1 && time->weekday<=7)?time->weekday:7;
    char date_str[30];
    snprintf(date_str,sizeof(date_str),"%4u/%02u/%02u %s",time->year,time->month,time->day,
    weekday_str[weekday]);
    ST7789_write_string(35,125,date_str,fill_color(0,0,0), bg_color_time,&font24_maple_bold);//日期显示
}
void main_page_indoor_refresh(float temperature,float humidity)
{
    char tem_str[5] = {'-','-','.','-','-'};
    char hum_str[5] = {'-','-','.','-','-'};
    if(temperature> -10.0f && temperature< 100.0f)
    {
        sprintf(tem_str,"%.2f",temperature);
        ST7789_write_string(41,207,tem_str,fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//温度显示
    }
    else
    {
        ST7789_write_string(41,207,"--.--",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//温度显示
    }
    if(humidity> 0.00f && humidity< 100.0f)
    {
        sprintf(hum_str,"%.2f",humidity);
        ST7789_write_string(41,247,hum_str,fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//湿度显示
    }
    else
    {
        ST7789_write_string(41,247,"--.--",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//湿度显示
    }
}

void main_page_outdoor_refresh(float temperature,const int weather_code)
{
    char tem_str[5] = {'-','-','.','-','-'};
    if(temperature> -30.0f && temperature< 100.0f)
    {
        sprintf(tem_str,"%.2f",temperature);
        ST7789_write_string(151,207,tem_str,fill_color(0,0,0), bg_color_outdoor,&font24_maple_bold);//温度显示
    }
    else
    {
        ST7789_write_string(151,207,"--.--",fill_color(0,0,0), bg_color_indoor,&font24_maple_bold);//温度显示
    }
    if(weather_code == -1)
    {
        ST7789_write_string(166,240,"  ",fill_color(0,0,0), bg_color_outdoor,&font54_maple_bold);//天气图标显示
    }
    const image_t *weather_pic = NULL;
    if(weather_code == 0 || weather_code == 2 || weather_code == 38)
    {
        weather_pic = &gImage_sunny_pic;
    }
    else if(weather_code == 1 || weather_code == 3)
    {
        weather_pic = &gImage_moon_pic;
    }
    else if(weather_code == 32 || weather_code == 33)
    {
        weather_pic = &gImage_windy_pic;
    }
    else if(weather_code == 4 || weather_code == 9)
    {
        weather_pic = &gImage_overcast_pic;
    }
    else if(weather_code == 5 || weather_code == 6 || weather_code == 7 || weather_code == 8)
    {
        weather_pic = &gImage_cloudy_pic;
    }
    else if(weather_code == 10 || weather_code == 13 || weather_code == 14 || weather_code == 15 ||
            weather_code == 16 || weather_code == 17 || weather_code == 18 || weather_code == 19)
    {
        weather_pic = &gImage_rainy_pic;
    }
    else if(weather_code == 11)
    {
        weather_pic = &gImage_rainstorm_pic;
    }
    else if(weather_code == 20 || weather_code == 21 || weather_code == 22 ||
            weather_code == 23 || weather_code == 24 || weather_code == 25)
    {
        weather_pic = &gImage_snowy_pic;
    }
    else{
        weather_pic = &gImage_sunny_pic;
    }
    if(weather_pic != NULL)
    {
        ST7789_Draw_image(166,240,weather_pic);
    }
    else
    {
        ST7789_write_string(166,240,"  ",fill_color(0,0,0), bg_color_outdoor,&font54_maple_bold);//天气图标显示
    }
}