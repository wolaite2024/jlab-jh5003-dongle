/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    transmit_gatt_client.c
  * @brief   transmit gatt client source file.
  * @details
  * @author  cen
  * @date    2024-05-31
  * @version v1.1
  ******************************************************************************
  */

#if TRANSMIT_CLIENT_SUPPORT
/** Add Includes here **/
#include <string.h>
#include "trace.h"
#include "gap_msg.h"
#include "transmit_gatt_client.h"

/** @brief  Transmit Service related UUIDs. */
#define TRANSMIT_SVC_CHAR_UUID_RX_DATA         0xFD03
#define TRANSMIT_SVC_CHAR_UUID_TX_DATA         0xFD04

uint8_t GATT_UUID128_TRANSMIT_SRV[16] = { 0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFD, 0x02, 0x00, 0x00};
/**<  Callback used to send data to app from transmit client layer. */
static P_FUN_CLIENT_GENERAL_APP_CB transmit_client_cb = NULL;

/**
  * @brief  Used by application, to write Characteristic.
  * @param[in]  conn_handle  connection handle.
  * @param[in]  length  write data length.
  * @param[in]  p_value point the value to write.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool transmit_client_write_char(uint16_t conn_handle, uint16_t length, uint8_t *p_value)
{
    T_ATTR_UUID srv_uuid;
    T_ATTR_UUID char_uuid;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint16_t handle = 0;
    T_GATT_WRITE_TYPE type = GATT_WRITE_TYPE_CMD;

    srv_uuid.is_uuid16 = false;
    srv_uuid.instance_id = 0;
    memcpy(srv_uuid.p.uuid128, GATT_UUID128_TRANSMIT_SRV, 16);
    char_uuid.is_uuid16 = true;
    char_uuid.instance_id = 0;
    char_uuid.p.uuid16 = TRANSMIT_SVC_CHAR_UUID_RX_DATA;

    if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
    {
        cause = gatt_client_write(conn_handle, type, handle, length, p_value, NULL);
    }

    if (cause == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool transmit_data_write(uint16_t conn_handle, uint8_t *p_transmit_data, uint16_t data_len)
{
    APP_PRINT_INFO2("transmit_data_write, conn_handle 0x%x, data_len 0x%x", conn_handle, data_len);

    if (p_transmit_data == NULL)
    {
        return false;
    }

    if (transmit_client_write_char(conn_handle, data_len, p_transmit_data))
    {
        return true;
    }
    return false;
}

bool transmit_client_enable_cccd(uint16_t conn_handle)
{
    T_ATTR_UUID srv_uuid;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    srv_uuid.is_uuid16 = false;
    srv_uuid.instance_id = 0;
    memcpy(srv_uuid.p.uuid128, GATT_UUID128_TRANSMIT_SRV, 16);

    cause = gatt_client_enable_srv_cccd(conn_handle, &srv_uuid, GATT_CLIENT_CONFIG_ALL);
    if (cause == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}


/**
 * @brief transmit BLE Client Callbacks.
*/
T_APP_RESULT transmit_client_callback(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;
    T_TRANSMIT_CLT_CB_DATA cb_data;
    memset(&cb_data, 0, sizeof(T_TRANSMIT_CLT_CB_DATA));

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            PROTOCOL_PRINT_INFO1("[TRANSMIT] GATT_CLIENT_EVENT_DIS_DONE: is_found %d",
                                 p_client_cb_data->dis_done.is_found);
            cb_data.cb_type = TRANSMIT_CLIENT_CB_TYPE_DISC_DONE;
            cb_data.cb_content.disc_done.conn_handle = conn_handle;
            cb_data.cb_content.disc_done.is_found = p_client_cb_data->dis_done.is_found;
            cb_data.cb_content.disc_done.load_from_ftl = p_client_cb_data->dis_done.load_from_ftl;

            if (cb_data.cb_content.disc_done.is_found)
            {
                if (cb_data.cb_content.disc_done.load_from_ftl == false)
                {
                    transmit_client_enable_cccd(conn_handle);
                }
            }
        }
        break;

    case GATT_CLIENT_EVENT_WRITE_RESULT:
        {
            PROTOCOL_PRINT_INFO3("[TRANSMIT] GATT_CLIENT_EVENT_WRITE_RESULT: conn_handle 0x%x, cause 0x%x, uuid 0x%x",
                                 conn_handle, p_client_cb_data->write_result.cause,
                                 p_client_cb_data->write_result.char_uuid.p.uuid16);
            if (p_client_cb_data->write_result.char_type == GATT_CHAR_VALUE &&
                p_client_cb_data->write_result.char_uuid.p.uuid16 == TRANSMIT_SVC_CHAR_UUID_RX_DATA)
            {
                cb_data.cb_type = TRANSMIT_CLIENT_CB_TYPE_WRITE_RESULT;
                cb_data.cb_content.write_result.type = TRANSMIT_WRITE_DATA;
                cb_data.cb_content.write_result.cause = p_client_cb_data->write_result.cause;
            }
        }
        break;

    case GATT_CLIENT_EVENT_CCCD_CFG:
        {
            PROTOCOL_PRINT_INFO3("[TRANSMIT] GATT_CLIENT_EVENT_CCCD_CFG: cause 0x%x, cccd_cfg %d, srv_instance_id %d",
                                 p_client_cb_data->cccd_cfg.cause,
                                 p_client_cb_data->cccd_cfg.cccd_cfg,
                                 p_client_cb_data->cccd_cfg.srv_instance_id);

            cb_data.cb_type = TRANSMIT_CLIENT_CB_TYPE_CCCD_CFG;
            cb_data.cb_content.cccd_cfg.cause = p_client_cb_data->cccd_cfg.cause;
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:
        {
            PROTOCOL_PRINT_INFO7("[TRANSMIT] GATT_CLIENT_EVENT_NOTIFY_IND: conn_handle 0x%x, srv_instance_id %d, uuid16[%d] 0x%x, handle 0x%x, data[%d] %b",
                                 conn_handle,
                                 p_client_cb_data->notify_ind.srv_instance_id,
                                 p_client_cb_data->notify_ind.char_uuid.instance_id,
                                 p_client_cb_data->notify_ind.char_uuid.p.uuid16,
                                 p_client_cb_data->notify_ind.handle,
                                 p_client_cb_data->notify_ind.value_size,
                                 TRACE_BINARY(p_client_cb_data->notify_ind.value_size,
                                              p_client_cb_data->notify_ind.p_value));

            cb_data.cb_type = TRANSMIT_CLIENT_CB_TYPE_NOTIFY_IND;
            cb_data.cb_content.notify_ind.conn_handle = p_client_cb_data->notify_ind.handle;
            cb_data.cb_content.notify_ind.p_value = p_client_cb_data->notify_ind.p_value;
            cb_data.cb_content.notify_ind.value_size = p_client_cb_data->notify_ind.value_size;
        }
        break;
    default:
        break;
    }

    if (transmit_client_cb)
    {
        (*transmit_client_cb)(conn_handle, GATT_UUID128_TRANSMIT_SRV, &cb_data);
    }

    return result;
}


/**
  * @brief      Add transmit service client to application.
  * @param[in]  app_cb pointer of app callback function to handle specific client module data.
  * @param[in]  link_num initialize link num.
  * @return Client ID of the specific client module.
  * @retval 0xff failed.
  * @retval other success.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        client_init(1);
        transmit_client_id = transmit_ble_add_client(app_client_callback, APP_MAX_LINKS);
    }
  * \endcode
  */
bool transmit_client_init(P_FUN_CLIENT_GENERAL_APP_CB app_cb)
{
    T_ATTR_UUID srv_uuid = {0};
    srv_uuid.is_uuid16 = false;
    memcpy(srv_uuid.p.uuid128, GATT_UUID128_TRANSMIT_SRV, 16);

    if (gatt_client_spec_register(&srv_uuid, transmit_client_callback) == GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        transmit_client_cb = app_cb;
        return true;
    }

    return false;
}
#endif
