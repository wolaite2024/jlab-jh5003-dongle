/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "trace.h"
#include "audio_mgr.h"
#include "audio_latency.h"
#include "os_queue.h"
#include "os_mem.h"
#include "audio_interval_eval.h"

#define MAX_ULTRA_LOW_PLC_LATENCY 80000

typedef enum
{
    AUDIO_LATENCY_TIMER_DYNAMIC_DEC_CHECK,
    AUDIO_LATENCY_TIMER_DYNAMIC_LAT_RESUME,
    AUDIO_LATENCY_TIMER_DYNAMIC_CHECK,
} T_AUDIO_LATENCY_TIMER;

typedef struct
{
    T_OS_QUEUE  handles;
    uint8_t id;
} T_AUDIO_LATENCY_DB;

typedef struct t_audio_latency_buffer_db
{
    struct t_audio_latency_buffer_db *p_next;
    T_MEDIA_BUFFER_ENTITY            *buffer_ent;
    T_AUDIO_LATENCY_EVT_CBACK         cback;
    void    *dynamic_latency_check_timer_handle;
    void    *dynamic_latency_dec_check_timer_handle;
    void    *dynamic_latency_keeper_resume_timer_handle;
    uint32_t last_latency_report;
    uint8_t id;
    uint8_t plc_when_interval_stable;
    uint8_t ultra_low_plc_cnt;
    bool    dynamic_latency_on;

} T_AUDIO_LATENCY_BUFFER_DB;

T_AUDIO_LATENCY_DB *audio_latency_db;


bool audio_latency_mgr_check_handle(T_AUDIO_LATENCY_HANDLE handle)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;
    if (buffer_lat_db)
    {
        if (os_queue_search(&audio_latency_db->handles, buffer_lat_db))
        {
            return true;
        }
    }
    return false;
}

bool audio_latency_mgr_init(void)
{
    int ret;

    audio_latency_db = os_mem_zalloc2(sizeof(T_AUDIO_LATENCY_DB));
    if (audio_latency_db == NULL)
    {
        ret = -1;
        goto fail_alloc_audio_latency_db;
    }

    os_queue_init(&audio_latency_db->handles);

    return true;
fail_alloc_audio_latency_db:
    AUDIO_PRINT_TRACE1("audio_latency_mgr_init: failed %d", ret);
    return false;
}

void audio_latency_mgr_deinit(void)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db;

    if (audio_latency_db)
    {
        buffer_lat_db = os_queue_out(&audio_latency_db->handles);
        while (buffer_lat_db != NULL)
        {
            if (buffer_lat_db->dynamic_latency_check_timer_handle)
            {
                sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
            }
            if (buffer_lat_db->dynamic_latency_dec_check_timer_handle)
            {
                sys_timer_delete(buffer_lat_db->dynamic_latency_dec_check_timer_handle);
            }
            if (buffer_lat_db->dynamic_latency_keeper_resume_timer_handle)
            {
                sys_timer_delete(buffer_lat_db->dynamic_latency_keeper_resume_timer_handle);
            }
            os_mem_free(buffer_lat_db);
            buffer_lat_db = os_queue_out(&audio_latency_db->handles);
        }
        os_mem_free(audio_latency_db);
        audio_latency_db = NULL;
    }
}

void audio_latency_reset(T_AUDIO_LATENCY_HANDLE handle)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }

    if (buffer_lat_db->dynamic_latency_check_timer_handle)
    {
        sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
        buffer_lat_db->dynamic_latency_check_timer_handle = NULL;
    }
    if (buffer_lat_db->dynamic_latency_dec_check_timer_handle)
    {
        sys_timer_delete(buffer_lat_db->dynamic_latency_dec_check_timer_handle);
        buffer_lat_db->dynamic_latency_dec_check_timer_handle = NULL;
    }
    if (buffer_lat_db->dynamic_latency_keeper_resume_timer_handle)
    {
        sys_timer_delete(buffer_lat_db->dynamic_latency_keeper_resume_timer_handle);
        buffer_lat_db->dynamic_latency_keeper_resume_timer_handle = NULL;
    }
}

