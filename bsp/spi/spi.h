#ifndef __SPI_H_
#define __SPI_H_
#include <stdint.h>
#include <stdbool.h>
void spi2_init(void);
bool spi2_write_byte(uint8_t data);
bool spi2_wait_idle(void);
#endif