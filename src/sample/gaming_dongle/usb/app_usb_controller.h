#ifndef __APP_USB_CONTROLLER_H__
#define __APP_USB_CONTROLLER_H__

#include "app_msg.h"

void app_usb_controller_io_msg(T_IO_MSG *io_driver_msg_recv);
bool app_usb_handle_controller_data_from_headset(uint8_t *data);
void app_usb_controller_init(void);

#endif
