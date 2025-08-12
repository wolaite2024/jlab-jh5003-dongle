/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "trace.h"
#include "audio_mgr.h"
#include "audio_effect.h"
#include "audio_path.h"
#include "audio_passthrough.h"

/* TODO Remove Start */
#include "anc_mgr.h"
#include "dsp_ipc.h"
/* TODO Remove End */

#define AUDIO_PASSTHROUGH_DAC_LEVEL_MIN     (0)
#define AUDIO_PASSTHROUGH_DAC_LEVEL_MAX     (15)
#define AUDIO_PASSTHROUGH_DAC_LEVEL_DEFAULT (10)

#define AUDIO_PASSTHROUGH_ADC_LEVEL_MIN     (0)
#define AUDIO_PASSTHROUGH_ADC_LEVEL_MAX     (15)
#define AUDIO_PASSTHROUGH_ADC_LEVEL_DEFAULT (10)

#define AUDIO_PASSTHROUGH_BRIGHTNESS_STRENGTH_MIN     (0.0f)
#define AUDIO_PASSTHROUGH_BRIGHTNESS_STRENGTH_MAX     (1.0f)
#define AUDIO_PASSTHROUGH_BRIGHTNESS_STRENGTH_DEFAULT (0.5f)

#define AUDIO_PASSTHROUGH_OVP_LEVEL_MIN     (0)
#define AUDIO_PASSTHROUGH_OVP_LEVEL_MAX     (15)
#define AUDIO_PASSTHROUGH_OVP_LEVEL_DEFAULT (7)

typedef enum t_audio_passthrough_session_state
{
    AUDIO_PASSTHROUGH_SESSION_STATE_RELEASED,
    AUDIO_PASSTHROUGH_SESSION_STATE_CREATED,
    AUDIO_PASSTHROUGH_SESSION_STATE_STARTED,
    AUDIO_PASSTHROUGH_SESSION_STATE_STARTING,
    AUDIO_PASSTHROUGH_SESSION_STATE_START_FAIL,
    AUDIO_PASSTHROUGH_SESSION_STATE_STOPED,
    AUDIO_PASSTHROUGH_SESSION_STATE_STOPPING,
    AUDIO_PASSTHROUGH_SESSION_STATE_STOP_FAIL,
} T_AUDIO_PASSTHROUGH_SESSION_STATE;

typedef struct t_audio_passthrough_db
{
    T_AUDIO_PATH_HANDLE               apt_handle;
    T_AUDIO_PATH_HANDLE               llapt_handle;
    T_AUDIO_PATH_HANDLE               curr_handle;
    T_AUDIO_PASSTHROUGH_MODE          mode;
    uint8_t                           llapt_scenario_id;
    T_AUDIO_PASSTHROUGH_SESSION_STATE state;
    uint8_t                           volume_out_level_min;
    uint8_t                           volume_out_level_max;
    bool                              volume_out_muted;
    uint8_t                           volume_out_level;
    float                             volume_out_scale;
    uint8_t                           volume_in_level_min;
    uint8_t                           volume_in_level_max;
    bool                              volume_in_muted;
    uint8_t                           volume_in_level;
    bool                              ovp_enable;
    uint8_t                           ovp_level;
    bool                              beamforming_enable;
    uint8_t                           beamforming_direction;
    bool                              effect_apply;
    T_OS_QUEUE                        effects;
    float                             brightness_strength_min;
    float                             brightness_strength_max;
    float                             brightness_strength;
} T_AUDIO_PASSTHROUGH_DB;

static T_AUDIO_PASSTHROUGH_DB audio_passthrough_db;

static bool audio_passthrough_effect_run(void);

