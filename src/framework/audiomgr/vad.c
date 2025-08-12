/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "trace.h"
#include "audio.h"
#include "audio_mgr.h"
#include "audio_path.h"
#include "vad.h"

/* TODO Remove Start */
#include "dsp_mgr.h"
/* TODO Remove End */


#define VAD_VOLUME_LEVEL_MIN        (0)
#define VAD_VOLUME_LEVEL_MAX        (15)
#define VAD_VOLUME_LEVEL_DEFAULT    (10)

typedef enum t_vad_action
{
    VAD_ACTION_NONE     = 0x00,
    VAD_ACTION_CREATE   = 0x01,
    VAD_ACTION_START    = 0x02,
    VAD_ACTION_STOP     = 0x03,
    VAD_ACTION_RELEASE  = 0x04,
} T_VAD_ACTION;

typedef enum t_vad_state
{
    VAD_STATE_RELEASED  = 0x00,
    VAD_STATE_CREATING  = 0x01,
    VAD_STATE_CREATED   = 0x02,
    VAD_STATE_STARTING  = 0x03,
    VAD_STATE_STARTED   = 0x04,
    VAD_STATE_STOPPING  = 0x05,
    VAD_STATE_STOPPED   = 0x06,
    VAD_STATE_RELEASING = 0x07,
} T_VAD_STATE;

typedef struct t_vad_db
{
    T_VAD_STATE         state;
    T_VAD_ACTION        action;
    T_AUDIO_PATH_HANDLE handle;
    uint8_t             volume_in;
} T_VAD_DB;

#if (AM_VAD_SUPPORT == 1)
static T_VAD_DB *vad_db = NULL;

static bool vad_path_cback(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_EVENT event, uint32_t param);

