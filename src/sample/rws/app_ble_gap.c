/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <string.h>
#include "trace.h"
#include "sysm.h"
#include "gap.h"
#include "ringtone.h"
#include "gap_conn_le.h"
#include "gap_bond_le.h"
#include "gap_ext_scan.h"
#include "dfu_api.h"
#include "ota_service.h"
#include "app_ble_gap.h"
#include "app_ble_service.h"
#include "app_main.h"
#include "app_transfer.h"
#include "app_report.h"
#include "app_cfg.h"
#include "remote.h"
#include "app_ble_common_adv.h"
#include "ble_conn.h"
#include "app_ble_device.h"
#include "app_cmd.h"
#include "ble_stream.h"
#include "ble_adv_data.h"
#include "app_bt_policy_api.h"
#include "app_audio_policy.h"
#include "app_ble_bond.h"
#include "app_roleswap.h"
#include "app_roleswap_control.h"
#include "app_auto_power_off.h"
#include "app_bt_sniffing.h"
#include "app_ble_whitelist.h"
#include "app_ota.h"
#include "ble_mgr.h"
#include "app_timer.h"
#include "gap_vendor.h"
#include "app_ble_rand_addr_mgr.h"
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
#include "gap_bond_manager.h"
#include "bt_bond_api.h"
#include "bt_bond_le.h"
#endif
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_gfps_device.h"
#include "app_gfps.h"
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
#include "app_gfps_finder.h"
#endif
#endif
#if XM_XIAOAI_FEATURE_SUPPORT
#include "app_xiaoai_transport.h"
#include "app_xm_xiaoai_ble_adv.h"
#include "app_xiaoai_device.h"
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
#include "app_xiaowei_transport.h"
#include "app_xiaowei_ble_adv.h"
#include "app_xiaowei_device.h"
#endif
#if AMA_FEATURE_SUPPORT
#include "app_ama_ble.h"
#include "app_ama.h"
#include "app_ama_transport.h"
#include "app_ama_device.h"
#endif
#if BISTO_FEATURE_SUPPORT
#include "bisto_api.h"
#include "app_bisto_ble.h"
#endif
#if F_APP_LEA_SUPPORT
#include "app_lea_adv.h"
#include "app_lea_pacs.h"
#include "app_lea_unicast_audio.h"
#endif
#if GATTC_TBL_STORAGE_SUPPORT
#include "gattc_tbl_storage.h"
#endif
#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
#include "bt_gatt_client.h"
#endif
#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
#include "asp_device_link.h"
#endif
#if F_APP_TUYA_SUPPORT
#include "rtk_tuya_ble_device.h"
#include "rtk_tuya_ble_adv.h"
#include "rtk_tuya_ble_app_demo.h"
#endif
#if F_APP_FINDMY_FEATURE_SUPPORT
#include "fmna_adv.h"
#include "fmna_crypto.h"
#include "fmna_state_machine.h"
#include "fmna_connection_platform.h"
#include "gap_adv.h"
#include "tps.h"
#endif
#if F_APP_SC_KEY_DERIVE_SUPPORT
#include "app_ble_sc_key_derive.h"
#endif

#if F_APP_GAMING_DONGLE_SUPPORT || F_APP_LEA_DONGLE_BINDING
#include "app_dongle_common.h"
#endif
#if F_APP_GAMING_DONGLE_SUPPORT
#include "app_dongle_dual_mode.h"
#if F_APP_B2B_ENGAGE_REDUCE_NSE
#include "app_vendor.h"
#endif
#endif
#if F_APP_GATT_CACHING_SUPPORT
#include "app_ble_gatt_caching.h"
#endif
#if F_APP_BOND_MGR_BLE_SYNC
#include "ble_bond_sync.h"
#endif

#if F_APP_EATT_SUPPORT
#include "app_ble_eatt.h"
#endif

#include "app_bt_point.h"

#if F_APP_HAS_SUPPORT
#include "app_lea_has.h"
#endif

#if F_APP_GAMING_LE_FIX_CHANNEL_SUPPORT
#include "gap_callback_le.h"
#include "transmit_svc_dongle.h"
#endif

#define LE_DATA_LEN_TX_OCTETS      251     //Maximum allowable setting value
#define LE_DATA_LEN_TX_TIME        0x0848  //ms, refers to the setting value of upperstack

typedef enum
{
    APP_TIMER_GAP_DISCONNECT_DELAY,
    APP_TIMER_GAP_WAIT_FOR_AUTHEN,
    APP_TIMER_GAP_RECOVER_LINK_PRIORITY,
} T_APP_GAP_TIMER;

static T_GAP_DEV_STATE le_state;
static uint8_t timer_idx_gap_delay_disconn = 0;
static uint8_t timer_idx_gap_wait_for_authen = 0;

#if F_APP_INCREASE_RTK_APP_LINK_PRIORITY_TEMPORARILY
static uint8_t priority_adjust_conn_id = 0;
static uint8_t timer_idx_recover_link_priority = 0;
#endif

static uint8_t app_ble_gap_timer_id = 0;
static uint8_t authen_fail_conn_id = 0xFF;
static uint8_t wait_for_authen_conn_id = 0xFF;

uint8_t scan_rsp_data_len;
uint8_t scan_rsp_data[GAP_MAX_LEGACY_ADV_LEN];

#if F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY
uint8_t le_public_bd_addr[6] = {0};
#endif

bool app_ble_gap_gen_scan_rsp_data(uint8_t *p_scan_len, uint8_t *p_scan_data)
{
    uint8_t device_name_len;

    if (p_scan_len == NULL || p_scan_data == NULL)
    {
        return false;
    }

    device_name_len = strlen((const char *)app_cfg_nv.device_name_le);

    if (device_name_len > GAP_MAX_LEGACY_ADV_LEN - 2)
    {
        device_name_len = GAP_MAX_LEGACY_ADV_LEN - 2;
    }

    p_scan_data[0] = device_name_len + 1;
    p_scan_data[1] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&p_scan_data[2], app_cfg_nv.device_name_le, device_name_len);
    *p_scan_len = device_name_len + 2;
    return true;
}

T_LE_LINK_ENCRYPTION_STATE app_ble_gap_get_link_encryption_status(T_APP_LE_LINK *p_link)
{
    if (p_link)
    {
        return (T_LE_LINK_ENCRYPTION_STATE)p_link->encryption_status;
    }
    else
    {
        APP_PRINT_ERROR0("app_ble_gap_get_link_encryption_status: link not exist");
        return LE_LINK_ERROR;
    }
}

static void app_ble_gap_handle_dev_state_change_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_TRACE5("app_ble_gap_handle_dev_state_change_evt: le_state.gap_adv_state %d, new_state.gap_adv_state %d, "
                     "le_state.gap_scan_state %d, new_state.gap_scan_state %d, cause 0x%04x",
                     le_state.gap_adv_state,
                     new_state.gap_adv_state,
                     le_state.gap_scan_state,
                     new_state.gap_scan_state,
                     cause);

    if (le_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_INFO8("app_ble_gap_handle_dev_state_change_evt: GAP stack ready role %d, tts_support %d, gfps_support %d, rtk_app_adv_support %d, ama_support %d, bisto_support %d, xiaoai_support %d, xiaowei_support %d",
                            app_cfg_nv.bud_role,
                            app_cfg_const.tts_support,
                            extend_app_cfg_const.gfps_support,
                            app_cfg_const.rtk_app_adv_support,
                            extend_app_cfg_const.ama_support,
                            extend_app_cfg_const.bisto_support,
                            extend_app_cfg_const.xiaoai_support,
                            extend_app_cfg_const.xiaowei_support);
#if F_APP_FINDMY_FEATURE_SUPPORT
            fmna_log_serial_number();
            le_adv_read_tx_power();
            /*stack ready*/
            fmna_state_machine_init();
#endif
        }
    }

    if (le_state.gap_adv_state != new_state.gap_adv_state)
    {
        app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_DEV_STATE_CHANGE);
    }

    le_state = new_state;
}

