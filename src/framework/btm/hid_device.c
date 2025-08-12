/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HID_DEVICE_SUPPORT == 1)

#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "hid_device.h"
#include "hid_parser.h"

typedef struct t_hid_device_link
{
    uint8_t                  bd_addr[6];
    uint8_t                  proto_mode;
    T_HID_CONTROL_CHANN      control_chann;
    T_HID_INTERRUPT_CHANN    interrupt_chann;
} T_HID_DEVICE_LINK;

typedef struct t_hid_device
{
    uint8_t              queue_id_control;
    uint8_t              queue_id_interrupt;
    uint8_t              link_num;
    bool                 boot_proto_mode;
    const uint8_t       *descriptor;
    uint16_t             descriptor_len;
    T_HID_DEVICE_LINK   *link;
    P_HID_DEVICE_CBACK   device_cback;
} T_HID_DEVICE;

static T_HID_DEVICE *hid_device = NULL;

T_HID_DEVICE_LINK *hid_device_alloc_link(uint8_t bd_addr[6])
{
    uint8_t            i;
    T_HID_DEVICE_LINK *p_link = NULL;

    for (i = 0; i < hid_device->link_num; i++)
    {
        if (hid_device->link[i].control_chann.state == HID_STATE_DISCONNECTED)
        {
            p_link = &(hid_device->link[i]);
            memcpy(p_link->bd_addr, bd_addr, 6);
            p_link->control_chann.state = HID_STATE_ALLOCATED;
            p_link->proto_mode = HID_MSG_PARAM_REPORT_PROTOCOL_MODE;
            break;
        }
    }

    return p_link;
}

void hid_device_free_link(T_HID_DEVICE_LINK *p_link)
{
    memset(p_link, 0, sizeof(T_HID_DEVICE_LINK));
}

T_HID_DEVICE_LINK *hid_find_link_by_cid(uint16_t cid)
{
    uint8_t            i;
    T_HID_DEVICE_LINK *p_link;

    for (i = 0; i < hid_device->link_num; i++)
    {
        p_link = &(hid_device->link[i]);
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

T_HID_DEVICE_LINK *hid_device_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t            i;
    T_HID_DEVICE_LINK *p_link = NULL;

    for (i = 0; i < hid_device->link_num; i++)
    {
        if (hid_device->link[i].control_chann.state != HID_STATE_DISCONNECTED &&
            !memcmp(hid_device->link[i].bd_addr, bd_addr, 6))
        {
            p_link = &(hid_device->link[i]);
            break;
        }
    }

    return p_link;
}

bool hid_device_control_connect_req(uint8_t bd_addr[6])
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_device_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = hid_device_alloc_link(bd_addr);
        if (p_link != NULL)
        {
            PROFILE_PRINT_TRACE1("hid_device_control_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            p_link->control_chann.state = HID_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_HID_CONTROL, UUID_HIDP, hid_device->queue_id_control,
                                  HID_CONTROL_MTU_SIZE, bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);

            return true;
        }
    }

    return false;
}

bool hid_device_interrupt_connect_req(uint8_t bd_addr[6])
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_device_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        PROFILE_PRINT_TRACE1("hid_device_interrupt_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

        if (p_link->interrupt_chann.state == HID_STATE_DISCONNECTED)
        {
            p_link->interrupt_chann.state = HID_STATE_CONNECTING;
            mpa_send_l2c_conn_req(PSM_HID_INTERRUPT, UUID_HIDP, hid_device->queue_id_interrupt,
                                  HID_INTERRUPT_MTU_SIZE, bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);

            return true;
        }
    }

    return false;
}

bool hid_device_control_disconnect_req(uint8_t bd_addr[6])
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_device_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        mpa_send_l2c_disconn_req(p_link->control_chann.cid);
        return true;
    }

    return false;
}

bool hid_device_interrupt_disconnect_req(uint8_t bd_addr[6])
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_device_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        mpa_send_l2c_disconn_req(p_link->interrupt_chann.cid);
        return true;
    }

    return false;
}

