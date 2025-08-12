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
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE2("bt_relay_pause_link: bd_addr %s, link %p",
                     TRACE_BDADDR(bd_addr), p_link);

    if (p_link != NULL)
    {

        if (gap_br_set_acl_arqn(p_link->acl_handle, GAP_ACL_ARQN_NACK) == GAP_CAUSE_SUCCESS)
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
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (gap_br_shadow_link(p_link->acl_handle, control_conn_handle,
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
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE3("bt_relay_start_shadow_info: bd_addr %s, link %p, control_handle 0x%04x",
                     TRACE_BDADDR(bd_addr), p_link, control_handle);

    if (p_link != NULL)
    {
        if (gap_br_shadow_pre_sync_info(p_link->acl_handle, control_handle, 0) == GAP_CAUSE_SUCCESS)
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

            if (bt_roleswap_get_spp_info(bd_addr, param, &spp_info) == false)
            {
                return;
            }

            bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_SPP, (uint8_t *)&spp_info, sizeof(spp_info));
            bt_roleswap_info_send(ROLESWAP_MODULE_SPP, ROLESWAP_SPP_CONN,
                                  (uint8_t *)&spp_info, sizeof(spp_info));

            bt_get_roleswap_rfc_info(bd_addr, spp_info.rfc_dlci, UUID_SERIAL_PORT);
        }
        break;

    case A2DP_PROFILE_MASK:
        {
            T_ROLESWAP_A2DP_INFO a2dp_info;

            if (bt_roleswap_get_a2dp_info(bd_addr, &a2dp_info) == false)
            {
                return;
            }

            bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_A2DP, (uint8_t *)&a2dp_info, sizeof(a2dp_info));
            bt_roleswap_info_send(ROLESWAP_MODULE_A2DP, ROLESWAP_A2DP_CONN,
                                  (uint8_t *)&a2dp_info, sizeof(a2dp_info));

            gap_br_get_handover_l2c_info(a2dp_info.sig_cid);
            gap_br_get_handover_l2c_info(a2dp_info.stream_cid);
        }
        break;

    case AVRCP_PROFILE_MASK:
        {
            T_ROLESWAP_AVRCP_INFO avrcp_info;

            if (bt_roleswap_get_avrcp_info(bd_addr, &avrcp_info) == false)
            {
                return;
            }

            bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_AVRCP, (uint8_t *)&avrcp_info, sizeof(avrcp_info));
            bt_roleswap_info_send(ROLESWAP_MODULE_AVRCP, ROLESWAP_AVRCP_CONN,
                                  (uint8_t *)&avrcp_info, sizeof(avrcp_info));

            gap_br_get_handover_l2c_info(avrcp_info.l2c_cid);
        }
        break;

    case HFP_PROFILE_MASK:
    case HSP_PROFILE_MASK:
        {
            T_ROLESWAP_HFP_INFO hfp_info;

            if (bt_roleswap_get_hfp_info(bd_addr, &hfp_info) == false)
            {
                return;
            }

            bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_HFP, (uint8_t *)&hfp_info, sizeof(hfp_info));
            bt_roleswap_info_send(ROLESWAP_MODULE_HFP, ROLESWAP_HFP_CONN,
                                  (uint8_t *)&hfp_info, sizeof(hfp_info));

            bt_get_roleswap_rfc_info(bd_addr, hfp_info.rfc_dlci, hfp_info.uuid);
        }
        break;

    case PBAP_PROFILE_MASK:
        {
            T_ROLESWAP_PBAP_INFO pbap_info;

            if (bt_roleswap_get_pbap_info(bd_addr, &pbap_info) == false)
            {
                return;
            }

            bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_PBAP, (uint8_t *)&pbap_info, sizeof(pbap_info));
            bt_roleswap_info_send(ROLESWAP_MODULE_PBAP, ROLESWAP_PBAP_CONN,
                                  (uint8_t *)&pbap_info, sizeof(pbap_info));

            if (pbap_info.obex_psm)
            {
                gap_br_get_handover_l2c_info(pbap_info.l2c_cid);
            }
            else
            {
                bt_get_roleswap_rfc_info(bd_addr, pbap_info.rfc_dlci, UUID_PBAP);
            }
        }
        break;

    case HID_DEVICE_PROFILE_MASK:
        {
            T_ROLESWAP_HID_DEVICE_INFO hid_device_info;

            if (bt_roleswap_get_hid_device_info(bd_addr, &hid_device_info) == false)
            {
                return;
            }

            bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_HID_DEVICE, (uint8_t *)&hid_device_info,
                                   sizeof(hid_info));
            bt_roleswap_info_send(ROLESWAP_MODULE_HID_DEVICE, ROLESWAP_HID_DEVICE_CONN,
                                  (uint8_t *)&hid_device_info, sizeof(hid_device_info));

            gap_br_get_handover_l2c_info(hid_device_info.control_cid);
            gap_br_get_handover_l2c_info(hid_device_info.interrupt_cid);
        }
        break;

    case HID_HOST_PROFILE_MASK:
        {
            T_ROLESWAP_HID_HOST_INFO hid_host_info;

            if (bt_roleswap_get_hid_host_info(bd_addr, &hid_host_info) == false)
            {
                return;
            }

            bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_HID_HOST, (uint8_t *)&hid_host_info,
                                   sizeof(hid_host_info));
            bt_roleswap_info_send(ROLESWAP_MODULE_HID_DEVICE, ROLESWAP_HID_HOST_CONN,
                                  (uint8_t *)&hid_host_info, sizeof(hid_host_info));

            gap_br_get_handover_l2c_info(hid_host_info.control_cid);
            gap_br_get_handover_l2c_info(hid_host_info.interrupt_cid);
        }
        break;

    default:
        break;
    }
}

