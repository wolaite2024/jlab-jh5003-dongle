/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_IAP_SUPPORT == 1)
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_sync.h"
#include "os_sched.h"
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "sys_timer.h"
#include "rfc.h"
#include "iap_cp.h"
#include "bt_iap_int.h"
#include "bt_iap.h"
#include "bt_mgr_int.h"

#define IAP2_PARA_HDR_SIZE              0x04

#define IAP2_MAX_TX_PKT_LEN             990

/* iAP packet macro */
#define IAP_START_BYTE                  0x55
#define IAP_LENGTH_MARKER               0x00

#define IAP2_START_BYTE1                0xFF
#define IAP2_START_BYTE2                0x5A

#define IAP2_MSG_START                  0x4040

#define IAP2_CTRL_ACK                   0x40
#define IAP2_CTRL_EAK_ACK               0x60
#define IAP2_CTRL_SYNC_ACK              0xC0
#define IAP2_CTRL_RESET                 0x10

#define CTRL_SESSION_ID                 0x01
#define EA_SESSION_ID                   0x02

#define IAP2_PKT_HDR_SIZE               0x09
#define IAP2_MSG_HDR_SIZE               0x06
#define IAP2_PARAM_HDR_SIZE             0x04
#define IAP2_PAYLOAD_CHECKSUM_SIZE      0x01
#define IAP2_EA_ID_SIZE                 0x02
#define IAP2_EA_HDR_SIZE                (IAP2_PKT_HDR_SIZE + IAP2_EA_ID_SIZE)
#define IAP2_CTRL_MSG_HDR_SIZE          (IAP2_PKT_HDR_SIZE + IAP2_MSG_HDR_SIZE)
#define IAP2_CTRL_MSG_PAYLOAD_HDR_SIZE  (IAP2_MSG_HDR_SIZE + IAP2_PARAM_HDR_SIZE)
#define IAP2_CTRL_MSG_PARAM_HDR_SIZE    (IAP2_PKT_HDR_SIZE + IAP2_MSG_HDR_SIZE + IAP2_PARAM_HDR_SIZE)

#define IAP2_DETECT_PKT_SIZE            0x06
#define IAP2_SYNC_PKT_SIZE              0x1A

#define IAP2_ACC_SEQ_OFFSET                             5
#define IAP2_DEV_SEQ_OFFSET                             6
#define IAP2_MAX_NUM_OF_OUTSTANDING_PKTS_OFFSET         10
#define IAP2_MAX_PKT_LEN_OFFSET                         11
#define IAP2_RETRANSMISSION_TIMEOUT_OFFSET              13
#define IAP2_CUMULATIVE_ACK_TIMEOUT_OFFSET              15
#define IAP2_NUM_OF_RETRANSMISSION_OFFSET               17
#define IAP2_MAX_CUMULATIVE_ACK_OFFSET                  18

/* iAP2 Messages */
#define MSG_START_CALL_STATE_UPDATES                0x4154
#define MSG_CALL_STATE_UPDATE                       0x4155
#define MSG_STOP_CALL_STATE_UPDATES                 0x4156
#define MSG_START_COMM_UPDATES                      0x4157
#define MSG_MUTE_STATUS_UPDATE                      0x4160

#define MSG_BT_COMPONENT_INFO           0x4E01
#define MSG_START_BT_CONN_UPDATES       0x4E03
#define MSG_BT_CONN_UPDATE              0x4E04
#define MSG_STOP_BT_CONN_UPDATES        0x4E05

#define MSG_START_HID                   0x6800
#define MSG_DEV_HID_REPORT              0x6801
#define MSG_ACC_HID_REPORT              0x6802
#define MSG_STOP_HID                    0x6803

#define MSG_START_IDENT                 0x1D00
#define MSG_IDENT_INFO                  0x1D01
#define MSG_IDENT_ACCEPTED              0x1D02
#define MSG_IDENT_REJECTED              0x1D03
#define MSG_CANCEL_IDENT                0x1D05

#define MSG_REQ_AUTHEN_CERT             0xAA00
#define MSG_AUTHEN_CERT                 0xAA01
#define MSG_REQ_AUTHEN_CHALLENGE_RSP    0xAA02
#define MSG_AUTHEN_RSP                  0xAA03
#define MSG_AUTHEN_FAILED               0xAA04
#define MSG_AUTHEN_SUCCEEDED            0xAA05

#define MSG_START_EA_PROTO_SESSION      0xEA00
#define MSG_STOP_EA_PROTO_SESSION       0xEA01
#define MSG_REQ_APP_LAUNCH              0xEA02
#define MSG_EA_PROTO_SESSION_STATUS     0xEA03

/* parameter ID for message start communication updates */
#define PARAM_ID_UPDATE_SIGNAL_STRENGTH 0x0001
#define PARAM_ID_UPDATE_MUTE_STATUS     0x0009

/* parameter ID for message mute status updates */
#define PARAM_ID_MUTE_STATUS            0x0000

/* parameter ID for message bluetooth component status */
#define PARAM_ID_BT_COMPONENT_STATUS    0x0000
#define PARAM_ID_COMPONENT_ID           0x0000
#define PARAM_ID_COMPONENT_ENABLED      0x0001

/* parameter ID for message start HID */
#define PARAM_ID_HID_COMPONENT_ID       0x0000
#define PARAM_ID_VENDOR_ID              0x0001
#define PARAM_ID_PRODUCT_ID             0x0002
#define PARAM_ID_LOCAL_KB_COUNTRY_CODE  0x0003
#define PARAM_ID_HID_REPORT_DESCRIPTOR  0x0004

/* parameter ID for message device HID report */
#define PARAM_ID_DEV_HID_COMPONENT_ID   0x0000
#define PARAM_ID_DEV_HID_REPORT         0x0001

/* parameter ID for message accessory HID report */
#define PARAM_ID_ACC_HID_COMPONENT_ID   0x0000
#define PARAM_ID_ACC_HID_REPORT         0x0001

/* parameter ID for message authentication certification */
#define PARAM_ID_AUTHEN_CERT            0x0000

/* parameter ID for message authentication response */
#define PARAM_ID_AUTHEN_RSP             0x0000

/* parameter ID for message start External Accessory Protocol session */
#define PARAM_ID_EAP_ID                 0x0000
#define PARAM_ID_EAP_SESSION_ID         0x0001

/* parameter ID for message External Accessory Protocol session status */
#define EAP_SESSION_STATUS_PARAM_ID_SESSION_ID        0x0000
#define EAP_SESSION_STATUS_PARAM_ID_SESSION_STATUS    0x0001

/* parameter ID for message request app launch */
#define PARAM_ID_APP_BUNDLE_ID          0x0000
#define PARAM_ID_APP_LAUNCH_METHOD      0x0001

#define HI_WORD(x)  ((uint8_t)((x & 0xFF00) >> 8))
#define LO_WORD(x)  ((uint8_t)(x))

typedef enum t_bt_iap_timer_id
{
    BT_IAP_AUTHEN_TOUT = 0x00,
    BT_IAP2_DETECT     = 0x01,
    BT_IAP2_SYNC       = 0x02,
    BT_IAP2_SEND_DATA  = 0x03,
    BT_IAP2_SIGNATURE  = 0x04,
} T_BT_IAP_TIMER_ID;

typedef enum t_bt_iap2_session_type
{
    BT_IAP2_SESSION_TYPE_CTRL               = 0x00,
    BT_IAP2_SESSION_TYPE_FILE_TRANSFER      = 0x01,
    BT_IAP2_SESSION_TYPE_EXTERNAL_ACCESSORY = 0x02,
} T_BT_IAP2_SESSION_TYPE;

typedef enum t_bt_iap_state
{
    BT_IAP_STATE_DISCONNECTED      = 0x00,
    BT_IAP_STATE_CONNECTING        = 0x01,
    BT_IAP_STATE_CONNECTED         = 0x02,
    BT_IAP_STATE_DETECT            = 0x03,
    BT_IAP_STATE_IAP2_SYNC         = 0x04,
    BT_IAP_STATE_IAP2_SYNC_CMPL    = 0x05,
    BT_IAP_STATE_IAP2_AUTHEN       = 0x06,
    BT_IAP_STATE_IAP2_CHALLENGE    = 0x07,
    BT_IAP_STATE_IAP2_IDENT        = 0x08,
    BT_IAP_STATE_IAP2_IDENTED      = 0x09,
    BT_IAP_STATE_DISCONNECTING     = 0x0a,
} T_BT_IAP_STATE;

typedef struct t_bt_iap_link
{
    struct t_bt_iap_link *next;
    T_BT_IAP_STATE        state;
    uint8_t               index;
    uint8_t               bd_addr[6];
    uint8_t               dlci;
    uint8_t               remote_credit;
    uint16_t              rfc_frame_size;

    T_SYS_TIMER_HANDLE    authen_timer;
    T_SYS_TIMER_HANDLE    sync_timer;
    T_SYS_TIMER_HANDLE    detect_timer;
    T_SYS_TIMER_HANDLE    signature_timer;

    T_OS_QUEUE            used_tx_q;
    T_OS_QUEUE            sent_tx_q;

    uint8_t               header[9];
    uint8_t               header_offset;
    uint8_t               header_len;
    uint16_t              payload_len;
    uint16_t              payload_offset;
    uint8_t              *payload;

    uint8_t               remain_tx_q_num;
    uint8_t               acc_pkt_seq;        /* tx seq */
    uint8_t               acked_seq;          /* seq num acked by device */
    uint8_t               dev_pkt_seq;        /* received dev seq num */

    uint16_t              dev_max_pkt_len;    /* max pkt len to send */
    uint16_t              dev_retrans_tout;
    uint16_t              dev_cumulative_ack_tout;
    uint8_t               dev_max_out_pkt;
    uint8_t               dev_max_retrans;
    uint8_t               dev_max_cumulative_ack;
} T_BT_IAP_LINK;

typedef struct t_bt_iap
{
    T_OS_QUEUE link_list;
    uint8_t    iap_service_id;
    uint8_t    tx_q_elem_num;

    /* parameters used for sync, can be changed by app using set param API */
    uint16_t   acc_max_pkt_len;
    uint16_t   acc_retrans_tout;
    uint16_t   acc_cumulative_ack_tout;
    uint8_t    acc_max_out_pkt;
    uint8_t    acc_max_retrans;
    uint8_t    acc_max_cumulative_ack;
    uint8_t    acc_start_seq;
} T_BT_IAP;

