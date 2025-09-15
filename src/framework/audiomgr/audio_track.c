/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>

#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "media_buffer.h"
#include "audio_type.h"
#include "audio_mgr.h"
#include "audio_path.h"
#include "audio_effect.h"
#include "audio_track.h"
#include "audio_route.h"

#define AUDIO_TRACK_PLAYBACK_DEFAULT_LATENCY  280
#define AUDIO_TRACK_VOICE_DEFAULT_LATENCY     100

typedef enum t_audio_session_state
{
    AUDIO_SESSION_STATE_RELEASED    = 0x00,
    AUDIO_SESSION_STATE_CREATING    = 0x01,
    AUDIO_SESSION_STATE_CREATED     = 0x02,
    AUDIO_SESSION_STATE_STARTING    = 0x03,
    AUDIO_SESSION_STATE_STARTED     = 0x04,
    AUDIO_SESSION_STATE_STOPPING    = 0x05,
    AUDIO_SESSION_STATE_STOPPED     = 0x06,
    AUDIO_SESSION_STATE_PAUSING     = 0x07,
    AUDIO_SESSION_STATE_PAUSED      = 0x08,
    AUDIO_SESSION_STATE_RESTARTING  = 0x09,
    AUDIO_SESSION_STATE_RELEASING   = 0x0a,
} T_AUDIO_SESSION_STATE;

typedef enum t_audio_track_action
{
    AUDIO_TRACK_ACTION_NONE     = 0x00,
    AUDIO_TRACK_ACTION_CREATE   = 0x01,
    AUDIO_TRACK_ACTION_START    = 0x02,
    AUDIO_TRACK_ACTION_STOP     = 0x03,
    AUDIO_TRACK_ACTION_PAUSE    = 0x04,
    AUDIO_TRACK_ACTION_RESTART  = 0x05,
    AUDIO_TRACK_ACTION_RELEASE  = 0x06,
} T_AUDIO_TRACK_ACTION;

typedef enum t_audio_buffer_state
{
    AUDIO_BUFFER_STATE_DETACHED = 0x00,
    AUDIO_BUFFER_STATE_ATTACHED = 0x01,
    AUDIO_BUFFER_STATE_PLAYED   = 0x02,
    AUDIO_BUFFER_STATE_STOPPED  = 0x03,
} T_AUDIO_BUFFER_STATE;

typedef enum t_dynamic_latency_sens_level
{
    DYNAMIC_LATENCY_SENS_LEVEL1  = 0,
    DYNAMIC_LATENCY_SENS_LEVEL2  = 1,
    DYNAMIC_LATENCY_SENS_LEVEL3  = 2,
    DYNAMIC_LATENCY_SENS_LEVEL_MAX  = 3,
} T_DYNAMIC_LATENCY_SENS_LEVEL;

typedef struct t_audio_track
{
    struct t_audio_track   *p_next;
    void                   *container;
    T_AUDIO_SESSION_STATE   state;
    T_AUDIO_TRACK_ACTION    action;
    T_AUDIO_STREAM_TYPE     stream_type;
    T_AUDIO_FORMAT_INFO     format_info;
    uint32_t                device;
    T_AUDIO_STREAM_MODE     mode;
    T_AUDIO_STREAM_USAGE    usage;
    uint16_t                latency;
    bool                    latency_fixed;
    uint8_t                 dynamic_latency_sens;
    uint8_t                 volume_out_max;
    uint8_t                 volume_out_min;
    bool                    volume_out_muted;
    uint8_t                 volume_out;
    uint8_t                 volume_in_max;
    uint8_t                 volume_in_min;
    bool                    volume_in_muted;
    uint8_t                 volume_in;
    T_AUDIO_BUFFER_STATE    buffer_state;
    P_AUDIO_TRACK_ASYNC_IO  async_write;
    P_AUDIO_TRACK_ASYNC_IO  async_read;
    T_AUDIO_PATH_HANDLE     path_handle;
    T_MEDIA_BUFFER_PROXY    buffer_proxy;
    T_OS_QUEUE              effects;
    bool                    effect_apply;
    uint8_t                 effect_bypass;
    uint16_t                seq_in_num;
    bool                    signal_in;
    bool                    signal_out;
    uint16_t                signal_in_interval;
    uint16_t                signal_out_interval;
    uint16_t                us_packet_length;
} T_AUDIO_TRACK;

typedef struct t_audio_track_db
{
    uint8_t        playback_volume_max;
    uint8_t        playback_volume_min;
    uint8_t        playback_volume_default;
    bool           playback_volume_muted;
    float          playback_volume_scale;
    uint8_t        voice_volume_out_max;
    uint8_t        voice_volume_out_min;
    uint8_t        voice_volume_out_default;
    bool           voice_volume_out_muted;
    float          voice_volume_out_scale;
    uint8_t        voice_volume_in_max;
    uint8_t        voice_volume_in_min;
    uint8_t        voice_volume_in_default;
    bool           voice_volume_in_muted;
    uint8_t        record_volume_max;
    uint8_t        record_volume_min;
    uint8_t        record_volume_default;
    bool           record_volume_muted;
    bool           multi_instance_enable;
    T_OS_QUEUE     idle_tracks;
    T_OS_QUEUE     playback_tracks;
    T_OS_QUEUE     voice_tracks;
    T_OS_QUEUE     record_tracks;
    T_AUDIO_TRACK *playback_buffer_owner;
    T_AUDIO_TRACK *voice_buffer_owner;
    T_AUDIO_TRACK *record_buffer_owner;
} T_AUDIO_TRACK_DB;

static T_AUDIO_TRACK_DB audio_track_db;

static bool audio_session_state_set(T_AUDIO_TRACK *track, T_AUDIO_SESSION_STATE state);

static T_AUDIO_CATEGORY audio_track_path_category_convert(T_AUDIO_STREAM_TYPE stream_type)
{
    T_AUDIO_CATEGORY path_category;

    switch (stream_type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        path_category = AUDIO_CATEGORY_AUDIO;
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        path_category = AUDIO_CATEGORY_VOICE;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        path_category = AUDIO_CATEGORY_RECORD;
        break;
    }

    return path_category;
}

static bool audio_track_queue_add(T_OS_QUEUE *queue, T_AUDIO_TRACK *track, bool high_priority)
{
    if (track->container != NULL)
    {
        /* Delete the track from its original track queue. */
        if (os_queue_delete(track->container, track) == false)
        {
            /* Track is not in its container queue. This shall not happen, anyway. */
            return false;
        }
    }

    track->container = queue;

    /* Insert the track as the first element of the queue if it's high priority;
     * otherwise, append the track into the queue.
     */
    if (high_priority == true)
    {
        os_queue_insert(track->container, NULL, track);
    }
    else
    {
        os_queue_in(track->container, track);
    }

    return true;
}

static bool audio_track_queue_remove(T_AUDIO_TRACK *track)
{
    if (track->container != NULL)
    {
        if (os_queue_delete(track->container, track) == false)
        {
            /* Track is not in its container queue. This shall not happen, anyway. */
            return false;
        }

        track->container = NULL;
    }

    return true;
}

static bool audio_track_is_starting(T_AUDIO_STREAM_TYPE type)
{
    T_AUDIO_TRACK *track;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        track = os_queue_peek(&audio_track_db.playback_tracks, 0);
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        track = os_queue_peek(&audio_track_db.voice_tracks, 0);
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        track = os_queue_peek(&audio_track_db.record_tracks, 0);
        break;
    }

    while (track != NULL)
    {
        if (track->state == AUDIO_SESSION_STATE_STARTING || track->state == AUDIO_SESSION_STATE_RESTARTING)
        {
            return true;
        }
        track = track->p_next;
    }

    return false;
}

void audio_track_reschedule(void)
{
    T_AUDIO_TRACK *tracks[4];
    T_AUDIO_TRACK *track;

    tracks[0] = os_queue_peek(&audio_track_db.playback_tracks, 0);
    tracks[1] = os_queue_peek(&audio_track_db.voice_tracks, 0);
    tracks[2] = os_queue_peek(&audio_track_db.record_tracks, 0);
    tracks[3] = os_queue_peek(&audio_track_db.idle_tracks, 0);

    for (uint8_t i = 0; i < 4; i++)
    {
        track = tracks[i];
        while (track != NULL)
        {
            if (track->action != AUDIO_TRACK_ACTION_NONE)
            {
                switch (track->action)
                {
                case AUDIO_TRACK_ACTION_START:
                    {
                        track->action = AUDIO_TRACK_ACTION_NONE;
                        audio_session_state_set(track, AUDIO_SESSION_STATE_STARTING);
                    }
                    break;

                case AUDIO_TRACK_ACTION_STOP:
                    {
                        track->action = AUDIO_TRACK_ACTION_NONE;
                        audio_session_state_set(track, AUDIO_SESSION_STATE_STOPPING);
                    }
                    break;

                case AUDIO_TRACK_ACTION_PAUSE:
                    {
                        track->action = AUDIO_TRACK_ACTION_NONE;
                        audio_session_state_set(track, AUDIO_SESSION_STATE_PAUSING);
                    }
                    break;

                case AUDIO_TRACK_ACTION_RESTART:
                    {
                        track->action = AUDIO_TRACK_ACTION_NONE;
                        audio_session_state_set(track, AUDIO_SESSION_STATE_RESTARTING);
                    }
                    break;

                case AUDIO_TRACK_ACTION_RELEASE:
                    {
                        track->action = AUDIO_TRACK_ACTION_NONE;
                        audio_session_state_set(track, AUDIO_SESSION_STATE_RELEASING);
                    }
                    break;

                default:
                    break;
                }
                break;
            }
            track = track->p_next;
        }
    }
}

static bool audio_track_queue_freeze(T_AUDIO_STREAM_TYPE type)
{
    T_AUDIO_TRACK *track;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        track = os_queue_peek(&audio_track_db.playback_tracks, 0);
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        track = os_queue_peek(&audio_track_db.voice_tracks, 0);
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        track = os_queue_peek(&audio_track_db.record_tracks, 0);
        break;
    }

    while (track != NULL)
    {
        audio_track_stop(track);
        track = track->p_next;
    }

    return true;
}

static bool audio_track_handle_check(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;

    track = (T_AUDIO_TRACK *)handle;

    if (track == NULL)
    {
        return false;
    }

    if (os_queue_search(&audio_track_db.idle_tracks, track) == true)
    {
        return true;
    }

    if (os_queue_search(&audio_track_db.playback_tracks, track) == true)
    {
        return true;
    }

    if (os_queue_search(&audio_track_db.voice_tracks, track) == true)
    {
        return true;
    }

    if (os_queue_search(&audio_track_db.record_tracks, track) == true)
    {
        return true;
    }

    return false;
}

static bool audio_track_effect_check(T_AUDIO_TRACK           *track,
                                     T_AUDIO_EFFECT_INSTANCE  instance)
{
    if (instance == NULL)
    {
        return false;
    }

    if (os_queue_search(&track->effects, instance) == true)
    {
        return true;
    }

    return false;
}

static bool audio_track_effect_enqueue(T_AUDIO_TRACK           *track,
                                       T_AUDIO_EFFECT_INSTANCE  instance)
{
    if (instance == NULL)
    {
        return false;
    }

    if (os_queue_search(&track->effects, instance) == true)
    {
        return true;
    }

    os_queue_in(&track->effects, instance);

    return true;
}

static bool audio_track_effect_dequeue(T_AUDIO_TRACK           *track,
                                       T_AUDIO_EFFECT_INSTANCE  instance)
{
    if (instance == NULL)
    {
        return false;
    }

    return os_queue_delete(&track->effects, instance);
}

static bool audio_track_effect_run(T_AUDIO_TRACK *track)
{
    T_AUDIO_EFFECT_INSTANCE instance;
    int32_t                 i = 0;

    while ((instance = os_queue_peek(&track->effects, i)) != NULL)
    {
        audio_effect_run(instance);
        i++;
    }

    return true;
}

