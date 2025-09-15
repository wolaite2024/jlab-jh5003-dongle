/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if (CONFIG_REALTEK_BTM_A2DP_SUPPORT == 1)
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "avdtp.h"
#include "sys_timer.h"

#define MAX_CODEC_INFO_SIZE                 12

#define AVDTP_SIG_MTU                       675
#define AVDTP_STREAM_MTU                    675
#define AVDTP_LDAC_STREAM_MTU               679

#define HDL_TYPE_CHECK_CODEC                0x03
#define HDL_TYPE_FILL_CODEC_INFO            0x04

#define AVDTP_SEP_INFO_SIZE                 2

#define AVDTP_VERSON_V1_3                   0x0103

#define AVDTP_MSG_TYPE_CMD                  0x00
#define AVDTP_MSG_TYPE_GENERAL_REJECT       0x01
#define AVDTP_MSG_TYPE_RSP_ACCEPT           0x02
#define AVDTP_MSG_TYPE_RSP_REJECT           0x03

#define AVDTP_PKT_TYPE_SINGLE               0x00
#define AVDTP_PKT_TYPE_START                0x01
#define AVDTP_PKT_TYPE_CONTINUE             0x02
#define AVDTP_PKT_TYPE_END                  0x03
#define AVDTP_PKT_TYPE_MASK                 0x0C

#define AVDTP_DISCOVER                      0x01
#define AVDTP_GET_CPBS                      0x02
#define AVDTP_SET_CFG                       0x03
#define AVDTP_GET_CFG                       0x04
#define AVDTP_RECFG                         0x05
#define AVDTP_OPEN                          0x06
#define AVDTP_START                         0x07
#define AVDTP_CLOSE                         0x08
#define AVDTP_SUSPEND                       0x09
#define AVDTP_ABORT                         0x0A
#define AVDTP_SECURITY_CONTROL              0x0B
#define AVDTP_GET_ALL_CPBS                  0x0C
#define AVDTP_DELAY_REPORT                  0x0D

#define CODEC_TYPE_SBC                      0x00
#define CODEC_TYPE_AAC                      0x02
#define CODEC_TYPE_USAC                     0x03
#define CODEC_TYPE_LDAC                     0xf0
#define CODEC_TYPE_LC3                      0xf1
#define CODEC_TYPE_LHDC                     0xf2

#define SRV_CATEG_MEDIA_TRANS               0x01
#define SRV_CATEG_REPORTING                 0x02
#define SRV_CATEG_RECOVERY                  0x03
#define SRV_CATEG_CP                        0x04
#define SRV_CATEG_HC                        0x05
#define SRV_CATEG_MULTIPLEXING              0x06
#define SRV_CATEG_MEDIA_CODEC               0x07
#define SRV_CATEG_DELAY_REPORT              0x08

#define CATEG_MIDIA_TRANS_MASK              0x01
#define CATEG_REPORTING_MASK                0x02
#define CATEG_RECOVERY_MASK                 0x04
#define CATEG_CP_MASK                       0x08
#define CATEG_HC_MASK                       0x10
#define CATEG_MULTIPLEXING_MASK             0x20
#define CATEG_MEDIA_CODEC_MASK              0x40
#define CATEG_DELAY_REPORT_MASK             0x80

#define MEDIA_TRANS_LOSC                    0x00
#define CP_LOSC                             0x02
#define MEDIA_CODEC_SBC_LOSC                0x06
#define MEDIA_CODEC_AAC_LOSC                0x08
#define MEDIA_CODEC_USAC_LOSC               0x09
#define MEDIA_CODEC_LDAC_LOSC               0x0A
#define MEDIA_CODEC_LC3_LOSC                0x0B
#define MEDIA_CODEC_LHDC_LOSC               0x0D
#define DELAY_REPORT_LOSC                   0x00

#define SEID_NOT_IN_USE                     0x00
#define SEID_IN_USE                         0x01

#define AVDTP_TSEP_SRC                      0
#define AVDTP_TSEP_SNK                      1

#define AVDTP_MEDIA_AUDIO                   0
#define AVDTP_MEDIA_VIDEO                   1
#define AVDTP_MEDIA_MULTI                   2

#define AUDIO_MEDIA_TYPE                    0x00

#define AUDIO_SRC                           0x00
#define AUDIO_SNK                           0x08

#define SBC_MEDIA_CODEC_TYPE                0x00
#define AAC_MEDIA_CODEC_TYPE                0x02
#define USAC_MEDIA_CODEC_TYPE               0x03
#define VENDOR_CODEC_TYPE                   0xff

#define CP_TYPE_LSB                         0x02
#define CP_TYPE_MSB                         0x00

#define BAD_HDR_FORMAT                      0x01
#define BAD_LENGTH                          0x11
#define BAD_ACP_SEID                        0x12
#define SEP_IN_USE                          0x13
#define SEP_NOT_IN_USE                      0x14
#define BAD_SERV_CATEG                      0x17
#define BAD_PAYLOAD_FORMAT                  0x18
#define NOT_SUPT_CMD                        0x19
#define INVALID_CPBS                        0x1A
#define BAD_RECOVERY_TYPE                   0x22
#define BAD_MEDIA_TRANSPORT_TYPE            0x23
#define BAD_RECOVERY_FORMAT                 0x25
#define BAD_ROHC_FORMAT                     0x26
#define BAD_CP_FORMAT                       0x27
#define BAD_MULTIPLEXING_FORMAT             0x28
#define UNSUPT_CFG                          0x29
#define BAD_STATE                           0x31

#define INVALID_CODEC_TYPE                      0xC1
#define NOT_SUPPORTED_CODEC_TYPE                0xC2
#define INVALID_SAMPLING_FREQUENCY              0xC3
#define NOT_SUPPORTED_SAMPLING_FREQUENCY        0xC4
#define INVALID_CHANNEL_MODE                    0xC5
#define NOT_SUPPORTED_CHANNEL_MODE              0xC6
#define INVALID_SUBBANDS                        0xC7
#define NOT_SUPPORTED_SUBBANDS                  0xC8
#define INVALID_ALLOCATION_METHOD               0xC9
#define NOT_SUPPORTED_ALLOCATION_METHOD         0xCA
#define INVALID_MINIMUM_BITPOOL_VALUE           0xCB
#define NOT_SUPPORTED_MINIMUM_BITPOOL_VALUE     0xCC
#define INVALID_MAXIMUM_BITPOOL_VALUE           0xCD
#define NOT_SUPPORTED_MAXIMUM_BITPOOL_VALUE     0xCE
#define INVALID_LAYER                           0xCF
#define NOT_SUPPORTED_LAYER                     0xD0
#define NOT_SUPPORTED_CRC                       0xD1
#define NOT_SUPPORTED_MPF                       0xD2
#define NOT_SUPPORTED_VBR                       0xD3
#define INVALID_BIT_RATE                        0xD4
#define NOT_SUPPORTED_BIT_RATE                  0xD5
#define INVALID_OBJECT_TYPE                     0xD6
#define NOT_SUPPORTED_OBJECT_TYPE               0xD7
#define INVALID_CHANNELS                        0xD8
#define NOT_SUPPORTED_CHANNELS                  0xD9
#define INVALID_VERSION                         0xDA
#define NOT_SUPPORTED_VERSION                   0xDB
#define NOT_SUPPORTED_MAXIMUM_SUL               0xDC
#define INVALID_BLOCK_LENGTH                    0xDD
#define INVALID_CP_TYPE                         0xE0
#define INVALID_CP_FORMAT                       0xE1
#define INVALID_CODEC_PARAMETER                 0xE2
#define NOT_SUPPORTED_CODEC_PARAMETER           0xE3
#define INVALID_DRC                             0xE4
#define NOT_SUPPORTED_DRC                       0xE5

typedef struct t_avdtp_start_pkt_hdr
{
    uint8_t trans_info; /* include Transaction label , packet type, Message type */
    uint8_t pkt_num;
    uint8_t signal_id;
} T_AVDTP_START_PKT_HDR;

typedef struct t_avdtp_contend_pkt_hdr
{
    uint8_t trans_info; /* include Transaction label , packet type, Message type */
} T_AVDTP_CONTINUE_PKT_HDR;

typedef struct t_avdtp_start_pkt
{
    T_AVDTP_START_PKT_HDR pkt_hdr;
    uint8_t               pkt_data[1];
} T_AVDTP_START_PKT;

typedef struct t_avdtp_continue_pkt
{
    T_AVDTP_CONTINUE_PKT_HDR pkt_hdr;
    uint8_t                  pkt_data[1];
} T_AVDTP_CONTINUE_PKT;

typedef struct t_avdtp_sig_hdr
{
    uint8_t trans_label: 4;
    uint8_t pkt_type: 2;
    uint8_t block_len: 2;
    uint8_t signal_id;
} T_AVDTP_SIG_HDR;

typedef enum t_avdtp_state
{
    AVDTP_STATE_DISCONNECTED        = 0x00,
    AVDTP_STATE_CONNECTING          = 0x01,
    AVDTP_STATE_CONNECTED           = 0x02,
    AVDTP_STATE_DISCONNECTING       = 0x03,
} T_AVDTP_STATE;

typedef enum t_avdtp_sig_state
{
    AVDTP_SIG_STATE_IDLE      = 0x00,
    AVDTP_SIG_STATE_CFG       = 0x01,
    AVDTP_SIG_STATE_OPENING   = 0x02,
    AVDTP_SIG_STATE_OPEN      = 0x03,
    AVDTP_SIG_STATE_STREAMING = 0x04,
    AVDTP_SIG_STATE_CLOSING   = 0x05,
    AVDTP_SIG_STATE_ABORTING  = 0x06,
} T_AVDTP_SIG_STATE;

typedef struct t_avdtp_remote_sep
{
    uint8_t sep_id;
    uint8_t cpbs_mask;
    uint8_t media_type;
    uint8_t media_codec_type;
    uint8_t media_codec_subtype;
    uint8_t media_codec_info[MAX_CODEC_INFO_SIZE];
    uint8_t cpbs_order[8];
} T_AVDTP_REMOTE_SEP;

typedef struct t_avdtp_local_sep
{
    struct t_avdtp_local_sep *next;
    uint8_t                   sep_id;
    bool                      in_use;
    uint8_t                   media_type;
    uint8_t                   codec_type;
    uint8_t                   sub_codec_type;
    uint8_t                   tsep;
    uint8_t                   media_codec_info[MAX_CODEC_INFO_SIZE];
} T_AVDTP_LOCAL_SEP;

typedef struct t_sig_chann
{
    T_AVDTP_REMOTE_SEP *avdtp_sep;
    uint8_t            *fragment_data;
    uint8_t            *cfg_cpbs;
    uint16_t            cid;
    uint16_t            fragment_data_len;
    uint16_t            remote_mtu;
    T_AVDTP_STATE       state;
    T_AVDTP_SIG_STATE   sig_state;
    uint8_t             int_flag;
    uint8_t             int_seid;
    uint8_t             acp_seid_idx;
    uint8_t             acp_seid;
    uint8_t             cpbs_len;
    uint8_t             codec_type;
    uint8_t             codec_subtype;
    uint8_t             recfg_codec_type;
    uint8_t             enable_cp_flag;
    uint8_t             delay_report_flag;
    uint8_t             tx_trans_label;
    uint8_t             rx_start_trans_label;
    uint8_t             acp_sep_no;
    uint8_t             codec_info[MAX_CODEC_INFO_SIZE];
    uint8_t             cmd_flag;
} T_SIG_CHANN;

typedef struct t_strm_chann
{
    uint16_t      cid;
    uint16_t      mtu_size;
    T_AVDTP_STATE state;
} T_STRM_CHANN;

typedef struct t_avdtp_link
{
    struct t_avdtp_link *next;
    uint8_t              bd_addr[6];
    uint16_t             avdtp_ver;
    T_SIG_CHANN          sig_chann;
    T_STRM_CHANN         strm_chann;
    uint8_t              conn_role;
    uint8_t              local_sep_role;
    T_SYS_TIMER_HANDLE   timer_handle;
} T_AVDTP_LINK;

typedef struct t_avdtp
{
    T_OS_QUEUE    link_list;
    P_AVDTP_CBACK cback;
    T_OS_QUEUE    local_sep_list;
    uint8_t       local_sep_num;
    uint8_t       queue_id;
    uint16_t      data_offset;
    uint16_t      sig_timer;
    uint8_t       service_category;
} T_AVDTP;

static T_AVDTP *avdtp;

static const uint8_t avdtp_lc3_codec_info[6] =
{
    0x5d, 0x00, 0x00, 0x00, 0x08, 0x00
};

static const uint8_t avdtp_ldac_codec_info[6] =
{
    0x2d, 0x01, 0x00, 0x00, 0xaa, 0x00
};

static const uint8_t avdtp_lhdc_v5_codec_info[6] =
{
    0x3a, 0x05, 0x00, 0x00, 0x35, 0x4c
};

void avdtp_tout_cback(T_SYS_TIMER_HANDLE handle)
{
    T_AVDTP_LINK  *link;

    link = (void *)sys_timer_id_get(handle);
    if (link != NULL)
    {
        PROTOCOL_PRINT_WARN2("avdtp_tout_cback: bd_addr %s, cmd %x",
                             TRACE_BDADDR(link->bd_addr), link->sig_chann.cmd_flag);

        switch (link->sig_chann.cmd_flag)
        {
        case AVDTP_DISCOVER:
        case AVDTP_GET_CPBS:
        case AVDTP_GET_ALL_CPBS:
        case AVDTP_SET_CFG:
            if (link->sig_chann.state == AVDTP_STATE_CONNECTED)
            {
                avdtp_signal_disconnect_req(link->bd_addr);
            }
            break;

        case AVDTP_OPEN:
        case AVDTP_START:
        case AVDTP_CLOSE:
            avdtp_signal_abort_req(link->bd_addr);
            break;

        case AVDTP_ABORT:
            if (link->strm_chann.state == AVDTP_STATE_CONNECTED)
            {
                avdtp_stream_disconnect_req(link->bd_addr);
            }

            avdtp_signal_disconnect_req(link->bd_addr);
            break;

        default:
            break;
        }
    }
}

T_AVDTP_LINK *avdtp_alloc_link(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = calloc(1, sizeof(T_AVDTP_LINK));
    if (link != NULL)
    {
        link->timer_handle = sys_timer_create("avdtp_sig_timer",
                                              SYS_TIMER_TYPE_LOW_PRECISION,
                                              (uint32_t)link,
                                              avdtp->sig_timer * 1000,
                                              false,
                                              avdtp_tout_cback);
        if (link->timer_handle != NULL)
        {
            memcpy(link->bd_addr, bd_addr, 6);
            os_queue_in(&avdtp->link_list, link);
        }
        else
        {
            free(link);
            link = NULL;
        }
    }

    return link;
}

void avdtp_free_link(T_AVDTP_LINK *link)
{
    if (os_queue_delete(&avdtp->link_list, link) == true)
    {
        if (link->sig_chann.cfg_cpbs != NULL)
        {
            free(link->sig_chann.cfg_cpbs);
        }

        if (link->sig_chann.avdtp_sep != NULL)
        {
            free(link->sig_chann.avdtp_sep);
        }

        if (link->sig_chann.fragment_data != NULL)
        {
            free(link->sig_chann.fragment_data);
        }

        if (link->timer_handle != NULL)
        {
            sys_timer_delete(link->timer_handle);
        }

        free(link);
    }
}

T_AVDTP_LINK *avdtp_find_link_by_addr(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = os_queue_peek(&avdtp->link_list, 0);
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

T_AVDTP_LINK *avdtp_find_link_by_cid(uint16_t cid)
{
    T_AVDTP_LINK *link;

    link = os_queue_peek(&avdtp->link_list, 0);
    while (link != NULL)
    {
        if ((link->sig_chann.cid == cid) || (link->strm_chann.cid == cid))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

T_AVDTP_LOCAL_SEP *avdtp_find_sep_by_id(uint8_t id)
{
    T_AVDTP_LOCAL_SEP *sep;

    sep = os_queue_peek(&avdtp->local_sep_list, 0);
    while (sep != NULL)
    {
        if (sep->sep_id == id)
        {
            break;
        }

        sep = sep->next;
    }

    return sep;
}

bool avdtp_vendor_codec_ldac_handler(uint8_t            role,
                                     uint8_t            type,
                                     uint8_t           *remote_codec_info,
                                     T_AVDTP_LOCAL_SEP *local_sep,
                                     uint8_t           *ret_buf)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&avdtp->local_sep_list, 0);
        while (sep != NULL)
        {
            if (sep->sub_codec_type == CODEC_TYPE_LDAC)
            {
                if (!memcmp(remote_codec_info, &sep->media_codec_info[0], 6) &&
                    sep->tsep != role)
                {
                    status = true;
                    break;
                }
            }
            sep = sep->next;
        }
    }
    else if (type == HDL_TYPE_FILL_CODEC_INFO)
    {
        ret_buf[0] = MEDIA_CODEC_LDAC_LOSC;
        ret_buf[1] = AUDIO_MEDIA_TYPE << 4;
        ret_buf[2] = VENDOR_CODEC_TYPE;
        memcpy(&ret_buf[3], &local_sep->media_codec_info[0], 6);

        /* sample frequency */
        if (remote_codec_info[6] & 0x01 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x01;/* 192K */
        }
        else if (remote_codec_info[6] & 0x02 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x02;/* 176.4K */
        }
        else if (remote_codec_info[6] & 0x04 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x04;/* 96K */
        }
        else if (remote_codec_info[6] & 0x08 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x08;/* 88.2K */
        }
        else if (remote_codec_info[6] & 0x10 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x10;/* 48K */
        }
        else if (remote_codec_info[6] & 0x20 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x20;/* 44.1K */
        }

        /* channel mode */
        if (remote_codec_info[7] & 0x01 & local_sep->media_codec_info[7])
        {
            ret_buf[10] = 0x01;/* stereo */
        }
        else if (remote_codec_info[7] & 0x02 & local_sep->media_codec_info[7])
        {
            ret_buf[10] = 0x02;/* dual channel */
        }
        else if (remote_codec_info[7] & 0x04 & local_sep->media_codec_info[7])
        {
            ret_buf[10] = 0x04;/* mono */
        }

        status = true;
    }

    return status;
}

