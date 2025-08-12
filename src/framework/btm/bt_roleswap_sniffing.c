/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "gap_handover_br.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_roleswap_int.h"

/* TODO Remove Start */
#include "vhci.h"
#include "low_stack.h"
/* TODO Remove End */

#define INVALID_HCI_HANDLE      0xFFFF

#define SNIFFING_AUDIO_INTERVAL         0x12
#define SNIFFING_AUDIO_FLUSH_TIMEOUT    0x70
#define SNIFFING_AUDIO_RSVD_SLOT        0x06
#define SNIFFING_AUDIO_INTERVAL_AFTER_ROLESWAP         0x18
#define SNIFFING_AUDIO_RSVD_SLOT_AFTER_ROLESWAP        0x0A

typedef enum
{
    SNIFFING_STATE_IDLE                   = 0x00,
    SNIFFING_STATE_SETUP_SNIFFING         = 0x01,
    SNIFFING_STATE_SNIFFING_CONNECTED     = 0x02,
    SNIFFING_STATE_SETUP_RECOVERY         = 0x03,
    SNIFFING_STATE_RECOVERY_CONNECTED     = 0x04,
    SNIFFING_STATE_DISCONN_RECOVERY       = 0x05,
    SNIFFING_STATE_COORDINATED_ROLESWAP   = 0x06,
    SNIFFING_STATE_UNCOORDINATED_ROLESWAP = 0x07,
    SNIFFING_STATE_IDLE_ROLESWAP          = 0x08,
} T_BT_SNIFFING_STATE;

typedef enum
{
    SNIFFING_SUBSTATE_IDLE                = 0x00,
    SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING = 0x01,
    SNIFFING_SUBSTATE_MODIFY_PARAM        = 0x02,
    SNIFFING_SUBSTATE_FLOW_STOP           = 0x03,
    SNIFFING_SUBSTATE_ENABLE_CONT_TRX     = 0x04,
    SNIFFING_SUBSTATE_FLUSH_RECOVERY      = 0x05,
    SNIFFING_SUBSTATE_DISCONN_RECOVERY    = 0x06,
    SNIFFING_SUBSTATE_SYNC_DATA           = 0x07,
    SNIFFING_SUBSTATE_SHADOW_LINK         = 0x08,
    SNIFFING_SUBSTATE_ROLE_SWITCH         = 0x09,
    SNIFFING_SUBSTATE_RECONN_RECOVERY     = 0x0a,
    SNIFFING_SUBSTATE_ROLESWAP_TERMINATE  = 0x0b,
} T_BT_SNIFFING_SUBSTATE;

typedef enum
{
    SNIFFING_EVT_CONN_SNIFFING              = 0x00,
    SNIFFING_EVT_CONN_RECOVERY              = 0x01,
    SNIFFING_EVT_CTRL_LINK_ACTIVE           = 0x02,
    SNIFFING_EVT_AUDIO_LINK_ACTIVE          = 0x03,
    SNIFFING_EVT_SCO_CONNECT                = 0x04,
    SNIFFING_EVT_SCO_DISCONNECT             = 0x05,
    SNIFFING_EVT_CTRL_CONNECT               = 0x06,
    SNIFFING_EVT_CTRL_DISCONNECT            = 0x07,
    SNIFFING_EVT_AUDIO_DISCONN              = 0x08,
    SNIFFING_EVT_SET_ACTIVE_STATE_RSP       = 0x09,
    SNIFFING_EVT_SHADOW_LINK_RSP            = 0x0A,
    SNIFFING_EVT_ACL_RX_EMPTY               = 0x0B,
    SNIFFING_EVT_HANDOVER_CONN_CMPL         = 0x0C,
    SNIFFING_EVT_HANDOVER_CMPL              = 0x0D,
    SNIFFING_EVT_RECOVERY_SETUP_RSP         = 0x0E,
    SNIFFING_EVT_RECOVERY_CONN_REQ          = 0x0F,
    SNIFFING_EVT_RECOVERY_CONN_CMPL         = 0x10,
    SNIFFING_EVT_RECOVERY_DISCONN_CMPL      = 0x11,
    SNIFFING_EVT_RECOVERY_CHANGED           = 0x12,
    SNIFFING_EVT_RECOVERY_RESET             = 0x13,
    SNIFFING_EVT_RECOVERY_FLUSH_CMPL        = 0x14,
    SNIFFING_EVT_SNIFFING_MODE_CHANGE       = 0x15,
    SNIFFING_EVT_VND_ROLE_SWITCH            = 0x16,
    SNIFFING_EVT_START_ROLESWAP             = 0x17,
    SNIFFING_EVT_START_ROLESWAP_CFM         = 0x18,
    SNIFFING_EVT_ADJUST_QOS                 = 0x19,
    SNIFFING_EVT_SHADOW_LINK_LOSS           = 0x1a,
    SNIFFING_EVT_ROLESWAP_TOKEN_RSP         = 0x1b,
    SNIFFING_EVT_SET_CONT_TRX_CMPL          = 0x1c,
    SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO   = 0x1d,
    SNIFFING_EVT_STOP_ROLESWAP              = 0x1e,
} T_BT_SNIFFING_EVENT;

typedef enum
{
    RECOVERY_LINK_TYPE_NONE = 0,
    RECOVERY_LINK_TYPE_A2DP = 1,
    RECOVERY_LINK_TYPE_SCO  = 2
} T_RECOVERY_LINK_TYPE;

typedef enum t_bt_sniffing_sync_info_type
{
    BT_SNIFFING_SETUP_SNIFFING_REQ         = 0x00,
    BT_SNIFFING_SETUP_RECOVERY_REQ         = 0x01,
    BT_SNIFFING_ROLESWAP_REQ               = 0x02,
    BT_SNIFFING_SETUP_SNIFFING_RSP         = 0x10,
    BT_SNIFFING_SETUP_RECOVERY_RSP         = 0x11,
    BT_SNIFFING_ROLESWAP_RSP               = 0x12,
    BT_SNIFFING_SETUP_SNIFFING_TERMINATE   = 0x20,
    BT_SNIFFING_SETUP_RECOVERY_TERMINATE   = 0x21,
    BT_SNIFFING_ROLESWAP_TERMINATE         = 0x22,
} T_BT_SNIFFING_SYNC_INFO_TYPE;

typedef struct t_bt_sniffing_setup_sniffing_req
{
    uint8_t     bd_addr[6];
} T_BT_SNIFFING_SETUP_SNIFFING_REQ;

typedef struct t_bt_sniffing_setup_recovery_req
{
    T_RECOVERY_LINK_TYPE    recovery_link_type;
} T_BT_SNIFFING_SETUP_RECOVERY_REQ;

typedef struct t_bt_sniffing_roleswap_req
{
    T_BT_SNIFFING_STATE    sniffing_state;
    bool                   stop_after_roleswap;
    uint8_t                context;
} T_BT_SNIFFING_ROLESWAP_REQ;

typedef struct t_bt_sniffing_setup_sniffing_rsp
{
    bool                   accept;
    uint8_t                context;
} T_BT_SNIFFING_SETUP_SNIFFING_RSP;

typedef struct t_bt_sniffing_setup_recovery_rsp
{
    bool                   accept;
    uint8_t                context;
} T_BT_SNIFFING_SETUP_RECOVERY_RSP;

typedef struct t_bt_sniffing_roleswap_rsp
{
    bool                   accept;
    uint8_t                context;
} T_BT_SNIFFING_ROLESWAP_RSP;

typedef struct t_bt_sniffing_setup_sniffing_terminate
{
    uint16_t               cause;
} T_BT_SNIFFING_SETUP_SNIFFING_TERMINATE;

typedef struct t_bt_sniffing_setup_recovery_terminate
{
    uint16_t               cause;
} T_BT_SNIFFING_SETUP_RECOVERY_TERMINATE;

typedef struct t_bt_sniffing_roleswap_terminate
{
    T_BT_SNIFFING_STATE    sniffing_state;
    uint16_t               cause;
} T_BT_SNIFFING_ROLESWAP_TERMINATE;

typedef union t_bt_sniffing_state_sync_info_param
{
    T_BT_SNIFFING_SETUP_SNIFFING_REQ        setup_sniffing_req;
    T_BT_SNIFFING_SETUP_RECOVERY_REQ        setup_recovery_req;
    T_BT_SNIFFING_ROLESWAP_REQ              roleswap_req;
    T_BT_SNIFFING_SETUP_SNIFFING_RSP        setup_sniffing_rsp;
    T_BT_SNIFFING_SETUP_RECOVERY_RSP        setup_recovery_rsp;
    T_BT_SNIFFING_ROLESWAP_RSP              roleswap_rsp;
    T_BT_SNIFFING_SETUP_SNIFFING_TERMINATE  setup_sniffing_terminate;
    T_BT_SNIFFING_SETUP_RECOVERY_TERMINATE  setup_recovery_terminate;
    T_BT_SNIFFING_ROLESWAP_TERMINATE        roleswap_terminate;
} T_BT_SNIFFING_STATE_SYNC_INFO_PARAM;

typedef struct t_bt_sniffing_state_sync_info
{
    T_BT_SNIFFING_SYNC_INFO_TYPE         type;
    T_BT_SNIFFING_STATE_SYNC_INFO_PARAM  param;
} T_BT_SNIFFING_STATE_SYNC_INFO;

typedef struct
{
    uint16_t    interval;
    uint16_t    flush_tout;
    uint8_t     rsvd_slot;
    uint8_t     idle_slot;
    uint8_t     idle_skip;
} T_RECOVERY_LINK_PARAM;

typedef struct
{
    uint8_t     bd_addr[6];
    uint16_t    cause;
} T_SNIFFING_SCO_DISCONN_PARAM;

#if (CONFIG_REALTEK_BTM_ROLESWAP_SUPPORT == 1)
static uint8_t audio_addr[6];
static uint16_t sniffing_handle = INVALID_HCI_HANDLE;
static uint16_t recovery_handle = INVALID_HCI_HANDLE;
static T_BT_SNIFFING_STATE bt_sniffing_state = SNIFFING_STATE_IDLE;
static T_BT_SNIFFING_SUBSTATE bt_sniffing_substate = SNIFFING_SUBSTATE_IDLE;
static T_RECOVERY_LINK_TYPE recovery_link_type = RECOVERY_LINK_TYPE_NONE;
static bool roleswap_terminate = false;
static bool stop_after_roleswap = false;    // if true, do not setup recovery link after roleswap
static uint8_t roleswap_req_context;
static P_REMOTE_ROLESWAP_SYNC_CBACK roleswap_callback = NULL;
static uint16_t a2dp_recovery_interval;
static uint16_t a2dp_recovery_flush_tout;
static uint8_t a2dp_recovery_rsvd_slot;
static uint8_t a2dp_recovery_idle_slot;
static uint8_t a2dp_recovery_idle_skip;

void bt_sniffing_handle_evt(uint16_t  event,
                            void     *p_data);

bool vhci_filter_all_pass(VHCI_PKT_TYPE  type,
                          void          *buf)
{
    return true;
}

bool vhci_filter_exclude_sniffing(VHCI_PKT_TYPE  type,
                                  void          *buf)
{
    if (type == VHCI_ACL_PKT)
    {
        uint16_t rx_handle;
        uint8_t *p = (uint8_t *)buf;

        LE_STREAM_TO_UINT16(rx_handle, p);
        rx_handle &= 0xfff;

        if (rx_handle == sniffing_handle)
        {
            //BTM_PRINT_TRACE1("vhci_filter_exclude_sniffing, rx handle = sniffing handle %d", rx_handle);
            return false;
        }
    }

    return true;
}

bool bt_roleswap_audio_cback(uint8_t       bd_addr[6],
                             T_BT_PM_EVENT event)
{
    T_BT_BR_LINK *p_link;
    bool          ret = true;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        switch (event)
        {
        case BT_PM_EVENT_LINK_CONNECTED:
            break;

        case BT_PM_EVENT_LINK_DISCONNECTED:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_SUCCESS:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_FAIL:
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_handle_evt(SNIFFING_EVT_AUDIO_LINK_ACTIVE, bd_addr);
            }
            break;

        case BT_PM_EVENT_SNIFF_ENTER_REQ:
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                /* secondary do not trigger sniff mode */
                ret = false;
            }
            else
            {
                /* primary only trigger sniff mode in idle state */
                if (bt_sniffing_state != SNIFFING_STATE_IDLE)
                {
                    ret = false;
                }
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_SUCCESS:
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_handle_evt(SNIFFING_EVT_AUDIO_LINK_ACTIVE, bd_addr);
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_FAIL:
            break;

        case BT_PM_EVENT_SNIFF_EXIT_REQ:
            break;
        }
    }

    return ret;
}

bool bt_roleswap_ctrl_cback(uint8_t       bd_addr[6],
                            T_BT_PM_EVENT event)
{
    T_BT_BR_LINK *p_link;
    bool          ret = true;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        switch (event)
        {
        case BT_PM_EVENT_LINK_CONNECTED:
            break;

        case BT_PM_EVENT_LINK_DISCONNECTED:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_SUCCESS:
            break;

        case BT_PM_EVENT_SNIFF_ENTER_FAIL:
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_handle_evt(SNIFFING_EVT_CTRL_LINK_ACTIVE, bd_addr);
            }
            break;

        case BT_PM_EVENT_SNIFF_ENTER_REQ:
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                /* secondary do not trigger sniff mode */
                ret = false;
            }
            else
            {
                /* primary only trigger sniff mode in idle state */
                if (bt_sniffing_state != SNIFFING_STATE_IDLE)
                {
                    ret = false;
                }
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_SUCCESS:
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_handle_evt(SNIFFING_EVT_CTRL_LINK_ACTIVE, bd_addr);
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_FAIL:
            break;

        case BT_PM_EVENT_SNIFF_EXIT_REQ:
            break;
        }
    }

    return ret;
}

void bt_roleswap_sniffing_conn_cmpl(uint8_t  bd_addr[6],
                                    uint16_t cause)
{
    T_BT_MSG_PAYLOAD payload;

    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = &cause;
    bt_mgr_dispatch(BT_MSG_SNIFFING_ACL_CONN_CMPL, &payload);
}

void bt_roleswap_sniffing_disconn_cmpl(uint8_t  bd_addr[6],
                                       uint16_t cause)
{
    T_BT_MSG_PAYLOAD payload;

    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = &cause;
    bt_mgr_dispatch(BT_MSG_SNIFFING_ACL_DISCONN_CMPL, &payload);
}

void bt_roleswap_recovery_conn_cmpl(uint8_t              bd_addr[6],
                                    T_RECOVERY_LINK_TYPE type,
                                    uint16_t             cause)
{
    T_ROLESWAP_DATA *p_data;
    T_ROLESWAP_RECOVERY_CONN_PARAM param;
    T_BT_MSG_PAYLOAD payload;
    T_BT_MSG msg;

    if (type == RECOVERY_LINK_TYPE_A2DP)
    {
        msg = BT_MSG_SNIFFING_A2DP_START;

        p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_A2DP);
        if (p_data)
        {
            param.p_param = &p_data->u.a2dp;
        }
        else
        {
            param.p_param = NULL;
        }
    }
    else
    {
        msg = BT_MSG_SNIFFING_SCO_START;

        p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_SCO);
        if (p_data)
        {
            param.p_param = &p_data->u.sco;
        }
        else
        {
            param.p_param = NULL;
        }
    }

    memcpy(payload.bd_addr, bd_addr, 6);
    param.cause = cause;
    payload.msg_buf = &param;
    bt_mgr_dispatch(msg, &payload);
}

void bt_roleswap_recovery_disconn_cmpl(uint8_t              bd_addr[6],
                                       T_RECOVERY_LINK_TYPE type,
                                       uint16_t             cause)
{
    T_BT_MSG_PAYLOAD payload;
    T_BT_MSG msg;

    if (type == RECOVERY_LINK_TYPE_A2DP)
    {
        msg = BT_MSG_SNIFFING_A2DP_STOP;
    }
    else
    {
        msg = BT_MSG_SNIFFING_SCO_STOP;
    }

    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = &cause;
    bt_mgr_dispatch(msg, &payload);
}

void bt_sniffing_roleswap_terminated(uint16_t cause)
{
    T_BT_MSG_PAYLOAD payload;

    payload.msg_buf = &cause;
    bt_mgr_dispatch(BT_MSG_ROLESWAP_TERMINATED, &payload);
}

