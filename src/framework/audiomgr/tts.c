/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdbool.h>
#include <stdint.h>
#include "trace.h"
#include "notification.h"
#include "tts.h"

uint8_t tts_volume_max_get(void)
{
    return notification_volume_max_get(NOTIFICATION_TYPE_TTS);
}

bool tts_volume_max_set(uint8_t volume)
{
    return notification_volume_max_set(NOTIFICATION_TYPE_TTS, volume);
}

uint8_t tts_volume_min_get(void)
{
    return notification_volume_min_get(NOTIFICATION_TYPE_TTS);
}

bool tts_volume_min_set(uint8_t volume)
{
    return notification_volume_min_set(NOTIFICATION_TYPE_TTS, volume);
}

uint8_t tts_volume_get(void)
{
    return notification_volume_get(NOTIFICATION_TYPE_TTS);
}

bool tts_volume_set(uint8_t volume)
{
    return notification_volume_set(NOTIFICATION_TYPE_TTS, volume);
}

bool tts_volume_mute(void)
{
    return notification_volume_mute(NOTIFICATION_TYPE_TTS);
}

bool tts_volume_unmute(void)
{
    return notification_volume_unmute(NOTIFICATION_TYPE_TTS);
}

float tts_volume_balance_get(void)
{
    return notification_volume_balance_get(NOTIFICATION_TYPE_TTS);
}

bool tts_volume_balance_set(float scale)
{
    return notification_volume_balance_set(NOTIFICATION_TYPE_TTS, scale);
}

T_TTS_HANDLE tts_create(uint32_t len, uint16_t frame_cnt, uint32_t cfg)
{
    T_TTS_HANDLE handle = NULL;

    AUDIO_PRINT_TRACE3("tts_create: len 0x%08x, frame_cnt 0x%04x, cfg 0x%08x",
                       len, frame_cnt, cfg);

    if ((len != 0) && (frame_cnt != 0))
    {
        handle = notification_tts_alloc(len, frame_cnt, cfg);
    }

    return handle;
}

bool tts_start(T_TTS_HANDLE handle, bool relay)
{
    AUDIO_PRINT_TRACE2("tts_start: handle %p, relay %u", handle, relay);

    if (handle != NULL)
    {
        return notification_push(NOTIFICATION_TYPE_TTS, 0, 0, handle, relay, 0x0, 0x0);
    }

    return false;
}

bool tts_play(T_TTS_HANDLE handle, void *buf, uint16_t len)
{
    AUDIO_PRINT_TRACE3("tts_play: handle %p, buf %p, len 0x%04x", handle, buf, len);

    if ((handle != NULL) && (buf != NULL) && (len != 0))
    {
        return notification_write(NOTIFICATION_TYPE_TTS, handle, buf, len);
    }

    return false;
}

bool tts_stop(T_TTS_HANDLE handle)
{
    AUDIO_PRINT_TRACE1("tts_stop: handle %p", handle);

    return notification_stop(NOTIFICATION_TYPE_TTS, handle);
}

bool tts_destroy(T_TTS_HANDLE handle)
{
    AUDIO_PRINT_TRACE1("tts_destroy: handle %p", handle);

    if (handle != NULL)
    {
        return notification_tts_free(handle);
    }

    return false;
}

uint8_t tts_remaining_count_get(void)
{
    return notification_remaining_count_get(NOTIFICATION_TYPE_TTS);
}
