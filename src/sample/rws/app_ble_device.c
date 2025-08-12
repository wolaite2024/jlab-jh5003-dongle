/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "gap_bond_le.h"
#include "sysm.h"
#include "btm.h"
#include "app_cfg.h"
#include "app_key_process.h"
#include "app_ble_gap.h"
#include "app_bt_policy_api.h"
#include "app_ble_common_adv.h"
#include "app_main.h"
#include "app_ble_bond.h"
#include "app_charger.h"
#include "ble_adv_data.h"
#include "app_tts.h"
#include "iap_stream.h"
#include "spp_stream.h"
#include "app_ble_rand_addr_mgr.h"
#include "string.h"
#include "platform_utils.h"
#if F_APP_RWS_BLE_USE_RPA_SUPPORT
#include "bt_bond_le.h"
#include "app_relay.h"
#include "app_ble_device.h"
#endif
#if F_APP_BOND_MGR_BLE_SYNC
#include "ble_bond_sync.h"
#endif
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
#include "bt_bond_le.h"
#include "app_bond.h"
#endif
#if F_APP_IAP_SUPPORT
#include "app_iap.h"
#endif

#if F_APP_IAP_RTK_SUPPORT
#include "app_iap_rtk.h"
#endif

#if F_APP_TUYA_SUPPORT
#include "rtk_tuya_ble_device.h"
#endif
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_gfps_device.h"
#include "app_gfps.h"
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT

#include "app_gfps_finder_adv.h"
#endif
#endif
#if AMA_FEATURE_SUPPORT
#include "app_ama_accessory.h"
#include "app_ama_device.h"
#include "app_ama_roleswap.h"
#endif
#if BISTO_FEATURE_SUPPORT
#include "app_bisto_ble.h"
#include "app_bisto.h"
#include "app_bisto_roleswap.h"
#endif
#if XM_XIAOAI_FEATURE_SUPPORT
#include "app_xiaoai_device.h"
#include "app_xiaoai_record.h"
#endif
#if F_APP_BLE_SWIFT_PAIR_SUPPORT
#include "app_ble_swift_pair.h"
#endif
#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
#include "app_teams_rws.h"
#endif
#if F_APP_TEAMS_BT_POLICY
#include "app_teams_ble_policy.h"
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
#include "app_xiaowei_device.h"
#include "app_xiaowei_record.h"
#include "app_xiaowei.h"
#endif
#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
#include "app_rtk_fast_pair_adv.h"
#endif
#if DMA_FEATURE_SUPPORT
#include "app_dma_ble.h"
#include "app_dma_device.h"
#endif

#if F_APP_DURIAN_SUPPORT
#include "app_durian.h"
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
#include "fmna_connection.h"
#include "app_findmy_ble_adv.h"
#endif

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
#include "app_gfps.h"
#endif

#if F_APP_GATT_CACHING_SUPPORT
#include "app_ble_gatt_caching.h"
#endif

void app_ble_device_handle_factory_reset(void)
{
    APP_PRINT_INFO0("app_ble_device_handle_factory_reset");
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT

#if F_APP_COMMON_DONGLE_SUPPORT
#if F_APP_KEEP_LEA_BOND && (F_APP_GAMING_DONGLE_SUPPORT == 0)
    app_bond_le_clear_non_lea_keys();
#endif

#else
    bt_le_clear_all_keys();
#endif

#else
    le_bond_clear_all_keys();
#endif

#if F_APP_GATT_CACHING_SUPPORT
    app_ble_gatt_caching_clear_service_change_state_all();
    app_ble_gatt_caching_clear_database_hash();
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_device_handle_factory_reset();
    }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_handle_factory_reset();
    }
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        app_tuya_handle_factory_reset();
    }
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
    fmna_connection_set_is_fmna_paired(false);
#endif
}

