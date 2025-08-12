#include <string.h>
#include <stdlib.h>
#include "errno.h"
#include "usb_spec20.h"
#include "usb_composite_dev.h"
#include "usb_utils.h"
#include "usb_isr.h"
#include "usb_audio_driver.h"
#include "usb_audio1_spec.h"
//#include "usb_audio2_spec.h"
#include "usb_pipe.h"
#include "trace.h"
#include "section.h"

static int fail_line = 0;

typedef enum {SET_CMD_UINT_IF, SET_CMD_UINT_EP, SET_CMD_UINT_INVALID} T_SET_CMD_UINT;
typedef enum {FREQ_CTRL_TYPE_IN, FREQ_CTRL_TYPE_OUT, FREQ_CTRL_TYPE_INVALID} T_FREQ_CTRL_TYPE;

#define FEEDBACK_PKT_SIZE_3B      (3)
#define FEEDBACK_PKT_SIZE_4B      (4)

typedef struct _control_entity_item
{
    struct _control_entity_item *p_next;
    T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR *ctrl_entity;
} T_CONTROL_ENTITY_ITEM;

typedef struct _set_cmd
{
    uint8_t uac_ver;
    T_SET_CMD_UINT uint;
    int cmd;
    void *priv;
} T_SET_CMD;

typedef struct _ep_priv
{
    void *fb_ep_handle;
    void *hs_feedback_ep_desc;
    void *fs_feedback_ep_desc;

    T_HAL_USB_ISO_REQUEST_BLOCK *fb_iso_urb;
} T_EP_PRIV;

typedef struct _alt_setting
{
    struct _alt_setting *p_next;

    uint8_t value;
    T_USB_ENDPOINT_DESC *hs_ep_desc;
    T_USB_ENDPOINT_DESC *fs_ep_desc;

    T_USB_AUDIO_DRIVER_ATTR attr;
} T_ALT_SETTING;

typedef struct _alt_settings
{
    union
    {
        uint32_t data;
        struct
        {
            uint32_t freq: 24;
            uint32_t rsv: 8;
        } uac2;
    } cs;
    T_USB_EP *ep;
    T_ALT_SETTING   *cur;
    T_USB_UTILS_LIST    list;
    void *owner;
} T_ALT_SETTINGS;

typedef struct _ac_topology
{
    void *pipe_tx;
    T_USB_ENDPOINT_DESC *hs_ep_desc;
    T_USB_ENDPOINT_DESC *fs_ep_desc;
    T_USB_UTILS_LIST ctrl_entity_list;
} T_AC_TOPOLOGY;

typedef struct _usb_audio_if
{
    struct _usb_audio_if *p_next;
    uint8_t dir;
    uint8_t sub_class;
    uint8_t cur_alt;

    T_USB_INTERFACE interface;
    union
    {
        T_ALT_SETTINGS *alt_settings;
        T_AC_TOPOLOGY *ac_topology;
    } priv;

    void *owner;
} T_USB_AUDIO_IF;

typedef struct _usb_audio_inst
{
    struct _usb_audio_inst *p_next;
    uint8_t uac_ver;
    T_USB_UTILS_LIST if_list;
    T_USB_AUDIO_DRIVER_CBS cbs;

    T_HAL_USB_ISO_REQUEST_BLOCK *iso_in_urb;
    uint8_t iso_in_proc_interval;
    uint16_t iso_in_max_buf_size;

    T_HAL_USB_ISO_REQUEST_BLOCK *iso_out_urb;
    uint8_t iso_out_proc_interval;
    uint16_t iso_out_max_buf_size;
    uint8_t *copy_buf;

} T_USB_AUDIO_INST;

typedef struct _usb_audio
{
    T_USB_UTILS_LIST insts;
} T_USB_AUDIO;

static T_USB_AUDIO g_usb_audio;

RAM_TEXT_SECTION
static int usb_audio_driver_iso_in_xfer_done(T_HAL_USB_ISO_REQUEST_BLOCK *iso_urb, uint8_t buf_num)
{
    uint8_t pkt_cnt = iso_urb->pkt_cnt;
    T_HAL_USB_ISO_PKT_INFO *iso_pkt = (buf_num == 0) ? iso_urb->iso_pkt0 : iso_urb->iso_pkt1;
    T_USB_AUDIO_INST *inst = (T_USB_AUDIO_INST *)iso_urb->priv;
    uint8_t *buf = (buf_num == 0) ? iso_urb->buf0 : iso_urb->buf1;
    uint32_t len = 0;
    for (uint8_t i = 0; i < pkt_cnt; i++)
    {
        len +=  iso_pkt[i].actual;
    }
    if (inst && inst->cbs.upstream)
    {
        inst->cbs.upstream(buf, len);
    }

    return 0;
}

RAM_TEXT_SECTION
static int usb_audio_driver_iso_sync_in_xfer_done(T_HAL_USB_ISO_REQUEST_BLOCK *iso_urb,
                                                  uint8_t buf_num)
{
    uint8_t pkt_cnt = iso_urb->pkt_cnt;
    T_HAL_USB_ISO_PKT_INFO *iso_pkt = (buf_num == 0) ? iso_urb->iso_pkt0 : iso_urb->iso_pkt1;
    T_USB_AUDIO_INST *inst = (T_USB_AUDIO_INST *)iso_urb->priv;
    uint8_t *buf = (buf_num == 0) ? iso_urb->buf0 : iso_urb->buf1;
    uint32_t len = 0;
    for (uint8_t i = 0; i < pkt_cnt; i++)
    {
        len +=  iso_pkt[i].actual;
    }
    if (inst && inst->cbs.feedback_d)
    {
        inst->cbs.feedback_d(buf, len);
    }
    return 0;
}

RAM_TEXT_SECTION
static int usb_audio_driver_iso_out_xfer_done(T_HAL_USB_ISO_REQUEST_BLOCK *iso_urb, uint8_t buf_num)
{
    uint8_t pkt_cnt = iso_urb->pkt_cnt;
    T_HAL_USB_ISO_PKT_INFO *iso_pkt = (buf_num == 0) ? iso_urb->iso_pkt0 : iso_urb->iso_pkt1;
    T_USB_AUDIO_INST *inst = (T_USB_AUDIO_INST *)iso_urb->priv;
    uint8_t *buf = (buf_num == 0) ? iso_urb->buf0 : iso_urb->buf1;
    uint32_t len = 0;
    for (uint8_t i = 0; i < pkt_cnt; i++)
    {
//        USB_PRINT_INFO3("usb_audio_driver_iso_out_xfer_done, status:%d, offset:0x%x, actual:0x%x",
//                        iso_pkt[i].status, iso_pkt[i].offset, iso_pkt[i].actual);
        memcpy(inst->copy_buf + len, buf + iso_pkt[i].offset, iso_pkt[i].actual);
        len +=  iso_pkt[i].actual;
    }

    if (inst && inst->cbs.downstream)
    {
        inst->cbs.downstream(inst->copy_buf, len);
    }
    return 0;
}

static T_ALT_SETTING *usb_audio_driver_proc_if_desc(T_USB_AUDIO_IF *audio, uint8_t sub_class,
                                                    T_USB_INTERFACE_DESC *desc)
{
    T_ALT_SETTING *cur_alt = NULL;
    // int fail_line = 0;

    if (sub_class == UAC_SUBCLASS_AUDIOCONTROL)
    {
        T_AC_TOPOLOGY *ac_topology = audio->priv.ac_topology;
        if (ac_topology == NULL)
        {
            ac_topology = malloc(sizeof(T_AC_TOPOLOGY));
            memset(ac_topology, 0, sizeof(T_AC_TOPOLOGY));
            USB_UTILS_LIST_INIT(&ac_topology->ctrl_entity_list);
            audio->priv.ac_topology = ac_topology;
        }
    }
    else if (sub_class == UAC_SUBCLASS_AUDIOSTREAMING)
    {

        T_ALT_SETTINGS *alt_settings = audio->priv.alt_settings;
        if (alt_settings == NULL)
        {
            alt_settings = malloc(sizeof(T_ALT_SETTINGS));
            memset(alt_settings, 0, sizeof(T_ALT_SETTINGS));
            USB_UTILS_LIST_INIT(&alt_settings->list);
            audio->priv.alt_settings = alt_settings;
            alt_settings->owner = audio;
        }

        T_ALT_SETTING *alt_setting = NULL;
        uint8_t alt = desc->bAlternateSetting;

        USB_UTILS_LIST_FOREACH((&alt_settings->list), T_ALT_SETTING *, alt_setting)
        {
            if (alt_setting->value == alt)
            {
                cur_alt = alt_setting;
                break;
            }
        }
        if (cur_alt == NULL)
        {
            alt_setting = malloc(sizeof(T_ALT_SETTING));
            if (alt_setting == NULL)
            {
                fail_line = __LINE__;
                goto end;
            }
            memset(alt_setting, 0, sizeof(T_ALT_SETTING));
            alt_setting->value = alt;
            cur_alt = alt_setting;
            USB_UTILS_LIST_INSERT_TAIL(&alt_settings->list, alt_setting);
        }
    }

end:
    USB_PRINT_INFO2("usb_audio_driver_proc_if_desc, cur_alt 0x%x, fail line %d", cur_alt, fail_line);
    return cur_alt;
}

