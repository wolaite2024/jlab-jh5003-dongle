#ifndef _APP_HID_RTK_CTRL_H_
#define _APP_HID_RTK_CTRL_H_
#include "stdint.h"
#include "app_msg.h"
#include "stdbool.h"
#include "app_cfg.h"

#if F_APP_HID_RTK_SUPPORT
void app_hid_rtk_ctrl_handle_send_msg(void);

bool app_hid_rtk_ctrl_send(uint16_t length, uint8_t *data);

void app_hid_rtk_recv_data(uint8_t *buf, uint16_t len);

void app_hid_rtk_init(void);

#endif

#endif
