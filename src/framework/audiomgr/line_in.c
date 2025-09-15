/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "audio_type.h"
#include "audio_mgr.h"
#include "audio_effect.h"
#include "audio_path.h"

#define LINE_IN_DAC_LEVEL_MIN       (0)
#define LINE_IN_DAC_LEVEL_MAX       (15)
#define LINE_IN_DAC_LEVEL_DEFAULT   (10)

#define LINE_IN_ADC_LEVEL_MIN       (0)
#define LINE_IN_ADC_LEVEL_MAX       (15)
#define LINE_IN_ADC_LEVEL_DEFAULT   (10)

typedef enum t_line_in_state
{
    LINE_IN_STATE_RELEASED  = 0x00,
    LINE_IN_STATE_CREATING  = 0x01,
    LINE_IN_STATE_CREATED   = 0x02,
    LINE_IN_STATE_STARTING  = 0x03,
    LINE_IN_STATE_STARTED   = 0x04,
    LINE_IN_STATE_STOPPING  = 0x05,
    LINE_IN_STATE_STOPPED   = 0x06,
    LINE_IN_STATE_PAUSING   = 0x07,
    LINE_IN_STATE_PAUSED    = 0x08,
    LINE_IN_STATE_RELEASING = 0x09,
} T_LINE_IN_STATE;

typedef enum t_line_in_action
{
    LINE_IN_ACTION_NONE     = 0x00,
    LINE_IN_ACTION_CREATE   = 0x01,
    LINE_IN_ACTION_START    = 0x02,
    LINE_IN_ACTION_STOP     = 0x03,
    LINE_IN_ACTION_PAUSE    = 0x04,
    LINE_IN_ACTION_RELEASE  = 0x05,
} T_LINE_IN_ACTION;

typedef struct t_line_in_db
{
    T_AUDIO_PATH_HANDLE handle;
    T_LINE_IN_STATE     state;
    T_LINE_IN_ACTION    action;
    uint8_t             volume_out_min;
    uint8_t             volume_out_max;
    uint8_t             volume_out;
    uint8_t             volume_in_min;
    uint8_t             volume_in_max;
    uint8_t             volume_in;
    bool                signal_in;
    bool                signal_out;
    uint16_t            signal_in_interval;
    uint16_t            signal_out_interval;
    float               volume_out_scale;
    bool                effect_apply;
    T_OS_QUEUE          effects;
    uint32_t            sample_rate;
    uint32_t            device;
} T_LINE_IN_DB;

#if (CONFIG_REALTEK_AM_LINE_IN_SUPPORT == 1)
static T_LINE_IN_DB *line_in_db = NULL;

static void line_in_volume_apply(void);
static bool line_in_effect_run(void);
static bool line_in_path_cback(T_AUDIO_PATH_HANDLE handle,
                               T_AUDIO_PATH_EVENT  event,
                               uint32_t            param);

