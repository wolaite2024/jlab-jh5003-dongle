/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdbool.h>

#include "os_mem.h"
#include "os_queue.h"
#include "audio_effect.h"

#define EFFECT_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr)-(size_t)(&((type *)0)->member)))

typedef struct t_audio_effect
{
    struct t_audio_effect  *p_next;
    T_OS_QUEUE_ELEM         list;
    void                   *owner;
    void                   *inheritor;
    P_AUDIO_EFFECT_CBACK    cback;
    bool                    enabled;
    P_AUDIO_EFFECT_ENABLE   enable;
    P_AUDIO_EFFECT_DISABLE  disable;
} T_AUDIO_EFFECT;

typedef struct t_audio_effect_db
{
    T_OS_QUEUE             instances;
} T_AUDIO_EFFECT_DB;

static T_AUDIO_EFFECT_DB audio_effect_db;

bool audio_effect_init(void)
{
    os_queue_init(&audio_effect_db.instances);
    return true;
}

void audio_effect_deinit(void)
{
    T_OS_QUEUE_ELEM *item;
    T_AUDIO_EFFECT  *effect;

    item = os_queue_out(&audio_effect_db.instances);
    while (item != NULL)
    {
        effect = EFFECT_CONTAINER_OF(item, T_AUDIO_EFFECT, list);
        item = item->p_next;
        /* TODO release base */
        os_mem_free(effect);
    }
}

T_AUDIO_EFFECT_INSTANCE audio_effect_create(P_AUDIO_EFFECT_ENABLE  enable,
                                            P_AUDIO_EFFECT_DISABLE disable)
{
    T_AUDIO_EFFECT *effect;

    effect = os_mem_alloc2(sizeof(T_AUDIO_EFFECT));
    if (effect != NULL)
    {
        effect->owner     = NULL;
        effect->inheritor = NULL;
        effect->cback     = NULL;
        effect->enabled   = false;
        effect->enable    = enable;
        effect->disable   = disable;

        os_queue_in(&audio_effect_db.instances, &effect->list);
    }

    return effect;
}

bool audio_effect_release(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        if (effect->cback != NULL)
        {
            effect->cback(instance, AUDIO_EFFECT_EVENT_RELEASED);
        }

        os_queue_delete(&audio_effect_db.instances, &effect->list);
        os_mem_free(effect);
        return true;
    }

    return false;
}

bool audio_effect_owner_set(T_AUDIO_EFFECT_INSTANCE  instance,
                            void                    *owner,
                            P_AUDIO_EFFECT_CBACK     cback)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        if (effect->owner == NULL &&
            owner         != NULL &&
            cback         != NULL)
        {
            effect->owner = owner;
            effect->cback = cback;

            effect->cback(instance, AUDIO_EFFECT_EVENT_CREATED);
            return true;
        }
    }

    return false;
}

void *audio_effect_owner_get(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        return effect->owner;
    }

    return NULL;
}

bool audio_effect_owner_clear(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        effect->owner = NULL;
        effect->cback = NULL;
        return true;
    }

    return false;
}

bool audio_effect_inheritor_set(T_AUDIO_EFFECT_INSTANCE  instance,
                                void                    *inheritor)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        if (effect->inheritor == NULL &&
            inheritor != NULL)
        {
            effect->inheritor = inheritor;
            return true;
        }
    }

    return false;
}

void *audio_effect_inheritor_get(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        return effect->inheritor;
    }

    return NULL;
}

bool audio_effect_inheritor_clear(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        effect->inheritor = NULL;
        return true;
    }

    return false;
}

bool audio_effect_enable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        if (effect->enabled == false)
        {
            effect->enabled = true;

            if (effect->cback != NULL)
            {
                effect->cback(instance, AUDIO_EFFECT_EVENT_ENABLED);
            }
        }

        return true;
    }

    return false;
}

bool audio_effect_disable(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        if (effect->enabled == true)
        {
            if (effect->cback != NULL)
            {
                effect->cback(instance, AUDIO_EFFECT_EVENT_DISABLED);
            }

            effect->enabled = false;
        }

        return true;
    }

    return false;
}

bool audio_effect_update(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        if (effect->enabled == true)
        {
            if (effect->cback != NULL)
            {
                effect->cback(instance, AUDIO_EFFECT_EVENT_UPDATED);
            }
        }

        return true;
    }

    return false;
}

bool audio_effect_run(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        if (effect->enabled == true)
        {
            if (effect->enable != NULL)
            {
                return effect->enable(instance);
            }
        }

        return true;
    }

    return false;
}

bool audio_effect_stop(T_AUDIO_EFFECT_INSTANCE instance)
{
    T_AUDIO_EFFECT *effect;

    effect = (T_AUDIO_EFFECT *)instance;
    if (os_queue_search(&audio_effect_db.instances, &effect->list) == true)
    {
        if (effect->enabled == true)
        {
            if (effect->disable != NULL)
            {
                return effect->disable(instance);
            }
        }

        return true;
    }

    return false;
}
