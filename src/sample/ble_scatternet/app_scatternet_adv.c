/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_scatternet_adv.c
   * @brief     This file handles BLE scatternet application routines.
   * @author    danni
   * @date      2022-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "ftl.h"
#include "trace.h"
#include "stdint.h"
#include "string.h"
#include "data_uart.h"
#include "ble_ext_adv.h"
#include "app_scatternet_adv.h"
#include "simple_ble_service.h"

/** @defgroup SCATTERNET_APP Scatternet Application
  * @brief Scatternet Application
  * @{
  */
/*============================================================================*
 *                              Constants
 *============================================================================*/

#define HI_WORD(x)  ((uint8_t)((x & 0xFF00) >> 8))
#define LO_WORD(x)  ((uint8_t)(x))

#define APP_STATIC_RANDOM_ADDR_OFFSET 0xC00

/*============================================================================*
 *                              Variables
 *============================================================================*/
uint8_t adv_handle = 0xFF;
static T_BLE_EXT_ADV_MGR_STATE adv_state = BLE_EXT_ADV_MGR_ADV_DISABLED;

/**
 * @brief scan response data struct
 *
 */
uint8_t scan_rsp_data[] =
{
    0x03,/* length */
    GAP_ADTYPE_APPEARANCE,/* type="Appearance" */
    LO_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
    HI_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
};

/**
 * @brief advertising data struct
 *
 */
uint8_t adv_data[] =
{
    /* Flags */
    0x02,/* length */
    GAP_ADTYPE_FLAGS,/* type="Flags" */
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    /* Service */
    0x03,/* length */
    GAP_ADTYPE_16BIT_COMPLETE,/*type="16 bit uuid"*/
    LO_WORD(GATT_UUID_SIMPLE_PROFILE),
    HI_WORD(GATT_UUID_SIMPLE_PROFILE),

    /* Local name */
    0x0F,/* length */
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,/*type="name"*/
    'B', 'L', 'E', '_', 'S', 'C', 'A', 'T', 'T', 'E', 'R', 'N', 'E', 'T',
};

/**
 * @brief static random address struct which is storged in ftl
 *
 */
typedef struct
{
    uint8_t is_exist;
    uint8_t reserved;
    uint8_t bd_addr[GAP_BD_ADDR_LEN];
} T_APP_STATIC_RANDOM_ADDR;

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief app_adv_callback
 *
 * @param cb_type
 * @ref BLE_EXT_ADV_STATE_CHANGE this message will be send to APP when advertising state changed
 * @ref BLE_EXT_ADV_SET_CONN_INFO this message will be send to APP when connection state changed
 *
 * @param p_cb_data
 */
void app_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));

    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            APP_PRINT_TRACE2("app_adv_callback: adv_state %d, adv_handle %d",
                             cb_data.p_ble_state_change->state, cb_data.p_ble_state_change->adv_handle);
            adv_state = cb_data.p_ble_state_change->state;

            if (adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                APP_PRINT_TRACE0("app_adv_callback: BLE_EXT_ADV_MGR_ADV_ENABLED");
                data_uart_print("GAP adv start\r\n");
            }
            else if (adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                APP_PRINT_TRACE0("app_adv_callback: BLE_EXT_ADV_MGR_ADV_DISABLED");
                data_uart_print("GAP adv stoped\r\n");
                switch (cb_data.p_ble_state_change->stop_cause)
                {
                case BLE_EXT_ADV_STOP_CAUSE_APP:
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_CONN:
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_TIMEOUT:
                    break;

                default:
                    break;
                }
                APP_PRINT_TRACE2("app_adv_callback: stack stop adv cause 0x%x, app stop adv cause 0x%02x",
                                 cb_data.p_ble_state_change->stop_cause, cb_data.p_ble_state_change->app_cause);
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        APP_PRINT_TRACE2("app_adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x, adv_handle %d",
                         cb_data.p_ble_conn_info->conn_id, cb_data.p_ble_conn_info->adv_handle);
        break;

    default:
        break;
    }
}

/**
 * @brief app_adv_load_static_random_address
 * if random address already exist in ftl, then load this address.
 * @param p_addr
 * @return uint32_t
 */
