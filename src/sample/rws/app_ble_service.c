/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "ringtone.h"
#include "gap_conn_le.h"
#include "stdlib.h"
#include "dfu_api.h"
#include "ota_service.h"
#include "transmit_svc_dongle.h"
#include "transmit_service.h"
#include "app_ble_service.h"
#include "app_ble_common_adv.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_transfer.h"
#include "app_cfg.h"
#include "app_ble_gap.h"
#include "app_lea_unicast_audio.h"
#include "bas.h"
#include "dis.h"
#if F_APP_CFU_BLE_CHANNEL_SUPPORT
#include "hids_cfu.h"
#include "app_cfu_transfer.h"
#endif
#include "ble_stream.h"
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "gfps.h"
#endif
#if F_APP_GATT_SERVER_EXT_API_SUPPORT
#include "profile_server_ext.h"
#else
#include "profile_server.h"
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
#include "xm_xiaoai_ble_service.h"
#include "xm_xiaoai_api.h"
#include "app_xm_xiaoai_ble_adv.h"
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
#include "xiaowei_ble_service.h"
#include "xiaowei_protocol.h"
#include "app_xiaowei.h"
#include "app_xiaowei_ble_adv.h"
#include "app_xiaowei_transport.h"
#endif

#if BISTO_FEATURE_SUPPORT
#include "app_bisto_ble.h"
#endif

#if AMA_FEATURE_SUPPORT
#include "app_ama_transport.h"
#endif

#if F_APP_LOCAL_PLAYBACK_SUPPORT
#include "playback_service.h"
#endif

#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
#include "app_asp_device.h"
#include "app_teams_hid.h"
#include "app_teams_rws.h"
#include "app_teams_util.h"
#include "teams_util.h"
#include "app_teams_util.h"
#include "asp_ble_service.h"
#include "asp_device_pkt.h"
#include "asp_device_api.h"
#if F_APP_TEAMS_BT_POLICY
#include "app_teams_bt_policy.h"
#include "app_teams_ble_policy.h"
#include "app_teams_ext_ftl.h"
#endif
#endif
#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio_hid.h"
#endif

#if F_APP_TUYA_SUPPORT
#include "rtk_tuya_ble_service.h"
#include "tuya_ble_api.h"
#include "rtk_tuya_ble_adv.h"
#include "rtk_tuya_ble_device.h"
#endif
#if F_APP_FINDMY_FEATURE_SUPPORT
#include "app_findmy.h"
#include "app_findmy_rws.h"
#include "fmna_gatt.h"
#include "fmna_gatt_platform.h"
#include "fmna_util.h"
#include "app_findmy_ble_adv.h"
#endif
#include "bt_gatt_svc.h"

#if F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT
#include "app_dongle_source_ctrl.h"
#endif

#if F_APP_BLE_HID_CONTROLLER_SUPPORT
#include "app_ble_hid_controller.h"
#endif

#if F_APP_GATT_OVER_BREDR_SUPPORT
#include "rtk_vendor_dis_gatt_svc.h"
#endif

#define MAX_BLE_SRV_NUM 16

static T_APP_RESULT app_ble_service_general_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    uint8_t conn_id;

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    T_SERVER_EXT_APP_CB_DATA *p_para = (T_SERVER_EXT_APP_CB_DATA *)p_data;
    le_get_conn_id_by_handle(p_para->event_data.send_data_result.conn_handle, &conn_id);
#else
    T_SERVER_APP_CB_DATA *p_para = (T_SERVER_APP_CB_DATA *)p_data;
    conn_id = p_para->event_data.send_data_result.conn_id;
