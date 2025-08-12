/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HFP_SUPPORT == 1)
#include <stdio.h>
#include <string.h>

#include "os_mem.h"
#include "trace.h"
#include "gap_br.h"
#include "hfp.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_hfp.h"
#include "bt_hfp_int.h"

#define SCO_PKT_TYPES_T1_T2  (GAP_PKT_TYPE_EV3 | GAP_PKT_TYPE_NO_3EV3 | \
                              GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_S4     (GAP_PKT_TYPE_NO_3EV3 | GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_S3     SCO_PKT_TYPES_S4
#define SCO_PKT_TYPES_D1     (GAP_PKT_TYPE_HV3 | GAP_PKT_TYPE_NO_2EV3 | GAP_PKT_TYPE_NO_3EV3 | \
                              GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)

bool bt_hfp_batt_level_report(uint8_t bd_addr[6],
                              uint8_t level)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_INFO3("bt_hfp_batt_level_report: bd_addr %s, link %p, level %d",
                    TRACE_BDADDR(bd_addr), p_link, level);

    if (p_link != NULL)
    {
        char buf[29];

        p_link->hfp_data.batt_level = level;

        switch (p_link->hfp_data.bat_report_type)
        {
        case BAT_REPORTING_TYPE_NONE:
            {
                snprintf(buf, 23, "AT+XAPL=005D-8763-E,6\r");
            }
            break;
        case BAT_REPORTING_TYPE_BIEV:
            {
                snprintf(buf, 15, "AT+BIEV=2,%u\r", level);
            }
            break;

        case BAT_REPORTING_TYPE_APPLE:
            {
                level = (level + 9) / 10;

                if (level > 0)
                {
                    level--;
                }

                /*Battery Level: string value between '0' and '9'*/
                snprintf(buf, 26, "AT+IPHONEACCEV=2,1,%u,2,0\r", level);
            }
            break;

        case BAT_REPORTING_TYPE_ANDROID:
            {
                level = (level + 9) / 10;

                /*AT+XEVENT=BATTERY,[Level],[NumberOfLevel],[MinutesOfTalk],[IsCharging]*/
                snprintf(buf, 29, "AT+XEVENT=BATTERY,%u,10,0,0\r", level);
            }
            break;

        default:
            return false;
        }

        return hfp_batt_level_report(p_link->bd_addr, buf, p_link->hfp_data.bat_report_type);
    }

    return false;
}

