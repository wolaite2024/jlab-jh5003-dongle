/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HID_HOST_SUPPORT == 1)

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "mpa.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_hid_host.h"
#include "bt_hid_parser.h"
#include "bt_hid_int.h"

/*hid_initiator_flag*/
#define INITIATE_CONN_MASK       0x01
#define INITIATE_DISCONN_MASK    0x02

typedef struct t_bt_hid_msg_item
{
    struct t_bt_hid_msg_item  *next;
    uint8_t                    msg_type;
    uint8_t                    msg_param;
    uint8_t                    msg_len;
    uint8_t                    msg[0];
} T_BT_HID_MSG_ITEM;

typedef struct t_bt_hid_host_link
{
    struct t_bt_hid_host_link  *next;
    uint8_t                     bd_addr[6];
    uint8_t                     proto_mode;
    uint8_t                     credits;
    uint8_t                     int_flag;
    bool                        virtual_unplug;
    uint16_t                    descriptor_len;
    uint8_t                    *descriptor;
    T_OS_QUEUE                  msg_list;
    T_BT_HID_CONTROL_CHANN      control_chann;
    T_BT_HID_INTERRUPT_CHANN    interrupt_chann;
} T_BT_HID_HOST_LINK;

typedef struct t_bt_hid_host
{
    T_OS_QUEUE           list;
    uint8_t              queue_id_control;
    uint8_t              queue_id_interrupt;
    bool                 boot_proto_mode;
} T_BT_HID_HOST;

static T_BT_HID_HOST *bt_hid_host;

T_BT_HID_HOST_LINK *bt_hid_host_alloc_link(uint8_t bd_addr[6])
{
    T_BT_HID_HOST_LINK *link;

    link = malloc(sizeof(T_BT_HID_HOST_LINK));
    if (link != NULL)
    {
        link->control_chann.state = BT_HID_STATE_DISCONNECTED;
        link->interrupt_chann.state = BT_HID_STATE_DISCONNECTED;
        link->proto_mode = BT_HID_MSG_PARAM_REPORT_PROTOCOL_MODE;
        link->credits = 1;
        link->descriptor = NULL;
        link->descriptor_len = 0;
        link->virtual_unplug = false;
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_init(&link->msg_list);
        os_queue_in(&bt_hid_host->list, link);
    }

    return link;
}

void bt_hid_host_free_link(T_BT_HID_HOST_LINK *link)
{
    if (os_queue_delete(&bt_hid_host->list, link) == true)
    {
        T_BT_HID_MSG_ITEM  *p_item;

        p_item = os_queue_out(&link->msg_list);
        while (p_item != NULL)
        {
            free(p_item);
            p_item = os_queue_out(&link->msg_list);
        }
        free(link);
    }
}

