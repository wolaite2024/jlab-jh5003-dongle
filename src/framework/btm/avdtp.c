/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if (CONFIG_REALTEK_BTM_A2DP_SUPPORT == 1)
#include <string.h>
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "avdtp.h"
#include "sys_timer.h"

#define AVDTP_SIG_MTU           675
#define AVDTP_STREAM_MTU        675
#define AVDTP_LDAC_STREAM_MTU   679

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
    uint8_t packet_num;
    uint8_t signal_id;
} T_AVDTP_START_PKT_HDR;

typedef struct t_avdtp_contend_pkt_hdr
{
    uint8_t trans_info; /* include Transaction label , packet type, Message type */
} T_AVDTP_CONTINUE_PKT_HDR;

typedef struct t_avdtp_start_pkt
{
    T_AVDTP_START_PKT_HDR packet_header;
    uint8_t packet_data[1];
} T_AVDTP_START_PKT;

typedef struct t_avdtp_continue_pkt
{
    T_AVDTP_CONTINUE_PKT_HDR packet_header;
    uint8_t packet_data[1];
} T_AVDTP_CONTINUE_PKT;

typedef struct t_avdtp_sig_hdr
{
    uint8_t trans_label: 4;
    uint8_t packet_type: 2;
    uint8_t block_Length: 2;
    uint8_t signal_id;
} T_AVDTP_SIG_HDR;

typedef enum t_avdtp_state
{
    AVDTP_STATE_DISCONNECTED        = 0x00,
    AVDTP_STATE_ALLOCATED           = 0x01,
    AVDTP_STATE_CONNECTING          = 0x02,
    AVDTP_STATE_CONNECTED           = 0x03,
    AVDTP_STATE_DISCONNECTING       = 0x04,
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
    uint8_t            *p_fragment_data;
    uint8_t            *p_cfg_cpbs;
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
    uint16_t            cid;
    T_AVDTP_STATE       state;
} T_STRM_CHANN;

typedef struct t_avdtp_link
{
    T_SYS_TIMER_HANDLE  timer_handle;
    T_SIG_CHANN         sig_chann;
    T_STRM_CHANN        strm_chann;
    uint8_t             bd_addr[6];
    uint16_t            avdtp_ver;
    uint8_t             conn_role;
    uint8_t             role;
} T_AVDTP_LINK;

typedef struct t_avdtp
{
    T_AVDTP_LINK       *link;
    P_AVDTP_CBACK       cback;
    T_OS_QUEUE          local_sep_list;
    uint8_t             local_sep_num;
    uint8_t             queue_id;
    uint16_t            data_offset;
    uint16_t            stream_latency;
    uint16_t            sig_timer;
    uint8_t             link_num;
    uint8_t             service_category;
} T_AVDTP;

T_AVDTP *p_avdtp;

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
    T_AVDTP_LINK  *p_link;
    p_link = (void *)sys_timer_id_get(handle);
    if (p_link != NULL)
    {
        PROTOCOL_PRINT_WARN2("avdtp_tout_cback: bd_addr %s, cmd %x",
                             TRACE_BDADDR(p_link->bd_addr), p_link->sig_chann.cmd_flag);

        switch (p_link->sig_chann.cmd_flag)
        {
        case AVDTP_DISCOVER:
        case AVDTP_GET_CPBS:
        case AVDTP_GET_ALL_CPBS:
        case AVDTP_SET_CFG:
            if (p_link->sig_chann.state == AVDTP_STATE_CONNECTED)
            {
                avdtp_signal_disconnect_req(p_link->bd_addr);
            }
            break;

        case AVDTP_OPEN:
        case AVDTP_START:
        case AVDTP_CLOSE:
            avdtp_signal_abort_req(p_link->bd_addr);
            break;

        case AVDTP_ABORT:
            if (p_link->strm_chann.state == AVDTP_STATE_CONNECTED)
            {
                avdtp_stream_disconnect_req(p_link->bd_addr);
            }

            avdtp_signal_disconnect_req(p_link->bd_addr);
            break;

        default:
            break;
        }
    }
}

T_AVDTP_LINK *avdtp_alloc_link(uint8_t bd_addr[6])
{
    uint8_t       i;
    T_AVDTP_LINK *p_link = NULL;

    for (i = 0; i < p_avdtp->link_num; i++)
    {
        if (p_avdtp->link[i].sig_chann.state == AVDTP_STATE_DISCONNECTED)
        {
            p_link = &p_avdtp->link[i];

            p_link->timer_handle = sys_timer_create("avdtp_sig_timer",
                                                    SYS_TIMER_TYPE_LOW_PRECISION,
                                                    (uint32_t)p_link,
                                                    p_avdtp->sig_timer * 1000,
                                                    false,
                                                    avdtp_tout_cback);
            if (p_link->timer_handle != NULL)
            {
                memcpy(p_link->bd_addr, bd_addr, 6);
                p_link->sig_chann.state = AVDTP_STATE_ALLOCATED;
                break;
            }
            else
            {
                return NULL;
            }
        }
    }

    PROTOCOL_PRINT_TRACE2("avdtp_alloc_link: bd_addr %s, link %p",
                          TRACE_BDADDR(bd_addr), p_link);

    return p_link;
}

void avdtp_free_link(T_AVDTP_LINK *p_link)
{
    PROTOCOL_PRINT_TRACE1("avdtp_free_link: link %p", p_link);

    if (p_link->sig_chann.p_cfg_cpbs != NULL)
    {
        free(p_link->sig_chann.p_cfg_cpbs);
    }

    if (p_link->sig_chann.avdtp_sep != NULL)
    {
        free(p_link->sig_chann.avdtp_sep);
    }

    if (p_link->sig_chann.p_fragment_data != NULL)
    {
        free(p_link->sig_chann.p_fragment_data);
    }

    if (p_link->timer_handle != NULL)
    {
        sys_timer_delete(p_link->timer_handle);
    }

    memset(p_link, 0, sizeof(T_AVDTP_LINK));
}

T_AVDTP_LINK *avdtp_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t       i;
    T_AVDTP_LINK *p_link = NULL;

    if (bd_addr == NULL)
    {
        return p_link;
    }

    for (i = 0; i < p_avdtp->link_num; i++)
    {
        if (!memcmp(p_avdtp->link[i].bd_addr, bd_addr, 6))
        {
            p_link = &p_avdtp->link[i];
            break;
        }
    }

    return p_link;
}

T_AVDTP_LINK *avdtp_find_link_by_cid(uint16_t cid)
{
    uint8_t       i;
    T_AVDTP_LINK *p_link = NULL;

    for (i = 0; i < p_avdtp->link_num; i++)
    {
        if ((p_avdtp->link[i].sig_chann.cid == cid) ||
            (p_avdtp->link[i].strm_chann.cid == cid))
        {
            p_link = &p_avdtp->link[i];
            break;
        }
    }

    return p_link;
}

bool avdtp_vendor_codec_ldac_handler(uint8_t            type,
                                     uint8_t           *ret_ptr,
                                     uint8_t           *remote_codec_info,
                                     T_AVDTP_LOCAL_SEP *local_sep,
                                     uint8_t            role)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
        while (sep != NULL)
        {
            if (sep->sub_codec_type == CODEC_TYPE_LDAC)
            {
                if ((!memcmp(remote_codec_info, &sep->media_codec_info[0], 6)) &&
                    (sep->tsep != role))
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
        ret_ptr[0] = MEDIA_CODEC_LDAC_LOSC;
        ret_ptr[1] = (AUDIO_MEDIA_TYPE << 4);
        ret_ptr[2] = VENDOR_CODEC_TYPE;
        memcpy(&ret_ptr[3], &local_sep->media_codec_info[0], 6);

        //set sample frequency
        if (remote_codec_info[6] & 0x01 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x01;//192K
        }
        else if (remote_codec_info[6] & 0x02 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x02;//176.4K
        }
        else if (remote_codec_info[6] & 0x04 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x04;//96K
        }
        else if (remote_codec_info[6] & 0x08 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x08;//88.2K
        }
        else if (remote_codec_info[6] & 0x10 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x10;//48K
        }
        else if (remote_codec_info[6] & 0x20 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x20;//44.1K
        }

        //set channel mode
        if (remote_codec_info[7] & 0x01 & local_sep->media_codec_info[7])
        {
            ret_ptr[10] = 0x01;//stereo
        }
        else if (remote_codec_info[7] & 0x02 & local_sep->media_codec_info[7])
        {
            ret_ptr[10] = 0x02;//dual channel
        }
        else if (remote_codec_info[7] & 0x04 & local_sep->media_codec_info[7])
        {
            ret_ptr[10] = 0x04;//mono
        }

        status = true;
    }

    return status;
}

bool avdtp_vendor_codec_lc3_handler(uint8_t            type,
                                    uint8_t           *ret_ptr,
                                    uint8_t           *remote_codec_info,
                                    T_AVDTP_LOCAL_SEP *local_sep,
                                    uint8_t            role)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
        while (sep != NULL)
        {
            if (sep->sub_codec_type == CODEC_TYPE_LC3)
            {
                if ((!memcmp(remote_codec_info, &sep->media_codec_info[0], 6)) &&
                    (sep->tsep != role))
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
        uint16_t local_frame_length;
        uint16_t remote_frame_length;

        ret_ptr[0] = MEDIA_CODEC_LC3_LOSC;
        ret_ptr[1] = (AUDIO_MEDIA_TYPE << 4);
        ret_ptr[2] = VENDOR_CODEC_TYPE;
        memcpy(&ret_ptr[3], &local_sep->media_codec_info[0], 6);

        //set sample frequency
        if (remote_codec_info[6] & 0x04 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x04;//48K
        }
        else if (remote_codec_info[6] & 0x08 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x08;//44.1K
        }
        else if (remote_codec_info[6] & 0x10 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x10;//32K
        }
        else if (remote_codec_info[6] & 0x20 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x20;//24K
        }
        else if (remote_codec_info[6] & 0x40 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x40;//16K
        }
        else if (remote_codec_info[6] & 0x80 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x80;//8K
        }


        //set channel number
        if (remote_codec_info[6] & 0x01 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] |= 0x01; //2 channel
        }
        else if (remote_codec_info[6] & 0x02 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] |= 0x02; //1 channel
        }

        //set frame duration
        if (remote_codec_info[7] & 0x02 & local_sep->media_codec_info[7])
        {
            ret_ptr[10] = 0x02; //10ms
        }
        else if (remote_codec_info[7] & 0x04 & local_sep->media_codec_info[7])
        {
            ret_ptr[10] = 0x04; //7.5ms
        }

        //set frame length
        local_frame_length = ((local_sep->media_codec_info[7] & 0x01) << 8) +
                             local_sep->media_codec_info[8];
        remote_frame_length = ((remote_codec_info[7] & 0x01) << 8) + remote_codec_info[8];
        if (local_frame_length > remote_frame_length)
        {
            ret_ptr[10] |= (remote_codec_info[7] & 0x01);
            ret_ptr[11] = remote_codec_info[8];
        }
        else
        {
            ret_ptr[10] |= (local_sep->media_codec_info[7] & 0x01);
            ret_ptr[11] = local_sep->media_codec_info[8];
        }

        status = true;
    }

    return status;
}

bool avdtp_vendor_codec_lhdc_handler(uint8_t            type,
                                     uint8_t            *ret_ptr,
                                     uint8_t            *remote_codec_info,
                                     T_AVDTP_LOCAL_SEP  *local_sep,
                                     uint8_t             role)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
        while (sep != NULL)
        {
            if (sep->sub_codec_type == CODEC_TYPE_LHDC)
            {
                if ((!memcmp(remote_codec_info, &sep->media_codec_info[0], 6)) &&
                    (sep->tsep != role))
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
        ret_ptr[0] = MEDIA_CODEC_LHDC_LOSC;
        ret_ptr[1] = (AUDIO_MEDIA_TYPE << 4);
        ret_ptr[2] = VENDOR_CODEC_TYPE;
        memcpy(&ret_ptr[3], &local_sep->media_codec_info[0], 6);

        /* set sample frequency */
        if (remote_codec_info[6] & 0x01 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x01; /* 192K */
        }
        else if (remote_codec_info[6] & 0x04 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x04; /* 96K */
        }
        else if (remote_codec_info[6] & 0x10 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x10; /* 48K */
        }
        else if (remote_codec_info[6] & 0x20 & local_sep->media_codec_info[6])
        {
            ret_ptr[9] = 0x20; /* 44.1K */
        }

        /* set min bitrate */
        if ((remote_codec_info[7] & 0xc0) < (local_sep->media_codec_info[7] & 0xc0))
        {
            ret_ptr[10] = local_sep->media_codec_info[7] & 0xc0;
        }
        else
        {
            ret_ptr[10] = remote_codec_info[7] & 0xc0;
        }

        /* set max bitrate */
        if ((remote_codec_info[7] & 0x30) == 0)
        {
            ret_ptr[10] |= local_sep->media_codec_info[7] & 0x30;
        }
        else if ((local_sep->media_codec_info[7] & 0x30) == 0)
        {
            ret_ptr[10] |= remote_codec_info[7] & 0x30;
        }
        else if ((remote_codec_info[7] & 0x30) > (local_sep->media_codec_info[7] & 0x30))
        {
            ret_ptr[10] |= local_sep->media_codec_info[7] & 0x30;
        }
        else
        {
            ret_ptr[10] |= remote_codec_info[7] & 0x30;
        }

        /* set bit depth */
        if (remote_codec_info[7] & 0x01 & local_sep->media_codec_info[7])
        {
            ret_ptr[10] |= 0x01; /* 32bit */
        }
        else if (remote_codec_info[7] & 0x02 & local_sep->media_codec_info[7])
        {
            ret_ptr[10] |= 0x02; /* 24bit */
        }
        else if (remote_codec_info[7] & 0x04 & local_sep->media_codec_info[7])
        {
            ret_ptr[10] |= 0x04; /* 16bit */
        }

        /* set version number */
        if (remote_codec_info[8] & 0x01 & local_sep->media_codec_info[8])
        {
            ret_ptr[11] |= 0x01;  /* v5.0 */
        }
        else if (remote_codec_info[8] & 0x02 & local_sep->media_codec_info[8])
        {
            ret_ptr[11] |= 0x02;  /* v5.1 */
        }
        else if (remote_codec_info[8] & 0x04 & local_sep->media_codec_info[8])
        {
            ret_ptr[11] |= 0x04;  /* v5.2 */
        }
        else if (remote_codec_info[8] & 0x08 & local_sep->media_codec_info[8])
        {
            ret_ptr[11] |= 0x08;  /* v5.3 */
        }

        /* set ll */
        if (remote_codec_info[9] & 0x40 & local_sep->media_codec_info[9])
        {
            ret_ptr[12] |= 0x40;
        }

        /* set meta */
        if (remote_codec_info[9] & 0x04 & local_sep->media_codec_info[9])
        {
            ret_ptr[12] |= 0x04;
        }

        /* set jas */
        if (remote_codec_info[9] & 0x02 & local_sep->media_codec_info[9])
        {
            ret_ptr[12] |= 0x02;
        }

        /* set ar */
        if (remote_codec_info[9] & 0x01 & local_sep->media_codec_info[9])
        {
            ret_ptr[12] |= 0x01;
        }

        status = true;
    }

    return status;
}

bool avdtp_codec_usac_handler(uint8_t            type,
                              uint8_t           *ret_ptr,
                              uint8_t           *remote_codec_info,
                              T_AVDTP_LOCAL_SEP *local_sep,
                              uint8_t            role)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
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

        ret_ptr[0] = MEDIA_CODEC_USAC_LOSC;
        ret_ptr[1] = (AUDIO_MEDIA_TYPE << 4);
        ret_ptr[2] = USAC_MEDIA_CODEC_TYPE;

        //object type
        ret_ptr[3] = 0x80;

        //sampling frequency
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
            sampling_frequency = (1 << (4 + m));
            if ((remote_sampling_frequency & sampling_frequency)
                && (local_sampling_frequency & sampling_frequency))
            {
                break;
            }
        }
        if (m == 26)
        {
            sampling_frequency = (1 << 10); //48K
        }
        ret_ptr[3] |= (uint8_t)(sampling_frequency >> 24);
        ret_ptr[4] = (uint8_t)(sampling_frequency >> 16);
        ret_ptr[5] = (uint8_t)(sampling_frequency >> 8);
        ret_ptr[6] = (uint8_t)sampling_frequency;

        //channel setting
        if ((remote_codec_info[3] & 0x04) && (local_sep->media_codec_info[3] & 0x04)) // 2 channel
        {
            ret_ptr[6] |= 0x04; //stereo
        }
        else if ((remote_codec_info[3] & 0x08) && (local_sep->media_codec_info[3] & 0x08)) // 1 channel
        {
            ret_ptr[6] |= 0x08; //mono
        }
        else
        {
            ret_ptr[6] |= 0x08; //mono
        }

        //VBR
        if ((remote_codec_info[4] & 0x80) && (local_sep->media_codec_info[4] & 0x80))
        {
            ret_ptr[7] = 0x80;
        }
        else
        {
            ret_ptr[7] = 0x00;
        }

        //Bit rate
        local_bit_rate = ((local_sep->media_codec_info[4] & 0x7F) << 16) +
                         (local_sep->media_codec_info[5] << 8) +
                         local_sep->media_codec_info[6];
        remote_bit_rate = ((remote_codec_info[4] & 0x7F) << 16) +
                          (remote_codec_info[5] << 8) +
                          remote_codec_info[6];
        if (local_bit_rate > remote_bit_rate)
        {
            ret_ptr[7] |= (remote_codec_info[4] & 0x7F);
            ret_ptr[8] = remote_codec_info[5];
            ret_ptr[9] = remote_codec_info[6];
        }
        else
        {
            ret_ptr[7] |= (local_sep->media_codec_info[4] & 0x7F);
            ret_ptr[8] = local_sep->media_codec_info[5];
            ret_ptr[9] = local_sep->media_codec_info[6];
        }

        status = true;
    }

    return status;
}

