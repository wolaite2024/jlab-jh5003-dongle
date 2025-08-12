/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HID_HOST_SUPPORT == 1)

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "trace.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_hid_host.h"
#include "bt_hid_int.h"
#include "hid_host.h"

/*hid_initiator_flag*/
#define INITIATE_CONN_MASK       0x01
#define INITIATE_DISCONN_MASK    0x02

void bt_hid_host_cback(uint8_t         bd_addr[6],
                       T_HID_HOST_MSG  msg_type,
                       void           *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = NULL;

    BTM_PRINT_INFO2("bt_hid_host_cback: bd_addr %s, msg 0x%02x", TRACE_BDADDR(bd_addr), msg_type);

    switch (msg_type)
    {
    case HID_HOST_MSG_CONTROL_CONN_IND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_IND, &payload);
        }
        else
        {
            hid_host_control_connect_cfm(bd_addr, false);
        }
        break;

    case HID_HOST_MSG_CONTROL_CONN_RSP:
        break;

    case HID_HOST_MSG_CONTROL_CONN_CMPL:
        if (p_link != NULL)
        {
            p_link->hid_data.control_chann_connected = true;
            p_link->connected_profile |= HID_HOST_PROFILE_MASK;

            if (p_link->hid_data.initiator & INITIATE_CONN_MASK)
            {
                if (p_link->hid_data.proto_mode == BT_HID_HOST_BOOT_PROTO_MODE)
                {
                    bt_hid_host_set_protocol_req(bd_addr,
                                                 BT_HID_HOST_BOOT_PROTO_MODE);
                }
                else
                {
                    hid_host_interrupt_connect_req(bd_addr);
                }
            }
        }
        else
        {
            hid_host_control_disconnect_req(bd_addr);
        }
        break;

    case HID_HOST_MSG_CONTROL_CONN_FAIL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            memset(&p_link->hid_data, 0, sizeof(T_HID_LINK_DATA));
            bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_FAIL, &payload);
        }
        break;

    case HID_HOST_MSG_CONTROL_DISCONNECTED:
        if (p_link != NULL)
        {
            p_link->connected_profile &= ~HID_HOST_PROFILE_MASK;
            p_link->hid_data.interrupt_chann_connected = false;
            p_link->hid_data.control_chann_connected = false;
            payload.msg_buf = msg_buf;
            memset(&(p_link->hid_data), 0, sizeof(T_HID_LINK_DATA));
            bt_mgr_dispatch(BT_MSG_HID_HOST_DISCONN_CMPL, &payload);
        }
        break;

    case HID_HOST_MSG_HID_CONTROL_IND:
        if (p_link != NULL)
        {
            T_HID_HOST_CONTROL_IND *ind = (T_HID_HOST_CONTROL_IND *)msg_buf;
            T_BT_HID_HOST_CONTROL_IND tmp;
            tmp.control_operation = ind->control_operation;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_HID_CONTROL_IND, &payload);
        }
        break;

    case HID_HOST_MSG_GET_REPORT_RSP:
        if (p_link != NULL)
        {
            T_HID_HOST_GET_REPORT_RSP *rsp = (T_HID_HOST_GET_REPORT_RSP *)msg_buf;
            T_BT_HID_HOST_GET_REPORT_RSP tmp;
            tmp.report_type = rsp->report_type;
            tmp.report_size = rsp->report_size;
            tmp.report_id = rsp->report_id;
            tmp.p_data = rsp->p_data;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_GET_REPORT_RSP, &payload);
        }
        break;

    case HID_HOST_MSG_SET_REPORT_RSP:
        if (p_link != NULL)
        {
            T_HID_HOST_SET_REPORT_RSP *rsp = (T_HID_HOST_SET_REPORT_RSP *)msg_buf;
            T_BT_HID_HOST_SET_REPORT_RSP tmp;
            tmp.result_code = rsp->result_code;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_SET_REPORT_RSP, &payload);
        }
        break;

    case HID_HOST_MSG_GET_PROTOCOL_RSP:
        if (p_link != NULL)
        {
            T_HID_HOST_GET_PROTOCOL_RSP *rsp = (T_HID_HOST_GET_PROTOCOL_RSP *)msg_buf;
            T_BT_HID_HOST_GET_PROTOCOL_RSP tmp;
            tmp.proto_mode = rsp->proto_mode;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_GET_PROTOCOL_RSP, &payload);
        }
        break;

    case HID_HOST_MSG_SET_PROTOCOL_RSP:
        if (p_link != NULL)
        {
            T_HID_HOST_SET_PROTOCOL_RSP *rsp = (T_HID_HOST_SET_PROTOCOL_RSP *)msg_buf;
            T_BT_HID_HOST_SET_PROTOCOL_RSP tmp;
            tmp.result_code = rsp->result_code;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_SET_PROTOCOL_RSP, &payload);
        }
        break;

    case HID_HOST_MSG_INTERRUPT_CONN_IND:
        if (p_link != NULL)
        {
            hid_host_interrupt_connect_cfm(bd_addr, true);
        }
        else
        {
            hid_host_interrupt_connect_cfm(bd_addr, false);
        }
        break;

    case HID_HOST_MSG_INTERRUPT_CONN_RSP:
        break;

    case HID_HOST_MSG_INTERRUPT_CONN_CMPL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            p_link->hid_data.interrupt_chann_connected = true;
            bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_CMPL, &payload);
        }
        else
        {
            hid_host_interrupt_disconnect_req(bd_addr);
        }
        break;

    case HID_HOST_MSG_INTERRUPT_CONN_FAIL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            memset(&p_link->hid_data, 0, sizeof(T_HID_LINK_DATA));
            bt_mgr_dispatch(BT_MSG_HID_HOST_CONN_FAIL, &payload);
        }
        break;

    case HID_HOST_MSG_INTERRUPT_DISCONNECTED:
        if (p_link != NULL)
        {
            p_link->hid_data.interrupt_chann_connected = false;
            if (p_link->hid_data.initiator & INITIATE_DISCONN_MASK)
            {
                p_link->hid_data.initiator &= ~INITIATE_DISCONN_MASK;
                hid_host_control_disconnect_req(p_link->bd_addr);
            }

            memset(&(p_link->hid_data), 0, sizeof(T_HID_LINK_DATA));
        }
        break;

    case HID_HOST_MSG_INTERRUPT_DATA_IND:
        if (p_link != NULL)
        {
            T_HID_HOST_DATA_IND *ind = (T_HID_HOST_DATA_IND *)msg_buf;
            T_BT_HID_HOST_DATA_IND tmp;
            tmp.report_type = ind->report_type;
            tmp.report_size = ind->report_size;
            tmp.report_id = ind->report_id;
            tmp.p_data = ind->p_data;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_HOST_INTERRUPT_DATA_IND, &payload);
        }
        break;

    default:
        break;
    }
}