static int usb_audio_driver_proc_ep_desc(T_USB_AUDIO_IF *audio, T_ALT_SETTING *alt_setting,
                                         T_USB_ENDPOINT_DESC *desc, uint8_t speed)
{
    uint8_t ep_addr = desc->bEndpointAddress;
    T_USB_EP *usb_ep = NULL;
    T_EP_PRIV *ep_priv = NULL;
    uint8_t sub_class = audio->sub_class;
    bool found = false;
    int ret = ESUCCESS;

    if ((desc->bmAttributes & USB_EP_USAGE_MSK) == USB_EP_USAGE_DATA)
    {
        USB_UTILS_LIST_FOREACH(&audio->interface.eps, T_USB_EP *, usb_ep)
        {
            if (usb_ep->addr == ep_addr)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            usb_ep = malloc(sizeof(T_USB_EP));
            if (usb_ep == NULL)
            {
                fail_line = __LINE__;
                ret = -ENOMEM;
                goto end;
            }
            memset(usb_ep, 0, sizeof(T_USB_EP));
            usb_ep->addr = ep_addr;
            usb_ep->ep_handle = hal_usb_ep_handle_get(usb_ep->addr);
            USB_UTILS_LIST_INSERT_TAIL(&audio->interface.eps, usb_ep);
        }

        if (sub_class == UAC_SUBCLASS_AUDIOSTREAMING)
        {
            if (alt_setting == NULL)
            {
                ret = -ENXIO;
                goto end;
            }
            T_ALT_SETTINGS *alt_settings = audio->priv.alt_settings;
            if (UE_GET_DIR(ep_addr) == USB_DIR_IN)
            {
                audio->dir = USB_AUDIO_DIR_IN;
            }
            else
            {
                audio->dir = USB_AUDIO_DIR_OUT;
            }
            if (speed == HAL_USB_SPEED_FULL)
            {
                alt_setting->fs_ep_desc = desc;
            }
            else
            {
                alt_setting->hs_ep_desc = desc;
            }
            alt_settings->ep = usb_ep;
        }
        else
        {
            T_AC_TOPOLOGY *ac_topology = audio->priv.ac_topology;
            if (speed == HAL_USB_SPEED_FULL)
            {
                ac_topology->fs_ep_desc = desc;
            }
            else
            {
                ac_topology->hs_ep_desc = desc;
            }
        }

    }
    else if ((desc->bmAttributes & USB_EP_USAGE_MSK) == USB_EP_USAGE_FEEDBACK)
    {
        uint8_t data_ep_addr = (ep_addr & (~USB_DIR_MASK)) | ((ep_addr & USB_DIR_MASK) ^ USB_DIR_MASK);
        USB_UTILS_LIST_FOREACH(&audio->interface.eps, T_USB_EP *, usb_ep)
        {
            if (usb_ep->addr == data_ep_addr)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            fail_line = __LINE__;
            ret = -ENXIO;
            goto end;
        }

        if (usb_ep->priv == NULL)
        {
            ep_priv = malloc(sizeof(T_EP_PRIV));
            if (ep_priv == NULL)
            {
                fail_line = __LINE__;
                ret = -ENOMEM;
                goto end;
            }
            usb_ep->priv = ep_priv;
        }
        else
        {
            ep_priv = usb_ep->priv;
        }

        ep_priv->fb_ep_handle = hal_usb_ep_handle_get(ep_addr);
        if (speed == HAL_USB_SPEED_FULL)
        {
            ep_priv->fs_feedback_ep_desc = desc;
        }
        else
        {
            ep_priv->hs_feedback_ep_desc = desc;
        }

        T_HAL_USB_ISO_REQUEST_BLOCK *fb_iso_urb = hal_usb_iso_urb_alloc(FEEDBACK_PKT_SIZE_4B);
        if (fb_iso_urb  == NULL)
        {
            fail_line = __LINE__;
            ret = -ENOMEM;
            goto end;
        }
        fb_iso_urb->data_per_frame = FEEDBACK_PKT_SIZE_4B;
        fb_iso_urb->pkt_cnt = 1;
        fb_iso_urb->complete = usb_audio_driver_iso_sync_in_xfer_done;
        fb_iso_urb->iso_pkt0 = malloc(sizeof(T_HAL_USB_ISO_PKT_INFO) * fb_iso_urb->pkt_cnt);
        fb_iso_urb->iso_pkt1 = malloc(sizeof(T_HAL_USB_ISO_PKT_INFO) * fb_iso_urb->pkt_cnt);
        ep_priv->fb_iso_urb = fb_iso_urb;

    }
end:
    USB_PRINT_INFO2("usb_audio_driver_proc_ep_desc, ret %d, fail line %d", ret, fail_line);
    return ret;
}

static int usb_audio_driver_parse_descs(T_USB_AUDIO_IF *audio_if, T_USB_DESC_HDR **if_descs,
                                        uint8_t speed)
{
    T_USB_DESC_HDR **descs = if_descs;
    T_USB_INTERFACE *interface = NULL;
    T_ALT_SETTING *cur_alt = NULL;
    uint8_t sub_class = UAC_SUBCLASS_UNDEFINED;
    // int fail_line = 0;
    int ret = ESUCCESS;

    if (if_descs == NULL || audio_if == NULL)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
        goto end;
    }

    interface = &audio_if->interface;

    for (; *descs; descs++)
    {
        T_USB_DESC_HDR *cur_desc = *descs;
        uint8_t desc_type = cur_desc->bDescriptorType;
        if (desc_type == USB_DESC_TYPE_INTERFACE)
        {
            sub_class = ((T_USB_INTERFACE_DESC *)cur_desc)->bInterfaceSubClass;
            ((T_USB_INTERFACE_DESC *)cur_desc)->bInterfaceNumber = interface->if_num;
            audio_if->sub_class = sub_class;
            cur_alt = usb_audio_driver_proc_if_desc(audio_if, sub_class, (T_USB_INTERFACE_DESC *)cur_desc);
            if (cur_alt == NULL)
            {
                if (sub_class == UAC_SUBCLASS_AUDIOSTREAMING)
                {
                    fail_line = __LINE__;
                    ret = -ENXIO;
                    goto end;
                }
            }
        }
        else if (desc_type == USB_DESC_TYPE_ENDPOINT)
        {
            ret = usb_audio_driver_proc_ep_desc(audio_if, cur_alt, (T_USB_ENDPOINT_DESC *)cur_desc, speed);
        }
    }

end:
    USB_PRINT_INFO2("usb_audio_driver_parse_if_descs, result %d, fail line %d", ret, fail_line);
    return ret;
}


static uint32_t usb_audio_driver_calc_iso_buffer_interval(T_USB_ENDPOINT_DESC *ep_desc,
                                                          uint8_t unit_cnt)
{

    uint32_t proc_interval = 1;//  fs must 1
    uint8_t enumed_speed = usb_composite_dev_enum_speed_get();

    if (enumed_speed == HAL_USB_SPEED_FULL)
    {
        proc_interval = 1 * unit_cnt;
    }
    else if (enumed_speed == HAL_USB_SPEED_HIGH)
    {
        proc_interval = (1 << ((ep_desc->bInterval) - 1)) * unit_cnt;
    }
    else
    {
        proc_interval = 1;
    }

    return proc_interval;
}

