#if F_APP_USB_GIP_SUPPORT
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "trace.h"
#include "Gip.h"
#include "GipBuild.h"
#include "GipConfig.h"
#include "app_timer.h"
#include "trace.h"
#include "usb_audio_stream.h"
#include "usb_gip.h"
#include "app_audio_pipe.h"
#include "app_cfg.h"
#include "app_gip.h"
#include "app_io_msg.h"
#include "app_usb.h"
#include "rtl876x_wdg.h"

#define XBOX_MODE_SWITCH    1

#if F_APP_GIP_CONTROLLER_WITH_AUDIO_SUPPORT
#define SECONDARY_DEVICE_SUPPORT    1
#else
#define SECONDARY_DEVICE_SUPPORT    0
#endif

#if XBOX_MODE_SWITCH
#include "section.h"
#include "app_dlps.h"
#include "hal_gpio_int.h"
#include "hal_gpio.h"
#define XBOX_MODE_SWITCH_PIN        ADC_1
#endif

#if SECONDARY_DEVICE_SUPPORT
static uint16_t second_pid = 0x02F6;
static uint8_t second_deviceid[8] = {0};

static bool app_gip_calc_secondary_device_id(uint8_t *second_deviceid, uint8_t idx);
#endif

#define GIP_USB_ADAPTOR_PLUG        0x01
#define GIP_USB_SECURITY_OK         0x02
#define APP_GIP_START_BT_CONNECT    0X01
#define APP_GIP_START_USB_ENUM      0X02

#define START_GIP_READY            (APP_GIP_START_BT_CONNECT|APP_GIP_START_USB_ENUM)
#define START_GIP_USB_ENUM_READY   (GIP_USB_ADAPTOR_PLUG|GIP_USB_SECURITY_OK)

static uint8_t gip_timer_queue_id = 0;
static uint8_t start_gip_ready_bit = 0;
static uint8_t start_usb_enum_ready_bit = 0;

//for test, get from security ic
//static uint16_t pidd = 0x12E6;
//static uint16_t vidd = 0x1038;
static uint8_t deviceid[8] = {0};
char serialnum[25] = {'\0'};
static uint8_t is_xbox_mode = 0;

static uint8_t timer_idx_gip_message_pump = 0;

static uint32_t app_gip_receive_packet_handler(void *handle, void *data, uint32_t data_size,
                                               int status);

static void app_gip_usb_enum_enable(uint8_t bitmap);
static void app_gip_usb_enum_complete(void);
static void app_gip_start_enable(uint8_t bitmap);
static void app_gip_start_disable(uint8_t bitmap);
static void app_gip_usb_plug_handle(void);
static bool app_gip_calc_device_id(uint8_t *puid);
static void *int_out_ep_handle = NULL;

typedef enum
{
    APP_TIMER_GIP_PUMP,
} T_GIP_TIMER;

#if XBOX_MODE_SWITCH
ISR_TEXT_SECTION void app_dlps_pad_wake_up_polarity_invert(uint8_t pinmux)
{
    if (pinmux != 0xFF)
    {
        uint8_t gpio_level = hal_gpio_get_input_level(pinmux);

        Pad_WakeupPolarityValue(pinmux,
                                gpio_level ? PAD_WAKEUP_POL_LOW : PAD_WAKEUP_POL_HIGH);
    }
}

static bool app_gip_calc_device_id(uint8_t *puid)
{
    if (puid == NULL)
    {
        return false;
    }

    char str[20] = {'\0'};
    char stm[25] = {'\0'};

    deviceid[0] = puid[12] ^ puid[8] ^ puid[4];
    deviceid[1] = puid[13] ^ puid[9] ^ puid[5];
    deviceid[2] = puid[14] ^ puid[10] ^ puid[6];
    deviceid[3] = puid[15] ^ puid[11] ^ puid[7];
    deviceid[4] = puid[12] ^ puid[2];
    deviceid[5] = puid[13] ^ puid[3];
    deviceid[6] = 0;
    deviceid[7] = 0;

    for (int s = 0; s <= 7; s ++)
    {
        sprintf(str, "%x", deviceid[7 - s]);

        if (str[1] == '\0')
        {
            str[1] = str[0];
            str[0] = '0';
        }

        strcat(stm, str);
        strcat(stm, " ");
    }

    memcpy(serialnum, stm, 25);

    return true;
}

