#ifndef __FONT_H__
#define __FONT_H__
#include <stdint.h>
typedef struct
{
    const char *name; // 字体名称
    const uint8_t *model; // 字模数据指针
}Chinese_font_t;
typedef struct
{
    uint8_t first_char; // 字体第一个字符
    uint16_t size; // 字体大小
    const char *ascii_map; // ASCII字符映射表
    const uint8_t *ascii_model; // 字模数据指针
    const Chinese_font_t *Chinese_model; // 中文字模数据指针
}font_t;

extern const font_t font16_maple;
extern const font_t font20_maple_bold;
extern const font_t font24_maple_bold;
extern const font_t font32_maple_bold;
extern const font_t font54_maple_bold;
extern const font_t font76_maple_extrabold;

#endif