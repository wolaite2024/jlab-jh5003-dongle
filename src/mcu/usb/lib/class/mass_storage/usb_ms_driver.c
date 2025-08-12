#include <stdlib.h>
#include <string.h>
#include "errno.h"
#include "trace.h"
#include "usb_spec20.h"
#include "usb_pipe.h"
#include "usb_composite_dev.h"
#include "usb_msc_bot.h"
#include "usb_ms_driver.h"
#include "os_sched.h"

#define USB_MS_DRIVER_IN_PIPE_XFER_MAX          (10)
#define USB_MS_DRIVER_PIPE_XFER_SIZE            (512)

static int fail_line = 0;

typedef enum {T_CBS_SCSI, T_CBS_MAX} T_CBS;

typedef enum _usb_ms_status
{
    USB_MS_STATUS_READY,
    USB_MS_STATUS_CBW_PHASE,
    USB_MS_STATUS_DATA_OUT_PHASE,
    USB_MS_STATUS_DATA_IN_PHASE,
    USB_MS_STATUS_CSW_PHASE,
    USB_MS_STATUS_CBW_FAIL,
    USB_MS_STATUS_PHASE_ERROR
} T_USB_MS_STATUS;

typedef struct _ep_priv
{
    T_USB_ENDPOINT_DESC *fs_desc;
    T_USB_ENDPOINT_DESC *hs_desc;
    bool enable;
} T_EP_PRIV;

typedef struct _usb_ms
{
    T_USB_INTERFACE intf;
    uint8_t cur_alt;
    void *pipe_tx;
    void *pipe_rx;
    uint32_t mtu_tx;
    uint32_t mtu_rx;
    T_RX_CB rx_cbs;
    uint32_t xfer_len;
    uint32_t remaining_len;
    USB_MS_DRIVER_TX_COMPLETE tx_complete;
    T_DISK_DRIVER disk;
    int cur_cbw_tag;
    T_USB_MS_STATUS ms_status;

} T_USB_MS;

T_USB_MS g_usb_ms;

static bool usb_ms_driver_cbw_valid(T_USB_MS *usb_ms, T_CBW *cbw, uint32_t actual_len)
{
    T_USB_MS_STATUS ms_status = usb_ms->ms_status;
    if (ms_status == USB_MS_STATUS_READY && actual_len == 0x1F && cbw->dCBWSignature == 0x43425355)
    {
        return true;
    }

    return false;
}

static bool usb_ms_driver_cbw_meaningful(T_USB_MS *usb_ms, T_CBW *cbw)
{
    return true;
}

static uint8_t usb_ms_driver_max_lun_get(void)
{
    return 0;
}

static int usb_ms_driver_csw_send(uint32_t tag, uint32_t data_residue, uint8_t status)
{
    T_CSW csw =
    {
        .dCSWSignature = 0x53425355,
        .dCSWTag = tag,
        .dCSWDataResidue = data_residue,
        .bCSWStatus = status
    };
    return usb_ms_driver_data_pipe_send(&csw, sizeof(T_CSW), NULL);
}

static int usb_ms_driver_zlp_send(void)
{
    uint8_t zlp = 0;
    return usb_ms_driver_data_pipe_send(&zlp, 0, NULL);
}

static int usb_ms_driver_data_pipe_stall_set(void *pipe)
{
    return usb_pipe_stall_set(pipe, -ENOTSUPP);
}

static int usb_ms_driver_data_pipe_stall_clear(void *pipe)
{
    int ret = ESUCCESS;
    ret = usb_pipe_stall_clear(pipe);

    if (ret == ESUCCESS && pipe == g_usb_ms.pipe_tx)
    {
        if (g_usb_ms.ms_status == USB_MS_STATUS_CBW_FAIL)
        {
            ret = usb_ms_driver_csw_send(g_usb_ms.cur_cbw_tag, g_usb_ms.remaining_len, CB_STATUS_FAILED);
            g_usb_ms.ms_status = USB_MS_STATUS_READY;
        }
        else if (g_usb_ms.ms_status == USB_MS_STATUS_PHASE_ERROR)
        {
            ret = usb_ms_driver_csw_send(g_usb_ms.cur_cbw_tag, g_usb_ms.remaining_len, CB_STATUS_PHASE_ERROR);
            g_usb_ms.ms_status = USB_MS_STATUS_READY;
        }
    }

    return ret;
}