bool avdtp_vendor_codec_lc3_handler(uint8_t            role,
                                    uint8_t            type,
                                    uint8_t           *remote_codec_info,
                                    T_AVDTP_LOCAL_SEP *local_sep,
                                    uint8_t           *ret_buf)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&avdtp->local_sep_list, 0);
        while (sep != NULL)
        {
            if (sep->sub_codec_type == CODEC_TYPE_LC3)
            {
                if (!memcmp(remote_codec_info, &sep->media_codec_info[0], 6) &&
                    sep->tsep != role)
                {
                    status = true;
                    break;
                }
            }
            sep = sep->next;
        }
    }
    else if (type == HDL_TYPE_FILL_CODEC_INFO)
    {
        uint16_t local_frame_len;
        uint16_t remote_frame_len;

        ret_buf[0] = MEDIA_CODEC_LC3_LOSC;
        ret_buf[1] = AUDIO_MEDIA_TYPE << 4;
        ret_buf[2] = VENDOR_CODEC_TYPE;
        memcpy(&ret_buf[3], &local_sep->media_codec_info[0], 6);

        /* sample frequency */
        if (remote_codec_info[6] & 0x04 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x04;/* 48K */
        }
        else if (remote_codec_info[6] & 0x08 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x08;/* 44.1K */
        }
        else if (remote_codec_info[6] & 0x10 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x10;/* 32K */
        }
        else if (remote_codec_info[6] & 0x20 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x20;/* 24K */
        }
        else if (remote_codec_info[6] & 0x40 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x40;/* 16K */
        }
        else if (remote_codec_info[6] & 0x80 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x80;/* 8K */
        }

        /* channel number */
        if (remote_codec_info[6] & 0x01 & local_sep->media_codec_info[6])
        {
            ret_buf[9] |= 0x01; /* 2 channel */
        }
        else if (remote_codec_info[6] & 0x02 & local_sep->media_codec_info[6])
        {
            ret_buf[9] |= 0x02; /* 1 channel */
        }

        /* frame duration */
        if (remote_codec_info[7] & 0x02 & local_sep->media_codec_info[7])
        {
            ret_buf[10] = 0x02; /* 10ms */
        }
        else if (remote_codec_info[7] & 0x04 & local_sep->media_codec_info[7])
        {
            ret_buf[10] = 0x04; /* 7.5ms */
        }

        /* frame length */
        local_frame_len = ((local_sep->media_codec_info[7] & 0x01) << 8) +
                          local_sep->media_codec_info[8];
        remote_frame_len = ((remote_codec_info[7] & 0x01) << 8) + remote_codec_info[8];
        if (local_frame_len > remote_frame_len)
        {
            ret_buf[10] |= remote_codec_info[7] & 0x01;
            ret_buf[11]  = remote_codec_info[8];
        }
        else
        {
            ret_buf[10] |= local_sep->media_codec_info[7] & 0x01;
            ret_buf[11]  = local_sep->media_codec_info[8];
        }

        status = true;
    }

    return status;
}

bool avdtp_vendor_codec_lhdc_handler(uint8_t             role,
                                     uint8_t             type,
                                     uint8_t            *remote_codec_info,
                                     T_AVDTP_LOCAL_SEP  *local_sep,
                                     uint8_t            *ret_buf)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&avdtp->local_sep_list, 0);
        while (sep != NULL)
        {
            if (sep->sub_codec_type == CODEC_TYPE_LHDC)
            {
                if (!memcmp(remote_codec_info, &sep->media_codec_info[0], 6) &&
                    sep->tsep != role)
                {
                    status = true;
                    break;
                }
            }
            sep = sep->next;
        }
    }
    else if (type == HDL_TYPE_FILL_CODEC_INFO)
    {
        ret_buf[0] = MEDIA_CODEC_LHDC_LOSC;
        ret_buf[1] = AUDIO_MEDIA_TYPE << 4;
        ret_buf[2] = VENDOR_CODEC_TYPE;
        memcpy(&ret_buf[3], &local_sep->media_codec_info[0], 6);

        /* sample frequency */
        if (remote_codec_info[6] & 0x01 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x01; /* 192K */
        }
        else if (remote_codec_info[6] & 0x04 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x04; /* 96K */
        }
        else if (remote_codec_info[6] & 0x10 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x10; /* 48K */
        }
        else if (remote_codec_info[6] & 0x20 & local_sep->media_codec_info[6])
        {
            ret_buf[9] = 0x20; /* 44.1K */
        }

        /* min bitrate */
        if ((remote_codec_info[7] & 0xc0) < (local_sep->media_codec_info[7] & 0xc0))
        {
            ret_buf[10] = local_sep->media_codec_info[7] & 0xc0;
        }
        else
        {
            ret_buf[10] = remote_codec_info[7] & 0xc0;
        }

        /* max bitrate */
        if ((remote_codec_info[7] & 0x30) == 0)
        {
            ret_buf[10] |= local_sep->media_codec_info[7] & 0x30;
        }
        else if ((local_sep->media_codec_info[7] & 0x30) == 0)
        {
            ret_buf[10] |= remote_codec_info[7] & 0x30;
        }
        else if ((remote_codec_info[7] & 0x30) > (local_sep->media_codec_info[7] & 0x30))
        {
            ret_buf[10] |= local_sep->media_codec_info[7] & 0x30;
        }
        else
        {
            ret_buf[10] |= remote_codec_info[7] & 0x30;
        }

        /* bit depth */
        if (remote_codec_info[7] & 0x01 & local_sep->media_codec_info[7])
        {
            ret_buf[10] |= 0x01; /* 32bit */
        }
        else if (remote_codec_info[7] & 0x02 & local_sep->media_codec_info[7])
        {
            ret_buf[10] |= 0x02; /* 24bit */
        }
        else if (remote_codec_info[7] & 0x04 & local_sep->media_codec_info[7])
        {
            ret_buf[10] |= 0x04; /* 16bit */
        }

        /* version number */
        if (remote_codec_info[8] & 0x01 & local_sep->media_codec_info[8])
        {
            ret_buf[11] |= 0x01;  /* v5.0 */
        }
        else if (remote_codec_info[8] & 0x02 & local_sep->media_codec_info[8])
        {
            ret_buf[11] |= 0x02;  /* v5.1 */
        }
        else if (remote_codec_info[8] & 0x04 & local_sep->media_codec_info[8])
        {
            ret_buf[11] |= 0x04;  /* v5.2 */
        }
        else if (remote_codec_info[8] & 0x08 & local_sep->media_codec_info[8])
        {
            ret_buf[11] |= 0x08;  /* v5.3 */
        }

        /* ll */
        if (remote_codec_info[9] & 0x40 & local_sep->media_codec_info[9])
        {
            ret_buf[12] |= 0x40;
        }

        /* meta */
        if (remote_codec_info[9] & 0x04 & local_sep->media_codec_info[9])
        {
            ret_buf[12] |= 0x04;
        }

        /* jas */
        if (remote_codec_info[9] & 0x02 & local_sep->media_codec_info[9])
        {
            ret_buf[12] |= 0x02;
        }

        /* ar */
        if (remote_codec_info[9] & 0x01 & local_sep->media_codec_info[9])
        {
            ret_buf[12] |= 0x01;
        }

        status = true;
    }

    return status;
}

bool avdtp_codec_usac_handler(uint8_t            role,
                              uint8_t            type,
                              uint8_t           *remote_codec_info,
                              T_AVDTP_LOCAL_SEP *local_sep,
                              uint8_t           *ret_buf)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&avdtp->local_sep_list, 0);
        while (sep != NULL)
        {
            if (sep->sub_codec_type == CODEC_TYPE_USAC)
            {
                if (sep->tsep != role)
                {
                    status = true;
                    break;
                }
            }
            sep = sep->next;
        }
    }
    else if (type == HDL_TYPE_FILL_CODEC_INFO)
    {
        uint32_t sampling_frequency;
        uint32_t local_sampling_frequency;
        uint32_t remote_sampling_frequency;
        uint32_t local_bit_rate;
        uint32_t remote_bit_rate;
        uint8_t  m;

        ret_buf[0] = MEDIA_CODEC_USAC_LOSC;
        ret_buf[1] = AUDIO_MEDIA_TYPE << 4;
        ret_buf[2] = USAC_MEDIA_CODEC_TYPE;

        /* object type */
        ret_buf[3] = 0x80;

        /* sampling frequency */
        local_sampling_frequency = ((local_sep->media_codec_info[0] & 0x3f) << 24) +
                                   (local_sep->media_codec_info[1] << 16) +
                                   (local_sep->media_codec_info[2] << 8) +
                                   (local_sep->media_codec_info[3] & 0xf0);
        remote_sampling_frequency = ((remote_codec_info[0] & 0x3f) << 24) +
                                    (remote_codec_info[1] << 16) +
                                    (remote_codec_info[2] << 8) +
                                    (remote_codec_info[3] & 0xf0);
        for (m = 0; m < 26; m++)
        {
            sampling_frequency = 1 << (4 + m);
            if ((remote_sampling_frequency & sampling_frequency)
                && (local_sampling_frequency & sampling_frequency))
            {
                break;
            }
        }

        if (m == 26)
        {
            sampling_frequency = 1 << 10; /* 48K */
        }

        ret_buf[3] |= (uint8_t)(sampling_frequency >> 24);
        ret_buf[4]  = (uint8_t)(sampling_frequency >> 16);
        ret_buf[5]  = (uint8_t)(sampling_frequency >> 8);
        ret_buf[6]  = (uint8_t)sampling_frequency;

        /* channel setting */
        if ((remote_codec_info[3] & 0x04) && (local_sep->media_codec_info[3] & 0x04))
        {
            ret_buf[6] |= 0x04; /* stereo */
        }
        else if ((remote_codec_info[3] & 0x08) && (local_sep->media_codec_info[3] & 0x08))
        {
            ret_buf[6] |= 0x08; /* mono */
        }
        else
        {
            ret_buf[6] |= 0x08;
        }

        /* VBR */
        if ((remote_codec_info[4] & 0x80) && (local_sep->media_codec_info[4] & 0x80))
        {
            ret_buf[7] = 0x80;
        }
        else
        {
            ret_buf[7] = 0x00;
        }

        /* Bit rate */
        local_bit_rate = ((local_sep->media_codec_info[4] & 0x7F) << 16) +
                         (local_sep->media_codec_info[5] << 8) +
                         local_sep->media_codec_info[6];
        remote_bit_rate = ((remote_codec_info[4] & 0x7F) << 16) +
                          (remote_codec_info[5] << 8) +
                          remote_codec_info[6];
        if (local_bit_rate > remote_bit_rate)
        {
            ret_buf[7] |= remote_codec_info[4] & 0x7F;
            ret_buf[8]  = remote_codec_info[5];
            ret_buf[9]  = remote_codec_info[6];
        }
        else
        {
            ret_buf[7] |= local_sep->media_codec_info[4] & 0x7F;
            ret_buf[8]  = local_sep->media_codec_info[5];
            ret_buf[9]  = local_sep->media_codec_info[6];
        }

        status = true;
    }

    return status;
}

bool avdtp_codec_aac_handler(uint8_t            role,
                             uint8_t            type,
                             uint8_t           *remote_codec_info,
                             T_AVDTP_LOCAL_SEP *local_sep,
                             uint8_t           *ret_buf)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&avdtp->local_sep_list, 0);
        while (sep != NULL)
        {
            if (sep->sub_codec_type == CODEC_TYPE_AAC)
            {
                if (sep->tsep != role)
                {
                    status = true;
                    break;
                }
            }
            sep = sep->next;
        }
    }
    else if (type == HDL_TYPE_FILL_CODEC_INFO)
    {
        uint32_t local_bit_rate;
        uint32_t remote_bit_rate;

        ret_buf[0] = MEDIA_CODEC_AAC_LOSC;
        ret_buf[1] = AUDIO_MEDIA_TYPE << 4;
        ret_buf[2] = AAC_MEDIA_CODEC_TYPE;

        /* object type */
        if ((remote_codec_info[0] & 0x80) && (local_sep->media_codec_info[0] & 0x80))
        {
            ret_buf[3] = 0x80;/* MPEG-2 AAC LC */
        }
        else if ((remote_codec_info[0] & 0x40) && (local_sep->media_codec_info[0] & 0x40))
        {
            ret_buf[3] = 0x40;/* MPEG-4 AAC LC */
        }
        else if ((remote_codec_info[0] & 0x20) && (local_sep->media_codec_info[0] & 0x20))
        {
            ret_buf[3] = 0x20;/* MPEG-4 AAC LTP */
        }
        else if ((remote_codec_info[0] & 0x10) && (local_sep->media_codec_info[0] & 0x10))
        {
            ret_buf[3] = 0x10;/* MPEG-4 AAC scalable */
        }
        else if ((remote_codec_info[0] & 0x08) && (local_sep->media_codec_info[0] & 0x08))
        {
            ret_buf[3] = 0x08;/* MPEG-4 HE-AAC */
        }
        else if ((remote_codec_info[0] & 0x04) && (local_sep->media_codec_info[0] & 0x04))
        {
            ret_buf[3] = 0x04;/* MPEG-4 HE-AACv2 */
        }
        else if ((remote_codec_info[0] & 0x02) && (local_sep->media_codec_info[0] & 0x02))
        {
            ret_buf[3] = 0x02;/* MPEG-4 AAC-ELDv2 */
        }
        else if ((remote_codec_info[0] & 0x01) && (local_sep->media_codec_info[0] & 0x01))
        {
            ret_buf[3] = 0x01;/* MPEG-D DRC */
        }
        else
        {
            ret_buf[3] = 0x80;
        }

        /* channel setting */
        if ((remote_codec_info[2] & 0x04) && (local_sep->media_codec_info[2] & 0x04))
        {
            ret_buf[5] = 0x04; /* stereo */
        }
        else if ((remote_codec_info[2] & 0x08) && (local_sep->media_codec_info[2] & 0x08))
        {
            ret_buf[5] = 0x08; /* mono */
        }
        else
        {
            ret_buf[5] = 0x08;
        }

        /* sampling frequency */
        if ((remote_codec_info[1] & 0x01) && (local_sep->media_codec_info[1] & 0x01))
        {
            ret_buf[4] = 0x01; /* 44.1K */
        }
        else if ((remote_codec_info[1] & 0x02) && (local_sep->media_codec_info[1] & 0x02))
        {
            ret_buf[4] = 0x02; /* 32K */
        }
        else if ((remote_codec_info[1] & 0x04) && (local_sep->media_codec_info[1] & 0x04))
        {
            ret_buf[4] = 0x04; /* 24K */
        }
        else if ((remote_codec_info[1] & 0x08) && (local_sep->media_codec_info[1] & 0x08))
        {
            ret_buf[4] = 0x08; /* 22.05K */
        }
        else if ((remote_codec_info[1] & 0x10) && (local_sep->media_codec_info[1] & 0x10))
        {
            ret_buf[4] = 0x10; /* 16K */
        }
        else if ((remote_codec_info[1] & 0x20) && (local_sep->media_codec_info[1] & 0x20))
        {
            ret_buf[4] = 0x20; /* 12K */
        }
        else if ((remote_codec_info[1] & 0x40) && (local_sep->media_codec_info[1] & 0x40))
        {
            ret_buf[4] = 0x40; /* 11.025K */
        }
        else if ((remote_codec_info[1] & 0x80) && (local_sep->media_codec_info[1] & 0x80))
        {
            ret_buf[4] = 0x80; /* 8K */
        }
        else if ((remote_codec_info[2] & 0x80) && (local_sep->media_codec_info[2] & 0x80))
        {
            ret_buf[4] = 0x00;
            ret_buf[5] |= 0x80; /* 48K */
        }
        else if ((remote_codec_info[2] & 0x40) && (local_sep->media_codec_info[2] & 0x40))
        {
            ret_buf[4] = 0x00;
            ret_buf[5] |= 0x40; /* 64K */
        }
        else if ((remote_codec_info[2] & 0x20) && (local_sep->media_codec_info[2] & 0x20))
        {
            ret_buf[4] = 0x00;
            ret_buf[5] |= 0x20; /* 88.2K */
        }
        else if ((remote_codec_info[2] & 0x10) && (local_sep->media_codec_info[2] & 0x10))
        {
            ret_buf[4] = 0x00;
            ret_buf[5] |= 0x10; /* 96K */
        }
        else
        {
            ret_buf[4] = 0x00;
            ret_buf[5] |= 0x80;
        }

        /* VBR */
        if ((remote_codec_info[3] & 0x80) && (local_sep->media_codec_info[3] & 0x80))
        {
            ret_buf[6] = 0x80;
        }
        else
        {
            ret_buf[6] = 0x00;
        }

        /* Bit rate */
        local_bit_rate = ((local_sep->media_codec_info[3] & 0x7F) << 16) +
                         (local_sep->media_codec_info[4] << 8) +
                         local_sep->media_codec_info[5];
        remote_bit_rate = ((remote_codec_info[3] & 0x7F) << 16) +
                          (remote_codec_info[4] << 8) +
                          remote_codec_info[5];
        if (local_bit_rate > remote_bit_rate)
        {
            ret_buf[6] |= remote_codec_info[3] & 0x7F;
            ret_buf[7]  = remote_codec_info[4];
            ret_buf[8]  = remote_codec_info[5];
        }
        else
        {
            ret_buf[6] |= local_sep->media_codec_info[3] & 0x7F;
            ret_buf[7]  = local_sep->media_codec_info[4];
            ret_buf[8]  = local_sep->media_codec_info[5];
        }

        status = true;
    }

    return status;
}

bool avdtp_codec_sbc_handler(uint8_t            role,
                             uint8_t            type,
                             uint8_t           *remote_codec_info,
                             T_AVDTP_LOCAL_SEP *local_sep,
                             uint8_t           *ret_buf)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&avdtp->local_sep_list, 0);
        while (sep != NULL)
        {
            if (sep->sub_codec_type == CODEC_TYPE_SBC)
            {
                if (sep->tsep != role)
                {
                    status = true;
                    break;
                }
            }
            sep = sep->next;
        }
    }
    else if (type == HDL_TYPE_FILL_CODEC_INFO)
    {
        ret_buf[0] = MEDIA_CODEC_SBC_LOSC;
        ret_buf[1] = AUDIO_MEDIA_TYPE << 4;
        ret_buf[2] = SBC_MEDIA_CODEC_TYPE;

        /* sample frequency */
        if ((remote_codec_info[0] & 0x10) && (local_sep->media_codec_info[0] & 0x10)) /* 48KHz */
        {
            ret_buf[3] = 0x10;
        }
        else if ((remote_codec_info[0] & 0x20) && (local_sep->media_codec_info[0] & 0x20)) /* 44.1KHz */
        {
            ret_buf[3] = 0x20;
        }
        else if ((remote_codec_info[0] & 0x40) && (local_sep->media_codec_info[0] & 0x40)) /* 32KHz */
        {
            ret_buf[3] = 0x40;
        }
        else if ((remote_codec_info[0] & 0x80) && (local_sep->media_codec_info[0] & 0x80)) /* 16KHz */
        {
            ret_buf[3] = 0x80;
        }
        else
        {
            ret_buf[3] = 0x10;
        }

        /* channel mode */
        if ((remote_codec_info[0] & 0x01) && (local_sep->media_codec_info[0] & 0x01)) /* joint stereo */
        {
            ret_buf[3] |= 0x01;
        }
        else if ((remote_codec_info[0] & 0x02) && (local_sep->media_codec_info[0] & 0x02)) /* stereo */
        {
            ret_buf[3] |= 0x02;
        }
        else if ((remote_codec_info[0] & 0x04) &&
                 (local_sep->media_codec_info[0] & 0x04)) /* dual channel */
        {
            ret_buf[3] |= 0x04;
        }
        else if ((remote_codec_info[0] & 0x08) && (local_sep->media_codec_info[0] & 0x08)) /* mono */
        {
            ret_buf[3] |= 0x08;
        }
        else
        {
            ret_buf[3] |= 0x08;
        }

        /* block number */
        if ((remote_codec_info[1] & 0x10) && (local_sep->media_codec_info[1] & 0x10))/* block_no:16 */
        {
            ret_buf[4] = 0x10;
        }
        else if ((remote_codec_info[1] & 0x20) && (local_sep->media_codec_info[1] & 0x20))/* block_no:12 */
        {
            ret_buf[4] = 0x20;
        }
        else if ((remote_codec_info[1] & 0x40) && (local_sep->media_codec_info[1] & 0x40))/* block_no:8 */
        {
            ret_buf[4] = 0x40;
        }
        else if ((remote_codec_info[1] & 0x80) && (local_sep->media_codec_info[1] & 0x80))/* block_no:4 */
        {
            ret_buf[4] = 0x80;
        }
        else
        {
            ret_buf[4] = 0x10;
        }

        /* subbands */
        if ((remote_codec_info[1] & 0x04) && (local_sep->media_codec_info[1] & 0x04))/* 8 subbnads */
        {
            ret_buf[4] |= 0x04;
        }
        else if ((remote_codec_info[1] & 0x08) && (local_sep->media_codec_info[1] & 0x08))/* 4 subbnads */
        {
            ret_buf[4] |= 0x08;
        }
        else
        {
            ret_buf[4] |= 0x04;
        }

        /* allocation method */
        if ((remote_codec_info[1] & 0x01) && (local_sep->media_codec_info[1] & 0x01))/* Loudness */
        {
            ret_buf[4] |= 0x01;
        }
        else if ((remote_codec_info[1] & 0x02) && (local_sep->media_codec_info[1] & 0x02))/* SNR */
        {
            ret_buf[4] |= 0x02;
        }
        else
        {
            ret_buf[4] |= 0x01;
        }

        /* min bit_pool */
        if (remote_codec_info[2] > local_sep->media_codec_info[2])
        {
            ret_buf[5] = remote_codec_info[2];
        }
        else
        {
            ret_buf[5] = local_sep->media_codec_info[2];
        }

        /* max bit_pool */
        if (remote_codec_info[3] < local_sep->media_codec_info[3])
        {
            ret_buf[6] = remote_codec_info[3];
        }
        else
        {
            ret_buf[6] = local_sep->media_codec_info[3];
        }

        status = true;
    }

    return status;
}

