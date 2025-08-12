/**
 * @copyright Copyright (C) 2023 Realtek Semiconductor Corporation.
 *
 * @file usb_cdc_driver.c
 * @version 1.0
 * @brief
 *
 * @note:
 */

#include <string.h>
#include <stdlib.h>
#include "usb_spec20.h"
#include "usb_composite_dev.h"
#include "hal_usb.h"
#include "usb_utils.h"
#include "errno.h"
#include "usb_cdc_spec.h"
#include "usb_cdc_driver.h"
#include "trace.h"
#include "usb_pipe.h"

static int fail_line = 0;

typedef struct _ep_priv
{
    T_USB_ENDPOINT_DESC *fs_desc;
    T_USB_ENDPOINT_DESC *hs_desc;
    void *pipe;
    uint8_t ep_enable: 1;
} T_EP_PRIV;

typedef struct _usb_cdc_if
{
    struct _usb_cdc_if *p_next;
    T_USB_INTERFACE intf;
    uint8_t cur_alt;
} T_USB_CDC_IF;

typedef struct _usb_cdc
{
    T_USB_UTILS_LIST if_list;
} T_USB_CDC;

T_USB_CDC *g_usb_cdc = NULL;

int usb_cdc_driver_set_line_coding_complete(T_HAL_USB_REQUEST_BLOCK *urb)
{
    //T_LINE_CODING *coding = (T_LINE_CODING *)urb->buf;
    return ESUCCESS;
}

int usb_cdc_driver_set_line_state_complete(T_HAL_USB_REQUEST_BLOCK *urb)
{
    //T_LINE_CODING *coding = (T_LINE_CODING *)urb->buf;
    return ESUCCESS;
}

static int usb_cdc_driver_if_ctrl_request_proc(T_USB_INTERFACE *interface,
                                               T_USB_DEVICE_REQUEST *ctrl_request,
                                               T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    T_HAL_USB_REQUEST_BLOCK      *urb = ctrl_urb;
    uint16_t                    w_index = (ctrl_request->wIndex);
    uint16_t                    w_value = (ctrl_request->wValue);
    uint16_t                    w_length = (ctrl_request->wLength);
    int status = 0;
    uint16_t length;
    length = w_length;


    USB_PRINT_INFO5("usb_cdc_driver_if_ctrl_request_proc, bmRequestType %02x, bRequest %02x, w_value %04x, w_index %04x, w_length %d",
                    ctrl_request->bmRequestType, ctrl_request->bRequest, w_value, w_index, w_length);

    switch (ctrl_request->bRequest)
    {
    case CDC_GET_LINE_CODING:
        {
            if ((ctrl_request->bmRequestType & USB_DIR_MASK) != USB_DIR_IN)
            {
                goto stall;
            }
            T_LINE_CODING coding = {.dwDTERate = 115200, .bCharFormat = 0, .bParityType = 0, .bDataBits = 8};
            memcpy(urb->buf, &coding, length);
        }
        break;
    case CDC_SET_LINE_CODING:
        {
            if ((ctrl_request->bmRequestType & USB_DIR_MASK) != USB_DIR_OUT)
            {
                goto stall;
            }
            urb->complete = usb_cdc_driver_set_line_coding_complete;
        }
        break;
    case CDC_SET_CONTROL_LINE_STATE:
        {
            if ((ctrl_request->bmRequestType & USB_DIR_MASK) != USB_DIR_OUT)
            {
                goto stall;
            }
            urb->complete = usb_cdc_driver_set_line_state_complete;
        }
        break;
    case USB_REQ_CODE_CLEAR_FEATURE:
        {
            if ((ctrl_request->bmRequestType & USB_DIR_MASK) != USB_DIR_OUT)
            {
                goto stall;
            }
            uint8_t addr = UGETW(w_index);
            void *ep_handle = hal_usb_ep_handle_get(addr);
            if (ep_handle == 0)
            {
                goto stall;
            }
            hal_usb_ep_stall_clear(ep_handle);
            goto done;
        }

    case USB_REQ_CODE_SET_FEATURE:
        {
            if ((ctrl_request->bmRequestType & USB_DIR_MASK) != USB_DIR_OUT)
            {
                goto stall;
            }
            if (UGETW(w_value) == UF_ENDPOINT_HALT)
            {
                uint8_t addr = UGETW(w_index);
                void *ep_handle = hal_usb_ep_handle_get(addr);
                if (ep_handle == 0)
                {
                    goto stall;
                }
                hal_usb_ep_stall_set(ep_handle, 0);
            }
            goto done;
        }

    default:
        goto stall;
    }

    urb->length = length;
    if (length > 0 && length < w_length)
    {
        urb->zlp = (length % usb_composite_dev_ep0_mps_get() == 0);
    }
    status = hal_usb_ep0_trx(urb->ep_handle, urb);

    if (status < 0)
    {
        USB_PRINT_ERROR0("usb_cdc_driver_if_ctrl_request_proc, hal_usb_ep0_trx error");
    }
done:
    return status;
stall:
    return -EOPNOTSUPP;
}

