/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "trace.h"
#include "sys_mgr.h"
#include "sys_timer.h"
#include "audio.h"
#include "audio_mgr.h"
#include "audio_remote.h"
#include "notification.h"
#include "audio_path.h"
#include "media_buffer.h"
#include "jitter_buffer.h"
#include "audio_latency.h"
/* TODO Remove Start */
#include "bt_mgr.h"
#include "audio_interval_eval.h"
#include "media_pool.h"

extern bool dsp_sport0_da_chann_set(uint8_t chann_mask);
extern bool dsp_sport0_ad_chann_set(uint8_t chann_mask);
/* TODO Remove End */

T_AUDIO_TRACK_CAUSE convert_exception_to_restart_casuse(uint32_t exception)
{
    T_AUDIO_TRACK_CAUSE cause;
    T_AUDIO_MGR_EXCEPTION exc = (T_AUDIO_MGR_EXCEPTION)exception;
    switch (exc)
    {
    case MEDIA_BUFFER_PACKET_LOST:
        cause = AUDIO_TRACK_CAUSE_JOIN_PACKET_LOST;
        break;
    case DSP_DECODE_EMPTY:
        cause = AUDIO_TRACK_CAUSE_BUFFER_EMPTY;
        break;
    case MEDIA_BUFFER_SEAMLESS_MISS_PKT:
    case MEDIA_BUFFER_SEAMLESS_TIMEOUT:
        cause = AUDIO_TRACK_CAUSE_REMOTE_JOIN_FAILED;
        break;
    default:
        cause = AUDIO_TRACK_CAUSE_UNKNOWN_ERROR;
        break;
    }
    return cause;
}

void media_buffer_check_miss_ack(T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent->miss_ack_cnt)
    {
        if (audio_remote_is_buffer_unsync(buffer_ent->audio_remote_handle))
        {
            if (audio_remote_buffer_PLC(buffer_ent->audio_remote_handle))
            {
                buffer_ent->miss_ack_cnt--;
            }
        }
        else
        {
            if (media_buffer_downstream_dsp(buffer_ent, true))
            {
                buffer_ent->miss_ack_cnt--;
            }
        }
    }
}


typedef struct t_audio_mgr_cback_item
{
    struct t_audio_mgr_cback_item *p_next;
    P_AUDIO_MGR_CBACK              cback;
} T_AUDIO_MGR_CBACK_ITEM;

T_AUDIO_DB *audio_db = NULL;


bool bt_clk_compare(uint32_t bt_clk_a, uint32_t bt_clk_b)
{
    if ((bt_clk_a > bt_clk_b  && bt_clk_a - bt_clk_b < 0X07FFFFFFF)
        ||
        (bt_clk_a < bt_clk_b && bt_clk_b - bt_clk_a > 0X07FFFFFFF)
       )
    {
        return true;
    }
    else
    {
        return false;
    }
}

T_MEDIA_BUFFER_ENTITY *audio_mgr_get_active_buffer(T_AUDIO_STREAM_TYPE type)
{
    if (type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        return (T_MEDIA_BUFFER_ENTITY *)audio_db->playback_buffer_handle;
    }
    else if (type == AUDIO_STREAM_TYPE_VOICE)
    {
        return (T_MEDIA_BUFFER_ENTITY *)audio_db->voice_buffer_handle;
    }
    else
    {
        return NULL;
    }
}

T_MEDIA_BUFFER_ENTITY *audio_mgr_get_playing_buffer(T_AUDIO_STREAM_TYPE type)
{
    if (type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        if (media_buffer_get_status((T_MEDIA_BUFFER_ENTITY *)(audio_db->playback_buffer_handle)) ==
            MEDIA_BUFFER_PLAYING)
        {
            return (T_MEDIA_BUFFER_ENTITY *)audio_db->playback_buffer_handle;
        }
        else
        {
            return NULL;
        }
    }
    else if (type == AUDIO_STREAM_TYPE_VOICE)
    {
        if (media_buffer_get_status((T_MEDIA_BUFFER_ENTITY *)(audio_db->voice_buffer_handle)) ==
            MEDIA_BUFFER_PLAYING)
        {
            return (T_MEDIA_BUFFER_ENTITY *)audio_db->voice_buffer_handle;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}

bool audio_mgr_cback_register(P_AUDIO_MGR_CBACK cback)
{
    T_AUDIO_MGR_CBACK_ITEM *p_item;

    p_item = (T_AUDIO_MGR_CBACK_ITEM *)audio_db->cback_list.p_first;
    while (p_item != NULL)
    {
        /* Sanity check if callback already registered */
        if (p_item->cback == cback)
        {
            return true;
        }

        p_item = p_item->p_next;
    }

    p_item = os_mem_alloc2(sizeof(T_AUDIO_MGR_CBACK_ITEM));
    if (p_item != NULL)
    {
        p_item->cback = cback;
        os_queue_in(&audio_db->cback_list, p_item);
        return true;
    }

    return false;
}

bool audio_mgr_cback_unregister(P_AUDIO_MGR_CBACK cback)
{
    T_AUDIO_MGR_CBACK_ITEM *p_item;
    bool ret = false;

    p_item = (T_AUDIO_MGR_CBACK_ITEM *)audio_db->cback_list.p_first;
    while (p_item != NULL)
    {
        if (p_item->cback == cback)
        {
            os_queue_delete(&audio_db->cback_list, p_item);
            os_mem_free(p_item);
            ret = true;
            break;
        }

        p_item = p_item->p_next;
    }

    return ret;
}

bool audio_mgr_event_post(T_AUDIO_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_AUDIO_MGR_CBACK_ITEM *p_item;

    AUDIO_PRINT_INFO1("audio_mgr_event_post: event_type 0x%04x", event_type);

    p_item = (T_AUDIO_MGR_CBACK_ITEM *)audio_db->cback_list.p_first;
    while (p_item != NULL)
    {
        p_item->cback(event_type, event_buf, buf_len);
        p_item = p_item->p_next;
    }

    return true;
}

void audio_buffer_uninit_state(T_AUDIO_MSG msg, T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }
    switch (msg)
    {
    case AUDIO_MSG_BUFFER_RESET:
        {
            audio_remote_reset(buffer_ent->audio_remote_handle);
        }
        break;

    case AUDIO_MSG_BUFFER_WRITE:
        {
            if (buffer_ent->buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                {
                    buffer_ent->max_latency_plc_count = media_buffer_cacu_frame_num(ULTRA_LOW_LATENCY_DYNAMIC_LATENCY_MS
                                                                                    -
                                                                                    buffer_ent->p_cfg->latency_back_up,
                                                                                    &(buffer_ent->p_cfg->format_info));
                }
                else if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY)
                {
                    buffer_ent->max_latency_plc_count = media_buffer_cacu_frame_num(LOW_LATENCY_DYNAMIC_LATENCY_MS -
                                                                                    buffer_ent->p_cfg->latency_back_up,
                                                                                    &(buffer_ent->p_cfg->format_info));
                }

                buffer_ent->start_decode_frame_cnt = media_buffer_cacu_frame_num(buffer_ent->p_cfg->latency,
                                                                                 &(buffer_ent->p_cfg->format_info));

                jitter_buffer_asrc_pid_init(buffer_ent->jitter_buffer_handle);

                if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
                    buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                {
                    AUDIO_PRINT_INFO4("media_buffer_init: gaming_mode start frame num %u latency %u fixed %u max_latency_plc_count %u",
                                      buffer_ent->start_decode_frame_cnt, buffer_ent->p_cfg->latency, buffer_ent->p_cfg->latency_fixed,
                                      buffer_ent->max_latency_plc_count);
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PREQUEUE);
                    audio_remote_reset(buffer_ent->audio_remote_handle);
                    if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED &&
                        buffer_ent->p_cfg->usage != AUDIO_STREAM_USAGE_LOCAL)
                    {
                        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                        {
                            buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                            buffer_ent->sec_path_ready = 0;
                            media_buffer_query_sec_path_state(buffer_ent);
                        }
                    }
                    else
                    {
                        audio_mgr_dispatch(AUDIO_MSG_BUFFER_WRITE, buffer_ent);
                    }
                }
                else if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_DIRECT)
                {
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                    audio_remote_reset(buffer_ent->audio_remote_handle);
                    buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                }
                else
                {
                    AUDIO_PRINT_INFO3("media_buffer_init: start frame num %u latency %u max_plc %u",
                                      buffer_ent->start_decode_frame_cnt, buffer_ent->p_cfg->latency,
                                      audio_db->max_plc_count);
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PREQUEUE);
                    audio_remote_reset(buffer_ent->audio_remote_handle);
                    if (buffer_ent->p_cfg->usage == AUDIO_STREAM_USAGE_SNOOP)
                    {
                        if (remote_session_state_get() == REMOTE_SESSION_STATE_CONNECTED &&
                            remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                        {
                            media_buffer_set_oneway_play_timer(buffer_ent);
                        }
                    }
                }
            }
            else if (buffer_ent->buffer_type == AUDIO_STREAM_TYPE_VOICE)
            {
                if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_DIRECT)
                {
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                    audio_remote_reset(buffer_ent->audio_remote_handle);
                    audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, BT_CLK_NONE, 0xffffffff, false);
                    media_buffer_downstream_dsp(buffer_ent, true);
                }
                else
                {
                    buffer_ent->start_decode_frame_cnt = media_buffer_cacu_frame_num(buffer_ent->p_cfg->latency,
                                                                                     &(buffer_ent->p_cfg->format_info));
                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PREQUEUE);
                    audio_remote_reset(buffer_ent->audio_remote_handle);
                }
            }
        }
        break;

    default:
        break;
    }
}

