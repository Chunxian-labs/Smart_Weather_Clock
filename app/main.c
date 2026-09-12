#include "application.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"


int main(void)
{
    application_start();

    vTaskStartScheduler();

    /* 只有内存不足、调度器无法启动时才会到这里 */
    while (1)
    {
        ;//code should never reach here
    }
    return 0;
}
void vAssertCalled(const char *file, int line)
{
    printf("Assert Called: %s(%d)\n", file, line);
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName)
{
    printf("Stack Overflowed: %s\n", pcTaskName);
    configASSERT(0);
}

void vApplicationMallocFailedHook( void )
{
    printf("Malloc Failed\n");
    configASSERT(0);
}