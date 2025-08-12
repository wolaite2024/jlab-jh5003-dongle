/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "os_mem.h"
#include "trace.h"
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

typedef enum
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

typedef enum
{
    LINK_IDLE           = 0x00,
    LINK_CONNECTING     = 0x01,
    LINK_CONNECTED      = 0x02,
    LINK_DISCONNECT_REQ = 0x03,
    LINK_DISCONNECTING  = 0x04,
} T_L2C_LINK_STATE;

/** RFCOMM IF Message Argument Definitions */
typedef struct
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

/** DLCI description structure */
typedef struct RFC_CHANN
{
    bool              used;
    uint8_t           dlci;
    uint8_t           index;
    T_DLCI_STATE      state;

    /** Link Status Data (only for DLCI==0) */
    bool              initiator;
    T_L2C_LINK_STATE  link_state;     /**< state of l2cap link */
    uint16_t          l2c_cid;        /**< l2cap connection id */
    uint16_t          mtu_size;       /**< Max MTU Size for Remote Peer */
    uint8_t           bd_addr[6];     /**< bd_addr of l2cap connection */
    bool              link_initiator; /**< initiator of the link */
    uint8_t           msc_handshake;  /**< bit 0:received msc cmd, bit 1: received msc rsp */

    /** RFCOMM Link Configuration Data (only for DLCI != 0 */
    struct RFC_CHANN *p_ctrl_chann;     /**< Back pointer to control channel for this channel */
    uint8_t           convergence_layer; /**< 0..3 for Convergence Layers 1..4, default 0 */
    uint16_t          frame_size;
    uint8_t           init_credits;
    uint8_t           profile_index;

    /** Credit Based Flow Control */
    uint8_t           remote_remain_credits;   /**< remote device receive ability */
    uint8_t           given_credits;           /**< credits to return to remote side */

    /** MSC Flow Control */
    uint8_t           us_flow_ctrl;     /**< 1: upstream flow is BLOCKED, 0: unblock */
    uint8_t           us_flow_break;    /**< break status byte */
    uint8_t           us_flow_active;   /**< bit 0: one MSC outgoing, Bit 1: open MSC req stored */
    uint8_t           ds_flow_ctrl;     /**< downstream flow is BLOCKED */

    /** Remote Line Status */
    uint8_t           rls;              /**< Remote Line Status */
    T_RPN             rpn;              /**< Parameters from RPN command */

    T_SYS_TIMER_HANDLE  wait_rsp_timer_handle;    /*wait for response */
    T_SYS_TIMER_HANDLE  wait_msc_timer_handle;    /*wait for msc handshake */
} T_RFC_CHANN;

typedef struct
{
    T_RFC_CHANN        *p_chann;

    T_RFC_PROFILE      *p_profile_cb;

    uint8_t             ds_offset;              /**< downstream data offset */
    uint8_t             queue_id;               /**< queue id */
    uint8_t             ds_pool_id;
    uint8_t             rsp_tout;
    uint8_t             msc_tout;
    uint8_t             dlci_num;
    uint8_t             profile_num;
    bool                enable_ertm;
} T_RFC;

static const uint8_t crc8EtsTable[256] =
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

T_RFC *p_rfc;

void rfc_wait_rsp_timeout(T_SYS_TIMER_HANDLE handle);
void rfc_wait_msc_timeout(T_SYS_TIMER_HANDLE handle);

uint8_t crc8EtsGen(uint8_t  *p,
                   uint16_t  len)
{
    uint8_t fcs = 0xff;

    while (len--)
    {
        fcs = crc8EtsTable[fcs ^ *p++];
    }
    fcs = 0xff - fcs;

    return fcs;
}

bool crc8EtsCheck(uint8_t  *p,
                  uint16_t  len,
                  uint8_t   rfcs)
{
    uint8_t fcs = 0xff;

    while (len--)
    {
        fcs = crc8EtsTable[fcs ^ *p++];
    }
    fcs = crc8EtsTable[fcs ^ rfcs];

    if (fcs == 0xcf)
    {
        return true;
    }

    return false;
}

void rfc_msg_to_profile(T_RFC_MSG_TYPE  type,
                        T_RFC_CHANN    *p_chann,
                        void           *p_data)
{
    P_RFC_PROFILE_CB profile_cb = NULL;

    if (p_chann->profile_index < p_rfc->profile_num)
    {
        profile_cb = p_rfc->p_profile_cb[p_chann->profile_index].cb;
    }

    if (profile_cb)
    {
        RFCOMM_PRINT_INFO1("rfc_msg_to_profile: msg type 0x%02x", type);
        profile_cb(type, p_data);
    }
}

T_RFC_CHANN *rfc_find_chann_by_idx(uint8_t index)
{
    T_RFC_CHANN *p_chann = NULL;

    if (index < p_rfc->dlci_num)
    {
        p_chann = &p_rfc->p_chann[index];
    }

    if (p_chann && p_chann->used)
    {
        return p_chann;
    }

    return NULL;
}

T_RFC_CHANN *rfc_find_chann_by_dlci(uint8_t  dlci,
                                    uint16_t cid)
{
    uint8_t i;
    T_RFC_CHANN *p_chann;

    for (i = 0; i < p_rfc->dlci_num; i++)
    {
        p_chann = &p_rfc->p_chann[i];
        if (p_chann->used && p_chann->dlci == dlci && p_chann->p_ctrl_chann->l2c_cid == cid)
        {
            return p_chann;
        }
    }

    return NULL;
}

T_RFC_CHANN *rfc_find_chann_by_addr(uint8_t bd_addr[6],
                                    uint8_t dlci)
{
    uint8_t i;
    T_RFC_CHANN *p_chann;

    for (i = 0; i < p_rfc->dlci_num; i++)
    {
        p_chann = &p_rfc->p_chann[i];
        if (p_chann->used && p_chann->dlci == dlci &&
            !memcmp(p_chann->p_ctrl_chann->bd_addr, bd_addr, 6))
        {
            return p_chann;
        }
    }

    return NULL;
}

void rfc_free_chann(T_RFC_CHANN *p_chann,
                    uint16_t     cause)
{
    if (p_chann->dlci)
    {
        T_RFC_DISCONN_CMPL msg;
        msg.dlci = p_chann->dlci;
        msg.cause = cause;
        memcpy(msg.bd_addr, p_chann->p_ctrl_chann->bd_addr, 6);
        rfc_msg_to_profile(RFC_DISCONN_CMPL, p_chann, (void *)&msg);
    }

    p_chann->used = false;
    sys_timer_delete(p_chann->wait_rsp_timer_handle);
    sys_timer_delete(p_chann->wait_msc_timer_handle);
}

void rfc_send_authen_req(T_RFC_CHANN *p_chann,
                         uint8_t      outgoing)
{
    mpa_send_rfc_authen_req(p_chann->p_ctrl_chann->bd_addr, p_chann->p_ctrl_chann->l2c_cid,
                            p_chann->dlci, p_chann->profile_index, outgoing);
}

T_RFC_CHANN *rfc_alloc_chann(uint8_t dlci)
{
    uint8_t i;
    T_RFC_CHANN *p_chann;

    for (i = 0; i < p_rfc->dlci_num; i++)
    {
        p_chann = &p_rfc->p_chann[i];

        if (p_chann->used == false)
        {
            memset(p_chann, 0, sizeof(T_RFC_CHANN));

            p_chann->wait_rsp_timer_handle = sys_timer_create("rfc_wait_rsp",
                                                              SYS_TIMER_TYPE_LOW_PRECISION,
                                                              (uint32_t)p_chann,
                                                              p_rfc->rsp_tout * 1000000,
                                                              false,
                                                              rfc_wait_rsp_timeout);
            if (p_chann->wait_rsp_timer_handle == NULL)
            {
                return NULL;
            }

            p_chann->wait_msc_timer_handle = sys_timer_create("rfc_wait_msc",
                                                              SYS_TIMER_TYPE_LOW_PRECISION,
                                                              (uint32_t)p_chann,
                                                              p_rfc->msc_tout * 1000000,
                                                              false,
                                                              rfc_wait_msc_timeout);
            if (p_chann->wait_msc_timer_handle == NULL)
            {
                sys_timer_delete(p_chann->wait_rsp_timer_handle);
                return NULL;
            }

            p_chann->used = true;
            p_chann->index = i;
            p_chann->dlci = dlci;
            p_chann->convergence_layer = RFC_CONVERGENCE_CREDIT_REQ;
            p_chann->frame_size = RFC_N1;
            p_chann->profile_index = p_rfc->profile_num;

            p_chann->rpn.dlci     = (dlci << 2) | 2 | RFC_EA_BIT;
            p_chann->rpn.baudrate = 0x07;       /* 115200 Baud */
            p_chann->rpn.oc3      = 0x03;       /* charformat 8N1 */
            p_chann->rpn.flctl    = 0x00;       /* no flow control */
            p_chann->rpn.xon      = 0x11;       /* Xon */
            p_chann->rpn.xoff     = 0x13;       /* Xoff */
            p_chann->rpn.pm1      = 0x00;       /* mask */
            p_chann->rpn.pm2      = 0x00;       /* mask */

            return p_chann;
        }
    }

    return NULL;
}

void rfc_init_credits(T_RFC_CHANN *p_chann,
                      uint16_t     credits)
{
    if (p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_REQ ||
        p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM)
    {
        p_chann->init_credits = credits;

        if (credits > 7)
        {
            p_chann->given_credits = credits - 7;
        }
        else
        {
            p_chann->given_credits = 0;
        }
    }
}

