/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "trace.h"
#include "notification.h"
#include "ringtone.h"

T_RINGTONE_MODE ringtone_mode_get(void)
{
    return (T_RINGTONE_MODE)notification_mode_get(NOTIFICATION_TYPE_RINGTONE);
}

bool ringtone_mode_set(T_RINGTONE_MODE mode)
{
    return notification_mode_set(NOTIFICATION_TYPE_RINGTONE, (uint8_t)mode);
}

uint8_t ringtone_volume_max_get(void)
{
    return notification_volume_max_get(NOTIFICATION_TYPE_RINGTONE);
}

bool ringtone_volume_max_set(uint8_t volume)
{
    return notification_volume_max_set(NOTIFICATION_TYPE_RINGTONE, volume);
}

uint8_t ringtone_volume_min_get(void)
{
    return notification_volume_min_get(NOTIFICATION_TYPE_RINGTONE);
}

bool ringtone_volume_min_set(uint8_t volume)
{
    return notification_volume_min_set(NOTIFICATION_TYPE_RINGTONE, volume);
}

uint8_t ringtone_volume_get(void)
{
    return notification_volume_get(NOTIFICATION_TYPE_RINGTONE);
}

bool ringtone_volume_set(uint8_t volume)
{
    T_RINGTONE_MODE mode;

    mode = (T_RINGTONE_MODE)notification_mode_get(NOTIFICATION_TYPE_RINGTONE);
    if (mode == RINGTONE_MODE_AUDIBLE)
    {
        return notification_volume_set(NOTIFICATION_TYPE_RINGTONE, volume);
    }

    return false;
}

bool ringtone_volume_mute(void)
{
    T_RINGTONE_MODE mode;

    mode = (T_RINGTONE_MODE)notification_mode_get(NOTIFICATION_TYPE_RINGTONE);
    if (mode == RINGTONE_MODE_AUDIBLE ||
        mode == RINGTONE_MODE_FIXED)
    {
        return notification_volume_mute(NOTIFICATION_TYPE_RINGTONE);
    }

    return false;
}

bool ringtone_volume_unmute(void)
{
    T_RINGTONE_MODE mode;

    mode = (T_RINGTONE_MODE)notification_mode_get(NOTIFICATION_TYPE_RINGTONE);
    if (mode == RINGTONE_MODE_AUDIBLE ||
        mode == RINGTONE_MODE_FIXED)
    {
        return notification_volume_unmute(NOTIFICATION_TYPE_RINGTONE);
    }

    return false;
}

float ringtone_volume_balance_get(void)
{
    return notification_volume_balance_get(NOTIFICATION_TYPE_RINGTONE);
}

bool ringtone_volume_balance_set(float scale)
{
    return notification_volume_balance_set(NOTIFICATION_TYPE_RINGTONE, scale);
}

bool ringtone_play(uint8_t index, bool relay)
{
    T_RINGTONE_MODE mode;

    mode = (T_RINGTONE_MODE)notification_mode_get(NOTIFICATION_TYPE_RINGTONE);

    AUDIO_PRINT_TRACE3("ringtone_play: mode %u, index 0x%02x, relay %d", mode, index, relay);

    if (mode == RINGTONE_MODE_SILENT)
    {
        return false;
    }

    if (index == NOTIFICATION_INVALID_INDEX)
    {
        return false;
    }

    return notification_push(NOTIFICATION_TYPE_RINGTONE, index, 0, NULL, relay, 0x0, 0x0);
}

bool ringtone_stop(void)
{
    return notification_stop(NOTIFICATION_TYPE_RINGTONE, NULL);
}

bool ringtone_cancel(uint8_t index, bool relay)
{
    return notification_cancel(NOTIFICATION_TYPE_RINGTONE, index, relay);
}

bool ringtone_flush(bool relay)
{
    return notification_flush(NOTIFICATION_TYPE_RINGTONE, relay);
}

uint8_t ringtone_remaining_count_get(void)
{
    return notification_remaining_count_get(NOTIFICATION_TYPE_RINGTONE);
}
