/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    bas_gatt_client.c
  * @brief   Head file for using battery service client.
  * @details Battery service client data structs and external functions declaration
  *          developed based on bt_gatt_client.h.
  * @author  jane
  * @date    2016-02-18
  * @version v1.0
  ******************************************************************************
  */

/** Add Includes here **/
#include <stdlib.h>
#include <trace.h>
#include "bas_gatt_client.h"

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

#define GATT_UUID_BATTERY                       0x180F
#define GATT_UUID_CHAR_BAS_LEVEL                0x2A19
/**
 * @brief  BAS client Link control block defination.
 */

/**<  Callback used to send data to app from BAS client layer. */
static P_FUN_BAS_CLIENT_APP_CB bas_client_cb = NULL;

/**
  * @brief      Config the BAS service CCCD data.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @param[in]  srv_instance_id Service instance id.
  * @param[in]  enable          Whether to enable CCCD.
  * \arg    true    Enable CCCD.
  * \arg    false   Disable CCCD.
  * @return Result of config the BAS service CCCD data.
  * @retval true  Config the BAS service CCCD data success.
  * @retval false Config the BAS service CCCD data failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void test(uint16_t conn_handle, uint8_t srv_instance_id)
    {
        bas_client_cfg_cccd(conn_handle, srv_instance_id, true);
    }
  * \endcode
  */
bool bas_client_cfg_cccd(uint16_t conn_handle, uint8_t srv_instance_id, bool enable)
{
    T_ATTR_UUID srv_uuid;
    T_ATTR_UUID char_uuid;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    srv_uuid.is_uuid16 = true;
    srv_uuid.instance_id = srv_instance_id;
    srv_uuid.p.uuid16 = GATT_UUID_BATTERY;
    char_uuid.is_uuid16 = true;
    char_uuid.instance_id = 0;
    char_uuid.p.uuid16 = GATT_UUID_CHAR_BAS_LEVEL;
    uint8_t cccd_cfg;
    if (enable)
    {
        cccd_cfg = GATT_CLIENT_CONFIG_ALL;
    }
    else
    {
        cccd_cfg = GATT_CLIENT_CONFIG_DISABLE;
    }

    if (gatt_client_check_cccd_enabled(conn_handle, &srv_uuid, &char_uuid) != enable)
    {
        cause = gatt_client_enable_char_cccd(conn_handle, &srv_uuid, &char_uuid, cccd_cfg);
    }

    return (cause == GAP_CAUSE_SUCCESS) ? true : false;
}

/**
  * @brief  Used by application, to read battery level.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @param[in]  srv_instance_id Service instance id.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    static T_USER_CMD_PARSE_RESULT cmd_basread(T_USER_CMD_PARSED_VALUE *p_parse_value)
    {
        uint16_t conn_handle = p_parse_value->dw_param[0];
        uint8_t srv_instance_id = p_parse_value->dw_param[1];
        bool ret = false;
        ret = bas_client_read_battery_level(conn_handle, srv_instance_id);
        ......
    }
  * \endcode
  */
bool bas_client_read_battery_level(uint16_t conn_handle, uint8_t srv_instance_id)
{
    T_ATTR_UUID srv_uuid;
    T_ATTR_UUID char_uuid;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint16_t handle = 0;
    srv_uuid.is_uuid16 = true;
    srv_uuid.instance_id = srv_instance_id;
    srv_uuid.p.uuid16 = GATT_UUID_BATTERY;
    char_uuid.is_uuid16 = true;
    char_uuid.instance_id = 0;
    char_uuid.p.uuid16 = GATT_UUID_CHAR_BAS_LEVEL;

    if (gatt_client_find_char_handle(conn_handle, &srv_uuid, &char_uuid, &handle))
    {
        cause = gatt_client_read(conn_handle, handle, NULL);
    }
    return (cause == GAP_CAUSE_SUCCESS) ? true : false;
}

