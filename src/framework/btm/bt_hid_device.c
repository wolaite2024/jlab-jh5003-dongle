/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HID_DEVICE_SUPPORT == 1)

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "mpa.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_hid_device.h"
#include "bt_hid_parser.h"
#include "bt_hid_int.h"

/*hid_initiator_flag*/
#define INITIATE_CONN_MASK       0x01
#define INITIATE_DISCONN_MASK    0x02

typedef struct t_bt_hid_device_link
{
    struct t_bt_hid_device_link *next;
    uint8_t                      bd_addr[6];
    uint8_t                      proto_mode;
    uint8_t                      int_flag;
    T_BT_HID_CONTROL_CHANN       control_chann;
    T_BT_HID_INTERRUPT_CHANN     interrupt_chann;
} T_BT_HID_DEVICE_LINK;

typedef struct t_bt_hid_device
{
    T_OS_QUEUE           list;
    uint8_t              queue_id_control;
    uint8_t              queue_id_interrupt;
    bool                 boot_proto_mode;
    const uint8_t       *descriptor;
    uint16_t             descriptor_len;
} T_BT_HID_DEVICE;

static T_BT_HID_DEVICE *bt_hid_device;

T_BT_HID_DEVICE_LINK *bt_hid_device_alloc_link(uint8_t bd_addr[6])
{
    T_BT_HID_DEVICE_LINK *link;

    link = malloc(sizeof(T_BT_HID_DEVICE_LINK));
    if (link != NULL)
    {
        link->control_chann.state = BT_HID_STATE_DISCONNECTED;
        link->interrupt_chann.state = BT_HID_STATE_DISCONNECTED;
        link->proto_mode = BT_HID_MSG_PARAM_REPORT_PROTOCOL_MODE;
        memcpy(link->bd_addr, bd_addr, 6);
        os_queue_in(&bt_hid_device->list, link);
    }

    return link;
}

void bt_hid_device_free_link(T_BT_HID_DEVICE_LINK *link)
{
    if (os_queue_delete(&bt_hid_device->list, link) == true)
    {
        free(link);
    }
}

T_BT_HID_DEVICE_LINK *bt_hid_device_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_HID_DEVICE_LINK *link;

    link = os_queue_peek(&bt_hid_device->list, 0);
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

T_BT_HID_DEVICE_LINK *bt_hid_device_find_link_by_cid(uint16_t cid)
{
    T_BT_HID_DEVICE_LINK *link;

    link = os_queue_peek(&bt_hid_device->list, 0);
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

bool bt_hid_device_report_size_check(T_BT_HID_DEVICE_LINK *link,
                                     uint16_t              report_id,
                                     T_BT_HID_REPORT_TYPE  report_type,
                                     uint16_t              report_size)
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
                                             bt_hid_device->descriptor_len,
                                             bt_hid_device->descriptor);
        if ((size == 0) || (size != report_size))
        {
            return false;
        }
    }
    return true;
}

T_BT_HID_REPORT_ID_STATUS bt_hid_device_report_id_status_get(T_BT_HID_DEVICE_LINK *link,
                                                             uint16_t              report_id)
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
                                           bt_hid_device->descriptor_len,
                                           bt_hid_device->descriptor);
    }

}

static int bt_hid_device_report_size_get(T_BT_HID_DEVICE_LINK *link,
                                         uint16_t              report_id,
                                         T_BT_HID_REPORT_TYPE  report_type,
                                         uint16_t              descriptor_len,
                                         const uint8_t        *descriptor)
{
    if (link->proto_mode == BT_HID_MSG_PARAM_BOOT_PROTOCOL_MODE)
    {
        switch (report_id)
        {
        case BT_HID_BOOT_MODE_KEYBOARD_ID:
            return 8;
        case BT_HID_BOOT_MODE_MOUSE_ID:
            return 3;
        default:
            return 0;
        }
    }
    else
    {
        return bt_hid_report_size_get_by_id(report_id,
                                            report_type,
                                            descriptor_len,
                                            descriptor);
    }
}