static bool line_in_state_set(T_LINE_IN_STATE state)
{
    bool ret = true;

    AUDIO_PRINT_TRACE3("line_in_state_set: curr state %u, next state %u, action %u",
                       line_in_db->state, state, line_in_db->action);

    switch (line_in_db->state)
    {
    case LINE_IN_STATE_RELEASED:
        if (state == LINE_IN_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            T_ANALOG_INFO info;

            info.sample_rate = line_in_db->sample_rate;

            line_in_db->state  = state;
            line_in_db->handle = audio_path_create(AUDIO_CATEGORY_ANALOG,
                                                   line_in_db->device,
                                                   &info,
                                                   AUDIO_STREAM_MODE_NORMAL,
                                                   line_in_db->volume_out,
                                                   line_in_db->volume_in,
                                                   line_in_path_cback);
            if (line_in_db->handle == NULL)
            {
                line_in_db->state = LINE_IN_STATE_RELEASED;
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            /* Starting released path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            /* Stopping released path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            /* Pausing released path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_RELEASING)
        {
            /* Releasing released path is forbidden. */
            ret = false;
        }
        break;

    case LINE_IN_STATE_CREATING:
        if (state == LINE_IN_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            line_in_db->state = state;

            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = line_in_state_set(LINE_IN_STATE_STARTING);
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = line_in_state_set(LINE_IN_STATE_RELEASING);
            }
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                /* Start the path immediately before the path created event
                 * received, so this action should be pending.
                 */
                line_in_db->action = LINE_IN_ACTION_START;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Drop the repeating path start action. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the creating path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                /* Stopping the creating path is forbidden. */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Cancel all pending actions when the creating path is pending
                 * for start action, but receives the stop action later.
                 */
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the creating path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                /* Pausing the creating path is forbidden. */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Cancel all pending actions when the creating path is pending
                 * for start action, but receives the pause action later.
                 */
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the creating path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_RELEASING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                line_in_db->action = LINE_IN_ACTION_RELEASE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same creating path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Override the pending start action for the creating path. */
                line_in_db->action = LINE_IN_ACTION_RELEASE;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in creating state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case LINE_IN_STATE_CREATED:
        if (state == LINE_IN_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            line_in_db->state = state;
            ret = audio_path_start(line_in_db->handle);
            if (ret == false)
            {
                line_in_db->state = LINE_IN_STATE_CREATED;
            }
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            /* Stopping the created path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            /* Pausing the created path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_RELEASING)
        {
            line_in_db->state = state;
            ret = audio_path_destory(line_in_db->handle);
            if (ret == false)
            {
                line_in_db->state = LINE_IN_STATE_CREATED;
            }
        }
        break;

    case LINE_IN_STATE_STARTING:
        if (state == LINE_IN_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                /* Drop the repeating path start action. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* Cancel all pending actions when the starting path is pending
                 * for stop action, but receives the start action later.
                 */
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the starting path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            line_in_db->state = state;
            line_in_db->effect_apply = false;

            line_in_volume_apply();

            if (line_in_db->signal_out)
            {
                audio_path_signal_out_monitoring_set(line_in_db->handle, true, line_in_db->signal_out_interval);
            }

            if (line_in_db->signal_in)
            {
                audio_path_signal_in_monitoring_set(line_in_db->handle, true, line_in_db->signal_in_interval);
            }

            audio_mgr_dispatch(AUDIO_MSG_LINE_IN_START, NULL);

            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = line_in_state_set(LINE_IN_STATE_STOPPING);
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = line_in_state_set(LINE_IN_STATE_PAUSING);
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = line_in_state_set(LINE_IN_STATE_RELEASING);
            }
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                line_in_db->action = LINE_IN_ACTION_STOP;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* Drop the repeating stop action. But this case is permitted. */
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* Override the pending pause action for the starting path. */
                line_in_db->action = LINE_IN_ACTION_STOP;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the starting path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                line_in_db->action = LINE_IN_ACTION_PAUSE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* Drop the pause action for the starting path, as the pause
                 * action cannot override the pending stop action.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* Drop the repeating pause action. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the starting path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_RELEASING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                line_in_db->action = LINE_IN_ACTION_RELEASE;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same starting path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* This case will not happen in reality, as the pending start
                 * action will be dropped directly in starting state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* Override the pending stop action for the starting path. */
                line_in_db->action = LINE_IN_ACTION_RELEASE;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case is permitted. */
                ret = true;
            }
        }
        break;

    case LINE_IN_STATE_STARTED:
        if (state == LINE_IN_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            /* Starting the started path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            line_in_db->state = state;
            ret = audio_path_stop(line_in_db->handle);
            if (ret == false)
            {
                line_in_db->state = LINE_IN_STATE_STARTED;
            }
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            line_in_db->state = state;

            /* Currently, there is no audio path pause interface. */
            ret = audio_path_stop(line_in_db->handle);
            if (ret == false)
            {
                /* Restore the line_in_db state. */
                line_in_db->state = LINE_IN_STATE_STARTED;
            }
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            line_in_db->state = state;
            ret = true;
        }

        else if (state == LINE_IN_STATE_RELEASING)
        {
            /* Started path cannot be released directly, so it should be
             * stopped first before released.
             */
            line_in_db->state  = LINE_IN_STATE_STOPPING;
            line_in_db->action = LINE_IN_ACTION_RELEASE;

            ret = audio_path_stop(line_in_db->handle);
            if (ret == false)
            {
                line_in_db->state  = LINE_IN_STATE_STARTED;
                line_in_db->action = LINE_IN_ACTION_NONE;
            }
        }
        break;

    case LINE_IN_STATE_STOPPING:
        if (state == LINE_IN_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                line_in_db->action = LINE_IN_ACTION_START;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Drop the repeating start action. But this case is permitted. */
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the stopping path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                /* Drop the repeating stop action. But this case is permitted. */
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Cancel all pending actions when the stopping path is pending
                 * for start action, but receives the stop action later.
                 */
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the stopping path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            line_in_db->state = state;
            audio_mgr_dispatch(AUDIO_MSG_LINE_IN_STOP, NULL);

            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = line_in_state_set(LINE_IN_STATE_STARTING);
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = line_in_state_set(LINE_IN_STATE_RELEASING);
            }
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                /* Drop the pause action for the stopping path, as the
                 * pause action cannot override the stop action.
                 */
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Cancel all pending actions when the stopping path is pending
                 * for start action, but receives the pause action later.
                 */
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the stopping path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_RELEASING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                line_in_db->action = LINE_IN_ACTION_RELEASE;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same stopping path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Override the pending start action for the stopping path. */
                line_in_db->action = LINE_IN_ACTION_RELEASE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in stopping state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case is permitted. */
                ret = true;
            }
        }
        break;

    case LINE_IN_STATE_STOPPED:
        if (state == LINE_IN_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            line_in_db->state = state;
            ret = audio_path_start(line_in_db->handle);
            if (ret == false)
            {
                line_in_db->state = LINE_IN_STATE_STOPPED;
            }
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            /* Stopping the stopped path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            /* Pausing the stopped path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_RELEASING)
        {
            line_in_db->state = state;
            ret = audio_path_destory(line_in_db->handle);
            if (ret == false)
            {
                line_in_db->state = LINE_IN_STATE_STOPPED;
            }
        }
        break;

    case LINE_IN_STATE_PAUSING:
        if (state == LINE_IN_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                line_in_db->action = LINE_IN_ACTION_START;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Drop the repeating start action. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will override the pausing state directly.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the pausing path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                /* Override the pausing state. */
                line_in_db->state = state;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Cancel all pending actions when the pausing path is pending
                 * for start action, but receives the stop action later.
                 */
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will override the pausing state directly.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the pausing path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            line_in_db->state = LINE_IN_STATE_PAUSED;

            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = line_in_state_set(LINE_IN_STATE_STARTING);
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will override the pausing state directly.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = line_in_state_set(LINE_IN_STATE_RELEASING);
            }
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                /* Drop the repeating pause action. But this case
                 * is permitted.
                 */
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Cancel all pending actions when the pausing path is pending
                 * for start action, but receives the pause action later.
                 */
                line_in_db->action = LINE_IN_ACTION_NONE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* This case will not happen in reality, as the pending stop
                 * action will override the pausing state directly.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* When the pausing path is pending for releasing, any
                 * following actions should be dropped.
                 */
                ret = false;
            }
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            /* This case will not happen in reality, as the pausing
             * audio path will send stopped event directly.
             */
            ret = false;
        }
        else if (state == LINE_IN_STATE_RELEASING)
        {
            if (line_in_db->action == LINE_IN_ACTION_NONE)
            {
                line_in_db->action = LINE_IN_ACTION_RELEASE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_CREATE)
            {
                /* This case will not happen in reality, as the pending create
                 * action of the same pausing path is impossible.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_START)
            {
                /* Override the pending start action for the pausing path. */
                line_in_db->action = LINE_IN_ACTION_RELEASE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_STOP)
            {
                /* Override the pending start action for the pausing path. */
                line_in_db->action = LINE_IN_ACTION_RELEASE;
                ret = true;
            }
            else if (line_in_db->action == LINE_IN_ACTION_PAUSE)
            {
                /* This case will not happen in reality, as the pending pause
                 * action will be dropped directly in pausing state.
                 */
                ret = false;
            }
            else if (line_in_db->action == LINE_IN_ACTION_RELEASE)
            {
                /* Drop the repeating release action. But this case
                 * is permitted.
                 */
                ret = true;
            }
        }
        break;

    case LINE_IN_STATE_PAUSED:
        if (state == LINE_IN_STATE_RELEASED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            line_in_db->state = state;
            ret = audio_path_start(line_in_db->handle);
            if (ret == false)
            {
                line_in_db->state = LINE_IN_STATE_PAUSED;
            }
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            line_in_db->state = state;
            line_in_db->effect_apply = false;

            line_in_volume_apply();
            ret = true;
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            line_in_db->state = state;
            ret = audio_path_stop(line_in_db->handle);
            if (ret == false)
            {
                /* Restore the path state. */
                line_in_db->state = LINE_IN_STATE_PAUSED;
            }
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            /* Pausing the paused path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_RELEASING)
        {
            line_in_db->state  = LINE_IN_STATE_STOPPING;
            line_in_db->action = LINE_IN_ACTION_RELEASE;

            ret = audio_path_stop(line_in_db->handle);
            if (ret == false)
            {
                /* Restore the path state. */
                line_in_db->state = LINE_IN_STATE_PAUSED;
                line_in_db->action = LINE_IN_ACTION_NONE;
            }
        }
        break;

    case LINE_IN_STATE_RELEASING:
        if (state == LINE_IN_STATE_RELEASED)
        {
            line_in_db->state  = state;
            line_in_db->action = LINE_IN_ACTION_NONE;
            line_in_db->handle = NULL;
            ret = true;
        }
        else if (state == LINE_IN_STATE_CREATING)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_CREATED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTING)
        {
            /* Starting the releasing path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STARTED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPING)
        {
            /* Stopping the releasing path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_STOPPED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSING)
        {
            /* Pausing the releasing path is forbidden. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_PAUSED)
        {
            /* This case will not happen in reality. */
            ret = false;
        }
        else if (state == LINE_IN_STATE_RELEASING)
        {
            /* Drop the repeating release action. But this case is permitted. */
            ret = true;
        }
        break;
    }

    return ret;
}