bool avdtp_stream_data_send(uint8_t   bd_addr[6],
                            uint16_t  seq_num,
                            uint32_t  time_stamp,
                            uint8_t   frame_num,
                            uint8_t  *data,
                            uint16_t  len)
{
    T_AVDTP_LINK *link;
    bool          ret = false;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
        {
            uint8_t  *pkt_ptr;
            uint16_t  pkt_len;

            pkt_len = len + 12;  /* avdtp header(12 bytes) */

            if (link->sig_chann.enable_cp_flag)
            {
                pkt_len += 1;
            }

            if (link->sig_chann.codec_type == SBC_MEDIA_CODEC_TYPE)
            {
                pkt_len += 1;
            }
            else if (link->sig_chann.codec_type == VENDOR_CODEC_TYPE)
            {
                if ((link->sig_chann.codec_subtype == CODEC_TYPE_LDAC) ||
                    (link->sig_chann.codec_subtype == CODEC_TYPE_LC3))
                {
                    pkt_len += 1;
                }
                else if (link->sig_chann.codec_subtype == CODEC_TYPE_LHDC)
                {
                    pkt_len += 2;
                }
            }

            pkt_ptr = mpa_get_l2c_buf(avdtp->queue_id,
                                      link->strm_chann.cid,
                                      0,
                                      pkt_len,
                                      avdtp->data_offset,
                                      true);
            if (pkt_ptr != NULL)
            {
                uint8_t *p;

                p = &pkt_ptr[avdtp->data_offset];
                BE_UINT8_TO_STREAM(p, 0x80);
                BE_UINT8_TO_STREAM(p, 0x60);
                BE_UINT16_TO_STREAM(p, seq_num);
                BE_UINT32_TO_STREAM(p, time_stamp);
                BE_UINT32_TO_STREAM(p, (uint64_t)0x00);

                if (link->sig_chann.enable_cp_flag)
                {
                    BE_UINT8_TO_STREAM(p, 0x00);
                }

                if (link->sig_chann.codec_type == SBC_MEDIA_CODEC_TYPE)
                {
                    BE_UINT8_TO_STREAM(p, frame_num & 0x0f);
                }
                else if (link->sig_chann.codec_type == VENDOR_CODEC_TYPE)
                {
                    if ((link->sig_chann.codec_subtype == CODEC_TYPE_LDAC) ||
                        (link->sig_chann.codec_subtype == CODEC_TYPE_LC3))
                    {
                        BE_UINT8_TO_STREAM(p, frame_num);
                    }
                    else if (link->sig_chann.codec_subtype == CODEC_TYPE_LHDC)
                    {
                        BE_UINT8_TO_STREAM(p, (frame_num & 0x0f) << 2);
                        BE_UINT8_TO_STREAM(p, (uint8_t)(seq_num % 0xff));
                    }
                }

                memcpy(p, data, len);
                mpa_send_l2c_data_req(pkt_ptr,
                                      avdtp->data_offset,
                                      link->strm_chann.cid,
                                      pkt_len,
                                      false);
                ret = true;
            }
        }
    }

    return ret;
}

void avdtp_signal_connect_req(uint8_t  bd_addr[6],
                              uint16_t avdtp_ver,
                              uint8_t  role,
                              uint16_t mtu_size)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = avdtp_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        link->sig_chann.state     = AVDTP_STATE_CONNECTING;
        link->avdtp_ver           = avdtp_ver;
        link->conn_role           = role;
        link->strm_chann.mtu_size = mtu_size;
        mpa_send_l2c_conn_req(PSM_AVDTP,
                              UUID_AVDTP,
                              avdtp->queue_id,
                              AVDTP_SIG_MTU,
                              bd_addr,
                              MPA_L2C_MODE_BASIC,
                              0xFFFF);
    }
}

void avdtp_stream_connect_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    PROTOCOL_PRINT_INFO1("avdtp_stream_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->strm_chann.state == AVDTP_STATE_DISCONNECTED)
        {
            link->strm_chann.state = AVDTP_STATE_CONNECTING;

            if (link->strm_chann.mtu_size == 0)
            {
                if (link->sig_chann.codec_type != VENDOR_CODEC_TYPE)
                {
                    mpa_send_l2c_conn_req(PSM_AVDTP,
                                          UUID_AVDTP,
                                          avdtp->queue_id,
                                          AVDTP_STREAM_MTU,
                                          bd_addr,
                                          MPA_L2C_MODE_BASIC,
                                          0xFFFF);
                }
                else
                {
                    mpa_send_l2c_conn_req(PSM_AVDTP,
                                          UUID_AVDTP,
                                          avdtp->queue_id,
                                          AVDTP_LDAC_STREAM_MTU,
                                          bd_addr,
                                          MPA_L2C_MODE_BASIC,
                                          0xFFFF);
                }
            }
            else
            {
                mpa_send_l2c_conn_req(PSM_AVDTP,
                                      UUID_AVDTP,
                                      avdtp->queue_id,
                                      link->strm_chann.mtu_size,
                                      bd_addr,
                                      MPA_L2C_MODE_BASIC,
                                      0xFFFF);
            }
        }
    }
}

bool avdtp_signal_disconnect_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    PROTOCOL_PRINT_TRACE1("avdtp_signal_disconnect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        mpa_send_l2c_disconn_req(link->sig_chann.cid);
        return true;
    }

    return false;
}

void avdtp_stream_disconnect_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    PROTOCOL_PRINT_TRACE1("avdtp_stream_disconnect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        mpa_send_l2c_disconn_req(link->strm_chann.cid);
    }
}

void avdtp_remote_capability_parse(T_AVDTP_REMOTE_SEP *sep,
                                   uint8_t            *capability,
                                   uint16_t            capability_len)
{
    uint8_t service_len;
    uint8_t service_category;
    uint8_t i = 0;

    sep->cpbs_mask = 0;
    memset(&sep->cpbs_order[0], 0xff, 8);

    while (capability_len != 0)
    {
        BE_STREAM_TO_UINT8(service_category, capability);
        BE_STREAM_TO_UINT8(service_len, capability);

        switch (service_category)
        {
        case SRV_CATEG_MEDIA_TRANS:
            {
                sep->cpbs_order[i] = SRV_CATEG_MEDIA_TRANS;
                i++;
                sep->cpbs_mask |= CATEG_MIDIA_TRANS_MASK;
            }
            break;

        case SRV_CATEG_CP:
            {
                sep->cpbs_order[i] = SRV_CATEG_CP;
                i++;
                sep->cpbs_mask |= CATEG_CP_MASK;
            }
            break;

        case SRV_CATEG_MEDIA_CODEC:
            {
                sep->cpbs_order[i]     = SRV_CATEG_MEDIA_CODEC;
                i++;
                sep->cpbs_mask        |= CATEG_MEDIA_CODEC_MASK;
                sep->media_type        = (*capability & 0xF0) >> 4;
                sep->media_codec_type  = *(capability + 1);
                if (sep->media_codec_type == SBC_MEDIA_CODEC_TYPE)
                {
                    sep->media_codec_subtype = CODEC_TYPE_SBC;
                    memcpy(&sep->media_codec_info[0], capability + 2, 4);
                }
                else if (sep->media_codec_type == AAC_MEDIA_CODEC_TYPE)
                {
                    sep->media_codec_subtype = CODEC_TYPE_AAC;
                    memcpy(&sep->media_codec_info[0], capability + 2, 6);
                }
                else if (sep->media_codec_type == USAC_MEDIA_CODEC_TYPE)
                {
                    sep->media_codec_subtype = CODEC_TYPE_USAC;
                    memcpy(&sep->media_codec_info[0], capability + 2, 7);
                }
                else if (sep->media_codec_type == VENDOR_CODEC_TYPE)
                {
                    if (memcmp(capability + 2, avdtp_ldac_codec_info, 6) == 0)
                    {
                        sep->media_codec_subtype = CODEC_TYPE_LDAC;
                        memcpy(&sep->media_codec_info[0], capability + 2, 8);
                    }
                    else if (memcmp(capability + 2, avdtp_lc3_codec_info, 6) == 0)
                    {
                        sep->media_codec_subtype = CODEC_TYPE_LC3;
                        memcpy(&sep->media_codec_info[0], capability + 2, 9);
                    }
                    else if (memcmp(capability + 2, avdtp_lhdc_v5_codec_info, 6) == 0)
                    {
                        sep->media_codec_subtype = CODEC_TYPE_LHDC;
                        memcpy(&sep->media_codec_info[0], capability + 2, 11);
                    }
                }
            }
            break;

        case SRV_CATEG_DELAY_REPORT:
            {
                sep->cpbs_order[i] = SRV_CATEG_DELAY_REPORT;
                i++;
                sep->cpbs_mask |= CATEG_DELAY_REPORT_MASK;
            }
            break;

        default:
            break;
        }

        if (capability_len > service_len + 2)
        {
            capability_len -= service_len + 2;
            capability     += service_len;
        }
        else
        {
            capability_len = 0;
        }
    }
}

uint8_t avdtp_config_parse(T_AVDTP_LINK      *link,
                           uint8_t           *capability,
                           uint16_t           capability_len,
                           T_AVDTP_LOCAL_SEP *sep)
{
    uint8_t service_len;
    uint8_t service_category;

    while (capability_len != 0)
    {
        BE_STREAM_TO_UINT8(service_category, capability);
        BE_STREAM_TO_UINT8(service_len, capability);

        switch (service_category)
        {
        case SRV_CATEG_MEDIA_CODEC:
            if (*(capability + 1) != SBC_MEDIA_CODEC_TYPE &&
                *(capability + 1) != AAC_MEDIA_CODEC_TYPE &&
                *(capability + 1) != USAC_MEDIA_CODEC_TYPE &&
                *(capability + 1) != VENDOR_CODEC_TYPE)
            {
                return INVALID_CODEC_TYPE;
            }

            if (sep->codec_type == *(capability + 1))
            {
                if (sep->codec_type == SBC_MEDIA_CODEC_TYPE)
                {
                    uint8_t sample_frequency  = *(capability + 2) & 0xf0;
                    uint8_t channel_mode      = *(capability + 2) & 0x0f;
                    uint8_t block_number      = *(capability + 3) & 0xf0;
                    uint8_t subbands          = *(capability + 3) & 0x0c;
                    uint8_t allocation_method = *(capability + 3) & 0x03;

                    if (sample_frequency != 0x80 && sample_frequency != 0x40 &&
                        sample_frequency != 0x20 && sample_frequency != 0x10)
                    {
                        return INVALID_SAMPLING_FREQUENCY;
                    }
                    else if ((sample_frequency & (sep->media_codec_info[0] & 0xf0)) == 0)
                    {
                        return NOT_SUPPORTED_SAMPLING_FREQUENCY;
                    }

                    if (channel_mode != 0x08 && channel_mode != 0x04 &&
                        channel_mode != 0x02 && channel_mode != 0x01)
                    {
                        return INVALID_CHANNEL_MODE;
                    }
                    else if ((channel_mode & (sep->media_codec_info[0] & 0x0f)) == 0)
                    {
                        return NOT_SUPPORTED_CHANNEL_MODE;
                    }

                    if (block_number != 0x80 && block_number != 0x40 &&
                        block_number != 0x20 && block_number != 0x10)
                    {
                        return INVALID_BLOCK_LENGTH;
                    }
                    else if ((block_number & (sep->media_codec_info[1] & 0xf0)) == 0)
                    {
                        return INVALID_BLOCK_LENGTH;
                    }

                    if (subbands != 0x08 && subbands != 0x04)
                    {
                        return INVALID_SUBBANDS;
                    }
                    else if ((subbands & (sep->media_codec_info[1] & 0x0c)) == 0)
                    {
                        return NOT_SUPPORTED_SUBBANDS;
                    }

                    if (allocation_method != 0x02 && allocation_method != 0x01)
                    {
                        return INVALID_ALLOCATION_METHOD;
                    }
                    else if ((allocation_method & (sep->media_codec_info[1] & 0x03)) == 0)
                    {
                        return NOT_SUPPORTED_ALLOCATION_METHOD;
                    }

                    if (*(capability + 4) < sep->media_codec_info[2])
                    {
                        return INVALID_MINIMUM_BITPOOL_VALUE;
                    }

                    if (*(capability + 5) > sep->media_codec_info[3])
                    {
                        return INVALID_MAXIMUM_BITPOOL_VALUE;
                    }
                }
                else if (sep->codec_type == AAC_MEDIA_CODEC_TYPE)
                {
                    uint8_t  object_type        = *(capability + 2) & 0xfe;
                    bool     drc                = *(capability + 2) & 0x01;
                    uint16_t sampling_frequency = (*(capability + 3) << 8) + (*(capability + 4) & 0xf0);
                    uint8_t  channel_setting    = *(capability + 4) & 0x0f;

                    if (object_type != 0x80 && object_type != 0x40 && object_type != 0x20 &&
                        object_type != 0x10 && object_type != 0x08 && object_type != 0x04 &&
                        object_type != 0x02)
                    {
                        return INVALID_OBJECT_TYPE;
                    }
                    else if ((object_type & (sep->media_codec_info[0] & 0xfe)) == 0)
                    {
                        return NOT_SUPPORTED_OBJECT_TYPE;
                    }

                    if (object_type == 0x80 && drc)
                    {
                        return INVALID_DRC;
                    }
                    else if (drc != (sep->media_codec_info[0] & 0x01))
                    {
                        return NOT_SUPPORTED_DRC;
                    }

                    if (sampling_frequency != 0x8000 && sampling_frequency != 0x4000 &&
                        sampling_frequency != 0x2000 && sampling_frequency != 0x1000 &&
                        sampling_frequency != 0x0800 && sampling_frequency != 0x0400 &&
                        sampling_frequency != 0x0200 && sampling_frequency != 0x0100 &&
                        sampling_frequency != 0x0080 && sampling_frequency != 0x0040 &&
                        sampling_frequency != 0x0020 && sampling_frequency != 0x0010)
                    {
                        return INVALID_SAMPLING_FREQUENCY;
                    }
                    else if ((sampling_frequency & ((sep->media_codec_info[1] << 8) +
                                                    (sep->media_codec_info[2] & 0xf0))) == 0)
                    {
                        return NOT_SUPPORTED_SAMPLING_FREQUENCY;
                    }

                    if (channel_setting != 0x08 && channel_setting != 0x04 &&
                        channel_setting != 0x02 && channel_setting != 0x01)
                    {
                        return INVALID_CHANNELS;
                    }
                    else if ((channel_setting & (sep->media_codec_info[2] & 0x0f)) == 0)
                    {
                        return NOT_SUPPORTED_CHANNELS;
                    }
                }

                link->sig_chann.codec_type = *(capability + 1);

                if (link->sig_chann.codec_type == VENDOR_CODEC_TYPE)
                {
                    if (!memcmp(capability + 2, avdtp_ldac_codec_info, 6))
                    {
                        link->sig_chann.codec_subtype = CODEC_TYPE_LDAC;
                    }
                    else if (!memcmp(capability + 2, avdtp_lc3_codec_info, 6))
                    {
                        link->sig_chann.codec_subtype = CODEC_TYPE_LC3;
                    }
                    else if (!memcmp(capability + 2, avdtp_lhdc_v5_codec_info, 6))
                    {
                        link->sig_chann.codec_subtype = CODEC_TYPE_LHDC;
                    }
                }

                if (service_len > 2)
                {
                    if (service_len - 2 > MAX_CODEC_INFO_SIZE)
                    {
                        memcpy(&link->sig_chann.codec_info[0], capability + 2, MAX_CODEC_INFO_SIZE);
                    }
                    else
                    {
                        memcpy(&link->sig_chann.codec_info[0], capability + 2, service_len - 2);
                    }
                }
            }
            else
            {
                return NOT_SUPPORTED_CODEC_TYPE;
            }

            break;

        case SRV_CATEG_CP:
            {
                if (avdtp->service_category & CATEG_CP_MASK)
                {
                    link->sig_chann.enable_cp_flag = 1;
                }
            }
            break;

        case SRV_CATEG_DELAY_REPORT:
            {
                if (avdtp->service_category & CATEG_DELAY_REPORT_MASK)
                {
                    link->sig_chann.delay_report_flag = 1;
                }
            }
            break;

        default:
            break;
        }

        /*PTS will send unknown cartegory*/
        if (capability_len > service_len + 2)
        {
            capability_len -= service_len + 2;
            capability     += service_len;
        }
        else
        {
            capability_len = 0;
        }
    }

    return 0;
}

