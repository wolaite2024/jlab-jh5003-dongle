#if F_APP_USB_GIP_SUPPORT && F_APP_USB_SUPPORT
#include <stddef.h>
#include "stdlib.h"
#include "trace.h"
#include "usb_spec20.h"
#include "usb_gip_desc.h"
#include "usb_gip_driver.h"
#include "usb_gip.h"
#include "usb_dm.h"
#include "string.h"
#include "section.h"
#include "GipConfig.h"
#include "os_mem.h"
#include "usb_audio_config.h"
#include "usb_audio.h"

static T_USB_GIP_PIPES *usb_gip_pipe = NULL;
static void *inst1 = NULL;
static void *inst2 = NULL;
uint8_t *gip_audio_in_buf = NULL;
uint8_t *gip_audio_out_buf = NULL;
uint32_t upstream_pkt_len = 0;
uint32_t downstream_pkt_len = 0;

#define USB_GIP_PIPE_CALLBACK(cb, pipe, param1, param2) \
    if(cb)                                                     \
    {                                                          \
        cb(pipe, param1, param2);                              \
    }

static T_GIP_STD_IF_DESC gip_data_std_if_desc =
{
    .bLength            = sizeof(T_GIP_STD_IF_DESC),
    .bDescriptorType    = GIP_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 2,
    .bInterfaceClass    = GIP_CLASS_CODE,
    .bInterfaceSubClass = GIP_SUBCLASS_CODE,
    .bInterfaceProtocol = GIP_PROTOCOL,
    .iInterface         = 0,
};

static T_GIP_STD_EP_DESC gip_data_int_out_ep_desc =
{
    .bLength           = sizeof(T_GIP_STD_EP_DESC),
    .bDescriptorType   = GIP_DT_ENDPOINT,
    .bEndpointAddress  = GIP_INT_OUT_ENDPOINT_ADDRESS,
    .bmAttributes      = GIP_EP_XFER_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 4,   //  Polling interval in milliseconds: must be = 4.
};

static T_GIP_STD_EP_DESC gip_data_int_in_ep_desc =
{
    .bLength           = sizeof(T_GIP_STD_EP_DESC),
    .bDescriptorType   = GIP_DT_ENDPOINT,
    .bEndpointAddress  = GIP_INT_IN_ENDPOINT_ADDRESS,
    .bmAttributes      = GIP_EP_XFER_INT,
    .wMaxPacketSize    = 0x40,
    .bInterval         = 4,   // must be >= 4
};

static T_GIP_STD_IF_DESC gip_audio_std_if_alt0_desc =
{
    .bLength            = sizeof(T_GIP_STD_IF_DESC),
    .bDescriptorType    = GIP_DT_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = GIP_CLASS_CODE,
    .bInterfaceSubClass = GIP_SUBCLASS_CODE,
    .bInterfaceProtocol = GIP_PROTOCOL,
    .iInterface         = 0,
};

static T_GIP_STD_IF_DESC gip_audio_std_if_alt1_desc =
{
    .bLength            = sizeof(T_GIP_STD_IF_DESC),
    .bDescriptorType    = GIP_DT_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 1,
    .bNumEndpoints      = 2,
    .bInterfaceClass    = GIP_CLASS_CODE,
    .bInterfaceSubClass = GIP_SUBCLASS_CODE,
    .bInterfaceProtocol = GIP_PROTOCOL,
    .iInterface         = 0,
};

static T_GIP_STD_EP_DESC gip_audio_iso_out_ep_desc =
{
    .bLength            = sizeof(T_GIP_STD_EP_DESC),
    .bDescriptorType    = GIP_DT_ENDPOINT,
    .bEndpointAddress   = GIP_ISO_OUT_ENDPOINT_ADDRESS,
    .bmAttributes       = GIP_EP_XFER_ISOC,
    .wMaxPacketSize     = 0xe0,
    .bInterval          = 1,   // full speed
};

static T_GIP_STD_EP_DESC gip_audio_iso_in_ep_desc =
{
    .bLength            = sizeof(T_GIP_STD_EP_DESC),
    .bDescriptorType    = GIP_DT_ENDPOINT,
    .bEndpointAddress   = GIP_ISO_IN_ENDPOINT_ADDRESS,
    .bmAttributes       = GIP_EP_XFER_ISOC,
    .wMaxPacketSize     = 0xe0,
    .bInterval          = 1,   // full speed
};

static T_GIP_MS_EXTD_CMP_ID_DESC gip_ms_extended_compatible_id_desc =
{
    .dwLength = sizeof(T_GIP_MS_EXTD_CMP_ID_DESC),
    .bcdVersion = 0x0100,
    .wIndex = 0x0004,
    .bCount = 0x01,
    .bFirstInterfaceNumber = 0,
    .bNumInterfaces = 0x2,   // 0x1: gip game controller without audio, 0x2: with audio
    .compatibleID = 0x0000303150494758,
    .subCompatibleID = 0x0,
};

