#include <stdlib.h>
#include <string.h>
#include "usb_pipe.h"
#include "trace.h"
#include "os_sync.h"
#include "errno.h"
#include "usb_spec20.h"
#include "usb_utils.h"
#include "usb_composite_dev.h"
#include "hal_usb.h"
#include "usb_task.h"

static int fail_line = 0;

typedef struct _t_usb_pipe_req
{
    struct _t_usb_pipe_req *p_next;
    void *buf;
    uint32_t len;
} T_USB_PIPE_REQ;

typedef struct _t_usb_pipe_tx
{
    void *buf;
    T_USB_PIPE_REQ *cur_req;
    T_USB_UTILS_LIST pending_reqs;
    T_USB_UTILS_LIST idle_reqs;
    T_USB_PIPE_REQ reqs[];
} T_USB_PIPE_TX;

typedef struct _t_usb_pipe_db
{
    struct _t_usb_pipe_db *p_next;
    void *ep_handle;
    T_USB_PIPE_ATTR attr;
    T_HAL_USB_REQUEST_BLOCK *urb;
    T_USB_ENDPOINT_DESC **ep_desc;
    USB_PIPE_CB cb;
    union
    {
        T_USB_PIPE_TX *tx;
        void *rx;
    } priv;
} T_USB_PIPE_DB;

static int usb_pipe_ep_data_send(void *handle, uint8_t *buf, uint32_t len);

T_USB_UTILS_LIST g_pipe_list = {.count = 0};

static T_USB_PIPE_DB *usb_pipe_match(void *drv)
{
    T_USB_PIPE_DB *walker = NULL;

    if (USB_UTILS_LIST_EMPTY(&g_pipe_list) == false)
    {
        USB_UTILS_LIST_FOREACH(&g_pipe_list,  T_USB_PIPE_DB *, walker)
        {
            if (walker->ep_handle == drv)
            {
                return walker;
            }
        }
    }

    return NULL;
}

void *usb_pipe_open(T_USB_ENDPOINT_DESC **desc, uint8_t ep_addr, T_USB_PIPE_ATTR attr,
                    uint8_t pending_req_num, USB_PIPE_CB cb)
{
    T_USB_PIPE_DB *pipe = NULL;
    T_HAL_USB_REQUEST_BLOCK *urb = NULL;
    void *ep_handle = NULL;
    T_USB_PIPE_REQ *req = NULL;

    ep_handle = hal_usb_ep_handle_get(ep_addr);
    pipe = usb_pipe_match(ep_handle);
    if (pipe)
    {
        goto end;
    }

    pipe = (T_USB_PIPE_DB *)malloc(sizeof(T_USB_PIPE_DB));
    if (!pipe)
    {
        fail_line = __LINE__;
        goto end;
    }
    memset(pipe, 0, sizeof(T_USB_PIPE_DB));

    memcpy(&(pipe->attr), &attr, sizeof(T_USB_PIPE_ATTR));
    pipe->ep_handle = ep_handle;
    if (USB_UTILS_LIST_EMPTY(&g_pipe_list))
    {
        USB_UTILS_LIST_INIT(&g_pipe_list);
    }
    USB_UTILS_LIST_INSERT_TAIL(&g_pipe_list, pipe);

    urb = hal_usb_urb_alloc(attr.mtu);
    urb->ep_handle = ep_handle;
    urb->complete_in_isr = attr.high_throughput;
    urb->priv = pipe;
    pipe->urb = urb;
    pipe->ep_desc = desc;
    pipe->cb = cb;

    if ((ep_addr & USB_DIR_MASK) && pending_req_num != 0)
    {
        pipe->priv.tx = malloc(sizeof(T_USB_PIPE_TX) + pending_req_num * sizeof(T_USB_PIPE_REQ));
        if (!pipe->priv.tx)
        {
            fail_line = __LINE__;
            goto end;
        }
        memset(pipe->priv.tx, 0, sizeof(T_USB_PIPE_TX) + pending_req_num * sizeof(T_USB_PIPE_REQ));
        pipe->priv.tx->buf = (uint8_t *)malloc(pending_req_num * pipe->attr.mtu);
        if (!pipe->priv.tx->buf)
        {
            fail_line = __LINE__;
            goto end;
        }
        memset(pipe->priv.tx->buf, 0, pending_req_num * pipe->attr.mtu);

        USB_UTILS_LIST_INIT(&(pipe->priv.tx->pending_reqs));
        USB_UTILS_LIST_INIT(&(pipe->priv.tx->idle_reqs));
        T_USB_PIPE_REQ *reqs = pipe->priv.tx->reqs;
        for (uint8_t i = 0; i < pending_req_num; i++)
        {
            req = reqs + i;
            memset(req, 0, sizeof(T_USB_PIPE_REQ));
            req->buf = (uint8_t *)pipe->priv.tx->buf + i * attr.mtu;
            USB_UTILS_LIST_INSERT_TAIL(&(pipe->priv.tx->idle_reqs), req);
        }
    }

end:
    USB_PRINT_INFO2("usb_pipe_open, pipe %x, fail line %d", pipe, fail_line);
    return pipe;
}

