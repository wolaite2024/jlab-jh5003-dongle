/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "os_queue.h"
#include "bt_types.h"
#include "sys_timer.h"
#include "mpa.h"
#include "rfc.h"

#define RFC_MTU_SIZE        1021

#define RFC_N1              127     /**< Default Frame Size */
#define RFC_N2              0       /**< Number of Retransmissions */
#define RFC_K               0       /**< Window Size */
#define RFC_T2              30      /**< Default timeout for p/f reply */
#define RFC_T1              0       /**< Default timeout for command reply */

/** RFCOMM Header Types */
#define RFC_SABM            0x2F
#define RFC_UA              0x63
#define RFC_DM              0x0F
#define RFC_DISC            0x43
#define RFC_UIH             0xEF
#define RFC_UI              0x03

/** RFCOMM DLC 0 Type Field Codings */
#define RFC_TYPE_PN         0x20   /**< Parameter Negotiation */
#define RFC_TYPE_PSC        0x10   /**< Power Saving Control */
#define RFC_TYPE_CLD        0x30   /**< Mux Closedown */
#define RFC_TYPE_TEST       0x08   /**< Echo Command */
#define RFC_TYPE_FCON       0x28   /**< Flow Control ON ,TS07 means able to receive*/
#define RFC_TYPE_FCOFF      0x18   /**< Flow Control OFF ,TS07 means blocked*/
#define RFC_TYPE_MSC        0x38   /**< Modem Status Command */
#define RFC_TYPE_NSC        0x04   /**< Non Supported Command */
#define RFC_TYPE_RPN        0x24   /**< Remote Port Negotiation */
#define RFC_TYPE_RLS        0x14   /**< Remote Line Status Command */
#define RFC_TYPE_SNC        0x34   /**< Remote Service Negotiation */

/** PN Codings for Info Transfer */
#define RFC_INFO_UIH        0x00
#define RFC_INFO_UI         0x01    /**not used*/
#define RFC_INFO_I          0x02    /**not used*/

/** SIZES */
#define RFC_SIZE_PN         8      /**< Size of PN Parameter Set */

#define RFC_HEADER_SIZE     5   /**< maximum size of UIH header, including credit field */
#define RFC_CRC_FIELD_SIZE  1

#define RFC_RSVD_SIZE       (RFC_HEADER_SIZE + RFC_CRC_FIELD_SIZE)

/** Convergence Layer Definitions */
#define RFC_CONVERGENCE_0               0   /**< Std Convergence Layer */
#define RFC_CONVERGENCE_CREDIT_REQ      15  /**< Convergence Layer with Credit Based Flow Control */
#define RFC_CONVERGENCE_CREDIT_CFM      14  /**< Convergence Layer with Credit Based Flow Control, positive reply */

/** Poll Bit */
#define RFC_POLL_BIT        0x10

/** GSM 07.10 MSC command status bits */
#define RFC_DV_BIT          0x80
#define RFC_IC_BIT          0x40
#define RFC_RTR_BIT         0x08
#define RFC_RTC_BIT         0x04
#define RFC_FLOW_BIT        0x02        /**< Flow Bit Position in RFCOMM MSC Command */
#define RFC_EA_BIT          0x01        /**< extension bit,1 not extended */

#define RFC_MSC_CMD_RCV         0x01
#define RFC_MSC_RSP_RCV         0x02

#define RFC_MSC_CMD_OUTGOING    0x01
#define RFC_MSC_CMD_PENDING     0x02

#define RFC_WAIT_RSP_TIMEOUT    20000000
#define RFC_WAIT_MSC_TIMEOUT    20000000

#define RFC_DEFAULT_RPN_BANDRATE  0x07   /* 115200 Baud */
#define RFC_DEFAULT_RPN_OC3       0x03   /* charformat 8N1 */
#define RFC_DEFAULT_RPN_FLCTL     0x00   /* no flow control */
#define RFC_DEFAULT_RPN_XON       0x11
#define RFC_DEFAULT_RPN_XOFF      0x13
#define RFC_DEFAULT_RPN_PM1       0x00
#define RFC_DEFAULT_RPN_PM2       0x00

typedef enum t_dlci_state
{
    /* common state for control channel and data channel */
    DLCI_IDLE               = 0x00,
    DLCI_CONNECTING         = 0x01,
    DLCI_CONNECTED          = 0x02,
    DLCI_DISCONNECTING      = 0x03,
    DLCI_DISCONNECT_PENDING = 0x04,
    /* data channel specific state */
    DLCI_OPEN               = 0x05,
    DLCI_CONFIG_OUTGOING    = 0x06,
    DLCI_CONFIG_INCOMING    = 0x07,
    DLCI_CONFIG_ACCEPTED    = 0x08,
} T_DLCI_STATE;

typedef enum t_l2c_link_state
{
    LINK_IDLE           = 0x00,
    LINK_CONNECTING     = 0x01,
    LINK_CONNECTED      = 0x02,
    LINK_DISCONNECT_REQ = 0x03,
    LINK_DISCONNECTING  = 0x04,
} T_L2C_LINK_STATE;

/** RFCOMM IF Message Argument Definitions */
typedef struct t_rpn
{
    uint8_t     dlci;
    uint8_t     baudrate;
    uint8_t     oc3;
    uint8_t     flctl;
    uint8_t     xon;
    uint8_t     xoff;
    uint8_t     pm1;
    uint8_t     pm2;
} T_RPN;

typedef struct t_service_item
{
    struct t_service_item *next;
    P_RFC_SERVICE_CB       cback;
    uint8_t                local_server_chann;
    uint8_t                service_id;
} T_SERVICE_ITEM;

/** DLCI description structure */
typedef struct t_rfc_chann
{
    struct t_rfc_chann *next;
    struct t_rfc_chann *ctrl_chann;   /**< Back pointer to control channel for this channel */
    T_SERVICE_ITEM     *service;
    uint8_t             dlci;
    T_DLCI_STATE        state;

    /** Link Status Data (only for DLCI==0) */
    bool                initiator;
    T_L2C_LINK_STATE    link_state;     /**< state of l2cap link */
    uint16_t            l2c_cid;        /**< l2cap connection id */
    uint16_t            mtu_size;       /**< Max MTU Size for Remote Peer */
    uint8_t             bd_addr[6];     /**< bd_addr of l2cap connection */
    bool                link_initiator; /**< initiator of the link */
    uint8_t             msc_handshake;  /**< bit 0:received msc cmd, bit 1: received msc rsp */

    /** RFCOMM Link Configuration Data (only for DLCI != 0 */
    uint8_t             convergence_layer; /**< 0..3 for Convergence Layers 1..4, default 0 */
    uint8_t             init_credits;
    uint16_t            frame_size;

    /** Credit Based Flow Control */
    uint8_t             remote_remain_credits;   /**< remote device receive ability */
    uint8_t             given_credits;           /**< credits to return to remote side */

    /** MSC Flow Control */
    uint8_t             us_flow_ctrl;     /**< 1: upstream flow is BLOCKED, 0: unblock */
    uint8_t             us_flow_break;    /**< break status byte */
    uint8_t             us_flow_active;   /**< bit 0: one MSC outgoing, Bit 1: open MSC req stored */
    uint8_t             ds_flow_ctrl;     /**< downstream flow is BLOCKED */

    uint8_t             rls;          /**< Remote Line Status */

    T_SYS_TIMER_HANDLE  wait_rsp_timer_handle;    /*wait for response */
    T_SYS_TIMER_HANDLE  wait_msc_timer_handle;    /*wait for msc handshake */
} T_RFC_CHANN;

typedef struct t_rfc
{
    T_OS_QUEUE service_list;
    T_OS_QUEUE chann_list;
    uint8_t    ds_offset;
    uint8_t    queue_id;
    bool       enable_ertm;
} T_RFC;

static const uint8_t crc8_ets_table[256] =
{
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75,
    0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69,
    0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D,
    0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51,
    0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05,
    0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19,
    0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D,
    0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21,
    0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95,
    0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89,
    0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD,
    0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1,
    0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5,
    0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9,
    0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD,
    0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1,
    0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF
};

static T_RFC *rfc_db;

void rfc_wait_rsp_timeout(T_SYS_TIMER_HANDLE handle);
void rfc_wait_msc_timeout(T_SYS_TIMER_HANDLE handle);

uint8_t crc8_ets_gen(uint8_t  *p,
                     uint16_t  len)
{
    uint8_t fcs = 0xff;

    while (len--)
    {
        fcs = crc8_ets_table[fcs ^ *p++];
    }
    fcs = 0xff - fcs;

    return fcs;
}

bool crc8_ets_check(uint8_t  *p,
                    uint16_t  len,
                    uint8_t   rfcs)
{
    uint8_t fcs = 0xff;

    while (len--)
    {
        fcs = crc8_ets_table[fcs ^ *p++];
    }
    fcs = crc8_ets_table[fcs ^ rfcs];

    if (fcs == 0xcf)
    {
        return true;
    }

    return false;
}

void rfc_msg_post(T_RFC_MSG_TYPE  type,
                  T_SERVICE_ITEM *service,
                  void           *data)
{
    if (os_queue_search(&rfc_db->service_list, service) == true)
    {
        RFCOMM_PRINT_INFO1("rfc_msg_post: type 0x%02x", type);
        service->cback(type, data);
    }
}

T_RFC_CHANN *rfc_find_chann_by_dlci(uint8_t  dlci,
                                    uint16_t cid)
{
    T_RFC_CHANN *chann;

    chann = os_queue_peek(&rfc_db->chann_list, 0);
    while (chann != NULL)
    {
        if (chann->dlci == dlci && chann->ctrl_chann->l2c_cid == cid)
        {
            break;
        }

        chann = chann->next;
    }

    return chann;
}

T_RFC_CHANN *rfc_find_chann_by_addr(uint8_t bd_addr[6],
                                    uint8_t dlci)
{
    T_RFC_CHANN *chann;

    chann = os_queue_peek(&rfc_db->chann_list, 0);
    while (chann != NULL)
    {
        if (chann->dlci == dlci && !memcmp(chann->ctrl_chann->bd_addr, bd_addr, 6))
        {
            break;
        }

        chann = chann->next;
    }

    return chann;
}

void rfc_free_chann(T_RFC_CHANN *chann,
                    uint16_t     cause)
{
    T_SERVICE_ITEM     *service;
    T_RFC_DISCONN_CMPL  msg;
    uint8_t             dlci;

    dlci = chann->dlci;
    service = chann->service;
    memcpy(msg.bd_addr, chann->ctrl_chann->bd_addr, 6);
    sys_timer_delete(chann->wait_rsp_timer_handle);
    sys_timer_delete(chann->wait_msc_timer_handle);
    os_queue_delete(&rfc_db->chann_list, chann);
    free(chann);

    if (dlci)
    {
        msg.dlci = dlci;
        msg.cause = cause;
        rfc_msg_post(RFC_DISCONN_CMPL, service, (void *)&msg);
    }
}

