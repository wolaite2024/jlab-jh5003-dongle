#if F_APP_GAMING_DONGLE_SUPPORT
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "os_sched.h"
#include "gap_vendor.h"
#include "sysm.h"
#include "bt_bond.h"
#include "btm.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "app_dongle_data_ctrl.h"
#include "app_dongle_dual_mode.h"
#include "trace.h"
#include "ble_ext_adv.h"
#include "app_bt_policy_api.h"
#include "app_device.h"
#include "app_bond.h"
#include "app_mmi.h"
#include "app_auto_power_off.h"
#include "app_link_util.h"
#include "app_main.h"
#include "app_audio_policy.h"
#include "app_dongle_source_ctrl.h"
#include "app_dongle_common.h"
#include "app_relay.h"
#include "app_a2dp.h"
#include "app_link_util.h"
#include "app_timer.h"
#include "app_bt_policy_int.h"
#include "app_bt_policy_api.h"
#include "audio.h"
#include "app_ble_common_adv.h"
#include "app_adv_stop_cause.h"
#include "app_dongle_spp.h"
#include "app_dongle_record.h"
#include "app_lea_adv.h"
#include "bt_hfp.h"
#include "bt_a2dp.h"
#include "app_sniff_mode.h"
#include "app_vendor.h"
#include "usb_host_detect.h"

#if F_APP_GAMING_CONTROLLER_SUPPORT
#include "app_dongle_controller.h"
#if F_APP_BLE_HID_CONTROLLER_SUPPORT
#include "app_ble_hid_controller.h"
#endif
#endif


#if F_APP_SLIDE_SWITCH_SUPPORT
#include "app_slide_switch.h"
#endif

#if F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
#include "app_dongle_data_ctrl.h"
#endif

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
#include "multitopology_ctrl.h"
#include "app_bt_sniffing.h"
#endif

#if F_APP_LEA_SUPPORT
#include "app_lea_ccp.h"
#include "transmit_svc_dongle.h"
#include "app_lea_adv.h"
#include "app_lea_unicast_audio.h"
#endif

#if F_APP_BLE_SWIFT_PAIR_SUPPORT
#include "app_ble_swift_pair.h"
#endif

#if F_APP_GAMING_WIRED_MODE_HANDLE
#include "ble_ext_adv.h"
#include "app_adv_stop_cause.h"
#endif

#if F_APP_BLE_HID_CONTROLLER_SUPPORT
#include "app_ble_gap.h"
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_gfps.h"
#include "app_gfps_device.h"
#endif

#if F_APP_MUTLILINK_SOURCE_PRIORITY_UI
#include "app_multilink_customer.h"
#endif

#define CALCULATE_A2DP_PKT_LOSS_RATE        0

#define DONGLE_DISCONNECT_RSSI_THRESHOLD    -65
#define SWITCH_RF_MODE_DEBOUNCE_TIME        5000
#define GAMING_CONTROLLER_LATENCY_PLUS      20

#define FIX_CHANNEL_DISALLOW_SNIFF_TIME     5000
#define FIX_CHANNEL_SNIFF_DETECT_TIME       (FIX_CHANNEL_DISALLOW_SNIFF_TIME - 1000)

typedef enum
{
    APP_TIMER_DISALLOW_DONGLE_A2DP                   = 0x00,
    APP_TIMER_LOW_RSSI_DISCONNECT_DONGLE             = 0x01,
    APP_TIMER_CHECK_CHANGE_PKT_TYPE                  = 0x02,
    APP_TIMER_CALL_ILDE_DISC_SCO                     = 0x03,
    APP_TIMER_SWITCH_RF_MODE_DEBOUNCE                = 0x04,
    APP_TIMER_SEND_FIX_CHAN_EXIT_SNIFF               = 0x05,
} T_APP_DONGLE_TIMER;

typedef enum
{
    DONGLE_ENABLE_BLE_ADV,
    DONGLE_DISABLE_BLE_ADV,
    DONGLE_LINKBACK_DONGLE,
    DONGLE_LINKBACK_PHONE,
    DONGLE_ENABLE_INQUIRY_SCAN,
    DONGLE_DISABLE_INQUIRY_SCAN,
    DONGLE_DISABLE_LINKBACK,
} T_APP_DUAL_MODE_ACTION;

typedef enum
{
    GAMING_SETTING_NONE,
    GAMING_SETTING_LINKBACK,
    GAMING_SETTING_MULTI_SRC_PLAYING,
    GAMING_SETTING_MULTI_SRC_WITH_BLE_CONNECTED,
    GAMING_SETTING_WITH_BLE_ADV,
} T_DONGLE_GAMING_SETTING;

typedef enum
{
    DONGLE_SYNC_EVENT_A2DP_DISC,
    DONGLE_SYNC_EVENT_ACL_DISC,
    DONGLE_SYNC_EVENT_ACL_CONN,
    DONGLE_SYNC_EVENT_BT_A2DP_PLAY_START,
    DONGLE_SYNC_EVENT_BT_A2DP_PLAY_STOP,
    DONGLE_SYNC_EVNET_DISALLOW_DONGLE_TX,
} T_DONGLE_SYNC_STATUS_EVENT;

typedef enum
{
    PHONE_EVENT_A2DP_START,
    PHONE_EVENT_A2DP_STOP,
    PHONE_EVENT_SCO_START,
    PHONE_EVENT_SCO_STOP,
    PHONE_EVENT_A2DP_DISC,
    PHONE_EVENT_PHONE_DISC,
    PHONE_EVENT_LEA_CALL_START,
    PHONE_EVENT_LEA_CALL_STOP,
} T_PHONE_STREAM_EVENT;

#if F_APP_ALLOW_LEGACY_GAMING_TX_3M
#define RSSI_THRESHOLD_CHANGE_TO_3M     -50
#define CHANGE_TO_3M_DETECT_TIME        2
#define CHANGE_TO_2M_DETECT_TIME        2
#endif

T_DONGLE_CONTROL_DATA dongle_ctrl_data;

static T_APP_DONGLE_RF_MODE next_rf_mode = DONGLE_RF_MODE_24G;
static T_APP_DONGLE_STATE dongle_state = DONGLE_STATE_STANDBY;
static uint8_t app_dongle_timer_id = 0;
static uint8_t timer_idx_call_idle_disc_sco = 0;
static uint8_t timer_idx_send_fix_chan_exit_sniff = 0;

#if F_APP_LEGACY_DONGLE_BINDING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
static uint8_t timer_idx_disallow_dongle_a2dp = 0;
#endif
#if F_APP_LOW_RSSI_DISCONNECT_DONGLE
static uint8_t timer_idx_low_rssi_disconnect_dongle = 0;
#endif
#if F_APP_ALLOW_LEGACY_GAMING_TX_3M
static uint8_t timer_idx_check_change_pkt_type = 0;
#endif
static uint8_t timer_idx_switch_rf_mode_debounce = 0;

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
static bool wait_cis_disc = false;
#endif

#if F_APP_GAMING_WIRED_MODE_HANDLE
static bool disable_rf_due_to_wired_handle = false;
#endif

void app_dongle_clear_dongle_status(void)
{
    memset(&dongle_status, 0, sizeof(dongle_status));

    app_dongle_streaming_handle(dongle_status.streaming_to_peer);
}

void app_dongle_sync_headset_status(void)
{
    // sync if not connected, or connected and role is primary
    bool sync_to_dongle = (app_db.remote_session_state != REMOTE_SESSION_STATE_CONNECTED) ||
                          (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY);

    if (sync_to_dongle)
    {
        app_dongle_send_cmd(DONGLE_CMD_SYNC_STATUS, (uint8_t *)&headset_status, sizeof(headset_status));
    }
}

static void linkback_handle_after_mode_switch(T_DONGLE_LINKBACK_STATUS status)
{
    if (status.trigger_bt_pairing)
    {
        app_bt_policy_enter_pairing_mode(true, true);
    }
    else
    {
        if (status.linkback_phone)
        {
            app_bt_policy_conn_all_b2s();
        }
    }

    if (status.linkback_dongle)
    {
        app_dongle_adv_start(false);
    }
}

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
/* update headset conn status exclude dongle link */
void app_dongle_update_headset_conn_status(T_HEADSET_CONN_EVENT event)
{
    uint32_t bond_flag = 0;
    bool legacy_device_connected = false;
    bool ble_device_connected = false;
    bool pairing_adv = (dongle_state == DONGLE_STATE_PAIRING) ? true : false;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_link_check_b2s_link_by_id(i))
        {
            bt_bond_flag_get(app_db.br_link[i].bd_addr, &bond_flag);

            if ((bond_flag & BOND_FLAG_DONGLE) == 0)
            {
                legacy_device_connected = true;
                break;
            }
        }
    }

    if (legacy_device_connected || ble_device_connected)
    {
        headset_status.conn_status = HEADSET_PHONE_CONNECTED;
    }
    else
    {
        headset_status.conn_status = HEADSET_PHONE_DISCONNECTED;
    }

    app_relay_async_single(APP_MODULE_TYPE_DONGLE_DUAL_MODE, APP_REMOTE_MSG_SYNC_HEADSET_STATUS);

    app_dongle_adv_start(pairing_adv);

    APP_PRINT_TRACE3("app_dongle_update_phone_acl_status: event %d legacy_connected %d ble_connected %d",
                     event, legacy_device_connected, ble_device_connected);

    app_dongle_sync_headset_status();
}
#endif

