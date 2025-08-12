/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _SPORT_MGR_H_
#define _SPORT_MGR_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void *T_SPORT_MGR_SESSION_HANDLE;

bool sport_mgr_init(void);

void sport_mgr_deinit(void);

T_SPORT_MGR_SESSION_HANDLE sport_mgr_session_create(T_AUDIO_CATEGORY category,
                                                    uint8_t          tx_chann_num,
                                                    uint8_t          rx_chann_num,
                                                    uint32_t         tx_sample_rate,
                                                    uint32_t         rx_sample_rate);

bool sport_mgr_destroy(T_SPORT_MGR_SESSION_HANDLE handle);

bool sport_mgr_enable(T_SPORT_MGR_SESSION_HANDLE handle);

bool sport_mgr_disable(T_SPORT_MGR_SESSION_HANDLE handle);

uint32_t sport_mgr_sample_rate_get(uint32_t sport_handle, uint32_t sport_path);

uint32_t sport_mgr_tx_sample_rate_get(T_SPORT_MGR_SESSION_HANDLE handle);

uint32_t sport_mgr_rx_sample_rate_get(T_SPORT_MGR_SESSION_HANDLE handle);

uint8_t sport_mgr_tx_channel_mode_get(T_SPORT_MGR_SESSION_HANDLE handle);

uint8_t sport_mgr_rx_channel_mode_get(T_SPORT_MGR_SESSION_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SPORT_MGR_H_ */
