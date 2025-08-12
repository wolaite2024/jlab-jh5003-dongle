/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HFP_AG_SUPPORT == 1)

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "trace.h"
#include "gap_br.h"
#include "sys_timer.h"
#include "hfp_ag.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_hfp_ag.h"
#include "bt_hfp_int.h"

#define SCO_PKT_TYPES_T1_T2  (GAP_PKT_TYPE_EV3 | GAP_PKT_TYPE_NO_3EV3 | \
                              GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_T2     (GAP_PKT_TYPE_NO_3EV3 | GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_T1     (GAP_PKT_TYPE_EV3 | GAP_PKT_TYPE_NO_2EV3 | GAP_PKT_TYPE_NO_3EV3 | \
                              GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_S4     (GAP_PKT_TYPE_NO_3EV3 | GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_S3     SCO_PKT_TYPES_S4
#define SCO_PKT_TYPES_S1     (GAP_PKT_TYPE_EV3 | GAP_PKT_TYPE_NO_2EV3 | GAP_PKT_TYPE_NO_3EV3 | \
                              GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_D1     (GAP_PKT_TYPE_HV3 | GAP_PKT_TYPE_NO_2EV3 | GAP_PKT_TYPE_NO_3EV3 | \
                              GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)

#define HFP_AG_DEFAULT_RING_REPEAT_INTERVAL (5000)  //ms

void bt_hfp_ag_ringing_start(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            hfp_ag_send_ring(p_link->bd_addr);
            hfp_ag_send_clip(p_link->bd_addr, p_link->hfp_data.ag_call_info.call_num,
                             p_link->hfp_data.ag_call_info.call_num_type);
            sys_timer_restart(p_link->hfp_data.timer_ring, p_link->hfp_data.ring_interval * 1000);
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            if ((p_link->hfp_data.supported_features & HFP_INBAND_RINGTONE_ENABLE) == 0)
            {
                hfp_ag_send_ring(p_link->bd_addr);
            }
            sys_timer_restart(p_link->hfp_data.timer_ring, p_link->hfp_data.ring_interval * 1000);
        }
    }
}

void bt_hfp_ag_codec_negotiate(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);

        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.codec_type_sent != CODEC_TYPE_NONE)
            {
                return;
            }

            if (p_link->hfp_data.sco_setting == HFP_AG_SCO_SETTING_ANY)
            {
                if ((p_link->hfp_data.supported_features & HFP_CODEC_NEGOTIATION_ENABLE) &&
                    (p_link->hfp_data.codec_updated))
                {
                    p_link->hfp_data.codec_updated = false;
                    hfp_ag_codec_negotiate(p_link->bd_addr, false, &p_link->hfp_data.codec_type_sent);
                }
                else
                {
                    if (p_link->hfp_data.codec_type == CODEC_TYPE_CVSD)
                    {
                        if (p_link->hfp_data.supported_features & HFP_ESCO_S4_ENABLE)
                        {
                            p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_S4;
                            gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 12, 0x0360, 2, SCO_PKT_TYPES_S4);
                        }
                        else
                        {
                            p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_S3;
                            gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 10, 0x0360, 1, SCO_PKT_TYPES_S3);
                        }
                    }
                    else if (p_link->hfp_data.codec_type == CODEC_TYPE_MSBC)
                    {
                        p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_T2;
                        gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 13, 0x0363, 2, SCO_PKT_TYPES_T2);
                    }
                    else if (p_link->hfp_data.codec_type == CODEC_TYPE_LC3)
                    {
                        p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_T2;
                        gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 13, 0x0363, 2, SCO_PKT_TYPES_T2);
                    }
                }
            }
            else
            {
                switch (p_link->hfp_data.sco_setting)
                {
                case HFP_AG_SCO_SETTING_T2:
                    p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_T1;
                    gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 8, 0x0363, 2, SCO_PKT_TYPES_T1);
                    break;

                case HFP_AG_SCO_SETTING_T1:
                    p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_ANY;
                    hfp_ag_codec_negotiate(p_link->bd_addr, true, &p_link->hfp_data.codec_type_sent);
                    break;

                case HFP_AG_SCO_SETTING_S4:
                case HFP_AG_SCO_SETTING_S3:
                    p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_S1;
                    gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 7, 0x0360, 1, SCO_PKT_TYPES_S1);
                    break;

                case HFP_AG_SCO_SETTING_S1:
                    p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_D1;
                    gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 5, 0x0360, 0, SCO_PKT_TYPES_D1);
                    break;

                default:
                    break;
                }
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_D1;
            gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 12, 0x0360, 0, SCO_PKT_TYPES_D1);
        }
    }
}

void hfp_ag_timeout_cb(T_SYS_TIMER_HANDLE handle)
{
    T_BT_BR_LINK *p_link;

    p_link = (void *)sys_timer_id_get(handle);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            bt_hfp_ag_ringing_start(p_link->bd_addr);
            if (p_link->hfp_data.supported_features & HFP_INBAND_RINGTONE_ENABLE)
            {
                T_BT_MSG_PAYLOAD payload;

                memcpy(payload.bd_addr, p_link->bd_addr, 6);
                payload.msg_buf = NULL;
                bt_mgr_dispatch(BT_MSG_HFP_AG_INBAND_RINGING_REQ, &payload);
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            bt_hfp_ag_ringing_start(p_link->bd_addr);
            if (p_link->hfp_data.supported_features & HFP_INBAND_RINGTONE_ENABLE)
            {
                T_BT_MSG_PAYLOAD payload;

                memcpy(payload.bd_addr, p_link->bd_addr, 6);
                payload.msg_buf = NULL;
                bt_mgr_dispatch(BT_MSG_HSP_AG_INBAND_RINGING_REQ, &payload);
            }
        }
    }
}

