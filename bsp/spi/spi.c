#include "spi.h"
#include "stm32f4xx.h"
#include "stdbool.h"
#include "stddef.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define SPI2_DMA_TIMEOUT_MS 500U
#define SPI2_DMA_IRQ_PRIORITY 6U
#define SPI2_DMA_INTERRUPTS (DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_FE)
#define SPI2_DMA_FLAGS (DMA_FLAG_TCIF4 | DMA_FLAG_HTIF4 | DMA_FLAG_TEIF4 | DMA_FLAG_DMEIF4 | DMA_FLAG_FEIF4)
/*
TCIF4	Transfer Complete，搬完了
HIF4    Half Transfer，搬了一半
TEIF4	Transfer Error，搬错了
DMEIF4	Direct Mode Error，直接模式搬错了
FEIF4	FIFO Error，FIFO搬错了
*/
/* 初始为空，DMA 完成或错误中断释放，发送任务等待。 */
static SemaphoreHandle_t spi2_dma_semaphore = NULL;
static SemaphoreHandle_t spi2_mutex = NULL;          // 保护多任务并发
static volatile bool spi2_dma_failed = false;
static void spi2_dma_init(void)
{
    spi2_dma_semaphore = xSemaphoreCreateBinary();//返回值是 SemaphoreHandle_t 句柄
    configASSERT(spi2_dma_semaphore != NULL); //如果创建失败，就会返回NULL，进入断言
        spi2_mutex = xSemaphoreCreateMutex();             
    configASSERT(spi2_mutex != NULL);
    //关掉 SPI2 的 DMA 发送请求，确保 DMA 流初始化期间不会有触发源
    SPI_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
    //把 DMA1_Stream4 的所有寄存器恢复默认值，清掉之前可能残留的
    DMA_DeInit(DMA1_Stream4);

    DMA_InitTypeDef DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_Channel = DMA_Channel_0; //选择 DMA 通道 0。STM32F4：SPI2_TX->DMA1 Stream4 - Channel 0
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR; //外设端地址设为 SPI2 数据寄存器 DR,DMA 每次搬运都往这里写
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral; //数据搬运方向：内存->外设

    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; //外设端地址不增量，每次都写 SPI2->DR
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable; //内存端地址默认不自增，由后续配置修改

    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //外设端数据宽度：16bit
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; //内存端数据宽度：16bit
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; //DMA 工作模式：普通模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High; //DMA 流的软件优先级：高，用于 DMA 内部请求仲裁
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; //FIFO 模式：禁用

    DMA_InitStructure.DMA_Memory0BaseAddr = 0; //内存端地址默认 0，由后续配置修改
    DMA_InitStructure.DMA_BufferSize = 1; //搬运数据量默认

    DMA_Init(DMA1_Stream4, &DMA_InitStructure);
    DMA_ClearFlag(DMA1_Stream4, SPI2_DMA_FLAGS);//清掉 Stream4 的所有标志位,防止 DeInit 前残留的标志在后续使能中断时立刻误触发

    NVIC_ClearPendingIRQ(DMA1_Stream4_IRQn); //清掉 Stream4 的中断挂起标志，防止NVIC一旦使能就会立刻进 ISR
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); //中断优先级分组：4位抢占优先级，0位响应优先级

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn; //DMA1 Stream4 中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = SPI2_DMA_IRQ_PRIORITY; //抢占优先级：6
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //响应优先级：0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //使能中断
    NVIC_Init(&NVIC_InitStructure);
}
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

    spi2_dma_init();

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
/* 返回前确保 DMA 已停止，避免继续读取调用者的栈变量或已复用的字模缓冲区。
 * 禁用 DMA 流时，会先完成当前正在进行的总线传输，而不是整个数据块。
 * 仅在任务临界区内调用，确保 DMA 中断被屏蔽。 */
static void spi2_dma_stop(void)
{
    SPI_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
    DMA_ITConfig(DMA1_Stream4, SPI2_DMA_INTERRUPTS, DISABLE); //禁用中断
    DMA_Cmd(DMA1_Stream4, DISABLE);
    while (DMA_GetCmdStatus(DMA1_Stream4) != DISABLE)
    {
         /* 等待硬件确认 DMA 流已关闭，不是等待整个数据块传输完成。 */
    }
    DMA_ClearFlag(DMA1_Stream4, SPI2_DMA_FLAGS);
    NVIC_ClearPendingIRQ(DMA1_Stream4_IRQn);
}