#endif

    APP_PRINT_INFO3("app_ble_service_general_srv_cb: conn_id %d eventId %d attrib_idx %d", conn_id,
                    p_para->eventId, p_para->event_data.send_data_result.attrib_idx);

    switch (p_para->eventId)
    {
    case PROFILE_EVT_SRV_REG_COMPLETE:
        break;

    case PROFILE_EVT_SEND_DATA_COMPLETE:
        /*APP_PRINT_TRACE2("PROFILE_EVT_SEND_DATA_COMPLETE: cause 0x%x, attrib_idx %d",
                         p_para->event_data.send_data_result.cause,
                         p_para->event_data.send_data_result.attrib_idx);*/
        if (p_para->event_data.send_data_result.attrib_idx == TRANSMIT_SVC_TX_DATA_INDEX)
        {
            T_CMD_PATH cmd_path = CMD_PATH_NONE;
            if (app_link_find_le_link_by_conn_handle(p_para->event_data.send_data_result.conn_handle))
            {
                cmd_path = CMD_PATH_LE;
            }
            else if (app_link_find_br_link_by_conn_handle(p_para->event_data.send_data_result.conn_handle))
            {
                cmd_path = CMD_PATH_GATT_OVER_BREDR;
            }
            APP_PRINT_INFO1("app_ble_service common_cb: PROFILE_EVT_SEND_DATA_COMPLETE cmd_path %d", cmd_path);
            app_transfer_pop_data_queue(cmd_path, false);
        }

#if AMA_FEATURE_SUPPORT
        if (extend_app_cfg_const.ama_support && app_ama_transport_supported(AMA_BLE_STREAM))
        {
            T_BLE_STREAM_EVENT_PARAM ble_stream_param;
            ble_stream_param.update_credit_param.conn_id = conn_id;
            ble_stream_param.update_credit_param.credits    = p_para->event_data.send_data_result.credits;
            ble_stream_param.update_credit_param.service_id = p_para->event_data.send_data_result.service_id;
            ble_stream_event_process(BLE_STREAM_UPDATE_CREDIT_EVENT, &ble_stream_param);
        }
#endif

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
        if (!gatt_svc_handle_profile_data_cmpl(p_para->event_data.send_data_result.conn_handle,
                                               p_para->event_data.send_data_result.cid,
                                               p_para->event_data.send_data_result.service_id,
                                               p_para->event_data.send_data_result.attrib_idx,
                                               p_para->event_data.send_data_result.credits,
                                               p_para->event_data.send_data_result.cause))
        {
            APP_PRINT_ERROR0("gatt_svc_handle_profile_data_cmpl failed");
        }
#else
        {
            uint16_t conn_handle = le_get_conn_handle(conn_id);
            if (!gatt_svc_handle_profile_data_cmpl(conn_handle, L2C_FIXED_CID_ATT,
                                                   p_para->event_data.send_data_result.service_id,
                                                   p_para->event_data.send_data_result.attrib_idx,
                                                   p_para->event_data.send_data_result.credits,
                                                   p_para->event_data.send_data_result.cause))
            {
                APP_PRINT_ERROR0("gatt_svc_handle_profile_data_cmpl failed");
            }
        }
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
        if (p_para->event_data.send_data_result.cause == GAP_SUCCESS)
        {
            fmna_gatt_indication_state_set(true);
            if (memcmp_val(&fmna_service_current_extended_packet_tx, 0,
                           sizeof(fmna_service_current_extended_packet_tx)))
            {
#if F_APP_FINDMY_USE_UARP
                if (p_ble_evt->evt.gatts_evt.params.hvc.handle == p_m_config->uarp_data_handle.value_handle)
                {
                    NRF_LOG_INFO("UARP Packet has finished being sent.");
                    fmna_uarp_packet_sent();
                }
#endif

                fmna_gatt_dispatch_send_next_packet();
            }
            else
            {
                FMNA_LOG_INFO("sending another indication");
                fmna_gatt_dispatch_send_packet_extension_indication();
            }
        }
#endif
        break;

    default:
        break;
    }
    return app_result;
}

