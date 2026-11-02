#ifndef __SPI_H_
#define __SPI_H_
#include <stdint.h>
#include <stdbool.h>
void spi2_init(void);
bool spi2_write_byte(uint8_t data);
bool spi2_wait_idle(void);

/* 仅由 LCD/UI 任务调用，通过二值信号量等待 DMA 完成。
 * 数据必须保持有效直到函数返回，禁止在中断或临界区中调用。 */
bool spi2_write_dma_16(const uint16_t *data,uint32_t length,bool memory_increment);
#endif