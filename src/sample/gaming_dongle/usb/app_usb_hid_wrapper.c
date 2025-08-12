/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
#include "trace.h"
#include <string.h>
#include "app_usb_hid_wrapper.h"
#include "app_usb_hid.h"
#include "usb_hid.h"
#include "app_usb.h"

static void *hid_intr_in_handle = NULL;

static int8_t app_usb_hid_set_report_handle(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                            uint16_t length)
{
    extern int8_t app_usb_hid_handle_set_report(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                                uint16_t length);
    return app_usb_hid_handle_set_report(req_value, data, length);
}

static int8_t app_usb_hid_get_report_handle(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                            uint16_t *length)
{
    uint8_t ret = 0;

    extern uint8_t app_usb_hid_handle_get_report(T_HID_DRIVER_REPORT_REQ_VAL req_value, uint8_t *data,
                                                 uint16_t *length);
    ret = app_usb_hid_handle_get_report(req_value, data, length);

    return ret;
}

//bool app_usb_hid_get_report_data_is_ready(uint8_t *data, uint8_t length)
#define USB_VOL_CMD_LEN     3
static uint32_t app_usb_hid_interrupt_pipe_send_complete(void *handle, void *buf, uint32_t result,
                                                         int status)
{
    extern void app_usb_hid_interrupt_in_complete_result(int result, uint8_t *buf);
    if (result <= USB_VOL_CMD_LEN) // optimize loading
    {
        app_usb_hid_interrupt_in_complete_result(result, (uint8_t *)buf);
    }
    return 0;
}

bool app_usb_hid_interrupt_pipe_send(void *data, uint16_t length)
{
    bool ret = false;

    if (app_usb_power_state() == USB_CONFIGURED)
    {
        if (hid_intr_in_handle == NULL)
        {
            T_USB_HID_ATTR attr =
            {
                .zlp = 1,
                .high_throughput = 0,
                .congestion_ctrl = HID_CONGESTION_CTRL_DROP_CUR,
                .rsv = 0,
                .mtu = HID_MAX_TRANSMISSION_UNIT
            };
            hid_intr_in_handle = usb_hid_data_pipe_open(HID_INT_IN_EP_1, attr, HID_MAX_PENDING_REQ_NUM,
                                                        app_usb_hid_interrupt_pipe_send_complete);
        }
        ret = usb_hid_data_pipe_send(hid_intr_in_handle, data, length);
    }
    return ret;
}

void app_usb_hid_wrapper_init(void)
{
    usb_hid_init();
//    hid_intr_in_handle = usb_hid_data_pipe_open(HID_INT_IN_EP_1);

    T_HID_CBS cbs = {.set_report = (INT_OUT_FUNC)app_usb_hid_set_report_handle, .get_report = (INT_IN_FUNC)app_usb_hid_get_report_handle};
    usb_hid_ual_register(cbs);
}
#endif
