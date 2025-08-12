#if F_APP_USB_UAC_SUPPORT && F_APP_USB_SUPPORT && ENABLE_UAC2
#include "usb_audio_config.h"
#if(USB_AUDIO_VERSION == USB_AUDIO_VERSION_1)
#include <stddef.h>
#include "trace.h"
#include "section.h"
#include "usb_audio1_spec.h"
#include "hal_adp.h"
#include "usb_spec20.h"
#include "usb_audio_driver.h"
#include "usb_audio.h"
#include "usb_dev.h"
#include "app_ipc.h"
#include "usb_msg.h"

static T_USB_AUDIO_PIPES *usb_audio1_pipe_2 = NULL;
static void *inst_2 = NULL;
static T_USB_AUDIO_DRIVER_CTRL_ATTR uac1_drv_ctrl_attr_2 = {.data = NULL, .len = 0};
#if UAC_SPK_SUPPORT
static bool get_range_spk = false;
#endif

#define USB_AUDIO1_PIPE_CALLBACK(cb, pipe, param1, param2) \
    if(cb)                                                     \
    {                                                          \
        cb(pipe, param1, param2);                              \
    }

#define UAC1_ISO_OUT_ENDPOINT_ADDRESS_2    0x04

#if UAC_SPK_SUPPORT
static const T_UAC1_IT_DESC     input_terminal_desc0 =
{
    .bLength            = sizeof(T_UAC1_IT_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_INPUT_TERMINAL,
    .bTerminalID        = UAC_SPK_2_ID_INPUT_TERMINAL,
    .wTerminalType      = UAC1_TERMINAL_STREAMING,
    .bAssocTerminal     = 0,
    .bNrChannels        = UAC1_SPK_2_CHAN_NUM,
    .wChannelConfig     = 3,
    .iChannelNames      = 0,
    .iTerminal          = 0,
};

static const T_UAC1_OT_DESC     output_terminal_desc0 =
{
    .bLength            = sizeof(T_UAC1_OT_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_OUTPUT_TERMINAL,
    .bTerminalID        = UAC_SPK_2_ID_OUTPUT_TERMINAL,
    .wTerminalType      = UAC1_OUTPUT_TERMINAL_COMMUNICATION_SPEAKER,
    .bAssocTerminal     = 0,
    .bSourceID          = UAC_SPK_2_ID_FEATURE_UNIT,
    .iTerminal          = 0,
};

static const T_UAC1_FEATURE_UNIT_DESC(UAC_SPK_2_ID_FEATURE_UNIT, 3)    feature_uint_desc0 =
{
    .bLength            = sizeof(feature_uint_desc0),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_FEATURE_UNIT,
    .bUnitID            = UAC_SPK_2_ID_FEATURE_UNIT,
    .bSourceID          = UAC_SPK_2_ID_INPUT_TERMINAL,
    .bControlSize       = 1,
    .bmaControls[0]     = UAC_CONTROL_BIT(UAC1_FU_MUTE_CONTROL) | UAC_CONTROL_BIT(UAC1_FU_VOLUME_CONTROL),
    .bmaControls[1]     = 0,
    .bmaControls[2]     = 0,
    .iFeature           = 0,
};
#endif

#if UAC_SPK_SUPPORT
static T_UAC1_STD_IF_DESC     interface_alt0_desc1 =
{
    .bLength            = sizeof(T_UAC1_STD_IF_DESC),
    .bDescriptorType    = UAC1_DT_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = UAC1_CLASS_CODE_AUDIO,
    .bInterfaceSubClass = UAC1_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static T_UAC1_STD_IF_DESC     interface_alt1_desc1 =
{
    .bLength            = sizeof(T_UAC1_STD_IF_DESC),
    .bDescriptorType    = UAC1_DT_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 1,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = UAC1_CLASS_CODE_AUDIO,
    .bInterfaceSubClass = UAC1_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
};

static const T_UAC1_AS_HDR_DESC    as1_header_desc1 =
{
    .bLength            = sizeof(T_UAC1_AS_HDR_DESC),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_AS_GENERAL,
    .bTerminalLink      = UAC_SPK_2_ID_INPUT_TERMINAL,
    .bDelay             = 1,
    .wFormatTag         = UAC1_FORMAT_TYPE_I_PCM,
};

static T_UAC1_FMT_TYPE_I_DESC(spk, UAC1_SPK_2_SAM_RATE_NUM) format_type_i_desc1 =
{
    .bLength            = sizeof(format_type_i_desc1),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_FORMAT_TYPE,
    .bFormatType        = UAC1_FORMAT_TYPE_I_PCM,
    .bNrChannels        = UAC1_SPK_2_CHAN_NUM,
    .bSubframeSize      = UAC1_SPK_2_BIT_RES / 8,
    .bBitResolution     = UAC1_SPK_2_BIT_RES,
    .bSamFreqType       = UAC1_SPK_2_SAM_RATE_NUM,
    .tSamFreq           =
    {
        [0] = {[0] = 0x80, [1] = 0xBB, [2] = 0x00},
    },
};

static T_UAC1_STD_ISO_EP_DESC     out_ep_desc_hs =
{
    .bLength            = sizeof(T_UAC1_STD_ISO_EP_DESC),
    .bDescriptorType    = UAC1_DT_ENDPOINT,
    .bEndpointAddress   = UAC1_ISO_OUT_ENDPOINT_ADDRESS_2,
    .bmAttributes       = UAC1_EP_XFER_ISOC | UAC1_EP_SYNC_ADAPTIVE,
    .wMaxPacketSize     = UAC_SPK2_MAX_PACKET_SIZE,
    .bInterval          = 4,
    .bRefresh           = 0,
    .bSynchAddress      = 0,
};

static T_UAC1_STD_ISO_EP_DESC     out_ep_desc_fs =
{
    .bLength            = sizeof(T_UAC1_STD_ISO_EP_DESC),
    .bDescriptorType    = UAC1_DT_ENDPOINT,
    .bEndpointAddress   = UAC1_ISO_OUT_ENDPOINT_ADDRESS_2,
    .bmAttributes       = UAC1_EP_XFER_ISOC | UAC1_EP_SYNC_ADAPTIVE,
    .wMaxPacketSize     = UAC_SPK2_MAX_PACKET_SIZE,
    .bInterval          = 1,
    .bRefresh           = 0,
    .bSynchAddress      = 0,
};

static const T_UAC1_ISO_EP_DESC iso_out_ep_desc =
{
    .bLength            = sizeof(T_UAC1_ISO_EP_DESC),
    .bDescriptorType    = UAC1_DT_CS_ENDPOINT,
    .bDescriptorSubtype = UAC1_EP_GENERAL,
    .bmAttributes       = 1,
    .bLockDelayUnits    = 1,
    .wLockDelay         = 1,
};

#endif

static T_UAC1_STD_IF_DESC     ac_interface_desc =
{
    .bLength            = sizeof(T_UAC1_STD_IF_DESC),
    .bDescriptorType    = UAC1_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = UAC1_CLASS_CODE_AUDIO,
    .bInterfaceSubClass = UAC1_SUBCLASS_AUDIOCONTROL,
    .bInterfaceProtocol = 0,
    .iInterface         = STRING_ID_UAC_2st,
};

static T_UAC1_AC_HDR_DESC(0, UAC1_STREAM_2_INTF_NUM) ac_hdr_desc =
{
    .bLength            = sizeof(ac_hdr_desc),
    .bDescriptorType    = UAC1_DT_CS_INTERFACE,
    .bDescriptorSubtype = UAC1_HEADER,
    .bcdADC             = 0x0100,
    .wTotalLength       = sizeof(ac_hdr_desc)
#if UAC_SPK_SUPPORT
    + sizeof(input_terminal_desc0)
    + sizeof(output_terminal_desc0)
    + sizeof(feature_uint_desc0)
#endif
    ,
    .bInCollection      = UAC1_STREAM_2_INTF_NUM,
    .baInterfaceNr[0]   = 4,
};

static const void  *uac1_descs_ctrl[] =
{
    (void *) &ac_interface_desc,
    (void *) &ac_hdr_desc,
#if UAC_SPK_SUPPORT
    (void *) &input_terminal_desc0,
    (void *) &output_terminal_desc0,
    (void *) &feature_uint_desc0,
#endif
    NULL
};

#if UAC_SPK_SUPPORT
static const void  *uac1_descs_spk_hs[] =
{
    (void *) &interface_alt0_desc1,
    (void *) &interface_alt1_desc1,
    (void *) &as1_header_desc1,
    (void *) &format_type_i_desc1,
    (void *) &out_ep_desc_hs,
    (void *) &iso_out_ep_desc,
    NULL
};

static const void  *uac1_descs_spk_fs[] =
{
    (void *) &interface_alt0_desc1,
    (void *) &interface_alt1_desc1,
    (void *) &as1_header_desc1,
    (void *) &format_type_i_desc1,
    (void *) &out_ep_desc_fs,
    (void *) &iso_out_ep_desc,
    NULL
};
#endif

#if UAC_SPK_SUPPORT
static int32_t vol_attr_set_spk(T_USB_AUDIO_DRIVER_CTRL_ATTR *drv_attr, uint8_t cmd, int value)
{
    T_CTRL_ATTR *vol_attr = (T_CTRL_ATTR *)drv_attr->data;
    int32_t vol = value; // - ((int32_t *)(vol_attr->data))[UAC1_MIN - 1]; // todo
    vol_attr->attr.cur = value;
    T_USB_AUDIO_PIPE_ATTR ctrl = {.dir = USB_AUDIO_DIR_OUT, .content.vol.type = USB_AUDIO_VOL_TYPE_INVALID};
    APP_PRINT_INFO2("vol_attr_set_spk_2,cmd:0x%x, value:0x%x", cmd, value);

    if (get_range_spk == false)
    {
        get_range_spk = true;
        ctrl.content.vol.type = USB_AUDIO_VOL_TYPE_RANGE;
        ctrl.content.vol.value = vol_attr->attr.max - vol_attr->attr.min;
        USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->ctrl, usb_audio1_pipe_2, USB_AUDIO_CTRL_EVT_VOL_GET,
                                 ctrl);
    }

    if (cmd == UAC1_CUR)
    {
        ctrl.content.vol.type = USB_AUDIO_VOL_TYPE_CUR;
        ctrl.content.vol.value = vol;
        USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->ctrl, usb_audio1_pipe_2, USB_AUDIO_CTRL_EVT_VOL_SET,
                                 ctrl);
    }
    else if (cmd == UAC1_RES)
    {
        ctrl.content.vol.type = USB_AUDIO_VOL_TYPE_RES;
        USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->ctrl, usb_audio1_pipe_2, USB_AUDIO_CTRL_EVT_VOL_SET,
                                 ctrl);
    }

    return 0;

}