typedef struct t_bt_iap2_tx_data_elem
{
    struct t_bt_iap2_tx_data_elem *next;
    T_SYS_TIMER_HANDLE             retrans_timer;
    uint16_t                       ea_session_id;
    uint16_t                       data_len; /* payload len exclude iAP2 pkt hdr and payload checksum */
    uint8_t                        session_id;
    uint8_t                        retrans_cnt;
    uint8_t                        data[0];
} T_BT_IAP2_TX_DATA_ELEM;

const uint8_t iap2_sync_packet[IAP2_SYNC_PKT_SIZE] =
{
    /* Header */
    0xFF,   /* Start of Packet 0xff5a */
    0x5A,
    HI_WORD(IAP2_SYNC_PKT_SIZE),   /* Packet Length High Byte */
    LO_WORD(IAP2_SYNC_PKT_SIZE),   /* Packet Length Low Byte */
    0x80,   /* Byte4:  Control Byte */
    0x00,   /* Byte5:  Packet Sequence Number */
    0x00,   /* Byte6:  Packet Acknowledgment Number */
    0x00,   /* Byte7:  Session Identifier */
    0x00,   /* Byte8:  Header Checksum */

    0x01,   /* Byte9:  Link Version */
    0x04,   /* Byte10: Maximum number of Outstanding Packets */
    0x03,   /* Byte11~12: Maximum Packet length 800 */
    0x20,
    0x01,   /* Byte13~14: Retransmission Timeout 500 */
    0xf4,
    0x00,   /* Byte15~16: Cumulative Acknowledgement Timeout 100 */
    0x64,
    0x03,   /* Byte17: Maximum Number of Retransmission */
    0x04,   /* Byte18: Maximum Cumulative Acknowledgements */

    /* Session info */
    CTRL_SESSION_ID,            /* iAP2 Session 1: Session ID */
    BT_IAP2_SESSION_TYPE_CTRL,     /* iAP2 Session 1: Session Type */
    0x01,                       /* iAP2 Session 1: Session Version */

    EA_SESSION_ID,                          /* iAP2 Session 2: Session ID */
    BT_IAP2_SESSION_TYPE_EXTERNAL_ACCESSORY,   /* iAP2 Session 2: Session Type */
    0x01,                                   /* iAP2 Session 2: Session Version */

    0x00,   /* Payload Checksum */
};

const uint8_t iap2_detect_code[IAP2_DETECT_PKT_SIZE] =
{
    0xFF,
    0x55,
    0x02,
    0x00,
    0xEE,
    0x10
};

T_BT_IAP *bt_iap;

void bt_iap_handle_timeout(T_SYS_TIMER_HANDLE handle);

T_BT_IAP_LINK *bt_iap_alloc_link(uint8_t bd_addr[6])
{
    T_BT_IAP_LINK *link;
    uint8_t        index;
    uint32_t       ret = 0;

    link = calloc(1, sizeof(T_BT_IAP_LINK));
    if (link == NULL)
    {
        ret = 1;
        goto fail_alloc_link;
    }

    for (index = 0; index < bt_iap->link_list.count; index++)
    {
        T_BT_IAP_LINK *pre_link;
        bool           index_used = false;

        pre_link = os_queue_peek(&bt_iap->link_list, 0);
        while (pre_link != NULL)
        {
            if (pre_link->index == index)
            {
                index_used = true;
                break;
            }

            pre_link = pre_link->next;
        }

        if (index_used == false)
        {
            break;
        }
    }
    link->index = index;

    link->authen_timer = sys_timer_create("iap_authen",
                                          SYS_TIMER_TYPE_LOW_PRECISION,
                                          (BT_IAP_AUTHEN_TOUT << 16) | index,
                                          15000 * 1000,
                                          false,
                                          bt_iap_handle_timeout);
    if (link->authen_timer == NULL)
    {
        ret = 2;
        goto fail_create_authen_timer;
    }

    link->sync_timer = sys_timer_create("iap_sync",
                                        SYS_TIMER_TYPE_LOW_PRECISION,
                                        (BT_IAP2_SYNC << 16) | index,
                                        1000 * 1000,
                                        false,
                                        bt_iap_handle_timeout);
    if (link->sync_timer == NULL)
    {
        ret = 3;
        goto fail_create_sync_timer;
    }

    link->detect_timer = sys_timer_create("iap_detect_retrans",
                                          SYS_TIMER_TYPE_LOW_PRECISION,
                                          (BT_IAP2_DETECT << 16) | index,
                                          1000 * 1000,
                                          false,
                                          bt_iap_handle_timeout);
    if (link->detect_timer == NULL)
    {
        ret = 4;
        goto fail_create_detect_retrans_timer;
    }

    link->signature_timer = sys_timer_create("iap_signature",
                                             SYS_TIMER_TYPE_LOW_PRECISION,
                                             (BT_IAP2_SIGNATURE << 16) | index,
                                             500 * 1000,
                                             false,
                                             bt_iap_handle_timeout);
    if (link->signature_timer == NULL)
    {
        ret = 5;
        goto fail_create_signature_timer;
    }

    memcpy(link->bd_addr, bd_addr, 6);
    link->state = BT_IAP_STATE_DISCONNECTED;
    os_queue_in(&bt_iap->link_list, link);

    return link;

fail_create_signature_timer:
    sys_timer_delete(link->detect_timer);
fail_create_detect_retrans_timer:
    sys_timer_delete(link->sync_timer);
fail_create_sync_timer:
    sys_timer_delete(link->authen_timer);
fail_create_authen_timer:
    free(link);
fail_alloc_link:
    PROFILE_PRINT_ERROR1("bt_iap_alloc_link: failed %d", ret);
    return NULL;
}

void bt_iap_free_link(T_BT_IAP_LINK *link)
{
    T_BT_IAP2_TX_DATA_ELEM *elem;

    if (link->authen_timer != NULL)
    {
        sys_timer_delete(link->authen_timer);
    }

    if (link->sync_timer != NULL)
    {
        sys_timer_delete(link->sync_timer);
    }

    if (link->detect_timer != NULL)
    {
        sys_timer_delete(link->detect_timer);
    }

    if (link->signature_timer != NULL)
    {
        sys_timer_delete(link->signature_timer);
    }

    if (link->payload)
    {
        free(link->payload);
    }

    while (link->used_tx_q.count)
    {
        elem = os_queue_out(&link->used_tx_q);
        free(elem);
    }

    while (link->sent_tx_q.count)
    {
        elem = os_queue_out(&link->sent_tx_q);

        if (elem->retrans_timer != NULL)
        {
            sys_timer_delete(elem->retrans_timer);
        }

        free(elem);
    }

    os_queue_delete(&bt_iap->link_list, link);
    free(link);
}