static void app_dongle_update_phone_stream_status(T_PHONE_STREAM_EVENT event)
{
    T_PHONE_STREAM_STATUS pre_status = headset_status.phone_status;
    T_PHONE_STREAM_STATUS new_status = headset_status.phone_status;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    if (event == PHONE_EVENT_A2DP_START)
    {
        new_status = PHONE_STREAM_A2DP;
    }
    else if (event == PHONE_EVENT_A2DP_STOP || event == PHONE_EVENT_A2DP_DISC)
    {
        if (pre_status == PHONE_STREAM_A2DP)
        {
            new_status = PHONE_STREAM_IDLE;
        }
    }
    else if (event == PHONE_EVENT_SCO_START)
    {
        new_status = PHONE_STREAM_HFP;
    }
    else if (event == PHONE_EVENT_SCO_STOP)
    {
        if (pre_status == PHONE_STREAM_HFP)
        {
            new_status = PHONE_STREAM_IDLE;
        }
    }
    else if (event == PHONE_EVENT_PHONE_DISC)
    {
        new_status = PHONE_STREAM_IDLE;
    }
#if F_APP_LEA_SUPPORT
    else if (event == PHONE_EVENT_LEA_CALL_START)
    {
        new_status = PHONE_STREAM_LEA_CALL;
    }
    else if (event == PHONE_EVENT_LEA_CALL_STOP)
    {
        if (pre_status == PHONE_STREAM_LEA_CALL)
        {
            new_status = PHONE_STREAM_IDLE;
        }
    }
#endif

    if (new_status == PHONE_STREAM_A2DP || new_status == PHONE_STREAM_HFP)
    {
        app_dongle_handle_heaset_adv_interval(ADV_INTERVAL_EVENT_BT_STREAMING_START);
    }
    else
    {
        app_dongle_handle_heaset_adv_interval(ADV_INTERVAL_EVENT_BT_STREAMING_STOP);
    }

    if (new_status != pre_status)
    {
        APP_PRINT_TRACE2("app_dongle_update_phone_stream_status: event %d status %d", event, new_status);

        headset_status.phone_status = new_status;

        app_dongle_sync_headset_status();

        if (app_db.device_state == APP_DEVICE_STATE_ON)
        {
            app_relay_async_single(APP_MODULE_TYPE_DONGLE_DUAL_MODE, APP_REMOTE_MSG_SYNC_HEADSET_STATUS);
        }
    }
}

static void app_dongle_cancel_linkback(void)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));
    bt_param.not_check_addr_flag = true;

    app_bt_policy_state_machine(EVENT_STOP_LINKBACK, &bt_param);
}

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
T_BT_EVENT_PARAM_A2DP_STREAM_START_IND a2dp_stream_para;
T_BT_EVENT_PARAM_SCO_CONN_CMPL sco_conn_cmpl_param;
#endif

#if TARGET_LEGACY_AUDIO_GAMING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
void app_dongle_adjust_gaming_latency(void)
{
    uint8_t active_idx = app_a2dp_get_active_idx();
    uint8_t *active_addr = app_db.br_link[active_idx].bd_addr;
    T_APP_BR_LINK *p_dongle_link = app_dongle_get_connected_dongle_link();

    if (p_dongle_link && p_dongle_link->a2dp_track_handle)
    {
        uint16_t latency_value = ULTRA_LOW_LATENCY_MS;
        uint8_t latency_plus = 0;

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
        latency_plus = app_vendor_get_htpoll_latency_plus();
#endif

        latency_value += latency_plus;

        if (dongle_status.usb_host_type == OS_TYPE_PS ||
            app_db.dongle_is_enable_mic)
        {
            latency_value += 5;
        }

        if (headset_status.increase_a2dp_interval)
        {
            latency_value += 10;
        }

        APP_PRINT_TRACE2("app_dongle_adjust_gaming_latency: latency_value %d latency_plus %d",
                         latency_value, latency_plus);

        app_dongle_set_gaming_latency(p_dongle_link->a2dp_track_handle, latency_value);
    }
}
#else
void app_dongle_adjust_gaming_latency(void)
{
}
#endif

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
bool app_dongle_disallow_bt_streaming(void)
{
    return (dongle_status.usb_is_streaming && dongle_status.silence_stream_for_a_while == false);
}
#endif

/*
* subcommand : 0x23
* bd address(6 byte)
* afh policy mode (1 byte): BT = 0, 2.4G = 1
* afh policy priority (1 byte): remote first = 0, local first = 1
*/
static void app_dongle_send_link_info_to_lowerstack(uint8_t *addr)
{
    uint8_t params[9] = {0};

    params[0] = 0x23;

    memcpy(params + 1, addr, 6);

    if (app_link_check_dongle_link(addr))
    {
        params[7] = 1; /* 2.4g */
    }
    else
    {
        params[7] = 0; /* bt */
    }

    params[8] = 1; /* local first; due to earbud is rx */

    APP_PRINT_TRACE1("app_dongle_send_link_info_to_lowerstack: %b", TRACE_BINARY(sizeof(params),
                                                                                 params));

    gap_vendor_cmd_req(0xfd81, sizeof(params), params);
}

static void app_dongle_handle_acl_disc_event(T_BT_EVENT_PARAM *param)
{
    if (app_link_check_dongle_link(param->acl_conn_disconn.bd_addr))
    {
        app_db.restore_dongle_recording = false;
#if F_APP_LOW_RSSI_DISCONNECT_DONGLE
        app_stop_timer(&timer_idx_low_rssi_disconnect_dongle);
        dongle_ctrl_data.low_rssi_disconnet_dongle = false;
#endif

#if F_APP_ALLOW_LEGACY_GAMING_TX_3M || F_APP_LOW_RSSI_DISCONNECT_DONGLE
        bt_link_rssi_report(param->acl_conn_disconn.bd_addr, false, 0);
#endif
    }

    uint8_t adv_handle = 0;

#if F_APP_LEA_DONGLE_BINDING
    adv_handle = app_lea_adv_get_handle();
#else // F_APP_LEGACY_DONGLE_BINDING
    adv_handle = app_dongle_get_legacy_adv_handle();
#endif

    app_dongle_handle_heaset_adv_interval(ADV_INTERVAL_EVENT_START_FAST_ADV);

    /* prevent 2.4g pairing being interrupted when force enter pairing in 2.4g mode only */
    bool curent_is_2_4g_pairing = (dongle_ctrl_data.enable_pairing &&
                                   (ble_ext_adv_mgr_get_adv_state(adv_handle) == BLE_EXT_ADV_MGR_ADV_ENABLED));

    if (app_cfg_const.enable_dongle_dual_mode)
    {
        if (app_cfg_const.enable_multi_link)
        {
            uint32_t bond_flag;

            if (bt_bond_flag_get(param->acl_conn_disconn.bd_addr, &bond_flag) &&
                (bond_flag & BOND_FLAG_DONGLE) &&
                curent_is_2_4g_pairing == false)
            {
                app_dongle_adv_start(false);
            }
        }
        else
        {
            if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G && curent_is_2_4g_pairing == false)
            {
                app_dongle_adv_start(false);
            }
        }
    }

#if F_APP_DONGLE_MULTI_PAIRING
    if (dongle_ctrl_data.switch_pairing_triggered)
    {
        app_dongle_switch_pairing_mode();
    }
#endif

    if (dongle_ctrl_data.linkback_src_after_disc)
    {
        dongle_ctrl_data.linkback_src_after_disc = false;
        linkback_handle_after_mode_switch(dongle_ctrl_data.linkback_status);
        memset(&dongle_ctrl_data.linkback_status, 0, sizeof(T_DONGLE_LINKBACK_STATUS));
    }
}