static bool line_in_path_cback(T_AUDIO_PATH_HANDLE handle,
                               T_AUDIO_PATH_EVENT  event,
                               uint32_t            param)
{
    AUDIO_PRINT_TRACE3("line_in_path_cback: handle %p, event 0x%02x, param 0x%08x",
                       handle, event, param);

    switch (event)
    {
    case AUDIO_PATH_EVT_CREATE:
        {
            line_in_state_set(LINE_IN_STATE_CREATED);
        }
        break;

    case AUDIO_PATH_EVT_IDLE:
        {
            line_in_state_set(LINE_IN_STATE_STOPPED);
        }
        break;

    case AUDIO_PATH_EVT_EFFECT_REQ:
        {
            line_in_db->effect_apply = true;
            line_in_effect_run();
        }
        break;

    case AUDIO_PATH_EVT_RUNNING:
        {
            line_in_state_set(LINE_IN_STATE_STARTED);
        }
        break;

    case AUDIO_PATH_EVT_SUSPEND:
        {
            line_in_state_set(LINE_IN_STATE_PAUSED);
        }
        break;

    case AUDIO_PATH_EVT_RELEASE:
        {
            line_in_state_set(LINE_IN_STATE_RELEASED);
        }
        break;

    case AUDIO_PATH_EVT_SIGNAL_OUT_REFRESH:
        {
            T_AUDIO_MSG_SIGNAL_OUT_REFRESH *p_msg = (T_AUDIO_MSG_SIGNAL_OUT_REFRESH *)param;
            T_AUDIO_MSG_PAYLOAD_LINE_IN_SIGNAL_OUT_REFRESH msg;

            msg.left_gain = p_msg->left_gain;
            msg.right_gain = p_msg->right_gain;
            audio_mgr_dispatch(AUDIO_MSG_LINE_IN_SIGNAL_OUT_REFRESH, &msg);
        }
        break;

    default:
        break;
    }

    return true;
}