void app_ble_device_handle_power_on_rtk_adv(void)
{
    APP_PRINT_INFO2("app_ble_device_handle_power_on_rtk_adv: timer_ota_adv_timeout %d, enable_power_on_adv_with_timeout %d",
                    app_cfg_const.timer_ota_adv_timeout, app_cfg_const.enable_power_on_adv_with_timeout);

    if (app_cfg_const.timer_ota_adv_timeout == 0)
    {
        /*power on always advertising*/
        app_ble_common_adv_start_rws(0);
    }
    else if (app_cfg_const.enable_power_on_adv_with_timeout)
    {
        /*power on advertisng with timeout*/
        app_ble_common_adv_start_rws(app_cfg_const.timer_ota_adv_timeout * 100);
    }
}

#if F_APP_RWS_BLE_USE_RPA_SUPPORT
#if F_APP_ERWS_SUPPORT
static void app_ble_device_irk_sync_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                                T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("app_ble_device_irk_sync_parse_cback: msg_type 0x%04x, status %d", msg_type,
                     status);
    switch (msg_type)
    {
    case APP_REMOTE_MSG_SECONDARY_SYNC_PRIMARY_IRK:
        if ((status == REMOTE_RELAY_STATUS_ASYNC_RCVD) &&
            (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
        {
            memcpy(app_cfg_nv.primary_irk, buf, 16);
            app_cfg_store(app_cfg_nv.primary_irk, 16);
        }
        break;

    case APP_REMOTE_MSG_PRIMARY_SYNC_SECONDARY_IRK:
        if ((status == REMOTE_RELAY_STATUS_ASYNC_RCVD) &&
            (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
        {
            memcpy(app_cfg_nv.secondary_irk, buf, 16);
            app_cfg_store(app_cfg_nv.secondary_irk, 16);
        }
        break;

    default:
        break;
    }
}
#endif

#if F_APP_ERWS_SUPPORT
static uint16_t app_ble_device_irk_sync_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;
    APP_PRINT_INFO1("app_ble_device_irk_sync_relay_cback: msg_type %d", msg_type);

    switch (msg_type)
    {
    case APP_REMOTE_MSG_SECONDARY_SYNC_PRIMARY_IRK:
        {
            payload_len = 16;
            msg_ptr = (uint8_t *)app_cfg_nv.primary_irk;
            skip = false;
        }
        break;

    case APP_REMOTE_MSG_PRIMARY_SYNC_SECONDARY_IRK:
        {
            payload_len = 16;
            msg_ptr = (uint8_t *)app_cfg_nv.secondary_irk;
            skip = false;
        }
        break;

    default:
        break;
    }

    uint16_t msg_len = payload_len + 4;
    if ((total == true) && (skip == true))
    {
        msg_len = 0;
    }

    if (buf != NULL)
    {
        if (((total == true) && (skip == false)) || (total == false))
        {
            buf[0] = (uint8_t)(msg_len & 0xFF);
            buf[1] = (uint8_t)(msg_len >> 8);
            buf[2] = APP_MODULE_TYPE_BLE_IRK_SYNC;
            buf[3] = msg_type;
            if (payload_len != 0 && msg_ptr != NULL)
            {
                memcpy(&buf[4], msg_ptr, payload_len);
            }
        }
        APP_PRINT_INFO1("app_ble_device_irk_sync_relay_cback: buf %b", TRACE_BINARY(msg_len, buf));
    }
    return msg_len;
}
#endif

#if F_APP_ERWS_SUPPORT
bool app_ble_device_irk_sync_relay_init(void)
{
    bool ret = app_relay_cback_register(app_ble_device_irk_sync_relay_cback,
                                        app_ble_device_irk_sync_parse_cback,
                                        APP_MODULE_TYPE_BLE_IRK_SYNC, APP_REMOTE_MSG_IRK_MAX_MSG_NUM);
    return ret;
}
#endif

static void app_ble_device_irk_handle_power_on(void)
{
    uint8_t temp_irk[16] = {0};

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        if (memcmp(app_cfg_nv.primary_irk, temp_irk, 16) == 0)
        {
            uint8_t primary_irk[16] = {0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33};
            uint8_t *p = primary_irk;

            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));

            memcpy(app_cfg_nv.primary_irk, primary_irk, 16);
            app_cfg_store(app_cfg_nv.primary_irk, 16);
        }

        bt_le_set_local_irk(GAP_IDENT_ADDR_PUBLIC, app_cfg_nv.bud_local_addr, app_cfg_nv.primary_irk);
        /*patch need modify le_bond_set_param and call sm_setup_local_irk when set GAP_PARAM_BOND_SET_LOCAL_IRK so it can work after stack ready*/
        le_bond_set_param(GAP_PARAM_BOND_SET_LOCAL_IRK, GAP_KEY_LEN, app_cfg_nv.primary_irk);

        /*use app_cfg_nv.primary_irk generate RPA*/
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
        app_gfps_update_rpa();
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
        gfps_finder_adv_update_rpa();
#endif
#endif
    }

    APP_PRINT_INFO4("app_ble_device_irk_handle_power_on: bud_role %d, local_addr %b, primary_irk %b, secondary_irk %b",
                    app_cfg_nv.bud_role, TRACE_BDADDR(app_cfg_nv.bud_local_addr),
                    TRACE_BINARY(16, app_cfg_nv.primary_irk), TRACE_BINARY(16, app_cfg_nv.secondary_irk));
}

