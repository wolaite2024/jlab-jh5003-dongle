/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "gap_handover_br.h"
#include "bt_roleswap.h"
#include "bt_roleswap_int.h"
#include "remote.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

typedef enum
{
    ROLESWAP_STATE_IDLE         = 0x00,
    ROLESWAP_STATE_LINK_SNIFF   = 0x01,
    ROLESWAP_STATE_LINK_SYNC    = 0x02,
    ROLESWAP_STATE_LINK_SUSPEND = 0x03,
    ROLESWAP_STATE_LINK_SHADOW  = 0x04,
    ROLESWAP_STATE_LINK_RESUME  = 0x05,
    ROLESWAP_STATE_ADDR_SWITCH  = 0x06,
    ROLESWAP_STATE_ROLE_SWITCH  = 0x07,
} T_ROLESWAP_STATE;

#if (CONFIG_REALTEK_BTM_ROLESWAP_SUPPORT == 1)
static T_ROLESWAP_STATE bt_roleswap_state = ROLESWAP_STATE_IDLE;

bool bt_relay_pause_link(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    BTM_PRINT_TRACE1("bt_relay_pause_link: bd_addr %s", TRACE_BDADDR(bd_addr));

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (gap_br_set_acl_arqn(link->acl_handle, GAP_ACL_ARQN_NACK) == GAP_CAUSE_SUCCESS)
        {
            bt_roleswap_state = ROLESWAP_STATE_LINK_SUSPEND;
            return true;
        }
        else
        {
            bt_roleswap_state = ROLESWAP_STATE_IDLE;
        }
    }

    return false;
}

bool bt_relay_start_shadow_link(uint8_t  bd_addr[6],
                                uint16_t control_conn_handle)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (gap_br_shadow_link(link->acl_handle, control_conn_handle,
                               GAP_SHADOW_SNIFF_OP_NO_SNIFFING) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_relay_start_shadow_info(uint8_t  bd_addr[6],
                                uint16_t control_handle)
{
    T_BT_LINK *link;

    BTM_PRINT_TRACE2("bt_relay_start_shadow_info: bd_addr %s, control_handle 0x%04x",
                     TRACE_BDADDR(bd_addr), control_handle);

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (gap_br_shadow_pre_sync_info(link->acl_handle, control_handle, 0) == GAP_CAUSE_SUCCESS)
        {
            bt_roleswap_state = ROLESWAP_STATE_LINK_SYNC;
            return true;
        }
        else
        {
            bt_roleswap_state = ROLESWAP_STATE_IDLE;
        }
    }

    return false;
}

