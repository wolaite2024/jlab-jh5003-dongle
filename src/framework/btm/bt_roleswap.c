/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_roleswap.h"
#include "bt_roleswap_int.h"

#include "low_stack.h"

#if (CONFIG_REALTEK_BTM_ROLESWAP_SUPPORT == 1)
const T_BT_ROLESWAP_PROTO *bt_roleswap_proto;

T_ROLESWAP_INFO *bt_find_roleswap_info_base(uint8_t bd_addr[6])
{
    uint8_t i;
    T_ROLESWAP_INFO *p_info;

    for (i = 0; i < btm_db.br_link_num; i++)
    {
        p_info = &btm_db.roleswap_info[i];

        if (p_info->used == true && !memcmp(p_info->bd_addr, bd_addr, 6))
        {
            return p_info;
        }
    }

    return NULL;
}

T_ROLESWAP_INFO *bt_alloc_roleswap_info_base(uint8_t bd_addr[6])
{
    uint8_t i;
    T_ROLESWAP_INFO *p_info;

    for (i = 0; i < btm_db.br_link_num; i++)
    {
        p_info = &btm_db.roleswap_info[i];

        if (p_info->used == false)
        {
            p_info->used = true;
            memcpy(p_info->bd_addr, bd_addr, 6);
            return p_info;
        }
    }

    return NULL;
}

bool bt_free_roleswap_info_base(T_ROLESWAP_INFO *p_info)
{
    p_info->used = false;

    return true;
}

void bt_roleswap_dump(void)
{
    uint8_t i;
    T_ROLESWAP_DATA *p_data;

    for (i = 0; i < btm_db.br_link_num; i++)
    {
        if (btm_db.roleswap_info[i].used)
        {
            BTM_PRINT_TRACE1("bt_roleswap_dump: dump info %s",
                             TRACE_BDADDR(btm_db.roleswap_info[i].bd_addr));

            p_data = (T_ROLESWAP_DATA *)btm_db.roleswap_info[i].info_list.p_first;
            while (p_data)
            {
                BTM_PRINT_TRACE3("bt_roleswap_dump: type %d, length %d, data %b",
                                 p_data->type, p_data->length, TRACE_BINARY(p_data->length, &p_data->u));
                p_data = p_data->p_next;
            }
        }
    }
}

bool bt_roleswap_get_sco_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_SCO_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_info->air_mode = p_link->sco_air_mode;
        p_info->pkt_len  = p_link->sco_packet_length;

        return true;
    }

    return false;
}

bool bt_roleswap_get_a2dp_info(uint8_t               bd_addr[6],
                               T_ROLESWAP_A2DP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;
    uint8_t *buf;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_info->codec_type = p_link->a2dp_data.codec_type;
        p_info->role = p_link->a2dp_data.role;
        p_info->content_proect = p_link->a2dp_data.a2dp_content_protect;
        memcpy(p_info->bd_addr, bd_addr, 6);

        buf = (uint8_t *)&p_info->codec_info;
        ARRAY_TO_STREAM(buf, &p_link->a2dp_data.codec_info, sizeof(T_A2DP_CODEC_INFO));

        p_info->last_seq_number = p_link->a2dp_data.last_seq_num;

        return a2dp_get_roleswap_info(bd_addr, p_info);
    }
    return false;
}

bool bt_roleswap_get_avrcp_info(uint8_t                bd_addr[6],
                                T_ROLESWAP_AVRCP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_info->play_status = p_link->avrcp_data.play_status;
        p_info->vol_change_registered = p_link->avrcp_data.vol_change_registered;
        memcpy(p_info->bd_addr, bd_addr, 6);

        return avrcp_get_roleswap_info(bd_addr, p_info);
    }

    return false;
}

bool bt_roleswap_get_hfp_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_HFP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_info->bat_report_type = p_link->hfp_data.bat_report_type;
        p_info->call_status = p_link->hfp_data.call_status;
        p_info->prev_call_status = p_link->hfp_data.prev_call_status;
        p_info->service_status = p_link->hfp_data.ag_status_ind.service_status;
        p_info->supported_features = p_link->hfp_data.supported_features;
        p_info->codec_type = p_link->hfp_data.codec_type;
        memcpy(p_info->bd_addr, bd_addr, 6);

        return hfp_get_roleswap_info(bd_addr, p_info);
    }

    return false;
}

bool bt_roleswap_get_pbap_info(uint8_t               bd_addr[6],
                               T_ROLESWAP_PBAP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_info->path = p_link->pbap_data.path;
        p_info->repos = (T_PBAP_REPOSITORY)p_link->pbap_data.repos;
        memcpy(p_info->bd_addr, bd_addr, 6);

        return pbap_get_roleswap_info(bd_addr, p_info);
    }

    return false;
}

bool bt_roleswap_get_hid_device_info(uint8_t                     bd_addr[6],
                                     T_ROLESWAP_HID_DEVICE_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_info->proto_mode = p_link->hid_data.proto_mode;
        memcpy(p_info->bd_addr, bd_addr, 6);

        return hid_device_get_roleswap_info(bd_addr, p_info);
    }

    return false;
}

bool bt_roleswap_get_hid_host_info(uint8_t                     bd_addr[6],
                                   T_ROLESWAP_HID_HOST_INFO   *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_info->proto_mode = p_link->hid_data.proto_mode;
        memcpy(p_info->bd_addr, bd_addr, 6);

        return hid_host_get_roleswap_info(bd_addr, p_info);
    }

    return false;
}

bool bt_roleswap_get_att_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_ATT_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        memcpy(p_info->bd_addr, bd_addr, 6);

        return att_get_roleswap_info(bd_addr, p_info);
    }

    return false;
}

bool bt_roleswap_get_spp_info(uint8_t              bd_addr[6],
                              uint8_t              local_server_chann,
                              T_ROLESWAP_SPP_INFO *p_info)
{
    if (spp_get_roleswap_info(bd_addr, local_server_chann, p_info))
    {
        memcpy(p_info->bd_addr, bd_addr, 6);
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_roleswap_get_iap_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_IAP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        memcpy(p_info->bd_addr, bd_addr, 6);
        return iap_get_roleswap_info(bd_addr, p_info);
    }

    return false;
}

bool bt_roleswap_get_avp_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_AVP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        memcpy(p_info->bd_addr, bd_addr, 6);
        return avp_get_roleswap_info(bd_addr, p_info);
    }

    return false;
}

bool bt_roleswap_get_rfc_info(uint8_t                   bd_addr[6],
                              uint8_t                   dlci,
                              T_ROLESWAP_RFC_DATA_INFO *p_data,
                              T_ROLESWAP_RFC_CTRL_INFO *p_ctrl)
{
    return rfc_get_roleswap_info(bd_addr, dlci, p_data, p_ctrl);
}