static int usb_ms_driver_bo_reset(T_USB_MS *usb_ms)
{
    usb_ms->ms_status = USB_MS_STATUS_READY;
    return 0;
}

static void usb_ms_driver_cbw_error_handle(T_USB_MS *usb_ms, uint8_t err)
{
    if (err == CB_STATUS_FAILED)
    {
        if (usb_ms->ms_status == USB_MS_STATUS_DATA_IN_PHASE)
        {
            usb_ms_driver_zlp_send();
            os_delay(2);
        }
        usb_ms_driver_data_pipe_stall_set(usb_ms->pipe_tx);

        usb_ms->ms_status = USB_MS_STATUS_CBW_FAIL;
    }
    else
    {
        usb_ms_driver_data_pipe_stall_set(usb_ms->pipe_tx);
        usb_ms_driver_data_pipe_stall_set(usb_ms->pipe_rx);
        usb_ms->ms_status = USB_MS_STATUS_PHASE_ERROR;
    }


    return;
}

static int usb_ms_driver_out_pipe_complete(void *handle, void *buf, uint32_t len, int status)
{
    T_USB_MS_STATUS ms_status = g_usb_ms.ms_status;
    int ret = ESUCCESS;

    USB_PRINT_INFO3("usb_ms_driver_out_pipe_complete, ms_status %d, len %d, remaining_len %d",
                    ms_status, len, g_usb_ms.remaining_len);

    if (ms_status == USB_MS_STATUS_DATA_OUT_PHASE)
    {
        if (g_usb_ms.rx_cbs.data)
        {
            g_usb_ms.rx_cbs.data(&g_usb_ms.disk, buf, len);
        }

        if (g_usb_ms.remaining_len != 0xFF)
        {
            g_usb_ms.remaining_len -= len;
        }
        if (g_usb_ms.remaining_len == 0 || g_usb_ms.remaining_len == 0xFF || len < g_usb_ms.mtu_rx)
        {
            g_usb_ms.ms_status = USB_MS_STATUS_CSW_PHASE;
            usb_ms_driver_csw_send(g_usb_ms.cur_cbw_tag, 0, CB_STATUS_OK);
        }
    }
    else if (ms_status == USB_MS_STATUS_READY)
    {
        if (g_usb_ms.rx_cbs.cmd)
        {
            T_CBW *cbw = (T_CBW *)buf;
            if (usb_ms_driver_cbw_valid(&g_usb_ms, cbw, len) && usb_ms_driver_cbw_meaningful(&g_usb_ms, cbw))
            {
                g_usb_ms.ms_status = USB_MS_STATUS_CBW_PHASE;
                g_usb_ms.remaining_len = cbw->dCBWDataTransferLength;
                g_usb_ms.cur_cbw_tag = cbw->dCBWTag;
                if (g_usb_ms.remaining_len)
                {
                    if ((cbw->bmCBWFlags & USB_DIR_MASK) == USB_DIR_OUT)
                    {
                        g_usb_ms.ms_status = USB_MS_STATUS_DATA_OUT_PHASE;
                    }
                    else
                    {
                        g_usb_ms.ms_status = USB_MS_STATUS_DATA_IN_PHASE;
                    }
                }

                ret = g_usb_ms.rx_cbs.cmd(&g_usb_ms.disk, buf);
                if (ret == -ENOTSUPP)
                {
                    USB_PRINT_INFO0("usb_ms_driver_out_pipe_complete, COMMAND NOT SUPPORT");
                    usb_ms_driver_cbw_error_handle(&g_usb_ms, CB_STATUS_FAILED);
                }
                else if (ret != ESUCCESS)
                {
                    usb_ms_driver_cbw_error_handle(&g_usb_ms, CB_STATUS_PHASE_ERROR);
                }
                else
                {
                    if (!g_usb_ms.remaining_len)
                    {
                        uint8_t status = CB_STATUS_OK;
                        g_usb_ms.ms_status = USB_MS_STATUS_CSW_PHASE;
                        usb_ms_driver_csw_send(g_usb_ms.cur_cbw_tag, 0, status);
                    }
                }
            }
            else
            {
                usb_ms_driver_cbw_error_handle(&g_usb_ms, CB_STATUS_PHASE_ERROR);
            }

        }
        else
        {
            usb_ms_driver_cbw_error_handle(&g_usb_ms, CB_STATUS_PHASE_ERROR);
        }
    }

    return 0;
}