void rfc_check_send_credits(void)
{
    uint8_t     *p_buf;
    uint8_t     *p;
    T_RFC_CHANN *p_chann;
    uint8_t      i;
    uint8_t      offset = p_rfc->ds_offset;

    for (i = 0; i < p_rfc->dlci_num; i++)
    {
        p_chann = &p_rfc->p_chann[i];

        if (p_chann->used && (p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM) &&
            (p_chann->given_credits > (p_chann->init_credits / 2)))
        {
            /* 1 byte addr, 1 byte control, 1 byte length, 1 byte credit , 1 byte crc */;
            p_buf = mpa_get_l2c_buf(p_rfc->queue_id, p_chann->p_ctrl_chann->l2c_cid,
                                    p_chann->dlci, 5, offset, false);
            if (p_buf == NULL)
            {
                RFCOMM_PRINT_ERROR2("rfc_check_send_credits: get buffer fail, dlci 0x%02x, given credits %d",
                                    p_chann->dlci, p_chann->given_credits);
                return;
            }

            p = p_buf + offset;
            if (p_chann->p_ctrl_chann->initiator)
            {
                LE_UINT8_TO_STREAM(p, (p_chann->dlci << 2) | 2 | RFC_EA_BIT);
            }
            else
            {
                LE_UINT8_TO_STREAM(p, (p_chann->dlci << 2) | 0 | RFC_EA_BIT);
            }

            LE_UINT8_TO_STREAM(p, RFC_UIH | RFC_POLL_BIT);
            LE_UINT8_TO_STREAM(p, RFC_EA_BIT);
            LE_UINT8_TO_STREAM(p, p_chann->given_credits);
            p_chann->given_credits = 0;

            LE_UINT8_TO_STREAM(p, crc8EtsGen(p_buf + offset, 2));

            mpa_send_l2c_data_req(p_buf, offset, p_chann->p_ctrl_chann->l2c_cid, 5, false);
        }
    }
}

void rfc_send_frame(T_RFC_CHANN *p_ctrl_chann,
                    uint8_t      dlci,
                    uint8_t      ctl,
                    uint8_t      cr,
                    uint8_t      poll,
                    uint8_t     *p_data,
                    uint16_t     len)
{
    uint8_t *p_buf;
    uint8_t *p;
    uint8_t  header_size;
    uint16_t length;
    uint8_t  offset = p_rfc->ds_offset;

    if (p_ctrl_chann->link_state != LINK_CONNECTED)
    {
        RFCOMM_PRINT_ERROR1("rfc_send_frame: invalid link state 0x%02x", p_ctrl_chann->link_state);
        return;
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

    p_buf = mpa_get_l2c_buf(p_rfc->queue_id, p_ctrl_chann->l2c_cid, dlci,
                            length, offset, false);
    if (p_buf == NULL)
    {
        RFCOMM_PRINT_ERROR2("rfc_send_frame: get buffer fail, dlci 0x%02x, length %d", dlci, length);
        return;
    }
    p = p_buf + offset;

    /* |EA(1bit) | C/R(1bit) | DLCI(6bit)| */
    *p = (dlci << 2) | RFC_EA_BIT;
    if ((cr && p_ctrl_chann->initiator) || (!cr && !p_ctrl_chann->initiator))
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
        memcpy(p, p_data, len);
        p += len;
    }

    /* for UIH, check only first 2 bytes */
    *p++ = crc8EtsGen(p_buf + offset, (uint16_t)(ctl == RFC_UIH ? 2 : 3));

    mpa_send_l2c_data_req(p_buf, offset, p_ctrl_chann->l2c_cid, length, false);
}

void rfc_send_sabm(T_RFC_CHANN *p_chann,
                   uint8_t      cmd,
                   uint8_t      poll)
{
    rfc_send_frame(p_chann->p_ctrl_chann, p_chann->dlci, RFC_SABM, cmd, poll, NULL, 0);
}

void rfc_send_ua(T_RFC_CHANN *p_chann,
                 uint8_t      cmd,
                 uint8_t      poll)
{
    rfc_send_frame(p_chann->p_ctrl_chann, p_chann->dlci, RFC_UA, cmd, poll, NULL, 0);
}

void rfc_send_dm(T_RFC_CHANN *p_ctrl_chann,
                 uint8_t      dlci,
                 uint8_t      cmd,
                 uint8_t      poll)
{
    rfc_send_frame(p_ctrl_chann, dlci, RFC_DM, cmd, poll, NULL, 0);
}

void rfc_send_disc(T_RFC_CHANN *p_chann,
                   uint8_t      cmd,
                   uint8_t      poll)
{
    rfc_send_frame(p_chann->p_ctrl_chann, p_chann->dlci, RFC_DISC, cmd, poll, NULL, 0);
}

void rfc_send_uih(T_RFC_CHANN *p_chann,
                  uint8_t      type,
                  uint8_t      cr,
                  uint8_t     *p,
                  uint16_t     len)
{
    uint8_t buf[32];  /* not especially elegant */
    buf[0] = (type << 2) | (cr << 1) | RFC_EA_BIT;
    buf[1] = (len << 1) + RFC_EA_BIT;

    /* limit the length (safety) */
    if (len > sizeof(buf) - 2)
    {
        len = sizeof(buf) - 2;
    }

    if (len)
    {
        memcpy(buf + 2, p, len);
    }

    rfc_send_frame(p_chann->p_ctrl_chann, p_chann->dlci, RFC_UIH, 1, 0, buf, len + 2);
}

void rfc_send_msc(T_RFC_CHANN *p_chann,
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

    rfc_send_uih(p_chann, RFC_TYPE_MSC, cr, buf, len);
}

void rfc_send_rls(T_RFC_CHANN *p_chann,
                  uint8_t      cr,
                  uint8_t      dlci,
                  uint8_t      status)
{
    uint8_t buf[2];

    buf[0] = (dlci << 2) | 2 | RFC_EA_BIT;
    buf[1] = status | RFC_EA_BIT;

    rfc_send_uih(p_chann, RFC_TYPE_RLS, cr, buf, sizeof(buf));
}

void rfc_close_data_chann(T_RFC_CHANN *p_ctrl_chann,
                          uint16_t     cause)
{
    T_RFC_CHANN *p_chann;
    uint8_t      i;

    for (i = 0; i < p_rfc->dlci_num; i++)
    {
        p_chann = &p_rfc->p_chann[i];

        if (p_chann->used && (p_chann->p_ctrl_chann == p_ctrl_chann) && p_chann->dlci)
        {
            rfc_free_chann(p_chann, cause);
        }
    }

    rfc_check_send_credits();
}

void rfc_check_disconn_ctrl_chann(T_RFC_CHANN *p_ctrl_chann)
{
    T_RFC_CHANN *p_chann;
    uint8_t      i;
    uint8_t      count = 0;

    for (i = 0; i < p_rfc->dlci_num; i++)
    {
        p_chann = &p_rfc->p_chann[i];
        if (p_chann->used && (p_chann->p_ctrl_chann == p_ctrl_chann) &&
            (p_chann != p_ctrl_chann) && (p_chann->state != DLCI_IDLE))
        {
            count++;
        }
    }

    if (count == 0)
    {
        switch (p_ctrl_chann->state)
        {
        case DLCI_IDLE: //link not establed
            if (p_ctrl_chann->link_state == LINK_CONNECTING && p_ctrl_chann->l2c_cid == 0)
            {
                p_ctrl_chann->link_state = LINK_DISCONNECT_REQ;
            }
            else
            {
                mpa_send_l2c_disconn_req(p_ctrl_chann->l2c_cid);
                p_ctrl_chann->link_state = LINK_DISCONNECTING;
            }
            break;

        case DLCI_CONNECTING: //SABM send, no UA received
            p_ctrl_chann->state = DLCI_DISCONNECT_PENDING;
            break;

        case DLCI_CONNECTED:
            p_ctrl_chann->state = DLCI_DISCONNECTING;
            sys_timer_start(p_ctrl_chann->wait_rsp_timer_handle);
            rfc_send_disc(p_ctrl_chann, 1, 1);
            break;

        default:
            break;
        }
    }
}

void rfc_wait_rsp_timeout(T_SYS_TIMER_HANDLE handle)
{
    T_RFC_CHANN *p_chann;

    p_chann = (void *)sys_timer_id_get(handle);
    if (p_chann)
    {
        RFCOMM_PRINT_INFO2("rfc_wait_rsp_timeout: chann %p, dlci %x", p_chann, p_chann->dlci);
        p_chann->state = DLCI_IDLE;

        if (p_chann->dlci)
        {
            rfc_check_disconn_ctrl_chann(p_chann->p_ctrl_chann);
        }
        else
        {
            rfc_close_data_chann(p_chann, RFC_ERR | RFC_ERR_TIMEOUT);
            mpa_send_l2c_disconn_req(p_chann->l2c_cid);
            p_chann->link_state = LINK_DISCONNECTING;
        }
        rfc_free_chann(p_chann, RFC_ERR | RFC_ERR_TIMEOUT);
    }
}

void rfc_wait_msc_timeout(T_SYS_TIMER_HANDLE handle)
{
    T_RFC_CHANN *p_chann;
    T_RFC_CONN_CMPL msg;

    p_chann = (void *)sys_timer_id_get(handle);
    if (p_chann)
    {
        RFCOMM_PRINT_INFO2("rfc_wait_msc_timeout: chann %p, dlci %x", p_chann, p_chann->dlci);

        p_chann->msc_handshake = RFC_MSC_CMD_RCV | RFC_MSC_RSP_RCV;

        msg.dlci = p_chann->dlci;
        msg.remain_credits = p_chann->remote_remain_credits;
        msg.profile_index = p_chann->profile_index;
        msg.frame_size = p_chann->frame_size;
        memcpy(msg.bd_addr, p_chann->p_ctrl_chann->bd_addr, 6);
        rfc_msg_to_profile(RFC_CONN_CMPL, p_chann, (void *)&msg);
    }
}

void rfc_decode_pn(T_RFC_CHANN *p_chann,
                   uint8_t     *buf,
                   uint16_t     len)
{
    if (len != RFC_SIZE_PN)
    {
        return;
    }

    p_chann->dlci = buf[0] & 0x3f;
    p_chann->convergence_layer = (buf[1] >> 4) & 0x0f;
    p_chann->frame_size = (uint16_t)buf[4] + ((uint16_t)buf[5] << 8);

    if ((p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_REQ) ||
        (p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM))
    {
        p_chann->remote_remain_credits = buf[7] & 0x07;
    }

    RFCOMM_PRINT_TRACE2("rfc_decode_pn: dlci 0x%02x, remote init credits %d",
                        p_chann->dlci, p_chann->remote_remain_credits);
}

void rfc_encode_pn(T_RFC_CHANN *p_chann,
                   uint8_t     *buf)
{
    buf[0] = p_chann->dlci;
    buf[1] = RFC_INFO_UIH + (p_chann->convergence_layer << 4);
    buf[2] = 0;
    buf[3] = RFC_T1;
    buf[4] = (uint8_t)(p_chann->frame_size & 0xff);
    buf[5] = (uint8_t)(p_chann->frame_size >> 8);
    buf[6] = RFC_N2;
    buf[7] = p_chann->init_credits > 7 ? 7 : p_chann->init_credits;
}