T_BT_HID_HOST_LINK *bt_hid_host_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_HID_HOST_LINK *link;

    link = os_queue_peek(&bt_hid_host->list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

T_BT_HID_HOST_LINK *bt_hid_host_find_link_by_cid(uint16_t cid)
{
    T_BT_HID_HOST_LINK *link;

    link = os_queue_peek(&bt_hid_host->list, 0);
    while (link != NULL)
    {
        if (link->control_chann.cid == cid || link->interrupt_chann.cid == cid)
        {
            break;
        }

        link = link->next;
    }

    return link;
}

bool bt_hid_host_control_msg_req(T_BT_HID_HOST_LINK *link,
                                 uint8_t             msg_type,
                                 uint8_t             msg_param,
                                 uint8_t            *buf,
                                 uint16_t            len)
{
    uint8_t *hid_buf;

    hid_buf = mpa_get_l2c_buf(bt_hid_host->queue_id_control,
                              link->control_chann.cid,
                              0,
                              BT_HID_HDR_LENGTH + len,
                              link->control_chann.ds_data_offset,
                              false);
    if (hid_buf != NULL)
    {
        hid_buf[link->control_chann.ds_data_offset] = (msg_type << 4) | (msg_param & 0x0f);
        memcpy(hid_buf + link->control_chann.ds_data_offset + BT_HID_HDR_LENGTH, buf, len);

        mpa_send_l2c_data_req(hid_buf,
                              link->control_chann.ds_data_offset,
                              link->control_chann.cid,
                              BT_HID_HDR_LENGTH + len,
                              false);

        link->credits = 0;

        return true;
    }

    return false;
}

bool bt_hid_host_control_flush_cmd(T_BT_HID_HOST_LINK *link)
{
    if (link->credits > 0)
    {
        T_BT_HID_MSG_ITEM *p_item;

        p_item = os_queue_peek(&link->msg_list, 0);
        if (p_item != NULL)
        {
            return bt_hid_host_control_msg_req(link,
                                               p_item->msg_type,
                                               p_item->msg_param,
                                               p_item->msg,
                                               p_item->msg_len);
        }
    }

    return false;
}

bool bt_hid_host_control_msg_send(uint8_t  bd_addr[6],
                                  uint8_t  msg_type,
                                  uint8_t  msg_param,
                                  uint8_t *buf,
                                  uint8_t  len)
{
    T_BT_HID_HOST_LINK *link;
    T_BT_HID_MSG_ITEM  *p_item;
    int32_t          ret = 0;

    link = bt_hid_host_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (BT_HID_HDR_LENGTH + len > link->control_chann.remote_mtu)
    {
        ret = 2;
        goto fail_invalid_msg_len;
    }

    if (msg_type == BT_HID_MSG_TYPE_HID_CONTROL &&
        msg_param == BT_HID_MSG_PARAM_CONTROL_VIRTUAL_CABLE_UNPLUG)
    {
        if (bt_hid_host_control_msg_req(link,
                                        msg_type,
                                        msg_param,
                                        buf,
                                        len) == false)
        {
            ret = 3;
            goto fail_send_msg;
        }

        link->virtual_unplug = true;
        return true;
    }

    p_item = malloc(sizeof(T_BT_HID_MSG_ITEM) + len);
    if (p_item == NULL)
    {
        ret = 4;
        goto fail_alloc_item;
    }

    p_item->msg_type = msg_type;
    p_item->msg_param = msg_param;
    p_item->msg_len = len;
    memcpy((uint8_t *)p_item->msg, buf, len);

    os_queue_in(&link->msg_list, p_item);

    return bt_hid_host_control_flush_cmd(link);

fail_alloc_item:
fail_send_msg:
fail_invalid_msg_len:
fail_invalid_addr:
    PROFILE_PRINT_ERROR1("bt_hid_host_control_msg_send: failed %d", -ret);
    return false;
}

bool bt_hid_host_interrupt_msg_send(uint8_t   bd_addr[6],
                                    uint8_t   msg_type,
                                    uint8_t   msg_param,
                                    uint8_t  *buf,
                                    uint16_t  len)
{
    T_BT_HID_HOST_LINK *link;
    uint8_t            *hid_buf;
    int32_t             ret = 0;

    link = bt_hid_host_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (link->interrupt_chann.state != BT_HID_STATE_CONNECTED)
    {
        ret = 2;
        goto fail_state_not_connected;
    }

    if ((BT_HID_HDR_LENGTH + len) > link->interrupt_chann.remote_mtu)
    {
        ret = 3;
        goto fail_invalid_len;
    }

    hid_buf = mpa_get_l2c_buf(bt_hid_host->queue_id_interrupt,
                              link->interrupt_chann.cid,
                              0,
                              BT_HID_HDR_LENGTH + len,
                              link->interrupt_chann.ds_data_offset,
                              false);
    if (hid_buf == NULL)
    {
        ret = 4;
        goto fail_get_l2c_buf;
    }

    hid_buf[link->interrupt_chann.ds_data_offset] = (msg_type << 4) | (msg_param & 0x0f);
    memcpy(hid_buf + link->interrupt_chann.ds_data_offset + BT_HID_HDR_LENGTH, buf, len);

    mpa_send_l2c_data_req(hid_buf,
                          link->interrupt_chann.ds_data_offset,
                          link->interrupt_chann.cid,
                          BT_HID_HDR_LENGTH + len,
                          false);

    return true;

fail_get_l2c_buf:
fail_invalid_len:
fail_state_not_connected:
fail_invalid_addr:
    PROFILE_PRINT_ERROR1("hid_host_interrupt_msg_send: failed %d", -ret);
    return false;
}

bool bt_hid_host_report_size_check(T_BT_HID_HOST_LINK  *link,
                                   uint16_t             report_id,
                                   T_BT_HID_REPORT_TYPE report_type,
                                   uint16_t             report_size)
{
    if (link->proto_mode == BT_HID_MSG_PARAM_BOOT_PROTOCOL_MODE)
    {
        switch (report_id)
        {
        case BT_HID_BOOT_MODE_KEYBOARD_ID:
            if (report_size < 8)
            {
                return false;
            }
            break;

        case BT_HID_BOOT_MODE_MOUSE_ID:
            if (report_size < 1)
            {
                return false;
            }
            break;

        default:
            return false;
        }
    }
    else
    {
        uint32_t size;
        size =  bt_hid_report_size_get_by_id(report_id,
                                             report_type,
                                             link->descriptor_len,
                                             link->descriptor);
        if ((size == 0) || (size != report_size))
        {
            return false;
        }
    }

    return true;
}

T_BT_HID_REPORT_ID_STATUS bt_hid_host_report_id_status_get(T_BT_HID_HOST_LINK *link,
                                                           uint16_t            report_id)
{
    if (link->proto_mode == BT_HID_MSG_PARAM_BOOT_PROTOCOL_MODE)
    {
        switch (report_id)
        {
        case BT_HID_BOOT_MODE_KEYBOARD_ID:
        case BT_HID_BOOT_MODE_MOUSE_ID:
            return BT_HID_REPORT_ID_VALID;

        default:
            return BT_HID_REPORT_ID_INVALID;
        }
    }
    else
    {
        return bt_hid_get_report_id_status(report_id,
                                           link->descriptor_len,
                                           link->descriptor);
    }
}

void bt_hid_host_handle_hid_control(T_BT_HID_HOST_LINK *link,
                                    uint8_t            *p_data,
                                    uint16_t            length)
{
    T_BT_HID_HOST_CONTROL_IND control_ind;
    T_BT_MSG_PAYLOAD          payload;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    control_ind.control_operation = p_data[0] & 0x0f;
    if (control_ind.control_operation == BT_HID_MSG_PARAM_CONTROL_VIRTUAL_CABLE_UNPLUG)
    {
        bt_hid_host_disconnect_req(link->bd_addr);
        link->virtual_unplug = true;
    }
}

void bt_hid_host_handle_handshake(T_BT_HID_HOST_LINK *link,
                                  uint8_t            *p_data,
                                  uint16_t            length)
{
    T_BT_HID_MSG_ITEM             *msg_item;

    msg_item = os_queue_out(&link->msg_list);
    if (msg_item != NULL)
    {
        uint8_t                        msg_param;
        T_BT_MSG_PAYLOAD               payload;
        T_BT_HID_HOST_SET_REPORT_RSP   set_report_rsp;
        T_BT_HID_HOST_SET_PROTOCOL_RSP set_protocol_rsp;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        msg_param = p_data[0] & 0x0f;
        link->control_chann.report_status = msg_param;
        link->credits = 1;
        switch (msg_item->msg_type)
        {
        case BT_HID_MSG_TYPE_SET_PROTOCOL:
            set_protocol_rsp.result_code = msg_param;
            if (msg_param == BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
            {
                link->proto_mode = msg_item->msg_param;
                if (link->interrupt_chann.state == BT_HID_STATE_DISCONNECTED)
                {
                    link->interrupt_chann.state = BT_HID_STATE_CONNECTING;
                    mpa_send_l2c_conn_req(PSM_HID_INTERRUPT, UUID_HIDP, bt_hid_host->queue_id_interrupt,
                                          BT_HID_INTERRUPT_MTU_SIZE, link->bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);
                }
            }
            else
            {
                bt_hid_host_disconnect_req(link->bd_addr);
            }
            payload.msg_buf = &set_protocol_rsp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_SET_PROTOCOL_RSP, &payload);
            break;

        case BT_HID_MSG_TYPE_SET_REPORT:
            set_report_rsp.result_code = msg_param;
            payload.msg_buf = &set_report_rsp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_SET_REPORT_RSP, &payload);
            break;

        case BT_HID_MSG_TYPE_GET_REPORT:
        case BT_HID_MSG_TYPE_GET_PROTOCOL:
        default:
            break;
        }
    }
}