void app_gip_mode_switch_handle_msg(T_IO_MSG *msg)
{
    uint8_t gpio_status = msg->u.param;

    app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
    if (gpio_status == hal_gpio_get_input_level(XBOX_MODE_SWITCH_PIN))
    {
        if (gpio_status)
        {
            APP_PRINT_INFO1("app_gip_mode_switch_handle_msg: USB_MODE, is_xbox_mode %d", is_xbox_mode);

            if (is_xbox_mode)
            {
                chip_reset(RESET_ALL);
            }
        }
        else
        {
            APP_PRINT_INFO1("app_gip_mode_switch_handle_msg: XBOX_MODE, is_xbox_mode %d", is_xbox_mode);

            if (!is_xbox_mode)
            {
                chip_reset(RESET_ALL);
            }
        }
    }
    hal_gpio_irq_enable(XBOX_MODE_SWITCH_PIN);
}

ISR_TEXT_SECTION
static void app_gip_mode_switch_intr_callback(uint32_t param)
{
    T_IO_MSG gpio_msg;
    uint8_t pin_num = XBOX_MODE_SWITCH_PIN;
    uint8_t gpio_status = hal_gpio_get_input_level(pin_num);

    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);
    hal_gpio_irq_disable(pin_num);

    if (GPIO_LEVEL_HIGH == gpio_status)
    {
        hal_gpio_irq_change_polarity(pin_num, GPIO_IRQ_ACTIVE_LOW);
    }
    else
    {
        hal_gpio_irq_change_polarity(pin_num, GPIO_IRQ_ACTIVE_HIGH);
    }

    app_dlps_pad_wake_up_polarity_invert(pin_num);

    gpio_msg.type = IO_MSG_TYPE_GPIO;
    gpio_msg.subtype = IO_MSG_GPIO_SLIDE_SWITCH_0;

    gpio_msg.u.param = gpio_status;

    app_io_msg_send(&gpio_msg);
}

void app_gip_mode_switch_board_init(void)
{
    hal_gpio_init_pin(XBOX_MODE_SWITCH_PIN, GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_UP);
}

void app_gip_mode_switch_driver_init(void)
{
    hal_gpio_set_up_irq(XBOX_MODE_SWITCH_PIN, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_HIGH, true);
    hal_gpio_register_isr_callback(XBOX_MODE_SWITCH_PIN, app_gip_mode_switch_intr_callback, 0);
    hal_gpio_irq_enable(XBOX_MODE_SWITCH_PIN);

    app_dlps_pad_wake_up_polarity_invert(XBOX_MODE_SWITCH_PIN);
}

void app_gip_mode_switch_init(void)
{
    app_gip_mode_switch_board_init();
    app_gip_mode_switch_driver_init();

    uint8_t gpio_status = hal_gpio_get_input_level(XBOX_MODE_SWITCH_PIN);

    if (gpio_status)
    {
        is_xbox_mode = 0;
        APP_PRINT_INFO0("app_gip_mode_switch_init: USB_MODE");
    }
    else
    {
        is_xbox_mode = 1;
        APP_PRINT_INFO0("app_gip_mode_switch_init: XBOX_MODE");
    }

    app_usb_set_gip_flag(is_xbox_mode);
}
#endif

void app_gip_handle_security_info(uint16_t pid, uint16_t vid, uint8_t *puid)
{
    if (app_gip_calc_device_id(puid))
    {
        APP_PRINT_TRACE4("app_gip_handle_security_info: gip_vid 0x%x, gip_pid 0x%x, gip_puid %b, deviceid %b",
                         vid, pid, TRACE_BINARY(20, puid), TRACE_BINARY(8, deviceid));
        GipDeviceSetDesc(pid, vid, deviceid, 0);

#if SECONDARY_DEVICE_SUPPORT
        if (app_gip_calc_secondary_device_id(second_deviceid, 1))
        {
            GipDeviceSetDesc(second_pid, vid, second_deviceid, 1);
        }
#endif

        app_usb_gip_cfg_desc(pid, vid, serialnum);
        app_gip_usb_enum_enable(GIP_USB_SECURITY_OK);
    }
}

#if SECONDARY_DEVICE_SUPPORT
static bool app_gip_calc_secondary_device_id(uint8_t *second_deviceid, uint8_t idx)
{
    bool ret = false;
    uint8_t zeros[8] = {0};

    if (memcmp(deviceid, zeros, 8))
    {
        memcpy(second_deviceid, deviceid, 8);

        uint8_t j = 0;

        for (uint8_t i = 1; i < 5 && j < 1; i++)
        {
            while (j < 1)
            {
                second_deviceid[6 - i] ++;

                if (second_deviceid[6 - i] == 0)
                {
                    break;
                }
                j++;
            }

            if (second_deviceid[6 - i] != 0)
            {
                break;
            }
        }

        ret = true;
    }

    APP_PRINT_TRACE1("app_gip_calc_secondary_device_id: deviceid %b", TRACE_BINARY(8, second_deviceid));

    return ret;
}

void app_gip_controller_audio_enable(bool enabled)
{
    GipControllerAudioSetEnabled(enabled);
}
#endif

