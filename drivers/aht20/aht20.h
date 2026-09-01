#ifndef __AHT20_H_
#define __AHT20_H_
#include <stdint.h>
#include <stdbool.h>
#define I2C_SLAVE_ADDRESS7 0x70
#define CHECK_STATUS_CMD 0x71
bool AHT20_Init(void);
bool AHT20_write(uint8_t data[],uint32_t len);
bool AHT20_read(uint8_t data[],uint32_t len);
bool AHT20_read_status(uint8_t *status);
bool AHT20_isbusy(void);
bool AHT20_isready(void);
bool AHT20_Measurement_start(void);
bool AHT20_Measurement_Delay(void);
bool AHT20_GetMeasuredData(float *temperature,float *humidity);
#endif