static bool audio_passthrough_path_cback(T_AUDIO_PATH_HANDLE handle, T_AUDIO_PATH_EVENT event,
                                         uint32_t param)
{
    bool ret = false;

    AUDIO_PRINT_TRACE4("audio_passthrough_path_cback: handle %p, event 0x%02x, param 0x%08x, state %02x",
                       handle, event, param, audio_passthrough_db.state);

    switch (event)
    {
    case AUDIO_PATH_EVT_RUNNING:
        {
            if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STOP_FAIL)
            {
                ret = audio_passthrough_disable();
                if (ret == true)
                {
                    audio_passthrough_db.state = AUDIO_PASSTHROUGH_SESSION_STATE_STOPPING;
                }
            }
            else
            {
                T_AUDIO_MSG_PAYLOAD_PASSTHROUGH_ENABLE payload;

                audio_passthrough_db.state = AUDIO_PASSTHROUGH_SESSION_STATE_STARTED;

                if (audio_passthrough_db.mode == AUDIO_PASSTHROUGH_MODE_NORMAL)
                {
                    audio_passthrough_db.effect_apply = false;

                    if (audio_passthrough_db.volume_out_muted == false)
                    {
                        audio_path_dac_level_set(handle,
                                                 audio_passthrough_db.volume_out_level,
                                                 audio_passthrough_db.volume_out_scale);
                    }
                    else
                    {
                        audio_path_dac_mute(handle);
                    }

                    if (audio_passthrough_db.volume_in_muted == false)
                    {
                        audio_path_adc_level_set(handle,
                                                 audio_passthrough_db.volume_in_level,
                                                 0.0f);
                    }
                    else
                    {
                        audio_path_adc_mute(handle);
                    }

                    if (audio_passthrough_db.ovp_enable == true)
                    {
                        dsp_ipc_apt_ovp_set(1, audio_passthrough_db.ovp_level);
                    }

                    if (audio_passthrough_db.beamforming_enable == true)
                    {
                        dsp_ipc_apt_beamforming_set(1, audio_passthrough_db.beamforming_direction);
                    }
                }
                else
                {
                    if (audio_passthrough_db.volume_out_muted == false)
                    {
                        audio_path_dac_level_set(handle,
                                                 audio_passthrough_db.volume_out_level,
                                                 audio_passthrough_db.volume_out_scale);
                    }
                    else
                    {
                        audio_path_dac_mute(handle);
                    }

                    audio_path_brightness_set(audio_passthrough_db.llapt_handle,
                                              audio_passthrough_db.brightness_strength);
                }

                payload.mode = audio_passthrough_db.mode;
                audio_mgr_dispatch(AUDIO_MSG_PASSTHROUGH_ENABLED, &payload);
            }
        }
        break;

    case AUDIO_PATH_EVT_IDLE:
        {
            if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_START_FAIL)
            {
                ret = audio_passthrough_enable(audio_passthrough_db.mode, audio_passthrough_db.llapt_scenario_id);

                if (ret == true)
                {
                    audio_passthrough_db.state = AUDIO_PASSTHROUGH_SESSION_STATE_STARTING;
                }
            }
            else
            {
                T_AUDIO_MSG_PAYLOAD_PASSTHROUGH_DISABLE payload;

                audio_passthrough_db.state = AUDIO_PASSTHROUGH_SESSION_STATE_STOPED;
                payload.mode = audio_passthrough_db.mode;

                audio_mgr_dispatch(AUDIO_MSG_PASSTHROUGH_DISABLED, &payload);
            }
        }
        break;

    case AUDIO_PATH_EVT_EFFECT_REQ:
        {
            if (audio_passthrough_db.mode == AUDIO_PASSTHROUGH_MODE_NORMAL)
            {
                audio_passthrough_db.effect_apply = true;
                audio_passthrough_effect_run();
            }
        }
        break;

    default:
        break;
    }

    return true;
}

