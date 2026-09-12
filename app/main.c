#include "application.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"


int main(void)
{
    application_init();
	while(1)
	{
		application_run();
	}
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