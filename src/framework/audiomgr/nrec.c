/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdbool.h>
#include <stdint.h>

#include "os_mem.h"
#include "trace.h"
#include "nrec.h"
#include "audio_effect.h"

/* TODO Remove Start */
#include "dsp_ipc.h"
/* TODO Remove End */

#define NREC_LEVEL_MIN     0
#define NREC_LEVEL_MAX     15
#define NREC_LEVEL_DEFAULT 10

typedef struct t_nrec_instance
{
    T_AUDIO_EFFECT_INSTANCE  base;
    T_NREC_CONTENT_TYPE      type;
    T_NREC_MODE              mode;
    uint8_t                  level;
    bool                     enabled;
} T_NREC_INSTANCE;

static bool nrec_effect_enable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_NREC_INSTANCE *nrec;
    int32_t          ret = 0;

    nrec = audio_effect_inheritor_get(instance);
    if (nrec == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (nrec->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    switch (nrec->type)
    {
    case NREC_CONTENT_TYPE_VOICE:
    case NREC_CONTENT_TYPE_RECORD:
        {
            if (dsp_ipc_voice_nrec_set(1) == false)
            {
                ret = 3;
                goto fail_set_ipc_voice_nrec;
            }
        }
        break;

    case NREC_CONTENT_TYPE_PASSTHROUGH:
        {
            if (dsp_ipc_apt_nrec_set(1, nrec->mode, nrec->level) == false)
            {
                ret = 4;
                goto fail_set_ipc_apt_nrec;
            }
        }
        break;

    default:
        {
            ret = 5;
            goto fail_get_nrec_type;
        }
    }

    return true;

fail_get_nrec_type:
fail_set_ipc_apt_nrec:
fail_set_ipc_voice_nrec:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("nrec_effect_enable: failed %d", -ret);
    return false;
}

static bool nrec_effect_disable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_NREC_INSTANCE *nrec;
    int32_t          ret = 0;

    nrec = audio_effect_inheritor_get(instance);
    if (nrec == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (nrec->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    switch (nrec->type)
    {
    case NREC_CONTENT_TYPE_VOICE:
    case NREC_CONTENT_TYPE_RECORD:
        {
            if (dsp_ipc_voice_nrec_set(0) == false)
            {
                ret = 3;
                goto fail_set_ipc_voice_nrec;
            }
        }
        break;

    case NREC_CONTENT_TYPE_PASSTHROUGH:
        {
            if (dsp_ipc_apt_nrec_set(0, nrec->mode, nrec->level) == false)
            {
                ret = 4;
                goto fail_set_ipc_apt_nrec;
            }
        }
        break;

    default:
        {
            ret = 5;
            goto fail_get_nrec_type;
        }
    }

    return true;

fail_get_nrec_type:
fail_set_ipc_apt_nrec:
fail_set_ipc_voice_nrec:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("nrec_effect_disable: failed %d", -ret);
    return false;
}

T_AUDIO_EFFECT_INSTANCE nrec_create(T_NREC_CONTENT_TYPE type, T_NREC_MODE mode, uint8_t level)
{
    T_NREC_INSTANCE *nrec;
    int32_t        ret = 0;

    nrec = os_mem_alloc2(sizeof(T_NREC_INSTANCE));
    if (nrec == NULL)
    {
        ret = 1;
        goto fail_alloc_nrec;
    }

    nrec->enabled = false;
    nrec->type    = type;
    nrec->mode    = mode;
    nrec->level   = level;

    nrec->base = audio_effect_create(nrec_effect_enable, nrec_effect_disable);
    if (nrec->base == NULL)
    {
        ret = 2;
        goto fail_create_effect;
    }

    if (audio_effect_inheritor_set(nrec->base, nrec) == false)
    {
        ret = 3;
        goto fail_set_inheritor;
    }

    AUDIO_PRINT_TRACE2("nrec_create: type %d, instance %p", nrec->type, nrec->base);

    return nrec->base;

fail_set_inheritor:
    audio_effect_release(nrec->base);
fail_create_effect:
    os_mem_free(nrec);
fail_alloc_nrec:
    AUDIO_PRINT_ERROR1("nrec_create: failed %d", -ret);
    return NULL;
}

bool nrec_enable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_NREC_INSTANCE *nrec;
    int32_t          ret = 0;

    AUDIO_PRINT_TRACE1("nrec_enable: instance %p", instance);

    nrec = audio_effect_inheritor_get(instance);
    if (nrec == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (nrec->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (nrec->enabled == true)
    {
        ret = 3;
        goto fail_check_state;
    }

    if (audio_effect_enable(nrec->base) == false)
    {
        ret = 4;
        goto fail_enable_effect;
    }

    nrec->enabled = true;
    return true;

fail_enable_effect:
fail_check_state:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("nrec_enable: failed %d", -ret);
    return false;
}

bool nrec_disable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_NREC_INSTANCE *nrec;
    int32_t          ret = 0;

    AUDIO_PRINT_TRACE1("nrec_disable: instance %p", instance);

    nrec = audio_effect_inheritor_get(instance);
    if (nrec == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (nrec->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (nrec->enabled == false)
    {
        ret = 3;
        goto fail_check_state;
    }

    if (audio_effect_disable(nrec->base) == false)
    {
        ret = 4;
        goto fail_disable_effect;
    }

    nrec->enabled = false;
    return true;

fail_disable_effect:
fail_check_state:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("nrec_disable: failed %d", -ret);
    return false;
}

bool nrec_release(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_NREC_INSTANCE *nrec;
    int32_t          ret = 0;

    AUDIO_PRINT_TRACE1("nrec_release: instance %p", instance);

    nrec = audio_effect_inheritor_get(instance);
    if (nrec == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (nrec->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (audio_effect_release(nrec->base) == false)
    {
        ret = 3;
        goto fail_release_effect;
    }

    os_mem_free(nrec);
    return true;

fail_release_effect:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("nrec_release: failed %d", -ret);
    return false;
}

bool nrec_mode_set(T_AUDIO_EFFECT_INSTANCE instance, T_NREC_MODE mode)
{
    T_NREC_INSTANCE *nrec;
    int32_t          ret = 0;

    AUDIO_PRINT_TRACE2("nrec_mode_set: instance %p, mode %d", instance, mode);

    nrec = audio_effect_inheritor_get(instance);
    if (nrec == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (nrec->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    nrec->mode = mode;

    if (nrec->enabled == true)
    {
        if (audio_effect_update(nrec->base) == false)
        {
            ret = 3;
            goto fail_update_effect;
        }
    }

    return true;

fail_update_effect:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("nrec_mode_set: failed %d", -ret);
    return false;
}

bool nrec_level_set(T_AUDIO_EFFECT_INSTANCE instance, uint8_t level)
{
    T_NREC_INSTANCE *nrec;
    int32_t          ret = 0;

    AUDIO_PRINT_TRACE2("nrec_level_set: instance %p, level %d", instance, level);

    nrec = audio_effect_inheritor_get(instance);
    if (nrec == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (nrec->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (level > NREC_LEVEL_MAX)
    {
        ret = 3;
        goto fail_check_level;
    }

    nrec->level = level;

    if (nrec->enabled == true)
    {
        if (audio_effect_update(nrec->base) == false)
        {
            ret = 4;
            goto fail_update_effect;
        }
    }

    return true;

fail_update_effect:
fail_check_level:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("nrec_level_set: failed %d", -ret);
    return false;
}