void audio_buffer_prequeue_state(T_AUDIO_MSG msg, T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    T_MEDIA_BUFFER_CFG *p_cfg = buffer_ent->p_cfg;

    if (p_cfg == NULL)
    {
        return;
    }

    switch (msg)
    {
    case AUDIO_MSG_BUFFER_RESET:
        {
            audio_remote_reset(buffer_ent->audio_remote_handle);
        }
        break;

    case AUDIO_MSG_BUFFER_WRITE:
        {
            if (p_cfg->usage == AUDIO_STREAM_USAGE_LOCAL ||
                (p_cfg->usage == AUDIO_STREAM_USAGE_SNOOP &&
                 remote_session_state_get() == REMOTE_SESSION_STATE_DISCONNECTED))
            {
                {
                    uint32_t bb_clock_slot;
                    uint32_t bb_clock_slot_sync;
                    uint16_t bb_clock_us;
                    T_BT_CLK_REF clk_ref;
                    uint16_t latency = buffer_ent->p_cfg->latency;

                    if (p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY)
                    {
                        clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                        if (clk_ref != BT_CLK_NONE)
                        {
                            bb_clock_slot_sync = (bb_clock_slot + latency * 1000 * 2 / 625) & 0x0fffffff;
                            AUDIO_PRINT_INFO1("media_buffer_set_playing: gaming_mode latency %d", latency);
                            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                            audio_path_timestamp_set(p_cfg->attached_path_handle, BT_CLK_SNIFFING, bb_clock_slot_sync, false);
                            media_buffer_downstream_dsp(buffer_ent, true);
                        }
                        else
                        {
                            if (media_buffer_ds_frame_cnt(buffer_ent) >= buffer_ent->start_decode_frame_cnt)
                            {
                                AUDIO_PRINT_INFO1("media_buffer_set_playing: gaming_mode total_frames %d",
                                                  media_buffer_ds_frame_cnt(buffer_ent));
                                media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                                audio_path_timestamp_set(p_cfg->attached_path_handle, BT_CLK_NONE, 0xffffffff, false);
                                media_buffer_downstream_dsp(buffer_ent, false);
                                media_buffer_downstream_dsp(buffer_ent, true);
                            }
                        }
                    }
                    else if (p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                    {
                        if (latency >= ULTRA_LOW_LAT_INSTANT_PLAY_LATENCY)
                        {
                            clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                            if (clk_ref != BT_CLK_NONE)
                            {
                                bb_clock_slot_sync = (bb_clock_slot + latency * 1000 * 2 / 625) & 0x0fffffff;
                                AUDIO_PRINT_INFO1("media_buffer_set_playing: gaming_mode latency %d", buffer_ent->p_cfg->latency);
                                media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                                audio_path_timestamp_set(p_cfg->attached_path_handle, BT_CLK_SNIFFING, bb_clock_slot_sync, false);
                                media_buffer_downstream_dsp(buffer_ent, true);
                            }
                            else
                            {
                                if (media_buffer_ds_frame_cnt(buffer_ent) >= buffer_ent->start_decode_frame_cnt)
                                {
                                    AUDIO_PRINT_INFO1("media_buffer_set_playing: gaming_mode total_frames %d",
                                                      media_buffer_ds_frame_cnt(buffer_ent));
                                    media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                                    audio_path_timestamp_set(p_cfg->attached_path_handle, BT_CLK_NONE, 0xffffffff, false);
                                    media_buffer_downstream_dsp(buffer_ent, false);
                                    media_buffer_downstream_dsp(buffer_ent, true);
                                }
                            }
                        }
                        else
                        {
                            AUDIO_PRINT_INFO0("media_buffer_set_playing: gaming_mode latency 0");
                            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                            clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                            audio_path_timestamp_set(p_cfg->attached_path_handle, clk_ref, 0xffffffff, false);
                            media_buffer_downstream_dsp(buffer_ent, true);
                        }
                    }
                    else
                    {
                        if (media_buffer_ds_frame_cnt(buffer_ent) >= buffer_ent->start_decode_frame_cnt / 2)
                        {
                            uint16_t play_time;

                            AUDIO_PRINT_INFO2("media_buffer_state: single play total_frames %d, goal %u",
                                              media_buffer_ds_frame_cnt(buffer_ent), buffer_ent->start_decode_frame_cnt);
                            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                            audio_remote_set_state(buffer_ent->audio_remote_handle, AUDIO_REMOTE_STATE_UNSYNC);
                            jitter_buffer_asrc_pid_init(buffer_ent->jitter_buffer_handle);
                            audio_path_synchronization_join_set(p_cfg->attached_path_handle, 0);

                            if (audio_remote_db->seamless_join &&
                                remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                            {
                                clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                                if (clk_ref != BT_CLK_NONE)
                                {
                                    media_buffer_cacu_playtime(buffer_ent, &play_time);
                                    bb_clock_slot_sync = (bb_clock_slot +  play_time * 1000 * 2 / 625) & 0x0fffffff;
                                    audio_path_timestamp_set(p_cfg->attached_path_handle, clk_ref, bb_clock_slot_sync, false);
                                    media_buffer_leading_downstream(buffer_ent);
                                }
                                else
                                {
                                    audio_path_timestamp_set(p_cfg->attached_path_handle, BT_CLK_NONE, 0xffffffff, false);
                                    media_buffer_leading_downstream(buffer_ent);
                                }
                            }
                            else
                            {
                                clk_ref = bt_piconet_clk_get(BT_CLK_SNIFFING, &bb_clock_slot, &bb_clock_us);
                                if (clk_ref != BT_CLK_NONE)
                                {
                                    media_buffer_cacu_playtime(buffer_ent, &play_time);
                                    bb_clock_slot_sync = (bb_clock_slot +  play_time * 1000 * 2 / 625) & 0x0fffffff;
                                    audio_path_timestamp_set(p_cfg->attached_path_handle, clk_ref, bb_clock_slot_sync, false);
                                }
                                else
                                {
                                    audio_path_timestamp_set(p_cfg->attached_path_handle, BT_CLK_NONE, 0xffffffff, false);
                                }
                                media_buffer_leading_downstream(buffer_ent);
                            }
                        }
                    }
                }
            }
            else
            {
                if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                {
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        if (buffer_ent->sec_path_ready)
                        {
                            bool dynamic_latency_on = false;
                            audio_latency_get_dynamic_latency_on(buffer_ent->audio_latency_handle,
                                                                 &dynamic_latency_on);
                            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                            audio_remote_ultra_low_latency_sync_play(buffer_ent->audio_remote_handle, dynamic_latency_on);
                        }
                        else
                        {
                            if (media_buffer_ds_pkt_cnt(buffer_ent) > 15)
                            {
                                buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                                          BUFFER_DIR_DOWNSTREAM);
                                media_buffer_set_status(buffer_ent, MEDIA_BUFFER_UNINIT);
                            }
                        }
                    }
                    else
                    {
                        if (audio_remote_get_state(buffer_ent->audio_remote_handle) == AUDIO_REMOTE_STATE_UNSYNC)
                        {
                            audio_remote_align_buffer(buffer_ent->audio_remote_handle);
                        }
                    }
                }
                else if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY)
                {
                    audio_track_volume_out_mute_all(AUDIO_STREAM_TYPE_PLAYBACK);
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        if (buffer_ent->sec_path_ready)
                        {
                            bool dynamic_latency_on = false;
                            audio_latency_get_dynamic_latency_on(buffer_ent->audio_latency_handle,
                                                                 &dynamic_latency_on);
                            media_buffer_set_status(buffer_ent, MEDIA_BUFFER_WAIT_TO_PLAY);
                            audio_remote_low_latency_sync_play(buffer_ent->audio_remote_handle, dynamic_latency_on);
                        }
                        else
                        {
                            if (media_buffer_ds_pkt_cnt(buffer_ent) > 5)
                            {
                                buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                                          BUFFER_DIR_DOWNSTREAM);
                                media_buffer_set_status(buffer_ent, MEDIA_BUFFER_UNINIT);
                            }

                        }
                    }
                    else
                    {
                        if (audio_remote_get_state(buffer_ent->audio_remote_handle) == AUDIO_REMOTE_STATE_UNSYNC)
                        {
                            audio_remote_align_buffer(buffer_ent->audio_remote_handle);
                        }
                    }
                }
                else
                {
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        if (media_buffer_ds_frame_cnt(buffer_ent) >= buffer_ent->start_decode_frame_cnt)
                        {
                            AUDIO_PRINT_WARN0("frames > start_decode_frame_cnt but fst sync rsp hasn't been received");
                        }
                        else
                        {
                            if (media_buffer_ds_pkt_cnt(buffer_ent) == MEDIA_SYNC_TRIGGER_CNT
                                && audio_remote_get_state(buffer_ent->audio_remote_handle) != AUDIO_REMOTE_STATE_SYNCED)
                            {
                                audio_remote_align_buffer(buffer_ent->audio_remote_handle);
                            }
                        }
                    }
                    else
                    {
                        if (media_buffer_ds_frame_cnt(buffer_ent) >= buffer_ent->start_decode_frame_cnt)
                        {
                            buffer_ent->ops.flush_fun(buffer_ent, 1, BUFFER_DIR_DOWNSTREAM);
                        }

                        if (audio_remote_get_state(buffer_ent->audio_remote_handle) == AUDIO_REMOTE_STATE_UNSYNC)
                        {
                            if (buffer_ent->buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
                            {
                                if (audio_remote_seamless_join_seq_contiuity_check(buffer_ent->audio_remote_handle))
                                {
                                    audio_remote_align_buffer(buffer_ent->audio_remote_handle);
                                }
                            }
                            else
                            {
                                audio_remote_align_buffer(buffer_ent->audio_remote_handle);
                            }
                        }
                        else if (audio_remote_get_state(buffer_ent->audio_remote_handle) ==
                                 AUDIO_REMOTE_STATE_SEAMLESS_JOIN)
                        {
                            //only sec runs here
                            audio_remote_find_join_pkt(buffer_ent->audio_remote_handle);
                        }
                    }
                }
            }

        }
        break;

    default:
        break;
    }
}

