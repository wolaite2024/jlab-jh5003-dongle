/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>

#include "os_mem.h"
#include "trace.h"
#include "btm.h"
#include "bt_spp.h"
#include "app_link_util.h"
#include "app_spp.h"
#include "app_cfg.h"
//#include "app_transfer.h"
#include "app_main.h"
//#include "app_cmd.h"
#include "app_sdp.h"
#include "bt_spp.h"

/* From app_cmd.h */
typedef enum t_pkt_type
{
    PKT_TYPE_SINGLE = 0x00,
    PKT_TYPE_START = 0x01,
    PKT_TYPE_CONT = 0x02,
    PKT_TYPE_END = 0x03
} T_PKT_TYPE;

#ifndef CMD_SYNC_BYTE
#define CMD_SYNC_BYTE   0xAA
#endif

static const uint8_t spp_service_class_uuid128[16] =
{
    0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
};

static void app_spp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_SPP_CONN_CMPL:
        {
            if (param->spp_conn_cmpl.local_server_chann != RFC_SPP_CHANN_NUM)
            {
                return;
            }

            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->spp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                p_link->rfc_credit = param->spp_conn_cmpl.link_credit;
                p_link->rfc_frame_size = param->spp_conn_cmpl.frame_size;
                //check credit
                if (p_link->rfc_credit)
                {
                    //app_pop_data_transfer_queue(CMD_PATH_SPP, false);
                }
            }
        }
        break;

    case BT_EVENT_SPP_CREDIT_RCVD:
        {
            if (param->spp_credit_rcvd.local_server_chann != RFC_SPP_CHANN_NUM)
            {
                return;
            }

            p_link = app_find_br_link(param->spp_credit_rcvd.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_spp_bt_cback: no acl link found");
                return;
            }
            if ((p_link->rfc_credit == 0) && (param->spp_credit_rcvd.link_credit))
            {
                //app_pop_data_transfer_queue(CMD_PATH_SPP, false);
            }
            p_link->rfc_credit = param->spp_credit_rcvd.link_credit;
        }
        break;

    case BT_EVENT_SPP_DATA_IND:
        {
            if (param->spp_data_ind.local_server_chann != RFC_SPP_CHANN_NUM)
            {
                return;
            }

            uint8_t     *p_data;
            uint16_t    len;
            uint8_t     app_idx;
            uint16_t    data_len;
            uint16_t    total_len;

            p_link = app_find_br_link(param->spp_data_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_spp_bt_cback: no acl link found");
                return;
            }
            app_idx = p_link->id;
            p_data = param->spp_data_ind.data;
            len = param->spp_data_ind.len;
            data_len = len;

            bt_spp_credits_give(app_db.br_link[app_idx].bd_addr, param->spp_data_ind.local_server_chann,
                                1);

            if (app_cfg_const.enable_rtk_vendor_cmd)
            {
                if (app_db.br_link[app_idx].p_embedded_cmd == NULL)
                {
                    uint16_t cmd_len;

                    //ios will auto combine two cmd into one pkt
                    while (data_len)
                    {
                        if (p_data[0] == CMD_SYNC_BYTE)
                        {
                            cmd_len = (p_data[2] | (p_data[3] << 8)) + 4; //sync_byte, seqn, length
                            if (data_len >= cmd_len)
                            {
                                //app_handle_cmd_set(&p_data[4], (cmd_len - 4), CMD_PATH_SPP, p_data[1], app_idx);
                                data_len -= cmd_len;
                                p_data += cmd_len;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            data_len--;
                            p_data++;
                        }
                    }

                    if (data_len)
                    {
                        app_db.br_link[app_idx].p_embedded_cmd = os_mem_alloc(RAM_TYPE_DATA_ON, data_len);
                        memcpy(app_db.br_link[app_idx].p_embedded_cmd, p_data, data_len);
                        app_db.br_link[app_idx].embedded_cmd_len = data_len;
                    }
                }
                else
                {
                    uint8_t *p_temp;
                    uint16_t cmd_len;

                    p_temp = app_db.br_link[app_idx].p_embedded_cmd;
                    total_len = app_db.br_link[app_idx].embedded_cmd_len + data_len;
                    app_db.br_link[app_idx].p_embedded_cmd = os_mem_alloc(RAM_TYPE_DATA_ON, total_len);
                    memcpy(app_db.br_link[app_idx].p_embedded_cmd, p_temp,
                           app_db.br_link[app_idx].embedded_cmd_len);
                    os_mem_free(p_temp);
                    memcpy(app_db.br_link[app_idx].p_embedded_cmd +
                           app_db.br_link[app_idx].embedded_cmd_len,
                           p_data, data_len);
                    app_db.br_link[app_idx].embedded_cmd_len = total_len;

                    p_data = app_db.br_link[app_idx].p_embedded_cmd;
                    data_len = total_len;
                    p_temp = app_db.br_link[app_idx].p_embedded_cmd;
                    app_db.br_link[app_idx].p_embedded_cmd = NULL;
                    //ios will auto combine two cmd into one pkt
                    while (data_len)
                    {
                        if (p_data[0] == CMD_SYNC_BYTE)
                        {
                            cmd_len = (p_data[2] | (p_data[3] << 8)) + 4; //sync_byte, seqn, length
                            if (data_len >= cmd_len)
                            {
                                //app_handle_cmd_set(&p_data[4], (cmd_len - 4), CMD_PATH_SPP, p_data[1], app_idx);
                                data_len -= cmd_len;
                                p_data += cmd_len;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            data_len--;
                            p_data++;
                        }
                    }

                    if (data_len)
                    {
                        app_db.br_link[app_idx].p_embedded_cmd = os_mem_alloc(RAM_TYPE_DATA_ON, data_len);
                        memcpy(app_db.br_link[app_idx].p_embedded_cmd, p_data, data_len);
                    }
                    app_db.br_link[app_idx].embedded_cmd_len = data_len;
                    os_mem_free(p_temp);
                }
            }
            else if (app_cfg_const.enable_data_uart)
            {
                uint8_t     *tx_ptr;
                uint16_t    total_len;
                uint8_t     pkt_type;
                uint16_t    pkt_len;

                pkt_type = PKT_TYPE_SINGLE;
                total_len = len;
                while (len)
                {
                    if (len > (app_db.external_mcu_mtu - 12))
                    {
                        pkt_len = app_db.external_mcu_mtu - 12;
                        if (pkt_type == PKT_TYPE_SINGLE)
                        {
                            pkt_type = PKT_TYPE_START;
                        }
                        else
                        {
                            pkt_type = PKT_TYPE_CONT;
                        }
                    }
                    else
                    {
                        pkt_len = len;
                        if (pkt_type != PKT_TYPE_SINGLE)
                        {
                            pkt_type = PKT_TYPE_END;
                        }
                    }
                    tx_ptr = os_mem_alloc(RAM_TYPE_DATA_ON, (pkt_len + 6));
                    if (tx_ptr != NULL)
                    {
                        tx_ptr[0] = app_idx;
                        tx_ptr[1] = pkt_type;
                        tx_ptr[2] = (uint8_t)total_len;
                        tx_ptr[3] = (uint8_t)(total_len >> 8);
                        tx_ptr[4] = (uint8_t)pkt_len;
                        tx_ptr[5] = (uint8_t)(pkt_len >> 8);
                        memcpy(&tx_ptr[6], p_data, pkt_len);

                        //app_report_event(CMD_PATH_UART, EVENT_LEGACY_DATA_TRANSFER, 0, tx_ptr, pkt_len + 6);

                        os_mem_free(tx_ptr);
                        p_data += pkt_len;
                        len -= pkt_len;
                    }
                }
            }
        }
        break;
    case BT_EVENT_SPP_CONN_IND:
        {
            if (param->spp_conn_ind.local_server_chann != RFC_SPP_CHANN_NUM)
            {
                return;
            }

            p_link = app_find_br_link(param->spp_conn_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_spp_bt_cback: no acl link found");
                return;
            }
            uint8_t local_server_chann = param->spp_conn_ind.local_server_chann;
            uint16_t frame_size = param->spp_conn_ind.frame_size;
            bt_spp_connect_cfm(p_link->bd_addr, local_server_chann, true, frame_size, 7);
        }
        break;

    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            if (param->spp_disconn_cmpl.local_server_chann != RFC_SPP_CHANN_NUM)
            {
                return;
            }

            p_link = app_find_br_link(param->spp_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                p_link->rfc_credit = 0;
            }

            //app_transfer_queue_reset(CMD_PATH_SPP);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_spp_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_spp_init(void)
{
    if (!(app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK))
    {
        APP_PRINT_WARN0("app_spp_init: not support spp");
        return;
    }

    bt_spp_init();

    bt_spp_service_register((uint8_t *)spp_service_class_uuid128, RFC_SPP_CHANN_NUM);

    bt_mgr_cback_register(app_spp_bt_cback);
}

