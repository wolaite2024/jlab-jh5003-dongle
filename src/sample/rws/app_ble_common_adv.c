/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "stdlib.h"
#include "os_queue.h"
#include "trace.h"
#include "gap_le_types.h"
#include "app_ble_timer.h"
#include "app_ble_common_adv.h"
#include "ble_ext_adv.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_ble_gap.h"
#include "app_tts.h"
#include "app_relay.h"
#include "app_ble_device.h"
#include <stdlib.h>
#include "app_cmd.h"
#include "app_ble_rand_addr_mgr.h"
#include "app_roleswap_control.h"
#include "app_vendor.h"

#if F_APP_GAMING_DONGLE_SUPPORT
#include "app_dongle_dual_mode.h"
#endif

#if F_APP_CHATGPT_SUPPORT
#include "app_chatgpt.h"
#endif

typedef enum
{
    SYNC_BLE_ADV_START_FLAG     = 0x00,

    LE_COMMON_REMOTE_MSG_MAX    = 0x01,
} LE_COMMON_REMOTE_MSG;


typedef struct CB_ELEM CB_ELEM;

typedef struct CB_ELEM
{
    CB_ELEM             *p_next;
    LE_COMMON_ADV_CB    cb;
} CB_ELEM;



struct
{
    bool use_static_addr_type;
    uint8_t adv_handle;
    uint8_t conn_id;
    T_BLE_EXT_ADV_MGR_STATE state;
    T_OS_QUEUE cb_queue;
} le_common_adv =
{
#if F_APP_SC_KEY_DERIVE_SUPPORT
    .use_static_addr_type = false,
#else
    .use_static_addr_type = true,
#endif
    .adv_handle = 0xff,
    .conn_id = 0xff,
    .state = BLE_EXT_ADV_MGR_ADV_DISABLED,
    .cb_queue = {0}
};

static uint8_t app_ble_common_adv_data[31] =
{
    /* This modification is used to solve issue: BLE name shown on Android list */
    /* TX Power Level */
    0x02,/* length */
    GAP_ADTYPE_POWER_LEVEL,
    0x00, /* Don't care now */

    /* Service */
    17,/* length */
    GAP_ADTYPE_128BIT_COMPLETE,/* "Complete 128-bit UUIDs available"*/
    /*transmit service uuid*/
    0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFD, 0x02, 0x00,
    0x00,/*bud role:0x00 single,0x01 pri, 0x02 sec*/

    /* Manufacturer Specific Data */
    9,/* length */
    0xFF,
    0x5D, 0x00 /* Company ID */
    /*local address*/
};