static void app_dongle_adv_control(bool dongle_streaming)
{
    if (app_cfg_nv.legacy_gaming_support_common_adv == false)
    {
        if (dongle_streaming)
        {
            app_dongle_handle_rtk_adv(false);
        }
        else
        {
            app_dongle_handle_rtk_adv(true);
        }
    }

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
//Bandwidth limitation, need stop adv when dongle streaming
    if (extend_app_cfg_const.gfps_support)
    {
        if (dongle_streaming)
        {
            if (app_gfps_adv_get_state() == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                ble_ext_adv_mgr_disable(app_gfps_adv_get_handle(), APP_STOP_ADV_CAUSE_LEGACY_GAMING_STREAMING);
            }
        }
        else
        {
            app_gfps_device_check_state((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
        }
    }
#endif

#if F_APP_LEA_SUPPORT
    if (dongle_streaming)
    {
        app_lea_adv_stop();
    }
    else
    {
        if (app_dongle_get_connected_lea_phone_link() == NULL)
        {
            app_lea_adv_start();
        }
    }
#endif
}

#if F_APP_ALLOW_LEGACY_GAMING_TX_3M
void app_dongle_enable_tx_3M(bool enable_3M)
{
    T_BT_ACL_PKT_TYPE pkt_type = BT_ACL_PKT_TYPE_2M;
    uint8_t dongle_addr[6] = {0};

    if (enable_3M)
    {
        headset_status.enable_3M = true;

        pkt_type = BT_ACL_PKT_TYPE_3M;
    }
    else
    {
        headset_status.enable_3M = false;
    }

    if (app_dongle_get_connected_dongle_addr(dongle_addr))
    {
        bt_acl_pkt_type_set(dongle_addr, pkt_type);
    }
    else
    {
#if F_APP_LEA_SUPPORT
        if (app_dongle_get_connected_lea_phone_link() == NULL)
        {
            app_lea_adv_start();
        }
#endif
    }
}

static void app_dongle_check_change_pkt_type(int8_t dongle_rssi)
{
    bool mic_data_tx_3M = false;
    bool force_tx_3M = false;

#if F_APP_GAMING_CONTROLLER_SUPPORT
    force_tx_3M = true;
#endif

    if (app_db.dongle_is_enable_mic &&
        (app_cfg_const.spp_voice_smaple_rate == RECORD_SAMPLE_RATE_32K ||
         app_cfg_const.spp_voice_smaple_rate == RECORD_SAMPLE_RATE_48K))
    {
        mic_data_tx_3M = true;
    }

    if (headset_status.enable_3M == false)
    {
        if (force_tx_3M ||
            (dongle_rssi > RSSI_THRESHOLD_CHANGE_TO_3M || mic_data_tx_3M))
        {
            if (timer_idx_check_change_pkt_type == 0)
            {
                app_start_timer(&timer_idx_check_change_pkt_type,
                                "check_change_pkt_type",
                                app_dongle_timer_id, APP_TIMER_CHECK_CHANGE_PKT_TYPE, 0, false,
                                CHANGE_TO_3M_DETECT_TIME * 1000);
            }
        }
        else
        {
            app_stop_timer(&timer_idx_check_change_pkt_type);
        }
    }
    else
    {
        if (dongle_rssi < RSSI_THRESHOLD_CHANGE_TO_3M)
        {
            if (timer_idx_check_change_pkt_type == 0)
            {
                app_start_timer(&timer_idx_check_change_pkt_type,
                                "check_change_pkt_type",
                                app_dongle_timer_id, APP_TIMER_CHECK_CHANGE_PKT_TYPE, 0, false,
                                CHANGE_TO_2M_DETECT_TIME * 1000);
            }
        }
        else
        {
            app_stop_timer(&timer_idx_check_change_pkt_type);
        }
    }
}
#endif

static void app_dongle_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = false;
    T_APP_BR_LINK *p_link = NULL;

    switch (event_type)
    {
    case BT_EVENT_SPP_CONN_CMPL:
        {
            if (app_link_check_dongle_link(param->spp_conn_cmpl.bd_addr))
            {
                app_dongle_sync_headset_status();

                app_gaming_sync_set_link_connected(true, param->spp_conn_cmpl.bd_addr);

                app_audio_update_dongle_flag(true);

#if F_APP_MUTLILINK_SOURCE_PRIORITY_UI
                p_link = app_link_find_br_link(param->spp_conn_cmpl.bd_addr);

                if (p_link)
                {
                    app_multilink_customer_set_dongle_priority(p_link->id);
                }
#endif
            }
        }
        break;

    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            if (app_link_check_dongle_link(param->spp_disconn_cmpl.bd_addr))
            {
                app_gaming_sync_set_link_connected(false, param->spp_disconn_cmpl.bd_addr);
            }
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {

        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            if (param->sco_conn_cmpl.cause == 0)
            {
#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
                memcpy(&sco_conn_cmpl_param, (uint8_t *)&param->sco_conn_cmpl,
                       sizeof(T_BT_EVENT_PARAM_SCO_CONN_CMPL));
#endif

                if (app_link_check_phone_link(param->sco_conn_cmpl.bd_addr))
                {
                    app_dongle_update_phone_stream_status(PHONE_EVENT_SCO_START);
                }
            }

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
            app_dongle_handle_hfp_call();
#endif
        }
        break;

    case BT_EVENT_SCO_DISCONNECTED:
        {
            if (app_link_check_phone_link(param->sco_disconnected.bd_addr))
            {
                app_dongle_update_phone_stream_status(PHONE_EVENT_SCO_STOP);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            if (app_link_check_phone_link(param->a2dp_stream_start_ind.bd_addr))
            {
                app_dongle_update_phone_stream_status(PHONE_EVENT_A2DP_START);
            }

            if (app_link_check_dongle_link(param->a2dp_stream_start_ind.bd_addr))
            {
                T_APP_BR_LINK *p_link = app_dongle_get_connected_dongle_link();

                if (p_link != NULL)
                {
                    app_link_update_a2dp_streaming(p_link, true);
                }

                if (app_cfg_nv.legacy_gaming_support_common_adv)
                {
                    ble_ext_adv_mgr_change_adv_interval(app_ble_common_adv_get_adv_handle(), 0x0400);
                }
            }

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
            memcpy(&a2dp_stream_para, (uint8_t *)&param->a2dp_stream_start_ind,
                   sizeof(T_BT_EVENT_PARAM_A2DP_STREAM_START_IND));
#endif
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            if (app_link_check_phone_link(param->a2dp_stream_start_ind.bd_addr))
            {
                app_dongle_update_phone_stream_status(PHONE_EVENT_A2DP_STOP);
            }

            if (app_link_check_dongle_link(param->a2dp_stream_stop.bd_addr))
            {
                T_APP_BR_LINK *p_link = app_dongle_get_connected_dongle_link();

                if (p_link != NULL)
                {
                    app_link_update_a2dp_streaming(p_link, false);
                }
            }
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            if (app_link_check_phone_link(param->a2dp_disconn_cmpl.bd_addr))
            {
                if (headset_status.phone_status == PHONE_STREAM_A2DP)
                {
                    app_dongle_update_phone_stream_status(PHONE_EVENT_A2DP_DISC);
                }
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            if (app_link_check_phone_link(param->acl_conn_disconn.bd_addr) &&
                param->acl_conn_disconn.cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
            {
                app_dongle_update_phone_stream_status(PHONE_EVENT_PHONE_DISC);

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
                app_dongle_update_headset_conn_status(HEADSET_CONN_EVENT_LEGACY_DISCONNECTED);
#endif
            }

            app_dongle_handle_acl_disc_event(param);

            handle = true;
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            uint8_t *addr = param->acl_conn_success.bd_addr;

#if F_APP_DONGLE_MULTI_PAIRING
            if (app_db.cmd_dsp_config_enable_multilink)
            {
                app_db.cmd_dsp_config_enable_multilink = 0;
            }
#endif

            if (app_link_check_b2s_link(addr))
            {
                app_dongle_send_link_info_to_lowerstack(addr);
            }

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
            if (app_link_check_phone_link(addr))
            {
                app_dongle_update_headset_conn_status(HEADSET_CONN_EVENT_LEGACY_CONNECTED);
            }
#endif

            if (app_link_check_dongle_link(addr))
            {
                memcpy(app_cfg_nv.dongle_addr, addr, 6);

                headset_status.enable_3M = false;
            }

            if (/* consider the first time connection has no bond flag yet
                   (connected_dongle_addr will be updated when recieve conn ind with gaming cod) */
                !memcmp(app_cfg_nv.dongle_addr, addr, 6) ||
                app_link_check_dongle_link(addr))
            {
#if F_APP_ALLOW_LEGACY_GAMING_TX_3M || F_APP_LOW_RSSI_DISCONNECT_DONGLE
                bt_link_rssi_report(param->acl_conn_success.bd_addr, true, 300);
#endif

#if F_APP_LEGACY_DONGLE_BINDING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
                dongle_ctrl_data.disallow_dongle_a2dp = true;
                app_start_timer(&timer_idx_disallow_dongle_a2dp,
                                "disallow dongle a2dp",
                                app_dongle_timer_id, APP_TIMER_DISALLOW_DONGLE_A2DP, 0, false,
                                2000);
#endif
            }

            handle = true;
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
#if F_APP_GAMING_CONTROLLER_SUPPORT
            if (app_link_check_dongle_link(param->acl_conn_ready.bd_addr))
            {
                gap_br_cfg_acl_link_flush_tout(param->acl_conn_ready.bd_addr, 10);

                handle = true;
            }
#endif
        }
        break;

    case BT_EVENT_LINK_RSSI_INFO:
        {
            bool is_dongle_link = app_link_check_dongle_link(param->link_rssi_info.bd_addr);

#if F_APP_ALLOW_LEGACY_GAMING_TX_3M
            if (is_dongle_link)
            {
                APP_PRINT_TRACE1("app_dongle_bt_cback: dongle rssi %d", param->link_rssi_info.rssi);

                app_dongle_check_change_pkt_type(param->link_rssi_info.rssi);
            }
#endif

#if F_APP_LOW_RSSI_DISCONNECT_DONGLE
            if (param->link_rssi_info.rssi != 0 && is_dongle_link)
            {
                if (param->link_rssi_info.rssi < DONGLE_DISCONNECT_RSSI_THRESHOLD)
                {
                    if (timer_idx_low_rssi_disconnect_dongle == 0 &&
                        dongle_ctrl_data.low_rssi_disconnet_dongle == false)
                    {
                        app_start_timer(&timer_idx_low_rssi_disconnect_dongle,
                                        "low_rssi_disconnect_dongle",
                                        app_dongle_timer_id, APP_TIMER_LOW_RSSI_DISCONNECT_DONGLE, 0, false,
                                        2000);
                    }
                }
                else
                {
                    if (timer_idx_low_rssi_disconnect_dongle)
                    {
                        app_stop_timer(&timer_idx_low_rssi_disconnect_dongle);
                    }
                }
            }
#endif

            handle = true;
        }
        break;

    case BT_EVENT_A2DP_SNIFFING_START_IND:
        {
#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
            memcpy(&a2dp_stream_para, (uint8_t *)&param->a2dp_sniffing_start_ind,
                   sizeof(T_BT_EVENT_PARAM_A2DP_SNIFFING_START_IND));
#endif
        }
        break;

    default:
        break;
    }

    if (handle)
    {
        APP_PRINT_TRACE1("app_dongle_bt_cback: event 0x%04x", event_type);
    }
}

#if F_APP_DONGLE_MULTI_PAIRING
static bool app_dongle_check_disc_src_before_switch_pairing(void)
{
    bool ret = false;

    uint8_t i = 0;
    uint32_t bond_flag;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        T_APP_BR_LINK *p_link = &app_db.br_link[i];

        if (p_link->used == true &&
            app_link_check_b2s_link(p_link->bd_addr) &&
            bt_bond_flag_get(p_link->bd_addr, &bond_flag))
        {
            bool disc_src = false;

            if (bond_flag & BOND_FLAG_DONGLE)
            {
                if (app_cfg_nv.is_bt_pairing)
                {
                    disc_src = true;
                }
            }
            else
            {
                if (app_cfg_nv.is_bt_pairing == false)
                {
                    disc_src = true;
                }
            }

            if (disc_src)
            {
                app_bt_policy_disconnect(p_link->bd_addr, ALL_PROFILE_MASK);
                ret = true;
                break;
            }
        }
    }


    APP_PRINT_TRACE1("app_dongle_check_disc_src_before_switch_pairing: ret %d", ret);

    return ret;
}

void app_dongle_switch_pairing_mode(void)
{
    int8_t err = 0;

    if (app_cfg_const.enable_dongle_multi_pairing == false)
    {
        err = -1;
        goto exit;
    }

    if (app_cfg_nv.dongle_rf_mode != DONGLE_RF_MODE_24G_BT)
    {
        err = -2;
        goto exit;
    }

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        err = -3;
        goto exit;
    }

#if F_APP_LEA_SUPPORT
    if (app_link_get_lea_link_num() != 0)
    {
        /* Don't switch pairing mode when 2.4G already connected */
        err = -4;
        goto exit;
    }
#endif

    dongle_ctrl_data.switch_pairing_triggered = true;

    if (app_link_get_b2s_link_num() != 0)
    {
        if (app_dongle_check_disc_src_before_switch_pairing())
        {
            err = -5;
            goto exit;
        }
    }

    app_cfg_nv.is_bt_pairing ^= 1;

#if F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT
    if (app_cfg_const.enable_24g_bt_audio_source_switch)
    {
        if ((app_cfg_nv.is_bt_pairing && app_cfg_nv.allowed_source == ALLOWED_SOURCE_24G) ||
            (app_cfg_nv.is_bt_pairing == false && app_cfg_nv.allowed_source == ALLOWED_SOURCE_BT))
        {
            app_dongle_switch_allowed_source();
        }
    }
#endif

    app_bt_policy_enter_pairing_mode(true, true);

    if (app_cfg_nv.is_bt_pairing == true)
    {
        app_dongle_adv_start(false);
    }
    else
    {
        app_dongle_adv_start(true);
    }

    dongle_ctrl_data.switch_pairing_triggered = false;

exit:
    APP_PRINT_TRACE2("app_dongle_switch_pairing_mode: err %d is_bt_pairing %d",
                     err, app_cfg_nv.is_bt_pairing);
}
#endif

void app_dongle_handle_rtk_adv(bool enable_adv)
{
    if (app_cfg_const.rtk_app_adv_support)
    {
        bool allow_play_adv = false;

        if (app_dongle_is_streaming() == false || app_cfg_nv.legacy_gaming_support_common_adv)
        {
            allow_play_adv = true;
        }

#if F_APP_GAMING_WIRED_MODE_HANDLE
        if (app_dongle_get_wired_status())
        {
            allow_play_adv = false;
        }
#endif

        if (enable_adv && allow_play_adv)
        {
            app_ble_common_adv_start_rws(app_cfg_const.timer_ota_adv_timeout * 100);
        }
        else //disable adv
        {
            if (app_ble_common_adv_get_state() == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                app_ble_common_adv_stop(APP_STOP_ADV_CAUSE_LEGACY_GAMING_STREAMING);
            }
        }
    }
}

uint8_t app_dongle_get_phone_bond_num(void)
{
    uint8_t i;
    uint8_t bond_num = app_bond_b2s_num_get();
    uint32_t bond_flag;
    uint8_t addr[6] = {0};
    uint8_t num = 0;

    for (i = 1; i <= bond_num; i++)
    {
        bond_flag = 0;
        if (app_bond_b2s_addr_get(i, addr))
        {
            bt_bond_flag_get(addr, &bond_flag);

            if ((bond_flag & BOND_FLAG_DONGLE) == 0)
            {
                ++num;
            }
        }
    }

    return num;
}

uint8_t app_dongle_get_dongle_num(void)
{
    uint8_t i;
    uint8_t bond_num = app_bond_b2s_num_get();
    uint32_t bond_flag;
    uint8_t addr[6] = {0};
    uint8_t num = 0;

    for (i = 1; i <= bond_num; i++)
    {
        bond_flag = 0;
        if (app_bond_b2s_addr_get(i, addr))
        {
            bt_bond_flag_get(addr, &bond_flag);

            if (bond_flag & BOND_FLAG_DONGLE)
            {
                ++num;
            }
        }
    }

#if TARGET_LE_AUDIO_GAMING
    /* dongle num always 1 if have le audio dongle record */
    num = app_cfg_nv.le_audio_dongle_connected_record ? 1 : 0;
#endif

    return num;
}

T_APP_DONGLE_STATE app_dongle_get_state(void)
{
    return dongle_state;
}

static bool app_dongle_shutdown_is_synchronous(void)
{
    bool ret = false;

    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED) &&
        (app_link_get_b2s_link_num() == 0))
    {
        ret = true;
    }

    return ret;
}

