/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <string.h>
#include <os_mem.h>
#include <trace.h>
#include "ual_types.h"
#include "ual_list.h"
//#include "ual_errno.h"
#include "ual_upperstack_cfg.h"

#include "gap_msg.h"
#include "dev_mgr.h"
#include "gap_ext_adv.h"
#include "ble_legacy_adv.h"
#include "ble_extend_adv.h"
#include "ual_adv.h"

typedef struct
{
    bool      ext_adv_support;
    bool      periodic_adv_support;
    uint16_t  max_ext_adv_len;
    uint8_t   max_num_of_adv_sets;
} T_ADV_MGR_CB;


static T_ADV_MGR_CB adv_mgr;

bool adv_suspend_req(void)
{
    if (adv_mgr.ext_adv_support)
    {
        return ble_ext_adv_suspend();
    }
    else
    {
        return ble_legacy_adv_mgr_suspend();
    }
}

void adv_resume_req(void)
{
    if (adv_mgr.ext_adv_support)
    {
        ble_ext_adv_resume();
    }
    else
    {
        ble_legacy_adv_mgr_resume();
    }
}

uint8_t ble_alloc_adv_instance(P_FUN_ADV_APP_CB app_callback, uint16_t adv_event_prop,
                               uint32_t primary_adv_interval_min, uint32_t primary_adv_interval_max,
                               T_BLE_BD_TYPE own_address_type, T_BLE_BD_TYPE peer_address_type,
                               uint8_t *p_peer_address, uint16_t adv_data_len, uint8_t *p_adv_data,
                               uint16_t scan_data_len, uint8_t *p_scan_data)
{
    uint8_t adv_handle = 0xFF;
    if (adv_mgr.ext_adv_support)
    {
        return ble_ext_adv_mgr_alloc_adv(app_callback, adv_event_prop,
                                         primary_adv_interval_min, primary_adv_interval_max,
                                         (T_GAP_LOCAL_ADDR_TYPE)own_address_type,
                                         (T_GAP_REMOTE_ADDR_TYPE)peer_address_type, p_peer_address,
                                         GAP_ADV_FILTER_ANY, adv_data_len, p_adv_data,
                                         scan_data_len, p_scan_data);

    }
    else
    {
        T_GAP_ADTYPE adv_type;
        if (!(adv_event_prop & EXT_ADV_EVT_PROP_USE_LEGACY_ADV))
        {
            APP_PRINT_ERROR1("ble_alloc_adv_instance: not Legacy event adv_event_prop %d", adv_event_prop);
            return 0xFF;
        }
        adv_handle = ble_legacy_adv_mgr_alloc_adv(app_callback);
        if (adv_handle != 0xFF)
        {
            if (adv_event_prop == LEGACY_ADV_IND_EVT)
            {
                adv_type = GAP_ADTYPE_ADV_IND;
            }
            else if (adv_event_prop == LEGACY_ADV_DIRECT_IND_LOW_EVT)
            {
                adv_type = GAP_ADTYPE_ADV_LDC_DIRECT_IND;
            }
            else if (adv_event_prop == LEGACY_ADV_DIRECT_IND_HIGH_EVT)
            {
                adv_type = GAP_ADTYPE_ADV_HDC_DIRECT_IND;
            }
            else if (adv_event_prop == LEGACY_ADV_SCAN_IND_EVT)
            {
                adv_type = GAP_ADTYPE_ADV_SCAN_IND;
            }
            else if (adv_event_prop == LEGACY_ADV_NONCONN_IND_EVT)
            {
                adv_type = GAP_ADTYPE_ADV_NONCONN_IND;
            }
            else
            {
                APP_PRINT_ERROR1("ble_alloc_adv_instance: adv_event_prop %d", adv_event_prop);
                return 0xFF;
            }
            ble_legacy_adv_mgr_set_adv_param(adv_handle, adv_type,
                                             primary_adv_interval_min, primary_adv_interval_max,
                                             (T_GAP_LOCAL_ADDR_TYPE)own_address_type,
                                             (T_GAP_REMOTE_ADDR_TYPE) peer_address_type, p_peer_address,
                                             GAP_ADV_FILTER_ANY);
            if (ble_legacy_adv_mgr_set_adv_data(adv_handle, adv_data_len, p_adv_data) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR0("ble_alloc_adv_instance: legacy adv set adv data fail");
            }
            if (ble_legacy_adv_mgr_set_adv_data(adv_handle, scan_data_len, p_scan_data) != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR0("ble_alloc_adv_instance: legacy adv set scan response data fail");
            }

        }
    }
    return adv_handle;
}


