/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdbool.h>
#include <stdint.h>

#include "os_mem.h"
#include "trace.h"
#include "audio_path.h"
#include "audio_mgr.h"
#include "anc.h"

/* TODO: Remove */
#include "anc_mgr.h"
/* TODO Remove End */

#define ANC_VOLUME_OUT_LEVEL_MIN        (0)
#define ANC_VOLUME_OUT_LEVEL_MAX        (15)
#define ANC_VOLUME_OUT_LEVEL_DEFAULT    (10)

#define ANC_VOLUME_IN_LEVEL_MIN         (0)
#define ANC_VOLUME_IN_LEVEL_MAX         (15)
#define ANC_VOLUME_IN_LEVEL_DEFAULT     (10)

typedef enum t_anc_action
{
    ANC_ACTION_NONE     = 0x00,
    ANC_ACTION_CREATE   = 0x01,
    ANC_ACTION_START    = 0x02,
    ANC_ACTION_STOP     = 0x03,
    ANC_ACTION_RELEASE  = 0x04,
} T_ANC_ACTION;

typedef enum t_anc_state
{
    ANC_STATE_RELEASED  = 0x00,
    ANC_STATE_CREATING  = 0x01,
    ANC_STATE_CREATED   = 0x02,
    ANC_STATE_STARTING  = 0x03,
    ANC_STATE_STARTED   = 0x04,
    ANC_STATE_STOPPING  = 0x05,
    ANC_STATE_STOPPED   = 0x06,
    ANC_STATE_RELEASING = 0x07,
} T_ANC_STATE;

typedef enum t_anc_adaptive_filter_action
{
    ANC_ADAPTIVE_FILTER_ACTION_NONE     = 0x00,
    ANC_ADAPTIVE_FILTER_ACTION_ENABLE   = 0x01,
    ANC_ADAPTIVE_FILTER_ACTION_PAUSE    = 0x02,
    ANC_ADAPTIVE_FILTER_ACTION_DISABLE  = 0x03,
} T_ANC_ADAPTIVE_FILTER_ACTION;

typedef struct t_anc_db
{
    T_AUDIO_PATH_HANDLE             handle;
    T_ANC_STATE                     state;
    T_ANC_ACTION                    action;
    uint8_t                         scenario_id;
    uint8_t                         volume_out;
    uint8_t                         volume_in;
    T_ANC_ADAPTIVE_FILTER_ACTION    adaptive_filter_action;
} T_ANC_DB;

#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)

static T_ANC_DB *anc_db = NULL;
static bool anc_path_cback(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_EVENT event, uint32_t param);