void bt_hfp_ag_cback(uint8_t       bd_addr[6],
                     T_HFP_AG_MSG  msg_type,
                     void         *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = NULL;

    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE3("bt_hfp_ag_cback: bd_addr %s, link %p, msg_type 0x%02x",
                     TRACE_BDADDR(bd_addr), p_link, msg_type);
    switch (msg_type)
    {
    case HFP_AG_MSG_RFC_CONN_IND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_CONN_IND, &payload);
        }
        else
        {
            hfp_ag_connect_cfm(bd_addr, false);
        }
        break;

    case HFP_AG_MSG_RFC_CONN:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_RFC_CONN *p_msg;

            p_msg = (T_HFP_AG_MSG_RFC_CONN *)msg_buf;
            p_link->hfp_data.hsp_active_fg = p_msg->hsp;
            p_link->hfp_data.hfp_state = HF_STATE_RFCOMM_CONNECTED;
        }
        else
        {
            hfp_ag_disconnect_req(bd_addr);
        }
        break;

    case HFP_AG_MSG_CONN:
        if (p_link != NULL)
        {
            p_link->hfp_data.ring_interval = HFP_AG_DEFAULT_RING_REPEAT_INTERVAL;
            p_link->hfp_data.hfp_role = HFP_ROLE_AG;

            p_link->hfp_data.timer_ring = sys_timer_create("hfp_ag_ring",
                                                           SYS_TIMER_TYPE_LOW_PRECISION,
                                                           (uint32_t)p_link,
                                                           p_link->hfp_data.ring_interval * 1000,
                                                           false,
                                                           hfp_ag_timeout_cb);

            p_link->hfp_data.codec_type = CODEC_TYPE_CVSD;
            p_link->hfp_data.sco_setting = HFP_AG_SCO_SETTING_ANY;

            if (p_link->hfp_data.hsp_active_fg)
            {
                p_link->hfp_data.hfp_state = HS_STATE_CONNECTED;
                p_link->connected_profile |= HSP_PROFILE_MASK;
                bt_mgr_dispatch(BT_MSG_HSP_AG_CONN_CMPL, &payload);
            }
            else
            {
                uint16_t local_capabilities;

                p_link->hfp_data.hfp_state = HF_STATE_CONNECTED;
                p_link->connected_profile |= HFP_PROFILE_MASK;

                hfp_ag_local_capabilities_get(&local_capabilities);
                hfp_ag_remote_capabilities_get(bd_addr, &p_link->hfp_data.capabilities);
                if (local_capabilities & AG_CAPABILITY_INBAND_RINGING)
                {
                    p_link->hfp_data.supported_features |= HFP_INBAND_RINGTONE_ENABLE;
                }

                if ((p_link->hfp_data.capabilities & HF_CAPABILITY_ESCO_S4_T2_SUPPORTED) &&
                    (local_capabilities & AG_CAPABILITY_ESCO_S4_T2_SUPPORTED))
                {
                    p_link->hfp_data.supported_features |= HFP_ESCO_S4_ENABLE;
                }

                if ((p_link->hfp_data.capabilities & HF_CAPABILITY_CODEC_NEGOTIATION) &&
                    (local_capabilities & AG_CAPABILITY_CODEC_NEGOTIATION))
                {
                    p_link->hfp_data.supported_features |= HFP_CODEC_NEGOTIATION_ENABLE;
                }

                bt_mgr_dispatch(BT_MSG_HFP_AG_CONN_CMPL, &payload);
                bt_mgr_dispatch(BT_MSG_HFP_AG_SUPPORTED_FEATURES, &payload);
            }

            bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
        }
        break;

    case HFP_AG_MSG_DISCONN:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;

            if (p_link->hfp_data.hsp_active_fg)
            {
                p_link->connected_profile &= ~HSP_PROFILE_MASK;
            }
            else
            {
                p_link->connected_profile &= ~HFP_PROFILE_MASK;
            }

            if (p_link->sco_handle)
            {
                gap_br_send_sco_disconn_req(p_link->bd_addr);
            }

            if (p_link->hfp_data.hsp_active_fg)
            {
                bt_mgr_dispatch(BT_MSG_HSP_AG_DISCONN_CMPL, &payload);
            }
            else
            {
                bt_mgr_dispatch(BT_MSG_HFP_AG_DISCONN_CMPL, &payload);
            }

            if (p_link->hfp_data.timer_ring != NULL)
            {
                sys_timer_delete(p_link->hfp_data.timer_ring);
            }

            memset(&p_link->hfp_data, 0, sizeof(T_HFP_LINK_DATA));
        }
        break;

    case HFP_AG_MSG_VTS:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_CALL_DTMF_CODE *p_msg = (T_HFP_AG_MSG_CALL_DTMF_CODE *)msg_buf;
            T_BT_HFP_AG_CALL_DTMF_CODE dtmf_code;

            dtmf_code.dtmf_code = p_msg->dtmf_code;
            payload.msg_buf = &dtmf_code;
            bt_mgr_dispatch(BT_MSG_HFP_AG_MSG_DTMF_CODE, &payload);
        }
        break;

    case HFP_AG_MSG_DIAL_WITH_NUMBER:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_DIAL_WITH_NUMBER *p_msg = (T_HFP_AG_MSG_DIAL_WITH_NUMBER *)msg_buf;
            T_BT_HFP_AG_DIAL_WITH_NUMBER dial_with_number;

            memcpy(dial_with_number.number, p_msg->number, 20);
            payload.msg_buf = &dial_with_number;
            bt_mgr_dispatch(BT_MSG_HFP_AG_MSG_DIAL_WITH_NUMBER, &payload);
        }
        break;

    case HFP_AG_MSG_DIAL_WITH_MEMORY:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_DIAL_WITH_MEMORY *p_msg = (T_HFP_AG_MSG_DIAL_WITH_MEMORY *)msg_buf;
            T_BT_HFP_AG_DIAL_WITH_MEMORY memory;

            memory.num = p_msg->num;
            payload.msg_buf = &memory;
            bt_mgr_dispatch(BT_MSG_HFP_AG_MSG_DIAL_WITH_MEMORY, &payload);
        }
        break;

    case HFP_AG_MSG_DIAL_LAST_NUMBER:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_MSG_DIAL_LAST_NUMBER, &payload);
        }
        break;

    case HFP_AG_MSG_ACCEPT_CALL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_ANSWER_REQ, &payload);
        }
        break;

    case HFP_AG_MSG_REJECT_HANGUP_CALL:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_TERMINATE_REQ, &payload);
        }
        break;

    case HFP_AG_MSG_INFORM_MICROPHONE_GAIN:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_MICROPHONE_VOLUME_CHANGED *p_msg;

            p_msg = (T_HFP_AG_MSG_MICROPHONE_VOLUME_CHANGED *)msg_buf;
            payload.msg_buf = &p_msg->volume;
            bt_mgr_dispatch(BT_MSG_HFP_AG_MIC_VOLUME_CHANGED, &payload);
        }
        break;

    case HFP_AG_MSG_INFORM_SPEAKER_GAIN:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_SPEAKER_VOLUME_CHANGED *p_msg;

            p_msg = (T_HFP_AG_MSG_SPEAKER_VOLUME_CHANGED *)msg_buf;
            payload.msg_buf = &p_msg->volume;
            bt_mgr_dispatch(BT_MSG_HFP_AG_SPK_VOLUME_CHANGED, &payload);
        }
        break;

    case HFP_AG_MSG_HSP_BUTTON_PRESS:
        if (p_link != NULL)
        {
            if (p_link->sco_handle == 0)
            {
                bt_mgr_dispatch(BT_MSG_HSP_AG_CALL_ANSWER_REQ, &payload);
            }
            else
            {
                bt_mgr_dispatch(BT_MSG_HSP_AG_CALL_TERMINATE_REQ, &payload);
            }
        }
        break;

    case HFP_AG_MSG_CLCC:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_CURR_CALLS_LIST_QUERY, &payload);
        }
        break;

    case HFP_AG_MSG_CODEC_NEGOTIATE:
        if (p_link != NULL)
        {
            p_link->hfp_data.sco_int_flag = true;
            bt_hfp_ag_codec_negotiate(p_link->bd_addr);
        }
        break;

    case HFP_AG_MSG_BIEV:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_HF_IND *p_msg;

            p_msg = (T_HFP_AG_MSG_HF_IND *)msg_buf;
            payload.msg_buf = &p_msg->ind_value;

            switch (p_msg->ind_id)
            {
            case HFP_HF_IND_ENHANCED_SAFETY_STATUS_ID:
                bt_mgr_dispatch(BT_MSG_HFP_AG_ENHANCED_SAFETY_STATUS, &payload);
                break;

            case HFP_HF_IND_BATTERY_LEVEL_STATUS_ID:
                bt_mgr_dispatch(BT_MSG_HFP_AG_BATTERY_LEVEL, &payload);
                break;

            default:
                break;
            }
        }
        break;

    case HFP_AG_MSG_IPHONEACCEV:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_HF_XAPL *p_msg;

            p_msg = (T_HFP_AG_MSG_HF_XAPL *)msg_buf;
            switch (p_msg->xapl_id)
            {
            case HFP_AG_HF_XAPL_BATTERY_STATUS_ID:
                p_msg->xapl_value = (p_msg->xapl_value + 1) * 10;
                payload.msg_buf = &p_msg->xapl_value;
                bt_mgr_dispatch(BT_MSG_HFP_AG_BATTERY_LEVEL, &payload);
                break;

            case HFP_AG_HF_XAPL_DOCK_STATUS_ID:
                break;

            case HFP_AG_HF_XAPL_SIRI_STATUS_ID:
                break;

            case HFP_AG_HF_XAPL_NR_STATUS_ID:
                break;

            default:
                break;
            }
        }
        break;

    case HFP_AG_MSG_XEVENT:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_HF_XEVENT *p_msg;

            p_msg = (T_HFP_AG_MSG_HF_XEVENT *)msg_buf;
            payload.msg_buf = &p_msg->battery_level;
            bt_mgr_dispatch(BT_MSG_HFP_AG_BATTERY_LEVEL, &payload);
        }
        break;

    case HFP_AG_MSG_CIND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_INDICATORS_STATUS_REQ, &payload);
        }
        break;

    case HFP_AG_MSG_CODEC_TYPE_SELECT:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_CODEC_TYPE_SELECTED *p_msg;

            p_msg = (T_HFP_AG_MSG_CODEC_TYPE_SELECTED *)msg_buf;
            if (p_link->hfp_data.codec_type_sent == p_msg->codec_type)
            {
                p_link->hfp_data.codec_type = p_msg->codec_type;
                p_link->hfp_data.codec_type_sent = CODEC_TYPE_NONE;
                bt_hfp_ag_codec_negotiate(p_link->bd_addr);
            }
            else
            {
                p_link->hfp_data.codec_type_sent = CODEC_TYPE_NONE;
                hfp_ag_send_error(bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
            }
        }
        break;

    case HFP_AG_MSG_BAC_SET:
        if (p_link != NULL)
        {
            p_link->hfp_data.codec_updated = (*(uint8_t *)msg_buf == 1) ? true : false;
            if (p_link->hfp_data.codec_type_sent != CODEC_TYPE_NONE)
            {
                p_link->hfp_data.codec_type_sent = CODEC_TYPE_NONE;
                bt_hfp_ag_codec_negotiate(p_link->bd_addr);
            }
        }
        break;

    case HFP_AG_MSG_CHLD:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_CHLD *p_msg;

            p_msg = (T_HFP_AG_MSG_CHLD *)msg_buf;
            switch (p_msg->value)
            {
            case HFP_AG_HF_3WAY_HELD_CALL_RELEASED:
                bt_mgr_dispatch(BT_MSG_HFP_AG_3WAY_HELD_CALL_RELEASED, &payload);
                break;

            case HFP_AG_HF_3WAY_ACTIVE_CALL_RELEASED:
                bt_mgr_dispatch(BT_MSG_HFP_AG_3WAY_ACTIVE_CALL_RELEASED, &payload);
                break;

            case HFP_AG_HF_3WAY_SWITCHED:
                bt_mgr_dispatch(BT_MSG_HFP_AG_3WAY_SWITCHED, &payload);
                break;

            case HFP_AG_HF_3WAY_MERGED:
                bt_mgr_dispatch(BT_MSG_HFP_AG_3WAY_MERGED, &payload);
                break;

            default:
                hfp_ag_send_error(bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
                break;
            }
        }
        break;

    case HFP_AG_MSG_CNUM:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_SUBSCRIBER_NUMBER_QUERY, &payload);
        }
        break;

    case HFP_AG_MSG_COPS_SET:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_NETWORK_NAME_FORMAT_SET, &payload);
        }
        break;

    case HFP_AG_MSG_COPS_QUERY:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_CURR_OPERATOR_QUERY, &payload);
        }
        break;

    case HFP_AG_MSG_NREC:
        if (p_link != NULL)
        {
            T_HFP_AG_MSG_NREC *p_msg;

            p_msg = (T_HFP_AG_MSG_NREC *)msg_buf;
            payload.msg_buf = &p_msg->value;
            bt_mgr_dispatch(BT_MSG_HFP_AG_NREC_STATUS, &payload);
        }
        break;

    case HFP_AG_MSG_ACTIVATE_VOICE_RECOGNITION:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_VOICE_RECOGNITION_ACTIVATION, &payload);
        }
        break;

    case HFP_AG_MSG_DEACTIVATE_VOICE_RECOGNITION:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_AG_VOICE_RECOGNITION_DEACTIVATION, &payload);
        }
        break;

    case HFP_AG_MSG_UNKNOWN_CMD:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_HFP_AG_UNKNOWN_CMD, &payload);
        }
        break;

    default:
        break;
    }
}