uint32_t app_adv_load_static_random_address(T_APP_STATIC_RANDOM_ADDR *p_addr)
{
    uint32_t result;
    result = ftl_load_from_storage(p_addr, APP_STATIC_RANDOM_ADDR_OFFSET,
                                   sizeof(T_APP_STATIC_RANDOM_ADDR));
    APP_PRINT_INFO1("app_adv_load_static_random_address: result 0x%x", result);
    if (result)
    {
        memset(p_addr, 0, sizeof(T_APP_STATIC_RANDOM_ADDR));
    }
    return result;
}

/**
 * @brief app_adv_save_static_random_address
 * save static random address into ftl
 * @param p_addr
 * @return uint32_t
 */
uint32_t app_adv_save_static_random_address(T_APP_STATIC_RANDOM_ADDR *p_addr)
{
    APP_PRINT_INFO0("app_adv_save_static_random_address");
    return ftl_save_to_storage(p_addr, APP_STATIC_RANDOM_ADDR_OFFSET, sizeof(T_APP_STATIC_RANDOM_ADDR));
}

/**
 * @brief app_adv_init_conn_public
 *        advertising parameter initialization: use public address to advertise connectable advertising.
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_LEGACY_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval for undirected and low duty directed advertising.
 *                   In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval for undirected and low duty directed advertising.
 *                   In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_adv_init_conn_public(void)
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    ble_ext_adv_mgr_init_adv_params(&adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, sizeof(adv_data), adv_data,
                                    sizeof(scan_rsp_data), scan_rsp_data, NULL);

    ble_ext_adv_mgr_register_callback(app_adv_callback, adv_handle);
}

/**
 * @brief app_adv_init_conn_random
 *        advertising parameter initialization: use static random address to advertise connectable advertising.
 * adv_handle        Identify an advertising set, which is assigned by @ref ble_ext_adv_mgr_init_adv_params.
 * adv_event_prop    Type of advertising event.
 *                   Values for legacy advertising PDUs: @ref T_LE_EXT_ADV_LEGACY_ADV_PROPERTY.
 * adv_interval_min  Minimum advertising interval for undirected and low duty directed advertising.
 *                   In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * adv_interval_max  Maximum advertising interval for undirected and low duty directed advertising.
 *                   In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * own_address_type  Local address type, @ref T_GAP_LOCAL_ADDR_TYPE.
 * peer_address_type Remote address type, GAP_REMOTE_ADDR_LE_PUBLIC or GAP_REMOTE_ADDR_LE_RANDOM in @ref T_GAP_REMOTE_ADDR_TYPE.
 *                   GAP_REMOTE_ADDR_LE_PUBLIC: Public Device Address or Public Identity Address.
 *                   GAP_REMOTE_ADDR_LE_RANDOM: Random Device Address or Random(static) Identity Address.
 * peer_address      Remote address.
 * filter_policy     Advertising filter policy: @ref T_GAP_ADV_FILTER_POLICY.
 */
void app_adv_init_conn_random(void)
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    bool gen_addr = true;
    T_APP_STATIC_RANDOM_ADDR random_addr;

    if (app_adv_load_static_random_address(&random_addr) == 0)
    {
        if ((random_addr.is_exist == true) && ((random_addr.bd_addr[5] & 0xC0) == 0xC0))
        {
            gen_addr = false;
        }
    }
    if (gen_addr)
    {
        if (le_gen_rand_addr(GAP_RAND_ADDR_STATIC, random_addr.bd_addr) == GAP_CAUSE_SUCCESS)
        {
            random_addr.is_exist = true;
            app_adv_save_static_random_address(&random_addr);
        }
    }
    APP_PRINT_INFO1("app_peripheral_adv_init_conn_random: random address %b",
                    TRACE_BDADDR(random_addr.bd_addr));

    ble_ext_adv_mgr_init_adv_params(&adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, sizeof(adv_data), adv_data,
                                    sizeof(scan_rsp_data), scan_rsp_data, random_addr.bd_addr);

    ble_ext_adv_mgr_register_callback(app_adv_callback, adv_handle);
}