T_BT_IAP_LINK *bt_iap_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_IAP_LINK *link;

    link = os_queue_peek(&bt_iap->link_list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

bool bt_iap_send_pkt(T_BT_IAP_LINK *link,
                     uint8_t       *data,
                     uint16_t       data_len)
{
    int32_t ret = 0;

    if (data_len > (link->remote_credit * link->rfc_frame_size))
    {
        ret = 1;
        goto fail_length_exceed;
    }

    while (data_len)
    {
        uint16_t pkt_len;

        pkt_len = (data_len > link->rfc_frame_size) ? link->rfc_frame_size : data_len;

        if (rfc_data_req(link->bd_addr, link->dlci, data, pkt_len, false) == true)
        {
            link->remote_credit--;
        }
        else
        {
            ret = 2;
            goto fail_send_data;
        }

        data_len -= pkt_len;
        data   += pkt_len;
    }

    return true;

fail_send_data:
fail_length_exceed:
    PROFILE_PRINT_ERROR4("bt_iap_send_pkt: failed %d, data len %u, frame size %u, credits %u",
                         -ret, data_len, link->rfc_frame_size, link->remote_credit);
    return false;
}

uint8_t bt_iap_cal_checksum(uint8_t  *data,
                            uint16_t  len)
{
    uint8_t check_sum = 0;

    while (len)
    {
        check_sum += *data;
        data++;
        len--;
    }

    return (uint8_t)(0xff - check_sum + 1); /* ((~check_sum)+1); */
}

void bt_iap2_sync_cmpl(T_BT_IAP_LINK *link)
{
    T_BT_IAP_CONN_INFO info;
    T_BT_MSG_PAYLOAD   payload;

    link->state = BT_IAP_STATE_IAP2_SYNC_CMPL;

    os_queue_init(&link->used_tx_q);
    os_queue_init(&link->sent_tx_q);

    if (link->dev_max_pkt_len > link->rfc_frame_size)
    {
        link->dev_max_pkt_len = link->rfc_frame_size;
    }

    link->remain_tx_q_num = bt_iap->tx_q_elem_num;

    info.max_data_len = link->dev_max_pkt_len - IAP2_EA_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = (void *)&info;
    bt_mgr_dispatch(BT_MSG_IAP_CONN_CMPL, &payload);
}

void bt_iap_authen_cmpl(T_BT_IAP_LINK *link,
                        bool           success)
{
    T_BT_MSG_PAYLOAD payload;

    sys_timer_stop(link->authen_timer);
    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;

    if (success)
    {
        bt_mgr_dispatch(BT_MSG_IAP_AUTHEN_SUCCESS, &payload);
    }
    else
    {
        bt_mgr_dispatch(BT_MSG_IAP_AUTHEN_FAIL, &payload);
    }
}

void bt_iap2_wrap_header(T_BT_IAP_LINK *link,
                         uint8_t       *hdr,
                         uint16_t       payload_len,
                         uint8_t        session_id)
{
    uint16_t pkt_len;
    uint8_t *p = hdr;

    if (payload_len)
    {
        pkt_len = IAP2_PKT_HDR_SIZE + payload_len + IAP2_PAYLOAD_CHECKSUM_SIZE;
    }
    else
    {
        pkt_len = IAP2_PKT_HDR_SIZE;
    }

    BE_UINT8_TO_STREAM(p, IAP2_START_BYTE1);
    BE_UINT8_TO_STREAM(p, IAP2_START_BYTE2);
    BE_UINT16_TO_STREAM(p, pkt_len);
    BE_UINT8_TO_STREAM(p, IAP2_CTRL_ACK);
    BE_UINT8_TO_STREAM(p, link->acc_pkt_seq);
    BE_UINT8_TO_STREAM(p, link->dev_pkt_seq);
    BE_UINT8_TO_STREAM(p, session_id);
    BE_UINT8_TO_STREAM(p, bt_iap_cal_checksum(hdr, IAP2_PKT_HDR_SIZE - 1));
}

void bt_iap2_wrap_msg(T_BT_IAP_LINK *link,
                      uint8_t       *pkt,
                      uint16_t       msg_type,
                      uint16_t       msg_len)
{
    uint8_t *p = pkt + IAP2_PKT_HDR_SIZE;

    BE_UINT16_TO_STREAM(p, IAP2_MSG_START);
    BE_UINT16_TO_STREAM(p, msg_len + IAP2_MSG_HDR_SIZE);
    BE_UINT16_TO_STREAM(p, msg_type);

    p = pkt + IAP2_CTRL_MSG_HDR_SIZE + msg_len;
    *p = bt_iap_cal_checksum(pkt + IAP2_PKT_HDR_SIZE, IAP2_MSG_HDR_SIZE + msg_len);
}

/* input param is the start address to put in message parameters, return address can be used to put in the next one */
uint8_t *bt_iap2_prep_param(uint8_t  *param,
                            uint16_t  param_id,
                            void     *data,
                            uint16_t  data_len)
{
    BE_UINT16_TO_STREAM(param, data_len + IAP2_PARAM_HDR_SIZE);
    BE_UINT16_TO_STREAM(param, param_id);
    ARRAY_TO_STREAM(param, data, data_len);

    return param;
}

bool bt_iap2_send_detect(T_BT_IAP_LINK *link)
{
    uint8_t iap2_detect[IAP2_DETECT_PKT_SIZE];

    memcpy(iap2_detect, iap2_detect_code, IAP2_DETECT_PKT_SIZE);

    return bt_iap_send_pkt(link, iap2_detect, IAP2_DETECT_PKT_SIZE);
}

bool bt_iap2_send_ack(T_BT_IAP_LINK *link)
{
    uint8_t buf[IAP2_PKT_HDR_SIZE];

    bt_iap2_wrap_header(link, buf, 0, 0);

    return bt_iap_send_pkt(link, buf, IAP2_PKT_HDR_SIZE);
}

bool bt_iap2_send_sync(T_BT_IAP_LINK *link)
{
    uint8_t buf[IAP2_SYNC_PKT_SIZE];

    memcpy(buf, iap2_sync_packet, IAP2_SYNC_PKT_SIZE);

    if (link->acc_pkt_seq != bt_iap->acc_start_seq)
    {
        buf[4] = IAP2_CTRL_SYNC_ACK;
    }

    buf[IAP2_ACC_SEQ_OFFSET] = link->acc_pkt_seq;
    buf[IAP2_DEV_SEQ_OFFSET] = link->dev_pkt_seq;
    buf[IAP2_PKT_HDR_SIZE - 1] = bt_iap_cal_checksum(buf, IAP2_PKT_HDR_SIZE - 1);

    BE_UINT16_TO_ARRAY(buf + IAP2_RETRANSMISSION_TIMEOUT_OFFSET, link->dev_retrans_tout);
    BE_UINT16_TO_ARRAY(buf + IAP2_CUMULATIVE_ACK_TIMEOUT_OFFSET, link->dev_cumulative_ack_tout);
    BE_UINT8_TO_ARRAY(buf + IAP2_NUM_OF_RETRANSMISSION_OFFSET, link->dev_max_retrans);
    BE_UINT8_TO_ARRAY(buf + IAP2_MAX_CUMULATIVE_ACK_OFFSET, link->dev_max_cumulative_ack);
    buf[IAP2_SYNC_PKT_SIZE - 1] = bt_iap_cal_checksum(buf + IAP2_PKT_HDR_SIZE,
                                                      IAP2_SYNC_PKT_SIZE - IAP2_PKT_HDR_SIZE - 1);

    return bt_iap_send_pkt(link, buf, IAP2_SYNC_PKT_SIZE);
}

void bt_iap2_update_acked_pkt(T_BT_IAP_LINK *link)
{
    uint8_t                       pkt_seq;
    T_BT_IAP2_TX_DATA_ELEM       *elem;
    T_BT_IAP_DATA_TRANSMITTED     msg;
    uint8_t                      *data;
    bool                          seq_check = false;

    PROFILE_PRINT_TRACE2("bt_iap2_update_acked_pkt: sent queue cnt %d, acked seq %d",
                         link->sent_tx_q.count, link->acked_seq);

    elem = (T_BT_IAP2_TX_DATA_ELEM *)link->sent_tx_q.p_first;
    while (elem)
    {
        if (elem->data[IAP2_ACC_SEQ_OFFSET] == link->acked_seq)
        {
            seq_check = true;
            break;
        }

        elem = elem->next;
    }

    if (seq_check == false)
    {
        return;
    }

    while (os_queue_peek(&link->sent_tx_q, 0))
    {
        elem = os_queue_out(&link->sent_tx_q);
        sys_timer_delete(elem->retrans_timer);
        data = elem->data;
        pkt_seq = *(data + IAP2_ACC_SEQ_OFFSET);

        if (elem->session_id == EA_SESSION_ID)
        {
            T_BT_MSG_PAYLOAD  payload;

            msg.eap_session_id = elem->ea_session_id;
            msg.success = true;
            memcpy(payload.bd_addr, link->bd_addr, 6);
            payload.msg_buf = (void *)&msg;
            bt_mgr_dispatch(BT_MSG_IAP_DATA_TRANSMITTED, &payload);
        }

        free(elem);
        link->remain_tx_q_num++;

        if (pkt_seq == link->acked_seq)
        {
            /* all pkt before ack seq are freed */
            break;
        }
    }
}

void bt_iap2_tx_data(T_BT_IAP_LINK *link)
{
    T_BT_IAP2_TX_DATA_ELEM *elem;

    elem = os_queue_peek(&link->used_tx_q, 0);
    /* no need to check tx window because tx queue is not larger than window */
    while (elem)
    {
        link->acc_pkt_seq++;
        bt_iap2_wrap_header(link, elem->data, elem->data_len, elem->session_id);

        if (bt_iap_send_pkt(link, elem->data,
                            IAP2_PKT_HDR_SIZE + elem->data_len + IAP2_PAYLOAD_CHECKSUM_SIZE) == true)
        {
            os_queue_delete(&link->used_tx_q, elem);
            os_queue_in(&link->sent_tx_q, elem);

            elem->retrans_timer = sys_timer_create("iap2_data_trans",
                                                   SYS_TIMER_TYPE_LOW_PRECISION,
                                                   (BT_IAP2_SEND_DATA << 16) | (link->acc_pkt_seq << 8) | link->index,
                                                   link->dev_retrans_tout * 1000,
                                                   false,
                                                   bt_iap_handle_timeout);
            sys_timer_start(elem->retrans_timer);

            PROFILE_PRINT_TRACE5("bt_iap2_tx_data: used queue elem cnt %u, credit %u, tx seq %u, acked seq %u, max out pkt %u",
                                 link->used_tx_q.count, link->remote_credit, link->acc_pkt_seq,
                                 link->acked_seq, link->dev_max_out_pkt);

            elem = os_queue_peek(&link->used_tx_q, 0);
        }
        else
        {
            link->acc_pkt_seq--;
            break;
        }
    }
}

void bt_iap_handle_iap_pkt(T_BT_IAP_LINK *link,
                           uint8_t       *data,
                           uint16_t       len)
{
    uint8_t *p = data;        /* payload start from lingoID */

    /* FF 55 02 00 EE 10 => device support iAP2, only handle this now */
    if ((len == 3) && (*p == 0x00) && (*(p + 1) == 0xEE) && (*(p + 2) == 0x10))
    {
        if (link->detect_timer != NULL)
        {
            sys_timer_delete(link->detect_timer);
            link->detect_timer = NULL;
        }

        link->acc_pkt_seq = bt_iap->acc_start_seq;
        link->dev_pkt_seq = 0;

        /* init sync param */
        link->dev_retrans_tout = bt_iap->acc_retrans_tout;
        link->dev_cumulative_ack_tout = bt_iap->acc_cumulative_ack_tout;
        link->dev_max_retrans = bt_iap->acc_max_retrans;
        link->dev_max_cumulative_ack = bt_iap->acc_max_cumulative_ack;

        bt_iap2_send_sync(link);
        link->state = BT_IAP_STATE_IAP2_SYNC;
        sys_timer_start(link->sync_timer);
    }
}

void bt_iap2_handle_ctrl_sync_ack(T_BT_IAP_LINK *link,
                                  uint8_t       *data)
{
    uint16_t retrans_tout;
    uint16_t cum_ack_tout;
    uint8_t  max_retrans;
    uint8_t  max_cum_ack;

    data += 1; /* link ver */
    BE_STREAM_TO_UINT8(link->dev_max_out_pkt, data);
    BE_STREAM_TO_UINT16(link->dev_max_pkt_len, data);
    BE_STREAM_TO_UINT16(retrans_tout, data);
    BE_STREAM_TO_UINT16(cum_ack_tout, data);
    BE_STREAM_TO_UINT8(max_retrans, data);
    BE_STREAM_TO_UINT8(max_cum_ack, data);

    if (link->dev_retrans_tout == retrans_tout &&
        link->dev_cumulative_ack_tout == cum_ack_tout &&
        link->dev_max_retrans == max_retrans &&
        link->dev_max_cumulative_ack == max_cum_ack)
    {
        bt_iap2_send_ack(link);

        if (link->state == BT_IAP_STATE_IAP2_SYNC)
        {
            bt_iap2_sync_cmpl(link);
        }
    }
    else
    {
        link->dev_retrans_tout = retrans_tout;
        link->dev_cumulative_ack_tout = cum_ack_tout;
        link->dev_max_retrans = max_retrans;
        link->dev_max_cumulative_ack = max_cum_ack;

        link->acc_pkt_seq++;      /* increase acc seq to send sync+ack pkt */
        bt_iap2_send_sync(link);
    }
}

void bt_iap2_handle_req_authen_cert(T_BT_IAP_LINK *link)
{
    uint8_t  *buf;
    uint8_t  *p;
    uint16_t  crf_len = 0;
    uint16_t  read_len = 0;
    uint16_t  access_len;
    uint8_t   read_page = CP_REG_ACD1;
    int32_t   ret = 0;

    if (link->state != BT_IAP_STATE_IAP2_SYNC_CMPL)
    {
        ret = 1;
        goto fail_state_error;
    }

    link->state = BT_IAP_STATE_IAP2_AUTHEN;

    if (cp_read_crf_data_len(&crf_len) == false)
    {
        ret = 2;
        goto fail_cp_read_len;
    }

    buf = calloc(1, crf_len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE +
                 IAP2_PAYLOAD_CHECKSUM_SIZE);
    if (buf == NULL)
    {
        ret = 3;
        goto fail_alloc_buf;
    }

    p = buf + IAP2_PKT_HDR_SIZE + IAP2_MSG_HDR_SIZE;
    BE_UINT16_TO_STREAM(p, crf_len + IAP2_PARAM_HDR_SIZE);
    BE_UINT16_TO_STREAM(p, PARAM_ID_AUTHEN_CERT);

    while (read_len < crf_len && read_page <= CP_REG_ACD10)
    {
        access_len = crf_len - read_len;
        if (access_len > 128)
        {
            access_len = 128;
        }

        if (cp_read_crf_data(read_page, p + read_len, access_len) == false)
        {
            ret = 4;
            goto fail_cp_read_data;
        }

        read_page++;
        read_len += access_len;
    }

    link->acc_pkt_seq++;
    bt_iap2_wrap_msg(link, buf, MSG_AUTHEN_CERT, crf_len + IAP2_PARAM_HDR_SIZE);
    bt_iap2_wrap_header(link, buf, crf_len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE,
                        CTRL_SESSION_ID);
    bt_iap_send_pkt(link, buf,
                    crf_len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE +
                    IAP2_PAYLOAD_CHECKSUM_SIZE);
    free(buf);
    return;

fail_cp_read_data:
    free(buf);
fail_alloc_buf:
fail_cp_read_len:
    bt_iap_authen_cmpl(link, false);
fail_state_error:
    PROFILE_PRINT_ERROR1("bt_iap2_handle_req_authen_cert: failed %d", -ret);
    return;
}

void bt_iap2_handle_req_authen_challenge_rsp_1(T_BT_IAP_LINK *link,
                                               uint8_t       *param,
                                               uint16_t       param_len)
{
    uint8_t  *data;
    uint16_t  len;
    int32_t   ret = 0;

    data = param + IAP2_PARAM_HDR_SIZE;
    len = param_len - IAP2_PARAM_HDR_SIZE;

    if (link->state != BT_IAP_STATE_IAP2_AUTHEN)
    {
        ret = 1;
        goto fail_state_error;
    }

    if (cp_write_cha_len(len) == false)
    {
        ret = 2;
        goto fail_cp_write_len;
    }

    if (cp_write_cha_data(data, len) == false)
    {
        ret = 3;
        goto fail_cp_write_data;
    }

    /* start new signature process */
    if (cp_ctrl(CP_CMD_START_SIG_GEN) == false)
    {
        ret = 4;
        goto fail_cp_ctrl;
    }

    sys_timer_start(link->signature_timer);
    return;

fail_cp_ctrl:
fail_cp_write_data:
fail_cp_write_len:
    bt_iap_authen_cmpl(link, false);
fail_state_error:
    PROFILE_PRINT_ERROR1("iap2_handle_req_authen_challenge_rsp: failed %d", -ret);
    return;
}

void bt_iap2_handle_req_authen_challenge_rsp_2(T_BT_IAP_LINK *link)
{
    uint8_t      *data;
    uint16_t      len;
    uint8_t      *buf;
    T_CP_PRO_RES  cp_res = CP_PRO_RES_NO_VALID;
    uint8_t       read_authen_cnt = 0;
    int32_t       ret = 0;

    while (read_authen_cnt < 3)
    {
        if (cp_read_proc_result(&cp_res) == true)
        {
            if (cp_res == CP_PRO_RES_SIG_GEN_OK)
            {
                break;
            }
        }
        read_authen_cnt++;
        os_delay(500);
    }

    if (cp_read_signature_len(&len) == false)
    {
        ret = 1;
        goto fail_cp_read_len;
    }

    buf = calloc(1, len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE +
                 IAP2_PAYLOAD_CHECKSUM_SIZE);
    if (buf == NULL)
    {
        ret = 2;
        goto fail_alloc_buf;
    }

    data = buf + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE;
    BE_UINT16_TO_STREAM(data, len + IAP2_PARAM_HDR_SIZE);
    BE_UINT16_TO_STREAM(data, PARAM_ID_AUTHEN_RSP);
    if (cp_read_signature_data(data, len) == false)
    {
        ret = 3;
        goto fail_cp_read_data;
    }

    link->acc_pkt_seq++;
    bt_iap2_wrap_msg(link, buf, MSG_AUTHEN_RSP, len + IAP2_PARAM_HDR_SIZE);
    bt_iap2_wrap_header(link, buf, len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE, CTRL_SESSION_ID);
    bt_iap_send_pkt(link, buf, len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE +
                    IAP2_PAYLOAD_CHECKSUM_SIZE);
    free(buf);
    return;

fail_cp_read_data:
    free(buf);
fail_alloc_buf:
fail_cp_read_len:
    PROFILE_PRINT_ERROR1("iap2_handle_req_authen_challenge_rsp: failed %d", -ret);
    bt_iap_authen_cmpl(link, false);
    return;
}

void bt_iap2_handle_start_eap_session(T_BT_IAP_LINK *link,
                                      uint8_t       *data,
                                      uint16_t       len)
{
    uint8_t                    *p;
    uint8_t                    *param = data;
    uint16_t                    param_len;
    uint16_t                    param_id;
    T_BT_MSG_PAYLOAD            payload;
    T_BT_IAP_START_EAP_SESSION  msg;

    bt_iap2_send_ack(link);

    while (param < data + len)
    {
        p = param;
        BE_STREAM_TO_UINT16(param_len, p);
        BE_STREAM_TO_UINT16(param_id, p);

        if (param_id == PARAM_ID_EAP_ID)
        {
            BE_STREAM_TO_UINT8(msg.eap_id, p);
        }
        else if (param_id == PARAM_ID_EAP_SESSION_ID)
        {
            BE_STREAM_TO_UINT16(msg.eap_session_id, p);
        }

        param += param_len;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = (void *)&msg;
    bt_mgr_dispatch(BT_MSG_IAP_START_EAP_SESSION, &payload);
}

void bt_iap2_handle_stop_eap_session(T_BT_IAP_LINK *link,
                                     uint8_t       *data,
                                     uint16_t       len)
{
    T_BT_IAP_STOP_EAP_SESSION msg;
    T_BT_MSG_PAYLOAD          payload;

    bt_iap2_send_ack(link);

    data += IAP2_PARAM_HDR_SIZE;
    BE_STREAM_TO_UINT16(msg.eap_session_id, data);
    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = (void *)&msg;
    bt_mgr_dispatch(BT_MSG_IAP_STOP_EAP_SESSION, &payload);
}

void bt_iap2_handle_eap_session_status(T_BT_IAP_LINK *link,
                                       uint8_t       *data,
                                       uint16_t       len)
{
    uint8_t                          *p;
    uint8_t                          *param = data;
    uint16_t                          param_len;
    uint16_t                          param_id;
    T_BT_MSG_PAYLOAD                  payload;
    T_BT_IAP_EAP_SESSION_STATUS_INFO  msg;

    bt_iap2_send_ack(link);

    while (param < data + len)
    {
        p = param;
        BE_STREAM_TO_UINT16(param_len, p);
        BE_STREAM_TO_UINT16(param_id, p);

        if (param_id == EAP_SESSION_STATUS_PARAM_ID_SESSION_ID)
        {
            BE_STREAM_TO_UINT16(msg.eap_session_id, p);
        }
        else if (param_id == EAP_SESSION_STATUS_PARAM_ID_SESSION_STATUS)
        {
            msg.eap_session_status = *(uint8_t *)p;
            p++;
        }

        param += param_len;
    }

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = (void *)&msg;
    bt_mgr_dispatch(BT_MSG_IAP_EAP_SESSION_STATUS, &payload);
}

void bt_iap2_handle_dev_hid_report(T_BT_IAP_LINK *link,
                                   uint8_t       *data,
                                   uint16_t       len)
{
    uint8_t *p;
    uint8_t *param = data;
    uint16_t param_len;
    uint16_t param_id;

    while (param < data + len)
    {
        p = param;
        BE_STREAM_TO_UINT16(param_id, p);
        BE_STREAM_TO_UINT16(param_len, p);

        switch (param_id)
        {
        case PARAM_ID_DEV_HID_COMPONENT_ID:
            break;

        case PARAM_ID_DEV_HID_REPORT:
            break;

        default:
            break;
        }

        param += param_len;
    }
}

void bt_iap2_handle_ctrl_session(T_BT_IAP_LINK *link,
                                 uint8_t       *data)
{
    uint8_t *p = data;
    uint16_t msg_id;
    uint16_t param_len;

    p += 2;     /* start of message */
    BE_STREAM_TO_UINT16(param_len, p);
    param_len -= IAP2_MSG_HDR_SIZE;
    BE_STREAM_TO_UINT16(msg_id, p);

    PROFILE_PRINT_TRACE2("bt_iap2_handle_ctrl_session: msg_id 0x%04x, param len %u", msg_id, param_len);

    switch (msg_id)
    {
    case MSG_REQ_AUTHEN_CERT:
        bt_iap2_send_ack(link);
        bt_iap2_handle_req_authen_cert(link);
        break;

    case MSG_REQ_AUTHEN_CHALLENGE_RSP:
        bt_iap2_send_ack(link);
        bt_iap2_handle_req_authen_challenge_rsp_1(link, p, param_len);
        break;

    case MSG_AUTHEN_FAILED:
        bt_iap_authen_cmpl(link, false);
        bt_iap2_send_ack(link);
        break;

    case MSG_AUTHEN_SUCCEEDED:
        bt_iap2_send_ack(link);
        break;

    case MSG_START_IDENT:
        {
            T_BT_MSG_PAYLOAD payload;

            memcpy(payload.bd_addr, link->bd_addr, 6);
            payload.msg_buf = NULL;
            bt_mgr_dispatch(BT_MSG_IAP_START_IDENT_REQ, &payload);
        }
        break;

    case MSG_IDENT_ACCEPTED:
        bt_iap_authen_cmpl(link, true);
        bt_iap2_send_ack(link);
        break;

    case MSG_IDENT_REJECTED:
        bt_iap_authen_cmpl(link, false);
        bt_iap2_send_ack(link);
        break;

    case MSG_START_EA_PROTO_SESSION:
        bt_iap2_handle_start_eap_session(link, p, param_len);
        break;

    case MSG_STOP_EA_PROTO_SESSION:
        bt_iap2_handle_stop_eap_session(link, p, param_len);
        break;

    case MSG_DEV_HID_REPORT:
        bt_iap2_handle_dev_hid_report(link, p, param_len);
        bt_iap2_send_ack(link);
        break;

    default:
        {
            T_BT_IAP_CTRL_MSG_IND ctrl_msg;
            T_BT_MSG_PAYLOAD      payload;

            ctrl_msg.msg_id = msg_id;
            ctrl_msg.param_len = param_len;
            ctrl_msg.p_param = p;
            memcpy(payload.bd_addr, link->bd_addr, 6);
            payload.msg_buf = (void *)&ctrl_msg;
            bt_mgr_dispatch(BT_MSG_IAP_CTRL_MSG_IND, &payload);

            bt_iap2_send_ack(link);
        }
        break;
    }
}

void bt_iap2_handle_reset(T_BT_IAP_LINK *link)
{
    T_BT_IAP2_TX_DATA_ELEM *elem;
    T_BT_MSG_PAYLOAD        payload;

    if (link->state == BT_IAP_STATE_IAP2_SYNC)
    {
        return;
    }

    /* clear variable */
    if (link->authen_timer)
    {
        sys_timer_stop(link->authen_timer);
    }

    if (link->sync_timer)
    {
        sys_timer_stop(link->sync_timer);
    }

    if (link->detect_timer)
    {
        sys_timer_stop(link->detect_timer);
    }

    if (link->signature_timer)
    {
        sys_timer_stop(link->signature_timer);
    }

    if (link->payload)
    {
        free(link->payload);
        link->payload = NULL;
        link->header_offset = 0;
    }

    while (link->used_tx_q.count)
    {
        elem = os_queue_out(&link->used_tx_q);
        free(elem);
    }

    while (link->sent_tx_q.count)
    {
        elem = os_queue_out(&link->sent_tx_q);
        sys_timer_delete(elem->retrans_timer);
        free(elem);
    }

    /* start sync procedure */
    link->acc_pkt_seq = bt_iap->acc_start_seq;

    link->dev_retrans_tout = bt_iap->acc_retrans_tout;
    link->dev_cumulative_ack_tout = bt_iap->acc_cumulative_ack_tout;
    link->dev_max_retrans = bt_iap->acc_max_retrans;
    link->dev_max_cumulative_ack = bt_iap->acc_max_cumulative_ack;

    bt_iap2_send_sync(link);
    link->state = BT_IAP_STATE_IAP2_SYNC;
    sys_timer_start(link->sync_timer);

    memcpy(payload.bd_addr, link->bd_addr, 6);
    payload.msg_buf = NULL;
    bt_mgr_dispatch(BT_MSG_IAP_RESET, &payload);
}

void bt_iap_handle_iap2_pkt(T_BT_IAP_LINK *link,
                            uint8_t       *data,
                            uint16_t       len)
{
    uint8_t *p;
    uint8_t control;
    uint8_t session_id;
    uint8_t dev_pkt_seq;

    p = &link->header[4];
    BE_STREAM_TO_UINT8(control, p);
    BE_STREAM_TO_UINT8(dev_pkt_seq, p);
    BE_STREAM_TO_UINT8(link->acked_seq, p);
    BE_STREAM_TO_UINT8(session_id, p);

    PROFILE_PRINT_TRACE5("bt_iap_handle_iap2_pkt: control %d, dev_pkt_seq %d, link->dev_pkt_seq %d, link->acked_seq %d, session_id %d",
                         control, dev_pkt_seq, link->dev_pkt_seq, link->acked_seq, session_id);

    bt_iap2_update_acked_pkt(link);

    switch (control)
    {
    case IAP2_CTRL_SYNC_ACK:
        sys_timer_stop(link->sync_timer);
        link->dev_pkt_seq = dev_pkt_seq;
        bt_iap2_handle_ctrl_sync_ack(link, data);
        break;

    case IAP2_CTRL_ACK:
        if (len)
        {
            if (session_id == CTRL_SESSION_ID)
            {
                link->dev_pkt_seq = dev_pkt_seq;
                bt_iap2_handle_ctrl_session(link, data);
            }
            else if (session_id == EA_SESSION_ID)
            {
                T_BT_IAP_DATA_IND msg;
                T_BT_MSG_PAYLOAD  payload;

                BE_STREAM_TO_UINT16(msg.eap_session_id, data);
                msg.p_data = data;
                msg.dev_seq_num = dev_pkt_seq;
                msg.len = len - IAP2_EA_ID_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
                memcpy(payload.bd_addr, link->bd_addr, 6);
                payload.msg_buf = (void *)&msg;
                bt_mgr_dispatch(BT_MSG_IAP_DATA_IND, &payload);
            }
        }
        break;

    case IAP2_CTRL_RESET:
        bt_iap2_handle_reset(link);
        break;

    default:
        break;
    }
}

void bt_iap_handle_rcv_data(T_BT_IAP_LINK *link,
                            uint8_t       *data,
                            uint16_t       len)
{
    uint8_t *p = data;

    if ((data[0] == IAP2_START_BYTE1) && (data[1] == IAP2_START_BYTE2))
    {
        if (link->header_offset < link->header_len)
        {
            PROFILE_PRINT_WARN0("bt_iap_handle_rcv_data: rcv new header, discard pre header");
            link->header_offset = 0;
        }
        else if (link->payload != NULL)
        {
            PROFILE_PRINT_WARN0("bt_iap_handle_rcv_data: rcv new header, discard pre payload");
            free(link->payload);
            link->payload = NULL;
            link->header_offset = 0;
        }
    }

    while (len)
    {
        if (link->header_offset == 0 || link->header_offset < link->header_len)
        {
            if (link->header_offset == 0)
            {
                link->header_len = 2;     /* at least 1 start and 1 length for iAP */
            }

            link->header[link->header_offset] = *p++;
            link->header_offset++;
            len--;

            if (link->header_offset == 2)
            {
                if (link->header[0] == IAP2_START_BYTE1 && link->header[1] == IAP2_START_BYTE2)
                {
                    link->header_len = 9;
                }
                else if (link->header[0] == IAP_START_BYTE)
                {
                    if (link->header[1] == IAP_LENGTH_MARKER)
                    {
                        link->header_len += 2;    /* length is 3 bytes */
                    }
                }
                else if (link->header[0] == IAP2_START_BYTE1 && link->header[1] == IAP_START_BYTE)
                {
                    link->header_len++;   /* 0xFF55 is also iAP header, check next byte for length */
                }
                else
                {
                    link->header_offset = 0;  /* wrong header */
                }
            }
            else if (link->header_offset == 3)
            {
                if (link->header[0] == IAP2_START_BYTE1 && link->header[1] == IAP_START_BYTE)
                {
                    if (link->header[2] == IAP_LENGTH_MARKER)
                    {
                        link->header_len += 2;    /* length is 3 bytes */
                    }
                }
            }

            if (link->header_offset == link->header_len)
            {
                if (link->header[0] == IAP2_START_BYTE1 && link->header[1] == IAP2_START_BYTE2)
                {
                    BE_ARRAY_TO_UINT16(link->payload_len, &link->header[2]);
                    link->payload_len -= 9;   /* remove iAP2 header */
                    if (link->payload_len == 0)
                    {
                        bt_iap_handle_iap2_pkt(link, p, link->payload_len);
                        link->header_offset = 0;
                    }
                }
                else if (link->header[0] == IAP_START_BYTE)
                {
                    if (link->header[1] == IAP_LENGTH_MARKER)
                    {
                        BE_ARRAY_TO_UINT16(link->payload_len, &link->header[2]);
                    }
                    else
                    {
                        link->payload_len = link->header[1];
                    }
                    link->payload_len++;  /* add checksum */
                }
                else if (link->header[0] == IAP2_START_BYTE1 && link->header[1] == IAP_START_BYTE)
                {
                    if (link->header[2] == IAP_LENGTH_MARKER)
                    {
                        BE_ARRAY_TO_UINT16(link->payload_len, &link->header[3]);
                    }
                    else
                    {
                        link->payload_len = link->header[2];
                    }
                    link->payload_len++;  /* add checksum */
                }
            }
        }
        else    /* header is parsed, check len */
        {
            if (link->payload != NULL)
            {
                uint16_t copy_len = link->payload_len - link->payload_offset;

                if (copy_len > len)
                {
                    copy_len = len;
                }

                memcpy(link->payload + link->payload_offset, p, copy_len);
                link->payload_offset += copy_len;
                p += copy_len;
                len -= copy_len;

                if (link->payload_offset == link->payload_len)
                {
                    if (link->header[0] == IAP2_START_BYTE1 && link->header[1] == IAP2_START_BYTE2)
                    {
                        bt_iap_handle_iap2_pkt(link, link->payload, link->payload_len);
                    }
                    else
                    {
                        bt_iap_handle_iap_pkt(link, link->payload, link->payload_len);
                    }

                    free(link->payload);
                    link->payload = NULL;
                    link->header_offset = 0;
                }
            }
            else
            {
                if (link->payload_len <= len)
                {
                    if (link->header[0] == IAP2_START_BYTE1 && link->header[1] == IAP2_START_BYTE2)
                    {
                        bt_iap_handle_iap2_pkt(link, p, link->payload_len);
                    }
                    else
                    {
                        bt_iap_handle_iap_pkt(link, p, link->payload_len);
                    }

                    p += link->payload_len;
                    len -= link->payload_len;
                    link->header_offset = 0;
                }
                else
                {
                    link->payload = calloc(1, link->payload_len);

                    if (link->payload == NULL)
                    {
                        link->header_offset = 0;
                        return;
                    }

                    memcpy(link->payload, p, len);
                    link->payload_offset = len;
                    len = 0;
                }
            }
        }
    }
}

void bt_iap_handle_timeout(T_SYS_TIMER_HANDLE handle)
{
    uint8_t        iap_timer_id;
    uint32_t       timer_id;
    uint16_t       timer_chann;
    T_BT_IAP_LINK *link;

    timer_id = sys_timer_id_get(handle);
    timer_chann = (uint16_t)timer_id;
    iap_timer_id = (uint8_t)(timer_id >> 16);

    PROFILE_PRINT_TRACE3("bt_iap_handle_timeout: timer_id %x, iap_timer_id 0x%02x, timer_chann 0x%04x",
                         timer_id, iap_timer_id, timer_chann);

    link = os_queue_peek(&bt_iap->link_list, 0);
    while (link != NULL)
    {
        if (link->index == timer_chann)
        {
            break;
        }

        link = link->next;
    }

    if (link == NULL)
    {
        return;
    }

    switch (iap_timer_id)
    {
    case BT_IAP_AUTHEN_TOUT:
        bt_iap_authen_cmpl(link, false);
        rfc_disconn_req(link->bd_addr, link->dlci);
        break;

    case BT_IAP2_DETECT:
        bt_iap2_send_detect(link);
        sys_timer_start(link->detect_timer);
        break;

    case BT_IAP2_SYNC:
        bt_iap2_send_sync(link);
        sys_timer_start(link->sync_timer);
        break;

    case BT_IAP2_SEND_DATA:
        {
            T_BT_IAP2_TX_DATA_ELEM    *elem;

            elem = (T_BT_IAP2_TX_DATA_ELEM *)link->sent_tx_q.p_first;
            while (elem)
            {
                if (elem->data[5] == (uint8_t)(timer_chann >> 8))
                {
                    sys_timer_delete(elem->retrans_timer);
                    elem->retrans_cnt++;
                    if (elem->retrans_cnt >= link->dev_max_retrans)
                    {
                        PROFILE_PRINT_ERROR2("bt_iap_handle_timeout: retrans cnt %u reaches max num %u",
                                             elem->retrans_cnt, link->dev_max_retrans);
                        elem = os_queue_out(&link->sent_tx_q);
                        if (elem->session_id == EA_SESSION_ID)
                        {
                            T_BT_MSG_PAYLOAD           payload;
                            T_BT_IAP_DATA_TRANSMITTED  msg;

                            msg.eap_session_id = elem->ea_session_id;
                            msg.success = false;
                            memcpy(payload.bd_addr, link->bd_addr, 6);
                            payload.msg_buf = (void *)&msg;
                            bt_mgr_dispatch(BT_MSG_IAP_DATA_TRANSMITTED, &payload);
                        }
                        free(elem);
                        link->remain_tx_q_num++;

                        rfc_disconn_req(link->bd_addr, link->dlci);
                    }
                    else
                    {
                        elem->data[6] = link->dev_pkt_seq;
                        elem->data[8] = bt_iap_cal_checksum(elem->data, IAP2_PKT_HDR_SIZE - 1);
                        bt_iap_send_pkt(link, elem->data,
                                        elem->data_len + IAP2_PKT_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE);

                        elem->retrans_timer = sys_timer_create("iap2_data_trans",
                                                               SYS_TIMER_TYPE_LOW_PRECISION,
                                                               (BT_IAP2_SEND_DATA << 16) | timer_chann,
                                                               link->dev_retrans_tout * 1000,
                                                               false,
                                                               bt_iap_handle_timeout);

                        sys_timer_start(elem->retrans_timer);
                    }

                    break;
                }

                elem = elem->next;
            }
        }
        break;

    case BT_IAP2_SIGNATURE:
        {
            sys_timer_stop(link->signature_timer);
            bt_iap2_handle_req_authen_challenge_rsp_2(link);
        }
        break;

    default:
        break;
    }
}


void bt_iap_handle_rfc_conn_ind(T_RFC_CONN_IND *ind)
{
    T_BT_IAP_LINK    *link;
    T_BT_MSG_PAYLOAD  payload;

    link = bt_iap_find_link_by_addr(ind->bd_addr);
    if (link)
    {
        rfc_conn_cfm(ind->bd_addr, link->dlci, RFC_REJECT, 0, 0);
        return;
    }

    link = bt_iap_alloc_link(ind->bd_addr);
    if (link)
    {
        T_BT_IAP_CONN_IND conn_ind;

        link->dlci = ind->dlci;

        conn_ind.rfc_frame_size = ind->frame_size;
        memcpy(payload.bd_addr, link->bd_addr, 6);
        payload.msg_buf = (void *)&conn_ind;
        bt_mgr_dispatch(BT_MSG_IAP_CONN_IND, &payload);
    }
    else
    {
        rfc_conn_cfm(ind->bd_addr, ind->dlci, RFC_REJECT, 0, 0);
    }
}

void bt_iap_handle_rfc_conn_cmpl(T_RFC_CONN_CMPL *cmpl)
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(cmpl->bd_addr);
    if (link == NULL)
    {
        rfc_disconn_req(cmpl->bd_addr, cmpl->dlci);
        return;
    }

    link->rfc_frame_size = cmpl->frame_size;
    link->remote_credit = cmpl->remain_credits;
    link->state = BT_IAP_STATE_CONNECTED;

    if (link->remote_credit != 0)
    {
        if (bt_iap2_send_detect(link))
        {
            sys_timer_start(link->detect_timer);
            sys_timer_start(link->authen_timer);
            link->state = BT_IAP_STATE_DETECT;
        }
    }
}

void bt_iap_handle_rfc_disconn_cmpl(T_RFC_DISCONN_CMPL *disc)
{
    T_BT_IAP_LINK    *link;
    T_BT_IAP_STATE    state;

    link = bt_iap_find_link_by_addr(disc->bd_addr);
    if (link)
    {
        T_BT_MSG_PAYLOAD  payload;

        state = link->state;
        bt_iap_free_link(link);

        memcpy(payload.bd_addr, disc->bd_addr, 6);

        if (state < BT_IAP_STATE_IAP2_SYNC_CMPL)
        {
            payload.msg_buf = (void *)&disc->cause;
            bt_mgr_dispatch(BT_MSG_IAP_CONN_FAIL, &payload);
        }
        else
        {
            T_BT_IAP_DISCONN_INFO info;

            info.cause = disc->cause;
            payload.msg_buf = (void *)&info;
            bt_mgr_dispatch(BT_MSG_IAP_DISCONN_CMPL, &payload);
        }
    }
}

void bt_iap_handle_rfc_credit_info(T_RFC_CREDIT_INFO *credit)
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(credit->bd_addr);
    if (link)
    {
        link->remote_credit = credit->remain_credits;

        if (link->state == BT_IAP_STATE_CONNECTED)
        {
            if (bt_iap2_send_detect(link))
            {
                sys_timer_start(link->detect_timer);
                sys_timer_start(link->authen_timer);
                link->state = BT_IAP_STATE_DETECT;
            }
        }

        bt_iap2_tx_data(link);
    }
}