static void app_ble_service_handle_rx_data(T_CMD_PATH cmd_path, T_APP_VENDOR_CMD *p_cmd,
                                           uint8_t *p_data,
                                           uint16_t data_len, uint8_t link_idx)
{
    uint16_t total_len;

    if (app_cfg_const.enable_rtk_vendor_cmd)
    {
        uint8_t rx_seqn;

        if (p_cmd->buf == NULL)
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
                        app_cmd_handler(&p_data[4], (cmd_len - 4), cmd_path, rx_seqn, link_idx);
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
                p_cmd->buf = malloc(data_len);
                if (p_cmd->buf != NULL)
                {
                    memcpy(p_cmd->buf, p_data, data_len);
                    p_cmd->len = data_len;
                }
            }
        }
        else
        {
            uint8_t *p_temp;
            uint16_t cmd_len;

            p_temp = p_cmd->buf;
            total_len = p_cmd->len + data_len;
            p_cmd->buf = malloc(total_len);
            if (p_cmd->buf != NULL)
            {
                memcpy(p_cmd->buf, p_temp, p_cmd->len);
                free(p_temp);
                memcpy(p_cmd->buf + p_cmd->len, p_data, data_len);
                p_cmd->len = total_len;
                data_len = total_len;
            }
            else
            {
                p_cmd->buf = p_temp;
                data_len = p_cmd->len;
            }
            p_data = p_cmd->buf;

            //ios will auto combine two cmd into one pkt
            while (data_len > 5)
            {
                if (p_data[0] == CMD_SYNC_BYTE)
                {
                    rx_seqn = p_data[1];
                    cmd_len = (p_data[2] | (p_data[3] << 8)) + 4;
                    if (data_len >= cmd_len)
                    {
                        app_cmd_handler(&p_data[4], (cmd_len - 4), cmd_path, rx_seqn, link_idx);
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
                p_temp = p_cmd->buf;
                p_cmd->buf = malloc(data_len);
                if (p_cmd->buf != NULL)
                {
                    memcpy(p_cmd->buf, p_data, data_len);
                    p_cmd->len = data_len;
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
                tx_ptr[0] = link_idx;
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

static T_APP_RESULT app_ble_service_transmit_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_TRANSMIT_SRV_CALLBACK_DATA *p_callback = (T_TRANSMIT_SRV_CALLBACK_DATA *)p_data;
    T_CMD_PATH cmd_path = CMD_PATH_LE;
    T_APP_LE_LINK *p_le_link = NULL;
    T_APP_BR_LINK *p_br_link = NULL;

    APP_PRINT_INFO2("app_ble_service_transmit_srv_cb: conn_id %d, msg_type %d", p_callback->conn_id,
                    p_callback->msg_type);

    if ((p_callback->chann_type == GAP_CHANN_TYPE_LE_ATT) ||
        (p_callback->chann_type == GAP_CHANN_TYPE_LE_ECFC))
    {
        cmd_path = CMD_PATH_LE;
        p_le_link = app_link_find_le_link_by_conn_id(p_callback->conn_id);
    }
    else
    {
        cmd_path = CMD_PATH_GATT_OVER_BREDR;
        p_br_link = app_link_find_br_link_by_conn_handle(p_callback->conn_handle);
    }

    if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
    {
        if (p_callback->attr_index == TRANSMIT_SVC_RX_DATA_INDEX)
        {
            uint8_t         *p_data;
            uint16_t        data_len;

            p_data = p_callback->msg_data.rx_data.p_value;
            data_len = p_callback->msg_data.rx_data.len;
            if ((cmd_path == CMD_PATH_LE) && (p_le_link != NULL))
            {
                p_le_link->cmd.tx_mask |= TX_ENABLE_CCCD_BIT;
                app_ble_service_handle_rx_data(cmd_path, &p_le_link->cmd, p_data, data_len, p_le_link->id);
            }
            else if ((cmd_path == CMD_PATH_GATT_OVER_BREDR) && (p_br_link != NULL))
            {
                p_br_link->cmd.tx_mask |= TX_ENABLE_CCCD_BIT;
                app_ble_service_handle_rx_data(cmd_path, &p_br_link->cmd, p_data, data_len, p_br_link->id);
            }
        }
    }
    else if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION)
    {
        if (p_callback->attr_index == TRANSMIT_SVC_TX_DATA_CCCD_INDEX)
        {
            if (p_callback->msg_data.notification_indification_value == TRANSMIT_SVC_TX_DATA_CCCD_ENABLE)
            {
                if ((cmd_path == CMD_PATH_LE) && (p_le_link != NULL))
                {
                    p_le_link->cmd.tx_mask |= TX_ENABLE_CCCD_BIT;
                }
                else if ((cmd_path == CMD_PATH_GATT_OVER_BREDR) && (p_br_link != NULL))
                {
                    p_br_link->cmd.tx_mask |= TX_ENABLE_CCCD_BIT;
                }
                APP_PRINT_INFO0("app_ble_service_transmit_srv_cb: TRANSMIT_SVC_TX_DATA_CCCD_ENABLE");
            }
            else if (p_callback->msg_data.notification_indification_value == TRANSMIT_SVC_TX_DATA_CCCD_DISABLE)
            {
                if ((cmd_path == CMD_PATH_LE) && (p_le_link != NULL))
                {
                    p_le_link->cmd.tx_mask &= ~TX_ENABLE_CCCD_BIT;
                }
                else if ((cmd_path == CMD_PATH_GATT_OVER_BREDR) && (p_br_link != NULL))
                {
                    p_br_link->cmd.tx_mask &= ~TX_ENABLE_CCCD_BIT;
                }
                APP_PRINT_INFO0("app_ble_service_transmit_srv_cb: TRANSMIT_SVC_TX_DATA_CCCD_DISABLE");
            }
        }
    }

    return app_result;
}

static T_APP_RESULT app_ble_service_transmit_srv_dongle_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_APP_LE_LINK *p_link;
    T_TRANSMIT_SRV_DONGLE_CALLBACK_DATA *p_callback = (T_TRANSMIT_SRV_DONGLE_CALLBACK_DATA *)p_data;

    APP_PRINT_INFO2("app_ble_service_transmit_srv_dongle_cb: conn_handle %x, msg_type %d",
                    p_callback->conn_handle,
                    p_callback->msg_type);
    p_link = app_link_find_le_link_by_conn_id(p_callback->conn_id);
    if (p_link != NULL)
    {
        if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
        {
            if (p_callback->attr_index == TRANSMIT_SVC_DONGLE_RX_DATA_INDEX)
            {
                uint8_t         *p_data;
                uint16_t        data_len;

                p_data = p_callback->msg_data.rx_data.p_value;
                data_len = p_callback->msg_data.rx_data.len;
                app_ble_service_handle_rx_data(CMD_PATH_LE, &p_link->cmd, p_data, data_len, p_link->id);
            }
            else if (p_callback->attr_index == TRANSMIT_SVC_DONGLE_DEVICE_INFO_INDEX)
            {
#if F_APP_TEAMS_BLE_POLICY
                T_TRANSMIT_SRV_DONGLE_DEVICE_INFO *p_teams_ble_dev_info = &(p_callback->msg_data.device_info);
                app_teams_ble_policy_set_dev_bond_info(p_teams_ble_dev_info->bd_addr,
                                                       APP_TEAMS_BLE_POLICY_SET_DEVICE_TYPE, sizeof(p_teams_ble_dev_info->device_type),
                                                       &(p_teams_ble_dev_info->device_type));
                app_teams_ble_policy_set_dev_bond_info(p_teams_ble_dev_info->bd_addr,
                                                       APP_TEAMS_BLE_POLICY_SET_DEVICE_NAME, TRANSMIT_SVC_DEVICE_INFO_MAX_NAME_LEN,
                                                       p_teams_ble_dev_info->device_name);
#endif
                p_link->remote_device_type = p_callback->msg_data.device_info.device_type;

#if TARGET_LE_AUDIO_GAMING
                if (p_link->remote_device_type == DEVICE_TYPE_DONGLE)
                {
                    //It's no need to update gaming status to dongle.
#if F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT && TARGET_LE_AUDIO_GAMING
                    if (app_cfg_const.enable_24g_bt_audio_source_switch)
                    {
                        app_dongle_update_allowed_source();
                    }
#endif
                }
#else
                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    app_report_gaming_mode_info(app_db.gaming_mode);
                }

                app_lea_uca_pwr_on_enter_gaming_mode();
#endif
                APP_PRINT_TRACE1("app_ble_service_transmit_srv_dongle_cb: device type %d",
                                 p_link->remote_device_type);
            }
        }
        else if (p_callback->msg_type == SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION)
        {
            if (p_callback->attr_index == TRANSMIT_SVC_DONGLE_TX_DATA_CCCD_INDEX)
            {
                if (p_callback->msg_data.notification_indification_value == TRANSMIT_SVC_DONGLE_TX_DATA_CCCD_ENABLE)
                {
                    p_link->cmd.dongle_tx_mask |= TX_ENABLE_CCCD_BIT;
                    APP_PRINT_INFO0("app_ble_service_transmit_srv_dongle_cb: TRANSMIT_SVC_DONGLE_TX_DATA_CCCD_ENABLE");
                }
                else if (p_callback->msg_data.notification_indification_value ==
                         TRANSMIT_SVC_DONGLE_TX_DATA_CCCD_DISABLE)
                {
                    p_link->cmd.dongle_tx_mask &= ~TX_ENABLE_CCCD_BIT;
                    APP_PRINT_INFO0("app_ble_service_transmit_srv_dongle_cb: TRANSMIT_SVC_DONGLE_TX_DATA_CCCD_DISABLE");
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
    APP_PRINT_INFO3("app_ble_service_ota_srv_cb: service_id %d, msg_type %d, channel_type %d",
                    service_id, p_ota_cb_data->msg_type, p_ota_cb_data->chann_type);
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
                if ((p_ota_cb_data->chann_type == GAP_CHANN_TYPE_LE_ATT) ||
                    (p_ota_cb_data->chann_type == GAP_CHANN_TYPE_LE_ECFC))
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

#if F_APP_LOCAL_PLAYBACK_SUPPORT
static T_APP_RESULT app_ble_service_playback_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_PLAYBACK_CALLBACK_DATA *p_playback_cb_data = (T_PLAYBACK_CALLBACK_DATA *)p_data;
    APP_PRINT_INFO2("app_ble_service_playback_srv_cb: service_id %d, msg_type %d",
                    service_id, p_playback_cb_data->msg_type);
    switch (p_playback_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        break;
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        break;

    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            APP_PRINT_INFO0("app_ble_service_playback_srv_cb SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION");
        }
        break;

    default:

        break;
    }

    return app_result;
}
#endif

static T_APP_RESULT app_ble_service_bas_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_BAS_CALLBACK_DATA *p_bas_cb_data = (T_BAS_CALLBACK_DATA *)p_data;
    switch (p_bas_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            if (p_bas_cb_data->msg_data.notification_indification_index == BAS_NOTIFY_BATTERY_LEVEL_ENABLE)
            {
                APP_PRINT_INFO0("app_ble_service_bas_srv_cb: BAS_NOTIFY_BATTERY_LEVEL_ENABLE");
            }
            else if (p_bas_cb_data->msg_data.notification_indification_index ==
                     BAS_NOTIFY_BATTERY_LEVEL_DISABLE)
            {
                APP_PRINT_INFO0("app_ble_service_bas_srv_cb: BAS_NOTIFY_BATTERY_LEVEL_DISABLE");
            }
#if F_APP_LEA_SUPPORT
            T_APP_LE_LINK *p_link;

            p_link = app_link_find_le_link_by_conn_handle(p_bas_cb_data->conn_handle);
            if (p_link != NULL)
            {
                p_link->bas_report_batt = p_bas_cb_data->msg_data.notification_indification_index;
            }
#endif
        }
        break;

    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            /* Update battery level for bas using */
            bas_set_parameter(BAS_PARAM_BATTERY_LEVEL, 1, &app_db.local_batt_level);
            APP_PRINT_INFO1("app_ble_service_bas_srv_cb: local_batt_level %d", app_db.local_batt_level);
        }
        break;

    default:
        break;
    }

    return app_result;
}

static T_APP_RESULT app_ble_service_dis_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_DIS_CALLBACK_DATA *p_dis_cb_data = (T_DIS_CALLBACK_DATA *)p_data;
    switch (p_dis_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            if (p_dis_cb_data->msg_data.read_value_index == DIS_READ_FIRMWARE_REV_INDEX)
            {
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
                if (extend_app_cfg_const.gfps_support)
                {
                    dis_set_parameter(DIS_PARAM_FIRMWARE_REVISION,
                                      sizeof(extend_app_cfg_const.gfps_version),
                                      (void *)extend_app_cfg_const.gfps_version);
                }
                else
#endif
                {
                    const uint8_t DISFirmwareRev[] = "1.0.0";
                    dis_set_parameter(DIS_PARAM_FIRMWARE_REVISION,
                                      sizeof(DISFirmwareRev),
                                      (void *)DISFirmwareRev);
                }
            }
            else if (p_dis_cb_data->msg_data.read_value_index == DIS_READ_PNP_ID_INDEX)
            {
#if F_APP_BLE_HID_CONTROLLER_SUPPORT
                static uint8_t dis_pnp_id[DIS_PNP_ID_LENGTH] =
                {
                    // Vendor ID source (1=Bluetooth SIG)
                    1,
                    // Vendor ID (vendor-specific)
                    LO_WORD(BLE_HID_CONTROLLER_VENDOR_ID), HI_WORD(BLE_HID_CONTROLLER_VENDOR_ID),
                    // Product ID (vendor-specific)
                    LO_WORD(BLE_HID_CONTROLLER_PRODUCT_ID), HI_WORD(BLE_HID_CONTROLLER_PRODUCT_ID),
                    // Product version (vendor-assigned-JJ.M.N)
                    LO_WORD(BLE_HID_CONTROLLER_PRODUCT_VERSION), HI_WORD(BLE_HID_CONTROLLER_PRODUCT_VERSION)
                };

                dis_set_parameter(DIS_PARAM_PNP_ID, sizeof(dis_pnp_id), (void *)dis_pnp_id);
#endif
            }
        }
        break;

    default:
        break;
    }

    return app_result;
}

