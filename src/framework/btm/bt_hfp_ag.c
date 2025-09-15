/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HFP_AG_SUPPORT == 1)

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "trace.h"
#include "gap_br.h"
#include "sys_timer.h"
#include "rfc.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "hfp_utils.h"
#include "bt_hfp_ag.h"
#include "bt_hfp_ag_int.h"

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

#define HFP_AG_INBAND_RINGTONE_ENABLE      0x01
#define HFP_AG_BATTERY_BIEV_ENABLE         0x02
#define HFP_AG_ESCO_S4_ENABLE              0x04
#define HFP_AG_CODEC_NEGOTIATION_ENABLE    0x08

#define HFP_AG_DEFAULT_RING_REPEAT_INTERVAL (5000)  /* ms */

/* Extended Audio Gateway Error Result Codes */
#define HFP_AG_ERR_PHONE_FAILURE    0   /* AG Failure */
#define HFP_AG_ERR_NO_CONN_PHONE    1   /* No connection to phone */
#define HFP_AG_ERR_OP_NOT_ALLOWED   3   /* Operation not allowed */
#define HFP_AG_ERR_OP_NOT_SUPPORTED 4   /* Operation not supported */
#define HFP_AG_ERR_PHSIM_PIN_REQ    5   /* PH-SIM PIN required */
#define HFP_AG_ERR_SIM_NOT_INSERTED 10  /* SIM not inserted */
#define HFP_AG_ERR_SIM_PIN_REQ      11  /* SIM PIN required */
#define HFP_AG_ERR_SIM_PUK_REQ      12  /* SIM PUK required */
#define HFP_AG_ERR_SIM_FAILURE      13  /* SIM failure */
#define HFP_AG_ERR_SIM_BUSY         14  /* SIM busy */
#define HFP_AG_ERR_INCORRECT_PWD    16  /* Incorrect password */
#define HFP_AG_ERR_SIM_PIN2_REQ     17  /* SIM PIN2 required */
#define HFP_AG_ERR_SIM_PUK2_REQ     18  /* SIM PUK2 required */
#define HFP_AG_ERR_MEMORY_FULL      20  /* Memory full */
#define HFP_AG_ERR_INVALID_INDEX    21  /* Invalid index */
#define HFP_AG_ERR_MEMORY_FAILURE   23  /* Memory failure */
#define HFP_AG_ERR_TEXT_TOO_LONG    24  /* Text string too long */
#define HFP_AG_ERR_INV_CHAR_IN_TSTR 25  /* Invalid characters in text string */
#define HFP_AG_ERR_DSTR_TOO_LONG    26  /* Dial string too long */
#define HFP_AG_ERR_INV_CHAR_IN_DSTR 27  /* Invalid characters in dial string */
#define HFP_AG_ERR_NO_NETWORK_SERV  30  /* No network service */
#define HFP_AG_ERR_NETWORK_TIME_OUT 31  /* Network timeout */
#define HFP_AG_ERR_NO_NET_EMG_ONLY  32  /* Network not allowed - emergency service only */

#define HFP_AG_IND_SERVICE_MASK              (0x0001)
#define HFP_AG_IND_CALL_MASK                 (0x0002)
#define HFP_AG_IND_CALLSETUP_MASK            (0x0004)
#define HFP_AG_IND_CALLHELD_MASK             (0x0008)
#define HFP_AG_IND_SIGNAL_MASK               (0x0010)
#define HFP_AG_IND_ROAM_MASK                 (0x0020)
#define HFP_AG_IND_BATTCHG_MASK              (0x0040)
#define HFP_AG_IND_ALL_MASK                  (0x007f)

#define HFP_AG_CHLD_VAL "(0,1,2,3)"
#define HFP_HF_SUPPORTED_INDICATORS_MASK (0x03)
#define HFP_HF_SUPPORTED_XAPL_FEATURES_MASK  (0x02U)

/*
 * HFP HF indicator Mask.
 * 0 bit for Enhanced Safety Status
 * 1 bit for Battery Level Status
 */
typedef enum t_hfp_hf_ind_id
{
    HFP_HF_IND_ENHANCED_SAFETY_STATUS_ID = 0x01,
    HFP_HF_IND_BATTERY_LEVEL_STATUS_ID   = 0x02,
} T_HFP_HF_IND_ID;

/*
 * XAPL FEATURES HF support.
 * Bit 0 = reserved
 * Bit 1 = The accessory supports battery reporting (reserved only for battery operated accessories).
 * Bit 2 = The accessory is docked or powered (reserved only for battery operated accessories).
 * Bit 3 = The accessory supports Siri status reporting.
 * Bit 4 = the accessory supports noise reduction (NR) status reporting.
 */
typedef enum t_hfp_hf_chld_id
{
    HFP_AG_HF_3WAY_HELD_CALL_RELEASED   = 0,
    HFP_AG_HF_3WAY_ACTIVE_CALL_RELEASED = 1,
    HFP_AG_HF_3WAY_SWITCHED             = 2,
    HFP_AG_HF_3WAY_MERGED               = 3,
} T_HFP_HF_CHLD_ID;

/*
 * XAPL FEATURES HF support.
 * Bit 0 = reserved
 * Bit 1 = The accessory supports battery reporting (reserved only for battery operated accessories).
 * Bit 2 = The accessory is docked or powered (reserved only for battery operated accessories).
 * Bit 3 = The accessory supports Siri status reporting.
 * Bit 4 = the accessory supports noise reduction (NR) status reporting.
 */
typedef enum t_hfp_hf_xapl_id
{
    HFP_AG_HF_XAPL_RESEVED_ID        = 0,
    HFP_AG_HF_XAPL_BATTERY_STATUS_ID = 1,
    HFP_AG_HF_XAPL_DOCK_STATUS_ID    = 2,
    HFP_AG_HF_XAPL_SIRI_STATUS_ID    = 3,
    HFP_AG_HF_XAPL_NR_STATUS_ID      = 4,
} T_HFP_HF_XAPL_ID;

typedef struct t_bt_hfp_ag_link
{
    struct t_bt_hfp_ag_link *next;
    void                    *timer_ring;
    uint8_t                  bd_addr[6];
    T_BT_HFP_STATE           hfp_state;
    T_HFP_SRV_LEVEL_STEP     service_level_step;
    T_HFP_CODEC_TYPE         codec_type;
    uint8_t                  dlci;
    uint8_t                  credits;
    uint8_t                  supported_features;
    uint16_t                 frame_size;
    uint16_t                 cpbs;
    uint16_t                 uuid;              /* indicates hfp or hsp */
    uint16_t                 rx_len;
    uint8_t                 *rx_ptr;
    T_OS_QUEUE               cmd_queue;
    uint32_t                 hf_indicator_mask; /* determined by the hf BIND AT Cmd */
    uint32_t                 ag_indicator_mask; /* determined by the hf BIA and CMER AT Cmd */
    uint32_t                 ag_hf_xapl_mask;   /* determined by the hf XAPL AT Cmd */
    T_HFP_AG_STATUS_IND      ag_status_ind;
    uint16_t                 ring_interval;
    bool                     cmer_enable;       /* indication status update 1:enabled 0:disabled */
    bool                     cmee_enable;       /* enhanced error code 1:enabled 0:disabled */
    bool                     clip_enable;       /* calling line identification 1:enabled 0:disabled */
    bool                     ccwa_enable;       /* calling waiting 1:enabled 0:disabled */
    bool                     cops_enable;       /* long alphanumeric name format 1:enabled 0:disabled */
    uint8_t                  supt_codecs;       /* bit0:CVSD bit1:mSBC bit2~7 vendor */
    uint8_t                  codec_type_sent;
    bool                     codec_updated;
    T_BT_HFP_AG_SCO_SETTING  sco_setting;
    bool                     sco_int_flag;
    uint8_t                  prev_call_status;
    uint8_t                  curr_call_status;
    uint8_t                  call_num_type;
    char                     call_num[20];
} T_BT_HFP_AG_LINK;

typedef struct t_bt_hfp_ag
{
    T_OS_QUEUE link_list;
    uint8_t    hfp_service_id;
    uint8_t    hsp_service_id;
    uint16_t   brsf_cpbs;
    uint8_t    supt_codecs;       /* bit0:CVSD;bit1:mSBC,bit2~7 vendor */
} T_BT_HFP_AG;

static T_BT_HFP_AG *bt_hfp_ag;

static const char hfp_ag_cind_info[] = "\r\n+CIND: (\"service\",(0-1)),(\"call\",(0,1)),"
                                       "(\"callsetup\",(0-3)),(\"callheld\",(0-2)),"
                                       "(\"signal\",(0-6)),(\"roam\",(0,1)),(\"battchg\",(0-5))\r\n";

void bt_hfp_ag_timeout_cback(T_SYS_TIMER_HANDLE handle);

T_BT_HFP_AG_LINK *bt_hfp_ag_alloc_link(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;
    uint32_t          ret = 0;

    link = calloc(1, sizeof(T_BT_HFP_AG_LINK));
    if (link == NULL)
    {
        ret = 1;
        goto fail_alloc_link;
    }

    link->timer_ring = sys_timer_create("hfp_ag_ring",
                                        SYS_TIMER_TYPE_LOW_PRECISION,
                                        (uint32_t)link,
                                        link->ring_interval * 1000,
                                        false,
                                        bt_hfp_ag_timeout_cback);
    if (link->timer_ring == NULL)
    {
        ret = 2;
        goto fail_create_timer;
    }

    memcpy(link->bd_addr, bd_addr, 6);
    link->hfp_state = HFP_STATE_DISCONNECTED;
    os_queue_in(&bt_hfp_ag->link_list, link);

    return link;

fail_create_timer:
    free(link);
fail_alloc_link:
    PROFILE_PRINT_ERROR1("bt_hfp_ag_alloc_link: failed %d", ret);
    return NULL;
}

void bt_hfp_ag_free_link(T_BT_HFP_AG_LINK *link)
{
    if (os_queue_delete(&bt_hfp_ag->link_list, link) == true)
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

        if (link->timer_ring != NULL)
        {
            sys_timer_delete(link->timer_ring);
        }

        free(link);
    }
}