void audio_latency_buffer_reset(T_AUDIO_LATENCY_HANDLE handle)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }
    buffer_ent = buffer_lat_db->buffer_ent;

    if (buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_LOW_LATENCY ||
        buffer_ent->p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
    {
        if (buffer_lat_db->dynamic_latency_on == false)
        {
            buffer_ent->p_cfg->plc_count = 0;
            buffer_ent->p_cfg->latency = buffer_ent->p_cfg->latency_back_up;
        }
        buffer_lat_db->ultra_low_plc_cnt = 0;
    }
    if (buffer_lat_db->dynamic_latency_check_timer_handle)
    {
        sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
        buffer_lat_db->dynamic_latency_check_timer_handle = NULL;
        buffer_lat_db->plc_when_interval_stable = 0;
    }
    buffer_lat_db->last_latency_report = 0;
}

T_AUDIO_LATENCY_HANDLE audio_latency_mgr_register(void *buffer_ent, T_AUDIO_LATENCY_EVT_CBACK cback)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db;
    buffer_lat_db = os_mem_zalloc2(sizeof(T_AUDIO_LATENCY_BUFFER_DB));
    if (buffer_lat_db == NULL)
    {
        return NULL;
    }
    buffer_lat_db->buffer_ent = buffer_ent;
    buffer_lat_db->cback = cback;
    buffer_lat_db->id = audio_latency_db->id;
    audio_latency_db->id++;

    os_queue_in(&audio_latency_db->handles, buffer_lat_db);

    return buffer_lat_db;

}

void audio_latency_mgr_unregister(T_AUDIO_LATENCY_HANDLE handle)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;
    if (buffer_lat_db)
    {
        if (os_queue_search(&audio_latency_db->handles, buffer_lat_db) == true)
        {
            os_queue_delete(&audio_latency_db->handles, buffer_lat_db);
            if (buffer_lat_db->dynamic_latency_check_timer_handle)
            {
                sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
            }
            if (buffer_lat_db->dynamic_latency_dec_check_timer_handle)
            {
                sys_timer_delete(buffer_lat_db->dynamic_latency_dec_check_timer_handle);
            }
            if (buffer_lat_db->dynamic_latency_keeper_resume_timer_handle)
            {
                sys_timer_delete(buffer_lat_db->dynamic_latency_keeper_resume_timer_handle);
            }
            os_mem_free(buffer_lat_db);
        }
    }
}

T_AUDIO_LATENCY_HANDLE audio_latency_find_handle_by_id(uint8_t id)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = NULL;

    buffer_lat_db = os_queue_peek(&audio_latency_db->handles, 0);

    while (buffer_lat_db)
    {
        if (buffer_lat_db->id != id)
        {
            buffer_lat_db = buffer_lat_db->p_next;
        }
        else
        {
            break;
        }
    }
    return buffer_lat_db;
}

