#if F_APP_USB_HID_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "os_sync.h"
#include "usb_hid_desc.h"
#include "usb_spec20.h"
#include "trace.h"
#include "usb_hid_spec.h"
#include "usb_hid.h"
#include "usb_pipe.h"
#include "app_hid_report_desc.h"
#include "errno.h"
#include "app_usb_vol_control.h"
#include "app_usb_hid_report.h"

uint32_t *hid_inst_0 = NULL;
#if (F_APP_SS_REVISE_HID == 1)
uint32_t *hid_inst_1 = NULL;
uint32_t *hid_inst_2 = NULL;
#endif

#if (F_APP_SS_REVISE_HID == 1)
static const char report_desc0[] =
{
    BUTTON_VOL_PLAY_CTRL_HID_DESC_ATTRIB_2,
    TELEPHONY_HID_DESC_ATTRIB,
#if F_APP_GAMING_CONTROLLER_SUPPORT
    GAME_PAD_HID_DESC_ATTRIB,
#endif

#if (F_APP_SS_SUPPORT_SPARK_TOOL == 1)
    //vendor define:0xFF07
    VENDOR_0xFF07_HID_DESC_PART_1,
#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
    VENDOR_0xFF07_HID_DESC_PART_2,
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)
    VENDOR_0xFF07_HID_DESC_PART_3,
#endif
    VENDOR_0xFF07_HID_DESC_END_COLLECTION,
#endif
};
#else
static const char report_desc0[] =
{
#if 0
    HID_REPORT_DESCS
#else
    BUTTON_VOL_PLAY_CTRL_HID_DESC_ATTRIB_2,

#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    TEAMS_ASP_HID_DESC_ATTRIB,
#endif

    TELEPHONY_HID_DESC_ATTRIB,

    //vendor define:0xFF07
    VENDOR_0xFF07_HID_DESC_PART_1,
#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
    VENDOR_0xFF07_HID_DESC_PART_2,
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)
    VENDOR_0xFF07_HID_DESC_PART_3,
#endif
    VENDOR_0xFF07_HID_DESC_END_COLLECTION,

    APP_CFU_HID_DESC_ATTRIB,
#endif
};
#endif

#if (F_APP_SS_REVISE_HID == 1)
static const char report_desc1[] =
{
    HID_REPORT_DESCS_SS_CFU
};

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT == 0
static const char report_desc2[] =
{
    HID_REPORT_DESCS_SS_CFU
};
#endif
#endif

typedef struct _t_hid_ual
{
    struct _t_hid_ual *p_next;
    T_HID_CBS cbs;
} T_HID_UAL;

T_OS_QUEUE ual_list;

static T_USB_INTERFACE_DESC hid_std_if_desc0 =
{
    .bLength            = sizeof(T_USB_INTERFACE_DESC),
    .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_CODE_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static const T_HID_CS_IF_DESC  hid_cs_if_desc0 =
{
    .bLength            = sizeof(T_HID_CS_IF_DESC),
    .bDescriptorType    = DESC_TYPE_HID,
    .bcdHID             = 0x0110,
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .desc[0]            =
    {
        .bDescriptorType = DESC_TYPE_REPORT,
        .wDescriptorLength = sizeof(report_desc0),
    },

};

static const T_USB_ENDPOINT_DESC int_in_ep_desc_fs0 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = USB_DIR_IN | 0x01,
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 1,
};

static const T_USB_ENDPOINT_DESC int_in_ep_desc_hs0 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = USB_DIR_IN | 0x01,
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 4,
};

#if (F_APP_SS_REVISE_HID == 1)
static T_USB_INTERFACE_DESC hid_std_if_desc1 =
{
    .bLength            = sizeof(T_USB_INTERFACE_DESC),
    .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_CODE_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static const T_HID_CS_IF_DESC hid_cs_if_desc1 =
{
    .bLength            = sizeof(T_HID_CS_IF_DESC),
    .bDescriptorType    = DESC_TYPE_HID,
    .bcdHID             = 0x0110,
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .desc[0]            =
    {
        .bDescriptorType = DESC_TYPE_REPORT,
        .wDescriptorLength = sizeof(report_desc1),
    },

};

static const T_USB_ENDPOINT_DESC int_ep_desc_fs1 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = USB_DIR_IN | 0x02,
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x04,
    .bInterval         = 1,
};