void bt_hid_host_handle_data(T_BT_HID_HOST_LINK *link,
                             uint8_t            *p_data,
                             uint16_t            length)
{
    T_BT_HID_MSG_ITEM             *msg_item;

    msg_item = os_queue_out(&link->msg_list);
    if (msg_item != NULL)
    {
        T_BT_MSG_PAYLOAD               payload;
        int                            offset = 0;
        T_BT_HID_HOST_GET_REPORT_RSP   get_report_rsp;
        T_BT_HID_HOST_GET_PROTOCOL_RSP get_protocol_rsp;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        if (msg_item->msg_type == BT_HID_MSG_TYPE_GET_PROTOCOL)
        {
            link->credits = 1;
            get_protocol_rsp.proto_mode = p_data[1];
            payload.msg_buf = &get_protocol_rsp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_GET_PROTOCOL_RSP, &payload);
        }
        else if (msg_item->msg_type == BT_HID_MSG_TYPE_GET_REPORT)
        {
            link->credits = 1;
            get_report_rsp.report_type = (T_BT_HID_REPORT_TYPE)p_data[offset++] & 0x03;
            get_report_rsp.report_id = 0;
            if (bt_hid_report_id_declared(link->descriptor_len, link->descriptor))
            {
                get_report_rsp.report_id = p_data[offset++];
            }
            get_report_rsp.report_size = length - offset;
            get_report_rsp.p_data = &p_data[offset];
            payload.msg_buf = &get_report_rsp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_GET_REPORT_RSP, &payload);
        }
    }
}

