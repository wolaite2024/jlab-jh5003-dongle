/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HFP_SUPPORT == 1)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "trace.h"
#include "os_queue.h"
#include "gap_br.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "hfp_utils.h"
#include "bt_hfp.h"
#include "bt_hfp_int.h"
#include "rfc.h"

#define SCO_PKT_TYPES_T1_T2  (GAP_PKT_TYPE_EV3 | GAP_PKT_TYPE_NO_3EV3 | \
                              GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_S4     (GAP_PKT_TYPE_NO_3EV3 | GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)
#define SCO_PKT_TYPES_S3     SCO_PKT_TYPES_S4
#define SCO_PKT_TYPES_D1     (GAP_PKT_TYPE_HV3 | GAP_PKT_TYPE_NO_2EV3 | GAP_PKT_TYPE_NO_3EV3 | \
                              GAP_PKT_TYPE_NO_2EV5 | GAP_PKT_TYPE_NO_3EV5)

#define HFP_INBAND_RINGTONE_ENABLE      0x01
#define HFP_BATTERY_BIEV_ENABLE         0x02
#define HFP_ESCO_S4_ENABLE              0x04
#define HFP_CODEC_NEGOTIATION_ENABLE    0x08

#define HF_BIND_ENHANCED_SAFETY         0x01
#define HF_BIND_BATTERY_LEVEL           0x02

#define HFP_AG_INDICATOR_COUNT 7

#define CLCC_IND_CALL_IDLE              0x00
#define CLCC_IND_CALL_ACTIVE            0x01
#define CLCC_IND_CALL_HELD              0x02
#define CLCC_IND_CALL_DIALING           0x04
#define CLCC_IND_CALL_DIAL_ALERT        0x08
#define CLCC_IND_CALL_INCOMING          0x10
#define CLCC_IND_CALL_WAITING           0x20

typedef enum t_hfp_api_id
{
    HFP_CMD_ID_DIAL_WITH_NUMBER               = 0x00,
    HFP_CMD_ID_DIAL_LAST_NUMBER               = 0x01,
    HFP_CMD_ID_SEND_DTMF                      = 0x02,
    HFP_CMD_ID_ACCEPT_CALL                    = 0x03,
    HFP_CMD_ID_REJECT_HANGUP_CALL             = 0x04,
    HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE   = 0x05,
    HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE = 0x06,
    HFP_CMD_ID_3WAY_CALL_CONTROL_0            = 0x07,
    HFP_CMD_ID_3WAY_CALL_CONTROL_1            = 0x08,
    HFP_CMD_ID_3WAY_CALL_CONTROL_2            = 0x09,
    HFP_CMD_ID_3WAY_CALL_CONTROL_3            = 0x0a,
    HFP_CMD_ID_3WAY_CALL_CONTROL_AB           = 0x0b,
    HFP_CMD_ID_INFORM_MICROPHONE_GAIN         = 0x0c,
    HFP_CMD_ID_INFORM_SPEAKER_GAIN            = 0x0d,
    HFP_CMD_ID_HSP_BUTTON_PRESS               = 0x0e,
    HFP_CMD_ID_SEND_CLCC                      = 0x0f,
    HFP_CMD_ID_ESTABLISH_VOICE                = 0x10,
    HFP_CMD_ID_BCS                            = 0x11,
    HFP_CMD_ID_BAC                            = 0x12,
    HFP_CMD_ID_BATT_XAPL                      = 0x13,
    HFP_CMD_ID_BATT_BIEV                      = 0x14,
    HFP_CMD_ID_BATT_IPHONEACCEV               = 0x15,
    HFP_CMD_ID_BATT_XEVENT                    = 0x16,
    HFP_CMD_ID_CLIP                           = 0x17,
    HFP_CMD_ID_NREC                           = 0x18,
    HFP_CMD_ID_CMEE                           = 0x19,
    HFP_CMD_ID_CCWA                           = 0x1a,
    HFP_CMD_ID_SET_NETWORK_OPERATOR_FORMAT    = 0x1b,
    HFP_CMD_ID_QUERY_NETWORK_NAME             = 0x1c,
    HFP_CMD_ID_QUERY_SUBSCRIBER_NUM           = 0x1d,
    HFP_CMD_ID_INDICATOR_ACTIVATE             = 0x1e,
    HFP_CMD_ID_INDICATOR_DEACTIVATE           = 0x1f,
    HFP_CMD_ID_VND_AT_CMD                     = 0x20,
} T_HFP_API_ID;

typedef enum t_bt_hfp_cmd_rsp
{
    HFP_CMD_OK          = 0x00,
    HFP_CMD_ERROR       = 0x01,
    HFP_CMD_TIMEOUT     = 0x02,
} T_BT_HFP_CMD_RSP;

/*voice recognition status of the phone as indicated */
typedef enum t_hfp_voice_recognition
{
    HFP_VOICE_RECOGNITION_IDLE   = 0x00,
    HFP_VOICE_RECOGNITION_ACTIVE = 0x01,
} T_HFP_VOICE_RECOGNITION;

typedef enum t_bt_hfp_batt_reporting_type
{
    BATT_REPORTING_TYPE_NONE    = 0x00,
    BATT_REPORTING_TYPE_BIEV    = 0x01,
    BATT_REPORTING_TYPE_APPLE   = 0x02,
    BATT_REPORTING_TYPE_ANDROID = 0x03,
} T_BT_HFP_BATT_REPORTING_TYPE;

typedef enum t_bt_hfp_call_hold_action
{
    RELEASE_HELD_OR_WAITING_CALL                    = 0x00,
    RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL = 0x01,
    HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL    = 0x02,
    JOIN_TWO_CALLS                                  = 0x03,
} T_BT_HFP_CALL_HOLD_ACTION;

typedef struct t_hfp_indicator_name_table
{
    char    *indicator_name;
    uint8_t  indicator_type;
} T_HFP_INDICATOR_NAME_TABLE;

typedef struct t_bt_hfp_link
{
    struct t_bt_hfp_link *next;
    void                 *timer_handle;
    uint8_t               bd_addr[6];
    T_BT_HFP_STATE        hfp_state;
    T_HFP_SRV_LEVEL_STEP  service_level_step;
    T_HFP_CODEC_TYPE      codec_type;
    uint8_t               dlci;
    uint8_t               credits;
    uint8_t               supported_features;
    uint16_t              frame_size;
    uint16_t              cpbs;
    uint16_t              uuid;             /* indicates hfp or hsp */
    uint8_t               supt_codecs;      /* bit0:CVSD bit1:mSBC bit2~15 vendor */
    uint8_t               indicator_cnt;
    uint8_t               indicator_type[20];
    uint8_t               indicator_range[20];
    T_OS_QUEUE            cmd_queue;        /* cmd from app api calls are buffered into this queue */
    T_HFP_AG_STATUS_IND   ag_status_ind;
    uint8_t              *rx_ptr;
    uint16_t              rx_len;
    uint8_t               batt_report_type;
    uint8_t               batt_level;
    uint8_t               clcc_ind;
    T_BT_HFP_CALL_STATUS  prev_call_status;
    T_BT_HFP_CALL_STATUS  curr_call_status;
} T_BT_HFP_LINK;

typedef struct t_bt_hfp
{
    T_OS_QUEUE link_list;
    uint16_t   brsf_cpbs;
    uint8_t    hfp_service_id;
    uint8_t    hsp_service_id;
    uint16_t   wait_rsp_tout;
    uint8_t    supt_codecs;      /* bit0:CVSD;bit1:mSBC,bit2~15 vendor */
} T_BT_HFP;

/* The mapping of indicator with name, cind response, type, and range */
const T_HFP_INDICATOR_NAME_TABLE INDICATOR_NAME_TABLE[HFP_AG_INDICATOR_COUNT] =
{
    { "service",    AG_INDICATOR_TYPE_SERVICE},
    { "call",       AG_INDICATOR_TYPE_CALL},
    { "callsetup",  AG_INDICATOR_TYPE_CALLSETUP},
    { "callheld",   AG_INDICATOR_TYPE_CALLHELD},
    { "signal",     AG_INDICATOR_TYPE_SIGNAL},
    { "roam",       AG_INDICATOR_TYPE_ROAM},
    { "battchg",    AG_INDICATOR_TYPE_BATTCHG},
};

static T_BT_HFP *bt_hfp;

void bt_hfp_tout_cback(T_SYS_TIMER_HANDLE handle);

T_BT_HFP_LINK *bt_hfp_alloc_link(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;
    uint32_t       ret = 0;

    link = calloc(1, sizeof(T_BT_HFP_LINK));
    if (link == NULL)
    {
        ret = 1;
        goto fail_alloc_link;
    }

    link->timer_handle = sys_timer_create("bt_hfp_wait_rsp",
                                          SYS_TIMER_TYPE_LOW_PRECISION,
                                          (uint32_t)link,
                                          bt_hfp->wait_rsp_tout * 1000,
                                          false,
                                          bt_hfp_tout_cback);
    if (link->timer_handle == NULL)
    {
        ret = 2;
        goto fail_create_timer;
    }

    memcpy(link->bd_addr, bd_addr, 6);
    link->hfp_state = HFP_STATE_DISCONNECTED;
    os_queue_in(&bt_hfp->link_list, link);

    return link;

fail_create_timer:
    free(link);
fail_alloc_link:
    PROFILE_PRINT_ERROR1("bt_hfp_alloc_link: failed %d", ret);
    return NULL;
}

void bt_hfp_free_link(T_BT_HFP_LINK *link)
{
    if (os_queue_delete(&bt_hfp->link_list, link) == true)
    {
        T_HFP_CMD_ITEM *cmd_item;

        cmd_item = os_queue_out(&link->cmd_queue);
        while (cmd_item)
        {
            free(cmd_item);
            cmd_item = os_queue_out(&link->cmd_queue);
        }

        if (link->rx_ptr != NULL)
        {
            free(link->rx_ptr);
        }

        if (link->timer_handle != NULL)
        {
            sys_timer_delete(link->timer_handle);
        }

        free(link);
    }
}

T_BT_HFP_LINK *bt_hfp_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link = NULL;

    if (bt_hfp != NULL)
    {
        link = os_queue_peek(&bt_hfp->link_list, 0);
        while (link != NULL)
        {
            if (!memcmp(link->bd_addr, bd_addr, 6))
            {
                break;
            }

            link = link->next;
        }
    }

    return link;
}