void audio_buffer_wait_play_state(T_AUDIO_MSG msg, T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }
    switch (msg)
    {
    case AUDIO_MSG_BUFFER_RESET:
        {
            buffer_ent->ops.reset_fun(buffer_ent);
        }
        break;

    case AUDIO_MSG_BUFFER_WRITE:
        {
            if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
            {
                buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent) - 1,
                                          BUFFER_DIR_DOWNSTREAM);
            }
            else if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_DIRECT)
            {
                media_buffer_set_status(buffer_ent, MEDIA_BUFFER_PLAYING);
                audio_path_timestamp_set(buffer_ent->p_cfg->attached_path_handle, BT_CLK_NONE, 0xffffffff, false);
                media_buffer_downstream_dsp(buffer_ent, true);
            }
        }
        break;

    default:
        break;
    }

}

void audio_buffer_playing_state(T_AUDIO_MSG msg, T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }
    switch (msg)
    {
    case AUDIO_MSG_BUFFER_RESET:
        {
            buffer_ent->ops.reset_fun(buffer_ent);

            if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
                buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    buffer_ent->delay_reset_path = true;
                    media_buffer_reset_relay(buffer_ent);
                }
                else
                {
                    if (!buffer_ent->delay_reset_path)
                    {
                        T_AUDIO_TRACK_STATE  state;
                        buffer_ent->p_cfg->restart_cause = convert_exception_to_restart_casuse(buffer_ent->exception);
                        audio_track_state_get(buffer_ent->p_cfg->track_handle, &state);
                        if (state == AUDIO_TRACK_STATE_STARTED)
                        {
                            audio_track_restart(buffer_ent->p_cfg->track_handle);
                        }
                    }
                }

            }
            else
            {
                if (!buffer_ent->delay_reset_path)
                {
                    T_AUDIO_TRACK_STATE  state;
                    buffer_ent->p_cfg->restart_cause = convert_exception_to_restart_casuse(buffer_ent->exception);
                    audio_track_state_get(buffer_ent->p_cfg->track_handle, &state);
                    if (state == AUDIO_TRACK_STATE_STARTED)
                    {
                        audio_track_restart(buffer_ent->p_cfg->track_handle);
                    }
                }
            }

        }
        break;

    case AUDIO_MSG_BUFFER_WRITE:
        {
            if (audio_remote_get_state(buffer_ent->audio_remote_handle) == AUDIO_REMOTE_STATE_SEAMLESS_JOIN)
            {
                //only pri runs here
                audio_remote_find_join_pkt(buffer_ent->audio_remote_handle);
            }

            if (buffer_ent->buffer_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
                    buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                {
                    if (audio_remote_is_buffer_unsync(buffer_ent->audio_remote_handle))
                    {
                        audio_remote_buffer_PLC(buffer_ent->audio_remote_handle);
                    }
                    media_buffer_downstream_dsp(buffer_ent, true);
                }
                else if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_DIRECT)
                {
                    while (media_buffer_downstream_dsp(buffer_ent, true));
                }
            }

            if (buffer_ent->p_cfg->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    buffer_ent->buffer_sync_trigger_cnt++;
                    if (audio_remote_peroid_sync_check(buffer_ent->audio_remote_handle,
                                                       buffer_ent->buffer_sync_trigger_cnt))
                    {
                        buffer_ent->buffer_sync_trigger_cnt = 0;
                        audio_remote_buffer_sync_req(buffer_ent->audio_remote_handle);
                    }
                }
                media_buffer_check_miss_ack(buffer_ent);

            }
            else if (buffer_ent->p_cfg->stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                media_buffer_downstream_dsp(buffer_ent, true);
            }
        }
        break;

    default:
        break;
    }
}

bool audio_mgr_dsp_msg_pipe(uint32_t id, void *msg)
{
    bool ret;

    ret = false;

    switch (id)
    {
    case AUDIO_PATH_EVT_DSP_INTER_MSG:
        {
            ret = audio_mgr_dispatch(AUDIO_MSG_A2DP_DSP_INTER_MSG, msg);
            if (ret == false)
            {
                audio_path_b2bmsg_interaction_timeout();
            }
        }
        break;
    }
    return ret;
}

void audio_mgr_buffer_event_handle(T_AUDIO_MSG msg, T_MEDIA_BUFFER_ENTITY *buffer_ent)
{
    switch (media_buffer_get_status(buffer_ent))
    {
    case MEDIA_BUFFER_UNINIT:
        {
            audio_buffer_uninit_state(msg, buffer_ent);
        }
        break;

    case MEDIA_BUFFER_PREQUEUE:
        {
            audio_buffer_prequeue_state(msg, buffer_ent);
        }
        break;

    case MEDIA_BUFFER_WAIT_TO_PLAY:
        {
            audio_buffer_wait_play_state(msg, buffer_ent);
        }
        break;

    case MEDIA_BUFFER_PLAYING:
        {
            audio_buffer_playing_state(msg, buffer_ent);
        }
        break;
    }
}