int _usb_pipe_close(T_USB_PIPE_DB *pipe)
{
    T_USB_PIPE_REQ *req = NULL;
    T_USB_ENDPOINT_DESC *ep_desc = NULL;

    uint32_t s = os_lock();
    pipe->ep_handle = NULL;
    if (pipe->urb)
    {
        hal_usb_urb_free(pipe->urb);
        pipe->urb = NULL;
    }
    pipe->cb = NULL;
    memset(&pipe->attr, 0, sizeof(T_USB_PIPE_ATTR));
    ep_desc = *pipe->ep_desc;
    if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN)
    {
        if (pipe->priv.tx->buf)
        {
            free(pipe->priv.tx->buf);
            pipe->priv.tx->buf = NULL;
        }
        USB_UTILS_LIST_FOREACH(&pipe->priv.tx->idle_reqs, T_USB_PIPE_REQ *, req)
        {
            USB_UTILS_LIST_REMOVE(&pipe->priv.tx->idle_reqs, req);
        }
        USB_UTILS_LIST_FOREACH(&pipe->priv.tx->pending_reqs, T_USB_PIPE_REQ *, req)
        {
            USB_UTILS_LIST_REMOVE(&pipe->priv.tx->pending_reqs, req);
        }
        free(pipe->priv.tx);
        pipe->priv.tx = NULL;
    }
    pipe->ep_desc = NULL;
    free(pipe);
    os_unlock(s);

    return ESUCCESS;
}

int usb_pipe_close(void *handle)
{
    T_USB_PIPE_DB *pipe = (T_USB_PIPE_DB *)handle;
    T_USB_PIPE_DB *pipe_tmp = NULL;
    bool found = false;

    if (pipe)
    {
        uint32_t s = os_lock();
        pipe->urb->priv = NULL;
        USB_UTILS_LIST_FOREACH(&g_pipe_list,  T_USB_PIPE_DB *, pipe_tmp)
        {
            if (pipe_tmp == pipe)
            {
                found = true;
                USB_UTILS_LIST_REMOVE(&g_pipe_list, pipe_tmp);
                break;
            }
        }
        os_unlock(s);

        if (!found)
        {
            USB_PRINT_ERROR1("usb_pipe_close fail pipe 0x%x", pipe);
            return -ENXIO;
        }
        usb_task_pending_call((T_USB_TASK_PENDING_CALL)_usb_pipe_close, pipe);
    }

    return ESUCCESS;
}

USB_USER_SPEC_SECTION
static void usb_pipe_pending_req_handle(T_USB_PIPE_DB *pipe)
{
    bool send_now = false;
    T_USB_PIPE_REQ  *req = NULL;
    uint32_t s;

    s = os_lock();
    if (pipe->priv.tx->cur_req == NULL && pipe->priv.tx->pending_reqs.count > 0)
    {
        USB_UTILS_LIST_REMOVE_HEAD(&(pipe->priv.tx->pending_reqs), req);
        pipe->priv.tx->cur_req = req;
        send_now = true;
    }
    os_unlock(s);

    if (send_now)
    {
        usb_pipe_ep_data_send(pipe, pipe->priv.tx->cur_req->buf, pipe->priv.tx->cur_req->len);
    }
}