static void app_ble_gap_handle_disconnecting_evt(T_APP_LE_LINK *p_link)
{
    if (p_link != NULL)
    {
        p_link->state = LE_LINK_STATE_DISCONNECTING;
    }
}

#if TARGET_LE_AUDIO_GAMING
static bool app_ble_gap_check_resume_common_adv(T_APP_LE_LINK *p_link)
{
    bool ret = false;

    if (memcmp(p_link->bd_addr, app_cfg_nv.dongle_addr, 6) &&
        app_link_le_check_common_link_exist() == false)
    {
        ret = true;
    }

    return ret;
}
#endif

static void app_ble_gap_handle_disconnected_evt(uint8_t conn_id, T_APP_LE_LINK *p_link,
                                                uint16_t disc_cause)
{
    uint8_t bd_addr[6];

    if (p_link != NULL)
    {
#if F_APP_CHARGER_CASE_SUPPORT
        app_cmd_charger_case_handle_ble_disconn(p_link->id);
#endif

        uint8_t local_disc_cause = p_link->local_disc_cause;
        bool handle_disconnect_when_common_link_free = false;

        for (uint8_t i = 0; i < p_link->disc_cb_list.count; i++)
        {
            T_LE_DISC_CB_ENTRY *p_entry;
            p_entry = os_queue_peek(&p_link->disc_cb_list, i);

            if (p_entry != NULL && p_entry->disc_callback != NULL)
            {
                APP_PRINT_INFO4("app_ble_gap_handle_disconnected_evt: conn_id %d, disc_callback 0x%x, i %d, total_count %d ",
                                conn_id, p_entry->disc_callback, i, p_link->disc_cb_list.count);
                p_entry->disc_callback(conn_id, p_link->local_disc_cause, disc_cause);
            }
        }
#if F_APP_BOND_MGR_BLE_SYNC
        if (disc_cause == (HCI_ERR | HCI_ERR_MIC_FAILURE))
        {
            ble_bond_sync_handle_ble_mic_failure_disconn(p_link);
        }
#endif
        app_transfer_handle_ble_disconnected(p_link, disc_cause);

#if (XM_XIAOAI_FEATURE_SUPPORT)
        if (extend_app_cfg_const.xiaoai_support)
        {
            app_xiaoai_handle_b2s_ble_disconnected(p_link->bd_addr, conn_id, p_link->local_disc_cause,
                                                   disc_cause);
        }
#endif
#if (F_APP_XIAOWEI_FEATURE_SUPPORT)
        if (extend_app_cfg_const.xiaowei_support)
        {
            app_xiaowei_handle_b2s_ble_disconnected(p_link->bd_addr, conn_id, p_link->local_disc_cause,
                                                    disc_cause);
        }
#endif
#if AMA_FEATURE_SUPPORT
        if (extend_app_cfg_const.ama_support)
        {
            app_ama_device_handle_ble_disconnected(p_link->bd_addr, conn_id, p_link->local_disc_cause,
                                                   disc_cause)
        }
#endif
#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
        T_ASP_INFO_LINK_STATE_DATA asp_link_state;
        T_ASP_DEVICE_LINK *p_asp_link = NULL;
        p_asp_link = asp_device_find_link_by_addr(p_link->bd_addr);
        if (p_asp_link && p_asp_link->link_state == ASP_DEVICE_LINK_CONNECTED_ACTIVE)
        {
            asp_link_state.asp_link_state = false;
            memcpy(asp_link_state.bd_addr, p_link->bd_addr, 6);
            asp_device_handle_asp_info(T_ASP_DEVICE_ASP_INFO_STATE, &asp_link_state, sizeof(asp_link_state));
        }
        asp_device_free_link(p_link->bd_addr);
#endif

#if F_APP_TUYA_SUPPORT
        if (extend_app_cfg_const.tuya_support)
        {
            app_tuya_handle_b2s_ble_disconnected(p_link->bd_addr, conn_id, p_link->local_disc_cause,
                                                 disc_cause);
        }
#endif

        if (authen_fail_conn_id == conn_id)
        {
            app_stop_timer(&timer_idx_gap_delay_disconn);
            authen_fail_conn_id = 0xFF;
        }

        if (wait_for_authen_conn_id == conn_id)
        {
            app_stop_timer(&timer_idx_gap_wait_for_authen);
            wait_for_authen_conn_id = 0xFF;
        }

        if (p_link->is_common_link
#if TARGET_LE_AUDIO_GAMING
            || app_ble_gap_check_resume_common_adv(p_link)
#endif
           )
        {
            handle_disconnect_when_common_link_free = true;
        }
        else
        {
            APP_PRINT_ERROR0("app_ble_gap_handle_disconnected_evt: common link not exist");
        }

        memcpy(bd_addr, p_link->bd_addr, 6);

        app_link_free_le_link(p_link);

        app_bt_point_link_num_changed(BP_LINK_TYPE_B2S_LE, BP_LINK_EVENT_DISCONNECT, bd_addr);

        if (handle_disconnect_when_common_link_free)
        {
            app_ble_common_adv_handle_ble_disconnected(conn_id, local_disc_cause, disc_cause);
        }

#if F_APP_LEA_SUPPORT
        if (local_disc_cause != LE_LOCAL_DISC_CAUSE_ROLESWAP)
        {
            app_lea_adv_check_restart_adv();
        }
#endif

#if F_APP_LEA_DONGLE_BINDING
        app_dongle_adv_start(false);
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
        /* handle ble disconnected after link free */
        app_dongle_handle_ble_disconnected(p_link->bd_addr);
#endif

#if F_APP_LEA_SUPPORT
        if (app_link_get_le_link_num() - app_link_get_lea_link_num() == 0)
#else
        if (app_link_get_le_link_num() == 0)
#endif
        {
            app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BLE_LINK_EXIST, app_cfg_const.timer_auto_power_off);

            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_BLE_DISC);
        }

        app_bt_policy_update_cpu_freq(BP_CPU_FREQ_EVENT_LE_LINK_STATE);
        app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_CONN_STATE_CHANGE);

        app_bt_policy_roleswitch_check_by_le_link();
    }
}

static bool app_ble_is_static_rand_addr(uint8_t *p_addr)
{
    return (p_addr[5] & 0xC0) == 0xC0;
}

static bool app_ble_is_rpa_addr(uint8_t *p_addr)
{
    return (p_addr[5] & 0xC0) == 0x40;
}

/**
 * @brief get remote bd address and address type and store them into p_link.
 *
 * @param p_link  @ref T_APP_LE_LINK
 *
 * remote use RPA(resolvable private address) and resolved address type is public address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_PUBLIC;
 * p_link->bd_addr is resloved public address;
 *
 * remote use RPA(resolvable private address) and resolved address type is static random address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_STATIC;
 * p_link->bd_addr is resloved static random address;
 *
 * remote use RPA(resolvable private address) and RPA can not be success resolved:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_UNRESOLVED;
 * p_link->bd_addr is RPA;
 * Reasons why RPA cannot be successfully resolved may be:
 * 1.Pairing failed or pairing was not performed, resulting in stack unable to obtain the remote identity address.
 * 2.Pairing is successful and stack can obtain the remote identity address, but RPA resolution fails.
 *
 * remote use static random address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_STATIC_RANDOM;
 * p_link->bd_addr is static random address;
 *
 * remote use public address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_PUBLIC;
 * p_link->bd_addr is public address;
 */