void bt_hfp_cback(uint8_t    bd_addr[6],
                  T_HFP_MSG  msg_type,
                  void      *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = NULL;

    p_link = bt_find_br_link(bd_addr);

    BTM_PRINT_TRACE3("bt_hfp_cback: bd_addr %s, link %p, msg_type 0x%02x",
                     TRACE_BDADDR(bd_addr), p_link, msg_type);

    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
    }

    switch (msg_type)
    {
    case HFP_MSG_RFC_CONN_IND:
        if (p_link != NULL)
        {
            bt_mgr_dispatch(BT_MSG_HFP_CONN_IND, &payload);
        }
        else
        {
            hfp_connect_cfm(bd_addr, false);
        }
        break;

    case HFP_MSG_RFC_CONN:
        if (p_link != NULL)
        {
            T_HFP_MSG_RFC_CONN *p_msg = (T_HFP_MSG_RFC_CONN *)msg_buf;

            p_link->hfp_data.hsp_active_fg = p_msg->hsp;
            p_link->hfp_data.hfp_state = HF_STATE_RFCOMM_CONNECTED;
            p_link->hfp_data.codec_type = CODEC_TYPE_CVSD;
        }
        else
        {
            hfp_disconnect_req(bd_addr);
        }
        break;

    case HFP_MSG_CONN:
        if (p_link != NULL)
        {
            p_link->hfp_data.hfp_role = HFP_ROLE_HF;
            if (p_link->hfp_data.hsp_active_fg)
            {
                p_link->hfp_data.hfp_state = HS_STATE_CONNECTED;
                p_link->connected_profile |= HSP_PROFILE_MASK;
            }
            else
            {
                p_link->hfp_data.hfp_state = HF_STATE_CONNECTED;
                p_link->connected_profile |= HFP_PROFILE_MASK;
                hfp_remote_capabilities_get(bd_addr, &p_link->hfp_data.capabilities);
                if (p_link->hfp_data.capabilities & AG_CAPABILITY_INBAND_RINGING)
                {
                    p_link->hfp_data.supported_features |= HFP_INBAND_RINGTONE_ENABLE;
                }

                if ((p_link->hfp_data.capabilities & AG_CAPABILITY_ESCO_S4_T2_SUPPORTED) &&
                    (hfp_local_capabilities_get() & HF_CAPABILITY_ESCO_S4_T2_SUPPORTED))
                {
                    p_link->hfp_data.supported_features |= HFP_ESCO_S4_ENABLE;
                }
            }

            if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
            {
                hfp_send_cmee_enable(bd_addr);
                hfp_send_ccwa(bd_addr);
                hfp_send_clip(bd_addr);
                /* Some AGs will not send CIND call status if call already exit before HFP SLC finish */
                hfp_send_clcc(bd_addr);
                hfp_network_operator_format_set(bd_addr);
            }

            if (p_link->hfp_data.hsp_active_fg)
            {
                bt_mgr_dispatch(BT_MSG_HSP_CONN_CMPL, &payload);
            }
            else
            {
                bt_mgr_dispatch(BT_MSG_HFP_CONN_CMPL, &payload);
                bt_mgr_dispatch(BT_MSG_HFP_SUPPORTED_FEATURES_IND, &payload);
            }

            bt_mgr_dispatch(BT_MSG_HFP_CALL_STATUS_CHANGED, &payload);
            bt_mgr_dispatch(BT_MSG_HFP_SERVICE_STATUS_CHANGED, &payload);
        }
        break;

    case HFP_MSG_CONN_FAIL:
        if (p_link != NULL)
        {
            payload.msg_buf = msg_buf;

            if (p_link->hfp_data.hsp_active_fg)
            {
                bt_mgr_dispatch(BT_MSG_HSP_CONN_FAIL, &payload);
            }
            else
            {
                bt_mgr_dispatch(BT_MSG_HFP_CONN_FAIL, &payload);
            }

            memset(&p_link->hfp_data, 0, sizeof(T_HFP_LINK_DATA));
        }
        break;

    case HFP_MSG_DISCONN:
        if (p_link != NULL)
        {
            if (p_link->hfp_data.hsp_active_fg)
            {
                p_link->connected_profile &= ~HSP_PROFILE_MASK;
            }
            else
            {
                p_link->connected_profile &= ~HFP_PROFILE_MASK;
            }

            payload.msg_buf = msg_buf;

            if (p_link->sco_handle)
            {
                gap_br_send_sco_disconn_req(p_link->bd_addr);
            }

            if (p_link->hfp_data.hsp_active_fg)
            {
                bt_mgr_dispatch(BT_MSG_HSP_DISCONN_CMPL, &payload);
            }
            else
            {
                bt_mgr_dispatch(BT_MSG_HFP_DISCONN_CMPL, &payload);
            }

            memset(&p_link->hfp_data, 0, sizeof(T_HFP_LINK_DATA));
        }
        break;

    case HFP_MSG_ACK_OK:
        if (p_link != NULL)
        {
            T_HFP_MSG_ACK_OK *p_msg = (T_HFP_MSG_ACK_OK *)msg_buf;

            switch (p_msg->last_cmd_id)
            {
            case HFP_CMD_ID_SEND_CLCC:
                {
                    if (p_link->hfp_data.clcc_ind & CLCC_IND_CALL_HELD)
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_HELD;

                        if (p_link->hfp_data.clcc_ind & CLCC_IND_CALL_ACTIVE)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_HELD;
                        }
                    }
                    else if (p_link->hfp_data.clcc_ind & CLCC_IND_CALL_ACTIVE)
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE;

                        if (p_link->hfp_data.clcc_ind & CLCC_IND_CALL_HELD)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_HELD;
                        }
                        if (p_link->hfp_data.clcc_ind & CLCC_IND_CALL_WAITING)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING;
                        }
                    }
                    else if (p_link->hfp_data.clcc_ind & (CLCC_IND_CALL_INCOMING | CLCC_IND_CALL_WAITING))
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_INCOMING;
                    }
                    else if (p_link->hfp_data.clcc_ind & (CLCC_IND_CALL_DIALING | CLCC_IND_CALL_DIAL_ALERT))
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_OUTGOING;
                    }
                    else if (p_link->hfp_data.clcc_ind == CLCC_IND_CALL_IDLE)
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_IDLE;
                    }

                    p_link->hfp_data.clcc_ind = CLCC_IND_CALL_IDLE;
                    bt_mgr_dispatch(BT_MSG_HFP_CALL_STATUS_CHANGED, &payload);
                }
                break;

            case HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_OK;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION, &payload);
                }
                break;

            case HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_OK;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION, &payload);
                }
                break;

            case HFP_CMD_ID_3WAY_CALL_CONTROL_0:
            case HFP_CMD_ID_3WAY_CALL_CONTROL_1:
            case HFP_CMD_ID_3WAY_CALL_CONTROL_2:
            case HFP_CMD_ID_3WAY_CALL_CONTROL_3:
                break;

            case HFP_CMD_ID_DIAL_LAST_NUMBER:
                break;

            case HFP_CMD_ID_DIAL_WITH_NUMBER:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_OK;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_DIAL_WITH_NUMBER_RESULT, &payload);
                }
                break;

            case HFP_CMD_ID_BATT_XAPL:
                {
                    p_link->hfp_data.bat_report_type = BAT_REPORTING_TYPE_APPLE;
                    bt_hfp_batt_level_report(p_link->bd_addr, p_link->hfp_data.batt_level);
                }
                break;

            case HFP_CMD_ID_SEND_DTMF:
            case HFP_CMD_ID_SET_NETWORK_OPERATOR_FORMAT:
            case HFP_CMD_ID_QUERY_NETWORK_NAME:
            case HFP_CMD_ID_QUERY_SUBSCRIBER_NUM:
                break;

            case HFP_CMD_ID_VND_AT_CMD:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_OK;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_VND_AT_CMD_RSP, &payload);
                }
                break;

            default:
                break;
            }
        }
        break;

    case HFP_MSG_ACK_ERROR:
    case HFP_MSG_CME_ERROR:
        if (p_link != NULL)
        {
            T_HFP_MSG_CME_ERROR *p_msg = (T_HFP_MSG_CME_ERROR *)msg_buf;

            switch (p_msg->last_cmd_id)
            {
            case HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_ERROR;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION, &payload);
                }
                break;

            case HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_ERROR;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION, &payload);
                }
                break;

            case HFP_CMD_ID_DIAL_WITH_NUMBER:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_ERROR;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_DIAL_WITH_NUMBER_RESULT, &payload);
                }
                break;

            case HFP_CMD_ID_DIAL_LAST_NUMBER:
                break;

            case HFP_CMD_ID_REJECT_HANGUP_CALL:
                if ((p_link->hfp_data.call_status == BT_HFP_CALL_OUTGOING) ||
                    (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE))
                {
                    hfp_call_hold_action(bd_addr, RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL);
                }
                break;

            case HFP_CMD_ID_BATT_XAPL:
                {
                    p_link->hfp_data.bat_report_type = BAT_REPORTING_TYPE_ANDROID;
                    bt_hfp_batt_level_report(p_link->bd_addr, p_link->hfp_data.batt_level);
                }
                break;

            case HFP_CMD_ID_VND_AT_CMD:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_ERROR;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_VND_AT_CMD_RSP, &payload);
                }
                break;

            default:
                break;
            }
        }
        break;

    case HFP_MSG_VOICE_RECOGNITION_STATUS:
        if (p_link != NULL)
        {
            T_HFP_MSG_VOICE_RECOGNITION_STATUS *p_msg = (T_HFP_MSG_VOICE_RECOGNITION_STATUS *)msg_buf;

            if (p_msg->status == HFP_VOICE_RECOGNITION_IDLE)
            {
                T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_OK;

                payload.msg_buf = &cmd_rsp;
                bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION, &payload);
            }
            else if (p_msg->status == HFP_VOICE_RECOGNITION_ACTIVE)
            {
                T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_OK;

                payload.msg_buf = &cmd_rsp;
                bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION, &payload);
            }
        }
        break;

    case HFP_MSG_RING:
        if (p_link != NULL)
        {
            if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
            {
                if (p_link->hfp_data.call_status == BT_HFP_CALL_IDLE)
                {
                    p_link->hfp_data.call_status = BT_HFP_CALL_INCOMING;
                }
            }
            else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
            {
                p_link->hfp_data.call_status = BT_HFP_CALL_INCOMING;
            }

            bt_mgr_dispatch(BT_MSG_HFP_RING_ALERT, &payload);
            bt_mgr_dispatch(BT_MSG_HFP_CALL_STATUS_CHANGED, &payload);
        }
        break;

    case HFP_MSG_CLIP:
        if (p_link != NULL)
        {
            T_HFP_MSG_CLIP *p_msg = (T_HFP_MSG_CLIP *)msg_buf;
            T_BT_HFP_CALLER_ID_IND call_clip;

            if (p_link->hfp_data.call_status == BT_HFP_CALL_INCOMING)
            {
                call_clip.type = p_msg->type;
                memcpy(call_clip.number, p_msg->number, 20);
                payload.msg_buf = &call_clip;
                bt_mgr_dispatch(BT_MSG_HFP_CALLER_ID_IND, &payload);
            }
        }
        break;

    case HFP_MSG_CCWA:
        if (p_link != NULL)
        {
            T_HFP_MSG_CCWA *p_msg = (T_HFP_MSG_CCWA *)msg_buf;
            T_BT_HFP_CALL_WAITING_IND call_waiting;

            /* AG shall send +CCWA unsolicited result code to the HF whenever
             * an incoming call is waiting during an ongoing call.
             */
            if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE)
            {
                call_waiting.type = p_msg->type;
                memcpy(call_waiting.number, p_msg->number, 20);
                payload.msg_buf = &call_waiting;
                bt_mgr_dispatch(BT_MSG_HFP_CALL_WAITING_IND, &payload);

                p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING;
                bt_mgr_dispatch(BT_MSG_HFP_CALL_STATUS_CHANGED, &payload);
            }
        }
        break;

    case HFP_MSG_CLCC:
        if (p_link != NULL)
        {
            T_HFP_MSG_CLCC *p_msg = (T_HFP_MSG_CLCC *)msg_buf;

            p_link->hfp_data.clcc_ind |= (1 << p_msg->status);
            payload.msg_buf = msg_buf;
            bt_mgr_dispatch(BT_MSG_HFP_CURRENT_CALL_LIST_IND, &payload);
        }
        break;

    case HFP_MSG_SET_MICROPHONE_GAIN:
        if (p_link != NULL)
        {
            T_HFP_MSG_MICROPHONE_VOLUME_SET *p_msg = (T_HFP_MSG_MICROPHONE_VOLUME_SET *)msg_buf;

            payload.msg_buf = &p_msg->volume;
            bt_mgr_dispatch(BT_MSG_HFP_MIC_GAIN_CHANGED, &payload);
        }
        break;

    case HFP_MSG_SET_SPEAKER_GAIN:
        if (p_link != NULL)
        {
            T_HFP_MSG_SPEAKER_VOLUME_SET *p_msg = (T_HFP_MSG_SPEAKER_VOLUME_SET *)msg_buf;

            payload.msg_buf = &p_msg->volume;
            bt_mgr_dispatch(BT_MSG_HFP_SPK_GAIN_CHANGED, &payload);
        }
        break;

    case HFP_MSG_AG_INDICATOR_EVENT:
        if (p_link != NULL)
        {
            T_HFP_MSG_AG_INDICATOR_EVENT *p_msg = (T_HFP_MSG_AG_INDICATOR_EVENT *)msg_buf;

            switch (p_msg->event_type)
            {
            case AG_INDICATOR_TYPE_UNKOWN:
                break;

            case AG_INDICATOR_TYPE_SERVICE:
                p_link->hfp_data.ag_status_ind.service_status = (T_HFP_SERVICE_STATUS)(p_msg->state);
                bt_mgr_dispatch(BT_MSG_HFP_SERVICE_STATUS_CHANGED, &payload);
                break;

            case AG_INDICATOR_TYPE_CALL:
                p_link->hfp_data.ag_status_ind.call_status = (T_HFP_CALL_STATUS)(p_msg->state);

                if (p_link->hfp_data.hfp_state != HF_STATE_CONNECTED) //service level
                {
                    p_link->hfp_data.call_status = BT_HFP_CALL_IDLE;
                    if (p_msg->state == HFP_CALL_STATUS_ACTIVE)
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE;
                    }
                }
                else //after finish service level
                {
                    if (p_msg->state == HFP_CALL_STATUS_IDLE)
                    {
                        if (p_link->hfp_data.call_status != BT_HFP_CALL_IDLE)
                        {
                            if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE)
                            {
                                p_link->hfp_data.call_status = BT_HFP_CALL_IDLE;
                            }
                            if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD)
                            {
                                p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE;
                            }
                            if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING)
                            {
                                p_link->hfp_data.call_status = BT_HFP_CALL_INCOMING;
                            }

                            hfp_send_clcc(bd_addr);
                        }
                    }
                    else if (p_msg->state == HFP_CALL_STATUS_ACTIVE)
                    {
                        if (p_link->hfp_data.call_status != BT_HFP_CALL_ACTIVE)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE;
                        }
                    }

                    bt_mgr_dispatch(BT_MSG_HFP_CALL_STATUS_CHANGED, &payload);
                }
                break;

            case AG_INDICATOR_TYPE_CALLSETUP:
                p_link->hfp_data.ag_status_ind.call_setup_status = (T_HFP_CALL_SETUP_STATUS)(p_msg->state);

                if (p_link->hfp_data.hfp_state != HF_STATE_CONNECTED) //service level
                {
                    if (p_msg->state == HFP_CALL_SETUP_STATUS_INCOMING_CALL)
                    {
                        if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING;
                        }
                        else
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_INCOMING;
                        }
                    }
                    else if ((p_msg->state == HFP_CALL_SETUP_STATUS_OUTGOING_CALL) ||
                             (p_msg->state == HFP_CALL_SETUP_STATUS_ALERTING))
                    {
                        if (p_link->hfp_data.call_status == BT_HFP_CALL_IDLE)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_OUTGOING;
                        }
                    }
                }
                else //after finish service level
                {
                    if (p_msg->state == HFP_CALL_SETUP_STATUS_IDLE)
                    {
                        if ((p_link->hfp_data.call_status == BT_HFP_CALL_INCOMING) ||
                            (p_link->hfp_data.call_status == BT_HFP_CALL_OUTGOING))
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_IDLE;
                        }
                        else if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE;
                        }
                    }
                    else if (p_msg->state == HFP_CALL_SETUP_STATUS_INCOMING_CALL)
                    {
                        if (p_link->hfp_data.call_status == BT_HFP_CALL_IDLE)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_INCOMING;
                        }
                        else if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING;
                        }
                    }
                    else if ((p_msg->state == HFP_CALL_SETUP_STATUS_OUTGOING_CALL) ||
                             (p_msg->state == HFP_CALL_SETUP_STATUS_ALERTING))
                    {
                        if (p_link->hfp_data.call_status == BT_HFP_CALL_IDLE)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_OUTGOING;
                        }
                    }

                    bt_mgr_dispatch(BT_MSG_HFP_CALL_STATUS_CHANGED, &payload);
                }
                break;

            case AG_INDICATOR_TYPE_CALLHELD:
                p_link->hfp_data.ag_status_ind.call_held_status = (T_HFP_CALL_HELD_STATUS)(p_msg->state);

                if (p_link->hfp_data.hfp_state != HF_STATE_CONNECTED) //service level
                {
                    if (p_msg->state == HFP_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL)
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_HELD;
                    }
                    else if (p_msg->state == HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL)
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_HELD;
                    }
                }
                else //after finish service level
                {
                    if (p_msg->state == HFP_CALL_HELD_STATUS_IDLE)
                    {
                        if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD)
                        {
                            p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE;
                        }
                        else if (p_link->hfp_data.call_status == BT_HFP_CALL_HELD)
                        {
                            /* When received this event, the call status may be call idle or call active.
                             * So do not change call status directly, use clcc to query.
                             */
                        }

                        hfp_send_clcc(bd_addr);
                    }
                    else if (p_msg->state == HFP_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL)
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_HELD;
                    }
                    else if (p_msg->state == HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL)
                    {
                        p_link->hfp_data.call_status = BT_HFP_CALL_HELD;
                    }

                    bt_mgr_dispatch(BT_MSG_HFP_CALL_STATUS_CHANGED, &payload);
                }
                break;

            case AG_INDICATOR_TYPE_SIGNAL:
                if (p_link != NULL)
                {
                    p_link->hfp_data.ag_status_ind.signal_status = p_msg->state;
                    bt_mgr_dispatch(BT_MSG_HFP_SIGNAL_IND, &payload);
                }
                break;

            case AG_INDICATOR_TYPE_ROAM:
                if (p_link != NULL)
                {
                    p_link->hfp_data.ag_status_ind.roam_status = (T_HFP_ROAM_STATUS)p_msg->state;
                    bt_mgr_dispatch(BT_MSG_HFP_ROAM_IND, &payload);
                }
                break;

            case AG_INDICATOR_TYPE_BATTCHG:
                if (p_link != NULL)
                {
                    p_link->hfp_data.ag_status_ind.batt_chg_status = p_msg->state;
                    bt_mgr_dispatch(BT_MSG_HFP_BATTERY_IND, &payload);
                }
                break;

            default:
                break;
            }
        }
        break;

    case HFP_MSG_SET_CODEC_TYPE:
        if (p_link != NULL)
        {
            T_HFP_MSG_SET_CODEC_TYPE *p_msg = msg_buf;

            p_link->hfp_data.codec_type = p_msg->codec_type;
        }
        break;

    case HFP_MSG_AG_INBAND_RINGTONE_SETTING:
        if (p_link != NULL)
        {
            T_HFP_MSG_AG_INBAND_RINGTONE_SET *p_msg = (T_HFP_MSG_AG_INBAND_RINGTONE_SET *)msg_buf;

            if (p_msg->ag_support)
            {
                p_link->hfp_data.supported_features |= HFP_INBAND_RINGTONE_ENABLE;
            }
            else
            {
                p_link->hfp_data.supported_features &= ~HFP_INBAND_RINGTONE_ENABLE;
            }
        }
        break;

    case HFP_MSG_AG_BIND:
        if (p_link != NULL)
        {
            T_HFP_MSG_BIND_STATUS *p_msg = (T_HFP_MSG_BIND_STATUS *)msg_buf;
            p_link->hfp_data.bat_report_type = BAT_REPORTING_TYPE_BIEV;

            if (p_msg->batt_ind_enable)
            {
                p_link->hfp_data.supported_features |= HFP_BATTERY_BIEV_ENABLE;
            }
            else
            {
                p_link->hfp_data.supported_features &= ~HFP_BATTERY_BIEV_ENABLE;
            }
        }
        break;

    case HFP_MSG_COPS:
        if (p_link != NULL)
        {
            T_HFP_MSG_COPS *p_msg = (T_HFP_MSG_COPS *)msg_buf;
            T_BT_HFP_NETWORK_OPERATOR_IND network_operator;

            network_operator.mode = p_msg->mode;
            network_operator.format = p_msg->format;
            memcpy(network_operator.name, p_msg->operator, 17);

            payload.msg_buf = &network_operator;
            bt_mgr_dispatch(BT_MSG_HFP_NETWORK_OPERATOR_IND, &payload);
        }
        break;

    case HFP_MSG_CNUM:
        if (p_link != NULL)
        {
            T_HFP_MSG_CNUM *p_msg = (T_HFP_MSG_CNUM *)msg_buf;
            T_BT_HFP_SUBSCRIBER_NUMBER_IND subscriber_num;

            memcpy(subscriber_num.number, p_msg->number, 20);
            subscriber_num.type = p_msg->type;
            subscriber_num.service = p_msg->service;

            payload.msg_buf = &subscriber_num;
            bt_mgr_dispatch(BT_MSG_HFP_SUBSCRIBER_NUMBER_IND, &payload);
        }
        break;

    case HFP_MSG_UNKNOWN_CMD:
        if (p_link != NULL)
        {
            T_HFP_MSG_UNKNOWN_CMD *p_msg = (T_HFP_MSG_UNKNOWN_CMD *)msg_buf;

            payload.msg_buf = p_msg->at_cmd;
            bt_mgr_dispatch(BT_MSG_HFP_UNKNOWN_CMD, &payload);
        }
        break;

    case HFP_MSG_ACK_TIMEOUT:
        if (p_link != NULL)
        {
            T_HFP_MSG_ACK_OK *p_msg = (T_HFP_MSG_ACK_OK *)msg_buf;

            switch (p_msg->last_cmd_id)
            {
            case HFP_CMD_ID_BATT_XAPL:
                {
                    p_link->hfp_data.bat_report_type = BAT_REPORTING_TYPE_APPLE;
                    bt_hfp_batt_level_report(p_msg->bd_addr, p_link->hfp_data.batt_level);
                }
                break;

            case HFP_CMD_ID_SEND_CLCC:
                {
                    p_link->hfp_data.clcc_ind = CLCC_IND_CALL_IDLE;
                }
                break;

            case HFP_CMD_ID_DIAL_WITH_NUMBER:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_TIMEOUT;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_DIAL_WITH_NUMBER_RESULT, &payload);
                }
                break;

            case HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_TIMEOUT;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION, &payload);
                }
                break;

            case HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_TIMEOUT;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION, &payload);
                }
                break;

            case HFP_CMD_ID_VND_AT_CMD:
                {
                    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_TIMEOUT;

                    payload.msg_buf = &cmd_rsp;
                    bt_mgr_dispatch(BT_MSG_HFP_VND_AT_CMD_RSP, &payload);
                }
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }
}

