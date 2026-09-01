#ifndef __I2C_H_
#define __I2C_H_
#include <stdbool.h>
#include <stdint.h>
#define I2C_SPEED 100000
void i2c2_init(void);
bool i2c2_write(uint8_t slave_address,uint8_t data[],uint32_t len);
bool i2c2_read(uint8_t slave_address,uint8_t data[],uint32_t len);
#endif
