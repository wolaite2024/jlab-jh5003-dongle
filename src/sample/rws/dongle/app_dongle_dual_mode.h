/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_DONGLE_BT_POLICY_H_
#define _APP_DONGLE_BT_POLICY_H_

#include <stdint.h>
#include "app_gaming_sync.h"
#include "audio_track.h"
#include "app_link_util.h"
#include "app_audio_policy.h"

typedef enum
{
    APP_REMOTE_MSG_SYNC_DONGLE_RF_MODE                    = 0x00,
    APP_REMOTE_MSG_SYNC_HEADSET_STATUS                    = 0x01,
    APP_REMOTE_MSG_DONGLE_MAX_MSG_NUM,
} T_APP_DONGLE_REMOTE_MSG;

typedef enum
{
    DONGLE_EVENT_STANDBY,
    DONGLE_EVENT_LINKBACK,
    DONGLE_EVENT_PAIRING,
} T_APP_DONGLE_EVENT;

typedef enum
{
    DONGLE_STATE_STANDBY,
    DONGLE_STATE_LINKBACK,
    DONGLE_STATE_PAIRING,
} T_APP_DONGLE_STATE;

typedef enum
{
    DONGLE_RF_MODE_24G,
    DONGLE_RF_MODE_BT,
    DONGLE_RF_MODE_24G_BT,
} T_APP_DONGLE_RF_MODE;

typedef enum
{
    DONGLE_PAIRING_MODE_NONE,
    DONGLE_PAIRING_MODE_24G,
    DONGLE_PAIRING_MODE_BT,
    DONGLE_PAIRING_MODE_ALL,
} T_APP_DONGLE_PAIRING_MODE;

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
typedef enum
{
    EVENT_2_4G_STREAM_START = 0x00,
    EVENT_2_4G_STREAM_STOP  = 0x01,
    EVENT_2_4G_CALL_ACTIVE  = 0x02,
    EVENT_2_4G_CALL_END     = 0x03,
    EVENT_HFP_CALL_ACTIVE   = 0x04,
    EVENT_HFP_CALL_END      = 0x05,
    EVENT_2_4G_DISCONNECTED = 0x06,
    EVENT_ALL               = 0xFF,
} T_APP_DONGLE_PREEMPT_EVENT;

typedef enum
{
    ACTION_2_4G_STREAM_PREEMPT_A2DP_STREAM      = 0x00,
    ACTION_2_4G_STREAM_STOP_RESUME_A2DP_STREAM  = 0x01,
    ACTION_2_4G_CALL_ACTIVE_PREEMPT_HFP_CALL    = 0x02,
    ACTION_2_4G_CALL_END_RESUME_HFP_CALL        = 0x03,
    ACTION_ALL = 0xFF,
} T_APP_DONGLE_PREEMPT_ACTION;
#endif

#if F_APP_GAMING_WIRED_MODE_HANDLE
typedef enum
{
    EVENT_LINEIN_PLUG_IN   = 0x00,
    EVENT_LINEIN_PLUG_OUT  = 0x01,
    EVENT_USB_AUDIO_START  = 0x02,
    EVENT_USB_AUDIO_STOP   = 0x03,
} T_APP_DONGLE_WIRED_MODE_EVENT;
#endif

typedef struct
{
    uint8_t disc_phone : 1;
    uint8_t disc_dongle : 1;
    uint8_t trigger_bt_pairing : 1;
    uint8_t trigger_dongle_pairing : 1;
    uint8_t linkback_dongle : 1;
    uint8_t linkback_phone : 1;
    uint8_t rsv : 2;
} T_DONGLE_LINKBACK_STATUS;

typedef struct
{
    uint8_t start_linkback_when_shutdown : 1;
    uint8_t linkback_timeout : 1;
    uint8_t enable_pairing : 1;
    uint8_t key_missing : 1;
    uint8_t enter_pairing_after_mode_switch : 1;
    uint8_t ignore_radio_mode_idle_when_shutdown : 1;
    uint8_t linkback_is_canceld : 1;
    uint8_t disallow_dongle_a2dp : 1;

    uint8_t low_rssi_disconnet_dongle : 1;
    uint8_t switch_pairing_triggered : 1;
    uint8_t linkback_src_after_disc : 1;
    uint8_t keep_pairing_mode_type : 2;
    uint8_t rsv : 3;

    T_DONGLE_LINKBACK_STATUS linkback_status;
} T_DONGLE_CONTROL_DATA;

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
typedef enum
{
    HEADSET_POWER_OFFING,
    HEADSET_CONN_EVENT_LEGACY_CONNECTED,
    HEADSET_CONN_EVENT_LEGACY_DISCONNECTED,
    HEADSET_CONN_EVENT_BLE_CONNECTED,
    HEADSET_CONN_EVENT_BLE_DISCONNECTED,
} T_HEADSET_CONN_EVENT;
#endif