void audio_latency_timeout_cback(T_SYS_TIMER_HANDLE handle)
{
    uint32_t timer_id;
    uint16_t event;
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db;

    timer_id = sys_timer_id_get(handle);
    AUDIO_PRINT_TRACE2("audio_latency_timeout_cback: timer_id 0x%02X handle %p",
                       timer_id, handle);
    event = timer_id >> 16;

    buffer_lat_db = audio_latency_find_handle_by_id(timer_id & 0x0000ffff);
    if (buffer_lat_db == NULL)
    {
        return;
    }

    buffer_ent = buffer_lat_db->buffer_ent;

    switch (event)
    {
    case AUDIO_LATENCY_TIMER_DYNAMIC_LAT_RESUME:
        {
            sys_timer_delete(buffer_lat_db->dynamic_latency_keeper_resume_timer_handle);
            buffer_lat_db->dynamic_latency_keeper_resume_timer_handle = NULL;
            if (buffer_ent->p_cfg == NULL)
            {
                break;
            }
            buffer_lat_db->dynamic_latency_on = false;
            buffer_ent->p_cfg->latency = buffer_ent->p_cfg->latency_back_up;
        }
        break;


    case AUDIO_LATENCY_TIMER_DYNAMIC_CHECK:
        {
            sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
            buffer_lat_db->dynamic_latency_check_timer_handle = NULL;
            buffer_lat_db->plc_when_interval_stable = 0;
        }
        break;

    case AUDIO_LATENCY_TIMER_DYNAMIC_DEC_CHECK:
        {
            sys_timer_delete(buffer_lat_db->dynamic_latency_dec_check_timer_handle);
            buffer_lat_db->dynamic_latency_dec_check_timer_handle = NULL;

            if (buffer_ent->p_cfg == NULL)
            {
                break;
            }
            if (buffer_lat_db->dynamic_latency_on)
            {
                uint16_t plc_count;

                if (buffer_ent->p_cfg->format_info.type == AUDIO_FORMAT_TYPE_SBC)
                {
                    plc_count = 5;
                }
                else
                {
                    plc_count = 1;
                }

                buffer_lat_db->cback(AUDIO_LATENCY_EVT_LOW_LAT_DECREASE_REQ, &plc_count, buffer_ent);
            }
        }
        break;

    default:
        break;
    }
}

void audio_latency_handle_inverval_unstable_evt(T_AUDIO_LATENCY_HANDLE handle)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }

    if (buffer_lat_db->dynamic_latency_dec_check_timer_handle)
    {
        sys_timer_delete(buffer_lat_db->dynamic_latency_dec_check_timer_handle);
        buffer_lat_db->dynamic_latency_dec_check_timer_handle = NULL;
    }

}

void audio_latency_handle_inverval_stable_evt(T_AUDIO_LATENCY_HANDLE handle,
                                              T_AUDIO_FORMAT_TYPE type)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }
    buffer_ent = buffer_lat_db->buffer_ent;

    if (buffer_lat_db->dynamic_latency_dec_check_timer_handle == NULL)
    {
        if (buffer_lat_db->dynamic_latency_on)
        {
            uint16_t plc_count;

            if (type == AUDIO_FORMAT_TYPE_SBC)
            {
                plc_count = 5;
            }
            else
            {
                plc_count = 1;
            }

            buffer_lat_db->cback(AUDIO_LATENCY_EVT_LOW_LAT_DECREASE_REQ, &plc_count, buffer_ent);
        }
    }
}

void audio_latency_resume(T_AUDIO_LATENCY_HANDLE handle)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }
    buffer_ent = buffer_lat_db->buffer_ent;


    buffer_lat_db->dynamic_latency_on = false;
    buffer_ent->p_cfg->latency = buffer_ent->p_cfg->latency_back_up;

    if (buffer_lat_db->dynamic_latency_keeper_resume_timer_handle)
    {
        sys_timer_delete(buffer_lat_db->dynamic_latency_keeper_resume_timer_handle);
        buffer_lat_db->dynamic_latency_keeper_resume_timer_handle = NULL;
    }
}

void audio_latency_dynamic_set(T_AUDIO_LATENCY_HANDLE handle, uint16_t latency, bool auto_resume)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }
    buffer_ent = buffer_lat_db->buffer_ent;

    buffer_lat_db->dynamic_latency_on = true;
    buffer_ent->p_cfg->latency = latency;

    if (auto_resume)
    {
        if (buffer_lat_db->dynamic_latency_keeper_resume_timer_handle == NULL)
        {
            buffer_lat_db->dynamic_latency_keeper_resume_timer_handle =
                sys_timer_create("dynamic_latency_res_check",
                                 SYS_TIMER_TYPE_LOW_PRECISION,
                                 (AUDIO_LATENCY_TIMER_DYNAMIC_LAT_RESUME << 16) | buffer_lat_db->id,
                                 DYNAMIC_DEC_CHECK_TIME2,
                                 false,
                                 audio_latency_timeout_cback);
            if (buffer_lat_db->dynamic_latency_keeper_resume_timer_handle != NULL)
            {
                sys_timer_start(buffer_lat_db->dynamic_latency_keeper_resume_timer_handle);
            }
        }
        else
        {
            sys_timer_start(buffer_lat_db->dynamic_latency_keeper_resume_timer_handle);
        }
    }
}