void rfc_send_authen_req(T_RFC_CHANN *chann,
                         uint8_t      outgoing)
{
    mpa_send_rfc_authen_req(chann->ctrl_chann->bd_addr, chann->ctrl_chann->l2c_cid,
                            chann->dlci, chann->service->service_id, outgoing);
}

T_RFC_CHANN *rfc_alloc_chann(uint8_t dlci)
{
    T_RFC_CHANN *chann;
    uint32_t     ret = 0;

    chann = calloc(1, sizeof(T_RFC_CHANN));
    if (chann == NULL)
    {
        ret = 1;
        goto fail_alloc_link;
    }

    chann->wait_rsp_timer_handle = sys_timer_create("rfc_wait_rsp",
                                                    SYS_TIMER_TYPE_LOW_PRECISION,
                                                    (uint32_t)chann,
                                                    RFC_WAIT_RSP_TIMEOUT,
                                                    false,
                                                    rfc_wait_rsp_timeout);
    if (chann->wait_rsp_timer_handle == NULL)
    {
        ret = 2;
        goto fail_create_rsp_timer;
    }

    chann->wait_msc_timer_handle = sys_timer_create("rfc_wait_msc",
                                                    SYS_TIMER_TYPE_LOW_PRECISION,
                                                    (uint32_t)chann,
                                                    RFC_WAIT_MSC_TIMEOUT,
                                                    false,
                                                    rfc_wait_msc_timeout);
    if (chann->wait_msc_timer_handle == NULL)
    {
        ret = 3;
        goto fail_create_msc_timer;

    }

    chann->dlci = dlci;
    chann->convergence_layer = RFC_CONVERGENCE_CREDIT_REQ;
    chann->frame_size = RFC_N1;

    os_queue_in(&rfc_db->chann_list, chann);
    return chann;

fail_create_msc_timer:
    sys_timer_delete(chann->wait_rsp_timer_handle);
fail_create_rsp_timer:
    free(chann);
fail_alloc_link:
    RFCOMM_PRINT_ERROR1("rfc_alloc_chann: failed %d", ret);
    return NULL;
}

void rfc_init_credits(T_RFC_CHANN *chann,
                      uint16_t     credits)
{
    if (chann->convergence_layer == RFC_CONVERGENCE_CREDIT_REQ ||
        chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM)
    {
        chann->init_credits = credits;

        if (credits > 7)
        {
            chann->given_credits = credits - 7;
        }
        else
        {
            chann->given_credits = 0;
        }
    }
}

void rfc_check_send_credits(void)
{
    T_RFC_CHANN *chann;

    chann = os_queue_peek(&rfc_db->chann_list, 0);
    while (chann != NULL)
    {
        if ((chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM) &&
            (chann->given_credits > (chann->init_credits / 2)))
        {
            uint8_t *buf;
            uint8_t  offset;

            offset = rfc_db->ds_offset;
            buf = mpa_get_l2c_buf(rfc_db->queue_id,
                                  chann->ctrl_chann->l2c_cid,
                                  chann->dlci,
                                  5,
                                  offset,
                                  false);
            if (buf != NULL)
            {
                uint8_t *p;

                p = buf + offset;
                if (chann->ctrl_chann->initiator)
                {
                    LE_UINT8_TO_STREAM(p, (chann->dlci << 2) | 2 | RFC_EA_BIT);
                }
                else
                {
                    LE_UINT8_TO_STREAM(p, (chann->dlci << 2) | 0 | RFC_EA_BIT);
                }

                LE_UINT8_TO_STREAM(p, RFC_UIH | RFC_POLL_BIT);
                LE_UINT8_TO_STREAM(p, RFC_EA_BIT);
                LE_UINT8_TO_STREAM(p, chann->given_credits);
                LE_UINT8_TO_STREAM(p, crc8_ets_gen(buf + offset, 2));
                chann->given_credits = 0;
                mpa_send_l2c_data_req(buf, offset, chann->ctrl_chann->l2c_cid, 5, false);
            }
        }

        chann = chann->next;
    }
}

bool rfc_send_frame(T_RFC_CHANN *ctrl_chann,
                    uint8_t      dlci,
                    uint8_t      ctl,
                    uint8_t      cr,
                    uint8_t      poll,
                    uint8_t     *data,
                    uint16_t     len)
{
    uint8_t *buf;
    uint8_t *p;
    uint8_t  header_size;
    uint16_t length;
    uint8_t  offset;
    int32_t  ret = 0;

    if (ctrl_chann->link_state != LINK_CONNECTED)
    {
        ret = 1;
        goto fail_link_state;
    }

    if (len > 127)
    {
        header_size = 4;      /* 2 byte length*/
    }
    else
    {
        header_size = 3;      /* 1 byte length*/
    }
    length = (uint16_t)(len + header_size + RFC_CRC_FIELD_SIZE);

    offset = rfc_db->ds_offset;
    buf = mpa_get_l2c_buf(rfc_db->queue_id, ctrl_chann->l2c_cid, dlci, length, offset, false);
    if (buf == NULL)
    {
        ret = 2;
        goto fail_get_buf;
    }
    p = buf + offset;
    /* |EA(1bit) | C/R(1bit) | DLCI(6bit)| */
    *p = (dlci << 2) | RFC_EA_BIT;
    if ((cr && ctrl_chann->initiator) || (!cr && !ctrl_chann->initiator))
    {
        *p |= 2;
    }
    p++;

    /* insert frame type and poll bit */
    *p = ctl;
    if (poll)
    {
        *p |= RFC_POLL_BIT;
    }
    p++;

    /* insert length field */
    if (len > 127)
    {
        *p++ = len << 1;
        *p++ = len >> 7;
    }
    else
    {
        *p++ = (len << 1) | RFC_EA_BIT;
    }
    if (len)
    {
        memcpy(p, data, len);
        p += len;
    }

    /* for UIH, check only first 2 bytes */
    *p = crc8_ets_gen(buf + offset, (uint16_t)(ctl == RFC_UIH ? 2 : 3));
    mpa_send_l2c_data_req(buf, offset, ctrl_chann->l2c_cid, length, false);
    return true;

fail_get_buf:
fail_link_state:
    RFCOMM_PRINT_ERROR1("rfc_send_frame: failed %d", -ret);
    return false;
}

void rfc_send_sabm(T_RFC_CHANN *chann,
                   uint8_t      cmd,
                   uint8_t      poll)
{
    rfc_send_frame(chann->ctrl_chann, chann->dlci, RFC_SABM, cmd, poll, NULL, 0);
}

void rfc_send_ua(T_RFC_CHANN *chann,
                 uint8_t      cmd,
                 uint8_t      poll)
{
    rfc_send_frame(chann->ctrl_chann, chann->dlci, RFC_UA, cmd, poll, NULL, 0);
}

void rfc_send_dm(T_RFC_CHANN *ctrl_chann,
                 uint8_t      dlci,
                 uint8_t      cmd,
                 uint8_t      poll)
{
    rfc_send_frame(ctrl_chann, dlci, RFC_DM, cmd, poll, NULL, 0);
}

void rfc_send_disc(T_RFC_CHANN *chann,
                   uint8_t      cmd,
                   uint8_t      poll)
{
    rfc_send_frame(chann->ctrl_chann, chann->dlci, RFC_DISC, cmd, poll, NULL, 0);
}

void rfc_send_uih(T_RFC_CHANN *chann,
                  uint8_t      type,
                  uint8_t      cr,
                  uint8_t     *data,
                  uint16_t     len)
{
    uint8_t buf[32];

    buf[0] = (type << 2) | (cr << 1) | RFC_EA_BIT;
    buf[1] = (len << 1) + RFC_EA_BIT;

    /* limit the length (safety) */
    if (len > sizeof(buf) - 2)
    {
        len = sizeof(buf) - 2;
    }

    if (len)
    {
        memcpy(buf + 2, data, len);
    }

    rfc_send_frame(chann->ctrl_chann, chann->dlci, RFC_UIH, 1, 0, buf, len + 2);
}

void rfc_send_msc(T_RFC_CHANN *chann,
                  uint8_t      cr,
                  uint8_t      dlci,
                  uint8_t      status,
                  uint8_t      sbreak)
{
    uint8_t buf[3];
    uint8_t len = 2;

    buf[0] = (dlci << 2) | 2 | RFC_EA_BIT;
    buf[1] = status | RFC_DV_BIT | RFC_RTR_BIT | RFC_RTC_BIT | RFC_EA_BIT;
    if (sbreak & 1)
    {
        buf[1] = status;
        buf[2] = sbreak;
        len    = 3;
    }

    rfc_send_uih(chann, RFC_TYPE_MSC, cr, buf, len);
}

void rfc_send_rls(T_RFC_CHANN *chann,
                  uint8_t      cr,
                  uint8_t      dlci,
                  uint8_t      status)
{
    uint8_t buf[2];

    buf[0] = (dlci << 2) | 2 | RFC_EA_BIT;
    buf[1] = status | RFC_EA_BIT;

    rfc_send_uih(chann, RFC_TYPE_RLS, cr, buf, sizeof(buf));
}

void rfc_close_data_chann(T_RFC_CHANN *ctrl_chann,
                          uint16_t     cause)
{
    T_RFC_CHANN *chann;
    uint8_t      i = 0;

    chann = os_queue_peek(&rfc_db->chann_list, i);
    while (chann != NULL)
    {
        if ((chann->ctrl_chann == ctrl_chann) && chann->dlci)
        {
            rfc_free_chann(chann, cause);
        }
        else
        {
            i++;
        }

        chann = os_queue_peek(&rfc_db->chann_list, i);
    }

    rfc_check_send_credits();
}

void rfc_check_disconn_ctrl_chann(T_RFC_CHANN *ctrl_chann)
{
    T_RFC_CHANN *chann;

    chann = os_queue_peek(&rfc_db->chann_list, 0);
    while (chann != NULL)
    {
        if ((chann->ctrl_chann == ctrl_chann) &&
            (chann != ctrl_chann) && (chann->state != DLCI_IDLE))
        {
            break;
        }

        chann = chann->next;
    }

    if (chann == NULL)
    {
        switch (ctrl_chann->state)
        {
        case DLCI_IDLE: //link not establed
            if (ctrl_chann->link_state == LINK_CONNECTING && ctrl_chann->l2c_cid == 0)
            {
                ctrl_chann->link_state = LINK_DISCONNECT_REQ;
            }
            else
            {
                mpa_send_l2c_disconn_req(ctrl_chann->l2c_cid);
                ctrl_chann->link_state = LINK_DISCONNECTING;
            }
            break;

        case DLCI_CONNECTING: //SABM send, no UA received
            ctrl_chann->state = DLCI_DISCONNECT_PENDING;
            break;

        case DLCI_CONNECTED:
            ctrl_chann->state = DLCI_DISCONNECTING;
            sys_timer_start(ctrl_chann->wait_rsp_timer_handle);
            rfc_send_disc(ctrl_chann, 1, 1);
            break;

        default:
            break;
        }
    }
}

