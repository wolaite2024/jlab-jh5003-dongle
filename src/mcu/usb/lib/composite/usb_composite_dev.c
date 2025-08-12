#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "platform_utils.h"
#include "usb_spec20.h"
#include "usb_composite_dev.h"
#include "hal_usb.h"
#include "errno.h"
#include "trace.h"
#include "usb_utils.h"
#include "usb_dm_int.h"
#include "usb_isr.h"
#include "section.h"

//#define CTRL_XFER_DATA_STAGE_MAX_SIZE    (512)
#define MS_OS_STRING_IDX (0xEE)
#define USB_LANGUAGE_ID_UNDEFINED (0x00)

uint16_t ctrl_xfer_data_stage_max_size = 512;

typedef struct _interface_item
{
    struct _interface_item *p_next;
    T_USB_INTERFACE *interface;

    void *cfg;
} T_INTERFACE_ITEM;

typedef struct _usb_cfg
{
    struct _usb_cfg *p_next;
    uint8_t value;
    T_USB_CONFIG_DESC *cfg_desc;
    T_OS_QUEUE if_list;

    void *dev;
} T_USB_CFG;

typedef union _usb_dev_status
{
    uint8_t d8;
    struct
    {
        uint8_t self_powered: 1;
        uint8_t remote_wakeup: 1;
        uint8_t rsv: 6;
    } b;

} T_USB_DEV_STATUS;

typedef struct _usb_string_item
{
    struct _usb_string_item *p_next;
    uint8_t id;
    const char *s;
} T_USB_STRING_ITEM;

typedef struct _usb_strings
{
    struct _usb_strings *p_next;
    uint16_t language;
    T_USB_UTILS_LIST string_list;
} T_USB_STRINGS;

typedef struct _usb_composite_dev
{
    T_HAL_USB_SPEED init_speed;
    T_HAL_USB_SPEED enum_speed;
    T_USB_DEV_STATUS status;
    T_USB_DEVICE_DESC *dev_desc;
    T_USB_STRING_DESC *string_desc;
    T_USB_UTILS_LIST all_strings;
    T_HAL_USB_REQUEST_BLOCK *ctrl_urb;
    void *ep0_handle;
    T_USB_DEVICE_DRIVER *driver;
    T_USB_CFG *cur_cfg;
    T_USB_UTILS_LIST cfg_list;
    T_USB_COMPOSITE_VENDOR_CBS vendor_cb;
} T_USB_COMPOSITE_DEV;

static T_USB_COMPOSITE_DEV *g_usb_composite_dev = NULL;

RAM_TEXT_SECTION
T_HAL_USB_SPEED usb_composite_dev_enum_speed_get(void)
{
    return g_usb_composite_dev->enum_speed;
}

static void usb_composite_dev_enum_speed_set(T_HAL_USB_SPEED speed)
{
    g_usb_composite_dev->enum_speed = speed;
}

static int usb_composite_dev_speed_enum_done(uint8_t speed)
{
    hal_usb_ep_enable(g_usb_composite_dev->ep0_handle, NULL);
    usb_composite_dev_enum_speed_set((T_HAL_USB_SPEED)speed);

    usb_dm_state_set(USB_DEFAULT);

    return ESUCCESS;
}

static T_USB_CFG *usb_composite_dev_search_cfg(uint32_t cfg_val)
{
    T_USB_CFG *cfg = NULL;
    bool found = false;

    USB_UTILS_LIST_FOREACH((&g_usb_composite_dev->cfg_list), T_USB_CFG *, cfg)
    {
        if (cfg->value == cfg_val)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        cfg = NULL;
    }

    return cfg;

}

static int usb_composite_dev_fillup_interface_desc(T_USB_INTERFACE *intf, uint8_t *buf,
                                                   bool other_speed)
{
    uint8_t enumed_speed = usb_composite_dev_enum_speed_get();
    uint8_t total_len = 0;
    struct usb_descriptor_header **if_descs = NULL;

    if (enumed_speed == HAL_USB_SPEED_FULL)
    {
        if (other_speed)
        {
            if_descs = intf->descs_hs;
        }
        else
        {
            if_descs = intf->descs_fs;
        }

    }
    else if (enumed_speed == HAL_USB_SPEED_HIGH)
    {
        if (other_speed)
        {
            if_descs = intf->descs_fs;
        }
        else
        {
            if_descs = intf->descs_hs;
        }
    }
    else
    {
        USB_PRINT_ERROR1("usb_composite_dev_fillup_interface_desc, unsupported speed:%d", enumed_speed);
        return -ENXIO;
    }

    while (*if_descs)
    {
        uint8_t len = (*if_descs)->bLength;
        memcpy(buf + total_len, *if_descs, len);
        total_len += len;
        if_descs++;
    }

    return total_len;
}