#if XM_XIAOAI_FEATURE_SUPPORT
static T_APP_RESULT app_ble_service_xm_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_XIAOAI_SERVICE_CALLBACK_DATA *p_xms_cb_data = (T_XIAOAI_SERVICE_CALLBACK_DATA *)p_data;
    switch (p_xms_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        {
            if (p_xms_cb_data->msg_data.write.opcode == XM_WRITE_RCSP_RX)
            {
                uint8_t bd_addr[6];
                uint8_t bd_type;
                le_get_conn_addr(p_xms_cb_data->conn_id, bd_addr, &bd_type);
                APP_PRINT_INFO2("app_ble_service_xm_srv_cb: bd_addr %s, bd_type %d", TRACE_BDADDR(bd_addr),
                                bd_type);
                T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(p_xms_cb_data->conn_id);
                if (p_link)
                {
                    APP_PRINT_INFO1("app_ble_service_xm_srv_cb: le link bd_addr %s", TRACE_BDADDR(p_link->bd_addr));
                    xm_rx_pkt(T_XM_CMD_COMMUNICATION_WAY_TYPE_BLE, p_link->bd_addr, p_link->conn_id,
                              p_xms_cb_data->msg_data.write.p_value,
                              p_xms_cb_data->msg_data.write.len);

                }
                else
                {
                    APP_PRINT_ERROR0("app_ble_service_xm_srv_cb: SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE can't find link");
                }
            }
        }
        break;
    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            if (p_xms_cb_data->msg_data.notification_indification_index == XIAOAI_CHAR_RCSP_TX_NOTIFY_ENABLE)
            {
                APP_PRINT_INFO0("app_ble_service_xm_srv_cb: XIAOAI_CHAR_RCSP_TX_NOTIFY_ENABLE");
                if (!app_cfg_const.enable_multi_link)
                {
                    le_xm_xiaoai_adv_stop();
                }
            }
            else if (p_xms_cb_data->msg_data.notification_indification_index ==
                     XIAOAI_CHAR_RCSP_TX_NOTIFY_DISABLE)
            {
                APP_PRINT_INFO0("app_ble_service_xm_srv_cb: XIAOAI_CHAR_RCSP_TX_NOTIFY_DISABLE");
            }
        }

    default:
        break;
    }

    return app_result;
}
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
static T_APP_RESULT app_ble_service_xiaowei_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_XIAOWEI_SERVICE_CALLBACK_DATA *p_xiaowei_cb_data = (T_XIAOWEI_SERVICE_CALLBACK_DATA *)p_data;

    uint8_t bd_addr[6];
    uint8_t bd_type;
    le_get_conn_addr(p_xiaowei_cb_data->conn_id, bd_addr, &bd_type);
    APP_PRINT_INFO2("app_ble_service_xiaowei_srv_cb: le_get_conn_addr bd_addr %s, bd_type %d",
                    TRACE_BDADDR(bd_addr), bd_type);

    switch (p_xiaowei_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        {
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(p_xiaowei_cb_data->conn_id);
            if (p_link)
            {
                T_XIAOWEI_LINK *p_xiaowei_link = app_xiaowei_find_le_link_by_addr(p_link->bd_addr);
                if (p_xiaowei_link && (p_xiaowei_link->auth_result == 0))
                {
                    xiaowei_set_access_auth_result(XIAOWEI_CONNECTION_TYPE_BLE, p_link->bd_addr,
                                                   p_xiaowei_cb_data->conn_id, true);
                }

                APP_PRINT_INFO1("app_ble_service_xiaowei_srv_cb: le link remote public addr %s",
                                TRACE_BDADDR(p_link->bd_addr));
                app_xiaowei_receive_data(XIAOWEI_CONNECTION_TYPE_BLE, p_link->bd_addr,
                                         p_link->conn_id, p_xiaowei_cb_data->msg_data.write.p_value, p_xiaowei_cb_data->msg_data.write.len);
            }
            else
            {
                APP_PRINT_ERROR0("app_ble_service_xiaowei_srv_cb: SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE can't find link");
            }
        }
        break;
    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            if (p_xiaowei_cb_data->msg_data.notification_indification_index == XIAOWEI_CHAR_TX_NOTIFY_ENABLE)
            {
                APP_PRINT_INFO0("app_ble_service_xiaowei_srv_cb: XIAOWEI_CHAR_TX_NOTIFY_ENABLE");

                if (!app_cfg_const.enable_multi_link)
                {
                    le_xiaowei_adv_stop();
                }
            }
            else if (p_xiaowei_cb_data->msg_data.notification_indification_index ==
                     XIAOWEI_CHAR_TX_NOTIFY_DISABLE)
            {
                APP_PRINT_INFO0("app_ble_service_xiaowei_srv_cb: XIAOWEI_CHAR_TX_NOTIFY_DISABLE");
            }
        }

    default:
        break;
    }

    return app_result;
}
#endif

