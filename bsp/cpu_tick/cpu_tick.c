#include "cpu_tick.h"
#include "stm32f4xx.h"
#include <stddef.h>
static volatile uint64_t cpu_tick_count = 0;
static cpu_tick_callback_t cpu_tick_callback = NULL;
void cpu_tick_set_callback(cpu_tick_callback_t func)
{
    if(func != NULL)
    {
        cpu_tick_callback = func;
    }
}
//@brief 初始化SysTick定时器,1ms中断一次
//该函数，是让装载值等于1ms走过的tick数，更新到val里面，val从规定的装载值，一直向下减到0
void cpu_tick_init(void)
{
    SysTick->LOAD = TICKS_PER_MS-1U; //1s经过的时钟数为SystemCoreClock,1ms就要/1000
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
}
//@brief 获取当前时间的tick数,即从开机到现在经过的tick数
uint64_t cpu_tick_get_now(void)
{
    uint64_t current_count;
    uint64_t last_count;
    do
    {
        last_count = cpu_tick_count;
        current_count = last_count + (SysTick->LOAD - SysTick->VAL);
        //后面的SysTick->LOAD - SysTick->VAL是当前SysTick计数器的值
        /* code */
    } while (last_count != cpu_tick_count);
    
    return current_count;
    //这样可以确保count是正确的，因为该函数可能随时被SysTick_Handler中断
    //防止在count计算的过程中，cpu_tick_count被中断修改，导致count不准确
}
uint64_t cpu_tick_get_ms(void)
{
    return cpu_tick_get_now()/TICKS_PER_MS;
}
uint64_t cpu_tick_get_us(void)
{
    return cpu_tick_get_now()/TICKS_PER_US;
}
//@brief 延时delay_us微秒
void cpu_delay_us(uint32_t delay_us)
{
    uint64_t now = cpu_tick_get_now();
    while(cpu_tick_get_now() - now < (uint64_t)delay_us * TICKS_PER_US);
}
void cpu_delay_ms(uint32_t delay_ms)
{
    uint64_t now = cpu_tick_get_now();
    while(cpu_tick_get_now() - now < (uint64_t)delay_ms * TICKS_PER_MS);
}

void SysTick_Handler(void)
{
    cpu_tick_count+=TICKS_PER_MS;//每1ms，增加TICKS_PER_MS的tick数
    if(cpu_tick_callback != NULL)
    {
        cpu_tick_callback();
    }
}