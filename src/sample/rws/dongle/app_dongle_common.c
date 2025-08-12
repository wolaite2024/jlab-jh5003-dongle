#if F_APP_COMMON_DONGLE_SUPPORT
#include <string.h>
#include "bt_bond.h"
#include "gap_br.h"
#include "sysm.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "app_bond.h"
#include "app_main.h"
#include "app_dongle_common.h"
#include "trace.h"
#include "app_timer.h"
#include "app_ble_common_adv.h"

#if F_APP_LEA_SUPPORT
#include "app_lea_mgr.h"
#include "transmit_svc_dongle.h"
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
#include "app_dongle_dual_mode.h"
#endif

#if F_APP_LEA_DONGLE_BINDING
#include "app_lea_pacs.h"
#include "csis_rsi.h"
#include "app_lea_unicast_audio.h"
#endif

#if F_APP_BLE_HID_CONTROLLER_SUPPORT
#include "app_ble_hid_controller.h"
#endif

#if F_APP_LEGACY_DONGLE_BINDING || F_APP_LEA_DONGLE_BINDING
bool dongle_pairing_non_intentionally = false;

#define HEADSET_ADV_STABLE_TIME                300000 // 300sec

#define DONGLE_ADV_INTERVAL_NORMAL             0xA0
#define DONGLE_ADV_INTERVAL_LONG               0x140 // 200ms 
#define OTHER_ADV_INTERVAL_NORMAL              0xA0
#define OTHER_ADV_INTERVAL_LONG                0x280

typedef enum
{
    HEADSET_ADV_STATE_INIT,
    HEADSET_ADV_STATE_BT_STREAMING,
    HEADSET_ADV_STATE_STABLE,
} T_APP_HEADSET_ADV_STATE;

#if F_APP_LEGACY_DONGLE_BINDING
static T_LEGACY_DONGLE_BLE_DATA dongle_adv;
static uint8_t legacy_dongle_adv_handle = 0xff;
#endif

#if F_APP_LEA_DONGLE_BINDING
static uint8_t dongle_lea_adv_conn_id = 0xff;
static T_DONGLE_LEA_ADV_DATA dongle_lea_adv;
static uint8_t lea_dongle_adv_handle = 0xff;

#endif
#endif

typedef enum
{
    APP_TIMER_INCREASE_HEADSET_ADV_INTERVAL            = 0x00,
} T_APP_DONGLE_TIMER;

static uint8_t app_common_dongle_timer_id = 0;
static uint8_t timer_idx_change_headset_adv_interval = 0;

static void app_dongle_common_set_ext_eir(void)
{
    uint8_t p_eir[10];

    p_eir[0] = 9; /* length */
    p_eir[1] = 0xFF;
    p_eir[2] = 0x5D;
    p_eir[3] = 0x00;
    p_eir[4] = 0x08;

    /* bit0: 0- stereo headset 1- TWS headset
       bit1: 1- support LowLatency with Gaming Dongle
             0- not support LowLatency with Gaming Dongle
    */
    p_eir[5] = (remote_session_role_get() == REMOTE_SESSION_ROLE_SINGLE) ? 0x02 : 0x03;

    /*
         bit 3~0: 0: sbc frame nums in each avdtp packet depend on Gaming Dongle
    */
    p_eir[6] = 0x0;

    //Set pairing ID
    p_eir[7] = (app_cfg_const.rws_custom_uuid >> 8);
    p_eir[8] = app_cfg_const.rws_custom_uuid & 0xFF;

    /*
        bit 1~0: Set SPP Voice Sample Rate.
        bit   2: Set Multilink feature bit.
        bit 7~3: rsv.
    */
    p_eir[9] = app_cfg_const.spp_voice_smaple_rate & 0x03;
    p_eir[9] |= 0 << 2;

    gap_br_set_ext_eir(&p_eir[0], 10);
}

static void app_dongle_common_device_event_cback(uint32_t event, void *msg)
{
    switch (event)
    {
    case APP_DEVICE_IPC_EVT_STACK_READY:
        {
            app_dongle_common_set_ext_eir();
        }
        break;

    default:
        break;
    }
}

T_APP_BR_LINK *app_dongle_get_connected_dongle_link(void)
{
    T_APP_BR_LINK *p_link = NULL;
    uint8_t addr[6] = {0};

    if (app_dongle_get_connected_dongle_addr(addr))
    {
        p_link = app_link_find_br_link(addr);
    }

    return p_link;
}

