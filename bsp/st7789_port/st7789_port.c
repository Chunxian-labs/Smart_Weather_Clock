#include "st7789_port.h"
#include "stm32f4xx.h"
//初始化LCD屏幕的引脚
void ST7789_Port_Init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    //初始化LCD各个引脚
    GPIO_SetBits(GPIOE,CS_PIN | RESET_PIN | DC_PIN | LED_PIN);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

    GPIO_InitStructure.GPIO_Pin = CS_PIN;
    GPIO_Init(CS_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = RESET_PIN;
    GPIO_Init(RESET_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = DC_PIN;
    GPIO_Init(DC_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = LED_PIN;
    GPIO_Init(LED_PORT, &GPIO_InitStructure);
}
void ST7789_CS_High(void)
{
    GPIO_SetBits(CS_PORT, CS_PIN);
}

void ST7789_CS_Low(void)
{
    GPIO_ResetBits(CS_PORT, CS_PIN);
}

void ST7789_DC_High(void)
{
    GPIO_SetBits(DC_PORT, DC_PIN);
}

void ST7789_DC_Low(void)
{
    GPIO_ResetBits(DC_PORT, DC_PIN);
}

void ST7789_Reset_High(void)
{
    GPIO_SetBits(RESET_PORT, RESET_PIN);
}

void ST7789_Reset_Low(void)
{
    GPIO_ResetBits(RESET_PORT, RESET_PIN);
}

void ST7789_Backlight_Set(bool state)
{
    GPIO_WriteBit(
        LED_PORT,
        LED_PIN,
        state ? Bit_SET : Bit_RESET
    );
}