bool avdtp_codec_aac_handler(uint8_t            type,
                             uint8_t           *ret_ptr,
                             uint8_t           *remote_codec_info,
                             T_AVDTP_LOCAL_SEP *local_sep,
                             uint8_t            role)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
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

        ret_ptr[0] = MEDIA_CODEC_AAC_LOSC;
        ret_ptr[1] = (AUDIO_MEDIA_TYPE << 4);
        ret_ptr[2] = AAC_MEDIA_CODEC_TYPE;

        //object type
        if ((remote_codec_info[0] & 0x80) && (local_sep->media_codec_info[0] & 0x80))
        {
            ret_ptr[3] = 0x80;//MPEG-2 AAC LC
        }
        else if ((remote_codec_info[0] & 0x40) && (local_sep->media_codec_info[0] & 0x40))
        {
            ret_ptr[3] = 0x40;//MPEG-4 AAC LC
        }
        else if ((remote_codec_info[0] & 0x20) && (local_sep->media_codec_info[0] & 0x20))
        {
            ret_ptr[3] = 0x20;//MPEG-4 AAC LTP
        }
        else if ((remote_codec_info[0] & 0x10) && (local_sep->media_codec_info[0] & 0x10))
        {
            ret_ptr[3] = 0x10;//MPEG-4 AAC scalable
        }
        else if ((remote_codec_info[0] & 0x08) && (local_sep->media_codec_info[0] & 0x08))
        {
            ret_ptr[3] = 0x08;//MPEG-4 HE-AAC
        }
        else if ((remote_codec_info[0] & 0x04) && (local_sep->media_codec_info[0] & 0x04))
        {
            ret_ptr[3] = 0x04;//MPEG-4 HE-AACv2
        }
        else if ((remote_codec_info[0] & 0x02) && (local_sep->media_codec_info[0] & 0x02))
        {
            ret_ptr[3] = 0x02;//MPEG-4 AAC-ELDv2
        }
        else if ((remote_codec_info[0] & 0x01) && (local_sep->media_codec_info[0] & 0x01))
        {
            ret_ptr[3] = 0x01;//MPEG-D DRC
        }
        else
        {
            ret_ptr[3] = 0x80; //MPEG-2 AAC LC
        }

        //channel setting
        if ((remote_codec_info[2] & 0x04) && (local_sep->media_codec_info[2] & 0x04))
        {
            ret_ptr[5] = 0x04; //stereo
        }
        else if ((remote_codec_info[2] & 0x08) && (local_sep->media_codec_info[2] & 0x08))
        {
            ret_ptr[5] = 0x08; //mono
        }
        else
        {
            ret_ptr[5] = 0x08; //mono
        }

        //sampling frequency
        if ((remote_codec_info[1] & 0x01) && (local_sep->media_codec_info[1] & 0x01))
        {
            ret_ptr[4] = 0x01; //44.1K
        }
        else if ((remote_codec_info[1] & 0x02) && (local_sep->media_codec_info[1] & 0x02))
        {
            ret_ptr[4] = 0x02; //32K
        }
        else if ((remote_codec_info[1] & 0x04) && (local_sep->media_codec_info[1] & 0x04))
        {
            ret_ptr[4] = 0x04; //24K
        }
        else if ((remote_codec_info[1] & 0x08) && (local_sep->media_codec_info[1] & 0x08))
        {
            ret_ptr[4] = 0x08; //22.05K
        }
        else if ((remote_codec_info[1] & 0x10) && (local_sep->media_codec_info[1] & 0x10))
        {
            ret_ptr[4] = 0x10; //16K
        }
        else if ((remote_codec_info[1] & 0x20) && (local_sep->media_codec_info[1] & 0x20))
        {
            ret_ptr[4] = 0x20; //12K
        }
        else if ((remote_codec_info[1] & 0x40) && (local_sep->media_codec_info[1] & 0x40))
        {
            ret_ptr[4] = 0x40; //11.025K
        }
        else if ((remote_codec_info[1] & 0x80) && (local_sep->media_codec_info[1] & 0x80))
        {
            ret_ptr[4] = 0x80; //8K
        }
        else if ((remote_codec_info[2] & 0x80) && (local_sep->media_codec_info[2] & 0x80))
        {
            ret_ptr[4] = 0x00;
            ret_ptr[5] |= 0x80; //48K
        }
        else if ((remote_codec_info[2] & 0x40) && (local_sep->media_codec_info[2] & 0x40))
        {
            ret_ptr[4] = 0x00;
            ret_ptr[5] |= 0x40; //64K
        }
        else if ((remote_codec_info[2] & 0x20) && (local_sep->media_codec_info[2] & 0x20))
        {
            ret_ptr[4] = 0x00;
            ret_ptr[5] |= 0x20; //88.2K
        }
        else if ((remote_codec_info[2] & 0x10) && (local_sep->media_codec_info[2] & 0x10))
        {
            ret_ptr[4] = 0x00;
            ret_ptr[5] |= 0x10; //96K
        }
        else
        {
            ret_ptr[4] = 0x00;
            ret_ptr[5] |= 0x80; //48k
        }

        //VBR
        if ((remote_codec_info[3] & 0x80) && (local_sep->media_codec_info[3] & 0x80))
        {
            ret_ptr[6] = 0x80;
        }
        else
        {
            ret_ptr[6] = 0x00;
        }

        //Bit rate
        local_bit_rate = ((local_sep->media_codec_info[3] & 0x7F) << 16) +
                         (local_sep->media_codec_info[4] << 8) +
                         local_sep->media_codec_info[5];
        remote_bit_rate = ((remote_codec_info[3] & 0x7F) << 16) +
                          (remote_codec_info[4] << 8) +
                          remote_codec_info[5];
        if (local_bit_rate > remote_bit_rate)
        {
            ret_ptr[6] |= (remote_codec_info[3] & 0x7F);
            ret_ptr[7] = remote_codec_info[4];
            ret_ptr[8] = remote_codec_info[5];
        }
        else
        {
            ret_ptr[6] |= (local_sep->media_codec_info[3] & 0x7F);
            ret_ptr[7] = local_sep->media_codec_info[4];
            ret_ptr[8] = local_sep->media_codec_info[5];
        }

        status = true;
    }

    return status;
}

bool avdtp_codec_sbc_handler(uint8_t            type,
                             uint8_t           *ret_ptr,
                             uint8_t           *remote_codec_info,
                             T_AVDTP_LOCAL_SEP *local_sep,
                             uint8_t            role)
{
    bool status = false;

    if (type == HDL_TYPE_CHECK_CODEC)
    {
        T_AVDTP_LOCAL_SEP *sep;

        sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
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
        ret_ptr[0] = MEDIA_CODEC_SBC_LOSC;
        ret_ptr[1] = (AUDIO_MEDIA_TYPE << 4);
        ret_ptr[2] = SBC_MEDIA_CODEC_TYPE;

        //sample frequency
        if ((remote_codec_info[0] & 0x10) && (local_sep->media_codec_info[0] & 0x10)) //48KHz
        {
            ret_ptr[3] = 0x10;
        }
        else if ((remote_codec_info[0] & 0x20) && (local_sep->media_codec_info[0] & 0x20)) // 44.1KHz
        {
            ret_ptr[3] = 0x20;
        }
        else if ((remote_codec_info[0] & 0x40) && (local_sep->media_codec_info[0] & 0x40)) //32KHz
        {
            ret_ptr[3] = 0x40;
        }
        else if ((remote_codec_info[0] & 0x80) && (local_sep->media_codec_info[0] & 0x80)) //16KHz
        {
            ret_ptr[3] = 0x80;
        }
        else
        {
            ret_ptr[3] = 0x10;
        }

        //channel mode
        if ((remote_codec_info[0] & 0x01) && (local_sep->media_codec_info[0] & 0x01)) //joint stereo
        {
            ret_ptr[3] |= 0x01;
        }
        else if ((remote_codec_info[0] & 0x02) && (local_sep->media_codec_info[0] & 0x02)) //stereo
        {
            ret_ptr[3] |= 0x02;
        }
        else if ((remote_codec_info[0] & 0x04) && (local_sep->media_codec_info[0] & 0x04)) //dual channel
        {
            ret_ptr[3] |= 0x04;
        }
        else if ((remote_codec_info[0] & 0x08) && (local_sep->media_codec_info[0] & 0x08)) //mono
        {
            ret_ptr[3] |= 0x08;
        }
        else
        {
            ret_ptr[3] |= 0x08;
        }

        //block number
        if ((remote_codec_info[1] & 0x10) && (local_sep->media_codec_info[1] & 0x10))//block_no:16
        {
            ret_ptr[4] = 0x10;
        }
        else if ((remote_codec_info[1] & 0x20) && (local_sep->media_codec_info[1] & 0x20))//block_no:12
        {
            ret_ptr[4] = 0x20;
        }
        else if ((remote_codec_info[1] & 0x40) && (local_sep->media_codec_info[1] & 0x40))//block_no:8
        {
            ret_ptr[4] = 0x40;
        }
        else if ((remote_codec_info[1] & 0x80) && (local_sep->media_codec_info[1] & 0x80))//block_no:4
        {
            ret_ptr[4] = 0x80;
        }
        else
        {
            ret_ptr[4] = 0x10;
        }

        //subbands
        if ((remote_codec_info[1] & 0x04) && (local_sep->media_codec_info[1] & 0x04))//8 subbnads
        {
            ret_ptr[4] |= 0x04;
        }
        else if ((remote_codec_info[1] & 0x08) && (local_sep->media_codec_info[1] & 0x08))// 4 subbnads
        {
            ret_ptr[4] |= 0x08;
        }
        else
        {
            ret_ptr[4] |= 0x04;
        }

        //allocation method
        if ((remote_codec_info[1] & 0x01) && (local_sep->media_codec_info[1] & 0x01))//Loudness
        {
            ret_ptr[4] |= 0x01;
        }
        else if ((remote_codec_info[1] & 0x02) && (local_sep->media_codec_info[1] & 0x02))//SNR
        {
            ret_ptr[4] |= 0x02;
        }
        else
        {
            ret_ptr[4] |= 0x01;
        }

        //min bit_pool
        if (remote_codec_info[2] > local_sep->media_codec_info[2])
        {
            ret_ptr[5] = remote_codec_info[2];
        }
        else
        {
            ret_ptr[5] = local_sep->media_codec_info[2];
        }

        //max bit_pool
        if (remote_codec_info[3] < local_sep->media_codec_info[3])
        {
            ret_ptr[6] = remote_codec_info[3];
        }
        else
        {
            ret_ptr[6] = local_sep->media_codec_info[3];
        }

        status = true;
    }

    return status;
}

bool avdtp_stream_data_send(uint8_t   bd_addr[6],
                            uint16_t  seq_num,
                            uint32_t  time_stamp,
                            uint8_t   frame_num,
                            uint8_t  *p_data,
                            uint16_t  len)
{
    T_AVDTP_LINK *p_link;
    uint8_t      *pkt_ptr;
    uint16_t      pkt_len;
    bool          ret = false;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
        {
            pkt_len = len + 12;  /* avdtp header(12 bytes) */

            if (p_link->sig_chann.enable_cp_flag)
            {
                pkt_len += 1;
            }

            if (p_link->sig_chann.codec_type == SBC_MEDIA_CODEC_TYPE)
            {
                pkt_len += 1;
            }
            else if (p_link->sig_chann.codec_type == VENDOR_CODEC_TYPE)
            {
                if ((p_link->sig_chann.codec_subtype == CODEC_TYPE_LDAC) ||
                    (p_link->sig_chann.codec_subtype == CODEC_TYPE_LC3))
                {
                    pkt_len += 1;
                }
                else if (p_link->sig_chann.codec_subtype == CODEC_TYPE_LHDC)
                {
                    pkt_len += 2;
                }
            }

            pkt_ptr = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->strm_chann.cid, 0, pkt_len,
                                      p_avdtp->data_offset, true);
            if (pkt_ptr != NULL)
            {
                uint8_t idx;

                idx = p_avdtp->data_offset;

                pkt_ptr[idx++] = 0x80; /* RTP=2 */
                pkt_ptr[idx++] = 0x60;
                pkt_ptr[idx++] = (uint8_t)(seq_num >> 8);
                pkt_ptr[idx++] = (uint8_t)seq_num;
                pkt_ptr[idx++] = (uint8_t)(time_stamp >> 24);
                pkt_ptr[idx++] = (uint8_t)(time_stamp >> 16);
                pkt_ptr[idx++] = (uint8_t)(time_stamp >> 8);
                pkt_ptr[idx++] = (uint8_t)time_stamp;
                pkt_ptr[idx++] = 0x00; /* sync. source */
                pkt_ptr[idx++] = 0x00;
                pkt_ptr[idx++] = 0x00;
                pkt_ptr[idx++] = 0x00;

                if (p_link->sig_chann.enable_cp_flag)
                {
                    pkt_ptr[idx++] = 0x00;
                }

                if (p_link->sig_chann.codec_type == SBC_MEDIA_CODEC_TYPE)
                {
                    pkt_ptr[idx++] = frame_num & 0x0f;
                }
                else if (p_link->sig_chann.codec_type == VENDOR_CODEC_TYPE)
                {
                    if ((p_link->sig_chann.codec_subtype == CODEC_TYPE_LDAC) ||
                        (p_link->sig_chann.codec_subtype == CODEC_TYPE_LC3))
                    {
                        pkt_ptr[idx++] = frame_num;
                    }
                    else if (p_link->sig_chann.codec_subtype == CODEC_TYPE_LHDC)
                    {
                        pkt_ptr[idx++] = (frame_num & 0x0f) << 2;
                        pkt_ptr[idx++] = (uint8_t)(seq_num % 0xff);
                    }
                }

                memcpy(&pkt_ptr[idx], p_data, len);
                mpa_send_l2c_data_req(pkt_ptr, p_avdtp->data_offset, p_link->strm_chann.cid,
                                      pkt_len, false);
                ret = true;
            }
        }
    }

    return ret;
}

void avdtp_signal_connect_req(uint8_t  bd_addr[6],
                              uint16_t avdtp_ver,
                              uint8_t  role)
{
    T_AVDTP_LINK *p_link;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = avdtp_alloc_link(bd_addr);
    }

    if (p_link != NULL)
    {
        p_link->sig_chann.state = AVDTP_STATE_CONNECTING;
        p_link->avdtp_ver = avdtp_ver;
        p_link->conn_role = role;
        mpa_send_l2c_conn_req(PSM_AVDTP, UUID_AVDTP, p_avdtp->queue_id, AVDTP_SIG_MTU,
                              bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);
    }
}

void avdtp_stream_connect_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;

    PROTOCOL_PRINT_INFO1("avdtp_stream_connect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->strm_chann.state == AVDTP_STATE_DISCONNECTED)
        {
            p_link->strm_chann.state = AVDTP_STATE_CONNECTING;

            if (p_link->sig_chann.codec_type != VENDOR_CODEC_TYPE)
            {
                mpa_send_l2c_conn_req(PSM_AVDTP, UUID_AVDTP, p_avdtp->queue_id, AVDTP_STREAM_MTU,
                                      bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);
            }
            else
            {
                mpa_send_l2c_conn_req(PSM_AVDTP, UUID_AVDTP, p_avdtp->queue_id, AVDTP_LDAC_STREAM_MTU,
                                      bd_addr, MPA_L2C_MODE_BASIC, 0xFFFF);
            }
        }
    }
}

bool avdtp_signal_disconnect_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;

    PROTOCOL_PRINT_TRACE1("avdtp_signal_disconnect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        mpa_send_l2c_disconn_req(p_link->sig_chann.cid);
        return true;
    }

    return false;
}

void avdtp_stream_disconnect_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;

    PROTOCOL_PRINT_TRACE1("avdtp_stream_disconnect_req: bd_addr %s", TRACE_BDADDR(bd_addr));

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        mpa_send_l2c_disconn_req(p_link->strm_chann.cid);
    }
}