bool bt_hid_host_control_req(uint8_t                          bd_addr[6],
                             T_BT_HID_HOST_CONTROL_OPERATION  operation,
                             uint8_t                         *buf,
                             uint16_t                         len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_HOST_PROFILE_MASK)
        {
            return hid_host_control_msg_send(bd_addr,
                                             HID_MSG_TYPE_HID_CONTROL,
                                             operation,
                                             buf,
                                             len);
        }
    }

    return false;
}

bool bt_hid_host_get_report_req(uint8_t                   bd_addr[6],
                                T_BT_HID_HOST_REPORT_TYPE report_type,
                                uint8_t                   report_id,
                                uint16_t                  buffer_size)
{
    T_BT_BR_LINK *p_link;
    uint8_t cmd_buf[] = {0, 0, 0};

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_HOST_PROFILE_MASK)
        {
            if (cmd_buf != NULL)
            {
                if (report_id == 0)
                {
                    if (buffer_size == 0)
                    {
                        return hid_host_control_msg_send(bd_addr,
                                                         HID_MSG_TYPE_GET_REPORT,
                                                         report_type,
                                                         NULL,
                                                         0);
                    }
                    else
                    {
                        memcpy(cmd_buf, &buffer_size, 2);
                        return hid_host_control_msg_send(bd_addr,
                                                         HID_MSG_TYPE_GET_REPORT,
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
                        return hid_host_control_msg_send(bd_addr,
                                                         HID_MSG_TYPE_GET_REPORT,
                                                         report_type & 0x03,
                                                         cmd_buf,
                                                         1);
                    }
                    else
                    {
                        cmd_buf[0] = report_id;
                        memcpy(cmd_buf + 1, &buffer_size, 2);
                        return hid_host_control_msg_send(bd_addr,
                                                         HID_MSG_TYPE_GET_REPORT,
                                                         (0x01 << 3) | (report_type & 0x03),
                                                         cmd_buf,
                                                         3);
                    }
                }
            }
        }
    }

    return false;
}

