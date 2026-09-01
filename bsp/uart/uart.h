#ifndef __UART_H_
#define __UART_H_
#include <stdint.h>
#include <stdbool.h>
typedef void(*USART_Callback_t)(uint8_t data);
void uart1_init(void);
void uart2_init(void);
bool uart2_write_byte(uint8_t data);
bool uart2_read_byte(uint8_t *data,uint32_t timeout_ms);
void USART1_CallBack_Set(USART_Callback_t func);
void USART2_CallBack_Set(USART_Callback_t func);
void uart2_rx_clear(void);
#endif