void app_ble_gap_get_remote_bd_addr_and_type(T_APP_LE_LINK *p_link)
{
    uint8_t remote_bd_addr[6] = {0};
    uint8_t resolved_addr[6] = {0};
    T_GAP_IDENT_ADDR_TYPE resolved_bd_type = (T_GAP_IDENT_ADDR_TYPE)0xFF;
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = (T_GAP_REMOTE_ADDR_TYPE)0xFF;

    le_get_conn_addr(p_link->conn_id, remote_bd_addr, &remote_bd_type);

    if (remote_bd_type == GAP_REMOTE_ADDR_LE_RANDOM)
    {
        if (app_ble_is_rpa_addr(remote_bd_addr))
        {
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
            if (bt_le_resolve_random_address(remote_bd_addr, resolved_addr, &resolved_bd_type) == true)
#else
            if (le_resolve_random_address(remote_bd_addr, resolved_addr, &resolved_bd_type) == true)
#endif
            {
                if (resolved_bd_type == GAP_IDENT_ADDR_PUBLIC)
                {
                    memcpy(p_link->bd_addr, resolved_addr, 6);
                    p_link->bd_type = APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_PUBLIC;
                }
                else if (resolved_bd_type == GAP_IDENT_ADDR_RAND)
                {
                    memcpy(p_link->bd_addr, resolved_addr, 6);
                    p_link->bd_type = APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_STATIC;
                }
            }
            else
            {
                memcpy(p_link->bd_addr, remote_bd_addr, 6);
                p_link->bd_type = APP_LE_REMOTE_BD_TYPE_RPA_AND_UNRESOLVED;
            }
        }
        else if (app_ble_is_static_rand_addr(remote_bd_addr))
        {
            memcpy(p_link->bd_addr, remote_bd_addr, 6);
            p_link->bd_type = APP_LE_REMOTE_BD_TYPE_STATIC_RANDOM;
        }
    }
    else if (remote_bd_type == GAP_REMOTE_ADDR_LE_PUBLIC)
    {
        memcpy(p_link->bd_addr, remote_bd_addr, 6);
        p_link->bd_type = APP_LE_REMOTE_BD_TYPE_PUBLIC;
    }
}

#if F_APP_INCREASE_RTK_APP_LINK_PRIORITY_TEMPORARILY
extern T_GAP_CAUSE le_vendor_set_priority(T_GAP_VENDOR_PRIORITY_PARAM *p_priority_param);

static void app_ble_gap_set_specefic_link_priority(uint8_t conn_id,
                                                   T_GAP_VENDOR_PRIORITY_LEVEL level)
{
    T_GAP_VENDOR_PRIORITY_PARAM param;

    APP_PRINT_TRACE2("app_ble_gap_set_specefic_link_priority: conn_id %d level %d", conn_id, level);

    memset(&param, 0, sizeof(param));

    param.set_priority_mode = GAP_VENDOR_UPDATE_PRIORITY;
    param.link_priority_mode = GAP_VENDOR_SET_SPECIFIC_LINK_PRIORITY;
    param.num_conn_ids = 1;
    param.p_conn_id_list[0].conn_id = conn_id;
    param.p_conn_id_list[0].conn_priority_level = level;

    le_vendor_set_priority(&param);
}

static void app_ble_gap_adjust_link_priority_for_a_while(uint8_t conn_id)
{
    APP_PRINT_TRACE1("app_ble_gap_adjust_link_priority: conn_id %d", conn_id);

    app_ble_gap_set_specefic_link_priority(conn_id, GAP_VENDOR_PRIORITY_LEVEL_4);
    priority_adjust_conn_id = conn_id;

    /* recover to original priority for a while */
    app_start_timer(&timer_idx_recover_link_priority, "recover_link_priority",
                    app_ble_gap_timer_id, APP_TIMER_GAP_RECOVER_LINK_PRIORITY, conn_id, false,
                    2000);
}

static void app_ble_gap_handle_common_link_priority(T_GAP_CONN_STATE new_state, uint8_t conn_id)
{
    T_GAP_CONN_INFO conn_info;

    if (new_state == GAP_CONN_STATE_CONNECTED)
    {
        le_get_conn_info(conn_id, &conn_info);

        if (memcmp(conn_info.remote_bd, app_cfg_nv.dongle_addr, 6))
        {
            app_ble_gap_adjust_link_priority_for_a_while(conn_id);
        }
    }
    else if (new_state == GAP_CONN_STATE_DISCONNECTED)
    {
        if (timer_idx_recover_link_priority)
        {
            if (conn_id == priority_adjust_conn_id)
            {
                app_stop_timer(&timer_idx_recover_link_priority);
            }
        }
    }
}
#endif

static void app_ble_gap_handle_new_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                                  uint16_t disc_cause)
{
    T_APP_LE_LINK *p_link;

    APP_PRINT_TRACE3("app_ble_gap_handle_new_conn_state_evt: conn_id %d, new_state %d, cause 0x%04x",
                     conn_id, new_state, disc_cause);

    p_link = app_link_find_le_link_by_conn_id(conn_id);

#if F_APP_INCREASE_RTK_APP_LINK_PRIORITY_TEMPORARILY
    app_ble_gap_handle_common_link_priority(new_state, conn_id);
#endif

    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTING:
        app_ble_gap_handle_disconnecting_evt(p_link);
        break;

    case GAP_CONN_STATE_DISCONNECTED:
        app_ble_gap_handle_disconnected_evt(conn_id, p_link, disc_cause);
        break;

    case GAP_CONN_STATE_CONNECTING:
        if (p_link == NULL)
        {
            p_link = app_link_alloc_le_link_by_conn_id(conn_id);
            if (p_link != NULL)
            {
                p_link->state = LE_LINK_STATE_CONNECTING;

#if F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY
                memcpy(p_link->le_pulbic_bd_addr, le_public_bd_addr, 6);
#endif
            }
#if BISTO_FEATURE_SUPPORT
            if (extend_app_cfg_const.bisto_support)
            {
                bisto_ble_set_conn_id(bisto_ble_get_instance(), conn_id);
            }
#endif
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        {
            if (p_link != NULL)
            {
                app_ble_gap_get_remote_bd_addr_and_type(p_link);
                p_link->conn_handle = le_get_conn_handle(conn_id);
                app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_CONN_STATE_CHANGE);

                /*update maximum transmission payload size and maximum packet transmission time*/
                le_set_data_len(conn_id, LE_DATA_LEN_TX_OCTETS, LE_DATA_LEN_TX_TIME);

#if AMA_FEATURE_SUPPORT
                if (extend_app_cfg_const.ama_support)
                {
                    app_ama_transport_stream_create(AMA_BLE_STREAM, NULL, conn_id);
                }
#endif

                if (p_link->state == LE_LINK_STATE_CONNECTING)
                {
                    app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_LINK_EXIST);

                    p_link->state = LE_LINK_STATE_CONNECTED;

                    le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &p_link->mtu_size, conn_id);
                    if (app_cfg_const.enable_data_uart)
                    {
                        app_report_event(CMD_PATH_UART, EVENT_LE_CONNECTED, 0, &p_link->id, 1);
                    }

                    app_bt_policy_update_cpu_freq(BP_CPU_FREQ_EVENT_LE_LINK_STATE);
                }

                /*secondary ear ota need modify the random address and start advertising,
                 when ble link connected, the random address shall be set back to le_rws_random_addr*/
                if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    uint8_t rand_addr[6] = {0};
                    app_ble_rand_addr_get(rand_addr);
                    app_ble_common_adv_set_random(rand_addr);
                }