void rfc_wait_rsp_timeout(T_SYS_TIMER_HANDLE handle)
{
    T_RFC_CHANN *chann;

    chann = (void *)sys_timer_id_get(handle);
    if (chann)
    {
        RFCOMM_PRINT_WARN2("rfc_wait_rsp_timeout: bd_addr %s, dlci 0x%02x",
                           TRACE_BDADDR(chann->bd_addr), chann->dlci);

        chann->state = DLCI_IDLE;
        if (chann->dlci)
        {
            rfc_check_disconn_ctrl_chann(chann->ctrl_chann);
        }
        else
        {
            rfc_close_data_chann(chann, RFC_ERR | RFC_ERR_TIMEOUT);
            mpa_send_l2c_disconn_req(chann->l2c_cid);
            chann->link_state = LINK_DISCONNECTING;
        }
        rfc_free_chann(chann, RFC_ERR | RFC_ERR_TIMEOUT);
    }
}

void rfc_wait_msc_timeout(T_SYS_TIMER_HANDLE handle)
{
    T_RFC_CHANN *chann;

    chann = (void *)sys_timer_id_get(handle);
    if (chann)
    {
        T_RFC_CONN_CMPL msg;

        RFCOMM_PRINT_WARN2("rfc_wait_msc_timeout: bd_addr %s, dlci 0x%02x",
                           TRACE_BDADDR(chann->bd_addr), chann->dlci);

        chann->msc_handshake = RFC_MSC_CMD_RCV | RFC_MSC_RSP_RCV;
        msg.dlci = chann->dlci;
        msg.remain_credits = chann->remote_remain_credits;
        msg.service_id = chann->service->service_id;
        msg.frame_size = chann->frame_size;
        memcpy(msg.bd_addr, chann->ctrl_chann->bd_addr, 6);
        rfc_msg_post(RFC_CONN_CMPL, chann->service, (void *)&msg);
    }
}

void rfc_decode_pn(T_RFC_CHANN *chann,
                   uint8_t     *buf,
                   uint16_t     len)
{
    if (len == RFC_SIZE_PN)
    {
        chann->dlci = buf[0] & 0x3f;
        chann->convergence_layer = (buf[1] >> 4) & 0x0f;
        chann->frame_size = (uint16_t)buf[4] + ((uint16_t)buf[5] << 8);

        if ((chann->convergence_layer == RFC_CONVERGENCE_CREDIT_REQ) ||
            (chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM))
        {
            chann->remote_remain_credits = buf[7] & 0x07;
        }
    }
}

void rfc_encode_pn(T_RFC_CHANN *chann,
                   uint8_t     *buf)
{
    buf[0] = chann->dlci;
    buf[1] = RFC_INFO_UIH + (chann->convergence_layer << 4);
    buf[2] = 0;
    buf[3] = RFC_T1;
    buf[4] = (uint8_t)(chann->frame_size & 0xff);
    buf[5] = (uint8_t)(chann->frame_size >> 8);
    buf[6] = RFC_N2;
    buf[7] = chann->init_credits > 7 ? 7 : chann->init_credits;
}

bool rfc_check_block(T_RFC_CHANN *chann)
{
    if (chann->ctrl_chann->ds_flow_ctrl & RFC_FLOW_BIT)
    {
        return true;      /* aggregate flow control (on mux channel) stop all channels */
    }

    if (chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM)
    {
        if (chann->remote_remain_credits == 0)
        {
            RFCOMM_PRINT_TRACE1("rfc_check_block: dlci 0x%02x credits is 0", chann->dlci);
            return true;
        }

        return false;
    }

    if (chann->ds_flow_ctrl & RFC_FLOW_BIT)
    {
        return true;      /* channel specific flow control */
    }

    return false;
}

T_RFC_CHANN *rfc_check_l2c_collision(T_RFC_CHANN *ctrl_chann)
{
    T_RFC_CHANN *new_ctrl_chann;

    new_ctrl_chann = os_queue_peek(&rfc_db->chann_list, 0);
    while (new_ctrl_chann != NULL)
    {
        if ((new_ctrl_chann->dlci == 0) &&
            (!memcmp(new_ctrl_chann->bd_addr, ctrl_chann->bd_addr, 6)) &&
            (new_ctrl_chann->l2c_cid != ctrl_chann->l2c_cid))
        {
            /* found another ctrl chann for the same addr */
            RFCOMM_PRINT_INFO5("rfc_check_l2c_collision: collision ctrl chann with cid 0x%04x state %d link init %d, fail cid 0x%04x link init %d",
                               new_ctrl_chann->l2c_cid, new_ctrl_chann->state, new_ctrl_chann->link_initiator,
                               ctrl_chann->l2c_cid, ctrl_chann->link_initiator);
            break;
        }

        new_ctrl_chann = new_ctrl_chann->next;
    }

    return new_ctrl_chann;
}

void rfc_change_ctrl_chann(T_RFC_CHANN *ctrl_chann,
                           T_RFC_CHANN *new_ctrl_chann)
{
    T_RFC_CHANN *data_chann;

    /* change all data channel allocated for the ctrl chann to the new one */
    data_chann = os_queue_peek(&rfc_db->chann_list, 0);
    while (data_chann != NULL)
    {
        if ((data_chann->ctrl_chann == ctrl_chann) && data_chann->dlci)
        {
            RFCOMM_PRINT_INFO2("rfc_change_ctrl_chann: change ctrl chann for chann with dlci 0x%x, state %d",
                               data_chann->dlci, data_chann->state);

            data_chann->ctrl_chann = new_ctrl_chann;

            // change direction bit if needed
            if (ctrl_chann->link_initiator != new_ctrl_chann->link_initiator)
            {
                T_RFC_DLCI_CHANGE_INFO info;

                info.prev_dlci = data_chann->dlci;
                data_chann->dlci ^= 0x01;
                info.curr_dlci = data_chann->dlci;
                memcpy(info.bd_addr, new_ctrl_chann->bd_addr, 6);
                rfc_msg_post(RFC_DLCI_CHANGE, data_chann->service, (void *)&info);
            }

            if (data_chann->state == DLCI_OPEN && new_ctrl_chann->state == DLCI_CONNECTED)
            {
                /* check if frameSize setting for this channel is acceptable according to L2CAP mtuSize */
                if ((data_chann->frame_size + RFC_RSVD_SIZE) > data_chann->ctrl_chann->mtu_size)
                {
                    data_chann->frame_size = data_chann->ctrl_chann->mtu_size - RFC_RSVD_SIZE;
                }

                rfc_send_authen_req(data_chann, 1);
            }
        }

        data_chann = data_chann->next;
    }
}

void rfc_handle_pn(T_RFC_CHANN *ctrl_chann,
                   uint8_t      tcr,
                   uint8_t     *p,
                   uint16_t     len)
{
    uint8_t      dlci;
    T_RFC_CHANN *chann;

    LE_ARRAY_TO_UINT8(dlci, p);
    chann = rfc_find_chann_by_dlci(dlci, ctrl_chann->l2c_cid);

    if (tcr)    /* this is a command */
    {
        bool old_desc = false;

        if (chann)
        {
            old_desc = true;
        }
        else
        {
            T_SERVICE_ITEM *item;

            item = os_queue_peek(&rfc_db->service_list, 0);
            while (item != NULL)
            {
                if (item->local_server_chann == ((dlci >> 1) & 0x1F))
                {
                    break;
                }

                item = item->next;
            }

            if (item == NULL)
            {
                /* the server channel is not registered, send an unsolicited DM without poll bit */
                rfc_send_dm(ctrl_chann, dlci, 0, 0);
                return;
            }

            chann = rfc_alloc_chann(dlci);
            if (!chann)
            {
                /* could not allocate descriptor, send an unsolicited DM without poll bit */
                rfc_send_dm(ctrl_chann, dlci, 0, 0);
                return;
            }

            chann->ctrl_chann = ctrl_chann;
            chann->service = item;
        }

        RFCOMM_PRINT_INFO2("rfc_handle_pn: cmd for dlci 0x%02x on state 0x%02x",
                           chann->dlci, chann->state);

        /* check if the link is already open, if the link is open, do reject any changes to its configuration  */
        if (((chann->state != DLCI_CONNECTED) || old_desc) && (chann->state != DLCI_CONFIG_ACCEPTED))
        {
            rfc_decode_pn(chann, p, len);

            /* check if credit based flow control is requested */
            if (chann->convergence_layer == RFC_CONVERGENCE_CREDIT_REQ)
            {
                chann->convergence_layer = RFC_CONVERGENCE_CREDIT_CFM;
            }

            /* check if frameSize setting for this channel is acceptable according to L2CAP mtu_size */
            if ((chann->frame_size + RFC_RSVD_SIZE) > chann->ctrl_chann->mtu_size)
            {
                chann->frame_size = chann->ctrl_chann->mtu_size - RFC_RSVD_SIZE;
            }

            chann->state = DLCI_CONFIG_INCOMING;
            rfc_send_authen_req(chann, 0);
        }
        else
        {
            uint8_t pn[RFC_SIZE_PN];
            rfc_encode_pn(chann, pn);  /* answer with unchanged configuration */
            rfc_send_uih(ctrl_chann, RFC_TYPE_PN, 0, pn, sizeof(pn));
        }
    }
    else /* this is a response */
    {
        RFCOMM_PRINT_INFO2("rfc_handle_pn: reponse for dlci 0x%02x, cid 0x%04x",
                           dlci, ctrl_chann->l2c_cid);
        if (chann != NULL)
        {
            /* check response on an open channel, a reconfig-request, this is only supported for UPF4, ignore responses */
            if (chann->state != DLCI_CONNECTED)
            {
                uint16_t frame_size;

                frame_size = chann->frame_size;
                rfc_decode_pn(chann, p, len);

                /* peer increased framesize in response, use request value (better: disconnect link) */
                if (chann->frame_size > frame_size)
                {
                    chann->frame_size = frame_size;
                }

                /* recheck the configuration that was sent by remote entity */
                if (chann->convergence_layer == RFC_CONVERGENCE_CREDIT_REQ)
                {
                    /* if the request is still set, then remote side does not understand credit based flow control (rfcomm p.15) */
                    chann->convergence_layer = RFC_CONVERGENCE_0;
                }

                chann->state = DLCI_CONNECTING;
                /* start SABM timer */
                sys_timer_start(chann->wait_rsp_timer_handle);
                rfc_send_sabm(chann, 1, 1);
            }
        }
    }
}

