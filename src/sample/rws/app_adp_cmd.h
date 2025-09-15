/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_ADP_CMD_H_
#define _APP_ADP_CMD_H_

#include <stdbool.h>
#include "app_loc_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif

#if F_APP_ADP_5V_CMD_SUPPORT || F_APP_ONE_WIRE_UART_SUPPORT
/** @defgroup APP_ADP_CMD APP Adaptor Command
  * @brief APP Adaptor Command
  * @{
  */

/**  @brief CHARGERBOX_SPECIAL_COMMAND type */
typedef enum
{
    APP_ADP_SPECIAL_CMD_NULL                = 0x00, // This CMD should not be used.
    APP_ADP_SPECIAL_CMD_CREATE_SPP_CON      = 0x01, // Create SPP connection
    APP_ADP_SPECIAL_CMD_FACTORY_MODE        = 0x02,
    APP_ADP_SPECIAL_CMD_RWS_STATUS_CHECK    = 0x03, // Get RWS engaged status
    APP_ADP_SPECIAL_CMD_OTA                 = 0x04, // Special CMD for OTA
    APP_ADP_SPECIAL_CMD_RWS_FORCE_ENGAGE    = 0x05, // Force RWS re-engaged with the same dongle ID
    APP_ADP_SPECIAL_CMD_RSV_6               = 0x06,
    APP_ADP_SPECIAL_CMD_RSV_7               = 0x07,
} T_APP_ADP_SPECIAL_CMD;

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup AUDIO_SMART_ADP_Exported_Types Audio smart adp cmd Types
    * @{
    */

/** @} */ /* End of group AUDIO_SMART_ADP_Exported_Types */

/**
    * @brief  adp pending cmd clear
    * @param  void
    * @return void
    */
void app_adp_cmd_clear_pending(void);

/**
    * @brief  adp pending cmd execute
    * @param  void
    * @return void
    */
void app_adp_cmd_pending_exec(void);

/**
    * @brief  adp specia cmd execute
    * @param  void
    * @return void
    */
void app_adp_cmd_special_cmd_handle(uint8_t jig_subcmd, uint8_t jig_dongle_id);

/**
    * @brief  battery valid check
    * @param  battery in/out
    * @return is valid battery
    */
bool app_adp_cmd_case_bat_check(uint8_t *bat_in, uint8_t *bat_out);

/**
    * @brief  battery valid check
    * @param  void
    * @return void
    */
void app_adp_cmd_delay_charger_enable(void);

/**
    * @brief  power on when out case
    * @param  void
    * @return void
    */
void app_adp_cmd_power_on_when_out_case(void);

/**
    * @brief  factory rst link disconnect delay
    * @param  delay time
    * @return none
    */
void app_adp_cmd_factory_reset_link_dis(uint16_t delay);

/**
    * @brief  bud change handle
    * @param  event
    * @param  from_remote
    * @param  para
    * @return none
    */
void app_adp_cmd_bud_loc_change_handle(T_BUD_LOCATION_EVENT evt, bool from_remote);
bool app_adp_cmd_in_case_timeout(void);

void app_adp_cmd_init(void);
void app_adp_cmd_check_disable_charger_timer_stop(void);

/**
    * @brief enter pcba shipping mode
    * @return none
    */
void app_adp_cmd_enter_pcba_shipping_mode(void);

/**
    * @brief  adp 1 wire uart cmd handle
    * @param  cmd_exec_index
    * @param  pay_load
    * @return none
    */
void app_adp_one_wire_cmd_handle_msg(uint32_t cmd_exec_index, uint8_t pay_load);

/** @} */ /* End of group APP_ADP_CMD */
#endif

#ifdef __cplusplus
}
#endif

#endif /*_APP_ADP_CMD_H_*/

