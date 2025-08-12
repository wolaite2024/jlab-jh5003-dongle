/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if (F_APP_ERWS_SUPPORT == 1)
#include <string.h>
#include "gap_br.h"
#include "sysm.h"
#include "trace.h"
#include "wdg.h"
#include "remote.h"
#include "audio_track.h"
#include "app_adp_cmd.h"
#include "app_timer.h"
#include "app_audio_policy.h"
#include "app_loc_mgr.h"
#include "app_ipc.h"
#include "app_roleswap_control.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_roleswap.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_multilink.h"
#include "app_ota.h"
#include "app_anc.h"
#include "app_ble_gap.h"
#include "app_mmi.h"
#include "app_ble_device.h"
#include "app_ble_common_adv.h"
#include "app_qol.h"
#include "app_key_process.h"
#include "app_vendor.h"

#if F_APP_LEA_SUPPORT
#include "app_lea_adv.h"
#endif

#if (F_APP_AIRPLANE_SUPPORT == 1)
#include "app_airplane.h"
#endif

#if F_APP_DURIAN_SUPPORT
#include "app_durian.h"
#endif

#if F_APP_CFU_FEATURE_SUPPORT
#include "app_common_cfu.h"
#endif

#include "app_sensor.h"

#if F_APP_TUYA_SUPPORT
#include "rtk_tuya_ble_ota.h"
#endif

extern bool le_get_conn_local_addr(uint16_t conn_handle, uint8_t *bd_addr, uint8_t *bd_type);

typedef struct
{
    uint8_t case_closed_power_off : 1;
    uint8_t in_case_power_off : 1;
    uint8_t single_power_off : 1; /* single power off by key, cmd set .... */
    uint8_t wait_profile_connected : 1;
    uint8_t wait_a2dp_stream_chann : 1;
    uint8_t mmi_trigger_roleswap : 1;
    uint8_t rsv : 2;
} T_ROLESWAP_SYS_STATUS;

typedef struct
{
    uint8_t disc_ble : 1;
    uint8_t disc_inactive_link : 1; /* multilink */
    uint8_t disc_inactive_link_ing : 1;
    uint8_t roleswap_is_going_to_do : 1;
    uint8_t rsv : 4;
} T_ROLESWAP_TRIGGER_STATUS;

typedef struct
{
    uint8_t local_lea_addr[6];
    uint8_t remote_lea_addr[6];
} T_ROLESWAP_LEA_ADDR_INFO;

typedef enum
{
    ROLESWAP_REJECT_REASON_NONE = 0,                    //0x00
    ROLESWAP_REJECT_REASON_BASIC_CHECK,                 //0x01
    ROLESWAP_REJECT_REASON_ONGOING,                     //0x02
    ROLESWAP_REJECT_REASON_BLE_EXIST,                   //0x03
    ROLESWAP_REJECT_REASON_ACL_ROLE_SLAVE,              //0x04
    ROLESWAP_REJECT_REASON_OTA,                         //0x05
    ROLESWAP_REJECT_REASON_TEAMS_CFU,                   //0x06
    ROLESWAP_REJECT_REASON_ANC_HANDLING,                //0x07
    ROLESWAP_REJECT_REASON_SNIFFING_STATE,              //0x08
    ROLESWAP_REJECT_REASON_PROFILE_NOT_CONN,            //0x09
    ROLESWAP_REJECT_REASON_PROFILE_CONNECTING,          //0x0A
    ROLESWAP_REJECT_REASON_REMOTE_IN_BOX,               //0x0B
    ROLESWAP_REJECT_REASON_CALL_REMOTE_IN_BOX,          //0x0C
    ROLESWAP_REJECT_REASON_CALL_REMOTE_OUT_EAR,         //0x0D
    ROLESWAP_REJECT_REASON_MEDIA_BUFFER_NOT_ENOUGH,     //0x0E
    ROLESWAP_REJECT_REASON_MULTILINK_EXIST,             //0x0F
    ROLESWAP_REJECT_REASON_DISC_BLE_OR_INACTIVE_LINK,   //0x10
    ROLESWAP_REJECT_REASON_DURIAN_FIX_MIC_VIOLATION,    //0x11
    ROLESWAP_REJECT_REASON_CHECK_ABS_VOL,               //0x12
    ROLESWAP_REJECT_REASON_A2DP_STREAM_CHANN_CONNECTING,//0x13
    ROLESWAP_REJECT_REASON_MEMS_START,                  //0x14
    ROLESWAP_REJECT_REASON_AIRPLANE_MODE,               //0x15
    ROLESWAP_REJECT_REASON_CALL_COMING,                 //0x16
    ROLESWAP_REJECT_REASON_STOP_LINKBACK_ING,           //0x17
    ROLESWAP_REJECT_REASON_LOCAL_BOOM_MIC_IN,           //0x18
    ROLESWAP_REJECT_REASON_TUYA_OTA,                    //0x19
    ROLESWAP_REJECT_REASON_HTPOLL_EXECING,              //0x20
} T_APP_ROLESWAP_REJECT_REASON;

typedef enum
{
    APP_TIMER_ROLESWAP_RETRY,
    APP_TIMER_ROLESWAP_HANDLE_EXCEPTION,
    APP_TIMER_ROLESWAP_WAIT_PROFILE_CONNECTED,
    APP_TIMER_ROLESWAP_POWER_OFF_PROTECT,
    APP_TIMER_ROLESWAP_STOP_LINKBACK,
} T_APP_ROLESWAP_CTRL_TIMER;

typedef enum
{
    APP_REMOTE_MSG_ROLESWAP_CTRL_TERMINATED,
    APP_REMOTE_MSG_ROLESWAP_CTRL_RESET_ORI_BUD_ROLE,
    APP_REMOTE_MSG_ROLESWAP_CTRL_SYNC_LEA_ADDR_INFO,
    APP_REMOTE_MSG_ROLESWAP_CTRL_TOTAL,
} T_ROLESWAP_CTRL_REMOTE_MSG;

typedef enum
{
    ROLESWAP_TRIGGER_LEVEL_HIGH,    /* high priority: need to power off after roleswap */
    ROLESWAP_TRIGGER_LEVEL_MEDIUM,  /* medium priority: to make the mic correct in call or lea 2nd roleswap */
    ROLESWAP_TRIGGER_LEVEL_LOW,     /* low priority: rssi, bat roleswap etc.... */
} T_APP_ROLESWAP_TRIGGER_LEVEL;

typedef enum
{
    /* highest priority */
    ROLESWAP_SINGLE_BUD_POWER_OFF = 0,          //0x00
    ROLESWAP_CLOSE_CASE_POWER_OFF,              //0x01
    ROLESWAP_IN_NON_SMART_CASE,                 //0x02
    ROLESWAP_IN_CASE_TIMEOUT_POWER_OFF,         //0x03
    ROLESWAP_DATA_CAPTURE,                      //0x04
    ROLESWAP_DURIAN_FIX_MIC,                    //0x05
    ROLESWAP_REMOTE_BOOM_MIC_IN,                //0x06
    ROLESWAP_CALL_IN_SMART_CASE_WHEN_CALL,      //0x07
    ROLESWAP_OUT_EAR_WHEN_CALL,                 //0x08
    ROLESWAP_LEA_2ND_ROLESWAP,                  //0x09
    ROLESWAP_MMI_TRIGGER,                       //0x0A
    ROLESWAP_BATTERY_LEVEL,                     //0x0B
    ROLESWAP_RSSI,                              //0x0C

    /* Note: this must be in the end */
    ROLESWAP_NONE,
} T_APP_ROLESWAP_LEVEL;

typedef struct
{
    bool (*trigger_check)(T_APP_ROLESWAP_CTRL_EVENT event);
    bool (*violation_check)(void);
    T_APP_ROLESWAP_TRIGGER_LEVEL level;
} T_ROLESWAP_CHECK;

static T_ROLESWAP_SYS_STATUS roleswap_sys_status;
static T_ROLESWAP_TRIGGER_STATUS roleswap_trigger_status;
static T_APP_ROLESWAP_STATUS roleswap_status;
static uint8_t app_roleswap_ctrl_timer_id = 0;
static uint8_t timer_idx_roleswap_retry = 0;
static uint8_t timer_idx_roleswap_handle_exception = 0;
static uint8_t timer_idx_roleswap_wait_profile_connected = 0;
static uint8_t timer_idx_roleswap_power_off_protect = 0;
static uint8_t timer_idx_roleswap_stop_linkback = 0;

#if F_APP_LEA_SUPPORT
static T_ROLESWAP_LEA_ADDR_INFO lea_addr_info;
#endif