void bt_hid_host_control_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_BT_HID_HOST_LINK *link;

    link = bt_hid_host_find_link_by_cid(data_ind->cid);
    if (link != NULL)
    {
        uint8_t *p_data;
        uint16_t length;
        uint8_t  msg_type;

        p_data = data_ind->data + data_ind->gap;
        length = data_ind->length;
        msg_type = p_data[0] >> 4;
        link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;

        switch (msg_type)
        {
        case BT_HID_MSG_TYPE_HID_CONTROL:
            bt_hid_host_handle_hid_control(link, p_data, length);
            break;

        case BT_HID_MSG_TYPE_HANDSHAKE:
            bt_hid_host_handle_handshake(link, p_data, length);
            break;

        case BT_HID_MSG_TYPE_DATA:
            bt_hid_host_handle_data(link, p_data, length);
            break;

        default:
            break;
        }

        bt_hid_host_control_flush_cmd(link);
    }
}

void bt_hid_host_interrupt_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_BT_HID_HOST_LINK *link;

    link = bt_hid_host_find_link_by_cid(data_ind->cid);
    if (link != NULL)
    {
        uint8_t               *p_data;
        uint16_t               length;
        uint8_t                offset = 0;
        T_BT_HID_HOST_DATA_IND ind;
        T_BT_MSG_PAYLOAD       payload;

        p_data = data_ind->data + data_ind->gap;
        length = data_ind->length;

        if (length <= BT_HID_HDR_LENGTH)
        {
            return;
        }
        ind.report_type = p_data[offset++] & 0x03;
        ind.report_id = 0;
        if (bt_hid_report_id_declared(link->descriptor_len, link->descriptor))
        {
            ind.report_id = p_data[offset++];
        }

        if (bt_hid_host_report_id_status_get(link, ind.report_id) ==
            BT_HID_REPORT_ID_INVALID)
        {
            return;
        }

        if (!bt_hid_host_report_size_check(link,
                                           ind.report_id,
                                           (T_BT_HID_REPORT_TYPE)ind.report_type,
                                           length - offset))
        {
            return;
        }
        ind.report_size = length - offset;
        ind.p_data = &p_data[offset];
        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &ind;
        bt_mgr_dispatch(BT_MSG_HID_HOST_INTERRUPT_DATA_IND, &payload);
    }
}