USB_USER_SPEC_SECTION
static int usb_pipe_in_xfer_done(T_HAL_USB_REQUEST_BLOCK *urb)
{
    T_USB_PIPE_REQ  *req = NULL;
    int ret = ESUCCESS;
    uint32_t s;
    T_USB_PIPE_DB *pipe = urb->priv;
    if (!pipe)
    {
        fail_line = __LINE__;
        ret = -EFAULT;
        goto end;
    }

    if (pipe->cb)
    {
        pipe->cb(pipe, urb->buf, urb->actual, urb->status);
    }
    if (pipe->priv.tx)
    {
        s = os_lock();
        USB_UTILS_LIST_INSERT_TAIL(&(pipe->priv.tx->idle_reqs), pipe->priv.tx->cur_req);
        pipe->priv.tx->cur_req = NULL;

        if (urb->status != ESUCCESS)
        {
            USB_UTILS_LIST_FOREACH(&pipe->priv.tx->pending_reqs, T_USB_PIPE_REQ *, req)
            {
                USB_UTILS_LIST_REMOVE(&pipe->priv.tx->pending_reqs, req);
                USB_UTILS_LIST_INSERT_TAIL(&(pipe->priv.tx->idle_reqs), req);
            }
        }
        os_unlock(s);

        usb_pipe_pending_req_handle(pipe);
    }

end:
    if (ret != ESUCCESS)
    {
        USB_PRINT_INFO5("usb_pipe_in_xfer_done, result %d, fail line %d, length %d, actual 0x%x, urb status %d",
                        ret, fail_line, urb->length, urb->actual, urb->status);
    }
    return ret;
}

USB_USER_SPEC_SECTION
static int usb_pipe_ep_data_send(void *handle, uint8_t *buf, uint32_t len)
{
    T_HAL_USB_REQUEST_BLOCK *urb = NULL;
    T_USB_ENDPOINT_DESC *ep_desc = NULL;
    T_USB_PIPE_DB *pipe = (T_USB_PIPE_DB *)handle;
    int ret = ESUCCESS;
    if (!pipe)
    {
        fail_line = __LINE__;
        ret = -EFAULT;
        goto end;
    }

    urb = pipe->urb;
    ep_desc = *(pipe->ep_desc);
    if (!ep_desc)
    {
        fail_line = __LINE__;
        ret = -EFAULT;
        goto end;
    }
    urb->length = len;
    urb->ep_handle = pipe->ep_handle;
    memcpy(urb->buf, buf, len);
    urb->complete = usb_pipe_in_xfer_done;

    if (pipe->attr.zlp)
    {
        if ((urb->length % ep_desc->wMaxPacketSize) == 0)
        {
            urb->zlp = 1;// send zlp
        }
        else
        {
            urb->zlp = 0;// send zlp
        }
    }
    ret = hal_usb_ep_tx(urb->ep_handle, urb);
    if (ret != ESUCCESS)
    {
        fail_line = __LINE__;
        ret = -EINVAL;
        goto end;
    }

    return ret;
end:
    if (ret != ESUCCESS)
    {
        USB_PRINT_INFO4("usb_pipe_ep_data_send, result %d, fail line %d, buf %x, len %d", ret, fail_line,
                        buf, len);
    }
    return ret;
}

USB_USER_SPEC_SECTION
int usb_pipe_send(void *handle, void *buf, uint32_t len)
{
    int ret = ESUCCESS;
    bool send_now = true;
    T_USB_PIPE_REQ  *req = NULL;
    T_USB_PIPE_DB *pipe = (T_USB_PIPE_DB *)handle;
    uint32_t s;
    uint8_t *buf_to_send = buf;
    uint32_t len_to_send = len;

    if (!pipe)
    {
        fail_line = __LINE__;
        ret = -EFAULT;
        goto end;
    }

    if (len > pipe->attr.mtu)
    {
        fail_line = __LINE__;
        ret = -EFAULT;
        goto end;
    }

    if (pipe->priv.tx)
    {
        s = os_lock();
        USB_UTILS_LIST_REMOVE_HEAD(&(pipe->priv.tx->idle_reqs), req);
        os_unlock(s);
        if (!req)
        {
            if (pipe->attr.congestion_ctrl == USB_PIPE_CONGESTION_CTRL_DROP_FIRST)
            {
                s = os_lock();
                USB_UTILS_LIST_REMOVE_HEAD(&(pipe->priv.tx->pending_reqs), req);
                os_unlock(s);
            }
            else
            {
                fail_line = __LINE__;
                ret = -EFAULT;
                goto end;
            }
        }

        memcpy(req->buf, buf, len);
        req->len = len;
        s = os_lock();
        USB_PRINT_INFO2("usb_pipe_send:0x%x-0x%x", pipe->priv.tx->pending_reqs.count,
                        pipe->priv.tx->cur_req);
        if (pipe->priv.tx->pending_reqs.count > 0 || pipe->priv.tx->cur_req != NULL)
        {
            USB_UTILS_LIST_INSERT_TAIL(&(pipe->priv.tx->pending_reqs), req);
            send_now = false;
        }
        else
        {
            pipe->priv.tx->cur_req = req;
            send_now = true;
        }
        os_unlock(s);
        buf_to_send = pipe->priv.tx->cur_req->buf;
        len_to_send =  pipe->priv.tx->cur_req->len;
    }
    if (send_now)
    {
        ret = usb_pipe_ep_data_send(pipe, buf_to_send, len_to_send);
        if (ret != ESUCCESS)
        {
            if (pipe->cb)
            {
                pipe->cb(pipe, pipe->urb->buf, 0, ret);
            }
            if (pipe->priv.tx)
            {
                s = os_lock();
                USB_UTILS_LIST_INSERT_TAIL(&(pipe->priv.tx->idle_reqs), pipe->priv.tx->cur_req);
                pipe->priv.tx->cur_req = NULL;
                USB_UTILS_LIST_FOREACH(&pipe->priv.tx->pending_reqs, T_USB_PIPE_REQ *, req)
                {
                    USB_UTILS_LIST_REMOVE(&pipe->priv.tx->pending_reqs, req);
                    USB_UTILS_LIST_INSERT_TAIL(&(pipe->priv.tx->idle_reqs), req);
                }
                os_unlock(s);
            }
        }
    }

end:
    if (ret != ESUCCESS)
    {
        USB_PRINT_INFO2("usb_pipe_send, result %d, fail line %d", ret, fail_line);
    }
    return ret;
}