static void app_dongle_check_disc_and_linkback(uint8_t new_mode)
{
    uint8_t i = 0;
    uint32_t bond_flag = 0;

    APP_PRINT_TRACE1("app_dongle_check_disc_and_linkback: new_mode %d", new_mode);

    if (app_cfg_const.enable_multi_link)
    {
        /* new mode is bt mode */
        if (new_mode == 1)
        {
            /* linkback phone */
            app_bt_policy_conn_all_b2s();
        }
        else
        {
            /* disc phone */
            for (i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                T_APP_BR_LINK *p_link = &app_db.br_link[i];

                if (p_link->used &&
                    app_link_check_b2s_link(p_link->bd_addr) &&
                    bt_bond_flag_get(p_link->bd_addr, &bond_flag) &&
                    (bond_flag & BOND_FLAG_DONGLE) == 0)
                {
                    app_bt_policy_disconnect(p_link->bd_addr, ALL_PROFILE_MASK);
                }
            }
        }
    }
    else
    {
        if (app_link_get_b2s_link_num() == 0)
        {
            app_bt_policy_conn_all_b2s();
        }
        else
        {
            dongle_ctrl_data.linkback_src_after_disc = true;
            app_bt_policy_disc_all_b2s();
        }
    }
}

#if (F_APP_ERWS_SUPPORT == 0)
bool app_dongle_dual_mode_linkback(void)
{
    bool linkback = false;
    uint8_t phone_num = app_dongle_get_phone_bond_num();
    bool ret = false;

    if (app_cfg_const.enable_dongle_dual_mode == false)
    {
        return false;
    }

    if (app_cfg_nv.dongle_rf_mode != DONGLE_RF_MODE_24G)
    {
        if (phone_num > 0)
        {
            linkback = true;
        }
    }

    APP_PRINT_TRACE2("app_dongle_dual_mode_linkback: phone_num %d linkback %d",
                     phone_num, linkback);

    if (linkback)
    {
        app_bt_policy_conn_all_b2s();
        ret = true;
    }

    return ret;
}
#endif

void app_dongle_shutdown_end_check(void)
{
    if (dongle_ctrl_data.start_linkback_when_shutdown)
    {
        if (app_cfg_const.enable_dongle_dual_mode && app_cfg_const.enable_multi_link == false)
        {
            if (app_dongle_get_phone_bond_num() == 0)
            {
                dongle_ctrl_data.enter_pairing_after_mode_switch = true;
            }
        }

        dongle_ctrl_data.start_linkback_when_shutdown = false;
        app_device_bt_policy_startup(true);
    }
}

T_PHONE_STREAM_STATUS app_dongle_get_phone_status(void)
{
    return headset_status.phone_status;
}

static void app_dongle_handle_state_change(T_APP_DONGLE_STATE pre_state,
                                           T_APP_DONGLE_STATE new_state)
{
    bool pairing_adv = false;

    if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G)
    {
        if (new_state == DONGLE_STATE_PAIRING)
        {
            pairing_adv = true;
        }

        app_dongle_adv_start(pairing_adv);
    }
    else if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_BT)
    {
        app_dongle_adv_stop();

#if F_APP_BLE_HID_CONTROLLER_SUPPORT
        if (new_state == DONGLE_STATE_PAIRING)
        {
            app_ble_controller_adv_start(true);
        }
#endif
    }
    else // DONGLE_RF_MODE_24G_BT
    {
        if ((app_link_get_b2s_link_num() < 2 && app_dongle_get_connected_dongle_link() == NULL)
#if F_APP_DONGLE_MULTI_PAIRING
            && (app_db.cmd_dsp_config_enable_multilink == 0)
#endif
           )
        {
            if (new_state == DONGLE_STATE_PAIRING)
            {
                if (app_cfg_const.enable_dongle_multi_pairing && app_cfg_nv.is_bt_pairing)
                {
                    pairing_adv = false;
                }
                else
                {
                    pairing_adv = true;
                }
            }

            app_dongle_adv_start(pairing_adv);
        }
        else
        {
            app_dongle_adv_stop();
        }

        app_ipc_publish(APP_DEVICE_IPC_TOPIC, APP_DEVICE_IPC_EVT_DONGLE_DUAL_MODE, NULL);
    }

#if F_APP_BLE_SWIFT_PAIR_SUPPORT
    if (app_cfg_const.swift_pair_support)
    {
        if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G)
        {
            app_swift_pair_stop_adv_change_timer();
            app_swift_pair_adv_stop(APP_STOP_ADV_CAUSE_EXIT_PAIRING_MODE);
        }
        else
        {
            if (new_state == DONGLE_STATE_PAIRING)
            {
                bool start_swift_pair_adv = false;

                if (app_cfg_const.enable_dongle_multi_pairing)
                {
                    if (app_cfg_nv.is_bt_pairing)
                    {
                        start_swift_pair_adv = true;
                    }
                }
                else
                {
                    start_swift_pair_adv = true;
                }

                if (start_swift_pair_adv)
                {
                    uint16_t swift_adv_keep_vendor_section_timeout = (app_cfg_const.timer_pairing_timeout ?
                                                                      (app_cfg_const.timer_pairing_timeout - APP_SWIFT_PAIR_DEFAULT_ADV_DATA_CHANGE_PRIOR_TIMEOUT) :
                                                                      0xffff);
                    if (!swift_adv_keep_vendor_section_timeout)
                    {
                        app_swift_pair_start_adv_change_timer(APP_SWIFT_PAIR_ADV_KEEP_VENDOR_SECTUON_MIN_TIMEOUT);
                    }
                    else if (swift_adv_keep_vendor_section_timeout != 0xffff)
                    {
                        app_swift_pair_start_adv_change_timer(swift_adv_keep_vendor_section_timeout);
                    }
                    app_swift_pair_adv_start(APP_SWIFT_PAIR_DEFAULT_TIMEOUT);
                }
            }
        }
    }
#endif

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    if (new_state == DONGLE_STATE_LINKBACK)
    {
        app_vendor_htpoll_control(B2S_HTPOLL_EVENT_LINKBACK_START);
    }
    else if (pre_state == DONGLE_STATE_LINKBACK)
    {
        app_vendor_htpoll_control(B2S_HTPOLL_EVENT_LINKBACK_STOP);
    }
#endif
}

void app_dongle_state_machine(T_APP_DONGLE_EVENT event)
{
    T_APP_DONGLE_STATE pre_state = dongle_state;
    T_APP_DONGLE_STATE new_state;

    if (event == DONGLE_EVENT_STANDBY)
    {
        new_state = DONGLE_STATE_STANDBY;
    }
    else if (event == DONGLE_EVENT_LINKBACK)
    {
        new_state = DONGLE_STATE_LINKBACK;
    }
    else // DONGLE_EVENT_PAIRING
    {
        new_state = DONGLE_STATE_PAIRING;
    }

    dongle_state = new_state;

    bool pre_linkback_status = headset_status.headset_linkback;

    if (dongle_state == DONGLE_STATE_LINKBACK)
    {
        headset_status.headset_linkback = true;
    }
    else
    {
        headset_status.headset_linkback = false;
    }

    if (pre_linkback_status != headset_status.headset_linkback)
    {
        app_dongle_sync_headset_status();
    }

    if (pre_state != new_state)
    {
        app_dongle_handle_state_change(pre_state, new_state);
    }

}
void app_dongle_dual_mode_switch(void)
{
    T_APP_DONGLE_RF_MODE prev_mode = (T_APP_DONGLE_RF_MODE)app_cfg_nv.dongle_rf_mode;
    T_APP_DONGLE_RF_MODE next_mode = DONGLE_RF_MODE_24G;
    uint8_t cause = 0;

    if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_BT)
    {
        if (app_cfg_const.enable_multi_link)
        {
            /* support 2.4G<->2.4G+BT when multilink enable */
            cause = 1;
            goto exit;
        }
        else
        {
            next_mode = DONGLE_RF_MODE_24G;
        }
    }
    else if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G)
    {
        if (app_cfg_const.enable_multi_link)
        {
            next_mode = DONGLE_RF_MODE_24G_BT;
        }
        else
        {
            next_mode = DONGLE_RF_MODE_BT;
        }
    }
    else if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G_BT)
    {
        if (app_cfg_const.enable_multi_link)
        {
            next_mode = DONGLE_RF_MODE_24G;
        }
        else
        {
            /* support 2.4G<->BT when multilink not enable */
            cause = 2;
            goto exit;
        }
    }

    app_dongle_switch_rf_mode(next_mode);

