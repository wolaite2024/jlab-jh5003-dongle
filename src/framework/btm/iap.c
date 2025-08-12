/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_IAP_SUPPORT == 1)
#include <string.h>
#include "os_mem.h"
#include "os_sync.h"
#include "os_sched.h"
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "sys_timer.h"
#include "rfc.h"
#include "iap_cp.h"
#include "iap.h"

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

typedef enum t_iap_timer_id
{
    IAP_AUTHEN_TOUT = 0x00,
    IAP2_DETECT     = 0x01,
    IAP2_SYNC       = 0x02,
    IAP2_SEND_DATA  = 0x03,
    IAP2_SIGNATURE  = 0x04,
} T_IAP_TIMER_ID;

typedef enum t_iap2_session_type
{
    IAP2_SESSION_TYPE_CTRL               = 0x00,
    IAP2_SESSION_TYPE_FILE_TRANSFER      = 0x01,
    IAP2_SESSION_TYPE_EXTERNAL_ACCESSORY = 0x02,
} T_IAP2_SESSION_TYPE;

typedef enum t_iap_state
{
    IAP_STATE_DISCONNECTED      = 0x00,
    IAP_STATE_ALLOCATED         = 0x01,
    IAP_STATE_CONNECTING        = 0x02,
    IAP_STATE_CONNECTED         = 0x03,
    IAP_STATE_DETECT            = 0x04,
    IAP_STATE_IAP2_SYNC         = 0x05,
    IAP_STATE_IAP2_SYNC_CMPL    = 0x06,
    IAP_STATE_IAP2_AUTHEN       = 0x07,
    IAP_STATE_IAP2_CHALLENGE    = 0x08,
    IAP_STATE_IAP2_IDENT        = 0x09,
    IAP_STATE_IAP2_IDENTED      = 0x0A,
} T_IAP_STATE;

typedef struct t_iap_link
{
    T_IAP_STATE     state;
    uint8_t         index;
    uint8_t         bd_addr[6];
    uint8_t         dlci;
    uint8_t         remote_credit;
    uint16_t        rfc_frame_size;

    T_SYS_TIMER_HANDLE   authen_timer;
    T_SYS_TIMER_HANDLE   sync_timer;
    T_SYS_TIMER_HANDLE   detect_timer;
    T_SYS_TIMER_HANDLE   signature_timer;

    T_OS_QUEUE      used_tx_q;
    T_OS_QUEUE      sent_tx_q;

    uint8_t         header[9];
    uint8_t         header_offset;
    uint8_t         header_len;
    uint16_t        payload_len;
    uint16_t        payload_offset;
    uint8_t        *p_payload;

    uint8_t         remain_tx_q_num;
    uint8_t         acc_pkt_seq;        /* tx seq */
    uint8_t         acked_seq;          /* seq num acked by device */
    uint8_t         dev_pkt_seq;        /* received dev seq num */

    uint16_t        dev_max_pkt_len;    /* max pkt len to send */
    uint16_t        dev_retrans_tout;
    uint16_t        dev_cumulative_ack_tout;
    uint8_t         dev_max_out_pkt;
    uint8_t         dev_max_retrans;
    uint8_t         dev_max_culumative_ack;
} T_IAP_LINK;

typedef struct t_iap
{
    P_IAP_CB    app_callback;
    T_IAP_LINK *p_link;

    uint8_t     iap_rfc_index;
    uint8_t     tx_q_elem_num;

    /* parameters used for sync, can be changed by app using set param API */
    uint16_t    acc_max_pkt_len;
    uint16_t    acc_retrans_tout;
    uint16_t    acc_cumulative_ack_tout;
    uint8_t     acc_max_out_pkt;
    uint8_t     acc_max_retrans;
    uint8_t     acc_max_culumative_ack;
    uint8_t     acc_start_seq;
    uint8_t     link_num;
} T_IAP;

typedef struct t_iap2_tx_data_elem
{
    struct t_iap2_tx_data_elem *p_next;
    T_SYS_TIMER_HANDLE          retrans_timer;
    uint16_t                    ea_session_id;
    uint16_t                    data_len; /* payload len exclude iAP2 pkt hdr and payload checksum */
    uint8_t                     session_id;
    uint8_t                     retrans_cnt;
    uint8_t                     data[1];
} T_IAP2_TX_DATA_ELEM;

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
    IAP2_SESSION_TYPE_CTRL,     /* iAP2 Session 1: Session Type */
    0x01,                       /* iAP2 Session 1: Session Version */

    EA_SESSION_ID,                          /* iAP2 Session 2: Session ID */
    IAP2_SESSION_TYPE_EXTERNAL_ACCESSORY,   /* iAP2 Session 2: Session Type */
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

T_IAP *p_iap;

void iap_handle_timeout(T_SYS_TIMER_HANDLE handle);

T_IAP_LINK *iap_alloc_link(uint8_t bd_addr[6])
{
    uint8_t i;
    T_IAP_LINK *p_link;
    uint32_t ret = 0;

    for (i = 0; i < p_iap->link_num; i++)
    {
        p_link = &p_iap->p_link[i];

        if (p_link->state == IAP_STATE_DISCONNECTED)
        {
            p_link->authen_timer = sys_timer_create("iap_authen",
                                                    SYS_TIMER_TYPE_LOW_PRECISION,
                                                    (IAP_AUTHEN_TOUT << 16) | i,
                                                    15000 * 1000,
                                                    false,
                                                    iap_handle_timeout);
            if (p_link->authen_timer == NULL)
            {
                ret = 1;
                goto fail_create_authen_timer;
            }

            p_link->sync_timer = sys_timer_create("iap_sync",
                                                  SYS_TIMER_TYPE_LOW_PRECISION,
                                                  (IAP2_SYNC << 16) | i,
                                                  1000 * 1000,
                                                  false,
                                                  iap_handle_timeout);
            if (p_link->sync_timer == NULL)
            {
                ret = 2;
                goto fail_create_sync_timer;
            }

            p_link->detect_timer = sys_timer_create("iap_detect_retrans",
                                                    SYS_TIMER_TYPE_LOW_PRECISION,
                                                    (IAP2_DETECT << 16) | i,
                                                    1000 * 1000,
                                                    false,
                                                    iap_handle_timeout);
            if (p_link->detect_timer == NULL)
            {
                ret = 3;
                goto fail_create_detect_retrans_timer;
            }

            p_link->signature_timer = sys_timer_create("iap_signature",
                                                       SYS_TIMER_TYPE_LOW_PRECISION,
                                                       (IAP2_SIGNATURE << 16) | i,
                                                       500 * 1000,
                                                       false,
                                                       iap_handle_timeout);
            if (p_link->signature_timer == NULL)
            {
                ret = 4;
                goto fail_create_signature_timer;
            }

            memcpy(p_link->bd_addr, bd_addr, 6);
            p_link->state = IAP_STATE_ALLOCATED;
            p_link->index = i;
            return p_link;
        }
    }

    return NULL;

fail_create_signature_timer:
    sys_timer_delete(p_link->detect_timer);
fail_create_detect_retrans_timer:
    sys_timer_delete(p_link->sync_timer);
fail_create_sync_timer:
    sys_timer_delete(p_link->authen_timer);
fail_create_authen_timer:
    PROFILE_PRINT_ERROR1("iap_alloc_link: failed %d", ret);
    return NULL;
}

void iap_free_link(T_IAP_LINK *p_link)
{
    T_IAP2_TX_DATA_ELEM *p_elem;

    if (p_link->authen_timer != NULL)
    {
        sys_timer_delete(p_link->authen_timer);
    }

    if (p_link->sync_timer != NULL)
    {
        sys_timer_delete(p_link->sync_timer);
    }

    if (p_link->detect_timer != NULL)
    {
        sys_timer_delete(p_link->detect_timer);
    }

    if (p_link->signature_timer != NULL)
    {
        sys_timer_delete(p_link->signature_timer);
    }

    if (p_link->p_payload)
    {
        os_mem_free(p_link->p_payload);
    }

    while (p_link->used_tx_q.count)
    {
        p_elem = os_queue_out(&p_link->used_tx_q);
        os_mem_free(p_elem);
    }

    while (p_link->sent_tx_q.count)
    {
        p_elem = os_queue_out(&p_link->sent_tx_q);

        if (p_elem->retrans_timer != NULL)
        {
            sys_timer_delete(p_elem->retrans_timer);
        }

        os_mem_free(p_elem);
    }

    memset(p_link, 0, sizeof(T_IAP_LINK));
}

T_IAP_LINK *iap_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t i;
    T_IAP_LINK *p_link;

    for (i = 0; i < p_iap->link_num; i++)
    {
        p_link = &p_iap->p_link[i];

        if ((p_link->state != IAP_STATE_DISCONNECTED) && (memcmp(p_link->bd_addr, bd_addr, 6) == 0))
        {
            return p_link;
        }
    }

    return NULL;
}

bool iap_send_pkt(T_IAP_LINK *p_link,
                  uint8_t    *p_data,
                  uint16_t    data_len)
{
    if (data_len > (p_link->remote_credit * p_link->rfc_frame_size))
    {
        PROFILE_PRINT_ERROR3("iap_send_pkt: data len %u exceed, frame size %u credits %u",
                             data_len, p_link->rfc_frame_size, p_link->remote_credit);
        return false;
    }

    while (data_len)
    {
        uint16_t pkt_len;

        pkt_len = (data_len > p_link->rfc_frame_size) ? p_link->rfc_frame_size : data_len;

        if (rfc_data_req(p_link->bd_addr, p_link->dlci, p_data, pkt_len, false) == true)
        {
            p_link->remote_credit--;
        }
        else
        {
            PROFILE_PRINT_ERROR0("iap_send_pkt: send rfc data fail");
            return false;
        }

        data_len -= pkt_len;
        p_data   += pkt_len;
    }

    return true;
}