bool bt_roleswap_get_bt_rfc_info(uint8_t                 bd_addr[6],
                                 uint8_t                 local_server_chann,
                                 T_ROLESWAP_BT_RFC_INFO *p_info)
{
    if (bt_rfc_get_roleswap_info(bd_addr, local_server_chann, p_info))
    {
        memcpy(p_info->bd_addr, bd_addr, 6);
        p_info->local_server_chann = local_server_chann;

        return true;
    }

    return false;
}

bool bt_roleswap_set_acl_info(T_ROLESWAP_ACL_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(p_info->bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(p_info->bd_addr);

        if (p_link != NULL)
        {
            bt_sniff_mode_config(p_info->bd_addr,
                                 BT_SNIFF_INTERVAL_BASE + p_link->link_id * BT_SNIFF_INTERVAL_OFFSET,
                                 BT_SNIFF_ATTEMPT,
                                 BT_SNIFF_TIMEOUT,
                                 BT_PM_TIMEOUT);
        }
    }

    if (p_link != NULL)
    {
        p_link->acl_handle = p_info->handle;
        p_link->acl_link_state = BT_LINK_STATE_CONNECTED;

        if (p_info->role == 0)  //master
        {
            p_link->acl_link_role_master = true;
        }
        else
        {
            p_link->acl_link_role_master = false;
        }

        if (p_info->mode == 2)  //sniff mode
        {
            p_link->pm_state = BT_LINK_PM_STATE_SNIFF;
        }
        else
        {
            p_link->pm_state = BT_LINK_PM_STATE_ACTIVE;
        }

        p_link->acl_link_authenticated = p_info->authen_state;

        if (p_info->encrypt_state == 1) //encrypt on
        {
            p_link->acl_link_encrypted = true;
        }
        else
        {
            p_link->acl_link_encrypted = false;
        }

        p_link->acl_link_policy = p_info->link_policy;
        if ((p_link->acl_link_policy & GAP_LINK_POLICY_SNIFF_MODE) == 0)
        {
            p_link->pm_enable = false;
        }
        else
        {
            p_link->pm_enable = true;
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool bt_roleswap_set_sco_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_SCO_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->sco_handle = p_info->handle;
        p_link->sco_air_mode = p_info->air_mode;
        p_link->sco_packet_length = p_info->pkt_len;

        return true;
    }
    else
    {
        return false;
    }
}

bool bt_roleswap_set_rfc_ctrl_info(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_RFC_CTRL_INFO *p_info)
{
    return rfc_set_ctrl_roleswap_info(bd_addr, p_info);
}

bool bt_roleswap_set_rfc_data_info(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_RFC_DATA_INFO *p_info)
{
    uint8_t profile_idx;

    switch (p_info->uuid)
    {
    case UUID_PBAP:
        profile_idx = pbap_get_rfc_profile_idx();
        break;

    case UUID_SERIAL_PORT:
        {
            T_ROLESWAP_DATA *p_data = bt_find_roleswap_spp_by_dlci(bd_addr, p_info->dlci);

            if (p_data == NULL)
            {
                return false;
            }

            if (spp_get_rfc_profile_idx(p_data->u.spp.local_server_chann, &profile_idx) == false)
            {
                return false;
            }
        }
        break;

    case UUID_HANDSFREE:
    case UUID_HEADSET:
        profile_idx = hfp_get_rfc_profile_idx(p_info->uuid);
        break;

    case UUID_IAP:
        profile_idx = iap_get_rfc_profile_idx();
        break;

    case UUID_BTRFC:
        {
            T_ROLESWAP_DATA *p_data = bt_find_roleswap_bt_rfc_by_dlci(bd_addr, p_info->dlci);

            if (p_data == NULL)
            {
                return false;
            }

            if (bt_rfc_find_profile_idx_by_server_chann(p_data->u.bt_rfc.local_server_chann,
                                                        &profile_idx) == false)
            {
                return false;
            }
        }
        break;

    default:
        return false;
    }

    return rfc_set_data_roleswap_info(bd_addr, profile_idx, p_info);
}

bool bt_roleswap_set_spp_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_SPP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (spp_set_roleswap_info(bd_addr, p_info) == false)
    {
        return false;
    }

    if (p_link->spp_data.spp_ref == 0)
    {
        p_link->connected_profile |= SPP_PROFILE_MASK;
        p_link->spp_data.iap_authen_flag = 0;
    }
    p_link->spp_data.spp_ref++;

    return true;
}

bool bt_roleswap_set_a2dp_info(uint8_t               bd_addr[6],
                               T_ROLESWAP_A2DP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;
    uint8_t *buf;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (a2dp_set_roleswap_info(bd_addr, p_info) == false)
    {
        return false;
    }

    p_link->connected_profile |= A2DP_PROFILE_MASK;

    p_link->a2dp_data.codec_type = p_info->codec_type;
    p_link->a2dp_data.role = p_info->role;
    p_link->a2dp_data.a2dp_content_protect = p_info->content_proect;
    p_link->a2dp_data.last_seq_num = p_info->last_seq_number;
    buf = (uint8_t *)&p_link->a2dp_data.codec_info;
    ARRAY_TO_STREAM(buf, &p_info->codec_info, sizeof(T_A2DP_CODEC_INFO));

    if (p_info->state == A2DP_STATE_STREAMING)
    {
        p_link->a2dp_data.streaming_fg = true;
    }
    else
    {
        p_link->a2dp_data.streaming_fg = false;
    }
    BTM_PRINT_TRACE1("bt_roleswap_set_a2dp_info: p_info->state %d", p_info->state);

    return true;
}

bool bt_roleswap_set_avrcp_info(uint8_t                bd_addr[6],
                                T_ROLESWAP_AVRCP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (avrcp_set_roleswap_info(bd_addr, p_info) == false)
    {
        return false;
    }

    p_link->connected_profile |= AVRCP_PROFILE_MASK;

    p_link->avrcp_data.play_status = p_info->play_status;
    p_link->avrcp_data.vol_change_registered = p_info->vol_change_registered;

    return true;
}

bool bt_roleswap_set_hfp_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_HFP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (hfp_set_roleswap_info(bd_addr, p_info) == false)
    {
        return false;
    }

    if (p_info->uuid == UUID_HANDSFREE)
    {
        p_link->hfp_data.hsp_active_fg = 0;
        p_link->hfp_data.hfp_state = HF_STATE_CONNECTED;
        p_link->connected_profile |= HFP_PROFILE_MASK;
    }
    else
    {
        p_link->hfp_data.hsp_active_fg = 1;
        p_link->hfp_data.hfp_state = HS_STATE_CONNECTED;
        p_link->connected_profile |= HSP_PROFILE_MASK;
    }
    p_link->hfp_data.capabilities = p_info->cpbs;
    p_link->hfp_data.bat_report_type = p_info->bat_report_type;
    p_link->hfp_data.call_status = (T_BT_HFP_CALL_STATUS)p_info->call_status;
    p_link->hfp_data.prev_call_status = (T_BT_HFP_CALL_STATUS)p_info->prev_call_status;
    p_link->hfp_data.ag_status_ind.service_status = (T_HFP_SERVICE_STATUS)p_info->service_status;
    p_link->hfp_data.supported_features = p_info->supported_features;
    p_link->hfp_data.codec_type = (T_HFP_CODEC_TYPE)p_info->codec_type;

    return true;
}