#if F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT
    if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G &&
        app_cfg_nv.allowed_source == ALLOWED_SOURCE_BT)
    {
        app_dongle_set_allowed_source(ALLOWED_SOURCE_24G);
    }
    else if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_BT &&
             app_cfg_nv.allowed_source == ALLOWED_SOURCE_24G)
    {
        app_dongle_set_allowed_source(ALLOWED_SOURCE_BT);
    }
#endif

exit:
    APP_PRINT_TRACE3("app_dongle_dual_mode_switch: mode (%d->%d) cause %d",
                     prev_mode, next_mode, cause);
}

#if F_APP_ERWS_SUPPORT
uint16_t app_dongle_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;
    uint8_t tmp = 0;

    switch (msg_type)
    {
    case APP_REMOTE_MSG_SYNC_DONGLE_RF_MODE:
        {
            tmp = app_cfg_nv.dongle_rf_mode;

            msg_ptr = &tmp;
            payload_len = 1;
            skip = false;
        }
        break;

    case APP_REMOTE_MSG_SYNC_HEADSET_STATUS:
        {
            msg_ptr = (uint8_t *)&headset_status;
            payload_len = sizeof(headset_status);

            skip = false;
        }
        break;

    default:
        break;
    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_DONGLE_DUAL_MODE, payload_len, msg_ptr,
                              skip, total);
}
#endif

#if F_APP_ERWS_SUPPORT
static void app_dongle_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                   T_REMOTE_RELAY_STATUS status)
{
    switch (msg_type)
    {
    case APP_REMOTE_MSG_SYNC_DONGLE_RF_MODE:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_RCVD) &&
                (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
            {
                uint8_t *p_info = (uint8_t *)buf;

                uint8_t tmp = p_info[0];

                APP_PRINT_TRACE2("app_dongle_parse_cback: dongle_rf_mode %d -> %d", app_cfg_nv.dongle_rf_mode, tmp);

                app_cfg_nv.dongle_rf_mode = tmp;
                app_ipc_publish(APP_DEVICE_IPC_TOPIC, APP_DEVICE_IPC_EVT_DONGLE_DUAL_MODE, NULL);
            }
        }
        break;

    case APP_REMOTE_MSG_SYNC_HEADSET_STATUS:
        {
            if ((status == REMOTE_RELAY_STATUS_ASYNC_RCVD) &&
                (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
            {
                T_PHONE_STREAM_STATUS pre_phone_stream_status = headset_status.phone_status;

                memcpy((void *)&headset_status, buf, sizeof(headset_status));

                APP_PRINT_TRACE1("headset_status %b", TRACE_BINARY(sizeof(headset_status), &headset_status));

                if (pre_phone_stream_status != headset_status.phone_status)
                {
                    if (headset_status.phone_status == PHONE_STREAM_A2DP ||
                        headset_status.phone_status == PHONE_STREAM_HFP)
                    {
                        app_dongle_handle_heaset_adv_interval(ADV_INTERVAL_EVENT_BT_STREAMING_START);
                    }
                    else
                    {
                        app_dongle_handle_heaset_adv_interval(ADV_INTERVAL_EVENT_BT_STREAMING_STOP);
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}
#endif

static void update_mode_according_to_slide_switch_status(void)
{
    T_APP_SLIDE_SWITCH_GPIO_ACTION action = app_slide_switch_get_dual_mode_gpio_action();

    if (action == APP_SLIDE_SWITCH_DUAL_MODE_GPIO_ACTION_BT)
    {
        if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G)
        {
            if (app_cfg_const.enable_multi_link)
            {
                app_cfg_nv.dongle_rf_mode = DONGLE_RF_MODE_24G_BT;
            }
            else
            {
                app_cfg_nv.dongle_rf_mode = DONGLE_RF_MODE_BT;
            }
            app_cfg_store(&app_cfg_nv.offset_led_breath_mode_min_brightness_level, 1);
        }
    }
    else if (action == APP_SLIDE_SWITCH_DUAL_MODE_GPIO_ACTION_2_4G)
    {
        if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G_BT)
        {
            app_cfg_nv.dongle_rf_mode = DONGLE_RF_MODE_24G;

            app_cfg_store(&app_cfg_nv.offset_led_breath_mode_min_brightness_level, 1);
        }
    }
}

static void get_switch_rf_mode_linkback_status(T_APP_DONGLE_RF_MODE mode,
                                               T_DONGLE_LINKBACK_STATUS *status,
                                               T_APP_BR_LINK *dongle_link, T_APP_BR_LINK *phone_link)
{
    if (mode == DONGLE_RF_MODE_24G)
    {
        /* disconnect phone and linkback dongle */
        if (phone_link != NULL)
        {
            status->disc_phone = true;
        }

        if (dongle_link == NULL)
        {
            status->linkback_dongle = true;
        }

        status->linkback_phone = false;
    }
    else if (mode == DONGLE_RF_MODE_BT)
    {
        /* disconnect dongle and linkback phone */
        if (dongle_link != NULL)
        {
            status->disc_dongle = true;
        }

        uint8_t phone_bond_num = app_dongle_get_phone_bond_num();

        if (phone_bond_num == 0)
        {
#if F_APP_BLE_HID_CONTROLLER_SUPPORT == 0
            status->trigger_bt_pairing = true;
#endif
        }
        else
        {
            if (phone_link == NULL)
            {
                status->linkback_phone = true;
            }
        }

        status->linkback_dongle = false;
    }
    else // DONGLE_RF_MODE_24G_BT
    {
        /* linkback dongle and phone */
        if (dongle_link == NULL)
        {
            status->linkback_dongle = true;
        }

        if (phone_link == NULL)
        {
            status->linkback_phone = true;
        }
    }

}

static void handle_switch_rf_mode(T_APP_DONGLE_RF_MODE mode)
{
    T_APP_BR_LINK *dongle_link = app_dongle_get_connected_dongle_link();
    T_APP_BR_LINK *phone_link = app_dongle_get_connected_phone_link();

    next_rf_mode = mode;

    /* debounce */
    if (timer_idx_switch_rf_mode_debounce == 0)
    {
        app_start_timer(&timer_idx_switch_rf_mode_debounce, "switch_rf_mode_debounce",
                        app_dongle_timer_id, APP_TIMER_SWITCH_RF_MODE_DEBOUNCE, 0, false,
                        SWITCH_RF_MODE_DEBOUNCE_TIME);
    }
    else
    {
        return;
    }

    if (mode == app_cfg_nv.dongle_rf_mode)
    {
        return;
    }

    get_switch_rf_mode_linkback_status(mode, &dongle_ctrl_data.linkback_status, dongle_link,
                                       phone_link);

    T_DONGLE_LINKBACK_STATUS status = dongle_ctrl_data.linkback_status;

    app_cfg_nv.dongle_rf_mode = mode;

    APP_PRINT_TRACE5("handle_switch_rf_mode: mode %d disc_phone %d disc_dongle %d linkback_phone %d linkback_dongle %d",
                     mode, status.disc_phone, status.disc_dongle, status.linkback_phone, status.linkback_dongle);

    /* check stop linkback phone */
    if (mode == DONGLE_RF_MODE_24G && app_bt_policy_get_state() == BP_STATE_LINKBACK)
    {
        app_dongle_cancel_linkback();
    }

#if F_APP_BLE_HID_CONTROLLER_SUPPORT
    if (mode == DONGLE_RF_MODE_24G)
    {
        app_ble_controller_adv_stop();

        app_ble_gap_disconnect_all(LE_LOCAL_DISC_CAUSE_DONGLE_MODE_SWITCH);
    }
    else
    {
        app_ble_controller_adv_start(false);
    }
#endif

    if (status.disc_phone)
    {
        app_bt_policy_disconnect(phone_link->bd_addr, ALL_PROFILE_MASK);
    }

    if (status.disc_dongle)
    {
        /* release audio track first to avoid noise */
        if (dongle_link && dongle_link->a2dp_track_handle)
        {
            app_audio_a2dp_track_release(dongle_link->bd_addr);
        }

        app_bt_policy_disconnect(dongle_link->bd_addr, ALL_PROFILE_MASK);
    }

#if F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT
    /* handle source switch */
    bool switch_source = false;

    if (app_cfg_const.enable_24g_bt_audio_source_switch)
    {
        if ((mode == DONGLE_RF_MODE_24G && app_cfg_nv.allowed_source == ALLOWED_SOURCE_BT) ||
            (mode == DONGLE_RF_MODE_BT && app_cfg_nv.allowed_source == ALLOWED_SOURCE_24G))
        {
            switch_source = true;
        }

        if (switch_source)
        {
            app_dongle_switch_allowed_source();
        }
    }
#endif

    if (status.disc_phone || status.disc_dongle)
    {
        dongle_ctrl_data.linkback_src_after_disc = true;
    }
    else
    {
        linkback_handle_after_mode_switch(status);
    }

    if (mode == DONGLE_RF_MODE_24G)
    {
        app_audio_tone_type_play(TONE_RF_MODE_24G, false, true);
    }
    else if (mode == DONGLE_RF_MODE_BT)
    {
        app_audio_tone_type_play(TONE_RF_MODE_BT, false, true);
    }
    else
    {
        app_audio_tone_type_play(TONE_RF_MODE_24G_BT, false, true);
    }
}

static void app_dongle_timer_cback(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
#if F_APP_LEGACY_DONGLE_BINDING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    case APP_TIMER_DISALLOW_DONGLE_A2DP:
        {
            app_stop_timer(&timer_idx_disallow_dongle_a2dp);

            dongle_ctrl_data.disallow_dongle_a2dp = false;
        }
        break;
#endif

#if F_APP_LOW_RSSI_DISCONNECT_DONGLE
    case APP_TIMER_LOW_RSSI_DISCONNECT_DONGLE:
        {
            uint8_t dongle_addr[6] = {0};

            app_stop_timer(&timer_idx_low_rssi_disconnect_dongle);

            if (app_dongle_get_connected_dongle_addr(dongle_addr))
            {
                dongle_ctrl_data.low_rssi_disconnet_dongle = true;
                app_bt_policy_disconnect(dongle_addr, ALL_PROFILE_MASK);
            }
        }
        break;
#endif

#if F_APP_ALLOW_LEGACY_GAMING_TX_3M
    case APP_TIMER_CHECK_CHANGE_PKT_TYPE:
        {
            app_stop_timer(&timer_idx_check_change_pkt_type);

            if (headset_status.enable_3M == false)
            {
                app_dongle_enable_tx_3M(true);
            }
            else
            {
                app_dongle_enable_tx_3M(false);
            }

            app_dongle_sync_headset_status();
        }
        break;
#endif

    case APP_TIMER_CALL_ILDE_DISC_SCO:
        {
            app_stop_timer(&timer_idx_call_idle_disc_sco);

            T_APP_BR_LINK *p_link;
            p_link = &(app_db.br_link[param]);

            if ((p_link->sco_handle != NULL) && (app_hfp_get_call_status() == APP_CALL_IDLE))
            {
                bt_hfp_audio_disconnect_req(p_link->bd_addr);
            }
        }
        break;

    case APP_TIMER_SWITCH_RF_MODE_DEBOUNCE:
        {
            app_stop_timer(&timer_idx_switch_rf_mode_debounce);

            if (next_rf_mode != app_cfg_nv.dongle_rf_mode)
            {
                handle_switch_rf_mode(next_rf_mode);
            }
        }
        break;

    case APP_TIMER_SEND_FIX_CHAN_EXIT_SNIFF:
        {
            uint8_t dongle_addr[6] = {0};

            app_stop_timer(&timer_idx_send_fix_chan_exit_sniff);

            if (app_dongle_get_connected_dongle_addr(dongle_addr))
            {
                app_sniff_mode_b2s_enable(dongle_addr, SNIFF_DISABLE_MASK_FIX_CHAN_DATA);
            }
        }
        break;

    default:
        break;
    }

}

uint16_t app_dongle_get_gaming_latency(void)
{
#if TARGET_LEGACY_AUDIO_GAMING || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    uint16_t latency_value = ULTRA_LOW_LATENCY_MS;
    uint8_t latency_plus = 0;

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    latency_plus = app_vendor_get_htpoll_latency_plus();
#endif

    return (latency_value + latency_plus);
#else
    return 0;
#endif
}

void app_dongle_handle_ble_disconnected(uint8_t *bd_addr)
{
    bool lea_dongle_disconnected = false;

#if TARGET_LE_AUDIO_GAMING
    if (!memcmp(bd_addr, (uint8_t[6]) {0}, 6))
    {
        return;
    }

    if (app_link_check_dongle_link(bd_addr))
    {
        lea_dongle_disconnected = true;
    }

    if (lea_dongle_disconnected)
    {
        app_dongle_handle_heaset_adv_interval(ADV_INTERVAL_EVENT_START_FAST_ADV);

        app_dongle_clear_dongle_status();

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
        app_dongle_handle_stream_preempt(EVENT_2_4G_DISCONNECTED);
#endif

        app_dongle_adv_start(false);

        app_gaming_sync_set_le_link_id(LE_LINK_ID_INVALID);
    }
#endif

    APP_PRINT_TRACE2("app_dongle_handle_ble_disconnected: %s lea_dongle_disconnected %d",
                     TRACE_BDADDR(bd_addr), lea_dongle_disconnected);

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    app_vendor_htpoll_control(B2S_HTPOLL_EVENT_BLE_DISCONNECTED);
#endif
}

static void app_dongle_set_sync_le_link_id(uint8_t *bd_addr)
{
    uint8_t i = 0;
    uint8_t link_id = LE_LINK_ID_INVALID;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (!memcmp(app_db.le_link[i].bd_addr, bd_addr, 6))
        {
            link_id = i;
            break;
        }
    }

    app_gaming_sync_set_le_link_id(link_id);
}

/* dongle ble connected handle */
void app_dongle_handle_ble_connected(uint8_t *bd_addr)
{
    bool lea_dongle_connected = false;

#if TARGET_LE_AUDIO_GAMING
    if (app_link_check_dongle_link(bd_addr))
    {
        app_dongle_adv_stop();

        lea_dongle_connected = true;
        app_dongle_sync_headset_status();

        app_dongle_set_sync_le_link_id(bd_addr);
    }
#endif

    APP_PRINT_TRACE2("app_dongle_handle_ble_connected: %s lea_dongle_connected %d",
                     TRACE_BDADDR(bd_addr), lea_dongle_connected);

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
    app_vendor_htpoll_control(B2S_HTPOLL_EVENT_BLE_CONNECTED);
#endif


#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
    bool is_a2dp_streaming = app_a2dp_is_streaming();

    APP_PRINT_TRACE2("app_dongle_handle_ble_connected: bud_role %d, is_a2dp_streaming %d",
                     app_cfg_nv.bud_role, is_a2dp_streaming);

    if (is_a2dp_streaming && !app_cfg_const.enable_24g_bt_audio_source_switch)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            T_APP_BR_LINK *p_link = NULL;

            p_link = &app_db.br_link[app_a2dp_get_active_idx()];

            if (p_link->avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING)
            {
                bt_avrcp_pause(p_link->bd_addr);
                p_link->avrcp_play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
            }
        }
    }
#endif

}