static T_USB_AUDIO_DRIVER_CTRL_ATTR *vol_attr_get_spk(T_USB_AUDIO_DRIVER_CTRL_ATTR *drv_attr,
                                                      uint8_t cmd)
{
    T_CTRL_ATTR *vol_attr = (T_CTRL_ATTR *)drv_attr->data;
    int32_t data = ((int32_t *)(vol_attr->data))[cmd - 1];
    uac1_drv_ctrl_attr_2.data = vol_attr->data + cmd - 1;
    uac1_drv_ctrl_attr_2.len = sizeof(data);

    T_USB_AUDIO_PIPE_ATTR ctrl = {.dir = USB_AUDIO_DIR_OUT, .content.vol.type = USB_AUDIO_VOL_TYPE_INVALID};
    if (cmd == UAC1_CUR)
    {
        ctrl.content.vol.type = USB_AUDIO_VOL_TYPE_CUR;
        ctrl.content.vol.value = data - vol_attr->attr.min;
        USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->ctrl, usb_audio1_pipe_2, USB_AUDIO_CTRL_EVT_VOL_GET,
                                 ctrl);
    }

    if (get_range_spk == false)
    {
        get_range_spk = true;
        ctrl.content.vol.type = USB_AUDIO_VOL_TYPE_RANGE;
        ctrl.content.vol.value = vol_attr->attr.max - vol_attr->attr.min;
        USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->ctrl, usb_audio1_pipe_2, USB_AUDIO_CTRL_EVT_VOL_GET,
                                 ctrl);
    }

    APP_PRINT_INFO3("vol_attr_get_spk_2,cmd:0x%x-0x%x-0x%x", cmd, data, drv_attr);
    return &uac1_drv_ctrl_attr_2;
}