static int usb_pipe_out_cb(T_HAL_USB_REQUEST_BLOCK *urb)
{
    int32_t status = urb->status;
    int ret = ESUCCESS;
    T_USB_PIPE_DB *pipe = NULL;

    pipe = urb->priv;

    if (status == -ESHUTDOWN)
    {
        fail_line = __LINE__;
        ret = -ESHUTDOWN;
        goto end;
    }

    if (pipe->cb)
    {
        pipe->cb(pipe, urb->buf, urb->actual, urb->status);
    }

    if (!hal_usb_ep_stall_status_get(urb->ep_handle))
    {
        ret = hal_usb_ep_rx(urb->ep_handle, urb);
    }

end:
    USB_PRINT_INFO5("usb_pipe_out_cb, result %d, fail line %d, status %d, buf 0x%x, actual 0x%x",
                    ret, fail_line, urb->status, urb->buf, urb->actual);
    return ret;
}

int usb_pipe_recv(void *handle)
{
    int ret = ESUCCESS;
    T_USB_PIPE_DB *pipe = (T_USB_PIPE_DB *)handle;
    T_HAL_USB_REQUEST_BLOCK *urb = NULL;

    if (!pipe)
    {
        fail_line = __LINE__;
        ret = -EFAULT;
        goto end;
    }

    urb = pipe->urb;
    urb->complete = usb_pipe_out_cb;
    urb->ep_handle = pipe->ep_handle;
    urb->length = pipe->attr.mtu;
    hal_usb_ep_rx(urb->ep_handle, urb);

end:
    USB_PRINT_INFO2("usb_pipe_recv, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_pipe_stall_set(void *handle, int err)
{
    void *ep_handle = NULL;
    T_USB_PIPE_DB *pipe = (T_USB_PIPE_DB *)handle;
    int ret = ESUCCESS;

    if (!pipe)
    {
        fail_line = __LINE__;
        ret = -EFAULT;
        goto end;
    }

    ep_handle = pipe->ep_handle;
    ret = hal_usb_ep_stall_set(ep_handle, err);
end:
    USB_PRINT_INFO2("usb_pipe_stall_set, result %d, fail line %d", ret, fail_line);
    return ret;
}

int usb_pipe_stall_clear(void *handle)
{
    T_USB_PIPE_DB *pipe = (T_USB_PIPE_DB *)handle;
    void *ep_handle = pipe->ep_handle;
    bool stall = hal_usb_ep_stall_status_get(pipe->urb->ep_handle);
    T_USB_ENDPOINT_DESC *ep_desc = *(pipe->ep_desc);

    hal_usb_ep_stall_clear(ep_handle);
    if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_OUT)
    {
        if (stall)
        {
            usb_pipe_recv(handle);
        }
    }
    return ESUCCESS;
}

T_USB_PIPE_ATTR *usb_pipe_attr_get(void *handle)
{
    T_USB_PIPE_DB *pipe = (T_USB_PIPE_DB *)handle;

    return &(pipe->attr);
}
