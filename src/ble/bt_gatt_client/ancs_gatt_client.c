/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    ancs_gatt_client.c
  * @brief   Head file for using ANCS client.
  * @details ASCS client data structs and external functions declaration
  *          developed based on bt_gatt_client.h.
  * @author  jane
  * @date    2016-02-18
  * @version v1.0
  ******************************************************************************
  */

/** Add Includes here **/
#include <stdlib.h>
#include <string.h>
#include <trace.h>
#include "ancs_gatt_client.h"

#define ANCS_UUID128_SRV 0xd0, 0x00, 0x2d, 0x12, 0x1e, 0x4b, 0x0f, 0xa4, 0x99, 0x4e, 0xce, 0xb5, 0x31, 0xf4, 0x05, 0x79
#define ANCS_UUID128_CHAR_CONTROL_POINT 0xd9, 0xd9, 0xaa, 0xfd, 0xbd, 0x9b, 0x21, 0x98, 0xa8, 0x49, 0xe1, 0x45, 0xf3, 0xd8, 0xd1, 0x69
#define ANCS_UUID128_CHAR_NOTIFICATION_SOURCE 0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C, 0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F
#define ANCS_UUID128_CHAR_DATA_SOURCE 0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE, 0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22
#define INVALID_HDL (0xff5f)

const T_ATTR_UUID ascs_srv_uuid =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {ANCS_UUID128_SRV}
};

const T_ATTR_UUID ancs_char_cp_uuid =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {ANCS_UUID128_CHAR_CONTROL_POINT}
};

const T_ATTR_UUID ancs_char_notification_src_uuid =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {ANCS_UUID128_CHAR_NOTIFICATION_SOURCE}
};

const T_ATTR_UUID ancs_char_data_src_uuid =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {ANCS_UUID128_CHAR_DATA_SOURCE}
};

/**<  Callback used to send data to app from ANCS client layer. */
static P_FUN_ANCS_CLIENT_APP_CB ancs_client_cb = NULL;

bool ancs_client_cfg_cccd(uint16_t conn_handle, T_ANCS_SOURCE type, bool enable)
{
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    T_ATTR_UUID *p_char_uuid = (T_ATTR_UUID *)&ancs_char_notification_src_uuid;
    uint8_t cccd_cfg;

    if (type == ANCS_DATA_SOURCE)
    {
        p_char_uuid = (T_ATTR_UUID *)&ancs_char_data_src_uuid;
    }

    if (enable)
    {
        cccd_cfg = GATT_CLIENT_CONFIG_ALL;
    }
    else
    {
        cccd_cfg = GATT_CLIENT_CONFIG_DISABLE;
    }

    if (gatt_client_check_cccd_enabled(conn_handle, (T_ATTR_UUID *)&ascs_srv_uuid,
                                       p_char_uuid) != enable)
    {
        cause = gatt_client_enable_char_cccd(conn_handle, (T_ATTR_UUID *)&ascs_srv_uuid,
                                             p_char_uuid, cccd_cfg);
    }

    return (cause == GAP_CAUSE_SUCCESS) ? true : false;
}

bool ancs_client_cp_get_notification_attr(uint16_t conn_handle, uint32_t notification_uid,
                                          uint8_t *p_attribute_ids, uint8_t attribute_ids_len)
{
    bool result = false;
    uint16_t handle = INVALID_HDL;
    uint8_t command_id = CP_CMD_ID_GET_NOTIFICATION_ATTR;
    uint16_t length = sizeof(command_id) + sizeof(notification_uid) + attribute_ids_len;

    if (attribute_ids_len > 25)
    {
        return false;
    }

    uint8_t buffer[30];
    uint16_t offset = 0;
    memcpy(buffer + offset, &command_id, sizeof(command_id));
    offset += sizeof(command_id);
    memcpy(buffer + offset, &notification_uid, sizeof(notification_uid));
    offset += sizeof(notification_uid);
    memcpy(buffer + offset, p_attribute_ids, attribute_ids_len);

    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ascs_srv_uuid,
                                     (T_ATTR_UUID *)&ancs_char_cp_uuid, &handle))
    {
        T_GAP_CAUSE cause = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle, length, buffer,
                                              NULL);
        if (cause == GAP_CAUSE_SUCCESS)
        {
            result = true;
        }
    }

    return result;
}

bool ancs_client_cp_get_app_attr(uint16_t conn_handle, char *p_app_identifier,
                                 uint8_t *p_attribute_ids, uint8_t attribute_ids_len)
{
    bool result = false;
    uint16_t handle = INVALID_HDL;
    uint8_t command_id = CP_CMD_ID_GET_APP_ATTR;
    uint16_t length = sizeof(command_id) + strlen((const char *)p_app_identifier) + attribute_ids_len +
                      1;
    uint8_t *p_buffer = NULL;
    uint16_t offset = 0;
    p_buffer = calloc(1, length);
    if (!p_buffer)
    {
        APP_PRINT_ERROR2("ancs_client_cp_get_app_attr: allocation failed, conn_handle %04x, length %d",
                         conn_handle,  length);
        return false;
    }
    memcpy(p_buffer + offset, &command_id, sizeof(command_id));
    offset += sizeof(command_id);
    memcpy(p_buffer + offset, p_app_identifier, strlen((const char *)p_app_identifier));
    offset += strlen((const char *)p_app_identifier);
    p_buffer[offset] = 0;
    offset += 1;
    memcpy(p_buffer + offset, p_attribute_ids, attribute_ids_len);

    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ascs_srv_uuid,
                                     (T_ATTR_UUID *)&ancs_char_cp_uuid, &handle))
    {
        T_GAP_CAUSE cause = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle, length, p_buffer,
                                              NULL);
        if (cause == GAP_CAUSE_SUCCESS)
        {
            result = true;
        }
    }

    if (p_buffer != NULL)
    {
        free(p_buffer);
    }

    return result;
}

