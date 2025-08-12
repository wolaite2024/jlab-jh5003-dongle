/**
 * Copyright (c) 2017, Realtek Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_INTERVAL_H_
#define _AUDIO_INTERVAL_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    AUDIO_INTERVAL_EVAL_EVT_STABLE,
    AUDIO_INTERVAL_EVAL_EVT_UNSTABLE,
} T_AUDIO_INTERVAL_EVAL_EVT;

typedef void *T_AUDIO_INTERVAL_EVAL_HANDLE;
typedef void (*T_AUDIO_INTERVAL_EVAL_EVT_CBACK)(T_AUDIO_INTERVAL_EVAL_EVT event, void *param,
                                                void *owner);

bool audio_interval_eval_init(void);
void audio_interval_eval_deinit(void);
T_AUDIO_INTERVAL_EVAL_HANDLE audio_interval_evaluator_register(void *buffer_ent,
                                                               T_AUDIO_INTERVAL_EVAL_EVT_CBACK cback);
void audio_interval_evaluator_unregister(T_AUDIO_INTERVAL_EVAL_HANDLE handle);
void audio_inteval_evaluator_update(T_AUDIO_INTERVAL_EVAL_HANDLE handle, uint16_t seq,
                                    uint32_t clk);
bool audio_interval_get_stable_state(T_AUDIO_INTERVAL_EVAL_HANDLE handle, bool *stable);
bool audio_inteval_evaluator_get_var(T_AUDIO_INTERVAL_EVAL_HANDLE handle, float *var);
bool audio_interval_evaluator_change_sensitivity(T_AUDIO_INTERVAL_EVAL_HANDLE handle,
                                                 uint8_t sens_level);

#ifdef __cplusplus
}
#endif

#endif /* #define _AUDIO_INTERVAL_H_ */