void rfc_handle_msc(T_RFC_CHANN *ctrl_chann,
                    uint8_t      tcr,
                    uint8_t     *p)
{
    uint8_t      dlci;
    T_RFC_CHANN *chann;

    LE_STREAM_TO_UINT8(dlci, p);
    dlci = dlci >> 2;
    RFCOMM_PRINT_INFO3("rfc_handle_msc: dlci 0x%02x, tcr %d, cid 0x%04x",
                       dlci, tcr, ctrl_chann->l2c_cid);

    chann = rfc_find_chann_by_dlci(dlci, ctrl_chann->l2c_cid);
    if (chann != NULL)
    {
        uint8_t msc_status;
        uint8_t status;

        LE_STREAM_TO_UINT8(status, p);
        msc_status = chann->msc_handshake;

        if (!tcr)   /* response */
        {
            chann->msc_handshake |= RFC_MSC_RSP_RCV;
            chann->us_flow_active &= ~RFC_MSC_CMD_OUTGOING;
            if (chann->us_flow_active & RFC_MSC_CMD_PENDING)
            {
                /* at least one intermediate request is stored locally --> send it now */
                chann->us_flow_active |= RFC_MSC_CMD_OUTGOING;    /* one MSC request outstanding */
                chann->us_flow_active &= ~RFC_MSC_CMD_PENDING;    /* nothing stored locally any more */
                sys_timer_start(chann->ctrl_chann->wait_rsp_timer_handle);
                rfc_send_msc(chann->ctrl_chann, 1, chann->dlci, chann->us_flow_ctrl,
                             chann->us_flow_break);
            }
        }
        else    /* command */
        {
            chann->msc_handshake |= RFC_MSC_CMD_RCV;
            rfc_send_msc(ctrl_chann, 0, dlci, status, 0);
            chann->ds_flow_ctrl = status;
        }

        if ((msc_status != chann->msc_handshake) &&
            (chann->msc_handshake == (RFC_MSC_CMD_RCV | RFC_MSC_RSP_RCV)))
        {
            T_RFC_CONN_CMPL msg;

            /* handshake never seen and T2 is not expired, so this is the first indication */
            sys_timer_stop(chann->wait_msc_timer_handle);

            msg.dlci = chann->dlci;
            msg.remain_credits = chann->remote_remain_credits;
            msg.service_id = chann->service->service_id;
            msg.frame_size = chann->frame_size;
            memcpy(msg.bd_addr, chann->ctrl_chann->bd_addr, 6);
            rfc_msg_post(RFC_CONN_CMPL, chann->service, (void *)&msg);

            /* there might have changes that affect the ability to send data */
            rfc_check_send_credits();
        }
    }
}

void rfc_handle_rls(T_RFC_CHANN *ctrl_chann,
                    uint8_t      tcr,
                    uint8_t     *p)
{
    uint8_t      dlci;
    T_RFC_CHANN *chann;

    LE_STREAM_TO_UINT8(dlci, p);
    dlci = dlci >> 2;
    chann = rfc_find_chann_by_dlci(dlci, ctrl_chann->l2c_cid);
    if (chann != NULL)
    {
        if (tcr)
        {
            uint8_t status;

            LE_STREAM_TO_UINT8(status, p);
            /* reply the command with a response, copy local values */
            rfc_send_rls(ctrl_chann, 0, dlci, status);
            chann->rls = status;
        }
    }
}

void rfc_handle_rpn(T_RFC_CHANN *ctrl_chann,
                    uint8_t      tcr,
                    T_RPN       *rpn,
                    uint16_t     tlen)
{
    if (tlen == 1 || tlen == 8)
    {
        T_RFC_CHANN *chann;
        T_RPN        rsp;
        uint8_t      dlci;

        dlci = rpn->dlci >> 2;
        chann = rfc_find_chann_by_dlci(dlci, ctrl_chann->l2c_cid);
        if (chann == NULL)
        {
            chann = rfc_alloc_chann(dlci);
            if (chann != NULL)
            {
                chann->ctrl_chann = ctrl_chann;
                memcpy(&rsp, rpn, (uint8_t)tlen);
            }
        }
        else
        {
            rsp.dlci     = (dlci << 2) | 2 | RFC_EA_BIT;
            rsp.baudrate = RFC_DEFAULT_RPN_BANDRATE;
            rsp.oc3      = RFC_DEFAULT_RPN_OC3;
            rsp.flctl    = RFC_DEFAULT_RPN_FLCTL;
            rsp.xon      = RFC_DEFAULT_RPN_XON;
            rsp.xoff     = RFC_DEFAULT_RPN_XOFF;
            rsp.pm1      = RFC_DEFAULT_RPN_PM1;
            rsp.pm2      = RFC_DEFAULT_RPN_PM2;
        }

        if (chann != NULL)
        {
            if (tcr)
            {
                /* if this is a command, send indication to upper layer */
                rfc_send_uih(ctrl_chann, RFC_TYPE_RPN, 0, (uint8_t *)&rsp, sizeof(T_RPN));
            }
        }
    }
}

void rfc_handle_uih(T_RFC_CHANN *ctrl_chann,
                    uint8_t     *data,
                    uint16_t     len,
                    bool         cr)
{
    uint8_t  *p;
    uint8_t   type;
    uint8_t   uih_cr;
    uint16_t  uih_len;

    p = data;
    LE_STREAM_TO_UINT8(type, p);
    uih_cr = type & 2;
    type >>= 2;

    LE_STREAM_TO_UINT8(uih_len, p);
    if (uih_len & RFC_EA_BIT)
    {
        uih_len >>= 1;
    }
    else
    {
        uih_len = (uih_len >> 1) | (*p++ << 7);
    }

    /* if this is a response, then stop response waiting timer */
    if (!uih_cr)
    {
        sys_timer_stop(ctrl_chann->wait_rsp_timer_handle);
    }

    switch (type)
    {
    case RFC_TYPE_PN:
        len -= p - data;
        rfc_handle_pn(ctrl_chann, uih_cr, p, len);
        break;

    case RFC_TYPE_FCON:
        if (uih_cr)
        {
            ctrl_chann->ds_flow_ctrl = false;
            rfc_send_uih(ctrl_chann, RFC_TYPE_FCON, 0, NULL, 0);
            rfc_check_send_credits();
        }
        break;

    case RFC_TYPE_FCOFF:
        if (uih_cr)
        {
            ctrl_chann->ds_flow_ctrl = RFC_FLOW_BIT;
            rfc_send_uih(ctrl_chann, RFC_TYPE_FCOFF, 0, NULL, 0);
        }
        break;

    case RFC_TYPE_MSC:
        rfc_handle_msc(ctrl_chann, uih_cr, p);
        break;

    case RFC_TYPE_RLS:
        rfc_handle_rls(ctrl_chann, uih_cr, p);
        break;

    case RFC_TYPE_RPN:
        rfc_handle_rpn(ctrl_chann, uih_cr, (T_RPN *)p, uih_len);
        break;

    case RFC_TYPE_TEST:
        if (uih_cr)
        {
            /* send back identical data as response */
            rfc_send_uih(ctrl_chann, RFC_TYPE_TEST, 0, p, uih_len);
        }
        break;

    case RFC_TYPE_NSC:
        /* NSC received:  shutdown all user channels and close the link */
        rfc_close_data_chann(ctrl_chann->ctrl_chann, RFC_ERR | RFC_ERR_NSC);
        mpa_send_l2c_disconn_req(ctrl_chann->ctrl_chann->l2c_cid);
        ctrl_chann->ctrl_chann->link_state = LINK_DISCONNECTING;
        break;

    default:
        RFCOMM_PRINT_ERROR1("rfc_handle_uih: unhandled type 0x%02x", type);
        if (cr)
        {
            /* reply with non supported command */
            rfc_send_uih(ctrl_chann, RFC_TYPE_NSC, 0, data, 1);
        }
        break;
    }
}

void rfc_handle_sabm(T_RFC_CHANN *chann,
                     uint8_t      poll)
{
    RFCOMM_PRINT_INFO6("rfc_handle_sabm: dlci 0x%02x, init %d, state 0x%02x, l2c_init %d, l2c_state 0x%02x, poll %d",
                       chann->dlci, chann->initiator, chann->state,
                       chann->link_initiator, chann->link_state, poll);

    if (!poll)
    {
        return;
    }

    if (chann->dlci == 0)     /* control channel */
    {
        T_RFC_CHANN *data_chann;

        if (chann->state == DLCI_CONNECTING)      /* conflict SABM */
        {
            chann->initiator = false;
            sys_timer_stop(chann->wait_rsp_timer_handle);   /* remote may not send UA for previous SABM */
        }

        rfc_send_ua(chann, 0, 1);
        chann->state = DLCI_CONNECTED;

        /* check if data chennel from collison need to connect */
        data_chann = os_queue_peek(&rfc_db->chann_list, 0);
        while (data_chann != NULL)
        {
            if (data_chann->state == DLCI_OPEN && data_chann->ctrl_chann == chann)
            {
                /* check if frameSize setting for this channel is acceptable according to L2CAP mtuSize */
                if ((data_chann->frame_size + RFC_RSVD_SIZE) > chann->mtu_size)
                {
                    data_chann->frame_size = chann->mtu_size - RFC_RSVD_SIZE;
                }

                rfc_send_authen_req(data_chann, 1);
            }

            data_chann = data_chann->next;
        }
    }
    else                        /* data channel */
    {
        switch (chann->state)
        {
        case DLCI_CONFIG_ACCEPTED:
            /* dlci is now open */
            chann->state = DLCI_CONNECTED;
            rfc_send_ua(chann, 0, 1);

            /* send initial MSC command on channel... */
            chann->us_flow_active |= RFC_MSC_CMD_OUTGOING;
            sys_timer_start(chann->ctrl_chann->wait_rsp_timer_handle);
            rfc_send_msc(chann->ctrl_chann, 1, chann->dlci, chann->us_flow_ctrl, 0);

            /* User channel is now connected, we must wait for MSC_IND before we can */
            /* signal to upper layer (Peiker - Verizone problem */
            sys_timer_start(chann->wait_msc_timer_handle);
            break;

        case DLCI_CONNECTED:
            /* dlci is already online, replace directly with UA */
            rfc_send_ua(chann, 0, 1);
            break;

        default:
            /* SABM on traffic channel received without prior TYPE_PN exchange: refuse by sending DM */
            rfc_send_dm(chann->ctrl_chann, chann->dlci, 0, 1);
            rfc_free_chann(chann, RFC_ERR | RFC_ERR_INVALID_STATE);
            break;
        }
    }
}

