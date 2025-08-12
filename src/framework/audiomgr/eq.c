/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "os_mem.h"
#include "trace.h"
#include "audio_effect.h"
#include "eq.h"

/* TODO Remove Start */
#include "dsp_ipc.h"
/* TODO Remove End */

typedef struct t_eq_instance
{
    T_AUDIO_EFFECT_INSTANCE  base;
    void                    *info_buf;
    uint16_t                 info_len;
    T_EQ_CONTENT_TYPE        type;
    bool                     enabled;
} T_EQ_INSTANCE;

static bool eq_effect_enable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_EQ_INSTANCE *eq;
    int32_t        ret = 0;

    eq = audio_effect_inheritor_get(instance);
    if (eq == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (eq->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (eq->info_buf != NULL &&
        eq->info_len != 0)
    {
        switch (eq->type)
        {
        case EQ_CONTENT_TYPE_AUDIO:
            {
                if (dsp_ipc_audio_eq_set(eq->info_buf, eq->info_len) == false)
                {
                    ret = 3;
                    goto fail_set_ipc_audio_eq;
                }
            }
            break;

        case EQ_CONTENT_TYPE_VOICE:
            {
                if (dsp_ipc_voice_eq_set(eq->info_buf, eq->info_len) == false)
                {
                    ret = 4;
                    goto fail_set_ipc_voice_eq;
                }
            }
            break;

        case EQ_CONTENT_TYPE_RECORD:
            {
                if (dsp_ipc_record_eq_set(eq->info_buf, eq->info_len) == false)
                {
                    ret = 5;
                    goto fail_set_ipc_record_eq;
                }
            }
            break;

        case EQ_CONTENT_TYPE_PASSTHROUGH:
            {
                if (dsp_ipc_apt_eq_set(eq->info_buf, eq->info_len) == false)
                {
                    ret = 6;
                    goto fail_set_ipc_passthrough_eq;
                }
            }
            break;

        default:
            {
                ret = 7;
                goto fail_get_eq_type;
            }
        }
    }

    return true;

fail_get_eq_type:
fail_set_ipc_passthrough_eq:
fail_set_ipc_record_eq:
fail_set_ipc_voice_eq:
fail_set_ipc_audio_eq:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("eq_effect_enable: failed %d", -ret);
    return false;
}

static bool eq_effect_disable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_EQ_INSTANCE *eq;
    int32_t        ret = 0;

    eq = audio_effect_inheritor_get(instance);
    if (eq == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (eq->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    switch (eq->type)
    {
    case EQ_CONTENT_TYPE_AUDIO:
        {
            if (dsp_ipc_audio_eq_clear() == false)
            {
                ret = 3;
                goto fail_clear_ipc_audio_eq;
            }
        }
        break;

    case EQ_CONTENT_TYPE_VOICE:
        {
            if (dsp_ipc_voice_eq_clear() == false)
            {
                ret = 4;
                goto fail_clear_ipc_voice_eq;
            }
        }
        break;

    case EQ_CONTENT_TYPE_RECORD:
        {
            if (dsp_ipc_record_eq_clear() == false)
            {
                ret = 5;
                goto fail_clear_ipc_record_eq;
            }
        }
        break;

    case EQ_CONTENT_TYPE_PASSTHROUGH:
        {
            if (dsp_ipc_apt_eq_clear() == false)
            {
                ret = 6;
                goto fail_clear_ipc_passthrough_eq;
            }
        }
        break;

    default:
        {
            ret = 7;
            goto fail_get_eq_type;
        }
    }

    return true;

fail_get_eq_type:
fail_clear_ipc_passthrough_eq:
fail_clear_ipc_record_eq:
fail_clear_ipc_voice_eq:
fail_clear_ipc_audio_eq:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("eq_effect_disable: failed %d", -ret);
    return false;
}

T_AUDIO_EFFECT_INSTANCE eq_create(T_EQ_CONTENT_TYPE type, void *info_buf, uint16_t info_len)
{
    T_EQ_INSTANCE *eq;
    int32_t        ret = 0;

    eq = os_mem_alloc2(sizeof(T_EQ_INSTANCE));
    if (eq == NULL)
    {
        ret = 1;
        goto fail_alloc_eq;
    }

    eq->enabled = false;
    eq->type    = type;

    eq->base = audio_effect_create(eq_effect_enable, eq_effect_disable);
    if (eq->base == NULL)
    {
        ret = 2;
        goto fail_create_effect;
    }

    if (audio_effect_inheritor_set(eq->base, eq) == false)
    {
        ret = 3;
        goto fail_set_inheritor;
    }

    eq->info_buf = NULL;
    eq->info_len = 0;

    if (info_buf != NULL &&
        info_len != 0)
    {
        eq->info_buf = os_mem_alloc2(info_len);
        if (eq->info_buf == NULL)
        {
            ret = 4;
            goto fail_alloc_info;
        }

        memcpy(eq->info_buf, info_buf, info_len);
        eq->info_len = info_len;
    }

    AUDIO_PRINT_INFO4("eq_create: instance %p, type %u, info_buf %p, info_len %u",
                      eq->base, type, info_buf, info_len);

    return eq->base;

fail_alloc_info:
    audio_effect_inheritor_clear(eq->base);
fail_set_inheritor:
    audio_effect_release(eq->base);
fail_create_effect:
    os_mem_free(eq);
fail_alloc_eq:
    AUDIO_PRINT_ERROR1("eq_create: failed %d", -ret);
    return NULL;
}

bool eq_enable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_EQ_INSTANCE *eq;
    int32_t        ret = 0;

    AUDIO_PRINT_INFO1("eq_enable: instance %p", instance);

    eq = audio_effect_inheritor_get(instance);
    if (eq == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (eq->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (eq->enabled == true)
    {
        ret = 3;
        goto fail_check_state;
    }

    if (audio_effect_enable(eq->base) == false)
    {
        ret = 4;
        goto fail_enable_effect;
    }

    eq->enabled = true;
    return true;

fail_enable_effect:
fail_check_state:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("eq_enable: failed %d", -ret);
    return false;
}

bool eq_disable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_EQ_INSTANCE *eq;
    int32_t        ret = 0;

    AUDIO_PRINT_INFO1("eq_disable: instance %p", instance);

    eq = audio_effect_inheritor_get(instance);
    if (eq == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (eq->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (eq->enabled == false)
    {
        ret = 3;
        goto fail_check_state;
    }

    if (audio_effect_disable(eq->base) == false)
    {
        ret = 4;
        goto fail_disable_effect;
    }

    eq->enabled = false;
    return true;

fail_disable_effect:
fail_check_state:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("eq_disable: failed %d", -ret);
    return false;
}

bool eq_set(T_AUDIO_EFFECT_INSTANCE instance, void *info_buf, uint16_t info_len)
{
    T_EQ_INSTANCE *eq;
    int32_t        ret = 0;

    AUDIO_PRINT_INFO3("eq_set: instance %p, info_buf %p, info_len %u",
                      instance, info_buf, info_len);

    eq = audio_effect_inheritor_get(instance);
    if (eq == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (eq->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (eq->info_buf != NULL &&
        info_buf != NULL &&
        eq->info_len == info_len &&
        !memcmp(eq->info_buf, info_buf, eq->info_len))
    {
        return true;
    }

    if (eq->info_buf != NULL)
    {
        os_mem_free(eq->info_buf);
        eq->info_buf = NULL;
        eq->info_len = 0;
    }

    if (info_buf != NULL &&
        info_len != 0)
    {
        eq->info_buf = os_mem_alloc2(info_len);
        if (eq->info_buf == NULL)
        {
            ret = 3;
            goto fail_alloc_info;
        }

        memcpy(eq->info_buf, info_buf, info_len);
        eq->info_len = info_len;
    }

    if (eq->enabled == true)
    {
        if (audio_effect_update(eq->base) == false)
        {
            ret = 4;
            goto fail_update_effect;
        }
    }

    return true;

fail_update_effect:
fail_alloc_info:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("eq_set: failed %d", -ret);
    return false;
}

bool eq_release(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_EQ_INSTANCE *eq;
    int32_t        ret = 0;

    AUDIO_PRINT_INFO1("eq_release: instance %p", instance);

    eq = audio_effect_inheritor_get(instance);
    if (eq == NULL)
    {
        ret = 1;
        goto fail_get_inheritor;
    }

    if (eq->base != instance)
    {
        ret = 2;
        goto fail_check_base;
    }

    if (audio_effect_release(eq->base) == false)
    {
        ret = 3;
        goto fail_release_effect;
    }

    if (eq->info_buf != NULL)
    {
        os_mem_free(eq->info_buf);
    }

    os_mem_free(eq);
    return true;

fail_release_effect:
fail_check_base:
fail_get_inheritor:
    AUDIO_PRINT_ERROR1("eq_release: failed %d", -ret);
    return false;
}