static void bt_hid_host_cback(void        *p_buf,
                              T_PROTO_MSG  msg)
{
    T_BT_HID_HOST_LINK *link;
    T_BT_MSG_PAYLOAD    payload;

    BTM_PRINT_INFO1("bt_hid_host_cback: msg 0x%02x", msg);

    switch (msg)
    {
    case L2C_CONN_IND:
        {
            T_MPA_L2C_CONN_IND *ind = (T_MPA_L2C_CONN_IND *)p_buf;

            link = bt_hid_host_find_link_by_addr(ind->bd_addr);
            if (ind->proto_id == bt_hid_host->queue_id_control)
            {
                if (link == NULL)
                {
                    link = bt_hid_host_alloc_link(ind->bd_addr);
                    if (link != NULL)
                    {
                        link->control_chann.cid = ind->cid;
                        link->control_chann.state = BT_HID_STATE_CONNECTING;
                        link->int_flag = 0;
                        memcpy(payload.bd_addr, link->bd_addr, 6);
                        bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_IND, &payload);
                    }
                    else
                    {
                        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                              BT_HID_CONTROL_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                    }
                }
                else
                {
                    mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                          BT_HID_CONTROL_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                }
            }
            else if (ind->proto_id == bt_hid_host->queue_id_interrupt)
            {
                if (link == NULL)
                {
                    mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                          BT_HID_INTERRUPT_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                }
                else
                {
                    if (link->interrupt_chann.state == BT_HID_STATE_DISCONNECTED)
                    {
                        link->interrupt_chann.cid = ind->cid;
                        link->interrupt_chann.state = BT_HID_STATE_CONNECTING;
                        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT, ind->cid,
                                              BT_HID_INTERRUPT_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                    }
                    else
                    {
                        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                              BT_HID_INTERRUPT_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                    }
                }
            }
        }
        break;

    case L2C_CONN_RSP:
        {
            T_MPA_L2C_CONN_RSP *rsp = (T_MPA_L2C_CONN_RSP *)p_buf;

            link = bt_hid_host_find_link_by_addr(rsp->bd_addr);
            if (link != NULL)
            {
                if (rsp->proto_id == bt_hid_host->queue_id_control)
                {
                    if (rsp->cause == 0)
                    {
                        link->control_chann.cid = rsp->cid;
                    }
                    else
                    {
                        bt_hid_host_free_link(link);

                        memcpy(payload.bd_addr, rsp->bd_addr, 6);
                        payload.msg_buf = &rsp->cause;
                        bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_FAIL, &payload);
                    }
                }
                else if (rsp->proto_id == bt_hid_host->queue_id_interrupt)
                {
                    if (rsp->cause == 0)
                    {
                        link->interrupt_chann.cid = rsp->cid;
                    }
                    else
                    {
                        bt_hid_host_free_link(link);

                        memcpy(payload.bd_addr, rsp->bd_addr, 6);
                        payload.msg_buf = &rsp->cause;
                        bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_FAIL, &payload);
                    }
                }
            }
        }
        break;

    case L2C_CONN_CMPL:
        {
            T_MPA_L2C_CONN_CMPL_INFO *p_info = (T_MPA_L2C_CONN_CMPL_INFO *)p_buf;

            link = bt_hid_host_find_link_by_cid(p_info->cid);
            if (link != NULL)
            {
                memcpy(payload.bd_addr, p_info->bd_addr, 6);

                if (p_info->proto_id == bt_hid_host->queue_id_control)
                {
                    if (p_info->cause == 0)
                    {
                        link->control_chann.state = BT_HID_STATE_CONNECTED;
                        link->control_chann.remote_mtu = p_info->remote_mtu;
                        link->control_chann.ds_data_offset = p_info->ds_data_offset;
                        if (link->int_flag & INITIATE_CONN_MASK)
                        {
                            if (link->proto_mode == BT_HID_HOST_BOOT_PROTO_MODE)
                            {
                                bt_hid_host_set_protocol_req(link->bd_addr,
                                                             BT_HID_HOST_BOOT_PROTO_MODE);
                            }
                            else
                            {
                                if (link->interrupt_chann.state == BT_HID_STATE_DISCONNECTED)
                                {
                                    link->interrupt_chann.state = BT_HID_STATE_CONNECTING;
                                    mpa_send_l2c_conn_req(PSM_HID_INTERRUPT, UUID_HIDP, bt_hid_host->queue_id_interrupt,
                                                          BT_HID_INTERRUPT_MTU_SIZE, link->bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);
                                }
                            }
                        }
                    }
                    else
                    {
                        bt_hid_host_free_link(link);

                        payload.msg_buf = &p_info->cause;
                        bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_FAIL, &payload);
                    }
                }
                else if (p_info->proto_id == bt_hid_host->queue_id_interrupt)
                {
                    if (p_info->cause == 0)
                    {
                        payload.msg_buf = &p_info->cause;
                        link->interrupt_chann.state = BT_HID_STATE_CONNECTED;
                        link->interrupt_chann.remote_mtu = p_info->remote_mtu;
                        link->interrupt_chann.ds_data_offset = p_info->ds_data_offset;
                        bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_CMPL, &payload);
                    }
                    else
                    {
                        bt_hid_host_free_link(link);

                        payload.msg_buf = &p_info->cause;
                        bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_FAIL, &payload);
                    }
                }
            }
        }
        break;

    case L2C_DATA_IND:
        {
            T_MPA_L2C_DATA_IND *ind = (T_MPA_L2C_DATA_IND *)p_buf;

            if (ind->proto_id == bt_hid_host->queue_id_control)
            {
                bt_hid_host_control_handle_l2c_data_ind(ind);
            }
            else if (ind->proto_id == bt_hid_host->queue_id_interrupt)
            {
                bt_hid_host_interrupt_handle_l2c_data_ind(ind);
            }
        }
        break;

    case L2C_DISCONN_IND:
        {
            T_MPA_L2C_DISCONN_IND *ind = (T_MPA_L2C_DISCONN_IND *)p_buf;

            link = bt_hid_host_find_link_by_cid(ind->cid);
            if (link != NULL)
            {
                T_BT_HID_HOST_DISCONN_CMPL disconn_cmpl;
                memcpy(payload.bd_addr, link->bd_addr, 6);

                if (ind->proto_id == bt_hid_host->queue_id_control)
                {
                    disconn_cmpl.virtual_unplug = link->virtual_unplug;
                    disconn_cmpl.cause = ind->cause;
                    bt_hid_host_free_link(link);

                    payload.msg_buf = &disconn_cmpl;
                    mpa_send_l2c_disconn_cfm(ind->cid);
                    bt_mgr_dispatch(BT_MSG_HID_HOST_DISCONN_CMPL, &payload);
                }
                else if (ind->proto_id == bt_hid_host->queue_id_interrupt)
                {
                    mpa_send_l2c_disconn_cfm(ind->cid);
                    memset(&link->interrupt_chann, 0, sizeof(T_BT_HID_INTERRUPT_CHANN));
                }

            }
        }
        break;

    case L2C_DISCONN_RSP:
        {
            T_MPA_L2C_DISCONN_RSP *rsp = (T_MPA_L2C_DISCONN_RSP *)p_buf;

            link = bt_hid_host_find_link_by_cid(rsp->cid);
            if (link != NULL)
            {
                T_BT_HID_HOST_DISCONN_CMPL disconn_cmpl;
                memcpy(payload.bd_addr, link->bd_addr, 6);

                if (rsp->proto_id == bt_hid_host->queue_id_control)
                {
                    disconn_cmpl.virtual_unplug = link->virtual_unplug;
                    disconn_cmpl.cause = rsp->cause;
                    bt_hid_host_free_link(link);

                    payload.msg_buf = &disconn_cmpl;

                    bt_mgr_dispatch(BT_MSG_HID_HOST_DISCONN_CMPL, &payload);
                }
                else if (rsp->proto_id == bt_hid_host->queue_id_interrupt)
                {
                    if (link->int_flag & INITIATE_DISCONN_MASK)
                    {
                        link->int_flag &= ~INITIATE_DISCONN_MASK;
                        mpa_send_l2c_disconn_req(link->control_chann.cid);
                        link->control_chann.state = BT_HID_STATE_DISCONNECTING;
                    }
                }
            }
        }
        break;
    }
}

