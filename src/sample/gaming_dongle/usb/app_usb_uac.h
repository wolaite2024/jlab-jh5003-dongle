/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_USB_AUDIO_H_
#define _APP_USB_AUDIO_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_msg.h"
#include "usb_audio_stream.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* uac control status due to headset connected with phone */
typedef struct
{
    bool delay_usb_tx_when_bt_play;
} T_UAC_CONTROL_STATUS;

#define UAC_DS_ACTIVE       0x01
#define UAC_US_ACTIVE       0x02

typedef struct
{
    void (*dongle_uac_cback_chnl_state)(uint8_t chnl);
    void (*dongle_uac_cback_stream_state)(uint8_t state);
    void (*dongle_uac_cback_spk_vol)(uint16_t vol);
    void (*dongle_uac_cback_mic_vol)(uint16_t vol);
} DONGLE_UAC_CB_F;

void app_usb_uac_init(void);

void app_usb_delay_usb_tx_start(void);
void app_usb_silence_stream_detect_stop(void);
void app_usb_uac_2_data_trans_ds_handle(void);

void app_usb_uac_ds_stream_data_stop_handle(void);

void app_usb_uac_check_usb_stream(void);
void app_usb_uac_clear_headset_status(void);

bool uac_get_pipe_dac_gain(uint8_t level, uint16_t *gain);
void uac_handle_spk_vol_chg_msg_handle(uint16_t vol);
void uac2_handle_spk_vol_chg_msg_handle(uint16_t vol);
void uac_handle_spk_mute_chg_msg(uint16_t vol);
void uac2_handle_spk_mute_chg_msg_handle(uint16_t vol);

void app_usb_uac_mic_vol_chg_handle(uint16_t vol);
void app_usb_uac_register_cbs(DONGLE_UAC_CB_F *p_func);
void app_usb_uac_unregister_cbs(void);

bool app_usb_stream_xmit_out(uint8_t *data, uint16_t len, T_UAC_LABEL uac_label);
void app_usb_set_stream_state_to_tx(bool streaming);

#if ENABLE_UAC2
#if (LEA_BIS_DUAL_UAC_SUPPORT == 1)
bool uac1_cback_ds_data_trans(uint8_t *data, uint16_t len);
bool uac2_cback_ds_data_trans(uint8_t *data, uint16_t len);
#else
bool uac1_cback_ds_data_trans_np(uint8_t *data, uint16_t len, T_UAC_LABEL uac_label);
bool uac2_cback_ds_data_trans_np(uint8_t *data, uint16_t len, T_UAC_LABEL uac_label);
#endif
#endif
bool app_usb_uac_cback_msg_ds_data_trans(uint8_t *data, uint16_t length, T_UAC_LABEL uac_label);

void app_usb_uac_cback_msg_us_buff_change(uint16_t data_space, uint16_t free_space,
                                          uint16_t data_out);

void app_usb_uac_data_trans_ds_handle(void);
#if ENABLE_UAC2
void app_usb_uac_2_data_trans_ds_handle(void);
#endif

void app_handle_uac_active(uint32_t param);
void app_handle_uac_inactive(uint32_t param);
void app_handle_uac_2_inactive(uint32_t param);

uint8_t app_get_usb_ds_state(void);
uint8_t app_get_usb_us_state(void);
uint8_t get_usb_uac1_state(void);
uint8_t get_usb_uac2_state(void);
uint8_t app_get_usb_stream_ready_to_tx(void);

extern T_UAC_CONTROL_STATUS uac_ctrl_status;
extern uint8_t dongle_chnl_state;

void app_usb_uac_stream_stop_handle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_USB_AUDIO_H_ */
