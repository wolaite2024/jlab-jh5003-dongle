/*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtc_calendar.c
* @brief
* @details
* @author   colin
* @date     2024-03-15
* @version  v1.0
*********************************************************************************************************
*/
#include "string.h"
#include "trace.h"
#include "rtc_calendar.h"
#include "vector_table.h"
#include "rtl876x_rtc.h"
#include "rtl876x_nvic.h"

typedef struct
{
    uint32_t               second_cnt_rtc;
    uint32_t               pre_rtc_tick_count;
    T_UTC_TIME             global_time;
} T_RTC_CLOCK_SYS;

typedef struct
{
    bool                   is_repeat;
    uint32_t               expiry_second;
    uint32_t               interval_second;
    P_RTC_ALARM_CB         callback;
} T_ALARN_PARA;

typedef struct
{
    COMPX_INDEX_t comp;
    uint32_t interrupt;
    uint32_t wakeup;
} T_ALARN_COMP;
/*============================================================================*
 *                         Constants
 *============================================================================*/


#define RTC_CLOCK_SOURCE_FREQ               32000
#define RTC_CNT_MAX_VALUE                   0xFFFFFF         //RTC->CNT: [23:0]
#define RTC_PRESCALER_VALUE                 0
#define CALENDAR_RTC_COMPARATOR             COMP0_INDEX
#define CALENDAR_RTC_COMP_INT               RTC_INT_CMP0
#define CALENDAR_RTC_COMP_WK_INT            RTC_CMP0_WK_INT
#define RTC_CALENDAR_UPDATE_TIME_MIN_S      (1)
#define RTC_CALENDAR_UPDATE_TIME_MAX_S      (4 * 60)        //max update time (8/2=4) minutes due to overflow condition
#define RTC_CALENDAR_ONE_CYCLE_TIME_S       (RTC_CNT_MAX_VALUE * (RTC_PRESCALER_VALUE + 1) / RTC_CLOCK_SOURCE_FREQ)

#define IsLeapYear(yr) (!((yr) % 400) || (((yr) % 100) && !((yr) % 4)))
#define YearLength(yr) (IsLeapYear(yr) ? 366 : 365)
#define BEGYEAR                     2000     // UTC started at 00:00:00 January 1, 2000
#define DAY                         86400UL  // 24 hours * 60 minutes * 60 seconds
#define ONE_MINUTE                  60UL     // 1 minute = 60 seconds
#define ONE_HOUR                    3600UL   // 1 hour = 60 * 60 seconds
#define SYSTEM_ORIGIN_DAY_OF_WEEK   (Sat)    //2000-01-01 is sat
#define LENGTH_OF_WEEK              (7)
#define LEAP_YEAR_DAYS              (366)
#define COMMON_YEAR_DAYS            (365)
#define ODD_MONTH_DAYS              (31)
#define EVEN_MONTH_DAYS             (30)
#define FEBRUARY_MIN_DAYS           (28)
#define MONTH_MAX                   (12)
#define HOUR_MAX                    (23)
#define MINUTE_MAX                  (59)
#define SECOND_MAX                  (59)

uint32_t rtc_calendar_update_time_sec = RTC_CALENDAR_UPDATE_TIME_MAX_S;

#define RTC_CALENDAR_COMP_UPDATE_CNT           ((RTC_CLOCK_SOURCE_FREQ / (RTC_PRESCALER_VALUE + 1)) * rtc_calendar_update_time_sec)

static P_RTC_CALENDAR_CB RTCCalendarCallBack = NULL;
static T_RTC_CLOCK_SYS RtcCalendar __attribute__((aligned(4)));

static T_ALARN_PARA alarm_para[ALARM_MAX_INDEX] = {0};
static const T_ALARN_COMP alarm[ALARM_MAX_INDEX] = {{COMP1_INDEX, RTC_INT_CMP1, RTC_CMP1_WK_INT},
    {COMP2_INDEX, RTC_INT_CMP2, RTC_CMP2_WK_INT},
    {COMP3_INDEX, RTC_INT_CMP3, RTC_CMP3_WK_INT}
};

