/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "trace.h"
#include "audio.h"
#include "audio_mgr.h"

typedef struct t_audio_volume_db
{
    uint8_t playback_volume_max;
    uint8_t playback_volume_min;
    bool    playback_volume_muted;
    uint8_t playback_volume;
    float   playback_volume_scale;
    uint8_t voice_out_volume_max;
    uint8_t voice_out_volume_min;
    bool    voice_out_volume_muted;
    uint8_t voice_out_volume;
    float   voice_out_volume_scale;
    uint8_t voice_in_volume_max;
    uint8_t voice_in_volume_min;
    bool    voice_in_volume_muted;
    uint8_t voice_in_volume;
    uint8_t record_volume_max;
    uint8_t record_volume_min;
    bool    record_volume_muted;
    uint8_t record_volume;
} T_AUDIO_VOLUME_DB;

static T_AUDIO_VOLUME_DB *audio_volume_db = NULL;

bool audio_volume_init(void)
{
    int32_t ret = 0;

    audio_volume_db = os_mem_alloc2(sizeof(T_AUDIO_VOLUME_DB));
    if (audio_volume_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    audio_volume_db->playback_volume_max    = VOLUME_PLAYBACK_MAX_LEVEL;
    audio_volume_db->playback_volume_min    = VOLUME_PLAYBACK_MIN_LEVEL;
    audio_volume_db->playback_volume_muted  = false;
    audio_volume_db->playback_volume        = VOLUME_PLAYBACK_DEFAULT_LEVEL;
    audio_volume_db->playback_volume_scale  = 0.0f;
    audio_volume_db->voice_out_volume_max   = VOLUME_VOICE_OUT_MAX_LEVEL;
    audio_volume_db->voice_out_volume_min   = VOLUME_VOICE_OUT_MIN_LEVEL;
    audio_volume_db->voice_out_volume_muted = false;
    audio_volume_db->voice_out_volume       = VOLUME_VOICE_OUT_DEFAULT_LEVEL;
    audio_volume_db->voice_out_volume_scale = 0.0f;
    audio_volume_db->voice_in_volume_max    = VOLUME_VOICE_IN_MAX_LEVEL;
    audio_volume_db->voice_in_volume_min    = VOLUME_VOICE_IN_MIN_LEVEL;
    audio_volume_db->voice_in_volume_muted  = false;
    audio_volume_db->voice_in_volume        = VOLUME_VOICE_IN_DEFAULT_LEVEL;
    audio_volume_db->record_volume_max      = VOLUME_RECORD_MAX_LEVEL;
    audio_volume_db->record_volume_min      = VOLUME_RECORD_MIN_LEVEL;
    audio_volume_db->record_volume_muted    = false;
    audio_volume_db->record_volume          = VOLUME_RECORD_DEFAULT_LEVEL;

    return true;

fail_alloc_db:
    AUDIO_PRINT_ERROR1("audio_volume_init: failed %d", -ret);
    return false;
}

void audio_volume_deinit(void)
{
    if (audio_volume_db != NULL)
    {
        os_mem_free(audio_volume_db);
    }
}

uint8_t audio_volume_out_max_get(T_AUDIO_STREAM_TYPE type)
{
    uint8_t max_volume = 0;

    if (audio_volume_db != NULL)
    {
        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            max_volume = audio_volume_db->playback_volume_max;
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            max_volume = audio_volume_db->voice_out_volume_max;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            break;
        }
    }

    return max_volume;
}

bool audio_volume_out_max_set(T_AUDIO_STREAM_TYPE type, uint8_t volume)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_MAX_SET payload;

    if (audio_volume_db != NULL)
    {
        if (volume == 0)
        {
            return false;
        }

        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            audio_volume_db->playback_volume_max = volume;
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            audio_volume_db->voice_out_volume_max = volume;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            return false;
        }

        payload.type    = type;
        payload.volume  = volume;

        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_OUT_MAX_SET, &payload);
    }

    return false;
}

uint8_t audio_volume_out_min_get(T_AUDIO_STREAM_TYPE type)
{
    uint8_t min_volume = 0;

    if (audio_volume_db != NULL)
    {
        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            min_volume = audio_volume_db->playback_volume_min;
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            min_volume = audio_volume_db->voice_out_volume_min;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            break;
        }
    }

    return min_volume;
}

bool audio_volume_out_min_set(T_AUDIO_STREAM_TYPE type, uint8_t volume)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_MIN_SET payload;

    if (audio_volume_db != NULL)
    {
        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            audio_volume_db->playback_volume_min = volume;
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            audio_volume_db->voice_out_volume_min = volume;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            return false;
        }

        payload.type    = type;
        payload.volume  = volume;

        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_OUT_MIN_SET, &payload);
    }

    return false;
}