#if F_APP_LEA_SUPPORT
                if (p_link->lea_link_state == LEA_LINK_IDLE && p_link->lea_device)
                {
                    app_lea_uca_link_sm(p_link->conn_handle, LEA_CONNECT, NULL);
                }
                app_lea_pacs_init_available_context(p_link->conn_handle);
#endif

                uint16_t conn_interval_min = RWS_LE_DEFAULT_MIN_CONN_INTERVAL;
                uint16_t conn_interval_max = RWS_LE_DEFAULT_MAX_CONN_INTERVAL;
                uint16_t conn_latency = RWS_LE_DEFAULT_SLAVE_LATENCY;
                uint16_t conn_supervision_timeout = RWS_LE_DEFAULT_SUPERVISION_TIMEOUT;

                ble_set_prefer_conn_param(conn_id, conn_interval_min, conn_interval_max, conn_latency,
                                          conn_supervision_timeout);

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
                if (extend_app_cfg_const.gfps_le_device_support)
                {
                    app_gfps_linkback_info_init(conn_id);
                }
#endif
                app_ota_send_srv_change_indication_info(conn_id);

                app_bt_policy_roleswitch_check_by_le_link();

#if F_APP_CHARGER_CASE_SUPPORT
                uint8_t connect_status = true;

                app_report_status_to_charger_case(CHARGER_CASE_GET_CONNECT_STATUS, &connect_status);
#endif
            };
        }
        break;

    default:
        break;
    }
}

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
T_APP_RESULT app_ble_gap_client_discov_cb(uint16_t conn_handle, T_GATT_CLIENT_EVENT type,
                                          void *p_data)
{
    if (type == GATT_CLIENT_EVENT_DIS_ALL_STATE)
    {
        T_GATT_CLIENT_DIS_ALL_DONE *p_disc = (T_GATT_CLIENT_DIS_ALL_DONE *)p_data;
        APP_PRINT_INFO2("app_ble_gap_client_discov_cb:is_success %d, load_from_ftl %d",
                        p_disc->state, p_disc->load_from_ftl);
    }

    return APP_RESULT_SUCCESS;
}
#endif

void app_ble_gap_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    APP_PRINT_INFO3("app_ble_gap_handle_authen_state_evt:conn_id %d, state %d, cause 0x%x",
                    conn_id, new_state, cause);

    if (new_state == GAP_AUTHEN_STATE_COMPLETE)
    {
        T_APP_LE_LINK *p_link;
        p_link = app_link_find_le_link_by_conn_id(conn_id);

        if (p_link != NULL)
        {
#if BISTO_FEATURE_SUPPORT
            if (extend_app_cfg_const.bisto_support)
            {
                bisto_ble_set_bonded(bisto_ble_get_instance(), true);
                app_bisto_ble_gatt_client_start_discovery(p_link->conn_id);
            }
#endif
            if (app_cfg_const.enable_data_uart)
            {
                uint8_t event_buff[3];
                event_buff[0] = p_link->id;
                event_buff[1] = cause;
                event_buff[2] = cause >> 8;
                app_report_event(CMD_PATH_UART, EVENT_LE_PAIR_STATUS, 0, &event_buff[0], 3);
            }

            if (cause == GAP_SUCCESS)
            {
                p_link->cmd.tx_mask |= TX_ENABLE_AUTHEN_BIT;
                p_link->cmd.dongle_tx_mask |= TX_ENABLE_AUTHEN_BIT;
                p_link->encryption_status = LE_LINK_ENCRYPTIONED;
                app_audio_tone_type_play(TONE_LE_PAIR_COMPLETE, false, false);
                app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_ENCRYPT_STATE_CHANGE);

                app_ble_gap_get_remote_bd_addr_and_type(p_link);

                app_bt_point_link_num_changed(BP_LINK_TYPE_B2S_LE, BP_LINK_EVENT_CONNECT, p_link->bd_addr);

#if F_APP_BLE_BOND_SYNC_SUPPORT
                app_ble_bond_add_send_to_sec(p_link->bd_addr);
#endif

#if AMA_FEATURE_SUPPORT | BISTO_FEATURE_SUPPORT
                ble_stream_copy_pub_addr(conn_id, p_link->bd_addr);
#endif

#if AMA_FEATURE_SUPPORT
                //app_ama_device_create should after copy addr for update ble_stream addr
                app_ama_device_create(COMMON_STREAM_BT_LE, p_link->bd_addr);
#endif

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
                if (extend_app_cfg_const.gfps_le_device_support)
                {
                    app_gfps_device_handle_b2s_ble_bonded(conn_id, p_link->bd_addr);
                }
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
                if (extend_app_cfg_const.xiaoai_support)
                {
                    app_xiaoai_handle_b2s_ble_connected(p_link->bd_addr, conn_id);
                    app_xiaoai_device_set_mtu(T_XM_CMD_COMMUNICATION_WAY_TYPE_BLE, p_link->bd_addr, conn_id,
                                              p_link->mtu_size - 3);
                }
#endif
#if F_APP_XIAOWEI_FEATURE_SUPPORT
                if (extend_app_cfg_const.xiaowei_support)
                {
                    app_xiaowei_handle_b2s_ble_connected(p_link->bd_addr, conn_id);
                    app_xiaowei_device_set_mtu(XIAOWEI_CONNECTION_TYPE_BLE, p_link->bd_addr, conn_id,
                                               p_link->mtu_size - 3);
                }
#endif
#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
                T_ASP_DEVICE_LINK_ALLOC_PARAM param_data;
                param_data.bd_addr = p_link->bd_addr;
                param_data.conn_id = conn_id;
                asp_device_alloc_link(ASP_DEVICE_LINK_BLE, &param_data);

#endif
#if F_APP_FINDMY_FEATURE_SUPPORT
#if F_BT_ANCS_CLIENT_SUPPORT
                ancs_start_discovery(conn_id);
#endif
#endif

#if F_APP_LEA_SUPPORT && F_APP_GAMING_DONGLE_SUPPORT
                app_dongle_check_exit_pairing_state(p_link->lea_link_state, p_link->bd_addr);
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
                app_dongle_handle_ble_connected(p_link->bd_addr);
#endif

                // set default ble connection parameters, LE supervision timeout 5sec
                uint16_t conn_interval_min = RWS_LE_DEFAULT_MIN_CONN_INTERVAL;
                uint16_t conn_interval_max = RWS_LE_DEFAULT_MAX_CONN_INTERVAL;
                uint16_t conn_latency = RWS_LE_DEFAULT_SLAVE_LATENCY;
                uint16_t conn_supervision_timeout = RWS_LE_DEFAULT_SUPERVISION_TIMEOUT;
                ble_set_prefer_conn_param(conn_id, conn_interval_min, conn_interval_max, conn_latency,
                                          conn_supervision_timeout);

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
                p_link->auth_cmpl = true;
                if (gatt_client_start_discovery_all(p_link->conn_handle,
                                                    app_ble_gap_client_discov_cb) == GAP_CAUSE_SUCCESS)
                {
                    p_link->start_discover = true;
                }
#endif

#if F_APP_HAS_SUPPORT
                app_lea_has_handle_authen_evt();
#endif
            }
            else
            {
                if (cause == (SM_ERR | SM_ERR_LINK_KEY_MISSING))
                {
                    //Keep lea link to wait master do SMP again, or let master disconnect link
                    wait_for_authen_conn_id = conn_id;
                    app_start_timer(&timer_idx_gap_wait_for_authen, "gap_wait_for_authen",
                                    app_ble_gap_timer_id, APP_TIMER_GAP_WAIT_FOR_AUTHEN, 0, false,
                                    5000);
                }
                else
                {
                    authen_fail_conn_id = conn_id;
                    app_start_timer(&timer_idx_gap_delay_disconn, "gap_disconnect_delay",
                                    app_ble_gap_timer_id, APP_TIMER_GAP_DISCONNECT_DELAY, 0, false,
                                    100);
                }

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
                p_link->auth_cmpl = false;
#endif
            }
        }
    }
    else if (new_state == GAP_AUTHEN_STATE_STARTED)
    {
        if (wait_for_authen_conn_id == conn_id)
        {
            app_stop_timer(&timer_idx_gap_wait_for_authen);
            wait_for_authen_conn_id = 0xFF;
        }
    }
}