void bt_relay_handle_profile_conn(uint8_t  bd_addr[6],
                                  uint32_t profile_mask,
                                  uint8_t  param)
{
    switch (profile_mask)
    {
    case SPP_PROFILE_MASK:
        {
            T_ROLESWAP_SPP_INFO spp_info;

            if (bt_spp_roleswap_info_get(bd_addr, param, &spp_info) == true)
            {
                bt_roleswap_info_alloc(bd_addr,
                                       ROLESWAP_TYPE_SPP,
                                       (uint8_t *)&spp_info,
                                       sizeof(spp_info));
                bt_roleswap_info_send(ROLESWAP_MODULE_SPP,
                                      ROLESWAP_SPP_CONN,
                                      (uint8_t *)&spp_info,
                                      sizeof(spp_info));
                bt_roleswap_rfc_info_get(bd_addr, spp_info.rfc_dlci, UUID_SERIAL_PORT);
            }
        }
        break;

    case A2DP_PROFILE_MASK:
        {
            T_ROLESWAP_A2DP_INFO a2dp_info;

            if (bt_a2dp_roleswap_info_get(bd_addr, &a2dp_info) == true)
            {
                bt_roleswap_info_alloc(bd_addr,
                                       ROLESWAP_TYPE_A2DP,
                                       (uint8_t *)&a2dp_info,
                                       sizeof(a2dp_info));
                bt_roleswap_info_send(ROLESWAP_MODULE_A2DP,
                                      ROLESWAP_A2DP_CONN,
                                      (uint8_t *)&a2dp_info,
                                      sizeof(a2dp_info));
                gap_br_get_handover_l2c_info(a2dp_info.sig_cid);
                gap_br_get_handover_l2c_info(a2dp_info.stream_cid);
            }
        }
        break;

    case AVRCP_PROFILE_MASK:
        {
            T_ROLESWAP_AVRCP_INFO avrcp_info;

            if (bt_avrcp_roleswap_info_get(bd_addr, &avrcp_info) == true)
            {
                bt_roleswap_info_alloc(bd_addr,
                                       ROLESWAP_TYPE_AVRCP,
                                       (uint8_t *)&avrcp_info,
                                       sizeof(avrcp_info));
                bt_roleswap_info_send(ROLESWAP_MODULE_AVRCP,
                                      ROLESWAP_AVRCP_CONN,
                                      (uint8_t *)&avrcp_info,
                                      sizeof(avrcp_info));
                gap_br_get_handover_l2c_info(avrcp_info.l2c_cid);
            }
        }
        break;

    case HFP_PROFILE_MASK:
    case HSP_PROFILE_MASK:
        {
            T_ROLESWAP_HFP_INFO hfp_info;

            if (bt_hfp_roleswap_info_get(bd_addr, &hfp_info) == true)
            {
                bt_roleswap_info_alloc(bd_addr,
                                       ROLESWAP_TYPE_HFP,
                                       (uint8_t *)&hfp_info,
                                       sizeof(hfp_info));
                bt_roleswap_info_send(ROLESWAP_MODULE_HFP,
                                      ROLESWAP_HFP_CONN,
                                      (uint8_t *)&hfp_info,
                                      sizeof(hfp_info));
                bt_roleswap_rfc_info_get(bd_addr, hfp_info.rfc_dlci, hfp_info.uuid);
            }
        }
        break;

    case PBAP_PROFILE_MASK:
        {
            T_ROLESWAP_PBAP_INFO pbap_info;

            if (bt_pbap_roleswap_info_get(bd_addr, &pbap_info) == true)
            {
                bt_roleswap_info_alloc(bd_addr,
                                       ROLESWAP_TYPE_PBAP,
                                       (uint8_t *)&pbap_info,
                                       sizeof(pbap_info));
                bt_roleswap_info_send(ROLESWAP_MODULE_PBAP,
                                      ROLESWAP_PBAP_CONN,
                                      (uint8_t *)&pbap_info,
                                      sizeof(pbap_info));

                if (pbap_info.obex_psm)
                {
                    gap_br_get_handover_l2c_info(pbap_info.l2c_cid);
                }
                else
                {
                    bt_roleswap_rfc_info_get(bd_addr, pbap_info.rfc_dlci, UUID_PBAP);
                }
            }
        }
        break;

    case HID_DEVICE_PROFILE_MASK:
        {
            T_ROLESWAP_HID_DEVICE_INFO hid_device_info;

            if (bt_hid_device_roleswap_info_get(bd_addr, &hid_device_info) == true)
            {
                bt_roleswap_info_alloc(bd_addr,
                                       ROLESWAP_TYPE_HID_DEVICE,
                                       (uint8_t *)&hid_device_info,
                                       sizeof(hid_device_info));
                bt_roleswap_info_send(ROLESWAP_MODULE_HID_DEVICE,
                                      ROLESWAP_HID_DEVICE_CONN,
                                      (uint8_t *)&hid_device_info,
                                      sizeof(hid_device_info));
                gap_br_get_handover_l2c_info(hid_device_info.control_cid);
                gap_br_get_handover_l2c_info(hid_device_info.interrupt_cid);
            }
        }
        break;

    case HID_HOST_PROFILE_MASK:
        {
            T_ROLESWAP_HID_HOST_INFO hid_host_info;

            if (bt_hid_host_roleswap_info_get(bd_addr, &hid_host_info) == true)
            {
                bt_roleswap_info_alloc(bd_addr,
                                       ROLESWAP_TYPE_HID_HOST,
                                       (uint8_t *)&hid_host_info,
                                       sizeof(hid_host_info));
                bt_roleswap_info_send(ROLESWAP_MODULE_HID_DEVICE,
                                      ROLESWAP_HID_HOST_CONN,
                                      (uint8_t *)&hid_host_info,
                                      sizeof(hid_host_info));
                gap_br_get_handover_l2c_info(hid_host_info.control_cid);
                gap_br_get_handover_l2c_info(hid_host_info.interrupt_cid);
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_handle_profile_disconn(uint8_t                           bd_addr[6],
                                     T_ROLESWAP_PROFILE_DISCONN_PARAM *param)
{
    uint8_t buf[7];

    memcpy(buf, bd_addr, 6);

    switch (param->profile_mask)
    {
    case SPP_PROFILE_MASK:
        buf[6] = param->param;
        bt_roleswap_spp_info_free(bd_addr, param->param);
        bt_roleswap_info_send(ROLESWAP_MODULE_SPP, ROLESWAP_SPP_DISCONN, buf, 7);
        break;

    case A2DP_PROFILE_MASK:
        bt_roleswap_a2dp_info_free(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_A2DP, ROLESWAP_A2DP_DISCONN, buf, 6);
        break;

    case AVRCP_PROFILE_MASK:
        bt_roleswap_avrcp_info_free(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_AVRCP, ROLESWAP_AVRCP_DISCONN, buf, 6);
        break;

    case HFP_PROFILE_MASK:
    case HSP_PROFILE_MASK:
        bt_roleswap_hfp_info_free(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_HFP, ROLESWAP_HFP_DISCONN, buf, 6);
        break;

    case PBAP_PROFILE_MASK:
        bt_roleswap_pbap_info_free(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_PBAP, ROLESWAP_PBAP_DISCONN, buf, 6);
        break;

    case HID_DEVICE_PROFILE_MASK:
        bt_roleswap_hid_device_info_free(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_HID_DEVICE, ROLESWAP_HID_DEVICE_DISCONN, buf, 6);
        break;

    case HID_HOST_PROFILE_MASK:
        bt_roleswap_hid_host_info_free(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_HID_HOST, ROLESWAP_HID_HOST_DISCONN, buf, 6);
        break;

    default:
        break;
    }
}

void bt_relay_handle_bud_role_switch(uint8_t bd_addr[6])
{
    T_GAP_HANDOVER_BUD_INFO info;
    T_REMOTE_SESSION_ROLE   session_role;
    uint8_t                 peer_addr[6];
    uint8_t                 local_addr[6];

    remote_peer_addr_get(peer_addr);
    remote_local_addr_get(local_addr);
    session_role = remote_session_role_get();

    BTM_PRINT_TRACE5("bt_relay_handle_bud_role_switch: state %u, role %u, remote addr %s, "
                     "peer addr %s, local addr %s",
                     bt_roleswap_state, session_role, TRACE_BDADDR(bd_addr),
                     TRACE_BDADDR(peer_addr), TRACE_BDADDR(local_addr));

    if (bt_roleswap_state != ROLESWAP_STATE_IDLE)
    {
        T_BT_LINK *link;

        memcpy(info.pre_bd_addr, peer_addr, 6);
        memcpy(info.curr_bd_addr, local_addr, 6);
        gap_br_set_handover_bud_info(&info);

        link = bt_link_find(info.pre_bd_addr);
        if (link != NULL)
        {
            memcpy(link->bd_addr, info.curr_bd_add, 6);
        }

        remote_peer_addr_set(info.curr_bd_addr);
        remote_local_addr_set(info.pre_bd_addr);

        if (session_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            bt_roleswap_state = ROLESWAP_STATE_IDLE;

            /* update bud profile database */
            bt_rdtp_set_roleswap_info(info.pre_bd_addr, info.curr_bd_addr);

            {
                T_BT_MSG_PAYLOAD payload;

                payload.msg_buf = &info;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_ADDR_STATUS, &payload);
            }
        }
        else if (session_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            bt_roleswap_state = ROLESWAP_STATE_ADDR_SWITCH;

            /* update bud profile database */
            bt_rdtp_set_roleswap_info(info.pre_bd_addr, info.curr_bd_addr);

            /* set local bdaddr with peer bud address */
            gap_set_bd_addr(peer_addr);
        }
    }
}

void bt_relay_handle_acl_status(uint8_t   bd_addr[6],
                                T_BT_MSG  msg,
                                void     *buf)
{
    uint8_t peer_addr[6];

    remote_peer_addr_get(peer_addr);

    /* skip bud link addr */
    if (memcmp(bd_addr, peer_addr, 6))
    {
        T_ROLESWAP_DATA *data;

        switch (msg)
        {
        case BT_MSG_ACL_CONN_READY:
            {
                T_ROLESWAP_INFO *base;

                base = bt_roleswap_info_base_find(bd_addr);
                if (base)
                {
                    BTM_PRINT_ERROR1("bt_relay_handle_acl_status: roleswap info base already exists %s",
                                     TRACE_BDADDR(bd_addr));
                    return;
                }

                base = bt_roleswap_info_base_alloc(bd_addr);
                if (base == NULL)
                {
                    BTM_PRINT_ERROR1("bt_relay_handle_acl_status: fail to alloc roleswap base %s",
                                     TRACE_BDADDR(bd_addr));
                    return;
                }

                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data == NULL)
                {
                    gap_br_get_handover_acl_info(bd_addr);
                }
            }
            break;

        case BT_MSG_ACL_CONN_ACTIVE:
            {
                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data != NULL)
                {
                    data->u.acl.mode = 0;
                    BTM_PRINT_TRACE1("bt_relay_handle_acl_status: mode %u", data->u.acl.mode);
                }
            }
            break;

        case BT_MSG_ACL_CONN_SNIFF:
            {
                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data)
                {
                    data->u.acl.mode = 2;
                    BTM_PRINT_TRACE1("bt_relay_handle_acl_status: mode %u", data->u.acl.mode);
                }
            }
            break;

        case BT_MSG_ACL_AUTHEN_SUCCESS:
            {
                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data)
                {
                    data->u.acl.authen_state = true;
                    BTM_PRINT_TRACE1("bt_relay_handle_acl_status: authen_state %u", data->u.acl.authen_state);
                }
            }
            break;

        case BT_MSG_ACL_AUTHEN_FAIL:
            {
                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data)
                {
                    data->u.acl.authen_state = false;
                    BTM_PRINT_TRACE1("bt_relay_handle_acl_status: authen_state %u", data->u.acl.authen_state);

                    gap_br_get_handover_sm_info(data->u.acl.bd_addr);
                }
            }
            break;

        case BT_MSG_ACL_CONN_ENCRYPTED:
            {
                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data)
                {
                    data->u.acl.encrypt_state = 1;
                    BTM_PRINT_TRACE1("bt_relay_handle_acl_status: encrypt_state %u", data->u.acl.encrypt_state);

                    gap_br_get_handover_sm_info(data->u.acl.bd_addr);
                }
            }
            break;

        case BT_MSG_ACL_CONN_NOT_ENCRYPTED:
            {
                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data)
                {
                    data->u.acl.encrypt_state = 0;
                    BTM_PRINT_TRACE1("bt_relay_handle_acl_status: encrypt_state %u", data->u.acl.encrypt_state);

                    gap_br_get_handover_sm_info(data->u.acl.bd_addr);
                }
            }
            break;

        case BT_MSG_ACL_ROLE_MASTER:
            {
                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data)
                {
                    data->u.acl.role = 0;
                    BTM_PRINT_TRACE1("bt_relay_handle_acl_status: role %u", data->u.acl.role);
                }
            }
            break;

        case BT_MSG_ACL_ROLE_SLAVE:
            {
                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data)
                {
                    data->u.acl.role = 1;
                    BTM_PRINT_TRACE1("bt_relay_handle_acl_status: role %u", data->u.acl.role);
                }
            }
            break;

        case BT_MSG_ACL_CONN_DISCONN:
            {
                BTM_PRINT_TRACE0("bt_relay_handle_acl_status: disconnected");

                uint16_t cause = *(uint16_t *)buf;
                if (cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                {
                    T_ROLESWAP_INFO *base;

                    base = bt_roleswap_info_base_find(bd_addr);
                    if (base != NULL)
                    {
                        bt_roleswap_info_base_free(base);
                    }

                    bt_roleswap_info_send(ROLESWAP_MODULE_ACL, ROLESWAP_ACL_DISCONN, bd_addr, 6);
                }
            }
            break;

        case BT_MSG_ACL_LINK_KEY_INFO:
            {
                data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_ACL);
                if (data)
                {
                    gap_br_get_handover_sm_info(data->u.acl.bd_addr);
                }
            }
            break;

        default:
            break;
        }
    }
    else
    {
        if (msg == BT_MSG_ACL_ROLE_MASTER || msg == BT_MSG_ACL_ROLE_SLAVE)
        {
            bt_relay_handle_bud_role_switch(bd_addr);
        }
        else if (msg == BT_MSG_ACL_CONN_DISCONN)
        {
            /* clear secondary roleswap database when bud link disconnected */
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                T_ROLESWAP_INFO *base;

                base = bt_roleswap_info_base_find(bd_addr);
                if (base != NULL)
                {
                    bt_roleswap_info_base_free(base);
                }
            }
        }
        else if (msg == BT_MSG_ACL_CONN_ACTIVE)
        {
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
                bt_roleswap_state == ROLESWAP_STATE_LINK_SNIFF)
            {
                uint16_t   control_handle = 0xFFFF;
                uint8_t    peer_addr[6];
                T_BT_LINK *link;

                remote_peer_addr_get(peer_addr);

                link = bt_link_find(peer_addr);
                if (link != NULL)
                {
                    control_handle = link->acl_handle;
                }

                link = os_queue_peek(&btm_db->link_list, 0);
                while (link != NULL)
                {
                    if (memcmp(link->bd_addr, peer_addr, 6))
                    {
                        bt_relay_start_shadow_info(link->bd_addr, control_handle);
                    }

                    link = link->next;
                }
            }
        }
    }
}