bool bt_hid_host_control_req(uint8_t                          bd_addr[6],
                             T_BT_HID_HOST_CONTROL_OPERATION  operation)
{
    return bt_hid_host_control_msg_send(bd_addr,
                                        BT_HID_MSG_TYPE_HID_CONTROL,
                                        operation,
                                        NULL,
                                        0);
}

bool bt_hid_host_get_report_req(uint8_t                   bd_addr[6],
                                T_BT_HID_HOST_REPORT_TYPE report_type,
                                uint8_t                   report_id,
                                uint16_t                  buffer_size)
{
    uint8_t cmd_buf[] = {0, 0, 0};

    if (report_id == 0)
    {
        if (buffer_size == 0)
        {
            return bt_hid_host_control_msg_send(bd_addr,
                                                BT_HID_MSG_TYPE_GET_REPORT,
                                                report_type,
                                                NULL,
                                                0);
        }
        else
        {
            memcpy(cmd_buf, &buffer_size, 2);
            return bt_hid_host_control_msg_send(bd_addr,
                                                BT_HID_MSG_TYPE_GET_REPORT,
                                                0x08 | (report_type & 0x03),
                                                cmd_buf,
                                                2);
        }
    }
    else
    {
        if (buffer_size == 0)
        {
            cmd_buf[0] = report_id;
            return bt_hid_host_control_msg_send(bd_addr,
                                                BT_HID_MSG_TYPE_GET_REPORT,
                                                report_type & 0x03,
                                                cmd_buf,
                                                1);
        }
        else
        {
            cmd_buf[0] = report_id;
            memcpy(cmd_buf + 1, &buffer_size, 2);
            return bt_hid_host_control_msg_send(bd_addr,
                                                BT_HID_MSG_TYPE_GET_REPORT,
                                                (0x01 << 3) | (report_type & 0x03),
                                                cmd_buf,
                                                3);
        }
    }
}

bool bt_hid_host_set_report_req(uint8_t                    bd_addr[6],
                                T_BT_HID_HOST_REPORT_TYPE  report_type,
                                uint8_t                   *buf,
                                uint16_t                   len)
{
    return bt_hid_host_control_msg_send(bd_addr,
                                        BT_HID_MSG_TYPE_SET_REPORT,
                                        report_type,
                                        buf,
                                        len);
}

bool bt_hid_host_get_protocol_req(uint8_t bd_addr[6])
{
    return bt_hid_host_control_msg_send(bd_addr,
                                        BT_HID_MSG_TYPE_GET_PROTOCOL,
                                        0,
                                        NULL,
                                        0);
}

