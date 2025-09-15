#if F_APP_USB_HID_SUPPORT & F_APP_USB_AUDIO_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "usb_dm.h"
#include "app_msg.h"
#include "usb_msg.h"
#include "usb_host_detect.h"
#include "trace.h"
#include "usb_hid.h"
#include "hal_adp.h"
#include "app_timer.h"
#include "usb_hid_spec.h"
#include "app_ipc.h"
#include "usb_msg.h"

#define SETUP_PKT_GET_DEV_DESC              ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_DEVICE) |   \
                                             (USB_REQ_CODE_GET_DESCRIPTOR << 8)| (USB_DESC_TYPE_DEVICE << 24)) //4 byte
#define SETUP_PKT_GET_REPORT_DESC(intf_num) ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE) |  \
                                             (USB_REQ_CODE_GET_DESCRIPTOR << 8) | (HID_DT_REPORT << 24) | \
                                             ((uint64_t)intf_num << 32))  // 6 byte
#define SETUP_PKT_CLEAR_FEATURE(ep_num)     ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_ENDPOINT)|  \
                                             (USB_REQ_CODE_CLEAR_FEATURE << 8)|((uint64_t)ep_num << 32))  //6 byte
#define SETUP_PKT_SET_ADDRESS               ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_DEVICE) | \
                                             (USB_REQ_CODE_SET_ADDRESS << 8))  // 2 byte

#define SETUP_PKT_SIZE                      8
#define AUDIO_VOLUME_MAX                    1

#define USB_HOST_DETECT_TIMER_IN_NAK_MS     500
#define USB_HOST_DETECT_TIMER_SET_VOL_MS    600

typedef enum
{
    USB_EVT_PLUG,
    USB_EVT_UNPLUG,
    USB_EVT_EP_IN_NAK,
    USB_EVT_CLEAR_FEATURE,
    USB_EVT_GET_REPORT_DESC,
    USB_EVT_GET_DEVICE_DESC,
    USB_EVT_SET_ADDRESS,
} T_USB_EVT;

typedef enum
{
    USB_HOST_DETECT_TIMER_MONITOR_IN_NAK,
    USB_HOST_DETECT_TIMER_MONITOR_SET_VOL,
} T_USB_HOST_DETECT_TIMER;

typedef enum
{
    CTRL_REQ_UNDEF,
    CTRL_REQ_SET_ADDRESS,
    CTRL_REQ_GET_DEVICE_DESC,
} T_CTRL_REQ;

typedef enum
{
    IN_NAK_STATUS_UNDEF,
    IN_NAK_STATUS_RCVD,
    IN_NAK_STATUS_NOTRCVD,
} T_IN_NAK_STATUS;

typedef enum
{
    VOL_STATUS_UNDEF,
    VOL_STATUS_SET,
    VOL_STATUS_NOTSET,
} T_VOL_STATUS;

typedef struct _recorder
{
    uint8_t clear_feature;
    uint8_t set_res;
    uint8_t audio_data_xmit;
    uint8_t audio_mute;
    T_IN_NAK_STATUS in_nak;
    T_VOL_STATUS set_volume;
    T_CTRL_REQ first_ctrlreq;
    T_DETECT_STATUS detect_status;
} T_RECORDER;

typedef struct _usb_host_detect_db
{
    uint8_t monitor_nak_idx;
    uint8_t hid_ep_nak_id;
    uint8_t monitor_vol_idx;
    uint8_t audio_set_vol_id;
    uint8_t is_max_volume;
    T_OS_TYPE os_type;
    T_RECORDER recorder;
    T_USB_HOST_DETECT_HID_INFO hid_info;
} T_USB_HOST_DETECT_DB;

typedef struct _usb_host_detect_elem
{
    struct _usb_host_detect_elem *p_next;
    HOST_DETECT_CB cb;
} T_USB_HOST_DETECT_ELEM;

static T_OS_QUEUE usb_host_detect_cb_list;
T_USB_HOST_DETECT_DB usb_host_detect_db;

static bool usb_host_detect_trigger_evt(T_USB_EVT evt, uint32_t param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB;
    gpio_msg.subtype = USB_MSG(USB_MSG_GROUP_HOST_DETECT, evt);
    gpio_msg.u.param = param;
    return app_io_msg_send(&gpio_msg);
}