bool spi2_write_dma_16(const uint16_t *DataAddr, uint32_t length, bool memory_increment)
{
    if (DataAddr == NULL || length == 0 || ((uintptr_t)DataAddr & 1U) != 0)
        return false;
    if (__get_IPSR() != 0)                                  /* not in ISR */
        return false;
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)  /* scheduler on */
        return false;
    if (__get_PRIMASK() != 0 || __get_BASEPRI() != 0)       /* IRQs enabled */
        return false;
    if (xSemaphoreTake(spi2_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
        return false;
    bool result = false;
    //如果上一次传输尾巴还没发完，现在启动新 DMA 会冲突
    if (!spi2_wait_not_busy())
    {
        goto done;
    }
    SPI_DataSizeConfig(SPI2, SPI_DataSize_16b);
    while (length > 0)
    {
        uint32_t chunk = length < 65535U ? length : 65535U;
        /* 清除上一次可能残留的完成信号。 */
        (void)xSemaphoreTake(spi2_dma_semaphore, 0);
        //通过临界区保护，防止配置到一半被中断打断
        taskENTER_CRITICAL();
        spi2_dma_stop();       //确保 DMA 处于干净停止状态，没有残留中断/标志
        spi2_dma_failed = false; //清上一块的失败标志，本块重新开始
        if(memory_increment)
        {
            /* 图片、字模：每发送一个像素，读取下一个地址。 */
            DMA1_Stream4->CR |= DMA_SxCR_MINC;   // 递增
        }
        else
        {
            /* 纯色：每发送一个像素，重复发送同一个地址。 */
            DMA1_Stream4->CR &= ~DMA_SxCR_MINC;  // 不递增
        }
        DMA1_Stream4 -> M0AR = (uint32_t)DataAddr;  // Memory 0 Address Register：源地址
        DMA1_Stream4 -> NDTR = chunk;      // Number of Data to Transfer：一次搬运数量

        DMA_ITConfig(DMA1_Stream4, SPI2_DMA_INTERRUPTS, ENABLE); // 先开中断
        DMA_Cmd(DMA1_Stream4, ENABLE);   //再开流
        SPI_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);  //  //最后开 SPI 的 DMA 请求源，确保 SPI2->DR 读取时有数据搬运
        taskEXIT_CRITICAL();
        //阻塞等待完成
        /*
        DMA 正常完成：ISR Give → 返回 pdTRUE。
        DMA 出错：ISR 也 Give → 返回 pdTRUE，但 spi2_dma_failed=true（下一步读）。
        超时（500ms 没完成）：返回 pdFALSE。
        */
        BaseType_t completed = xSemaphoreTake(spi2_dma_semaphore, pdMS_TO_TICKS(SPI2_DMA_TIMEOUT_MS));
        //临界区内读失败标志 + 停 DMA
        taskENTER_CRITICAL();
        bool failed = spi2_dma_failed;
        spi2_dma_stop();
        taskEXIT_CRITICAL();
        if (completed != pdTRUE)
        {
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
    taskEXIT_CRITICAL();
    if (!spi2_wait_not_busy())
    {
        result = false;
    }
    SPI_DataSizeConfig(SPI2, SPI_DataSize_8b);
    (void)xSemaphoreTake(spi2_dma_semaphore, 0);
    xSemaphoreGive(spi2_mutex);//释放互斥锁，允许其他任务使用 SPI2
    return result;
}

void DMA1_Stream4_IRQHandler(void)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    bool failed = DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_TEIF4) != RESET ||
                  DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_DMEIF4) != RESET ||
                  DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_FEIF4) != RESET;//正常情况下，DMA 传输完成时不会触发这三个错误标志位
    
    bool complete = DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_TCIF4) != RESET;
    DMA_ClearFlag(DMA1_Stream4, SPI2_DMA_FLAGS);
    if (!failed && !complete)//如果进了中断，但是即没有出错，也没有完成，说明是半传输中断，直接返回
    {
        return;
    }
    SPI_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);  //断掉 SPI 的 DMA 请求源，防止 ISR 返回后 SPI 继续要数据
    DMA_ITConfig(DMA1_Stream4, SPI2_DMA_INTERRUPTS, DISABLE); //关中断，防止重复进 ISR
    spi2_dma_failed = failed;
    if (failed)//非正常情况，DMA 传输错误，直接禁用 DMA 流，避免继续搬运数据
    {
        DMA_Cmd(DMA1_Stream4, DISABLE); //关 DMA 流本体
    }
    xSemaphoreGiveFromISR(spi2_dma_semaphore, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}
