#ifndef __ST7789_H_
#define __ST7789_H_
#include <stdint.h>
#include <stdbool.h>
#include "image.h"
#include "font.h"
#define ST7789_WIDTH 240
#define ST7789_HEIGHT 320
/**
 * @brief  将8‑8‑8 RGB转为ST7789使用的RGB565 16位颜色
 * @param  R 红色 0~255
 * @param  G 绿色 0~255
 * @param  B 蓝色 0~255
 * @retval uint16_t RGB565颜色值
 * 格式位布局：[R5][G6][B5]  16bit
 * R:保留高5位  R&0xF8  <<8   放到bit15~11
 * G:保留高6位  G&0xFC  <<3   放到bit10~5
 * B:保留高5位  B>>3         放到bit4~0
 * 按位或拼接得到16bit颜色
 */
#define fill_color(R,G,B) (((R & 0XF8) << 8)| ((G & 0XFC) << 3) | (B >> 3))
void ST7789_Init(void);
bool ST7789_Init_display(void);
bool ST7789_fill_color(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color);
bool ST7789_write_ascii(uint16_t x,uint16_t y,char ch,uint16_t color,uint16_t bg_color,const font_t *font);
bool ST7789_write_Chinese(uint16_t x,uint16_t y,char *ch,uint16_t color,uint16_t bg_color,const font_t *font);
bool ST7789_write_string(uint16_t x,uint16_t y,char *str,uint16_t color,uint16_t bg_color,const font_t *font);
bool ST7789_Draw_image(uint16_t x,uint16_t y,const image_t *image);
#endif