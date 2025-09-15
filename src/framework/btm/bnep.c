/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "mpa.h"
#include "bt_mgr.h"
#include "bnep.h"

#define BNEP_FLAG_IS_INITIATOR    0x01
#define BNEP_FLAG_SETUP_SENT      0x02
#define BNEP_FLAG_NET_TYPE_SENT   0x04
#define BNEP_FLAG_MULTI_ADDR_SENT 0x08
#define BNEP_FLAG_SETUP_RCVD      0x10
#define BNEP_FLAG_CONN_COMPLETED  0x20

#define BNEP_MAX_NET_TYPE_FILTERS   5
#define BNEP_MAX_MULTI_ADDR_FILTERS 5

#define BNEP_MTU                               1691
#define BNEP_UUID_SIZE                         2
#define BNEP_ADDR_SIZE                         6
#define BNEP_NETWORK_PROTOCOL_TYPE_SIZE        2
#define BNEP_802_1_Q_TCI_SIZE                  2
#define BNEP_TYPE_SIZE                         1
#define BNEP_CONTROL_TYPE_SIZE                 1
#define BNEP_802_1_Q_PAYLOAD_LEN               4
#define BNEP_802_1_Q_PROTOCOL                  0x8100 /* IEEE 802.1Q tag */

/* BNEP packet types */
#define BNEP_PKT_TYPE_GENERAL_ETHERNET                  0x00
#define BNEP_PKT_TYPE_CONTROL                           0x01
#define BNEP_PKT_TYPE_COMPRESSED_ETHERNET               0x02
#define BNEP_PKT_TYPE_COMPRESSED_ETHERNET_SOURCE_ONLY   0x03
#define BNEP_PKT_TYPE_COMPRESSED_ETHERNET_DEST_ONLY     0x04

/* BNEP control types */
#define BNEP_CONTROL_TYPE_COMMAND_NOT_UNDERSTOOD        0x00
#define BNEP_CONTROL_TYPE_SETUP_CONNECTION_REQUEST      0x01
#define BNEP_CONTROL_TYPE_SETUP_CONNECTION_RESPONSE     0x02
#define BNEP_CONTROL_TYPE_FILTER_NET_TYPE_SET           0x03
#define BNEP_CONTROL_TYPE_FILTER_NET_TYPE_RESPONSE      0x04
#define BNEP_CONTROL_TYPE_FILTER_MULTI_ADDR_SET         0x05
#define BNEP_CONTROL_TYPE_FILTER_MULTI_ADDR_RESPONSE    0x06

/* BNEP extension header types */
#define BNEP_EXT_HEADER_TYPE_EXTENSION_CONTROL          0x00

/* BNEP setup response codes */
#define BNEP_RSP_SETUP_SUCCESS                          0x0000
#define BNEP_RSP_SETUP_INVALID_DEST_UUID                0x0001
#define BNEP_RSP_SETUP_INVALID_SOURCE_UUID              0x0002
#define BNEP_RSP_SETUP_INVALID_SERVICE_UUID_SIZE        0x0003
#define BNEP_RSP_SETUP_CONNECTION_NOT_ALLOWED           0x0004

/* BNEP filter response codes */
#define BNEP_RSP_FILTER_SUCCESS                         0x0000
#define BNEP_RSP_FILTER_UNSUPPORTED_REQUEST             0x0001
#define BNEP_RSP_FILTER_ERR_INVALID_RANGE               0x0002
#define BNEP_RSP_FILTER_ERR_TOO_MANY_FILTERS            0x0003
#define BNEP_RSP_FILTER_ERR_SECURITY                    0x0004

#define BNEP_CONNECTION_TIMEOUT                         10000  /* uint is ms */
#define BNEP_RETRY_COUNT                                3

typedef enum t_bnep_state
{
    BNEP_STATE_DISCONNECTED  = 0x00,
    BNEP_STATE_CONNECTING    = 0x01,
    BNEP_STATE_CONNECTED     = 0x02,
    BNEP_STATE_DISCONNECTING = 0x03
} T_BNEP_STATE;

typedef struct t_bnep_link
{
    struct t_bnep_link *next;
    uint8_t             local_addr[6];
    uint8_t             remote_addr[6];
    uint8_t             conn_flag;
    T_BNEP_STATE        state;
    uint16_t            cid;
    uint16_t            remote_mtu;
    uint16_t            ds_data_offset;
    uint8_t             setup_conn_retry_count;
    uint8_t             net_type_filter_retry_count;
    uint8_t             multi_addr_filter_retry_count;
    uint16_t            src_uuid;
    uint16_t            dst_uuid;
    uint16_t            prev_src_uuid;
    uint16_t            prev_dst_uuid;
    T_SYS_TIMER_HANDLE  setup_conn_timer;
    T_SYS_TIMER_HANDLE  net_type_filter_timer;
    T_SYS_TIMER_HANDLE  multi_addr_filter_timer;
    uint16_t            rcvd_net_type_filter_count;
    uint16_t            rcvd_multi_addr_filter_count;
    uint16_t            sent_net_type_filter_count;
    uint16_t            sent_multi_addr_filter_count;
    uint16_t           *rcvd_net_type_filter_start;
    uint16_t           *rcvd_net_type_filter_end;
    uint8_t (*rcvd_multi_addr_filter_start)[6];
    uint8_t (*rcvd_multi_addr_filter_end)[6];
    uint16_t           *sent_net_type_filter_start;
    uint16_t           *sent_net_type_filter_end;
    uint8_t (*sent_multi_addr_filter_start)[6];
    uint8_t (*sent_multi_addr_filter_end)[6];
} T_BNEP_LINK;

typedef struct t_bnep
{
    T_OS_QUEUE           list;
    uint8_t              queue_id;
    P_BNEP_CBACK         cback;
} T_BNEP;

static const uint8_t bnep_frame_hdr_size[] = {14, 1, 2, 8, 8};

static T_BNEP *bnep = NULL;

T_BNEP_LINK *bnep_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BNEP_LINK *link;

    link = os_queue_peek(&bnep->list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->remote_addr, bd_addr, 6))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

T_BNEP_LINK *bnep_find_link_by_cid(uint16_t cid)
{
    T_BNEP_LINK *link;

    link = os_queue_peek(&bnep->list, 0);
    while (link != NULL)
    {
        if (link->cid == cid)
        {
            break;
        }

        link = link->next;
    }

    return link;
}

void bnep_setup_conn_tout_cback(T_SYS_TIMER_HANDLE handle)
{
    T_BNEP_LINK *link;

    link = (T_BNEP_LINK *)sys_timer_id_get(handle);
    if (os_queue_search(&bnep->list, link) == true)
    {
        PROTOCOL_PRINT_TRACE1("bnep_setup_conn_tout_cback: bd_addr %s",
                              TRACE_BDADDR(link->remote_addr));

        if (link->setup_conn_timer != NULL)
        {
            sys_timer_delete(link->setup_conn_timer);
            link->setup_conn_timer = NULL;
        }

        if (link->setup_conn_retry_count++ <= BNEP_RETRY_COUNT)
        {
            link->conn_flag &= ~BNEP_FLAG_SETUP_SENT;
            bnep_setup_connection_req(link->remote_addr,
                                      link->src_uuid,
                                      link->dst_uuid);
        }
        else
        {
            bnep_disconnect_req(link->remote_addr);
        }
    }
}

