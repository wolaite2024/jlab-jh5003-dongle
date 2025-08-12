/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_scatternet_scan.c
   * @brief     This file handles BLE observer application routines.
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
#include "ble_mgr.h"
#include "ble_scan.h"
#include "trace.h"
#include "string.h"
#include "stdint.h"
#include "app_scatternet_scan.h"
#include "simple_ble_config.h"
#include "app_scatternet_link_mgr.h"

/** @defgroup  SCATTERNET_APP Scatternet Application
    * @brief This file handles BLE Scatternet application routines.
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/
#define HI_WORD(x)  ((uint8_t)((x & 0xFF00) >> 8))
#define LO_WORD(x)  ((uint8_t)(x))

static BLE_SCAN_HDL app_scan_hdl = NULL;

uint8_t filter_ad_struct[3] =
{
    GAP_ADTYPE_16BIT_COMPLETE,/*type="16 bit uuid"*/
    LO_WORD(GATT_UUID_SIMPLE_PROFILE),
    HI_WORD(GATT_UUID_SIMPLE_PROFILE),
};
/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief app_scan_cb used to notify APP when receive BLE_SCAN_REPORT or BLE_SCAN_PARAM_CHANGES
 * BLE_SCAN_REPORT:
 * report this event to APP when scanning to advertising device.
 * BLE_SCAN_PARAM_CHANGES:
 * report this event to APP when scan parameter has been modified.
 * @param evt @ref BLE_SCAN_EVT
 * @param data @ref BLE_SCAN_EVT_DATA Information of le extended advertising report.
 */
static void app_scan_cb(BLE_SCAN_EVT evt, BLE_SCAN_EVT_DATA *data)
{
    uint8_t scan_state = ble_scan_get_cur_state();

    switch (evt)
    {
    case BLE_SCAN_REPORT:
        APP_PRINT_INFO6("app_scan_cb: BLE_SCAN_REPORT event_type 0x%x, bd_addr %s, addr_type %d, rssi %d, data_len %d, data_status 0x%x",
                        data->report->event_type,
                        TRACE_BDADDR(data->report->bd_addr),
                        data->report->addr_type,
                        data->report->rssi,
                        data->report->data_len,
                        data->report->data_status);
        APP_PRINT_INFO0("Found simple ble service");
        link_mgr_add_device(data->report->bd_addr, data->report->addr_type);
        break;

    default:
        break;
    }
}

/**
 * @brief app_scan_start
 * @param filter_policy
 * @param filter_duplicate
 */
void app_scan_start(uint8_t filter_policy, uint8_t filter_duplicate)
{
    BLE_SCAN_PARAM param;
    BLE_SCAN_FILTER scan_filter;

    APP_PRINT_TRACE2("app_scan_start: filter_policy 0x%x, filter_duplicate 0x%x",
                     filter_policy, filter_duplicate);

    memset(&param, 0, sizeof(param));
    memset(&scan_filter, 0, sizeof(scan_filter));

    param.scan_param_1m.scan_type = GAP_SCAN_MODE_ACTIVE;
    param.scan_param_1m.scan_interval = 400;
    param.scan_param_1m.scan_window = 100;
    param.scan_param_coded.scan_type = GAP_SCAN_MODE_ACTIVE;
    param.scan_param_coded.scan_interval = 440;
    param.scan_param_coded.scan_window = 100;
    param.ext_filter_duplicate = (T_GAP_SCAN_FILTER_DUPLICATE)filter_duplicate;
    param.ext_filter_policy = (T_GAP_SCAN_FILTER_POLICY)filter_policy;
    param.own_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    param.phys = GAP_EXT_SCAN_PHYS_1M_BIT;

    scan_filter.filter_flags = BLE_SCAN_FILTER_ADV_DATA_BIT;
    scan_filter.ad_len = 0x03;
    scan_filter.ad_struct = filter_ad_struct;
    ble_scan_start(&app_scan_hdl, app_scan_cb, &param, &scan_filter);
}

/**
 * @brief app_scan_stop
 *
 */
void app_scan_stop(void)
{
    ble_scan_stop(&app_scan_hdl);
}
/** @} */ /* End of group SCATTERNET_APP */