bool audio_mgr_dispatch(T_AUDIO_MSG msg, void *buf)
{
    bool ret = false;

    if (msg != AUDIO_MSG_BUFFER_WRITE)
    {
        AUDIO_PRINT_INFO1("audio_mgr_dispatch: msg 0x%04x", msg);
    }

    switch (msg)
    {
    case AUDIO_MSG_BUFFER_RESET:
    case AUDIO_MSG_BUFFER_WRITE:
        {
            T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)buf;
            audio_mgr_buffer_event_handle(msg, buffer_ent);
        }
        break;

    case AUDIO_MSG_BUFFER_LEVEL_LOW:
        {
            T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)buf;
            uint16_t buffer_level = 0;
            T_AUDIO_EVENT_PARAM_TRACK_BUFFER_LOW buffer_low_param;

            media_buffer_level_get((T_MEDIA_BUFFER_PROXY)buffer_ent->p_cfg, &buffer_level);
            buffer_low_param.buffer_level_ms = buffer_level;
            buffer_low_param.handle = buffer_ent->p_cfg->track_handle;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_BUFFER_LOW, &buffer_low_param,
                                       sizeof(T_AUDIO_EVENT_PARAM_TRACK_BUFFER_LOW));
        }
        break;

    case AUDIO_MSG_BUFFER_LEVEL_HIGH:
        {
            T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)buf;
            uint16_t buffer_level = 0;
            T_AUDIO_EVENT_PARAM_TRACK_BUFFER_HIGH buffer_high_param;

            media_buffer_level_get((T_MEDIA_BUFFER_PROXY)buffer_ent->p_cfg, &buffer_level);
            buffer_high_param.buffer_level_ms = buffer_level;
            buffer_high_param.handle = buffer_ent->p_cfg->track_handle;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_BUFFER_HIGH, &buffer_high_param,
                                       sizeof(T_AUDIO_EVENT_PARAM_TRACK_BUFFER_HIGH));
        }
        break;

    case AUDIO_MSG_BUFFER_STATE_PLAYING:
        {
            ret = audio_mgr_event_post(AUDIO_EVENT_BUFFER_STATE_PLAYING, NULL, 0);
        }
        break;

    case AUDIO_MSG_DSP_DECODE_EMPTY:
        {
            T_MEDIA_BUFFER_CFG *cfg = (T_MEDIA_BUFFER_CFG *)buf;
            if (cfg->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                audio_mgr_exception((T_MEDIA_BUFFER_ENTITY *)cfg->buffer_handle, DSP_DECODE_EMPTY, 0,
                                    0);
            }
            else
            {
                AUDIO_PRINT_ERROR0("dsp reprot empty for inappropriate track");
            }
        }
        break;

    case AUDIO_MSG_DSP_SYNC_UNLOCK:
        {
            T_MEDIA_BUFFER_CFG *cfg = (T_MEDIA_BUFFER_CFG *)buf;
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                audio_mgr_exception((T_MEDIA_BUFFER_ENTITY *)cfg->buffer_handle, DSP_UNSYNC, 0, 0);
            }
        }
        break;

    case AUDIO_MSG_A2DP_DSP_SYNC_LOCK:
        {
            T_MEDIA_BUFFER_CFG *cfg = (T_MEDIA_BUFFER_CFG *)buf;
            T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)cfg->buffer_handle;
            media_buffer_handle_sync_lock(buffer_ent);
        }
        break;

    case AUDIO_MSG_A2DP_DSP_SYNC_V2_SUCC:
        {
            T_MEDIA_BUFFER_CFG *cfg = (T_MEDIA_BUFFER_CFG *)buf;
            T_MEDIA_BUFFER_ENTITY *buffer_ent = (T_MEDIA_BUFFER_ENTITY *)cfg->buffer_handle;
            media_buffer_handle_sync_v2_succ(buffer_ent);
        }
        break;

    case AUDIO_MSG_DSP_UNSYNC:
        {
            T_MEDIA_BUFFER_CFG *cfg = (T_MEDIA_BUFFER_CFG *)buf;
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                audio_mgr_exception((T_MEDIA_BUFFER_ENTITY *)cfg->buffer_handle, DSP_UNSYNC, 0, 0);
            }
        }
        break;

    case AUDIO_MSG_DSP_SYNC_EMPTY:
        {
            T_MEDIA_BUFFER_CFG *cfg = (T_MEDIA_BUFFER_CFG *)buf;
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                audio_mgr_exception((T_MEDIA_BUFFER_ENTITY *)cfg->buffer_handle, DSP_SYNC_EMPTY, 0, 0);
            }
        }
        break;

    case AUDIO_MSG_DSP_LOST_TIMESTAMP:
        {
            T_MEDIA_BUFFER_CFG *cfg = (T_MEDIA_BUFFER_CFG *)buf;
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                audio_mgr_exception((T_MEDIA_BUFFER_ENTITY *)cfg->buffer_handle, DSP_LOST_TIMESTAMP, 0, 0);
            }
        }
        break;

    case AUDIO_MSG_A2DP_DSP_INTER_MSG:
        {
            uint8_t *p_buf = (uint8_t *)buf;
            ret = remote_async_msg_relay(audio_db->relay_handle, AUDIO_REMOTE_DSP_INTER_MSG, buf,
                                         p_buf[0], false);

        }
        break;

    case AUDIO_MSG_TRACK_DATA_IND:
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_DATA_IND *msg = buf;
            T_AUDIO_EVENT_PARAM param;

            param.track_data_ind.handle = msg->handle;
            param.track_data_ind.len    = msg->len;

            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_DATA_IND, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_DATA_XMIT:
        {
            T_MEDIA_BUFFER_ENTITY *buffer_ent;
            T_MEDIA_BUFFER_CFG *p_buffer_cfg;

            p_buffer_cfg = (T_MEDIA_BUFFER_CFG *)buf;
            buffer_ent = (T_MEDIA_BUFFER_ENTITY *)p_buffer_cfg->buffer_handle;

            if (media_buffer_get_status(buffer_ent) != MEDIA_BUFFER_PLAYING)
            {
                AUDIO_PRINT_TRACE0("audio_mgr_dispatch: not handle ack if buffer state is not playing");
                break;
            }

            if (audio_path_is_running(buffer_ent->p_cfg->attached_path_handle) == false)
            {
                AUDIO_PRINT_TRACE0("audio_mgr_dispatch: not handle ack if path state is not running");
                break;
            }

            if (p_buffer_cfg->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
                    buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                {
                    if (audio_remote_is_buffer_unsync(buffer_ent->audio_remote_handle))
                    {
                        audio_remote_buffer_PLC(buffer_ent->audio_remote_handle);
                    }
                    media_buffer_downstream_dsp(buffer_ent, true);
                }
                else
                {
                    if (audio_remote_is_buffer_unsync(buffer_ent->audio_remote_handle))
                    {
                        if (audio_remote_buffer_PLC(buffer_ent->audio_remote_handle) == false)
                        {
                            buffer_ent->miss_ack_cnt++;
                        }
                    }
                    else
                    {
                        if (media_buffer_downstream_dsp(buffer_ent, true) == false)
                        {
                            buffer_ent->miss_ack_cnt++;
                        }
                    }
                }
            }
            else if (p_buffer_cfg->stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                if (media_buffer_downstream_dsp(buffer_ent, true) == false)
                {
                    buffer_ent->miss_ack_cnt++;
                }
            }
        }
        break;

    case AUDIO_MSG_NOTIFICATION_START:
        {
            T_AUDIO_MSG_PAYLOAD_NOTIFICATION_START *payload;
            T_AUDIO_EVENT_PARAM param;

            payload = buf;

            if (payload->type == NOTIFICATION_TYPE_RINGTONE)
            {
                param.ringtone_started.index = payload->index;
                ret = audio_mgr_event_post(AUDIO_EVENT_RINGTONE_STARTED, &param, sizeof(param));
            }
            else if (payload->type == NOTIFICATION_TYPE_VP)
            {
                param.voice_prompt_started.index = payload->index;
                ret = audio_mgr_event_post(AUDIO_EVENT_VOICE_PROMPT_STARTED, &param, sizeof(param));
            }
            else if (payload->type == NOTIFICATION_TYPE_TTS)
            {
                param.tts_started.handle = payload->handle;
                ret = audio_mgr_event_post(AUDIO_EVENT_TTS_STARTED, &param, sizeof(param));
            }
        }
        break;

    case AUDIO_MSG_NOTIFICATION_STOP:
        {
            T_AUDIO_MSG_PAYLOAD_NOTIFICATION_STOP *payload;
            T_AUDIO_EVENT_PARAM param;

            payload = buf;

            if (payload->type == NOTIFICATION_TYPE_RINGTONE)
            {
                param.ringtone_stopped.index = payload->index;
                ret = audio_mgr_event_post(AUDIO_EVENT_RINGTONE_STOPPED, &param, sizeof(param));
            }
            else if (payload->type == NOTIFICATION_TYPE_VP)
            {
                param.voice_prompt_stopped.index = payload->index;
                ret = audio_mgr_event_post(AUDIO_EVENT_VOICE_PROMPT_STOPPED, &param, sizeof(param));
            }
            else if (payload->type == NOTIFICATION_TYPE_TTS)
            {
                param.tts_stopped.handle = payload->handle;
                ret = audio_mgr_event_post(AUDIO_EVENT_TTS_STOPPED, &param, sizeof(param));
            }
        }
        break;

    case AUDIO_MSG_TTS_BUFFER_EMPTY:
        {
            T_AUDIO_EVENT_PARAM param;

            param.tts_alerted.handle = (T_TTS_HANDLE)buf;

            ret = audio_mgr_event_post(AUDIO_EVENT_TTS_ALERTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TTS_BUFFER_LOW:
        {
            T_AUDIO_EVENT_PARAM param;

            param.tts_alerted.handle = (T_TTS_HANDLE)buf;

            ret = audio_mgr_event_post(AUDIO_EVENT_TTS_ALERTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TTS_BUFFER_HIGH:
        {
            T_AUDIO_EVENT_PARAM param;

            param.tts_paused.handle = (T_TTS_HANDLE)buf;

            ret = audio_mgr_event_post(AUDIO_EVENT_TTS_PAUSED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TTS_BUFFER_NORMAL:
        {
            T_AUDIO_EVENT_PARAM param;

            param.tts_resumed.handle = (T_TTS_HANDLE)buf;

            ret = audio_mgr_event_post(AUDIO_EVENT_TTS_RESUMED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TTS_BUFFER_FULL:
        {
            T_AUDIO_EVENT_PARAM param;

            param.tts_exceeded.handle = (T_TTS_HANDLE)buf;

            ret = audio_mgr_event_post(AUDIO_EVENT_TTS_EXCEEDED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_CREATE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_CREATE *payload;

            payload = buf;

            /* TODO bind to media buffer according to stream type. */

            param.track_state_changed.handle = payload->handle;
            param.track_state_changed.state  = AUDIO_TRACK_STATE_CREATED;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_STATE_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_START:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_START *payload;
            T_MEDIA_BUFFER_CFG *p_cfg;
            T_MEDIA_BUFFER_ENTITY *buffer_ent;

            payload = buf;

            p_cfg = (T_MEDIA_BUFFER_CFG *)payload->buffer_proxy;
            if (p_cfg == NULL)
            {
                break;
            }

            buffer_ent = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;

            param.track_state_changed.handle = payload->handle;
            param.track_state_changed.state  = AUDIO_TRACK_STATE_STARTED;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_STATE_CHANGED, &param, sizeof(param));

            if (buffer_ent->paused)
            {
                buffer_ent->paused = 0;
            }
        }
        break;

    case AUDIO_MSG_TRACK_RESTART:
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_RESTART *payload;
            T_MEDIA_BUFFER_CFG *p_cfg;
            T_AUDIO_EVENT_PARAM param;

            payload = buf;

            p_cfg = (T_MEDIA_BUFFER_CFG *)payload->buffer_proxy;
            if (p_cfg == NULL)
            {
                break;
            }

            param.track_state_changed.handle = payload->handle;
            param.track_state_changed.state  = AUDIO_TRACK_STATE_RESTARTED;
            param.track_state_changed.cause = p_cfg->restart_cause;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_STATE_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_SUSPEND_RESUME:
        {

        }
        break;

    case AUDIO_MSG_TRACK_STOP:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_STOP *payload;

            payload = buf;

            /* TODO deactive the media buffer according to stream type. */

            param.track_state_changed.handle = payload->handle;
            param.track_state_changed.state  = AUDIO_TRACK_STATE_STOPPED;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_STATE_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_SUSPEND:
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_PAUSE *payload = buf;
            T_MEDIA_BUFFER_CFG *p_cfg;
            T_MEDIA_BUFFER_ENTITY *buffer_ent;

            p_cfg = (T_MEDIA_BUFFER_CFG *)payload->buffer_proxy;
            if (p_cfg == NULL)
            {
                break;
            }

            buffer_ent = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;

            /*
            uncomment it if you wanna keep data in paused or suspended state
            buffer_ent.reset_wo_flush = 1;
            */
            if (AUDIO_STREAM_TYPE_PLAYBACK == payload->stream_type)
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_WAIT_TO_PLAY)
                {
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        media_buffer_reset_relay(buffer_ent);
                    }
                }
                buffer_ent->ops.reset_fun(buffer_ent);
            }
            else if (AUDIO_STREAM_TYPE_VOICE == payload->stream_type)
            {
                buffer_ent->ops.reset_fun(buffer_ent);
            }
        }
        break;

    case AUDIO_MSG_TRACK_PAUSE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_PAUSE *payload = buf;
            T_MEDIA_BUFFER_CFG *p_cfg;
            T_MEDIA_BUFFER_ENTITY *buffer_ent;

            p_cfg = (T_MEDIA_BUFFER_CFG *)payload->buffer_proxy;
            if (p_cfg == NULL)
            {
                break;
            }

            buffer_ent = (T_MEDIA_BUFFER_ENTITY *)p_cfg->buffer_handle;

            /* TODO deactive the media buffer according to stream type. */

            param.track_state_changed.handle = payload->handle;
            param.track_state_changed.state  = AUDIO_TRACK_STATE_PAUSED;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_STATE_CHANGED, &param, sizeof(param));
            /*
            uncomment it if you wanna keep data in paused or suspended state
            buffer_ent.reset_wo_flush = 1;
            */
            if (AUDIO_STREAM_TYPE_PLAYBACK == payload->stream_type ||
                AUDIO_STREAM_TYPE_VOICE == payload->stream_type)
            {
                buffer_ent->ops.reset_fun(buffer_ent);
                buffer_ent->paused = 1;
            }
        }
        break;

    case AUDIO_MSG_TRACK_RELEASE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_RELEASE *payload;

            payload = buf;

            /* TODO detach the media buffer according to stream type. */

            param.track_state_changed.handle = payload->handle;
            param.track_state_changed.state  = AUDIO_TRACK_STATE_RELEASED;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_STATE_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_VOLUME_OUT_CHANGE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_OUT_CHANGE *payload;

            payload = buf;

            param.track_volume_out_changed.handle = payload->handle;
            param.track_volume_out_changed.prev_volume = payload->prev_volume;
            param.track_volume_out_changed.curr_volume = payload->curr_volume;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_VOLUME_OUT_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_VOLUME_OUT_MUTE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_OUT_MUTE *payload;

            payload = buf;

            param.track_volume_out_muted.handle = payload->handle;
            param.track_volume_out_muted.volume = payload->volume;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_VOLUME_OUT_MUTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_VOLUME_OUT_UNMUTE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_OUT_UNMUTE *payload;

            payload = buf;

            param.track_volume_out_unmuted.handle = payload->handle;
            param.track_volume_out_unmuted.volume = payload->volume;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_VOLUME_OUT_UNMUTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_VOLUME_IN_CHANGE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_IN_CHANGE *payload;

            payload = buf;

            param.track_volume_in_changed.handle = payload->handle;
            param.track_volume_in_changed.prev_volume = payload->prev_volume;
            param.track_volume_in_changed.curr_volume = payload->curr_volume;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_VOLUME_IN_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_VOLUME_IN_MUTE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_IN_MUTE *payload;

            payload = buf;

            param.track_volume_in_muted.handle = payload->handle;
            param.track_volume_in_muted.volume = payload->volume;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_VOLUME_IN_MUTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_TRACK_VOLUME_IN_UNMUTE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_IN_UNMUTE *payload;

            payload = buf;

            param.track_volume_in_unmuted.handle = payload->handle;
            param.track_volume_in_unmuted.volume = payload->volume;
            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_VOLUME_IN_UNMUTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_VOLUME_OUT_MAX_SET:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_MAX_SET *payload;

            payload = buf;
            ret = audio_track_volume_out_max_set(payload->type, payload->volume);
        }
        break;

    case AUDIO_MSG_VOLUME_OUT_MIN_SET:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_MIN_SET *payload;

            payload = buf;
            ret = audio_track_volume_out_min_set(payload->type, payload->volume);
        }
        break;

    case AUDIO_MSG_VOLUME_OUT_CHANGE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_CHANGE *payload;

            payload = buf;
            param.volume_out_changed.type        = payload->type;
            param.volume_out_changed.prev_volume = payload->prev_volume;
            param.volume_out_changed.curr_volume = payload->curr_volume;

            audio_track_volume_out_set_all(payload->type, payload->curr_volume);
            ret = audio_mgr_event_post(AUDIO_EVENT_VOLUME_OUT_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_VOLUME_OUT_MUTE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_MUTE *payload;

            payload = buf;
            param.volume_out_muted.type         = payload->type;
            param.volume_out_muted.volume       = payload->volume;

            audio_track_volume_out_mute_all(payload->type);
            ret = audio_mgr_event_post(AUDIO_EVENT_VOLUME_OUT_MUTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_VOLUME_OUT_UNMUTE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_UNMUTE *payload;

            payload = buf;
            param.volume_out_unmuted.type       = payload->type;
            param.volume_out_unmuted.volume     = payload->volume;

            audio_track_volume_out_unmute_all(payload->type);
            ret = audio_mgr_event_post(AUDIO_EVENT_VOLUME_OUT_UNMUTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_VOLUME_OUT_CHANN_SET:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_CHANN_SET *payload;

            payload = buf;
            ret = dsp_sport0_da_chann_set((uint8_t)payload->mask);
        }
        break;

    case AUDIO_MSG_VOLUME_IN_MAX_SET:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_IN_MAX_SET *payload;

            payload = buf;
            ret = audio_track_volume_in_max_set(payload->type, payload->volume);
        }
        break;

    case AUDIO_MSG_VOLUME_IN_MIN_SET:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_IN_MIN_SET *payload;

            payload = buf;
            ret = audio_track_volume_in_min_set(payload->type, payload->volume);
        }
        break;

    case AUDIO_MSG_VOLUME_IN_CHANGE:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_IN_CHANGE *payload;
            T_AUDIO_EVENT_PARAM param;

            payload = buf;
            param.volume_in_changed.type        = payload->type;
            param.volume_in_changed.prev_volume = payload->prev_volume;
            param.volume_in_changed.curr_volume = payload->curr_volume;

            audio_track_volume_in_set_all(payload->type, payload->curr_volume);
            ret = audio_mgr_event_post(AUDIO_EVENT_VOLUME_IN_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_VOLUME_IN_MUTED:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_IN_MUTE *payload;
            T_AUDIO_EVENT_PARAM param;

            payload = buf;
            param.volume_in_muted.type      = payload->type;
            param.volume_in_muted.volume    = payload->volume;

            audio_track_volume_in_mute_all(payload->type);
            ret = audio_mgr_event_post(AUDIO_EVENT_VOLUME_IN_MUTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_VOLUME_IN_UNMUTE:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_IN_UNMUTE *payload;
            T_AUDIO_EVENT_PARAM param;

            payload = buf;
            param.volume_in_unmuted.type    = payload->type;
            param.volume_in_unmuted.volume  = payload->volume;

            audio_track_volume_in_unmute_all(payload->type);
            ret = audio_mgr_event_post(AUDIO_EVENT_VOLUME_IN_UNMUTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_VOLUME_IN_CHANN_SET:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_IN_CHANN_SET *payload;

            payload = buf;
            ret = dsp_sport0_ad_chann_set((uint8_t)payload->mask);
        }
        break;

    case AUDIO_MSG_VOLUME_BALANCE_CHANGE:
        {
            T_AUDIO_MSG_PAYLOAD_VOLUME_BALANCE_CHANGE *payload;
            T_AUDIO_EVENT_PARAM param;

            payload = buf;
            param.volume_balance_changed.type       = payload->type;
            param.volume_balance_changed.prev_scale = payload->prev_scale;
            param.volume_balance_changed.curr_scale = payload->curr_scale;

            audio_track_volume_balance_set(payload->type, payload->curr_scale);
            ret = audio_mgr_event_post(AUDIO_EVENT_VOLUME_BALANCE_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_VOICE_ACTIVITY_DETECTED:
        {
            T_AUDIO_EVENT_PARAM param;
            ret = audio_mgr_event_post(AUDIO_EVENT_VOICE_ACTIVITY_DETECTED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_DSP_RETURN_JOIN_INFO:
        {
            T_MEDIA_BUFFER_ENTITY *buffer_ent;
            buffer_ent = audio_mgr_get_playing_buffer(AUDIO_STREAM_TYPE_PLAYBACK);
            if (buffer_ent)
            {
                audio_remote_dsp_seamless_join_handle(buffer_ent, buf);
            }
            else
            {
                buffer_ent = audio_mgr_get_playing_buffer(AUDIO_STREAM_TYPE_VOICE);
                if (buffer_ent)
                {
                    audio_remote_dsp_seamless_join_handle(buffer_ent, buf);
                }
            }
        }
        break;

    case AUDIO_MSG_TRACK_SIGNAL_OUT_REFRESH:
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_SIGNAL_OUT_REFRESH *param = (T_AUDIO_MSG_PAYLOAD_TRACK_SIGNAL_OUT_REFRESH
                                                                   *)buf;
            T_AUDIO_EVENT_PARAM payload;

            payload.track_signal_out_refreshed.handle = param->handle;
            payload.track_signal_out_refreshed.left_gain = param->left_gain;
            payload.track_signal_out_refreshed.right_gain = param->right_gain;

            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_SIGNAL_OUT_REFRESHED, &payload, sizeof(payload));
        }
        break;

    case AUDIO_MSG_STREAM_LATENCY_REPORT:
        {
            T_MEDIA_BUFFER_ENTITY *buffer_ent;
            T_AUDIO_MSG_STREAM_LATENCY_RPT_EVENT *param = (T_AUDIO_MSG_STREAM_LATENCY_RPT_EVENT *)buf;
            uint32_t timestamps[2] = {0};
            uint32_t average_latency_us = param->average_packet_latency * 625;
            uint32_t average_fifo_queuing = param->average_fifo_queuing;
            T_AUDIO_EVENT_PARAM payload;

            buffer_ent = audio_mgr_get_playing_buffer(AUDIO_STREAM_TYPE_PLAYBACK);
            if (buffer_ent == NULL || buffer_ent->p_cfg == NULL)
            {
                break;
            }

            payload.track_latency_reported.handle = buffer_ent->p_cfg->track_handle;
            payload.track_latency_reported.preset_latency = buffer_ent->p_cfg->latency;
            payload.track_latency_reported.instant_latency = (average_fifo_queuing + average_latency_us) / 1000;
            payload.track_latency_reported.dejitter_compensation = jitter_buffer_compensation_get(
                                                                       buffer_ent->jitter_buffer_handle);
            payload.track_latency_reported.dsp_plc_sum = param->dsp_plc_sum;

            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_LATENCY_REPORTED, &payload, sizeof(payload));

            AUDIO_PRINT_TRACE5("latency_report: count %u avg %uus %u %u dsp_plc 0x%04x",
                               param->normal_packet_count,
                               average_latency_us, average_fifo_queuing, (average_fifo_queuing + average_latency_us) / 1000,
                               param->dsp_plc_sum);

            media_buffer_packet_record_get_timestamps(buffer_ent->audio_remote_handle, timestamps, 2);

            if (buffer_ent->p_cfg->usage != AUDIO_STREAM_USAGE_LOCAL)
            {
                if (average_fifo_queuing == 0 ||
                    (timestamps[0] == 0 && timestamps[1] == 0) ||
                    (timestamps[0] == 0xffffffff && timestamps[1] == 0xffffffff) ||
                    average_latency_us > 500000)
                {
                    AUDIO_PRINT_TRACE0("latency_report: timestamps improbable");
                    break;
                }
            }

            audio_latency_record_update(buffer_ent->audio_latency_handle,
                                        average_latency_us + average_fifo_queuing);

            if (buffer_ent->ignore_latency_report)
            {
                buffer_ent->ignore_latency_report--;
                break;
            }

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                break;
            }

            if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY)
            {
                if (remote_session_state_get() == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    jitter_buffer_asrc_pid_controller(buffer_ent->jitter_buffer_handle, average_latency_us,
                                                      average_fifo_queuing);
                }
                else
                {
                    jitter_buffer_low_latency_keeper(buffer_ent->jitter_buffer_handle,
                                                     average_latency_us + average_fifo_queuing);
                }
            }
            else
            {
                if (jitter_buffer_asrc_pid_is_block(buffer_ent->jitter_buffer_handle))
                {
                    break;
                }
                jitter_buffer_asrc_pid_controller(buffer_ent->jitter_buffer_handle, average_latency_us,
                                                  average_fifo_queuing);
            }
        }
        break;

    case AUDIO_MSG_DSP_PLC:
        {
            T_AUDIO_MSG_DSP_PLC_EVENT *plc;
            T_MEDIA_BUFFER_ENTITY *buffer_ent;

            plc = (T_AUDIO_MSG_DSP_PLC_EVENT *)buf;

            buffer_ent = audio_mgr_get_playing_buffer(AUDIO_STREAM_TYPE_PLAYBACK);
            if (buffer_ent == NULL)
            {
                break;
            }

            if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_PCM)
            {
                audio_mgr_exception(buffer_ent, MEDIA_BUFFER_DSP_PLC_ERR, 0, 0);
                break;
            }

            AUDIO_PRINT_INFO7("dsp_plc_event:last local %u total %u counter %u local seq %u plc num %d max plc %u cur plc %u",
                              buffer_ent->ds_local_seq_last, plc->total_frames,
                              plc->frame_counter, plc->local_seq, plc->plc_frame_num,
                              buffer_ent->max_latency_plc_count, buffer_ent->p_cfg->plc_count);
            media_buffer_handle_plc_event(buffer_ent, plc->total_frames, plc->frame_counter, plc->local_seq,
                                          plc->plc_frame_num);
        }
        break;

    case AUDIO_MSG_DECODER_PLC_NOTIFY:
        {
            T_AUDIO_MSG_DECODER_PLC_NOTIFY_EVENT *plc_notify;
            T_MEDIA_BUFFER_ENTITY *buffer_ent;
            T_AUDIO_EVENT_PARAM param;

            buffer_ent = audio_mgr_get_playing_buffer(AUDIO_STREAM_TYPE_PLAYBACK);
            if (buffer_ent == NULL)
            {
                break;
            }

            plc_notify = (T_AUDIO_MSG_DECODER_PLC_NOTIFY_EVENT *)buf;
            param.track_decoder_plc_notify_reported.handle = buffer_ent->p_cfg->track_handle;
            param.track_decoder_plc_notify_reported.plc_sample_num = plc_notify->plc_sample_num;
            param.track_decoder_plc_notify_reported.total_sample_num = plc_notify->total_sample_num;
            param.track_decoder_plc_notify_reported.continue_sample_num = plc_notify->continue_sample_num;

            ret = audio_mgr_event_post(AUDIO_EVENT_TRACK_DECODER_PLC_NOTIFY_REPORTED, &param, sizeof(param));

            AUDIO_PRINT_INFO3("decoder_plc_notify: plc_sample_num %u total_sample_num %u continue_sample_num %u",
                              plc_notify->plc_sample_num, plc_notify->total_sample_num, plc_notify->continue_sample_num);
        }
        break;

    case AUDIO_MSG_ANC_ENABLED:
        {
            ret = audio_mgr_event_post(AUDIO_EVENT_ANC_ENABLED, NULL, 0);
        }
        break;

    case AUDIO_MSG_ANC_DISABLED:
        {
            ret = audio_mgr_event_post(AUDIO_EVENT_ANC_DISABLED, NULL, 0);
        }
        break;

    case AUDIO_MSG_ANC_ADAPTIVE_FILTER_STARTED:
        {
            ret = audio_mgr_event_post(AUDIO_EVENT_ANC_ADAPTIVE_FILTER_STARTED, NULL, 0);
        }
        break;

    case AUDIO_MSG_ANC_ADAPTIVE_FILTER_PAUSED:
        {
            ret = audio_mgr_event_post(AUDIO_EVENT_ANC_ADAPTIVE_FILTER_PAUSED, NULL, 0);
        }
        break;

    case AUDIO_MSG_ANC_ADAPTIVE_FILTER_STOPPED:
        {
            ret = audio_mgr_event_post(AUDIO_EVENT_ANC_ADAPTIVE_FILTER_STOPPED, NULL, 0);
        }
        break;

    case AUDIO_MSG_PASSTHROUGH_ENABLED:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_PASSTHROUGH_ENABLE *payload;

            payload = (T_AUDIO_MSG_PAYLOAD_PASSTHROUGH_ENABLE *)buf;
            param.passthrough_started.mode = payload->mode;

            ret = audio_mgr_event_post(AUDIO_EVENT_PASSTHROUGH_ENABLED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_PASSTHROUGH_DISABLED:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_PASSTHROUGH_DISABLE *payload;

            payload = (T_AUDIO_MSG_PAYLOAD_PASSTHROUGH_DISABLE *)buf;
            param.passthrough_stopped.mode = payload->mode;

            ret = audio_mgr_event_post(AUDIO_EVENT_PASSTHROUGH_DISABLED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_LINE_IN_START:
        {
            ret =  audio_mgr_event_post(AUDIO_EVENT_LINE_IN_STARTED, NULL, 0);
        }
        break;

    case AUDIO_MSG_LINE_IN_STOP:
        {
            ret = audio_mgr_event_post(AUDIO_EVENT_LINE_IN_STOPPED, NULL, 0);
        }
        break;

    case AUDIO_MSG_LINE_IN_VOLUME_OUT_CHANGE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_LINE_IN_VOLUME_OUT_CHANGE *payload;

            payload = buf;
            param.line_in_volume_out_changed.prev_volume = payload->prev_volume;
            param.line_in_volume_out_changed.curr_volume = payload->curr_volume;

            ret = audio_mgr_event_post(AUDIO_EVENT_LINE_IN_VOLUME_OUT_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_LINE_IN_VOLUME_IN_CHANGE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_LINE_IN_VOLUME_IN_CHANGE *payload;

            payload = buf;
            param.line_in_volume_in_changed.prev_volume = payload->prev_volume;
            param.line_in_volume_in_changed.curr_volume = payload->curr_volume;

            ret = audio_mgr_event_post(AUDIO_EVENT_LINE_IN_VOLUME_IN_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_LINE_IN_VOLUME_BALANCE_CHANGE:
        {
            T_AUDIO_EVENT_PARAM param;
            T_AUDIO_MSG_PAYLOAD_LINE_IN_VOLUME_BALANCE_CHANGE *payload;

            payload = buf;
            param.line_in_volume_balance_changed.prev_scale = payload->prev_scale;
            param.line_in_volume_balance_changed.curr_scale = payload->curr_scale;

            ret = audio_mgr_event_post(AUDIO_EVENT_LINE_IN_VOLUME_BALANCE_CHANGED, &param, sizeof(param));
        }
        break;

    case AUDIO_MSG_LINE_IN_SIGNAL_OUT_REFRESH:
        {
            T_AUDIO_MSG_PAYLOAD_LINE_IN_SIGNAL_OUT_REFRESH *param =
                (T_AUDIO_MSG_PAYLOAD_LINE_IN_SIGNAL_OUT_REFRESH *)buf;
            T_AUDIO_EVENT_PARAM payload;

            payload.line_in_signal_out_refreshed.left_gain = param->left_gain;
            payload.line_in_signal_out_refreshed.right_gain = param->right_gain;

            ret = audio_mgr_event_post(AUDIO_EVENT_LINE_IN_SIGNAL_OUT_REFRESHED, &payload, sizeof(payload));
        }
        break;

    default:
        break;
    }

    return ret;
}

void audio_mgr_relay_cback(uint16_t event, T_REMOTE_RELAY_STATUS status, void *buf, uint16_t len)
{
    AUDIO_PRINT_TRACE2("audio_mgr_relay_cback: event 0x%04x, status %u", event, status);

    switch (event)
    {
    case AUDIO_REMOTE_SYNC_PARAM:
        {
            T_AUDIO_SYNC_PARAMS *p_msg = (T_AUDIO_SYNC_PARAMS *)buf;
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_remote_seamless_join_set(p_msg->seamless_join, p_msg->force_join);
            }
        }
        break;

    case AUDIO_REMOTE_SEAMLESS_JOIN_SET:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                T_AUDIO_REMOTE_SEAMLESS_JOIN_SET *p_msg = (T_AUDIO_REMOTE_SEAMLESS_JOIN_SET *)buf;
                audio_remote_seamless_join_set(p_msg->enable, p_msg->force_join);
            }
        }
        break;

    case AUDIO_REMOTE_DSP_INTER_MSG:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                audio_path_synchronization_data_send(NULL, (uint8_t *)buf, ((uint8_t *)buf)[0]);
            }
        }
        break;

    default:
        break;
    }
}

