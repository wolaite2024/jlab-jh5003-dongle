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

void rtc_calendar_demo(void)
{
    T_UTC_TIME defut_utc_time;

    defut_utc_time.year = 2024;
    defut_utc_time.month = 3;
    defut_utc_time.day = 28;
    defut_utc_time.hour = 10;
    defut_utc_time.minutes = 41;
    defut_utc_time.seconds = 0;

    rtc_calendar_register_callback(rtc_callback);

    if (!rtc_calendar_int(&defut_utc_time, 10))
    {
        IO_PRINT_INFO0("rtc_calendar_demo: wrong defut_utc_time");
    }

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);

    io_dlps_register();

    power_mode_set(POWER_DLPS_MODE);
}