#if F_APP_LEA_SUPPORT
T_APP_LE_LINK *app_dongle_get_connected_lea_phone_link()
{
    T_APP_LE_LINK *p_lea_link = NULL;
    uint8_t i;

    if (app_link_get_le_link_num())
    {
        for (i = 0; i < MAX_BLE_LINK_NUM; i++)
        {
            if (app_db.le_link[i].used &&
                app_db.le_link[i].lea_link_state >= LEA_LINK_CONNECTED &&
                memcmp(app_db.le_link[i].bd_addr, app_cfg_nv.dongle_addr, 6))
            {
                p_lea_link = &app_db.le_link[i];
                break;
            }
        }
    }

    return p_lea_link;
}
#endif

T_APP_BR_LINK *app_dongle_get_connected_phone_link(void)
{
    T_APP_BR_LINK *p_link = NULL;
    uint32_t bond_flag;
    uint8_t i;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_link_check_b2s_link_by_id(i))
        {
            bt_bond_flag_get(app_db.br_link[i].bd_addr, &bond_flag);

            if ((bond_flag & BOND_FLAG_DONGLE) == 0)
            {
                p_link = app_link_find_br_link(app_db.br_link[i].bd_addr);
                break;
            }
        }
    }

    return p_link;
}

bool app_dongle_get_connected_dongle_addr(uint8_t *addr)
{
    bool ret = false;
    uint8_t i = 0;
    uint32_t bond_flag;

    for (i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_db.br_link[i].used == true)
        {
            bt_bond_flag_get(app_db.br_link[i].bd_addr, &bond_flag);

            if (bond_flag & BOND_FLAG_DONGLE)
            {
                memcpy(addr, app_db.br_link[i].bd_addr, 6);
                ret = true;
                break;
            }
        }
    }

    return ret;
}

bool app_dongle_is_dongle_addr(uint8_t *check_addr)
{
    bool ret = false;
    uint32_t bond_flag = 0;

    if (bt_bond_flag_get(check_addr, &bond_flag) &&
        (bond_flag & BOND_FLAG_DONGLE))
    {
        ret = true;
    }

    return ret;
}

void app_dongle_save_dongle_addr(uint8_t *addr)
{
    if (memcmp(app_cfg_nv.dongle_addr, addr, 6))
    {
        APP_PRINT_TRACE1("app_dongle_save_dongle_addr: %s", TRACE_BDADDR(addr));

        memcpy(app_cfg_nv.dongle_addr, addr, 6);

        app_cfg_store(app_cfg_nv.dongle_addr, 6);
    }
}

#if F_APP_LEA_SUPPORT
T_APP_LE_LINK *app_dongle_get_le_audio_dongle_link(void)
{
    T_APP_LE_LINK *p_lea_link = NULL;
    uint8_t i;

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].used)
        {
            if ((app_db.le_link[i].remote_device_type == DEVICE_TYPE_DONGLE) &&
                (app_db.le_link[i].lea_link_state >= LEA_LINK_CONNECTED))
            {
                p_lea_link = &app_db.le_link[i];
                break;
            }

            if (!memcmp(app_db.le_link[i].bd_addr, app_cfg_nv.dongle_addr, 6))
            {
                p_lea_link = &app_db.le_link[i];
                break;
            }
        }
    }

    return p_lea_link;
}

T_APP_LE_LINK *app_dongle_get_le_audio_phone_link(void)
{
    T_APP_LE_LINK *p_lea_link = NULL;
    uint8_t i;

    if (app_link_get_le_link_num())
    {
        for (i = 0; i < MAX_BLE_LINK_NUM; i++)
        {
            if ((app_db.le_link[i].used) &&
                (app_db.le_link[i].remote_device_type != DEVICE_TYPE_DONGLE) &&
                (app_db.le_link[i].lea_link_state >= LEA_LINK_CONNECTED))
            {
                p_lea_link = &app_db.le_link[i];
                break;
            }
        }
    }

    return p_lea_link;
}


#if F_APP_GAMING_DONGLE_SUPPORT
bool app_dongle_is_iso_data_from_dongle(uint16_t conn_handle)
{
    bool ret = false;

    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    if (p_link != NULL && p_link->remote_device_type == DEVICE_TYPE_DONGLE)
    {
        ret = true;
    }

    return ret;
}

void app_dongle_check_exit_pairing_state(uint8_t link_state, uint8_t *bd_addr)
{
    if (link_state == LEA_LINK_CONNECTED && app_bt_policy_is_pairing())
    {
        if (!app_bt_policy_check_keep_pairing_state(bd_addr))
        {
            app_bt_policy_exit_pairing_mode();
        }
    }
}
#endif
#endif