void bt_sniffing_set_info(T_ROLESWAP_INFO *p_base,
                          bool             for_roleswap)
{
    uint16_t type;
    T_ROLESWAP_DATA *p_data;
    T_BT_MSG_PAYLOAD payload;
    T_BT_MSG msg;

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

                        if (for_roleswap)
                        {
                            memcpy(payload.bd_addr, p_base->bd_addr, 6);
                            payload.msg_buf = p_info;
                            bt_mgr_dispatch(BT_MSG_ROLESWAP_ACL_STATUS, &payload);
                        }
                        else
                        {
                            bt_roleswap_sniffing_conn_cmpl(p_base->bd_addr,
                                                           HCI_SUCCESS);
                        }
                    }
                    break;

                case ROLESWAP_TYPE_SCO:
                    {
                        T_ROLESWAP_SCO_INFO *p_info = &p_data->u.sco;

                        bt_roleswap_set_sco_info(p_base->bd_addr, p_info);

                        if (for_roleswap)
                        {
                            msg = BT_MSG_ROLESWAP_SCO_STATUS;
                        }
                        else
                        {
                            msg = BT_MSG_SNIFFING_SCO_CONN_CMPL;
                        }

                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = p_info;
                        bt_mgr_dispatch(msg, &payload);
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

                        if (for_roleswap)
                        {
                            msg = BT_MSG_ROLESWAP_SPP_STATUS;
                        }
                        else
                        {
                            msg = BT_MSG_SNIFFING_SPP_CONN_CMPL;
                        }
                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.spp;
                        bt_mgr_dispatch(msg, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_A2DP:
                    {
                        bt_roleswap_set_a2dp_info(p_base->bd_addr, &p_data->u.a2dp);

                        BTM_PRINT_INFO5("bt_sniffing_set_info: sig_cid 0x%04x, stream_cid 0x%04x, sig_state 0x%04x, a2dp.state 0x%04x, last_seq_number %d",
                                        p_data->u.a2dp.sig_cid, p_data->u.a2dp.stream_cid,
                                        p_data->u.a2dp.sig_state, p_data->u.a2dp.state,
                                        p_data->u.a2dp.last_seq_number);

                        if (for_roleswap)
                        {
                            msg = BT_MSG_ROLESWAP_A2DP_STATUS;
                        }
                        else
                        {
                            msg = BT_MSG_SNIFFING_A2DP_CONN_CMPL;
                        }
                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.a2dp;
                        bt_mgr_dispatch(msg, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_AVRCP:
                    {
                        bt_roleswap_set_avrcp_info(p_base->bd_addr, &p_data->u.avrcp);

                        if (for_roleswap)
                        {
                            msg = BT_MSG_ROLESWAP_AVRCP_STATUS;
                        }
                        else
                        {
                            msg = BT_MSG_SNIFFING_AVRCP_CONN_CMPL;
                        }
                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.avrcp;
                        bt_mgr_dispatch(msg, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_HFP:
                    {
                        bt_roleswap_set_hfp_info(p_base->bd_addr, &p_data->u.hfp);

                        if (for_roleswap)
                        {
                            msg = BT_MSG_ROLESWAP_HFP_STATUS;
                        }
                        else
                        {
                            msg = BT_MSG_SNIFFING_HFP_CONN_CMPL;
                        }
                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.hfp;
                        bt_mgr_dispatch(msg, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_PBAP:
                    {
                        bt_roleswap_set_pbap_info(p_base->bd_addr, &p_data->u.pbap);

                        if (for_roleswap)
                        {
                            msg = BT_MSG_ROLESWAP_PBAP_STATUS;
                        }
                        else
                        {
                            msg = BT_MSG_SNIFFING_PBAP_CONN_CMPL;
                        }
                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.pbap;
                        bt_mgr_dispatch(msg, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_HID_DEVICE:
                    {
                        bt_roleswap_set_hid_device_info(p_base->bd_addr, &p_data->u.hid_device);

                        if (for_roleswap)
                        {
                            msg = BT_MSG_ROLESWAP_HID_DEVICE_STATUS;
                        }
                        else
                        {
                            msg = BT_MSG_SNIFFING_HID_DEVICE_CONN_CMPL;
                        }
                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.hid_device;
                        bt_mgr_dispatch(msg, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_HID_HOST:
                    {
                        bt_roleswap_set_hid_host_info(p_base->bd_addr, &p_data->u.hid_host);

                        if (for_roleswap)
                        {
                            msg = BT_MSG_ROLESWAP_HID_HOST_STATUS;
                        }
                        else
                        {
                            msg = BT_MSG_SNIFFING_HID_HOST_CONN_CMPL;
                        }
                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.hid_host;
                        bt_mgr_dispatch(msg, &payload);
                    }
                    break;

                case ROLESWAP_TYPE_IAP:
                    {
                        bt_roleswap_set_iap_info(p_base->bd_addr, &p_data->u.iap);

                        if (for_roleswap)
                        {
                            msg = BT_MSG_ROLESWAP_IAP_STATUS;
                        }
                        else
                        {
                            msg = BT_MSG_SNIFFING_IAP_CONN_CMPL;
                        }
                        memcpy(payload.bd_addr, p_base->bd_addr, 6);
                        payload.msg_buf = &p_data->u.iap;
                        bt_mgr_dispatch(msg, &payload);
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

                        if (for_roleswap)
                        {
                            T_GAP_HANDOVER_GATT_INFO gatt;

                            memcpy(gatt.bd_addr, p_base->bd_addr, 6);
                            gatt.cid = p_data->u.att.l2c_cid;
                            gatt.remote_mtu = p_data->u.att.remote_mtu;
                            gatt.local_mtu = p_data->u.att.local_mtu;
                            gatt.offset = p_data->u.att.data_offset;
                            gap_br_set_handover_gatt_info(&gatt);
                        }
                    }
                    break;

                case ROLESWAP_TYPE_BT_RFC:
                    {
                        bt_roleswap_set_bt_rfc_info(p_base->bd_addr, &p_data->u.bt_rfc);
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

void bt_sniffing_set(bool for_roleswap)
{
    uint8_t i;
    T_ROLESWAP_INFO *p_info;

    for (i = 0; i < btm_db.br_link_num; i++)
    {
        p_info = &btm_db.roleswap_info[i];

        if (p_info->used)
        {
            if (memcmp(p_info->bd_addr, audio_addr, 6) == 0)
            {
                bt_sniffing_set_info(p_info, for_roleswap);
            }
        }
    }
}

bool bt_sniffing_sync_state(T_BT_SNIFFING_STATE_SYNC_INFO sync_info)
{
    uint8_t peer_addr[6];
    T_BT_BR_LINK *p_ctrl;

    remote_peer_addr_get(peer_addr);
    p_ctrl  = bt_find_br_link(peer_addr);
    if (p_ctrl)
    {
        if (gap_br_sniffing_state_sync(p_ctrl->acl_handle, sizeof(T_BT_SNIFFING_STATE_SYNC_INFO),
                                       (uint8_t *)(&sync_info)) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

void bt_sniffing_change_state(T_BT_SNIFFING_STATE state)
{
    BTM_PRINT_INFO2("bt_sniffing_change_state: change from %d to %d", bt_sniffing_state, state);
    bt_sniffing_substate = SNIFFING_SUBSTATE_IDLE;
    bt_sniffing_state = state;
}

bool bt_sniffing_sync_pause_link(uint8_t *audio_addr,
                                 uint8_t *ctrl_addr,
                                 uint8_t  sync_type)
{
    uint8_t i;
    T_BT_BR_LINK *p_ctrl;
    T_BT_BR_LINK *p_audio;
    bool sniff_pend = false;

    p_ctrl  = bt_find_br_link(ctrl_addr);
    p_audio = bt_find_br_link(audio_addr);

    if (p_audio == NULL || p_ctrl == NULL)
    {
        BTM_PRINT_ERROR2("bt_sniffing_sync_pause_link: link not ready, p_audio %p, p_ctrl %p",
                         p_audio, p_ctrl);
        return false;
    }

    BTM_PRINT_TRACE6("bt_sniffing_sync_pause_link: ctrl_addr %s, master %d, connected_profile 0x%04x, "
                     "audio addr %s, master %d, connected_profile 0x%04x",
                     TRACE_BDADDR(ctrl_addr), p_ctrl->acl_link_role_master, p_ctrl->connected_profile,
                     TRACE_BDADDR(audio_addr), p_audio->acl_link_role_master, p_audio->connected_profile);

    for (i = 0; i < btm_db.br_link_num; i++)
    {
        if (btm_db.br_link[i].acl_link_state == BT_LINK_STATE_CONNECTED)
        {
            if (bt_sniff_mode_exit(&btm_db.br_link[i], true) == false)
            {
                sniff_pend = true;
            }
        }
    }

    if (sniff_pend == false)
    {
        gap_br_shadow_pre_sync_info(p_audio->acl_handle, p_ctrl->acl_handle, sync_type);

        if (gap_br_set_acl_active_state(p_audio->acl_handle, GAP_ACL_FLOW_STOP,
                                        GAP_ACL_SUSPEND_TYPE_STOP_PER_LINK) != GAP_CAUSE_SUCCESS)
        {
            return false;
        }
        bt_sniffing_substate = SNIFFING_SUBSTATE_FLOW_STOP;
    }
    else
    {
        bt_sniffing_substate = SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING;
    }

    return true;
}

bool bt_sniffing_pause_link(uint8_t *audio_addr,
                            uint8_t *ctrl_addr)
{
    T_BT_BR_LINK *p_ctrl;
    T_BT_BR_LINK *p_audio;
    bool ctrl_exit_sniff_mode;
    bool audio_exit_sniff_mode;

    p_audio = bt_find_br_link(audio_addr);
    p_ctrl  = bt_find_br_link(ctrl_addr);

    if (p_audio == NULL || p_ctrl == NULL)
    {
        return false;
    }

    ctrl_exit_sniff_mode = bt_sniff_mode_exit(p_ctrl, false);
    audio_exit_sniff_mode = bt_sniff_mode_exit(p_audio, false);
    if (ctrl_exit_sniff_mode == true && audio_exit_sniff_mode == true)
    {
        if (gap_br_set_acl_active_state(p_audio->acl_handle, GAP_ACL_FLOW_STOP,
                                        GAP_ACL_SUSPEND_TYPE_STOP_PER_LINK) != GAP_CAUSE_SUCCESS)
        {
            return false;
        }
        bt_sniffing_substate = SNIFFING_SUBSTATE_FLOW_STOP;
    }
    else
    {
        bt_sniffing_substate = SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING;
    }

    return true;
}

bool bt_sniffing_resume_link(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;
    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE2("bt_sniffing_resume_link: bd_addr %s, p_link %p", TRACE_BDADDR(bd_addr), p_link);

    if (p_link == NULL)
    {
        return false;
    }

    gap_br_set_acl_active_state(p_link->acl_handle, GAP_ACL_FLOW_GO, 0);

    return true;
}

bool bt_sniffing_set_continuous_trx(uint8_t bd_addr[6],
                                    uint8_t enable,
                                    uint8_t option)
{
    T_BT_BR_LINK *p_link;
    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE3("bt_sniffing_set_continuous_trx: addr %s, p_link %p, enable %d",
                     TRACE_BDADDR(bd_addr), p_link, enable);

    if (p_link)
    {
        if (gap_br_set_continuous_txrx(p_link->acl_handle, enable, option) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_sniffing_pre_setup_recovery(uint8_t *audio_addr,
                                    uint8_t *ctrl_addr)
{
    uint8_t i;
    T_BT_BR_LINK *p_audio;
    T_BT_BR_LINK *p_ctrl;
    bool sniff_pend = false;

    p_audio = bt_find_br_link(audio_addr);
    p_ctrl  = bt_find_br_link(ctrl_addr);

    if (p_audio == NULL || p_ctrl == NULL)
    {
        return false;
    }

    for (i = 0; i < btm_db.br_link_num; i++)
    {
        if (btm_db.br_link[i].acl_link_state == BT_LINK_STATE_CONNECTED)
        {
            if (bt_sniff_mode_exit(&btm_db.br_link[i], false) == false)
            {
                sniff_pend = true;
            }
        }
    }

    if (sniff_pend == false)
    {
        if (bt_sniffing_set_continuous_trx(ctrl_addr, 1, 1) == false)
        {
            return false;
        }
        bt_sniffing_substate = SNIFFING_SUBSTATE_ENABLE_CONT_TRX;
    }
    else
    {
        bt_sniffing_substate = SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING;
    }

    return true;
}

bool bt_sniffing_shadow_link(uint8_t                bd_addr[6],
                             uint8_t               *ctrl_addr,
                             T_GAP_SHADOW_SNIFF_OP  sniff_op)
{
    T_BT_BR_LINK *p_audio;
    T_BT_BR_LINK *p_ctrl;

    p_audio = bt_find_br_link(bd_addr);
    p_ctrl = bt_find_br_link(ctrl_addr);

    if (p_audio == NULL || p_ctrl == NULL)
    {
        return false;
    }

    BTM_PRINT_INFO3("bt_sniffing_shadow_link: bd_addr %s, ctrl addr %s, sniff_op %d",
                    TRACE_BDADDR(bd_addr), TRACE_BDADDR(ctrl_addr), sniff_op);

    if (gap_br_shadow_link(p_audio->acl_handle, p_ctrl->acl_handle, sniff_op) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }

    return true;
}

bool bt_sniffing_stop_roleswap(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;
    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE2("bt_sniffing_stop_roleswap: bd_addr %s, p_link %p", TRACE_BDADDR(bd_addr), p_link);

    if (p_link != NULL)
    {
        gap_br_set_acl_active_state(p_link->acl_handle, GAP_ACL_FLOW_GO, 0);

        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            T_ROLESWAP_DATA *p_data;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                gap_br_cfg_acl_link_policy(p_data->u.acl.bd_addr, p_data->u.acl.link_policy);
            }
        }

        return true;
    }

    return false;
}

bool bt_sniffing_setup_recovery_link(uint8_t   bd_addr[6],
                                     uint8_t  *remote_addr,
                                     uint8_t   audio_type,
                                     uint16_t  interval,
                                     uint16_t  flush_tout,
                                     uint8_t   rsvd_slot,
                                     uint16_t  expected_seq,
                                     uint8_t   idle_slot,
                                     uint8_t   idle_skip)
{
    T_BT_BR_LINK *p_ctrl;
    T_BT_BR_LINK *p_audio;
    uint16_t tgt_handle;
    uint16_t stream_cid;

    p_ctrl = bt_find_br_link(remote_addr);
    p_audio = bt_find_br_link(bd_addr);

    if (p_ctrl == NULL || p_audio == NULL)
    {
        return false;
    }

    BTM_PRINT_INFO7("bt_sniffing_setup_recovery_link: bd_addr %s, remote_addr %s, audio_type %d, "
                    "inteval %d, flash_tout %d, rsvd slot %d, expected_seq %d",
                    TRACE_BDADDR(bd_addr), TRACE_BDADDR(remote_addr), audio_type, interval,
                    flush_tout, rsvd_slot, expected_seq);

    if (audio_type == RECOVERY_LINK_TYPE_A2DP)
    {
        tgt_handle = p_audio->acl_handle;
    }
    else
    {
        tgt_handle = p_audio->sco_handle;
    }

    stream_cid = a2dp_get_stream_cid(bd_addr);

    BTM_PRINT_TRACE4("bt_sniffing_setup_recovery_link: role %u, acl_handle %d, tgt_handle %d, stream_cid 0x%x",
                     remote_session_role_get(), p_ctrl->acl_handle, tgt_handle, stream_cid);

    gap_br_setup_audio_recovery_link(p_ctrl->acl_handle, audio_type, tgt_handle,
                                     stream_cid, interval, flush_tout, rsvd_slot,
                                     expected_seq, idle_slot, idle_skip);

    return true;
}

bool bt_sniffing_create_recovery_link(uint8_t  bd_addr[6],
                                      uint8_t *remote_addr,
                                      uint8_t  type,
                                      bool     roleswap)
{
    uint16_t interval = 0;
    uint16_t flush_tout = 0;
    uint8_t rsvd_slot = 0;
    uint16_t expected_seq = 0;

    BTM_PRINT_INFO3("bt_sniffing_create_recovery_link: bd_addr %s, remote_addr %s, type %d",
                    TRACE_BDADDR(bd_addr), TRACE_BDADDR(remote_addr), type);

    if (type == RECOVERY_LINK_TYPE_A2DP)
    {
        if (roleswap)
        {
            interval = SNIFFING_AUDIO_INTERVAL;
            flush_tout = SNIFFING_AUDIO_FLUSH_TIMEOUT;
            rsvd_slot = SNIFFING_AUDIO_RSVD_SLOT;
            a2dp_recovery_idle_slot = 0;
            a2dp_recovery_idle_skip = 0;
        }
        else
        {
            T_BT_BR_LINK *p_link;

            interval = a2dp_recovery_interval;
            flush_tout = a2dp_recovery_flush_tout;
            rsvd_slot = a2dp_recovery_rsvd_slot;

            p_link = bt_find_br_link(bd_addr);
            if (p_link)
            {
                if (p_link->a2dp_data.last_seq_num != 0)
                {
                    expected_seq = p_link->a2dp_data.last_seq_num + 1;
                }
            }
        }
    }

    BTM_PRINT_INFO6("bt_sniffing_create_recovery_link: interval 0x%04x, flush_tout 0x%04x, rsvd_slot 0x%02x, expected_seq 0x%04x,"
                    "idle_slot 0x%02x, idle_skip 0x%02x",
                    interval, flush_tout, rsvd_slot, expected_seq, a2dp_recovery_idle_slot, a2dp_recovery_idle_skip);

    return bt_sniffing_setup_recovery_link(bd_addr, remote_addr, type, interval,
                                           flush_tout, rsvd_slot, expected_seq,
                                           a2dp_recovery_idle_slot, a2dp_recovery_idle_skip);
}

bool bt_sniffing_recovery_link_reply(uint8_t  bd_addr[6],
                                     uint8_t *remote_addr,
                                     uint8_t  type,
                                     uint8_t  in_order)
{
    T_BT_BR_LINK *p_ctrl;
    T_BT_BR_LINK *p_audio;
    uint16_t tgt_handle;

    BTM_PRINT_TRACE4("bt_sniffing_recovery_link_reply: bd_addr %s, remote_addr %s, type %d, in_order %d",
                     TRACE_BDADDR(bd_addr), TRACE_BDADDR(remote_addr), type, in_order);

    p_ctrl = bt_find_br_link(remote_addr);
    p_audio = bt_find_br_link(bd_addr);

    if (p_ctrl == NULL || p_audio == NULL)
    {
        return false;
    }

    if (type == RECOVERY_LINK_TYPE_A2DP)     //a2dp
    {
        tgt_handle = sniffing_handle;
    }
    else
    {
        tgt_handle = p_audio->sco_handle;
    }

    BTM_PRINT_TRACE2("bt_sniffing_recovery_link_reply: acl_handle 0x%04x, tgt_handle 0x%04x",
                     p_ctrl->acl_handle, tgt_handle);

    if (tgt_handle == INVALID_HCI_HANDLE || tgt_handle == 0)
    {
        return false;
    }

    if (gap_br_audio_recovery_link_req_reply(p_ctrl->acl_handle, type, tgt_handle,
                                             in_order) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    return false;
}

bool bt_sniffing_flush_recovery_link(uint8_t bd_addr[6],
                                     uint8_t timeout)
{
    T_BT_BR_LINK *p_link;
    p_link = bt_find_br_link(bd_addr);

    if (p_link)
    {
        BTM_PRINT_TRACE2("bt_sniffing_flush_recovery_link: recovery_handle 0x%04x, timeout %d",
                         recovery_handle, timeout);

        if (gap_br_flush_audio_recovery_link(recovery_handle, timeout) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    BTM_PRINT_ERROR1("bt_sniffing_flush_recovery_link: fail to flush recovery link for bd_addr %s",
                     TRACE_BDADDR(bd_addr));

    return false;
}

bool bt_sniffing_reset_recovery_link(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    BTM_PRINT_TRACE2("bt_sniffing_reset_recovery_link: recovery_handle 0x%04x, p_link %p",
                     recovery_handle, p_link);

    if (p_link)
    {
        if (gap_br_reset_audio_recovery_link(recovery_handle) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    BTM_PRINT_ERROR1("bt_sniffing_reset_recovery_link: fail to reset recovery link for bd_addr %s",
                     TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_sniffing_disconn_recovery_link(uint8_t bd_addr[6],
                                       uint8_t reason)
{
    T_BT_BR_LINK *p_link;
    p_link = bt_find_br_link(bd_addr);

    if (p_link)
    {
        BTM_PRINT_TRACE2("bt_sniffing_disconn_recovery_link: recovery_handle 0x%04x, reason 0x%04x",
                         recovery_handle, reason);

        if (gap_br_remove_audio_recovery_link(recovery_handle, reason) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    BTM_PRINT_ERROR1("bt_sniffing_disconn_recovery_link: fail to disconn recovery link for bd_addr %s",
                     TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_sniffing_vendor_role_switch(uint8_t  bd_addr[6],
                                    uint16_t flush_tout)
{
    T_BT_BR_LINK *p_link;
    uint8_t role;

    p_link = bt_find_br_link(bd_addr);
    if (p_link)
    {
        role = p_link->acl_link_role_master ? 1 : 0;

        BTM_PRINT_TRACE3("bt_sniffing_vendor_role_switch: addr %s, role %d, flush_tout %d",
                         TRACE_BDADDR(bd_addr), role, flush_tout);

        if (gap_br_vendor_role_switch(p_link->acl_handle, role, flush_tout) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

bool bt_sniffing_set_a2dp_dup_num(bool    enable,
                                  uint8_t nack_num,
                                  bool    quick_flush)
{

    BTM_PRINT_TRACE3("bt_sniffing_set_a2dp_dup_num: nack_num %u, enable %u quick_flush %u", nack_num,
                     enable, quick_flush);
    if (gap_br_vendor_set_a2dp_dup_num(enable, nack_num, quick_flush))
    {
        return true;
    }
    return false;
}

void bt_sniffing_sync_token(uint8_t *audio_addr)
{
    T_BT_BR_LINK *p_link;
    int32_t expect_seq;
    uint8_t token_buf[1];
    uint8_t *p_token = token_buf;

    p_link = bt_find_br_link(audio_addr);
    if (p_link)
    {
        expect_seq = recovery_link_get_expected_a2dp_seq(p_link->acl_handle);
        if (expect_seq > 0)
        {
            p_link->a2dp_data.last_seq_num = expect_seq - 1;
        }
    }

    bt_roleswap_sync(audio_addr);

    if (roleswap_callback)
    {
        roleswap_callback();
    }

    LE_UINT8_TO_STREAM(p_token, stop_after_roleswap);

    bt_roleswap_info_send(ROLESWAP_MODULE_CTRL, ROLESWAP_CTRL_TOKEN_REQ,
                          token_buf, 1);
}

void bt_sniffing_state_idle(uint16_t  event,
                            void     *p_data)
{
    uint8_t peer_addr[6];

    BTM_PRINT_INFO2("bt_sniffing_state_idle: event 0x%04x, recovery_link_type %d",
                    event, recovery_link_type);

    remote_peer_addr_get(peer_addr);

    switch (event)
    {
    case SNIFFING_EVT_CTRL_CONNECT:
        vhci_set_filter(vhci_filter_all_pass);
        break;

    case SNIFFING_EVT_CONN_SNIFFING:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            bt_sniffing_change_state(SNIFFING_STATE_SETUP_SNIFFING);
            bt_sniffing_handle_evt(SNIFFING_EVT_CONN_SNIFFING, p_data);
        }
        break;

    /* link disconnect during setup/roleswap procedure */
    case SNIFFING_EVT_ACL_RX_EMPTY:
    case SNIFFING_EVT_HANDOVER_CMPL:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            bt_sniffing_resume_link(audio_addr);
        }
        break;

    case SNIFFING_EVT_HANDOVER_CONN_CMPL:
        break;

    /* no sniffing link */
    case SNIFFING_EVT_START_ROLESWAP:
        {
            uint8_t *bd_addr = (uint8_t *)p_data;

            if (bt_find_br_link(bd_addr) != NULL)
            {
                bt_sniffing_change_state(SNIFFING_STATE_UNCOORDINATED_ROLESWAP);
                bt_sniffing_handle_evt(SNIFFING_EVT_START_ROLESWAP, p_data);
            }
            else
            {
                T_BT_BR_LINK *p_link;

                p_link = bt_find_br_link(peer_addr);
                if (p_link)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_IDLE_ROLESWAP);
                    bt_sniffing_handle_evt(SNIFFING_EVT_START_ROLESWAP, p_data);
                }
            }
        }
        break;

    case SNIFFING_EVT_VND_ROLE_SWITCH:
        {
            T_GAP_VENDOR_ROLE_SWITCH *p_info = (T_GAP_VENDOR_ROLE_SWITCH *)p_data;
            T_BT_MSG_PAYLOAD payload;
            T_GAP_HANDOVER_BUD_INFO info;

            if (p_info->cause == 0)
            {
                /* update bud link database */
                memcpy(btm_db.remote_link->bd_addr, p_info->remote_addr, 6);
                if (p_info->new_role == GAP_BR_LINK_ROLE_MASTER)
                {
                    btm_db.remote_link->acl_link_role_master = true;
                }
                else
                {
                    btm_db.remote_link->acl_link_role_master = false;
                }

                remote_peer_addr_set(p_info->remote_addr);
                remote_local_addr_set(p_info->local_addr);

                /* update bud profile database */
                rdtp_handle_roleswap(p_info->local_addr, p_info->remote_addr);

                memcpy(info.pre_bd_addr, p_info->local_addr, 6);
                memcpy(info.curr_bd_addr, p_info->remote_addr, 6);
                info.curr_link_role = p_info->new_role;
                payload.msg_buf = &info;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_ADDR_STATUS, &payload);

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    remote_session_role_set(REMOTE_SESSION_ROLE_SECONDARY);
                }
                else
                {
                    remote_session_role_set(REMOTE_SESSION_ROLE_PRIMARY);
                }
                BTM_PRINT_TRACE5("bt_sniffing_state_idle: vnd role switch, role %d, recovery_link_type %d, "
                                 "stop_after_roleswap %d, remote addr %s, local addr %s",
                                 remote_session_role_get(), recovery_link_type, stop_after_roleswap,
                                 TRACE_BDADDR(p_info->remote_addr), TRACE_BDADDR(p_info->local_addr));

                payload.msg_buf = &p_info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO:
        {
            T_BT_SNIFFING_STATE_SYNC_INFO *p_info = (T_BT_SNIFFING_STATE_SYNC_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (p_info->type == BT_SNIFFING_SETUP_SNIFFING_REQ)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_SETUP_SNIFFING);
                }
                else if (p_info->type == BT_SNIFFING_ROLESWAP_REQ)
                {
                    T_BT_MSG_PAYLOAD payload;
                    T_BT_ROLESWAP_REQ roleswap_req;

                    bt_sniffing_change_state(p_info->param.roleswap_req.sniffing_state);

                    roleswap_req.context = p_info->param.roleswap_req.context;
                    payload.msg_buf = &roleswap_req;
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_REQ, &payload);
                }
            }
        }
        break;

    /* link disconnect during setup recovery/roleswap procedure */
    case SNIFFING_EVT_SET_CONT_TRX_CMPL:
        {
            T_GAP_SET_CONTINUOUS_TRX_CMPL *p_cmpl = (T_GAP_SET_CONTINUOUS_TRX_CMPL *)p_data;

            if (p_cmpl->cause == HCI_SUCCESS && p_cmpl->enable)
            {
                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_state_setup_sniffing(uint16_t  event,
                                      void     *p_data)
{
    uint8_t peer_addr[6];
    T_BT_SNIFFING_STATE_SYNC_INFO sync_info;

    BTM_PRINT_INFO2("bt_sniffing_state_setup_sniffing: event 0x%04x, recovery_link_type %d",
                    event, recovery_link_type);

    remote_peer_addr_get(peer_addr);

    switch (event)
    {
    case SNIFFING_EVT_CONN_SNIFFING:
        {
            sync_info.type = BT_SNIFFING_SETUP_SNIFFING_REQ;
            memcpy(sync_info.param.setup_sniffing_req.bd_addr, audio_addr, 6);
            bt_sniffing_sync_state(sync_info);

            if (bt_sniffing_sync_pause_link(audio_addr, peer_addr, 0x00) == false)
            {
                sync_info.type = BT_SNIFFING_SETUP_SNIFFING_TERMINATE;
                sync_info.param.setup_sniffing_terminate.cause = HCI_ERR | HCI_ERR_INVALID_STATE;

                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                bt_sniffing_sync_state(sync_info);
            }
        }
        break;

    case SNIFFING_EVT_CTRL_LINK_ACTIVE:
    case SNIFFING_EVT_AUDIO_LINK_ACTIVE:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
            bt_sniffing_substate == SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING)
        {
            if (bt_sniffing_sync_pause_link(audio_addr, peer_addr, 0x00) == false)
            {
                sync_info.type = BT_SNIFFING_SETUP_SNIFFING_TERMINATE;
                sync_info.param.setup_sniffing_terminate.cause = HCI_ERR | HCI_ERR_INVALID_STATE;

                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                bt_sniffing_sync_state(sync_info);
            }
        }
        break;

    case SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO:
        {
            T_BT_SNIFFING_STATE_SYNC_INFO *p_info = (T_BT_SNIFFING_STATE_SYNC_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (p_info->type == BT_SNIFFING_SETUP_SNIFFING_TERMINATE)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                }
            }
        }
        break;

    case SNIFFING_EVT_SET_ACTIVE_STATE_RSP:
        {
            T_GAP_SET_ACL_ACTIVE_STATE_RSP *p_rsp = (T_GAP_SET_ACL_ACTIVE_STATE_RSP *)p_data;

            if (p_rsp->cause)
            {
                sync_info.type = BT_SNIFFING_SETUP_SNIFFING_TERMINATE;
                sync_info.param.setup_sniffing_terminate.cause = p_rsp->cause;

                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                bt_sniffing_sync_state(sync_info);
                bt_roleswap_sniffing_conn_cmpl(audio_addr, p_rsp->cause);
            }
        }
        break;

    case SNIFFING_EVT_AUDIO_DISCONN:
        {
            uint16_t cause = *(uint16_t *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                sync_info.type = BT_SNIFFING_SETUP_SNIFFING_TERMINATE;
                sync_info.param.setup_sniffing_terminate.cause = cause;
                bt_sniffing_sync_state(sync_info);
            }

            bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            bt_roleswap_sniffing_conn_cmpl(audio_addr, cause);
        }
        break;

    case SNIFFING_EVT_CTRL_DISCONNECT:
        {
            uint16_t cause = *(uint16_t *)p_data;

            bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_resume_link(audio_addr);
                bt_roleswap_sniffing_conn_cmpl(audio_addr, cause);
            }
        }
        break;

    case SNIFFING_EVT_ACL_RX_EMPTY:
        {
            T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *p_info = (T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *)p_data;

            if (p_info->cause)
            {
                sync_info.type = BT_SNIFFING_SETUP_SNIFFING_TERMINATE;
                sync_info.param.setup_sniffing_terminate.cause = p_info->cause;

                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                bt_sniffing_sync_state(sync_info);
                bt_roleswap_sniffing_conn_cmpl(audio_addr, p_info->cause);
                break;
            }

            bt_roleswap_sync(audio_addr);

            if (bt_sniffing_shadow_link(audio_addr, peer_addr,
                                        GAP_SHADOW_SNIFF_OP_PEER_SNIFFING) == false)
            {
                sync_info.type = BT_SNIFFING_SETUP_SNIFFING_TERMINATE;
                sync_info.param.setup_sniffing_terminate.cause = HCI_ERR | HCI_ERR_INVALID_STATE;

                bt_sniffing_resume_link(audio_addr);
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                bt_sniffing_sync_state(sync_info);
            }
        }
        break;

    case SNIFFING_EVT_SHADOW_LINK_RSP:
        {
            T_GAP_SHADOW_LINK_RSP *p_rsp = (T_GAP_SHADOW_LINK_RSP *)p_data;

            if (p_rsp->cause)
            {
                sync_info.type = BT_SNIFFING_SETUP_SNIFFING_TERMINATE;
                sync_info.param.setup_sniffing_terminate.cause = p_rsp->cause;

                bt_sniffing_resume_link(audio_addr);
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                bt_sniffing_sync_state(sync_info);
                bt_roleswap_sniffing_conn_cmpl(audio_addr, p_rsp->cause);
            }
        }
        break;

    case SNIFFING_EVT_HANDOVER_CMPL:
        {
            T_GAP_HANDOVER_CMPL_INFO *p_info = (T_GAP_HANDOVER_CMPL_INFO *)p_data;

            if (p_info->cause)
            {
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_resume_link(audio_addr);
                    bt_roleswap_sniffing_conn_cmpl(audio_addr, p_info->cause);
                }
                break;
            }

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                T_BT_BR_LINK *p_link;

                bt_sniffing_set(false);

                BTM_PRINT_TRACE0("bt_sniffing_state_setup_sniffing: secondary sniffing ready, set vhci exclude");
                vhci_set_filter(vhci_filter_exclude_sniffing);

                // secondary register audio pm cback
                p_link = bt_find_br_link(audio_addr);
                if (p_link)
                {
                    bt_pm_cback_register(p_link->bd_addr, bt_roleswap_audio_cback);
                }
            }

            bt_sniffing_resume_link(audio_addr);
            bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);

            /* primary notify upper layer sniffing link setup success */
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                BTM_PRINT_TRACE0("bt_sniffing_state_setup_sniffing: notify setup recovery link");
                bt_roleswap_sniffing_conn_cmpl(audio_addr, HCI_SUCCESS);
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_state_sniffing_connected(uint16_t  event,
                                          void     *p_data)
{
    uint8_t peer_addr[6];

    BTM_PRINT_INFO2("bt_sniffing_state_sniffing_connected: event 0x%04x, recovery type %d",
                    event, recovery_link_type);

    remote_peer_addr_get(peer_addr);

    switch (event)
    {
    case SNIFFING_EVT_CTRL_DISCONNECT:
        {
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            }
            else
            {
                gap_br_disconn_sniffing_link(sniffing_handle, HCI_ERR_REMOTE_USER_TERMINATE);
            }
        }
        break;

    case SNIFFING_EVT_AUDIO_DISCONN:
        {
            uint16_t cause = *(uint16_t *)p_data;

            bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                bt_roleswap_sniffing_disconn_cmpl(audio_addr, cause);
            }
        }
        break;

    /* link disconnect during setup recovery/roleswap procedure */
    case SNIFFING_EVT_SET_CONT_TRX_CMPL:
        {
            T_GAP_SET_CONTINUOUS_TRX_CMPL *p_cmpl = (T_GAP_SET_CONTINUOUS_TRX_CMPL *)p_data;

            if (p_cmpl->cause == HCI_SUCCESS && p_cmpl->enable)
            {
                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
            }
        }
        break;

    /* secondary change state */
    case SNIFFING_EVT_RECOVERY_CONN_REQ:
        {
            T_BT_MSG_PAYLOAD payload;
            T_ROLESWAP_DATA *p_roleswap_data;
            T_GAP_AUDIO_RECOVERY_LINK_REQ_IND *p_ind = (T_GAP_AUDIO_RECOVERY_LINK_REQ_IND *)p_data;

            BTM_PRINT_TRACE0("bt_sniffing_state_sniffing_connected: recovery_link request ind");

            p_roleswap_data = bt_find_roleswap_data(p_ind->bd_addr, ROLESWAP_TYPE_A2DP);
            if (p_roleswap_data)
            {
                memcpy(payload.bd_addr, p_ind->bd_addr, 6);
                payload.msg_buf = &p_roleswap_data->u.a2dp;
                bt_mgr_dispatch(BT_MSG_SNIFFING_A2DP_START_IND, &payload);

                bt_sniffing_recovery_link_reply(p_ind->bd_addr, peer_addr, p_ind->audio_type, 0x01);

                bt_sniffing_change_state(SNIFFING_STATE_SETUP_RECOVERY);
            }
        }
        break;

    case SNIFFING_EVT_START_ROLESWAP:
        {
            bt_sniffing_change_state(SNIFFING_STATE_COORDINATED_ROLESWAP);
            bt_sniffing_handle_evt(SNIFFING_EVT_START_ROLESWAP, p_data);
        }
        break;

    /* secondary change state */
    case SNIFFING_EVT_SNIFFING_MODE_CHANGE:
        {
            T_GAP_HANDOVER_CMPL_INFO *p_info = (T_GAP_HANDOVER_CMPL_INFO *)p_data;
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (p_info->cause == HCI_SUCCESS)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_COORDINATED_ROLESWAP);
                }
            }
        }
        break;

    case SNIFFING_EVT_SHADOW_LINK_LOSS:
        bt_sniffing_change_state(SNIFFING_STATE_IDLE);
        break;

    case SNIFFING_EVT_CONN_RECOVERY:
        {
            bt_sniffing_change_state(SNIFFING_STATE_SETUP_RECOVERY);
            bt_sniffing_handle_evt(SNIFFING_EVT_CONN_RECOVERY, p_data);
        }
        break;

    case SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO:
        {
            T_BT_SNIFFING_STATE_SYNC_INFO *p_info = (T_BT_SNIFFING_STATE_SYNC_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (p_info->type == BT_SNIFFING_SETUP_RECOVERY_REQ)
                {
                    recovery_link_type = p_info->param.setup_recovery_req.recovery_link_type;
                    if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
                    {
                        T_BT_MSG_PAYLOAD payload;
                        T_ROLESWAP_DATA *p_data;

                        p_data = bt_find_roleswap_data(audio_addr, ROLESWAP_TYPE_A2DP);
                        if (p_data)
                        {
                            memcpy(payload.bd_addr, audio_addr, 6);
                            payload.msg_buf = &p_data->u.a2dp;
                            bt_mgr_dispatch(BT_MSG_SNIFFING_A2DP_START_IND, &payload);

                            bt_sniffing_change_state(SNIFFING_STATE_SETUP_RECOVERY);
                        }
                    }
                    else
                    {
                        bt_sniffing_change_state(SNIFFING_STATE_SETUP_RECOVERY);
                    }
                }
                else if (p_info->type == BT_SNIFFING_ROLESWAP_REQ)
                {
                    T_BT_MSG_PAYLOAD payload;
                    T_BT_ROLESWAP_REQ roleswap_req;

                    stop_after_roleswap = p_info->param.roleswap_req.stop_after_roleswap;
                    bt_sniffing_change_state(SNIFFING_STATE_COORDINATED_ROLESWAP);

                    roleswap_req.context = p_info->param.roleswap_req.context;
                    payload.msg_buf = &roleswap_req;
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_REQ, &payload);
                }
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_state_setup_recovery(uint16_t  event,
                                      void     *p_data)
{
    uint8_t peer_addr[6];
    T_BT_SNIFFING_STATE_SYNC_INFO sync_info;

    BTM_PRINT_INFO2("bt_sniffing_state_setup_recovery: event 0x%04x, recovery type %d",
                    event, recovery_link_type);

    remote_peer_addr_get(peer_addr);

    switch (event)
    {
    case SNIFFING_EVT_AUDIO_DISCONN:
        {
            uint16_t cause = *(uint16_t *)p_data;
            T_RECOVERY_LINK_TYPE type = recovery_link_type;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
            }

            bt_sniffing_change_state(SNIFFING_STATE_IDLE);

            recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            bt_roleswap_recovery_conn_cmpl(audio_addr, type, cause);

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                bt_roleswap_sniffing_disconn_cmpl(audio_addr, cause);
            }
        }
        break;

    case SNIFFING_EVT_CTRL_DISCONNECT:
        {
            uint16_t cause = *(uint16_t *)p_data;
            T_RECOVERY_LINK_TYPE type = recovery_link_type;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            }
            else
            {
                gap_br_disconn_sniffing_link(sniffing_handle, HCI_ERR_REMOTE_USER_TERMINATE);
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
            }

            recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            bt_roleswap_recovery_conn_cmpl(audio_addr, type, cause);
        }
        break;

    case SNIFFING_EVT_CONN_RECOVERY:
        {
            sync_info.type = BT_SNIFFING_SETUP_RECOVERY_REQ;
            sync_info.param.setup_recovery_req.recovery_link_type = recovery_link_type;
            bt_sniffing_sync_state(sync_info);

            if (bt_sniffing_pre_setup_recovery(audio_addr, peer_addr) == false)
            {
                T_RECOVERY_LINK_TYPE type = recovery_link_type;

                sync_info.type = BT_SNIFFING_SETUP_RECOVERY_TERMINATE;

                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                bt_sniffing_sync_state(sync_info);
                bt_roleswap_recovery_conn_cmpl(audio_addr, type, HCI_ERR | HCI_ERR_INVALID_STATE);
            }

        }
        break;

    case SNIFFING_EVT_CTRL_LINK_ACTIVE:
    case SNIFFING_EVT_AUDIO_LINK_ACTIVE:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
            recovery_link_type != RECOVERY_LINK_TYPE_NONE &&
            bt_sniffing_substate == SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING)
        {
            if (bt_sniffing_pre_setup_recovery(audio_addr, peer_addr) == false)
            {
                T_RECOVERY_LINK_TYPE type = recovery_link_type;

                sync_info.type = BT_SNIFFING_SETUP_RECOVERY_TERMINATE;

                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                bt_sniffing_sync_state(sync_info);
                bt_roleswap_recovery_conn_cmpl(audio_addr, type, HCI_ERR | HCI_ERR_INVALID_STATE);
            }
        }
        break;

    case SNIFFING_EVT_SET_ACTIVE_STATE_RSP:
        {
            T_GAP_SET_ACL_ACTIVE_STATE_RSP *p_rsp = (T_GAP_SET_ACL_ACTIVE_STATE_RSP *)p_data;

            if (p_rsp->cause)
            {
                T_RECOVERY_LINK_TYPE type = recovery_link_type;

                sync_info.type = BT_SNIFFING_SETUP_RECOVERY_TERMINATE;
                sync_info.param.setup_recovery_terminate.cause = p_rsp->cause;

                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                bt_sniffing_sync_state(sync_info);

                bt_roleswap_recovery_conn_cmpl(audio_addr, type, p_rsp->cause);
            }
        }
        break;

    case SNIFFING_EVT_RECOVERY_CONN_REQ:
        {
            T_GAP_AUDIO_RECOVERY_LINK_REQ_IND *p_ind = (T_GAP_AUDIO_RECOVERY_LINK_REQ_IND *)p_data;

            bt_sniffing_recovery_link_reply(p_ind->bd_addr, peer_addr, p_ind->audio_type, 0x01);
        }
        break;

    case SNIFFING_EVT_SET_CONT_TRX_CMPL:
        {
            T_GAP_SET_CONTINUOUS_TRX_CMPL *p_cmpl = (T_GAP_SET_CONTINUOUS_TRX_CMPL *)p_data;

            BTM_PRINT_TRACE2("bt_sniffing_state_setup_recovery: set cont trx cause 0x%04x, enable %d",
                             p_cmpl->cause, p_cmpl->enable);

            if (p_cmpl->cause == HCI_SUCCESS && p_cmpl->enable)
            {
                bt_sniffing_create_recovery_link(audio_addr, peer_addr,
                                                 recovery_link_type, false);
            }
            else if (p_cmpl->cause)
            {
                T_RECOVERY_LINK_TYPE type = recovery_link_type;

                sync_info.type = BT_SNIFFING_SETUP_RECOVERY_TERMINATE;
                sync_info.param.setup_recovery_terminate.cause = p_cmpl->cause;

                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                bt_sniffing_sync_state(sync_info);
                bt_roleswap_recovery_conn_cmpl(audio_addr, type, p_cmpl->cause);
            }
        }
        break;

    case SNIFFING_EVT_RECOVERY_SETUP_RSP:
        {
            T_GAP_SETUP_AUDIO_RECOVERY_LINK_RSP *p_rsp = (T_GAP_SETUP_AUDIO_RECOVERY_LINK_RSP *)p_data;

            BTM_PRINT_TRACE1("bt_sniffing_state_setup_recovery: setup recovery link cause 0x%04x",
                             p_rsp->cause);

            if (p_rsp->cause)
            {
                T_RECOVERY_LINK_TYPE type = recovery_link_type;

                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                sync_info.type = BT_SNIFFING_SETUP_RECOVERY_TERMINATE;
                sync_info.param.setup_recovery_terminate.cause = p_rsp->cause;

                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                bt_sniffing_sync_state(sync_info);

                bt_roleswap_recovery_conn_cmpl(audio_addr, type, p_rsp->cause);
            }
        }
        break;

    case SNIFFING_EVT_RECOVERY_CONN_CMPL:
        {
            T_GAP_AUDIO_RECOVERY_LINK_CONN_CMPL_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_CONN_CMPL_INFO *)p_data;

            BTM_PRINT_TRACE2("bt_sniffing_state_setup_recovery: recovery connect cause 0x%04x, audio_type %d",
                             p_info->cause, p_info->audio_type);

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                gap_br_set_continuous_txrx(p_info->ctrl_handle, 0, 0);
            }

            if (p_info->cause)
            {
                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
            }
            else
            {
                recovery_link_type = (T_RECOVERY_LINK_TYPE)p_info->audio_type;
                recovery_handle = p_info->recovery_handle;

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    if (p_info->audio_type == RECOVERY_LINK_TYPE_A2DP)
                    {
                        BTM_PRINT_INFO0("bt_sniffing_state_setup_recovery: secondary connd, type A2DP, set vhci pass all");
                        vhci_set_filter(vhci_filter_all_pass);
                    }
                    else if (p_info->audio_type == RECOVERY_LINK_TYPE_SCO)
                    {
                        BTM_PRINT_INFO0("bt_sniffing_state_setup_recovery: secondary connd, type SCO, set vhci exclude sniffing");
                        vhci_set_filter(vhci_filter_exclude_sniffing);
                    }
                }

                bt_sniffing_change_state(SNIFFING_STATE_RECOVERY_CONNECTED);
            }

            bt_roleswap_recovery_conn_cmpl(audio_addr, (T_RECOVERY_LINK_TYPE)p_info->audio_type,
                                           p_info->cause);
        }
        break;

    case SNIFFING_EVT_RECOVERY_CHANGED:
        {
            T_GAP_AUDIO_RECOVERY_LINK_CONNECTION_CHANGE_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_CONNECTION_CHANGE_INFO *)p_data;

            BTM_PRINT_TRACE2("bt_sniffing_state_setup_recovery: recovery changed from type %d to %d",
                             recovery_link_type, p_info->audio_type);

            recovery_link_type = (T_RECOVERY_LINK_TYPE)p_info->audio_type;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
            }

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
                {
                    BTM_PRINT_TRACE0("bt_sniffing_state_setup_recovery: secondary changed, type A2DP, set vhci pass all");
                    vhci_set_filter(vhci_filter_all_pass);
                }
                else if (recovery_link_type == RECOVERY_LINK_TYPE_SCO)
                {
                    BTM_PRINT_TRACE0("bt_sniffing_state_setup_recovery: secondary changed, type SCO, set vhci exclude sniffing");
                    vhci_set_filter(vhci_filter_exclude_sniffing);
                }
            }

            bt_sniffing_change_state(SNIFFING_STATE_RECOVERY_CONNECTED);
            bt_roleswap_recovery_conn_cmpl(audio_addr, recovery_link_type,
                                           HCI_SUCCESS);
        }
        break;

    /* secondary got recovery connection req first */
    case SNIFFING_EVT_SCO_CONNECT:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
        {
            bt_sniffing_recovery_link_reply(p_data, peer_addr, RECOVERY_LINK_TYPE_SCO, 0x01);
        }
        break;

    case SNIFFING_EVT_SHADOW_LINK_LOSS:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            T_GAP_SHADOW_LINK_LOSS_INFO *p_info;
            T_RECOVERY_LINK_TYPE type = recovery_link_type;

            p_info = (T_GAP_SHADOW_LINK_LOSS_INFO *)p_data;

            recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
            bt_sniffing_change_state(SNIFFING_STATE_IDLE);

            bt_roleswap_recovery_conn_cmpl(audio_addr, type, p_info->reason);
        }
        break;

    /* sco disconn during setup sco recovery link */
    case SNIFFING_EVT_SCO_DISCONNECT:
        if (recovery_link_type == RECOVERY_LINK_TYPE_SCO)
        {
            T_SNIFFING_SCO_DISCONN_PARAM *p_param;

            p_param = (T_SNIFFING_SCO_DISCONN_PARAM *)p_data;
            recovery_link_type = RECOVERY_LINK_TYPE_NONE;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                sync_info.type = BT_SNIFFING_SETUP_RECOVERY_TERMINATE;
                sync_info.param.setup_recovery_terminate.cause = p_param->cause;

                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                bt_sniffing_sync_state(sync_info);
            }
            else
            {
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
            }

            bt_roleswap_recovery_conn_cmpl(audio_addr, RECOVERY_LINK_TYPE_SCO,
                                           p_param->cause);
        }
        break;

    case SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO:
        {
            T_BT_SNIFFING_STATE_SYNC_INFO *p_info = (T_BT_SNIFFING_STATE_SYNC_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (p_info->type == BT_SNIFFING_SETUP_RECOVERY_TERMINATE)
                {
                    uint16_t cause = HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE;
                    T_RECOVERY_LINK_TYPE type = recovery_link_type;

                    recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                    bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);

                    bt_roleswap_recovery_conn_cmpl(audio_addr, type, cause);
                }
                else if (p_info->type == BT_SNIFFING_ROLESWAP_REQ)
                {
                    T_BT_MSG_PAYLOAD payload;
                    T_BT_ROLESWAP_REQ roleswap_req;

                    stop_after_roleswap = p_info->param.roleswap_req.stop_after_roleswap;
                    bt_sniffing_change_state(SNIFFING_STATE_COORDINATED_ROLESWAP);

                    roleswap_req.context = p_info->param.roleswap_req.context;
                    payload.msg_buf = &roleswap_req;
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_REQ, &payload);
                }
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_state_recovery_connected(uint16_t  event,
                                          void     *p_data)
{
    uint8_t peer_addr[6];

    BTM_PRINT_INFO2("bt_sniffing_state_recovery_connected: event 0x%04x, recovery type %d",
                    event, recovery_link_type);

    remote_peer_addr_get(peer_addr);

    switch (event)
    {
    case SNIFFING_EVT_CTRL_DISCONNECT:
        {
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            }
            else
            {
                gap_br_disconn_sniffing_link(sniffing_handle, HCI_ERR_REMOTE_USER_TERMINATE);
            }
        }
        break;

    case SNIFFING_EVT_AUDIO_DISCONN:
        {
            recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            bt_sniffing_change_state(SNIFFING_STATE_IDLE);

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                uint16_t cause = *(uint16_t *)p_data;
                bt_roleswap_sniffing_disconn_cmpl(audio_addr, cause);
            }
        }
        break;

    /* receive recovery link disconn from lower */
    case SNIFFING_EVT_RECOVERY_DISCONN_CMPL:
        {
            T_RECOVERY_LINK_TYPE type = recovery_link_type;
            T_GAP_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                BTM_PRINT_TRACE0("bt_sniffing_state_recovery_connected: secondary disconn, set vhci exclude sniffing");
                vhci_set_filter(vhci_filter_exclude_sniffing);
            }

            if (p_info->reason != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
            {
                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            }

            bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
            bt_roleswap_recovery_disconn_cmpl(audio_addr, type, p_info->reason);
        }
        break;

    /* secondary change state */
    case SNIFFING_EVT_RECOVERY_CONN_REQ:
        {
            T_GAP_AUDIO_RECOVERY_LINK_REQ_IND *p_ind = (T_GAP_AUDIO_RECOVERY_LINK_REQ_IND *)p_data;

            bt_sniffing_recovery_link_reply(p_ind->bd_addr, peer_addr, p_ind->audio_type, 0x01);

            bt_sniffing_change_state(SNIFFING_STATE_SETUP_RECOVERY);
        }
        break;

    case SNIFFING_EVT_CONN_RECOVERY:
        {
            bt_sniffing_change_state(SNIFFING_STATE_SETUP_RECOVERY);
            bt_sniffing_handle_evt(SNIFFING_EVT_CONN_RECOVERY, p_data);
        }
        break;

    /* secondary change state */
    case SNIFFING_EVT_SNIFFING_MODE_CHANGE:
        {
            T_GAP_HANDOVER_CMPL_INFO *p_info = (T_GAP_HANDOVER_CMPL_INFO *)p_data;
            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (p_info->cause == HCI_SUCCESS)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_COORDINATED_ROLESWAP);
                }
            }
        }
        break;

    case SNIFFING_EVT_START_ROLESWAP:
        {
            bt_sniffing_change_state(SNIFFING_STATE_COORDINATED_ROLESWAP);
            bt_sniffing_handle_evt(SNIFFING_EVT_START_ROLESWAP, p_data);
        }
        break;

    case SNIFFING_EVT_ADJUST_QOS:
        {
            T_RECOVERY_LINK_PARAM *p_param = (T_RECOVERY_LINK_PARAM *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_substate = SNIFFING_SUBSTATE_MODIFY_PARAM;
                bt_sniffing_setup_recovery_link(audio_addr, peer_addr, RECOVERY_LINK_TYPE_A2DP,
                                                p_param->interval, p_param->flush_tout, p_param->rsvd_slot,
                                                0, p_param->idle_slot, p_param->idle_skip);
            }
        }
        break;

    case SNIFFING_EVT_RECOVERY_SETUP_RSP:
        {
            T_BT_MSG_PAYLOAD payload;
            T_GAP_SETUP_AUDIO_RECOVERY_LINK_RSP *p_rsp = (T_GAP_SETUP_AUDIO_RECOVERY_LINK_RSP *)p_data;

            if (p_rsp->cause)
            {
                bt_sniffing_substate = SNIFFING_SUBSTATE_IDLE;
                if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
                {
                    memcpy(payload.bd_addr, audio_addr, 6);
                    payload.msg_buf = &p_rsp->cause;
                    bt_mgr_dispatch(BT_MSG_SNIFFING_A2DP_CONFIG_CMPL, &payload);
                }
            }
        }
        break;

    case SNIFFING_EVT_RECOVERY_CHANGED:
        {
            T_BT_MSG_PAYLOAD payload;
            T_GAP_AUDIO_RECOVERY_LINK_CONNECTION_CHANGE_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_CONNECTION_CHANGE_INFO *)p_data;

            BTM_PRINT_TRACE2("bt_sniffing_state_recovery_connected: recovery_link_type changed from %d to %d",
                             recovery_link_type, p_info->audio_type);

            bt_sniffing_substate = SNIFFING_SUBSTATE_IDLE;
            recovery_link_type = (T_RECOVERY_LINK_TYPE)p_info->audio_type;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {

                if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
                {
                    BTM_PRINT_TRACE0("bt_sniffing_state_recovery_connected: secondary changed, type A2DP, set vhci pass all");
                    vhci_set_filter(vhci_filter_all_pass);
                }
                else if (recovery_link_type == RECOVERY_LINK_TYPE_SCO)
                {
                    BTM_PRINT_TRACE0("bt_sniffing_state_recovery_connected: secondary changed, type SCO, set vhci exclude sniffing");
                    vhci_set_filter(vhci_filter_exclude_sniffing);
                }
            }

            memcpy(payload.bd_addr, audio_addr, 6);
            payload.msg_buf = &p_info->cause;
            if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
            {
                bt_mgr_dispatch(BT_MSG_SNIFFING_A2DP_CONFIG_CMPL, &payload);
            }
        }
        break;

    case SNIFFING_EVT_SHADOW_LINK_LOSS:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            bt_sniffing_change_state(SNIFFING_STATE_IDLE);
        }
        break;

    case SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO:
        {
            T_BT_SNIFFING_STATE_SYNC_INFO *p_info = (T_BT_SNIFFING_STATE_SYNC_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (p_info->type == BT_SNIFFING_SETUP_RECOVERY_REQ)
                {
                    recovery_link_type = p_info->param.setup_recovery_req.recovery_link_type;
                    if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
                    {
                        T_BT_MSG_PAYLOAD payload;
                        T_ROLESWAP_DATA *p_data;

                        p_data = bt_find_roleswap_data(audio_addr, ROLESWAP_TYPE_A2DP);
                        if (p_data)
                        {
                            memcpy(payload.bd_addr, audio_addr, 6);
                            payload.msg_buf = &p_data->u.a2dp;
                            bt_mgr_dispatch(BT_MSG_SNIFFING_A2DP_START_IND, &payload);

                            bt_sniffing_change_state(SNIFFING_STATE_SETUP_RECOVERY);
                        }
                    }
                    else
                    {
                        bt_sniffing_change_state(SNIFFING_STATE_SETUP_RECOVERY);
                    }
                }
                else if (p_info->type == BT_SNIFFING_ROLESWAP_REQ)
                {
                    T_BT_MSG_PAYLOAD payload;
                    T_BT_ROLESWAP_REQ roleswap_req;

                    stop_after_roleswap = p_info->param.roleswap_req.stop_after_roleswap;
                    bt_sniffing_change_state(SNIFFING_STATE_COORDINATED_ROLESWAP);

                    roleswap_req.context = p_info->param.roleswap_req.context;
                    payload.msg_buf = &roleswap_req;
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_REQ, &payload);
                }
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_state_disconn_recovery(uint16_t  event,
                                        void     *p_data)
{
    BTM_PRINT_INFO2("bt_sniffing_state_disconn_recovery: event 0x%04x, recovery_link_type %d",
                    event, recovery_link_type);

    switch (event)
    {
    case SNIFFING_EVT_CTRL_DISCONNECT:
    case SNIFFING_EVT_AUDIO_DISCONN:
        {
            recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            bt_sniffing_change_state(SNIFFING_STATE_IDLE);
        }
        break;

    /* recovery link disconn by framework, now only a2dp stop */
    case SNIFFING_EVT_RECOVERY_DISCONN_CMPL:
        {
            T_GAP_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                BTM_PRINT_TRACE0("bt_sniffing_state_disconn_recovery: secondary disconn, set vhci exclude sniffing");
                vhci_set_filter(vhci_filter_exclude_sniffing);
            }

            bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
            bt_roleswap_recovery_disconn_cmpl(audio_addr, recovery_link_type,
                                              p_info->reason);

            if (p_info->reason != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
            {
                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            }
        }
        break;

    case SNIFFING_EVT_SHADOW_LINK_LOSS:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            bt_sniffing_change_state(SNIFFING_STATE_IDLE);
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_state_coordinated_roleswap(uint16_t  event,
                                            void     *p_data)
{
    uint8_t peer_addr[6];
    T_BT_SNIFFING_STATE_SYNC_INFO sync_info;

    BTM_PRINT_INFO2("bt_sniffing_state_coordinated_roleswap: event 0x%04x, recovery_link_type %d",
                    event, recovery_link_type);

    remote_peer_addr_get(peer_addr);

    switch (event)
    {
    case SNIFFING_EVT_START_ROLESWAP:
        {
            sync_info.type = BT_SNIFFING_ROLESWAP_REQ;
            sync_info.param.roleswap_req.sniffing_state = SNIFFING_STATE_COORDINATED_ROLESWAP;
            sync_info.param.roleswap_req.stop_after_roleswap = stop_after_roleswap;
            sync_info.param.roleswap_req.context = roleswap_req_context;
            bt_sniffing_sync_state(sync_info);
        }
        break;

    case SNIFFING_EVT_START_ROLESWAP_CFM:
        {
            T_BT_SNIFFING_ROLESWAP_RSP *rsp = (T_BT_SNIFFING_ROLESWAP_RSP *)p_data;

            if (rsp->accept == false)
            {
                if (recovery_handle != INVALID_HCI_HANDLE)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_RECOVERY_CONNECTED);
                }
                else
                {
                    bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                }
            }
            else
            {
                bt_mgr_dispatch(BT_MSG_ROLESWAP_START, NULL);
            }

            sync_info.type = BT_SNIFFING_ROLESWAP_RSP;
            sync_info.param.roleswap_rsp.accept = rsp->accept;
            sync_info.param.roleswap_rsp.context = rsp->context;
            bt_sniffing_sync_state(sync_info);
        }
        break;

    case SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO:
        {
            T_BT_SNIFFING_STATE_SYNC_INFO *p_info = (T_BT_SNIFFING_STATE_SYNC_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY &&
                p_info->type == BT_SNIFFING_ROLESWAP_TERMINATE)
            {
                bt_sniffing_change_state(p_info->param.roleswap_terminate.sniffing_state);
                bt_sniffing_roleswap_terminated(HCI_SUCCESS);
            }
            else if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
                     p_info->type == BT_SNIFFING_ROLESWAP_RSP)
            {
                T_BT_MSG_PAYLOAD payload;
                T_BT_ROLESWAP_RSP rsp;

                rsp.accept = p_info->param.roleswap_rsp.accept;
                rsp.context = p_info->param.roleswap_rsp.context;
                payload.msg_buf = &rsp;

                if (p_info->param.roleswap_rsp.accept == false)
                {
                    if (recovery_handle != INVALID_HCI_HANDLE)
                    {
                        bt_sniffing_change_state(SNIFFING_STATE_RECOVERY_CONNECTED);
                    }
                    else
                    {
                        bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                    }

                    bt_mgr_dispatch(BT_MSG_ROLESWAP_RSP, &payload);
                    return;
                }

                if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
                {
                    bt_sniffing_substate = SNIFFING_SUBSTATE_MODIFY_PARAM;
                    bt_sniffing_setup_recovery_link(audio_addr, peer_addr, RECOVERY_LINK_TYPE_A2DP,
                                                    SNIFFING_AUDIO_INTERVAL,
                                                    SNIFFING_AUDIO_FLUSH_TIMEOUT,
                                                    SNIFFING_AUDIO_RSVD_SLOT,
                                                    0, 0, 0);
                }
                else
                {
                    if (bt_sniffing_pause_link(audio_addr, peer_addr) == false)
                    {
                        uint16_t err = HCI_ERR | HCI_ERR_INVALID_STATE;

                        sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                        sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_SNIFFING_CONNECTED;
                        sync_info.param.roleswap_terminate.cause = err;

                        bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                        bt_sniffing_sync_state(sync_info);

                        payload.msg_buf = &err;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);

                        return;
                    }
                }
                bt_mgr_dispatch(BT_MSG_ROLESWAP_RSP, &payload);
            }
        }
        break;

    case SNIFFING_EVT_SET_ACTIVE_STATE_RSP:
        {
            T_BT_MSG_PAYLOAD payload;
            T_GAP_SET_ACL_ACTIVE_STATE_RSP *p_rsp = (T_GAP_SET_ACL_ACTIVE_STATE_RSP *)p_data;

            if (bt_sniffing_substate == SNIFFING_SUBSTATE_ROLESWAP_TERMINATE)
            {
                if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
                {
                    bt_sniffing_setup_recovery_link(audio_addr, peer_addr, RECOVERY_LINK_TYPE_A2DP,
                                                    a2dp_recovery_interval, a2dp_recovery_flush_tout,
                                                    a2dp_recovery_rsvd_slot, 0,
                                                    a2dp_recovery_idle_slot, a2dp_recovery_idle_skip);
                }
                else
                {
                    if (recovery_handle != INVALID_HCI_HANDLE)
                    {
                        bt_sniffing_change_state(SNIFFING_STATE_RECOVERY_CONNECTED);
                        sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_RECOVERY_CONNECTED;
                    }
                    else
                    {
                        bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                        sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_SNIFFING_CONNECTED;
                    }

                    sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                    sync_info.param.roleswap_terminate.cause = HCI_ERR | HCI_ERR_INVALID_STATE;
                    bt_sniffing_sync_state(sync_info);

                    if (roleswap_terminate)
                    {
                        roleswap_terminate = false;
                        bt_sniffing_roleswap_terminated(HCI_SUCCESS);
                    }
                }
            }
            else if (bt_sniffing_substate == SNIFFING_SUBSTATE_FLOW_STOP)
            {
                if (p_rsp->cause)
                {
                    if (recovery_handle != INVALID_HCI_HANDLE)
                    {
                        bt_sniffing_change_state(SNIFFING_STATE_RECOVERY_CONNECTED);
                        sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_RECOVERY_CONNECTED;
                    }
                    else
                    {
                        bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                        sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_SNIFFING_CONNECTED;
                    }
                    sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                    sync_info.param.roleswap_terminate.cause = HCI_ERR | HCI_ERR_INVALID_STATE;
                    bt_sniffing_sync_state(sync_info);

                    if (roleswap_terminate)
                    {
                        roleswap_terminate = false;
                        bt_sniffing_roleswap_terminated(HCI_SUCCESS);
                    }
                    else
                    {
                        payload.msg_buf = &p_rsp->cause;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
                    }
                }
            }
        }
        break;

    case SNIFFING_EVT_CTRL_DISCONNECT:
        {
            uint16_t cause = *(uint16_t *)p_data;
            T_BT_MSG_PAYLOAD payload;

            if (recovery_link_type != RECOVERY_LINK_TYPE_NONE)
            {
                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            }

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
                bt_sniffing_resume_link(audio_addr);
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            }
            else
            {
                gap_br_disconn_sniffing_link(sniffing_handle, HCI_ERR_REMOTE_USER_TERMINATE);
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
            }

            if (roleswap_terminate)
            {
                roleswap_terminate = false;
                bt_sniffing_roleswap_terminated(HCI_SUCCESS);
            }
            else
            {
                payload.msg_buf = &cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case SNIFFING_EVT_AUDIO_DISCONN:
        {
            uint16_t cause = *(uint16_t *)p_data;
            T_BT_MSG_PAYLOAD payload;

            bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            if (recovery_link_type != RECOVERY_LINK_TYPE_NONE)
            {
                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
            }

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
                bt_sniffing_resume_link(audio_addr);
            }
            else
            {
                bt_roleswap_sniffing_disconn_cmpl(audio_addr, cause);
            }

            if (roleswap_terminate)
            {
                roleswap_terminate = false;
                bt_sniffing_roleswap_terminated(HCI_SUCCESS);
            }
            else
            {
                payload.msg_buf = &cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case SNIFFING_EVT_RECOVERY_CHANGED:
        {
            if (bt_sniffing_substate == SNIFFING_SUBSTATE_ROLESWAP_TERMINATE)
            {
                T_GAP_AUDIO_RECOVERY_LINK_CONNECTION_CHANGE_INFO *p_info =
                    (T_GAP_AUDIO_RECOVERY_LINK_CONNECTION_CHANGE_INFO *)p_data;

                BTM_PRINT_TRACE2("bt_sniffing_state_coordinated_roleswap: recovery_link_type changed from %d to %d",
                                 recovery_link_type, p_info->audio_type);

                roleswap_terminate = false;
                recovery_link_type = (T_RECOVERY_LINK_TYPE)p_info->audio_type;

                sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_RECOVERY_CONNECTED;
                sync_info.param.roleswap_terminate.cause = HCI_ERR | HCI_ERR_INVALID_STATE;
                bt_sniffing_sync_state(sync_info);

                bt_sniffing_change_state(SNIFFING_STATE_RECOVERY_CONNECTED);
                bt_sniffing_roleswap_terminated(HCI_SUCCESS);
            }
            else
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    if (roleswap_terminate)
                    {
                        /* change parameters back */
                        if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
                        {
                            bt_sniffing_setup_recovery_link(audio_addr, peer_addr, RECOVERY_LINK_TYPE_A2DP,
                                                            a2dp_recovery_interval, a2dp_recovery_flush_tout,
                                                            a2dp_recovery_rsvd_slot, 0,
                                                            a2dp_recovery_idle_slot, a2dp_recovery_idle_skip);
                            bt_sniffing_substate = SNIFFING_SUBSTATE_ROLESWAP_TERMINATE;
                        }
                    }
                    else
                    {
                        bt_sniffing_pause_link(audio_addr, peer_addr);
                    }
                }
            }

        }
        break;

    case SNIFFING_EVT_CTRL_LINK_ACTIVE:
    case SNIFFING_EVT_AUDIO_LINK_ACTIVE:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            if (roleswap_terminate)
            {
                roleswap_terminate = false;

                if (recovery_handle != INVALID_HCI_HANDLE)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_RECOVERY_CONNECTED);
                    sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_RECOVERY_CONNECTED;
                }
                else
                {
                    bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                    sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_SNIFFING_CONNECTED;
                }

                sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                sync_info.param.roleswap_terminate.cause = HCI_ERR | HCI_ERR_INVALID_STATE;
                bt_sniffing_sync_state(sync_info);
                bt_sniffing_roleswap_terminated(HCI_SUCCESS);
            }
            else if (bt_sniffing_substate == SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING)
            {
                bt_sniffing_pause_link(audio_addr, peer_addr);
            }
        }
        break;

    case SNIFFING_EVT_ACL_RX_EMPTY:
        {
            T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *p_info = (T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *)p_data;
            T_BT_MSG_PAYLOAD payload;

            if (roleswap_terminate)
            {
                bt_sniffing_resume_link(audio_addr);
                bt_sniffing_substate = SNIFFING_SUBSTATE_ROLESWAP_TERMINATE;
                break;
            }

            if (p_info->cause)
            {
                BTM_PRINT_ERROR1("bt_sniffing_state_coordinated_roleswap: acl suspend fail, cause 0x%04x",
                                 p_info->cause);

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_stop_roleswap(audio_addr);
                    bt_sniffing_substate = SNIFFING_SUBSTATE_ROLESWAP_TERMINATE;
                }

                payload.msg_buf = &p_info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);

                return;
            }

            bt_sniffing_set_continuous_trx(peer_addr, 1, 0); // enable

            if (recovery_handle != INVALID_HCI_HANDLE)
            {
                bt_sniffing_flush_recovery_link(audio_addr, 0x40);
                bt_sniffing_substate = SNIFFING_SUBSTATE_FLUSH_RECOVERY;
            }
            else
            {
                bt_sniffing_sync_token(audio_addr);
                bt_sniffing_substate = SNIFFING_SUBSTATE_SYNC_DATA;
            }

            bt_mgr_dispatch(BT_MSG_ROLESWAP_FLOW_STOP, &payload);
        }
        break;

    case SNIFFING_EVT_RECOVERY_FLUSH_CMPL:
        {
            if (roleswap_terminate)
            {
                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
                bt_sniffing_stop_roleswap(audio_addr);
                bt_sniffing_substate = SNIFFING_SUBSTATE_ROLESWAP_TERMINATE;
            }
            else
            {
                bt_sniffing_substate = SNIFFING_SUBSTATE_DISCONN_RECOVERY;
                bt_sniffing_disconn_recovery_link(audio_addr, HCI_ERR_CONN_ROLESWAP);
            }
        }
        break;

    case SNIFFING_EVT_RECOVERY_DISCONN_CMPL:
        {
            T_GAP_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO *)p_data;

            if (roleswap_terminate)
            {
                /* recoonect recovery link */
                bt_sniffing_create_recovery_link(audio_addr, peer_addr,
                                                 recovery_link_type, false);
                bt_sniffing_substate = SNIFFING_SUBSTATE_ROLESWAP_TERMINATE;
            }
            else
            {
                if (stop_after_roleswap)
                {
                    bt_roleswap_recovery_disconn_cmpl(audio_addr, recovery_link_type,
                                                      p_info->reason);
                }

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_sync_token(audio_addr);
                    bt_sniffing_substate = SNIFFING_SUBSTATE_SYNC_DATA;
                }
            }
        }
        break;

    case SNIFFING_EVT_ROLESWAP_TOKEN_RSP:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            T_BT_MSG_PAYLOAD payload;

            if (roleswap_terminate)
            {
                /* reconnect recovery link */
                bt_sniffing_create_recovery_link(audio_addr, peer_addr,
                                                 recovery_link_type, false);

                bt_sniffing_substate = SNIFFING_SUBSTATE_ROLESWAP_TERMINATE;
                break;
            }

            bt_mgr_dispatch(BT_MSG_ROLESWAP_SYNC_CMPL, &payload);

            if (bt_sniffing_shadow_link(audio_addr, peer_addr,
                                        GAP_SHADOW_SNIFF_OP_SELF_SNIFFING) == false)
            {
                uint16_t err = HCI_ERR | HCI_ERR_NO_MEMORY;

                bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
                bt_sniffing_stop_roleswap(audio_addr);
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);

                sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_SNIFFING_CONNECTED;
                sync_info.param.roleswap_terminate.cause = err;
                bt_sniffing_sync_state(sync_info);

                payload.msg_buf = &err;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
            else
            {
                bt_sniffing_substate = SNIFFING_SUBSTATE_SHADOW_LINK;
            }
        }
        break;

    case SNIFFING_EVT_SHADOW_LINK_RSP:
        {
            T_GAP_SHADOW_LINK_RSP *p_rsp = (T_GAP_SHADOW_LINK_RSP *)p_data;
            T_BT_MSG_PAYLOAD payload;

            if (p_rsp->cause)
            {
                bt_sniffing_stop_roleswap(audio_addr);
                bt_mgr_dispatch(BT_MSG_ROLESWAP_FLOW_GO, &payload);

                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);

                sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_SNIFFING_CONNECTED;
                sync_info.param.roleswap_terminate.cause = HCI_ERR | HCI_ERR_INVALID_STATE;
                bt_sniffing_sync_state(sync_info);

                if (recovery_link_type != RECOVERY_LINK_TYPE_NONE)
                {
                    recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                }

                payload.msg_buf = &p_rsp->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case SNIFFING_EVT_SNIFFING_MODE_CHANGE:
        {
            T_GAP_HANDOVER_CMPL_INFO *p_info = (T_GAP_HANDOVER_CMPL_INFO *)p_data;
            T_BT_MSG_PAYLOAD payload;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
            {
                if (p_info->cause != HCI_SUCCESS)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);
                    payload.msg_buf = &p_info->cause;
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
                }
            }
        }
        break;

    case SNIFFING_EVT_HANDOVER_CMPL:
        {
            T_GAP_HANDOVER_CMPL_INFO *p_info = (T_GAP_HANDOVER_CMPL_INFO *)p_data;
            T_BT_MSG_PAYLOAD payload;

            if (p_info->cause)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_set_continuous_trx(peer_addr, 0, 0);

                    bt_sniffing_stop_roleswap(audio_addr);
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_FLOW_GO, &payload);
                }

                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);

                if (recovery_link_type != RECOVERY_LINK_TYPE_NONE)
                {
                    recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                }

                payload.msg_buf = &p_info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);

                break;
            }

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                BTM_PRINT_TRACE0("bt_sniffing_state_coordinated_roleswap: primary change to secondary, set vhci exclude sniffing");
                remote_session_role_set(REMOTE_SESSION_ROLE_SECONDARY);
                vhci_set_filter(vhci_filter_exclude_sniffing);

                bt_mgr_dispatch(BT_MSG_ROLESWAP_IDENT_CHANGE, &payload);
            }
            else
            {
                BTM_PRINT_TRACE0("bt_sniffing_state_coordinated_roleswap: secondary change to primary, set vhci all pass");
                remote_session_role_set(REMOTE_SESSION_ROLE_PRIMARY);

                bt_sniffing_set(true);
                vhci_set_filter(vhci_filter_all_pass);
                bt_sniffing_vendor_role_switch(peer_addr, 0x30);
                bt_sniffing_substate = SNIFFING_SUBSTATE_ROLE_SWITCH;
            }
        }
        break;

    case SNIFFING_EVT_VND_ROLE_SWITCH:
        {
            T_GAP_VENDOR_ROLE_SWITCH *p_info = (T_GAP_VENDOR_ROLE_SWITCH *)p_data;
            T_BT_MSG_PAYLOAD payload;
            T_GAP_HANDOVER_BUD_INFO info;

            if (p_info->cause)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_vendor_role_switch(peer_addr, 0x30);
                }
            }
            else
            {
                /* update bud link database */
                memcpy(btm_db.remote_link->bd_addr, p_info->remote_addr, 6);
                if (p_info->new_role == GAP_BR_LINK_ROLE_MASTER)
                {
                    btm_db.remote_link->acl_link_role_master = true;
                }
                else
                {
                    btm_db.remote_link->acl_link_role_master = false;
                }

                remote_peer_addr_set(p_info->remote_addr);
                remote_local_addr_set(p_info->local_addr);

                /* update bud profile database */
                rdtp_handle_roleswap(p_info->local_addr, p_info->remote_addr);

                memcpy(info.pre_bd_addr, p_info->local_addr, 6);
                memcpy(info.curr_bd_addr, p_info->remote_addr, 6);
                info.curr_link_role = p_info->new_role;
                payload.msg_buf = &info;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_ADDR_STATUS, &payload);

                BTM_PRINT_TRACE5("bt_sniffing_state_coordinated_roleswap: vnd role switch, role %d, recovery_link_type %d, "
                                 "stop_after_roleswap %d, remote addr %s, local addr %s",
                                 remote_session_role_get(), recovery_link_type, stop_after_roleswap,
                                 TRACE_BDADDR(p_info->remote_addr), TRACE_BDADDR(p_info->local_addr));

                if (recovery_link_type == RECOVERY_LINK_TYPE_NONE || stop_after_roleswap)
                {
                    if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        bt_sniffing_set_continuous_trx(p_info->remote_addr, 0x00, 0);
                        bt_sniffing_stop_roleswap(audio_addr);
                    }
                    else
                    {
                        bt_sniffing_resume_link(audio_addr);
                    }

                    bt_mgr_dispatch(BT_MSG_ROLESWAP_FLOW_GO, &payload);

                    bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);

                    if (stop_after_roleswap)
                    {
                        stop_after_roleswap = false;
                        if (recovery_link_type != RECOVERY_LINK_TYPE_NONE)
                        {
                            recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                        }
                    }

                    payload.msg_buf = &p_info->cause;
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);

                    return;
                }

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_setup_recovery_link(audio_addr, p_info->remote_addr, recovery_link_type,
                                                    SNIFFING_AUDIO_INTERVAL_AFTER_ROLESWAP,
                                                    SNIFFING_AUDIO_FLUSH_TIMEOUT,
                                                    SNIFFING_AUDIO_RSVD_SLOT_AFTER_ROLESWAP, 0, 0, 0);
                    bt_sniffing_substate = SNIFFING_SUBSTATE_RECONN_RECOVERY;
                }

            }
        }
        break;

    case SNIFFING_EVT_RECOVERY_CONN_REQ:
        {
            T_GAP_AUDIO_RECOVERY_LINK_REQ_IND *p_ind = (T_GAP_AUDIO_RECOVERY_LINK_REQ_IND *)p_data;

            BTM_PRINT_TRACE0("bt_sniffing_state_coordinated_roleswap: recovery_link request ind");

            bt_sniffing_recovery_link_reply(p_ind->bd_addr, peer_addr, p_ind->audio_type, 0x01);
        }
        break;

    case SNIFFING_EVT_RECOVERY_CONN_CMPL:
        {
            T_GAP_AUDIO_RECOVERY_LINK_CONN_CMPL_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_CONN_CMPL_INFO *)p_data;
            T_BT_MSG_PAYLOAD payload;
            uint16_t roleswap_cause = HCI_SUCCESS;  /* roleswap success in this setp */

            BTM_PRINT_TRACE2("bt_sniffing_state_coordinated_roleswap: recovery connect cause 0x%04x, audio_type %d",
                             p_info->cause, p_info->audio_type);

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_sniffing_set_continuous_trx(peer_addr, 0x00, 0x00);
                bt_sniffing_stop_roleswap(audio_addr);
            }
            else
            {
                bt_sniffing_resume_link(audio_addr);
            }

            bt_mgr_dispatch(BT_MSG_ROLESWAP_FLOW_GO, &payload);

            if (p_info->cause)
            {
                bt_sniffing_change_state(SNIFFING_STATE_SNIFFING_CONNECTED);

                if (recovery_link_type == RECOVERY_LINK_TYPE_A2DP)
                {
                    recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                    bt_roleswap_recovery_disconn_cmpl(audio_addr, RECOVERY_LINK_TYPE_A2DP, p_info->cause);
                }
                else if (recovery_link_type == RECOVERY_LINK_TYPE_SCO)
                {
                    recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                    bt_roleswap_recovery_disconn_cmpl(audio_addr, RECOVERY_LINK_TYPE_SCO, p_info->cause);
                }
            }
            else
            {
                recovery_link_type = (T_RECOVERY_LINK_TYPE)p_info->audio_type;
                recovery_handle = p_info->recovery_handle;

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    if (p_info->audio_type == RECOVERY_LINK_TYPE_A2DP)
                    {
                        BTM_PRINT_INFO0("bt_sniffing_state_coordinated_roleswap: secondary connd, type A2DP, set vhci pass all");
                        vhci_set_filter(vhci_filter_all_pass);
                    }
                    else if (p_info->audio_type == RECOVERY_LINK_TYPE_SCO)
                    {
                        BTM_PRINT_INFO0("bt_sniffing_state_coordinated_roleswap: secondary connd, type SCO, set vhci exclude sniffing");
                        vhci_set_filter(vhci_filter_exclude_sniffing);
                    }
                }

                bt_sniffing_change_state(SNIFFING_STATE_RECOVERY_CONNECTED);
            }

            payload.msg_buf = &roleswap_cause;
            bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
        }
        break;

    case SNIFFING_EVT_STOP_ROLESWAP:
        roleswap_terminate = true;
        break;

    default:
        break;
    }
}