#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
static T_APP_RESULT app_ble_service_asp_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_ASPS_CALLBACK_DATA *p_asp_cb_data = (T_ASPS_CALLBACK_DATA *)p_data;

    uint8_t bd_addr[6];
    uint8_t bd_type;
    //uint8_t app_asps_message = 0;
    //uint8_t app_asps_attn = 0;
    le_get_conn_addr(p_asp_cb_data->conn_id, bd_addr, &bd_type);
    uint8_t p_asp_ble_service_read_buffer[ASP_DEVICE_MAX_PKT_FOR_BLE] = {0};
    uint8_t asp_ble_pkt_size = 0;
    APP_PRINT_INFO2("app_ble_service_asp_srv_cb: le_get_conn_addr bd_addr %s, bd_type %d",
                    TRACE_BDADDR(bd_addr), bd_type);

    switch (p_asp_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            if (p_asp_cb_data->msg_data.read_value_index == GATT_SVC_ASPS_CHAR_MESSAGE_INDEX)
            {
                asp_ble_pkt_size = asp_device_api_get_ble_pkt_buf(p_asp_ble_service_read_buffer);
                if (asp_ble_pkt_size)
                {
                    APP_PRINT_INFO2("app_ble_service_asp_srv_cb: pkt_size %d, data %b", asp_ble_pkt_size,
                                    TRACE_BINARY(asp_ble_pkt_size, p_asp_ble_service_read_buffer));
                    asps_set_parameter(ASP_SERVICE_PARAM_MESSSAGE, asp_ble_pkt_size, p_asp_ble_service_read_buffer);
                }
                //asps_set_parameter(ASP_SERVICE_PARAM_MESSSAGE, 1, &app_asps_message);
            }
            else if (p_asp_cb_data->msg_data.read_value_index == GATT_SVC_ASPS_CHAR_ATTN_INDEX)
            {
                //asps_set_parameter(ASP_SERVICE_PARAM_ATTN, 1, &app_asps_attn);
            }
        }
        break;

    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        {
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(p_asp_cb_data->conn_id);
            if (p_link)
            {
                APP_PRINT_INFO1("app_ble_service_asp_srv_cb: le link bd_addr %s", TRACE_BDADDR(p_link->bd_addr));
                asp_device_api_receive_pkt(p_link->bd_addr, p_asp_cb_data->msg_data.write.len,
                                           p_asp_cb_data->msg_data.write.p_value);
            }
            else
            {
                APP_PRINT_ERROR0("app_ble_service_asp_srv_cb: SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE can't find link");
            }
        }
        break;
    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            if (p_asp_cb_data->msg_data.notification_indification_index == ASPS_NOTIFY_MESSAGE_STATUS_ENABLE)
            {
                APP_PRINT_INFO0("app_ble_service_asp_srv_cb: ASPS_NOTIFY_MESSAGE_STATUS_ENABLE");

                if (!app_cfg_const.enable_multi_link)
                {
                    //to do
                }
            }
            else if (p_asp_cb_data->msg_data.notification_indification_index ==
                     ASPS_NOTIFY_MESSAGE_STATUS_DISABLE)
            {
                APP_PRINT_INFO0("app_ble_service_asp_srv_cb: ASPS_NOTIFY_MESSAGE_STATUS_DISABLE");
            }
        }

    default:
        break;
    }

    return app_result;
}
#endif