bool audio_passthrough_init(void)
{
    audio_passthrough_db.apt_handle           = NULL;
    audio_passthrough_db.llapt_handle         = NULL;
    audio_passthrough_db.curr_handle          = NULL;
    audio_passthrough_db.state                = AUDIO_PASSTHROUGH_SESSION_STATE_RELEASED;
    audio_passthrough_db.volume_out_level_min = AUDIO_PASSTHROUGH_DAC_LEVEL_MIN;
    audio_passthrough_db.volume_out_level_max = AUDIO_PASSTHROUGH_DAC_LEVEL_MAX;
    audio_passthrough_db.volume_out_muted     = false;
    audio_passthrough_db.volume_out_level     = AUDIO_PASSTHROUGH_DAC_LEVEL_DEFAULT;
    audio_passthrough_db.volume_out_scale     = 0.0f;
    audio_passthrough_db.volume_in_level_min  = AUDIO_PASSTHROUGH_ADC_LEVEL_MIN;
    audio_passthrough_db.volume_in_level_max  = AUDIO_PASSTHROUGH_ADC_LEVEL_MAX;
    audio_passthrough_db.volume_in_muted      = false;
    audio_passthrough_db.volume_in_level      = AUDIO_PASSTHROUGH_ADC_LEVEL_DEFAULT;
    audio_passthrough_db.brightness_strength_min  = AUDIO_PASSTHROUGH_BRIGHTNESS_STRENGTH_MIN;
    audio_passthrough_db.brightness_strength_max  = AUDIO_PASSTHROUGH_BRIGHTNESS_STRENGTH_MAX;
    audio_passthrough_db.brightness_strength      = AUDIO_PASSTHROUGH_BRIGHTNESS_STRENGTH_DEFAULT;
    audio_passthrough_db.ovp_enable           = false;
    audio_passthrough_db.ovp_level            = AUDIO_PASSTHROUGH_OVP_LEVEL_DEFAULT;
    audio_passthrough_db.beamforming_enable   = false;
    audio_passthrough_db.beamforming_direction = 0;
    audio_passthrough_db.llapt_scenario_id    = 0;
    audio_passthrough_db.effect_apply         = false;
    os_queue_init(&audio_passthrough_db.effects);

    return true;
}

void audio_passthrough_deinit(void)
{
    T_AUDIO_EFFECT_INSTANCE instance;

    while ((instance = os_queue_out(&audio_passthrough_db.effects)) != NULL)
    {
        audio_effect_owner_clear(instance);
    }

    if (audio_passthrough_db.apt_handle != NULL)
    {
        audio_path_destory(audio_passthrough_db.apt_handle);
        audio_passthrough_db.apt_handle = NULL;
    }

    if (audio_passthrough_db.llapt_handle != NULL)
    {
        audio_path_destory(audio_passthrough_db.llapt_handle);
        audio_passthrough_db.llapt_handle = NULL;
    }

    audio_passthrough_db.curr_handle = NULL;
}

bool audio_passthrough_create(uint32_t sample_rate)
{
    int32_t ret = 0;
    T_APT_INFO info;

    info.sample_rate = sample_rate;

    if (audio_passthrough_db.apt_handle != NULL)
    {
        ret = 1;
        goto fail_apt_path_exist;
    }

    audio_passthrough_db.apt_handle = audio_path_create(AUDIO_CATEGORY_APT,
                                                        AUDIO_DEVICE_IN_MIC,
                                                        &info,
                                                        AUDIO_STREAM_MODE_NORMAL,
                                                        audio_passthrough_db.volume_out_level,
                                                        audio_passthrough_db.volume_in_level,
                                                        audio_passthrough_path_cback);
    if (audio_passthrough_db.apt_handle == NULL)
    {
        ret = 2;
        goto fail_create_apt_path;
    }

    if (audio_passthrough_db.llapt_handle != NULL)
    {
        ret = 3;
        goto fail_llapt_path_exist;
    }

    audio_passthrough_db.llapt_handle = audio_path_create(AUDIO_CATEGORY_LLAPT,
                                                          AUDIO_DEVICE_IN_MIC,
                                                          NULL,
                                                          AUDIO_STREAM_MODE_NORMAL,
                                                          audio_passthrough_db.volume_out_level,
                                                          audio_passthrough_db.volume_in_level,
                                                          audio_passthrough_path_cback);
    if (audio_passthrough_db.llapt_handle == NULL)
    {
        ret = 4;
        goto fail_create_llapt_path;
    }

    audio_passthrough_db.state       = AUDIO_PASSTHROUGH_SESSION_STATE_CREATED;
    audio_passthrough_db.curr_handle = audio_passthrough_db.apt_handle;

    return true;

fail_create_llapt_path:
fail_llapt_path_exist:
    audio_path_destory(audio_passthrough_db.apt_handle);
    audio_passthrough_db.apt_handle = NULL;
fail_create_apt_path:
fail_apt_path_exist:
    AUDIO_PRINT_ERROR1("audio_passthrough_create: failed %d ", -ret);
    return false;
}