void bt_iap_handle_rfc_data_ind(T_RFC_DATA_IND *ind)
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(ind->bd_addr);
    if (link)
    {
        link->remote_credit = ind->remain_credits;

        if (ind->length)
        {
            bt_iap_handle_rcv_data(link, ind->buf, ind->length);
        }

        bt_iap2_tx_data(link);
    }

    rfc_data_cfm(ind->bd_addr, ind->dlci, 1);
}

void bt_iap_handle_rfc_dlci_change(T_RFC_DLCI_CHANGE_INFO *info)
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(info->bd_addr);
    if (link)
    {
        link->dlci = info->curr_dlci;
    }
}

void bt_iap_rfc_cb(T_RFC_MSG_TYPE  msg_type,
                   void           *msg)
{
    PROFILE_PRINT_TRACE1("bt_iap_rfc_cb: msg_type %d", msg_type);

    switch (msg_type)
    {
    case RFC_CONN_IND:
        bt_iap_handle_rfc_conn_ind((T_RFC_CONN_IND *)msg);
        break;

    case RFC_CONN_CMPL:
        bt_iap_handle_rfc_conn_cmpl((T_RFC_CONN_CMPL *)msg);
        break;


    case RFC_DISCONN_CMPL:
        bt_iap_handle_rfc_disconn_cmpl((T_RFC_DISCONN_CMPL *)msg);
        break;

    case RFC_CREDIT_INFO:
        bt_iap_handle_rfc_credit_info((T_RFC_CREDIT_INFO *)msg);
        break;

    case RFC_DATA_IND:
        bt_iap_handle_rfc_data_ind((T_RFC_DATA_IND *)msg);
        break;

    case RFC_DLCI_CHANGE:
        bt_iap_handle_rfc_dlci_change((T_RFC_DLCI_CHANGE_INFO *)msg);
        break;

    default:
        break;
    }
}