static void app_ble_device_irk_handle_engage_role_decided(void)
{
    uint8_t temp_irk[16] = {0};

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        if (memcmp(app_cfg_nv.primary_irk, temp_irk, 16) == 0)
        {
            uint8_t primary_irk[16] = {0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33};
            uint8_t *p = primary_irk;

            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));

            memcpy(app_cfg_nv.primary_irk, primary_irk, 16);
            app_cfg_store(app_cfg_nv.primary_irk, 16);
        }

        bt_le_set_local_irk(GAP_IDENT_ADDR_PUBLIC, app_cfg_nv.bud_local_addr, app_cfg_nv.primary_irk);
        /*patch need modify le_bond_set_param and call sm_setup_local_irk when set GAP_PARAM_BOND_SET_LOCAL_IRK so it can work after stack ready*/
        le_bond_set_param(GAP_PARAM_BOND_SET_LOCAL_IRK, GAP_KEY_LEN, app_cfg_nv.primary_irk);

        /*use app_cfg_nv.primary_irk generate RPA*/
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
        app_gfps_update_rpa();
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
        gfps_finder_adv_update_rpa();
#endif
#endif
    }
    else
    {
        if (memcmp(app_cfg_nv.secondary_irk, temp_irk, 16) == 0)
        {
            uint8_t secondary_irk[16] = {0x44, 0x55, 0x66, 0x77, 0x44, 0x55, 0x66, 0x77, 0x44, 0x55, 0x66, 0x77, 0x44, 0x55, 0x66, 0x77};
            uint8_t *p = secondary_irk;

            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));
            BE_UINT32_TO_STREAM(p, platform_random(0xFFFFFFFF));

            memcpy(app_cfg_nv.secondary_irk, secondary_irk, 16);
            app_cfg_store(app_cfg_nv.secondary_irk, 16);
        }

        bt_le_set_local_irk(GAP_IDENT_ADDR_PUBLIC, app_cfg_nv.bud_local_addr, app_cfg_nv.secondary_irk);
        le_bond_set_param(GAP_PARAM_BOND_SET_LOCAL_IRK, GAP_KEY_LEN, app_cfg_nv.secondary_irk);

        /*use app_cfg_nv.secondary_irk generate RPA*/
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
        app_gfps_update_rpa();
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
        gfps_finder_adv_update_rpa();
#endif
#endif
    }

    APP_PRINT_INFO4("app_ble_device_irk_handle_engage_role_decided: bud_role %d, local_addr %b, primary_irk %b, secondary_irk %b",
                    app_cfg_nv.bud_role, TRACE_BDADDR(app_cfg_nv.bud_local_addr),
                    TRACE_BINARY(16, app_cfg_nv.primary_irk), TRACE_BINARY(16, app_cfg_nv.secondary_irk));
}

