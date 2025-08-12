/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdbool.h>
#include <stdint.h>

#include "os_mem.h"
#include "trace.h"
#include "audio_path.h"
#include "audio_effect.h"
#include "sidetone.h"

typedef struct t_sidetone_instance
{
    T_AUDIO_EFFECT_INSTANCE base;
    int16_t                 gain;
    T_SIDETONE_TYPE         type;
    uint8_t                 level;
    bool                    enabled;
} T_SIDETONE_INSTANCE;

#define SIDETONE_VOLUME_OUT_LEVEL_MIN        (0)
#define SIDETONE_VOLUME_OUT_LEVEL_MAX        (15)
#define SIDETONE_VOLUME_OUT_LEVEL_DEFAULT    (10)

#define SIDETONE_VOLUME_IN_LEVEL_MIN         (0)
#define SIDETONE_VOLUME_IN_LEVEL_MAX         (15)
#define SIDETONE_VOLUME_IN_LEVEL_DEFAULT     (10)

#define SIDETONE_GAIN_MAX                    (12)
#define SIDETONE_GAIN_MIN                    (-47.625)

typedef enum t_sidetone_action
{
    SIDETONE_ACTION_NONE     = 0x00,
    SIDETONE_ACTION_CREATE   = 0x01,
    SIDETONE_ACTION_START    = 0x02,
    SIDETONE_ACTION_STOP     = 0x03,
    SIDETONE_ACTION_RELEASE  = 0x04,
} T_SIDETONE_ACTION;

typedef enum t_sidetone_state
{
    SIDETONE_STATE_RELEASED  = 0x00,
    SIDETONE_STATE_CREATING  = 0x01,
    SIDETONE_STATE_CREATED   = 0x02,
    SIDETONE_STATE_STARTING  = 0x03,
    SIDETONE_STATE_STARTED   = 0x04,
    SIDETONE_STATE_STOPPING  = 0x05,
    SIDETONE_STATE_STOPPED   = 0x06,
    SIDETONE_STATE_RELEASING = 0x07,
} T_SIDETONE_STATE;

typedef struct t_sidetone_db
{
    T_AUDIO_PATH_HANDLE             handle;
    T_SIDETONE_STATE                state;
    T_SIDETONE_ACTION               action;
} T_SIDETONE_DB;

static T_SIDETONE_DB *sidetone_db = NULL;
static bool sidetone_path_cback(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_EVENT event,
                                uint32_t param);