void bt_sniffing_state_uncoordinated_roleswap(uint16_t  event,
                                              void     *p_data)
{
    uint8_t peer_addr[6];
    T_BT_SNIFFING_STATE_SYNC_INFO sync_info;

    BTM_PRINT_INFO2("bt_sniffing_state_uncoordinated_roleswap: event 0x%04x, recovery_link_type %d",
                    event, recovery_link_type);

    remote_peer_addr_get(peer_addr);

    switch (event)
    {
    case SNIFFING_EVT_START_ROLESWAP:
        {
            sync_info.type = BT_SNIFFING_ROLESWAP_REQ;
            sync_info.param.roleswap_req.sniffing_state = SNIFFING_STATE_UNCOORDINATED_ROLESWAP;
            sync_info.param.roleswap_req.stop_after_roleswap = stop_after_roleswap;
            sync_info.param.roleswap_req.context = roleswap_req_context;
            bt_sniffing_sync_state(sync_info);
        }
        break;

    case SNIFFING_EVT_START_ROLESWAP_CFM:
        {
            T_BT_SNIFFING_ROLESWAP_RSP *rsp = (T_BT_SNIFFING_ROLESWAP_RSP *)p_data;

            if (rsp->accept == false)
            {
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            }
            else
            {
                bt_mgr_dispatch(BT_MSG_ROLESWAP_START, NULL);
            }

            sync_info.type = BT_SNIFFING_ROLESWAP_RSP;
            sync_info.param.roleswap_rsp.accept = rsp->accept;
            sync_info.param.roleswap_rsp.context = rsp->context;
            bt_sniffing_sync_state(sync_info);
        }
        break;

    case SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO:
        {
            T_BT_SNIFFING_STATE_SYNC_INFO *p_info = (T_BT_SNIFFING_STATE_SYNC_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY &&
                p_info->type == BT_SNIFFING_ROLESWAP_TERMINATE)
            {
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                bt_sniffing_roleswap_terminated(HCI_SUCCESS);
            }
            else if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
                     p_info->type == BT_SNIFFING_ROLESWAP_RSP)
            {
                T_BT_MSG_PAYLOAD payload;
                T_BT_ROLESWAP_RSP rsp;

                rsp.accept = p_info->param.roleswap_rsp.accept;
                rsp.context = p_info->param.roleswap_rsp.context;
                payload.msg_buf = &rsp;

                if (p_info->param.roleswap_rsp.accept == false)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_RSP, &payload);
                    return;
                }

                if (bt_sniffing_sync_pause_link(audio_addr, peer_addr, 0x00) == false)
                {
                    uint16_t err = HCI_ERR | HCI_ERR_INVALID_STATE;

                    bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                    sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                    sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_IDLE;
                    sync_info.param.roleswap_terminate.cause = err;
                    bt_sniffing_sync_state(sync_info);

                    payload.msg_buf = &err;
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
                }
                else
                {
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_RSP, &payload);
                }
            }
        }
        break;

    case SNIFFING_EVT_SET_ACTIVE_STATE_RSP:
        {
            T_BT_MSG_PAYLOAD payload;
            T_GAP_SET_ACL_ACTIVE_STATE_RSP *p_rsp = (T_GAP_SET_ACL_ACTIVE_STATE_RSP *)p_data;

            if (bt_sniffing_substate == SNIFFING_SUBSTATE_ROLESWAP_TERMINATE)
            {
                roleswap_terminate = false;
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_IDLE;
                sync_info.param.roleswap_terminate.cause = HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE;
                bt_sniffing_sync_state(sync_info);
            }
            else if (bt_sniffing_substate == SNIFFING_SUBSTATE_FLOW_STOP)
            {
                if (p_rsp->cause)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                    sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                    sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_IDLE;
                    sync_info.param.roleswap_terminate.cause = HCI_ERR | HCI_ERR_INVALID_STATE;
                    bt_sniffing_sync_state(sync_info);

                    if (roleswap_terminate)
                    {
                        roleswap_terminate = false;
                        bt_sniffing_roleswap_terminated(HCI_SUCCESS);
                    }
                    else
                    {
                        payload.msg_buf = &p_rsp->cause;
                        bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
                    }
                }
            }
        }
        break;

    case SNIFFING_EVT_CTRL_DISCONNECT:
    case SNIFFING_EVT_AUDIO_DISCONN:
        {
            uint16_t cause = *(uint16_t *)p_data;
            T_BT_MSG_PAYLOAD payload;

            bt_sniffing_change_state(SNIFFING_STATE_IDLE);

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_IDLE;
                sync_info.param.roleswap_terminate.cause = cause;
                bt_sniffing_sync_state(sync_info);

                bt_sniffing_resume_link(audio_addr);

                if (roleswap_terminate)
                {
                    roleswap_terminate = false;
                }
            }

            payload.msg_buf = &cause;
            bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
        }
        break;

    case SNIFFING_EVT_CTRL_LINK_ACTIVE:
    case SNIFFING_EVT_AUDIO_LINK_ACTIVE:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            if (roleswap_terminate)
            {
                roleswap_terminate = false;
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_IDLE;
                sync_info.param.roleswap_terminate.cause = HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE;
                bt_sniffing_sync_state(sync_info);

                bt_sniffing_roleswap_terminated(HCI_SUCCESS);
            }
            else if (bt_sniffing_substate == SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING)
            {
                bt_sniffing_sync_pause_link(audio_addr, peer_addr, 0x00);
            }
        }
        break;

    case SNIFFING_EVT_ACL_RX_EMPTY:
        {
            T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *p_info = (T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *)p_data;
            T_BT_MSG_PAYLOAD payload;

            if (roleswap_terminate)
            {
                bt_sniffing_substate = SNIFFING_SUBSTATE_ROLESWAP_TERMINATE;
                bt_sniffing_resume_link(audio_addr);

                break;
            }

            if (p_info->cause)
            {
                BTM_PRINT_ERROR1("bt_sniffing_state_uncoordinated_roleswap: acl suspend fail, cause 0x%04x",
                                 p_info->cause);

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_stop_roleswap(audio_addr);
                    bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                    sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                    sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_IDLE;
                    sync_info.param.roleswap_terminate.cause = p_info->cause;
                    bt_sniffing_sync_state(sync_info);
                }

                payload.msg_buf = &p_info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);

                return;
            }

            bt_mgr_dispatch(BT_MSG_ROLESWAP_FLOW_STOP, &payload);

            bt_sniffing_sync_token(audio_addr);
            bt_sniffing_substate = SNIFFING_SUBSTATE_SYNC_DATA;
        }
        break;

    case SNIFFING_EVT_ROLESWAP_TOKEN_RSP:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            T_BT_MSG_PAYLOAD payload;

            if (roleswap_terminate)
            {
                bt_sniffing_substate = SNIFFING_SUBSTATE_ROLESWAP_TERMINATE;
                bt_sniffing_resume_link(audio_addr);
                break;
            }

            bt_mgr_dispatch(BT_MSG_ROLESWAP_SYNC_CMPL, &payload);

            if (bt_sniffing_shadow_link(audio_addr, peer_addr,
                                        GAP_SHADOW_SNIFF_OP_NO_SNIFFING) == false)
            {
                uint16_t err = HCI_ERR | HCI_ERR_NO_MEMORY;

                // bt_sniffing_set_continuous_trx(peer_addr, 0, 0);
                bt_sniffing_stop_roleswap(audio_addr);
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);

                payload.msg_buf = &err;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case SNIFFING_EVT_SHADOW_LINK_RSP:
        {
            T_GAP_SHADOW_LINK_RSP *p_rsp = (T_GAP_SHADOW_LINK_RSP *)p_data;
            T_BT_MSG_PAYLOAD payload;

            if (p_rsp->cause)
            {
                bt_sniffing_stop_roleswap(audio_addr);
                bt_mgr_dispatch(BT_MSG_ROLESWAP_FLOW_GO, &payload);

                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_IDLE;
                sync_info.param.roleswap_terminate.cause = p_rsp->cause;
                bt_sniffing_sync_state(sync_info);

                payload.msg_buf = &p_rsp->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case SNIFFING_EVT_HANDOVER_CMPL:
        {
            T_GAP_HANDOVER_CMPL_INFO *p_info = (T_GAP_HANDOVER_CMPL_INFO *)p_data;
            T_BT_MSG_PAYLOAD payload;
            T_BT_BR_LINK *p_link;

            if (p_info->cause)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_stop_roleswap(audio_addr);
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_FLOW_GO, &payload);
                }

                bt_sniffing_change_state(SNIFFING_STATE_IDLE);

                payload.msg_buf = &p_info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);

                break;
            }

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                BTM_PRINT_TRACE0("bt_sniffing_state_uncoordinated_roleswap: primary change to secondary, set vhci exclude sniffing");
                remote_session_role_set(REMOTE_SESSION_ROLE_SECONDARY);
                vhci_set_filter(vhci_filter_exclude_sniffing);

                bt_mgr_dispatch(BT_MSG_ROLESWAP_IDENT_CHANGE, &payload);
            }
            else
            {
                BTM_PRINT_TRACE0("bt_sniffing_state_uncoordinated_roleswap: secondary change to primary, set vhci all pass");
                remote_session_role_set(REMOTE_SESSION_ROLE_PRIMARY);

                bt_sniffing_set(true);
                vhci_set_filter(vhci_filter_all_pass);

                p_link = bt_find_br_link(audio_addr);
                if (p_link)
                {
                    bt_pm_cback_register(p_link->bd_addr, bt_roleswap_audio_cback);
                }

                bt_sniffing_vendor_role_switch(peer_addr, 0x30);
                bt_sniffing_substate = SNIFFING_SUBSTATE_ROLE_SWITCH;
            }
        }
        break;

    case SNIFFING_EVT_VND_ROLE_SWITCH:
        {
            T_GAP_VENDOR_ROLE_SWITCH *p_info = (T_GAP_VENDOR_ROLE_SWITCH *)p_data;
            T_BT_MSG_PAYLOAD payload;
            T_GAP_HANDOVER_BUD_INFO info;

            if (p_info->cause)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_vendor_role_switch(peer_addr, 0x30);
                }
            }
            else
            {
                /* update bud link database */
                memcpy(btm_db.remote_link->bd_addr, p_info->remote_addr, 6);
                if (p_info->new_role == GAP_BR_LINK_ROLE_MASTER)
                {
                    btm_db.remote_link->acl_link_role_master = true;
                }
                else
                {
                    btm_db.remote_link->acl_link_role_master = false;
                }

                remote_peer_addr_set(p_info->remote_addr);
                remote_local_addr_set(p_info->local_addr);

                /* update bud profile database */
                rdtp_handle_roleswap(p_info->local_addr, p_info->remote_addr);

                memcpy(info.pre_bd_addr, p_info->local_addr, 6);
                memcpy(info.curr_bd_addr, p_info->remote_addr, 6);
                info.curr_link_role = p_info->new_role;
                payload.msg_buf = &info;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_ADDR_STATUS, &payload);

                BTM_PRINT_TRACE5("bt_sniffing_state_uncoordinated_roleswap: vnd role switch, role %d, recovery_link_type %d, "
                                 "stop_after_roleswap %d, remote addr %s, local addr %s",
                                 remote_session_role_get(), recovery_link_type, stop_after_roleswap,
                                 TRACE_BDADDR(p_info->remote_addr), TRACE_BDADDR(p_info->local_addr));

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    // bt_sniffing_set_continuous_trx(p_info->remote_addr, 0x00, 0);
                    bt_sniffing_stop_roleswap(audio_addr);
                }
                else
                {
                    bt_sniffing_resume_link(audio_addr);
                }

                bt_mgr_dispatch(BT_MSG_ROLESWAP_FLOW_GO, &payload);

                bt_sniffing_change_state(SNIFFING_STATE_IDLE);

                payload.msg_buf = &p_info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case SNIFFING_EVT_STOP_ROLESWAP:
        roleswap_terminate = true;
        break;

    default:
        break;
    }
}