bool bt_iap_init(uint8_t         rfc_chann_num,
                 P_CP_WRITE_FUNC cp_write,
                 P_CP_READ_FUNC  cp_read)
{
    int32_t ret = 0;

    bt_iap = calloc(1, sizeof(T_BT_IAP));
    if (bt_iap == NULL)
    {
        ret = 1;
        goto fail_alloc_iap;
    }

    if (rfc_cback_register(rfc_chann_num,
                           bt_iap_rfc_cb,
                           &bt_iap->iap_service_id) == false)
    {
        ret = 2;
        goto fail_reg_rfc_cb;
    }

    cp_init(cp_write, cp_read);

    bt_iap->acc_max_pkt_len = 0x0320;
    bt_iap->acc_retrans_tout = 0x01f4;
    bt_iap->acc_cumulative_ack_tout = 0x0064;
    bt_iap->acc_max_out_pkt = 0x04;
    bt_iap->acc_max_retrans = 0x04;
    bt_iap->acc_max_cumulative_ack = 0x04;
    bt_iap->acc_start_seq = 0xC0;
    bt_iap->tx_q_elem_num = 3;

    return true;

fail_reg_rfc_cb:
    free(bt_iap);
    bt_iap = NULL;
fail_alloc_iap:
    PROFILE_PRINT_ERROR1("bt_iap_init: failed %d", -ret);
    return false;
}