bool rfc_check_block(T_RFC_CHANN *p_chann)
{
    if (!p_chann->used)
    {
        return false;     /* closed channels are always free... */
    }

    if (p_chann->p_ctrl_chann->ds_flow_ctrl & RFC_FLOW_BIT)
    {
        return true;      /* aggregate flow control (on mux channel) stop all channels */
    }

    if (p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM)
    {
        if (p_chann->remote_remain_credits == 0)
        {
            RFCOMM_PRINT_TRACE1("rfc_check_block: dlci 0x%02x credits is 0", p_chann->dlci);
            return true;
        }

        return false;
    }

    if (p_chann->ds_flow_ctrl & RFC_FLOW_BIT)
    {
        return true;      /* channel specific flow control */
    }

    return false;
}

T_RFC_CHANN *rfc_check_l2c_collision(T_RFC_CHANN *p_ctrl_chann)
{
    uint8_t i = 0;
    T_RFC_CHANN *p_new_ctrl_chann = NULL;

    for (i = 0; i < p_rfc->dlci_num; i++)
    {
        p_new_ctrl_chann = &p_rfc->p_chann[i];
        if (p_new_ctrl_chann->used && p_new_ctrl_chann->dlci == 0 &&
            !memcmp(p_new_ctrl_chann->bd_addr, p_ctrl_chann->bd_addr, 6) &&
            p_new_ctrl_chann->l2c_cid != p_ctrl_chann->l2c_cid)
        {
            /* found another ctrl chann for the same addr */
            RFCOMM_PRINT_INFO5("rfc_check_l2c_collision: found collision ctrl chann with l2c cid 0x%0x state %d link init %d, fail cid 0x%x link init %d",
                               p_new_ctrl_chann->l2c_cid, p_new_ctrl_chann->state, p_new_ctrl_chann->link_initiator,
                               p_ctrl_chann->l2c_cid, p_ctrl_chann->link_initiator);
            return p_new_ctrl_chann;
        }
    }

    return NULL;
}

void rfc_change_ctrl_chann(T_RFC_CHANN *p_ctrl_chann,
                           T_RFC_CHANN *p_new_ctrl_chann)
{
    uint8_t i;
    T_RFC_CHANN *p_chann;

    /* change all data channel allocated for the ctrl chann to the new one */
    for (i = 0; i < p_rfc->dlci_num; i++)
    {
        p_chann = &p_rfc->p_chann[i];

        if (p_chann->used && (p_chann->p_ctrl_chann == p_ctrl_chann) && p_chann->dlci)
        {
            RFCOMM_PRINT_INFO2("rfc_change_ctrl_chann: change ctrl chann for chann with dlci 0x%x, state %d",
                               p_chann->dlci, p_chann->state);

            p_chann->p_ctrl_chann = p_new_ctrl_chann;

            // change direction bit if needed
            if (p_ctrl_chann->link_initiator != p_new_ctrl_chann->link_initiator)
            {
                T_RFC_DLCI_CHANGE_INFO info;

                info.pre_dlci = p_chann->dlci;

                p_chann->dlci ^= 0x01;

                info.curr_dlci = p_chann->dlci;
                memcpy(info.bd_addr, p_new_ctrl_chann->bd_addr, 6);
                rfc_msg_to_profile(RFC_DLCI_CHANGE, p_chann, (void *)&info);
            }

            if (p_chann->state == DLCI_OPEN && p_new_ctrl_chann->state == DLCI_CONNECTED)
            {
                /* check if frameSize setting for this channel is acceptable according to L2CAP mtuSize */
                if ((p_chann->frame_size + RFC_RSVD_SIZE) > p_chann->p_ctrl_chann->mtu_size)
                {
                    p_chann->frame_size = p_chann->p_ctrl_chann->mtu_size - RFC_RSVD_SIZE;
                }

                rfc_send_authen_req(p_chann, 1);
            }
        }
    }
}

void rfc_handle_pn(T_RFC_CHANN *p_ctrl_chann,
                   uint8_t      tcr,
                   uint8_t     *p,
                   uint16_t     len)
{
    uint8_t dlci;
    T_RFC_CHANN *p_chann;

    LE_ARRAY_TO_UINT8(dlci, p);

    if (tcr)    /* this is a command */
    {
        bool old_desc = false;

        p_chann = rfc_find_chann_by_dlci(dlci, p_ctrl_chann->l2c_cid);
        if (p_chann)
        {
            old_desc = true;
        }
        else
        {
            uint8_t index = 0;

            /* find profile callback by server channel number */
            for (index = 0; index < p_rfc->profile_num; index++)
            {
                if (p_rfc->p_profile_cb[index].server_chann == ((dlci >> 1) & 0x1F))
                {
                    break;
                }
            }

            if (index == p_rfc->profile_num)
            {
                /* the server channel is not registered, send an unsolicited DM without poll bit */
                rfc_send_dm(p_ctrl_chann, dlci, 0, 0);
                return;
            }

            p_chann = rfc_alloc_chann(dlci);
            if (!p_chann)
            {
                /* could not allocate descriptor, send an unsolicited DM without poll bit */
                rfc_send_dm(p_ctrl_chann, dlci, 0, 0);
                return;
            }

            p_chann->p_ctrl_chann = p_ctrl_chann;
            p_chann->profile_index = index;
        }

        RFCOMM_PRINT_INFO2("rfc_handle_pn: receive PN cmd for dlci 0x%02x on state 0x%02x",
                           p_chann->dlci, p_chann->state);

        /* check if the link is already open, if the link is open, do reject any changes to its configuration  */
        if (((p_chann->state != DLCI_CONNECTED) || old_desc) && (p_chann->state != DLCI_CONFIG_ACCEPTED))
        {
            rfc_decode_pn(p_chann, p, len);

            /* check if credit based flow control is requested */
            if (p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_REQ)
            {
                p_chann->convergence_layer = RFC_CONVERGENCE_CREDIT_CFM;
            }

            /* check if frameSize setting for this channel is acceptable according to L2CAP mtu_size */
            if ((p_chann->frame_size + RFC_RSVD_SIZE) > p_chann->p_ctrl_chann->mtu_size)
            {
                p_chann->frame_size = p_chann->p_ctrl_chann->mtu_size - RFC_RSVD_SIZE;
            }

            p_chann->state = DLCI_CONFIG_INCOMING;
            rfc_send_authen_req(p_chann, 0);
        }
        else
        {
            uint8_t pn[RFC_SIZE_PN];
            rfc_encode_pn(p_chann, pn);  /* answer with unchanged configuration */
            rfc_send_uih(p_ctrl_chann, RFC_TYPE_PN, 0, pn, sizeof(pn));
        }
    }
    else            /* this is a response */
    {
        uint16_t frame_size;

        p_chann = rfc_find_chann_by_dlci(dlci, p_ctrl_chann->l2c_cid);

        RFCOMM_PRINT_INFO3("rfc_handle_pn: reponse for dlci 0x%02x, cid 0x%04x, channel %p",
                           dlci, p_ctrl_chann->l2c_cid, p_chann);

        if (!p_chann)
        {
            return;
        }

        if (p_chann->state == DLCI_CONNECTED)
        {
            /* response on an open channel, a reconfig-request, this is only supported for UPF4, ignore responses */
            return;
        }

        frame_size = p_chann->frame_size;

        rfc_decode_pn(p_chann, p, len);

        /* peer increased framesize in response, use request value (better: disconnect link) */
        if (p_chann->frame_size > frame_size)
        {
            p_chann->frame_size = frame_size;
        }

        /* recheck the configuration that was sent by remote entity */
        if (p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_REQ)
        {
            /* if the request is still set, then remote side does not understand credit based flow control (rfcomm p.15) */
            p_chann->convergence_layer = RFC_CONVERGENCE_0;
        }

        p_chann->state = DLCI_CONNECTING;

        /* start SABM timer */
        sys_timer_start(p_chann->wait_rsp_timer_handle);
        rfc_send_sabm(p_chann, 1, 1);
    }
}

void rfc_handle_msc(T_RFC_CHANN *p_ctrl_chann,
                    uint8_t      tcr,
                    uint8_t     *p)
{
    uint8_t dlci;
    uint8_t status;
    uint8_t msc_status;
    T_RFC_CHANN *p_chann;

    LE_STREAM_TO_UINT8(dlci, p);
    LE_STREAM_TO_UINT8(status, p);

    dlci = dlci >> 2;
    p_chann = rfc_find_chann_by_dlci(dlci, p_ctrl_chann->l2c_cid);

    RFCOMM_PRINT_INFO4("rfc_handle_msc: dlci 0x%02x, cr %d, cid 0x%04x, channel %p",
                       dlci, tcr, p_ctrl_chann->l2c_cid, p_chann);

    if (!p_chann)
    {
        return;
    }

    msc_status = p_chann->msc_handshake;

    if (!tcr)   /* response */
    {
        p_chann->msc_handshake |= RFC_MSC_RSP_RCV;

        p_chann->us_flow_active &= ~RFC_MSC_CMD_OUTGOING;
        if (p_chann->us_flow_active & RFC_MSC_CMD_PENDING)
        {
            /* at least one intermediate request is stored locally --> send it now */
            p_chann->us_flow_active |= RFC_MSC_CMD_OUTGOING;    /* one MSC request outstanding */
            p_chann->us_flow_active &= ~RFC_MSC_CMD_PENDING;    /* nothing stored locally any more */
            sys_timer_start(p_chann->p_ctrl_chann->wait_rsp_timer_handle);
            rfc_send_msc(p_chann->p_ctrl_chann, 1, p_chann->dlci, p_chann->us_flow_ctrl,
                         p_chann->us_flow_break);
        }
    }
    else        /* command */
    {
        p_chann->msc_handshake |= RFC_MSC_CMD_RCV;

        rfc_send_msc(p_ctrl_chann, 0, dlci, status, 0);
        p_chann->ds_flow_ctrl = status;
    }

    if ((msc_status != p_chann->msc_handshake) &&
        (p_chann->msc_handshake == (RFC_MSC_CMD_RCV | RFC_MSC_RSP_RCV)))
    {
        T_RFC_CONN_CMPL msg;

        /* handshake never seen and T2 is not expired, so this is the first indication */
        sys_timer_stop(p_chann->wait_msc_timer_handle);

        RFCOMM_PRINT_INFO1("rfc_handle_msc: channel connected with frame size %d", p_chann->frame_size);

        msg.dlci = p_chann->dlci;
        msg.remain_credits = p_chann->remote_remain_credits;
        msg.profile_index = p_chann->profile_index;
        msg.frame_size = p_chann->frame_size;
        memcpy(msg.bd_addr, p_chann->p_ctrl_chann->bd_addr, 6);
        rfc_msg_to_profile(RFC_CONN_CMPL, p_chann, (void *)&msg);

        /* there might have changes that affect the ability to send data */
        rfc_check_send_credits();
    }
}

