/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HID_DEVICE_SUPPORT == 1)

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "trace.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_hid_device.h"
#include "bt_hid_int.h"
#include "hid_device.h"

/*hid_initiator_flag*/
#define INITIATE_CONN_MASK       0x01
#define INITIATE_DISCONN_MASK    0x02

void bt_hid_device_cback(uint8_t           bd_addr[6],
                         T_HID_DEVICE_MSG  msg_type,
                         void             *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = NULL;

    BTM_PRINT_INFO2("bt_hid_device_cback: bd_addr %s, msg 0x%02x", TRACE_BDADDR(bd_addr), msg_type);

    switch (msg_type)
    {
    case HID_DEVICE_MSG_CONTROL_CONN_IND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_IND, &payload);
        }
        else
        {
            hid_device_control_connect_cfm(bd_addr, false);
        }
        break;

    case HID_DEVICE_MSG_CONTROL_CONN_RSP:
        break;

    case HID_DEVICE_MSG_CONTROL_CONN_CMPL:
        if (p_link != NULL)
        {
            p_link->hid_data.control_chann_connected = true;
            p_link->connected_profile |= HID_DEVICE_PROFILE_MASK;
            p_link->hid_data.proto_mode = BT_HID_DEVICE_REPORT_PROTO_MODE;

            if (p_link->hid_data.initiator & INITIATE_CONN_MASK)
            {
                hid_device_interrupt_connect_req(bd_addr);
            }
        }
        else
        {
            hid_device_control_disconnect_req(bd_addr);
        }
        break;

    case HID_DEVICE_MSG_CONTROL_CONN_FAIL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            memset(&p_link->hid_data, 0, sizeof(T_HID_LINK_DATA));
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_FAIL, &payload);
        }
        break;

    case HID_DEVICE_MSG_CONTROL_DISCONNECTED:
        if (p_link != NULL)
        {
            p_link->connected_profile &= ~HID_DEVICE_PROFILE_MASK;
            p_link->hid_data.interrupt_chann_connected = false;
            p_link->hid_data.control_chann_connected = false;
            payload.msg_buf = msg_buf;
            memset(&(p_link->hid_data), 0, sizeof(T_HID_LINK_DATA));
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_DISCONN_CMPL, &payload);
        }
        break;

    case HID_DEVICE_MSG_CONTROL_DATA_IND:
        if (p_link != NULL)
        {
            T_HID_DEVICE_DATA_IND *ind = (T_HID_DEVICE_DATA_IND *)msg_buf;
            T_BT_HID_DEVICE_DATA_IND tmp;
            tmp.report_type = ind->report_type;
            tmp.report_size = ind->report_size;
            tmp.report_id = ind->report_id;
            tmp.p_data = ind->p_data;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONTROL_DATA_IND, &payload);
        }
        break;

    case HID_DEVICE_MSG_GET_REPORT_IND:
        if (p_link != NULL)
        {
            T_HID_DEVICE_GET_REPORT_IND *ind = (T_HID_DEVICE_GET_REPORT_IND *)msg_buf;
            T_BT_HID_DEVICE_GET_REPORT_IND tmp;
            tmp.report_type = ind->report_type;
            tmp.report_size = ind->report_size;
            tmp.report_id = ind->report_id;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_GET_REPORT_IND, &payload);
        }
        break;

    case HID_DEVICE_MSG_SET_REPORT_IND:
        if (p_link != NULL)
        {
            T_HID_DEVICE_SET_REPORT_IND *ind = (T_HID_DEVICE_SET_REPORT_IND *)msg_buf;
            T_BT_HID_DEVICE_SET_REPORT_IND tmp;
            tmp.report_type = ind->report_type;
            tmp.report_id = ind->report_id;
            tmp.report_size = ind->report_size;
            tmp.p_data = ind->p_data;
            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_SET_REPORT_IND, &payload);
        }
        break;

    case HID_DEVICE_MSG_GET_PROTOCOL_IND:
        if (p_link != NULL)
        {
            T_HID_DEVICE_GET_PROTOCOL_IND *ind = (T_HID_DEVICE_GET_PROTOCOL_IND *)msg_buf;
            T_BT_HID_DEVICE_GET_PROTOCOL_IND tmp;
            bt_hid_device_get_protocol_rsp(bd_addr, (T_BT_HID_DEVICE_PROTOCOL_MODE)ind->proto_mode);
            tmp.proto_mode = ind->proto_mode;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_GET_PROTOCOL_IND, &payload);
        }
        break;

    case HID_DEVICE_MSG_SET_PROTOCOL_IND:
        if (p_link != NULL)
        {
            T_HID_DEVICE_SET_PROTOCOL_IND *ind = (T_HID_DEVICE_SET_PROTOCOL_IND *)msg_buf;
            T_BT_HID_DEVICE_SET_PROTOCOL_IND tmp;
            tmp.proto_mode = ind->proto_mode;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_SET_PROTOCOL_IND, &payload);
        }
        break;

    case HID_DEVICE_MSG_SET_IDLE_IND:
        if (p_link != NULL)
        {
            T_HID_DEVICE_SET_IDLE_IND *ind = (T_HID_DEVICE_SET_IDLE_IND *)msg_buf;
            T_BT_HID_DEVICE_SET_IDLE_IND tmp;
            tmp.report_status = ind->report_status;

            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_SET_IDLE_IND, &payload);
        }
        break;

    case HID_DEVICE_MSG_INTERRUPT_CONN_IND:
        if (p_link != NULL)
        {
            hid_device_interrupt_connect_cfm(bd_addr, true);
        }
        else
        {
            hid_device_interrupt_connect_cfm(bd_addr, false);
        }
        break;

    case HID_DEVICE_MSG_INTERRUPT_CONN_RSP:
        break;

    case HID_DEVICE_MSG_INTERRUPT_CONN_CMPL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            p_link->hid_data.interrupt_chann_connected = true;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_CMPL, &payload);
            bt_pm_sm(p_link, BT_PM_EVENT_SNIFF_ENTER_REQ);
        }
        else
        {
            hid_device_interrupt_disconnect_req(bd_addr);
        }
        break;

    case HID_DEVICE_MSG_INTERRUPT_CONN_FAIL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            memset(&p_link->hid_data, 0, sizeof(T_HID_LINK_DATA));
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_FAIL, &payload);
        }
        break;

    case HID_DEVICE_MSG_INTERRUPT_DISCONNECTED:
        if (p_link != NULL)
        {
            p_link->hid_data.interrupt_chann_connected = false;
            if (p_link->hid_data.initiator & INITIATE_DISCONN_MASK)
            {
                p_link->hid_data.initiator &= ~INITIATE_DISCONN_MASK;
                hid_device_control_disconnect_req(p_link->bd_addr);
            }

            memset(&(p_link->hid_data), 0, sizeof(T_HID_LINK_DATA));
        }
        break;

    case HID_DEVICE_MSG_INTERRUPT_DATA_IND:
        if (p_link != NULL)
        {
            T_HID_DEVICE_DATA_IND *ind = (T_HID_DEVICE_DATA_IND *)msg_buf;
            T_BT_HID_DEVICE_DATA_IND tmp;
            tmp.report_type = ind->report_type;
            tmp.report_size = ind->report_size;
            tmp.report_id = ind->report_id;
            tmp.p_data = ind->p_data;
            payload.msg_buf = &tmp;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_INTERRUPT_DATA_IND, &payload);
        }
        break;

    default:
        break;
    }
}

