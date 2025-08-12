/**
*****************************************************************************************
*     Copyright (C) 2021 Realtek Semiconductor Corporation.
*****************************************************************************************
  * @file
  * @brief
  * @details
  * @author
  * @date
  * @version
  ***************************************************************************************
  * @attention
  ***************************************************************************************
  */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/

#ifndef _APP_SPP_CMD_H_
#define _APP_SPP_CMD_H_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "stdint.h"
#include "stdbool.h"

/*============================================================================*
 *                         Macros
 *============================================================================*/


/*============================================================================*
 *                         Types
 *============================================================================*/

/*============================================================================*
 *                         Functions
 *============================================================================*/

void app_spp_cmd_init(void);

void app_spp_cmd_received(uint8_t *addr, uint8_t *data, uint16_t len);

bool app_spp_cfu_send(uint8_t *data, uint16_t len);

#if F_APP_CUSTOMIZED_SBC_VP_FLOW_SUPPORT
void app_spp_send_customized_vp_data(uint32_t addr, uint8_t *data, uint16_t len);
void app_spp_send_write_vp_finish(void);
void app_spp_erase_flash(uint8_t *addr);
#endif

#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
void app_spp_register_usb_pt_cb(void);
#endif

#ifdef __cplusplus
}
#endif

#endif  // _POLICY_CMD_H_

