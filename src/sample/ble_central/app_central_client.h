/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_central_client.h
   * @brief     This file handles BLE central application routines.
   * @author    danni
   * @date      2022-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_CENTRAL_CLIENT__
#define _APP_CENTRAL_CLIENT__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <profile_client.h>
#if F_APP_BLE_ANCS_CLIENT_SUPPORT
#include <ancs.h>
#endif
#if F_APP_BLE_AMS_CLIENT_SUPPORT
#include <ams.h>
#endif

/** @defgroup  CENTRAL_APP Central Application
    * @brief This file handles BLE central application routines.
    * @{
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
extern T_CLIENT_ID simple_ble_client_id;/**< Simple ble service client id*/
extern T_CLIENT_ID gaps_client_id;/**< gap service client id*/
extern T_CLIENT_ID bas_client_id;/**< battery service client id*/

/**
 * @brief app_central_client_discov_services
          Discovery GATT services in ble_peripheral project.
 * @param  conn_id connection ID.
 * @param  start   true - first call this function after conncection, false - not first
 * @retval None
 */
void app_central_client_discov_services(uint8_t conn_id, bool start);

/**
 * @brief app_central_client_init
          Add GATT clients and register callbacks
 * @return void
 */
void app_central_client_init(void);

/** End of CENTRAL_APP
* @}
*/
#ifdef __cplusplus
}
#endif
#endif
