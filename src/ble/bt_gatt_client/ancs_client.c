#if F_APP_BLE_ANCS_CLIENT_SUPPORT
/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    ancs_client.c
  * @brief
  * @details
  * @author  jane
  * @date    2016-02-18
  * @version v1.0
  ******************************************************************************
  */
/** Add Includes here **/
#include <string.h>
#include "ancs_client.h"
#include "os_queue.h"
#include <trace.h>

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/
/**
 * @brief  ANCS Link control block definition.
 */


#define ANCS_UUID128_SRV 0xd0, 0x00, 0x2d, 0x12, 0x1e, 0x4b, 0x0f, 0xa4, 0x99, 0x4e, 0xce, 0xb5, 0x31, 0xf4, 0x05, 0x79
#define ANCS_UUID128_CHAR_CONTROL_POINT 0xd9, 0xd9, 0xaa, 0xfd, 0xbd, 0x9b, 0x21, 0x98, 0xa8, 0x49, 0xe1, 0x45, 0xf3, 0xd8, 0xd1, 0x69
#define ANCS_UUID128_CHAR_NOTIFICATION_SOURCE 0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C, 0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F
#define ANCS_UUID128_CHAR_DATA_SOURCE 0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE, 0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22
#define INVALID_HDL (0xff5f)

const T_ATTR_UUID ancs_uuid =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {ANCS_UUID128_SRV}
};


const T_ATTR_UUID ancs_char_ctrl_pnt =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {ANCS_UUID128_CHAR_CONTROL_POINT}
};

const T_ATTR_UUID ancs_char_noti_src =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {ANCS_UUID128_CHAR_NOTIFICATION_SOURCE}
};

const T_ATTR_UUID ancs_char_data_src =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {ANCS_UUID128_CHAR_DATA_SOURCE}
};



static struct
{
    ANCS_CLIENT_CB cb;
} ancs =
{
    .cb = NULL
};