bool avdtp_signal_discover_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;

        pkt_len = sizeof(T_AVDTP_SIG_HDR);
        pkt = mpa_get_l2c_buf(avdtp->queue_id,
                              link->sig_chann.cid,
                              0,
                              pkt_len,
                              avdtp->data_offset,
                              false);
        if (pkt != NULL)
        {
            PROTOCOL_PRINT_TRACE1("avdtp_signal_discover_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            pkt[avdtp->data_offset] = (link->sig_chann.tx_trans_label |
                                       (AVDTP_PKT_TYPE_SINGLE << 2) |
                                       AVDTP_MSG_TYPE_CMD);
            pkt[avdtp->data_offset + 1] = AVDTP_DISCOVER;
            mpa_send_l2c_data_req(pkt,
                                  avdtp->data_offset,
                                  link->sig_chann.cid,
                                  pkt_len,
                                  false);
            link->sig_chann.tx_trans_label += 0x10;
            link->sig_chann.cmd_flag = AVDTP_DISCOVER;
            sys_timer_start(link->timer_handle);

            return true;
        }
    }

    return false;
}

bool avdtp_signal_abort_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;

        pkt_len = sizeof(T_AVDTP_SIG_HDR) + 1;
        pkt = mpa_get_l2c_buf(avdtp->queue_id,
                              link->sig_chann.cid,
                              0,
                              pkt_len,
                              avdtp->data_offset,
                              false);
        if (pkt != NULL)
        {
            PROTOCOL_PRINT_TRACE1("avdtp_signal_abort_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            pkt[avdtp->data_offset] = (link->sig_chann.tx_trans_label |
                                       (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            pkt[avdtp->data_offset + 1] = AVDTP_ABORT;
            if (link->sig_chann.int_flag == 1)
            {
                pkt[avdtp->data_offset + 2] = link->sig_chann.acp_seid;
            }
            else
            {
                pkt[avdtp->data_offset + 2] = link->sig_chann.int_seid;
            }
            mpa_send_l2c_data_req(pkt,
                                  avdtp->data_offset,
                                  link->sig_chann.cid,
                                  pkt_len,
                                  false);
            link->sig_chann.tx_trans_label += 0x10;
            link->sig_chann.cmd_flag = AVDTP_ABORT;
            sys_timer_start(link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_delay_report_req(uint8_t  bd_addr[6],
                                   uint16_t delay)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;

        if (link->sig_chann.sig_state != AVDTP_SIG_STATE_CFG &&
            link->sig_chann.sig_state != AVDTP_SIG_STATE_OPENING &&
            link->sig_chann.sig_state != AVDTP_SIG_STATE_OPEN &&
            link->sig_chann.sig_state != AVDTP_SIG_STATE_STREAMING)
        {
            return false;
        }

        if (link->sig_chann.delay_report_flag && (link->local_sep_role == AVDTP_TSEP_SNK))
        {
            pkt_len = sizeof(T_AVDTP_SIG_HDR) + 3;
            pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                  link->sig_chann.cid,
                                  0,
                                  pkt_len,
                                  avdtp->data_offset,
                                  false);
            if (pkt != NULL)
            {
                PROTOCOL_PRINT_TRACE1("avdtp_signal_delay_report_req: bd_addr %s", TRACE_BDADDR(bd_addr));

                pkt[avdtp->data_offset] = (link->sig_chann.tx_trans_label |
                                           (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
                pkt[avdtp->data_offset + 1] = AVDTP_DELAY_REPORT;
                if (link->sig_chann.int_flag == 1)
                {
                    pkt[avdtp->data_offset + 2] = link->sig_chann.acp_seid;
                }
                else
                {
                    pkt[avdtp->data_offset + 2] = link->sig_chann.int_seid;
                }
                /* delay unit in 0.1ms */
                pkt[avdtp->data_offset + 3] = (uint8_t)((delay * 10) >> 8);
                pkt[avdtp->data_offset + 4] = (uint8_t)(delay * 10);
                mpa_send_l2c_data_req(pkt,
                                      avdtp->data_offset,
                                      link->sig_chann.cid,
                                      pkt_len,
                                      false);
                link->sig_chann.tx_trans_label += 0x10;
                return true;
            }
        }
    }

    return false;
}

bool avdtp_signal_get_capability_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;

        pkt_len = sizeof(T_AVDTP_SIG_HDR) + 1;
        pkt = mpa_get_l2c_buf(avdtp->queue_id,
                              link->sig_chann.cid,
                              0,
                              pkt_len,
                              avdtp->data_offset,
                              false);
        if (pkt != NULL)
        {
            uint8_t     temp;

            PROTOCOL_PRINT_TRACE1("avdtp_signal_get_capability_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            pkt[avdtp->data_offset] = (link->sig_chann.tx_trans_label |
                                       (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            if (link->avdtp_ver >= AVDTP_VERSON_V1_3)
            {
                link->sig_chann.cmd_flag = AVDTP_GET_ALL_CPBS;
            }
            else
            {
                link->sig_chann.cmd_flag = AVDTP_GET_CPBS;
            }
            pkt[avdtp->data_offset + 1] = link->sig_chann.cmd_flag;
            temp = link->sig_chann.acp_seid_idx;
            pkt[avdtp->data_offset + 2] = link->sig_chann.avdtp_sep[temp].sep_id;

            mpa_send_l2c_data_req(pkt,
                                  avdtp->data_offset,
                                  link->sig_chann.cid,
                                  pkt_len,
                                  false);
            link->sig_chann.tx_trans_label += 0x10;
            sys_timer_start(link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_cfg_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;
        uint8_t  capability_len;
        uint8_t  capability_flag;
        uint8_t  i;

        if (link->sig_chann.sig_state != AVDTP_SIG_STATE_IDLE)
        {
            return false;
        }

        i = link->sig_chann.acp_seid_idx;
        capability_len = 0;
        capability_flag = 0;
        if ((link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_MIDIA_TRANS_MASK) &&
            (avdtp->service_category & CATEG_MIDIA_TRANS_MASK))
        {
            capability_len  += MEDIA_TRANS_LOSC + 2;
            capability_flag |= CATEG_MIDIA_TRANS_MASK;
        }

        if ((link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_CP_MASK) &&
            (avdtp->service_category & CATEG_CP_MASK))
        {
            capability_len  += CP_LOSC + 2;
            capability_flag |= CATEG_CP_MASK;
        }

        if ((link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_DELAY_REPORT_MASK) &&
            (avdtp->service_category & CATEG_DELAY_REPORT_MASK))
        {
            capability_len  += DELAY_REPORT_LOSC + 2;
            capability_flag |= CATEG_DELAY_REPORT_MASK;
        }

        if ((link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_MEDIA_CODEC_MASK) &&
            (avdtp->service_category & CATEG_MEDIA_CODEC_MASK))
        {
            if (link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
            {
                if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                {
                    capability_len += MEDIA_CODEC_LDAC_LOSC + 2;
                }
                else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                {
                    capability_len += MEDIA_CODEC_LC3_LOSC + 2;
                }
                else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                {
                    capability_len += MEDIA_CODEC_LHDC_LOSC + 2;
                }
            }
            else if (link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
            {
                capability_len += MEDIA_CODEC_AAC_LOSC + 2;
            }
            else
            {
                capability_len += MEDIA_CODEC_SBC_LOSC + 2;
            }
            capability_flag |= CATEG_MEDIA_CODEC_MASK;
        }

        link->sig_chann.enable_cp_flag    = 0;
        link->sig_chann.delay_report_flag = 0;

        pkt_len = sizeof(T_AVDTP_SIG_HDR) + 2 + capability_len;
        pkt = mpa_get_l2c_buf(avdtp->queue_id,
                              link->sig_chann.cid,
                              0,
                              pkt_len,
                              avdtp->data_offset,
                              false);
        if (pkt != NULL)
        {
            uint8_t           *p;
            T_AVDTP_LOCAL_SEP *sep;
            uint8_t            j = 0;

            p = &pkt[avdtp->data_offset];
            BE_UINT8_TO_STREAM(p, (link->sig_chann.tx_trans_label | (AVDTP_PKT_TYPE_SINGLE << 2) |
                                   AVDTP_MSG_TYPE_CMD));
            BE_UINT8_TO_STREAM(p, AVDTP_SET_CFG);
            BE_UINT8_TO_STREAM(p, link->sig_chann.avdtp_sep[i].sep_id);
            sep = os_queue_peek(&avdtp->local_sep_list, 0);
            while (sep != NULL)
            {
                if (sep->sub_codec_type == link->sig_chann.avdtp_sep[i].media_codec_subtype &&
                    sep->tsep != link->conn_role)
                {
                    BE_UINT8_TO_STREAM(p, sep->sep_id << 2);
                    break;
                }
                sep = sep->next;
            }

            if (sep == NULL)
            {
                return false;
            }

            while (link->sig_chann.avdtp_sep[i].cpbs_order[j] != 0xff)
            {
                switch (link->sig_chann.avdtp_sep[i].cpbs_order[j])
                {
                case SRV_CATEG_MEDIA_TRANS:
                    if (capability_flag & CATEG_MIDIA_TRANS_MASK)
                    {
                        BE_UINT8_TO_STREAM(p, SRV_CATEG_MEDIA_TRANS);
                        BE_UINT8_TO_STREAM(p, MEDIA_TRANS_LOSC);
                    }
                    break;

                case SRV_CATEG_CP:
                    if (capability_flag & CATEG_CP_MASK)
                    {
                        link->sig_chann.enable_cp_flag = 1;
                        BE_UINT8_TO_STREAM(p, SRV_CATEG_CP);
                        BE_UINT8_TO_STREAM(p, CP_LOSC);
                        BE_UINT8_TO_STREAM(p, CP_TYPE_LSB);
                        BE_UINT8_TO_STREAM(p, CP_TYPE_MSB);
                    }
                    break;

                case SRV_CATEG_MEDIA_CODEC:
                    if (capability_flag & CATEG_MEDIA_CODEC_MASK)
                    {
                        uint8_t temp;

                        link->sig_chann.codec_type = link->sig_chann.avdtp_sep[i].media_codec_type;
                        BE_UINT8_TO_STREAM(p, SRV_CATEG_MEDIA_CODEC);
                        if (link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                        {
                            link->sig_chann.codec_subtype = link->sig_chann.avdtp_sep[i].media_codec_subtype;

                            if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                            {
                                avdtp_vendor_codec_ldac_handler(link->conn_role,
                                                                HDL_TYPE_FILL_CODEC_INFO,
                                                                link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                sep,
                                                                p);
                            }
                            else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                            {
                                avdtp_vendor_codec_lc3_handler(link->conn_role,
                                                               HDL_TYPE_FILL_CODEC_INFO,
                                                               link->sig_chann.avdtp_sep[i].media_codec_info,
                                                               sep,
                                                               p);
                            }
                            else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                            {
                                avdtp_vendor_codec_lhdc_handler(link->conn_role,
                                                                HDL_TYPE_FILL_CODEC_INFO,
                                                                link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                sep,
                                                                p);
                            }
                        }
                        else if (link->sig_chann.avdtp_sep[i].media_codec_type == USAC_MEDIA_CODEC_TYPE)
                        {
                            avdtp_codec_usac_handler(link->conn_role,
                                                     HDL_TYPE_FILL_CODEC_INFO,
                                                     link->sig_chann.avdtp_sep[i].media_codec_info,
                                                     sep,
                                                     p);
                        }
                        else if (link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
                        {
                            avdtp_codec_aac_handler(link->conn_role,
                                                    HDL_TYPE_FILL_CODEC_INFO,
                                                    link->sig_chann.avdtp_sep[i].media_codec_info,
                                                    sep,
                                                    p);
                        }
                        else if (link->sig_chann.avdtp_sep[i].media_codec_type == SBC_MEDIA_CODEC_TYPE)
                        {
                            avdtp_codec_sbc_handler(link->conn_role,
                                                    HDL_TYPE_FILL_CODEC_INFO,
                                                    link->sig_chann.avdtp_sep[i].media_codec_info,
                                                    sep,
                                                    p);
                        }

                        temp = *p;
                        memcpy(&link->sig_chann.codec_info[0], p + 3, temp - 2);
                        p += temp + 1;
                    }
                    break;

                case SRV_CATEG_DELAY_REPORT:
                    if (capability_flag & CATEG_DELAY_REPORT_MASK)
                    {
                        link->sig_chann.delay_report_flag = 1;
                        BE_UINT8_TO_STREAM(p, SRV_CATEG_DELAY_REPORT);
                        BE_UINT8_TO_STREAM(p, DELAY_REPORT_LOSC);
                    }
                    break;

                default:
                    break;
                }
                j++;
            }
            PROTOCOL_PRINT_TRACE3("avdtp_signal_cfg_req: bd_addr %s, acp_seid 0x%02x, int_seid 0x%02x",
                                  TRACE_BDADDR(bd_addr), pkt[avdtp->data_offset + 2] >> 2,
                                  pkt[avdtp->data_offset + 3] >> 2);
            mpa_send_l2c_data_req(pkt,
                                  avdtp->data_offset,
                                  link->sig_chann.cid,
                                  pkt_len,
                                  false);
            link->sig_chann.tx_trans_label += 0x10;
            link->sig_chann.cmd_flag        = AVDTP_SET_CFG;
            link->sig_chann.int_flag        = 1;
            link->local_sep_role            = sep->tsep;
            sys_timer_start(link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_recfg_req(uint8_t bd_addr[6],
                            uint8_t codec_type,
                            uint8_t role)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;
        uint8_t  capability_len;
        uint8_t  capability_flag;
        uint8_t  media_codec_type;
        uint8_t  media_codec_subtype;
        uint8_t  i;

        if (link->sig_chann.sig_state != AVDTP_SIG_STATE_OPEN)
        {
            return false;
        }

        if (link->sig_chann.acp_sep_no == 0)
        {
            link->conn_role = role;
            link->sig_chann.recfg_codec_type = codec_type;
            return avdtp_signal_discover_req(bd_addr);
        }

        switch (codec_type)
        {
        case AVDTP_CODEC_TYPE_SBC:
            {
                media_codec_type    = SBC_MEDIA_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_SBC;
            }
            break;

        case AVDTP_CODEC_TYPE_AAC:
            {
                media_codec_type    = AAC_MEDIA_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_AAC;
            }
            break;

        case AVDTP_CODEC_TYPE_USAC:
            {
                media_codec_type    = USAC_MEDIA_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_USAC;
            }
            break;

        case AVDTP_CODEC_TYPE_LDAC:
            {
                media_codec_type    = VENDOR_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_LDAC;
            }
            break;

        case AVDTP_CODEC_TYPE_LC3:
            {
                media_codec_type    = VENDOR_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_LC3;
            }
            break;

        case AVDTP_CODEC_TYPE_LHDC:
            {
                media_codec_type    = VENDOR_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_LHDC;
            }
            break;

        default:
            return false;
        }

        for (i = 0; i < link->sig_chann.acp_sep_no; i++)
        {
            if (link->sig_chann.avdtp_sep[i].media_codec_type == media_codec_type)
            {
                if (link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                {
                    if (link->sig_chann.avdtp_sep[i].media_codec_subtype == media_codec_subtype)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }

        if (i < link->sig_chann.acp_sep_no)
        {
            capability_len  = 0;
            capability_flag = 0;

            if ((link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_CP_MASK) &&
                (avdtp->service_category & CATEG_CP_MASK))
            {
                capability_len  += CP_LOSC + 2;
                capability_flag |= CATEG_CP_MASK;
            }

            if ((link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_MEDIA_CODEC_MASK) &&
                (avdtp->service_category & CATEG_MEDIA_CODEC_MASK))
            {
                if (link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                {
                    if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                    {
                        capability_len += MEDIA_CODEC_LDAC_LOSC + 2;
                    }
                    else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                    {
                        capability_len += MEDIA_CODEC_LC3_LOSC + 2;
                    }
                    else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                    {
                        capability_len += MEDIA_CODEC_LHDC_LOSC + 2;
                    }
                }
                else if (link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
                {
                    capability_len += MEDIA_CODEC_AAC_LOSC + 2;
                }
                else
                {
                    capability_len += MEDIA_CODEC_SBC_LOSC + 2;
                }
                capability_flag |= CATEG_MEDIA_CODEC_MASK;
            }

            link->sig_chann.enable_cp_flag = 0;
            pkt_len = sizeof(T_AVDTP_SIG_HDR) + 1 + capability_len;
            pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                  link->sig_chann.cid,
                                  0,
                                  pkt_len,
                                  avdtp->data_offset,
                                  false);
            if (pkt != NULL)
            {
                uint8_t  j = 0;
                uint8_t *p = &pkt[avdtp->data_offset];

                BE_UINT8_TO_STREAM(p, (link->sig_chann.tx_trans_label |
                                       (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD));
                BE_UINT8_TO_STREAM(p, AVDTP_RECFG);
                BE_UINT8_TO_STREAM(p, link->sig_chann.avdtp_sep[i].sep_id);

                while (link->sig_chann.avdtp_sep[i].cpbs_order[j] != 0xff)
                {
                    switch (link->sig_chann.avdtp_sep[i].cpbs_order[j])
                    {
                    case SRV_CATEG_CP:
                        if (capability_flag & CATEG_CP_MASK)
                        {
                            link->sig_chann.enable_cp_flag = 1;
                            BE_UINT8_TO_STREAM(p, SRV_CATEG_CP);
                            BE_UINT8_TO_STREAM(p, CP_LOSC);
                            BE_UINT8_TO_STREAM(p, CP_TYPE_LSB);
                            BE_UINT8_TO_STREAM(p, CP_TYPE_MSB);
                        }
                        break;

                    case SRV_CATEG_MEDIA_CODEC:
                        if (capability_flag & CATEG_MEDIA_CODEC_MASK)
                        {
                            uint8_t            temp;
                            T_AVDTP_LOCAL_SEP *sep;

                            sep = os_queue_peek(&avdtp->local_sep_list, 0);
                            while (sep != NULL)
                            {
                                if (sep->sub_codec_type == link->sig_chann.avdtp_sep[i].media_codec_subtype &&
                                    sep->tsep != link->conn_role)
                                {
                                    break;
                                }
                                sep = sep->next;
                            }

                            if (sep == NULL)
                            {
                                return false;
                            }

                            link->sig_chann.codec_type = link->sig_chann.avdtp_sep[i].media_codec_type;
                            link->local_sep_role       = sep->tsep;

                            BE_UINT8_TO_STREAM(p, SRV_CATEG_MEDIA_CODEC);
                            if (link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                            {
                                link->sig_chann.codec_subtype = link->sig_chann.avdtp_sep[i].media_codec_subtype;

                                if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                                {
                                    avdtp_vendor_codec_ldac_handler(link->conn_role,
                                                                    HDL_TYPE_FILL_CODEC_INFO,
                                                                    link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                    sep,
                                                                    p);
                                }
                                else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                                {
                                    avdtp_vendor_codec_lc3_handler(link->conn_role,
                                                                   HDL_TYPE_FILL_CODEC_INFO,
                                                                   link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                   sep,
                                                                   p);
                                }
                                else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                                {
                                    avdtp_vendor_codec_lhdc_handler(link->conn_role,
                                                                    HDL_TYPE_FILL_CODEC_INFO,
                                                                    link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                    sep,
                                                                    p);
                                }
                            }
                            else if (link->sig_chann.avdtp_sep[i].media_codec_type == USAC_MEDIA_CODEC_TYPE)
                            {
                                avdtp_codec_usac_handler(link->conn_role,
                                                         HDL_TYPE_FILL_CODEC_INFO,
                                                         link->sig_chann.avdtp_sep[i].media_codec_info,
                                                         sep,
                                                         p);
                            }
                            else if (link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
                            {
                                avdtp_codec_aac_handler(link->conn_role,
                                                        HDL_TYPE_FILL_CODEC_INFO,
                                                        link->sig_chann.avdtp_sep[i].media_codec_info,
                                                        sep,
                                                        p);
                            }
                            else if (link->sig_chann.avdtp_sep[i].media_codec_type == SBC_MEDIA_CODEC_TYPE)
                            {
                                avdtp_codec_sbc_handler(link->conn_role,
                                                        HDL_TYPE_FILL_CODEC_INFO,
                                                        link->sig_chann.avdtp_sep[i].media_codec_info,
                                                        sep,
                                                        p);
                            }

                            temp = *p;
                            memcpy(&link->sig_chann.codec_info[0], p + 3, temp - 2);
                            p += temp + 1;
                        }
                        break;

                    default:
                        break;
                    }
                    j++;
                }

                mpa_send_l2c_data_req(pkt,
                                      avdtp->data_offset,
                                      link->sig_chann.cid,
                                      pkt_len,
                                      false);
                link->sig_chann.tx_trans_label += 0x10;
                link->sig_chann.cmd_flag        = AVDTP_RECFG;
                link->sig_chann.int_flag        = 1;
                sys_timer_start(link->timer_handle);
                return true;
            }
        }
    }

    return false;
}

bool avdtp_signal_open_req(uint8_t bd_addr[6],
                           uint8_t role)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;

        if (link->sig_chann.sig_state == AVDTP_SIG_STATE_IDLE)
        {
            link->conn_role = role;
            return avdtp_signal_discover_req(bd_addr);
        }
        else if (link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG)
        {
            pkt_len = sizeof(T_AVDTP_SIG_HDR) + 1;
            pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                  link->sig_chann.cid,
                                  0,
                                  pkt_len,
                                  avdtp->data_offset,
                                  false);
            if (pkt != NULL)
            {
                PROTOCOL_PRINT_TRACE1("avdtp_signal_open_req: bd_addr %s", TRACE_BDADDR(bd_addr));

                pkt[avdtp->data_offset] = (link->sig_chann.tx_trans_label |
                                           (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
                pkt[avdtp->data_offset + 1] = AVDTP_OPEN;

                if (link->sig_chann.int_flag == 1)
                {
                    pkt[avdtp->data_offset + 2] = link->sig_chann.acp_seid;
                }
                else
                {
                    pkt[avdtp->data_offset + 2] = link->sig_chann.int_seid;
                }

                mpa_send_l2c_data_req(pkt,
                                      avdtp->data_offset,
                                      link->sig_chann.cid,
                                      pkt_len,
                                      false);
                link->sig_chann.tx_trans_label += 0x10;
                link->sig_chann.cmd_flag        = AVDTP_OPEN;
                link->sig_chann.sig_state       = AVDTP_SIG_STATE_OPENING;
                sys_timer_start(link->timer_handle);

                return true;
            }
        }
    }

    return false;
}

bool avdtp_signal_start_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;

        PROTOCOL_PRINT_TRACE3("avdtp_signal_start_req: bd_addr %s, sig_state 0x%02x, cmd_flag 0x%02x",
                              TRACE_BDADDR(bd_addr), link->sig_chann.sig_state,
                              link->sig_chann.cmd_flag);

        if (link->sig_chann.sig_state != AVDTP_SIG_STATE_OPEN ||
            link->sig_chann.cmd_flag == AVDTP_START)
        {
            return false;
        }

        pkt_len = sizeof(T_AVDTP_SIG_HDR) + 1;
        pkt = mpa_get_l2c_buf(avdtp->queue_id,
                              link->sig_chann.cid,
                              0,
                              pkt_len,
                              avdtp->data_offset,
                              false);
        if (pkt != NULL)
        {
            pkt[avdtp->data_offset] = (link->sig_chann.tx_trans_label |
                                       (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            pkt[avdtp->data_offset + 1] = AVDTP_START;
            if (link->sig_chann.int_flag == 1)
            {
                pkt[avdtp->data_offset + 2] = link->sig_chann.acp_seid;
            }
            else
            {
                pkt[avdtp->data_offset + 2] = link->sig_chann.int_seid;
            }
            mpa_send_l2c_data_req(pkt,
                                  avdtp->data_offset,
                                  link->sig_chann.cid,
                                  pkt_len,
                                  false);
            link->sig_chann.tx_trans_label += 0x10;
            link->sig_chann.cmd_flag = AVDTP_START;
            sys_timer_start(link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_suspend_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;

        PROTOCOL_PRINT_TRACE3("avdtp_signal_suspend_req: bd_addr %s, sig_state 0x%02x, cmd_flag 0x%02x",
                              TRACE_BDADDR(bd_addr), link->sig_chann.sig_state,
                              link->sig_chann.cmd_flag);

        if (link->sig_chann.sig_state != AVDTP_SIG_STATE_STREAMING ||
            link->sig_chann.cmd_flag == AVDTP_SUSPEND)
        {
            return false;
        }

        pkt_len = sizeof(T_AVDTP_SIG_HDR) + 1;
        pkt = mpa_get_l2c_buf(avdtp->queue_id,
                              link->sig_chann.cid,
                              0,
                              pkt_len,
                              avdtp->data_offset,
                              false);
        if (pkt != NULL)
        {
            pkt[avdtp->data_offset] = (link->sig_chann.tx_trans_label |
                                       (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            pkt[avdtp->data_offset + 1] = AVDTP_SUSPEND;
            if (link->sig_chann.int_flag == 1)
            {
                pkt[avdtp->data_offset + 2] = link->sig_chann.acp_seid;
            }
            else
            {
                pkt[avdtp->data_offset + 2] = link->sig_chann.int_seid;
            }

            mpa_send_l2c_data_req(pkt,
                                  avdtp->data_offset,
                                  link->sig_chann.cid,
                                  pkt_len,
                                  false);
            link->sig_chann.tx_trans_label += 0x10;
            link->sig_chann.cmd_flag = AVDTP_SUSPEND;
            sys_timer_start(link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_close_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *pkt;
        uint8_t  pkt_len;

        if ((link->sig_chann.sig_state != AVDTP_SIG_STATE_OPEN)
            && (link->sig_chann.sig_state != AVDTP_SIG_STATE_STREAMING))
        {
            return false;
        }

        pkt_len = sizeof(T_AVDTP_SIG_HDR) + 1;
        pkt = mpa_get_l2c_buf(avdtp->queue_id,
                              link->sig_chann.cid,
                              0,
                              pkt_len,
                              avdtp->data_offset,
                              false);
        if (pkt != NULL)
        {
            PROTOCOL_PRINT_TRACE1("avdtp_signal_close_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            pkt[avdtp->data_offset] = (link->sig_chann.tx_trans_label |
                                       (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            pkt[avdtp->data_offset + 1] = AVDTP_CLOSE;
            if (link->sig_chann.int_flag == 1)
            {
                pkt[avdtp->data_offset + 2] = link->sig_chann.acp_seid;
            }
            else
            {
                pkt[avdtp->data_offset + 2] = link->sig_chann.int_seid;
            }
            mpa_send_l2c_data_req(pkt,
                                  avdtp->data_offset,
                                  link->sig_chann.cid,
                                  pkt_len,
                                  false);
            link->sig_chann.tx_trans_label += 0x10;
            link->sig_chann.cmd_flag = AVDTP_CLOSE;
            sys_timer_start(link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_cmd_bad_state_proc(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t *signal_pkt;
        uint8_t *rsp_pkt;
        uint8_t  signal_id;
        uint8_t  trans_label;
        uint8_t  rsp_len;

        signal_pkt  = link->sig_chann.fragment_data;
        trans_label = *signal_pkt & 0xF0;
        signal_id   = *(signal_pkt + 1) & 0x3F;

        PROTOCOL_PRINT_WARN3("avdtp_cmd_bad_state_proc: bd_addr %s, signal_id 0x%02x, cid 0x%04x",
                             TRACE_BDADDR(bd_addr), signal_id, link->sig_chann.cid);

        switch (signal_id)
        {
        case AVDTP_SET_CFG:
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2; /* service category and error code */
            break;
        case AVDTP_OPEN:
        case AVDTP_CLOSE:
        case AVDTP_DELAY_REPORT:
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 1; /* error code */
            break;
        case AVDTP_START:
        case AVDTP_SUSPEND:
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2; /* ACP SEID and error code */
            break;

        default:
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 1; /* error code */
            break;
        }

        rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                  link->sig_chann.cid,
                                  0,
                                  rsp_len,
                                  avdtp->data_offset,
                                  false);
        if (rsp_pkt != NULL)
        {
            uint8_t *p = &rsp_pkt[avdtp->data_offset];

            BE_UINT8_TO_STREAM(p, (trans_label | (AVDTP_PKT_TYPE_SINGLE << 2) |
                                   AVDTP_MSG_TYPE_RSP_REJECT));
            BE_UINT8_TO_STREAM(p, signal_id);
            switch (signal_id)
            {
            case AVDTP_OPEN:
            case AVDTP_CLOSE:
            case AVDTP_DELAY_REPORT:
                BE_UINT8_TO_STREAM(p, BAD_STATE);
                break;
            case AVDTP_SET_CFG:
            case AVDTP_RECFG:
                BE_UINT8_TO_STREAM(p, SRV_CATEG_MEDIA_CODEC);
                BE_UINT8_TO_STREAM(p, BAD_STATE);
                break;
            case AVDTP_START:
            case AVDTP_SUSPEND:
                BE_UINT8_TO_STREAM(p, *(signal_pkt + 2));
                BE_UINT8_TO_STREAM(p, BAD_STATE);
                break;
            default:
                BE_UINT8_TO_STREAM(p, BAD_STATE);
                break;
            }

            mpa_send_l2c_data_req(rsp_pkt,
                                  avdtp->data_offset,
                                  link->sig_chann.cid,
                                  rsp_len,
                                  false);
            return true;
        }
    }

    return false;
}

bool avdtp_cmd_error_check_proc(T_AVDTP_LINK *link)
{
    uint16_t           pkt_len;
    uint8_t            trans_label;
    uint8_t            acp_seid;
    uint8_t            signal_id;
    uint8_t           *signal_pkt;
    uint8_t            error_code = 0;
    uint8_t            rsp_len = 0;
    uint8_t            error_category_type = 0;
    T_AVDTP_LOCAL_SEP *sep;

    signal_pkt  = link->sig_chann.fragment_data;
    pkt_len     = link->sig_chann.fragment_data_len;
    acp_seid    = (*(signal_pkt + 2) & 0xFC) >> 2;
    trans_label = *signal_pkt & 0xF0;
    signal_id   = *(signal_pkt + 1) & 0x3F;

    sep = avdtp_find_sep_by_id(acp_seid);

    if (sep == NULL && signal_id != AVDTP_DISCOVER)
    {
        error_code = BAD_ACP_SEID;
    }

    switch (signal_id)
    {
    case AVDTP_DISCOVER:
        rsp_len = sizeof(T_AVDTP_SIG_HDR) + 1;
        if (pkt_len != 2)
        {
            error_code = BAD_LENGTH;
        }
        break;

    case AVDTP_GET_CPBS:
    case AVDTP_GET_ALL_CPBS:
        if (sep != NULL)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 1;

            if (pkt_len != 3)
            {
                error_code = BAD_LENGTH;
            }
        }
        break;

    case AVDTP_SET_CFG:
        if (sep != NULL)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2;

            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG)
            {
                error_code = SEP_IN_USE;
            }
            if (pkt_len < 4)
            {
                error_code = BAD_LENGTH;
            }
            else if (pkt_len == 4)
            {
                error_code = BAD_SERV_CATEG;
                error_category_type = 0x00;
            }
            else
            {
                uint8_t *capability_ptr;
                uint8_t  capability_len;

                capability_ptr = signal_pkt + 4;
                capability_len = pkt_len - 4;
                while (capability_len != 0)
                {
                    if (*capability_ptr > SRV_CATEG_DELAY_REPORT)
                    {
                        error_code = BAD_SERV_CATEG;
                        error_category_type = *capability_ptr;
                        break;
                    }
                    else
                    {
                        if (*capability_ptr == SRV_CATEG_MEDIA_CODEC)
                        {
                            if (((*(capability_ptr + 2) >> 4) & 0x0F) != AUDIO_MEDIA_TYPE)
                            {
                                error_code = INVALID_CPBS;
                                break;
                            }
                        }
                        else if (*capability_ptr == SRV_CATEG_MEDIA_TRANS)
                        {
                            if (*(capability_ptr + 1) != MEDIA_TRANS_LOSC)
                            {
                                error_code = BAD_MEDIA_TRANSPORT_TYPE;
                                break;
                            }
                        }
                        capability_len -= *(capability_ptr + 1) + 2;
                        capability_ptr  = capability_ptr + * (capability_ptr + 1) + 2;
                    }
                }
            }
        }

        break;

    case AVDTP_GET_CFG:
        if (sep != NULL)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 1;

            if (pkt_len != 3)
            {
                error_code = BAD_LENGTH;
            }
        }
        break;

    case AVDTP_RECFG:
        if (sep != NULL)
        {
            uint8_t *capability_ptr;
            uint8_t  capability_len;

            rsp_len        = sizeof(T_AVDTP_SIG_HDR) + 2;
            capability_ptr = signal_pkt + 3;
            capability_len = pkt_len - 3;
            while (capability_len != 0)
            {
                switch (*capability_ptr)
                {
                case SRV_CATEG_CP:
                    capability_len -= *(capability_ptr + 1) + 2;
                    capability_ptr  = capability_ptr + * (capability_ptr + 1) + 2;
                    break;

                case SRV_CATEG_MEDIA_CODEC:
                    if (((*(capability_ptr + 2) >> 4) & 0x0F) != AUDIO_MEDIA_TYPE)
                    {
                        error_code = INVALID_CPBS;
                        error_category_type = *capability_ptr;
                    }
                    capability_len -= *(capability_ptr + 1) + 2;
                    capability_ptr  = capability_ptr + * (capability_ptr + 1) + 2;
                    break;

                case SRV_CATEG_MEDIA_TRANS:
                    error_code = INVALID_CPBS;
                    error_category_type = *capability_ptr;
                    capability_len = 0;
                    break;

                default:
                    error_code = BAD_SERV_CATEG;
                    error_category_type = *capability_ptr;
                    capability_len = 0;
                    break;
                }
            }
        }
        break;

    case AVDTP_OPEN:
        if (sep != NULL)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 1;

            if (pkt_len != 3)
            {
                error_code = BAD_LENGTH;
            }
        }
        break;

    case AVDTP_START:
        if (sep != NULL)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2;

            if (pkt_len < 3)
            {
                error_code = BAD_LENGTH;
            }
        }
        break;

    case AVDTP_CLOSE:
        if (sep != NULL)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 1;

            if (pkt_len != 3)
            {
                error_code = BAD_LENGTH;
            }
        }
        break;

    case AVDTP_SUSPEND:
        if (sep != NULL)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2;

            if (pkt_len < 3)
            {
                error_code = BAD_LENGTH;
            }
        }
        break;

    case AVDTP_SECURITY_CONTROL:
        if (sep != NULL)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 1;

            if (pkt_len < 3)
            {
                error_code = BAD_LENGTH;
            }
        }
        break;

    default:
        break;
    }

    if (error_code != 0)
    {
        uint8_t *rsp_pkt;

        rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                  link->sig_chann.cid,
                                  0,
                                  rsp_len,
                                  avdtp->data_offset,
                                  false);
        if (rsp_pkt != NULL)
        {
            uint8_t *p = &rsp_pkt[avdtp->data_offset];

            PROTOCOL_PRINT_WARN3("avdtp_cmd_error_check_proc: bd_addr %s, signal_id 0x%02x, cid 0x%04x",
                                 TRACE_BDADDR(link->bd_addr), signal_id, link->sig_chann.cid);

            BE_UINT8_TO_STREAM(p, (trans_label | (AVDTP_PKT_TYPE_SINGLE << 2) |
                                   AVDTP_MSG_TYPE_RSP_REJECT));
            BE_UINT8_TO_STREAM(p, signal_id);
            switch (signal_id)
            {
            case AVDTP_DISCOVER:
            case AVDTP_GET_CPBS:
            case AVDTP_GET_ALL_CPBS:
            case AVDTP_GET_CFG:
            case AVDTP_OPEN:
            case AVDTP_CLOSE:
            case AVDTP_SECURITY_CONTROL:
                BE_UINT8_TO_STREAM(p, error_code);
                break;
            case AVDTP_SET_CFG:
            case AVDTP_RECFG:
                BE_UINT8_TO_STREAM(p, error_category_type);
                BE_UINT8_TO_STREAM(p, error_code);
                break;
            case AVDTP_START:
            case AVDTP_SUSPEND:
                BE_UINT8_TO_STREAM(p, *(signal_pkt + 2));
                BE_UINT8_TO_STREAM(p, error_code);
                break;
            }
            mpa_send_l2c_data_req(rsp_pkt,
                                  avdtp->data_offset,
                                  link->sig_chann.cid,
                                  rsp_len,
                                  false);
        }

        return 1;
    }
    else
    {
        return 0;
    }
}

bool avdtp_signal_start_cfm(uint8_t bd_addr[6],
                            bool    accept)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t  rsp_len;
        uint8_t  trans_label;
        uint8_t *rsp_pkt;
        uint8_t *p;

        trans_label = link->sig_chann.rx_start_trans_label;

        if (accept == true)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR);
            rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                      link->sig_chann.cid,
                                      0,
                                      rsp_len,
                                      avdtp->data_offset,
                                      false);

            if (rsp_pkt != NULL)
            {
                p = &rsp_pkt[avdtp->data_offset];

                BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                       AVDTP_MSG_TYPE_RSP_ACCEPT));
                BE_UINT8_TO_STREAM(p, AVDTP_START);
                mpa_send_l2c_data_req(rsp_pkt,
                                      avdtp->data_offset,
                                      link->sig_chann.cid,
                                      rsp_len,
                                      false);

                link->sig_chann.sig_state = AVDTP_SIG_STATE_STREAMING;
                return true;
            }
        }
        else
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2;
            rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                      link->sig_chann.cid,
                                      0,
                                      rsp_len,
                                      avdtp->data_offset,
                                      false);

            if (rsp_pkt != NULL)
            {
                p = &rsp_pkt[avdtp->data_offset];
                BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                       AVDTP_MSG_TYPE_RSP_REJECT));
                BE_UINT8_TO_STREAM(p, AVDTP_START);
                BE_UINT8_TO_STREAM(p, link->sig_chann.int_seid);
                BE_UINT8_TO_STREAM(p, BAD_STATE);
                mpa_send_l2c_data_req(rsp_pkt,
                                      avdtp->data_offset,
                                      link->sig_chann.cid,
                                      rsp_len,
                                      false);
                return true;
            }
        }
    }

    return false;
}

void avdtp_rx_cmd_proc(T_AVDTP_LINK *link)
{
    uint16_t  pkt_len;
    uint8_t   trans_label;
    uint8_t   acp_seid;
    uint8_t   signal_id;
    uint8_t  *signal_pkt;
    uint8_t  *rsp_pkt;
    uint16_t  cid;

    signal_pkt  = link->sig_chann.fragment_data;
    pkt_len     = link->sig_chann.fragment_data_len;
    acp_seid    = (*(signal_pkt + 2) & 0xFC) >> 2;
    trans_label = *signal_pkt & 0xF0;
    signal_id   = *(signal_pkt + 1) & 0x3F;
    cid         = link->sig_chann.cid;

    PROTOCOL_PRINT_INFO3("avdtp_rx_cmd_proc: bd_addr %s, signal_id 0x%02x, cid 0x%04x",
                         TRACE_BDADDR(link->bd_addr), signal_id, cid);

    if (avdtp_cmd_error_check_proc(link) == false)
    {
        uint8_t *p;
        uint8_t  rsp_len;

        switch (signal_id)
        {
        case AVDTP_DISCOVER:
            {
                uint8_t            seid_in_use;
                T_AVDTP_LOCAL_SEP *sep;

                rsp_len = sizeof(T_AVDTP_SIG_HDR);

                rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                          cid,
                                          0,
                                          rsp_len + avdtp->local_sep_num * AVDTP_SEP_INFO_SIZE,
                                          avdtp->data_offset,
                                          false);
                if (rsp_pkt != NULL)
                {
                    p = &rsp_pkt[avdtp->data_offset];
                    BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                           AVDTP_MSG_TYPE_RSP_ACCEPT));
                    BE_UINT8_TO_STREAM(p, AVDTP_DISCOVER);

                    sep = os_queue_peek(&avdtp->local_sep_list, 0);
                    while (sep != NULL)
                    {
                        if (sep->in_use)
                        {
                            seid_in_use = SEID_IN_USE << 1;
                        }
                        else
                        {
                            seid_in_use = SEID_NOT_IN_USE << 1;
                        }

                        BE_UINT8_TO_STREAM(p, (sep->sep_id << 2) | seid_in_use);
                        BE_UINT8_TO_STREAM(p, (sep->media_type << 4) | (sep->tsep << 3));
                        rsp_len += AVDTP_SEP_INFO_SIZE;

                        sep = sep->next;
                    }

                    mpa_send_l2c_data_req(rsp_pkt,
                                          avdtp->data_offset,
                                          cid,
                                          rsp_len,
                                          false);
                }

                avdtp->cback(cid, AVDTP_MSG_SIG_DISCOVER_IND, NULL);
            }
            break;

        case AVDTP_GET_CPBS :
        case AVDTP_GET_ALL_CPBS:
            {
                T_AVDTP_LOCAL_SEP *sep;
                uint8_t            capability_len = 0;

                sep = avdtp_find_sep_by_id(acp_seid);

                if (sep == NULL)
                {
                    return;
                }

                if (avdtp->service_category & CATEG_MIDIA_TRANS_MASK)
                {
                    capability_len += MEDIA_TRANS_LOSC + 2;
                }
                if (avdtp->service_category & CATEG_MEDIA_CODEC_MASK)
                {
                    if (sep->sub_codec_type == CODEC_TYPE_SBC)
                    {
                        capability_len += MEDIA_CODEC_SBC_LOSC + 2;
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_AAC)
                    {
                        capability_len += MEDIA_CODEC_AAC_LOSC + 2;
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_USAC)
                    {
                        capability_len += MEDIA_CODEC_USAC_LOSC + 2;
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_LDAC)
                    {
                        capability_len += MEDIA_CODEC_LDAC_LOSC + 2;
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_LC3)
                    {
                        capability_len += MEDIA_CODEC_LC3_LOSC + 2;
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_LHDC)
                    {
                        capability_len += MEDIA_CODEC_LHDC_LOSC + 2;
                    }
                }
                if (avdtp->service_category & CATEG_CP_MASK)
                {
                    capability_len += CP_LOSC + 2;
                }
                if (signal_id == AVDTP_GET_ALL_CPBS)
                {
                    capability_len += DELAY_REPORT_LOSC + 2;
                }
                rsp_len = sizeof(T_AVDTP_SIG_HDR) + capability_len;

                rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                          cid,
                                          0,
                                          rsp_len,
                                          avdtp->data_offset,
                                          false);
                if (rsp_pkt != NULL)
                {
                    p = &rsp_pkt[avdtp->data_offset];
                    BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                           AVDTP_MSG_TYPE_RSP_ACCEPT));
                    BE_UINT8_TO_STREAM(p, signal_id);
                    if (avdtp->service_category & CATEG_MIDIA_TRANS_MASK)
                    {
                        BE_UINT8_TO_STREAM(p, SRV_CATEG_MEDIA_TRANS);
                        BE_UINT8_TO_STREAM(p, MEDIA_TRANS_LOSC);
                    }

                    if (avdtp->service_category & CATEG_MEDIA_CODEC_MASK)
                    {
                        BE_UINT8_TO_STREAM(p, SRV_CATEG_MEDIA_CODEC);

                        if (sep->sub_codec_type == CODEC_TYPE_SBC)
                        {
                            BE_UINT8_TO_STREAM(p, MEDIA_CODEC_SBC_LOSC);
                            BE_UINT8_TO_STREAM(p, (AUDIO_MEDIA_TYPE << 4));
                            BE_UINT8_TO_STREAM(p, SBC_MEDIA_CODEC_TYPE);
                            memcpy(p, &sep->media_codec_info[0], 4);
                            p += 4;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_AAC)
                        {
                            BE_UINT8_TO_STREAM(p, MEDIA_CODEC_AAC_LOSC);
                            BE_UINT8_TO_STREAM(p, (AUDIO_MEDIA_TYPE << 4));
                            BE_UINT8_TO_STREAM(p, AAC_MEDIA_CODEC_TYPE);
                            memcpy(p, &sep->media_codec_info[0], 6);
                            p += 6;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_USAC)
                        {
                            BE_UINT8_TO_STREAM(p, MEDIA_CODEC_USAC_LOSC);
                            BE_UINT8_TO_STREAM(p, (AUDIO_MEDIA_TYPE << 4));
                            BE_UINT8_TO_STREAM(p, USAC_MEDIA_CODEC_TYPE);
                            memcpy(p, &sep->media_codec_info[0], 7);
                            p += 7;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_LDAC)
                        {
                            BE_UINT8_TO_STREAM(p, MEDIA_CODEC_LDAC_LOSC);
                            BE_UINT8_TO_STREAM(p, (AUDIO_MEDIA_TYPE << 4));
                            BE_UINT8_TO_STREAM(p, VENDOR_CODEC_TYPE);
                            memcpy(p, &sep->media_codec_info[0], 8);
                            p += 8;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_LC3)
                        {
                            BE_UINT8_TO_STREAM(p, MEDIA_CODEC_LC3_LOSC);
                            BE_UINT8_TO_STREAM(p, (AUDIO_MEDIA_TYPE << 4));
                            BE_UINT8_TO_STREAM(p, VENDOR_CODEC_TYPE);
                            memcpy(p, &sep->media_codec_info[0], 9);
                            p += 9;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_LHDC)
                        {
                            BE_UINT8_TO_STREAM(p, MEDIA_CODEC_LHDC_LOSC);
                            BE_UINT8_TO_STREAM(p, (AUDIO_MEDIA_TYPE << 4));
                            BE_UINT8_TO_STREAM(p, VENDOR_CODEC_TYPE);
                            memcpy(p, &sep->media_codec_info[0], 11);
                            p += 11;
                        }
                    }

                    if (avdtp->service_category & CATEG_CP_MASK)
                    {
                        BE_UINT8_TO_STREAM(p, SRV_CATEG_CP);
                        BE_UINT8_TO_STREAM(p, CP_LOSC);
                        BE_UINT8_TO_STREAM(p, CP_TYPE_LSB);
                        BE_UINT8_TO_STREAM(p, CP_TYPE_MSB);
                    }

                    if (signal_id == AVDTP_GET_ALL_CPBS)
                    {
                        BE_UINT8_TO_STREAM(p, SRV_CATEG_DELAY_REPORT);
                        BE_UINT8_TO_STREAM(p, DELAY_REPORT_LOSC);
                    }
                    mpa_send_l2c_data_req(rsp_pkt,
                                          avdtp->data_offset,
                                          cid,
                                          rsp_len,
                                          false);
                }

                avdtp->cback(cid, AVDTP_MSG_SIG_GET_CPBS_IND, NULL);
            }
            break;

        case AVDTP_SET_CFG:
            {
                if (link->sig_chann.sig_state == AVDTP_SIG_STATE_IDLE)
                {
                    uint8_t            result;
                    T_AVDTP_LOCAL_SEP *sep;

                    sep = avdtp_find_sep_by_id(acp_seid);

                    if (sep == NULL)
                    {
                        return;
                    }

                    link->sig_chann.int_seid       = *(signal_pkt + 3);
                    link->sig_chann.enable_cp_flag = 0;

                    result = avdtp_config_parse(link,
                                                (uint8_t *)(signal_pkt + 4),
                                                pkt_len - 4,
                                                sep);

                    if (result != 0)
                    {
                        rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2;
                        rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                                  cid,
                                                  0,
                                                  rsp_len,
                                                  avdtp->data_offset,
                                                  false);

                        if (rsp_pkt != NULL)
                        {
                            p = &rsp_pkt[avdtp->data_offset];
                            BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                                   AVDTP_MSG_TYPE_RSP_REJECT));
                            BE_UINT8_TO_STREAM(p, AVDTP_SET_CFG);
                            BE_UINT8_TO_STREAM(p, SRV_CATEG_MEDIA_CODEC);
                            BE_UINT8_TO_STREAM(p, result);
                            mpa_send_l2c_data_req(rsp_pkt,
                                                  avdtp->data_offset,
                                                  cid,
                                                  rsp_len,
                                                  false);
                        }
                    }
                    else
                    {
                        uint8_t ind_data[4 + MAX_CODEC_INFO_SIZE];

                        if (link->sig_chann.cfg_cpbs != NULL)
                        {
                            free(link->sig_chann.cfg_cpbs);
                        }
                        link->sig_chann.cpbs_len = pkt_len - 4;
                        link->sig_chann.cfg_cpbs = malloc(pkt_len - 4);
                        if (link->sig_chann.cfg_cpbs != NULL)
                        {
                            memcpy(link->sig_chann.cfg_cpbs, signal_pkt + 4, pkt_len - 4);

                            rsp_len = sizeof(T_AVDTP_SIG_HDR);
                            rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                                      cid,
                                                      0,
                                                      rsp_len,
                                                      avdtp->data_offset,
                                                      false);
                            if (rsp_pkt != NULL)
                            {
                                p = &rsp_pkt[avdtp->data_offset];
                                BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                                       AVDTP_MSG_TYPE_RSP_ACCEPT));
                                BE_UINT8_TO_STREAM(p, AVDTP_SET_CFG);
                                mpa_send_l2c_data_req(rsp_pkt,
                                                      avdtp->data_offset,
                                                      cid,
                                                      rsp_len,
                                                      false);
                            }

                            link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;
                            link->sig_chann.int_flag  = 0;
                            link->local_sep_role      = sep->tsep;

                            ind_data[0] = link->local_sep_role;
                            ind_data[1] = link->sig_chann.codec_type;
                            ind_data[2] = link->sig_chann.enable_cp_flag;
                            ind_data[3] = link->sig_chann.delay_report_flag;
                            memcpy(&ind_data[4], &link->sig_chann.codec_info[0], MAX_CODEC_INFO_SIZE);
                            avdtp->cback(cid, AVDTP_MSG_SIG_SET_CFG_IND, &ind_data[0]);
                        }
                    }
                }
                else
                {
                    avdtp_cmd_bad_state_proc(link->bd_addr);
                }
            }
            break;

        case AVDTP_GET_CFG:
            {
                rsp_len = sizeof(T_AVDTP_SIG_HDR) + link->sig_chann.cpbs_len;
                rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                          link->sig_chann.cid,
                                          0,
                                          rsp_len,
                                          avdtp->data_offset,
                                          false);
                if (rsp_pkt != NULL)
                {
                    p = &rsp_pkt[avdtp->data_offset];
                    BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                           AVDTP_MSG_TYPE_RSP_ACCEPT));
                    BE_UINT8_TO_STREAM(p, AVDTP_GET_CFG);
                    memcpy(p, (uint8_t *)link->sig_chann.cfg_cpbs, link->sig_chann.cpbs_len);
                    mpa_send_l2c_data_req(rsp_pkt,
                                          avdtp->data_offset,
                                          link->sig_chann.cid,
                                          rsp_len,
                                          false);
                }

                avdtp->cback(cid, AVDTP_MSG_SIG_GET_CFG_IND, NULL);
            }
            break;

        case AVDTP_RECFG:
            {
                if (link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
                {
                    uint8_t            result;
                    T_AVDTP_LOCAL_SEP *sep;

                    sep = avdtp_find_sep_by_id(acp_seid);

                    if (sep == NULL)
                    {
                        return;
                    }

                    link->sig_chann.enable_cp_flag = 0;

                    result = avdtp_config_parse(link,
                                                (uint8_t *)(signal_pkt + 3),
                                                pkt_len - 3,
                                                sep);

                    if (result != 0)
                    {
                        rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2;
                        rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                                  cid,
                                                  0,
                                                  rsp_len,
                                                  avdtp->data_offset,
                                                  false);

                        if (rsp_pkt != NULL)
                        {
                            p = &rsp_pkt[avdtp->data_offset];
                            BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                                   AVDTP_MSG_TYPE_RSP_REJECT));
                            BE_UINT8_TO_STREAM(p, AVDTP_RECFG);
                            BE_UINT8_TO_STREAM(p, SRV_CATEG_MEDIA_CODEC);
                            BE_UINT8_TO_STREAM(p, result);
                            mpa_send_l2c_data_req(rsp_pkt,
                                                  avdtp->data_offset,
                                                  cid,
                                                  rsp_len,
                                                  false);
                        }
                    }
                    else
                    {
                        uint8_t ind_data[4 + MAX_CODEC_INFO_SIZE];

                        if (link->sig_chann.cfg_cpbs != NULL)
                        {
                            free(link->sig_chann.cfg_cpbs);
                        }
                        link->sig_chann.cpbs_len = pkt_len - 3;
                        link->sig_chann.cfg_cpbs = malloc(pkt_len - 3);
                        if (link->sig_chann.cfg_cpbs != NULL)
                        {
                            memcpy(link->sig_chann.cfg_cpbs, signal_pkt + 3, pkt_len - 3);

                            rsp_len = sizeof(T_AVDTP_SIG_HDR);
                            rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                                      link->sig_chann.cid,
                                                      0,
                                                      rsp_len,
                                                      avdtp->data_offset,
                                                      false);
                            if (rsp_pkt != NULL)
                            {
                                p = &rsp_pkt[avdtp->data_offset];
                                BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                                       AVDTP_MSG_TYPE_RSP_ACCEPT));
                                BE_UINT8_TO_STREAM(p, AVDTP_RECFG);
                                mpa_send_l2c_data_req(rsp_pkt,
                                                      avdtp->data_offset,
                                                      link->sig_chann.cid,
                                                      rsp_len,
                                                      false);
                            }

                            link->local_sep_role = sep->tsep;

                            ind_data[0] = link->local_sep_role;
                            ind_data[1] = link->sig_chann.codec_type;
                            ind_data[2] = link->sig_chann.enable_cp_flag;
                            ind_data[3] = link->sig_chann.delay_report_flag;
                            memcpy(&ind_data[4], &link->sig_chann.codec_info[0],
                                   MAX_CODEC_INFO_SIZE);
                            avdtp->cback(cid, AVDTP_MSG_SIG_RECFG_IND, &ind_data[0]);
                        }
                    }
                }
                else
                {
                    avdtp_cmd_bad_state_proc(link->bd_addr);
                }
            }
            break;

        case AVDTP_OPEN:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG)
            {
                rsp_len = sizeof(T_AVDTP_SIG_HDR);
                rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                          link->sig_chann.cid,
                                          0,
                                          rsp_len,
                                          avdtp->data_offset,
                                          false);
                if (rsp_pkt != NULL)
                {
                    p = &rsp_pkt[avdtp->data_offset];
                    BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                           AVDTP_MSG_TYPE_RSP_ACCEPT));
                    BE_UINT8_TO_STREAM(p, AVDTP_OPEN);
                    mpa_send_l2c_data_req(rsp_pkt,
                                          avdtp->data_offset,
                                          link->sig_chann.cid,
                                          rsp_len,
                                          false);
                }

                link->sig_chann.sig_state = AVDTP_SIG_STATE_OPENING;
                avdtp->cback(cid, AVDTP_MSG_SIG_OPEN_IND, NULL);
            }
            else
            {
                avdtp_cmd_bad_state_proc(link->bd_addr);
            }
            break;

        case AVDTP_START:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
            {
                link->sig_chann.rx_start_trans_label = trans_label;
                avdtp->cback(cid, AVDTP_MSG_SIG_START_IND, NULL);
            }
            else
            {
                avdtp_cmd_bad_state_proc(link->bd_addr);
            }
            break;

        case AVDTP_CLOSE:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING ||
                link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN ||
                link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                rsp_len = sizeof(T_AVDTP_SIG_HDR);
                rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                          link->sig_chann.cid,
                                          0,
                                          rsp_len,
                                          avdtp->data_offset,
                                          false);
                if (rsp_pkt != NULL)
                {
                    p = &rsp_pkt[avdtp->data_offset];
                    BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                           AVDTP_MSG_TYPE_RSP_ACCEPT));
                    BE_UINT8_TO_STREAM(p, AVDTP_CLOSE);
                    mpa_send_l2c_data_req(rsp_pkt,
                                          avdtp->data_offset,
                                          link->sig_chann.cid,
                                          rsp_len,
                                          false);
                }
                link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                avdtp->cback(cid, AVDTP_MSG_SIG_CLOSE_IND, NULL);
            }
            else
            {
                avdtp_cmd_bad_state_proc(link->bd_addr);
            }
            break;

        case AVDTP_SUSPEND:
            rsp_len = sizeof(T_AVDTP_SIG_HDR);
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                          link->sig_chann.cid,
                                          0,
                                          rsp_len,
                                          avdtp->data_offset,
                                          false);
                if (rsp_pkt != NULL)
                {
                    p = &rsp_pkt[avdtp->data_offset];
                    BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                           AVDTP_MSG_TYPE_RSP_ACCEPT));
                    BE_UINT8_TO_STREAM(p, AVDTP_SUSPEND);
                    mpa_send_l2c_data_req(rsp_pkt,
                                          avdtp->data_offset,
                                          link->sig_chann.cid,
                                          rsp_len,
                                          false);
                }

                link->sig_chann.sig_state = AVDTP_SIG_STATE_OPEN;
                avdtp->cback(cid, AVDTP_MSG_SIG_SUSPEND_IND, NULL);
            }
            else
            {
                avdtp_cmd_bad_state_proc(link->bd_addr);
            }
            break;

        case AVDTP_ABORT:
            {
                rsp_len = sizeof(T_AVDTP_SIG_HDR);
                rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                          link->sig_chann.cid,
                                          0,
                                          rsp_len,
                                          avdtp->data_offset,
                                          false);
                if (rsp_pkt != NULL)
                {
                    p = &rsp_pkt[avdtp->data_offset];
                    BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                           AVDTP_MSG_TYPE_RSP_ACCEPT));
                    BE_UINT8_TO_STREAM(p, AVDTP_ABORT);
                    mpa_send_l2c_data_req(rsp_pkt,
                                          avdtp->data_offset,
                                          link->sig_chann.cid,
                                          rsp_len,
                                          false);
                }

                link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                avdtp->cback(cid, AVDTP_MSG_SIG_ABORT_IND, NULL);
            }
            break;

        case AVDTP_SECURITY_CONTROL:
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + (pkt_len - 3);
            rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                      cid,
                                      0,
                                      rsp_len,
                                      avdtp->data_offset,
                                      false);
            if (rsp_pkt != NULL)
            {
                p = &rsp_pkt[avdtp->data_offset];
                BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                       AVDTP_MSG_TYPE_RSP_ACCEPT));
                BE_UINT8_TO_STREAM(p, signal_id);
                if (pkt_len - 3 > 0)
                {
                    memcpy(p, &signal_pkt[3], pkt_len - 3);
                    p += pkt_len - 3;
                }
                mpa_send_l2c_data_req(rsp_pkt,
                                      avdtp->data_offset,
                                      cid,
                                      rsp_len,
                                      false);
            }

            avdtp->cback(cid, AVDTP_MSG_SIG_SECURITY_CONTROL_IND, NULL);
            break;

        case AVDTP_DELAY_REPORT:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG ||
                link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING ||
                link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN ||
                link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                uint16_t delay;

                delay = ((uint16_t)(*(signal_pkt + 3)) << 8) + (uint16_t)(*(signal_pkt + 4));
                rsp_len = sizeof(T_AVDTP_SIG_HDR);
                rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                          link->sig_chann.cid,
                                          0,
                                          rsp_len,
                                          avdtp->data_offset,
                                          false);
                if (rsp_pkt != NULL)
                {
                    p = &rsp_pkt[avdtp->data_offset];
                    BE_UINT8_TO_STREAM(p, trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                       AVDTP_MSG_TYPE_RSP_ACCEPT);
                    BE_UINT8_TO_STREAM(p, signal_id);
                    mpa_send_l2c_data_req(rsp_pkt,
                                          avdtp->data_offset,
                                          link->sig_chann.cid,
                                          rsp_len,
                                          false);
                }

                avdtp->cback(cid, AVDTP_MSG_SIG_DELAY_REPORT_IND, &delay);
            }
            else
            {
                avdtp_cmd_bad_state_proc(link->bd_addr);
            }
            break;

        default:
            rsp_len = sizeof(T_AVDTP_SIG_HDR); /* ACP SEID and error code */
            rsp_pkt = mpa_get_l2c_buf(avdtp->queue_id,
                                      cid,
                                      0,
                                      rsp_len,
                                      avdtp->data_offset,
                                      false);
            if (rsp_pkt != NULL)
            {
                p = &rsp_pkt[avdtp->data_offset];
                BE_UINT8_TO_STREAM(p, (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                       AVDTP_MSG_TYPE_GENERAL_REJECT));
                BE_UINT8_TO_STREAM(p, signal_id);
                mpa_send_l2c_data_req(rsp_pkt,
                                      avdtp->data_offset,
                                      cid,
                                      rsp_len,
                                      false);
            }
            break;
        }
    }
}

