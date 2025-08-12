/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HID_HOST_SUPPORT == 1)

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "os_mem.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "hid_host.h"
#include "hid_parser.h"

typedef struct t_hid_msg_item
{
    struct t_hid_msg_item  *next;
    uint8_t                 msg_type;
    uint8_t                 msg_param;
    uint8_t                 msg_len;
    uint8_t                 msg[0];
} T_HID_MSG_ITEM;

typedef struct t_hid_host_link
{
    uint8_t                  bd_addr[6];
    uint8_t                  proto_mode;
    uint8_t                  credits;
    T_OS_QUEUE               msg_list;
    T_HID_CONTROL_CHANN      control_chann;
    T_HID_INTERRUPT_CHANN    interrupt_chann;
} T_HID_HOST_LINK;

typedef struct  t_hid_host
{
    uint8_t              queue_id_control;
    uint8_t              queue_id_interrupt;
    uint8_t              link_num;
    bool                 boot_proto_mode;
    const uint8_t       *descriptor;
    uint16_t             descriptor_len;
    T_HID_HOST_LINK     *link;
    P_HID_HOST_CBACK     host_cback;
} T_HID_HOST;

static T_HID_HOST *hid_host = NULL;

T_HID_HOST_LINK *hid_host_alloc_link(uint8_t bd_addr[6])
{
    uint8_t          i;
    T_HID_HOST_LINK *p_link = NULL;

    for (i = 0; i < hid_host->link_num; i++)
    {
        if (hid_host->link[i].control_chann.state == HID_STATE_DISCONNECTED)
        {
            p_link = &(hid_host->link[i]);
            memcpy(p_link->bd_addr, bd_addr, 6);
            p_link->control_chann.state = HID_STATE_ALLOCATED;
            p_link->credits = 1;
            p_link->proto_mode = HID_MSG_PARAM_REPORT_PROTOCOL_MODE;
            os_queue_init(&p_link->msg_list);
            break;
        }
    }

    return p_link;
}

void hid_host_free_link(T_HID_HOST_LINK *p_link)
{
    memset(p_link, 0, sizeof(T_HID_HOST_LINK));
}

T_HID_HOST_LINK *hid_host_find_link_by_cid(uint16_t cid)
{
    uint8_t          i;
    T_HID_HOST_LINK *p_link;

    for (i = 0; i < hid_host->link_num; i++)
    {
        p_link = &(hid_host->link[i]);
        if ((p_link->control_chann.state != HID_STATE_DISCONNECTED &&
             p_link->control_chann.cid == cid) ||
            (p_link->interrupt_chann.state != HID_STATE_DISCONNECTED &&
             p_link->interrupt_chann.cid == cid))
        {
            return p_link;
        }
    }

    return NULL;
}

T_HID_HOST_LINK *hid_host_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t          i;
    T_HID_HOST_LINK *p_link = NULL;

    for (i = 0; i < hid_host->link_num; i++)
    {
        if (hid_host->link[i].control_chann.state != HID_STATE_DISCONNECTED &&
            !memcmp(hid_host->link[i].bd_addr, bd_addr, 6))
        {
            p_link = &(hid_host->link[i]);
            break;
        }
    }

    return p_link;
}

bool hid_host_control_connect_req(uint8_t bd_addr[6])
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = hid_host_alloc_link(bd_addr);
        if (p_link != NULL)
        {
            PROFILE_PRINT_TRACE1("hid_host_control_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            p_link->control_chann.state = HID_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_HID_CONTROL, UUID_HIDP, hid_host->queue_id_control,
                                  HID_CONTROL_MTU_SIZE, bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);

            return true;
        }
    }

    return false;
}

bool hid_host_interrupt_connect_req(uint8_t bd_addr[6])
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        PROFILE_PRINT_TRACE1("hid_host_interrupt_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

        if (p_link->interrupt_chann.state == HID_STATE_DISCONNECTED)
        {
            p_link->interrupt_chann.state = HID_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_HID_INTERRUPT, UUID_HIDP, hid_host->queue_id_interrupt,
                                  HID_INTERRUPT_MTU_SIZE, bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);

            return true;
        }
    }

    return false;
}

