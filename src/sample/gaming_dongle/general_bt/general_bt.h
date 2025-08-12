/*
 *  Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 */
#ifndef __GENERAL_BT_H__
#define __GENERAL_BT_H__

#include "btm.h"

#ifdef __cplusplus
extern "C" {
#endif

enum general_cfg_type
{
    GENERAL_CFG_LOW_LATENCY = 0,
    GENERAL_CFG_AUTOPAIR_ENABLE,
    GENERAL_CFG_AUTOPAIR_GENERAL,
    GENERAL_CFG_AUTOPAIR_TIMEOUT,
    GENERAL_CFG_AUTOPAIR_RSSI,
};

enum general_led_state
{
    GENERAL_LED_BT_IDLE,
    GENERAL_LED_BT_CONNECTING,
    GENERAL_LED_BT_RECONNECTING,
    GENERAL_LED_BT_CONNECTED,
};

struct general_ev_device_connected
{
    uint8_t id;
    uint8_t reserved;
    uint8_t ba[6];
};

struct general_ev_device_disconnected
{
    uint8_t id;
    uint8_t reserved;
    uint8_t ba[6];
};

struct general_ev_profile_connected
{
    uint8_t id;
    uint8_t reserved;
    uint8_t ba[6];
    uint32_t profile; /* See app_link_util.h */
    uint32_t profile_data;
};

struct general_ev_profile_disconnected
{
    uint8_t id;
    uint8_t reserved;
    uint8_t ba[6];
    uint32_t profile; /* See app_link_util.h */
    uint32_t profile_data;
};

struct general_ev_device_found
{
    T_BT_EVENT_PARAM_INQUIRY_RESULT result;
};

struct general_ev_led_state
{
    uint8_t id;
    uint8_t led_state;
};

int general_bt_init(void);
//void gaming_set_cfg(uint8_t type, uint8_t *value, uint8_t len);
int general_bt_enable(void);
void general_bt_disable(void);
int general_bt_connect(uint8_t id, uint8_t *bdaddr);
int general_bt_disconnect_by_bdaddr(uint8_t *bdaddr);
int general_bt_disconnect_by_id(uint8_t id);
void general_bt_remove_bond_by_bdaddr(uint8_t *bdaddr);
void general_bt_remove_bond_by_id(uint8_t id);
int general_bt_start_discovery(void);
int general_bt_stop_discovery(void);
int general_bt_get_bond_bdaddr(uint8_t id, uint8_t *bdaddr);
bool general_bt_is_connected(void);
void general_bt_profile_conn_cmpl(uint8_t *bd_addr, uint32_t profile_mask);
void general_bt_profile_disconn_cmpl(uint8_t *bd_addr, uint16_t cause,
                                     uint32_t profile_mask);
void general_bt_try_start_stream(void);
void general_bt_set_volume(uint32_t vol, uint8_t mute);
void general_bt_audio_capture_switch(bool active);
bool general_bt_is_idle(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif /* __GAMING_BT_H__ */
