#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
#include <bt_types.h>
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "os_queue.h"
#include "rtl876x.h"
#include "app_usb_hid.h"
#include "app_usb_passthrough.h"
#include "app_timer.h"
#include "app_spp_cmd.h"


#define USB_PT_REPACK_BY_DONGLE 1

#define RAM_TYPE_USB_PT         APP_DONGLE_RAM_TYPE
/* passthrough package len max not include report id */
#define PT_PKG_LEN_MAX              (1024)

/* usb hid resend pkg len max, include report id */
#define USB_PT_PKG_LEN_MAX         (63)
#define USB_PT_PAYLOAD_LEN_MAX      (58)

#define USB_PT_SEND_RETRY_MAX       (10)

#define USB_PT_SEND_TIMEOUT         (1000)    //100ms

#define USB_PT_SEND_QUEUE_LEN_MAX   (10)
#define PT_HEADER_SIZE      (5)


/* pt_send_timer used for wait ack, when timeout resend */
static uint8_t timer_idx_usb_pt_send = 0;
static uint8_t usb_pt_timer_id = 0;

typedef enum
{
    USB_PT_TIMER_ID_SEND = 0x01,
} T_USB_PT_TIMER_ID;

typedef struct __attribute__((packed))
{
    uint8_t report_id;
    uint16_t total_len;
    uint16_t pkg_idx;
    uint8_t payload[USB_PT_PAYLOAD_LEN_MAX];
} usb_hid_pt_pkg_t;

typedef struct __attribute__((packed))
{
    struct usb_pkg_queue_t *p_next;
    usb_hid_pt_pkg_t usb_pt_pkg;
} usb_pkg_queue_t;

typedef struct
{
    uint16_t total_len;
    uint16_t cur_pkg_idx;
    uint16_t offset;
    uint8_t *buf;
} usb_pt_recv_t;

typedef struct
{
    T_OS_QUEUE
    queue;         /* send queue, all send pkg fill in this queue, not include ack */
    usb_pkg_queue_t   *sending_pkg;        /* current sending pkg which waiting ack */
    //uint8_t             waiting_ack;
    uint8_t             retry_count;        /* sending_pkg resend time, when can not get ack pkg */
} usb_pt_send_t;

usb_pt_send_t g_usb_pt_send;

P_PT_SENDOUT_CB func_usb_pt_send_out;

/**/
usb_pt_recv_t g_usb_pt_recv;

static void usb_pt_send_timer_start(void);

static void app_usb_pt_try_send(void)
{
    APP_PRINT_INFO1("app_usb_pt_try_send, before g_usb_pt_send.queue.count %d",
                    g_usb_pt_send.queue.count);
    if (g_usb_pt_send.sending_pkg != NULL)
    {
        APP_PRINT_INFO0("app_usb_pt_try_send, send pkg not NULL!");
        return;
    }
    if (!g_usb_pt_send.queue.count)
    {
        return;
    }

    usb_pkg_queue_t *p_pkg = os_queue_out(&(g_usb_pt_send.queue));
    if (!p_pkg)
    {
        return;
    }

    APP_PRINT_INFO1("app_usb_pt_try_send, after g_usb_pt_send.queue.count %d",
                    g_usb_pt_send.queue.count);

    usb_hid_pt_pkg_t usb_pt;
    memcpy(&usb_pt, &(p_pkg->usb_pt_pkg), sizeof(usb_hid_pt_pkg_t));

    /* get real pkg_len */
    uint8_t pkg_len = USB_PT_PAYLOAD_LEN_MAX + PT_HEADER_SIZE;
    APP_PRINT_INFO2("app_usb_pt_try_send, p_pt->pkg_idx %d, p_pt->total_len %d", usb_pt.pkg_idx,
                    usb_pt.total_len);
    if ((usb_pt.pkg_idx + 1) * USB_PT_PAYLOAD_LEN_MAX > usb_pt.total_len)
    {
        pkg_len = usb_pt.total_len - usb_pt.pkg_idx * USB_PT_PAYLOAD_LEN_MAX + PT_HEADER_SIZE;
    }
    APP_PRINT_INFO1("app_usb_pt_try_send, pkt_len %d", pkg_len);

    g_usb_pt_send.sending_pkg = p_pkg;
    usb_pt_send_timer_start();
    app_usb_hid_interrupt_in((uint8_t *)&usb_pt, USB_PT_PKG_LEN_MAX);
}