static int32_t mute_attr_set(T_USB_AUDIO_DRIVER_CTRL_ATTR *attr, uint8_t cmd, int value)
{
    T_CTRL_ATTR *mute_attr = (T_CTRL_ATTR *)attr->data;
    T_USB_AUDIO_PIPE_ATTR ctrl = {.dir = USB_AUDIO_DIR_OUT, .content.mute.value = value};
    mute_attr->attr.cur = value;
    APP_PRINT_INFO2("mute_attr_set_2,cmd:0x%x, value:0x%x", cmd, value);
    USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->ctrl, usb_audio1_pipe_2, USB_AUDIO_CTRL_EVT_MUTE_SET,
                             ctrl);
    return 0;

}

static T_USB_AUDIO_DRIVER_CTRL_ATTR *mute_attr_get(T_USB_AUDIO_DRIVER_CTRL_ATTR *drv_attr,
                                                   uint8_t cmd)
{
    T_CTRL_ATTR *mute_attr = (T_CTRL_ATTR *)drv_attr->data;
    int32_t data = mute_attr->attr.cur;
    T_USB_AUDIO_PIPE_ATTR ctrl = {.dir = USB_AUDIO_DIR_OUT, .content.mute.value = data};
    uac1_drv_ctrl_attr_2.data = &(mute_attr->attr.cur);
    uac1_drv_ctrl_attr_2.len = sizeof(data);
    USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->ctrl, usb_audio1_pipe_2, USB_AUDIO_CTRL_EVT_MUTE_GET,
                             ctrl);
    APP_PRINT_INFO2("mute_attr_get_2,cmd:0x%x, value:0x%x", cmd, data);
    return &uac1_drv_ctrl_attr_2;
}
#endif