bool hid_host_control_disconnect_req(uint8_t bd_addr[6])
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        mpa_send_l2c_disconn_req(p_link->control_chann.cid);
        return true;
    }

    return false;
}

bool hid_host_interrupt_disconnect_req(uint8_t bd_addr[6])
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        mpa_send_l2c_disconn_req(p_link->interrupt_chann.cid);
        return true;
    }

    return false;
}

bool hid_host_control_connect_cfm(uint8_t bd_addr[6],
                                  bool    accept)
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        uint16_t cid;
        T_MPA_L2C_CONN_CFM_CAUSE rsp;

        cid = p_link->control_chann.cid;
        rsp = MPA_L2C_CONN_ACCEPT;
        if (accept == false)
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;
            hid_host_free_link(p_link);
        }
        mpa_send_l2c_conn_cfm(rsp,
                              cid,
                              HID_CONTROL_MTU_SIZE,
                              MPA_L2C_MODE_BASIC,
                              0xFFFF);
        return true;
    }

    return false;
}

bool hid_host_interrupt_connect_cfm(uint8_t bd_addr[6],
                                    bool    accept)
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        T_MPA_L2C_CONN_CFM_CAUSE rsp;

        rsp = MPA_L2C_CONN_ACCEPT;
        if (accept == false)
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;
            p_link->interrupt_chann.state = HID_STATE_DISCONNECTED;
        }

        mpa_send_l2c_conn_cfm(rsp,
                              p_link->interrupt_chann.cid,
                              HID_INTERRUPT_MTU_SIZE,
                              MPA_L2C_MODE_BASIC,
                              0xFFFF);
        return true;
    }

    return false;
}

bool hid_host_control_msg_req(T_HID_HOST_LINK *p_link,
                              uint8_t          msg_type,
                              uint8_t          msg_param,
                              uint8_t         *buf,
                              uint16_t         len)
{
    uint8_t *hid_buf;

    hid_buf = mpa_get_l2c_buf(hid_host->queue_id_control, p_link->control_chann.cid,
                              0,
                              HID_HDR_LENGTH + len,
                              p_link->control_chann.ds_data_offset,
                              false);
    if (hid_buf != NULL)
    {
        *(hid_buf + p_link->control_chann.ds_data_offset) = (msg_type << 4) | (msg_param & 0x0f);
        memcpy(hid_buf + p_link->control_chann.ds_data_offset + HID_HDR_LENGTH, buf, len);

        mpa_send_l2c_data_req(hid_buf,
                              p_link->control_chann.ds_data_offset,
                              p_link->control_chann.cid,
                              HID_HDR_LENGTH + len,
                              false);

        p_link->credits = 0;

        return true;
    }

    return false;
}

bool hid_host_control_flush_cmd(T_HID_HOST_LINK *p_link)
{
    if (p_link->credits > 0)
    {
        T_HID_MSG_ITEM *p_item;

        p_item = os_queue_peek(&p_link->msg_list, 0);
        if (p_item != NULL)
        {
            return hid_host_control_msg_req(p_link,
                                            p_item->msg_type,
                                            p_item->msg_param,
                                            p_item->msg,
                                            p_item->msg_len);
        }
    }
    return false;
}