bool bt_hfp_ag_connect_req(uint8_t bd_addr[6],
                           uint8_t server_chann,
                           bool    is_hfp)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        if ((p_link->connected_profile & (HFP_PROFILE_MASK | HSP_PROFILE_MASK)) == 0)
        {
            if (hfp_ag_connect_req(bd_addr, server_chann, is_hfp))
            {
                p_link->hfp_data.hfp_state = HF_STATE_CONNECTING;
                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);

        if (p_link->sco_handle)
        {
            gap_br_send_sco_disconn_req(p_link->bd_addr);
        }

        return hfp_ag_disconnect_req(bd_addr);
    }

    return false;
}

bool bt_hfp_ag_connect_cfm(uint8_t bd_addr[6],
                           bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return hfp_ag_connect_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_hfp_ag_audio_connect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if ((p_link->hfp_data.hfp_state == HF_STATE_CONNECTED) ||
            (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED))
        {
            if (p_link->sco_handle == 0)
            {
                p_link->hfp_data.sco_int_flag = true;
                bt_hfp_ag_codec_negotiate(p_link->bd_addr);
                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_audio_connect_cfm(uint8_t bd_addr[6],
                                 bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (accept == true)
        {
            if ((p_link->hfp_data.codec_type == CODEC_TYPE_MSBC) ||
                (p_link->hfp_data.codec_type == CODEC_TYPE_LC3))
            {
                /* eSCO parameter set to "T2" */
                if (gap_br_send_sco_conn_cfm(bd_addr,
                                             8000,
                                             8000,
                                             13,
                                             0x0363,
                                             2,
                                             SCO_PKT_TYPES_T1_T2,
                                             GAP_CFM_CAUSE_ACCEPT) == GAP_CAUSE_SUCCESS)
                {
                    return true;
                }
            }
            else
            {
                if (p_link->is_esco == 0)
                {
                    /* SCO link
                       voice setting |= 0x0300 mean bypass MAC HW audio decoder/encoder*/
                    if (gap_br_send_sco_conn_cfm(bd_addr,
                                                 8000,
                                                 8000,
                                                 5,
                                                 0x0360,
                                                 0,
                                                 SCO_PKT_TYPES_D1,
                                                 GAP_CFM_CAUSE_ACCEPT) == GAP_CAUSE_SUCCESS)
                    {
                        return true;
                    }
                }
                else
                {
                    if (p_link->hfp_data.supported_features & HFP_ESCO_S4_ENABLE)
                    {
                        /* eSCO parameter set to "S4" */
                        if (gap_br_send_sco_conn_cfm(bd_addr,
                                                     8000,
                                                     8000,
                                                     12,
                                                     0x0360,
                                                     2,
                                                     SCO_PKT_TYPES_S4,
                                                     GAP_CFM_CAUSE_ACCEPT) == GAP_CAUSE_SUCCESS)
                        {
                            return true;
                        }
                    }
                    else
                    {
                        /* eSCO parameter set to "S3" */
                        if (gap_br_send_sco_conn_cfm(bd_addr,
                                                     8000,
                                                     8000,
                                                     10,
                                                     0x0360,
                                                     1,
                                                     SCO_PKT_TYPES_S3,
                                                     GAP_CFM_CAUSE_ACCEPT) == GAP_CAUSE_SUCCESS)
                        {
                            return true;
                        }
                    }
                }
            }
        }
        else
        {
            if (gap_br_send_sco_conn_cfm(bd_addr,
                                         8000,
                                         8000,
                                         13,
                                         0x0363,
                                         2,
                                         SCO_PKT_TYPES_S4,
                                         GAP_CFM_CAUSE_REJECT) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_audio_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->sco_handle)
        {
            if (gap_br_send_sco_disconn_req(p_link->bd_addr) == GAP_CAUSE_SUCCESS)
            {
                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_send_vnd_at_cmd_req(uint8_t     bd_addr[6],
                                   const char *at_cmd)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return hfp_ag_try_general_at_cmd(bd_addr, at_cmd);
    }

    return false;
}

bool bt_hfp_ag_ok_send(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return hfp_ag_send_ok(bd_addr);
    }

    return false;
}

bool bt_hfp_ag_error_send(uint8_t bd_addr[6],
                          uint8_t error_code)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return hfp_ag_send_error(bd_addr, error_code);
    }

    return false;
}

void bt_hfp_ag_call_status_update(uint8_t bd_addr[6],
                                  uint8_t indicator_type)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        switch (indicator_type)
        {
        case AG_INDICATOR_TYPE_CALL:
            {
                if (p_link->hfp_data.ag_status_ind.call_status == HFP_CALL_STATUS_IDLE)
                {
                    if (p_link->hfp_data.call_status != BT_HFP_AG_CALL_IDLE)
                    {
                        if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE)
                        {
                            p_link->hfp_data.call_status = BT_HFP_AG_CALL_IDLE;
                        }
                        if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD)
                        {
                            p_link->hfp_data.call_status = BT_HFP_AG_CALL_ACTIVE;
                        }
                        if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
                        {
                            p_link->hfp_data.call_status = BT_HFP_AG_CALL_INCOMING;
                        }
                    }
                }
                else if (p_link->hfp_data.ag_status_ind.call_status == HFP_CALL_STATUS_ACTIVE)
                {
                    if (p_link->hfp_data.call_status != BT_HFP_AG_CALL_ACTIVE)
                    {
                        p_link->hfp_data.call_status = BT_HFP_AG_CALL_ACTIVE;
                    }
                }
            }
            break;

        case AG_INDICATOR_TYPE_CALLSETUP:
            {
                if (p_link->hfp_data.ag_status_ind.call_setup_status == HFP_CALL_SETUP_STATUS_IDLE)
                {
                    if ((p_link->hfp_data.call_status == BT_HFP_AG_CALL_INCOMING) ||
                        (p_link->hfp_data.call_status == BT_HFP_AG_CALL_OUTGOING))
                    {
                        p_link->hfp_data.call_status = BT_HFP_AG_CALL_IDLE;
                    }
                    else if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
                    {
                        p_link->hfp_data.call_status = BT_HFP_AG_CALL_ACTIVE;
                    }
                }
                else if (p_link->hfp_data.ag_status_ind.call_setup_status == HFP_CALL_SETUP_STATUS_INCOMING_CALL)
                {
                    if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_IDLE)
                    {
                        p_link->hfp_data.call_status = BT_HFP_AG_CALL_INCOMING;
                    }
                    else if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE)
                    {
                        p_link->hfp_data.call_status = BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING;
                    }
                }
                else if ((p_link->hfp_data.ag_status_ind.call_setup_status == HFP_CALL_SETUP_STATUS_OUTGOING_CALL)
                         ||
                         (p_link->hfp_data.ag_status_ind.call_setup_status == HFP_CALL_SETUP_STATUS_ALERTING))
                {
                    if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_IDLE)
                    {
                        p_link->hfp_data.call_status = BT_HFP_AG_CALL_OUTGOING;
                    }
                }
            }
            break;

        case AG_INDICATOR_TYPE_CALLHELD:
            {
                if (p_link->hfp_data.ag_status_ind.call_held_status == HFP_CALL_HELD_STATUS_IDLE)
                {
                    if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD)
                    {
                        p_link->hfp_data.call_status = BT_HFP_AG_CALL_ACTIVE;
                    }
                    else if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_HELD)
                    {
                        p_link->hfp_data.call_status = BT_HFP_AG_CALL_IDLE;
                    }
                }
                else if (p_link->hfp_data.ag_status_ind.call_held_status ==
                         HFP_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL)
                {
                    if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
                    {
                        p_link->hfp_data.call_status = BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD;
                    }
                }
                else if (p_link->hfp_data.ag_status_ind.call_held_status ==
                         HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL)
                {
                    p_link->hfp_data.call_status = BT_HFP_AG_CALL_HELD;
                }
            }
            break;
        }
    }

    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = NULL;

    bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
}