#if F_APP_LEGACY_DONGLE_BINDING || F_APP_LEA_DONGLE_BINDING

#if F_APP_LEA_DONGLE_BINDING
uint8_t app_dongle_lea_adv_get_conn_id(void)
{
    return dongle_lea_adv_conn_id;
}

void app_dongle_lea_adv_disconn_cback(uint8_t conn_id, uint8_t local_disc_cause,
                                      uint16_t disc_cause)
{
    APP_PRINT_TRACE2("app_dongle_lea_adv_disconn_cback: disc_cause 0x%x, cause 0x%x", local_disc_cause,
                     disc_cause);

    T_APP_LE_LINK *p_link;

    p_link = app_link_find_le_link_by_conn_id(conn_id);

    if (p_link)
    {
        app_lea_uca_link_sm(p_link->conn_handle, LEA_DISCONNECT, &disc_cause);

#if F_APP_GAMING_DONGLE_SUPPORT
        app_dongle_handle_ble_disconnected(p_link->bd_addr);
#endif
    }

    dongle_lea_adv_conn_id = 0xff;
}

void app_dongle_lea_adv_cback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;

    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));

    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            APP_PRINT_TRACE3("app_dongle_lea_adv_cback: state %d, adv_handle %d, stop_cause %d",
                             cb_data.p_ble_state_change->state,
                             cb_data.p_ble_state_change->adv_handle, cb_data.p_ble_state_change->stop_cause);
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        {
            APP_PRINT_TRACE2("app_dongle_lea_adv_cback: BLE_EXT_ADV_SET_CONN_INFO, adv_handle %d, conn_id 0x%x",
                             cb_data.p_ble_state_change->adv_handle, cb_data.p_ble_conn_info->conn_id);

            dongle_lea_adv_conn_id = cb_data.p_ble_conn_info->conn_id;

            app_link_reg_le_link_disc_cb(cb_data.p_ble_conn_info->conn_id, app_dongle_lea_adv_disconn_cback);
        }
        break;

    default:
        break;
    }
    return;
}

static void app_dongle_lea_adv_get(T_DONGLE_LEA_ADV_DATA *adv_data)
{
    adv_data->length = sizeof(T_DONGLE_LEA_ADV_DATA) - 1;
    adv_data->ad_type = 0xff;
    adv_data->ad_type_flag = 0x00;

    adv_data->pairing_id_LO = (uint8_t)(app_cfg_const.rws_custom_uuid);
    adv_data->pairing_id_HI = (uint8_t)((app_cfg_const.rws_custom_uuid & 0xFF00) >> 8);
    adv_data->pairing_bit = app_db.pairing_bit;
    adv_data->bud_side = app_cfg_const.bud_side;
    adv_data->is_stereo = (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE) ? 1 : 0;

    /* use the last 3 byte of mac of factory bud role primary as bud id */
    if (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
    {
        adv_data->pri_bud_mac[0] = app_db.factory_addr[0];
        adv_data->pri_bud_mac[1] = app_db.factory_addr[1];
        adv_data->pri_bud_mac[2] = app_db.factory_addr[2];
    }
    else
    {
        adv_data->pri_bud_mac[0] = app_cfg_nv.bud_peer_factory_addr[0];
        adv_data->pri_bud_mac[1] = app_cfg_nv.bud_peer_factory_addr[1];
        adv_data->pri_bud_mac[2] = app_cfg_nv.bud_peer_factory_addr[2];
    }

#if (TARGET_LE_AUDIO_GAMING == 1)
    adv_data->low_latency = true;

    memcpy(adv_data->public_addr, app_cfg_nv.bud_local_addr, 6);
#endif

#if F_APP_CSIS_SUPPORT
    uint8_t psri_data[CSI_RSI_LEN];
    if (csis_gen_rsi(app_cfg_nv.lea_sirk, psri_data))
    {
        memcpy(adv_data->psri_data, psri_data, CSI_RSI_LEN);
    }
#endif

    APP_PRINT_TRACE1("app_dongle_lea_adv_get: adv_data %b", TRACE_BINARY(sizeof(T_DONGLE_LEA_ADV_DATA),
                                                                         adv_data));
}

static void app_dongle_lea_dongle_adv_init()
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint16_t adv_interval = DONGLE_ADV_INTERVAL_NORMAL;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    app_dongle_lea_adv_get(&dongle_lea_adv);

    ble_ext_adv_mgr_init_adv_params(&lea_dongle_adv_handle,
                                    adv_event_prop,
                                    adv_interval,
                                    adv_interval,
                                    own_address_type,
                                    peer_address_type,
                                    peer_address,
                                    filter_policy,
                                    31, (uint8_t *)&dongle_lea_adv, 0, NULL, NULL);

    ble_ext_adv_mgr_change_adv_phy(lea_dongle_adv_handle, GAP_PHYS_PRIM_ADV_1M, GAP_PHYS_2M);
    ble_ext_adv_mgr_register_callback(app_dongle_lea_adv_cback, lea_dongle_adv_handle);
}
#endif

