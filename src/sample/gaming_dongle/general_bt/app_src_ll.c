#include "app_src_ll.h"
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>

#include "os_mem.h"
#include "trace.h"
#include "btm.h"
#include "bt_spp.h"
#include "app_link_util.h"

#include "app_cfg.h"
#include "app_main.h"

#include "app_cmd.h"


/*
//set ll cmd le
static uint8_t ll_cmd_set[] =
{
    0xAA,           //sync word
    0x01,           //seq no
    0x05, 0x00,     //length
    0x00, 0x0B,     //Command ID
    0x01,           //enable 1, disable 0
    0x14, 0x00      //latency 20 ms
};

//get ll cmd, le
static uint8_t ll_cmd_get[] =
{
    0xAA,           //sync word
    0x01,           //seq
    0x02, 0x00,     //length
    0x01, 0x0B      // Command ID
};
*/

/*
1. CMD_INFO_REQ
    return EVENT_INFO_RSP

2. CMD_GET_LOW_LATENCY_MODE_STATUS:
    return 1.EVENT_ACK
              2.EVENT_LOW_LATENCY_STATUS

3. 3.2 CMD_LOW_LATENCY_MODE
    return EVENT_ACK
*/

#define SRC_RTK_LT_MAX  0x003C

/*
static uint8_t src_rtk_vendor_cmd_checksum_calc(uint8_t *data, uint16_t len)
{
    uint8_t checksum = 0;
    int i;

    for (i = 0; i < len; i++)
    {
        checksum += *(data + i);
    }

    checksum = 0xff - checksum + 1;

    return checksum;
}
*/

//in_len = sizeof(data)
static uint8_t *src_rtk_vendor_cmd_gen(uint16_t opcode, uint8_t *data, uint16_t in_len,
                                       uint16_t *out_len)
{
    const uint8_t sync_word = 0xAA;
    static uint8_t seq = 0;

    uint8_t *buf = NULL;
    uint16_t total_len = 0;

    //sizeof(sync_word) + sizeof(seq) + sizeof(len) + sizeof(opcode) + len;
    total_len = 1 + 1 + 2 + 2 + in_len;

    buf = os_mem_zalloc(RAM_TYPE_DATA_ON, total_len);

    if (buf == NULL)
    {
        *out_len = 0;
        return NULL;
    }

    buf[0] = sync_word;
    buf[1] = seq++;
    buf[2] = (uint8_t)((in_len + 2) & 0x00ff);
    buf[3] = (uint8_t)(((in_len + 2) & 0xff00) >> 8);
    buf[4] = (uint8_t)(opcode & 0x00ff);
    buf[5] = (uint8_t)((opcode & 0xff00) >> 8);

    if (data != NULL)
    {
        memcpy(buf + 6, data, in_len);
    }

    //buf[total_len - 1] = src_rtk_vendor_cmd_checksum_calc(buf+1, total_len-2);

    *out_len = total_len;

    return buf;
}

static void src_rtk_get_feature(uint8_t *bd_addr, uint8_t local_server_chann)
{
    uint16_t out_len = 0;
    uint8_t *cmd_buf = src_rtk_vendor_cmd_gen(CMD_INFO_REQ, NULL, 0, &out_len);

    if (cmd_buf != NULL)
    {
        bt_spp_data_send(bd_addr, local_server_chann, cmd_buf, out_len, false);
        os_mem_free(cmd_buf);
    }
    else
    {
        APP_PRINT_ERROR0("src_rtk_get_feature: no buffer available!");
    }
}

static void src_rtk_ll_get(uint8_t *bd_addr, uint8_t local_server_chann)
{
    uint16_t out_len = 0;
    uint8_t *cmd_buf = src_rtk_vendor_cmd_gen(0x0B01, NULL, 0, &out_len);

    if (cmd_buf != NULL)
    {
        bt_spp_data_send(bd_addr, local_server_chann, cmd_buf, out_len, false);
        os_mem_free(cmd_buf);
    }
    else
    {
        APP_PRINT_ERROR0("src_rtk_ll_get: no buffer available!");
    }
}