bool line_in_init(void)
{
    int32_t ret = 0;

    line_in_db = os_mem_alloc2(sizeof(T_LINE_IN_DB));
    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    line_in_db->state          = LINE_IN_STATE_RELEASED;
    line_in_db->action         = LINE_IN_ACTION_NONE;
    line_in_db->volume_out_min = LINE_IN_DAC_LEVEL_MIN;
    line_in_db->volume_out_max = LINE_IN_DAC_LEVEL_MAX;
    line_in_db->volume_out     = LINE_IN_DAC_LEVEL_DEFAULT;
    line_in_db->volume_out_scale = 0.0f;
    line_in_db->volume_in_min  = LINE_IN_ADC_LEVEL_MIN;
    line_in_db->volume_in_max  = LINE_IN_ADC_LEVEL_MAX;
    line_in_db->volume_in      = LINE_IN_ADC_LEVEL_DEFAULT;
    line_in_db->signal_in      = false;
    line_in_db->signal_out     = false;
    line_in_db->signal_in_interval  = 0;
    line_in_db->signal_out_interval = 0;
    line_in_db->effect_apply   = false;
    line_in_db->handle         = NULL;
    line_in_db->sample_rate    = 0;
    os_queue_init(&line_in_db->effects);

    return true;

fail_alloc_db:
    AUDIO_PRINT_ERROR1("line_in_init: failed %d", -ret);
    return false;
}

