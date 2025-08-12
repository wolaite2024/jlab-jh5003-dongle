/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_scatternet_scan.h
   * @brief     Routines to create App task and handle events & messages
   * @author    danni
   * @date      2022-06-02
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_SCATTERNET_SCAN_H_
#define _APP_SCATTERNET_SCAN_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"

#define APP_SCAN_INTERVAL     (0xA0)
#define APP_SCAN_WINDOW       (0x98)

/** @defgroup  SCATTERNET_APP Scatternet Application
    * @brief This file handles BLE Scatternet application routines.
    * @{
    */
/**
 * @brief app_scan_start
 * @param filter_policy
 * @param filter_duplicate
 */
void app_scan_start(uint8_t filter_policy, uint8_t filter_duplicate);

/**
 * @brief app_scan_stop
 *
 */
void app_scan_stop(void);

/** End of SCATTERNET_APP
* @}
*/
#ifdef __cplusplus
}
#endif
#endif