void avdtp_remote_capability_parse(T_AVDTP_LINK *p_link,
                                   uint8_t      *p_capability,
                                   uint16_t      capability_len,
                                   uint8_t       endpoint_index)
{
    uint8_t  temp_len;
    uint8_t  i = 0;

    p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_mask = 0;
    memset(&(p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_order[0]), 0xff, 8);

    while (capability_len != 0)
    {
        temp_len = *(p_capability + 1);

        switch (*p_capability)
        {
        case SRV_CATEG_MEDIA_TRANS:
            {
                p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_order[i] = SRV_CATEG_MEDIA_TRANS;
                i++;
                p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_mask |= CATEG_MIDIA_TRANS_MASK;
            }
            break;

        case SRV_CATEG_CP:
            {
                p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_order[i] = SRV_CATEG_CP;
                i++;
                p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_mask |= CATEG_CP_MASK;
            }
            break;

        case SRV_CATEG_MEDIA_CODEC:
            {
                p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_order[i] = SRV_CATEG_MEDIA_CODEC;
                i++;
                p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_mask |= CATEG_MEDIA_CODEC_MASK;
                p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_type = *(p_capability + 3);
                p_link->sig_chann.avdtp_sep[endpoint_index].media_type = (*(p_capability + 2) & 0xF0) >> 4;
                if (p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_type == SBC_MEDIA_CODEC_TYPE)
                {
                    p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_subtype = CODEC_TYPE_SBC;
                    memcpy(&(p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_info[0]),
                           (p_capability + 4), 4);
                }
                else if (p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_type ==
                         AAC_MEDIA_CODEC_TYPE)
                {
                    p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_subtype = CODEC_TYPE_AAC;
                    memcpy(&(p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_info[0]),
                           (p_capability + 4), 6);
                }
                else if (p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_type ==
                         USAC_MEDIA_CODEC_TYPE)
                {
                    p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_subtype = CODEC_TYPE_USAC;
                    memcpy(&(p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_info[0]),
                           (p_capability + 4), 7);
                }
                else if (p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_type == VENDOR_CODEC_TYPE)
                {
                    if (memcmp((p_capability + 4), avdtp_ldac_codec_info, 6) == 0)
                    {
                        p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_subtype = CODEC_TYPE_LDAC;
                        memcpy(&(p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_info[0]), (p_capability + 4), 8);
                    }
                    else if (memcmp((p_capability + 4), avdtp_lc3_codec_info, 6) == 0)
                    {
                        p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_subtype = CODEC_TYPE_LC3;
                        memcpy(&(p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_info[0]), (p_capability + 4), 9);
                    }
                    else if (memcmp((p_capability + 4), avdtp_lhdc_v5_codec_info, 6) == 0)
                    {
                        p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_subtype = CODEC_TYPE_LHDC;
                        memcpy(&(p_link->sig_chann.avdtp_sep[endpoint_index].media_codec_info[0]), (p_capability + 4), 11);
                    }
                }
            }
            break;

        case SRV_CATEG_DELAY_REPORT:
            {
                p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_order[i] = SRV_CATEG_DELAY_REPORT;
                i++;
                p_link->sig_chann.avdtp_sep[endpoint_index].cpbs_mask |= CATEG_DELAY_REPORT_MASK;
            }
            break;

        default:
            break;
        }

        /* PTS will send unknown category */
        if (capability_len > (temp_len + 2))
        {
            capability_len -= (temp_len + 2);
            p_capability += (temp_len + 2);
        }
        else
        {
            capability_len = 0;
        }
    }
}