bool bt_hfp_cmd_send(T_BT_HFP_LINK *link,
                     const char    *at_cmd,
                     uint8_t        cmd_len)
{
    if (rfc_data_req(link->bd_addr, link->dlci, (uint8_t *)at_cmd,
                     cmd_len, false) == true)
    {
        link->credits = 0;
        sys_timer_start(link->timer_handle);
        return true;
    }

    return false;
}

bool bt_hfp_cmd_flush(T_BT_HFP_LINK *link)
{
    if ((link->hfp_state >= HFP_STATE_CONNECTED) && (link->credits > 0))
    {
        T_HFP_CMD_ITEM *cmd_item;

        cmd_item = os_queue_peek(&link->cmd_queue, 0);
        if (cmd_item != NULL)
        {
            return bt_hfp_cmd_send(link, cmd_item->at_cmd, cmd_item->cmd_len);
        }
    }

    return false;
}

bool bt_hfp_cmd_process(uint8_t     bd_addr[6],
                        uint8_t     cmd_id,
                        const char *at_cmd)
{
    T_BT_HFP_LINK  *link;
    T_HFP_CMD_ITEM *cmd_item;
    int             cmd_len;
    int32_t         ret = 0;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    cmd_len = strlen(at_cmd);
    if ((cmd_len < 1) || (cmd_len > link->frame_size))
    {
        ret = 2;
        goto fail_invalid_cmd_length;
    }

    if (at_cmd[cmd_len - 1] != '\r')
    {
        ret = 3;
        goto fail_invalid_cmd_delimiter;
    }

    cmd_item = calloc(1, sizeof(T_HFP_CMD_ITEM) + cmd_len);
    if (cmd_item == NULL)
    {
        ret = 4;
        goto fail_alloc_cmd_item;
    }

    cmd_item->cmd_id = cmd_id;
    cmd_item->cmd_len = cmd_len;
    memcpy((char *)cmd_item->at_cmd, at_cmd, cmd_len);

    os_queue_in(&link->cmd_queue, cmd_item);

    bt_hfp_cmd_flush(link);
    return true;

fail_alloc_cmd_item:
fail_invalid_cmd_delimiter:
fail_invalid_cmd_length:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_hfp_cmd_process: bd_addr %s, failed %d",
                         TRACE_BDADDR(bd_addr), -ret);
    return false;
}

void bt_hfp_call_status_set(T_BT_HFP_LINK *link, T_BT_HFP_CALL_STATUS call_status)
{
    if (link->curr_call_status != call_status)
    {
        T_BT_MSG_PAYLOAD          payload;
        T_BT_HFP_CALL_STATUS_INFO info;

        link->prev_call_status = link->curr_call_status;
        link->curr_call_status = call_status;
        memcpy(payload.bd_addr, link->bd_addr, 6);
        info.prev_call_status = link->prev_call_status;
        info.curr_call_status = link->curr_call_status;
        payload.msg_buf = &info;
        bt_mgr_dispatch(BT_MSG_HFP_CALL_STATUS_CHANGED, &payload);
    }
}

bool bt_hfp_pm_cback(uint8_t       bd_addr[6],
                     T_BT_PM_EVENT event)
{
    T_BT_HFP_LINK *link;
    bool           ret = true;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
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
            break;

        case BT_PM_EVENT_SNIFF_ENTER_REQ:
            {
                if (link->curr_call_status != BT_HFP_CALL_IDLE)
                {
                    /* sniff mode disallowed */
                    ret = false;
                }
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_SUCCESS:
            break;

        case BT_PM_EVENT_SNIFF_EXIT_FAIL:
            break;

        case BT_PM_EVENT_SNIFF_EXIT_REQ:
            break;
        }
    }

    return ret;
}

bool bt_hfp_batt_level_report(uint8_t bd_addr[6],
                              uint8_t level)
{
    T_BT_HFP_LINK *link;

    BTM_PRINT_TRACE2("bt_hfp_batt_level_report: bd_addr %s, level %d", TRACE_BDADDR(bd_addr), level);

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        char    buf[29];
        uint8_t cmd_id = 0;

        link->batt_level = level;

        switch (link->batt_report_type)
        {
        case BATT_REPORTING_TYPE_NONE:
            {
                cmd_id = HFP_CMD_ID_BATT_XAPL;
                snprintf(buf, 23, "AT+XAPL=005D-8763-E,6\r");
            }
            break;
        case BATT_REPORTING_TYPE_BIEV:
            {
                cmd_id = HFP_CMD_ID_BATT_BIEV;
                snprintf(buf, 15, "AT+BIEV=2,%u\r", level);
            }
            break;

        case BATT_REPORTING_TYPE_APPLE:
            {
                level = (level + 9) / 10;

                if (level > 0)
                {
                    level--;
                }

                cmd_id = HFP_CMD_ID_BATT_IPHONEACCEV;
                /* Battery Level: string value between '0' and '9' */
                snprintf(buf, 26, "AT+IPHONEACCEV=2,1,%u,2,0\r", level);
            }
            break;

        case BATT_REPORTING_TYPE_ANDROID:
            {
                level = (level + 9) / 10;

                cmd_id = HFP_CMD_ID_BATT_XEVENT;
                /* AT+XEVENT=BATTERY,[Level],[NumberOfLevel],[MinutesOfTalk],[IsCharging] */
                snprintf(buf, 29, "AT+XEVENT=BATTERY,%u,10,0,0\r", level);
            }
            break;

        default:
            return false;
        }

        return bt_hfp_cmd_process(bd_addr, cmd_id, buf);
    }

    return false;
}

bool bt_hfp_connect_req(uint8_t bd_addr[6],
                        uint8_t server_chann,
                        bool    is_hfp)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_hfp_alloc_link(bd_addr);
        if (link != NULL)
        {
            uint8_t service_id;

            if (is_hfp)
            {
                service_id = bt_hfp->hfp_service_id;
            }
            else
            {
                service_id = bt_hfp->hsp_service_id;
            }

            if (rfc_conn_req(bd_addr,
                             server_chann,
                             service_id,
                             RFC_DEFAULT_MTU,
                             RFC_DEFAULT_CREDIT,
                             &link->dlci) == true)
            {
                link->hfp_state = HFP_STATE_CONNECTING;
                return true;
            }
            else
            {
                bt_hfp_free_link(link);
            }
        }
    }

    return false;
}

bool bt_hfp_disconnect_req(uint8_t bd_addr[6])
{

    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        bt_sco_disconn_req(bd_addr);
        return rfc_disconn_req(bd_addr, link->dlci);
    }

    return false;
}

bool bt_hfp_send_vnd_at_cmd_req(uint8_t     bd_addr[6],
                                const char *at_cmd)
{
    return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_VND_AT_CMD, at_cmd);
}

bool bt_hfp_speaker_gain_level_report(uint8_t bd_addr[6],
                                      uint8_t level)
{
    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL)
    {
        T_BT_HFP_LINK *link;

        link = bt_hfp_find_link_by_addr(bd_addr);
        if (link != NULL)
        {
            char buf[11];

            snprintf(buf, 11, "AT+VGS=%u\r", level);
            return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_INFORM_SPEAKER_GAIN, buf);
        }
    }

    return false;
}

bool bt_hfp_microphone_gain_level_report(uint8_t bd_addr[6],
                                         uint8_t level)
{
    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL)
    {
        T_BT_HFP_LINK *link;

        link = bt_hfp_find_link_by_addr(bd_addr);
        if (link != NULL)
        {
            char buf[11];

            snprintf(buf, 11, "AT+VGM=%u\r", level);
            return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_INFORM_MICROPHONE_GAIN, buf);
        }
    }

    return false;
}

bool bt_hfp_dial_with_number_req(uint8_t     bd_addr[6],
                                 const char *number)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            char buf[25];

            snprintf(buf, 25, "ATD%s;\r", number);
            return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_DIAL_WITH_NUMBER, buf);
        }
    }

    return false;
}

bool bt_hfp_hsp_button_press(uint8_t bd_addr[6])
{
    char buf[13];

    snprintf(buf, 13, "AT+CKPD=200\r");
    return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_HSP_BUTTON_PRESS, buf);
}

bool bt_hfp_dial_last_number_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            char buf[9];

            snprintf(buf, 9, "AT+BLDN\r");
            return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_DIAL_LAST_NUMBER, buf);
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            return bt_hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_call_answer_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);

        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_CALL_INCOMING)
            {
                char buf[5];

                snprintf(buf, 5, "ATA\r");
                return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_ACCEPT_CALL, buf);
            }
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            return bt_hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_call_terminate_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if ((link->curr_call_status == BT_HFP_CALL_INCOMING) ||
                (link->curr_call_status == BT_HFP_CALL_OUTGOING) ||
                (link->curr_call_status == BT_HFP_CALL_ACTIVE) ||
                (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING) ||
                (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD))
            {
                char buf[9];

                snprintf(buf, 9, "AT+CHUP\r");
                return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_REJECT_HANGUP_CALL, buf);
            }
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            return bt_hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_call_hold_action(uint8_t                   bd_addr[6],
                             T_BT_HFP_CALL_HOLD_ACTION control)
{
    uint8_t app_cmd_id = 0;
    char    buf[11];

    snprintf(buf, 11, "AT+CHLD=%u\r", (uint8_t)control);
    if (control == RELEASE_HELD_OR_WAITING_CALL)
    {
        app_cmd_id = HFP_CMD_ID_3WAY_CALL_CONTROL_0;
    }
    else if (control == RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL)
    {
        app_cmd_id = HFP_CMD_ID_3WAY_CALL_CONTROL_1;
    }
    else if (control == HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL)
    {
        app_cmd_id = HFP_CMD_ID_3WAY_CALL_CONTROL_2;
    }
    else if (control == JOIN_TWO_CALLS)
    {
        app_cmd_id = HFP_CMD_ID_3WAY_CALL_CONTROL_3;
    }

    return bt_hfp_cmd_process(bd_addr, app_cmd_id, buf);
}

bool bt_hfp_call_hold_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            return bt_hfp_call_hold_action(bd_addr, HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL);
        }
    }

    return false;
}

bool bt_hfp_held_call_accept_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            return bt_hfp_call_hold_action(bd_addr, HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL);
        }
    }

    return false;
}

bool bt_hfp_held_call_release_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            return bt_hfp_call_hold_action(bd_addr, RELEASE_HELD_OR_WAITING_CALL);
        }
    }

    return false;
}

bool bt_hfp_send_clcc(T_BT_HFP_LINK *link)
{
    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_ENHANCED_CALL_STATUS)
    {
        char buf[9];

        snprintf(buf, 9, "AT+CLCC\r");
        return bt_hfp_cmd_process(link->bd_addr, HFP_CMD_ID_SEND_CLCC, buf);
    }

    return false;
}