uint8_t iap_cal_checksum(uint8_t  *p_data,
                         uint16_t  len)
{
    uint8_t check_sum = 0;

    while (len)
    {
        check_sum += *p_data;
        p_data++;
        len--;
    }

    return (uint8_t)(0xff - check_sum + 1); /* ((~check_sum)+1); */
}

void iap2_sync_cmpl(T_IAP_LINK *p_link)
{
    T_IAP_CONN_CMPL_INFO info;

    p_link->state = IAP_STATE_IAP2_SYNC_CMPL;

    os_queue_init(&p_link->used_tx_q);
    os_queue_init(&p_link->sent_tx_q);

    if (p_link->dev_max_pkt_len > p_link->rfc_frame_size)
    {
        p_link->dev_max_pkt_len = p_link->rfc_frame_size;
    }

    p_link->remain_tx_q_num = p_iap->tx_q_elem_num;

    info.max_data_len = p_link->dev_max_pkt_len - IAP2_EA_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;

    if (p_iap->app_callback)
    {
        p_iap->app_callback(p_link->bd_addr, IAP_MSG_CONNECTED, (void *)&info);
    }
}

void iap_authen_cmpl(T_IAP_LINK *p_link,
                     bool        success)
{
    sys_timer_stop(p_link->authen_timer);

    if (p_iap->app_callback)
    {
        if (success)
        {
            p_iap->app_callback(p_link->bd_addr, IAP_MSG_AUTHEN_SUCCESS, NULL);
        }
        else
        {
            p_iap->app_callback(p_link->bd_addr, IAP_MSG_AUTHEN_FAIL, NULL);
        }
    }
}

void iap2_wrap_header(T_IAP_LINK *p_link,
                      uint8_t    *p_hdr,
                      uint16_t    payload_len,
                      uint8_t     session_id)
{
    uint16_t pkt_len;
    uint8_t *p = p_hdr;

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
    BE_UINT8_TO_STREAM(p, p_link->acc_pkt_seq);
    BE_UINT8_TO_STREAM(p, p_link->dev_pkt_seq);
    BE_UINT8_TO_STREAM(p, session_id);
    BE_UINT8_TO_STREAM(p, iap_cal_checksum(p_hdr, IAP2_PKT_HDR_SIZE - 1));
}

void iap2_wrap_msg(T_IAP_LINK *p_link,
                   uint8_t    *p_pkt,
                   uint16_t    msg_type,
                   uint16_t    msg_len)
{
    uint8_t *p = p_pkt + IAP2_PKT_HDR_SIZE;

    BE_UINT16_TO_STREAM(p, IAP2_MSG_START);
    BE_UINT16_TO_STREAM(p, msg_len + IAP2_MSG_HDR_SIZE);
    BE_UINT16_TO_STREAM(p, msg_type);

    p = p_pkt + IAP2_CTRL_MSG_HDR_SIZE + msg_len;
    *p = iap_cal_checksum(p_pkt + IAP2_PKT_HDR_SIZE, IAP2_MSG_HDR_SIZE + msg_len);
}

/* input p_param is the start address to put in message parameters, return address can be used to put in the next one */
uint8_t *iap2_prep_param(uint8_t  *p_param,
                         uint16_t  param_id,
                         void     *p_data,
                         uint16_t  data_len)
{
    BE_UINT16_TO_STREAM(p_param, data_len + IAP2_PARAM_HDR_SIZE);
    BE_UINT16_TO_STREAM(p_param, param_id);
    ARRAY_TO_STREAM(p_param, p_data, data_len);

    return p_param;
}

bool iap2_send_detect(T_IAP_LINK *p_link)
{
    uint8_t iap2_detect[IAP2_DETECT_PKT_SIZE];

    memcpy(iap2_detect, iap2_detect_code, IAP2_DETECT_PKT_SIZE);

    return iap_send_pkt(p_link, iap2_detect, IAP2_DETECT_PKT_SIZE);
}

bool iap2_send_ack(T_IAP_LINK *p_link)
{
    uint8_t buf[IAP2_PKT_HDR_SIZE];

    iap2_wrap_header(p_link, buf, 0, 0);

    return iap_send_pkt(p_link, buf, IAP2_PKT_HDR_SIZE);
}

bool iap2_send_sync(T_IAP_LINK *p_link)
{
    uint8_t buf[IAP2_SYNC_PKT_SIZE];

    memcpy(buf, iap2_sync_packet, IAP2_SYNC_PKT_SIZE);

    if (p_link->acc_pkt_seq != p_iap->acc_start_seq)
    {
        buf[4] = IAP2_CTRL_SYNC_ACK;
    }

    buf[IAP2_ACC_SEQ_OFFSET] = p_link->acc_pkt_seq;
    buf[IAP2_DEV_SEQ_OFFSET] = p_link->dev_pkt_seq;
    buf[IAP2_PKT_HDR_SIZE - 1] = iap_cal_checksum(buf, IAP2_PKT_HDR_SIZE - 1);

    BE_UINT16_TO_ARRAY(buf + IAP2_RETRANSMISSION_TIMEOUT_OFFSET, p_link->dev_retrans_tout);
    BE_UINT16_TO_ARRAY(buf + IAP2_CUMULATIVE_ACK_TIMEOUT_OFFSET, p_link->dev_cumulative_ack_tout);
    BE_UINT8_TO_ARRAY(buf + IAP2_NUM_OF_RETRANSMISSION_OFFSET, p_link->dev_max_retrans);
    BE_UINT8_TO_ARRAY(buf + IAP2_MAX_CUMULATIVE_ACK_OFFSET, p_link->dev_max_culumative_ack);
    buf[IAP2_SYNC_PKT_SIZE - 1] = iap_cal_checksum(buf + IAP2_PKT_HDR_SIZE,
                                                   IAP2_SYNC_PKT_SIZE - IAP2_PKT_HDR_SIZE - 1);

    return iap_send_pkt(p_link, buf, IAP2_SYNC_PKT_SIZE);
}

void iap2_update_acked_pkt(T_IAP_LINK *p_link)
{
    uint8_t pkt_seq;
    T_IAP2_TX_DATA_ELEM *p_elem;
    T_IAP_DATA_TRANSMITTED msg;
    uint8_t *p_data;
    bool notify;
    bool seq_check = false;

    PROFILE_PRINT_TRACE2("iap2_update_acked_pkt: before sent queue elem cnt %u, free queue elem cnt %u",
                         p_link->sent_tx_q.count, p_link->remain_tx_q_num);

    p_elem = (T_IAP2_TX_DATA_ELEM *)p_link->sent_tx_q.p_first;
    while (p_elem)
    {
        if (p_elem->data[IAP2_ACC_SEQ_OFFSET] == p_link->acked_seq)
        {
            seq_check = true;
            break;
        }

        p_elem = p_elem->p_next;
    }

    if (seq_check == false)
    {
        if (p_link->sent_tx_q.count != 0)
        {
            PROFILE_PRINT_ERROR1("iap2_update_acked_pkt: acked seq %d is not in sent q", p_link->acked_seq);
        }
        return;
    }

    while (os_queue_peek(&p_link->sent_tx_q, 0))
    {
        p_elem = os_queue_out(&p_link->sent_tx_q);
        p_data = p_elem->data;
        pkt_seq = *(p_data + IAP2_ACC_SEQ_OFFSET);

        if (p_elem->session_id == EA_SESSION_ID)
        {
            msg.eap_session_id = p_elem->ea_session_id;
            msg.data_len = p_elem->data_len;
            msg.success = true;
            notify = true;
        }
        else
        {
            notify = false;
        }

        PROFILE_PRINT_INFO2("iap2_update_acked_pkt: pkt seq %u, ack seq %u", pkt_seq, p_link->acked_seq);

        sys_timer_delete(p_elem->retrans_timer);

        if ((p_iap->app_callback) && (notify == true))
        {
            p_iap->app_callback(p_link->bd_addr, IAP_MSG_DATA_TRANSMITTED, (void *)&msg);
        }

        os_mem_free(p_elem);
        p_link->remain_tx_q_num++;

        if (pkt_seq == p_link->acked_seq)
        {
            /* all pkt before ack seq are freed */
            break;
        }
    }

    PROFILE_PRINT_TRACE2("iap2_update_acked_pkt: after sent queue elem cnt %d, free queue elem cnt %d",
                         p_link->sent_tx_q.count, p_link->remain_tx_q_num);
}

void iap2_tx_data(T_IAP_LINK *p_link)
{
    T_IAP2_TX_DATA_ELEM *p_elem;

    p_elem = os_queue_peek(&p_link->used_tx_q, 0);
    /* no need to check tx window because tx queue is not larger than window */
    while (p_elem)
    {
        p_link->acc_pkt_seq++;
        iap2_wrap_header(p_link, p_elem->data, p_elem->data_len, p_elem->session_id);

        if (iap_send_pkt(p_link, p_elem->data,
                         IAP2_PKT_HDR_SIZE + p_elem->data_len + IAP2_PAYLOAD_CHECKSUM_SIZE) == true)
        {
            os_queue_delete(&p_link->used_tx_q, p_elem);
            os_queue_in(&p_link->sent_tx_q, p_elem);

            p_elem->retrans_timer = sys_timer_create("iap2_data_trans",
                                                     SYS_TIMER_TYPE_LOW_PRECISION,
                                                     (IAP2_SEND_DATA << 16) | (p_link->acc_pkt_seq << 8) | p_link->index,
                                                     p_link->dev_retrans_tout * 1000,
                                                     false,
                                                     iap_handle_timeout);
            sys_timer_start(p_elem->retrans_timer);

            PROFILE_PRINT_TRACE5("iap2_tx_data: used queue elem cnt %u, credit %u, tx seq %u, acked seq %u, max out pkt %u",
                                 p_link->used_tx_q.count, p_link->remote_credit, p_link->acc_pkt_seq,
                                 p_link->acked_seq, p_link->dev_max_out_pkt);

            p_elem = os_queue_peek(&p_link->used_tx_q, 0);
        }
        else
        {
            p_link->acc_pkt_seq--;
            break;
        }
    }
}