void bnep_net_type_filter_tout_cback(T_SYS_TIMER_HANDLE handle)
{
    T_BNEP_LINK *link;

    link = (T_BNEP_LINK *)sys_timer_id_get(handle);
    if (os_queue_search(&bnep->list, link) == true)
    {
        PROTOCOL_PRINT_TRACE1("bnep_net_type_filter_tout_cback: bd_addr %s",
                              TRACE_BDADDR(link->remote_addr));

        if (link->net_type_filter_timer != NULL)
        {
            sys_timer_delete(link->net_type_filter_timer);
            link->net_type_filter_timer = NULL;
        }

        if (link->net_type_filter_retry_count++ <= BNEP_RETRY_COUNT)
        {
            link->conn_flag &= ~BNEP_FLAG_NET_TYPE_SENT;
            bnep_filter_net_type_set(link->remote_addr,
                                     link->sent_net_type_filter_count,
                                     link->sent_net_type_filter_start,
                                     link->sent_net_type_filter_end);
        }
        else
        {
            bnep_disconnect_req(link->remote_addr);
        }
    }
}

void bnep_multi_addr_filter_tout_cback(T_SYS_TIMER_HANDLE handle)
{
    T_BNEP_LINK *link;

    link = (T_BNEP_LINK *)sys_timer_id_get(handle);
    if (os_queue_search(&bnep->list, link) == true)
    {
        PROTOCOL_PRINT_TRACE1("bnep_multi_addr_filter_tout_cback: bd_addr %s",
                              TRACE_BDADDR(link->remote_addr));

        if (link->multi_addr_filter_timer != NULL)
        {
            sys_timer_delete(link->multi_addr_filter_timer);
            link->multi_addr_filter_timer = NULL;
        }

        if (link->multi_addr_filter_retry_count++ <= BNEP_RETRY_COUNT)
        {
            link->conn_flag &= ~BNEP_FLAG_MULTI_ADDR_SENT;
            bnep_filter_multi_addr_set(link->remote_addr,
                                       link->sent_multi_addr_filter_count,
                                       link->sent_multi_addr_filter_start,
                                       link->sent_multi_addr_filter_end);
        }
        else
        {
            bnep_disconnect_req(link->remote_addr);
        }
    }
}

T_BNEP_LINK *bnep_alloc_link(uint8_t bd_addr[6])
{
    T_BNEP_LINK *link;

    link = calloc(1, sizeof(T_BNEP_LINK));
    if (link != NULL)
    {
        link->state = BNEP_STATE_DISCONNECTED;
        memcpy(link->remote_addr, bd_addr, 6);
        os_queue_in(&bnep->list, link);
    }

    return link;
}

void bnep_free_link(T_BNEP_LINK *link)
{
    if (os_queue_delete(&bnep->list, link) == true)
    {
        if (link->setup_conn_timer != NULL)
        {
            sys_timer_delete(link->setup_conn_timer);
        }

        if (link->net_type_filter_timer != NULL)
        {
            sys_timer_delete(link->net_type_filter_timer);
        }

        if (link->multi_addr_filter_timer != NULL)
        {
            sys_timer_delete(link->multi_addr_filter_timer);
        }

        if (link->rcvd_net_type_filter_start != NULL)
        {
            free(link->rcvd_net_type_filter_start);
        }

        if (link->rcvd_net_type_filter_end != NULL)
        {
            free(link->rcvd_net_type_filter_end);
        }

        if (link->sent_net_type_filter_start != NULL)
        {
            free(link->sent_net_type_filter_start);
        }

        if (link->sent_net_type_filter_end != NULL)
        {
            free(link->sent_net_type_filter_end);
        }

        if (link->rcvd_multi_addr_filter_start != NULL)
        {
            free(link->rcvd_multi_addr_filter_start);
        }

        if (link->rcvd_multi_addr_filter_end != NULL)
        {
            free(link->rcvd_multi_addr_filter_end);
        }

        if (link->sent_multi_addr_filter_start != NULL)
        {
            free(link->sent_multi_addr_filter_start);
        }

        if (link->sent_multi_addr_filter_end != NULL)
        {
            free(link->sent_multi_addr_filter_end);
        }

        free(link);
    }
}

bool bnep_control_packet_send(T_BNEP_LINK *link,
                              uint8_t      control_type,
                              uint8_t     *buf,
                              uint16_t     len)
{
    uint16_t  pkt_len;
    uint8_t  *bnep_buf;
    int32_t   ret = 0;

    pkt_len = len + BNEP_TYPE_SIZE + BNEP_CONTROL_TYPE_SIZE;
    if (pkt_len > link->remote_mtu)
    {
        ret = 1;
        goto fail_invalid_len;
    }

    bnep_buf = mpa_get_l2c_buf(bnep->queue_id,
                               link->cid,
                               0,
                               pkt_len,
                               link->ds_data_offset,
                               false);
    if (bnep_buf == NULL)
    {
        ret = 2;
        goto fail_get_l2c_buf;
    }

    bnep_buf[link->ds_data_offset] = BNEP_PKT_TYPE_CONTROL;
    bnep_buf[link->ds_data_offset + 1] = control_type;
    memcpy(bnep_buf + link->ds_data_offset + 2, buf, len);

    mpa_send_l2c_data_req(bnep_buf,
                          link->ds_data_offset,
                          link->cid,
                          pkt_len,
                          false);

    return true;

fail_get_l2c_buf:
fail_invalid_len:
    PROTOCOL_PRINT_ERROR3("bnep_control_packet_send: failed %d, bd_addr %s, control_type 0x%02x",
                          -ret, link->remote_addr, control_type);
    return false;
}

bool bnep_control_command_not_understood_send(uint8_t bd_addr[6],
                                              uint8_t control_type)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        return bnep_control_packet_send(link,
                                        BNEP_CONTROL_TYPE_COMMAND_NOT_UNDERSTOOD,
                                        &control_type,
                                        1);
    }

    return false;
}

bool bnep_setup_connection_req(uint8_t  bd_addr[6],
                               uint16_t src_uuid,
                               uint16_t dst_uuid)
{
    T_BNEP_LINK *link;
    uint8_t      buf[5];
    uint8_t     *p;
    uint32_t     ret = 0;

    link = bnep_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_no_link;
    }

    if (link->conn_flag & (BNEP_FLAG_SETUP_SENT | BNEP_FLAG_SETUP_RCVD))
    {
        ret = 2;
        goto fail_invalid_conn_flag;
    }

    p = buf;
    BE_UINT8_TO_STREAM(p, BNEP_UUID_SIZE);
    BE_UINT16_TO_STREAM(p, dst_uuid);
    BE_UINT16_TO_STREAM(p, src_uuid);

    link->setup_conn_timer = sys_timer_create("bnep_setup_conn",
                                              SYS_TIMER_TYPE_LOW_PRECISION,
                                              (uint32_t)link,
                                              BNEP_CONNECTION_TIMEOUT * 1000,
                                              false,
                                              bnep_setup_conn_tout_cback);

    if (link->setup_conn_timer == NULL)
    {
        ret = 3;
        goto fail_create_timer;
    }

    if (bnep_control_packet_send(link,
                                 BNEP_CONTROL_TYPE_SETUP_CONNECTION_REQUEST,
                                 buf,
                                 sizeof(buf)) == false)
    {
        ret = 4;
        goto fail_send_pkt;
    }

    link->src_uuid = src_uuid;
    link->dst_uuid = dst_uuid;
    link->conn_flag |= BNEP_FLAG_SETUP_SENT;
    sys_timer_start(link->setup_conn_timer);
    return true;

fail_send_pkt:
    sys_timer_delete(link->setup_conn_timer);
    link->setup_conn_timer = NULL;
fail_create_timer:
fail_invalid_conn_flag:
fail_no_link:
    PROTOCOL_PRINT_ERROR2("bnep_setup_connection_req: failed %d, bd_addr %s",
                          -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bnep_setup_connection_rsp(uint8_t  bd_addr[6],
                               uint16_t rsp_msg)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->conn_flag & BNEP_FLAG_SETUP_RCVD)
        {
            uint8_t buf[2];

            link->conn_flag &= ~BNEP_FLAG_SETUP_RCVD;
            BE_UINT16_TO_ARRAY(buf, rsp_msg);
            if (rsp_msg == BNEP_RSP_SETUP_SUCCESS)
            {
                link->conn_flag |= BNEP_FLAG_CONN_COMPLETED;
            }
            else
            {
                if (link->conn_flag & BNEP_FLAG_CONN_COMPLETED)
                {
                    link->dst_uuid = link->prev_dst_uuid;
                    link->src_uuid = link->prev_src_uuid;
                }
            }

            return bnep_control_packet_send(link,
                                            BNEP_CONTROL_TYPE_SETUP_CONNECTION_RESPONSE,
                                            buf,
                                            sizeof(uint16_t));
        }
    }

    return false;
}