bool hid_device_control_connect_cfm(uint8_t bd_addr[6],
                                    bool    accept)
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_device_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        uint16_t cid;
        T_MPA_L2C_CONN_CFM_CAUSE rsp;

        cid = p_link->control_chann.cid;
        rsp = MPA_L2C_CONN_ACCEPT;
        if (accept == false)
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;
            hid_device_free_link(p_link);
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

bool hid_device_interrupt_connect_cfm(uint8_t bd_addr[6],
                                      bool    accept)
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_device_find_link_by_addr(bd_addr);
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

bool hid_device_control_msg_send(uint8_t   bd_addr[6],
                                 uint8_t   msg_type,
                                 uint8_t   msg_param,
                                 uint8_t  *buf,
                                 uint16_t  len)
{
    T_HID_DEVICE_LINK *p_link;
    uint8_t           *hid_buf;
    int32_t            ret = 0;

    p_link = hid_device_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (HID_HDR_LENGTH + len > p_link->control_chann.remote_mtu)
    {
        ret = 2;
        goto fail_invalid_len;
    }

    hid_buf = mpa_get_l2c_buf(hid_device->queue_id_control,
                              p_link->control_chann.cid,
                              0,
                              HID_HDR_LENGTH + len,
                              p_link->control_chann.ds_data_offset,
                              false);
    if (hid_buf == NULL)
    {
        ret = 3;
        goto fail_get_l2c_buf;
    }

    *(hid_buf + p_link->control_chann.ds_data_offset) = (msg_type << 4) | (msg_param & 0x0f);
    memcpy(hid_buf + p_link->control_chann.ds_data_offset + HID_HDR_LENGTH, buf, len);

    mpa_send_l2c_data_req(hid_buf,
                          p_link->control_chann.ds_data_offset,
                          p_link->control_chann.cid,
                          HID_HDR_LENGTH + len,
                          false);

    return true;

fail_get_l2c_buf:
fail_invalid_len:
fail_invalid_addr:
    PROFILE_PRINT_ERROR3("hid_device_control_msg_send: failed %d, bd_addr %s, msg_type 0x%02x",
                         -ret, TRACE_BDADDR(bd_addr), msg_type);
    return false;
}

bool hid_device_interrupt_msg_send(uint8_t   bd_addr[6],
                                   uint8_t   msg_type,
                                   uint8_t   msg_param,
                                   uint8_t  *buf,
                                   uint16_t  len)
{
    T_HID_DEVICE_LINK *p_link;
    uint8_t           *hid_buf;
    int32_t            ret = 0;

    p_link = hid_device_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (p_link->interrupt_chann.state != HID_STATE_CONNECTED)
    {
        ret = 2;
        goto fail_invalid_state;
    }

    if ((HID_HDR_LENGTH + len) > p_link->interrupt_chann.remote_mtu)
    {
        ret = 3;
        goto fail_invalid_len;
    }

    hid_buf = mpa_get_l2c_buf(hid_device->queue_id_interrupt,
                              p_link->interrupt_chann.cid,
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
fail_invalid_state:
fail_invalid_addr:
    PROFILE_PRINT_ERROR3("hid_device_interrupt_msg_send: failed %d, bd_addr %s, msg_type 0x%02x",
                         -ret, TRACE_BDADDR(bd_addr), msg_type);
    return false;
}

bool hid_device_report_size_check(uint16_t          cid,
                                  uint16_t          report_id,
                                  T_HID_REPORT_TYPE report_type,
                                  uint16_t          report_size)
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_find_link_by_cid(cid);
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
                                              hid_device->descriptor_len,
                                              hid_device->descriptor);
            if ((size == 0) || (size != report_size))
            {
                return false;
            }
        }
        return true;
    }

    return false;
}

T_HID_REPORT_ID_STATUS hid_device_report_id_status_get(uint16_t cid,
                                                       uint16_t report_id)
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_find_link_by_cid(cid);
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
            return hid_get_report_id_status(report_id, hid_device->descriptor_len, hid_device->descriptor);
        }
    }

    return HID_REPORT_ID_UNDECLARED;
}