bool bt_iap_param_set(T_BT_IAP_PARAM_TYPE  type,
                      uint8_t              len,
                      void                *value)
{
    switch (type)
    {
    case BT_IAP_PARAM_ACC_MAX_PKT_LEN:
        if (len == sizeof(uint16_t) && (*((uint16_t *)value) >= 24))
        {
            bt_iap->acc_max_pkt_len = *((uint16_t *)value);
            return true;
        }
        break;

    case BT_IAP_PARAM_ACC_RETRANS_TOUT:
        if (len == sizeof(uint16_t) && (*((uint16_t *)value) >= 20))
        {
            bt_iap->acc_retrans_tout = *((uint16_t *)value);
            return true;
        }
        break;

    case BT_IAP_PARAM_ACC_CUMULATIVE_ACK_TOUT:
        if (len == sizeof(uint16_t) && (*((uint16_t *)value) >= 10) &&
            (*((uint16_t *)value) <= (bt_iap->acc_retrans_tout / 2)))
        {
            bt_iap->acc_cumulative_ack_tout = *((uint16_t *)value);
            return true;
        }
        break;

    case BT_IAP_PARAM_ACC_MAX_OUT_PKT_NUM:
        if (len == sizeof(uint8_t) && (*((uint8_t *)value) > 0) && (*((uint8_t *)value) <= 127))
        {
            bt_iap->acc_max_out_pkt = *((uint8_t *)value);
            return true;
        }
        break;

    case BT_IAP_PARAM_ACC_MAX_RETRANS_NUM:
        if (len == sizeof(uint8_t) && (*((uint8_t *)value) > 0) && (*((uint8_t *)value) <= 30))
        {
            bt_iap->acc_max_retrans = *((uint8_t *)value);
            return true;
        }
        break;

    case BT_IAP_PARAM_ACC_MAX_CULUMATIVE_ACK:
        if (len == sizeof(uint8_t) && (*((uint8_t *)value) <= bt_iap->acc_max_out_pkt))
        {
            bt_iap->acc_max_cumulative_ack = *((uint8_t *)value);
            return true;
        }
        break;

    case BT_IAP_PARAM_ACC_START_SEQ_NUM:
        if (len == sizeof(uint8_t))
        {
            bt_iap->acc_start_seq = *((uint8_t *)value);
            return true;
        }
        break;

    case BT_IAP_PARAM_TX_Q_ELEM_NUM:
        if (len == sizeof(uint8_t))
        {
            bt_iap->tx_q_elem_num = *((uint8_t *)value);
            return true;
        }
        break;

    default:
        break;
    }

    return false;
}