void bt_relay_handle_profile_disconn(uint8_t                           bd_addr[6],
                                     T_ROLESWAP_PROFILE_DISCONN_PARAM *p_param)
{
    uint8_t buf[7];

    memcpy(buf, bd_addr, 6);

    switch (p_param->profile_mask)
    {
    case SPP_PROFILE_MASK:
        buf[6] = p_param->param;
        bt_roleswap_free_spp_info(bd_addr, p_param->param);
        bt_roleswap_info_send(ROLESWAP_MODULE_SPP, ROLESWAP_SPP_DISCONN, buf, 7);
        break;

    case A2DP_PROFILE_MASK:
        bt_roleswap_free_a2dp_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_A2DP, ROLESWAP_A2DP_DISCONN, buf, 6);
        break;

    case AVRCP_PROFILE_MASK:
        bt_roleswap_free_avrcp_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_AVRCP, ROLESWAP_AVRCP_DISCONN, buf, 6);
        break;

    case HFP_PROFILE_MASK:
    case HSP_PROFILE_MASK:
        bt_roleswap_free_hfp_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_HFP, ROLESWAP_HFP_DISCONN, buf, 6);
        break;

    case PBAP_PROFILE_MASK:
        bt_roleswap_free_pbap_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_PBAP, ROLESWAP_PBAP_DISCONN, buf, 6);
        break;

    case HID_DEVICE_PROFILE_MASK:
        bt_roleswap_free_hid_device_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_HID_DEVICE, ROLESWAP_HID_DEVICE_DISCONN, buf, 6);
        break;

    case HID_HOST_PROFILE_MASK:
        bt_roleswap_free_hid_host_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_HID_HOST, ROLESWAP_HID_HOST_DISCONN, buf, 6);
        break;


    default:
        break;
    }
}

void bt_relay_handle_bud_role_switch(uint8_t bd_addr[6])
{
    T_GAP_HANDOVER_BUD_INFO info;
    T_REMOTE_SESSION_ROLE session_role;
    uint8_t peer_addr[6];
    uint8_t local_addr[6];

    remote_peer_addr_get(peer_addr);
    remote_local_addr_get(local_addr);
    session_role = remote_session_role_get();

    BTM_PRINT_TRACE5("bt_relay_handle_bud_role_switch: state %u, role %u, remote addr %s, "
                     "peer addr %s, local addr %s",
                     bt_roleswap_state, session_role, TRACE_BDADDR(bd_addr),
                     TRACE_BDADDR(peer_addr), TRACE_BDADDR(local_addr));

    if (bt_roleswap_state != ROLESWAP_STATE_IDLE)
    {
        if (session_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            bt_roleswap_state = ROLESWAP_STATE_IDLE;

            /* Pseudo secondary may have not changed its bdaddr, while pseudo primary
             * still updates it. Discard pseudo secondary's bd_addr.
             */
            memcpy(info.pre_bd_addr, peer_addr, 6);
            memcpy(info.curr_bd_addr, local_addr, 6);
            gap_br_set_handover_bud_info(&info);

            /* update bud link database */
            memcpy(btm_db.remote_link->bd_addr, info.curr_bd_addr, 6);
            remote_peer_addr_set(info.curr_bd_addr);
            remote_local_addr_set(info.pre_bd_addr);

            /* update bud profile database */
            rdtp_handle_roleswap(info.pre_bd_addr, info.curr_bd_addr);

            {
                T_BT_MSG_PAYLOAD payload;

                payload.msg_buf = &info;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_ADDR_STATUS, &payload);
            }
        }
        else if (session_role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            bt_roleswap_state = ROLESWAP_STATE_ADDR_SWITCH;

            /* bd_addr should be equal with peer_addr */
            memcpy(info.pre_bd_addr, peer_addr, 6);
            memcpy(info.curr_bd_addr, local_addr, 6);
            gap_br_set_handover_bud_info(&info);

            /* update bud link database */
            memcpy(btm_db.remote_link->bd_addr, info.curr_bd_addr, 6);
            remote_peer_addr_set(info.curr_bd_addr);
            remote_local_addr_set(info.pre_bd_addr);

            /* update bud profile database */
            rdtp_handle_roleswap(info.pre_bd_addr, info.curr_bd_addr);

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
        T_ROLESWAP_DATA *p_data;

        switch (msg)
        {
        case BT_MSG_ACL_CONN_READY:
            {
                T_ROLESWAP_INFO *p_base;

                p_base = bt_find_roleswap_info_base(bd_addr);
                if (p_base)
                {
                    BTM_PRINT_ERROR1("bt_relay_handle_acl_status: roleswap info base already exists %s",
                                     TRACE_BDADDR(bd_addr));
                    return;
                }

                p_base = bt_alloc_roleswap_info_base(bd_addr);
                if (p_base == NULL)
                {
                    BTM_PRINT_ERROR1("bt_relay_handle_acl_status: fail to alloc roleswap base %s",
                                     TRACE_BDADDR(bd_addr));
                    return;
                }

                p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
                if (p_data == NULL)
                {
                    gap_br_get_handover_acl_info(bd_addr);
                }
            }
            break;

        case BT_MSG_ACL_CONN_ACTIVE:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data != NULL)
            {
                p_data->u.acl.mode = 0;
                BTM_PRINT_TRACE1("bt_relay_handle_acl_status: mode %u", p_data->u.acl.mode);
            }
            break;

        case BT_MSG_ACL_CONN_SNIFF:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.mode = 2;
                BTM_PRINT_TRACE1("bt_relay_handle_acl_status: mode %u", p_data->u.acl.mode);
            }
            break;

        case BT_MSG_ACL_AUTHEN_SUCCESS:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.authen_state = true;
                BTM_PRINT_TRACE1("bt_relay_handle_acl_status: authen_state %u", p_data->u.acl.authen_state);
            }
            break;

        case BT_MSG_ACL_AUTHEN_FAIL:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.authen_state = false;
                BTM_PRINT_TRACE1("bt_relay_handle_acl_status: authen_state %u", p_data->u.acl.authen_state);

                gap_br_get_handover_sm_info(p_data->u.acl.bd_addr);
            }
            break;

        case BT_MSG_ACL_CONN_ENCRYPTED:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.encrypt_state = 1;
                BTM_PRINT_TRACE1("bt_relay_handle_acl_status: encrypt_state %u", p_data->u.acl.encrypt_state);

                gap_br_get_handover_sm_info(p_data->u.acl.bd_addr);
            }
            break;

        case BT_MSG_ACL_CONN_NOT_ENCRYPTED:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.encrypt_state = 0;
                BTM_PRINT_TRACE1("bt_relay_handle_acl_status: encrypt_state %u", p_data->u.acl.encrypt_state);

                gap_br_get_handover_sm_info(p_data->u.acl.bd_addr);
            }
            break;

        case BT_MSG_ACL_ROLE_MASTER:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.role = 0;
                BTM_PRINT_TRACE1("bt_relay_handle_acl_status: role %u", p_data->u.acl.role);
            }
            break;

        case BT_MSG_ACL_ROLE_SLAVE:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.role = 1;
                BTM_PRINT_TRACE1("bt_relay_handle_acl_status: role %u", p_data->u.acl.role);
            }
            break;

        case BT_MSG_ACL_CONN_DISCONN:
            BTM_PRINT_TRACE0("bt_relay_handle_acl_status: disconnected");

            uint16_t cause = *(uint16_t *)buf;

            if (cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
            {
                /* acl disconnect will be received duing shadow link */
                return;
            }

            bt_roleswap_free_acl_info(bd_addr);
            bt_roleswap_info_send(ROLESWAP_MODULE_ACL, ROLESWAP_ACL_DISCONN, bd_addr, 6);
            break;

        case BT_MSG_ACL_LINK_KEY_INFO:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                gap_br_get_handover_sm_info(p_data->u.acl.bd_addr);
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
                bt_roleswap_free_acl_info(bd_addr);
            }
        }
        else if (msg == BT_MSG_ACL_CONN_ACTIVE)
        {
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
                bt_roleswap_state == ROLESWAP_STATE_LINK_SNIFF)
            {
                uint8_t  i;
                uint16_t control_handle = 0xFFFF;

                for (i = 0; i < btm_db.br_link_num; i++)
                {
                    if (btm_db.br_link[i].connected_profile & RDTP_PROFILE_MASK)
                    {
                        control_handle = btm_db.br_link[i].acl_handle;
                        break;
                    }
                }

                for (i = 0; i < btm_db.br_link_num; i++)
                {
                    if (btm_db.br_link[i].connected_profile & ~RDTP_PROFILE_MASK)
                    {
                        bt_relay_start_shadow_info(btm_db.br_link[i].bd_addr, control_handle);
                    }
                }
            }
        }
    }
}