bool app_dongle_set_gaming_latency(T_AUDIO_TRACK_HANDLE p_handle, uint16_t latency_value)
{
    bool ret = false;

    if (p_handle != NULL)
    {
        ret = audio_track_latency_set(p_handle, latency_value, true);

        APP_PRINT_TRACE2("app_dongle_set_gaming_latency: ret %d latency %d", ret, latency_value);
    }

    return ret;
}

void app_dongle_exit_pairing_mode(void)
{
    app_bt_policy_exit_pairing_mode();
}

bool app_dongle_enter_pairing_mode(T_APP_DONGLE_PAIRING_MODE mode)
{
    uint8_t cause = 0;
    bool ret = true;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        cause = 1;
        goto exit;
    }

    if (mode == DONGLE_PAIRING_MODE_NONE)
    {
        cause = 2;
        goto exit;
    }

    if (app_hfp_get_call_status() != APP_CALL_IDLE)
    {
        cause = 3;
        goto exit;
    }

    if (app_cfg_const.enable_dongle_multi_pairing == false)
    {
        if (mode != DONGLE_PAIRING_MODE_ALL)
        {
            cause = 4;
            goto exit;
        }
    }
    else
    {
        if (mode != DONGLE_PAIRING_MODE_24G && mode != DONGLE_PAIRING_MODE_BT)
        {
            cause = 5;
            goto exit;
        }
    }

#if TARGET_LE_AUDIO_GAMING
    if (app_dongle_get_le_audio_dongle_link())
#else
    if (app_dongle_get_connected_dongle_link())
#endif
    {
        if (mode == DONGLE_PAIRING_MODE_24G)
        {
            cause = 6;
            goto exit;
        }
    }

#if F_APP_LEA_SUPPORT == 0
    if (app_dongle_is_streaming())
    {
        cause = 7;
        goto exit;
    }
#endif

    dongle_ctrl_data.keep_pairing_mode_type = mode;

    app_bt_policy_enter_pairing_mode(true, true);

exit:
    if (cause != 0)
    {
        ret = false;
    }

    APP_PRINT_TRACE3("app_dongle_enter_pairing_mode: ret %d mode %d cause -%d", ret, mode, cause);

    return ret;
}

#if F_APP_B2B_ENGAGE_IMPROVE_BY_LEA_DONGLE
void app_dongle_lea_handle_cis_link_number(void)
{
    bool bud_side = dongle_status.snk_audio_loc - 1;
    uint8_t cis_link_number = dongle_status.src_conn_num;
    bool is_engaged = (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) ? true : false;
    bool is_phone_connected = app_link_get_b2s_link_num() > 0 ? true : false;

    if (bud_side != app_cfg_const.bud_side)
    {
        return;
    }

    if (cis_link_number == 2)
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY && !is_engaged && !is_phone_connected)
        {
            app_bt_policy_change_to_sec_and_conn_pri();
        }
    }
}
#endif

#if F_APP_LEA_SUPPORT
void app_donlge_enable_cis_higher_priority(bool enable)
{
    uint8_t params[2] = {0};

    params[0] = 0x05;
    params[1] = (uint8_t)enable;

    gap_vendor_cmd_req(0xfda5, sizeof(params), params);
}
#endif

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
bool app_dongle_get_wait_cis_disc(void)
{
    return wait_cis_disc;
}

void app_dongle_handle_cis_disconnected(void)
{
    if (wait_cis_disc)
    {
        T_APP_BR_LINK *p_link = NULL;

        p_link = &app_db.br_link[app_a2dp_get_active_idx()];

        app_bt_sniffing_start_sco_sniffing(p_link->bd_addr);

        wait_cis_disc = false;
    }
}

void app_dongle_handle_hfp_call(void)
{
    APP_PRINT_TRACE1("app_dongle_handle_hfp_call: call_status %d", app_hfp_get_call_status());

    if ((app_hfp_get_call_status() != APP_CALL_IDLE) ||
        app_dongle_get_phone_status() == PHONE_STREAM_HFP)
    {
        mtc_cis_audio_conext_change(false);
        if (mtc_ase_release())
        {
            wait_cis_disc = true;
        }
        mtc_topology_dm(MTC_TOPO_EVENT_SET_BR_MODE);
    }
}

void app_dongle_le_audio_handle_mic(bool mic_enable)
{
    APP_PRINT_TRACE1("app_dongle_le_audio_handle_mic: mic_enable %d", mic_enable);

    if (mic_enable)
    {
        app_db.dongle_is_enable_mic = true;
        app_dongle_handle_stream_preempt(EVENT_2_4G_CALL_ACTIVE);
    }
    else
    {
        app_db.dongle_is_enable_mic = false;
        app_dongle_handle_stream_preempt(EVENT_2_4G_CALL_END);
    }
}