bool hid_host_control_msg_send(uint8_t  bd_addr[6],
                               uint8_t  msg_type,
                               uint8_t  msg_param,
                               uint8_t *buf,
                               uint8_t  len)
{
    T_HID_HOST_LINK *p_link;
    T_HID_MSG_ITEM  *p_item;
    int32_t          ret = 0;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (HID_HDR_LENGTH + len > p_link->control_chann.remote_mtu)
    {
        ret = 2;
        goto fail_invalid_msg_len;
    }

    if (msg_type == HID_MSG_TYPE_HID_CONTROL &&
        msg_param == HID_MSG_PARAM_CONTROL_VIRTUAL_CABLE_UNPLUG)
    {
        if (hid_host_control_msg_req(p_link,
                                     msg_type,
                                     msg_param,
                                     buf,
                                     len) == false)
        {
            ret = 3;
            goto fail_send_msg;
        }
        return true;
    }

    p_item = os_mem_zalloc2(sizeof(T_HID_MSG_ITEM) + len);
    if (p_item == NULL)
    {
        ret = 4;
        goto fail_alloc_item;
    }

    p_item->msg_type = msg_type;
    p_item->msg_param = msg_param;
    p_item->msg_len = len;
    memcpy((uint8_t *)p_item->msg, buf, len);

    os_queue_in(&p_link->msg_list, p_item);

    return hid_host_control_flush_cmd(p_link);

fail_alloc_item:
fail_send_msg:
fail_invalid_msg_len:
fail_invalid_addr:
    PROFILE_PRINT_ERROR1("hid_host_control_msg_send: failed %d", -ret);
    return false;
}

bool hid_host_interrupt_msg_send(uint8_t   bd_addr[6],
                                 uint8_t   msg_type,
                                 uint8_t   msg_param,
                                 uint8_t  *buf,
                                 uint16_t  len)
{
    T_HID_HOST_LINK *p_link;
    uint8_t         *hid_buf;
    int32_t          ret = 0;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (p_link->interrupt_chann.state != HID_STATE_CONNECTED)
    {
        ret = 2;
        goto fail_state_not_connected;
    }

    if ((HID_HDR_LENGTH + len) > p_link->interrupt_chann.remote_mtu)
    {
        ret = 3;
        goto fail_invalid_len;
    }

    hid_buf = mpa_get_l2c_buf(hid_host->queue_id_interrupt, p_link->interrupt_chann.cid,
                              0,
                              HID_HDR_LENGTH + len,
                              p_link->interrupt_chann.ds_data_offset,
                              false);
    if (hid_buf == NULL)
    {
        ret = 4;
        goto fail_get_l2c_buf;
    }

    *(hid_buf + p_link->interrupt_chann.ds_data_offset) = (msg_type << 4) | (msg_param & 0x0f);
    memcpy(hid_buf + p_link->interrupt_chann.ds_data_offset + HID_HDR_LENGTH, buf, len);

    mpa_send_l2c_data_req(hid_buf,
                          p_link->interrupt_chann.ds_data_offset,
                          p_link->interrupt_chann.cid,
                          HID_HDR_LENGTH + len,
                          false);

    return true;

fail_get_l2c_buf:
fail_invalid_len:
fail_state_not_connected:
fail_invalid_addr:
    PROFILE_PRINT_ERROR1("hid_host_interrupt_msg_send: failed %d", -ret);
    return false;
}

bool hid_host_report_size_check(uint16_t          cid,
                                uint16_t          report_id,
                                T_HID_REPORT_TYPE report_type,
                                uint16_t          report_size)
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_cid(cid);
    if (p_link != NULL)
    {
        if (p_link->proto_mode == HID_MSG_PARAM_BOOT_PROTOCOL_MODE)
        {
            switch (report_id)
            {
            case HID_BOOT_MODE_KEYBOARD_ID:
                if (report_size < 8)
                {
                    return false;
                }
                break;
            case HID_BOOT_MODE_MOUSE_ID:
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
            size =  hid_report_size_get_by_id(report_id,
                                              report_type,
                                              hid_host->descriptor_len,
                                              hid_host->descriptor);
            if ((size == 0) || (size != report_size))
            {
                return false;
            }
        }
        return true;
    }

    return false;
}

T_HID_REPORT_ID_STATUS hid_host_report_id_status_get(uint16_t cid,
                                                     uint16_t report_id)
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_cid(cid);
    if ((p_link != NULL))
    {
        if (p_link->proto_mode == HID_MSG_PARAM_BOOT_PROTOCOL_MODE)
        {
            switch (report_id)
            {
            case HID_BOOT_MODE_KEYBOARD_ID:
            case HID_BOOT_MODE_MOUSE_ID:
                return HID_REPORT_ID_VALID;
            default:
                return HID_REPORT_ID_INVALID;
            }
        }
        else
        {
            return hid_get_report_id_status(report_id, hid_host->descriptor_len, hid_host->descriptor);
        }
    }

    return HID_REPORT_ID_UNDECLARED;
}