void rfc_handle_rls(T_RFC_CHANN *p_ctrl_chann,
                    uint8_t      tcr,
                    uint8_t     *p)
{
    uint8_t dlci;
    uint8_t status;
    T_RFC_CHANN *p_chann;

    LE_STREAM_TO_UINT8(dlci, p);
    LE_STREAM_TO_UINT8(status, p);

    dlci = dlci >> 2;
    p_chann = rfc_find_chann_by_dlci(dlci, p_ctrl_chann->l2c_cid);

    if (!p_chann)
    {
        return;
    }

    if (tcr)
    {
        /* reply the command with a response, copy local values */
        rfc_send_rls(p_ctrl_chann, 0, dlci, status);
        p_chann->rls = status;
    }
}

void rfc_handle_rpn(T_RFC_CHANN *p_ctrl_chann,
                    uint8_t      tcr,
                    T_RPN       *p_rpn,
                    uint16_t     tlen)
{
    T_RFC_CHANN *p_chann;
    uint8_t dlci = p_rpn->dlci >> 2;

    if (tlen != 1 && tlen != 8)
    {
        return;
    }

    p_chann = rfc_find_chann_by_dlci(dlci, p_ctrl_chann->l2c_cid);
    if (!p_chann)
    {
        p_chann = rfc_alloc_chann(dlci);

        if (!p_chann)
        {
            return;
        }

        p_chann->p_ctrl_chann = p_ctrl_chann;
        memcpy(&p_chann->rpn, p_rpn, (uint8_t)tlen);
    }

    if (tcr)
    {
        /* if this is a command, send indication to upper layer */
        rfc_send_uih(p_ctrl_chann, RFC_TYPE_RPN, 0, (uint8_t *)&p_chann->rpn, sizeof(p_chann->rpn));
    }
}

void rfc_handle_uih(T_RFC_CHANN *p_ctrl_chann,
                    uint8_t     *p_data,
                    uint16_t     len,
                    bool         cr)
{
    uint8_t  type;
    uint8_t  uih_cr;
    uint8_t *p = p_data;
    uint16_t uih_len;

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
        sys_timer_stop(p_ctrl_chann->wait_rsp_timer_handle);
    }

    switch (type)
    {
    case RFC_TYPE_PN:
        len -= p - p_data;
        rfc_handle_pn(p_ctrl_chann, uih_cr, p, len);
        break;

    case RFC_TYPE_FCON:
        if (uih_cr)
        {
            p_ctrl_chann->ds_flow_ctrl = false;
            rfc_send_uih(p_ctrl_chann, RFC_TYPE_FCON, 0, NULL, 0);
            rfc_check_send_credits();
        }
        break;

    case RFC_TYPE_FCOFF:
        if (uih_cr)
        {
            p_ctrl_chann->ds_flow_ctrl = RFC_FLOW_BIT;
            rfc_send_uih(p_ctrl_chann, RFC_TYPE_FCOFF, 0, NULL, 0);
        }
        break;

    case RFC_TYPE_MSC:
        rfc_handle_msc(p_ctrl_chann, uih_cr, p);
        break;

    case RFC_TYPE_RLS:
        rfc_handle_rls(p_ctrl_chann, uih_cr, p);
        break;

    case RFC_TYPE_RPN:
        rfc_handle_rpn(p_ctrl_chann, uih_cr, (T_RPN *)p, uih_len);
        break;

    case RFC_TYPE_TEST:
        if (uih_cr)
        {
            /* send back identical data as response */
            rfc_send_uih(p_ctrl_chann, RFC_TYPE_TEST, 0, p, uih_len);
        }
        break;

    case RFC_TYPE_NSC:
        /* NSC received:  shutdown all user channels and close the link */
        rfc_close_data_chann(p_ctrl_chann->p_ctrl_chann, RFC_ERR | RFC_ERR_NSC);
        mpa_send_l2c_disconn_req(p_ctrl_chann->p_ctrl_chann->l2c_cid);
        p_ctrl_chann->p_ctrl_chann->link_state = LINK_DISCONNECTING;
        break;

    default:
        RFCOMM_PRINT_ERROR1("rfc_handle_uih: unhandled type 0x%02x", type);
        if (cr)
        {
            /* reply with non supported command */
            rfc_send_uih(p_ctrl_chann, RFC_TYPE_NSC, 0, p_data, 1);
        }
        break;
    }
}

void rfc_handle_sabm(T_RFC_CHANN *p_chann,
                     uint8_t      poll)
{
    RFCOMM_PRINT_INFO6("rfc_handle_sabm: dlci 0x%02x, init %d, state 0x%02x, l2c_init %d, l2c_state 0x%02x, poll %d",
                       p_chann->dlci, p_chann->initiator, p_chann->state, p_chann->link_initiator,
                       p_chann->link_state, poll);

    if (!poll)
    {
        return;
    }

    if (p_chann->dlci == 0)     /* control channel */
    {
        uint8_t i;

        if (p_chann->state == DLCI_CONNECTING)      /* conflict SABM */
        {
            p_chann->initiator = false;
            sys_timer_stop(p_chann->wait_rsp_timer_handle);   /* remote may not send UA for previous SABM */
        }

        rfc_send_ua(p_chann, 0, 1);
        p_chann->state = DLCI_CONNECTED;

        /* check if data chennel from collison need to connect */
        for (i = 0; i < p_rfc->dlci_num; i++)
        {
            T_RFC_CHANN *p_tmp_chann = &p_rfc->p_chann[i];
            if (p_tmp_chann->used && p_tmp_chann->state == DLCI_OPEN &&
                p_tmp_chann->p_ctrl_chann == p_chann)
            {
                /* check if frameSize setting for this channel is acceptable according to L2CAP mtuSize */
                if ((p_tmp_chann->frame_size + RFC_RSVD_SIZE) > p_chann->mtu_size)
                {
                    p_tmp_chann->frame_size = p_chann->mtu_size - RFC_RSVD_SIZE;
                }

                rfc_send_authen_req(p_tmp_chann, 1);
            }
        }
    }
    else                        /* data channel */
    {
        switch (p_chann->state)
        {
        case DLCI_CONFIG_ACCEPTED:
            /* dlci is now open */
            p_chann->state = DLCI_CONNECTED;
            rfc_send_ua(p_chann, 0, 1);

            /* send initial MSC command on channel... */
            p_chann->us_flow_active |= RFC_MSC_CMD_OUTGOING;
            sys_timer_start(p_chann->p_ctrl_chann->wait_rsp_timer_handle);
            rfc_send_msc(p_chann->p_ctrl_chann, 1, p_chann->dlci, p_chann->us_flow_ctrl, 0);

            /* User channel is now connected, we must wait for MSC_IND before we can */
            /* signal to upper layer (Peiker - Verizone problem */
            sys_timer_start(p_chann->wait_msc_timer_handle);
            break;

        case DLCI_CONNECTED:
            /* dlci is already online, replace directly with UA */
            rfc_send_ua(p_chann, 0, 1);
            break;

        default:
            /* SABM on traffic channel received without prior TYPE_PN exchange: refuse by sending DM */
            rfc_send_dm(p_chann->p_ctrl_chann, p_chann->dlci, 0, 1);
            rfc_free_chann(p_chann, RFC_ERR | RFC_ERR_INVALID_STATE);
            break;
        }
    }
}

void rfc_handle_ua(T_RFC_CHANN *p_chann,
                   uint8_t      poll)
{
    RFCOMM_PRINT_INFO3("rfc_handle_ua: dlci 0x%02x, state 0x%02x, poll %d",
                       p_chann->dlci, p_chann->state, poll);

    if (!poll)
    {
        return;
    }

    sys_timer_stop(p_chann->wait_rsp_timer_handle);

    if (p_chann->dlci == 0)     /* control channel */
    {
        switch (p_chann->state)
        {
        case DLCI_CONNECTING:
            {
                int i;
                p_chann->state = DLCI_CONNECTED;

                for (i = 0; i < p_rfc->dlci_num; i++)
                {
                    T_RFC_CHANN *p_tmp_chann = &p_rfc->p_chann[i];
                    if (p_tmp_chann->used && p_tmp_chann->state == DLCI_OPEN &&
                        p_tmp_chann->p_ctrl_chann == p_chann)
                    {
                        /* check if frameSize setting for this channel is acceptable according to L2CAP mtuSize */
                        if ((p_tmp_chann->frame_size + RFC_RSVD_SIZE) > p_chann->mtu_size)
                        {
                            p_tmp_chann->frame_size = p_chann->mtu_size - RFC_RSVD_SIZE;
                        }

                        rfc_send_authen_req(p_tmp_chann, 1);
                    }
                }
            }
            break;

        case DLCI_DISCONNECT_PENDING:
            p_chann->state = DLCI_DISCONNECTING;
            sys_timer_start(p_chann->wait_rsp_timer_handle);
            rfc_send_disc(p_chann, 1, 1);
            break;

        case DLCI_DISCONNECTING:
            p_chann->state = DLCI_IDLE;
            rfc_close_data_chann(p_chann, RFC_SUCCESS);
            mpa_send_l2c_disconn_req(p_chann->l2c_cid);
            p_chann->link_state = LINK_DISCONNECTING;
            break;

        default:
            break;
        }
    }
    else                        /* data channel */
    {
        switch (p_chann->state)
        {
        case DLCI_CONNECTING:
            p_chann->state = DLCI_CONNECTED;

            /* User channel is now connected, we must wait for MSC_IND before we can */
            /* signal to upper layer (Peiker - Verizone problem                      */
            sys_timer_start(p_chann->wait_msc_timer_handle);

            p_chann->us_flow_active |= RFC_MSC_CMD_OUTGOING;
            sys_timer_start(p_chann->p_ctrl_chann->wait_rsp_timer_handle);
            rfc_send_msc(p_chann->p_ctrl_chann, 1, p_chann->dlci, p_chann->us_flow_ctrl, 0);
            break;

        case DLCI_DISCONNECT_PENDING:
            p_chann->state = DLCI_DISCONNECTING;
            sys_timer_start(p_chann->wait_rsp_timer_handle);
            rfc_send_disc(p_chann, 1, 1);
            break;

        case DLCI_DISCONNECTING:
            p_chann->state = DLCI_IDLE;
            rfc_check_disconn_ctrl_chann(p_chann->p_ctrl_chann);
            rfc_free_chann(p_chann, RFC_SUCCESS);
            break;

        default:
            break;
        }
    }
}