static void audio_track_effect_control_apply(T_AUDIO_TRACK *track, uint8_t action)
{
    switch (track->stream_type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        {
            audio_path_decoder_effect_control(track->path_handle, action);
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        {
            audio_path_encoder_effect_control(track->path_handle, action);
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        {
            audio_path_decoder_effect_control(track->path_handle, action);
            audio_path_encoder_effect_control(track->path_handle, action);
        }
        break;

    default:
        break;
    }
}

static bool audio_track_effect_stop(T_AUDIO_TRACK *track)
{
    T_AUDIO_EFFECT_INSTANCE instance;
    int32_t                 i = 0;

    while ((instance = os_queue_peek(&track->effects, i)) != NULL)
    {
        audio_effect_stop(instance);
        i++;
    }

    return true;
}

static bool audio_track_effect_clear(T_AUDIO_TRACK *track)
{
    T_AUDIO_EFFECT_INSTANCE instance;

    while ((instance = os_queue_out(&track->effects)) != NULL)
    {
        audio_effect_owner_clear(instance);
    }

    return true;
}

void audio_track_volume_out_mute_all(T_AUDIO_STREAM_TYPE type)
{
    T_OS_QUEUE    *queue = NULL;
    T_AUDIO_TRACK *track;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        if (audio_track_db.playback_volume_muted == false)
        {
            queue = &audio_track_db.playback_tracks;
            audio_track_db.playback_volume_muted = true;
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if (audio_track_db.voice_volume_out_muted == false)
        {
            queue = &audio_track_db.voice_tracks;
            audio_track_db.voice_volume_out_muted = true;
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        break;
    }

    if (queue != NULL)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            if (track->volume_out_muted == false)
            {
                audio_path_dac_mute(track->path_handle);
            }

            track = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                if (track->volume_out_muted == false)
                {
                    audio_path_dac_mute(track->path_handle);
                }
            }

            track = track->p_next;
        }
    }
}

void audio_track_volume_out_unmute_all(T_AUDIO_STREAM_TYPE type)
{
    T_OS_QUEUE    *queue = NULL;
    T_AUDIO_TRACK *track;
    float          scale;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        if (audio_track_db.playback_volume_muted == true)
        {
            queue = &audio_track_db.playback_tracks;
            audio_track_db.playback_volume_muted = false;
            scale = audio_track_db.playback_volume_scale;
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if (audio_track_db.voice_volume_out_muted == true)
        {
            queue = &audio_track_db.voice_tracks;
            audio_track_db.voice_volume_out_muted = false;
            scale = audio_track_db.voice_volume_out_scale;
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        {
            queue = NULL;
            scale = 0.0f;
        }
        break;
    }

    if (queue != NULL)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            if (track->volume_out_muted == false)
            {
                audio_path_dac_level_set(track->path_handle, track->volume_out, scale);
            }

            track = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                if (track->volume_out_muted == false)
                {
                    audio_path_dac_level_set(track->path_handle, track->volume_out, scale);
                }
            }

            track = track->p_next;
        }
    }
}

void audio_track_volume_out_set_all(T_AUDIO_STREAM_TYPE type, uint8_t volume)
{
    T_OS_QUEUE    *queue = NULL;
    T_AUDIO_TRACK *track;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        queue = &audio_track_db.playback_tracks;
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        queue = &audio_track_db.voice_tracks;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        break;
    }

    if (queue != NULL)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            audio_track_volume_out_set(track, volume);
            track = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                audio_track_volume_out_set(track, volume);
            }

            track = track->p_next;
        }
    }
}

void audio_track_volume_in_mute_all(T_AUDIO_STREAM_TYPE type)
{
    T_OS_QUEUE    *queue = NULL;
    T_AUDIO_TRACK *track;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if (audio_track_db.voice_volume_in_muted == false)
        {
            queue = &audio_track_db.voice_tracks;
            audio_track_db.voice_volume_in_muted = true;
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        if (audio_track_db.record_volume_muted == false)
        {
            queue = &audio_track_db.record_tracks;
            audio_track_db.record_volume_muted = true;
        }
        break;
    }

    if (queue != NULL)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            if (track->volume_in_muted == false)
            {
                audio_path_adc_mute(track->path_handle);
            }

            track = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                if (track->volume_in_muted == false)
                {
                    audio_path_adc_mute(track->path_handle);
                }
            }

            track = track->p_next;
        }
    }
}

void audio_track_volume_in_unmute_all(T_AUDIO_STREAM_TYPE type)
{
    T_OS_QUEUE    *queue = NULL;
    T_AUDIO_TRACK *track;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if (audio_track_db.voice_volume_in_muted == true)
        {
            queue = &audio_track_db.voice_tracks;
            audio_track_db.voice_volume_in_muted = false;
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        if (audio_track_db.record_volume_muted == true)
        {
            queue = &audio_track_db.record_tracks;
            audio_track_db.record_volume_muted = false;
        }
        break;
    }

    if (queue != NULL)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            if (track->volume_in_muted == false)
            {
                audio_path_adc_level_set(track->path_handle, track->volume_in, 0.0f);
            }

            track = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                if (track->volume_in_muted == false)
                {
                    audio_path_adc_level_set(track->path_handle, track->volume_in, 0.0f);
                }
            }

            track = track->p_next;
        }
    }
}

void audio_track_volume_in_set_all(T_AUDIO_STREAM_TYPE type, uint8_t volume)
{
    T_OS_QUEUE    *queue = NULL;
    T_AUDIO_TRACK *track;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        queue = &audio_track_db.voice_tracks;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        queue = &audio_track_db.record_tracks;
        break;
    }

    if (queue != NULL)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            audio_track_volume_in_set(track, volume);
            track = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                audio_track_volume_in_set(track, volume);
            }

            track = track->p_next;
        }
    }
}

static void audio_track_volume_apply(T_AUDIO_TRACK *track)
{
    switch (track->stream_type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        if ((audio_track_db.playback_volume_muted == true) ||
            (track->volume_out_muted == true))
        {
            audio_path_dac_mute(track->path_handle);
        }
        else
        {
            audio_path_dac_level_set(track->path_handle, track->volume_out,
                                     audio_track_db.playback_volume_scale);
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if ((audio_track_db.voice_volume_out_muted == true) ||
            (track->volume_out_muted == true))
        {
            audio_path_dac_mute(track->path_handle);
        }
        else
        {
            audio_path_dac_level_set(track->path_handle, track->volume_out,
                                     audio_track_db.voice_volume_out_scale);
        }

        if ((audio_track_db.voice_volume_in_muted == true) ||
            (track->volume_in_muted == true))
        {
            audio_path_adc_mute(track->path_handle);
        }
        else
        {
            audio_path_adc_level_set(track->path_handle, track->volume_in, 0.0f);
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        if ((audio_track_db.record_volume_muted == true) ||
            (track->volume_in_muted == true))
        {
            audio_path_adc_mute(track->path_handle);
        }
        else
        {
            audio_path_adc_level_set(track->path_handle, track->volume_in, 0.0f);
        }
        break;
    }
}

bool audio_track_volume_balance_set(T_AUDIO_STREAM_TYPE stream_type, float scale)
{
    T_OS_QUEUE    *queue;
    T_AUDIO_TRACK *track;

    switch (stream_type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        queue = &audio_track_db.playback_tracks;
        audio_track_db.playback_volume_scale = scale;
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        queue = &audio_track_db.voice_tracks;
        audio_track_db.voice_volume_out_scale = scale;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        queue = NULL;
        break;
    }

    if (queue != NULL)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            audio_track_volume_out_set(track, track->volume_out);
            track = track->p_next;
        }

        return true;
    }

    return false;
}

static T_AUDIO_TRACK *audio_track_find_by_path(T_AUDIO_PATH_HANDLE handle)
{
    T_AUDIO_TRACK *track;

    track = os_queue_peek(&audio_track_db.idle_tracks, 0);
    while (track != NULL)
    {
        if (track->path_handle != NULL &&
            track->path_handle == handle)
        {
            return track;
        }

        track = track->p_next;
    }

    track = os_queue_peek(&audio_track_db.playback_tracks, 0);
    while (track != NULL)
    {
        if (track->path_handle != NULL &&
            track->path_handle == handle)
        {
            return track;
        }

        track = track->p_next;
    }

    track = os_queue_peek(&audio_track_db.voice_tracks, 0);
    while (track != NULL)
    {
        if (track->path_handle != NULL &&
            track->path_handle == handle)
        {
            return track;
        }

        track = track->p_next;
    }

    track = os_queue_peek(&audio_track_db.record_tracks, 0);
    while (track != NULL)
    {
        if (track->path_handle != NULL &&
            track->path_handle == handle)
        {
            return track;
        }

        track = track->p_next;
    }

    return track;
}

static bool audio_track_buffer_attach(T_AUDIO_TRACK *track)
{

    track->buffer_state = AUDIO_BUFFER_STATE_ATTACHED;
    track->buffer_proxy = media_buffer_attach(track->stream_type, track->usage, track->mode,
                                              &track->format_info, track, track->path_handle);
    media_buffer_latency_set(track->buffer_proxy, track->latency, track->latency_fixed);
    return true;
}

static bool audio_track_buffer_detach(T_AUDIO_TRACK *track)
{
    track->buffer_state  = AUDIO_BUFFER_STATE_DETACHED;
    media_buffer_detach(track->buffer_proxy);
    track->buffer_proxy = NULL;

    return true;
}

static bool audio_track_buffer_play(T_AUDIO_TRACK *track)
{
    track->buffer_state = AUDIO_BUFFER_STATE_PLAYED;
    media_buffer_active_proxy(track->buffer_proxy);

    return true;
}

static bool audio_track_buffer_stop(T_AUDIO_TRACK *track)
{
    track->buffer_state = AUDIO_BUFFER_STATE_STOPPED;
    media_buffer_deactive_proxy(track->buffer_proxy);

    return true;
}