void iap_handle_iap_pkt(T_IAP_LINK *p_link,
                        uint8_t    *p_data,
                        uint16_t    len)
{
    uint8_t *p = p_data;        /* payload start from lingoID */

    /* FF 55 02 00 EE 10 => device support iAP2, only handle this now */
    if ((len == 3) && (*p == 0x00) && (*(p + 1) == 0xEE) && (*(p + 2) == 0x10))
    {
        if (p_link->detect_timer != NULL)
        {
            sys_timer_delete(p_link->detect_timer);
            p_link->detect_timer = NULL;
        }

        p_link->acc_pkt_seq = p_iap->acc_start_seq;
        p_link->dev_pkt_seq = 0;

        /* init sync param */
        p_link->dev_retrans_tout = p_iap->acc_retrans_tout;
        p_link->dev_cumulative_ack_tout = p_iap->acc_cumulative_ack_tout;
        p_link->dev_max_retrans = p_iap->acc_max_retrans;
        p_link->dev_max_culumative_ack = p_iap->acc_max_culumative_ack;

        iap2_send_sync(p_link);
        p_link->state = IAP_STATE_IAP2_SYNC;
        sys_timer_start(p_link->sync_timer);
    }
}

void iap2_handle_ctrl_sync_ack(T_IAP_LINK *p_link,
                               uint8_t    *p_data)
{
    uint16_t retrans_tout;
    uint16_t cum_ack_tout;
    uint8_t  max_retrans;
    uint8_t  max_cum_ack;

    p_data += 1; /* link ver */
    BE_STREAM_TO_UINT8(p_link->dev_max_out_pkt, p_data);
    BE_STREAM_TO_UINT16(p_link->dev_max_pkt_len, p_data);
    BE_STREAM_TO_UINT16(retrans_tout, p_data);
    BE_STREAM_TO_UINT16(cum_ack_tout, p_data);
    BE_STREAM_TO_UINT8(max_retrans, p_data);
    BE_STREAM_TO_UINT8(max_cum_ack, p_data);

    if (p_link->dev_retrans_tout == retrans_tout && p_link->dev_cumulative_ack_tout == cum_ack_tout &&
        p_link->dev_max_retrans == max_retrans && p_link->dev_max_culumative_ack == max_cum_ack)
    {
        iap2_send_ack(p_link);

        if (p_link->state == IAP_STATE_IAP2_SYNC)
        {
            iap2_sync_cmpl(p_link);
        }
    }
    else
    {
        p_link->dev_retrans_tout = retrans_tout;
        p_link->dev_cumulative_ack_tout = cum_ack_tout;
        p_link->dev_max_retrans = max_retrans;
        p_link->dev_max_culumative_ack = max_cum_ack;

        p_link->acc_pkt_seq++;      /* increase acc seq to send sync+ack pkt */
        iap2_send_sync(p_link);
    }
}

void iap2_handle_req_authen_cert(T_IAP_LINK *p_link)
{
    uint8_t *p_buf = NULL;
    uint8_t *p;
    uint16_t crf_len = 0;
    uint16_t read_len = 0;
    uint16_t access_len = 0;
    uint8_t read_page = CP_REG_ACD1;

    if (p_link->state != IAP_STATE_IAP2_SYNC_CMPL)
    {
        return;
    }
    PROFILE_PRINT_INFO0("iap2_handle_req_authen_cert: ++");

    p_link->state = IAP_STATE_IAP2_AUTHEN;

    if (cp_read_crf_data_len(&crf_len) == false)
    {
        PROFILE_PRINT_ERROR0("iap2_handle_req_authen_cert: cp read crf data len fail");
        goto FAIL;
    }

    p_buf = os_mem_zalloc2(crf_len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE +
                           IAP2_PAYLOAD_CHECKSUM_SIZE);
    if (p_buf == NULL)
    {
        PROFILE_PRINT_ERROR0("iap2_handle_req_authen_cert: fail to alloc memory for authen data");
        goto FAIL;
    }

    PROFILE_PRINT_INFO1("iap2_handle_req_authen_cert: crf_len: %d", crf_len);

    p = p_buf + IAP2_PKT_HDR_SIZE + IAP2_MSG_HDR_SIZE;
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
            goto FAIL;
        }

        read_page++;
        read_len += access_len;
    }

    p_link->acc_pkt_seq++;
    iap2_wrap_msg(p_link, p_buf, MSG_AUTHEN_CERT, crf_len + IAP2_PARAM_HDR_SIZE);
    iap2_wrap_header(p_link, p_buf, crf_len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE, CTRL_SESSION_ID);
    iap_send_pkt(p_link, p_buf, crf_len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE +
                 IAP2_PAYLOAD_CHECKSUM_SIZE);
    os_mem_free(p_buf);
    PROFILE_PRINT_INFO0("iap2_handle_req_authen_cert: --");
    return;

FAIL:

    iap_authen_cmpl(p_link, false);
    if (p_buf)
    {
        os_mem_free(p_buf);
    }
    return;
}

void iap2_handle_req_authen_challenge_rsp_1(T_IAP_LINK *p_link,
                                            uint8_t    *p_param,
                                            uint16_t    param_len)
{
    uint8_t *p_data;
    uint16_t len;
    uint8_t error = 0;

    PROFILE_PRINT_INFO0("iap2_handle_req_authen_challenge_rsp_1: ++");

    p_data = p_param + IAP2_PARAM_HDR_SIZE;
    len = param_len - IAP2_PARAM_HDR_SIZE;

    if (p_link->state != IAP_STATE_IAP2_AUTHEN)
    {
        return;
    }

    if (cp_write_cha_len(len) == false)
    {
        error = 1;
        goto FAIL;
    }

    if (cp_write_cha_data(p_data, len) == false)
    {
        error = 2;
        goto FAIL;
    }

    /* start new signature process */
    if (cp_ctrl(CP_CMD_START_SIG_GEN) == false)
    {
        error = 3;
        goto FAIL;
    }

    sys_timer_start(p_link->signature_timer);
    PROFILE_PRINT_INFO0("iap2_handle_req_authen_challenge_rsp_1: --");
    return;
FAIL:

    PROFILE_PRINT_ERROR1("iap2_handle_req_authen_challenge_rsp: error %d", error);

    iap_authen_cmpl(p_link, false);
    return;
}

void iap2_handle_req_authen_challenge_rsp_2(T_IAP_LINK *p_link)
{
    uint8_t *p_data;
    uint16_t len;
    uint8_t *p_buf = NULL;
    T_CP_PRO_RES cp_res = CP_PRO_RES_NO_VALID;
    uint8_t read_authen_cnt = 0;
    uint8_t error = 0;

    PROFILE_PRINT_INFO0("iap2_handle_req_authen_challenge_rsp: ++");

    while (read_authen_cnt < 3)
    {
        if (cp_read_proc_result(&cp_res) == false)
        {
            PROFILE_PRINT_ERROR1("iap2_handle_req_authen_challenge_rsp: read_authen_cnt %d", read_authen_cnt);
        }
        else
        {
            if (cp_res == CP_PRO_RES_SIG_GEN_OK)
            {

                PROFILE_PRINT_INFO0("iap2_handle_req_authen_challenge_rsp: Accessory signature successfully generated");
                break;
            }
            else
            {
                PROFILE_PRINT_ERROR1("iap2_handle_req_authen_challenge_rsp fail: cp_res %d", cp_res);
            }
        }
        read_authen_cnt++;
        os_delay(500);
    }

    if (cp_read_signature_len(&len) == false)
    {
        error = 6;
        goto FAIL;
    }

    p_buf = os_mem_zalloc2(len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE +
                           IAP2_PAYLOAD_CHECKSUM_SIZE);
    if (p_buf == NULL)
    {
        goto FAIL;
    }

    p_data = p_buf + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE;
    BE_UINT16_TO_STREAM(p_data, len + IAP2_PARAM_HDR_SIZE);
    BE_UINT16_TO_STREAM(p_data, PARAM_ID_AUTHEN_RSP);
    if (cp_read_signature_data(p_data, len) == false)
    {
        goto FAIL;
    }

    p_link->acc_pkt_seq++;
    iap2_wrap_msg(p_link, p_buf, MSG_AUTHEN_RSP, len + IAP2_PARAM_HDR_SIZE);
    iap2_wrap_header(p_link, p_buf, len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE, CTRL_SESSION_ID);
    iap_send_pkt(p_link, p_buf, len + IAP2_PARAM_HDR_SIZE + IAP2_MSG_HDR_SIZE + IAP2_PKT_HDR_SIZE +
                 IAP2_PAYLOAD_CHECKSUM_SIZE);
    os_mem_free(p_buf);
    PROFILE_PRINT_INFO0("iap2_handle_req_authen_challenge_rsp: --");
    return;

FAIL:

    PROFILE_PRINT_ERROR1("iap2_handle_req_authen_challenge_rsp: error %d", error);

    iap_authen_cmpl(p_link, false);
    if (p_buf)
    {
        os_mem_free(p_buf);
    }
    return;
}

void iap2_handle_start_eap_session(T_IAP_LINK *p_link,
                                   uint8_t    *p_data,
                                   uint16_t    len)
{
    uint8_t *p;
    uint8_t *p_param = p_data;
    uint16_t param_len;
    uint16_t param_id;
    T_IAP_START_EAP_SESSION msg;

    iap2_send_ack(p_link);

    while (p_param < p_data + len)
    {
        p = p_param;
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

        p_param += param_len;
    }

    if (p_iap->app_callback)
    {
        p_iap->app_callback(p_link->bd_addr, IAP_MSG_START_EAP_SESSION, (void *)&msg);
    }
}