void app_ble_gap_handle_mtu_info(uint8_t conn_id, uint16_t mtu)
{
    T_APP_LE_LINK *p_link;
    p_link = app_link_find_le_link_by_conn_id(conn_id);
    if (p_link != NULL)
    {
        p_link->mtu_size = mtu;

#if AMA_FEATURE_SUPPORT
        app_ama_ble_set_mtu(p_link->conn_id, p_link->mtu_size - 3);
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
        app_xiaoai_device_set_mtu(T_XM_CMD_COMMUNICATION_WAY_TYPE_BLE, p_link->bd_addr, p_link->conn_id,
                                  p_link->mtu_size - 3);
#endif

#if F_APP_XIAOWEI_FEATURE_SUPPORT
        app_xiaowei_device_set_mtu(XIAOWEI_CONNECTION_TYPE_BLE, p_link->bd_addr, p_link->conn_id,
                                   p_link->mtu_size - 3);
#endif

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
        p_link->mtu_received = true;
        if ((p_link->start_discover == false) && p_link->auth_cmpl)
        {
            if (gatt_client_start_discovery_all(p_link->conn_handle,
                                                app_ble_gap_client_discov_cb) == GAP_CAUSE_SUCCESS)
            {
                p_link->start_discover = true;
            }
        }
#endif
    }
}

void app_ble_gap_handle_justwork_bond_cfm(uint8_t conn_id)
{
    T_GAP_CFM_CAUSE cause = GAP_CFM_CAUSE_ACCEPT;
    le_bond_just_work_confirm(conn_id, cause);
    APP_PRINT_INFO1("app_ble_gap_handle_justwork_bond_cfm: cause %d", cause);
}

void app_ble_gap_handle_numerical_comparison_bond_cfm(uint8_t conn_id)
{
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
        {
            app_gfps_handle_ble_user_confirm(conn_id);
        }
        else
        {
#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
            if (extend_app_cfg_const.gfps_le_device_support)
            {
                app_gfps_handle_additional_ble_user_confirm(conn_id);
            }
#endif
        }
    }
    else
#endif
    {
        le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
    }
}

void app_ble_gap_update_sniffing_param(T_GAP_EXT_ADV_STATE adv_state, uint16_t cause)
{
    if (adv_state == EXT_ADV_STATE_ADVERTISING)
    {
        app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_DEV_STATE_CHANGE);
    }
    else if ((adv_state == EXT_ADV_STATE_IDLE) && (cause != 0))
    {
        app_bt_sniffing_param_update(APP_BT_SNIFFING_EVENT_BLE_DEV_STATE_CHANGE);
    }
}