bool audio_bt_ipc_cback(uint32_t id, void *msg)
{
    switch (id)
    {
    case BT_IPC_REMOTE_CONNECTED:
        {
            T_AUDIO_SYNC_PARAMS msg;
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                msg.seamless_join = audio_remote_db->seamless_join;
                msg.force_join = audio_remote_db->force_join;
                remote_async_msg_relay(audio_db->relay_handle,
                                       AUDIO_REMOTE_SYNC_PARAM,
                                       &msg,
                                       sizeof(T_AUDIO_SYNC_PARAMS),
                                       false);
            }
        }
        break;

    case BT_IPC_REMOTE_SWAP_START:
        {
            uint16_t role = remote_session_role_get();
            audio_path_synchronization_role_swap(NULL, role, true);
        }
        break;

    case BT_IPC_REMOTE_SWAP_STOP:
        {
            uint16_t role = remote_session_role_get();
            audio_path_synchronization_role_swap(NULL, role, false);
        }
        break;

    case BT_IPC_REMOTE_DISCONNECTED:
        {
            T_MEDIA_BUFFER_ENTITY *buffer_ent;

            buffer_ent = audio_mgr_get_playing_buffer(AUDIO_STREAM_TYPE_PLAYBACK);
            notification_sync_lost();
            if (buffer_ent)
            {
                audio_remote_reset(buffer_ent->audio_remote_handle);
                jitter_buffer_asrc_pid_unblock(buffer_ent->jitter_buffer_handle);
            }
        }
        break;

    default:
        break;
    }

    return true;
}