void rfc_handle_dm(T_RFC_CHANN *p_chann)
{
    RFCOMM_PRINT_INFO3("rfc_handle_dm: chann used %d, dlci 0x%02x, state 0x%02x",
                       p_chann->used, p_chann->dlci, p_chann->state);

    if (!p_chann->used || p_chann->state == DLCI_IDLE)
    {
        return;
    }

    sys_timer_stop(p_chann->wait_rsp_timer_handle);
    p_chann->state = DLCI_IDLE;

    if (p_chann->dlci)
    {
        rfc_check_disconn_ctrl_chann(p_chann->p_ctrl_chann);
    }
    else
    {
        rfc_close_data_chann(p_chann, RFC_ERR | RFC_ERR_INVALID_STATE);
        mpa_send_l2c_disconn_req(p_chann->l2c_cid);
        p_chann->link_state = LINK_DISCONNECTING;
    }

    rfc_free_chann(p_chann, RFC_ERR | RFC_ERR_INVALID_STATE);
    rfc_check_send_credits();
}

void rfc_handle_disc(T_RFC_CHANN *p_chann,
                     uint8_t      poll)
{
    RFCOMM_PRINT_INFO3("rfc_handle_disc: dlci 0x%02x, state 0x%02x, poll %d",
                       p_chann->dlci, p_chann->state, poll);

    if (!poll)
    {
        return;
    }

    switch (p_chann->state)
    {
    case DLCI_CONNECTED:
        rfc_send_ua(p_chann, 0, 1);
        break;

    case DLCI_DISCONNECTING:
        rfc_send_ua(p_chann, 0, 1);

        /* we handle this internally as if a UA was received */
        sys_timer_stop(p_chann->wait_rsp_timer_handle);
        p_chann->state = DLCI_IDLE;
        if (p_chann->dlci == 0)
        {
            rfc_close_data_chann(p_chann, RFC_SUCCESS);
            mpa_send_l2c_disconn_req(p_chann->l2c_cid);
            p_chann->link_state = LINK_DISCONNECTING;
        }
        else
        {
            rfc_check_disconn_ctrl_chann(p_chann->p_ctrl_chann);
            rfc_free_chann(p_chann, RFC_SUCCESS);
        }
        return;

    default:
        /* this is a DISC on a not open channel */
        rfc_send_dm(p_chann->p_ctrl_chann, p_chann->dlci, 0, 1);
        break;
    }

    p_chann->state = DLCI_IDLE;

    if (p_chann->dlci == 0)     /* l2cap chann will be disconnect by remote side */
    {
        rfc_close_data_chann(p_chann, RFC_SUCCESS);
    }
    else
    {
        rfc_free_chann(p_chann, RFC_SUCCESS);
    }

    rfc_check_send_credits();
}

void rfc_handle_l2c_conn_ind(T_MPA_L2C_CONN_IND *p_ind)
{
    T_MPA_L2C_CONN_CFM_CAUSE status = MPA_L2C_CONN_NO_RESOURCE;
    T_RFC_CHANN *p_ctrl_chann = rfc_alloc_chann(0);
    uint8_t mode = MPA_L2C_MODE_BASIC;

    /* do not check if ctrl chann exists, when collision happens, android phone will start a timer
       when receive our l2cap connection request, if we acccept its l2cap connection request, it will
       reject ours. The two control channel will be distinguished by cid. */
    if (p_ctrl_chann)
    {
        status = MPA_L2C_CONN_ACCEPT;
        p_ctrl_chann->l2c_cid = p_ind->cid;
        p_ctrl_chann->p_ctrl_chann = p_ctrl_chann;
        memcpy(p_ctrl_chann->bd_addr, p_ind->bd_addr, 6);
        p_ctrl_chann->link_state = LINK_CONNECTING;
    }

    if (p_rfc->enable_ertm)
    {
        mode |= MPA_L2C_MODE_ERTM;
    }

    mpa_send_l2c_conn_cfm(status, p_ind->cid, RFC_MTU_SIZE, mode, 0xFFFF);
}

void rfc_handle_l2c_conn_rsp(T_MPA_L2C_CONN_RSP *p_rsp)
{
    T_RFC_CHANN *p_ctrl_chann = rfc_find_chann_by_addr(p_rsp->bd_addr, 0);

    RFCOMM_PRINT_INFO3("rfc_handle_l2c_conn_rsp: addr %s, cause 0x%04x, channel %p",
                       TRACE_BDADDR(p_rsp->bd_addr), p_rsp->cause, p_ctrl_chann);

    if (!p_ctrl_chann)
    {
        return;
    }

    if (p_rsp->cause)
    {
        rfc_close_data_chann(p_ctrl_chann, p_rsp->cause);
        p_ctrl_chann->link_state = LINK_IDLE;
        rfc_free_chann(p_ctrl_chann, p_rsp->cause);
        return;
    }

    p_ctrl_chann->l2c_cid = p_rsp->cid;

    //FIXME JW not needed anymore
    if (p_ctrl_chann->link_state == LINK_DISCONNECT_REQ)
    {
        RFCOMM_PRINT_WARN1("rfc_handle_l2c_conn_rsp: disc requested for cid 0x%04x", p_rsp->cid);
        rfc_close_data_chann(p_ctrl_chann, p_rsp->cause);
        mpa_send_l2c_disconn_req(p_rsp->cid);
        p_ctrl_chann->link_state = LINK_DISCONNECTING;
    }
}

void rfc_handle_l2c_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *p_info)
{
    T_RFC_CHANN *p_ctrl_chann = NULL;
    T_RFC_CHANN *p_new_ctrl_chann = NULL;

    p_ctrl_chann = rfc_find_chann_by_dlci(0, p_info->cid);

    RFCOMM_PRINT_INFO4("rfc_handle_l2c_conn_cmpl: cause 0x%04x, cid 0x%04x, mtu %d, channel %p",
                       p_info->cause, p_info->cid, p_info->remote_mtu, p_ctrl_chann);

    if (!p_ctrl_chann)
    {
        if (p_info->cause == 0)
        {
            mpa_send_l2c_disconn_req(p_info->cid);
        }
        return;
    }

    if (p_info->cause)
    {
        /* check if this fail happened because of collision */
        p_new_ctrl_chann = rfc_check_l2c_collision(p_ctrl_chann);

        if (p_new_ctrl_chann)
        {
            rfc_change_ctrl_chann(p_ctrl_chann, p_new_ctrl_chann);
        }
        else
        {
            rfc_close_data_chann(p_ctrl_chann, p_info->cause);
        }

        p_ctrl_chann->link_state = LINK_IDLE;
        rfc_free_chann(p_ctrl_chann, p_info->cause);
    }
    else
    {
        /* incase phone accept our l2cap connection request after we accept its */
        p_new_ctrl_chann = rfc_check_l2c_collision(p_ctrl_chann);
        if (p_new_ctrl_chann)
        {
            rfc_change_ctrl_chann(p_ctrl_chann, p_new_ctrl_chann);
            mpa_send_l2c_disconn_req(p_ctrl_chann->l2c_cid);
        }
        else
        {
            p_rfc->ds_offset  = p_info->ds_data_offset;
            p_ctrl_chann->mtu_size = p_info->remote_mtu;
            p_ctrl_chann->link_state = LINK_CONNECTED;

            if (p_ctrl_chann->state == DLCI_IDLE && p_ctrl_chann->link_initiator)
            {
                p_ctrl_chann->initiator = true;
                p_ctrl_chann->state = DLCI_CONNECTING;
                sys_timer_start(p_ctrl_chann->wait_rsp_timer_handle);
                rfc_send_sabm(p_ctrl_chann, 1, 1);
            }
        }
    }
}

void rfc_handle_l2c_disconn_ind(T_MPA_L2C_DISCONN_IND *p_ind)
{
    T_RFC_CHANN *p_ctrl_chann;

    mpa_send_l2c_disconn_cfm(p_ind->cid);
    p_ctrl_chann = rfc_find_chann_by_dlci(0, p_ind->cid);

    RFCOMM_PRINT_INFO3("rfc_handle_l2c_disconn_ind: cid 0x%04x, cause 0x%04x, channel %p",
                       p_ind->cid, p_ind->cause, p_ctrl_chann);

    if (!p_ctrl_chann)
    {
        return;
    }

    p_ctrl_chann->link_state = LINK_IDLE;
    rfc_close_data_chann(p_ctrl_chann, p_ind->cause);
    rfc_free_chann(p_ctrl_chann, p_ind->cause);
}

void rfc_handle_l2c_disconn_rsp(T_MPA_L2C_DISCONN_RSP *p_rsp)
{
    T_RFC_CHANN *p_ctrl_chann;

    p_ctrl_chann = rfc_find_chann_by_dlci(0, p_rsp->cid);

    RFCOMM_PRINT_INFO2("rfc_handle_l2c_disconn_rsp: cid 0x%04x, channel %p",
                       p_rsp->cid, p_ctrl_chann);

    if (!p_ctrl_chann)
    {
        return;
    }

    p_ctrl_chann->link_state = LINK_IDLE;
    rfc_close_data_chann(p_ctrl_chann, p_rsp->cause);
    rfc_free_chann(p_ctrl_chann, p_rsp->cause);
}