static int usb_ms_driver_in_pipe_complete(void *handle, void *buf, uint32_t len, int status)
{
    int ret = 0;
    static uint32_t xfer_len = 0;
    USB_PRINT_INFO5("usb_ms_driver_in_pipe_complete, ms_status %d, remaining_len %d, len %d, g_usb_ms.xfer_len %d, xfer_len %d",
                    g_usb_ms.ms_status, g_usb_ms.remaining_len, len, g_usb_ms.xfer_len, xfer_len);
    if (g_usb_ms.ms_status == USB_MS_STATUS_DATA_IN_PHASE)
    {
        xfer_len += len;

        if ((g_usb_ms.xfer_len == xfer_len) && g_usb_ms.tx_complete)
        {
            g_usb_ms.tx_complete(&g_usb_ms.disk, xfer_len, status);
            xfer_len = 0;
        }

        if (g_usb_ms.remaining_len != 0xFF)
        {
            g_usb_ms.remaining_len -= len;
        }

        if (g_usb_ms.remaining_len == 0 || g_usb_ms.remaining_len == 0xFF || len < g_usb_ms.mtu_tx)
        {
            xfer_len = 0;
            g_usb_ms.ms_status = USB_MS_STATUS_CSW_PHASE;
            ret = usb_ms_driver_csw_send(g_usb_ms.cur_cbw_tag, 0, CB_STATUS_OK);
        }
    }
    else if (g_usb_ms.ms_status == USB_MS_STATUS_CSW_PHASE)
    {
        xfer_len = 0;
        g_usb_ms.ms_status = USB_MS_STATUS_READY;
    }
    else
    {
//        ret = usb_ms_driver_data_pipe_stall_set(g_usb_ms.pipe_tx);
    }

    return ret;
}

static int usb_ms_driver_ep_desc_proc(T_USB_MS *usb_ms, T_USB_ENDPOINT_DESC *desc, uint8_t speed)
{
    uint8_t ep_addr = desc->bEndpointAddress;
    T_USB_EP *usb_ep = NULL;
    T_EP_PRIV *ep_priv = NULL;
    T_USB_INTERFACE *intf = &usb_ms->intf;
    bool found = false;
    int ret = ESUCCESS;

    USB_UTILS_LIST_FOREACH(&intf->eps, T_USB_EP *, usb_ep)
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

        ep_priv = malloc(sizeof(T_EP_PRIV));
        if (ep_priv == NULL)
        {
            fail_line = __LINE__;
            ret = -ENOMEM;
            free(usb_ep);
            goto end;
        }
        memset(ep_priv, 0, sizeof(T_EP_PRIV));
        usb_ep->priv = ep_priv;
        USB_UTILS_LIST_INSERT_TAIL(&intf->eps, usb_ep);
    }
    else
    {
        ep_priv = usb_ep->priv;
    }

    if (speed == HAL_USB_SPEED_FULL)
    {
        ep_priv->fs_desc = desc;
    }
    else
    {
        ep_priv->hs_desc = desc;
    }

end:
    USB_PRINT_INFO2("usb_ms_driver_ep_desc_proc, result %d, fail line %d", ret, fail_line);
    return ret;
}