uint8_t avdtp_capability_check(T_AVDTP_LINK       *p_link,
                               uint8_t            *p_capability,
                               uint16_t            capability_len,
                               T_AVDTP_LOCAL_SEP  *sep)
{
    uint8_t  temp_len;

    while (capability_len != 0)
    {
        temp_len = *(p_capability + 1);

        switch (*p_capability)
        {
        case SRV_CATEG_MEDIA_CODEC:
            if ((*(p_capability + 3) != SBC_MEDIA_CODEC_TYPE) &&
                (*(p_capability + 3) != AAC_MEDIA_CODEC_TYPE) &&
                (*(p_capability + 3) != USAC_MEDIA_CODEC_TYPE) &&
                (*(p_capability + 3) != VENDOR_CODEC_TYPE))
            {
                return INVALID_CODEC_TYPE;
            }

            if (sep->codec_type == *(p_capability + 3))
            {
                if (sep->codec_type == SBC_MEDIA_CODEC_TYPE)
                {
                    uint8_t sample_frequency = *(p_capability + 4) & 0xf0;
                    uint8_t channel_mode = *(p_capability + 4) & 0x0f;
                    uint8_t block_number = *(p_capability + 5) & 0xf0;
                    uint8_t subbands = *(p_capability + 5) & 0x0c;
                    uint8_t allocation_method = *(p_capability + 5) & 0x03;

                    if ((sample_frequency != 0x80) && (sample_frequency != 0x40) &&
                        (sample_frequency != 0x20) && (sample_frequency != 0x10))
                    {
                        return INVALID_SAMPLING_FREQUENCY;
                    }
                    else if ((sample_frequency & (sep->media_codec_info[0] & 0xf0)) == 0)
                    {
                        return NOT_SUPPORTED_SAMPLING_FREQUENCY;
                    }

                    if ((channel_mode != 0x08) && (channel_mode != 0x04) &&
                        (channel_mode != 0x02) && (channel_mode != 0x01))
                    {
                        return INVALID_CHANNEL_MODE;
                    }
                    else if ((channel_mode & (sep->media_codec_info[0] & 0x0f)) == 0)
                    {
                        return NOT_SUPPORTED_CHANNEL_MODE;
                    }

                    if ((block_number != 0x80) && (block_number != 0x40) &&
                        (block_number != 0x20) && (block_number != 0x10))
                    {
                        return INVALID_BLOCK_LENGTH;
                    }
                    else if ((block_number & (sep->media_codec_info[1] & 0xf0)) == 0)
                    {
                        return INVALID_BLOCK_LENGTH;
                    }

                    if ((subbands != 0x08) && (subbands != 0x04))
                    {
                        return INVALID_SUBBANDS;
                    }
                    else if ((subbands & (sep->media_codec_info[1] & 0x0c)) == 0)
                    {
                        return NOT_SUPPORTED_SUBBANDS;
                    }

                    if ((allocation_method != 0x02) && (allocation_method != 0x01))
                    {
                        return INVALID_ALLOCATION_METHOD;
                    }
                    else if ((allocation_method & (sep->media_codec_info[1] & 0x03)) == 0)
                    {
                        return NOT_SUPPORTED_ALLOCATION_METHOD;
                    }

                    if (*(p_capability + 6) < sep->media_codec_info[2])
                    {
                        return INVALID_MINIMUM_BITPOOL_VALUE;
                    }

                    if (*(p_capability + 7) > sep->media_codec_info[3])
                    {
                        return INVALID_MAXIMUM_BITPOOL_VALUE;
                    }
                }
                else if (sep->codec_type == AAC_MEDIA_CODEC_TYPE)
                {
                    uint8_t  object_type = *(p_capability + 4) & 0xfe;
                    bool  drc = *(p_capability + 4) & 0x01;
                    uint16_t sampling_frequency = (*(p_capability + 5) << 8) + (*(p_capability + 6) & 0xf0);
                    uint8_t  channel_setting = *(p_capability + 6) & 0x0f;

                    if ((object_type != 0x80) && (object_type != 0x40) &&
                        (object_type != 0x20) && (object_type != 0x10) &&
                        (object_type != 0x08) && (object_type != 0x04) &&
                        (object_type != 0x02))
                    {
                        return INVALID_OBJECT_TYPE;
                    }
                    else if ((object_type & (sep->media_codec_info[0] & 0xfe)) == 0)
                    {
                        return NOT_SUPPORTED_OBJECT_TYPE;
                    }

                    if ((object_type == 0x80) && drc)
                    {
                        return INVALID_DRC;
                    }
                    else if (drc != (sep->media_codec_info[0] & 0x01))
                    {
                        return NOT_SUPPORTED_DRC;
                    }

                    if ((sampling_frequency != 0x8000) && (sampling_frequency != 0x4000) &&
                        (sampling_frequency != 0x2000) && (sampling_frequency != 0x1000) &&
                        (sampling_frequency != 0x0800) && (sampling_frequency != 0x0400) &&
                        (sampling_frequency != 0x0200) && (sampling_frequency != 0x0100) &&
                        (sampling_frequency != 0x0080) && (sampling_frequency != 0x0040) &&
                        (sampling_frequency != 0x0020) && (sampling_frequency != 0x0010))
                    {
                        return INVALID_SAMPLING_FREQUENCY;
                    }
                    else if ((sampling_frequency & ((sep->media_codec_info[1] << 8) +
                                                    (sep->media_codec_info[2] & 0xf0))) == 0)
                    {
                        return NOT_SUPPORTED_SAMPLING_FREQUENCY;
                    }

                    if ((channel_setting != 0x08) && (channel_setting != 0x04) &&
                        (channel_setting != 0x02) && (channel_setting != 0x01))
                    {
                        return INVALID_CHANNELS;
                    }
                    else if ((channel_setting & (sep->media_codec_info[2] & 0x0f)) == 0)
                    {
                        return NOT_SUPPORTED_CHANNELS;
                    }
                }

                p_link->sig_chann.codec_type = *(p_capability + 3);

                if (p_link->sig_chann.codec_type == VENDOR_CODEC_TYPE)
                {
                    if (!memcmp(p_capability + 4, avdtp_ldac_codec_info, 6))
                    {
                        p_link->sig_chann.codec_subtype = CODEC_TYPE_LDAC;
                    }
                    else if (!memcmp(p_capability + 4, avdtp_lc3_codec_info, 6))
                    {
                        p_link->sig_chann.codec_subtype = CODEC_TYPE_LC3;
                    }
                    else if (!memcmp(p_capability + 4, avdtp_lhdc_v5_codec_info, 6))
                    {
                        p_link->sig_chann.codec_subtype = CODEC_TYPE_LHDC;
                    }
                }

                if (temp_len > 2)
                {
                    if ((temp_len - 2) > MAX_CODEC_INFO_SIZE)
                    {
                        memcpy(&(p_link->sig_chann.codec_info[0]),
                               (p_capability + 4), MAX_CODEC_INFO_SIZE);
                    }
                    else
                    {
                        memcpy(&(p_link->sig_chann.codec_info[0]),
                               (p_capability + 4), (temp_len - 2));
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
                if (p_avdtp->service_category & CATEG_CP_MASK)
                {
                    p_link->sig_chann.enable_cp_flag = 1;
                }
            }
            break;

        case SRV_CATEG_DELAY_REPORT:
            {
                if (p_avdtp->service_category & CATEG_DELAY_REPORT_MASK)
                {
                    p_link->sig_chann.delay_report_flag = 1;
                }
            }
            break;

        default:
            break;
        }

        /*PTS will send unknown cartegory*/
        if (capability_len > (temp_len + 2))
        {
            capability_len -= (temp_len + 2);
            p_capability += (temp_len + 2);
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
    T_AVDTP_LINK *p_link;
    uint8_t *p_packet;
    uint8_t  packet_len;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        packet_len = sizeof(T_AVDTP_SIG_HDR);
        p_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, packet_len,
                                   p_avdtp->data_offset, false);
        if (p_packet != NULL)
        {
            PROTOCOL_PRINT_TRACE1("avdtp_signal_discover_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            p_packet[p_avdtp->data_offset] = (p_link->sig_chann.tx_trans_label | (AVDTP_PKT_TYPE_SINGLE << 2) |
                                              AVDTP_MSG_TYPE_CMD);
            p_packet[p_avdtp->data_offset + 1] = AVDTP_DISCOVER;
            mpa_send_l2c_data_req(p_packet, p_avdtp->data_offset, p_link->sig_chann.cid, packet_len, false);
            p_link->sig_chann.tx_trans_label += 0x10;
            p_link->sig_chann.cmd_flag = AVDTP_DISCOVER;
            sys_timer_start(p_link->timer_handle);

            return true;
        }
    }

    return false;
}

bool avdtp_signal_abort_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;
    uint8_t     *p_packet;
    uint8_t     packet_len;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        packet_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        p_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, packet_len,
                                   p_avdtp->data_offset, false);
        if (p_packet != NULL)
        {
            PROTOCOL_PRINT_TRACE1("avdtp_signal_abort_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            p_packet[p_avdtp->data_offset] = (p_link->sig_chann.tx_trans_label |
                                              (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            p_packet[p_avdtp->data_offset + 1] = AVDTP_ABORT;
            if (p_link->sig_chann.int_flag == 1)
            {
                p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.acp_seid;
            }
            else
            {
                p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.int_seid;
            }
            mpa_send_l2c_data_req(p_packet, p_avdtp->data_offset, p_link->sig_chann.cid, packet_len, false);
            p_link->sig_chann.tx_trans_label += 0x10;
            p_link->sig_chann.cmd_flag = AVDTP_ABORT;
            sys_timer_start(p_link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_delay_report_req(uint8_t  bd_addr[6],
                                   uint16_t latency)
{
    T_AVDTP_LINK *p_link;
    uint8_t         *p_pkt;
    uint8_t         pkt_len;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->sig_chann.sig_state != AVDTP_SIG_STATE_CFG &&
            p_link->sig_chann.sig_state != AVDTP_SIG_STATE_OPENING &&
            p_link->sig_chann.sig_state != AVDTP_SIG_STATE_OPEN &&
            p_link->sig_chann.sig_state != AVDTP_SIG_STATE_STREAMING)
        {
            return false;
        }

        if (p_link->sig_chann.delay_report_flag && (p_link->conn_role == AVDTP_TSEP_SRC))
        {
            pkt_len = (sizeof(T_AVDTP_SIG_HDR) + 3);
            p_pkt = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, pkt_len,
                                    p_avdtp->data_offset, false);
            if (p_pkt != NULL)
            {
                PROTOCOL_PRINT_TRACE1("avdtp_signal_delay_report_req: bd_addr %s", TRACE_BDADDR(bd_addr));

                p_pkt[p_avdtp->data_offset] = (p_link->sig_chann.tx_trans_label |
                                               (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
                p_pkt[p_avdtp->data_offset + 1] = AVDTP_DELAY_REPORT;
                if (p_link->sig_chann.int_flag == 1)
                {
                    p_pkt[p_avdtp->data_offset + 2] = p_link->sig_chann.acp_seid;
                }
                else
                {
                    p_pkt[p_avdtp->data_offset + 2] = p_link->sig_chann.int_seid;
                }
                /* latency unit in 0.1ms */
                p_pkt[p_avdtp->data_offset + 3] = (uint8_t)((latency * 10) >> 8);
                p_pkt[p_avdtp->data_offset + 4] = (uint8_t)(latency * 10);
                mpa_send_l2c_data_req(p_pkt, p_avdtp->data_offset, p_link->sig_chann.cid, pkt_len, false);
                p_link->sig_chann.tx_trans_label += 0x10;
                return true;
            }
        }
    }

    return false;
}

bool avdtp_signal_get_capability_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;
    uint8_t     *p_packet;
    uint8_t     packet_len;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        packet_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        p_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, packet_len,
                                   p_avdtp->data_offset, false);
        if (p_packet != NULL)
        {
            uint8_t     temp;

            PROTOCOL_PRINT_TRACE1("avdtp_signal_get_capability_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            p_packet[p_avdtp->data_offset] = (p_link->sig_chann.tx_trans_label |
                                              (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            if (p_link->avdtp_ver >= AVDTP_VERSON_V1_3)
            {
                p_link->sig_chann.cmd_flag = AVDTP_GET_ALL_CPBS;
            }
            else
            {
                p_link->sig_chann.cmd_flag = AVDTP_GET_CPBS;
            }
            p_packet[p_avdtp->data_offset + 1] = p_link->sig_chann.cmd_flag;
            temp = p_link->sig_chann.acp_seid_idx;
            p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.avdtp_sep[temp].sep_id;

            mpa_send_l2c_data_req(p_packet, p_avdtp->data_offset, p_link->sig_chann.cid, packet_len, false);
            p_link->sig_chann.tx_trans_label += 0x10;
            sys_timer_start(p_link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_cfg_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;
    uint8_t      *p_packet;
    uint8_t       packet_len;
    uint8_t       capability_length;
    uint8_t       capability_flag;
    uint8_t       i;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->sig_chann.sig_state != AVDTP_SIG_STATE_IDLE)
        {
            return false;
        }

        i = p_link->sig_chann.acp_seid_idx;
        capability_length = 0;
        capability_flag = 0;
        if ((p_link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_MIDIA_TRANS_MASK)
            && (p_avdtp->service_category & CATEG_MIDIA_TRANS_MASK))
        {
            capability_length += (MEDIA_TRANS_LOSC + 2);
            capability_flag |= CATEG_MIDIA_TRANS_MASK;
        }

        if ((p_link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_CP_MASK)
            && (p_avdtp->service_category & CATEG_CP_MASK))
        {
            capability_length += (CP_LOSC + 2);
            capability_flag |= CATEG_CP_MASK;
        }

        if ((p_link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_DELAY_REPORT_MASK)
            && (p_avdtp->service_category & CATEG_DELAY_REPORT_MASK))
        {
            capability_length += (DELAY_REPORT_LOSC + 2);
            capability_flag |= CATEG_DELAY_REPORT_MASK;
        }

        if ((p_link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_MEDIA_CODEC_MASK)
            && (p_avdtp->service_category & CATEG_MEDIA_CODEC_MASK))
        {
            if (p_link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
            {
                if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                {
                    capability_length += (MEDIA_CODEC_LDAC_LOSC + 2);
                }
                else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                {
                    capability_length += (MEDIA_CODEC_LC3_LOSC + 2);
                }
                else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                {
                    capability_length += (MEDIA_CODEC_LHDC_LOSC + 2);
                }
            }
            else if (p_link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
            {
                capability_length += (MEDIA_CODEC_AAC_LOSC + 2);
            }
            else
            {
                capability_length += (MEDIA_CODEC_SBC_LOSC + 2);
            }
            capability_flag |= CATEG_MEDIA_CODEC_MASK;
        }

        p_link->sig_chann.enable_cp_flag = 0;
        packet_len = (sizeof(T_AVDTP_SIG_HDR) + 2 + capability_length);
        p_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, packet_len,
                                   p_avdtp->data_offset, false);
        if (p_packet != NULL)
        {
            uint8_t     index;
            uint8_t     j = 0;
            T_AVDTP_LOCAL_SEP *sep;

            index = p_avdtp->data_offset;
            p_packet[index++] = (p_link->sig_chann.tx_trans_label |
                                 (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            p_packet[index++] = AVDTP_SET_CFG;
            p_packet[index++] = p_link->sig_chann.avdtp_sep[i].sep_id;

            sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
            while (sep != NULL)
            {
                if ((sep->sub_codec_type == p_link->sig_chann.avdtp_sep[i].media_codec_subtype) &&
                    (sep->tsep != p_link->conn_role))
                {
                    p_packet[index++] = sep->sep_id << 2; //INT_SEID
                    break;
                }
                sep = sep->next;
            }

            if (sep == NULL)
            {
                return false;
            }

            while (p_link->sig_chann.avdtp_sep[i].cpbs_order[j] != 0xff)
            {
                switch (p_link->sig_chann.avdtp_sep[i].cpbs_order[j])
                {
                case SRV_CATEG_MEDIA_TRANS:
                    if (capability_flag & CATEG_MIDIA_TRANS_MASK)
                    {
                        p_packet[index++] = SRV_CATEG_MEDIA_TRANS;
                        p_packet[index++] = MEDIA_TRANS_LOSC;
                    }
                    break;

                case SRV_CATEG_CP:
                    if (capability_flag & CATEG_CP_MASK)
                    {
                        p_link->sig_chann.enable_cp_flag = 1;
                        p_packet[index++] = SRV_CATEG_CP;
                        p_packet[index++] = CP_LOSC;
                        p_packet[index++] = CP_TYPE_LSB;
                        p_packet[index++] = CP_TYPE_MSB;
                    }
                    break;

                case SRV_CATEG_MEDIA_CODEC:
                    if (capability_flag & CATEG_MEDIA_CODEC_MASK)
                    {
                        uint8_t temp;

                        p_link->sig_chann.codec_type = p_link->sig_chann.avdtp_sep[i].media_codec_type;

                        p_packet[index++] = SRV_CATEG_MEDIA_CODEC;
                        if (p_link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                        {
                            p_link->sig_chann.codec_subtype = p_link->sig_chann.avdtp_sep[i].media_codec_subtype;
                            if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                            {
                                avdtp_vendor_codec_ldac_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                                &p_packet[index],
                                                                p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                sep,
                                                                p_link->conn_role);
                            }
                            else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                            {
                                avdtp_vendor_codec_lc3_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                               &p_packet[index],
                                                               p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                               sep,
                                                               p_link->conn_role);
                            }
                            else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                            {
                                avdtp_vendor_codec_lhdc_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                                &p_packet[index],
                                                                p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                sep,
                                                                p_link->conn_role);
                            }
                        }
                        else if (p_link->sig_chann.avdtp_sep[i].media_codec_type == USAC_MEDIA_CODEC_TYPE)
                        {
                            avdtp_codec_usac_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                     &p_packet[index],
                                                     p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                     sep,
                                                     p_link->conn_role);
                        }
                        else if (p_link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
                        {
                            avdtp_codec_aac_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                    &p_packet[index],
                                                    p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                    sep,
                                                    p_link->conn_role);
                        }
                        else if (p_link->sig_chann.avdtp_sep[i].media_codec_type == SBC_MEDIA_CODEC_TYPE)
                        {
                            avdtp_codec_sbc_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                    &p_packet[index],
                                                    p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                    sep,
                                                    p_link->conn_role);
                        }

                        temp = p_packet[index];
                        memcpy(&(p_link->sig_chann.codec_info[0]), &(p_packet[(index + 3)]), (temp - 2));
                        index += (temp + 1);
                    }
                    break;

                case SRV_CATEG_DELAY_REPORT:
                    if (capability_flag & CATEG_DELAY_REPORT_MASK)
                    {
                        p_packet[index++] = SRV_CATEG_DELAY_REPORT;
                        p_packet[index++] = DELAY_REPORT_LOSC;
                    }
                    break;

                //case SRV_CATEG_REPORTING:
                //case SRV_CATEG_RECOVERY:
                //case SRV_CATEG_HC:
                //case SRV_CATEG_MULTIPLEXING:
                default:
                    break;
                }
                j++;
            }
            PROTOCOL_PRINT_TRACE3("avdtp_signal_cfg_req: bd_addr %s, acp_seid 0x%02x, int_seid 0x%02x",
                                  TRACE_BDADDR(bd_addr), p_packet[p_avdtp->data_offset + 2] >> 2,
                                  p_packet[p_avdtp->data_offset + 3] >> 2);
            mpa_send_l2c_data_req(p_packet, p_avdtp->data_offset, p_link->sig_chann.cid, packet_len, false);
            p_link->sig_chann.tx_trans_label += 0x10;
            p_link->sig_chann.cmd_flag = AVDTP_SET_CFG;
            p_link->sig_chann.int_flag = 1;
            p_link->role = sep->tsep;
            sys_timer_start(p_link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_recfg_req(uint8_t bd_addr[6],
                            uint8_t codec_type,
                            uint8_t role)
{
    T_AVDTP_LINK *p_link;
    uint8_t      *p_packet;
    uint8_t       packet_len;
    uint8_t       capability_length;
    uint8_t       capability_flag;
    uint8_t       media_codec_type;
    uint8_t       media_codec_subtype;
    uint8_t       i;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->sig_chann.sig_state != AVDTP_SIG_STATE_OPEN)
        {
            return false;
        }

        if (p_link->sig_chann.acp_sep_no == 0)
        {
            p_link->conn_role = role;
            p_link->sig_chann.recfg_codec_type = codec_type;
            return avdtp_signal_discover_req(bd_addr);
        }

        switch (codec_type)
        {
        case A2DP_CODEC_TYPE_SBC:
            {
                media_codec_type = SBC_MEDIA_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_SBC;
            }
            break;

        case A2DP_CODEC_TYPE_AAC:
            {
                media_codec_type = AAC_MEDIA_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_AAC;
            }
            break;

        case A2DP_CODEC_TYPE_USAC:
            {
                media_codec_type = USAC_MEDIA_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_USAC;
            }
            break;

        case A2DP_CODEC_TYPE_LDAC:
            {
                media_codec_type = VENDOR_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_LDAC;
            }
            break;

        case A2DP_CODEC_TYPE_LC3:
            {
                media_codec_type = VENDOR_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_LC3;
            }
            break;

        case A2DP_CODEC_TYPE_LHDC:
            {
                media_codec_type = VENDOR_CODEC_TYPE;
                media_codec_subtype = CODEC_TYPE_LHDC;
            }
            break;

        default:
            return false;
        }

        for (i = 0; i < p_link->sig_chann.acp_sep_no; i++)
        {
            if (p_link->sig_chann.avdtp_sep[i].media_codec_type == media_codec_type)
            {
                if (p_link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                {
                    if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == media_codec_subtype)
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

        if (i < p_link->sig_chann.acp_sep_no)
        {
            capability_length = 0;
            capability_flag = 0;

            if ((p_link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_CP_MASK)
                && (p_avdtp->service_category & CATEG_CP_MASK))
            {
                capability_length += (CP_LOSC + 2);
                capability_flag |= CATEG_CP_MASK;
            }

            if ((p_link->sig_chann.avdtp_sep[i].cpbs_mask & CATEG_MEDIA_CODEC_MASK)
                && (p_avdtp->service_category & CATEG_MEDIA_CODEC_MASK))
            {
                if (p_link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                {
                    if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                    {
                        capability_length += (MEDIA_CODEC_LDAC_LOSC + 2);
                    }
                    else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                    {
                        capability_length += (MEDIA_CODEC_LC3_LOSC + 2);
                    }
                    else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                    {
                        capability_length += (MEDIA_CODEC_LHDC_LOSC + 2);
                    }
                }
                else if (p_link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
                {
                    capability_length += (MEDIA_CODEC_AAC_LOSC + 2);
                }
                else
                {
                    capability_length += (MEDIA_CODEC_SBC_LOSC + 2);
                }
                capability_flag |= CATEG_MEDIA_CODEC_MASK;
            }

            p_link->sig_chann.enable_cp_flag = 0;
            packet_len = (sizeof(T_AVDTP_SIG_HDR) + 1 + capability_length);
            p_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, packet_len,
                                       p_avdtp->data_offset, false);
            if (p_packet != NULL)
            {
                uint8_t     index;
                uint8_t     j = 0;

                index = p_avdtp->data_offset;
                p_packet[index++] = (p_link->sig_chann.tx_trans_label |
                                     (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
                p_packet[index++] = AVDTP_RECFG;
                p_packet[index++] = p_link->sig_chann.avdtp_sep[i].sep_id;

                while (p_link->sig_chann.avdtp_sep[i].cpbs_order[j] != 0xff)
                {
                    switch (p_link->sig_chann.avdtp_sep[i].cpbs_order[j])
                    {
                    case SRV_CATEG_CP:
                        if (capability_flag & CATEG_CP_MASK)
                        {
                            p_link->sig_chann.enable_cp_flag = 1;
                            p_packet[index++] = SRV_CATEG_CP;
                            p_packet[index++] = CP_LOSC;
                            p_packet[index++] = CP_TYPE_LSB;
                            p_packet[index++] = CP_TYPE_MSB;
                        }
                        break;

                    case SRV_CATEG_MEDIA_CODEC:
                        if (capability_flag & CATEG_MEDIA_CODEC_MASK)
                        {
                            uint8_t temp;
                            T_AVDTP_LOCAL_SEP *sep;

                            sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
                            while (sep != NULL)
                            {
                                if ((sep->sub_codec_type == p_link->sig_chann.avdtp_sep[i].media_codec_subtype) &&
                                    (sep->tsep != p_link->conn_role))
                                {
                                    break;
                                }
                                sep = sep->next;
                            }

                            if (sep == NULL)
                            {
                                return false;
                            }

                            p_link->sig_chann.codec_type = p_link->sig_chann.avdtp_sep[i].media_codec_type;

                            p_packet[index++] = SRV_CATEG_MEDIA_CODEC;
                            if (p_link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                            {
                                p_link->sig_chann.codec_subtype = p_link->sig_chann.avdtp_sep[i].media_codec_subtype;

                                if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                                {
                                    avdtp_vendor_codec_ldac_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                                    &p_packet[index],
                                                                    p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                    sep,
                                                                    p_link->conn_role);
                                }
                                else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                                {
                                    avdtp_vendor_codec_lc3_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                                   &p_packet[index],
                                                                   p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                   sep,
                                                                   p_link->conn_role);
                                }
                                else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                                {
                                    avdtp_vendor_codec_lhdc_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                                    &p_packet[index],
                                                                    p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                    sep,
                                                                    p_link->conn_role);
                                }
                            }
                            else if (p_link->sig_chann.avdtp_sep[i].media_codec_type == USAC_MEDIA_CODEC_TYPE)
                            {
                                avdtp_codec_usac_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                         &p_packet[index],
                                                         p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                         sep,
                                                         p_link->conn_role);
                            }
                            else if (p_link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
                            {
                                avdtp_codec_aac_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                        &p_packet[index],
                                                        p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                        sep,
                                                        p_link->conn_role);
                            }
                            else if (p_link->sig_chann.avdtp_sep[i].media_codec_type == SBC_MEDIA_CODEC_TYPE)
                            {
                                avdtp_codec_sbc_handler(HDL_TYPE_FILL_CODEC_INFO,
                                                        &p_packet[index],
                                                        p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                        sep,
                                                        p_link->conn_role);
                            }

                            temp = p_packet[index];
                            memcpy(&(p_link->sig_chann.codec_info[0]), &(p_packet[(index + 3)]), (temp - 2));
                            index += (temp + 1);
                        }
                        break;

                    default:
                        break;
                    }
                    j++;
                }

                mpa_send_l2c_data_req(p_packet, p_avdtp->data_offset, p_link->sig_chann.cid, packet_len, false);
                p_link->sig_chann.tx_trans_label += 0x10;
                p_link->sig_chann.cmd_flag = AVDTP_RECFG;
                p_link->sig_chann.int_flag = 1;
                sys_timer_start(p_link->timer_handle);
                return true;
            }
        }
    }

    return false;
}

bool avdtp_signal_open_req(uint8_t bd_addr[6],
                           uint8_t role)
{
    T_AVDTP_LINK *p_link;
    uint8_t     *p_packet;
    uint8_t     packet_len;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_IDLE)
        {
            p_link->conn_role = role;
            return avdtp_signal_discover_req(bd_addr);
        }
        else if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG)
        {
            packet_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
            p_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, packet_len,
                                       p_avdtp->data_offset, false);
            if (p_packet != NULL)
            {
                PROTOCOL_PRINT_TRACE1("avdtp_signal_open_req: bd_addr %s", TRACE_BDADDR(bd_addr));

                p_packet[p_avdtp->data_offset] = (p_link->sig_chann.tx_trans_label |
                                                  (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
                p_packet[p_avdtp->data_offset + 1] = AVDTP_OPEN;

                if (p_link->sig_chann.int_flag == 1)
                {
                    p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.acp_seid;
                }
                else
                {
                    p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.int_seid;
                }

                mpa_send_l2c_data_req(p_packet, p_avdtp->data_offset, p_link->sig_chann.cid, packet_len, false);
                p_link->sig_chann.tx_trans_label += 0x10;
                p_link->sig_chann.cmd_flag = AVDTP_OPEN;
                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_OPENING;
                sys_timer_start(p_link->timer_handle);

                return true;
            }
        }
    }

    return false;
}

bool avdtp_signal_start_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;
    uint8_t     *p_packet;
    uint8_t     packet_len;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        PROTOCOL_PRINT_TRACE3("avdtp_signal_start_req: bd_addr %s, sig_state 0x%02x, cmd_flag 0x%02x",
                              TRACE_BDADDR(bd_addr), p_link->sig_chann.sig_state, p_link->sig_chann.cmd_flag);

        if (p_link->sig_chann.sig_state != AVDTP_SIG_STATE_OPEN ||
            p_link->sig_chann.cmd_flag == AVDTP_START)
        {
            return false;
        }

        packet_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        p_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, packet_len,
                                   p_avdtp->data_offset, false);
        if (p_packet != NULL)
        {
            p_packet[p_avdtp->data_offset] = (p_link->sig_chann.tx_trans_label |
                                              (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            p_packet[p_avdtp->data_offset + 1] = AVDTP_START;
            if (p_link->sig_chann.int_flag == 1)
            {
                p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.acp_seid;
            }
            else
            {
                p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.int_seid;
            }
            mpa_send_l2c_data_req(p_packet, p_avdtp->data_offset, p_link->sig_chann.cid, packet_len, false);
            p_link->sig_chann.tx_trans_label += 0x10;
            p_link->sig_chann.cmd_flag = AVDTP_START;
            sys_timer_start(p_link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_suspend_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;
    uint8_t     *p_packet;
    uint8_t     packet_len;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        PROTOCOL_PRINT_TRACE3("avdtp_signal_suspend_req: bd_addr %s, sig_state 0x%02x, cmd_flag 0x%02x",
                              TRACE_BDADDR(bd_addr), p_link->sig_chann.sig_state, p_link->sig_chann.cmd_flag);

        if (p_link->sig_chann.sig_state != AVDTP_SIG_STATE_STREAMING ||
            p_link->sig_chann.cmd_flag == AVDTP_SUSPEND)
        {
            return false;
        }

        packet_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        p_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, packet_len,
                                   p_avdtp->data_offset, false);
        if (p_packet != NULL)
        {
            p_packet[p_avdtp->data_offset] = (p_link->sig_chann.tx_trans_label |
                                              (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            p_packet[p_avdtp->data_offset + 1] = AVDTP_SUSPEND;
            if (p_link->sig_chann.int_flag == 1)
            {
                p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.acp_seid;
            }
            else
            {
                p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.int_seid;
            }

            mpa_send_l2c_data_req(p_packet, p_avdtp->data_offset, p_link->sig_chann.cid, packet_len, false);
            p_link->sig_chann.tx_trans_label += 0x10;
            p_link->sig_chann.cmd_flag = AVDTP_SUSPEND;
            sys_timer_start(p_link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_signal_close_req(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;
    uint8_t     *p_packet;
    uint8_t     packet_len;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if ((p_link->sig_chann.sig_state != AVDTP_SIG_STATE_OPEN)
            && (p_link->sig_chann.sig_state != AVDTP_SIG_STATE_STREAMING))
        {
            return false;
        }

        packet_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        p_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, packet_len,
                                   p_avdtp->data_offset, false);
        if (p_packet != NULL)
        {
            PROTOCOL_PRINT_TRACE1("avdtp_signal_close_req: bd_addr %s", TRACE_BDADDR(bd_addr));

            p_packet[p_avdtp->data_offset] = (p_link->sig_chann.tx_trans_label |
                                              (AVDTP_PKT_TYPE_SINGLE << 2) | AVDTP_MSG_TYPE_CMD);
            p_packet[p_avdtp->data_offset + 1] = AVDTP_CLOSE;
            if (p_link->sig_chann.int_flag == 1)
            {
                p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.acp_seid;
            }
            else
            {
                p_packet[p_avdtp->data_offset + 2] = p_link->sig_chann.int_seid;
            }
            mpa_send_l2c_data_req(p_packet, p_avdtp->data_offset, p_link->sig_chann.cid, packet_len, false);
            p_link->sig_chann.tx_trans_label += 0x10;
            p_link->sig_chann.cmd_flag = AVDTP_CLOSE;
            sys_timer_start(p_link->timer_handle);
            return true;
        }
    }

    return false;
}

bool avdtp_cmd_bad_state_proc(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;
    uint8_t     *p_signal_pkt;
    uint8_t     *p_rsp_packet;
    uint8_t     signal_id;
    uint8_t     trans_label;
    uint8_t     rsp_len;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        p_signal_pkt = p_link->sig_chann.p_fragment_data;
        trans_label = (*p_signal_pkt & 0xF0);
        signal_id = (*(p_signal_pkt + 1)) & 0x3F;

        PROTOCOL_PRINT_WARN3("avdtp_cmd_bad_state_proc: bd_addr %s, signal_id 0x%02x, cid 0x%04x",
                             TRACE_BDADDR(bd_addr), signal_id, p_link->sig_chann.cid);

        switch (signal_id)
        {
        case AVDTP_SET_CFG:
            rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 2); /* service category and error code */
            break;
        case AVDTP_OPEN:
        case AVDTP_CLOSE:
        case AVDTP_DELAY_REPORT:
            rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 1); /* error code */
            break;
        case AVDTP_START:
        case AVDTP_SUSPEND:
            rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 2); /* ACP SEID and error code */
            break;

        default:
            rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 1); /* error code */
            break;
        }

        p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                       p_avdtp->data_offset, false);
        if (p_rsp_packet != NULL)
        {
            uint8_t     index;

            index = p_avdtp->data_offset;
            p_rsp_packet[index] = (trans_label | (AVDTP_PKT_TYPE_SINGLE << 2) |
                                   AVDTP_MSG_TYPE_RSP_REJECT);
            index++;
            p_rsp_packet[index] = signal_id;
            index++;
            switch (signal_id)
            {
            case AVDTP_OPEN:
            case AVDTP_CLOSE:
            case AVDTP_DELAY_REPORT:
                p_rsp_packet[index] = BAD_STATE;
                index++;
                break;
            case AVDTP_SET_CFG:
            case AVDTP_RECFG:
                p_rsp_packet[index] = SRV_CATEG_MEDIA_CODEC;
                index++;
                p_rsp_packet[index] = BAD_STATE;
                index++;
                break;
            case AVDTP_START:
            case AVDTP_SUSPEND:
                p_rsp_packet[index] = *(p_signal_pkt + 2);
                index++;
                p_rsp_packet[index] = BAD_STATE;
                index++;
                break;
            default:
                p_rsp_packet[index] = BAD_STATE;
                index++;
                break;
            }

            mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                  p_link->sig_chann.cid, rsp_len, false);
            return true;
        }
    }

    return false;
}

bool avdtp_cmd_error_check_proc(T_AVDTP_LINK *p_link)
{
    uint16_t avdtp_pkt_len;
    uint8_t trans_label;
    uint8_t acp_seid;
    uint8_t signal_id;
    uint8_t *p_signal_pkt;
    uint8_t error_code = 0;
    uint8_t rsp_len = 0;
    uint8_t error_category_type = 0;
    T_AVDTP_LOCAL_SEP  *sep;

    p_signal_pkt = p_link->sig_chann.p_fragment_data;
    avdtp_pkt_len = p_link->sig_chann.fragment_data_len;
    acp_seid = ((*(p_signal_pkt + 2)) & 0xFC) >> 2;
    trans_label = (*p_signal_pkt & 0xF0);
    signal_id = ((*(p_signal_pkt + 1)) & 0x3F);

    sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
    while (sep != NULL)
    {
        if (sep->sep_id == acp_seid)
        {
            break;
        }
        sep = sep->next;
    }

    switch (signal_id)
    {
    case AVDTP_DISCOVER:
        rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        if (avdtp_pkt_len != 2)
        {
            error_code = BAD_LENGTH;
        }
        break;

    case AVDTP_GET_CPBS:
    case AVDTP_GET_ALL_CPBS:
        rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        if (sep == NULL)
        {
            error_code = BAD_ACP_SEID;
        }

        if (avdtp_pkt_len != 3)
        {
            error_code = BAD_LENGTH;
        }
        break;

    case AVDTP_SET_CFG:
        rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 2);
        if (sep == NULL)
        {
            error_code = BAD_ACP_SEID;
        }
        if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG)
        {
            error_code = SEP_IN_USE;
        }
        if (avdtp_pkt_len < 4)
        {
            error_code = BAD_LENGTH;
        }
        else if (avdtp_pkt_len == 4)
        {
            error_code = BAD_SERV_CATEG;
            error_category_type = 0x00;
        }
        else
        {
            uint8_t *p_capability_ptr;
            uint8_t capability_len;

            p_capability_ptr = (p_signal_pkt + 4);
            capability_len = avdtp_pkt_len - 4;
            while (capability_len != 0)
            {
                if (*p_capability_ptr > SRV_CATEG_DELAY_REPORT)
                {
                    error_code = BAD_SERV_CATEG;
                    error_category_type = *p_capability_ptr;
                    break;
                }
                else
                {
                    if (*p_capability_ptr == SRV_CATEG_MEDIA_CODEC)
                    {
                        if (((*(p_capability_ptr + 2) >> 4) & 0x0F) != AUDIO_MEDIA_TYPE)
                        {
                            error_code = INVALID_CPBS;
                            break;
                        }
                    }
                    else if (*p_capability_ptr == SRV_CATEG_MEDIA_TRANS)
                    {
                        if (*(p_capability_ptr + 1) != MEDIA_TRANS_LOSC)
                        {
                            error_code = BAD_MEDIA_TRANSPORT_TYPE;
                            break;
                        }
                    }
                    capability_len -= (*(p_capability_ptr + 1) + 2);
                    p_capability_ptr = (p_capability_ptr + * (p_capability_ptr + 1) + 2);
                }
            }
        }
        break;

    case AVDTP_GET_CFG:
        rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        if (sep == NULL)
        {
            error_code = BAD_ACP_SEID;
        }

        if (avdtp_pkt_len != 3)
        {
            error_code = BAD_LENGTH;
        }
        break;

    case AVDTP_RECFG:
        {
            rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 2);
            if (sep != NULL)
            {
                uint8_t *p_capability_ptr;
                uint8_t capability_len;

                p_capability_ptr = (p_signal_pkt + 3);
                capability_len = avdtp_pkt_len - 3;
                while (capability_len != 0)
                {
                    switch (*p_capability_ptr)
                    {
                    case SRV_CATEG_CP:
                        capability_len -= (*(p_capability_ptr + 1) + 2);
                        p_capability_ptr = (p_capability_ptr + * (p_capability_ptr + 1) + 2);
                        break;

                    case SRV_CATEG_MEDIA_CODEC:
                        if (((*(p_capability_ptr + 2) >> 4) & 0x0F) != AUDIO_MEDIA_TYPE)
                        {
                            error_code = INVALID_CPBS;
                            error_category_type = *p_capability_ptr;
                        }
                        capability_len -= (*(p_capability_ptr + 1) + 2);
                        p_capability_ptr = (p_capability_ptr + * (p_capability_ptr + 1) + 2);
                        break;

                    case SRV_CATEG_MEDIA_TRANS:
                        error_code = INVALID_CPBS;
                        error_category_type = *p_capability_ptr;
                        capability_len = 0;
                        break;

                    default:
                        error_code = BAD_SERV_CATEG;
                        error_category_type = *p_capability_ptr;
                        capability_len = 0;
                        break;
                    }
                }
            }
            else
            {
                error_code = BAD_ACP_SEID;
            }
        }
        break;

    case AVDTP_OPEN:
        rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        if (sep == NULL)
        {
            error_code = BAD_ACP_SEID;
        }

        if (avdtp_pkt_len != 3)
        {
            error_code = BAD_LENGTH;
        }
        break;

    case AVDTP_START:
        rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 2);
        if (sep == NULL)
        {
            error_code = BAD_ACP_SEID;
        }

        if (avdtp_pkt_len < 3)
        {
            error_code = BAD_LENGTH;
        }
        break;

    case AVDTP_CLOSE:
        rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        if (sep == NULL)
        {
            error_code = BAD_ACP_SEID;
        }
        if (avdtp_pkt_len != 3)
        {
            error_code = BAD_LENGTH;
        }
        break;

    case AVDTP_SUSPEND:
        rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 2);
        if (sep == NULL)
        {
            error_code = BAD_ACP_SEID;
        }

        if (avdtp_pkt_len < 3)
        {
            error_code = BAD_LENGTH;
        }
        break;

    case AVDTP_SECURITY_CONTROL:
        rsp_len = (sizeof(T_AVDTP_SIG_HDR) + 1);
        if (sep == NULL)
        {
            error_code = BAD_ACP_SEID;
        }
        if (avdtp_pkt_len < 3)
        {
            error_code = BAD_LENGTH;
        }
        break;

    default:
        break;
    }

    if (error_code != 0)
    {
        uint8_t *p_rsp_packet;
        p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                       p_avdtp->data_offset, false);
        if (p_rsp_packet != NULL)
        {
            uint8_t index;

            PROTOCOL_PRINT_WARN3("avdtp_cmd_error_check_proc: bd_addr %s, signal_id 0x%02x, cid 0x%04x",
                                 TRACE_BDADDR(p_link->bd_addr), signal_id, p_link->sig_chann.cid);

            index = p_avdtp->data_offset;
            p_rsp_packet[index] = (trans_label | (AVDTP_PKT_TYPE_SINGLE << 2) |
                                   AVDTP_MSG_TYPE_RSP_REJECT);
            index++;
            p_rsp_packet[index] = signal_id;
            index++;
            switch (signal_id)
            {
            case AVDTP_DISCOVER:
            case AVDTP_GET_CPBS:
            case AVDTP_GET_ALL_CPBS:
            case AVDTP_GET_CFG:
            case AVDTP_OPEN:
            case AVDTP_CLOSE:
            case AVDTP_SECURITY_CONTROL:
                p_rsp_packet[index] = error_code;
                index++;
                break;
            case AVDTP_SET_CFG:
            case AVDTP_RECFG:
                p_rsp_packet[index] = error_category_type;
                index++;
                p_rsp_packet[index] = error_code;
                index++;
                break;
            case AVDTP_START:
            case AVDTP_SUSPEND:
                p_rsp_packet[index] = *(p_signal_pkt + 2);
                index++;
                p_rsp_packet[index] = error_code;
                index++;
                break;
            }
            mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                  p_link->sig_chann.cid, rsp_len, false);
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
    T_AVDTP_LINK *p_link;
    uint8_t       rsp_len;
    uint8_t       index;
    uint8_t       trans_label;
    uint8_t      *p_rsp_packet;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        trans_label = p_link->sig_chann.rx_start_trans_label;

        if (accept == true)
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR);
            p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                           p_avdtp->data_offset, false);

            if (p_rsp_packet != NULL)
            {
                index = p_avdtp->data_offset;
                p_rsp_packet[index] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                       AVDTP_MSG_TYPE_RSP_ACCEPT);
                index++;
                p_rsp_packet[index] = AVDTP_START;
                index++;
                mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                      p_link->sig_chann.cid, rsp_len, false);

                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_STREAMING;
                return true;
            }
        }
        else
        {
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2;
            p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                           p_avdtp->data_offset, false);

            if (p_rsp_packet != NULL)
            {
                index = p_avdtp->data_offset;
                p_rsp_packet[index] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                       AVDTP_MSG_TYPE_RSP_REJECT);
                index++;
                p_rsp_packet[index] = AVDTP_START;
                index++;
                p_rsp_packet[index] = p_link->sig_chann.int_seid;
                index++;
                p_rsp_packet[index] = BAD_STATE;
                index++;
                mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                      p_link->sig_chann.cid, rsp_len, false);
                return true;
            }
        }
    }

    return false;
}