bool bt_hfp_ag_indicators_send(uint8_t                          bd_addr[6],
                               T_BT_HFP_AG_SERVICE_INDICATOR    service_indicator,
                               T_BT_HFP_AG_CALL_INDICATOR       call_indicator,
                               T_BT_HFP_AG_CALL_SETUP_INDICATOR call_setup_indicator,
                               T_BT_HFP_AG_CALL_HELD_INDICATOR  call_held_indicator,
                               uint8_t                          signal_indicator,
                               T_BT_HFP_AG_ROAMING_INDICATOR    roaming_indicator,
                               uint8_t                          batt_chg_indicator)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->hfp_data.ag_status_ind.service_status = (T_HFP_SERVICE_STATUS)service_indicator;
        p_link->hfp_data.ag_status_ind.signal_status = signal_indicator;
        p_link->hfp_data.ag_status_ind.roam_status = (T_HFP_ROAM_STATUS)roaming_indicator;
        p_link->hfp_data.ag_status_ind.batt_chg_status = batt_chg_indicator;
        p_link->hfp_data.ag_status_ind.call_status = (T_HFP_CALL_STATUS)call_indicator;
        p_link->hfp_data.ag_status_ind.call_setup_status = (T_HFP_CALL_SETUP_STATUS)call_setup_indicator;
        p_link->hfp_data.ag_status_ind.call_held_status = (T_HFP_CALL_HELD_STATUS)call_held_indicator;

        hfp_ag_send_cind(bd_addr, (T_HFP_SERVICE_STATUS)service_indicator,
                         p_link->hfp_data.ag_status_ind.call_status,
                         p_link->hfp_data.ag_status_ind.call_setup_status,
                         p_link->hfp_data.ag_status_ind.call_held_status,
                         signal_indicator,
                         (T_HFP_ROAM_STATUS)roaming_indicator,
                         batt_chg_indicator);

        bt_hfp_ag_call_status_update(bd_addr, AG_INDICATOR_TYPE_CALL);
        bt_hfp_ag_call_status_update(bd_addr, AG_INDICATOR_TYPE_CALLSETUP);
        bt_hfp_ag_call_status_update(bd_addr, AG_INDICATOR_TYPE_CALLHELD);
    }

    return false;
}