bool bt_hfp_connect_req(uint8_t bd_addr[6],
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
            if (hfp_connect_req(bd_addr, server_chann, is_hfp))
            {
                p_link->hfp_data.hfp_state = HF_STATE_CONNECTING;
                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_disconnect_req(uint8_t bd_addr[6])
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

        return hfp_disconnect_req(bd_addr);
    }

    return false;
}

bool bt_hfp_send_vnd_at_cmd_req(uint8_t     bd_addr[6],
                                const char *at_cmd)
{
    return hfp_send_vnd_cmd(bd_addr, at_cmd);
}

bool bt_hfp_speaker_gain_level_report(uint8_t bd_addr[6],
                                      uint8_t level)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return hfp_inform_ag_speaker_gain(bd_addr, level);
    }

    return false;
}

bool bt_hfp_microphone_gain_level_report(uint8_t bd_addr[6],
                                         uint8_t level)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return hfp_inform_ag_microphone_gain(bd_addr, level);
    }

    return false;
}

bool bt_hfp_dial_with_number_req(uint8_t     bd_addr[6],
                                 const char *number)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_dial_with_number(bd_addr, number);
        }
    }

    return false;
}

bool bt_hfp_dial_last_number_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_dial_last_number(bd_addr);
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            return hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_call_answer_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);

        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_CALL_INCOMING)
            {
                return hfp_accept_phone_call(bd_addr);
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            return hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_call_terminate_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if ((p_link->hfp_data.call_status == BT_HFP_CALL_INCOMING) ||
                (p_link->hfp_data.call_status == BT_HFP_CALL_OUTGOING) ||
                (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE) ||
                (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING) ||
                (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD))
            {
                return hfp_reject_phone_call(bd_addr);
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            return hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_call_hold_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_call_hold_action(bd_addr, HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL);
        }
    }

    return false;
}