#if F_APP_TUYA_SUPPORT
static T_APP_RESULT app_ble_service_tuya_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    T_TUYA_SERVICE_CALLBACK_DATA *p_tuya_cb_data = (T_TUYA_SERVICE_CALLBACK_DATA *)p_data;

    uint8_t bd_addr[6];
    uint8_t bd_type;
    le_get_conn_addr(p_tuya_cb_data->conn_id, bd_addr, &bd_type);
    APP_PRINT_INFO2("app_ble_service_tuya_srv_cb: le_get_conn_addr bd_addr %s, bd_type %d",
                    TRACE_BDADDR(bd_addr), bd_type);

    switch (p_tuya_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        {
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(p_tuya_cb_data->conn_id);
            if (p_link)
            {
                tuya_ble_status_t ret;
                ret = tuya_ble_gatt_receive_data(p_tuya_cb_data->msg_data.write.p_value,
                                                 p_tuya_cb_data->msg_data.write.len);
                APP_PRINT_INFO1("app_ble_service_tuya_srv_cb: SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE ret %d", ret);
            }
            else
            {
                APP_PRINT_ERROR0("app_ble_service_tuya_srv_cb: SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE can't find link");
            }
        }
        break;
    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            if (p_tuya_cb_data->msg_data.notification_indification_index == TUYA_CHAR_TX_NOTIFY_ENABLE)
            {
                APP_PRINT_INFO0("app_ble_service_tuya_srv_cb: TUYA_CHAR_TX_NOTIFY_ENABLE");
            }
            else if (p_tuya_cb_data->msg_data.notification_indification_index ==
                     TUYA_CHAR_TX_NOTIFY_DISABLE)
            {
                APP_PRINT_INFO0("app_ble_service_xiaowei_srv_cb: XIAOWEI_CHAR_TX_NOTIFY_DISABLE");
            }
        }
    case  SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            if (p_tuya_cb_data->msg_data.read_value_index == TUYA_CHAR_RX_READ_VALUE)
            {
                APP_PRINT_INFO0("app_ble_service_tuya_srv_cb: TUYA_CHAR_RX_READ_VALUE");
                //uint8_t value[5] = {'d', 'd', 'd', 'd', 'd'};
                //app_tuya_ble_set_read_value(value, 5); //for test
            }
        }
        break;
    default:
        break;
    }

    return app_result;
}
#endif