void bt_sniffing_state_idle_roleswap(uint16_t  event,
                                     void     *p_data)
{
    uint8_t peer_addr[6];
    T_BT_SNIFFING_STATE_SYNC_INFO sync_info;

    BTM_PRINT_INFO2("bt_sniffing_state_idle_roleswap: event 0x%04x, recovery_link_type %d",
                    event, recovery_link_type);

    remote_peer_addr_get(peer_addr);

    switch (event)
    {
    case SNIFFING_EVT_START_ROLESWAP:
        {
            sync_info.type = BT_SNIFFING_ROLESWAP_REQ;
            sync_info.param.roleswap_req.sniffing_state = SNIFFING_STATE_IDLE_ROLESWAP;
            sync_info.param.roleswap_req.stop_after_roleswap = stop_after_roleswap;
            sync_info.param.roleswap_req.context = roleswap_req_context;
            bt_sniffing_sync_state(sync_info);
        }
        break;

    case SNIFFING_EVT_START_ROLESWAP_CFM:
        {
            T_BT_SNIFFING_ROLESWAP_RSP *rsp = (T_BT_SNIFFING_ROLESWAP_RSP *)p_data;

            if (rsp->accept == false)
            {
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            }
            else
            {
                bt_mgr_dispatch(BT_MSG_ROLESWAP_START, NULL);
            }

            sync_info.type = BT_SNIFFING_ROLESWAP_RSP;
            sync_info.param.roleswap_rsp.accept = rsp->accept;
            sync_info.param.roleswap_rsp.context = rsp->context;
            bt_sniffing_sync_state(sync_info);
        }
        break;

    case SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO:
        {
            T_BT_SNIFFING_STATE_SYNC_INFO *p_info = (T_BT_SNIFFING_STATE_SYNC_INFO *)p_data;

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY &&
                p_info->type == BT_SNIFFING_ROLESWAP_TERMINATE)
            {
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
            }
            else if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY &&
                     p_info->type == BT_SNIFFING_ROLESWAP_RSP)
            {
                T_BT_BR_LINK *p_link;
                T_BT_MSG_PAYLOAD payload;
                T_BT_ROLESWAP_RSP rsp;

                rsp.accept = p_info->param.roleswap_rsp.accept;
                rsp.context = p_info->param.roleswap_rsp.context;
                payload.msg_buf = &rsp;

                if (p_info->param.roleswap_rsp.accept == false)
                {
                    bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                    bt_mgr_dispatch(BT_MSG_ROLESWAP_RSP, &payload);
                    return;
                }

                p_link = bt_find_br_link(peer_addr);
                if (p_link)
                {
                    if (bt_sniff_mode_exit(p_link, false) == true)
                    {
                        bt_sniffing_vendor_role_switch(peer_addr, 0x30);
                        bt_sniffing_substate = SNIFFING_SUBSTATE_ROLE_SWITCH;
                    }
                    else
                    {
                        bt_sniffing_substate = SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING;
                    }

                    bt_mgr_dispatch(BT_MSG_ROLESWAP_RSP, &payload);
                }
            }
        }
        break;

    case SNIFFING_EVT_CTRL_DISCONNECT:
        {
            uint16_t cause = *(uint16_t *)p_data;
            T_BT_MSG_PAYLOAD payload;

            if (roleswap_terminate)
            {
                roleswap_terminate = false;
            }

            bt_sniffing_change_state(SNIFFING_STATE_IDLE);

            payload.msg_buf = &cause;
            bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
        }
        break;

    case SNIFFING_EVT_CTRL_LINK_ACTIVE:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            if (roleswap_terminate)
            {
                roleswap_terminate = false;
                bt_sniffing_change_state(SNIFFING_STATE_IDLE);
                sync_info.type = BT_SNIFFING_ROLESWAP_TERMINATE;
                sync_info.param.roleswap_terminate.sniffing_state = SNIFFING_STATE_IDLE;
                sync_info.param.roleswap_terminate.cause = HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE;
                bt_sniffing_sync_state(sync_info);
            }
            else if (bt_sniffing_substate == SNIFFING_SUBSTATE_LINK_ACTIVE_PENDING)
            {
                bt_sniffing_vendor_role_switch(peer_addr, 0x30);
                bt_sniffing_substate = SNIFFING_SUBSTATE_ROLE_SWITCH;
            }
        }
        break;

    case SNIFFING_EVT_VND_ROLE_SWITCH:
        {
            T_GAP_VENDOR_ROLE_SWITCH *p_info = (T_GAP_VENDOR_ROLE_SWITCH *)p_data;
            T_BT_MSG_PAYLOAD payload;
            T_GAP_HANDOVER_BUD_INFO info;

            if (p_info->cause)
            {
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    bt_sniffing_vendor_role_switch(peer_addr, 0x30);
                }
            }
            else
            {
                /* update bud link database */
                memcpy(btm_db.remote_link->bd_addr, p_info->remote_addr, 6);
                if (p_info->new_role == GAP_BR_LINK_ROLE_MASTER)
                {
                    btm_db.remote_link->acl_link_role_master = true;
                }
                else
                {
                    btm_db.remote_link->acl_link_role_master = false;
                }

                remote_peer_addr_set(p_info->remote_addr);
                remote_local_addr_set(p_info->local_addr);

                /* update bud profile database */
                rdtp_handle_roleswap(p_info->local_addr, p_info->remote_addr);

                memcpy(info.pre_bd_addr, p_info->local_addr, 6);
                memcpy(info.curr_bd_addr, p_info->remote_addr, 6);
                info.curr_link_role = p_info->new_role;
                payload.msg_buf = &info;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_ADDR_STATUS, &payload);

                BTM_PRINT_TRACE5("bt_sniffing_state_idle_roleswap: vnd role switch, role %d, recovery_link_type %d, "
                                 "stop_after_roleswap %d, remote addr %s, local addr %s",
                                 remote_session_role_get(), recovery_link_type, stop_after_roleswap,
                                 TRACE_BDADDR(p_info->remote_addr), TRACE_BDADDR(p_info->local_addr));

                if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    BTM_PRINT_TRACE0("bt_sniffing_state_idle_roleswap: primary change to secondary");
                    remote_session_role_set(REMOTE_SESSION_ROLE_SECONDARY);
                }
                else
                {
                    BTM_PRINT_TRACE0("bt_sniffing_state_idle_roleswap: secondary change to primary");
                    remote_session_role_set(REMOTE_SESSION_ROLE_PRIMARY);
                }

                bt_sniffing_change_state(SNIFFING_STATE_IDLE);

                payload.msg_buf = &p_info->cause;
                bt_mgr_dispatch(BT_MSG_ROLESWAP_CMPL, &payload);
            }
        }
        break;

    case SNIFFING_EVT_STOP_ROLESWAP:
        roleswap_terminate = true;
        break;

    default:
        break;
    }
}