bool bt_hfp_ag_init(uint8_t  link_num,
                    uint8_t  rfc_hfp_chann_num,
                    uint8_t  rfc_hsp_chann_num,
                    uint16_t supported_features,
                    uint8_t  supported_codecs)

{
    return hfp_ag_init(link_num,
                       rfc_hfp_chann_num,
                       rfc_hsp_chann_num,
                       bt_hfp_ag_cback,
                       supported_features,
                       supported_codecs);
}

bool bt_hfp_ag_call_incoming(uint8_t     bd_addr[6],
                             const char *call_num,
                             uint8_t     call_num_len,
                             uint8_t     call_num_type)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if ((p_link->hfp_data.hfp_state == HF_STATE_CONNECTED) ||
            (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED))
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_IDLE)
            {
                T_BT_MSG_PAYLOAD payload;

                p_link->hfp_data.call_status = BT_HFP_AG_CALL_INCOMING;

                if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
                {
                    p_link->hfp_data.ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_INCOMING_CALL;

                    memset(&p_link->hfp_data.ag_call_info, 0, sizeof(p_link->hfp_data.ag_call_info));
                    p_link->hfp_data.ag_call_info.call_num_type = call_num_type;
                    if (call_num_len <= 20)
                    {
                        memcpy(p_link->hfp_data.ag_call_info.call_num, call_num, call_num_len - 1);
                    }
                    else
                    {
                        memcpy(p_link->hfp_data.ag_call_info.call_num, call_num, 19);
                    }

                    hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_INCOMING_CALL);
                }

                if (p_link->hfp_data.supported_features & HFP_INBAND_RINGTONE_ENABLE)
                {
                    if (p_link->sco_handle == 0)
                    {
                        p_link->hfp_data.sco_int_flag = true;
                        bt_hfp_ag_codec_negotiate(p_link->bd_addr);
                    }
                }
                else
                {
                    bt_hfp_ag_ringing_start(bd_addr);
                }

                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_hold(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE)
            {
                p_link->hfp_data.call_status = BT_HFP_AG_CALL_HELD;
                p_link->hfp_data.ag_status_ind.call_held_status = HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL;
                hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLHELD, HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL);

                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
    }

    return false;
}