static int hid_device_report_size_get(uint16_t           cid,
                                      uint16_t           report_id,
                                      T_HID_REPORT_TYPE  report_type,
                                      uint16_t           descriptor_len,
                                      const uint8_t     *descriptor)
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_find_link_by_cid(cid);
    if (p_link != NULL)
    {
        if (p_link->proto_mode == HID_MSG_PARAM_BOOT_PROTOCOL_MODE)
        {
            switch (report_id)
            {
            case HID_BOOT_MODE_KEYBOARD_ID:
                return 8;
            case HID_BOOT_MODE_MOUSE_ID:
                return 3;
            default:
                return 0;
            }
        }
        else
        {
            return hid_report_size_get_by_id(report_id, report_type, descriptor_len, descriptor);
        }
    }

    return 0;
}

uint8_t hid_set_report_status_get(uint16_t           cid,
                                  T_HID_REPORT_TYPE  report_type,
                                  uint16_t           report_size,
                                  uint8_t           *report)
{
    uint8_t  pos = 0;
    uint16_t report_id = 0;

    if (hid_report_id_declared(hid_device->descriptor_len, hid_device->descriptor))
    {
        report_id = report[pos++];
        if (hid_device_report_id_status_get(cid, report_id) == HID_REPORT_ID_INVALID)
        {
            return HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID;
        }
    }

    if (!hid_device_report_size_check(cid, report_id, report_type, report_size - pos))
    {
        return HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
    }
    return HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;
}