void avdtp_rx_rsp_proc(T_AVDTP_LINK *link)
{
    uint8_t  signal_id;
    uint8_t *signal_pkt;
    uint16_t pkt_len;
    uint8_t  temp;

    signal_pkt = link->sig_chann.fragment_data;
    pkt_len    = link->sig_chann.fragment_data_len;
    signal_id  = *(signal_pkt + 1) & 0x3F;

    PROTOCOL_PRINT_INFO4("avdtp_rx_rsp_proc: bd_addr %s, signal_id 0x%02x, msg_type 0x%02x, cmd_flag 0x%02x",
                         TRACE_BDADDR(link->bd_addr), signal_id, *signal_pkt & 0x03,
                         link->sig_chann.cmd_flag);

    if (signal_id != AVDTP_DELAY_REPORT)
    {
        if (link->sig_chann.cmd_flag != signal_id)
        {
            return;
        }
        link->sig_chann.cmd_flag = 0;
    }
    sys_timer_stop(link->timer_handle);

    if ((*signal_pkt & 0x03) == AVDTP_MSG_TYPE_RSP_ACCEPT)
    {
        switch (signal_id)
        {
        case AVDTP_DISCOVER:
            {
                uint8_t acp_seid_status;
                uint8_t sep_type;
                uint8_t i;

                link->sig_chann.acp_sep_no = 0;
                if (pkt_len >= 4)
                {
                    temp = (pkt_len - 2) >> 1;
                    for (i = 0; i < temp; i++)
                    {
                        acp_seid_status = *(signal_pkt + 2 + (i << 1)) & 0x02;
                        sep_type        = *(signal_pkt + 3 + (i << 1)) & 0xF8;
                        if (acp_seid_status == SEID_NOT_IN_USE)
                        {
                            acp_seid_status = *(signal_pkt + 2 + (i << 1)) & 0x02;
                            sep_type        = *(signal_pkt + 3 + (i << 1)) & 0xF8;

                            if (acp_seid_status == SEID_NOT_IN_USE)
                            {
                                if (link->conn_role == AVDTP_TSEP_SRC)
                                {
                                    if (sep_type == AUDIO_SRC)
                                    {
                                        link->sig_chann.acp_sep_no++;
                                    }
                                }
                                else if (link->conn_role == AVDTP_TSEP_SNK)
                                {
                                    if (sep_type == AUDIO_SNK)
                                    {
                                        link->sig_chann.acp_sep_no++;
                                    }
                                }
                            }
                        }
                    }
                }

                if (link->sig_chann.acp_sep_no)
                {
                    uint8_t j = 0;

                    if (link->sig_chann.avdtp_sep != NULL)
                    {
                        free(link->sig_chann.avdtp_sep);
                    }
                    temp = link->sig_chann.acp_sep_no * sizeof(T_AVDTP_REMOTE_SEP);
                    link->sig_chann.avdtp_sep = malloc(temp);
                    if (link->sig_chann.avdtp_sep == NULL)
                    {
                        return;
                    }

                    temp = (pkt_len - 2) >> 1;
                    for (i = 0; i < temp; i++)
                    {
                        acp_seid_status = *(signal_pkt + 2 + (i << 1)) & 0x02;
                        sep_type        = *(signal_pkt + 3 + (i << 1)) & 0xF8;

                        if (acp_seid_status == SEID_NOT_IN_USE)
                        {
                            if (link->conn_role == AVDTP_TSEP_SRC)
                            {
                                if (sep_type == AUDIO_SRC)
                                {
                                    link->sig_chann.avdtp_sep[j].sep_id =
                                        *(signal_pkt + 2 + (i << 1)) & 0xFC;
                                    j++;
                                    if (j >= link->sig_chann.acp_sep_no)
                                    {
                                        break;
                                    }
                                }
                            }
                            else if (link->conn_role == AVDTP_TSEP_SNK)
                            {
                                if (sep_type == AUDIO_SNK)
                                {
                                    link->sig_chann.avdtp_sep[j].sep_id =
                                        *(signal_pkt + 2 + (i << 1)) & 0xFC;
                                    j++;
                                    if (j >= link->sig_chann.acp_sep_no)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    link->sig_chann.acp_seid_idx = 0;
                    avdtp_signal_get_capability_req(link->bd_addr);
                    avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_DISCOVER_RSP, NULL);
                }
                else
                {
                    avdtp_signal_disconnect_req(link->bd_addr);
                }
            }
            break;

        case AVDTP_GET_CPBS:
        case AVDTP_GET_ALL_CPBS:
            {
                avdtp_remote_capability_parse(&link->sig_chann.avdtp_sep[link->sig_chann.acp_seid_idx],
                                              (uint8_t *)(signal_pkt + 2),
                                              pkt_len - 2);
                link->sig_chann.acp_seid_idx++;
                if (link->sig_chann.acp_seid_idx == link->sig_chann.acp_sep_no)
                {
                    uint8_t i;

                    for (i = 0; i < link->sig_chann.acp_sep_no; i++)
                    {
                        if (link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                        {
                            if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                            {
                                if (avdtp_vendor_codec_ldac_handler(link->conn_role,
                                                                    HDL_TYPE_CHECK_CODEC,
                                                                    link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                    NULL,
                                                                    NULL) == true)
                                {
                                    break;
                                }
                            }
                            else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                            {
                                if (avdtp_vendor_codec_lc3_handler(link->conn_role,
                                                                   HDL_TYPE_CHECK_CODEC,
                                                                   link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                   NULL,
                                                                   NULL) == true)
                                {
                                    break;
                                }
                            }
                            else if (link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                            {
                                if (avdtp_vendor_codec_lhdc_handler(link->conn_role,
                                                                    HDL_TYPE_CHECK_CODEC,
                                                                    link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                    NULL,
                                                                    NULL) == true)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    if (i == link->sig_chann.acp_sep_no)
                    {
                        for (i = 0; i < link->sig_chann.acp_sep_no; i++)
                        {
                            if (link->sig_chann.avdtp_sep[i].media_codec_type == USAC_MEDIA_CODEC_TYPE)
                            {
                                if (avdtp_codec_usac_handler(link->conn_role,
                                                             HDL_TYPE_CHECK_CODEC,
                                                             link->sig_chann.avdtp_sep[i].media_codec_info,
                                                             NULL,
                                                             NULL) == true)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    if (i == link->sig_chann.acp_sep_no)
                    {
                        for (i = 0; i < link->sig_chann.acp_sep_no; i++)
                        {
                            if (link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
                            {
                                if (avdtp_codec_aac_handler(link->conn_role,
                                                            HDL_TYPE_CHECK_CODEC,
                                                            link->sig_chann.avdtp_sep[i].media_codec_info,
                                                            NULL,
                                                            NULL) == true)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    if (i == link->sig_chann.acp_sep_no)
                    {
                        for (i = 0; i < link->sig_chann.acp_sep_no; i++)
                        {
                            if (link->sig_chann.avdtp_sep[i].media_codec_type == SBC_MEDIA_CODEC_TYPE)
                            {
                                if (avdtp_codec_sbc_handler(link->conn_role,
                                                            HDL_TYPE_CHECK_CODEC,
                                                            link->sig_chann.avdtp_sep[i].media_codec_info,
                                                            NULL,
                                                            NULL) == true)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    if (i < link->sig_chann.acp_sep_no)
                    {
                        link->sig_chann.acp_seid_idx = i;
                        link->sig_chann.acp_seid     = link->sig_chann.avdtp_sep[i].sep_id;

                        if (link->sig_chann.sig_state == AVDTP_SIG_STATE_IDLE)
                        {
                            avdtp_signal_cfg_req(link->bd_addr);
                        }
                        else if (link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
                        {
                            avdtp_signal_recfg_req(link->bd_addr,
                                                   link->sig_chann.recfg_codec_type,
                                                   link->conn_role);
                        }
                    }
                    else
                    {
                        link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                        link->sig_chann.int_flag = 0;

                        avdtp_signal_disconnect_req(link->bd_addr);
                    }
                }
                else
                {
                    avdtp_signal_get_capability_req(link->bd_addr);
                }

                avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_GET_CPBS_RSP, NULL);
            }
            break;

        case AVDTP_GET_CFG:
            avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_GET_CFG_RSP, NULL);
            break;

        case AVDTP_RECFG:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
            {
                uint8_t ind_data[4 + MAX_CODEC_INFO_SIZE];

                ind_data[0] = link->local_sep_role;
                ind_data[1] = link->sig_chann.codec_type;
                ind_data[2] = link->sig_chann.enable_cp_flag;
                ind_data[3] = link->sig_chann.delay_report_flag;
                memcpy(&ind_data[4], &link->sig_chann.codec_info[0], MAX_CODEC_INFO_SIZE);
                avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_RECFG_RSP, &ind_data[0]);
            }
            break;

        case AVDTP_SET_CFG:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_IDLE)
            {
                uint8_t ind_data[4 + MAX_CODEC_INFO_SIZE];

                link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;

                ind_data[0] = link->local_sep_role;
                ind_data[1] = link->sig_chann.codec_type;
                ind_data[2] = link->sig_chann.enable_cp_flag;
                ind_data[3] = link->sig_chann.delay_report_flag;
                memcpy(&ind_data[4], &link->sig_chann.codec_info[0], MAX_CODEC_INFO_SIZE);
                avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_SET_CFG_RSP, &ind_data[0]);

                avdtp_signal_open_req(link->bd_addr, link->conn_role);
            }
            break;

        case AVDTP_OPEN:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING)
            {
                avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_OPEN_RSP, NULL);
            }
            break;

        case AVDTP_START:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
            {
                link->sig_chann.sig_state = AVDTP_SIG_STATE_STREAMING;
                avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_START_RSP, NULL);
            }
            break;

        case AVDTP_CLOSE:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN ||
                link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                link->sig_chann.int_flag  = 0;
                link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_CLOSE_RSP, NULL);
            }
            break;

        case AVDTP_ABORT:
            {
                link->sig_chann.int_flag  = 0;
                link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_ABORT_RSP, NULL);
            }
            break;

        case AVDTP_SUSPEND:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                link->sig_chann.sig_state = AVDTP_SIG_STATE_OPEN;
                avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_SUSPEND_RSP, NULL);
            }
            break;

        case AVDTP_DELAY_REPORT:
            if (link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG ||
                link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING ||
                link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN ||
                link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                avdtp->cback(link->sig_chann.cid, AVDTP_MSG_SIG_DELAY_REPORT_RSP, NULL);
            }
            break;

        default:
            break;
        }
    }
    else if ((*signal_pkt & 0x03) == AVDTP_MSG_TYPE_RSP_REJECT)
    {
        switch (signal_id)
        {
        case AVDTP_OPEN:
            {
                avdtp_signal_abort_req(link->bd_addr);
            }
            break;

        case AVDTP_GET_CPBS:
        case AVDTP_GET_ALL_CPBS:
            {
                link->sig_chann.acp_seid_idx++;
                if (link->sig_chann.acp_seid_idx < link->sig_chann.acp_sep_no)
                {
                    avdtp_signal_get_capability_req(link->bd_addr);
                }
            }
            break;

        case AVDTP_CLOSE:
            {
                avdtp_signal_abort_req(link->bd_addr);
            }
            break;

        case AVDTP_ABORT:
            {
                if (link->strm_chann.state == AVDTP_STATE_CONNECTED)
                {
                    avdtp_stream_disconnect_req(link->bd_addr);
                }

                avdtp_signal_disconnect_req(link->bd_addr);
            }
            break;

        case AVDTP_RECFG:
            {
                avdtp_signal_disconnect_req(link->bd_addr);
            }
            break;

        case AVDTP_SET_CFG:
            {
                avdtp_signal_disconnect_req(link->bd_addr);
            }
            break;

        default:
            break;
        }
    }
}

void avdtp_signal_proc(T_AVDTP_LINK *link)
{
    uint8_t *pkt;
    uint8_t  msg_type;

    pkt      = link->sig_chann.fragment_data;
    msg_type = *pkt & 0x03;
    if (msg_type == AVDTP_MSG_TYPE_CMD)
    {
        avdtp_rx_cmd_proc(link);
    }
    else
    {
        avdtp_rx_rsp_proc(link);
    }
}

void avdtp_handle_signal_ind(T_AVDTP_LINK       *link,
                             T_MPA_L2C_DATA_IND *data_ind)
{
    uint8_t  *pkt;
    uint16_t  pkt_len;
    uint8_t   pkt_type;

    pkt     = data_ind->data + data_ind->gap;
    pkt_len = data_ind->length;

    pkt_type = (*pkt & AVDTP_PKT_TYPE_MASK) >> 2;

    switch (pkt_type)
    {
    case AVDTP_PKT_TYPE_SINGLE:
        if (link->sig_chann.fragment_data != NULL)
        {
            free(link->sig_chann.fragment_data);
        }
        link->sig_chann.fragment_data = pkt;
        link->sig_chann.fragment_data_len = pkt_len;
        avdtp_signal_proc(link);
        link->sig_chann.fragment_data = NULL;
        break;

    case AVDTP_PKT_TYPE_START:
        {
            if (pkt_len > sizeof(T_AVDTP_START_PKT_HDR))
            {
                T_AVDTP_START_PKT *start_pkt;
                uint8_t            pkt_num;
                uint16_t           data_len;
                uint16_t           total_len;

                start_pkt = (T_AVDTP_START_PKT *)pkt;
                pkt_num   = start_pkt->pkt_hdr.pkt_num;
                data_len  = pkt_len - sizeof(T_AVDTP_START_PKT_HDR);
                total_len = data_len * pkt_num;
                if (link->sig_chann.fragment_data != NULL)
                {
                    free(link->sig_chann.fragment_data);
                }
                link->sig_chann.fragment_data = malloc(total_len + 2);
                if (link->sig_chann.fragment_data == NULL)
                {
                    return;
                }
                link->sig_chann.fragment_data[0] = start_pkt->pkt_hdr.trans_info;
                link->sig_chann.fragment_data[1] = start_pkt->pkt_hdr.signal_id;
                memcpy(&link->sig_chann.fragment_data[2],
                       (uint8_t *)(&start_pkt->pkt_data[0]), data_len);
                link->sig_chann.fragment_data_len = data_len + 2;
            }
        }
        break;

    case AVDTP_PKT_TYPE_CONTINUE:
    case AVDTP_PKT_TYPE_END:
        {
            T_AVDTP_CONTINUE_PKT *cont_end_pkt;

            if (link->sig_chann.fragment_data == NULL)
            {
                return;
            }

            cont_end_pkt = (T_AVDTP_CONTINUE_PKT *)pkt;
            if ((uint8_t)(cont_end_pkt->pkt_hdr.trans_info & 0xF0) ==
                (link->sig_chann.fragment_data[0] & 0xF0))
            {
                uint16_t data_len;

                data_len = pkt_len - sizeof(T_AVDTP_CONTINUE_PKT_HDR);
                memcpy(&link->sig_chann.fragment_data[(link->sig_chann.fragment_data_len)],
                       (uint8_t *)(&cont_end_pkt->pkt_data[0]), data_len);
                link->sig_chann.fragment_data_len += data_len;
            }

            if (pkt_type == AVDTP_PKT_TYPE_END)
            {
                avdtp_signal_proc(link);
                free(link->sig_chann.fragment_data);
                link->sig_chann.fragment_data = NULL;
            }
        }
        break;
    }
}

void avdtp_handle_data_ind(T_MPA_L2C_DATA_IND *data_ind)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_cid(data_ind->cid);
    if (link != NULL)
    {
        if (link->sig_chann.cid == data_ind->cid)
        {
            avdtp_handle_signal_ind(link, data_ind);
        }
        else
        {
            avdtp->cback(data_ind->cid, AVDTP_MSG_STREAM_DATA_IND, data_ind);
        }
    }
}

void avdtp_handle_data_rsp(T_MPA_L2C_DATA_RSP *data_rsp)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_cid(data_rsp->cid);
    if (link != NULL)
    {
        avdtp->cback(data_rsp->cid, AVDTP_MSG_STREAM_DATA_RSP, link->bd_addr);
    }
}

void avdtp_handle_conn_req(T_MPA_L2C_CONN_IND *conn_ind)
{
    T_AVDTP_LINK *link;

    PROTOCOL_PRINT_TRACE2("avdtp_handle_conn_req: bd_addr %s, cid 0x%04x",
                          TRACE_BDADDR(conn_ind->bd_addr), conn_ind->cid);

    link = avdtp_find_link_by_addr(conn_ind->bd_addr);
    if (link == NULL)
    {
        link = avdtp_alloc_link(conn_ind->bd_addr);
        if (link != NULL)
        {
            link->sig_chann.cid   = conn_ind->cid;
            link->sig_chann.state = AVDTP_STATE_CONNECTING;
            avdtp->cback(conn_ind->cid, AVDTP_MSG_SIG_CONN_REQ, conn_ind->bd_addr);
        }
        else
        {
            mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                                  conn_ind->cid,
                                  AVDTP_SIG_MTU,
                                  MPA_L2C_MODE_BASIC,
                                  0xFFFF);
        }
    }
    else
    {
        T_MPA_L2C_CONN_CFM_CAUSE rsp;

        if (link->sig_chann.state == AVDTP_STATE_CONNECTED &&
            link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING &&
            link->strm_chann.state == AVDTP_STATE_DISCONNECTED)
        {
            rsp = MPA_L2C_CONN_ACCEPT;

            link->strm_chann.cid   = conn_ind->cid;
            link->strm_chann.state = AVDTP_STATE_CONNECTING;
            avdtp->cback(conn_ind->cid, AVDTP_MSG_STREAM_CONN_REQ, conn_ind->bd_addr);
        }
        else
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;

            link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;
        }

        if (link->strm_chann.mtu_size == 0)
        {
            if (link->sig_chann.codec_type != VENDOR_CODEC_TYPE)
            {
                mpa_send_l2c_conn_cfm(rsp,
                                      conn_ind->cid,
                                      AVDTP_STREAM_MTU,
                                      MPA_L2C_MODE_BASIC,
                                      0xFFFF);
            }
            else
            {
                mpa_send_l2c_conn_cfm(rsp,
                                      conn_ind->cid,
                                      AVDTP_LDAC_STREAM_MTU,
                                      MPA_L2C_MODE_BASIC,
                                      0xFFFF);
            }
        }
        else
        {
            mpa_send_l2c_conn_cfm(rsp,
                                  conn_ind->cid,
                                  link->strm_chann.mtu_size,
                                  MPA_L2C_MODE_BASIC,
                                  0xFFFF);
        }
    }
}

bool avdtp_signal_connect_cfm(uint8_t  bd_addr[6],
                              uint16_t mtu_size,
                              bool     accept)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint16_t                 cid;
        T_MPA_L2C_CONN_CFM_CAUSE rsp;

        cid = link->sig_chann.cid;
        rsp = MPA_L2C_CONN_ACCEPT;
        link->strm_chann.mtu_size = mtu_size;

        if (accept == false)
        {
            avdtp_free_link(link);
            rsp = MPA_L2C_CONN_NO_RESOURCE;
        }

        mpa_send_l2c_conn_cfm(rsp, cid, AVDTP_SIG_MTU, MPA_L2C_MODE_BASIC, 0xFFFF);
        return true;
    }

    return false;
}