static void app_roleswap_ctrl_set_status(T_APP_ROLESWAP_STATUS status, uint16_t line)
{
    APP_PRINT_TRACE2("app_roleswap_ctrl_set_status: status %d line %d", status, line);

    if (status == APP_ROLESWAP_STATUS_BUSY &&
        timer_idx_roleswap_handle_exception == 0)
    {
        app_start_timer(&timer_idx_roleswap_handle_exception, "roleswap_handle_exception",
                        app_roleswap_ctrl_timer_id, APP_TIMER_ROLESWAP_HANDLE_EXCEPTION, 0, false,
                        5000);
    }
    else if (status == APP_ROLESWAP_STATUS_IDLE)
    {
        app_stop_timer(&timer_idx_roleswap_handle_exception);
    }

    roleswap_status = status;
    app_bt_policy_update_cpu_freq(BP_CPU_FREQ_EVENT_ROLESWAP);

}

#if F_APP_ERWS_SUPPORT
static uint16_t app_roleswap_ctrl_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    bool skip = true;
    uint8_t *msg_ptr = NULL;
    uint16_t payload_len = 0;

    switch (msg_type)
    {
#if F_APP_LEA_SUPPORT
    case APP_REMOTE_MSG_ROLESWAP_CTRL_SYNC_LEA_ADDR_INFO:
        {
            payload_len = sizeof(lea_addr_info.local_lea_addr);
            msg_ptr = lea_addr_info.local_lea_addr;

            skip = false;
        }
        break;
#endif

    default:
        break;
    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_ROLESWAP_CTRL, payload_len, msg_ptr, skip,
                              total);
}

static void app_roleswap_ctrl_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                          T_REMOTE_RELAY_STATUS status)
{
    bool handle = false;

    switch (msg_type)
    {
    case APP_REMOTE_MSG_ROLESWAP_CTRL_TERMINATED:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_roleswap_ctrl_set_status(APP_ROLESWAP_STATUS_IDLE, __LINE__);
            }

            handle = true;
        }
        break;

#if F_APP_LEA_SUPPORT
    case APP_REMOTE_MSG_ROLESWAP_CTRL_SYNC_LEA_ADDR_INFO:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                memcpy(lea_addr_info.remote_lea_addr, buf, 6);

                APP_PRINT_TRACE2("app_roleswap_ctrl_parse_cback: remote_lea_addr: %s bud_local_addr %s",
                                 TRACE_BDADDR(lea_addr_info.remote_lea_addr), TRACE_BDADDR(app_cfg_nv.bud_local_addr));

                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_LEA_SYNC_ADDR_INFO);
            }
        }
        break;
#endif

    default:
        break;
    }

    if (handle)
    {
        APP_PRINT_TRACE2("app_roleswap_ctrl_parse_cback: msg_type 0x%02X status 0x%02X", msg_type, status);
    }

}
#endif

T_APP_ROLESWAP_STATUS app_roleswap_ctrl_get_status(void)
{
    return roleswap_status;
}

static void app_roleswap_ctrl_exec_roleswap(void)
{
    APP_PRINT_TRACE0("app_roleswap_ctrl_exec_roleswap");

    if (app_bt_sniffing_roleswap(false))
    {
        if (app_db.connected_num_before_roleswap > app_link_get_b2s_link_num())
        {
            app_bt_policy_stop_reconnect_timer();
        }

        app_roleswap_ctrl_set_status(APP_ROLESWAP_STATUS_BUSY, __LINE__);
    }
    else
    {
        app_start_timer(&timer_idx_roleswap_retry, "roleswap_retry",
                        app_roleswap_ctrl_timer_id, APP_TIMER_ROLESWAP_RETRY, 0, false,
                        300);
    }
}

static void app_roleswap_ctrl_reconnect_inactive_link(void)
{
    //If no profile connected when disconnect inactive link, connected tone is need.
    app_db.disallow_connected_tone_after_inactive_connected = (app_db.connected_tone_need == false) ?
                                                              true : false;

    app_multi_reconnect_inactive_link();
    app_audio_set_connected_tone_need(false);
}


static void app_roleswap_ctrl_cancel_restore_link(void)
{
    APP_PRINT_TRACE2("app_roleswap_ctrl_cancel_restore_link: restore_ble %d restore_multilink %d",
                     roleswap_trigger_status.disc_ble, roleswap_trigger_status.disc_inactive_link);

    /* restore ble or multilink if need */
    if (roleswap_trigger_status.disc_ble)
    {
        roleswap_trigger_status.disc_ble = 0;

        T_BT_EVENT_PARAM_REMOTE_ROLESWAP_STATUS remote_roleswap_status;

        /* force restore ble */
        remote_roleswap_status.device_role = REMOTE_SESSION_ROLE_PRIMARY;
        remote_roleswap_status.status = BT_ROLESWAP_STATUS_FAIL;
        app_ble_device_handle_role_swap(&remote_roleswap_status);

        /* ble_common_adv_after_roleswap is set false in app_ble_device_handle_role_swap */
        app_ble_common_adv_sync_start_flag();
    }

    if (roleswap_trigger_status.disc_inactive_link)
    {
        roleswap_trigger_status.disc_inactive_link = 0;

        if (app_db.connected_num_before_roleswap > app_link_get_b2s_link_num())
        {
            app_roleswap_ctrl_reconnect_inactive_link();
        }
        else if (app_db.connected_num_before_roleswap == app_link_get_b2s_link_num())
        {
            roleswap_trigger_status.disc_inactive_link_ing = 1;
        }
    }
}

#if (F_APP_QOL_MONITOR_SUPPORT == 1)
static bool app_roleswap_ctrl_media_buffer_enough(void)
{
    T_APP_BR_LINK *p_link = NULL;
    bool ret = true;

    p_link = &app_db.br_link[app_a2dp_get_active_idx()];

    if (p_link && p_link->a2dp_track_handle)
    {
        audio_track_buffer_level_get(p_link->a2dp_track_handle, &app_db.buffer_level_local);

        if ((app_db.buffer_level_remote < BUFFER_LEVEL_MINIMUM_THRESHOLD_TO_ENABLE_RSSI_ROLESWAP) ||
            (app_db.buffer_level_local < BUFFER_LEVEL_MINIMUM_THRESHOLD_TO_ENABLE_RSSI_ROLESWAP))
        {
            APP_PRINT_TRACE2("app_roleswap_ctrl_media_buffer_enough: local %d remote %d",
                             app_db.buffer_level_local, app_db.buffer_level_remote);

            ret = false;
        }
    }

    return ret;
}
#endif

/* trigger poweroff after roleswap */
static bool app_roleswap_ctrl_poweroff_triggered(void)
{
    bool ret = false;

    if (app_db.device_state == APP_DEVICE_STATE_ON)
    {
        if (roleswap_sys_status.in_case_power_off || roleswap_sys_status.single_power_off ||
            roleswap_sys_status.case_closed_power_off)
        {
            ret = true;
        }
    }

    return ret;
}