#if F_APP_LEGACY_DONGLE_BINDING
static void app_dongle_legacy_dongle_adv_init()
{
    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop =
        LE_EXT_ADV_LEGACY_ADV_NON_SCAN_NON_CONN_UNDIRECTED;
    uint16_t adv_interval = DONGLE_ADV_INTERVAL_NORMAL;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;

    ble_ext_adv_mgr_init_adv_params(&legacy_dongle_adv_handle,
                                    adv_event_prop,
                                    adv_interval,
                                    adv_interval,
                                    own_address_type,
                                    peer_address_type,
                                    peer_address,
                                    filter_policy,
                                    0, NULL, 0, NULL, NULL);

}

static void app_dongle_adv_get(T_LEGACY_DONGLE_BLE_DATA *adv_data)
{
    adv_data->length = sizeof(T_LEGACY_DONGLE_BLE_DATA) - 1;
    adv_data->type = 0xff;

    adv_data->eir_data.length = sizeof(T_DONGLE_EIR_DATA) - 1;
    adv_data->eir_data.type = 0xff;
    adv_data->eir_data.magic_data[0] = 0x5d;
    adv_data->eir_data.magic_data[1] = 0x00;
    adv_data->eir_data.magic_data[2] = 0x08;
    adv_data->eir_data.headset_type = (remote_session_role_get() == REMOTE_SESSION_ROLE_SINGLE) ? 0x02 :
                                      0x03;
    adv_data->eir_data.sbc_frame_num = 0;
    adv_data->eir_data.pairing_id_1 = (app_cfg_const.rws_custom_uuid >> 8) & 0xFF;
    adv_data->eir_data.pairing_id_2 = app_cfg_const.rws_custom_uuid & 0xFF;

    adv_data->eir_data.feature_set = app_cfg_const.spp_voice_smaple_rate & 0x03;
    adv_data->eir_data.feature_set |= 0 << 2;

#if F_APP_A2DP_CODEC_LC3_SUPPORT
    adv_data->eir_data.feature_set |= 1 << 3;
#endif

    APP_PRINT_TRACE1("app_dongle_adv_get: %b", TRACE_BINARY(sizeof(T_LEGACY_DONGLE_BLE_DATA),
                                                            adv_data));
}

uint8_t app_dongle_get_legacy_adv_handle(void)
{
    return legacy_dongle_adv_handle;
}
#endif

void app_dongle_adv_start(bool enable_pairing)
{
    uint8_t disallow_reason = 0;

    if (enable_pairing && dongle_pairing_non_intentionally)
    {
        APP_PRINT_TRACE0("app_dongle_adv_start: disallow non-intentionally dongle pairing");

        enable_pairing = false;
    }
    dongle_pairing_non_intentionally = false;

#if F_APP_LEA_DONGLE_BINDING
    if (app_dongle_get_le_audio_dongle_link() != NULL)
    {
        disallow_reason = 2;
        goto exit;
    }
#elif F_APP_LEGACY_DONGLE_BINDING
    if (app_dongle_get_connected_dongle_link() != NULL &&
        dongle_ctrl_data.switch_pairing_triggered == false)
    {
        disallow_reason = 3;
        goto exit;
    }
#endif

#if F_APP_GAMING_DONGLE_SUPPORT && F_APP_LEGACY_DONGLE_BINDING
    if (enable_pairing == 0)
    {
        if (app_cfg_const.enable_dongle_dual_mode)
        {
            if (app_cfg_const.enable_multi_link == false)
            {
                if (app_cfg_nv.dongle_rf_mode != DONGLE_RF_MODE_24G)
                {
                    disallow_reason = 4;
                    goto exit;
                }
            }
        }
    }
#endif

#if F_APP_DONGLE_MULTI_PAIRING
    if (app_cfg_const.enable_dongle_multi_pairing)
    {
        if (app_dongle_get_state() == DONGLE_STATE_PAIRING
            && app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G_BT
            && enable_pairing == 0)
        {
            disallow_reason = 5;
            goto exit;
        }
    }
#endif

    if (app_db.device_state != APP_DEVICE_STATE_ON)
    {
        disallow_reason = 6;
        goto exit;
    }

    if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_BT)
    {
        disallow_reason = 7;
        goto exit;
    }