static uint8_t rtc_calendar_month_length_calc(uint8_t lpyr, uint8_t mon)
{
    uint8_t days = ODD_MONTH_DAYS;

    if (mon == 1)   // feb
    {
        days = (FEBRUARY_MIN_DAYS + lpyr);
    }
    else
    {
        if (mon > 6)   // aug-dec
        {
            mon--;
        }

        if (mon & 1)
        {
            days = EVEN_MONTH_DAYS;
        }
    }
    return (days);
}

static bool rtc_calendar_check_utc_time(T_UTC_TIME *utc_time)
{
    if (utc_time->year < BEGYEAR)
    {
        return false;
    }

    if ((utc_time->month == 0) || (utc_time->month > MONTH_MAX))
    {
        return false;
    }

    if ((utc_time->day == 0) ||
        (utc_time->day > rtc_calendar_month_length_calc(IsLeapYear(utc_time->year),
                                                        utc_time->month)))
    {
        return false;
    }

    if ((utc_time->hour > HOUR_MAX) || (utc_time->minutes > MINUTE_MAX) ||
        (utc_time->seconds > SECOND_MAX))
    {
        return false;
    }

    return true;
}

T_UTC_TIME rtc_calendar_timestamp_to_date(uint32_t timestamp)
{
    T_UTC_TIME utc_time = RtcCalendar.global_time;
    /* calculate the time less than a day - hours, minutes, seconds */
    {

        uint32_t day = timestamp % DAY;
        utc_time.seconds = day % ONE_MINUTE;
        utc_time.minutes = (day % ONE_HOUR) / ONE_MINUTE;
        utc_time.hour = day / ONE_HOUR;
    }

    /* Fill in the calendar - day, month, year */
    {
        uint16_t numDays = timestamp / DAY;

        utc_time.year = BEGYEAR;
        while (numDays >= YearLength(utc_time.year))
        {
            numDays -= YearLength(utc_time.year);
            utc_time.year++;
        }

        utc_time.month = 0;

        while (numDays >= rtc_calendar_month_length_calc(IsLeapYear(utc_time.year),
                                                         utc_time.month))
        {
            numDays -= rtc_calendar_month_length_calc(IsLeapYear(utc_time.year),
                                                      utc_time.month);
            utc_time.month++;
        }

        utc_time.day = numDays + 1;
        utc_time.month++;
    }

    return utc_time;
}

uint32_t rtc_calendar_date_to_timestamp(T_UTC_TIME *utc_time)
{

    if (!rtc_calendar_check_utc_time(utc_time))
    {
        return 0;
    }

    uint32_t i = 0;
    uint32_t timestamp = 0;

    /* day time */
    timestamp += utc_time->seconds;
    timestamp += utc_time->minutes * ONE_MINUTE;
    timestamp += utc_time->hour * ONE_HOUR;

    uint8_t leapYear = IsLeapYear(utc_time->year);

    timestamp += DAY * (utc_time->day - 1);

    for (i = 0; i < utc_time->month - 1; ++i)
    {
        /* month start from 1 */
        timestamp += rtc_calendar_month_length_calc(leapYear, i) * DAY;
    }

    for (i = BEGYEAR; i < utc_time->year ; ++i)
    {
        if (IsLeapYear(i))
        {
            timestamp += DAY * LEAP_YEAR_DAYS;
        }
        else
        {
            timestamp += DAY * COMMON_YEAR_DAYS;
        }
    }

    return timestamp;
}

static void rtc_calendar_update(void)
{
    RtcCalendar.second_cnt_rtc += rtc_calendar_update_time_sec;
    RtcCalendar.pre_rtc_tick_count = RTC_GetCounter();
    /* get UTCTime time */
    RtcCalendar.global_time = rtc_calendar_timestamp_to_date(RtcCalendar.second_cnt_rtc);

    if (RTCCalendarCallBack != NULL)
    {
        RTCCalendarCallBack(&(RtcCalendar.global_time));
    }
}