static bool usb_host_detect_dm_cb(T_USB_DM_EVT evt, T_USB_DM_EVT_PARAM *param)
{
    T_USB_HOST_DETECT_HID_INFO info = {0};
    if (!memcmp(&usb_host_detect_db.hid_info, &info, sizeof(T_USB_HOST_DETECT_HID_INFO)))
    {
        APP_PRINT_ERROR0("Please register hid_info by usb_host_detect_hid_info_register");
        return 0;
    }
    T_USB_HOST_DETECT_HID_INFO hid_info = usb_host_detect_db.hid_info;
    if (evt == USB_DM_EVT_EP_IN_NAK)
    {
        uint8_t ep_num = param->ep_in_nak.ep_num;
        if (evt == USB_DM_EVT_EP_IN_NAK && ep_num == (hid_info.ep_num & 0x1))
        {
            USB_PRINT_INFO0("usb_host_detect_dm_cb, in_nak_rcvd");
            usb_host_detect_trigger_evt(USB_EVT_EP_IN_NAK, 0);
        }
    }
    else if (evt == USB_DM_EVT_SETUP_PEEK)
    {
        uint8_t *setup_pkt = param->setup_peek.pkt;
        uint64_t pkt;
        memcpy(&pkt, setup_pkt, SETUP_PKT_SIZE);
        if ((pkt & 0xFFFFFFFF) == SETUP_PKT_GET_DEV_DESC)
        {
            usb_host_detect_trigger_evt(USB_EVT_GET_DEVICE_DESC, 0);
        }
        else if ((pkt & 0xFFFF) == SETUP_PKT_SET_ADDRESS)
        {
            usb_host_detect_trigger_evt(USB_EVT_SET_ADDRESS, 0);
        }
        else if ((pkt & 0xFFFFFFFFFFFF) == SETUP_PKT_CLEAR_FEATURE(hid_info.ep_num))
        {
            usb_host_detect_trigger_evt(USB_EVT_CLEAR_FEATURE, 0);
        }
        else if ((pkt & 0xFFFFFFFFFFFF) == SETUP_PKT_GET_REPORT_DESC(hid_info.intf_num))
        {
            usb_host_detect_trigger_evt(USB_EVT_GET_REPORT_DESC, 0);
        }
    }
    return 0;
}

static void usb_host_detect_event_post(T_OS_TYPE os_type)
{
    T_USB_HOST_DETECT_ELEM *p_elem = NULL;

    if (usb_host_detect_db.os_type == os_type)
    {
        return;
    }

    for (uint8_t i = 0; i < usb_host_detect_cb_list.count; i++)
    {
        p_elem = os_queue_peek(&usb_host_detect_cb_list, i);

        if (p_elem != NULL && p_elem->cb != NULL)
        {
            p_elem->cb(os_type);
        }
    }

    usb_host_detect_db.os_type = os_type;
}

void usb_host_detect_os_type_handle(void)
{
    T_RECORDER recorder = usb_host_detect_db.recorder;
//  USB_PRINT_INFO8("usb_host_detect_os_type_handle, %x-%x-%x-%x-%x-%x-%x-%x", recorder.in_nak, recorder.clear_feature, recorder.detect_status,
//  recorder.set_volume, recorder.set_res, recorder.first_ctrlreq, recorder.audio_mute, recorder.audio_data_xmit);
    if (recorder.in_nak == IN_NAK_STATUS_NOTRCVD)
    {
        usb_host_detect_event_post(OS_TYPE_PS);
        //USB_PRINT_INFO0("usb_host_detect_os_type_handle, OS_TYPE_PS");
        return;
    }
    else if (recorder.in_nak == IN_NAK_STATUS_UNDEF)
    {
        return;
    }
    else
    {
        if (usb_host_detect_db.os_type == OS_TYPE_PS)
        {
            usb_host_detect_event_post(OS_TYPE_UNDEF);
        }
    }
    if (recorder.clear_feature)
    {
        usb_host_detect_event_post(OS_TYPE_SWITCH);
        //USB_PRINT_INFO0("usb_host_detect_os_type_handle, OS_TYPE_SWITCH");
        return;
    }
    if (recorder.detect_status == DETECT_STATUS_IN_PROGRESS)
    {
        if (recorder.set_volume == VOL_STATUS_UNDEF)
        {
            return;
        }
        else if (recorder.set_volume == VOL_STATUS_NOTSET)
        {
            if (recorder.set_res)
            {
                usb_host_detect_event_post(OS_TYPE_ANDROID);
                //USB_PRINT_INFO0("usb_host_detect_os_type_handle, OS_TYPE_ANDROID");
                return;
            }
        }
        if (recorder.first_ctrlreq == CTRL_REQ_GET_DEVICE_DESC)
        {
            usb_host_detect_event_post(OS_TYPE_WINDOWS);
            //USB_PRINT_INFO0("usb_host_detect_os_type_handle, OS_TYPE_WINDOWS");
        }
        else
        {
            usb_host_detect_event_post(OS_TYPE_IOS);
            //USB_PRINT_INFO0("usb_host_detect_os_type_handle, OS_TYPE_IOS");
        }
        return;
    }
    else if (recorder.detect_status == DETECT_STATUS_COMPLETE)
    {
        T_USB_DM_EVT_MSK evt_msk = {0};
        evt_msk.b.in_nak = 1;
        usb_dm_evt_msk_change(evt_msk, usb_host_detect_dm_cb);
        return;
    }
    if (!recorder.audio_data_xmit | recorder.audio_mute)
    {
        return;
    }

    usb_host_detect_db.recorder.detect_status = DETECT_STATUS_IN_PROGRESS;

    usb_host_detect_db.hid_info.pipe_open();

    if (usb_host_detect_db.is_max_volume == AUDIO_VOLUME_MAX)
    {
        usb_host_detect_db.hid_info.volume_down();
    }
    else
    {
        usb_host_detect_db.hid_info.volume_up();
    }
    usb_host_detect_db.hid_info.volume_release();
    app_start_timer(&usb_host_detect_db.monitor_vol_idx, "monitor_setvol",
                    usb_host_detect_db.audio_set_vol_id, USB_HOST_DETECT_TIMER_MONITOR_SET_VOL,
                    NULL, false, USB_HOST_DETECT_TIMER_SET_VOL_MS);

    return;
}