static bool audio_track_path_cback(T_AUDIO_PATH_HANDLE handle,
                                   T_AUDIO_PATH_EVENT event,
                                   uint32_t param)
{
    T_AUDIO_TRACK *track;

    track = audio_track_find_by_path(handle);

    AUDIO_PRINT_TRACE4("audio_track_path_cback: handle %p, event 0x%02x, param 0x%08x, track %p",
                       handle, event, param, track);

    if (track != NULL)
    {
        switch (event)
        {
        case AUDIO_PATH_EVT_RELEASE:
            {
                audio_session_state_set(track, AUDIO_SESSION_STATE_RELEASED);
            }
            break;

        case AUDIO_PATH_EVT_CREATE:
            {
                /* This case will not be entered, because the interactions between audio track
                 * and audio path are synchronous currently, thus AUDIO_PATH_EVT_CREATE event is
                 * issued before the audio path handle bound to its audio track.
                 */
                audio_session_state_set(track, AUDIO_SESSION_STATE_CREATED);
            }
            break;

        case AUDIO_PATH_EVT_IDLE:
            {
                audio_session_state_set(track, AUDIO_SESSION_STATE_STOPPED);
            }
            break;

        case AUDIO_PATH_EVT_EFFECT_REQ:
            {
                track->effect_apply = true;

                audio_track_effect_control_apply(track, track->effect_bypass);
                audio_track_effect_run(track);
            }
            break;

        case AUDIO_PATH_EVT_RUNNING:
            {
                audio_session_state_set(track, AUDIO_SESSION_STATE_STARTED);
            }
            break;

        case AUDIO_PATH_EVT_SUSPEND:
            {
                audio_session_state_set(track, AUDIO_SESSION_STATE_PAUSED);
            }
            break;

        case AUDIO_PATH_EVT_REQ_DATA:
            audio_mgr_dispatch(AUDIO_MSG_TRACK_DATA_XMIT, track->buffer_proxy);
            break;

        case AUDIO_PATH_EVT_DATA_IND:
            {
                void     *peek_buf;
                uint16_t  peek_len;
                uint16_t  actual_len;
                uint32_t  timestamp;
                uint8_t   frame_num;
                uint8_t *payload;
                uint16_t payload_len;
                uint16_t len;
                T_AUDIO_STREAM_STATUS  status = AUDIO_STREAM_STATUS_CORRECT;
                T_H2D_STREAM_HEADER2 *d2h_hdr;

                peek_len = audio_path_data_peek(track->path_handle);
                if (peek_len > track->us_packet_length)
                {
                    AUDIO_PRINT_ERROR2("AUDIO_PATH_EVT_DATA_IND: peek_len %u us_packet_length %u", peek_len,
                                       track->us_packet_length);
                    break;
                }

                peek_buf = os_mem_alloc2(peek_len);
                if (peek_buf == NULL)
                {
                    break;
                }

                len = audio_path_data_recv(track->path_handle, peek_buf, peek_len);
                if (len == 0)
                {
                    AUDIO_PRINT_ERROR2("AUDIO_PATH_EVT_DATA_IND: peek_len %u len %u", peek_len, len);
                    os_mem_free(peek_buf);
                    break;
                }
                if (track->state != AUDIO_SESSION_STATE_STARTED)
                {
                    os_mem_free(peek_buf);
                    break;
                }

                d2h_hdr = (T_H2D_STREAM_HEADER2 *)peek_buf;

                frame_num = d2h_hdr->frame_number;
                track->seq_in_num = d2h_hdr->local_seq;
                timestamp = d2h_hdr->timestamp;
                payload = d2h_hdr->payload;
                payload_len = d2h_hdr->payload_length;

                AUDIO_PRINT_INFO4("AUDIO_PATH_EVT_DATA_IND: payload_len %u timestamp 0x%x seq %u frame_num %u",
                                  payload_len, timestamp, track->seq_in_num, frame_num);

                if (track->async_read != NULL)
                {

                    if (track->async_read(track,
                                          &timestamp,
                                          &track->seq_in_num,
                                          &status,
                                          &frame_num,
                                          payload,
                                          payload_len,
                                          &actual_len))
                    {
                        if (actual_len != payload_len)
                        {
                            media_buffer_us_write(track->buffer_proxy,
                                                  payload + actual_len,
                                                  payload_len - actual_len,
                                                  timestamp,
                                                  track->seq_in_num,
                                                  status,
                                                  frame_num);
                        }
                    }
                    else
                    {
                        media_buffer_us_write(track->buffer_proxy,
                                              payload,
                                              payload_len,
                                              timestamp,
                                              track->seq_in_num,
                                              status,
                                              frame_num);
                    }
                }
                else
                {
                    uint16_t written_len;
                    T_AUDIO_MSG_PAYLOAD_TRACK_DATA_IND msg;

                    written_len = media_buffer_us_write(track->buffer_proxy,
                                                        payload,
                                                        payload_len,
                                                        timestamp,
                                                        track->seq_in_num,
                                                        status,
                                                        frame_num);
                    if (written_len != 0)
                    {
                        msg.handle = track;
                        msg.len    = written_len;
                        audio_mgr_dispatch(AUDIO_MSG_TRACK_DATA_IND, &msg);
                    }
                }
                os_mem_free(peek_buf);
            }
            break;

        case AUDIO_PATH_EVT_DATA_EMPTY:
            {
                audio_mgr_dispatch(AUDIO_MSG_DSP_DECODE_EMPTY, track->buffer_proxy);
            }
            break;

        case AUDIO_PATH_EVT_SIGNAL_OUT_REFRESH:
            {
                T_AUDIO_MSG_SIGNAL_OUT_REFRESH *p_msg = (T_AUDIO_MSG_SIGNAL_OUT_REFRESH *)param;
                T_AUDIO_MSG_PAYLOAD_TRACK_SIGNAL_OUT_REFRESH msg;

                msg.handle = track;
                msg.left_gain = p_msg->left_gain;
                msg.right_gain = p_msg->right_gain;
                audio_mgr_dispatch(AUDIO_MSG_TRACK_SIGNAL_OUT_REFRESH, &msg);
            }
            break;

        case AUDIO_PATH_EVT_DSP_LATENCY_RPT:
            {
                audio_mgr_dispatch(AUDIO_MSG_STREAM_LATENCY_REPORT, (T_AUDIO_MSG_STREAM_LATENCY_RPT_EVENT *)param);
            }
            break;

        case AUDIO_PATH_EVT_DSP_PLC:
            {
                audio_mgr_dispatch(AUDIO_MSG_DSP_PLC, (T_AUDIO_MSG_DSP_PLC_EVENT *)param);
            }
            break;

        case AUDIO_PATH_EVT_DECODER_PLC_NOTIFY:
            {
                audio_mgr_dispatch(AUDIO_MSG_DECODER_PLC_NOTIFY, (T_AUDIO_MSG_DECODER_PLC_NOTIFY_EVENT *)param);
            }
            break;

        case AUDIO_PATH_EVT_DSP_SYNC_UNLOCK:
            {
                if ((track->stream_type == AUDIO_STREAM_TYPE_VOICE) ||
                    (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK))
                {
                    audio_mgr_dispatch(AUDIO_MSG_DSP_SYNC_UNLOCK, track->buffer_proxy);
                }
            }
            break;

        case AUDIO_PATH_EVT_DSP_SYNC_LOCK:
            {
                if ((track->stream_type == AUDIO_STREAM_TYPE_VOICE) ||
                    track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                {
                    audio_mgr_dispatch(AUDIO_MSG_A2DP_DSP_SYNC_LOCK, track->buffer_proxy);
                }
            }
            break;

        case AUDIO_PATH_EVT_DSP_SYNC_V2_SUCC:
            {
                if ((track->stream_type == AUDIO_STREAM_TYPE_VOICE) ||
                    track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
                {
                    audio_mgr_dispatch(AUDIO_MSG_A2DP_DSP_SYNC_V2_SUCC, track->buffer_proxy);
                }
            }
            break;

        case AUDIO_PATH_EVT_DSP_JOIN_INFO:
            {
                audio_mgr_dispatch(AUDIO_MSG_DSP_RETURN_JOIN_INFO, (T_AUDIO_MSG_DSP_JOIN_INFO *)param);
            }
            break;

        case AUDIO_PATH_EXC_DSP_UNSYNC:
            {
                if ((track->stream_type == AUDIO_STREAM_TYPE_VOICE) ||
                    (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK))
                {
                    audio_mgr_dispatch(AUDIO_MSG_DSP_UNSYNC, track->buffer_proxy);
                }
            }
            break;

        case AUDIO_PATH_EXC_DSP_SYNC_EMPTY:
            {
                audio_mgr_dispatch(AUDIO_MSG_DSP_SYNC_EMPTY, track->buffer_proxy);
            }
            break;

        case AUDIO_PATH_EXC_DSP_LOST_TIMESTAMP:
            {
                audio_mgr_dispatch(AUDIO_MSG_DSP_LOST_TIMESTAMP, track->buffer_proxy);
            }
            break;

        default:
            break;
        }

        return true;
    }

    return false;
}

static bool audio_session_state_set(T_AUDIO_TRACK *track, T_AUDIO_SESSION_STATE state)
{
    bool ret = false;

    AUDIO_PRINT_TRACE5("audio_session_state_set: track %p, curr state %u, next state %u, action %u, codec %u",
                       track, track->state, state, track->action, track->format_info.type);

    switch (track->state)
    {
    case AUDIO_SESSION_STATE_RELEASED:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            T_AUDIO_CATEGORY       path_category;
            path_category  = audio_track_path_category_convert(track->stream_type);

            track->path_handle = audio_path_create(path_category,
                                                   track->device,
                                                   &track->format_info,
                                                   track->mode,
                                                   track->volume_out,
                                                   track->volume_in,
                                                   audio_track_path_cback);
            if (track->path_handle != NULL)
            {
                /* Update track session state first, as audio path callback
                 * will invoke audio_track_start.
                 */
                track->state = state;

                /* Enqueue track before audio path created, as audio path callback
                 * may manipulate the idle track first.
                 */
                audio_track_queue_add(&audio_track_db.idle_tracks, track, false);

                /* Attach track to the matched media buffer. */
                audio_track_buffer_attach(track);

                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_CREATED);
                if (ret == false)
                {
                    track->state = AUDIO_SESSION_STATE_RELEASED;
                    audio_track_buffer_detach(track);
                    audio_track_queue_remove(track);
                }
            }
            else
            {
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            /* Starting released audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            /* Stopping released audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            /* Pausing released audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            /* Restarting released audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            /* Releasing released audio track is forbidden. */
            ret = false;
        }
        break;

    case AUDIO_SESSION_STATE_CREATING:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_CREATE payload;

            track->state        = state;

            payload.handle      = track;
            payload.stream_type = track->stream_type;

            audio_mgr_dispatch(AUDIO_MSG_TRACK_CREATE, &payload);

            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_STARTING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_RESTARTING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_RELEASING);
            }
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Start audio track immediately after audio_track_create invoked,
                 * and the audio path created event has not been received, so this
                 * action should be pending.
                 */
                track->action = AUDIO_TRACK_ACTION_START;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Drop repeating audio_track_start invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Override the pending restart action for the creating audio track. */
                track->action = AUDIO_TRACK_ACTION_START;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the creating audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Stopping the creating audio track is forbidden. */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Cancel all pending actions when the creating audio track is
                 * pending for start action, but receives the stop action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Cancel all pending actions when the creating audio track is
                 * pending for restart action, but receives the stop action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the creating audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Pausing the creating audio track is forbidden. */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Cancel all pending actions when the creating audio track is
                 * pending for start action, but receives the pause action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Cancel all pending actions when the creating audio track is
                 * pending for restart action, but receives the pause action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the creating audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Restart audio track immediately after audio_track_create invoked,
                 * and the audio path created event has not been received, so this
                 * action should be pending.
                 */
                track->action = AUDIO_TRACK_ACTION_RESTART;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Override the pending start action for the creating audio track. */
                track->action = AUDIO_TRACK_ACTION_RESTART;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Drop repeating audio_track_restart invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the creating audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Override the pending start action for the creating audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Override the pending restart action for the creating audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* Drop repeating audio_track_release invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case AUDIO_SESSION_STATE_CREATED:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            if (audio_track_db.multi_instance_enable == false)
            {
                if (audio_track_is_starting(track->stream_type))
                {
                    track->action = AUDIO_TRACK_ACTION_START;
                    break;
                }
            }

            track->state = state;

            ret = audio_track_queue_remove(track);
            if (ret == false)
            {
                AUDIO_PRINT_ERROR1("audio_session_state_set: failed to remove track %p", track);

                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_CREATED;
                break;
            }

            if (audio_track_db.multi_instance_enable == false)
            {
                /* Only one of the tracks with the same stream type can be playing. */
                audio_track_queue_freeze(track->stream_type);

                /* Suppress all audio playback tracks in the voice scenario. */
                if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
                {
                    audio_track_queue_freeze(AUDIO_STREAM_TYPE_PLAYBACK);
                }
            }

            /* The lastest started track has the highest priority. */
            if (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                audio_track_queue_add(&audio_track_db.playback_tracks, track, true);
            }
            else if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                audio_track_queue_add(&audio_track_db.voice_tracks, track, true);
            }
            else if (track->stream_type == AUDIO_STREAM_TYPE_RECORD)
            {
                audio_track_queue_add(&audio_track_db.record_tracks, track, true);
            }

            ret = audio_path_start(track->path_handle);
            if (ret == false)
            {
                /* TODO resume other tracks. */

                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_CREATED;
                audio_track_queue_add(&audio_track_db.idle_tracks, track, false);
                break;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            if (track->action == AUDIO_TRACK_ACTION_START || track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
            }
            /* Stopping the created audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            if (track->action == AUDIO_TRACK_ACTION_START || track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
            }
            /* Pausing the created audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            if (audio_track_db.multi_instance_enable == false)
            {
                if (audio_track_is_starting(track->stream_type))
                {
                    track->action = AUDIO_TRACK_ACTION_RESTART;
                    break;
                }
            }

            track->state = state;

            ret = audio_track_queue_remove(track);
            if (ret == false)
            {
                AUDIO_PRINT_ERROR1("audio_session_state_set: failed to remove track %p", track);

                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_CREATED;
                break;
            }

            if (audio_track_db.multi_instance_enable == false)
            {
                /* Only one of the tracks with the same stream type can be playing. */
                audio_track_queue_freeze(track->stream_type);

                /* Suppress all audio playback tracks in the voice scenario. */
                if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
                {
                    audio_track_queue_freeze(AUDIO_STREAM_TYPE_PLAYBACK);
                }
            }

            /* The lastest started track has the highest priority. */
            if (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                audio_track_queue_add(&audio_track_db.playback_tracks, track, true);
            }
            else if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                audio_track_queue_add(&audio_track_db.voice_tracks, track, true);
            }
            else if (track->stream_type == AUDIO_STREAM_TYPE_RECORD)
            {
                audio_track_queue_add(&audio_track_db.record_tracks, track, true);
            }

            ret = audio_path_start(track->path_handle);
            if (ret == false)
            {
                /* TODO resume other tracks. */

                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_CREATED;
                audio_track_queue_add(&audio_track_db.idle_tracks, track, false);
                break;
            }
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            track->state = AUDIO_SESSION_STATE_RELEASING;

            /* Detach track's media buffer. */
            audio_track_buffer_detach(track);

            ret = audio_path_destory(track->path_handle);
            if (ret == false)
            {
                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_CREATED;
                audio_track_buffer_attach(track);
                break;
            }
        }
        break;

    case AUDIO_SESSION_STATE_STARTING:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Drop repeating audio_track_start invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Cancel all pending actions when the starting audio track is
                 * pending for stop action, but receives the start action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* Cancel all pending actions when the starting audio track is
                 * pending for pause action, but receives the start action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the starting audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_START payload;

            track->state        = state;
            track->effect_apply = false;
            track->seq_in_num = 0;
            /* Activate the track's bound media buffer. */
            audio_track_buffer_play(track);
            audio_track_volume_apply(track);
            media_buffer_low_latency_config(track->buffer_proxy);

            if (track->signal_out)
            {
                if (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK ||
                    track->stream_type == AUDIO_STREAM_TYPE_VOICE)
                {
                    audio_path_signal_out_monitoring_set(track->path_handle, true, track->signal_out_interval);
                }
            }

            if (track->signal_in)
            {
                if (track->stream_type == AUDIO_STREAM_TYPE_VOICE || track->stream_type == AUDIO_STREAM_TYPE_RECORD)
                {
                    audio_path_signal_in_monitoring_set(track->path_handle, true, track->signal_in_interval);
                }
            }

            payload.handle       = track;
            payload.stream_type  = track->stream_type;
            payload.buffer_proxy = track->buffer_proxy;

            audio_mgr_dispatch(AUDIO_MSG_TRACK_START, &payload);

            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                audio_track_reschedule();
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_STOPPING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_PAUSING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_RESTARTING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_RELEASING);
            }
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_STOP;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Drop repeating audio_track_stop invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* Override the pending pause action for the starting audio track. */
                track->action = AUDIO_TRACK_ACTION_STOP;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the starting audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_PAUSE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Drop the pause action for the starting audio track, as the
                 * pause action cannot override the pending stop action.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* Drop repeating audio_track_pause invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the starting audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Drop audio_track_restart invocation. But this case
                 * is permitted.
                 */
                track->action = AUDIO_TRACK_ACTION_RESTART;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                track->action = AUDIO_TRACK_ACTION_RESTART;
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Cancel all pending actions when the starting audio track is
                 * pending for stop action, but receives the restart action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* Cancel all pending actions when the starting audio track is
                 * pending for pause action, but receives the restart action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the starting audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Override the pending stop action for the starting audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* Override the pending pause action for the starting audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* Drop repeating audio_track_release invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case AUDIO_SESSION_STATE_STARTED:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            /* Starting the started audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            track->state = state;

            audio_track_effect_stop(track);

            ret = audio_path_stop(track->path_handle);
            if (ret == false)
            {
                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_STARTED;
                break;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            track->state = state;

            audio_track_effect_stop(track);

            /* Currently, there is no audio path pause interface. */
            ret = audio_path_stop(track->path_handle);
            if (ret == false)
            {
                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_STARTED;
                break;
            }
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_PAUSE payload;

            track->state        = state;

            payload.handle      = track;
            payload.stream_type = track->stream_type;
            payload.buffer_proxy = track->buffer_proxy;

            audio_mgr_dispatch(AUDIO_MSG_TRACK_SUSPEND, &payload);
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            track->state = state;

            audio_track_effect_stop(track);

            ret = audio_path_stop(track->path_handle);
            if (ret == false)
            {
                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_STARTED;
                break;
            }
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            /* Started audio track cannot be released directly, so it should be
             * stopped first before released.
             */
            track->state  = AUDIO_SESSION_STATE_STOPPING;
            track->action = AUDIO_TRACK_ACTION_RELEASE;

            audio_track_effect_stop(track);

            ret = audio_path_stop(track->path_handle);
            if (ret == false)
            {
                /* Restore the track state. */
                track->state  = AUDIO_SESSION_STATE_STARTED;
                track->action = AUDIO_TRACK_ACTION_NONE;
                break;
            }
        }
        break;

    case AUDIO_SESSION_STATE_STOPPING:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_START;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Drop repeating audio_track_start invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Override the pending restart action for the stopping audio track. */
                track->action = AUDIO_TRACK_ACTION_START;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the stopping audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Drop repeating audio_track_stop invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Cancel all pending actions when the stopping audio track is
                 * pending for start action, but receives the stop action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Cancel all pending actions when the stopping audio track is
                 * pending for restart action, but receives the stop action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the stopping audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_STOP payload;

            track->state        = state;
            /* Deactivate the track's bound media buffer. */
            audio_track_buffer_stop(track);

            payload.handle      = track;
            payload.stream_type = track->stream_type;

            audio_mgr_dispatch(AUDIO_MSG_TRACK_STOP, &payload);

            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                audio_track_reschedule();
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_STARTING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_RESTARTING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_RELEASING);
            }
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Drop the pause action for the stopping audio track, as the
                 * pause action cannot override the stop action.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Cancel all pending actions when the stopping audio track is
                 * pending for start action, but receives the pause action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Cancel all pending actions when the stopping audio track is
                 * pending for restart action, but receives the pause action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the stopping audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_RESTART;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Override the pending start action for the stopping audio track. */
                track->action = AUDIO_TRACK_ACTION_RESTART;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Drop repeating audio_track_start invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the stopping audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Override the pending start action for the stopping audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Override the pending restart action for the stopping audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* Drop repeating audio_track_release invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case AUDIO_SESSION_STATE_STOPPED:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            track->state = state;

            ret = audio_track_queue_remove(track);
            if (ret == false)
            {
                AUDIO_PRINT_ERROR1("audio_session_state_set: failed to remove track %p", track);

                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_STOPPED;
                break;
            }

            if (audio_track_db.multi_instance_enable == false)
            {
                /* Only one of the tracks with the same stream type can be playing. */
                audio_track_queue_freeze(track->stream_type);

                /* Suppress all audio playback tracks in the voice scenario. */
                if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
                {
                    audio_track_queue_freeze(AUDIO_STREAM_TYPE_PLAYBACK);
                }
            }


            /* TODO record the lastest track; check if it is, do not reset (pause) the bound media buffer. */
            /* Currently, reset the media buffer. */

            /* The lastest started track has the highest priority. */
            if (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                audio_track_queue_add(&audio_track_db.playback_tracks, track, true);
            }
            else if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                audio_track_queue_add(&audio_track_db.voice_tracks, track, true);
            }
            else if (track->stream_type == AUDIO_STREAM_TYPE_RECORD)
            {
                audio_track_queue_add(&audio_track_db.record_tracks, track, true);
            }

            ret = audio_path_start(track->path_handle);
            if (ret == false)
            {
                /* TODO resume other tracks. */

                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_STOPPED;
                break;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            /* Stopping the stopped audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            /* Pausing the stopped audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            track->state = state;

            ret = audio_track_queue_remove(track);
            if (ret == false)
            {
                AUDIO_PRINT_ERROR1("audio_session_state_set: failed to remove track %p", track);

                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_STOPPED;
                break;
            }

            if (audio_track_db.multi_instance_enable == false)
            {
                /* Only one of the tracks with the same stream type can be playing. */
                audio_track_queue_freeze(track->stream_type);

                /* Suppress all audio playback tracks in the voice scenario. */
                if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
                {
                    audio_track_queue_freeze(AUDIO_STREAM_TYPE_PLAYBACK);
                }
            }


            /* TODO record the lastest track; check if it is, do not reset (pause) the bound media buffer. */
            /* Currently, reset the media buffer. */

            /* The lastest started track has the highest priority. */
            if (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
            {
                audio_track_queue_add(&audio_track_db.playback_tracks, track, true);
            }
            else if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                audio_track_queue_add(&audio_track_db.voice_tracks, track, true);
            }
            else if (track->stream_type == AUDIO_STREAM_TYPE_RECORD)
            {
                audio_track_queue_add(&audio_track_db.record_tracks, track, true);
            }

            ret = audio_path_start(track->path_handle);
            if (ret == false)
            {
                /* TODO resume other tracks. */

                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_STOPPED;
                break;
            }
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            track->state = AUDIO_SESSION_STATE_RELEASING;

            /* Detach track's media buffer. */
            audio_track_buffer_detach(track);

            ret = audio_path_destory(track->path_handle);
            if (ret == false)
            {
                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_STOPPED;
                audio_track_buffer_attach(track);
                break;
            }
        }
        break;

    case AUDIO_SESSION_STATE_PAUSING:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_START;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Drop repeating audio_track_start invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will override the pausing state directly.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Override the pending restart action for the pausing audio track. */
                track->action = AUDIO_TRACK_ACTION_START;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the pausing audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_STOP;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Cancel all pending actions when the pausing audio track is
                 * pending for start action, but receives the stop action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will override the pausing state directly.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Cancel all pending actions when the pausing audio track is
                 * pending for restart action, but receives the stop action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the starting audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_PAUSE payload;

            track->state        = AUDIO_SESSION_STATE_PAUSED;

            payload.handle      = track;
            payload.stream_type = track->stream_type;
            payload.buffer_proxy = track->buffer_proxy;

            audio_mgr_dispatch(AUDIO_MSG_TRACK_PAUSE, &payload);

            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                audio_track_reschedule();
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_STARTING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will override the pausing state directly.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_RESTARTING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_RELEASING);
            }
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Drop repeating audio_track_pause invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Cancel all pending actions when the pausing audio track is
                 * pending for start action, but receives the pause action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will override the pausing state directly.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Cancel all pending actions when the pausing audio track is
                 * pending for restart action, but receives the pause action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the pausing audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality, as the pausing
             * audio path will send stopped event directly.
             */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_RESTART;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Override the pending start action for the pausing audio track. */
                track->action = AUDIO_TRACK_ACTION_RESTART;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will override the pausing state directly.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Drop repeating audio_track_restart invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the pausing audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* Override the pending start action for the pausing audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Override the pending start action for the pausing audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* Override the pending restart action for the pausing audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* Drop repeating audio_track_release invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case AUDIO_SESSION_STATE_PAUSED:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            track->state = state;

            if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                audio_track_queue_freeze(AUDIO_STREAM_TYPE_PLAYBACK);
            }

            ret = audio_path_start(track->path_handle);

            if (ret == false)
            {
                track->state = AUDIO_SESSION_STATE_PAUSED;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_START payload;

            track->state        = state;
            track->effect_apply = false;
            track->seq_in_num = 0;

            audio_track_volume_apply(track);

            media_buffer_low_latency_config(track->buffer_proxy);

            payload.handle       = track;
            payload.stream_type  = track->stream_type;
            payload.buffer_proxy = track->buffer_proxy;

            /* This state changing event is triggered from audio path. */
            audio_mgr_dispatch(AUDIO_MSG_TRACK_SUSPEND_RESUME, &payload);
            audio_track_reschedule();
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            track->state = state;

            ret = audio_path_stop(track->path_handle);
            if (ret == false)
            {
                /* Restore the track state. */
                track->state = AUDIO_SESSION_STATE_STARTED;
                break;
            }

        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            /* Pausing the paused audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            track->state = state;

            /* Suppress all audio playback tracks in the voice scenario. */
            if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
            {
                audio_track_queue_freeze(AUDIO_STREAM_TYPE_PLAYBACK);
            }
            ret = audio_path_start(track->path_handle);
            if (ret == false)
            {
                track->state = AUDIO_SESSION_STATE_PAUSED;
            }
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {

            track->state = AUDIO_SESSION_STATE_STOPPING;
            track->action = AUDIO_TRACK_ACTION_RELEASE;

            /* Deactivate the bound media buffer. */
            audio_track_buffer_stop(track);

            audio_path_stop(track->path_handle);
        }
        break;

    case AUDIO_SESSION_STATE_RESTARTING:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Drop audio_track_start invocation. But this case is permitted. */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same restarting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Cancel all pending actions when the restarting audio track is
                 * pending for stop action, but receives the start action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* Cancel all pending actions when the restarting audio track is
                 * pending for pause action, but receives the start action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the restarting audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_RESTART payload;

            track->state         = state;
            track->effect_apply  = false;
            track->seq_in_num = 0;
            payload.handle       = track;
            payload.stream_type  = track->stream_type;
            payload.buffer_proxy = track->buffer_proxy;

            /* Activate the track's bound media buffer. */
            audio_track_buffer_play(track);
            audio_track_volume_apply(track);

            media_buffer_low_latency_config(track->buffer_proxy);

            if (track->signal_out)
            {
                if (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK ||
                    track->stream_type == AUDIO_STREAM_TYPE_VOICE)
                {
                    audio_path_signal_out_monitoring_set(track->path_handle, true, track->signal_out_interval);
                }
            }

            if (track->signal_in)
            {
                if (track->stream_type == AUDIO_STREAM_TYPE_VOICE ||
                    track->stream_type == AUDIO_STREAM_TYPE_RECORD)
                {
                    audio_path_signal_in_monitoring_set(track->path_handle, true, track->signal_in_interval);
                }
            }

            audio_mgr_dispatch(AUDIO_MSG_TRACK_RESTART, &payload);

            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                audio_track_reschedule();
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same restarting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_STOPPING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_PAUSING);
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = audio_session_state_set(track, AUDIO_SESSION_STATE_RELEASING);
            }
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_STOP;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same restarting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Drop repeating audio_track_stop invocation. But this case
                * is permitted.
                */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* Override the pending pause action for the restarting audio track. */
                track->action = AUDIO_TRACK_ACTION_STOP;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the restarting audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {

            ret = audio_path_start(track->path_handle);
            if (ret == false)
            {
                track->state = state;
            }
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_PAUSE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same restarting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action of the same restarting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Override the pending stop action for the restarting audio track. */
                track->action = AUDIO_TRACK_ACTION_PAUSE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the restarting audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                /* Drop repeating audio_track_restart invocation. But this case is permitted. */
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same restarting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Cancel all pending actions when the restarting audio track is
                 * pending for stop action, but receives the restart action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* Cancel all pending actions when the restarting audio track is
                 * pending for pause action, but receives the restart action later.
                 */
                track->action = AUDIO_TRACK_ACTION_NONE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* When the restarting audio track is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            if (track->action == AUDIO_TRACK_ACTION_NONE)
            {
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same restarting audio track is impossible.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_STOP)
            {
                /* Override the pending stop action for the restarting audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_PAUSE)
            {
                /* Override the pending pause action for the restarting audio track. */
                track->action = AUDIO_TRACK_ACTION_RELEASE;
                ret = true;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RESTART)
            {
                /* This case will not happen in reality, as the pending restart
                 * action will be dropped directly in restarting state.
                 */
                ret = false;
            }
            else if (track->action == AUDIO_TRACK_ACTION_RELEASE)
            {
                /* Drop repeating audio_track_release invocation. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case AUDIO_SESSION_STATE_RELEASING:
        if (state == AUDIO_SESSION_STATE_RELEASED)
        {
            T_AUDIO_MSG_PAYLOAD_TRACK_RELEASE payload;

            track->state        = state;
            track->action       = AUDIO_TRACK_ACTION_NONE;

            payload.handle      = track;
            payload.stream_type = track->stream_type;

            audio_track_effect_clear(track);
            audio_track_queue_remove(track);
            audio_mgr_dispatch(AUDIO_MSG_TRACK_RELEASE, &payload);
            os_mem_free(track);
            audio_track_reschedule();
        }
        else if (state == AUDIO_SESSION_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTING)
        {
            /* Starting the releasing audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPING)
        {
            /* Stopping the releasing audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSING)
        {
            /* Pausing the releasing audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RESTARTING)
        {
            /* Restarting the releasing audio track is forbidden. */
            ret = false;
        }
        else if (state == AUDIO_SESSION_STATE_RELEASING)
        {
            /* Drop repeating audio_track_release invocation. But this case
             * is permitted.
             */
            ret = true;
        }
        break;
    }

    return ret;
}