bool bt_roleswap_set_pbap_info(uint8_t               bd_addr[6],
                               T_ROLESWAP_PBAP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (pbap_set_roleswap_info(bd_addr, p_info) == false)
    {
        return false;
    }

    p_link->connected_profile |= PBAP_PROFILE_MASK;

    p_link->pbap_data.path = p_info->path;
    p_link->pbap_data.repos = (T_BT_PBAP_REPOSITORY)p_info->repos;

    return true;
}

bool bt_roleswap_set_hid_device_info(uint8_t                     bd_addr[6],
                                     T_ROLESWAP_HID_DEVICE_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (hid_device_set_roleswap_info(bd_addr, p_info) == false)
    {
        return false;
    }

    p_link->connected_profile |= HID_DEVICE_PROFILE_MASK;

    p_link->hid_data.proto_mode = p_info->proto_mode;

    return true;
}

bool bt_roleswap_set_hid_host_info(uint8_t *bd_addr, T_ROLESWAP_HID_HOST_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (hid_host_set_roleswap_info(bd_addr, p_info) == false)
    {
        return false;
    }

    p_link->connected_profile |= HID_HOST_PROFILE_MASK;

    p_link->hid_data.proto_mode = p_info->proto_mode;

    return true;
}

bool bt_roleswap_set_att_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_ATT_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    return att_set_roleswap_info(bd_addr, p_info);
}

bool bt_roleswap_set_iap_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_IAP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    BTM_PRINT_INFO4("bt_roleswap_set_iap_info: dev max pkt len %d, acc seq %d, acked seq %d, dev seq %d",
                    p_info->dev_max_pkt_len, p_info->acc_pkt_seq, p_info->acked_seq, p_info->dev_pkt_seq);

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (iap_set_roleswap_info(bd_addr, p_info) == false)
    {
        return false;
    }

    p_link->connected_profile |= IAP_PROFILE_MASK;

    return true;
}

bool bt_roleswap_set_avp_info(uint8_t              bd_addr[6],
                              T_ROLESWAP_AVP_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (avp_set_roleswap_info(bd_addr, p_info) == false)
    {
        return false;
    }

    return true;
}

bool bt_roleswap_set_bt_rfc_info(uint8_t                 bd_addr[6],
                                 T_ROLESWAP_BT_RFC_INFO *p_info)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    return bt_rfc_set_roleswap_info(p_info);
}

bool bt_roleswap_info_send(uint8_t   module,
                           uint8_t   submodule,
                           uint8_t  *p_info,
                           uint16_t  len)
{
    uint8_t *p_buf;
    uint8_t *p;
    bool     ret = true;

    p_buf = os_mem_alloc2(ROLESWAP_MSG_HDR_LEN + len);
    if (p_buf == NULL)
    {
        return false;
    }

    BTM_PRINT_TRACE3("bt_roleswap_info_send: module 0x%x, submodule 0x%x, data %b",
                     module, submodule, TRACE_BINARY(len, p_info));

    p = p_buf;
    LE_UINT8_TO_STREAM(p, module);
    LE_UINT8_TO_STREAM(p, submodule);
    LE_UINT16_TO_STREAM(p, len);
    ARRAY_TO_STREAM(p, p_info, len);

    ret = remote_async_msg_relay(btm_db.relay_handle,
                                 BT_REMOTE_MSG_ROLESWAP_INFO_XMIT,
                                 p_buf,
                                 p - p_buf,
                                 false);

    os_mem_free(p_buf);

    return ret;
}

T_ROLESWAP_DATA *bt_find_roleswap_data(uint8_t bd_addr[6],
                                       uint8_t type)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return NULL;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == type)
        {
            return p_data;
        }

        p_data = p_data->p_next;
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_rfc_data(uint8_t bd_addr[6],
                                           uint8_t dlci)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return NULL;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == ROLESWAP_TYPE_RFC_DATA && p_data->u.rfc_data.dlci == dlci)
        {
            return p_data;
        }

        p_data = p_data->p_next;
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_rfc_data_by_cid(uint8_t  bd_addr[6],
                                                  uint16_t cid)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return NULL;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == ROLESWAP_TYPE_RFC_DATA && p_data->u.rfc_data.l2c_cid == cid)
        {
            return p_data;
        }

        p_data = p_data->p_next;
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_spp(uint8_t bd_addr[6],
                                      uint8_t local_server_chann)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return NULL;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == ROLESWAP_TYPE_SPP && p_data->u.spp.local_server_chann == local_server_chann)
        {
            return p_data;
        }

        p_data = p_data->p_next;
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_spp_by_dlci(uint8_t bd_addr[6],
                                              uint8_t dlci)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return NULL;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == ROLESWAP_TYPE_SPP && p_data->u.spp.rfc_dlci == dlci)
        {
            return p_data;
        }

        p_data = p_data->p_next;
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_rfc_ctrl(uint8_t  bd_addr[6],
                                           uint16_t cid)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return NULL;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == ROLESWAP_TYPE_RFC_CTRL && p_data->u.rfc_ctrl.l2c_cid == cid)
        {
            return p_data;
        }

        p_data = p_data->p_next;
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_bt_rfc(uint8_t bd_addr[6],
                                         uint8_t local_server_chann)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return NULL;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == ROLESWAP_TYPE_BT_RFC &&
            p_data->u.bt_rfc.local_server_chann == local_server_chann)
        {
            return p_data;
        }

        p_data = p_data->p_next;
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_bt_rfc_by_dlci(uint8_t bd_addr[6],
                                                 uint8_t dlci)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return NULL;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == ROLESWAP_TYPE_BT_RFC && p_data->u.bt_rfc.dlci == dlci)
        {
            return p_data;
        }

        p_data = p_data->p_next;
    }

    return NULL;
}

T_ROLESWAP_DATA *bt_find_roleswap_l2c(uint8_t  bd_addr[6],
                                      uint16_t cid)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return NULL;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == ROLESWAP_TYPE_L2C && p_data->u.l2c.local_cid == cid)
        {
            return p_data;
        }

        p_data = p_data->p_next;
    }

    return NULL;
}