static int usb_cdc_driver_descs_parse(T_USB_INTERFACE *interface, T_USB_DESC_HDR **descs,
                                      uint8_t speed)
{
    T_USB_DESC_HDR **desc = descs;
    T_USB_EP *ep = NULL;
    T_EP_PRIV *ep_priv;
    int ret = ESUCCESS;
    bool found = false;

    for (; *desc; desc++)
    {
        if ((*desc)->bDescriptorType == USB_DESC_TYPE_INTERFACE)
        {
            //allocate interface id
            T_USB_INTERFACE_DESC *cur_desc = (T_USB_INTERFACE_DESC *)(*desc);
            cur_desc->bInterfaceNumber = interface->if_num;
        }
        else if ((*desc)->bDescriptorType == USB_DESCTYPE_CLASS_INTERFACE)
        {
            T_USB_INTERFACE_DESC *cur_desc = (T_USB_INTERFACE_DESC *)(*desc);
            if (((T_CDC_UNION_FUNC_DESC *)cur_desc)->bDescriptorSubtype == 0x06)
            {
                ((T_CDC_UNION_FUNC_DESC *)cur_desc)->bControlInterface = interface->if_num;
                ((T_CDC_UNION_FUNC_DESC *)cur_desc)->bSubordinateInterface = interface->if_num + 1;
            }
        }
        else if ((*desc)->bDescriptorType == USB_DESC_TYPE_ENDPOINT)
        {
            T_USB_DESC_HDR *ep_desc = *desc;
            USB_UTILS_LIST_FOREACH((&interface->eps), T_USB_EP *, ep)
            {
                ep_priv = ep->priv;
                if (ep_priv->fs_desc->bEndpointAddress == ((T_USB_ENDPOINT_DESC *)ep_desc)->bEndpointAddress)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                ep = malloc(sizeof(T_USB_EP));
                if (ep == NULL)
                {
                    fail_line = __LINE__;
                    ret = -ENOMEM;
                    goto end;
                }
                memset(ep, 0, sizeof(T_USB_EP));
                ep_priv = malloc(sizeof(T_EP_PRIV));
                if (ep_priv == NULL)
                {
                    fail_line = __LINE__;
                    free(ep);
                    ret = -ENOMEM;
                    goto end;
                }
                memset(ep_priv, 0, sizeof(T_EP_PRIV));
                ep->priv = ep_priv;
                ep->addr = ((T_USB_ENDPOINT_DESC *)*desc)->bEndpointAddress;
                ep->ep_handle = hal_usb_ep_handle_get(((T_USB_ENDPOINT_DESC *)*desc)->bEndpointAddress);
                USB_UTILS_LIST_INSERT_TAIL(&interface->eps, ep);
            }
            if (speed == HAL_USB_SPEED_FULL)
            {
                ep_priv->fs_desc = (T_USB_ENDPOINT_DESC *)*desc;
            }
            else if (speed == HAL_USB_SPEED_HIGH)
            {
                ep_priv->hs_desc = (T_USB_ENDPOINT_DESC *)*desc;
            }
        }
    }
end:
    USB_PRINT_INFO2("usb_cdc_driver_descs_parse, ret %d, fail line %d", ret, fail_line);
    return ret;
}

static int usb_cdc_driver_if_create(T_USB_INTERFACE *interface)
{
    int ret = ESUCCESS;

    ret = usb_cdc_driver_descs_parse(interface, interface->descs_fs, HAL_USB_SPEED_FULL);
    ret += usb_cdc_driver_descs_parse(interface, interface->descs_hs, HAL_USB_SPEED_HIGH);
    return ret;
}

static int usb_cdc_driver_if_release(T_USB_INTERFACE *interface)
{
    int ret = ESUCCESS;

    T_USB_EP *ep = NULL;
    USB_UTILS_LIST_FOREACH((&interface->eps), T_USB_EP *, ep)
    {
        if (ep->priv)
        {
            free(ep->priv);
            ep->priv = NULL;
        }
        free(ep);
    }
    return ret;
}