bool bt_hid_device_get_report_rsp(uint8_t                      bd_addr[6],
                                  T_BT_HID_DEVICE_REPORT_TYPE  report_type,
                                  uint8_t                     *buf,
                                  uint16_t                     len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_DEVICE_PROFILE_MASK)
        {
            return hid_device_control_msg_send(bd_addr,
                                               HID_MSG_TYPE_DATA,
                                               report_type,
                                               buf,
                                               len);
        }
    }

    return false;
}

bool bt_hid_device_get_protocol_rsp(uint8_t                       bd_addr[6],
                                    T_BT_HID_DEVICE_PROTOCOL_MODE proto_mode)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_DEVICE_PROFILE_MASK)
        {
            return hid_device_control_msg_send(bd_addr,
                                               HID_MSG_TYPE_DATA,
                                               0,
                                               &proto_mode,
                                               1);
        }
    }

    return false;
}

bool bt_hid_device_control_req(uint8_t                            bd_addr[6],
                               T_BT_HID_DEVICE_CONTROL_OPERATION  operation,
                               uint8_t                           *buf,
                               uint16_t                           len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_DEVICE_PROFILE_MASK)
        {
            return hid_device_control_msg_send(bd_addr,
                                               HID_MSG_TYPE_HID_CONTROL,
                                               operation,
                                               buf,
                                               len);
        }
    }

    return false;
}