static bool vad_state_set(T_VAD_STATE state)
{
    bool ret = true;

    AUDIO_PRINT_TRACE3("vad_state_set: curr state %u, next state %u, action %u",
                       vad_db->state, state, vad_db->action);

    switch (vad_db->state)
    {
    case VAD_STATE_RELEASED:
        if (state == VAD_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATING)
        {
            vad_db->state  = state;
            vad_db->handle = audio_path_create(AUDIO_CATEGORY_VAD,
                                               AUDIO_DEVICE_IN_MIC,
                                               NULL,
                                               AUDIO_STREAM_MODE_NORMAL,
                                               0,
                                               VAD_VOLUME_LEVEL_DEFAULT,
                                               vad_path_cback);
            if (vad_db->handle == NULL)
            {
                vad_db->state = VAD_STATE_RELEASED;
                ret = false;
            }
        }
        else if (state == VAD_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTING)
        {
            /* Starting released path is forbidden. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPING)
        {
            /* Stopping released path is forbidden. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_RELEASING)
        {
            /* Releasing released path is forbidden. */
            ret = false;
        }
        break;

    case VAD_STATE_CREATING:
        if (state == VAD_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATED)
        {
            vad_db->state = state;

            if (vad_db->action == VAD_ACTION_NONE)
            {
                ret = true;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                vad_db->action = VAD_ACTION_NONE;
                ret = vad_state_set(VAD_STATE_STARTING);
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                vad_db->action = VAD_ACTION_NONE;
                ret = vad_state_set(VAD_STATE_RELEASING);
            }
        }
        else if (state == VAD_STATE_STARTING)
        {
            if (vad_db->action == VAD_ACTION_NONE)
            {
                /* Start the path immediately before the path created event
                 * received, so this action should be pending.
                 */
                vad_db->action = VAD_ACTION_START;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* Drop the repeating path start action. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                /* When the creating path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == VAD_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPING)
        {
            if (vad_db->action == VAD_ACTION_NONE)
            {
                /* Stopping the creating path is forbidden. */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* Cancel all pending actions when the creating path is pending
                 * for start action, but receives the stop action later.
                 */
                vad_db->action = VAD_ACTION_NONE;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                /* When the creating path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == VAD_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_RELEASING)
        {
            if (vad_db->action == VAD_ACTION_NONE)
            {
                vad_db->action = VAD_ACTION_RELEASE;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* Override the pending start action for the creating path. */
                vad_db->action = VAD_ACTION_RELEASE;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case VAD_STATE_CREATED:
        if (state == VAD_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTING)
        {
            vad_db->state = state;
            ret = audio_path_start(vad_db->handle);
            if (ret == false)
            {
                vad_db->state = VAD_STATE_CREATED;
            }
        }
        else if (state == VAD_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPING)
        {
            /* Stopping the created path is forbidden. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_RELEASING)
        {
            vad_db->state = state;
            ret = audio_path_destory(vad_db->handle);
            if (ret == false)
            {
                vad_db->state = VAD_STATE_CREATED;
            }
        }
        break;

    case VAD_STATE_STARTING:
        if (state == VAD_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTING)
        {
            if (vad_db->action == VAD_ACTION_NONE)
            {
                /* Drop the repeating path start action. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* Cancel all pending actions when the starting path is pending
                 * for stop action, but receives the start action later.
                 */
                vad_db->action = VAD_ACTION_NONE;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                /* When the starting path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == VAD_STATE_STARTED)
        {
            vad_db->state = state;

            if (vad_db->action == VAD_ACTION_NONE)
            {
                ret = true;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                vad_db->action = VAD_ACTION_NONE;
                ret = vad_state_set(VAD_STATE_STOPPING);
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                vad_db->action = VAD_ACTION_NONE;
                ret = vad_state_set(VAD_STATE_RELEASING);
            }
        }
        else if (state == VAD_STATE_STOPPING)
        {
            if (vad_db->action == VAD_ACTION_NONE)
            {
                vad_db->action = VAD_ACTION_STOP;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* Drop the repeating stop action. But this case is permitted. */
                ret = true;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                /* When the starting path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == VAD_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_RELEASING)
        {
            if (vad_db->action == VAD_ACTION_NONE)
            {
                vad_db->action = VAD_ACTION_RELEASE;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* Override the pending stop action for the starting path. */
                vad_db->action = VAD_ACTION_RELEASE;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case is permitted. */
                ret = true;
            }
        }
        break;

    case VAD_STATE_STARTED:
        if (state == VAD_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTING)
        {
            /* Starting the started path is forbidden. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPING)
        {
            vad_db->state = state;
            ret = audio_path_stop(vad_db->handle);
            if (ret == false)
            {
                vad_db->state = VAD_STATE_STARTED;
            }
        }
        else if (state == VAD_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_RELEASING)
        {
            /* Started path cannot be released directly, so it should be
             * stopped first before released.
             */
            vad_db->state  = VAD_STATE_STOPPING;
            vad_db->action = VAD_ACTION_RELEASE;

            ret = audio_path_stop(vad_db->handle);
            if (ret == false)
            {
                vad_db->state  = VAD_STATE_STARTED;
                vad_db->action = VAD_ACTION_NONE;
            }
        }
        break;

    case VAD_STATE_STOPPING:
        if (state == VAD_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTING)
        {
            if (vad_db->action == VAD_ACTION_NONE)
            {
                vad_db->action = VAD_ACTION_START;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* Drop the repeating start action. But this case is permitted. */
                ret = true;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                /* When the stopping path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == VAD_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPING)
        {
            if (vad_db->action == VAD_ACTION_NONE)
            {
                /* Drop the repeating stop action. But this case is permitted. */
                ret = true;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* Cancel all pending actions when the stopping path is pending
                 * for start action, but receives the stop action later.
                 */
                vad_db->action = VAD_ACTION_NONE;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                /* When the stopping path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == VAD_STATE_STOPPED)
        {
            vad_db->state = state;

            if (vad_db->action == VAD_ACTION_NONE)
            {
                ret = true;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                vad_db->action = VAD_ACTION_NONE;
                ret = vad_state_set(VAD_STATE_STARTING);
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                vad_db->action = VAD_ACTION_NONE;
                ret = vad_state_set(VAD_STATE_RELEASING);
            }
        }
        else if (state == VAD_STATE_RELEASING)
        {
            if (vad_db->action == VAD_ACTION_NONE)
            {
                vad_db->action = VAD_ACTION_RELEASE;
            }
            else if (vad_db->action == VAD_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping path is impossible.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_START)
            {
                /* Override the pending start action for the stopping path. */
                vad_db->action = VAD_ACTION_RELEASE;
            }
            else if (vad_db->action == VAD_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (vad_db->action == VAD_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case is permitted. */
                ret = true;
            }
        }
        break;

    case VAD_STATE_STOPPED:
        if (state == VAD_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTING)
        {
            vad_db->state = state;
            ret = audio_path_start(vad_db->handle);
            if (ret == false)
            {
                vad_db->state = VAD_STATE_STOPPED;
            }
        }
        else if (state == VAD_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPING)
        {
            /* Stopping the stopped path is forbidden. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_RELEASING)
        {
            vad_db->state = state;
            ret = audio_path_destory(vad_db->handle);
            if (ret == false)
            {
                vad_db->state = VAD_STATE_STOPPED;
            }
        }
        break;

    case VAD_STATE_RELEASING:
        if (state == VAD_STATE_RELEASED)
        {
            vad_db->state  = state;
            vad_db->action = VAD_ACTION_NONE;
            vad_db->handle = NULL;
        }
        else if (state == VAD_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTING)
        {
            /* Starting the releasing path is forbidden. */
            ret = false;
        }
        else if (state == VAD_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPING)
        {
            /* Stopping the releasing path is forbidden. */
            ret = false;
        }
        else if (state == VAD_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == VAD_STATE_RELEASING)
        {
            /* Drop the repeating release action. But this case is permitted. */
            ret = true;
        }
        break;
    }

    return ret;
}

static bool vad_path_cback(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_EVENT event, uint32_t param)
{
    AUDIO_PRINT_TRACE3("vad_path_cback: handle %p, event 0x%02x, param 0x%08x",
                       handle, event, param);

    switch (event)
    {
    case AUDIO_PATH_EVT_RELEASE:
        {
            vad_state_set(VAD_STATE_RELEASED);
        }
        break;

    case AUDIO_PATH_EVT_CREATE:
        {
            vad_state_set(VAD_STATE_CREATED);
        }
        break;

    case AUDIO_PATH_EVT_RUNNING:
        {
            vad_state_set(VAD_STATE_STARTED);
        }
        break;

    case AUDIO_PATH_EVT_IDLE:
        {
            vad_state_set(VAD_STATE_STOPPED);
        }
        break;

    case AUDIO_PATH_EVT_DATA_IND:
        {
            audio_mgr_dispatch(AUDIO_MSG_VOICE_ACTIVITY_DETECTED, NULL);
        }
        break;

    default:
        break;
    }

    return true;
}

bool vad_init(void)
{
    int32_t ret = 0;

    vad_db = os_mem_alloc2(sizeof(T_VAD_DB));
    if (vad_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    vad_db->state  = VAD_STATE_RELEASED;
    vad_db->action = VAD_ACTION_NONE;
    vad_db->volume_in = VAD_VOLUME_LEVEL_DEFAULT;

    if (vad_state_set(VAD_STATE_CREATING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    dsp_vad_param_load();

    return true;

fail_set_state:
    os_mem_free(vad_db);
    vad_db = NULL;
fail_alloc_db:
    AUDIO_PRINT_ERROR1("vad_init: ret %d", -ret);
    return false;
}

void vad_deinit(void)
{
    if (vad_db != NULL)
    {
        if (vad_db->handle != NULL)
        {
            vad_state_set(VAD_STATE_RELEASING);
        }

        os_mem_free(vad_db);
        vad_db = NULL;
    }
}

bool vad_enable(void)
{
    if (vad_db != NULL)
    {
        return vad_state_set(VAD_STATE_STARTING);
    }

    return false;
}

bool vad_disable(void)
{
    if (vad_db != NULL)
    {
        return vad_state_set(VAD_STATE_STOPPING);
    }

    return false;
}
#else
bool vad_init(void)
{
    /* Let VAD initialization pass. */
    return true;
}

void vad_deinit(void)
{

}

bool vad_enable(void)
{
    return false;
}

bool vad_disable(void)
{
    return false;
}

#endif /* AM_VAD_SUPPORT */