bool bnep_filter_net_type_set(uint8_t   bd_addr[6],
                              uint16_t  filter_num,
                              uint16_t *start_array,
                              uint16_t *end_array)
{
    T_BNEP_LINK *link;
    uint8_t     *buf;
    uint8_t     *p;
    uint16_t     i;
    uint32_t     ret = 0;

    link = bnep_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_no_link;
    }

    if (!(link->conn_flag & BNEP_FLAG_CONN_COMPLETED) ||
        (link->conn_flag & BNEP_FLAG_NET_TYPE_SENT))
    {
        ret = 2;
        goto fail_invalid_conn_flag;
    }

    buf = malloc(2 + filter_num * 2 * BNEP_NETWORK_PROTOCOL_TYPE_SIZE);
    if (buf == NULL)
    {
        ret = 3;
        goto fail_alloc_msg_buf;
    }

    p = buf;
    BE_UINT16_TO_STREAM(p, filter_num * 2 * BNEP_NETWORK_PROTOCOL_TYPE_SIZE);
    link->sent_net_type_filter_count = filter_num;

    link->sent_net_type_filter_start = malloc(filter_num * BNEP_NETWORK_PROTOCOL_TYPE_SIZE);
    if (link->sent_net_type_filter_start == NULL)
    {
        ret = 4;
        goto fail_alloc_sent_net_type_filter_start;
    }

    link->sent_net_type_filter_end = malloc(filter_num * BNEP_NETWORK_PROTOCOL_TYPE_SIZE);
    if (link->sent_net_type_filter_end == NULL)
    {
        ret = 5;
        goto fail_alloc_sent_net_type_filter_end;
    }

    for (i = 0; i < filter_num; i++)
    {
        link->sent_net_type_filter_start[i] = start_array[i];
        link->sent_net_type_filter_end[i] = end_array[i];
        BE_UINT16_TO_STREAM(p, start_array[i]);
        BE_UINT16_TO_STREAM(p, end_array[i]);
    }

    link->net_type_filter_timer = sys_timer_create("bnep_net_type_filter",
                                                   SYS_TIMER_TYPE_LOW_PRECISION,
                                                   (uint32_t)link,
                                                   BNEP_CONNECTION_TIMEOUT * 1000,
                                                   false,
                                                   bnep_net_type_filter_tout_cback);
    if (link->net_type_filter_timer == NULL)
    {
        ret = 6;
        goto fail_create_timer;
    }

    if (bnep_control_packet_send(link,
                                 BNEP_CONTROL_TYPE_FILTER_NET_TYPE_SET,
                                 buf,
                                 p - buf) == false)
    {
        ret = 7;
        goto fail_send_pkt;
    }

    link->conn_flag |= BNEP_FLAG_NET_TYPE_SENT;
    sys_timer_start(link->net_type_filter_timer);
    free(buf);
    return true;

fail_send_pkt:
    sys_timer_delete(link->net_type_filter_timer);
    link->net_type_filter_timer = NULL;
fail_create_timer:
    free(link->sent_net_type_filter_end);
    link->sent_net_type_filter_end = NULL;
fail_alloc_sent_net_type_filter_end:
    free(link->sent_net_type_filter_start);
    link->sent_net_type_filter_start = NULL;
fail_alloc_sent_net_type_filter_start:
    link->sent_net_type_filter_count = 0;
    free(buf);
fail_alloc_msg_buf:
fail_invalid_conn_flag:
fail_no_link:
    PROTOCOL_PRINT_ERROR2("bnep_filter_net_type_set: failed %d, bd_addr %s",
                          -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bnep_filter_net_type_rsp(uint8_t  bd_addr[6],
                              uint16_t rsp_msg)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t buf[2];

        BE_UINT16_TO_ARRAY(buf, rsp_msg);
        return bnep_control_packet_send(link,
                                        BNEP_CONTROL_TYPE_FILTER_NET_TYPE_RESPONSE,
                                        buf,
                                        sizeof(uint16_t));
    }

    return false;
}

bool bnep_filter_multi_addr_set(uint8_t  bd_addr[6],
                                uint16_t filter_num,
                                uint8_t  start_array[][6],
                                uint8_t  end_array[][6])
{
    T_BNEP_LINK *link;
    uint8_t     *buf;
    uint8_t     *p;
    uint16_t     i;
    uint32_t     ret = 0;;

    link = bnep_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_no_link;
    }

    if (!(link->conn_flag & BNEP_FLAG_CONN_COMPLETED) ||
        (link->conn_flag & BNEP_FLAG_MULTI_ADDR_SENT))
    {
        ret = 2;
        goto fail_invalid_conn_flag;
    }

    buf = malloc(2 + filter_num * 2 * BNEP_ADDR_SIZE);
    if (buf == NULL)
    {
        ret = 3;
        goto fail_alloc_msg_buf;
    }

    p = buf;
    BE_UINT16_TO_STREAM(p, filter_num * 2 * BNEP_ADDR_SIZE);
    link->sent_multi_addr_filter_start = malloc(filter_num * BNEP_ADDR_SIZE);
    link->sent_multi_addr_filter_count = filter_num;
    if (link->sent_multi_addr_filter_start == NULL)
    {
        ret = 4;
        goto fail_alloc_sent_multi_addr_filter_start;
    }

    link->sent_multi_addr_filter_end = malloc(filter_num * BNEP_ADDR_SIZE);
    if (link->sent_multi_addr_filter_end == NULL)
    {
        ret = 5;
        goto fail_alloc_sent_multi_addr_filter_end;
    }

    for (i = 0; i < filter_num; i++)
    {
        memcpy(link->sent_multi_addr_filter_start[i], start_array[i], BNEP_ADDR_SIZE);
        memcpy(link->sent_multi_addr_filter_end[i], end_array[i], BNEP_ADDR_SIZE);
        ARRAY_TO_STREAM(p, start_array[i], BNEP_ADDR_SIZE);
        ARRAY_TO_STREAM(p, end_array[i], BNEP_ADDR_SIZE);
    }

    link->multi_addr_filter_timer = sys_timer_create("bnep_multi_addr_filter",
                                                     SYS_TIMER_TYPE_LOW_PRECISION,
                                                     (uint32_t)link,
                                                     BNEP_CONNECTION_TIMEOUT * 1000,
                                                     false,
                                                     bnep_multi_addr_filter_tout_cback);
    if (link->multi_addr_filter_timer == NULL)
    {
        ret = 6;
        goto fail_create_timer;
    }

    if (bnep_control_packet_send(link,
                                 BNEP_CONTROL_TYPE_FILTER_MULTI_ADDR_SET,
                                 buf,
                                 p - buf) == false)
    {
        ret = 7;
        goto fail_send_pkt;
    }

    link->conn_flag |= BNEP_FLAG_MULTI_ADDR_SENT;
    sys_timer_start(link->multi_addr_filter_timer);
    free(buf);
    return true;

fail_send_pkt:
    sys_timer_delete(link->multi_addr_filter_timer);
    link->multi_addr_filter_timer = NULL;
fail_create_timer:
    free(link->sent_multi_addr_filter_end);
    link->sent_multi_addr_filter_end = NULL;
fail_alloc_sent_multi_addr_filter_end:
    free(link->sent_multi_addr_filter_start);
    link->sent_multi_addr_filter_start = NULL;
fail_alloc_sent_multi_addr_filter_start:
    link->sent_multi_addr_filter_count = 0;
    free(buf);