static void rtc_calendar_comp_update(void)
{
    uint32_t CompareValue = 0;
    CompareValue = RTC_GetComp(CALENDAR_RTC_COMPARATOR) + RTC_CALENDAR_COMP_UPDATE_CNT;

    if (CompareValue > RTC_CNT_MAX_VALUE)
    {
        CompareValue = CompareValue - RTC_CNT_MAX_VALUE;
    }
    RTC_SetComp(CALENDAR_RTC_COMPARATOR, CompareValue & RTC_CNT_MAX_VALUE);
}

static void rtc_calendar_set_alarm_cnt(COMPX_INDEX_t comp_index, uint32_t second)
{
    if (second > RTC_CALENDAR_ONE_CYCLE_TIME_S)
    {
        RTC_SetComp(comp_index, RTC_CNT_MAX_VALUE);
        return;
    }

    uint32_t CompareValue = 0;
    CompareValue = RTC_GetCounter() + ((RTC_CLOCK_SOURCE_FREQ / (RTC_PRESCALER_VALUE + 1)) * second);

    if (CompareValue > RTC_CNT_MAX_VALUE)
    {
        CompareValue = CompareValue - RTC_CNT_MAX_VALUE;
    }

    RTC_SetComp(comp_index, CompareValue & RTC_CNT_MAX_VALUE);
}

static void rtc_calendar_alarm_update(T_ALARM_INDEX alarm_index)
{
    uint32_t current_second = rtc_calendar_get_timestamp();

    if (alarm_para[alarm_index].expiry_second <= current_second)
    {
        T_UTC_TIME utc_time = rtc_calendar_timestamp_to_date(current_second);
        alarm_para[alarm_index].callback(alarm_index, &utc_time);

        if (alarm_para[alarm_index].is_repeat)
        {
            alarm_para[alarm_index].expiry_second = current_second + alarm_para[alarm_index].interval_second;
        }
        else
        {
            RTC_CompINTConfig(alarm[alarm_index].interrupt | alarm[alarm_index].wakeup, DISABLE);
            return;
        }
    }

    rtc_calendar_set_alarm_cnt(alarm[alarm_index].comp,
                               alarm_para[alarm_index].expiry_second - current_second);
}

/**
  * @brief   RTC interrupt handler
  * @param   No parameter.
  * @return  void
  */
static void rtc_calendar_handler(void)
{
    if (RTC_GetINTStatus(CALENDAR_RTC_COMP_INT) == SET)
    {
        rtc_calendar_update();
        rtc_calendar_comp_update();
        RTC_ClearCompINT(CALENDAR_RTC_COMPARATOR);
        RTC_ClearCompWkINT(CALENDAR_RTC_COMPARATOR);
    }

    for (T_ALARM_INDEX alarm_index = ALARM_0; alarm_index < ALARM_MAX_INDEX; alarm_index++)
    {
        if (RTC_GetINTStatus(alarm[alarm_index].interrupt) == SET)
        {
            rtc_calendar_alarm_update(alarm_index);
            RTC_ClearCompINT(alarm[alarm_index].comp);
            RTC_ClearCompWkINT(alarm[alarm_index].comp);
        }
    }
}

static uint32_t rtc_calendar_calculate_diff_second(uint32_t cur_cnt, uint32_t pre_cnt)
{
    uint32_t diff_second = 0;

    if (cur_cnt > pre_cnt)
    {
        diff_second = (cur_cnt - pre_cnt) / (RTC_CLOCK_SOURCE_FREQ / (RTC_PRESCALER_VALUE + 1));
    }
    else
    {
        diff_second = (cur_cnt + RTC_CNT_MAX_VALUE - pre_cnt) / (RTC_CLOCK_SOURCE_FREQ /
                                                                 (RTC_PRESCALER_VALUE + 1));
    }

    return diff_second;
}