T_BT_HFP_AG_LINK *bt_hfp_ag_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link = NULL;

    if (bt_hfp_ag != NULL)
    {
        link = os_queue_peek(&bt_hfp_ag->link_list, 0);
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

bool bt_hfp_ag_connect_req(uint8_t bd_addr[6],
                           uint8_t server_chann,
                           bool    is_hfp)
{
    T_BT_HFP_AG_LINK *link;
    uint8_t           service_id;
    int32_t           ret = 0;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    link = bt_hfp_ag_alloc_link(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (is_hfp)
    {
        service_id = bt_hfp_ag->hfp_service_id;
    }
    else
    {
        service_id = bt_hfp_ag->hsp_service_id;
    }

    if (rfc_conn_req(bd_addr, server_chann, service_id, RFC_DEFAULT_MTU, 0, &link->dlci) == false)
    {
        ret = 3;
        goto fail_conn_rfc;
    }

    link->ag_indicator_mask = HFP_AG_IND_ALL_MASK;
    link->ag_hf_xapl_mask = HFP_HF_SUPPORTED_XAPL_FEATURES_MASK;

    return true;

fail_conn_rfc:
    bt_hfp_ag_free_link(link);
fail_alloc_link:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_hfp_ag_connect_req: bd_addr %s, failed %d", TRACE_BDADDR(bd_addr), -ret);
    return false;
}

bool bt_hfp_ag_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(bd_addr, false);
        bt_sco_disconn_req(bd_addr);
        return rfc_disconn_req(bd_addr, link->dlci);
    }

    return false;
}

bool bt_hfp_ag_connect_cfm(uint8_t bd_addr[6],
                           bool    accept)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint16_t status;

        status = accept ? RFC_ACCEPT : RFC_REJECT;
        rfc_conn_cfm(bd_addr, link->dlci, status, link->frame_size, 0);
        if (accept == false)
        {
            bt_hfp_ag_free_link(link);
        }

        return true;
    }

    return false;
}

bool bt_hfp_ag_cmd_flush(T_BT_HFP_AG_LINK *link)
{
    T_HFP_CMD_ITEM *cmd_item;

    while ((link->cmd_queue.count > 0) && (link->credits > 0))
    {
        cmd_item = os_queue_out(&link->cmd_queue);
        if (cmd_item != NULL)
        {
            bool ret;

            ret = rfc_data_req(link->bd_addr,
                               link->dlci,
                               (uint8_t *)cmd_item->at_cmd,
                               cmd_item->cmd_len,
                               false);
            if (ret == true)
            {
                link->credits--;
            }

            free(cmd_item);
            return ret;
        }
    }

    return true;
}

bool bt_hfp_ag_cmd_send(uint8_t     bd_addr[6],
                        const char *at_cmd)
{
    T_BT_HFP_AG_LINK *link;
    int               cmd_len;
    T_HFP_CMD_ITEM   *cmd_item;
    int32_t           ret = 0;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (link->hfp_state < HFP_STATE_SRV_LEVEL_CONNECTING)
    {
        ret = 2;
        goto fail_invalid_state;
    }

    cmd_len = strlen(at_cmd);
    if ((cmd_len < 2) || (cmd_len > link->frame_size))
    {
        ret = 3;
        goto fail_invalid_cmd_length;
    }

    if ((at_cmd[cmd_len - 2] != '\r') && (at_cmd[cmd_len - 1] != '\n'))
    {
        ret = 4;
        goto fail_invalid_cmd_delimiter;
    }

    cmd_item = calloc(1, sizeof(T_HFP_CMD_ITEM) + cmd_len);
    if (cmd_item == NULL)
    {
        ret = 5;
        goto fail_alloc_cmd_item;
    }

    memcpy((char *)cmd_item->at_cmd, at_cmd, cmd_len);
    cmd_item->cmd_len = cmd_len;
    os_queue_in(&link->cmd_queue, cmd_item);

    return bt_hfp_ag_cmd_flush(link);

fail_alloc_cmd_item:
fail_invalid_cmd_delimiter:
fail_invalid_cmd_length:
fail_invalid_state:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_hfp_ag_cmd_send: bd_addr %s, failed %d",
                         TRACE_BDADDR(bd_addr), -ret);
    return false;
}

void bt_hfp_ag_call_status_set(T_BT_HFP_AG_LINK *link, T_BT_HFP_AG_CALL_STATUS call_status)
{
    if ((link->hfp_state == HFP_STATE_CONNECTED) || (link->hfp_state == HSP_STATE_CONNECTED))
    {
        if (link->curr_call_status != call_status)
        {
            T_BT_MSG_PAYLOAD             payload;
            T_BT_HFP_AG_CALL_STATUS_INFO info;

            link->prev_call_status = link->curr_call_status;
            link->curr_call_status = call_status;
            memcpy(payload.bd_addr, link->bd_addr, 6);
            info.prev_call_status = link->prev_call_status;
            info.curr_call_status = link->curr_call_status;
            payload.msg_buf = &info;
            bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
        }
    }
    else
    {
        link->curr_call_status = call_status;
    }
}

bool bt_hfp_ag_pm_cback(uint8_t       bd_addr[6],
                        T_BT_PM_EVENT event)
{
    T_BT_HFP_AG_LINK *link;
    bool              ret = true;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
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
                if (link->curr_call_status != BT_HFP_AG_CALL_IDLE)
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

bool bt_hfp_ag_ok_send(uint8_t bd_addr[6])
{
    char buf[7];

    snprintf(buf, 7, "\r\nOK\r\n");
    return bt_hfp_ag_cmd_send(bd_addr, buf);
}

bool bt_hfp_ag_error_send(uint8_t bd_addr[6],
                          uint8_t error_code)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        char buf[19];

        if (link->cmee_enable)
        {
            snprintf(buf, 19, "\r\n+CME ERROR: %u\r\n", error_code);
        }
        else
        {
            snprintf(buf, 10, "\r\nERROR\r\n");
        }

        return bt_hfp_ag_cmd_send(bd_addr, buf);
    }

    return false;
}

bool bt_hfp_ag_codec_negotiate_send(T_BT_HFP_AG_LINK *link,
                                    bool              fallback)
{
    uint8_t codec_type = CODEC_TYPE_CVSD;
    char    buf[14];

    if (fallback == false)
    {
        uint8_t codec_bitmap;

        codec_bitmap = link->supt_codecs & bt_hfp_ag->supt_codecs;

        if (codec_bitmap & (1 << (CODEC_TYPE_LC3 - 1)))
        {
            codec_type = CODEC_TYPE_LC3;
        }
        else if (codec_bitmap & (1 << (CODEC_TYPE_MSBC - 1)))
        {
            codec_type = CODEC_TYPE_MSBC;
        }
    }

    link->codec_type_sent = codec_type;
    snprintf(buf, 14, "\r\n+BCS: %u\r\n", codec_type);

    return bt_hfp_ag_cmd_send(link->bd_addr, buf);
}

void bt_hfp_ag_codec_negotiate(T_BT_HFP_AG_LINK *link)
{
    bt_sniff_mode_exit(link->bd_addr, false);

    if (link->hfp_state == HFP_STATE_CONNECTED)
    {
        if (link->codec_type_sent != CODEC_TYPE_NONE)
        {
            return;
        }

        if (link->sco_setting == HFP_AG_SCO_SETTING_ANY)
        {
            if ((link->supported_features & HFP_AG_CODEC_NEGOTIATION_ENABLE) &&
                (link->codec_updated))
            {
                link->codec_updated = false;
                bt_hfp_ag_codec_negotiate_send(link, false);
            }
            else
            {
                if (link->codec_type == CODEC_TYPE_CVSD)
                {
                    if (link->supported_features & HFP_AG_ESCO_S4_ENABLE)
                    {
                        link->sco_setting = HFP_AG_SCO_SETTING_S4;
                        gap_br_send_sco_conn_req(link->bd_addr, 8000, 8000, 12, 0x0360, 2, SCO_PKT_TYPES_S4);
                    }
                    else
                    {
                        link->sco_setting = HFP_AG_SCO_SETTING_S3;
                        gap_br_send_sco_conn_req(link->bd_addr, 8000, 8000, 10, 0x0360, 1, SCO_PKT_TYPES_S3);
                    }
                }
                else if (link->codec_type == CODEC_TYPE_MSBC)
                {
                    link->sco_setting = HFP_AG_SCO_SETTING_T2;
                    gap_br_send_sco_conn_req(link->bd_addr, 8000, 8000, 13, 0x0363, 2, SCO_PKT_TYPES_T2);
                }
                else if (link->codec_type == CODEC_TYPE_LC3)
                {
                    link->sco_setting = HFP_AG_SCO_SETTING_T2;
                    gap_br_send_sco_conn_req(link->bd_addr, 8000, 8000, 13, 0x0363, 2, SCO_PKT_TYPES_T2);
                }
            }
        }
        else
        {
            switch (link->sco_setting)
            {
            case HFP_AG_SCO_SETTING_T2:
                link->sco_setting = HFP_AG_SCO_SETTING_T1;
                gap_br_send_sco_conn_req(link->bd_addr, 8000, 8000, 8, 0x0363, 2, SCO_PKT_TYPES_T1);
                break;

            case HFP_AG_SCO_SETTING_T1:
                link->sco_setting = HFP_AG_SCO_SETTING_ANY;
                bt_hfp_ag_codec_negotiate_send(link, true);
                break;

            case HFP_AG_SCO_SETTING_S4:
            case HFP_AG_SCO_SETTING_S3:
                link->sco_setting = HFP_AG_SCO_SETTING_S1;
                gap_br_send_sco_conn_req(link->bd_addr, 8000, 8000, 7, 0x0360, 1, SCO_PKT_TYPES_S1);
                break;

            case HFP_AG_SCO_SETTING_S1:
                link->sco_setting = HFP_AG_SCO_SETTING_D1;
                gap_br_send_sco_conn_req(link->bd_addr, 8000, 8000, 5, 0x0360, 0, SCO_PKT_TYPES_D1);
                break;

            default:
                break;
            }
        }
    }
    else if (link->hfp_state == HSP_STATE_CONNECTED)
    {
        link->sco_setting = HFP_AG_SCO_SETTING_D1;
        gap_br_send_sco_conn_req(link->bd_addr, 8000, 8000, 12, 0x0360, 0, SCO_PKT_TYPES_D1);
    }
}

bool bt_hfp_ag_audio_connect_req(uint8_t bd_addr[6])
{
    T_BT_LINK *acl_link;

    acl_link = bt_link_find(bd_addr);
    if (acl_link != NULL)
    {
        if (acl_link->sco_state == BT_LINK_SCO_STATE_DISCONNECTED)
        {
            T_BT_HFP_AG_LINK *link;

            link = bt_hfp_ag_find_link_by_addr(bd_addr);
            if (link != NULL)
            {
                if ((link->hfp_state == HFP_STATE_CONNECTED) ||
                    (link->hfp_state == HSP_STATE_CONNECTED))
                {
                    link->sco_int_flag = true;
                    bt_hfp_ag_codec_negotiate(link);
                    return true;
                }
            }
        }
    }

    return false;
}

