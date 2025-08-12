/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_peripheral_app.h
   * @brief     This file handles BLE peripheral application routines.
   * @author    danni
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

#ifndef _APP_PERIPHERAL_GAP__
#define _APP_PERIPHERAL_GAP__

#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include <profile_server.h>

/** @defgroup PERIPH_APP Peripheral Application
  * @brief Peripheral Application
  * @{
  */

/*============================================================================*
 *                              Variables
 *============================================================================*/
extern T_SERVER_ID simp_srv_id; /**< Simple ble service id*/
extern T_SERVER_ID bas_srv_id;  /**< Battery service id */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief app_peripheral_gap_handle_io_msg
 * All the application messages are pre-handled in this function
 * @note All the IO MSGs are sent to this function, then the event handling
 * function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void app_peripheral_gap_handle_io_msg(T_IO_MSG io_msg);

/**
  * @brief app_peripheral_gap_callback
  * Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result @ref T_APP_RESULT
  */
T_APP_RESULT app_peripheral_gap_callback(uint8_t cb_type, void *p_cb_data);

/**
  * @brief app_peripheral_gap_init
  * Initialize peripheral and gap bond manager related parameters
  * @return void
  */
void app_peripheral_gap_init(void);

/** End of PERIPH_APP
* @}
*/

#ifdef __cplusplus
}
#endif

#endif