fail_alloc_msg_buf:
fail_invalid_conn_flag:
fail_no_link:
    PROTOCOL_PRINT_ERROR2("bnep_filter_multi_addr_set: failed %d, bd_addr %s",
                          -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bnep_filter_multi_addr_rsp(uint8_t  bd_addr[6],
                                uint16_t rsp_msg)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t buf[2];

        BE_UINT16_TO_ARRAY(buf, rsp_msg);
        return bnep_control_packet_send(link,
                                        BNEP_CONTROL_TYPE_FILTER_MULTI_ADDR_RESPONSE,
                                        buf,
                                        sizeof(uint16_t));
    }

    return false;
}

static bool bnep_protocol_filter(T_BNEP_LINK *link,
                                 uint16_t     protocol_type)
{
    uint16_t i;

    if (link->rcvd_net_type_filter_count == 0)
    {
        return true;
    }

    for (i = 0; i < link->rcvd_net_type_filter_count; i++)
    {
        if ((protocol_type >= link->rcvd_net_type_filter_start[i]) &&
            (protocol_type <= link->rcvd_net_type_filter_end[i]))
        {
            return true;
        }
    }

    return false;
}

static bool bnep_multicast_filter(T_BNEP_LINK *link,
                                  uint8_t      bd_addr[6])
{
    if (bd_addr[0] & 0x01)
    {
        if (link->rcvd_multi_addr_filter_count)
        {
            uint16_t i;

            for (i = 0; i < link->rcvd_multi_addr_filter_count; i++)
            {
                if ((memcmp(bd_addr, link->rcvd_multi_addr_filter_start[i], BNEP_ADDR_SIZE) >= 0) &&
                    (memcmp(bd_addr, link->rcvd_multi_addr_filter_end[i], BNEP_ADDR_SIZE) <= 0))
                {
                    return true;
                }
            }

            return false;
        }
    }

    return true;
}

bool bnep_send(uint8_t   bd_addr[6],
               uint8_t  *buf,
               uint16_t  len)
{
    T_BNEP_LINK  *link;
    uint8_t      *p;
    uint8_t      *bnep_buf;
    uint16_t      bnep_buf_len;
    uint8_t       addr_source[6];
    uint8_t       addr_dest[6];
    uint16_t      protocol_type;
    uint16_t      payload_len;
    uint8_t       has_source  = 0;
    uint8_t       has_dest = 0;
    int32_t       ret = 0;

    link = bnep_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (!(link->conn_flag & BNEP_FLAG_CONN_COMPLETED))
    {
        ret = 2;
        goto fail_invalid_conn_flag;
    }

    p = buf;
    STREAM_TO_ARRAY(addr_dest, p, BNEP_ADDR_SIZE);
    STREAM_TO_ARRAY(addr_source, p, BNEP_ADDR_SIZE);
    BE_STREAM_TO_UINT16(protocol_type, p);

    payload_len = len - 2 * BNEP_ADDR_SIZE - BNEP_NETWORK_PROTOCOL_TYPE_SIZE;
    if (protocol_type == BNEP_802_1_Q_PROTOCOL)
    {
        if (payload_len < BNEP_802_1_Q_PAYLOAD_LEN)
        {
            ret = 3;
            goto fail_invalid_802_1_Q_payload_len;
        }

        payload_len = BNEP_802_1_Q_PAYLOAD_LEN;
        STREAM_SKIP_LEN(p, BNEP_802_1_Q_TCI_SIZE);
        BE_STREAM_TO_UINT16(protocol_type, p);
    }

    if (!bnep_protocol_filter(link, protocol_type) ||
        !bnep_multicast_filter(link, addr_dest))
    {
        if (protocol_type != BNEP_802_1_Q_PROTOCOL)
        {
            ret = 4;
            goto fail_filtered_out;
        }
    }

    if (memcmp(addr_dest, link->remote_addr, BNEP_ADDR_SIZE))
    {
        has_dest = 1;
    }

    if (memcmp(addr_source, link->local_addr, BNEP_ADDR_SIZE))
    {
        has_source = 1;
    }

    bnep_buf_len = BNEP_TYPE_SIZE + has_dest * BNEP_ADDR_SIZE + has_source * BNEP_ADDR_SIZE
                   + BNEP_NETWORK_PROTOCOL_TYPE_SIZE + payload_len;

    if (bnep_buf_len > link->remote_mtu)
    {
        ret = 5;
        goto fail_invalid_len;
    }

    bnep_buf = mpa_get_l2c_buf(bnep->queue_id,
                               link->cid,
                               0,
                               bnep_buf_len,
                               link->ds_data_offset,
                               false);
    if (bnep_buf == NULL)
    {
        ret = 6;
        goto fail_get_l2c_buf;
    }

    p = &bnep_buf[link->ds_data_offset];

    if (has_source && has_dest)
    {
        BE_UINT8_TO_STREAM(p, BNEP_PKT_TYPE_GENERAL_ETHERNET);
    }
    else if (has_source && !has_dest)
    {
        BE_UINT8_TO_STREAM(p, BNEP_PKT_TYPE_COMPRESSED_ETHERNET_SOURCE_ONLY);
    }
    else if (!has_source && has_dest)
    {
        BE_UINT8_TO_STREAM(p, BNEP_PKT_TYPE_COMPRESSED_ETHERNET_DEST_ONLY);
    }
    else
    {
        BE_UINT8_TO_STREAM(p, BNEP_PKT_TYPE_COMPRESSED_ETHERNET);
    }

    if (has_dest)
    {
        ARRAY_TO_STREAM(p, addr_dest, BNEP_ADDR_SIZE);
    }

    if (has_source)
    {
        ARRAY_TO_STREAM(p, addr_source, BNEP_ADDR_SIZE);
    }

    BE_UINT16_TO_STREAM(p, protocol_type);

    memcpy(p, buf + 2 * BNEP_ADDR_SIZE + BNEP_NETWORK_PROTOCOL_TYPE_SIZE, payload_len);

    mpa_send_l2c_data_req(bnep_buf,
                          link->ds_data_offset,
                          link->cid,
                          bnep_buf_len,
                          false);

    return true;

fail_get_l2c_buf:
fail_invalid_len:
fail_filtered_out:
fail_invalid_802_1_Q_payload_len:
fail_invalid_conn_flag:
fail_invalid_addr:
    PROTOCOL_PRINT_ERROR2("bnep_send: failed %d, bd_addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bnep_connect_req(uint8_t  local_addr[6],
                      uint8_t  remote_addr[6],
                      uint16_t src_uuid,
                      uint16_t dst_uuid)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_addr(remote_addr);
    if (link == NULL)
    {
        link = bnep_alloc_link(remote_addr);
        if (link != NULL)
        {
            link->state     = BNEP_STATE_CONNECTING;
            link->conn_flag = BNEP_FLAG_IS_INITIATOR;
            link->src_uuid  = src_uuid;
            link->dst_uuid  = dst_uuid;
            memcpy(link->local_addr, local_addr, 6);
            mpa_send_l2c_conn_req(PSM_BNEP, UUID_BNEP, bnep->queue_id, BNEP_MTU,
                                  remote_addr, MPA_L2C_MODE_BASIC, 0xFFFF);
            return true;
        }
    }
    else
    {
        if (link->state == BNEP_STATE_CONNECTED)
        {
            return bnep_setup_connection_req(remote_addr, src_uuid, dst_uuid);
        }
    }

    return false;
}

bool bnep_connect_cfm(uint8_t local_addr[6],
                      uint8_t remote_addr[6],
                      bool    accept)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_addr(remote_addr);
    if (link != NULL)
    {
        if (accept)
        {
            memcpy(link->local_addr, local_addr, 6);
            mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT,
                                  link->cid,
                                  BNEP_MTU,
                                  MPA_L2C_MODE_BASIC,
                                  0xFFFF);
        }
        else
        {
            mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                                  link->cid,
                                  BNEP_MTU,
                                  MPA_L2C_MODE_BASIC,
                                  0xFFFF);
            bnep_free_link(link);
        }

        return true;
    }

    return false;
}

