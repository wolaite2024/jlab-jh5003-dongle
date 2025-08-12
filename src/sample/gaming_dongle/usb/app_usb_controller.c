#if F_APP_GAMING_CONTROLLER_SUPPORT
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "app_io_msg.h"
#include "section.h"
#include "hw_tim.h"
#include "ring_buffer.h"
#include "app_usb_hid.h"
#include "app_usb_controller.h"
#include "trace.h"

#if F_APP_USB_GIP_SUPPORT
#include "app_gip.h"
#endif

#define CONTROLLER_BUF_SIZE         256

#define HID_REPORT_TO_USB_HOST_INTERVAL           2000 //us

static T_HW_TIMER_HANDLE controller_data_timer_handle = NULL;
static uint8_t *controller_data_buf = NULL;
static T_RING_BUFFER controller_data_ring_buf;

static void app_usb_send_controller_data_to_host(uint8_t *data, uint8_t len)
{
    bool is_xbox_mode = false;

#if F_APP_USB_GIP_SUPPORT
    if (app_gip_get_switch_mode())
    {
        is_xbox_mode = true;
    }
#endif

    if (is_xbox_mode == false)
    {
        app_usb_hid_interrupt_in(data, len);
    }
#if F_APP_USB_GIP_SUPPORT
    else
    {
        /* use GipPacketSend send to XBOX usb host */
        APP_PRINT_TRACE0("GipPacketSend");
    }
#endif
}

void app_usb_controller_io_msg(T_IO_MSG *io_driver_msg_recv)
{
    uint8_t data_size = 0;
    uint8_t buf[64] = {0};

    if (ring_buffer_get_data_count(&controller_data_ring_buf) == 0)
    {
        return;
    }

    if (ring_buffer_read(&controller_data_ring_buf, sizeof(uint8_t), &data_size))
    {
        if (data_size <= sizeof(buf))
        {
            if (ring_buffer_read(&controller_data_ring_buf, data_size, buf))
            {
                app_usb_send_controller_data_to_host(buf, (uint8_t)data_size);
            }
        }
        else
        {
            APP_PRINT_ERROR1("app_usb_controller_io_msg: invalid size %d", data_size);
        }
    }
}

bool app_usb_handle_controller_data_from_headset(uint8_t *data)
{
    uint8_t data_cnt = data[0];
    uint8_t *ptr = data + 1;
    uint8_t data_len = 0;
    uint8_t hid_buf[128] = {0};
    bool ret = true;
    uint8_t failed_cause = 0;

    for (uint8_t i = 0; i < data_cnt; i++)
    {
        data_len = *ptr;
        ptr++;

        memcpy(hid_buf, ptr, data_len);
        ptr += data_len;

        uint8_t buf[64] = {0};
        uint8_t *ptr2 = buf;

        memcpy(ptr2++, &data_len, sizeof(data_len));
        memcpy(ptr2, hid_buf, data_len);

        uint16_t write_size = data_len + 1;

        if (ring_buffer_get_remaining_space(&controller_data_ring_buf) < write_size)
        {
            failed_cause = 1;
            ret = false;
            goto exit;
        }

        ring_buffer_write(&controller_data_ring_buf, buf, write_size);
    }

exit:

    if (ring_buffer_get_data_count(&controller_data_ring_buf) != 0)
    {
        bool hw_timer_active = false;
        uint8_t hid_size = 0;
        uint8_t buf[64] = {0};

        hw_timer_is_active(controller_data_timer_handle, &hw_timer_active);

        if (hw_timer_active == false)
        {
            hw_timer_start(controller_data_timer_handle);
        }

        if (ring_buffer_read(&controller_data_ring_buf, sizeof(hid_size), &hid_size))
        {
            if (hid_size <= sizeof(buf))
            {
                if (ring_buffer_read(&controller_data_ring_buf, hid_size, buf))
                {
                    app_usb_send_controller_data_to_host(buf, hid_size);
                }
            }
            else
            {
                APP_PRINT_ERROR1("app_usb_handle_controller_data_from_headset: invalid size %d", hid_size);
            }
        }
    }

    if (ret == false)
    {
        APP_PRINT_ERROR1("app_usb_handle_controller_data_from_headset failed: cause -%d", failed_cause);
    }

    return ret;
}

RAM_TEXT_SECTION static void controller_data_timer_isr_cb(T_HW_TIMER_HANDLE handle)
{
    T_IO_MSG controller_msg;

    APP_PRINT_TRACE0("controller_data_timer_isr_cb");

    if (ring_buffer_get_data_count(&controller_data_ring_buf) != 0)
    {
        controller_msg.type = IO_MSG_TYPE_CONTROLLER;

        app_io_msg_send(&controller_msg);
    }
    else
    {
        hw_timer_stop(controller_data_timer_handle);
    }
}

void app_usb_controller_init(void)
{
    if (controller_data_buf == NULL)
    {
        controller_data_buf = malloc(CONTROLLER_BUF_SIZE);

        ring_buffer_init(&controller_data_ring_buf, controller_data_buf, CONTROLLER_BUF_SIZE);
    }

    controller_data_timer_handle = hw_timer_create("controller_data_timer_handle",
                                                   HID_REPORT_TO_USB_HOST_INTERVAL,
                                                   true,
                                                   controller_data_timer_isr_cb);
}

#endif