void iap2_handle_stop_eap_session(T_IAP_LINK *p_link,
                                  uint8_t    *p_data,
                                  uint16_t    len)
{
    T_IAP_STOP_EAP_SESSION msg;

    iap2_send_ack(p_link);

    p_data += IAP2_PARAM_HDR_SIZE;
    BE_STREAM_TO_UINT16(msg.eap_session_id, p_data);

    if (p_iap->app_callback)
    {
        p_iap->app_callback(p_link->bd_addr, IAP_MSG_STOP_EAP_SESSION, (void *)&msg);
    }
}

void iap2_handle_eap_session_status(T_IAP_LINK *p_link,
                                    uint8_t    *p_data,
                                    uint16_t    len)
{
    uint8_t *p;
    uint8_t *p_param = p_data;
    uint16_t param_len;
    uint16_t param_id;
    T_IAP_EAP_SESSION_STATUS_INFO msg;

    iap2_send_ack(p_link);

    while (p_param < p_data + len)
    {
        p = p_param;
        BE_STREAM_TO_UINT16(param_len, p);
        BE_STREAM_TO_UINT16(param_id, p);

        if (param_id == EAP_SESSION_STATUS_PARAM_ID_SESSION_ID)
        {
            BE_STREAM_TO_UINT16(msg.eap_session_id, p);
        }
        else if (param_id == EAP_SESSION_STATUS_PARAM_ID_SESSION_STATUS)
        {
            msg.eap_session_status = *(T_IAP_EAP_SESSION_STATUS *)p;
            p++;
        }

        p_param += param_len;
    }

    if (p_iap->app_callback)
    {
        p_iap->app_callback(p_link->bd_addr, IAP_MSG_EAP_SESSION_STATUS, (void *)&msg);
    }
}

void iap2_handle_dev_hid_report(T_IAP_LINK *p_link,
                                uint8_t    *p_data,
                                uint16_t    len)
{
    uint8_t *p;
    uint8_t *p_param = p_data;
    uint16_t param_len;
    uint16_t param_id;

    while (p_param < p_data + len)
    {
        p = p_param;
        BE_STREAM_TO_UINT16(param_id, p);
        BE_STREAM_TO_UINT16(param_len, p);

        switch (param_id)
        {
        case PARAM_ID_DEV_HID_COMPONENT_ID:
            {
                uint16_t hid_component_id;

                BE_STREAM_TO_UINT16(hid_component_id, p);
                PROFILE_PRINT_INFO1("iap2_handle_dev_hid_report: hid_component_id 0x%04x", hid_component_id);
            }
            break;

        case PARAM_ID_DEV_HID_REPORT:
            {
                PROFILE_PRINT_INFO3("iap2_handle_dev_hid_report: hid report len: %u, data: 0x%02x 0x%02x",
                                    param_len - IAP2_PARAM_HDR_SIZE, *p, *(p + 1));
            }
            break;

        default:
            break;
        }

        p_param += param_len;
    }
}

void iap2_handle_ctrl_session(T_IAP_LINK *p_link,
                              uint8_t    *p_data)
{
    uint8_t *p = p_data;
    uint16_t msg_id;
    uint16_t param_len;

    p += 2;     /* start of message */
    BE_STREAM_TO_UINT16(param_len, p);
    param_len -= IAP2_MSG_HDR_SIZE;
    BE_STREAM_TO_UINT16(msg_id, p);

    PROFILE_PRINT_TRACE2("iap2_handle_ctrl_session: msg_id 0x%04x, param len %u", msg_id, param_len);

    switch (msg_id)
    {
    case MSG_REQ_AUTHEN_CERT:
        iap2_send_ack(p_link);
        iap2_handle_req_authen_cert(p_link);
        break;

    case MSG_REQ_AUTHEN_CHALLENGE_RSP:
        iap2_send_ack(p_link);
        iap2_handle_req_authen_challenge_rsp_1(p_link, p, param_len);
        break;

    case MSG_AUTHEN_FAILED:
        iap_authen_cmpl(p_link, false);
        iap2_send_ack(p_link);
        break;

    case MSG_AUTHEN_SUCCEEDED:
        iap2_send_ack(p_link);
        break;

    case MSG_START_IDENT:
        if (p_iap->app_callback)
        {
            p_iap->app_callback(p_link->bd_addr, IAP_MSG_START_IDENT_REQ, NULL);
        }
        break;

    case MSG_IDENT_ACCEPTED:
        iap_authen_cmpl(p_link, true);
        iap2_send_ack(p_link);
        break;

    case MSG_IDENT_REJECTED:
        iap_authen_cmpl(p_link, false);
        iap2_send_ack(p_link);
        break;

    case MSG_START_EA_PROTO_SESSION:
        iap2_handle_start_eap_session(p_link, p, param_len);
        break;

    case MSG_STOP_EA_PROTO_SESSION:
        iap2_handle_stop_eap_session(p_link, p, param_len);
        break;

    case MSG_DEV_HID_REPORT:
        iap2_handle_dev_hid_report(p_link, p, param_len);
        iap2_send_ack(p_link);
        break;

    default:
        {
            T_IAP_CTRL_MSG_IND ctrl_msg;

            ctrl_msg.msg_id = msg_id;
            ctrl_msg.param_len = param_len;
            ctrl_msg.p_param = p;
            if (p_iap->app_callback)
            {
                p_iap->app_callback(p_link->bd_addr, IAP_MSG_CTRL_MSG_IND, (void *)&ctrl_msg);
            }

            iap2_send_ack(p_link);
        }
        break;
    }
}

void iap2_handle_reset(T_IAP_LINK *p_link)
{
    T_IAP2_TX_DATA_ELEM *p_elem;

    if (p_link->state == IAP_STATE_IAP2_SYNC)
    {
        return;
    }

    /* clear variable */
    if (p_link->authen_timer)
    {
        sys_timer_stop(p_link->authen_timer);
    }

    if (p_link->sync_timer)
    {
        sys_timer_stop(p_link->sync_timer);
    }

    if (p_link->detect_timer)
    {
        sys_timer_stop(p_link->detect_timer);
    }

    if (p_link->signature_timer)
    {
        sys_timer_stop(p_link->signature_timer);
    }

    if (p_link->p_payload)
    {
        os_mem_free(p_link->p_payload);
        p_link->p_payload = NULL;
        p_link->header_offset = 0;
    }

    while (p_link->used_tx_q.count)
    {
        p_elem = os_queue_out(&p_link->used_tx_q);
        os_mem_free(p_elem);
    }

    while (p_link->sent_tx_q.count)
    {
        p_elem = os_queue_out(&p_link->sent_tx_q);
        sys_timer_delete(p_elem->retrans_timer);
        os_mem_free(p_elem);
    }

    /* start sync procedure */
    p_link->acc_pkt_seq = p_iap->acc_start_seq;

    p_link->dev_retrans_tout = p_iap->acc_retrans_tout;
    p_link->dev_cumulative_ack_tout = p_iap->acc_cumulative_ack_tout;
    p_link->dev_max_retrans = p_iap->acc_max_retrans;
    p_link->dev_max_culumative_ack = p_iap->acc_max_culumative_ack;

    iap2_send_sync(p_link);
    p_link->state = IAP_STATE_IAP2_SYNC;
    sys_timer_start(p_link->sync_timer);

    /* notify upper layer */
    p_iap->app_callback(p_link->bd_addr, IAP_MSG_RESET, NULL);
}

void iap_handle_iap2_pkt(T_IAP_LINK *p_link,
                         uint8_t    *p_data,
                         uint16_t    len)
{
    uint8_t *p;
    uint8_t control;
    uint8_t session_id;
    uint8_t dev_pkt_seq;

    p = &p_link->header[4];
    BE_STREAM_TO_UINT8(control, p);
    BE_STREAM_TO_UINT8(dev_pkt_seq, p);
    BE_STREAM_TO_UINT8(p_link->acked_seq, p);
    BE_STREAM_TO_UINT8(session_id, p);

    PROFILE_PRINT_TRACE5("iap_handle_iap2_pkt: control %d, dev_pkt_seq %d, p_link->dev_pkt_seq %d, p_link->acked_seq %d, session_id %d",
                         control, dev_pkt_seq, p_link->dev_pkt_seq, p_link->acked_seq, session_id);

    iap2_update_acked_pkt(p_link);

    switch (control)
    {
    case IAP2_CTRL_SYNC_ACK:
        sys_timer_stop(p_link->sync_timer);
        p_link->dev_pkt_seq = dev_pkt_seq;
        iap2_handle_ctrl_sync_ack(p_link, p_data);
        break;

    case IAP2_CTRL_ACK:
        if (len)
        {
            if (session_id == CTRL_SESSION_ID)
            {
                p_link->dev_pkt_seq = dev_pkt_seq;
                iap2_handle_ctrl_session(p_link, p_data);
            }
            else if (session_id == EA_SESSION_ID)
            {
                T_IAP_DATA_IND msg;

                BE_STREAM_TO_UINT16(msg.eap_session_id, p_data);
                msg.p_data = p_data;
                msg.dev_seq_num = dev_pkt_seq;
                msg.len = len - IAP2_EA_ID_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;

                if (p_iap->app_callback)
                {
                    p_iap->app_callback(p_link->bd_addr, IAP_MSG_DATA_IND, (void *)&msg);
                }
            }
        }
        break;

    case IAP2_CTRL_RESET:
        iap2_handle_reset(p_link);
        break;

    default:
        break;
    }
}