void avdtp_rx_cmd_proc(T_AVDTP_LINK *p_link)
{
    uint16_t    avdtp_pkt_len;
    uint8_t     trans_label;
    uint8_t     acp_seid;
    uint8_t     signal_id;
    uint8_t    *p_signal_pkt;
    uint8_t    *p_rsp_packet;
    uint16_t    cid;

    p_signal_pkt = p_link->sig_chann.p_fragment_data;
    avdtp_pkt_len = p_link->sig_chann.fragment_data_len;
    acp_seid = ((*(p_signal_pkt + 2)) & 0xFC) >> 2;
    trans_label = (*p_signal_pkt & 0xF0);
    signal_id = ((*(p_signal_pkt + 1)) & 0x3F);
    cid = p_link->sig_chann.cid;

    PROTOCOL_PRINT_INFO3("avdtp_rx_cmd_proc: bd_addr %s, signal_id 0x%02x, cid 0x%04x",
                         TRACE_BDADDR(p_link->bd_addr), signal_id, cid);

    if (avdtp_cmd_error_check_proc(p_link) == false)
    {
        uint8_t     index;
        uint8_t     rsp_len;

        switch (signal_id)
        {
        case AVDTP_DISCOVER:
            {
                uint8_t seid_in_use;
                T_AVDTP_LOCAL_SEP  *sep;

                rsp_len = sizeof(T_AVDTP_SIG_HDR);

                p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, cid, 0,
                                               rsp_len + p_avdtp->local_sep_num * AVDTP_SEP_INFO_SIZE,
                                               p_avdtp->data_offset, false);
                if (p_rsp_packet != NULL)
                {
                    index = p_avdtp->data_offset;
                    p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                             AVDTP_MSG_TYPE_RSP_ACCEPT);
                    p_rsp_packet[index++] = AVDTP_DISCOVER;

                    sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
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

                        p_rsp_packet[index++] = (sep->sep_id << 2) | seid_in_use;
                        p_rsp_packet[index++] = (sep->media_type << 4) | (sep->tsep << 3);
                        rsp_len += AVDTP_SEP_INFO_SIZE;

                        sep = sep->next;
                    }

                    mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset, cid, rsp_len, false);
                }

                p_avdtp->cback(cid, AVDTP_MSG_SIG_DISCOVER_IND, NULL);
            }
            break;

        case AVDTP_GET_CPBS :
        case AVDTP_GET_ALL_CPBS:
            {
                uint8_t  capability_length;
                T_AVDTP_LOCAL_SEP  *sep;

                capability_length = 0;

                sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
                while (sep != NULL)
                {
                    if (sep->sep_id == acp_seid)
                    {
                        break;
                    }

                    sep = sep->next;
                }

                if (sep == NULL)
                {
                    return;
                }

                if (p_avdtp->service_category & CATEG_MIDIA_TRANS_MASK)
                {
                    capability_length += (MEDIA_TRANS_LOSC + 2);
                }
                if (p_avdtp->service_category & CATEG_MEDIA_CODEC_MASK)
                {
                    if (sep->sub_codec_type == CODEC_TYPE_SBC)
                    {
                        capability_length += (MEDIA_CODEC_SBC_LOSC + 2);
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_AAC)
                    {
                        capability_length += (MEDIA_CODEC_AAC_LOSC + 2);
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_USAC)
                    {
                        capability_length += (MEDIA_CODEC_USAC_LOSC + 2);
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_LDAC)
                    {
                        capability_length += (MEDIA_CODEC_LDAC_LOSC + 2);
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_LC3)
                    {
                        capability_length += (MEDIA_CODEC_LC3_LOSC + 2);
                    }
                    else if (sep->sub_codec_type == CODEC_TYPE_LHDC)
                    {
                        capability_length += (MEDIA_CODEC_LHDC_LOSC + 2);
                    }
                }
                if (p_avdtp->service_category & CATEG_CP_MASK)
                {
                    capability_length += (CP_LOSC + 2);
                }
                if (signal_id == AVDTP_GET_ALL_CPBS)
                {
                    capability_length += (DELAY_REPORT_LOSC + 2);
                }
                rsp_len = (sizeof(T_AVDTP_SIG_HDR) + capability_length);

                p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, cid, 0, rsp_len,
                                               p_avdtp->data_offset, false);
                if (p_rsp_packet != NULL)
                {
                    index = p_avdtp->data_offset;
                    p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                             AVDTP_MSG_TYPE_RSP_ACCEPT);
                    p_rsp_packet[index++] = signal_id;
                    if (p_avdtp->service_category & CATEG_MIDIA_TRANS_MASK)
                    {
                        p_rsp_packet[index++] = SRV_CATEG_MEDIA_TRANS;
                        p_rsp_packet[index++] = MEDIA_TRANS_LOSC;
                    }

                    if (p_avdtp->service_category & CATEG_MEDIA_CODEC_MASK)
                    {
                        p_rsp_packet[index++] = SRV_CATEG_MEDIA_CODEC;

                        if (sep->sub_codec_type == CODEC_TYPE_SBC)
                        {
                            p_rsp_packet[index++] = MEDIA_CODEC_SBC_LOSC;
                            p_rsp_packet[index++] = (AUDIO_MEDIA_TYPE << 4);
                            p_rsp_packet[index++] = SBC_MEDIA_CODEC_TYPE;
                            memcpy(&p_rsp_packet[index], &sep->media_codec_info[0], 4);
                            index += 4;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_AAC)
                        {
                            p_rsp_packet[index++] = MEDIA_CODEC_AAC_LOSC;
                            p_rsp_packet[index++] = (AUDIO_MEDIA_TYPE << 4);
                            p_rsp_packet[index++] = AAC_MEDIA_CODEC_TYPE;
                            memcpy(&p_rsp_packet[index], &sep->media_codec_info[0], 6);
                            index += 6;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_USAC)
                        {
                            p_rsp_packet[index++] = MEDIA_CODEC_USAC_LOSC;
                            p_rsp_packet[index++] = (AUDIO_MEDIA_TYPE << 4);
                            p_rsp_packet[index++] = USAC_MEDIA_CODEC_TYPE;
                            memcpy(&p_rsp_packet[index], &sep->media_codec_info[0], 7);
                            index += 7;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_LDAC)
                        {
                            p_rsp_packet[index++] = MEDIA_CODEC_LDAC_LOSC;
                            p_rsp_packet[index++] = (AUDIO_MEDIA_TYPE << 4);
                            p_rsp_packet[index++] = VENDOR_CODEC_TYPE;
                            memcpy(&p_rsp_packet[index], &sep->media_codec_info[0], 8);
                            index += 8;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_LC3)
                        {
                            p_rsp_packet[index++] = MEDIA_CODEC_LC3_LOSC;
                            p_rsp_packet[index++] = (AUDIO_MEDIA_TYPE << 4);
                            p_rsp_packet[index++] = VENDOR_CODEC_TYPE;
                            memcpy(&p_rsp_packet[index], &sep->media_codec_info[0], 9);
                            index += 9;
                        }
                        else if (sep->sub_codec_type == CODEC_TYPE_LHDC)
                        {
                            p_rsp_packet[index++] = MEDIA_CODEC_LHDC_LOSC;
                            p_rsp_packet[index++] = (AUDIO_MEDIA_TYPE << 4);
                            p_rsp_packet[index++] = VENDOR_CODEC_TYPE;
                            memcpy(&p_rsp_packet[index], &sep->media_codec_info[0], 11);
                            index += 11;
                        }
                    }

                    if (p_avdtp->service_category & CATEG_CP_MASK)
                    {
                        p_rsp_packet[index++] = SRV_CATEG_CP;
                        p_rsp_packet[index++] = CP_LOSC;
                        p_rsp_packet[index++] = CP_TYPE_LSB;
                        p_rsp_packet[index++] = CP_TYPE_MSB;
                    }

                    if (signal_id == AVDTP_GET_ALL_CPBS)
                    {
                        p_rsp_packet[index++] = SRV_CATEG_DELAY_REPORT;
                        p_rsp_packet[index++] = DELAY_REPORT_LOSC;
                    }
                    mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset, cid, rsp_len, false);
                }

                p_avdtp->cback(cid, AVDTP_MSG_SIG_GET_CPBS_IND, NULL);
            }
            break;

        case AVDTP_SET_CFG:
            {
                if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_IDLE)
                {
                    uint8_t result;
                    T_AVDTP_LOCAL_SEP  *sep;

                    sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
                    while (sep != NULL)
                    {
                        if (sep->sep_id == acp_seid)
                        {
                            break;
                        }

                        sep = sep->next;
                    }

                    if (sep == NULL)
                    {
                        return;
                    }

                    p_link->sig_chann.int_seid = *(p_signal_pkt + 3);
                    p_link->sig_chann.enable_cp_flag = 0;

                    result = avdtp_capability_check(p_link, (uint8_t *)(p_signal_pkt + 4), (avdtp_pkt_len - 4), sep);

                    if (result != 0)
                    {
                        rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2;
                        p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, cid, 0, rsp_len,
                                                       p_avdtp->data_offset, false);

                        if (p_rsp_packet != NULL)
                        {
                            index = p_avdtp->data_offset;
                            p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                                     AVDTP_MSG_TYPE_RSP_REJECT);
                            p_rsp_packet[index++] = AVDTP_SET_CFG;
                            p_rsp_packet[index++] = SRV_CATEG_MEDIA_CODEC;
                            p_rsp_packet[index++] = result;
                            mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset, cid, rsp_len, false);
                        }
                    }
                    else
                    {
                        uint8_t ind_data[4 + MAX_CODEC_INFO_SIZE];

                        if (p_link->sig_chann.p_cfg_cpbs != NULL)
                        {
                            free(p_link->sig_chann.p_cfg_cpbs);
                        }
                        p_link->sig_chann.cpbs_len = avdtp_pkt_len - 4;
                        p_link->sig_chann.p_cfg_cpbs = malloc((avdtp_pkt_len - 4));
                        if (p_link->sig_chann.p_cfg_cpbs != NULL)
                        {
                            memcpy(p_link->sig_chann.p_cfg_cpbs,
                                   (p_signal_pkt + 4), (avdtp_pkt_len - 4));

                            rsp_len = sizeof(T_AVDTP_SIG_HDR);
                            p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, cid, 0, rsp_len,
                                                           p_avdtp->data_offset, false);
                            if (p_rsp_packet != NULL)
                            {
                                index = p_avdtp->data_offset;
                                p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                                         AVDTP_MSG_TYPE_RSP_ACCEPT);
                                p_rsp_packet[index++] = AVDTP_SET_CFG;
                                mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset, cid, rsp_len, false);
                            }

                            p_link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;
                            p_link->sig_chann.int_flag = 0;
                            p_link->role = sep->tsep;

                            ind_data[0] = p_link->role;
                            ind_data[1] = p_link->sig_chann.codec_type;
                            ind_data[2] = p_link->sig_chann.enable_cp_flag;
                            ind_data[3] = p_link->sig_chann.delay_report_flag;
                            memcpy(&(ind_data[4]), &(p_link->sig_chann.codec_info[0]), MAX_CODEC_INFO_SIZE);
                            p_avdtp->cback(cid, AVDTP_MSG_SIG_SET_CFG_IND, &(ind_data[0]));

                            avdtp_signal_delay_report_req(p_link->bd_addr, p_avdtp->stream_latency);
                        }
                    }
                }
                else
                {
                    avdtp_cmd_bad_state_proc(p_link->bd_addr);
                }
            }
            break;

        case AVDTP_GET_CFG:
            {
                rsp_len = (sizeof(T_AVDTP_SIG_HDR) + p_link->sig_chann.cpbs_len);
                p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                               p_avdtp->data_offset, false);
                if (p_rsp_packet != NULL)
                {
                    index = p_avdtp->data_offset;
                    p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                             AVDTP_MSG_TYPE_RSP_ACCEPT);
                    p_rsp_packet[index++] = AVDTP_GET_CFG;
                    memcpy((uint8_t *)&p_rsp_packet[index],
                           (uint8_t *)p_link->sig_chann.p_cfg_cpbs,
                           p_link->sig_chann.cpbs_len);
                    mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                          p_link->sig_chann.cid, rsp_len, false);
                }

                p_avdtp->cback(cid, AVDTP_MSG_SIG_GET_CFG_IND, NULL);
            }
            break;

        case AVDTP_RECFG:
            {
                if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
                {
                    uint8_t result;
                    T_AVDTP_LOCAL_SEP  *sep;

                    sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
                    while (sep != NULL)
                    {
                        if (sep->sep_id == acp_seid)
                        {
                            break;
                        }

                        sep = sep->next;
                    }

                    if (sep == NULL)
                    {
                        return;
                    }

                    p_link->sig_chann.enable_cp_flag = 0;

                    result = avdtp_capability_check(p_link, (uint8_t *)(p_signal_pkt + 3), (avdtp_pkt_len - 3), sep);

                    if (result != 0)
                    {
                        rsp_len = sizeof(T_AVDTP_SIG_HDR) + 2;
                        p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, cid, 0, rsp_len,
                                                       p_avdtp->data_offset, false);

                        if (p_rsp_packet != NULL)
                        {
                            index = p_avdtp->data_offset;
                            p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                                     AVDTP_MSG_TYPE_RSP_REJECT);
                            p_rsp_packet[index++] = AVDTP_RECFG;
                            p_rsp_packet[index++] = SRV_CATEG_MEDIA_CODEC;
                            p_rsp_packet[index++] = result;
                            mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset, cid, rsp_len, false);
                        }
                    }
                    else
                    {
                        uint8_t ind_data[4 + MAX_CODEC_INFO_SIZE];

                        if (p_link->sig_chann.p_cfg_cpbs != NULL)
                        {
                            free(p_link->sig_chann.p_cfg_cpbs);
                        }
                        p_link->sig_chann.cpbs_len = avdtp_pkt_len - 3;
                        p_link->sig_chann.p_cfg_cpbs = malloc((avdtp_pkt_len - 3));
                        if (p_link->sig_chann.p_cfg_cpbs != NULL)
                        {
                            memcpy(p_link->sig_chann.p_cfg_cpbs, (p_signal_pkt + 3), (avdtp_pkt_len - 3));

                            rsp_len = sizeof(T_AVDTP_SIG_HDR);
                            p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                                           p_avdtp->data_offset, false);
                            if (p_rsp_packet != NULL)
                            {
                                index = p_avdtp->data_offset;
                                p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                                         AVDTP_MSG_TYPE_RSP_ACCEPT);
                                p_rsp_packet[index++] = AVDTP_RECFG;
                                mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                                      p_link->sig_chann.cid, rsp_len, false);
                            }

                            p_link->role = sep->tsep;

                            ind_data[0] = p_link->role;
                            ind_data[1] = p_link->sig_chann.codec_type;
                            ind_data[2] = p_link->sig_chann.enable_cp_flag;
                            ind_data[3] = p_link->sig_chann.delay_report_flag;
                            memcpy(&(ind_data[4]), &(p_link->sig_chann.codec_info[0]), MAX_CODEC_INFO_SIZE);
                            p_avdtp->cback(cid, AVDTP_MSG_SIG_RECFG_IND, &(ind_data[0]));
                        }
                    }
                }
                else
                {
                    avdtp_cmd_bad_state_proc(p_link->bd_addr);
                }
            }
            break;

        case AVDTP_OPEN:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG)
            {
                rsp_len = sizeof(T_AVDTP_SIG_HDR);
                p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                               p_avdtp->data_offset, false);
                if (p_rsp_packet != NULL)
                {
                    index = p_avdtp->data_offset;
                    p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                             AVDTP_MSG_TYPE_RSP_ACCEPT);
                    p_rsp_packet[index++] = AVDTP_OPEN;
                    mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                          p_link->sig_chann.cid, rsp_len, false);
                }

                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_OPENING;
                p_avdtp->cback(cid, AVDTP_MSG_SIG_OPEN_IND, NULL);
            }
            else
            {
                avdtp_cmd_bad_state_proc(p_link->bd_addr);
            }
            break;

        case AVDTP_START:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
            {
                p_link->sig_chann.rx_start_trans_label = trans_label;
                p_avdtp->cback(cid, AVDTP_MSG_SIG_START_IND, NULL);
            }
            else
            {
                avdtp_cmd_bad_state_proc(p_link->bd_addr);
            }
            break;

        case AVDTP_CLOSE:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING ||
                p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN ||
                p_link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                rsp_len = sizeof(T_AVDTP_SIG_HDR);
                p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                               p_avdtp->data_offset, false);
                if (p_rsp_packet != NULL)
                {
                    index = p_avdtp->data_offset;
                    p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                             AVDTP_MSG_TYPE_RSP_ACCEPT);
                    p_rsp_packet[index++] = AVDTP_CLOSE;
                    mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                          p_link->sig_chann.cid, rsp_len, false);
                }
                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                p_avdtp->cback(cid, AVDTP_MSG_SIG_CLOSE_IND, NULL);
            }
            else
            {
                avdtp_cmd_bad_state_proc(p_link->bd_addr);
            }
            break;

        case AVDTP_SUSPEND:
            rsp_len = sizeof(T_AVDTP_SIG_HDR);
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                               p_avdtp->data_offset, false);
                if (p_rsp_packet != NULL)
                {
                    index = p_avdtp->data_offset;
                    p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                             AVDTP_MSG_TYPE_RSP_ACCEPT);
                    p_rsp_packet[index++] = AVDTP_SUSPEND;
                    mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                          p_link->sig_chann.cid, rsp_len, false);
                }

                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_OPEN;
                p_avdtp->cback(cid, AVDTP_MSG_SIG_SUSPEND_IND, NULL);
            }
            else
            {
                avdtp_cmd_bad_state_proc(p_link->bd_addr);
            }
            break;

        case AVDTP_ABORT:
            {
                rsp_len = sizeof(T_AVDTP_SIG_HDR);
                p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                               p_avdtp->data_offset, false);
                if (p_rsp_packet != NULL)
                {
                    index = p_avdtp->data_offset;
                    p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                             AVDTP_MSG_TYPE_RSP_ACCEPT);
                    p_rsp_packet[index++] = AVDTP_ABORT;
                    mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                          p_link->sig_chann.cid, rsp_len, false);
                }

                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                p_avdtp->cback(cid, AVDTP_MSG_SIG_ABORT_IND, NULL);
            }
            break;

        case AVDTP_SECURITY_CONTROL:
            rsp_len = sizeof(T_AVDTP_SIG_HDR) + (avdtp_pkt_len - 3);
            p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, cid, 0, rsp_len,
                                           p_avdtp->data_offset, false);
            if (p_rsp_packet != NULL)
            {
                index = p_avdtp->data_offset;
                p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                         AVDTP_MSG_TYPE_RSP_ACCEPT);
                p_rsp_packet[index++] = signal_id;
                if ((avdtp_pkt_len - 3) > 0)
                {
                    memcpy(&p_rsp_packet[index], &p_signal_pkt[3], (avdtp_pkt_len - 3));
                }
                mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset, cid, rsp_len, false);
            }

            p_avdtp->cback(cid, AVDTP_MSG_SIG_SECURITY_CONTROL_IND, NULL);
            break;

        case AVDTP_DELAY_REPORT:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG ||
                p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING ||
                p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN ||
                p_link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                rsp_len = sizeof(T_AVDTP_SIG_HDR);
                p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, p_link->sig_chann.cid, 0, rsp_len,
                                               p_avdtp->data_offset, false);
                if (p_rsp_packet != NULL)
                {
                    index = p_avdtp->data_offset;
                    p_rsp_packet[index++] = trans_label | AVDTP_PKT_TYPE_SINGLE << 2 | AVDTP_MSG_TYPE_RSP_ACCEPT;
                    p_rsp_packet[index++] = signal_id;
                    mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset,
                                          p_link->sig_chann.cid, rsp_len, false);
                }

                p_avdtp->cback(cid, AVDTP_MSG_SIG_DELAY_REPORT_IND, NULL);
            }
            else
            {
                avdtp_cmd_bad_state_proc(p_link->bd_addr);
            }
            break;

        default:
            rsp_len = sizeof(T_AVDTP_SIG_HDR); /* ACP SEID and error code */
            p_rsp_packet = mpa_get_l2c_buf(p_avdtp->queue_id, cid, 0, rsp_len,
                                           p_avdtp->data_offset, false);
            if (p_rsp_packet != NULL)
            {
                index = p_avdtp->data_offset;
                p_rsp_packet[index++] = (trans_label | AVDTP_PKT_TYPE_SINGLE << 2 |
                                         AVDTP_MSG_TYPE_GENERAL_REJECT);
                p_rsp_packet[index++] = signal_id;
                mpa_send_l2c_data_req(p_rsp_packet, p_avdtp->data_offset, cid, rsp_len, false);
            }
            break;
        }
    }
}