void hid_host_control_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *p_l2c_data_ind)
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_cid(p_l2c_data_ind->cid);
    if (p_link != NULL)
    {
        int pos = 0;
        uint8_t *p_data;
        uint16_t length;
        uint8_t msg_type;
        uint8_t msg_param;
        T_HID_MSG_ITEM *msg_item;

        p_data = p_l2c_data_ind->data + p_l2c_data_ind->gap;
        length = p_l2c_data_ind->length;
        msg_type = p_data[0] >> 4;
        msg_param = p_data[0] & 0x0f;
        p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;

        T_HID_HOST_CONTROL_IND hid_host_control_ind;
        T_HID_HOST_GET_REPORT_RSP hid_host_get_report_rsp;
        T_HID_HOST_SET_REPORT_RSP hid_host_set_report_rsp;
        T_HID_HOST_GET_PROTOCOL_RSP hid_host_get_protocol_rsp;
        T_HID_HOST_SET_PROTOCOL_RSP hid_host_set_protocol_rsp;

        switch (msg_type)
        {
        case HID_MSG_TYPE_HID_CONTROL:
            if (msg_param == HID_MSG_PARAM_CONTROL_VIRTUAL_CABLE_UNPLUG)
            {
                hid_host_control_ind.control_operation = HID_MSG_PARAM_CONTROL_VIRTUAL_CABLE_UNPLUG;
                if (p_link->interrupt_chann.state == HID_STATE_CONNECTED)
                {
                    hid_host_interrupt_disconnect_req(p_link->bd_addr);
                }
                if (p_link->control_chann.state == HID_STATE_CONNECTED)
                {
                    hid_host_control_disconnect_req(p_link->bd_addr);
                }
                hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_HID_CONTROL_IND, &hid_host_control_ind);
            }
            break;

        case HID_MSG_TYPE_HANDSHAKE:
            p_link->control_chann.report_status = msg_param;
            p_link->credits = 1;
            msg_item = os_queue_out(&p_link->msg_list);
            if (msg_item != NULL)
            {
                switch (msg_item->msg_type)
                {
                case HID_MSG_TYPE_SET_PROTOCOL:
                    hid_host_set_protocol_rsp.result_code = msg_param;
                    if (msg_param == HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
                    {
                        p_link->proto_mode = msg_item->msg_param;
                        if (p_link->interrupt_chann.state == HID_STATE_DISCONNECTED)
                        {
                            hid_host_interrupt_connect_req(p_link->bd_addr);
                        }
                    }
                    else
                    {
                        if (p_link->interrupt_chann.state != HID_STATE_CONNECTED)
                        {
                            hid_host_control_disconnect_req(p_link->bd_addr);
                        }
                    }
                    hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_SET_PROTOCOL_RSP, &hid_host_set_protocol_rsp);
                    break;

                case HID_MSG_TYPE_SET_REPORT:
                    hid_host_set_report_rsp.result_code = msg_param;
                    hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_SET_REPORT_RSP, &hid_host_set_report_rsp);
                    break;

                case HID_MSG_TYPE_GET_REPORT:
                case HID_MSG_TYPE_GET_PROTOCOL:
                default:
                    break;
                }
            }
            break;

        case HID_MSG_TYPE_DATA:
            msg_item = os_queue_out(&p_link->msg_list);
            if (msg_item != NULL)
            {
                if (msg_item->msg_type == HID_MSG_TYPE_GET_PROTOCOL)
                {
                    p_link->credits = 1;
                    hid_host_get_protocol_rsp.proto_mode = p_data[1];
                    hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_GET_PROTOCOL_RSP, &hid_host_get_protocol_rsp);
                }
                else if (msg_item->msg_type == HID_MSG_TYPE_GET_REPORT)
                {
                    p_link->credits = 1;
                    hid_host_get_report_rsp.report_type = (T_HID_REPORT_TYPE)p_data[pos++] & 0x03;
                    hid_host_get_report_rsp.report_id = 0;
                    if (hid_report_id_declared(hid_host->descriptor_len, hid_host->descriptor))
                    {
                        hid_host_get_report_rsp.report_id = p_data[pos++];
                    }
                    hid_host_get_report_rsp.report_size = length - pos;
                    hid_host_get_report_rsp.p_data = &p_data[pos];
                    hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_GET_REPORT_RSP, &hid_host_get_report_rsp);
                }
            }
            break;

        default:
            PROFILE_PRINT_INFO0("ignore invalid HID Control message");
            break;
        }

        hid_host_control_flush_cmd(p_link);
    }
}