/**
 * @brief app_adv_start
 *        start advertising
 * @param duration_10ms If non-zero, indicates the duration that advertising is enabled.
                        0x0000:        Always advertising, no advertising duration.
                        0x0001-0xFFFF: Advertising duration, in units of 10ms.
 * @return true         BLE protocol stack has already receive this command and ready to execute,
 *                      when this command execution complete, BLE protocol stack will send BLE_EXT_ADV_MGR_ADV_ENABLED to APP. @ref app_peripheral_adv_callback
 * @return false        There has some errors that cause the BLE protocol stack fail to receive this command.
 */
bool app_adv_start(uint16_t duration_10ms)
{
    if (adv_state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        APP_PRINT_INFO0("app_adv_start");
        if (ble_ext_adv_mgr_enable(adv_handle, duration_10ms) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
        return false;
    }

    APP_PRINT_TRACE0("app_adv_start: Already started");
    return true;
}

/**
 * @brief app_adv_stop
 *        stop advertising
 * @param app_cause please reference app_adv_stop_cause.h
 *                  if you want to add new advertising stop cause, please added in app_adv_stop_cause.h
 * @return true     BLE protocol stack has already receive this command and ready to execute,
 *                  when this command execution complete, BLE protocol stack will send BLE_EXT_ADV_MGR_ADV_DISABLED to APP. @ref app_peripheral_adv_callback
 * @return false    There has some errors that cause the BLE protocol stack fail to receive this command.
 */
bool app_adv_stop(int8_t app_cause)
{
    if (adv_state == BLE_EXT_ADV_MGR_ADV_ENABLED)
    {
        if (ble_ext_adv_mgr_disable(adv_handle, app_cause) == GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_INFO0("app_adv_stop");
            return true;
        }
        return false;
    }

    APP_PRINT_TRACE0("app_adv_stop: Already stoped");
    return true;
}

/**
 * @brief app_adv_get_state
 *        get advertising state
 * @return T_BLE_EXT_ADV_MGR_STATE
 */
T_BLE_EXT_ADV_MGR_STATE app_adv_get_state(void)
{
    return ble_ext_adv_mgr_get_adv_state(adv_handle);
}

/**
 * @brief app_adv_update_randomaddr  update random address
 *
 * @param random_address
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_adv_update_randomaddr(uint8_t *random_address)
{
    return ble_ext_adv_mgr_set_random(adv_handle, random_address);
}

/**
 * @brief app_adv_update_advdata  update advertising data
 *
 * @param p_adv_data   BLE protocol stack will not reallocate memory for adv data,
 *                     so p_adv_data shall point to a global memory.
 *                     if you don't want to set adv data, set default value NULL.
 * @param adv_data_len Advertising data or scan response data length shall not exceed 31 bytes.
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_adv_update_advdata(uint8_t *p_adv_data, uint16_t adv_data_len)
{
    return ble_ext_adv_mgr_set_adv_data(adv_handle, adv_data_len, p_adv_data);
}

/**
 * @brief app_adv_update_scanrspdata update scan response data
 *
 * @param p_scan_data   BLE protocol stack will not reallocate memory for scan response data,
 *                      so p_scan_data shall point to a global memory.
 *                      if you don't want to set scan response data, set default value NULL.
 * @param scan_data_len Advertising data or scan response data length shall not exceed 31 bytes.
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_adv_update_scanrspdata(uint8_t *p_scan_data, uint16_t scan_data_len)
{
    return ble_ext_adv_mgr_set_scan_response_data(adv_handle, scan_data_len, p_scan_data);
}

/**
 * @brief app_adv_update_interval update advertising interval
 *
 * @param adv_interval advertising interval for undirected and low duty directed advertising.
 *                     In units of 0.625ms, range: 0x000020 to 0xFFFFFF.
 * @return T_GAP_CAUSE
 */
T_GAP_CAUSE app_adv_update_interval(uint16_t adv_interval)
{
    return ble_ext_adv_mgr_change_adv_interval(adv_handle, adv_interval);
}

/** End of SCATTERNET_APP
* @}
*/