void rfc_handle_l2c_data_ind(T_MPA_L2C_DATA_IND *p_ind)
{
    uint8_t *p = p_ind->data + p_ind->gap;
    T_RFC_CHANN *p_chann;
    uint8_t dlci;
    uint8_t cr;                   /* cr bit from frame */
    uint8_t type;
    uint8_t poll;                 /* poll bit from frame */
    uint16_t len;
    uint8_t return_credits = 0;
    bool command;

    if (p_ind->length < 4)
    {
        return;
    }

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
        p_ind->length -= 3;
    }
    else
    {
        len >>= 1;
        len |= (uint16_t)(*p++) << 7;
        p_ind->length -= 4;
    }

    if (dlci && (type == RFC_UIH) && poll)
    {
        /* this is a UIH frame on traffic channel mit poll bit set: it contains backCredits field! */
        return_credits = *p++; /* fetch the return credits and remove the field from the payload */
        p_ind->length--;
    }

    /* make some additional syntax checks on the frame */
    if (len + RFC_CRC_FIELD_SIZE != p_ind->length)
    {
        return;
    }

    if (crc8EtsCheck(p_ind->data + p_ind->gap, type == RFC_UIH ? 2 : 3, *(p + len)) == false)
    {
        return;
    }

    p_chann = rfc_find_chann_by_dlci(dlci, p_ind->cid);

    if (!p_chann)
    {
        if (dlci)
        {
            T_RFC_CHANN *p_ctrl_chann = rfc_find_chann_by_dlci(0, p_ind->cid);

            if (p_ctrl_chann)
            {
                if (type == RFC_SABM)
                {
                    /* recv sabm with out pn or chann is freed when cfm not accept, just send dm */
                    rfc_send_dm(p_ctrl_chann, dlci, 0, 1);
                }
                else if (type != RFC_DM)
                {
                    /* we ignore DM on non existing channels */
                    if ((cr && !p_ctrl_chann->initiator) || (!cr && p_ctrl_chann->initiator))
                    {
                        rfc_send_dm(p_ctrl_chann, dlci, 0, 0);
                    }
                }
            }
        }

        return;
    }

    /* it is a command if cr=1 and peer is initiator or if cr=0 and peer is not initiator */
    if ((cr && !p_chann->p_ctrl_chann->initiator) || (!cr && p_chann->p_ctrl_chann->initiator))
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
        rfc_handle_sabm(p_chann, poll);
        break;

    case RFC_UA:
        rfc_handle_ua(p_chann, poll);
        break;

    case RFC_DM:
        rfc_handle_dm(p_chann);
        break;

    case RFC_DISC:
        rfc_handle_disc(p_chann, poll);
        break;

    case RFC_UIH:
        /* check if the link is connected, answer with DM if not */
        if (p_chann->state != DLCI_CONNECTED)
        {
            rfc_send_dm(p_chann->p_ctrl_chann, p_chann->dlci, 0, 0);
            break;
        }

        if (dlci == 0)
        {
            rfc_handle_uih(p_chann, p, len, command);
            break;
        }

        /* user data on dlci */
        if ((p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM) && poll && return_credits)
        {
            /* there is a return credit field in the packet */
            p_chann->remote_remain_credits += return_credits;

            RFCOMM_PRINT_TRACE3("rfc_handle_l2c_data_ind: dlci 0x%02x, get credits %d, credits now %d",
                                p_chann->dlci, return_credits, p_chann->remote_remain_credits);

            rfc_check_send_credits();
        }

        if (len > 0)     /* remote send data */
        {
            T_RFC_DATA_IND msg;
            msg.dlci = p_chann->dlci;
            msg.remain_credits = p_chann->remote_remain_credits;
            msg.buf = p;
            msg.length = len;
            memcpy(msg.bd_addr, p_chann->p_ctrl_chann->bd_addr, 6);
            rfc_msg_to_profile(RFC_DATA_IND, p_chann, (void *)&msg);
        }
        else            /* remote only send credits */
        {
            T_RFC_CREDIT_INFO msg;
            msg.dlci = p_chann->dlci;
            msg.remain_credits = p_chann->remote_remain_credits;
            memcpy(msg.bd_addr, p_chann->p_ctrl_chann->bd_addr, 6);
            rfc_msg_to_profile(RFC_CREDIT_INFO, p_chann, (void *)&msg);
        }
        break;

    default:
        RFCOMM_PRINT_ERROR1("rfc_handle_l2c_data_ind: unknown data type 0x%02x", type);
        break;
    }
}

void rfc_handle_l2c_data_rsp(T_MPA_L2C_DATA_RSP *p_rsp)
{
    T_RFC_CHANN *p_chann;

    p_chann = rfc_find_chann_by_dlci(p_rsp->dlci, p_rsp->cid);
    if (p_chann != NULL)
    {
        T_RFC_DATA_RSP msg;

        msg.dlci = p_rsp->dlci;
        memcpy(msg.bd_addr, p_chann->p_ctrl_chann->bd_addr, 6);
        rfc_msg_to_profile(RFC_DATA_RSP, p_chann, (void *)&msg);
    }
}

void rfc_handle_sec_reg_rsp(T_MPA_L2C_SEC_REG_RSP *p_rsp)
{
    uint8_t index;
    T_RFC_SEC_REG_RSP sec_rsp;
    T_RFC_PROFILE *p_profile;

    sec_rsp.server_chann = p_rsp->server_chann;
    sec_rsp.uuid = p_rsp->uuid;
    sec_rsp.cause = p_rsp->cause;
    sec_rsp.active = p_rsp->active;

    for (index = 0; index < p_rfc->profile_num; index++)
    {
        p_profile = &p_rfc->p_profile_cb[index];

        if ((p_profile->server_chann == p_rsp->server_chann) && p_profile->cb)
        {
            p_profile->cb(RFC_SEC_REG_RSP, (void *)&sec_rsp);
            return;
        }
    }
}

void rfc_handle_author_ind(T_MPA_AUTHOR_REQ_IND *p_ind)
{
    /* for rfcomm, uuid in authorization request indicate is profile_cb_index we send in authentication request */
    P_RFC_PROFILE_CB profile_cb = NULL;

    if (p_ind->uuid < p_rfc->profile_num)
    {
        profile_cb = p_rfc->p_profile_cb[p_ind->uuid].cb;
    }

    if (profile_cb)
    {
        profile_cb(RFC_AUTHOR_IND, (void *)p_ind);
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
        RFCOMM_PRINT_ERROR1("rfc_handle_l2c_msg: unknown message 0x%02x", l2c_msg);
        break;
    }
}

void rfc_handle_authen_rsp(T_MPA_RFC_AUTHEN_RSP *p_rsp)
{
    T_RFC_CHANN *p_chann;

    p_chann = rfc_find_chann_by_dlci(p_rsp->dlci, p_rsp->cid);

    RFCOMM_PRINT_INFO4("rfc_handle_authen_rsp: dlci 0x%02x, cid 0x%04x, cause 0x%04x, channel %p",
                       p_rsp->dlci, p_rsp->cid, p_rsp->cause, p_chann);

    if (p_chann == NULL)
    {
        return;
    }

    if (p_rsp->outgoing)
    {
        if (p_rsp->cause)
        {
            /* outgoing connection rejected by security manager */
            p_chann->state = DLCI_IDLE;
            rfc_check_disconn_ctrl_chann(p_chann->p_ctrl_chann);
            rfc_free_chann(p_chann, RFC_ERR | RFC_ERR_REJECT_SECURITY);
            return;
        }

        if (p_chann->state == DLCI_OPEN)
        {
            uint8_t pn[RFC_SIZE_PN];
            rfc_encode_pn(p_chann, pn);
            p_chann->state = DLCI_CONFIG_OUTGOING;
            sys_timer_start(p_chann->wait_rsp_timer_handle);
            rfc_send_uih(p_chann->p_ctrl_chann, RFC_TYPE_PN, 1, pn, sizeof(pn));
        }
    }
    else
    {
        if (p_rsp->cause)
        {
            /* incoming connection rejected by security manager */
            /* do not answer with DM, but answer with PN because some Microsoft implementations have problems
               with TYPE_PN / DM exchange and seem to prefer TYPE_PN / TYPE_PN followed  by SABM / DM.
               The channel descriptor for this DLCI is deallocated, so the subsequent SABM will occur on an
               unconfigured DLCI, therefore being rejected.
               */
            uint8_t pn[RFC_SIZE_PN];
            rfc_encode_pn(p_chann, pn);    /* answer with unchanged configuration */
            rfc_send_uih(p_chann->p_ctrl_chann, RFC_TYPE_PN, 0, pn, sizeof(pn));
            rfc_free_chann(p_chann, RFC_ERR | RFC_ERR_REJECT_SECURITY);
            return;
        }

        if (p_chann->state == DLCI_CONFIG_INCOMING)
        {
            T_RFC_CONN_IND conn_ind;
            conn_ind.frame_size = p_chann->frame_size;
            conn_ind.dlci = p_chann->dlci;
            memcpy(conn_ind.bd_addr, p_chann->p_ctrl_chann->bd_addr, 6);
            rfc_msg_to_profile(RFC_CONN_IND, p_chann, (void *)&conn_ind);
        }
    }
}

bool rfc_init(void)
{
    int32_t ret = 0;

    p_rfc = (T_RFC *)os_mem_zalloc2(sizeof(T_RFC));
    if (p_rfc == NULL)
    {
        ret = 1;
        goto fail_alloc_rfc;
    }

    p_rfc->rsp_tout = 20;
    p_rfc->msc_tout = 20;
    p_rfc->ds_offset = 24;
    p_rfc->dlci_num = 14;
    p_rfc->profile_num = 8;

    if (p_rfc->dlci_num != 0)
    {
        p_rfc->p_chann = os_mem_zalloc2(sizeof(T_RFC_CHANN) * p_rfc->dlci_num);
        if (p_rfc->p_chann == NULL)
        {
            ret = 2;
            goto fail_alloc_chann;
        }
    }

    if (p_rfc->profile_num != 0)
    {
        p_rfc->p_profile_cb = os_mem_zalloc2(sizeof(T_RFC_PROFILE) * p_rfc->profile_num);
        if (p_rfc->p_profile_cb == NULL)
        {
            ret = 3;
            goto fail_alloc_profile_cb;
        }
    }

    if (mpa_reg_l2c_proto(PSM_RFCOMM, rfc_handle_l2c_msg, &p_rfc->queue_id) == false)
    {
        ret = 4;
        goto fail_reg_l2c;
    }

    mpa_reg_rfc_authen_cb(rfc_handle_authen_rsp);

    return true;

fail_reg_l2c:
    os_mem_free(p_rfc->p_profile_cb);
fail_alloc_profile_cb:
    os_mem_free(p_rfc->p_chann);
fail_alloc_chann:
    os_mem_free(p_rfc);
    p_rfc = NULL;
fail_alloc_rfc:
    PROFILE_PRINT_ERROR1("rfc_init: failed %d", -ret);
    return false;
}

