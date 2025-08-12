/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    dis_gatt_client.c
  * @brief
  * @details
  * @author  jane
  * @date    2016-02-18
  * @version v1.0
  ******************************************************************************
  */

/** Add Includes here **/
#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "dis_gatt_client.h"

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

/**<  Callback used to send data to app from DIS client layer. */
static P_FUN_DIS_CLIENT_APP_CB dis_client_cb = NULL;

/**
  * @brief  Used by application, to read characteristic from server by using UUIDs.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @param[in]  srv_instance_id Service instance id.
  * @param[in]  char_uuid       characteristics uuid.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool dis_client_read_char(uint16_t conn_handle, uint8_t srv_instance_id, uint16_t char_uuid)
{
    T_ATTR_UUID service_uuid;
    T_ATTR_UUID characteristic_uuid;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint16_t handle = 0;
    service_uuid.is_uuid16 = true;
    service_uuid.instance_id = srv_instance_id;
    service_uuid.p.uuid16 = GATT_UUID_DEVICE_INFORMATION_SERVICE;
    characteristic_uuid.is_uuid16 = true;
    characteristic_uuid.instance_id = 0;
    characteristic_uuid.p.uuid16 = char_uuid;

    if (gatt_client_find_char_handle(conn_handle, &service_uuid, &characteristic_uuid, &handle))
    {
        cause = gatt_client_read(conn_handle, handle, NULL);
    }
    return (cause == GAP_CAUSE_SUCCESS) ? true : false;
}

T_APP_RESULT dis_client_cbs(uint16_t conn_handle, T_GATT_CLIENT_EVENT type, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GATT_CLIENT_DATA *p_client_cb_data = (T_GATT_CLIENT_DATA *)p_data;

    switch (type)
    {
    case GATT_CLIENT_EVENT_DIS_DONE:
        {
            PROTOCOL_PRINT_INFO1("[DIS] GATT_CLIENT_EVENT_DIS_DONE: is_found %d",
                                 p_client_cb_data->dis_done.is_found);
            T_DIS_CLIENT_DIS_DONE dis_done = {0};
            dis_done.is_found = p_client_cb_data->dis_done.is_found;
            dis_done.load_from_ftl = p_client_cb_data->dis_done.load_from_ftl;
            dis_done.srv_instance_num = p_client_cb_data->dis_done.srv_instance_num;

            if (dis_client_cb)
            {
                result = (* dis_client_cb)(conn_handle, GATT_MSG_DIS_CLIENT_DIS_DONE, &dis_done);
            }
        }
        break;

    case GATT_CLIENT_EVENT_READ_RESULT:
        {
            PROTOCOL_PRINT_INFO2("[DIS] GATT_CLIENT_EVENT_READ_RESULT: value_size %d, uuid 0x%x",
                                 p_client_cb_data->read_result.value_size,
                                 p_client_cb_data->read_result.char_uuid.p.uuid16);
            bool cb_flag = false;
            T_DIS_CLIENT_READ_RESULT read_result = {0};
            read_result.srv_instance_id = p_client_cb_data->read_result.srv_instance_id;
            read_result.cause = p_client_cb_data->read_result.cause;
            read_result.char_uuid = p_client_cb_data->read_result.char_uuid.p.uuid16;
            read_result.value_size = p_client_cb_data->read_result.value_size;

            if (read_result.cause == ATT_SUCCESS)
            {
                cb_flag = true;
                switch (read_result.char_uuid)
                {
                case GATT_UUID_CHAR_MODEL_NUMBER:
                case GATT_UUID_CHAR_SERIAL_NUMBER:
                case GATT_UUID_CHAR_FIRMWARE_REVISION:
                case GATT_UUID_CHAR_HARDWARE_REVISION:
                case GATT_UUID_CHAR_SOFTWARE_REVISION:
                case GATT_UUID_CHAR_MANUFACTURER_NAME:
                    {
                        read_result.data.char_utf8_string = p_client_cb_data->read_result.p_value;
                    }
                    break;

                case GATT_UUID_CHAR_SYSTEM_ID:
                    {
                        if (p_client_cb_data->read_result.value_size == 8)
                        {
                            memcpy(read_result.data.system_id.mid,
                                   &p_client_cb_data->read_result.p_value[0], 5);
                            memcpy(read_result.data.system_id.oui,
                                   &p_client_cb_data->read_result.p_value[5], 3);
                        }
                        else
                        {
                            result = APP_RESULT_INVALID_VALUE_SIZE;
                            read_result.cause = ATT_ERR_INVALID_VALUE_SIZE;
                        }
                    }
                    break;

                case GATT_UUID_CHAR_IEEE_CERTIF_DATA_LIST:
                    {
                        read_result.data.ieee_certif_data_list =
                            p_client_cb_data->read_result.p_value;
                    }
                    break;

                case GATT_UUID_CHAR_PNP_ID:
                    {
                        if (p_client_cb_data->read_result.value_size == 7)
                        {
                            LE_STREAM_TO_UINT8(read_result.data.pnp_id.vendor_id_source,
                                               p_client_cb_data->read_result.p_value);
                            LE_STREAM_TO_UINT16(read_result.data.pnp_id.vendor_id,
                                                p_client_cb_data->read_result.p_value);
                            LE_STREAM_TO_UINT16(read_result.data.pnp_id.product_id,
                                                p_client_cb_data->read_result.p_value);
                            LE_STREAM_TO_UINT16(read_result.data.pnp_id.product_version,
                                                p_client_cb_data->read_result.p_value);
                        }
                        else
                        {
                            result = APP_RESULT_INVALID_VALUE_SIZE;
                            read_result.cause = ATT_ERR_INVALID_VALUE_SIZE;
                        }
                    }
                    break;

                case GATT_UUID_CHAR_UDI_FOR_MEDICAL_DEVICES:
                    {
                        read_result.data.udi_for_medical_dev =
                            p_client_cb_data->read_result.p_value;
                    }
                    break;

                default:
                    break;
                }
            }

            if (dis_client_cb && cb_flag)
            {
                result = (* dis_client_cb)(conn_handle, GATT_MSG_DIS_CLIENT_READ_RESULT, &read_result);
            }
        }
        break;

    default:
        break;
    }

    return result;
}

/**
  * @brief      Add dis client to application.
  * @param[in]  app_cb   pointer of app callback function to handle specific client module data.
  * @return Result of add the specific client module.
  * @retval true  Add client module success.
  * @retval false Add client module failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        if(dis_client_init(app_dis_client_callback))
        {
            APP_PRINT_ERROR0("app_le_profile_init: add dis client fail!");
        }
    }
  * \endcode
  */
bool dis_client_init(P_FUN_DIS_CLIENT_APP_CB app_cb)
{
    T_ATTR_UUID srv_uuid = {0};
    srv_uuid.is_uuid16 = true;
    srv_uuid.p.uuid16 = GATT_UUID_DEVICE_INFORMATION_SERVICE;

    if (gatt_client_spec_register(&srv_uuid, dis_client_cbs) == GAP_CAUSE_SUCCESS)
    {
        /* register callback for profile to inform application that some events happened. */
        dis_client_cb = app_cb;
        return true;
    }

    return false;
}