uint8_t audio_volume_out_get(T_AUDIO_STREAM_TYPE type)
{
    uint8_t curr_volume = 0;

    if (audio_volume_db != NULL)
    {
        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            curr_volume = audio_volume_db->playback_volume;
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            curr_volume = audio_volume_db->voice_out_volume;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            break;
        }
    }

    return curr_volume;
}

bool audio_volume_out_set(T_AUDIO_STREAM_TYPE type, uint8_t volume)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_CHANGE payload;
    uint8_t                               prev_volume;
    uint8_t                               max_volume;
    uint8_t                               min_volume;
    int32_t                               ret = 0;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        {
            if ((volume <= audio_volume_db->playback_volume_max) &&
                (volume >= audio_volume_db->playback_volume_min))
            {
                prev_volume = audio_volume_db->playback_volume;
                audio_volume_db->playback_volume = volume;
            }
            else
            {
                max_volume = audio_volume_db->playback_volume_max;
                min_volume = audio_volume_db->playback_volume_min;
                ret = 1;
            }
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        {
            if ((volume <= audio_volume_db->voice_out_volume_max) &&
                (volume >= audio_volume_db->voice_out_volume_min))
            {
                prev_volume = audio_volume_db->voice_out_volume;
                audio_volume_db->voice_out_volume = volume;
            }
            else
            {
                max_volume = audio_volume_db->voice_out_volume_max;
                min_volume = audio_volume_db->voice_out_volume_min;
                ret = 2;
            }
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        {
            /* No volume out for record stream. */
            prev_volume = audio_volume_db->record_volume;
            max_volume  = audio_volume_db->record_volume_max;
            min_volume  = audio_volume_db->record_volume_min;
            ret = 3;
        }
        break;
    }

    if (ret == 0)
    {
        AUDIO_PRINT_TRACE3("audio_volume_out_set: type %u, prev volume %u, curr volume %u",
                           type, prev_volume, volume);

        payload.type        = type;
        payload.prev_volume = prev_volume;
        payload.curr_volume = volume;
        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_OUT_CHANGE, &payload);
    }
    else
    {
        AUDIO_PRINT_ERROR5("audio_volume_out_set: failed %d, type %u, volume %u, max volume %u, min volume %u",
                           -ret, type, volume, max_volume, min_volume);
        return false;
    }
}

bool audio_volume_out_mute(T_AUDIO_STREAM_TYPE type)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_MUTE payload;
    int32_t                             ret = 0;

    payload.type = type;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        if (audio_volume_db->playback_volume_muted == false)
        {
            payload.volume = audio_volume_db->playback_volume;
            audio_volume_db->playback_volume_muted = true;
        }
        else
        {
            ret = 1;
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if (audio_volume_db->voice_out_volume_muted == false)
        {
            payload.volume = audio_volume_db->voice_out_volume;
            audio_volume_db->voice_out_volume_muted = true;
        }
        else
        {
            ret = 2;
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        {
            ret = 3;
        }
        break;
    }

    if (ret == 0)
    {
        AUDIO_PRINT_TRACE2("audio_volume_out_mute: type %u, muted volume %u",
                           payload.type, payload.volume);

        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_OUT_MUTE, &payload);
    }
    else
    {
        AUDIO_PRINT_ERROR2("audio_volume_out_mute: failed %d, type %u",
                           -ret, payload.type);
        return false;
    }
}

bool audio_volume_out_unmute(T_AUDIO_STREAM_TYPE type)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_UNMUTE payload;
    int32_t                               ret = 0;

    payload.type = type;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        if (audio_volume_db->playback_volume_muted == true)
        {
            payload.volume = audio_volume_db->playback_volume;
            audio_volume_db->playback_volume_muted = false;
        }
        else
        {
            ret = 1;
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if (audio_volume_db->voice_out_volume_muted == true)
        {
            payload.volume = audio_volume_db->voice_out_volume;
            audio_volume_db->voice_out_volume_muted = false;
        }
        else
        {
            ret = 2;
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        {
            ret = 3;
        }
        break;
    }

    if (ret == 0)
    {
        AUDIO_PRINT_TRACE2("audio_volume_out_unmute: type %u, unmuted volume %u",
                           payload.type, payload.volume);

        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_OUT_UNMUTE, &payload);
    }
    else
    {
        AUDIO_PRINT_ERROR2("audio_volume_out_unmute: failed %d, type %u",
                           -ret, payload.type);
        return false;
    }
}

bool audio_volume_out_is_muted(T_AUDIO_STREAM_TYPE type)
{
    bool is_muted;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        is_muted = audio_volume_db->playback_volume_muted;
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        is_muted = audio_volume_db->voice_out_volume_muted;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        is_muted = false;
        break;
    }

    return is_muted;
}