static int usb_ms_driver_descs_parse(T_USB_MS *usb_ms, T_USB_DESC_HDR **if_descs, uint8_t speed)
{
    T_USB_DESC_HDR **descs = if_descs;
    T_USB_INTERFACE *intf = NULL;
    int ret = ESUCCESS;

    if (if_descs == NULL || usb_ms == NULL)
    {
        fail_line = __LINE__;
        ret = -ENXIO;
        goto end;
    }

    intf = &usb_ms->intf;

    for (; *descs; descs++)
    {
        T_USB_DESC_HDR *cur_desc = *descs;
        uint8_t desc_type = cur_desc->bDescriptorType;
        if (desc_type == USB_DESC_TYPE_INTERFACE)
        {
            ((T_USB_INTERFACE_DESC *)cur_desc)->bInterfaceNumber = intf->if_num;
        }
        else if (desc_type == USB_DESC_TYPE_ENDPOINT)
        {
            ret = usb_ms_driver_ep_desc_proc(usb_ms, (T_USB_ENDPOINT_DESC *)cur_desc, speed);
        }
    }

end:
    USB_PRINT_INFO2("usb_ms_driver_descs_parse, result %d, fail line %d", ret, fail_line);
    return ret;
}

static int usb_ms_driver_if_num_change(uint8_t actual_if_num, T_USB_DESC_HDR **if_descs,
                                       uint8_t speed)
{
    T_USB_DESC_HDR **descs = if_descs;
    int ret = ESUCCESS;

    if (if_descs == NULL)
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
    }

end:
    USB_PRINT_INFO2("usb_ms_driver_change_if_num, result %d, fail line %d", ret, fail_line);
    return ret;
}

static int usb_ms_driver_if_create(T_USB_INTERFACE *interface)
{
    uint8_t actual_if_num = interface->if_num;
    int ret = ESUCCESS;

    ret = usb_ms_driver_if_num_change(actual_if_num, interface->descs_fs, HAL_USB_SPEED_FULL);
    ret += usb_ms_driver_if_num_change(actual_if_num, interface->descs_hs, HAL_USB_SPEED_HIGH);

    return ret;
}

static int usb_ms_driver_if_ctrl_request_proc(T_USB_INTERFACE *interface,
                                              T_USB_DEVICE_REQUEST *ctrl_request,
                                              T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    T_USB_MS *usb_ms = USB_UTILS_CONTAINER_OF(interface, T_USB_MS, intf);
    T_HAL_USB_REQUEST_BLOCK      *urb = ctrl_urb;
    uint16_t                    w_length = (ctrl_request->wLength);
    uint8_t                     b_request = (ctrl_request->bRequest);
    uint8_t                     bm_request_type = (ctrl_request->bmRequestType);
    uint16_t w_index = (ctrl_request->wIndex);
    uint16_t w_value = (ctrl_request->wValue);
    int ret = 0;
    USB_PRINT_INFO3("usb_ms_driver_if_ctrl_request_proc, b_request 0x%x, bm_request_type 0x%x, w_length %d",
                    b_request, bm_request_type, w_length);
    switch (b_request | (bm_request_type << 8))
    {
    case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8 | USB_MS_REQ_GET_MAX_LUN):
        {
            uint8_t max_lun = usb_ms_driver_max_lun_get();
            if (w_length == 1 && w_value == 0)  // wlength = 1 & wvalue = 0 is valid
            {
                memcpy(urb->buf, &max_lun, 1);
                ret = 1;
            }
            else
            {
                goto stall;
            }
        }
        break;

    case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8 | USB_MS_REQ_BO_RESET):
        {
            if (w_length == 0 && w_value == 0)
            {
                usb_ms_driver_bo_reset(usb_ms);
            }
            else
            {
                goto stall;
            }
        }
        break;

    case ((USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_ENDPOINT) << 8 | USB_REQ_CODE_CLEAR_FEATURE):
        {
            uint8_t addr = UGETW(w_index);
            uint8_t dir = addr & USB_DIR_MASK;

            if (dir == USB_DIR_IN)
            {
                usb_ms_driver_data_pipe_stall_clear(usb_ms->pipe_tx);
            }
            else if (dir == USB_DIR_OUT)
            {
                usb_ms_driver_data_pipe_stall_clear(usb_ms->pipe_rx);
            }
        }
        goto done;

    case ((USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_ENDPOINT) << 8 | USB_REQ_CODE_SET_FEATURE):
        {
            uint8_t addr = UGETW(w_index);
            uint8_t dir = addr & USB_DIR_MASK;

            if (dir == USB_DIR_IN)
            {
                usb_ms_driver_data_pipe_stall_set(usb_ms->pipe_tx);
            }
            else if (dir == USB_DIR_OUT)
            {
                usb_ms_driver_data_pipe_stall_set(usb_ms->pipe_rx);
            }
            else
            {
            }
        }
        goto done;
    default:
        goto stall;
    }

    if (ret >= 0)
    {
        urb->length = USB_UTILS_MIN(ret, w_length);
        ret = hal_usb_ep0_trx(urb->ep_handle, urb);
        if (ret < 0)
        {
            USB_PRINT_ERROR0("usb_ms_driver_if_ctrl_request_proc, hal_usb_ep0_trx error");
        }
    }