bool bt_roleswap_alloc_info(uint8_t   bd_addr[6],
                            uint8_t   type,
                            uint8_t  *p_info,
                            uint16_t  len)
{
    T_ROLESWAP_DATA *p_data = NULL;
    T_ROLESWAP_INFO *p_base;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        BTM_PRINT_ERROR1("bt_roleswap_alloc_info: fail to find roleswap base %s",
                         TRACE_BDADDR(bd_addr));
        return false;
    }

    if (type == ROLESWAP_TYPE_RFC_CTRL)
    {
        T_ROLESWAP_RFC_CTRL_INFO *p_ctrl = (T_ROLESWAP_RFC_CTRL_INFO *)p_info;
        p_data = bt_find_roleswap_rfc_ctrl(bd_addr, p_ctrl->l2c_cid);
        if (p_data)
        {
            return false;
        }
    }

    p_data = os_mem_zalloc2(sizeof(T_ROLESWAP_DATA));
    if (p_data == NULL)
    {
        return false;
    }

    p_data->type = type;
    p_data->length = len;
    memcpy((uint8_t *)&p_data->u, p_info, len);

    os_queue_in(&p_base->info_list, p_data);

    bt_roleswap_dump();

    return true;
}

void bt_get_roleswap_rfc_info(uint8_t  bd_addr[6],
                              uint8_t  dlci,
                              uint16_t uuid)
{
    T_ROLESWAP_RFC_CTRL_INFO ctrl;
    T_ROLESWAP_RFC_DATA_INFO data;

    if (bt_roleswap_get_rfc_info(bd_addr, dlci, &data, &ctrl) == false)
    {
        return;
    }

    data.uuid = uuid;
    memcpy(ctrl.bd_addr, bd_addr, 6);
    memcpy(data.bd_addr, bd_addr, 6);

    // check if rfcomm control session need to alloc and send
    if (bt_find_roleswap_rfc_ctrl(bd_addr, data.l2c_cid) == NULL)
    {
        bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_RFC_CTRL, (uint8_t *)&ctrl, sizeof(ctrl));
        bt_roleswap_info_send(ROLESWAP_MODULE_RFC, ROLESWAP_RFC_CTRL_CONN,
                              (uint8_t *)&ctrl, sizeof(ctrl));

        gap_br_get_handover_l2c_info(ctrl.l2c_cid);
    }

    bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_RFC_DATA, (uint8_t *)&data, sizeof(data));
    bt_roleswap_info_send(ROLESWAP_MODULE_RFC, ROLESWAP_RFC_DATA_CONN,
                          (uint8_t *)&data, sizeof(data));
}

bool bt_roleswap_free_info(uint8_t          bd_addr[6],
                           T_ROLESWAP_DATA *p_data)
{
    T_ROLESWAP_INFO *p_base;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return false;
    }
    if (os_queue_delete(&p_base->info_list, p_data))
    {
        os_mem_free(p_data);
    }
    else
    {
        BTM_PRINT_ERROR0("bt_roleswap_free_info: del queue fail");

        return false;
    }

    return true;
}

void bt_roleswap_free_acl_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    T_ROLESWAP_DATA *p_next;
    T_ROLESWAP_INFO *p_base;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        BTM_PRINT_ERROR1("bt_roleswap_free_acl_info: fail to find roleswap base %s",
                         TRACE_BDADDR(bd_addr));
        return;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;

    while (p_data)
    {
        p_next = p_data->p_next;

        os_queue_delete(&p_base->info_list, p_data);
        os_mem_free(p_data);

        p_data = p_next;
    }

    bt_free_roleswap_info_base(p_base);

    bt_roleswap_dump();
}

void bt_roleswap_free_l2c_info(uint8_t  bd_addr[6],
                               uint16_t cid)
{
    T_ROLESWAP_DATA *p_data;

    p_data = bt_find_roleswap_l2c(bd_addr, cid);
    if (p_data == NULL)
    {
        return;
    }

    bt_roleswap_free_info(bd_addr, p_data);
}

void bt_check_free_roleswap_rfc_ctrl(uint8_t  bd_addr[6],
                                     uint16_t cid)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        if (p_data->type == ROLESWAP_TYPE_RFC_DATA && p_data->u.rfc_data.l2c_cid == cid)
        {
            return;
        }

        p_data = p_data->p_next;
    }

    //no rfcomm data channel on this l2cap channel, free rfc ctrl and l2c
    p_data = bt_find_roleswap_rfc_ctrl(bd_addr, cid);
    if (p_data == NULL)
    {
        return;
    }

    bt_roleswap_free_info(bd_addr, p_data);

    bt_roleswap_free_l2c_info(bd_addr, cid);
}

bool bt_roleswap_check_l2c_cid(uint8_t  bd_addr[6],
                               uint16_t cid)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return false;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;

    while (p_data)
    {
        switch (p_data->type)
        {
        case ROLESWAP_TYPE_RFC_CTRL:
            if (p_data->u.rfc_ctrl.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_A2DP:
            if (p_data->u.a2dp.sig_cid == cid || p_data->u.a2dp.stream_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_AVRCP:
            if (p_data->u.avrcp.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_HFP:
            if (p_data->u.hfp.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_SPP:
            if (p_data->u.spp.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_PBAP:
            if (p_data->u.pbap.l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_HID_DEVICE:
            if (p_data->u.hid_device.control_cid == cid || p_data->u.hid_device.interrupt_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_HID_HOST:
            if (p_data->u.hid_host.control_cid == cid || p_data->u.hid_host.interrupt_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_AVP:
            if (p_data->u.avp.control_l2c_cid == cid || p_data->u.avp.audio_l2c_cid == cid)
            {
                return true;
            }
            break;

        case ROLESWAP_TYPE_ATT:
            if (p_data->u.att.l2c_cid == cid)
            {
                return true;
            }
            break;

        default:
            break;
        }

        p_data = p_data->p_next;
    }

    return false;
}

void bt_roleswap_free_rfc_info(uint8_t bd_addr[6],
                               uint8_t dlci)
{
    uint16_t cid;
    T_ROLESWAP_DATA *p_data;

    // find rfc data channel info and delete
    p_data = bt_find_roleswap_rfc_data(bd_addr, dlci);
    if (p_data == NULL)
    {
        return;
    }

    cid = p_data->u.rfc_data.l2c_cid;
    bt_roleswap_free_info(bd_addr, p_data);

    //check if rfc ctrl channel info should be delete
    bt_check_free_roleswap_rfc_ctrl(bd_addr, cid);
}

void bt_roleswap_free_spp_info(uint8_t bd_addr[6],
                               uint8_t local_server_chann)
{
    T_ROLESWAP_DATA *p_data;
    uint8_t dlci;

    p_data = bt_find_roleswap_spp(bd_addr, local_server_chann);
    if (p_data == NULL)
    {
        return;
    }

    dlci = p_data->u.spp.rfc_dlci;

    bt_roleswap_free_info(bd_addr, p_data);
    bt_roleswap_free_rfc_info(bd_addr, dlci);
}

void bt_roleswap_free_a2dp_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint16_t sig_cid;
    uint16_t stream_cid;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_A2DP);
    if (p_data == NULL)
    {
        return;
    }

    sig_cid = p_data->u.a2dp.sig_cid;
    stream_cid = p_data->u.a2dp.stream_cid;

    bt_roleswap_free_info(bd_addr, p_data);

    bt_roleswap_free_l2c_info(bd_addr, sig_cid);
    bt_roleswap_free_l2c_info(bd_addr, stream_cid);
}

void bt_roleswap_free_avrcp_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint16_t l2c_cid;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_AVRCP);
    if (p_data == NULL)
    {
        return;
    }

    l2c_cid = p_data->u.avrcp.l2c_cid;

    bt_roleswap_free_info(bd_addr, p_data);

    bt_roleswap_free_l2c_info(bd_addr, l2c_cid);
}

void bt_roleswap_free_hfp_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint8_t dlci;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_HFP);
    if (p_data == NULL)
    {
        return;
    }

    dlci = p_data->u.hfp.rfc_dlci;

    bt_roleswap_free_info(bd_addr, p_data);

    bt_roleswap_free_rfc_info(bd_addr, dlci);
}

void bt_roleswap_free_pbap_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint16_t cid;
    uint8_t dlci;
    bool obex_over_l2c;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_PBAP);
    if (p_data == NULL)
    {
        return;
    }

    cid = p_data->u.pbap.l2c_cid;
    dlci = p_data->u.pbap.rfc_dlci;
    obex_over_l2c = p_data->u.pbap.obex_psm ? true : false;

    bt_roleswap_free_info(bd_addr, p_data);

    if (obex_over_l2c)
    {
        bt_roleswap_free_l2c_info(bd_addr, cid);
    }
    else
    {
        bt_roleswap_free_rfc_info(bd_addr, dlci);
    }
}