void hid_control_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *p_l2c_data_ind)
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_find_link_by_cid(p_l2c_data_ind->cid);
    if (p_link != NULL)
    {
        uint8_t pos = 0;
        uint16_t report_size;
        uint16_t proto_mode;
        uint8_t *p_data;
        uint16_t length;
        uint8_t msg_type;

        p_data = p_l2c_data_ind->data + p_l2c_data_ind->gap;
        length = p_l2c_data_ind->length;
        msg_type = p_data[0] >> 4;
        p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;

        T_HID_DEVICE_GET_REPORT_IND get_report_ind;
        T_HID_DEVICE_SET_REPORT_IND set_report_ind;
        T_HID_DEVICE_GET_PROTOCOL_IND get_protocol_ind;
        T_HID_DEVICE_SET_PROTOCOL_IND set_protocol_ind;
        T_HID_DEVICE_SET_IDLE_IND set_idle_ind;
        T_HID_DEVICE_DATA_IND data_ind;

        switch (msg_type)
        {
        case HID_MSG_TYPE_GET_REPORT:
            get_report_ind.report_type = (T_HID_REPORT_TYPE)p_data[pos++] & 0x03;
            get_report_ind.report_size = 0;
            get_report_ind.report_id = 0;
            switch (p_link->proto_mode)
            {
            case HID_MSG_PARAM_BOOT_PROTOCOL_MODE:
                if (length < 2)
                {
                    p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
                    break;
                }
                get_report_ind.report_id = p_data[pos++];
                break;
            case HID_MSG_PARAM_REPORT_PROTOCOL_MODE:
                if (!hid_report_id_declared(hid_device->descriptor_len, hid_device->descriptor))
                {
                    if (p_data[0] & 0x08)
                    {
                        if (length > 3)
                        {
                            p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID;
                        }
                    }
                    else
                    {
                        if (length > 1)
                        {
                            p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID;
                        }
                    }
                    break;
                }
                if (length < 2)
                {
                    p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
                    break;
                }
                get_report_ind.report_id = p_data[pos++];
                break;
            }
            if (p_link->control_chann.report_status != HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
            {
                break;
            }
            if (hid_device_report_id_status_get(p_l2c_data_ind->cid, get_report_ind.report_id) ==
                HID_REPORT_ID_INVALID)
            {
                p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID;
            }
            if (p_link->control_chann.report_status != HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
            {
                break;
            }
            if (!hid_report_type_is_valid(get_report_ind.report_type,
                                          hid_device->descriptor_len,
                                          hid_device->descriptor))
            {
                p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
                break;
            }
            report_size = hid_device_report_size_get(p_l2c_data_ind->cid,
                                                     get_report_ind.report_id,
                                                     (T_HID_REPORT_TYPE)get_report_ind.report_type,
                                                     hid_device->descriptor_len, hid_device->descriptor);

            if ((p_data[0] & 0x08) && (length >= (pos + 1)))
            {
                uint16_t buffer_size;
                uint16_t tmp;

                buffer_size = (uint16_t)(((uint16_t) p_data[pos]) | (((uint16_t)p_data[pos + 1]) << 8));
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
                if (HID_CONTROL_MTU_SIZE < tmp)
                {
                    tmp = HID_CONTROL_MTU_SIZE;
                }
                get_report_ind.report_size = tmp;
            }
            else
            {
                if (report_size > HID_CONTROL_MTU_SIZE)
                {
                    get_report_ind.report_size = HID_CONTROL_MTU_SIZE;
                }
                else
                {
                    get_report_ind.report_size = report_size;
                }
            }
            hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_GET_REPORT_IND, &get_report_ind);
            break;

        case HID_MSG_TYPE_SET_REPORT:
            set_report_ind.report_id = 0;
            set_report_ind.report_size = 0;
            set_report_ind.report_type = (T_HID_REPORT_TYPE)p_data[pos++] & 0x03;

            if (length < 1)
            {
                p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
                break;
            }

            switch (p_link->proto_mode)
            {
            case HID_MSG_PARAM_BOOT_PROTOCOL_MODE:
                if (length < 3)
                {
                    p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
                    break;
                }

                p_link->control_chann.report_status = hid_set_report_status_get(p_l2c_data_ind->cid,
                                                                                (T_HID_REPORT_TYPE)set_report_ind.report_type,
                                                                                length - 1, &p_data[1]);

                if (p_link->control_chann.report_status != HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
                {
                    break;
                }
                set_report_ind.report_id = p_data[pos++];
                set_report_ind.p_data = &p_data[pos];
                set_report_ind.report_size = length - pos;
                hid_device_control_msg_send(p_link->bd_addr,
                                            HID_MSG_TYPE_HANDSHAKE,
                                            p_link->control_chann.report_status,
                                            NULL,
                                            0);
                hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_SET_REPORT_IND, &set_report_ind);
                break;

            case HID_MSG_PARAM_REPORT_PROTOCOL_MODE:
                p_link->control_chann.report_status = hid_set_report_status_get(p_l2c_data_ind->cid,
                                                                                (T_HID_REPORT_TYPE)set_report_ind.report_type,
                                                                                length - 1, &p_data[1]);

                if (p_link->control_chann.report_status != HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
                {
                    break;
                }
                if (length >= 2)
                {
                    if (hid_report_id_declared(hid_device->descriptor_len, hid_device->descriptor))
                    {
                        set_report_ind.report_id = p_data[pos++];
                        set_report_ind.p_data = &p_data[pos];
                        set_report_ind.report_size = length - pos;
                    }
                    else
                    {
                        set_report_ind.p_data = &p_data[pos];
                        set_report_ind.report_size = length - pos;
                    }

                    hid_device_control_msg_send(p_link->bd_addr,
                                                HID_MSG_TYPE_HANDSHAKE,
                                                p_link->control_chann.report_status,
                                                NULL,
                                                0);
                    hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_SET_REPORT_IND, &set_report_ind);
                }
                else
                {
                    uint8_t payload[] = {0};
                    set_report_ind.p_data = payload;
                    hid_device_control_msg_send(p_link->bd_addr,
                                                HID_MSG_TYPE_HANDSHAKE,
                                                p_link->control_chann.report_status,
                                                NULL,
                                                0);
                    hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_SET_REPORT_IND, &set_report_ind);
                }

                break;
            default:
                break;
            }
            break;

        case HID_MSG_TYPE_GET_PROTOCOL:
            if (length != 1)
            {
                p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
                break;
            }
            p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;
            get_protocol_ind.proto_mode = p_link->proto_mode;
            hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_GET_PROTOCOL_IND, &get_protocol_ind);
            break;

        case HID_MSG_TYPE_SET_PROTOCOL:
            if (length != 1)
            {
                p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
                break;
            }
            proto_mode = p_data[0] & 0x01;
            if (proto_mode == HID_MSG_PARAM_BOOT_PROTOCOL_MODE && hid_device->boot_proto_mode == false)
            {
                p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER;
                break;
            }
            p_link->proto_mode = proto_mode;
            p_link->control_chann.report_status = HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL;
            set_protocol_ind.proto_mode = p_link->proto_mode;
            hid_device_control_msg_send(p_link->bd_addr,
                                        HID_MSG_TYPE_HANDSHAKE,
                                        p_link->control_chann.report_status,
                                        NULL,
                                        0);
            hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_SET_PROTOCOL_IND, &set_protocol_ind);
            break;

        case HID_MSG_TYPE_SET_IDLE:
            set_idle_ind.report_status = p_link->control_chann.report_status;
            hid_device_control_msg_send(p_link->bd_addr,
                                        HID_MSG_TYPE_HANDSHAKE,
                                        p_link->control_chann.report_status,
                                        NULL,
                                        0);
            hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_SET_IDLE_IND, &set_idle_ind);
            break;

        case HID_MSG_TYPE_HID_CONTROL:
            break;

        case HID_MSG_TYPE_DATA:
            if (length < 2)
            {
                break;
            }
            data_ind.report_type = p_data[pos++] & 0x03;
            data_ind.report_id = 0;
            if (hid_report_id_declared(hid_device->descriptor_len, hid_device->descriptor))
            {
                data_ind.report_id = p_data[pos++];
            }
            if (hid_device_report_id_status_get(p_l2c_data_ind->cid, data_ind.report_id) ==
                HID_REPORT_ID_INVALID)
            {
                break;
            }
            if (!hid_device_report_size_check(p_l2c_data_ind->cid, data_ind.report_id,
                                              (T_HID_REPORT_TYPE)data_ind.report_type,
                                              length - pos))
            {
                break;
            }
            data_ind.report_size = length - pos;
            data_ind.p_data = &p_data[pos];
            hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_CONTROL_DATA_IND, &data_ind);
            break;
        default:
            hid_device_control_msg_send(p_link->bd_addr,
                                        HID_MSG_TYPE_HANDSHAKE,
                                        HID_MSG_PARAM_HANDSHAKE_ERR_UNSUPPORTED_REQUEST,
                                        NULL,
                                        0);
            break;
        }
        if (p_link->control_chann.report_status != HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL)
        {
            hid_device_control_msg_send(p_link->bd_addr,
                                        HID_MSG_TYPE_HANDSHAKE,
                                        p_link->control_chann.report_status,
                                        NULL,
                                        0);
        }
    }
}

