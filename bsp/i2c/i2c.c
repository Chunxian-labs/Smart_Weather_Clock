#include "i2c.h"
#include "stm32f4xx.h"
#include "stddef.h"
void i2c2_init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOB, &GPIO_InitStructure);


    I2C_InitTypeDef I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0X00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;

    /* I2C1 Peripheral Enable */
    I2C_Cmd(I2C2, ENABLE);
    /* Apply I2C1 configuration after enabling it */
    I2C_Init(I2C2, &I2C_InitStructure);
}
static bool Check_Event_Delay(uint32_t event)
{
    uint32_t start = 0;
    while(!I2C_CheckEvent(I2C2, event))
    {
        start++;
        if(start > 1000000)
        {
            start = 0;
            return false;
        }
    }
    return true;
}
bool i2c2_write(uint8_t slave_address,uint8_t data[],uint32_t len)
{
    if(data == NULL || len == 0)
    {
        return false;
    }
    I2C_GenerateSTART(I2C2, ENABLE);
    if(!Check_Event_Delay(I2C_EVENT_MASTER_MODE_SELECT))
    {
        I2C_GenerateSTOP(I2C2, ENABLE);
        return false;
    } 
    I2C_Send7bitAddress(I2C2, slave_address, I2C_Direction_Transmitter);
    if(!Check_Event_Delay(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        I2C_GenerateSTOP(I2C2, ENABLE);
        return false;
    }
    for(uint32_t i = 0;i<len;i++)
    {
        I2C_SendData(I2C2, data[i]);
        if(!Check_Event_Delay(I2C_EVENT_MASTER_BYTE_TRANSMITTING)) 
        {
            I2C_GenerateSTOP(I2C2, ENABLE);
            return false;
        }
    }
    I2C_GenerateSTOP(I2C2, ENABLE);
    return true;
}
bool i2c2_read(uint8_t slave_address,uint8_t data[],uint32_t len)
{
    if(data == NULL || len == 0)
    {
        return false;
    }
    I2C_AcknowledgeConfig(I2C2, ENABLE);
    I2C_GenerateSTART(I2C2, ENABLE);
    if(!Check_Event_Delay(I2C_EVENT_MASTER_MODE_SELECT))
    {
        I2C_GenerateSTOP(I2C2, ENABLE);
        return false;
    } 
    I2C_Send7bitAddress(I2C2, slave_address, I2C_Direction_Receiver);
    if(!Check_Event_Delay(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    {
        I2C_GenerateSTOP(I2C2, ENABLE);
        return false;
    }
    for(uint32_t i = 0;i<len;i++)
    {
        if(i==(len-1))
        {
            I2C_AcknowledgeConfig(I2C2, DISABLE);
        }
        if(!Check_Event_Delay(I2C_EVENT_MASTER_BYTE_RECEIVED))
        {
            I2C_GenerateSTOP(I2C2, ENABLE);
            return false;
        }
         data[i] = I2C_ReceiveData(I2C2);
    }
    I2C_GenerateSTOP(I2C2, ENABLE);
    return true;
}