bool bt_hfp_ag_held_call_accept(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_HELD)
            {
                p_link->hfp_data.call_status = BT_HFP_AG_CALL_ACTIVE;
                p_link->hfp_data.ag_status_ind.call_held_status = HFP_CALL_HELD_STATUS_IDLE;
                hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLHELD, HFP_CALL_HELD_STATUS_IDLE);

                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
    }

    return false;
}

bool bt_hfp_ag_held_call_release(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_HELD)
            {
                p_link->hfp_data.call_status = HFP_CALL_HELD_STATUS_IDLE;
                p_link->hfp_data.ag_status_ind.call_held_status = HFP_CALL_HELD_STATUS_IDLE;
                hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLHELD, HFP_CALL_HELD_STATUS_IDLE);
                hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALL, HFP_CALL_STATUS_IDLE);

                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_answer(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        sys_timer_stop(p_link->hfp_data.timer_ring);

        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_INCOMING ||
                p_link->hfp_data.call_status == BT_HFP_AG_CALL_OUTGOING)
            {
                p_link->hfp_data.call_status = BT_HFP_AG_CALL_ACTIVE;
                p_link->hfp_data.ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_IDLE;
                hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALL, HFP_CALL_STATUS_ACTIVE);
                hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_IDLE);

                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_INCOMING ||
                p_link->hfp_data.call_status == BT_HFP_AG_CALL_OUTGOING)
            {
                p_link->hfp_data.call_status = BT_HFP_AG_CALL_ACTIVE;
                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_terminate(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        sys_timer_stop(p_link->hfp_data.timer_ring);
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_INCOMING ||
                p_link->hfp_data.call_status == BT_HFP_AG_CALL_OUTGOING ||
                p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE)
            {
                if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE)
                {
                    hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALL, HFP_CALL_STATUS_IDLE);
                }
                else
                {
                    p_link->hfp_data.ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_IDLE;
                    hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_IDLE);
                }

                p_link->hfp_data.call_status = BT_HFP_AG_CALL_IDLE;

                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_INCOMING ||
                p_link->hfp_data.call_status == BT_HFP_AG_CALL_OUTGOING ||
                p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE)
            {
                p_link->hfp_data.call_status = BT_HFP_AG_CALL_IDLE;
                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_dial(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_IDLE)
            {
                p_link->hfp_data.call_status = BT_HFP_AG_CALL_OUTGOING;
                p_link->hfp_data.ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_OUTGOING_CALL;
                hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_OUTGOING_CALL);

                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_IDLE)
            {
                p_link->hfp_data.call_status = BT_HFP_AG_CALL_OUTGOING;
                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);

            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_alert(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_OUTGOING)
            {
                p_link->hfp_data.ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_ALERTING;
                hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_ALERTING);
                memcpy(payload.bd_addr, bd_addr, 6);
                payload.msg_buf = NULL;

                return bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_waiting_send(uint8_t     bd_addr[6],
                                 const char *call_num,
                                 uint8_t     call_num_len,
                                 uint8_t     call_num_type)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_AG_CALL_ACTIVE ||
                p_link->hfp_data.call_status == BT_HFP_AG_CALL_HELD)
            {
                char name[20] = {0};

                if (call_num_len <= 20)
                {
                    memcpy(name, call_num, call_num_len - 1);
                }
                else
                {
                    memcpy(name, call_num, 19);
                }

                return hfp_ag_send_ccwa(bd_addr, name, call_num_type);
            }
        }
    }

    return false;
}