void line_in_deinit(void)
{
    if (line_in_db != NULL)
    {
        T_AUDIO_EFFECT_INSTANCE instance;

        while ((instance = os_queue_out(&line_in_db->effects)) != NULL)
        {
            audio_effect_owner_clear(instance);
        }

        if (line_in_db->handle != NULL)
        {
            audio_path_destory(line_in_db->handle);
            line_in_db->handle = NULL;
        }

        os_mem_free(line_in_db);
        line_in_db = NULL;
    }
}

bool line_in_create(uint32_t device,
                    uint32_t sample_rate)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    if (line_in_db->handle != NULL)
    {
        ret = 2;
        goto fail_invalid_handle;
    }

    line_in_db->sample_rate = sample_rate;
    line_in_db->device      = device;

    if (line_in_state_set(LINE_IN_STATE_CREATING) == false)
    {
        ret = 3;
        goto fail_create_path;
    }

    return true;

fail_create_path:
fail_invalid_handle:
fail_invalid_db:
    AUDIO_PRINT_ERROR1("line_in_create: failed %d", -ret);

    return false;
}

bool line_in_destroy(void)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    if (line_in_state_set(LINE_IN_STATE_RELEASING)  == false)
    {
        ret = 2;
        goto fail_destroy_path;
    }

    line_in_db->handle = NULL;

    return true;

fail_destroy_path:
fail_invalid_db:

    AUDIO_PRINT_ERROR1("line_in_destroy: failed %d", -ret);
    return false;
}

bool line_in_start(void)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    AUDIO_PRINT_TRACE3("line_in_start: handle %p, state %u, action %u",
                       line_in_db->handle, line_in_db->state, line_in_db->action);

    if (line_in_state_set(LINE_IN_STATE_STARTING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_invalid_db:
    AUDIO_PRINT_ERROR1("line_in_start: failed %d", -ret);
    return false;
}

bool line_in_stop(void)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    AUDIO_PRINT_TRACE3("line_in_stop: handle %p, state %u, action %u",
                       line_in_db->handle, line_in_db->state, line_in_db->action);

    if (line_in_state_set(LINE_IN_STATE_STOPPING) == false)
    {
        ret = 2;
        goto fail_set_state;
    }

    return true;

fail_set_state:
fail_invalid_db:
    AUDIO_PRINT_ERROR1("line_in_stop: failed %d", -ret);
    return false;
}

static void line_in_volume_apply(void)
{
    audio_path_dac_level_set(line_in_db->handle, line_in_db->volume_out, line_in_db->volume_out_scale);
    audio_path_adc_level_set(line_in_db->handle, line_in_db->volume_in, 0.0f);
}

uint8_t line_in_volume_out_max_get(void)
{
    if (line_in_db != NULL)
    {
        return line_in_db->volume_out_max;
    }

    return 0;
}

bool line_in_volume_out_max_set(uint8_t volume)
{
    if (line_in_db != NULL)
    {
        line_in_db->volume_out_max = volume;
        return true;
    }

    return false;
}

uint8_t line_in_volume_out_min_get(void)
{
    if (line_in_db != NULL)
    {
        return line_in_db->volume_out_min;
    }

    return 0;
}

bool line_in_volume_out_min_set(uint8_t volume)
{
    if (line_in_db != NULL)
    {
        line_in_db->volume_out_min = volume;
        return true;
    }

    return false;
}

uint8_t line_in_volume_out_get(void)
{
    if (line_in_db != NULL)
    {
        return line_in_db->volume_out;
    }

    return 0;
}