static uint32_t usb_audio_driver_calc_data_per_frame(T_USB_ENDPOINT_DESC *ep_desc, uint8_t format,
                                                     uint8_t ch_cnt,
                                                     uint32_t sample_rate)
{
    uint32_t sample_num = 0;
    uint32_t sample_num_r = 0;
    uint32_t sample_num_q = 0;
    uint8_t actual_ch_cnt = ch_cnt;
    uint8_t byte_per_sample = format / 8;
    uint32_t frm_size = 0;
    uint8_t enumed_speed = usb_composite_dev_enum_speed_get();

    if (enumed_speed == HAL_USB_SPEED_HIGH)
    {
        sample_num_r = sample_rate * (1 << ((ep_desc->bInterval) - 1)) % (8 * 1000);
        sample_num_q = sample_rate * (1 << ((ep_desc->bInterval) - 1)) / (8 * 1000);
        if (sample_num_r)
        {
            sample_num = sample_num_q + 1;
        }
        else
        {
            sample_num = sample_num_q;
        }
        frm_size = sample_num * byte_per_sample * actual_ch_cnt;
    }
    else
    {
        sample_num_r = sample_rate % 1000;
        sample_num_q = sample_rate / 1000;
        if (sample_num_r)
        {
            sample_num = sample_num_q + 1;
        }
        else
        {
            sample_num = sample_num_q;
        }
        frm_size = sample_num * byte_per_sample * actual_ch_cnt;
    }
    USB_PRINT_INFO4("usb_audio_driver_calc_data_per_frame, sample_num %d, byte_per_sample %d, actual_ch_cnt %d, frm_size %d",
                    sample_num, byte_per_sample, actual_ch_cnt, frm_size);
    return frm_size;
}

static int usb_audio_driver_streaming_activate(T_USB_AUDIO_IF *audio, T_USB_EP *ep,
                                               T_ALT_SETTING *cur_alt)
{
    T_USB_AUDIO_INST *inst = (T_USB_AUDIO_INST *)audio->owner;
    T_USB_AUDIO_DRIVER_ATTR *attr = &cur_alt->attr;
    T_USB_ENDPOINT_DESC *ep_desc = NULL;
    T_USB_ENDPOINT_DESC *fb_ep_desc = NULL;
    T_HAL_USB_ISO_REQUEST_BLOCK *iso_urb = NULL;
    T_EP_PRIV *ep_priv = ep->priv;
    T_HAL_USB_SPEED speed = usb_composite_dev_enum_speed_get();
    uint8_t proc_interval = 0;
    bool fb_en = false;
    uint8_t uac_ver = inst->uac_ver;
    uint32_t sample_rate_max_tolerance = 0;
    uint8_t fb_pkt_size = FEEDBACK_PKT_SIZE_4B;
    int32_t ret = ESUCCESS;

    if (speed == HAL_USB_SPEED_HIGH)
    {
        ep_desc = (void *)cur_alt->hs_ep_desc;
        if (ep_priv)
        {
            fb_ep_desc = ep_priv->hs_feedback_ep_desc;
        }
    }
    else
    {
        ep_desc = (void *)cur_alt->fs_ep_desc;
        if (ep_priv)
        {
            fb_ep_desc = ep_priv->fs_feedback_ep_desc;
        }
    }

    ep->desc = ep_desc;
    ret = hal_usb_ep_enable(ep->ep_handle, ep_desc);


    if (fb_ep_desc)
    {
        ret += hal_usb_ep_enable(ep_priv->fb_ep_handle, fb_ep_desc);
        if (ret == 0)
        {
            uint32_t fb_val_default = 0;
            if (uac_ver == USB_AUDIO_VERSION_1 || speed == HAL_USB_SPEED_FULL)
            {
                fb_val_default = TO_FB_FORMAT_10_14(attr->cur_sample_rate);
                fb_pkt_size = FEEDBACK_PKT_SIZE_3B;
            }
            else
            {
                fb_val_default = TO_FB_FORMAT_16_16(attr->cur_sample_rate);
                fb_pkt_size = FEEDBACK_PKT_SIZE_4B;
            }
            iso_urb = ep_priv->fb_iso_urb;
            iso_urb->data_per_frame = fb_pkt_size;
            memcpy(iso_urb->buf0, &fb_val_default, iso_urb->data_per_frame);
            iso_urb->buf_proc_intrvl = usb_audio_driver_calc_iso_buffer_interval(fb_ep_desc, iso_urb->pkt_cnt);
            iso_urb->priv = inst;
            ret = hal_usb_iso_ep_start(ep_priv->fb_ep_handle, iso_urb);
        }
        fb_en = true;
        USB_PRINT_INFO4("usb_audio_driver_streaming_activate, ep_priv 0x%x, ep_priv->hs_feedback_ep_desc 0x%x, ep_priv->fs_feedback_ep_desc 0x%x, fb_ep_desc 0x%x",
                        ep_priv, ep_priv->hs_feedback_ep_desc, ep_priv->fs_feedback_ep_desc, fb_ep_desc);
    }

    if (ret == 0)
    {
        if (audio->dir == USB_AUDIO_DIR_IN)
        {
            iso_urb = inst->iso_in_urb;
            proc_interval = inst->iso_in_proc_interval;
            iso_urb->complete = usb_audio_driver_iso_in_xfer_done;
            memset(iso_urb->buf0, 0, inst->iso_in_max_buf_size);
            memset(iso_urb->buf1, 0, inst->iso_in_max_buf_size);
        }
        else
        {
            iso_urb = inst->iso_out_urb;
            proc_interval = inst->iso_out_proc_interval;
            iso_urb->complete = usb_audio_driver_iso_out_xfer_done;
        }
        iso_urb->buf_proc_intrvl = usb_audio_driver_calc_iso_buffer_interval(ep_desc, proc_interval);
        if (fb_en)
        {
            sample_rate_max_tolerance = attr->cur_sample_rate * (100 + PERCENT_SAMPLE_RATE_TOLERANCE) / 100;
        }
        else
        {
            sample_rate_max_tolerance = attr->cur_sample_rate;
        }
        iso_urb->data_per_frame = usb_audio_driver_calc_data_per_frame(ep_desc, attr->bit_width,
                                                                       attr->chann_num,
                                                                       sample_rate_max_tolerance);

        if (inst->cbs.activate)
        {
            inst->cbs.activate(attr->dir, attr->bit_width, attr->cur_sample_rate, attr->chann_num);
        }
        ret = hal_usb_iso_ep_start(ep->ep_handle, iso_urb);
        USB_PRINT_INFO5("usb_audio_driver_streaming_activate, dir %d, bit_width %d, cur_sample_rate %d, chann_num %d, sample_rate_max_tolerance %d",
                        attr->dir, attr->bit_width,
                        attr->cur_sample_rate, attr->chann_num, sample_rate_max_tolerance);
    }

    return ret;
}

static int usb_audio_driver_streaming_deactivate(T_USB_AUDIO_IF *audio, T_USB_EP *ep,
                                                 T_ALT_SETTING *cur_alt)
{
    T_USB_AUDIO_INST *inst = (T_USB_AUDIO_INST *)audio->owner;
    T_USB_AUDIO_DRIVER_ATTR *attr = &cur_alt->attr;
    T_HAL_USB_ISO_REQUEST_BLOCK *iso_urb = NULL;
    T_EP_PRIV *ep_priv = ep->priv;
    T_HAL_USB_SPEED speed = usb_composite_dev_enum_speed_get();
    int32_t ret = ESUCCESS;

    if (audio->dir == USB_AUDIO_DIR_IN)
    {
        iso_urb = inst->iso_in_urb;
    }
    else
    {
        iso_urb = inst->iso_out_urb;
    }
    USB_PRINT_INFO1("usb_audio_driver_streaming_deactivate %d", attr->dir);
    ret = hal_usb_iso_ep_stop(ep->ep_handle,  iso_urb);
    if (ret == 0)
    {
        ret = hal_usb_ep_disable(ep->ep_handle);
    }

    if (ep_priv)
    {
        ret += hal_usb_iso_ep_stop(ep_priv->fb_ep_handle, ep_priv->fb_iso_urb);
        if (ret == 0)
        {
            ret = hal_usb_ep_disable(ep_priv->fb_ep_handle);
        }
    }

    if (ret == ESUCCESS)
    {
        if (inst->cbs.deactivate)
        {
            ret = inst->cbs.deactivate(attr->dir);
        }
    }

    return ret;
}