void bt_sniffing_handle_evt(uint16_t  event,
                            void     *p_data)
{
    uint8_t peer_addr[6];

    remote_peer_addr_get(peer_addr);

    BTM_PRINT_INFO6("bt_sniffing_handle_evt: remote addr %s, role %d, bt_sniffing_state 0x%02x, sniffing_handle 0x%04x, "
                    "recovery_handle 0x%04x, event 0x%04x",
                    TRACE_BDADDR(peer_addr), remote_session_role_get(), bt_sniffing_state,
                    sniffing_handle, recovery_handle, event);

    switch (bt_sniffing_state)
    {
    case SNIFFING_STATE_IDLE:
        bt_sniffing_state_idle(event, p_data);
        break;

    case SNIFFING_STATE_SETUP_SNIFFING:
        bt_sniffing_state_setup_sniffing(event, p_data);
        break;

    case SNIFFING_STATE_SNIFFING_CONNECTED:
        bt_sniffing_state_sniffing_connected(event, p_data);
        break;

    case SNIFFING_STATE_SETUP_RECOVERY:
        bt_sniffing_state_setup_recovery(event, p_data);
        break;

    case SNIFFING_STATE_RECOVERY_CONNECTED:
        bt_sniffing_state_recovery_connected(event, p_data);
        break;

    case SNIFFING_STATE_DISCONN_RECOVERY:
        bt_sniffing_state_disconn_recovery(event, p_data);
        break;

    case SNIFFING_STATE_COORDINATED_ROLESWAP:
        bt_sniffing_state_coordinated_roleswap(event, p_data);
        break;

    case SNIFFING_STATE_UNCOORDINATED_ROLESWAP:
        bt_sniffing_state_uncoordinated_roleswap(event, p_data);
        break;

    case SNIFFING_STATE_IDLE_ROLESWAP:
        bt_sniffing_state_idle_roleswap(event, p_data);
        break;

    default:
        BTM_PRINT_ERROR2("bt_sniffing_handle_evt: unknown, bt_sniffing_state 0x%02x, event 0x%04x",
                         bt_sniffing_state, event);
        break;
    }
}

bool bt_sniffing_start_roleswap(uint8_t                      bd_addr[6],
                                uint8_t                      context,
                                bool                         stop_after_shadow,
                                P_REMOTE_ROLESWAP_SYNC_CBACK cback)
{
    BTM_PRINT_INFO2("bt_sniffing_start_roleswap: stop_after_shadow %d, bt_sniffing_state 0x%02x",
                    stop_after_shadow, bt_sniffing_state);

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY)
    {
        BTM_PRINT_ERROR1("bt_sniffing_start_roleswap: invalid role %d", remote_session_role_get());
        return false;
    }

    if ((bt_sniffing_state != SNIFFING_STATE_IDLE &&
         bt_sniffing_state != SNIFFING_STATE_SNIFFING_CONNECTED &&
         bt_sniffing_state != SNIFFING_STATE_RECOVERY_CONNECTED) ||
        bt_sniffing_substate != SNIFFING_SUBSTATE_IDLE)
    {
        return false;
    }

    roleswap_terminate = false;
    stop_after_roleswap = stop_after_shadow;
    roleswap_req_context = context;
    roleswap_callback = cback;
    memcpy(audio_addr, bd_addr, 6);

    bt_sniffing_handle_evt(SNIFFING_EVT_START_ROLESWAP, bd_addr);

    return true;
}

bool bt_sniffing_roleswap_cfm(bool    accept,
                              uint8_t context)
{
    T_BT_SNIFFING_ROLESWAP_RSP rsp;

    BTM_PRINT_INFO2("bt_sniffing_roleswap_cfm: accept %d, context %d", accept, context);

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_SECONDARY)
    {
        BTM_PRINT_ERROR1("bt_sniffing_roleswap_cfm: invalid role %d", remote_session_role_get());
        return false;
    }

    if (bt_sniffing_state != SNIFFING_STATE_IDLE_ROLESWAP &&
        bt_sniffing_state != SNIFFING_STATE_COORDINATED_ROLESWAP &&
        bt_sniffing_state != SNIFFING_STATE_UNCOORDINATED_ROLESWAP)
    {
        return false;
    }

    rsp.accept = accept;
    rsp.context = context;

    bt_sniffing_handle_evt(SNIFFING_EVT_START_ROLESWAP_CFM, &rsp);

    return true;
}

bool bt_sniffing_terminate_roleswap(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;
    T_REMOTE_SESSION_ROLE role;

    BTM_PRINT_INFO1("bt_sniffing_terminate_roleswap: bt_sniffing_state 0x%02x", bt_sniffing_state);

    role = remote_session_role_get();
    p_link = bt_find_br_link(bd_addr);

    if (role != REMOTE_SESSION_ROLE_PRIMARY || p_link == NULL)
    {
        BTM_PRINT_ERROR2("bt_sniffing_terminate_roleswap: invalid role %d, link %p",
                         role, p_link);
        return false;
    }

    if (bt_sniffing_substate >= SNIFFING_SUBSTATE_SHADOW_LINK)
    {
        BTM_PRINT_ERROR1("bt_sniffing_terminate_roleswap: invalid state %d",
                         bt_sniffing_substate);
        return false;
    }

    bt_sniffing_handle_evt(SNIFFING_EVT_STOP_ROLESWAP, bd_addr);

    return true;
}

bool bt_sniffing_del_spp(uint8_t  bd_addr[6],
                         uint8_t  local_server_chann,
                         uint16_t cause)
{
    T_BT_BR_LINK *p_link;
    T_BT_SPP_DISCONN_CMPL info;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (spp_del_roleswap_info(bd_addr, local_server_chann) == false)
    {
        return false;
    }

    p_link->spp_data.spp_ref--;
    if (p_link->spp_data.spp_ref == 0)
    {
        p_link->connected_profile &= ~SPP_PROFILE_MASK;
        p_link->spp_data.iap_authen_flag = 0;
    }

    info.cause = cause;
    info.local_server_chann = local_server_chann;

    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = &info;
    bt_mgr_dispatch(BT_MSG_SPP_DISCONN_CMPL, &payload);

    return true;
}

bool bt_sniffing_del_a2dp(uint8_t  bd_addr[6],
                          uint16_t cause)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (a2dp_del_roleswap_info(bd_addr) == false)
    {
        return false;
    }

    p_link->connected_profile &= ~A2DP_PROFILE_MASK;
    memset(&(p_link->a2dp_data), 0, sizeof(T_A2DP_LINK_DATA));

    payload.msg_buf = &cause;
    memcpy(payload.bd_addr, bd_addr, 6);
    bt_mgr_dispatch(BT_MSG_A2DP_DISCONN_CMPL, &payload);

    return true;
}

bool bt_sniffing_del_avrcp(uint8_t  bd_addr[6],
                           uint16_t cause)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (avrcp_del_roleswap_info(bd_addr) == false)
    {
        return false;
    }

    p_link->connected_profile &= ~AVRCP_PROFILE_MASK;

    p_link->avrcp_data.vol_change_registered = false;
    p_link->avrcp_data.play_status = 0;

    payload.msg_buf = &cause;
    memcpy(payload.bd_addr, bd_addr, 6);
    bt_mgr_dispatch(BT_MSG_AVRCP_DISCONN_CMPL, &payload);

    return true;
}

bool bt_sniffing_del_hfp(uint8_t  bd_addr[6],
                         uint16_t cause)
{
    T_BT_BR_LINK *p_link;
    T_BT_HFP_DISCONN_INFO info;
    T_BT_MSG_PAYLOAD payload;
    T_BT_MSG msg;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (hfp_del_roleswap_info(bd_addr) == false)
    {
        return false;
    }

    if (p_link->hfp_data.hsp_active_fg)
    {
        msg = BT_MSG_HSP_DISCONN_CMPL;
    }
    else
    {
        msg = BT_MSG_HFP_DISCONN_CMPL;
    }

    p_link->connected_profile &= ~(HFP_PROFILE_MASK | HSP_PROFILE_MASK);
    memset(&p_link->hfp_data, 0, sizeof(T_HFP_LINK_DATA));

    info.cause = cause;

    payload.msg_buf = &info;
    memcpy(payload.bd_addr, bd_addr, 6);
    bt_mgr_dispatch(msg, &payload);

    return true;
}

bool bt_sniffing_del_pbap(uint8_t  bd_addr[6],
                          uint16_t cause)
{
    T_BT_BR_LINK *p_link;
    T_PBAP_DISCONN_INFO info;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (pbap_del_roleswap_info(bd_addr) == false)
    {
        return false;
    }

    p_link->connected_profile &= ~PBAP_PROFILE_MASK;

    info.cause = cause;

    payload.msg_buf = &info;
    memcpy(payload.bd_addr, bd_addr, 6);
    bt_mgr_dispatch(BT_MSG_PBAP_DISCONN_CMPL, &payload);

    return true;
}

bool bt_sniffing_del_hid_device(uint8_t  bd_addr[6],
                                uint16_t cause)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (hid_device_del_roleswap_info(bd_addr) == false)
    {
        return false;
    }

    p_link->connected_profile &= ~HID_DEVICE_PROFILE_MASK;
    memset(&(p_link->hid_data), 0, sizeof(T_HID_LINK_DATA));

    payload.msg_buf = &cause;
    memcpy(payload.bd_addr, bd_addr, 6);
    bt_mgr_dispatch(BT_MSG_HID_DEVICE_DISCONN_CMPL, &payload);

    return true;
}

bool bt_sniffing_del_hid_host(uint8_t *bd_addr, uint16_t cause)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (hid_host_del_roleswap_info(bd_addr) == false)
    {
        return false;
    }

    p_link->connected_profile &= ~HID_HOST_PROFILE_MASK;
    memset(&(p_link->hid_data), 0, sizeof(T_HID_LINK_DATA));

    payload.msg_buf = &cause;
    memcpy(payload.bd_addr, bd_addr, 6);
    bt_mgr_dispatch(BT_MSG_HID_HOST_DISCONN_CMPL, &payload);

    return true;
}


bool bt_sniffing_del_att(uint8_t  bd_addr[6],
                         uint16_t cause)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    return att_del_roleswap_info(bd_addr);
}

bool bt_sniffing_del_iap(uint8_t  bd_addr[6],
                         uint16_t cause)
{
    T_BT_BR_LINK *p_link;
    T_IAP_DISCONN_INFO info;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (iap_del_roleswap_info(bd_addr) == false)
    {
        return false;
    }

    p_link->connected_profile &= ~IAP_PROFILE_MASK;

    info.cause = cause;

    payload.msg_buf = &info;
    memcpy(payload.bd_addr, bd_addr, 6);
    bt_mgr_dispatch(BT_MSG_IAP_DISCONN_CMPL, &payload);

    return true;
}

bool bt_sniffing_del_avp_control(uint8_t  bd_addr[6],
                                 uint16_t cause)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (avp_control_del_roleswap_info(bd_addr) == false)
    {
        return false;
    }

    return true;
}

bool bt_sniffing_del_avp_audio(uint8_t bd_addr[6], uint16_t cause)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (avp_audio_del_roleswap_info(bd_addr) == false)
    {
        return false;
    }

    return true;
}

bool bt_sniffing_del_bt_rfc(uint8_t bd_addr[6],
                            uint8_t server_chann)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    return bt_rfc_del_roleswap_info(bd_addr, server_chann);
}

void bt_sniffing_free_rfc(uint8_t bd_addr[6],
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
    rfc_del_data_roleswap_info(dlci, cid);

    //check if rfc ctrl channel info should be delete
    p_data = bt_find_roleswap_rfc_data_by_cid(bd_addr, cid);
    if (p_data)
    {
        return;
    }

    //no rfcomm data channel on this l2cap channel, free rfc ctrl and l2c
    p_data = bt_find_roleswap_rfc_ctrl(bd_addr, cid);
    if (p_data == NULL)
    {
        return;
    }

    bt_roleswap_free_info(bd_addr, p_data);

    rfc_del_ctrl_roleswap_info(bd_addr);

    bt_roleswap_free_l2c_info(bd_addr, cid);
    gap_br_del_handover_l2c_info(cid);
}

void bt_snffing_sync_acl_state(T_ROLESWAP_ACL_INFO *p_info)
{
    T_ROLESWAP_ACL_TRANSACT acl_transact;

    memcpy(acl_transact.bd_addr, p_info->bd_addr, 6);
    acl_transact.role = p_info->role;
    acl_transact.mode = p_info->mode;
    acl_transact.link_policy = p_info->link_policy;
    acl_transact.superv_tout = p_info->superv_tout;
    acl_transact.authen_state = p_info->authen_state;
    acl_transact.encrypt_state = p_info->encrypt_state;

    bt_roleswap_info_send(ROLESWAP_MODULE_ACL, ROLESWAP_ACL_UPDATE,
                          (uint8_t *)&acl_transact, sizeof(acl_transact));
}

void bt_sniffing_handle_acl_status(uint8_t   bd_addr[6],
                                   T_BT_MSG  msg,
                                   void     *buf)
{
    uint8_t peer_addr[6];
    T_REMOTE_SESSION_ROLE role;

    remote_peer_addr_get(peer_addr);
    role = remote_session_role_get();

    /* skip bud link addr */
    if (memcmp(bd_addr, peer_addr, 6))
    {
        T_ROLESWAP_DATA *p_data;

        if (role == REMOTE_SESSION_ROLE_SECONDARY)
        {
            if (msg == BT_MSG_ACL_CONN_DISCONN)
            {
                uint16_t cause = *(uint16_t *)buf;

                BTM_PRINT_TRACE2("bt_sniffing_handle_acl_status: secondary sniffing link disconn %s, audio addr %s",
                                 TRACE_BDADDR(bd_addr), TRACE_BDADDR(audio_addr));

                sniffing_handle = INVALID_HCI_HANDLE;
                recovery_link_type = RECOVERY_LINK_TYPE_NONE;
                bt_sniffing_handle_evt(SNIFFING_EVT_AUDIO_DISCONN, &cause);
            }

            return;
        }
        else if (role == REMOTE_SESSION_ROLE_SINGLE)
        {
            return;
        }

        switch (msg)
        {
        case BT_MSG_ACL_CONN_READY:
            {
                T_ROLESWAP_INFO *p_base;
                T_BT_BR_LINK *p_link;

                // primary register audio pm cback
                p_link = bt_find_br_link(bd_addr);
                if (p_link)
                {
                    bt_pm_cback_register(p_link->bd_addr, bt_roleswap_audio_cback);
                }

                p_base = bt_find_roleswap_info_base(bd_addr);
                if (p_base)
                {
                    BTM_PRINT_ERROR1("bt_sniffing_handle_acl_status: roleswap info base already exists %s",
                                     TRACE_BDADDR(bd_addr));
                    return;
                }

                p_base = bt_alloc_roleswap_info_base(bd_addr);
                if (p_base == NULL)
                {
                    BTM_PRINT_ERROR1("bt_sniffing_handle_acl_status: fail to alloc roleswap base %s",
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
            if (p_data)
            {
                p_data->u.acl.mode = 0;
                BTM_PRINT_TRACE1("bt_sniffing_handle_acl_status: mode %u", p_data->u.acl.mode);

                if (bt_sniffing_state == SNIFFING_STATE_COORDINATED_ROLESWAP)
                {
                    bt_snffing_sync_acl_state(&p_data->u.acl);
                }
            }
            break;

        case BT_MSG_ACL_CONN_SNIFF:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.mode = 2;
                BTM_PRINT_TRACE1("bt_sniffing_handle_acl_status: mode %u", p_data->u.acl.mode);

                if (bt_sniffing_state == SNIFFING_STATE_COORDINATED_ROLESWAP)
                {
                    bt_snffing_sync_acl_state(&p_data->u.acl);
                }
            }
            break;

        case BT_MSG_ACL_AUTHEN_SUCCESS:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.authen_state = true;
                BTM_PRINT_TRACE1("bt_sniffing_handle_acl_status: authen_state %u", p_data->u.acl.authen_state);
            }
            break;

        case BT_MSG_ACL_AUTHEN_FAIL:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.authen_state = false;
                BTM_PRINT_TRACE1("bt_sniffing_handle_acl_status: authen_state %u", p_data->u.acl.authen_state);

                gap_br_get_handover_sm_info(p_data->u.acl.bd_addr);
            }
            break;

        case BT_MSG_ACL_CONN_ENCRYPTED:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.encrypt_state = 1;
                BTM_PRINT_TRACE1("bt_sniffing_handle_acl_status: encrypt_state %u", p_data->u.acl.encrypt_state);

                gap_br_get_handover_sm_info(p_data->u.acl.bd_addr);
            }
            break;

        case BT_MSG_ACL_CONN_NOT_ENCRYPTED:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.encrypt_state = 0;
                BTM_PRINT_TRACE1("bt_sniffing_handle_acl_status: encrypt_state %u", p_data->u.acl.encrypt_state);

                gap_br_get_handover_sm_info(p_data->u.acl.bd_addr);
            }
            break;

        case BT_MSG_ACL_ROLE_MASTER:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.role = 0;
                BTM_PRINT_TRACE1("bt_sniffing_handle_acl_status: role %u", p_data->u.acl.role);
            }
            break;

        case BT_MSG_ACL_ROLE_SLAVE:
            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                p_data->u.acl.role = 1;
                BTM_PRINT_TRACE1("bt_sniffing_handle_acl_status: role %u", p_data->u.acl.role);
            }
            break;

        case BT_MSG_ACL_CONN_DISCONN:
            {
                uint16_t cause = *(uint16_t *)buf;

                BTM_PRINT_INFO2("bt_sniffing_handle_acl_status: disconnected %s, audio addr %s",
                                TRACE_BDADDR(bd_addr), TRACE_BDADDR(audio_addr));

                if (cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
                {
                    break;
                }

                if (!memcmp(bd_addr, audio_addr, 6))
                {
                    bt_sniffing_handle_evt(SNIFFING_EVT_AUDIO_DISCONN, &cause);
                    sniffing_handle = INVALID_HCI_HANDLE;
                }

                bt_roleswap_free_acl_info(bd_addr);
                bt_roleswap_info_send(ROLESWAP_MODULE_ACL, ROLESWAP_ACL_DISCONN, bd_addr, 6);
            }
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
        switch (msg)
        {
        case BT_MSG_ACL_CONN_DISCONN:
            {
                uint16_t cause = *(uint16_t *)buf;

                /* clear secondary roleswap database and disconn sniffing link */
                if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    uint8_t i;

                    for (i = 0; i < btm_db.br_link_num; i++)
                    {
                        T_ROLESWAP_INFO *p_info;

                        p_info = &btm_db.roleswap_info[i];
                        if (p_info->used == true)
                        {
                            bt_roleswap_free_acl_info(p_info->bd_addr);
                        }
                    }
                }

                bt_sniffing_handle_evt(SNIFFING_EVT_CTRL_DISCONNECT, &cause);

                BTM_PRINT_INFO0("bt_sniffing_handle_acl_status: set vhci pass all");
                vhci_set_filter(vhci_filter_all_pass);
            }
            break;

        case BT_MSG_ACL_CONN_READY:
            {
                T_BT_BR_LINK *p_link;

                p_link = bt_find_br_link(bd_addr);
                if (p_link)
                {
                    bt_pm_cback_register(p_link->bd_addr, bt_roleswap_ctrl_cback);
                }
            }
            break;

        default:
            break;
        }
    }
}