static void hid_pt_ack_process(uint8_t *data, uint8_t length)
{
    usb_hid_pt_pkg_t *p_pt = (usb_hid_pt_pkg_t *)data;
    if (p_pt->pkg_idx == g_usb_pt_send.sending_pkg->usb_pt_pkg.pkg_idx)
    {
        if (g_usb_pt_send.sending_pkg)
        {
            free(g_usb_pt_send.sending_pkg);
            g_usb_pt_send.sending_pkg = NULL;
        }
        app_stop_timer(&timer_idx_usb_pt_send);
        app_usb_pt_try_send();
    }
}

static void usb_pt_recv_reset(void)
{
    if (g_usb_pt_recv.buf)
    {
        free(g_usb_pt_recv.buf);
    }
    g_usb_pt_recv.buf = NULL;
    g_usb_pt_recv.total_len = 0;
    g_usb_pt_recv.cur_pkg_idx = 0;
    g_usb_pt_recv.offset = 0;
}

static void hid_pt_send_ack(uint8_t *data, uint16_t length)
{
    usb_hid_pt_pkg_t *p_pt = (usb_hid_pt_pkg_t *)data;

    usb_hid_pt_pkg_t pt_ack;
    pt_ack.report_id = REPORT_ID_PASSTHROUGH_CMD_INPUT;
    pt_ack.total_len = 0;
    pt_ack.pkg_idx = p_pt->pkg_idx;
    memset(pt_ack.payload, 0, USB_PT_PAYLOAD_LEN_MAX);
    app_usb_hid_interrupt_in((uint8_t *)&pt_ack, USB_PT_PKG_LEN_MAX);
}


void app_usb_hid_handle_passthrough(uint8_t *data, uint8_t length)
{
	APP_PRINT_INFO2("----> data %b length %x", TRACE_BINARY(length, data), length);
	
    if ((!data) || (!length))
    {
        return;
    }

    usb_hid_pt_pkg_t *p_pt = (usb_hid_pt_pkg_t *)data;
    if (p_pt->total_len == 0) //ack
    {
        hid_pt_ack_process(data, length);
        return;
    }

    APP_PRINT_INFO2("app_usb_hid_handle_passthrough data %b length %x", TRACE_BINARY(5, data), length);
#if USB_PT_REPACK_BY_DONGLE
    /* new pkg */
    if (p_pt->pkg_idx == 0)
    {
        usb_pt_recv_reset();
        if (p_pt->total_len <= USB_PT_PAYLOAD_LEN_MAX)
        {
            if (func_usb_pt_send_out)
            {
                func_usb_pt_send_out(p_pt->payload, p_pt->total_len);
            }
            hid_pt_send_ack(data, length);
        }
        else
        {
            g_usb_pt_recv.buf = calloc(1, p_pt->total_len);
            if (g_usb_pt_recv.buf == NULL)
            {
                APP_PRINT_ERROR1("app_usb_hid_handle_passthrough zalloc failed len %d", p_pt->total_len);
                hid_pt_send_ack(data, length);
                return;
            }
            memcpy(g_usb_pt_recv.buf + g_usb_pt_recv.offset, p_pt->payload, length - PT_HEADER_SIZE);
            g_usb_pt_recv.offset = length - PT_HEADER_SIZE;
            g_usb_pt_recv.total_len = p_pt->total_len;
            g_usb_pt_recv.cur_pkg_idx = p_pt->pkg_idx;
            hid_pt_send_ack(data, length);
        }
    }
    else
    {
        if ((g_usb_pt_recv.total_len != p_pt->total_len)
            || (g_usb_pt_recv.cur_pkg_idx != p_pt->pkg_idx - 1))
        {
            APP_PRINT_ERROR4("app_usb_hid_handle_passthrough, total_len %x, pkg_total_len %x, cur_pkg_idx %x, pkg_idx %x",
                             g_usb_pt_recv.total_len, p_pt->total_len, g_usb_pt_recv.cur_pkg_idx, p_pt->pkg_idx);
            hid_pt_send_ack(data, length);
            usb_pt_recv_reset();
            return;
        }
        APP_PRINT_INFO1("app_usb_hid_handle_passthrough, payload len: %d", MIN(USB_PT_PAYLOAD_LEN_MAX,
                                                                               (g_usb_pt_recv.total_len - g_usb_pt_recv.offset)));
        memcpy(g_usb_pt_recv.buf + g_usb_pt_recv.offset, p_pt->payload, MIN(USB_PT_PAYLOAD_LEN_MAX,
                                                                            (g_usb_pt_recv.total_len - g_usb_pt_recv.offset)));
        g_usb_pt_recv.offset += MIN(USB_PT_PAYLOAD_LEN_MAX,
                                    (g_usb_pt_recv.total_len - g_usb_pt_recv.offset));
        g_usb_pt_recv.cur_pkg_idx = p_pt->pkg_idx;
        /*FIX ME: what to do if total len is small than get data*/
        if (p_pt->total_len <= g_usb_pt_recv.offset)
        {
            if (func_usb_pt_send_out)
            {
                func_usb_pt_send_out(g_usb_pt_recv.buf, p_pt->total_len);
            }
            usb_pt_recv_reset();
        }
        hid_pt_send_ack(data, length);
    }
#else
    if (func_usb_pt_send_out)
    {
        func_usb_pt_send_out(data, length);
    }
    hid_pt_send_ack(data, length);
#endif
}

