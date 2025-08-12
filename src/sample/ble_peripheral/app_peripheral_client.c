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
    client_init(F_APP_BLE_ANCS_CLIENT_SUPPORT + F_APP_BLE_AMS_CLIENT_SUPPORT);

#if F_APP_BLE_ANCS_CLIENT_SUPPORT
    ancs_init(APP_MAX_LINKS);
#endif

#if F_APP_BLE_AMS_CLIENT_SUPPORT
    ams_init(APP_MAX_LINKS);
#endif
}

/** End of PERIPH_APP
* @}
*/