void rfc_handle_ua(T_RFC_CHANN *chann,
                   uint8_t      poll)
{
    RFCOMM_PRINT_INFO3("rfc_handle_ua: dlci 0x%02x, state 0x%02x, poll %d",
                       chann->dlci, chann->state, poll);
    if (!poll)
    {
        return;
    }
    sys_timer_stop(chann->wait_rsp_timer_handle);

    if (chann->dlci == 0)     /* control channel */
    {
        switch (chann->state)
        {
        case DLCI_CONNECTING:
            {
                T_RFC_CHANN *data_chann;

                chann->state = DLCI_CONNECTED;
                data_chann = os_queue_peek(&rfc_db->chann_list, 0);
                while (data_chann != NULL)
                {
                    if (data_chann->state == DLCI_OPEN && data_chann->ctrl_chann == chann)
                    {
                        /* check if frameSize setting for this channel is acceptable according to L2CAP mtuSize */
                        if ((data_chann->frame_size + RFC_RSVD_SIZE) > chann->mtu_size)
                        {
                            data_chann->frame_size = chann->mtu_size - RFC_RSVD_SIZE;
                        }
                        rfc_send_authen_req(data_chann, 1);
                    }

                    data_chann = data_chann->next;
                }
            }
            break;

        case DLCI_DISCONNECT_PENDING:
            {
                chann->state = DLCI_DISCONNECTING;
                sys_timer_start(chann->wait_rsp_timer_handle);
                rfc_send_disc(chann, 1, 1);
            }
            break;

        case DLCI_DISCONNECTING:
            {
                chann->state = DLCI_IDLE;
                rfc_close_data_chann(chann, RFC_SUCCESS);
                mpa_send_l2c_disconn_req(chann->l2c_cid);
                chann->link_state = LINK_DISCONNECTING;
            }
            break;

        default:
            break;
        }
    }
    else                        /* data channel */
    {
        switch (chann->state)
        {
        case DLCI_CONNECTING:
            {
                chann->state = DLCI_CONNECTED;
                /* User channel is now connected, we must wait for MSC_IND before we can */
                /* signal to upper layer (Peiker - Verizone problem                      */
                sys_timer_start(chann->wait_msc_timer_handle);
                chann->us_flow_active |= RFC_MSC_CMD_OUTGOING;
                sys_timer_start(chann->ctrl_chann->wait_rsp_timer_handle);
                rfc_send_msc(chann->ctrl_chann, 1, chann->dlci, chann->us_flow_ctrl, 0);
            }
            break;

        case DLCI_DISCONNECT_PENDING:
            {
                chann->state = DLCI_DISCONNECTING;
                sys_timer_start(chann->wait_rsp_timer_handle);
                rfc_send_disc(chann, 1, 1);
            }
            break;

        case DLCI_DISCONNECTING:
            {
                chann->state = DLCI_IDLE;
                rfc_check_disconn_ctrl_chann(chann->ctrl_chann);
                rfc_free_chann(chann, RFC_SUCCESS);
            }
            break;

        default:
            break;
        }
    }
}

void rfc_handle_dm(T_RFC_CHANN *chann)
{
    RFCOMM_PRINT_INFO2("rfc_handle_dm: dlci 0x%02x, state 0x%02x", chann->dlci, chann->state);

    if (chann->state != DLCI_IDLE)
    {
        sys_timer_stop(chann->wait_rsp_timer_handle);
        chann->state = DLCI_IDLE;

        if (chann->dlci)
        {
            rfc_check_disconn_ctrl_chann(chann->ctrl_chann);
        }
        else
        {
            rfc_close_data_chann(chann, RFC_ERR | RFC_ERR_INVALID_STATE);
            mpa_send_l2c_disconn_req(chann->l2c_cid);
            chann->link_state = LINK_DISCONNECTING;
        }

        rfc_free_chann(chann, RFC_ERR | RFC_ERR_INVALID_STATE);
        rfc_check_send_credits();
    }
}

void rfc_handle_disc(T_RFC_CHANN *chann,
                     uint8_t      poll)
{
    RFCOMM_PRINT_INFO3("rfc_handle_disc: dlci 0x%02x, state 0x%02x, poll %d",
                       chann->dlci, chann->state, poll);

    if (poll)
    {
        switch (chann->state)
        {
        case DLCI_CONNECTED:
            rfc_send_ua(chann, 0, 1);
            break;

        case DLCI_DISCONNECTING:
            rfc_send_ua(chann, 0, 1);
            /* we handle this internally as if a UA was received */
            sys_timer_stop(chann->wait_rsp_timer_handle);
            chann->state = DLCI_IDLE;
            if (chann->dlci == 0)
            {
                rfc_close_data_chann(chann, RFC_SUCCESS);
                mpa_send_l2c_disconn_req(chann->l2c_cid);
                chann->link_state = LINK_DISCONNECTING;
            }
            else
            {
                rfc_check_disconn_ctrl_chann(chann->ctrl_chann);
                rfc_free_chann(chann, RFC_SUCCESS);
            }
            return;

        default:
            /* this is a DISC on a not open channel */
            rfc_send_dm(chann->ctrl_chann, chann->dlci, 0, 1);
            break;
        }

        chann->state = DLCI_IDLE;
        if (chann->dlci == 0)     /* l2cap chann will be disconnect by remote side */
        {
            rfc_close_data_chann(chann, RFC_SUCCESS);
        }
        else
        {
            rfc_free_chann(chann, RFC_SUCCESS);
        }
        rfc_check_send_credits();
    }
}

void rfc_handle_l2c_conn_ind(T_MPA_L2C_CONN_IND *ind)
{
    T_RFC_CHANN              *ctrl_chann;
    uint8_t                   mode = MPA_L2C_MODE_BASIC;
    T_MPA_L2C_CONN_CFM_CAUSE  status = MPA_L2C_CONN_NO_RESOURCE;

    /* do not check if ctrl chann exists, when collision happens, android phone will start a timer
       when receive our l2cap connection request, if we acccept its l2cap connection request, it will
       reject ours. The two control channel will be distinguished by cid. */
    ctrl_chann = rfc_alloc_chann(0);
    if (ctrl_chann)
    {
        status = MPA_L2C_CONN_ACCEPT;
        ctrl_chann->l2c_cid = ind->cid;
        ctrl_chann->ctrl_chann = ctrl_chann;
        memcpy(ctrl_chann->bd_addr, ind->bd_addr, 6);
        ctrl_chann->link_state = LINK_CONNECTING;
    }

    if (rfc_db->enable_ertm)
    {
        mode |= MPA_L2C_MODE_ERTM;
    }

    mpa_send_l2c_conn_cfm(status, ind->cid, RFC_MTU_SIZE, mode, 0xFFFF);
}

void rfc_handle_l2c_conn_rsp(T_MPA_L2C_CONN_RSP *rsp)
{
    T_RFC_CHANN *ctrl_chann;

    RFCOMM_PRINT_INFO2("rfc_handle_l2c_conn_rsp: bd_addr %s, cause 0x%04x",
                       TRACE_BDADDR(rsp->bd_addr), rsp->cause);

    ctrl_chann = rfc_find_chann_by_addr(rsp->bd_addr, 0);
    if (ctrl_chann)
    {
        if (rsp->cause)
        {
            rfc_close_data_chann(ctrl_chann, rsp->cause);
            ctrl_chann->link_state = LINK_IDLE;
            rfc_free_chann(ctrl_chann, rsp->cause);
            return;
        }

        /* FIXME JW not needed anymore */
        ctrl_chann->l2c_cid = rsp->cid;
        if (ctrl_chann->link_state == LINK_DISCONNECT_REQ)
        {
            RFCOMM_PRINT_WARN1("rfc_handle_l2c_conn_rsp: disc requested for cid 0x%04x", rsp->cid);
            rfc_close_data_chann(ctrl_chann, rsp->cause);
            mpa_send_l2c_disconn_req(rsp->cid);
            ctrl_chann->link_state = LINK_DISCONNECTING;
        }
    }
}

void rfc_handle_l2c_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *info)
{
    T_RFC_CHANN *ctrl_chann;
    T_RFC_CHANN *new_ctrl_chann;

    RFCOMM_PRINT_INFO4("rfc_handle_l2c_conn_cmpl: bd_addr %s, cause 0x%04x, cid 0x%04x, mtu %d",
                       TRACE_BDADDR(info->bd_addr), info->cause, info->cid, info->remote_mtu);

    ctrl_chann = rfc_find_chann_by_dlci(0, info->cid);
    if (!ctrl_chann)
    {
        if (info->cause == 0)
        {
            mpa_send_l2c_disconn_req(info->cid);
        }
        return;
    }

    if (info->cause)
    {
        /* check if this fail happened because of collision */
        new_ctrl_chann = rfc_check_l2c_collision(ctrl_chann);

        if (new_ctrl_chann)
        {
            rfc_change_ctrl_chann(ctrl_chann, new_ctrl_chann);
        }
        else
        {
            rfc_close_data_chann(ctrl_chann, info->cause);
        }

        ctrl_chann->link_state = LINK_IDLE;
        rfc_free_chann(ctrl_chann, info->cause);
    }
    else
    {
        /* incase phone accept our l2cap connection request after we accept its */
        new_ctrl_chann = rfc_check_l2c_collision(ctrl_chann);
        if (new_ctrl_chann)
        {
            rfc_change_ctrl_chann(ctrl_chann, new_ctrl_chann);
            mpa_send_l2c_disconn_req(ctrl_chann->l2c_cid);
        }
        else
        {
            rfc_db->ds_offset  = info->ds_data_offset;
            ctrl_chann->mtu_size = info->remote_mtu;
            ctrl_chann->link_state = LINK_CONNECTED;

            if (ctrl_chann->state == DLCI_IDLE && ctrl_chann->link_initiator)
            {
                ctrl_chann->initiator = true;
                ctrl_chann->state = DLCI_CONNECTING;
                sys_timer_start(ctrl_chann->wait_rsp_timer_handle);
                rfc_send_sabm(ctrl_chann, 1, 1);
            }
        }
    }
}

void rfc_handle_l2c_disconn_ind(T_MPA_L2C_DISCONN_IND *ind)
{
    T_RFC_CHANN *ctrl_chann;

    RFCOMM_PRINT_INFO1("rfc_handle_l2c_disconn_ind: cid 0x%04x", ind->cid);

    mpa_send_l2c_disconn_cfm(ind->cid);
    ctrl_chann = rfc_find_chann_by_dlci(0, ind->cid);
    if (ctrl_chann != NULL)
    {
        ctrl_chann->link_state = LINK_IDLE;
        rfc_close_data_chann(ctrl_chann, ind->cause);
        rfc_free_chann(ctrl_chann, ind->cause);
    }
}

void rfc_handle_l2c_disconn_rsp(T_MPA_L2C_DISCONN_RSP *rsp)
{
    T_RFC_CHANN *ctrl_chann;

    RFCOMM_PRINT_INFO1("rfc_handle_l2c_disconn_rsp: cid 0x%04x", rsp->cid);

    ctrl_chann = rfc_find_chann_by_dlci(0, rsp->cid);
    if (ctrl_chann != NULL)
    {
        ctrl_chann->link_state = LINK_IDLE;
        rfc_close_data_chann(ctrl_chann, rsp->cause);
        rfc_free_chann(ctrl_chann, rsp->cause);
    }
}