T_APP_RESULT bas_client_cbs(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            T_BAS_CLIENT_DIS_DONE dis_done = {0};
            dis_done.is_found = p_client_cb_data->dis_done.is_found;
            dis_done.load_from_ftl = p_client_cb_data->dis_done.load_from_ftl;
            dis_done.srv_instance_num = p_client_cb_data->dis_done.srv_instance_num;

            if (bas_client_cb)
            {
                result = (* bas_client_cb)(conn_handle, GATT_MSG_BAS_CLIENT_DIS_DONE, &dis_done);
            }
        }
        break;

    case GATT_CLIENT_EVENT_READ_RESULT:
        {
            T_BAS_CLIENT_READ_BATTERY_LEVEL_RESULT read_result = {0};
            read_result.srv_instance_id = p_client_cb_data->read_result.srv_instance_id;
            read_result.cause = p_client_cb_data->read_result.cause;

            if (read_result.cause == ATT_SUCCESS)
            {
                if (p_client_cb_data->read_result.char_uuid.p.uuid16 == GATT_UUID_CHAR_BAS_LEVEL)
                {
                    if (p_client_cb_data->read_result.value_size == 1)
                    {
                        read_result.battery_level = p_client_cb_data->read_result.p_value[0];
                    }
                    else
                    {
                        result = APP_RESULT_INVALID_VALUE_SIZE;
                        read_result.cause = ATT_ERR_INVALID_VALUE_SIZE;
                    }
                }
            }

            if (bas_client_cb)
            {
                (* bas_client_cb)(conn_handle, GATT_MSG_BAS_CLIENT_READ_BATTERY_LEVEL_RESULT,
                                  &read_result);
            }
        }
        break;

    case GATT_CLIENT_EVENT_NOTIFY_IND:
        {
            T_BAS_CLIENT_BATTERY_LEVEL_NOTIFY notify_data = {0};
            notify_data.srv_instance_id = p_client_cb_data->notify_ind.srv_instance_id;

            if (p_client_cb_data->notify_ind.char_uuid.p.uuid16 == GATT_UUID_CHAR_BAS_LEVEL)
            {
                if (p_client_cb_data->notify_ind.value_size != 1)
                {
                    return APP_RESULT_INVALID_VALUE_SIZE;
                }
                notify_data.battery_level = p_client_cb_data->notify_ind.p_value[0];
            }

            if (bas_client_cb)
            {
                result = (* bas_client_cb)(conn_handle, GATT_MSG_BAS_CLIENT_BATTERY_LEVEL_NOTIFY, &notify_data);
            }
        }
        break;

    case GATT_CLIENT_EVENT_CCCD_CFG:
        {
            T_BAS_CLIENT_CCCD_CFG_RESULT cccd_info = {0};

            cccd_info.srv_instance_id = p_client_cb_data->cccd_cfg.srv_instance_id;
            cccd_info.cause = p_client_cb_data->cccd_cfg.cause;

            if (p_client_cb_data->cccd_cfg.char_uuid.p.uuid16 == GATT_UUID_CHAR_BAS_LEVEL)
            {
                if (p_client_cb_data->cccd_cfg.cccd_cfg & GATT_CLIENT_CONFIG_NOTIFICATION)
                {
                    cccd_info.enable = true;
                }
                else
                {
                    cccd_info.enable = false;
                }
            }

            if (bas_client_cb)
            {
                result = (* bas_client_cb)(conn_handle, GATT_MSG_BAS_CLIENT_CCCD_CFG_RESULT, &cccd_info);
            }
        }
        break;

    default:
        break;
    }

    return result;
}

/**
  * @brief      Add bas client to application.
  * @param[in]  app_cb   pointer of app callback function to handle specific client module data.
  * @return Result of add the specific client module.
  * @retval true  Add client module success.
  * @retval false Add client module failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        if(bas_client_init(app_bas_client_callback))
        {
            APP_PRINT_ERROR0("app_le_profile_init: add bas client fail!");
        }
    }
  * \endcode
  */
bool bas_client_init(P_FUN_BAS_CLIENT_APP_CB app_cb)
{
    T_ATTR_UUID srv_uuid = {0};
    srv_uuid.is_uuid16 = true;
    srv_uuid.p.uuid16 = GATT_UUID_BATTERY;

    if (gatt_client_spec_register(&srv_uuid, bas_client_cbs) == GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        bas_client_cb = app_cb;
        return true;
    }

    return false;
}

