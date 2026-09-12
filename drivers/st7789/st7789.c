#include "st7789.h"
#include "st7789_port.h"
#include "spi.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>
#include <string.h>
#include "image.h"
#include "font.h"
void ST7789_Init(void)
{
    spi2_init();
    ST7789_Port_Init();
}
static void ST7789_Reset(void)
{
//GPIO_ResetBits(RESET_PORT, RESET_PIN);
    ST7789_Reset_Low();
    vTaskDelay(pdMS_TO_TICKS(1));
    //根据数据手册，当延时时间大于等于10us时，ST7789会自动复位，需要等待10us以上
    // GPIO_SetBits(RESET_PORT, RESET_PIN);
    ST7789_Reset_High();
    vTaskDelay(pdMS_TO_TICKS(120));
    //等待120ms之后，才可以进行唤醒操作
}
static bool ST7789_WriteRegister(uint8_t reg,uint8_t data[],uint16_t len)
{
    ST7789_CS_Low();//拉低片选信号，选中ST7789
    ST7789_DC_Low();//发送的是命令,高电平数据，低电平命令
    if(spi2_write_byte(reg) == false)
    {
        ST7789_CS_High();//拉高片选信号，取消选中ST7789
        return false;
    }
    if(spi2_wait_idle()==false)
    {
        ST7789_CS_High();//拉高片选信号，取消选中ST7789
        return false;
    }
    ST7789_DC_High();//发送的是数据,低电平命令，高电平数据
    for(uint16_t i = 0;i<len;i++)
    {
        if(spi2_write_byte(data[i]) == false)
        {
            ST7789_CS_High();//拉高片选信号，取消选中ST7789
            return false;
        }
    }
    if(spi2_wait_idle()==false)
    {
        ST7789_CS_High();//拉高片选信号，取消选中ST7789
        return false;
    }
    ST7789_CS_High();//拉高片选信号，取消选中ST7789
    return true;
}
static bool ST7789_room_isavailable(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
    if(x1 >= ST7789_WIDTH || y1 >= ST7789_HEIGHT
    || x2 >= ST7789_WIDTH || y2 >= ST7789_HEIGHT)
    {
        return false;
    }
    if(x1 > x2 || y1 > y2)
    {
        return false;
    }
    return true;
}
static bool ST7789_Set_Range_gram_pre(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
    if(!ST7789_WriteRegister(0x2A, (uint8_t[]){(x1>>8)&0xFF, x1&0xFF, (x2>>8)&0xFF, x2&0xFF}, 4))
        return false;
    if(!ST7789_WriteRegister(0x2B, (uint8_t[]){(y1>>8)&0xFF, y1&0xFF, (y2>>8)&0xFF, y2&0xFF}, 4))
        return false;
    if(!ST7789_WriteRegister(0x2C, NULL, 0))
        return false;
    return true;
}
bool ST7789_fill_color(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color)
{
    if(ST7789_room_isavailable(x1,y1,x2,y2) == false)
    {
        return false;
    }
    if(!ST7789_Set_Range_gram_pre(x1,y1,x2,y2))
    {
        return false;
    }
    ST7789_CS_Low();//拉低片选信号，选中ST7789
    ST7789_DC_High();//发送的是数据,低电平命令，高电平数据
    uint32_t total_pixels = (x2 - x1 + 1) * (y2 - y1 + 1);
    if(!spi2_write_dma_16(&color,total_pixels,false))
    {
        ST7789_CS_High();//拉高片选信号，取消选中ST7789
        return false;
    }
    ST7789_CS_High();//拉高片选信号，取消选中ST7789
    return true;
}
static void ST7789_Set_BlackLight(bool state)
{
    ST7789_Backlight_Set(state);
}
bool ST7789_Init_display(void)
{
    ST7789_Reset();
    if(!ST7789_WriteRegister(0x11, NULL, 0))   // Sleep out
        return false;
    vTaskDelay(pdMS_TO_TICKS(5));
    if(!ST7789_WriteRegister(0x36, (uint8_t[]){0x00}, 1))
        return false;
    if(!ST7789_WriteRegister(0x3A, (uint8_t[]){0x55}, 1))
        return false;
    if(!ST7789_WriteRegister(0xB2,(uint8_t[]){0x0C,0x0C,0x00,0x33,0x33}, 5))
        return false;
    if(!ST7789_WriteRegister(0xB7, (uint8_t[]){0x46}, 1))
        return false;
    if(!ST7789_WriteRegister(0xBB, (uint8_t[]){0x1B}, 1))
        return false;
    if(!ST7789_WriteRegister(0xC0, (uint8_t[]){0x2C}, 1))
        return false;
    if(!ST7789_WriteRegister(0xC2, (uint8_t[]){0x01}, 1))
        return false;
    if(!ST7789_WriteRegister(0xC4, (uint8_t[]){0x20}, 1))
        return false;
    if(!ST7789_WriteRegister(0xC6, (uint8_t[]){0x0F}, 1))
        return false;
    if(!ST7789_WriteRegister(0xD0, (uint8_t[]){0xA4,0xA1}, 2))
        return false;
    if(!ST7789_WriteRegister(0xD6, (uint8_t[]){0xA1}, 1))
        return false;
    if(!ST7789_WriteRegister(0xE0,
        (uint8_t[]){
            0xF0,0x00,0x06,0x04,0x05,0x05,0x31,
            0x44,0x48,0x36,0x12,0x12,0x2B,0x34
        }, 14))
        return false;
    if(!ST7789_WriteRegister(0xE1,
        (uint8_t[]){
            0xF0,0x0B,0x0F,0x0F,0x0D,0x26,0x31,
            0x43,0x47,0x38,0x14,0x14,0x2C,0x32
        }, 14))
        return false;
    if(!ST7789_WriteRegister(0x21, NULL, 0))
        return false;
    if(!ST7789_WriteRegister(0x29, NULL, 0))
        return false;
    if(!ST7789_fill_color(0,0,ST7789_WIDTH - 1,ST7789_HEIGHT - 1,0x0000))
        return false;
    ST7789_Set_BlackLight(true);
    return true;
}
static bool ST7789_draw_font(uint16_t x,uint16_t y,uint16_t fwidth,uint16_t fheight,const uint8_t *font_model,uint16_t color,uint16_t bg_color)
{
    if(font_model == NULL)
    {
        return false;
    }

    if(fwidth > 76 || fheight > 76)
    {
        return false;
    }
    uint16_t bytes_per_row = (fwidth + 7) / 8;

    static uint16_t buffer[76 * 76];
    uint16_t *buf_ptr = buffer;

    for(uint16_t row = 0; row < fheight; row++)
    {
        const uint8_t *row_data =
            font_model + row * bytes_per_row;

        for(uint16_t col = 0; col < fwidth; col++)
        {
            uint8_t byte = row_data[col / 8];
            uint8_t bit = 7 - (col % 8);

            uint16_t pixel_color =
                (byte & (1 << bit)) ? color : bg_color;

            *buf_ptr++ = pixel_color;
        }
    }
    if(!ST7789_Set_Range_gram_pre(x,y,x + fwidth - 1,y + fheight - 1))
    {
        return false;
    }

    ST7789_CS_Low();
    ST7789_DC_High();

    if(!spi2_write_dma_16(buffer,buf_ptr - buffer,true))
    {
        ST7789_CS_High();
        return false;
    }
    ST7789_CS_High();
    return true;
}

