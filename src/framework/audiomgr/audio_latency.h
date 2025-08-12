/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_LATENCY_H_
#define _AUDIO_LATENCY_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    AUDIO_LATENCY_EVT_LAT_INCREASED,
    AUDIO_LATENCY_EVT_LAT_DECREASED,
    AUDIO_LATENCY_EVT_LOW_LAT_INCREASE_REQ,
    AUDIO_LATENCY_EVT_LOW_LAT_DECREASE_REQ,
    AUDIO_LATENCY_EVT_MAX_LAT_PLC,
    AUDIO_LATENCY_EVT_PLC_FIX,
    AUDIO_LATENCY_EVT_ULTRA_LOW_LAT_PLC_REQ,
} T_AUDIO_LATENCY_EVT;


typedef void *T_AUDIO_LATENCY_HANDLE;
typedef void (*T_AUDIO_LATENCY_EVT_CBACK)(T_AUDIO_LATENCY_EVT event, void *param, void *owner);

bool audio_latency_mgr_init(void);
void audio_latency_mgr_deinit(void);
T_AUDIO_LATENCY_HANDLE audio_latency_mgr_register(void *buffer_ent,
                                                  T_AUDIO_LATENCY_EVT_CBACK cback);
void audio_latency_mgr_unregister(T_AUDIO_LATENCY_HANDLE handle);
bool audio_latency_adjust_check(T_AUDIO_LATENCY_HANDLE handle);
void audio_latency_buffer_reset(T_AUDIO_LATENCY_HANDLE handle);
void audio_latency_reset(T_AUDIO_LATENCY_HANDLE handle);
void audio_latency_handle_inverval_unstable_evt(T_AUDIO_LATENCY_HANDLE handle);
void audio_latency_handle_inverval_stable_evt(T_AUDIO_LATENCY_HANDLE handle,
                                              T_AUDIO_FORMAT_TYPE type);
void audio_latency_low_lat_dynamic_increase(T_AUDIO_LATENCY_HANDLE handle, uint16_t plc_count,
                                            bool auto_dec);
void audio_latency_low_lat_dynamic_decrease(T_AUDIO_LATENCY_HANDLE handle, uint16_t plc_count);
void audio_latency_dynamic_set(T_AUDIO_LATENCY_HANDLE handle, uint16_t latency, bool auto_resume);
void audio_latency_resume(T_AUDIO_LATENCY_HANDLE handle);
void audio_latency_handle_plc_event(T_AUDIO_LATENCY_HANDLE handle, uint16_t local_seq,
                                    uint16_t plc_total_frames,
                                    uint16_t plc_frame_counter, uint16_t plc_local_seq, uint8_t plc_frame_num);
void audio_latency_ultra_low_latency_plc_req(T_AUDIO_LATENCY_HANDLE handle, uint8_t frame_num);
void audio_latency_record_update(T_AUDIO_LATENCY_HANDLE handle, uint32_t latency_report);
bool audio_latency_get_dynamic_latency_on(T_AUDIO_LATENCY_HANDLE handle, bool *dynamic_latency_on);
bool audio_latency_set_dynamic_latency_on(T_AUDIO_LATENCY_HANDLE handle, bool dynamic_latency_on);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_LATENCY_H_ */