bool bt_hid_host_set_protocol_req(uint8_t                     bd_addr[6],
                                  T_BT_HID_HOST_PROTOCOL_MODE proto_mode)
{
    return bt_hid_host_control_msg_send(bd_addr,
                                        BT_HID_MSG_TYPE_SET_PROTOCOL,
                                        proto_mode,
                                        NULL,
                                        0);
}

bool bt_hid_host_interrupt_data_send(uint8_t                    bd_addr[6],
                                     T_BT_HID_HOST_REPORT_TYPE  report_type,
                                     uint8_t                   *buf,
                                     uint16_t                   len)
{
    return bt_hid_host_interrupt_msg_send(bd_addr,
                                          BT_HID_MSG_TYPE_DATA,
                                          report_type,
                                          buf,
                                          len);
}

bool bt_hid_host_connect_req(uint8_t bd_addr[6],
                             uint8_t proto_mode)
{
    T_BT_HID_HOST_LINK *link;

    PROFILE_PRINT_TRACE2("bt_hid_host_connect_req: bd_addr %s, proto_mode %d",
                         TRACE_BDADDR(bd_addr), proto_mode);

    link = bt_hid_host_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_hid_host_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        link->control_chann.state = BT_HID_STATE_CONNECTING;
        link->int_flag = INITIATE_CONN_MASK;
        link->proto_mode = proto_mode;

        bt_sniff_mode_exit(bd_addr, false);
        mpa_send_l2c_conn_req(PSM_HID_CONTROL, UUID_HIDP, bt_hid_host->queue_id_control,
                              BT_HID_CONTROL_MTU_SIZE, bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);
        return true;
    }

    return false;
}

bool bt_hid_host_connect_cfm(uint8_t bd_addr[6],
                             bool    accept)
{
    T_BT_HID_HOST_LINK *link;

    link = bt_hid_host_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        if (accept)
        {
            mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT,
                                  link->control_chann.cid,
                                  BT_HID_CONTROL_MTU_SIZE,
                                  MPA_L2C_MODE_BASIC,
                                  0xFFFF);
        }
        else
        {
            mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                                  link->control_chann.cid,
                                  BT_HID_CONTROL_MTU_SIZE,
                                  MPA_L2C_MODE_BASIC,
                                  0xFFFF);
            bt_hid_host_free_link(link);
        }

        return true;
    }

    return false;
}

bool bt_hid_host_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_HID_HOST_LINK *link;

    PROFILE_PRINT_TRACE1("bt_hid_host_disconnect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    link = bt_hid_host_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);

        if (link->interrupt_chann.state == BT_HID_STATE_CONNECTED)
        {
            link->int_flag |= INITIATE_DISCONN_MASK;
            link->interrupt_chann.state = BT_HID_STATE_DISCONNECTING;
            mpa_send_l2c_disconn_req(link->interrupt_chann.cid);
            return true;
        }
        else if (link->control_chann.state == BT_HID_STATE_CONNECTED)
        {
            link->control_chann.state = BT_HID_STATE_DISCONNECTING;
            mpa_send_l2c_disconn_req(link->control_chann.cid);
            return true;
        }
    }

    return false;
}

bool bt_hid_host_descriptor_set(uint8_t   bd_addr[6],
                                uint8_t  *descriptor,
                                uint16_t  len)
{
    T_BT_HID_HOST_LINK *link;

    link = bt_hid_host_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        link->descriptor     = descriptor;
        link->descriptor_len = len;
        return true;
    }

    return false;
}

bool bt_hid_host_init(bool boot_proto_mode)
{
    int32_t ret = 0;

    bt_hid_host = calloc(1, sizeof(T_BT_HID_HOST));
    if (bt_hid_host == NULL)
    {
        ret = 1;
        goto fail_alloc_hid;
    }

    bt_hid_host->boot_proto_mode  = boot_proto_mode;

    if (mpa_reg_l2c_proto(PSM_HID_CONTROL, bt_hid_host_cback,
                          &bt_hid_host->queue_id_control) == false)
    {
        ret = 2;
        goto fail_reg_l2c_hid_control;
    }

    if (mpa_reg_l2c_proto(PSM_HID_INTERRUPT, bt_hid_host_cback,
                          &bt_hid_host->queue_id_interrupt) == false)
    {
        ret = 3;
        goto fail_reg_l2c_hid_interrupt;
    }

    return true;

fail_reg_l2c_hid_interrupt:
fail_reg_l2c_hid_control:
    free(bt_hid_host);
    bt_hid_host = NULL;
fail_alloc_hid:
    PROFILE_PRINT_ERROR1("bt_hid_host_init: failed %d", -ret);
    return false;
}