/**
  * @brief  Used by application, to set the notifcation flag of notification source.
  * @param[in]  conn_id connection ID.
  * @param[in]  notify  value to enable or disable notify.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool ancs_set_notification_source_notify(uint16_t conn_handle, bool notify)
{
    gatt_client_enable_char_cccd(conn_handle, (T_ATTR_UUID *)&ancs_uuid,
                                 (T_ATTR_UUID *)&ancs_char_noti_src, notify);

    return true;
}

/**
  * @brief  Used by application, to set the notifcation flag of data source.
  * @param[in]  conn_id connection ID.
  * @param[in]  notify  value to enable or disable notify.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool ancs_set_data_source_notify(uint16_t conn_handle, bool notify)
{
    gatt_client_enable_char_cccd(conn_handle, (T_ATTR_UUID *)&ancs_uuid,
                                 (T_ATTR_UUID *)&ancs_char_data_src, notify);

    return true;
}


/**
  * @brief  Used by application, to get the notifcation attribute.
  * @param[in]  conn_id           connection ID.
  * @param[in]  notification_uid  value to enable or disable notify.
  * @param[in]  p_attribute_ids   Pointer to attribute ids.
  * @param[in]  attribute_ids_len Length of attribute ids.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool ancs_get_notification_attr(uint16_t conn_handle, uint32_t notification_uid,
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

    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ancs_uuid,
                                     (T_ATTR_UUID *)&ancs_char_ctrl_pnt, &handle))
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

/**
  * @brief  Used by application, to get the app attribute.
  * @param[in]  conn_id           connection ID.
  * @param[in]  p_app_identifier  value to enable or disable notify.
  * @param[in]  p_attribute_ids   Pointer to attribute ids.
  * @param[in]  attribute_ids_len Length of attribute ids.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool ancs_get_app_attr(uint16_t conn_handle, char *p_app_identifier, uint8_t *p_attribute_ids,
                       uint8_t attribute_ids_len)
{
    bool result = false;
    uint16_t handle = INVALID_HDL;
    uint8_t command_id = CP_CMD_ID_GET_APP_ATTR;
    uint16_t length = sizeof(command_id) + strlen((const char *)p_app_identifier) + attribute_ids_len +
                      1;
    uint8_t *p_buffer = NULL;
    uint16_t offset = 0;
    p_buffer = os_mem_zalloc(RAM_TYPE_DATA_ON, length);
    if (!p_buffer)
    {
        PROFILE_PRINT_ERROR2("ancs_get_app_attr: allocation failed, conn_handle %04x, length %d",
                             conn_handle,
                             length);
        return false;
    }
    memcpy(p_buffer + offset, &command_id, sizeof(command_id));
    offset += sizeof(command_id);
    memcpy(p_buffer + offset, p_app_identifier, strlen((const char *)p_app_identifier));
    offset += strlen((const char *)p_app_identifier);
    p_buffer[offset] = 0;
    offset += 1;
    memcpy(p_buffer + offset, p_attribute_ids, attribute_ids_len);

    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ancs_uuid,
                                     (T_ATTR_UUID *)&ancs_char_ctrl_pnt, &handle))
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
        os_mem_free(p_buffer);
    }

    return result;
}

/**
  * @brief  Used by application, to perfome the notication action.
  * @param[in]  conn_id          Connection ID.
  * @param[in]  notification_uid Notification UUID.
  * @param[in]  action_id        Action id.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool ancs_perform_notification_action(uint16_t conn_handle, uint32_t notification_uid,
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

    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ancs_uuid,
                                     (T_ATTR_UUID *)&ancs_char_ctrl_pnt, &handle))
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


static T_APP_RESULT client_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                              void *p_data)
{
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;
    T_ANCS_CB_DATA cb_data = {};
    PROTOCOL_PRINT_INFO2("ancs_client client_cb: conn_handle %04x, type %d", conn_handle, type);

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            PROTOCOL_PRINT_INFO1("ancs_client client_cb: is_found %d",
                                 p_client_cb_data->dis_done.is_found);

            cb_data.cb_type = ANCS_CLIENT_CB_TYPE_DISC_STATE;
            cb_data.cb_content.disc_state.is_found = p_client_cb_data->dis_done.is_found;
            cb_data.cb_content.disc_state.load_from_ftl = p_client_cb_data->dis_done.load_from_ftl;

        }
        break;
    case GATT_CLIENT_EVENT_READ_RESULT:
        break;
    case GATT_CLIENT_EVENT_WRITE_RESULT:
        PROTOCOL_PRINT_INFO3("ancs_client client_cb: handle 0x%04x, char_type %d, uuid128 %s",
                             p_client_cb_data->write_result.handle, p_client_cb_data->write_result.char_type,
                             TRACE_BINARY(16, p_client_cb_data->write_result.char_uuid.p.uuid128));
        cb_data.cb_type = ANCS_CLIENT_CB_TYPE_WRITE_RESULT;
        cb_data.cb_content.write_result.cause = p_client_cb_data->write_result.cause;
        if (p_client_cb_data->write_result.char_uuid.is_uuid16 == false)
        {
            if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ancs_char_ctrl_pnt.p.uuid128,
                       16) == 0)
            {
                cb_data.cb_content.write_result.type = ANCS_WRITE_CONTROL_POINT;
            }
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:
        PROTOCOL_PRINT_INFO2("ancs_client client_cb: handle 0x%04x, uuid128 %s",
                             p_client_cb_data->notify_ind.handle, TRACE_BINARY(16,
                                                                               p_client_cb_data->notify_ind.char_uuid.p.uuid128));
        cb_data.cb_type = ANCS_CLIENT_CB_TYPE_NOTIF_IND_RESULT;
        if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ancs_char_noti_src.p.uuid128,
                   16) == 0)
        {
            cb_data.cb_content.notify_data.type = ANCS_FROM_NOTIFICATION_SOURCE;
        }
        else if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ancs_char_data_src.p.uuid128,
                        16) == 0)
        {
            cb_data.cb_content.notify_data.type = ANCS_FROM_DATA_SOURCE;
        }
        cb_data.cb_content.notify_data.p_value = p_client_cb_data->notify_ind.p_value;
        cb_data.cb_content.notify_data.value_size = p_client_cb_data->notify_ind.value_size;
        break;

    case GATT_CLIENT_EVENT_CCCD_CFG:
        PROTOCOL_PRINT_INFO2("ancs_client client_cb: uuid128 %s, cccd_cfg %d",
                             TRACE_BINARY(16, p_client_cb_data->cccd_cfg.char_uuid.p.uuid128),
                             p_client_cb_data->cccd_cfg.cccd_cfg);
        cb_data.cb_type = ANCS_CLIENT_CB_TYPE_WRITE_RESULT;
        if (memcmp(p_client_cb_data->cccd_cfg.char_uuid.p.uuid128, ancs_char_noti_src.p.uuid128,
                   16) == 0)
        {
            if (p_client_cb_data->cccd_cfg.cccd_cfg)
            {
                cb_data.cb_content.write_result.type = ANCS_WRITE_NOTIFICATION_SOURCE_NOTIFY_ENABLE;
            }
            else
            {
                cb_data.cb_content.write_result.type = ANCS_WRITE_NOTIFICATION_SOURCE_NOTIFY_DISABLE;
            }
        }
        else if (memcmp(p_client_cb_data->cccd_cfg.char_uuid.p.uuid128, ancs_char_data_src.p.uuid128,
                        16) == 0)
        {
            if (p_client_cb_data->cccd_cfg.cccd_cfg)
            {
                cb_data.cb_content.write_result.type = ANCS_WRITE_DATA_SOURCE_NOTIFY_ENABLE;
            }
            else
            {
                cb_data.cb_content.write_result.type = ANCS_WRITE_DATA_SOURCE_NOTIFY_DISABLE;
            }
        }
        break;
    case GATT_CLIENT_EVENT_CONN_DEL:
        cb_data.cb_type = ANCS_CLIENT_CB_TYPE_DISCONNECT_INFO;
        break;

    default:
        break;
    }


    /* Send discover state to application if needed. */
    if (ancs.cb)
    {
        (*ancs.cb)(conn_handle, &cb_data);
    }

    return APP_RESULT_SUCCESS;
}


/**
 * @brief       Add ancs client.
 *
 * @param[in]   app_cb  Callbackto notify client read/write/notify/indicate events.
 * @param[in]   link_num Initialize link number
 * @return Client ID of the specific client module.
 * @retval 0xff failed.
 * @retval other success.
 *
 * <b>Example usage</b>
 * \code{.c}
    void ancs_init(void)
    {
        ancs_client = ancs_client_init(ancs.cb, link_num);
    }
 * \endcode
 */
bool ancs_client_init(ANCS_CLIENT_CB app_cb)
{
    if (gatt_client_spec_register((T_ATTR_UUID *)&ancs_uuid, client_cb) != GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        return false;
    }

    /* register callback for profile to inform application that some events happened. */
    ancs.cb = app_cb;

    return true;
}
#endif
