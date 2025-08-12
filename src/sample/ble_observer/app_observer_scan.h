/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_observer_scan.h
   * @brief     Routines to create App task and handle events & messages
   * @author    danni
   * @date      2022-06-02
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_OBSERVER_SCAN_H_
#define _APP_OBSERVER_SCAN_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"

#define APP_SCAN_INTERVAL     (0xA0)
#define APP_SCAN_WINDOW       (0x98)

/** @defgroup  OB_APP Observer Application
    * @brief This file handles BLE observer application routines.
    * @{
    */
/**
 * @brief app_scan_start
 * @param scan_interval The frequency of scan, in units of 0.625ms, range: 0x0004 to 0xFFFF.
 * @param scan_window   The length of scan, in units of 0.625ms, range: 0x0004 to 0xFFFF.
 */
void app_scan_start(uint16_t scan_interval, uint16_t scan_window);

/**
 * @brief app_scan_stop
 *
 */
void app_scan_stop(void);

/** End of OB_APP
* @}
*/
#ifdef __cplusplus
}
#endif
#endif
