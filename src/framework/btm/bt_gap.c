/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "os_mem.h"
#include <stdlib.h>
#include "trace.h"
#include "bt_types.h"
#include "gap.h"
#include "gap_br.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

#define HCI_VND_MONITOR_REPORT_EVENT     0x3B
#define SCO_PACKET_LEN              60

static bool bt_sco_msbc_header_check(uint8_t *buf,
                                     uint8_t  buf_len)
{
    bool ret = false;

    switch (buf_len)
    {
    case 0:
        break;

    case 1:
        if (buf[0] == 0x01)
        {
            ret = true;
        }
        break;

    case 2:
        if (buf[0] == 0x01 &&
            (buf[1] == 0x08 || buf[1] == 0x38 || buf[1] == 0xc8 || buf[1] == 0xf8))
        {
            ret = true;
        }
        break;

    default:
        if (buf[0] == 0x01 &&
            (buf[1] == 0x08 || buf[1] == 0x38 || buf[1] == 0xc8 || buf[1] == 0xf8) &&
            buf[2] == 0xad)
        {
            ret = true;
        }
        break;
    }

    return ret;
}

void bt_handle_sdp_discov_info(uint8_t             *attr,
                               uint16_t             attr_len,
                               T_BT_SDP_ATTR_INFO  *attr_info)
{
    uint8_t *attr_start = attr;
    uint8_t *attr_end = attr_start + attr_len;
    uint8_t *p_attr;
    uint8_t *attr_param;
    uint8_t *elem;
    uint16_t loop;

    attr_info->p_attr = attr;
    attr_info->attr_len = attr_len;

    p_attr = gap_br_find_sdp_attr(attr_start, attr_end, SDP_ATTR_SRV_CLASS_ID_LIST);
    loop = 1;
    while ((elem = gap_br_access_sdp_elem(p_attr, attr_end, loop)) != NULL)
    {
        if (gap_br_get_sdp_uuid_value(elem,
                                      attr_end,
                                      (T_GAP_UUID_TYPE *)(&attr_info->srv_class_uuid_type),
                                      (T_GAP_UUID_DATA *)(&attr_info->srv_class_uuid_data)))
        {
            break;
        }
        else
        {
            loop++;
        }
    }

    p_attr = gap_br_find_sdp_attr(attr_start, attr_end, SDP_ATTR_PROFILE_DESC_LIST);
    if (p_attr)
    {
        loop = 1;
        while ((elem = gap_br_access_sdp_elem(p_attr, attr_end, loop)) != NULL)
        {
            attr_param = gap_br_access_sdp_elem(elem, attr_end, 2);

            if (attr_param)
            {
                attr_info->profile_version = (uint16_t)gap_br_get_sdp_value(attr_param, attr_end);
                break;
            }
            loop++;
        }
    }

    p_attr = gap_br_find_sdp_attr(attr_start, attr_end, SDP_ATTR_PROTO_DESC_LIST);
    loop = 1;
    while ((elem = gap_br_access_sdp_elem(p_attr, attr_end, loop)) != NULL)
    {
        attr_param = gap_br_access_sdp_elem(elem, attr_end, 2);

        if (attr_param)
        {
            switch ((uint16_t)gap_br_get_sdp_value(gap_br_access_sdp_elem(elem, attr_end, 1), attr_end))
            {
            case UUID_RFCOMM:
                attr_info->server_channel = (uint8_t)gap_br_get_sdp_value(attr_param, attr_end);
                break;

            default:
                attr_info->protocol_version = (uint16_t)gap_br_get_sdp_value(attr_param, attr_end);
                break;
            }
        }
        loop++;
    }

    p_attr = gap_br_find_sdp_attr(attr_start, attr_end, SDP_ATTR_SUPPORTED_FEATURES);
    if (p_attr)
    {
        attr_info->supported_feat = (uint16_t)gap_br_get_sdp_value(p_attr, attr_end);
    }

    /* get l2cap psm */
    p_attr = gap_br_find_sdp_attr(attr_start, attr_end, SDP_ATTR_L2C_PSM);
    if (p_attr)
    {
        attr_info->l2c_psm = (uint16_t)gap_br_get_sdp_value(p_attr, attr_end);
    }

    /* get supported repositories */
    p_attr = gap_br_find_sdp_attr(attr_start, attr_end, SDP_ATTR_SUPPORTED_REPOSITORIES);
    if (p_attr)
    {
        attr_info->supported_repos = (uint8_t)gap_br_get_sdp_value(p_attr, attr_end);
    }

    /* for pbap, supported features is 0x0317 and size is uint32_t while 10 bits are used now */
    p_attr = gap_br_find_sdp_attr(attr_start, attr_end, SDP_ATTR_PBAP_SUPPORTED_FEAT);
    if (p_attr)
    {
        attr_info->pbap_supported_feat = gap_br_get_sdp_value(p_attr, attr_end);
    }
    else
    {
        attr_info->pbap_supported_feat = 0x00000003;
    }
}