done:
    return ret;

stall:
    return -EOPNOTSUPP;
}

static int usb_ms_driver_if_alt_get(T_USB_INTERFACE *interface, T_HAL_USB_REQUEST_BLOCK *ctrl_urb)
{
    T_USB_MS *usb_ms = USB_UTILS_CONTAINER_OF(interface, T_USB_MS, intf);
    memset(ctrl_urb->buf, usb_ms->cur_alt, 1);
    USB_PRINT_INFO1("usb_ms_driver_if_alt_get, cur_alt %d", usb_ms->cur_alt);
    return 1;
}

static int usb_ms_driver_if_alt_set(T_USB_INTERFACE *interface, T_HAL_USB_REQUEST_BLOCK *ctrl_urb,
                                    uint8_t alt)
{
    T_USB_MS *usb_ms = USB_UTILS_CONTAINER_OF(interface, T_USB_MS, intf);
    T_USB_EP *ep_item = NULL;
    uint8_t speed = usb_composite_dev_enum_speed_get();
    int ret = ESUCCESS;

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
            USB_PRINT_ERROR1("usb_ms_driver_if_alt_set, unsupported speed %d", speed);
        }
        ep_desc = ep_item->desc;
        if (ep_priv->enable)
        {
            ret += hal_usb_ep_disable(ep_item->ep_handle);
        }
        ret += hal_usb_ep_enable(ep_item->ep_handle, ep_desc);
        if (ret < 0)
        {
            fail_line = __LINE__;
            goto end;
        }

        T_USB_PIPE_ATTR attr =
        {
            .zlp = 0,
            .high_throughput = 0,
            .congestion_ctrl = USB_PIPE_CONGESTION_CTRL_DROP_CUR,
            .rsv = 0,
            .mtu = USB_MS_DRIVER_PIPE_XFER_SIZE,
        };
        if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == 0)
        {
            if (!usb_ms->pipe_rx)
            {
                usb_ms->mtu_rx = USB_MS_DRIVER_PIPE_XFER_SIZE;
                usb_ms->pipe_rx = usb_pipe_open(&(ep_item->desc), ep_desc->bEndpointAddress, attr, 1,
                                                (USB_PIPE_CB)usb_ms_driver_out_pipe_complete);
            }
            usb_pipe_recv(usb_ms->pipe_rx);
        }
        else
        {
            if (!usb_ms->pipe_tx)
            {
                usb_ms->mtu_tx = USB_MS_DRIVER_PIPE_XFER_SIZE;
                usb_ms->pipe_tx = usb_pipe_open(&(ep_item->desc), ep_desc->bEndpointAddress, attr,
                                                USB_MS_DRIVER_IN_PIPE_XFER_MAX, (USB_PIPE_CB)usb_ms_driver_in_pipe_complete);
            }
        }
        ep_priv->enable = true;
    }
    usb_ms->cur_alt = alt;
end:
    USB_PRINT_INFO4("usb_ms_driver_if_alt_set, alt %d, interface %d, ret %d, fail line %d", alt,
                    interface->if_num, ret, fail_line);
    return ret;
}