static int usb_composite_dev_construct_config_descs(T_USB_COMPOSITE_DEV *dev, uint8_t cfg_val,
                                                    bool other_speed)
{
    T_USB_CONFIG_DESC *cfg_desc = NULL;
    T_INTERFACE_ITEM *if_item = NULL;
    T_USB_INTERFACE *intf = NULL;
    uint16_t total_len = sizeof(T_USB_CONFIG_DESC);
    T_USB_CFG *cfg = NULL;
    //TODO support multiple configurations
    USB_UTILS_LIST_FOREACH(&dev->cfg_list, T_USB_CFG *, cfg)
    {
        USB_PRINT_INFO2("usb_composite_dev_construct_config_descs:%d-%d", cfg->value, cfg_val);
        if (cfg->value == cfg_val)
        {
            cfg_desc = cfg->cfg_desc;
            USB_UTILS_LIST_FOREACH((&cfg->if_list), T_INTERFACE_ITEM *, if_item)
            {
                intf = if_item->interface;
                total_len += usb_composite_dev_fillup_interface_desc(intf, dev->ctrl_urb->buf + total_len,
                                                                     other_speed);
            }
        }
        if (cfg_desc)
        {
            if (other_speed)
            {
                cfg_desc->bDescriptorType = USB_DESC_TYPE_OTHER_SPEED_CONFIG;
            }
            else
            {
                cfg_desc->bDescriptorType = USB_DESC_TYPE_CONFIG;
            }
            cfg_desc->wTotalLength = total_len;
            memcpy(dev->ctrl_urb->buf, cfg_desc, sizeof(T_USB_CONFIG_DESC));
        }
    }

    return total_len;
}

static int usb_composite_dev_construct_qual_desc(T_USB_COMPOSITE_DEV *dev)
{
    T_USB_QUALIFIER_DESC qual_desc =
    {
        .bLength = sizeof(T_USB_QUALIFIER_DESC),
        .bDescriptorType = USB_DESC_TYPE_DEVICE_QUALIFIER,
        .bcdUSB = dev->dev_desc->bcdUSB,
        .bDeviceClass = dev->dev_desc->bDeviceClass,
        .bDeviceSubClass = dev->dev_desc->bDeviceSubClass,
        .bDeviceProtocol = dev->dev_desc->bDeviceProtocol,
        .bMaxPacketSize0 = dev->dev_desc->bMaxPacketSize0,
        .bNumConfigurations = dev->dev_desc->bNumConfigurations,
        .bRESERVED = 0,
    };
    memcpy(dev->ctrl_urb->buf, &qual_desc, sizeof(T_USB_QUALIFIER_DESC));
    return sizeof(T_USB_QUALIFIER_DESC);
}

int usb_composite_dev_convert_usb_string(uint8_t *target_buf, const char *s/*, uint16_t* len*/)
{
    int status = -1;
    uint8_t *src = (uint8_t *)s;
    uint8_t len_ori = strlen(s);
    uint8_t len_target = sizeof(T_USB_DESC_HDR) + len_ori * 2;

    if (target_buf == NULL)
    {
        status = -1;
        return status;
    }

    if ((len_ori == 0) || (len_ori > 127))
    {
        status = -1;
        return status;
    }
    memset(target_buf, 0x00, len_target);

    for (uint8_t idx = 0; idx < len_ori; idx++)
    {
        memcpy((uint8_t *)(target_buf + sizeof(T_USB_DESC_HDR) + 2 * idx),
               (uint8_t *)(src + idx), 1);
    }
    T_USB_DESC_HDR hdr = {.bLength = len_target, .bDescriptorType = USB_DESC_TYPE_STRING};

    memcpy((uint8_t *)target_buf, &hdr, sizeof(T_USB_DESC_HDR));
    status = 0;

    return status;
}

static int usb_composite_dev_construct_string_desc(T_USB_UTILS_LIST *all_strings, uint8_t *buf)
{
    T_USB_STRINGS *strings = NULL;
    T_USB_DESC_HDR string_desc_hdr = {.bLength = sizeof(T_USB_DESC_HDR), .bDescriptorType = USB_DESC_TYPE_STRING};
    int len =  sizeof(T_USB_DESC_HDR);

    USB_UTILS_LIST_FOREACH(all_strings, T_USB_STRINGS *, strings)
    {
        if (strings->language == USB_LANGUAGE_ID_UNDEFINED)
        {
            continue;
        }
        memcpy(buf + len, &strings->language, 2);
        len += 2;
    }

    string_desc_hdr.bLength = len;
    memcpy(buf, &string_desc_hdr, sizeof(T_USB_DESC_HDR));

    return len;
}

static int usb_composite_dev_fill_string(T_USB_COMPOSITE_DEV *dev, uint8_t *buf, uint16_t language,
                                         int id)
{
    int len = -ENOTSUPP;

    if (id == 0)
    {
        len = usb_composite_dev_construct_string_desc(&dev->all_strings, buf);
    }
    else
    {
        T_USB_UTILS_LIST *all_strings = &dev->all_strings;
        T_USB_STRINGS *strings = NULL;
        T_USB_STRING_ITEM *string_item = NULL;
        bool found = false;

        USB_UTILS_LIST_FOREACH(all_strings, T_USB_STRINGS *, strings)
        {
            if (strings->language == language)
            {
                USB_UTILS_LIST_FOREACH(&strings->string_list, T_USB_STRING_ITEM *, string_item)
                {
                    if (string_item->id == id)
                    {
                        found = true;
                        break;
                    }
                }

            }

            if (found)
            {
                break;
            }
        }
        if (found)
        {
            usb_composite_dev_convert_usb_string(buf, (const char *)string_item->s);
            len  = buf[0];
        }

    }

    return len;
}

int usb_composite_dev_vendor_cbs_register(T_USB_COMPOSITE_VENDOR_CBS *cbs)
{
    g_usb_composite_dev->vendor_cb.get = cbs->get;
    g_usb_composite_dev->vendor_cb.set = cbs->set;
    return ESUCCESS;
}

int usb_composite_dev_vendor_cbs_unregister(void)
{
    memset(&g_usb_composite_dev->vendor_cb, 0, sizeof(T_USB_COMPOSITE_VENDOR_CBS));
    return ESUCCESS;
}