void app_ble_gap_handle_gap_msg(T_IO_MSG *p_io_msg)
{
    T_LE_GAP_MSG stack_msg;
    memcpy(&stack_msg, &p_io_msg->u.param, sizeof(p_io_msg->u.param));
    APP_PRINT_TRACE1("app_ble_gap_handle_gap_msg: subtype %d", p_io_msg->subtype);

#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT
    ble_audio_handle_gap_msg(p_io_msg->subtype, stack_msg);
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
    fmna_ble_peripheral_evt(p_io_msg);
#endif
    ble_mgr_handle_gap_msg(p_io_msg->subtype, &stack_msg);

    switch (p_io_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            app_ble_gap_handle_dev_state_change_evt(stack_msg.msg_data.gap_dev_state_change.new_state,
                                                    stack_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_ble_gap_handle_new_conn_state_evt(stack_msg.msg_data.gap_conn_state_change.conn_id,
                                                  (T_GAP_CONN_STATE)stack_msg.msg_data.gap_conn_state_change.new_state,
                                                  stack_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            app_ble_gap_handle_mtu_info(stack_msg.msg_data.gap_conn_mtu_info.conn_id,
                                        stack_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            app_ble_gap_handle_authen_state_evt(stack_msg.msg_data.gap_authen_state.conn_id,
                                                stack_msg.msg_data.gap_authen_state.new_state,
                                                stack_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            app_ble_gap_handle_justwork_bond_cfm(stack_msg.msg_data.gap_bond_just_work_conf.conn_id);
        }
        break;

    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {
            app_ble_gap_handle_numerical_comparison_bond_cfm(stack_msg.msg_data.gap_bond_user_conf.conn_id);
        }
        break;

    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE:
        {
            app_ble_gap_update_sniffing_param((T_GAP_EXT_ADV_STATE)
                                              stack_msg.msg_data.gap_ext_adv_state_change.new_state,
                                              stack_msg.msg_data.gap_ext_adv_state_change.cause);
        }
        break;

#if F_APP_SLAVE_LATENCY_UPDATE_SUPPORT
    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            if (stack_msg.msg_data.gap_conn_param_update.status == GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS)
            {
                app_cmd_check_slave_latency_update(stack_msg.msg_data.gap_conn_param_update.conn_id, false);
            }
        }
        break;
#endif
    default:
        break;
    }
}

static bool app_ble_gap_conn_ind_get_bd_addr(uint8_t *bd_addr, uint8_t *remote_bd_addr,
                                             T_GAP_REMOTE_ADDR_TYPE remote_bd_type)
{
    bool ret = true;
    uint8_t resolved_addr[6] = {0};
    T_GAP_IDENT_ADDR_TYPE resolved_bd_type = (T_GAP_IDENT_ADDR_TYPE)0xFF;

    if (remote_bd_type == GAP_REMOTE_ADDR_LE_RANDOM)
    {
        if (app_ble_is_rpa_addr(remote_bd_addr))
        {
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
            if (bt_le_resolve_random_address(remote_bd_addr, resolved_addr, &resolved_bd_type))
#else
            if (le_resolve_random_address(remote_bd_addr, resolved_addr, &resolved_bd_type))
#endif
            {
                if (resolved_bd_type == GAP_IDENT_ADDR_PUBLIC)
                {
                    memcpy(bd_addr, resolved_addr, 6);
                }
                else if (resolved_bd_type == GAP_IDENT_ADDR_RAND)
                {
                    memcpy(bd_addr, resolved_addr, 6);
                }
            }
            else
            {
                ret = false;
            }
        }
        else if (app_ble_is_static_rand_addr(remote_bd_addr))
        {
            memcpy(bd_addr, remote_bd_addr, 6);
        }
        else
        {
            ret = false;
        }
    }
    else if (remote_bd_type == GAP_REMOTE_ADDR_LE_PUBLIC)
    {
        memcpy(bd_addr, remote_bd_addr, 6);
    }

    APP_PRINT_TRACE4("app_ble_gap_conn_ind_get_bd_addr ret %d, type: 0x%02X, input_addr %s, output_addr %s",
                     ret, remote_bd_type, TRACE_BDADDR(remote_bd_addr), TRACE_BDADDR(bd_addr));

    return ret;
}

static T_APP_RESULT app_ble_gap_cb(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA cb_data;

    memcpy(&cb_data, p_cb_data, sizeof(T_LE_CB_DATA));

#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT
    ble_audio_handle_gap_cb(cb_type, p_cb_data);
#endif

    ble_mgr_handle_gap_cb(cb_type, p_cb_data);

    switch (cb_type)
    {
#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
    case GAP_MSG_APP_BOND_MANAGER_INFO:
        {
            result = bt_bond_mgr_handle_gap_msg(cb_data.p_le_cb_data);
        }
        break;
#endif
    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE_INFO:
        {
            app_ble_gap_update_sniffing_param((T_GAP_EXT_ADV_STATE)
                                              cb_data.p_le_ext_adv_state_change_info->state,
                                              cb_data.p_le_ext_adv_state_change_info->cause);
        }
        break;
    case GAP_MSG_LE_EXT_ADV_REPORT_INFO:
#if F_APP_ONE_WIRE_UART_SUPPORT
        if (app_cfg_nv.one_wire_start_force_engage)
        {
            if (!memcmp(cb_data.p_le_ext_adv_report_info->bd_addr, app_cfg_nv.bud_peer_factory_addr, 6))
            {
                APP_PRINT_TRACE1("GAP_MSG_LE_EXT_ADV_REPORT_INFO: scan peer factory addr %s",
                                 TRACE_BDADDR(app_cfg_nv.bud_peer_factory_addr));
            }
            else
            {
                break;
            }
        }
#endif
        break;

    case GAP_MSG_LE_CONN_UPDATE_IND:
        result = APP_RESULT_ACCEPT;
        break;

    case GAP_MSG_LE_EXT_ADV_START_SETTING:
        break;

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        {
            APP_PRINT_INFO2("app_ble_gap_cb: GAP_MSG_LE_MODIFY_WHITE_LIST operation 0x%x, cause 0x%x",
                            cb_data.p_le_modify_white_list_rsp->operation,
                            cb_data.p_le_modify_white_list_rsp->cause);

            app_ble_whitelist_set_operation_state(false);
            app_ble_whitelist_handle_pending_operation();
        }
        break;

#if !CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
    case GAP_MSG_LE_BOND_MODIFY_INFO:
        {
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_MODIFY_INFO: type 0x%x",
                            cb_data.p_le_bond_modify_info->type);
#if GATTC_TBL_STORAGE_SUPPORT
            gattc_tbl_storage_handle_bond_modify(cb_data.p_le_bond_modify_info);
#endif
        }
        break;
#endif
#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT || F_APP_TMAP_BMR_SUPPORT
    case GAP_MSG_LE_EXT_SCAN_STATE_CHANGE_INFO:
        {
            if (cb_data.p_le_ext_scan_state_change_info->state == GAP_SCAN_STATE_IDLE)
            {
                APP_PRINT_INFO0("GAP ext scan stop");
            }
            else if (cb_data.p_le_ext_scan_state_change_info->state == GAP_SCAN_STATE_SCANNING)
            {
                APP_PRINT_INFO0("GAP ext scan start");
            }
        }
        break;
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
    case GAP_MSG_LE_ADV_READ_TX_POWER:
        APP_PRINT_INFO2("GAP_MSG_LE_ADV_READ_TX_POWER: cause 0x%x, tx_power_level 0x%x",
                        cb_data.p_le_adv_read_tx_power_rsp->cause,
                        cb_data.p_le_adv_read_tx_power_rsp->tx_power_level);
        tps_set_parameter(TPS_PARAM_TX_POWER, 1, &(cb_data.p_le_adv_read_tx_power_rsp->tx_power_level));
        break;

    case GAP_MSG_LE_BOND_KEY_REQ:
        {
            result = APP_RESULT_REJECT;

            T_LE_KEY_ENTRY *key_entry;
            T_LE_DEV_INFO dev_info;
            T_LE_LTK *local_ltk;

            memset(&dev_info, 0, sizeof(T_LE_DEV_INFO));
            key_entry = le_get_high_priority_bond();
            if ((key_entry != NULL) && le_get_dev_info(key_entry, &dev_info))
            {
                local_ltk = (T_LE_LTK *)dev_info.local_ltk;
                cb_data.p_le_bond_key_req->key_len = local_ltk->link_key_length;
                memcpy(cb_data.p_le_bond_key_req->link_key, local_ltk->key, local_ltk->link_key_length);
                result = APP_RESULT_ACCEPT;
            }

            APP_PRINT_INFO4("GAP_MSG_LE_BOND_KEY_REQ: key_length 0x%x, key_type 0x%x, link_key %b, result 0x%x",
                            cb_data.p_le_bond_key_req->key_len,
                            cb_data.p_le_bond_key_req->key_type,
                            TRACE_BINARY(cb_data.p_le_bond_key_req->key_len, cb_data.p_le_bond_key_req->link_key),
                            result
                           );
        }
        break;
#endif


    case GAP_MSG_LE_CREATE_CONN_IND:
        {
            result = APP_RESULT_ACCEPT;

#if F_APP_LEA_SUPPORT
            uint8_t remote_bd_addr[6] = {0};
            uint8_t resolved_addr[6] = {0};
            uint8_t resolved_bd_type = 0xFF;

            memcpy(remote_bd_addr, cb_data.p_le_create_conn_ind->bd_addr, 6);

#if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
            if (bt_le_resolve_random_address(remote_bd_addr, resolved_addr, &resolved_bd_type) == true)
#else
            if (le_resolve_random_address(remote_bd_addr, resolved_addr, &resolved_bd_type) == true)
#endif
            {
                for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
                {
                    if (app_db.le_link[i].used == true &&
                        !memcmp(app_db.le_link[i].bd_addr, resolved_addr, 6))
                    {
                        return APP_RESULT_REJECT;
                    }
                }
            }
#endif

#if F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY && TARGET_LEGACY_AUDIO_GAMING
#if F_APP_LEA_SUPPORT
            T_APP_LE_LINK *p_le_phone_link = app_dongle_get_le_audio_phone_link();
            T_APP_BR_LINK *p_br_phone_link =  app_dongle_get_connected_phone_link();
            result = APP_RESULT_ACCEPT;

            if (app_ble_gap_conn_ind_get_bd_addr(le_public_bd_addr,
                                                 cb_data.p_le_create_conn_ind->bd_addr,
                                                 cb_data.p_le_create_conn_ind->remote_addr_type) == false)
            {
                APP_PRINT_ERROR0("GAP_MSG_LE_CREATE_CONN_IND: Fail to reslove le public address");
                break;
            }

            if ((p_le_phone_link != NULL) && (memcmp(le_public_bd_addr, p_le_phone_link->le_pulbic_bd_addr, 6)))
            {
                APP_PRINT_TRACE0("GAP_MSG_LE_CREATE_CONN_IND: One lea phone already connected");
                result = APP_RESULT_REJECT;
                break;
            }

            if ((p_br_phone_link != NULL) && (memcmp(le_public_bd_addr, p_br_phone_link->bd_addr, 6))
#if F_APP_DONGLE_MULTI_PAIRING
                && (app_db.cmd_dsp_config_enable_multilink == 0)
#endif
               )
            {
                APP_PRINT_TRACE0("GAP_MSG_LE_CREATE_CONN_IND: One legacy phone already connected");
                result = APP_RESULT_REJECT;
                break;
            }
#endif
#endif

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
            if (extend_app_cfg_const.gfps_le_device_support)
            {
                if (app_gfps_ble_conn_info_handle(cb_data.p_le_create_conn_ind->bd_addr,
                                                  cb_data.p_le_create_conn_ind->remote_addr_type) == true)
                {
                    result = APP_RESULT_ACCEPT;
                }
                else
                {
                    result = APP_RESULT_REJECT;
                }
            }
#endif
        }
        break;

#if F_APP_GAMING_LE_FIX_CHANNEL_SUPPORT
    case GAP_MSG_LE_FIXED_CHANN_DATA_IND:
        {
            T_LE_FIXED_CHANN_DATA_IND *data_ind = cb_data.p_le_fixed_chann_data_ind;
            T_APP_LE_LINK *p_dongle_link = app_link_find_le_link_by_conn_id(data_ind->conn_id);

            if (p_dongle_link != NULL)
            {
                p_dongle_link->remote_device_type = DEVICE_TYPE_DONGLE;
            }

            app_gaming_sync_disassemble_data(NULL, data_ind->p_data, data_ind->value_len);
        }
        break;

#endif

    default:
        break;
    }

    return result;
}

bool app_ble_gap_disconnect(T_APP_LE_LINK *p_link, T_LE_LOCAL_DISC_CAUSE disc_cause)
{
    if (p_link != NULL)
    {
        APP_PRINT_TRACE2("app_ble_gap_disconnect: conn_id %d, disc_cause %d",
                         p_link->conn_id, disc_cause);
        if (le_disconnect(p_link->conn_id) == GAP_CAUSE_SUCCESS)
        {
            p_link->local_disc_cause = disc_cause;
            return true;
        }
    }
    return false;
}

void app_ble_gap_disconnect_all(T_LE_LOCAL_DISC_CAUSE disc_cause)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t        i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        p_link = app_link_find_le_link_by_conn_id(i);
        if (p_link != NULL && p_link->used)
        {

            if ((p_link->state == LE_LINK_STATE_CONNECTING
                 || p_link->state == LE_LINK_STATE_CONNECTED)
#if F_APP_LEA_SUPPORT
                && p_link->lea_link_state == LEA_LINK_IDLE)
#else
               )
#endif
            {
                app_ble_gap_disconnect(p_link, disc_cause);
            }
        }
    }
}