extern T_DONGLE_CONTROL_DATA dongle_ctrl_data;

#if F_APP_DONGLE_MULTI_PAIRING
void app_dongle_switch_pairing_mode(void);
#endif

void app_dongle_sync_headset_status(void);

#if (F_APP_ERWS_SUPPORT == 0)
bool app_dongle_dual_mode_linkback(void);
#endif
void app_dongle_shutdown_end_check(void);
void app_dongle_state_machine(T_APP_DONGLE_EVENT event);
void app_dongle_dual_mode_switch(void);

void app_dongle_handle_rtk_adv(bool enable_adv);
uint8_t app_dongle_get_phone_bond_num(void);
uint8_t app_dongle_get_dongle_num(void);
T_APP_DONGLE_STATE app_dongle_get_state(void);
void app_dongle_enable_tx_3M(bool enable_3M);
void app_dongle_handle_ble_disconnected(uint8_t *bd_addr);
void app_dongle_handle_ble_connected(uint8_t *bd_addr);
uint16_t app_dongle_get_gaming_latency(void);
bool app_dongle_set_gaming_latency(T_AUDIO_TRACK_HANDLE p_handle, uint16_t latency_value);
void app_dongle_adjust_gaming_latency(void);
/* enable/disable rtk common adv coexist with legacy gaming */
void app_dongle_set_common_adv_coexist_in_legacy_gaming(bool enable);
void app_dongle_call_status_handler(T_APP_CALL_STATUS call_status, uint8_t idx);


/**
    * @brief        get power on vp by dongle rf mode
    * @pram         void
    * @return       power on vp type
    */
T_APP_AUDIO_TONE_TYPE app_dongle_get_power_on_vp(void);

/**
    * @brief        switch dongle rf mode
    * @pram         mode: dongle rf mode
    * @return       true if set mode success
    */
bool app_dongle_switch_rf_mode(T_APP_DONGLE_RF_MODE mode);

/**
    * @brief        check extra action when user wants to trigger mmi action
    * @pram         action: mmi action
    * @return       void
    */
void app_dongle_check_mmi_action(uint8_t action);

void app_dongle_a2dp_start_handle(void);

/**
    * @brief  dongle is streaming or not
    * @param  void
    * @return true/false
    */
bool app_dongle_is_streaming(void);

/**
    * @brief        check if lea adv can start
    * @pram         void
    * @return       true lea adv can start
    */
bool app_dongle_lea_adv_start_check(void);

void app_dongle_dual_mode_init(void);

void app_dongle_exit_pairing_mode(void);
bool app_dongle_enter_pairing_mode(T_APP_DONGLE_PAIRING_MODE mode);

/**
    * @brief        clear dongle status
    * @pram         void
    * @return       void
    */
void app_dongle_clear_dongle_status(void);

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
bool app_dongle_disallow_bt_streaming(void);
void app_dongle_le_audio_handle_mic(bool mic_enable);
void app_dongle_handle_hfp_call(void);
void app_dongle_handle_stream_preempt(T_APP_DONGLE_PREEMPT_EVENT event);
bool app_dongle_get_wait_cis_disc(void);
void app_dongle_handle_cis_disconnected(void);
#endif

#if F_APP_GAMING_WIRED_MODE_HANDLE
void app_dongle_wired_mode_handle(T_APP_DONGLE_WIRED_MODE_EVENT event);
#endif

void app_dongle_lea_handle_dongle_status(void);
T_PHONE_STREAM_STATUS app_dongle_get_phone_status(void);
void app_dongle_streaming_handle(bool streaming);
bool app_dongle_send_fix_channel_data(uint8_t *addr, uint16_t cid, uint8_t *p_data, uint16_t length,
                                      bool flushable);
void app_dongle_streaming_detect(void);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
void app_dongle_update_headset_conn_status(T_HEADSET_CONN_EVENT event);
#endif
#endif

