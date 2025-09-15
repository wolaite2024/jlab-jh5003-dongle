#if F_APP_BLE_AMS_CLIENT_SUPPORT
/**
*****************************************************************************************
*     Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    ams_client.c
  * @brief
  * @details
  * @author
  * @date
  * @version v1.0
  ***************************************************************************************
*/
#include <string.h>
#include "ams_client.h"
#include "bt_gatt_client.h"
#include "os_mem.h"
#include "trace.h"

/**
 * @brief  AMS Link control block defination.
 */
typedef struct
{
    T_AMS_DISC_STATE    disc_state;
    bool                remote_cmd_notify;
    bool                entity_upd_notify;
    uint16_t            hdl_cache[AMS_HDL_CACHE_LEN];
} T_AMS_LINK, *P_AMS_LINK;

//AMS service UUID 89D3502B-0F36-433A-8EF4-C502AD55F8DC
#define AMS_SRV_UUID128 0xdc, 0xf8, 0x55, 0xad, 0x02, 0xc5, 0xf4, 0x8e, 0x3a, 0x43, 0x36, 0x0f, 0x2b, 0x50, 0xd3, 0x89
//Remote Command UUID 9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2 (writeable, notifiable)
#define AMS_CHAR_REMOTE_CMD_UUID128 0xc2, 0x51, 0xca, 0xf7, 0x56, 0x0e, 0xdf, 0xb8, 0x8a, 0x4a, 0xb1, 0x57, 0xd8, 0x81, 0x3c, 0x9b
//Entity Update UUID 2F7CABCE-808D-411F-9A0C-BB92BA96C102 (writeable with response, notifiable)
#define AMS_CHAR_ENTITY_UPD_UUID128 0x02, 0xc1, 0x96, 0xba, 0x92, 0xbb, 0x0c, 0x9a, 0x1f, 0x41, 0x8d, 0x80, 0xce, 0xab, 0x7c, 0x2f
//Entity Attribute UUID C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7 (readable, writeable)
#define AMS_CHAR_ENTITY_ATTR_UUID128 0xd7, 0xd5, 0xbb, 0x70, 0xa8, 0xa3, 0xab, 0xa6, 0xd8, 0x46, 0xab, 0x23, 0x8c, 0xf3, 0xb2, 0xc6

#define INVALID_HDL (0xffff)

const T_ATTR_UUID ams_uuid =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {AMS_SRV_UUID128},
};


const T_ATTR_UUID ams_char_rmt_cmd =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {AMS_CHAR_REMOTE_CMD_UUID128},
};


const T_ATTR_UUID ams_char_entity_upd =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {AMS_CHAR_ENTITY_UPD_UUID128},
};


const T_ATTR_UUID ams_char_entity_attr =
{
    .is_uuid16 = false,
    .instance_id = 0,
    .p.uuid128 = {AMS_CHAR_ENTITY_ATTR_UUID128},
};


static struct
{
    AMS_CLIENT_CB cb;
} ams =
{
    .cb = NULL,
};


/**
 * @brief  AMS remote command subscribe.
 */
bool ams_subscribe_remote_cmd(uint16_t conn_handle, bool subscribe)
{
    gatt_client_enable_char_cccd(conn_handle, (T_ATTR_UUID *)&ams_uuid,
                                 (T_ATTR_UUID *)&ams_char_rmt_cmd, subscribe);

    return true;
}

/**
 * @brief  AMS write remote command.
 */
bool ams_write_remote_cmd(uint16_t conn_handle, T_AMS_REMOTE_CMD_ID cmd_id)
{
    uint16_t handle = INVALID_HDL;
    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ams_uuid,
                                     (T_ATTR_UUID *)&ams_char_rmt_cmd, &handle))
    {
        T_GAP_CAUSE cause = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle, 1,
                                              (uint8_t *)&cmd_id, NULL);
    }

    return true;
}


/**
 * @brief  AMS entity update subscribe.
 */
bool ams_subscribe_entity_upd(uint16_t conn_handle, bool subscribe)
{
    gatt_client_enable_char_cccd(conn_handle, (T_ATTR_UUID *)&ams_uuid,
                                 (T_ATTR_UUID *)&ams_char_entity_upd, subscribe);
    return false;
}

