/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_roleswap.h"
#include "bt_roleswap_int.h"
#include "bt_hfp_int.h"
#include "bt_mac.h"


#if (CONFIG_REALTEK_BTM_ROLESWAP_SUPPORT == 1)
const T_BT_ROLESWAP_PROTO *bt_roleswap_proto;

T_ROLESWAP_INFO *bt_roleswap_info_base_find(uint8_t bd_addr[6])
{
    T_ROLESWAP_INFO *roleswap_info;

    roleswap_info = os_queue_peek(&btm_db.roleswap_info_list, 0);
    while (roleswap_info != NULL)
    {
        if (!memcmp(roleswap_info->bd_addr, bd_addr, 6))
        {
            return roleswap_info;
        }

        roleswap_info = roleswap_info->next;
    }

    return NULL;
}

T_ROLESWAP_INFO *bt_roleswap_info_base_alloc(uint8_t bd_addr[6])
{
    T_ROLESWAP_INFO *info;

    info = calloc(1, sizeof(T_ROLESWAP_INFO));
    if (info != NULL)
    {
        memcpy(info->bd_addr, bd_addr, 6);
        os_queue_init(&info->info_list);
        os_queue_in(&btm_db.roleswap_info_list, info);
    }

    return info;
}

bool bt_roleswap_info_base_free(T_ROLESWAP_INFO *info)
{
    if (os_queue_delete(&btm_db.roleswap_info_list, info) == true)
    {
        T_ROLESWAP_DATA *data;

        data = os_queue_out(&info->info_list);
        while (data != NULL)
        {
            free(data);
            data = os_queue_out(&info->info_list);
        }

        free(info);
        return true;
    }

    return false;
}

void bt_roleswap_dump(void)
{
    T_ROLESWAP_INFO *roleswap_info;
    T_ROLESWAP_DATA *data;

    roleswap_info = os_queue_peek(&btm_db.roleswap_info_list, 0);
    while (roleswap_info != NULL)
    {
        BTM_PRINT_TRACE1("bt_roleswap_dump: dump info %s",
                         TRACE_BDADDR(roleswap_info->bd_addr));

        data = (T_ROLESWAP_DATA *)roleswap_info->info_list.p_first;
        while (data)
        {
            BTM_PRINT_TRACE3("bt_roleswap_dump: type %d, length %d, data %b",
                             data->type, data->length, TRACE_BINARY(data->length, &data->u));
            data = data->next;
        }

        roleswap_info = roleswap_info->next;
    }
}

bool bt_roleswap_sco_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_SCO_INFO *info)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        info->air_mode = link->sco_air_mode;
        info->pkt_len  = link->sco_packet_length;

        return true;
    }

    return false;
}

bool bt_roleswap_acl_info_set(T_ROLESWAP_ACL_INFO *info)
{
    T_BT_LINK *link;

    link = bt_link_find(info->bd_addr);
    if (link == NULL)
    {
        link = bt_link_alloc(info->bd_addr);
        if (link != NULL)
        {
            bt_sniff_mode_config(info->bd_addr,
                                 BT_SNIFF_INTERVAL_BASE + link->index * BT_SNIFF_INTERVAL_OFFSET,
                                 BT_SNIFF_ATTEMPT,
                                 BT_SNIFF_TIMEOUT,
                                 BT_PM_TIMEOUT);
        }
    }

    if (link != NULL)
    {
        link->acl_handle = info->handle;
        link->acl_link_state = BT_LINK_STATE_CONNECTED;

        if (info->role == 0)  /* master */
        {
            link->acl_link_role_master = true;
        }
        else
        {
            link->acl_link_role_master = false;
        }

        if (info->mode == 2)  /* sniff mode */
        {
            link->pm_state = BT_LINK_PM_STATE_SNIFF;
        }
        else
        {
            link->pm_state = BT_LINK_PM_STATE_ACTIVE;
        }

        link->acl_link_authenticated = info->authen_state;

        if (info->encrypt_state == 1) /* encrypt on */
        {
            link->acl_link_encrypted = true;
        }
        else
        {
            link->acl_link_encrypted = false;
        }

        link->acl_link_policy = info->link_policy;
        if ((link->acl_link_policy & GAP_LINK_POLICY_SNIFF_MODE) == 0)
        {
            link->pm_enable = false;
        }
        else
        {
            link->pm_enable = true;
        }

        return true;
    }

    return false;
}

bool bt_roleswap_sco_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_SCO_INFO *info)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        link->sco_buf = calloc(60, sizeof(uint8_t));
        if (link->sco_buf != NULL)
        {
            link->sco_handle = info->handle;
            link->sco_air_mode = info->air_mode;
            link->sco_packet_length = info->pkt_len;
            link->sco_state = BT_LINK_SCO_STATE_CONNECTED;
            return true;
        }
    }

    return false;
}

bool bt_roleswap_rfc_ctrl_info_set(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_RFC_CTRL_INFO *info)
{
    return rfc_ctrl_roleswap_info_set(bd_addr, info);
}

bool bt_roleswap_rfc_data_info_set(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_RFC_DATA_INFO *info)
{
    uint8_t service_id;
    bool    ret = false;

    switch (info->uuid)
    {
    case UUID_PBAP:
        ret = bt_pbap_service_id_get(&service_id);
        break;

    case UUID_SERIAL_PORT:
        {
            T_ROLESWAP_DATA *data = bt_find_roleswap_spp_by_dlci(bd_addr, info->dlci);
            if (data != NULL)
            {
                ret = bt_spp_service_id_get(data->u.spp.local_server_chann, &service_id);
            }
        }
        break;

    case UUID_HANDSFREE:
    case UUID_HEADSET:
        ret = bt_hfp_service_id_get(info->uuid, &service_id);
        break;

    case UUID_IAP:
        ret = bt_iap_service_id_get(&service_id);
        break;

    case UUID_BTRFC:
        {
            T_ROLESWAP_DATA *data = bt_find_roleswap_bt_rfc_by_dlci(bd_addr, info->dlci);
            if (data != NULL)
            {
                ret = bt_rfc_service_id_get(data->u.bt_rfc.local_server_chann, &service_id);
            }
        }
        break;

    default:
        break;
    }

    if (ret == true)
    {
        return rfc_data_roleswap_info_set(bd_addr, service_id, info);
    }

    return false;
}