bool line_in_volume_out_set(uint8_t volume)
{
    T_AUDIO_MSG_PAYLOAD_LINE_IN_VOLUME_OUT_CHANGE payload;
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    if (volume < line_in_db->volume_out_min ||
        volume > line_in_db->volume_out_max)
    {
        ret = 2;
        goto fail_check_volume;
    }

    if (audio_path_dac_level_set(line_in_db->handle, volume, line_in_db->volume_out_scale) == false)
    {
        ret = 3;
        goto fail_set_volume;
    }

    payload.prev_volume     = line_in_db->volume_out;
    payload.curr_volume     = volume;

    line_in_db->volume_out  = volume;

    return audio_mgr_dispatch(AUDIO_MSG_LINE_IN_VOLUME_OUT_CHANGE, &payload);

fail_set_volume:
fail_check_volume:
fail_invalid_db:
    AUDIO_PRINT_ERROR2("line_in_volume_out_set: volume %u, failed %d",
                       volume, -ret);
    return false;
}

uint8_t line_in_volume_in_max_get(void)
{
    if (line_in_db != NULL)
    {
        return line_in_db->volume_in_max;
    }

    return 0;
}

bool line_in_volume_in_max_set(uint8_t volume)
{
    if (line_in_db != NULL)
    {
        line_in_db->volume_in_max = volume;
        return true;
    }

    return false;
}

uint8_t line_in_volume_in_min_get(void)
{
    if (line_in_db != NULL)
    {
        return line_in_db->volume_in_min;
    }

    return 0;
}

bool line_in_volume_in_min_set(uint8_t volume)
{
    if (line_in_db != NULL)
    {
        line_in_db->volume_in_min = volume;
        return true;
    }

    return false;
}

uint8_t line_in_volume_in_get(void)
{
    if (line_in_db != NULL)
    {
        return line_in_db->volume_in;
    }

    return 0;
}

bool line_in_volume_in_set(uint8_t volume)
{
    T_AUDIO_MSG_PAYLOAD_LINE_IN_VOLUME_IN_CHANGE payload;
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    if (volume < line_in_db->volume_in_min ||
        volume > line_in_db->volume_in_max)
    {
        ret = 2;
        goto fail_check_volume;
    }

    if (audio_path_adc_level_set(line_in_db->handle, volume, 0.0f) == false)
    {
        ret = 3;
        goto fail_set_volume;
    }

    payload.prev_volume     = line_in_db->volume_in;
    payload.curr_volume     = volume;

    line_in_db->volume_in   = volume;

    return audio_mgr_dispatch(AUDIO_MSG_LINE_IN_VOLUME_IN_CHANGE, &payload);

fail_set_volume:
fail_check_volume:
fail_invalid_db:
    AUDIO_PRINT_ERROR2("line_in_volume_in_set: volume %u, failed %d",
                       volume, -ret);
    return false;
}

bool line_in_signal_out_monitoring_start(uint16_t refresh_interval)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    line_in_db->signal_out = true;
    line_in_db->signal_out_interval = refresh_interval;
    audio_path_signal_out_monitoring_set(line_in_db->handle, true, refresh_interval);

    return true;

fail_invalid_db:
    AUDIO_PRINT_ERROR1("line_in_signal_out_monitoring_start: failed %d", -ret);
    return false;
}

bool line_in_signal_out_monitoring_stop(void)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    line_in_db->signal_out = false;
    line_in_db->signal_out_interval = 0;
    audio_path_signal_out_monitoring_set(line_in_db->handle, false, 0);

    return true;

fail_invalid_db:
    AUDIO_PRINT_ERROR1("line_in_signal_out_monitoring_stop: failed %d", -ret);
    return false;
}

bool line_in_signal_in_monitoring_start(uint16_t refresh_interval)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    line_in_db->signal_in = true;
    line_in_db->signal_in_interval = refresh_interval;
    audio_path_signal_in_monitoring_set(line_in_db->handle, true, refresh_interval);

    return true;

fail_invalid_db:
    AUDIO_PRINT_ERROR1("line_in_signal_in_monitoring_start: failed %d", -ret);
    return false;
}

bool line_in_signal_in_monitoring_stop(void)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    line_in_db->signal_in = false;
    line_in_db->signal_in_interval = 0;
    audio_path_signal_in_monitoring_set(line_in_db->handle, false, 0);

    return true;

fail_invalid_db:
    AUDIO_PRINT_ERROR1("line_in_signal_in_monitoring_stop: failed %d", -ret);
    return false;
}

float line_in_volume_balance_get(void)
{
    if (line_in_db != NULL)
    {
        return line_in_db->volume_out_scale;
    }

    return 0.0f;
}