bool bt_hid_device_interrupt_data_send(uint8_t                      bd_addr[6],
                                       T_BT_HID_DEVICE_REPORT_TYPE  report_type,
                                       uint8_t                     *buf,
                                       uint16_t                     len)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->connected_profile & HID_DEVICE_PROFILE_MASK)
        {
            return hid_device_interrupt_msg_send(bd_addr, HID_MSG_TYPE_DATA, report_type, buf, len);
        }
    }

    return false;
}

bool bt_hid_device_connect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & HID_DEVICE_PROFILE_MASK) == 0)
        {
            bt_sniff_mode_exit(p_link, false);
            p_link->hid_data.initiator = INITIATE_CONN_MASK;
            return hid_device_control_connect_req(bd_addr);
        }
    }

    return false;
}

bool bt_hid_device_connect_cfm(uint8_t bd_addr[6],
                               bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return hid_device_control_connect_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_hid_device_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);

        if (p_link->hid_data.interrupt_chann_connected)
        {
            p_link->hid_data.initiator |= INITIATE_DISCONN_MASK;
            return hid_device_interrupt_disconnect_req(bd_addr);
        }
        else if (p_link->hid_data.control_chann_connected)
        {
            return hid_device_control_disconnect_req(bd_addr);
        }

    }

    return false;
}

bool bt_hid_device_descriptor_set(const uint8_t *descriptor,
                                  uint16_t       len)
{
    return hid_device_descriptor_set(descriptor, len);
}

bool bt_hid_device_init(uint8_t link_num,
                        bool    boot_proto_mode)
{
    return hid_device_init(link_num, boot_proto_mode, bt_hid_device_cback);
}

#else
#include <stdint.h>
#include <stdbool.h>

#include "bt_hid_device.h"

bool bt_hid_device_get_report_rsp(uint8_t                      bd_addr[6],
                                  T_BT_HID_DEVICE_REPORT_TYPE  report_type,
                                  uint8_t                     *buf,
                                  uint16_t                     len)
{
    return false;
}

bool bt_hid_device_get_protocol_rsp(uint8_t                       bd_addr[6],
                                    T_BT_HID_DEVICE_PROTOCOL_MODE proto_mode)
{
    return false;
}

bool bt_hid_device_control_req(uint8_t                            bd_addr[6],
                               T_BT_HID_DEVICE_CONTROL_OPERATION  operation,
                               uint8_t                           *buf,
                               uint16_t                           len)
{
    return false;
}

bool bt_hid_device_interrupt_data_send(uint8_t                      bd_addr[6],
                                       T_BT_HID_DEVICE_REPORT_TYPE  report_type,
                                       uint8_t                     *buf,
                                       uint16_t                     len)
{
    return false;
}

bool bt_hid_device_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hid_device_connect_cfm(uint8_t bd_addr[6],
                               bool    accept)
{
    return false;
}

bool bt_hid_device_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hid_device_descriptor_set(const uint8_t *descriptor,
                                  uint16_t       len)
{
    return false;
}

bool bt_hid_device_init(uint8_t link_num,
                        bool    boot_proto_mode)
{
    return false;
}
#endif
