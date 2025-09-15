/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_peripheral_client.c
   * @brief     This file handles BLE peripheral application routines.
   * @author    danni
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "app_peripheral_client.h"
#include "bt_gatt_client.h"
#if F_APP_BT_ANCS_CLIENT_SUPPORT
#include "ancs_sample.h"
#endif

/** @defgroup PERIPH_APP Peripheral Application
  * @brief Peripheral Application
  * @{
  */
/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief app_peripheral_client_init
 * Init ANCS Client and AMS Client
 */
void app_peripheral_client_init(void)
{
#if F_APP_BT_GATT_CLIENT_SUPPORT
    gatt_client_init(GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT |  GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT);

#if F_APP_BT_ANCS_CLIENT_SUPPORT
    app_ancs_client_init();
#endif
#endif
}

/** End of PERIPH_APP
* @}
*/