void bt_relay_handle_role_switch_fail(uint8_t  bd_addr[6],
                                      uint16_t cause)
{
}

void bt_relay_handle_sco_disconn(uint8_t  bd_addr[6],
                                 uint16_t cause)
{
    T_ROLESWAP_DATA *p_data;

    if (cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
    {
        /* sco disconnect will be received duing shadow link */
        return;
    }

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_SCO);
    if (p_data == NULL)
    {
        return;
    }

    bt_roleswap_free_info(bd_addr, p_data);
    bt_roleswap_info_send(ROLESWAP_MODULE_SCO, ROLESWAP_SCO_DISCONN, bd_addr, 6);
}

void bt_relay_handle_ctrl_conn(void)
{
    uint8_t i;

    /* bud link is created after source/primary link. */
    for (i = 0; i < btm_db.br_link_num; i++)
    {
        BTM_PRINT_TRACE3("bt_relay_handle_ctrl_conn: link %d, state %d, profile 0x%02x",
                         i, btm_db.br_link[i].acl_link_state,
                         btm_db.br_link[i].connected_profile);

        if ((btm_db.br_link[i].acl_link_state == BT_LINK_STATE_CONNECTED) &&
            ((btm_db.br_link[i].connected_profile & RDTP_PROFILE_MASK) == 0))
        {
            bt_roleswap_transfer(btm_db.br_link[i].bd_addr);
        }
    }
}