#if F_APP_CFU_BLE_CHANNEL_SUPPORT
static T_APP_RESULT app_ble_service_hids_cfu_srv_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_HID_CALLBACK_DATA *p_hid_cb_data = (T_HID_CALLBACK_DATA *)p_data;
    T_APP_CFU_DATA hid_msg = {0};
    uint8_t bd_addr[6];
    uint8_t bd_type;
    uint8_t conn_id = 0;
    le_get_conn_id_by_handle(p_hid_cb_data->conn_handle, &conn_id);
    le_get_conn_addr(conn_id, bd_addr, &bd_type);
    hid_msg.ble_data.conn_id = conn_id;
    APP_PRINT_INFO2("app_ble_service_hids_cfu_srv_cb: rcv hid msg, bd_addr %s, msg_type %d",
                    TRACE_BDADDR(bd_addr), p_hid_cb_data->msg_type);

    switch (p_hid_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            // use profile default value
        }
        break;

    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        {
            hid_msg.ble_data.report_type =
                p_hid_cb_data->msg_data.write_msg.write_parameter.report_data.report_type;
            hid_msg.ble_data.data_len = p_hid_cb_data->msg_data.write_msg.write_parameter.report_data.reportLen;
            hid_msg.ble_data.p_data = p_hid_cb_data->msg_data.write_msg.write_parameter.report_data.report;
            APP_PRINT_TRACE3("app_ble_service_hids_cfu_srv_cb: report id %d, report type %d, length %d",
                             p_hid_cb_data->msg_data.write_msg.write_parameter.report_data.report_id,
                             p_hid_cb_data->msg_data.write_msg.write_parameter.report_data.report_type,
                             p_hid_cb_data->msg_data.write_msg.write_parameter.report_data.reportLen);
            if (app_cfu_ble_header_parse(&hid_msg))
            {
                app_cfu_handle_report(p_hid_cb_data->msg_data.write_msg.write_parameter.report_data.report_id,
                                      APP_CFU_REPORT_SOURCE_BLE, &hid_msg);
            }

        }
        break;
    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            //not change cccd
        }

    default:
        break;
    }

    return app_result;
}
#endif

#if F_APP_GATT_OVER_BREDR_SUPPORT
T_APP_RESULT app_ble_service_rvdis_gatt_svc_cb(uint16_t conn_handle, uint16_t cid, uint8_t type,
                                               void *p_data)
{
    bool ret = true;
    if (type == GATT_MSG_RVDIS_SERVER_READ_CHAR_IND)
    {
        T_RVDIS_SERVER_READ_CHAR_IND *p_read = (T_RVDIS_SERVER_READ_CHAR_IND *)p_data;

        APP_PRINT_INFO3("app_ble_service_rvdis_gatt_svc_cb: service id %d, char_uuid 0x%x, offset %d",
                        p_read->service_id, p_read->char_uuid, p_read->offset);

        switch (p_read->char_uuid)
        {
        case GATT_UUID_CHAR_PREFERRED_GATT_TRANSPORT_TYPE:
            {
                uint8_t preferred_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_LE;

#if F_APP_GATT_OVER_BREDR_SUPPORT
                preferred_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_BREDR;
#endif
                ret = rvdis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid,
                                              p_read->offset, GATT_CHAR_PREFERRED_GATT_TRANSPORT_TYPE_LEN, &preferred_transport_type);
            }
            break;

        case GATT_UUID_CHAR_CURRENT_GATT_TRANSPORT_TYPE:
            {
                uint8_t current_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_LE;

                if (cid == L2C_FIXED_CID_ATT)
                {
                    current_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_LE;
                }
                else
                {
                    current_transport_type = RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_BREDR;
                }


                ret = rvdis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid,
                                              p_read->offset, GATT_CHAR_CURRENT_GATT_TRANSPORT_TYPE_LEN, &current_transport_type);
            }
            break;

        default:
            break;
        }
    }

    if (ret)
    {
        return APP_RESULT_SUCCESS;
    }
    else
    {
        return APP_RESULT_APP_ERR;
    }
}
#endif

