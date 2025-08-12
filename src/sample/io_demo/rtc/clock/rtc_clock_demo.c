/*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtc_clock_demo.c
* @brief
* @details
* @author
* @date     2022-09-26
* @version  v0.1
*********************************************************************************************************
*/
#include "string.h"
#include "trace.h"
#include "vector_table.h"
#include "rtl876x_rcc.h"
#include "rtl876x_rtc.h"
#include "rtl876x_nvic.h"
#include "rtc_clock_demo.h"

static pSystemClockCB SystemClockCallBack = NULL;


static uint8_t rtc_clock_month_length_calc(uint8_t lpyr, uint8_t mon);
static void rtc_clock_convert_to_utc_time(uint32_t sec_time);
static void rtc_handler(void);

/**
  * @brief   rtc clock system clock init
  * @param   init offset second, sometimes is zero
  * @return  void
  */
static void rtc_clock_system_clock_init(uint32_t second)
{
    RTC_DeInit();
    RTC_SetPrescaler(RTC_PRESCALER_VALUE);
    RTC_MaskINTConfig(RTC_INT_CMP0, ENABLE);
    RTC_CompINTConfig(RTC_INT_CMP0, DISABLE);

    RamVectorTableUpdate(RTC_VECTORn, rtc_handler);
    /* Config RTC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    RTC_SystemWakeupConfig(ENABLE);
    rtc_clock_convert_to_utc_time(second);
    /* Init ticks from RTC */
    RtcClockSys.second_cnt_rtc = second;
    RtcClockSys.pre_rtc_tick_count = 0;
    rtc_clock_start();
}

void rtc_clock_demo(void)
{
    T_TIME_UNION defut_utc_time;

    defut_utc_time.time.year = 22;
    defut_utc_time.time.month = 9;
    defut_utc_time.time.day = 28;
    defut_utc_time.time.hours = 23;
    defut_utc_time.time.minute = 59;
    defut_utc_time.time.seconds = 59;

    rtc_clock_system_clock_init(0);
    rtc_clock_set_system_clock(defut_utc_time);
}

static void rtc_clock_sys_update(void)
{
    RtcClockSys.second_cnt_rtc = RtcClockSys.second_cnt_rtc + (60 - RtcClockSys.second_cnt_rtc % 60);
    RtcClockSys.pre_rtc_tick_count = RTC_GetCounter();
    /* get UTCTime time */
    rtc_clock_convert_to_utc_time(RtcClockSys.second_cnt_rtc);

    IO_PRINT_INFO6("rtc_clock_sys_update: year %d, month %d, day %d, hour %d, minute %d, second %d",
                   RtcClockSys.global_time.year, RtcClockSys.global_time.month,
                   RtcClockSys.global_time.day, RtcClockSys.global_time.hour,
                   RtcClockSys.global_time.minutes, RtcClockSys.global_time.seconds);
    if (SystemClockCallBack != NULL)
    {
        SystemClockCallBack();
    }
}

/**
  * @brief   RTC interrupt handler
  * @param   No parameter.
  * @return  void
  */
static void rtc_handler(void)
{
    uint32_t CompareValue = 0;

    IO_PRINT_INFO3("rtc_handler: RTC->INT_SR 0x%x, RTC->CR0 0x%x, RTC->INT_MASK 0x%x",
                   RTC->INT_SR, RTC->CR0, RTC->INT_MASK);
    if (RTC_GetINTStatus(RTC_INT_CMP0) == SET)
    {
        IO_PRINT_INFO0("rtc_handler: RTC_INT_CMP0");
        rtc_clock_sys_update();
        CompareValue = RTC_GetComp(ClOCK_RTC_COMPARATOR) + \
                       (RTC_CLOCK_SOURCE_FREQ / (RTC_PRESCALER_VALUE + 1)) * (60 - RtcClockSys.second_cnt_rtc %
                                                                              60); //minute interrupt
        RTC_SetComp(ClOCK_RTC_COMPARATOR, CompareValue & 0xFFFFFF);
        RTC_ClearCompINT(ClOCK_RTC_COMPARATOR);
    }
    if (RTC_GetINTStatus(RTC_INT_OVF) == SET)
    {
        IO_PRINT_INFO0("rtc_handler: RTC Overflow");
        RTC_ClearOverFlowINT();
    }
}

static uint8_t rtc_clock_month_length_calc(uint8_t lpyr, uint8_t mon)
{
    uint8_t days = 31;

    if (mon == 1)   // feb
    {
        days = (28 + lpyr);
    }
    else
    {
        if (mon > 6)   // aug-dec
        {
            mon--;
        }

        if (mon & 1)
        {
            days = 30;
        }
    }
    return (days);
}