void hid_interrupt_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *p_l2c_data_ind)
{
    T_HID_DEVICE_LINK *p_link;
    uint8_t     pos = 0;

    p_link = hid_find_link_by_cid(p_l2c_data_ind->cid);
    if (p_link != NULL)
    {
        uint8_t *p_data;
        uint16_t length;
        T_HID_DEVICE_DATA_IND data_ind;

        p_data = p_l2c_data_ind->data + p_l2c_data_ind->gap;
        length = p_l2c_data_ind->length;
        if (length < 2)
        {
            return;
        }
        data_ind.report_type = p_data[pos++] & 0x03;
        data_ind.report_id = 0;
        if (hid_report_id_declared(hid_device->descriptor_len, hid_device->descriptor))
        {
            data_ind.report_id = p_data[pos++];
        }
        if (hid_device_report_id_status_get(p_l2c_data_ind->cid, data_ind.report_id) ==
            HID_REPORT_ID_INVALID)
        {
            return;
        }
        if (!hid_device_report_size_check(p_l2c_data_ind->cid, data_ind.report_id,
                                          (T_HID_REPORT_TYPE)data_ind.report_type,
                                          length - pos))
        {
            return;
        }
        data_ind.report_size = length - pos;
        data_ind.p_data = &p_data[pos];
        hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_INTERRUPT_DATA_IND, &data_ind);
    }
}