static void app_ble_device_irk_handle_remote_conn_cmpl(void)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_BLE_IRK_SYNC,
                                            APP_REMOTE_MSG_SECONDARY_SYNC_PRIMARY_IRK,
                                            (uint8_t *)app_cfg_nv.primary_irk, 16);
    }
    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_BLE_IRK_SYNC,
                                            APP_REMOTE_MSG_PRIMARY_SYNC_SECONDARY_IRK,
                                            (uint8_t *)app_cfg_nv.secondary_irk, 16);
    }
}

static void app_ble_device_irk_handle_role_swap(void)
{
    if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        bt_le_set_local_irk(GAP_IDENT_ADDR_PUBLIC, app_cfg_nv.bud_local_addr, app_cfg_nv.primary_irk);
        le_bond_set_param(GAP_PARAM_BOND_SET_LOCAL_IRK, GAP_KEY_LEN, app_cfg_nv.primary_irk);

        /*use app_cfg_nv.primary_irk generate RPA*/
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
        app_gfps_update_rpa();
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
        gfps_finder_adv_update_rpa();
#endif
#endif
    }
    else
    {
        bt_le_set_local_irk(GAP_IDENT_ADDR_PUBLIC, app_cfg_nv.bud_local_addr, app_cfg_nv.secondary_irk);
        le_bond_set_param(GAP_PARAM_BOND_SET_LOCAL_IRK, GAP_KEY_LEN, app_cfg_nv.secondary_irk);

        /*use app_cfg_nv.secondary_irk generate RPA*/
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
        app_gfps_update_rpa();
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
        gfps_finder_adv_update_rpa();
#endif
#endif
    }

    APP_PRINT_INFO4("app_ble_device_irk_handle_role_swap: bud_role %d, local_addr %b, primary_irk %b, secondary_irk %b",
                    app_cfg_nv.bud_role, TRACE_BDADDR(app_cfg_nv.bud_local_addr),
                    TRACE_BINARY(16, app_cfg_nv.primary_irk), TRACE_BINARY(16, app_cfg_nv.secondary_irk));
}
#endif

void app_ble_device_handle_power_on(void)
{
    APP_PRINT_INFO0("app_ble_device_handle_power_on");
    bool is_pairing = app_key_is_enter_pairing();
    ble_ext_adv_print_info();
    ble_adv_data_enable();

#if F_APP_RWS_BLE_USE_RPA_SUPPORT
    app_ble_device_irk_handle_power_on();
#endif

    if (app_cfg_const.rtk_app_adv_support && (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        app_ble_device_handle_power_on_rtk_adv();
    }

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_device_handle_power_on(is_pairing);
    }
#endif

#if AMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.ama_support)
    {
        app_ama_accessory_handle_power_on(is_pairing);
    }
#endif

#if DMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.dma_support)
    {
        app_dma_ble_adv_start();
    }
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        app_xiaoai_handle_power_on(is_pairing);
    }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_handle_power_on(is_pairing);
    }
#endif

#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_handle_power_on(is_pairing);
    }
#endif

#if F_APP_BLE_SWIFT_PAIR_SUPPORT
    if (app_cfg_const.swift_pair_support)
    {
        app_swift_pair_handle_power_on();
    }
#endif

#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
    if (app_cfg_const.enable_rtk_fast_pair_adv)
    {
        app_rtk_fast_pair_handle_power_on();
    }
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        app_tuya_handle_power_on();
    }
#endif
}

void app_ble_device_handle_power_off(void)
{
    APP_PRINT_INFO0("app_ble_device_handle_power_off");

    ble_adv_data_disable();

    if (app_cfg_const.rtk_app_adv_support)
    {
        if (app_ble_common_adv_get_state() != BLE_EXT_ADV_MGR_ADV_DISABLED)
        {
            app_ble_common_adv_stop(APP_STOP_ADV_CAUSE_POWER_OFF);
        }
    }
#if F_APP_TTS_SUPPORT
    if (app_cfg_const.tts_support)
    {
        app_tts_handle_power_off();
    }
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_device_handle_power_off();
    }
