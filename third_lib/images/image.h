#ifndef __IMAGE_H__
#define __IMAGE_H__
#include <stdint.h>
typedef struct
{
    uint16_t width; // 图片宽度
    uint16_t height; // 图片高度
    const uint8_t *image_data; // 图片数据指针
}image_t;

extern const image_t image_welcome_page;
extern const image_t image_error_page_pic;
extern const image_t image_wifi;

extern const image_t gImage_thermometer_pic;
extern const image_t gImage_moon_pic;
extern const image_t gImage_sunny_pic;
extern const image_t gImage_windy_pic;
extern const image_t gImage_cloudy_pic;
extern const image_t gImage_snowy_pic;
extern const image_t gImage_rainy_pic;
extern const image_t gImage_overcast_pic;
extern const image_t gImage_rainstorm_pic;
extern const image_t gImage_wifi_pic;
#endif