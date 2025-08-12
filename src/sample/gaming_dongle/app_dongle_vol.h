/**
*****************************************************************************************
*     Copyright (C) 2021 Realtek Semiconductor Corporation.
*****************************************************************************************
  * @file
  * @brief
  * @details
  * @author
  * @date
  * @version
  ***************************************************************************************
  * @attention
  ***************************************************************************************
  */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/

#ifndef _APP_DONGLE_VOLUME_H_
#define _APP_DONGLE_VOLUME_H_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include "string.h"
#include "trace.h"

/*============================================================================*
 *                         Macros
 *============================================================================*/


/*============================================================================*
 *                         Types
 *============================================================================*/
typedef enum
{
    DONGLE_HOST_AUX,                //default
    DONGLE_HOST_HID_INVALID,        //hid send fail, like PS
    DONGLE_HOST_HID_NO_FEEDBACK,    //hid send ok, no host vol feedback, like android
    DONGLE_HOST_HID_FEEDBACK,       //hid send ok, host vol feedback, like pc
} T_DONGLE_HOST_TYPE;

typedef enum
{
    DONGLE_HOST_STATE_INIT,
    DONGLE_HOST_STATE_CHECKING,
    DONGLE_HOST_STATE_CHECKED,
} T_DONGLE_HOST_STATE;

typedef enum
{
    VOL_TIMER_ID_A2U_SET_VOL        = 0x01,         //timeout means no host vol feedback, android pc both may happen
    VOL_TIMER_ID_U2A_SET_VOL        = 0x02,         //wait timeout to proc u2a set vol
    VOL_TIMER_ID_HID_SEND           = 0x03,         //hid send interval, wait timeout to send next hid cmd
    VOL_TIMER_ID_HID_VALID_CHECK    = 0x04,         //hid send start this timer, when timeout means host hid not support
    VOL_TIMER_ID_A2U_PROC_INTERVAL  = 0x05,         //wait timeout to proc feedback, in case one hid cmd get two or more feedback
    VOL_TIMER_ID_INIT_U2A_VOL       = 0x06,         //timeout means no u2a vol when usb init, send incre cmd to get host vol
    VOL_TIMER_ID_HID_VALID_CHECK_BEGIN = 0x07,      //timeout send hid valid check
    VOL_TIMER_ID_U2A_FEEDBACK_CHECK = 0x08,         //timeout clear adjusting flag
    VOL_TIMER_ID_HID_RELEASE_CMD    = 0x09,
} T_VOL_TIMER_ID;

/*============================================================================*
 *                         Functions
 *============================================================================*/


void app_dongle_handle_u2a_set_vol(uint8_t vol);

void app_dongle_handle_a2u_set_vol(uint8_t vol, bool mute);

void host_type_check_start(void);

uint8_t set_dongle_host_state(uint8_t state);

uint8_t get_dongle_host_vol(void);

uint8_t get_dongle_host_mute(void);

void stop_usb_hid_check_timer(void);

void app_dongle_vol_init(void);

void app_dongle_set_avrcp_vol(void);

void update_dongle_spk_mute_status(uint16_t mute);

void app_dongle_set_host_type(uint8_t host_type);

void app_dongle_host_type_check(uint8_t type);

#ifdef __cplusplus
}
#endif

#endif  // _APP_USB_VOLUME_H_