static int usb_cdc_driver_if_alt_get(T_USB_INTERFACE *interface, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    T_USB_CDC_IF *cdc = USB_UTILS_CONTAINER_OF(interface, T_USB_CDC_IF, intf);
    memset(ctrl_urb->buf, cdc->cur_alt, 1);
    USB_PRINT_INFO1("usb_cdc_driver_if_alt_get, cur_alt %d", cdc->cur_alt);
    return 1;
}

static int usb_cdc_driver_if_alt_set(T_USB_INTERFACE *interface, T_HAL_USB_REQUEST_BLOCK *ctrl_urb,
                                     uint8_t alt)
{
    T_USB_CDC_IF *cdc = USB_UTILS_CONTAINER_OF(interface, T_USB_CDC_IF, intf);
    int ret = ESUCCESS;
    T_USB_EP *ep_item = NULL;
    uint8_t speed = usb_composite_dev_enum_speed_get();

    USB_UTILS_LIST_FOREACH((&interface->eps), T_USB_EP *, ep_item)
    {
        T_USB_ENDPOINT_DESC *ep_desc = NULL;
        T_EP_PRIV *ep_priv = ep_item->priv;
        if (speed == HAL_USB_SPEED_FULL)
        {
            ep_item->desc = ep_priv->fs_desc;
        }
        else if (speed == HAL_USB_SPEED_HIGH)
        {
            ep_item->desc = ep_priv->hs_desc;
        }
        else
        {
            USB_PRINT_ERROR1("usb_cdc_driver_if_alt_set, unsupported speed %d", speed);
        }
        ep_desc = ep_item->desc;
        if (ep_priv->ep_enable)
        {
            ret += hal_usb_ep_disable(ep_item->ep_handle);
        }
        ret += hal_usb_ep_enable(ep_item->ep_handle, ep_desc);
        if (ret < 0)
        {
            fail_line = __LINE__;
            goto end;
        }
        ep_priv->ep_enable = 1;
        if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_OUT)
        {
            usb_pipe_recv(ep_priv->pipe);
        }
    }
    cdc->cur_alt = alt;
end:
    USB_PRINT_INFO4("usb_cdc_driver_if_alt_set, alt %d, interface %d, ret %d, fail line %d", alt,
                    interface->if_num, ret, fail_line);
    return ret;
}

int usb_cdc_driver_if_suspend(T_USB_INTERFACE *interface)
{
    int ret = ESUCCESS;
    T_USB_EP *ep = NULL;
    T_EP_PRIV *ep_priv = NULL;
    USB_UTILS_LIST_FOREACH((&interface->eps), T_USB_EP *, ep)
    {
        ep_priv = ep->priv;
        ret += hal_usb_ep_disable(ep->ep_handle);
        ep_priv->ep_enable = 0;
    }
    return ret;
}

int usb_cdc_driver_if_resume(T_USB_INTERFACE *interface)
{
    int ret = ESUCCESS;
    T_USB_EP *ep = NULL;
    T_EP_PRIV *ep_priv = NULL;
    USB_UTILS_LIST_FOREACH((&interface->eps), T_USB_EP *, ep)
    {
        ep_priv = ep->priv;
        if (!ep_priv->ep_enable)
        {
            ret += hal_usb_ep_enable(ep->ep_handle, ep->desc);
            ep_priv->ep_enable = 1;
        }
        if ((ep->addr & USB_DIR_MASK) == 0)
        {
            usb_pipe_recv(ep_priv->pipe);
        }
    }
    return ret;
}

T_USB_EP *usb_cdc_driver_ep_find_by_addr(uint8_t addr)
{
    T_USB_CDC_IF *cdc = NULL;
    T_USB_INTERFACE *interface = NULL;
    T_USB_EP *ep;

    USB_UTILS_LIST_FOREACH((&g_usb_cdc->if_list), T_USB_CDC_IF *, cdc)
    {
        interface = &cdc->intf;
        USB_UTILS_LIST_FOREACH((&interface->eps), T_USB_EP *, ep)
        {
            if (ep->addr == addr)
            {
                return ep;
            }
        }
    }
    return NULL;
}

void *usb_cdc_driver_inst_alloc(void)
{
    T_USB_CDC_IF *cdc = (T_USB_CDC_IF *)malloc(sizeof(T_USB_CDC_IF));
    memset(cdc, 0, sizeof(T_USB_CDC_IF));
    return cdc;
}