/* FIXME: alway send USB_PT_PKG_LEN_MAX in pkg no mater length is less than USB_PT_PKG_LEN_MAX */
bool app_usb_hid_send_passthrough(uint8_t *data, uint16_t length)
{
    if ((!data) || (!length))
    {
        return false;
    }

    if (length > PT_PKG_LEN_MAX)
    {
        return false;
    }
    APP_PRINT_INFO2("app_usb_hid_send_passthrough data %b length %x", TRACE_BINARY(5, data), length);
    uint8_t cur_queue_len = g_usb_pt_send.queue.count;
    uint16_t remain_len = length;
    uint16_t offset = 0;
    uint16_t pkg_idx = 0;
    usb_pkg_queue_t *p_pkg = NULL;

    while (remain_len)
    {
        p_pkg = calloc(1, USB_PT_PKG_LEN_MAX + 4);
        if (!p_pkg)
        {
            /* release all pkg filled in this time */
            while (g_usb_pt_send.queue.count > cur_queue_len)
            {
                free(os_queue_out(&g_usb_pt_send.queue));
            }
            return false;
        }
        p_pkg->p_next = NULL;
        p_pkg->usb_pt_pkg.report_id = REPORT_ID_PASSTHROUGH_CMD_INPUT;
        p_pkg->usb_pt_pkg.total_len = length;
        p_pkg->usb_pt_pkg.pkg_idx = pkg_idx;
        memcpy(p_pkg->usb_pt_pkg.payload, data + offset, MIN(remain_len, USB_PT_PAYLOAD_LEN_MAX));
        APP_PRINT_INFO3("app_usb_hid_send_passthrough, addr %x total_len %d payload %b", p_pkg,
                        p_pkg->usb_pt_pkg.total_len, TRACE_BINARY(remain_len, p_pkg->usb_pt_pkg.payload));
        os_queue_in(&(g_usb_pt_send.queue), p_pkg);
        /* try to send */
        app_usb_pt_try_send();

        pkg_idx++;
        offset += MIN(remain_len, USB_PT_PAYLOAD_LEN_MAX);
        remain_len -= MIN(remain_len, USB_PT_PAYLOAD_LEN_MAX);
    }

    return true;
}


static void usb_pt_send_timer_start(void)
{
    app_start_timer(&timer_idx_usb_pt_send, "usb_pt_send",
                    usb_pt_timer_id, USB_PT_TIMER_ID_SEND, 0, false,
                    USB_PT_SEND_TIMEOUT);
}

static void usb_pt_handle_send_timeout(void)
{
    if (!g_usb_pt_send.sending_pkg)
    {
        return;
    }

    usb_hid_pt_pkg_t usb_pt;
    /* drop no ack pkg, and send next pkg */
    if (g_usb_pt_send.retry_count > USB_PT_SEND_RETRY_MAX)
    {
        if (g_usb_pt_send.sending_pkg)
        {
            free(g_usb_pt_send.sending_pkg);
            g_usb_pt_send.sending_pkg = NULL;
        }
        g_usb_pt_send.retry_count = 0;
        app_usb_pt_try_send();
    }
    else
    {
        g_usb_pt_send.retry_count++;
        memcpy(&usb_pt, &(g_usb_pt_send.sending_pkg->usb_pt_pkg), sizeof(usb_hid_pt_pkg_t));

        usb_pt_send_timer_start();
        app_usb_hid_interrupt_in((uint8_t *)&usb_pt, USB_PT_PKG_LEN_MAX);
    }
}

static void usb_pt_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case USB_PT_TIMER_ID_SEND:
        app_stop_timer(&timer_idx_usb_pt_send);
        usb_pt_handle_send_timeout();
        break;
    default:
        break;
    }
}

void usb_pt_register_cb(P_PT_SENDOUT_CB cb)
{
    func_usb_pt_send_out = cb;
}

void app_usb_hid_pt_init(void)
{
    os_queue_init(&g_usb_pt_send.queue);
    g_usb_pt_send.sending_pkg = NULL;
    g_usb_pt_send.retry_count = 0;

    app_spp_register_usb_pt_cb();
    app_timer_reg_cb(usb_pt_timeout_cb, &usb_pt_timer_id);
}
#endif

