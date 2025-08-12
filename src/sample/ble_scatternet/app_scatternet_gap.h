/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_scatternet_gap.h
   * @brief     This file handles BLE scatternet application routines.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_SCATTERNET_GAP_H_
#define _APP_SCATTERNET_GAP_H_

#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "app_msg.h"
#include <profile_client.h>
#include <profile_server.h>

/** @defgroup SCATTERNET_APP Scatternet Application
  * @brief Scatternet Application
  * @{
  */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief    All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void app_handle_io_msg(T_IO_MSG io_msg);

#if F_BT_AIRPLANE_MODE_SUPPORT
void app_gap_common_callback(uint8_t cb_type, void *p_cb_data);
#endif

#if F_BT_GAPS_CHAR_WRITEABLE
T_APP_RESULT gap_service_callback(T_SERVER_ID service_id, void *p_para);
#endif

/**
  * @brief app_gap_init
  * Initialize peripheral and gap bond manager related parameters
  * @return void
  */
void app_gap_init(void);
/** End of SCATTERNET_APP
* @}
*/
#ifdef __cplusplus
}
#endif
#endif
