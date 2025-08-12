/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_EFFECT_H_
#define _AUDIO_EFFECT_H_

#include <stdbool.h>
#include <stdint.h>

#include "audio_type.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_audio_effect_event
{
    AUDIO_EFFECT_EVENT_CREATED  = 0x00,
    AUDIO_EFFECT_EVENT_ENABLED  = 0x01,
    AUDIO_EFFECT_EVENT_DISABLED = 0x02,
    AUDIO_EFFECT_EVENT_UPDATED  = 0x03,
    AUDIO_EFFECT_EVENT_RELEASED = 0x04,
} T_AUDIO_EFFECT_EVENT;

typedef void (*P_AUDIO_EFFECT_CBACK)(T_AUDIO_EFFECT_INSTANCE instance, T_AUDIO_EFFECT_EVENT event);

typedef bool (*P_AUDIO_EFFECT_ENABLE)(T_AUDIO_EFFECT_INSTANCE instance);

typedef bool (*P_AUDIO_EFFECT_DISABLE)(T_AUDIO_EFFECT_INSTANCE instance);

T_AUDIO_EFFECT_INSTANCE audio_effect_create(P_AUDIO_EFFECT_ENABLE  enable,
                                            P_AUDIO_EFFECT_DISABLE disable);

bool audio_effect_release(T_AUDIO_EFFECT_INSTANCE instance);

bool audio_effect_owner_set(T_AUDIO_EFFECT_INSTANCE  instance,
                            void                    *owner,
                            P_AUDIO_EFFECT_CBACK     cback);

void *audio_effect_owner_get(T_AUDIO_EFFECT_INSTANCE instance);

bool audio_effect_owner_clear(T_AUDIO_EFFECT_INSTANCE instance);

bool audio_effect_inheritor_set(T_AUDIO_EFFECT_INSTANCE  instance,
                                void                    *inheritor);

void *audio_effect_inheritor_get(T_AUDIO_EFFECT_INSTANCE instance);

bool audio_effect_inheritor_clear(T_AUDIO_EFFECT_INSTANCE instance);

bool audio_effect_enable(T_AUDIO_EFFECT_INSTANCE instance);

bool audio_effect_disable(T_AUDIO_EFFECT_INSTANCE instance);

bool audio_effect_update(T_AUDIO_EFFECT_INSTANCE instance);

bool audio_effect_run(T_AUDIO_EFFECT_INSTANCE instance);

bool audio_effect_stop(T_AUDIO_EFFECT_INSTANCE instance);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_EFFECT_H_ */