void hid_host_interrupt_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *p_l2c_data_ind)
{
    T_HID_HOST_LINK *p_link;
    uint8_t          pos = 0;

    p_link = hid_host_find_link_by_cid(p_l2c_data_ind->cid);
    if (p_link != NULL)
    {
        uint8_t *p_data = p_l2c_data_ind->data + p_l2c_data_ind->gap;
        uint16_t length = p_l2c_data_ind->length;
        T_HID_HOST_DATA_IND hid_host_data_ind;

        if (length < 2)
        {
            return;
        }
        hid_host_data_ind.report_type = p_data[pos++] & 0x03;
        hid_host_data_ind.report_id = 0;
        if (hid_report_id_declared(hid_host->descriptor_len, hid_host->descriptor))
        {
            hid_host_data_ind.report_id = p_data[pos++];
        }
        if (hid_host_report_id_status_get(p_l2c_data_ind->cid, hid_host_data_ind.report_id) ==
            HID_REPORT_ID_INVALID)
        {
            return;
        }
        if (!hid_host_report_size_check(p_l2c_data_ind->cid, hid_host_data_ind.report_id,
                                        (T_HID_REPORT_TYPE)hid_host_data_ind.report_type,
                                        length - pos))
        {
            return;
        }
        hid_host_data_ind.report_size = length - pos;
        hid_host_data_ind.p_data = &p_data[pos];
        hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_INTERRUPT_DATA_IND, &hid_host_data_ind);
    }
}