void iap_handle_rcv_data(T_IAP_LINK *p_link,
                         uint8_t    *p_data,
                         uint16_t    len)
{
    uint8_t *p = p_data;

    if ((p_data[0] == IAP2_START_BYTE1) && (p_data[1] == IAP2_START_BYTE2))
    {
        if (p_link->header_offset < p_link->header_len)
        {
            PROFILE_PRINT_WARN0("iap_handle_rcv_data: rcv new header, discard pre header");
            p_link->header_offset = 0;
        }
        else if (p_link->p_payload != NULL)
        {
            PROFILE_PRINT_WARN0("iap_handle_rcv_data: rcv new header, discard pre payload");
            os_mem_free(p_link->p_payload);
            p_link->p_payload = NULL;
            p_link->header_offset = 0;
        }
    }

    while (len)
    {
        if (p_link->header_offset == 0 || p_link->header_offset < p_link->header_len)
        {
            if (p_link->header_offset == 0)
            {
                p_link->header_len = 2;     /* at least 1 start and 1 length for iAP */
            }

            p_link->header[p_link->header_offset] = *p++;
            p_link->header_offset++;
            len--;

            if (p_link->header_offset == 2)
            {
                if (p_link->header[0] == IAP2_START_BYTE1 && p_link->header[1] == IAP2_START_BYTE2)
                {
                    p_link->header_len = 9;
                }
                else if (p_link->header[0] == IAP_START_BYTE)
                {
                    if (p_link->header[1] == IAP_LENGTH_MARKER)
                    {
                        p_link->header_len += 2;    /* length is 3 bytes */
                    }
                }
                else if (p_link->header[0] == IAP2_START_BYTE1 && p_link->header[1] == IAP_START_BYTE)
                {
                    p_link->header_len++;   /* 0xFF55 is also iAP header, check next byte for length */
                }
                else
                {
                    p_link->header_offset = 0;  /* wrong header */
                }
            }
            else if (p_link->header_offset == 3)
            {
                if (p_link->header[0] == IAP2_START_BYTE1 && p_link->header[1] == IAP_START_BYTE)
                {
                    if (p_link->header[2] == IAP_LENGTH_MARKER)
                    {
                        p_link->header_len += 2;    /* length is 3 bytes */
                    }
                }
            }

            if (p_link->header_offset == p_link->header_len)
            {
                if (p_link->header[0] == IAP2_START_BYTE1 && p_link->header[1] == IAP2_START_BYTE2)
                {
                    BE_ARRAY_TO_UINT16(p_link->payload_len, &p_link->header[2]);
                    p_link->payload_len -= 9;   /* remove iAP2 header */
                    if (p_link->payload_len == 0)
                    {
                        iap_handle_iap2_pkt(p_link, p, p_link->payload_len);
                        p_link->header_offset = 0;
                    }
                }
                else if (p_link->header[0] == IAP_START_BYTE)
                {
                    if (p_link->header[1] == IAP_LENGTH_MARKER)
                    {
                        BE_ARRAY_TO_UINT16(p_link->payload_len, &p_link->header[2]);
                    }
                    else
                    {
                        p_link->payload_len = p_link->header[1];
                    }
                    p_link->payload_len++;  /* add checksum */
                }
                else if (p_link->header[0] == IAP2_START_BYTE1 && p_link->header[1] == IAP_START_BYTE)
                {
                    if (p_link->header[2] == IAP_LENGTH_MARKER)
                    {
                        BE_ARRAY_TO_UINT16(p_link->payload_len, &p_link->header[3]);
                    }
                    else
                    {
                        p_link->payload_len = p_link->header[2];
                    }
                    p_link->payload_len++;  /* add checksum */
                }
            }
        }
        else    /* header is parsed, check len */
        {
            if (p_link->p_payload != NULL)
            {
                uint16_t copy_len = p_link->payload_len - p_link->payload_offset;

                if (copy_len > len)
                {
                    copy_len = len;
                }

                memcpy(p_link->p_payload + p_link->payload_offset, p, copy_len);
                p_link->payload_offset += copy_len;
                p += copy_len;
                len -= copy_len;

                if (p_link->payload_offset == p_link->payload_len)
                {
                    if (p_link->header[0] == IAP2_START_BYTE1 && p_link->header[1] == IAP2_START_BYTE2)
                    {
                        iap_handle_iap2_pkt(p_link, p_link->p_payload, p_link->payload_len);
                    }
                    else
                    {
                        iap_handle_iap_pkt(p_link, p_link->p_payload, p_link->payload_len);
                    }

                    os_mem_free(p_link->p_payload);
                    p_link->p_payload = NULL;
                    p_link->header_offset = 0;
                }
            }
            else
            {
                if (p_link->payload_len <= len)
                {
                    if (p_link->header[0] == IAP2_START_BYTE1 && p_link->header[1] == IAP2_START_BYTE2)
                    {
                        iap_handle_iap2_pkt(p_link, p, p_link->payload_len);
                    }
                    else
                    {
                        iap_handle_iap_pkt(p_link, p, p_link->payload_len);
                    }

                    p += p_link->payload_len;
                    len -= p_link->payload_len;
                    p_link->header_offset = 0;
                }
                else
                {
                    p_link->p_payload = os_mem_zalloc2(p_link->payload_len);

                    if (p_link->p_payload == NULL)
                    {
                        p_link->header_offset = 0;
                        return;
                    }

                    memcpy(p_link->p_payload, p, len);
                    p_link->payload_offset = len;
                    len = 0;
                }
            }
        }
    }
}

void iap_handle_timeout(T_SYS_TIMER_HANDLE handle)
{
    uint8_t iap_timer_id;
    uint32_t timer_id;
    uint16_t timer_chann;
    T_IAP_LINK *p_link;

    timer_id = sys_timer_id_get(handle);
    timer_chann = (uint16_t)timer_id;
    iap_timer_id = (uint8_t)(timer_id >> 16);

    PROFILE_PRINT_TRACE3("iap_handle_timeout: timer_id %x, iap_timer_id 0x%02x, timer_chann 0x%04x",
                         timer_id, iap_timer_id, timer_chann);

    p_link = &p_iap->p_link[(uint8_t)timer_chann];

    switch (iap_timer_id)
    {
    case IAP_AUTHEN_TOUT:
        PROFILE_PRINT_ERROR0("iap_handle_timeout: authentication timeout");
        iap_authen_cmpl(p_link, false);
        rfc_disconn_req(p_link->bd_addr, p_link->dlci);
        break;

    case IAP2_DETECT:
        iap2_send_detect(p_link);
        sys_timer_start(p_link->detect_timer);
        break;

    case IAP2_SYNC:
        iap2_send_sync(p_link);
        sys_timer_start(p_link->sync_timer);
        break;

    case IAP2_SEND_DATA:
        {
            T_IAP2_TX_DATA_ELEM *p_elem;
            T_IAP_DATA_TRANSMITTED msg;

            p_elem = (T_IAP2_TX_DATA_ELEM *)p_link->sent_tx_q.p_first;

            while (p_elem)
            {
                if (p_elem->data[5] == (uint8_t)(timer_chann >> 8))
                {
                    sys_timer_delete(p_elem->retrans_timer);
                    p_elem->retrans_cnt++;
                    if (p_elem->retrans_cnt >= p_link->dev_max_retrans)
                    {
                        PROFILE_PRINT_ERROR2("iap_handle_timeout: retrans cnt %u reaches max num %u",
                                             p_elem->retrans_cnt, p_link->dev_max_retrans);
                        p_elem = os_queue_out(&p_link->sent_tx_q);
                        if (p_elem->session_id == EA_SESSION_ID)
                        {
                            msg.eap_session_id = p_elem->ea_session_id;
                            msg.data_len = p_elem->data_len;
                            msg.success = false;

                            if (p_iap->app_callback)
                            {
                                p_iap->app_callback(p_link->bd_addr, IAP_MSG_DATA_TRANSMITTED, (void *)&msg);
                            }
                        }
                        os_mem_free(p_elem);
                        p_link->remain_tx_q_num++;

                        rfc_disconn_req(p_link->bd_addr, p_link->dlci);
                    }
                    else
                    {
                        p_elem->data[6] = p_link->dev_pkt_seq;
                        p_elem->data[8] = iap_cal_checksum(p_elem->data, IAP2_PKT_HDR_SIZE - 1);
                        iap_send_pkt(p_link, p_elem->data,
                                     p_elem->data_len + IAP2_PKT_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE);

                        p_elem->retrans_timer = sys_timer_create("iap2_data_trans",
                                                                 SYS_TIMER_TYPE_LOW_PRECISION,
                                                                 (IAP2_SEND_DATA << 16) | timer_chann,
                                                                 p_link->dev_retrans_tout * 1000,
                                                                 false,
                                                                 iap_handle_timeout);

                        sys_timer_start(p_elem->retrans_timer);

                        PROFILE_PRINT_TRACE1("iap_handle_timeout: retrans for seq %u", p_elem->data[5]);
                    }

                    break;
                }

                p_elem = p_elem->p_next;
            }
        }
        break;

    case IAP2_SIGNATURE:
        {
            sys_timer_stop(p_link->signature_timer);
            iap2_handle_req_authen_challenge_rsp_2(p_link);
        }
        break;

    default:
        break;
    }
}


void iap_handle_rfc_conn_ind(T_RFC_CONN_IND *p_ind)
{
    T_IAP_LINK *p_link;
    T_IAP_CONN_IND ind;

    p_link = iap_find_link_by_addr(p_ind->bd_addr);
    if (p_link)
    {
        rfc_conn_cfm(p_ind->bd_addr, p_link->dlci, RFC_REJECT, 0, 0);
        return;
    }

    p_link = iap_alloc_link(p_ind->bd_addr);
    if (p_link)
    {
        p_link->dlci = p_ind->dlci;

        ind.rfc_frame_size = p_ind->frame_size;

        if (p_iap->app_callback)
        {
            p_iap->app_callback(p_link->bd_addr, IAP_MSG_CONN_IND, (void *)&ind);
        }
    }
    else
    {
        rfc_conn_cfm(p_ind->bd_addr, p_ind->dlci, RFC_REJECT, 0, 0);
    }
}