bool ancs_client_cp_perform_notification_action(uint16_t conn_handle, uint32_t notification_uid,
                                                uint8_t action_id)
{
    bool result = false;
    uint16_t handle = INVALID_HDL;
    uint8_t command_id = CP_CMD_ID_PERFORM_NOTIFICATION_ACTION;
    uint16_t length = sizeof(command_id) + sizeof(notification_uid) + sizeof(action_id);

    uint8_t buffer[12];
    uint16_t offset = 0;
    memcpy(buffer + offset, &command_id, sizeof(command_id));
    offset += sizeof(command_id);
    memcpy(buffer + offset, &notification_uid, sizeof(notification_uid));
    offset += sizeof(notification_uid);
    memcpy(buffer + offset, &action_id, sizeof(action_id));

    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ascs_srv_uuid,
                                     (T_ATTR_UUID *)&ancs_char_cp_uuid, &handle))
    {
        T_GAP_CAUSE cause = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle, length, buffer,
                                              NULL);
        if (cause == GAP_CAUSE_SUCCESS)
        {
            result = true;
        }
    }

    return result;
}

T_APP_RESULT ancs_client_cbs(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            T_ANCS_CLIENT_DIS_DONE dis_done = {0};
            dis_done.is_found = p_client_cb_data->dis_done.is_found;
            dis_done.load_from_ftl = p_client_cb_data->dis_done.load_from_ftl;

            if (ancs_client_cb)
            {
                result = (* ancs_client_cb)(conn_handle, GATT_MSG_ANCS_CLIENT_DIS_DONE, &dis_done);
            }
        }
        break;

    case GATT_CLIENT_EVENT_WRITE_RESULT:
        {
            T_ANCS_CLIENT_WRITE_CP_RESULT write_result = {0};
            write_result.cause = p_client_cb_data->write_result.cause;

            if (ancs_client_cb)
            {
                result = (* ancs_client_cb)(conn_handle, GATT_MSG_ANCS_CLIENT_WRITE_CP_RESULT, &write_result);
            }
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:
        {
            if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128,
                       ancs_char_notification_src_uuid.p.uuid128,
                       16) == 0)
            {
                T_ANCS_CLIENT_NOTIFICATION_SOURCE notify_data = {0};
                if (sizeof(T_ANCS_CLIENT_NOTIFICATION_SOURCE) == p_client_cb_data->notify_ind.value_size)
                {
                    memcpy(&notify_data, p_client_cb_data->notify_ind.p_value, p_client_cb_data->notify_ind.value_size);
                    if (ancs_client_cb)
                    {
                        result = (* ancs_client_cb)(conn_handle, GATT_MSG_ANCS_CLIENT_NOTIFICATION_SOURCE, &notify_data);
                    }
                }
            }
            else if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128,
                            ancs_char_data_src_uuid.p.uuid128,
                            16) == 0)
            {
                T_ANCS_CLIENT_DATA_SOURCE notify_data = {0};
                notify_data.p_value = p_client_cb_data->notify_ind.p_value;
                notify_data.value_size = p_client_cb_data->notify_ind.value_size;

                if (ancs_client_cb)
                {
                    result = (* ancs_client_cb)(conn_handle, GATT_MSG_ANCS_CLIENT_DATA_SOURCE, &notify_data);
                }
            }
        }
        break;

    case GATT_CLIENT_EVENT_CCCD_CFG:
        {
            T_ANCS_CLIENT_CCCD_CFG_RESULT cccd_info = {(T_ANCS_SOURCE)0};

            cccd_info.cause = p_client_cb_data->cccd_cfg.cause;
            if (p_client_cb_data->cccd_cfg.cccd_cfg)
            {
                cccd_info.enable = true;
            }

            if (memcmp(p_client_cb_data->cccd_cfg.char_uuid.p.uuid128,
                       ancs_char_notification_src_uuid.p.uuid128,
                       16) == 0)
            {
                cccd_info.type = ANCS_NOTIFICATION_SOURCE;
            }
            else if (memcmp(p_client_cb_data->cccd_cfg.char_uuid.p.uuid128, ancs_char_data_src_uuid.p.uuid128,
                            16) == 0)
            {
                cccd_info.type = ANCS_DATA_SOURCE;
            }

            if (ancs_client_cb)
            {
                result = (* ancs_client_cb)(conn_handle, GATT_MSG_ANCS_CLIENT_CCCD_CFG_RESULT, &cccd_info);
            }
        }
        break;

    case GATT_CLIENT_EVENT_CONN_DEL:
        {
            T_ANCS_CLIENT_CONN_DEL conn_del = {0};
            conn_del.transport_le = p_client_cb_data->conn_del.transport_le;
            if (ancs_client_cb)
            {
                result = (* ancs_client_cb)(conn_handle, GATT_MSG_ANCS_CLIENT_CONN_DEL, &conn_del);
            }
        }
        break;

    default:
        break;
    }

    return result;
}

bool ancs_client_init(P_FUN_ANCS_CLIENT_APP_CB app_cb)
{
    if (gatt_client_spec_register((T_ATTR_UUID *)&ascs_srv_uuid, ancs_client_cbs) == GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        ancs_client_cb = app_cb;
        return true;
    }

    return false;
}