void avdtp_handle_conn_rsp(T_MPA_L2C_CONN_RSP *conn_rsp)
{
    T_AVDTP_LINK *link;

    PROTOCOL_PRINT_TRACE2("avdtp_handle_conn_rsp: cid 0x%04x, status 0x%04x",
                          conn_rsp->cid, conn_rsp->cause);

    link = avdtp_find_link_by_addr(conn_rsp->bd_addr);
    if (link != NULL)
    {
        if (conn_rsp->cause == 0)
        {
            if (link->sig_chann.state == AVDTP_STATE_CONNECTING)
            {
                link->sig_chann.cid = conn_rsp->cid;
                avdtp->cback(conn_rsp->cid, AVDTP_MSG_SIG_CONN_RSP, conn_rsp->bd_addr);
            }
            else if (link->strm_chann.state == AVDTP_STATE_CONNECTING)
            {
                link->strm_chann.cid = conn_rsp->cid;
                avdtp->cback(conn_rsp->cid, AVDTP_MSG_STREAM_CONN_RSP, conn_rsp->bd_addr);
            }
        }
        else
        {
            uint16_t info = conn_rsp->cause;

            if (link->sig_chann.state == AVDTP_STATE_CONNECTING)
            {
                avdtp_free_link(link);
                avdtp->cback(conn_rsp->cid, AVDTP_MSG_SIG_CONN_FAIL, &info);
            }
            else if (link->strm_chann.state == AVDTP_STATE_CONNECTING)
            {
                link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;
                link->strm_chann.state    = AVDTP_STATE_DISCONNECTED;
                avdtp->cback(conn_rsp->cid, AVDTP_MSG_STREAM_CONN_FAIL, &info);
            }
        }
    }
}

