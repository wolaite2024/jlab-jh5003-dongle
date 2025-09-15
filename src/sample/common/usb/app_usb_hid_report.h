#ifndef __USB_HID_REPORT_H__
#define __USB_HID_REPORT_H__

#include <stdint.h>
#include "app_msg.h"

typedef bool (*USB_HID_REPORT_CB)(uint8_t *data, uint8_t size);

/* handle os msg of hid report */
void usb_hid_report_handle(void);

/* Buffer HID data and ensure a fixed interval between each HID report transmission */
bool usb_hid_report_buffered_send(uint8_t *data, uint8_t size);

/* Register the callback for sending HID reports to the USB host and set the buffer size */
void usb_hid_report_register_cb(USB_HID_REPORT_CB cb, uint16_t buf_size, uint16_t msg_type,
                                uint16_t msg_sub_type);

#endif