void rfc_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *ind)
{
    uint8_t     *p;
    T_RFC_CHANN *chann;
    uint8_t      dlci;
    uint8_t      cr;                   /* cr bit from frame */
    uint8_t      type;
    uint8_t      poll;                 /* poll bit from frame */
    uint16_t     len;
    uint8_t      return_credits = 0;
    bool         command;

    if (ind->length < 4)
    {
        return;
    }

    p = ind->data + ind->gap;
    dlci = *p++;
    cr = (dlci >> 1) & 1;
    dlci = dlci >> 2;

    type = *p++;
    poll = type & RFC_POLL_BIT;
    type = type & ~RFC_POLL_BIT;

    len = *p++;
    if (len & 1)
    {
        len >>= 1;
        ind->length -= 3;
    }
    else
    {
        len >>= 1;
        len |= (uint16_t)(*p++) << 7;
        ind->length -= 4;
    }

    if (dlci && (type == RFC_UIH) && poll)
    {
        /* this is a UIH frame on traffic channel mit poll bit set: it contains backCredits field! */
        return_credits = *p++; /* fetch the return credits and remove the field from the payload */
        ind->length--;
    }

    /* make some additional syntax checks on the frame */
    if (len + RFC_CRC_FIELD_SIZE != ind->length)
    {
        return;
    }

    if (crc8_ets_check(ind->data + ind->gap, type == RFC_UIH ? 2 : 3, *(p + len)) == false)
    {
        return;
    }

    chann = rfc_find_chann_by_dlci(dlci, ind->cid);
    if (!chann)
    {
        if (dlci)
        {
            T_RFC_CHANN *ctrl_chann;

            ctrl_chann = rfc_find_chann_by_dlci(0, ind->cid);
            if (ctrl_chann)
            {
                if (type == RFC_SABM)
                {
                    /* recv sabm with out pn or chann is freed when cfm not accept, just send dm */
                    rfc_send_dm(ctrl_chann, dlci, 0, 1);
                }
                else if (type != RFC_DM)
                {
                    /* we ignore DM on non existing channels */
                    if ((cr && !ctrl_chann->initiator) || (!cr && ctrl_chann->initiator))
                    {
                        rfc_send_dm(ctrl_chann, dlci, 0, 0);
                    }
                }
            }
        }

        return;
    }

    /* it is a command if cr=1 and peer is initiator or if cr=0 and peer is not initiator */
    if ((cr && !chann->ctrl_chann->initiator) || (!cr && chann->ctrl_chann->initiator))
    {
        command = true;
    }
    else
    {
        command = false;
    }

    switch (type)
    {
    case RFC_SABM:
        rfc_handle_sabm(chann, poll);
        break;

    case RFC_UA:
        rfc_handle_ua(chann, poll);
        break;

    case RFC_DM:
        rfc_handle_dm(chann);
        break;

    case RFC_DISC:
        rfc_handle_disc(chann, poll);
        break;

    case RFC_UIH:
        /* check if the link is connected, answer with DM if not */
        if (chann->state != DLCI_CONNECTED)
        {
            rfc_send_dm(chann->ctrl_chann, chann->dlci, 0, 0);
            break;
        }

        if (dlci == 0)
        {
            rfc_handle_uih(chann, p, len, command);
            break;
        }

        /* user data on dlci */
        if ((chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM) && poll && return_credits)
        {
            /* there is a return credit field in the packet */
            chann->remote_remain_credits += return_credits;

            RFCOMM_PRINT_TRACE3("rfc_handle_l2c_data_ind: dlci 0x%02x, get credits %d, credits now %d",
                                chann->dlci, return_credits, chann->remote_remain_credits);

            rfc_check_send_credits();
        }

        if (len > 0)     /* remote send data */
        {
            T_RFC_DATA_IND msg;

            msg.dlci = chann->dlci;
            msg.remain_credits = chann->remote_remain_credits;
            msg.buf = p;
            msg.length = len;
            memcpy(msg.bd_addr, chann->ctrl_chann->bd_addr, 6);
            rfc_msg_post(RFC_DATA_IND, chann->service, (void *)&msg);
        }
        else            /* remote only send credits */
        {
            T_RFC_CREDIT_INFO msg;

            msg.dlci = chann->dlci;
            msg.remain_credits = chann->remote_remain_credits;
            memcpy(msg.bd_addr, chann->ctrl_chann->bd_addr, 6);
            rfc_msg_post(RFC_CREDIT_INFO, chann->service, (void *)&msg);
        }
        break;

    default:
        break;
    }
}

void rfc_handle_l2c_data_rsp(T_MPA_L2C_DATA_RSP *rsp)
{
    T_RFC_CHANN *chann;

    chann = rfc_find_chann_by_dlci(rsp->dlci, rsp->cid);
    if (chann != NULL)
    {
        T_RFC_DATA_RSP msg;

        msg.dlci = rsp->dlci;
        memcpy(msg.bd_addr, chann->ctrl_chann->bd_addr, 6);
        rfc_msg_post(RFC_DATA_RSP, chann->service, (void *)&msg);
    }
}

void rfc_handle_sec_reg_rsp(T_MPA_L2C_SEC_REG_RSP *rsp)
{
    T_RFC_SEC_REG_RSP  sec_rsp;
    T_SERVICE_ITEM    *item;

    sec_rsp.server_chann = rsp->server_chann;
    sec_rsp.active = rsp->active;
    sec_rsp.cause = rsp->cause;

    item = os_queue_peek(&rfc_db->service_list, 0);
    while (item != NULL)
    {
        if (item->local_server_chann == rsp->server_chann)
        {
            item->cback(RFC_SEC_REG_RSP, (void *)&sec_rsp);
            return;
        }

        item = item->next;
    }
}

void rfc_handle_author_ind(T_MPA_AUTHOR_REQ_IND *ind)
{
    /* for rfcomm, uuid in authorization request indicate is service_cb_index we send in authentication request */
    T_SERVICE_ITEM *item;

    item = os_queue_peek(&rfc_db->service_list, 0);
    while (item != NULL)
    {
        if (item->service_id == ind->uuid)
        {
            item->cback(RFC_AUTHOR_IND, (void *)ind);
            break;
        }

        item = item->next;
    }
}

void rfc_handle_l2c_msg(void        *buf,
                        T_PROTO_MSG  l2c_msg)
{
    switch (l2c_msg)
    {
    case L2C_CONN_IND:
        rfc_handle_l2c_conn_ind((T_MPA_L2C_CONN_IND *)buf);
        break;

    case L2C_CONN_RSP:
        rfc_handle_l2c_conn_rsp((T_MPA_L2C_CONN_RSP *)buf);
        break;

    case L2C_CONN_CMPL:
        rfc_handle_l2c_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)buf);
        break;

    case L2C_DATA_IND:
        rfc_handle_l2c_data_ind((T_MPA_L2C_DATA_IND *)buf);
        break;

    case L2C_DATA_RSP:
        rfc_handle_l2c_data_rsp((T_MPA_L2C_DATA_RSP *)buf);
        break;

    case L2C_DISCONN_IND:
        rfc_handle_l2c_disconn_ind((T_MPA_L2C_DISCONN_IND *)buf);
        break;

    case L2C_DISCONN_RSP:
        rfc_handle_l2c_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)buf);
        break;

    case L2C_SEC_REG_RSP:
        rfc_handle_sec_reg_rsp((T_MPA_L2C_SEC_REG_RSP *)buf);
        break;

    case L2C_PROTO_AUTHOR_IND:
        rfc_handle_author_ind((T_MPA_AUTHOR_REQ_IND *)buf);
        break;

    default:
        break;
    }
}

void rfc_handle_authen_rsp(T_MPA_RFC_AUTHEN_RSP *rsp)
{
    T_RFC_CHANN *chann;

    RFCOMM_PRINT_INFO3("rfc_handle_authen_rsp: dlci 0x%02x, cid 0x%04x, cause 0x%04x",
                       rsp->dlci, rsp->cid, rsp->cause);

    chann = rfc_find_chann_by_dlci(rsp->dlci, rsp->cid);
    if (chann != NULL)
    {
        if (rsp->outgoing)
        {
            if (rsp->cause)
            {
                /* outgoing connection rejected by security manager */
                chann->state = DLCI_IDLE;
                rfc_check_disconn_ctrl_chann(chann->ctrl_chann);
                rfc_free_chann(chann, RFC_ERR | RFC_ERR_REJECT_SECURITY);
                return;
            }

            if (chann->state == DLCI_OPEN)
            {
                uint8_t pn[RFC_SIZE_PN];
                rfc_encode_pn(chann, pn);
                chann->state = DLCI_CONFIG_OUTGOING;
                sys_timer_start(chann->wait_rsp_timer_handle);
                rfc_send_uih(chann->ctrl_chann, RFC_TYPE_PN, 1, pn, sizeof(pn));
            }
        }
        else
        {
            if (rsp->cause)
            {
                /* incoming connection rejected by security manager */
                /* do not answer with DM, but answer with PN because some Microsoft implementations have problems
                with TYPE_PN / DM exchange and seem to prefer TYPE_PN / TYPE_PN followed  by SABM / DM.
                The channel descriptor for this DLCI is deallocated, so the subsequent SABM will occur on an
                unconfigured DLCI, therefore being rejected.
                */
                uint8_t pn[RFC_SIZE_PN];

                rfc_encode_pn(chann, pn);    /* answer with unchanged configuration */
                rfc_send_uih(chann->ctrl_chann, RFC_TYPE_PN, 0, pn, sizeof(pn));
                rfc_free_chann(chann, RFC_ERR | RFC_ERR_REJECT_SECURITY);
            }
            else
            {
                if (chann->state == DLCI_CONFIG_INCOMING)
                {
                    T_RFC_CONN_IND conn_ind;

                    conn_ind.frame_size = chann->frame_size;
                    conn_ind.dlci = chann->dlci;
                    memcpy(conn_ind.bd_addr, chann->ctrl_chann->bd_addr, 6);
                    rfc_msg_post(RFC_CONN_IND, chann->service, (void *)&conn_ind);
                }
            }
        }
    }
}

bool rfc_init(void)
{
    int32_t ret = 0;

    rfc_db = calloc(1, sizeof(T_RFC));
    if (rfc_db == NULL)
    {
        ret = 1;
        goto fail_alloc_rfc;
    }

    rfc_db->ds_offset = 24;
    rfc_db->enable_ertm = false;

    if (mpa_reg_l2c_proto(PSM_RFCOMM, rfc_handle_l2c_msg, &rfc_db->queue_id) == false)
    {
        ret = 2;
        goto fail_reg_l2c;
    }

    mpa_reg_rfc_authen_cb(rfc_handle_authen_rsp);

    return true;

fail_reg_l2c:
    free(rfc_db);
    rfc_db = NULL;
fail_alloc_rfc:
    RFCOMM_PRINT_ERROR1("rfc_init: failed %d", -ret);
    return false;
}

