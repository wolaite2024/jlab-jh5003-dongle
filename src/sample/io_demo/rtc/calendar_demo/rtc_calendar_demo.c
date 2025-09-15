/*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtc_calendar_demo.c
* @brief
* @details
* @author
* @date     2024-03-28
* @version  v1.0
*********************************************************************************************************
*/
#include "trace.h"
#include "io_dlps.h"
#include "pm.h"
#include "rtc_calendar.h"

void rtc_callback(T_UTC_TIME *global_time)
{
    IO_PRINT_INFO6("rtc_callback: year %d, month %d, day %d, hour %d, minute %d, second %d",
                   global_time->year, global_time->month,
                   global_time->day, global_time->hour,
                   global_time->minutes, global_time->seconds);
}

void alarm_callback(T_ALARM_INDEX alarm_index, T_UTC_TIME *utc_time)
{
    IO_PRINT_INFO7("alarm_callback: alarm %d year %d, month %d, day %d, hour %d, minute %d, second %d",
                   alarm_index,
                   utc_time->year, utc_time->month,
                   utc_time->day, utc_time->hour,
                   utc_time->minutes, utc_time->seconds);
}

void rtc_calendar_demo(void)
{
    T_UTC_TIME default_utc_time;

    default_utc_time.year = 2024;
    default_utc_time.month = 3;
    default_utc_time.day = 28;
    default_utc_time.hour = 10;
    default_utc_time.minutes = 41;
    default_utc_time.seconds = 0;

    rtc_calendar_register_callback(rtc_callback);

    if (!rtc_calendar_int(&default_utc_time, 10))
    {
        IO_PRINT_INFO0("rtc_calendar_demo: wrong default_utc_time");
    }

    default_utc_time.year = 2024;
    default_utc_time.month = 3;
    default_utc_time.day = 28;
    default_utc_time.hour = 10;
    default_utc_time.minutes = 41;
    default_utc_time.seconds = 20;

    rtc_calendar_add_alarm_by_utc(ALARM_0, &default_utc_time, alarm_callback);
    rtc_calendar_add_alarm_by_second(ALARM_1, 5, alarm_callback, false);
    rtc_calendar_add_alarm_by_second(ALARM_2, 15, alarm_callback, true);

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);

    io_dlps_register();

    power_mode_set(POWER_DLPS_MODE);
}