/* calculate day of week */
T_DAY_OF_WEEK rtc_calendar_get_current_day_of_week(void)
{
    uint32_t day = RtcCalendar.second_cnt_rtc / DAY;

    T_DAY_OF_WEEK today = (T_DAY_OF_WEEK)(((day % LENGTH_OF_WEEK) + SYSTEM_ORIGIN_DAY_OF_WEEK) %
                                          LENGTH_OF_WEEK);

    return today;
}

uint32_t rtc_calendar_get_timestamp(void)
{

    uint32_t cur_rtc_tick_count = RTC_GetCounter();
    uint32_t diff_second = 0;

    diff_second = rtc_calendar_calculate_diff_second(cur_rtc_tick_count,
                                                     RtcCalendar.pre_rtc_tick_count);

    return (RtcCalendar.second_cnt_rtc + diff_second);
}

void rtc_calendar_get_utc_time(T_UTC_TIME *utc_time)
{
    uint32_t cur_rtc_tick_count = RTC_GetCounter();
    uint32_t diff_second = 0;

    diff_second = rtc_calendar_calculate_diff_second(cur_rtc_tick_count,
                                                     RtcCalendar.pre_rtc_tick_count);

    *utc_time = rtc_calendar_timestamp_to_date(RtcCalendar.second_cnt_rtc + diff_second);

    return;
}

bool rtc_calendar_set_utc_time(T_UTC_TIME *utc_time)
{
    if (!rtc_calendar_check_utc_time(utc_time))
    {
        return false;
    }
    RtcCalendar.second_cnt_rtc = rtc_calendar_date_to_timestamp(utc_time);
    RtcCalendar.global_time = *utc_time;

    return true;
}

void rtc_calendar_register_callback(P_RTC_CALENDAR_CB cb)
{
    RTCCalendarCallBack = cb;
}

void rtc_calendar_unregister_callback(void)
{
    RTCCalendarCallBack = NULL;
}

void rtc_calendar_set_update_interval(uint32_t update_interval_sec)
{
    RTC_CompINTConfig(CALENDAR_RTC_COMP_INT | CALENDAR_RTC_COMP_WK_INT, DISABLE);
    RTC_ClearCompINT(CALENDAR_RTC_COMPARATOR);
    RTC_ClearCompWkINT(CALENDAR_RTC_COMPARATOR);

    if (update_interval_sec < RTC_CALENDAR_UPDATE_TIME_MAX_S)
    {
        if (update_interval_sec < RTC_CALENDAR_UPDATE_TIME_MIN_S)
        {
            rtc_calendar_update_time_sec = RTC_CALENDAR_UPDATE_TIME_MIN_S;
        }
        else
        {
            rtc_calendar_update_time_sec =  update_interval_sec;
        }
    }

    RtcCalendar.second_cnt_rtc = rtc_calendar_get_timestamp();

    uint32_t CompareValue = 0;
    CompareValue = RTC_GetCounter() + RTC_CALENDAR_COMP_UPDATE_CNT;

    if (CompareValue > RTC_CNT_MAX_VALUE)
    {
        CompareValue = CompareValue - RTC_CNT_MAX_VALUE;
    }
    RTC_SetComp(CALENDAR_RTC_COMPARATOR, CompareValue & RTC_CNT_MAX_VALUE);

    RTC_CompINTConfig(CALENDAR_RTC_COMP_INT | CALENDAR_RTC_COMP_WK_INT, ENABLE);
}

bool rtc_calendar_add_alarm_by_utc(T_ALARM_INDEX alarm_index, T_UTC_TIME *utc_time,
                                   P_RTC_ALARM_CB cb)
{
    uint32_t second = rtc_calendar_date_to_timestamp(utc_time);
    uint32_t current_second = rtc_calendar_get_timestamp();

    if (second <= current_second)
    {
        return false;
    }

    RTC_CompINTConfig(alarm[alarm_index].interrupt | alarm[alarm_index].wakeup, DISABLE);
    RTC_ClearCompINT(alarm[alarm_index].comp);
    RTC_ClearCompWkINT(alarm[alarm_index].comp);

    alarm_para[alarm_index].expiry_second = second;
    alarm_para[alarm_index].interval_second = 0;
    alarm_para[alarm_index].is_repeat = false;
    alarm_para[alarm_index].callback = cb;

    rtc_calendar_set_alarm_cnt(alarm[alarm_index].comp, second - current_second);

    RTC_CompINTConfig(alarm[alarm_index].interrupt | alarm[alarm_index].wakeup, ENABLE);

    return true;
}

