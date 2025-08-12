/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_HFP_AG_H_
#define _APP_HFP_AG_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_HFP_AG App HFP AG
  * @brief This file handle HFP AG profile related process
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_HFP_AG_Exported_Functions App HFP Ag Functions
    * @{
    */
bool app_src_hfp_set_spk_vol(int16_t vol);

typedef void (*APP_USB_TELEPHONY_CB)(uint8_t *data, uint8_t length);
void app_usb_telephony_register_cb(APP_USB_TELEPHONY_CB telephony_state_cb);

/**
    * @brief  HFP AG module init.
    * @param  void
    * @return void
    */
void app_hfp_ag_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_HFP_AG_H_ */
