#ifndef _APP_USB_CALL_CONTROL_H_
#define _APP_USB_CALL_CONTROL_H_

#include "stdint.h"

void app_usb_hid_handle_set_Telephony_cmd(uint8_t *data, uint8_t length);

void app_usb_call_control_init(void);

#endif