void iap_handle_rfc_conn_cmpl(T_RFC_CONN_CMPL *p_cmpl)
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(p_cmpl->bd_addr);
    if (p_link == NULL)
    {
        rfc_disconn_req(p_cmpl->bd_addr, p_cmpl->dlci);
        return;
    }

    p_link->rfc_frame_size = p_cmpl->frame_size;
    p_link->remote_credit = p_cmpl->remain_credits;
    p_link->state = IAP_STATE_CONNECTED;

    if (p_link->remote_credit != 0)
    {
        if (iap2_send_detect(p_link))
        {
            sys_timer_start(p_link->detect_timer);
            sys_timer_start(p_link->authen_timer);
            p_link->state = IAP_STATE_DETECT;
        }
    }
}

void iap_handle_rfc_disconn_cmpl(T_RFC_DISCONN_CMPL *p_disc)
{
    T_IAP_LINK *p_link;
    T_IAP_DISCONN_INFO info;
    T_IAP_STATE state;

    p_link = iap_find_link_by_addr(p_disc->bd_addr);
    if (p_link != NULL)
    {
        state = p_link->state;
        iap_free_link(p_link);

        if (state < IAP_STATE_IAP2_SYNC_CMPL)
        {
            p_iap->app_callback(p_disc->bd_addr, IAP_MSG_CONN_FAIL, (void *)&p_disc->cause);
        }
        else
        {
            info.cause = p_disc->cause;
            p_iap->app_callback(p_disc->bd_addr, IAP_MSG_DISCONNTED, (void *)&info);
        }
    }
}

void iap_handle_rfc_credit_info(T_RFC_CREDIT_INFO *p_credit)
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(p_credit->bd_addr);
    if (p_link)
    {
        p_link->remote_credit = p_credit->remain_credits;

        if (p_link->state == IAP_STATE_CONNECTED)
        {
            if (iap2_send_detect(p_link))
            {
                sys_timer_start(p_link->detect_timer);
                sys_timer_start(p_link->authen_timer);
                p_link->state = IAP_STATE_DETECT;
            }
        }

        iap2_tx_data(p_link);
    }
}

void iap_handle_rfc_data_ind(T_RFC_DATA_IND *p_ind)
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(p_ind->bd_addr);
    if (p_link)
    {
        p_link->remote_credit = p_ind->remain_credits;

        if (p_ind->length)
        {
            iap_handle_rcv_data(p_link, p_ind->buf, p_ind->length);
        }

        iap2_tx_data(p_link);
    }

    rfc_data_cfm(p_ind->bd_addr, p_ind->dlci, 1);
}

void iap_handle_rfc_dlci_change(T_RFC_DLCI_CHANGE_INFO *p_info)
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(p_info->bd_addr);
    if (p_link)
    {
        p_link->dlci = p_info->curr_dlci;
    }
}

void iap_rfc_cb(T_RFC_MSG_TYPE  msg_type,
                void           *p_msg)
{
    PROFILE_PRINT_TRACE1("iap_rfc_cb: msg_type %d", msg_type);

    switch (msg_type)
    {
    case RFC_CONN_IND:
        iap_handle_rfc_conn_ind((T_RFC_CONN_IND *)p_msg);
        break;

    case RFC_CONN_CMPL:
        iap_handle_rfc_conn_cmpl((T_RFC_CONN_CMPL *)p_msg);
        break;


    case RFC_DISCONN_CMPL:
        iap_handle_rfc_disconn_cmpl((T_RFC_DISCONN_CMPL *)p_msg);
        break;

    case RFC_CREDIT_INFO:
        iap_handle_rfc_credit_info((T_RFC_CREDIT_INFO *)p_msg);
        break;

    case RFC_DATA_IND:
        iap_handle_rfc_data_ind((T_RFC_DATA_IND *)p_msg);
        break;

    case RFC_DLCI_CHANGE:
        iap_handle_rfc_dlci_change((T_RFC_DLCI_CHANGE_INFO *)p_msg);
        break;

    default:
        break;
    }
}

bool iap_init(uint8_t  link_num,
              uint8_t  iap_chann_num,
              P_IAP_CB callback)
{
    int32_t ret = 0;

    p_iap = os_mem_zalloc2(sizeof(T_IAP));
    if (p_iap == NULL)
    {
        ret = 1;
        goto fail_alloc_iap;
    }

    p_iap->link_num = link_num;

    p_iap->p_link = os_mem_zalloc2(p_iap->link_num * sizeof(T_IAP_LINK));
    if (p_iap->p_link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (rfc_reg_cb(iap_chann_num, iap_rfc_cb, &p_iap->iap_rfc_index) == false)
    {
        ret = 3;
        goto fail_reg_rfc_cb;
    }

    p_iap->app_callback = callback;

    p_iap->acc_max_pkt_len = 0x0320;
    p_iap->acc_retrans_tout = 0x01f4;
    p_iap->acc_cumulative_ack_tout = 0x0064;
    p_iap->acc_max_out_pkt = 0x04;
    p_iap->acc_max_retrans = 0x04;
    p_iap->acc_max_culumative_ack = 0x04;
    p_iap->acc_start_seq = 0xC0;

    p_iap->tx_q_elem_num = 3;

    return true;

fail_reg_rfc_cb:
    os_mem_free(p_iap->p_link);
fail_alloc_link:
    os_mem_free(p_iap);
    p_iap = NULL;
fail_alloc_iap:
    PROFILE_PRINT_ERROR1("iap_init: failed %d", -ret);
    return false;
}

bool iap_set_param(T_IAP_PARAM_TYPE  type,
                   uint8_t           len,
                   void             *p_value)
{
    switch (type)
    {
    case IAP_PARAM_ACC_MAX_PKT_LEN:
        if (len == sizeof(uint16_t) && (*((uint16_t *)p_value) >= 24))
        {
            p_iap->acc_max_pkt_len = *((uint16_t *)p_value);
            return true;
        }
        break;

    case IAP_PARAM_ACC_RETRANS_TOUT:
        if (len == sizeof(uint16_t) && (*((uint16_t *)p_value) >= 20))
        {
            p_iap->acc_retrans_tout = *((uint16_t *)p_value);
            return true;
        }
        break;

    case IAP_PARAM_ACC_CUMULATIVE_ACK_TOUT:
        if (len == sizeof(uint16_t) && (*((uint16_t *)p_value) >= 10) &&
            (*((uint16_t *)p_value) <= (p_iap->acc_retrans_tout / 2)))
        {
            p_iap->acc_cumulative_ack_tout = *((uint16_t *)p_value);
            return true;
        }
        break;

    case IAP_PARAM_ACC_MAX_OUT_PKT_NUM:
        if (len == sizeof(uint8_t) && (*((uint8_t *)p_value) > 0) && (*((uint8_t *)p_value) <= 127))
        {
            p_iap->acc_max_out_pkt = *((uint8_t *)p_value);
            return true;
        }
        break;

    case IAP_PARAM_ACC_MAX_RETRANS_NUM:
        if (len == sizeof(uint8_t) && (*((uint8_t *)p_value) > 0) && (*((uint8_t *)p_value) <= 30))
        {
            p_iap->acc_max_retrans = *((uint8_t *)p_value);
            return true;
        }
        break;

    case IAP_PARAM_ACC_MAX_CULUMATIVE_ACK:
        if (len == sizeof(uint8_t) && (*((uint8_t *)p_value) <= p_iap->acc_max_out_pkt))
        {
            p_iap->acc_max_culumative_ack = *((uint8_t *)p_value);
            return true;
        }
        break;

    case IAP_PARAM_ACC_START_SEQ_NUM:
        if (len == sizeof(uint8_t))
        {
            p_iap->acc_start_seq = *((uint8_t *)p_value);
            return true;
        }
        break;

    case IAP_PARAM_TX_Q_ELEM_NUM:
        if (len == sizeof(uint8_t))
        {
            p_iap->tx_q_elem_num = *((uint8_t *)p_value);
            return true;
        }
        break;

    default:
        break;
    }

    return false;
}

bool iap_connect_req(uint8_t  bd_addr[6],
                     uint8_t  server_chann,
                     uint16_t frame_size,
                     uint8_t  init_credits)
{
    T_IAP_LINK *p_link;
    uint8_t dlci;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link)
    {
        return false;
    }

    p_link = iap_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (rfc_conn_req(bd_addr, server_chann, frame_size, init_credits,
                     p_iap->iap_rfc_index, &dlci) == true)
    {
        p_link->dlci = dlci;
        p_link->state = IAP_STATE_CONNECTING;
        return true;
    }
    else
    {
        iap_free_link(p_link);
        return false;
    }
}

bool iap_connect_cfm(uint8_t  bd_addr[6],
                     bool     accept,
                     uint16_t frame_size,
                     uint8_t  init_credits)
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (accept)
    {
        p_link->state = IAP_STATE_CONNECTING;
        rfc_conn_cfm(p_link->bd_addr, p_link->dlci, RFC_ACCEPT, frame_size, init_credits);
    }
    else
    {
        rfc_conn_cfm(p_link->bd_addr, p_link->dlci, RFC_REJECT, 0, 0);
        iap_free_link(p_link);
    }

    return true;
}

bool iap_disconnect_req(uint8_t bd_addr[6])
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link)
    {
        return rfc_disconn_req(p_link->bd_addr, p_link->dlci);
    }

    return false;
}

