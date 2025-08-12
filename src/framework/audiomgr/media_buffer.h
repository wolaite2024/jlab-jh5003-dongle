/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _MEDIA_BUFFER_H_
#define _MEDIA_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include "media_pool.h"
#include "audio_type.h"

/* TODO Remove Start */
#include "os_queue.h"
/* TODO Remove End */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define INVALID_BT_CLOCK        0xf0000000


typedef void *T_MEDIA_BUFFER_HANDLE;

typedef bool (*P_MEDIABUFFER_FLUSH_FUNC)(uint16_t cnt, void *param);
typedef void *(*P_MEDIABUFFER_PEEK_FUNC)(int index, uint16_t *len, uint32_t *timestamp);
typedef uint16_t (*P_MEDIABUFFER_SHRINK_FUNC)(uint16_t len);
typedef bool (*P_MEDIABUFFER_WRITE_FUNC)(void *data, uint16_t len, void *param);
typedef uint16_t (*P_MEDIABUFFER_READ_FUNC)(void *buf, uint16_t len, uint32_t *timestamp);
typedef void (*P_MEDIABUFFER_RESET_FUNC)(void);
typedef void (*P_MEDIABUFFER_DUMP_FUNC)(void);

typedef enum
{
    MEDIA_BUFFER_TYPE_VOICE     = 0x00,
    MEDIA_BUFFER_TYPE_A2DP      = 0x01,
    MEDIA_BUFFER_TYPE_RECORD    = 0x02,
} T_MEDIA_BUFFER_TYPE;

typedef enum
{
    DSP_SYNC_BEGIN_CLK_NONE         = 0x00,
    DSP_SYNC_BEGIN_CLK_A2DP         = 0x01,
    DSP_SYNC_BEGIN_CLK_VOICE        = 0x02,
    DSP_SYNC_BEGIN_CLK_RINGTONE     = 0x03,
    DSP_SYNC_BEGIN_CLK_VP           = 0x04,
    DSP_SYNC_BEGIN_CLK_MIX          = 0x05,
    DSP_SYNC_BEGIN_CLK_ASRC         = 0x06,
} T_DSP_SYNC_BEGIN_CLK_TYPE;


typedef struct t_mediabuffer_op
{
    T_MEDIA_BUFFER_TYPE         buffer_type;
    T_MEDIA_POOL_HANDLE         pool_handle;
    P_MEDIABUFFER_FLUSH_FUNC    flush_fun;
    P_MEDIABUFFER_PEEK_FUNC     peek_fun;
    P_MEDIABUFFER_SHRINK_FUNC   shrink_fun;
    P_MEDIABUFFER_WRITE_FUNC    write_fun;
    P_MEDIABUFFER_READ_FUNC     read_fun;
    P_MEDIABUFFER_RESET_FUNC    reset_fun;
    P_MEDIABUFFER_DUMP_FUNC     dump_fun;
} T_MEDIABUFFER_OP;

typedef enum
{
    BUFFER_DIR_DOWNSTREAM,
    BUFFER_DIR_UPSTREAM,
} T_MEDIA_BUFFER_DIRECTION;

typedef bool (*P_MEDIA_BUFFER_OP_FLUSH)(void *buffer_self, uint16_t cnt,
                                        T_MEDIA_BUFFER_DIRECTION direction);
typedef void *(*P_MEDIA_BUFFER_OP_PEEK)(void *buffer_self, int index, uint16_t *len,
                                        uint32_t *timestamp, T_MEDIA_BUFFER_DIRECTION direction);
typedef uint16_t (*P_MEDIA_BUFFER_OP_SHRINK)(void *buffer_self, void *p_packet, uint16_t len,
                                             T_MEDIA_BUFFER_DIRECTION direction);
typedef bool (*P_MEDIA_BUFFER_OP_WRITE)(void *buffer_self, void *data, uint16_t len,
                                        uint16_t *written_len, void *param, T_MEDIA_BUFFER_DIRECTION direction);
