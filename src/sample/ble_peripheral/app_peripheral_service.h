/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      peripheral_app.h
   * @brief     This file handles BLE peripheral application routines.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

#ifndef _APP_PERIPHERAL_SERVICE__
#define _APP_PERIPHERAL_SERVICE__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <profile_server.h>
#include <simple_ble_service.h>
#include <bas.h>

/** @defgroup PERIPH_APP Peripheral Application
  * @brief Peripheral Application
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief app_peripheral_service_init
 * Init simple service, battery service and register service callback
 *
 */
void app_peripheral_service_init(void);

/** End of PERIPH_APP
* @}
*/

#ifdef __cplusplus
}
#endif
#endif