void hid_host_l2c_callback(void        *p_buf,
                           T_PROTO_MSG  l2c_msg)
{
    switch (l2c_msg)
    {
    case L2C_CONN_IND:
        {
            T_MPA_L2C_CONN_IND *ind = (T_MPA_L2C_CONN_IND *)p_buf;
            T_HID_HOST_LINK *p_link = hid_host_find_link_by_addr(ind->bd_addr);

            if (ind->proto_id == hid_host->queue_id_control)
            {
                if (p_link == NULL)
                {
                    p_link = hid_host_alloc_link(ind->bd_addr);
                    if (p_link != NULL)
                    {
                        p_link->control_chann.cid = ind->cid;
                        p_link->control_chann.state = HID_STATE_CONNECTING;
                        hid_host->host_cback(ind->bd_addr, HID_HOST_MSG_CONTROL_CONN_IND, p_buf);
                    }
                    else
                    {
                        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                              HID_CONTROL_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                    }
                }
                else
                {
                    mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                          HID_CONTROL_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                }
            }
            else if (ind->proto_id == hid_host->queue_id_interrupt)
            {
                if (p_link == NULL)
                {
                    mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                          HID_INTERRUPT_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                }
                else
                {
                    if (p_link->interrupt_chann.state == HID_STATE_DISCONNECTED)
                    {
                        p_link->interrupt_chann.cid = ind->cid;
                        p_link->interrupt_chann.state = HID_STATE_CONNECTING;
                        hid_host->host_cback(ind->bd_addr, HID_HOST_MSG_INTERRUPT_CONN_IND, p_buf);
                    }
                    else
                    {
                        mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, ind->cid,
                                              HID_INTERRUPT_MTU_SIZE, MPA_L2C_MODE_BASIC, 0xFFFF);
                    }
                }
            }
        }
        break;

    case L2C_CONN_RSP:
        {
            T_MPA_L2C_CONN_RSP *ind = (T_MPA_L2C_CONN_RSP *)p_buf;
            T_HID_HOST_LINK *p_link = hid_host_find_link_by_addr(ind->bd_addr);

            if (ind->proto_id == hid_host->queue_id_control)
            {
                if (ind->cause == 0)
                {
                    if (p_link != NULL)
                    {
                        p_link->control_chann.cid = ind->cid;
                        hid_host->host_cback(ind->bd_addr, HID_HOST_MSG_CONTROL_CONN_RSP, p_buf);
                    }
                    else
                    {
                        hid_host_control_disconnect_req(ind->bd_addr);
                    }
                }
                else
                {
                    if (p_link != NULL)
                    {
                        hid_host_free_link(p_link);
                    }

                    hid_host->host_cback(ind->bd_addr, HID_HOST_MSG_CONTROL_CONN_FAIL, &ind->cause);
                }
            }
            else if (ind->proto_id == hid_host->queue_id_interrupt)
            {
                if (ind->cause == 0)
                {
                    if (p_link != NULL)
                    {
                        p_link->interrupt_chann.cid = ind->cid;
                        hid_host->host_cback(ind->bd_addr, HID_HOST_MSG_INTERRUPT_CONN_RSP, p_buf);
                    }
                    else
                    {
                        hid_host_interrupt_disconnect_req(ind->bd_addr);
                    }
                }
                else
                {
                    if (p_link != NULL)
                    {
                        memset(&p_link->interrupt_chann, 0, sizeof(T_HID_INTERRUPT_CHANN));
                    }

                    hid_host->host_cback(ind->bd_addr, HID_HOST_MSG_INTERRUPT_CONN_FAIL, &ind->cause);
                }
            }
        }
        break;

    case L2C_CONN_CMPL:
        {
            T_MPA_L2C_CONN_CMPL_INFO *p_info = (T_MPA_L2C_CONN_CMPL_INFO *)p_buf;
            T_HID_HOST_LINK *p_link;

            p_link = hid_host_find_link_by_cid(p_info->cid);
            if (p_link != NULL)
            {
                T_HID_HOST_CONN_CMPL_INFO info;

                memcpy(info.bd_addr, p_info->bd_addr, 6);
                info.cause = p_info->cause;

                if (p_info->proto_id == hid_host->queue_id_control)
                {
                    if (p_info->cause)
                    {
                        hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_CONTROL_CONN_FAIL, &p_info->cause);
                        hid_host_free_link(p_link);
                    }
                    else
                    {
                        p_link->control_chann.state = HID_STATE_CONNECTED;
                        p_link->control_chann.remote_mtu = p_info->remote_mtu;
                        p_link->control_chann.ds_data_offset = p_info->ds_data_offset;
                        hid_host->host_cback(p_info->bd_addr, HID_HOST_MSG_CONTROL_CONN_CMPL, &info);
                    }
                }
                else if (p_info->proto_id == hid_host->queue_id_interrupt)
                {
                    if (p_info->cause)
                    {
                        memset(&p_link->interrupt_chann, 0, sizeof(T_HID_INTERRUPT_CHANN));
                        hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_INTERRUPT_CONN_FAIL, &p_info->cause);
                    }
                    else
                    {
                        p_link->interrupt_chann.state = HID_STATE_CONNECTED;
                        p_link->interrupt_chann.remote_mtu = p_info->remote_mtu;
                        p_link->interrupt_chann.ds_data_offset = p_info->ds_data_offset;
                        hid_host->host_cback(p_info->bd_addr, HID_HOST_MSG_INTERRUPT_CONN_CMPL, &info);
                    }
                }
            }
        }
        break;

    case L2C_DATA_IND:
        {
            T_MPA_L2C_DATA_IND *ind = (T_MPA_L2C_DATA_IND *)p_buf;
            if (ind->proto_id == hid_host->queue_id_control)
            {
                hid_host_control_handle_l2c_data_ind(ind);
            }
            else if (ind->proto_id == hid_host->queue_id_interrupt)
            {
                hid_host_interrupt_handle_l2c_data_ind(ind);
            }
        }
        break;

    case L2C_DISCONN_IND:
        {
            T_MPA_L2C_DISCONN_IND *ind = (T_MPA_L2C_DISCONN_IND *)p_buf;
            T_HID_HOST_LINK *p_link = hid_host_find_link_by_cid(ind->cid);

            if (ind->proto_id == hid_host->queue_id_control)
            {
                if (p_link != NULL)
                {
                    hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_CONTROL_DISCONNECTED, &ind->cause);
                    hid_host_free_link(p_link);
                }
                mpa_send_l2c_disconn_cfm(ind->cid);
            }
            else if (ind->proto_id == hid_host->queue_id_interrupt)
            {
                if (p_link != NULL)
                {
                    hid_host->host_cback(p_link->bd_addr, HID_HOST_MSG_INTERRUPT_DISCONNECTED, &ind->cause);
                    memset(&p_link->interrupt_chann, 0, sizeof(T_HID_INTERRUPT_CHANN));
                }
                mpa_send_l2c_disconn_cfm(ind->cid);
            }
        }
        break;

    case L2C_DISCONN_RSP:
        {
            T_MPA_L2C_DISCONN_RSP *rsp = (T_MPA_L2C_DISCONN_RSP *)p_buf;
            T_HID_HOST_LINK *p_link;

            p_link = hid_host_find_link_by_cid(rsp->cid);
            if (p_link != NULL)
            {
                T_HID_HOST_DISCONN_RSP_INFO info;

                memcpy(info.bd_addr, p_link->bd_addr, 6);
                info.cause = rsp->cause;

                if (rsp->proto_id == hid_host->queue_id_control)
                {
                    hid_host_free_link(p_link);
                    hid_host->host_cback(info.bd_addr, HID_HOST_MSG_CONTROL_DISCONNECTED, &info);
                }
                else if (rsp->proto_id == hid_host->queue_id_interrupt)
                {
                    p_link->interrupt_chann.state = HID_STATE_DISCONNECTED;
                    p_link->interrupt_chann.cid = 0;
                    hid_host->host_cback(info.bd_addr, HID_HOST_MSG_INTERRUPT_DISCONNECTED, &info);
                }
            }
        }
        break;

    default:
        break;
    }
}

