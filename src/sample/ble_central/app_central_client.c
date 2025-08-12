/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_central_client.c
   * @brief     This file handles BLE central application routines.
   * @author    danni
   * @date      2022-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "app_central_client.h"
#include <app_central_link_mgr.h>

/** @defgroup CENTRAL_APP Central Application
    * @brief This file handles BLE central application routines.
    * @{
    */
T_CLIENT_ID simple_ble_client_id;/**< Simple ble service client id*/
T_CLIENT_ID gaps_client_id;/**< gap service client id*/
T_CLIENT_ID bas_client_id;/**< battery service client id*/
/*============================================================================*
 *                              Functions
 *============================================================================*/

/** @defgroup  CENTRAL_SRV_DIS GATT Services discovery and storage
    * @brief GATT Services discovery and storage
    * @{
    */
/**
 * @brief app_central_client_discov_services
          Discovery GATT services in ble_peripheral project.
 * @param  conn_id connection ID.
 * @param  start   true - first call this function after conncection, false - not first
 * @retval None
 */
void app_central_client_discov_services(uint8_t conn_id, bool start)
{
    if (app_link_table[conn_id].conn_state != GAP_CONN_STATE_CONNECTED)
    {
        APP_PRINT_ERROR1("app_central_client_discov_services: conn_id %d not connected", conn_id);
        return;
    }
    if (start)
    {
#if F_BT_GATT_SRV_HANDLE_STORAGE
        bool is_disc = true;
        T_APP_SRVS_HDL_TABLE app_srvs_table;
        if (app_load_srvs_hdl_table(&app_srvs_table) == 0)
        {
            if ((app_srvs_table.srv_found_flags != 0) &&
                (app_srvs_table.bd_type == app_link_table[conn_id].bd_type) &&
                (memcmp(app_srvs_table.bd_addr, app_link_table[conn_id].bd_addr, GAP_BD_ADDR_LEN) == 0))
            {
                APP_PRINT_INFO1("app_central_client_discov_services: load from flash, srv_found_flags 0x%x",
                                app_srvs_table.srv_found_flags);
                app_link_table[conn_id].srv_found_flags = app_srvs_table.srv_found_flags;
                if (app_srvs_table.srv_found_flags & APP_DISCOV_GAPS_FLAG)
                {
                    gaps_set_hdl_cache(conn_id, app_srvs_table.gaps_hdl_cache, sizeof(uint16_t) * HDL_GAPS_CACHE_LEN);
                }
                if (app_srvs_table.srv_found_flags & APP_DISCOV_SIMP_FLAG)
                {
                    simp_ble_client_set_hdl_cache(conn_id, app_srvs_table.simp_hdl_cache,
                                                  sizeof(uint16_t) * HDL_SIMBLE_CACHE_LEN);
                }
                if (app_srvs_table.srv_found_flags & APP_DISCOV_BAS_FLAG)
                {
                    bas_set_hdl_cache(conn_id, app_srvs_table.bas_hdl_cache, sizeof(uint16_t) * HDL_BAS_CACHE_LEN);
                }
                is_disc = false;
            }
        }
        else
        {
            APP_PRINT_ERROR0("app_central_client_discov_services: failed");
        }

        if (is_disc)
        {
            if (gaps_start_discovery(conn_id) == false)
            {
                APP_PRINT_ERROR1("app_central_client_discov_services: discover gaps failed conn_id %d", conn_id);
            }
        }
#else
        if (gaps_start_discovery(conn_id) == false)
        {
            APP_PRINT_ERROR1("app_central_client_discov_services: discover gaps failed conn_id %d", conn_id);
        }
#endif
        return;
    }
    if ((app_link_table[conn_id].discovered_flags & APP_DISCOV_SIMP_FLAG) == 0)
    {
        if (simp_ble_client_start_discovery(conn_id) == false)
        {
            APP_PRINT_ERROR1("app_central_client_discov_services: discover simp failed conn_id %d", conn_id);
        }
    }
    else if ((app_link_table[conn_id].discovered_flags & APP_DISCOV_BAS_FLAG) == 0)
    {
        if (bas_start_discovery(conn_id) == false)
        {
            APP_PRINT_ERROR1("app_central_client_discov_services: discover bas failed conn_id %d", conn_id);
        }
    }
    else
    {
        APP_PRINT_INFO2("app_central_client_discov_services: discover complete, conn_id %d, srv_found_flags 0x%x",
                        conn_id, app_link_table[conn_id].srv_found_flags);
#if F_BT_GATT_SRV_HANDLE_STORAGE
        if (app_link_table[conn_id].srv_found_flags != 0)
        {
            T_APP_SRVS_HDL_TABLE app_srvs_table;
            memset(&app_srvs_table, 0, sizeof(T_APP_SRVS_HDL_TABLE));
            app_srvs_table.bd_type = app_link_table[conn_id].bd_type;
            app_srvs_table.srv_found_flags = app_link_table[conn_id].srv_found_flags;
            memcpy(app_srvs_table.bd_addr, app_link_table[conn_id].bd_addr, GAP_BD_ADDR_LEN);
            gaps_get_hdl_cache(conn_id, app_srvs_table.gaps_hdl_cache, sizeof(uint16_t) * HDL_GAPS_CACHE_LEN);
            simp_ble_client_get_hdl_cache(conn_id, app_srvs_table.simp_hdl_cache,
                                          sizeof(uint16_t) * HDL_SIMBLE_CACHE_LEN);
            bas_get_hdl_cache(conn_id, app_srvs_table.bas_hdl_cache, sizeof(uint16_t) * HDL_BAS_CACHE_LEN);
            if (app_save_srvs_hdl_table(&app_srvs_table) != 0)
            {
                APP_PRINT_ERROR0("app_central_client_discov_services: fail");
            }
        }
#endif
    }
    return;
}
/** @} */ /* End of group CENTRAL_SRV_DIS */