void bt_sniffing_handle_profile_conn(uint8_t  bd_addr[6],
                                     uint32_t profile_mask,
                                     uint8_t  param)
{
    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY)
    {
        return;
    }

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

            if (param == ROLESWAP_A2DP_PARAM_SINGAL)
            {
                bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_A2DP, (uint8_t *)&a2dp_info, sizeof(a2dp_info));
                bt_roleswap_info_send(ROLESWAP_MODULE_A2DP, ROLESWAP_A2DP_CONN,
                                      (uint8_t *)&a2dp_info, sizeof(a2dp_info));

                gap_br_get_handover_l2c_info(a2dp_info.sig_cid);
            }
            else
            {
                T_ROLESWAP_DATA *p_data;

                p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_A2DP);
                if (p_data == NULL)
                {
                    BTM_PRINT_ERROR0("bt_sniffing_handle_profile_conn: fail to find a2dp info");
                    return;
                }

                memcpy((uint8_t *)&p_data->u.a2dp, (uint8_t *)&a2dp_info, sizeof(a2dp_info));

                bt_roleswap_info_send(ROLESWAP_MODULE_A2DP, ROLESWAP_A2DP_STREAM_CONN,
                                      (uint8_t *)&a2dp_info, sizeof(a2dp_info));
                gap_br_get_handover_l2c_info(a2dp_info.stream_cid);
            }
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
                                   sizeof(hid_device_info));
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

            bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_HID_DEVICE, (uint8_t *)&hid_host_info,
                                   sizeof(hid_host_info));
            bt_roleswap_info_send(ROLESWAP_MODULE_HID_HOST, ROLESWAP_HID_HOST_CONN,
                                  (uint8_t *)&hid_host_info, sizeof(hid_host_info));

            gap_br_get_handover_l2c_info(hid_host_info.control_cid);
            gap_br_get_handover_l2c_info(hid_host_info.interrupt_cid);
        }
        break;

    case IAP_PROFILE_MASK:
        {
            T_ROLESWAP_IAP_INFO iap_info;

            if (bt_roleswap_get_iap_info(bd_addr, &iap_info) == false)
            {
                return;
            }

            bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_IAP, (uint8_t *)&iap_info, sizeof(iap_info));
            bt_roleswap_info_send(ROLESWAP_MODULE_IAP, ROLESWAP_IAP_CONN,
                                  (uint8_t *)&iap_info, sizeof(iap_info));

            bt_get_roleswap_rfc_info(bd_addr, iap_info.dlci, UUID_IAP);
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_handle_profile_disconn(uint8_t                           bd_addr[6],
                                        T_ROLESWAP_PROFILE_DISCONN_PARAM *p_param)
{
    uint8_t buf[9];

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
        p_param->cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
    {
        return;
    }

    memcpy(buf, bd_addr, 6);
    LE_UINT16_TO_ARRAY(buf + 6, p_param->cause);

    switch (p_param->profile_mask)
    {
    case SPP_PROFILE_MASK:
        buf[8] = p_param->param;
        bt_roleswap_free_spp_info(bd_addr, p_param->param);
        bt_roleswap_info_send(ROLESWAP_MODULE_SPP, ROLESWAP_SPP_DISCONN, buf, 9);
        break;

    case A2DP_PROFILE_MASK:
        if (p_param->param == ROLESWAP_A2DP_PARAM_SINGAL)
        {
            bt_roleswap_free_a2dp_info(bd_addr);
            bt_roleswap_info_send(ROLESWAP_MODULE_A2DP, ROLESWAP_A2DP_DISCONN, buf, 8);
        }
        else
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t stream_cid;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_A2DP);
            if (p_data == NULL)
            {
                return;
            }

            stream_cid = p_data->u.a2dp.stream_cid;
            p_data->u.a2dp.stream_cid = 0;
            bt_roleswap_free_l2c_info(bd_addr, stream_cid);

            bt_roleswap_info_send(ROLESWAP_MODULE_A2DP, ROLESWAP_A2DP_STREAM_DISCONN, buf, 8);
        }
        break;

    case AVRCP_PROFILE_MASK:
        bt_roleswap_free_avrcp_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_AVRCP, ROLESWAP_AVRCP_DISCONN, buf, 8);
        break;

    case HFP_PROFILE_MASK:
    case HSP_PROFILE_MASK:
        bt_roleswap_free_hfp_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_HFP, ROLESWAP_HFP_DISCONN, buf, 8);
        break;

    case PBAP_PROFILE_MASK:
        bt_roleswap_free_pbap_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_PBAP, ROLESWAP_PBAP_DISCONN, buf, 8);
        break;

    case HID_DEVICE_PROFILE_MASK:
        bt_roleswap_free_hid_device_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_HID_DEVICE, ROLESWAP_HID_DEVICE_DISCONN, buf, 8);
        break;

    case HID_HOST_PROFILE_MASK:
        bt_roleswap_free_hid_host_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_HID_HOST, ROLESWAP_HID_HOST_DISCONN, buf, 8);
        break;

    case IAP_PROFILE_MASK:
        bt_roleswap_free_iap_info(bd_addr);
        bt_roleswap_info_send(ROLESWAP_MODULE_IAP, ROLESWAP_IAP_DISCONN, buf, 8);
        break;

    default:
        break;
    }
}

void bt_sniffing_handle_sco_disconn(uint8_t  bd_addr[6],
                                    uint16_t cause)
{
    T_ROLESWAP_DATA *p_data;
    T_SNIFFING_SCO_DISCONN_PARAM param;

    if (cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
    {
        return;
    }

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_SCO);
    if (p_data == NULL)
    {
        return;
    }

    bt_roleswap_free_info(bd_addr, p_data);

    if (!memcmp(audio_addr, bd_addr, 6))
    {
        memcpy(param.bd_addr, bd_addr, 6);
        param.cause = cause;
        bt_sniffing_handle_evt(SNIFFING_EVT_SCO_DISCONNECT, &param);
    }
}

void bt_sniffing_handle_bt_rfc_conn(uint8_t bd_addr[6],
                                    uint8_t server_chann)
{
    T_ROLESWAP_BT_RFC_INFO info;

    if (bt_roleswap_get_bt_rfc_info(bd_addr, server_chann, &info) == false)
    {
        return;
    }

    bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_BT_RFC, (uint8_t *)&info, sizeof(info));
    bt_roleswap_info_send(ROLESWAP_MODULE_BT_RFC, ROLESWAP_BT_RFC_CONN, (uint8_t *)&info,
                          sizeof(T_ROLESWAP_BT_RFC_INFO));
    bt_get_roleswap_rfc_info(bd_addr, info.dlci, UUID_BTRFC);
}

void bt_sniffing_handle_bt_rfc_disconn(uint8_t  bd_addr[6],
                                       uint8_t  server_chann,
                                       uint16_t cause)
{
    uint8_t buf[9];

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
        cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
    {
        return;
    }

    memcpy(buf, bd_addr, 6);
    buf[6] = server_chann;
    LE_UINT16_TO_ARRAY(buf + 7, cause);

    bt_roleswap_free_bt_rfc_info(bd_addr, server_chann);
    bt_roleswap_info_send(ROLESWAP_MODULE_BT_RFC, ROLESWAP_BT_RFC_DISCONN, buf, 9);
}

void bt_sniffing_handle_bt_avp_control_conn(uint8_t bd_addr[6])
{
    T_ROLESWAP_AVP_INFO avp_info;
    T_ROLESWAP_DATA *p_data;

    if (bt_roleswap_get_avp_info(bd_addr, &avp_info) == false)
    {
        return;
    }

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_AVP);
    if (p_data != NULL)
    {
        memcpy((uint8_t *)&p_data->u.avp, (uint8_t *)&avp_info, sizeof(avp_info));
    }
    else
    {
        bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_AVP, (uint8_t *)&avp_info, sizeof(avp_info));
    }


    bt_roleswap_info_send(ROLESWAP_MODULE_AVP, ROLESWAP_AVP_CONTROL_CONN,
                          (uint8_t *)&avp_info, sizeof(avp_info));

    gap_br_get_handover_l2c_info(avp_info.control_l2c_cid);

}

void bt_sniffing_handle_bt_avp_audio_conn(uint8_t *bd_addr)
{
    T_ROLESWAP_AVP_INFO avp_info;
    T_ROLESWAP_DATA *p_data;

    if (bt_roleswap_get_avp_info(bd_addr, &avp_info) == false)
    {
        return;
    }

    p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_AVP);
    if (p_data != NULL)
    {
        memcpy((uint8_t *)&p_data->u.avp, (uint8_t *)&avp_info, sizeof(avp_info));
    }
    else
    {
        bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_AVP, (uint8_t *)&avp_info, sizeof(avp_info));
    }

    bt_roleswap_info_send(ROLESWAP_MODULE_AVP, ROLESWAP_AVP_AUDIO_CONN,
                          (uint8_t *)&avp_info, sizeof(avp_info));

    gap_br_get_handover_l2c_info(avp_info.audio_l2c_cid);
}

void bt_sniffing_handle_bt_avp_control_disconn(uint8_t *bd_addr, uint16_t cause)
{
    uint8_t buf[9];

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
        cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
    {
        return;
    }

    memcpy(buf, bd_addr, 6);
    LE_UINT16_TO_ARRAY(buf + 6, cause);
    bt_roleswap_free_avp_control_info(bd_addr);
    bt_roleswap_info_send(ROLESWAP_MODULE_AVP, ROLESWAP_AVP_CONTROL_DISCONN, buf, 8);
}

void bt_sniffing_handle_bt_avp_audio_disconn(uint8_t  bd_addr[6],
                                             uint16_t cause)
{
    uint8_t buf[9];

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
        cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
    {
        return;
    }

    memcpy(buf, bd_addr, 6);
    LE_UINT16_TO_ARRAY(buf + 6, cause);
    bt_roleswap_free_avp_audio_info(bd_addr);
    bt_roleswap_info_send(ROLESWAP_MODULE_AVP, ROLESWAP_AVP_AUDIO_DISCONN, buf, 8);
}

void bt_sniffing_handle_bt_att_conn(uint8_t bd_addr[6])
{
    T_ROLESWAP_ATT_INFO att_info;

    if (bt_roleswap_get_att_info(bd_addr, &att_info) == false)
    {
        return;
    }

    bt_roleswap_alloc_info(bd_addr, ROLESWAP_TYPE_ATT, (uint8_t *)&att_info, sizeof(att_info));
    bt_roleswap_info_send(ROLESWAP_MODULE_ATT, ROLESWAP_ATT_CONN, (uint8_t *)&att_info,
                          sizeof(att_info));

    gap_br_get_handover_l2c_info(att_info.l2c_cid);

}

