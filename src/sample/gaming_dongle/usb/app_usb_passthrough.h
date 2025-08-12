#include "stdint.h"
#include "app_msg.h"
#include "stdbool.h"
#ifndef _APP_USB_PASSTHROUGH_H_
#define _APP_USB_PASSTHROUGH_H_



typedef bool(*P_PT_SENDOUT_CB)(uint8_t *data, uint16_t length);


bool app_usb_hid_send_passthrough(uint8_t *data, uint16_t length);

void usb_pt_register_cb(P_PT_SENDOUT_CB cb);

void app_usb_hid_pt_init(void);
#endif