bool bt_hfp_ag_audio_connect_cfm(uint8_t bd_addr[6],
                                 bool    accept)
{
    T_BT_LINK *acl_link;

    acl_link = bt_link_find(bd_addr);
    if (acl_link != NULL)
    {
        if (acl_link->sco_state == BT_LINK_SCO_STATE_CONNECTING)
        {
            T_BT_HFP_AG_LINK *link;

            link = bt_hfp_ag_find_link_by_addr(bd_addr);
            if (link != NULL)
            {
                if (accept == true)
                {
                    if ((link->codec_type == CODEC_TYPE_MSBC) ||
                        (link->codec_type == CODEC_TYPE_LC3))
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
                        if (acl_link->is_esco == 0)
                        {
                            /*
                            * SCO link
                            * voice setting |= 0x0300 mean bypass MAC HW audio decoder/encoder
                            */
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
                            if (link->supported_features & HFP_AG_ESCO_S4_ENABLE)
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
        }
    }

    return false;
}

bool bt_hfp_ag_audio_disconnect_req(uint8_t bd_addr[6])
{
    return bt_sco_disconn_req(bd_addr);
}

bool bt_hfp_ag_send_ring(uint8_t bd_addr[6])
{
    char buf[9];

    snprintf(buf, 9, "\r\nRING\r\n");
    return bt_hfp_ag_cmd_send(bd_addr, buf);
}

bool bt_hfp_ag_send_clip(uint8_t     bd_addr[6],
                         const char *call_num,
                         uint8_t     call_num_type)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->clip_enable)
        {
            char buf[37];

            snprintf(buf, 37, "\r\n+CLIP: \"%s\",%u\r\n", call_num, call_num_type);
            return bt_hfp_ag_cmd_send(bd_addr, buf);
        }
    }

    return false;
}

void bt_hfp_ag_ringing_start(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            bt_hfp_ag_send_ring(link->bd_addr);
            bt_hfp_ag_send_clip(link->bd_addr,
                                link->call_num,
                                link->call_num_type);
            sys_timer_restart(link->timer_ring, link->ring_interval * 1000);
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            if ((link->supported_features & HFP_AG_INBAND_RINGTONE_ENABLE) == 0)
            {
                bt_hfp_ag_send_ring(link->bd_addr);
            }
            sys_timer_restart(link->timer_ring, link->ring_interval * 1000);
        }
    }
}

bool bt_hfp_ag_send_vnd_at_cmd_req(uint8_t     bd_addr[6],
                                   const char *at_cmd)
{
    return bt_hfp_ag_cmd_send(bd_addr, at_cmd);
}

void bt_hfp_ag_indicator_handle(uint8_t bd_addr[6],
                                uint8_t indicator_type)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        switch (indicator_type)
        {
        case AG_INDICATOR_TYPE_CALL:
            {
                if (link->ag_status_ind.call_status == HFP_CALL_STATUS_IDLE)
                {
                    if (link->curr_call_status != BT_HFP_AG_CALL_IDLE)
                    {
                        if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE)
                        {
                            bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_IDLE);
                        }
                        else if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD)
                        {
                            bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE);
                        }
                        else if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
                        {
                            bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_INCOMING);
                        }
                    }
                }
                else if (link->ag_status_ind.call_status == HFP_CALL_STATUS_ACTIVE)
                {
                    if (link->curr_call_status != BT_HFP_AG_CALL_ACTIVE)
                    {
                        bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE);
                    }
                }
            }
            break;

        case AG_INDICATOR_TYPE_CALLSETUP:
            {
                if (link->ag_status_ind.call_setup_status == HFP_CALL_SETUP_STATUS_IDLE)
                {
                    if ((link->curr_call_status == BT_HFP_AG_CALL_INCOMING) ||
                        (link->curr_call_status == BT_HFP_AG_CALL_OUTGOING))
                    {
                        bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_IDLE);
                    }
                    else if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
                    {
                        bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE);
                    }
                }
                else if (link->ag_status_ind.call_setup_status == HFP_CALL_SETUP_STATUS_INCOMING_CALL)
                {
                    if (link->curr_call_status == BT_HFP_AG_CALL_IDLE)
                    {
                        bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_INCOMING);
                    }
                    else if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE)
                    {
                        bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING);
                    }
                }
                else if ((link->ag_status_ind.call_setup_status == HFP_CALL_SETUP_STATUS_OUTGOING_CALL) ||
                         (link->ag_status_ind.call_setup_status == HFP_CALL_SETUP_STATUS_ALERTING))
                {
                    if (link->curr_call_status == BT_HFP_AG_CALL_IDLE)
                    {
                        bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_OUTGOING);
                    }
                }
            }
            break;

        case AG_INDICATOR_TYPE_CALLHELD:
            {
                if (link->ag_status_ind.call_held_status == HFP_CALL_HELD_STATUS_IDLE)
                {
                    if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD)
                    {
                        bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE);
                    }
                    else if (link->curr_call_status == BT_HFP_AG_CALL_HELD)
                    {
                        bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_IDLE);
                    }
                }
                else if (link->ag_status_ind.call_held_status ==
                         HFP_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL)
                {
                    if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE_WITH_CALL_WAITING)
                    {
                        bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE_WITH_CALL_HOLD);
                    }
                }
                else if (link->ag_status_ind.call_held_status ==
                         HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL)
                {
                    bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_HELD);
                }
            }
            break;
        }
    }
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
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        char buf[25];

        link->ag_status_ind.service_status = (T_HFP_SERVICE_STATUS)service_indicator;
        link->ag_status_ind.signal_status = signal_indicator;
        link->ag_status_ind.roam_status = (T_HFP_ROAM_STATUS)roaming_indicator;
        link->ag_status_ind.batt_chg_status = batt_chg_indicator;
        link->ag_status_ind.call_status = (T_HFP_CALL_STATUS)call_indicator;
        link->ag_status_ind.call_setup_status = (T_HFP_CALL_SETUP_STATUS)call_setup_indicator;
        link->ag_status_ind.call_held_status = (T_HFP_CALL_HELD_STATUS)call_held_indicator;

        snprintf(buf, 25, "\r\n+CIND: %u,%u,%u,%u,%u,%u,%u\r\n",
                 service_indicator,
                 call_indicator,
                 call_setup_indicator,
                 call_held_indicator,
                 signal_indicator,
                 roaming_indicator,
                 batt_chg_indicator);

        bt_hfp_ag_cmd_send(bd_addr, buf);
        bt_hfp_ag_indicator_handle(bd_addr, AG_INDICATOR_TYPE_CALL);
        bt_hfp_ag_indicator_handle(bd_addr, AG_INDICATOR_TYPE_CALLSETUP);
        bt_hfp_ag_indicator_handle(bd_addr, AG_INDICATOR_TYPE_CALLHELD);

        return true;
    }

    return false;
}

bool bt_hfp_ag_send_ciev(uint8_t bd_addr[6],
                         uint8_t ind_id,
                         uint8_t ind_value)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->cmer_enable)
        {
            char buf[15];

            if ((ind_id == AG_INDICATOR_TYPE_CALL) ||
                (ind_id == AG_INDICATOR_TYPE_CALLSETUP) ||
                (ind_id == AG_INDICATOR_TYPE_CALLHELD))
            {
                snprintf(buf, 15, "\r\n+CIEV: %u,%u\r\n", (uint8_t)(ind_id + 1), ind_value);
                return bt_hfp_ag_cmd_send(bd_addr, buf);
            }
            else
            {
                if (link->ag_indicator_mask & (1 << ind_id))
                {
                    snprintf(buf, 15, "\r\n+CIEV: %u,%u\r\n", (uint8_t)(ind_id + 1), ind_value);
                    return bt_hfp_ag_cmd_send(bd_addr, buf);
                }
            }

            return true;
        }
    }

    return false;
}

bool bt_hfp_ag_call_incoming(uint8_t     bd_addr[6],
                             const char *call_num,
                             uint8_t     call_num_len,
                             uint8_t     call_num_type)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if ((link->hfp_state == HFP_STATE_CONNECTED) ||
            (link->hfp_state == HSP_STATE_CONNECTED))
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_IDLE)
            {
                if (link->hfp_state == HFP_STATE_CONNECTED)
                {
                    link->ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_INCOMING_CALL;

                    memset(&link->call_num, 0, sizeof(link->call_num));
                    link->call_num_type = call_num_type;
                    if (call_num_len <= 20)
                    {
                        memcpy(link->call_num, call_num, call_num_len - 1);
                    }
                    else
                    {
                        memcpy(link->call_num, call_num, 19);
                    }

                    bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_INCOMING_CALL);
                }

                if (link->supported_features & HFP_AG_INBAND_RINGTONE_ENABLE)
                {
                    T_BT_LINK *acl_link;

                    acl_link = bt_link_find(bd_addr);
                    if (acl_link != NULL)
                    {
                        if (acl_link->sco_state == BT_LINK_SCO_STATE_DISCONNECTED)
                        {
                            link->sco_int_flag = true;
                            bt_hfp_ag_codec_negotiate(link);
                        }
                    }
                }
                else
                {
                    bt_hfp_ag_ringing_start(bd_addr);
                }

                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_INCOMING);
                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_answer(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        sys_timer_stop(link->timer_ring);

        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_INCOMING ||
                link->curr_call_status == BT_HFP_AG_CALL_OUTGOING)
            {
                link->ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_IDLE;
                bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALL, HFP_CALL_STATUS_ACTIVE);
                bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_IDLE);
                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE);

                return true;
            }
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_INCOMING ||
                link->curr_call_status == BT_HFP_AG_CALL_OUTGOING)
            {
                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE);

                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_hold(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE)
            {
                link->ag_status_ind.call_held_status = HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL;
                bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLHELD, HFP_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL);

                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_HELD);

                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_held_call_accept(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_HELD)
            {
                link->ag_status_ind.call_held_status = HFP_CALL_HELD_STATUS_IDLE;
                bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLHELD, HFP_CALL_HELD_STATUS_IDLE);
                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE);

                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_held_call_release(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_HELD)
            {
                link->ag_status_ind.call_held_status = HFP_CALL_HELD_STATUS_IDLE;
                bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLHELD, HFP_CALL_HELD_STATUS_IDLE);
                bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALL, HFP_CALL_STATUS_IDLE);
                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_IDLE);

                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_terminate(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        sys_timer_stop(link->timer_ring);
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_INCOMING ||
                link->curr_call_status == BT_HFP_AG_CALL_OUTGOING ||
                link->curr_call_status == BT_HFP_AG_CALL_ACTIVE)
            {
                if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE)
                {
                    bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALL, HFP_CALL_STATUS_IDLE);
                }
                else
                {
                    link->ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_IDLE;
                    bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_IDLE);
                }

                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_IDLE);
                return true;
            }
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_INCOMING ||
                link->curr_call_status == BT_HFP_AG_CALL_OUTGOING ||
                link->curr_call_status == BT_HFP_AG_CALL_ACTIVE)
            {
                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_IDLE);
                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_dial(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_IDLE)
            {
                link->ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_OUTGOING_CALL;
                bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_OUTGOING_CALL);
                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_OUTGOING);

                return true;
            }
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_IDLE)
            {
                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_OUTGOING);

                return true;
            }
        }
    }

    return false;
}