bool audio_track_init(void)
{
    os_queue_init(&audio_track_db.idle_tracks);
    os_queue_init(&audio_track_db.playback_tracks);
    os_queue_init(&audio_track_db.voice_tracks);
    os_queue_init(&audio_track_db.record_tracks);

    audio_track_db.playback_buffer_owner = NULL;
    audio_track_db.voice_buffer_owner    = NULL;
    audio_track_db.record_buffer_owner   = NULL;

    audio_track_db.playback_volume_max      = VOLUME_PLAYBACK_MAX_LEVEL;
    audio_track_db.playback_volume_min      = VOLUME_PLAYBACK_MIN_LEVEL;
    audio_track_db.playback_volume_default  = VOLUME_PLAYBACK_DEFAULT_LEVEL;
    audio_track_db.playback_volume_muted    = false;
    audio_track_db.playback_volume_scale    = 0.0f;

    audio_track_db.voice_volume_out_max     = VOLUME_VOICE_OUT_MAX_LEVEL;
    audio_track_db.voice_volume_out_min     = VOLUME_VOICE_OUT_MIN_LEVEL;
    audio_track_db.voice_volume_out_default = VOLUME_VOICE_OUT_DEFAULT_LEVEL;
    audio_track_db.voice_volume_out_muted   = false;
    audio_track_db.voice_volume_out_scale   = 0.0f;

    audio_track_db.voice_volume_in_max      = VOLUME_VOICE_IN_MAX_LEVEL;
    audio_track_db.voice_volume_in_min      = VOLUME_VOICE_IN_MIN_LEVEL;
    audio_track_db.voice_volume_in_default  = VOLUME_VOICE_IN_DEFAULT_LEVEL;
    audio_track_db.voice_volume_in_muted    = false;

    audio_track_db.record_volume_max        = VOLUME_RECORD_MAX_LEVEL;
    audio_track_db.record_volume_min        = VOLUME_RECORD_MIN_LEVEL;
    audio_track_db.record_volume_default    = VOLUME_RECORD_DEFAULT_LEVEL;
    audio_track_db.record_volume_muted      = false;

    audio_track_db.multi_instance_enable    = false;

    return true;
}