#if F_APP_LEA_DONGLE_BINDING
    app_db.pairing_bit = enable_pairing;

    app_dongle_lea_adv_get(&dongle_lea_adv);

    uint8_t dongle_adv_random_addr[6];

    memcpy(dongle_adv_random_addr, app_cfg_nv.le_single_random_addr, 6);

    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        dongle_adv_random_addr[0] += 0xAA;
        dongle_adv_random_addr[1] += 0xAA;
        dongle_adv_random_addr[2] += 0xAA;
    }
    else
    {
        dongle_adv_random_addr[0] += 0xBB;
        dongle_adv_random_addr[1] += 0xBB;
        dongle_adv_random_addr[2] += 0xBB;
    }

    ble_ext_adv_mgr_set_random(lea_dongle_adv_handle, dongle_adv_random_addr);
    ble_ext_adv_mgr_set_adv_data(lea_dongle_adv_handle, sizeof(dongle_lea_adv),
                                 (uint8_t *)&dongle_lea_adv);
    ble_ext_adv_mgr_enable(lea_dongle_adv_handle, 0);
#elif F_APP_LEGACY_DONGLE_BINDING
    app_dongle_adv_get(&dongle_adv);
    dongle_adv.pairing_bit = enable_pairing;

    ble_ext_adv_mgr_set_adv_data(legacy_dongle_adv_handle,
                                 sizeof(dongle_adv), (uint8_t *)&dongle_adv);
    ble_ext_adv_mgr_enable(legacy_dongle_adv_handle, 0);
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
    dongle_ctrl_data.enable_pairing = enable_pairing;
#endif
exit:

#if F_APP_LEA_DONGLE_BINDING
    APP_PRINT_TRACE3("app_dongle_adv_start: pairing %d reason %d dongle_adv_random_addr %s",
                     enable_pairing, disallow_reason, TRACE_BDADDR(dongle_adv_random_addr));
#elif F_APP_LEGACY_DONGLE_BINDING
    APP_PRINT_TRACE2("app_dongle_adv_start: pairing %d reason %d", enable_pairing, disallow_reason);
#endif
}

void app_dongle_adv_stop(void)
{
    APP_PRINT_TRACE0("app_dongle_adv_stop");

#if F_APP_LEA_DONGLE_BINDING
    ble_ext_adv_mgr_disable(lea_dongle_adv_handle, 0);
#elif F_APP_LEGACY_DONGLE_BINDING
    ble_ext_adv_mgr_disable(legacy_dongle_adv_handle, 0);
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
    dongle_ctrl_data.enable_pairing = false;
#endif
}

uint8_t app_dongle_get_change_headset_adv_timer_idx(void)
{
    return timer_idx_change_headset_adv_interval;
}

static bool app_dongle_need_adv_when_power_on(void)
{
    bool ret = false;

#if F_APP_LEGACY_DONGLE_BINDING
    if (app_cfg_const.enable_multi_link)
    {
        ret = true;
    }
    else
    {
        if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_24G)
        {
            ret = true;
        }
    }
#endif

#if F_APP_LEA_DONGLE_BINDING
#if F_APP_B2B_ENGAGE_IMPROVE_BY_LEA_DONGLE
    /* don't start cis adv when power on, start after shaking timeout */
    ret = false;
#else
    /* always start cis adv when power on */
    ret = true;
#endif
#endif

    return ret;
}

#endif

static void app_dongle_dm_cback(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = false;

    switch (event_type)
    {
#if F_APP_LEGACY_DONGLE_BINDING || F_APP_LEA_DONGLE_BINDING
    case SYS_EVENT_POWER_ON:
        {
            if (app_dongle_need_adv_when_power_on())
            {
                app_dongle_adv_start(false);
            }

            app_start_timer(&timer_idx_change_headset_adv_interval, "change_headset_adv_interval",
                            app_common_dongle_timer_id, APP_TIMER_INCREASE_HEADSET_ADV_INTERVAL, 0, false,
                            HEADSET_ADV_STABLE_TIME);

#if F_APP_BLE_HID_CONTROLLER_SUPPORT
            if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_BT)
            {
                app_ble_controller_adv_start(false);
            }
#endif

            handle = true;
        }
        break;

    case SYS_EVENT_POWER_OFF:
        {
            app_dongle_adv_stop();

            handle = true;
        }
        break;
#endif

    default:
        break;
    }

    if (handle)
    {
        APP_PRINT_TRACE1("app_dongle_dm_cback: event 0x%04x", event_type);
    }
}

