/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    ias_gatt_client.c
  * @brief   IAS BLE client source file.
  * @details IAS client data structs and external functions declaration
  *          developed based on bt_gatt_client.h.
  * @author
  * @date
  * @version v1.0
  ******************************************************************************
  */

/** Add Includes here **/
#include <trace.h>
#include <string.h>
#include <ias_gatt_client.h>
#include <os_mem.h>

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

/**<  Callback used to send data to app from IAS client layer. */
static P_FUN_IAS_CLIENT_APP_CB ias_client_cb = NULL;

/**
  * @brief  Used by application, to write alert level characteristic value.
  * @param[in]  conn_handle  Connection handle of the ACL link.
  * @param[in]  alert_level  IAS alert level: @ref T_IAS_CLIENT_IAS_ALERT_LEVEL.
  * @retval true send write command to upper stack success.
  * @retval false send write command to upper stack failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void test(uint16_t conn_handle)
    {
        if(!ias_client_write_alert_level(conn_handle, IAS_NO_ALERT))
        {
            APP_PRINT_ERROR0("ias_client_write_alert_level: write alert level fail!");
        }
    }
  * \endcode
  */
bool ias_client_write_alert_level(uint16_t conn_handle, T_IAS_ALERT_LEVEL alert_level)
{
    T_ATTR_UUID srv_uuid;
    T_ATTR_UUID char_uuid;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint16_t handle = 0;
    T_GATT_WRITE_TYPE type = GATT_WRITE_TYPE_CMD;

    srv_uuid.is_uuid16 = true;
    srv_uuid.instance_id = 0;
    srv_uuid.p.uuid16 = GATT_UUID_IMMEDIATE_ALERT_SERVICE;
    char_uuid.is_uuid16 = true;
    char_uuid.instance_id = 0;
    char_uuid.p.uuid16 = GATT_UUID_CHAR_ALERT_LEVEL;

    if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
    {
        cause = gatt_client_write(conn_handle, type, handle, sizeof(uint8_t), &alert_level, NULL);
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

/**
 * @brief IAS BLE Client Callbacks.
*/
T_APP_RESULT ias_client_cbs(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            PROTOCOL_PRINT_INFO1("[IAS] GATT_CLIENT_EVENT_DIS_DONE: is_found %d",
                                 p_client_cb_data->dis_done.is_found);
            T_IAS_CLIENT_DIS_DONE dis_done = {0};

            dis_done.is_found = p_client_cb_data->dis_done.is_found;
            dis_done.load_from_ftl = p_client_cb_data->dis_done.load_from_ftl;

            if (ias_client_cb)
            {
                result = (*ias_client_cb)(conn_handle, GATT_MSG_IAS_CLIENT_DIS_DONE, &dis_done);
            }
        }
        break;

    case GATT_CLIENT_EVENT_WRITE_RESULT:
        {
            PROTOCOL_PRINT_INFO3("[IAS] GATT_CLIENT_EVENT_WRITE_RESULT: conn_handle 0x%x, cause 0x%x, uuid 0x%x",
                                 conn_handle, p_client_cb_data->write_result.cause,
                                 p_client_cb_data->write_result.char_uuid.p.uuid16);
            T_IAS_CLIENT_WRITE_ALERT_LEVEL_RESULT write_result = {0};

            write_result.cause = p_client_cb_data->write_result.cause;

            if (ias_client_cb)
            {
                result = (*ias_client_cb)(conn_handle, GATT_MSG_IAS_CLIENT_WRITE_ALERT_LEVEL_RESULT, &write_result);
            }
        }
        break;

    default:
        break;
    }

    return result;
}

/**
  * @brief      Add IAS service client to application.
  * @param[in]  app_cb Pointer of app callback function to handle specific client module data.
  * @return Result of add the specific client module.
  * @retval true  Add client module success.
  * @retval false Add client module failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        if(!ias_client_init(app_ias_client_callback))
        {
            APP_PRINT_ERROR0("app_le_profile_init: add ias client fail!");
        }
    }
  * \endcode
  */
bool ias_client_init(P_FUN_IAS_CLIENT_APP_CB app_cb)
{
    T_ATTR_UUID srv_uuid = {0};
    srv_uuid.is_uuid16 = true;
    srv_uuid.p.uuid16 = GATT_UUID_IMMEDIATE_ALERT_SERVICE;

    if (gatt_client_spec_register(&srv_uuid, ias_client_cbs) == GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        ias_client_cb = app_cb;
        return true;
    }

    return false;
}

