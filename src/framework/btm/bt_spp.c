/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "os_mem.h"
#include "trace.h"
#include "spp.h"
#include "bt_spp_int.h"
#include "bt_spp.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

uint8_t spp_service_num = 0;
T_SPP_SERVICE *p_spp_service = NULL;

void bt_spp_cback(uint8_t    bd_addr[6],
                  T_SPP_MSG  msg_type,
                  void      *msg_buf)
{
    T_BT_BR_LINK *p_link;
    T_BT_MSG_PAYLOAD payload;

    p_link = bt_find_br_link(bd_addr);

    memcpy(payload.bd_addr, bd_addr, 6);
    payload.msg_buf = msg_buf;

    switch (msg_type)
    {
    case SPP_MSG_CONN_IND:
        {
            T_SPP_CONN_IND *p_ind = (T_SPP_CONN_IND *)msg_buf;

            if (p_link != NULL)
            {
                bt_mgr_dispatch(BT_MSG_SPP_CONN_IND, &payload);
            }
            else
            {
                spp_connect_cfm(bd_addr, p_ind->local_server_chann, false, 0, 0);
            }
        }
        break;

    case SPP_MSG_CONN_CMPL:
        {
            T_SPP_CONN_CMPL *p_cmpl = (T_SPP_CONN_CMPL *)msg_buf;

            if (p_link != NULL)
            {
                if (p_link->spp_data.spp_ref == 0)
                {
                    p_link->connected_profile |= SPP_PROFILE_MASK;
                }
                p_link->spp_data.spp_ref++;

                bt_mgr_dispatch(BT_MSG_SPP_CONN_CMPL, &payload);
            }
            else
            {
                spp_disconnect_req(bd_addr, p_cmpl->local_server_chann);
            }
        }
        break;

    case SPP_MSG_CONN_FAIL:
        {
            if (p_link != NULL)
            {
                payload.msg_buf = msg_buf;
                bt_mgr_dispatch(BT_MSG_SPP_CONN_FAIL, &payload);
            }
        }
        break;

    case SPP_MSG_DISCONN_CMPL:
        {
            if (p_link != NULL)
            {
                bt_mgr_dispatch(BT_MSG_SPP_DISCONN_CMPL, &payload);

                if (p_link->spp_data.spp_ref > 0)
                {
                    p_link->spp_data.spp_ref--;
                }
                if (p_link->spp_data.spp_ref == 0)
                {
                    p_link->connected_profile &= ~SPP_PROFILE_MASK;
                }
            }
        }
        break;

    case SPP_MSG_DATA_IND:
        {
            if (p_link != NULL)
            {
                bt_mgr_dispatch(BT_MSG_SPP_DATA_IND, &payload);
            }
        }
        break;

    case SPP_MSG_DATA_RSP:
        {
            if (p_link != NULL)
            {
                bt_mgr_dispatch(BT_MSG_SPP_DATA_RSP, &payload);
            }
        }
        break;

    case SPP_MSG_CREDIT_INFO:
        {
            if (p_link != NULL)
            {
                bt_mgr_dispatch(BT_MSG_SPP_CREDIT_INFO, &payload);
            }
        }
        break;

    default:
        break;
    }
}

bool bt_spp_init(uint8_t link_num,
                 uint8_t service_num)
{
    if (p_spp_service != NULL)
    {
        return false;
    }

    p_spp_service = os_mem_zalloc2(service_num * sizeof(T_SPP_SERVICE));
    if (p_spp_service == NULL)
    {
        return false;
    }

    if (spp_init(link_num, service_num) == false)
    {
        os_mem_free(p_spp_service);
        p_spp_service = NULL;

        return false;
    }

    spp_service_num = service_num;
    return true;
}

bool bt_spp_service_register(uint8_t *service_uuid,
                             uint8_t  server_chann)
{
    uint8_t i;
    T_SPP_SERVICE *p_service;

    for (i = 0; i < spp_service_num; i++)
    {
        p_service = &p_spp_service[i];
        if (p_service->server_chann == 0)
        {
            p_service->server_chann = server_chann;
            memcpy(p_service->uuid, service_uuid, 16);

            return spp_reg_cb(server_chann, bt_spp_cback);
        }
    }

    return false;
}

bool bt_spp_registered_uuid_check(T_BT_SPP_UUID_TYPE  type,
                                  T_BT_SPP_UUID_DATA *data,
                                  uint8_t            *local_server_chann)
{
    uint8_t i;
    /* base UUID 128 for Bluetooth */
    uint8_t uuid[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

    switch (type)
    {
    case BT_SDP_UUID16:
        uuid[2] = (uint8_t)(data->uuid_16 >> 8);
        uuid[3] = (uint8_t)data->uuid_16;
        break;

    case BT_SDP_UUID32:
        uuid[0] = (uint8_t)(data->uuid_32 >> 24);
        uuid[1] = (uint8_t)(data->uuid_32 >> 16);
        uuid[2] = (uint8_t)(data->uuid_32 >> 8);
        uuid[3] = (uint8_t)data->uuid_32;
        break;

    case BT_SDP_UUID128:
        memcpy(uuid, data->uuid_128, 16);
        break;

    default:
        return false;
    }

    for (i = 0; i < spp_service_num; i++)
    {
        if (memcmp(p_spp_service[i].uuid, uuid, 16) == 0)
        {
            *local_server_chann = p_spp_service[i].server_chann;
            return true;
        }
    }

    return false;
}

bool bt_spp_ertm_mode_set(bool enable)
{
    return spp_set_ertm_mode(enable);
}

bool bt_spp_connect_req(uint8_t  bd_addr[6],
                        uint8_t  rmt_server_chann,
                        uint16_t frame_size,
                        uint8_t  credits,
                        uint8_t  local_server_chann)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        p_link = bt_alloc_br_link(bd_addr);
    }

    if (p_link != NULL)
    {
        return spp_connect_req(bd_addr, rmt_server_chann, frame_size, credits, local_server_chann);
    }
    else
    {
        BTM_PRINT_ERROR0("bt_spp_connect_req: error!!");
    }
    return false;
}

bool bt_spp_connect_cfm(uint8_t  bd_addr[6],
                        uint8_t  local_server_chann,
                        bool     accept,
                        uint16_t frame_size,
                        uint8_t  credits)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        return spp_connect_cfm(bd_addr, local_server_chann, accept, frame_size, credits);
    }
    else
    {
        BTM_PRINT_ERROR0("bt_spp_connect_cfm: error!!");
        return false;
    }
}

bool bt_spp_disconnect_req(uint8_t bd_addr[6],
                           uint8_t local_server_chann)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        return spp_disconnect_req(bd_addr, local_server_chann);
    }

    return false;
}

bool bt_spp_disconnect_all_req(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bt_sniff_mode_exit(p_link, false);
        spp_disconnect_all(bd_addr);
        return true;
    }

    return false;
}

bool bt_spp_data_send(uint8_t   bd_addr[6],
                      uint8_t   local_server_chann,
                      uint8_t  *data,
                      uint16_t  len,
                      bool      ack)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    return spp_send_data(bd_addr, local_server_chann, data, len, ack);
}

bool bt_spp_credits_give(uint8_t bd_addr[6],
                         uint8_t local_server_chann,
                         uint8_t credits)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link == NULL)
    {
        BTM_PRINT_ERROR0("bt_spp_credits_give: no br link");
        return false;
    }

    return spp_send_credit(bd_addr, local_server_chann, credits);
}