static const uint8_t *ASCII_Get_model(const char ch,const font_t *font)
{
    if(!ch || !font->ascii_model)
    {
        return NULL;
    }
    uint16_t bytes_per_row = (font->size/2+7)/8;//每一行需要的字节数
    uint16_t bytes_per_char = font->size*bytes_per_row;//每个字符需要的字节数
    if(font->ascii_map!=NULL)
    {
        const char *map = font->ascii_map; //创建一个叫map的指针，让它指向ascii_map字符串的开头
        while(*map != '\0')
        {
            if(*map == ch)
            {
                return font->ascii_model + (map-font->ascii_map)*bytes_per_char;
            }
            map++;
        }
        return font->ascii_model; //如果字符不在映射表中，返回第一个字符的模型地址
    }
    return font->ascii_model + (ch - font->first_char)*bytes_per_char;
    //如果字符不在映射表中，按照ascii表顺序计算模型地址
}
bool ST7789_write_ascii(uint16_t x,uint16_t y,char ch,uint16_t color,uint16_t bg_color,const font_t *font)
{
    if(ch<0x20 || ch>0x7E) return false;//如果字符不在ASCII表中，直接返回
    if(font==NULL) return false;
    //写一个字符需要的宽度和高度
    uint16_t fwidth = font->size/2;
    uint16_t fheight = font->size;
    if(ST7789_room_isavailable(x,y,x+fwidth-1,y+fheight-1) == false)
    {
        return false;
    }
    const uint8_t *model_ptr = ASCII_Get_model(ch,font);
    if(model_ptr == NULL) return false;//获取字体模型指针，指向当前字符的模型
    if(!ST7789_draw_font(x,y,fwidth,fheight,model_ptr,color,bg_color))
    {
        return false;
    }
    if(spi2_wait_idle()==false)
    {
        ST7789_CS_High();//拉高片选信号，取消选中ST7789
        return false;
    }
    ST7789_CS_High();//拉高片选信号，取消选中ST7789
    return true;
}
bool ST7789_write_Chinese(uint16_t x,uint16_t y,char *ch,uint16_t color,uint16_t bg_color,const font_t *font)
{
    if(ch==NULL) return false;
    if(font==NULL) return false;
    if(font->Chinese_model == NULL) return false;
    const Chinese_font_t *Chinese_model_ptr = font->Chinese_model;
    for(;Chinese_model_ptr->name != NULL;Chinese_model_ptr++)
    {
        if(strcmp(Chinese_model_ptr->name, ch) == 0)
        {
            break;
        }
    }
    if(Chinese_model_ptr->name == NULL) return false;//如果未找到对应的中文字体，直接返回
    uint16_t fwidth = font->size;
    uint16_t fheight = font->size;
    if(ST7789_room_isavailable(x,y,x+fwidth-1,y+fheight-1) == false)
    {
        return false;
    }
    if(!ST7789_draw_font(x,y,fwidth,fheight,Chinese_model_ptr->model,color,bg_color))
    {
        return false;
    }
    if(spi2_wait_idle()==false)
    {
        ST7789_CS_High();//拉高片选信号，取消选中ST7789
        return false;
    }
    ST7789_CS_High();
    return true;
}
// UTF-8 字符长度检测（GCC 默认以 UTF-8 存储字符串字面量）
static int utf8_char_length(const char *str)
{
    if ((*str & 0x80) == 0) return 1;      // 1 byte:  0xxxxxxx (ASCII)
    if ((*str & 0xE0) == 0xC0) return 2;   // 2 bytes: 110xxxxx
    if ((*str & 0xF0) == 0xE0) return 3;   // 3 bytes: 1110xxxx (中文!)
    if ((*str & 0xF8) == 0xF0) return 4;   // 4 bytes: 11110xxx
    return -1;                              // Invalid UTF-8
}
bool ST7789_write_string(uint16_t x,uint16_t y,char *str,uint16_t color,uint16_t bg_color,const font_t *font)
{
    while(*str != '\0')//遍历字符串，直到最后'\0'的时候停止
    {
        int len = utf8_char_length(str);
        if(len < 0)//如果不是有效的UTF-8编码，直接跳过
        {
            str++;
            continue;
        }
        else if(len == 1) //如果是ASCII字符，直接调用ST7789_write_ascii函数
        {
            if(ST7789_write_ascii(x,y,*str,color,bg_color,font) == false)
            {
                return false;
            }
            str++;
            x += font->size/2; //移动x坐标，准备绘制下一个字符
        }
        else
        {
            char utf8_char[5] = {0}; // 用于存储单个UTF-8字符，最多4字节 + 1字节终止符
            strncpy(utf8_char, str, len); // 复制当前UTF-8字符到utf8_char
            utf8_char[len] = '\0'; // 确保字符串以'\0'结尾
            if(!ST7789_write_Chinese(x,y,utf8_char,color,bg_color,font))
            {
                return false;
            }
            str += len; //移动指针，准备绘制下一个字符
            x += font->size; //移动x坐标，准备绘制下一个字符
        }
    }
    return true;
}
bool ST7789_Draw_image(uint16_t x,uint16_t y,const image_t *image)
{
    if(x>= ST7789_WIDTH || y>= ST7789_HEIGHT || image == NULL) return false;
    if(x+image->width-1 >= ST7789_WIDTH || y+image->height-1 >= ST7789_HEIGHT) return false;
    uint32_t image_pixel = image->width*image->height;//图片为16位彩色RGB图片，所以需要两字节
    if(!ST7789_Set_Range_gram_pre(x,y,x+image->width-1,y+image->height-1))
    {
        return false;
    }
    ST7789_CS_Low();
    ST7789_DC_High();
    if(!spi2_write_dma_16((const uint16_t *)image->image_data,image_pixel,true))
    {
        ST7789_CS_High();//拉高片选信号，取消选中ST7789
        return false;
    }
    ST7789_CS_High();
    return true;
}