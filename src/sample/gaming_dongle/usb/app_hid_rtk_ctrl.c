#include <string.h>
#include <stdlib.h>
#include <bt_types.h>
#include "trace.h"
//#include "os_sched.h"
#include "os_mem.h"
#include "os_queue.h"
#include "os_timer.h"
#include "os_sync.h"
#include "rtl876x.h"
#include "stdlib.h"
#include "app_usb_hid.h"
#include "app_io_msg.h"
#include "app_usb_passthrough.h"
#include "usb_hid.h"
#include "app_timer.h"

#ifdef F_APP_CFU_RTL8773DO_DONGLE
#include "app_common_cfu.h"
#endif
#if F_APP_HID_RTK_SUPPORT



////////////customized 24 27 data/////////////////
#define HID_RTK_SEND_QUEUE_DATA_MAX     10
#define HID_RTK_SEND_RETRY_MAX          10
#define HID_RTK_SEND_TIMEOUT            100 //ms
#define HID_RTK_SEND_SPLIT_LEN_MAX      63
#define HID_RTK_SPLIT_PALYLOAD_MAX      60
#define HID_RTK_PKG_PLAYLOAD_MAX        2047
#define HID_RTK_HDR_LEN                 (3)

typedef struct
{
    uint8_t *buf;
    uint16_t total_len;
    uint16_t offset;
    uint16_t seq_num;
} T_HID_RTK_PKG_CTRL;

typedef enum
{
    TIMER_ID_HID_RTK_SEND  = 0x01,
} T_HID_RTK_TIMER_ID;

typedef struct t_hid_queue_pkg
{
    struct t_hid_queue_pkg *next;
    uint16_t len;
    uint8_t *buf;
} T_HID_QUEUE_PKG;

#pragma  pack(1)
typedef struct
{
    uint8_t report_id;
    uint16_t len: 11;
    uint16_t seq_num: 5;
    uint8_t payload[];
} T_HID_RTK_PKG;
#pragma  pack()

#define HID_QUEUE_HDR_SIZE    (sizeof(T_HID_QUEUE_PKG) - 4)

typedef struct
{
    T_OS_QUEUE          send_queue;
    T_HID_RTK_PKG_CTRL  send_ctrl;
    T_HID_RTK_PKG_CTRL  recv_ctrl;
    uint8_t             split_len;
    uint8_t             split_buf[HID_RTK_SEND_SPLIT_LEN_MAX];
    uint8_t             send_retry_cnt;
    bool                sending;
} T_HID_RTK;

static uint8_t hid_rtk_timer_id = 0;
static uint8_t timer_idx_rtk_send = 0;

T_HID_RTK *hid_rtk_db = NULL;

static bool app_hid_rtk_send_split(uint8_t *buf, uint16_t len);

static void hid_rtk_send_timer_stop(void);

static void hid_rtk_send_timer_start(void);


static bool app_hid_rtk_send_split(uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len > HID_RTK_PKG_PLAYLOAD_MAX))
    {
        return false;
    }

    uint8_t payload_len = 0;
    T_HID_RTK_PKG_CTRL *p_send = &(hid_rtk_db->send_ctrl);
    if (hid_rtk_db->send_ctrl.total_len)
    {
        APP_PRINT_ERROR1("app_hid_rtk_send_split: unfinished %d", hid_rtk_db->send_ctrl.total_len);
        return false;
    }

    p_send->buf = buf;
    p_send->total_len = len;
    p_send->offset = 0;
    p_send->seq_num = 0;

    payload_len = (len > HID_RTK_SPLIT_PALYLOAD_MAX) ? HID_RTK_SPLIT_PALYLOAD_MAX : len;
    memset(hid_rtk_db->split_buf, 0, HID_RTK_SEND_SPLIT_LEN_MAX);
    hid_rtk_db->split_buf[0] = REPORT_ID_CTRL_DATA_IN_REQUEST;
    hid_rtk_db->split_buf[1] = (p_send->total_len) & 0xFF;
    hid_rtk_db->split_buf[2] = (p_send->seq_num << 3) | ((p_send->total_len >> 8) & 0x07);
    memcpy(hid_rtk_db->split_buf + 3, buf, payload_len);
    hid_rtk_db->split_len = HID_RTK_SEND_SPLIT_LEN_MAX;

    app_usb_hid_interrupt_in(hid_rtk_db->split_buf, HID_RTK_SEND_SPLIT_LEN_MAX);
    hid_rtk_send_timer_start();
    return true;
}