void bt_relay_handle_sco_disconn(uint8_t  bd_addr[6],
                                 uint16_t cause)
{
    T_ROLESWAP_DATA *data;

    if (cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
    {
        data = bt_roleswap_data_find(bd_addr, ROLESWAP_TYPE_SCO);
        if (data != NULL)
        {
            bt_roleswap_info_free(bd_addr, data);
            bt_roleswap_info_send(ROLESWAP_MODULE_SCO, ROLESWAP_SCO_DISCONN, bd_addr, 6);
        }
    }
}

void bt_relay_handle_ctrl_conn(void)
{
    T_BT_LINK *link;
    uint8_t    peer_addr[6];

    remote_peer_addr_get(peer_addr);

    /* bud link is created after source/primary link. */
    link = os_queue_peek(&btm_db->link_list, 0);
    while (link != NULL)
    {
        if ((link->acl_link_state == BT_LINK_STATE_CONNECTED) &&
            memcmp(link->bd_addr, peer_addr, 6))
        {
            bt_roleswap_transfer(link->bd_addr);
        }

        link = link->next;
    }
}

bool bt_relay_start_roleswap(uint8_t                      bd_addr[6],
                             uint8_t                      context,
                             bool                         stop_after_shadow,
                             P_REMOTE_ROLESWAP_SYNC_CBACK cback)
{
    T_BT_LINK *link;

    BTM_PRINT_TRACE1("bt_relay_start_roleswap: bd_addr %s", TRACE_BDADDR(bd_addr));

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        T_BT_LINK *remote_link;
        uint8_t    peer_addr[6];

        remote_peer_addr_get(peer_addr);

        remote_link = bt_link_find(peer_addr);
        if (remote_link != NULL)
        {
            if (bt_sniff_mode_exit(remote_link->bd_addr, true) == true)
            {
                return bt_relay_start_shadow_info(link->bd_addr, remote_link->acl_handle);
            }
            else
            {
                bt_roleswap_state = ROLESWAP_STATE_LINK_SNIFF;
                return true;
            }
        }
    }

    return false;
}

bool bt_relay_roleswap_stop(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);

    BTM_PRINT_TRACE2("bt_relay_roleswap_stop: bd_addr %s, link %p",
                     TRACE_BDADDR(bd_addr), link);

    if (link != NULL)
    {
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            gap_br_set_acl_arqn(link->acl_handle, GAP_ACL_ARQN_ACK);
        }

        return true;
    }

    return false;
}