bool bt_hfp_ag_microphone_gain_set(uint8_t bd_addr[6],
                                   uint8_t level)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED ||
            p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            return hfp_ag_send_vgm(bd_addr, level);
        }
    }

    return false;
}

bool bt_hfp_ag_speaker_gain_set(uint8_t bd_addr[6],
                                uint8_t level)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED ||
            p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            return hfp_ag_send_vgs(bd_addr, level);
        }
    }

    return false;
}

bool bt_hfp_ag_ring_interval_set(uint8_t  bd_addr[6],
                                 uint16_t interval)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (interval == 0)
        {
            p_link->hfp_data.ring_interval = HFP_AG_DEFAULT_RING_REPEAT_INTERVAL;
        }
        else
        {
            p_link->hfp_data.ring_interval = interval;
        }
    }

    return false;
}

bool bt_hfp_ag_inband_ringing_set(uint8_t bd_addr[6],
                                  bool    enable)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            uint16_t local_capabilities;

            hfp_ag_local_capabilities_get(&local_capabilities);
            if (local_capabilities & AG_CAPABILITY_INBAND_RINGING)
            {
                if (enable)
                {
                    p_link->hfp_data.supported_features |= HFP_INBAND_RINGTONE_ENABLE;
                }
                else
                {
                    p_link->hfp_data.supported_features &= ~HFP_INBAND_RINGTONE_ENABLE;
                }

                return hfp_ag_inband_ringing_set(bd_addr, enable);
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            if (enable)
            {
                p_link->hfp_data.supported_features |= HFP_INBAND_RINGTONE_ENABLE;
            }
            else
            {
                p_link->hfp_data.supported_features &= ~HFP_INBAND_RINGTONE_ENABLE;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_current_calls_list_send(uint8_t                          bd_addr[6],
                                       uint8_t                          call_idx,
                                       uint8_t                          call_dir,
                                       T_BT_HFP_AG_CURRENT_CALL_STATUS  call_status,
                                       T_BT_HFP_AG_CURRENT_CALL_MODE    call_mode,
                                       uint8_t                          mpty,
                                       const char                      *call_num,
                                       uint8_t                          call_num_type)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_ag_current_calls_list_send(bd_addr, call_idx, call_dir, call_status,
                                                  call_mode, mpty, call_num, call_num_type);
        }
    }

    return false;
}

bool bt_hfp_ag_service_indicator_send(uint8_t                       bd_addr[6],
                                      T_BT_HFP_AG_SERVICE_INDICATOR status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_SERVICE, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_call_indicator_send(uint8_t                    bd_addr[6],
                                   T_BT_HFP_AG_CALL_INDICATOR status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            p_link->hfp_data.ag_status_ind.call_status = (T_HFP_CALL_STATUS)status;
            bt_hfp_ag_call_status_update(bd_addr, AG_INDICATOR_TYPE_CALL);

            return hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALL, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_call_setup_indicator_send(uint8_t                          bd_addr[6],
                                         T_BT_HFP_AG_CALL_SETUP_INDICATOR status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            p_link->hfp_data.ag_status_ind.call_setup_status = (T_HFP_CALL_SETUP_STATUS)status;
            bt_hfp_ag_call_status_update(bd_addr, AG_INDICATOR_TYPE_CALLSETUP);

            return hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_call_held_indicator_send(uint8_t                         bd_addr[6],
                                        T_BT_HFP_AG_CALL_HELD_INDICATOR status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            p_link->hfp_data.ag_status_ind.call_held_status = (T_HFP_CALL_HELD_STATUS)status;
            bt_hfp_ag_call_status_update(bd_addr, AG_INDICATOR_TYPE_CALLHELD);

            return hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLHELD, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_signal_strength_send(uint8_t bd_addr[6],
                                    uint8_t value)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_SIGNAL, value);
        }
    }

    return false;
}

bool bt_hfp_ag_roaming_indicator_send(uint8_t                       bd_addr[6],
                                      T_BT_HFP_AG_ROAMING_INDICATOR status)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_ROAM, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_battery_charge_send(uint8_t bd_addr[6],
                                   uint8_t value)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_BATTCHG, value);
        }
    }

    return false;
}

bool bt_hfp_ag_network_operator_name_send(uint8_t     bd_addr[6],
                                          const char *operator,
                                          uint8_t     operator_length)
{
    char name[17] = {0};
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (operator_length <= 17)
            {
                memcpy(name, operator, operator_length - 1);
            }
            else
            {
                memcpy(name, operator, 16);
            }

            hfp_ag_send_cops(bd_addr, name);
        }
    }

    return false;
}

bool bt_hfp_ag_subscriber_number_send(uint8_t     bd_addr[6],
                                      const char *call_num,
                                      uint8_t     call_num_len,
                                      uint8_t     call_num_type,
                                      uint8_t     service)
{
    char num[20] = {0};
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (call_num_len <= 20)
            {
                memcpy(num, call_num, call_num_len - 1);
            }
            else
            {
                memcpy(num, call_num, 19);
            }

            hfp_ag_send_cnum(bd_addr, num, call_num_type, service);
        }
    }

    return false;
}