uint8_t audio_volume_in_max_get(T_AUDIO_STREAM_TYPE type)
{
    uint8_t max_volume = 0;

    if (audio_volume_db != NULL)
    {
        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            max_volume = audio_volume_db->voice_in_volume_max;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            max_volume = audio_volume_db->record_volume_max;
            break;
        }
    }

    return max_volume;
}

bool audio_volume_in_max_set(T_AUDIO_STREAM_TYPE type, uint8_t volume)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_IN_MAX_SET payload;

    if (audio_volume_db != NULL)
    {
        if (volume == 0)
        {
            return false;
        }

        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            audio_volume_db->voice_out_volume_max = volume;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            audio_volume_db->record_volume_max = volume;
            break;
        }

        payload.type    = type;
        payload.volume  = volume;

        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_IN_MAX_SET, &payload);
    }

    return false;
}

uint8_t audio_volume_in_min_get(T_AUDIO_STREAM_TYPE type)
{
    uint8_t min_volume = 0;

    if (audio_volume_db != NULL)
    {
        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            min_volume = audio_volume_db->voice_in_volume_min;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            min_volume = audio_volume_db->record_volume_min;
            break;
        }
    }

    return min_volume;
}

bool audio_volume_in_min_set(T_AUDIO_STREAM_TYPE type, uint8_t volume)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_IN_MIN_SET payload;

    if (audio_volume_db != NULL)
    {
        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            return false;

        case AUDIO_STREAM_TYPE_VOICE:
            audio_volume_db->voice_in_volume_min = volume;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            audio_volume_db->record_volume_min = volume;
            break;
        }

        payload.type    = type;
        payload.volume  = volume;

        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_IN_MIN_SET, &payload);
    }

    return false;
}

uint8_t audio_volume_in_get(T_AUDIO_STREAM_TYPE type)
{
    uint8_t curr_volume = 0;

    if (audio_volume_db != NULL)
    {
        switch (type)
        {
        case AUDIO_STREAM_TYPE_PLAYBACK:
            break;

        case AUDIO_STREAM_TYPE_VOICE:
            curr_volume = audio_volume_db->voice_in_volume;
            break;

        case AUDIO_STREAM_TYPE_RECORD:
            curr_volume = audio_volume_db->record_volume;
            break;
        }
    }

    return curr_volume;
}

bool audio_volume_in_set(T_AUDIO_STREAM_TYPE type, uint8_t volume)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_IN_CHANGE payload;
    uint8_t                              prev_volume;
    uint8_t                              max_volume;
    uint8_t                              min_volume;
    int32_t                              ret = 0;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        {
            /* No volume in for playback stream. */
            prev_volume = audio_volume_db->playback_volume;
            max_volume  = audio_volume_db->playback_volume_max;
            min_volume  = audio_volume_db->playback_volume_min;
            ret = 1;
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        {
            if ((volume <= audio_volume_db->voice_in_volume_max) &&
                (volume >= audio_volume_db->voice_in_volume_min))
            {
                prev_volume = audio_volume_db->voice_in_volume;
                audio_volume_db->voice_in_volume = volume;
            }
            else
            {
                max_volume = audio_volume_db->voice_in_volume_max;
                min_volume = audio_volume_db->voice_in_volume_min;
                ret = 2;
            }
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        {
            if ((volume <= audio_volume_db->record_volume_max) &&
                (volume >= audio_volume_db->record_volume_min))
            {
                prev_volume = audio_volume_db->record_volume;
                audio_volume_db->record_volume = volume;
            }
            else
            {
                max_volume = audio_volume_db->record_volume_max;
                min_volume = audio_volume_db->record_volume_min;
                ret = 3;
            }
        }
        break;
    }

    if (ret == 0)
    {
        AUDIO_PRINT_TRACE3("audio_volume_in_set: type %u, prev volume %u, curr volume %u",
                           type, prev_volume, volume);

        payload.type        = type;
        payload.prev_volume = prev_volume;
        payload.curr_volume = volume;
        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_IN_CHANGE, &payload);
    }
    else
    {
        AUDIO_PRINT_ERROR5("audio_volume_in_set: failed %d, type %u, volume %u, max volume %u, min volume %u",
                           -ret, type, volume, max_volume, min_volume);
        return false;
    }
}