bool bt_hfp_ag_call_alert(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_OUTGOING)
            {
                link->ag_status_ind.call_setup_status = HFP_CALL_SETUP_STATUS_ALERTING;
                bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, HFP_CALL_SETUP_STATUS_ALERTING);
                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_OUTGOING);

                return true;
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
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED && link->ccwa_enable == true)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_ACTIVE ||
                link->curr_call_status == BT_HFP_AG_CALL_HELD)
            {
                char name[20] = {0};
                char buf[37];

                if (call_num_len <= 20)
                {
                    memcpy(name, call_num, call_num_len - 1);
                }
                else
                {
                    memcpy(name, call_num, 19);
                }

                snprintf(buf, 37, "\r\n+CCWA: \"%s\",%u\r\n", name, call_num_type);
                return bt_hfp_ag_cmd_send(bd_addr, buf);
            }
        }
    }

    return false;
}

bool bt_hfp_ag_microphone_gain_set(uint8_t bd_addr[6],
                                   uint8_t level)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED ||
            link->hfp_state == HSP_STATE_CONNECTED)
        {
            char buf[13];

            if (link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY)
            {
                if ((link->cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0)
                {
                    return false;
                }

                snprintf(buf, 13, "\r\n+VGM: %u\r\n", level);
            }
            else
            {
                snprintf(buf, 12, "\r\n+VGM=%u\r\n", level);
            }

            return bt_hfp_ag_cmd_send(bd_addr, buf);
        }
    }

    return false;
}

bool bt_hfp_ag_speaker_gain_set(uint8_t bd_addr[6],
                                uint8_t level)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED ||
            link->hfp_state == HSP_STATE_CONNECTED)
        {
            char buf[13];

            if (link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY)
            {
                if ((link->cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0)
                {
                    return false;
                }

                snprintf(buf, 13, "\r\n+VGS: %u\r\n", level);
            }
            else
            {
                snprintf(buf, 12, "\r\n+VGS=%u\r\n", level);
            }

            return bt_hfp_ag_cmd_send(bd_addr, buf);
        }
    }

    return false;
}

bool bt_hfp_ag_ring_interval_set(uint8_t  bd_addr[6],
                                 uint16_t interval)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (interval == 0)
        {
            link->ring_interval = HFP_AG_DEFAULT_RING_REPEAT_INTERVAL;
        }
        else
        {
            link->ring_interval = interval;
        }

        return true;
    }

    return false;
}

bool bt_hfp_ag_inband_ringing_set(uint8_t bd_addr[6],
                                  bool    enable)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if (bt_hfp_ag->brsf_cpbs & AG_CAPABILITY_INBAND_RINGING)
            {
                uint8_t value;
                char    buf[13];

                if (enable)
                {
                    link->supported_features |= HFP_AG_INBAND_RINGTONE_ENABLE;
                    value = 1;
                }
                else
                {
                    link->supported_features &= ~HFP_AG_INBAND_RINGTONE_ENABLE;
                    value = 0;
                }

                snprintf(buf, 13, "\r\n+BSIR: %u\r\n", value);
                return bt_hfp_ag_cmd_send(bd_addr, buf);
            }
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            if (enable)
            {
                link->supported_features |= HFP_AG_INBAND_RINGTONE_ENABLE;
            }
            else
            {
                link->supported_features &= ~HFP_AG_INBAND_RINGTONE_ENABLE;
            }

            return true;
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
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            char buf[45];

            if (call_num != NULL)
            {
                snprintf(buf, 45, "\r\n+CLCC: %u,%u,%u,%u,%u,%s,%u\r\n",
                         call_idx,
                         call_dir,
                         call_status,
                         call_mode,
                         mpty,
                         call_num,
                         call_num_type);
            }
            else
            {
                snprintf(buf, 45, "\r\n+CLCC: %u,%u,%u,%u,%u\r\n",
                         call_idx,
                         call_dir,
                         call_status,
                         call_mode,
                         mpty);
            }

            return bt_hfp_ag_cmd_send(bd_addr, buf);
        }
    }

    return false;
}

bool bt_hfp_ag_service_indicator_send(uint8_t                       bd_addr[6],
                                      T_BT_HFP_AG_SERVICE_INDICATOR status)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            return bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_SERVICE, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_call_indicator_send(uint8_t                    bd_addr[6],
                                   T_BT_HFP_AG_CALL_INDICATOR status)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            link->ag_status_ind.call_status = (T_HFP_CALL_STATUS)status;
            bt_hfp_ag_indicator_handle(bd_addr, AG_INDICATOR_TYPE_CALL);

            return bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALL, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_call_setup_indicator_send(uint8_t                          bd_addr[6],
                                         T_BT_HFP_AG_CALL_SETUP_INDICATOR status)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            link->ag_status_ind.call_setup_status = (T_HFP_CALL_SETUP_STATUS)status;
            bt_hfp_ag_indicator_handle(bd_addr, AG_INDICATOR_TYPE_CALLSETUP);

            return bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLSETUP, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_call_held_indicator_send(uint8_t                         bd_addr[6],
                                        T_BT_HFP_AG_CALL_HELD_INDICATOR status)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            link->ag_status_ind.call_held_status = (T_HFP_CALL_HELD_STATUS)status;
            bt_hfp_ag_indicator_handle(bd_addr, AG_INDICATOR_TYPE_CALLHELD);

            return bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_CALLHELD, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_signal_strength_send(uint8_t bd_addr[6],
                                    uint8_t value)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            return bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_SIGNAL, value);
        }
    }

    return false;
}

bool bt_hfp_ag_roaming_indicator_send(uint8_t                       bd_addr[6],
                                      T_BT_HFP_AG_ROAMING_INDICATOR status)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            return bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_ROAM, (uint8_t)status);
        }
    }

    return false;
}

bool bt_hfp_ag_battery_charge_send(uint8_t bd_addr[6],
                                   uint8_t value)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            return bt_hfp_ag_send_ciev(bd_addr, AG_INDICATOR_TYPE_BATTCHG, value);
        }
    }

    return false;
}

bool bt_hfp_ag_network_operator_name_send(uint8_t     bd_addr[6],
                                          const char *operator,
                                          uint8_t     operator_length)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED && link->cops_enable == true)
        {
            char name[17] = {0};
            char buf[35];

            if (operator_length <= 17)
            {
                memcpy(name, operator, operator_length - 1);
            }
            else
            {
                memcpy(name, operator, 16);
            }

            snprintf(buf, 35, "\r\n+COPS: ,3,0,\"%s\"\r\n", name);
            return bt_hfp_ag_cmd_send(bd_addr, buf);
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

    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            char num[20] = {0};
            char buf[41];

            if (call_num_len <= 20)
            {
                memcpy(num, call_num, call_num_len - 1);
            }
            else
            {
                memcpy(num, call_num, 19);
            }

            snprintf(buf, 41, "\r\n+CNUM: ,\"%s\",%u,,%u\r\n", num, call_num_type, service);
            return bt_hfp_ag_cmd_send(bd_addr, buf);
        }
    }

    return false;
}

bool bt_hfp_ag_voice_recognition_set(uint8_t bd_addr[6],
                                     bool    enable)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            if ((link->cpbs & HF_CAPABILITY_VOICE_RECOGNITION) &&
                (bt_hfp_ag->brsf_cpbs & AG_CAPABILITY_VOICE_RECOGNITION))
            {
                char    buf[13];
                uint8_t value;

                if (enable)
                {
                    value = 1;
                }
                else
                {
                    value = 0;
                }

                snprintf(buf, 13, "\r\n+BVRA: %u\r\n", value);
                return bt_hfp_ag_cmd_send(bd_addr, buf);
            }
        }
    }

    return false;
}

void bt_hfp_ag_handle_audio_conn_rsp(uint8_t bd_addr[6], uint16_t cause)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (cause)
        {
            link->sco_setting = HFP_AG_SCO_SETTING_ANY;
        }
    }
}

void bt_hfp_ag_handle_audio_conn_cmpl(uint8_t bd_addr[6], uint16_t cause)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_MSG_PAYLOAD payload;

        if ((cause) && (link->sco_setting != HFP_AG_SCO_SETTING_D1))
        {
            if (link->sco_int_flag)
            {
                bt_hfp_ag_codec_negotiate(link);
            }

            return;
        }

        link->sco_int_flag = false;
        link->sco_setting = HFP_AG_SCO_SETTING_ANY;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = &link->codec_type;
        bt_mgr_dispatch(BT_MSG_HFP_AG_CODEC_TYPE_SELECTED, &payload);

        if (link->curr_call_status == BT_HFP_AG_CALL_INCOMING)
        {
            if (link->supported_features & HFP_AG_INBAND_RINGTONE_ENABLE)
            {
                bt_hfp_ag_ringing_start(bd_addr);
                payload.msg_buf = NULL;
                bt_mgr_dispatch(BT_MSG_HFP_AG_INBAND_RINGING_REQ, &payload);
            }
        }

        if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            if (link->curr_call_status == BT_HFP_AG_CALL_IDLE)
            {
                bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_ACTIVE);
            }
        }
    }
}

void bt_hfp_ag_handle_audio_disconn(uint8_t bd_addr[6])
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            bt_hfp_ag_call_status_set(link, BT_HFP_AG_CALL_IDLE);
        }
    }
}

