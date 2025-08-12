/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_observer_gap.h
   * @brief     This file handles BLE observer application routines.
   * @author    danni
   * @date      2022-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

#ifndef _APP_OBSERVER_GAP_H_
#define _APP_OBSERVER_GAP_H_

#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include <gap_le.h>

/** @defgroup OB_APP Observer Application
  * @brief Observer Application
  * @{
  */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief app_handle_io_msg
 *        All the application messages are pre-handled in this function
 * @note  All the IO MSGs are sent to this function, then the event handling
 *        function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return    void
 */
void app_handle_io_msg(T_IO_MSG io_msg);

/**
  * @brief app_gap_callback
  *        Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval    result @ref T_APP_RESULT
  */
T_APP_RESULT app_gap_callback(uint8_t cb_type, void *p_cb_data);

/**
 * @brief app_gap_init
 *        register gap message callback and initialize ble manager module.
 */
void app_gap_init(void);

/** End of OB_APP
* @}
*/

#ifdef __cplusplus
}
#endif
#endif