void hid_device_l2c_callback(void        *p_buf,
                             T_PROTO_MSG  l2c_msg)
{
    switch (l2c_msg)
    {
    case L2C_CONN_IND:
        {
            T_MPA_L2C_CONN_IND *ind = (T_MPA_L2C_CONN_IND *)p_buf;
            T_HID_DEVICE_LINK *p_link = hid_device_find_link_by_addr(ind->bd_addr);

            if (ind->proto_id == hid_device->queue_id_control)
            {
                if (p_link == NULL)
                {
                    p_link = hid_device_alloc_link(ind->bd_addr);
                    if (p_link != NULL)
                    {
                        p_link->control_chann.cid = ind->cid;
                        p_link->control_chann.state = HID_STATE_CONNECTING;
                        hid_device->device_cback(ind->bd_addr, HID_DEVICE_MSG_CONTROL_CONN_IND, p_buf);
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
            else if (ind->proto_id == hid_device->queue_id_interrupt)
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
                        hid_device->device_cback(ind->bd_addr, HID_DEVICE_MSG_INTERRUPT_CONN_IND, p_buf);
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
            T_HID_DEVICE_LINK *p_link = hid_device_find_link_by_addr(ind->bd_addr);

            if (ind->proto_id == hid_device->queue_id_control)
            {
                if (ind->cause == 0)
                {
                    if (p_link != NULL)
                    {
                        p_link->control_chann.cid = ind->cid;
                        hid_device->device_cback(ind->bd_addr, HID_DEVICE_MSG_CONTROL_CONN_RSP, p_buf);
                    }
                    else
                    {
                        hid_device_control_disconnect_req(ind->bd_addr);
                    }
                }
                else
                {
                    if (p_link != NULL)
                    {
                        hid_device_free_link(p_link);
                    }

                    hid_device->device_cback(ind->bd_addr, HID_DEVICE_MSG_CONTROL_CONN_FAIL, &ind->cause);
                }
            }
            else if (ind->proto_id == hid_device->queue_id_interrupt)
            {
                if (ind->cause == 0)
                {
                    if (p_link != NULL)
                    {
                        p_link->interrupt_chann.cid = ind->cid;
                        hid_device->device_cback(ind->bd_addr, HID_DEVICE_MSG_INTERRUPT_CONN_RSP, p_buf);
                    }
                    else
                    {
                        hid_device_interrupt_disconnect_req(ind->bd_addr);
                    }
                }
                else
                {
                    if (p_link != NULL)
                    {
                        memset(&p_link->interrupt_chann, 0, sizeof(T_HID_INTERRUPT_CHANN));
                    }

                    hid_device->device_cback(ind->bd_addr, HID_DEVICE_MSG_INTERRUPT_CONN_FAIL, &ind->cause);
                }
            }
        }
        break;

    case L2C_CONN_CMPL:
        {
            T_MPA_L2C_CONN_CMPL_INFO *p_info = (T_MPA_L2C_CONN_CMPL_INFO *)p_buf;
            T_HID_DEVICE_LINK *p_link;

            p_link = hid_find_link_by_cid(p_info->cid);
            if (p_link != NULL)
            {
                T_HID_DEVICE_CONN_CMPL_INFO info;

                memcpy(info.bd_addr, p_info->bd_addr, 6);
                info.cause = p_info->cause;

                if (p_info->proto_id == hid_device->queue_id_control)
                {
                    if (p_info->cause)
                    {
                        hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_CONTROL_CONN_FAIL, &p_info->cause);
                        hid_device_free_link(p_link);
                    }
                    else
                    {
                        p_link->control_chann.state = HID_STATE_CONNECTED;
                        p_link->control_chann.remote_mtu = p_info->remote_mtu;
                        p_link->control_chann.ds_data_offset = p_info->ds_data_offset;
                        hid_device->device_cback(p_info->bd_addr, HID_DEVICE_MSG_CONTROL_CONN_CMPL, &info);
                    }
                }
                else if (p_info->proto_id == hid_device->queue_id_interrupt)
                {
                    if (p_info->cause)
                    {
                        memset(&p_link->interrupt_chann, 0, sizeof(T_HID_INTERRUPT_CHANN));
                        hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_INTERRUPT_CONN_FAIL, &p_info->cause);
                    }
                    else
                    {
                        p_link->interrupt_chann.state = HID_STATE_CONNECTED;
                        p_link->interrupt_chann.remote_mtu = p_info->remote_mtu;
                        p_link->interrupt_chann.ds_data_offset = p_info->ds_data_offset;
                        hid_device->device_cback(p_info->bd_addr, HID_DEVICE_MSG_INTERRUPT_CONN_CMPL, &info);
                    }
                }
            }
        }
        break;

    case L2C_DATA_IND:
        {
            T_MPA_L2C_DATA_IND *ind = (T_MPA_L2C_DATA_IND *)p_buf;
            if (ind->proto_id == hid_device->queue_id_control)
            {
                hid_control_handle_l2c_data_ind(ind);
            }
            else if (ind->proto_id == hid_device->queue_id_interrupt)
            {
                hid_interrupt_handle_l2c_data_ind(ind);
            }
        }
        break;

    case L2C_DISCONN_IND:
        {
            T_MPA_L2C_DISCONN_IND *ind = (T_MPA_L2C_DISCONN_IND *)p_buf;
            T_HID_DEVICE_LINK *p_link = hid_find_link_by_cid(ind->cid);

            if (ind->proto_id == hid_device->queue_id_control)
            {
                if (p_link != NULL)
                {
                    hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_CONTROL_DISCONNECTED, &ind->cause);
                    hid_device_free_link(p_link);
                }
                mpa_send_l2c_disconn_cfm(ind->cid);
            }
            else if (ind->proto_id == hid_device->queue_id_interrupt)
            {
                if (p_link != NULL)
                {
                    hid_device->device_cback(p_link->bd_addr, HID_DEVICE_MSG_INTERRUPT_DISCONNECTED, &ind->cause);
                    memset(&p_link->interrupt_chann, 0, sizeof(T_HID_INTERRUPT_CHANN));
                }
                mpa_send_l2c_disconn_cfm(ind->cid);
            }
        }
        break;

    case L2C_DISCONN_RSP:
        {
            T_MPA_L2C_DISCONN_RSP *rsp = (T_MPA_L2C_DISCONN_RSP *)p_buf;
            T_HID_DEVICE_LINK *p_link;

            p_link = hid_find_link_by_cid(rsp->cid);
            if (p_link != NULL)
            {
                T_HID_DEVICE_DISCONN_RSP_INFO info;

                memcpy(info.bd_addr, p_link->bd_addr, 6);
                info.cause = rsp->cause;

                if (rsp->proto_id == hid_device->queue_id_control)
                {
                    hid_device_free_link(p_link);
                    hid_device->device_cback(info.bd_addr, HID_DEVICE_MSG_CONTROL_DISCONNECTED, &info);
                }
                else if (rsp->proto_id == hid_device->queue_id_interrupt)
                {
                    p_link->interrupt_chann.state = HID_STATE_DISCONNECTED;
                    p_link->interrupt_chann.cid = 0;
                    hid_device->device_cback(info.bd_addr, HID_DEVICE_MSG_INTERRUPT_DISCONNECTED, &info);
                }
            }
        }
        break;

    default:
        break;
    }
}

