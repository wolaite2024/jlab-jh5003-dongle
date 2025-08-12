/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtc_calendar.h
* @brief     This file provides api to use rtc calendar
* @details
* @author    colin
* @date      2024-03-15
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef _RTC_CALENDAR_H_
#define _RTC_CALENDAR_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
/*============================================================================*
 *                         Types
 *============================================================================*/

typedef struct
{
    uint16_t year;    // 2000+
    uint8_t month;    // 1-12
    uint8_t day;      // 1-31
    uint8_t seconds;  // 0-59
    uint8_t minutes;  // 0-59
    uint8_t hour;     // 0-23
} T_UTC_TIME;

typedef enum
{
    MOn  = 0,
    Tues = 1,
    Wed  = 2,
    Thur = 3,
    Fri  = 4,
    Sat  = 5,
    Sun  = 6
} T_DAY_OF_WEEK;

typedef void (* pRTCCalendarCB)(T_UTC_TIME *global_time);


/*============================================================================*
 *                         Functions
 *============================================================================*/

/**
 * \brief   Get current day of week.
 * \return  The day of week
 */
T_DAY_OF_WEEK rtc_calendar_get_current_day_of_week(void);

/**
 * \brief   Convert timestamp(s) to date, start at 00:00:00 January 1, 2000.
 * \param[in] timestamp: Timestamp(s) to be converted
 * \return  The UTC date converted
 */
T_UTC_TIME rtc_calendar_timestamp_to_date(uint32_t timestamp);

/**
 * \brief   Convert date to timestamp(s).
 * \param[in] utc_time: Time to be converted, start at 00:00:00 January 1, 2000.
 * \return  The timestamp converted, 0 means utc_time is wrong
 */
uint32_t rtc_calendar_date_to_timestamp(T_UTC_TIME *utc_time);

/**
 * \brief   Get current calendar timestamp(s).
 * \return  Current calendar timestamp(s).
 */
uint32_t rtc_calendar_get_timestamp(void);

/**
 * \brief   Get calendar utc time.
 * \param[in] utc_time: Calendar utc time pointer
 * \return  None.
 */
void rtc_calendar_get_utc_time(T_UTC_TIME *utc_time);

/**
 * \brief   Unregister rtc calendar callback.
 * \return  None.
 */
void rtc_calendar_unregister_callback(void);

/**
 * \brief   Register rtc calendar callback.
 * \param[in] cb: Calendar callback
 * \return  None.
 */
void rtc_calendar_register_callback(pRTCCalendarCB cb);

/**
 * \brief   Set calendar utc time.
 * \param[in] utc_time: Calendar utc time which started at 00:00:00 January 1, 2000
 * \return  True means setting calendar clock success, false means utc_time wrong
 */
bool rtc_calendar_set_utc_time(T_UTC_TIME *utc_time);

/**
 * \brief   Init rtc calendar.
 * \param[in] defut_utc_time: Calendar default time which started at 00:00:00 January 1, 2000
 * \param[in] update_freq_s: Calendar update interval, the value can be 1 ~ 240s
 * \return  True means init rtc calendar success, false means defut_utc_time wrong
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void calendar_callback(T_UTC_TIME *global_time)
 * {
 *   IO_PRINT_INFO6("calendar_callback: year %d, month %d, day %d, hour %d, minute %d, second %d",
 *                   global_time->year, global_time->month,
 *                   global_time->day, global_time->hour,
 *                   global_time->minutes, global_time->seconds);
 * }
 *
 * void rtc_calendar_module_int(void)
 * {
 *     T_UTC_TIME defut_utc_time;
 *
 *     defut_utc_time.time.year = 2024;
 *     defut_utc_time.time.month = 3;
 *     defut_utc_time.time.day = 15;
 *     defut_utc_time.time.hours = 16;
 *     defut_utc_time.time.minute = 15;
 *     defut_utc_time.time.seconds = 30;
 *
 *     rtc_calendar_register_callback(calendar_callback);
 *     rtc_calendar_int(&defut_utc_time, 10);
 * }
 * \endcode
 */
bool rtc_calendar_int(T_UTC_TIME *defut_utc_time, uint32_t update_interval_sec);

#ifdef __cplusplus
}
#endif


#endif //_RTC_CALENDAR_H_

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/