bool rfc_cback_register(uint8_t           server_chann,
                        P_RFC_SERVICE_CB  cback,
                        uint8_t          *service_id)
{
    T_SERVICE_ITEM *item;
    uint8_t         index;
    int32_t         ret;

    if (rfc_db == NULL)
    {
        if (!rfc_init())
        {
            ret = 1;
            goto fail_init_rfc;
        }
    }

    if (cback == NULL)
    {
        ret = 2;
        goto fail_invalid_cback;
    }

    item = os_queue_peek(&rfc_db->service_list, 0);
    while (item != NULL)
    {
        if (item->local_server_chann == server_chann)
        {
            ret = 3;
            goto fail_server_chann;
        }

        item = item->next;
    }

    for (index = 0; index < rfc_db->service_list.count; index++)
    {
        bool index_used = false;

        item = os_queue_peek(&rfc_db->service_list, 0);
        while (item != NULL)
        {
            if (item->service_id == index)
            {
                index_used = true;
                break;
            }

            item = item->next;
        }

        if (index_used == false)
        {
            break;
        }
    }

    item = malloc(sizeof(T_SERVICE_ITEM));
    if (item == NULL)
    {
        ret = 4;
        goto fail_alloc_item;
    }

    item->cback = cback;
    item->service_id = index;
    item->local_server_chann = server_chann;
    os_queue_in(&rfc_db->service_list, item);
    *service_id = index;
    return true;

fail_alloc_item:
fail_server_chann:
fail_invalid_cback:
fail_init_rfc:
    RFCOMM_PRINT_ERROR1("rfc_cback_register: failed %d", -ret);
    return false;
}

bool rfc_reg_sec(uint8_t service_id,
                 uint8_t server_chann,
                 uint8_t active,
                 uint8_t sec)
{
    T_SERVICE_ITEM *item;

    item = os_queue_peek(&rfc_db->service_list, 0);
    while (item != NULL)
    {
        if (item->service_id == service_id &&
            item->local_server_chann == server_chann)
        {
            mpa_send_l2c_sec_reg_req(active, PSM_RFCOMM, server_chann, item->service_id, sec);
            return true;
        }

        item = item->next;
    }

    return false;
}

bool rfc_set_ertm_mode(bool enable)
{
    if (rfc_db != NULL)
    {
        rfc_db->enable_ertm = enable;
        return true;
    }

    return false;
}

bool rfc_conn_req(uint8_t   bd_addr[6],
                  uint8_t   server_chann,
                  uint8_t   service_id,
                  uint16_t  frame_size,
                  uint8_t   max_credits,
                  uint8_t  *dlci)
{
    uint8_t         mode = MPA_L2C_MODE_BASIC;
    bool            new_control = false;
    T_RFC_CHANN    *data_chann;
    T_RFC_CHANN    *ctrl_chann;
    T_SERVICE_ITEM *item;
    int32_t         ret = 0;

    if (frame_size == 0)
    {
        frame_size = RFC_DEFAULT_MTU;
    }

    *dlci = server_chann << 1;

    ctrl_chann = rfc_find_chann_by_addr(bd_addr, 0);
    if (!ctrl_chann)
    {
        ctrl_chann = rfc_alloc_chann(0);
        if (!ctrl_chann)
        {
            ret = 1;
            goto fail_no_ctrl_chann;
        }

        ctrl_chann->ctrl_chann = ctrl_chann;
        memcpy(ctrl_chann->bd_addr, bd_addr, 6);
        new_control = true;
    }

    if ((ctrl_chann->state == DLCI_IDLE) || ctrl_chann->initiator)
    {
        /* we are / will be initiator of the connection: our direction bit is 1 */
        /* the direction bit of the remote side is "0" */
    }
    else
    {
        *dlci |= 1;
    }

    /* now the DLCI also contains the direction bit */
    data_chann = rfc_find_chann_by_addr(bd_addr, *dlci);
    if (data_chann)
    {
        if (new_control)
        {
            rfc_free_chann(ctrl_chann, RFC_ERR_INVALID_STATE);
        }
        ret = 2;
        goto fail_exist_data_chann;
    }

    data_chann = rfc_alloc_chann(*dlci);
    if (!data_chann)
    {
        if (new_control)
        {
            rfc_free_chann(ctrl_chann, RFC_ERR_INVALID_STATE);
        }
        ret = 3;
        goto fail_no_data_chann;
    }

    item = os_queue_peek(&rfc_db->service_list, 0);
    while (item != NULL)
    {
        if (item->service_id == service_id)
        {
            break;
        }

        item = item->next;
    }

    rfc_init_credits(data_chann, max_credits);
    data_chann->ctrl_chann  = ctrl_chann;
    data_chann->service = item;
    data_chann->state = DLCI_OPEN;
    data_chann->frame_size = frame_size;

    RFCOMM_PRINT_INFO6("rfc_conn_req: bd_addr %s, server channel 0x%02x, frame size %d, max credits %d, link state %d, ctrl chann state %d",
                       TRACE_BDADDR(bd_addr), server_chann, frame_size, max_credits, ctrl_chann->link_state,
                       ctrl_chann->state);

    switch (ctrl_chann->link_state)
    {
    case LINK_IDLE:
        if (rfc_db->enable_ertm)
        {
            mode |= MPA_L2C_MODE_ERTM;
        }
        mpa_send_l2c_conn_req(PSM_RFCOMM, UUID_RFCOMM, rfc_db->queue_id, RFC_MTU_SIZE, bd_addr, mode,
                              0xFFFF);
        ctrl_chann->link_initiator = true;
        ctrl_chann->service = item;
        ctrl_chann->link_state = LINK_CONNECTING;
        break;

    case LINK_CONNECTED:
        switch (ctrl_chann->state)
        {
        case DLCI_IDLE:
            ctrl_chann->initiator = true;
            ctrl_chann->state = DLCI_CONNECTING;
            sys_timer_start(ctrl_chann->wait_rsp_timer_handle);
            rfc_send_sabm(ctrl_chann, 1, 1);
            break;

        case DLCI_CONNECTED:
            /* check if frameSize setting for this channel is acceptable according to L2CAP mtuSize */
            if ((data_chann->frame_size + RFC_RSVD_SIZE) > data_chann->ctrl_chann->mtu_size)
            {
                data_chann->frame_size = data_chann->ctrl_chann->mtu_size - RFC_RSVD_SIZE;
            }
            rfc_send_authen_req(data_chann, 1);
            break;

        case DLCI_DISCONNECTING:
            ret = 4;
            goto fail_wrong_state;

        default:
            break;
        }
        break;

    case LINK_DISCONNECT_REQ:
        ctrl_chann->link_state = LINK_CONNECTING;
        break;

    case LINK_DISCONNECTING:
        sys_timer_delete(data_chann->wait_rsp_timer_handle);
        sys_timer_delete(data_chann->wait_msc_timer_handle);
        os_queue_delete(&rfc_db->chann_list, data_chann);
        free(data_chann);
        ret = 5;
        goto fail_wrong_state;

    default:
        break;
    }

    return true;

fail_wrong_state:
fail_no_data_chann:
fail_exist_data_chann:
fail_no_ctrl_chann:
    RFCOMM_PRINT_ERROR1("rfc_conn_req: failed %d", -ret);
    return false;
}

void rfc_conn_cfm(uint8_t  bd_addr[6],
                  uint8_t  dlci,
                  uint16_t status,
                  uint16_t frame_size,
                  uint8_t  max_credits)
{
    T_RFC_CHANN *chann;

    chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (chann != NULL)
    {
        RFCOMM_PRINT_INFO6("rfc_conn_cfm: bd_addr %s, dlci 0x%02x, status 0x%04x, max credits %d, frame size %d, state 0x%02x",
                           TRACE_BDADDR(bd_addr), dlci, status, max_credits, frame_size, chann->state);

        if (status)   /* this is a reject !*/
        {
            /* do not answer with DM, but answer with PN because some Microsoft implementations have problems
            with TYPE_PN / DM exchange and seem to prefer TYPE_PN / TYPE_PN followed  by SABM / DM.
            The channel descriptor for this DLCI is deallocated, so the subsequent SABM will occur on an
            unconfigured DLCI, therefore being rejected.
            */
            uint8_t pn[RFC_SIZE_PN];

            rfc_encode_pn(chann, pn);    /* answer with unchanged configuration */
            rfc_send_uih(chann->ctrl_chann, RFC_TYPE_PN, 0, pn, sizeof(pn));
            rfc_free_chann(chann, RFC_SUCCESS);
        }
        else
        {
            if (chann->ctrl_chann->link_state != LINK_DISCONNECT_REQ)
            {
                if (chann->state == DLCI_CONFIG_INCOMING)
                {
                    /* opening a channel where we received a PN command for channel configuration */
                    uint8_t pn[RFC_SIZE_PN];

                    chann->us_flow_ctrl = 0;
                    chann->frame_size = (frame_size > chann->frame_size) ? chann->frame_size : frame_size;
                    rfc_init_credits(chann, max_credits);
                    chann->state = DLCI_CONFIG_ACCEPTED;
                    rfc_encode_pn(chann, pn);    /* answer with (changed) configuration */
                    rfc_send_uih(chann->ctrl_chann, RFC_TYPE_PN, 0, pn, sizeof(pn));
                }
            }
        }
    }
}

bool rfc_disconn_req(uint8_t bd_addr[6],
                     uint8_t dlci)
{
    T_RFC_CHANN *chann;
    int32_t      ret;

    chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (!chann)
    {
        ret = 1;
        goto fail_invalid_param;
    }

    RFCOMM_PRINT_INFO4("rfc_disconn_req: bd_addr %s, dlci 0x%02x, link state 0x%02x, chann state 0x%02x",
                       TRACE_BDADDR(bd_addr), dlci, chann->ctrl_chann->link_state, chann->state);

    if (chann->ctrl_chann->link_state == LINK_CONNECTING &&
        chann->ctrl_chann->l2c_cid == 0)
    {
        chann->ctrl_chann->link_state = LINK_DISCONNECT_REQ;
        return true;
    }

    switch (chann->state)
    {
    case DLCI_IDLE:
    case DLCI_DISCONNECTING:
        ret = 2;
        goto fail_wrong_state;

    case DLCI_OPEN:         //SABM not send
        chann->state = DLCI_IDLE;
        rfc_check_disconn_ctrl_chann(chann->ctrl_chann);
        rfc_free_chann(chann, RFC_SUCCESS);
        break;

    case DLCI_CONNECTING:   //SABM send, UA not received
        chann->state = DLCI_DISCONNECT_PENDING;
        break;

    default:
        chann->state = DLCI_DISCONNECTING;
        sys_timer_start(chann->wait_rsp_timer_handle);
        rfc_send_disc(chann, 1, 1);
        rfc_check_send_credits();
        break;
    }

    return true;

fail_wrong_state:
fail_invalid_param:
    RFCOMM_PRINT_ERROR1("rfc_disconn_req: failed %d", -ret);
    return false;
}