void audio_track_deinit(void)
{

}

bool audio_track_policy_set(T_AUDIO_TRACK_POLICY policy)
{
    if (policy == AUDIO_TRACK_POLICY_SINGLE_STREAM)
    {
        audio_track_db.multi_instance_enable = false;
    }
    else if (policy == AUDIO_TRACK_POLICY_MULTI_STREAM)
    {
        audio_track_db.multi_instance_enable = true;
    }

    return true;
}

T_AUDIO_TRACK_HANDLE audio_track_create(T_AUDIO_STREAM_TYPE    stream_type,
                                        T_AUDIO_STREAM_MODE    mode,
                                        T_AUDIO_STREAM_USAGE   usage,
                                        T_AUDIO_FORMAT_INFO    format_info,
                                        uint8_t                volume_out,
                                        uint8_t                volume_in,
                                        uint32_t               device,
                                        P_AUDIO_TRACK_ASYNC_IO async_write,
                                        P_AUDIO_TRACK_ASYNC_IO async_read)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = os_mem_alloc2(sizeof(T_AUDIO_TRACK));
    if (track == NULL)
    {
        ret = 1;
        goto fail_alloc_track;
    }

    AUDIO_PRINT_INFO8("audio_track_create: handle %p, stream_type %u, mode %u, usage %u, "
                      "volume_out %u, volume_in %u, async_write %p, async_read %p",
                      track, stream_type, mode, usage, volume_out, volume_in, async_write, async_read);

    track->container        = NULL;
    track->state            = AUDIO_SESSION_STATE_RELEASED;
    track->action           = AUDIO_TRACK_ACTION_NONE;
    track->stream_type      = stream_type;
    track->mode             = mode;
    track->usage            = usage;
    track->format_info      = format_info;
    track->device           = device;
    track->latency_fixed    = false;
    track->dynamic_latency_sens = 0;
    track->buffer_state     = AUDIO_BUFFER_STATE_DETACHED;
    track->buffer_proxy     = NULL;
    track->async_write      = async_write;
    track->async_read       = async_read;
    track->path_handle      = NULL;
    track->volume_out_muted = false;
    track->volume_in_muted  = false;
    track->signal_in        = false;
    track->signal_out       = false;
    track->signal_in_interval   = 1000;
    track->signal_out_interval  = 1000;

    if (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        track->latency          = AUDIO_TRACK_PLAYBACK_DEFAULT_LATENCY;
        track->volume_out_max   = audio_track_db.playback_volume_max;
        track->volume_out_min   = audio_track_db.playback_volume_min;
        track->volume_out       = audio_track_db.playback_volume_default;

        if (track->volume_out >= track->volume_out_min &&
            track->volume_out <= track->volume_out_max)
        {
            track->volume_out = volume_out;
        }
    }
    else if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
    {
        track->latency          = AUDIO_TRACK_VOICE_DEFAULT_LATENCY;
        track->volume_out_max   = audio_track_db.voice_volume_out_max;
        track->volume_out_min   = audio_track_db.voice_volume_out_min;
        track->volume_out       = audio_track_db.voice_volume_out_default;
        track->volume_in_max    = audio_track_db.voice_volume_in_max;
        track->volume_in_min    = audio_track_db.voice_volume_in_min;
        track->volume_in        = audio_track_db.voice_volume_in_default;
        track->us_packet_length = media_buffer_cacu_frames_length(&format_info) +
                                  sizeof(T_H2D_STREAM_HEADER2);

        if (track->volume_out >= track->volume_out_min &&
            track->volume_out <= track->volume_out_max)
        {
            track->volume_out = volume_out;
        }

        if (track->volume_in >= track->volume_in_min &&
            track->volume_in <= track->volume_in_max)
        {
            track->volume_in = volume_in;
        }
    }
    else
    {
        track->volume_in_max    = audio_track_db.record_volume_max;
        track->volume_in_min    = audio_track_db.record_volume_min;
        track->volume_in        = audio_track_db.record_volume_default;
        track->us_packet_length = media_buffer_cacu_frames_length(&format_info) +
                                  sizeof(T_H2D_STREAM_HEADER2);

        if (track->volume_in >= track->volume_in_min &&
            track->volume_in <= track->volume_in_max)
        {
            track->volume_in = volume_in;
        }
    }

    track->effect_apply  = false;
    track->effect_bypass = 0;
    os_queue_init(&track->effects);

    if (audio_session_state_set(track, AUDIO_SESSION_STATE_CREATING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return (T_AUDIO_TRACK_HANDLE)track;

fail_set_state:
    os_mem_free(track);
fail_alloc_track:
    AUDIO_PRINT_ERROR1("audio_track_create: failed %d", -ret);
    return NULL;
}

bool audio_track_start(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    AUDIO_PRINT_INFO3("audio_track_start: handle %p, state %u, action %u",
                      handle, track->state, track->action);

    if (audio_session_state_set(track, AUDIO_SESSION_STATE_STARTING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_start: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_restart(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    AUDIO_PRINT_INFO3("audio_track_restart: handle %p, state %u, action %u",
                      handle, track->state, track->action);

    if (audio_session_state_set(track, AUDIO_SESSION_STATE_RESTARTING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_restart: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_pause(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    AUDIO_PRINT_INFO3("audio_track_pause: handle %p, state %u, action %u",
                      handle, track->state, track->action);

    if (audio_session_state_set(track, AUDIO_SESSION_STATE_PAUSING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_pause: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_stop(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    AUDIO_PRINT_INFO3("audio_track_stop: handle %p, state %u, action %u",
                      handle, track->state, track->action);

    if (audio_session_state_set(track, AUDIO_SESSION_STATE_STOPPING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_stop: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_flush(T_AUDIO_TRACK_HANDLE handle)
{
    /* TODO */
    return false;
}

bool audio_track_release(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_invalid_handle;
    }

    AUDIO_PRINT_INFO3("audio_track_release: handle %p, state %u, action %u",
                      handle, track->state, track->action);

    if (audio_session_state_set(track, AUDIO_SESSION_STATE_RELEASING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_invalid_handle:
    AUDIO_PRINT_ERROR2("audio_track_release: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_write(T_AUDIO_TRACK_HANDLE   handle,
                       uint32_t               timestamp,
                       uint16_t               seq_num,
                       T_AUDIO_STREAM_STATUS  status,
                       uint8_t                frame_num,
                       void                  *buf,
                       uint16_t               len,
                       uint16_t              *written_len)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK &&
        track->latency_fixed == false &&
        track->mode == AUDIO_STREAM_MODE_LOW_LATENCY)
    {
        float var;
        media_buffer_inteval_evaluator_update(track->buffer_proxy, seq_num, timestamp);
        if (media_buffer_inteval_evaluator_get_var(track->buffer_proxy, &var))
        {
            media_buffer_latency_set2(track->buffer_proxy, track->latency, track->latency_fixed, var);
        }
    }

    if (track->async_write != NULL)
    {
        ret = 2;
        goto fail_invalid_write;
    }

    if (track->state != AUDIO_SESSION_STATE_STARTED)
    {
        ret = 3;
        goto fail_invalid_state;
    }

    if (media_buffer_ds_write(track->buffer_proxy,
                              timestamp,
                              seq_num,
                              status,
                              frame_num,
                              buf,
                              len,
                              written_len) == false)
    {
        ret = 4;
        goto fail_write_buffer;
    }

    return true;

fail_write_buffer:
fail_invalid_state:
fail_invalid_write:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_write: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_read(T_AUDIO_TRACK_HANDLE   handle,
                      uint32_t              *timestamp,
                      uint16_t              *seq_num,
                      T_AUDIO_STREAM_STATUS *status,
                      uint8_t               *frame_num,
                      void                  *buf,
                      uint16_t               len,
                      uint16_t              *read_len)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->async_read != NULL)
    {
        ret = 2;
        goto fail_invalid_read;
    }

    if (track->state != AUDIO_SESSION_STATE_STARTED)
    {
        ret = 3;
        goto fail_invalid_state;
    }

    if (track->buffer_state == AUDIO_BUFFER_STATE_PLAYED)
    {
        *read_len = media_buffer_us_read(track->buffer_proxy, buf, len, timestamp, seq_num, status,
                                         frame_num);
    }
    else
    {
        ret = 4;
        goto fail_invalid_buffer;
    }

    return true;

fail_invalid_buffer:
fail_invalid_state:
fail_invalid_read:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_read: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_threshold_set(T_AUDIO_TRACK_HANDLE handle,
                               uint16_t             upper_threshold,
                               uint16_t             lower_threshold)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_invalid_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_PLAYBACK)
    {
        ret = 2;
        goto fail_invalid_type;
    }

    AUDIO_PRINT_INFO3("audio_track_threshold_set: handle %p, upper_threshold %u, lower_threshold %u",
                      handle, upper_threshold, lower_threshold);

    media_buffer_threshold_set(track->buffer_proxy, upper_threshold, lower_threshold);
    return true;

fail_invalid_type:
fail_invalid_handle:
    AUDIO_PRINT_ERROR2("audio_track_threshold_set: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_state_get(T_AUDIO_TRACK_HANDLE  handle,
                           T_AUDIO_TRACK_STATE  *state)
{
    T_AUDIO_TRACK *track;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == true)
    {
        switch (track->state)
        {
        case AUDIO_SESSION_STATE_RELEASED:
        case AUDIO_SESSION_STATE_RELEASING:
            *state = AUDIO_TRACK_STATE_RELEASED;
            break;

        case AUDIO_SESSION_STATE_CREATING:
        case AUDIO_SESSION_STATE_CREATED:
            *state = AUDIO_TRACK_STATE_CREATED;
            break;

        case AUDIO_SESSION_STATE_STARTING:
        case AUDIO_SESSION_STATE_STARTED:
        case AUDIO_SESSION_STATE_RESTARTING:
            *state = AUDIO_TRACK_STATE_STARTED;
            break;

        case AUDIO_SESSION_STATE_STOPPING:
        case AUDIO_SESSION_STATE_STOPPED:
            *state = AUDIO_TRACK_STATE_STOPPED;
            break;

        case AUDIO_SESSION_STATE_PAUSING:
        case AUDIO_SESSION_STATE_PAUSED:
            *state = AUDIO_TRACK_STATE_PAUSED;
            break;
        }

        return true;
    }

    return false;
}

bool audio_track_stream_type_get(T_AUDIO_TRACK_HANDLE handle,
                                 T_AUDIO_STREAM_TYPE *stream_type)
{
    T_AUDIO_TRACK *track;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == true)
    {
        *stream_type = track->stream_type;
        return true;
    }

    return false;
}

bool audio_track_format_info_get(T_AUDIO_TRACK_HANDLE handle,
                                 T_AUDIO_FORMAT_INFO *format_info)
{
    T_AUDIO_TRACK *track;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == true)
    {
        *format_info = track->format_info;
        return true;
    }

    return false;
}

bool audio_track_device_get(T_AUDIO_TRACK_HANDLE handle,
                            uint32_t *device)
{
    T_AUDIO_TRACK *track;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == true)
    {
        *device = track->device;
        return true;
    }

    return false;
}

bool audio_track_mode_get(T_AUDIO_TRACK_HANDLE  handle,
                          T_AUDIO_STREAM_MODE  *mode)
{
    T_AUDIO_TRACK *track;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == true)
    {
        *mode = track->mode;
        return true;
    }

    return false;
}

bool audio_track_usage_get(T_AUDIO_TRACK_HANDLE  handle,
                           T_AUDIO_STREAM_USAGE *usage)

{
    T_AUDIO_TRACK *track;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == true)
    {
        *usage = track->usage;
        return true;
    }

    return false;
}

bool audio_track_latency_max_get(T_AUDIO_TRACK_HANDLE handle,
                                 uint16_t *latency)
{
    /* TODO */
    return false;
}

bool audio_track_latency_min_get(T_AUDIO_TRACK_HANDLE handle,
                                 uint16_t *latency)
{
    /* TODO */
    return false;
}

bool audio_track_latency_get(T_AUDIO_TRACK_HANDLE  handle,
                             uint16_t             *latency)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_invalid_handle;
    }

    *latency = track->latency;
    return true;

fail_invalid_handle:
    AUDIO_PRINT_ERROR2("audio_track_latency_get: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_latency_set(T_AUDIO_TRACK_HANDLE handle,
                             uint16_t             latency,
                             bool                 fixed)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_invalid_handle;
    }

    AUDIO_PRINT_INFO3("audio_track_latency_set: handle %p, latency %u fixed %u",
                      handle, latency, fixed);

    track->latency_fixed = fixed;
    track->latency = latency;
    media_buffer_latency_set(track->buffer_proxy, latency, fixed);
    return true;

fail_invalid_handle:
    AUDIO_PRINT_ERROR2("audio_track_latency_set: handle %p, failed %d", handle, -ret);
    return false;
}

bool audio_track_plc_notify_set(T_AUDIO_TRACK_HANDLE handle,
                                uint16_t             interval,
                                uint32_t             threshold,
                                bool                 enable)
{
    if (audio_track_handle_check(handle) == false)
    {
        return false;
    }

    audio_path_plc_notify_set(interval, threshold, enable);
    AUDIO_PRINT_INFO3("audio_track_plc_notify_set: handle %p, interval %u enable %u",
                      handle, interval, enable);

    return true;
}

bool audio_track_buffer_level_get(T_AUDIO_TRACK_HANDLE handle, uint16_t *level)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;
    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_invalid_handle;
    }

    if (media_buffer_level_get(track->buffer_proxy, level) == false)
    {
        ret = 2;
        goto fail_invalid_level;
    }

    return true;

fail_invalid_level:
fail_invalid_handle:
    AUDIO_PRINT_ERROR2("audio_track_buffer_level_get: handle %p, failed %d", handle, -ret);
    return false;
}

static uint8_t audio_track_volume_scale(uint8_t volume,
                                        uint8_t src_volume,
                                        uint8_t dst_volume)
{
    return (volume * dst_volume + src_volume / 2) / src_volume;
}

bool audio_track_volume_out_max_get(T_AUDIO_TRACK_HANDLE  handle,
                                    uint8_t              *volume)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_PLAYBACK &&
        track->stream_type != AUDIO_STREAM_TYPE_VOICE)
    {
        ret = 2;
        goto fail_check_type;
    }

    *volume = track->volume_out_max;
    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_out_max_get: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_out_max_set(T_AUDIO_STREAM_TYPE type,
                                    uint8_t             volume)
{
    T_OS_QUEUE    *queue;
    T_AUDIO_TRACK *track;
    uint8_t        volume_max = 0;
    uint8_t        volume_min = 0;
    uint8_t        volume_default = 0;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        volume_max      = audio_track_db.playback_volume_max;
        volume_min      = audio_track_db.playback_volume_min;
        volume_default  = audio_track_db.playback_volume_default;

        audio_track_db.playback_volume_min = audio_track_volume_scale(volume_min,
                                                                      volume_max,
                                                                      volume);
        audio_track_db.playback_volume_default = audio_track_volume_scale(volume_default,
                                                                          volume_max,
                                                                          volume);
        audio_track_db.playback_volume_max = volume;

        queue = &audio_track_db.playback_tracks;
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        volume_max      = audio_track_db.voice_volume_out_max;
        volume_min      = audio_track_db.voice_volume_out_min;
        volume_default  = audio_track_db.voice_volume_out_default;

        audio_track_db.voice_volume_out_min = audio_track_volume_scale(volume_min,
                                                                       volume_max,
                                                                       volume);
        audio_track_db.voice_volume_out_default = audio_track_volume_scale(volume_default,
                                                                           volume_max,
                                                                           volume);
        audio_track_db.voice_volume_out_max = volume;

        queue = &audio_track_db.voice_tracks;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        return false;
    }

    if (volume != volume_max)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            track->volume_out_min   = audio_track_volume_scale(track->volume_out_min,
                                                               track->volume_out_max,
                                                               volume);
            track->volume_out       = audio_track_volume_scale(track->volume_out,
                                                               track->volume_out_max,
                                                               volume);
            track->volume_out_max   = volume;
            track                   = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                track->volume_out_min   = audio_track_volume_scale(track->volume_out_min,
                                                                   track->volume_out_max,
                                                                   volume);
                track->volume_out       = audio_track_volume_scale(track->volume_out,
                                                                   track->volume_out_max,
                                                                   volume);
                track->volume_out_max   = volume;
            }

            track = track->p_next;
        }
    }

    return true;
}