static const T_USB_ENDPOINT_DESC int_ep_desc_hs1 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = USB_DIR_IN | 0x02,
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x04,
    .bInterval         = 1,
};

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT == 0
static T_USB_INTERFACE_DESC hid_std_if_desc2 =
{
    .bLength            = sizeof(T_USB_INTERFACE_DESC),
    .bDescriptorType    = USB_DESC_TYPE_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_CODE_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static const T_HID_CS_IF_DESC  hid_cs_if_desc2 =
{
    .bLength            = sizeof(T_HID_CS_IF_DESC),
    .bDescriptorType    = DESC_TYPE_HID,
    .bcdHID             = 0x0110,
    .bCountryCode       = 0,
    .bNumDescriptors    = 1,
    .desc[0]            =
    {
        .bDescriptorType = DESC_TYPE_REPORT,
        .wDescriptorLength = sizeof(report_desc2),
    },

};

static const T_USB_ENDPOINT_DESC int_ep_desc_fs2 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = USB_DIR_IN | 0x04,
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 1,
};

static const T_USB_ENDPOINT_DESC int_ep_desc_hs2 =
{
    .bLength           = sizeof(T_USB_ENDPOINT_DESC),
    .bDescriptorType   = USB_DESC_TYPE_ENDPOINT,
    .bEndpointAddress  = USB_DIR_IN | 0x04,
    .bmAttributes      = USB_EP_TYPE_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 4,
};
#endif
#endif

static void *const hid_if_descs_fs0[] =
{
    (void *) &hid_std_if_desc0,
    (void *) &hid_cs_if_desc0,
    (void *) &int_in_ep_desc_fs0,
    NULL,
};

static void *const hid_if_descs_hs0[] =
{
    (void *) &hid_std_if_desc0,
    (void *) &hid_cs_if_desc0,
    (void *) &int_in_ep_desc_hs0,
    NULL,
};

#if (F_APP_SS_REVISE_HID == 1)
static void *const hid_if_descs_fs1[] =
{
    (void *) &hid_std_if_desc1,
    (void *) &hid_cs_if_desc1,
    (void *) &int_ep_desc_fs1,
    NULL,
};

static void *const hid_if_descs_hs1[] =
{
    (void *) &hid_std_if_desc1,
    (void *) &hid_cs_if_desc1,
    (void *) &int_ep_desc_hs1,
    NULL,
};

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT == 0
static void *const hid_if_descs_fs2[] =
{
    (void *) &hid_std_if_desc2,
    (void *) &hid_cs_if_desc2,
    (void *) &int_ep_desc_fs2,
    NULL,
};

static void *const hid_if_descs_hs2[] =
{
    (void *) &hid_std_if_desc2,
    (void *) &hid_cs_if_desc2,
    (void *) &int_ep_desc_hs2,
    NULL,
};
#endif
#endif

void *usb_hid_data_pipe_open(uint8_t ep_addr, T_USB_HID_ATTR attr, uint8_t pending_req_num,
                             USB_HID_DATA_PIPE_CB cb)
{
    T_USB_HID_DRIVER_ATTR driver_attr;
    memcpy(&driver_attr, &attr, sizeof(T_USB_HID_DRIVER_ATTR));
    if (driver_attr.mtu < int_in_ep_desc_fs0.wMaxPacketSize)
    {
        driver_attr.mtu = int_in_ep_desc_fs0.wMaxPacketSize;
    }
    if (pending_req_num < USB_HID_MAX_PENDING_REQ_NUM)
    {
        pending_req_num = USB_HID_MAX_PENDING_REQ_NUM;
    }
    return usb_hid_driver_data_pipe_open(ep_addr, driver_attr, pending_req_num, cb);
}