void bt_roleswap_free_hid_device_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint16_t control_cid;
    uint16_t interrupt_cid;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_HID_DEVICE);
    if (p_data == NULL)
    {
        return;
    }

    control_cid = p_data->u.hid_device.control_cid;
    interrupt_cid = p_data->u.hid_device.interrupt_cid;

    bt_roleswap_free_info(bd_addr, p_data);

    bt_roleswap_free_l2c_info(bd_addr, control_cid);
    bt_roleswap_free_l2c_info(bd_addr, interrupt_cid);
}

void bt_roleswap_free_hid_host_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint16_t control_cid;
    uint16_t interrupt_cid;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_HID_HOST);
    if (p_data == NULL)
    {
        return;
    }

    control_cid = p_data->u.hid_host.control_cid;
    interrupt_cid = p_data->u.hid_host.interrupt_cid;

    bt_roleswap_free_info(bd_addr, p_data);

    bt_roleswap_free_l2c_info(bd_addr, control_cid);
    bt_roleswap_free_l2c_info(bd_addr, interrupt_cid);
}

void bt_roleswap_free_att_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint16_t  cid;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ATT);
    if (p_data == NULL)
    {
        return;
    }

    cid = p_data->u.att.l2c_cid;

    bt_roleswap_free_info(bd_addr, p_data);

    bt_roleswap_free_l2c_info(bd_addr, cid);
}

void bt_roleswap_free_iap_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint8_t dlci;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_IAP);
    if (p_data == NULL)
    {
        return;
    }

    dlci = p_data->u.iap.dlci;

    bt_roleswap_free_info(bd_addr, p_data);
    bt_roleswap_free_rfc_info(bd_addr, dlci);
}

void bt_roleswap_free_avp_control_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint16_t cid;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_AVP);
    if (p_data == NULL)
    {
        return;
    }

    cid = p_data->u.avp.control_l2c_cid;
    bt_roleswap_free_l2c_info(bd_addr, cid);

    if (p_data->u.avp.audio_l2c_cid)
    {
        p_data->u.avp.control_l2c_cid = 0;
        p_data->u.avp.control_state = 0;
        p_data->u.avp.control_data_offset = 0;
    }
    else
    {
        bt_roleswap_free_info(bd_addr, p_data);
    }
}

void bt_roleswap_free_avp_audio_info(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;
    uint16_t cid;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_AVP);
    if (p_data == NULL)
    {
        return;
    }

    cid = p_data->u.avp.audio_l2c_cid;
    bt_roleswap_free_l2c_info(bd_addr, cid);

    if (p_data->u.avp.control_l2c_cid)
    {
        p_data->u.avp.audio_l2c_cid = 0;
        p_data->u.avp.audio_state = 0;
        p_data->u.avp.audio_data_offset = 0;
    }
    else
    {
        bt_roleswap_free_info(bd_addr, p_data);
    }
}

void bt_roleswap_free_bt_rfc_info(uint8_t bd_addr[6],
                                  uint8_t local_server_chann)
{
    uint8_t dlci;
    T_ROLESWAP_DATA *p_data;

    p_data = bt_find_roleswap_bt_rfc(bd_addr, local_server_chann);
    if (p_data == NULL)
    {
        return;
    }

    dlci = p_data->u.bt_rfc.dlci;

    bt_roleswap_free_info(bd_addr, p_data);
    bt_roleswap_free_rfc_info(bd_addr, dlci);
}

