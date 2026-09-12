#include "bsp.h"
#include "rtc.h"
#include "uart.h"
#include "i2c.h"
#include "spi.h"
#include <stdio.h>
void bsp_init(void)
{
    RTC_Config();
    // cpu_tick_init();
    uart1_init();
    uart2_init();
    i2c2_init();
    spi2_init();
    printf("[SYS] Build Date: %s %s\r\n",__DATE__,__TIME__);
}