static void app_roleswap_ctrl_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = false;

    switch (event_type)
    {
    case BT_EVENT_HFP_CONN_CMPL:
    case BT_EVENT_A2DP_CONN_CMPL:
    case BT_EVENT_AVRCP_CONN_CMPL:
    /* disallow roleswap immediately before seamless join finished */
    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            handle = true;

            if (app_link_get_b2s_link_num() != 0) //No need to check profile if no ACL connected
            {
                roleswap_sys_status.wait_profile_connected = true;
                app_start_timer(&timer_idx_roleswap_wait_profile_connected, "roleswap_wait_profile_connected",
                                app_roleswap_ctrl_timer_id, APP_TIMER_ROLESWAP_WAIT_PROFILE_CONNECTED, 0, false,
                                3500);
            }

            if (event_type == BT_EVENT_A2DP_CONN_CMPL)
            {
                roleswap_sys_status.wait_a2dp_stream_chann = true;
            }

            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
            {
                app_relay_async_single(APP_MODULE_TYPE_ROLESWAP_CTRL,
                                       APP_REMOTE_MSG_ROLESWAP_CTRL_SYNC_LEA_ADDR_INFO);
            }
        }
        break;

    case BT_EVENT_REMOTE_DISCONN_CMPL:
        {
            handle = true;
            app_roleswap_ctrl_set_status(APP_ROLESWAP_STATUS_IDLE, __LINE__);

            if (app_roleswap_ctrl_poweroff_triggered())
            {
                /* b2b disconnected during roleswap triggered; poweroff directly */
                app_mmi_handle_action(MMI_DEV_POWER_OFF);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            handle = true;
            bool addr_match = !memcmp(param->acl_conn_disconn.bd_addr, app_db.resume_addr, 6);

            APP_PRINT_TRACE5("BT_EVENT_ACL_CONN_DISCONN: %d cause 0x%04X %d (%s %s)", addr_match,
                             param->acl_conn_disconn.cause, roleswap_trigger_status.disc_inactive_link,
                             TRACE_BDADDR(param->acl_conn_disconn.bd_addr), TRACE_BDADDR(app_db.resume_addr));

            if (roleswap_trigger_status.disc_inactive_link)
            {
                if (addr_match &&
                    (param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)  ||
                     param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE) ||
                     param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)  ||
                     param->acl_conn_disconn.cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)))
                {
                    app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_ACL_DISC);
                }
            }
            else if (roleswap_trigger_status.disc_inactive_link_ing)
            {
                roleswap_trigger_status.disc_inactive_link_ing = 0;

                app_roleswap_ctrl_reconnect_inactive_link();
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_MASTER:
        {
            handle = true;

            if (app_link_check_b2b_link(param->acl_role_master.bd_addr))
            {
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_B2B_ACL_ROLE_MASTER);
            }
        }
        break;

    case BT_EVENT_REMOTE_ROLESWAP_STATUS:
        {
            handle = true;

            T_BT_ROLESWAP_STATUS roleswap_status = param->remote_roleswap_status.status;

            APP_PRINT_TRACE3("app_roleswap_ctrl_bt_cback: roleswap status 0x%02X, last_bud_loc_event 0x%02X, bud_role %d",
                             roleswap_status, app_db.last_bud_loc_event, app_cfg_nv.bud_role);

            if (roleswap_status == BT_ROLESWAP_STATUS_SUCCESS)
            {
#if F_APP_LEA_SUPPORT
                app_roleswap_check_lea_addr_match(ADDR_CHECK_ROLESWAP_CMPL, NULL);
#endif

                app_roleswap_ctrl_set_status(APP_ROLESWAP_STATUS_IDLE, __LINE__);
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_ROLESWAP_SUCCESS);

                /*check whether to transfer call when roleswap ends*/
#if F_APP_DURIAN_SUPPORT
                app_durian_mmi_call_transfer_check();
#else
                if (app_audio_call_transfer_check())
                {
                    app_mmi_handle_action(MMI_HF_TRANSFER_CALL);
                }
#endif
            }
            else if (roleswap_status == BT_ROLESWAP_STATUS_TERMINATED)
            {
                app_roleswap_ctrl_set_status(APP_ROLESWAP_STATUS_IDLE, __LINE__);
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_ROLESWAP_TERMINATED);

                uint8_t null_data = 0;
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_ROLESWAP_CTRL,
                                                    APP_REMOTE_MSG_ROLESWAP_CTRL_TERMINATED, &null_data, sizeof(null_data));
            }
            else if (roleswap_status == BT_ROLESWAP_STATUS_START_REQ)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    if (app_db.device_state == APP_DEVICE_STATE_ON)
                    {
                        app_roleswap_ctrl_set_status(APP_ROLESWAP_STATUS_BUSY, __LINE__);

                        remote_roleswap_cfm(true, 0);
                    }
                    else
                    {
                        remote_roleswap_cfm(false, 0);
                    }
                }
            }
            else if (roleswap_status == BT_ROLESWAP_STATUS_START_RSP)
            {
                /* role swap is rejected by peer ; direct power off */
                if (!param->remote_roleswap_status.u.start_rsp.accept)
                {
                    app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_REJCT_BY_PEER);
                }
            }
            else if (roleswap_status == BT_ROLESWAP_STATUS_FAIL)
            {
                uint16_t fail_cause = param->remote_roleswap_status.cause;

                if ((fail_cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                    (fail_cause == (HCI_ERR | HCI_ERR_CONTROLLER_BUSY)))
                {
                    app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_ROLESWAP_FAILED_RETRY);
                }
            }

            if (app_db.pending_mmi_action_in_roleswap)
            {
                /* roleswap fail will reboot */
                if (roleswap_status == BT_ROLESWAP_STATUS_SUCCESS ||
                    roleswap_status == BT_ROLESWAP_STATUS_TERMINATED)
                {
                    app_key_execute_action(app_db.pending_mmi_action_in_roleswap);
                    app_db.pending_mmi_action_in_roleswap = 0;
                }
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN_FAIL:
    case BT_EVENT_A2DP_DISCONN_CMPL:
    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            handle = true;

            roleswap_sys_status.wait_a2dp_stream_chann = false;
        }
        break;

    default:
        break;
    }

    if (handle)
    {
        APP_PRINT_TRACE1("app_roleswap_ctrl_bt_cback: event 0x%04X", event_type);
    }
}

static void app_roleswap_ctrl_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO2("app_roleswap_ctrl_timeout_cb: timer_evt %d param 0x%02X", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_ROLESWAP_RETRY:
        {
            app_stop_timer(&timer_idx_roleswap_retry);

            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_RETRY);
        }
        break;

    case APP_TIMER_ROLESWAP_HANDLE_EXCEPTION:
        {
            app_stop_timer(&timer_idx_roleswap_handle_exception);

            chip_reset(RESET_ALL);
        }
        break;

    case APP_TIMER_ROLESWAP_WAIT_PROFILE_CONNECTED:
        {
            app_stop_timer(&timer_idx_roleswap_wait_profile_connected);
            roleswap_sys_status.wait_profile_connected = false;

            app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_ALL_PROFILE_CONNECTED);
        }
        break;

    case APP_TIMER_ROLESWAP_POWER_OFF_PROTECT:
        {
            app_stop_timer(&timer_idx_roleswap_power_off_protect);
            app_mmi_handle_action(MMI_DEV_POWER_OFF);
        }
        break;

    case APP_TIMER_ROLESWAP_STOP_LINKBACK:
        {
            app_stop_timer(&timer_idx_roleswap_stop_linkback);

            app_roleswap_ctrl_exec_roleswap();
        }
        break;

    default:
        break;
    }
}

/* basic condition to do roleswap */
static bool app_roleswap_ctrl_basic_check(void)
{
    bool ret = true;

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED ||
        app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY ||
        app_db.device_state != APP_DEVICE_STATE_ON)
    {
        ret = false;
    }

    return ret;
}

#if (F_APP_POWER_TEST == 0)
static bool app_roleswap_ctrl_battery_check(void)
{
    bool ret = false;

    if ((app_db.remote_batt_level > BUD_BATT_BOTH_ROLESWAP_THRESHOLD) &&
        (app_db.local_batt_level < BUD_BATT_BOTH_ROLESWAP_THRESHOLD))
    {
        ret = true;
    }

    return ret;
}

#if (F_APP_QOL_MONITOR_SUPPORT == 1)
static bool app_roleswap_ctrl_rssi_check(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    T_APP_BR_LINK *p_link = NULL;

    p_link = &app_db.br_link[app_a2dp_get_active_idx()];

    if (p_link && p_link->a2dp_track_handle)
    {
        audio_track_buffer_level_get(p_link->a2dp_track_handle, &app_db.buffer_level_local);

        if ((app_db.rssi_local + app_cfg_const.roleswap_rssi_threshold < app_db.rssi_remote) &&
            (app_db.rssi_remote >= RSSI_MINIMUM_THRESHOLD_TO_ENABLE_RSSI_ROLESWAP) &&
            (app_db.rssi_local <= RSSI_MAXIMUM_THRESHOLD_TO_ENABLE_RSSI_ROLESWAP) &&
            (app_db.rssi_local >= RSSI_MINIMUM_THRESHOLD_TO_ENABLE_RSSI_ROLESWAP))
        {
            if (event == APP_ROLESWAP_CTRL_EVENT_DECODE_EMPTY)
            {
                ret = true;
            }
            else
            {
                /* check buffer level to prevent carton */
                if ((app_db.buffer_level_remote >= BUFFER_LEVEL_MINIMUM_THRESHOLD_TO_ENABLE_RSSI_ROLESWAP) &&
                    (app_db.buffer_level_local >= BUFFER_LEVEL_MINIMUM_THRESHOLD_TO_ENABLE_RSSI_ROLESWAP))
                {
                    ret = true;
                }
            }
        }
    }

    if (event == APP_ROLESWAP_CTRL_EVENT_BUFFER_LEVEL_CHANGED)
    {
        APP_PRINT_TRACE6("app_roleswap_ctrl_rssi_check: ret %d rssi local %d remote %d threshold %d buffer local %d remote %d",
                         ret, app_db.rssi_local, app_db.rssi_remote, app_cfg_const.roleswap_rssi_threshold,
                         app_db.buffer_level_local, app_db.buffer_level_remote);
    }

    return ret;
}
#endif
#endif

