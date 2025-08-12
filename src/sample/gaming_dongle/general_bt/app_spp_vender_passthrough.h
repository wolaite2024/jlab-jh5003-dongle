/*
 *  Copyright (C) 2020 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#ifndef __APP_SPP_VENDER_PASSTHROUGH__
#define __APP_SPP_VENDER_PASSTHROUGH__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//#define SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT

#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
#define RFC_VENDER_PASSTHROUGH_CHANN_NUM            18
#define APP_SPP_VENDER_PASSTHROUGH_PROFILE_MASK     0x00200000    /**< PRAHA DELEGATE profile bitmask */

void app_spp_vender_passthrough_init(void);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __APP_SRC_HS_CFU_H__ */