#endif

#if AMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.ama_support)
    {
        app_ama_accessory_handle_power_off();
    }
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        app_xiaoai_handle_power_off();
    }
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_handle_power_off();
    }
#endif

#if F_APP_BLE_SWIFT_PAIR_SUPPORT
    if (app_cfg_const.swift_pair_support)
    {
        app_swift_pair_handle_power_off();
    }
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        app_tuya_handle_power_off();
    }
#endif

    app_ble_gap_disconnect_all(LE_LOCAL_DISC_CAUSE_POWER_OFF);
}

void app_ble_device_handle_radio_mode(T_BT_DEVICE_MODE radio_mode)
{
    static T_BT_DEVICE_MODE radio_mode_pre = BT_DEVICE_MODE_IDLE;
    APP_PRINT_INFO2("app_ble_device_handle_radio_mode: radio_mode %d radio_mode_pre %d", radio_mode,
                    radio_mode_pre);
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support &&
        !extend_app_cfg_const.gfps_le_device_support)
    {
        app_gfps_device_handle_radio_mode(radio_mode);
    }
#endif

    if (radio_mode == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
    {
        //rtk_adv_enable();
    }
    else
    {
        //rtk_adv_disable();
#if F_APP_DURIAN_SUPPORT
        if (radio_mode_pre == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
        {
            app_durian_adv_pairing_finish();
        }
#endif
    }

    radio_mode_pre = radio_mode;
}

void app_ble_device_handle_enter_pair_mode(void)
{
    APP_PRINT_INFO0("app_ble_device_handle_enter_pair_mode");

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        app_xiaoai_handle_enter_pair_mode();
    }
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_handle_enter_pair_mode();
    }
#endif

#if AMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.ama_support)
    {
        app_ama_accessory_handle_enter_pair_mode();
    }
#endif

#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_handle_enter_pair_mode();
    }
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        app_tuya_handle_enter_pair_mode();
    }
#endif

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        APP_PRINT_INFO0("app_ble_device_handle_enter_pair_mode: gfps force enter pairing mode");
        app_gfps_le_device_adv_start((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    }
#endif

    if (app_cfg_const.rtk_app_adv_support)
    {
        app_ble_device_handle_power_on_rtk_adv();
    }
}

void app_ble_device_handle_engage_role_decided(T_BT_DEVICE_MODE radio_mode)
{
    APP_PRINT_INFO3("app_ble_device_handle_engage_role_decided: bud_role %d, radio_mode %d, first_engaged %d",
                    app_cfg_nv.bud_role, radio_mode, app_cfg_nv.first_engaged);
#if F_APP_RWS_BLE_USE_RPA_SUPPORT
    app_ble_device_irk_handle_engage_role_decided();
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_device_handle_engage_role_decided();
    }
#endif

#if AMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.ama_support)
    {
        app_ama_roleswap_handle_engage_role_decided();
    }
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        app_xiaoai_handle_engage_role_decided(radio_mode);
    }
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_handle_engage_role_decided(radio_mode);
    }
#endif

#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_roleswap_role_decide();
    }
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        app_tuya_handle_engage_role_decided();
    }
#endif

    if (app_cfg_const.rtk_app_adv_support)
    {
        app_ble_common_adv_handle_engage_role_decided();
    }

    app_ble_rand_addr_handle_role_decieded();
}