/**
 * @brief  AMS write entity update command.
    entity_attr_id, first byte is entity id, the other bytes are attribute id list.
 */
bool ams_write_entity_upd_cmd(uint16_t conn_handle, uint8_t *p_value, uint8_t value_len)
{
    uint16_t handle = INVALID_HDL;
    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ams_uuid,
                                     (T_ATTR_UUID *)&ams_char_entity_upd, &handle))
    {
        T_GAP_CAUSE cause = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle, value_len, p_value,
                                              NULL);
        if (cause == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief  AMS write entity attribute.
 */
static bool write_entity_attr(uint16_t conn_handle, T_AMS_ENTITY_ATTR entity_attr)
{
    uint16_t handle = INVALID_HDL;
    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ams_uuid,
                                     (T_ATTR_UUID *)&ams_char_entity_attr, &handle))
    {
        T_GAP_CAUSE cause = gatt_client_write(conn_handle, GATT_WRITE_TYPE_REQ, handle,
                                              sizeof(entity_attr), (uint8_t *)&entity_attr, NULL);
        if (cause == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief  AMS read entity attribute.
 */
bool ams_read_entity_attr(uint16_t conn_handle, T_AMS_ENTITY_ATTR entity_attr)
{
    uint16_t handle = INVALID_HDL;

    write_entity_attr(conn_handle, entity_attr);

    if (gatt_client_find_char_handle(conn_handle, (T_ATTR_UUID *)&ams_uuid,
                                     (T_ATTR_UUID *)&ams_char_entity_attr, &handle))
    {
        if (gatt_client_read(conn_handle, handle, NULL))
        {
            return true;
        }
    }

    return false;
}


static T_APP_RESULT client_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                              void *p_data)
{
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;
    T_AMS_CB_DATA cb_data = {};
    PROTOCOL_PRINT_INFO2("ams_client client_cb: conn_handle %04x, type %d", conn_handle, type);

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            PROTOCOL_PRINT_INFO1("ams_client client_cb: is_found %d",
                                 p_client_cb_data->dis_done.is_found);

            cb_data.cb_type = AMS_CLIENT_CB_TYPE_DISC_STATE;
            cb_data.cb_content.disc_state.is_found = p_client_cb_data->dis_done.is_found;
            cb_data.cb_content.disc_state.load_from_ftl = p_client_cb_data->dis_done.load_from_ftl;
        }
        break;
    case GATT_CLIENT_EVENT_READ_RESULT:
        PROTOCOL_PRINT_INFO3("ams_client client_cb: handle 0x%04x, char_type %d, uuid128 %s",
                             p_client_cb_data->read_result.handle, p_client_cb_data->read_result.char_type,
                             TRACE_BINARY(16, p_client_cb_data->read_result.char_uuid.p.uuid128));
        cb_data.cb_type = AMS_CLIENT_CB_TYPE_READ_RESULT;
        if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ams_char_entity_upd.p.uuid128,
                   16) == 0)
        {
            cb_data.cb_content.read_data.type = AMS_READ_FROM_ENTITY_UPD;
            cb_data.cb_content.read_data.p_value = p_client_cb_data->read_result.p_value;
            cb_data.cb_content.read_data.value_size = p_client_cb_data->read_result.value_size;
        }
        else if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ams_char_entity_attr.p.uuid128,
                        16) == 0)
        {
            cb_data.cb_content.read_data.type = AMS_READ_FROM_ENTITY_ATTR;
            cb_data.cb_content.read_data.p_value = p_client_cb_data->read_result.p_value;
            cb_data.cb_content.read_data.value_size = p_client_cb_data->read_result.value_size;
        }

        break;
    case GATT_CLIENT_EVENT_WRITE_RESULT:
        PROTOCOL_PRINT_INFO3("ams_client client_cb: handle 0x%04x, char_type %d, uuid128 %s",
                             p_client_cb_data->write_result.handle, p_client_cb_data->write_result.char_type,
                             TRACE_BINARY(16, p_client_cb_data->write_result.char_uuid.p.uuid128));
        cb_data.cb_type = AMS_CLIENT_CB_TYPE_WRITE_RESULT;
        cb_data.cb_content.write_result.cause = p_client_cb_data->write_result.cause;
        if (p_client_cb_data->write_result.char_uuid.is_uuid16 == false)
        {
            if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ams_char_rmt_cmd.p.uuid128, 16) == 0)
            {
                cb_data.cb_content.write_result.type = AMS_WRITE_REMOTE_CMD_VALUE;
            }
            else if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ams_char_entity_upd.p.uuid128,
                            16) == 0)
            {
                cb_data.cb_content.write_result.type = AMS_WRITE_ENTITY_UPD_VALUE;
            }
            else if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ams_char_entity_attr.p.uuid128,
                            16) == 0)
            {
                cb_data.cb_content.write_result.type = AMS_WRITE_ENTITY_ATTR_VALUE;
            }
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:
        PROTOCOL_PRINT_INFO2("ams_client client_cb: handle 0x%04x, uuid128 %s",
                             p_client_cb_data->notify_ind.handle, TRACE_BINARY(16,
                                                                               p_client_cb_data->notify_ind.char_uuid.p.uuid128));
        cb_data.cb_type = AMS_CLIENT_CB_TYPE_NOTIF_IND_RESULT;
        if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ams_char_rmt_cmd.p.uuid128, 16) == 0)
        {
            cb_data.cb_content.notify_data.type = AMS_NOTIFY_FROM_REMOTE_CMD;
        }
        else if (memcmp(p_client_cb_data->write_result.char_uuid.p.uuid128, ams_char_entity_upd.p.uuid128,
                        16) == 0)
        {
            cb_data.cb_content.notify_data.type = AMS_NOTIFY_FROM_ENTITY_UPD;
        }
        cb_data.cb_content.notify_data.p_value = p_client_cb_data->notify_ind.p_value;
        cb_data.cb_content.notify_data.value_size = p_client_cb_data->notify_ind.value_size;
        break;

    case GATT_CLIENT_EVENT_CCCD_CFG:
        PROTOCOL_PRINT_INFO2("ams_client client_cb: uuid128 %s, cccd_cfg %d",
                             TRACE_BINARY(16, p_client_cb_data->cccd_cfg.char_uuid.p.uuid128),
                             p_client_cb_data->cccd_cfg.cccd_cfg);
        cb_data.cb_type = AMS_CLIENT_CB_TYPE_WRITE_RESULT;
        if (memcmp(p_client_cb_data->cccd_cfg.char_uuid.p.uuid128, ams_char_entity_upd.p.uuid128,
                   16) == 0)
        {
            if (p_client_cb_data->cccd_cfg.cccd_cfg)
            {
                cb_data.cb_content.write_result.type = AMS_WRITE_ENTITY_UPD_NOTIFY_ENABLE;
            }
            else
            {
                cb_data.cb_content.write_result.type = AMS_WRITE_ENTITY_UPD_NOTIFY_DISABLE;
            }
        }
        else if (memcmp(p_client_cb_data->cccd_cfg.char_uuid.p.uuid128, ams_char_rmt_cmd.p.uuid128,
                        16) == 0)
        {
            if (p_client_cb_data->cccd_cfg.cccd_cfg)
            {
                cb_data.cb_content.write_result.type = AMS_WRITE_REMOTE_CMD_NOTIFY_ENABLE;
            }
            else
            {
                cb_data.cb_content.write_result.type = AMS_WRITE_REMOTE_CMD_NOTIFY_DISABLE;
            }
        }
        break;
    case GATT_CLIENT_EVENT_CONN_DEL:
        cb_data.cb_type = AMS_CLIENT_CB_TYPE_DISCONNECT_INFO;
        break;

    default:
        break;
    }

    ams.cb(conn_handle, &cb_data);

    return APP_RESULT_SUCCESS;
}

/**
 * @brief       Add ams client.
 *
 * @param[in]   app_cb  Callback to notify client read/write/notify/indicate events.
 * @param[in]   link_num Initialize link number
 * @return Client ID of the specific client module.
 * @retval 0xff failed.
 * @retval other success.
 */
bool ams_client_init(AMS_CLIENT_CB app_cb)
{
    if (gatt_client_spec_register((T_ATTR_UUID *)&ams_uuid, client_cb) != GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        return false;
    }

    /* register callback for profile to inform application that some events happened. */
    ams.cb = app_cb;

    return true;
}
#endif