bool audio_passthrough_destroy(void)
{
    if (audio_passthrough_db.apt_handle != NULL)
    {
        audio_path_destory(audio_passthrough_db.apt_handle);
        audio_passthrough_db.apt_handle = NULL;
    }

    if (audio_passthrough_db.llapt_handle != NULL)
    {
        audio_path_destory(audio_passthrough_db.llapt_handle);
        audio_passthrough_db.llapt_handle = NULL;
    }

    audio_passthrough_db.state       = AUDIO_PASSTHROUGH_SESSION_STATE_RELEASED;
    audio_passthrough_db.curr_handle = NULL;

    return true;
}

bool audio_passthrough_enable(T_AUDIO_PASSTHROUGH_MODE mode, uint8_t llapt_scenario_id)
{
    bool ret = false;
    T_ANC_LLAPT_CFG config;

    audio_passthrough_db.mode              = mode;
    audio_passthrough_db.llapt_scenario_id = llapt_scenario_id;

    if (audio_passthrough_db.mode == AUDIO_PASSTHROUGH_MODE_NORMAL)
    {
        audio_passthrough_db.curr_handle = audio_passthrough_db.apt_handle;
    }
    else
    {
        audio_passthrough_db.curr_handle = audio_passthrough_db.llapt_handle;
    }

    if ((audio_passthrough_db.state !=  AUDIO_PASSTHROUGH_SESSION_STATE_STARTED) &&
        (audio_passthrough_db.state != AUDIO_PASSTHROUGH_SESSION_STATE_STARTING))
    {

        config.sub_type = ANC_IMAGE_SUB_TYPE_APT_IDLE_COEF;
        config.scenario_id = audio_passthrough_db.llapt_scenario_id;
        audio_path_cfg_set(audio_passthrough_db.curr_handle, &config);

        ret = audio_path_start(audio_passthrough_db.curr_handle);
        if (ret == true)
        {
            audio_passthrough_db.state = AUDIO_PASSTHROUGH_SESSION_STATE_STARTING;
        }
        else
        {
            audio_passthrough_db.state = AUDIO_PASSTHROUGH_SESSION_STATE_START_FAIL;
        }
    }

    AUDIO_PRINT_INFO5("audio_passthrough_enable: handle %p, ret %02d, state %02x, mode %d, llapt scenario %d",
                      audio_passthrough_db.curr_handle, ret, audio_passthrough_db.state, mode, llapt_scenario_id);

    return ret;
}

bool audio_passthrough_disable(void)
{
    if ((audio_passthrough_db.state != AUDIO_PASSTHROUGH_SESSION_STATE_STOPED) &&
        (audio_passthrough_db.state != AUDIO_PASSTHROUGH_SESSION_STATE_STOPPING))
    {
        if (audio_path_stop(audio_passthrough_db.curr_handle))
        {
            audio_passthrough_db.state = AUDIO_PASSTHROUGH_SESSION_STATE_STOPPING;
        }
        else
        {
            audio_passthrough_db.state = AUDIO_PASSTHROUGH_SESSION_STATE_STOP_FAIL;
        }
    }

    AUDIO_PRINT_INFO2("audio_passthrough_disable: handle %p, state %02x",
                      audio_passthrough_db.curr_handle, audio_passthrough_db.state);

    return true;
}

T_AUDIO_PASSTHROUGH_STATE audio_passthrough_state_get(void)
{
    T_AUDIO_PASSTHROUGH_STATE state;

    state = AUDIO_PASSTHROUGH_STATE_RELEASED;

    if (audio_passthrough_db.curr_handle != NULL)
    {
        switch (audio_passthrough_db.state)
        {
        case AUDIO_PASSTHROUGH_SESSION_STATE_RELEASED:
            state = AUDIO_PASSTHROUGH_STATE_RELEASED;
            break;

        case AUDIO_PASSTHROUGH_SESSION_STATE_CREATED:
            state = AUDIO_PASSTHROUGH_STATE_CREATED;
            break;

        case AUDIO_PASSTHROUGH_SESSION_STATE_STARTED:
        case AUDIO_PASSTHROUGH_SESSION_STATE_STOP_FAIL:
            state = AUDIO_PASSTHROUGH_STATE_STARTED;
            break;

        case AUDIO_PASSTHROUGH_SESSION_STATE_STARTING:
            state = AUDIO_PASSTHROUGH_STATE_STARTING;
            break;

        case AUDIO_PASSTHROUGH_SESSION_STATE_STOPED:
        case AUDIO_PASSTHROUGH_SESSION_STATE_START_FAIL:
            state = AUDIO_PASSTHROUGH_STATE_STOPPED;
            break;

        case AUDIO_PASSTHROUGH_SESSION_STATE_STOPPING:
            state = AUDIO_PASSTHROUGH_STATE_STOPPING;
            break;
        }
    }

    return state;
}

