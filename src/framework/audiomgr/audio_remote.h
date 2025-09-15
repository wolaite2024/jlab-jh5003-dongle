/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_REMOTE_H_
#define _AUDIO_REMOTE_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "audio_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SEAMLESS_JOIN_MAX_REY_CNT 5

typedef enum
{
    AUDIO_REMOTE_STATE_UNSYNC,
    AUDIO_REMOTE_STATE_ALIGNING,
    AUDIO_REMOTE_STATE_SEAMLESS_JOIN,
    AUDIO_REMOTE_STATE_SYNCED,
} T_AUDIO_REMOTE_STATE;

typedef enum
{
    AUDIO_REMOTE_EVENT_BUFFER_SYNC,
    AUDIO_REMOTE_EVENT_ALIGNED,
    AUDIO_REMOTE_EVENT_SYNC_PLAY_RSV,
    AUDIO_REMOTE_EVENT_SYNC_PLAY_START,
} T_AUDIO_REMOTE_EVT;

typedef struct t_audio_remote_db
{
    T_REMOTE_RELAY_HANDLE   relay_handle;
    T_OS_QUEUE              remote_db_list;
    bool                    seamless_join;
    bool                    force_join;
    uint8_t                 endpoint_count;
} T_AUDIO_REMOTE_DB;

typedef struct t_media_packet_record
{
    struct t_media_packet_record *p_next;
    struct t_media_packet_record *p_prev;
    uint32_t    timestamp;
    int         seq_offset;
    uint16_t    avdtp_seq_num;
    uint16_t    local_seq_num;
    uint16_t    frame_counter;
    uint8_t     frame_number;
    uint8_t     used;
} T_MEDIA_PACKET_RECORD;

typedef struct t_audio_latency_record
{
    bool        dynamic_latency_on;
    uint16_t    plc_count;
    uint16_t    latency;
    uint16_t    latency_back_up;
    uint8_t     latency_override;
} T_AUDIO_REMOTE_LATENCY_INFO;

typedef void *T_AUDIO_REMOTE_HANDLE;
typedef void (*T_AUDIO_REMOTE_EVT_CBACK)(T_AUDIO_REMOTE_EVT event, void *param, void *owner);
typedef void (*T_AUDIO_REMOTE_RELAY_CBACK)(uint16_t event, void *param,
                                           T_REMOTE_RELAY_STATUS status, void *owner);

bool audio_remote_init(void);
void audio_remote_deinit(void);
void audio_remote_reset(T_AUDIO_REMOTE_HANDLE handle);
T_AUDIO_REMOTE_HANDLE audio_remote_media_buffer_register(T_AUDIO_STREAM_TYPE type,
                                                         void *buffer_ent,
                                                         T_AUDIO_REMOTE_EVT_CBACK event_cback,
                                                         T_AUDIO_REMOTE_RELAY_CBACK relay_cback);
void audio_remote_media_buffer_unregister(T_AUDIO_REMOTE_HANDLE handle);
void audio_remote_set_state(T_AUDIO_REMOTE_HANDLE handle, T_AUDIO_REMOTE_STATE state);
T_AUDIO_REMOTE_STATE audio_remote_get_state(T_AUDIO_REMOTE_HANDLE handle);

void audio_remote_seamless_join_set(bool enable, bool force_join);
bool audio_remote_seamless_join_seq_contiuity_check(T_AUDIO_REMOTE_HANDLE handle);
void audio_remote_dsp_seamless_join_handle(T_MEDIA_BUFFER_ENTITY *buffer_ent, void *param);
bool audio_remote_find_join_pkt(T_AUDIO_REMOTE_HANDLE handle);
void audio_remote_seamless_join_retry(T_AUDIO_REMOTE_HANDLE handle, T_AUDIO_MGR_EXCEPTION exc);
void audio_remote_cancel_seamless_join(T_AUDIO_REMOTE_HANDLE handle);

void audio_remote_ultra_low_latency_sync_play(T_AUDIO_REMOTE_HANDLE handle,
                                              bool dynamic_latency_on);
void audio_remote_low_latency_sync_play(T_AUDIO_REMOTE_HANDLE handle, bool danamic_latency_on);
void audio_mgr_appoint_playtime(T_AUDIO_REMOTE_HANDLE handle, uint32_t set_timing_ms,
                                bool dynamic_latency_on);
bool audio_remote_align_buffer(T_AUDIO_REMOTE_HANDLE handle);
bool audio_remote_peroid_sync_check(T_AUDIO_REMOTE_HANDLE handle, uint16_t buffer_sync_trigger_cnt);
void audio_remote_buffer_sync_req(T_AUDIO_REMOTE_HANDLE handle);

void media_buffer_packet_record_add(T_AUDIO_REMOTE_HANDLE handle, void *p_packet);
int media_buffer_packet_record_fix_by_local_seq(T_AUDIO_REMOTE_HANDLE handle, uint16_t local_seq,
                                                uint16_t frame_counter,
                                                uint8_t frame_number);
uint8_t media_buffer_packet_record_get_timestamps(T_AUDIO_REMOTE_HANDLE handle,
                                                  uint32_t *p_timestamp, uint8_t cnt);

bool audio_remote_is_buffer_unsync(T_AUDIO_REMOTE_HANDLE handle);
bool audio_remote_buffer_PLC(T_AUDIO_REMOTE_HANDLE handle);
void  audio_remote_set_frame_diff(T_AUDIO_REMOTE_HANDLE handle, int frame_diff);
int audio_remote_get_frame_diff(T_AUDIO_REMOTE_HANDLE handle);
void audio_remote_handle_lost_packet(T_AUDIO_REMOTE_HANDLE handle, uint8_t count);
void audio_remote_sync_lock(T_AUDIO_REMOTE_HANDLE handle);
bool audio_remote_async_msg_relay(T_AUDIO_REMOTE_HANDLE handle,
                                  uint16_t               msg_id,
                                  void                  *msg_buf,
                                  uint16_t               msg_len,
                                  bool                   loopback);
bool audio_remote_sync_msg_relay(T_AUDIO_REMOTE_HANDLE handle,
                                 uint16_t               msg_id,
                                 void                  *msg_buf,
                                 uint16_t               msg_len,
                                 T_REMOTE_TIMER_TYPE    timer_type,
                                 uint32_t               timer_period,
                                 bool                   loopback);

extern T_AUDIO_REMOTE_DB *audio_remote_db;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_REMOTE_H_ */