bool bnep_disconnect_req(uint8_t bd_addr[6])
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        link->state = BNEP_STATE_DISCONNECTING;
        mpa_send_l2c_disconn_req(link->cid);
        return true;
    }

    return false;
}

uint8_t *bnep_handle_connection_req(T_BNEP_LINK *link,
                                    uint8_t     *buf,
                                    uint16_t     len)
{
    uint8_t               *p;
    uint8_t                uuid_size;
    uint16_t               pkt_len;
    T_BNEP_SETUP_CONN_IND  conn_ind;
    int32_t                ret = 0;

    p = buf;
    BE_STREAM_TO_UINT8(uuid_size, p);

    if (uuid_size != 2 &&
        uuid_size != 4 &&
        uuid_size != 16)
    {
        ret = 1;
        p = NULL;
        link->conn_flag |= BNEP_FLAG_SETUP_RCVD;
        bnep_setup_connection_rsp(link->remote_addr, BNEP_RSP_SETUP_INVALID_SERVICE_UUID_SIZE);
        goto fail_invalid_uuid_size;
    }

    pkt_len = sizeof(uuid_size) + 2 * uuid_size;
    if (len < pkt_len)
    {
        ret = 2;
        p = NULL;
        goto fail_invalid_len;
    }

    if (link->state != BNEP_STATE_CONNECTED)
    {
        ret = 3;
        p = NULL;
        goto fail_invalid_state;
    }

    if (link->conn_flag & (BNEP_FLAG_SETUP_RCVD | BNEP_FLAG_SETUP_SENT))
    {
        ret = 4;
        STREAM_SKIP_LEN(p, 2 * uuid_size);
        goto fail_invalid_conn_flag;
    }

    link->conn_flag |= BNEP_FLAG_SETUP_RCVD;

    if (link->conn_flag & BNEP_FLAG_IS_INITIATOR &&
        !(link->conn_flag & BNEP_FLAG_CONN_COMPLETED))
    {
        ret = 5;
        STREAM_SKIP_LEN(p, 2 * uuid_size);
        bnep_setup_connection_rsp(link->remote_addr, BNEP_RSP_SETUP_CONNECTION_NOT_ALLOWED);
        goto fail_not_allowed;
    }

    if (link->conn_flag & BNEP_FLAG_CONN_COMPLETED)
    {
        link->prev_src_uuid = link->src_uuid;
        link->prev_dst_uuid = link->dst_uuid;
    }

    BE_STREAM_TO_UINT16(link->dst_uuid, p);
    STREAM_SKIP_LEN(p, uuid_size - sizeof(uint16_t));
    BE_STREAM_TO_UINT16(link->src_uuid, p);
    STREAM_SKIP_LEN(p, uuid_size - sizeof(uint16_t));

    if (link->dst_uuid != UUID_PANU &&
        link->dst_uuid != UUID_NAP &&
        link->dst_uuid != UUID_GN)
    {
        ret = 6;
        bnep_setup_connection_rsp(link->remote_addr, BNEP_RSP_SETUP_INVALID_DEST_UUID);
        goto fail_invalid_dst_uuid;
    }

    if (link->src_uuid != UUID_PANU &&
        link->src_uuid != UUID_NAP &&
        link->src_uuid != UUID_GN)
    {
        ret = 7;
        bnep_setup_connection_rsp(link->remote_addr, BNEP_RSP_SETUP_INVALID_SOURCE_UUID);
        goto fail_invalid_src_uuid;
    }

    if (link->src_uuid != UUID_PANU &&
        link->dst_uuid != UUID_PANU)
    {
        ret = 8;
        bnep_setup_connection_rsp(link->remote_addr, BNEP_RSP_SETUP_INVALID_SOURCE_UUID);
        goto fail_invalid_src_uuid;
    }

    if ((link->conn_flag & BNEP_FLAG_CONN_COMPLETED) &&
        link->src_uuid == link->prev_src_uuid &&
        link->dst_uuid == link->prev_dst_uuid)
    {
        bnep_setup_connection_rsp(link->remote_addr, BNEP_RSP_SETUP_SUCCESS);
        return p;
    }

    conn_ind.dst_uuid = link->dst_uuid;
    conn_ind.src_uuid = link->src_uuid;
    bnep->cback(link->cid, BNEP_MSG_SETUP_CONN_IND, &conn_ind);
    return p;

fail_invalid_src_uuid:
fail_invalid_dst_uuid:
fail_not_allowed:
fail_invalid_conn_flag:
fail_invalid_state:
fail_invalid_len:
fail_invalid_uuid_size:
    PROTOCOL_PRINT_ERROR2("bnep_handle_connection_req: failed %d, bd_addr %s",
                          -ret, TRACE_BDADDR(link->remote_addr));
    return p;
}

uint8_t *bnep_handle_connection_rsp(T_BNEP_LINK *link,
                                    uint8_t     *buf,
                                    uint16_t     len)
{
    uint8_t               *p;
    uint16_t               rsp_msg;
    T_BNEP_SETUP_CONN_RSP  conn_rsp;
    int32_t                ret = 0;

    if (len < sizeof(rsp_msg))
    {
        ret = 1;
        p = NULL;
        goto fail_invalid_len;
    }

    if (link->state != BNEP_STATE_CONNECTED)
    {
        ret = 2;
        p = NULL;
        goto fail_invalid_state;
    }

    p = buf;
    BE_STREAM_TO_UINT16(rsp_msg, p);

    if (!(link->conn_flag & BNEP_FLAG_SETUP_SENT))
    {
        ret = 3;
        goto fail_invalid_conn_flag;
    }

    if (link->setup_conn_timer != NULL)
    {
        sys_timer_delete(link->setup_conn_timer);
        link->setup_conn_timer = NULL;
    }

    link->setup_conn_retry_count = 0;

    if (rsp_msg == BNEP_RSP_SETUP_SUCCESS)
    {
        link->conn_flag |= BNEP_FLAG_CONN_COMPLETED;
        link->conn_flag &= ~BNEP_FLAG_SETUP_SENT;
    }
    else
    {
        if (link->conn_flag & BNEP_FLAG_CONN_COMPLETED)
        {
            link->conn_flag &= ~BNEP_FLAG_SETUP_SENT;
            link->src_uuid = link->prev_src_uuid;
            link->dst_uuid = link->prev_dst_uuid;
        }
        else
        {
            mpa_send_l2c_disconn_req(link->cid);
            link->state = BNEP_STATE_DISCONNECTING;
        }
    }

    conn_rsp.rsp_msg = rsp_msg;
    bnep->cback(link->cid, BNEP_MSG_SETUP_CONN_RSP, &conn_rsp);

    return p;

fail_invalid_conn_flag:
fail_invalid_state:
fail_invalid_len:
    PROTOCOL_PRINT_ERROR2("bnep_handle_connection_rsp: failed %d, bd_addr %s",
                          -ret, TRACE_BDADDR(link->remote_addr));
    return p;
}