uint8_t audio_passthrough_volume_out_max_get(void)
{
    return audio_passthrough_db.volume_out_level_max;
}

bool audio_passthrough_volume_out_max_set(uint8_t level)
{
    audio_passthrough_db.volume_out_level_max = level;
    return true;
}

uint8_t audio_passthrough_volume_out_min_get(void)
{
    return audio_passthrough_db.volume_out_level_min;
}

bool audio_passthrough_volume_out_min_set(uint8_t level)
{
    audio_passthrough_db.volume_out_level_min = level;
    return true;
}

uint8_t audio_passthrough_volume_out_get(void)
{
    return audio_passthrough_db.volume_out_level;
}

bool audio_passthrough_volume_out_set(uint8_t level)
{
    if (level >= audio_passthrough_db.volume_out_level_min &&
        level <= audio_passthrough_db.volume_out_level_max)
    {
        audio_passthrough_db.volume_out_level = level;

        if (audio_passthrough_db.volume_out_muted == false)
        {
            return audio_path_dac_level_set(audio_passthrough_db.curr_handle,
                                            level,
                                            audio_passthrough_db.volume_out_scale);
        }

        return true;
    }

    return false;
}

bool audio_passthrough_volume_out_mute(void)
{
    if (audio_passthrough_db.volume_out_muted == false)
    {
        audio_passthrough_db.volume_out_muted = true;
        return audio_path_dac_mute(audio_passthrough_db.curr_handle);
    }

    return false;
}

bool audio_passthrough_volume_out_unmute(void)
{
    if (audio_passthrough_db.volume_out_muted == true)
    {
        audio_passthrough_db.volume_out_muted = false;
        return audio_path_dac_level_set(audio_passthrough_db.curr_handle,
                                        audio_passthrough_db.volume_out_level,
                                        audio_passthrough_db.volume_out_scale);
    }

    return false;
}

bool audio_passthrough_volume_out_is_muted(void)
{
    return audio_passthrough_db.volume_out_muted;
}

uint8_t audio_passthrough_volume_in_max_get(void)
{
    return audio_passthrough_db.volume_in_level_max;
}

bool audio_passthrough_volume_in_max_set(uint8_t level)
{
    audio_passthrough_db.volume_in_level_max = level;
    return true;
}

uint8_t audio_passthrough_volume_in_min_get(void)
{
    return audio_passthrough_db.volume_in_level_min;
}

bool audio_passthrough_volume_in_min_set(uint8_t level)
{
    audio_passthrough_db.volume_in_level_min = level;
    return true;
}

uint8_t audio_passthrough_volume_in_get(void)
{
    return audio_passthrough_db.volume_in_level;
}

bool audio_passthrough_volume_in_set(uint8_t level)
{
    if (level >= audio_passthrough_db.volume_in_level_min &&
        level <= audio_passthrough_db.volume_in_level_max)
    {
        audio_passthrough_db.volume_in_level = level;

        if (audio_passthrough_db.volume_in_muted == false)
        {
            return audio_path_adc_level_set(audio_passthrough_db.curr_handle,
                                            level,
                                            0.0f);
        }

        return true;
    }

    return false;
}

bool audio_passthrough_volume_in_mute(void)
{
    if (audio_passthrough_db.volume_in_muted == false)
    {
        audio_passthrough_db.volume_in_muted = true;
        return audio_path_adc_mute(audio_passthrough_db.curr_handle);
    }

    return false;
}

bool audio_passthrough_volume_in_unmute(void)
{
    if (audio_passthrough_db.volume_in_muted == true)
    {
        audio_passthrough_db.volume_in_muted = false;
        return audio_path_adc_level_set(audio_passthrough_db.curr_handle,
                                        audio_passthrough_db.volume_in_level,
                                        0.0f);
    }

    return false;
}