static void app_ble_gap_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_READY:
        {
            if (app_cfg_const.rtk_app_adv_support || app_cfg_const.tts_support)
            {
                /* init here to avoid app_cfg_nv.bud_local_addr no mac info (due to factory reset) */
                app_ble_common_adv_init();
            }
        }
        break;

    default:
        break;
    }
}

static uint32_t app_ble_gap_get_ext_adv_num(void)
{
    uint8_t adv_handle_number = 0;

#if F_APP_DURIAN_SUPPORT
    {
        adv_handle_number++;//for durian adv
    }
#endif

#if F_APP_ERWS_SUPPORT
    {
        adv_handle_number++;//for engage adv
    }
#endif

    if (extend_app_cfg_const.ama_support || extend_app_cfg_const.xiaowei_support ||
        extend_app_cfg_const.bisto_support ||
        (extend_app_cfg_const.xiaoai_support &&
         !app_cfg_const.enable_rtk_charging_box))
    {
        adv_handle_number++;//for public address adv,those adv has a single adv handle.
    }

    if (app_cfg_const.swift_pair_support)
    {
        adv_handle_number++;//for swift pair adv
    }

    if (app_cfg_const.rtk_app_adv_support)
    {
        adv_handle_number++;//for common adv
    }

    if (app_cfg_nv.ota_tooling_start)
    {
        adv_handle_number++;//for ota tooling adv
    }

    if (app_cfg_const.enable_rtk_fast_pair_adv)
    {
        adv_handle_number++;//for rtk fast pair adv
    }

    if (app_cfg_const.enable_rtk_charging_box)
    {
        adv_handle_number++;//for xiaomi chargerbox adv
    }
#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
    if (extend_app_cfg_const.gfps_support)
    {
        adv_handle_number++;//for gfps adv
#if CONFIG_REALTEK_GFPS_FINDER_SUPPORT
        if (extend_app_cfg_const.gfps_finder_support)
        {
            adv_handle_number++;//for gfps finder beacon adv
        }
#endif
    }
#endif
#if F_APP_TUYA_SUPPORT
    if (extend_app_cfg_const.tuya_support)
    {
        adv_handle_number++;//for tuya adv
    }
#endif
#if F_APP_BLE_SWIFT_PAIR_SUPPORT
    adv_handle_number++;
#endif

#if F_APP_TEAMS_BT_POLICY
    adv_handle_number += MAX_BLE_LINK_NUM;
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
    if (app_cfg_const.enable_dongle_dual_mode)
    {
        adv_handle_number++;
    }
#endif

#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT
    adv_handle_number++;
#endif
#if F_APP_CSIS_SUPPORT
    adv_handle_number++;
#endif
#if F_APP_TMAP_BMR_SUPPORT
    adv_handle_number++;
#endif

#if F_APP_FINDMY_FEATURE_SUPPORT
    adv_handle_number++;//for findmy adv
#endif

    return adv_handle_number;
}


void app_ble_gap_ble_mgr_init(void)
{
    BLE_MGR_PARAMS param = {0};

    param.ble_ext_adv.enable = true;
    param.ble_ext_adv.adv_num = app_ble_gap_get_ext_adv_num();

    param.ble_conn.enable = true;
    uint8_t supported_max_le_link_num = le_get_max_link_num();
    param.ble_conn.link_num = ((MAX_BLE_LINK_NUM <= supported_max_le_link_num) ? MAX_BLE_LINK_NUM :
                               supported_max_le_link_num);

    if (extend_app_cfg_const.ama_support || extend_app_cfg_const.xiaowei_support ||
        extend_app_cfg_const.bisto_support || extend_app_cfg_const.xiaoai_support)
    {
        param.ble_adv_data.enable = true;
        param.ble_adv_data.update_scan_data = true;
        param.ble_adv_data.adv_interval = (extend_app_cfg_const.multi_adv_interval * 8) / 5;
        app_ble_gap_gen_scan_rsp_data(&scan_rsp_data_len, scan_rsp_data);
        param.ble_adv_data.scan_rsp_len = scan_rsp_data_len;
        param.ble_adv_data.scan_rsp_data = scan_rsp_data;
    }

    param.ble_scan.enable = true;

    ble_mgr_init(&param);
}

void app_ble_gap_vendor_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_VENDOR_CB_DATA cb_data;

    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_VENDOR_CB_DATA));
    APP_PRINT_INFO2("app_ble_gap_vendor_callback:cb_type %d, cause 0x%x", cb_type,
                    cb_data.gap_vendor_cause.cause);
    switch (cb_type)
    {
    case GAP_MSG_EXT_ADV_HANDLE_PRIORITY_SET:
        break;
    default:
        break;
    }

    return;
}

