/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_MMI_H_
#define _APP_MMI_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_MMI App MMI
  * @brief App MMI
  * @{
  */

#define DONGLE_LEGACY_IDLE              0x00
#define DONGLE_LEGACY_CONNECT           0x01     /*connecting/pairing/connected*/
#define DONGLE_BIS_IDLE                 0x02
#define DONGLE_BIS_STREAMING            0x03
#define DONGLE_CIS_IDLE                 0x04
#define DONGLE_CIS_CONNECT              0x05     /*connecting/pairing/connected*/

/**  @brief  action ID parameter for @ref app_mmi_handle_action */
typedef enum
{
    MMI_NULL                    = 0x00,

    MMI_DEV_MIC_MUTE            = 0x07,
    MMI_DEV_MIC_UNMUTE          = 0x08,
    MMI_DEV_MIC_VOL_UP          = 0x15,
    MMI_DEV_MIC_VOL_DOWN        = 0x16,

    MMI_DEV_POWER_ON            = 0x54,
    MMI_DEV_POWER_OFF           = 0x56,
    MMI_DEV_FACTORY_RESET       = 0x58,
    MMI_DEV_BT_MODE_SWITCH      = 0x59,

    MMI_BT_MODE_LEGACY          = 0x60,
    MMI_BT_LEGACY_LINK1_SCAN          = 0x61,
    MMI_BT_LEGACY_LINK1_RECONNECT     = 0x62,
    MMI_BT_LEGACY_LINK1_DISCONNECT    = 0x63,
    MMI_BT_LEGACY_LINK2_SCAN          = 0x64,
    MMI_BT_LEGACY_LINK2_RECONNECT     = 0x65,
    MMI_BT_LEGACY_LINK2_DISCONNECT    = 0x66,

    MMI_BT_MODE_BIS             = 0x70,
    MMI_BT_BIS_START            = 0x71,
    MMI_BT_BIS_STOP             = 0x72,

    MMI_BT_MODE_CIS             = 0x80,
    MMI_BT_CIS_SCAN             = 0x81,
    MMI_BT_CIS_RECONNECT        = 0x82,
    MMI_BT_CIS_DISCONNECT       = 0x83,

    MMI_DUT_TEST_MODE           = 0xA1,
    MMI_TOTAL
} T_MMI_ACTION;

typedef enum
{
    DONGLE_BT_MODE_LEGACY = 0,
    DONGLE_BT_MODE_BIS,
    DONGLE_BT_MODE_CIS,
    DONGLE_BT_MODE_MAX
} T_DONGLE_BT_MODE;

/**
    * @brief  App need to process key message are handle in this function.
    *         User trigger the specific key process, then bbpro would send le advertising data.
    * @param  action key event
    * @return void
    */
void app_mmi_handle_action(uint8_t action);

/**
    * @brief  In rws condition, MMI need to sync or not.
    * @param  action key event
    * @return void
    */
bool app_mmi_is_local_execute(uint8_t action);

void app_mmi_init(void);

void app_bt_mode_init(void);

/**
 * @brief dongle current BT mode
 *
 * @return uint8_t :refs T_DONGLE_BT_MODE
 */
uint8_t app_get_cur_bt_mode(void);

/**
 * @brief when BT mode change, gap stop timer
 *
 */
void bt_mode_change_app_stop_timer(void);

__weak bool app_audio_tone_play(uint8_t index, bool flush, bool relay);
uint8_t app_dongle_get_status(void);

/** End of APP_MMI
* @}
*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_MMI_H_ */