bool bt_hfp_current_call_list_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            return bt_hfp_send_clcc(link);
        }
    }

    return false;
}

bool bt_hfp_release_held_or_waiting_call_req(uint8_t bd_addr[6])
{
    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_CALL_WAITING_OR_3WAY)
    {
        T_BT_HFP_LINK *link;

        link = bt_hfp_find_link_by_addr(bd_addr);
        if (link != NULL)
        {
            if (link->hfp_state == HFP_STATE_CONNECTED)
            {
                if ((link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD) ||
                    (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
                {
                    return bt_hfp_call_hold_action(bd_addr, RELEASE_HELD_OR_WAITING_CALL);
                }
            }
        }
    }

    return false;
}

bool bt_hfp_release_active_call_accept_held_or_waiting_call_req(uint8_t bd_addr[6])
{
    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_CALL_WAITING_OR_3WAY)
    {
        T_BT_HFP_LINK *link;

        link = bt_hfp_find_link_by_addr(bd_addr);
        if (link != NULL)
        {
            if (link->hfp_state == HFP_STATE_CONNECTED)
            {
                if ((link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD) ||
                    (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
                {
                    return bt_hfp_call_hold_action(bd_addr, RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL);
                }
            }
        }
    }

    return false;
}

bool bt_hfp_hold_active_call_accept_held_or_waiting_call_req(uint8_t bd_addr[6])
{
    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_CALL_WAITING_OR_3WAY)
    {
        T_BT_HFP_LINK *link;

        link = bt_hfp_find_link_by_addr(bd_addr);
        if (link != NULL)
        {
            if ((link->hfp_state == HFP_STATE_CONNECTED) &&
                (link->cpbs & AG_CAPABILITY_3WAY))
            {
                if ((link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD) ||
                    (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING))
                {
                    return bt_hfp_call_hold_action(bd_addr, HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL);
                }
            }
        }
    }

    return false;
}

bool bt_hfp_join_two_calls_req(uint8_t bd_addr[6])
{
    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_CALL_WAITING_OR_3WAY)
    {
        T_BT_HFP_LINK *link;

        link = bt_hfp_find_link_by_addr(bd_addr);
        if (link != NULL)
        {
            if (link->hfp_state == HFP_STATE_CONNECTED)
            {
                if (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD)
                {
                    return bt_hfp_call_hold_action(bd_addr, JOIN_TWO_CALLS);
                }
            }
        }
    }

    return false;
}

bool bt_hfp_connect_cfm(uint8_t bd_addr[6],
                        bool    accept)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint16_t status;

        status = accept ? RFC_ACCEPT : RFC_REJECT;
        rfc_conn_cfm(bd_addr, link->dlci, status, link->frame_size, RFC_DEFAULT_CREDIT);
        if (accept == false)
        {
            bt_hfp_free_link(link);
        }

        return true;
    }

    return false;
}

bool bt_hfp_audio_connect_req(uint8_t bd_addr[6])
{
    T_BT_LINK *acl_link;

    acl_link = bt_link_find(bd_addr);
    if (acl_link != NULL)
    {
        if (acl_link->sco_state == BT_LINK_SCO_STATE_DISCONNECTED)
        {
            T_BT_HFP_LINK *link;

            link = bt_hfp_find_link_by_addr(bd_addr);
            if (link != NULL)
            {
                if (link->hfp_state == HFP_STATE_CONNECTED)
                {
                    bt_sniff_mode_exit(bd_addr, false);

                    if ((link->cpbs & AG_CAPABILITY_CODEC_NEGOTIATION) &&
                        (bt_hfp->brsf_cpbs & HF_CAPABILITY_CODEC_NEGOTIATION))
                    {
                        char buf[8];

                        snprintf(buf, 8, "AT+BCC\r");
                        return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_ESTABLISH_VOICE, buf);
                    }
                    else
                    {
                        if ((link->cpbs & AG_CAPABILITY_ESCO_S4_T2_SUPPORTED) &&
                            (bt_hfp->brsf_cpbs & HF_CAPABILITY_ESCO_S4_T2_SUPPORTED))
                        {
                            gap_br_send_sco_conn_req(bd_addr, 8000, 8000, 12, 0x0360, 2, SCO_PKT_TYPES_S4);
                        }
                        else
                        {
                            gap_br_send_sco_conn_req(bd_addr, 8000, 8000, 10, 0x0360, 1, SCO_PKT_TYPES_S3);
                        }
                    }

                    return true;
                }
                else if (link->hfp_state == HSP_STATE_CONNECTED)
                {
                    return bt_hfp_hsp_button_press(bd_addr);
                }
            }
        }
    }

    return false;
}

bool bt_hfp_audio_connect_cfm(uint8_t bd_addr[6],
                              bool    accept)
{
    T_BT_LINK *acl_link;

    acl_link = bt_link_find(bd_addr);
    if (acl_link != NULL)
    {
        if (acl_link->sco_state == BT_LINK_SCO_STATE_CONNECTING)
        {
            if (accept == true)
            {
                T_BT_HFP_LINK *link;

                link = bt_hfp_find_link_by_addr(bd_addr);
                if (link != NULL)
                {
                    if ((link->codec_type == CODEC_TYPE_MSBC) ||
                        (link->codec_type == CODEC_TYPE_LC3))
                    {
                        /*
                        * eSCO parameter set to "T2"
                        * voice setting |= 0x0300 mean bypass MAC HW audio decoder/encoder
                        */
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
                        if (acl_link->is_esco == 0)
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
                            /*
                            * eSCO parameter set to "S4", iop: S4 must be used in
                            * mac cvsd whether supported or not
                            */
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
    }

    return false;
}

bool bt_hfp_audio_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            return bt_sco_disconn_req(bd_addr);
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            return bt_hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_voice_recognition_enable_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if ((link->cpbs & AG_CAPABILITY_VOICE_RECOGNITION) &&
                (bt_hfp->brsf_cpbs & HF_CAPABILITY_VOICE_RECOGNITION))
            {
                char buf[11];

                snprintf(buf, 11, "AT+BVRA=1\r");
                return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE, buf);
            }
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            return bt_hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_voice_recognition_disable_req(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if ((link->cpbs & AG_CAPABILITY_VOICE_RECOGNITION) &&
                (bt_hfp->brsf_cpbs & HF_CAPABILITY_VOICE_RECOGNITION))
            {
                char buf[11];

                snprintf(buf, 11, "AT+BVRA=0\r");
                return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE, buf);
            }
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            return bt_hfp_hsp_button_press(bd_addr);
        }
    }

    return false;
}

bool bt_hfp_dtmf_code_transmit(uint8_t bd_addr[6],
                               char    code)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_CALL_ACTIVE ||
                link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING ||
                link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD)
            {
                char buf[10];

                snprintf(buf, 10, "AT+VTS=%c\r", code);
                return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_SEND_DTMF, buf);
            }
        }
    }

    return false;
}

bool bt_hfp_network_operator_name_query(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            char buf[10];

            snprintf(buf, 10, "AT+COPS?\r");
            return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_QUERY_NETWORK_NAME, buf);
        }
    }

    return false;
}

bool bt_hfp_subscriber_number_query(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            char buf[9];

            snprintf(buf, 9, "AT+CNUM\r");
            return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_QUERY_SUBSCRIBER_NUM, buf);
        }
    }

    return false;
}

bool bt_hfp_indicator_activate(uint8_t  bd_addr[6],
                               uint32_t indicator_types)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            char    buf[25];
            uint8_t index;
            uint8_t control[7] = {0};

            for (index = 0; index < link->indicator_cnt; index++)
            {
                if (indicator_types & (0x01 << link->indicator_type[index]))
                {
                    control[index] = 1;
                }
            }

            snprintf(buf, 25, "AT+BIA=%u,%u,%u,%u,%u,%u,%u\r",
                     control[0],
                     control[1],
                     control[2],
                     control[3],
                     control[4],
                     control[5],
                     control[6]);

            return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_INDICATOR_ACTIVATE, buf);
        }
    }

    return false;
}

bool bt_hfp_indicator_deactivate(uint8_t  bd_addr[6],
                                 uint32_t indicator_types)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            char    buf[25];
            uint8_t index;
            uint8_t control[7] = {1};

            for (index = 0; index < link->indicator_cnt; index++)
            {
                if (indicator_types & (0x01 << link->indicator_type[index]))
                {
                    control[index] = 0;
                }
            }

            snprintf(buf, 25, "AT+BIA=%u,%u,%u,%u,%u,%u,%u\r",
                     control[0],
                     control[1],
                     control[2],
                     control[3],
                     control[4],
                     control[5],
                     control[6]);

            return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_INDICATOR_DEACTIVATE, buf);
        }
    }

    return false;
}

bool bt_hfp_nrec_disable(uint8_t bd_addr[6])
{
    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_EC_NR)
    {
        T_BT_HFP_LINK *link;

        link = bt_hfp_find_link_by_addr(bd_addr);
        if (link != NULL)
        {
            if (link->hfp_state == HFP_STATE_CONNECTED)
            {
                char buf[11];

                snprintf(buf, 11, "AT+NREC=0\r");
                return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_NREC, buf);
            }
        }
    }

    return false;
}

void bt_hfp_bac_cmd_assemble(char *buf)
{
    uint8_t index = 8;
    uint8_t i;

    snprintf(buf, 9, "AT+BAC=1");

    for (i = 1; i < 8; i++)
    {
        if ((bt_hfp->supt_codecs >> i) & 0x01)
        {
            snprintf(buf + index, 3, ",%u", (uint8_t)(i + 1));
            index += 2;
        }
    }

    snprintf(buf + index, 2, "\r");
}

bool bt_hfp_send_cmee_enable(T_BT_HFP_LINK *link)
{
    if (link->cpbs & AG_CAPABILITY_EXTENED_ERROR_RESULT)
    {
        char buf[11];

        snprintf(buf, 11, "AT+CMEE=1\r");
        return bt_hfp_cmd_process(link->bd_addr, HFP_CMD_ID_CMEE, buf);
    }

    return false;
}

bool bt_hfp_send_ccwa(T_BT_HFP_LINK *link)
{
    char buf[11];

    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_CALL_WAITING_OR_3WAY)
    {
        snprintf(buf, 11, "AT+CCWA=1\r");
        return bt_hfp_cmd_process(link->bd_addr, HFP_CMD_ID_CCWA, buf);
    }

    return false;
}

