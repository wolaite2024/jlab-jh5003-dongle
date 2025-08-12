/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_scatternet_service.c
   * @brief     This file handles BLE peripheral application routines.
   * @author    danni
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "gatt_builtin_services.h"
#include "app_scatternet_service.h"
#include "trace.h"
#include "gap_conn_le.h"
#include "bt_gatt_svc.h"

/** @defgroup SCATTERNET_APP Scatternet Application
  * @brief Scatternet Application
  * @{
  */
/*============================================================================*
 *                              Variables
 *============================================================================*/
T_SERVER_ID simp_srv_id;/**< Simple ble service id*/
T_SERVER_ID bas_srv_id;/**< Battery service id */


/*============================================================================*
 *                              Functions
 *============================================================================*/
#if F_BT_GAPS_CHAR_WRITEABLE
/** @defgroup  SCATTERNET_GAPS_WRITE GAP Service Callback Handler
    * @brief Use @ref F_BT_GAPS_CHAR_WRITEABLE to open
    * @{
    */
/**
 * @brief    All the BT GAP service callback events are handled in this function
 * @param[in] service_id  Profile service ID
 * @param[in] p_para      Pointer to callback data
 * @return   Indicates the function call is successful or not
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT gap_service_callback(T_SERVER_ID service_id, void *p_para)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GAPS_CALLBACK_DATA *p_gap_data = (T_GAPS_CALLBACK_DATA *)p_para;
    APP_PRINT_INFO2("gap_service_callback conn_id = %d msg_type = %d\n", p_gap_data->conn_id,
                    p_gap_data->msg_type);
    if (p_gap_data->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
    {
        switch (p_gap_data->msg_data.opcode)
        {
        case GAPS_WRITE_DEVICE_NAME:
            {
                T_LOCAL_NAME device_name;
                memcpy(device_name.local_name, p_gap_data->msg_data.p_value, p_gap_data->msg_data.len);
                device_name.local_name[p_gap_data->msg_data.len] = 0;
                flash_save_local_name(&device_name);
            }
            break;

        case GAPS_WRITE_APPEARANCE:
            {
                uint16_t appearance_val;
                T_LOCAL_APPEARANCE appearance;

                LE_ARRAY_TO_UINT16(appearance_val, p_gap_data->msg_data.p_value);
                appearance.local_appearance = appearance_val;
                flash_save_local_appearance(&appearance);
            }
            break;

        default:
            break;
        }
    }
    return result;
}
/** @} */
#endif
/**
    * @brief app_service_callback
    * All the BT Profile service callback events are handled in this function
    * @note Then the event handling function shall be called according to the service_id
    * @param service_id  Profile service ID
    * @param p_data      Pointer to callback data
    * @return T_APP_RESULT, which indicates the function call is successful or not
    * @retval APP_RESULT_SUCCESS  Function run successfully
    * @retval others              Function run failed, and return number indicates the reason
    */
