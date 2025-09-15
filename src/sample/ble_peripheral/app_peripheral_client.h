/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_peripheral_client.h
   * @brief     This file handles BLE peripheral application routines.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

#ifndef _APP_PERIPHERAL_CLIENT__
#define _APP_PERIPHERAL_CLIENT__

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/


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
void app_peripheral_client_init(void);

/** End of PERIPH_APP
* @}
*/

#ifdef __cplusplus
}
#endif
#endif