void avdtp_rx_rsp_proc(T_AVDTP_LINK *p_link)
{
    uint8_t signal_id;
    uint8_t *p_signal_pkt;
    uint16_t avdtp_pkt_len;
    uint8_t temp;

    p_signal_pkt = p_link->sig_chann.p_fragment_data;
    avdtp_pkt_len = p_link->sig_chann.fragment_data_len;
    signal_id = ((*(p_signal_pkt + 1)) & 0x3F);

    PROTOCOL_PRINT_INFO4("avdtp_rx_rsp_proc: bd_addr %s, signal_id 0x%02x, msg_type 0x%02x, cmd_flag 0x%02x",
                         TRACE_BDADDR(p_link->bd_addr), signal_id, (*p_signal_pkt & 0x03),
                         p_link->sig_chann.cmd_flag);

    if (signal_id != AVDTP_DELAY_REPORT)
    {
        if (p_link->sig_chann.cmd_flag != signal_id)
        {
            return;
        }
        p_link->sig_chann.cmd_flag = 0;
    }
    sys_timer_stop(p_link->timer_handle);

    if ((*p_signal_pkt & 0x03) == AVDTP_MSG_TYPE_RSP_ACCEPT)
    {
        switch (signal_id)
        {
        case AVDTP_DISCOVER:
            {
                uint8_t acp_seid_status;
                uint8_t sep_type;
                uint8_t i;

                p_link->sig_chann.acp_sep_no = 0;
                if (avdtp_pkt_len >= 4)
                {
                    temp = ((avdtp_pkt_len - 2) >> 1);
                    for (i = 0; i < temp; i++)
                    {
                        acp_seid_status = ((*(p_signal_pkt + 2 + (i << 1))) & 0x02);
                        sep_type = ((*(p_signal_pkt + 3 + (i << 1))) & 0xF8);
                        if (acp_seid_status == SEID_NOT_IN_USE)
                        {
                            acp_seid_status = ((*(p_signal_pkt + 2 + (i << 1))) & 0x02);
                            sep_type = ((*(p_signal_pkt + 3 + (i << 1))) & 0xF8);

                            if (acp_seid_status == SEID_NOT_IN_USE)
                            {
                                if (p_link->conn_role == AVDTP_TSEP_SRC)
                                {
                                    if (sep_type == AUDIO_SRC)
                                    {
                                        p_link->sig_chann.acp_sep_no++;
                                    }
                                }
                                else if (p_link->conn_role == AVDTP_TSEP_SNK)
                                {
                                    if (sep_type == AUDIO_SNK)
                                    {
                                        p_link->sig_chann.acp_sep_no++;
                                    }
                                }
                            }
                        }
                    }
                }

                if (p_link->sig_chann.acp_sep_no)
                {
                    uint8_t j = 0;

                    if (p_link->sig_chann.avdtp_sep != NULL)
                    {
                        free(p_link->sig_chann.avdtp_sep);
                    }
                    temp = (p_link->sig_chann.acp_sep_no * sizeof(T_AVDTP_REMOTE_SEP));
                    p_link->sig_chann.avdtp_sep = malloc(temp);
                    if (p_link->sig_chann.avdtp_sep == NULL)
                    {
                        return;
                    }

                    temp = ((avdtp_pkt_len - 2) >> 1);
                    for (i = 0; i < temp; i++)
                    {
                        acp_seid_status = ((*(p_signal_pkt + 2 + (i << 1))) & 0x02);
                        sep_type = ((*(p_signal_pkt + 3 + (i << 1))) & 0xF8);

                        if (acp_seid_status == SEID_NOT_IN_USE)
                        {
                            if (p_link->conn_role == AVDTP_TSEP_SRC)
                            {
                                if (sep_type == AUDIO_SRC)
                                {
                                    p_link->sig_chann.avdtp_sep[j].sep_id =
                                        ((*(p_signal_pkt + 2 + (i << 1))) & 0xFC);
                                    j++;
                                    if (j >= p_link->sig_chann.acp_sep_no)
                                    {
                                        break;
                                    }
                                }
                            }
                            else if (p_link->conn_role == AVDTP_TSEP_SNK)
                            {
                                if (sep_type == AUDIO_SNK)
                                {
                                    p_link->sig_chann.avdtp_sep[j].sep_id =
                                        ((*(p_signal_pkt + 2 + (i << 1))) & 0xFC);
                                    j++;
                                    if (j >= p_link->sig_chann.acp_sep_no)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    p_link->sig_chann.acp_seid_idx = 0;
                    avdtp_signal_get_capability_req(p_link->bd_addr);
                    p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_DISCOVER_RSP, NULL);
                }
                else
                {
                    avdtp_signal_disconnect_req(p_link->bd_addr);
                }
            }
            break;

        case AVDTP_GET_CPBS:
        case AVDTP_GET_ALL_CPBS:
            {
                avdtp_remote_capability_parse(p_link, (uint8_t *)(p_signal_pkt + 2), (avdtp_pkt_len - 2),
                                              p_link->sig_chann.acp_seid_idx);
                p_link->sig_chann.acp_seid_idx++;
                if (p_link->sig_chann.acp_seid_idx == p_link->sig_chann.acp_sep_no)
                {
                    uint8_t    i;

                    for (i = 0; i < p_link->sig_chann.acp_sep_no; i++)
                    {
                        if (p_link->sig_chann.avdtp_sep[i].media_codec_type == VENDOR_CODEC_TYPE)
                        {
                            if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LDAC)
                            {
                                if (avdtp_vendor_codec_ldac_handler(HDL_TYPE_CHECK_CODEC,
                                                                    NULL,
                                                                    p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                    NULL,
                                                                    p_link->conn_role) == true)
                                {
                                    break;
                                }
                            }
                            else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LC3)
                            {
                                if (avdtp_vendor_codec_lc3_handler(HDL_TYPE_CHECK_CODEC,
                                                                   NULL,
                                                                   p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                   NULL,
                                                                   p_link->conn_role) == true)
                                {
                                    break;
                                }
                            }
                            else if (p_link->sig_chann.avdtp_sep[i].media_codec_subtype == CODEC_TYPE_LHDC)
                            {
                                if (avdtp_vendor_codec_lhdc_handler(HDL_TYPE_CHECK_CODEC,
                                                                    NULL,
                                                                    p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                                    NULL,
                                                                    p_link->conn_role) == true)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    if (i == p_link->sig_chann.acp_sep_no)
                    {
                        for (i = 0; i < p_link->sig_chann.acp_sep_no; i++)
                        {
                            if (p_link->sig_chann.avdtp_sep[i].media_codec_type == USAC_MEDIA_CODEC_TYPE)
                            {
                                if (avdtp_codec_usac_handler(HDL_TYPE_CHECK_CODEC,
                                                             NULL,
                                                             p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                             NULL,
                                                             p_link->conn_role) == true)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    if (i == p_link->sig_chann.acp_sep_no)
                    {
                        for (i = 0; i < p_link->sig_chann.acp_sep_no; i++)
                        {
                            if (p_link->sig_chann.avdtp_sep[i].media_codec_type == AAC_MEDIA_CODEC_TYPE)
                            {
                                if (avdtp_codec_aac_handler(HDL_TYPE_CHECK_CODEC,
                                                            NULL,
                                                            p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                            NULL,
                                                            p_link->conn_role) == true)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    if (i == p_link->sig_chann.acp_sep_no)
                    {
                        for (i = 0; i < p_link->sig_chann.acp_sep_no; i++)
                        {
                            if (p_link->sig_chann.avdtp_sep[i].media_codec_type == SBC_MEDIA_CODEC_TYPE)
                            {
                                if (avdtp_codec_sbc_handler(HDL_TYPE_CHECK_CODEC,
                                                            NULL,
                                                            p_link->sig_chann.avdtp_sep[i].media_codec_info,
                                                            NULL,
                                                            p_link->conn_role) == true)
                                {
                                    break;
                                }
                            }
                        }
                    }

                    if (i < p_link->sig_chann.acp_sep_no)
                    {
                        p_link->sig_chann.acp_seid_idx = i;
                        p_link->sig_chann.acp_seid = p_link->sig_chann.avdtp_sep[i].sep_id;

                        if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_IDLE)
                        {
                            avdtp_signal_cfg_req(p_link->bd_addr);
                        }
                        else if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
                        {
                            avdtp_signal_recfg_req(p_link->bd_addr, p_link->sig_chann.recfg_codec_type, p_link->conn_role);
                        }
                    }
                    else
                    {
                        p_link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                        p_link->sig_chann.int_flag = 0;

                        avdtp_signal_disconnect_req(p_link->bd_addr);
                    }
                }
                else
                {
                    avdtp_signal_get_capability_req(p_link->bd_addr);
                }

                p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_GET_CPBS_RSP, NULL);
            }
            break;

        case AVDTP_GET_CFG:
            p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_GET_CFG_RSP, NULL);
            break;

        case AVDTP_RECFG:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
            {
                uint8_t     ind_data[4 + MAX_CODEC_INFO_SIZE];

                ind_data[0] = p_link->role;
                ind_data[1] = p_link->sig_chann.codec_type;
                ind_data[2] = p_link->sig_chann.enable_cp_flag;
                ind_data[3] = p_link->sig_chann.delay_report_flag;
                memcpy(&(ind_data[4]), &(p_link->sig_chann.codec_info[0]), MAX_CODEC_INFO_SIZE);
                p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_RECFG_RSP, &ind_data[0]);

                avdtp_signal_delay_report_req(p_link->bd_addr, p_avdtp->stream_latency);
            }
            break;

        case AVDTP_SET_CFG:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_IDLE)
            {
                uint8_t     ind_data[4 + MAX_CODEC_INFO_SIZE];

                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;

                ind_data[0] = p_link->role;
                ind_data[1] = p_link->sig_chann.codec_type;
                ind_data[2] = p_link->sig_chann.enable_cp_flag;
                ind_data[3] = p_link->sig_chann.delay_report_flag;
                memcpy(&(ind_data[4]), &(p_link->sig_chann.codec_info[0]), MAX_CODEC_INFO_SIZE);
                p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_SET_CFG_RSP, &(ind_data[0]));

                avdtp_signal_delay_report_req(p_link->bd_addr, p_avdtp->stream_latency);
                avdtp_signal_open_req(p_link->bd_addr, p_link->conn_role);
            }
            break;

        case AVDTP_OPEN:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING)
            {
                p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_OPEN_RSP, NULL);
            }
            break;

        case AVDTP_START:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN)
            {
                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_STREAMING;
                p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_START_RSP, NULL);
            }
            break;

        case AVDTP_CLOSE:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN ||
                p_link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                p_link->sig_chann.int_flag = 0;
                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_CLOSE_RSP, NULL);
            }
            break;

        case AVDTP_ABORT:
            {
                p_link->sig_chann.int_flag = 0;
                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_ABORT_RSP, NULL);
            }
            break;

        case AVDTP_SUSPEND:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_OPEN;
                p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_SUSPEND_RSP, NULL);
            }
            break;

        case AVDTP_DELAY_REPORT:
            if (p_link->sig_chann.sig_state == AVDTP_SIG_STATE_CFG ||
                p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING ||
                p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPEN ||
                p_link->sig_chann.sig_state == AVDTP_SIG_STATE_STREAMING)
            {
                p_avdtp->cback(p_link->sig_chann.cid, AVDTP_MSG_SIG_DELAY_REPORT_RSP, NULL);
            }
            break;

        default:
            break;
        }
    }
    else if ((*p_signal_pkt & 0x03) == AVDTP_MSG_TYPE_RSP_REJECT)
    {
        switch (signal_id)
        {
        case AVDTP_OPEN:
            {
                avdtp_signal_abort_req(p_link->bd_addr);
            }
            break;

        case AVDTP_GET_CPBS:
        case AVDTP_GET_ALL_CPBS:
            {
                p_link->sig_chann.acp_seid_idx++;
                if (p_link->sig_chann.acp_seid_idx < p_link->sig_chann.acp_sep_no)
                {
                    avdtp_signal_get_capability_req(p_link->bd_addr);
                }
            }
            break;

        case AVDTP_CLOSE:
            {
                avdtp_signal_abort_req(p_link->bd_addr);
            }
            break;

        case AVDTP_ABORT:
            {
                if (p_link->strm_chann.state == AVDTP_STATE_CONNECTED)
                {
                    avdtp_stream_disconnect_req(p_link->bd_addr);
                }

                avdtp_signal_disconnect_req(p_link->bd_addr);
            }
            break;

        case AVDTP_RECFG:
            {
                avdtp_signal_disconnect_req(p_link->bd_addr);
            }
            break;

        case AVDTP_SET_CFG:
            {
                avdtp_signal_disconnect_req(p_link->bd_addr);
            }
            break;

        default:
            break;
        }
    }
}

