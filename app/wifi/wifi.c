#include "wifi.h"
#include "esp32.h"
#include "cpu_tick.h"
#include "uart.h"
#include "ui.h"
#include <stddef.h>
#include <stdio.h>
void wifi_init(void)
{
    	if(!ESP32_AT_Init())
	{
		printf("[AT]ESP32_AT_Init failed\r\n");
		goto err;
	}
	printf("[AT]ESP32_AT_Init success\r\n");
		if(!ESP_Send_CMD("AT+RESTORE",1000))
	{
		printf("[AT]Restore failed\r\n");
		goto err;
	}
	cpu_delay_us(5000*1000);
	if(!ESP_Wifi_Init())
	{
		printf("[WIFI]ESP_Wifi_Init failed\r\n");
		goto err;
	}
	printf("[WIFI]ESP_Wifi_Init success\r\n");

	return;
	
err:
	error_page_display();
	while(1)
	{
		printf("Error\r\n");
		cpu_delay_us(1000*1000);
	}
}
void wifi_connect(void)
{
	if(!ESP_Wifi_Connect("iPhone 17Pro","123456677",NULL))
	{
		printf("[WIFI]ESP_Wifi_Connect failed\r\n");
		goto err;
	}
	printf("[WIFI]ESP_Wifi_Connect success\r\n");
	if(!ESP_SNTP_Init())
	{
		printf("[SNTP]ESP_SNTP_Init failed\r\n");
		goto err;
	}
	printf("[SNTP]ESP_SNTP_Init success\r\n");

    return;

err:
	error_page_display();
	while(1)
	{
		printf("Error\r\n");
		cpu_delay_us(1000*1000);
	}
}