void avdtp_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *conn_cmpl)
{
    T_AVDTP_LINK *link;

    PROTOCOL_PRINT_TRACE2("avdtp_handle_conn_cmpl: cid 0x%04x, status 0x%04x",
                          conn_cmpl->cid, conn_cmpl->cause);

    link = avdtp_find_link_by_cid(conn_cmpl->cid);
    if (link != NULL)
    {
        if (conn_cmpl->cause == 0)
        {
            if (link->sig_chann.cid == conn_cmpl->cid)
            {
                link->sig_chann.sig_state  = AVDTP_SIG_STATE_IDLE;
                link->sig_chann.state      = AVDTP_STATE_CONNECTED;
                link->sig_chann.remote_mtu = conn_cmpl->remote_mtu;
                avdtp->data_offset         = conn_cmpl->ds_data_offset;
                avdtp->cback(conn_cmpl->cid, AVDTP_MSG_SIG_CONNECTED, conn_cmpl);
            }
            else
            {
                link->sig_chann.sig_state = AVDTP_SIG_STATE_OPEN;
                link->strm_chann.state    = AVDTP_STATE_CONNECTED;
                avdtp->cback(conn_cmpl->cid, AVDTP_MSG_STREAM_CONNECTED, conn_cmpl);
            }
        }
        else
        {
            uint16_t info = conn_cmpl->cause;

            if (link->sig_chann.state == AVDTP_STATE_CONNECTING)
            {
                avdtp_free_link(link);
                avdtp->cback(conn_cmpl->cid, AVDTP_MSG_SIG_CONN_FAIL, &info);
            }
            else if (link->strm_chann.state == AVDTP_STATE_CONNECTING)
            {
                link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;
                link->strm_chann.state    = AVDTP_STATE_DISCONNECTED;
                avdtp->cback(conn_cmpl->cid, AVDTP_MSG_STREAM_CONN_FAIL, &info);
            }
        }
    }
    else
    {
        mpa_send_l2c_disconn_req(conn_cmpl->cid);
    }
}