static void src_rtk_ll_enable(uint8_t *bd_addr, uint8_t local_server_chann, bool enable,
                              uint16_t lt)
{
    uint16_t out_len = 0;
    uint8_t *cmd_buf = NULL;

    uint8_t param[3] = {0};

    if (enable)
    {
        param[0] = 0x01;
        param[1] = (uint8_t)(lt & 0x00ff);
        param[2] = (uint8_t)((lt & 0xff00) >> 8);
        cmd_buf = src_rtk_vendor_cmd_gen(0x0B00, param, 3, &out_len);
    }
    else
    {
        param[0] = 0x00;
        cmd_buf = src_rtk_vendor_cmd_gen(0x0B00, param, 1, &out_len);
    }

    if (cmd_buf != NULL)
    {
        bt_spp_data_send(bd_addr, local_server_chann, cmd_buf, out_len, false);
        os_mem_free(cmd_buf);
    }
    else
    {
        APP_PRINT_ERROR0("src_rtk_ll_enable: no buffer available!");
    }
}

static void src_rtk_ll_handle_event_ack(uint8_t *bd_addr,
                                        uint8_t local_server_chann,
                                        uint8_t *param,
                                        uint16_t len)
{
    uint16_t cmd_id = param[0] + (param[1] << 8);

    APP_PRINT_INFO3("src_rtk_ll_handle_event_ack: %s, cmd_id %04x, param[2] 0x%x",
                    TRACE_BDADDR(bd_addr), cmd_id, param[2]);

    switch (cmd_id)
    {
    case CMD_INFO_REQ:
        src_rtk_ll_get(bd_addr, local_server_chann);
        break;

    default:
        break;
    }
}

static void src_rtk_ll_handle_event_ll_status(uint8_t *bd_addr,
                                              uint8_t local_server_chann,
                                              uint8_t *param,
                                              uint16_t len)
{
    uint8_t mode = param[0];
    uint16_t lt = param[1] + (param[2] << 8);

    APP_PRINT_INFO2("src_rtk_ll_handle_event_ll_status: %d, %04x",
                    mode, lt);

    if ((!mode) || (lt > SRC_RTK_LT_MAX))
    {
        src_rtk_ll_enable(bd_addr, local_server_chann, true, SRC_RTK_LT_MAX);
    }
}

static void src_rtk_ll_handle_spp_data(uint8_t *bd_addr,
                                       uint8_t local_server_chann,
                                       uint8_t *p_data,
                                       uint16_t len)
{
    uint16_t data_len = 0;
    uint16_t cmd_id = 0;

    if (p_data[0] == CMD_SYNC_BYTE)
    {
        data_len = p_data[2] + (p_data[3] << 8);
        cmd_id = p_data[4] + (p_data[5] << 8);

        APP_PRINT_INFO2("app_src_handle_spp_data: opcode %04x, data_len %d", cmd_id, data_len);

        switch (cmd_id)
        {
        case CMD_ACK:
            src_rtk_ll_handle_event_ack(bd_addr, local_server_chann, p_data + 6, len - 6);
            break;

        case 0x0B00:
            src_rtk_ll_handle_event_ll_status(bd_addr, local_server_chann, p_data + 6, len - 6);
            break;

        default:
            break;
        }
    }
}

static void src_rtk_ll_spp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    //only send & recv data, handle ll data

    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_SPP_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->spp_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                if (p_link->rfc_credit)
                {
                    APP_PRINT_INFO0("app_src_spp_bt_cback: try to set ll");
                    src_rtk_get_feature(p_link->bd_addr,
                                        param->spp_conn_cmpl.local_server_chann);
                }
            }
        }
        break;

    case BT_EVENT_SPP_DATA_IND:
        {
            uint8_t     *p_data;
            uint16_t    len;

            p_data = param->spp_data_ind.data;
            len = param->spp_data_ind.len;

            p_link = app_find_br_link(param->spp_data_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_src_spp_bt_cback: no acl link found");
                return;
            }

            src_rtk_ll_handle_spp_data(param->spp_data_ind.bd_addr,
                                       param->spp_data_ind.local_server_chann,
                                       p_data,
                                       len);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_src_spp_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_src_ll_init(void)
{
    bt_mgr_cback_register(src_rtk_ll_spp_bt_cback);
}