static void rtc_clock_convert_to_utc_time(uint32_t sec_time)
{
    /* calculate the time less than a day - hours, minutes, seconds */
    {
        uint32_t day = sec_time % DAY;
        RtcClockSys.global_time.seconds = day % 60UL;
        RtcClockSys.global_time.minutes = (day % 3600UL) / 60UL;
        RtcClockSys.global_time.hour = day / 3600UL;
    }

    /* Fill in the calendar - day, month, year */
    {
        uint16_t numDays = sec_time / DAY;
        RtcClockSys.global_time.year = BEGYEAR;
        while (numDays >= YearLength(RtcClockSys.global_time.year))
        {
            numDays -= YearLength(RtcClockSys.global_time.year);
            RtcClockSys.global_time.year++;
        }

        RtcClockSys.global_time.month = 0;
        while (numDays >= rtc_clock_month_length_calc(IsLeapYear(RtcClockSys.global_time.year),
                                                      RtcClockSys.global_time.month))
        {
            numDays -= rtc_clock_month_length_calc(IsLeapYear(RtcClockSys.global_time.year),
                                                   RtcClockSys.global_time.month);
            RtcClockSys.global_time.month++;
        }
        RtcClockSys.global_time.day = numDays;
        RtcClockSys.global_time.month = RtcClockSys.global_time.month + 1;
        RtcClockSys.global_time.day = RtcClockSys.global_time.day + 1;
    }
}

void rtc_clock_start(void)
{
    uint32_t CompareValue;

    IO_PRINT_INFO1("rtc_clock_start: second_diff_value %d",
                   (60 - RtcClockSys.second_cnt_rtc % 60));
    CompareValue = 32000;//trigger RTC compare interrupter afer 1 second.
    RTC_SetComp(ClOCK_RTC_COMPARATOR, CompareValue & 0xFFFFFF);
    RTC_MaskINTConfig(RTC_INT_CMP0, DISABLE);
    RTC_CompINTConfig(RTC_INT_CMP0, ENABLE);
    RTC_RunCmd(ENABLE);
    IO_PRINT_INFO0("rtc_clock_start: end");
}

void rtc_clock_set_system_clock(T_TIME_UNION time)
{
    uint32_t i = 0;
    uint32_t offset = 0;

    //day time
    offset += time.time.seconds;
    offset += time.time.minute * 60;
    offset += time.time.hours * 60 * 60;

    uint8_t leapYear = IsLeapYear(time.time.year + 2000);

    offset += DAY * (time.time.day - 1);

    for (i = 0; i < time.time.month - 1; ++i)
    {
        //month start from 1
        offset += rtc_clock_month_length_calc(leapYear, i) * DAY;
    }

    for (i = 0; i < time.time.year ; ++i)
    {
        if (IsLeapYear(i + 2000))
        {
            offset += DAY * 366;
        }
        else
        {
            offset += DAY * 365;
        }
    }
    RtcClockSys.second_cnt_rtc = offset;
    rtc_clock_convert_to_utc_time(RtcClockSys.second_cnt_rtc);
}

/* calculate day of week */
T_DAY_OF_WEEK rtc_clock_get_day_of_week(uint32_t secTime)
{
    uint32_t day = secTime / DAY;

    T_DAY_OF_WEEK today = (T_DAY_OF_WEEK)(((day % LENGTH_OF_WEEK) + SYSTEM_ORIGIN_DAY_OF_WEEK) %
                                          LENGTH_OF_WEEK);

    return today;
}

uint8_t rtc_clock_get_system_clock_second(void)
{

    uint32_t cur_rtc_tick_count = RTC_GetCounter();
    uint32_t diff_second = 0;
    if (cur_rtc_tick_count > RtcClockSys.pre_rtc_tick_count)
    {
        diff_second = (cur_rtc_tick_count - RtcClockSys.pre_rtc_tick_count) / RTC_CLOCK_SOURCE_FREQ;
    }
    else
    {
        diff_second = (cur_rtc_tick_count + 0xffffff - RtcClockSys.pre_rtc_tick_count) /
                      RTC_CLOCK_SOURCE_FREQ;
    }
    return (RtcClockSys.second_cnt_rtc + diff_second) % 60;
}

uint32_t rtc_clock_convert_time_to_second(T_TIME_UNION time)
{
    uint32_t i = 0;
    uint32_t offset = 0;

    /* day time */
    offset += time.time.seconds;
    offset += time.time.minute * 60;
    offset += time.time.hours * 60 * 60;

    uint8_t leapYear = IsLeapYear(time.time.year + 2000);

    offset += DAY * (time.time.day - 1);

    for (i = 0; i < time.time.month - 1; ++i)
    {
        /* month start from 1 */
        offset += rtc_clock_month_length_calc(leapYear, i) * DAY;
    }

    for (i = 0; i < time.time.year ; ++i)
    {
        if (IsLeapYear(i + 2000))
        {
            offset += DAY * 366;
        }
        else
        {
            offset += DAY * 365;
        }
    }

    return offset;
}
