/*
 *  Copyright (C) 2020 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#ifndef __APP_SPP_AUDIO_H__
#define __APP_SPP_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void spp_audio_plc(void);
void spp_audio_assemble(uint8_t *bdaddr, uint8_t *buf, uint16_t len, bool is_fix_chann_data);
void app_spp_audio_init(void);
bool app_send_out_by_spp(uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __APP_SPP_AUDIO_H__ */