void audio_latency_low_lat_dynamic_decrease(T_AUDIO_LATENCY_HANDLE handle, uint16_t plc_count)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }
    buffer_ent = buffer_lat_db->buffer_ent;

    if (buffer_ent->p_cfg == NULL)
    {
        return;
    }

    if (buffer_ent->p_cfg->plc_count == 0 || buffer_lat_db->dynamic_latency_on == false)
    {
        return;
    }

    if (buffer_ent->p_cfg->plc_count < plc_count)
    {
        plc_count = buffer_ent->p_cfg->plc_count;
    }
    buffer_ent->p_cfg->latency -= media_buffer_cacu_frames_duration(plc_count,
                                                                    &buffer_ent->p_cfg->format_info);

    buffer_ent->p_cfg->plc_count -= plc_count;
    if (buffer_ent->p_cfg->plc_count == 0)
    {
        buffer_lat_db->dynamic_latency_on = false;
        buffer_ent->p_cfg->latency = buffer_ent->p_cfg->latency_back_up;
    }

    AUDIO_PRINT_TRACE3("audio_latency_low_lat_dynamic_decrease: %u %u %u", buffer_ent->p_cfg->latency,
                       buffer_ent->p_cfg->plc_count, plc_count);

    if (buffer_ent->p_cfg->plc_count != 0)
    {
        if (buffer_lat_db->dynamic_latency_dec_check_timer_handle == NULL)
        {
            buffer_lat_db->dynamic_latency_dec_check_timer_handle =
                sys_timer_create("dynamic_latency_dec_check",
                                 SYS_TIMER_TYPE_LOW_PRECISION,
                                 (AUDIO_LATENCY_TIMER_DYNAMIC_DEC_CHECK << 16) | buffer_lat_db->id,
                                 DYNAMIC_DEC_CHECK_TIME,
                                 false,
                                 audio_latency_timeout_cback);
            if (buffer_lat_db->dynamic_latency_dec_check_timer_handle != NULL)
            {
                sys_timer_start(buffer_lat_db->dynamic_latency_dec_check_timer_handle);
            }
        }
    }

    buffer_lat_db->cback(AUDIO_LATENCY_EVT_LAT_DECREASED, &plc_count, buffer_ent);
}


void audio_latency_low_lat_dynamic_increase(T_AUDIO_LATENCY_HANDLE handle, uint16_t plc_count,
                                            bool auto_dec)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }
    buffer_ent = buffer_lat_db->buffer_ent;

    if (auto_dec)
    {
        if (buffer_lat_db->dynamic_latency_dec_check_timer_handle == NULL)
        {
            buffer_lat_db->dynamic_latency_dec_check_timer_handle =
                sys_timer_create("dynamic_latency_dec_check",
                                 SYS_TIMER_TYPE_LOW_PRECISION,
                                 (AUDIO_LATENCY_TIMER_DYNAMIC_DEC_CHECK << 16) | buffer_lat_db->id,
                                 DYNAMIC_DEC_CHECK_TIME * 2,
                                 false,
                                 audio_latency_timeout_cback);
            if (buffer_lat_db->dynamic_latency_dec_check_timer_handle != NULL)
            {
                sys_timer_start(buffer_lat_db->dynamic_latency_dec_check_timer_handle);
            }
        }
    }
    else
    {
        if (buffer_lat_db->dynamic_latency_dec_check_timer_handle)
        {
            sys_timer_delete(buffer_lat_db->dynamic_latency_dec_check_timer_handle);
            buffer_lat_db->dynamic_latency_dec_check_timer_handle = NULL;
        }
    }
    if (buffer_lat_db->dynamic_latency_on == false)
    {
        buffer_ent->p_cfg->plc_count = plc_count;
        buffer_lat_db->dynamic_latency_on = true;
    }
    else
    {
        buffer_ent->p_cfg->plc_count += plc_count;
    }
    buffer_ent->p_cfg->latency += media_buffer_cacu_frames_duration(plc_count,
                                                                    &buffer_ent->p_cfg->format_info);

    AUDIO_PRINT_TRACE4("audio_latency_low_lat_dynamic_increase: lat %u d_plc %u plc_count %u auto_dec %u",
                       buffer_ent->p_cfg->latency,
                       plc_count, buffer_ent->p_cfg->plc_count, auto_dec);

    buffer_lat_db->cback(AUDIO_LATENCY_EVT_LAT_INCREASED, NULL, buffer_ent);
}