bool bt_hfp_held_call_accept_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_call_hold_action(bd_addr, HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL);
        }
    }

    return false;
}

bool bt_hfp_held_call_release_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_call_hold_action(bd_addr, RELEASE_HELD_OR_WAITING_CALL);
        }
    }

    return false;
}

bool bt_hfp_current_call_list_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_send_clcc(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_release_held_or_waiting_call_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if ((p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD) ||
                (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
            {
                return hfp_call_hold_action(bd_addr, RELEASE_HELD_OR_WAITING_CALL);
            }
        }
    }

    return false;
}

bool bt_hfp_release_active_call_accept_held_or_waiting_call_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if ((p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD) ||
                (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
            {
                return hfp_call_hold_action(bd_addr, RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL);
            }
        }
    }

    return false;
}

bool bt_hfp_hold_active_call_accept_held_or_waiting_call_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.capabilities & AG_CAPABILITY_3WAY)
            {
                if ((p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD) ||
                    (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
                {
                    return hfp_call_hold_action(bd_addr, HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL);
                }
            }
        }
    }

    return false;
}

bool bt_hfp_join_two_calls_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD)
            {
                return hfp_call_hold_action(bd_addr, JOIN_TWO_CALLS);
            }
        }
    }

    return false;
}

bool bt_hfp_connect_cfm(uint8_t bd_addr[6],
                        bool    accept)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return hfp_connect_cfm(bd_addr, accept);
    }

    return false;
}

