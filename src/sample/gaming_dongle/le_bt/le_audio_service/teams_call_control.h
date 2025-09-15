#ifndef _APP_CALL_CONTROL_H_
#define _APP_CALL_CONTROL_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "profile_server.h"
#include "ccp_mgr.h"
#include "ble_audio.h"

/* FIXME: HID report map not support HOLD YET */
#define USB_HID_HOLD_SUPPORT        1

typedef struct
{
    uint16_t hook_switch    : 1;    /* 0:terminate 1:accept */
    uint16_t line_busy      : 1;
    uint16_t line           : 1;
    uint16_t mute           : 1;    /* 0:un-mute 1:mute */
    uint16_t flash          : 1;
#if (USB_HID_HOLD_SUPPORT == 1)
    uint16_t hold           : 1;
    uint16_t redial         : 1;
#else
    uint16_t redial         : 1;
    uint16_t speed_dial     : 1;
#endif
    uint16_t key_pad        : 4;
    uint16_t button         : 1;    /* 1:reject */
    uint16_t rsv            : 4;
} T_TELEPHONY_HID_INPUT;

#if LE_AUDIO_CCP_SERVER_SUPPORT

void teams_call_control_init(T_SERVER_ID ccp_id);
uint16_t teams_call_handle_ccp_msg(T_LE_AUDIO_MSG msg, void *buf);
T_APP_RESULT teams_handle_ccp_op(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val);
void teams_call_handle_telephony_msg(uint16_t msg);
uint16_t le_tbs_get_enabled_cccd(void);
void le_tbs_handle_usb_upstream(bool enable);
void teams_handle_mics_msg(uint8_t *bd_addr, uint8_t mute);
bool le_tbs_get_mute_bit(void);

bool le_tbs_handle_server_msg(T_LE_AUDIO_MSG msg, void *buf);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