static int usb_composite_dev_set_vendor_request_done(T_HAL_USB_REQUEST_BLOCK *urb)
{
    T_HAL_USB_REQUEST_BLOCK *ctrl_urb = (T_HAL_USB_REQUEST_BLOCK *)urb;
    int ret = ESUCCESS;
    USB_PRINT_INFO3("usb_composite_dev_set_vendor_request_done, actual:%d, priv:0x%x, status %d",
                    urb->actual, ctrl_urb->priv, ctrl_urb->status);
    if (ctrl_urb->priv)
    {
        uint16_t request_cmd = *(uint16_t *)ctrl_urb->priv;

        if (g_usb_composite_dev->vendor_cb.set)
        {
            g_usb_composite_dev->vendor_cb.set(request_cmd, ctrl_urb->buf, ctrl_urb->actual);
        }

        ctrl_urb->priv = NULL;
    }

    return ret;
}

static int usb_composite_dev_set_vendor_request_proc(T_USB_DEVICE_REQUEST *ctrl_request,
                                                     T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t len = (ctrl_request->wLength);
    static uint8_t cur_cmd = 0;
    int ret = len;

    cur_cmd = ctrl_request->bRequest;
    ctrl_urb->priv = &cur_cmd;
    ctrl_urb->complete = usb_composite_dev_set_vendor_request_done;

    return ret;
}

static int usb_composite_dev_get_vendor_request_proc(T_USB_DEVICE_REQUEST *ctrl_request,
                                                     T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t len = (ctrl_request->wLength);
    uint8_t cmd = ctrl_request->bRequest;

    if (g_usb_composite_dev->vendor_cb.get)
    {
        g_usb_composite_dev->vendor_cb.get(cmd, ctrl_urb->buf, len);
        ctrl_urb->priv = NULL;
    }
    else
    {
        return -ENOTSUPP;
    }

    return len;
}

static int usb_composite_dev_vendor_request_proc(T_USB_DEVICE_REQUEST *ctrl_request,
                                                 T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    T_HAL_USB_REQUEST_BLOCK      *urb = ctrl_urb;
    uint16_t                    w_index = (ctrl_request->wIndex);
    uint16_t                    w_value = (ctrl_request->wValue);
    uint16_t                    w_length = (ctrl_request->wLength);
    int ret;
    ret = w_length;

    switch (ctrl_request->bmRequestType)
    {
    case USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE:
        ret = usb_composite_dev_set_vendor_request_proc(ctrl_request, ctrl_urb);
        break;
    case USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE:
        ret = usb_composite_dev_get_vendor_request_proc(ctrl_request, ctrl_urb);
        break;

    default:
        goto stall;

    }

    USB_PRINT_INFO5("usb_composite_dev_vendor_request_proc:"" %02x.%02x v%04x i%04x l%d",
                    ctrl_request->bmRequestType, ctrl_request->bRequest, w_value, w_index, w_length);
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
            USB_PRINT_ERROR0("usb_composite_dev_vendor_request_proc:"" hal_usb_ep0_trx error");
        }
    }
    return ret;
stall:
    return -ENOTSUPP;
}