void app_dongle_handle_stream_preempt(T_APP_DONGLE_PREEMPT_EVENT event)
{
    bool is_a2dp_streaming = false;
    bool is_bt_sniffing = false;
    bool is_a2dp_track_started = false;
    T_APP_DONGLE_PREEMPT_ACTION action = ACTION_ALL;
    uint8_t active_hf_idx = app_hfp_get_active_idx();
    T_APP_CALL_STATUS hfp_call_status = app_hfp_get_call_status();
    uint8_t ret = 0;
    bool b2b_connected = (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) ? true : false;

    if (app_cfg_const.enable_24g_bt_audio_source_switch)
    {
        return;
    }

    T_APP_BR_LINK *p_link = NULL;
    p_link = &app_db.br_link[app_a2dp_get_active_idx()];

    if (p_link)
    {
        T_AUDIO_TRACK_STATE state;
        audio_track_state_get(p_link->a2dp_track_handle, &state);

        if (state == AUDIO_TRACK_STATE_STARTED ||
            state == AUDIO_TRACK_STATE_RESTARTED)
        {
            is_a2dp_streaming = true;
            is_a2dp_track_started = true;
        }
    }

    if (headset_status.phone_status == PHONE_STREAM_A2DP || app_a2dp_is_streaming())
    {
        is_a2dp_streaming = true;
    }

    if (b2b_connected)
    {
        if (p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_STARTING ||
            p_link->bt_sniffing_state == APP_BT_SNIFFING_STATE_SNIFFING)
        {
            is_bt_sniffing = true;
        }
    }

    APP_PRINT_TRACE5("app_dongle_handle_stream_preempt: event 0x%02x, is_a2dp_streaming %d, is_bt_sniffing %d, hfp_call_status %d, is_a2dp_track_started %d",
                     event, is_a2dp_streaming, is_bt_sniffing, hfp_call_status, is_a2dp_track_started);

    // handle event
    switch (event)
    {
    case EVENT_2_4G_STREAM_START:
        {
            // disallow sniffing whether a2dp is streaming or not
            app_link_disallow_legacy_stream(true);

            if (is_a2dp_streaming)
            {
                action = ACTION_2_4G_STREAM_PREEMPT_A2DP_STREAM;
            }
        }
        break;

    case EVENT_2_4G_STREAM_STOP:
        {
            // allow sniffing
            app_link_disallow_legacy_stream(false);

            if (is_a2dp_streaming && hfp_call_status == APP_CALL_IDLE)
            {
                action = ACTION_2_4G_STREAM_STOP_RESUME_A2DP_STREAM;
            }
        }
        break;

    case EVENT_2_4G_CALL_ACTIVE:
        {
            if (hfp_call_status != APP_CALL_IDLE)
            {
                action = ACTION_2_4G_CALL_ACTIVE_PREEMPT_HFP_CALL;
            }
        }
        break;

    case EVENT_2_4G_CALL_END:
        {
            if (hfp_call_status != APP_CALL_IDLE)
            {
                action = ACTION_2_4G_CALL_END_RESUME_HFP_CALL;
            }
        }
        break;

    case EVENT_2_4G_DISCONNECTED:
        {
            // allow sniffing
            app_link_disallow_legacy_stream(false);

            if (hfp_call_status != APP_CALL_IDLE)
            {
                action = ACTION_2_4G_CALL_END_RESUME_HFP_CALL;
            }

            if (is_a2dp_streaming && is_a2dp_track_started == false)
            {
                action = ACTION_2_4G_STREAM_STOP_RESUME_A2DP_STREAM;
            }

        }
        break;
    }

    APP_PRINT_TRACE3("app_dongle_handle_stream_preempt: action 0x%02x, b2b_connected %d, bud_role %d",
                     action, b2b_connected, app_cfg_nv.bud_role);

    // handle preempt action
    switch (action)
    {
    case ACTION_2_4G_STREAM_PREEMPT_A2DP_STREAM:
        {
            T_APP_BR_LINK *p_link = NULL;

            p_link = &app_db.br_link[app_a2dp_get_active_idx()];
            if (b2b_connected && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                //app_bt_sniffing_stop(p_link->bd_addr, BT_SNIFFING_TYPE_A2DP);
                bt_sniffing_link_disconnect(p_link->bd_addr);
            }

            // release a2dp track
            if (p_link != NULL)
            {
                if (p_link->a2dp_track_handle != NULL)
                {
                    audio_track_release(p_link->a2dp_track_handle);
                    p_link->a2dp_track_handle = NULL;
                }

                if (p_link->eq_instance != NULL)
                {
                    eq_release(p_link->eq_instance);
                    p_link->eq_instance = NULL;
                }
                p_link->playback_muted = 0;
            }
        }
        break;

    case ACTION_2_4G_STREAM_STOP_RESUME_A2DP_STREAM:
        {
            mtc_topology_dm(MTC_TOPO_EVENT_SET_BR_MODE);

            ret = app_audio_a2dp_stream_resume((uint8_t *)&a2dp_stream_para);
        }
        break;

    case ACTION_2_4G_CALL_ACTIVE_PREEMPT_HFP_CALL:
        {
            if (b2b_connected && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                bt_hfp_audio_disconnect_req(app_db.br_link[active_hf_idx].bd_addr);
                //app_bt_sniffing_stop(app_db.br_link[active_hf_idx].bd_addr, BT_SNIFFING_TYPE_SCO);
            }

            mtc_cis_audio_conext_change(true);
            mtc_set_btmode(MULTI_PRO_BT_CIS);
        }
        break;

    case ACTION_2_4G_CALL_END_RESUME_HFP_CALL:
        {
            if (b2b_connected && app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_bt_sniffing_hfp_connect(app_db.br_link[active_hf_idx].bd_addr);
            }

            mtc_cis_audio_conext_change(false);
            mtc_ase_release();
            mtc_topology_dm(MTC_TOPO_EVENT_SET_BR_MODE);
        }
        break;
    }

    if (ret)
    {
        APP_PRINT_TRACE1("app_dongle_handle_stream_preempt: ret %d", ret);
    }
}
#endif

void app_dongle_lea_handle_dongle_status(void)
{
    APP_PRINT_TRACE1("app_dongle_lea_handle_dongle_status: dongle_status %b",
                     TRACE_BINARY(sizeof(T_DONGLE_STATUS), &dongle_status));

    app_dongle_save_dongle_addr(dongle_status.dongle_addr);

#if F_APP_B2B_ENGAGE_IMPROVE_BY_LEA_DONGLE
    app_dongle_lea_handle_cis_link_number();
#endif
}

void app_dongle_set_common_adv_coexist_in_legacy_gaming(bool enable)
{
    if (app_cfg_nv.legacy_gaming_support_common_adv != enable)
    {
        app_cfg_nv.legacy_gaming_support_common_adv = enable;

        app_cfg_store(&app_cfg_nv.offset_led_breath_mode_min_brightness_level, 1);
    }
}

static void app_dongle_phnoe_preempt_call_idle_handler()
{
    app_db.restore_dongle_recording = false;
    app_dongle_update_is_mic_enable(true);
    app_dongle_start_recording();
}

static void app_dongle_phone_preempt_call_active_handler()
{
    app_db.restore_dongle_recording = true;
    app_dongle_update_is_mic_enable(false);
    app_dongle_stop_recording();
}

void app_dongle_call_status_handler(T_APP_CALL_STATUS call_status, uint8_t idx)
{
    if (app_db.dongle_is_enable_mic && (call_status != APP_CALL_IDLE))
    {
        app_dongle_phone_preempt_call_active_handler();
    }
    else if (app_db.restore_dongle_recording && (call_status == APP_CALL_IDLE))
    {
        app_dongle_phnoe_preempt_call_idle_handler();
    }

    if ((app_db.br_link[idx].sco_handle != NULL) && (call_status == APP_CALL_IDLE))
    {
        if (timer_idx_call_idle_disc_sco == 0)
        {
            app_start_timer(&timer_idx_call_idle_disc_sco, "disc_sco",
                            app_dongle_timer_id, APP_TIMER_CALL_ILDE_DISC_SCO, idx, false,
                            2000);
        }
    }
}

T_APP_AUDIO_TONE_TYPE app_dongle_get_power_on_vp(void)
{
    T_APP_AUDIO_TONE_TYPE tone_type;

    if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G)
    {
        tone_type = TONE_POWER_ON_24G_MODE;
    }
    else if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_BT)
    {
        tone_type = TONE_POWER_ON_BT_MODE;
    }
    else
    {
        tone_type = TONE_POWER_ON_BT_24G_MODE;
    }

    return tone_type;
}

bool app_dongle_switch_rf_mode(T_APP_DONGLE_RF_MODE mode)
{
    bool ret = false;
    uint32_t bond_flag = 0;
    uint32_t plan_profs = 0;


#if TARGET_LE_AUDIO_GAMING == 0
    if (app_cfg_const.enable_multi_link == false &&
        mode == DONGLE_RF_MODE_24G_BT)
    {
        APP_PRINT_ERROR0("app_dongle_switch_rf_mode: multilink not enable!!");
        return false;
    }
#endif

    APP_PRINT_TRACE1("app_dongle_switch_rf_mode: mode %d", mode);

    handle_switch_rf_mode(mode);

    return true;
}

void app_dongle_check_mmi_action(uint8_t action)
{
    app_bt_policy_update_cpu_freq(BP_CPU_FREQ_EVENT_GAMING_MMI_TRIGGER);

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
    uint8_t active_a2dp_idx = app_a2dp_get_active_idx();

    if (action == MMI_AV_PLAY_PAUSE && !app_cfg_const.enable_24g_bt_audio_source_switch)
    {
        if ((app_link_get_lea_link_num() != 0) && (mtc_is_lea_cis_stream(NULL) == false) &&
            (app_db.br_link[active_a2dp_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_PAUSED ||
             app_db.br_link[active_a2dp_idx].avrcp_play_status == BT_AVRCP_PLAY_STATUS_STOPPED))
        {
            // BT and 2.4G both stream idle, 2.4G has high priority
            mtc_set_btmode(MULTI_PRO_BT_CIS);
        }
    }
#endif
}

void app_dongle_streaming_handle(bool streaming)
{
    static bool dongle_streaming = false;

    APP_PRINT_TRACE1("app_dongle_streaming_handle: %d", streaming);

    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));
    bt_param.not_check_addr_flag = true;

    if (streaming)
    {
        if (dongle_streaming == false)
        {
            app_bt_policy_update_cpu_freq(BP_CPU_FREQ_EVENT_GAMING_STREAMING_START);

            app_bt_policy_state_machine(EVENT_ENTER_GAMING_DONGLE_STREAMING, &bt_param);

            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_dongle_adv_control(true);
            }

            dongle_streaming = true;
        }
    }
    else
    {
        if (dongle_streaming)
        {
            app_bt_policy_update_cpu_freq(BP_CPU_FREQ_EVENT_GAMING_STREAMING_STOP);

            app_bt_policy_state_machine(EVENT_EXIT_GAMING_DONGLE_STREAMING, &bt_param);

            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_dongle_adv_control(false);
            }

            dongle_streaming = false;
        }
    }
}