/** @defgroup  CENTRAL_CLIIENT_CALLBACK Profile Client Callback Event Handler
    * @brief Handle profile client callback event
    * @{
    */
/**
 * @brief app_central_client_callback
          Callback will be called when data sent from profile client layer.
 * @param  client_id the ID distinguish which module sent the data.
 * @param  conn_id   connection ID.
 * @param  p_data    pointer to data.
 * @retval result    @ref T_APP_RESULT
 */
T_APP_RESULT app_central_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("app_central_client_callback: client_id %d, conn_id %d",
                    client_id, conn_id);
    if (client_id == gaps_client_id)
    {
        T_GAPS_CLIENT_CB_DATA *p_gaps_cb_data = (T_GAPS_CLIENT_CB_DATA *)p_data;
        switch (p_gaps_cb_data->cb_type)
        {
        case GAPS_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_gaps_cb_data->cb_content.disc_state)
            {
            case DISC_GAPS_DONE:
                app_link_table[conn_id].discovered_flags |= APP_DISCOV_GAPS_FLAG;
                app_link_table[conn_id].srv_found_flags |= APP_DISCOV_GAPS_FLAG;
                app_central_client_discov_services(conn_id, false);
                APP_PRINT_INFO0("app_central_client_callback: discover gaps procedure done.");
                break;
            case DISC_GAPS_FAILED:
                app_link_table[conn_id].discovered_flags |= APP_DISCOV_GAPS_FLAG;
                app_central_client_discov_services(conn_id, false);
                APP_PRINT_INFO0("app_central_client_callback: discover gaps request failed.");
                break;
            default:
                break;
            }
            break;
        case GAPS_CLIENT_CB_TYPE_READ_RESULT:
            switch (p_gaps_cb_data->cb_content.read_result.type)
            {
            case GAPS_READ_DEVICE_NAME:
                if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("app_central_client_callback: GAPS_READ_DEVICE_NAME device name %s.",
                                    TRACE_STRING(p_gaps_cb_data->cb_content.read_result.data.device_name.p_value));
                }
                else
                {
                    APP_PRINT_INFO1("app_central_client_callback: GAPS_READ_DEVICE_NAME failded cause 0x%x",
                                    p_gaps_cb_data->cb_content.read_result.cause);
                }
                break;
            case GAPS_READ_APPEARANCE:
                if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("app_central_client_callback: GAPS_READ_APPEARANCE appearance %d",
                                    p_gaps_cb_data->cb_content.read_result.data.appearance);
                }
                else
                {
                    APP_PRINT_INFO1("app_central_client_callback: GAPS_READ_APPEARANCE failded cause 0x%x",
                                    p_gaps_cb_data->cb_content.read_result.cause);
                }
                break;
            case GAPS_READ_CENTRAL_ADDR_RESOLUTION:
                if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("app_central_client_callback: GAPS_READ_CENTRAL_ADDR_RESOLUTION central_addr_res %d",
                                    p_gaps_cb_data->cb_content.read_result.data.central_addr_res);
                }
                else
                {
                    APP_PRINT_INFO1("app_central_client_callback: GAPS_READ_CENTRAL_ADDR_RESOLUTION failded cause 0x%x",
                                    p_gaps_cb_data->cb_content.read_result.cause);
                }
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
    }
    else if (client_id == simple_ble_client_id)
    {
        T_SIMP_CLIENT_CB_DATA *p_simp_client_cb_data = (T_SIMP_CLIENT_CB_DATA *)p_data;
        uint16_t value_size;
        uint8_t *p_value;
        switch (p_simp_client_cb_data->cb_type)
        {
        case SIMP_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_simp_client_cb_data->cb_content.disc_state)
            {
            case DISC_SIMP_DONE:
                app_link_table[conn_id].discovered_flags |= APP_DISCOV_SIMP_FLAG;
                app_link_table[conn_id].srv_found_flags |= APP_DISCOV_SIMP_FLAG;
                app_central_client_discov_services(conn_id, false);
                APP_PRINT_INFO0("app_central_client_callback: discover simp procedure done.");
                break;
            case DISC_SIMP_FAILED:
                app_link_table[conn_id].discovered_flags |= APP_DISCOV_SIMP_FLAG;
                app_central_client_discov_services(conn_id, false);
                APP_PRINT_INFO0("app_central_client_callback: discover simp request failed.");
                break;
            default:
                break;
            }
            break;
        case SIMP_CLIENT_CB_TYPE_READ_RESULT:
            switch (p_simp_client_cb_data->cb_content.read_result.type)
            {
            case SIMP_READ_V1_READ:
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    value_size = p_simp_client_cb_data->cb_content.read_result.data.v1_read.value_size;
                    p_value = p_simp_client_cb_data->cb_content.read_result.data.v1_read.p_value;
                    APP_PRINT_INFO2("app_central_client_callback: SIMP_READ_V1_READ value_size %d, value %b",
                                    value_size, TRACE_BINARY(value_size, p_value));
                }
                else
                {
                    APP_PRINT_ERROR1("app_central_client_callback: SIMP_READ_V1_READ failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                }
                break;
            case SIMP_READ_V3_NOTIFY_CCCD:
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("app_central_client_callback: SIMP_READ_V3_NOTIFY_CCCD notify %d",
                                    p_simp_client_cb_data->cb_content.read_result.data.v3_notify_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("app_central_client_callback: SIMP_READ_V3_NOTIFY_CCCD failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                };
                break;
            case SIMP_READ_V4_INDICATE_CCCD:
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("app_central_client_callback: SIMP_READ_V4_INDICATE_CCCD indicate %d",
                                    p_simp_client_cb_data->cb_content.read_result.data.v4_indicate_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("app_central_client_callback: SIMP_READ_V4_INDICATE_CCCD failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                };
                break;

            default:
                break;
            }
            break;
        case SIMP_CLIENT_CB_TYPE_WRITE_RESULT:
            switch (p_simp_client_cb_data->cb_content.write_result.type)
            {
            case SIMP_WRITE_V2_WRITE:
                APP_PRINT_INFO1("app_central_client_callback: SIMP_WRITE_V2_WRITE write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                break;
            case SIMP_WRITE_V3_NOTIFY_CCCD:
                APP_PRINT_INFO1("app_central_client_callback: SIMP_WRITE_V3_NOTIFY_CCCD write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                break;
            case SIMP_WRITE_V4_INDICATE_CCCD:
                APP_PRINT_INFO1("app_central_client_callback: SIMP_WRITE_V4_INDICATE_CCCD write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                break;
            default:
                break;
            }
            break;
        case SIMP_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
            switch (p_simp_client_cb_data->cb_content.notif_ind_data.type)
            {
            case SIMP_V3_NOTIFY:
                value_size = p_simp_client_cb_data->cb_content.notif_ind_data.data.value_size;
                p_value = p_simp_client_cb_data->cb_content.notif_ind_data.data.p_value;
                APP_PRINT_INFO2("app_central_client_callback: SIMP_V3_NOTIFY value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                break;
            case SIMP_V4_INDICATE:
                value_size = p_simp_client_cb_data->cb_content.notif_ind_data.data.value_size;
                p_value = p_simp_client_cb_data->cb_content.notif_ind_data.data.p_value;
                APP_PRINT_INFO2("app_central_client_callback: SIMP_V4_INDICATE value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
    }
    else if (client_id == bas_client_id)
    {
        T_BAS_CLIENT_CB_DATA *p_bas_cb_data = (T_BAS_CLIENT_CB_DATA *)p_data;
        switch (p_bas_cb_data->cb_type)
        {
        case BAS_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_bas_cb_data->cb_content.disc_state)
            {
            case DISC_BAS_DONE:
                app_link_table[conn_id].discovered_flags |= APP_DISCOV_BAS_FLAG;
                app_link_table[conn_id].srv_found_flags |= APP_DISCOV_BAS_FLAG;
                app_central_client_discov_services(conn_id, false);
                APP_PRINT_INFO0("app_central_client_callback: discover bas procedure done");
                break;
            case DISC_BAS_FAILED:
                app_link_table[conn_id].discovered_flags |= APP_DISCOV_BAS_FLAG;
                app_central_client_discov_services(conn_id, false);
                APP_PRINT_INFO0("app_central_client_callback: discover bas procedure failed");
                break;
            default:
                break;
            }
            break;
        case BAS_CLIENT_CB_TYPE_READ_RESULT:
            switch (p_bas_cb_data->cb_content.read_result.type)
            {
            case BAS_READ_BATTERY_LEVEL:
                if (p_bas_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("app_central_client_callback: BAS_READ_BATTERY_LEVEL battery level %d",
                                    p_bas_cb_data->cb_content.read_result.data.battery_level);
                }
                else
                {
                    APP_PRINT_ERROR1("app_central_client_callback: BAS_READ_BATTERY_LEVEL failed cause 0x%x",
                                     p_bas_cb_data->cb_content.read_result.cause);
                }
                break;
            case BAS_READ_NOTIFY:
                if (p_bas_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("app_central_client_callback: BAS_READ_NOTIFY notify %d",
                                    p_bas_cb_data->cb_content.read_result.data.notify);
                }
                else
                {
                    APP_PRINT_ERROR1("app_central_client_callback: BAS_READ_NOTIFY failed cause 0x%x",
                                     p_bas_cb_data->cb_content.read_result.cause);
                };
                break;

            default:
                break;
            }
            break;
        case BAS_CLIENT_CB_TYPE_WRITE_RESULT:
            switch (p_bas_cb_data->cb_content.write_result.type)
            {
            case BAS_WRITE_NOTIFY_ENABLE:
                APP_PRINT_INFO1("app_central_client_callback: BAS_WRITE_NOTIFY_ENABLE write result 0x%x",
                                p_bas_cb_data->cb_content.write_result.cause);
                break;
            case BAS_WRITE_NOTIFY_DISABLE:
                APP_PRINT_INFO1("app_central_client_callback: BAS_WRITE_NOTIFY_DISABLE write result 0x%x",
                                p_bas_cb_data->cb_content.write_result.cause);
                break;
            default:
                break;
            }
            break;
        case BAS_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
            APP_PRINT_INFO1("app_central_client_callback: BAS_CLIENT_CB_TYPE_NOTIF_IND_RESULT battery level %d",
                            p_bas_cb_data->cb_content.notify_data.battery_level);
            break;

        default:
            break;
        }
    }

    return result;
}
/**
 * @brief app_central_client_init
          Add GATT clients and register callbacks
 * @return void
 */
void app_central_client_init(void)
{
    client_init(3);
    gaps_client_id  = gaps_add_client(app_central_client_callback, APP_MAX_LINKS);
    simple_ble_client_id = simp_ble_add_client(app_central_client_callback, APP_MAX_LINKS);
    bas_client_id = bas_add_client(app_central_client_callback, APP_MAX_LINKS);
}
/** @} */ /* End of group CENTRAL_CLIENT_CALLBACK */
/** End of CENTRAL_APP
* @}
*/
