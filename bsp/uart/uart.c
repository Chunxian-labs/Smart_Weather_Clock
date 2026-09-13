#include "uart.h"
#include "stm32f4xx.h"
#include "ring_buffer.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>
#include <stdbool.h>
//USART1-TX PA9
//USART1-RX PA10
static USART_Callback_t USART1_Callback_func = NULL;
static USART_Callback_t USART2_Callback_func = NULL;
static ring_buffer_t uart1_rx_buffer;
static ring_buffer_t uart2_rx_buffer;
static volatile uint32_t uart2_rx_overflow_count = 0;
//USART1-中断服务函数
void USART1_CallBack_Set(USART_Callback_t func)
{
    if(func != NULL)
    {
        USART1_Callback_func = func;
    }
}
//USART2-中断服务函数
void USART2_CallBack_Set(USART_Callback_t func)
{
    if(func != NULL)
    {
        USART2_Callback_func = func;
    }
}
//USART1-初始化，用于stm32主控与PC通信
void uart1_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure;
    USART_StructInit(&USART_InitStructure);
  /* USARTx configured as follows:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
//USART2-初始化，用于stm32主控与ESP32通信
void uart2_init(void)
{
    ring_buffer_init(&uart2_rx_buffer);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure;
    USART_StructInit(&USART_InitStructure);
  /* USARTx configured as follows:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
//USART1-写入数据，将数据发送到PC
bool uart1_write_byte(uint8_t data)
{
    TickType_t start = xTaskGetTickCount();
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET)
    {
        if(xTaskGetTickCount() - start >= pdMS_TO_TICKS(100))
        {
            return false;
        }
    }
    USART_SendData(USART1, (uint16_t)data);
    return true;
}
//USART1-读取数据，从uart1_rx_buffer中读取数据
bool uart1_read_byte(uint8_t *data,uint32_t timeout_ms)
{
    if(data == NULL)
        return false;
    TickType_t start = xTaskGetTickCount();
    while(ring_buffer_is_empty(&uart1_rx_buffer))
    {
        if(xTaskGetTickCount() - start >= pdMS_TO_TICKS(timeout_ms))
        {
            return false;
        }
    }
    return ring_buffer_read(&uart1_rx_buffer,data);
}
//USART2-写入数据，将数据发送到ESP32
bool uart2_write_byte(uint8_t data)
{
    TickType_t start = xTaskGetTickCount();
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET)
    {
        if(xTaskGetTickCount() - start >= pdMS_TO_TICKS(100))
        {
            return false;
        }
    }
    USART_SendData(USART2, (uint16_t)data);
    return true;
}
//USART2-读取数据，从uart2_rx_buffer中读取数据
bool uart2_read_byte(uint8_t *data,uint32_t timeout_ms)
{
    if(data == NULL)
        return false;
    TickType_t start = xTaskGetTickCount();
    while(ring_buffer_is_empty(&uart2_rx_buffer))
    {
        if(xTaskGetTickCount() - start >= pdMS_TO_TICKS(timeout_ms))
        {
            return false;
        }
    }
    return ring_buffer_read(&uart2_rx_buffer,data);
}
void uart2_rx_clear(void)
{
    ring_buffer_clear(&uart2_rx_buffer);
}
//USART1-中断服务函数，用于接收PC发送的数据，数据存入uart1_rx_buffer
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
            uint8_t data = (uint8_t)USART_ReceiveData(USART1);
            ring_buffer_write(&uart1_rx_buffer,data);
    }
}
//USART2-中断服务函数，用于接收ESP32发送的数据，数据存入uart2_rx_buffer
void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t data = (uint8_t)USART_ReceiveData(USART2);
        if(!ring_buffer_write(&uart2_rx_buffer,data))
        {
            uart2_rx_overflow_count++;
        }
    }
}
//重定义printf函数,通过USART1串口打印
int _write(int file ,char *ptr, int len)
{
    (void)file;
    for(int i=0;i<len;i++)
    {
        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET);
        USART_SendData(USART1, (uint16_t)(*ptr++));
    }
    return len;
}