bool audio_track_volume_out_min_get(T_AUDIO_TRACK_HANDLE  handle,
                                    uint8_t              *volume)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_PLAYBACK &&
        track->stream_type != AUDIO_STREAM_TYPE_VOICE)
    {
        ret = 2;
        goto fail_check_type;
    }

    *volume = track->volume_out_min;
    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_out_min_get: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_out_min_set(T_AUDIO_STREAM_TYPE type,
                                    uint8_t             volume)
{
    T_OS_QUEUE    *queue;
    T_AUDIO_TRACK *track;
    uint8_t        volume_min = 0;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        volume_min = audio_track_db.playback_volume_min;
        audio_track_db.playback_volume_min = volume;
        queue = &audio_track_db.playback_tracks;
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        volume_min = audio_track_db.voice_volume_out_min;
        audio_track_db.voice_volume_out_min = volume;
        queue = &audio_track_db.voice_tracks;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        return false;
    }

    if (volume != volume_min)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            track->volume_out_min = volume;
            track                 = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                track->volume_out_min = volume;
            }

            track = track->p_next;
        }
    }

    return true;
}

bool audio_track_volume_out_get(T_AUDIO_TRACK_HANDLE  handle,
                                uint8_t              *volume)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_PLAYBACK &&
        track->stream_type != AUDIO_STREAM_TYPE_VOICE)
    {
        ret = 2;
        goto fail_check_type;
    }

    *volume = track->volume_out;
    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_out_get: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_out_set(T_AUDIO_TRACK_HANDLE handle,
                                uint8_t              volume)
{
    T_AUDIO_TRACK *track;
    T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_OUT_CHANGE payload;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    AUDIO_PRINT_INFO2("audio_track_volume_out_set: volume %u, volume_out_muted %u",
                      volume, track->volume_out_muted);

    if (volume < track->volume_out_min ||
        volume > track->volume_out_max)
    {
        ret = 2;
        goto fail_check_volume;
    }

    if (track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        if ((audio_track_db.playback_volume_muted == false) &&
            (track->volume_out_muted == false))
        {
            if (audio_path_dac_level_set(track->path_handle,
                                         volume,
                                         audio_track_db.playback_volume_scale) == false)
            {
                ret = 3;
                goto fail_set_volume;
            }
        }
    }
    else if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
    {
        if ((audio_track_db.voice_volume_out_muted == false) &&
            (track->volume_out_muted == false))
        {
            if (audio_path_dac_level_set(track->path_handle,
                                         volume,
                                         audio_track_db.voice_volume_out_scale) == false)
            {
                ret = 4;
                goto fail_set_volume;
            }
        }
    }
    else
    {
        ret = 5;
        goto fail_check_type;
    }

    payload.handle          = track;
    payload.prev_volume     = track->volume_out;
    payload.curr_volume     = volume;

    track->volume_out       = volume;

    return audio_mgr_dispatch(AUDIO_MSG_TRACK_VOLUME_OUT_CHANGE, &payload);

fail_check_type:
fail_set_volume:
fail_check_volume:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_out_set: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_out_mute(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_OUT_MUTE payload;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_PLAYBACK &&
        track->stream_type != AUDIO_STREAM_TYPE_VOICE)
    {
        ret = 2;
        goto fail_check_type;
    }

    if (track->volume_out_muted == true)
    {
        ret = 3;
        goto fail_check_muted;
    }

    if (audio_path_dac_mute(track->path_handle) == false)
    {
        ret = 4;
        goto fail_mute_volume;
    }

    track->volume_out_muted = true;

    payload.handle  = track;
    payload.volume  = track->volume_out;

    audio_mgr_dispatch(AUDIO_MSG_TRACK_VOLUME_OUT_MUTE, &payload);
    return true;

fail_mute_volume:
fail_check_muted:
fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_out_mute: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_out_unmute(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_OUT_UNMUTE payload;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_PLAYBACK &&
        track->stream_type != AUDIO_STREAM_TYPE_VOICE)
    {
        ret = 2;
        goto fail_check_type;
    }

    if (track->volume_out_muted == false)
    {
        ret = 3;
        goto fail_check_muted;
    }

    if ((track->stream_type == AUDIO_STREAM_TYPE_PLAYBACK) &&
        (audio_track_db.playback_volume_muted == false))
    {
        if (audio_path_dac_level_set(track->path_handle,
                                     track->volume_out,
                                     audio_track_db.playback_volume_scale) == false)
        {
            ret = 4;
            goto fail_unmute_volume;
        }
    }
    else if ((track->stream_type == AUDIO_STREAM_TYPE_VOICE) &&
             (audio_track_db.voice_volume_out_muted == false))
    {
        if (audio_path_dac_level_set(track->path_handle,
                                     track->volume_out,
                                     audio_track_db.voice_volume_out_scale) == false)
        {
            ret = 5;
            goto fail_unmute_volume;
        }
    }

    track->volume_out_muted = false;

    payload.handle  = track;
    payload.volume  = track->volume_out;

    audio_mgr_dispatch(AUDIO_MSG_TRACK_VOLUME_OUT_UNMUTE, &payload);
    return true;

fail_unmute_volume:
fail_check_muted:
fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_out_unmute: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_out_is_muted(T_AUDIO_TRACK_HANDLE  handle, bool *is_muted)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    *is_muted = track->volume_out_muted;
    return true;

fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_out_is_muted: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_signal_out_monitoring_start(T_AUDIO_TRACK_HANDLE handle, uint16_t refresh_interval)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_PLAYBACK &&
        track->stream_type != AUDIO_STREAM_TYPE_VOICE)
    {
        ret = 2;
        goto fail_check_type;
    }
    track->signal_out = true;
    track->signal_out_interval = refresh_interval;
    audio_path_signal_out_monitoring_set(track->path_handle, true, refresh_interval);

    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_signal_out_monitoring_start: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_signal_out_monitoring_stop(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_PLAYBACK &&
        track->stream_type != AUDIO_STREAM_TYPE_VOICE)
    {
        ret = 2;
        goto fail_check_type;
    }

    track->signal_out = false;
    track->signal_out_interval = 0;
    audio_path_signal_out_monitoring_set(track->path_handle, false, 0);

    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_signal_out_monitoring_stop: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_signal_in_monitoring_start(T_AUDIO_TRACK_HANDLE handle, uint16_t refresh_interval)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_RECORD &&
        track->stream_type != AUDIO_STREAM_TYPE_VOICE)
    {
        ret = 2;
        goto fail_check_type;
    }

    track->signal_in = true;
    track->signal_in_interval = refresh_interval;
    audio_path_signal_in_monitoring_set(track->path_handle, true, refresh_interval);

    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_signal_in_monitoring_start: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_signal_in_monitoring_stop(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_RECORD &&
        track->stream_type != AUDIO_STREAM_TYPE_VOICE)
    {
        ret = 2;
        goto fail_check_type;
    }

    track->signal_in = false;
    track->signal_in_interval = 0;
    audio_path_signal_in_monitoring_set(track->path_handle, false, 0);

    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_signal_in_monitoring_stop: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_in_max_get(T_AUDIO_TRACK_HANDLE  handle,
                                   uint8_t              *volume)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_VOICE &&
        track->stream_type != AUDIO_STREAM_TYPE_RECORD)
    {
        ret = 2;
        goto fail_check_type;
    }

    *volume = track->volume_in_max;
    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_in_max_get: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_in_max_set(T_AUDIO_STREAM_TYPE type,
                                   uint8_t             volume)
{
    T_OS_QUEUE    *queue;
    T_AUDIO_TRACK *track;
    uint8_t        volume_max = 0;
    uint8_t        volume_min = 0;
    uint8_t        volume_default = 0;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        return false;

    case AUDIO_STREAM_TYPE_VOICE:
        volume_max      = audio_track_db.voice_volume_in_max;
        volume_min      = audio_track_db.voice_volume_in_min;
        volume_default  = audio_track_db.voice_volume_in_default;

        audio_track_db.voice_volume_in_min = audio_track_volume_scale(volume_min,
                                                                      volume_max,
                                                                      volume);
        audio_track_db.voice_volume_in_default = audio_track_volume_scale(volume_default,
                                                                          volume_max,
                                                                          volume);
        audio_track_db.voice_volume_in_max = volume;

        queue = &audio_track_db.voice_tracks;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        volume_max      = audio_track_db.record_volume_max;
        volume_min      = audio_track_db.record_volume_min;
        volume_default  = audio_track_db.record_volume_default;

        audio_track_db.record_volume_min = audio_track_volume_scale(volume_min,
                                                                    volume_max,
                                                                    volume);
        audio_track_db.record_volume_default = audio_track_volume_scale(volume_default,
                                                                        volume_max,
                                                                        volume);
        audio_track_db.record_volume_max = volume;

        queue = &audio_track_db.record_tracks;
        break;
    }

    if (volume != volume_max)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            track->volume_in_min   = audio_track_volume_scale(track->volume_in_min,
                                                              track->volume_in_max,
                                                              volume);
            track->volume_in       = audio_track_volume_scale(track->volume_in,
                                                              track->volume_in_max,
                                                              volume);
            track->volume_in_max   = volume;
            track                  = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                track->volume_in_min   = audio_track_volume_scale(track->volume_in_min,
                                                                  track->volume_in_max,
                                                                  volume);
                track->volume_in       = audio_track_volume_scale(track->volume_in,
                                                                  track->volume_in_max,
                                                                  volume);
                track->volume_in_max   = volume;
            }

            track = track->p_next;
        }
    }

    return true;
}

