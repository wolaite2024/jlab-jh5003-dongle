/*
 *  Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 */

#ifndef __GENERAL_AUDIO_WRAPPER_H__
#define __GENERAL_AUDIO_WRAPPER_H__

#define PTR_TO_UINT(p)  ((uint32_t)((void *)(p)))
#define UINT_TO_PTR(u)  ((void *)((uint32_t *)(u)))

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t general_get_status(uint8_t index);
int general_audio_enter(void);
void general_audio_exit(void);
int general_audio_init(void *evt_queue, void *msg_queue);
int general_audio_mmi(uint8_t mmi_action, void *params);

#ifdef __cplusplus
}
#endif

#endif /* __LEGACY_AUDIO_WRAPPER_H__ */