bool bt_roleswap_info_send(uint8_t   module,
                           uint8_t   submodule,
                           uint8_t  *info,
                           uint16_t  len)
{
    uint8_t *buf;
    uint8_t *p;
    bool     ret = false;

    buf = os_mem_alloc2(ROLESWAP_MSG_HDR_LEN + len);
    if (buf != NULL)
    {
        BTM_PRINT_TRACE3("bt_roleswap_info_send: module 0x%x, submodule 0x%x, data %b",
                         module, submodule, TRACE_BINARY(len, info));

        p = buf;
        LE_UINT8_TO_STREAM(p, module);
        LE_UINT8_TO_STREAM(p, submodule);
        LE_UINT16_TO_STREAM(p, len);
        ARRAY_TO_STREAM(p, info, len);

        ret = remote_async_msg_relay(btm_db.relay_handle,
                                     BT_REMOTE_MSG_ROLESWAP_INFO_XMIT,
                                     buf,
                                     p - buf,
                                     false);
        os_mem_free(buf);
    }

    return ret;
}

T_ROLESWAP_DATA *bt_roleswap_data_find(uint8_t bd_addr[6],
                                       uint8_t type)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == type)
            {
                return data;
            }

            data = data->next;
        }
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_roleswap_rfc_data_find(uint8_t bd_addr[6],
                                           uint8_t dlci)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == ROLESWAP_TYPE_RFC_DATA && data->u.rfc_data.dlci == dlci)
            {
                return data;
            }

            data = data->next;
        }
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_rfc_data_by_cid(uint8_t  bd_addr[6],
                                                  uint16_t cid)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == ROLESWAP_TYPE_RFC_DATA && data->u.rfc_data.l2c_cid == cid)
            {
                return data;
            }

            data = data->next;
        }
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_roleswap_spp_find(uint8_t bd_addr[6],
                                      uint8_t local_server_chann)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == ROLESWAP_TYPE_SPP &&
                data->u.spp.local_server_chann == local_server_chann)
            {
                return data;
            }

            data = data->next;
        }
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_spp_by_dlci(uint8_t bd_addr[6],
                                              uint8_t dlci)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == ROLESWAP_TYPE_SPP && data->u.spp.rfc_dlci == dlci)
            {
                return data;
            }

            data = data->next;
        }
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_roleswap_rfc_ctrl_find(uint8_t  bd_addr[6],
                                           uint16_t cid)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == ROLESWAP_TYPE_RFC_CTRL && data->u.rfc_ctrl.l2c_cid == cid)
            {
                return data;
            }

            data = data->next;
        }
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_roleswap_bt_rfc_find(uint8_t bd_addr[6],
                                         uint8_t local_server_chann)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == ROLESWAP_TYPE_BT_RFC &&
                data->u.bt_rfc.local_server_chann == local_server_chann)
            {
                return data;
            }

            data = data->next;
        }
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_bt_rfc_by_dlci(uint8_t bd_addr[6],
                                                 uint8_t dlci)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == ROLESWAP_TYPE_BT_RFC && data->u.bt_rfc.dlci == dlci)
            {
                return data;
            }

            data = data->next;
        }
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_roleswap_l2c_find(uint8_t  bd_addr[6],
                                      uint16_t cid)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == ROLESWAP_TYPE_L2C && data->u.l2c.local_cid == cid)
            {
                return data;
            }

            data = data->next;
        }
    }

    return NULL;
}

bool bt_roleswap_info_alloc(uint8_t   bd_addr[6],
                            uint8_t   type,
                            uint8_t  *info,
                            uint16_t  len)
{
    T_ROLESWAP_DATA *data = NULL;
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base == NULL)
    {
        BTM_PRINT_ERROR1("bt_roleswap_info_alloc: fail to find roleswap base %s",
                         TRACE_BDADDR(bd_addr));
        return false;
    }

    if (type == ROLESWAP_TYPE_RFC_CTRL)
    {
        T_ROLESWAP_RFC_CTRL_INFO *ctrl = (T_ROLESWAP_RFC_CTRL_INFO *)info;
        data = bt_roleswap_rfc_ctrl_find(bd_addr, ctrl->l2c_cid);
        if (data)
        {
            return false;
        }
    }

    data = os_mem_zalloc2(sizeof(T_ROLESWAP_DATA));
    if (data == NULL)
    {
        return false;
    }

    data->type = type;
    data->length = len;
    memcpy((uint8_t *)&data->u, info, len);

    os_queue_in(&base->info_list, data);

    bt_roleswap_dump();

    return true;
}

void bt_roleswap_rfc_info_get(uint8_t  bd_addr[6],
                              uint8_t  dlci,
                              uint16_t uuid)
{

    T_ROLESWAP_RFC_DATA_INFO data;

    if (rfc_data_roleswap_info_get(bd_addr, dlci, &data) == true)
    {
        data.uuid = uuid;
        memcpy(data.bd_addr, bd_addr, 6);

        /* check if rfcomm control session need to alloc and send */
        if (bt_roleswap_rfc_ctrl_find(bd_addr, data.l2c_cid) == NULL)
        {
            T_ROLESWAP_RFC_CTRL_INFO ctrl;

            if (rfc_ctrl_roleswap_info_get(bd_addr, data.l2c_cid, &ctrl) == true)
            {
                memcpy(ctrl.bd_addr, bd_addr, 6);
                bt_roleswap_info_alloc(bd_addr,
                                       ROLESWAP_TYPE_RFC_CTRL,
                                       (uint8_t *)&ctrl,
                                       sizeof(ctrl));
                bt_roleswap_info_send(ROLESWAP_MODULE_RFC,
                                      ROLESWAP_RFC_CTRL_CONN,
                                      (uint8_t *)&ctrl,
                                      sizeof(ctrl));
                gap_br_get_handover_l2c_info(ctrl.l2c_cid);
            }
        }

        bt_roleswap_info_alloc(bd_addr, ROLESWAP_TYPE_RFC_DATA, (uint8_t *)&data, sizeof(data));
        bt_roleswap_info_send(ROLESWAP_MODULE_RFC,
                              ROLESWAP_RFC_DATA_CONN,
                              (uint8_t *)&data,
                              sizeof(data));
    }
}