static T_GIP_MS_OS_STRING_DESC gip_ms_os_string_desc =
{
    .bLength = sizeof(T_GIP_MS_OS_STRING_DESC),
    .bDescriptorType = GIP_DT_STRING,
    .qwSignature[0] = 0x4d,
    .qwSignature[1] = 0x0,
    .qwSignature[2] = 0x53,
    .qwSignature[3] = 0x0,
    .qwSignature[4] = 0x46,
    .qwSignature[5] = 0x0,
    .qwSignature[6] = 0x54,
    .qwSignature[7] = 0x0,
    .qwSignature[8] = 0x31,
    .qwSignature[9] = 0x0,
    .qwSignature[10] = 0x30,
    .qwSignature[11] = 0x0,
    .qwSignature[12] = 0x30,
    .qwSignature[13] = 0x0,
    .bMS_VendorCode = GIP_MS_VENDORCODE,
    .bPad = 0,
};

static const void *gip_data_descs_fs[] =
{
    (void *) &gip_data_std_if_desc,
    (void *) &gip_data_int_out_ep_desc,
    (void *) &gip_data_int_in_ep_desc,
    NULL,
};

static const void *gip_data_descs_hs[] =
{
    (void *) &gip_data_std_if_desc,
    (void *) &gip_data_int_out_ep_desc,
    (void *) &gip_data_int_in_ep_desc,
    NULL,
};

static const void *gip_audio_descs_fs[] =
{
    (void *) &gip_audio_std_if_alt0_desc,
    (void *) &gip_audio_std_if_alt1_desc,
    (void *) &gip_audio_iso_out_ep_desc,
    (void *) &gip_audio_iso_in_ep_desc,
    NULL,
};

static const void *gip_audio_descs_hs[] =
{
    (void *) &gip_audio_std_if_alt0_desc,
    (void *) &gip_audio_std_if_alt1_desc,
    (void *) &gip_audio_iso_out_ep_desc,
    (void *) &gip_audio_iso_in_ep_desc,
    NULL,
};

static int32_t vol_attr_set_spk(int value)
{
    // host will send volume range 0~100 to device on GIP
    int32_t vol = (int32_t)value;
    T_USB_GIP_PIPE_ATTR attr = {.dir = USB_GIP_DIR_OUT, .content.vol.value = vol};
    APP_PRINT_INFO2("vol_attr_set_spk, value:0x%x, vol 0x%x", value, vol);
    USB_GIP_PIPE_CALLBACK(usb_gip_pipe->ctrl, usb_gip_pipe, USB_AUDIO_CTRL_EVT_VOL_SET, attr);

    return 0;
}

static int32_t vol_attr_set_mic(int value)
{
    int32_t vol = value;// - ((int32_t *)(*(int32_t *)ctrl))[UAC1_MIN - 1];
    T_USB_GIP_PIPE_ATTR attr = {.dir = USB_GIP_DIR_IN, .content.vol.value = vol};
    APP_PRINT_INFO1("vol_attr_set_mic, value:0x%x", value);
    USB_GIP_PIPE_CALLBACK(usb_gip_pipe->ctrl, usb_gip_pipe, USB_AUDIO_CTRL_EVT_VOL_SET, attr);
    return 0;

}

RAM_TEXT_SECTION
static int usb_gip_upstream(uint8_t *buf, uint16_t len)
{
    USB_PRINT_INFO0("usb_gip_upstream");

    uint32_t offset = 0;
    uint32_t pkt_len = upstream_pkt_len;   // sample_rate * channel_num * bit_width
    uint32_t pkt_cnt = USB_AUDIO_US_INTERVAL;
    GIP_AUDIO_IN_HEADER header;
    USB_GIP_PIPE_CALLBACK(usb_gip_pipe->upstream, usb_gip_pipe, gip_audio_in_buf,
                          len - pkt_cnt * GIP_AUDIO_HEADER_LEN);

    for (int desc_idx = 0; desc_idx < pkt_cnt; desc_idx++)
    {
        if (len != 0)
        {
            // obtain header
            GipDeviceGetHeader(&header);
            memcpy((buf + offset), &header, GIP_AUDIO_HEADER_LEN);
            memcpy((buf + offset + GIP_AUDIO_HEADER_LEN), gip_audio_in_buf + desc_idx * pkt_len, pkt_len);
            offset += (pkt_len + GIP_AUDIO_HEADER_LEN);
        }
    }
    return 0;
}