void bt_sniffing_handle_bt_att_disconn(uint8_t  bd_addr[6],
                                       uint16_t cause)
{
    uint8_t buf[9];

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
        cause == (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
    {
        return;
    }

    memcpy(buf, bd_addr, 6);
    LE_UINT16_TO_ARRAY(buf + 6, cause);
    bt_roleswap_free_att_info(bd_addr);
    bt_roleswap_info_send(ROLESWAP_MODULE_ATT, ROLESWAP_ATT_DISCONN, buf, 8);
}

void bt_sniffing_handle_ctrl_conn(void)
{
    uint8_t i;

    /* bud link is created after source/primary link. */
    for (i = 0; i < btm_db.br_link_num; i++)
    {
        BTM_PRINT_TRACE3("bt_sniffing_handle_ctrl_conn: roleswap link %d, acl_link_state %d, connected_profile 0x%02x",
                         i, btm_db.br_link[i].acl_link_state,
                         btm_db.br_link[i].connected_profile);

        if ((btm_db.br_link[i].acl_link_state == BT_LINK_STATE_CONNECTED) &&
            ((btm_db.br_link[i].connected_profile & RDTP_PROFILE_MASK) == 0))
        {
            bt_roleswap_transfer(btm_db.br_link[i].bd_addr);
        }
    }

    bt_sniffing_handle_evt(SNIFFING_EVT_CTRL_CONNECT, NULL);
}

void bt_sniffing_recv_acl(uint8_t   submodule,
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
            if (p_base == NULL)
            {
                p_base = bt_alloc_roleswap_info_base(p->bd_addr);
            }

            if (p_base == NULL)
            {
                return;
            }

            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data)
            {
                // secondary rcv 0x39 evt before when setup sniffing link, update param except handle
                p_data->u.acl.bd_type = p->bd_type;
                p_data->u.acl.conn_type = p->conn_type;
                p_data->u.acl.role = p->role;
                p_data->u.acl.mode = p->mode;
                p_data->u.acl.link_policy = p->link_policy;
                p_data->u.acl.superv_tout = p->superv_tout;
                p_data->u.acl.authen_state = p->authen_state;
                p_data->u.acl.encrypt_state = p->encrypt_state;
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_ACL, p_info, len);
            }
        }
        break;

    case ROLESWAP_ACL_DISCONN:
        BTM_PRINT_TRACE1("bt_roleswap_recv_acl_info: disconn %s", TRACE_BDADDR(p_info));
        bt_roleswap_free_acl_info(p_info);  // profile/stack info will be deleted when recv disconn evt
        break;

    case ROLESWAP_ACL_UPDATE:
        {
            T_ROLESWAP_ACL_TRANSACT *p_transact;

            p_transact = (T_ROLESWAP_ACL_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_ACL);
            if (p_data == NULL)
            {
                return;
            }

            p_data->u.acl.role = p_transact->role;
            p_data->u.acl.mode = p_transact->mode;
            p_data->u.acl.link_policy = p_transact->link_policy;
            p_data->u.acl.superv_tout = p_transact->superv_tout;
            p_data->u.acl.authen_state = p_transact->authen_state;
            p_data->u.acl.encrypt_state = p_transact->encrypt_state;
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_l2c(uint8_t  submodule,
                          uint16_t  len,
                          uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_L2C_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_L2C_INFO *p;
            T_GAP_HANDOVER_L2C_INFO l2c;

            p = (T_ROLESWAP_L2C_INFO *)p_info;
            p_data = bt_find_roleswap_l2c(p->bd_addr, p->local_cid);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.l2c, p_info, sizeof(T_ROLESWAP_L2C_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_L2C, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_l2c: sniff link not exist");
                return;
            }

            l2c.local_cid = p->local_cid;
            l2c.remote_cid = p->remote_cid;
            l2c.local_mtu = p->local_mtu;
            l2c.remote_mtu = p->remote_mtu;
            l2c.local_mps = p->local_mps;
            l2c.remote_mps = p->remote_mps;
            l2c.psm = p->psm;
            l2c.role = p->role;
            l2c.mode = p->mode;
            memcpy(l2c.bd_addr, p->bd_addr, 6);

            gap_br_set_handover_l2c_info(&l2c);
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_sm(uint8_t  submodule,
                         uint16_t  len,
                         uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_SM_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_SM_INFO *p;
            T_GAP_HANDOVER_SM_INFO sm;

            p = (T_ROLESWAP_SM_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_SM);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.sm, p_info, sizeof(T_ROLESWAP_SM_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_SM, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_sm: sniff link not exist");
                return;
            }

            sm.mode = p->mode;
            sm.state = p->state;
            sm.sec_state = p->sec_state;
            sm.remote_authen = p->remote_authen;
            sm.remote_io = p->remote_io;
            memcpy(sm.bd_addr, p->bd_addr, 6);

            gap_br_set_handover_sm_info(&sm);
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
                STREAM_TO_ARRAY(&p_data->u.sm, p_info, sizeof(T_ROLESWAP_SM_INFO));
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_rfc(uint8_t  submodule,
                          uint16_t  len,
                          uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_RFC_CTRL_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_RFC_CTRL_INFO *p;

            p = (T_ROLESWAP_RFC_CTRL_INFO *)p_info;
            p_data = bt_find_roleswap_rfc_ctrl(p->bd_addr, p->l2c_cid);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.rfc_ctrl, p_info, sizeof(T_ROLESWAP_RFC_CTRL_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_RFC_CTRL, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_rfc: sniff link not exist");
                return;
            }
            bt_roleswap_set_rfc_ctrl_info(p->bd_addr, p);
        }
        break;

    case ROLESWAP_RFC_DATA_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_RFC_DATA_INFO *p;

            p = (T_ROLESWAP_RFC_DATA_INFO *)p_info;
            p_data = bt_find_roleswap_rfc_data(p->bd_addr, p->dlci);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.rfc_data, p_info, sizeof(T_ROLESWAP_RFC_DATA_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_RFC_DATA, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_rfc: sniff link not exist");
                return;
            }
            bt_roleswap_set_rfc_data_info(p->bd_addr, p);
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

void bt_sniffing_recv_spp(uint8_t  submodule,
                          uint16_t  len,
                          uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_SPP_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_SPP_INFO *p;
            T_BT_MSG_PAYLOAD payload;

            p = (T_ROLESWAP_SPP_INFO *)p_info;
            p_data = bt_find_roleswap_spp_by_dlci(p->bd_addr, p->rfc_dlci);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.spp, p_info, sizeof(T_ROLESWAP_SPP_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_SPP, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_spp: sniff link not exist");
                return;
            }
            bt_roleswap_set_spp_info(p->bd_addr, p);

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = p;
            bt_mgr_dispatch(BT_MSG_SNIFFING_SPP_CONN_CMPL, &payload);
        }
        break;

    case ROLESWAP_SPP_DISCONN:
        {
            uint8_t bd_addr[6];
            uint16_t cause;
            uint8_t local_server_chann;
            T_ROLESWAP_DATA *p_data;
            uint8_t dlci;

            memcpy(bd_addr, p_info, 6);
            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            LE_STREAM_TO_UINT8(local_server_chann, p_info);

            p_data = bt_find_roleswap_spp(bd_addr, local_server_chann);
            if (p_data == NULL)
            {
                return;
            }

            dlci = p_data->u.spp.rfc_dlci;

            bt_sniffing_del_spp(bd_addr, local_server_chann, cause);
            bt_roleswap_free_info(bd_addr, p_data);

            bt_sniffing_free_rfc(bd_addr, dlci);
        }
        break;

    case ROLESWAP_SPP_TRANSACT:
        {
            T_ROLESWAP_SPP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_SPP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_spp(p_transact->bd_addr, p_transact->local_server_chann);
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

void bt_sniffing_recv_a2dp(uint8_t  submodule,
                           uint16_t  len,
                           uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_A2DP_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_A2DP_INFO *p;
            T_BT_MSG_PAYLOAD payload;

            p = (T_ROLESWAP_A2DP_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_A2DP);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.a2dp, p_info, sizeof(T_ROLESWAP_A2DP_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_A2DP, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_a2dp: sniff link not exist");
                return;
            }

            bt_roleswap_set_a2dp_info(p->bd_addr, p);

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = p;
            bt_mgr_dispatch(BT_MSG_SNIFFING_A2DP_CONN_CMPL, &payload);
        }
        break;

    case ROLESWAP_A2DP_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t sig_cid;
            uint16_t stream_cid;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_A2DP);
            if (p_data == NULL)
            {
                return;
            }

            sig_cid = p_data->u.a2dp.sig_cid;
            stream_cid = p_data->u.a2dp.stream_cid;

            bt_roleswap_free_info(bd_addr, p_data);

            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_sniffing_del_a2dp(bd_addr, cause);

            bt_roleswap_free_l2c_info(bd_addr, sig_cid);
            gap_br_del_handover_l2c_info(sig_cid);

            if (stream_cid)
            {
                bt_roleswap_free_l2c_info(bd_addr, stream_cid);
                gap_br_del_handover_l2c_info(stream_cid);
            }
        }
        break;

    case ROLESWAP_A2DP_STREAM_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_A2DP_INFO *p;

            p = (T_ROLESWAP_A2DP_INFO *)p_info;

            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_A2DP);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.a2dp, p_info, sizeof(T_ROLESWAP_A2DP_INFO));
            }
            else
            {
                /* this would happen when b2b connect first */
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_A2DP, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_a2dp: sniff link not exist");
                return;
            }

            bt_roleswap_set_a2dp_info(p->bd_addr, p);

            BTM_PRINT_INFO4("bt_sniffing_recv_a2dp: sig_cid 0x%04x, stream_cid 0x%04x, sig_state 0x%04x, state 0x%04x",
                            p->sig_cid, p->stream_cid, p->sig_state, p->state);
        }
        break;

    case ROLESWAP_A2DP_STREAM_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t stream_cid;
            uint8_t *bd_addr = p_info;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_A2DP);
            if (p_data == NULL)
            {
                return;
            }

            stream_cid = p_data->u.a2dp.stream_cid;
            p_data->u.a2dp.stream_cid = 0;
            bt_roleswap_free_l2c_info(bd_addr, stream_cid);
            gap_br_del_handover_l2c_info(stream_cid);
        }
        break;

    case ROLESWAP_A2DP_TRANSACT:
        {
            T_ROLESWAP_A2DP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;
            uint8_t *buf;
            p_transact = (T_ROLESWAP_A2DP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_A2DP);
            if (p_data)
            {
                p_data->u.a2dp.sig_state = p_transact->sig_state;
                p_data->u.a2dp.state = p_transact->state;
                p_data->u.a2dp.tx_trans_label = p_transact->tx_trans_label;
                p_data->u.a2dp.rx_start_trans_label = p_transact->rx_start_trans_label;
                p_data->u.a2dp.last_seq_number = p_transact->last_seq_number;
                p_data->u.a2dp.cmd_flag = p_transact->cmd_flag;
                p_data->u.a2dp.codec_type = p_transact->codec_type;
                buf = (uint8_t *)&p_data->u.a2dp.codec_info;
                ARRAY_TO_STREAM(buf, &p_transact->codec_info, sizeof(T_A2DP_CODEC_INFO));

                BTM_PRINT_INFO2("bt_sniffing_recv_a2dp: sig_cid 0x%x, stream_cid %d",
                                p_data->u.a2dp.sig_cid, p_data->u.a2dp.stream_cid);
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_avrcp(uint8_t  submodule,
                            uint16_t  len,
                            uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_AVRCP_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_AVRCP_INFO *p;
            T_BT_MSG_PAYLOAD payload;

            p = (T_ROLESWAP_AVRCP_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_AVRCP);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.avrcp, p_info, sizeof(T_ROLESWAP_AVRCP_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_AVRCP, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_avrcp: sniff link not exist");
                return;
            }
            bt_roleswap_set_avrcp_info(p->bd_addr, p);

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = p;
            bt_mgr_dispatch(BT_MSG_SNIFFING_AVRCP_CONN_CMPL, &payload);
        }
        break;

    case ROLESWAP_AVRCP_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t l2c_cid;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_AVRCP);
            if (p_data == NULL)
            {
                return;
            }

            l2c_cid = p_data->u.avrcp.l2c_cid;

            bt_roleswap_free_info(bd_addr, p_data);

            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_sniffing_del_avrcp(bd_addr, cause);

            bt_roleswap_free_l2c_info(bd_addr, l2c_cid);
            gap_br_del_handover_l2c_info(l2c_cid);
        }
        break;

    case ROLESWAP_AVRCP_TRANSACT:
        {
            T_ROLESWAP_AVRCP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_AVRCP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_AVRCP);
            if (p_data == NULL)
            {
                return;
            }

            p_data->u.avrcp.avctp_state = p_transact->avctp_state;
            p_data->u.avrcp.state = p_transact->state;
            p_data->u.avrcp.play_status = p_transact->play_status;
            p_data->u.avrcp.cmd_credits = p_transact->cmd_credits;
            p_data->u.avrcp.transact_label = p_transact->transact_label;
            p_data->u.avrcp.vol_change_registered = p_transact->vol_change_registered;
            p_data->u.avrcp.vol_change_pending_transact = p_transact->vol_change_pending_transact;
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_hfp(uint8_t  submodule,
                          uint16_t  len,
                          uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_HFP_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_HFP_INFO *p;
            T_BT_MSG_PAYLOAD payload;

            p = (T_ROLESWAP_HFP_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_HFP);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.hfp, p_info, sizeof(T_ROLESWAP_HFP_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_HFP, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_hfp: sniff link not exist");
                return;
            }

            bt_roleswap_set_hfp_info(p->bd_addr, p);

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = p;
            bt_mgr_dispatch(BT_MSG_SNIFFING_HFP_CONN_CMPL, &payload);
        }
        break;

    case ROLESWAP_HFP_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint8_t dlci;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_HFP);
            if (p_data == NULL)
            {
                return;
            }

            dlci = p_data->u.hfp.rfc_dlci;

            bt_roleswap_free_info(bd_addr, p_data);

            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_sniffing_del_hfp(bd_addr, cause);

            bt_sniffing_free_rfc(bd_addr, dlci);
        }
        break;

    case ROLESWAP_HFP_TRANSACT:
        {
            T_ROLESWAP_HFP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_HFP_TRANSACT *)p_info;

            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_HFP);
            if (p_data == NULL)
            {
                return;
            }

            p_data->u.hfp.state = p_transact->state;
            p_data->u.hfp.bat_report_type = p_transact->bat_report_type;
            p_data->u.hfp.at_cmd_credits = p_transact->at_cmd_credits;
            p_data->u.hfp.call_status = p_transact->call_status;
            p_data->u.hfp.prev_call_status = p_transact->prev_call_status;
            p_data->u.hfp.service_status = p_transact->service_status;
            p_data->u.hfp.supported_features = p_transact->supported_features;
            p_data->u.hfp.codec_type = p_transact->codec_type;
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_pbap(uint8_t  submodule,
                           uint16_t  len,
                           uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_PBAP_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_PBAP_INFO *p;
            T_BT_MSG_PAYLOAD payload;

            p = (T_ROLESWAP_PBAP_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_PBAP);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.pbap, p_info, sizeof(T_ROLESWAP_PBAP_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_PBAP, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_pbap: sniff link not exist");
                return;
            }
            bt_roleswap_set_pbap_info(p->bd_addr, p);

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = p;
            bt_mgr_dispatch(BT_MSG_SNIFFING_PBAP_CONN_CMPL, &payload);
        }
        break;

    case ROLESWAP_PBAP_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t cid;
            uint8_t dlci;
            bool obex_over_l2c;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_PBAP);
            if (p_data == NULL)
            {
                return;
            }

            cid = p_data->u.pbap.l2c_cid;
            dlci = p_data->u.pbap.rfc_dlci;
            obex_over_l2c = p_data->u.pbap.obex_psm ? true : false;

            bt_roleswap_free_info(bd_addr, p_data);

            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_sniffing_del_pbap(bd_addr, cause);

            if (obex_over_l2c)
            {
                bt_roleswap_free_l2c_info(bd_addr, cid);
                gap_br_del_handover_l2c_info(cid);
            }
            else
            {
                bt_sniffing_free_rfc(bd_addr, dlci);
            }
        }
        break;

    case ROLESWAP_PBAP_TRANSACT:
        {
            T_ROLESWAP_PBAP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_PBAP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_PBAP);
            if (p_data == NULL)
            {
                return;
            }

            p_data->u.pbap.obex_state = p_transact->obex_state;
            p_data->u.pbap.pbap_state = p_transact->pbap_state;
            p_data->u.pbap.path = p_transact->path;
            p_data->u.pbap.repos = p_transact->repos;
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_hid_device(uint8_t   submodule,
                                 uint16_t  len,
                                 uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_HID_DEVICE_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_HID_DEVICE_INFO *p;
            T_BT_MSG_PAYLOAD payload;

            p = (T_ROLESWAP_HID_DEVICE_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_HID_DEVICE);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.hid_device, p_info, sizeof(T_ROLESWAP_HID_DEVICE_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_HID_DEVICE, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_hid_device: sniff link not exist");
                return;
            }
            bt_roleswap_set_hid_device_info(p->bd_addr, p);

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = p;
            bt_mgr_dispatch(BT_MSG_SNIFFING_HID_DEVICE_CONN_CMPL, &payload);
        }
        break;

    case ROLESWAP_HID_DEVICE_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t control_cid;
            uint16_t interrupt_cid;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_HID_DEVICE);
            if (p_data == NULL)
            {
                return;
            }

            control_cid = p_data->u.hid_device.control_cid;
            interrupt_cid = p_data->u.hid_device.interrupt_cid;

            bt_roleswap_free_info(bd_addr, p_data);

            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_sniffing_del_hid_device(bd_addr, cause);

            bt_roleswap_free_l2c_info(bd_addr, control_cid);
            gap_br_del_handover_l2c_info(control_cid);

            bt_roleswap_free_l2c_info(bd_addr, interrupt_cid);
            gap_br_del_handover_l2c_info(interrupt_cid);
        }
        break;

    case ROLESWAP_HID_DEVICE_TRANSACT:
        {
            T_ROLESWAP_HID_DEVICE_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;
            p_transact = (T_ROLESWAP_HID_DEVICE_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_HID_DEVICE);
            if (p_data)
            {
                p_data->u.hid_device.control_state = p_transact->control_state;
                p_data->u.hid_device.interrupt_state = p_transact->interrupt_state;
                p_data->u.hid_device.proto_mode = p_transact->proto_mode;
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_hid_host(uint8_t submodule, uint16_t len, uint8_t *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_HID_HOST_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_HID_HOST_INFO *p;
            T_BT_MSG_PAYLOAD payload;

            p = (T_ROLESWAP_HID_HOST_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_HID_HOST);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.hid_host, p_info, sizeof(T_ROLESWAP_HID_HOST_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_HID_HOST, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_hid_host: sniff link not exist");
                return;
            }
            bt_roleswap_set_hid_host_info(p->bd_addr, p);

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = p;
            bt_mgr_dispatch(BT_MSG_SNIFFING_HID_HOST_CONN_CMPL, &payload);
        }
        break;

    case ROLESWAP_HID_HOST_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t control_cid;
            uint16_t interrupt_cid;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_HID_HOST);
            if (p_data == NULL)
            {
                return;
            }

            control_cid = p_data->u.hid_host.control_cid;
            interrupt_cid = p_data->u.hid_host.interrupt_cid;

            bt_roleswap_free_info(bd_addr, p_data);

            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_sniffing_del_hid_host(bd_addr, cause);

            bt_roleswap_free_l2c_info(bd_addr, control_cid);
            gap_br_del_handover_l2c_info(control_cid);

            bt_roleswap_free_l2c_info(bd_addr, interrupt_cid);
            gap_br_del_handover_l2c_info(interrupt_cid);
        }
        break;

    case ROLESWAP_HID_HOST_TRANSACT:
        {
            T_ROLESWAP_HID_HOST_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;
            p_transact = (T_ROLESWAP_HID_HOST_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_HID_HOST);
            if (p_data)
            {
                p_data->u.hid_host.control_state = p_transact->control_state;
                p_data->u.hid_host.interrupt_state = p_transact->interrupt_state;
                p_data->u.hid_host.proto_mode = p_transact->proto_mode;
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_att(uint8_t  submodule,
                          uint16_t  len,
                          uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_ATT_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_ATT_INFO *p;

            p = (T_ROLESWAP_ATT_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_ATT);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.att, p_info, sizeof(T_ROLESWAP_ATT_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_ATT, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_att: sniff link not exist");
                return;
            }

            bt_roleswap_set_att_info(p->bd_addr, p);
        }
        break;

    case ROLESWAP_ATT_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t cid;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_ATT);
            if (p_data == NULL)
            {
                return;
            }

            cid = p_data->u.att.l2c_cid;

            bt_roleswap_free_info(bd_addr, p_data);
            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_sniffing_del_att(bd_addr, cause);
            gap_br_del_handover_gatt_info(bd_addr, cid);

            bt_roleswap_free_l2c_info(bd_addr, cid);
            gap_br_del_handover_l2c_info(cid);
        }
        break;

    case ROLESWAP_ATT_TRANSACT:
        {
            T_ROLESWAP_ATT_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_ATT_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_ATT);
            if (p_data == NULL)
            {
                return;
            }

            p_data->u.att.state = p_transact->state;
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_iap(uint8_t  submodule,
                          uint16_t  len,
                          uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_IAP_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_IAP_INFO *p;
            T_BT_MSG_PAYLOAD payload;

            p = (T_ROLESWAP_IAP_INFO *)p_info;
            p_data = bt_find_roleswap_data(p->bd_addr, ROLESWAP_TYPE_IAP);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.iap, p_info, sizeof(T_ROLESWAP_IAP_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_IAP, p_info, len);
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_iap: sniff link not exist");
                return;
            }
            bt_roleswap_set_iap_info(p->bd_addr, p);

            memcpy(payload.bd_addr, p->bd_addr, 6);
            payload.msg_buf = p;
            bt_mgr_dispatch(BT_MSG_SNIFFING_IAP_CONN_CMPL, &payload);
        }
        break;

    case ROLESWAP_IAP_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint8_t dlci;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_IAP);
            if (p_data == NULL)
            {
                return;
            }

            dlci = p_data->u.iap.dlci;

            bt_roleswap_free_info(bd_addr, p_data);

            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_sniffing_del_iap(bd_addr, cause);

            bt_sniffing_free_rfc(bd_addr, dlci);
        }
        break;

    case ROLESWAP_IAP_TRANSACT:
        {
            T_ROLESWAP_IAP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_IAP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_IAP);
            if (p_data == NULL)
            {
                return;
            }

            p_data->u.iap.remote_credit = p_transact->remote_credit;
            p_data->u.iap.state = p_transact->state;
            p_data->u.iap.acc_pkt_seq = p_transact->acc_pkt_seq;
            p_data->u.iap.acked_seq = p_transact->acked_seq;
            p_data->u.iap.dev_pkt_seq = p_transact->dev_pkt_seq;
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_avp(uint8_t  submodule,
                          uint16_t  len,
                          uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_AVP_CONTROL_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_AVP_INFO *p;

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

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_avp: sniff link not exist");
                return;
            }
            bt_roleswap_set_avp_info(p->bd_addr, p);
        }
        break;

    case ROLESWAP_AVP_CONTROL_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t cid;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_AVP);
            if (p_data == NULL)
            {
                return;
            }

            cid = p_data->u.avp.control_l2c_cid;

            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_roleswap_free_avp_control_info(bd_addr);
            bt_sniffing_del_avp_control(bd_addr, cause);
            gap_br_del_handover_l2c_info(cid);
        }
        break;

    case ROLESWAP_AVP_AUDIO_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_AVP_INFO *p;

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

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_avp: sniff link not exist");
                return;
            }
            bt_roleswap_set_avp_info(p->bd_addr, p);
        }
        break;

    case ROLESWAP_AVP_AUDIO_DISCONN:
        {
            T_ROLESWAP_DATA *p_data;
            uint16_t cid;
            uint8_t *bd_addr = p_info;
            uint16_t cause;

            p_data = bt_find_roleswap_data(bd_addr, ROLESWAP_TYPE_AVP);
            if (p_data == NULL)
            {
                return;
            }

            cid = p_data->u.avp.audio_l2c_cid;

            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT16(cause, p_info);
            bt_roleswap_free_avp_audio_info(bd_addr);
            bt_sniffing_del_avp_audio(bd_addr, cause);
            gap_br_del_handover_l2c_info(cid);
        }
        break;

    case ROLESWAP_AVP_TRANSACT:
        {
            T_ROLESWAP_AVP_TRANSACT *p_transact;
            T_ROLESWAP_DATA *p_data;

            p_transact = (T_ROLESWAP_AVP_TRANSACT *)p_info;
            p_data = bt_find_roleswap_data(p_transact->bd_addr, ROLESWAP_TYPE_AVP);
            if (p_data == NULL)
            {
                return;
            }

            p_data->u.avp.control_state = p_transact->control_state;
            p_data->u.avp.audio_state = p_transact->audio_state;
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_ctrl(uint8_t  submodule,
                           uint16_t  len,
                           uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_CTRL_TOKEN_REQ:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
        {
            LE_STREAM_TO_UINT8(stop_after_roleswap, p_info);

            bt_roleswap_info_send(ROLESWAP_MODULE_CTRL, ROLESWAP_CTRL_TOKEN_RSP, NULL, 0);
        }
        break;

    case ROLESWAP_CTRL_TOKEN_RSP:
        bt_sniffing_handle_evt(SNIFFING_EVT_ROLESWAP_TOKEN_RSP, NULL);
        break;

    case ROLESWAP_CTRL_DISCONN_SNIFFING_REQ:
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
        {
            if (bt_sniffing_state == SNIFFING_STATE_RECOVERY_CONNECTED)
            {
                bt_sniffing_disconn_recovery_link(audio_addr, HCI_ERR_REMOTE_USER_TERMINATE);
                bt_roleswap_info_send(ROLESWAP_MODULE_CTRL, ROLESWAP_CTRL_DISCONN_SNIFFING_REQ,
                                      audio_addr, 6);
            }
        }
        else if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
        {
            if ((bt_sniffing_state == SNIFFING_STATE_SNIFFING_CONNECTED) ||
                (bt_sniffing_state == SNIFFING_STATE_SETUP_RECOVERY) ||
                (bt_sniffing_state == SNIFFING_STATE_RECOVERY_CONNECTED))
            {
                gap_br_disconn_sniffing_link(sniffing_handle, HCI_ERR_REMOTE_USER_TERMINATE);
            }
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv_bt_rfc(uint8_t  submodule,
                             uint16_t  len,
                             uint8_t  *p_info)
{
    switch (submodule)
    {
    case ROLESWAP_BT_RFC_CONN:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_BT_RFC_INFO *p = (T_ROLESWAP_BT_RFC_INFO *)p_info;

            p_data = bt_find_roleswap_bt_rfc_by_dlci(p->bd_addr, p->dlci);
            if (p_data)
            {
                STREAM_TO_ARRAY(&p_data->u.bt_rfc, p_info, sizeof(T_ROLESWAP_BT_RFC_INFO));
            }
            else
            {
                bt_roleswap_alloc_info(p->bd_addr, ROLESWAP_TYPE_BT_RFC, p_info, sizeof(T_ROLESWAP_BT_RFC_INFO));
            }

            if (sniffing_handle == INVALID_HCI_HANDLE)
            {
                BTM_PRINT_TRACE0("bt_sniffing_recv_bt_rfc: sniff link not exist");
                return;
            }
            bt_roleswap_set_bt_rfc_info(p->bd_addr, p);
        }
        break;

    case ROLESWAP_BT_RFC_DISCONN:
        {
            uint8_t bd_addr[6];
            uint8_t local_server_chann;
            uint8_t dlci;
            T_ROLESWAP_DATA *p_data;

            memcpy(bd_addr, p_info, 6);
            STREAM_SKIP_LEN(p_info, 6);
            LE_STREAM_TO_UINT8(local_server_chann, p_info);

            p_data = bt_find_roleswap_bt_rfc(bd_addr, local_server_chann);
            if (p_data == NULL)
            {
                return;
            }

            dlci = p_data->u.bt_rfc.dlci;
            bt_sniffing_del_bt_rfc(bd_addr, local_server_chann);
            bt_roleswap_free_info(bd_addr, p_data);

            bt_sniffing_free_rfc(bd_addr, dlci);
        }
        break;

    default:
        break;
    }
}

void bt_sniffing_recv(uint8_t  *p_data,
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
            BTM_PRINT_ERROR2("bt_sniffing_recv: excepted len %u, remaining data_len %d",
                             len, data_len);
            return;
        }

        BTM_PRINT_INFO2("bt_sniffing_recv: module %u, submodule %d", module, submodule);

        switch (module)
        {
        case ROLESWAP_MODULE_ACL:
            bt_sniffing_recv_acl(submodule, len, p);
            break;

        case ROLESWAP_MODULE_L2C:
            bt_sniffing_recv_l2c(submodule, len, p);
            break;

        case ROLESWAP_MODULE_SM:
            bt_sniffing_recv_sm(submodule, len, p);
            break;

        case ROLESWAP_MODULE_RFC:
            bt_sniffing_recv_rfc(submodule, len, p);
            break;

        case ROLESWAP_MODULE_SPP:
            bt_sniffing_recv_spp(submodule, len, p);
            break;

        case ROLESWAP_MODULE_A2DP:
            bt_sniffing_recv_a2dp(submodule, len, p);
            break;

        case ROLESWAP_MODULE_AVRCP:
            bt_sniffing_recv_avrcp(submodule, len, p);
            break;

        case ROLESWAP_MODULE_HFP:
            bt_sniffing_recv_hfp(submodule, len, p);
            break;

        case ROLESWAP_MODULE_PBAP:
            bt_sniffing_recv_pbap(submodule, len, p);
            break;

        case ROLESWAP_MODULE_HID_DEVICE:
            bt_sniffing_recv_hid_device(submodule, len, p);
            break;

        case ROLESWAP_MODULE_HID_HOST:
            bt_sniffing_recv_hid_host(submodule, len, p);
            break;

        case ROLESWAP_MODULE_IAP:
            bt_sniffing_recv_iap(submodule, len, p);
            break;

        case ROLESWAP_MODULE_AVP:
            bt_sniffing_recv_avp(submodule, len, p);
            break;

        case ROLESWAP_MODULE_ATT:
            bt_sniffing_recv_att(submodule, len, p);
            break;

        case ROLESWAP_MODULE_CTRL:
            bt_sniffing_recv_ctrl(submodule, len, p);
            break;

        case ROLESWAP_MODULE_BT_RFC:
            bt_sniffing_recv_bt_rfc(submodule, len, p);
            break;

        default:
            BTM_PRINT_ERROR2("bt_sniffing_recv: unknown module %u, submodule %u", module, submodule);
            break;
        }

        data_len -= len + ROLESWAP_MSG_HDR_LEN;
        p += len;
    }
}

void bt_sniffing_cback(void                       *p_buf,
                       T_GAP_BR_HANDOVER_MSG_TYPE  msg)
{
    BTM_PRINT_TRACE1("bt_sniffing_cback: message 0x%02x", msg);

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

            BTM_PRINT_TRACE4("bt_sniffing_cback: GAP_BR_GET_ACL_INFO_RSP, conn_type 0x%x, handle %d, mode %d, role %d",
                             acl.conn_type, acl.handle, acl.mode, acl.role);

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

            bt_sniffing_handle_evt(SNIFFING_EVT_SCO_CONNECT, p_info->bd_addr);
        }
        break;

    case GAP_BR_GET_SM_INFO_RSP:
        {
            T_ROLESWAP_DATA *p_data;
            T_ROLESWAP_SM_INFO sm;
            T_GAP_HANDOVER_SM_INFO *p_info = (T_GAP_HANDOVER_SM_INFO *)p_buf;
            T_BT_BR_LINK *p_link;

            p_link = bt_find_br_link(p_info->bd_addr);
            if ((p_link == NULL) || (p_link->acl_link_state != BT_LINK_STATE_CONNECTED))
            {
                break;
            }

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

            if (bt_roleswap_check_l2c_cid(p_info->bd_addr, p_info->local_cid) == false)
            {
                break;
            }

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

    case GAP_BR_SNIFFING_STATE_SYNC_INFO:
        {
            T_GAP_SNIFFING_STATE_SYNC_INFO *p_info = (T_GAP_SNIFFING_STATE_SYNC_INFO *)p_buf;

            if (p_info->param_len == sizeof(T_BT_SNIFFING_STATE_SYNC_INFO))
            {
                bt_sniffing_handle_evt(SNIFFING_EVT_SNIFFING_STATE_SYNC_INFO, p_info->param);
            }
            else
            {
                BTM_PRINT_TRACE1("bt_sniffing_cback: sniffing_state_sync info, len err %d",
                                 p_info->param_len);
            }
        }
        break;

    case GAP_BR_VENDOR_ROLE_SWITCH:
        {
            T_GAP_VENDOR_ROLE_SWITCH *p_info = (T_GAP_VENDOR_ROLE_SWITCH *)p_buf;
            BTM_PRINT_TRACE6("bt_sniffing_cback: vnd role switch, cause 0x%04x, bt_sniffing_state 0x%04x, role %d,"
                             "link new_role %d, local_address %s, remote_address %s",
                             p_info->cause, bt_sniffing_state, remote_session_role_get(), p_info->new_role,
                             TRACE_BDADDR(p_info->local_addr), TRACE_BDADDR(p_info->remote_addr));

            bt_sniffing_handle_evt(SNIFFING_EVT_VND_ROLE_SWITCH, p_buf);
        }
        break;

    case GAP_BR_SET_ACL_ACTIVE_STATE_RSP:
        {
            T_GAP_SET_ACL_ACTIVE_STATE_RSP *p_rsp = (T_GAP_SET_ACL_ACTIVE_STATE_RSP *)p_buf;
            if (p_rsp->cause)
            {
                BTM_PRINT_ERROR1("bt_sniffing_cback: set acl active state fail, cause 0x%04x",
                                 p_rsp->cause);
            }

            bt_sniffing_handle_evt(SNIFFING_EVT_SET_ACTIVE_STATE_RSP, p_buf);
        }
        break;

    case GAP_BR_SHADOW_LINK_RSP:
        {
            T_GAP_SHADOW_LINK_RSP *p_rsp = (T_GAP_SHADOW_LINK_RSP *)p_buf;

            BTM_PRINT_INFO1("bt_sniffing_cback: shadow link cause 0x%04x", p_rsp->cause);

            bt_sniffing_handle_evt(SNIFFING_EVT_SHADOW_LINK_RSP, p_buf);
        }
        break;

    case GAP_BR_ACL_SUSPEND_RX_EMPTY_INFO:
        {
            T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *p_info = (T_GAP_ACL_SUSPEND_RX_EMPTY_INFO *)p_buf;

            BTM_PRINT_INFO2("bt_sniffing_cback: acl suspend rx empty cause 0x%04x, handle 0x%04x",
                            p_info->cause, p_info->handle);

            bt_sniffing_handle_evt(SNIFFING_EVT_ACL_RX_EMPTY, p_buf);
        }
        break;

    case GAP_BR_HANDOVER_CONN_CMPL_INFO:
        {
            T_GAP_HANDOVER_CONN_CMPL_INFO *p_info = (T_GAP_HANDOVER_CONN_CMPL_INFO *)p_buf;
            if (p_info->cause)
            {
                BTM_PRINT_ERROR1("bt_sniffing_cback: hovr conn fail, cause 0x%04x",
                                 p_info->cause);
                return;
            }

            BTM_PRINT_INFO1("bt_sniffing_cback: hovr conn success, link_type %d",  p_info->link_type);

            if (remote_session_role_get() == REMOTE_SESSION_ROLE_PRIMARY)
            {
                break;
            }

            if (p_info->link_type == 0 || p_info->link_type == 2)   // sco or esco
            {
                T_ROLESWAP_SCO_INFO sco;
                T_BT_BR_LINK *p_link;

                sco.handle = p_info->handle;
                sco.type = p_info->link_type;
                sco.air_mode = p_info->esco_air_mode;
                LE_ARRAY_TO_UINT16(sco.pkt_len, p_info->esco_rx_packet_length);
                memcpy(sco.bd_addr, p_info->bd_addr, 6);

                bt_roleswap_alloc_info(p_info->bd_addr, ROLESWAP_TYPE_SCO, (uint8_t *)&sco,
                                       sizeof(T_ROLESWAP_SCO_INFO));
                p_link = bt_find_br_link(p_info->bd_addr);
                if (p_link)
                {
                    T_BT_MSG_PAYLOAD payload;

                    bt_roleswap_set_sco_info(p_info->bd_addr, &sco);

                    memcpy(payload.bd_addr, p_info->bd_addr, 6);
                    payload.msg_buf = &sco;
                    bt_mgr_dispatch(BT_MSG_SNIFFING_SCO_CONN_CMPL, &payload);
                }

                bt_sniffing_handle_evt(SNIFFING_EVT_SCO_CONNECT, p_info->bd_addr);
            }
            else    //acl
            {
                T_ROLESWAP_INFO *p_base;
                T_ROLESWAP_DATA *p_data;

                p_base = bt_find_roleswap_info_base(p_info->bd_addr);
                if (p_base == NULL)
                {
                    p_base = bt_alloc_roleswap_info_base(p_info->bd_addr);
                }

                if (p_base == NULL)
                {
                    BTM_PRINT_ERROR1("bt_sniffing_cback: fail to alloc roleswap base for addr %s",
                                     TRACE_BDADDR(p_info->bd_addr));
                    return;
                }

                p_data = bt_find_roleswap_data(p_info->bd_addr, ROLESWAP_TYPE_ACL);
                if (p_data == NULL)
                {
                    T_ROLESWAP_ACL_INFO acl_data;

                    acl_data.authen_state = true;

                    acl_data.handle = p_info->handle;
                    acl_data.encrypt_state = p_info->encrypt_enabled;
                    acl_data.conn_type = 0x00;  /* CONN_TYPE_BR */
                    acl_data.bd_type = 0x10;    /* REMOTE_ADDR_CLASSIC */
                    acl_data.role = p_info->acl_role;       /* slave */
                    memcpy(acl_data.bd_addr, p_info->bd_addr, 6);

                    bt_roleswap_alloc_info(p_info->bd_addr, ROLESWAP_TYPE_ACL,
                                           (uint8_t *)&acl_data, sizeof(T_ROLESWAP_ACL_INFO));
                }
                else
                {
                    p_data->u.acl.handle = p_info->handle;
                    p_data->u.acl.encrypt_state = p_info->encrypt_enabled;
                }

                sniffing_handle = p_info->handle;
                memcpy(audio_addr, p_info->bd_addr, 6);

                bt_sniffing_handle_evt(SNIFFING_EVT_HANDOVER_CONN_CMPL, NULL);
            }
        }
        break;

    case GAP_BR_HANDOVER_CMPL_INFO:
        {
            T_GAP_HANDOVER_CMPL_INFO *p_info = (T_GAP_HANDOVER_CMPL_INFO *)p_buf;

            BTM_PRINT_TRACE3("bt_sniffing_cback: GAP_BR_HANDOVER_CMPL_INFO, role %d, bt_sniffing_state %d, cause 0x%04x",
                             remote_session_role_get(), bt_sniffing_state, p_info->cause);

            bt_sniffing_handle_evt(SNIFFING_EVT_HANDOVER_CMPL, p_info);
        }
        break;

    case GAP_BR_SHADOW_CTRL_LINK_CHANGE_INFO:
        {
            T_GAP_SHADOW_CTRL_LINK_CHANGE_INFO *p_info = (T_GAP_SHADOW_CTRL_LINK_CHANGE_INFO *)p_buf;
            BTM_PRINT_TRACE3("bt_sniffing_cback: ctrl link change info, cause 0x%04x, target_handle 0x%04x, ctrl_handle 0x%04x",
                             p_info->cause, p_info->target_handle, p_info->ctrl_handle);
        }
        break;

    case GAP_BR_SHADOW_SNIFFING_MODE_CHANGE_INFO:
        {
            T_GAP_SHADOW_SNIFFING_MODE_CHANGE_INFO *p_info = (T_GAP_SHADOW_SNIFFING_MODE_CHANGE_INFO *)p_buf;
            BTM_PRINT_TRACE3("bt_sniffing_cback: sniffing mode change info, cause 0x%04x, target_handle 0x%04x, current_sniffing_mode %d",
                             p_info->cause, p_info->target_handle, p_info->current_sniffing_mode);

            bt_sniffing_handle_evt(SNIFFING_EVT_SNIFFING_MODE_CHANGE, p_buf);
        }
        break;

    case GAP_BR_SHADOW_PRE_SYNC_INFO_RSP:
        {
            T_GAP_SHADOW_PRE_SYNC_INFO_RSP *p_rsp = (T_GAP_SHADOW_PRE_SYNC_INFO_RSP *)p_buf;
            BTM_PRINT_TRACE1("bt_sniffing_cback: pre sync info cause 0x%04x", p_rsp->cause);
        }
        break;

    case GAP_BR_AUDIO_RECOVERY_LINK_REQ_IND:
        {
            bt_sniffing_handle_evt(SNIFFING_EVT_RECOVERY_CONN_REQ, p_buf);
        }
        break;

    case GAP_BR_AUDIO_RECOVERY_LINK_CONN_CMPL_INFO:
        {
            T_GAP_AUDIO_RECOVERY_LINK_CONN_CMPL_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_CONN_CMPL_INFO *)p_buf;

            if (p_info->cause == 0 && p_info->audio_type == RECOVERY_LINK_TYPE_A2DP)
            {
                a2dp_recovery_interval = p_info->a2dp_interval;
                a2dp_recovery_flush_tout = p_info->a2dp_flush_tout;
                a2dp_recovery_rsvd_slot = p_info->a2dp_rsvd_slot;
                a2dp_recovery_idle_slot = p_info->a2dp_idle_slot;
                a2dp_recovery_idle_skip = p_info->a2dp_idle_skip;
            }

            bt_sniffing_handle_evt(SNIFFING_EVT_RECOVERY_CONN_CMPL, p_buf);
        }
        break;

    case GAP_BR_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO:
        {
            T_GAP_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_DISCONN_CMPL_INFO *)p_buf;

            BTM_PRINT_TRACE1("bt_sniffing_cback: recovery link disconn reason 0x%04x",
                             p_info->reason);

            recovery_handle = INVALID_HCI_HANDLE;

            bt_sniffing_handle_evt(SNIFFING_EVT_RECOVERY_DISCONN_CMPL, p_buf);
        }
        break;

    case GAP_BR_AUDIO_RECOVERY_LINK_CONNECTION_CHANGE_INFO:
        {
            T_GAP_AUDIO_RECOVERY_LINK_CONNECTION_CHANGE_INFO *p_info =
                (T_GAP_AUDIO_RECOVERY_LINK_CONNECTION_CHANGE_INFO *)p_buf;

            bt_sniffing_handle_evt(SNIFFING_EVT_RECOVERY_CHANGED, p_buf);

            if (p_info->cause == 0 && p_info->audio_type == RECOVERY_LINK_TYPE_A2DP &&
                bt_sniffing_state != SNIFFING_STATE_COORDINATED_ROLESWAP)
            {
                a2dp_recovery_interval = p_info->a2dp_interval;
                a2dp_recovery_flush_tout = p_info->a2dp_flush_timeout;
                a2dp_recovery_rsvd_slot = p_info->a2dp_ctrl_resvd_slot;
                a2dp_recovery_idle_slot = p_info->a2dp_idle_slot;
                a2dp_recovery_idle_skip = p_info->a2dp_idle_skip;
            }
        }
        break;

    case GAP_BR_AUDIO_RECOVERY_LINK_CONNECTION_RESET_INFO:
        {
            bt_sniffing_handle_evt(SNIFFING_EVT_RECOVERY_RESET, p_buf);
        }
        break;

    case GAP_BR_AUDIO_RECOVERY_LINK_FLUSH_COMPLETE_INFO:
        {
            bt_sniffing_handle_evt(SNIFFING_EVT_RECOVERY_FLUSH_CMPL, p_buf);
        }
        break;

    case GAP_BR_SHADOW_LINK_LOSS_INFO:
        {
            T_GAP_SHADOW_LINK_LOSS_INFO *p_info;

            p_info = (T_GAP_SHADOW_LINK_LOSS_INFO *)p_buf;

            BTM_PRINT_TRACE1("bt_sniffing_cback: sniffing link disconn reason 0x%04x", p_info->reason);

            sniffing_handle = INVALID_HCI_HANDLE;
            bt_sniffing_handle_evt(SNIFFING_EVT_SHADOW_LINK_LOSS, p_buf);
            bt_roleswap_sniffing_disconn_cmpl(audio_addr, p_info->reason);
        }
        break;

    case GAP_BR_SETUP_AUDIO_RECOVERY_LINK_RSP:
        {
            bt_sniffing_handle_evt(SNIFFING_EVT_RECOVERY_SETUP_RSP, p_buf);
        }
        break;

    case GAP_BR_SET_CONTINUOUS_TRX_CMPL:
        {
            bt_sniffing_handle_evt(SNIFFING_EVT_SET_CONT_TRX_CMPL, p_buf);
        }
        break;

    default:
        {
            uint16_t cause = *(uint16_t *)p_buf;
            if (cause)
            {
                BTM_PRINT_ERROR2("bt_sniffing_cback: msg 0x%02x error, cause 0x%04x", msg, cause);
            }
        }
        break;
    }
}