/******************************************************************/
#if UAC_SPK_SUPPORT
static T_CTRL_ATTR vol_attr_spk =
{
    .attr =
    {
        .cur = UAC1_SPK_VOL_CUR,
        .min = UAC1_SPK_VOL_MIN,
        .max = UAC1_SPK_VOL_MAX,
        .res = UAC1_SPK_VOL_RES,
    }
};

static T_CTRL_ATTR mute_attr_spk =
{
    .attr =
    {
        .cur = 0,
        .min = 0,
        .max = 1,
        .res = 1,
    }
};

static const T_USB_AUDIO_DRIVER_CTRL  vol_ctrl_spk =
{
    .type = UAC1_FU_VOLUME_CONTROL,
    .attr = {.data = (void *) &vol_attr_spk, .len = sizeof(vol_attr_spk)},
    .get = (T_USB_AUDIO_CTRL_GET_FUNC)vol_attr_get_spk,
    .set = (T_USB_AUDIO_CTRL_SET_FUNC)vol_attr_set_spk,
};

static const T_USB_AUDIO_DRIVER_CTRL  mute_ctrl_spk =
{
    .type = UAC1_FU_MUTE_CONTROL,
    .attr = {.data = (void *) &mute_attr_spk, .len = sizeof(mute_attr_spk)},
    .get = (T_USB_AUDIO_CTRL_GET_FUNC)mute_attr_get,
    .set = (T_USB_AUDIO_CTRL_SET_FUNC)mute_attr_set,
};
#endif

#if UAC_SPK_SUPPORT
static const T_USB_AUDIO_DRIVER_CTRL_ENTITY(UAC_SPK_2_ID_FEATURE_UNIT,
                                            UAC1_SPK_2_UNIT_CTRL_NUM) uac1_ctrl_entity0_2 =
{
    .entity_id = UAC_SPK_2_ID_FEATURE_UNIT,
    .ctrl_num = UAC1_SPK_2_UNIT_CTRL_NUM,
    .ctrls[0] = (T_USB_AUDIO_DRIVER_CTRL *) &vol_ctrl_spk,
    .ctrls[1] = (T_USB_AUDIO_DRIVER_CTRL *) &mute_ctrl_spk,
};
#endif

static const T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR *ctrls[] =
{
    (const T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR *) &uac1_ctrl_entity0_2,
    NULL
};