bool bt_hfp_send_clip(T_BT_HFP_LINK *link)
{
    if (bt_hfp->brsf_cpbs & HF_CAPABILITY_CLI)
    {
        char buf[11];

        snprintf(buf, 11, "AT+CLIP=1\r");
        return bt_hfp_cmd_process(link->bd_addr, HFP_CMD_ID_CLIP, buf);
    }

    return false;
}

bool bt_hfp_network_operator_format_set(uint8_t bd_addr[6])
{
    char buf[13];

    snprintf(buf, 13, "AT+COPS=3,0\r");
    return bt_hfp_cmd_process(bd_addr, HFP_CMD_ID_SET_NETWORK_OPERATOR_FORMAT, buf);
}

uint8_t bt_hfp_get_indicator_type_by_name(char *name)
{
    for (int i = 0; i < sizeof(INDICATOR_NAME_TABLE) / sizeof(INDICATOR_NAME_TABLE[0]); i++)
    {
        if (!strcmp(INDICATOR_NAME_TABLE[i].indicator_name, name))
        {
            return INDICATOR_NAME_TABLE[i].indicator_type;
        }
    }

    return AG_INDICATOR_TYPE_UNKOWN;
}

void bt_hfp_update_ag_indicator_status(T_BT_HFP_LINK *link,
                                       uint8_t        index,
                                       uint8_t        status)
{
    index--; /* begin with 0 */
    if (index < link->indicator_cnt)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);

        switch (link->indicator_type[index])
        {
        case AG_INDICATOR_TYPE_UNKOWN:
            break;

        case AG_INDICATOR_TYPE_SERVICE:
            link->ag_status_ind.service_status = (T_HFP_SERVICE_STATUS)status;
            payload.msg_buf = &link->ag_status_ind.service_status;
            bt_mgr_dispatch(BT_MSG_HFP_SERVICE_STATUS_CHANGED, &payload);
            break;

        case AG_INDICATOR_TYPE_CALL:
            link->ag_status_ind.call_status = (T_HFP_CALL_STATUS)status;

            if (link->hfp_state != HFP_STATE_CONNECTED)
            {
                link->curr_call_status = BT_HFP_CALL_IDLE;
                if (status == HFP_CALL_STATUS_ACTIVE)
                {
                    link->curr_call_status = BT_HFP_CALL_ACTIVE;
                }
            }
            else
            {
                if (status == HFP_CALL_STATUS_IDLE)
                {
                    if (link->curr_call_status == BT_HFP_CALL_ACTIVE)
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_IDLE);
                    }
                    else if (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD)
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_ACTIVE);
                    }
                    else if (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING)
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_INCOMING);
                    }
                }
                else if (status == HFP_CALL_STATUS_ACTIVE)
                {
                    if (link->curr_call_status != BT_HFP_CALL_ACTIVE)
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_ACTIVE);
                    }
                }
            }
            break;

        case AG_INDICATOR_TYPE_CALLSETUP:
            link->ag_status_ind.call_setup_status = (T_HFP_CALL_SETUP_STATUS)status;

            if (link->hfp_state != HFP_STATE_CONNECTED)
            {
                if (status == HFP_CALL_SETUP_STATUS_INCOMING_CALL)
                {
                    if (link->curr_call_status == BT_HFP_CALL_ACTIVE)
                    {
                        link->curr_call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING;
                    }
                    else
                    {
                        link->curr_call_status = BT_HFP_CALL_INCOMING;
                    }
                }
                else if ((status == HFP_CALL_SETUP_STATUS_OUTGOING_CALL) ||
                         (status == HFP_CALL_SETUP_STATUS_ALERTING))
                {
                    if (link->curr_call_status == BT_HFP_CALL_IDLE)
                    {
                        link->curr_call_status = BT_HFP_CALL_OUTGOING;
                    }
                }
            }
            else
            {
                if (status == HFP_CALL_SETUP_STATUS_IDLE)
                {
                    if ((link->curr_call_status == BT_HFP_CALL_INCOMING) ||
                        (link->curr_call_status == BT_HFP_CALL_OUTGOING))
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_IDLE);
                    }
                    else if (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING)
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_ACTIVE);
                    }
                }
                else if (status == HFP_CALL_SETUP_STATUS_INCOMING_CALL)
                {
                    if (link->curr_call_status == BT_HFP_CALL_IDLE)
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_INCOMING);
                    }
                    else if (link->curr_call_status == BT_HFP_CALL_ACTIVE)
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING);
                    }
                }
                else if ((status == HFP_CALL_SETUP_STATUS_OUTGOING_CALL) ||
                         (status == HFP_CALL_SETUP_STATUS_ALERTING))
                {
                    if (link->curr_call_status == BT_HFP_CALL_IDLE)
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_OUTGOING);
                    }
                }
            }
            break;

        case AG_INDICATOR_TYPE_CALLHELD:
            link->ag_status_ind.call_held_status = (T_HFP_CALL_HELD_STATUS)status;

            if (link->hfp_state != HFP_STATE_CONNECTED) /* service level connecting */
            {
                if (status == HFP_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL)
                {
                    link->curr_call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_HELD;
                }
                else if (status == HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL)
                {
                    link->curr_call_status = BT_HFP_CALL_HELD;
                }
            }
            else /* after finish service level */
            {
                if (status == HFP_CALL_HELD_STATUS_IDLE)
                {
                    if (link->curr_call_status == BT_HFP_CALL_ACTIVE_WITH_CALL_HELD)
                    {
                        bt_hfp_call_status_set(link, BT_HFP_CALL_ACTIVE);
                    }
                    else if (link->curr_call_status == BT_HFP_CALL_HELD)
                    {
                        /*
                         * When received this event, the call status may be call idle or call active.
                         * So do not change call status directly, use clcc to query.
                         */
                    }

                    bt_hfp_send_clcc(link);
                }
                else if (status == HFP_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL)
                {
                    bt_hfp_call_status_set(link, BT_HFP_CALL_ACTIVE_WITH_CALL_HELD);
                }
                else if (status == HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL)
                {
                    bt_hfp_call_status_set(link, BT_HFP_CALL_HELD);
                }
            }
            break;

        case AG_INDICATOR_TYPE_SIGNAL:
            {
                link->ag_status_ind.signal_status = status;
                payload.msg_buf = &link->ag_status_ind.signal_status;
                bt_mgr_dispatch(BT_MSG_HFP_SIGNAL_IND, &payload);
            }
            break;

        case AG_INDICATOR_TYPE_ROAM:
            {
                link->ag_status_ind.roam_status = (T_HFP_ROAM_STATUS)status;
                payload.msg_buf = &link->ag_status_ind.roam_status;
                bt_mgr_dispatch(BT_MSG_HFP_ROAM_IND, &payload);
            }
            break;

        case AG_INDICATOR_TYPE_BATTCHG:
            {
                link->ag_status_ind.batt_chg_status = status;
                payload.msg_buf = &link->ag_status_ind.batt_chg_status;
                bt_mgr_dispatch(BT_MSG_HFP_BATTERY_IND, &payload);
            }
            break;

        default:
            break;
        }
    }
}

void bt_hfp_srv_level_established(T_BT_HFP_LINK *link)
{
    T_BT_MSG_PAYLOAD          payload;
    T_BT_HFP_CALL_STATUS_INFO info;

    link->credits = 1;
    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;

    if (link->uuid == UUID_HEADSET)
    {
        link->hfp_state = HSP_STATE_CONNECTED;
        bt_pm_cback_register(link->bd_addr, bt_hfp_pm_cback);
        bt_mgr_dispatch(BT_MSG_HSP_CONN_CMPL, &payload);
    }
    else
    {
        link->hfp_state = HFP_STATE_CONNECTED;

        if (link->cpbs & AG_CAPABILITY_INBAND_RINGING)
        {
            link->supported_features |= HFP_INBAND_RINGTONE_ENABLE;
        }

        if ((link->cpbs & AG_CAPABILITY_ESCO_S4_T2_SUPPORTED) &&
            (bt_hfp->brsf_cpbs & HF_CAPABILITY_ESCO_S4_T2_SUPPORTED))
        {
            link->supported_features |= HFP_ESCO_S4_ENABLE;
        }

        bt_hfp_send_cmee_enable(link);
        bt_hfp_send_ccwa(link);
        bt_hfp_send_clip(link);
        /* Some AGs will not send CIND call status if call already exit before HFP SLC finish */
        bt_hfp_send_clcc(link);
        bt_hfp_network_operator_format_set(link->bd_addr);

        bt_pm_cback_register(link->bd_addr, bt_hfp_pm_cback);
        bt_mgr_dispatch(BT_MSG_HFP_CONN_CMPL, &payload);

        payload.msg_buf = &link->cpbs;
        bt_mgr_dispatch(BT_MSG_HFP_SUPPORTED_FEATURES_IND, &payload);
    }

    if (link->curr_call_status != BT_HFP_CALL_IDLE)
    {
        info.curr_call_status = link->curr_call_status;
        info.prev_call_status = link->prev_call_status;
        payload.msg_buf = &info;
        bt_mgr_dispatch(BT_MSG_HFP_CALL_STATUS_CHANGED, &payload);
    }

    payload.msg_buf = &link->ag_status_ind.service_status;
    bt_mgr_dispatch(BT_MSG_HFP_SERVICE_STATUS_CHANGED, &payload);
}

