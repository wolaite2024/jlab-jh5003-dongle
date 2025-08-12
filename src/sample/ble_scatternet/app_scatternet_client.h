/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_scatternet_client.h
   * @brief     This file handles BLE scatternet application routines.
   * @author    danni
   * @date      2022-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_SCATTERNET_CLIENT__
#define _APP_SCATTERNET_CLIENT__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "profile_client.h"

/** @defgroup  SCATTERNET_APP Scatternet Application
    * @brief This file handles BLE Scatternet application routines.
    * @{
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
extern T_CLIENT_ID simple_ble_client_id;/**< Simple ble service client id*/
extern T_CLIENT_ID gaps_client_id;/**< gap service client id*/
extern T_CLIENT_ID bas_client_id;/**< battery service client id*/

/**
 * @brief app_client_init
          Add GATT clients and register callbacks
 * @return void
 */
void app_client_init(void);

/** End of SCATTERNET_APP
* @}
*/
#ifdef __cplusplus
}
#endif
#endif