RAM_TEXT_SECTION
static int usb_audio_upstream(uint8_t *buf, uint16_t len)
{
    USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->upstream, usb_audio1_pipe_2, buf, len);
    return 0;
}

RAM_TEXT_SECTION
static int usb_audio_downstream(uint8_t *buf, uint16_t len)
{
    USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->downstream, usb_audio1_pipe_2, buf, len);
    return 0;
}

static int usb_audio_activate(uint8_t dir, uint8_t bit_res, uint32_t sample_rate, uint8_t chan_num)
{
    T_USB_AUDIO_CTRL_EVT evt = USB_AUDIO_CTRL_EVT_ACTIVATE;
    T_USB_AUDIO_PIPE_ATTR ctrl =
    {
        .dir = dir,
        .content.audio =
        {
            .bit_width = bit_res,
            .channels = chan_num,
            .sample_rate = sample_rate,
        },
    };
    USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->ctrl, usb_audio1_pipe_2, evt, ctrl);
    return 0;
}

static int usb_audio_deactivate(uint8_t dir)
{
    T_USB_AUDIO_CTRL_EVT evt = USB_AUDIO_CTRL_EVT_DEACTIVATE;
    T_USB_AUDIO_PIPE_ATTR ctrl = {.dir = dir, .content.d32 = 0};
    USB_AUDIO1_PIPE_CALLBACK(usb_audio1_pipe_2->ctrl, usb_audio1_pipe_2, evt, ctrl);
    return 0;
}

static void usb_audio_adp_state_change_cb(T_ADP_PLUG_EVENT event, void *user_data)
{
    if (event == ADP_EVENT_PLUG_OUT)
    {
#if UAC_SPK_SUPPORT
        vol_attr_spk.attr.cur = UAC1_SPK_VOL_CUR;
        mute_attr_spk.attr.cur = 0;
        get_range_spk = false;
#endif
    }
}

void usb_audio_sec_init(T_USB_AUDIO_PIPES *pipe)
{
    usb_audio1_pipe_2 = pipe;
    inst_2 = usb_audio_driver_inst_alloc(USB_AUDIO_VERSION_1, USB_AUDIO_SEC_DS_INTERVAL, 0);
    usb_audio_driver_desc_register(inst_2, (T_USB_AUDIO_DRIVER_DESC_HDR **)uac1_descs_ctrl,
                                   (T_USB_AUDIO_DRIVER_DESC_HDR **)uac1_descs_ctrl);
    usb_audio_driver_desc_register(inst_2, (T_USB_AUDIO_DRIVER_DESC_HDR **)uac1_descs_spk_fs,
                                   (T_USB_AUDIO_DRIVER_DESC_HDR **)uac1_descs_spk_hs);
    usb_audio_driver_ctrl_register(inst_2, (T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR **)ctrls);
    T_USB_AUDIO_DRIVER_ATTR attr;
#if UAC_SPK_SUPPORT
    attr.dir = USB_AUDIO_DIR_OUT;
    attr.chann_num = 0;
    attr.bit_width = 0;
    attr.cur_sample_rate = 0;
    attr.max_sample_rate = 0;
    usb_audio_driver_attr_init(inst_2, 0, attr);

    attr.dir = USB_AUDIO_DIR_OUT;
    attr.chann_num = UAC1_SPK_2_CHAN_NUM;
    attr.bit_width = UAC1_SPK_2_BIT_RES;
    attr.cur_sample_rate = UAC_SAM_RATE_48000;
    attr.max_sample_rate = UAC1_SPK_2_SAM_RATE_MAX;
    usb_audio_driver_attr_init(inst_2, 1, attr);
#endif

    T_USB_AUDIO_DRIVER_CBS cbs;
    cbs.activate = (USB_AUDIO_DRV_CB_ACTIVATE)usb_audio_activate;
    cbs.deactivate = (USB_AUDIO_DRV_CB_DEACTIVATE)usb_audio_deactivate;
    cbs.upstream = usb_audio_upstream;
    cbs.downstream = usb_audio_downstream;
    cbs.feedback_d = NULL;
    usb_audio_driver_cb_register(inst_2, &cbs);

    adp_register_state_change_cb(ADP_DETECT_5V, (P_ADP_PLUG_CBACK)usb_audio_adp_state_change_cb, NULL);
}
#endif
#endif

