/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_flags.h
   * @brief     This file is used to config app functions.
   * @author    danni
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_FLAGS_H_
#define _APP_FLAGS_H_


/** @defgroup  PERIPH_Config Peripheral App Configuration
    * @brief This file is used to config app functions.
    * @{
    */
/*============================================================================*
 *                              Constants
 *============================================================================*/

/**
 * @brief Config BLE Max Link number
 *
 */
#define APP_MAX_LINKS 1

/**
 * @brief Config DLPS
 * 0-Disable DLPS, 1-Enable DLPS
 *
 */
#define F_DLPS_EN 1

/**
 * @brief  Config ANCS Client
 * 0-Not built in, 1-Open ANCS client function
 *
 */
#define F_APP_BT_GATT_CLIENT_SUPPORT  0
#define F_APP_BT_ANCS_CLIENT_SUPPORT  (F_APP_BT_GATT_CLIENT_SUPPORT & 1)

/**
 * @brief Config ANCS Parameters
 *
 */
#define F_BT_ANCS_APP_FILTER   (F_APP_BT_ANCS_CLIENT_SUPPORT & 1)
#define F_BT_ANCS_GET_APP_ATTR (F_APP_BT_ANCS_CLIENT_SUPPORT & 0)

/**
 * @brief Config ANCS Client debug log
 *  0-close, 1-open
 *
 */
#define F_BT_ANCS_CLIENT_DEBUG (F_APP_BT_ANCS_CLIENT_SUPPORT & 0)


#define F_APP_GATT_SERVER_EXT_API_SUPPORT        1
/** @} */ /* End of group PERIPH_Config */
#endif