static bool app_roleswap_ctrl_profile_check(void)
{
    uint32_t connected_profile = app_link_conn_profiles();
    bool ret = false;

    if (/* at least one profile connected (a2dp, hfp, spp) with legacy link*/
        ((app_db.b2s_connected_num != 0) &&
         ((connected_profile & A2DP_PROFILE_MASK) || (connected_profile & HFP_PROFILE_MASK) ||
          (connected_profile & SPP_PROFILE_MASK)))
#if F_APP_LEA_SUPPORT
        || ((app_db.b2s_connected_num == 0) && (app_link_get_lea_link_num() != 0))
#endif
    )
    {
        ret = true;
    }

    return ret;
}

static bool app_roleswap_ctrl_call_triggered(void)
{
    bool ret = false;

    if (app_bt_policy_get_call_status() != APP_CALL_IDLE)
    {
        ret = true;
    }
#if F_APP_DURIAN_SUPPORT
    /* for voice assistant */
    else if (app_durian_avp_get_opus_status() == AVP_VIOCE_RECOGNITION_ENCODE_START)
    {
        ret = true;
    }
#endif

    return ret;
}

#if F_APP_DURIAN_SUPPORT
static T_APP_ROLESWAP_REJECT_REASON app_roleswap_ctrl_durian_fix_mic_violation(void)
{
    T_APP_ROLESWAP_REJECT_REASON ret = ROLESWAP_REJECT_REASON_NONE;

    if (app_roleswap_ctrl_call_triggered())
    {
        if (app_durian_audio_get_mic_set() == BT_AVP_MIC_ALLWAYS_LEFT)
        {
            if (app_cfg_const.bud_side == DEVICE_ROLE_LEFT)
            {
                ret = ROLESWAP_REJECT_REASON_DURIAN_FIX_MIC_VIOLATION;
            }
        }
        else if (app_durian_audio_get_mic_set() == BT_AVP_MIC_ALLWAYS_RIGHT)
        {
            if (app_cfg_const.bud_side == DEVICE_ROLE_RIGHT)
            {
                ret = ROLESWAP_REJECT_REASON_DURIAN_FIX_MIC_VIOLATION;
            }
        }
    }

    return ret;
}

static bool app_roleswap_ctrl_durian_mic_check(void)
{
    bool ret = false;

    if (app_roleswap_ctrl_call_triggered())
    {
        if (app_durian_audio_get_mic_set() == BT_AVP_MIC_ALLWAYS_LEFT)
        {
            if (app_cfg_const.bud_side == DEVICE_ROLE_RIGHT)
            {
                ret = true;
            }
        }
        else if (app_durian_audio_get_mic_set() == BT_AVP_MIC_ALLWAYS_RIGHT)
        {
            if (app_cfg_const.bud_side == DEVICE_ROLE_LEFT)
            {
                ret = true;
            }
        }
    }

    return ret;
}
#endif

/* in non smart box to power off*/
static bool app_roleswap_ctrl_in_non_smart_box(void)
{
    bool ret = false;

    if (app_cfg_const.enable_rtk_charging_box == false && app_cfg_const.enable_inbox_power_off &&
        app_db.local_loc == BUD_LOC_IN_CASE && app_db.remote_loc != BUD_LOC_IN_CASE)
    {
        ret = true;
    }

    return ret;
}

/* in smart box to power off*/
static bool app_roleswap_ctrl_in_smart_box_timeout(void)
{
    bool ret = false;

    if (
#if F_APP_ADP_CMD_SUPPORT
        (app_adp_cmd_in_case_timeout() == true) &&
#endif
        (app_db.remote_loc != BUD_LOC_IN_CASE))
    {
        ret = true;
    }

    return ret;
}

/* in smart box when call */
static bool app_roleswap_ctrl_in_box_when_call(void)
{
    bool ret = false;

    if (app_cfg_const.enable_rtk_charging_box == true &&
        app_roleswap_ctrl_call_triggered() &&
        app_db.local_loc == BUD_LOC_IN_CASE && app_db.remote_loc != BUD_LOC_IN_CASE)
    {
        ret = true;
    }

    return ret;
}

/* out ear when call */
static bool app_roleswap_ctrl_out_ear_when_call(void)
{
    bool ret = false;

    if (LIGHT_SENSOR_ENABLED &&
        app_roleswap_ctrl_call_triggered() &&
        app_db.local_loc != BUD_LOC_IN_EAR && app_db.remote_loc == BUD_LOC_IN_EAR)
    {
        ret = true;
    }

    return ret;
}

/* non smart box remote in box */
static bool app_roleswap_ctrl_remote_in_non_smart_box(void)
{
    bool ret = false;

    if (app_cfg_const.enable_rtk_charging_box == false &&
        app_db.local_loc != BUD_LOC_IN_CASE && app_db.remote_loc == BUD_LOC_IN_CASE)
    {
        ret = true;
    }

    return ret;
}

/* smart box remote in box when call */
static bool app_roleswap_ctrl_call_remote_in_smart_box(void)
{
    bool ret = false;

    if (app_cfg_const.enable_rtk_charging_box == true &&
        app_roleswap_ctrl_call_triggered() &&
        app_db.local_loc != BUD_LOC_IN_CASE && app_db.remote_loc == BUD_LOC_IN_CASE)
    {
        ret = true;
    }

    return ret;
}

/* remote out ear when call */
static bool app_roleswap_ctrl_call_remote_out_ear(void)
{
    bool ret = false;

    if (LIGHT_SENSOR_ENABLED &&
        app_roleswap_ctrl_call_triggered() &&
        app_db.local_loc == BUD_LOC_IN_EAR && app_db.remote_loc != BUD_LOC_IN_EAR)
    {
        ret = true;
    }

    return ret;
}

static T_APP_ROLESWAP_REJECT_REASON app_roleswap_ctrl_1st_stage_reject_check(void)
{
    T_APP_ROLESWAP_REJECT_REASON reject_reason = ROLESWAP_REJECT_REASON_NONE;
    T_APP_BR_LINK *p_b2b_link = app_link_find_br_link(app_cfg_nv.bud_peer_addr);

#if F_APP_LEA_SUPPORT
    uint8_t le_app_link_num = app_link_get_le_link_num() - app_link_get_lea_link_num();
#else
    uint8_t le_app_link_num = app_link_get_le_link_num();
#endif

    if (app_roleswap_ctrl_basic_check() == false)
    {
        reject_reason = ROLESWAP_REJECT_REASON_BASIC_CHECK;
        goto exit;
    }

    if (roleswap_sys_status.wait_profile_connected)
    {
        reject_reason = ROLESWAP_REJECT_REASON_PROFILE_CONNECTING;
        goto exit;
    }

    if (app_roleswap_ctrl_profile_check() == false)
    {
        reject_reason = ROLESWAP_REJECT_REASON_PROFILE_NOT_CONN;
        goto exit;
    }

    if (roleswap_sys_status.wait_a2dp_stream_chann)
    {
        reject_reason = ROLESWAP_REJECT_REASON_A2DP_STREAM_CHANN_CONNECTING;
        goto exit;
    }

    if (roleswap_status != APP_ROLESWAP_STATUS_IDLE)
    {
        reject_reason = ROLESWAP_REJECT_REASON_ONGOING;
        goto exit;
    }

    if ((p_b2b_link->acl_link_role == BT_LINK_ROLE_SLAVE)
#if F_APP_LEA_SUPPORT
        && (app_link_get_b2s_link_num() != 0)
#endif
       )
    {
        reject_reason = ROLESWAP_REJECT_REASON_ACL_ROLE_SLAVE;
        goto exit;
    }

    if (app_link_get_b2s_link_num() != 0)
    {
        if (app_bt_sniffing_roleswap_check() == false)
        {
            reject_reason = ROLESWAP_REJECT_REASON_SNIFFING_STATE;
            goto exit;
        }
    }

#if F_APP_B2B_HTPOLL_CONTROL
    if (app_vendor_htpoll_execing())
    {
        reject_reason = ROLESWAP_REJECT_REASON_HTPOLL_EXECING;
        goto exit;
    }
#endif

    if (le_app_link_num > 0)
    {
        reject_reason = ROLESWAP_REJECT_REASON_BLE_EXIST;
        goto exit;
    }

    if (app_link_get_b2s_link_num() > 1)
    {
        reject_reason = ROLESWAP_REJECT_REASON_MULTILINK_EXIST;
        goto exit;
    }

    if (app_avrcp_abs_vol_check_timer_exist())
    {
        reject_reason = ROLESWAP_REJECT_REASON_CHECK_ABS_VOL;
        goto exit;
    }

exit:
    return reject_reason;
}

