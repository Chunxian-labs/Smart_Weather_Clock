#include "rtc.h"
#include "cpu_tick.h"
#include "stm32f4xx.h"
#include <string.h>
void RTC_Config(void)
{
  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  /* Allow access to RTC */
  PWR_BackupAccessCmd(ENABLE);

  RTC_InitTypeDef RTC_InitStructure;
  RTC_StructInit(&RTC_InitStructure);

  /* 优先使用 LSE 32.768kHz 外部晶振 */
  RCC_LSEConfig(RCC_LSE_ON);
  uint32_t start = (uint32_t)cpu_tick_get_ms();
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
    if((uint32_t)cpu_tick_get_ms() - start > 5000)
    {
      /* LSE 一直没就绪（板子可能没焊 32.768k 晶振），退回 LSI 内部低速时钟 */
      RCC_LSEConfig(RCC_LSE_OFF);
      RCC_LSICmd(ENABLE);
      while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
      RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
      /* LSI 约 32kHz：32000/((124+1)*(255+1)) = 1Hz */
      RTC_InitStructure.RTC_AsynchPrediv = 124;
      RTC_InitStructure.RTC_SynchPrediv = 255;
      goto rtc_init;
    }
  }
  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

rtc_init:
  RTC_Init(&RTC_InitStructure);

  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);

  /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();
}
static void RTC_Set_Time_Once(const rtc_time_t *time)
{
    RTC_DateTypeDef RTC_DateStructure;
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateStructInit(&RTC_DateStructure);
    RTC_TimeStructInit(&RTC_TimeStructure);

    RTC_DateStructure.RTC_Year = time->year - 2000;
    RTC_DateStructure.RTC_Month = time->month;
    RTC_DateStructure.RTC_Date = time->day;
    RTC_DateStructure.RTC_WeekDay = (time->weekday >= 1 && time->weekday <= 7) ? time->weekday : 1;
    RTC_TimeStructure.RTC_Hours = time->hour;
    RTC_TimeStructure.RTC_Minutes = time->minute;
    RTC_TimeStructure.RTC_Seconds = time->second;

    /* 真正写入 RTC 硬件寄存器（之前漏掉了这两句，时间根本没写进去） */
    RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);
    RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);
   }
void RTC_Set_Time(const rtc_time_t *time)
{
    rtc_time_t rtime;
    do
    {
        RTC_Set_Time_Once(time);
        RTC_Get_Time(&rtime);   // 真正读回校验，而不是再写一遍
    } while(time->second != rtime.second);
}
static void RTC_Get_Time_Once(rtc_time_t *time)
{
    RTC_DateTypeDef RTC_DateStructure;
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateStructInit(&RTC_DateStructure);
    RTC_TimeStructInit(&RTC_TimeStructure);
    RTC_GetDate(RTC_Format_BIN,&RTC_DateStructure);
    RTC_GetTime(RTC_Format_BIN,&RTC_TimeStructure);

    time->year = 2000+RTC_DateStructure.RTC_Year;
    time->month = RTC_DateStructure.RTC_Month;
    time->day = RTC_DateStructure.RTC_Date;
    time->weekday = RTC_DateStructure.RTC_WeekDay;
    time->hour = RTC_TimeStructure.RTC_Hours;
    time->minute = RTC_TimeStructure.RTC_Minutes;
    time->second = RTC_TimeStructure.RTC_Seconds;
   }
void RTC_Get_Time(rtc_time_t *time)
{
    rtc_time_t time1,time2;
    do
    {
        RTC_Get_Time_Once(&time1);
        RTC_Get_Time_Once(&time2);
    } while(memcmp(&time1,&time2,sizeof(rtc_time_t)) !=0);

    memcpy(time,&time1,sizeof(rtc_time_t));
}