/*
 *  Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 */

#ifndef __LE_AUDIO_WRAPPER_H__
#define __LE_AUDIO_WRAPPER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int le_audio_cis_enter(void);
int le_audio_cis_exit(void);
int le_audio_bis_enter(void);
int le_audio_bis_exit(void);
int le_audio_mmi_cis(uint8_t mmi_action, void *params);
int le_audio_mmi_bis(uint8_t mmi_action, void *params);
int le_audio_init(void *evt_queue, void *msg_queue);

uint8_t le_audio_bis_get_status(void);
uint8_t le_audio_cis_get_status(void);

#if F_APP_LEA_DONGLE_BINDING
void le_audio_power_on_start_scan(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __LE_AUDIO_WRAPPER_H__ */