bool bt_hfp_ag_voice_recognition_set(uint8_t bd_addr[6],
                                     bool    enable)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_ag_send_bvra(bd_addr, enable);
        }
    }

    return false;
}

#else
#include <stdint.h>
#include <stdbool.h>

#include "bt_hfp_ag.h"

void bt_hfp_ag_codec_negotiate(uint8_t bd_addr[6])
{
}

void bt_hfp_ag_ringing_start(uint8_t bd_addr[6])
{
}

bool bt_hfp_ag_connect_req(uint8_t bd_addr[6],
                           uint8_t server_chann,
                           bool    is_hfp)
{
    return false;
}

bool bt_hfp_ag_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_connect_cfm(uint8_t bd_addr[6],
                           bool    accept)
{
    return false;
}

bool bt_hfp_ag_audio_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_audio_connect_cfm(uint8_t bd_addr[6],
                                 bool    accept)
{
    return false;
}

bool bt_hfp_ag_audio_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_send_vnd_at_cmd_req(uint8_t     bd_addr[6],
                                   const char *at_cmd)
{
    return false;
}

bool bt_hfp_ag_ok_send(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_error_send(uint8_t bd_addr[6],
                          uint8_t error_code)
{
    return false;
}

bool bt_hfp_ag_indicators_send(uint8_t                          bd_addr[6],
                               T_BT_HFP_AG_SERVICE_INDICATOR    service_indicator,
                               T_BT_HFP_AG_CALL_INDICATOR       call_indicator,
                               T_BT_HFP_AG_CALL_SETUP_INDICATOR call_setup_indicator,
                               T_BT_HFP_AG_CALL_HELD_INDICATOR  call_held_indicator,
                               uint8_t                          signal_indicator,
                               T_BT_HFP_AG_ROAMING_INDICATOR    roaming_indicator,
                               uint8_t                          batt_chg_indicator)
{
    return false;
}

bool bt_hfp_ag_init(uint8_t  link_num,
                    uint8_t  rfc_hfp_chann_num,
                    uint8_t  rfc_hsp_chann_num,
                    uint16_t supported_features,
                    uint8_t  supported_codecs)
{
    return false;
}

bool bt_hfp_ag_call_incoming(uint8_t     bd_addr[6],
                             const char *call_num,
                             uint8_t     call_num_len,
                             uint8_t     call_num_type)
{
    return false;
}

bool bt_hfp_ag_call_hold(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_held_call_accept(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_held_call_release(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_call_answer(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_call_terminate(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_call_dial(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_call_alert(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_ag_call_waiting_send(uint8_t     bd_addr[6],
                                 const char *call_num,
                                 uint8_t     call_num_len,
                                 uint8_t     call_num_type)
{
    return false;
}

bool bt_hfp_ag_microphone_gain_set(uint8_t bd_addr[6],
                                   uint8_t level)
{
    return false;
}

bool bt_hfp_ag_speaker_gain_set(uint8_t bd_addr[6],
                                uint8_t level)
{
    return false;
}

bool bt_hfp_ag_ring_interval_set(uint8_t  bd_addr[6],
                                 uint16_t interval)
{
    return false;
}

bool bt_hfp_ag_inband_ringing_set(uint8_t bd_addr[6],
                                  bool    enable)
{
    return false;
}

bool bt_hfp_ag_current_calls_list_send(uint8_t                          bd_addr[6],
                                       uint8_t                          call_idx,
                                       uint8_t                          call_dir,
                                       T_BT_HFP_AG_CURRENT_CALL_STATUS  call_status,
                                       T_BT_HFP_AG_CURRENT_CALL_MODE    call_mode,
                                       uint8_t                          mpty,
                                       const char                      *call_num,
                                       uint8_t                          call_num_type)
{
    return false;
}

bool bt_hfp_ag_service_indicator_send(uint8_t                       bd_addr[6],
                                      T_BT_HFP_AG_SERVICE_INDICATOR status)
{
    return false;
}

bool bt_hfp_ag_call_indicator_send(uint8_t                    bd_addr[6],
                                   T_BT_HFP_AG_CALL_INDICATOR status)
{
    return false;
}

bool bt_hfp_ag_call_setup_indicator_send(uint8_t                          bd_addr[6],
                                         T_BT_HFP_AG_CALL_SETUP_INDICATOR status)
{
    return false;
}

bool bt_hfp_ag_call_held_indicator_send(uint8_t                         bd_addr[6],
                                        T_BT_HFP_AG_CALL_HELD_INDICATOR status)
{
    return false;
}

bool bt_hfp_ag_signal_strength_send(uint8_t bd_addr[6],
                                    uint8_t value)
{
    return false;
}

bool bt_hfp_ag_roaming_indicator_send(uint8_t                       bd_addr[6],
                                      T_BT_HFP_AG_ROAMING_INDICATOR status)
{
    return false;
}

bool bt_hfp_ag_battery_charge_send(uint8_t bd_addr[6],
                                   uint8_t value)
{
    return false;
}

bool bt_hfp_ag_network_operator_name_send(uint8_t     bd_addr[6],
                                          const char *operator,
                                          uint8_t     operator_length)
{
    return false;
}

bool bt_hfp_ag_subscriber_number_send(uint8_t     bd_addr[6],
                                      const char *call_num,
                                      uint8_t     call_num_len,
                                      uint8_t     call_num_type,
                                      uint8_t     service)
{
    return false;
}

bool bt_hfp_ag_voice_recognition_set(uint8_t bd_addr[6],
                                     bool    enable)
{
    return false;
}

#endif

