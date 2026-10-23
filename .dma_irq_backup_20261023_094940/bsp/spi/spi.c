#include "spi.h"
#include "stm32f4xx.h"
#include "stdbool.h"
#include "stddef.h"
void spi2_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE); 
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Init(GPIOC, &GPIO_InitStructure);


    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;//时钟极性，空闲状态SCLK为低电平
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;//时钟相位，数据在哪一个边沿采样
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    //SPI2挂在在APB1总线上，APB1的时钟为42MHz，分频4后为10.5MHz,小于16.66MHz，满足ST7789的时钟要求

    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);
    SPI_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
    /*!< Enable the SPI2  */
    SPI_Cmd(SPI2, ENABLE);
}
static bool spi2_wait_txe(void)//等待数据发送完成
{
    uint32_t timeout = 0;

    while(!SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE))
    {
        timeout++;

        if(timeout > 1000000)
        {
            return false;
        }
    }

    return true;
}

static bool spi2_wait_not_busy(void)//等待SPI2空闲
{
    uint32_t timeout = 0;

    while(SPI_GetFlagStatus(SPI2, SPI_FLAG_BSY) != RESET)
    {
        timeout++;

        if(timeout > 1000000)
        {
            return false;
        }
    }

    return true;
}
bool spi2_write_byte(uint8_t data)
{
    SPI_SendData(SPI2, data);

    if(!spi2_wait_txe())
    {
        return false;
    }
    return true;
}
bool spi2_wait_idle(void)
{
    return spi2_wait_not_busy();
}
// @brief 使用DMA方式发送数据到SPI2
// @param DataAddr 数据地址
// @param length 数据长度
// @param memory_increment 是否自动增加内存地址
// @return true 成功
// @return false 失败
bool spi2_write_dma_16(const uint16_t *DataAddr,uint32_t length,bool memory_increment)
{
    if(DataAddr == NULL || length == 0)
    {
        return false;
    }
    if(!spi2_wait_not_busy())
    {
        return false;
    }
    SPI_DataSizeConfig(SPI2, SPI_DataSize_16b);//设置数据宽度为16位
    DMA_InitTypeDef DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR);//目的地址为SPI2的数据寄存器
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;//内存到外设
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设地址不变
    DMA_InitStructure.DMA_MemoryInc = 
    memory_increment ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable;//内存地址自动增加
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据宽度为16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//内存数据宽度为16位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    while(length > 0)
    {
        uint32_t chunk_size = length < 65535 ? length : 65535; // DMA传输的最大长度为65535
        DMA_InitStructure.DMA_BufferSize = chunk_size;//数据长度
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)DataAddr;//源地址为数据地址
        DMA_ClearFlag(DMA1_Stream4, DMA_FLAG_TCIF4);
        DMA_Init(DMA1_Stream4, &DMA_InitStructure);
        DMA_Cmd(DMA1_Stream4, ENABLE);
        while (DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_TCIF4) == RESET);
        DMA_Cmd(DMA1_Stream4, DISABLE);
        DMA_ClearFlag(DMA1_Stream4, DMA_FLAG_TCIF4);
        length -= chunk_size;
        if(memory_increment)
        {
            DataAddr += chunk_size;
        }
    }

    if(!spi2_wait_not_busy())
    {
        SPI_DataSizeConfig(SPI2, SPI_DataSize_8b);//恢复数据宽度为8位
        return false;
    }
    SPI_DataSizeConfig(SPI2, SPI_DataSize_8b);//恢复数据宽度为8位
    return true;
}