bool rfc_flow_ctrl_req(uint8_t bd_addr[6],
                       uint8_t dlci,
                       uint8_t flow_status,
                       uint8_t sbreak)
{
    T_RFC_CHANN *chann;

    chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (!chann || chann->state != DLCI_CONNECTED)
    {
        return false;
    }

    /* store new flow state */
    chann->us_flow_ctrl = flow_status;
    chann->us_flow_break = sbreak;

    if (chann->dlci == 0)
    {
        /* Flow Control Command from App Layer on Control Channel, this is aggregate flow control */
        sys_timer_start(chann->wait_rsp_timer_handle);
        rfc_send_uih(chann, (chann->us_flow_ctrl & RFC_FLOW_BIT) == 0 ? RFC_TYPE_FCON :
                     RFC_TYPE_FCOFF, 1, NULL, 0);
    }
    else
    {
        /* generateMSC_CONF locally and immediate */
        /* Flow Control Command from App Layer on User Channel, send Modem Status Command to Peer */
        if (chann->us_flow_active & RFC_MSC_CMD_OUTGOING)
        {
            /* there is an outstanding MSC on this link: do not send now, but only store parameter values (already done)
               and remember the fact in bit 1*/
            chann->us_flow_active |= RFC_MSC_CMD_PENDING;
        }
        else
        {
            /* there is currently no outstanding MSC, send the MSC request and indicate MSC outstanding */
            chann->us_flow_active |= RFC_MSC_CMD_OUTGOING;
            sys_timer_start(chann->ctrl_chann->wait_rsp_timer_handle);
            rfc_send_msc(chann->ctrl_chann, 1, chann->dlci, chann->us_flow_ctrl, sbreak);
        }
    }

    return true;
}

bool rfc_data_req(uint8_t   bd_addr[6],
                  uint8_t   dlci,
                  uint8_t  *data,
                  uint16_t  len,
                  bool      ack)
{
    T_RFC_CHANN *chann;
    uint8_t      header_size = 0;
    bool         credit_field = false;
    uint8_t     *buf;
    uint8_t     *p;
    int32_t      ret = 0;

    chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (!chann)
    {
        ret = 1;
        goto fail_no_chann;
    }

    if ((chann->state != DLCI_CONNECTED) || rfc_check_block(chann))
    {
        ret = 2;
        goto fail_wrong_state;
    }

    if ((len > chann->frame_size) || (len == 0))
    {
        ret = 3;
        goto fail_wrong_len;
    }

    if (len > 127)
    {
        header_size = 4;      /* 1 byte addr, 1 byte control, 2 bytes length*/
    }
    else
    {
        header_size = 3;      /* 1 byte addr, 1 byte control, 1 byte length*/
    }

    if ((chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM) && chann->given_credits)
    {
        credit_field = true;
        header_size += 1;
    }

    buf = mpa_get_l2c_buf(rfc_db->queue_id, chann->ctrl_chann->l2c_cid, chann->dlci,
                          len + header_size + RFC_CRC_FIELD_SIZE, rfc_db->ds_offset, ack);
    if (buf == NULL)
    {
        ret = 4;
        goto fail_get_buf;
    }

    p = buf + rfc_db->ds_offset;

    /* build the message header */
    if (chann->ctrl_chann->initiator)
    {
        LE_UINT8_TO_STREAM(p, (chann->dlci << 2) | 2 | RFC_EA_BIT);
    }
    else
    {
        LE_UINT8_TO_STREAM(p, (chann->dlci << 2) | 0 | RFC_EA_BIT);
    }

    if (credit_field)
    {
        LE_UINT8_TO_STREAM(p, RFC_UIH | RFC_POLL_BIT);
    }
    else
    {
        LE_UINT8_TO_STREAM(p, RFC_UIH);
    }

    if (len > 127)
    {
        LE_UINT8_TO_STREAM(p, len << 1);
        LE_UINT8_TO_STREAM(p, len >> 7);
    }
    else
    {
        LE_UINT8_TO_STREAM(p, (len << 1) | RFC_EA_BIT);
    }

    if (credit_field)
    {
        LE_UINT8_TO_STREAM(p, chann->given_credits);
        chann->given_credits = 0;
    }

    memcpy(p, data, len);
    p += len;
    LE_UINT8_TO_STREAM(p, crc8_ets_gen(buf + rfc_db->ds_offset, 2));
    mpa_send_l2c_data_req(buf, rfc_db->ds_offset, chann->ctrl_chann->l2c_cid,
                          len + header_size + 1, false);

    if ((chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM) && len)
    {
        /* decrement credits only for non zero packets */
        chann->remote_remain_credits--;
    }

    rfc_check_send_credits();
    return true;

fail_get_buf:
fail_wrong_len:
fail_wrong_state:
fail_no_chann:
    RFCOMM_PRINT_ERROR1("rfc_data_req: failed %d", -ret);
    return false;
}

bool rfc_data_cfm(uint8_t bd_addr[6],
                  uint8_t dlci,
                  uint8_t rsp_num)
{
    T_RFC_CHANN *chann;

    chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (chann != NULL)
    {
        if (chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM &&
            chann->state != DLCI_DISCONNECTING)
        {
            chann->given_credits += rsp_num;
            RFCOMM_PRINT_TRACE2("rfc_data_cfm: dlci 0x%02x, given credits %d",
                                chann->dlci, chann->given_credits);
        }

        rfc_check_send_credits();
        return true;
    }

    return false;
}

bool rfc_get_cid(uint8_t   bd_addr[6],
                 uint8_t   dlci,
                 uint16_t *cid)
{
    T_RFC_CHANN *chann;

    chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (chann != NULL)
    {
        *cid = chann->ctrl_chann->l2c_cid;
        return true;
    }

    return false;
}

bool rfc_ctrl_roleswap_info_get(uint8_t                   bd_addr[6],
                                uint16_t                  cid,
                                T_ROLESWAP_RFC_CTRL_INFO *ctrl)
{
    T_RFC_CHANN *ctrl_chann;

    ctrl_chann = rfc_find_chann_by_dlci(0, cid);
    if (ctrl_chann != NULL)
    {
        ctrl->initiator      = ctrl_chann->initiator;
        ctrl->link_initiator = ctrl_chann->link_initiator;
        ctrl->l2c_cid        = ctrl_chann->l2c_cid;
        ctrl->mtu_size       = ctrl_chann->mtu_size;
        ctrl->data_offset    = rfc_db->ds_offset;

        return true;
    }

    return false;
}

bool rfc_data_roleswap_info_get(uint8_t                   bd_addr[6],
                                uint8_t                   dlci,
                                T_ROLESWAP_RFC_DATA_INFO *data)
{
    T_RFC_CHANN *data_chann;

    data_chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (data_chann != NULL)
    {
        data->dlci                  = data_chann->dlci;
        data->l2c_cid               = data_chann->ctrl_chann->l2c_cid;
        data->frame_size            = data_chann->frame_size;
        data->init_credits          = data_chann->init_credits;
        data->remote_remain_credits = data_chann->remote_remain_credits;
        data->given_credits         = data_chann->given_credits;

        return true;
    }

    return false;
}

bool rfc_ctrl_roleswap_info_set(uint8_t                   bd_addr[6],
                                T_ROLESWAP_RFC_CTRL_INFO *info)
{
    T_RFC_CHANN *ctrl_chann;

    ctrl_chann = rfc_find_chann_by_addr(bd_addr, 0);
    if (ctrl_chann == NULL)
    {
        ctrl_chann = rfc_alloc_chann(0);
    }

    if (ctrl_chann != NULL)
    {
        ctrl_chann->ctrl_chann   = ctrl_chann;
        ctrl_chann->initiator      = info->initiator;
        ctrl_chann->state          = DLCI_CONNECTED;
        ctrl_chann->link_state     = LINK_CONNECTED;
        ctrl_chann->l2c_cid        = info->l2c_cid;
        ctrl_chann->mtu_size       = info->mtu_size;
        ctrl_chann->link_initiator = info->link_initiator;
        ctrl_chann->msc_handshake  = RFC_MSC_CMD_RCV | RFC_MSC_RSP_RCV;
        memcpy(ctrl_chann->bd_addr, bd_addr, 6);
        rfc_db->ds_offset = info->data_offset;

        return true;
    }

    return false;
}

bool rfc_data_roleswap_info_set(uint8_t                   bd_addr[6],
                                uint8_t                   service_id,
                                T_ROLESWAP_RFC_DATA_INFO *info)
{
    T_RFC_CHANN    *ctrl_chann;
    T_RFC_CHANN    *data_chann;
    T_SERVICE_ITEM *item;
    int32_t         ret = 0;

    ctrl_chann = rfc_find_chann_by_addr(bd_addr, 0);
    if (ctrl_chann == NULL)
    {
        ret = 1;
        goto fail_find_chann;
    }

    data_chann = rfc_find_chann_by_dlci(info->dlci, info->l2c_cid);
    if (data_chann == NULL)
    {
        data_chann = rfc_alloc_chann(info->dlci);
    }

    if (data_chann == NULL)
    {
        ret = 2;
        goto fail_alloc_chann;
    }

    item = os_queue_peek(&rfc_db->service_list, 0);
    while (item != NULL)
    {
        if (item->service_id == service_id)
        {
            break;
        }

        item = item->next;
    }

    data_chann->ctrl_chann            = ctrl_chann;
    data_chann->state                 = DLCI_CONNECTED;
    data_chann->convergence_layer     = RFC_CONVERGENCE_CREDIT_CFM;
    data_chann->frame_size            = info->frame_size;
    data_chann->init_credits          = info->init_credits;
    data_chann->remote_remain_credits = info->remote_remain_credits;
    data_chann->given_credits         = info->given_credits;
    data_chann->service               = item;

    return true;

fail_alloc_chann:
fail_find_chann:
    RFCOMM_PRINT_ERROR1("rfc_data_roleswap_info_set: failed %d", -ret);
    return false;
}

bool rfc_ctrl_roleswap_info_del(uint8_t bd_addr[6])
{
    T_RFC_CHANN *ctrl_chann;

    ctrl_chann = rfc_find_chann_by_addr(bd_addr, 0);
    if (ctrl_chann != NULL)
    {
        rfc_free_chann(ctrl_chann, RFC_SUCCESS);
        return true;
    }

    return false;
}

bool rfc_data_roleswap_info_del(uint8_t  dlci,
                                uint16_t cid)
{
    T_RFC_CHANN *data_chann;

    data_chann = rfc_find_chann_by_dlci(dlci, cid);
    if (data_chann != NULL)
    {
        rfc_free_chann(data_chann, RFC_SUCCESS);
        return true;
    }

    return false;
}