void bt_hid_host_deinit(void)
{
    if (bt_hid_host != NULL)
    {
        T_BT_HID_HOST_LINK *link;

        link = os_queue_out(&bt_hid_host->list);
        while (link != NULL)
        {
            T_BT_HID_MSG_ITEM  *p_item;

            p_item = os_queue_out(&link->msg_list);
            while (p_item != NULL)
            {
                free(p_item);
                p_item = os_queue_out(&link->msg_list);
            }
            free(link);
            link = os_queue_out(&bt_hid_host->list);
        }

        free(bt_hid_host);
        bt_hid_host = NULL;
    }
}

bool bt_hid_host_roleswap_info_get(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_HID_HOST_INFO *info)
{
    T_BT_HID_HOST_LINK *link;

    link = bt_hid_host_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        memcpy(info->bd_addr, bd_addr, 6);
        info->proto_mode            = link->proto_mode;
        info->control_cid           = link->control_chann.cid;
        info->control_remote_mtu    = link->control_chann.remote_mtu;
        info->control_state         = link->control_chann.state;
        info->control_data_offset   = link->control_chann.ds_data_offset;
        info->interrupt_cid         = link->interrupt_chann.cid;
        info->interrupt_remote_mtu  = link->interrupt_chann.remote_mtu;
        info->interrupt_state       = link->interrupt_chann.state;
        info->interrupt_data_offset = link->interrupt_chann.ds_data_offset;

        return true;
    }

    return false;
}

bool bt_hid_host_roleswap_info_set(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_HID_HOST_INFO *info)
{
    T_BT_HID_HOST_LINK *link;

    link = bt_hid_host_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_hid_host_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        link->proto_mode                     = info->proto_mode;
        link->control_chann.cid              = info->control_cid;
        link->control_chann.remote_mtu       = info->control_remote_mtu;
        link->control_chann.state            = (T_BT_HID_STATE)info->control_state;
        link->control_chann.ds_data_offset   = info->control_data_offset;
        link->interrupt_chann.cid            = info->interrupt_cid;
        link->interrupt_chann.remote_mtu     = info->interrupt_remote_mtu;
        link->interrupt_chann.state          = (T_BT_HID_STATE)info->interrupt_state;
        link->interrupt_chann.ds_data_offset = info->interrupt_data_offset;

        return true;
    }

    return false;
}

bool bt_hid_host_roleswap_info_del(uint8_t bd_addr[6])
{
    T_BT_HID_HOST_LINK *link;

    link = bt_hid_host_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_hid_host_free_link(link);
        return true;
    }

    return false;
}
#else
#include <stdint.h>
#include <stdbool.h>

#include "bt_hid_host.h"
#include "bt_hid_int.h"

bool bt_hid_host_control_req(uint8_t                          bd_addr[6],
                             T_BT_HID_HOST_CONTROL_OPERATION  operation)
{
    return false;
}

bool bt_hid_host_get_report_req(uint8_t                   bd_addr[6],
                                T_BT_HID_HOST_REPORT_TYPE report_type,
                                uint8_t                   report_id,
                                uint16_t                  buffer_size)
{
    return false;
}

bool bt_hid_host_set_report_req(uint8_t                    bd_addr[6],
                                T_BT_HID_HOST_REPORT_TYPE  report_type,
                                uint8_t                   *buf,
                                uint16_t                   len)
{
    return false;
}

bool bt_hid_host_get_protocol_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hid_host_set_protocol_req(uint8_t                     bd_addr[6],
                                  T_BT_HID_HOST_PROTOCOL_MODE proto_mode)
{
    return false;
}

bool bt_hid_host_interrupt_data_send(uint8_t                    bd_addr[6],
                                     T_BT_HID_HOST_REPORT_TYPE  report_type,
                                     uint8_t                   *buf,
                                     uint16_t                   len)
{
    return false;
}

bool bt_hid_host_connect_req(uint8_t bd_addr[6],
                             uint8_t proto_mode)
{
    return false;
}

bool bt_hid_host_connect_cfm(uint8_t bd_addr[6],
                             bool    accept)
{
    return false;
}

bool bt_hid_host_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hid_host_descriptor_set(uint8_t   bd_addr[6],
                                uint8_t  *descriptor,
                                uint16_t  len)
{
    return false;
}

bool bt_hid_host_init(bool boot_proto_mode)
{
    return false;
}

void bt_hid_host_deinit(void)
{

}

bool bt_hid_host_roleswap_info_get(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_HID_HOST_INFO *info)
{
    return false;
}

bool bt_hid_host_roleswap_info_set(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_HID_HOST_INFO *info)
{
    return false;
}

bool bt_hid_host_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}
#endif