typedef void (*P_MEDIA_BUFFER_OP_DUMP)(void *buffer_self);
typedef void (*P_MEDIA_BUFFER_OP_RESET)(void *buffer_self);

typedef struct t_media_buffer_op
{
    P_MEDIA_BUFFER_OP_FLUSH    flush_fun;
    P_MEDIA_BUFFER_OP_PEEK     peek_fun;
    P_MEDIA_BUFFER_OP_SHRINK   shrink_fun;
    P_MEDIA_BUFFER_OP_WRITE    write_fun;
    P_MEDIA_BUFFER_OP_DUMP     dump_fun;
    P_MEDIA_BUFFER_OP_RESET     reset_fun;
} T_MEDIA_BUFFER_OP;

typedef struct t_media_data_hdr
{
    struct t_media_data_hdr     *p_next;
    uint32_t            used_block_size;
    uint32_t            bt_clk;
    uint16_t            payload_length;
    uint16_t            avdtp_seq_number;
    uint16_t            read_offset;
    uint16_t            frame_counter;
    uint16_t            local_seq_number;
    uint8_t             frame_number; //0 for useless, other for used
    uint8_t             pkt_format;
    uint8_t             pkt_status;
    uint8_t             rsv[3];
    uint8_t             p_data[0];
} T_MEDIA_DATA_HDR;

typedef struct t_media_buffer_cfg
{
    void                *buffer_handle;
    void                *track_handle;
    void                *attached_path_handle;
    T_AUDIO_STREAM_TYPE stream_type;
    T_AUDIO_STREAM_USAGE usage;
    T_AUDIO_STREAM_MODE  mode;
    T_AUDIO_FORMAT_INFO format_info;
    uint16_t    lower_threshold_ms;
    uint16_t    upper_threshold_ms;
    uint16_t    latency;
    uint16_t    latency_back_up;
    bool        latency_fixed;
    uint8_t     latency_override;
    uint16_t plc_count;
    uint32_t restart_cause;
} T_MEDIA_BUFFER_CFG;

typedef enum
{
    MEDIA_BUFFER_UNINIT,
    MEDIA_BUFFER_PREQUEUE,
    MEDIA_BUFFER_WAIT_TO_PLAY,
    MEDIA_BUFFER_PLAYING,
} T_MEDIA_BUFFER_STATUS;

typedef struct t_buffer_queue
{
    T_OS_QUEUE queue;
    uint16_t total_frames_in_buffer;
    uint16_t total_payload_bytes_in_buffer;
    uint16_t total_block_bytes_in_buffer;
} T_BUFFER_QUEUE;

typedef struct t_media_buffer_entity
{
    struct t_media_buffer_entity   *p_next;
    void        *audio_remote_handle;
    void        *jitter_buffer_handle;
    void        *audio_latency_handle;
    void        *audio_interval_eval_handle;
    void        *oneway_play_timer;
    void        *last_packet_ptr;
    uint32_t    last_ts;
    uint16_t    last_seq;
    uint16_t    start_decode_frame_cnt;
    uint16_t    frame_len;
    uint32_t    ds_bt_clk_last;
    uint32_t    exception;
    uint16_t    max_latency_plc_count;
    uint16_t    buffer_id;
    uint16_t    ds_frame_counter_last;
    uint16_t    ds_local_seq_last;
    uint16_t    buffer_sync_trigger_cnt;
    uint8_t     miss_ack_cnt;
    uint8_t     delay_reset_path;
    uint8_t     voice_plc_check_interval;
    T_MEDIA_BUFFER_STATUS status;
    uint8_t     reset_wo_flush;
    uint8_t     paused;
    bool        last_seq_updated;
    bool        local_audio_sync;
    uint8_t     ignore_latency_report;
    uint8_t     buffer_type;
    uint8_t     sec_path_ready;
    uint8_t     media_buffer_fst_sync_tid;

    T_MEDIA_POOL_HANDLE pool_handle;
    T_MEDIA_BUFFER_OP ops;
    T_BUFFER_QUEUE downstream_queue;
    T_BUFFER_QUEUE upstream_queue;
    T_MEDIA_BUFFER_CFG *p_cfg;
} T_MEDIA_BUFFER_ENTITY;