uint8_t bt_hid_set_report_status_get(T_BT_HID_DEVICE_LINK *link,
                                     T_BT_HID_REPORT_TYPE  report_type,
                                     uint16_t              report_size,
                                     uint8_t              *report)
{
    uint8_t  offset = 0;
    uint16_t report_id = 0;

    if (bt_hid_report_id_declared(bt_hid_device->descriptor_len, bt_hid_device->descriptor))
    {
        report_id = report[offset++];
        if (bt_hid_device_report_id_status_get(link, report_id) == BT_HID_REPORT_ID_INVALID)
        {
            return BT_HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID;
        }
    }

    if (!bt_hid_device_report_size_check(link, report_id, report_type, report_size - offset))
    {
        return BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
    }

    return BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;
}

bool bt_hid_device_control_msg_send(uint8_t   bd_addr[6],
                                    uint8_t   msg_type,
                                    uint8_t   msg_param,
                                    uint8_t  *buf,
                                    uint16_t  len)
{
    T_BT_HID_DEVICE_LINK *link;
    uint8_t              *hid_buf;
    int32_t               ret = 0;

    link = bt_hid_device_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (BT_HID_HDR_LENGTH + len > link->control_chann.remote_mtu)
    {
        ret = 2;
        goto fail_invalid_len;
    }

    hid_buf = mpa_get_l2c_buf(bt_hid_device->queue_id_control,
                              link->control_chann.cid,
                              0,
                              BT_HID_HDR_LENGTH + len,
                              link->control_chann.ds_data_offset,
                              false);
    if (hid_buf == NULL)
    {
        ret = 3;
        goto fail_get_l2c_buf;
    }

    hid_buf[link->control_chann.ds_data_offset] = (msg_type << 4) | (msg_param & 0x0f);
    memcpy(hid_buf + link->control_chann.ds_data_offset + BT_HID_HDR_LENGTH, buf, len);

    mpa_send_l2c_data_req(hid_buf,
                          link->control_chann.ds_data_offset,
                          link->control_chann.cid,
                          BT_HID_HDR_LENGTH + len,
                          false);

    return true;

fail_get_l2c_buf:
fail_invalid_len:
fail_invalid_addr:
    PROFILE_PRINT_ERROR3("bt_hid_device_control_msg_send: failed %d, bd_addr %s, msg_type 0x%02x",
                         -ret, TRACE_BDADDR(bd_addr), msg_type);
    return false;
}