static int usb_composite_dev_class_request_proc(T_USB_DEVICE_REQUEST *ctrl_request,
                                                T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t wIndex = (ctrl_request->wIndex);
    uint8_t  if_num = wIndex & 0xFF;

    T_USB_CFG *cfg = NULL;
    T_INTERFACE_ITEM *if_item = NULL;
    T_USB_INTERFACE *intf = NULL;

    int  ret = -EOPNOTSUPP;
    USB_UTILS_LIST_FOREACH((&g_usb_composite_dev->cfg_list), T_USB_CFG *, cfg)
    {
        switch (ctrl_request->bmRequestType & USB_RECIP_MASK)
        {
        case USB_RECIP_DEVICE:
            {
                USB_PRINT_INFO0("usb_composite_dev_class_request_proc: USB_RECIP_DEVICE");
                USB_UTILS_LIST_FOREACH((&cfg->if_list), T_INTERFACE_ITEM *, if_item)
                {
                    ret = if_item->interface->ctrl_request_proc(if_item->interface, ctrl_request, ctrl_urb);
                    if (ret != -EOPNOTSUPP)
                    {
                        break;
                    }
                }
                if (ret == -EOPNOTSUPP)
                {
                    ret = usb_composite_dev_vendor_request_proc(ctrl_request, ctrl_urb);
                }
                break;
            }
        case USB_RECIP_INTERFACE:
            {
                USB_UTILS_LIST_FOREACH((&cfg->if_list), T_INTERFACE_ITEM *, if_item)
                {
                    intf = if_item->interface;

                    if (intf->if_num == if_num)
                    {
                        ret = if_item->interface->ctrl_request_proc(if_item->interface, ctrl_request, ctrl_urb);
                        break;
                    }
                }
                break;
            }

        case USB_RECIP_ENDPOINT:
            {
                uint8_t ep_addr = (wIndex & 0x80) | (wIndex & 0x0f);
                bool found = false;
                USB_PRINT_INFO1("usb_composite_dev_class_request_proc: USB_RECIP_ENDPOINT, endp = %d", ep_addr);
                void *ep_handle = hal_usb_ep_handle_get(ep_addr);
                T_USB_EP *ep_item =  NULL;
                USB_UTILS_LIST_FOREACH((&cfg->if_list), T_INTERFACE_ITEM *, if_item)
                {
                    intf = if_item->interface;
                    USB_UTILS_LIST_FOREACH((&intf->eps), T_USB_EP *, ep_item)
                    {
                        USB_PRINT_INFO4("usb_composite_dev_class_request_proc: endpoint Handle = 0x%x-0x%x, addr:0x%x, if num:%d",
                                        ep_item->ep_handle, ep_handle, ep_item->addr, intf->if_num);
                        if (ep_item->ep_handle == ep_handle)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (found)
                    {
                        break;
                    }
                }

                if (found)
                {
                    ret = if_item->interface->ctrl_request_proc(if_item->interface, ctrl_request, ctrl_urb);
                }
                break;
            }
        }

        if (ret < 0)
        {
            ret = hal_usb_ep_stall_set(g_usb_composite_dev->ep0_handle, ret);
        }
    }
    return ret;
}

static int usb_composite_dev_get_desc(T_USB_COMPOSITE_DEV *dev, T_USB_DEVICE_REQUEST *ctrl_request,
                                      T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t wIndex = (ctrl_request->wIndex);
    uint16_t wValue = (ctrl_request->wValue);
    uint16_t wLength = (ctrl_request->wLength);
    T_HAL_USB_REQUEST_BLOCK *urb = ctrl_urb;

    int32_t ret = ESUCCESS;

    if ((ctrl_request->bmRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE)
    {
        return usb_composite_dev_class_request_proc(ctrl_request, urb);
    }
    else
    {
        switch (wValue >> 8)
        {
        case USB_DESC_TYPE_DEVICE:

            ret = USB_UTILS_MIN(wLength, (uint16_t) sizeof(T_USB_DEVICE_DESC));
            memcpy(urb->buf, g_usb_composite_dev->dev_desc, ret);

            USB_PRINT_INFO1("usb_composite_dev_get_desc:""USB_GET_DESCRIPTOR(DEVICE), Length = 0x%x",
                            ret);
            break;

        case USB_DESC_TYPE_CONFIG:
            ret = usb_composite_dev_construct_config_descs(g_usb_composite_dev, (wValue & 0xff) + 1, false);
            if (ret >= 0)
            {
                ret = USB_UTILS_MIN(wLength, (uint16_t) ret);
            }
            USB_PRINT_INFO1("usb_composite_dev_get_desc:""USB_GET_DESCRIPTOR(CONFIG), Length = 0x%x",
                            wLength);
            break;
        case USB_DESC_TYPE_DEVICE_QUALIFIER:
            {
                if (usb_composite_dev_enum_speed_get() == HAL_USB_SPEED_HIGH)
                {
                    ret =  usb_composite_dev_construct_qual_desc(g_usb_composite_dev);
                    if (ret >= 0)
                    {
                        ret = USB_UTILS_MIN(wLength, (uint16_t) ret);
                    }
                }
                else
                {
                    ret = -ENOTSUPP;
                }
            }
            break;
        case USB_DESC_TYPE_OTHER_SPEED_CONFIG:
            {
                ret =  usb_composite_dev_construct_config_descs(g_usb_composite_dev, (wValue & 0xff) + 1, true);
                if (ret >= 0)
                {
                    ret = USB_UTILS_MIN(wLength, (uint16_t) ret);
                }
            }
            break;
        case USB_DESC_TYPE_STRING:

            ret = usb_composite_dev_fill_string(g_usb_composite_dev, urb->buf, wIndex, wValue & 0xff);

            if (ret >= 0)
            {
                ret = USB_UTILS_MIN(wLength, (uint16_t) ret);
            }
            USB_PRINT_INFO1("usb_composite_dev_get_desc:""USB_GET_DESCRIPTOR(STRING(%d))",
                            wValue & 0xff);
            break;
        default:
            ret = -ENOTSUPP;
        }
    }
    return ret;
}

static int usb_composite_dev_reset_cfg(T_USB_CFG *cfg, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    T_INTERFACE_ITEM *if_item = NULL;
    T_USB_INTERFACE *intf = NULL;
    int32_t ret = ESUCCESS;

    USB_UTILS_LIST_FOREACH((&cfg->if_list), T_INTERFACE_ITEM *, if_item)
    {
        intf = if_item->interface;

        ret = intf->alt_set(if_item->interface, ctrl_urb, 0);
    }

    return ret;
}

static int usb_composite_dev_set_cfg(T_USB_COMPOSITE_DEV *dev, T_USB_DEVICE_REQUEST *ctrl_request,
                                     T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t cfg_val = ctrl_request->wValue & 0xFF;
    T_USB_CFG *cur_cfg =  dev->cur_cfg;
    T_USB_CFG *new_cfg = usb_composite_dev_search_cfg(cfg_val);
    int32_t ret = ESUCCESS;

    USB_PRINT_INFO1("usb_composite_dev_set_cfg:"" USB_REQ_SET_CONFIGURATION[%u]", cfg_val);
    if (ctrl_request->bmRequestType != USB_DIR_OUT)
    {
        ret = -EOPNOTSUPP;
        goto end;
    }

    if (cur_cfg)
    {
        ret = usb_composite_dev_reset_cfg(cur_cfg, ctrl_urb);
    }

    if (new_cfg == NULL && cfg_val != 0)
    {
        ret = -ENXIO;
        goto end;
    }

    if (new_cfg)
    {
        ret += usb_composite_dev_reset_cfg(new_cfg, ctrl_urb);
        usb_dm_state_set(USB_CONFIGURED);
    }
    else
    {
        usb_dm_state_set(USB_ADDRESSED);
    }

    if (ret == ESUCCESS)
    {
        dev->cur_cfg = new_cfg;
    }

end:
    return ret;
}

static int usb_composite_dev_get_cfg(T_USB_COMPOSITE_DEV *dev, T_USB_DEVICE_REQUEST *ctrl_request,
                                     T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t wLength = (ctrl_request->wLength);
    T_HAL_USB_REQUEST_BLOCK *urb = ctrl_urb;
    T_USB_CFG *cfg = dev->cur_cfg;
    int32_t ret = ESUCCESS;

    if (ctrl_request->bmRequestType != USB_DIR_IN)
    {
        ret = -EOPNOTSUPP;
        goto end;
    }
    if (cfg)
    {
        *(uint8_t *)urb->buf = cfg->cfg_desc->bConfigurationValue;
    }
    else
    {
        *(uint8_t *)urb->buf = 0;
    }
    ret = USB_UTILS_MIN(wLength, (uint16_t) 1);
end:
    return ret;
}

static int usb_composite_dev_std_if_request_proc(T_USB_COMPOSITE_DEV *dev,
                                                 T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t wIndex = (ctrl_request->wIndex);
    uint8_t  if_num = wIndex & 0xFF;
    uint16_t wValue = (ctrl_request->wValue);
    uint16_t wLength = (ctrl_request->wLength);
    uint8_t bRequest = ctrl_request->bRequest;
    T_HAL_USB_REQUEST_BLOCK *urb = ctrl_urb;
    T_INTERFACE_ITEM *if_item = NULL;
    T_USB_INTERFACE *intf = NULL;
    T_USB_CFG *cfg = dev->cur_cfg;
    int32_t ret = ESUCCESS;

    USB_UTILS_LIST_FOREACH((&cfg->if_list), T_INTERFACE_ITEM *, if_item)
    {
        intf = if_item->interface;
        if (intf->if_num == if_num)
        {
            if (bRequest == USB_REQ_CODE_GET_INTERFACE)
            {
                ret = intf->alt_get(intf, urb);
                ret = USB_UTILS_MIN(wLength, (uint16_t) ret);
            }
            else
            {
                ret = intf->alt_set(intf, urb, wValue);
            }
        }
    }
    return ret;
}

static int usb_composite_dev_get_status(T_USB_COMPOSITE_DEV *dev,
                                        T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    T_HAL_USB_REQUEST_BLOCK *urb = ctrl_urb;
    uint8_t recipient = UT_GET_RECIPIENT(ctrl_request->bmRequestType);
    uint16_t status = 0;

    int32_t ret = ESUCCESS;

    USB_PRINT_INFO0("usb_composite_dev_get_status:"" USB_GET_STATUS");
    switch (recipient)
    {
    case USB_RECIP_DEVICE:
        {
            status = (uint16_t)(g_usb_composite_dev->status.d8);
            break;
        }
    case USB_RECIP_INTERFACE:
        status = 0;
        break;

    case USB_RECIP_ENDPOINT:
        {
            void *ep_handle = hal_usb_ep_handle_get(UGETW(ctrl_request->wIndex));
            if (ep_handle == NULL || UGETW(ctrl_request->wLength) > 2)
            {
                ret = -EOPNOTSUPP;
                goto end;
            }

            status = hal_usb_ep_stall_status_get(ep_handle);
            break;
        }
    default:
        ret = -ENOTSUPP;
        goto end;
    }
    ret = 2;
    memcpy(urb->buf, &status, 2);
end:
    return ret;
}

static int usb_composite_dev_clear_feature(T_USB_COMPOSITE_DEV *dev,
                                           T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t wIndex = (ctrl_request->wIndex);
    uint16_t wValue = (ctrl_request->wValue);
    uint8_t recipient = UT_GET_RECIPIENT(ctrl_request->bmRequestType);
    T_HAL_USB_REQUEST_BLOCK *urb = ctrl_urb;
    int32_t ret = ESUCCESS;

    USB_PRINT_INFO0("usb_composite_dev_clear_feature:"" USB_CLEAR_FEATURE");
    switch (recipient)
    {
    case USB_RECIP_DEVICE:
        {
            switch (UGETW(wValue))
            {
            case UF_DEVICE_REMOTE_WAKEUP:
                dev->status.b.remote_wakeup = 0;
                break;
            case UF_TEST_MODE:
                //TODO Add CLEAR_FEATURE for TEST modes. */
                break;
            default:
                ret = -EOPNOTSUPP;
                break;
            }
            break;
        }

    case USB_RECIP_ENDPOINT:
        {
            uint8_t addr = UGETW(wIndex);
            if (addr == 0)
            {
                void *ep_handle = hal_usb_ep_handle_get(addr);
                if (ep_handle == 0)
                {
                    ret = -EOPNOTSUPP;
                    break;
                }
                hal_usb_ep_stall_clear(ep_handle);
            }
            else
            {
                ret = usb_composite_dev_class_request_proc(ctrl_request, urb);
            }
            break;
        }

    default:
        {
            ret = -EOPNOTSUPP;
            break;
        }
    }

    return ret;
}

static int usb_composite_dev_test_mode_enter(T_HAL_USB_REQUEST_BLOCK *urb)
{
    uint32_t test_mode_sel = (uint32_t)urb->priv;

    //wait for status stage done
    platform_delay_ms(2);
    USB_PRINT_INFO0("usb_composite_dev_test_mode_enter");
    hal_usb_do_test_mode((uint8_t)test_mode_sel);

    return 0;
}

static int usb_composite_dev_set_feature(T_USB_COMPOSITE_DEV *dev,
                                         T_USB_DEVICE_REQUEST *ctrl_request, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint16_t wIndex = (ctrl_request->wIndex);
    uint16_t wValue = (ctrl_request->wValue);
    T_HAL_USB_REQUEST_BLOCK *urb = ctrl_urb;
    uint8_t recipient = UT_GET_RECIPIENT(ctrl_request->bmRequestType);
    int32_t ret = ESUCCESS;

    USB_PRINT_INFO0("usb_composite_dev_set_feature:"" USB_SET_FEATURE");
    switch (recipient)
    {
    case USB_RECIP_DEVICE:
        switch (UGETW(wValue))
        {
        case UF_DEVICE_REMOTE_WAKEUP:
            dev->status.b.remote_wakeup = 1;
            break;
        case UF_TEST_MODE:
            {
                uint32_t test_mode_sel = UGETW(wIndex) >> 8;
                urb->priv = (void *)test_mode_sel;
                urb->complete = usb_composite_dev_test_mode_enter;
            }
            break;
        default:
            ret = -EOPNOTSUPP;
            break;

        }
        break;

    case USB_RECIP_INTERFACE:
    case USB_RECIP_ENDPOINT:
        ret = usb_composite_dev_class_request_proc(ctrl_request, urb);
        break;
    }

    return ret;
}

uint8_t usb_composite_dev_ep0_mps_get(void)
{
    return (g_usb_composite_dev->dev_desc->bMaxPacketSize0);
}

static int usb_composite_dev_standard_request_proc(T_USB_DEVICE_REQUEST *ctrl_request,
                                                   T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    uint8_t bRequest = ctrl_request->bRequest;
    uint32_t w_length = ctrl_request->wLength;
    T_HAL_USB_REQUEST_BLOCK *urb = ctrl_urb;

    int32_t ret = ESUCCESS;

    USB_PRINT_INFO1("usb_composite_dev_standard_request_proc bRequest:%d", bRequest);
    switch (bRequest)
    {
    case USB_REQ_CODE_GET_DESCRIPTOR:
        {
            ret = usb_composite_dev_get_desc(g_usb_composite_dev, ctrl_request, urb);
            break;
        }

    case USB_REQ_CODE_SET_CONFIGURATION:
        {
            ret = usb_composite_dev_set_cfg(g_usb_composite_dev, ctrl_request, urb);
            break;
        }
    case USB_REQ_CODE_GET_CONFIGURATION:
        {
            USB_PRINT_INFO0("usb_composite_dev_standard_request_proc:"" USB_GET_CONFIGURATION");

            ret = usb_composite_dev_get_cfg(g_usb_composite_dev, ctrl_request, urb);
            break;
        }
    case USB_REQ_CODE_GET_INTERFACE:
    case USB_REQ_CODE_SET_INTERFACE:
        {
            ret = usb_composite_dev_std_if_request_proc(g_usb_composite_dev, ctrl_request, urb);
            break;
        }

    case USB_REQ_CODE_GET_STATUS:
        {
            ret = usb_composite_dev_get_status(g_usb_composite_dev, ctrl_request, urb);
            break;

        }
    case USB_REQ_CODE_CLEAR_FEATURE:
        {
            ret = usb_composite_dev_clear_feature(g_usb_composite_dev, ctrl_request, urb);
            break;
        }

    case USB_REQ_CODE_SET_FEATURE:
        {
            ret = usb_composite_dev_set_feature(g_usb_composite_dev, ctrl_request, urb);
            break;
        }
    case USB_REQ_CODE_SET_ADDRESS:
        {
            USB_PRINT_INFO1("usb_composite_dev_standard_request_proc:"" USB_REQ_CODE_SET_ADDRESS:%d",
                            UGETW(ctrl_request->wValue));
            ret = hal_usb_set_address(UGETW(ctrl_request->wValue));
            usb_dm_state_set(USB_ADDRESSED);
            break;
        }
    default:
        ret = -EOPNOTSUPP;
        break;
    }

    if (ret >= 0)
    {
        urb->length = ret;
        if (ret > 0 && ret < w_length)
        {
            urb->zlp = (ret % usb_composite_dev_ep0_mps_get() == 0);
        }
        ret = hal_usb_ep0_trx(g_usb_composite_dev->ep0_handle, urb);

        if (urb->length == 0 && urb->complete)
        {
            urb->status = ret;
            urb->complete(urb);
        }
    }
    else
    {
        hal_usb_ep_stall_set(g_usb_composite_dev->ep0_handle, ret);
    }
    return ret;

}


static int usb_composite_dev_setup(uint8_t *ctrl)
{
    T_USB_DEVICE_REQUEST *ctrl_request  = (T_USB_DEVICE_REQUEST *)ctrl;

    uint8_t request_type = ctrl_request->bmRequestType;

    int ret = 0;
//    USB_PRINT_INFO1("usb_composite_dev_setup:0x%x", request_type);
    g_usb_composite_dev->ctrl_urb->complete = NULL;
    g_usb_composite_dev->ctrl_urb->zlp = 0;
    switch (request_type & USB_TYPE_MASK)
    {
    case USB_TYPE_STANDARD:
        {
            ret = usb_composite_dev_standard_request_proc(ctrl_request, g_usb_composite_dev->ctrl_urb);
        }
        break;

    case USB_TYPE_CLASS:
    case USB_TYPE_VENDOR:
        {
            ret = usb_composite_dev_class_request_proc(ctrl_request, g_usb_composite_dev->ctrl_urb);
        }
        break;

    default:
        ret = -EINVAL;
        break;
    }

    return ret;

}

static int usb_composite_dev_suspend(void)
{
    T_INTERFACE_ITEM *if_item = NULL;
    T_USB_CFG *cfg = NULL;

    USB_UTILS_LIST_FOREACH((&g_usb_composite_dev->cfg_list), T_USB_CFG *, cfg)
    {
        USB_UTILS_LIST_FOREACH(&cfg->if_list, T_INTERFACE_ITEM *, if_item)
        {
            T_USB_INTERFACE *interface = if_item->interface;
            if (interface && interface->suspend)
            {
                interface->suspend(interface);
            }
        }

    }
    // usb_dm_suspend_enter();
    return ESUCCESS;
}

static int usb_composite_dev_resume(void)
{
    T_INTERFACE_ITEM *if_item = NULL;
    T_USB_CFG *cfg = NULL;

    USB_UTILS_LIST_FOREACH((&g_usb_composite_dev->cfg_list), T_USB_CFG *, cfg)
    {
        USB_UTILS_LIST_FOREACH(&cfg->if_list, T_INTERFACE_ITEM *, if_item)
        {
            T_USB_INTERFACE *interface = if_item->interface;
            if (interface && interface->resume)
            {
                interface->resume(interface);
            }
        }

    }

    return ESUCCESS;
}

int usb_composite_dev_remote_wakeup(void)
{
    int ret = ESUCCESS;
    uint8_t usb_dm_state = usb_dm_state_get();

    if (usb_dm_state == USB_SUSPENDED)
    {
        ret = usb_dm_suspend_exit();
        if (ret)
        {
            USB_PRINT_ERROR0("usb_composite_dev_remote_wakeup, exit suspend state fail");
            return ret;
        }
    }

    if (g_usb_composite_dev->status.b.remote_wakeup)
    {
        ret = hal_usb_remote_wakeup();
    }
    else
    {
        ret = -ENOTSUPP;
        USB_PRINT_ERROR0("usb_composite_dev_remote_wakeup, NOT SUPPORT remote wakeup");
    }
    return ret;
}

static const T_USB_DEVICE_DRIVER g_usb_composite_drv =
{
    .reset = NULL,
    .speed_enum_done = usb_composite_dev_speed_enum_done,
    .setup = usb_composite_dev_setup,
    .suspend = usb_composite_dev_suspend,
    .resume = usb_composite_dev_resume,
};

int usb_composite_dev_interface_add(T_USB_INTERFACE *interface, uint8_t cfg_val)
{
    T_USB_CFG *cfg = usb_composite_dev_search_cfg(cfg_val);

    if (cfg)
    {
        T_INTERFACE_ITEM *if_item = malloc(sizeof(T_INTERFACE_ITEM));
        memset(if_item, 0, sizeof(T_INTERFACE_ITEM));
        if_item->interface = interface;
        if_item->cfg = cfg;
        USB_UTILS_LIST_INSERT_TAIL(&cfg->if_list, if_item);

        T_USB_CONFIG_DESC *cfg_desc = cfg->cfg_desc;
        interface->if_num = cfg_desc->bNumInterfaces;
        cfg_desc->bNumInterfaces += 1;
        if (interface->create)
        {
            return interface->create(interface);
        }

    }

    return -ENXIO;

}

int usb_composite_dev_interface_remove(T_USB_INTERFACE *interface, uint8_t cfg_val)
{
    T_USB_CFG *cfg = usb_composite_dev_search_cfg(cfg_val);
    T_INTERFACE_ITEM *if_item = NULL;
    uint8_t if_num = 0;
    bool found = false;
    int ret = ESUCCESS;

    if (!interface)
    {
        ret = -ENXIO;
        goto end;
    }

    if_num = interface->if_num;
    if (cfg)
    {
        USB_UTILS_LIST_FOREACH(&cfg->if_list, T_INTERFACE_ITEM *, if_item)
        {
            if (if_item->interface && if_item->interface->if_num == if_num)
            {
                found = true;
                USB_UTILS_LIST_REMOVE(&cfg->if_list, if_item);
                cfg->cfg_desc->bNumInterfaces--;
                if (if_item->interface->release)
                {
                    ret = if_item->interface->release(if_item->interface);
                }
                free(if_item);
                break;
            }
        }
    }

    if (!found)
    {
        ret = -ENXIO;
    }
end:
    USB_PRINT_INFO1("usb_composite_dev_interface_remove, ret:%d", ret);
    return ret;
}

int usb_composite_dev_string_add(uint16_t language, uint8_t id, const char *s)
{
    T_USB_UTILS_LIST *all_strings = &g_usb_composite_dev->all_strings;
    T_USB_STRINGS *strings = NULL;
    T_USB_STRING_ITEM *string_item = NULL;
    bool found = false;
    int ret = ESUCCESS;

    USB_UTILS_LIST_FOREACH(all_strings, T_USB_STRINGS *, strings)
    {
        if (strings->language == language)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        strings = malloc(sizeof(T_USB_STRINGS));
        if (strings)
        {
            memset(strings, 0, sizeof(T_USB_STRINGS));
            strings->language = language;
            USB_UTILS_LIST_INIT(&strings->string_list);
            USB_UTILS_LIST_INSERT_TAIL(all_strings, strings);
        }
        else
        {
            ret = -ENOMEM;
            goto end;
        }
    }

    string_item = malloc(sizeof(T_USB_STRING_ITEM));
    if (string_item)
    {
        memset(string_item, 0, sizeof(T_USB_STRING_ITEM));
        string_item->id = id;
        string_item->s = s;
    }
    else
    {
        ret = -ENOMEM;
        goto end;
    }
    USB_UTILS_LIST_INSERT_TAIL(&strings->string_list, string_item);


end:
    USB_PRINT_INFO1("usb_composite_dev_string_add, ret:%d", ret);
    return ret;
}

int usb_composite_dev_string_remove(uint16_t language, uint8_t id, const char *s)
{
    T_USB_UTILS_LIST *all_strings = &g_usb_composite_dev->all_strings;
    T_USB_STRINGS *strings = NULL;
    T_USB_STRING_ITEM *string_item = NULL;
    bool found = false;
    int ret = ESUCCESS;

    USB_UTILS_LIST_FOREACH(all_strings, T_USB_STRINGS *, strings)
    {
        if (strings->language == language)
        {
            USB_UTILS_LIST_FOREACH(&strings->string_list, T_USB_STRING_ITEM *, string_item)
            {
                if (string_item->id == id)
                {
                    found = true;
                    USB_UTILS_LIST_REMOVE(&strings->string_list, string_item);
                    free(string_item);

                    if (USB_UTILS_LIST_EMPTY(&strings->string_list))
                    {
                        USB_UTILS_LIST_REMOVE(all_strings, strings);
                        free(strings);
                    }
                    break;
                }
            }
            if (found)
            {
                break;
            }
        }
    }

    if (!found)
    {
        ret = -ENXIO;
    }

    USB_PRINT_INFO1("usb_composite_dev_string_remove, ret:%d", ret);
    return ret;
}

int usb_composite_dev_cfg_add(T_USB_CONFIG_DESC *cfg_desc)
{
    T_USB_CFG *usb_cfg = NULL;
    int ret = ESUCCESS;

    if (cfg_desc)
    {
        usb_cfg = malloc(sizeof(T_USB_CFG));
        if (usb_cfg)
        {
            cfg_desc->bNumInterfaces = 0;
            memset(usb_cfg, 0, sizeof(T_USB_CFG));
            usb_cfg->cfg_desc = cfg_desc;
            usb_cfg->dev = g_usb_composite_dev;
            usb_cfg->value = cfg_desc->bConfigurationValue;
            USB_UTILS_LIST_INIT(&usb_cfg->if_list);
            USB_UTILS_LIST_INSERT_TAIL(&g_usb_composite_dev->cfg_list, usb_cfg);

            if (cfg_desc->bmAttributes & USB_ATTRIBUTE_SELFPOWERED)
            {
                g_usb_composite_dev->status.b.self_powered = 1;
            }
        }
        else
        {
            ret = -ENOMEM;
        }
    }
    else
    {
        ret = -ENXIO;
    }

    USB_PRINT_INFO1("usb_composite_dev_cfg_add, ret:%d", ret);
    return ret;
}

int usb_composite_dev_cfg_remove(T_USB_CONFIG_DESC *cfg_desc)
{
    uint8_t cfg_val = cfg_desc->bConfigurationValue;
    T_USB_CFG *usb_cfg = NULL;
    int ret = ESUCCESS;

    usb_cfg = usb_composite_dev_search_cfg(cfg_val);
    if (usb_cfg)
    {
        T_INTERFACE_ITEM *if_item = NULL;
        USB_UTILS_LIST_FOREACH(&usb_cfg->if_list, T_INTERFACE_ITEM *, if_item)
        {
            usb_composite_dev_interface_remove(if_item->interface, cfg_val);
        }
        USB_UTILS_LIST_REMOVE(&g_usb_composite_dev->cfg_list, usb_cfg);
        free(usb_cfg);
    }

    USB_PRINT_INFO1("usb_composite_dev_cfg_remove, ret:%d", ret);
    return ret;
}

int usb_composite_dev_ctrl_xfer_buf_init(uint16_t buf_len)
{
    ctrl_xfer_data_stage_max_size = buf_len;
    return ESUCCESS;
}

int usb_composite_dev_init(T_USB_DEVICE_DESC *dev_desc)
{
    int ret = ESUCCESS;

    g_usb_composite_dev = malloc(sizeof(T_USB_COMPOSITE_DEV));
    if (g_usb_composite_dev)
    {
        memset(g_usb_composite_dev, 0, sizeof(T_USB_COMPOSITE_DEV));
        g_usb_composite_dev->dev_desc = (T_USB_DEVICE_DESC *)dev_desc;

        USB_UTILS_LIST_INIT(&g_usb_composite_dev->cfg_list);
        USB_UTILS_LIST_INIT(&g_usb_composite_dev->all_strings);
        g_usb_composite_dev->driver = (T_USB_DEVICE_DRIVER *)&g_usb_composite_drv;
        usb_isr_device_driver_register(g_usb_composite_dev->driver);

        g_usb_composite_dev->ctrl_urb = hal_usb_urb_alloc(ctrl_xfer_data_stage_max_size);
        // g_usb_composite_dev->ctrl_urb->complete = setup_complete;

        g_usb_composite_dev->ep0_handle = hal_usb_ep_handle_get(0);
        g_usb_composite_dev->ctrl_urb->ep_handle = g_usb_composite_dev->ep0_handle;
        g_usb_composite_dev->cur_cfg = NULL;
    }
    else
    {
        ret = -ENOMEM;
    }

    return ret;
}

int usb_composite_dev_deinit(void)
{
    int ret = ESUCCESS;

    if (g_usb_composite_dev)
    {
        usb_isr_device_driver_unregister(g_usb_composite_dev->driver);

        if (g_usb_composite_dev->ctrl_urb)
        {
            hal_usb_urb_free(g_usb_composite_dev->ctrl_urb);
        }

        free(g_usb_composite_dev);
        g_usb_composite_dev = NULL;
    }

    return ret;
}