bool bt_roleswap_info_free(uint8_t          bd_addr[6],
                           T_ROLESWAP_DATA *data)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        if (os_queue_delete(&base->info_list, data) == true)
        {
            os_mem_free(data);

            return true;
        }
    }

    return false;
}

void bt_roleswap_l2c_info_free(uint8_t  bd_addr[6],
                               uint16_t cid)
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_l2c_find(bd_addr, cid);
    if (data != NULL)
    {
        bt_roleswap_info_free(bd_addr, data);
    }
}

void bt_roleswap_rfc_ctrl_check_free(uint8_t  bd_addr[6],
                                     uint16_t cid)
{
    T_ROLESWAP_INFO *base;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base != NULL)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == ROLESWAP_TYPE_RFC_DATA && data->u.rfc_data.l2c_cid == cid)
            {
                return;
            }

            data = data->next;
        }

        /* no rfcomm data channel on this l2cap channel, free rfc ctrl and l2c */
        data = bt_roleswap_rfc_ctrl_find(bd_addr, cid);
        if (data != NULL)
        {
            bt_roleswap_info_free(bd_addr, data);
            bt_roleswap_l2c_info_free(bd_addr, cid);
        }
    }
}

bool bt_roleswap_l2c_cid_check(uint8_t  bd_addr[6],
                               uint16_t cid)
{
    T_ROLESWAP_INFO *base;
    T_ROLESWAP_DATA *data;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base == NULL)
    {
        return false;
    }

    data = (T_ROLESWAP_DATA *)base->info_list.p_first;

    while (data)
    {
        switch (data->type)
        {
        case ROLESWAP_TYPE_RFC_CTRL:
            if (data->u.rfc_ctrl.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_A2DP:
            if (data->u.a2dp.sig_cid == cid || data->u.a2dp.stream_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_AVRCP:
            if (data->u.avrcp.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_HFP:
            if (data->u.hfp.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_SPP:
            if (data->u.spp.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_PBAP:
            if (data->u.pbap.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_HID_DEVICE:
            if (data->u.hid_device.control_cid == cid || data->u.hid_device.interrupt_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_HID_HOST:
            if (data->u.hid_host.control_cid == cid || data->u.hid_host.interrupt_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_AVP:
            if (data->u.avp.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_ATT:
            if (data->u.att.l2c_cid == cid)
            {
                return true;
            }
            break;

        default:
            break;
        }

        data = data->next;
    }

    return false;
}

void bt_roleswap_rfc_info_free(uint8_t bd_addr[6],
                               uint8_t dlci)
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_rfc_data_find(bd_addr, dlci);
    if (data != NULL)
    {
        uint16_t cid;

        cid = data->u.rfc_data.l2c_cid;
        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_rfc_ctrl_check_free(bd_addr, cid);
    }
}

void bt_roleswap_spp_info_free(uint8_t bd_addr[6],
                               uint8_t local_server_chann)
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_spp_find(bd_addr, local_server_chann);
    if (data != NULL)
    {
        uint8_t dlci;

        dlci = data->u.spp.rfc_dlci;
        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_rfc_info_free(bd_addr, dlci);
    }
}

void bt_roleswap_a2dp_info_free(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_A2DP);
    if (data != NULL)
    {
        uint16_t sig_cid;
        uint16_t stream_cid;

        sig_cid = data->u.a2dp.sig_cid;
        stream_cid = data->u.a2dp.stream_cid;
        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_l2c_info_free(bd_addr, sig_cid);
        bt_roleswap_l2c_info_free(bd_addr, stream_cid);
    }
}

void bt_roleswap_avrcp_info_free(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_AVRCP);
    if (data != NULL)
    {
        uint16_t l2c_cid;

        l2c_cid = data->u.avrcp.l2c_cid;
        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_l2c_info_free(bd_addr, l2c_cid);
    }
}

void bt_roleswap_hfp_info_free(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_HFP);
    if (data != NULL)
    {
        uint8_t dlci;

        dlci = data->u.hfp.rfc_dlci;
        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_rfc_info_free(bd_addr, dlci);
    }
}

void bt_roleswap_pbap_info_free(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_PBAP);
    if (data != NULL)
    {
        uint16_t cid;
        uint8_t  dlci;
        bool     obex_over_l2c;

        cid = data->u.pbap.l2c_cid;
        dlci = data->u.pbap.rfc_dlci;
        obex_over_l2c = data->u.pbap.obex_psm ? true : false;

        bt_roleswap_info_free(bd_addr, data);

        if (obex_over_l2c)
        {
            bt_roleswap_l2c_info_free(bd_addr, cid);
        }
        else
        {
            bt_roleswap_rfc_info_free(bd_addr, dlci);
        }
    }
}

void bt_roleswap_hid_device_info_free(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_HID_DEVICE);
    if (data != NULL)
    {
        uint16_t control_cid;
        uint16_t interrupt_cid;

        control_cid = data->u.hid_device.control_cid;
        interrupt_cid = data->u.hid_device.interrupt_cid;

        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_l2c_info_free(bd_addr, control_cid);
        bt_roleswap_l2c_info_free(bd_addr, interrupt_cid);
    }
}

void bt_roleswap_hid_host_info_free(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_HID_HOST);
    if (data != NULL)
    {
        uint16_t control_cid;
        uint16_t interrupt_cid;

        control_cid = data->u.hid_host.control_cid;
        interrupt_cid = data->u.hid_host.interrupt_cid;

        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_l2c_info_free(bd_addr, control_cid);
        bt_roleswap_l2c_info_free(bd_addr, interrupt_cid);
    }
}

