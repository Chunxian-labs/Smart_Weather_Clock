#ifndef __SPI_H_
#define __SPI_H_
#include <stdint.h>
#include <stdbool.h>
void spi2_init(void);
bool spi2_write_byte(uint8_t data);
bool spi2_wait_idle(void);
/* Watch in debugger; wait_ticks is elapsed RTOS time, not CPU utilization. */
typedef struct {
    uint32_t blocks_started;
    uint32_t blocks_completed;
    uint32_t errors;
    uint32_t timeouts;
    uint32_t wait_ticks;
} spi2_dma_debug_t;
extern volatile spi2_dma_debug_t spi2_dma_debug;
/* Task context only; sole LCD/UI owner; notification index 1 reserved.
 * Data remains valid until this function returns. Do not call inside a critical section. */
bool spi2_write_dma_16(const uint16_t *data,uint32_t length,bool memory_increment);
#endif