void audio_latency_handle_plc_event(T_AUDIO_LATENCY_HANDLE handle, uint16_t local_seq,
                                    uint16_t plc_total_frames,
                                    uint16_t plc_frame_counter, uint16_t plc_local_seq, uint8_t plc_frame_num)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;
    uint16_t current_plc_count;
    T_MEDIA_BUFFER_CFG *p_cfg;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }

    buffer_ent = buffer_lat_db->buffer_ent;
    p_cfg = buffer_ent->p_cfg;

    if (p_cfg == NULL)
    {
        return;

    }
    current_plc_count = p_cfg->plc_count;

    if (buffer_lat_db->dynamic_latency_dec_check_timer_handle)
    {
        sys_timer_start(buffer_lat_db->dynamic_latency_dec_check_timer_handle);
    }

    if ((uint16_t)(local_seq + 1) == plc_local_seq)
    {
        if (p_cfg->latency_fixed)
        {
            if (p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
            {
                if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    if (buffer_lat_db->last_latency_report < MAX_ULTRA_LOW_PLC_LATENCY)
                    {
                        AUDIO_PRINT_TRACE1("audio_latency_handle_plc_event: allow PLC once %d",
                                           buffer_lat_db->last_latency_report);
                    }
                    else
                    {
                        buffer_lat_db->cback(AUDIO_LATENCY_EVT_PLC_FIX, &plc_frame_num, buffer_ent);
                    }
                }
            }
            else
            {
                buffer_lat_db->cback(AUDIO_LATENCY_EVT_PLC_FIX, &plc_frame_num, buffer_ent);
            }
        }
        else
        {
            bool adjust_latency = audio_latency_adjust_check(buffer_ent->audio_latency_handle);

            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (adjust_latency)
                {
                    sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
                    buffer_lat_db->dynamic_latency_check_timer_handle = NULL;
                    if (current_plc_count > buffer_ent->max_latency_plc_count)
                    {
                        buffer_lat_db->cback(AUDIO_LATENCY_EVT_MAX_LAT_PLC, &plc_frame_num, buffer_ent);
                    }
                    else
                    {
                        buffer_lat_db->cback(AUDIO_LATENCY_EVT_LOW_LAT_INCREASE_REQ, &plc_frame_num, buffer_ent);
                    }
                }
                else
                {
                    buffer_lat_db->cback(AUDIO_LATENCY_EVT_PLC_FIX, &plc_frame_num, buffer_ent);
                }
            }
            else
            {
                if (adjust_latency)
                {
                    if (current_plc_count > buffer_ent->max_latency_plc_count)
                    {
                        AUDIO_PRINT_INFO0("dsp_plc_event: max latency, not inc");
                        return;
                    }
                    else
                    {
                        buffer_lat_db->cback(AUDIO_LATENCY_EVT_LOW_LAT_INCREASE_REQ, &plc_frame_num, buffer_ent);
                    }
                }
            }
        }
    }
    else if (local_seq == plc_local_seq)
    {
        if (p_cfg->latency_fixed)
        {
            if (p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
            {
                if (buffer_lat_db->last_latency_report != 0 &&
                    buffer_lat_db->last_latency_report < MAX_ULTRA_LOW_PLC_LATENCY)
                {
                    buffer_lat_db->cback(AUDIO_LATENCY_EVT_ULTRA_LOW_LAT_PLC_REQ, NULL, buffer_ent);
                }
            }
        }
        else
        {
            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
                buffer_lat_db->dynamic_latency_check_timer_handle = NULL;
                buffer_lat_db->plc_when_interval_stable = 0;
                if (current_plc_count > buffer_ent->max_latency_plc_count)
                {
                    buffer_lat_db->cback(AUDIO_LATENCY_EVT_MAX_LAT_PLC, &plc_frame_num, buffer_ent);
                }
                else
                {
                    T_MEDIA_DATA_HDR *p_media_packet;

                    if (media_buffer_backup_downstream(buffer_ent))
                    {
                        p_media_packet = buffer_ent->last_packet_ptr;
                        buffer_lat_db->cback(AUDIO_LATENCY_EVT_LOW_LAT_INCREASE_REQ, &p_media_packet->frame_number,
                                             buffer_ent);
                    }
                }
            }
            else
            {
                if (audio_latency_adjust_check(buffer_ent->audio_latency_handle))
                {
                    if (current_plc_count > buffer_ent->max_latency_plc_count)
                    {
                        AUDIO_PRINT_INFO0("dsp_plc_event: max latency, not inc");
                        return;
                    }
                    else
                    {
                        buffer_lat_db->cback(AUDIO_LATENCY_EVT_LOW_LAT_INCREASE_REQ, &plc_frame_num, buffer_ent);
                    }
                }
            }
        }
    }
    else
    {
        if (p_cfg->latency_fixed)
        {
            if (p_cfg->mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
            {
                if (buffer_lat_db->last_latency_report != 0 &&
                    buffer_lat_db->last_latency_report < MAX_ULTRA_LOW_PLC_LATENCY)
                {
                    buffer_lat_db->cback(AUDIO_LATENCY_EVT_ULTRA_LOW_LAT_PLC_REQ, NULL, buffer_ent);
                }
            }
        }
        else
        {
            if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
            {
                sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
                buffer_lat_db->dynamic_latency_check_timer_handle = NULL;
                buffer_lat_db->plc_when_interval_stable = 0;
                if (current_plc_count > buffer_ent->max_latency_plc_count)
                {
                    buffer_lat_db->cback(AUDIO_LATENCY_EVT_MAX_LAT_PLC, &plc_frame_num, buffer_ent);
                }
                else
                {
                    T_MEDIA_DATA_HDR *p_media_packet;

                    if (media_buffer_backup_downstream(buffer_ent))
                    {
                        p_media_packet = buffer_ent->last_packet_ptr;
                        buffer_lat_db->cback(AUDIO_LATENCY_EVT_LOW_LAT_INCREASE_REQ, &p_media_packet->frame_number,
                                             buffer_ent);
                    }
                }
            }
            else
            {
                if (audio_latency_adjust_check(buffer_ent->audio_latency_handle))
                {
                    if (current_plc_count > buffer_ent->max_latency_plc_count)
                    {
                        AUDIO_PRINT_INFO0("dsp_plc_event: max latency, not inc");
                        return;
                    }
                    else
                    {
                        buffer_lat_db->cback(AUDIO_LATENCY_EVT_LOW_LAT_INCREASE_REQ, &plc_frame_num, buffer_ent);
                    }
                }
            }
        }
    }
}