void app_ble_service_init(void)
{
    /** NOTES: 4 includes transimit service, ota service, bas, dis service.
     *  transimit service dongle is added when use LEA.
     *  if more ble service are added, you need to modify this value.
     * */

#if F_APP_LEA_SUPPORT
    uint8_t server_num = 5;
#else
    uint8_t server_num = 4;
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        server_num++;
    }
#endif

#if F_APP_LOCAL_PLAYBACK_SUPPORT
    if (app_cfg_const.local_playback_support)
    {
        server_num++;
    }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
    server_num++;
#endif

#if BISTO_FEATURE_SUPPORT
    server_num += BISTO_GATT_SERVICE_NUM;
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    server_num++;
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
    server_num++;
#endif
#endif

#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
    server_num++;
#endif

#if F_APP_CFU_BLE_CHANNEL_SUPPORT
    server_num++;
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        server_num++;
    }
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
#if F_APP_FINDMY_USE_UARP
    server_num += 4;
#else
    server_num += 3;
#endif
#endif

#if F_APP_BLE_HID_CONTROLLER_SUPPORT
    server_num = server_num + 1;
#endif

#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT
    server_num += 3; //ASCS, PACS and CAS
#endif

#if F_APP_TMAP_BMR_SUPPORT
    server_num++; // BASS
#endif

#if F_APP_VCS_SUPPORT
    server_num++;
#if F_APP_AICS_SUPPORT
    server_num++;
#endif
#endif

#if F_APP_MICS_SUPPORT
    server_num++;
#endif

#if F_APP_CSIS_SUPPORT
    server_num++;
#endif

#if F_APP_TMAS_SUPPORT
    server_num++;
#endif

#if F_APP_HAS_SUPPORT
    server_num++;
#endif

#if F_APP_CHATGPT_SUPPORT
    server_num++;
#endif

#if F_APP_GATT_OVER_BREDR_SUPPORT
    server_num++;
#endif

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    server_cfg_use_ext_api(true);
    APP_PRINT_INFO0("app_ble_service_init: server_cfg_use_ext_api true");
    server_ext_register_app_cb(app_ble_service_general_srv_cb);
#else
    server_register_app_cb(app_ble_service_general_srv_cb);
#endif

    server_init(server_num);

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    gatt_svc_init(GATT_SVC_USE_EXT_SERVER, 0);
#else
    gatt_svc_init(GATT_SVC_USE_NORMAL_SERVER, 0);
#endif

#if F_APP_DURIAN_SUPPORT

#else
    transmit_srv_add(app_ble_service_transmit_srv_cb);

#if F_APP_LEA_SUPPORT
    transmit_srv_dongle_add(app_ble_service_transmit_srv_dongle_cb);
#endif

#if F_APP_GATT_OVER_BREDR_SUPPORT
    ota_add_service(app_ble_service_ota_srv_cb);
#else
    if (app_cfg_const.rtk_app_adv_support)
    {
        ota_add_service(app_ble_service_ota_srv_cb);
    }
#endif

#if F_APP_TEAMS_BT_POLICY
    teams_bas_id = bas_add_service(app_ble_service_bas_srv_cb);
#elif F_APP_LEA_SUPPORT
    lea_bas_id = bas_add_service(app_ble_service_bas_srv_cb);
#else
    bas_add_service(app_ble_service_bas_srv_cb);
#endif

    dis_add_service(app_ble_service_dis_srv_cb);//Add for GFPS

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        xiaoai_ble_service_add_service(app_ble_service_xm_srv_cb);
    }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_init();
        xiaowei_ble_service_add_service(app_ble_service_xiaowei_srv_cb);
    }
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        app_tuya_ble_service_add_service(app_ble_service_tuya_srv_cb);
    }
#endif

#if F_APP_LOCAL_PLAYBACK_SUPPORT
    if (app_cfg_const.local_playback_support)
    {
        playback_add_service(app_ble_service_playback_srv_cb);
    }
#endif

#if F_APP_CFU_BLE_CHANNEL_SUPPORT
    hids_add_service(app_ble_service_hids_cfu_srv_cb);
#endif

#if F_APP_GATT_OVER_BREDR_SUPPORT
    rvdis_add_service(app_ble_service_rvdis_gatt_svc_cb);
#endif

#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
    app_asp_device_init();
    app_teams_hid_init();
    app_teams_util_init();

#if F_APP_TEAMS_BT_POLICY
    app_teams_ext_ftl_init();
    app_teams_bt_policy_init();
#endif

    asps_add_service(app_ble_service_asp_srv_cb);
#if 0 //due to usb not support teams, so not init usb send data api
    teams_util_init(app_hid_interrupt_in, app_asp_device_send_pkt_by_ble);
#else
    teams_util_init(NULL, app_asp_device_send_pkt_by_ble);
#endif
    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
    {
        app_teams_rws_init();
    }
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
    app_findmy_init();
    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SINGLE)
    {
        app_findmy_relay_init();
    }
    app_findmy_adv_init();
#endif

#endif // F_APP_DURIAN_SUPPORT
}