int usb_ms_driver_if_suspend(T_USB_INTERFACE *interface)
{
    int ret = ESUCCESS;
    T_USB_EP *ep = NULL;
    T_EP_PRIV *ep_priv = NULL;
    USB_UTILS_LIST_FOREACH((&interface->eps), T_USB_EP *, ep)
    {
        ep_priv = ep->priv;
        ret += hal_usb_ep_disable(ep->ep_handle);
        ep_priv->enable = false;
    }
    return ret;
}

int usb_ms_driver_if_resume(T_USB_INTERFACE *interface)
{
    int ret = ESUCCESS;
    T_USB_EP *ep = NULL;
    T_EP_PRIV *ep_priv = NULL;
    T_USB_MS *usb_ms = USB_UTILS_CONTAINER_OF(interface, T_USB_MS, intf);
    USB_UTILS_LIST_FOREACH((&interface->eps), T_USB_EP *, ep)
    {
        ep_priv = ep->priv;
        if (!ep_priv->enable)
        {
            ret += hal_usb_ep_enable(ep->ep_handle, ep->desc);
            ep_priv->enable = true;
            if ((ep->addr & USB_DIR_MASK) == 0)
            {
                usb_pipe_recv(usb_ms->pipe_rx);
            }
        }
    }
    return ret;
}

int usb_ms_driver_if_desc_register(T_USB_DESC_HDR **descs_fs,  T_USB_DESC_HDR **descs_hs)
{
    int ret = 0;

    g_usb_ms.intf.if_num = 0;
    g_usb_ms.intf.descs_fs = (struct usb_descriptor_header **)descs_fs;
    g_usb_ms.intf.descs_hs = (struct usb_descriptor_header **)descs_hs;
    g_usb_ms.intf.create = usb_ms_driver_if_create;
    g_usb_ms.intf.release = NULL;
    g_usb_ms.intf.ctrl_request_proc = usb_ms_driver_if_ctrl_request_proc;
    g_usb_ms.intf.alt_get = usb_ms_driver_if_alt_get;
    g_usb_ms.intf.alt_set = usb_ms_driver_if_alt_set;
    g_usb_ms.intf.suspend = usb_ms_driver_if_suspend;
    g_usb_ms.intf.resume = usb_ms_driver_if_resume;
    USB_UTILS_LIST_INIT(&g_usb_ms.intf.eps);
    if (descs_fs)
    {
        ret += usb_ms_driver_descs_parse(&g_usb_ms, descs_fs, HAL_USB_SPEED_FULL);
    }
    if (descs_hs)
    {
        ret += usb_ms_driver_descs_parse(&g_usb_ms, descs_hs, HAL_USB_SPEED_HIGH);
    }
    return ESUCCESS;
}

int usb_ms_driver_disk_register(T_DISK_DRIVER *disk)
{
    memcpy(&g_usb_ms.disk, disk, sizeof(T_DISK_DRIVER));

    return ESUCCESS;
}

int usb_ms_driver_data_pipe_send(void *data, uint32_t len, USB_MS_DRIVER_TX_COMPLETE complete)
{
    void *pipe_tx = g_usb_ms.pipe_tx;
    uint16_t mtu = g_usb_ms.mtu_tx;
    uint8_t xfer_num = len / mtu + ((len % mtu) ? 1 : 0);
    uint32_t offset = 0;
    uint32_t remaining = len;
    uint32_t xfer_len = 0;

    g_usb_ms.tx_complete = complete;
    g_usb_ms.xfer_len = len;
    for (uint8_t i = 0; i < xfer_num; i++)
    {
        if (remaining >= mtu)
        {
            xfer_len = mtu;
        }
        else
        {
            xfer_len = remaining;
        }
        remaining -= xfer_len;
        usb_pipe_send(pipe_tx, ((uint8_t *)data) + offset, xfer_len);
        offset += xfer_len;
    }
    return 0;
}

int usb_ms_driver_data_pipe_ind_cbs_register(T_RX_CB *cbs)
{
    memcpy(&g_usb_ms.rx_cbs, cbs, sizeof(T_RX_CB));
    return ESUCCESS;
}

int usb_ms_driver_init(void)
{
    usb_composite_dev_interface_add(&g_usb_ms.intf, 1);
    return ESUCCESS;
}

