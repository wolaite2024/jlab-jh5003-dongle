/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_SASS_POLICY_H_
#define _APP_SASS_POLICY_H_

#include <stdint.h>

#if CONFIG_REALTEK_GFPS_SASS_SUPPORT
#include "gfps_sass_conn_status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SASS_PAGESCAN_WINDOW        0x12
#define SASS_PAGESCAN_INTERVAL      0x400

#define SWITCH_RET_OK               0x00
#define SWITCH_RET_FAILED           0x01
#define SWITCH_RET_REDUNDANT        0x02

/**
 * @brief Multipoint switching preference flag
 * Bit 0 (MSB): A2DP vs A2DP (default 0)
 * Bit 1: HFP vs HFP (default 0)
 * Bit 2: A2DP vs HFP (default 0)
 * Bit 3: HFP vs A2DP (default 1)
*/
#define SASS_A2DP_A2DP 0x0001
#define SASS_SCO_SCO   0x0002
#define SASS_A2DP_SCO  0x0004
#define SASS_SCO_A2DP  0x0008
#define SASS_A2DP_VA   0x0010
#define SASS_SCO_VA    0x0020
#define SASS_VA_A2DP   0x0040
#define SASS_VA_SCO    0x0080
#define SASS_VA_VA     0x0100

typedef enum
{
    SASS_PREEMPTIVE_FEATURE_BIT_SET,
    SASS_PREEMPTIVE_FEATURE_BIT_GET,
    SASS_LINK_SWITCH,
    SASS_SWITCH_BACK,
    SASS_MULTILINK_ON_OFF,
} T_SASS_CMD;

void app_sass_policy_set_capability(uint8_t *addr);
void app_sass_policy_set_multipoint_state(uint8_t enable);
void app_sass_policy_set_switch_preference(uint8_t flag);
uint8_t app_sass_policy_get_switch_preference(void);
void app_sass_policy_set_advanced_switching_setting(uint8_t flag);
uint8_t app_sass_policy_get_advanced_switching_setting(void);
uint8_t app_sass_policy_switch_active_audio_source(uint8_t *bd_addr, uint8_t switch_flag,
                                                   bool self);
void app_sass_policy_switch_back(uint8_t event);
void app_sass_policy_initiated_connection(uint8_t *addr, bool triggered_by_audio_switch);
void app_sass_policy_init(void);
void app_sass_policy_reset(void);
void app_sass_policy_sync_conn_bit_map(uint8_t bitmap);
uint8_t app_sass_policy_get_conn_bit_map(void);
uint8_t app_sass_policy_get_disc_link(void);
void app_sass_policy_link_back_end(void);
void app_sass_policy_profile_conn_handle(uint8_t id);
bool app_sass_policy_is_sass_device(uint8_t *addr);
bool app_sass_policy_is_target_drop_device(uint8_t idx);
#if CONFIG_REALTEK_GFPS_SASS_SUPPORT
T_SASS_CONN_STATE app_sass_policy_get_connection_state(void);
#endif
bool app_sass_policy_get_original_enable_multi_link(void);
uint8_t app_sass_get_available_connection_num(void);

#ifdef __cplusplus
}
#endif

#endif