bool bt_hfp_audio_connect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            bt_sniff_mode_exit(p_link, false);

            if (p_link->sco_handle == 0)
            {
                if ((p_link->hfp_data.capabilities & AG_CAPABILITY_CODEC_NEGOTIATION) &&
                    (hfp_local_capabilities_get() & HF_CAPABILITY_CODEC_NEGOTIATION))
                {
                    hfp_ask_ag_establish_voice_chann(p_link->bd_addr);
                }
                else
                {
                    if ((p_link->hfp_data.capabilities & AG_CAPABILITY_ESCO_S4_T2_SUPPORTED) &&
                        (hfp_local_capabilities_get() & HF_CAPABILITY_ESCO_S4_T2_SUPPORTED))
                    {
                        gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 12, 0x0360, 2, SCO_PKT_TYPES_S4);
                    }
                    else
                    {
                        gap_br_send_sco_conn_req(p_link->bd_addr, 8000, 8000, 10, 0x0360, 1, SCO_PKT_TYPES_S3);
                    }
                }
            }

            return true;
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            return hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_audio_connect_cfm(uint8_t bd_addr[6],
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
                /* eSCO parameter set to "T2"
                   voice setting |= 0x0300 mean bypass MAC HW audio decoder/encoder*/
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
                    /* SCO link */
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
                    /* eSCO parameter set to "S4", iop: S4 must be used in
                       mac cvsd whether supported or not */
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

bool bt_hfp_audio_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->sco_handle)
            {
                if (gap_br_send_sco_disconn_req(p_link->bd_addr) == GAP_CAUSE_SUCCESS)
                {
                    return true;
                }
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            return hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_voice_recognition_enable_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if ((p_link->hfp_data.capabilities & AG_CAPABILITY_VOICE_RECOGNITION) &&
                (hfp_local_capabilities_get() & HF_CAPABILITY_VOICE_RECOGNITION))
            {
                return hfp_set_voice_recognition(bd_addr, 1);
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            return hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_voice_recognition_disable_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if ((p_link->hfp_data.capabilities & AG_CAPABILITY_VOICE_RECOGNITION) &&
                (hfp_local_capabilities_get() & HF_CAPABILITY_VOICE_RECOGNITION))
            {
                return hfp_set_voice_recognition(bd_addr, 0);
            }
        }
        else if (p_link->hfp_data.hfp_state == HS_STATE_CONNECTED)
        {
            return hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_dtmf_code_transmit(uint8_t bd_addr[6],
                               char    code)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            if (p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE ||
                p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING ||
                p_link->hfp_data.call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD)
            {
                return hfp_dtmf_send(bd_addr, code);
            }
        }
    }

    return false;
}