static void app_gip_usb_enum_enable(uint8_t bitmap)
{
    if (bitmap & START_GIP_USB_ENUM_READY)
    {
        start_usb_enum_ready_bit |= bitmap;
    }
    else
    {
        return;
    }

    if (start_usb_enum_ready_bit == START_GIP_USB_ENUM_READY)
    {
        app_usb_start();
    }
}
static void app_gip_start_enable(uint8_t bitmap)
{
    APP_PRINT_TRACE2("app_gip_start_enable %x, cur %x", bitmap, start_gip_ready_bit);

    if (bitmap & START_GIP_READY)
    {
        start_gip_ready_bit |= bitmap;
    }
    else
    {
        return;
    }

    if (start_gip_ready_bit == START_GIP_READY)
    {
        app_gip_start();
    }
}

static void app_gip_start_disable(uint8_t bitmap)
{
    APP_PRINT_TRACE2("app_gip_start_disable %x, cur %x", bitmap, start_gip_ready_bit);

    start_gip_ready_bit &= ~ bitmap;
}

static uint32_t app_gip_receive_packet_handler(void *handle, void *data, uint32_t data_size,
                                               int status)
{
    T_IO_MSG io_msg;

    io_msg.type = IO_MSG_TYPE_USB_GIP;
    io_msg.subtype = 0;

    io_msg.u.buf = data;

    app_io_msg_send(&io_msg);

    return 0;
}

static void app_gip_usb_enum_complete(void)
{
    GipDeviceInit();

    GipUsbEnumerated(true);

    app_gip_start_enable(APP_GIP_START_USB_ENUM);
}

static void app_gip_usb_plug_handle(void)
{
    app_gip_usb_enum_enable(GIP_USB_ADAPTOR_PLUG);
}

static void app_gip_usb_suspend_handle(void)
{
    app_gip_start_disable(APP_GIP_START_USB_ENUM);
    app_stop_timer(&timer_idx_gip_message_pump);
}

static void app_gip_timer_callback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_gip_timer_callback: timer_id 0x%02x, timer_chann %d",
                     timer_id, timer_chann);

    switch (timer_id)
    {
    case APP_TIMER_GIP_PUMP:
        {
            GipMessagePump();

            if (timer_idx_gip_message_pump != 0)
            {
                app_start_timer(&timer_idx_gip_message_pump, "app_gip_message_pump",
                                gip_timer_queue_id, APP_TIMER_GIP_PUMP, 0, false,
                                8);
            }
        }
        break;

    default:
        break;
    }
}

uint8_t app_gip_get_switch_mode(void)
{
    return is_xbox_mode;
}

void app_gip_start(void)
{
    GipDeviceSetConnType();
    GipMessagePump();
    app_start_timer(&timer_idx_gip_message_pump, "app_gip_message_pump",
                    gip_timer_queue_id, APP_TIMER_GIP_PUMP, 0, false,
                    8);
}

void app_gip_handle_bt_evt(T_APP_GIP_BT_EVT evt)
{
    switch (evt)
    {
    case GIP_BT_EVT_CONN:
        {
            app_gip_start_enable(APP_GIP_START_BT_CONNECT);
        }
        break;

    case GIP_BT_EVT_DISCONN:
        {
            GipDeviceStop();
            app_stop_timer(&timer_idx_gip_message_pump);
            app_gip_start_disable(APP_GIP_START_BT_CONNECT);
        }
        break;

    default:
        break;
    }
}

void app_gip_usb_msg_handle(T_IO_MSG *msg)
{
    uint8_t *data = (uint8_t *)(msg->u.buf);
    uint32_t data_size = sizeof(data);

    GipPacketReceive(GipConnType_USB, data, data_size);
}

void app_gip_init(void)
{
    if (is_xbox_mode)
    {
        app_usb_start_enum_register(app_gip_usb_plug_handle);
        app_usb_suspend_register(app_gip_usb_suspend_handle);
        app_timer_reg_cb(app_gip_timer_callback, &gip_timer_queue_id);

        if (int_out_ep_handle == NULL)
        {
            T_USB_GIP_ATTR attr =
            {
                .zlp = 1,
                .high_throughput = 0,
                .congestion_ctrl = GIP_CONGESTION_CTRL_DROP_CUR,
                .rsv = 0,
                .mtu = GIP_INT_OUT_MAX_TRANSMISSION_UNIT
            };
            int_out_ep_handle = usb_gip_data_pipe_open(GIP_INT_OUT_ENDPOINT_ADDRESS, attr, 1,
                                                       app_gip_receive_packet_handler);
        }
        app_usb_enum_done_register((GIP_CB_ENUM_DONE)app_gip_usb_enum_complete);
    }
}
#endif
