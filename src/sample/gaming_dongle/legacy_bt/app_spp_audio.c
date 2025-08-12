/*
 *  Copyright (C) 2020 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "bt_rfc.h"
#include "app_spp_audio.h"
#include "app_link_util.h"
#include "gaming_bt.h"
#include "app_sdp.h"
#include "general_bt.h"
#include "app_src_policy.h"
#include "app_ctrl_pkt_policy.h"
#include "app_audio_pipe.h"
#include "app_gaming_sync.h"

#ifdef LEGACY_BT_GAMING
#include "usb_audio_stream.h"
#endif

#define MAX_SPP_AUDIO_CACHE (3 + 512 + 1)

typedef struct
{
    uint8_t bd_addr[6];
    bool    connected;
    bool    disable_sniff;
} t_spp_dev;

t_spp_dev g_spp_dev;

static void app_spp_audio_cback(uint8_t *bd_addr, T_BT_RFC_MSG_TYPE msg_type, void *msg_buf)
{
    T_APP_BR_LINK *p_link;
    if (msg_type != BT_RFC_MSG_DATA_IND)
    {
        APP_PRINT_INFO1("app_spp_audio_cback: msg_type 0x%02x", msg_type);
    }
    switch (msg_type)
    {
    case BT_RFC_MSG_CONN_IND:
        {
            T_BT_RFC_CONN_IND *p_ind = (T_BT_RFC_CONN_IND *)msg_buf;

            APP_PRINT_INFO2("app_spp_audio_cback(): Recv rfc conn ind %b %u",
                            TRACE_BDADDR(bd_addr),
                            p_ind->local_server_chann);
            if (p_ind->local_server_chann != RFC_SPP_AUDIO_CHANN_NUM)
            {
                break;
            }

            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                bt_rfc_conn_cfm(p_link->bd_addr, p_ind->local_server_chann, true, p_ind->frame_size, 100);
            }
            else
            {
                APP_PRINT_WARN0("app_spp_audio_cback: no acl link found");
                bt_rfc_conn_cfm(bd_addr, p_ind->local_server_chann, false, p_ind->frame_size, 7);
            }
        }
        break;

    case BT_RFC_MSG_CONN_CMPL:
        {
            T_BT_RFC_CONN_CMPL *p_cmpl = (T_BT_RFC_CONN_CMPL *)msg_buf;

            APP_PRINT_INFO2("app_spp_audio_cback(): Recv rfc conn cmpl %b %u",
                            TRACE_BDADDR(bd_addr),
                            p_cmpl->local_server_chann);
            if (p_cmpl->local_server_chann != RFC_SPP_AUDIO_CHANN_NUM)
            {
                break;
            }

            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
#ifdef LEGACY_BT_GAMING
                gaming_bt_profile_conn_cmpl(bd_addr, SPP_AUDIO_PROFILE_MASK);

                app_gaming_sync_set_link_connected(true, bd_addr);
#else
                general_bt_profile_conn_cmpl(bd_addr, SPP_AUDIO_PROFILE_MASK);
#endif
                g_spp_dev.connected = true;
                memcpy(g_spp_dev.bd_addr, bd_addr, 6);

#ifdef LEGACY_BT_GAMING
                legacy_gaming_verify(bd_addr);
#endif
            }
        }
        break;

    case BT_RFC_MSG_DISCONN_CMPL:
        {
            T_BT_RFC_DISCONN_CMPL *p_info = (T_BT_RFC_DISCONN_CMPL *)msg_buf;

            APP_PRINT_INFO2("app_spp_audio_cback(): Recv rfc disconn cmpl %b %u",
                            TRACE_BDADDR(bd_addr),
                            p_info->local_server_chann);
            if (p_info->local_server_chann != RFC_SPP_AUDIO_CHANN_NUM)
            {

                break;
            }

            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
#ifdef LEGACY_BT_GAMING
                gaming_bt_profile_disconn_cmpl(bd_addr, p_info->cause,
                                               SPP_AUDIO_PROFILE_MASK);

                app_gaming_sync_set_link_connected(false, bd_addr);
#else
                general_bt_profile_disconn_cmpl(bd_addr, p_info->cause, SPP_AUDIO_PROFILE_MASK);
#endif
                g_spp_dev.connected = false;
                memset(g_spp_dev.bd_addr, 0, 6);
            }
        }
        break;

    case BT_RFC_MSG_DATA_IND:
        {
            T_BT_RFC_DATA_IND *p_ind = (T_BT_RFC_DATA_IND *)msg_buf;

            if (p_ind->local_server_chann != RFC_SPP_AUDIO_CHANN_NUM)
            {
                APP_PRINT_INFO2("app_spp_audio_cback(): Recv unexpected data from %b %u",
                                TRACE_BDADDR(bd_addr),
                                p_ind->local_server_chann);
                break;
            }

            /* APP_PRINT_INFO2("app_spp_audio_cback(): len %u, data %b",
             *                 p_ind->length,
             *                 TRACE_BINARY(p_ind->length, p_ind->buf));
             */
            p_link = app_find_br_link(bd_addr);
            if (p_link == NULL)
            {
                return;
            }

            app_gaming_sync_disassemble_data(bd_addr, p_ind->buf, p_ind->length);

            bt_rfc_credits_give(bd_addr, p_ind->local_server_chann, 1);
        }
        break;

    case BT_RFC_MSG_CREDIT_INFO:
        break;

    default:
        break;
    }
}

bool app_send_out_by_spp(uint8_t *data, uint16_t len)
{
    if (g_spp_dev.connected == false)
    {
        return false;
    }

    if (data[4] == 'R' && data[5] == 'T' && data[6] == 'K')
    {
        if (!g_spp_dev.disable_sniff)
        {
            bt_sniff_mode_disable(g_spp_dev.bd_addr);
            g_spp_dev.disable_sniff = true;
            APP_PRINT_INFO0("app_send_out_by_spp: disable sniff mode");
        }
    }
    /* FW_UPDATE_COMMAND_FLAGS, last packet flag == 0x40.*/
    if (data[6] == 'K' && data[9] == 0x2a && data[10] == 0x40)
    {
        if (g_spp_dev.disable_sniff)
        {
            bt_sniff_mode_enable(g_spp_dev.bd_addr, 784, 816, 0, 0);
            g_spp_dev.disable_sniff = false;
            APP_PRINT_INFO0("app_send_out_by_spp: enable sniff mode");
        }
    }

    if (!bt_rfc_data_send(g_spp_dev.bd_addr, RFC_SPP_AUDIO_CHANN_NUM, data, len, false))
    {
        APP_PRINT_WARN0("app_send_out_by_spp: send data err");
        return false;
    }
    return true;
}

void app_spp_audio_init(void)
{
    bt_rfc_profile_register(RFC_SPP_AUDIO_CHANN_NUM, app_spp_audio_cback);
    memset(g_spp_dev.bd_addr, 0, 6);
    g_spp_dev.connected = false;
}