static int usb_audio_driver_if_num_change(uint8_t actual_if_num, T_USB_AUDIO_IF *audio_if,
                                          uint8_t speed)
{
    T_USB_DESC_HDR **descs = (speed == HAL_USB_SPEED_FULL) ? audio_if->interface.descs_fs :
                             audio_if->interface.descs_hs;
    T_USB_AUDIO_INST *inst = audio_if->owner;
    uint8_t uac_ver = inst->uac_ver;
    int ret = ESUCCESS;

    if (descs == NULL)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
        goto end;
    }

    for (; *descs; descs++)
    {
        T_USB_DESC_HDR *cur_desc = *descs;
        uint8_t desc_type = cur_desc->bDescriptorType;
        if (desc_type == USB_DESC_TYPE_INTERFACE)
        {
            ((T_USB_INTERFACE_DESC *)cur_desc)->bInterfaceNumber = actual_if_num;

        }
        else if (desc_type == UAC_DT_CS_INTERFACE)
        {
            if (USB_AUDIO_VERSION_1 == uac_ver)
            {
                uint8_t subtype = ((T_UAC_CS_DESC_HDR *)cur_desc)->bDescriptorSubtype;
                if (subtype == UAC1_HEADER && audio_if->sub_class == UAC1_SUBCLASS_AUDIOCONTROL)
                {
                    uint8_t offset_ifnr = sizeof(T_UAC1_AC_HDR_DESC(0, 0));
                    uint8_t *addr_if_nr = (uint8_t *)cur_desc + offset_ifnr;
                    uint8_t as_if_num = cur_desc->bLength - offset_ifnr;
                    for (uint8_t i = 0; i < as_if_num; i++)
                    {
                        *(addr_if_nr + i) = actual_if_num + 1 + i;
                    }
                }
            }
        }
    }

end:
    USB_PRINT_INFO2("usb_audio_driver_change_if_num, result %d, fail line %d", ret, fail_line);
    return ret;
}

static int usb_audio_driver_if_create(T_USB_INTERFACE *interface)
{
    T_USB_AUDIO_IF *audio_if = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    T_USB_AUDIO_INST *inst = (T_USB_AUDIO_INST *)audio_if->owner;
    uint8_t actual_if_num = interface->if_num;
    int ret = ESUCCESS;

    ret = usb_audio_driver_if_num_change(actual_if_num, audio_if, HAL_USB_SPEED_FULL);
    ret += usb_audio_driver_if_num_change(actual_if_num, audio_if, HAL_USB_SPEED_HIGH);

    if (audio_if->sub_class == UAC_SUBCLASS_AUDIOSTREAMING)
    {
        if (audio_if->dir == USB_AUDIO_DIR_OUT)
        {
            inst->iso_out_urb = hal_usb_iso_urb_alloc(inst->iso_out_max_buf_size);
            if (inst->iso_out_urb)
            {
                inst->iso_out_urb->pkt_cnt = inst->iso_out_proc_interval;
                inst->iso_out_urb->iso_pkt0 = malloc(sizeof(T_HAL_USB_ISO_PKT_INFO) * inst->iso_out_proc_interval);
                memset(inst->iso_out_urb->iso_pkt0, 0,
                       sizeof(T_HAL_USB_ISO_PKT_INFO) * inst->iso_out_proc_interval);
                inst->iso_out_urb->iso_pkt1 = malloc(sizeof(T_HAL_USB_ISO_PKT_INFO) * inst->iso_out_proc_interval);
                memset(inst->iso_out_urb->iso_pkt1, 0,
                       sizeof(T_HAL_USB_ISO_PKT_INFO) * inst->iso_out_proc_interval);
                inst->iso_out_urb->priv = (void *)inst;
            }
            inst->copy_buf = malloc(inst->iso_out_max_buf_size);
            memset(inst->copy_buf, 0, inst->iso_out_max_buf_size);
        }
        else
        {
            inst->iso_in_urb = hal_usb_iso_urb_alloc(inst->iso_in_max_buf_size);
            if (inst->iso_in_urb)
            {
                inst->iso_in_urb->pkt_cnt = inst->iso_in_proc_interval;
                inst->iso_in_urb->iso_pkt0 = malloc(sizeof(T_HAL_USB_ISO_PKT_INFO) * inst->iso_in_proc_interval);
                memset(inst->iso_in_urb->iso_pkt0, 0, sizeof(T_HAL_USB_ISO_PKT_INFO) * inst->iso_in_proc_interval);
                inst->iso_in_urb->iso_pkt1 = malloc(sizeof(T_HAL_USB_ISO_PKT_INFO) * inst->iso_in_proc_interval);
                memset(inst->iso_in_urb->iso_pkt1, 0, sizeof(T_HAL_USB_ISO_PKT_INFO) * inst->iso_in_proc_interval);
                inst->iso_in_urb->priv = (void *)inst;
            }
        }
    }

    return ret;
}

static int usb_audio_driver_if_release(T_USB_INTERFACE *interface)
{
    int ret = ESUCCESS;
    T_USB_AUDIO_IF *audio_if = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    T_USB_AUDIO_INST *inst = (T_USB_AUDIO_INST *)audio_if->owner;

    if (audio_if->sub_class == UAC_SUBCLASS_AUDIOSTREAMING)
    {
        if (audio_if->dir == USB_AUDIO_DIR_OUT)
        {
            if (inst->iso_out_urb)
            {
                free(inst->iso_out_urb->iso_pkt0);
                free(inst->iso_out_urb->iso_pkt1);
                hal_usb_iso_urb_free(inst->iso_out_urb);
                inst->iso_out_urb = NULL;
            }
            if (inst->copy_buf)
            {
                free(inst->copy_buf);
                inst->copy_buf = NULL;
            }
        }
        else
        {
            if (inst->iso_in_urb)
            {
                free(inst->iso_in_urb->iso_pkt0);
                free(inst->iso_in_urb->iso_pkt1);
                hal_usb_iso_urb_free(inst->iso_in_urb);
                inst->iso_in_urb = NULL;
            }
        }
    }

    return ret;
}

static int usb_audio_driver_if_request_done(T_HAL_USB_REQUEST_BLOCK *urb)
{
    T_HAL_USB_REQUEST_BLOCK *ctrl_urb = (T_HAL_USB_REQUEST_BLOCK *)urb;
    int ret = ESUCCESS;
    USB_PRINT_INFO2("usb_audio_driver_if_request_done, len %d, priv 0x%x", ctrl_urb->actual,
                    ctrl_urb->priv);
    if (ctrl_urb->priv)
    {
        T_SET_CMD *set_cmd = (T_SET_CMD *)ctrl_urb->priv;

        if (set_cmd->uint == SET_CMD_UINT_IF)
        {
            T_USB_AUDIO_DRIVER_CTRL *ctrl = (T_USB_AUDIO_DRIVER_CTRL *)set_cmd->priv;
            if (ctrl && ctrl->set)
            {
                int value = 0;
                memcpy(&value, urb->buf, USB_UTILS_MIN(4, urb->actual));
                ret = ctrl->set(&ctrl->attr, set_cmd->cmd, value);
            }
        }
        free(set_cmd);
        ctrl_urb->priv = NULL;
    }

    return ret;
}

static int usb_audio_driver_ep_request_done(T_HAL_USB_REQUEST_BLOCK *urb)
{
    T_HAL_USB_REQUEST_BLOCK *ctrl_urb = (T_HAL_USB_REQUEST_BLOCK *)urb;
    int ret = ESUCCESS;
    USB_PRINT_INFO2("usb_audio_driver_ep_request_done, actual %d, priv 0x%x", urb->actual,
                    ctrl_urb->priv);
    if (ctrl_urb->priv)
    {
        T_SET_CMD *set_cmd = (T_SET_CMD *)ctrl_urb->priv;

        uint8_t uac_ver = set_cmd->uac_ver;
        if (set_cmd->uint == SET_CMD_UINT_EP)
        {
            if (uac_ver == USB_AUDIO_VERSION_1)
            {
                uint32_t cur_sample_rate = (*((uint32_t *)urb->buf)) & 0x00FFFFFF;
                uint32_t pre_sample_rate = 0;
                T_ALT_SETTINGS *alt_settings = (T_ALT_SETTINGS *)set_cmd->priv;
                if (alt_settings)
                {
                    USB_PRINT_INFO3("usb_audio_driver_ep_request_done, alt_settings 0x%x, cur 0x%x, ep 0x%x",
                                    alt_settings, alt_settings->cur, alt_settings->ep);
                    if (alt_settings->cur && alt_settings->cur->value)
                    {
                        T_USB_AUDIO_IF *audio = (T_USB_AUDIO_IF *)alt_settings->owner;
                        pre_sample_rate = alt_settings->cur->attr.cur_sample_rate;
                        if (pre_sample_rate != cur_sample_rate)
                        {
                            usb_audio_driver_streaming_deactivate(audio, alt_settings->ep, alt_settings->cur);
                            alt_settings->cur->attr.cur_sample_rate = cur_sample_rate;
                            usb_audio_driver_streaming_activate(audio, alt_settings->ep, alt_settings->cur);
                        }
                    }
                }
            }
        }
        free(set_cmd);

        ctrl_urb->priv = NULL;
    }

    return ret;
}