static T_APP_ROLESWAP_REJECT_REASON app_roleswap_ctrl_2nd_stage_reject_check(
    T_APP_ROLESWAP_CTRL_EVENT event, T_APP_ROLESWAP_LEVEL roleswap_level)
{
    T_APP_ROLESWAP_REJECT_REASON reject_reason = ROLESWAP_REJECT_REASON_NONE;
#if F_APP_ANC_SUPPORT
    T_ANC_FEATURE_MAP feature_map;
    feature_map.d32 = anc_tool_get_feature_map();
#endif

#if F_APP_LEA_SUPPORT
    uint8_t le_app_link_num = app_link_get_le_link_num() - app_link_get_lea_link_num();
#else
    uint8_t le_app_link_num = app_link_get_le_link_num();
#endif


#if (F_APP_QOL_MONITOR_SUPPORT == 1)
    bool media_buffer_enough = app_roleswap_ctrl_media_buffer_enough();
#else
    bool media_buffer_enough = true;
#endif

    if (app_ota_dfu_is_busy())
    {
        reject_reason = ROLESWAP_REJECT_REASON_OTA;
    }
#if F_APP_TUYA_SUPPORT
    else if (tuya_ota_is_busy())
    {
        reject_reason = ROLESWAP_REJECT_REASON_TUYA_OTA;
    }
#endif
    else if ((app_hfp_get_call_status() == APP_CALL_OUTGOING) ||
             (app_hfp_get_call_status() == APP_CALL_INCOMING))
    {
        /* to prevent sco con happened during roleswap */
        reject_reason = ROLESWAP_REJECT_REASON_CALL_COMING;
    }
#if (F_APP_AIRPLANE_SUPPORT == 1)
    else if (app_airplane_mode_get())
    {
        reject_reason = ROLESWAP_REJECT_REASON_AIRPLANE_MODE;
    }
#endif
#if (F_APP_SENSOR_MEMS_SUPPORT == 1)
    else if (app_link_get_misc_num(APP_LINK_MEMS) > 0)
    {
        reject_reason = ROLESWAP_REJECT_REASON_MEMS_START;
    }
#endif
    else if (timer_idx_roleswap_stop_linkback != 0)
    {
        reject_reason = ROLESWAP_REJECT_REASON_STOP_LINKBACK_ING;
    }
#if F_APP_CFU_FEATURE_SUPPORT
    else if (app_cfu_is_in_process())
    {
        reject_reason = ROLESWAP_REJECT_REASON_TEAMS_CFU;
    }
#endif
#if F_APP_ANC_SUPPORT
    else if (app_anc_ramp_tool_is_busy() || (feature_map.user_mode == DISABLE))
    {
        reject_reason = ROLESWAP_REJECT_REASON_ANC_HANDLING;
    }
#endif
    else if (event != APP_ROLESWAP_CTRL_EVENT_DECODE_EMPTY /* buffer empty */ &&
             media_buffer_enough == false)
    {
        reject_reason = ROLESWAP_REJECT_REASON_MEDIA_BUFFER_NOT_ENOUGH;
    }
    else if (le_app_link_num > 0)
    {
        reject_reason = ROLESWAP_REJECT_REASON_BLE_EXIST;
    }
    else if (app_link_get_b2s_link_num() > 1)
    {
        reject_reason = ROLESWAP_REJECT_REASON_MULTILINK_EXIST;
    }

    if (roleswap_level == ROLESWAP_BATTERY_LEVEL)
    {
        if (reject_reason == ROLESWAP_REJECT_REASON_BLE_EXIST  ||
#if F_APP_SENSOR_MEMS_SUPPORT
            reject_reason == ROLESWAP_REJECT_REASON_MEMS_START ||
#endif
            reject_reason == ROLESWAP_REJECT_REASON_MULTILINK_EXIST)
        {
            reject_reason = ROLESWAP_REJECT_REASON_NONE;
        }
    }

    return reject_reason;
}

static bool roleswap_single_bud_power_off_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (roleswap_sys_status.single_power_off)
        {
            ret = true;
        }
    }

    return ret;
};

static bool roleswap_close_case_power_off_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (roleswap_sys_status.case_closed_power_off)
        {
            ret = true;
        }
    }

    return ret;
}

static bool roleswap_in_non_smart_case_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (app_roleswap_ctrl_in_non_smart_box())
        {
            ret = true;
        }
    }

    return ret;
}

static bool roleswap_in_case_timeout_power_off_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (app_roleswap_ctrl_in_smart_box_timeout())
        {
            ret = true;
        }
    }

    return ret;
}

#if F_APP_SUPPORT_CAPTURE_DUAL_BUD | F_APP_SUPPORT_CAPTURE_ACOUSTICS_MP
static bool roleswap_data_capture_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if ((event == APP_ROLESWAP_CTRL_EVENT_DATA_CAPTURE)
            || (app_data_capture_need_roleswap() && (event == APP_ROLESWAP_CTRL_EVENT_BLE_DISC)))
        {
            ret = true;
        }
    }

    return ret;
}

static bool roleswap_data_capture_violation(void)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (app_data_capture_get_state() != 0)
        {
            ret = true;
        }
    }

    return ret;
}
#endif

static bool roleswap_src_link_exist(void)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0
#if F_APP_LEA_SUPPORT
        || app_link_get_lea_link_num() > 0
#endif
       )
    {
        ret = true;
    }

    return ret;
}

#if F_APP_EXT_MIC_SWITCH_SUPPORT
static bool roleswap_remote_boom_mic_in_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (roleswap_src_link_exist())
    {
        if (app_db.local_boom_mic_in == false && app_db.remote_boom_mic_in == true)
        {
            ret = true;
        }
    }

    return ret;
}

static bool roleswap_remote_boom_mic_in_violation(void)
{
    bool ret = false;

    if (roleswap_src_link_exist())
    {
        if (app_db.local_boom_mic_in == true && app_db.remote_boom_mic_in == false)
        {
            ret = true;
        }
    }

    return ret;
}
#endif

static bool roleswap_mmi_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (roleswap_src_link_exist())
    {
        if (roleswap_sys_status.mmi_trigger_roleswap)
        {
            ret = true;
        }
    }

    return ret;
}

#if F_APP_DURIAN_SUPPORT
static bool roleswap_durian_fix_mic_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (app_roleswap_ctrl_durian_mic_check())
        {
            ret = true;
        }
    }

    return ret;
}

static bool roleswap_durian_fix_mic_violation(void)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (app_roleswap_ctrl_call_triggered())
        {
            if (app_durian_audio_get_mic_set() == BT_AVP_MIC_ALLWAYS_LEFT)
            {
                if (app_cfg_const.bud_side == DEVICE_ROLE_LEFT)
                {
                    ret = true;
                }
            }
            else if (app_durian_audio_get_mic_set() == BT_AVP_MIC_ALLWAYS_RIGHT)
            {
                if (app_cfg_const.bud_side == DEVICE_ROLE_RIGHT)
                {
                    ret = true;
                }
            }
        }
    }

    return ret;
}
#endif

static bool roleswap_call_in_smart_case_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (roleswap_src_link_exist())
    {
        if (app_roleswap_ctrl_in_box_when_call())
        {
            ret = true;
        }
    }

    return ret;
}

static bool roleswap_call_in_smart_case_violation(void)
{
    bool ret = false;

    if (roleswap_src_link_exist())
    {
        if (app_cfg_const.enable_rtk_charging_box == true &&
            app_roleswap_ctrl_call_triggered() &&
            app_db.local_loc != BUD_LOC_IN_CASE && app_db.remote_loc == BUD_LOC_IN_CASE)
        {
            ret = true;
        }
    }

    return ret;
}

static bool roleswap_call_out_ear_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (roleswap_src_link_exist())
    {
        if (app_roleswap_ctrl_out_ear_when_call())
        {
            ret = true;
        }
    }

    return ret;
}

static bool roleswap_call_out_ear_violation(void)
{
    bool ret = false;

    if (roleswap_src_link_exist())
    {
        if (LIGHT_SENSOR_ENABLED &&
            app_roleswap_ctrl_call_triggered() &&
            app_db.local_loc == BUD_LOC_IN_EAR && app_db.remote_loc != BUD_LOC_IN_EAR)
        {
            ret = true;
        }
    }

    return ret;
}