#if F_APP_LEA_SUPPORT
bool app_dongle_lea_adv_start_check(void)
{
    bool ret = true;
    uint8_t reject_reason = 0;

    if (0)
    {}
#if (TARGET_LE_AUDIO_GAMING == 0)
    else if (app_dongle_is_streaming())
    {
        reject_reason = 1;
    }
#endif
#if F_APP_ALLOW_ONE_DONGLE_AND_ONE_PHONE_ONLY
    else if (app_dongle_get_connected_lea_phone_link())
    {
        reject_reason = 2;
    }
#endif
#if F_APP_GAMING_WIRED_MODE_HANDLE
    else if (app_dongle_get_wired_status())
    {
        reject_reason = 3;
    }
#endif

    if (reject_reason != 0)
    {
        ret = false;

        APP_PRINT_ERROR1("app_dongle_lea_adv_start_check: reason %d", reject_reason);
    }

    return ret;
}
#endif

void app_dongle_a2dp_start_handle(void)
{
    uint16_t latency = 0;
    T_APP_BR_LINK *p_link = app_dongle_get_connected_dongle_link();

    if (p_link && p_link->a2dp_track_handle)
    {
        latency = app_dongle_get_gaming_latency();
        app_dongle_set_gaming_latency(p_link->a2dp_track_handle, latency);

        bt_a2dp_stream_delay_report_req(p_link->bd_addr, latency);
    }
}

bool app_dongle_is_streaming(void)
{
    return dongle_status.streaming_to_peer ? true : false;
}

static void app_dongle_dual_mode_device_event_cback(uint32_t event, void *msg)
{
    switch (event)
    {
#if F_APP_LEA_SUPPORT
    case APP_DEVICE_IPC_EVT_LEA_CCP_CALL_STATUS:
        {
            uint16_t conn_handle = app_lea_ccp_get_active_conn_handle();
            T_APP_LE_LINK *p_le_link = app_link_find_le_link_by_conn_handle(conn_handle);
            T_APP_CALL_STATUS call_status = app_lea_ccp_get_call_status();

            if (p_le_link == NULL)
            {
                APP_PRINT_ERROR0("app_dongle_dual_mode_device_event_cback: Fail to get le link");
                return;
            }

            APP_PRINT_TRACE3("app_dongle_dual_mode_device_event_cback: device: %d, call_status: %d source: %d",
                             p_le_link->remote_device_type, call_status, app_cfg_nv.allowed_source);

            if ((call_status != APP_CALL_IDLE) &&
                (p_le_link->remote_device_type != DEVICE_TYPE_DONGLE))
            {
                //it meas that LEA phone call on-going.
                app_dongle_update_phone_stream_status(PHONE_EVENT_LEA_CALL_START);

#if TARGET_LEGACY_AUDIO_GAMING
                if (app_cfg_nv.allowed_source == ALLOWED_SOURCE_24G)
                {
                    if (app_db.dongle_is_enable_mic)
                    {
                        app_dongle_phone_preempt_call_active_handler();
                    }
                }
#endif
            }
            else if ((call_status == APP_CALL_IDLE) &&
                     (p_le_link->remote_device_type != DEVICE_TYPE_DONGLE))
            {
                //it meas that LEA pohne call end.
                app_dongle_update_phone_stream_status(PHONE_EVENT_LEA_CALL_STOP);

#if TARGET_LEGACY_AUDIO_GAMING
                if (app_cfg_nv.allowed_source == ALLOWED_SOURCE_24G)
                {
                    if (app_db.restore_dongle_recording)
                    {
                        app_dongle_phnoe_preempt_call_idle_handler();
                    }
                }
#endif
            }
        }
        break;
#endif

    default:
        break;
    }
}

bool app_dongle_send_fix_channel_data(uint8_t *addr, uint16_t cid, uint8_t *p_data, uint16_t length,
                                      bool flushable)
{
    bool ret = false;
    T_APP_BR_LINK *p_link = app_link_find_b2s_link(addr);
    bool restart_timer = false;
    static uint32_t last_start_time = 0;
    uint32_t curr_time = sys_timestamp_get();
    uint8_t cause = 0;

    if (p_link == NULL)
    {
        cause = 1;
        goto exit;
    }

    if (p_link->acl_link_in_sniffmode_flg)
    {
        app_sniff_mode_b2s_disable(p_link->bd_addr, SNIFF_DISABLE_MASK_FIX_CHAN_DATA);
    }

    if (timer_idx_send_fix_chan_exit_sniff == 0)
    {
        last_start_time = 0;
    }

    /* check restart timer to reduce sys loading (reduce the frequency to call app_start_timer)*/
    if (curr_time > last_start_time)
    {
        if ((curr_time - last_start_time) > FIX_CHANNEL_SNIFF_DETECT_TIME)
        {
            restart_timer = true;
        }
    }
    else
    {
        if ((0xffffffff - last_start_time + curr_time) > FIX_CHANNEL_SNIFF_DETECT_TIME)
        {
            restart_timer = true;
        }
    }

    if (restart_timer)
    {
        app_start_timer(&timer_idx_send_fix_chan_exit_sniff,
                        "send_fix_chan_exit_sniff",
                        app_dongle_timer_id, APP_TIMER_SEND_FIX_CHAN_EXIT_SNIFF, 0, false,
                        FIX_CHANNEL_DISALLOW_SNIFF_TIME);

        last_start_time = curr_time;
    }

    if (gap_br_send_fix_chann_data(addr, cid, p_data, length, flushable) != GAP_CAUSE_SUCCESS)
    {
        cause = 2;
        goto exit;
    }

exit:
    if (cause != 0)
    {
        APP_PRINT_ERROR1("app_dongle_send_fix_channel_data: failed cause -%d", cause);
    }
    else
    {
        ret = true;
    }

    return ret;
}

#if F_APP_GAMING_WIRED_MODE_HANDLE
bool app_dongle_get_wired_status(void)
{
    return disable_rf_due_to_wired_handle;
}

void app_dongle_wired_mode_handle(T_APP_DONGLE_WIRED_MODE_EVENT event)
{
    bool is_disconn_bt = false;
    uint8_t stop_adv_cause = 0;
    bool is_recover_bt = false;

    static bool is_usb_audio = false;
    static bool is_line_in = false;

    switch (event)
    {
#if F_APP_LINEIN_SUPPORT
    case EVENT_LINEIN_PLUG_IN:
        {
            is_line_in = true;
            is_disconn_bt = true;
            stop_adv_cause = APP_STOP_ADV_CAUSE_GAMING_LINEIN;
        }
        break;

    case EVENT_LINEIN_PLUG_OUT:
        {
            is_line_in = false;
        }
        break;
#endif

#if F_APP_USB_AUDIO_SUPPORT
    case EVENT_USB_AUDIO_START:
        {
            is_disconn_bt = true;
            is_usb_audio = true;
            stop_adv_cause = APP_STOP_ADV_CAUSE_GAMING_USB_AUDIO;
        }
        break;

    case EVENT_USB_AUDIO_STOP:
        {
            is_usb_audio = false;
        }
        break;
#endif

    default:
        break;
    }

    if (is_disconn_bt)
    {
        disable_rf_due_to_wired_handle = true;
        ble_ext_adv_mgr_disable_all(stop_adv_cause);
        gap_write_airplan_mode(1);

        if (app_bt_policy_get_state() != BP_STATE_IDLE)
        {
            app_db.bt_wait_shutdown_finish_to_startup = 0;
            app_bt_policy_shutdown();
        }
    }
    else
    {
        is_recover_bt = true;

#if F_APP_LINEIN_SUPPORT
        if (is_line_in)
        {
            is_recover_bt = false;
        }
#endif

#if F_APP_USB_AUDIO_SUPPORT
        if (is_usb_audio)
        {
            is_recover_bt = false;
        }
#endif

        if (is_recover_bt)
        {
            disable_rf_due_to_wired_handle = false;
            gap_write_airplan_mode(0);

            if (app_db.device_state == APP_DEVICE_STATE_ON)
            {
                if (app_bt_policy_get_state() == BP_STATE_IDLE)
                {
                    if (app_bt_policy_is_shutdown_step_state())
                    {
                        app_db.bt_wait_shutdown_finish_to_startup = 1;
                    }
                    else
                    {
                        app_db.bt_wait_shutdown_finish_to_startup = 0;
                        app_device_bt_policy_startup(true);
                    }
                }

                app_dongle_adv_start(false);
                app_dongle_handle_rtk_adv(true);
            }
        }
    }

    APP_PRINT_TRACE6("app_dongle_wired_mode_handle: event %d, line_in %d, usb_audio %d, disconn_bt %d, recover_bt %d, delay_start %d",
                     event, is_line_in, is_usb_audio, is_disconn_bt, is_recover_bt,
                     app_db.bt_wait_shutdown_finish_to_startup);
}
#endif

void app_dongle_dual_mode_init(void)
{

#if F_APP_SLIDE_SWITCH_SUPPORT
    update_mode_according_to_slide_switch_status();
#endif

    dongle_state = DONGLE_STATE_STANDBY;

#if F_APP_GAMING_CHAT_MIXING_SUPPORT
    headset_status.audio_mixing_support = true;
#endif

    app_ipc_subscribe(APP_DEVICE_IPC_TOPIC, app_dongle_dual_mode_device_event_cback);

    bt_mgr_cback_register(app_dongle_bt_cback);

#if F_APP_ERWS_SUPPORT
    app_relay_cback_register(app_dongle_relay_cback, app_dongle_parse_cback,
                             APP_MODULE_TYPE_DONGLE_DUAL_MODE, APP_REMOTE_MSG_DONGLE_MAX_MSG_NUM);
#endif

    app_timer_reg_cb(app_dongle_timer_cback, &app_dongle_timer_id);

#if F_APP_LEA_SUPPORT
    app_donlge_enable_cis_higher_priority(true);
#endif
}
#endif
