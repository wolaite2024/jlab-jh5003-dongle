/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _NOTIFICATION_H_
#define _NOTIFICATION_H_

#include <stdbool.h>
#include <stdint.h>

#include "tts.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NOTIFICATION_INVALID_INDEX  0xFF

#define NOTIFICATION_TYPE_NONE      0x00
#define NOTIFICATION_TYPE_RINGTONE  0x01
#define NOTIFICATION_TYPE_VP        0x02
#define NOTIFICATION_TYPE_TTS       0x03

typedef enum
{
    NOTIFICATION_STATE_IDLE     = 0x00,
    NOTIFICATION_STATE_RINGTONE = 0x01,
    NOTIFICATION_STATE_VP       = 0x02,
    NOTIFICATION_STATE_TTS      = 0x03,
    NOTIFICATION_STATE_RELAY    = 0x04,
} T_NOTIFICATION_STATE;

bool notification_init(uint16_t pool_size);

void notification_deinit(void);

uint8_t notification_mode_get(uint8_t type);

bool notification_mode_set(uint8_t type, uint8_t mode);

uint8_t notification_volume_max_get(uint8_t type);

bool notification_volume_max_set(uint8_t type, uint8_t volume);

uint8_t notification_volume_min_get(uint8_t type);

bool notification_volume_min_set(uint8_t type, uint8_t volume);

uint8_t notification_volume_get(uint8_t type);

bool notification_volume_set(uint8_t type, uint8_t volume);

bool notification_volume_mute(uint8_t type);

bool notification_volume_unmute(uint8_t type);

float notification_volume_balance_get(uint8_t type);

bool notification_volume_balance_set(uint8_t type, float scale);

bool notification_push(uint8_t type, uint8_t index, uint8_t language,
                       void *tts_handle, bool relay, uint32_t addr, uint32_t len);

bool notification_stop(uint8_t type, void *handle);

bool notification_write(uint8_t type, void *tts_handle, uint8_t *buf, uint16_t len);

bool notification_flush(uint8_t type, bool relay);

bool notification_cancel(uint8_t type, uint8_t index, bool relay);

T_TTS_HANDLE notification_tts_alloc(uint32_t length, uint16_t frame_cnt, uint32_t cfg);

bool notification_tts_free(T_TTS_HANDLE handle);

void notification_set_sync_dsp_para(bool sycn_type);

void notification_set_mix_dsp_para(uint32_t bt_clk);

T_NOTIFICATION_STATE notification_get_state(void);

uint8_t notification_get_pending_elem_num(void);

uint8_t notification_remaining_count_get(uint8_t type);

void notification_sync_lost(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NOTIFICATION_H_ */