static bool sidetone_effect_enable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_SIDETONE_INSTANCE *sidetone;
    int32_t              ret = 0;

    sidetone = audio_effect_inheritor_get(instance);
    if (sidetone == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (sidetone->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    AUDIO_PRINT_TRACE3("sidetone_effect_enable: type %d, gain %d, hpf_level %d",
                       sidetone->type, sidetone->gain, sidetone->level);

    switch (sidetone->type)
    {
    case SIDETONE_TYPE_SW:
        if (audio_path_sw_sidetone_enable(sidetone->gain, sidetone->level) == false)
        {
            ret = 3;
            goto fail_set_dsp_sidetone;
        }
        break;

    case SIDETONE_TYPE_HW:
        if (audio_path_hw_sidetone_enable(sidetone->gain, sidetone->level) == false)
        {
            ret = 4;
            goto fail_set_codec_sidetone;
        }
        break;

    default:
        {
            ret = 5;
            goto fail_get_sidetone_type;
        }
    }

    return true;

fail_get_sidetone_type:
fail_set_codec_sidetone:
fail_set_dsp_sidetone:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("sidetone_effect_enable: failed %d", -ret);
    return false;
}

static bool sidetone_effect_disable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_SIDETONE_INSTANCE *sidetone;
    int32_t              ret = 0;

    sidetone = audio_effect_inheritor_get(instance);
    if (sidetone == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (sidetone->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    AUDIO_PRINT_TRACE3("sidetone_effect_disable: type %d, gain %d, hpf_level %d",
                       sidetone->type, sidetone->gain, sidetone->level);

    switch (sidetone->type)
    {
    case SIDETONE_TYPE_SW:
        if (audio_path_sw_sidetone_disable() == false)
        {
            ret = 3;
            goto fail_set_dsp_sidetone;
        }
        break;

    case SIDETONE_TYPE_HW:
        if (audio_path_hw_sidetone_disable() == false)
        {
            ret = 4;
            goto fail_set_codec_sidetone;
        }
        break;

    default:
        {
            ret = 5;
            goto fail_get_sidetone_type;
        }
    }

    return true;

fail_get_sidetone_type:
fail_set_codec_sidetone:
fail_set_dsp_sidetone:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("sidetone_effect_disable: failed %d", -ret);
    return false;
}

T_AUDIO_EFFECT_INSTANCE sidetone_create(T_SIDETONE_TYPE type,
                                        int16_t         gain,
                                        uint8_t         level)
{
    T_SIDETONE_INSTANCE *sidetone;
    int32_t              ret = 0;

    AUDIO_PRINT_TRACE2("sidetone_create: gain %d, hpf_level %u",
                       gain, level);

    sidetone = os_mem_alloc2(sizeof(T_SIDETONE_INSTANCE));
    if (sidetone == NULL)
    {
        ret = 1;
        goto fail_alloc_sidetone;
    }

    sidetone->base = audio_effect_create(sidetone_effect_enable, sidetone_effect_disable);
    if (sidetone->base == NULL)
    {
        ret = 2;
        goto fail_create_effect;
    }

    if (audio_effect_inheritor_set(sidetone->base, sidetone) == false)
    {
        ret = 3;
        goto fail_set_inheritor;
    }

    sidetone->enabled    = false;
    sidetone->type       = type;
    sidetone->gain       = gain;
    sidetone->level      = level;

    return sidetone->base;

fail_set_inheritor:
    audio_effect_release(sidetone->base);
fail_create_effect:
    os_mem_free(sidetone);
fail_alloc_sidetone:
    AUDIO_PRINT_ERROR1("sidetone_create: failed %d", -ret);
    return NULL;
}

bool sidetone_enable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_SIDETONE_INSTANCE *sidetone;
    int32_t              ret = 0;

    AUDIO_PRINT_TRACE1("sidetone_enable: instance %p", instance);

    sidetone = audio_effect_inheritor_get(instance);
    if (sidetone == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (sidetone->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (sidetone->enabled == true)
    {
        ret = 3;
        goto fail_check_state;
    }

    if (audio_effect_enable(sidetone->base) == false)
    {
        ret = 4;
        goto fail_enable_effect;
    }

    sidetone->enabled = true;
    return true;

fail_enable_effect:
fail_check_state:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("sidetone_enable: failed %d", -ret);
    return false;
}

bool sidetone_disable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_SIDETONE_INSTANCE *sidetone;
    int32_t              ret = 0;

    AUDIO_PRINT_TRACE1("sidetone_disable: instance %p", instance);

    sidetone = audio_effect_inheritor_get(instance);
    if (sidetone == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (sidetone->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (sidetone->enabled == false)
    {
        ret = 3;
        goto fail_check_state;
    }

    if (audio_effect_disable(sidetone->base) == false)
    {
        ret = 4;
        goto fail_disable_effect;
    }

    sidetone->enabled = false;
    return true;

fail_disable_effect:
fail_check_state:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("sidetone_disable: failed %d", -ret);
    return false;
}

bool sidetone_release(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_SIDETONE_INSTANCE *sidetone;
    int32_t              ret = 0;

    AUDIO_PRINT_TRACE1("sidetone_release: instance %p", instance);

    sidetone = audio_effect_inheritor_get(instance);
    if (sidetone == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (sidetone->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (audio_effect_release(sidetone->base) == false)
    {
        ret = 3;
        goto fail_release_effect;
    }

    os_mem_free(sidetone);
    return true;

fail_release_effect:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("sidetone_release: failed %d", -ret);
    return false;
}

bool sidetone_gain_set(T_AUDIO_EFFECT_INSTANCE instance, int16_t gain)
{
    T_SIDETONE_INSTANCE *sidetone;
    T_AUDIO_PATH_HANDLE handle;
    int32_t ret = 0;

    AUDIO_PRINT_TRACE2("sidetone_gain_set: instance %p, gain %d", instance, gain);

    sidetone = audio_effect_inheritor_get(instance);
    if (sidetone == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (sidetone->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    handle = audio_effect_owner_get(instance);
    if (handle == NULL)
    {
        ret = 3;
        goto fail_get_owner;
    }

    sidetone->gain = gain;

    if (sidetone->enabled == true)
    {
        if (audio_effect_update(sidetone->base) == false)
        {
            ret = 4;
            goto fail_update_effect;
        }
    }

    return true;

fail_update_effect:
fail_get_owner:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("sidetone_gain_set: failed %d", -ret);
    return false;
}

static bool sidetone_state_set(T_SIDETONE_STATE state)
{
    bool ret = false;

    AUDIO_PRINT_TRACE3("sidetone_state_set: curr state %u, next state %u, action %u",
                       sidetone_db->state, state, sidetone_db->action);

    switch (sidetone_db->state)
    {
    case SIDETONE_STATE_RELEASED:
        if (state == SIDETONE_STATE_CREATING)
        {
            sidetone_db->state  = state;
            sidetone_db->handle = audio_path_create(AUDIO_CATEGORY_SIDETONE,
                                                    AUDIO_DEVICE_OUT_DEFAULT,
                                                    NULL,
                                                    AUDIO_STREAM_MODE_NORMAL,
                                                    SIDETONE_VOLUME_OUT_LEVEL_DEFAULT,
                                                    SIDETONE_VOLUME_IN_LEVEL_DEFAULT,
                                                    sidetone_path_cback);
            if (sidetone_db->handle == NULL)
            {
                sidetone_db->state = SIDETONE_STATE_RELEASED;
            }
            else
            {
                ret = true;
            }
        }
        break;

    case SIDETONE_STATE_CREATING:
        if (state == SIDETONE_STATE_CREATED)
        {
            sidetone_db->state = state;

            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_START)
            {
                sidetone_db->action = SIDETONE_ACTION_NONE;
                ret = sidetone_state_set(SIDETONE_STATE_STARTING);
            }
            else if (sidetone_db->action == SIDETONE_ACTION_RELEASE)
            {
                sidetone_db->action = SIDETONE_ACTION_NONE;
                ret = sidetone_state_set(SIDETONE_STATE_RELEASING);
            }
        }
        else if (state == SIDETONE_STATE_STARTING)
        {
            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                /* Start the path immediately before the path created event
                 * received, so this action should be pending.
                 */
                sidetone_db->action = SIDETONE_ACTION_START;
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_START)
            {
                /* Drop the repeating path start action. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        else if (state == SIDETONE_STATE_STOPPING)
        {
            if (sidetone_db->action == SIDETONE_ACTION_START)
            {
                /* Cancel all pending actions when the creating path is pending
                 * for start action, but receives the stop action later.
                 */
                sidetone_db->action = SIDETONE_ACTION_NONE;
                ret = true;
            }
        }
        else if (state == SIDETONE_STATE_RELEASING)
        {
            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                sidetone_db->action = SIDETONE_ACTION_RELEASE;
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_START)
            {
                /* Override the pending start action for the creating path. */
                sidetone_db->action = SIDETONE_ACTION_RELEASE;
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case SIDETONE_STATE_CREATED:
        if (state == SIDETONE_STATE_STARTING)
        {
            sidetone_db->state = state;

            ret = audio_path_start(sidetone_db->handle);
            if (ret == false)
            {
                sidetone_db->state = SIDETONE_STATE_CREATED;
            }
        }
        else if (state == SIDETONE_STATE_RELEASING)
        {
            sidetone_db->state = state;

            ret = audio_path_destory(sidetone_db->handle);
            if (ret == false)
            {
                sidetone_db->state = SIDETONE_STATE_CREATED;
            }
        }
        break;

    case SIDETONE_STATE_STARTING:
        if (state == SIDETONE_STATE_STARTING)
        {
            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                /* Drop the repeating path start action. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_STOP)
            {
                /* Cancel all pending actions when the starting path is pending
                 * for stop action, but receives the start action later.
                 */
                sidetone_db->action = SIDETONE_ACTION_NONE;
                ret = true;
            }
        }
        else if (state == SIDETONE_STATE_STARTED)
        {
            sidetone_db->state = state;

            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_STOP)
            {
                sidetone_db->action = SIDETONE_ACTION_NONE;
                ret = sidetone_state_set(SIDETONE_STATE_STOPPING);
            }
            else if (sidetone_db->action == SIDETONE_ACTION_RELEASE)
            {
                sidetone_db->action = SIDETONE_ACTION_NONE;
                ret = sidetone_state_set(SIDETONE_STATE_RELEASING);
            }
        }
        else if (state == SIDETONE_STATE_STOPPING)
        {
            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                sidetone_db->action = SIDETONE_ACTION_STOP;
                ret = true;
            }
        }
        else if (state == SIDETONE_STATE_RELEASING)
        {
            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                sidetone_db->action = SIDETONE_ACTION_RELEASE;
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_STOP)
            {
                /* Override the pending stop action for the starting path. */
                sidetone_db->action = SIDETONE_ACTION_RELEASE;
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case is permitted. */
                ret = true;
            }
        }
        break;

    case SIDETONE_STATE_STARTED:
        if (state == SIDETONE_STATE_STOPPING)
        {
            sidetone_db->state = state;

            ret = audio_path_stop(sidetone_db->handle);
            if (ret == false)
            {
                sidetone_db->state = SIDETONE_STATE_STARTED;
            }
        }
        else if (state == SIDETONE_STATE_RELEASING)
        {
            /* Started path cannot be released directly, so it should be
             * stopped first before released.
             */
            sidetone_db->state  = SIDETONE_STATE_STOPPING;
            sidetone_db->action = SIDETONE_ACTION_RELEASE;

            ret = audio_path_stop(sidetone_db->handle);
            if (ret == false)
            {
                sidetone_db->state  = SIDETONE_STATE_STARTED;
                sidetone_db->action = SIDETONE_ACTION_NONE;
            }
        }
        break;

    case SIDETONE_STATE_STOPPING:
        if (state == SIDETONE_STATE_STARTING)
        {
            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                sidetone_db->action = SIDETONE_ACTION_START;
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_START)
            {
                /* Drop the repeating start action. But this case is permitted. */
                ret = true;
            }
        }
        else if (state == SIDETONE_STATE_STOPPING)
        {
            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                /* Drop the repeating stop action. But this case is permitted. */
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_START)
            {
                /* Cancel all pending actions when the stopping path is pending
                 * for start action, but receives the stop action later.
                 */
                sidetone_db->action = SIDETONE_ACTION_NONE;
                ret = true;
            }
        }
        else if (state == SIDETONE_STATE_STOPPED)
        {
            sidetone_db->state = state;

            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_START)
            {
                sidetone_db->action = SIDETONE_ACTION_NONE;
                ret = sidetone_state_set(SIDETONE_STATE_STARTING);
            }
            else if (sidetone_db->action == SIDETONE_ACTION_RELEASE)
            {
                sidetone_db->action = SIDETONE_ACTION_NONE;
                ret = sidetone_state_set(SIDETONE_STATE_RELEASING);
            }
        }
        else if (state == SIDETONE_STATE_RELEASING)
        {
            if (sidetone_db->action == SIDETONE_ACTION_NONE)
            {
                sidetone_db->action = SIDETONE_ACTION_RELEASE;
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_START)
            {
                /* Override the pending start action for the stopping path. */
                sidetone_db->action = SIDETONE_ACTION_RELEASE;
                ret = true;
            }
            else if (sidetone_db->action == SIDETONE_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case is permitted. */
                ret = true;
            }
        }
        break;

    case SIDETONE_STATE_STOPPED:
        if (state == SIDETONE_STATE_STARTING)
        {
            sidetone_db->state = state;

            ret = audio_path_start(sidetone_db->handle);
            if (ret == false)
            {
                sidetone_db->state = SIDETONE_STATE_STOPPED;
            }
        }
        else if (state == SIDETONE_STATE_RELEASING)
        {
            sidetone_db->state = state;

            ret = audio_path_destory(sidetone_db->handle);
            if (ret == false)
            {
                sidetone_db->state = SIDETONE_STATE_STOPPED;
            }
        }
        break;

    case SIDETONE_STATE_RELEASING:
        if (state == SIDETONE_STATE_RELEASED)
        {
            sidetone_db->state  = state;
            sidetone_db->action = SIDETONE_ACTION_NONE;
            sidetone_db->handle = NULL;
            ret = true;
        }
        else if (state == SIDETONE_STATE_RELEASING)
        {
            /* Drop the repeating release action. But this case is permitted. */
            ret = true;
        }
        break;
    }

    return ret;
}