static void app_ble_common_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));
    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            APP_PRINT_TRACE2("app_ble_common_adv_callback: adv_state %d, adv_handle %d",
                             cb_data.p_ble_state_change->state, cb_data.p_ble_state_change->adv_handle);
            le_common_adv.state = cb_data.p_ble_state_change->state;
            if (le_common_adv.state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
                if (app_cfg_nv.legacy_gaming_support_common_adv)
                {
                    app_vendor_htpoll_control(B2S_HTPOLL_EVENT_COMMON_ADV_ENABLE);
                }
#endif
            }
            else
            {
#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
                if (app_cfg_nv.legacy_gaming_support_common_adv)
                {
                    app_vendor_htpoll_control(B2S_HTPOLL_EVENT_COMMON_ADV_DISABLE);
                }
#endif

                switch (cb_data.p_ble_state_change->stop_cause)
                {
                case BLE_EXT_ADV_STOP_CAUSE_APP:
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_CONN:
                    {
#if F_APP_CHARGER_CASE_SUPPORT
                        app_ble_common_adv_start_rws(0);
#endif
                    }
                    break;

                case BLE_EXT_ADV_STOP_CAUSE_TIMEOUT:
                    if ((app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SECONDARY) && le_common_adv.use_static_addr_type)
                    {
                        /*secondary ear ota need modify the random address and start advertising,
                        when advertising timeout, the random address shall be set back to le_rws_random_addr*/
                        uint8_t rand_addr[6] = {0};
                        app_ble_rand_addr_get(rand_addr);
                        app_ble_common_adv_set_random(rand_addr);
                    }
                    break;

                default:
                    break;
                }
                APP_PRINT_TRACE2("app_ble_common_adv_callback: stack stop adv cause 0x%x, app stop adv cause 0x%02x",
                                 cb_data.p_ble_state_change->stop_cause, cb_data.p_ble_state_change->app_cause);
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        APP_PRINT_TRACE4("app_ble_common_adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x, adv_handle %d, local_addr_type %d, local_bd %b",
                         cb_data.p_ble_conn_info->conn_id,
                         cb_data.p_ble_conn_info->adv_handle,
                         cb_data.p_ble_conn_info->local_addr_type,
                         TRACE_BDADDR(cb_data.p_ble_conn_info->local_addr));

        if (cb_data.p_ble_conn_info->adv_handle == le_common_adv.adv_handle)
        {
            /*both lea adv and common adv use public address.
             When phone tries to connect to lea adv, it mistakenly connects to the common adv,
             This causes common adv to be forced to stop, In this case, we need to reopen common adv.
            */
            app_ble_timer_set_current_link_conn_id(cb_data.p_ble_conn_info->conn_id);
            app_ble_timer_start_check_common_link();
        }

        break;

    default:
        break;
    }

    CB_ELEM *p_cb_elem = (CB_ELEM *)le_common_adv.cb_queue.p_first;
    APP_PRINT_TRACE1("app_ble_common_adv_callback: p_cb_elem %p", p_cb_elem);

    for (; p_cb_elem != NULL; p_cb_elem = p_cb_elem->p_next)
    {
        APP_PRINT_TRACE1("app_ble_common_adv_callback: cb %p", p_cb_elem->cb);
        p_cb_elem->cb(cb_type, cb_data);
    }

    return;
}

/**
 * @brief app_ble_common_adv_ota_data_update
 * This api can only be used for secondary ear ota update adv data.
 * Note: Ensure that the advertising address of the secondary ear and the primary ear are different.
 *       If the advertising addresses on both sides are the same, the phone may not know which ear to be upgrade.
 * @return true
 * @return false
 */
bool app_ble_common_adv_ota_data_update(void)
{
    uint8_t mac_addr[6];

    app_ble_common_adv_data[20] = app_cfg_nv.bud_role;
    app_ble_common_adv_data[21] = 3 + sizeof(mac_addr);
    app_ble_common_adv_data[22] = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
    app_ble_common_adv_data[23] = 0x5D;
    app_ble_common_adv_data[24] = 0x00;

    for (int i = 0; i < 6; i++)
    {
        mac_addr[i] = app_cfg_nv.bud_peer_addr[5 - i];
    }
    memcpy(&app_ble_common_adv_data[25], mac_addr, sizeof(mac_addr));

    if (le_common_adv.use_static_addr_type)
    {
        /*if use static random address to start adv,
        primary ear and secondary ear has the same static random address,
        so secondary ear ota need modify the random address and then start advertising,
        when advertising timeout, the random address shall be set back to le_rws_random_addr,
        when secondary ble link connected, the random address shall be set back to le_rws_random_addr*/
        uint8_t ota_random_address[6] = {0};
        app_ble_rand_addr_get(ota_random_address);
        ota_random_address[0]++;
        app_ble_common_adv_set_random(ota_random_address);
    }
    else
    {
        /*if use public address to start adv,
        primary ear and secondary ear has different public address, so we need do nothing*/
    }

    if (ble_ext_adv_mgr_set_adv_data(le_common_adv.adv_handle, sizeof(app_ble_common_adv_data),
                                     app_ble_common_adv_data) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void app_ble_common_adv_bud_role_update(T_REMOTE_SESSION_ROLE role)
{
    /*app_ble_common_adv_data update bud role*/
    if (app_ble_common_adv_data[20] != role)
    {
        app_ble_common_adv_data[20] = role;
        ble_ext_adv_mgr_set_adv_data(le_common_adv.adv_handle, sizeof(app_ble_common_adv_data),
                                     app_ble_common_adv_data);
    }
    else
    {
        //bud role not change
    }
}

void app_ble_common_adv_sec_stop_adv_and_link()
{
    APP_PRINT_INFO0("app_ble_common_adv_sec_stop_adv_and_link");

    //app_ble_common_adv_stop
    if (le_common_adv.state != BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        app_ble_common_adv_stop(APP_STOP_ADV_CAUSE_ROLE_SECONDARY);
    }

    //ble link disconnect
    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].is_common_link)
        {
            app_ble_gap_disconnect(&app_db.le_link[i], LE_LOCAL_DISC_CAUSE_SECONDARY);
        }
    }
}