static int usb_audio_driver_get_if_request_proc(T_USB_INTERFACE *interface,
                                                T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint8_t ctrl_type = (ctrl_request->wValue >> 8) & 0xFF;
    uint8_t id = (((ctrl_request->wIndex) >> 8) & 0xFF);
    uint16_t len = (ctrl_request->wLength);
    uint8_t cmd = (ctrl_request->bRequest & 0x0F);

    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    T_USB_UTILS_LIST *entity_list = NULL;
    T_CONTROL_ENTITY_ITEM *ctrl_entity_item = NULL;
    T_USB_AUDIO_DRIVER_CTRL **ctrl = NULL;
    T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR *ctrl_entity = NULL;
    T_USB_AUDIO_DRIVER_CTRL_ATTR *attr = NULL;
    uint8_t ctrl_num = 0;
    bool found = false;

    if (audio->sub_class != UAC_SUBCLASS_AUDIOCONTROL)
    {
        return -ENOTSUPP;
    }

    entity_list = &audio->priv.ac_topology->ctrl_entity_list;
    USB_UTILS_LIST_FOREACH(entity_list, T_CONTROL_ENTITY_ITEM *, ctrl_entity_item)
    {

        ctrl_entity = ctrl_entity_item->ctrl_entity;
        ctrl_num = ctrl_entity->ctrl_num;

        if (ctrl_entity->entity_id == id)
        {
            ctrl = ((T_USB_AUDIO_DRIVER_CTRL_ENTITY(0, 1) *)ctrl_entity)->ctrls;
            while (*ctrl)
            {

                if ((*ctrl)->type == ctrl_type)
                {
                    if ((*ctrl)->get)
                    {
                        attr = (*ctrl)->get(&(*ctrl)->attr, cmd);
                    }
                    found = true;
                    break;
                }
                ctrl++;
                ctrl_num--;
            }

        }
        if (found)
        {
            break;
        }
    }

    if (attr != NULL)
    {
        ctrl_urb->priv = NULL;
        ctrl_urb->complete = NULL;
        len = USB_UTILS_MIN(attr->len, len);
        memcpy(ctrl_urb->buf, attr->data, len);
    }
    else
    {
        return -ENOTSUPP;
    }

    return len;
}

static int usb_audio_driver_set_if_request_proc(T_USB_INTERFACE *interface,
                                                T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint8_t ctrl_type = (ctrl_request->wValue >> 8) & 0xFF;
    uint8_t id = (((ctrl_request->wIndex) >> 8) & 0xFF);
    uint8_t cmd = (ctrl_request->bRequest & 0x0F);
    uint16_t len = (ctrl_request->wLength);

    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    T_USB_UTILS_LIST *ctrl_entity_list = NULL;
    T_CONTROL_ENTITY_ITEM *ctrl_entity_item = NULL;
    T_USB_AUDIO_DRIVER_CTRL **ctrl = NULL;
    T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR *ctrl_entity = NULL;
    uint8_t ctrl_num = 0;
    bool found = false;
    //int ret = ESUCCESS;

    if (audio->sub_class != UAC_SUBCLASS_AUDIOCONTROL)
    {
        return -ENOTSUPP;
    }

    ctrl_entity_list =  &audio->priv.ac_topology->ctrl_entity_list;
    USB_UTILS_LIST_FOREACH(ctrl_entity_list, T_CONTROL_ENTITY_ITEM *, ctrl_entity_item)
    {

        ctrl_entity = ctrl_entity_item->ctrl_entity;
        ctrl_num = ctrl_entity->ctrl_num;

        if (ctrl_entity->entity_id == id)
        {
            ctrl = ((T_USB_AUDIO_DRIVER_CTRL_ENTITY(0, 1) *)ctrl_entity)->ctrls;
            while (ctrl_num)
            {

                if ((*ctrl)->type == ctrl_type)
                {
                    found = true;
                    break;
                }
                ctrl++;
                ctrl_num--;
            }
        }

        if (found)
        {
            break;
        }
    }

    if (found)
    {
        T_SET_CMD *set_cmd = malloc(sizeof(T_SET_CMD));
        if (set_cmd)
        {
            memset(set_cmd, 0, sizeof(T_SET_CMD));
            set_cmd->uint = SET_CMD_UINT_IF;
            set_cmd->cmd = cmd;
            set_cmd->priv = *ctrl;
        }
        else
        {
            return -ENOMEM;
        }
        ctrl_urb->priv = set_cmd;
        ctrl_urb->complete = usb_audio_driver_if_request_done;
    }
    else
    {
        return -ENOTSUPP;
    }

    return len;
}

static int usb_audio_driver_set_ep_request_proc(T_USB_INTERFACE *interface,
                                                T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t ep = ctrl_request->wIndex;
    uint16_t len = (ctrl_request->wLength);

    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    T_USB_AUDIO_INST *inst = NULL;
    uint8_t uac_ver = 0;
    T_ALT_SETTINGS *alt_settings = NULL;
    T_SET_CMD *set_cmd = NULL;
    int ret = len;

    alt_settings = audio->priv.alt_settings;
    inst = (T_USB_AUDIO_INST *)audio->owner;

    if (inst)
    {
        uac_ver = inst->uac_ver;
    }
    else
    {
        fail_line = __LINE__;
        ret = -ENXIO;
        goto end;
    }

    set_cmd = malloc(sizeof(T_SET_CMD));
    if (set_cmd)
    {
        memset(set_cmd, 0, sizeof(T_SET_CMD));
        set_cmd->uint = SET_CMD_UINT_EP;
        set_cmd->uac_ver = uac_ver;
        if (uac_ver == USB_AUDIO_VERSION_1)
        {
            set_cmd->priv = alt_settings;
        }
    }
    else
    {
        ret = -ENOMEM;
        goto end;
    }
    ctrl_urb->priv = set_cmd;
    ctrl_urb->complete = usb_audio_driver_ep_request_done;

end:
    USB_PRINT_INFO3("usb_audio_driver_set_ep_request_proc, ep 0x%x, result %d, fail line %d", ep, ret,
                    fail_line);
    return ret;
}

static int usb_audio_driver_get_ep_request_proc(T_USB_INTERFACE *interface,
                                                T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    int ret = ESUCCESS;
    uint8_t ep = (((ctrl_request->wIndex) >> 8) & 0xFF);
    uint16_t len = (ctrl_request->wLength);
    uint16_t w_value = (ctrl_request->wValue);

    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    T_USB_AUDIO_INST *inst = NULL;
    uint8_t uac_ver = 0;

    USB_PRINT_INFO4("usb_audio_driver_get_ep_request_proc, bRequest 0x%x, w_value 0x%04x, len %d, endpoint %d\n",
                    ctrl_request->bRequest, w_value, len, ep);

    inst = (T_USB_AUDIO_INST *)audio->owner;

    if (inst)
    {
        uac_ver = inst->uac_ver;
    }
    else
    {
        fail_line = __LINE__;
        ret = -ENXIO;
        goto end;
    }

    if (uac_ver == USB_AUDIO_VERSION_1)
    {
        switch (ctrl_request->bRequest)
        {
        case UAC1_GET_CUR:
        case UAC1_GET_MIN:
        case UAC1_GET_MAX:
        case UAC1_GET_RES:
            ret = len;
            break;
        case UAC1_GET_MEM:
            break;
        default:
            break;
        }
    }
end:
    USB_PRINT_INFO3("usb_audio_driver_get_ep_request_proc, ep 0x%x, result %d, fail line %d", ep, ret,
                    fail_line);
    return ret;
}

static int usb_audio_driver_ep_stall_set(T_USB_INTERFACE *interface,
                                         T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t w_value = (ctrl_request->wValue);
    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    uint8_t sub_class = audio->sub_class;
    T_AC_TOPOLOGY *ac_topology = NULL;
    int ret = 0;

    if (UGETW(w_value) == UF_ENDPOINT_HALT && sub_class == UAC_SUBCLASS_AUDIOCONTROL)
    {
        ac_topology = audio->priv.ac_topology;
        ret = usb_pipe_stall_set(ac_topology->pipe_tx, 0);
    }
    else
    {
        ret = -ENOTSUPP;
    }

    return ret;
}

static int usb_audio_driver_ep_stall_clear(T_USB_INTERFACE *interface,
                                           T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t w_value = (ctrl_request->wValue);
    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    uint8_t sub_class = audio->sub_class;
    T_AC_TOPOLOGY *ac_topology = NULL;
    int ret = 0;

    if (UGETW(w_value) == UF_ENDPOINT_HALT && sub_class == UAC_SUBCLASS_AUDIOCONTROL)
    {
        ac_topology = audio->priv.ac_topology;
        ret = usb_pipe_stall_clear(ac_topology->pipe_tx);
    }
    else
    {
        ret = -ENOTSUPP;
    }

    return ret;
}

