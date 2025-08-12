/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_scatternet_service.h
   * @brief     This file handles BLE peripheral application routines.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

#ifndef _APP_SCATTERNET_SERVICE__
#define _APP_SCATTERNET_SERVICE__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "profile_server.h"
#include "simple_ble_service.h"
#include "bas.h"

/** @defgroup SCATTERNET_APP Scatternet Application
  * @brief Scatternet Application
  * @{
  */
extern T_SERVER_ID simp_srv_id;/**< Simple ble service id*/
extern T_SERVER_ID bas_srv_id;/**< Battery service id */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief app_service_init
 * Init simple service, battery service and register service callback
 *
 */
void app_service_init(void);

/** End of SCATTERNET_APP
* @}
*/

#ifdef __cplusplus
}
#endif
#endif