uint8_t *bnep_handle_filter_net_type_set(T_BNEP_LINK *link,
                                         uint8_t     *buf,
                                         uint16_t     len)
{
    uint8_t                        *p;
    uint16_t                        list_length;
    uint16_t                        start;
    uint16_t                        end;
    uint16_t                        i;
    uint16_t                        pkt_len;
    T_BNEP_FILTER_NET_TYPE_SET_IND  set_ind;
    uint16_t                        filter_num;
    int32_t                         ret = 0;

    if (len < sizeof(list_length))
    {
        ret = 1;
        p = NULL;
        goto fail_invalid_param;
    }

    p = buf;
    BE_STREAM_TO_UINT16(list_length, p);
    pkt_len = sizeof(list_length) + list_length;
    if (len < pkt_len)
    {
        ret = 2;
        p = NULL;
        goto fail_invalid_list_len;
    }

    if (!(link->conn_flag & BNEP_FLAG_CONN_COMPLETED))
    {
        ret = 3;
        goto fail_invalid_conn_flag;
    }

    if (list_length % 4 != 0)
    {
        ret = 4;
        goto fail_invalid_list_length;
    }

    filter_num = list_length >> 2;

    if (filter_num > BNEP_MAX_NET_TYPE_FILTERS)
    {
        ret = 5;
        bnep_filter_net_type_rsp(link->remote_addr, BNEP_RSP_FILTER_ERR_TOO_MANY_FILTERS);
        goto fail_too_many_filters;
    }

    for (i = 0; i < filter_num; i++)
    {
        BE_ARRAY_TO_UINT16(start, p + i * 2 * BNEP_NETWORK_PROTOCOL_TYPE_SIZE);
        BE_ARRAY_TO_UINT16(end, p + (2 * i + 1) * BNEP_NETWORK_PROTOCOL_TYPE_SIZE);

        if (start > end)
        {
            ret = 6;
            bnep_filter_net_type_rsp(link->remote_addr, BNEP_RSP_FILTER_ERR_INVALID_RANGE);
            goto fail_invalid_range;
        }
    }

    link->rcvd_net_type_filter_count = filter_num;
    link->rcvd_net_type_filter_start = malloc(filter_num * BNEP_NETWORK_PROTOCOL_TYPE_SIZE);
    if (link->rcvd_net_type_filter_start == NULL)
    {
        ret = 7;
        goto fail_alloc_rcvd_net_type_filter_start;
    }

    link->rcvd_net_type_filter_end = malloc(filter_num * BNEP_NETWORK_PROTOCOL_TYPE_SIZE);
    if (link->rcvd_net_type_filter_end == NULL)
    {
        ret = 8;
        goto fail_alloc_rcvd_net_type_filter_end;
    }

    for (i = 0; i < filter_num; i++)
    {
        BE_STREAM_TO_UINT16(link->rcvd_net_type_filter_start[i], p);
        BE_STREAM_TO_UINT16(link->rcvd_net_type_filter_end[i], p);
    }

    set_ind.filter_num = filter_num;
    set_ind.filter_start = link->rcvd_net_type_filter_start;
    set_ind.filter_end = link->rcvd_net_type_filter_end;
    bnep->cback(link->cid, BNEP_MSG_FILTER_NET_TYPE_SET_IND, &set_ind);

    bnep_filter_net_type_rsp(link->remote_addr, BNEP_RSP_FILTER_SUCCESS);
    return p;

fail_alloc_rcvd_net_type_filter_end:
    free(link->rcvd_net_type_filter_start);
    link->rcvd_net_type_filter_start = NULL;
fail_alloc_rcvd_net_type_filter_start:
    link->rcvd_net_type_filter_count = 0;
fail_invalid_range:
fail_too_many_filters:
fail_invalid_list_length:
fail_invalid_conn_flag:
    STREAM_SKIP_LEN(p, list_length);
fail_invalid_list_len:
fail_invalid_param:
    PROTOCOL_PRINT_ERROR2("bnep_handle_filter_net_type_set: failed %d, bd_addr %s",
                          -ret, TRACE_BDADDR(link->remote_addr));
    return p;
}

uint8_t *bnep_handle_filter_net_type_rsp(T_BNEP_LINK *link,
                                         uint8_t     *buf,
                                         uint16_t     len)
{
    uint8_t                        *p;
    uint16_t                        rsp_msg;
    T_BNEP_FILTER_NET_TYPE_SET_RSP  set_rsp;
    int32_t                         ret = 0;

    if (len < sizeof(rsp_msg))
    {
        ret = 1;
        p = NULL;
        goto fail_invalid_len;
    }

    p = buf;
    BE_STREAM_TO_UINT16(rsp_msg, p);

    if (!(link->conn_flag & (BNEP_FLAG_CONN_COMPLETED | BNEP_FLAG_NET_TYPE_SENT)))
    {
        ret = 2;
        goto fail_invalid_conn_flag;
    }

    if (link->net_type_filter_timer != NULL)
    {
        sys_timer_delete(link->net_type_filter_timer);
        link->net_type_filter_timer = NULL;
    }

    link->net_type_filter_retry_count = 0;
    link->conn_flag &= ~BNEP_FLAG_NET_TYPE_SENT;
    if (link->sent_net_type_filter_start != NULL)
    {
        free(link->sent_net_type_filter_start);
        link->sent_net_type_filter_start = NULL;
    }

    if (link->sent_net_type_filter_end != NULL)
    {
        free(link->sent_net_type_filter_end);
        link->sent_net_type_filter_end = NULL;
    }

    set_rsp.rsp_msg = rsp_msg;
    bnep->cback(link->cid, BNEP_MSG_FILTER_NET_TYPE_SET_RSP, &set_rsp);
    return p;

fail_invalid_conn_flag:
fail_invalid_len:
    PROTOCOL_PRINT_ERROR2("bnep_handle_filter_net_type_rsp: failed %d, bd_addr %s",
                          -ret, TRACE_BDADDR(link->remote_addr));
    return p;
}

uint8_t *bnep_handle_filter_multi_addr_set(T_BNEP_LINK *link,
                                           uint8_t     *buf,
                                           uint16_t     len)
{
    uint8_t                         *p;
    uint16_t                         list_length;
    uint16_t                         pkt_len;
    T_BNEP_FILTER_MULTI_ADDR_SET_IND set_ind;
    uint16_t                         filter_num;
    uint16_t                         i;
    int32_t                          ret = 0;

    if (len < sizeof(list_length))
    {
        ret = 1;
        pkt_len = 0;
        p = NULL;
        goto fail_invalid_param;
    }

    p = buf;
    BE_STREAM_TO_UINT16(list_length, p);
    pkt_len = sizeof(list_length) + list_length;
    if (len < pkt_len)
    {
        ret = 2;
        p = NULL;
        goto fail_invalid_list_len;
    }

    if (!(link->conn_flag & BNEP_FLAG_CONN_COMPLETED))
    {
        ret = 3;
        goto fail_invalid_conn_flag;
    }

    if (list_length % 12 != 0)
    {
        ret = 4;
        bnep_filter_multi_addr_rsp(link->remote_addr, BNEP_RSP_FILTER_ERR_INVALID_RANGE);
        goto fail_invalid_list_length;
    }

    filter_num = list_length / 12;
    if (filter_num > BNEP_MAX_MULTI_ADDR_FILTERS)
    {
        ret = 5;
        bnep_filter_multi_addr_rsp(link->remote_addr, BNEP_RSP_FILTER_ERR_INVALID_RANGE);
        goto fail_too_many_filters;
    }

    for (i = 0; i < filter_num; i++)
    {
        if (memcmp(p + 2 * i * BNEP_ADDR_SIZE, p + (2 * i + 1) * BNEP_ADDR_SIZE, BNEP_ADDR_SIZE) > 0)
        {
            ret = 6;
            bnep_filter_multi_addr_rsp(link->remote_addr, BNEP_RSP_FILTER_ERR_INVALID_RANGE);
            goto fail_invalid_range;
        }
    }

    link->rcvd_multi_addr_filter_count = filter_num;
    link->rcvd_multi_addr_filter_start = malloc(filter_num * BNEP_ADDR_SIZE);
    if (link->rcvd_multi_addr_filter_start == NULL)
    {
        ret = 7;
        goto fail_alloc_rcvd_multi_addr_filter_start;
    }

    link->rcvd_multi_addr_filter_end = malloc(filter_num * BNEP_ADDR_SIZE);
    if (link->rcvd_multi_addr_filter_end == NULL)
    {
        ret = 8;
        goto fail_alloc_rcvd_multi_addr_filter_end;
    }

    for (i = 0; i < filter_num; i++)
    {
        STREAM_TO_ARRAY(link->rcvd_multi_addr_filter_start[i], p, BNEP_ADDR_SIZE);
        STREAM_TO_ARRAY(link->rcvd_multi_addr_filter_end[i], p, BNEP_ADDR_SIZE);
    }

    set_ind.filter_num = filter_num;
    set_ind.filter_start = link->rcvd_multi_addr_filter_start;
    set_ind.filter_end = link->rcvd_multi_addr_filter_end;
    bnep->cback(link->cid, BNEP_MSG_FILTER_MULTI_ADDR_SET_IND, &set_ind);

    bnep_filter_multi_addr_rsp(link->remote_addr, BNEP_RSP_FILTER_SUCCESS);
    return p;

fail_alloc_rcvd_multi_addr_filter_end:
    free(link->rcvd_multi_addr_filter_start);
    link->rcvd_multi_addr_filter_start = NULL;
fail_alloc_rcvd_multi_addr_filter_start:
    link->rcvd_multi_addr_filter_count = 0;
fail_invalid_range:
fail_too_many_filters:
fail_invalid_list_length:
fail_invalid_conn_flag:
    STREAM_SKIP_LEN(p, list_length);
fail_invalid_list_len:
fail_invalid_param:
    PROTOCOL_PRINT_ERROR2("bnep_handle_filter_multi_addr_set: failed %d, bd_addr %s",
                          -ret, TRACE_BDADDR(link->remote_addr));
    return p;
}