static int usb_audio_driver_if_ctrl_request_proc(T_USB_INTERFACE *interface,
                                                 T_USB_DEVICE_REQUEST *ctrl_request,
                                                 T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    T_HAL_USB_REQUEST_BLOCK      *urb = ctrl_urb;
    uint16_t                    w_index = (ctrl_request->wIndex);
    uint16_t                    w_value = (ctrl_request->wValue);
    uint16_t                    w_length = (ctrl_request->wLength);
    uint8_t                     b_request = (ctrl_request->bRequest);
    uint8_t                     bm_request_type = (ctrl_request->bmRequestType);
    int ret;
    ret = w_length;

    USB_PRINT_INFO5("usb_audio_driver_if_ctrl_request_proc, bmRequestType %02x, bRequest %02x, w_value %04x, w_index %04x, w_length %d",
                    ctrl_request->bmRequestType, ctrl_request->bRequest, w_value, w_index, w_length);
    switch (bm_request_type)
    {
    case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
        ret = usb_audio_driver_set_if_request_proc(interface, ctrl_request, ctrl_urb);
        break;
    case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
        ret = usb_audio_driver_get_if_request_proc(interface, ctrl_request, ctrl_urb);
        break;
    case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
        ret = usb_audio_driver_set_ep_request_proc(interface, ctrl_request, ctrl_urb);
        break;
    case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
        ret = usb_audio_driver_get_ep_request_proc(interface, ctrl_request, ctrl_urb);
        break;
    case USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_ENDPOINT:
        if (b_request == USB_REQ_CODE_CLEAR_FEATURE)
        {
            ret = usb_audio_driver_ep_stall_clear(interface, ctrl_request, ctrl_urb);
        }
        else if (b_request == USB_REQ_CODE_SET_FEATURE)
        {
            ret = usb_audio_driver_ep_stall_set(interface, ctrl_request, ctrl_urb);
        }
        else
        {
            ret = -EOPNOTSUPP;
        }
        goto done;
    default:
        ret = -EOPNOTSUPP;
        break;
    }

    if (ret >= 0)
    {
        ret = USB_UTILS_MIN(ret, w_length);
        if (ret > 0 && ret < w_length)
        {
            urb->zlp = (ret % usb_composite_dev_ep0_mps_get() == 0);
        }
        urb->length = ret;
        ret = hal_usb_ep0_trx(urb->ep_handle, urb);
        if (ret < 0)
        {
            USB_PRINT_ERROR0("usb_audio_driver_if_ctrl_request_proc, hal_usb_ep0_trx error");
        }
    }
done:
    return ret;
}

static int usb_audio_driver_if_alt_set(T_USB_INTERFACE *interface,
                                       T_HAL_USB_REQUEST_BLOCK *ctrl_urb, uint8_t alt)
{
    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    uint8_t sub_class = audio->sub_class;
//    T_HAL_USB_ISO_REQUEST_BLOCK *iso_urb = NULL;
    uint8_t if_num = interface->if_num;
    T_USB_EP *ep = NULL;
    T_USB_ENDPOINT_DESC **ep_desc = NULL;
    T_USB_AUDIO_INST *inst = (T_USB_AUDIO_INST *)audio->owner;
    T_HAL_USB_SPEED speed = usb_composite_dev_enum_speed_get();
    int32_t ret = ESUCCESS;

    if (sub_class == UAC_SUBCLASS_AUDIOCONTROL)
    {
        T_AC_TOPOLOGY *ac_topology = audio->priv.ac_topology;
        if (speed == HAL_USB_SPEED_HIGH)
        {
            ep_desc = &(ac_topology->hs_ep_desc);
        }
        else
        {
            ep_desc = &(ac_topology->fs_ep_desc);
        }
        if (*ep_desc)
        {
            void *ep_handle = hal_usb_ep_handle_get((*ep_desc)->bEndpointAddress);
            hal_usb_ep_disable(ep_handle);
            hal_usb_ep_enable(ep_handle, *ep_desc);
            T_USB_PIPE_ATTR attr =
            {
                .zlp = 0,
                .high_throughput = 0,
                .congestion_ctrl = USB_PIPE_CONGESTION_CTRL_DROP_CUR,
                .rsv = 0,
                .mtu = (*ep_desc)->wMaxPacketSize,
            };
            ac_topology->pipe_tx = usb_pipe_open(ep_desc, (*ep_desc)->bEndpointAddress, attr, 1, NULL);
            USB_PRINT_INFO2("usb_pipe_open, ep_desc 0x%x, ep_addr 0x%x", *ep_desc,
                            (*ep_desc)->bEndpointAddress);
        }
    }
    else
    {
        T_ALT_SETTINGS *alt_settings = NULL;
        T_ALT_SETTING *alt_setting = NULL;
        alt_settings = (T_ALT_SETTINGS *)audio->priv.alt_settings;
        ep = alt_settings->ep;

        if (audio->cur_alt)
        {
            usb_audio_driver_streaming_deactivate(audio, ep, alt_settings->cur);
            alt_settings->cur = NULL;
        }

        USB_UTILS_LIST_FOREACH((&alt_settings->list), T_ALT_SETTING *, alt_setting)
        {
            if (alt_setting->value == alt)
            {
                break;
            }
        }

        if (alt_setting == NULL)
        {
            fail_line = __LINE__;
            ret = -ENXIO;
            goto end;
        }
        alt_settings->cur = alt_setting;

        if (inst->uac_ver == USB_AUDIO_VERSION_2)
        {
            if (alt_settings->cs.uac2.freq != 0)
            {
                alt_setting->attr.cur_sample_rate = (uint32_t)alt_settings->cs.uac2.freq;
            }
        }


        if (ep)
        {

            if (alt)
            {
                usb_audio_driver_streaming_activate(audio, ep, alt_setting);
            }
            else
            {

            }

        }

    }
    audio->cur_alt = alt;

end:
    USB_PRINT_INFO4("usb_audio_driver_if_alt_set, alt %d, interface %d, ret %d, fail line %d", alt,
                    if_num, ret, fail_line);

    return ret;
}

int usb_audio_driver_if_alt_get(T_USB_INTERFACE *interface, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    uint8_t alt = audio->cur_alt;

    memset(ctrl_urb->buf, alt, 1);

    return 1;
}

void *usb_audio_driver_inst_alloc(uint8_t version, uint8_t proc_interval_out,
                                  uint8_t proc_interval_in)
{
    T_USB_AUDIO_INST *audio_inst = malloc(sizeof(T_USB_AUDIO_INST));
    if (audio_inst)
    {
        memset(audio_inst, 0, sizeof(T_USB_AUDIO_INST));
        audio_inst->uac_ver = version;
        audio_inst->iso_in_proc_interval = proc_interval_in;
        audio_inst->iso_out_proc_interval = proc_interval_out;
        USB_UTILS_LIST_INIT(&audio_inst->if_list);
        if (USB_UTILS_LIST_EMPTY(&g_usb_audio.insts))
        {
            USB_UTILS_LIST_INIT(&g_usb_audio.insts);
        }
        USB_UTILS_LIST_INSERT_TAIL(&g_usb_audio.insts, audio_inst);
    }

    return audio_inst;
}