bool hid_device_init(uint8_t            link_num,
                     bool               boot_proto_mode,
                     P_HID_DEVICE_CBACK cback)
{
    int32_t ret = 0;

    hid_device = os_mem_alloc2(sizeof(T_HID_DEVICE));
    if (hid_device == NULL)
    {
        ret = 1;
        goto fail_alloc_hid;
    }

    hid_device->link_num         = link_num;
    hid_device->boot_proto_mode  = boot_proto_mode;
    hid_device->descriptor       = NULL;
    hid_device->descriptor_len   = 0;
    hid_device->device_cback     = cback;

    hid_device->link = os_mem_zalloc2(hid_device->link_num * sizeof(T_HID_DEVICE_LINK));
    if (hid_device->link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (mpa_reg_l2c_proto(PSM_HID_CONTROL, hid_device_l2c_callback,
                          &hid_device->queue_id_control) == false)
    {
        ret = 3;
        goto fail_reg_l2c_hid_control;
    }
    if (mpa_reg_l2c_proto(PSM_HID_INTERRUPT, hid_device_l2c_callback,
                          &hid_device->queue_id_interrupt) == false)
    {
        ret = 4;
        goto fail_reg_l2c_hid_interrupt;
    }

    return true;

fail_reg_l2c_hid_interrupt:
fail_reg_l2c_hid_control:
    os_mem_free(hid_device->link);
    hid_device->link = NULL;
fail_alloc_link:
    os_mem_free(hid_device);
    hid_device = NULL;
fail_alloc_hid:
    PROFILE_PRINT_ERROR1("hid_device_init: failed %d", -ret);
    return false;
}

bool hid_device_descriptor_set(const uint8_t *descriptor,
                               uint16_t       len)
{
    if (hid_device != NULL)
    {
        hid_device->descriptor       = descriptor;
        hid_device->descriptor_len   = len;
        return true;
    }

    return false;
}

bool hid_device_get_roleswap_info(uint8_t                     bd_addr[6],
                                  T_ROLESWAP_HID_DEVICE_INFO *p_info)
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_device_find_link_by_addr(bd_addr);
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

bool hid_device_set_roleswap_info(uint8_t                     bd_addr[6],
                                  T_ROLESWAP_HID_DEVICE_INFO *p_info)
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_device_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = hid_device_alloc_link(bd_addr);
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

bool hid_device_del_roleswap_info(uint8_t bd_addr[6])
{
    T_HID_DEVICE_LINK *p_link;

    p_link = hid_device_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        hid_device_free_link(p_link);
        return true;
    }

    return false;
}
#else
#include <stdint.h>
#include <stdbool.h>