RAM_TEXT_SECTION
static int usb_gip_downstream(uint8_t *buf, uint16_t len)
{
    USB_PRINT_INFO0("usb_gip_downstream");

    uint32_t offset = 0;
    uint32_t pkt_len = downstream_pkt_len;  // sample_rate * channel_num * bit_width
    uint32_t pkt_cnt = USB_AUDIO_DS_INTERVAL;
    uint32_t actual_len = pkt_len * pkt_cnt;
    GIP_AUDIO_OUT_HEADER *header = NULL;

    for (int desc_idx = 0; desc_idx < pkt_cnt; desc_idx++)
    {
        if (len != 0)
        {
            // remove header
            header = (GIP_AUDIO_OUT_HEADER *)(buf + desc_idx * (pkt_len + GIP_AUDIO_HEADER_LEN));
            if (header->MessageType == 0x60 && (header->Flags & 0x20))
            {
                memcpy(gip_audio_out_buf + offset, buf + offset + (desc_idx + 1) * GIP_AUDIO_HEADER_LEN, pkt_len);
                offset += pkt_len;
            }
        }
    }

    USB_GIP_PIPE_CALLBACK(usb_gip_pipe->downstream, usb_gip_pipe, (uint8_t *)gip_audio_out_buf,
                          actual_len);

    return 0;
}

static int usb_gip_activate(uint8_t dir, uint8_t bit_res, uint32_t sample_rate, uint8_t chan_num)
{
    if (dir == 1)  //out
    {
        downstream_pkt_len = bit_res / 8 * sample_rate * chan_num / 1000;
    }
    else if (dir == 2) // in
    {
        upstream_pkt_len = bit_res / 8 * sample_rate * chan_num / 1000;
    }
    T_USB_AUDIO_CTRL_EVT evt = USB_AUDIO_CTRL_EVT_ACTIVATE;
    T_USB_GIP_PIPE_ATTR ctrl =
    {
        .dir = dir,
        .content.audio =
        {
            .bit_width = bit_res,
            .channels = chan_num,
            .sample_rate = sample_rate,
        },
    };
    USB_GIP_PIPE_CALLBACK(usb_gip_pipe->ctrl, usb_gip_pipe, evt, ctrl);
    return 0;
}

static int usb_gip_deactivate(uint8_t dir)
{
    T_USB_AUDIO_CTRL_EVT evt = USB_AUDIO_CTRL_EVT_DEACTIVATE;
    T_USB_GIP_PIPE_ATTR attr = {.dir = dir, .content.d32 = 0};
    USB_GIP_PIPE_CALLBACK(usb_gip_pipe->ctrl, usb_gip_pipe, evt, attr);
    return 0;
}

void *usb_gip_data_pipe_open(uint8_t ep_addr, T_USB_GIP_ATTR attr, uint8_t pending_req_num,
                             USB_GIP_DATA_PIPE_CB cb)
{
    T_USB_GIP_DRIVER_ATTR driver_attr;
    memcpy(&driver_attr, &attr, sizeof(T_USB_GIP_DRIVER_ATTR));
    return usb_gip_driver_data_pipe_open(ep_addr, driver_attr, pending_req_num, cb);
}

bool usb_gip_data_pipe_send(void *handle, void *buf, uint32_t len)
{
    usb_gip_driver_data_pipe_send(handle, buf, len);
    return true;
}

int usb_gip_data_pipe_close(void *handle)
{
    return usb_gip_driver_data_pipe_close(handle);
}


void usb_gip_init(T_USB_GIP_PIPES *pipe)
{
    usb_gip_pipe = pipe;
    gip_audio_in_buf = (uint8_t *)malloc(ISO_IN_MAX_BUF_SIZE);
    gip_audio_out_buf = (uint8_t *)malloc(ISO_OUT_MAX_BUF_SIZE);

    usb_gip_driver_ms_desc_register((void *) &gip_ms_os_string_desc,
                                    (void *) &gip_ms_extended_compatible_id_desc);

    inst1 = usb_gip_driver_inst_alloc();
    usb_gip_driver_if_desc_register(inst1, (T_USB_GIP_DRIVER_DESC_HDR **)gip_data_descs_hs,
                                    (T_USB_GIP_DRIVER_DESC_HDR **)gip_data_descs_fs);

    inst2 = usb_gip_driver_inst_alloc();
    usb_gip_driver_if_desc_register(inst2, (T_USB_GIP_DRIVER_DESC_HDR **)gip_audio_descs_hs,
                                    (T_USB_GIP_DRIVER_DESC_HDR **)gip_audio_descs_fs);

    T_USB_GIP_DRIVER_XMIT_CBS cbs;
    cbs.upstream = usb_gip_upstream;
    cbs.downstream = usb_gip_downstream;
    usb_gip_driver_cb_register(inst2, &cbs, USB_AUDIO_DS_INTERVAL, USB_AUDIO_US_INTERVAL);

    T_USB_GIP_AUDIO_CBS audio_cbs;
    audio_cbs.activate = (USB_GIP_AUDIO_CB_ACTIVATE)usb_gip_activate;
    audio_cbs.deactivate = (USB_GIP_AUDIO_CB_DEACTIVATE)usb_gip_deactivate;
    audio_cbs.volset = (USB_GIP_AUDIO_CB_VOLSET)vol_attr_set_spk;
    audio_cbs.micset = (USB_GIP_AUDIO_CB_MICSET)vol_attr_set_mic;
    GipDeviceCallbackRegister((T_GIP_AUDIO_CBS *)&audio_cbs);

    usb_gip_driver_init();
}



#endif