bool iap_send_ident_info(uint8_t   bd_addr[6],
                         uint8_t  *p_ident,
                         uint16_t  ident_len)
{
    T_IAP_LINK *p_link;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint8_t *p_data;
    uint16_t total_len;

    PROFILE_PRINT_INFO1("iap_send_ident_info: ident_len %d", ident_len);

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR0("iap_send_ident_info: invalid addr");
        return false;
    }

    total_len = ident_len + IAP2_CTRL_MSG_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_send_ident_info: data_len too larger %d",
                             ident_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = ident_len + IAP2_MSG_HDR_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p_data = p_elem->data;
    memcpy(p_data + IAP2_CTRL_MSG_HDR_SIZE, p_ident, ident_len);
    iap2_wrap_msg(p_link, p_data, MSG_IDENT_INFO, ident_len);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;

}

bool iap_eap_session_status_send(uint8_t                  bd_addr[6],
                                 uint16_t                 session_id,
                                 T_IAP_EAP_SESSION_STATUS status)
{
    T_IAP_LINK *p_link;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint16_t total_len;
    uint8_t *p_data;
    uint8_t  data[2];

    PROFILE_PRINT_INFO2("iap_eap_session_status_send: session_id 0x%02x, status %d",
                        session_id, status);

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR0("iap_eap_session_status_send: invalid addr");
        return false;
    }

    total_len = IAP2_CTRL_MSG_HDR_SIZE + (IAP2_PARAM_HDR_SIZE + sizeof(uint16_t)) +
                (IAP2_PARAM_HDR_SIZE + sizeof(uint16_t)) + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_eap_session_status_send: len too larger %d",
                             total_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p_data = p_elem->data;
    p_data += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_ARRAY(data, session_id);
    p_data = iap2_prep_param(p_data, EAP_SESSION_STATUS_PARAM_ID_SESSION_ID, data, sizeof(uint16_t));

    iap2_prep_param(p_data, EAP_SESSION_STATUS_PARAM_ID_SESSION_STATUS, &status,
                    sizeof(T_IAP_EAP_SESSION_STATUS));

    iap2_wrap_msg(p_link, p_elem->data, MSG_EA_PROTO_SESSION_STATUS,
                  p_elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_launch_app(uint8_t                  bd_addr[6],
                    char                    *boundle_id,
                    uint8_t                  len_boundle_id,
                    T_IAP_APP_LAUNCH_METHOD  method)
{
    T_IAP_LINK *p_link;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint16_t total_len;
    uint8_t *p_data;

    PROFILE_PRINT_INFO2("iap_launch_app: boundle_id %s, method %d", TRACE_STRING(boundle_id), method);

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR0("iap_launch_app: invalid addr");
        return false;
    }

    total_len = len_boundle_id + IAP2_CTRL_MSG_PARAM_HDR_SIZE + IAP2_PARAM_HDR_SIZE +
                sizeof(T_IAP_APP_LAUNCH_METHOD) + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_launch_app: len_boundle_id too larger %d", len_boundle_id);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p_data = p_elem->data;
    p_data = iap2_prep_param(p_data + IAP2_CTRL_MSG_HDR_SIZE, PARAM_ID_APP_BUNDLE_ID, boundle_id,
                             len_boundle_id);
    iap2_prep_param(p_data, PARAM_ID_APP_LAUNCH_METHOD, &method, sizeof(T_IAP_APP_LAUNCH_METHOD));
    iap2_wrap_msg(p_link, p_elem->data, MSG_REQ_APP_LAUNCH, p_elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_hid_start(uint8_t   bd_addr[6],
                   uint16_t  hid_component_id,
                   uint16_t  vid,
                   uint16_t  pid,
                   uint8_t  *hid_report_desc,
                   uint16_t  hid_report_desc_len)
{
    T_IAP_LINK *p_link;
    uint16_t total_len;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint8_t *p;
    uint8_t  data[2];

    PROFILE_PRINT_TRACE3("iap_hid_start: hid component identifier 0x%x, vid 0x%x, pid 0x%x",
                         hid_component_id, vid, pid);

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint16_t) +
                (IAP2_PARAM_HDR_SIZE + sizeof(uint16_t)) +
                (IAP2_PARAM_HDR_SIZE + sizeof(uint16_t)) +
                (IAP2_PARAM_HDR_SIZE + hid_report_desc_len) + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_hid_start: data_len too larger %d", total_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p = p_elem->data;
    p += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_ARRAY(data, hid_component_id);
    p = iap2_prep_param(p, PARAM_ID_HID_COMPONENT_ID, data, sizeof(uint16_t));

    BE_UINT16_TO_ARRAY(data, vid);
    p = iap2_prep_param(p, PARAM_ID_VENDOR_ID, data, sizeof(uint16_t));

    BE_UINT16_TO_ARRAY(data, pid);
    p = iap2_prep_param(p, PARAM_ID_PRODUCT_ID, data, sizeof(uint16_t));

    p = iap2_prep_param(p, PARAM_ID_HID_REPORT_DESCRIPTOR, hid_report_desc, hid_report_desc_len);

    iap2_wrap_msg(p_link, p_elem->data, MSG_START_HID, p_elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_hid_send_report(uint8_t   bd_addr[6],
                         uint16_t  hid_component_id,
                         uint8_t  *hid_report,
                         uint16_t  hid_report_len)
{
    T_IAP_LINK *p_link;
    uint16_t total_len;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint8_t *p;
    uint8_t  data[2];

    PROFILE_PRINT_INFO2("iap_hid_send_report: hid component identifier 0x%x, hid report len %u",
                        hid_component_id, hid_report_len);

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint16_t) +
                (IAP2_PARAM_HDR_SIZE + hid_report_len) + IAP2_PAYLOAD_CHECKSUM_SIZE;

    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_hid_send_report: data_len too larger %d", total_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p = p_elem->data;
    p += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_ARRAY(data, hid_component_id);
    p = iap2_prep_param(p, PARAM_ID_ACC_HID_COMPONENT_ID, data, sizeof(uint16_t));
    p = iap2_prep_param(p, PARAM_ID_ACC_HID_REPORT, hid_report, hid_report_len);

    iap2_wrap_msg(p_link, p_elem->data, MSG_ACC_HID_REPORT, p_elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_hid_stop(uint8_t  bd_addr[6],
                  uint16_t hid_component_id)
{
    T_IAP_LINK *p_link;
    uint16_t total_len;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint8_t *p;
    uint8_t  data[2];

    PROFILE_PRINT_INFO1("iap_hid_stop: hid component identifier 0x%x", hid_component_id);

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint16_t) + IAP2_PAYLOAD_CHECKSUM_SIZE;

    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_hid_send_report: data_len too larger %d", total_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p = p_elem->data;

    BE_UINT16_TO_ARRAY(data, hid_component_id);
    p += IAP2_CTRL_MSG_HDR_SIZE;
    iap2_prep_param(p, PARAM_ID_HID_COMPONENT_ID, data, sizeof(uint16_t));
    iap2_wrap_msg(p_link, p_elem->data, MSG_STOP_HID, p_elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_send_bt_comp_info(uint8_t  bd_addr[6],
                           uint16_t comp_id,
                           bool     enable)
{
    T_IAP_LINK *p_link;
    uint16_t total_len;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint8_t *p;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE +
                (IAP2_PARAM_HDR_SIZE + sizeof(uint16_t)) +
                (IAP2_PARAM_HDR_SIZE + sizeof(uint8_t));
    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_send_bt_comp_info: data_len too larger %d", total_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p = p_elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_STREAM(p, IAP2_PARA_HDR_SIZE + 1 + IAP2_PARA_HDR_SIZE + IAP2_PARA_HDR_SIZE + 2);
    BE_UINT16_TO_STREAM(p, PARAM_ID_BT_COMPONENT_STATUS);

    BE_UINT16_TO_STREAM(p, IAP2_PARA_HDR_SIZE + 2);
    BE_UINT16_TO_STREAM(p, PARAM_ID_COMPONENT_ID);
    BE_UINT16_TO_STREAM(p, comp_id);

    BE_UINT16_TO_STREAM(p, IAP2_PARA_HDR_SIZE + 1);
    BE_UINT16_TO_STREAM(p, PARAM_ID_COMPONENT_ENABLED);
    BE_UINT8_TO_STREAM(p, enable);

    iap2_wrap_msg(p_link, p_elem->data, MSG_BT_COMPONENT_INFO, p_elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_send_start_bt_conn_update(uint8_t  bd_addr[6],
                                   uint16_t comp_id)
{
    T_IAP_LINK *p_link;
    uint16_t total_len;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint8_t *p;
    uint8_t  data[2];

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint16_t) + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_send_bt_comp_info: data_len too larger %d", total_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p = p_elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    BE_UINT16_TO_ARRAY(data, comp_id);
    iap2_prep_param(p, PARAM_ID_COMPONENT_ID, data, sizeof(uint16_t));
    iap2_wrap_msg(p_link, p_elem->data, MSG_START_BT_CONN_UPDATES,
                  p_elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_send_start_comm_update(uint8_t  bd_addr[6],
                                uint16_t param_id)
{
    T_IAP_LINK *p_link;
    uint16_t total_len;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint8_t *p;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_send_bt_comp_info: data_len too larger %d", total_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = IAP2_CTRL_MSG_PAYLOAD_HDR_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p = p_elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    iap2_prep_param(p, param_id, NULL, 0);
    iap2_wrap_msg(p_link, p_elem->data, MSG_START_COMM_UPDATES, IAP2_PARAM_HDR_SIZE);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_send_mute_status_update(uint8_t bd_addr[6],
                                 bool    status)
{
    T_IAP_LINK *p_link;
    uint16_t total_len;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint8_t *p;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    total_len = IAP2_CTRL_MSG_PARAM_HDR_SIZE + sizeof(uint8_t) + IAP2_PAYLOAD_CHECKSUM_SIZE;

    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_send_bt_comp_info: data_len too larger %d", total_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = total_len - IAP2_PKT_HDR_SIZE - IAP2_PAYLOAD_CHECKSUM_SIZE;
    p_elem->session_id = CTRL_SESSION_ID;

    p = p_elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    iap2_prep_param(p, PARAM_ID_MUTE_STATUS, &status, sizeof(uint8_t));
    iap2_wrap_msg(p_link, p_elem->data, MSG_MUTE_STATUS_UPDATE, p_elem->data_len - IAP2_MSG_HDR_SIZE);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_send_cmd(uint8_t   bd_addr[6],
                  uint16_t  msg_type,
                  uint8_t  *p_param,
                  uint16_t  param_len)
{
    T_IAP_LINK *p_link;
    uint16_t total_len;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint8_t *p;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    total_len = IAP2_CTRL_MSG_HDR_SIZE + param_len + IAP2_PAYLOAD_CHECKSUM_SIZE;

    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_send_cmd: data_len too larger %d", total_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->data_len = IAP2_MSG_HDR_SIZE + param_len;
    p_elem->session_id = CTRL_SESSION_ID;

    p = p_elem->data;

    p += IAP2_CTRL_MSG_HDR_SIZE;

    memcpy(p, p_param, param_len);
    iap2_wrap_msg(p_link, p_elem->data, msg_type, param_len);

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_send_data(uint8_t   bd_addr[6],
                   uint16_t  session_id,
                   uint8_t  *p_data,
                   uint16_t  data_len)
{
    T_IAP_LINK *p_link;
    T_IAP2_TX_DATA_ELEM *p_elem;
    uint16_t total_len;
    uint8_t *p;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        PROFILE_PRINT_ERROR0("iap_send_data: invalid addr");
        return false;
    }

    total_len = data_len + IAP2_EA_HDR_SIZE + IAP2_PAYLOAD_CHECKSUM_SIZE;
    if (total_len > p_link->dev_max_pkt_len)
    {
        PROFILE_PRINT_ERROR1("iap_send_data: data_len too larger %d", data_len);
        return false;
    }

    if (p_link->remain_tx_q_num == 0)
    {
        return false;
    }

    p_elem = os_mem_zalloc2(total_len + sizeof(T_IAP2_TX_DATA_ELEM));
    if (p_elem == NULL)
    {
        return false;
    }

    p_link->remain_tx_q_num--;
    p_elem->ea_session_id = session_id;
    p_elem->data_len = data_len + IAP2_EA_ID_SIZE;
    p_elem->session_id = EA_SESSION_ID;

    p = p_elem->data;

    /* fill in header data later */
    p += IAP2_PKT_HDR_SIZE;
    BE_UINT16_TO_STREAM(p, session_id);
    memcpy(p, p_data, data_len);
    p += data_len;
    BE_UINT8_TO_STREAM(p, iap_cal_checksum((uint8_t *)p_elem->data + IAP2_PKT_HDR_SIZE,
                                           p_elem->data_len));

    os_queue_in(&p_link->used_tx_q, p_elem);

    iap2_tx_data(p_link);

    return true;
}

bool iap_send_ack(uint8_t bd_addr[6],
                  uint8_t ack_seq)
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_link->dev_pkt_seq = ack_seq;
    iap2_send_ack(p_link);

    return true;
}

bool iap_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_IAP_INFO *p_data)
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_data->remote_credit = p_link->remote_credit;
    p_data->rfc_frame_size = p_link->rfc_frame_size;
    p_data->dlci = p_link->dlci;

    p_data->dev_max_pkt_len = p_link->dev_max_pkt_len;
    p_data->dev_retrans_tout = p_link->dev_retrans_tout;
    p_data->dev_cumulative_ack_tout = p_link->dev_cumulative_ack_tout;
    p_data->dev_max_out_pkt = p_link->dev_max_out_pkt;
    p_data->dev_max_retrans = p_link->dev_max_retrans;
    p_data->dev_max_culumative_ack = p_link->dev_max_culumative_ack;
    p_data->acked_seq = p_link->acked_seq;
    p_data->acc_pkt_seq = p_link->acc_pkt_seq;
    p_data->dev_pkt_seq = p_link->dev_pkt_seq;
    p_data->state = p_link->state;

    return true;
}

bool iap_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_IAP_INFO *p_data)
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = iap_alloc_link(bd_addr);
    }

    if (p_link)
    {
        p_link->state = (T_IAP_STATE)p_data->state;
        p_link->remote_credit = p_data->remote_credit;
        p_link->rfc_frame_size = p_data->rfc_frame_size;
        p_link->dlci = p_data->dlci;

        p_link->dev_max_pkt_len = p_data->dev_max_pkt_len;
        p_link->dev_retrans_tout = p_data->dev_retrans_tout;
        p_link->dev_cumulative_ack_tout = p_data->dev_cumulative_ack_tout;
        p_link->dev_max_out_pkt = p_data->dev_max_out_pkt;
        p_link->dev_max_retrans = p_data->dev_max_retrans;
        p_link->dev_max_culumative_ack = p_data->dev_max_culumative_ack;
        p_link->acked_seq = p_data->acked_seq;
        p_link->dev_pkt_seq = p_data->dev_pkt_seq;
        p_link->acc_pkt_seq = p_data->acc_pkt_seq;

        if (p_link->state > IAP_STATE_IAP2_SYNC)
        {
            T_IAP2_TX_DATA_ELEM *p_elem;

            if (p_link->p_payload)
            {
                os_mem_free(p_link->p_payload);
            }

            while (p_link->used_tx_q.count)
            {
                p_elem = os_queue_out(&p_link->used_tx_q);
                if (p_elem != NULL)
                {
                    os_mem_free(p_elem);
                }
            }

            while (p_link->sent_tx_q.count)
            {
                p_elem = os_queue_out(&p_link->sent_tx_q);
                if (p_elem != NULL)
                {
                    sys_timer_delete(p_elem->retrans_timer);
                    os_mem_free(p_elem);
                }
            }

            os_queue_init(&p_link->used_tx_q);
            os_queue_init(&p_link->sent_tx_q);

            if (p_link->dev_max_pkt_len > p_link->rfc_frame_size)
            {
                p_link->dev_max_pkt_len = p_link->rfc_frame_size;
            }

            p_link->remain_tx_q_num = p_iap->tx_q_elem_num;
        }

        return true;
    }

    return false;
}

bool iap_del_roleswap_info(uint8_t bd_addr[6])
{
    T_IAP_LINK *p_link;

    p_link = iap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    iap_free_link(p_link);

    return true;
}

uint8_t iap_get_rfc_profile_idx(void)
{
    return p_iap->iap_rfc_index;
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "iap.h"

bool iap_init(uint8_t  link_num,
              uint8_t  iap_chann_num,
              P_IAP_CB callback)
{
    return false;
}

bool iap_set_param(T_IAP_PARAM_TYPE  type,
                   uint8_t           len,
                   void             *p_value)
{
    return false;
}

bool iap_connect_req(uint8_t  bd_addr[6],
                     uint8_t  server_chann,
                     uint16_t frame_size,
                     uint8_t  init_credits)
{
    return false;
}

bool iap_connect_cfm(uint8_t  bd_addr[6],
                     bool     accept,
                     uint16_t frame_size,
                     uint8_t  init_credits)
{
    return false;
}

bool iap_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

uint8_t *iap2_prep_param(uint8_t  *p_param,
                         uint16_t  param_id,
                         void     *p_data,
                         uint16_t  data_len)
{
    return NULL;
}

bool iap_send_ident_info(uint8_t   bd_addr[6],
                         uint8_t  *p_ident,
                         uint16_t  ident_len)
{
    return false;
}

bool iap_eap_session_status_send(uint8_t                  bd_addr[6],
                                 uint16_t                 session_id,
                                 T_IAP_EAP_SESSION_STATUS status)
{
    return false;
}

bool iap_launch_app(uint8_t                  bd_addr[6],
                    char                    *boundle_id,
                    uint8_t                  len_boundle_id,
                    T_IAP_APP_LAUNCH_METHOD  method)
{
    return false;
}

bool iap_hid_start(uint8_t   bd_addr[6],
                   uint16_t  hid_component_id,
                   uint16_t  vid, uint16_t pid,
                   uint8_t  *hid_report_desc,
                   uint16_t  hid_report_desc_len)
{
    return false;
}

bool iap_hid_send_report(uint8_t   bd_addr[6],
                         uint16_t  hid_component_id,
                         uint8_t  *hid_report,
                         uint16_t  hid_report_len)
{
    return false;
}

bool iap_hid_stop(uint8_t  bd_addr[6],
                  uint16_t hid_component_id)
{
    return false;
}

bool iap_send_bt_comp_info(uint8_t  bd_addr[6],
                           uint16_t comp_id,
                           bool     enable)
{
    return false;
}

bool iap_send_start_bt_conn_update(uint8_t  bd_addr[6],
                                   uint16_t comp_id)
{
    return false;
}

bool iap_send_start_comm_update(uint8_t  bd_addr[6],
                                uint16_t param_id)
{
    return false;
}

bool iap_send_mute_status_update(uint8_t bd_addr[6],
                                 bool    status)
{
    return false;
}

bool iap_send_cmd(uint8_t   bd_addr[6],
                  uint16_t  msg_type,
                  uint8_t  *p_param,
                  uint16_t  param_len)
{
    return false;
}

bool iap_send_data(uint8_t   bd_addr[6],
                   uint16_t  session_id,
                   uint8_t  *p_data,
                   uint16_t  data_len)
{
    return false;
}

bool iap_send_ack(uint8_t bd_addr[6],
                  uint8_t ack_seq)
{
    return false;
}

bool iap_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_IAP_INFO *p_data)
{
    return false;
}

bool iap_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_IAP_INFO *p_data)
{
    return false;
}

bool iap_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}

uint8_t iap_get_rfc_profile_idx(void)
{
    return false;
}
#endif