void bt_roleswap_transfer(uint8_t bd_addr[6])
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;
    uint8_t         *p_buf;
    uint8_t         *p;
    uint16_t         len = 0;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        switch (p_data->type)
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

        p_data = p_data->p_next;
    }

    if (len != 0)
    {
        p_buf = os_mem_zalloc2(len);
        if (p_buf == NULL)
        {
            return;
        }

        p = p_buf;
        p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
        while (p_data)
        {
            switch (p_data->type)
            {
            case ROLESWAP_TYPE_ACL:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_ACL);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_ACL_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_ACL_INFO));
                    ARRAY_TO_STREAM(p, &p_data->u.acl, sizeof(T_ROLESWAP_ACL_INFO));

                    memcpy(bd_addr, &p_data->u.acl.bd_addr, 6);
                }
                break;

            case ROLESWAP_TYPE_SCO:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_SCO);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_SCO_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_SCO_INFO));
                    ARRAY_TO_STREAM(p, &p_data->u.sco, sizeof(T_ROLESWAP_SCO_INFO));
                }
                break;

            case ROLESWAP_TYPE_SM:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_SM);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_SM_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_SM_INFO));
                    ARRAY_TO_STREAM(p, &p_data->u.sm, sizeof(T_ROLESWAP_SM_INFO));
                }
                break;

            case ROLESWAP_TYPE_L2C:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_L2C);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_L2C_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_L2C_INFO));
                    ARRAY_TO_STREAM(p, &p_data->u.l2c, sizeof(T_ROLESWAP_L2C_INFO));
                }
                break;

            case ROLESWAP_TYPE_RFC_CTRL:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_RFC);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_RFC_CTRL_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_RFC_CTRL_INFO));
                    ARRAY_TO_STREAM(p, &p_data->u.rfc_ctrl, sizeof(T_ROLESWAP_RFC_CTRL_INFO));
                }
                break;

            case ROLESWAP_TYPE_RFC_DATA:
                {
                    if (bt_roleswap_get_rfc_info(p_data->u.rfc_data.bd_addr, p_data->u.rfc_data.dlci,
                                                 &p_data->u.rfc_data, NULL))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_RFC);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_RFC_DATA_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_RFC_DATA_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.rfc_data, sizeof(T_ROLESWAP_RFC_DATA_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_A2DP:
                {
                    if (bt_roleswap_get_a2dp_info(p_data->u.a2dp.bd_addr, &p_data->u.a2dp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_A2DP);
                        if (p_data->u.a2dp.stream_cid == 0)
                        {
                            LE_UINT8_TO_STREAM(p, ROLESWAP_A2DP_CONN);
                        }
                        else
                        {
                            LE_UINT8_TO_STREAM(p, ROLESWAP_A2DP_STREAM_CONN);
                        }
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_A2DP_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.a2dp, sizeof(T_ROLESWAP_A2DP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_AVRCP:
                {
                    if (bt_roleswap_get_avrcp_info(p_data->u.avrcp.bd_addr, &p_data->u.avrcp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_AVRCP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_AVRCP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_AVRCP_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.avrcp, sizeof(T_ROLESWAP_AVRCP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_HFP:
                {
                    if (bt_roleswap_get_hfp_info(p_data->u.hfp.bd_addr, &p_data->u.hfp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HFP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_HFP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HFP_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.hfp, sizeof(T_ROLESWAP_HFP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_SPP:
                {
                    if (bt_roleswap_get_spp_info(p_data->u.spp.bd_addr, p_data->u.spp.local_server_chann,
                                                 &p_data->u.spp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_SPP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_SPP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_SPP_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.spp, sizeof(T_ROLESWAP_SPP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_PBAP:
                {
                    if (bt_roleswap_get_pbap_info(p_data->u.pbap.bd_addr, &p_data->u.pbap))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_PBAP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_PBAP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_PBAP_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.pbap, sizeof(T_ROLESWAP_PBAP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_HID_DEVICE:
                {
                    if (bt_roleswap_get_hid_device_info(p_data->u.hid_device.bd_addr, &p_data->u.hid_device))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HID_DEVICE);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_HID_DEVICE_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HID_DEVICE_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.hid_device, sizeof(T_ROLESWAP_HID_DEVICE_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_HID_HOST:
                {
                    if (bt_roleswap_get_hid_host_info(p_data->u.hid_host.bd_addr, &p_data->u.hid_host))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HID_HOST);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_HID_HOST_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HID_HOST_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.hid_host, sizeof(T_ROLESWAP_HID_HOST_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_IAP:
                {
                    if (bt_roleswap_get_iap_info(p_data->u.iap.bd_addr, &p_data->u.iap))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_IAP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_IAP_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_IAP_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.iap, sizeof(T_ROLESWAP_IAP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_AVP:
                {
                    if (bt_roleswap_get_avp_info(p_data->u.avp.bd_addr, &p_data->u.avp))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_AVP);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_AVP_CONTROL_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_AVP_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.avp, sizeof(T_ROLESWAP_AVP_INFO));
                    }
                }
                break;

            case ROLESWAP_TYPE_BT_RFC:
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_BT_RFC);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_BT_RFC_CONN);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_BT_RFC_INFO));
                    ARRAY_TO_STREAM(p, &p_data->u.bt_rfc, sizeof(T_ROLESWAP_BT_RFC_INFO));
                }
                break;

            case ROLESWAP_TYPE_ATT:
                {
                    if (bt_roleswap_get_att_info(p_data->u.att.bd_addr, &p_data->u.att))
                    {
                        LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_ATT);
                        LE_UINT8_TO_STREAM(p, ROLESWAP_ATT_CONN);
                        LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_ATT_INFO));
                        ARRAY_TO_STREAM(p, &p_data->u.att, sizeof(T_ROLESWAP_ATT_INFO));
                    }
                }
                break;

            default:
                break;
            }

            p_data = p_data->p_next;
        }

        remote_async_msg_relay(btm_db.relay_handle,
                               BT_REMOTE_MSG_ROLESWAP_INFO_XMIT,
                               p_buf,
                               p - p_buf,
                               false);

        os_mem_free(p_buf);
    }
}

void bt_roleswap_sync(uint8_t bd_addr[6])
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;
    uint8_t *p_buf;
    uint8_t *p;
    uint16_t len = 0;

    p_base = bt_find_roleswap_info_base(bd_addr);
    if (p_base == NULL)
    {
        return;
    }

    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        switch (p_data->type)
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

        p_data = p_data->p_next;
    }

    if (len == 0)
    {
        return;
    }

    p_buf = os_mem_zalloc2(len);
    if (p_buf == NULL)
    {
        return;
    }

    p = p_buf;
    p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;
    while (p_data)
    {
        switch (p_data->type)
        {
        case ROLESWAP_TYPE_ACL:
            {
                T_ROLESWAP_ACL_TRANSACT acl_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_ACL);
                LE_UINT8_TO_STREAM(p, ROLESWAP_ACL_UPDATE);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_ACL_TRANSACT));

                memcpy(acl_transact.bd_addr, p_data->u.acl.bd_addr, 6);
                acl_transact.role = p_data->u.acl.role;
                acl_transact.mode = p_data->u.acl.mode;
                acl_transact.link_policy = p_data->u.acl.link_policy;
                acl_transact.superv_tout = p_data->u.acl.superv_tout;
                acl_transact.authen_state = p_data->u.acl.authen_state;
                acl_transact.encrypt_state = p_data->u.acl.encrypt_state;

                ARRAY_TO_STREAM(p, &acl_transact, sizeof(T_ROLESWAP_ACL_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_RFC_DATA:
            {
                T_ROLESWAP_RFC_TRANSACT rfc_transact;

                if (bt_roleswap_get_rfc_info(p_data->u.rfc_data.bd_addr, p_data->u.rfc_data.dlci,
                                             &p_data->u.rfc_data, NULL))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_RFC);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_RFC_DATA_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_RFC_TRANSACT));

                    memcpy(rfc_transact.bd_addr, p_data->u.rfc_data.bd_addr, 6);
                    rfc_transact.dlci = p_data->u.rfc_data.dlci;
                    rfc_transact.remote_remain_credits = p_data->u.rfc_data.remote_remain_credits;
                    rfc_transact.given_credits = p_data->u.rfc_data.given_credits;

                    ARRAY_TO_STREAM(p, &rfc_transact, sizeof(T_ROLESWAP_RFC_TRANSACT));
                }
            }
            break;

        case ROLESWAP_TYPE_SPP:
            {
                T_ROLESWAP_SPP_TRANSACT spp_transact;

                if (bt_roleswap_get_spp_info(bd_addr, p_data->u.spp.local_server_chann, &p_data->u.spp))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_SPP);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_SPP_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_SPP_TRANSACT));

                    memcpy(spp_transact.bd_addr, p_data->u.spp.bd_addr, 6);
                    spp_transact.local_server_chann = p_data->u.spp.local_server_chann;
                    spp_transact.state = p_data->u.spp.state;
                    spp_transact.remote_credit = p_data->u.spp.remote_credit;

                    ARRAY_TO_STREAM(p, &spp_transact, sizeof(T_ROLESWAP_SPP_TRANSACT));
                }
            }
            break;

        case ROLESWAP_TYPE_A2DP:
            {
                T_ROLESWAP_A2DP_TRANSACT a2dp_transact;
                uint8_t *buf;
                if (bt_roleswap_get_a2dp_info(bd_addr, &p_data->u.a2dp))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_A2DP);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_A2DP_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_A2DP_TRANSACT));

                    memcpy(a2dp_transact.bd_addr, p_data->u.a2dp.bd_addr, 6);
                    a2dp_transact.sig_state = p_data->u.a2dp.sig_state;
                    a2dp_transact.state = p_data->u.a2dp.state;
                    a2dp_transact.tx_trans_label = p_data->u.a2dp.tx_trans_label;
                    a2dp_transact.rx_start_trans_label = p_data->u.a2dp.rx_start_trans_label;
                    a2dp_transact.last_seq_number = p_data->u.a2dp.last_seq_number;
                    a2dp_transact.cmd_flag  = p_data->u.a2dp.cmd_flag;
                    a2dp_transact.codec_type = p_data->u.a2dp.codec_type;
                    buf = (uint8_t *)&a2dp_transact.codec_info;
                    ARRAY_TO_STREAM(buf, &p_data->u.a2dp.codec_info, sizeof(T_A2DP_CODEC_INFO));

                    ARRAY_TO_STREAM(p, &a2dp_transact, sizeof(T_ROLESWAP_A2DP_TRANSACT));
                }
            }
            break;

        case ROLESWAP_TYPE_AVRCP:
            {
                T_ROLESWAP_AVRCP_TRANSACT avrcp_transact;

                if (bt_roleswap_get_avrcp_info(bd_addr, &p_data->u.avrcp))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_AVRCP);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_AVRCP_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_AVRCP_TRANSACT));

                    memcpy(avrcp_transact.bd_addr, p_data->u.avrcp.bd_addr, 6);
                    avrcp_transact.avctp_state = p_data->u.avrcp.avctp_state;
                    avrcp_transact.state = p_data->u.avrcp.state;
                    avrcp_transact.play_status = p_data->u.avrcp.play_status;
                    avrcp_transact.cmd_credits = p_data->u.avrcp.cmd_credits;
                    avrcp_transact.transact_label = p_data->u.avrcp.transact_label;
                    avrcp_transact.vol_change_registered = p_data->u.avrcp.vol_change_registered;
                    avrcp_transact.vol_change_pending_transact = p_data->u.avrcp.vol_change_pending_transact;

                    ARRAY_TO_STREAM(p, &avrcp_transact, sizeof(T_ROLESWAP_AVRCP_TRANSACT));
                }
            }
            break;

        case ROLESWAP_TYPE_HFP:
            if (bt_roleswap_get_hfp_info(bd_addr, &p_data->u.hfp))
            {
                T_ROLESWAP_HFP_TRANSACT hfp_transact;

                LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HFP);
                LE_UINT8_TO_STREAM(p, ROLESWAP_HFP_TRANSACT);
                LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HFP_TRANSACT));

                memcpy(hfp_transact.bd_addr, p_data->u.hfp.bd_addr, 6);
                hfp_transact.state = p_data->u.hfp.state;
                hfp_transact.bat_report_type = p_data->u.hfp.bat_report_type;
                hfp_transact.at_cmd_credits = p_data->u.hfp.at_cmd_credits;
                hfp_transact.call_status = p_data->u.hfp.call_status;
                hfp_transact.prev_call_status = p_data->u.hfp.prev_call_status;
                hfp_transact.service_status = p_data->u.hfp.service_status;
                hfp_transact.supported_features = p_data->u.hfp.supported_features;
                hfp_transact.codec_type = p_data->u.hfp.codec_type;

                ARRAY_TO_STREAM(p, &hfp_transact, sizeof(T_ROLESWAP_HFP_TRANSACT));
            }
            break;

        case ROLESWAP_TYPE_PBAP:
            {
                T_ROLESWAP_PBAP_TRANSACT pbap_transact;

                if (bt_roleswap_get_pbap_info(bd_addr, &p_data->u.pbap))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_PBAP);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_PBAP_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_PBAP_TRANSACT));

                    memcpy(pbap_transact.bd_addr, p_data->u.pbap.bd_addr, 6);
                    pbap_transact.obex_state = p_data->u.pbap.obex_state;
                    pbap_transact.pbap_state = p_data->u.pbap.pbap_state;
                    pbap_transact.path = p_data->u.pbap.path;
                    pbap_transact.repos = p_data->u.pbap.repos;

                    ARRAY_TO_STREAM(p, &pbap_transact, sizeof(T_ROLESWAP_PBAP_TRANSACT));
                }
            }
            break;

        case ROLESWAP_TYPE_HID_DEVICE:
            {
                T_ROLESWAP_HID_DEVICE_TRANSACT hid_transact;

                if (bt_roleswap_get_hid_device_info(bd_addr, &p_data->u.hid_device))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HID_DEVICE);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_HID_DEVICE_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HID_DEVICE_TRANSACT));

                    memcpy(hid_transact.bd_addr, p_data->u.hid_device.bd_addr, 6);
                    hid_transact.proto_mode = p_data->u.hid_device.proto_mode;
                    hid_transact.control_state = p_data->u.hid_device.control_state;
                    hid_transact.interrupt_state = p_data->u.hid_device.interrupt_state;

                    ARRAY_TO_STREAM(p, &hid_transact, sizeof(T_ROLESWAP_HID_DEVICE_TRANSACT));
                }
            }
            break;

        case ROLESWAP_TYPE_HID_HOST:
            {
                T_ROLESWAP_HID_HOST_TRANSACT hid_host_transact;

                if (bt_roleswap_get_hid_host_info(bd_addr, &p_data->u.hid_host))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_HID_HOST);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_HID_HOST_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_HID_HOST_TRANSACT));

                    memcpy(hid_host_transact.bd_addr, p_data->u.hid_host.bd_addr, 6);
                    hid_host_transact.proto_mode = p_data->u.hid_host.proto_mode;
                    hid_host_transact.control_state = p_data->u.hid_host.control_state;
                    hid_host_transact.interrupt_state = p_data->u.hid_host.interrupt_state;

                    ARRAY_TO_STREAM(p, &hid_host_transact, sizeof(T_ROLESWAP_HID_HOST_TRANSACT));
                }
            }
            break;

        case ROLESWAP_TYPE_IAP:
            {
                T_ROLESWAP_IAP_TRANSACT iap_transact;

                if (bt_roleswap_get_iap_info(bd_addr, &p_data->u.iap))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_IAP);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_IAP_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_IAP_TRANSACT));

                    memcpy(iap_transact.bd_addr, bd_addr, 6);
                    iap_transact.remote_credit = p_data->u.iap.remote_credit;
                    iap_transact.state = p_data->u.iap.state;
                    iap_transact.acc_pkt_seq = p_data->u.iap.acc_pkt_seq;
                    iap_transact.acked_seq = p_data->u.iap.acked_seq;
                    iap_transact.dev_pkt_seq = p_data->u.iap.dev_pkt_seq;

                    ARRAY_TO_STREAM(p, &iap_transact, sizeof(T_ROLESWAP_IAP_TRANSACT));
                }
            }
            break;

        case ROLESWAP_TYPE_AVP:
            {
                T_ROLESWAP_AVP_TRANSACT avp_transact;

                if (bt_roleswap_get_avp_info(bd_addr, &p_data->u.avp))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_AVP);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_AVP_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_AVP_TRANSACT));

                    memcpy(avp_transact.bd_addr, p_data->u.avp.bd_addr, 6);
                    avp_transact.control_state = p_data->u.avp.control_state;
                    avp_transact.audio_state = p_data->u.avp.audio_state;

                    ARRAY_TO_STREAM(p, &avp_transact, sizeof(T_ROLESWAP_AVP_TRANSACT));
                }
            }
            break;

        case ROLESWAP_TYPE_ATT:
            {
                T_ROLESWAP_ATT_TRANSACT att_transact;

                if (bt_roleswap_get_att_info(bd_addr, &p_data->u.att))
                {
                    LE_UINT8_TO_STREAM(p, ROLESWAP_MODULE_ATT);
                    LE_UINT8_TO_STREAM(p, ROLESWAP_ATT_TRANSACT);
                    LE_UINT16_TO_STREAM(p, sizeof(T_ROLESWAP_ATT_TRANSACT));

                    memcpy(att_transact.bd_addr, p_data->u.att.bd_addr, 6);
                    att_transact.state = p_data->u.att.state;

                    ARRAY_TO_STREAM(p, &att_transact, sizeof(T_ROLESWAP_ATT_TRANSACT));
                }
            }
            break;

        default:
            break;
        }

        p_data = p_data->p_next;
    }

    remote_async_msg_relay(btm_db.relay_handle,
                           BT_REMOTE_MSG_ROLESWAP_INFO_XMIT,
                           p_buf,
                           p - p_buf,
                           false);

    os_mem_free(p_buf);
}