uint8_t *bnep_handle_filter_multi_addr_rsp(T_BNEP_LINK *link,
                                           uint8_t     *buf,
                                           uint16_t     len)
{
    uint8_t                          *p;
    uint16_t                          rsp_msg;
    T_BNEP_FILTER_MULTI_ADDR_SET_RSP  set_rsp;
    int32_t                           ret = 0;

    if (len < sizeof(rsp_msg))
    {
        ret = 1;
        p = NULL;
        goto fail_invalid_len;
    }

    p = buf;
    BE_STREAM_TO_UINT16(rsp_msg, p);

    if (!(link->conn_flag & (BNEP_FLAG_CONN_COMPLETED | BNEP_FLAG_MULTI_ADDR_SENT)))
    {
        ret = 2;
        goto fail_invalid_conn_flag;
    }

    if (link->multi_addr_filter_timer != NULL)
    {
        sys_timer_delete(link->multi_addr_filter_timer);
        link->multi_addr_filter_timer = NULL;
    }

    link->multi_addr_filter_retry_count = 0;
    link->conn_flag &= ~BNEP_FLAG_MULTI_ADDR_SENT;

    if (link->sent_multi_addr_filter_start != NULL)
    {
        free(link->sent_multi_addr_filter_start);
        link->sent_multi_addr_filter_start = NULL;
    }

    if (link->sent_multi_addr_filter_end != NULL)
    {
        free(link->sent_multi_addr_filter_end);
        link->sent_multi_addr_filter_end = NULL;
    }

    set_rsp.rsp_msg = rsp_msg;
    bnep->cback(link->cid, BNEP_MSG_FILTER_MULTI_ADDR_SET_RSP, &set_rsp);
    return p;

fail_invalid_conn_flag:
fail_invalid_len:
    PROTOCOL_PRINT_ERROR2("bnep_handle_filter_multi_addr_rsp: failed %d, bd_addr %s",
                          -ret, TRACE_BDADDR(link->remote_addr));
    return p;
}

uint8_t *bnep_handle_control_packet(T_BNEP_LINK *link,
                                    uint8_t     *buf,
                                    uint16_t     len,
                                    bool         is_ext)
{
    if (len > 0)
    {
        uint8_t *p;
        uint8_t  control_type;

        p = buf;
        BE_STREAM_TO_UINT8(control_type, p);
        len--;

        switch (control_type)
        {
        case BNEP_CONTROL_TYPE_COMMAND_NOT_UNDERSTOOD:
            if (len >= 1)
            {
                STREAM_SKIP_LEN(p, 1);
            }
            else
            {
                return NULL;
            }
            break;

        case BNEP_CONTROL_TYPE_SETUP_CONNECTION_REQUEST:
            if (is_ext)
            {
                STREAM_SKIP_LEN(p, len);
            }
            else
            {
                p = bnep_handle_connection_req(link, p, len);
            }
            break;

        case BNEP_CONTROL_TYPE_SETUP_CONNECTION_RESPONSE:
            if (is_ext)
            {
                STREAM_SKIP_LEN(p, len);
            }
            else
            {
                p = bnep_handle_connection_rsp(link, p, len);
            }
            break;

        case BNEP_CONTROL_TYPE_FILTER_NET_TYPE_SET:
            p = bnep_handle_filter_net_type_set(link, p, len);
            break;

        case BNEP_CONTROL_TYPE_FILTER_NET_TYPE_RESPONSE:
            p = bnep_handle_filter_net_type_rsp(link, p, len);
            break;

        case BNEP_CONTROL_TYPE_FILTER_MULTI_ADDR_SET:
            p = bnep_handle_filter_multi_addr_set(link, p, len);
            break;

        case BNEP_CONTROL_TYPE_FILTER_MULTI_ADDR_RESPONSE:
            p = bnep_handle_filter_multi_addr_rsp(link, p, len);
            break;

        default:
            bnep_control_command_not_understood_send(link->remote_addr, control_type);
            if (is_ext)
            {
                STREAM_SKIP_LEN(p, len);
            }
            else
            {
                p = NULL;
            }
            break;
        }

        return p;
    }

    return NULL;
}

void bnep_handle_ethernet_packet(T_BNEP_LINK *link,
                                 uint8_t      addr_dest[6],
                                 uint8_t      addr_source[6],
                                 uint16_t     protocol_type,
                                 uint8_t     *buf,
                                 uint16_t     len)
{
    uint8_t *pkt;

    pkt = malloc(BNEP_NETWORK_PROTOCOL_TYPE_SIZE + 2 * BNEP_ADDR_SIZE + len);
    if (pkt != NULL)
    {
        T_BNEP_ETHERNET_PACKET_IND  packet_ind;
        uint8_t                    *p;

        p = pkt;
        ARRAY_TO_STREAM(p, addr_dest, BNEP_ADDR_SIZE);
        ARRAY_TO_STREAM(p, addr_source, BNEP_ADDR_SIZE);
        BE_UINT16_TO_STREAM(p, protocol_type);
        memcpy(p, buf, len);
        packet_ind.buf = pkt;
        packet_ind.len = BNEP_NETWORK_PROTOCOL_TYPE_SIZE + 2 * BNEP_ADDR_SIZE + len;
        bnep->cback(link->cid, BNEP_MSG_ETHERNET_PACKET_IND, &packet_ind);
        free(pkt);
    }
}

void bnep_handle_conn_ind(T_MPA_L2C_CONN_IND *conn_ind)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_addr(conn_ind->bd_addr);
    if (link == NULL)
    {
        link = bnep_alloc_link(conn_ind->bd_addr);
        if (link != NULL)
        {
            link->cid   = conn_ind->cid;
            link->state = BNEP_STATE_CONNECTING;
            bnep->cback(link->cid, BNEP_MSG_CONN_IND, conn_ind->bd_addr);
        }
        else
        {
            mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                                  conn_ind->cid,
                                  BNEP_MTU,
                                  MPA_L2C_MODE_BASIC,
                                  0xFFFF);
        }
    }
    else
    {
        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                              conn_ind->cid,
                              BNEP_MTU,
                              MPA_L2C_MODE_BASIC,
                              0xFFFF);
    }
}