static bool sidetone_path_cback(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_EVENT event,
                                uint32_t param)
{
    bool ret = false;

    AUDIO_PRINT_TRACE3("sidetone_path_cback: handle %p, event 0x%02x, param 0x%08x",
                       handle, event, param);

    switch (event)
    {
    case AUDIO_PATH_EVT_RELEASE:
        {
            ret = sidetone_state_set(SIDETONE_STATE_RELEASED);
        }
        break;

    case AUDIO_PATH_EVT_CREATE:
        {
            ret = sidetone_state_set(SIDETONE_STATE_CREATED);
        }
        break;

    case AUDIO_PATH_EVT_RUNNING:
        {
            ret = sidetone_state_set(SIDETONE_STATE_STARTED);
        }
        break;

    case AUDIO_PATH_EVT_IDLE:
        {
            ret = sidetone_state_set(SIDETONE_STATE_STOPPED);
        }
        break;

    default:
        break;
    }

    return ret;
}

bool sidetone_init(void)
{
    int32_t ret = 0;

    sidetone_db = os_mem_alloc2(sizeof(T_SIDETONE_DB));
    if (sidetone_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    sidetone_db->state                   = SIDETONE_STATE_RELEASED;
    sidetone_db->action                  = SIDETONE_ACTION_NONE;

    if (sidetone_state_set(SIDETONE_STATE_CREATING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_alloc_db:
    AUDIO_PRINT_ERROR1("sidetone_init: failed %d", -ret);
    return false;
}

void sidetone_deinit(void)
{
    if (sidetone_db != NULL)
    {
        if (sidetone_db->handle != NULL)
        {
            sidetone_state_set(SIDETONE_STATE_RELEASING);
        }

        os_mem_free(sidetone_db);
        sidetone_db = NULL;
    }
}

bool sidetone_path_enable(void)
{
    if (sidetone_db != NULL)
    {
        return sidetone_state_set(SIDETONE_STATE_STARTING);
    }

    return false;
}

bool sidetone_path_disable(void)
{
    if (sidetone_db != NULL)
    {
        return sidetone_state_set(SIDETONE_STATE_STOPPING);
    }

    return false;
}

void sidetone_path_gain_set(double gain)
{
    if ((gain > SIDETONE_GAIN_MAX) || (gain < SIDETONE_GAIN_MIN))
    {
        AUDIO_PRINT_ERROR0("sidetone_path_gain_set: failed");
    }
    else if (sidetone_db != NULL)
    {
        audio_path_sidetone_gain_set(sidetone_db->handle, (int16_t)(gain * 128));
    }
}