bool bt_hid_device_interrupt_msg_send(uint8_t   bd_addr[6],
                                      uint8_t   msg_type,
                                      uint8_t   msg_param,
                                      uint8_t  *buf,
                                      uint16_t  len)
{
    T_BT_HID_DEVICE_LINK *link;
    uint8_t              *hid_buf;
    int32_t               ret = 0;

    link = bt_hid_device_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (link->interrupt_chann.state != BT_HID_STATE_CONNECTED)
    {
        ret = 2;
        goto fail_invalid_state;
    }

    if ((BT_HID_HDR_LENGTH + len) > link->interrupt_chann.remote_mtu)
    {
        ret = 3;
        goto fail_invalid_len;
    }

    hid_buf = mpa_get_l2c_buf(bt_hid_device->queue_id_interrupt,
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
fail_invalid_state:
fail_invalid_addr:
    PROFILE_PRINT_ERROR3("hid_device_interrupt_msg_send: failed %d, bd_addr %s, msg_type 0x%02x",
                         -ret, TRACE_BDADDR(bd_addr), msg_type);
    return false;
}

void bt_hid_device_handle_get_report(T_BT_HID_DEVICE_LINK *link,
                                     uint8_t              *p_data,
                                     uint16_t              length)
{
    T_BT_HID_DEVICE_GET_REPORT_IND get_report_ind;
    uint16_t                       report_size;
    uint8_t                        offset = 0;
    T_BT_MSG_PAYLOAD               payload;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    get_report_ind.report_type = (T_BT_HID_DEVICE_REPORT_TYPE)p_data[offset++] & 0x03;
    get_report_ind.report_size = 0;
    get_report_ind.report_id = 0;

    switch (link->proto_mode)
    {
    case BT_HID_MSG_PARAM_BOOT_PROTOCOL_MODE:
        if (length < 2)
        {
            link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
            break;
        }
        get_report_ind.report_id = p_data[offset++];
        break;

    case BT_HID_MSG_PARAM_REPORT_PROTOCOL_MODE:
        if (!bt_hid_report_id_declared(bt_hid_device->descriptor_len, bt_hid_device->descriptor))
        {
            if (p_data[0] & 0x08)
            {
                if (length > 3)
                {
                    link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID;
                }
            }
            else
            {
                if (length > 1)
                {
                    link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID;
                }
            }
            break;
        }
        if (length < 2)
        {
            link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
            break;
        }
        get_report_ind.report_id = p_data[offset++];
        break;

    default:
        break;
    }

    if (link->control_chann.report_status != BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
    {
        return;
    }

    if (bt_hid_device_report_id_status_get(link, get_report_ind.report_id) ==
        BT_HID_REPORT_ID_INVALID)
    {
        link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID;
        return;
    }

    if (!bt_hid_report_type_is_valid(get_report_ind.report_type,
                                     bt_hid_device->descriptor_len,
                                     bt_hid_device->descriptor))
    {
        link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
        return;
    }

    report_size = bt_hid_device_report_size_get(link,
                                                get_report_ind.report_id,
                                                (T_BT_HID_REPORT_TYPE)get_report_ind.report_type,
                                                bt_hid_device->descriptor_len,
                                                bt_hid_device->descriptor);

    if ((p_data[0] & 0x08) && (length >= (offset + 1)))
    {
        uint16_t buffer_size;
        uint16_t tmp;

        buffer_size = (uint16_t)(((uint16_t) p_data[offset]) | (((uint16_t)p_data[offset + 1]) << 8));
        if (buffer_size <= report_size)
        {
            if (get_report_ind.report_id == 0)
            {
                tmp = buffer_size;
            }
            else
            {
                tmp = buffer_size - 1;
            }
        }
        else
        {
            tmp = report_size;
        }
        if (BT_HID_CONTROL_MTU_SIZE < tmp)
        {
            tmp = BT_HID_CONTROL_MTU_SIZE;
        }
        get_report_ind.report_size = tmp;
    }
    else
    {
        if (report_size > BT_HID_CONTROL_MTU_SIZE)
        {
            get_report_ind.report_size = BT_HID_CONTROL_MTU_SIZE;
        }
        else
        {
            get_report_ind.report_size = report_size;
        }
    }

    payload.msg_buf = &get_report_ind;
    bt_mgr_dispatch(BT_MSG_HID_DEVICE_GET_REPORT_IND, &payload);
}

void bt_hid_device_handle_set_report(T_BT_HID_DEVICE_LINK *link,
                                     uint8_t              *p_data,
                                     uint16_t              length)
{
    T_BT_HID_DEVICE_SET_REPORT_IND set_report_ind;
    uint8_t                        offset = 0;
    T_BT_MSG_PAYLOAD               payload;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    set_report_ind.report_id = 0;
    set_report_ind.report_size = 0;
    set_report_ind.report_type = (T_BT_HID_DEVICE_REPORT_TYPE)p_data[offset++] & 0x03;

    if (length < 1)
    {
        link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
        return;
    }

    switch (link->proto_mode)
    {
    case BT_HID_MSG_PARAM_BOOT_PROTOCOL_MODE:
        if (length < 3)
        {
            link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
            break;
        }

        link->control_chann.report_status = bt_hid_set_report_status_get(link,
                                                                         (T_BT_HID_REPORT_TYPE)set_report_ind.report_type,
                                                                         length - 1, &p_data[1]);

        if (link->control_chann.report_status == BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
        {
            set_report_ind.report_id = p_data[offset++];
            set_report_ind.p_data = &p_data[offset];
            set_report_ind.report_size = length - offset;
            bt_hid_device_control_msg_send(link->bd_addr,
                                           BT_HID_MSG_TYPE_HANDSHAKE,
                                           link->control_chann.report_status,
                                           NULL,
                                           0);

            payload.msg_buf = &set_report_ind;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_SET_REPORT_IND, &payload);
        }
        break;

    case BT_HID_MSG_PARAM_REPORT_PROTOCOL_MODE:
        link->control_chann.report_status = bt_hid_set_report_status_get(link,
                                                                         (T_BT_HID_REPORT_TYPE)set_report_ind.report_type,
                                                                         length - 1, &p_data[1]);

        if (link->control_chann.report_status != BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
        {
            break;
        }
        if (length >= 2)
        {
            if (bt_hid_report_id_declared(bt_hid_device->descriptor_len, bt_hid_device->descriptor))
            {
                set_report_ind.report_id = p_data[offset++];
                set_report_ind.p_data = &p_data[offset];
                set_report_ind.report_size = length - offset;
            }
            else
            {
                set_report_ind.p_data = &p_data[offset];
                set_report_ind.report_size = length - offset;
            }

            bt_hid_device_control_msg_send(link->bd_addr,
                                           BT_HID_MSG_TYPE_HANDSHAKE,
                                           link->control_chann.report_status,
                                           NULL,
                                           0);
            payload.msg_buf = &set_report_ind;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_SET_REPORT_IND, &payload);
        }
        else
        {
            uint8_t tmp[] = {0};
            set_report_ind.p_data = tmp;
            bt_hid_device_control_msg_send(link->bd_addr,
                                           BT_HID_MSG_TYPE_HANDSHAKE,
                                           link->control_chann.report_status,
                                           NULL,
                                           0);
            payload.msg_buf = &set_report_ind;
            bt_mgr_dispatch(BT_MSG_HID_DEVICE_SET_REPORT_IND, &payload);
        }
        break;

    default:
        break;
    }
}

void bt_hid_device_handle_get_protocol(T_BT_HID_DEVICE_LINK *link,
                                       uint8_t              *p_data,
                                       uint16_t              length)
{
    T_BT_HID_DEVICE_GET_PROTOCOL_IND get_protocol_ind;
    T_BT_MSG_PAYLOAD                 payload;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    if (length != 1)
    {
        link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
        return;
    }
    link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;
    get_protocol_ind.proto_mode = link->proto_mode;
    bt_hid_device_control_msg_send(link->bd_addr,
                                   BT_HID_MSG_TYPE_DATA,
                                   0,
                                   &link->proto_mode,
                                   1);
    payload.msg_buf = &get_protocol_ind;
    bt_mgr_dispatch(BT_MSG_HID_DEVICE_GET_PROTOCOL_IND, &payload);
}

void bt_hid_device_handle_set_protocol(T_BT_HID_DEVICE_LINK *link,
                                       uint8_t              *p_data,
                                       uint16_t              length)
{
    T_BT_HID_DEVICE_SET_PROTOCOL_IND set_protocol_ind;
    T_BT_MSG_PAYLOAD                 payload;
    uint16_t                         proto_mode;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    if (length != 1)
    {
        link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
        return;
    }
    proto_mode = p_data[0] & 0x01;
    if (proto_mode == BT_HID_MSG_PARAM_BOOT_PROTOCOL_MODE && bt_hid_device->boot_proto_mode == false)
    {
        link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
        return;
    }
    link->proto_mode = proto_mode;
    link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;
    set_protocol_ind.proto_mode = link->proto_mode;
    bt_hid_device_control_msg_send(link->bd_addr,
                                   BT_HID_MSG_TYPE_HANDSHAKE,
                                   link->control_chann.report_status,
                                   NULL,
                                   0);
    payload.msg_buf = &set_protocol_ind;
    bt_mgr_dispatch(BT_MSG_HID_DEVICE_SET_PROTOCOL_IND, &payload);
}

void bt_hid_device_handle_set_idle(T_BT_HID_DEVICE_LINK *link,
                                   uint8_t              *p_data,
                                   uint16_t              length)
{
    T_BT_HID_DEVICE_SET_IDLE_IND set_idle_ind;
    T_BT_MSG_PAYLOAD             payload;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    set_idle_ind.report_status = link->control_chann.report_status;
    bt_hid_device_control_msg_send(link->bd_addr,
                                   BT_HID_MSG_TYPE_HANDSHAKE,
                                   link->control_chann.report_status,
                                   NULL,
                                   0);
    payload.msg_buf = &set_idle_ind;
    bt_mgr_dispatch(BT_MSG_HID_DEVICE_SET_IDLE_IND, &payload);
}

void bt_hid_device_handle_hid_control(T_BT_HID_DEVICE_LINK *link,
                                      uint8_t              *p_data,
                                      uint16_t              length)
{
    T_BT_HID_DEVICE_CONTROL_IND control_ind;
    T_BT_MSG_PAYLOAD            payload;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    control_ind.control_operation = p_data[0] & 0x0f;
    if (control_ind.control_operation == BT_HID_MSG_PARAM_CONTROL_VIRTUAL_CABLE_UNPLUG)
    {
        bt_hid_device_disconnect_req(link->bd_addr);
    }
}

void bt_hid_device_control_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_BT_HID_DEVICE_LINK *link;

    link = bt_hid_device_find_link_by_cid(data_ind->cid);
    if (link != NULL)
    {
        uint8_t           *p_data;
        uint16_t           length;
        uint8_t            msg_type;

        p_data = data_ind->data + data_ind->gap;
        length = data_ind->length;
        msg_type = p_data[0] >> 4;
        link->control_chann.report_status = BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;

        switch (msg_type)
        {
        case BT_HID_MSG_TYPE_GET_REPORT:
            bt_hid_device_handle_get_report(link, p_data, length);
            break;

        case BT_HID_MSG_TYPE_SET_REPORT:
            bt_hid_device_handle_set_report(link, p_data, length);
            break;

        case BT_HID_MSG_TYPE_GET_PROTOCOL:
            bt_hid_device_handle_get_protocol(link, p_data, length);
            break;

        case BT_HID_MSG_TYPE_SET_PROTOCOL:
            bt_hid_device_handle_set_protocol(link, p_data, length);
            break;

        case BT_HID_MSG_TYPE_SET_IDLE:
            bt_hid_device_handle_set_idle(link, p_data, length);
            break;

        case BT_HID_MSG_TYPE_HID_CONTROL:
            bt_hid_device_handle_hid_control(link, p_data, length);
            break;

        default:
            bt_hid_device_control_msg_send(link->bd_addr,
                                           BT_HID_MSG_TYPE_HANDSHAKE,
                                           BT_HID_MSG_PARAM_HANDSHAKE_ERR_UNSUPPORTED_REQUEST,
                                           NULL,
                                           0);
            break;
        }

        if (link->control_chann.report_status != BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
        {
            bt_hid_device_control_msg_send(link->bd_addr,
                                           BT_HID_MSG_TYPE_HANDSHAKE,
                                           link->control_chann.report_status,
                                           NULL,
                                           0);
        }
    }
}

void bt_hid_device_interrupt_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_BT_HID_DEVICE_LINK *link;

    link = bt_hid_device_find_link_by_cid(data_ind->cid);
    if (link != NULL)
    {
        uint8_t                  *p_data;
        uint16_t                  length;
        T_BT_MSG_PAYLOAD          payload;
        T_BT_HID_DEVICE_DATA_IND  ind;
        uint8_t                   offset = 0;

        p_data = data_ind->data + data_ind->gap;
        length = data_ind->length;
        if (length <= BT_HID_HDR_LENGTH)
        {
            return;
        }
        ind.report_type = p_data[offset++] & 0x03;
        ind.report_id = 0;
        if (bt_hid_report_id_declared(bt_hid_device->descriptor_len, bt_hid_device->descriptor))
        {
            ind.report_id = p_data[offset++];
        }
        if (bt_hid_device_report_id_status_get(link, ind.report_id) ==
            BT_HID_REPORT_ID_INVALID)
        {
            return;
        }
        if (!bt_hid_device_report_size_check(link, ind.report_id,
                                             (T_BT_HID_REPORT_TYPE)ind.report_type,
                                             length - offset))
        {
            return;
        }
        ind.report_size = length - offset;
        ind.p_data = &p_data[offset];
        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &ind;
        bt_mgr_dispatch(BT_MSG_HID_DEVICE_INTERRUPT_DATA_IND, &payload);
    }
}

static void bt_hid_device_cback(void        *p_buf,
                                T_PROTO_MSG  msg)
{
    T_BT_HID_DEVICE_LINK *link;
    T_BT_MSG_PAYLOAD      payload;

    BTM_PRINT_INFO1("bt_hid_device_cback: msg 0x%02x", msg);

    switch (msg)
    {
    case L2C_CONN_IND:
        {
            T_MPA_L2C_CONN_IND *ind = (T_MPA_L2C_CONN_IND *)p_buf;

            link = bt_hid_device_find_link_by_addr(ind->bd_addr);
            if (ind->proto_id == bt_hid_device->queue_id_control)
            {
                if (link == NULL)
                {
                    link = bt_hid_device_alloc_link(ind->bd_addr);
                    if (link != NULL)
                    {
                        link->control_chann.cid = ind->cid;
                        link->control_chann.state = BT_HID_STATE_CONNECTING;
                        link->int_flag = 0;
                        memcpy(payload.bd_addr, ind->bd_addr, 6);
                        bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_IND, &payload);
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
            else if (ind->proto_id == bt_hid_device->queue_id_interrupt)
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

            link = bt_hid_device_find_link_by_addr(rsp->bd_addr);
            if (link != NULL)
            {
                if (rsp->proto_id == bt_hid_device->queue_id_control)
                {
                    if (rsp->cause == 0)
                    {
                        link->control_chann.cid = rsp->cid;
                    }
                    else
                    {
                        bt_hid_device_free_link(link);

                        memcpy(payload.bd_addr, rsp->bd_addr, 6);
                        payload.msg_buf = &rsp->cause;
                        bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_FAIL, &payload);
                    }
                }
                else if (rsp->proto_id == bt_hid_device->queue_id_interrupt)
                {
                    if (rsp->cause == 0)
                    {
                        link->interrupt_chann.cid = rsp->cid;
                    }
                    else
                    {
                        bt_hid_device_free_link(link);

                        memcpy(payload.bd_addr, rsp->bd_addr, 6);
                        payload.msg_buf = &rsp->cause;
                        bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_FAIL, &payload);
                    }
                }
            }
        }
        break;

    case L2C_CONN_CMPL:
        {
            T_MPA_L2C_CONN_CMPL_INFO *p_info = (T_MPA_L2C_CONN_CMPL_INFO *)p_buf;

            link = bt_hid_device_find_link_by_cid(p_info->cid);
            if (link != NULL)
            {
                memcpy(payload.bd_addr, p_info->bd_addr, 6);

                if (p_info->proto_id == bt_hid_device->queue_id_control)
                {
                    if (p_info->cause == 0)
                    {
                        link->control_chann.state = BT_HID_STATE_CONNECTED;
                        link->control_chann.remote_mtu = p_info->remote_mtu;
                        link->control_chann.ds_data_offset = p_info->ds_data_offset;
                        if (link->int_flag & INITIATE_CONN_MASK)
                        {
                            link->interrupt_chann.state = BT_HID_STATE_CONNECTING;
                            mpa_send_l2c_conn_req(PSM_HID_INTERRUPT, UUID_HIDP, bt_hid_device->queue_id_interrupt,
                                                  BT_HID_INTERRUPT_MTU_SIZE, link->bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);
                        }
                    }
                    else
                    {
                        bt_hid_device_free_link(link);

                        payload.msg_buf = &p_info->cause;
                        bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_FAIL, &payload);
                    }
                }
                else if (p_info->proto_id == bt_hid_device->queue_id_interrupt)
                {
                    if (p_info->cause == 0)
                    {
                        payload.msg_buf = &p_info->cause;
                        link->interrupt_chann.state = BT_HID_STATE_CONNECTED;
                        link->interrupt_chann.remote_mtu = p_info->remote_mtu;
                        link->interrupt_chann.ds_data_offset = p_info->ds_data_offset;
                        bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_CMPL, &payload);
                        bt_pm_sm(p_info->bd_addr, BT_PM_EVENT_SNIFF_ENTER_REQ);
                    }
                    else
                    {
                        bt_hid_device_free_link(link);

                        payload.msg_buf = &p_info->cause;
                        bt_mgr_dispatch(BT_MSG_HID_DEVICE_CONN_FAIL, &payload);
                    }
                }
            }
        }
        break;

    case L2C_DATA_IND:
        {
            T_MPA_L2C_DATA_IND *ind = (T_MPA_L2C_DATA_IND *)p_buf;

            if (ind->proto_id == bt_hid_device->queue_id_control)
            {
                bt_hid_device_control_handle_l2c_data_ind(ind);
            }
            else if (ind->proto_id == bt_hid_device->queue_id_interrupt)
            {
                bt_hid_device_interrupt_handle_l2c_data_ind(ind);
            }
        }
        break;

    case L2C_DISCONN_IND:
        {
            T_MPA_L2C_DISCONN_IND *ind = (T_MPA_L2C_DISCONN_IND *)p_buf;

            link = bt_hid_device_find_link_by_cid(ind->cid);
            if (link != NULL)
            {
                memcpy(payload.bd_addr, link->bd_addr, 6);

                if (ind->proto_id == bt_hid_device->queue_id_control)
                {
                    bt_hid_device_free_link(link);

                    payload.msg_buf = &ind->cause;
                    mpa_send_l2c_disconn_cfm(ind->cid);
                    bt_mgr_dispatch(BT_MSG_HID_DEVICE_DISCONN_CMPL, &payload);
                }
                else if (ind->proto_id == bt_hid_device->queue_id_interrupt)
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

            link = bt_hid_device_find_link_by_cid(rsp->cid);
            if (link != NULL)
            {
                memcpy(payload.bd_addr, link->bd_addr, 6);

                if (rsp->proto_id == bt_hid_device->queue_id_control)
                {
                    bt_hid_device_free_link(link);

                    payload.msg_buf = &rsp->cause;
                    bt_mgr_dispatch(BT_MSG_HID_DEVICE_DISCONN_CMPL, &payload);
                }
                else if (rsp->proto_id == bt_hid_device->queue_id_interrupt)
                {
                    link->interrupt_chann.state = BT_HID_STATE_DISCONNECTED;
                    link->interrupt_chann.cid = 0;
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

    default:
        break;
    }
}

bool bt_hid_device_get_report_rsp(uint8_t                      bd_addr[6],
                                  T_BT_HID_DEVICE_REPORT_TYPE  report_type,
                                  uint8_t                     *buf,
                                  uint16_t                     len)
{
    return bt_hid_device_control_msg_send(bd_addr,
                                          BT_HID_MSG_TYPE_DATA,
                                          report_type,
                                          buf,
                                          len);
}

bool bt_hid_device_control_req(uint8_t                            bd_addr[6],
                               T_BT_HID_DEVICE_CONTROL_OPERATION  operation)
{
    return bt_hid_device_control_msg_send(bd_addr,
                                          BT_HID_MSG_TYPE_HID_CONTROL,
                                          operation,
                                          NULL,
                                          0);
}

bool bt_hid_device_interrupt_data_send(uint8_t                      bd_addr[6],
                                       T_BT_HID_DEVICE_REPORT_TYPE  report_type,
                                       uint8_t                     *buf,
                                       uint16_t                     len)
{
    return bt_hid_device_interrupt_msg_send(bd_addr,
                                            BT_HID_MSG_TYPE_DATA,
                                            report_type,
                                            buf,
                                            len);
}

bool bt_hid_device_connect_req(uint8_t bd_addr[6])
{
    T_BT_HID_DEVICE_LINK *link;

    PROFILE_PRINT_TRACE1("bt_hid_device_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    link = bt_hid_device_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_hid_device_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        link->control_chann.state = BT_HID_STATE_CONNECTING;
        link->int_flag = INITIATE_CONN_MASK;

        bt_sniff_mode_exit(bd_addr, false);
        mpa_send_l2c_conn_req(PSM_HID_CONTROL, UUID_HIDP, bt_hid_device->queue_id_control,
                              BT_HID_CONTROL_MTU_SIZE, bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);
        return true;
    }

    return false;
}

bool bt_hid_device_connect_cfm(uint8_t bd_addr[6],
                               bool    accept)
{
    T_BT_HID_DEVICE_LINK *link;

    link = bt_hid_device_find_link_by_addr(bd_addr);
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
            bt_hid_device_free_link(link);
        }

        return true;
    }

    return false;
}

bool bt_hid_device_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_HID_DEVICE_LINK *link;

    PROFILE_PRINT_TRACE1("bt_hid_device_disconnect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    link = bt_hid_device_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);

        if (link->interrupt_chann.state == BT_HID_STATE_CONNECTED)
        {
            link->int_flag |= INITIATE_DISCONN_MASK;
            link->interrupt_chann.state = BT_HID_STATE_DISCONNECTING;
            mpa_send_l2c_disconn_req(link->interrupt_chann.cid);
        }
        else if (link->control_chann.state == BT_HID_STATE_CONNECTED)
        {
            link->control_chann.state = BT_HID_STATE_DISCONNECTING;
            mpa_send_l2c_disconn_req(link->control_chann.cid);
        }

        return true;
    }

    return false;
}