bool audio_track_volume_in_min_get(T_AUDIO_TRACK_HANDLE  handle,
                                   uint8_t              *volume)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_VOICE &&
        track->stream_type != AUDIO_STREAM_TYPE_RECORD)
    {
        ret = 2;
        goto fail_check_type;
    }

    *volume = track->volume_out_min;
    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_in_min_get: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_in_min_set(T_AUDIO_STREAM_TYPE type,
                                   uint8_t             volume)
{
    T_OS_QUEUE    *queue;
    T_AUDIO_TRACK *track;
    uint8_t        volume_min = 0;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        return false;

    case AUDIO_STREAM_TYPE_VOICE:
        volume_min = audio_track_db.voice_volume_in_min;
        audio_track_db.voice_volume_in_min = volume;
        queue = &audio_track_db.voice_tracks;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        volume_min = audio_track_db.record_volume_min;
        audio_track_db.record_volume_min = volume;
        queue = &audio_track_db.record_tracks;
        break;
    }

    if (volume != volume_min)
    {
        track = os_queue_peek(queue, 0);
        while (track != NULL)
        {
            track->volume_in_min = volume;
            track                = track->p_next;
        }

        track = os_queue_peek(&audio_track_db.idle_tracks, 0);
        while (track != NULL)
        {
            if (track->stream_type == type)
            {
                track->volume_in_min = volume;
            }

            track = track->p_next;
        }
    }

    return true;
}

bool audio_track_volume_in_get(T_AUDIO_TRACK_HANDLE  handle,
                               uint8_t              *volume)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_VOICE &&
        track->stream_type != AUDIO_STREAM_TYPE_RECORD)
    {
        ret = 2;
        goto fail_check_type;
    }

    *volume = track->volume_in;
    return true;

fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_in_get: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_in_set(T_AUDIO_TRACK_HANDLE handle,
                               uint8_t              volume)
{
    T_AUDIO_TRACK *track;
    T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_IN_CHANGE payload;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    AUDIO_PRINT_INFO2("audio_track_volume_in_set: volume %u, volume_in_muted %u",
                      volume, track->volume_in_muted);

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (volume < track->volume_in_min ||
        volume > track->volume_in_max)
    {
        ret = 2;
        goto fail_check_volume;
    }

    if (track->stream_type == AUDIO_STREAM_TYPE_VOICE)
    {
        if ((audio_track_db.voice_volume_in_muted == false) &&
            (track->volume_in_muted == false))
        {
            if (audio_path_adc_level_set(track->path_handle, volume, 0.0f) == false)
            {
                ret = 3;
                goto fail_set_volume;
            }
        }
    }
    else if (track->stream_type == AUDIO_STREAM_TYPE_RECORD)
    {
        if ((audio_track_db.record_volume_muted == false) &&
            (track->volume_in_muted == false))
        {
            if (audio_path_adc_level_set(track->path_handle, volume, 0.0f) == false)
            {
                ret = 4;
                goto fail_set_volume;
            }
        }
    }
    else
    {
        ret = 5;
        goto fail_check_type;
    }

    payload.handle          = track;
    payload.prev_volume     = track->volume_in;
    payload.curr_volume     = volume;

    track->volume_in        = volume;

    audio_mgr_dispatch(AUDIO_MSG_TRACK_VOLUME_IN_CHANGE, &payload);
    return true;

fail_check_type:
fail_set_volume:
fail_check_volume:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_in_set: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_in_mute(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_IN_MUTE payload;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_VOICE &&
        track->stream_type != AUDIO_STREAM_TYPE_RECORD)
    {
        ret = 2;
        goto fail_check_type;
    }

    if (track->volume_in_muted == true)
    {
        ret = 3;
        goto fail_check_mute;
    }

    if (audio_path_adc_mute(track->path_handle) == false)
    {
        ret = 4;
        goto fail_mute_volume;
    }

    track->volume_in_muted = true;

    payload.handle  = track;
    payload.volume  = track->volume_in;

    audio_mgr_dispatch(AUDIO_MSG_TRACK_VOLUME_IN_MUTE, &payload);
    return true;

fail_mute_volume:
fail_check_mute:
fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_in_mute: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_in_unmute(T_AUDIO_TRACK_HANDLE handle)
{
    T_AUDIO_TRACK *track;
    T_AUDIO_MSG_PAYLOAD_TRACK_VOLUME_IN_UNMUTE payload;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (track->stream_type != AUDIO_STREAM_TYPE_VOICE &&
        track->stream_type != AUDIO_STREAM_TYPE_RECORD)
    {
        ret = 2;
        goto fail_check_type;
    }

    if (track->volume_in_muted == false)
    {
        ret = 3;
        goto fail_check_muted;
    }

    if ((track->stream_type == AUDIO_STREAM_TYPE_VOICE) &&
        (audio_track_db.voice_volume_in_muted == false))
    {
        if (audio_path_adc_level_set(track->path_handle, track->volume_in, 0.0f) == false)
        {
            ret = 4;
            goto fail_unmute_volume;
        }
    }
    else if ((track->stream_type == AUDIO_STREAM_TYPE_RECORD) &&
             (audio_track_db.record_volume_muted == false))
    {
        if (audio_path_adc_level_set(track->path_handle, track->volume_in, 0.0f) == false)
        {
            ret = 5;
            goto fail_unmute_volume;
        }
    }

    track->volume_in_muted = false;

    payload.handle  = track;
    payload.volume  = track->volume_in;

    audio_mgr_dispatch(AUDIO_MSG_TRACK_VOLUME_IN_UNMUTE, &payload);
    return true;

fail_unmute_volume:
fail_check_muted:
fail_check_type:
fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_in_unmute: handle %p, failed %d",
                       handle, -ret);
    return false;
}

bool audio_track_volume_in_is_muted(T_AUDIO_TRACK_HANDLE  handle, bool *is_muted)
{
    T_AUDIO_TRACK *track;
    int32_t ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    *is_muted = track->volume_in_muted;
    return true;

fail_check_handle:
    AUDIO_PRINT_ERROR2("audio_track_volume_in_is_muted: handle %p, failed %d",
                       handle, -ret);
    return false;
}

static void audio_track_effect_cback(T_AUDIO_EFFECT_INSTANCE instance,
                                     T_AUDIO_EFFECT_EVENT    event)
{
    T_AUDIO_TRACK *track;

    track = audio_effect_owner_get(instance);

    AUDIO_PRINT_INFO3("audio_track_effect_cback: instance %p, track %p, event 0x%02x",
                      instance, track, event);

    if (audio_track_handle_check(track) == true)
    {
        switch (event)
        {
        case AUDIO_EFFECT_EVENT_CREATED:
            break;

        case AUDIO_EFFECT_EVENT_ENABLED:
            if (track->state == AUDIO_SESSION_STATE_STARTED)
            {
                audio_effect_run(instance);
            }
            else if (track->state == AUDIO_SESSION_STATE_STARTING ||
                     track->state == AUDIO_SESSION_STATE_PAUSED ||
                     track->state == AUDIO_SESSION_STATE_RESTARTING)
            {
                if (track->effect_apply == true)
                {
                    audio_effect_run(instance);
                }
            }
            break;

        case AUDIO_EFFECT_EVENT_DISABLED:
            if (track->state == AUDIO_SESSION_STATE_STARTED)
            {
                audio_effect_stop(instance);
            }
            else if (track->state == AUDIO_SESSION_STATE_STARTING ||
                     track->state == AUDIO_SESSION_STATE_PAUSED ||
                     track->state == AUDIO_SESSION_STATE_RESTARTING)
            {
                if (track->effect_apply == true)
                {
                    audio_effect_stop(instance);
                }
            }
            break;

        case AUDIO_EFFECT_EVENT_UPDATED:
            if (track->state == AUDIO_SESSION_STATE_STARTED)
            {
                audio_effect_run(instance);
            }
            else if (track->state == AUDIO_SESSION_STATE_STARTING ||
                     track->state == AUDIO_SESSION_STATE_PAUSED ||
                     track->state == AUDIO_SESSION_STATE_RESTARTING)
            {
                if (track->effect_apply == true)
                {
                    audio_effect_run(instance);
                }
            }
            break;

        case AUDIO_EFFECT_EVENT_RELEASED:
            if (track->state == AUDIO_SESSION_STATE_STARTED)
            {
                audio_effect_stop(instance);
            }
            else if (track->state == AUDIO_SESSION_STATE_STARTING ||
                     track->state == AUDIO_SESSION_STATE_PAUSED ||
                     track->state == AUDIO_SESSION_STATE_RESTARTING)
            {
                if (track->effect_apply == true)
                {
                    audio_effect_stop(instance);
                }
            }

            audio_track_effect_dequeue(track, instance);
            audio_effect_owner_clear(instance);
            break;
        }
    }
}

bool audio_track_effect_control(T_AUDIO_TRACK_HANDLE handle,
                                uint8_t              action)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    track->effect_bypass = action;

    if (track->state == AUDIO_SESSION_STATE_STARTED)
    {
        audio_track_effect_control_apply(track, action);
    }

    return true;

fail_check_handle:
    AUDIO_PRINT_ERROR3("audio_track_effect_control: handle %p, action %d, failed %d",
                       handle, action, -ret);
    return false;
}

bool audio_track_effect_attach(T_AUDIO_TRACK_HANDLE    handle,
                               T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (audio_track_effect_enqueue(track, instance) == false)
    {
        ret = 2;
        goto fail_enqueue_effect;
    }

    if (audio_effect_owner_set(instance, track, audio_track_effect_cback) == false)
    {
        ret = 3;
        goto fail_set_owner;
    }

    if (track->state == AUDIO_SESSION_STATE_STARTED)
    {
        audio_effect_run(instance);
    }

    return true;

fail_set_owner:
    audio_track_effect_dequeue(track, instance);
fail_enqueue_effect:
fail_check_handle:
    AUDIO_PRINT_ERROR3("audio_track_effect_attach: handle %p, instance %p, failed %d",
                       handle, instance, -ret);
    return false;
}

bool audio_track_effect_detach(T_AUDIO_TRACK_HANDLE    handle,
                               T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_TRACK *track;
    int32_t        ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_check_handle;
    }

    if (audio_track_effect_check(track, instance) == false)
    {
        ret = 2;
        goto fail_check_instance;
    }

    if (audio_effect_owner_clear(instance) == false)
    {
        ret = 3;
        goto fail_clear_owner;
    }

    if (audio_track_effect_dequeue(track, instance) == false)
    {
        ret = 4;
        goto fail_dequeue_effect;
    }

    if (track->state == AUDIO_SESSION_STATE_STARTED)
    {
        audio_effect_stop(instance);
    }

    return true;

fail_dequeue_effect:
    audio_effect_owner_set(instance, track, audio_track_effect_cback);
fail_clear_owner:
fail_check_instance:
fail_check_handle:
    AUDIO_PRINT_ERROR3("audio_track_effect_detach: handle %p, instance %p, failed %d",
                       handle, instance, -ret);
    return false;
}

static int16_t audio_track_gain_scale(int16_t gain_db, float scale)
{
    float gain;

    if (fabs(scale) >= (0.375f / 128.0f))
    {
        gain  = pow(10.0f, gain_db / 128.0f / 20.0f);
        gain *= scale;
    }
    else
    {
        gain = pow(10.0f, (-128 * 128) / 128.0f / 20.0f);
    }

    return (int16_t)(20 * log10(gain) * 128);
}

bool audio_track_out_scale_set(T_AUDIO_TRACK_HANDLE handle,
                               float                scale_left,
                               float                scale_right)
{
    T_AUDIO_TRACK          *track;
    T_AUDIO_CATEGORY        path_category;
    int32_t                 ret = 0;
    T_AUDIO_ROUTE_DAC_GAIN  route_gain;
    int16_t                 left_gain;
    int16_t                 right_gain;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_invalid_handle;
    }

    if (scale_left < 0.0f || scale_left > 1.0f ||
        scale_right < 0.0f || scale_right > 1.0f)
    {
        ret = 2;
        goto fail_invalid_scale;
    }

    path_category = audio_track_path_category_convert(track->stream_type);
    if (path_category != AUDIO_CATEGORY_AUDIO)
    {
        ret = 3;
        goto fail_invalid_stream_type;
    }

    if (track->state != AUDIO_SESSION_STATE_STARTED)
    {
        ret = 4;
        goto fail_invalid_state;
    }

    if (audio_route_dac_gain_get(AUDIO_CATEGORY_AUDIO, track->volume_out, &route_gain) == true)
    {
        left_gain  = audio_track_gain_scale(route_gain.dac_gain, scale_left);
        right_gain  = audio_track_gain_scale(route_gain.dac_gain, scale_right);

        audio_path_dac_gain_set(AUDIO_CATEGORY_AUDIO, left_gain, right_gain);
    }

    return true;

fail_invalid_state:
fail_invalid_stream_type:
fail_invalid_scale:
fail_invalid_handle:
    AUDIO_PRINT_ERROR4("audio_track_out_scale_set: handle %p, scale_left %d/1000, scale_left %d/1000, ret %d",
                       handle, (int32_t)(scale_left * 1000), (int32_t)(scale_right * 1000), -ret);
    return false;
}

bool audio_track_out_db_set(T_AUDIO_TRACK_HANDLE handle,
                            int16_t              left_db,
                            int16_t              right_db)
{
    T_AUDIO_TRACK          *track;
    T_AUDIO_CATEGORY        path_category;
    int32_t                 ret = 0;

    track = (T_AUDIO_TRACK *)handle;

    if (audio_track_handle_check(handle) == false)
    {
        ret = 1;
        goto fail_invalid_handle;
    }

    if (track->volume_out_muted == true)
    {
        ret = 5;
        goto fail_volume_out_muted;
    }

    if (left_db < -128 || left_db > 0 ||
        right_db < -128 || right_db > 0)
    {
        ret = 2;
        goto fail_invalid_db_value;
    }

    path_category = audio_track_path_category_convert(track->stream_type);
    if (path_category != AUDIO_CATEGORY_AUDIO)
    {
        ret = 3;
        goto fail_invalid_stream_type;
    }

    if (track->state != AUDIO_SESSION_STATE_STARTED)
    {
        ret = 4;
        goto fail_invalid_state;
    }

    audio_path_dac_gain_set(AUDIO_CATEGORY_AUDIO, left_db * 128, right_db * 128);

    return true;

fail_invalid_state:
fail_volume_out_muted:
fail_invalid_stream_type:
fail_invalid_db_value:
fail_invalid_handle:
    AUDIO_PRINT_ERROR4("audio_track_out_db_set: handle %p, left_db 0x%04x, right_db 0x%04x, ret %d",
                       handle, left_db, right_db, -ret);
    return false;
}