bool bt_relay_start_roleswap(uint8_t                      bd_addr[6],
                             uint8_t                      context,
                             bool                         stop_after_shadow,
                             P_REMOTE_ROLESWAP_SYNC_CBACK cback)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE2("bt_relay_start_roleswap: bd_addr %s, link %p",
                     TRACE_BDADDR(bd_addr), p_link);

    if (p_link != NULL)
    {
        T_BT_BR_LINK *remote_link;
        uint8_t peer_addr[6];

        remote_peer_addr_get(peer_addr);

        remote_link = bt_find_br_link(peer_addr);
        if (remote_link != NULL)
        {
            if (bt_sniff_mode_exit(remote_link, true) == true)
            {
                return bt_relay_start_shadow_info(p_link->bd_addr, remote_link->acl_handle);
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
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE2("bt_relay_roleswap_stop: bd_addr %s, link %p",
                     TRACE_BDADDR(bd_addr), p_link);

    if (p_link != NULL)
    {
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            gap_br_set_acl_arqn(p_link->acl_handle, GAP_ACL_ARQN_ACK);
        }

        return true;
    }

    return false;
}

void bt_relay_recv_acl_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *p_info)
{
    T_ROLESWAP_INFO *p_base;
    T_ROLESWAP_DATA *p_data;

    switch (submodule)
    {
    case ROLESWAP_ACL_CONN:
        {
            T_ROLESWAP_ACL_INFO *p;

            p = (T_ROLESWAP_ACL_INFO *)p_info;
            p_base = bt_find_roleswap_info_base(p->bd_addr);
            if (p_base)
            {
                BTM_PRINT_ERROR1("bt_relay_recv_acl_info: roleswap info base already exists %s",
                                 TRACE_BDADDR(p->bd_addr));
                return;
            }

            p_base = bt_alloc_roleswap_info_base(p->bd_addr);
            if (p_base)
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_ACL, p_info, len);
            }
        }
        break;

    case ROLESWAP_ACL_DISCONN:
        BTM_PRINT_TRACE1("bt_relay_recv_acl_info: disconn %s", TRACE_BDADDR(p_info));
        bt_roleswap_free_acl_info(p_info);
        break;

    case ROLESWAP_ACL_UPDATE:
        {
            T_ROLESWAP_ACL_TRANSACT *p_transact;

            p_transact = (T_ROLESWAP_ACL_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.role = p_transact->role;
                p_data->u.acl.mode = p_transact->mode;
                p_data->u.acl.link_policy = p_transact->link_policy;
                p_data->u.acl.superv_tout = p_transact->superv_tout;
                p_data->u.acl.authen_state = p_transact->authen_state;
                p_data->u.acl.encrypt_state = p_transact->encrypt_state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_sco_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *p_info)
{
    T_ROLESWAP_DATA *p_data;

    switch (submodule)
    {
    case ROLESWAP_SCO_CONN:
        {
            T_ROLESWAP_SCO_INFO *p;

            p = (T_ROLESWAP_SCO_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_SCO, p_info, len);
        }
        break;
    case ROLESWAP_SCO_DISCONN:
        {
            p_data = bt_find_roleswap_data(p_info, ROLESWAP_TYPE_SCO);
            if (p_data)
            {
                bt_roleswap_free_info(p_info, p_data);
            }
        }
        break;
    default:
        break;
    }
}

void bt_relay_recv_sm_info(uint8_t   submodule,
                           uint16_t  len,
                           uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_SM_CONN:
        {
            T_ROLESWAP_SM_INFO *p;

            p = (T_ROLESWAP_SM_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_SM, p_info, len);
        }
        break;

    case ROLESWAP_SM_UPDATE:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_SM_INFO *p;

            p = (T_ROLESWAP_SM_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_SM);
            if (p_data)
            {
                memcpy((uint8_t *)&p_data->u.sm, (uint8_t *)p_info, sizeof(T_ROLESWAP_SM_INFO));
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_l2c_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_L2C_CONN:
        {
            T_ROLESWAP_L2C_INFO *p;

            p = (T_ROLESWAP_L2C_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_L2C, p_info, len);
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_rfc_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_RFC_CTRL_CONN:
        {
            T_ROLESWAP_RFC_CTRL_INFO *p;

            p = (T_ROLESWAP_RFC_CTRL_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_RFC_CTRL, p_info, len);
        }
        break;

    case ROLESWAP_RFC_DATA_CONN:
        {
            T_ROLESWAP_RFC_DATA_INFO *p;

            p = (T_ROLESWAP_RFC_DATA_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_RFC_DATA, p_info, len);
        }
        break;

    case ROLESWAP_RFC_DATA_TRANSACT:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_RFC_TRANSACT *p_transact;

            p_transact = (T_ROLESWAP_RFC_TRANSACT *)p_info;
            p_data = bt_find_roleswap_rfc_data(p_transact->bd_addr, p_transact->dlci);
            if (p_data == NULL)
            {
                return;
            }

            p_data->u.rfc_data.given_credits = p_transact->given_credits;
            p_data->u.rfc_data.remote_remain_credits = p_transact->remote_remain_credits;
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_spp_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_SPP_CONN:
        {
            T_ROLESWAP_SPP_INFO *p;

            p = (T_ROLESWAP_SPP_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_SPP, p_info, len);
        }
        break;

    case ROLESWAP_SPP_DISCONN:
        {
            uint8_t bd_addr[6];
            uint8_t local_server_chann;

            memcpy(bd_addr, p_info, 6);
            local_server_chann = *(p_info + 6);
            bt_roleswap_free_spp_info(bd_addr, local_server_chann);
        }
        break;

    case ROLESWAP_SPP_TRANSACT:
        {
            T_ROLESWAP_SPP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_SPP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_spp(p_transact->bd_addr,  p_transact->local_server_chann);
            if (p_data)
            {
                p_data->u.spp.state = p_transact->state;
                p_data->u.spp.remote_credit = p_transact->remote_credit;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_a2dp_info(uint8_t   submodule,
                             uint16_t  len,
                             uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_A2DP_CONN:
        {
            T_ROLESWAP_A2DP_INFO *p;

            p = (T_ROLESWAP_A2DP_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_A2DP, p_info, len);
        }
        break;

    case ROLESWAP_A2DP_DISCONN:
        bt_roleswap_free_a2dp_info(p_info);
        break;

    case ROLESWAP_A2DP_TRANSACT:
        {
            T_ROLESWAP_A2DP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_A2DP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_A2DP);
            if (p_data)
            {
                p_data->u.a2dp.sig_state = p_transact->sig_state;
                p_data->u.a2dp.state = p_transact->state;
                p_data->u.a2dp.tx_trans_label = p_transact->tx_trans_label;
                p_data->u.a2dp.rx_start_trans_label = p_transact->rx_start_trans_label;
                p_data->u.a2dp.cmd_flag = p_transact->cmd_flag;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_avrcp_info(uint8_t   submodule,
                              uint16_t  len,
                              uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_AVRCP_CONN:
        {
            T_ROLESWAP_AVRCP_INFO *p;

            p = (T_ROLESWAP_AVRCP_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_AVRCP, p_info, len);
        }
        break;

    case ROLESWAP_AVRCP_DISCONN:
        bt_roleswap_free_avrcp_info(p_info);
        break;

    case ROLESWAP_AVRCP_TRANSACT:
        {
            T_ROLESWAP_AVRCP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_AVRCP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_AVRCP);
            if (p_data)
            {
                p_data->u.avrcp.avctp_state = p_transact->avctp_state;
                p_data->u.avrcp.state = p_transact->state;
                p_data->u.avrcp.play_status = p_transact->play_status;
                p_data->u.avrcp.cmd_credits = p_transact->cmd_credits;
                p_data->u.avrcp.transact_label = p_transact->transact_label;
                p_data->u.avrcp.vol_change_registered = p_transact->vol_change_registered;
                p_data->u.avrcp.vol_change_pending_transact = p_transact->vol_change_pending_transact;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_hfp_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_HFP_CONN:
        {
            T_ROLESWAP_HFP_INFO *p;

            p = (T_ROLESWAP_HFP_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_HFP, p_info, len);
        }
        break;

    case ROLESWAP_HFP_DISCONN:
        bt_roleswap_free_hfp_info(p_info);
        break;

    case ROLESWAP_HFP_TRANSACT:
        {
            T_ROLESWAP_HFP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_HFP_TRANSACT *)p_info;

            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_HFP);
            if (p_data)
            {
                p_data->u.hfp.state = p_transact->state;
                p_data->u.hfp.bat_report_type = p_transact->bat_report_type;
                p_data->u.hfp.at_cmd_credits = p_transact->at_cmd_credits;
                p_data->u.hfp.call_status = p_transact->call_status;
                p_data->u.hfp.prev_call_status = p_transact->prev_call_status;
                p_data->u.hfp.service_status = p_transact->service_status;
                p_data->u.hfp.supported_features = p_transact->supported_features;
                p_data->u.hfp.codec_type = p_transact->codec_type;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_pbap_info(uint8_t   submodule,
                             uint16_t  len,
                             uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_PBAP_CONN:
        {
            T_ROLESWAP_PBAP_INFO *p;

            p = (T_ROLESWAP_PBAP_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_PBAP, p_info, len);
        }
        break;

    case ROLESWAP_PBAP_DISCONN:
        bt_roleswap_free_pbap_info(p_info);
        break;

    case ROLESWAP_PBAP_TRANSACT:
        {
            T_ROLESWAP_PBAP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_PBAP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_PBAP);
            if (p_data)
            {
                p_data->u.pbap.obex_state = p_transact->obex_state;
                p_data->u.pbap.pbap_state = p_transact->pbap_state;
                p_data->u.pbap.path = p_transact->path;
                p_data->u.pbap.repos = p_transact->repos;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_hid_device_info(uint8_t   submodule,
                                   uint16_t  len,
                                   uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_HID_DEVICE_CONN:
        {
            T_ROLESWAP_HID_DEVICE_INFO *p;

            p = (T_ROLESWAP_HID_DEVICE_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_HID_DEVICE, p_info, len);
        }
        break;

    case ROLESWAP_HID_DEVICE_DISCONN:
        bt_roleswap_free_hid_device_info(p_info);
        break;

    case ROLESWAP_HID_DEVICE_TRANSACT:
        {
            T_ROLESWAP_HID_DEVICE_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_HID_DEVICE_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_HID_DEVICE);
            if (p_data)
            {
                p_data->u.hid.proto_mode = p_transact->proto_mode;
                p_data->u.hid.control_state = p_transact->control_state;
                p_data->u.hid.interrupt_state = p_transact->interrupt_state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_hid_host_info(uint8_t submodule, uint16_t len, uint8_t *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_HID_HOST_CONN:
        {
            T_ROLESWAP_HID_HOST_INFO *p;

            p = (T_ROLESWAP_HID_HOST_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_HID_HOST, p_info, len);
        }
        break;

    case ROLESWAP_HID_HOST_DISCONN:
        bt_roleswap_free_hid_host_info(p_info);
        break;

    case ROLESWAP_HID_HOST_TRANSACT:
        {
            T_ROLESWAP_HID_HOST_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_HID_HOST_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_HID_HOST);
            if (p_data)
            {
                p_data->u.hid_host.proto_mode = p_transact->proto_mode;
                p_data->u.hid_host.control_state = p_transact->control_state;
                p_data->u.hid_host.interrupt_state = p_transact->interrupt_state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_avp_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_AVP_CONTROL_CONN:
        {
            p = (T_ROLESWAP_AVP_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_AVP);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.avp, p_info, sizeof(T_ROLESWAP_AVP_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_AVP, p_info, len);
            }
        }
        break;

    case ROLESWAP_AVP_CONTROL_DISCONN:
        bt_roleswap_free_avp_control_info(p_info);
        break;

    case ROLESWAP_AVP_AUDIO_CONN:
        {
            p = (T_ROLESWAP_AVP_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_AVP);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.avp, p_info, sizeof(T_ROLESWAP_AVP_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_AVP, p_info, len);
            }
        }
        break;

    case ROLESWAP_AVP_AUDIO_DISCONN:
        bt_roleswap_free_avp_audio_info(p_info);
        break;

    case ROLESWAP_AVP_TRANSACT:
        {
            T_ROLESWAP_AVP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_AVP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_AVP);
            if (p_data)
            {
                p_data->u.avp.control_state = p_transact->control_state;
                p_data->u.avp.audio_state = p_transact->audio_state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv_att_info(uint8_t   submodule,
                            uint16_t  len,
                            uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_ATT_CONN:
        {
            T_ROLESWAP_ATT_INFO *p;

            p = (T_ROLESWAP_ATT_INFO *)p_info;
            bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_ATT, p_info, len);
        }
        break;

    case ROLESWAP_ATT_DISCONN:
        bt_roleswap_free_att_info(p_info);
        break;

    case ROLESWAP_ATT_TRANSACT:
        {
            T_ROLESWAP_ATT_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_ATT_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_ATT);
            if (p_data)
            {
                p_data->u.att.state = p_transact->state;
            }
        }
        break;

    default:
        break;
    }
}

void bt_relay_recv(uint8_t  *p_data,
                   uint16_t  data_len)
{
    uint8_t  *p;
    uint8_t   module;
    uint8_t   submodule;
    uint16_t  len;

    p = p_data;

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

void bt_relay_set_info(T_ROLESWAP_INFO *p_base)
{
    uint16_t type;
    T_ROLESWAP_DATA *p_data;
    T_BT_MSG_PAYLOAD payload;

    for (type = ROLESWAP_TYPE_ACL; type < ROLESWAP_TYPE_ALL; type++)
    {
        p_data = (T_ROLESWAP_DATA *)p_base->info_list.p_first;

        while (p_data)
        {
            if (p_data->type == type)
            {
                switch (p_data->type)
                {
                case ROLESWAP_TYPE_ACL:
                    {
                        T_ROLESWAP_ACL_INFO *p_info = &p_data->u.acl;

                        bt_roleswap_set_acl_info(p_info);

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = p_info;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_ACL_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_SCO:
                    {
                        T_ROLESWAP_SCO_INFO *p_info = &p_data->u.sco;

                        bt_roleswap_set_sco_info(p_base->bd_addr, p_info);

                        payload.msg_buf = p_info;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_SCO_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_SM:
                    {
                        T_GAP_HANDOVER_SM_INFO sm;
                        T_ROLESWAP_SM_INFO *p_info = &p_data->u.sm;

                        sm.mode = p_info->mode;
                        sm.state = p_info->state;
                        sm.sec_state = p_info->sec_state;
                        sm.remote_authen = p_info->remote_authen;
                        sm.remote_io = p_info->remote_io;
                        memcpy(sm.bd_addr, p_info->bd_addr, 6);
                        gap_br_set_handover_sm_info(&sm);
                    }
                    break;

                case ROLESWAP_TYPE_L2C:
                    {
                        T_GAP_HANDOVER_L2C_INFO l2c;
                        T_ROLESWAP_L2C_INFO *p_info = &p_data->u.l2c;

                        l2c.local_cid = p_info->local_cid;
                        l2c.remote_cid = p_info->remote_cid;
                        l2c.local_mtu = p_info->local_mtu;
                        l2c.remote_mtu = p_info->remote_mtu;
                        l2c.local_mps = p_info->local_mps;
                        l2c.remote_mps = p_info->remote_mps;
                        l2c.psm = p_info->psm;
                        l2c.role = p_info->role;
                        l2c.mode = p_info->mode;
                        memcpy(l2c.bd_addr, p_info->bd_addr, 6);
                        gap_br_set_handover_l2c_info(&l2c);
                    }
                    break;

                case ROLESWAP_TYPE_RFC_CTRL:
                    bt_roleswap_set_rfc_ctrl_info(p_base->bd_addr, &p_data->u.rfc_ctrl);
                    break;

                case ROLESWAP_TYPE_RFC_DATA:
                    bt_roleswap_set_rfc_data_info(p_base->bd_addr, &p_data->u.rfc_data);
                    break;

                case ROLESWAP_TYPE_SPP:
                    {
                        bt_roleswap_set_spp_info(p_base->bd_addr, &p_data->u.spp);

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.spp;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_SPP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_A2DP:
                    {
                        bt_roleswap_set_a2dp_info(p_base->bd_addr, &p_data->u.a2dp);

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.a2dp;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_A2DP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_AVRCP:
                    {
                        bt_roleswap_set_avrcp_info(p_base->bd_addr, &p_data->u.avrcp);

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.avrcp;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_AVRCP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_HFP:
                    {
                        bt_roleswap_set_hfp_info(p_base->bd_addr, &p_data->u.hfp);

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.hfp;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_HFP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_PBAP:
                    {
                        bt_roleswap_set_pbap_info(p_base->bd_addr, &p_data->u.pbap);

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = NULL;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_PBAP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_HID_DEVICE:
                    {
                        bt_roleswap_set_hid_device_info(p_base->bd_addr, &p_data->u.hid_device);

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.hid_device;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_HID_DEVICE_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_HID_HOST:
                    {
                        bt_roleswap_set_hid_host_info(p_base->bd_addr, &p_data->u.hid_host);

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.hid_host;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_HID_HOST_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_IAP:
                    {
                        bt_roleswap_set_iap_info(p_base->bd_addr, &p_data->u.iap);

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = NULL;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_IAP_STATUS, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_AVP:
                    {
                        bt_roleswap_set_avp_info(p_base->bd_addr, &p_data->u.avp);
                    }
                    break;

                case ROLESWAP_TYPE_ATT:
                    {
                        bt_roleswap_set_att_info(p_base->bd_addr, &p_data->u.att);
                    }
                    break;

                default:
                    break;
                }
            }

            p_data = p_data->p_next;
        }
    }
}

void bt_relay_set(void)
{
    uint8_t i;
    T_ROLESWAP_INFO *p_info;

    for (i = 0; i < btm_db.br_link_num; i++)
    {
        p_info = &btm_db.roleswap_info[i];

        if (p_info->used)
        {
            bt_relay_set_info(p_info);
        }
    }
}

void bt_relay_callback(void                       *p_buf,
                       T_GAP_BR_HANDOVER_MSG_TYPE  msg)
{
    BTM_PRINT_TRACE1("bt_relay_callback: message 0x%02x", msg);

    switch (msg)
    {
    case GAP_BR_GET_ACL_INFO_RSP:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_ACL_INFO acl;
            T_GAP_HANDOVER_ACL_INFO *p_info = (T_GAP_HANDOVER_ACL_INFO *)p_buf;
            T_BT_BR_LINK *p_link;

            p_link = bt_find_br_link(p_info->bd_addr);
            if ((p_link == NULL) || (p_link->acl_link_state != BT_LINK_STATE_CONNECTED))
            {
                break;
            }

            acl.authen_state = p_link->acl_link_authenticated;

            acl.bd_type = p_info->bd_type;
            acl.conn_type = p_info->conn_type;
            acl.encrypt_state = p_info->encrypt_state;
            acl.handle = p_info->handle;
            acl.link_policy = p_info->link_policy;
            acl.mode = p_info->mode;
            acl.role = p_info->role;
            acl.superv_tout = p_info->superv_tout;
            memcpy(acl.bd_addr, p_info->bd_addr, 6);

            p_data = bt_find_roleswap_data(p_info->bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data != NULL)
            {
                memcpy((uint8_t *)&p_data->u.acl, (uint8_t *)&acl, sizeof(T_ROLESWAP_ACL_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p_info->bd_addr, ROLESWAP_TYPE_ACL, (uint8_t *)&acl,
                                       sizeof(T_ROLESWAP_ACL_INFO));
            }

            bt_roleswap_info_send(ROLESWAP_MODULE_ACL, ROLESWAP_ACL_CONN,
                                  (uint8_t *)&acl, sizeof(T_ROLESWAP_ACL_INFO));
        }
        break;

    case GAP_BR_GET_SCO_INFO_RSP:
        {
            T_ROLESWAP_SCO_INFO sco;
            T_GAP_HANDOVER_SCO_INFO *p_info = (T_GAP_HANDOVER_SCO_INFO *)p_buf;

            sco.handle = p_info->handle;
            sco.type = p_info->type;
            memcpy(sco.bd_addr, p_info->bd_addr, 6);
            bt_roleswap_get_sco_info(p_info->bd_addr, &sco);

            bt_roleswap_alloc_info(p_info->bd_addr, ROLESWAP_TYPE_SCO, (uint8_t *)&sco,
                                   sizeof(T_ROLESWAP_SCO_INFO));
            bt_roleswap_info_send(ROLESWAP_MODULE_SCO, ROLESWAP_SCO_CONN, (uint8_t *)&sco,
                                  sizeof(T_ROLESWAP_SCO_INFO));
        }
        break;

    case GAP_BR_GET_SM_INFO_RSP:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_SM_INFO sm;
            T_GAP_HANDOVER_SM_INFO *p_info = (T_GAP_HANDOVER_SM_INFO *)p_buf;

            sm.mode = p_info->mode;
            sm.state = p_info->state;
            sm.sec_state = p_info->sec_state;
            sm.remote_authen = p_info->remote_authen;
            sm.remote_io = p_info->remote_io;
            memcpy(sm.bd_addr, p_info->bd_addr, 6);

            p_data = bt_find_roleswap_data(p_info->bd_addr, ROLESWAP_TYPE_SM);
            if (p_data)
            {
                memcpy((uint8_t *)&p_data->u.sm, (uint8_t *)&sm, sizeof(T_ROLESWAP_SM_INFO));
                bt_roleswap_info_send(ROLESWAP_MODULE_SM, ROLESWAP_SM_UPDATE, (uint8_t *)&sm,
                                      sizeof(T_ROLESWAP_SM_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p_info->bd_addr, ROLESWAP_TYPE_SM, (uint8_t *)&sm,
                                       sizeof(T_ROLESWAP_SM_INFO));
                bt_roleswap_info_send(ROLESWAP_MODULE_SM, ROLESWAP_SM_CONN, (uint8_t *)&sm,
                                      sizeof(T_ROLESWAP_SM_INFO));
            }
        }
        break;

    case GAP_BR_GET_L2C_INFO_RSP:
        {
            T_ROLESWAP_L2C_INFO l2c;
            T_GAP_HANDOVER_L2C_INFO *p_info = (T_GAP_HANDOVER_L2C_INFO *)p_buf;

            l2c.local_cid = p_info->local_cid;
            l2c.remote_cid = p_info->remote_cid;
            l2c.local_mtu = p_info->local_mtu;
            l2c.remote_mtu = p_info->remote_mtu;
            l2c.local_mps = p_info->local_mps;
            l2c.remote_mps = p_info->remote_mps;
            l2c.psm = p_info->psm;
            l2c.role = p_info->role;
            l2c.mode = p_info->mode;
            memcpy(l2c.bd_addr, p_info->bd_addr, 6);

            bt_roleswap_alloc_info(p_info->bd_addr, ROLESWAP_TYPE_L2C, (uint8_t *)&l2c,
                                   sizeof(T_ROLESWAP_L2C_INFO));
            bt_roleswap_info_send(ROLESWAP_MODULE_L2C, ROLESWAP_L2C_CONN, (uint8_t *)&l2c,
                                  sizeof(T_ROLESWAP_L2C_INFO));
        }
        break;

    case SET_BD_ADDR_RSP:
        {
            T_GAP_SET_BD_ADDR_RSP *p_rsp = (T_GAP_SET_BD_ADDR_RSP *)p_buf;
            uint8_t peer_addr[6];
            uint8_t local_addr[6];

            remote_peer_addr_get(peer_addr);
            remote_local_addr_get(local_addr);

            BTM_PRINT_TRACE5("bt_relay_callback: set bd addr rsp, cause 0x%04x, state %u, "
                             "role %u, peer addr %s, local addr %s",
                             p_rsp->cause, bt_roleswap_state, remote_session_role_get(),
                             TRACE_BDADDR(peer_addr), TRACE_BDADDR(local_addr));

            if (bt_roleswap_state == ROLESWAP_STATE_IDLE)
            {
                T_BT_MSG_PAYLOAD payload;

                memcpy(payload.bd_addr, p_rsp->bd_addr, 6);
                payload.msg_buf = &p_rsp->cause;
                bt_mgr_dispatch(BT_MSG_GAP_LOCAL_ADDR_CHANGED, &payload);
            }
            else
            {
                if (p_rsp->cause)
                {
                    /* TODO shall report to APP? */
                    return;
                }

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
                    bt_roleswap_state = ROLESWAP_STATE_IDLE;

                    {
                        T_BT_MSG_PAYLOAD payload;
                        T_GAP_HANDOVER_BUD_INFO info;

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
            T_GAP_SHADOW_PRE_SYNC_INFO_RSP *p_rsp = (T_GAP_SHADOW_PRE_SYNC_INFO_RSP *)p_buf;
            uint8_t i;

            if (p_rsp->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: shadow info failed 0x%04x", p_rsp->cause);
                bt_roleswap_state = ROLESWAP_STATE_IDLE;
                return;
            }

            for (i = 0; i < btm_db.br_link_num; i++)
            {
                if (btm_db.br_link[i].connected_profile & ~RDTP_PROFILE_MASK)
                {
                    bt_relay_pause_link(btm_db.br_link[i].bd_addr);
                }
            }
        }
        break;

    case GAP_BR_SET_ACL_ACTIVE_STATE_RSP:
        {
            T_GAP_SET_ACL_ACTIVE_STATE_RSP *p_rsp = (T_GAP_SET_ACL_ACTIVE_STATE_RSP *)p_buf;
            if (p_rsp->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: set acl active state fail, cause 0x%04x", p_rsp->cause);
                bt_roleswap_state = ROLESWAP_STATE_IDLE;
            }
        }
        break;

    case GAP_BR_SHADOW_LINK_RSP:
        {
            T_GAP_SHADOW_LINK_RSP *p_rsp = (T_GAP_SHADOW_LINK_RSP *)p_buf;
            T_BT_MSG_PAYLOAD payload;
            uint8_t i;

            if (p_rsp->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: shadow link fail, cause 0x%04x", p_rsp->cause);

                bt_roleswap_state = ROLESWAP_STATE_IDLE;

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    for (i = 0; i < btm_db.br_link_num; i++)
                    {
                        if (btm_db.br_link[i].connected_profile & ~RDTP_PROFILE_MASK)
                        {
                            bt_relay_roleswap_stop(btm_db.br_link[i].bd_addr);
                        }
                    }
                }

                payload.msg_buf = &p_rsp->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case GAP_BR_ACL_SUSPEND_RX_EMPTY_INFO:
        {
            T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *p_info = (T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *)p_buf;
            T_BT_MSG_PAYLOAD payload;
            uint16_t control_conn_handle = 0xFFFF;
            uint8_t i;

            if (p_info->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: acl suspend fail, cause 0x%04x", p_info->cause);

                bt_roleswap_state = ROLESWAP_STATE_IDLE;

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    for (i = 0; i < btm_db.br_link_num; i++)
                    {
                        if (btm_db.br_link[i].connected_profile & ~RDTP_PROFILE_MASK)
                        {
                            bt_relay_roleswap_stop(btm_db.br_link[i].bd_addr);
                        }
                    }
                }

                payload.msg_buf = &p_info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_STATE, &payload);

                return;
            }

            for (i = 0; i < btm_db.br_link_num; i++)
            {
                if (btm_db.br_link[i].connected_profile & RDTP_PROFILE_MASK)
                {
                    control_conn_handle = btm_db.br_link[i].acl_handle;
                    break;
                }
            }

            if (control_conn_handle == 0xFFFF)
            {
                BTM_PRINT_ERROR0("bt_relay_callback: control handle disconnected");
            }

            for (i = 0; i < btm_db.br_link_num; i++)
            {
                BTM_PRINT_TRACE2("bt_relay_callback: link %d connected profile 0x%02x",
                                 i, btm_db.br_link[i].connected_profile);

                /* Do not shadow bud link */
                if (btm_db.br_link[i].connected_profile &&
                    (btm_db.br_link[i].connected_profile & RDTP_PROFILE_MASK) == 0)
                {
                    bt_roleswap_sync(btm_db.br_link[i].bd_addr);

                    if (bt_relay_start_shadow_link(btm_db.br_link[i].bd_addr, control_conn_handle))
                    {
                        bt_roleswap_state = ROLESWAP_STATE_LINK_SHADOW;
                    }
                }
            }
        }
        break;

    case GAP_BR_HANDOVER_CONN_CMPL_INFO:
        {
            T_GAP_HANDOVER_CONN_CMPL_INFO *p_info = (T_GAP_HANDOVER_CONN_CMPL_INFO *)p_buf;
            if (p_info->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: roleswap connection fail, cause 0x%04x", p_info->cause);
                return;
            }

            /* secondary starts roleswap */
            bt_roleswap_state = ROLESWAP_STATE_LINK_RESUME;

            if (p_info->link_type == 0 || p_info->link_type == 2) //sco or esco
            {
                T_ROLESWAP_DATA *p_data = bt_find_roleswap_data(p_info->bd_addr, ROLESWAP_TYPE_SCO);
                if (p_data == NULL)
                {
                    BTM_PRINT_ERROR0("bt_relay_callback: fail to find roleswap info for SCO");
                    return;
                }

                p_data->u.sco.handle = p_info->handle;
            }
            else    //acl
            {
                T_ROLESWAP_DATA *p_data = bt_find_roleswap_data(p_info->bd_addr, ROLESWAP_TYPE_ACL);
                if (p_data == NULL)
                {
                    BTM_PRINT_ERROR0("bt_relay_callback: fail to find roleswap info for ACL");
                    return;
                }

                p_data->u.acl.handle = p_info->handle;
                p_data->u.acl.encrypt_state = p_info->encrypt_enabled;
            }
        }
        break;

    case GAP_BR_HANDOVER_CMPL_INFO:
        {
            T_GAP_HANDOVER_CMPL_INFO *p_info = (T_GAP_HANDOVER_CMPL_INFO *)p_buf;
            T_BT_MSG_PAYLOAD payload;
            uint8_t peer_addr[6];
            uint8_t i;

            if (p_info->cause)
            {
                BTM_PRINT_ERROR1("bt_relay_callback: roleswap complete fail, cause 0x%04x", p_info->cause);

                bt_roleswap_state = ROLESWAP_STATE_IDLE;

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    for (i = 0; i < btm_db.br_link_num; i++)
                    {
                        if (btm_db.br_link[i].connected_profile & ~RDTP_PROFILE_MASK)
                        {
                            bt_relay_roleswap_stop(btm_db.br_link[i].bd_addr);
                        }
                    }
                }

                payload.msg_buf = &p_info->cause;
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

            for (i = 0; i < btm_db.br_link_num; i++)
            {
                BTM_PRINT_TRACE5("bt_relay_callback: %u, role %u, connected_profile 0x%08x, bd_addr %s, handle 0x%04x",
                                 i, remote_session_role_get(), btm_db.br_link[i].connected_profile,
                                 TRACE_BDADDR(btm_db.br_link[i].bd_addr), btm_db.br_link[i].acl_handle);
            }

            payload.msg_buf = &p_info->cause;
            bt_mgr_dispatch(BT_MSG_ROLESWAP_STATE, &payload);

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                /* TODO Audiolib should cache bud link type to distinguish BR from LE */
                for (i = 0; i < btm_db.br_link_num; i++)
                {
                    if (btm_db.br_link[i].connected_profile & ~RDTP_PROFILE_MASK)
                    {
                        bt_relay_roleswap_stop(btm_db.br_link[i].bd_addr);
                    }
                }

                remote_peer_addr_get(peer_addr);
                gap_set_bd_addr(peer_addr);
            }
        }
        break;

    default:
        {
            uint16_t cause = *(uint16_t *)p_buf;
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