void usb_host_detect_evt_handle(uint8_t evt, uint32_t param)
{
    switch (evt)
    {
    case USB_EVT_PLUG:
        {
            memset(&usb_host_detect_db.recorder, 0, sizeof(T_RECORDER));
            usb_host_detect_event_post(OS_TYPE_UNDEF);
            T_USB_DM_EVT_MSK evt_msk = {0};
            evt_msk.b.in_nak = 1;
            evt_msk.b.setup_peek = 1;
            usb_dm_evt_msk_change(evt_msk, usb_host_detect_dm_cb);
        }
        break;

    case USB_EVT_EP_IN_NAK:
        {
            app_stop_timer(&usb_host_detect_db.monitor_nak_idx);
            usb_host_detect_db.recorder.in_nak = IN_NAK_STATUS_RCVD;
            usb_host_detect_os_type_handle();
        }
        break;

    case USB_EVT_GET_DEVICE_DESC:
        {
            if (usb_host_detect_db.recorder.first_ctrlreq == CTRL_REQ_UNDEF)
            {
                usb_host_detect_db.recorder.first_ctrlreq = CTRL_REQ_GET_DEVICE_DESC;
            }
        }
        break;

    case USB_EVT_SET_ADDRESS:
        {
            if (usb_host_detect_db.recorder.first_ctrlreq == CTRL_REQ_UNDEF)
            {
                usb_host_detect_db.recorder.first_ctrlreq = CTRL_REQ_SET_ADDRESS;
            }
        }
        break;

    case USB_EVT_CLEAR_FEATURE:
        {
            usb_host_detect_db.recorder.clear_feature = true;
            usb_host_detect_os_type_handle();
        }
        break;

    case USB_EVT_GET_REPORT_DESC:
        {
            app_start_timer(&usb_host_detect_db.monitor_nak_idx, "monitor_innak",
                            usb_host_detect_db.hid_ep_nak_id, USB_HOST_DETECT_TIMER_MONITOR_IN_NAK, NULL, false,
                            USB_HOST_DETECT_TIMER_IN_NAK_MS);
        }
        break;

    default:
        break;
    }
    USB_PRINT_INFO1("usb_host_detect_evt_handle, evt %x", evt);
}