bool hid_host_init(uint8_t          link_num,
                   bool             boot_proto_mode,
                   P_HID_HOST_CBACK host_cback)
{
    int32_t ret = 0;

    hid_host = os_mem_alloc2(sizeof(T_HID_HOST));
    if (hid_host == NULL)
    {
        ret = 1;
        goto fail_alloc_hid;
    }

    hid_host->link_num         = link_num;
    hid_host->boot_proto_mode  = boot_proto_mode;
    hid_host->descriptor       = NULL;
    hid_host->descriptor_len   = 0;
    hid_host->host_cback       = host_cback;

    hid_host->link = os_mem_zalloc2(hid_host->link_num * sizeof(T_HID_HOST_LINK));
    if (hid_host->link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (mpa_reg_l2c_proto(PSM_HID_CONTROL, hid_host_l2c_callback,
                          &hid_host->queue_id_control) == false)
    {
        ret = 3;
        goto fail_reg_l2c_hid_control;
    }
    if (mpa_reg_l2c_proto(PSM_HID_INTERRUPT, hid_host_l2c_callback,
                          &hid_host->queue_id_interrupt) == false)
    {
        ret = 4;
        goto fail_reg_l2c_hid_interrupt;
    }

    return true;

fail_reg_l2c_hid_interrupt:
fail_reg_l2c_hid_control:
    os_mem_free(hid_host->link);
    hid_host->link = NULL;
fail_alloc_link:
    os_mem_free(hid_host);
    hid_host = NULL;
fail_alloc_hid:
    PROFILE_PRINT_ERROR1("hid_host_init: failed %d", -ret);
    return false;
}

bool hid_host_descriptor_set(const uint8_t *descriptor,
                             uint16_t       len)
{
    if (hid_host != NULL)
    {
        hid_host->descriptor       = descriptor;
        hid_host->descriptor_len   = len;
        return true;
    }

    return false;
}

bool hid_host_get_roleswap_info(uint8_t                   bd_addr[6],
                                T_ROLESWAP_HID_HOST_INFO *p_info)
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        p_info->proto_mode              = p_link->proto_mode;
        p_info->control_cid             = p_link->control_chann.cid;
        p_info->control_remote_mtu      = p_link->control_chann.remote_mtu;
        p_info->control_state           = p_link->control_chann.state;
        p_info->control_data_offset     = p_link->control_chann.ds_data_offset;
        p_info->interrupt_cid           = p_link->interrupt_chann.cid;
        p_info->interrupt_remote_mtu    = p_link->interrupt_chann.remote_mtu;
        p_info->interrupt_state         = p_link->interrupt_chann.state;
        p_info->interrupt_data_offset   = p_link->interrupt_chann.ds_data_offset;

        return true;
    }

    return false;
}