void app_ble_common_adv_handle_roleswap(T_REMOTE_SESSION_ROLE role)
{
    /*app_ble_common_adv_data update bud role*/
    bool common_link_exist = false;

    app_ble_common_adv_bud_role_update(role);
    common_link_exist = app_link_le_check_common_link_exist();
    APP_PRINT_INFO3("app_ble_common_adv_handle_roleswap: app_ble_common_adv_data %b, common_link_exist %d %d",
                    TRACE_BINARY(sizeof(app_ble_common_adv_data), app_ble_common_adv_data),
                    common_link_exist, app_db.ble_common_adv_after_roleswap);

    if (role == REMOTE_SESSION_ROLE_PRIMARY && app_db.ble_common_adv_after_roleswap)
    {
        app_ble_common_adv_start_rws(app_cfg_const.timer_ota_adv_timeout * 100);
    }
    else if (role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        app_ble_common_adv_sec_stop_adv_and_link();
    }
    app_db.ble_common_adv_after_roleswap = false;
}

void app_ble_common_adv_handle_role_swap_fail(T_REMOTE_SESSION_ROLE device_role)
{
    if (device_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_ble_common_adv_start_rws(app_cfg_const.timer_ota_adv_timeout * 100);
    }
}

void app_ble_common_adv_handle_engage_role_decided(void)
{
    /*app_ble_common_adv_data update bud role*/
    bool common_link_exist = false;

    app_ble_common_adv_bud_role_update((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    common_link_exist = app_link_le_check_common_link_exist();
    APP_PRINT_INFO2("app_ble_common_adv_handle_engage_role_decided: app_ble_common_adv_data %b, common_link_exist %d",
                    TRACE_BINARY(sizeof(app_ble_common_adv_data), app_ble_common_adv_data), common_link_exist);

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        app_ble_device_handle_power_on_rtk_adv();
    }
    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        app_ble_common_adv_sec_stop_adv_and_link();
    }
}

bool app_ble_common_adv_start_rws(uint16_t duration_10ms)
{
    if (app_link_get_le_link_num() >= MAX_BLE_LINK_NUM)
    {
        APP_PRINT_ERROR0("app_ble_common_adv_start_rws: ble link number exceed the max number");
        return false;
    }

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        APP_PRINT_TRACE0("app_ble_common_adv_start_rws: bud_role 2, should not start adv");
        return false;
    }
    bool ret = app_ble_common_adv_start(duration_10ms);
    return ret;
}


#if F_APP_ERWS_SUPPORT

uint16_t app_ble_common_adv_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    APP_PRINT_INFO1("app_ble_common_adv_relay_cback: msg_type %d", msg_type);

    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    switch (msg_type)
    {
    case SYNC_BLE_ADV_START_FLAG:
        {
            payload_len = 1;
            msg_ptr = &app_db.ble_common_adv_after_roleswap;
        }
        break;

    default:
        break;

    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_BLE_COMMON_ADV, payload_len, msg_ptr, skip,
                              total);
}


static void app_ble_common_adv_parse_cback(LE_COMMON_REMOTE_MSG msg_type, uint8_t *buf,
                                           uint16_t len, T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("app_ble_common_adv_parse_cback: msg = 0x%x, status = %x", msg_type, status);

    switch (msg_type)
    {
    case SYNC_BLE_ADV_START_FLAG:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    uint8_t *p_info = (uint8_t *)buf;
                    app_db.ble_common_adv_after_roleswap = p_info[0];
                }
            }
        }
        break;

    default:
        break;
    }

}
#endif