void bt_handle_acl_status_info(T_GAP_ACL_STATUS_INFO *info)
{
    T_BT_LINK        *link;
    T_BT_MSG_PAYLOAD  payload;

    link = bt_link_find(info->bd_addr);

    BTM_PRINT_INFO3("bt_handle_acl_status_info: bd_addr %s, link %p, status 0x%02x",
                    TRACE_BDADDR(info->bd_addr), link, info->status);

    memcpy(payload.bd_addr, info->bd_addr, 6);
    payload.msg_buf = NULL;

    switch (info->status)
    {
    case GAP_ACL_CONN_ACTIVE:
        if (link != NULL)
        {
            bt_pm_sm(link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_SUCCESS);

            if (link->role_switch_status == BT_ROLE_SWITCH_MASTER_PENDING)
            {
                bt_link_role_switch(link->bd_addr, true);
            }
            else if (link->role_switch_status == BT_ROLE_SWITCH_SLAVE_PENDING)
            {
                bt_link_role_switch(link->bd_addr, false);
            }

            bt_mgr_dispatch(BT_MSG_ACL_CONN_ACTIVE, &payload);
        }
        break;

    case GAP_ACL_CONN_SNIFF:
        if (link != NULL)
        {
            uint16_t interval = info->p.sniff.interval;

            bt_pm_sm(link->bd_addr, BT_PM_EVENT_SNIFF_ENTER_SUCCESS);

            if (link->role_switch_status == BT_ROLE_SWITCH_MASTER_PENDING)
            {
                bt_link_role_switch(link->bd_addr, true);
            }
            else if (link->role_switch_status == BT_ROLE_SWITCH_SLAVE_PENDING)
            {
                bt_link_role_switch(link->bd_addr, false);
            }

            payload.msg_buf = &interval;
            bt_mgr_dispatch(BT_MSG_ACL_CONN_SNIFF, &payload);
        }
        break;

    case GAP_ACL_CONN_ENCRYPTED:
        if (link != NULL)
        {
            link->acl_link_encrypted = true;

            if (link->role_switch_status == BT_ROLE_SWITCH_MASTER_PENDING)
            {
                bt_link_role_switch(link->bd_addr, true);
            }
            else if (link->role_switch_status == BT_ROLE_SWITCH_SLAVE_PENDING)
            {
                bt_link_role_switch(link->bd_addr, false);
            }

            bt_mgr_dispatch(BT_MSG_ACL_CONN_ENCRYPTED, &payload);
        }
        break;

    case GAP_ACL_CONN_NOT_ENCRYPTED:
        if (link != NULL)
        {
            uint16_t cause = info->p.authen.cause;

            link->acl_link_encrypted = false;

            payload.msg_buf = &cause;
            bt_mgr_dispatch(BT_MSG_ACL_CONN_NOT_ENCRYPTED, &payload);
        }
        break;

    case GAP_ACL_CONN_BR_SC_START:
        if (link != NULL)
        {
            link->acl_link_sc_ongoing = true;
            bt_mgr_dispatch(BT_MSG_ACL_CONN_BR_SC_START, &payload);
        }
        break;

    case GAP_ACL_CONN_BR_SC_CMPL:
        if (link != NULL)
        {
            uint16_t cause = info->p.br_sc_cmpl.cause;

            link->acl_link_sc_ongoing = false;

            if (link->role_switch_status == BT_ROLE_SWITCH_MASTER_PENDING)
            {
                bt_link_role_switch(link->bd_addr, true);
            }
            else if (link->role_switch_status == BT_ROLE_SWITCH_SLAVE_PENDING)
            {
                bt_link_role_switch(link->bd_addr, false);
            }

            payload.msg_buf = &cause;
            bt_mgr_dispatch(BT_MSG_ACL_CONN_BR_SC_CMPL, &payload);
        }
        break;

    case GAP_ACL_CONN_SUCCESS:
        {
            uint16_t interval;
            uint32_t timeout;
            uint8_t peer_addr[6] = {0};

            if (link == NULL)
            {
                link = bt_link_alloc(info->bd_addr);
            }

            if (link != NULL)
            {
                BTM_PRINT_INFO2("acl (%x) link connect success, handle 0x%x",
                                link->index, info->p.conn_success.handle);

                uint16_t handle = info->p.conn_success.handle;
                link->acl_link_state = BT_LINK_STATE_CONNECTED;
                link->acl_handle = handle;

                payload.msg_buf = &handle;

                remote_peer_addr_get(peer_addr);

                if (memcmp(info->bd_addr, peer_addr, 6) == 0)
                {
                    interval = BT_SNIFF_INTERVAL_REMOTE;
                    timeout = BT_PM_TIMEOUT_REMOTE;
                }
                else
                {
                    interval = BT_SNIFF_INTERVAL_BASE + link->index * BT_SNIFF_INTERVAL_OFFSET;
                    timeout = BT_PM_TIMEOUT;
                }

                bt_sniff_mode_config(info->bd_addr, interval, BT_SNIFF_ATTEMPT, BT_SNIFF_TIMEOUT, timeout);
                bt_mgr_dispatch(BT_MSG_ACL_CONN_SUCCESS, &payload);
            }
        }
        break;

    case GAP_ACL_CONN_READY:
        if (link != NULL)
        {
            bt_pm_sm(link->bd_addr, BT_PM_EVENT_LINK_CONNECTED);
            bt_mgr_dispatch(BT_MSG_ACL_CONN_READY, &payload);
        }
        break;

    case GAP_ACL_CONN_FAIL:
        {
            uint16_t cause = info->p.conn_fail.cause;

            BTM_PRINT_ERROR2("acl connection fail to bd_addr [%s] with reason: 0x%02X",
                             TRACE_BDADDR(info->bd_addr), cause);

            payload.msg_buf = &cause;

            if (link != NULL)
            {
                bt_link_free(link);
            }

            /* Fixme: SDP Discover procedure shall allocate ACL link
               database, or the ACL link may not exist. */
            bt_mgr_dispatch(BT_MSG_ACL_CONN_FAIL, &payload);
        }
        break;

    case GAP_ACL_CONN_DISCONN:
        if (link != NULL)
        {
            uint16_t cause = info->p.conn_disconn.cause;

            BTM_PRINT_TRACE2("acl (%x) link disconnected, status=0x%02X",
                             link->index, info->p.conn_disconn.cause);

            if (link->sco_state == BT_LINK_SCO_STATE_CONNECTED)
            {
                payload.msg_buf = &info->p.conn_disconn.cause;
                bt_mgr_dispatch(BT_MSG_SCO_DISCONN_CMPL, &payload);
            }
            bt_link_free(link);

            payload.msg_buf = &cause;
            bt_mgr_dispatch(BT_MSG_ACL_CONN_DISCONN, &payload);
        }
        break;

    case GAP_ACL_ROLE_MASTER:
        if (link != NULL)
        {
            link->acl_link_role_master = true;
            link->role_switch_status = BT_ROLE_SWITCH_IDLE;
            bt_mgr_dispatch(BT_MSG_ACL_ROLE_MASTER, &payload);
        }
        break;

    case GAP_ACL_ROLE_SLAVE:
        if (link != NULL)
        {
            link->acl_link_role_master = false;
            link->role_switch_status = BT_ROLE_SWITCH_IDLE;
            bt_mgr_dispatch(BT_MSG_ACL_ROLE_SLAVE, &payload);
        }
        break;

    case GAP_ACL_AUTHEN_START:
        if (link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_ACL_AUTHEN_START, &payload);
        }
        break;

    case GAP_ACL_AUTHEN_FAIL:
        if (link != NULL)
        {
            uint16_t cause = info->p.authen.cause;

            link->acl_link_authenticated = false;

            payload.msg_buf = &cause;
            bt_mgr_dispatch(BT_MSG_ACL_AUTHEN_FAIL, &payload);
        }
        break;

    case GAP_ACL_AUTHEN_SUCCESS:
        if (link != NULL)
        {
            link->acl_link_authenticated = true;
            bt_mgr_dispatch(BT_MSG_ACL_AUTHEN_SUCCESS, &payload);
        }
        break;

    default:
        break;
    }
}