static void usb_host_detect_ipc_cback(uint32_t id, void *msg)
{
    switch (id)
    {
    case USB_IPC_EVT_AUDIO_DS_VOL_SET:
        {
            if (usb_host_detect_db.monitor_vol_idx == 0)
            {
                usb_host_detect_db.is_max_volume = *(uint8_t *)msg;
                //USB_PRINT_INFO1("usb_host_detect_ipc_cback, is_max_volume %x", usb_host_detect_db.is_max_volume);
                usb_host_detect_db.recorder.set_volume = VOL_STATUS_UNDEF;
            }
            else
            {
                usb_host_detect_db.recorder.set_volume = VOL_STATUS_SET;
                usb_host_detect_os_type_handle();
                app_stop_timer(&usb_host_detect_db.monitor_vol_idx);
                if (usb_host_detect_db.is_max_volume == AUDIO_VOLUME_MAX)
                {
                    usb_host_detect_db.hid_info.volume_up();
                }
                else
                {
                    usb_host_detect_db.hid_info.volume_down();
                }
                usb_host_detect_db.hid_info.volume_release();
                usb_host_detect_db.recorder.detect_status = DETECT_STATUS_COMPLETE;
            }
        }
        break;
    case USB_IPC_EVT_AUDIO_DS_XMIT:
        {
            usb_host_detect_db.recorder.audio_data_xmit = true;
            usb_host_detect_os_type_handle();
        }
        break;
    case USB_IPC_EVT_AUDIO_DS_STOP:
        {
            usb_host_detect_db.recorder.audio_data_xmit = false;
        }
        break;
    case USB_IPC_EVT_AUDIO_MUTE_CTRL:
        {
            uint32_t mute = *(uint32_t *)msg;
            if (mute)
            {
                usb_host_detect_db.recorder.audio_mute = true;
            }
            else
            {
                usb_host_detect_db.recorder.audio_mute = false;
                usb_host_detect_os_type_handle();
            }
        }
        break;
    case USB_IPC_EVT_AUDIO_SET_RES:
        {
            usb_host_detect_db.recorder.set_res = true;
        }
        break;
    default:
        break;
    }
    USB_PRINT_INFO1("usb_host_detect_ipc_cback, id %x", id);
}

static void usb_host_detect_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case USB_HOST_DETECT_TIMER_MONITOR_IN_NAK:
        {
            app_stop_timer(&usb_host_detect_db.monitor_nak_idx);
            usb_host_detect_db.recorder.in_nak = IN_NAK_STATUS_NOTRCVD;
            usb_host_detect_os_type_handle();
        }
        break;

    case USB_HOST_DETECT_TIMER_MONITOR_SET_VOL:
        {
            app_stop_timer(&usb_host_detect_db.monitor_vol_idx);
            usb_host_detect_db.recorder.set_volume = VOL_STATUS_NOTSET;
            usb_host_detect_os_type_handle();
            if (usb_host_detect_db.is_max_volume == AUDIO_VOLUME_MAX)
            {
                usb_host_detect_db.hid_info.volume_up();
            }
            else
            {
                usb_host_detect_db.hid_info.volume_down();
            }
            usb_host_detect_db.hid_info.volume_release();
            usb_host_detect_db.recorder.detect_status = DETECT_STATUS_COMPLETE;
        }
        break;

    default:
        break;
    }
}

static void usb_host_detect_adp_state_change_cb(T_ADP_PLUG_EVENT event, void *user_data)
{
    if (event == ADP_EVENT_PLUG_IN)
    {
        usb_host_detect_trigger_evt(USB_EVT_PLUG, 0);
    }
}

T_DETECT_STATUS usb_host_detect_status_peek(void)
{
    return usb_host_detect_db.recorder.detect_status;
}

T_OS_TYPE usb_host_detect_os_type_peek(void)
{
    return usb_host_detect_db.os_type;
}

void usb_host_detect_hid_info_register(T_USB_HOST_DETECT_HID_INFO info)
{
    memcpy(&usb_host_detect_db.hid_info, &info, sizeof(T_USB_HOST_DETECT_HID_INFO));
}

bool usb_host_detect_cback_register(HOST_DETECT_CB cb)
{
    T_USB_HOST_DETECT_ELEM *p_elem = (T_USB_HOST_DETECT_ELEM *)usb_host_detect_cb_list.p_first;

    while (p_elem != NULL)
    {
        if (p_elem->cb == cb)
        {
            return true;
        }

        p_elem = p_elem->p_next;
    }

    p_elem = malloc(sizeof(T_USB_HOST_DETECT_ELEM));
    if (p_elem != NULL)
    {
        p_elem->cb = cb;
        os_queue_in(&usb_host_detect_cb_list, p_elem);
        return true;
    }

    return false;
}

void usb_host_detect_init(void)
{
    T_USB_DM_EVT_MSK evt_msk = {0};
    evt_msk.b.in_nak = 1;
    evt_msk.b.setup_peek = 1;
    usb_dm_cb_register(evt_msk, usb_host_detect_dm_cb);
    memset(&usb_host_detect_db, 0, sizeof(T_USB_HOST_DETECT_DB));

    os_queue_init(&usb_host_detect_cb_list);

    app_timer_reg_cb(usb_host_detect_timeout_cb, &usb_host_detect_db.hid_ep_nak_id);
    app_timer_reg_cb(usb_host_detect_timeout_cb, &usb_host_detect_db.audio_set_vol_id);
    app_ipc_subscribe(USB_IPC_TOPIC, usb_host_detect_ipc_cback);
    adp_register_state_change_cb(ADP_DETECT_5V, (P_ADP_PLUG_CBACK)usb_host_detect_adp_state_change_cb,
                                 NULL);
}
#endif