bool hid_host_set_roleswap_info(uint8_t                   bd_addr[6],
                                T_ROLESWAP_HID_HOST_INFO *p_info)
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = hid_host_alloc_link(bd_addr);
        if (p_link == NULL)
        {
            return false;
        }
    }

    p_link->proto_mode                      = p_info->proto_mode;
    p_link->control_chann.cid               = p_info->control_cid;
    p_link->control_chann.remote_mtu        = p_info->control_remote_mtu;
    p_link->control_chann.state             = (T_HID_STATE)p_info->control_state;
    p_link->control_chann.ds_data_offset    = p_info->control_data_offset;
    p_link->interrupt_chann.cid             = p_info->interrupt_cid;
    p_link->interrupt_chann.remote_mtu      = p_info->interrupt_remote_mtu;
    p_link->interrupt_chann.state           = (T_HID_STATE)p_info->interrupt_state;
    p_link->interrupt_chann.ds_data_offset  = p_info->interrupt_data_offset;

    return true;
}

bool hid_host_del_roleswap_info(uint8_t bd_addr[6])
{
    T_HID_HOST_LINK *p_link;

    p_link = hid_host_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        hid_host_free_link(p_link);
        return true;
    }

    return false;
}
#else
#include <stdint.h>
#include <stdbool.h>

#include "hid_host.h"

bool hid_host_control_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool hid_host_interrupt_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool hid_host_control_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool hid_host_interrupt_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool hid_host_control_connect_cfm(uint8_t bd_addr[6],
                                  bool    accept)
{
    return false;
}

bool hid_host_interrupt_connect_cfm(uint8_t bd_addr[6],
                                    bool    accept)
{
    return false;
}

bool hid_host_control_msg_send(uint8_t  bd_addr[6],
                               uint8_t  msg_type,
                               uint8_t  msg_param,
                               uint8_t *buf,
                               uint8_t  len)
{
    return false;
}

bool hid_host_interrupt_msg_send(uint8_t   bd_addr[6],
                                 uint8_t   msg_type,
                                 uint8_t   msg_param,
                                 uint8_t  *buf,
                                 uint16_t  len)
{
    return false;
}

bool hid_host_init(uint8_t          link_num,
                   bool             boot_proto_mode,
                   P_HID_HOST_CBACK cback)
{
    return false;
}

bool hid_host_descriptor_set(const uint8_t *descriptor,
                             uint16_t       len)
{
    return false;
}

bool hid_host_get_roleswap_info(uint8_t                   bd_addr[6],
                                T_ROLESWAP_HID_HOST_INFO *p_info)
{
    return false;
}

bool hid_host_set_roleswap_info(uint8_t                   bd_addr[6],
                                T_ROLESWAP_HID_HOST_INFO *p_info)
{
    return false;
}

bool hid_host_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}
#endif