typedef struct
{
    uint16_t    local_seq_num;
    uint16_t    frame_counter;
    uint16_t    l2cap_seq_num;
    uint16_t    rsv;
} T_MEDIA_BUFFER_SYNC_UNIT;

typedef struct
{
    uint8_t tid;
    uint16_t rsv;
    uint16_t cnt;
    uint16_t avdtp_seqs[0];
} T_MEDIA_BUFFER_FST_SYNC_CMD;

typedef struct
{
    uint8_t tid;
    uint16_t rsv;
    uint32_t bt_clock;
} T_VOICE_BUFFER_FST_SYNC_CMD;

typedef struct
{
    uint16_t join_local_seq;
    uint16_t join_local_frame_cnt;
    uint32_t join_pkt_id;
    uint32_t join_bt_clk;
} T_JOIN_FST_SYNC_CMD;

typedef struct
{
    uint16_t avdtp_seq_matched;
    uint8_t tid;
    bool    found;
} T_MEDIA_FIRST_SYNC_PKT_RSP;

typedef struct
{
    uint8_t tid;
    uint8_t buffer_type;
    uint16_t count;
    uint32_t IDs[0];
} T_BUFFER_FIRST_SYNC_CMD;

typedef struct
{
    uint8_t tid;
    uint8_t buffer_type;
    uint8_t found;
    uint8_t cause;
    uint32_t match_id;
} T_BUFFER_FIRST_SYNC_RSP;


typedef void *T_MEDIA_BUFFER_PROXY;

bool media_buffer_init(void);
void media_buffer_deinit(void);

void media_buffer_threshold_set(T_MEDIA_BUFFER_PROXY proxy, uint16_t upper_threshold_ms,
                                uint16_t lower_threshold_ms);
void media_buffer_latency_set2(T_MEDIA_BUFFER_PROXY proxy, uint16_t latency, bool fixed, float var);
void media_buffer_latency_set(T_MEDIA_BUFFER_PROXY proxy, uint16_t latency, bool fixed);
void media_buffer_latency_get(T_MEDIA_BUFFER_PROXY proxy, uint16_t *latency);
T_MEDIA_BUFFER_PROXY media_buffer_attach(T_AUDIO_STREAM_TYPE   stream_type,
                                         T_AUDIO_STREAM_USAGE  usage,
                                         T_AUDIO_STREAM_MODE   mode,
                                         T_AUDIO_FORMAT_INFO  *format_info_ref,
                                         void                 *track_handle,
                                         void                 *path_handle);
void media_buffer_detach(T_MEDIA_BUFFER_PROXY proxy);
uint16_t media_buffer_us_write(T_MEDIA_BUFFER_PROXY proxy,
                               void                 *buf,
                               uint16_t              len,
                               uint32_t              timestamp,
                               uint16_t             seq,
                               T_AUDIO_STREAM_STATUS  status,
                               uint8_t              frame_num);
bool media_buffer_ds_write(T_MEDIA_BUFFER_PROXY   proxy,
                           uint32_t               timestamp,
                           uint16_t               seq,
                           T_AUDIO_STREAM_STATUS  status,
                           uint8_t                frame_num,
                           void                  *buf,
                           uint16_t               len,
                           uint16_t              *written_len);
uint16_t media_buffer_us_read(T_MEDIA_BUFFER_PROXY   proxy,
                              void                  *buf,
                              uint16_t               len,
                              uint32_t              *timestamp,
                              uint16_t              *seq_num,
                              T_AUDIO_STREAM_STATUS *status,
                              uint8_t               *frame_num);
