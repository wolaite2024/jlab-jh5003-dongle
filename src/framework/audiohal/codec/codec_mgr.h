/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _CODEC_MGR_H_
#define _CODEC_MGR_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_codec_mgr_state
{
    CODEC_MGR_STATE_POWER_OFF       = 0x00,
    CODEC_MGR_STATE_WAIT_POWER_OFF  = 0x01,
    CODEC_MGR_STATE_MUTED           = 0x02,
    CODEC_MGR_STATE_WAIT_MUTE       = 0x03,
    CODEC_MGR_STATE_ACTIVED         = 0x04,
    CODEC_MGR_STATE_WAIT_ACTIVE     = 0x05,
} T_CODEC_MGR_STATE;

typedef enum t_codec_mgr_session_state
{
    CODEC_MGR_SESSION_STATE_DISABLED   = 0x00,
    CODEC_MGR_SESSION_STATE_DISABLING  = 0x01,
    CODEC_MGR_SESSION_STATE_ENABLED    = 0x02,
    CODEC_MGR_SESSION_STATE_ENABLING   = 0x03,
} T_CODEC_MGR_SESSION_STATE;

typedef enum t_codec_mgr_event
{
    CODEC_MGR_EVENT_ENABLE,
    CODEC_MGR_EVENT_ENABLE_COMPLETE,
    CODEC_MGR_EVENT_MUTE,
    CODEC_MGR_EVENT_MUTE_COMPLETE,
    CODEC_MGR_EVENT_DISABLE,
    CODEC_MGR_EVENT_DISABLE_COMPLETE,
    CODEC_MGR_EVENT_SIDETONE_ENABLED,
    CODEC_MGR_EVENT_SIDETONE_DISABLED,
} T_CODEC_MGR_EVENT;

typedef struct t_codec_mgr_event_param
{
    void *context;
    uint8_t state;
} T_CODEC_MGR_EVENT_PARAM;

typedef void *T_CODEC_MGR_SESSION_HANDLE;

typedef bool (*P_CODEC_MGR_CBACK)(T_CODEC_MGR_EVENT event);

bool codec_mgr_init(P_CODEC_MGR_CBACK cback);

void codec_mgr_deinit(void);

T_CODEC_MGR_SESSION_HANDLE codec_mgr_session_create(T_AUDIO_CATEGORY category,
                                                    uint32_t         dac_sample_rate,
                                                    uint32_t         adc_sample_rate,
                                                    uint8_t          dac_gain_level,
                                                    uint8_t          adc_gain_level,
                                                    void            *context);

bool codec_mgr_session_destroy(T_CODEC_MGR_SESSION_HANDLE handle);

T_CODEC_MGR_STATE codec_mgr_get_state(void);

void codec_mgr_adc_gain_set(T_CODEC_MGR_SESSION_HANDLE handle,
                            uint8_t level);

void codec_mgr_adc_gain_mute(T_CODEC_MGR_SESSION_HANDLE handle);

void codec_mgr_dac_gain_set(T_CODEC_MGR_SESSION_HANDLE handle,
                            uint8_t level);

bool codec_mgr_session_enable(T_CODEC_MGR_SESSION_HANDLE handle);

bool codec_mgr_session_disable(T_CODEC_MGR_SESSION_HANDLE handle);

T_CODEC_MGR_SESSION_STATE codec_mgr_session_state_get(T_CODEC_MGR_SESSION_HANDLE handle);

void codec_mgr_power_off(void);

void codec_mgr_bias_setting(void);

bool codec_mgr_sidetone_set(int16_t gain, uint8_t level, uint8_t enable);

void codec_mgr_sidetone_gain_set(int16_t gain);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CODEC_MGR_H_ */