static void app_hid_rtk_recv_handle_ack(uint8_t *buf, uint16_t len)
{
    T_HID_RTK_PKG *p_pkg = (T_HID_RTK_PKG *)buf;
    T_HID_RTK_PKG_CTRL *p_send = &(hid_rtk_db->send_ctrl);
    uint16_t remain_len = 0;
    uint8_t payload_len = 0;
    uint8_t *p_send_buf = NULL;
    T_HID_QUEUE_PKG *p_elem = NULL;

    if (p_pkg->seq_num != p_send->seq_num)
    {
        APP_PRINT_WARN2("app_hid_rtk_recv_handle_ack: send seq_num %x, ack seq_num %x",
                        p_send->seq_num, p_pkg->seq_num);
        return;
    }

    hid_rtk_send_timer_stop();
    remain_len = p_send->total_len - p_send->seq_num * HID_RTK_SPLIT_PALYLOAD_MAX;
    if (remain_len > HID_RTK_SPLIT_PALYLOAD_MAX)
    {
        p_send->seq_num ++;
        p_send->offset += HID_RTK_SPLIT_PALYLOAD_MAX;
        p_send_buf = p_send->buf + p_send->offset;
        payload_len = HID_RTK_SPLIT_PALYLOAD_MAX;
        memset(hid_rtk_db->split_buf, 0, HID_RTK_SEND_SPLIT_LEN_MAX);
        hid_rtk_db->split_buf[0] = REPORT_ID_CTRL_DATA_IN_REQUEST;
        hid_rtk_db->split_buf[1] = (p_send->total_len) & 0xFF;
        hid_rtk_db->split_buf[2] = (p_send->seq_num << 3) | ((p_send->total_len >> 8) & 0x07);
        memcpy(hid_rtk_db->split_buf + 3, p_send_buf, payload_len);
        hid_rtk_db->split_len = HID_RTK_SEND_SPLIT_LEN_MAX;

        app_usb_hid_interrupt_in(hid_rtk_db->split_buf, HID_RTK_SEND_SPLIT_LEN_MAX);
        hid_rtk_send_timer_start();
    }
    else
    {
        p_elem = os_queue_out(&(hid_rtk_db->send_queue));
        free(p_elem->buf);
        free(p_elem);
        p_send->buf = NULL;
        p_send->total_len = 0;
        p_send->offset = 0;
        p_send->seq_num = 0;
        hid_rtk_db->sending  = false;

        if (hid_rtk_db->send_queue.count)
        {
            T_IO_MSG gpio_msg;
            gpio_msg.type = IO_MSG_TYPE_USB_HID;
            gpio_msg.subtype = USB_HID_MSG_TYPE_CTRL_DATA_REQUEST;
            gpio_msg.u.param = 0;
            hid_rtk_db->sending = true;
            if (app_io_msg_send(&gpio_msg) == false)
            {
                hid_rtk_db->sending = false;
                p_elem = os_queue_out(&(hid_rtk_db->send_queue));
                free(p_elem->buf);
                free(p_elem);
            }
        }
    }
}