void ble_remove_adv(uint8_t adv_handle)
{
    if (adv_mgr.ext_adv_support)
    {
        ble_adv_mgr_rmv_ext_adv(adv_handle);
    }
    else
    {
        ble_legacy_adv_mgr_remove_adv(adv_handle);
    }
}

bool ble_enable_adv(uint8_t adv_handle, uint32_t duration_ms)
{
    bool ret = true;
    if (adv_mgr.ext_adv_support)
    {
        if (ble_enable_ext_adv(adv_handle, (uint16_t)(duration_ms / 10)) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }
    else
    {
        if (ble_legacy_adv_mgr_enable(adv_handle, duration_ms) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }
    return ret;
}

//FIX TODO app_cause needs to define
bool ble_disable_adv(uint8_t adv_handle, uint8_t app_cause)
{
    bool ret = true;
    if (adv_mgr.ext_adv_support)
    {
        if (ble_disable_ext_adv(adv_handle, app_cause) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }
    else
    {
        if (ble_legacy_adv_mgr_disable(adv_handle, app_cause) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }

    return ret;
}


bool ble_set_adv_param(uint8_t adv_handle, T_ADV_PARAM_CONF conf_msk, T_ADV_PARAMS *p_param)
{
    bool ret = true;
    if (conf_msk == 0 || p_param == NULL)
    {
        return true;
    }

    if (adv_mgr.ext_adv_support)
    {
        if (ble_adv_mgr_set_ext_adv_param(adv_handle, conf_msk, p_param) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }
    else
    {
        if (ble_legacy_set_adv_param_by_mask(adv_handle, conf_msk, p_param) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }

    return ret;
}


bool ble_set_adv_data(uint8_t adv_handle, uint16_t adv_data_len, uint8_t *p_adv_data)
{
    bool ret = true;
    if (adv_data_len == 0 || p_adv_data == NULL)
    {
        return false;
    }

    if (adv_mgr.ext_adv_support)
    {
        if (ble_adv_mgr_set_ext_adv_data(adv_handle, adv_data_len, p_adv_data) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }
    else
    {
        if (ble_legacy_adv_mgr_set_adv_data(adv_handle, adv_data_len, p_adv_data) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }

    return ret;
}

bool ble_set_scan_response_data(uint8_t adv_handle, uint16_t scan_data_len, uint8_t *p_scan_data)
{
    bool ret = true;
    if (scan_data_len == 0 || p_scan_data == NULL)
    {
        return false;
    }

    if (adv_mgr.ext_adv_support)
    {
        if (ble_adv_mgr_set_ext_scan_response_data(adv_handle, scan_data_len,
                                                   p_scan_data) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }
    else
    {
        if (ble_legacy_adv_mgr_set_scan_rsp_data(adv_handle, scan_data_len,
                                                 p_scan_data) != GAP_CAUSE_SUCCESS)
        {
            ret = false;
        }
    }

    return ret;
}


bool ble_adv_init_pa(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                     uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop,
                     uint16_t periodic_adv_data_len, uint8_t *p_periodic_adv_data)
{
    if (!adv_mgr.periodic_adv_support)
    {
        return false;
    }

    if (ble_ext_adv_mgr_alloc_pa(adv_handle, periodic_adv_interval_min,
                                 periodic_adv_interval_max, periodic_adv_prop,
                                 periodic_adv_data_len, p_periodic_adv_data) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }
    return true;
}

bool ble_adv_enable_pa(uint8_t adv_handle)
{
    if (!adv_mgr.periodic_adv_support)
    {
        return false;
    }

    if (ble_enable_pa(adv_handle) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }

    return true;
}

bool ble_adv_disable_pa(uint8_t adv_handle, uint8_t app_cause)
{
    if (!adv_mgr.periodic_adv_support)
    {
        return false;
    }

    if (ble_disable_pa(adv_handle, app_cause) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }

    return true;
}

bool ble_adv_update_pa_param(uint8_t adv_handle, uint16_t periodic_adv_interval_min,
                             uint16_t periodic_adv_interval_max, uint16_t periodic_adv_prop)
{
    if (!adv_mgr.periodic_adv_support)
    {
        return false;
    }

    if (ble_ext_adv_mgr_update_pa_param(adv_handle, periodic_adv_interval_min,
                                        periodic_adv_interval_max, periodic_adv_prop) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }

    return true;
}

bool ble_adv_update_set_pa_data(uint8_t adv_handle, uint16_t periodic_adv_data_len,
                                uint8_t *p_periodic_adv_data, bool pa_unchanged_data_flag)
{
    if (!adv_mgr.periodic_adv_support)
    {
        return false;
    }

    if (ble_ext_adv_mgr_set_pa_data(adv_handle, periodic_adv_data_len,
                                    p_periodic_adv_data, pa_unchanged_data_flag) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }

    return true;
}

#if F_BT_LE_5_2_ISOC_BIS_SUPPORT
bool ble_adv_create_big(uint8_t adv_handle, T_BIG_PARAM *p_param, P_ISOC_BROADCAST_CB callback)
{
    if (!le_check_supported_features(LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX3,
                                     LE_SUPPORT_FEATURES_ISOCHRONOUS_BROADCASTER_MASK_BIT))
    {
        return false;
    }

    if (le_create_big(adv_handle, p_param, callback) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }

    return true;
}

bool ble_adv_terminate_big(uint8_t big_handle, uint8_t reason)
{
    if (!le_check_supported_features(LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX3,
                                     LE_SUPPORT_FEATURES_ISOCHRONOUS_BROADCASTER_MASK_BIT))
    {
        return false;
    }

    if (le_terminate_big(big_handle, reason) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }

    return true;
}

bool ble_adv_big_setup_data_path(uint16_t bis_conn_handle, uint8_t data_path_direction,
                                 uint8_t data_path_id, uint8_t codec_id[5], uint32_t controller_delay,
                                 uint8_t codec_config_len, uint8_t *p_codec_config)
{
    if (gap_big_mgr_setup_data_path(bis_conn_handle, data_path_direction, data_path_id,
                                    codec_id, controller_delay,
                                    codec_config_len, p_codec_config) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }

    return true;
}

bool ble_adv_big_remove_data_path(uint16_t bis_conn_handle, uint8_t data_path_direction)
{
    if (gap_big_mgr_remove_data_path(bis_conn_handle, data_path_direction) != GAP_CAUSE_SUCCESS)
    {
        return false;
    }

    return true;
}
#endif

void ble_adv_mgr_init()
{
    memset(&adv_mgr, 0, sizeof(T_ADV_MGR_CB));
    if (le_check_supported_features(LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1,
                                    LE_SUPPORT_FEATURES_LE_EXTENDED_ADV_BIT))
    {
        adv_mgr.ext_adv_support = true;
        le_ext_adv_get_param(GAP_PARAM_EXT_ADV_MAX_DATA_LEN, &adv_mgr.max_ext_adv_len);
        le_ext_adv_get_param(GAP_PARAM_EXT_ADV_MAX_SETS, &adv_mgr.max_num_of_adv_sets);
        ble_extend_adv_mgr_init();
        if (le_check_supported_features(LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1,
                                        LE_SUPPORT_FEATURES_LE_PERIODIC_ADV_MASK_BIT))
        {
            adv_mgr.periodic_adv_support = true;
            ble_pa_mgr_init();
        }
    }
    else
    {
        ble_legacy_adv_mgr_init();
    }

}

bool ble_is_ext_adv()
{
    return adv_mgr.ext_adv_support;
}