bool audio_volume_in_mute(T_AUDIO_STREAM_TYPE type)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_IN_MUTE payload;
    int32_t                            ret = 0;

    payload.type = type;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        {
            ret = 1;
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if (audio_volume_db->voice_in_volume_muted == false)
        {
            payload.volume = audio_volume_db->voice_in_volume;
            audio_volume_db->voice_in_volume_muted = true;
        }
        else
        {
            ret = 2;
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        if (audio_volume_db->record_volume_muted == false)
        {
            payload.volume = audio_volume_db->record_volume;
            audio_volume_db->record_volume_muted = true;
        }
        else
        {
            ret = 3;
        }
        break;
    }

    if (ret == 0)
    {
        AUDIO_PRINT_TRACE2("audio_volume_in_mute: type %u, muted volume %u",
                           payload.type, payload.volume);

        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_IN_MUTED, &payload);
    }
    else
    {
        AUDIO_PRINT_ERROR2("audio_volume_in_mute: failed %d, type %u",
                           -ret, payload.type);
        return false;
    }
}

bool audio_volume_in_unmute(T_AUDIO_STREAM_TYPE type)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_IN_UNMUTE payload;
    int32_t                              ret = 0;

    payload.type = type;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        {
            ret = 1;
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if (audio_volume_db->voice_in_volume_muted == true)
        {
            payload.volume = audio_volume_db->voice_in_volume;
            audio_volume_db->voice_in_volume_muted = false;
        }
        else
        {
            ret = 2;
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        if (audio_volume_db->record_volume_muted == true)
        {
            payload.volume = audio_volume_db->record_volume;
            audio_volume_db->record_volume_muted = false;
        }
        else
        {
            ret = 3;
        }
        break;
    }

    if (ret == 0)
    {
        AUDIO_PRINT_TRACE2("audio_volume_in_unmute: type %u, unmuted volume %u",
                           payload.type, payload.volume);

        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_IN_UNMUTE, &payload);
    }
    else
    {
        AUDIO_PRINT_ERROR2("audio_volume_in_unmute: failed %d, type %u",
                           -ret, payload.type);
        return false;
    }
}

bool audio_volume_in_is_muted(T_AUDIO_STREAM_TYPE type)
{
    bool is_muted;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        is_muted = false;
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        is_muted = audio_volume_db->voice_in_volume_muted;
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        is_muted = audio_volume_db->record_volume_muted;
        break;
    }

    return is_muted;
}

bool audio_volume_out_channel_set(T_AUDIO_VOLUME_CHANNEL_MASK mask)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_OUT_CHANN_SET payload;

    payload.mask = mask;
    return audio_mgr_dispatch(AUDIO_MSG_VOLUME_OUT_CHANN_SET, &payload);
}

bool audio_volume_in_channel_set(T_AUDIO_VOLUME_CHANNEL_MASK mask)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_IN_CHANN_SET payload;

    payload.mask = mask;
    return audio_mgr_dispatch(AUDIO_MSG_VOLUME_IN_CHANN_SET, &payload);
}

float audio_volume_balance_get(T_AUDIO_STREAM_TYPE type)
{
    float scale;

    if (type == AUDIO_STREAM_TYPE_PLAYBACK)
    {
        scale = audio_volume_db->playback_volume_scale;
    }
    else if (type == AUDIO_STREAM_TYPE_VOICE)
    {
        scale = audio_volume_db->voice_out_volume_scale;
    }
    else
    {
        /* Apply volume balance only to volume out streams. */
        scale = 0.0f;
    }

    return scale;
}

bool audio_volume_balance_set(T_AUDIO_STREAM_TYPE type, float scale)
{
    T_AUDIO_MSG_PAYLOAD_VOLUME_BALANCE_CHANGE payload;
    float                                     prev_scale;
    int32_t                                   ret = 0;

    switch (type)
    {
    case AUDIO_STREAM_TYPE_PLAYBACK:
        if (scale >= -1.0f && scale <= 1.0f)
        {
            prev_scale = audio_volume_db->playback_volume_scale;
            audio_volume_db->playback_volume_scale = scale;
        }
        else
        {
            ret = 1;
        }
        break;

    case AUDIO_STREAM_TYPE_VOICE:
        if (scale >= -1.0f && scale <= 1.0f)
        {
            prev_scale = audio_volume_db->voice_out_volume_scale;
            audio_volume_db->voice_out_volume_scale = scale;
        }
        else
        {
            ret = 2;
        }
        break;

    case AUDIO_STREAM_TYPE_RECORD:
        {
            /* Apply volume balance only to volume out streams. */
            ret = 3;
        }
        break;
    }

    if (ret == 0)
    {
        AUDIO_PRINT_TRACE2("audio_volume_balance_set: type %u, scale %d/1000",
                           type, (int32_t)(scale * 1000));

        payload.type       = type;
        payload.prev_scale = prev_scale;
        payload.curr_scale = scale;
        return audio_mgr_dispatch(AUDIO_MSG_VOLUME_BALANCE_CHANGE, &payload);
    }
    else
    {
        AUDIO_PRINT_ERROR2("audio_volume_balance_set: failed %d, type %u",
                           -ret, type);
        return false;
    }
}