T_BT_CLK_REF bt_sniffing_get_piconet_clk(T_BT_CLK_REF  clk_ref,
                                         uint32_t     *bb_clock_timer,
                                         uint16_t     *bb_clock_us)
{
    uint16_t handle;
    uint8_t peer_addr[6];
    int ret = 0;

    remote_peer_addr_get(peer_addr);

    if (bt_sniffing_state == SNIFFING_STATE_SETUP_RECOVERY && clk_ref == BT_CLK_CTRL)
    {
        ret = 1;
        goto ctrl_ref_sniffing_state_error;
    }

    if (clk_ref == BT_CLK_NONE)
    {
        if (sniffing_handle != INVALID_HCI_HANDLE &&
            bt_sniffing_state >= SNIFFING_STATE_SNIFFING_CONNECTED &&
            bt_sniffing_state <= SNIFFING_STATE_COORDINATED_ROLESWAP)
        {
            handle = sniffing_handle;
            clk_ref = BT_CLK_SNIFFING;
        }
        else
        {
            T_BT_BR_LINK *p_ctrl;
            p_ctrl = bt_find_br_link(peer_addr);

            if (p_ctrl == NULL)
            {
                ret = 2;
                goto none_ref_none_link;
            }
            clk_ref = BT_CLK_CTRL;
            handle = p_ctrl->acl_handle;
            if (handle == 0)
            {
                ret = 3;
                goto none_ref_ctrl_handle_nonxistent;
            }
        }

        if (rws_read_bt_piconet_clk(handle, bb_clock_timer, bb_clock_us) == false)
        {
            ret = 4;
            goto none_ref_ctrl_clk_get_failed;
        }

        BTM_PRINT_TRACE5("bt_sniffing_get_piconet_clk: handle 0x%x, sniffing handle 0x%x, state 0x%02x,  bt_clock 0x%x, clk_ref %u",
                         handle, sniffing_handle, bt_sniffing_state, *bb_clock_timer, clk_ref);

        return clk_ref;

    }
    else if (clk_ref == BT_CLK_SNIFFING)
    {
        if (sniffing_handle != INVALID_HCI_HANDLE)
        {
            if (rws_read_bt_piconet_clk(sniffing_handle, bb_clock_timer, bb_clock_us) == false)
            {
                ret = 5;
                goto sniffing_ref_sniffing_clk_get_failed;
            }
            BTM_PRINT_TRACE3("bt_sniffing_get_piconet_clk: clk_ref %u, bt_sniffing_state 0x%02x,  bb_clock_timer 0x%x",
                             clk_ref, bt_sniffing_state, *bb_clock_timer);
            return BT_CLK_SNIFFING;
        }
        else
        {
            T_BT_BR_LINK *p_link_active;

            p_link_active = bt_find_br_link(btm_db.br_link[btm_db.active_a2dp_index].bd_addr);
            if (p_link_active == NULL)
            {
                ret = 6;
                goto sniffing_ref_sniffing_link_nonexistent;
            }
            else
            {
                if (rws_read_bt_piconet_clk(p_link_active->acl_handle, bb_clock_timer, bb_clock_us) == false)
                {
                    ret = 7;
                    goto sniffing_ref_active_link_clk_get_failed;
                }
                BTM_PRINT_TRACE3("bt_sniffing_get_piconet_clk: clk_ref %u, bt_sniffing_state 0x%02x,  bb_clock_timer 0x%x",
                                 clk_ref, bt_sniffing_state, *bb_clock_timer);
                return BT_CLK_SNIFFING;
            }
        }
    }
    else
    {
        T_BT_BR_LINK *p_ctrl;
        p_ctrl = bt_find_br_link(peer_addr);

        if (p_ctrl == NULL)
        {
            ret = 8;
            goto ctrl_ref_link_nonexistent;
        }

        if (bt_sniffing_state == SNIFFING_STATE_UNCOORDINATED_ROLESWAP ||
            bt_sniffing_state == SNIFFING_STATE_COORDINATED_ROLESWAP)
        {
            ret = 9;
            goto ctrl_ref_get_while_roleswapping;
        }

        if (rws_read_bt_piconet_clk(p_ctrl->acl_handle, bb_clock_timer, bb_clock_us) == false)
        {
            ret = 10;
            goto ctrl_ref_get_failed;
        }
        BTM_PRINT_TRACE3("bt_sniffing_get_specific_piconet_clk: clk_ref %u, bt_sniffing_state 0x%02x,  bb_clock_timer 0x%x",
                         clk_ref, bt_sniffing_state, *bb_clock_timer);
        return BT_CLK_CTRL;
    }

ctrl_ref_get_failed:
ctrl_ref_get_while_roleswapping:
ctrl_ref_link_nonexistent:
sniffing_ref_active_link_clk_get_failed:
sniffing_ref_sniffing_link_nonexistent:
sniffing_ref_sniffing_clk_get_failed:
none_ref_ctrl_clk_get_failed:
none_ref_ctrl_handle_nonxistent:
none_ref_none_link:
ctrl_ref_sniffing_state_error:
    BTM_PRINT_WARN1("bt_sniffing_get_piconet_clk: error %d", -ret);
    return BT_CLK_NONE;
}

bool bt_sniffing_get_piconet_id(T_BT_CLK_REF  clk_ref,
                                uint8_t      *clk_index,
                                uint8_t      *role)
{
    uint16_t handle;
    T_BT_BR_LINK *p_ctrl;
    uint8_t peer_addr[6];
    int ret = 0;
    bool get_ret = false;
    remote_peer_addr_get(peer_addr);

    if (clk_ref == BT_CLK_NONE)
    {
        if (sniffing_handle != INVALID_HCI_HANDLE &&
            bt_sniffing_state >= SNIFFING_STATE_SNIFFING_CONNECTED &&
            bt_sniffing_state <= SNIFFING_STATE_COORDINATED_ROLESWAP)
        {
            handle = sniffing_handle;
            get_ret = bt_clk_index_read(handle, clk_index, role);
            BTM_PRINT_TRACE5("bt_sniffing_get_piconet_id: sniff handle 0x%x sniff_handle 0x%x clock_id 0x%x, ref_type 0x%x ,get_ret %d",
                             handle, sniffing_handle, *clk_index, clk_ref, get_ret);
            return get_ret;
        }
        else
        {
            p_ctrl = bt_find_br_link(peer_addr);
            if (p_ctrl == NULL)
            {
                ret = 1;
                goto none_ref_none_link;
            }
            handle = p_ctrl->acl_handle;

            if (handle == 0)
            {
                ret = 2;
                goto none_ref_ctrl_handle_nonxistent;
            }

            get_ret = bt_clk_index_read(handle, clk_index, role);
            BTM_PRINT_TRACE6("bt_sniffing_get_piconet_id: ctrl handle 0x%x sniff_handle 0x%x clock_id 0x%x, ref_type 0x%x role %u, get_ret %d",
                             handle, sniffing_handle, *clk_index, clk_ref, *role, get_ret);
            return get_ret;
        }
    }
    else if (clk_ref == BT_CLK_SNIFFING)
    {
        if (sniffing_handle != INVALID_HCI_HANDLE)
        {
            handle = sniffing_handle;
            get_ret = bt_clk_index_read(handle, clk_index, role);
            BTM_PRINT_TRACE5("bt_sniffing_get_piconet_id: handle 0x%x sniff_handle 0x%x clock_id 0x%x, ref_type 0x%x ,get_ret %d",
                             handle, sniffing_handle, *clk_index, clk_ref, get_ret);
            return get_ret;
        }
        else
        {
            T_BT_BR_LINK *p_link_active;

            p_link_active = bt_find_br_link(btm_db.br_link[btm_db.active_a2dp_index].bd_addr);
            if (p_link_active == NULL)
            {
                ret = 3;
                goto sniffing_ref_sniffing_link_nonexistent;
            }
            else
            {
                handle = p_link_active->acl_handle;
                if (handle == 0)
                {
                    ret = 4;
                    goto sniffing_ref_sniffing_handle_nonexistent;
                }
                get_ret = bt_clk_index_read(handle, clk_index, role);
                BTM_PRINT_TRACE5("bt_sniffing_get_piconet_id: handle 0x%x sniff_handle 0x%x clock_id 0x%x, ref_type 0x%x, get_ret %d",
                                 handle, sniffing_handle, *clk_index, clk_ref, get_ret);
                return get_ret;
            }

        }
    }
    else // clk_ref == BT_CLK_CTRL
    {
        p_ctrl = bt_find_br_link(peer_addr);
        if (p_ctrl == NULL)
        {
            return false;
        }

        if (bt_sniffing_state == SNIFFING_STATE_UNCOORDINATED_ROLESWAP ||
            bt_sniffing_state == SNIFFING_STATE_COORDINATED_ROLESWAP)
        {
            ret = 5;
            goto ctrl_ref_get_while_roleswapping;
        }

        handle = p_ctrl->acl_handle;
        if (handle == 0)
        {
            ret = 6;
            goto ctrl_ref_handle_nonexistent;
        }
        get_ret = bt_clk_index_read(handle, clk_index, role);
        BTM_PRINT_TRACE6("bt_sniffing_get_piconet_id: handle 0x%x sniff_handle 0x%x clock_id 0x%x, ref_type 0x%x role %u, get_ret %d",
                         handle, sniffing_handle, *clk_index, clk_ref, *role, get_ret);
        return get_ret;
    }

ctrl_ref_handle_nonexistent:
ctrl_ref_get_while_roleswapping:
sniffing_ref_sniffing_handle_nonexistent:
sniffing_ref_sniffing_link_nonexistent:
none_ref_ctrl_handle_nonxistent:
none_ref_none_link:
    BTM_PRINT_WARN1("bt_sniffing_get_piconet_id: error %d", -ret);
    return false;
}

bool bt_roleswap_conn_sniffing_link(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_audio;
    T_BT_BR_LINK *p_ctrl;
    uint8_t peer_addr[6];

    remote_peer_addr_get(peer_addr);

    BTM_PRINT_TRACE3("bt_roleswap_conn_sniffing_link: audio addr %s, peer_addr %s, sniffing state 0x%02x",
                     TRACE_BDADDR(bd_addr), TRACE_BDADDR(peer_addr), bt_sniffing_state);

    p_audio = bt_find_br_link(bd_addr);
    p_ctrl = bt_find_br_link(peer_addr);

    if (p_audio == NULL || p_audio->acl_link_state != BT_LINK_STATE_CONNECTED)
    {
        return false;
    }

    if (p_ctrl == NULL || p_ctrl->acl_link_state != BT_LINK_STATE_CONNECTED ||
        p_ctrl->acl_link_role_master == 0 || p_audio->acl_link_role_master ||
        remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
        bt_sniffing_state != SNIFFING_STATE_IDLE)
    {
        return false;
    }

    memcpy(audio_addr, bd_addr, 6);
    sniffing_handle = p_audio->acl_handle;

    bt_sniffing_handle_evt(SNIFFING_EVT_CONN_SNIFFING, bd_addr);

    return true;
}

bool bt_roleswap_disconn_sniffing_link(uint8_t bd_addr[6])
{
    BTM_PRINT_TRACE3("bt_roleswap_disconn_sniffing_link: addr %s, sniffing state 0x%02x, audio addr %s",
                     TRACE_BDADDR(bd_addr), bt_sniffing_state, TRACE_BDADDR(audio_addr));

    if (memcmp(audio_addr, bd_addr, 6))
    {
        return false;
    }

    if ((bt_sniffing_state == SNIFFING_STATE_SNIFFING_CONNECTED) ||
        (bt_sniffing_state == SNIFFING_STATE_SETUP_RECOVERY))
    {
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
        {
            gap_br_disconn_sniffing_link(sniffing_handle, HCI_ERR_REMOTE_USER_TERMINATE);
        }
        else
        {
            bt_roleswap_info_send(ROLESWAP_MODULE_CTRL, ROLESWAP_CTRL_DISCONN_SNIFFING_REQ,
                                  audio_addr, 6);
        }
    }
    else if (bt_sniffing_state == SNIFFING_STATE_RECOVERY_CONNECTED)
    {
        if (remote_session_role_get() == REMOTE_SESSION_ROLE_SECONDARY)
        {
            bt_roleswap_info_send(ROLESWAP_MODULE_CTRL, ROLESWAP_CTRL_DISCONN_SNIFFING_REQ,
                                  audio_addr, 6);
        }
        else
        {
            bt_sniffing_disconn_recovery_link(audio_addr, HCI_ERR_REMOTE_USER_TERMINATE);
            bt_roleswap_info_send(ROLESWAP_MODULE_CTRL, ROLESWAP_CTRL_DISCONN_SNIFFING_REQ,
                                  audio_addr, 6);
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool bt_roleswap_conn_audio_recovery(uint8_t  bd_addr[6],
                                     uint16_t interval,
                                     uint16_t flush_tout,
                                     uint8_t  rsvd_slot,
                                     uint8_t  idle_slot,
                                     uint8_t  idle_skip)
{
    BTM_PRINT_TRACE8("bt_roleswap_conn_audio_recovery: addr %s, sniffing state 0x%02x, audio addr %s"
                     "inteval 0x%04x, flush_tou 0x%04x, rsvd_slot 0x%02x, idle_slot 0x%02x, idle_skip 0x%02x",
                     TRACE_BDADDR(bd_addr), bt_sniffing_state, TRACE_BDADDR(audio_addr),
                     interval, flush_tout, rsvd_slot, idle_slot, idle_skip);

    if (memcmp(audio_addr, bd_addr, 6))
    {
        return false;
    }

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
        (bt_sniffing_state != SNIFFING_STATE_SNIFFING_CONNECTED &&
         bt_sniffing_state != SNIFFING_STATE_RECOVERY_CONNECTED) ||
        bt_sniffing_substate != SNIFFING_SUBSTATE_IDLE)

    {
        return false;
    }

    recovery_link_type = RECOVERY_LINK_TYPE_A2DP;
    a2dp_recovery_interval = interval;
    a2dp_recovery_flush_tout = flush_tout;
    a2dp_recovery_rsvd_slot = rsvd_slot;
    a2dp_recovery_idle_slot = idle_slot;
    a2dp_recovery_idle_skip = idle_skip;

    bt_sniffing_handle_evt(SNIFFING_EVT_CONN_RECOVERY, bd_addr);

    return true;
}

bool bt_roleswap_disconn_audio_recovery(uint8_t bd_addr[6],
                                        uint8_t reason)
{
    BTM_PRINT_TRACE3("bt_roleswap_disconn_audio_recovery: addr %s, sniffing state 0x%02x, audio addr %s",
                     TRACE_BDADDR(bd_addr), bt_sniffing_state, TRACE_BDADDR(audio_addr));

    if (memcmp(audio_addr, bd_addr, 6))
    {
        return false;
    }

    if (bt_sniffing_state != SNIFFING_STATE_RECOVERY_CONNECTED ||
        recovery_handle == INVALID_HCI_HANDLE || recovery_link_type != RECOVERY_LINK_TYPE_A2DP)
    {
        return false;
    }

    return bt_sniffing_disconn_recovery_link(audio_addr, reason);
}

bool bt_roleswap_cfg_audio_recovery(uint8_t  bd_addr[6],
                                    uint16_t interval,
                                    uint16_t flush_tout,
                                    uint8_t  rsvd_slot,
                                    uint8_t  idle_slot,
                                    uint8_t  idle_skip)
{
    T_RECOVERY_LINK_PARAM param;

    BTM_PRINT_TRACE8("bt_roleswap_cfg_audio_recovery: addr %s, sniffing state 0x%02x, audio addr %s"
                     "inteval 0x%04x, flush_tou 0x%04x, rsvd_slot 0x%02x, idle_slot 0x%02x, idle_skip 0x%02x",
                     TRACE_BDADDR(bd_addr), bt_sniffing_state, TRACE_BDADDR(audio_addr),
                     interval, flush_tout, rsvd_slot, idle_slot, idle_skip);

    if (memcmp(audio_addr, bd_addr, 6))
    {
        return false;
    }

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
        bt_sniffing_state != SNIFFING_STATE_RECOVERY_CONNECTED ||
        recovery_link_type != RECOVERY_LINK_TYPE_A2DP ||
        bt_sniffing_substate != SNIFFING_SUBSTATE_IDLE)
    {
        return false;
    }

    param.interval = interval;
    param.flush_tout = flush_tout;
    param.rsvd_slot = rsvd_slot;
    param.idle_slot = idle_slot;
    param.idle_skip = idle_skip;

    a2dp_recovery_idle_slot = idle_slot;
    a2dp_recovery_idle_skip = idle_skip;

    bt_sniffing_handle_evt(SNIFFING_EVT_ADJUST_QOS, &param);

    return true;
}

bool bt_roleswap_conn_voice_recovery(uint8_t bd_addr[6])
{
    BTM_PRINT_TRACE3("bt_roleswap_conn_voice_recovery: addr %s, sniffing state 0x%02x, audio addr %s",
                     TRACE_BDADDR(bd_addr), bt_sniffing_state, TRACE_BDADDR(audio_addr));

    if (memcmp(audio_addr, bd_addr, 6))
    {
        return false;
    }

    if (remote_session_role_get() != REMOTE_SESSION_ROLE_PRIMARY ||
        (bt_sniffing_state != SNIFFING_STATE_SNIFFING_CONNECTED &&
         bt_sniffing_state != SNIFFING_STATE_RECOVERY_CONNECTED) ||
        bt_sniffing_substate != SNIFFING_SUBSTATE_IDLE)
    {
        return false;
    }

    recovery_link_type = RECOVERY_LINK_TYPE_SCO;
    bt_sniffing_handle_evt(SNIFFING_EVT_CONN_RECOVERY, bd_addr);

    return true;
}

bool bt_roleswap_disconn_voice_recovery(uint8_t bd_addr[6],
                                        uint8_t reason)
{
    BTM_PRINT_TRACE3("bt_roleswap_disconn_voice_recovery: addr %s, sniffing state 0x%02x, audio addr %s",
                     TRACE_BDADDR(bd_addr), bt_sniffing_state, TRACE_BDADDR(audio_addr));

    if (memcmp(audio_addr, bd_addr, 6))
    {
        return false;
    }

    if (bt_sniffing_state != SNIFFING_STATE_RECOVERY_CONNECTED ||
        recovery_handle == INVALID_HCI_HANDLE ||
        recovery_link_type != RECOVERY_LINK_TYPE_SCO)
    {
        return false;
    }

    return bt_sniffing_disconn_recovery_link(audio_addr, reason);
}

const T_BT_ROLESWAP_PROTO bt_sniffing_proto =
{
    .acl_status = bt_sniffing_handle_acl_status,
    .profile_conn = bt_sniffing_handle_profile_conn,
    .profile_disconn = bt_sniffing_handle_profile_disconn,
    .sco_disconn = bt_sniffing_handle_sco_disconn,
    .bt_rfc_conn = bt_sniffing_handle_bt_rfc_conn,
    .bt_rfc_disconn = bt_sniffing_handle_bt_rfc_disconn,
    .bt_avp_control_conn = bt_sniffing_handle_bt_avp_control_conn,
    .bt_avp_control_disconn = bt_sniffing_handle_bt_avp_control_disconn,
    .bt_avp_audio_conn = bt_sniffing_handle_bt_avp_audio_conn,
    .bt_avp_audio_disconn = bt_sniffing_handle_bt_avp_audio_disconn,
    .bt_att_conn = bt_sniffing_handle_bt_att_conn,
    .bt_att_disconn = bt_sniffing_handle_bt_att_disconn,
    .ctrl_conn = bt_sniffing_handle_ctrl_conn,
    .recv = bt_sniffing_recv,
    .cback = bt_sniffing_cback,
    .start = bt_sniffing_start_roleswap,
    .cfm = bt_sniffing_roleswap_cfm,
    .stop = bt_sniffing_terminate_roleswap,
    .get_piconet_clk = bt_sniffing_get_piconet_clk,
    .get_piconet_id = bt_sniffing_get_piconet_id,
};
#else
bool bt_roleswap_conn_sniffing_link(uint8_t bd_addr[6])
{
    return false;
}

bool bt_roleswap_disconn_sniffing_link(uint8_t bd_addr[6])
{
    return false;
}

bool bt_roleswap_conn_audio_recovery(uint8_t  bd_addr[6],
                                     uint16_t interval,
                                     uint16_t flush_tout,
                                     uint8_t  rsvd_slot,
                                     uint8_t  idle_slot,
                                     uint8_t  idle_skip)
{
    return false;
}

bool bt_roleswap_disconn_audio_recovery(uint8_t bd_addr[6],
                                        uint8_t reason)
{
    return false;
}

bool bt_roleswap_cfg_audio_recovery(uint8_t  bd_addr[6],
                                    uint16_t interval,
                                    uint16_t flush_tout,
                                    uint8_t  rsvd_slot,
                                    uint8_t  idle_slot,
                                    uint8_t  idle_skip)
{
    return false;
}

bool bt_roleswap_conn_voice_recovery(uint8_t bd_addr[6])
{
    return false;
}

bool bt_roleswap_disconn_voice_recovery(uint8_t bd_addr[6],
                                        uint8_t reason)
{
    return false;
}

bool bt_sniffing_set_a2dp_dup_num(bool    enable,
                                  uint8_t nack_num,
                                  bool    quick_flush)
{
    return false;
}

void bt_roleswap_sniffing_conn_cmpl(uint8_t  bd_addr[6],
                                    uint16_t cause)
{

}
#endif