static void app_common_dongle_timer_cback(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
#if F_APP_LEGACY_DONGLE_BINDING || F_APP_LEA_DONGLE_BINDING
    case APP_TIMER_INCREASE_HEADSET_ADV_INTERVAL:
        {
            app_stop_timer(&timer_idx_change_headset_adv_interval);

            app_dongle_handle_heaset_adv_interval(ADV_INTERVAL_EVENT_TIMEOUT);
        }
        break;
#endif

    default:
        break;
    }
}

#if F_APP_LEGACY_DONGLE_BINDING || F_APP_LEA_DONGLE_BINDING
void app_dongle_handle_heaset_adv_interval(T_APP_HEADSET_ADV_EVENT event)
{
    static T_APP_HEADSET_ADV_STATE adv_state;
    T_APP_HEADSET_ADV_STATE old_adv_state = adv_state;
    bool long_adv_interval = false;
    uint16_t dongle_adv_interval = DONGLE_ADV_INTERVAL_NORMAL;
    uint16_t other_adv_interval = OTHER_ADV_INTERVAL_NORMAL;

    if (adv_state == HEADSET_ADV_STATE_INIT)
    {
        if (event == ADV_INTERVAL_EVENT_BT_STREAMING_START)
        {
            adv_state = HEADSET_ADV_STATE_BT_STREAMING;
        }
        else if (event == ADV_INTERVAL_EVENT_TIMEOUT)
        {
            adv_state = HEADSET_ADV_STATE_STABLE;
        }
    }
    else if (adv_state == HEADSET_ADV_STATE_BT_STREAMING)
    {
        if (event == ADV_INTERVAL_EVENT_BT_STREAMING_STOP)
        {
            adv_state = HEADSET_ADV_STATE_INIT;
        }
        else if (event == ADV_INTERVAL_EVENT_TIMEOUT)
        {
            adv_state = HEADSET_ADV_STATE_STABLE;
        }
    }

    if (adv_state == HEADSET_ADV_STATE_STABLE || adv_state == HEADSET_ADV_STATE_BT_STREAMING)
    {
        long_adv_interval = true;
    }

    if (long_adv_interval)
    {
        dongle_adv_interval = DONGLE_ADV_INTERVAL_LONG;
        other_adv_interval = OTHER_ADV_INTERVAL_LONG;
    }

    /* rtk common adv interval */
#if F_APP_GAMING_DONGLE_SUPPORT
    if (app_cfg_const.rtk_app_adv_support)
    {
        uint8_t le_common_adv_handle = app_ble_common_adv_get_adv_handle();
        ble_ext_adv_mgr_change_adv_interval(le_common_adv_handle, other_adv_interval);
    }
#endif

    /* dongle adv interval */
#if F_APP_LEA_DONGLE_BINDING || F_APP_LEGACY_DONGLE_BINDING
    uint8_t dongle_adv_handle = 0;

#if F_APP_LEA_DONGLE_BINDING
    dongle_adv_handle = lea_dongle_adv_handle;
#else
    dongle_adv_handle = legacy_dongle_adv_handle;
#endif

    ble_ext_adv_mgr_change_adv_interval(dongle_adv_handle, dongle_adv_interval);
#endif

    APP_PRINT_TRACE3("app_dongle_handle_heaset_adv_interval: event %d state (%d->%d)", event,
                     old_adv_state, adv_state);
}
#endif

void app_dongle_common_init(void)
{
    app_ipc_subscribe(APP_DEVICE_IPC_TOPIC, app_dongle_common_device_event_cback);

    sys_mgr_cback_register(app_dongle_dm_cback);

    app_timer_reg_cb(app_common_dongle_timer_cback, &app_common_dongle_timer_id);

#if F_APP_LEGACY_DONGLE_BINDING
    app_dongle_legacy_dongle_adv_init();
#endif
#if F_APP_LEA_DONGLE_BINDING
    app_dongle_lea_dongle_adv_init();
#endif
}
#endif