bool audio_sys_ipc_cback(uint32_t id, void *msg)
{
    switch (id)
    {
    case SYS_IPC_POWER_ON:
        {

        }
        break;

    case SYS_IPC_POWER_OFF:
        {
            audio_path_power_off();
        }
        break;

    default:
        break;
    }

    return true;
}

bool audio_mgr_init(uint16_t playback_pool_size,
                    uint16_t voice_pool_size,
                    uint16_t record_pool_size,
                    uint16_t notification_pool_size)
{
    int32_t ret = 0;

    audio_db = os_mem_zalloc2(sizeof(T_AUDIO_DB));
    if (audio_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    audio_db->playback_pool_size = playback_pool_size;
    audio_db->voice_pool_size = voice_pool_size;
    audio_db->record_pool_size = record_pool_size;
    audio_db->latency_keeper_enable = true;
    audio_db->latency_keeper_policy = LATENCY_KEEPER_POLICY_BY_ASRC;
    audio_db->max_plc_count = DEFAULT_RESET_PLC_COUNT;

    if (audio_path_init() == false)
    {
        ret = 2;
        goto fail_init_audio_path;
    }

    if (audio_remote_init() == false)
    {
        ret = 3;
        goto fail_init_audio_remote;
    }

    if (audio_volume_init() == false)
    {
        ret = 4;
        goto fail_init_audio_volume;
    }

    if (audio_track_init() == false)
    {
        ret = 5;
        goto fail_init_audio_track;
    }

    if (line_in_init() == false)
    {
        ret = 6;
        goto fail_init_line_in;
    }

    if (audio_effect_init() == false)
    {
        ret = 7;
        goto fail_init_audio_effect;
    }

    if (notification_init(notification_pool_size) == false)
    {
        ret = 8;
        goto fail_init_notification;
    }

    if (anc_init() == false)
    {
        ret = 9;
        goto fail_init_anc;
    }

    if (audio_passthrough_init() == false)
    {
        ret = 10;
        goto fail_init_audio_passthrough;
    }

    if (vad_init() == false)
    {
        ret = 11;
        goto fail_init_vad;
    }

    if (sidetone_init() == false)
    {
        ret = 12;
        goto fail_init_sidetone;
    }

    audio_db->relay_handle = remote_relay_register(audio_mgr_relay_cback);
    if (audio_db->relay_handle == NULL)
    {
        ret = 13;
        goto fail_register_remote_relay;
    }

    audio_db->sys_ipc_handle = sys_ipc_subscribe(SYS_IPC_TOPIC, audio_sys_ipc_cback);
    if (audio_db->sys_ipc_handle == NULL)
    {
        ret = 14;
        goto fail_subscribe_sys_ipc_cback;
    }

    audio_db->bt_ipc_handle = sys_ipc_subscribe(BT_IPC_TOPIC, audio_bt_ipc_cback);
    if (audio_db->bt_ipc_handle == NULL)
    {
        ret = 15;
        goto fail_subscribe_bt_ipc_cback;
    }

    if (jb_init() == false)
    {
        ret = 16;
        goto fail_init_jb;
    }

    if (audio_latency_mgr_init() == false)
    {
        ret = 17;
        goto fail_init_audio_latency;
    }

    if (audio_interval_eval_init() == false)
    {
        ret = 18;
        goto fail_init_audio_interval_eval;
    }

    if (audio_pipe_init() == false)
    {
        ret = 19;
        goto fail_init_audio_pipe;
    }

    os_queue_init(&audio_db->cback_list);

    if (audio_path_cback_register(audio_mgr_dsp_msg_pipe) == false)
    {
        ret = 20;
        goto fail_cback_register;
    }

    if (media_buffer_init() == false)
    {
        ret = 21;
        goto fail_init_media_buffer;
    }

    return true;

fail_init_media_buffer:
    media_buffer_deinit();
fail_cback_register:
fail_init_audio_pipe:
    audio_interval_eval_deinit();
fail_init_audio_interval_eval:
    audio_latency_mgr_deinit();
fail_init_audio_latency:
    jb_deinit();
fail_init_jb:
    sys_ipc_unsubscribe(audio_db->bt_ipc_handle);
fail_subscribe_bt_ipc_cback:
    sys_ipc_unsubscribe(audio_db->sys_ipc_handle);
fail_subscribe_sys_ipc_cback:
    remote_relay_unregister(audio_db->relay_handle);
fail_register_remote_relay:
    sidetone_deinit();
fail_init_sidetone:
    vad_deinit();
fail_init_vad:
    audio_passthrough_deinit();
fail_init_audio_passthrough:
    anc_deinit();
fail_init_anc:
    notification_deinit();
fail_init_notification:
    audio_effect_deinit();
fail_init_audio_effect:
    line_in_deinit();
fail_init_line_in:
    audio_track_deinit();
fail_init_audio_track:
    audio_volume_deinit();
fail_init_audio_volume:
    audio_remote_deinit();
fail_init_audio_remote:
    audio_path_deinit();
fail_init_audio_path:
    os_mem_free(audio_db);
    audio_db = NULL;
fail_alloc_db:
    AUDIO_PRINT_ERROR1("audio_mgr_init: failed %d", -ret);
    return false;
}

void audio_mgr_set_max_plc_count(uint8_t plc_count)
{

    if (plc_count >= 1 && plc_count <= 7)
    {
        audio_db->max_plc_count = plc_count;
    }
}


void audio_mgr_exception(T_MEDIA_BUFFER_ENTITY *buffer_ent, T_AUDIO_MGR_EXCEPTION exc,
                         uint8_t from_remote, uint32_t param)
{
    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }

    buffer_ent->exception = exc;
    if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
    {
        T_AUDIO_EXC_CMD cmd;
        cmd.exc = exc;
        cmd.stream_type = buffer_ent->p_cfg->stream_type;
        cmd.param = param;

        if (exc != DSP_SYNC_EMPTY &&
            exc != DSP_DECODE_EMPTY &&
            exc != MEDIA_BUFFER_PRI_RESET_BUFFER &&
            exc != MEDIA_BUFFER_PACKET_LOST &&
            exc != MEDIA_BUFFER_JBC_CLK_ERR &&
            exc != MEDIA_BUFFER_REMOTE_STATE_ERR)
        {
            if (media_buffer_get_status(buffer_ent) != MEDIA_BUFFER_UNINIT)
            {
                media_buffer_relay_exception(buffer_ent, (uint8_t *)&cmd);
            }
            else
            {
                AUDIO_PRINT_TRACE0("audio_mgr_exc: not report exception if buffer is uninit");
            }
        }
    }
    else
    {
        if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING)
        {
            if (audio_remote_get_state(buffer_ent->audio_remote_handle) == AUDIO_REMOTE_STATE_SEAMLESS_JOIN)
            {
                audio_remote_cancel_seamless_join(buffer_ent->audio_remote_handle);
            }

            if (audio_remote_db->force_join && from_remote)
            {
                if (exc == MEDIA_BUFFER_FULL || exc == DSP_UNSYNC)
                {
                    AUDIO_PRINT_INFO1("audio_mgr_exc: pri ignores sec exceptions 0x%x", exc);
                    return;
                }
            }
        }
    }

    AUDIO_PRINT_INFO4("audio_mgr_exception2: 0x%x from_remote %d para 0x%x buffer state %u", exc,
                      from_remote, param, media_buffer_get_status(buffer_ent));
    if ((exc & 0x0f00) == DSP_EXC_GROUP && from_remote == 0 &&
        media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING &&
        buffer_ent->delay_reset_path == false)
    {
        buffer_ent->p_cfg->restart_cause = convert_exception_to_restart_casuse(exc);
        buffer_ent->delay_reset_path = true;
    }

    if (media_buffer_get_status(buffer_ent) != MEDIA_BUFFER_UNINIT)
    {
        switch (exc)
        {
        case MEDIA_BUFFER_FULL:
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING && from_remote == 0)
                {
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        if (buffer_ent->p_cfg->mode != AUDIO_STREAM_MODE_LOW_LATENCY &&
                            buffer_ent->p_cfg->mode != AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                        {
                            bool dynamic_latency_on = false;
                            audio_latency_get_dynamic_latency_on(buffer_ent->audio_latency_handle,
                                                                 &dynamic_latency_on);
                            if (dynamic_latency_on)
                            {
                                media_buffer_dynamic_latency_resume(buffer_ent);
                            }
                        }
                    }
                }
                buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                          BUFFER_DIR_DOWNSTREAM);
                audio_mgr_dispatch(AUDIO_MSG_BUFFER_RESET, buffer_ent);
            }
            break;

        case MEDIA_BUFFER_FST_SYNC_ERR:
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (from_remote == 1)
                    {
                        if (buffer_ent->media_buffer_fst_sync_tid == (uint8_t)(param & 0x00ff) &&
                            (param >> 16) == FST_SYNC_STATE_ERROR)
                        {
                            buffer_ent->ops.flush_fun(buffer_ent,
                                                      media_buffer_ds_pkt_cnt(buffer_ent) - MEDIA_SYNC_TRIGGER_CNT, BUFFER_DIR_DOWNSTREAM);
                            if (media_buffer_ds_pkt_cnt(buffer_ent) >= MEDIA_SYNC_TRIGGER_CNT)
                            {
                                audio_remote_align_buffer(buffer_ent->audio_remote_handle);
                            }
                        }
                    }
                    else
                    {
                        if ((param >> 16) == FST_SYNC_DATA_EXPIRED)
                        {
                            if (media_buffer_ds_pkt_cnt(buffer_ent) > MEDIA_SYNC_TRIGGER_CNT)
                            {
                                buffer_ent->ops.flush_fun(buffer_ent,
                                                          media_buffer_ds_pkt_cnt(buffer_ent) - MEDIA_SYNC_TRIGGER_CNT, BUFFER_DIR_DOWNSTREAM);
                            }
                            audio_remote_align_buffer(buffer_ent->audio_remote_handle);
                        }
                        else
                        {
                            if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PREQUEUE)
                            {
                                if (media_buffer_ds_pkt_cnt(buffer_ent) - MEDIA_SYNC_TRIGGER_CNT > 0)
                                {
                                    buffer_ent->ops.flush_fun(buffer_ent,
                                                              media_buffer_ds_pkt_cnt(buffer_ent) - MEDIA_SYNC_TRIGGER_CNT, BUFFER_DIR_DOWNSTREAM);
                                }
                                audio_remote_align_buffer(buffer_ent->audio_remote_handle);

                            }
                            else
                            {
                                audio_mgr_dispatch(AUDIO_MSG_BUFFER_RESET, buffer_ent);
                            }
                        }
                    }
                }
                else
                {
                    audio_mgr_dispatch(AUDIO_MSG_BUFFER_RESET, buffer_ent);
                }
            }
            break;

        case DSP_DECODE_EMPTY:
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PLAYING && from_remote == 0)
                {
                    if (buffer_ent->p_cfg->mode != AUDIO_STREAM_MODE_LOW_LATENCY &&
                        buffer_ent->p_cfg->mode != AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
                    {
                        bool dynamic_latency_on = false;
                        audio_latency_get_dynamic_latency_on(buffer_ent->audio_latency_handle,
                                                             &dynamic_latency_on);
                        if (buffer_ent->p_cfg->latency_fixed == false && dynamic_latency_on == false)
                        {
                            media_buffer_dynamic_latency_set(buffer_ent,
                                                             buffer_ent->p_cfg->latency_back_up * (1 + NORMAL_MODE_DYNAMIC_LATENCY_RATIO), true);
                        }
                    }
                }
                audio_mgr_dispatch(AUDIO_MSG_BUFFER_RESET, buffer_ent);

            }
            break;

        case MEDIA_BUFFER_PACKET_LOST:
            {
                if (media_buffer_get_status(buffer_ent) == MEDIA_BUFFER_PREQUEUE)
                {
                    buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                              BUFFER_DIR_DOWNSTREAM);
                    buffer_ent->last_seq_updated = false;
                }
                audio_mgr_dispatch(AUDIO_MSG_BUFFER_RESET, buffer_ent);
            }
            break;

        default:
            {
                audio_mgr_dispatch(AUDIO_MSG_BUFFER_RESET, buffer_ent);
            }
            break;
        }
    }
    else
    {
        if (exc == MEDIA_BUFFER_FULL)
        {
            buffer_ent->ops.flush_fun(buffer_ent, media_buffer_ds_pkt_cnt(buffer_ent),
                                      BUFFER_DIR_DOWNSTREAM);
            audio_mgr_dispatch(AUDIO_MSG_BUFFER_RESET, buffer_ent);
        }
    }
}

bool audio_remote_join_set(bool enable, bool force_join)
{
    T_AUDIO_REMOTE_SEAMLESS_JOIN_SET msg;

    msg.enable = enable;
    msg.force_join = force_join;

    audio_remote_seamless_join_set(enable, force_join);
    remote_async_msg_relay(audio_db->relay_handle,
                           AUDIO_REMOTE_SEAMLESS_JOIN_SET,
                           &msg,
                           sizeof(T_AUDIO_REMOTE_SEAMLESS_JOIN_SET),
                           false);
    return true;
}