bool rfc_reg_cb(uint8_t           chann_num,
                P_RFC_PROFILE_CB  callback,
                uint8_t          *p_idx)
{
    uint8_t index = 0;

    if (p_rfc == NULL)
    {
        if (!rfc_init())
        {
            RFCOMM_PRINT_ERROR0("rfc_reg_cb: init rfcomm fail");
            return false;
        }
    }

    for (index = 0; index < p_rfc->profile_num; index++)
    {
        if ((p_rfc->p_profile_cb[index].cb != NULL) &&
            (p_rfc->p_profile_cb[index].server_chann == chann_num))
        {
            RFCOMM_PRINT_WARN1("rfc_reg_cb: channel number %d is used", chann_num);
            return false;
        }
    }

    for (index = 0; index < p_rfc->profile_num; index++)
    {
        if (p_rfc->p_profile_cb[index].cb == NULL)
        {
            p_rfc->p_profile_cb[index].server_chann = chann_num;
            p_rfc->p_profile_cb[index].cb = callback;

            *p_idx = index;
            return true;
        }
    }

    return false;
}

bool rfc_reg_sec(uint8_t  active,
                 uint16_t server_chann,
                 uint8_t  profile_idx,
                 uint8_t  sec)
{
    mpa_send_l2c_sec_reg_req(active, PSM_RFCOMM, server_chann, profile_idx, sec);

    return true;
}

bool rfc_set_ertm_mode(bool enable)
{
    if (p_rfc != NULL)
    {
        p_rfc->enable_ertm = enable;
        return true;
    }

    return false;
}

bool rfc_conn_req(uint8_t   bd_addr[6],
                  uint8_t   server_chann,
                  uint16_t  frame_size,
                  uint8_t   max_credits,
                  uint8_t   profile_index,
                  uint8_t  *p_dlci)
{
    uint8_t dlci;
    uint8_t mode = MPA_L2C_MODE_BASIC;
    bool new_control = false;
    T_RFC_CHANN *p_data_chann;
    T_RFC_CHANN *p_ctrl_chann;
    int8_t result = 0;

    RFCOMM_PRINT_INFO4("rfc_conn_req: server channel 0x%02x, addr %s, frame size %d, max credits %d",
                       server_chann, TRACE_BDADDR(bd_addr), frame_size, max_credits);

    if (frame_size == 0)
    {
        frame_size = RFC_DEFAULT_MTU;
    }

    dlci = server_chann << 1;

    p_ctrl_chann = rfc_find_chann_by_addr(bd_addr, 0);
    if (!p_ctrl_chann)
    {
        p_ctrl_chann = rfc_alloc_chann(0);
        if (!p_ctrl_chann)
        {
            result = 1;
            goto fail_no_ctrl_chann;
        }

        p_ctrl_chann->p_ctrl_chann = p_ctrl_chann;
        memcpy(p_ctrl_chann->bd_addr, bd_addr, 6);
        new_control = true;
    }

    if ((p_ctrl_chann->state == DLCI_IDLE) || p_ctrl_chann->initiator)
    {
        /* we are / will be initiator of the connection: our direction bit is 1 */
        /* the direction bit of the remote side is "0" */
    }
    else
    {
        dlci |= 1;
    }

    /* now the DLCI also contains the direction bit */
    p_data_chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (p_data_chann)
    {
        RFCOMM_PRINT_ERROR1("rfc_conn_req: data chann already exists dlci 0x%02x", dlci);
        if (new_control)
        {
            rfc_free_chann(p_ctrl_chann, RFC_ERR_INVALID_STATE);
        }
        result = 2;
        goto fail_exist_data_chann;
    }

    p_data_chann = rfc_alloc_chann(dlci);
    if (!p_data_chann)
    {
        if (new_control)
        {
            rfc_free_chann(p_ctrl_chann, RFC_ERR_INVALID_STATE);
        }
        result = 3;
        goto fail_no_data_chann;
    }

    rfc_init_credits(p_data_chann, max_credits);
    p_data_chann->p_ctrl_chann  = p_ctrl_chann;
    p_data_chann->profile_index = profile_index;
    p_data_chann->state = DLCI_OPEN;
    p_data_chann->frame_size = frame_size;

    *p_dlci = p_data_chann->dlci;

    RFCOMM_PRINT_INFO2("rfc_conn_req: link state %d, ctrl chann state %d",
                       p_ctrl_chann->link_state, p_ctrl_chann->state);

    switch (p_ctrl_chann->link_state)
    {
    case LINK_IDLE:
        if (p_rfc->enable_ertm)
        {
            mode |= MPA_L2C_MODE_ERTM;
        }
        mpa_send_l2c_conn_req(PSM_RFCOMM, UUID_RFCOMM, p_rfc->queue_id, RFC_MTU_SIZE, bd_addr, mode,
                              0xFFFF);
        p_ctrl_chann->link_initiator = true;
        p_ctrl_chann->profile_index = profile_index;
        p_ctrl_chann->link_state = LINK_CONNECTING;
        break;

    case LINK_CONNECTED:
        switch (p_ctrl_chann->state)
        {
        case DLCI_IDLE:
            p_ctrl_chann->initiator = true;
            p_ctrl_chann->state = DLCI_CONNECTING;
            sys_timer_start(p_ctrl_chann->wait_rsp_timer_handle);
            rfc_send_sabm(p_ctrl_chann, 1, 1);
            break;

        case DLCI_CONNECTED:
            /* check if frameSize setting for this channel is acceptable according to L2CAP mtuSize */
            if ((p_data_chann->frame_size + RFC_RSVD_SIZE) > p_data_chann->p_ctrl_chann->mtu_size)
            {
                p_data_chann->frame_size = p_data_chann->p_ctrl_chann->mtu_size - RFC_RSVD_SIZE;
            }
            rfc_send_authen_req(p_data_chann, 1);
            break;

        case DLCI_DISCONNECTING:
            result = 4;
            goto fail_wrong_state;

        default:
            break;
        }
        break;

    case LINK_DISCONNECT_REQ:
        p_ctrl_chann->link_state = LINK_CONNECTING;
        break;

    case LINK_DISCONNECTING:
        sys_timer_delete(p_data_chann->wait_rsp_timer_handle);
        sys_timer_delete(p_data_chann->wait_msc_timer_handle);
        p_data_chann->used = false;
        result = 5;
        goto fail_wrong_state;

    default:
        break;
    }

    return true;

fail_wrong_state:
fail_no_data_chann:
fail_exist_data_chann:
fail_no_ctrl_chann:

    RFCOMM_PRINT_ERROR1("rfc_conn_req: failed %d", -result);
    return false;
}

void rfc_conn_cfm(uint8_t  bd_addr[6],
                  uint8_t  dlci,
                  uint16_t status,
                  uint16_t frame_size,
                  uint8_t  max_credits)
{
    T_RFC_CHANN *p_chann = rfc_find_chann_by_addr(bd_addr, dlci);

    if (!p_chann)
    {
        return;
    }

    RFCOMM_PRINT_INFO6("rfc_conn_cfm: addr %s, dlci 0x%02x, accept 0x%04x, max credits %d, frame size %d, state 0x%02x",
                       TRACE_BDADDR(bd_addr), dlci, status, max_credits, frame_size, p_chann->state);

    if (status)   /* this is a reject !*/
    {
        /* do not answer with DM, but answer with PN because some Microsoft implementations have problems
           with TYPE_PN / DM exchange and seem to prefer TYPE_PN / TYPE_PN followed  by SABM / DM.
           The channel descriptor for this DLCI is deallocated, so the subsequent SABM will occur on an
           unconfigured DLCI, therefore being rejected.
         */
        uint8_t pn[RFC_SIZE_PN];
        rfc_encode_pn(p_chann, pn);    /* answer with unchanged configuration */
        rfc_send_uih(p_chann->p_ctrl_chann, RFC_TYPE_PN, 0, pn, sizeof(pn));

        rfc_free_chann(p_chann, RFC_SUCCESS);
    }
    else
    {
        if (p_chann->p_ctrl_chann->link_state == LINK_DISCONNECT_REQ)
        {
            return;
        }

        if (p_chann->state == DLCI_CONFIG_INCOMING)
        {
            /* opening a channel where we received a PN command for channel configuration */
            uint8_t pn[RFC_SIZE_PN];
            p_chann->us_flow_ctrl = 0;
            p_chann->frame_size = (frame_size > p_chann->frame_size) ? p_chann->frame_size : frame_size;
            rfc_init_credits(p_chann, max_credits);
            p_chann->state = DLCI_CONFIG_ACCEPTED;
            rfc_encode_pn(p_chann, pn);    /* answer with (changed) configuration */
            rfc_send_uih(p_chann->p_ctrl_chann, RFC_TYPE_PN, 0, pn, sizeof(pn));
        }
    }
}

bool rfc_disconn_req(uint8_t bd_addr[6],
                     uint8_t dlci)
{
    int8_t result = 0;
    T_RFC_CHANN *p_chann;

    p_chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (!p_chann)
    {
        result = 1;
        goto fail_invalid_param;
    }

    RFCOMM_PRINT_INFO4("rfc_disconn_req: addr %s, handle 0x%02x, link state 0x%02x, chann state 0x%02x",
                       TRACE_BDADDR(bd_addr), dlci, p_chann->p_ctrl_chann->link_state, p_chann->state);

    if (p_chann->p_ctrl_chann->link_state == LINK_CONNECTING &&
        p_chann->p_ctrl_chann->l2c_cid == 0)
    {
        p_chann->p_ctrl_chann->link_state = LINK_DISCONNECT_REQ;
        return true;
    }

    switch (p_chann->state)
    {
    case DLCI_IDLE:
    case DLCI_DISCONNECTING:
        result = 2;
        goto fail_wrong_state;

    case DLCI_OPEN:         //SABM not send
        p_chann->state = DLCI_IDLE;
        rfc_check_disconn_ctrl_chann(p_chann->p_ctrl_chann);
        rfc_free_chann(p_chann, RFC_SUCCESS);
        break;

    case DLCI_CONNECTING:   //SABM send, UA not received
        p_chann->state = DLCI_DISCONNECT_PENDING;
        break;

    default:
        p_chann->state = DLCI_DISCONNECTING;
        sys_timer_start(p_chann->wait_rsp_timer_handle);
        rfc_send_disc(p_chann, 1, 1);

        rfc_check_send_credits();
        break;
    }

    return true;

fail_wrong_state:
fail_invalid_param:
    RFCOMM_PRINT_ERROR1("rfc_disconn_req: failed %d", -result);
    return false;
}