bool bt_iap_connect_req(uint8_t  bd_addr[6],
                        uint8_t  server_chann,
                        uint16_t frame_size,
                        uint8_t  init_credits)
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_iap_alloc_link(bd_addr);
    }

    if (link)
    {
        if (link->state == BT_IAP_STATE_DISCONNECTED)
        {
            uint8_t        dlci;

            if (rfc_conn_req(bd_addr,
                             server_chann,
                             bt_iap->iap_service_id,
                             frame_size,
                             init_credits,
                             &dlci) == true)
            {
                link->dlci = dlci;
                link->state = BT_IAP_STATE_CONNECTING;
                return true;
            }
            else
            {
                bt_iap_free_link(link);
            }
        }
    }

    return false;
}

bool bt_iap_connect_cfm(uint8_t  bd_addr[6],
                        bool     accept,
                        uint16_t frame_size,
                        uint8_t  init_credits)
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link)
    {
        if (accept)
        {
            link->state = BT_IAP_STATE_CONNECTING;
            rfc_conn_cfm(link->bd_addr, link->dlci, RFC_ACCEPT, frame_size, init_credits);
        }
        else
        {
            rfc_conn_cfm(link->bd_addr, link->dlci, RFC_REJECT, 0, 0);
            bt_iap_free_link(link);
        }

        return true;
    }

    return false;
}

bool bt_iap_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link)
    {
        link->state = BT_IAP_STATE_DISCONNECTING;
        bt_sniff_mode_exit(bd_addr, false);
        return rfc_disconn_req(link->bd_addr, link->dlci);
    }

    return false;
}

bool bt_iap_ident_info_send(uint8_t   bd_addr[6],
                            uint8_t  *ident,
                            uint16_t  ident_len)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint8_t                *data;
    uint16_t                total_len;
    int32_t                 ret = 0;

    PROFILE_PRINT_INFO1("bt_iap_ident_info_send: ident_len %d", ident_len);

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = ident_len + IAP2_CTRL_MSG_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = ident_len + IAP2_MSG_HDR_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    data = elem->data;
    memcpy(data + IAP2_CTRL_MSG_HDR_SIZE, ident, ident_len);
    bt_iap2_wrap_msg(link, data, MSG_IDENT_INFO, ident_len);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_ident_info_send: failed %d, addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_eap_session_status_send(uint8_t                     bd_addr[6],
                                    uint16_t                    session_id,
                                    T_BT_IAP_EAP_SESSION_STATUS status)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *data_buf;
    uint8_t                 data[2];
    int32_t                 ret = 0;

    PROFILE_PRINT_INFO2("bt_iap_eap_session_status_send: session_id 0x%02x, status %d",
                        session_id, status);

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = IAP2_CTRL_MSG_HDR_SIZE + (IAP2_PARAM_HDR_SIZE + sizeof(uint16_t)) +
                (IAP2_PARAM_HDR_SIZE + sizeof(T_BT_IAP_EAP_SESSION_STATUS)) + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    data_buf = elem->data;
    data_buf += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_ARRAY(data, session_id);
    data_buf = bt_iap2_prep_param(data_buf, EAP_SESSION_STATUS_PARAM_ID_SESSION_ID, data,
                                  sizeof(uint16_t));

    bt_iap2_prep_param(data_buf, EAP_SESSION_STATUS_PARAM_ID_SESSION_STATUS, &status,
                       sizeof(T_BT_IAP_EAP_SESSION_STATUS));

    bt_iap2_wrap_msg(link, elem->data, MSG_EA_PROTO_SESSION_STATUS,
                     elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_eap_session_status_send: failed %d, addr %s",
                         -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_app_launch(uint8_t                     bd_addr[6],
                       char                       *boundle_id,
                       uint8_t                     len_boundle_id,
                       T_BT_IAP_APP_LAUNCH_METHOD  method)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *data;
    int32_t                 ret = 0;

    PROFILE_PRINT_INFO2("bt_iap_app_launch: boundle_id %s, method %d",
                        TRACE_STRING(boundle_id), method);

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = len_boundle_id + IAP2_CTRL_MSG_PARAM_HDR_SIZE + IAP2_PARAM_HDR_SIZE +
                sizeof(T_BT_IAP_APP_LAUNCH_METHOD) + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    data = elem->data;
    data = bt_iap2_prep_param(data + IAP2_CTRL_MSG_HDR_SIZE, PARAM_ID_APP_BUNDLE_ID, boundle_id,
                              len_boundle_id);
    bt_iap2_prep_param(data, PARAM_ID_APP_LAUNCH_METHOD, &method, sizeof(T_BT_IAP_APP_LAUNCH_METHOD));
    bt_iap2_wrap_msg(link, elem->data, MSG_REQ_APP_LAUNCH, elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_app_launch: failed %d, addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_hid_start(uint8_t   bd_addr[6],
                      uint16_t  hid_component_id,
                      uint16_t  vid,
                      uint16_t  pid,
                      uint8_t  *hid_report_desc,
                      uint16_t  hid_report_desc_len)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *p;
    uint8_t                 data[2];
    int32_t                 ret = 0;

    PROFILE_PRINT_TRACE3("bt_iap_hid_start: hid component identifier 0x%x, vid 0x%x, pid 0x%x",
                         hid_component_id, vid, pid);

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint16_t) +
                (IAP2_PARAM_HDR_SIZE + sizeof(uint16_t)) +
                (IAP2_PARAM_HDR_SIZE + sizeof(uint16_t)) +
                (IAP2_PARAM_HDR_SIZE + hid_report_desc_len) + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    p = elem->data;
    p += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_ARRAY(data, hid_component_id);
    p = bt_iap2_prep_param(p, PARAM_ID_HID_COMPONENT_ID, data, sizeof(uint16_t));

    BE_UINT16_TO_ARRAY(data, vid);
    p = bt_iap2_prep_param(p, PARAM_ID_VENDOR_ID, data, sizeof(uint16_t));

    BE_UINT16_TO_ARRAY(data, pid);
    p = bt_iap2_prep_param(p, PARAM_ID_PRODUCT_ID, data, sizeof(uint16_t));

    p = bt_iap2_prep_param(p, PARAM_ID_HID_REPORT_DESCRIPTOR, hid_report_desc, hid_report_desc_len);

    bt_iap2_wrap_msg(link, elem->data, MSG_START_HID, elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_hid_start: failed %d, addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_hid_report_send(uint8_t   bd_addr[6],
                            uint16_t  hid_component_id,
                            uint8_t  *hid_report,
                            uint16_t  hid_report_len)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *p;
    uint8_t                 data[2];
    int32_t                 ret = 0;

    PROFILE_PRINT_INFO2("bt_iap_hid_report_send: hid component identifier 0x%x, hid report len %u",
                        hid_component_id, hid_report_len);

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint16_t) +
                (IAP2_PARAM_HDR_SIZE + hid_report_len) + IAP2_PAYLOAD_CHECKSUM_SIZE;

    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    p = elem->data;
    p += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_ARRAY(data, hid_component_id);
    p = bt_iap2_prep_param(p, PARAM_ID_ACC_HID_COMPONENT_ID, data, sizeof(uint16_t));
    p = bt_iap2_prep_param(p, PARAM_ID_ACC_HID_REPORT, hid_report, hid_report_len);

    bt_iap2_wrap_msg(link, elem->data, MSG_ACC_HID_REPORT, elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_hid_report_send: failed %d, addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_hid_stop(uint8_t  bd_addr[6],
                     uint16_t hid_component_id)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *p;
    uint8_t                 data[2];
    int32_t                 ret = 0;

    PROFILE_PRINT_INFO1("bt_iap_hid_stop: hid component identifier 0x%x", hid_component_id);

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint16_t) + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    p = elem->data;

    BE_UINT16_TO_ARRAY(data, hid_component_id);
    p += IAP2_CTRL_MSG_HDR_SIZE;
    bt_iap2_prep_param(p, PARAM_ID_HID_COMPONENT_ID, data, sizeof(uint16_t));
    bt_iap2_wrap_msg(link, elem->data, MSG_STOP_HID, elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_hid_stop: failed %d, addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_comp_info_send(uint8_t  bd_addr[6],
                           uint16_t comp_id,
                           bool     enable)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *p;
    int32_t                 ret = 0;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE +
                (IAP2_PARAM_HDR_SIZE + sizeof(uint16_t)) +
                (IAP2_PARAM_HDR_SIZE + sizeof(uint8_t));
    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    p = elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_STREAM(p, IAP2_PARA_HDR_SIZE + 1 + IAP2_PARA_HDR_SIZE + IAP2_PARA_HDR_SIZE + 2);
    BE_UINT16_TO_STREAM(p, PARAM_ID_BT_COMPONENT_STATUS);

    BE_UINT16_TO_STREAM(p, IAP2_PARA_HDR_SIZE + 2);
    BE_UINT16_TO_STREAM(p, PARAM_ID_COMPONENT_ID);
    BE_UINT16_TO_STREAM(p, comp_id);

    BE_UINT16_TO_STREAM(p, IAP2_PARA_HDR_SIZE + 1);
    BE_UINT16_TO_STREAM(p, PARAM_ID_COMPONENT_ENABLED);
    BE_UINT8_TO_STREAM(p, enable);

    bt_iap2_wrap_msg(link, elem->data, MSG_BT_COMPONENT_INFO, elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_comp_info_send: failed %d, addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_conn_update_start(uint8_t  bd_addr[6],
                              uint16_t comp_id)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *p;
    uint8_t                 data[2];
    int32_t                 ret = 0;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint16_t) + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    p = elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_ARRAY(data, comp_id);
    bt_iap2_prep_param(p, PARAM_ID_COMPONENT_ID, data, sizeof(uint16_t));
    bt_iap2_wrap_msg(link, elem->data, MSG_START_BT_CONN_UPDATES,
                     elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_conn_update_start: failed %d, addr %s", -ret,
                         TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_comm_update_start(uint8_t  bd_addr[6],
                              uint16_t param_id)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *p;
    int32_t                 ret = 0;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = IAP2_CTRL_MSG_PAYLOAD_HDR_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    p = elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    bt_iap2_prep_param(p, param_id, NULL, 0);
    bt_iap2_wrap_msg(link, elem->data, MSG_START_COMM_UPDATES, IAP2_PARAM_HDR_SIZE);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_comm_update_start: failed %d, addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_mute_status_update(uint8_t bd_addr[6],
                               bool    status)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *p;
    int32_t                 ret = 0;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint8_t) + IAP2_PAYLOAD_CHECKSUM_SIZE;

    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    elem->session_id = CTRL_SESSION_ID;

    p = elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    bt_iap2_prep_param(p, PARAM_ID_MUTE_STATUS, &status, sizeof(uint8_t));
    bt_iap2_wrap_msg(link, elem->data, MSG_MUTE_STATUS_UPDATE,
                     elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_mute_status_update: failed %d, addr %s",
                         -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_send_cmd(uint8_t   bd_addr[6],
                     uint16_t  msg_type,
                     uint8_t  *param,
                     uint16_t  param_len)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *p;
    int32_t                 ret = 0;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = IAP2_CTRL_MSG_HDR_SIZE + param_len + IAP2_PAYLOAD_CHECKSUM_SIZE;

    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->data_len = IAP2_MSG_HDR_SIZE + param_len;
    elem->session_id = CTRL_SESSION_ID;

    p = elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    memcpy(p, param, param_len);
    bt_iap2_wrap_msg(link, elem->data, msg_type, param_len);

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_send_cmd: failed %d, addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_data_send(uint8_t   bd_addr[6],
                      uint16_t  session_id,
                      uint8_t  *data,
                      uint16_t  data_len)
{
    T_BT_IAP_LINK          *link;
    T_BT_IAP2_TX_DATA_ELEM *elem;
    uint16_t                total_len;
    uint8_t                *p;
    int32_t                 ret = 0;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    total_len = data_len + IAP2_EA_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > link->dev_max_pkt_len)
    {
        ret = 2;
        goto fail_length_exceed;
    }

    if (link->remain_tx_q_num == 0)
    {
        ret = 3;
        goto fail_no_tx_elem;
    }

    elem = calloc(1, total_len + sizeof(T_BT_IAP2_TX_DATA_ELEM));
    if (elem == NULL)
    {
        ret = 4;
        goto fail_elem_null;
    }

    link->remain_tx_q_num--;
    elem->ea_session_id = session_id;
    elem->data_len = data_len + IAP2_EA_ID_SIZE;
    elem->session_id = EA_SESSION_ID;

    p = elem->data;

    /* fill in header data later */
    p += IAP2_PKT_HDR_SIZE;
    BE_UINT16_TO_STREAM(p, session_id);
    memcpy(p, data, data_len);
    p += data_len;
    BE_UINT8_TO_STREAM(p, bt_iap_cal_checksum((uint8_t *)elem->data + IAP2_PKT_HDR_SIZE,
                                              elem->data_len));

    os_queue_in(&link->used_tx_q, elem);

    bt_iap2_tx_data(link);

    return true;

fail_elem_null:
fail_no_tx_elem:
fail_length_exceed:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("bt_iap_data_send: failed %d, addr %s", -ret, TRACE_BDADDR(bd_addr));
    return false;
}

bool bt_iap_ack_send(uint8_t bd_addr[6],
                     uint8_t ack_seq)
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link)
    {
        if (bt_iap2_send_ack(link) == true)
        {
            link->dev_pkt_seq = ack_seq;
            return true;
        }
    }

    return false;
}