T_MEDIA_BUFFER_STATUS media_buffer_get_status(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_set_status(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_MEDIA_BUFFER_STATUS status);
bool media_buffer_downstream_dsp(T_MEDIA_BUFFER_ENTITY *buffer, bool flush);
uint16_t media_buffer_ds_frame_cnt(T_MEDIA_BUFFER_ENTITY *buffer_ent);
uint16_t media_buffer_ds_pkt_cnt(T_MEDIA_BUFFER_ENTITY *buffer_ent);
uint16_t media_buffer_us_frame_cnt(T_MEDIA_BUFFER_ENTITY *buffer_ent);
uint16_t media_buffer_us_pkt_cnt(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_deactive_proxy(T_MEDIA_BUFFER_PROXY proxy);
void media_buffer_active_proxy(T_MEDIA_BUFFER_PROXY proxy);
uint16_t media_buffer_cacu_frame_num(uint16_t time_ms, T_AUDIO_FORMAT_INFO *format_info);
uint16_t media_buffer_cacu_frames_duration(uint16_t frame_num, T_AUDIO_FORMAT_INFO *format_info);
uint16_t media_buffer_cacu_frame_len(T_MEDIA_BUFFER_ENTITY *buffer_ent,
                                     T_AUDIO_FORMAT_INFO   *format_info,
                                     void                  *param);
bool media_buffer_backup_downstream(T_MEDIA_BUFFER_ENTITY *buffer_ent);
bool media_buffer_level_get(T_MEDIA_BUFFER_PROXY proxy, uint16_t *level);
uint16_t media_buffer_get_ds_watermark(T_MEDIA_BUFFER_ENTITY *buffer_ent);
uint16_t media_buffer_get_us_watermark(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_low_latency_config(T_MEDIA_BUFFER_PROXY proxy);
bool media_buffer_clear_downstream_info(T_MEDIA_BUFFER_ENTITY *buffer_ent);
bool media_buffer_set_downstream_info(T_MEDIA_BUFFER_ENTITY *buffer_ent,
                                      uint16_t ds_frame_counter_last, uint16_t ds_local_seq_last, uint32_t ds_bt_clk_last);
bool media_buffer_get_downstream_info(T_MEDIA_BUFFER_ENTITY *buffer_ent,
                                      uint16_t *ds_frame_counter_last, uint16_t *ds_local_seq_last, uint32_t *ds_bt_clk_last);
bool media_buffer_get_stream_type(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_AUDIO_STREAM_TYPE *type);
bool media_buffer_get_mode(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_AUDIO_STREAM_MODE *mode);
bool media_buffer_get_format_type(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_AUDIO_FORMAT_TYPE *type);
void media_buffer_check_miss_ack(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_leading_downstream(T_MEDIA_BUFFER_ENTITY *buffer_ent);
bool media_buffer_cacu_playtime(T_MEDIA_BUFFER_ENTITY *buffer_ent, uint16_t *playtime_ms);
void media_buffer_handle_plc_event(T_MEDIA_BUFFER_ENTITY *buffer_ent, uint16_t plc_total_frames,
                                   uint16_t plc_frame_counter, uint16_t plc_local_seq, uint8_t plc_frame_num);
void media_buffer_dynamic_latency_resume(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_dynamic_latency_set(T_MEDIA_BUFFER_ENTITY *buffer_ent, uint16_t latency,
                                      bool auto_resume);
void media_buffer_handle_sync_lock(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_handle_sync_v2_succ(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_set_oneway_play_timer(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_relay_exception(T_MEDIA_BUFFER_ENTITY *buffer_ent, uint8_t *cmd);
void media_buffer_query_sec_path_state(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_reset_relay(T_MEDIA_BUFFER_ENTITY *buffer_ent);
void media_buffer_inteval_evaluator_update(T_MEDIA_BUFFER_PROXY proxy, uint16_t seq, uint32_t clk);
bool media_buffer_inteval_evaluator_get_var(T_MEDIA_BUFFER_PROXY proxy, float *var);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MEDIA_BUFFER_H_ */