bool bt_hid_host_set_report_req(uint8_t                    bd_addr[6],
                                T_BT_HID_HOST_REPORT_TYPE  report_type,
                                uint8_t                   *buf,
                                uint16_t                   len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_HOST_PROFILE_MASK)
        {
            return hid_host_control_msg_send(bd_addr,
                                             HID_MSG_TYPE_SET_REPORT,
                                             report_type,
                                             buf,
                                             len);
        }
    }

    return false;
}

bool bt_hid_host_get_protocol_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_HOST_PROFILE_MASK)
        {
            return hid_host_control_msg_send(bd_addr,
                                             HID_MSG_TYPE_GET_PROTOCOL,
                                             0,
                                             NULL,
                                             0);
        }
    }

    return false;
}

bool bt_hid_host_set_protocol_req(uint8_t                     bd_addr[6],
                                  T_BT_HID_HOST_PROTOCOL_MODE proto_mode)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_HOST_PROFILE_MASK)
        {
            return hid_host_control_msg_send(bd_addr,
                                             HID_MSG_TYPE_SET_PROTOCOL,
                                             proto_mode,
                                             NULL,
                                             0);
        }
    }

    return false;
}

bool bt_hid_host_interrupt_data_send(uint8_t                    bd_addr[6],
                                     T_BT_HID_HOST_REPORT_TYPE  report_type,
                                     uint8_t                   *buf,
                                     uint16_t                   len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_HOST_PROFILE_MASK)
        {
            return hid_host_interrupt_msg_send(bd_addr,
                                               HID_MSG_TYPE_DATA,
                                               report_type,
                                               buf,
                                               len);
        }
    }

    return false;
}

bool bt_hid_host_connect_req(uint8_t bd_addr[6],
                             uint8_t proto_mode)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & HID_HOST_PROFILE_MASK) == 0)
        {
            bt_sniff_mode_exit(p_link, false);
            p_link->hid_data.initiator = INITIATE_CONN_MASK;
            p_link->hid_data.proto_mode = proto_mode;
            return hid_host_control_connect_req(bd_addr);
        }
    }

    return false;
}

bool bt_hid_host_connect_cfm(uint8_t bd_addr[6],
                             bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return hid_host_control_connect_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_hid_host_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);

        if (p_link->hid_data.interrupt_chann_connected)
        {
            p_link->hid_data.initiator |= INITIATE_DISCONN_MASK;
            return hid_host_interrupt_disconnect_req(bd_addr);
        }
        else if (p_link->hid_data.control_chann_connected)
        {
            return hid_host_control_disconnect_req(bd_addr);
        }

    }

    return false;
}

bool bt_hid_host_descriptor_set(const uint8_t *descriptor,
                                uint16_t       len)
{
    return hid_host_descriptor_set(descriptor, len);
}

bool bt_hid_host_init(uint8_t link_num,
                      bool    boot_proto_mode)
{
    return hid_host_init(link_num, boot_proto_mode, bt_hid_host_cback);
}
#else
#include <stdint.h>
#include <stdbool.h>

#include "bt_hid_host.h"

bool bt_hid_host_control_req(uint8_t                          bd_addr[6],
                             T_BT_HID_HOST_CONTROL_OPERATION  operation,
                             uint8_t                         *buf,
                             uint16_t                         len)
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

bool bt_hid_host_descriptor_set(const uint8_t *descriptor,
                                uint16_t       len)
{
    return false;
}

bool bt_hid_host_init(uint8_t link_num,
                      bool    boot_proto_mode)
{
    return false;
}
#endif