int usb_cdc_driver_inst_free(void *inst)
{
    free(inst);
    return ESUCCESS;
}

int usb_cdc_driver_if_desc_register(void *inst, void *hs_desc, void *fs_desc)
{
    int ret = ESUCCESS;
    T_USB_CDC_IF *cdc = (T_USB_CDC_IF *)inst;

    cdc->intf.if_num = 0;
    cdc->intf.descs_fs = fs_desc;
    cdc->intf.descs_hs = hs_desc;
    cdc->intf.ctrl_request_proc = usb_cdc_driver_if_ctrl_request_proc;
    cdc->intf.create = usb_cdc_driver_if_create;
    cdc->intf.release = usb_cdc_driver_if_release;
    cdc->intf.alt_get = usb_cdc_driver_if_alt_get;
    cdc->intf.alt_set = usb_cdc_driver_if_alt_set;
    cdc->intf.suspend = usb_cdc_driver_if_suspend;
    cdc->intf.resume = usb_cdc_driver_if_resume;
    USB_UTILS_LIST_INIT((&cdc->intf.eps));

    if (g_usb_cdc == NULL)
    {
        g_usb_cdc = (T_USB_CDC *)malloc(sizeof(T_USB_CDC));
        if (g_usb_cdc == NULL)
        {
            fail_line = __LINE__;
            ret = -ENOMEM;
            goto end;
        }
        memset(g_usb_cdc, 0, sizeof(T_USB_CDC));
        USB_UTILS_LIST_INIT(&g_usb_cdc->if_list);
    }
    USB_UTILS_LIST_INSERT_TAIL(&g_usb_cdc->if_list, cdc);

end:
    USB_PRINT_INFO2("usb_cdc_driver_if_desc_register, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_cdc_driver_if_desc_unregister(void *inst)
{
    T_USB_CDC_IF *cdc = NULL;
    bool found = false;
    int ret = ESUCCESS;

    USB_UTILS_LIST_FOREACH((&g_usb_cdc->if_list), T_USB_CDC_IF *, cdc)
    {
        if (cdc == inst)
        {
            found = true;
            USB_UTILS_LIST_REMOVE(&g_usb_cdc->if_list, cdc);
        }
    }

    if (!found)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
    }

    USB_PRINT_INFO2("usb_cdc_driver_inst_free, result %d, fail line %d", ret, fail_line);
    return ret;
}

void *usb_cdc_driver_data_pipe_open(uint8_t ep_addr, T_USB_CDC_DRIVER_ATTR attr,
                                    uint8_t pending_req_num, USB_CDC_DRIVER_CB cb)
{
    T_USB_PIPE_ATTR pipe_attr;
    T_USB_EP *ep = usb_cdc_driver_ep_find_by_addr(ep_addr);
    void *pipe = NULL;
    if (ep == NULL)
    {
        USB_PRINT_ERROR0("usb_cdc_driver_data_pipe_open, ep NULL");
        return NULL;
    }

    memcpy(&pipe_attr, &attr, sizeof(T_USB_PIPE_ATTR));
    pipe = usb_pipe_open(&(ep->desc), ep_addr, pipe_attr, pending_req_num, cb);
    ((T_EP_PRIV *)ep->priv)->pipe = pipe;
    return pipe;
}

int usb_cdc_driver_data_pipe_close(void *handle)
{
    return usb_pipe_close(handle);
}

int usb_cdc_driver_data_pipe_send(void *handle, void *buf, uint32_t len)
{
    return usb_pipe_send(handle, buf, len);
}

int usb_cdc_driver_init(void)
{
    T_USB_CDC_IF *cdc = NULL;
    T_USB_INTERFACE *interface = NULL;

    USB_UTILS_LIST_FOREACH((&g_usb_cdc->if_list), T_USB_CDC_IF *, cdc)
    {
        interface = &cdc->intf;
        usb_composite_dev_interface_add(interface, 1);
    }

    return ESUCCESS;
}

int usb_cdc_driver_deinit(void)
{
    T_USB_CDC_IF *cdc = NULL;
    T_USB_INTERFACE *interface = NULL;

    USB_UTILS_LIST_FOREACH((&g_usb_cdc->if_list), T_USB_CDC_IF *, cdc)
    {
        interface = &cdc->intf;
        usb_composite_dev_interface_remove(interface, 1);
    }

    return ESUCCESS;
}