void avdtp_handle_disconn_ind(T_MPA_L2C_DISCONN_IND *disconn_ind)
{
    T_AVDTP_LINK *link;

    PROTOCOL_PRINT_TRACE1("avdtp_handle_disconn_ind: cid 0x%04x", disconn_ind->cid);

    link = avdtp_find_link_by_cid(disconn_ind->cid);
    if (link != NULL)
    {
        if (link->sig_chann.cid == disconn_ind->cid)
        {
            if (link->strm_chann.state == AVDTP_STATE_CONNECTED)
            {
                mpa_send_l2c_disconn_req(link->strm_chann.cid);
                avdtp->cback(link->strm_chann.cid, AVDTP_MSG_STREAM_DISCONNECTED, &disconn_ind->cause);
            }

            avdtp_free_link(link);
            avdtp->cback(disconn_ind->cid, AVDTP_MSG_SIG_DISCONNECTED, &disconn_ind->cause);
        }
        else
        {
            link->strm_chann.state = AVDTP_STATE_DISCONNECTED;
            link->strm_chann.cid   = 0;
            avdtp->cback(disconn_ind->cid, AVDTP_MSG_STREAM_DISCONNECTED, &disconn_ind->cause);
        }
    }

    mpa_send_l2c_disconn_cfm(disconn_ind->cid);
}

void avdtp_handle_disconn_rsp(T_MPA_L2C_DISCONN_RSP *disconn_rsp)
{
    T_AVDTP_LINK *link;

    PROTOCOL_PRINT_TRACE1("avdtp_handle_disconn_rsp: cid 0x%04x", disconn_rsp->cid);

    link = avdtp_find_link_by_cid(disconn_rsp->cid);
    if (link != NULL)
    {
        if (link->sig_chann.cid == disconn_rsp->cid)
        {
            avdtp_free_link(link);
            avdtp->cback(disconn_rsp->cid, AVDTP_MSG_SIG_DISCONNECTED, &disconn_rsp->cause);
        }
        else
        {
            link->strm_chann.state = AVDTP_STATE_DISCONNECTED;
            link->strm_chann.cid   = 0;
            avdtp->cback(disconn_rsp->cid, AVDTP_MSG_STREAM_DISCONNECTED, &disconn_rsp->cause);
        }
    }
}

void avdtp_l2c_cback(void        *buf,
                     T_PROTO_MSG  msg)
{
    switch (msg)
    {
    case L2C_CONN_IND:
        avdtp_handle_conn_req((T_MPA_L2C_CONN_IND *)buf);
        break;

    case L2C_CONN_RSP:
        avdtp_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)buf);
        break;

    case L2C_CONN_CMPL:
        avdtp_handle_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)buf);
        break;

    case L2C_DATA_IND:
        avdtp_handle_data_ind((T_MPA_L2C_DATA_IND *)buf);
        break;

    case L2C_DATA_RSP:
        avdtp_handle_data_rsp((T_MPA_L2C_DATA_RSP *)buf);
        break;

    case L2C_DISCONN_IND:
        avdtp_handle_disconn_ind((T_MPA_L2C_DISCONN_IND *)buf);
        break;

    case L2C_DISCONN_RSP:
        avdtp_handle_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)buf);
        break;

    default:
        break;
    }
}

bool avdtp_codec_add(uint8_t             role,
                     T_AVDTP_CODEC_TYPE  codec_type,
                     uint8_t            *media_codec_info)
{
    T_AVDTP_LOCAL_SEP *sep;
    T_AVDTP_LINK      *link;

    link = os_queue_peek(&avdtp->link_list, 0);
    while (link != NULL)
    {
        if (link->sig_chann.sig_state != AVDTP_SIG_STATE_IDLE)
        {
            return false;
        }

        link = link->next;
    }

    sep = calloc(1, sizeof(T_AVDTP_LOCAL_SEP));
    if (sep == NULL)
    {
        return false;
    }

    switch (codec_type)
    {
    case AVDTP_CODEC_TYPE_SBC:
        {
            sep->codec_type     = SBC_MEDIA_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_SBC;
        }
        break;

    case AVDTP_CODEC_TYPE_AAC:
        {
            sep->codec_type     = AAC_MEDIA_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_AAC;
        }
        break;

    case AVDTP_CODEC_TYPE_USAC:
        {
            sep->codec_type     = USAC_MEDIA_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_USAC;
        }
        break;

    case AVDTP_CODEC_TYPE_LDAC:
        {
            sep->codec_type     = VENDOR_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_LDAC;
        }
        break;

    case AVDTP_CODEC_TYPE_LC3:
        {
            sep->codec_type     = VENDOR_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_LC3;
        }
        break;

    case AVDTP_CODEC_TYPE_LHDC:
        {
            sep->codec_type     = VENDOR_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_LHDC;
        }
        break;

    default:
        {
            free(sep);
        }
        return false;
    }

    sep->tsep       = role;
    sep->media_type = AVDTP_MEDIA_AUDIO;
    sep->in_use     = false;
    memcpy(sep->media_codec_info, media_codec_info, MAX_CODEC_INFO_SIZE);
    avdtp->local_sep_num++;
    sep->sep_id     = avdtp->local_sep_num;
    os_queue_in(&avdtp->local_sep_list, sep);

    return true;
}

bool avdtp_codec_delete(uint8_t            role,
                        T_AVDTP_CODEC_TYPE codec_type)
{
    T_AVDTP_LINK      *link;
    T_AVDTP_LOCAL_SEP *sep;
    uint8_t            i = 0;
    bool               ret = false;

    link = os_queue_peek(&avdtp->link_list, 0);
    while (link != NULL)
    {
        if (link->sig_chann.sig_state != AVDTP_SIG_STATE_IDLE)
        {
            return false;
        }

        link = link->next;
    }

    sep = os_queue_peek(&avdtp->local_sep_list, i);
    while (sep != NULL)
    {
        if (sep->tsep == role && sep->sub_codec_type == codec_type)
        {
            os_queue_delete(&avdtp->local_sep_list, sep);
            free(sep);
            avdtp->local_sep_num--;
            ret = true;
        }
        else
        {
            sep->sep_id = ++i;
        }

        sep = os_queue_peek(&avdtp->local_sep_list, i);
    }

    return ret;
}

bool avdtp_init(uint8_t       service_capabilities,
                P_AVDTP_CBACK cback)
{
    int32_t ret = 0;

    avdtp = calloc(1, sizeof(T_AVDTP));
    if (avdtp == NULL)
    {
        ret = 1;
        goto fail_alloc_avdtp;
    }

    if (mpa_reg_l2c_proto(PSM_AVDTP, avdtp_l2c_cback, &avdtp->queue_id) == false)
    {
        ret = 2;
        goto fail_reg_l2c;
    }

    avdtp->sig_timer        = 5000;
    avdtp->cback            = cback;
    avdtp->service_category = service_capabilities;
    avdtp->local_sep_num    = 0;

    return true;

fail_reg_l2c:
    free(avdtp);
    avdtp = NULL;
fail_alloc_avdtp:
    PROTOCOL_PRINT_ERROR1("avdtp_init: failed %d", -ret);
    return false;
}

bool avdtp_roleswap_info_get(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVDTP_INFO *info)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        info->int_flag             = link->sig_chann.int_flag;
        info->int_seid             = link->sig_chann.int_seid;
        info->acp_seid_idx         = link->sig_chann.acp_seid_idx;
        info->acp_seid             = link->sig_chann.acp_seid;
        info->tx_trans_label       = link->sig_chann.tx_trans_label;
        info->rx_start_trans_label = link->sig_chann.rx_start_trans_label;
        info->sig_state            = (uint8_t)link->sig_chann.sig_state;
        info->cmd_flag             = link->sig_chann.cmd_flag;
        info->delay_report         = link->sig_chann.delay_report_flag;
        info->data_offset          = avdtp->data_offset;

        return true;
    }

    return false;
}

bool avdtp_roleswap_info_set(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVDTP_INFO *info)
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = avdtp_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        link->sig_chann.acp_seid_idx         = info->acp_seid_idx;
        link->sig_chann.acp_seid             = info->acp_seid;
        link->sig_chann.cid                  = info->sig_cid;
        link->sig_chann.cmd_flag             = info->cmd_flag;
        link->sig_chann.int_flag             = info->int_flag;
        link->sig_chann.int_seid             = info->int_seid;
        link->sig_chann.sig_state            = (T_AVDTP_SIG_STATE)info->sig_state;
        link->sig_chann.state                = AVDTP_STATE_CONNECTED;
        link->sig_chann.tx_trans_label       = info->tx_trans_label;
        link->sig_chann.rx_start_trans_label = info->rx_start_trans_label;
        link->sig_chann.delay_report_flag    = info->delay_report;

        link->strm_chann.cid                 = info->stream_cid;
        link->strm_chann.state               = AVDTP_STATE_CONNECTED;

        avdtp->data_offset                   = info->data_offset;

        return true;
    }

    return false;
}

bool avdtp_roleswap_info_del(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *link;

    link = avdtp_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        avdtp_free_link(link);
        return true;
    }

    return false;
}

#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "avdtp.h"

bool avdtp_init(uint8_t       service_capabilities,
                P_AVDTP_CBACK cback)
{
    return false;
}

bool avdtp_codec_add(uint8_t             role,
                     T_AVDTP_CODEC_TYPE  codec_type,
                     uint8_t            *media_codec_info)
{
    return false;
}

bool avdtp_codec_delete(uint8_t            role,
                        T_AVDTP_CODEC_TYPE codec_type)
{
    return false;
}

bool avdtp_stream_data_send(uint8_t   bd_addr[6],
                            uint16_t  seq_num,
                            uint32_t  time_stamp,
                            uint8_t   frame_num,
                            uint8_t  *p_data,
                            uint16_t  len)
{
    return false;
}

void avdtp_signal_connect_req(uint8_t  bd_addr[6],
                              uint16_t avdtp_ver,
                              uint8_t  role,
                              uint16_t mtu_size)
{

}

void avdtp_stream_connect_req(uint8_t bd_addr[6])
{

}

bool avdtp_signal_connect_cfm(uint8_t  bd_addr[6],
                              uint16_t mtu_size,
                              bool     accept)
{
    return false;
}

bool avdtp_signal_discover_req(uint8_t bd_addr[6])
{
    return false;
}

bool avdtp_signal_get_capability_req(uint8_t bd_addr[6])
{
    return false;
}

bool avdtp_signal_cfg_req(uint8_t bd_addr[6])
{
    return false;
}

bool avdtp_signal_open_req(uint8_t bd_addr[6],
                           uint8_t role)
{
    return false;
}

bool avdtp_signal_recfg_req(uint8_t bd_addr[6],
                            uint8_t codec_type,
                            uint8_t role)
{
    return false;
}

bool avdtp_signal_delay_report_req(uint8_t  bd_addr[6],
                                   uint16_t delay)
{
    return false;
}

bool avdtp_signal_start_req(uint8_t bd_addr[6])
{
    return false;
}

bool avdtp_signal_suspend_req(uint8_t bd_addr[6])
{
    return false;
}

bool avdtp_signal_close_req(uint8_t bd_addr[6])
{
    return false;
}

bool avdtp_signal_abort_req(uint8_t bd_addr[6])
{
    return false;
}

bool avdtp_signal_start_cfm(uint8_t bd_addr[6],
                            bool    accept)
{
    return false;
}

bool avdtp_signal_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

void avdtp_stream_disconnect_req(uint8_t bd_addr[6])
{

}

bool avdtp_roleswap_info_get(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVDTP_INFO *info)
{
    return false;
}

bool avdtp_roleswap_info_set(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVDTP_INFO *info)
{
    return false;
}

bool avdtp_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}
#endif