bool bt_hid_device_descriptor_set(const uint8_t *descriptor,
                                  uint16_t       len)
{
    if (bt_hid_device != NULL)
    {
        bt_hid_device->descriptor       = descriptor;
        bt_hid_device->descriptor_len   = len;
        return true;
    }

    return false;
}

bool bt_hid_device_init(bool boot_proto_mode)
{
    int32_t ret = 0;

    bt_hid_device = calloc(1, sizeof(T_BT_HID_DEVICE));
    if (bt_hid_device == NULL)
    {
        ret = 1;
        goto fail_alloc_hid;
    }

    bt_hid_device->boot_proto_mode  = boot_proto_mode;
    bt_hid_device->descriptor       = NULL;
    bt_hid_device->descriptor_len   = 0;

    if (mpa_reg_l2c_proto(PSM_HID_CONTROL, bt_hid_device_cback,
                          &bt_hid_device->queue_id_control) == false)
    {
        ret = 2;
        goto fail_reg_l2c_hid_control;
    }

    if (mpa_reg_l2c_proto(PSM_HID_INTERRUPT, bt_hid_device_cback,
                          &bt_hid_device->queue_id_interrupt) == false)
    {
        ret = 3;
        goto fail_reg_l2c_hid_interrupt;
    }

    return true;

fail_reg_l2c_hid_interrupt:
fail_reg_l2c_hid_control:
    free(bt_hid_device);
    bt_hid_device = NULL;
fail_alloc_hid:
    PROFILE_PRINT_ERROR1("bt_hid_device_init: failed %d", -ret);
    return false;
}