int32_t bt_hfp_ag_handle_brsf_set(T_BT_HFP_AG_LINK *link,
                                  const char       *str)
{
    int     res;
    int     offset;
    char    buf[17];
    int32_t ret = 0;

    res = sscanf(str, "AT+BRSF=%hu%n", &link->cpbs, &offset);
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

    snprintf(buf, 17, "\r\n+BRSF: %u\r\n", bt_hfp_ag->brsf_cpbs);
    bt_hfp_ag_cmd_send(link->bd_addr, buf);

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_bac_set(T_BT_HFP_AG_LINK *link,
                                 const char       *str)
{
    int      res;
    int      offset;
    uint8_t  codec_bitmap = 0;
    uint8_t  codec_type;
    int32_t  ret = 0;

    if (strncmp("AT+BAC=", str, 7))
    {
        ret = 1;
        goto fail_bac_cmd;
    }

    str += 7;
    while ((res = sscanf(str, "%hhu%n", &codec_type, &offset)) == 1)
    {
        codec_bitmap |= (1 << (codec_type - 1));

        str += offset;
        if (*str != ',')
        {
            break;
        }

        str++;
    }

    if (res < 1)
    {
        ret = 2;
        goto fail_bac_set_value;
    }

    if (*str != '\0')
    {
        ret = 3;
        goto fail_bac_set_end_part;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    if (link->supt_codecs != codec_bitmap)
    {
        link->codec_updated = true;
    }
    else
    {
        link->codec_updated = false;
    }

    link->supt_codecs = codec_bitmap;
    if (link->codec_type_sent != CODEC_TYPE_NONE)
    {
        link->codec_type_sent = CODEC_TYPE_NONE;
        bt_hfp_ag_codec_negotiate(link);
    }

    return 0;

fail_send_ok:
fail_bac_set_end_part:
fail_bac_set_value:
fail_bac_cmd:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_cind_test(T_BT_HFP_AG_LINK *link,
                                   const char       *str)
{
    int32_t ret = 0;

    if (strncmp("AT+CIND=?", str, 9))
    {
        ret = 1;
        goto fail_cind_test_cmd;
    }

    bt_hfp_ag_cmd_send(link->bd_addr, hfp_ag_cind_info);

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 2;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_cind_test_cmd:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_cind_read(T_BT_HFP_AG_LINK *link,
                                   const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    int32_t          ret = 0;

    if (strncmp("AT+CIND?", str, 8))
    {
        ret = 1;
        goto fail_cind_read_cmd;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;
    bt_mgr_dispatch(BT_MSG_HFP_AG_INDICATORS_STATUS_REQ, &payload);
    return 0;

fail_cind_read_cmd:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_nrec(T_BT_HFP_AG_LINK *link,
                              const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    int              res;
    int              offset;
    uint8_t          value;
    int32_t          ret = 0;

    res = sscanf(str, "AT+NREC=%hhu%n", &value, &offset);
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
    payload.msg_buf = &value;
    bt_mgr_dispatch(BT_MSG_HFP_AG_NREC_STATUS, &payload);
    return 0;

fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_cmer_set(T_BT_HFP_AG_LINK *link,
                                  const char       *str)
{
    int     res;
    int     offset;
    uint8_t mode;
    uint8_t keypad;
    uint8_t display;
    uint8_t cmer_enable;
    int32_t ret = 0;

    res = sscanf(str, "AT+CMER=%hhu,%hhu,%hhu,%hhu%n", &mode, &keypad, &display, &cmer_enable, &offset);
    if (res < 4)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (mode != 3 || keypad != 0 || display != 0)
    {
        ret = 2;
        goto fail_error_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 3;
        goto fail_end_part;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    link->cmer_enable = (cmer_enable == 1);

    return 0;

fail_send_ok:
fail_end_part:
fail_error_value:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_chld_test(T_BT_HFP_AG_LINK *link,
                                   const char       *str)
{
    char    buf[21];
    int32_t ret = 0;

    if (strncmp("AT+CHLD=?", str, 9))
    {
        ret = 1;
        goto fail_chld_chld_test;
    }

    snprintf(buf, 21, "\r\n+CHLD: %s\r\n", HFP_AG_CHLD_VAL);
    bt_hfp_ag_cmd_send(link->bd_addr, buf);

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 2;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_chld_chld_test:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_bind_set(T_BT_HFP_AG_LINK *link,
                                  const char       *str)
{
    int      res;
    int      offset;
    uint32_t hf_indicator_mask;
    int32_t  ret = 0;

    if (strncmp("AT+BIND=", str, 8))
    {
        ret = 1;
        goto fail_bind_set_cmd;
    }

    str += 8;
    while ((res = sscanf(str, "%u%n", &hf_indicator_mask, &offset)) == 1)
    {
        link->hf_indicator_mask |= (1 << (hf_indicator_mask - 1));

        str += offset;
        if (*str != ',')
        {
            break;
        }

        str++;
    }

    if (res < 1)
    {
        ret = 2;
        goto fail_bind_set_value;
    }

    if (*str != '\0')
    {
        ret = 3;
        goto fail_bind_set_end_part;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    link->hf_indicator_mask &= HFP_HF_SUPPORTED_INDICATORS_MASK;

    return 0;

fail_send_ok:
fail_bind_set_end_part:
fail_bind_set_value:
fail_bind_set_cmd:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_bind_test(T_BT_HFP_AG_LINK *link,
                                   const char       *str)
{
    char    buf[17];
    uint8_t i;
    int     offset = 10;
    int32_t ret = 0;

    if (strncmp("AT+BIND=?", str, 9))
    {
        ret = 1;
        goto fail_bind_test_cmd;
    }

    snprintf(buf, 11, "\r\n+BIND: (");

    if (HFP_HF_SUPPORTED_INDICATORS_MASK > 0)
    {
        for (i = 0; HFP_HF_SUPPORTED_INDICATORS_MASK >> i; i++)
        {
            if (HFP_HF_SUPPORTED_INDICATORS_MASK & (1 << i))
            {
                snprintf(buf + offset, 3, "%u,", (uint8_t)(i + 1));
                offset += 2;
            }
            else
            {
                snprintf(buf + offset, 2, ",");
                offset += 1;
            }
        }

        snprintf(buf + offset - 1, 4, ")\r\n");
    }
    else
    {
        snprintf(buf + offset, 4, ")\r\n");
    }

    bt_hfp_ag_cmd_send(link->bd_addr, buf);

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 2;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_bind_test_cmd:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_bind_read(T_BT_HFP_AG_LINK *link,
                                   const char        *str)
{
    uint8_t i;
    char    buf[15];
    int32_t ret = 0;

    if (strncmp("AT+BIND?", str, 8))
    {
        ret = 1;
        goto fail_bind_read_cmd;
    }

    for (i = 0; HFP_HF_SUPPORTED_INDICATORS_MASK >> i; i++)
    {
        bool hf_ind_mask_check = link->hf_indicator_mask & (1 << i);
        if (hf_ind_mask_check)
        {
            snprintf(buf, 15, "\r\n+BIND: %u,%u\r\n", (uint8_t)(i + 1), hf_ind_mask_check);
            bt_hfp_ag_cmd_send(link->bd_addr, buf);
        }
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 2;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_bind_read_cmd:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_bcs_set(T_BT_HFP_AG_LINK *link,
                                 const char       *str)
{
    int     res;
    int     offset;
    uint8_t codec_type;
    int32_t ret = 0;

    res = sscanf(str, "AT+BCS=%hhu%n", &codec_type, &offset);
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

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    if (link->codec_type_sent == codec_type)
    {
        link->codec_type = (T_HFP_CODEC_TYPE)codec_type;
        link->codec_type_sent = CODEC_TYPE_NONE;
        bt_hfp_ag_codec_negotiate(link);
    }
    else
    {
        link->codec_type_sent = CODEC_TYPE_NONE;
        bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    }

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_bcc(T_BT_HFP_AG_LINK *link,
                             const char       *str)
{
    if (bt_hfp_ag_ok_send(link->bd_addr) == true)
    {
        link->sco_int_flag = true;
        bt_hfp_ag_codec_negotiate(link);
        return 0;
    }

    return 1;
}

int32_t bt_hfp_ag_handle_ata(T_BT_HFP_AG_LINK *link,
                             const char       *str)
{
    if (bt_hfp_ag_ok_send(link->bd_addr) == true)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = NULL;
        bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_ANSWER_REQ, &payload);

        return 0;
    }

    return 1;
}

int32_t bt_hfp_ag_handle_chup(T_BT_HFP_AG_LINK *link,
                              const char       *str)
{
    if (bt_hfp_ag_ok_send(link->bd_addr) == true)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = NULL;
        bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_TERMINATE_REQ, &payload);

        return 0;
    }

    return 1;
}

int32_t bt_hfp_ag_handle_button_press(T_BT_HFP_AG_LINK *link,
                                      const char       *str)
{
    if (bt_hfp_ag_ok_send(link->bd_addr) == true)
    {
        T_BT_MSG_PAYLOAD payload;

        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = NULL;
        bt_mgr_dispatch(BT_MSG_HSP_AG_BUTTON_PRESS, &payload);

        return 0;
    }

    return 1;
}

int32_t bt_hfp_ag_handle_vgs_set(T_BT_HFP_AG_LINK *link,
                                 const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    uint8_t          volume;
    int              res;
    int              offset;
    int32_t          ret = 0;

    res = sscanf(str, "AT+VGS=%hhu%n", &volume, &offset);
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

    if ((link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY) &&
        ((link->cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0))
    {
        ret = 3;
        goto fail_support_cmd;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &volume;
    bt_mgr_dispatch(BT_MSG_HFP_AG_SPK_VOLUME_CHANGED, &payload);

    return 0;

fail_send_ok:
fail_support_cmd:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_vgm_set(T_BT_HFP_AG_LINK *link,
                                 const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    uint8_t          volume;
    int              res;
    int              offset;
    int32_t          ret = 0;

    res = sscanf(str, "AT+VGM=%hhu%n", &volume, &offset);
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

    if ((link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY) &&
        ((link->cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0))
    {
        ret = 3;
        goto fail_support_cmd;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &volume;
    bt_mgr_dispatch(BT_MSG_HFP_AG_MIC_VOLUME_CHANGED, &payload);

    return 0;

fail_send_ok:
fail_support_cmd:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_xapl_set(T_BT_HFP_AG_LINK *link,
                                  const char       *str)
{
    int     res = 0;
    int     offset;
    char    buf[20];
    int32_t ret = 0;

    res = sscanf(str, "AT+XAPL=%*4[^-]-%*4[^-]-%*4[^,],%u%n", &link->ag_hf_xapl_mask, &offset);
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

    snprintf(buf, 20, "\r\n+XAPL=iPhone,%u\r\n", HFP_HF_SUPPORTED_XAPL_FEATURES_MASK);
    link->ag_hf_xapl_mask &= HFP_HF_SUPPORTED_XAPL_FEATURES_MASK;
    if (bt_hfp_ag_cmd_send(link->bd_addr, buf) == false)
    {
        ret = 3;
        goto fail_send_xapl;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_send_xapl:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_iphoneaccev_set(T_BT_HFP_AG_LINK *link,
                                         const char       *str)
{
    int     res;
    int     offset;
    uint8_t i = 0;
    uint8_t xapl_num;
    uint8_t xapl_id;
    uint8_t xapl_value;
    int32_t ret = 0;

    res = sscanf(str, "AT+IPHONEACCEV=%hhu%n", &xapl_num, &offset);
    if (res < 0)
    {
        ret = 1;
        goto fail_iphoneaccev_num_get;
    }

    str += offset;
    while ((res = sscanf(str, ",%hhu,%hhu%n", &xapl_id, &xapl_value, &offset)) == 2)
    {
        switch (xapl_id)
        {
        case HFP_AG_HF_XAPL_BATTERY_STATUS_ID:
            {
                T_BT_MSG_PAYLOAD payload;
                uint8_t          battery_level;

                battery_level = (xapl_value + 1) * 10;
                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = &battery_level;
                bt_mgr_dispatch(BT_MSG_HFP_AG_BATTERY_LEVEL, &payload);
            }
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

        i++;
        str += offset;
        if (*str != ',')
        {
            break;
        }
    }

    if (res < 2)
    {
        ret = 2;
        goto fail_iphoneaccev_value;
    }

    if (i != xapl_num)
    {
        ret = 3;
        goto fail_iphoneaccev_oversize;
    }

    if (*str != '\0')
    {
        ret = 4;
        goto fail_iphoneaccev_end_part;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 5;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_iphoneaccev_end_part:
fail_iphoneaccev_oversize:
fail_iphoneaccev_value:
fail_iphoneaccev_num_get:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_xevent_set(T_BT_HFP_AG_LINK *link,
                                    const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    uint8_t          battery_level;
    int              res;
    int              offset;
    uint16_t         level;
    uint16_t         level_num;
    int32_t          ret = 0;

    res = sscanf(str, "AT+XEVENT=BATTERY,%hu,%hu,%*u,%*u%n", &level, &level_num, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (level_num == 0)
    {
        ret = 2;
        goto fail_invalid_level_num;
    }

    if (*(str + offset) != '\0')
    {
        ret = 3;
        goto fail_end_part;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    battery_level = (level * 100) / level_num;
    payload.msg_buf = &battery_level;
    bt_mgr_dispatch(BT_MSG_HFP_AG_BATTERY_LEVEL, &payload);

    return 0;

fail_send_ok:
fail_end_part:
fail_invalid_level_num:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_biev_set(T_BT_HFP_AG_LINK *link,
                                  const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    int              res;
    int              offset;
    uint16_t         ind_id;
    uint32_t         ind_value;
    int32_t          ret = 0;

    res = sscanf(str, "AT+BIEV=%hu,%u%n", &ind_id, &ind_value, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (ind_id > HFP_HF_IND_BATTERY_LEVEL_STATUS_ID)
    {
        ret = 2;
        goto fail_invalid_id;
    }

    if (*(str + offset) != '\0')
    {
        ret = 3;
        goto fail_end_part;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &ind_value;
    if (ind_id == HFP_HF_IND_ENHANCED_SAFETY_STATUS_ID)
    {
        bt_mgr_dispatch(BT_MSG_HFP_AG_ENHANCED_SAFETY_STATUS, &payload);
    }
    else if (ind_id == HFP_HF_IND_BATTERY_LEVEL_STATUS_ID)
    {
        bt_mgr_dispatch(BT_MSG_HFP_AG_BATTERY_LEVEL, &payload);
    }

    return 0;

fail_send_ok:
fail_end_part:
fail_invalid_id:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_bia_set(T_BT_HFP_AG_LINK *link,
                                 const char       *str)
{
    int     res;
    int     offset = 0;
    uint8_t i = 0;
    uint8_t value;
    int32_t ret = 0;

    str += 7;
    while ((res = sscanf(str, "%hhu%n", &value, &offset)) >= 0)
    {
        if (res >= 0)
        {
            if (res == 1)
            {
                if (value == 1)
                {
                    link->ag_indicator_mask |= (1 << i);
                }
                else if (value == 0)
                {
                    link->ag_indicator_mask &= ~(1 << i);
                }
                else
                {
                    ret = 1;
                    goto fail_bia_invalid_value;
                }
            }

            i++;

            str += offset;
            offset = 0;
            if (*str != ',')
            {
                break;
            }

            str++;
        }
    }

    if (res < 1)
    {
        ret = 2;
        goto fail_bia_value_get;
    }

    if (i > MAX_AG_INDICATOR_COUNT)
    {
        ret = 3;
        goto fail_bia_oversize;
    }

    if (*str != '\0')
    {
        ret = 4;
        goto fail_bia_set_end_part;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 5;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_bia_set_end_part:
fail_bia_oversize:
fail_bia_value_get:
fail_bia_invalid_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_cmee_set(T_BT_HFP_AG_LINK *link,
                                  const char       *str)
{
    int     res;
    int     offset;
    uint8_t cmee_enable;
    int32_t ret = 0;

    res = sscanf(str, "AT+CMEE=%hhu%n", &cmee_enable, &offset);
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

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    link->cmee_enable = (cmee_enable == 1);

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_clip_set(T_BT_HFP_AG_LINK *link,
                                  const char       *str)
{
    int     res;
    int     offset;
    uint8_t clip_enable;
    int32_t ret = 0;

    res = sscanf(str, "AT+CLIP=%hhu%n", &clip_enable, &offset);
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

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    link->clip_enable = (clip_enable == 1);

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_clcc(T_BT_HFP_AG_LINK *link,
                              const char       *str)
{
    T_BT_MSG_PAYLOAD payload;

    if (*(str + 7) != '\0')
    {
        bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
        return 1;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;
    bt_mgr_dispatch(BT_MSG_HFP_AG_CURR_CALLS_LIST_QUERY, &payload);
    return 0;
}

int32_t bt_hfp_ag_handle_atd(T_BT_HFP_AG_LINK *link,
                             const char       *str)
{
    T_BT_HFP_AG_DIAL_WITH_NUMBER dial_with_number;
    T_BT_MSG_PAYLOAD             payload;
    int                          res;
    int                          offset;
    int32_t                      ret = 0;

    memset(&dial_with_number, 0, sizeof(dial_with_number));

    res = sscanf(str, "ATD%20[^;];%n", dial_with_number.number, &offset);
    if ((res < 1) || (offset <= 0))
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &dial_with_number;
    bt_mgr_dispatch(BT_MSG_HFP_AG_MSG_DIAL_WITH_NUMBER, &payload);

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_ext_atd(T_BT_HFP_AG_LINK *link,
                                 const char       *str)
{
    T_BT_HFP_AG_DIAL_WITH_MEMORY msg;
    T_BT_MSG_PAYLOAD             payload;
    int                          res;
    int                          offset;
    int32_t                      ret = 0;

    res = sscanf(str, "ATD>%hhu;%n", &msg.num, &offset);
    if ((res < 1) || (offset <= 0))
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
    payload.msg_buf = &msg;
    bt_mgr_dispatch(BT_MSG_HFP_AG_MSG_DIAL_WITH_MEMORY, &payload);

    return 0;

fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_bldn(T_BT_HFP_AG_LINK *link,
                              const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    int32_t          ret = 0;

    if (*(str + 7) != '\0')
    {
        ret = 1;
        goto fail_end_part;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;
    bt_mgr_dispatch(BT_MSG_HFP_AG_MSG_DIAL_LAST_NUMBER, &payload);

    return 0;

fail_end_part:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_chld(T_BT_HFP_AG_LINK *link,
                              const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    uint8_t          value;
    int              res;
    int              offset;
    int32_t          ret = 0;

    res = sscanf(str, "AT+CHLD=%hhu%n", &value, &offset);
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
    payload.msg_buf = NULL;
    switch (value)
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
        bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
        break;
    }

    return 0;

fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_ccwa(T_BT_HFP_AG_LINK *link,
                              const char       *str)
{
    int     res;
    int     offset;
    uint8_t ccwa_enable;
    int32_t ret = 0;

    res = sscanf(str, "AT+CCWA=%hhu%n", &ccwa_enable, &offset);
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

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    link->ccwa_enable = (ccwa_enable == 1);

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_cnum(T_BT_HFP_AG_LINK *link,
                              const char       *str)
{
    T_BT_MSG_PAYLOAD payload;

    if (*(str + 7) != '\0')
    {
        bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
        return 1;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;
    bt_mgr_dispatch(BT_MSG_HFP_AG_SUBSCRIBER_NUMBER_QUERY, &payload);

    return 0;
}

int32_t bt_hfp_ag_handle_vts(T_BT_HFP_AG_LINK *link,
                             const char       *str)
{
    T_BT_HFP_AG_CALL_DTMF_CODE msg;
    T_BT_MSG_PAYLOAD           payload;
    int                        res;
    int                        offset;
    int32_t                    ret = 0;

    res = sscanf(str, "AT+VTS=%c%n", &msg.dtmf_code, &offset);
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

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = &msg;
    bt_mgr_dispatch(BT_MSG_HFP_AG_MSG_DTMF_CODE, &payload);

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_cops_set(T_BT_HFP_AG_LINK *link,
                                  const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    int32_t          ret = 0;

    if (strncmp("AT+COPS=3,0", str, 11))
    {
        ret = 1;
        goto fail_value_get;
    }

    if (*(str + 11) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if (bt_hfp_ag_ok_send(link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    link->cops_enable = true;
    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;
    bt_mgr_dispatch(BT_MSG_HFP_AG_NETWORK_NAME_FORMAT_SET, &payload);

    return 0;

fail_send_ok:
fail_end_part:
fail_value_get:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_bvra(T_BT_HFP_AG_LINK *link,
                              const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    int              res;
    int              offset;
    uint8_t          enable;
    int32_t          ret = 0;

    res = sscanf(str, "AT+BVRA=%hhu%n", &enable, &offset);
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
    payload.msg_buf = NULL;
    if (enable == 1)
    {
        bt_mgr_dispatch(BT_MSG_HFP_AG_VOICE_RECOGNITION_ACTIVATION, &payload);
    }
    else if (enable == 0)
    {
        bt_mgr_dispatch(BT_MSG_HFP_AG_VOICE_RECOGNITION_DEACTIVATION, &payload);
    }

    return ret;

fail_end_part:
fail_no_value:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t bt_hfp_ag_handle_cops_read(T_BT_HFP_AG_LINK *link,
                                   const char       *str)
{
    T_BT_MSG_PAYLOAD payload;
    int32_t          ret = 0;

    if (link->cops_enable == false)
    {
        ret = 1;
        goto fail_cops_enable;
    }

    if (*(str + 8) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;
    bt_mgr_dispatch(BT_MSG_HFP_AG_CURR_OPERATOR_QUERY, &payload);

    return 0;

fail_end_part:
fail_cops_enable:
    bt_hfp_ag_error_send(link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

void bt_hfp_ag_timeout_cback(T_SYS_TIMER_HANDLE handle)
{
    T_BT_HFP_AG_LINK *link;

    link = (void *)sys_timer_id_get(handle);
    if (os_queue_search(&bt_hfp_ag->link_list, link) == true)
    {
        if (link->hfp_state == HFP_STATE_CONNECTED)
        {
            bt_hfp_ag_ringing_start(link->bd_addr);
            if (link->supported_features & HFP_AG_INBAND_RINGTONE_ENABLE)
            {
                T_BT_MSG_PAYLOAD payload;

                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = NULL;
                bt_mgr_dispatch(BT_MSG_HFP_AG_INBAND_RINGING_REQ, &payload);
            }
        }
        else if (link->hfp_state == HSP_STATE_CONNECTED)
        {
            bt_hfp_ag_ringing_start(link->bd_addr);
            if (link->supported_features & HFP_AG_INBAND_RINGTONE_ENABLE)
            {
                T_BT_MSG_PAYLOAD payload;

                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = NULL;
                bt_mgr_dispatch(BT_MSG_HSP_AG_INBAND_RINGING_REQ, &payload);
            }
        }
    }
}

void bt_hfp_ag_srv_level_established(T_BT_HFP_AG_LINK *link)
{
    T_BT_MSG_PAYLOAD payload;

    memcpy(payload.bd_addr, link->bd_addr, 6);
    link->ring_interval = HFP_AG_DEFAULT_RING_REPEAT_INTERVAL;
    link->codec_type = CODEC_TYPE_CVSD;
    link->sco_setting = HFP_AG_SCO_SETTING_ANY;
    bt_pm_cback_register(link->bd_addr, bt_hfp_ag_pm_cback);

    if (link->uuid == UUID_HEADSET_AUDIO_GATEWAY)
    {
        link->hfp_state = HSP_STATE_CONNECTED;
        bt_mgr_dispatch(BT_MSG_HSP_AG_CONN_CMPL, &payload);
    }
    else
    {
        link->hfp_state = HFP_STATE_CONNECTED;

        if (bt_hfp_ag->brsf_cpbs & AG_CAPABILITY_INBAND_RINGING)
        {
            link->supported_features |= HFP_AG_INBAND_RINGTONE_ENABLE;
        }

        if ((link->cpbs & HF_CAPABILITY_ESCO_S4_T2_SUPPORTED) &&
            (bt_hfp_ag->brsf_cpbs & AG_CAPABILITY_ESCO_S4_T2_SUPPORTED))
        {
            link->supported_features |= HFP_AG_ESCO_S4_ENABLE;
        }

        if ((link->cpbs & HF_CAPABILITY_CODEC_NEGOTIATION) &&
            (bt_hfp_ag->brsf_cpbs & AG_CAPABILITY_CODEC_NEGOTIATION))
        {
            link->supported_features |= HFP_AG_CODEC_NEGOTIATION_ENABLE;
        }

        bt_mgr_dispatch(BT_MSG_HFP_AG_CONN_CMPL, &payload);

        payload.msg_buf = &link->cpbs;
        bt_mgr_dispatch(BT_MSG_HFP_AG_SUPPORTED_FEATURES, &payload);
    }

    if (link->curr_call_status != link->prev_call_status)
    {
        T_BT_HFP_AG_CALL_STATUS_INFO info;

        info.prev_call_status = link->prev_call_status;
        info.curr_call_status = link->curr_call_status;
        payload.msg_buf = &info;
        bt_mgr_dispatch(BT_MSG_HFP_AG_CALL_STATUS_CHANGED, &payload);
    }
}

void bt_hfp_ag_srv_level_step(T_BT_HFP_AG_LINK *link,
                              const char       *str,
                              bool              result_ok)
{
    int32_t ret = 0;

    switch (link->service_level_step)
    {
    case HFP_SRV_LEVEL_NONE:
        {
            memset(&link->cmd_queue, 0, sizeof(link->cmd_queue));
            link->service_level_step = HFP_SRV_LEVEL_BRSF;
        }
        break;

    case HFP_SRV_LEVEL_BRSF: /* mandatory */
        {
            ret = bt_hfp_ag_handle_brsf_set(link, str);
            if (ret == 0)
            {
                if ((link->cpbs & HF_CAPABILITY_CODEC_NEGOTIATION) &&
                    (bt_hfp_ag->brsf_cpbs & AG_CAPABILITY_CODEC_NEGOTIATION))
                {
                    link->service_level_step = HFP_SRV_LEVEL_BAC;
                }
                else
                {
                    link->service_level_step = HFP_SRV_LEVEL_CIND_TEST;
                }
            }
        }
        break;

    case HFP_SRV_LEVEL_BAC: /* optional */
        {
            ret = bt_hfp_ag_handle_bac_set(link, str);
            if (ret == 0)
            {
                link->service_level_step = HFP_SRV_LEVEL_CIND_TEST;
            }
        }
        break;

    case HFP_SRV_LEVEL_CIND_TEST: /* mandatory */
        {
            ret = bt_hfp_ag_handle_cind_test(link, str);
            if (ret == 0)
            {
                link->service_level_step = HFP_SRV_LEVEL_CIND_INQUIRY;
            }
        }
        break;

    case HFP_SRV_LEVEL_CIND_INQUIRY: /* mandatory */
        /* The AT+CIND? read command is used to get current status of the AG indicators. */
        {
            ret = bt_hfp_ag_handle_cind_read(link, str);
            if (ret == 0)
            {
                link->service_level_step = HFP_SRV_LEVEL_ACTIVATE_INDICATOR;
            }
        }
        break;

    case HFP_SRV_LEVEL_ACTIVATE_INDICATOR: /* mandatory */
        /*
         * AT+CMER=3,0,0,1 activates indicator events reporting.
         * AT+CMER=3,0,0,0 deactivates indicator events reporting.
         */
        {
            ret = bt_hfp_ag_handle_cmer_set(link, str);
            if (ret == 0)
            {
                if ((link->cpbs & HF_CAPABILITY_CALL_WAITING_OR_3WAY) &&
                    (bt_hfp_ag->brsf_cpbs & AG_CAPABILITY_3WAY))
                {
                    link->service_level_step = HFP_SRV_LEVEL_CHLD_TEST;
                }
                else if ((link->cpbs & HF_CAPABILITY_HF_INDICATORS) &&
                         (bt_hfp_ag->brsf_cpbs & AG_CAPABILITY_HF_INDICATORS))
                {
                    link->service_level_step = HFP_SRV_LEVEL_BIND;
                }
                else
                {
                    link->service_level_step = HFP_SRV_LEVEL_ESTABLISHED;
                    bt_hfp_ag_srv_level_established(link);
                }
            }
        }
        break;

    case HFP_SRV_LEVEL_CHLD_TEST: /* optional, after this ServiceLevelConnection is Established */
        {
            /* AT+CHLD=? get call hold capabilities */
            ret = bt_hfp_ag_handle_chld_test(link, str);
            if (ret == 0)
            {
                if ((link->cpbs & HF_CAPABILITY_HF_INDICATORS) &&
                    (bt_hfp_ag->brsf_cpbs & AG_CAPABILITY_HF_INDICATORS))
                {
                    link->service_level_step = HFP_SRV_LEVEL_BIND;
                }
                else
                {
                    link->service_level_step = HFP_SRV_LEVEL_ESTABLISHED;
                    bt_hfp_ag_srv_level_established(link);
                }
            }
        }
        break;

    case HFP_SRV_LEVEL_BIND:
        {
            ret = bt_hfp_ag_handle_bind_set(link, str);
            if (ret == 0)
            {
                link->service_level_step = HFP_SRV_LEVEL_BIND_TEST;
            }
        }
        break;

    case HFP_SRV_LEVEL_BIND_TEST:
        {
            ret = bt_hfp_ag_handle_bind_test(link, str);
            if (ret == 0)
            {
                link->service_level_step = HFP_SRV_LEVEL_BIND_INQUIRY;
            }
        }
        break;

    case HFP_SRV_LEVEL_BIND_INQUIRY:
        {
            ret = bt_hfp_ag_handle_bind_read(link, str);
            if (ret == 0)
            {
                link->service_level_step = HFP_SRV_LEVEL_ESTABLISHED;
                bt_hfp_ag_srv_level_established(link);
            }
        }
        break;

    default:
        break;
    }

    if (ret != 0)
    {
        PROFILE_PRINT_ERROR2("bt_hfp_ag_srv_level_step: step %d, failed %d",
                             link->service_level_step, -ret);
    }
}

void bt_hfp_ag_srv_level_continue(T_BT_HFP_AG_LINK *link,
                                  const char       *str,
                                  bool              result_ok)
{
    if (link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY)
    {
        bt_hfp_ag_srv_level_step(link, str, result_ok);
    }
    else
    {
        bt_hfp_ag_srv_level_established(link);
    }
}

void bt_hfp_ag_srv_level_start(T_BT_HFP_AG_LINK *link)
{
    link->service_level_step = HFP_SRV_LEVEL_NONE;

    bt_hfp_ag_srv_level_continue(link, NULL, true);
}

void bt_hfp_ag_handle_at_cmd(T_BT_HFP_AG_LINK *link,
                             const char    *hf_at_cmd)
{
    char    *at_cmd;
    int32_t  ret = 0;

    if (link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTING)
    {
        bt_hfp_ag_srv_level_continue(link, hf_at_cmd, true);
    }
    else
    {
        if (!strncmp("AT+BCS=", hf_at_cmd, 7))
        {
            at_cmd = "AT+BCS=";
            ret = bt_hfp_ag_handle_bcs_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+BAC=", hf_at_cmd, 7))
        {
            at_cmd = "AT+BAC=";
            ret = bt_hfp_ag_handle_bac_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+BCC", hf_at_cmd, 6))
        {
            at_cmd = "AT+BCC=";
            ret = bt_hfp_ag_handle_bcc(link, hf_at_cmd);
        }
        else if (!strncmp("ATA", hf_at_cmd, 3))
        {
            at_cmd = "ATA";
            ret = bt_hfp_ag_handle_ata(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CHUP", hf_at_cmd, 7))
        {
            at_cmd = "AT+CHUP=";
            ret = bt_hfp_ag_handle_chup(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CKPD=200", hf_at_cmd, 11))
        {
            at_cmd = "AT+CKPD=200";
            ret = bt_hfp_ag_handle_button_press(link, hf_at_cmd);
        }
        else if (!strncmp("AT+VGS=", hf_at_cmd, 7))
        {
            at_cmd = "AT+VGS=";
            ret = bt_hfp_ag_handle_vgs_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+VGM=", hf_at_cmd, 7))
        {
            at_cmd = "AT+VGM=";
            ret = bt_hfp_ag_handle_vgm_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+XAPL=", hf_at_cmd, 8))
        {
            at_cmd = "AT+XAPL=";
            ret = bt_hfp_ag_handle_xapl_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+IPHONEACCEV=", hf_at_cmd, 15))
        {
            at_cmd = "AT+IPHONEACCEV=";
            ret = bt_hfp_ag_handle_iphoneaccev_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+XEVENT=", hf_at_cmd, 10))
        {
            at_cmd = "AT+XEVENT=";
            ret = bt_hfp_ag_handle_xevent_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+BIEV=", hf_at_cmd, 8))
        {
            at_cmd = "AT+BIEV=";
            ret = bt_hfp_ag_handle_biev_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+BIA=", hf_at_cmd, 7))
        {
            at_cmd = "AT+BIA=";
            ret = bt_hfp_ag_handle_bia_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CMEE=", hf_at_cmd, 8))
        {
            at_cmd = "AT+CMEE=";
            ret = bt_hfp_ag_handle_cmee_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CLIP=", hf_at_cmd, 8))
        {
            at_cmd = "AT+CLIP=";
            ret = bt_hfp_ag_handle_clip_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CLCC", hf_at_cmd, 7))
        {
            at_cmd = "AT+CLCC";
            ret = bt_hfp_ag_handle_clcc(link, hf_at_cmd);
        }
        else if (!strncmp("ATD>", hf_at_cmd, 4))
        {
            at_cmd = "ATD>";
            ret = bt_hfp_ag_handle_ext_atd(link, hf_at_cmd);
        }
        else if (!strncmp("ATD", hf_at_cmd, 3))
        {
            at_cmd = "ATD";
            ret = bt_hfp_ag_handle_atd(link, hf_at_cmd);
        }
        else if (!strncmp("AT+BLDN", hf_at_cmd, 7))
        {
            at_cmd = "AT+BLDN";
            ret = bt_hfp_ag_handle_bldn(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CHLD", hf_at_cmd, 7))
        {
            at_cmd = "AT+CHLD";
            ret = bt_hfp_ag_handle_chld(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CCWA", hf_at_cmd, 7))
        {
            at_cmd = "AT+CCWA";
            ret = bt_hfp_ag_handle_ccwa(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CNUM", hf_at_cmd, 7))
        {
            at_cmd = "AT+CNUM";
            ret = bt_hfp_ag_handle_cnum(link, hf_at_cmd);
        }
        else if (!strncmp("AT+COPS=", hf_at_cmd, 8))
        {
            at_cmd = "AT+COPS=";
            ret = bt_hfp_ag_handle_cops_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+COPS?", hf_at_cmd, 8))
        {
            at_cmd = "AT+COPS?";
            ret = bt_hfp_ag_handle_cops_read(link, hf_at_cmd);
        }
        else if (!strncmp("AT+VTS=", hf_at_cmd, 7))
        {
            at_cmd = "AT+VTS=";
            ret = bt_hfp_ag_handle_vts(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CMER=", hf_at_cmd, 8))
        {
            at_cmd = "AT+CMER=";
            ret = bt_hfp_ag_handle_cmer_set(link, hf_at_cmd);
        }
        else if (!strncmp("AT+CIND?", hf_at_cmd, 8))
        {
            at_cmd = "AT+CIND?";
            ret = bt_hfp_ag_handle_cind_read(link, hf_at_cmd);
        }
        else if (!strncmp("AT+NREC=", hf_at_cmd, 8))
        {
            at_cmd = "AT+NREC=";
            ret = bt_hfp_ag_handle_nrec(link, hf_at_cmd);
        }
        else if (!strncmp("AT+BVRA", hf_at_cmd, 7))
        {
            at_cmd = "AT+BVRA";
            ret = bt_hfp_ag_handle_bvra(link, hf_at_cmd);
        }
        else /* unkown AT command*/
        {
            T_BT_MSG_PAYLOAD payload;

            memcpy(payload.bd_addr, link->bd_addr, 6);
            payload.msg_buf = (void *)hf_at_cmd;
            bt_mgr_dispatch(BT_MSG_HFP_AG_VENDOR_CMD, &payload);
        }

        if (ret != 0)
        {
            PROFILE_PRINT_ERROR2("bt_hfp_ag_handle_at_cmd: %s, failed %d", TRACE_STRING(at_cmd), -ret);
        }
    }
}

void bt_hfp_ag_handle_rfc_conn_cmpl(uint8_t  service_id,
                                    uint8_t  bd_addr[6],
                                    uint8_t  dlci,
                                    uint8_t  credits,
                                    uint16_t frame_size)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (service_id == bt_hfp_ag->hfp_service_id)
        {
            link->uuid = UUID_HANDSFREE_AUDIO_GATEWAY;
        }
        else if (service_id == bt_hfp_ag->hsp_service_id)
        {
            link->uuid = UUID_HEADSET_AUDIO_GATEWAY;
        }

        link->frame_size = frame_size;

        if (credits == 0)
        {
            link->hfp_state = HFP_STATE_RFC_CONN_NO_CREDITS;
        }
        else
        {
            link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
            link->credits = credits;
            bt_hfp_ag_srv_level_start(link);
        }

        rfc_data_cfm(bd_addr, dlci, RFC_DEFAULT_CREDIT);
    }
    else
    {
        rfc_disconn_req(bd_addr, dlci);
    }
}

void bt_hfp_ag_handle_rfc_data_ind(T_RFC_DATA_IND *data_msg)
{
    T_BT_HFP_AG_LINK *link;

    link = bt_hfp_ag_find_link_by_addr(data_msg->bd_addr);
    if (link != NULL)
    {
        link->credits = data_msg->remain_credits;
        if (link->hfp_state == HFP_STATE_RFC_CONN_NO_CREDITS && data_msg->remain_credits > 0)
        {
            link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
            bt_hfp_ag_srv_level_start(link);
        }

        if (data_msg->length != 0)
        {
            uint8_t  *data;
            uint8_t  *temp_ptr = NULL;
            uint16_t  length;

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
                    bt_hfp_ag_handle_at_cmd(link, (const char *)temp_ptr);
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
    }
}

void bt_hfp_ag_handle_rfc_msg(T_RFC_MSG_TYPE  msg_type,
                              void           *msg)
{
    switch (msg_type)
    {
    case RFC_CONN_IND:
        {
            T_RFC_CONN_IND    *conn_ind_msg;
            T_BT_HFP_AG_LINK  *link;

            conn_ind_msg = (T_RFC_CONN_IND *)msg;
            link = bt_hfp_ag_find_link_by_addr(conn_ind_msg->bd_addr);
            if (link == NULL)
            {
                link = bt_hfp_ag_alloc_link(conn_ind_msg->bd_addr);
                if (link == NULL)
                {
                    rfc_conn_cfm(conn_ind_msg->bd_addr, conn_ind_msg->dlci, RFC_REJECT,
                                 conn_ind_msg->frame_size, RFC_DEFAULT_CREDIT);
                }
                else
                {
                    T_BT_MSG_PAYLOAD payload;

                    link->dlci = conn_ind_msg->dlci;
                    link->ag_indicator_mask = HFP_AG_IND_ALL_MASK;
                    link->ag_hf_xapl_mask = HFP_HF_SUPPORTED_XAPL_FEATURES_MASK;
                    link->frame_size = conn_ind_msg->frame_size;
                    memcpy(payload.bd_addr, conn_ind_msg->bd_addr, 6);
                    payload.msg_buf = NULL;
                    bt_mgr_dispatch(BT_MSG_HFP_AG_CONN_IND, &payload);
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
            T_RFC_CONN_CMPL *complete_msg;

            complete_msg = (T_RFC_CONN_CMPL *)msg;
            bt_hfp_ag_handle_rfc_conn_cmpl(complete_msg->service_id,
                                           complete_msg->bd_addr,
                                           complete_msg->dlci,
                                           complete_msg->remain_credits,
                                           complete_msg->frame_size);
        }
        break;

    case RFC_DISCONN_CMPL:
        {
            T_RFC_DISCONN_CMPL *disc_msg;
            T_BT_HFP_AG_LINK   *link;

            disc_msg = (T_RFC_DISCONN_CMPL *)msg;
            link = bt_hfp_ag_find_link_by_addr(disc_msg->bd_addr);
            if (link != NULL)
            {
                T_BT_HFP_AG_DISCONN_INFO info;
                T_BT_MSG_PAYLOAD         payload;
                uint16_t                 uuid;

                uuid = link->uuid;
                bt_pm_cback_unregister(link->bd_addr, bt_hfp_ag_pm_cback);
                bt_hfp_ag_free_link(link);

                memcpy(payload.bd_addr, disc_msg->bd_addr, 6);
                info.cause = disc_msg->cause;
                payload.msg_buf = &info;
                if (uuid == UUID_HEADSET_AUDIO_GATEWAY)
                {
                    bt_mgr_dispatch(BT_MSG_HSP_AG_DISCONN_CMPL, &payload);
                }
                else
                {
                    bt_mgr_dispatch(BT_MSG_HFP_AG_DISCONN_CMPL, &payload);
                }
            }
        }
        break;

    case RFC_CREDIT_INFO:
        {
            T_RFC_CREDIT_INFO *credit_msg;
            T_BT_HFP_AG_LINK  *link;

            credit_msg = (T_RFC_CREDIT_INFO *)msg;
            link = bt_hfp_ag_find_link_by_addr(credit_msg->bd_addr);
            if (link != NULL)
            {
                link->credits = credit_msg->remain_credits;
                if (link->hfp_state == HFP_STATE_RFC_CONN_NO_CREDITS && credit_msg->remain_credits > 0)
                {
                    link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
                    bt_hfp_ag_srv_level_start(link);
                }
                else if (link->hfp_state >= HFP_STATE_SRV_LEVEL_CONNECTING && credit_msg->remain_credits > 0)
                {
                    bt_hfp_ag_cmd_flush(link);
                }
            }
        }
        break;

    case RFC_DATA_IND:
        bt_hfp_ag_handle_rfc_data_ind((T_RFC_DATA_IND *)msg);
        break;

    default:
        break;
    }
}

bool bt_hfp_ag_init(uint8_t  rfc_hfp_chann_num,
                    uint8_t  rfc_hsp_chann_num,
                    uint16_t supported_features,
                    uint8_t  supported_codecs)
{
    int32_t ret = 0;

    bt_hfp_ag = calloc(1, sizeof(T_BT_HFP_AG));
    if (bt_hfp_ag == NULL)
    {
        ret = 1;
        goto fail_alloc_hfp;
    }
    if (rfc_cback_register(rfc_hfp_chann_num,
                           bt_hfp_ag_handle_rfc_msg,
                           &bt_hfp_ag->hfp_service_id) == false)
    {
        ret = 2;
        goto fail_reg_hfp_ag_rfc_cb;
    }

    if (rfc_cback_register(rfc_hsp_chann_num,
                           bt_hfp_ag_handle_rfc_msg,
                           &bt_hfp_ag->hsp_service_id) == false)
    {
        ret = 3;
        goto fail_reg_hsp_ag_rfc_cb;
    }

    bt_hfp_ag->brsf_cpbs = supported_features;
    bt_hfp_ag->supt_codecs = supported_codecs;
    return true;

fail_reg_hsp_ag_rfc_cb:
fail_reg_hfp_ag_rfc_cb:
    free(bt_hfp_ag);
    bt_hfp_ag = NULL;
fail_alloc_hfp:
    PROFILE_PRINT_ERROR1("bt_hfp_ag_init: failed %d", -ret);
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

bool bt_hfp_ag_init(uint8_t  rfc_hfp_chann_num,
                    uint8_t  rfc_hsp_chann_num,
                    uint16_t supported_features,
                    uint8_t  supported_codecs)
{
    return false;
}

void bt_hfp_ag_handle_audio_conn_rsp(uint8_t bd_addr[6], uint16_t cause)
{
}

void bt_hfp_ag_handle_audio_conn_cmpl(uint8_t bd_addr[6], uint16_t cause)
{
}

void bt_hfp_ag_handle_audio_disconn(uint8_t bd_addr[6])
{
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