static void app_hid_rtk_recv_handle_cmd(uint8_t *buf, uint16_t len)
{
    T_HID_RTK_PKG_CTRL *p_recv = &(hid_rtk_db->recv_ctrl);
    T_HID_RTK_PKG *p_pkg = (T_HID_RTK_PKG *)buf;
    uint16_t opcode = *(uint16_t *)(p_pkg->payload);
    uint16_t remain_len = 0;
    uint8_t hid_ack_pkg[HID_RTK_SEND_SPLIT_LEN_MAX] = {0x0};

    hid_ack_pkg[0] = REPORT_ID_CTRL_DATA_IN_REQUEST;
    hid_ack_pkg[1] = 0x04;
    hid_ack_pkg[2] = p_pkg->seq_num;
    hid_ack_pkg[3] = 0x00;
    hid_ack_pkg[4] = 0x00;
    hid_ack_pkg[5] = opcode & 0xFF;
    hid_ack_pkg[6] = (opcode >> 8) & 0xFF;

    if (p_pkg->len < 2)
    {
        APP_PRINT_ERROR1("app_hid_rtk_recv_handle_cmd: len %x", p_pkg->len);
        return;
    }

    if (p_pkg->len <= p_pkg->seq_num * HID_RTK_SPLIT_PALYLOAD_MAX)
    {
        APP_PRINT_ERROR2("app_hid_rtk_recv_handle_cmd: pkg len %x, seq_num %x", p_pkg->len, p_pkg->seq_num);
        return;
    }

    if (p_recv->total_len)
    {
        if (p_pkg->seq_num != p_recv->seq_num + 1)
        {
            APP_PRINT_WARN2("app_hid_rtk_recv_handle_cmd: recv seq_num %x, pkg seq_num %x", p_recv->seq_num,
                            p_pkg->seq_num);
            return;
        }
        /*opcode in the first hid packeg*/
        hid_ack_pkg[5] = p_recv->buf[0];
        hid_ack_pkg[6] = p_recv->buf[1];

        remain_len = p_pkg->len - p_pkg->seq_num * HID_RTK_SPLIT_PALYLOAD_MAX;

        if (remain_len > HID_RTK_SPLIT_PALYLOAD_MAX)
        {
            memcpy(p_recv->buf + p_recv->offset, p_pkg->payload, HID_RTK_SPLIT_PALYLOAD_MAX);
            p_recv->offset += HID_RTK_SPLIT_PALYLOAD_MAX;
            p_recv->seq_num += 1;
        }
        else
        {
            memcpy(p_recv->buf + p_recv->offset, p_pkg->payload, remain_len);
            if (app_usb_hid_cb_f.app_usb_hid_bt_ctrl_data_recv_cb)
            {
                app_usb_hid_cb_f.app_usb_hid_bt_ctrl_data_recv_cb(p_recv->total_len, p_recv->buf);
            }
            free(p_recv->buf);
            p_recv->buf = NULL;
            p_recv->total_len = 0;
            p_recv->offset = 0;
            p_recv->seq_num = 0;
        }
    }
    else
    {
        if (p_pkg->len <= HID_RTK_SPLIT_PALYLOAD_MAX)
        {
            if (app_usb_hid_cb_f.app_usb_hid_bt_ctrl_data_recv_cb)
            {
                app_usb_hid_cb_f.app_usb_hid_bt_ctrl_data_recv_cb(p_pkg->len, p_pkg->payload);
            }
        }
        else
        {
            p_recv->buf = calloc(1, p_pkg->len);
            if (p_recv->buf == NULL)
            {
                APP_PRINT_ERROR1("app_hid_rtk_recv_handle_cmd: alloc %x fail", p_pkg->len);
                return;
            }
            p_recv->total_len = p_pkg->len;
            p_recv->seq_num = 0;
            memcpy(p_recv->buf, p_pkg->payload, HID_RTK_SPLIT_PALYLOAD_MAX);
            p_recv->offset += HID_RTK_SPLIT_PALYLOAD_MAX;
        }
    }
    app_usb_hid_interrupt_in(hid_ack_pkg, sizeof(hid_ack_pkg));
}

static void hid_rtk_send_timer_stop(void)
{
    app_stop_timer(&timer_idx_rtk_send);
}

static void hid_rtk_send_timer_start(void)
{
    app_start_timer(&timer_idx_rtk_send,
                    "hid_rtk_send_timer",
                    hid_rtk_timer_id,
                    TIMER_ID_HID_RTK_SEND,
                    0,
                    false,
                    HID_RTK_SEND_TIMEOUT);
}

static void hid_rtk_handle_send_timeout(void)
{
    APP_PRINT_INFO2("hid_rtk_handle_send_timeout: ack lost, retry %d, queue count %d",
                    hid_rtk_db->send_retry_cnt, hid_rtk_db->send_queue.count);
    T_HID_QUEUE_PKG *p_elem = NULL;

    if (hid_rtk_db->send_retry_cnt >= HID_RTK_SEND_RETRY_MAX)
    {
        APP_PRINT_ERROR1("hid_rtk_handle_send_timeout: retry cnt %d", hid_rtk_db->send_retry_cnt);
        while (hid_rtk_db->send_queue.count)
        {
            p_elem = os_queue_out(&hid_rtk_db->send_queue);
            free(p_elem->buf);
            free(p_elem);
        }
        hid_rtk_db->sending = false;
        hid_rtk_db->send_ctrl.buf = NULL;
        hid_rtk_db->send_ctrl.total_len = 0;
        hid_rtk_db->send_ctrl.offset = 0;
        hid_rtk_db->send_ctrl.seq_num = 0;
        return;
    }

    if (hid_rtk_db->split_len <= HID_RTK_SEND_SPLIT_LEN_MAX)
    {
        app_usb_hid_interrupt_in(hid_rtk_db->split_buf, hid_rtk_db->split_len);
    }
    hid_rtk_db->send_retry_cnt++;
    hid_rtk_send_timer_start();
}


static void hid_rtk_timeout_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_INFO2("hid_rtk_timeout_cback: id %x chann %x", timer_id, timer_chann);
    switch (timer_id)
    {
    case TIMER_ID_HID_RTK_SEND:
        {
            hid_rtk_send_timer_stop();
            hid_rtk_handle_send_timeout();
        }
        break;

    default:
        break;
    }
}

