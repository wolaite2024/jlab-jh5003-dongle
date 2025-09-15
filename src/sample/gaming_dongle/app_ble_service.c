/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdlib.h>
#include "trace.h"
#include "os_mem.h"
#include "dfu_api.h"
#include "transmit_service.h"
#include "bt_gatt_svc.h"
#include "ota_service.h"
#include "app_ble_service.h"
#include "app_main.h"
#include "app_ble_gap.h"
#include "app_report.h"
#include "app_cmd.h"
#include "app_cfg.h"
#include "app_ble_common_adv.h"

static void app_ble_service_handle_rx_data(T_APP_LE_LINK *p_link, uint8_t *p_data,
                                           uint16_t data_len)
{
    uint16_t total_len;

    if (app_cfg_const.enable_rtk_vendor_cmd)
    {
        uint8_t rx_seqn;

        if (p_link->p_embedded_cmd == NULL)
        {
            uint16_t cmd_len;

            while (data_len > 5)
            {
                if (p_data[0] == CMD_SYNC_BYTE)
                {
                    rx_seqn = p_data[1];
                    cmd_len = (p_data[2] | (p_data[3] << 8)) + 4; //sync_byte, seqn, length
                    if (data_len >= cmd_len)
                    {
                        app_cmd_handler(&p_data[4], (cmd_len - 4), CMD_PATH_LE, rx_seqn, p_link->id);
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
                p_link->p_embedded_cmd = malloc(data_len);
                if (p_link->p_embedded_cmd != NULL)
                {
                    memcpy(p_link->p_embedded_cmd, p_data, data_len);
                    p_link->embedded_cmd_len = data_len;
                }
            }
        }
        else
        {
            uint8_t *p_temp;
            uint16_t cmd_len;

            p_temp = p_link->p_embedded_cmd;
            total_len = p_link->embedded_cmd_len + data_len;
            p_link->p_embedded_cmd = malloc(total_len);
            if (p_link->p_embedded_cmd != NULL)
            {
                memcpy(p_link->p_embedded_cmd, p_temp, p_link->embedded_cmd_len);
                free(p_temp);
                memcpy(p_link->p_embedded_cmd + p_link->embedded_cmd_len, p_data, data_len);
                p_link->embedded_cmd_len = total_len;
                data_len = total_len;
            }
            else
            {
                p_link->p_embedded_cmd = p_temp;
                data_len = p_link->embedded_cmd_len;
            }
            p_data = p_link->p_embedded_cmd;

            //ios will auto combine two cmd into one pkt
            while (data_len > 5)
            {
                if (p_data[0] == CMD_SYNC_BYTE)
                {
                    rx_seqn = p_data[1];
                    cmd_len = (p_data[2] | (p_data[3] << 8)) + 4;
                    if (data_len >= cmd_len)
                    {
                        app_cmd_handler(&p_data[4], (cmd_len - 4), CMD_PATH_LE, rx_seqn, p_link->id);
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

            if (data_len && p_data != NULL)
            {
                p_temp = p_link->p_embedded_cmd;
                p_link->p_embedded_cmd = malloc(data_len);
                if (p_link->p_embedded_cmd != NULL)
                {
                    memcpy(p_link->p_embedded_cmd, p_data, data_len);
                    p_link->embedded_cmd_len = data_len;
                    free(p_temp);
                }
            }
        }
    }
    else if (app_cfg_const.enable_data_uart)
    {
        uint8_t     *tx_ptr;
        uint8_t     pkt_type;
        uint16_t    pkt_len;

        pkt_type = PKT_TYPE_SINGLE;
        total_len = data_len;
        while (data_len)
        {
            if (data_len > (app_db.external_mcu_mtu - 12))
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
                pkt_len = data_len;
                if (pkt_type != PKT_TYPE_SINGLE)
                {
                    pkt_type = PKT_TYPE_END;
                }
            }
            tx_ptr = malloc(pkt_len + 6);
            if (tx_ptr != NULL)
            {
                tx_ptr[0] = p_link->id;
                tx_ptr[1] = pkt_type;
                tx_ptr[2] = (uint8_t)total_len;
                tx_ptr[3] = (uint8_t)(total_len >> 8);
                tx_ptr[4] = (uint8_t)pkt_len;
                tx_ptr[5] = (uint8_t)(pkt_len >> 8);
                memcpy(&tx_ptr[6], p_data, pkt_len);

                app_report_event(CMD_PATH_UART, EVENT_LE_DATA_TRANSFER, 0, tx_ptr, pkt_len + 6);

                free(tx_ptr);
            }
            p_data += pkt_len;
            data_len -= pkt_len;
        }
    }
    else
    {
        APP_PRINT_ERROR0("app_ble_service_handle_rx_data: enable_rtk_vendor_cmd and enable_data_uart not set");
    }
}

uint8_t app_ble_service_transfer(uint8_t app_idx, uint8_t *data, uint32_t len)
{
    uint8_t result = LE_SEND_OK;

    if (app_db.le_link[app_idx].state != LE_LINK_STATE_CONNECTED)
    {
        result = LE_NOT_CONN;
        return result;
    }

    APP_PRINT_INFO1("app_ble_service_transfer: tx flag 0x%x",
                    app_db.le_link[app_idx].transmit_srv_tx_enable_fg);
    if (app_db.le_link[app_idx].transmit_srv_tx_enable_fg != TX_ENABLE_READY)
    {
        result = LE_NOT_READY;
        return result;
    }

    uint16_t cid;
    uint8_t cid_num;

    gap_chann_get_cid(app_db.le_link[app_idx].conn_handle, 1, &cid, &cid_num);

    if (!transmit_srv_tx_data(app_db.le_link[app_idx].conn_handle, cid, len, data))
    {
        result = LE_SEND_FAIL;
    }

    return result;
}

static T_APP_RESULT app_ble_service_transmit_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_APP_LE_LINK *p_link;
    T_TRANSMIT_SRV_CALLBACK_DATA *p_callback = (T_TRANSMIT_SRV_CALLBACK_DATA *)p_data;

    APP_PRINT_INFO2("app_ble_service_transmit_srv_cb: conn_id %d, msg_type %d", p_callback->conn_id,
                    p_callback->msg_type);
    p_link = app_link_find_le_link_by_conn_id(p_callback->conn_id);
    if (p_link != NULL)
    {
        if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
        {
            if (p_callback->attr_index == TRANSMIT_SVC_RX_DATA_INDEX)
            {
                uint8_t         *p_data;
                uint16_t        data_len;

                p_data = p_callback->msg_data.rx_data.p_value;
                data_len = p_callback->msg_data.rx_data.len;
                app_ble_service_handle_rx_data(p_link, p_data, data_len);
            }
        }
        else if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION)
        {
            if (p_callback->attr_index == TRANSMIT_SVC_TX_DATA_CCCD_INDEX)
            {
                app_ble_common_adv_set_conn_id(p_callback->conn_id);

                if (p_callback->msg_data.notification_indification_value == TRANSMIT_SVC_TX_DATA_CCCD_ENABLE)
                {
                    p_link->transmit_srv_tx_enable_fg |= TX_ENABLE_CCCD_BIT;
                    APP_PRINT_INFO0("app_ble_service_transmit_srv_cb: TRANSMIT_SVC_TX_DATA_CCCD_ENABLE");
                }
                else if (p_callback->msg_data.notification_indification_value == TRANSMIT_SVC_TX_DATA_CCCD_DISABLE)
                {
                    p_link->transmit_srv_tx_enable_fg &= ~TX_ENABLE_CCCD_BIT;
                    APP_PRINT_INFO0("app_ble_service_transmit_srv_cb: TRANSMIT_SVC_TX_DATA_CCCD_DISABLE");
                }
            }
        }
    }

    return app_result;
}

static T_APP_RESULT app_ble_service_ota_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_OTA_CALLBACK_DATA *p_ota_cb_data = (T_OTA_CALLBACK_DATA *)p_data;
    APP_PRINT_INFO2("app_ble_service_ota_srv_cb: service_id %d, msg_type %d",
                    service_id, p_ota_cb_data->msg_type);
    switch (p_ota_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        break;
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        if (OTA_WRITE_CHAR_VAL == p_ota_cb_data->msg_data.write.opcode &&
            OTA_VALUE_ENTER == p_ota_cb_data->msg_data.write.value)
        {
            /* Check battery level first */
            if (app_db.local_batt_level >= 30)
            {
                T_APP_LE_LINK *p_link;
                p_link = app_link_find_le_link_by_conn_id(p_ota_cb_data->conn_id);
                /* Battery level is greater than or equal to 30 percent */
                if (p_link != NULL)
                {
                    app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_SWITCH_TO_OTA);
                }
                APP_PRINT_INFO1("app_ble_service_ota_srv_cb: Preparing switch into OTA mode conn_id %d",
                                p_ota_cb_data->conn_id);
            }
            else
            {
                /* Battery level is less than 30 percent */
                APP_PRINT_WARN1("app_ble_service_ota_srv_cb: Battery level is not enough to support OTA, local_batt_level %d",
                                app_db.local_batt_level);
            }
        }
        break;

    default:

        break;
    }

    return app_result;
}

void app_ble_service_init(void)
{
    transmit_srv_add(app_ble_service_transmit_srv_cb);
    ota_add_service(app_ble_service_ota_srv_cb);
}