void avdtp_signal_proc(T_AVDTP_LINK *p_link)
{
    uint8_t *p_avdtp_packet;
    uint8_t message_type;

    p_avdtp_packet = p_link->sig_chann.p_fragment_data;
    message_type = (*(p_avdtp_packet) & 0x03);
    if (message_type == AVDTP_MSG_TYPE_CMD)
    {
        avdtp_rx_cmd_proc(p_link);
    }
    else
    {
        avdtp_rx_rsp_proc(p_link);
    }
}

void avdtp_handle_signal_ind(T_AVDTP_LINK       *p_link,
                             T_MPA_L2C_DATA_IND *p_data_ind)
{
    uint8_t        *p_avdtp_packet;
    uint16_t        avdtp_pkt_len;
    uint8_t         packet_type;

    p_avdtp_packet  = p_data_ind->data + p_data_ind->gap;
    avdtp_pkt_len = p_data_ind->length;

    packet_type = (((*p_avdtp_packet) & AVDTP_PKT_TYPE_MASK) >> 2);

    switch (packet_type)
    {
    case AVDTP_PKT_TYPE_SINGLE:
        if (p_link->sig_chann.p_fragment_data != NULL)
        {
            free(p_link->sig_chann.p_fragment_data);
        }
        p_link->sig_chann.p_fragment_data = p_avdtp_packet;
        p_link->sig_chann.fragment_data_len = avdtp_pkt_len;
        avdtp_signal_proc(p_link);
        p_link->sig_chann.p_fragment_data = NULL;
        break;

    case AVDTP_PKT_TYPE_START:
        {
            if (avdtp_pkt_len > sizeof(T_AVDTP_START_PKT_HDR))
            {
                T_AVDTP_START_PKT    *p_start_packet;
                uint8_t                 packet_num;
                uint16_t                data_len;
                uint16_t                total_len;

                p_start_packet = (T_AVDTP_START_PKT *)p_avdtp_packet;
                packet_num = p_start_packet->packet_header.packet_num;
                data_len = avdtp_pkt_len - sizeof(T_AVDTP_START_PKT_HDR);
                total_len = data_len * packet_num;
                if (p_link->sig_chann.p_fragment_data != NULL)
                {
                    free(p_link->sig_chann.p_fragment_data);
                }
                p_link->sig_chann.p_fragment_data = malloc((total_len + 2));
                if (p_link->sig_chann.p_fragment_data == NULL)
                {
                    return;
                }
                p_link->sig_chann.p_fragment_data[0] = p_start_packet->packet_header.trans_info;
                p_link->sig_chann.p_fragment_data[1] = p_start_packet->packet_header.signal_id;
                memcpy(&(p_link->sig_chann.p_fragment_data[2]),
                       (uint8_t *)(&p_start_packet->packet_data[0]), data_len);
                p_link->sig_chann.fragment_data_len = data_len + 2;
            }
        }
        break;

    case AVDTP_PKT_TYPE_CONTINUE:
    case AVDTP_PKT_TYPE_END:
        {
            T_AVDTP_CONTINUE_PKT   *p_cont_end_packet;

            if (p_link->sig_chann.p_fragment_data == NULL)
            {
                return;
            }

            p_cont_end_packet = (T_AVDTP_CONTINUE_PKT *)p_avdtp_packet;
            if ((uint8_t)(p_cont_end_packet->packet_header.trans_info & 0xF0) ==
                (p_link->sig_chann.p_fragment_data[0] & 0xF0))
            {
                uint16_t                     data_len;

                data_len = avdtp_pkt_len - sizeof(T_AVDTP_CONTINUE_PKT_HDR);
                memcpy(&(p_link->sig_chann.p_fragment_data[(p_link->sig_chann.fragment_data_len)]),
                       (uint8_t *)(&(p_cont_end_packet->packet_data[0])), data_len);
                p_link->sig_chann.fragment_data_len += data_len;
            }

            if (packet_type == AVDTP_PKT_TYPE_END)
            {
                avdtp_signal_proc(p_link);
                free(p_link->sig_chann.p_fragment_data);
                p_link->sig_chann.p_fragment_data = NULL;
            }
        }
        break;
    }
}

void avdtp_handle_data_ind(T_MPA_L2C_DATA_IND *p_data_ind)
{
    T_AVDTP_LINK *p_link;

    p_link = avdtp_find_link_by_cid(p_data_ind->cid);
    if (p_link != NULL)
    {
        if (p_link->sig_chann.cid == p_data_ind->cid)
        {
            avdtp_handle_signal_ind(p_link, p_data_ind);
        }
        else
        {
            p_avdtp->cback(p_data_ind->cid, AVDTP_MSG_STREAM_DATA_IND, p_data_ind);
        }
    }
}

void avdtp_handle_data_rsp(T_MPA_L2C_DATA_RSP *p_data_rsp)
{
    T_AVDTP_LINK *p_link;

    p_link = avdtp_find_link_by_cid(p_data_rsp->cid);
    if (p_link != NULL)
    {
        p_avdtp->cback(p_data_rsp->cid, AVDTP_MSG_STREAM_DATA_RSP, p_link->bd_addr);
    }
}

void avdtp_handle_conn_req(T_MPA_L2C_CONN_IND *p_conn_ind)
{
    T_AVDTP_LINK   *p_link;
    T_MPA_L2C_CONN_CFM_CAUSE rsp;

    PROTOCOL_PRINT_TRACE2("avdtp_handle_conn_req: bd_addr %s, cid 0x%04x",
                          TRACE_BDADDR(p_conn_ind->bd_addr), p_conn_ind->cid);

    p_link = avdtp_find_link_by_addr(p_conn_ind->bd_addr);
    if (p_link == NULL)
    {
        p_link = avdtp_alloc_link(p_conn_ind->bd_addr);
        if (p_link != NULL)
        {
            p_link->sig_chann.cid = p_conn_ind->cid;
            p_link->sig_chann.state = AVDTP_STATE_CONNECTING;
            p_avdtp->cback(p_conn_ind->cid, AVDTP_MSG_SIG_CONN_REQ, p_conn_ind->bd_addr);
        }
        else
        {
            mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, p_conn_ind->cid, AVDTP_SIG_MTU,
                                  MPA_L2C_MODE_BASIC, 0xFFFF);
        }
    }
    else
    {
        if (p_link->sig_chann.state == AVDTP_STATE_CONNECTED &&
            p_link->sig_chann.sig_state == AVDTP_SIG_STATE_OPENING &&
            p_link->strm_chann.state == AVDTP_STATE_DISCONNECTED)
        {
            rsp = MPA_L2C_CONN_ACCEPT;

            p_link->strm_chann.cid = p_conn_ind->cid;
            p_link->strm_chann.state = AVDTP_STATE_CONNECTING;
            p_avdtp->cback(p_conn_ind->cid, AVDTP_MSG_STREAM_CONN_REQ, p_conn_ind->bd_addr);
        }
        else
        {
            rsp = MPA_L2C_CONN_NO_RESOURCE;

            p_link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;
        }

        if (p_link->sig_chann.codec_type != VENDOR_CODEC_TYPE)
        {
            mpa_send_l2c_conn_cfm(rsp, p_conn_ind->cid, AVDTP_STREAM_MTU, MPA_L2C_MODE_BASIC, 0xFFFF);
        }
        else
        {
            mpa_send_l2c_conn_cfm(rsp, p_conn_ind->cid, AVDTP_LDAC_STREAM_MTU, MPA_L2C_MODE_BASIC, 0xFFFF);
        }
    }
}