#if F_APP_LEA_SUPPORT
static bool roleswap_lea_2nd_roleswap_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;
    uint8_t null_addr[6] = {0};
    bool local_lea_connected = memcmp(lea_addr_info.local_lea_addr, null_addr, 6);
    bool remote_lea_connected = memcmp(lea_addr_info.remote_lea_addr, null_addr, 6);
    bool addr_mismatch = false;

    if (local_lea_connected && !remote_lea_connected)
    {
        if (memcmp(app_cfg_nv.bud_local_addr, lea_addr_info.local_lea_addr, 6))
        {
            addr_mismatch = true;
        }
    }
    else if (!local_lea_connected && remote_lea_connected)
    {
        if (memcmp(app_cfg_nv.bud_peer_addr, lea_addr_info.remote_lea_addr, 6))
        {
            addr_mismatch = true;
        }
    }

    if (addr_mismatch)
    {
        ret = true;
    }

    return ret;
}

static bool roleswap_lea_2nd_roleswap_violation(void)
{
    bool ret = false;
    uint8_t null_addr[6] = {0};
    bool local_lea_connected = memcmp(lea_addr_info.local_lea_addr, null_addr, 6);
    bool remote_lea_connected = memcmp(lea_addr_info.remote_lea_addr, null_addr, 6);
    bool addr_match = false;

    if (local_lea_connected && !remote_lea_connected)
    {
        if (!memcmp(app_cfg_nv.bud_local_addr, lea_addr_info.local_lea_addr, 6))
        {
            addr_match = true;
        }
    }
    else if (!local_lea_connected && remote_lea_connected)
    {
        if (!memcmp(app_cfg_nv.bud_peer_addr, lea_addr_info.remote_lea_addr, 6))
        {
            addr_match = true;
        }
    }

    if (addr_match)
    {
        ret = true;
    }

    return ret;
}


void app_roleswap_check_lea_addr_match(T_APP_ROLESWAP_LEA_ADDR_CHECK_EVENT event, void *data)
{
    uint8_t null_addr[6] = {0};
    uint8_t legacy_local_addr[6] = {0};
    uint8_t lea_local_addr[6] = {0};
    uint8_t bd_type = 0;
    T_APP_LE_LINK *p_link = NULL;

    if (event == ADDR_CHECK_LEA_CONNECTED || event == ADDR_CHECK_LEA_DISCONNECTED)
    {
        p_link = data;

        if (p_link != NULL)
        {
#if TARGET_LE_AUDIO_GAMING
            if (!memcmp(p_link->bd_addr, app_cfg_nv.dongle_addr, 6))
            {
                APP_PRINT_TRACE0("dongle connected, no need to do 2nd roleswap");

                /* no need to handle lea 2nd roleswap for gaming dongle link;
                *  cause we use random address
                */
                return;
            }
#endif
            le_get_conn_local_addr(p_link->conn_handle, lea_local_addr, &bd_type);

            if (event == ADDR_CHECK_LEA_CONNECTED)
            {
                if (!memcmp(lea_addr_info.local_lea_addr, null_addr, 6))
                {
                    memcpy(lea_addr_info.local_lea_addr, lea_local_addr, 6);
                }
            }
            else
            {
                if (!memcmp(lea_addr_info.local_lea_addr, lea_local_addr, 6))
                {
                    memset(lea_addr_info.local_lea_addr, 0, 6);
                }
            }
        }
    }

    APP_PRINT_TRACE3("app_roleswap_check_lea_addr_match: event %d local_lea_addr %s bud_local_addr %s",
                     event, TRACE_BDADDR(lea_addr_info.local_lea_addr), TRACE_BDADDR(app_cfg_nv.bud_local_addr));

    app_relay_async_single(APP_MODULE_TYPE_ROLESWAP_CTRL,
                           APP_REMOTE_MSG_ROLESWAP_CTRL_SYNC_LEA_ADDR_INFO);

    app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_CHECK_LEA_ADDR_MATCH);
}
#endif

#if (F_APP_POWER_TEST == 0)
static bool roleswap_battery_level_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (app_db.local_batt_level < BUD_BATT_BOTH_ROLESWAP_THRESHOLD ||
            app_db.remote_batt_level < BUD_BATT_BOTH_ROLESWAP_THRESHOLD)
        {
            /* check low voltage swap */
            if ((event == APP_ROLESWAP_CTRL_EVENT_BATTERY) && app_roleswap_ctrl_battery_check())
            {
                ret = true;
            }
        }
    }

    return ret;
}

static bool roleswap_battery_level_violation(void)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (app_db.local_batt_level < BUD_BATT_BOTH_ROLESWAP_THRESHOLD ||
            app_db.remote_batt_level < BUD_BATT_BOTH_ROLESWAP_THRESHOLD)
        {
            if ((app_db.remote_batt_level < BUD_BATT_BOTH_ROLESWAP_THRESHOLD) &&
                (app_db.local_batt_level > BUD_BATT_BOTH_ROLESWAP_THRESHOLD))
            {
                ret = true;
            }
        }
    }

    return ret;
}

#if F_APP_QOL_MONITOR_SUPPORT
static bool roleswap_rssi_trigger(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool ret = false;

    if (app_link_get_b2s_link_num() > 0)
    {
        if (app_roleswap_ctrl_rssi_check(event))
        {
            ret = true;
        }
    }

    return ret;
}
#endif
#endif

static const T_ROLESWAP_CHECK roleswap_check[] =
{
    [ROLESWAP_SINGLE_BUD_POWER_OFF] = {
        .trigger_check   = roleswap_single_bud_power_off_trigger,
        .violation_check = NULL,
        .level           = ROLESWAP_TRIGGER_LEVEL_HIGH,
    },

    [ROLESWAP_CLOSE_CASE_POWER_OFF] = {
        .trigger_check   = roleswap_close_case_power_off_trigger,
        .violation_check = NULL,
        .level           = ROLESWAP_TRIGGER_LEVEL_HIGH,
    },

    [ROLESWAP_IN_NON_SMART_CASE] = {
        .trigger_check   = roleswap_in_non_smart_case_trigger,
        .violation_check = NULL,
        .level           = ROLESWAP_TRIGGER_LEVEL_HIGH,
    },

    [ROLESWAP_IN_CASE_TIMEOUT_POWER_OFF] = {
        .trigger_check   = roleswap_in_case_timeout_power_off_trigger,
        .violation_check = NULL,
        .level           = ROLESWAP_TRIGGER_LEVEL_HIGH,
    },

#if F_APP_SUPPORT_CAPTURE_DUAL_BUD | F_APP_SUPPORT_CAPTURE_ACOUSTICS_MP
    [ROLESWAP_DATA_CAPTURE] = {
        .trigger_check   = roleswap_data_capture_trigger,
        .violation_check = roleswap_data_capture_violation,
        .level           = ROLESWAP_TRIGGER_LEVEL_HIGH,
    },
#endif

#if F_APP_DURIAN_SUPPORT
    [ROLESWAP_DURIAN_FIX_MIC] = {
        .trigger_check   = roleswap_durian_fix_mic_trigger,
        .violation_check = roleswap_durian_fix_mic_violation,
        .level           = ROLESWAP_TRIGGER_LEVEL_MEDIUM,
    },
#endif

#if F_APP_EXT_MIC_SWITCH_SUPPORT
    [ROLESWAP_REMOTE_BOOM_MIC_IN] = {
        .trigger_check   = roleswap_remote_boom_mic_in_trigger,
        .violation_check = roleswap_remote_boom_mic_in_violation,
        .level           = ROLESWAP_TRIGGER_LEVEL_MEDIUM,
    },
#endif

    [ROLESWAP_CALL_IN_SMART_CASE_WHEN_CALL] = {
        .trigger_check   = roleswap_call_in_smart_case_trigger,
        .violation_check = roleswap_call_in_smart_case_violation,
        .level           = ROLESWAP_TRIGGER_LEVEL_MEDIUM,
    },

    [ROLESWAP_OUT_EAR_WHEN_CALL] = {
        .trigger_check   = roleswap_call_out_ear_trigger,
        .violation_check = roleswap_call_out_ear_violation,
        .level           = ROLESWAP_TRIGGER_LEVEL_MEDIUM,
    },

#if F_APP_DISABLE_LEA_PHONE_SUPPORT == 0
#if F_APP_LEA_SUPPORT
    [ROLESWAP_LEA_2ND_ROLESWAP] = {
        .trigger_check   = roleswap_lea_2nd_roleswap_trigger,
        .violation_check = roleswap_lea_2nd_roleswap_violation,
        .level           = ROLESWAP_TRIGGER_LEVEL_MEDIUM,
    },
#endif
#endif

    [ROLESWAP_MMI_TRIGGER] = {
        .trigger_check   = roleswap_mmi_trigger,
        .violation_check = NULL,
        .level           = ROLESWAP_TRIGGER_LEVEL_LOW,
    },

#if (F_APP_POWER_TEST == 0)
    [ROLESWAP_BATTERY_LEVEL] = {
        .trigger_check   = roleswap_battery_level_trigger,
        .violation_check = roleswap_battery_level_violation,
        .level           = ROLESWAP_TRIGGER_LEVEL_LOW,
    },

#if F_APP_QOL_MONITOR_SUPPORT
    [ROLESWAP_RSSI] = {
        .trigger_check   = roleswap_rssi_trigger,
        .violation_check = NULL,
        .level           = ROLESWAP_TRIGGER_LEVEL_LOW,
    },
#endif
#endif

};

