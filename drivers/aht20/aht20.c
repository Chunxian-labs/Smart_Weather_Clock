#include "aht20.h"
#include "i2c.h"
#include "cpu_tick.h"

/*
I2C2
SCL PB10
SDA PB11
*/
static uint8_t INITIALIZATION_CMD[3] = {0xBE,0x08,0x00};
static uint8_t MEASUREMENT_CMD[3] = {0xAC,0x33,0x00};

bool AHT20_Init(void)
{
    cpu_delay_us(1000*40);
    if(!AHT20_isready())
    {
        if(!AHT20_write(INITIALIZATION_CMD,3)) return false;
    }
    return true;
}
bool AHT20_write(uint8_t data[],uint32_t len)
{
    return i2c2_write(I2C_SLAVE_ADDRESS7,data,len);
}
bool AHT20_read(uint8_t data[],uint32_t len)
{
    return i2c2_read(I2C_SLAVE_ADDRESS7,data,len);
}
bool AHT20_read_status(uint8_t *status)
{
    uint8_t cmd = CHECK_STATUS_CMD;
    if(!AHT20_write(&cmd,1)) return false;
    if(!AHT20_read(status,1)) return false;
    return true;
}
bool AHT20_isbusy(void)
{
    uint8_t status;
    if(!AHT20_read_status(&status)) return false;
    if((status&0x80)) return true;
    else return false;
}
bool AHT20_isready(void)
{
    uint8_t status;
    if(!AHT20_read_status(&status)) return false;
    if(!(status&0x08)) return false;
    else return true;
}
bool AHT20_Measurement_start(void)
{
    return AHT20_write(MEASUREMENT_CMD,3);
}
bool AHT20_Measurement_Delay(void)
{
    for(uint8_t t=0;t<200;t++)
    {
        cpu_delay_us(1000);
        if(!AHT20_isbusy()) return true;
    }
    return false;
}
bool AHT20_GetMeasuredData(float *temperature,float *humidity)
{
    uint8_t data[6] = {0};
    if(!AHT20_Measurement_Delay()) return false;
    if(!AHT20_read(data,6)) return false;
    uint32_t raw_humidity = ((uint32_t) (data[1]<<12)
     | (uint32_t) (data[2]<<4)
      | (uint32_t) ((data[3]&0xF0)>>4));
    uint32_t raw_temperature = ((uint32_t) ((data[3]&0x0F)<<16)
     | (uint32_t) (data[4]<<8)
     | (uint32_t) (data[5]));

    *humidity = (float)raw_humidity*100.0f/1024.0f/1024.0f;
    *temperature = (float)raw_temperature*200.0f/1024.0f/1024.0f-50.0f;
    return true;
}

/*
how to use:
	AHT20_Init();
	float temperature = 0.0f;
	float humidity = 0.0f;
	AHT20_Measurement_start();
	AHT20_Measurement_Delay();
	AHT20_GetMeasuredData(&temperature,&humidity);
	printf("temperature: %.2f, humidity: %.2f\r\n",temperature,humidity);
*/