void app_ble_device_handle_role_swap(T_BT_EVENT_PARAM_REMOTE_ROLESWAP_STATUS *p_role_swap_status)
{
    APP_PRINT_INFO2("app_ble_device_handle_role_swap: status 0x%02x, device_role %d",
                    p_role_swap_status->status, p_role_swap_status->device_role);

#if F_APP_RWS_BLE_USE_RPA_SUPPORT
    app_ble_device_irk_handle_role_swap();
#endif

    if (p_role_swap_status->status == BT_ROLESWAP_STATUS_SUCCESS)
    {
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
        if (extend_app_cfg_const.gfps_support)
        {
            app_gfps_device_handle_role_swap(p_role_swap_status->device_role);
        }
#endif

#if AMA_FEATURE_SUPPORT
        if (extend_app_cfg_const.ama_support)
        {
            app_ama_roleswap_handle_role_swap_success(p_role_swap_status->device_role);
        }
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
        if (extend_app_cfg_const.xiaoai_support)
        {
            app_xiaoai_handle_role_swap_success(p_role_swap_status->device_role);
        }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
        if (extend_app_cfg_const.xiaowei_support)
        {
            app_xiaowei_handle_role_swap_success(p_role_swap_status->device_role);
        }
#endif

#if BISTO_FEATURE_SUPPORT
        if (extend_app_cfg_const.bisto_support)
        {
            app_bisto_roleswap_success(p_role_swap_status->device_role);
        }
#endif

#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
        app_teams_rws_handle_role_swap_success(p_role_swap_status->device_role);
#endif

#if F_APP_TUYA_SUPPORT
        if (extend_app_cfg_const.tuya_support)
        {
            app_tuya_handle_role_swap_success(p_role_swap_status->device_role);
        }
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
        app_findmy_handle_role_swap_success(p_role_swap_status->device_role);
#endif

        if (app_cfg_const.rtk_app_adv_support)
        {
            app_ble_common_adv_handle_roleswap(p_role_swap_status->device_role);
        }
    }
    else if (p_role_swap_status->status == BT_ROLESWAP_STATUS_FAIL)
    {
#if XM_XIAOAI_FEATURE_SUPPORT
        if (extend_app_cfg_const.xiaoai_support)
        {
            app_xiaoai_handle_role_swap_fail(p_role_swap_status->device_role);
        }
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
        if (extend_app_cfg_const.xiaowei_support)
        {
            app_xiaowei_handle_role_swap_fail(p_role_swap_status->device_role);
        }
#endif

#if AMA_FEATURE_SUPPORT
        if (extend_app_cfg_const.ama_support)
        {
            app_ama_roleswap_handle_role_swap_fail(p_role_swap_status->device_role);
        }
#endif

#if BISTO_FEATURE_SUPPORT
        if (extend_app_cfg_const.bisto_support)
        {
            app_bisto_roleswap_failed();
        }
#endif

#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
        app_teams_rws_handle_role_swap_failed(p_role_swap_status->device_role);
#endif

#if F_APP_TUYA_SUPPORT
        if (extend_app_cfg_const.tuya_support)
        {
            app_tuya_handle_role_swap_fail(p_role_swap_status->device_role);
        }
#endif

        if (app_cfg_const.rtk_app_adv_support)
        {
            app_ble_common_adv_handle_role_swap_fail(p_role_swap_status->device_role);
        }
    }
}

void app_ble_device_handle_remote_conn_cmpl(void)
{
#if F_APP_RWS_BLE_USE_RPA_SUPPORT
    app_ble_device_irk_handle_remote_conn_cmpl();
#endif

#if AMA_FEATURE_SUPPORT | BISTO_FEATURE_SUPPORT | F_APP_IAP_RTK_SUPPORT
    iap_stream_send_all_info_to_sec();
    spp_stream_send_all_info_to_sec();
#endif

#if F_APP_BLE_BOND_SYNC_SUPPORT
    app_ble_bond_sync_all_info();
#endif

#if F_APP_BOND_MGR_BLE_SYNC
    ble_bond_sync_handle_remote_conn_cmpl();
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_device_handle_remote_conn_cmpl();
    }
#endif

#if AMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.ama_support)
    {
        app_ama_roleswap_handle_remote_conn_cmpl();
    }
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        app_xiaoai_handle_remote_conn_cmpl();
    }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_handle_remote_conn_cmpl();
    }
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        app_tuya_handle_remote_conn_cmpl();
    }
#endif

#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_roleswap_handle_remote_conn_cmpl();
    }
#endif