static T_APP_ROLESWAP_LEVEL app_roleswap_ctrl_trigger_check(T_APP_ROLESWAP_CTRL_EVENT event)
{
    T_APP_ROLESWAP_LEVEL trigger_level = ROLESWAP_NONE;
    T_APP_ROLESWAP_LEVEL disallow_level  = ROLESWAP_NONE;
    T_APP_ROLESWAP_LEVEL disallow_level_check = ROLESWAP_NONE;
    T_APP_ROLESWAP_LEVEL actual_trigger = ROLESWAP_NONE;
    uint8_t i = 0;

    if (app_roleswap_ctrl_basic_check() == false)
    {
        goto exit;
    }

    for (i = 0; i < sizeof(roleswap_check) / sizeof(T_ROLESWAP_CHECK); i++)
    {
        if (roleswap_check[i].trigger_check != NULL &&
            roleswap_check[i].trigger_check(event))
        {
            trigger_level = (T_APP_ROLESWAP_LEVEL)i;
            break;
        }
    }

    if (trigger_level != ROLESWAP_NONE && trigger_level >= 1)
    {
        /* check if current roleswap violate higher priority roleswap rule */
        disallow_level_check = (T_APP_ROLESWAP_LEVEL)(trigger_level - 1);

        for (i = 0; i <= disallow_level_check; i++)
        {
            if (roleswap_check[i].violation_check != NULL &&
                roleswap_check[i].violation_check())
            {
                disallow_level = (T_APP_ROLESWAP_LEVEL)i;
                break;
            }
        }
    }

    if (trigger_level != ROLESWAP_NONE)
    {
        if (disallow_level == ROLESWAP_NONE)
        {
            actual_trigger = trigger_level;
        }
    }

exit:
    if (trigger_level != ROLESWAP_NONE)
    {
        APP_PRINT_TRACE3("app_roleswap_ctrl_trigger_check: trigger 0x%02x disallow_level 0x%02x actual_trigger 0x%02x",
                         trigger_level, disallow_level, actual_trigger);
    }

    return actual_trigger;
}

static bool app_roleswap_ctrl_need_poweroff_after_roleswap(void)
{
    uint8_t reason = 0;
    bool ret = false;

    if (roleswap_sys_status.in_case_power_off)
    {
        reason = 1;
    }
    else if (roleswap_sys_status.single_power_off)
    {
        reason = 2;
    }
    else if (roleswap_sys_status.case_closed_power_off)
    {
        reason = 3;
    }

    if (reason != 0)
    {
        APP_PRINT_TRACE1("app_roleswap_ctrl_need_poweroff_after_roleswap: reason %d", reason);

        ret = true;
    }

    return ret;
}

static void app_roleswap_ctrl_update_sys_status(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool cancel_power_off_protect = false;

    APP_PRINT_TRACE1("app_roleswap_ctrl_update_sys_status: event 0x%02X", event);

    if (event == APP_ROLESWAP_CTRL_EVENT_SINGLE_BUD_TO_POWER_OFF)
    {
        roleswap_sys_status.single_power_off = true;
    }
    else if ((event == APP_ROLESWAP_CTRL_EVENT_LOCAL_IN_CASE &&
              app_cfg_const.enable_rtk_charging_box == false && app_cfg_const.enable_inbox_power_off) ||
             (event == APP_ROLESWAP_CTRL_EVENT_IN_CASE_TIMEOUT_TO_POWER_OFF &&
              app_cfg_const.enable_rtk_charging_box == true &&
              app_cfg_const.do_not_auto_power_off_when_case_not_close == false))
    {
        //non-smart case & smart case timeout
        roleswap_sys_status.in_case_power_off = true;
    }
    else if (event == APP_ROLESWAP_CTRL_EVENT_CLOSE_CASE_POWER_OFF)
    {
        roleswap_sys_status.case_closed_power_off = true;
    }
    else if (event == APP_ROLESWAP_CTRL_EVENT_LOCAL_OUT_CASE ||
             event == APP_ROLESWAP_CTRL_EVENT_OPEN_CASE)
    {
        if (event == APP_ROLESWAP_CTRL_EVENT_LOCAL_OUT_CASE)
        {
            roleswap_sys_status.in_case_power_off = false;
        }

        roleswap_sys_status.case_closed_power_off = false;
        cancel_power_off_protect = true;
    }
    else if (event == APP_ROLESWAP_CTRL_EVENT_MMI_TRIGGER_ROLESWAP)
    {
        if (app_roleswap_ctrl_basic_check())
        {
            roleswap_sys_status.mmi_trigger_roleswap = true;
        }
    }

    if (app_roleswap_ctrl_poweroff_triggered())
    {
        app_start_timer(&timer_idx_roleswap_power_off_protect, "roleswap_power_off_protect",
                        app_roleswap_ctrl_timer_id, APP_TIMER_ROLESWAP_POWER_OFF_PROTECT, 0, false,
                        5000);
    }

    if (cancel_power_off_protect)
    {
        app_stop_timer(&timer_idx_roleswap_power_off_protect);
    }

}

static bool app_roleswap_ctrl_must_wait_condition(T_APP_ROLESWAP_REJECT_REASON reject_reason)
{
    bool ret = false;

    /* Note: this condition must only block for a while */
    if (reject_reason == ROLESWAP_REJECT_REASON_ONGOING ||
        reject_reason == ROLESWAP_REJECT_REASON_ACL_ROLE_SLAVE ||
        reject_reason == ROLESWAP_REJECT_REASON_PROFILE_NOT_CONN ||
        reject_reason == ROLESWAP_REJECT_REASON_PROFILE_CONNECTING ||
        reject_reason == ROLESWAP_REJECT_REASON_SNIFFING_STATE ||
        reject_reason == ROLESWAP_REJECT_REASON_A2DP_STREAM_CHANN_CONNECTING ||
        reject_reason == ROLESWAP_REJECT_REASON_HTPOLL_EXECING)
    {
        ret = true;
    }

    return ret;
}