bool bt_hfp_srv_level_step(T_BT_HFP_LINK *link,
                           bool           result_ok)
{
    char buf[47];

    while (1)
    {
        link->service_level_step++;
        switch (link->service_level_step)
        {
        case HFP_SRV_LEVEL_BRSF: /* mandatory */
            {
                snprintf(buf, 15, "AT+BRSF=%u\r", bt_hfp->brsf_cpbs);
                return bt_hfp_cmd_send(link, buf, strlen(buf));
            }

        case HFP_SRV_LEVEL_BAC: /* mandatory */
            if ((link->cpbs & AG_CAPABILITY_CODEC_NEGOTIATION) &&
                (bt_hfp->brsf_cpbs & HF_CAPABILITY_CODEC_NEGOTIATION))
            {
                bt_hfp_bac_cmd_assemble(buf);
                return bt_hfp_cmd_send(link, buf, strlen(buf));
            }
            else
            {
                continue;
            }

        case HFP_SRV_LEVEL_CIND_TEST: /* mandatory */
            {
                snprintf(buf, 11, "AT+CIND=?\r");
                return bt_hfp_cmd_send(link, buf, strlen(buf));
            }

        case HFP_SRV_LEVEL_CIND_INQUIRY: /* mandatory */
            /* The AT+CIND? read command is used to get current status of the AG indicators. */
            {
                snprintf(buf, 10, "AT+CIND?\r");
                return bt_hfp_cmd_send(link, buf, strlen(buf));
            }

        case HFP_SRV_LEVEL_ACTIVATE_INDICATOR: /* mandatory */
            /*
             * AT+CMER=3,0,0,1 activates indicator events reporting.
             * AT+CMER=3,0,0,0 deactivates indicator events reporting.
             */
            {
                snprintf(buf, 17, "AT+CMER=3,0,0,1\r");
                return bt_hfp_cmd_send(link, buf, strlen(buf));
            }

        case HFP_SRV_LEVEL_CHLD_TEST: /* mandatory, after this ServiceLevelConnection is Established */
            if ((link->cpbs & AG_CAPABILITY_3WAY) &&
                (bt_hfp->brsf_cpbs & HF_CAPABILITY_CALL_WAITING_OR_3WAY))
            {
                /* get call hold capabilities */
                snprintf(buf, 11, "AT+CHLD=?\r");
                return bt_hfp_cmd_send(link, buf, strlen(buf));
            }
            else
            {
                continue;
            }

        case HFP_SRV_LEVEL_BIND:
            if ((link->cpbs & AG_CAPABILITY_HF_INDICATORS) &&
                (bt_hfp->brsf_cpbs & HF_CAPABILITY_HF_INDICATORS))
            {
                snprintf(buf, 11, "AT+BIND=2\r");
                return bt_hfp_cmd_send(link, buf, strlen(buf));
            }
            else
            {
                link->service_level_step += 2;
                continue;
            }

        case HFP_SRV_LEVEL_BIND_TEST:
            if (result_ok == true)
            {
                snprintf(buf, 11, "AT+BIND=?\r");
                return bt_hfp_cmd_send(link, buf, strlen(buf));
            }
            else
            {
                link->service_level_step++;
                continue;
            }

        case HFP_SRV_LEVEL_BIND_INQUIRY:
            {
                snprintf(buf, 10, "AT+BIND?\r");
                return bt_hfp_cmd_send(link, buf, strlen(buf));
            }

        default:
            return false;
        }
    }
}

void bt_hfp_srv_level_continue(T_BT_HFP_LINK *link,
                               bool           result_ok)
{
    if (link->uuid == UUID_HANDSFREE)
    {
        if (false == bt_hfp_srv_level_step(link, result_ok))
        {
            bt_hfp_srv_level_established(link);
        }
    }
    else
    {
        bt_hfp_srv_level_established(link);
    }
}

void bt_hfp_srv_level_start(T_BT_HFP_LINK *link)
{
    link->service_level_step = HFP_SRV_LEVEL_NONE;
    bt_hfp_srv_level_continue(link, true);
}

void bt_hfp_handle_audio_conn_cmpl(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, bd_addr, 6);
        payload.msg_buf = &link->codec_type;
        bt_mgr_dispatch(BT_MSG_HFP_CODEC_TYPE_SELECTED, &payload);

        if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            bt_hfp_call_status_set(link, BT_HFP_CALL_ACTIVE);
        }
    }
}

void bt_hfp_handle_audio_disconn(uint8_t bd_addr[6])
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            bt_hfp_call_status_set(link, BT_HFP_CALL_IDLE);
        }
    }
}

void bt_hfp_handle_rfc_conn_cmpl(T_RFC_CONN_CMPL *cmpl_msg)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(cmpl_msg->bd_addr);
    if (link != NULL)
    {
        if (cmpl_msg->service_id == bt_hfp->hfp_service_id)
        {
            link->uuid = UUID_HANDSFREE;
        }
        else if (cmpl_msg->service_id == bt_hfp->hsp_service_id)
        {
            link->uuid = UUID_HEADSET;
        }
        else
        {
            return;
        }

        PROFILE_PRINT_TRACE4("bt_hfp_handle_rfc_conn_cmpl: bd_addr %s, dlci %d, tx_credits %d, uuid 0x%04x",
                             TRACE_BDADDR(cmpl_msg->bd_addr), cmpl_msg->dlci, cmpl_msg->remain_credits, link->uuid);

        link->frame_size = cmpl_msg->frame_size;
        link->codec_type = CODEC_TYPE_CVSD;

        /* when connected credits may = 0, wait till credits > 0 */
        if (cmpl_msg->remain_credits == 0)
        {
            link->hfp_state = HFP_STATE_RFC_CONN_NO_CREDITS;
        }
        else
        {
            link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
            bt_hfp_srv_level_start(link);
        }
    }
    else
    {
        rfc_disconn_req(cmpl_msg->bd_addr, cmpl_msg->dlci);
    }
}

void bt_hfp_handle_ok(T_BT_HFP_LINK *link,
                      uint8_t        cmd_id)
{
    T_BT_MSG_PAYLOAD payload;
    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_OK;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &cmd_rsp;

    switch (cmd_id)
    {
    case HFP_CMD_ID_SEND_CLCC:
        {
            T_BT_HFP_CALL_STATUS call_status = link->curr_call_status;

            if (link->clcc_ind & CLCC_IND_CALL_HELD)
            {
                call_status = BT_HFP_CALL_HELD;

                if (link->clcc_ind & CLCC_IND_CALL_ACTIVE)
                {
                    call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_HELD;
                }
            }
            else if (link->clcc_ind & CLCC_IND_CALL_ACTIVE)
            {
                call_status = BT_HFP_CALL_ACTIVE;

                if (link->clcc_ind & CLCC_IND_CALL_HELD)
                {
                    call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_HELD;
                }
                if (link->clcc_ind & CLCC_IND_CALL_WAITING)
                {
                    call_status = BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING;
                }
            }
            else if (link->clcc_ind & (CLCC_IND_CALL_INCOMING | CLCC_IND_CALL_WAITING))
            {
                call_status = BT_HFP_CALL_INCOMING;
            }
            else if (link->clcc_ind & (CLCC_IND_CALL_DIALING | CLCC_IND_CALL_DIAL_ALERT))
            {
                call_status = BT_HFP_CALL_OUTGOING;
            }
            else if (link->clcc_ind == CLCC_IND_CALL_IDLE)
            {
                call_status = BT_HFP_CALL_IDLE;
            }

            link->clcc_ind = CLCC_IND_CALL_IDLE;
            bt_hfp_call_status_set(link, call_status);
        }
        break;

    case HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE:
        {
            bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION, &payload);
        }
        break;

    case HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE:
        {
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
            bt_mgr_dispatch(BT_MSG_HFP_DIAL_WITH_NUMBER_RESULT, &payload);
        }
        break;

    case HFP_CMD_ID_BATT_XAPL:
        {
            link->batt_report_type = BATT_REPORTING_TYPE_APPLE;
            bt_hfp_batt_level_report(link->bd_addr, link->batt_level);
        }
        break;

    case HFP_CMD_ID_SEND_DTMF:
    case HFP_CMD_ID_SET_NETWORK_OPERATOR_FORMAT:
    case HFP_CMD_ID_QUERY_NETWORK_NAME:
    case HFP_CMD_ID_QUERY_SUBSCRIBER_NUM:
        break;

    case HFP_CMD_ID_VND_AT_CMD:
        {
            bt_mgr_dispatch(BT_MSG_HFP_VND_AT_CMD_RSP, &payload);
        }
        break;

    default:
        break;
    }
}

void bt_hfp_handle_error(T_BT_HFP_LINK *link,
                         uint8_t        cmd_id)
{
    T_BT_MSG_PAYLOAD payload;
    T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_ERROR;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &cmd_rsp;

    switch (cmd_id)
    {
    case HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE:
        {
            bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION, &payload);
        }
        break;

    case HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE:
        {
            bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION, &payload);
        }
        break;

    case HFP_CMD_ID_DIAL_WITH_NUMBER:
        {
            bt_mgr_dispatch(BT_MSG_HFP_DIAL_WITH_NUMBER_RESULT, &payload);
        }
        break;

    case HFP_CMD_ID_DIAL_LAST_NUMBER:
        break;

    case HFP_CMD_ID_REJECT_HANGUP_CALL:
        break;

    case HFP_CMD_ID_BATT_XAPL:
        {
            link->batt_report_type = BATT_REPORTING_TYPE_ANDROID;
            bt_hfp_batt_level_report(link->bd_addr, link->batt_level);
        }
        break;

    case HFP_CMD_ID_VND_AT_CMD:
        {
            bt_mgr_dispatch(BT_MSG_HFP_VND_AT_CMD_RSP, &payload);
        }
        break;

    default:
        break;
    }
}