bool line_in_volume_balance_set(float scale)
{
    T_AUDIO_MSG_PAYLOAD_LINE_IN_VOLUME_BALANCE_CHANGE payload;
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    if (scale < -1.0f ||
        scale > 1.0f)
    {
        ret = 2;
        goto fail_check_scale;
    }

    if (audio_path_dac_level_set(line_in_db->handle, line_in_db->volume_out, scale) == false)
    {
        ret = 3;
        goto fail_set_scale;
    }

    payload.prev_scale      = line_in_db->volume_out_scale;
    payload.curr_scale      = scale;

    line_in_db->volume_out_scale = scale;

    return audio_mgr_dispatch(AUDIO_MSG_LINE_IN_VOLUME_BALANCE_CHANGE, &payload);

fail_set_scale:
fail_check_scale:
fail_invalid_db:
    AUDIO_PRINT_ERROR2("line_in_volume_balance_set: scale %d/1000, failed %d",
                       (int32_t)(scale * 1000), -ret);
    return false;
}

static bool line_in_effect_check(T_AUDIO_EFFECT_INSTANCE instance)
{
    if (instance != NULL)
    {
        return os_queue_search(&line_in_db->effects, instance);
    }

    return false;
}

static bool line_in_effect_enqueue(T_AUDIO_EFFECT_INSTANCE instance)
{
    if (instance != NULL)
    {
        if (os_queue_search(&line_in_db->effects, instance) == false)
        {
            os_queue_in(&line_in_db->effects, instance);
        }

        return true;
    }

    return false;
}

static bool line_in_effect_dequeue(T_AUDIO_EFFECT_INSTANCE instance)
{
    if (instance != NULL)
    {
        return os_queue_delete(&line_in_db->effects, instance);
    }

    return false;
}

static bool line_in_effect_run(void)
{
    T_AUDIO_EFFECT_INSTANCE instance;
    int32_t                 i = 0;

    while ((instance = os_queue_peek(&line_in_db->effects, i)) != NULL)
    {
        audio_effect_run(instance);
        i++;
    }

    return true;
}

static void line_in_effect_cback(T_AUDIO_EFFECT_INSTANCE instance,
                                 T_AUDIO_EFFECT_EVENT    event)
{
    T_AUDIO_PATH_HANDLE handle;

    handle = audio_effect_owner_get(instance);

    AUDIO_PRINT_TRACE3("line_in_effect_cback: instance %p, handle %p, event 0x%02x",
                       instance, handle, event);

    if (handle == line_in_db->handle)
    {
        switch (event)
        {
        case AUDIO_EFFECT_EVENT_CREATED:
            break;

        case AUDIO_EFFECT_EVENT_ENABLED:
            if (line_in_db->state == LINE_IN_STATE_STARTED)
            {
                audio_effect_run(instance);
            }
            else if (line_in_db->state == LINE_IN_STATE_STARTING ||
                     line_in_db->state == LINE_IN_STATE_PAUSED)
            {
                if (line_in_db->effect_apply == true)
                {
                    audio_effect_run(instance);
                }
            }
            break;

        case AUDIO_EFFECT_EVENT_DISABLED:
            if (line_in_db->state == LINE_IN_STATE_STARTED)
            {
                audio_effect_stop(instance);
            }
            else if (line_in_db->state == LINE_IN_STATE_STARTING ||
                     line_in_db->state == LINE_IN_STATE_PAUSED)
            {
                if (line_in_db->effect_apply == true)
                {
                    audio_effect_stop(instance);
                }
            }
            break;

        case AUDIO_EFFECT_EVENT_UPDATED:
            if (line_in_db->state == LINE_IN_STATE_STARTED)
            {
                audio_effect_run(instance);
            }
            else if (line_in_db->state == LINE_IN_STATE_STARTING ||
                     line_in_db->state == LINE_IN_STATE_PAUSED)
            {
                if (line_in_db->effect_apply == true)
                {
                    audio_effect_run(instance);
                }
            }
            break;

        case AUDIO_EFFECT_EVENT_RELEASED:
            if (line_in_db->state == LINE_IN_STATE_STARTED)
            {
                audio_effect_stop(instance);
            }
            else if (line_in_db->state == LINE_IN_STATE_STARTING ||
                     line_in_db->state == LINE_IN_STATE_PAUSED)
            {
                if (line_in_db->effect_apply == true)
                {
                    audio_effect_stop(instance);
                }
            }

            line_in_effect_dequeue(instance);
            audio_effect_owner_clear(instance);
            break;
        }
    }
}