#if F_APP_RTK_FAST_PAIR_ADV_FEATURE_SUPPORT
    if (app_cfg_const.enable_rtk_fast_pair_adv)
    {
        app_rtk_fast_pair_handle_remote_conn_cmpl();
    }
#endif

#if F_APP_IAP_SUPPORT
    app_iap_handle_remote_conn_cmpl();
#endif

#if F_APP_IAP_RTK_SUPPORT
    app_iap_rtk_handle_remote_conn_cmpl();
#endif

    app_ble_rand_addr_handle_remote_conn_cmpl();
}

void app_ble_device_handle_remote_disconn_cmpl(void)
{
#if F_APP_BLE_BOND_SYNC_SUPPORT
    app_ble_bond_sync_recovery();
#endif

#if F_APP_BOND_MGR_BLE_SYNC
    ble_bond_sync_handle_disconn_cmpl();
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        app_xiaoai_handle_remote_disconn_cmpl();
    }
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_handle_remote_disconn_cmpl();
    }
#endif

#if AMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.ama_support)
    {
        app_ama_roleswap_handle_remote_disconn_cmpl();
    }
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    app_gfps_device_handle_remote_disconnect_cmpl();
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        app_tuya_handle_remote_disconn_cmpl();
    }
#endif
}

void app_ble_device_handle_b2s_bt_connected(uint8_t *bd_addr)
{
#if AMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.ama_support)
    {
        app_ama_device_handle_b2s_connected(bd_addr);
    }
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        app_xiaoai_handle_b2s_bt_connected(bd_addr);
    }
#endif

#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_bt_handle_b2s_connected(bd_addr);
    }
#endif

#if DMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.dma_support)
    {
        app_dma_device_handle_b2s_connected(bd_addr);
    }
#endif
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        app_gfps_handle_b2s_connected(bd_addr);
    }
#endif
#if F_APP_TEAMS_BT_POLICY
    T_TEAMS_BLE_ADV_MGR *p_teams_ble_adv_mgr = NULL;
    T_APP_BR_LINK *p_link = NULL;
    if (le_find_key_entry(bd_addr, GAP_REMOTE_ADDR_LE_PUBLIC))
    {
        p_link = app_link_find_br_link(bd_addr);
        if (p_link)
        {
            p_teams_ble_adv_mgr = app_teams_ble_policy_find_ble_adv_mgr_by_idx(p_link->id);
            if (p_teams_ble_adv_mgr)
            {
                app_teams_ble_policy_set_adv_param(p_teams_ble_adv_mgr->handle, bd_addr);
                app_teams_ble_policy_adv_start(p_teams_ble_adv_mgr->handle, APP_TEAMS_BLE_POLICY_ADV_DURATION);
            }
        }
    }
#endif

}

void app_ble_device_handle_b2s_bt_disconnected(uint8_t *bd_addr)
{
#if AMA_FEATURE_SUPPORT
    if (extend_app_cfg_const.ama_support)
    {
        app_ama_device_handle_b2s_disconnected(bd_addr);
    }
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaoai_support)
    {
        app_xiaoai_handle_b2s_bt_disconnected(bd_addr);
    }
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (extend_app_cfg_const.xiaowei_support)
    {
        app_xiaowei_handle_b2s_bt_disconnected(bd_addr);
    }
#endif

#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_bt_handle_b2s_disconnected(bd_addr);
    }
#endif

#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        app_tuya_handle_b2s_bt_disconnected(bd_addr);
    }
#endif
}

static void app_ble_device_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        app_ble_device_handle_role_swap(&param->remote_roleswap_status);
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_ble_device_bt_cback: event_type 0x%04x", event_type);
    }
}

bool app_ble_device_is_active(void)
{
    bool ret = false;

#if XM_XIAOAI_FEATURE_SUPPORT
    if (app_xiaoai_is_recording())
    {
        ret = true;
    }
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
    if (app_xiaowei_is_recording())
    {
        ret = true;
    }
#endif

    return ret;
}

void app_ble_device_init(void)
{
    bt_mgr_cback_register(app_ble_device_bt_cback);
}