static bool anc_state_set(T_ANC_STATE state)
{
    bool ret = false;

    AUDIO_PRINT_TRACE3("anc_state_set: curr state %u, next state %u, action %u",
                       anc_db->state, state, anc_db->action);

    switch (anc_db->state)
    {
    case ANC_STATE_RELEASED:
        if (state == ANC_STATE_CREATING)
        {
            anc_db->state  = state;
            anc_db->handle = audio_path_create(AUDIO_CATEGORY_ANC,
                                               AUDIO_DEVICE_OUT_DEFAULT,
                                               NULL,
                                               AUDIO_STREAM_MODE_NORMAL,
                                               anc_db->volume_out,
                                               anc_db->volume_in,
                                               anc_path_cback);
            if (anc_db->handle == NULL)
            {
                anc_db->state = ANC_STATE_RELEASED;
            }
            else
            {
                ret = true;
            }
        }
        break;

    case ANC_STATE_CREATING:
        if (state == ANC_STATE_CREATED)
        {
            anc_db->state = state;

            if (anc_db->action == ANC_ACTION_NONE)
            {
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_START)
            {
                anc_db->action = ANC_ACTION_NONE;
                ret = anc_state_set(ANC_STATE_STARTING);
            }
            else if (anc_db->action == ANC_ACTION_RELEASE)
            {
                anc_db->action = ANC_ACTION_NONE;
                ret = anc_state_set(ANC_STATE_RELEASING);
            }
        }
        else if (state == ANC_STATE_STARTING)
        {
            if (anc_db->action == ANC_ACTION_NONE)
            {
                /* Start the path immediately before the path created event
                 * received, so this action should be pending.
                 */
                anc_db->action = ANC_ACTION_START;
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_START)
            {
                /* Drop the repeating path start action. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        else if (state == ANC_STATE_STOPPING)
        {
            if (anc_db->action == ANC_ACTION_START)
            {
                /* Cancel all pending actions when the creating path is pending
                 * for start action, but receives the stop action later.
                 */
                anc_db->action = ANC_ACTION_NONE;
                ret = true;
            }
        }
        else if (state == ANC_STATE_RELEASING)
        {
            if (anc_db->action == ANC_ACTION_NONE)
            {
                anc_db->action = ANC_ACTION_RELEASE;
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_START)
            {
                /* Override the pending start action for the creating path. */
                anc_db->action = ANC_ACTION_RELEASE;
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case ANC_STATE_CREATED:
        if (state == ANC_STATE_STARTING)
        {
            T_ANC_LLAPT_CFG config;

            anc_db->state = state;

            config.sub_type = ANC_IMAGE_SUB_TYPE_ANC_COEF;
            config.scenario_id = anc_db->scenario_id;
            audio_path_cfg_set(anc_db->handle, &config);

            ret = audio_path_start(anc_db->handle);
            if (ret == false)
            {
                anc_db->state = ANC_STATE_CREATED;
            }
        }
        else if (state == ANC_STATE_RELEASING)
        {
            anc_db->state = state;

            ret = audio_path_destory(anc_db->handle);
            if (ret == false)
            {
                anc_db->state = ANC_STATE_CREATED;
            }
        }
        break;

    case ANC_STATE_STARTING:
        if (state == ANC_STATE_STARTING)
        {
            if (anc_db->action == ANC_ACTION_NONE)
            {
                /* Drop the repeating path start action. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_STOP)
            {
                /* Cancel all pending actions when the starting path is pending
                 * for stop action, but receives the start action later.
                 */
                anc_db->action = ANC_ACTION_NONE;
                ret = true;
            }
        }
        else if (state == ANC_STATE_STARTED)
        {
            anc_db->state = state;

            if (anc_db->adaptive_filter_action == ANC_ADAPTIVE_FILTER_ACTION_ENABLE)
            {
                anc_mgr_adaptive_filter_start();
            }
            else if (anc_db->adaptive_filter_action == ANC_ADAPTIVE_FILTER_ACTION_PAUSE)
            {
                anc_mgr_adaptive_filter_pause();
            }
            else if (anc_db->adaptive_filter_action == ANC_ADAPTIVE_FILTER_ACTION_DISABLE)
            {
                anc_mgr_adaptive_filter_stop();
            }

            anc_db->adaptive_filter_action = ANC_ADAPTIVE_FILTER_ACTION_NONE;

            audio_mgr_dispatch(AUDIO_MSG_ANC_ENABLED, NULL);

            if (anc_db->action == ANC_ACTION_NONE)
            {
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_STOP)
            {
                anc_db->action = ANC_ACTION_NONE;
                ret = anc_state_set(ANC_STATE_STOPPING);
            }
            else if (anc_db->action == ANC_ACTION_RELEASE)
            {
                anc_db->action = ANC_ACTION_NONE;
                ret = anc_state_set(ANC_STATE_RELEASING);
            }
        }
        else if (state == ANC_STATE_STOPPING)
        {
            if (anc_db->action == ANC_ACTION_NONE)
            {
                anc_db->action = ANC_ACTION_STOP;
                ret = true;
            }
        }
        else if (state == ANC_STATE_RELEASING)
        {
            if (anc_db->action == ANC_ACTION_NONE)
            {
                anc_db->action = ANC_ACTION_RELEASE;
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_STOP)
            {
                /* Override the pending stop action for the starting path. */
                anc_db->action = ANC_ACTION_RELEASE;
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case is permitted. */
                ret = true;
            }
        }
        break;

    case ANC_STATE_STARTED:
        if (state == ANC_STATE_STOPPING)
        {
            anc_db->state = state;

            ret = audio_path_stop(anc_db->handle);
            if (ret == false)
            {
                anc_db->state = ANC_STATE_STARTED;
            }
        }
        else if (state == ANC_STATE_RELEASING)
        {
            /* Started path cannot be released directly, so it should be
             * stopped first before released.
             */
            anc_db->state  = ANC_STATE_STOPPING;
            anc_db->action = ANC_ACTION_RELEASE;

            ret = audio_path_stop(anc_db->handle);
            if (ret == false)
            {
                anc_db->state  = ANC_STATE_STARTED;
                anc_db->action = ANC_ACTION_NONE;
            }
        }
        break;

    case ANC_STATE_STOPPING:
        if (state == ANC_STATE_STARTING)
        {
            if (anc_db->action == ANC_ACTION_NONE)
            {
                anc_db->action = ANC_ACTION_START;
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_START)
            {
                /* Drop the repeating start action. But this case is permitted. */
                ret = true;
            }
        }
        else if (state == ANC_STATE_STOPPING)
        {
            if (anc_db->action == ANC_ACTION_NONE)
            {
                /* Drop the repeating stop action. But this case is permitted. */
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_START)
            {
                /* Cancel all pending actions when the stopping path is pending
                 * for start action, but receives the stop action later.
                 */
                anc_db->action = ANC_ACTION_NONE;
                ret = true;
            }
        }
        else if (state == ANC_STATE_STOPPED)
        {
            anc_db->state = state;
            anc_db->adaptive_filter_action = ANC_ADAPTIVE_FILTER_ACTION_NONE;

            audio_mgr_dispatch(AUDIO_MSG_ANC_DISABLED, NULL);

            if (anc_db->action == ANC_ACTION_NONE)
            {
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_START)
            {
                anc_db->action = ANC_ACTION_NONE;
                ret = anc_state_set(ANC_STATE_STARTING);
            }
            else if (anc_db->action == ANC_ACTION_RELEASE)
            {
                anc_db->action = ANC_ACTION_NONE;
                ret = anc_state_set(ANC_STATE_RELEASING);
            }
        }
        else if (state == ANC_STATE_RELEASING)
        {
            if (anc_db->action == ANC_ACTION_NONE)
            {
                anc_db->action = ANC_ACTION_RELEASE;
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_START)
            {
                /* Override the pending start action for the stopping path. */
                anc_db->action = ANC_ACTION_RELEASE;
                ret = true;
            }
            else if (anc_db->action == ANC_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case is permitted. */
                ret = true;
            }
        }
        break;

    case ANC_STATE_STOPPED:
        if (state == ANC_STATE_STARTING)
        {
            T_ANC_LLAPT_CFG config;

            anc_db->state = state;

            config.sub_type = ANC_IMAGE_SUB_TYPE_ANC_COEF;
            config.scenario_id = anc_db->scenario_id;
            audio_path_cfg_set(anc_db->handle, &config);

            ret = audio_path_start(anc_db->handle);
            if (ret == false)
            {
                anc_db->state = ANC_STATE_STOPPED;
            }
        }
        else if (state == ANC_STATE_RELEASING)
        {
            anc_db->state = state;

            ret = audio_path_destory(anc_db->handle);
            if (ret == false)
            {
                anc_db->state = ANC_STATE_STOPPED;
            }
        }
        break;

    case ANC_STATE_RELEASING:
        if (state == ANC_STATE_RELEASED)
        {
            anc_db->state  = state;
            anc_db->action = ANC_ACTION_NONE;
            anc_db->handle = NULL;
            ret = true;
        }
        else if (state == ANC_STATE_RELEASING)
        {
            /* Drop the repeating release action. But this case is permitted. */
            ret = true;
        }
        break;
    }

    return ret;
}

static bool anc_path_cback(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_EVENT event, uint32_t param)
{
    bool ret = false;

    AUDIO_PRINT_TRACE3("anc_path_cback: handle %p, event 0x%02x, param 0x%08x",
                       handle, event, param);

    switch (event)
    {
    case AUDIO_PATH_EVT_RELEASE:
        {
            ret = anc_state_set(ANC_STATE_RELEASED);
        }
        break;

    case AUDIO_PATH_EVT_CREATE:
        {
            ret = anc_state_set(ANC_STATE_CREATED);
        }
        break;

    case AUDIO_PATH_EVT_RUNNING:
        {
            ret = anc_state_set(ANC_STATE_STARTED);
        }
        break;

    case AUDIO_PATH_EVT_IDLE:
        {
            ret = anc_state_set(ANC_STATE_STOPPED);
        }
        break;

    default:
        break;
    }

    return ret;
}

bool anc_init(void)
{
    int32_t ret = 0;

    anc_db = os_mem_alloc2(sizeof(T_ANC_DB));
    if (anc_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    anc_db->state                   = ANC_STATE_RELEASED;
    anc_db->action                  = ANC_ACTION_NONE;
    anc_db->scenario_id             = 0;
    anc_db->volume_out              = ANC_VOLUME_OUT_LEVEL_DEFAULT;
    anc_db->volume_in               = ANC_VOLUME_IN_LEVEL_DEFAULT;
    anc_db->adaptive_filter_action  = ANC_ADAPTIVE_FILTER_ACTION_NONE;

    if (anc_state_set(ANC_STATE_CREATING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_alloc_db:
    AUDIO_PRINT_ERROR1("anc_init: failed %d", -ret);
    return false;
}

void anc_deinit(void)
{
    if (anc_db != NULL)
    {
        if (anc_db->handle != NULL)
        {
            anc_state_set(ANC_STATE_RELEASING);
        }

        os_mem_free(anc_db);
        anc_db = NULL;
    }
}

bool anc_enable(uint8_t scenario_id)
{
    if (anc_db != NULL)
    {
        anc_db->scenario_id = scenario_id;
        return anc_state_set(ANC_STATE_STARTING);
    }

    return false;
}

bool anc_disable(void)
{
    if (anc_db != NULL)
    {
        return anc_state_set(ANC_STATE_STOPPING);
    }

    return false;
}

bool anc_adaptive_filter_start(void)
{
    if (anc_db != NULL)
    {
        if (anc_db->state == ANC_STATE_STARTED)
        {
            anc_db->adaptive_filter_action = ANC_ADAPTIVE_FILTER_ACTION_NONE;
            anc_mgr_adaptive_filter_start();
        }
        else
        {
            anc_db->adaptive_filter_action = ANC_ADAPTIVE_FILTER_ACTION_ENABLE;
        }

        return true;
    }

    return false;
}

bool anc_adaptive_filter_pause(void)
{
    if (anc_db != NULL)
    {
        if (anc_db->state == ANC_STATE_STARTED)
        {
            anc_db->adaptive_filter_action = ANC_ADAPTIVE_FILTER_ACTION_NONE;
            anc_mgr_adaptive_filter_pause();
        }
        else
        {
            anc_db->adaptive_filter_action = ANC_ADAPTIVE_FILTER_ACTION_PAUSE;
        }

        return true;
    }

    return false;
}

bool anc_adaptive_filter_stop(void)
{
    if (anc_db != NULL)
    {
        if (anc_db->state == ANC_STATE_STARTED)
        {
            anc_db->adaptive_filter_action = ANC_ADAPTIVE_FILTER_ACTION_NONE;
            anc_mgr_adaptive_filter_stop();
        }
        else
        {
            anc_db->adaptive_filter_action = ANC_ADAPTIVE_FILTER_ACTION_DISABLE;
        }

        return true;
    }

    return false;
}
#else   /* CONFIG_REALTEK_AM_ANC_SUPPORT */
bool anc_init(void)
{
    return true;
}

void anc_deinit(void)
{
    return;
}

bool anc_enable(uint8_t scenario_id)
{
    return false;
}

bool anc_disable(void)
{
    return false;
}

bool anc_adaptive_filter_start(void)
{
    return false;
}

bool anc_adaptive_filter_pause(void)
{
    return false;
}

bool anc_adaptive_filter_stop(void)
{
    return false;
}

#endif  /* CONFIG_REALTEK_AM_ANC_SUPPORT */