void bnep_handle_conn_rsp(T_MPA_L2C_CONN_RSP *conn_rsp)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_addr(conn_rsp->bd_addr);
    if (link != NULL)
    {
        if (conn_rsp->cause == 0)
        {
            if (link->state == BNEP_STATE_CONNECTING)
            {
                link->cid = conn_rsp->cid;
                bnep->cback(conn_rsp->cid, BNEP_MSG_CONN_RSP, conn_rsp);
            }
        }
        else
        {
            if (link->state == BNEP_STATE_CONNECTING)
            {
                uint16_t info;

                info = conn_rsp->cause;
                bnep_free_link(link);
                bnep->cback(conn_rsp->cid, BNEP_MSG_CONN_FAIL, &info);
            }
        }
    }
}

void bnep_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *conn_cmpl)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_cid(conn_cmpl->cid);
    if (link != NULL)
    {
        if (conn_cmpl->cause == 0)
        {
            link->state          = BNEP_STATE_CONNECTED;
            link->remote_mtu     = conn_cmpl->remote_mtu;
            link->ds_data_offset = conn_cmpl->ds_data_offset;
            bnep->cback(link->cid, BNEP_MSG_CONN_CMPL, conn_cmpl);
            if (link->conn_flag & BNEP_FLAG_IS_INITIATOR)
            {
                bnep_setup_connection_req(link->remote_addr, link->src_uuid, link->dst_uuid);
            }
        }
        else
        {
            if (link->state == BNEP_STATE_CONNECTING)
            {
                uint16_t info;

                info = conn_cmpl->cause;
                bnep_free_link(link);
                bnep->cback(conn_cmpl->cid, BNEP_MSG_CONN_FAIL, &info);
            }
        }
    }
}

void bnep_handle_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_BNEP_LINK *link;
    uint8_t     *data;
    uint8_t     *p;
    uint16_t     len;
    uint8_t      bnep_type;
    uint8_t      ext_flag;
    uint8_t      ext_type;
    uint8_t      ext_len;
    uint8_t      addr_dest[6];
    uint8_t      addr_source[6];
    uint16_t     protocol_type;
    int32_t      ret = 0;

    link = bnep_find_link_by_cid(data_ind->cid);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_cid;
    }

    data = data_ind->data + data_ind->gap;
    p    = data;
    len  = data_ind->length;
    BE_STREAM_TO_UINT8(bnep_type, p);
    ext_flag   = bnep_type >> 7;
    bnep_type &= 0x7F;

    if (len - (p - data) < bnep_frame_hdr_size[bnep_type])
    {
        ret = 2;
        goto fail_invalid_len;
    }

    switch (bnep_type)
    {
    case BNEP_PKT_TYPE_GENERAL_ETHERNET:
        STREAM_TO_ARRAY(addr_dest, p, 6);
        STREAM_TO_ARRAY(addr_source, p, 6);
        BE_STREAM_TO_UINT16(protocol_type, p);
        break;

    case BNEP_PKT_TYPE_COMPRESSED_ETHERNET:
        memcpy(addr_dest, link->remote_addr, 6);
        memcpy(addr_source, link->local_addr, 6);
        BE_STREAM_TO_UINT16(protocol_type, p);
        break;

    case BNEP_PKT_TYPE_COMPRESSED_ETHERNET_SOURCE_ONLY:
        memcpy(addr_dest, link->remote_addr, 6);
        STREAM_TO_ARRAY(addr_source, p, 6);
        BE_STREAM_TO_UINT16(protocol_type, p);
        break;

    case BNEP_PKT_TYPE_COMPRESSED_ETHERNET_DEST_ONLY:
        STREAM_TO_ARRAY(addr_dest, p, 6);
        memcpy(addr_source, link->local_addr, 6);
        BE_STREAM_TO_UINT16(protocol_type, p);
        break;

    case BNEP_PKT_TYPE_CONTROL:
        p = bnep_handle_control_packet(link, p, len - (p - data), false);
        if (p == NULL)
        {
            ret = 3;
            goto fail_invalid_control_pkt;
        }
        break;

    default:
        ret = 4;
        goto fail_unknown_type;
    }

    while (ext_flag && p && p - data + 2 <= len)
    {
        BE_STREAM_TO_UINT8(ext_type, p);
        ext_flag  = ext_type >> 7;
        ext_type &= 0x7F;
        BE_STREAM_TO_UINT8(ext_len, p);

        if (ext_len > len - (p - data))
        {
            ret = 5;
            goto fail_invalid_ext_len;
        }

        if (ext_type != BNEP_EXT_HEADER_TYPE_EXTENSION_CONTROL)
        {
            STREAM_SKIP_LEN(p, ext_len);
        }
        else
        {
            p = bnep_handle_control_packet(link, p, ext_len, true);
        }
    }

    if (bnep_type != BNEP_PKT_TYPE_CONTROL)
    {
        if (link->conn_flag & BNEP_FLAG_CONN_COMPLETED)
        {
            if (p != NULL)
            {
                bnep_handle_ethernet_packet(link, addr_dest, addr_source,
                                            protocol_type, p, len - (p - data));
            }
        }
    }

    return;

fail_invalid_ext_len:
fail_unknown_type:
fail_invalid_control_pkt:
fail_invalid_len:
fail_invalid_cid:
    PROTOCOL_PRINT_ERROR1("bnep_handle_data_ind: failed %d", -ret);
}

void bnep_handle_disconn_ind(T_MPA_L2C_DISCONN_IND *disconn_ind)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_cid(disconn_ind->cid);
    if (link != NULL)
    {
        bnep_free_link(link);
    }

    mpa_send_l2c_disconn_cfm(disconn_ind->cid);
    bnep->cback(disconn_ind->cid, BNEP_MSG_DISCONN_CMPL, &disconn_ind->cause);
}

void bnep_handle_disconn_rsp(T_MPA_L2C_DISCONN_RSP *disconn_rsp)
{
    T_BNEP_LINK *link;

    link = bnep_find_link_by_cid(disconn_rsp->cid);
    if (link != NULL)
    {
        bnep_free_link(link);
        bnep->cback(disconn_rsp->cid, BNEP_MSG_DISCONN_CMPL, &disconn_rsp->cause);
    }
}

void bnep_l2c_cback(void        *p_buf,
                    T_PROTO_MSG  msg)
{
    switch (msg)
    {
    case L2C_CONN_IND:
        bnep_handle_conn_ind((T_MPA_L2C_CONN_IND *)p_buf);
        break;

    case L2C_CONN_RSP:
        bnep_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)p_buf);
        break;

    case L2C_CONN_CMPL:
        bnep_handle_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)p_buf);
        break;

    case L2C_DATA_IND:
        bnep_handle_data_ind((T_MPA_L2C_DATA_IND *)p_buf);
        break;

    case L2C_DISCONN_IND:
        bnep_handle_disconn_ind((T_MPA_L2C_DISCONN_IND *)p_buf);
        break;

    case L2C_DISCONN_RSP:
        bnep_handle_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)p_buf);
        break;

    default:
        break;
    }
}

bool bnep_init(P_BNEP_CBACK cback)
{
    int32_t ret = 0;

    bnep = calloc(1, sizeof(T_BNEP));
    if (bnep == NULL)
    {
        ret = 1;
        goto fail_alloc_bnep;
    }

    if (mpa_reg_l2c_proto(PSM_BNEP, bnep_l2c_cback, &bnep->queue_id) == false)
    {
        ret = 2;
        goto fail_reg_l2c;
    }

    bnep->cback = cback;

    return true;

fail_reg_l2c:
    free(bnep);
    bnep = NULL;
fail_alloc_bnep:
    PROTOCOL_PRINT_ERROR1("bnep_init: failed %d", -ret);
    return false;
}

void bnep_deinit(void)
{
    if (bnep != NULL)
    {
        T_BNEP_LINK *link;

        link = os_queue_out(&bnep->list);
        while (link != NULL)
        {
            free(link);
            link = os_queue_out(&bnep->list);
        }

        free(bnep);
        bnep = NULL;
    }
}