bool avdtp_signal_connect_cfm(uint8_t bd_addr[6],
                              bool    accept)
{
    T_AVDTP_LINK *p_link;
    T_MPA_L2C_CONN_CFM_CAUSE rsp;

    rsp = MPA_L2C_CONN_ACCEPT;
    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        uint16_t cid;

        cid = p_link->sig_chann.cid;

        if (accept == false)
        {
            avdtp_free_link(p_link);
            rsp = MPA_L2C_CONN_NO_RESOURCE;
        }

        mpa_send_l2c_conn_cfm(rsp, cid, AVDTP_SIG_MTU, MPA_L2C_MODE_BASIC, 0xFFFF);
        return true;
    }

    return false;
}

void avdtp_handle_conn_rsp(T_MPA_L2C_CONN_RSP *p_conn_rsp)
{
    T_AVDTP_LINK *p_link;

    PROTOCOL_PRINT_TRACE2("avdtp_handle_conn_rsp: cid 0x%04x, status 0x%04x",
                          p_conn_rsp->cid, p_conn_rsp->cause);

    p_link = avdtp_find_link_by_addr(p_conn_rsp->bd_addr);
    if (p_link != NULL)
    {
        if (p_conn_rsp->cause == 0)
        {
            if (p_link->sig_chann.state == AVDTP_STATE_CONNECTING)
            {
                p_link->sig_chann.cid = p_conn_rsp->cid;
                p_avdtp->cback(p_conn_rsp->cid, AVDTP_MSG_SIG_CONN_RSP, p_conn_rsp->bd_addr);
            }
            else if (p_link->strm_chann.state == AVDTP_STATE_CONNECTING)
            {
                p_link->strm_chann.cid = p_conn_rsp->cid;
                p_avdtp->cback(p_conn_rsp->cid, AVDTP_MSG_STREAM_CONN_RSP, p_conn_rsp->bd_addr);
            }
        }
        else
        {
            if (p_link->sig_chann.state == AVDTP_STATE_CONNECTING)
            {
                uint16_t info;
                info = p_conn_rsp->cause;
                avdtp_free_link(p_link);
                p_avdtp->cback(p_conn_rsp->cid, AVDTP_MSG_SIG_CONN_FAIL, &info);
            }
            else if (p_link->strm_chann.state == AVDTP_STATE_CONNECTING)
            {
                uint16_t info = p_conn_rsp->cause;

                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;
                p_link->strm_chann.state = AVDTP_STATE_DISCONNECTED;
                p_avdtp->cback(p_conn_rsp->cid, AVDTP_MSG_STREAM_CONN_FAIL, &info);
            }
        }
    }
}

void avdtp_handle_conn_cmpl(T_MPA_L2C_CONN_CMPL_INFO *p_conn_cmpl)
{
    T_AVDTP_LINK *p_link;

    PROTOCOL_PRINT_TRACE2("avdtp_handle_conn_cmpl: cid 0x%04x, status 0x%04x",
                          p_conn_cmpl->cid, p_conn_cmpl->cause);

    p_link = avdtp_find_link_by_cid(p_conn_cmpl->cid);
    if (p_link != NULL)
    {
        if (p_conn_cmpl->cause)
        {
            uint16_t info = p_conn_cmpl->cause;

            if (p_link->sig_chann.state == AVDTP_STATE_CONNECTING)
            {
                avdtp_free_link(p_link);
                p_avdtp->cback(p_conn_cmpl->cid, AVDTP_MSG_SIG_CONN_FAIL, &info);
            }
            else if (p_link->strm_chann.state == AVDTP_STATE_CONNECTING)
            {
                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_CFG;
                p_link->strm_chann.state = AVDTP_STATE_DISCONNECTED;
                p_avdtp->cback(p_conn_cmpl->cid, AVDTP_MSG_STREAM_CONN_FAIL, &info);
            }
        }
        else
        {
            if (p_link->sig_chann.cid == p_conn_cmpl->cid)
            {
                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_IDLE;
                p_link->sig_chann.state = AVDTP_STATE_CONNECTED;
                p_link->sig_chann.remote_mtu = p_conn_cmpl->remote_mtu;
                p_avdtp->data_offset = p_conn_cmpl->ds_data_offset;
                p_avdtp->cback(p_conn_cmpl->cid, AVDTP_MSG_SIG_CONNECTED, p_conn_cmpl);
            }
            else
            {
                p_link->sig_chann.sig_state = AVDTP_SIG_STATE_OPEN;
                p_link->strm_chann.state = AVDTP_STATE_CONNECTED;
                p_avdtp->cback(p_conn_cmpl->cid, AVDTP_MSG_STREAM_CONNECTED, p_conn_cmpl);
            }
        }
    }
    else
    {
        mpa_send_l2c_disconn_req(p_conn_cmpl->cid);
    }
}

void avdtp_handle_disconn_ind(T_MPA_L2C_DISCONN_IND *p_disconn_ind)
{
    T_AVDTP_LINK *p_link;

    PROTOCOL_PRINT_TRACE1("avdtp_handle_disconn_ind: cid 0x%04x", p_disconn_ind->cid);

    p_link = avdtp_find_link_by_cid(p_disconn_ind->cid);
    if (p_link != NULL)
    {
        if (p_link->sig_chann.cid == p_disconn_ind->cid)
        {
            if (p_link->strm_chann.state == AVDTP_STATE_CONNECTED)
            {
                mpa_send_l2c_disconn_req(p_link->strm_chann.cid);
                p_avdtp->cback(p_link->strm_chann.cid, AVDTP_MSG_STREAM_DISCONNECTED, &p_disconn_ind->cause);
            }

            avdtp_free_link(p_link);
            p_avdtp->cback(p_disconn_ind->cid, AVDTP_MSG_SIG_DISCONNECTED, &p_disconn_ind->cause);
        }
        else
        {
            p_link->strm_chann.state = AVDTP_STATE_DISCONNECTED;
            p_link->strm_chann.cid = 0;
            p_avdtp->cback(p_disconn_ind->cid, AVDTP_MSG_STREAM_DISCONNECTED, &p_disconn_ind->cause);
        }
    }

    mpa_send_l2c_disconn_cfm(p_disconn_ind->cid);
}

void avdtp_handle_disconn_rsp(T_MPA_L2C_DISCONN_RSP *p_disconn_rsp)
{
    T_AVDTP_LINK *p_link;

    PROTOCOL_PRINT_TRACE1("avdtp_handle_disconn_rsp: cid 0x%04x", p_disconn_rsp->cid);

    p_link = avdtp_find_link_by_cid(p_disconn_rsp->cid);
    if (p_link != NULL)
    {
        if (p_link->sig_chann.cid == p_disconn_rsp->cid)
        {
            avdtp_free_link(p_link);
            p_avdtp->cback(p_disconn_rsp->cid, AVDTP_MSG_SIG_DISCONNECTED, &p_disconn_rsp->cause);
        }
        else
        {
            p_link->strm_chann.state = AVDTP_STATE_DISCONNECTED;
            p_link->strm_chann.cid = 0;
            p_avdtp->cback(p_disconn_rsp->cid, AVDTP_MSG_STREAM_DISCONNECTED, &p_disconn_rsp->cause);
        }
    }
}

void avdtp_l2c_cback(void        *p_buf,
                     T_PROTO_MSG  msg)
{
    switch (msg)
    {
    case L2C_CONN_IND:
        avdtp_handle_conn_req((T_MPA_L2C_CONN_IND *)p_buf);
        break;

    case L2C_CONN_RSP:
        avdtp_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)p_buf);
        break;

    case L2C_CONN_CMPL:
        avdtp_handle_conn_cmpl((T_MPA_L2C_CONN_CMPL_INFO *)p_buf);
        break;

    case L2C_DATA_IND:
        avdtp_handle_data_ind((T_MPA_L2C_DATA_IND *)p_buf);
        break;

    case L2C_DATA_RSP:
        avdtp_handle_data_rsp((T_MPA_L2C_DATA_RSP *)p_buf);
        break;

    case L2C_DISCONN_IND:
        avdtp_handle_disconn_ind((T_MPA_L2C_DISCONN_IND *)p_buf);
        break;

    case L2C_DISCONN_RSP:
        avdtp_handle_disconn_rsp((T_MPA_L2C_DISCONN_RSP *)p_buf);
        break;

    default:
        break;
    }
}

bool avdtp_codec_add(uint8_t            role,
                     T_A2DP_CODEC_TYPE  codec_type,
                     uint8_t           *media_codec_info)
{
    uint8_t            i;
    T_AVDTP_LOCAL_SEP *sep;

    for (i = 0; i < p_avdtp->link_num; i++)
    {
        if (p_avdtp->link[i].sig_chann.sig_state != AVDTP_SIG_STATE_IDLE)
        {
            return false;
        }
    }

    sep = calloc(1, sizeof(T_AVDTP_LOCAL_SEP));
    if (sep == NULL)
    {
        return false;
    }

    switch (codec_type)
    {
    case A2DP_CODEC_TYPE_SBC:
        {
            sep->codec_type = SBC_MEDIA_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_SBC;
        }
        break;

    case A2DP_CODEC_TYPE_AAC:
        {
            sep->codec_type = AAC_MEDIA_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_AAC;
        }
        break;

    case A2DP_CODEC_TYPE_USAC:
        {
            sep->codec_type = USAC_MEDIA_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_USAC;
        }
        break;

    case A2DP_CODEC_TYPE_LDAC:
        {
            sep->codec_type = VENDOR_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_LDAC;
        }
        break;

    case A2DP_CODEC_TYPE_LC3:
        {
            sep->codec_type = VENDOR_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_LC3;
        }
        break;

    case A2DP_CODEC_TYPE_LHDC:
        {
            sep->codec_type = VENDOR_CODEC_TYPE;
            sep->sub_codec_type = CODEC_TYPE_LHDC;
        }
        break;

    default:
        {
            free(sep);
        }
        return false;
    }

    sep->tsep = role;
    sep->media_type = AVDTP_MEDIA_AUDIO;
    sep->in_use = false;
    memcpy(sep->media_codec_info, media_codec_info, MAX_CODEC_INFO_SIZE);
    p_avdtp->local_sep_num++;
    sep->sep_id = p_avdtp->local_sep_num;
    os_queue_in(&p_avdtp->local_sep_list, sep);

    return true;
}

bool avdtp_codec_delete(uint8_t            role,
                        T_A2DP_CODEC_TYPE  codec_type)
{
    uint8_t            j;
    uint8_t            i = 0;
    T_AVDTP_LOCAL_SEP *sep;
    bool               ret = false;

    for (j = 0; j < p_avdtp->link_num; j++)
    {
        if (p_avdtp->link[j].sig_chann.sig_state != AVDTP_SIG_STATE_IDLE)
        {
            return false;
        }
    }

    sep = os_queue_peek(&p_avdtp->local_sep_list, 0);
    while (sep != NULL)
    {
        if ((sep->tsep == role) && (sep->sub_codec_type == codec_type))
        {
            os_queue_delete(&p_avdtp->local_sep_list, sep);
            free(sep);
            p_avdtp->local_sep_num--;
            ret = true;
        }
        else
        {
            sep->sep_id = ++i;
        }

        sep = sep->next;
    }

    return ret;
}

bool avdtp_init(uint8_t       link_num,
                uint16_t      latency,
                uint8_t       service_capabilities,
                P_AVDTP_CBACK cback)
{
    int32_t ret = 0;

    p_avdtp = calloc(1, sizeof(T_AVDTP));
    if (p_avdtp == NULL)
    {
        ret = 1;
        goto fail_alloc_avdtp;
    }

    p_avdtp->link_num = link_num;

    p_avdtp->link = calloc(p_avdtp->link_num, sizeof(T_AVDTP_LINK));
    if (p_avdtp->link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (mpa_reg_l2c_proto(PSM_AVDTP, avdtp_l2c_cback, &p_avdtp->queue_id) == false)
    {
        ret = 3;
        goto fail_reg_l2c;
    }

    p_avdtp->stream_latency = latency;
    p_avdtp->sig_timer = 5000;   /* unit in ms */
    p_avdtp->cback = cback;
    p_avdtp->service_category = service_capabilities;
    p_avdtp->local_sep_num = 0;

    return true;

fail_reg_l2c:
    free(p_avdtp->link);
fail_alloc_link:
    free(p_avdtp);
    p_avdtp = NULL;
fail_alloc_avdtp:
    PROTOCOL_PRINT_ERROR1("avdtp_init: failed %d", -ret);
    return false;
}

bool avdtp_get_roleswap_info(uint8_t               bd_addr[6],
                             T_ROLESWAP_A2DP_INFO *p_info)
{
    T_AVDTP_LINK *p_link;

    p_link = avdtp_find_link_by_addr(bd_addr);

    if (p_link != NULL)
    {
        p_info->int_flag = p_link->sig_chann.int_flag;
        p_info->int_seid = p_link->sig_chann.int_seid;
        p_info->acp_seid_idx = p_link->sig_chann.acp_seid_idx;
        p_info->acp_seid = p_link->sig_chann.acp_seid;
        p_info->tx_trans_label = p_link->sig_chann.tx_trans_label;
        p_info->rx_start_trans_label = p_link->sig_chann.rx_start_trans_label;
        p_info->sig_state = (uint8_t)p_link->sig_chann.sig_state;
        p_info->cmd_flag = p_link->sig_chann.cmd_flag;
        p_info->delay_report = p_link->sig_chann.delay_report_flag;
        p_info->data_offset  = p_avdtp->data_offset;

        return true;
    }

    return false;
}

bool avdtp_set_roleswap_info(uint8_t               bd_addr[6],
                             T_ROLESWAP_A2DP_INFO *p_info)
{
    T_AVDTP_LINK *p_link;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = avdtp_alloc_link(bd_addr);
    }

    if (p_link != NULL)
    {
        p_link->sig_chann.acp_seid_idx = p_info->acp_seid_idx;
        p_link->sig_chann.acp_seid = p_info->acp_seid;
        p_link->sig_chann.cid = p_info->sig_cid;
        p_link->sig_chann.cmd_flag = p_info->cmd_flag;
        p_link->sig_chann.int_flag = p_info->int_flag;
        p_link->sig_chann.int_seid = p_info->int_seid;
        p_link->sig_chann.sig_state = (T_AVDTP_SIG_STATE)p_info->sig_state;
        p_link->sig_chann.state = AVDTP_STATE_CONNECTED;
        p_link->sig_chann.tx_trans_label = p_info->tx_trans_label;
        p_link->sig_chann.rx_start_trans_label = p_info->rx_start_trans_label;

        p_link->strm_chann.cid = p_info->stream_cid;
        p_link->strm_chann.state = AVDTP_STATE_CONNECTED;
        p_link->sig_chann.delay_report_flag = p_info->delay_report;

        p_avdtp->data_offset = p_info->data_offset;

        return true;
    }
    else
    {
        return false;
    }
}

bool avdtp_del_roleswap_info(uint8_t bd_addr[6])
{
    T_AVDTP_LINK *p_link;

    p_link = avdtp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    avdtp_free_link(p_link);

    return true;
}

#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "avdtp.h"

bool avdtp_init(uint8_t       link_num,
                uint16_t      latency,
                uint8_t       service_capabilities,
                P_AVDTP_CBACK cback)
{
    return false;
}

bool avdtp_codec_add(uint8_t           role,
                     T_A2DP_CODEC_TYPE  codec_type,
                     uint8_t           *media_codec_info)
{
    return false;
}

bool avdtp_codec_delete(uint8_t           role,
                        T_A2DP_CODEC_TYPE codec_type)
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
                              uint8_t  role)
{

}

void avdtp_stream_connect_req(uint8_t bd_addr[6])
{

}

bool avdtp_signal_connect_cfm(uint8_t bd_addr[6],
                              bool    accept)
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
                                   uint16_t latency)
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

bool avdtp_get_roleswap_info(uint8_t               bd_addr[6],
                             T_ROLESWAP_A2DP_INFO *p_info)
{
    return false;
}

bool avdtp_set_roleswap_info(uint8_t               bd_addr[6],
                             T_ROLESWAP_A2DP_INFO *p_info)
{
    return false;
}

bool avdtp_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}
#endif