void bt_handle_vendor_evt_info(T_GAP_VENDOR_EVT_INFO *info)
{
    uint8_t *p;
    uint8_t  sub_evt;

    p = info->param;
    LE_STREAM_TO_UINT8(sub_evt, p);

    if (sub_evt == HCI_VND_MONITOR_REPORT_EVENT)
    {
        uint16_t   handle;
        T_BT_LINK *link;

        LE_STREAM_TO_UINT16(handle, p);

        link = bt_link_find_by_handle(handle);
        if (link != NULL)
        {
            uint8_t          type;
            T_BT_MSG         msg;
            T_BT_MSG_PAYLOAD payload;

            LE_STREAM_TO_UINT8(type, p);

            if (type == 0)  /* per */
            {
                msg = BT_MSG_VND_PER_INFO;
            }
            else if (type == 1) /* rssi */
            {
                msg = BT_MSG_VND_RSSI_INFO;
            }
            else
            {
                return;
            }

            payload.msg_buf = p;
            memcpy(payload.bd_addr, link->bd_addr, 6);
            bt_mgr_dispatch(msg, &payload);
        }
    }
}

void bt_gap_cback(void         *p_buf,
                  T_GAP_BR_MSG  br_msg)
{
    T_BT_MSG_PAYLOAD  payload;
    T_BT_LINK        *link;

    BTM_PRINT_TRACE1("bt_gap_cback: msg 0x%04x", br_msg);

    payload.msg_buf = NULL;

    switch (br_msg)
    {
    case GAP_BR_PROTO_REG_CMPL:
        {
            gap_br_set_page_with_scan(1);

            gap_get_param(GAP_PARAM_BD_ADDR, payload.bd_addr);
            bt_mgr_dispatch(BT_MSG_GAP_BT_READY, &payload);
        }
        break;

    case GAP_BR_SET_DEV_NAME_RSP:
        {
            T_GAP_DEV_CFG_RSP *p_rsp;

            p_rsp = (T_GAP_DEV_CFG_RSP *)p_buf;
            payload.msg_buf = &p_rsp->cause;
            bt_mgr_dispatch(BT_MSG_GAP_LOCAL_NAME_CHANGED, &payload);
        }
        break;

    case GAP_BR_SET_RADIO_MODE_RSP:
        {
            T_GAP_RADIO_MODE_SET_RSP *p_rsp;

            p_rsp = (T_GAP_RADIO_MODE_SET_RSP *)p_buf;
            payload.msg_buf = &p_rsp->cause;
            bt_mgr_dispatch(BT_MSG_GAP_RADIO_MODE_RSP, &payload);
        }
        break;

    case GAP_BR_ADD_SDP_RECORD_RSP:
        {
            T_GAP_ADD_SDP_RECORD_RSP *p_rsp;

            p_rsp = (T_GAP_ADD_SDP_RECORD_RSP *)p_buf;
            payload.msg_buf = p_rsp;
            bt_mgr_dispatch(BT_MSG_GAP_ADD_SDP_RECORD_RSP, &payload);
        }
        break;

    case GAP_BR_DEL_SDP_RECORD_RSP:
        {
            T_GAP_DEL_SDP_RECORD_RSP *p_rsp;

            p_rsp = (T_GAP_DEL_SDP_RECORD_RSP *)p_buf;
            payload.msg_buf = p_rsp;
            bt_mgr_dispatch(BT_MSG_GAP_DEL_SDP_RECORD_RSP, &payload);
        }
        break;

    case GAP_BR_SDP_ATTR_INFO:
        {
            T_GAP_SDP_ATTR_INFO *p;
            T_BT_SDP_ATTR_INFO   sdp_attr_info;

            p = (T_GAP_SDP_ATTR_INFO *)p_buf;
            bt_handle_sdp_discov_info(p->p_attr, p->attr_len, &sdp_attr_info);

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = &sdp_attr_info;
            bt_mgr_dispatch(BT_MSG_GAP_SDP_ATTR_INFO, &payload);
        }
        break;

    case GAP_BR_DID_ATTR_INFO:
        {
            T_GAP_DID_ATTR_INFO *p;

            p = (T_GAP_DID_ATTR_INFO *)p_buf;

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_GAP_DID_ATTR_INFO, &payload);
        }
        break;

    case GAP_BR_SDP_DISCOV_RSP://this means SDP finish
        {
            T_GAP_SDP_DISCOV_RSP *p;

            p = (T_GAP_SDP_DISCOV_RSP *)p_buf;
            if (p->cause)
            {
                memcpy(payload.bd_addr, p->bd_addr, 6);
                payload.msg_buf = &p->cause;
                bt_mgr_dispatch(BT_MSG_GAP_SDP_DISCOV_CMPL, &payload);
            }
        }
        break;

    case GAP_BR_SDP_DISCOV_CMPL://sdp channel disconnect
        {
            T_GAP_SDP_DISCOV_CMPL_INFO *p;

            p = (T_GAP_SDP_DISCOV_CMPL_INFO *)p_buf;
            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = &p->cause;
            bt_mgr_dispatch(BT_MSG_GAP_SDP_DISCOV_CMPL, &payload);
        }
        break;

    case GAP_BR_SDP_STOP_DISCOV_RSP://this means SDP cancel page
        {
            T_GAP_STOP_SDP_DISCOV_RSP *p;

            p = (T_GAP_STOP_SDP_DISCOV_RSP *)p_buf;
            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = &p->cause;
            bt_mgr_dispatch(BT_MSG_GAP_SDP_DISCOV_STOP, &payload);
        }
        break;

    case GAP_BR_SCO_CONN_IND:
        {
            T_GAP_SCO_CONN_IND *p_data;

            p_data = (T_GAP_SCO_CONN_IND *)p_buf;

            link = bt_link_find(p_data->bd_addr);
            if (link != NULL)
            {
                link->is_esco = p_data->is_esco;
                link->sco_state = BT_LINK_SCO_STATE_CONNECTING;
                memcpy(payload.bd_addr, p_data->bd_addr, 6);
                payload.msg_buf = p_data;
                bt_mgr_dispatch(BT_MSG_SCO_CONN_IND, &payload);
            }
        }
        break;

    case GAP_BR_SCO_CONN_RSP:
        {
            T_GAP_SCO_CONN_RSP *p_data;

            p_data = (T_GAP_SCO_CONN_RSP *)p_buf;

            link = bt_link_find(p_data->bd_addr);
            if (link != NULL)
            {
                link->sco_state = BT_LINK_SCO_STATE_CONNECTING;
                memcpy(payload.bd_addr, p_data->bd_addr, 6);
                payload.msg_buf = p_data;
                bt_mgr_dispatch(BT_MSG_SCO_CONN_RSP, &payload);
            }
        }
        break;

    case GAP_BR_SCO_CONN_CMPL:
        {
            T_GAP_SCO_CONN_CMPL_INFO *p_data;

            p_data = (T_GAP_SCO_CONN_CMPL_INFO *)p_buf;

            link = bt_link_find(p_data->bd_addr);
            if (link != NULL)
            {
                if (p_data->cause == 0)
                {
                    link->sco_buf = calloc(60, sizeof(uint8_t));
                    if (link->sco_buf != NULL)
                    {
                        link->sco_state = BT_LINK_SCO_STATE_CONNECTED;
                        link->is_esco = p_data->is_esco;
                        link->sco_handle = p_data->handle;
                        link->sco_air_mode = p_data->air_mode;
                        if (p_data->is_esco)
                        {
                            link->sco_packet_length = p_data->rx_length;
                        }
                        else
                        {
                            link->sco_packet_length = 30;
                        }
                    }
                }

                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = p_data;
                bt_mgr_dispatch(BT_MSG_SCO_CONN_CMPL, &payload);
            }
        }
        break;

    case GAP_BR_SCO_DATA_IND:
        {
            T_GAP_SCO_DATA_IND *p_data;

            p_data = (T_GAP_SCO_DATA_IND *)p_buf;
            link = bt_link_find(p_data->bd_addr);
            if (link != NULL)
            {
                if (link->sco_buf != NULL)
                {
                    T_BT_SCO_DATA_IND  sco_data_ind;
                    uint8_t           *sco_buf;
                    uint8_t            sco_len;

                    sco_buf = &p_data->data[p_data->gap];
                    sco_len = p_data->len;

                    memcpy(payload.bd_addr, p_data->bd_addr, 6);
                    sco_data_ind.bt_clk = p_data->bt_clock;
                    sco_data_ind.status = (uint8_t)p_data->status;
                    sco_data_ind.handle = link->sco_handle;
                    payload.msg_buf = &sco_data_ind;

                    if (link->sco_air_mode == GAP_AIR_MODE_TRANSPARENT)
                    {
                        if (p_data->status == 0)
                        {
                            uint8_t  i = 0;

                            while (i < sco_len)
                            {
                                switch (link->curr_sco_len)
                                {
                                case 0:
                                    if (bt_sco_msbc_header_check(&sco_buf[i], sco_len - i) == true)
                                    {
                                        if (sco_len - i >= 3)
                                        {
                                            memcpy(link->sco_buf, &sco_buf[i], 3);
                                            link->curr_sco_len = 3;
                                            i += 3;
                                        }
                                        else
                                        {
                                            memcpy(link->sco_buf, &sco_buf[i], sco_len - i);
                                            link->curr_sco_len = sco_len - i;
                                            i += sco_len - i;
                                        }
                                    }
                                    else
                                    {
                                        i++;
                                    }
                                    break;

                                case 1:
                                    if (sco_len - i >= 2)
                                    {
                                        link->sco_buf[1] = sco_buf[i];
                                        link->sco_buf[2] = sco_buf[i + 1];

                                        if (bt_sco_msbc_header_check(link->sco_buf, 3) == true)
                                        {
                                            link->curr_sco_len = 3;
                                            i += 2;
                                        }
                                        else
                                        {
                                            link->curr_sco_len = 0;
                                            memset(link->sco_buf, 0, 3);
                                        }
                                    }
                                    else
                                    {
                                        link->sco_buf[1] = sco_buf[i];

                                        if (bt_sco_msbc_header_check(link->sco_buf, 2) == true)
                                        {
                                            link->curr_sco_len = 2;
                                            i++;
                                        }
                                        else
                                        {
                                            link->curr_sco_len = 0;
                                            memset(link->sco_buf, 0, 2);
                                        }
                                    }
                                    break;

                                case 2:
                                    {
                                        link->sco_buf[2] = sco_buf[i];

                                        if (bt_sco_msbc_header_check(link->sco_buf, 3) == true)
                                        {
                                            link->curr_sco_len = 3;
                                            i++;
                                        }
                                        else
                                        {
                                            link->curr_sco_len = 0;
                                            memset(link->sco_buf, 0, 3);
                                        }
                                    }
                                    break;

                                default:
                                    if (SCO_PACKET_LEN - link->curr_sco_len <= sco_len - i)
                                    {
                                        memcpy(&link->sco_buf[link->curr_sco_len], &sco_buf[i],
                                               SCO_PACKET_LEN - link->curr_sco_len);
                                        sco_data_ind.length = SCO_PACKET_LEN - 3;
                                        sco_data_ind.p_data = &link->sco_buf[2];
                                        i += SCO_PACKET_LEN - link->curr_sco_len;
                                        link->curr_sco_len = 0;
                                        bt_mgr_dispatch(BT_MSG_SCO_DATA_IND, &payload);
                                    }
                                    else
                                    {
                                        memcpy(&link->sco_buf[link->curr_sco_len], &sco_buf[i], sco_len - i);
                                        link->curr_sco_len += sco_len - i;
                                        i += sco_len - i;
                                    }
                                    break;
                                }
                            }
                        }
                        else
                        {
                            link->curr_sco_len = 0;
                            memset(link->sco_buf, 0, SCO_PACKET_LEN);
                            sco_data_ind.length = sco_len;
                            sco_data_ind.p_data = sco_buf;
                            bt_mgr_dispatch(BT_MSG_SCO_DATA_IND, &payload);
                        }
                    }
                    else /* CVSD */
                    {
                        if (p_data->status == 0)
                        {
                            sco_data_ind.length = sco_len;
                            sco_data_ind.p_data = sco_buf;
                        }
                        else
                        {
                            memset(link->sco_buf, 0x55, SCO_PACKET_LEN);
                            sco_data_ind.length = sco_len;
                            sco_data_ind.p_data = link->sco_buf;
                        }

                        bt_mgr_dispatch(BT_MSG_SCO_DATA_IND, &payload);
                    }
                }
            }
        }
        break;

    case GAP_BR_SCO_DISCONN_RSP:
        {
            T_GAP_SCO_DISCONN_RSP *p_data = (T_GAP_SCO_DISCONN_RSP *)p_buf;

            payload.msg_buf = &p_data->reason;
            memcpy(payload.bd_addr, p_data->bd_addr, 6);
            bt_mgr_dispatch(BT_MSG_SCO_DISCONN_CMPL, &payload);
        }
        break;

    case GAP_BR_SCO_DISCONN_IND:
        {
            T_GAP_SCO_DISCONN_IND *p_data = (T_GAP_SCO_DISCONN_IND *)p_buf;

            payload.msg_buf = &p_data->cause;
            memcpy(payload.bd_addr, p_data->bd_addr, 6);
            bt_mgr_dispatch(BT_MSG_SCO_DISCONN_CMPL, &payload);
        }
        break;

    case GAP_BR_REMOTE_NAME_INFO:
        {
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_GAP_REMOTE_NAME_RSP, &payload);
        }
        break;

    case GAP_BR_ACL_STATUS_INFO:
        {
            bt_handle_acl_status_info(p_buf);
        }
        break;

    case GAP_BR_ROLE_SWITCH_RSP:
        {
            T_GAP_ACL_CFG_LINK_ROLE_RSP *p_data;

            p_data = (T_GAP_ACL_CFG_LINK_ROLE_RSP *)p_buf;

            link = bt_link_find(p_data->bd_addr);
            if (link != NULL)
            {
                /* Role switch success will be handled in ACL Status Event. */
                if (p_data->cause != BTIF_SUCCESS)
                {
                    link->role_switch_status = BT_ROLE_SWITCH_IDLE;

                    {
                        memcpy(payload.bd_addr, p_data->bd_addr, 6);
                        payload.msg_buf = &p_data->cause;
                        bt_mgr_dispatch(BT_MSG_ACL_ROLE_SWITCH_FAIL, &payload);
                    }
                }
            }
        }
        break;

    case GAP_BR_ENTER_SNIFF_MODE_RSP:
        {
            T_GAP_ACL_ENTER_SNIFF_RSP *p_data;

            p_data = (T_GAP_ACL_ENTER_SNIFF_RSP *)p_buf;

            link = bt_link_find(p_data->bd_addr);
            if (link != NULL)
            {
                /* Sniff-enter success will be handled in ACL Status Event. */
                if (p_data->cause != BTIF_SUCCESS)
                {
                    memcpy(payload.bd_addr, p_data->bd_addr, 6);
                    payload.msg_buf = &p_data->cause;
                    bt_mgr_dispatch(BT_MSG_ACL_SNIFF_ENTER_FAIL, &payload);
                }
            }
        }
        break;

    case GAP_BR_EXIT_SNIFF_MODE_RSP:
        {
            T_GAP_ACL_EXIT_SNIFF_RSP *p_data;

            p_data = (T_GAP_ACL_EXIT_SNIFF_RSP *)p_buf;

            link = bt_link_find(p_data->bd_addr);
            if (link != NULL)
            {
                /* Sniff-exit success will be handled in ACL Status Event. */
                if (p_data->cause != BTIF_SUCCESS)
                {
                    memcpy(payload.bd_addr, p_data->bd_addr, 6);
                    payload.msg_buf = &p_data->cause;
                    bt_mgr_dispatch(BT_MSG_ACL_SNIFF_EXIT_FAIL, &payload);
                }
            }
        }
        break;

    case GAP_BR_LINK_POLICY_RSP:
        {
            T_GAP_ACL_CFG_LINK_POLICY_RSP *p_data;

            p_data = (T_GAP_ACL_CFG_LINK_POLICY_RSP *)p_buf;
            if (p_data->cause == BTIF_SUCCESS)
            {
                memcpy(payload.bd_addr, p_data->bd_addr, 6);
                bt_mgr_dispatch(BT_MSG_ACL_LINK_POLICY_RSP, &payload);
            }
        }
        break;

    case GAP_BR_ACL_CONN_IND:
        {
            T_GAP_ACL_CONN_IND *p_ind = (T_GAP_ACL_CONN_IND *)p_buf;

            link = bt_link_find(p_ind->bd_addr);
            if (link)
            {
                BTM_PRINT_ERROR1("bt_gap_cback: link already connected with addr %s",
                                 TRACE_BDADDR(p_ind->bd_addr));
                gap_br_reject_acl_conn(p_ind->bd_addr, GAP_ACL_REJECT_UNACCEPTABLE_ADDR);
                return;
            }

            link = bt_link_alloc(p_ind->bd_addr);
            if (link == NULL)
            {
                BTM_PRINT_ERROR1("bt_gap_cback: no link resouce for addr %s",
                                 TRACE_BDADDR(p_ind->bd_addr));
                gap_br_reject_acl_conn(p_ind->bd_addr, GAP_ACL_REJECT_LIMITED_RESOURCE);
                return;
            }

            link->acl_link_state = BT_LINK_STATE_CONNECTING;
            memcpy(payload.bd_addr, p_ind->bd_addr, 6);
            payload.msg_buf = &p_ind->cod;
            bt_mgr_dispatch(BT_MSG_ACL_CONN_IND, &payload);
        }
        break;

    case GAP_BR_INQUIRY_RESULT:
        {
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_GAP_INQUIRY_RESULT_INFO, &payload);
        }
        break;

    case GAP_BR_INQUIRY_RSP:
        {
            T_GAP_INQUIRY_RSP *p;

            p = (T_GAP_INQUIRY_RSP *)p_buf;
            payload.msg_buf = &p->cause;
            bt_mgr_dispatch(BT_MSG_GAP_INQUIRY_RSP, &payload);
        }
        break;

    case GAP_BR_INQUIRY_CMPL:
        {
            T_GAP_INQUIRY_CMPL_INFO *p;

            p = (T_GAP_INQUIRY_CMPL_INFO *)p_buf;
            payload.msg_buf = &p->cause;
            bt_mgr_dispatch(BT_MSG_GAP_INQUIRY_CMPL, &payload);
        }
        break;

    case GAP_BR_INQUIRY_CANCEL_RSP:
        {
            T_GAP_INQUIRY_CANCEL_RSP *p;

            p = (T_GAP_INQUIRY_CANCEL_RSP *)p_buf;
            payload.msg_buf = &p->cause;
            bt_mgr_dispatch(BT_MSG_GAP_INQUIRY_CANCEL_RSP, &payload);
        }
        break;

    case GAP_BR_PERIODIC_INQUIRY_RSP:
        {
            T_GAP_PERIODIC_INQUIRY_RSP *p;

            p = (T_GAP_PERIODIC_INQUIRY_RSP *)p_buf;
            payload.msg_buf = &p->cause;
            bt_mgr_dispatch(BT_MSG_GAP_PERIODIC_INQUIRY_RSP, &payload);
        }
        break;

    case GAP_BR_PERIODIC_INQUIRY_CANCEL_RSP:
        {
            T_GAP_PERIODIC_INQUIRY_CANCEL_RSP *p;

            p = (T_GAP_PERIODIC_INQUIRY_CANCEL_RSP *)p_buf;
            payload.msg_buf = &p->cause;
            bt_mgr_dispatch(BT_MSG_GAP_PERIODIC_INQUIRY_CANCEL_RSP, &payload);
        }
        break;

    case GAP_BR_LINK_KEY_INFO:
        {
            T_GAP_LINK_KEY_INFO *info = (T_GAP_LINK_KEY_INFO *)p_buf;
            memcpy(payload.bd_addr, info->bd_addr, 6);
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_ACL_LINK_KEY_INFO, &payload);
        }
        break;

    case GAP_BR_LINK_KEY_REQ_IND:
        {
            T_GAP_LINK_KEY_REQ_IND *info = (T_GAP_LINK_KEY_REQ_IND *)p_buf;
            memcpy(payload.bd_addr, info->bd_addr, 6);
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_ACL_LINK_KEY_REQ_IND, &payload);
        }
        break;

    case GAP_BR_PIN_CODE_REQ_IND:
        {
            T_GAP_PIN_CODE_REQ_IND *info = (T_GAP_PIN_CODE_REQ_IND *)p_buf;
            memcpy(payload.bd_addr, info->bd_addr, 6);
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_ACL_PIN_CODE_REQ_IND, &payload);
        }
        break;

    case GAP_BR_USER_CONFIRM_REQ:
        {
            T_GAP_USER_CFM_REQ_IND *info = (T_GAP_USER_CFM_REQ_IND *)p_buf;
            memcpy(payload.bd_addr, info->bd_addr, 6);
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_ACL_USER_CONFIRM_REQ, &payload);
        }
        break;

    case GAP_BR_PASSKEY_INPUT:
        {
            T_GAP_USER_PASSKEY_REQ_IND *info = (T_GAP_USER_PASSKEY_REQ_IND *)p_buf;
            memcpy(payload.bd_addr, info->bd_addr, 6);
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_ACL_USER_PASSKEY_REQ_IND, &payload);
        }
        break;

    case GAP_BR_PASSKEY_DISPLAY:
        {
            T_GAP_USER_PASSKEY_NOTIF_INFO *info = (T_GAP_USER_PASSKEY_NOTIF_INFO *)p_buf;
            memcpy(payload.bd_addr, info->bd_addr, 6);
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_ACL_USER_PASSKEY_NOTIF, &payload);
        }
        break;

    case GAP_BR_READ_RSSI_RSP:
        {
            T_GAP_READ_RSSI_RSP *info = (T_GAP_READ_RSSI_RSP *)p_buf;
            memcpy(payload.bd_addr, info->bd_addr, 6);
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_ACL_READ_RSSI_RSP, &payload);
        }
        break;

    case GAP_BR_SETUP_QOS_RSP:
        {
            T_GAP_SETUP_QOS_RSP *p_rsp = (T_GAP_SETUP_QOS_RSP *)p_buf;

            link = bt_link_find_by_handle(p_rsp->handle);
            if (link)
            {
                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = p_buf;
                bt_mgr_dispatch(BT_MSG_ACL_SET_QOS_RSP, &payload);
            }
        }
        break;

    case GAP_BR_SETUP_QOS_CMPL:
        {
            T_GAP_SETUP_QOS_CMPL *info = (T_GAP_SETUP_QOS_CMPL *)p_buf;
            memcpy(payload.bd_addr, info->bd_addr, 6);
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_ACL_SET_QOS_CMPL, &payload);
        }
        break;

    case GAP_BR_SET_TX_POWER_RSP:
        {
            payload.msg_buf = p_buf;
            bt_mgr_dispatch(BT_MSG_VND_SET_TX_POWER_RSP, &payload);
        }
        break;

    case GAP_BR_LOCAL_OOB_RSP:
        {
            payload.msg_buf = (void *)p_buf;
            bt_mgr_dispatch(BT_MSG_GAP_LOCAL_OOB_DATA_RSP, &payload);
        }
        break;

    case GAP_BR_LOCAL_OOB_EXTENDED_RSP:
        {
            payload.msg_buf = (void *)p_buf;
            bt_mgr_dispatch(BT_MSG_GAP_LOCAL_OOB_EXTENDED_DATA_RSP, &payload);
        }
        break;

    case GAP_BR_SET_RSSI_GOLDEN_RANGE_RSP:
        {
            payload.msg_buf = (void *)p_buf;
            bt_mgr_dispatch(BT_MSG_VND_SET_RSSI_GOLDEN_RANGE_RSP, &payload);
        }
        break;

    case GAP_BR_SET_IDLE_ZONE_RSP:
        payload.msg_buf = (void *)p_buf;
        bt_mgr_dispatch(BT_MSG_VND_SET_IDLE_ZONE_RSP, &payload);
        break;

    case GAP_BR_VND_TRAFFIC_QOS_RSP:
        payload.msg_buf = (void *)p_buf;
        bt_mgr_dispatch(BT_MSG_VND_TRAFFIC_QOS_RSP, &payload);
        break;

    case GAP_BR_VND_TRAFFIC_QOS_CMPL:
        payload.msg_buf = (void *)p_buf;
        bt_mgr_dispatch(BT_MSG_VND_TRAFFIC_QOS_CMPL, &payload);
        break;

    case GAP_BR_VENDOR_EVT:
        bt_handle_vendor_evt_info((T_GAP_VENDOR_EVT_INFO *)p_buf);
        break;

    default:
        break;
    }
}

bool bt_gap_init(void)
{
    gap_br_init(bt_gap_cback);
    return true;
}