bool app_roleswap_ctrl_check(T_APP_ROLESWAP_CTRL_EVENT event)
{
    bool disallow_call_roleswap_in_out_ear_case = false;
    T_APP_ROLESWAP_REJECT_REASON reject_reason = ROLESWAP_REJECT_REASON_NONE;
    bool roleswap_immediately = false;
    bool roleswap_later = false;
    bool roleswap_triggered = false;
    T_APP_ROLESWAP_LEVEL roleswap_level = ROLESWAP_NONE;
    bool both_buds_in_case = (app_db.local_loc == BUD_LOC_IN_CASE &&
                              app_db.remote_loc == BUD_LOC_IN_CASE);

#if F_APP_LEA_SUPPORT
    uint8_t le_app_link_num = app_link_get_le_link_num() - app_link_get_lea_link_num();
#else
    uint8_t le_app_link_num = app_link_get_le_link_num();
#endif

    if (event == APP_ROLESWAP_CTRL_EVENT_ROLESWAP_SUCCESS ||
        event == APP_ROLESWAP_CTRL_EVENT_REJCT_BY_PEER ||
        event == APP_ROLESWAP_CTRL_EVENT_ROLESWAP_TERMINATED ||
        event == APP_ROLESWAP_CTRL_EVENT_ROLESWAP_FAILED_RETRY)
    {
        bool need_to_power_off = app_roleswap_ctrl_need_poweroff_after_roleswap();
        bool clear_flags = true;

        if ((event != APP_ROLESWAP_CTRL_EVENT_ROLESWAP_SUCCESS) && (need_to_power_off == false))
        {
            clear_flags = false;
        }

        /* clear pending disc flags */
        if (clear_flags)
        {
            roleswap_trigger_status.disc_ble = 0;
            roleswap_trigger_status.disc_inactive_link = 0;
            roleswap_trigger_status.roleswap_is_going_to_do = 0;
            roleswap_sys_status.mmi_trigger_roleswap = 0;
        }

        /* handle power off after roleswap or reject by peer */
        if (need_to_power_off)
        {
            bool do_power_off = false;

            if (event == APP_ROLESWAP_CTRL_EVENT_ROLESWAP_TERMINATED)
            {
                if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    do_power_off = true;
                }
                else
                {
                    app_roleswap_ctrl_update_sys_status(event);
                }
            }
            else
            {
                do_power_off = true;
            }

            if (do_power_off)
            {
                gap_br_send_acl_disconn_req(app_cfg_nv.bud_peer_addr);

                app_mmi_handle_action(MMI_DEV_POWER_OFF);
                goto exit;
            }
        }
    }
    else
    {
        app_roleswap_ctrl_update_sys_status(event);
    }

    roleswap_level = app_roleswap_ctrl_trigger_check(event);
    reject_reason = app_roleswap_ctrl_1st_stage_reject_check();

    if (roleswap_level != ROLESWAP_NONE && reject_reason == ROLESWAP_REJECT_REASON_NONE)
    {
        if (roleswap_check[roleswap_level].level == ROLESWAP_TRIGGER_LEVEL_LOW)
        {
            reject_reason = app_roleswap_ctrl_2nd_stage_reject_check(event, roleswap_level);
        }
    }

    if (roleswap_level != ROLESWAP_NONE && both_buds_in_case == false)
    {
        if (reject_reason == ROLESWAP_REJECT_REASON_NONE ||
            roleswap_check[roleswap_level].level != ROLESWAP_TRIGGER_LEVEL_LOW)
        {
            if (app_roleswap_ctrl_must_wait_condition(reject_reason))
            {
                roleswap_later = true;
            }
            else
            {
                roleswap_immediately = true;
            }
        }
        else
        {
            roleswap_later = true;
        }
    }

    if (roleswap_immediately)
    {
        roleswap_trigger_status.roleswap_is_going_to_do = 1;

        /* handle restart common adv after role swap */
        if ((app_link_le_check_rtk_link_exist() == true) ||
            (app_ble_common_adv_get_state() == BLE_EXT_ADV_MGR_ADV_ENABLED))
        {
            //ble link or ble adv exist before roleswap, need start ble adv after roleswap
            if (app_db.ble_common_adv_after_roleswap == false)
            {
                app_db.ble_common_adv_after_roleswap = true;
                app_ble_common_adv_sync_start_flag();
            }
        }

        bool disc_ble_or_inactive_link = false;

        /* disconnect multilink */
        if (roleswap_trigger_status.disc_inactive_link == 0 &&
            app_link_get_b2s_link_num() > 1)
        {
            if (roleswap_trigger_status.disc_inactive_link == 0)
            {
                app_multi_roleswap_disconnect_inactive_link();

                uint8_t cmd_ptr[7];
                memcpy(&cmd_ptr[0], app_db.resume_addr, 6);
                cmd_ptr[6] = app_db.connected_num_before_roleswap;
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_RESUME_BT_ADDRESS,
                                                    cmd_ptr, 7);
            }

            roleswap_trigger_status.disc_inactive_link = 1;
            disc_ble_or_inactive_link = true;
        }

        /* disconnect ble */
        if (le_app_link_num > 0)
        {
            if (roleswap_trigger_status.disc_ble == 0)
            {
                app_ble_gap_disconnect_all(LE_LOCAL_DISC_CAUSE_ROLESWAP);
            }

            roleswap_trigger_status.disc_ble = 1;
            disc_ble_or_inactive_link = true;
        }

        if (disc_ble_or_inactive_link ||
            /* make sure ble and mutilink disc before roleswap;
               this must check the real condition to prevent priority check issue */
            (le_app_link_num > 0 || app_link_get_b2s_link_num() > 1))
        {
            reject_reason = ROLESWAP_REJECT_REASON_DISC_BLE_OR_INACTIVE_LINK;
            goto exit;
        }

        /* roleswap is triggered */
        roleswap_trigger_status.roleswap_is_going_to_do = 0;

        if (roleswap_status == APP_ROLESWAP_STATUS_IDLE)
        {
            if (app_bt_policy_get_radio_mode() == BT_DEVICE_MODE_CONNECTABLE ||
                app_bt_policy_get_radio_mode() == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE ||
                app_bt_policy_get_state() == BP_STATE_LINKBACK)
            {
                /* close page and page scan before roleswap */
                app_bt_policy_prepare_for_roleswap();
                /* start a timer to exec roleswap after closing page and page scan */
                app_start_timer(&timer_idx_roleswap_stop_linkback, "roleswap_stop_linkback",
                                app_roleswap_ctrl_timer_id, APP_TIMER_ROLESWAP_STOP_LINKBACK, 0, false,
                                100);

            }
            else
            {
                app_roleswap_ctrl_exec_roleswap();
            }
        }
    }

exit:
    if (roleswap_immediately == false)
    {
        roleswap_trigger_status.roleswap_is_going_to_do = 0;
    }

    if (roleswap_immediately || roleswap_later || roleswap_status != APP_ROLESWAP_STATUS_IDLE)
    {
        roleswap_triggered = true;
    }
    else
    {
        if (roleswap_trigger_status.roleswap_is_going_to_do == 0 &&
            (roleswap_trigger_status.disc_ble || roleswap_trigger_status.disc_inactive_link))
        {
            app_roleswap_ctrl_cancel_restore_link();
        }

        // /* restore multilink inactive link */
        // if (event == APP_ROLESWAP_CTRL_EVENT_ROLESWAP_SUCCESS && app_db.connected_num_before_roleswap)
        // {
        //     /* only primary */
        //     app_roleswap_ctrl_reconnect_inactive_link();
        // }
    }

    APP_PRINT_TRACE6("app_roleswap_ctrl_check: roleswap_immediately (%d %d) event 0x%02X, level 0x%02X, reject 0x%02X, both_in_case %d",
                     roleswap_immediately, roleswap_later, event, roleswap_level, reject_reason,
                     both_buds_in_case);

    return roleswap_triggered;
}

static void app_roleswap_ctrl_bud_loc_event_cback(uint32_t event, void *msg)
{
    bool from_remote = *((bool *)msg);

    if (from_remote)
    {
        switch (event)
        {
        case EVENT_IN_CASE:
            {
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_REMOTE_IN_CASE);
            }
            break;

        case EVENT_OUT_CASE:
            {
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_REMOTE_OUT_CASE);
            }
            break;

        case EVENT_IN_EAR:
            {
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_REMOTE_IN_EAR);
            }
            break;

        case EVENT_OUT_EAR:
            {
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_REMOTE_OUT_EAR);
            }
            break;
        }
    }
    else
    {
        switch (event)
        {
        case EVENT_IN_CASE:
            {
                bool roleswap_is_triggered = app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_LOCAL_IN_CASE);

                /* if dut mode in box; it must power off */
                if (app_cfg_const.enable_inbox_power_off && roleswap_is_triggered == false)
                {
                    app_mmi_handle_action(MMI_DEV_POWER_OFF);
                }
            }
            break;

        case EVENT_OUT_CASE:
            {
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_LOCAL_OUT_CASE);
            }
            break;

        case EVENT_IN_EAR:
            {
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_LOCAL_IN_EAR);
            }
            break;

        case EVENT_OUT_EAR:
            {
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_LOCAL_OUT_EAR);
            }
            break;
        }
    }

    APP_PRINT_TRACE2("app_roleswap_ctrl_bud_loc_event_cback 0x%02X, from_remote %d", event,
                     from_remote);
}

void app_roleswap_ctrl_init(void)
{
    app_timer_reg_cb(app_roleswap_ctrl_timeout_cb, &app_roleswap_ctrl_timer_id);
    bt_mgr_cback_register(app_roleswap_ctrl_bt_cback);
#if F_APP_ERWS_SUPPORT
    app_relay_cback_register(app_roleswap_ctrl_relay_cback, app_roleswap_ctrl_parse_cback,
                             APP_MODULE_TYPE_ROLESWAP_CTRL, APP_REMOTE_MSG_ROLESWAP_CTRL_TOTAL);
# endif
    app_ipc_subscribe(BUD_LOCATION_IPC_TOPIC, app_roleswap_ctrl_bud_loc_event_cback);
}
#else

#include <stdbool.h>
#include "app_roleswap_control.h"

T_APP_ROLESWAP_STATUS app_roleswap_ctrl_get_status(void)
{
    return APP_ROLESWAP_STATUS_IDLE;
}

bool app_roleswap_ctrl_check(T_APP_ROLESWAP_CTRL_EVENT event)
{
    return false;
}

void app_roleswap_ctrl_init(void)
{
    return;
}

#endif