int usb_hid_data_pipe_send(void *handle, void *buf, uint32_t len)
{
    USB_PRINT_INFO1("usb_hid_data_pipe_send, data %b", TRACE_BINARY(len, buf));
    usb_hid_driver_data_pipe_send(handle, buf, len);
    return ESUCCESS;
}

int usb_hid_data_pipe_close(void *handle)
{
    return usb_hid_driver_data_pipe_close(handle);
}

int usb_hid_ual_register(T_HID_CBS cbs)
{
    T_HID_UAL *ual_node = malloc(sizeof(T_HID_UAL));
    memcpy(&ual_node->cbs, &cbs, sizeof(T_USB_HID_DRIVER_CBS));
    os_queue_in(&ual_list, ual_node);
    return 0;
}

int32_t usb_hid_get_report(T_HID_DRIVER_REPORT_REQ_VAL req_value, void *buf, uint16_t *len)
{
    uint32_t ret = 0;
    T_HID_UAL *ual = (T_HID_UAL *)ual_list.p_first;
    while (ual)
    {
        if (ual->cbs.get_report)
        {
            ret += ual->cbs.get_report(req_value, buf, len);
        }
        ual = ual->p_next;
    }
    return ret;
}

int32_t usb_hid_set_report(T_HID_DRIVER_REPORT_REQ_VAL req_value, void *buf, uint16_t len)
{
    uint32_t ret = 0;
    T_HID_UAL *ual = (T_HID_UAL *)ual_list.p_first;
    while (ual)
    {
        if (ual->cbs.set_report)
        {
            ret += ual->cbs.set_report(req_value, buf, len);
        }
        ual = ual->p_next;
    }
    return ret;
}

#if F_APP_USB_AUDIO_SUPPORT
void usb_hid_pipe_open(void)
{
    return;
}

void usb_hid_volume_up(void)
{
    uint8_t report[] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x01, 0x00};

    usb_hid_report_buffered_send(report, sizeof(report));
}

void usb_hid_volume_down(void)
{
    uint8_t report[] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x02, 0x00};

    usb_hid_report_buffered_send(report, sizeof(report));
}

void usb_hid_volume_release(void)
{
    uint8_t report[] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};

    usb_hid_report_buffered_send(report, sizeof(report));
}
#endif

void usb_hid_init(void)
{
    hid_inst_0 = usb_hid_driver_inst_alloc();
    usb_hid_driver_if_desc_register(hid_inst_0, (void *)hid_if_descs_hs0, (void *)hid_if_descs_fs0,
                                    (void *)report_desc0);
#if (F_APP_SS_REVISE_HID == 1)
    hid_inst_1 = usb_hid_driver_inst_alloc();
    usb_hid_driver_if_desc_register(hid_inst_1, (void *)hid_if_descs_hs1, (void *)hid_if_descs_fs1,
                                    (void *)report_desc1);

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT == 0
    hid_inst_2 = usb_hid_driver_inst_alloc();
    usb_hid_driver_if_desc_register(hid_inst_2, (void *)hid_if_descs_hs2, (void *)hid_if_descs_fs2,
                                    (void *)report_desc2);
#endif
#endif

    T_USB_HID_DRIVER_CBS cbs;
    cbs.get_report = usb_hid_get_report;
    cbs.set_report = usb_hid_set_report;
    usb_hid_driver_cbs_register(hid_inst_0, &cbs);
    usb_hid_driver_init();

#if F_APP_USB_AUDIO_SUPPORT
    T_USB_HOST_DETECT_HID_INFO hid_info =
    {
        .intf_num = hid_std_if_desc0.bInterfaceNumber,
        .ep_num = int_in_ep_desc_fs0.bEndpointAddress,
        .pipe_open = usb_hid_pipe_open,
        .volume_up = usb_hid_volume_up,
        .volume_down = usb_hid_volume_down,
        .volume_release = usb_hid_volume_release
    };
    usb_host_detect_hid_info_register(hid_info);
    app_vol_control_hid_info_register(hid_info);
#endif
}
#endif