bool audio_passthrough_volume_in_is_muted(void)
{
    return audio_passthrough_db.volume_in_muted;
}

float audio_passthrough_volume_balance_get(void)
{
    return audio_passthrough_db.volume_out_scale;
}

bool audio_passthrough_volume_balance_set(float scale)
{
    if (scale >= -1.0f && scale <= 1.0f)
    {
        audio_passthrough_db.volume_out_scale = scale;
        audio_path_dac_level_set(audio_passthrough_db.curr_handle,
                                 audio_passthrough_db.volume_out_level,
                                 audio_passthrough_db.volume_out_scale);
        return true;
    }

    return false;
}

bool audio_passthrough_brightness_set(float strength)
{
    if (strength >= audio_passthrough_db.brightness_strength_min &&
        strength <= audio_passthrough_db.brightness_strength_max)
    {
        audio_passthrough_db.brightness_strength = strength;

        if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
        {
            audio_path_brightness_set(audio_passthrough_db.llapt_handle, strength);
        }
        return true;
    }
    return false;
}

bool audio_passthrough_ovp_enable(uint8_t level)
{
    if (audio_passthrough_db.ovp_enable == false)
    {
        if (audio_passthrough_db.ovp_level <= AUDIO_PASSTHROUGH_OVP_LEVEL_MAX)
        {
            audio_passthrough_db.ovp_enable = true;
            audio_passthrough_db.ovp_level  = level;

            if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
            {
                dsp_ipc_apt_ovp_set(1, level);
            }

            return true;
        }
    }

    return false;
}

bool audio_passthrough_ovp_set(uint8_t level)
{
    if (audio_passthrough_db.ovp_level <= AUDIO_PASSTHROUGH_OVP_LEVEL_MAX)
    {
        audio_passthrough_db.ovp_level  = level;

        if (audio_passthrough_db.ovp_enable == true)
        {
            if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
            {
                dsp_ipc_apt_ovp_set(1, level);
            }
        }

        return true;
    }

    return false;
}

bool audio_passthrough_ovp_disable(void)
{
    if (audio_passthrough_db.ovp_enable == true)
    {
        audio_passthrough_db.ovp_enable = false;
        audio_passthrough_db.ovp_level  = AUDIO_PASSTHROUGH_OVP_LEVEL_DEFAULT;

        if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
        {
            dsp_ipc_apt_ovp_set(0, AUDIO_PASSTHROUGH_OVP_LEVEL_DEFAULT);
        }

        return true;
    }

    return false;
}

bool audio_passthrough_beamforming_enable(uint8_t direction)
{
    if (audio_passthrough_db.beamforming_enable == false)
    {
        audio_passthrough_db.beamforming_enable    = true;
        audio_passthrough_db.beamforming_direction = direction;

        if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
        {
            dsp_ipc_apt_beamforming_set(1, direction);
        }

        return true;
    }

    return false;
}

bool audio_passthrough_beamforming_set(uint8_t direction)
{
    audio_passthrough_db.beamforming_direction = direction;

    if (audio_passthrough_db.beamforming_enable == true)
    {
        if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
        {
            dsp_ipc_apt_beamforming_set(1, direction);
        }
    }

    return true;
}

bool audio_passthrough_beamforming_disable(void)
{
    if (audio_passthrough_db.beamforming_enable == true)
    {
        audio_passthrough_db.beamforming_enable    = false;
        audio_passthrough_db.beamforming_direction = 0;

        if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
        {
            dsp_ipc_apt_beamforming_set(0, 0);
        }

        return true;
    }

    return false;
}

static bool audio_passthrough_effect_check(T_AUDIO_EFFECT_INSTANCE instance)
{
    if (instance == NULL)
    {
        return false;
    }

    if (os_queue_search(&audio_passthrough_db.effects, instance) == true)
    {
        return true;
    }

    return false;
}

static bool audio_passthrough_effect_enqueue(T_AUDIO_EFFECT_INSTANCE instance)
{
    if (instance == NULL)
    {
        return false;
    }

    if (os_queue_search(&audio_passthrough_db.effects, instance) == true)
    {
        return true;
    }

    os_queue_in(&audio_passthrough_db.effects, instance);
    return true;
}