void bt_relay_recv_acl_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *info)
{
    T_ROLESWAP_INFO *base;
    T_ROLESWAP_DATA *data;

    switch (submodule)
    {
    case ROLESWAP_ACL_CONN:
        {
            T_ROLESWAP_ACL_INFO *p;

            p = (T_ROLESWAP_ACL_INFO *)info;
            base = bt_roleswap_info_base_find(p->bd_addr);
            if (base)
            {
                BTM_PRINT_ERROR1("bt_relay_recv_acl_info: roleswap info base already exists %s",
                                 TRACE_BDADDR(p->bd_addr));
                return;
            }

            base = bt_roleswap_info_base_alloc(p->bd_addr);
            if (base)
            {
                bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_ACL, info, len);
            }
        }
        break;

    case ROLESWAP_ACL_DISCONN:
        {
            T_ROLESWAP_INFO *base;

            BTM_PRINT_TRACE1("bt_relay_recv_acl_info: disconn %s", TRACE_BDADDR(info));

            base = bt_roleswap_info_base_find(info);
            if (base != NULL)
            {
                bt_roleswap_info_base_free(base);
            }
        }
        break;

    case ROLESWAP_ACL_UPDATE:
        {
            T_ROLESWAP_ACL_TRANSACT *transact;

            transact = (T_ROLESWAP_ACL_TRANSACT *)info;
            data = bt_roleswap_data_find(transact->bd_addr, ROLESWAP_TYPE_ACL);
            if (data)
            {
                data->u.acl.role = transact->role;
                data->u.acl.mode = transact->mode;
                data->u.acl.link_policy = transact->link_policy;
                data->u.acl.superv_tout = transact->superv_tout;
                data->u.acl.authen_state = transact->authen_state;
                data->u.acl.encrypt_state = transact->encrypt_state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_sco_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *info)
{
    T_ROLESWAP_DATA *data;

    switch (submodule)
    {
    case ROLESWAP_SCO_CONN:
        {
            T_ROLESWAP_SCO_INFO *p;

            p = (T_ROLESWAP_SCO_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_SCO, info, len);
        }
        break;
    case ROLESWAP_SCO_DISCONN:
        {
            data = bt_roleswap_data_find(info, ROLESWAP_TYPE_SCO);
            if (data)
            {
                bt_roleswap_info_free(info, data);
            }
        }
        break;
    default:
        break;
    }
}

void bt_relay_recv_sm_info(uint8_t   submodule,
                           uint16_t  len,
                           uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_SM_CONN:
        {
            T_ROLESWAP_SM_INFO *p;

            p = (T_ROLESWAP_SM_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_SM, info, len);
        }
        break;

    case ROLESWAP_SM_UPDATE:
        {
            T_ROLESWAP_DATA    *data;
            T_ROLESWAP_SM_INFO *p;

            p = (T_ROLESWAP_SM_INFO *)info;
            data = bt_roleswap_data_find(p->bd_addr, ROLESWAP_TYPE_SM);
            if (data)
            {
                memcpy((uint8_t *)&data->u.sm, (uint8_t *)info, sizeof(T_ROLESWAP_SM_INFO));
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_l2c_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_L2C_CONN:
        {
            T_ROLESWAP_L2C_INFO *p;

            p = (T_ROLESWAP_L2C_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_L2C, info, len);
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_rfc_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_RFC_CTRL_CONN:
        {
            T_ROLESWAP_RFC_CTRL_INFO *p;

            p = (T_ROLESWAP_RFC_CTRL_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_RFC_CTRL, info, len);
        }
        break;

    case ROLESWAP_RFC_DATA_CONN:
        {
            T_ROLESWAP_RFC_DATA_INFO *p;

            p = (T_ROLESWAP_RFC_DATA_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_RFC_DATA, info, len);
        }
        break;

    case ROLESWAP_RFC_DATA_TRANSACT:
        {
            T_ROLESWAP_DATA         *data;
            T_ROLESWAP_RFC_TRANSACT *transact;

            transact = (T_ROLESWAP_RFC_TRANSACT *)info;
            data = bt_roleswap_rfc_data_find(transact->bd_addr, transact->dlci);
            if (data != NULL)
            {
                data->u.rfc_data.given_credits = transact->given_credits;
                data->u.rfc_data.remote_remain_credits = transact->remote_remain_credits;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_spp_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_SPP_CONN:
        {
            T_ROLESWAP_SPP_INFO *p;

            p = (T_ROLESWAP_SPP_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_SPP, info, len);
        }
        break;

    case ROLESWAP_SPP_DISCONN:
        {
            uint8_t bd_addr[6];
            uint8_t local_server_chann;

            memcpy(bd_addr, info, 6);
            local_server_chann = *(info + 6);
            bt_roleswap_spp_info_free(bd_addr, local_server_chann);
        }
        break;

    case ROLESWAP_SPP_TRANSACT:
        {
            T_ROLESWAP_SPP_TRANSACT *transact;
            T_ROLESWAP_DATA         *data;

            transact = (T_ROLESWAP_SPP_TRANSACT *)info;
            data = bt_roleswap_spp_find(transact->bd_addr, transact->local_server_chann);
            if (data)
            {
                data->u.spp.state = transact->state;
                data->u.spp.remote_credit = transact->remote_credit;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_a2dp_info(uint8_t   submodule,
                             uint16_t  len,
                             uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_A2DP_CONN:
        {
            T_ROLESWAP_A2DP_INFO *p;

            p = (T_ROLESWAP_A2DP_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_A2DP, info, len);
        }
        break;

    case ROLESWAP_A2DP_DISCONN:
        bt_roleswap_a2dp_info_free(info);
        break;

    case ROLESWAP_A2DP_TRANSACT:
        {
            T_ROLESWAP_A2DP_TRANSACT *transact;
            T_ROLESWAP_DATA          *data;

            transact = (T_ROLESWAP_A2DP_TRANSACT *)info;
            data = bt_roleswap_data_find(transact->bd_addr, ROLESWAP_TYPE_A2DP);
            if (data)
            {
                data->u.a2dp.sig_state = transact->sig_state;
                data->u.a2dp.state = transact->state;
                data->u.a2dp.tx_trans_label = transact->tx_trans_label;
                data->u.a2dp.rx_start_trans_label = transact->rx_start_trans_label;
                data->u.a2dp.cmd_flag = transact->cmd_flag;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_avrcp_info(uint8_t   submodule,
                              uint16_t  len,
                              uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_AVRCP_CONN:
        {
            T_ROLESWAP_AVRCP_INFO *p;

            p = (T_ROLESWAP_AVRCP_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_AVRCP, info, len);
        }
        break;

    case ROLESWAP_AVRCP_DISCONN:
        bt_roleswap_avrcp_info_free(info);
        break;

    case ROLESWAP_AVRCP_TRANSACT:
        {
            T_ROLESWAP_AVRCP_TRANSACT *transact;
            T_ROLESWAP_DATA           *data;

            transact = (T_ROLESWAP_AVRCP_TRANSACT *)info;
            data = bt_roleswap_data_find(transact->bd_addr, ROLESWAP_TYPE_AVRCP);
            if (data)
            {
                data->u.avrcp.avctp_state = transact->avctp_state;
                data->u.avrcp.state = transact->state;
                data->u.avrcp.play_status = transact->play_status;
                data->u.avrcp.cmd_credits = transact->cmd_credits;
                data->u.avrcp.transact_label = transact->transact_label;
                data->u.avrcp.vol_change_pending_transact = transact->vol_change_pending_transact;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_hfp_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_HFP_CONN:
        {
            T_ROLESWAP_HFP_INFO *p;

            p = (T_ROLESWAP_HFP_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_HFP, info, len);
        }
        break;

    case ROLESWAP_HFP_DISCONN:
        bt_roleswap_hfp_info_free(info);
        break;

    case ROLESWAP_HFP_TRANSACT:
        {
            T_ROLESWAP_HFP_TRANSACT *transact;
            T_ROLESWAP_DATA         *data;

            transact = (T_ROLESWAP_HFP_TRANSACT *)info;

            data = bt_roleswap_data_find(transact->bd_addr, ROLESWAP_TYPE_HFP);
            if (data)
            {
                data->u.hfp.state = transact->state;
                data->u.hfp.batt_report_type = transact->batt_report_type;
                data->u.hfp.at_cmd_credits = transact->at_cmd_credits;
                data->u.hfp.curr_call_status = transact->curr_call_status;
                data->u.hfp.prev_call_status = transact->prev_call_status;
                data->u.hfp.service_status = transact->service_status;
                data->u.hfp.supported_features = transact->supported_features;
                data->u.hfp.codec_type = transact->codec_type;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_pbap_info(uint8_t   submodule,
                             uint16_t  len,
                             uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_PBAP_CONN:
        {
            T_ROLESWAP_PBAP_INFO *p;

            p = (T_ROLESWAP_PBAP_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_PBAP, info, len);
        }
        break;

    case ROLESWAP_PBAP_DISCONN:
        bt_roleswap_pbap_info_free(info);
        break;

    case ROLESWAP_PBAP_TRANSACT:
        {
            T_ROLESWAP_PBAP_TRANSACT *transact;
            T_ROLESWAP_DATA          *data;

            transact = (T_ROLESWAP_PBAP_TRANSACT *)info;
            data = bt_roleswap_data_find(transact->bd_addr, ROLESWAP_TYPE_PBAP);
            if (data)
            {
                data->u.pbap.obex_state = transact->obex_state;
                data->u.pbap.pbap_state = transact->pbap_state;
                data->u.pbap.path = transact->path;
                data->u.pbap.repos = transact->repos;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_hid_device_info(uint8_t   submodule,
                                   uint16_t  len,
                                   uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_HID_DEVICE_CONN:
        {
            T_ROLESWAP_HID_DEVICE_INFO *p;

            p = (T_ROLESWAP_HID_DEVICE_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_HID_DEVICE, info, len);
        }
        break;

    case ROLESWAP_HID_DEVICE_DISCONN:
        bt_roleswap_hid_device_info_free(info);
        break;

    case ROLESWAP_HID_DEVICE_TRANSACT:
        {
            T_ROLESWAP_HID_DEVICE_TRANSACT *transact;
            T_ROLESWAP_DATA                *data;

            transact = (T_ROLESWAP_HID_DEVICE_TRANSACT *)info;
            data = bt_roleswap_data_find(transact->bd_addr, ROLESWAP_TYPE_HID_DEVICE);
            if (data)
            {
                data->u.hid_device.proto_mode = transact->proto_mode;
                data->u.hid_device.control_state = transact->control_state;
                data->u.hid_device.interrupt_state = transact->interrupt_state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_hid_host_info(uint8_t   submodule,
                                 uint16_t  len,
                                 uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_HID_HOST_CONN:
        {
            T_ROLESWAP_HID_HOST_INFO *p;

            p = (T_ROLESWAP_HID_HOST_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_HID_HOST, info, len);
        }
        break;

    case ROLESWAP_HID_HOST_DISCONN:
        bt_roleswap_hid_host_info_free(info);
        break;

    case ROLESWAP_HID_HOST_TRANSACT:
        {
            T_ROLESWAP_HID_HOST_TRANSACT *transact;
            T_ROLESWAP_DATA              *data;

            transact = (T_ROLESWAP_HID_HOST_TRANSACT *)info;
            data = bt_roleswap_data_find(transact->bd_addr, ROLESWAP_TYPE_HID_HOST);
            if (data)
            {
                data->u.hid_host.proto_mode = transact->proto_mode;
                data->u.hid_host.control_state = transact->control_state;
                data->u.hid_host.interrupt_state = transact->interrupt_state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_avp_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_AVP_CONN:
        {
            T_ROLESWAP_AVP_INFO *p;

            p = (T_ROLESWAP_AVP_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_AVP, info, len);
        }
        break;

    case ROLESWAP_AVP_DISCONN:
        bt_roleswap_avp_info_free(info);
        break;

    case ROLESWAP_AVP_TRANSACT:
        {
            T_ROLESWAP_AVP_TRANSACT *transact;
            T_ROLESWAP_DATA         *data;

            transact = (T_ROLESWAP_AVP_TRANSACT *)info;
            data = bt_roleswap_data_find(transact->bd_addr, ROLESWAP_TYPE_AVP);
            if (data)
            {
                data->u.avp.state = transact->state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_att_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *info)
{
    switch (submodule)
    {
    case ROLESWAP_ATT_CONN:
        {
            T_ROLESWAP_ATT_INFO *p;

            p = (T_ROLESWAP_ATT_INFO *)info;
            bt_roleswap_info_alloc(p->bd_addr, ROLESWAP_TYPE_ATT, info, len);
        }
        break;

    case ROLESWAP_ATT_DISCONN:
        bt_roleswap_att_info_free(info);
        break;

    case ROLESWAP_ATT_TRANSACT:
        {
            T_ROLESWAP_ATT_TRANSACT *transact;
            T_ROLESWAP_DATA         *data;

            transact = (T_ROLESWAP_ATT_TRANSACT *)info;
            data = bt_roleswap_data_find(transact->bd_addr, ROLESWAP_TYPE_ATT);
            if (data)
            {
                data->u.att.state = transact->state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv(uint8_t  *data,
                   uint16_t  data_len)
{
    uint8_t  *p;
    uint8_t   module;
    uint8_t   submodule;
    uint16_t  len;

    p = data;

    while (data_len >= ROLESWAP_MSG_HDR_LEN)
    {
        LE_STREAM_TO_UINT8(module, p);
        LE_STREAM_TO_UINT8(submodule, p);
        LE_STREAM_TO_UINT16(len, p);

        if (len > data_len - ROLESWAP_MSG_HDR_LEN)
        {
            BTM_PRINT_ERROR2("bt_relay_recv: excepted len %u, remaining len %d", len, data_len);
            return;
        }

        switch (module)
        {
        case ROLESWAP_MODULE_ACL:
            bt_relay_recv_acl_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_SCO:
            bt_relay_recv_sco_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_L2C:
            bt_relay_recv_l2c_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_SM:
            bt_relay_recv_sm_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_RFC:
            bt_relay_recv_rfc_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_SPP:
            bt_relay_recv_spp_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_A2DP:
            bt_relay_recv_a2dp_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_AVRCP:
            bt_relay_recv_avrcp_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_HFP:
            bt_relay_recv_hfp_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_PBAP:
            bt_relay_recv_pbap_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_HID_DEVICE:
            bt_relay_recv_hid_device_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_HID_HOST:
            bt_relay_recv_hid_host_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_AVP:
            bt_relay_recv_avp_info(submodule, len, p);
            break;

        case ROLESWAP_MODULE_ATT:
            bt_relay_recv_att_info(submodule, len, p);
            break;

        default:
            BTM_PRINT_ERROR2("bt_relay_recv: unknown module %u, submodule %u", module, submodule);
            break;
        }

        data_len -= len + ROLESWAP_MSG_HDR_LEN;
        p += len;
    }
}

void bt_relay_set_info(T_ROLESWAP_INFO *base)
{
    uint16_t type;

    for (type = ROLESWAP_TYPE_ACL; type < ROLESWAP_TYPE_ALL; type++)
    {
        T_ROLESWAP_DATA *data;

        data = (T_ROLESWAP_DATA *)base->info_list.p_first;
        while (data)
        {
            if (data->type == type)
            {
                T_BT_MSG_PAYLOAD payload;

                switch (data->type)
                {
                case ROLESWAP_TYPE_ACL:
                    {
                        T_ROLESWAP_ACL_INFO *info = &data->u.acl;

                        bt_roleswap_acl_info_set(info);

                        memcpy(payload.bd_addr, base->bd_addr, 6);
                        payload.msg_buf = info;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_ACL_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_SCO:
                    {
                        T_ROLESWAP_SCO_INFO *info = &data->u.sco;

                        bt_roleswap_sco_info_set(base->bd_addr, info);

                        payload.msg_buf = info;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_SCO_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_SM:
                    {
                        T_GAP_HANDOVER_SM_INFO  sm;
                        T_ROLESWAP_SM_INFO     *info = &data->u.sm;

                        sm.mode = info->mode;
                        sm.state = info->state;
                        sm.sec_state = info->sec_state;
                        sm.remote_authen = info->remote_authen;
                        sm.remote_io = info->remote_io;
                        memcpy(sm.bd_addr, info->bd_addr, 6);
                        gap_br_set_handover_sm_info(&sm);
                    }
                    break;

                case ROLESWAP_TYPE_L2C:
                    {
                        T_GAP_HANDOVER_L2C_INFO  l2c;
                        T_ROLESWAP_L2C_INFO     *info = &data->u.l2c;

                        l2c.local_cid = info->local_cid;
                        l2c.remote_cid = info->remote_cid;
                        l2c.local_mtu = info->local_mtu;
                        l2c.remote_mtu = info->remote_mtu;
                        l2c.local_mps = info->local_mps;
                        l2c.remote_mps = info->remote_mps;
                        l2c.psm = info->psm;
                        l2c.role = info->role;
                        l2c.mode = info->mode;
                        memcpy(l2c.bd_addr, info->bd_addr, 6);
                        gap_br_set_handover_l2c_info(&l2c);
                    }
                    break;

                case ROLESWAP_TYPE_RFC_CTRL:
                    bt_roleswap_rfc_ctrl_info_set(base->bd_addr, &data->u.rfc_ctrl);
                    break;

                case ROLESWAP_TYPE_RFC_DATA:
                    bt_roleswap_rfc_data_info_set(base->bd_addr, &data->u.rfc_data);
                    break;

                case ROLESWAP_TYPE_SPP:
                    {
                        bt_spp_roleswap_info_set(base->bd_addr, &data->u.spp);

                        memcpy(payload.bd_addr, base->bd_addr, 6);
                        payload.msg_buf = &data->u.spp;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_SPP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_A2DP:
                    {
                        bt_a2dp_roleswap_info_set(base->bd_addr, &data->u.a2dp);

                        memcpy(payload.bd_addr, base->bd_addr, 6);
                        payload.msg_buf = &data->u.a2dp;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_A2DP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_AVRCP:
                    {
                        bt_avrcp_roleswap_info_set(base->bd_addr, &data->u.avrcp);

                        memcpy(payload.bd_addr, base->bd_addr, 6);
                        payload.msg_buf = &data->u.avrcp;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_AVRCP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_HFP:
                    {
                        bt_hfp_roleswap_info_set(base->bd_addr, &data->u.hfp);

                        memcpy(payload.bd_addr, base->bd_addr, 6);
                        payload.msg_buf = &data->u.hfp;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_HFP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_PBAP:
                    {
                        bt_pbap_roleswap_info_set(base->bd_addr, &data->u.pbap);

                        memcpy(payload.bd_addr, base->bd_addr, 6);
                        payload.msg_buf = NULL;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_PBAP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_HID_DEVICE:
                    {
                        bt_hid_device_roleswap_info_set(base->bd_addr, &data->u.hid_device);

                        memcpy(payload.bd_addr, base->bd_addr, 6);
                        payload.msg_buf = &data->u.hid_device;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_HID_DEVICE_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_HID_HOST:
                    {
                        bt_hid_host_roleswap_info_set(base->bd_addr, &data->u.hid_host);

                        memcpy(payload.bd_addr, base->bd_addr, 6);
                        payload.msg_buf = &data->u.hid_host;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_HID_HOST_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_IAP:
                    {
                        bt_iap_roleswap_info_set(base->bd_addr, &data->u.iap);

                        memcpy(payload.bd_addr, base->bd_addr, 6);
                        payload.msg_buf = NULL;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_IAP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_AVP:
                    {
                        bt_avp_roleswap_info_set(base->bd_addr, &data->u.avp);
                    }
                    break;

                case ROLESWAP_TYPE_ATT:
                    {
                        bt_att_roleswap_info_set(base->bd_addr, &data->u.att);
                    }
                    break;

                default:
                    break;
                }
            }

            data = data->next;
        }
    }
}

void bt_relay_set(void)
{
    T_ROLESWAP_INFO *roleswap_info;

    roleswap_info = os_queue_peek(&btm_db->roleswap_info_list, 0);
    while (roleswap_info != NULL)
    {
        bt_relay_set_info(roleswap_info);

        roleswap_info = roleswap_info->next;
    }
}

void bt_relay_callback(void                       *buf,
                       T_GAP_BR_HANDOVER_MSG_TYPE  msg)
{
    BTM_PRINT_TRACE1("bt_relay_callback: message 0x%02x", msg);

    switch (msg)
    {
    case GAP_BR_GET_ACL_INFO_RSP:
        {
            T_ROLESWAP_DATA         *data;
            T_ROLESWAP_ACL_INFO      acl;
            T_GAP_HANDOVER_ACL_INFO *info = (T_GAP_HANDOVER_ACL_INFO *)buf;
            T_BT_LINK               *link;

            link = bt_link_find(info->bd_addr);
            if ((link == NULL) || (link->acl_link_state != BT_LINK_STATE_CONNECTED))
            {
                break;
            }

            acl.authen_state = link->acl_link_authenticated;

            acl.bd_type = info->bd_type;
            acl.conn_type = info->conn_type;
            acl.encrypt_state = info->encrypt_state;
            acl.handle = info->handle;
            acl.link_policy = info->link_policy;
            acl.mode = info->mode;
            acl.role = info->role;
            acl.superv_tout = info->superv_tout;
            memcpy(acl.bd_addr, info->bd_addr, 6);

            data = bt_roleswap_data_find(info->bd_addr, ROLESWAP_TYPE_ACL);
            if (data != NULL)
            {
                memcpy((uint8_t *)&data->u.acl, (uint8_t *)&acl, sizeof(T_ROLESWAP_ACL_INFO));
            }
            else
            {
                bt_roleswap_info_alloc(info->bd_addr, ROLESWAP_TYPE_ACL, (uint8_t *)&acl,
                                       sizeof(T_ROLESWAP_ACL_INFO));
            }

            bt_roleswap_info_send(ROLESWAP_MODULE_ACL, ROLESWAP_ACL_CONN,
                                  (uint8_t *)&acl, sizeof(T_ROLESWAP_ACL_INFO));
        }
        break;

    case GAP_BR_GET_SCO_INFO_RSP:
        {
            T_ROLESWAP_SCO_INFO      sco;
            T_GAP_HANDOVER_SCO_INFO *info = (T_GAP_HANDOVER_SCO_INFO *)buf;

            sco.handle = info->handle;
            sco.type = info->type;
            memcpy(sco.bd_addr, info->bd_addr, 6);
            bt_roleswap_sco_info_get(info->bd_addr, &sco);

            bt_roleswap_info_alloc(info->bd_addr,
                                   ROLESWAP_TYPE_SCO,
                                   (uint8_t *)&sco,
                                   sizeof(T_ROLESWAP_SCO_INFO));
            bt_roleswap_info_send(ROLESWAP_MODULE_SCO,
                                  ROLESWAP_SCO_CONN,
                                  (uint8_t *)&sco,
                                  sizeof(T_ROLESWAP_SCO_INFO));
        }
        break;

    case GAP_BR_GET_SM_INFO_RSP:
        {
            T_ROLESWAP_DATA        *data;
            T_ROLESWAP_SM_INFO      sm;
            T_GAP_HANDOVER_SM_INFO *info = (T_GAP_HANDOVER_SM_INFO *)buf;

            sm.mode = info->mode;
            sm.state = info->state;
            sm.sec_state = info->sec_state;
            sm.remote_authen = info->remote_authen;
            sm.remote_io = info->remote_io;
            memcpy(sm.bd_addr, info->bd_addr, 6);

            data = bt_roleswap_data_find(info->bd_addr, ROLESWAP_TYPE_SM);
            if (data)
            {
                memcpy((uint8_t *)&data->u.sm, (uint8_t *)&sm, sizeof(T_ROLESWAP_SM_INFO));
                bt_roleswap_info_send(ROLESWAP_MODULE_SM,
                                      ROLESWAP_SM_UPDATE,
                                      (uint8_t *)&sm,
                                      sizeof(T_ROLESWAP_SM_INFO));
            }
            else
            {
                bt_roleswap_info_alloc(info->bd_addr,
                                       ROLESWAP_TYPE_SM,
                                       (uint8_t *)&sm,
                                       sizeof(T_ROLESWAP_SM_INFO));
                bt_roleswap_info_send(ROLESWAP_MODULE_SM,
                                      ROLESWAP_SM_CONN,
                                      (uint8_t *)&sm,
                                      sizeof(T_ROLESWAP_SM_INFO));
            }
        }
        break;

    case GAP_BR_GET_L2C_INFO_RSP:
        {
            T_ROLESWAP_L2C_INFO      l2c;
            T_GAP_HANDOVER_L2C_INFO *info = (T_GAP_HANDOVER_L2C_INFO *)buf;

            l2c.local_cid = info->local_cid;
            l2c.remote_cid = info->remote_cid;
            l2c.local_mtu = info->local_mtu;
            l2c.remote_mtu = info->remote_mtu;
            l2c.local_mps = info->local_mps;
            l2c.remote_mps = info->remote_mps;
            l2c.psm = info->psm;
            l2c.role = info->role;
            l2c.mode = info->mode;
            memcpy(l2c.bd_addr, info->bd_addr, 6);

            bt_roleswap_info_alloc(info->bd_addr,
                                   ROLESWAP_TYPE_L2C,
                                   (uint8_t *)&l2c,
                                   sizeof(T_ROLESWAP_L2C_INFO));
            bt_roleswap_info_send(ROLESWAP_MODULE_L2C,
                                  ROLESWAP_L2C_CONN,
                                  (uint8_t *)&l2c,
                                  sizeof(T_ROLESWAP_L2C_INFO));
        }
        break;

    case SET_BD_ADDR_RSP:
        {
            T_GAP_SET_BD_ADDR_RSP *rsp = (T_GAP_SET_BD_ADDR_RSP *)buf;
            uint8_t                peer_addr[6];
            uint8_t                local_addr[6];

            remote_peer_addr_get(peer_addr);
            remote_local_addr_get(local_addr);

            BTM_PRINT_TRACE5("bt_relay_callback: set bd addr rsp, cause 0x%04x, state %u, "
                             "role %u, peer addr %s, local addr %s",
                             rsp->cause, bt_roleswap_state, remote_session_role_get(),
                             TRACE_BDADDR(peer_addr), TRACE_BDADDR(local_addr));

            if (bt_roleswap_state == ROLESWAP_STATE_IDLE)
            {
                T_BT_MSG_PAYLOAD payload;

                memcpy(payload.bd_addr, rsp->bd_addr, 6);
                payload.msg_buf = &rsp->cause;
                bt_mgr_dispatch(BT_MSG_GAP_LOCAL_ADDR_CHANGED, &payload);
            }
            else
            {
                if (!rsp->cause)
                {
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        bt_roleswap_state = ROLESWAP_STATE_ROLE_SWITCH;

                        /* update pseudo primary local bdaddr when role switched later;
                        * and pseudo secondary still uses its origin bdaddr.
                        */
                        bt_link_role_switch(peer_addr, true);
                    }
                    else if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
                    {
                        T_BT_MSG_PAYLOAD        payload;
                        T_GAP_HANDOVER_BUD_INFO info;

                        bt_roleswap_state = ROLESWAP_STATE_IDLE;
                        memcpy(info.pre_bd_addr, local_addr, 6);
                        memcpy(info.curr_bd_addr, peer_addr, 6);
                        payload.msg_buf = &info;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_ADDR_STATUS, &payload);
                    }
                }
            }
        }
        break;

    case GAP_BR_SHADOW_PRE_SYNC_INFO_RSP:
        {
            T_GAP_SHADOW_PRE_SYNC_INFO_RSP *rsp = (T_GAP_SHADOW_PRE_SYNC_INFO_RSP *)buf;

            if (rsp->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: shadow info failed 0x%04x", rsp->cause);
                bt_roleswap_state = ROLESWAP_STATE_IDLE;
            }
            else
            {
                T_BT_LINK *link;
                int8_t     peer_addr[6];

                remote_peer_addr_get(peer_addr);

                link = os_queue_peek(&btm_db->link_list, 0);
                while (link != NULL)
                {
                    if (memcmp(link->bd_addr, peer_addr, 6))
                    {
                        bt_relay_pause_link(link->bd_addr);
                    }

                    link = link->next;
                }
            }
        }
        break;

    case GAP_BR_SET_ACL_ACTIVE_STATE_RSP:
        {
            T_GAP_SET_ACL_ACTIVE_STATE_RSP *rsp = (T_GAP_SET_ACL_ACTIVE_STATE_RSP *)buf;

            if (rsp->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: set acl active state fail, cause 0x%04x", rsp->cause);
                bt_roleswap_state = ROLESWAP_STATE_IDLE;
            }
        }
        break;

    case GAP_BR_SHADOW_LINK_RSP:
        {
            T_GAP_SHADOW_LINK_RSP *rsp = (T_GAP_SHADOW_LINK_RSP *)buf;

            if (rsp->cause)
            {
                T_BT_MSG_PAYLOAD payload;

                BTM_PRINT_ERROR1("bt_relay_callback: shadow link fail, cause 0x%04x", rsp->cause);

                bt_roleswap_state = ROLESWAP_STATE_IDLE;

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    T_BT_LINK *link;
                    int8_t     peer_addr[6];

                    remote_peer_addr_get(peer_addr);

                    link = os_queue_peek(&btm_db->link_list, 0);
                    while (link != NULL)
                    {
                        if (memcmp(link->bd_addr, peer_addr, 6))
                        {
                            bt_relay_roleswap_stop(link->bd_addr);
                        }

                        link = link->next;
                    }
                }

                payload.msg_buf = &rsp->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case GAP_BR_ACL_SUSPEND_RX_EMPTY_INFO:
        {
            T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *info = (T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *)buf;
            T_BT_MSG_PAYLOAD                 payload;
            uint16_t                         control_conn_handle = 0xFFFF;
            T_BT_LINK                       *link;
            uint8_t                          peer_addr[6];

            remote_peer_addr_get(peer_addr);

            if (info->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: acl suspend fail, cause 0x%04x", info->cause);

                bt_roleswap_state = ROLESWAP_STATE_IDLE;

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    link = os_queue_peek(&btm_db->link_list, 0);
                    while (link != NULL)
                    {
                        if (memcmp(link->bd_addr, peer_addr, 6))
                        {
                            bt_relay_roleswap_stop(link->bd_addr);
                        }

                        link = link->next;
                    }
                }

                payload.msg_buf = &info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_STATE, &payload);

                return;
            }

            link = bt_link_find(peer_addr);
            if (link != NULL)
            {
                control_conn_handle = link->acl_handle;
            }

            if (control_conn_handle == 0xFFFF)
            {
                BTM_PRINT_ERROR0("bt_relay_callback: control handle disconnected");
            }

            link = os_queue_peek(&btm_db->link_list, 0);
            while (link != NULL)
            {
                if (memcmp(link->bd_addr, peer_addr, 6))
                {
                    bt_roleswap_sync(link->bd_addr);

                    if (bt_relay_start_shadow_link(link->bd_addr, control_conn_handle))
                    {
                        bt_roleswap_state = ROLESWAP_STATE_LINK_SHADOW;
                    }
                }

                link = link->next;
            }
        }
        break;

    case GAP_BR_HANDOVER_CONN_CMPL_INFO:
        {
            T_GAP_HANDOVER_CONN_CMPL_INFO *info = (T_GAP_HANDOVER_CONN_CMPL_INFO *)buf;

            if (info->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: roleswap connection fail, cause 0x%04x", info->cause);
                return;
            }

            /* secondary starts roleswap */
            bt_roleswap_state = ROLESWAP_STATE_LINK_RESUME;

            if (info->link_type == 0 || info->link_type == 2) /* sco or esco */
            {
                T_ROLESWAP_DATA *data = bt_roleswap_data_find(info->bd_addr, ROLESWAP_TYPE_SCO);
                if (data != NULL)
                {
                    data->u.sco.handle = info->handle;
                }
            }
            else    /* acl */
            {
                T_ROLESWAP_DATA *data = bt_roleswap_data_find(info->bd_addr, ROLESWAP_TYPE_ACL);
                if (data != NULL)
                {
                    data->u.acl.handle = info->handle;
                    data->u.acl.encrypt_state = info->encrypt_enabled;
                }
            }
        }
        break;

    case GAP_BR_HANDOVER_CMPL_INFO:
        {
            T_GAP_HANDOVER_CMPL_INFO *info = (T_GAP_HANDOVER_CMPL_INFO *)buf;
            T_BT_MSG_PAYLOAD          payload;
            T_BT_LINK                *link;

            if (info->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: roleswap complete fail, cause 0x%04x", info->cause);

                bt_roleswap_state = ROLESWAP_STATE_IDLE;

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    uint8_t peer_addr[6];

                    remote_peer_addr_get(peer_addr);

                    link = os_queue_peek(&btm_db->link_list, 0);
                    while (link != NULL)
                    {
                        if (memcmp(link->bd_addr, peer_addr, 6))
                        {
                            bt_relay_roleswap_stop(link->bd_addr);
                        }

                        link = link->next;
                    }
                }

                payload.msg_buf = &info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_STATE, &payload);

                return;
            }

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                BTM_PRINT_TRACE0("bt_relay_callback: primary roleswap to secondary");

                remote_session_role_set(REMOTE_SESSION_ROLE_SECONDARY);
                bt_roleswap_state = ROLESWAP_STATE_ROLE_SWITCH;
            }
            else
            {
                BTM_PRINT_TRACE0("bt_relay_callback: secondary roleswap to primary");

                remote_session_role_set(REMOTE_SESSION_ROLE_PRIMARY);
                bt_roleswap_state = ROLESWAP_STATE_ADDR_SWITCH;
                bt_relay_set();
            }

            payload.msg_buf = &info->cause;
            bt_mgr_dispatch(BT_MSG_ROLESWAP_STATE, &payload);

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                uint8_t peer_addr[6];

                remote_peer_addr_get(peer_addr);

                /* TODO Audiolib should cache bud link type to distinguish BR from LE */
                link = os_queue_peek(&btm_db->link_list, 0);
                while (link != NULL)
                {
                    if (memcmp(link->bd_addr, peer_addr, 6))
                    {
                        bt_relay_roleswap_stop(link->bd_addr);
                    }

                    link = link->next;
                }

                gap_set_bd_addr(peer_addr);
            }
        }
        break;

    default:
        {
            uint16_t cause = *(uint16_t *)buf;

            if (cause)
            {
                BTM_PRINT_ERROR2("bt_relay_callback: msg %d error, cause 0x%04x", msg, cause);
            }
        }
        break;
    }
}

const T_BT_ROLESWAP_PROTO bt_relay_proto =
{
    .acl_status = bt_relay_handle_acl_status,
    .profile_conn = bt_relay_handle_profile_conn,
    .profile_disconn = bt_relay_handle_profile_disconn,
    .sco_disconn = bt_relay_handle_sco_disconn,
    .ctrl_conn = bt_relay_handle_ctrl_conn,
    .recv = bt_relay_recv,
    .cback = bt_relay_callback,
    .start = bt_relay_start_roleswap,
};
#endif