bool rfc_flow_ctrl_req(uint8_t bd_addr[6],
                       uint8_t dlci,
                       uint8_t flow_status,
                       uint8_t sbreak)
{
    T_RFC_CHANN *p_chann = rfc_find_chann_by_addr(bd_addr, dlci);

    if (!p_chann || p_chann->state != DLCI_CONNECTED)
    {
        return false;
    }

    /* store new flow state */
    p_chann->us_flow_ctrl = flow_status;
    p_chann->us_flow_break = sbreak;

    if (p_chann->dlci == 0)
    {
        /* Flow Control Command from App Layer on Control Channel, this is aggregate flow control */
        sys_timer_start(p_chann->wait_rsp_timer_handle);
        rfc_send_uih(p_chann, (p_chann->us_flow_ctrl & RFC_FLOW_BIT) == 0 ? RFC_TYPE_FCON :
                     RFC_TYPE_FCOFF, 1, NULL, 0);
    }
    else
    {
        /* generateMSC_CONF locally and immediate */
        /* Flow Control Command from App Layer on User Channel, send Modem Status Command to Peer */
        if (p_chann->us_flow_active & RFC_MSC_CMD_OUTGOING)
        {
            /* there is an outstanding MSC on this link: do not send now, but only store parameter values (already done)
               and remember the fact in bit 1*/
            p_chann->us_flow_active |= RFC_MSC_CMD_PENDING;
        }
        else
        {
            /* there is currently no outstanding MSC, send the MSC request and indicate MSC outstanding */
            p_chann->us_flow_active |= RFC_MSC_CMD_OUTGOING;
            sys_timer_start(p_chann->p_ctrl_chann->wait_rsp_timer_handle);
            rfc_send_msc(p_chann->p_ctrl_chann, 1, p_chann->dlci, p_chann->us_flow_ctrl, sbreak);
        }
    }

    return true;
}

bool rfc_data_req(uint8_t   bd_addr[6],
                  uint8_t   dlci,
                  uint8_t  *p_data,
                  uint16_t  len,
                  bool      ack)
{
    T_RFC_CHANN *p_chann;
    uint8_t header_size = 0;
    bool credit_field = false;
    uint8_t *p_buf;
    uint8_t *p;
    uint8_t  offset = p_rfc->ds_offset;

    p_chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (!p_chann)
    {
        return false;
    }

    if ((p_chann->state != DLCI_CONNECTED) || rfc_check_block(p_chann))
    {
        RFCOMM_PRINT_ERROR2("rfc_data_req: dlci 0x%02x in wrong state 0x%02x",
                            p_chann->dlci, p_chann->state);
        return false;
    }

    if ((len > p_chann->frame_size) || (len == 0))
    {
        RFCOMM_PRINT_ERROR2("rfc_data_req: len %d not right, frame size %d", len, p_chann->frame_size);
        return false;
    }

    if (len > 127)
    {
        header_size = 4;      /* 1 byte addr, 1 byte control, 2 bytes length*/
    }
    else
    {
        header_size = 3;      /* 1 byte addr, 1 byte control, 1 byte length*/
    }

    if ((p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM) && p_chann->given_credits)
    {
        credit_field = true;
        header_size += 1;
    }

    p_buf = mpa_get_l2c_buf(p_rfc->queue_id, p_chann->p_ctrl_chann->l2c_cid, p_chann->dlci,
                            len + header_size + RFC_CRC_FIELD_SIZE, offset, ack);
    if (p_buf == NULL)
    {
        RFCOMM_PRINT_ERROR1("rfc_data_req: get buffer failed for len %d", len + header_size + 1);
        return false;
    }

    p = p_buf + offset;

    /* build the message header */
    if (p_chann->p_ctrl_chann->initiator)
    {
        LE_UINT8_TO_STREAM(p, (p_chann->dlci << 2) | 2 | RFC_EA_BIT);
    }
    else
    {
        LE_UINT8_TO_STREAM(p, (p_chann->dlci << 2) | 0 | RFC_EA_BIT);
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
        LE_UINT8_TO_STREAM(p, p_chann->given_credits);
        p_chann->given_credits = 0;
    }

    memcpy(p, p_data, len);
    p += len;

    LE_UINT8_TO_STREAM(p, crc8EtsGen(p_buf + offset, 2));

    mpa_send_l2c_data_req(p_buf, offset, p_chann->p_ctrl_chann->l2c_cid,
                          len + header_size + 1, false);

    if ((p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM) && len)
    {
        /* decrement credits only for non zero packets */
        p_chann->remote_remain_credits--;
    }

    rfc_check_send_credits();
    return true;
}

bool rfc_data_cfm(uint8_t bd_addr[6],
                  uint8_t dlci,
                  uint8_t rsp_num)
{
    T_RFC_CHANN *p_chann = rfc_find_chann_by_addr(bd_addr, dlci);

    if (!p_chann)
    {
        return false;
    }

    if (p_chann->convergence_layer == RFC_CONVERGENCE_CREDIT_CFM &&
        p_chann->state != DLCI_DISCONNECTING)
    {
        p_chann->given_credits += rsp_num;

        RFCOMM_PRINT_TRACE2("rfc_data_cfm: dlci 0x%02x, given credits %d",
                            p_chann->dlci, p_chann->given_credits);
    }

    rfc_check_send_credits();

    return true;
}

bool rfc_get_cid(uint8_t   bd_addr[6],
                 uint8_t   dlci,
                 uint16_t *p_cid)
{
    T_RFC_CHANN *p_chann;

    p_chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (p_chann == NULL)
    {
        return false;
    }

    *p_cid  = p_chann->p_ctrl_chann->l2c_cid;

    return true;
}

bool rfc_get_roleswap_info(uint8_t                   bd_addr[6],
                           uint8_t                   dlci,
                           T_ROLESWAP_RFC_DATA_INFO *p_data,
                           T_ROLESWAP_RFC_CTRL_INFO *p_ctrl)
{
    T_RFC_CHANN *p_data_chann;
    T_RFC_CHANN *p_ctrl_chann;

    p_data_chann = rfc_find_chann_by_addr(bd_addr, dlci);
    if (p_data_chann == NULL)
    {
        return false;
    }

    p_ctrl_chann = p_data_chann->p_ctrl_chann;

    p_data->dlci          = p_data_chann->dlci;
    p_data->l2c_cid       = p_ctrl_chann->l2c_cid;
    p_data->frame_size    = p_data_chann->frame_size;
    p_data->init_credits  = p_data_chann->init_credits;
    p_data->remote_remain_credits = p_data_chann->remote_remain_credits;
    p_data->given_credits = p_data_chann->given_credits;

    if (p_ctrl)
    {
        p_ctrl->initiator      = p_ctrl_chann->initiator;
        p_ctrl->link_initiator = p_ctrl_chann->link_initiator;
        p_ctrl->l2c_cid        = p_ctrl_chann->l2c_cid;
        p_ctrl->mtu_size       = p_ctrl_chann->mtu_size;
        p_ctrl->data_offset    = p_rfc->ds_offset;
    }

    return true;
}

bool rfc_set_ctrl_roleswap_info(uint8_t                   bd_addr[6],
                                T_ROLESWAP_RFC_CTRL_INFO *p_info)
{
    T_RFC_CHANN *p_ctrl_chann;

    p_ctrl_chann = rfc_find_chann_by_addr(bd_addr, 0);
    if (p_ctrl_chann == NULL)
    {
        p_ctrl_chann = rfc_alloc_chann(0);
    }

    if (p_ctrl_chann == NULL)
    {
        RFCOMM_PRINT_ERROR0("rfc_set_ctrl_roleswap_info: fail to alloc ctrl chann");
        return false;
    }

    p_ctrl_chann->p_ctrl_chann   = p_ctrl_chann;
    p_ctrl_chann->initiator      = p_info->initiator;
    p_ctrl_chann->state          = DLCI_CONNECTED;
    p_ctrl_chann->link_state     = LINK_CONNECTED;
    p_ctrl_chann->l2c_cid        = p_info->l2c_cid;
    p_ctrl_chann->mtu_size       = p_info->mtu_size;
    p_ctrl_chann->link_initiator = p_info->link_initiator;
    p_ctrl_chann->msc_handshake  = RFC_MSC_CMD_RCV | RFC_MSC_RSP_RCV;
    memcpy(p_ctrl_chann->bd_addr, bd_addr, 6);

    p_rfc->ds_offset = p_info->data_offset;

    return true;
}

bool rfc_set_data_roleswap_info(uint8_t                   bd_addr[6],
                                uint8_t                   profile_idx,
                                T_ROLESWAP_RFC_DATA_INFO *p_info)

{
    T_RFC_CHANN *p_ctrl_chann;
    T_RFC_CHANN *p_data_chann;

    p_ctrl_chann = rfc_find_chann_by_addr(bd_addr, 0);
    if (p_ctrl_chann == NULL)
    {
        RFCOMM_PRINT_ERROR0("rfc_set_data_roleswap_info: fail to find ctrl chann");
        return false;
    }

    p_data_chann = rfc_find_chann_by_dlci(p_info->dlci, p_info->l2c_cid);
    if (p_data_chann == NULL)
    {
        p_data_chann = rfc_alloc_chann(p_info->dlci);
    }

    if (p_data_chann == NULL)
    {
        RFCOMM_PRINT_ERROR0("rfc_set_data_roleswap_info: fail to alloc data chann");
        return false;
    }

    p_data_chann->p_ctrl_chann      = p_ctrl_chann;
    p_data_chann->state             = DLCI_CONNECTED;
    p_data_chann->convergence_layer = RFC_CONVERGENCE_CREDIT_CFM;
    p_data_chann->frame_size        = p_info->frame_size;
    p_data_chann->init_credits      = p_info->init_credits;
    p_data_chann->profile_index     = profile_idx;
    p_data_chann->remote_remain_credits = p_info->remote_remain_credits;
    p_data_chann->given_credits     = p_info->given_credits;

    return true;
}

bool rfc_del_ctrl_roleswap_info(uint8_t bd_addr[6])
{
    T_RFC_CHANN *p_ctrl_chann;

    p_ctrl_chann = rfc_find_chann_by_addr(bd_addr, 0);
    if (p_ctrl_chann == NULL)
    {
        return false;
    }

    rfc_free_chann(p_ctrl_chann, RFC_SUCCESS);
    return true;
}

bool rfc_del_data_roleswap_info(uint8_t  dlci,
                                uint16_t cid)
{
    T_RFC_CHANN *p_data_chann;

    p_data_chann = rfc_find_chann_by_dlci(dlci, cid);
    if (p_data_chann == NULL)
    {
        return false;
    }

    rfc_free_chann(p_data_chann, RFC_SUCCESS);
    return true;
}