static bool audio_passthrough_effect_dequeue(T_AUDIO_EFFECT_INSTANCE instance)
{
    if (instance == NULL)
    {
        return false;
    }

    return os_queue_delete(&audio_passthrough_db.effects, instance);
}

static bool audio_passthrough_effect_run(void)
{
    T_AUDIO_EFFECT_INSTANCE instance;
    int32_t                 i = 0;

    while ((instance = os_queue_peek(&audio_passthrough_db.effects, i)) != NULL)
    {
        audio_effect_run(instance);
        i++;
    }

    return true;
}

static void audio_passthrough_effect_cback(T_AUDIO_EFFECT_INSTANCE instance,
                                           T_AUDIO_EFFECT_EVENT    event)
{
    T_AUDIO_PATH_HANDLE handle;

    handle = audio_effect_owner_get(instance);

    AUDIO_PRINT_TRACE3("audio_passthrough_effect_cback: instance %p, handle %p, event 0x%02x",
                       instance, handle, event);

    if (handle == audio_passthrough_db.apt_handle)
    {
        switch (event)
        {
        case AUDIO_EFFECT_EVENT_CREATED:
            break;

        case AUDIO_EFFECT_EVENT_ENABLED:
            if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
            {
                audio_effect_run(instance);
            }
            else if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTING)
            {
                if (audio_passthrough_db.effect_apply == true)
                {
                    audio_effect_run(instance);
                }
            }
            break;

        case AUDIO_EFFECT_EVENT_DISABLED:
            if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
            {
                audio_effect_stop(instance);
            }
            else if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTING)
            {
                if (audio_passthrough_db.effect_apply == true)
                {
                    audio_effect_stop(instance);
                }
            }
            break;

        case AUDIO_EFFECT_EVENT_UPDATED:
            if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
            {
                audio_effect_run(instance);
            }
            else if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTING)
            {
                if (audio_passthrough_db.effect_apply == true)
                {
                    audio_effect_run(instance);
                }
            }
            break;

        case AUDIO_EFFECT_EVENT_RELEASED:
            if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
            {
                audio_effect_stop(instance);
            }
            else if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTING)
            {
                if (audio_passthrough_db.effect_apply == true)
                {
                    audio_effect_stop(instance);
                }
            }

            audio_passthrough_effect_dequeue(instance);
            audio_effect_owner_clear(instance);
            break;
        }
    }
}

bool audio_passthrough_effect_attach(T_AUDIO_EFFECT_INSTANCE instance)
{
    int32_t ret = 0;

    if (audio_passthrough_effect_enqueue(instance) == false)
    {
        ret = 1;
        goto fail_enqueue_effect;
    }

    if (audio_effect_owner_set(instance,
                               audio_passthrough_db.apt_handle,
                               audio_passthrough_effect_cback) == false)
    {
        ret = 2;
        goto fail_set_owner;
    }

    if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
    {
        if (audio_passthrough_db.mode == AUDIO_PASSTHROUGH_MODE_NORMAL)
        {
            audio_effect_run(instance);
        }
    }

    return true;

fail_set_owner:
    audio_passthrough_effect_dequeue(instance);
fail_enqueue_effect:
    AUDIO_PRINT_ERROR2("audio_passthrough_effect_attach: instance %p, failed %d",
                       instance, -ret);
    return false;
}

bool audio_passthrough_effect_detach(T_AUDIO_EFFECT_INSTANCE instance)
{
    int32_t ret = 0;

    if (audio_passthrough_effect_check(instance) == false)
    {
        ret = 1;
        goto fail_check_instance;
    }

    if (audio_effect_owner_clear(instance) == false)
    {
        ret = 2;
        goto fail_clear_owner;
    }

    if (audio_passthrough_effect_dequeue(instance) == false)
    {
        ret = 3;
        goto fail_dequeue_effect;
    }

    if (audio_passthrough_db.state == AUDIO_PASSTHROUGH_SESSION_STATE_STARTED)
    {
        audio_effect_stop(instance);
    }

    return true;

fail_dequeue_effect:
    audio_effect_owner_set(instance,
                           audio_passthrough_db.apt_handle,
                           audio_passthrough_effect_cback);
fail_clear_owner:
fail_check_instance:
    AUDIO_PRINT_ERROR2("audio_passthrough_effect_detach: instance %p, failed %d",
                       instance, -ret);
    return false;
}