bool app_ble_common_adv_start(uint16_t duration_10ms)
{
    if (le_common_adv.state == BLE_EXT_ADV_MGR_ADV_DISABLED)
    {
        if (ble_ext_adv_mgr_enable(le_common_adv.adv_handle, duration_10ms) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        APP_PRINT_TRACE0("app_ble_common_adv_start: Already started");
        return true;
    }
}

bool app_ble_common_adv_stop(int8_t app_cause)
{
    APP_PRINT_INFO0("app_ble_common_adv_stop");
    if (ble_ext_adv_mgr_disable(le_common_adv.adv_handle, app_cause) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

T_BLE_EXT_ADV_MGR_STATE app_ble_common_adv_get_state(void)
{
    return le_common_adv.state;
}

uint8_t app_ble_common_adv_get_adv_handle(void)
{
    return le_common_adv.adv_handle;
}

void app_ble_common_adv_set_random(uint8_t *random_address)
{
    ble_ext_adv_mgr_set_random(le_common_adv.adv_handle, random_address);
}

void app_ble_common_adv_handle_ble_disconnected(uint8_t conn_id, uint8_t local_disc_cause,
                                                uint16_t disc_cause)
{
    if (app_cfg_const.rtk_app_adv_support)
    {
        if (app_roleswap_ctrl_get_status() == APP_ROLESWAP_STATUS_IDLE &&
            local_disc_cause != LE_LOCAL_DISC_CAUSE_ROLESWAP)
        {
            app_cmd_update_eq_ctrl(false, true);
        }

        if (local_disc_cause != LE_LOCAL_DISC_CAUSE_POWER_OFF &&
            local_disc_cause != LE_LOCAL_DISC_CAUSE_ROLESWAP &&
            local_disc_cause != LE_LOCAL_DISC_CAUSE_OTA_RESET &&
            local_disc_cause != LE_LOCAL_DISC_CAUSE_RWS_OTA_SUCCESS)
        {
#if F_APP_GAMING_DONGLE_SUPPORT
            app_dongle_handle_rtk_adv(true);
#else
            app_ble_device_handle_power_on_rtk_adv();
#endif
        }
    }
}

#if F_APP_ERWS_SUPPORT
void app_ble_common_adv_sync_start_flag(void)
{
    app_relay_async_single(APP_MODULE_TYPE_BLE_COMMON_ADV, SYNC_BLE_ADV_START_FLAG);
}
#endif


void app_ble_common_adv_cb_reg(LE_COMMON_ADV_CB cb)
{
    CB_ELEM *p_cb_elem = calloc(1, sizeof(CB_ELEM));

    p_cb_elem->cb = cb;

    APP_PRINT_TRACE2("app_ble_common_adv_cb_reg: p_cb_elem %p, cb %p", p_cb_elem, cb);

    os_queue_in(&le_common_adv.cb_queue, p_cb_elem);
}

void app_ble_common_adv_enable_multilink(void)
{
    uint8_t common_link_num = 0;

    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].is_common_link == true)
        {
            common_link_num++;
        }
    }

    if (common_link_num < app_cfg_const.supported_common_link_number)
    {
        app_ble_device_handle_power_on_rtk_adv();
    }

    APP_PRINT_TRACE2("app_ble_common_adv_enable_multilink: existed common link num %d, supported max common link num",
                     common_link_num, app_cfg_const.supported_common_link_number);
}

static void app_ble_common_adv_rand_addr_cb(RANDOM_ADDR_MGR_EVT_DATA *evt_data)
{
    APP_PRINT_TRACE1("app_ble_tts_ota app_ble_common_adv_rand_addr_cb: evt %d", evt_data->evt);

    uint8_t *rand_addr = NULL;

    if (le_common_adv.use_static_addr_type == false)
    {
        return;
    }

    if (evt_data->evt == RANDOM_ADDR_TWS_ADDR_UPD)
    {
        rand_addr = evt_data->upd_tws_addr.addr;
    }
    else if (evt_data->evt == RANDOM_ADDR_TWS_ADDR_RECVED)
    {
        rand_addr = evt_data->recv_tws_addr.addr;
    }

    app_ble_common_adv_set_random(rand_addr);
}