bool rtc_calendar_add_alarm_by_second(T_ALARM_INDEX alarm_index, uint32_t second,
                                      P_RTC_ALARM_CB cb, bool is_repeat)
{

    if (second == 0)
    {
        return false;
    }

    uint32_t current_second = rtc_calendar_get_timestamp();

    RTC_CompINTConfig(alarm[alarm_index].interrupt | alarm[alarm_index].wakeup, DISABLE);
    RTC_ClearCompINT(alarm[alarm_index].comp);
    RTC_ClearCompWkINT(alarm[alarm_index].comp);

    alarm_para[alarm_index].expiry_second = current_second + second;
    alarm_para[alarm_index].interval_second = second;
    alarm_para[alarm_index].is_repeat = is_repeat;
    alarm_para[alarm_index].callback = cb;

    rtc_calendar_set_alarm_cnt(alarm[alarm_index].comp, second);

    RTC_CompINTConfig(alarm[alarm_index].interrupt | alarm[alarm_index].wakeup, ENABLE);

    return true;
}

void rtc_calendar_delete_alarm(T_ALARM_INDEX alarm_index)
{
    RTC_CompINTConfig(alarm[alarm_index].interrupt | alarm[alarm_index].wakeup, DISABLE);
    RTC_ClearCompINT(alarm[alarm_index].comp);
    RTC_ClearCompWkINT(alarm[alarm_index].comp);
}

bool rtc_calendar_int(T_UTC_TIME *default_utc_time, uint32_t update_interval_sec)
{

    if (!rtc_calendar_set_utc_time(default_utc_time))
    {
        return false;
    }

    if (update_interval_sec < RTC_CALENDAR_UPDATE_TIME_MAX_S)
    {
        if (update_interval_sec < RTC_CALENDAR_UPDATE_TIME_MIN_S)
        {
            rtc_calendar_update_time_sec = RTC_CALENDAR_UPDATE_TIME_MIN_S;
        }
        else
        {
            rtc_calendar_update_time_sec =  update_interval_sec;
        }
    }

    RTC_DeInit();
    RTC_SetPrescaler(RTC_PRESCALER_VALUE);
    RTC_MaskINTConfig(CALENDAR_RTC_COMP_INT | CALENDAR_RTC_COMP_WK_INT, ENABLE);
    RTC_CompINTConfig(CALENDAR_RTC_COMP_INT | CALENDAR_RTC_COMP_WK_INT, DISABLE);

    RamVectorTableUpdate(RTC_VECTORn, rtc_calendar_handler);
    /* Config RTC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    RTC_SystemWakeupConfig(ENABLE);

    RtcCalendar.pre_rtc_tick_count = 0;

    RTC_SetComp(CALENDAR_RTC_COMPARATOR, RTC_CALENDAR_COMP_UPDATE_CNT);
    RTC_MaskINTConfig(CALENDAR_RTC_COMP_INT | CALENDAR_RTC_COMP_WK_INT, DISABLE);
    RTC_CompINTConfig(CALENDAR_RTC_COMP_INT | CALENDAR_RTC_COMP_WK_INT, ENABLE);
    RTC_RunCmd(ENABLE);

    IO_PRINT_INFO6("rtc_calendar_int: year %d, month %d, day %d, hour %d, minute %d, second %d",
                   RtcCalendar.global_time.year, RtcCalendar.global_time.month,
                   RtcCalendar.global_time.day, RtcCalendar.global_time.hour,
                   RtcCalendar.global_time.minutes, RtcCalendar.global_time.seconds);

    return true;
}