bool audio_latency_adjust_check(T_AUDIO_LATENCY_HANDLE handle)
{
    T_MEDIA_BUFFER_ENTITY *buffer_ent;
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;
    bool stable;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return false;
    }

    buffer_ent = buffer_lat_db->buffer_ent;

    if (media_buffer_get_status(buffer_ent) != MEDIA_BUFFER_PLAYING)
    {
        return false;
    }

    if (buffer_ent->p_cfg->latency_fixed)
    {
        return false;
    }

    if (buffer_lat_db->dynamic_latency_check_timer_handle)
    {
        if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
        {
            if (audio_interval_get_stable_state(buffer_ent->audio_interval_eval_handle, &stable))
            {

                if (stable == false)
                {
                    sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
                    buffer_lat_db->dynamic_latency_check_timer_handle = NULL;
                    buffer_lat_db->plc_when_interval_stable = 0;
                    return true;
                }
                else
                {
                    buffer_lat_db->plc_when_interval_stable++;
                    AUDIO_PRINT_INFO1("dsp_plc_event: interval is stable but plc %u",
                                      buffer_lat_db->plc_when_interval_stable);
                    if (buffer_lat_db->plc_when_interval_stable > 2)
                    {
                        sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
                        buffer_lat_db->dynamic_latency_check_timer_handle = NULL;
                        buffer_lat_db->plc_when_interval_stable = 0;

                        return true;
                    }
                    else
                    {
                        sys_timer_start(buffer_lat_db->dynamic_latency_check_timer_handle);
                        return false;
                    }
                }
            }
            else
            {
                AUDIO_PRINT_INFO0("dsp_plc_event: interval eval is not ready");
                return false;
            }
        }
        else
        {
            buffer_lat_db->plc_when_interval_stable++;
            AUDIO_PRINT_INFO1("dsp_plc_event: interval is stable but plc %u",
                              buffer_lat_db->plc_when_interval_stable);
            if (buffer_lat_db->plc_when_interval_stable > 2)
            {
                sys_timer_delete(buffer_lat_db->dynamic_latency_check_timer_handle);
                buffer_lat_db->dynamic_latency_check_timer_handle = NULL;
                buffer_lat_db->plc_when_interval_stable = 0;

                return true;
            }
            else
            {
                sys_timer_start(buffer_lat_db->dynamic_latency_check_timer_handle);
                return false;
            }
        }
    }
    else
    {
        buffer_lat_db->dynamic_latency_check_timer_handle = sys_timer_create("dynamic_latency_check",
                                                                             SYS_TIMER_TYPE_LOW_PRECISION,
                                                                             (AUDIO_LATENCY_TIMER_DYNAMIC_CHECK << 16) | buffer_lat_db->id,
                                                                             DYNAMIC_CHECK_TIME,
                                                                             false,
                                                                             audio_latency_timeout_cback);
        if (buffer_lat_db->dynamic_latency_check_timer_handle != NULL)
        {
            sys_timer_start(buffer_lat_db->dynamic_latency_check_timer_handle);
            buffer_lat_db->plc_when_interval_stable = 0;
        }
        return false;
    }
}


void audio_latency_ultra_low_latency_plc_req(T_AUDIO_LATENCY_HANDLE handle, uint8_t frame_num)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }

    buffer_lat_db->ultra_low_plc_cnt += frame_num;
}

void audio_latency_record_update(T_AUDIO_LATENCY_HANDLE handle, uint32_t latency_report)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;

    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return;
    }

    buffer_lat_db->last_latency_report = latency_report;
}

bool audio_latency_get_dynamic_latency_on(T_AUDIO_LATENCY_HANDLE handle, bool *dynamic_latency_on)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;
    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return false;
    }

    *dynamic_latency_on = buffer_lat_db->dynamic_latency_on;
    return true;
}

bool audio_latency_set_dynamic_latency_on(T_AUDIO_LATENCY_HANDLE handle, bool dynamic_latency_on)
{
    T_AUDIO_LATENCY_BUFFER_DB *buffer_lat_db = handle;
    if (audio_latency_mgr_check_handle(handle) == false)
    {
        return false;
    }

    buffer_lat_db->dynamic_latency_on = dynamic_latency_on;
    return true;
}
