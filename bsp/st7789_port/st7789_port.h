#ifndef __ST7789_PORT_H_
#define __ST7789_PORT_H_
#include <stdbool.h>
#define CS_PORT       GPIOE
#define CS_PIN        GPIO_Pin_2

#define RESET_PORT    GPIOE
#define RESET_PIN     GPIO_Pin_3

#define DC_PORT       GPIOE
#define DC_PIN        GPIO_Pin_4

#define LED_PORT      GPIOE
#define LED_PIN       GPIO_Pin_5
void ST7789_Port_Init(void);

void ST7789_CS_High(void);
void ST7789_CS_Low(void);

void ST7789_DC_High(void);
void ST7789_DC_Low(void);

void ST7789_Reset_High(void);
void ST7789_Reset_Low(void);

void ST7789_Backlight_Set(bool state);
#endif