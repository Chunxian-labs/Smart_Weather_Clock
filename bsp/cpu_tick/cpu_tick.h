#ifndef __CPU_TICK_H_
#define __CPU_TICK_H_
#include <stdint.h>
#define TICKS_PER_MS  (SystemCoreClock / 1000U)
#define TICKS_PER_US  (SystemCoreClock / 1000U / 1000U)

typedef void (*cpu_tick_callback_t)(void);
void cpu_tick_set_callback(cpu_tick_callback_t func);
void cpu_tick_init(void);
void cpu_delay_us(uint32_t delay_us);
void cpu_delay_ms(uint32_t delay_ms);
uint64_t cpu_tick_get_ms(void);
uint64_t cpu_tick_get_us(void);
uint64_t cpu_tick_get_now(void);

#endif