void app_hid_rtk_ctrl_handle_send_msg(void)
{
    APP_PRINT_INFO2("app_hid_rtk_ctrl_handle_send_msg: sending %x count %x",
                    hid_rtk_db->sending, hid_rtk_db->send_queue.count);
    if (hid_rtk_db->sending == false)
    {
        return;
    }

    if (hid_rtk_db->send_queue.count == 0)
    {
        return;
    }

    T_HID_QUEUE_PKG *p_queue_pkg = os_queue_peek(&(hid_rtk_db->send_queue), 0);

    if (app_hid_rtk_send_split(p_queue_pkg->buf, p_queue_pkg->len) == false)
    {
        APP_PRINT_WARN1("app_hid_rtk_ctrl_handle_send_msg: fail %b", TRACE_BINARY(4, p_queue_pkg->buf));
        free(p_queue_pkg);
    }
}

bool app_hid_rtk_ctrl_send(uint16_t length, uint8_t *data)
{
#if 0
    APP_PRINT_TRACE4("app_hid_rtk_ctrl_send: queue_count %d, retry_cnt %d, sending %d, data len %d",
                     hid_rtk_db->send_queue.count, hid_rtk_db->send_retry_cnt,
                     hid_rtk_db->sending, length);
    APP_PRINT_TRACE2("app_hid_rtk_ctrl_send: len %d, data %b",
                     length, TRACE_BINARY(length, data));
#endif
    if ((data == NULL) || (length > HID_RTK_PKG_PLAYLOAD_MAX))
    {
        return false;
    }

    if (hid_rtk_db->send_queue.count > HID_RTK_SEND_QUEUE_DATA_MAX)
    {
        return false;
    }

    T_HID_QUEUE_PKG *p_pkg = calloc(1, sizeof(T_HID_QUEUE_PKG));
    if (!p_pkg)
    {
        APP_PRINT_ERROR1("app_hid_rtk_ctrl_send: alloc pkg %x fail", sizeof(T_HID_QUEUE_PKG));
        return false;
    }
    p_pkg->buf = calloc(1, length);
    if (p_pkg->buf == NULL)
    {
        free(p_pkg);
        APP_PRINT_ERROR1("app_hid_rtk_ctrl_send: alloc buf %x fail", length);
        return false;
    }

    p_pkg->len = length;
    memcpy(p_pkg->buf, data, length);
    os_queue_in(&(hid_rtk_db->send_queue), p_pkg);

    if (hid_rtk_db->sending)
    {
        return true;
    }

    T_IO_MSG gpio_msg;
    gpio_msg.type = IO_MSG_TYPE_USB_HID;
    gpio_msg.subtype = USB_HID_MSG_TYPE_CTRL_DATA_REQUEST;
    gpio_msg.u.param = 0;
    hid_rtk_db->sending = true;

    if (app_io_msg_send(&gpio_msg) == false)
    {
        os_queue_delete(&(hid_rtk_db->send_queue), p_pkg);
        free(p_pkg->buf);
        free(p_pkg);
        hid_rtk_db->sending = false;
        APP_PRINT_ERROR0("app_hid_rtk_ctrl_send: msg send fail");
        return false;
    }
    return true;
}


void app_hid_rtk_recv_data(uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0))
    {
        return;
    }

    T_HID_RTK_PKG *p_pkg = (T_HID_RTK_PKG *)buf;
    uint16_t opcode = *(uint16_t *)(p_pkg->payload);
#if 0
    APP_PRINT_INFO2("app_hid_rtk_recv_data: data %b, opcode %x", TRACE_BINARY(len, buf),
                    opcode);
    APP_PRINT_INFO4("app_hid_rtk_recv_data: report %x, len %x seq %x data %x", p_pkg->report_id,
                    p_pkg->len, p_pkg->seq_num, p_pkg->payload[0]);
#endif
    if (opcode == 0) //hid ack
    {
        app_hid_rtk_recv_handle_ack(buf, len);
    }
    else
    {
        app_hid_rtk_recv_handle_cmd(buf, len);
    }
}

void app_hid_rtk_init(void)
{
    hid_rtk_db = calloc(1, sizeof(T_HID_RTK));
    if (hid_rtk_db == NULL)
    {
        return;
    }

    os_queue_init(&(hid_rtk_db->send_queue));
    app_timer_reg_cb(hid_rtk_timeout_cback, &hid_rtk_timer_id);
}

#endif
