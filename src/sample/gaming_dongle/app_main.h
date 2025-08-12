/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_MAIN_H_
#define _APP_MAIN_H_

#include <stdint.h>

#include "app_link_util.h"
#include "app_device.h"
#include "voice_prompt.h"
#include "engage.h"
#include "remote.h"
#include "charger_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_MAIN App Main
  * @brief Main entry function for audio sample application.
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_MAIN_Exported_Macros App Main Macros
    * @{
    */
#define UART_BUD_RX     (P3_0)

#define RWS_PRIMARY_VALID_OK             0x01
#define RWS_SECONDARY_VALID_OK           0x02

/** End of APP_MAIN_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_MAIN_Exported_Types App Main Types
    * @{
    */

/** @brief Bud location */
typedef enum
{
    BUD_LOC_UNKNOWN     = 0x00,
    BUD_LOC_IN_CASE     = 0x01,
    BUD_LOC_IN_AIR      = 0x02,
    BUD_LOC_IN_EAR      = 0x03,
} T_BUD_LOCATION;

/**  @brief  App define global app data structure */
typedef struct
{
    T_APP_BR_LINK               *br_link;
    uint16_t                    external_mcu_mtu;
    uint8_t                     local_loc;                 /**< local bud location */
    uint8_t                     local_batt_level;          /**< local battery level */
    uint8_t                     remote_batt_level;         /**< remote battery level */
    uint8_t                     case_battery;              /**< rtk case battery level*/

    uint8_t                     factory_addr[6];            /**< local factory address */
    uint8_t                     avrcp_play_status;
    uint8_t                     hall_switch_status;         /**< hall switch status */

    T_APP_DEVICE_STATE          device_state;
    uint8_t                     remote_session_state;
    bool                        key0_wake_up;
    bool                        peri_wake_up;
    bool                        combine_poweron_key_wake_up;

    uint8_t                     usb_audio_path;

    uint8_t ble_ota_support: 1;// TTS and OTA will use it
    uint8_t tts_support : 1;
    uint8_t gfps_support : 1;
    uint8_t ama_support : 1;
    uint8_t bisto_support : 1;
    uint8_t xiaoai_support : 1;
    uint8_t anc_support : 1;
    uint8_t reserved : 1;

    bool                        power_off_cause_in_box;

    bool                        ignore_led_mode;
    bool                        power_on_by_cmd;

    uint8_t                     sd_playback_switch;
    uint8_t                     disallow_charging_led;

    void                        *line_in_track_handle;

    bool                        bt_is_ready;
    bool                        force_enter_dut_mode_when_acl_connected;
} T_APP_DB;
/** End of APP_MAIN_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_MAIN_Exported_Variables App Main Variables
    * @{
    */
extern T_APP_DB app_db;

extern void *audio_evt_queue_handle;
extern void *audio_io_queue_handle;
/** End of APP_MAIN_Exported_Variables
    * @}
    */

/** End of APP_MAIN
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_MAIN_H_ */