bool line_in_effect_attach(T_AUDIO_EFFECT_INSTANCE instance)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    if (line_in_effect_enqueue(instance) == false)
    {
        ret = 2;
        goto fail_enqueue_effect;
    }

    if (audio_effect_owner_set(instance,
                               line_in_db->handle,
                               line_in_effect_cback) == false)
    {
        ret = 3;
        goto fail_set_owner;
    }

    if (line_in_db->state == LINE_IN_STATE_STARTED)
    {
        audio_effect_run(instance);
    }

    AUDIO_PRINT_TRACE1("line_in_effect_attach: instance %p", instance);
    return true;

fail_set_owner:
    line_in_effect_dequeue(instance);
fail_enqueue_effect:
fail_invalid_db:
    AUDIO_PRINT_ERROR2("line_in_effect_attach: instance %p, failed %d",
                       instance, -ret);
    return false;
}

bool line_in_effect_detach(T_AUDIO_EFFECT_INSTANCE instance)
{
    int32_t ret = 0;

    if (line_in_db == NULL)
    {
        ret = 1;
        goto fail_invalid_db;
    }

    if (line_in_effect_check(instance) == false)
    {
        ret = 2;
        goto fail_check_instance;
    }

    if (audio_effect_owner_clear(instance) == false)
    {
        ret = 3;
        goto fail_clear_owner;
    }

    if (line_in_effect_dequeue(instance) == false)
    {
        ret = 4;
        goto fail_dequeue_effect;
    }

    if (line_in_db->state == LINE_IN_STATE_STARTED)
    {
        audio_effect_stop(instance);
    }

    AUDIO_PRINT_TRACE1("line_in_effect_detach: instance %p", instance);
    return true;

fail_dequeue_effect:
    audio_effect_owner_set(instance,
                           line_in_db->handle,
                           line_in_effect_cback);
fail_clear_owner:
fail_check_instance:
fail_invalid_db:
    AUDIO_PRINT_ERROR2("line_in_effect_detach: instance %p, failed %d",
                       instance, -ret);
    return false;
}
#else
bool line_in_init(void)
{
    /* Let Line-in initialization pass. */
    return true;
}

void line_in_deinit(void)
{

}

bool line_in_create(uint32_t device,
                    uint32_t sample_rate)
{
    return false;
}

bool line_in_destroy(void)
{
    return false;
}

bool line_in_start(void)
{
    return false;
}

bool line_in_stop(void)
{
    return false;
}

uint8_t line_in_volume_out_max_get(void)
{
    return 0;
}

bool line_in_volume_out_max_set(uint8_t level)
{
    return false;
}

uint8_t line_in_volume_out_min_get(void)
{
    return 0;
}

bool line_in_volume_out_min_set(uint8_t volume)
{
    return false;
}

uint8_t line_in_volume_out_get(void)
{
    return 0;
}

bool line_in_volume_out_set(uint8_t volume)
{
    return false;
}

uint8_t line_in_volume_in_max_get(void)
{
    return 0;
}

bool line_in_volume_in_max_set(uint8_t volume)
{
    return false;
}

uint8_t line_in_volume_in_min_get(void)
{
    return 0;
}

bool line_in_volume_in_min_set(uint8_t volume)
{
    return false;
}

uint8_t line_in_volume_in_get(void)
{
    return 0;
}

bool line_in_volume_in_set(uint8_t volume)
{
    return false;
}

bool line_in_signal_out_monitoring_start(uint16_t refresh_interval)
{
    return false;
}

bool line_in_signal_out_monitoring_stop(void)
{
    return false;
}

bool line_in_signal_in_monitoring_start(uint16_t refresh_interval)
{
    return false;
}

bool line_in_signal_in_monitoring_stop(void)
{
    return false;
}

float line_in_volume_balance_get(void)
{
    return 0.0f;
}

bool line_in_volume_balance_set(float scale)
{
    return false;
}

bool line_in_effect_attach(T_AUDIO_EFFECT_INSTANCE instance)
{
    return false;
}

bool line_in_effect_detach(T_AUDIO_EFFECT_INSTANCE instance)
{
    return false;
}
#endif /* CONFIG_REALTEK_AM_LINE_IN_SUPPORT */