#include "hid_device.h"

bool hid_device_control_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool hid_device_interrupt_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool hid_device_control_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool hid_device_interrupt_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool hid_device_control_connect_cfm(uint8_t bd_addr[6],
                                    bool    accept)
{
    return false;
}

bool hid_device_interrupt_connect_cfm(uint8_t bd_addr[6],
                                      bool    accept)
{
    return false;
}

bool hid_device_control_msg_send(uint8_t   bd_addr[6],
                                 uint8_t   msg_type,
                                 uint8_t   msg_param,
                                 uint8_t  *buf,
                                 uint16_t  len)
{
    return false;
}

bool hid_device_interrupt_msg_send(uint8_t   bd_addr[6],
                                   uint8_t   msg_type,
                                   uint8_t   msg_param,
                                   uint8_t  *buf,
                                   uint16_t  len)
{
    return false;
}

bool hid_device_init(uint8_t            link_num,
                     bool               boot_proto_mode,
                     P_HID_DEVICE_CBACK cback)
{
    return false;
}

bool hid_device_descriptor_set(const uint8_t *descriptor,
                               uint16_t       len)
{
    return false;
}

bool hid_device_get_roleswap_info(uint8_t                     bd_addr[6],
                                  T_ROLESWAP_HID_DEVICE_INFO *p_info)
{
    return false;
}

bool hid_device_set_roleswap_info(uint8_t                     bd_addr[6],
                                  T_ROLESWAP_HID_DEVICE_INFO *p_info)
{
    return false;
}

bool hid_device_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}
#endif