int32_t bt_hfp_handle_cme_error(T_BT_HFP_LINK *link,
                                const char    *str,
                                uint8_t        cmd_id)
{
    uint8_t error_number;
    int     res;
    int     offset;
    int32_t ret = 0;

    res = sscanf(str, "+CME ERROR: %hhu%n", &error_number, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    bt_hfp_handle_error(link, cmd_id);

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_brsf(T_BT_HFP_LINK *link,
                           const char    *str)
{
    int     res;
    int     offset;
    int32_t ret = 0;

    res = sscanf(str, "+BRSF: %hu%n", &link->cpbs, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_cind(T_BT_HFP_LINK *link,
                           const char    *str)
{
    int     res;
    int     offset;
    int32_t ret = 0;

    str += 6;
    if (*str == ' ')
    {
        str++;
    }

    if (*str == '(')
    {
        char name[20];
        uint8_t max;

        while ((res = sscanf(str, "(\"%19[^\"]\",(0%*[-,]%hhu))%n", name, &max, &offset)) == 2)
        {
            link->indicator_type[link->indicator_cnt]  = bt_hfp_get_indicator_type_by_name(name);
            link->indicator_range[link->indicator_cnt] = max;

            str += offset;
            link->indicator_cnt++;

            if (*str != ',')
            {
                break;
            }

            str++;
        }

        if (res < 2)
        {
            ret = 1;
            goto fail_cind_list_value;
        }

        if (*str != '\0')
        {
            ret = 2;
            goto fail_cind_list_end_part;
        }
    }
    else
    {
        uint8_t index = 1;
        uint8_t value = 0;

        while ((res = sscanf(str, "%hhu%n", &value, &offset)) == 1)
        {
            bt_hfp_update_ag_indicator_status(link, index, value);

            str += offset;

            /* check if more values are present */
            if (*str != ',')
            {
                break;
            }

            index++;
            str++;
        }

        if (res < 1)
        {
            ret = 3;
            goto fail_cind_item_value;
        }

        if (*str != '\0')
        {
            ret = 4;
            goto fail_cind_item_end_part;
        }
    }

    return 0;

fail_cind_item_end_part:
fail_cind_item_value:
fail_cind_list_end_part:
fail_cind_list_value:
    return ret;
}

int32_t bt_hfp_handle_clcc(T_BT_HFP_LINK *link,
                           const char    *str)
{
    /*
     * <idx>,<dir>,<status>,<mode>,<mprty>[,<number>,<type>]
     * "+CLCC: 1,0,3,0,0,"10010",129"
     */
    T_BT_HFP_CLCC_IND msg;
    T_BT_MSG_PAYLOAD  payload;
    int               res;
    int               offset;
    int32_t           ret = 0;

    msg.type = 128;
    memset(msg.number, 0, sizeof(msg.number));
    memcpy(msg.bd_addr, link->bd_addr, 6);

    res = sscanf(str, "+CLCC: %hhu,%hhu,%hhu,%hhu,%hhu%n", &msg.call_idx, &msg.dir_incoming,
                 &msg.status, &msg.mode, &msg.mpty, &offset);
    if (res < 5)
    {
        ret = 1;
        goto fail_mandatory_part;
    }

    str += offset;
    offset = 0;

    if (*str == ',')
    {
        /* check optional part */
        res = sscanf(str, ",\"%20[^\"]\",%hhu%n", msg.number, &msg.type, &offset);
        if (res == 0)
        {
            sscanf(str, ",\"\",%hhu%n", &msg.type, &offset);
        }
    }

    link->clcc_ind |= (1 << msg.status);
    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &msg;
    bt_mgr_dispatch(BT_MSG_HFP_CURRENT_CALL_LIST_IND, &payload);

    return 0;

fail_mandatory_part:
    return ret;
}

void bt_hfp_handle_ring(T_BT_HFP_LINK *link)
{
    T_BT_MSG_PAYLOAD payload;
    bool             is_inband;

    if (link->supported_features & HFP_INBAND_RINGTONE_ENABLE)
    {
        is_inband = true;
    }
    else
    {
        is_inband = false;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &is_inband;
    bt_mgr_dispatch(BT_MSG_HFP_RING_ALERT, &payload);
    if (link->hfp_state == HFP_STATE_CONNECTED)
    {
        if (link->curr_call_status == BT_HFP_CALL_IDLE)
        {
            bt_hfp_call_status_set(link, BT_HFP_CALL_INCOMING);
        }
    }
    else if (link->hfp_state == HSP_STATE_CONNECTED)
    {
        bt_hfp_call_status_set(link, BT_HFP_CALL_INCOMING);
    }
}

int32_t bt_hfp_handle_bvra(T_BT_HFP_LINK *link,
                           const char    *str)
{
    T_BT_HFP_CMD_RSP cmd_rsp;
    T_BT_MSG_PAYLOAD payload;
    uint8_t          status;
    int              res;
    int              offset;
    int32_t          ret = 0;

    res = sscanf(str, "+BVRA: %hhu%n", &status, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    cmd_rsp = HFP_CMD_OK;
    payload.msg_buf = &cmd_rsp;
    if (status == HFP_VOICE_RECOGNITION_IDLE)
    {
        bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION, &payload);
    }
    else if (status == HFP_VOICE_RECOGNITION_ACTIVE)
    {
        bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION, &payload);
    }

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_clip(T_BT_HFP_LINK *link,
                           const char    *str)
{
    T_BT_HFP_CALLER_ID_IND call_clip;;
    int                    res;
    int                    offset;
    int32_t                ret = 0;

    memset(call_clip.number, 0, sizeof(call_clip.number));

    res = sscanf(str, "+CLIP: \"%20[^\"]\",%hhu%n", call_clip.number, &call_clip.type, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    if ((*(str + offset) != ',') &&
        (*(str + offset) != '\0'))
    {
        ret = 2;
        goto fail_end_part;
    }

    if (link->curr_call_status == BT_HFP_CALL_INCOMING)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &call_clip;
        bt_mgr_dispatch(BT_MSG_HFP_CALLER_ID_IND, &payload);
    }

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_ciev(T_BT_HFP_LINK *link,
                           const char    *str)
{
    int     res;
    int     offset;
    uint8_t index;
    uint8_t value;
    int32_t ret = 0;

    res = sscanf(str, "+CIEV: %hhu,%hhu%n", &index, &value, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    bt_hfp_update_ag_indicator_status(link, index, value);

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_ccwa(T_BT_HFP_LINK *link,
                           const char    *str)
{
    T_BT_HFP_CALL_WAITING_IND call_waiting;
    int                       res;
    int                       offset;
    int32_t                   ret = 0;

    memset(call_waiting.number, 0, sizeof(call_waiting.number));

    /* +CCWA: <number>,<type>,<class>,[<alpha>][,<CLI validity>]
     * In the +CCWA result code only <number> and <type> parameters are covered in HFP specification.
     * Other parameters are not considered relevant in HFP and shall be ignored by the HF.
     */

    res = sscanf(str, "+CCWA: \"%20[^\"]\",%hhu%n", call_waiting.number, &call_waiting.type, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    /*
     * AG shall send +CCWA unsolicited result code to the HF whenever
     * an incoming call is waiting during an ongoing call.
     */
    if (link->curr_call_status == BT_HFP_CALL_ACTIVE)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &call_waiting;
        bt_mgr_dispatch(BT_MSG_HFP_CALL_WAITING_IND, &payload);

        bt_hfp_call_status_set(link, BT_HFP_CALL_ACTIVE_WITH_CALL_WAITING);
    }

    return 0;

fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_vgm(T_BT_HFP_LINK *link,
                          const char    *str)
{
    T_BT_MSG_PAYLOAD payload;
    uint8_t          volume;
    int              res;
    int              offset;
    int32_t          ret = 0;

    res = sscanf(str, "+VGM%*[:=]%hhu%n", &volume, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if ((bt_hfp->brsf_cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0)
    {
        ret = 3;
        goto fail_support_event;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &volume;
    bt_mgr_dispatch(BT_MSG_HFP_MIC_GAIN_CHANGED, &payload);

    return 0;

fail_support_event:
fail_end_part:
fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_vgs(T_BT_HFP_LINK *link,
                          const char    *str)
{
    T_BT_MSG_PAYLOAD payload;
    uint8_t          volume;
    int              res;
    int              offset;
    int32_t          ret = 0;

    res = sscanf(str, "+VGS%*[:=]%hhu%n", &volume, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if ((bt_hfp->brsf_cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0)
    {
        ret = 3;
        goto fail_support_event;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &volume;
    bt_mgr_dispatch(BT_MSG_HFP_SPK_GAIN_CHANGED, &payload);

    return 0;

fail_support_event:
fail_end_part:
fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_bcs(T_BT_HFP_LINK *link,
                          const char    *str)
{
    char    buf[47];
    int     res;
    int     offset;
    uint8_t codec_type;
    int32_t ret = 0;

    res = sscanf(str, "+BCS: %hhu%n", &codec_type, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if ((bt_hfp->supt_codecs >> (codec_type - 1)) & 0x01)
    {
        link->codec_type = (T_HFP_CODEC_TYPE)codec_type;
        snprintf(buf, 12, "AT+BCS=%u\r", codec_type);
        bt_hfp_cmd_process(link->bd_addr, HFP_CMD_ID_BCS, buf);
    }
    else /* If the received ID is not available, the HF shall respond with AT+BAC with its available codecs. */
    {
        bt_hfp_bac_cmd_assemble(buf);
        bt_hfp_cmd_process(link->bd_addr, HFP_CMD_ID_BAC, buf);
    }

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_bsir(T_BT_HFP_LINK *link,
                           const char    *str)
{
    int     res;
    int     offset;
    uint8_t value;
    int32_t ret = 0;

    res = sscanf(str, "+BSIR: %hhu%n", &value, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if (value)
    {
        link->supported_features |= HFP_INBAND_RINGTONE_ENABLE;
    }
    else
    {
        link->supported_features &= ~HFP_INBAND_RINGTONE_ENABLE;
    }

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t bt_hfp_handle_bind(T_BT_HFP_LINK *link,
                           const char    *str)
{
    int     res;
    int     offset;
    uint8_t indicator;
    uint8_t state;
    int32_t ret = 0;

    str += 6;
    if (*str == ' ')
    {
        str++;
    }

    /*
     * Response to AT+BIND=? format : +BIND: (<a>,<b>,<c>,...,<n>)
     * or to AT+BIND? format : +BIND: <a>,<state>
     */
    if ((*str != '('))
    {
        /* Unsolicited or Response to AT+BIND? format : +BIND: <a>,<state> */
        res = sscanf(str, " %hhu,%hhu%n", &indicator, &state, &offset);
        if (res < 2)
        {
            ret = 1;
            goto fail_no_bind_item_value;
        }

        if (*(str + offset) != '\0')
        {
            ret = 2;
            goto fail_bind_item_end_part;
        }

        if (indicator == HF_BIND_BATTERY_LEVEL)
        {
            link->batt_report_type = BATT_REPORTING_TYPE_BIEV;
            if (state == 1)
            {
                link->supported_features |= HFP_BATTERY_BIEV_ENABLE;
            }
            else
            {
                link->supported_features &= ~HFP_BATTERY_BIEV_ENABLE;
            }
        }
    }

    return 0;

fail_bind_item_end_part:
fail_no_bind_item_value:
    return ret;
}

int32_t bt_hfp_handle_cops(T_BT_HFP_LINK *link,
                           const char    *str)
{
    T_BT_HFP_NETWORK_OPERATOR_IND msg;
    T_BT_MSG_PAYLOAD              payload;
    int                           res;
    int                           offset;
    int32_t                       ret = 0;

    msg.mode = 0;
    msg.format = 0;
    memset(msg.name, 0, sizeof(msg.name));

    res = sscanf(str, "+COPS: %hhu,0,\"%16[^\"]\"%n", &msg.mode, msg.name, &offset);
    if (res <= 0)
    {
        ret = 1;
        goto fail_no_mode_operator_name;
    }

    if (res == 1)
    {
        offset = 0;
        res = sscanf(str, "+COPS: %hhu,0,\"\"%n", &msg.mode, &offset);
        if (res <= 0)
        {
            ret = 2;
            goto fail_no_mode_null_operator_name;
        }

        /* filter "+COPS: 1,0,\"" when res is 1. */
        if (offset == 0)
        {
            ret = 3;
            goto fail_mode_no_null_operator_name;
        }
    }

    str += offset;
    if (*str != '\0')
    {
        ret = 4;
        goto fail_end_part;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &msg;
    bt_mgr_dispatch(BT_MSG_HFP_NETWORK_OPERATOR_IND, &payload);

    return 0;

fail_end_part:
fail_mode_no_null_operator_name:
fail_no_mode_null_operator_name:
fail_no_mode_operator_name:
    return ret;
}

int32_t bt_hfp_handle_cnum(T_BT_HFP_LINK *link,
                           const char    *str)
{
    T_BT_HFP_SUBSCRIBER_NUMBER_IND msg;
    T_BT_MSG_PAYLOAD               payload;
    int                            res;
    int                            offset;
    int32_t                        ret = 0;

    msg.type = 128;
    msg.service = 4;
    memset(msg.number, 0, sizeof(msg.number));

    res = sscanf(str, "+CNUM: ,\"%20[^\"]\",%hhu%n", msg.number, &msg.type, &offset);
    if (res < 0)
    {
        ret = 1;
        goto fail_no_number;
    }

    if (res == 0)
    {
        res = sscanf(str, "+CNUM: ,\"\",%hhu%n", &msg.type, &offset);
        if (res <= 0)
        {
            ret = 2;
            goto fail_null_number_no_type;
        }
    }
    else if (res == 1)
    {
        ret = 3;
        goto fail_number_no_type;
    }

    str += offset;
    offset = 0;

    if (*str == ',')
    {
        /* check optional part */
        res = sscanf(str, ",,%hhu%n", &msg.service, &offset);
        if (res < 1)
        {
            ret = 4;
            goto fail_no_service;
        }
    }

    str += offset;
    if (*str != '\0')
    {
        ret = 5;
        goto fail_end_part;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &msg;
    bt_mgr_dispatch(BT_MSG_HFP_SUBSCRIBER_NUMBER_IND, &payload);

    return 0;

fail_end_part:
fail_no_service:
fail_number_no_type:
fail_null_number_no_type:
fail_no_number:
    return ret;
}

void bt_hfp_handle_at_rsp(T_BT_HFP_LINK *link,
                          const char    *at_response)
{
    char    *at_rsp;
    int32_t  ret = 0;

    if (!strncmp("OK", at_response, 2) ||
        !strncmp("ERROR", at_response, 5) ||
        !strncmp("+CME ERROR", at_response, 10))
    {
        /* Hayes processing is finished */
        sys_timer_stop(link->timer_handle);
        if (link->hfp_state >= HFP_STATE_CONNECTED)
        {
            T_HFP_CMD_ITEM *cmd_item;

            link->credits = 1;

            cmd_item = os_queue_out(&link->cmd_queue);
            if (cmd_item != NULL)
            {
                if (!strncmp("OK", at_response, 2))
                {
                    bt_hfp_handle_ok(link, cmd_item->cmd_id);
                }
                else if (!strncmp("ERROR", at_response, 5))
                {
                    bt_hfp_handle_error(link, cmd_item->cmd_id);
                }
                else /* "+CME ERROR:", only when +CME ERROR is enabled by AT+CMEE=1 */
                {
                    at_rsp = "+CME ERROR";
                    ret = bt_hfp_handle_cme_error(link, at_response, cmd_item->cmd_id);
                }

                free(cmd_item);
                bt_hfp_cmd_flush(link);
            }
        }
        else if (link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTING)
        {
            if (!strncmp("OK", at_response, 2))
            {
                bt_hfp_srv_level_continue(link, true);
            }
            else
            {
                bt_hfp_srv_level_continue(link, false);
            }
        }
    }
    else
    {
        /*
         * 1. There is an inconsistency in the spec between gsm 07.07 and hs / hf
         *    hs defines '=' as the separator, hf defines the ':' as the separator
         *    therefore we completely ignore the separator on the incoming side.
         * 2. Iphone send +BRSF:<AG supported features bitmap> with no space
         */
        if (!strncmp("+BRSF", at_response, 5))
        {
            at_rsp = "+BRSF";
            ret = bt_hfp_handle_brsf(link, at_response);
        }
        else if (!strncmp("+CIND:", at_response, 6))
        {
            at_rsp = "+CIND:";
            ret = bt_hfp_handle_cind(link, at_response);
        }
        else if (!strncmp("+CHLD", at_response, 5))
        {
            /*
             * Parse a capability list.
             * GSM Spec ETS 300 916:
             * It is recommended (although optional) that test command returns a list of
             * operations which are supported.
             * The call number required by some operations shall be denoted by "x" (e.g. +CHLD: (0,1,1x,2,2x,3)).
             */
        }
        else if (!strncmp("+CLCC", at_response, 5))
        {
            at_rsp = "+CLCC";
            ret = bt_hfp_handle_clcc(link, at_response);
        }
        else if (!strncmp("RING", at_response, 4))
        {
            bt_hfp_handle_ring(link);
        }
        else if (!strncmp("+BVRA", at_response, 5))
        {
            at_rsp = "+BVRA";
            ret = bt_hfp_handle_bvra(link, at_response);
        }
        else if (!strncmp("+CLIP", at_response, 5))
        {
            at_rsp = "+CLIP";
            ret = bt_hfp_handle_clip(link, at_response);
        }
        else if (!strncmp("+CIEV", at_response, 5))
        {
            at_rsp = "+CIEV";
            ret = bt_hfp_handle_ciev(link, at_response);
        }
        else if (!strncmp("+CCWA", at_response, 5))
        {
            at_rsp = "+CCWA";
            ret = bt_hfp_handle_ccwa(link, at_response);
        }
        else if (!strncmp("+VGM", at_response, 4))
        {
            at_rsp = "+VGM";
            ret = bt_hfp_handle_vgm(link, at_response);
        }
        else if (!strncmp("+VGS", at_response, 4))
        {
            at_rsp = "+VGS";
            ret = bt_hfp_handle_vgs(link, at_response);
        }
        else if (!strncmp("+BCS", at_response, 4))
        {
            at_rsp = "+BCS";
            ret = bt_hfp_handle_bcs(link, at_response);
        }
        else if (!strncmp("+BSIR", at_response, 5))
        {
            at_rsp = "+BSIR";
            ret = bt_hfp_handle_bsir(link, at_response);
        }
        else if (!strncmp("+BIND:", at_response, 6))
        {
            at_rsp = "+BIND";
            ret = bt_hfp_handle_bind(link, at_response);
        }
        else if (!strncmp("+COPS", at_response, 5))
        {
            at_rsp = "+COPS";
            ret = bt_hfp_handle_cops(link, at_response);
        }
        else if (!strncmp("+CNUM", at_response, 5))
        {
            at_rsp = "+CNUM";
            ret = bt_hfp_handle_cnum(link, at_response);
        }
        else /* vendor AT command */
        {
            T_BT_MSG_PAYLOAD payload;

            memcpy(payload.bd_addr, link->bd_addr, 6);
            payload.msg_buf = (void *)at_response;
            bt_mgr_dispatch(BT_MSG_HFP_VENDOR_CMD, &payload);
        }
    }

    if (ret != 0)
    {
        PROFILE_PRINT_ERROR2("bt_hfp_handle_at_rsp: %s, failed %d", TRACE_STRING(at_rsp), -ret);
    }
}

void bt_hfp_handle_rfc_data_ind(T_RFC_DATA_IND *data_msg)
{
    T_BT_HFP_LINK *link;
    uint8_t       *data;
    uint8_t       *temp_ptr = NULL;
    uint16_t       length;

    link = bt_hfp_find_link_by_addr(data_msg->bd_addr);
    if (link == NULL)
    {
        return;
    }

    if (link->hfp_state == HFP_STATE_RFC_CONN_NO_CREDITS && data_msg->remain_credits > 0)
    {
        link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
        bt_hfp_srv_level_start(link);
    }

    if (data_msg->length == 0)
    {
        return;
    }

    if (link->rx_ptr != NULL)
    {
        temp_ptr = link->rx_ptr;
        link->rx_ptr = malloc((link->rx_len + data_msg->length));
        if (link->rx_ptr == NULL)
        {
            return;
        }
        memcpy(link->rx_ptr, temp_ptr, link->rx_len);
        free(temp_ptr);
        memcpy(&(link->rx_ptr[link->rx_len]), data_msg->buf, data_msg->length);
        data = link->rx_ptr;
        length = link->rx_len + data_msg->length;
    }
    else
    {
        data = data_msg->buf;
        length = data_msg->length;
    }
    link->rx_len = 0;

    while (length)
    {
        if ((*data > ' ') || ((*data == ' ') && (link->rx_len > 0)))
        {
            if (link->rx_len == 0)
            {
                temp_ptr = data;
            }
            link->rx_len++;
        }
        else if ((*data == '\r') && (link->rx_len > 0))
        {
            *data = '\0';
            bt_hfp_handle_at_rsp(link, (const char *)temp_ptr);
            link->rx_len = 0;
        }
        data++;
        length--;
    }

    data = link->rx_ptr;
    link->rx_ptr = NULL;
    if (link->rx_len)
    {
        link->rx_ptr = malloc(link->rx_len);
        if (link->rx_ptr == NULL)
        {
            return;
        }
        memcpy(link->rx_ptr, temp_ptr, link->rx_len);
    }
    if (data != NULL)
    {
        free(data);
    }

    rfc_data_cfm(data_msg->bd_addr, data_msg->dlci, 1);
}

void bt_hfp_handle_rfc_msg(T_RFC_MSG_TYPE  msg_type,
                           void           *msg)
{
    PROFILE_PRINT_INFO1("bt_hfp_handle_rfc_msg: msg_type 0x%02x", msg_type);

    switch (msg_type)
    {
    case RFC_CONN_IND:
        {
            T_RFC_CONN_IND *conn_ind_msg = (T_RFC_CONN_IND *)msg;
            T_BT_HFP_LINK *link = bt_hfp_find_link_by_addr(conn_ind_msg->bd_addr);

            if (link == NULL)
            {
                link = bt_hfp_alloc_link(conn_ind_msg->bd_addr);
                if (link == NULL)
                {
                    rfc_conn_cfm(conn_ind_msg->bd_addr, conn_ind_msg->dlci, RFC_REJECT,
                                 conn_ind_msg->frame_size, RFC_DEFAULT_CREDIT);
                }
                else
                {
                    T_BT_MSG_PAYLOAD payload;

                    link->dlci = conn_ind_msg->dlci;
                    link->frame_size = conn_ind_msg->frame_size;

                    memcpy(payload.bd_addr, link->bd_addr, 6);
                    payload.msg_buf = NULL;
                    bt_mgr_dispatch(BT_MSG_HFP_CONN_IND, &payload);
                }
            }
            else
            {
                rfc_conn_cfm(conn_ind_msg->bd_addr, conn_ind_msg->dlci, RFC_REJECT,
                             conn_ind_msg->frame_size, RFC_DEFAULT_CREDIT);
            }
        }
        break;

    case RFC_CONN_CMPL:
        {
            bt_hfp_handle_rfc_conn_cmpl((T_RFC_CONN_CMPL *)msg);
        }
        break;

    case RFC_DISCONN_CMPL:
        {
            T_RFC_DISCONN_CMPL *disc_msg = (T_RFC_DISCONN_CMPL *)msg;
            T_BT_HFP_LINK      *link;

            link = bt_hfp_find_link_by_addr(disc_msg->bd_addr);
            if (link != NULL)
            {
                if (link->dlci == disc_msg->dlci)
                {
                    T_BT_MSG_PAYLOAD payload;
                    uint16_t         cause;

                    cause = disc_msg->cause;
                    payload.msg_buf = &cause;
                    memcpy(payload.bd_addr, disc_msg->bd_addr, 6);

                    if (link->hfp_state >= HFP_STATE_CONNECTED)
                    {
                        bt_pm_cback_unregister(link->bd_addr, bt_hfp_pm_cback);

                        if (link->uuid == UUID_HEADSET)
                        {
                            bt_hfp_free_link(link);
                            bt_mgr_dispatch(BT_MSG_HSP_DISCONN_CMPL, &payload);
                        }
                        else
                        {
                            bt_hfp_free_link(link);
                            bt_mgr_dispatch(BT_MSG_HFP_DISCONN_CMPL, &payload);
                        }
                    }
                    else
                    {
                        if (link->uuid == UUID_HEADSET)
                        {
                            bt_hfp_free_link(link);
                            bt_mgr_dispatch(BT_MSG_HSP_CONN_FAIL, &payload);
                        }
                        else
                        {
                            bt_hfp_free_link(link);
                            bt_mgr_dispatch(BT_MSG_HFP_CONN_FAIL, &payload);
                        }
                    }
                }
            }
        }
        break;

    case RFC_CREDIT_INFO:
        {
            T_RFC_CREDIT_INFO *credit_msg = (T_RFC_CREDIT_INFO *)msg;
            T_BT_HFP_LINK *link = bt_hfp_find_link_by_addr(credit_msg->bd_addr);
            if (link != NULL)
            {
                if ((link->hfp_state == HFP_STATE_RFC_CONN_NO_CREDITS) &&
                    (credit_msg->remain_credits > 0))
                {
                    link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
                    bt_hfp_srv_level_start(link);
                }
            }
        }
        break;

    case RFC_DATA_IND:
        bt_hfp_handle_rfc_data_ind((T_RFC_DATA_IND *)msg);
        break;

    case RFC_DLCI_CHANGE:
        {
            T_RFC_DLCI_CHANGE_INFO *info = (T_RFC_DLCI_CHANGE_INFO *)msg;
            T_BT_HFP_LINK *link;

            link = bt_hfp_find_link_by_addr(info->bd_addr);
            if (link)
            {
                link->dlci = info->curr_dlci;
            }
        }
        break;

    default:
        break;
    }
}

void bt_hfp_tout_cback(T_SYS_TIMER_HANDLE handle)
{
    T_BT_HFP_LINK *link;

    link = (void *)sys_timer_id_get(handle);
    if (os_queue_search(&bt_hfp->link_list, link) == true)
    {
        if (link->hfp_state >= HFP_STATE_CONNECTED)
        {
            T_HFP_CMD_ITEM *cmd_item;

            cmd_item = os_queue_out(&link->cmd_queue);
            if (cmd_item != NULL)
            {
                T_BT_MSG_PAYLOAD payload;
                T_BT_HFP_CMD_RSP cmd_rsp = HFP_CMD_TIMEOUT;

                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = &cmd_rsp;
                link->credits = 1;
                switch (cmd_item->cmd_id)
                {
                case HFP_CMD_ID_BATT_XAPL:
                    {
                        link->batt_report_type = BATT_REPORTING_TYPE_APPLE;
                        bt_hfp_batt_level_report(link->bd_addr, link->batt_level);
                    }
                    break;

                case HFP_CMD_ID_SEND_CLCC:
                    {
                        link->clcc_ind = CLCC_IND_CALL_IDLE;
                    }
                    break;

                case HFP_CMD_ID_DIAL_WITH_NUMBER:
                    {
                        bt_mgr_dispatch(BT_MSG_HFP_DIAL_WITH_NUMBER_RESULT, &payload);
                    }
                    break;

                case HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE:
                    {
                        bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_ACTIVATION, &payload);
                    }
                    break;

                case HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE:
                    {
                        bt_mgr_dispatch(BT_MSG_HFP_VOICE_RECOGNITION_DEACTIVATION, &payload);
                    }
                    break;

                case HFP_CMD_ID_VND_AT_CMD:
                    {
                        bt_mgr_dispatch(BT_MSG_HFP_VND_AT_CMD_RSP, &payload);
                    }
                    break;

                default:
                    break;
                }

                free(cmd_item);
                bt_hfp_cmd_flush(link);
            }
        }
        else if (link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTING)
        {
            bt_hfp_srv_level_continue(link, false);
        }
    }
}

bool bt_hfp_init(uint8_t  rfc_hfp_chann_num,
                 uint8_t  rfc_hsp_chann_num,
                 uint16_t supported_features,
                 uint8_t  supported_codecs)
{
    int32_t ret = 0;

    bt_hfp = calloc(1, sizeof(T_BT_HFP));
    if (bt_hfp == NULL)
    {
        ret = 1;
        goto fail_alloc_hfp;
    }

    if (rfc_cback_register(rfc_hfp_chann_num,
                           bt_hfp_handle_rfc_msg,
                           &bt_hfp->hfp_service_id) == false)
    {
        ret = 2;
        goto fail_reg_hfp_rfc_cback;
    }

    if (rfc_cback_register(rfc_hsp_chann_num,
                           bt_hfp_handle_rfc_msg,
                           &bt_hfp->hsp_service_id) == false)
    {
        ret = 3;
        goto fail_reg_hsp_rfc_cback;
    }

    bt_hfp->wait_rsp_tout = 3000; /* uint is ms */
    bt_hfp->brsf_cpbs = supported_features;
    bt_hfp->supt_codecs = supported_codecs;

    return true;

fail_reg_hsp_rfc_cback:
fail_reg_hfp_rfc_cback:
    free(bt_hfp);
    bt_hfp = NULL;
fail_alloc_hfp:
    PROFILE_PRINT_ERROR1("bt_hfp_init: failed %d", -ret);
    return false;
}

bool bt_hfp_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_HFP_INFO *info)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t count;

        memcpy(info->bd_addr, bd_addr, 6);
        info->cpbs = link->cpbs;
        info->uuid = link->uuid;
        info->at_cmd_credits = link->credits;
        info->rfc_dlci = link->dlci;
        info->state = link->hfp_state;
        info->frame_size = link->frame_size;
        info->indicator_cnt = link->indicator_cnt;
        info->batt_report_type = link->batt_report_type;
        info->curr_call_status = link->curr_call_status;
        info->prev_call_status = link->prev_call_status;
        info->service_status = link->ag_status_ind.service_status;
        info->supported_features = link->supported_features;
        info->codec_type = link->codec_type;

        for (count = 0; count < info->indicator_cnt; count++)
        {
            info->indicator_type[count] = link->indicator_type[count];
            info->indicator_range[count] = link->indicator_range[count];
        }

        return rfc_get_cid(link->bd_addr, info->rfc_dlci, &info->l2c_cid);
    }

    return false;
}

bool bt_hfp_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_HFP_INFO *info)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_hfp_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        uint8_t count;

        link->cpbs = info->cpbs;
        link->credits = info->at_cmd_credits;
        link->hfp_state = (T_BT_HFP_STATE)info->state;
        link->uuid = info->uuid;
        link->dlci = info->rfc_dlci;
        link->frame_size = info->frame_size;
        link->batt_report_type = info->batt_report_type;
        link->curr_call_status = (T_BT_HFP_CALL_STATUS)info->curr_call_status;
        link->prev_call_status = (T_BT_HFP_CALL_STATUS)info->prev_call_status;
        link->ag_status_ind.service_status = (T_HFP_SERVICE_STATUS)info->service_status;
        link->supported_features = info->supported_features;
        link->codec_type = (T_HFP_CODEC_TYPE)info->codec_type;

        if (link->credits == 0)
        {
            sys_timer_start(link->timer_handle);
        }

        link->indicator_cnt = info->indicator_cnt;
        for (count = 0; count < info->indicator_cnt; count++)
        {
            link->indicator_type[count] = info->indicator_type[count];
            link->indicator_range[count] = info->indicator_range[count];
        }

        bt_pm_cback_register(bd_addr, bt_hfp_pm_cback);

        return true;
    }

    return false;
}

bool bt_hfp_roleswap_info_del(uint8_t  bd_addr[6],
                              uint16_t cause)
{
    T_BT_HFP_LINK *link;

    link = bt_hfp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;
        T_BT_MSG         msg;

        if (link->uuid == UUID_HEADSET)
        {
            msg = BT_MSG_HSP_DISCONN_CMPL;
        }
        else
        {
            msg = BT_MSG_HFP_DISCONN_CMPL;
        }

        bt_pm_cback_unregister(bd_addr, bt_hfp_pm_cback);
        bt_hfp_free_link(link);
        payload.msg_buf = &cause;
        memcpy(payload.bd_addr, bd_addr, 6);
        bt_mgr_dispatch(msg, &payload);

        return true;
    }

    return false;
}

bool bt_hfp_service_id_get(uint16_t  uuid,
                           uint8_t  *service_id)
{
    if (bt_hfp != NULL)
    {
        if (uuid == UUID_HANDSFREE)
        {
            *service_id = bt_hfp->hfp_service_id;
        }
        else
        {
            *service_id = bt_hfp->hsp_service_id;
        }

        return true;
    }

    return false;
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bt_hfp_int.h"

bool bt_hfp_init(uint8_t  rfc_hfp_chann_num,
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

void bt_hfp_handle_audio_conn_cmpl(uint8_t bd_addr[6])
{
    return;
}

void bt_hfp_handle_audio_disconn(uint8_t bd_addr[6])
{
    return;
}

bool bt_hfp_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_HFP_INFO *info)
{
    return false;
}

bool bt_hfp_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_HFP_INFO *info)
{
    return false;
}

bool bt_hfp_roleswap_info_del(uint8_t  bd_addr[6],
                              uint16_t cause)
{
    return false;
}

uint8_t bt_hfp_rfc_profile_idx_get(uint16_t uuid)
{
    return false;
}
#endif