bool bt_hfp_network_operator_name_query(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_network_operator_name_query(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_subscriber_number_query(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_subscriber_num_query(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_indicator_activate(uint8_t  bd_addr[6],
                               uint32_t indicator_types)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_indicator_activate(bd_addr, indicator_types);
        }
    }

    return false;
}

bool bt_hfp_indicator_deactivate(uint8_t  bd_addr[6],
                                 uint32_t indicator_types)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_indicator_deactivate(bd_addr, indicator_types);
        }
    }

    return false;
}

bool bt_hfp_nrec_disable(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->hfp_data.hfp_state == HF_STATE_CONNECTED)
        {
            return hfp_send_nrec_disable(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_init(uint8_t  link_num,
                 uint8_t  rfc_hfp_chann_num,
                 uint8_t  rfc_hsp_chann_num,
                 uint16_t supported_features,
                 uint8_t  supported_codecs)
{
    return hfp_init(link_num,
                    rfc_hfp_chann_num,
                    rfc_hsp_chann_num,
                    bt_hfp_cback,
                    supported_features,
                    supported_codecs);
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "hfp.h"

bool bt_hfp_init(uint8_t  link_num,
                 uint8_t  rfc_hfp_chann_num,
                 uint8_t  rfc_hsp_chann_num,
                 uint16_t supported_features,
                 uint8_t  supported_codecs)
{
    return false;
}

bool bt_hfp_connect_req(uint8_t bd_addr[6],
                        uint8_t server_chann,
                        bool    is_hfp)
{
    return false;
}

bool bt_hfp_connect_cfm(uint8_t bd_addr[6],
                        bool    accept)
{
    return false;
}

bool bt_hfp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_send_vnd_at_cmd_req(uint8_t     bd_addr[6],
                                const char *at_cmd)
{
    return false;
}

bool bt_hfp_dial_with_number_req(uint8_t     bd_addr[6],
                                 const char *number)
{
    return false;
}

bool bt_hfp_dial_last_number_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_call_answer_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_call_terminate_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_call_hold_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_held_call_accept_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_held_call_release_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_current_call_list_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_release_held_or_waiting_call_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_release_active_call_accept_held_or_waiting_call_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_hold_active_call_accept_held_or_waiting_call_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_join_two_calls_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_audio_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_audio_connect_cfm(uint8_t bd_addr[6],
                              bool    accept)
{
    return false;
}

bool bt_hfp_audio_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_voice_recognition_enable_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_voice_recognition_disable_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_speaker_gain_level_report(uint8_t bd_addr[6],
                                      uint8_t level)
{
    return false;
}

bool bt_hfp_microphone_gain_level_report(uint8_t bd_addr[6],
                                         uint8_t level)
{
    return false;
}

bool bt_hfp_dtmf_code_transmit(uint8_t bd_addr[6],
                               char    code)
{
    return false;
}

bool bt_hfp_network_operator_name_query(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_subscriber_number_query(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_batt_level_report(uint8_t bd_addr[6],
                              uint8_t level)
{
    return false;
}

bool bt_hfp_nrec_disable(uint8_t bd_addr[6])
{
    return false;
}

bool bt_hfp_indicator_activate(uint8_t  bd_addr[6],
                               uint32_t indicator_types)
{
    return false;
}

bool bt_hfp_indicator_deactivate(uint8_t  bd_addr[6],
                                 uint32_t indicator_types)
{
    return false;
}
#endif