bool bt_iap_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_IAP_INFO *info)
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        memcpy(info->bd_addr, bd_addr, 6);
        info->remote_credit = link->remote_credit;
        info->rfc_frame_size = link->rfc_frame_size;
        info->dlci = link->dlci;

        info->dev_max_pkt_len = link->dev_max_pkt_len;
        info->dev_retrans_tout = link->dev_retrans_tout;
        info->dev_cumulative_ack_tout = link->dev_cumulative_ack_tout;
        info->dev_max_out_pkt = link->dev_max_out_pkt;
        info->dev_max_retrans = link->dev_max_retrans;
        info->dev_max_cumulative_ack = link->dev_max_cumulative_ack;
        info->acked_seq = link->acked_seq;
        info->acc_pkt_seq = link->acc_pkt_seq;
        info->dev_pkt_seq = link->dev_pkt_seq;
        info->state = link->state;

        return true;
    }

    return false;
}

bool bt_iap_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_IAP_INFO *info)
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_iap_alloc_link(bd_addr);
    }

    if (link)
    {
        link->state = (T_BT_IAP_STATE)info->state;
        link->remote_credit = info->remote_credit;
        link->rfc_frame_size = info->rfc_frame_size;
        link->dlci = info->dlci;

        link->dev_max_pkt_len = info->dev_max_pkt_len;
        link->dev_retrans_tout = info->dev_retrans_tout;
        link->dev_cumulative_ack_tout = info->dev_cumulative_ack_tout;
        link->dev_max_out_pkt = info->dev_max_out_pkt;
        link->dev_max_retrans = info->dev_max_retrans;
        link->dev_max_cumulative_ack = info->dev_max_cumulative_ack;
        link->acked_seq = info->acked_seq;
        link->dev_pkt_seq = info->dev_pkt_seq;
        link->acc_pkt_seq = info->acc_pkt_seq;

        if (link->state > BT_IAP_STATE_IAP2_SYNC)
        {
            T_BT_IAP2_TX_DATA_ELEM *elem;

            if (link->payload)
            {
                free(link->payload);
            }

            while (link->used_tx_q.count)
            {
                elem = os_queue_out(&link->used_tx_q);
                if (elem != NULL)
                {
                    free(elem);
                }
            }

            while (link->sent_tx_q.count)
            {
                elem = os_queue_out(&link->sent_tx_q);
                if (elem != NULL)
                {
                    sys_timer_delete(elem->retrans_timer);
                    free(elem);
                }
            }

            os_queue_init(&link->used_tx_q);
            os_queue_init(&link->sent_tx_q);

            if (link->dev_max_pkt_len > link->rfc_frame_size)
            {
                link->dev_max_pkt_len = link->rfc_frame_size;
            }

            link->remain_tx_q_num = bt_iap->tx_q_elem_num;
        }

        return true;
    }

    return false;
}

bool bt_iap_roleswap_info_del(uint8_t bd_addr[6])
{
    T_BT_IAP_LINK *link;

    link = bt_iap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        return false;
    }

    bt_iap_free_link(link);

    return true;
}

bool bt_iap_service_id_get(uint8_t *service_id)
{
    if (bt_iap != NULL)
    {
        *service_id = bt_iap->iap_service_id;
        return true;
    }

    return false;
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bt_iap_int.h"
#include "bt_iap.h"

bool bt_iap_init(uint8_t         rfc_chann_num,
                 P_CP_WRITE_FUNC cp_write,
                 P_CP_READ_FUNC  cp_read)
{
    return false;
}

bool bt_iap_param_set(T_BT_IAP_PARAM_TYPE  type,
                      uint8_t              len,
                      void                *value)
{
    return false;
}

bool bt_iap_connect_req(uint8_t  bd_addr[6],
                        uint8_t  server_chann,
                        uint16_t frame_size,
                        uint8_t  init_credits)
{
    return false;
}

bool bt_iap_connect_cfm(uint8_t  bd_addr[6],
                        bool     accept,
                        uint16_t frame_size,
                        uint8_t  init_credits)
{
    return false;
}

bool bt_iap_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

uint8_t *bt_iap2_prep_param(uint8_t  *param,
                            uint16_t  param_id,
                            void     *data,
                            uint16_t  data_len)
{
    return NULL;
}

bool bt_iap_ident_info_send(uint8_t   bd_addr[6],
                            uint8_t  *ident,
                            uint16_t  ident_len)
{
    return false;
}

bool bt_iap_eap_session_status_send(uint8_t                     bd_addr[6],
                                    uint16_t                    session_id,
                                    T_BT_IAP_EAP_SESSION_STATUS status)
{
    return false;
}

bool bt_iap_app_launch(uint8_t                     bd_addr[6],
                       char                       *boundle_id,
                       uint8_t                     len_boundle_id,
                       T_BT_IAP_APP_LAUNCH_METHOD  method)
{
    return false;
}

bool bt_iap_hid_start(uint8_t   bd_addr[6],
                      uint16_t  hid_component_id,
                      uint16_t  vid, uint16_t pid,
                      uint8_t  *hid_report_desc,
                      uint16_t  hid_report_desc_len)
{
    return false;
}

bool bt_iap_hid_report_send(uint8_t   bd_addr[6],
                            uint16_t  hid_component_id,
                            uint8_t  *hid_report,
                            uint16_t  hid_report_len)
{
    return false;
}

bool bt_iap_hid_stop(uint8_t  bd_addr[6],
                     uint16_t hid_component_id)
{
    return false;
}

bool bt_iap_comp_info_send(uint8_t  bd_addr[6],
                           uint16_t comp_id,
                           bool     enable)
{
    return false;
}

bool bt_iap_conn_update_start(uint8_t  bd_addr[6],
                              uint16_t comp_id)
{
    return false;
}

bool bt_iap_comm_update_start(uint8_t  bd_addr[6],
                              uint16_t param_id)
{
    return false;
}

bool bt_iap_mute_status_update(uint8_t bd_addr[6],
                               bool    status)
{
    return false;
}

bool bt_iap_send_cmd(uint8_t   bd_addr[6],
                     uint16_t  msg_type,
                     uint8_t  *param,
                     uint16_t  param_len)
{
    return false;
}

bool bt_iap_data_send(uint8_t   bd_addr[6],
                      uint16_t  session_id,
                      uint8_t  *data,
                      uint16_t  data_len)
{
    return false;
}

bool bt_iap_ack_send(uint8_t bd_addr[6],
                     uint8_t ack_seq)
{
    return false;
}

bool bt_iap_roleswap_info_get(uint8_t              bd_addr[6],
                              T_ROLESWAP_IAP_INFO *info)
{
    return false;
}

bool bt_iap_roleswap_info_set(uint8_t              bd_addr[6],
                              T_ROLESWAP_IAP_INFO *info)
{
    return false;
}

bool bt_iap_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}

bool bt_iap_service_id_get(uint8_t *service_id)
{
    return false;
}
#endif