static void app_ble_gap_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_ble_gap_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_GAP_DISCONNECT_DELAY:
        {
            app_stop_timer(&timer_idx_gap_delay_disconn);
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(authen_fail_conn_id);

            if (p_link != NULL)
            {
                authen_fail_conn_id = 0xFF;
                p_link->cmd.tx_mask &= ~TX_ENABLE_AUTHEN_BIT;
                p_link->cmd.dongle_tx_mask &= ~TX_ENABLE_AUTHEN_BIT;
                p_link->encryption_status = LE_LINK_UNENCRYPTIONED;
                app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED);
            }
        }
        break;

    case APP_TIMER_GAP_WAIT_FOR_AUTHEN:
        {
            app_stop_timer(&timer_idx_gap_wait_for_authen);
            T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_id(wait_for_authen_conn_id);

            if (p_link != NULL)
            {
                wait_for_authen_conn_id = 0xFF;
                p_link->cmd.tx_mask &= ~TX_ENABLE_AUTHEN_BIT;
                p_link->cmd.dongle_tx_mask &= ~TX_ENABLE_AUTHEN_BIT;
                p_link->encryption_status = LE_LINK_UNENCRYPTIONED;
                app_ble_gap_disconnect(p_link, LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED);
            }
        }
        break;

#if F_APP_INCREASE_RTK_APP_LINK_PRIORITY_TEMPORARILY
    case APP_TIMER_GAP_RECOVER_LINK_PRIORITY:
        {
            uint8_t conn_id = (uint8_t)param;

            app_stop_timer(&timer_idx_recover_link_priority);

            app_ble_gap_set_specefic_link_priority(conn_id, GAP_VENDOR_PRIORITY_LEVEL_1);
        }
        break;
#endif

    default:
        break;
    }
}

static void app_ble_gap_common_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_CB_DATA));
    APP_PRINT_INFO1("app_ble_gap_common_callback: cb_type = 0x%x", cb_type);

    ble_mgr_handle_gap_common_cb(cb_type, p_cb_data);

#if F_APP_GATT_CACHING_SUPPORT
    app_ble_gatt_caching_common_callback(cb_type, p_cb_data);
#endif

#if F_APP_B2B_ENGAGE_REDUCE_NSE
    app_vendor_ble_gap_handle(cb_type, p_cb_data);
#endif

    switch (cb_type)
    {
    case GAP_MSG_VENDOR_CMD_RSP:
        {
            APP_PRINT_INFO3("app_ble_gap_common_callback: GAP_MSG_VENDOR_CMD_RSP command 0x%x, cause 0x%x, param_len %d",
                            cb_data.p_gap_vendor_cmd_rsp->command,
                            cb_data.p_gap_vendor_cmd_rsp->cause,
                            cb_data.p_gap_vendor_cmd_rsp->param_len);

            if ((cb_data.p_gap_vendor_cmd_rsp->command == MP_CMD_HCI_OPCODE) &&
                (cb_data.p_gap_vendor_cmd_rsp->is_cmpl_evt == true))
            {
                app_cmd_handle_mp_cmd_hci_evt(p_cb_data);
            }
        }
        break;

    case GAP_MSG_VENDOR_EVT_INFO:
        {
            APP_PRINT_INFO1("app_ble_gap_common_callback: GAP_MSG_VENDOR_EVT_INFO param_len %d",
                            cb_data.p_gap_vendor_evt_info->param_len);
        }
        break;

    default:
        break;
    }

    return;
}

void app_ble_gap_init(void)
{
    bt_mgr_cback_register(app_ble_gap_bt_cback);
    gap_register_app_cb(app_ble_gap_common_callback);

#if F_APP_GATT_CACHING_SUPPORT
    app_ble_gatt_caching_init();
#endif

    le_register_app_cb(app_ble_gap_cb);
    app_ble_rand_addr_init();
    app_ble_gap_ble_mgr_init();

    app_ble_whitelist_init();
    gap_vendor_register_cb(app_ble_gap_vendor_callback);
#if F_APP_SC_KEY_DERIVE_SUPPORT
    app_ble_key_derive_init();
#endif

#if GATTC_TBL_STORAGE_SUPPORT
    gattc_tbl_storage_init();
#endif

#if F_APP_EATT_SUPPORT
    app_ble_eatt_ecfc_init();
#endif

#if F_APP_RWS_BLE_USE_RPA_SUPPORT
#if F_APP_ERWS_SUPPORT
    app_ble_device_irk_sync_relay_init();
#endif
#endif

    app_timer_reg_cb(app_ble_gap_timeout_cb, &app_ble_gap_timer_id);
}

void app_ble_gap_param_init(void)
{
    /*set device name and device appearance*/
    uint16_t appearance;
    uint8_t handle_conn_ind = false;

    if (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SINGLE)
    {
        appearance = GAP_APPEARANCE_CATEGORY_WEARABLE_AUDIO_DEVICE << 6 |
                     GAP_APPEARANCE_SUB_CATEGORY_EARBUD;
    }
    else
    {
        appearance = GAP_APPEARANCE_CATEGORY_WEARABLE_AUDIO_DEVICE << 6 |
                     GAP_APPEARANCE_SUB_CATEGORY_HEADSET;
    }

    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, app_cfg_nv.device_name_le);

    /*slave init mtu request*/
    uint8_t slave_init_gatt_mtu_req = false;
    le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_gatt_mtu_req),
                     &slave_init_gatt_mtu_req);

#if CONFIG_REALTEK_GFPS_LE_DEVICE_SUPPORT
    if (extend_app_cfg_const.gfps_le_device_support)
    {
        handle_conn_ind = true;
    }
#endif

#if F_APP_LEA_SUPPORT
    handle_conn_ind = true;
#endif

#if F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY
    handle_conn_ind = true;
#endif

    if (handle_conn_ind == true)
    {
        le_set_gap_param(GAP_PARAM_HANDLE_CREATE_CONN_IND, sizeof(handle_conn_ind), &handle_conn_ind);
    }


#if BISTO_FEATURE_SUPPORT
    if (extend_app_cfg_const.bisto_support)
    {
        app_bisto_ble_set_name();
    }
#endif

    /*set GAP Bond Manager parameters*/
    uint32_t passkey             = 0;
    uint8_t use_fixed_passkey    = false;
    uint16_t sec_req_requirement = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;

    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(uint32_t), &passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(uint8_t), &use_fixed_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &sec_req_requirement);

    /*Initialize ble link*/
    uint8_t supported_max_le_link_num = le_get_max_link_num();
    uint8_t link_num = ((MAX_BLE_LINK_NUM <= supported_max_le_link_num) ? MAX_BLE_LINK_NUM :
                        supported_max_le_link_num);
    le_gap_init(link_num);

    /*set GAP_PARAM_CIS_HOST_SUPPORT */
#if F_APP_TMAP_CT_SUPPORT || F_APP_TMAP_UMR_SUPPORT
    bool cis_flag = true;
    le_set_gap_param(GAP_PARAM_CIS_HOST_SUPPORT, sizeof(cis_flag), &cis_flag);
#endif

    /* Initialize scan parameters */
    T_GAP_SCAN_MODE  scan_mode              = GAP_SCAN_MODE_PASSIVE;
    uint8_t scan_filter_policy              = GAP_SCAN_FILTER_ANY;
    uint8_t scan_filter_duplicate           = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
    T_GAP_LOCAL_ADDR_TYPE  own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;

    T_GAP_LE_EXT_SCAN_PARAM extended_scan_param[GAP_EXT_SCAN_MAX_PHYS_NUM];
    extended_scan_param[0].scan_type     = scan_mode;
    extended_scan_param[0].scan_interval = 0x10;
    extended_scan_param[0].scan_window   = 0x10;

    uint8_t  scan_phys         = GAP_EXT_SCAN_PHYS_1M_BIT;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period   = 0;

    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_LOCAL_ADDR_TYPE, sizeof(own_address_type),
                          &own_address_type);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                          &scan_filter_policy);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                          &scan_filter_duplicate);
    le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_1M, &extended_scan_param[0]);
}