void bt_hid_device_deinit(void)
{
    if (bt_hid_device != NULL)
    {
        T_BT_HID_DEVICE_LINK *link;

        link = os_queue_out(&bt_hid_device->list);
        while (link != NULL)
        {
            free(link);
            link = os_queue_out(&bt_hid_device->list);
        }

        free(bt_hid_device);
        bt_hid_device = NULL;
    }
}

bool bt_hid_device_roleswap_info_get(uint8_t                     bd_addr[6],
                                     T_ROLESWAP_HID_DEVICE_INFO *info)
{
    T_BT_HID_DEVICE_LINK *link;

    link = bt_hid_device_find_link_by_addr(bd_addr);
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

bool bt_hid_device_roleswap_info_set(uint8_t                     bd_addr[6],
                                     T_ROLESWAP_HID_DEVICE_INFO *info)
{
    T_BT_HID_DEVICE_LINK *link;

    link = bt_hid_device_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_hid_device_alloc_link(bd_addr);
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

bool bt_hid_device_roleswap_info_del(uint8_t bd_addr[6])
{
    T_BT_HID_DEVICE_LINK *link;

    link = bt_hid_device_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_hid_device_free_link(link);
        return true;
    }

    return false;
}
#else
#include <stdint.h>
#include <stdbool.h>

#include "bt_hid_device.h"
#include "bt_hid_int.h"

bool bt_hid_device_get_report_rsp(uint8_t                      bd_addr[6],
                                  T_BT_HID_DEVICE_REPORT_TYPE  report_type,
                                  uint8_t                     *buf,
                                  uint16_t                     len)
{
    return false;
}

bool bt_hid_device_control_req(uint8_t                            bd_addr[6],
                               T_BT_HID_DEVICE_CONTROL_OPERATION  operation)
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

bool bt_hid_device_init(bool boot_proto_mode)
{
    return false;
}

void bt_hid_device_deinit(void)
{

}

bool bt_hid_device_roleswap_info_get(uint8_t                     bd_addr[6],
                                     T_ROLESWAP_HID_DEVICE_INFO *info)
{
    return false;
}

bool bt_hid_device_roleswap_info_set(uint8_t                     bd_addr[6],
                                     T_ROLESWAP_HID_DEVICE_INFO *info)
{
    return false;
}

bool bt_hid_device_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}
#endif
