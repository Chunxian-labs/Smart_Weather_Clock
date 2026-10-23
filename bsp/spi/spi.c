#include "spi.h"
#include "stm32f4xx.h"
#include "stdbool.h"
#include "stddef.h"
#include "FreeRTOS.h"
#include "task.h"

/* Index 0 belongs to application events. Index 1 is reserved for SPI2 DMA. */
#define SPI2_DMA_NOTIFY_INDEX 1U
#define SPI2_DMA_TIMEOUT_MS 500U
#define SPI2_DMA_IRQ_PRIORITY 6U
#define SPI2_DMA_INTERRUPTS (DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_FE)
#define SPI2_DMA_FLAGS (DMA_FLAG_TCIF4 | DMA_FLAG_HTIF4 | DMA_FLAG_TEIF4 | DMA_FLAG_DMEIF4 | DMA_FLAG_FEIF4)
#if configTASK_NOTIFICATION_ARRAY_ENTRIES <= SPI2_DMA_NOTIFY_INDEX
#error "SPI2 DMA requires at least two task notification entries"
#endif

static TaskHandle_t volatile spi2_dma_waiter = NULL;
static volatile bool spi2_dma_failed = false;
volatile spi2_dma_debug_t spi2_dma_debug = {0};

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
    /* Enable requests only after the DMA stream and waiter are ready. */
    SPI_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
    DMA_Cmd(DMA1_Stream4, DISABLE);
    DMA_ITConfig(DMA1_Stream4, SPI2_DMA_INTERRUPTS, DISABLE);
    DMA_ClearFlag(DMA1_Stream4, SPI2_DMA_FLAGS);
    NVIC_ClearPendingIRQ(DMA1_Stream4_IRQn);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = DMA1_Stream4_IRQn;
    /* Numerically >= FreeRTOS max-syscall priority (currently 5). */
    nvic.NVIC_IRQChannelPreemptionPriority = SPI2_DMA_IRQ_PRIORITY;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
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
/* Stop before returning: callers may pass stack variables or reuse a glyph buffer.
 * Disabling a stream finishes its current bus beat, not the full DMA block.
 * Call only inside a task critical section, with the DMA IRQ masked. */
static void spi2_dma_stop(void)
{
    SPI_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
    DMA_ITConfig(DMA1_Stream4, SPI2_DMA_INTERRUPTS, DISABLE);
    DMA_Cmd(DMA1_Stream4, DISABLE);
    while (DMA_GetCmdStatus(DMA1_Stream4) != DISABLE)
    {
        /* Brief hardware shutdown handshake; not the transfer-completion wait. */
    }
    DMA_ClearFlag(DMA1_Stream4, SPI2_DMA_FLAGS);
    NVIC_ClearPendingIRQ(DMA1_Stream4_IRQn);
}

/* Synchronous API, scheduler-blocking wait. The UI task owns SPI2 and LCD CS. */
bool spi2_write_dma_16(const uint16_t *DataAddr, uint32_t length, bool memory_increment)
{
    if (DataAddr == NULL || length == 0 || ((uintptr_t)DataAddr & 1U) != 0 ||
        __get_IPSR() != 0 || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING ||
        __get_PRIMASK() != 0 || __get_BASEPRI() != 0)
    {
        return false;
    }
    /* Serial LCD ownership is required at the higher UI layer. */
    taskENTER_CRITICAL();
    if (spi2_dma_waiter != NULL)
    {
        taskEXIT_CRITICAL();
        return false;
    }
    spi2_dma_waiter = xTaskGetCurrentTaskHandle();
    taskEXIT_CRITICAL();

    bool result = false;
    if (!spi2_wait_not_busy())
    {
        goto done;
    }
    SPI_DataSizeConfig(SPI2, SPI_DataSize_16b);
    DMA_InitTypeDef dma;
    DMA_StructInit(&dma);
    dma.DMA_Channel = DMA_Channel_0;
    dma.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR;
    dma.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = memory_increment ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_FIFOMode = DMA_FIFOMode_Disable;

    while (length > 0)
    {
        uint32_t chunk = length < 65535U ? length : 65535U;
        dma.DMA_BufferSize = chunk;
        dma.DMA_Memory0BaseAddr = (uint32_t)DataAddr;
        /* A notification that arrives before Take remains pending. */
        (void)ulTaskNotifyTakeIndexed(SPI2_DMA_NOTIFY_INDEX, pdTRUE, 0);
        taskENTER_CRITICAL();
        spi2_dma_stop();
        spi2_dma_failed = false;
        DMA_Init(DMA1_Stream4, &dma);
        DMA_ITConfig(DMA1_Stream4, SPI2_DMA_INTERRUPTS, ENABLE);
        ++spi2_dma_debug.blocks_started;
        DMA_Cmd(DMA1_Stream4, ENABLE);
        SPI_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
        taskEXIT_CRITICAL();

        TickType_t before_wait = xTaskGetTickCount();
        uint32_t notified = ulTaskNotifyTakeIndexed(SPI2_DMA_NOTIFY_INDEX, pdTRUE,
                                                   pdMS_TO_TICKS(SPI2_DMA_TIMEOUT_MS));
        spi2_dma_debug.wait_ticks += xTaskGetTickCount() - before_wait;
        taskENTER_CRITICAL();
        bool failed = spi2_dma_failed;
        spi2_dma_stop();
        taskEXIT_CRITICAL();
        if (notified == 0)
        {
            ++spi2_dma_debug.timeouts;
            goto done;
        }
        if (failed)
        {
            goto done;
        }
        length -= chunk;
        if (memory_increment)
        {
            DataAddr += chunk;
        }
    }
    result = true;

done:
    taskENTER_CRITICAL();
    spi2_dma_stop();
    spi2_dma_waiter = NULL;
    taskEXIT_CRITICAL();
    /* TC means memory->SPI DR is complete; BSY covers the final bits on SCK.
     * Keep this tiny tail wait before restoring 8-bit mode / releasing LCD CS. */
    if (!spi2_wait_not_busy())
    {
        result = false;
    }
    SPI_DataSizeConfig(SPI2, SPI_DataSize_8b);
    /* Also remove a completion racing with a timeout, for the next call. */
    (void)ulTaskNotifyTakeIndexed(SPI2_DMA_NOTIFY_INDEX, pdTRUE, 0);
    return result;
}

void DMA1_Stream4_IRQHandler(void)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    bool failed = DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_TEIF4) != RESET ||
                  DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_DMEIF4) != RESET ||
                  DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_FEIF4) != RESET;
    bool complete = DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_TCIF4) != RESET;
    DMA_ClearFlag(DMA1_Stream4, SPI2_DMA_FLAGS);
    if (!failed && !complete)
    {
        return;
    }
    SPI_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
    DMA_ITConfig(DMA1_Stream4, SPI2_DMA_INTERRUPTS, DISABLE);
    spi2_dma_failed = failed;
    if (failed)
    {
        ++spi2_dma_debug.errors;
        DMA_Cmd(DMA1_Stream4, DISABLE);
    }
    else
    {
        ++spi2_dma_debug.blocks_completed;
    }
    TaskHandle_t waiter = spi2_dma_waiter;
    if (waiter != NULL)
    {
        vTaskNotifyGiveIndexedFromISR(waiter, SPI2_DMA_NOTIFY_INDEX,
                                     &higher_priority_task_woken);
    }
    portYIELD_FROM_ISR(higher_priority_task_woken);
}