int usb_audio_driver_inst_free(void *inst)
{
    T_USB_AUDIO_INST *audio_inst = NULL;
    bool found = false;
    int ret = ESUCCESS;

    USB_UTILS_LIST_FOREACH(&g_usb_audio.insts, T_USB_AUDIO_INST *, audio_inst)
    {
        if (audio_inst == inst)
        {
            found = true;
            USB_UTILS_LIST_REMOVE(&g_usb_audio.insts, audio_inst);
            free(audio_inst);
        }
    }

    if (!found)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
    }

    USB_PRINT_INFO2("usb_audio_driver_inst_free, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_audio_driver_if_suspend(struct _usb_interface *interface)
{
    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    uint8_t sub_class = audio->sub_class;
    T_ALT_SETTINGS *alt_settings = NULL;
    T_ALT_SETTING *alt_setting = NULL;
    T_AC_TOPOLOGY *ac_topology = NULL;
    T_USB_ENDPOINT_DESC *ep_desc = NULL;
    T_HAL_USB_SPEED speed = usb_composite_dev_enum_speed_get();
    T_USB_EP *ep = NULL;
    int32_t ret = ESUCCESS;

    if (sub_class == UAC_SUBCLASS_AUDIOCONTROL)
    {
        ac_topology = audio->priv.ac_topology;
        if (speed == HAL_USB_SPEED_HIGH)
        {
            ep_desc = ac_topology->hs_ep_desc;
        }
        else
        {
            ep_desc = ac_topology->fs_ep_desc;
        }
        if (ep_desc)
        {
            hal_usb_ep_disable(hal_usb_ep_handle_get(ep_desc->bEndpointAddress));
        }
    }
    else
    {
        if (audio->cur_alt)
        {
            alt_settings = (T_ALT_SETTINGS *)audio->priv.alt_settings;
            if (alt_settings)
            {
                alt_setting = alt_settings->cur;
                ep = alt_settings->ep;
                usb_audio_driver_streaming_deactivate(audio, ep, alt_setting);
            }
            audio->cur_alt = 0;
        }
    }

    return ret;

}

int usb_audio_driver_if_resume(struct _usb_interface *interface)
{
    T_USB_AUDIO_IF *audio = USB_UTILS_CONTAINER_OF(interface, T_USB_AUDIO_IF, interface);
    uint8_t sub_class = audio->sub_class;
    T_AC_TOPOLOGY *ac_topology = NULL;
    T_USB_ENDPOINT_DESC *ep_desc = NULL;
    T_HAL_USB_SPEED speed = usb_composite_dev_enum_speed_get();
    int32_t ret = ESUCCESS;

    if (sub_class == UAC_SUBCLASS_AUDIOCONTROL)
    {
        ac_topology = audio->priv.ac_topology;
        if (speed == HAL_USB_SPEED_HIGH)
        {
            ep_desc = ac_topology->hs_ep_desc;
        }
        else
        {
            ep_desc = ac_topology->fs_ep_desc;
        }
        if (ep_desc)
        {
            hal_usb_ep_enable(hal_usb_ep_handle_get(ep_desc->bEndpointAddress), ep_desc);
        }
    }

    return ret;
}

int usb_audio_driver_desc_register(void *inst, T_USB_AUDIO_DRIVER_DESC_HDR **descs_fs,
                                   T_USB_AUDIO_DRIVER_DESC_HDR **descs_hs)
{
    T_USB_AUDIO_IF *audio_if = malloc(sizeof(T_USB_AUDIO_IF));
//    uint8_t actual_if_num = 0;
    int ret = ESUCCESS;

    if (audio_if == NULL)
    {
        fail_line = __LINE__;
        ret = -ENOMEM;
        goto end;
    }
    memset(audio_if, 0, sizeof(T_USB_AUDIO_IF));
    audio_if->interface.descs_fs = (struct usb_descriptor_header **)descs_fs;
    audio_if->interface.descs_hs = (struct usb_descriptor_header **)descs_hs;
    audio_if->interface.create = usb_audio_driver_if_create;
    audio_if->interface.release = usb_audio_driver_if_release;
    audio_if->interface.ctrl_request_proc = usb_audio_driver_if_ctrl_request_proc;
    audio_if->interface.alt_get = usb_audio_driver_if_alt_get;
    audio_if->interface.alt_set = usb_audio_driver_if_alt_set;
    audio_if->interface.suspend = usb_audio_driver_if_suspend;
    audio_if->interface.resume = usb_audio_driver_if_resume;
    audio_if->owner = inst;
    USB_UTILS_LIST_INIT(&audio_if->interface.eps);
    USB_UTILS_LIST_INSERT_TAIL(&((T_USB_AUDIO_INST *)inst)->if_list, audio_if);
    ret = usb_audio_driver_parse_descs(audio_if, audio_if->interface.descs_fs, HAL_USB_SPEED_FULL);
    ret += usb_audio_driver_parse_descs(audio_if, audio_if->interface.descs_hs, HAL_USB_SPEED_HIGH);
    // ret = usb_composite_dev_interface_add(&audio_if->interface, 1);

end:
    USB_PRINT_INFO2("usb_audio_driver_desc_register, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_audio_driver_desc_unregister(void *inst)
{
    T_USB_AUDIO_INST *audio_inst = (T_USB_AUDIO_INST *)inst;
    T_USB_AUDIO_IF *audio_if = NULL;
    int ret = ESUCCESS;

    if (!inst)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
        goto end;
    }

    USB_UTILS_LIST_FOREACH(&audio_inst->if_list, T_USB_AUDIO_IF *, audio_if)
    {
        T_USB_EP *usb_ep = NULL;
        USB_UTILS_LIST_FOREACH(&audio_if->interface.eps, T_USB_EP *, usb_ep)
        {
            USB_UTILS_LIST_REMOVE(&audio_if->interface.eps, usb_ep);
            free(usb_ep);
        }

        USB_UTILS_LIST_REMOVE(&audio_inst->if_list, audio_if);
        free(audio_if);
    }

end:
    USB_PRINT_INFO2("usb_audio_driver_desc_unregister, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_audio_driver_ctrl_register(void *inst, T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR *ctrl[])
{
    T_USB_AUDIO_IF *audio_if = NULL;
    uint8_t sub_class = 0;
    T_USB_UTILS_LIST *ctrl_entity_list = NULL;
    T_USB_UTILS_LIST *if_list = &((T_USB_AUDIO_INST *)inst)->if_list;
    int ret = ESUCCESS;

    USB_UTILS_LIST_FOREACH(if_list, T_USB_AUDIO_IF *, audio_if)
    {
        sub_class = audio_if->sub_class;
        USB_PRINT_INFO2("usb_audio_driver_ctrl_register, sub_class 0x%x, audio_if 0x%x", sub_class,
                        audio_if);
        if (sub_class == UAC_SUBCLASS_AUDIOCONTROL)
        {
            ctrl_entity_list = &audio_if->priv.ac_topology->ctrl_entity_list;

            while (*ctrl)
            {
                T_USB_AUDIO_DRIVER_CTRL_ENTITY_HDR *cur_entity = *ctrl;
                T_CONTROL_ENTITY_ITEM *ctrl_item = malloc(sizeof(T_CONTROL_ENTITY_ITEM));
                if (ctrl_item == NULL)
                {
                    ret = -ENOMEM;
                    fail_line = __LINE__;
                    goto end;
                }
                memset(ctrl_item, 0, sizeof(T_CONTROL_ENTITY_ITEM));
                ctrl_item->ctrl_entity = cur_entity;
                USB_UTILS_LIST_INSERT_TAIL(ctrl_entity_list, ctrl_item);
                ctrl++;
            }
        }
    }

end:
    USB_PRINT_INFO2("usb_audio_driver_ctrl_register, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_audio_driver_ctrl_unregister(void *inst)
{
    T_USB_AUDIO_IF *audio_if = NULL;
    uint8_t sub_class = 0;
    T_USB_UTILS_LIST *ctrl_entity_list = NULL;
    T_USB_UTILS_LIST *if_list = NULL;
    T_CONTROL_ENTITY_ITEM *ctrl_entity_item = NULL;
    T_USB_AUDIO_INST *audio_inst = (T_USB_AUDIO_INST *)inst;
    int ret = ESUCCESS;

    if (!audio_inst)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
        goto end;
    }

    if_list = &audio_inst->if_list;
    USB_UTILS_LIST_FOREACH(if_list, T_USB_AUDIO_IF *, audio_if)
    {
        sub_class = audio_if->sub_class;
        if (sub_class == UAC_SUBCLASS_AUDIOCONTROL)
        {
            ctrl_entity_list = &audio_if->priv.ac_topology->ctrl_entity_list;
            USB_UTILS_LIST_FOREACH(ctrl_entity_list, T_CONTROL_ENTITY_ITEM *, ctrl_entity_item)
            {
                USB_UTILS_LIST_REMOVE(ctrl_entity_list, ctrl_entity_item);
                free(ctrl_entity_item);
            }
            free(audio_if->priv.ac_topology);
            audio_if->priv.ac_topology = NULL;
            break;
        }
    }


end:
    USB_PRINT_INFO2("usb_audio_driver_ctrl_unregister, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_audio_driver_cb_register(void *inst, T_USB_AUDIO_DRIVER_CBS *cbs)
{
    T_USB_AUDIO_INST *audio_inst = (T_USB_AUDIO_INST *)inst;

    memcpy(&audio_inst->cbs, cbs, sizeof(T_USB_AUDIO_DRIVER_CBS));

    return ESUCCESS;
}

int usb_audio_driver_cb_unregister(void *inst)
{
    T_USB_AUDIO_INST *audio_inst = (T_USB_AUDIO_INST *)inst;

    memset(&audio_inst->cbs, 0, sizeof(T_USB_AUDIO_DRIVER_CBS));

    return ESUCCESS;
}

int usb_audio_driver_attr_init(void *inst, uint8_t alt_num, T_USB_AUDIO_DRIVER_ATTR attr)
{
    T_USB_AUDIO_INST *audio_inst = (T_USB_AUDIO_INST *)inst;
    T_USB_UTILS_LIST *if_list = &audio_inst->if_list;
    T_USB_AUDIO_IF *audio_if = NULL;
    uint8_t sub_class = 0;
    uint32_t max_buf_size = 0;
    T_ALT_SETTING *alt_setting = NULL;
    T_ALT_SETTINGS *alt_settings = NULL;
    uint32_t max_sample_rate = attr.max_sample_rate * (100 + PERCENT_SAMPLE_RATE_TOLERANCE) / 100;
    int ret = ESUCCESS;

    if (attr.dir == USB_AUDIO_DIR_OUT)
    {
        max_buf_size = attr.bit_width / 8 * attr.chann_num * ((max_sample_rate + 999) / 1000) *
                       (audio_inst->iso_out_proc_interval);
        audio_inst->iso_out_max_buf_size = (audio_inst->iso_out_max_buf_size > max_buf_size) ?
                                           audio_inst->iso_out_max_buf_size : max_buf_size;
    }
    else
    {
        max_buf_size = attr.bit_width / 8 * attr.chann_num * ((max_sample_rate + 999) / 1000) *
                       (audio_inst->iso_in_proc_interval);
        audio_inst->iso_in_max_buf_size = (audio_inst->iso_in_max_buf_size > max_buf_size) ?
                                          audio_inst->iso_in_max_buf_size : max_buf_size;
    }

    USB_UTILS_LIST_FOREACH(if_list, T_USB_AUDIO_IF *, audio_if)
    {
        bool found = false;
        sub_class = audio_if->sub_class;

        if (sub_class == UAC_SUBCLASS_AUDIOSTREAMING)
        {
            alt_settings = (T_ALT_SETTINGS *)audio_if->priv.alt_settings;
            if (alt_settings == NULL)
            {
                fail_line = __LINE__;
                ret = -ENXIO;
                goto end;
            }


            USB_UTILS_LIST_FOREACH((&alt_settings->list), T_ALT_SETTING *, alt_setting)
            {
                USB_PRINT_INFO4("usb_audio_driver_attr_init, alt_num %d-%d, dir %d-%d", alt_setting->value, alt_num,
                                attr.dir, audio_if->dir);
                if (alt_setting->value == alt_num && attr.dir == audio_if->dir)
                {
                    memcpy(&alt_setting->attr, &attr, sizeof(T_USB_AUDIO_DRIVER_ATTR));
                    found = true;
                    break;
                }
            }

            if (found)
            {
                break;
            }

        }
    }

    if (alt_setting == NULL)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
        goto end;
    }

end:
    USB_PRINT_INFO2("usb_audio_driver_attr_init, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_audio_driver_freq_change(void *inst, T_USB_AUDIO_DIR dir, uint32_t freq)
{
    T_USB_AUDIO_INST *audio_inst = (T_USB_AUDIO_INST *)inst;
    T_USB_UTILS_LIST *if_list = &audio_inst->if_list;
    T_USB_AUDIO_IF *audio_if = NULL;
    T_ALT_SETTINGS *alt_settings = NULL;
    bool found = false;
    uint8_t sub_class = 0;
    int ret = ESUCCESS;

    if (audio_inst->uac_ver != USB_AUDIO_VERSION_2)
    {
        fail_line = __LINE__;
        ret = -ESRCH;
        goto end;
    }

    USB_UTILS_LIST_FOREACH(if_list, T_USB_AUDIO_IF *, audio_if)
    {
        sub_class = audio_if->sub_class;

        if (sub_class == UAC_SUBCLASS_AUDIOSTREAMING && audio_if->dir == dir)
        {

            alt_settings = (T_ALT_SETTINGS *)audio_if->priv.alt_settings;
            if (alt_settings == NULL)
            {
                fail_line = __LINE__;
                ret = -ENXIO;
                goto end;
            }
            alt_settings->cs.uac2.freq = freq;
            found = true;
            break;

        }
    }

    if (!found)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
    }
end:
    USB_PRINT_INFO2("usb_audio_driver_freq_change, result %d, fail line %d", ret, fail_line);
    return ret;
}

USB_AUDIO_DRIVER_INTR_PIPE usb_audio_driver_intr_msg_pipe_open(void *inst)
{
    T_USB_AUDIO_INST *audio_inst = (T_USB_AUDIO_INST *)inst;
    T_USB_UTILS_LIST *if_list = &audio_inst->if_list;;
    T_USB_AUDIO_IF *audio_if = NULL;
    uint8_t sub_class = 0;
    T_AC_TOPOLOGY *ac_topology = NULL;

    USB_UTILS_LIST_FOREACH(if_list, T_USB_AUDIO_IF *, audio_if)
    {
        sub_class = audio_if->sub_class;

        if (sub_class == UAC_SUBCLASS_AUDIOCONTROL)
        {
            ac_topology = audio_if->priv.ac_topology;
            break;
        }
    }

    return ac_topology;
}

int usb_audio_driver_intr_msg_pipe_send(USB_AUDIO_DRIVER_INTR_PIPE handle, uint8_t *data,
                                        uint8_t len)
{
    T_AC_TOPOLOGY *ac_topology = (T_AC_TOPOLOGY *)handle;
    void *pipe = ac_topology->pipe_tx;
    int ret = ESUCCESS;

    if (pipe)
    {
        ret = usb_pipe_send(pipe, data, len);
    }
    else
    {
        ret = -EINVAL;
    }

    return ret;
}

int usb_audio_driver_intr_msg_pipe_close(USB_AUDIO_DRIVER_INTR_PIPE handle)
{
    int ret = ESUCCESS;

    return ret;
}

int usb_audio_driver_attr_deinit(void *inst)
{
    T_USB_AUDIO_INST *audio_inst = (T_USB_AUDIO_INST *)inst;
    T_USB_UTILS_LIST *if_list = NULL;
    T_USB_AUDIO_IF *audio_if = NULL;
    uint8_t sub_class = 0;
    T_ALT_SETTING *alt_setting = NULL;
    T_ALT_SETTINGS *alt_settings = NULL;
    int ret = ESUCCESS;

    if (!audio_inst)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
        goto end;
    }
    if_list = &audio_inst->if_list;
    USB_UTILS_LIST_FOREACH(if_list, T_USB_AUDIO_IF *, audio_if)
    {
        sub_class = audio_if->sub_class;

        if (sub_class == UAC_SUBCLASS_AUDIOSTREAMING)
        {
            alt_settings = (T_ALT_SETTINGS *)audio_if->priv.alt_settings;
            if (alt_settings == NULL)
            {
                fail_line = __LINE__;
                ret = -ENXIO;
                goto end;
            }

            USB_UTILS_LIST_FOREACH((&alt_settings->list), T_ALT_SETTING *, alt_setting)
            {
                USB_UTILS_LIST_REMOVE((&alt_settings->list), alt_setting);
                free(alt_setting);
            }
            free(alt_settings);
            audio_if->priv.alt_settings = NULL;
        }
    }
end:
    USB_PRINT_INFO2("usb_audio_driver_attr_deinit, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_audio_driver_init(void)
{
    T_USB_AUDIO_INST *inst = NULL;
    T_USB_AUDIO_IF *audio_if = NULL;
    USB_UTILS_LIST_FOREACH(&g_usb_audio.insts, T_USB_AUDIO_INST *, inst)
    {
        USB_UTILS_LIST_FOREACH(&inst->if_list, T_USB_AUDIO_IF *, audio_if)
        {
            usb_composite_dev_interface_add(&audio_if->interface, 1);
        }
    }
    USB_ISR_USE_ISOC();

    return 0;
}

int usb_audio_driver_deinit(void)
{
    T_USB_AUDIO_INST *inst = NULL;
    T_USB_AUDIO_IF *audio_if = NULL;
    USB_UTILS_LIST_FOREACH(&g_usb_audio.insts, T_USB_AUDIO_INST *, inst)
    {
        USB_UTILS_LIST_FOREACH(&inst->if_list, T_USB_AUDIO_IF *, audio_if)
        {
            usb_composite_dev_interface_remove(&audio_if->interface, 1);
        }
    }


    return 0;
}