void bt_roleswap_att_info_free(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ATT);
    if (data != NULL)
    {
        uint16_t  cid;

        cid = data->u.att.l2c_cid;
        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_l2c_info_free(bd_addr, cid);
    }
}

void bt_roleswap_iap_info_free(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_IAP);
    if (data != NULL)
    {
        uint8_t dlci;

        dlci = data->u.iap.dlci;
        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_rfc_info_free(bd_addr, dlci);
    }
}

void bt_roleswap_avp_info_free(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_AVP);
    if (data != NULL)
    {
        uint16_t cid;

        cid = data->u.avp.l2c_cid;
        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_l2c_info_free(bd_addr, cid);
    }
}

void bt_roleswap_bt_rfc_info_free(uint8_t bd_addr[6],
                                  uint8_t local_server_chann)
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_bt_rfc_find(bd_addr, local_server_chann);
    if (data != NULL)
    {
        uint8_t dlci;

        dlci = data->u.bt_rfc.dlci;
        bt_roleswap_info_free(bd_addr, data);
        bt_roleswap_rfc_info_free(bd_addr, dlci);
    }
}

void bt_roleswap_transfer(uint8_t bd_addr[6])
{
    T_ROLESWAP_INFO *base;
    T_ROLESWAP_DATA *data;
    uint8_t         *buf;
    uint8_t         *p;
    uint16_t         len = 0;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base == NULL)
    {
        return;
    }

    data = (T_ROLESWAP_DATA *)base->info_list.p_first;
    while (data)
    {
        switch (data->type)
        {
        case ROLESWAP_TYPE_ACL:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_ACL_INFO);
            break;
        case ROLESWAP_TYPE_SCO:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_SCO_INFO);
            break;
        case ROLESWAP_TYPE_SM:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_SM_INFO);
            break;
        case ROLESWAP_TYPE_L2C:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_L2C_INFO);
            break;
        case ROLESWAP_TYPE_RFC_CTRL:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_RFC_CTRL_INFO);
            break;
        case ROLESWAP_TYPE_RFC_DATA:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_RFC_DATA_INFO);
            break;
        case ROLESWAP_TYPE_A2DP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_A2DP_INFO);
            break;
        case ROLESWAP_TYPE_AVRCP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_AVRCP_INFO);
            break;
        case ROLESWAP_TYPE_HFP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_HFP_INFO);
            break;
        case ROLESWAP_TYPE_SPP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_SPP_INFO);
            break;
        case ROLESWAP_TYPE_PBAP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_PBAP_INFO);
            break;
        case ROLESWAP_TYPE_HID_DEVICE:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_HID_DEVICE_INFO);
            break;
        case ROLESWAP_TYPE_HID_HOST:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_HID_HOST_INFO);
            break;
        case ROLESWAP_TYPE_IAP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_IAP_INFO);
            break;
        case ROLESWAP_TYPE_AVP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_AVP_INFO);
            break;
        case ROLESWAP_TYPE_BT_RFC:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_BT_RFC_INFO);
            break;
        case ROLESWAP_TYPE_ATT:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_ATT_INFO);
            break;
        default:
            break;
        }

        data = data->next;
    }

    if (len != 0)
    {
        buf = os_mem_zalloc2(len);
        if (buf == NULL)
        {
            return;
        }

        p = buf;
        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            switch (data->type)
            {
            case ROLESWAP_TYPE_ACL:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_ACL);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_ACL_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_ACL_INFO));
                    ARRAY_TO_STREAM(p, &data->u.acl, sizeof(T_ROLESWAP_ACL_INFO));

                    memcpy(bd_addr, &data->u.acl.bd_addr, 6);
                }
                break;

            case ROLESWAP_TYPE_SCO:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_SCO);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_SCO_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_SCO_INFO));
                    ARRAY_TO_STREAM(p, &data->u.sco, sizeof(T_ROLESWAP_SCO_INFO));
                }
                break;

            case ROLESWAP_TYPE_SM:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_SM);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_SM_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_SM_INFO));
                    ARRAY_TO_STREAM(p, &data->u.sm, sizeof(T_ROLESWAP_SM_INFO));
                }
                break;

            case ROLESWAP_TYPE_L2C:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_L2C);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_L2C_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_L2C_INFO));
                    ARRAY_TO_STREAM(p, &data->u.l2c, sizeof(T_ROLESWAP_L2C_INFO));
                }
                break;

            case ROLESWAP_TYPE_RFC_CTRL:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_RFC);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_RFC_CTRL_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_RFC_CTRL_INFO));
                    ARRAY_TO_STREAM(p, &data->u.rfc_ctrl, sizeof(T_ROLESWAP_RFC_CTRL_INFO));
                }
                break;

            case ROLESWAP_TYPE_RFC_DATA:
                {
                    if (rfc_data_roleswap_info_get(data->u.rfc_data.bd_addr,
                                                   data->u.rfc_data.dlci,
                                                   &data->u.rfc_data))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_RFC);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_RFC_DATA_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_RFC_DATA_INFO));
                        ARRAY_TO_STREAM(p, &data->u.rfc_data, sizeof(T_ROLESWAP_RFC_DATA_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_A2DP:
                {
                    if (bt_a2dp_roleswap_info_get(data->u.a2dp.bd_addr, &data->u.a2dp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_A2DP);
                        if (data->u.a2dp.stream_cid == 0)
                        {
                            LE_UINT8_TO_STREAM(p, ROLESWAP_A2DP_CONN);
                        }
                        else
                        {
                            LE_UINT8_TO_STREAM(p, ROLESWAP_A2DP_STREAM_CONN);
                        }
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_A2DP_INFO));
                        ARRAY_TO_STREAM(p, &data->u.a2dp, sizeof(T_ROLESWAP_A2DP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_AVRCP:
                {
                    if (bt_avrcp_roleswap_info_get(data->u.avrcp.bd_addr, &data->u.avrcp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_AVRCP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_AVRCP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_AVRCP_INFO));
                        ARRAY_TO_STREAM(p, &data->u.avrcp, sizeof(T_ROLESWAP_AVRCP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_HFP:
                {
                    if (bt_hfp_roleswap_info_get(data->u.hfp.bd_addr, &data->u.hfp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HFP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_HFP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HFP_INFO));
                        ARRAY_TO_STREAM(p, &data->u.hfp, sizeof(T_ROLESWAP_HFP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_SPP:
                {
                    if (bt_spp_roleswap_info_get(data->u.spp.bd_addr, data->u.spp.local_server_chann,
                                                 &data->u.spp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_SPP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_SPP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_SPP_INFO));
                        ARRAY_TO_STREAM(p, &data->u.spp, sizeof(T_ROLESWAP_SPP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_PBAP:
                {
                    if (bt_pbap_roleswap_info_get(data->u.pbap.bd_addr, &data->u.pbap))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_PBAP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_PBAP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_PBAP_INFO));
                        ARRAY_TO_STREAM(p, &data->u.pbap, sizeof(T_ROLESWAP_PBAP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_HID_DEVICE:
                {
                    if (bt_hid_device_roleswap_info_get(data->u.hid_device.bd_addr, &data->u.hid_device))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HID_DEVICE);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_HID_DEVICE_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HID_DEVICE_INFO));
                        ARRAY_TO_STREAM(p, &data->u.hid_device, sizeof(T_ROLESWAP_HID_DEVICE_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_HID_HOST:
                {
                    if (bt_hid_host_roleswap_info_get(data->u.hid_host.bd_addr, &data->u.hid_host))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HID_HOST);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_HID_HOST_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HID_HOST_INFO));
                        ARRAY_TO_STREAM(p, &data->u.hid_host, sizeof(T_ROLESWAP_HID_HOST_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_IAP:
                {
                    if (bt_iap_roleswap_info_get(data->u.iap.bd_addr, &data->u.iap))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_IAP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_IAP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_IAP_INFO));
                        ARRAY_TO_STREAM(p, &data->u.iap, sizeof(T_ROLESWAP_IAP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_AVP:
                {
                    if (bt_avp_roleswap_info_get(data->u.avp.bd_addr, &data->u.avp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_AVP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_AVP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_AVP_INFO));
                        ARRAY_TO_STREAM(p, &data->u.avp, sizeof(T_ROLESWAP_AVP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_BT_RFC:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_BT_RFC);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_BT_RFC_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_BT_RFC_INFO));
                    ARRAY_TO_STREAM(p, &data->u.bt_rfc, sizeof(T_ROLESWAP_BT_RFC_INFO));
                }
                break;

            case ROLESWAP_TYPE_ATT:
                {
                    if (bt_att_roleswap_info_get(data->u.att.bd_addr, &data->u.att))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_ATT);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_ATT_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_ATT_INFO));
                        ARRAY_TO_STREAM(p, &data->u.att, sizeof(T_ROLESWAP_ATT_INFO));
                    }
                }
                break;

            default:
                break;
            }

            data = data->next;
        }

        remote_async_msg_relay(btm_db.relay_handle,
                               BT_REMOTE_MSG_ROLESWAP_INFO_XMIT,
                               buf,
                               p - buf,
                               false);

        os_mem_free(buf);
    }
}

void bt_roleswap_sync(uint8_t bd_addr[6])
{
    T_ROLESWAP_INFO *base;
    T_ROLESWAP_DATA *data;
    uint8_t         *buf;
    uint8_t         *p;
    uint16_t         len = 0;

    base = bt_roleswap_info_base_find(bd_addr);
    if (base == NULL)
    {
        return;
    }

    data = (T_ROLESWAP_DATA *)base->info_list.p_first;
    while (data)
    {
        switch (data->type)
        {
        case ROLESWAP_TYPE_ACL:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_ACL_TRANSACT);
            break;
        case ROLESWAP_TYPE_RFC_DATA:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_RFC_TRANSACT);
            break;
        case ROLESWAP_TYPE_SPP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_SPP_TRANSACT);
            break;
        case ROLESWAP_TYPE_A2DP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_A2DP_TRANSACT);
            break;
        case ROLESWAP_TYPE_AVRCP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_AVRCP_TRANSACT);
            break;
        case ROLESWAP_TYPE_HFP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_HFP_TRANSACT);
            break;
        case ROLESWAP_TYPE_PBAP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_PBAP_TRANSACT);
            break;
        case ROLESWAP_TYPE_HID_DEVICE:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_HID_DEVICE_TRANSACT);
            break;
        case ROLESWAP_TYPE_HID_HOST:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_HID_HOST_TRANSACT);
            break;
        case ROLESWAP_TYPE_IAP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_IAP_TRANSACT);
            break;
        case ROLESWAP_TYPE_AVP:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_AVP_TRANSACT);
            break;
        case ROLESWAP_TYPE_ATT:
            len += ROLESWAP_MSG_HDR_LEN + sizeof(T_ROLESWAP_ATT_TRANSACT);
            break;
        default:
            break;
        }

        data = data->next;
    }

    if (len == 0)
    {
        return;
    }

    buf = os_mem_zalloc2(len);
    if (buf == NULL)
    {
        return;
    }

    p = buf;
    data = (T_ROLESWAP_DATA *)base->info_list.p_first;
    while (data)
    {
        switch (data->type)
        {
        case ROLESWAP_TYPE_ACL:
            {
                T_ROLESWAP_ACL_TRANSACT acl_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_ACL);
                LE_UINT8_TO_STREAM(p, ROLESWAP_ACL_UPDATE);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_ACL_TRANSACT));

                memcpy(acl_transact.bd_addr, data->u.acl.bd_addr, 6);
                acl_transact.role = data->u.acl.role;
                acl_transact.mode = data->u.acl.mode;
                acl_transact.link_policy = data->u.acl.link_policy;
                acl_transact.superv_tout = data->u.acl.superv_tout;
                acl_transact.authen_state = data->u.acl.authen_state;
                acl_transact.encrypt_state = data->u.acl.encrypt_state;

                ARRAY_TO_STREAM(p, &acl_transact, sizeof(T_ROLESWAP_ACL_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_RFC_DATA:
            if (rfc_data_roleswap_info_get(data->u.rfc_data.bd_addr,
                                           data->u.rfc_data.dlci,
                                           &data->u.rfc_data))
            {
                T_ROLESWAP_RFC_TRANSACT rfc_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_RFC);
                LE_UINT8_TO_STREAM(p, ROLESWAP_RFC_DATA_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_RFC_TRANSACT));

                memcpy(rfc_transact.bd_addr, data->u.rfc_data.bd_addr, 6);
                rfc_transact.dlci = data->u.rfc_data.dlci;
                rfc_transact.remote_remain_credits = data->u.rfc_data.remote_remain_credits;
                rfc_transact.given_credits = data->u.rfc_data.given_credits;

                ARRAY_TO_STREAM(p, &rfc_transact, sizeof(T_ROLESWAP_RFC_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_SPP:
            if (bt_spp_roleswap_info_get(bd_addr, data->u.spp.local_server_chann, &data->u.spp))
            {
                T_ROLESWAP_SPP_TRANSACT spp_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_SPP);
                LE_UINT8_TO_STREAM(p, ROLESWAP_SPP_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_SPP_TRANSACT));

                memcpy(spp_transact.bd_addr, data->u.spp.bd_addr, 6);
                spp_transact.local_server_chann = data->u.spp.local_server_chann;
                spp_transact.state = data->u.spp.state;
                spp_transact.remote_credit = data->u.spp.remote_credit;

                ARRAY_TO_STREAM(p, &spp_transact, sizeof(T_ROLESWAP_SPP_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_A2DP:
            if (bt_a2dp_roleswap_info_get(bd_addr, &data->u.a2dp))
            {
                T_ROLESWAP_A2DP_TRANSACT  a2dp_transact;
                uint8_t                  *codec_info;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_A2DP);
                LE_UINT8_TO_STREAM(p, ROLESWAP_A2DP_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_A2DP_TRANSACT));

                memcpy(a2dp_transact.bd_addr, data->u.a2dp.bd_addr, 6);
                a2dp_transact.sig_state = data->u.a2dp.sig_state;
                a2dp_transact.state = data->u.a2dp.state;
                a2dp_transact.tx_trans_label = data->u.a2dp.tx_trans_label;
                a2dp_transact.rx_start_trans_label = data->u.a2dp.rx_start_trans_label;
                a2dp_transact.last_seq_number = data->u.a2dp.last_seq_number;
                a2dp_transact.cmd_flag  = data->u.a2dp.cmd_flag;
                a2dp_transact.codec_type = data->u.a2dp.codec_type;
                codec_info = (uint8_t *)&a2dp_transact.codec_info;
                ARRAY_TO_STREAM(codec_info, &data->u.a2dp.codec_info, sizeof(T_BT_A2DP_CODEC_INFO));

                ARRAY_TO_STREAM(p, &a2dp_transact, sizeof(T_ROLESWAP_A2DP_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_AVRCP:
            if (bt_avrcp_roleswap_info_get(bd_addr, &data->u.avrcp))
            {
                T_ROLESWAP_AVRCP_TRANSACT avrcp_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_AVRCP);
                LE_UINT8_TO_STREAM(p, ROLESWAP_AVRCP_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_AVRCP_TRANSACT));

                memcpy(avrcp_transact.bd_addr, data->u.avrcp.bd_addr, 6);
                avrcp_transact.avctp_state = data->u.avrcp.avctp_state;
                avrcp_transact.state = data->u.avrcp.state;
                avrcp_transact.play_status = data->u.avrcp.play_status;
                avrcp_transact.cmd_credits = data->u.avrcp.cmd_credits;
                avrcp_transact.transact_label = data->u.avrcp.transact_label;
                avrcp_transact.vol_change_pending_transact = data->u.avrcp.vol_change_pending_transact;

                ARRAY_TO_STREAM(p, &avrcp_transact, sizeof(T_ROLESWAP_AVRCP_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_HFP:
            if (bt_hfp_roleswap_info_get(bd_addr, &data->u.hfp))
            {
                T_ROLESWAP_HFP_TRANSACT hfp_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HFP);
                LE_UINT8_TO_STREAM(p, ROLESWAP_HFP_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HFP_TRANSACT));

                memcpy(hfp_transact.bd_addr, data->u.hfp.bd_addr, 6);
                hfp_transact.state = data->u.hfp.state;
                hfp_transact.batt_report_type = data->u.hfp.batt_report_type;
                hfp_transact.at_cmd_credits = data->u.hfp.at_cmd_credits;
                hfp_transact.curr_call_status = data->u.hfp.curr_call_status;
                hfp_transact.prev_call_status = data->u.hfp.prev_call_status;
                hfp_transact.service_status = data->u.hfp.service_status;
                hfp_transact.supported_features = data->u.hfp.supported_features;
                hfp_transact.codec_type = data->u.hfp.codec_type;

                ARRAY_TO_STREAM(p, &hfp_transact, sizeof(T_ROLESWAP_HFP_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_PBAP:
            if (bt_pbap_roleswap_info_get(bd_addr, &data->u.pbap))
            {
                T_ROLESWAP_PBAP_TRANSACT pbap_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_PBAP);
                LE_UINT8_TO_STREAM(p, ROLESWAP_PBAP_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_PBAP_TRANSACT));

                memcpy(pbap_transact.bd_addr, data->u.pbap.bd_addr, 6);
                pbap_transact.obex_state = data->u.pbap.obex_state;
                pbap_transact.pbap_state = data->u.pbap.pbap_state;
                pbap_transact.path = data->u.pbap.path;
                pbap_transact.repos = data->u.pbap.repos;

                ARRAY_TO_STREAM(p, &pbap_transact, sizeof(T_ROLESWAP_PBAP_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_HID_DEVICE:
            if (bt_hid_device_roleswap_info_get(bd_addr, &data->u.hid_device))
            {
                T_ROLESWAP_HID_DEVICE_TRANSACT hid_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HID_DEVICE);
                LE_UINT8_TO_STREAM(p, ROLESWAP_HID_DEVICE_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HID_DEVICE_TRANSACT));

                memcpy(hid_transact.bd_addr, data->u.hid_device.bd_addr, 6);
                hid_transact.proto_mode = data->u.hid_device.proto_mode;
                hid_transact.control_state = data->u.hid_device.control_state;
                hid_transact.interrupt_state = data->u.hid_device.interrupt_state;

                ARRAY_TO_STREAM(p, &hid_transact, sizeof(T_ROLESWAP_HID_DEVICE_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_HID_HOST:
            if (bt_hid_host_roleswap_info_get(bd_addr, &data->u.hid_host))
            {
                T_ROLESWAP_HID_HOST_TRANSACT hid_host_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HID_HOST);
                LE_UINT8_TO_STREAM(p, ROLESWAP_HID_HOST_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HID_HOST_TRANSACT));

                memcpy(hid_host_transact.bd_addr, data->u.hid_host.bd_addr, 6);
                hid_host_transact.proto_mode = data->u.hid_host.proto_mode;
                hid_host_transact.control_state = data->u.hid_host.control_state;
                hid_host_transact.interrupt_state = data->u.hid_host.interrupt_state;

                ARRAY_TO_STREAM(p, &hid_host_transact, sizeof(T_ROLESWAP_HID_HOST_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_IAP:
            if (bt_iap_roleswap_info_get(bd_addr, &data->u.iap))
            {
                T_ROLESWAP_IAP_TRANSACT iap_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_IAP);
                LE_UINT8_TO_STREAM(p, ROLESWAP_IAP_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_IAP_TRANSACT));

                memcpy(iap_transact.bd_addr, bd_addr, 6);
                iap_transact.remote_credit = data->u.iap.remote_credit;
                iap_transact.state = data->u.iap.state;
                iap_transact.acc_pkt_seq = data->u.iap.acc_pkt_seq;
                iap_transact.acked_seq = data->u.iap.acked_seq;
                iap_transact.dev_pkt_seq = data->u.iap.dev_pkt_seq;

                ARRAY_TO_STREAM(p, &iap_transact, sizeof(T_ROLESWAP_IAP_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_AVP:
            if (bt_avp_roleswap_info_get(bd_addr, &data->u.avp))
            {
                T_ROLESWAP_AVP_TRANSACT avp_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_AVP);
                LE_UINT8_TO_STREAM(p, ROLESWAP_AVP_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_AVP_TRANSACT));

                memcpy(avp_transact.bd_addr, data->u.avp.bd_addr, 6);
                avp_transact.state = data->u.avp.state;

                ARRAY_TO_STREAM(p, &avp_transact, sizeof(T_ROLESWAP_AVP_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_ATT:
            if (bt_att_roleswap_info_get(bd_addr, &data->u.att))
            {
                T_ROLESWAP_ATT_TRANSACT att_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_ATT);
                LE_UINT8_TO_STREAM(p, ROLESWAP_ATT_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_ATT_TRANSACT));

                memcpy(att_transact.bd_addr, data->u.att.bd_addr, 6);
                att_transact.state = data->u.att.state;

                ARRAY_TO_STREAM(p, &att_transact, sizeof(T_ROLESWAP_ATT_TRANSACT));
            }
            break;

        default:
            break;
        }

        data = data->next;
    }

    remote_async_msg_relay(btm_db.relay_handle,
                           BT_REMOTE_MSG_ROLESWAP_INFO_XMIT,
                           buf,
                           p - buf,
                           false);

    os_mem_free(buf);
}

void bt_roleswap_handle_link_policy(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *data;

    data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
    if (data != NULL)
    {
        uint16_t link_policy;

        if (bt_link_policy_get(bd_addr, &link_policy) == true)
        {
            data->u.acl.link_policy = link_policy;
        }
    }
}

void bt_roleswap_handle_acl_status(uint8_t   bd_addr[6],
                                   T_BT_MSG  msg,
                                   void     *buf)
{
    bt_roleswap_proto->acl_status(bd_addr, msg, buf);
}

void bt_roleswap_handle_profile_conn(uint8_t  bd_addr[6],
                                     uint32_t profile_mask,
                                     uint8_t  param)
{
    bt_roleswap_proto->profile_conn(bd_addr, profile_mask, param);
}

void bt_roleswap_handle_profile_disconn(uint8_t                           bd_addr[6],
                                        T_ROLESWAP_PROFILE_DISCONN_PARAM *param)
{
    bt_roleswap_proto->profile_disconn(bd_addr, param);
}

void bt_roleswap_handle_sco_conn(uint8_t bd_addr[6])
{
    gap_br_get_handover_sco_info(bd_addr);
}

void bt_roleswap_handle_sco_disconn(uint8_t  bd_addr[6],
                                    uint16_t cause)
{
    bt_roleswap_proto->sco_disconn(bd_addr, cause);
}

void bt_roleswap_handle_bt_rfc_conn(uint8_t bd_addr[6],
                                    uint8_t server_channel)
{
    bt_roleswap_proto->bt_rfc_conn(bd_addr, server_channel);
}

void bt_roleswap_handle_bt_rfc_disconn(uint8_t  bd_addr[6],
                                       uint8_t  server_channel,
                                       uint16_t cause)
{
    bt_roleswap_proto->bt_rfc_disconn(bd_addr, server_channel, cause);
}

void bt_roleswap_handle_bt_avp_conn(uint8_t bd_addr[6])
{
    bt_roleswap_proto->bt_avp_conn(bd_addr);
}

void bt_roleswap_handle_bt_avp_disconn(uint8_t bd_addr[6],
                                       uint16_t cause)
{
    bt_roleswap_proto->bt_avp_disconn(bd_addr, cause);
}

void bt_roleswap_handle_bt_att_conn(uint8_t bd_addr[6])
{
    bt_roleswap_proto->bt_att_conn(bd_addr);
}

void bt_roleswap_handle_bt_att_disconn(uint8_t  bd_addr[6],
                                       uint16_t cause)
{
    bt_roleswap_proto->bt_att_disconn(bd_addr, cause);
}

void bt_roleswap_handle_ctrl_conn(void)
{
    bt_roleswap_proto->ctrl_conn();
}

void bt_roleswap_recv(uint8_t  *data,
                      uint16_t  data_len)
{
    bt_roleswap_proto->recv(data, data_len);
}

bool remote_roleswap_start(uint8_t                      bd_addr[6],
                           uint8_t                      context,
                           bool                         stop_after_shadow,
                           P_REMOTE_ROLESWAP_SYNC_CBACK cback)
{
    return bt_roleswap_proto->start(bd_addr, context, stop_after_shadow, cback);
}

bool remote_roleswap_cfm(bool    accept,
                         uint8_t context)
{
    return bt_roleswap_proto->cfm(accept, context);
}

bool remote_roleswap_stop(uint8_t bd_addr[6])
{
    return bt_roleswap_proto->stop(bd_addr);
}

T_BT_CLK_REF bt_roleswap_get_piconet_clk(T_BT_CLK_REF  clk_ref,
                                         uint32_t     *bb_clock_timer,
                                         uint16_t     *bb_clock_us)
{
    return bt_roleswap_proto->get_piconet_clk(clk_ref, bb_clock_timer, bb_clock_us);
}

bool bt_roleswap_get_piconet_id(T_BT_CLK_REF  clk_ref,
                                uint8_t      *clk_index,
                                uint8_t      *role)
{
    return bt_roleswap_proto->get_piconet_id(clk_ref, clk_index, role);
}

bool bt_roleswap_init(void)
{
    os_queue_init(&btm_db.roleswap_info_list);
    bt_roleswap_proto = &bt_sniffing_proto;
    gap_br_reg_handover_cb(bt_roleswap_proto->cback);

    return true;
}

#else
bool bt_roleswap_init(void)
{
    return true;
}

void bt_roleswap_handle_acl_status(uint8_t   bd_addr[6],
                                   T_BT_MSG  msg,
                                   void     *buf)
{

}

void bt_roleswap_handle_link_policy(uint8_t bd_addr[6])
{

}

void bt_roleswap_handle_profile_conn(uint8_t  bd_addr[6],
                                     uint32_t profile_mask,
                                     uint8_t  param)
{

}

void bt_roleswap_handle_profile_disconn(uint8_t                           bd_addr[6],
                                        T_ROLESWAP_PROFILE_DISCONN_PARAM *param)
{

}

void bt_roleswap_handle_sco_disconn(uint8_t  bd_addr[6],
                                    uint16_t cause)
{

}

void bt_roleswap_handle_sco_conn(uint8_t bd_addr[6])
{

}

void bt_roleswap_handle_bt_rfc_conn(uint8_t bd_addr[6],
                                    uint8_t server_chann)
{

}

void bt_roleswap_handle_bt_rfc_disconn(uint8_t  bd_addr[6],
                                       uint8_t  server_chann,
                                       uint16_t cause)
{

}

void bt_roleswap_handle_bt_avp_conn(uint8_t bd_addr[6])
{

}

void bt_roleswap_handle_bt_avp_disconn(uint8_t bd_addr[6],
                                       uint16_t cause)
{

}

void bt_roleswap_handle_ctrl_conn(void)
{

}

void bt_roleswap_recv(uint8_t  *data,
                      uint16_t  data_len)
{

}

T_BT_CLK_REF bt_roleswap_get_piconet_clk(T_BT_CLK_REF  clk,
                                         uint32_t     *bb_clock_timer,
                                         uint16_t     *bb_clock_us)
{
    if (clk == BT_CLK_SNIFFING)
    {
        T_BT_LINK *link_active;

        link_active = bt_link_find(btm_db.active_a2dp_addr);
        if (link_active == NULL)
        {
            BTM_PRINT_TRACE0("bt_roleswap_get_piconet_clk: no sniff handle");
            return BT_CLK_NONE;
        }
        else
        {
            rws_read_bt_piconet_clk(link_active->acl_handle, bb_clock_timer, bb_clock_us);
            BTM_PRINT_TRACE2("bt_roleswap_get_piconet_clk: clk_ref %u,  bb_clock_timer 0x%x",
                             clk, *bb_clock_timer);
            return BT_CLK_SNIFFING;
        }
    }

    return BT_CLK_NONE;
}

bool bt_roleswap_get_piconet_id(T_BT_CLK_REF  clk_ref,
                                uint8_t      *clk_index,
                                uint8_t      *role)
{
    bool get_ret = false;
    if (clk_ref == BT_CLK_SNIFFING)
    {
        T_BT_LINK *link_active;

        link_active = bt_link_find(btm_db.active_a2dp_addr);
        if (link_active == NULL)
        {
            BTM_PRINT_TRACE0("bt_roleswap_get_piconet_id: no sniff handle");
            return false;
        }
        else
        {
            get_ret = bt_clk_index_read(link_active->acl_handle, clk_index, role);
            BTM_PRINT_TRACE4("bt_roleswap_get_piconet_id: handle 0x%x clock_id 0x%x, ref_type 0x%x ,get_ret %d",
                             link_active->acl_handle, *clk_index, clk_ref, get_ret);
            return get_ret;
        }
    }

    return false;
}

bool remote_roleswap_start(uint8_t                      bd_addr[6],
                           uint8_t                      context,
                           bool                         stop_after_shadow,
                           P_REMOTE_ROLESWAP_SYNC_CBACK cback)
{
    return false;
}

bool remote_roleswap_cfm(bool    accept,
                         uint8_t context)
{
    return false;
}

bool remote_roleswap_stop(uint8_t bd_addr[6])
{
    return false;
}
#endif