T_APP_RESULT app_service_callback(T_SERVER_ID service_id, void *p_data)
{
    uint8_t conn_id = 0xFF;
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    if (service_id == SERVICE_PROFILE_GENERAL_ID)
    {
#if F_APP_GATT_SERVER_EXT_API_SUPPORT
        T_SERVER_EXT_APP_CB_DATA *p_param = (T_SERVER_EXT_APP_CB_DATA *)p_data;
        le_get_conn_id_by_handle(p_param->event_data.send_data_result.conn_handle, &conn_id);
#else
        T_SERVER_APP_CB_DATA *p_param = (T_SERVER_APP_CB_DATA *)p_data;
        conn_id = p_param->event_data.send_data_result.conn_id;
#endif

        switch (p_param->eventId)
        {
        case PROFILE_EVT_SRV_REG_COMPLETE:
            APP_PRINT_INFO1("app_service_callback: PROFILE_EVT_SRV_REG_COMPLETE result %d",
                            p_param->event_data.service_reg_result);
            break;

        case PROFILE_EVT_SEND_DATA_COMPLETE:
            APP_PRINT_INFO5("app_service_callback: PROFILE_EVT_SEND_DATA_COMPLETE conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d",
                            conn_id,
                            p_param->event_data.send_data_result.cause,
                            p_param->event_data.send_data_result.service_id,
                            p_param->event_data.send_data_result.attrib_idx,
                            p_param->event_data.send_data_result.credits);

            if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
            {
                APP_PRINT_INFO0("app_service_callback: PROFILE_EVT_SEND_DATA_COMPLETE success");
            }
            else
            {
                APP_PRINT_ERROR0("app_service_callback: PROFILE_EVT_SEND_DATA_COMPLETE failed");
            }

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
            if (!gatt_svc_handle_profile_data_cmpl(p_param->event_data.send_data_result.conn_handle,
                                                   p_param->event_data.send_data_result.cid,
                                                   p_param->event_data.send_data_result.service_id,
                                                   p_param->event_data.send_data_result.attrib_idx,
                                                   p_param->event_data.send_data_result.credits,
                                                   p_param->event_data.send_data_result.cause))
            {
                APP_PRINT_ERROR0("gatt_svc_handle_profile_data_cmpl failed");
            }
#else
            {
                uint16_t conn_handle = le_get_conn_handle(conn_id);
                if (!gatt_svc_handle_profile_data_cmpl(conn_handle, L2C_FIXED_CID_ATT,
                                                       p_param->event_data.send_data_result.service_id,
                                                       p_param->event_data.send_data_result.attrib_idx,
                                                       p_param->event_data.send_data_result.credits,
                                                       p_param->event_data.send_data_result.cause))
                {
                    APP_PRINT_ERROR0("gatt_svc_handle_profile_data_cmpl failed");
                }
            }
#endif
            break;

        default:
            break;
        }
    }
    else if (service_id == simp_srv_id)
    {
        TSIMP_CALLBACK_DATA *p_simp_cb_data = (TSIMP_CALLBACK_DATA *)p_data;
        switch (p_simp_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_simp_cb_data->msg_data.notification_indification_index)
                {
                case SIMP_NOTIFY_INDICATE_V3_ENABLE:
                    {
                        APP_PRINT_INFO0("app_service_callback: SIMP_NOTIFY_INDICATE_V3_ENABLE");
                    }
                    break;

                case SIMP_NOTIFY_INDICATE_V3_DISABLE:
                    {
                        APP_PRINT_INFO0("app_service_callback: SIMP_NOTIFY_INDICATE_V3_DISABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V4_ENABLE:
                    {
                        APP_PRINT_INFO0("app_service_callback: SIMP_NOTIFY_INDICATE_V4_ENABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V4_DISABLE:
                    {
                        APP_PRINT_INFO0("app_service_callback: SIMP_NOTIFY_INDICATE_V4_DISABLE");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_simp_cb_data->msg_data.read_value_index == SIMP_READ_V1)
                {
                    uint8_t value[2] = {0x01, 0x02};
                    APP_PRINT_INFO0("app_service_callback: SIMP_READ_V1");
                    simp_ble_service_set_parameter(SIMPLE_BLE_SERVICE_PARAM_V1_READ_CHAR_VAL, 2, &value);
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
                switch (p_simp_cb_data->msg_data.write.opcode)
                {
                case SIMP_WRITE_V2:
                    {
                        APP_PRINT_INFO2("app_service_callback: SIMP_WRITE_V2 write type %d, len %d",
                                        p_simp_cb_data->msg_data.write.write_type,
                                        p_simp_cb_data->msg_data.write.len);
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        default:
            break;
        }
    }
    else if (service_id == bas_srv_id)
    {
        T_BAS_CALLBACK_DATA *p_bas_cb_data = (T_BAS_CALLBACK_DATA *)p_data;
        switch (p_bas_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_bas_cb_data->msg_data.notification_indification_index)
                {
                case BAS_NOTIFY_BATTERY_LEVEL_ENABLE:
                    {
                        APP_PRINT_INFO0("app_service_callback: BAS_NOTIFY_BATTERY_LEVEL_ENABLE");
                    }
                    break;

                case BAS_NOTIFY_BATTERY_LEVEL_DISABLE:
                    {
                        APP_PRINT_INFO0("app_service_callback: BAS_NOTIFY_BATTERY_LEVEL_DISABLE");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_bas_cb_data->msg_data.read_value_index == BAS_READ_BATTERY_LEVEL)
                {
                    uint8_t battery_level = 90;
                    APP_PRINT_INFO1("app_service_callback: BAS_READ_BATTERY_LEVEL %d", battery_level);
                    bas_set_parameter(BAS_PARAM_BATTERY_LEVEL, 1, &battery_level);
                }
            }
            break;

        default:
            break;
        }
    }

    return app_result;
}

/**
 * @brief app_service_init
 * Init simple service, battery service and register service callback
 *
 */
void app_service_init(void)
{
#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    server_cfg_use_ext_api(true);
    APP_PRINT_INFO0("app_service_init: server_cfg_use_ext_api true");
    server_ext_register_app_cb(app_service_callback);
#else
    server_register_app_cb(app_service_callback);
#endif

    server_init(2);

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    gatt_svc_init(GATT_SVC_USE_EXT_SERVER, 0);
#else
    gatt_svc_init(GATT_SVC_USE_NORMAL_SERVER, 0);
#endif

    simp_srv_id = simp_ble_service_add_service(app_service_callback);
    bas_srv_id  = bas_add_service(app_service_callback);
}

/** End of SCATTERNET_APP
* @}
*/