bool app_ble_common_adv_update_scan_rsp_data(void)
{
    /*scan_rsp_data update ble device name*/
    app_ble_gap_gen_scan_rsp_data(&scan_rsp_data_len, scan_rsp_data);
    T_GAP_CAUSE ret = ble_ext_adv_mgr_set_scan_response_data(le_common_adv.adv_handle,
                                                             scan_rsp_data_len, scan_rsp_data);

    if (ret == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void app_ble_common_adv_update_peer_addr(void)
{
    for (int i = 0; i < 6; i++)
    {
        app_ble_common_adv_data[25 + i] = app_cfg_nv.bud_peer_addr[5 - i];
    }

    ble_ext_adv_mgr_set_adv_data(le_common_adv.adv_handle, sizeof(app_ble_common_adv_data),
                                 app_ble_common_adv_data);
}

#if F_APP_GATT_OVER_BREDR_SUPPORT
extern uint8_t GATT_UUID128_RVDIS[16];
static void app_ble_common_adv_load_rvdis_uuid(uint8_t *data)
{
    memcpy(data, GATT_UUID128_RVDIS, sizeof(GATT_UUID128_RVDIS));
}
#endif

void app_ble_common_adv_init(void)
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint16_t adv_interval_min = 0xA0;
    uint16_t adv_interval_max = 0xB0;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;

    if (le_common_adv.use_static_addr_type)
    {
        own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    }
    else
    {
        own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    }

    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    uint8_t data_len = 0;

    /* company id */
    app_ble_common_adv_data[20] = app_cfg_nv.bud_role;

#if F_APP_DURIAN_SUPPORT
    app_ble_common_adv_data[23] = 0x5D;//app_cfg_const.company_id not exist
    app_ble_common_adv_data[24] = 0x00;
#else
    app_ble_common_adv_data[23] = app_cfg_const.company_id[0];
    app_ble_common_adv_data[24] = app_cfg_const.company_id[1];
#endif

    uint8_t *spk1_mac = NULL;

    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE))
    {
        spk1_mac = app_cfg_nv.bud_local_addr;
    }
    else
    {
        spk1_mac = app_cfg_nv.bud_peer_addr;
    }

    for (int i = 0; i < 6; i++)
    {
        app_ble_common_adv_data[25 + i] = spk1_mac[5 - i];
    }

#if F_APP_GATT_OVER_BREDR_SUPPORT
    app_ble_common_adv_load_rvdis_uuid(&app_ble_common_adv_data[5]);
#endif

#if F_APP_DURIAN_SUPPORT
    data_len = sizeof(app_cfg_nv.bud_local_addr) + 2;//2:sizeof(app_cfg_const.company_id)
#else
    data_len = sizeof(app_cfg_nv.bud_local_addr) + sizeof(app_cfg_const.company_id);
#endif

#if F_APP_CHATGPT_SUPPORT
    app_chatgpt_load_uuid(&app_ble_common_adv_data[5]);
#endif

    app_ble_gap_gen_scan_rsp_data(&scan_rsp_data_len, scan_rsp_data);

    uint8_t le_random_addr[6] = {0};
    app_ble_rand_addr_get(le_random_addr);

    ble_ext_adv_mgr_init_adv_params(&le_common_adv.adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, 23 + data_len, app_ble_common_adv_data,
                                    scan_rsp_data_len, scan_rsp_data, le_random_addr);

    APP_PRINT_TRACE2("app_ble_common_adv_init: app_ble_common_adv_data %b, le_random_addr %s",
                     TRACE_BINARY(sizeof(app_ble_common_adv_data), app_ble_common_adv_data),
                     TRACE_BDADDR(le_random_addr));

    ble_ext_adv_mgr_register_callback(app_ble_common_adv_callback, le_common_adv.adv_handle);

#if F_APP_ERWS_SUPPORT
    app_relay_cback_register(app_ble_common_adv_relay_cback,
                             (P_APP_PARSE_CBACK)app_ble_common_adv_parse_cback,
                             APP_MODULE_TYPE_BLE_COMMON_ADV, LE_COMMON_REMOTE_MSG_MAX);
#endif
    os_queue_init(&le_common_adv.cb_queue);

    app_ble_rand_addr_cb_reg(app_ble_common_adv_rand_addr_cb);
    app_ble_timer_init();
}
