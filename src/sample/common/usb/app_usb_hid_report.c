#if F_APP_USB_HID_SUPPORT

#include <stdlib.h>
#include "os_sync.h"
#include "trace.h"
#include "ring_buffer.h"
#include "app_usb_hid_report.h"
#include "app_timer.h"
#include "app_io_msg.h"

typedef enum
{
    USB_HID_REPORT_TIMEOUT,
} T_USB_HID_REPORT_TIMER;

#define USB_HID_REPORT_INTERVAL     50   //ms

static USB_HID_REPORT_CB hid_report_cb;
static T_RING_BUFFER hid_report_ring_buf;
static uint8_t *hid_report_buf;
static uint8_t usb_hid_report_timer_id = 0;
static uint8_t timer_idx_usb_hid_report;
static uint16_t hid_msg_type;
static uint16_t hid_msg_subtype;

static void usb_hid_process_tx_buffer(void)
{
    uint16_t remain_size = ring_buffer_get_data_count(&hid_report_ring_buf);

    if (remain_size == 0 || hid_report_cb == NULL)
    {
        return;
    }

    uint8_t size;
    ring_buffer_peek(&hid_report_ring_buf, 1, &size);

    if (remain_size >= size + 1)
    {
        ring_buffer_remove(&hid_report_ring_buf, 1);

        uint8_t *buf = malloc(size);

        if (buf != NULL)
        {
            ring_buffer_read(&hid_report_ring_buf, size, buf);
            hid_report_cb(buf, size);
            free(buf);
        }
        else
        {
            ring_buffer_remove(&hid_report_ring_buf, size);
        }
    }
    else
    {
        ring_buffer_remove(&hid_report_ring_buf, remain_size);
    }
}

static void usb_hid_report_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case USB_HID_REPORT_TIMEOUT:
        {
            if (ring_buffer_get_data_count(&hid_report_ring_buf) == 0)
            {
                app_stop_timer(&timer_idx_usb_hid_report);
            }
            else
            {
                usb_hid_process_tx_buffer();
            }
        }
        break;

    default:
        break;
    }
}

bool usb_hid_report_buffered_send(uint8_t *data, uint8_t size)
{
    bool ret = false;

    uint32_t s = os_lock();

    if (ring_buffer_get_remaining_space(&hid_report_ring_buf) >= (size + 1))
    {
        T_IO_MSG gpio_msg = {0};

        gpio_msg.type = hid_msg_type;
        gpio_msg.subtype = hid_msg_subtype;

        if (app_io_msg_send(&gpio_msg))
        {
            ring_buffer_write(&hid_report_ring_buf, &size, 1);
            ring_buffer_write(&hid_report_ring_buf, data, size);

            ret = true;
        }
        else
        {
            APP_PRINT_ERROR0("usb_hid_report_buffered_send: msg send fail");
        }
    }

    os_unlock(s);

    APP_PRINT_TRACE2("usb_hid_report_buffered_send: size %d ret %d", size, ret);

    return ret;
}

void usb_hid_report_handle(void)
{
    if (ring_buffer_get_data_count(&hid_report_ring_buf) > 0)
    {
        if (timer_idx_usb_hid_report == 0)
        {
            app_start_timer(&timer_idx_usb_hid_report, "usb_hid_report",
                            usb_hid_report_timer_id, USB_HID_REPORT_TIMEOUT, 0, true,
                            USB_HID_REPORT_INTERVAL);

            usb_hid_process_tx_buffer();
        }
    }
}

void usb_hid_report_register_cb(USB_HID_REPORT_CB cb, uint16_t buf_size, uint16_t msg_type,
                                uint16_t msg_sub_type)
{
    if (hid_report_cb == NULL)
    {
        hid_report_cb = cb;
        hid_msg_type = msg_type;
        hid_msg_subtype = msg_sub_type;

        hid_report_buf = calloc(1, buf_size);
        ring_buffer_init(&hid_report_ring_buf, hid_report_buf, buf_size);

        app_timer_reg_cb(usb_hid_report_timeout_cb, &usb_hid_report_timer_id);
    }
}

#endif
