/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __APP_LED_CTL_H_
#define __APP_LED_CTL_H_

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdbool.h>
#include <stdint.h>
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "app_io_msg.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup LED_MODULE Led Module
  * @brief modulization for sleep led module for simple application usage.
  * @{
  */

/*============================================================================*
  *                                   Types
  *============================================================================*/
/** @defgroup LED_MODULE_Exported_Types Led Module Exported Types
  * @{
  */
/** @} */ /* End of group LED_MODULE_Exported_Types */

/*============================================================================*
  *                                Functions
  *============================================================================*/
/** @defgroup LED_MODULE_Exported_Functions Led Module Exported Functions
    * @{
    */
/**
 * @brief Rseet the Sleep LED registers to their default value.
 */
extern void app_led_reset(void);

void app_led_set_custom_config(uint8_t led, uint8_t *config);

/** @} */ /* End of group LED_MODULE_Exported_Functions */

/** @} */ /* End of group LED_MODULE */

typedef enum t_app_led_mode
{
    //public mode
    APP_LED_MODE_ALL_OFF                = 0x00,
    APP_LED_MODE_ALL_ON                 = 0x01,
    APP_LED_MODE_POWER_OFF              = 0x02,
    APP_LED_MODE_POWER_ON               = 0x03,

    APP_LED_MODE_FACTORY_RESET          = 0x04,

    APP_LED_MODE_BT_BIS                 = 0x05,
    APP_LED_MODE_BT_CIS                 = 0x06,
    APP_LED_MODE_BT_LEGACY              = 0x07,

    APP_LED_MODE_BROADCASTING           = 0x08,
    APP_LED_MODE_BST_IDLE               = 0x09,

    APP_LED_MODE_UNICAST_IDLE           = 0x0a,
    APP_LED_MODE_UNICAST_BG_SCAN        = 0x0b,
    APP_LED_MODE_UNICAST_PAIRING        = 0x0c,
    APP_LED_MODE_UNICAST_CONNECTED      = 0x0d,

    //gaming mode
    APP_LED_MODE_LINK1_STANDBY          = 0x0e,
    APP_LED_MODE_LINK1_PAIRING          = 0x0f,
    APP_LED_MODE_LINK1_RECONNECTING     = 0x10,
    APP_LED_MODE_LINK1_PAIRED           = 0x11,
    APP_LED_MODE_LINK1_CONNECTED        = 0x12,

    APP_LED_MODE_LINK2_STANDBY          = 0x13,
    APP_LED_MODE_LINK2_PAIRING          = 0x14,
    APP_LED_MODE_LINK2_RECONNECTING     = 0x15,
    APP_LED_MODE_LINK2_PAIRED           = 0x16,
    APP_LED_MODE_LINK2_CONNECTED        = 0x17,

    //teams mode
#ifdef LEGACY_BT_GENERAL
    APP_LED_MODE_ESTABLISH_CONNECTION_WITH_TEAMS_APP = 0x18,
    APP_LED_MODE_FAIL_OR_NO_CONNECTION_WITH_TEAMS = 0x19,
    APP_LED_MODE_TEAMS_BUTTON_IS_PRESSED_BUT_NO_CONNECTION = 0x1a,
    APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE = 0x1b,
    APP_LED_MODE_TEAMS_MEETING_HAS_STARTED_OR_INCOMING_MEETING_NOTIFICATION = 0x1c,
    APP_LED_MODE_TEAMS_IN_MEETING = 0x1d,
    APP_LED_MODE_TEAMS_VOICE_MAIL_NOTIFICATION_OR_MISSED_CALL_NOTIFICATION = 0x1e,
    APP_LED_MODE_INVOKING_VOICE_SKILLS_IN_TEAMS_APP = 0x1f,
    APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS = 0x20,
    APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT = 0x21,
    APP_LED_MODE_TEAMS_IN_CALL = 0x22,
    APP_LED_MODE_TEAMS_STATUS_MUTE = 0x23,
    APP_LED_MODE_TEAMS_STATUS_UNMUTE = 0x24,

    APP_LED_MODE_DISCONNECTED = 0x25,
    APP_LED_MODE_CONNECTED = 0x26,
    APP_LED_MODE_PARING = 0x27,
#endif
    APP_LED_MODE_COUNT_NUM,

} T_APP_LED_MODE;

typedef enum
{
    LED_TIMER_ID_CONNECT_WITH_TEAMS = 0x01,
    LED_TIMER_ID_FLASH_ON = 0x02,
    LED_TIMER_ID_FLASH_OFF = 0x03,
    LED_TIMER_ID_PULSE_CONTINUE = 0x04,
    LED_TIMER_ID_DEVICE_DISCONNECTED = 0x05,
    LED_TIMER_ID_DEVICE_PAIRING = 0x06,
} T_LED_TIMER_ID;

#ifdef LEGACY_BT_GENERAL
void app_stop_flash_on_and_off_timer(void);
void app_stop_pulse_white_timer(void);
void app_stop_missed_call_timer(void);
bool app_led_set_before_white(void);
bool app_led_control_timer_connect_with_teams_started(void);
#endif

void app_led_init(void);
void app_led_set_mode(T_APP_LED_MODE mode);
bool led_is_all_keep_off(void);

#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* __APP_LED_CTL_H_ */
