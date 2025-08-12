/*
 *  Copyright (C) 2020 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#ifndef _APP_SRC_AVRCP_H_
#define _APP_SRC_AVRCP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>

void app_src_avrcp_init(void);
bool app_src_avrcp_set_spk_vol(uint8_t vol);
void app_src_avrcp_set_spk_mute(bool mute);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SRC_AVRCP_H_ */