void bt_roleswap_handle_link_policy(uint8_t bd_addr[6])
{
    T_ROLESWAP_DATA *p_data;

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
    if (p_data != NULL)
    {
        uint16_t link_policy;

        if (bt_link_policy_get(bd_addr, &link_policy) == true)
        {
            p_data->u.acl.link_policy = link_policy;
            BTM_PRINT_TRACE1("bt_roleswap_handle_link_policy: link_policy %u", p_data->u.acl.link_policy);
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
                                        T_ROLESWAP_PROFILE_DISCONN_PARAM *p_param)
{
    bt_roleswap_proto->profile_disconn(bd_addr, p_param);
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

void bt_roleswap_handle_bt_avp_control_conn(uint8_t bd_addr[6])
{
    bt_roleswap_proto->bt_avp_control_conn(bd_addr);
}

void bt_roleswap_handle_bt_avp_control_disconn(uint8_t  bd_addr[6],
                                               uint16_t cause)
{
    bt_roleswap_proto->bt_avp_control_disconn(bd_addr, cause);
}

void bt_roleswap_handle_bt_avp_audio_conn(uint8_t bd_addr[6])
{
    bt_roleswap_proto->bt_avp_audio_conn(bd_addr);
}

void bt_roleswap_handle_bt_avp_audio_disconn(uint8_t bd_addr[6], uint16_t cause)
{
    bt_roleswap_proto->bt_avp_audio_disconn(bd_addr, cause);
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

void bt_roleswap_recv(uint8_t  *p_data,
                      uint16_t  data_len)
{
    bt_roleswap_proto->recv(p_data, data_len);
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
    uint8_t i;

    bt_roleswap_proto = &bt_sniffing_proto;

    btm_db.roleswap_info = os_mem_zalloc2(sizeof(T_ROLESWAP_INFO) * btm_db.br_link_num);
    if (btm_db.roleswap_info == NULL)
    {
        return false;
    }

    for (i = 0; i < btm_db.br_link_num; i++)
    {
        os_queue_init(&btm_db.roleswap_info[i].info_list);
    }

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
                                        T_ROLESWAP_PROFILE_DISCONN_PARAM *p_param)
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

void bt_roleswap_handle_bt_avp_control_conn(uint8_t bd_addr[6])
{

}

void bt_roleswap_handle_bt_avp_control_disconn(uint8_t  bd_addr[6],
                                               uint16_t cause)
{

}

void bt_roleswap_handle_bt_avp_audio_conn(uint8_t bd_addr[6])
{

}

void bt_roleswap_handle_bt_avp_audio_disconn(uint8_t  bd_addr[6],
                                             uint16_t cause)
{

}

void bt_roleswap_handle_ctrl_conn(void)
{

}

void bt_roleswap_recv(uint8_t  *p_data,
                      uint16_t  data_len)
{

}

T_BT_CLK_REF bt_roleswap_get_piconet_clk(T_BT_CLK_REF  clk,
                                         uint32_t     *bb_clock_timer,
                                         uint16_t     *bb_clock_us)
{
    if (clk == BT_CLK_SNIFFING)
    {
        T_BT_BR_LINK *p_link_active;

        p_link_active = bt_find_br_link(btm_db.br_link[btm_db.active_a2dp_index].bd_addr);
        if (p_link_active == NULL)
        {
            BTM_PRINT_TRACE0("bt_roleswap_get_piconet_clk: no sniff handle");
            return BT_CLK_NONE;
        }
        else
        {
            rws_read_bt_piconet_clk(p_link_active->acl_handle, bb_clock_timer, bb_clock_us);
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
        T_BT_BR_LINK *p_link_active;

        p_link_active = bt_find_br_link(btm_db.br_link[btm_db.active_a2dp_index].bd_addr);
        if (p_link_active == NULL)
        {
            BTM_PRINT_TRACE0("bt_roleswap_get_piconet_id: no sniff handle");
            return false;
        }
        else
        {
            get_ret = bt_clk_index_read(p_link_active->acl_handle, clk_index, role);
            BTM_PRINT_TRACE4("bt_roleswap_get_piconet_id: handle 0x%x clock_id 0x%x, ref_type 0x%x ,get_ret %d",
                             p_link_active->acl_handle, *clk_index, clk_ref, get_ret);
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
