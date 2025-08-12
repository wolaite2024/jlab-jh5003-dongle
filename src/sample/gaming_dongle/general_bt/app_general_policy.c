#include "trace.h"
#include "btm.h"
#include "app_link_util.h"
#include "bt_spp.h"
#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "bt_hfp_ag.h"
#include "bt_rfc.h"
#include "mem_types.h"
#include "os_mem.h"
#include "app_cfg.h"
#include "ftl.h"
//#include "dm.h"
#include "app_timer.h"
#include "app_general_policy.h"
#include "app_src_bond_storage.h"
#include "bt_bond.h"
#include "app_switch_detect.h"
#include "rtl876x_wdg.h"
//#include "app_usb_hid.h"
#include "app_sdp_a2dp_src.h"
#include "ual_bluetooth.h"
#include "app_src_storage_target_dev.h"
#include "app_src_audio.h"
#include "app_src_ble.h"
//#include "gap_mp.h"
#include "app_ctrl_cfg.h"
//#include "usb_lib_ext.h"
#include "app_led_ctl.h"
#include "app_vendor_cfg.h"
#include "app_cfu.h"
#include "app_spp_vender_passthrough.h"
#include "app_src_asp.h"
#include "gap_br.h"
#include "general_bt.h"
#include "app_usb_uac.h"
#include "app_usb_hid.h"
#include "app_device.h"

#define MS_SURFACE                  (1)
#define MS_RF_SCHEDULER_ENABLE      (0)

#ifdef BTDONGLE_BQB_MODE_ENABLE
#include "hogp_client.h"
#include "app_bqb_mode_switch.h"
#endif
#ifdef LEGACY_BT_GENERAL

extern bool teams_is_off;
extern bool isIncomingCall;
extern bool isOutgoingCall;
extern bool app_hfp_isInCall;
extern bool isMute;
extern bool plug_flag;
extern bool hs_is_mute;
extern T_BT_HFP_AG_CURRENT_CALL_STATUS curr_call_status;
bool hfp_is_connected = false;
static bool silent_detect_finished = false;
static bool non_teams_is_inCall = false; //wechat and so on

#define F_BT_LEGACY_APP_KEY_MANAGER         1

#define SINK_DEV_NUM_MAX                    1
#define RECONN_RETRY_COUNT_MAX              6
#define CONN_RETRY_COUNT_MAX                12

#define REMOTE_COD_MASK                     (0x000400)
#define REMOTE_RSSI_LEVEL_MIN               (-80)

#define FTL_SRC_MODE_OFFSET                 (4096)
#define FTL_SRC_MODE_SIZE                   (4)

#define AVDTP_STREAM_DATA_FLUSH_TIMEOUT     (100)

#define SRC_POWERON_DELAY_TIMEOUT           (2000)
#define SRC_PAIRING_MODE_TIMEOUT            (60000)
#define SRC_ACL_EXIST_DELAY_TIMEOUT         (2000)
#define SRC_A2DP_CONN_SETUP_DELAY_TIMEOUT   (10000)
#define SRC_2M_ACL_MAX_PAYLOAD_SIZE         (679)
#define SRC_2M_ACL_AVDTP_STREAM_OFFSET      (18)

#define SRC_STREAMING_IDLE_DETECT_TIMEOUT   (100)

#define SRC_STREAMING_IDLE_DETECT_MAX_COUNT (20)

#define SRC_PAIR_SUCCESS_REPORT_TIMEOUT     (2000)

#define SRC_SILENT_DETECT_TIMEOUT           (300)

#define SRC_SCO_CONN_SETUP_DELAY_TIMEOUT    (200)

#define SRC_ACL_SUPERVISION_TIMEOUT         (8000) //timeout = 0.625 * this marco
#define SRC_USB_STREAM_RESTART_TIMEOUT      (2500)

#define AUX_STATE_IN    0x01
#define AUX_STATE_OUT   0x02

#define SWITCH_STATE_SINK   0x01
#define SWITCH_STATE_SRC    0x02

#ifdef CUSTOMER_FIXED_SETTINGS
#define CUSTOMER_FIXED_SETTINGS_CONN_RETRY_COUNT_MAX    2
static bool poweron_autopair = false;
#endif

#define SRC_PROFILES_RETRY_COUNT        (10)

#define SRC_A2DP_STREAM_MAX_CREDITS            (8)

T_APP_DEV sink_dev_list[SINK_DEV_NUM_MAX];
T_APP_SRC_CUR_ACTIVE_INFO cur_active_dev_info;

static bool ota_mode = false;
static bool shutdown = false;

static uint8_t src_policy_timer_id = 0;
static uint8_t timer_idx_src_pairing_mode = 0;
static uint8_t timer_idx_src_poweron_delay = 0;
static uint8_t timer_idx_silent_detect = 0;
static uint8_t timer_idx_sco_conn_delay = 0;
static uint8_t timer_idx_usb_stream_restart = 0;

static uint8_t switch_state;
static bool a2dp_channel_ready = false;
static bool avrcp_is_connected = false;

static bool src_is_reconnecting = 0;

static uint8_t disconn_reason = 0;

static bool src_dev_ready = false;

#define SRC_ENABLE_AUDIO_US_PKT_LOGS_STAT

#ifdef SRC_ENABLE_AUDIO_US_PKT_LOGS_STAT
static bool pkt_loss_count_flag = false;
static uint32_t pkt_invalid = 0;
static uint32_t pkt_no_data = 0;
static uint32_t pkt_partial_lost = 0;
#endif

#ifdef BTDONGLE_BQB_MODE_ENABLE
static bool app_a2dp_opened = false;
#endif
const uint32_t src_supported_profiles[] =
{
    DEVICE_INFO_MASK,
#if 0
    MANUFACTURER_DATA_MASK,
#endif
    HFP_PROFILE_MASK,
    HSP_PROFILE_MASK,
    PBAP_PROFILE_MASK,
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    ASP_PROFILE_MASK,
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
    SPP_VENDER_PASSTHROUGH_PROFILE_MASK,
#endif
    AVRCP_PROFILE_MASK,
    A2DP_PROFILE_MASK,
    SPP_PROFILE_MASK,
    IAP_PROFILE_MASK,
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
    SPP_AUDIO_PROFILE_MASK,
#endif
};

const uint32_t TAR_CONN_PROFILES_MASK = A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
                                        | ASP_PROFILE_MASK
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
                                        | SPP_VENDER_PASSTHROUGH_PROFILE_MASK
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
                                        | SPP_AUDIO_PROFILE_MASK
#endif
                                        | HFP_PROFILE_MASK;

#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
const static uint8_t UUID_ASP[16] =
{
    0x54, 0x3c, 0xea, 0x22,
    0xad, 0x9c,
    0x42, 0x54,
    0xa4, 0x7e,
    0x5e, 0x1a, 0xe2, 0x5a, 0x40, 0xbd,
};
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
const static uint8_t UUID_VENDER_PASSTHROUGH[16] =
{
    0x12, 0xA2, 0x4D, 0x2E,
    0xFE, 0x14,
    0x48, 0x8e,
    0x93, 0xD2,
    0x17, 0x3C, 0x63, 0x01, 0x00, 0x00
};
#endif
#if 0
const static uint8_t UUID_MANUFACTURER_DATA[16] =
{
    0x40, 0xbf, 0xe9, 0x57,
    0x1d, 0x78,
    0x49, 0x6a,
    0x8b, 0x50,
    0x96, 0xd5, 0xe7, 0xc9, 0x64, 0x5f,
};
#endif
//#define A2DP_INTERVAL_PACKET

#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
const static uint8_t UUID_SPP_AUDIO[16] =
{
    0x12, 0xA2, 0x4D, 0x2E,
    0xFE, 0x14,
    0x48, 0x8e,
    0x93, 0xD2,
    0x17, 0x3C, 0x5A, 0x01, 0x00, 0x00
};
#endif

#ifdef AUDIO_DELAY_TEST_SCO_US
static bool first_sco_us_enc_pkt = true;
#endif

#ifdef AUDIO_DELAY_TEST_SCO_DS
static bool first_sco_ds_enc_pkt = true;
#endif

#ifdef AUDIO_DELAY_TEST_MUSIC
static bool first_music_enc_pkt = true;
#endif

typedef struct
{
    uint8_t *bd_addr;
    uint16_t   cause;
} T_BT_PARAM;

enum discovery_state
{
    DISCOVERY_STOPPED = 0,
    DISCOVERY_STARTING,
    DISCOVERY_FINDING,
    DISCOVERY_STOPPING,
};

extern uint16_t player_id;

//static APP_SRC_AUDIO_CONN_RESULT_CB src_pair_state_cb = NULL;
static APP_SRC_PAIR_RESULT_CB src_pair_state = NULL;

//only true when Pair2Dev, false when IDLE or pair finished
static bool report_pair_state = false;

static bool src_start_disconn(uint8_t *bd_addr);
//static uint32_t get_profile_mask_from_bond_flag(uint16_t bond_flag);
static uint32_t get_bond_flag_from_profile_mask(uint32_t profile_mask);

static void src_dev_conn(uint8_t index, uint8_t *bd_addr, uint32_t profile_mask);
static void src_dev_profile_conn(uint8_t index);
static bool src_profile_conn(uint8_t *bd_addr, uint32_t profile_mask, T_APP_SRC_SDP_INFO sdp_info);

static void src_profile_conn_finish(uint8_t index);
static void add_conneted_profile(uint8_t *bd_addr, uint32_t profile_mask);
static void del_connected_profile(uint8_t *bd_addr, uint32_t profile_mask);
static void src_profile_disconn(uint8_t *bd_addr, uint32_t profile_mask);
static bool get_connected_profile(uint8_t *bd_addr, uint32_t *profile_masks);

static void src_start_reconn(uint8_t index);
static void src_start_conn(uint8_t index);
static void src_handle_dev_pending_evt(uint8_t index, T_APP_SRC_KEY_EVENT pending_evt);

//static bool src_is_aux_in(void);
//extern bool bt_a2dp_stream_send(uint8_t *bd_addr, uint8_t *p_data, uint16_t len, uint8_t frame_no);

static uint8_t discov_state;
static uint8_t client_discovering;

#ifdef SRC_ENABLE_AUDIO_US_PKT_LOGS_STAT
static void src_enable_audio_us_pkt_loss_count(bool flag);
#endif

extern uint8_t app_get_cur_bt_mode(void);

void app_a2dp_src_key_cfg_init(void)
{
    app_cfg_const.key_table[0][0][KEY1] = 0xf3;
    app_cfg_const.key_table[1][0][KEY1] = 0xf2;
    app_cfg_const.key_table[0][0][KEY2] = 0xf5;
    app_cfg_const.key_table[1][0][KEY2] = 0xf4;

    app_cfg_const.hybrid_key_mapping[HY_KEY7][0] = 0x02;    //key1 only
    app_cfg_const.hybrid_key_mapping[HY_KEY7][1] = 0x03;    //multi click 2 times
    app_cfg_const.hybrid_key_table[0][HY_KEY7] = 0xf6;      //event
}

void app_load_role(void)
{
//    switch_state = app_aux_switch_state_get();

    APP_PRINT_INFO1("app_load_role: %d", switch_state);
}

bool app_a2dp_is_src(void)
{
#ifdef CUSTOMER_FIXED_SETTINGS
    return true;
#else
    if (switch_state == SWITCH_STATE_SRC)
    {
        return true;
    }
    else
    {
        return false;
    }
#endif
}

bool app_a2dp_role_switch_needed(void)
{
#if 0
    if (app_aux_switch_state_get() != switch_state)
    {
        return true;
    }
    else
    {
        return false;
    }
#endif
    return false;
}

static bool src_is_busy(void)
{
    return cur_active_dev_info.is_busy;
}

static bool src_is_cur_dev(uint8_t index)
{
    if (!src_is_busy())
    {
        return false;
    }
    else
    {
        return (index == cur_active_dev_info.index);
    }
}

static void src_set_active_sink_dev(uint8_t index)
{
    cur_active_dev_info.index = index;
    cur_active_dev_info.is_busy = true;
}

static void src_dev_deactivate(void)
{
    cur_active_dev_info.is_busy = false;
}

static bool src_dev_all_idle(void)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].dev_state != STATE_DEV_IDLE)
        {
            break;
        }
    }

    if (i == SINK_DEV_NUM_MAX)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void src_check_product_role_switch(void)
{
    if (app_a2dp_role_switch_needed() && src_dev_all_idle())
    {
        //need to check shutdown ?
        app_cfg_nv.app_is_power_on = 1;
        app_cfg_store();
        WDG_SystemReset(RESET_ALL);
    }
}

bool src_policy_find_dev_index_by_addr(uint8_t *bd_addr, uint8_t *index)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (!memcmp(bd_addr, sink_dev_list[i].remote_info.bd_addr, 6))
        {
            *index = i;
            return true;
        }
    }

    return false;
}

static void src_dev_cleanup(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_dev_cleanup: invalid index %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    //free buffer
    if (sink_dev->stream_info.buffer != NULL)
    {
        os_mem_free(sink_dev->stream_info.buffer);
        sink_dev->stream_info.buffer = NULL;
    }

    //stop timer
    app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
    app_stop_timer(&sink_dev->timer_idx_pair_suc_report_delay);
    app_stop_timer(&sink_dev->timer_idx_stream_idle);
    app_stop_timer(&sink_dev->timer_idx_a2dp_conn_setup_delay);

    memset(sink_dev, 0, sizeof(T_APP_DEV));
}

static void src_dev_cleanup_restore_pending_info(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;
    T_APP_PENDING_INFO pending_info;
    bool in_use = false;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_dev_cleanup_restore_pending_info: invalid index %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    in_use = sink_dev->in_use;
    pending_info.cod = sink_dev->pending_info.cod;
    memcpy(pending_info.remote_bd, sink_dev->pending_info.remote_bd, 6);

    src_dev_cleanup(index);

    sink_dev->in_use = in_use;
    sink_dev->pending_info.cod = pending_info.cod;
    memcpy(sink_dev->pending_info.remote_bd, pending_info.remote_bd, 6);
}

static void src_dev_cleanup_restore_dev_info(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t remote_bd[6];
    uint32_t cod;
    bool in_use = false;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_dev_cleanup_restore_dev_info: invalid index %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    in_use = sink_dev->in_use;
    cod = sink_dev->remote_info.cod;
    memcpy(remote_bd, sink_dev->remote_info.bd_addr, 6);

    src_dev_cleanup(index);

    sink_dev->in_use = in_use;
    sink_dev->remote_info.cod = cod;
    memcpy(sink_dev->remote_info.bd_addr, remote_bd, 6);
}


#if F_BT_LEGACY_APP_KEY_MANAGER
bool src_policy_find_dev_index_by_bond_info(uint8_t *bd_addr, uint8_t *index)
{
    uint8_t i;

    if (app_src_legacy_get_paired_idx(bd_addr, &i))
    {
        *index = i;
        return true;
    }
    else
    {
        return false;
    }
}
#endif

static void  src_dev_state_set(uint8_t index, uint16_t dev_state)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_dev_state_set: invalid index %d", index);
        return ;
    }

    sink_dev = &sink_dev_list[index];

    if (dev_state >= STATE_DEV_MAX)
    {
        APP_PRINT_WARN1("src_dev_state_set: invalid state 0x%04x", dev_state);
        return ;
    }

    APP_PRINT_INFO2("src_dev_state_set: index %d, dev_state %d", index, dev_state);

    sink_dev->dev_state = dev_state;
}

static void src_bt_state_set(uint8_t index, uint16_t bt_state)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_bt_state_set: invalid index %d", index);
        return ;
    }

    sink_dev = &sink_dev_list[index];

    if (bt_state >= STATE_BT_MAX)
    {
        APP_PRINT_WARN1("src_bt_state_set: invalid state 0x%04x", bt_state);
        return ;
    }

    APP_PRINT_INFO2("src_bt_state_set: index %d, bt_state %d", index, bt_state);

    sink_dev->bt_state = bt_state;
}

static void src_led_set_hfp_disconnected()
{
    hfp_is_connected = false;
    app_stop_flash_on_and_off_timer();
    app_stop_pulse_white_timer();
    if (app_get_cur_bt_mode() == 0)
    {
#if (FUNC_DEBUG_DRAFT == 0)
        app_led_set_mode(APP_LED_MODE_ALL_OFF);
        app_led_set_mode(APP_LED_MODE_BT_LEGACY);
#endif
    }
}

static void src_led_set_idle(uint8_t index)
{
    APP_PRINT_INFO1("src_led_set_idle: %d", index);

    if (index == 0)
    {
        //app_led_set_mode(APP_LED_MODE_LINK1_STANDBY);
    }
    else if (index == 1)
    {
        //app_led_set_mode(APP_LED_MODE_LINK2_STANDBY);
    }
}

static void src_led_set_reconnecting(uint8_t index)
{
    APP_PRINT_INFO1("src_led_set_reconnecting: %d", index);

    if (index == 0)
    {
        //app_led_set_mode(APP_LED_MODE_LINK1_RECONNECTTING);
    }
    else if (index == 1)
    {
        //app_led_set_mode(APP_LED_MODE_LINK2_RECONNECTTING);
    }
}

static void src_led_set_connecting(uint8_t index)
{
    APP_PRINT_INFO1("src_led_set_connecting: %d", index);

    if (index == 0)
    {
        app_led_set_mode(APP_LED_MODE_PARING);
    }
    else if (index == 1)
    {
        app_led_set_mode(APP_LED_MODE_PARING);
    }
}

static void src_led_set_hfp_connected(uint8_t *bd_addr)
{
    hfp_is_connected = true;

    //app_stop_flash_on_and_off_timer();
    T_APP_DEV *sink_dev = &sink_dev_list[0];
    app_stop_pulse_white_timer();
    APP_PRINT_TRACE4("src_led_set_hfp_connected, isIncomingCall: %d, isOutgoingCall:%d, app_hfp_isInCall: %d, isMute: %d",
                     isIncomingCall, isOutgoingCall, app_hfp_isInCall, isMute);
    if (app_hfp_isInCall && !plug_flag)
    {
        if (curr_call_status == BT_HFP_AG_CURRENT_CALL_STATUS_HELD)
        {
            app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
        }
        else
        {
            if (sink_dev->sco_state == SCO_SETUP_STATE_IDLE)
            {
                sink_dev->sco_state = SCO_SETUP_STATE_CONNECTING;
                bt_hfp_ag_audio_connect_req(bd_addr);
            }
            if (isMute)
            {
                bt_hfp_ag_microphone_gain_set(bd_addr, 0);
                hs_is_mute = true;
                app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
            }
            else
            {
                bt_hfp_ag_microphone_gain_set(bd_addr, 15);
                hs_is_mute = false;
                app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
            }
        }
    }
    else if (app_hfp_isInCall)
    {
        if (isMute)
        {
            app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
        }
        else
        {
            app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
        }
    }
    else if (isIncomingCall)
    {
        bt_hfp_ag_call_setup_indicator_send(bd_addr, BT_HFP_AG_CALL_SETUP_STATUS_INCOMING_CALL);
        if (sink_dev->sco_state == SCO_SETUP_STATE_IDLE)
        {
            sink_dev->sco_state = SCO_SETUP_STATE_CONNECTING;
            bt_hfp_ag_audio_connect_req(bd_addr);
        }
        app_led_set_mode(APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS);
    }
    else if (isOutgoingCall)
    {
        bt_hfp_ag_call_setup_indicator_send(bd_addr, BT_HFP_AG_CALL_SETUP_STATUS_OUTGOING_CALL);
        if (sink_dev->sco_state == SCO_SETUP_STATE_IDLE)
        {
            sink_dev->sco_state = SCO_SETUP_STATE_CONNECTING;
            bt_hfp_ag_audio_connect_req(bd_addr);
        }
        app_led_set_mode(
            APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT);
    }
    else
    {
        app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
    }
}

static void src_led_set_connected(uint8_t index)
{
    APP_PRINT_INFO1("src_led_set_connected: %d", index);

    if (index == 0)
    {
        //app_led_set_mode(APP_LED_MODE_LINK1_CONNECTED);
    }
    else if (index == 1)
    {
        //app_led_set_mode(APP_LED_MODE_LINK2_CONNECTED);
    }
}

static void src_enter_reconnecting_state(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_enter_reconnecting_state: invalid index %d", index);
        return ;
    }

    sink_dev = &sink_dev_list[index];
#if 0
    memset(sink_dev, 0, sizeof(T_APP_DEV));
#endif
    src_dev_state_set(index, STATE_DEV_RECONNECTING);
    src_bt_state_set(index, STATE_BT_IDLE);

    sink_dev->retry_count = RECONN_RETRY_COUNT_MAX;
    sink_dev->in_use = true;

    //set led here
    src_led_set_reconnecting(index);
    app_led_set_mode(APP_LED_MODE_PARING);
}

static void src_enter_connecting_state(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_enter_connecting_state: invalid index %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    src_dev_cleanup(index);

    sink_dev->in_use = true;
    src_dev_state_set(index, STATE_DEV_CONNECTING);
    src_bt_state_set(index, STATE_BT_IDLE);

#ifdef CUSTOMER_FIXED_SETTINGS
    if (poweron_autopair)
    {
        sink_dev->retry_count = CUSTOMER_FIXED_SETTINGS_CONN_RETRY_COUNT_MAX;
    }
    else
    {
        sink_dev->retry_count = CONN_RETRY_COUNT_MAX;
    }
#else
    sink_dev->retry_count = CONN_RETRY_COUNT_MAX;
#endif

    //set led here
    src_led_set_connecting(index);
}

static bool src_dev_is_bonded(uint8_t index)
{
    uint32_t bond_flag = 0;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_dev_is_bonded: index %d is invalid", index);
        return false;
    }
#if F_BT_LEGACY_APP_KEY_MANAGER
    bond_flag = app_src_legacy_get_bond_flag_by_index(index);

    if (bond_flag & APP_SRC_BOND_FLAG_A2DP)
#else
    if (bt_bond_flag_get(sink_dev_list[index].remote_info.bd_addr, &bond_flag) &&
        (bond_flag & APP_SRC_BOND_FLAG_A2DP))
#endif
    {
        return true;
    }

    return false;
}

#if 0
static uint8_t src_get_bond_no_conn_num(void)
{
    uint8_t i = 0;
    uint8_t num = 0;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (src_dev_is_bonded(i) &&
            (sink_dev_list[i].bt_state != STATE_BT_ACL_CONNECTED)
            && (sink_dev_list[i].dev_state != STATE_DEV_CONNECTING))
        {
            num++;
        }
    }

    return num;
}
#endif

static uint8_t src_get_target_no_conn_num(void)
{
    uint8_t i = 0;
    uint8_t num = 0;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if ((sink_dev_list[i].in_use) &&
            (sink_dev_list[i].bt_state != STATE_BT_ACL_CONNECTED)
            && (sink_dev_list[i].dev_state != STATE_DEV_CONNECTING)
            && (sink_dev_list[i].dev_state != STATE_DEV_RECONNECTING))
        {
            num++;
        }
    }

    return num;
}


static void src_set_bt_mode(T_BT_DEVICE_MODE mode)
{
    static T_BT_DEVICE_MODE cur_mode = BT_DEVICE_MODE_IDLE;

    if (shutdown)
    {
        mode = BT_DEVICE_MODE_IDLE;
    }
    else if (ota_mode)
    {
        mode = BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE;
    }

    if (cur_mode != mode)
    {
        cur_mode = mode;

        bt_device_mode_set(cur_mode);
    }
}

static void src_check_set_bt_mode(void)
{
#if 0
    if (src_get_bond_no_conn_num() > 0)
#else
    if (src_get_target_no_conn_num() > 0)
#endif
    {
        src_set_bt_mode(BT_DEVICE_MODE_CONNECTABLE);
    }
    else if (src_is_reconnecting)
    {
        src_set_bt_mode(BT_DEVICE_MODE_CONNECTABLE);
    }
    else
    {
        src_set_bt_mode(BT_DEVICE_MODE_IDLE);
    }
}

#ifdef BTDONGLE_BQB_MODE_ENABLE
static uint8_t timer_idx_src_bqb_ringbuf = 0;

void bqb_ringbuf_send_start_timer(void)
{
    app_start_timer(&timer_idx_src_bqb_ringbuf, "src_bqb_ringbuf",
                    src_policy_timer_id, SRC_TIMER_ID_BQB_USB_SET_REPORT, 0, false,
                    2);
}
#endif

void src_enter_pairing_mode(void)
{
    APP_PRINT_INFO0("src_enter_pairing_mode");
    ota_mode = true;
    src_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);

    app_start_timer(&timer_idx_src_pairing_mode, "src_pairing_mode",
                    src_policy_timer_id, SRC_TIMER_ID_PAIRING_MODE, 0, false,
                    SRC_PAIRING_MODE_TIMEOUT);

    //app_led_set_mode(APP_LED_MODE_OTA_PAIRING);
}

static void src_schedule_exit(void)
{
    src_check_set_bt_mode();
}

static void src_dev_schedule(void)
{
    uint8_t i, j;
    T_APP_DEV *sink_dev = NULL;

#if SINK_DEV_NUM_MAX == 1
    for (i = cur_active_dev_info.index, j = 0;
         j < SINK_DEV_NUM_MAX; i = 0, j++)
#else
    for (i = cur_active_dev_info.index, j = 0;
         j < SINK_DEV_NUM_MAX;
         i = (i + 1) % SINK_DEV_NUM_MAX, j++)
#endif
    {
        sink_dev = &sink_dev_list[i];

        APP_PRINT_INFO3("src_dev_schedule: index %d, dev_state %02x, bt_state %02x",
                        i, sink_dev->dev_state, sink_dev->bt_state);

        if ((sink_dev->dev_state == STATE_DEV_RECONNECTING) &&
            (sink_dev->retry_count > 0))
        {
#if 0
            if (src_dev_is_bonded(i))
            {
                src_start_reconn(i);
                break;
            }
            else
            {
                //no bonded device, exit
                memset(sink_dev, 0, sizeof(T_APP_DEV));
                src_led_set_idle(i);
            }
#else
            src_start_reconn(i);
#endif
        }
        else if ((sink_dev->dev_state == STATE_DEV_CONNECTING) &&
                 (sink_dev->retry_count > 0))
        {
            src_start_conn(i);
            break;
        }
    }

    if (j == SINK_DEV_NUM_MAX)
    {
        //no dev to be schduled
        src_schedule_exit();
    }
}

static void src_dev_schedule_next(void)
{
#if SINK_DEV_NUM_MAX == 1
    cur_active_dev_info.index = 0;
#else
    cur_active_dev_info.index++;
    cur_active_dev_info.index = cur_active_dev_info.index % SINK_DEV_NUM_MAX;
#endif
    src_dev_schedule();
}

static void src_save_target_audio_dev(uint8_t index)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_SRC_TARGET_DEV save_dev;
    uint8_t temp_len;

    memset(&save_dev, 0, sizeof(T_APP_SRC_TARGET_DEV));

    save_dev.dev_type = DEV_SLOT_HS;
    save_dev.bt_mode = DEV_MODE_LEGACY;
    save_dev.addr_type = 0x00;

    save_dev.ms_dev_type = sink_dev->remote_info.cod;
    save_dev.vid = sink_dev->remote_info.vid;
    save_dev.pid = sink_dev->remote_info.pid;

    memcpy(save_dev.remote_bd, sink_dev->remote_info.bd_addr, 6);

    temp_len = strlen((const char *)sink_dev->remote_info.hwid);
    if (temp_len >= HWID_LEN_MAX)
    {
        temp_len = HWID_LEN_MAX - 1;
    }
    memcpy(save_dev.hardware_id, sink_dev->remote_info.hwid, temp_len);

    temp_len = strlen((const char *)sink_dev->remote_info.name);
    if (temp_len >= GAP_DEVICE_NAME_LEN)
    {
        temp_len = GAP_DEVICE_NAME_LEN - 1;
    }
    memcpy(save_dev.remote_name, sink_dev->remote_info.name, temp_len);

    save_dev.is_valid = 1;

    app_src_storage_set_target_dev(save_dev);
}

//NOTE: only handle connect cmd result, report_pair_state is true
static void src_handle_pair_result(uint8_t index, bool is_success)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_SRC_TARGET_DEV old_dev;
    bool old_dev_valid = false;

    old_dev_valid = app_src_storage_get_target_dev(TARGET_DEVICE_TYPE_HEADSET, &old_dev);

    if (is_success)
    {
        if (memcmp(sink_dev->remote_info.bd_addr, old_dev.remote_bd, 6))
        {
            bt_bond_delete(old_dev.remote_bd);
        }

        //src_save_target_audio_dev(index);
    }
    else
    {
        if (!old_dev_valid)
        {
            bt_bond_delete(sink_dev->remote_info.bd_addr);
            src_dev_cleanup(index);
        }
        else if (memcmp(sink_dev->remote_info.bd_addr, old_dev.remote_bd, 6))
        {
            bt_bond_delete(sink_dev->remote_info.bd_addr);
            src_dev_cleanup(index);

            memcpy(sink_dev->remote_info.bd_addr, old_dev.remote_bd, 6);
            sink_dev->remote_info.cod = old_dev.ms_dev_type;
            sink_dev->in_use = true;
        }
        else //new device is old device
        {
            src_dev_cleanup_restore_dev_info(index);
        }
    }
}

//autopair/reconn result
static void src_handle_action_result(uint8_t index, bool conn_ok)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    uint16_t temp_dev_state;
    uint8_t temp_retry_count;
    uint32_t bond_flag;
    T_APP_BR_LINK *p_link;

    APP_PRINT_INFO4("src_handle_action_result: index %d, conn_ok %d, dev_state %02x, retry_count %d",
                    index, conn_ok, sink_dev->dev_state, sink_dev->retry_count);

    app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);

    p_link = app_find_br_link(sink_dev->remote_info.bd_addr);

    if (conn_ok)
    {
        APP_PRINT_INFO3("src_handle_action_result: cod %08x, name %s, hwid %s",
                        sink_dev->remote_info.cod,
                        TRACE_STRING(sink_dev->remote_info.name),
                        TRACE_STRING(sink_dev->remote_info.hwid));

        src_is_reconnecting = false;

        src_dev_state_set(index, STATE_DEV_CONNECTED);
        sink_dev->retry_count = 0;
        sink_dev->inquiry_dev_found = 0;

        src_led_set_connected(index);
        app_led_set_mode(APP_LED_MODE_CONNECTED);

        //signal channel connected, no discover cmd received
        //start avdtp discover by open api
        if (p_link)
        {
            //triger avdtp discover if a2dp connected from remote
            if ((p_link->connected_profile & A2DP_PROFILE_MASK) &&
                (!sink_dev->a2dp_codec_configured) &&
                sink_dev->a2dp_connected_from_remote)
            {
                bt_a2dp_stream_open_req(sink_dev->remote_info.bd_addr, BT_A2DP_ROLE_SNK);
            }

            APP_PRINT_INFO4("src_handle_action_result: p_link->connected_profile: 0x%08x, avrcp_conn %d, stream_state %d, is_streaming %d",
                            p_link->connected_profile, avrcp_is_connected,
                            sink_dev->stream_info.stream_state,
                            app_get_usb_ds_state());
            if (app_usb_audio_is_us_streaming() &&
                (p_link->connected_profile & HFP_PROFILE_MASK) &&
                (app_cfg_const.dongle_media_device == 0))
            {
                app_notify_policy_us_audio_status(true);
            }
            else if ((p_link->connected_profile & A2DP_PROFILE_MASK) && avrcp_is_connected &&
                     (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_IDLE) &&
                     (app_get_usb_ds_state() || app_cfg_const.dongle_media_device))
            {
                if (a2dp_channel_ready)
                {
                    bt_a2dp_stream_start_req(sink_dev->remote_info.bd_addr);
                    app_stop_timer(&timer_idx_usb_stream_restart);
                    sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STARTING;
                    bt_sniff_mode_disable(sink_dev->remote_info.bd_addr);
                }
            }
            else
            {
                //currently no audio streaming, enable sniff mode
                bt_sniff_mode_enable(sink_dev->remote_info.bd_addr, 784, 816, 0, 0);
            }

            src_save_target_audio_dev(index);

            if ((src_pair_state != NULL) && report_pair_state)
            {
                src_handle_pair_result(index, true);

                app_start_timer(&sink_dev->timer_idx_pair_suc_report_delay, "pair_suc_report_delay",
                                src_policy_timer_id, SRC_TIMER_ID_PAIR_SUC_REPORT_DELAY_SINK0 + index, 0, false,
                                SRC_PAIR_SUCCESS_REPORT_TIMEOUT);
            }

//            app_usb_hid_connection_state_notify(DEV_SLOT_HS,
//                                                BT_CONN_STATE_CONNECTED,
//                                                src_get_audio_device_profile_state());
            app_bt_connection_state_report_event(BT_CONN_STATE_CONNECTED, src_get_audio_device_profile_state());
        }
        else
        {
            APP_PRINT_ERROR0("src_handle_action_result: p_link is NULL!");
        }

#if F_BT_LEGACY_APP_KEY_MANAGER
        bond_flag = app_src_legacy_get_bond_flag_by_index(index);
        if (bond_flag & APP_SRC_BOND_FLAG_HFP)
#else
        if (bt_bond_flag_get(sink_dev->remote_info.bd_addr, &bond_flag) &&
            (bond_flag & APP_SRC_BOND_FLAG_HFP))
#endif
        {
            //bt_hfp_ag_audio_connect_req(sink_dev->remote_info.bd_addr);
        }
#ifdef CUSTOMER_FIXED_SETTINGS
        if (poweron_autopair)
        {
            poweron_autopair = false;
        }
#endif
    }
    else
    {
        sink_dev->retry_count--;

        if (sink_dev->retry_count <= 0)
        {
            src_is_reconnecting = false;

            src_led_set_idle(index);
#if 0
#ifdef CUSTOMER_FIXED_SETTINGS
            if (poweron_autopair)
            {
                poweron_autopair = false;

                uint8_t i;
                for (i = 0; i < SINK_DEV_NUM_MAX; i++)
                {
                    src_led_set_idle(i);
                    key_short_press_handle(i);
                }
            }
#endif
#endif
            if ((src_pair_state != NULL) && report_pair_state)
            {
                src_handle_pair_result(index, false);
                src_pair_state(sink_dev->remote_info.bd_addr, PAIR_STATE_FAIL);
                report_pair_state = false;
            }
            else
            {
                src_dev_cleanup_restore_dev_info(index);
            }
        }
        else
        {
            //retry later, keep dev_state & retry_count only
            APP_PRINT_INFO4("src_handle_action_result: index %d, conn_ok %d, dev_state %02x, retry_count %d",
                            index, conn_ok, sink_dev->dev_state, sink_dev->retry_count);


            temp_dev_state = sink_dev->dev_state;
            temp_retry_count = sink_dev->retry_count;

            src_dev_cleanup_restore_dev_info(index);

            src_dev_state_set(index, temp_dev_state);
            sink_dev->retry_count = temp_retry_count;
        }
    }

    src_dev_deactivate();
    src_dev_schedule_next();
}

/*
static bool load_bond_info(uint8_t index)
{
    uint8_t bd_addr[6] = {0};
    uint8_t bond_flag = 0;
    T_APP_DEV* sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("load_bond_info: index %d is invalid", index);
        return false;
    }

    sink_dev = &sink_dev_list[index];
    bond_flag = app_src_legacy_get_bond_flag_by_index(index);

    if ((bond_flag & APP_SRC_BOND_FLAG_A2DP) && app_src_legacy_get_bond_addr_by_index(index, bd_addr))
    {
        memcpy(sink_dev->remote_info.bd_addr, bd_addr, 6);
        sink_dev->profile_mask_plan = get_profile_mask_from_bond_flag(bond_flag);
        return true;
    }

    return false;
}
*/

static bool src_get_inquiry_dev_index(uint8_t *index)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].bt_state == STATE_BT_INQUIRY)
        {
            break;
        }
    }

    if (i < SINK_DEV_NUM_MAX)
    {
        *index = i;
        return true;
    }
    else
    {
        return false;
    }
}

static void src_handle_inquiry_rsp(uint16_t cause)
{
    uint8_t index = 0;

    if (src_get_inquiry_dev_index(&index))
    {

        APP_PRINT_INFO2("src_handle_inquiry_rsp: cause %d, dev_state 0x%04x",
                        cause, sink_dev_list[index].dev_state);

        if (cause)
        {
            src_handle_action_result(index, false);
        }
    }
}

static void src_handle_inquiry_res(T_BT_EVENT_PARAM_INQUIRY_RESULT inquiry_result)
{
    uint8_t index = 0;
    T_APP_DEV *sink_dev;

#ifdef BTDONGLE_BQB_MODE_ENABLE
    extern bool bqb_open_inquiry;
    if (app_is_bqb_mode() && bqb_open_inquiry)
    {
        extern uint8_t pts_addr[6];
        if (!memcmp(pts_addr, inquiry_result.bd_addr, 6))
        {
            extern void app_bqb_ring_usb_send(uint8_t *buf, uint16_t size);
            char buf[30];
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "pts %02x %02x %02x %02x %02x %02x found",
                    pts_addr[5], pts_addr[4], pts_addr[3], pts_addr[2], pts_addr[1], pts_addr[0]);
            app_bqb_ring_usb_send((uint8_t *)buf, strlen(buf));
            gap_br_stop_inquiry();
            bqb_open_inquiry = false;
        }
    }
#endif

    if (src_get_inquiry_dev_index(&index))
    {
        sink_dev = &sink_dev_list[index];
        APP_PRINT_INFO5("sink_dev->inquiry_dev_found %d, dev_state %d, bd_addr %b, cod %x, rssi %d",
                        sink_dev->inquiry_dev_found, sink_dev->dev_state,
                        TRACE_BDADDR(inquiry_result.bd_addr), inquiry_result.cod, inquiry_result.rssi);
        if ((!sink_dev->inquiry_dev_found) &&
            (sink_dev->dev_state == STATE_DEV_CONNECTING))
        {
            if ((inquiry_result.cod & REMOTE_COD_MASK) &&
                (inquiry_result.rssi >= app_cfg_const.dongle_autopair_rssi))
            {

                if (app_find_br_link(inquiry_result.bd_addr) == NULL)
                {
                    memcpy(sink_dev->remote_info.bd_addr, inquiry_result.bd_addr, 6);
                    sink_dev->remote_info.cod = inquiry_result.cod;
                    memcpy(sink_dev->remote_info.name, inquiry_result.name, GAP_DEVICE_NAME_LEN);
                    sink_dev->inquiry_dev_found = true;

                    //wait for inquiry cancel rsp
                    gap_br_stop_inquiry();
                    app_scan_state_change_event(LEGACY_SCAN_OFF);
                    //src_dev_conn(index, inquiry_result.bd_addr, A2DP_PROFILE_MASK | SPP_PROFILE_MASK);
                }
                else
                {
                    APP_PRINT_TRACE1("src_handle_inquiry_res: %s already connected",
                                     TRACE_BDADDR(inquiry_result.bd_addr));
                }
            }
        }
        else
        {
            //ignore
        }
    }
}

static void src_handle_inquiry_cmpl(uint16_t cause)
{
    uint8_t index = 0;

    if (src_get_inquiry_dev_index(&index))
    {
        APP_PRINT_INFO2("src_handle_inquiry_cmpl: cause 0x%04x, dev_state %d",
                        cause, sink_dev_list[index].dev_state);

        if (sink_dev_list[index].dev_state == STATE_DEV_CONNECTING)
        {
            src_handle_action_result(index, false);
        }
    }
}

static void src_handle_inquiry_cancel_cmpl(uint16_t cause)
{
    uint8_t index = 0;
    T_APP_DEV *sink_dev = NULL;
    T_APP_SRC_KEY_EVENT pending_evt = KEY_NON;

    if (src_get_inquiry_dev_index(&index))
    {
        sink_dev = &sink_dev_list[index];
        pending_evt = sink_dev->pending_event;

        switch (sink_dev->dev_state)
        {
        case STATE_DEV_CONNECTING:
            {
                if (sink_dev->inquiry_dev_found)
                {
#ifdef BTDONGLE_BQB_MODE_ENABLE
                    if (app_is_bqb_mode())
                    {
                        src_dev_conn(index, sink_dev->remote_info.bd_addr,
                                     A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK |
                                     HFP_PROFILE_MASK);
                    }
                    else
#endif
                    {
                        src_dev_conn(index, sink_dev->remote_info.bd_addr,
                                     A2DP_PROFILE_MASK | HFP_PROFILE_MASK |
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
                                     ASP_PROFILE_MASK |
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
                                     SPP_AUDIO_PROFILE_MASK |
#endif
#if 0
                                     MANUFACTURER_DATA_MASK |
#endif
                                     DEVICE_INFO_MASK);
                    }
                }
            }
            break;

        case STATE_DEV_CLEANUP:
            {
                src_handle_dev_pending_evt(index, pending_evt);
            }
            break;

        default:
            break;
        }
    }
}

static void src_handle_acl_conn_req(uint8_t *bd_addr)
{
    uint8_t index;

    for (index = 0; index < SINK_DEV_NUM_MAX; index++)
    {
        APP_PRINT_INFO2("src_handle_acl_conn_req: %s %d",
                        TRACE_BDADDR(sink_dev_list[index].remote_info.bd_addr),
                        sink_dev_list[index].dev_state);
    }

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_MASTER);
        //bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_SLAVE);
        app_stop_timer(&sink_dev_list[0].timer_idx_acl_exist_delay);
    }
#if F_BT_LEGACY_APP_KEY_MANAGER
    else if (src_policy_find_dev_index_by_bond_info(bd_addr, &index))
    {
        bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_MASTER);
        app_stop_timer(&sink_dev_list[0].timer_idx_acl_exist_delay);
        memcpy(sink_dev_list[0].remote_info.bd_addr, bd_addr, 6);
    }
#endif
    else if (ota_mode)
    {
        bt_acl_conn_accept(bd_addr, BT_LINK_ROLE_MASTER);
        app_stop_timer(&sink_dev_list[0].timer_idx_acl_exist_delay);
    }
    else
    {
        bt_acl_conn_reject(bd_addr, BT_ACL_REJECT_LIMITED_RESOURCE);
    }
}

static void idle_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("idle_handle_acl_status: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("idle_handle_acl_status: %s, status %02x, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), para->cause, sink_dev->bt_state);

    //TODO: connected by remote

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("idle_handle_acl_status: disconnected, %04x",
                            para->cause);

            app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
            app_stop_timer(&sink_dev->timer_idx_a2dp_conn_setup_delay);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            disconn_reason = (uint8_t)(para->cause & 0x00ff);

            src_bt_state_set(index, STATE_BT_IDLE);

            sink_dev->profile_mask_plan = 0;
            sink_dev->profile_mask_remain = 0;
            sink_dev->profile_mask_doing = 0;
            sink_dev->a2dp_codec_configured = false;
            sink_dev->a2dp_conn_setup_delay_once = false;
            sink_dev->a2dp_connected_from_remote = false;
        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link == NULL)
            {
                p_link = app_alloc_br_link(para->bd_addr);
            }

            gap_br_cfg_acl_link_flush_tout(para->bd_addr, AVDTP_STREAM_DATA_FLUSH_TIMEOUT);

            src_bt_state_set(index, STATE_BT_ACL_CONNECTED);

            bt_remote_name_req(sink_dev->remote_info.bd_addr);

            app_start_timer(&sink_dev->timer_idx_acl_exist_delay, "acl_init_from_remote",
                            src_policy_timer_id, SRC_TIMER_ID_ACL_EXIST_SINK0 + index, 0, false,
                            SRC_ACL_EXIST_DELAY_TIMEOUT);
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            sink_dev->conn_ready = true;

            //only enable in connected state
            bt_sniff_mode_disable(para->bd_addr);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                if (p_link->acl_link_role == BT_LINK_ROLE_MASTER)
                {
//                    bt_link_role_switch_disable(info->bd_addr);
                }
            }
            gap_br_cfg_acl_link_supv_tout(para->bd_addr, SRC_ACL_SUPERVISION_TIMEOUT);
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->auth_flag = true;
            }
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        break;

    case BT_EVENT_ACL_ROLE_MASTER:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->acl_link_role = BT_LINK_ROLE_MASTER;
            }

            if (sink_dev->conn_ready)
            {
                //bt_link_role_switch_disable(info->bd_addr);
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->acl_link_role = BT_LINK_ROLE_SLAVE;
            }

#if (MS_SURFACE == 0)
            if (sink_dev->conn_ready)
            {
                bt_link_role_switch(info->bd_addr, true);
            }
#endif
        }
        break;

    default:
        break;
    }
}

static void reconnecting_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("reconnecting_handle_acl_status: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("reconnecting_handle_acl_status: %s, status %02x, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), para->cause, sink_dev->bt_state);

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("reconnecting_handle_acl_status: disconnected, %04x",
                            para->cause);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            disconn_reason = (uint8_t)(para->cause & 0x00ff);

            app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
            app_stop_timer(&sink_dev->timer_idx_a2dp_conn_setup_delay);

            src_bt_state_set(index, STATE_BT_IDLE);

            sink_dev->profile_mask_plan = 0;
            sink_dev->profile_mask_remain = 0;
            sink_dev->profile_mask_doing = 0;
            sink_dev->a2dp_codec_configured = false;
            sink_dev->a2dp_conn_setup_delay_once = false;
            sink_dev->a2dp_connected_from_remote = false;

            if (src_is_cur_dev(index))
            {
                src_handle_action_result(index, false);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {
            APP_PRINT_INFO1("reconnecting_handle_acl_status: conn fail 0x%04x",
                            para->cause);

            if (sink_dev->bt_state == STATE_BT_PAGING)
            {
                if (para->cause == (HCI_ERR | HCI_ERR_ACL_CONN_EXIST))
                {
                    //continue to do sdp did
                    //gap_br_start_did_discov(info->bd_addr);

                    src_dev_state_set(index, STATE_DEV_IDLE);
                    sink_dev->profile_mask_doing = 0;
                    app_start_timer(&sink_dev->timer_idx_acl_exist_delay, "acl_exist_delay",
                                    src_policy_timer_id, SRC_TIMER_ID_ACL_EXIST_SINK0 + index, 0, false,
                                    SRC_ACL_EXIST_DELAY_TIMEOUT);
                }
                else
                {
                    src_bt_state_set(index, STATE_BT_IDLE);
                    app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
                    src_handle_action_result(index, false);
                }
            }
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link == NULL)
            {
                p_link = app_alloc_br_link(para->bd_addr);
            }

            if (sink_dev->bt_state != STATE_BT_PAGING)
            {
                //should be non-active state, connected from remote
                //re-useacl exists timer

                app_start_timer(&sink_dev->timer_idx_acl_exist_delay, "acl_init_from_remote",
                                src_policy_timer_id, SRC_TIMER_ID_ACL_EXIST_SINK0 + index, 0, false,
                                SRC_ACL_EXIST_DELAY_TIMEOUT);
            }

            gap_br_cfg_acl_link_flush_tout(para->bd_addr, AVDTP_STREAM_DATA_FLUSH_TIMEOUT);

            src_bt_state_set(index, STATE_BT_ACL_CONNECTED);

            bt_remote_name_req(sink_dev->remote_info.bd_addr);
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            sink_dev->conn_ready = true;

            //only enable in connected state
            bt_sniff_mode_disable(para->bd_addr);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                if (p_link->acl_link_role == BT_LINK_ROLE_MASTER)
                {
//                    bt_link_role_switch_disable(info->bd_addr);
                }
            }

            gap_br_cfg_acl_link_supv_tout(para->bd_addr, SRC_ACL_SUPERVISION_TIMEOUT);
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->auth_flag = true;
            }
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            APP_PRINT_INFO1("reconnecting_handle_acl_status: auth fail, %04x",
                            para->cause);

            if (para->cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL))
            {
                //wait disconnect complete event to cleanup dev
#if F_BT_LEGACY_APP_KEY_MANAGER
                app_src_legacy_delete_bond_by_index(index);
#else
#if (MS_SURFACE == 0)
                bt_bond_delete(info->bd_addr);
#endif
#endif
            }
            else if (para->cause == (HCI_ERR | HCI_ERR_KEY_MISSING))
            {
                //only delete link key in active connection setup
#if F_BT_LEGACY_APP_KEY_MANAGER
                app_src_legacy_delete_bond_by_index(index);
#else
                if (report_pair_state)
                {
                    bt_bond_delete(para->bd_addr);
                }
#endif
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_MASTER:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->acl_link_role = BT_LINK_ROLE_MASTER;
            }

            if (sink_dev->conn_ready)
            {
//                bt_link_role_switch_disable(info->bd_addr);
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->acl_link_role = BT_LINK_ROLE_SLAVE;
                bt_link_role_switch(para->bd_addr, true);
            }
        }
        break;

    default:
        break;
    }
}

static void connecting_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("connecting_handle_acl_status: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("connecting_handle_acl_status: %s, status %d, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), para->cause, sink_dev->bt_state);

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("connecting_handle_acl_status: disconnected, 0x%04x",
                            para->cause);

            app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
            app_stop_timer(&sink_dev->timer_idx_a2dp_conn_setup_delay);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            disconn_reason = (uint8_t)(para->cause & 0x00ff);

            if (sink_dev->bt_state == STATE_BT_ACL_CONNECTED)
            {
                src_bt_state_set(index, STATE_BT_IDLE);

                sink_dev->profile_mask_plan = 0;
                sink_dev->profile_mask_remain = 0;
                sink_dev->profile_mask_doing = 0;
                sink_dev->a2dp_codec_configured = false;
                sink_dev->a2dp_conn_setup_delay_once = false;
                sink_dev->a2dp_connected_from_remote = false;

                src_handle_action_result(index, false);
            }
        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {
            APP_PRINT_INFO1("connecting_handle_acl_status: conn fail 0x%04x",
                            para->cause);

            if (sink_dev->bt_state == STATE_BT_PAGING)
            {
                if (para->cause == (HCI_ERR | HCI_ERR_ACL_CONN_EXIST))
                {
                    //continue to do sdp did
                    //gap_br_start_did_discov(info->bd_addr);
                    app_start_timer(&sink_dev->timer_idx_acl_exist_delay, "acl_exist_delay",
                                    src_policy_timer_id, SRC_TIMER_ID_ACL_EXIST_SINK0 + index, 0, false,
                                    SRC_ACL_EXIST_DELAY_TIMEOUT);
                }
                else
                {
                    src_bt_state_set(index, STATE_BT_IDLE);
                    src_handle_action_result(index, false);
                }
            }
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link == NULL)
            {
                p_link = app_alloc_br_link(para->bd_addr);
            }

            gap_br_cfg_acl_link_flush_tout(para->bd_addr, AVDTP_STREAM_DATA_FLUSH_TIMEOUT);

            src_bt_state_set(index, STATE_BT_ACL_CONNECTED);

            bt_remote_name_req(sink_dev->remote_info.bd_addr);
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            sink_dev->conn_ready = true;

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
#if (MS_SURFACE == 0)
                if (p_link->acl_link_role == BT_LINK_ROLE_MASTER)
                {
//                    bt_link_role_switch_disable(info->bd_addr);
                }
                else
                {
                    bt_link_role_switch(info->bd_addr, true);
                }
#endif
            }
            gap_br_cfg_acl_link_supv_tout(para->bd_addr, SRC_ACL_SUPERVISION_TIMEOUT);
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->auth_flag = true;
            }
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            APP_PRINT_INFO1("connecting_handle_acl_status: auth fail, %04x",
                            para->cause);

            if (para->cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL))
            {
                //wait disconnect complete event to cleanup dev
#if F_BT_LEGACY_APP_KEY_MANAGER
                app_src_legacy_delete_bond_by_index(index);
#else
#if (MS_SURFACE == 0)
                bt_bond_delete(info->bd_addr);
#endif
#endif
            }
            else if (para->cause == (HCI_ERR | HCI_ERR_KEY_MISSING))
            {
#if F_BT_LEGACY_APP_KEY_MANAGER
                app_src_legacy_delete_bond_by_index(index);
#else
                //only delete link key in active connection setup
                if (report_pair_state)
                {
                    bt_bond_delete(para->bd_addr);
                }
#endif
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_MASTER:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->acl_link_role = BT_LINK_ROLE_MASTER;
            }

            if (sink_dev->conn_ready)
            {
#if (MS_SURFACE == 0)
//                bt_link_role_switch_disable(info->bd_addr);
#endif
            }
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->acl_link_role = BT_LINK_ROLE_SLAVE;
            }

#if (MS_SURFACE == 0)
            if (sink_dev->conn_ready)
            {
                bt_link_role_switch(info->bd_addr, true);
            }
#endif
        }
        break;

    default:
        break;
    }
}

static void connected_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("connected_handle_acl_status: %s, status %d, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), para->cause, sink_dev->bt_state);

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("connected_handle_acl_status: disconnected, 0x%04x",
                            para->cause);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            disconn_reason = (uint8_t)(para->cause & 0x00ff);

            src_dev_cleanup_restore_dev_info(index);

            src_led_set_idle(index);

            //app_usb_hid_connection_state_notify(DEV_SLOT_HS, BT_CONN_STATE_DISCONNECTED, 0);
            app_bt_connection_state_report_event(BT_CONN_STATE_DISCONNECTED, 0);
            app_led_set_mode(APP_LED_MODE_DISCONNECTED);
            if (disconn_reason != HCI_ERR_LOCAL_HOST_TERMINATE)
            {
                APP_PRINT_INFO1("connected_handle_acl_status: busy state, 0x%04x",
                                src_is_busy());
                if (!src_is_busy())
                {
                    src_is_reconnecting = true;
                    src_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
                }
                else
                {
                    //wait for reschdule
                }
            }
        }
        break;
    case BT_EVENT_ACL_CONN_FAIL:
        break;
    case BT_EVENT_ACL_CONN_SUCCESS:
        break;
    case BT_EVENT_ACL_CONN_READY:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                if (p_link->acl_link_role == BT_LINK_ROLE_MASTER)
                {
#if (MS_SURFACE == 0)
//                    bt_link_role_switch_disable(info->bd_addr);
#endif
                }
            }
            gap_br_cfg_acl_link_supv_tout(para->bd_addr, SRC_ACL_SUPERVISION_TIMEOUT);
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        break;
    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            APP_PRINT_INFO1("connected_handle_acl_status: auth fail, %04x",
                            para->cause);

            if (para->cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL))
            {
                //wait disconnect complete event to cleanup dev
#if F_BT_LEGACY_APP_KEY_MANAGER
                app_src_legacy_delete_bond_by_index(index);
#else
#if (MS_SURFACE == 0)
                bt_bond_delete(info->bd_addr);
#endif
#endif
            }
            else if (para->cause == (HCI_ERR | HCI_ERR_KEY_MISSING))
            {
                //only delete link key in active connection setup
                if (report_pair_state)
                {
                    bt_bond_delete(para->bd_addr);
                }
            }
        }
        break;
    case BT_EVENT_ACL_ROLE_MASTER:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->acl_link_role = BT_LINK_ROLE_MASTER;
            }

            //bt_link_role_switch_disable(info->bd_addr);
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                p_link->acl_link_role = BT_LINK_ROLE_SLAVE;
            }

#if (MS_SURFACE == 0)
            bt_link_role_switch(info->bd_addr, true);
#endif
        }
        break;
    default:
        break;
    }
}

static void cleanup_handle_acl_status(uint8_t index, T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_SRC_KEY_EVENT pending_evt = sink_dev->pending_event;

    APP_PRINT_INFO3("cleanup_handle_acl_status: %s, status %d, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), para->cause, sink_dev->bt_state);

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("cleanup_handle_acl_status: disconnected, %04x",
                            para->cause);

            src_bt_state_set(index, STATE_BT_IDLE);

            disconn_reason = (uint8_t)(para->cause & 0x00ff);

            sink_dev->profile_mask_plan = 0;
            sink_dev->profile_mask_remain = 0;
            sink_dev->profile_mask_doing = 0;
            sink_dev->a2dp_codec_configured = false;
            sink_dev->a2dp_conn_setup_delay_once = false;
            sink_dev->a2dp_connected_from_remote = false;

            app_stop_timer(&sink_dev->timer_idx_acl_exist_delay);
            app_stop_timer(&sink_dev->timer_idx_stream_idle);
            app_stop_timer(&sink_dev->timer_idx_a2dp_conn_setup_delay);
            app_stop_timer(&sink_dev->timer_idx_pair_suc_report_delay);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            src_handle_dev_pending_evt(index, pending_evt);

            //src_led_set_idle(index);
            app_led_set_mode(APP_LED_MODE_DISCONNECTED);
            if (app_device_is_power_on() == false)
            {
                app_led_set_mode(APP_LED_MODE_POWER_OFF);
            }
        }
        break;
    case BT_EVENT_ACL_CONN_FAIL:
        {
            APP_PRINT_INFO1("cleanup_handle_acl_status: conn fail, %04x",
                            para->cause);
            src_dev_state_set(index, STATE_DEV_IDLE);
            src_handle_dev_pending_evt(index, pending_evt);

            src_led_set_idle(index);
        }
        break;
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            APP_PRINT_INFO0("cleanup_handle_acl_status: connected, disconnect");

            if (sink_dev->bt_state == STATE_BT_PAGING)
            {
                gap_br_send_acl_disconn_req(para->bd_addr);
            }
            else
            {
                //warning ?
            }
        }
        break;
    case BT_EVENT_ACL_CONN_READY:
        break;
    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        break;
    case BT_EVENT_ACL_AUTHEN_FAIL:
        break;
    case BT_EVENT_ACL_ROLE_MASTER:
        break;
    case BT_EVENT_ACL_ROLE_SLAVE:
        break;
    default:
        break;
    }
}

static void src_ota_handle_acl_status(T_BT_PARAM *para, T_BT_EVENT event_type)
{
    T_APP_BR_LINK *p_link;

    APP_PRINT_INFO2("src_ota_handle_acl_status: %s, status %d, bt_state %d",
                    TRACE_BDADDR(para->bd_addr), para->cause);

    //TODO: connected by remote

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            p_link = app_find_br_link(para->bd_addr);
            if (p_link == NULL)
            {
                p_link = app_alloc_br_link(para->bd_addr);
            }

        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            APP_PRINT_INFO1("src_ota_handle_acl_status: disconnected, %04x",
                            para->cause);

            p_link = app_find_br_link(para->bd_addr);
            if (p_link != NULL)
            {
                app_free_br_link(p_link);
            }

            disconn_reason = (uint8_t)(para->cause & 0x00ff);
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            bt_acl_pkt_type_set(para->bd_addr, BT_ACL_PKT_TYPE_2M);
        }
        break;

    default:
        break;
    }
}


static void src_handle_acl_link_status(T_BT_PARAM *para, T_BT_EVENT event_type)
{
    uint8_t index = 0;

    APP_PRINT_INFO2("src_handle_acl_link_status: %s, %02x",
                    TRACE_BDADDR(para->bd_addr), event_type);

#if F_BT_LEGACY_APP_KEY_MANAGER
    if (src_policy_find_dev_index_by_addr(para->bd_addr, &index) ||
        src_policy_find_dev_index_by_bond_info(para->bd_addr, &index))
#else
    if (src_policy_find_dev_index_by_addr(para->bd_addr, &index))
#endif
    {
        APP_PRINT_INFO1("src_handle_acl_link_status: dev state %04x",
                        sink_dev_list[index].dev_state);

        switch (sink_dev_list[index].dev_state)
        {
        case STATE_DEV_IDLE:
            {
                idle_handle_acl_status(index, para, event_type);
            }
            break;

        case STATE_DEV_RECONNECTING:
            {
                reconnecting_handle_acl_status(index, para, event_type);
            }
            break;

        case STATE_DEV_CONNECTING:
            {
                connecting_handle_acl_status(index, para, event_type);
            }
            break;

        case STATE_DEV_CONNECTED:
            {
                connected_handle_acl_status(index, para, event_type);
            }
            break;

        case STATE_DEV_CLEANUP:
            {
                cleanup_handle_acl_status(index, para, event_type);
            }
            break;

        default:
            break;
        }
    }
    else
    {
        if (ota_mode)
        {
            src_ota_handle_acl_status(para, event_type);
        }
    }

    src_check_set_bt_mode();
}

static void src_handle_acl_remote_name_rsp(T_BT_EVENT_PARAM_REMOTE_NAME_RSP rsp)
{
    uint8_t index = 0;
    uint8_t name_length = 0;
    T_APP_DEV *sink_dev = NULL;

    if (rsp.cause)
    {
        APP_PRINT_WARN2("src_handle_acl_remote_name_rsp: cause %d for 5s",
                        rsp.cause, TRACE_BDADDR(rsp.bd_addr));

        return;
    }

    if (src_policy_find_dev_index_by_addr(rsp.bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        name_length = strlen(rsp.name);

        if (name_length >= GAP_DEVICE_NAME_LEN)
        {
            name_length = GAP_DEVICE_NAME_LEN - 1;
        }

        memcpy(sink_dev->remote_info.name, rsp.name, name_length);
        sink_dev->remote_info.name[name_length] = 0;
    }
}

static void src_handle_role_switch_fail(uint8_t *bd_addr, uint16_t cause)
{
    APP_PRINT_INFO2("src_handle_role_switch_fail: %s, %d", TRACE_BDADDR(bd_addr), cause);

    //local collision, try again
    if (cause == (HCI_ERR | HCI_ERR_DIFF_TRANS_COLLISION))
    {
        bt_link_role_switch(bd_addr, true);
    }
}

static void src_handle_link_key_info(uint8_t *bd_addr, uint8_t *link_key,
                                     T_BT_LINK_KEY_TYPE key_type)
{
    uint8_t index = 0;

#if F_BT_LEGACY_APP_KEY_MANAGER
    if (src_policy_find_dev_index_by_addr(bd_addr, &index) ||
        src_policy_find_dev_index_by_bond_info(bd_addr, &index))
#else
    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
#endif
    {
#if F_BT_LEGACY_APP_KEY_MANAGER
        app_src_legacy_save_bond(index, bd_addr, link_key, key_type);
        if (key_type == 0x07) // ltk to link key
        {
            uint32_t bond_flag = 0;
            bond_flag |= APP_SRC_BOND_FLAG_A2DP;
            app_src_legacy_add_bond_flag(bd_addr, bond_flag);
            // src_enter_reconnecting_state(index);
            // src_start_reconn(index);
        }
#else
        bt_bond_key_set(bd_addr, link_key, key_type);
#endif

#if HID_DEBUG_INTERFACE_SUPPORT
        APP_PRINT_INFO0("src_handle_link_key_info: report link_key");
        app_usb_hid_send_debug_data(0, link_key, 16);
#endif
    }
    else
    {
        //save in OTA mode ???
    }
}

void general_bt_set_volume(uint32_t vol, uint8_t mute)
{
    uint8_t i;

    APP_PRINT_INFO1("general_bt_set_volume: volume 0x%02x", vol);
    if (mute)
    {
        vol = 0;
    }

    uint32_t profile = AVRCP_PROFILE_MASK;
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        profile = AVRCP_PROFILE_MASK;
        if (get_connected_profile(sink_dev_list[i].remote_info.bd_addr, &profile))
        {
            if (bt_avrcp_absolute_volume_set(sink_dev_list[i].remote_info.bd_addr, vol))
            {
                APP_PRINT_INFO2("general_bt_set_volume: %b set vol 0x%02x succeeded",
                                TRACE_BDADDR(sink_dev_list[i].remote_info.bd_addr),
                                vol);
            }
            else
            {
                APP_PRINT_INFO2("general_bt_set_volume: %b set vol 0x%02x failed",
                                TRACE_BDADDR(sink_dev_list[i].remote_info.bd_addr),
                                vol);
            }
        }
        profile = HFP_PROFILE_MASK;
        if (get_connected_profile(sink_dev_list[i].remote_info.bd_addr, &profile))
        {
            APP_PRINT_INFO1("general_bt_set_volume: profile %d true", profile);
            extern void app_src_hfp_set_spk_vol(int16_t vol);
            app_src_hfp_set_spk_vol(vol);
        }
    }
}

static void src_handle_link_key_req(uint8_t *bd_addr)
{
    uint8_t index;
    uint8_t link_key[16];
    T_BT_LINK_KEY_TYPE key_type;

#if F_BT_LEGACY_APP_KEY_MANAGER
    if ((src_policy_find_dev_index_by_addr(bd_addr, &index) ||
         src_policy_find_dev_index_by_bond_info(bd_addr, &index)) &&
        app_src_legacy_get_bond_by_addr(bd_addr, link_key, &key_type))
#else
    if (src_policy_find_dev_index_by_addr(bd_addr, &index) &&
        bt_bond_key_get(bd_addr, link_key, (uint8_t *)&key_type))
#endif
    {
        bt_link_key_cfm(bd_addr, true, key_type, link_key);
#if HID_DEBUG_INTERFACE_SUPPORT
        APP_PRINT_INFO0("src_handle_link_key_req: report link_key");
        app_usb_hid_send_debug_data(0, link_key, 16);
#endif
    }
    else
    {
        bt_link_key_cfm(bd_addr, false, key_type, link_key);
    }
}
#if 0
static void src_get_sdp_manufacturer_data(uint8_t index, uint8_t *sdp_attr, uint16_t len)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t *p_start;
    uint8_t *p_end;
    uint8_t *p_attr;
    uint8_t type;
    uint8_t size_index;
    uint8_t val_len;

    if ((sdp_attr == NULL) || (len == 0))
    {
        APP_PRINT_WARN0("src_get_sdp_str_elem_val: err in params");
        return;
    }

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_get_sdp_manufacturer_data: index %d is invalid", index);
        return;
    }

    p_start = sdp_attr;
    p_end = p_start + len;

    p_attr = legacy_find_sdp_attr(p_start, p_end, 0x0301);  //HWID attr
    if (p_attr == NULL)
    {
        APP_PRINT_INFO0("src_get_sdp_manufacturer_data: no attr");
        return;
    }

    /* header of attribute value */
    BE_STREAM_TO_UINT8(size_index, p_attr);
    type = (size_index >> 3) & 0x1f;
    size_index = size_index & 0x07;

    switch (size_index)
    {
    case 0:
        if (type == 0)       /* for null type , len is zero */
        {
            val_len = 0;
        }
        else                            /* otherwise, len is 1 */
        {
            val_len = 1;
        }
        break;

    case 1:
        val_len = 2;
        break;

    case 2:
        val_len = 4;
        break;

    case 3:
        val_len = 8;
        break;

    case 4:
        val_len = 16;
        break;

    case 5:
        BE_STREAM_TO_UINT8(val_len, p_attr);
        break;

    case 6:
        BE_STREAM_TO_UINT16(val_len, p_attr);
        break;

    case 7:
        BE_STREAM_TO_UINT32(val_len, p_attr);
        break;
    }


    if ((type == 0x04) && (val_len > 0))   //string type
    {
        sink_dev = &sink_dev_list[index];

        if (val_len >= HWID_LEN_MAX)
        {
            val_len = HWID_LEN_MAX - 1;
        }

        memcpy(sink_dev->remote_info.hwid, p_attr, val_len);
        sink_dev->remote_info.hwid[val_len] = 0;
    }
}
#endif
static void src_handle_sdp_attr_info(uint8_t *bd_addr, T_BT_SDP_ATTR_INFO sdp_attr)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index;
    uint8_t local_server_chann;

    APP_PRINT_INFO1("src_handle_sdp_attr_info: %s", TRACE_BDADDR(bd_addr));

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        APP_PRINT_INFO1("src_handle_sdp_attr_info: doing_profile %08x",
                        sink_dev->profile_mask_doing);

        if ((sink_dev->dev_state == STATE_DEV_RECONNECTING) ||
            (sink_dev->dev_state == STATE_DEV_CONNECTING))
        {
            if (SPP_PROFILE_MASK == sink_dev->profile_mask_doing)
            {

                if (bt_spp_registered_uuid_check((T_BT_SPP_UUID_TYPE)(sdp_attr.srv_class_uuid_type),
                                                 (T_BT_SPP_UUID_DATA *) & (sdp_attr.srv_class_uuid_data),
                                                 &local_server_chann))
                {
                    APP_PRINT_INFO2("src_handle_sdp_attr_info: local %02x, remote %02x",
                                    local_server_chann, sdp_attr.server_channel);

                    sink_dev->sdp_info.is_found = true;
                    sink_dev->sdp_info.local_server_channel = local_server_chann;
                    sink_dev->sdp_info.server_channel = sdp_attr.server_channel;
                }
            }
            else if (HFP_PROFILE_MASK == sink_dev->profile_mask_doing)
            {
                sink_dev->sdp_info.is_found = true;
                APP_PRINT_INFO1("src_handle_sdp_attr_info handle %x", sdp_attr.server_channel);
                sink_dev->sdp_info.server_channel = sdp_attr.server_channel;
            }
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
            else if (ASP_PROFILE_MASK == sink_dev->profile_mask_doing)
            {
                sink_dev->sdp_info.is_found = true;
                sink_dev->sdp_info.server_channel = sdp_attr.server_channel;
            }
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
            else if (SPP_VENDER_PASSTHROUGH_PROFILE_MASK == sink_dev->profile_mask_doing)
            {
                sink_dev->sdp_info.is_found = true;
                sink_dev->sdp_info.server_channel = sdp_attr.server_channel;
            }
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
            else if (SPP_AUDIO_PROFILE_MASK == sink_dev->profile_mask_doing)
            {
                sink_dev->sdp_info.is_found = true;
                sink_dev->sdp_info.server_channel = sdp_attr.server_channel;
            }
#endif
#if 0
            else if (MANUFACTURER_DATA_MASK == sink_dev->profile_mask_doing)
            {
                //only read attr data, not to connect
                //sink_dev->sdp_info.is_found = true;
                src_get_sdp_manufacturer_data(index, sdp_attr.p_attr, sdp_attr.attr_len);
            }
#endif
            else
            {
                sink_dev->sdp_info.is_found = true;
            }
        }
    }
}

static void src_handle_did_attr_info(T_BT_EVENT_PARAM_DID_ATTR_INFO info)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index;

    if (src_policy_find_dev_index_by_addr(info.bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        sink_dev->remote_info.vid = info.vendor_id;
        sink_dev->remote_info.pid = info.product_id;

        APP_PRINT_INFO2("src_handle_did_attr_info: vid %04x, pid %04x",
                        info.vendor_id, info.product_id);
    }
}


static void src_handle_sdp_cmpl(uint8_t *bd_addr, uint16_t cause)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        APP_PRINT_INFO5("src_handle_sdp_cmpl: %s mask %08x, dev_state 0x%04x, found %d, cause 0x%04x",
                        TRACE_BDADDR(sink_dev->remote_info.bd_addr),
                        sink_dev->profile_mask_doing,
                        sink_dev->dev_state,
                        sink_dev->sdp_info.is_found,
                        cause);

        if ((sink_dev->dev_state == STATE_DEV_RECONNECTING) ||
            (sink_dev->dev_state == STATE_DEV_CONNECTING))
        {
            sink_dev->sdp_info.is_busy = false;

            if (cause)
            {
                //only l2cap err handled here
                if (cause & L2C_ERR)
                {
                    if (cause == (L2C_ERR | L2C_ERR_SECURITY_BLOCK))
                    {
                        gap_br_send_acl_disconn_req(bd_addr);
                    }
                    else
                    {
                        src_profile_conn_finish(index);
                        src_dev_profile_conn(index);
                    }
                }
            }
            else if (sink_dev->sdp_info.is_found)
            {
                if (!src_profile_conn(sink_dev->remote_info.bd_addr,
                                      sink_dev->profile_mask_doing,
                                      sink_dev->sdp_info))
                {
                    APP_PRINT_INFO2("src_handle_sdp_cmpl: failed to connect profile %08x with %s",
                                    sink_dev->profile_mask_doing,
                                    TRACE_BDADDR(sink_dev->remote_info.bd_addr));

                    src_profile_conn_finish(index);
                    src_dev_profile_conn(index);
                }
            }
            else
            {
                src_profile_conn_finish(index);
                src_dev_profile_conn(index);
            }
        }
    }
}

static void src_handle_a2dp_conn_ind(uint8_t *bd_addr)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;

    //accepted in app_src_a2dp.c

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];
        sink_dev->a2dp_connected_from_remote = true;

        APP_PRINT_INFO1("src_handle_a2dp_conn_ind: a2dp connected from remote %s",
                        TRACE_BDADDR(bd_addr));
    }
}


void src_handle_profile_conn_cmpl(uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;
    uint32_t bond_flag = 0;

    APP_PRINT_INFO2("src_handle_profile_conn_cmpl: %s mask %08x",
                    TRACE_BDADDR(bd_addr), profile_mask);

    add_conneted_profile(bd_addr, profile_mask);
    if (profile_mask == HFP_PROFILE_MASK)
    {
        src_led_set_hfp_connected(bd_addr);
    }

#if F_BT_LEGACY_APP_KEY_MANAGER
    if (src_policy_find_dev_index_by_addr(bd_addr, &index) ||
        src_policy_find_dev_index_by_bond_info(bd_addr, &index))
#else
    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
#endif
    {
        sink_dev = &sink_dev_list[index];

        APP_PRINT_INFO2("src_handle_profile_conn_cmpl: index %d, dev_state %02x",
                        index, sink_dev->dev_state);

        bond_flag = get_bond_flag_from_profile_mask(profile_mask);
        if (bond_flag != 0)
        {
#if F_BT_LEGACY_APP_KEY_MANAGER
            app_src_legacy_add_bond_flag(bd_addr, bond_flag);
#else
            bt_bond_flag_add(bd_addr, bond_flag);
#endif
        }

        if (profile_mask == A2DP_PROFILE_MASK)
        {
            sink_dev->stream_info.credits = SRC_A2DP_STREAM_MAX_CREDITS;
        }

        if (profile_mask == AVRCP_PROFILE_MASK)
        {
            avrcp_is_connected = true;
        }

        switch (sink_dev->dev_state)
        {
        case STATE_DEV_IDLE:
            {
                //wait for acl exist timeout handle
#if 0
                if ((profile_mask == A2DP_PROFILE_MASK) || (profile_mask == HFP_PROFILE_MASK))
                {
                    src_dev_state_set(index, STATE_DEV_CONNECTED);
                    memcpy(sink_dev->remote_info.bd_addr, bd_addr, 6);
                    src_led_set_connected(index);
                }
#endif
            }
            break;

        case STATE_DEV_RECONNECTING:
        case STATE_DEV_CONNECTING:
            {
                /*
                    if acl connected from local, profile from remote,
                    in case this profile is the current one,
                    check sdp's is_busy flag to avoid starting a new sdp
                    while doing the last one
                */
                if (sink_dev->sdp_info.is_busy)
                {
                    APP_PRINT_INFO0("src_handle_profile_conn_cmpl, not initiated from local, ignore");
                    return;
                }

                //if connected from remote, profile_mask_doing will be 0x00000000
                if (sink_dev->profile_mask_doing == profile_mask)
                {
                    src_profile_conn_finish(index);
                    src_dev_profile_conn(index);
                }
            }
            break;

        case STATE_DEV_CONNECTED:
            {
//                app_usb_hid_connection_state_notify(DEV_SLOT_HS,
//                                                    src_get_audio_device_conn_state(),
//                                                    src_get_audio_device_profile_state());
                app_bt_connection_state_report_event(src_get_audio_device_conn_state(),
                                                     src_get_audio_device_profile_state());
                if ((sink_dev->sco_state == SCO_SETUP_STATE_IDLE) && (app_usb_audio_is_us_streaming()))
                {
                    sink_dev->sco_state = SCO_SETUP_STATE_CONNECTING;
                    bt_hfp_ag_audio_connect_req(bd_addr);
                }
            }
            break;

        case STATE_DEV_CLEANUP:
            {
                APP_PRINT_INFO0("src_handle_profile_conn_cmpl: disconnect in cleaup");
                src_profile_disconn(bd_addr, profile_mask);
            }
            break;

        default:
            break;
        }
    }
}

static uint16_t src_calc_sbc_frame_size(uint8_t channel_mode,
                                        uint8_t blk_len,
                                        uint8_t subbands,
                                        uint8_t bitpool)
{
    uint8_t channel_num = 0;
    uint8_t block_num = 0;
    uint8_t subband_num = 0;
    uint16_t frame_size = 0;
    uint8_t joint = 0;
    uint16_t temp = 0;

    switch (channel_mode)
    {
    case 0x01:
        joint = 1;
        channel_num = 2;
        break;

    case 0x02:
    case 0x04:
        channel_num = 2;
        break;

    case 0x08:
        channel_num = 1;
        break;

    default:
        break;
    }

    switch (blk_len)
    {
    case 0x10:
        block_num = 16;
        break;

    case 0x20:
        block_num = 12;
        break;

    case 0x40:
        block_num = 8;
        break;

    case 0x80:
        block_num = 4;
        break;

    default:
        break;
    }

    switch (subbands)
    {
    case 0x04:
        subband_num = 8;
        break;

    case 0x08:
        subband_num = 4;
        break;

    default:
        break;
    }

    if (channel_mode & 0x0c)
    {
        //mono or dual channel
        temp = block_num * channel_num * bitpool;

        frame_size = 4 + subband_num * channel_num / 2 + temp / 8;

        if (temp % 8 != 0)
        {
            frame_size++;
        }
    }
    else if (channel_mode & 0x03)
    {
        //stereo or joint stereo
        temp = joint * subband_num + block_num * bitpool;

        frame_size = 4 + subband_num * channel_num / 2 + temp / 8;

        if (temp % 8 != 0)
        {
            frame_size++;
        }
    }

    APP_PRINT_INFO1("src_calc_sbc_frame_size: frame size %d", frame_size);

    return frame_size;
}

static uint16_t get_a2dp_sbc_sample_rate(uint8_t sf_mask)
{
    uint16_t sample_rate = 0;

    switch (sf_mask)
    {
    case 0x80:
        sample_rate = 16000;
        break;

    case 0x40:
        sample_rate = 32000;
        break;

    case 0x20:
        sample_rate = 44100;
        break;

    case 0x10:
        sample_rate = 48000;
        break;

    default:
        APP_PRINT_WARN1("get_sbc_sample_rate: unknown mask %02x", sf_mask);
        break;
    }

    return sample_rate;
}

static T_AUDIO_SBC_CHANNEL_MODE get_a2dp_sbc_channel_mode(uint8_t cm_mask)
{
    T_AUDIO_SBC_CHANNEL_MODE cm = AUDIO_SBC_CHANNEL_MODE_MONO;

    switch (cm_mask)
    {
    case 0x08:
        cm = AUDIO_SBC_CHANNEL_MODE_MONO;
        break;

    case 0x04:
        cm = AUDIO_SBC_CHANNEL_MODE_DUAL;
        break;

    case 0x02:
        cm = AUDIO_SBC_CHANNEL_MODE_STEREO;
        break;

    case 0x01:
        cm = AUDIO_SBC_CHANNEL_MODE_JOINT_STEREO;
        break;

    default:
        APP_PRINT_WARN1("get_a2dp_sbc_channel_mode: unknown mask %02x", cm_mask);
        break;
    }

    return cm;
}

static uint8_t get_a2dp_sbc_block_length(uint8_t bl_mask)
{
    uint8_t bl = 0;

    switch (bl_mask)
    {
    case 0x80:
        bl = 4;
        break;

    case 0x40:
        bl = 8;
        break;

    case 0x20:
        bl = 12;
        break;

    case 0x10:
        bl = 16;
        break;

    default:
        APP_PRINT_WARN1("get_a2dp_sbc_block_length: unknown mask %02x", bl_mask);
        break;
    }

    return bl;
}

static uint8_t get_a2dp_sbc_subbands(uint8_t sbs_mask)
{
    uint8_t sbs = 0;

    switch (sbs_mask)
    {

    case 0x08:
        sbs = 4;
        break;

    case 0x04:
        sbs = 8;
        break;

    default:
        APP_PRINT_WARN1("get_a2dp_sbc_subbands: unknown mask %02x", sbs_mask);
        break;
    }

    return sbs;
}

static uint8_t get_a2dp_sbc_allocation_method(uint8_t am_mask)
{
    uint8_t am = 0;

    switch (am_mask)
    {

    case 0x02:
        am = 1;
        break;

    case 0x01:
        am = 0;
        break;

    default:
        APP_PRINT_WARN1("get_a2dp_sbc_allocation_method: unknown mask %02x", am_mask);
        break;
    }

    return am;
}


static void get_audio_format_info_from_a2dp_cfg(T_AUDIO_FORMAT_INFO *info,
                                                T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL cfg)
{
    switch (cfg.codec_type)
    {
    case BT_A2DP_CODEC_TYPE_SBC:
        {
            info->type = AUDIO_FORMAT_TYPE_SBC;

            info->attr.sbc.sample_rate =
                get_a2dp_sbc_sample_rate(cfg.codec_info.sbc.sampling_frequency);

            info->attr.sbc.chann_mode =
                get_a2dp_sbc_channel_mode(cfg.codec_info.sbc.channel_mode);

            info->attr.sbc.block_length =
                get_a2dp_sbc_block_length(cfg.codec_info.sbc.block_length);

            info->attr.sbc.subband_num =
                get_a2dp_sbc_subbands(cfg.codec_info.sbc.subbands);

            info->attr.sbc.allocation_method =
                get_a2dp_sbc_allocation_method(cfg.codec_info.sbc.allocation_method);

            info->attr.sbc.bitpool = cfg.codec_info.sbc.max_bitpool;

        }
        break;

    default:
        break;
    }
}

static void src_handle_a2dp_config_cmpl(T_BT_EVENT_PARAM_A2DP_CONFIG_CMPL cfg)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;

    APP_PRINT_INFO1("src_handle_a2dp_config_cmpl: %s", TRACE_BDADDR(cfg.bd_addr));

#if F_BT_LEGACY_APP_KEY_MANAGER
    if (src_policy_find_dev_index_by_addr(cfg.bd_addr, &index) ||
        src_policy_find_dev_index_by_bond_info(cfg.bd_addr, &index))
#else
    if (src_policy_find_dev_index_by_addr(cfg.bd_addr, &index))
#endif
    {
        sink_dev = &sink_dev_list[index];
        get_audio_format_info_from_a2dp_cfg(&sink_dev->codec_info, cfg);
        sink_dev->a2dp_codec_configured = true;
    }
}

#ifdef A2DP_INTERVAL_PACKET
static uint16_t src_cal_time_packet_num(uint32_t sample_rate,
                                        uint8_t interval_ms, uint8_t blocks, uint8_t subbands)
{
    uint64_t temp1 = 0;
    uint32_t temp2 = 0;
    uint16_t pkt_num = 0;

    temp1 = sample_rate * interval_ms;
    temp2 = blocks * subbands * 1000;
    pkt_num = temp1 / temp2;

    if (temp1 % temp2 != 0)
    {
        pkt_num++;
    }

    APP_PRINT_INFO5("src_cal_time_packet_num: %d, %d, %d, %d, %d",
                    sample_rate, interval_ms, blocks, subbands, pkt_num);

    return pkt_num;
}
#endif

static void src_init_a2dp_stream_info(uint8_t index, uint16_t mtu)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_SRC_STREAM_INFO *info = &sink_dev->stream_info;
    T_APP_BR_LINK *p_link = app_find_br_link(sink_dev->remote_info.bd_addr);

#ifdef A2DP_INTERVAL_PACKET
    uint16_t interval_pkt_num = 0;
#endif

    if (p_link != NULL)
    {
        sink_dev->stream_info.codec_type = p_link->a2dp_codec_type;

        if (p_link->a2dp_codec_type == BT_A2DP_CODEC_TYPE_SBC)
        {
            info->sbc_frame_size = src_calc_sbc_frame_size(
                                       p_link->a2dp_codec_info.sbc.channel_mode,
                                       p_link->a2dp_codec_info.sbc.block_length,
                                       p_link->a2dp_codec_info.sbc.subbands,
                                       p_link->a2dp_codec_info.sbc.max_bitpool);

            if (info->sbc_frame_size == 0)
            {
                APP_PRINT_ERROR0("src_init_a2dp_stream_info sbc_frame_size is 0!");
                return;
            }

            if (mtu > SRC_2M_ACL_MAX_PAYLOAD_SIZE)
            {
                mtu = SRC_2M_ACL_MAX_PAYLOAD_SIZE;
            }

#ifdef A2DP_INTERVAL_PACKET
            interval_pkt_num = src_cal_time_packet_num(sink_dev->codec_info.attr.sbc.sample_rate,
                                                       15,
                                                       sink_dev->codec_info.attr.sbc.block_length,
                                                       sink_dev->codec_info.attr.sbc.subband_num);

            if (mtu - SRC_2M_ACL_AVDTP_STREAM_OFFSET >= interval_pkt_num * info->sbc_frame_size)
            {
                info->sbc_max_frame_num = interval_pkt_num;
            }
            else
            {
                info->sbc_max_frame_num = (mtu - SRC_2M_ACL_AVDTP_STREAM_OFFSET) / info->sbc_frame_size;
            }
#else
            info->sbc_max_frame_num = (mtu - SRC_2M_ACL_AVDTP_STREAM_OFFSET) / info->sbc_frame_size;
#endif
            info->sbc_offset = 0;
            info->buf_size = info->sbc_max_frame_num * info->sbc_frame_size;

            info->buffer = os_mem_zalloc(RAM_TYPE_DATA_ON, info->buf_size);

            APP_PRINT_INFO3("src_init_a2dp_stream_info: mtu %d, max frame num %d, buf_size %d",
                            mtu, info->sbc_max_frame_num, info->buf_size);

            if (info->buffer == NULL)
            {
                APP_PRINT_INFO0("src_calc_a2dp_max_frame: can not alloc buffer !!!");
            }
        }
        else
        {

        }
    }

}

static void src_dev_handle_voice_data(uint8_t index, void *buf, uint16_t len)
{
    //bool ret = false;
    //uint8_t msbc_buf[60] = {0};

    //const uint8_t header[4] = {0x08, 0x38, 0xc8, 0xf8};
    static uint8_t header_index = 0;

    T_APP_DEV *sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("src_dev_handle_voice_data: %d %d %d", index, len, header_index);

    if ((len == 57) && (sink_dev->sco_state == SCO_SETUP_STATE_CONNECTED))
    {
        //msbc_buf[0] = 0x01;
        ///msbc_buf[1] = header[header_index++];

        //memcpy(msbc_buf + 2, buf, len);

        //ret = bt_sco_data_send(sink_dev->remote_info.bd_addr, msbc_buf, 60);
        //APP_PRINT_INFO1("src_dev_handle_voice_data: %d", ret);
#ifdef AUDIO_DELAY_TEST_SCO_DS
        if (first_sco_ds_enc_pkt)
        {
            Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
            Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
            first_sco_ds_enc_pkt = false;
        }
#endif
        bt_sco_data_send(sink_dev->remote_info.bd_addr, header_index, buf, len);
        header_index++;
        //header_index %= 4;
    }
}

static void src_dev_handle_read_data(uint8_t index,
                                     uint16_t seq_num, uint8_t frame_num, void *buf, uint16_t len)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_SRC_STREAM_INFO *stream_info = &sink_dev->stream_info;
    uint8_t *buf_in = (uint8_t *)buf;
    uint16_t buf_offset = 0;
    bool ret = false;

    //APP_PRINT_INFO4("src_dev_handle_read_data: index %d, seq %d, frame_num %d, len %d",
    //                index, seq_num, frame_num, len);


    APP_PRINT_INFO3("src_dev_handle_read_data state %x buffer %x credits %x", stream_info->stream_state,
                    stream_info->buffer, stream_info->credits);
    if ((stream_info->stream_state == A2DP_STREAM_STATE_STARTED) && (stream_info->buffer != NULL))
    {
        sink_dev->stream_frame_count++;

        if (stream_info->codec_type == BT_A2DP_CODEC_TYPE_SBC)
        {
            while (len >= stream_info->sbc_frame_size)
            {
                memcpy(stream_info->buffer + stream_info->sbc_offset,
                       buf_in + buf_offset, stream_info->sbc_frame_size);

                stream_info->sbc_offset += stream_info->sbc_frame_size;
                buf_offset += stream_info->sbc_frame_size;
                len -= stream_info->sbc_frame_size;

                if (stream_info->sbc_offset == stream_info->buf_size)
                {
#ifdef AUDIO_DELAY_TEST_MUSIC
                    if (first_music_enc_pkt)
                    {
                        Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                        Pad_Config(P3_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                        first_music_enc_pkt = false;
                    }
#endif
                    //APP_PRINT_TRACE6("heap memory scan main end:data on %d, data off %d, buffer on %d, DTCM0 %d ITCM1 %d DSPSHARE %d",
                    //                 os_mem_peek(RAM_TYPE_DATA_ON), os_mem_peek(RAM_TYPE_DATA_OFF),
                    //                 os_mem_peek(RAM_TYPE_BUFFER_ON), os_mem_peek(RAM_TYPE_DTCM0),
                    //                 os_mem_peek(RAM_TYPE_ITCM1), os_mem_peek(RAM_TYPE_DSPSHARE));
                    if (stream_info->credits)
                    {
                        ret = bt_a2dp_stream_data_send(sink_dev->remote_info.bd_addr,
                                                       stream_info->a2dp_seq_num,
                                                       stream_info->a2dp_timestamp,
                                                       stream_info->sbc_max_frame_num,
                                                       stream_info->buffer,
                                                       stream_info->buf_size);

                        if (ret)
                        {
                            stream_info->credits--;
                        }
                        else
                        {
                            APP_PRINT_ERROR1("src_dev_handle_read_data: failed to sent for %s",
                                             TRACE_BDADDR(sink_dev->remote_info.bd_addr));
                        }
                    }
                    else
                    {
                        APP_PRINT_ERROR1("src_dev_handle_read_data: no credits for %s",
                                         TRACE_BDADDR(sink_dev->remote_info.bd_addr));
                    }
                    stream_info->a2dp_seq_num++;
                    stream_info->a2dp_timestamp += stream_info->sbc_max_frame_num *
                                                   sink_dev->codec_info.attr.sbc.block_length *
                                                   sink_dev->codec_info.attr.sbc.subband_num;
                    stream_info->sbc_offset = 0;
                }
            }
        }
        else
        {

        }
    }
}

bool usb_audio_data_cb(uint8_t *buf, uint16_t len, uint16_t frame_num)
{
    uint8_t i;

    if ((buf != NULL) && (len > 0) && (frame_num > 0))
    {
        if (len == 57)
        {
            for (i = 0; i < SINK_DEV_NUM_MAX; i++)
            {
                src_dev_handle_voice_data(i, buf, 57);
            }
        }
        else
        {
            for (i = 0; i < SINK_DEV_NUM_MAX; i++)
            {
                src_dev_handle_read_data(i, 0, frame_num, buf, len);
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

/*
static bool sbc_audio_read_cb(uint32_t *timestamp, uint16_t *seq_num, uint8_t  *frame_num,
                              void *buf, uint16_t  required_len, uint16_t *actual_len)
{
    uint8_t i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        src_dev_handle_read_data(i, *seq_num, *frame_num, buf, required_len);
    }

    *actual_len = required_len;
    return true;
}
*/


static void src_handle_a2dp_open(uint8_t *bd_addr, uint16_t remote_mtu)
{
    uint8_t index = 0;
    uint32_t bond_flag = 0;
    T_APP_DEV *sink_dev = &sink_dev_list[0];

    APP_PRINT_INFO2("src_handle_a2dp_open: %s mtu %d",
                    TRACE_BDADDR(bd_addr), remote_mtu);

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev_list[index].stream_info.a2dp_seq_num = 0;
        sink_dev_list[index].stream_info.a2dp_timestamp = 0;
        a2dp_channel_ready = true;
        src_init_a2dp_stream_info(index, remote_mtu);

#if F_BT_LEGACY_APP_KEY_MANAGER
        bond_flag = app_src_legacy_get_bond_flag_by_index(index);

        if (!((sink_dev->sco_state == SCO_SETUP_STATE_CONNECTED) && (bond_flag & APP_SRC_BOND_FLAG_A2DP)))
#else
        if (!(app_usb_audio_is_us_streaming() &&
              bt_bond_flag_get(bd_addr, &bond_flag) &&
              (bond_flag & APP_SRC_BOND_FLAG_A2DP)))
#endif
        {
#ifdef BTDONGLE_BQB_MODE_ENABLE
            if (app_is_bqb_mode())
            {
                app_a2dp_opened = true;
            }
#endif
            if (app_cfg_const.dongle_media_device)
            {
                app_usb_audio_music_create(sink_dev_list[index].codec_info);
                if (avrcp_is_connected)
                {
                    if (bt_a2dp_stream_start_req(bd_addr))
                    {
                        app_stop_timer(&timer_idx_usb_stream_restart);
                        sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STARTING;
                        bt_sniff_mode_disable(sink_dev->remote_info.bd_addr);
                    }
                }
            }
            else
            {
                if (app_get_usb_ds_state() && (!silent_detect_finished))
                {
                    app_usb_audio_music_create(sink_dev_list[index].codec_info);
                    if (avrcp_is_connected)
                    {
                        if (bt_a2dp_stream_start_req(bd_addr))
                        {
                            app_stop_timer(&timer_idx_usb_stream_restart);
                            sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STARTING;
                            bt_sniff_mode_disable(sink_dev->remote_info.bd_addr);
                        }
                    }
                }
            }
        }
    }
}

static uint8_t src_get_stream_dev_num(void)
{
    uint8_t num = 0;
    int i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].stream_info.stream_state == A2DP_STREAM_STATE_STARTED)
        {
            num++;
        }
    }

    return num;
}

static void src_handle_a2dp_stream_start_ind(uint8_t *bd_addr)
{
    uint8_t index = 0;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        bt_a2dp_stream_start_cfm(bd_addr, true);

        bt_avrcp_play_status_change_req(bd_addr, BT_AVRCP_PLAY_STATUS_PLAYING);
        sink_dev_list[index].stream_info.stream_state = A2DP_STREAM_STATE_STARTED;
        //app_src_sbc_voice_start_capture(sink_dev_list[index].codec_info,
        //                                sbc_audio_read_cb);
#ifdef AUDIO_DELAY_TEST_MUSIC
        first_music_enc_pkt = true;
#endif
        //app_usb_audio_music_create(sink_dev_list[index].codec_info);

        app_start_timer(&sink_dev_list[index].timer_idx_stream_idle, "stream_idle",
                        src_policy_timer_id, SRC_TIMER_ID_STREAM_IDLE_SINK0 + index, 0, false,
                        SRC_STREAMING_IDLE_DETECT_TIMEOUT);

//        src_stream_state_notify_cfu();
    }
}

static void src_handle_a2dp_stream_start_rsp(uint8_t *bd_addr)
{
    uint8_t index = 0;
    T_APP_DEV *sink_dev = NULL;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        //bt_a2dp_stream_start_cfm(bd_addr, true);
        sink_dev = &sink_dev_list[index];

        APP_PRINT_INFO4("src_handle_a2dp_stream_start_rsp: %s, %d, %d, %d",
                        TRACE_BDADDR(bd_addr),
                        sink_dev->stream_info.stream_state,
                        sink_dev->hfp_codec_selected,
                        sink_dev->sco_state);

        if (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_STARTING)
        {
            bt_avrcp_play_status_change_req(bd_addr, BT_AVRCP_PLAY_STATUS_PLAYING);
            sink_dev_list[index].stream_info.stream_state = A2DP_STREAM_STATE_STARTED;
            //app_src_sbc_voice_start_capture(sink_dev_list[index].codec_info,
            //                                sbc_audio_read_cb);
#ifdef AUDIO_DELAY_TEST_MUSIC
            first_music_enc_pkt = true;
#endif

            if ((sink_dev->sco_state == SCO_SETUP_STATE_IDLE) && (!sink_dev->hfp_codec_selected))
            {

                app_usb_audio_music_create(sink_dev_list[index].codec_info);

                app_start_timer(&sink_dev_list[index].timer_idx_stream_idle, "stream_idle",
                                src_policy_timer_id, SRC_TIMER_ID_STREAM_IDLE_SINK0 + index, 0, false,
                                SRC_STREAMING_IDLE_DETECT_TIMEOUT);

//                src_stream_state_notify_cfu();
            }
            else
            {
                if (bt_a2dp_stream_suspend_req(sink_dev->remote_info.bd_addr))
                {
                    sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STOPPING;
                }
            }
        }
    }
}

static void src_handle_a2dp_stream_data_rsp(uint8_t *bd_addr)
{
    uint8_t index = 0;
    T_APP_DEV *sink_dev = NULL;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        sink_dev->stream_info.credits++;

        if (sink_dev->stream_info.credits > SRC_A2DP_STREAM_MAX_CREDITS)
        {
            sink_dev->stream_info.credits = SRC_A2DP_STREAM_MAX_CREDITS;
        }
    }
}

static void src_handle_a2dp_stream_stop(uint8_t *bd_addr)
{
    uint8_t index = 0;
    T_APP_DEV *sink_dev = NULL;


    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_IDLE;
        sink_dev->stream_info.sbc_offset = 0;
        sink_dev->stream_frame_count = 0;
        sink_dev->stream_idle_count = 0;
        sink_dev->stream_info.a2dp_seq_num = 0;
#ifdef BTDONGLE_BQB_MODE_ENABLE
        if (app_is_bqb_mode())
        {
            app_a2dp_opened = false;
        }
#endif
        app_stop_timer(&sink_dev->timer_idx_stream_idle);

//        src_stream_state_notify_cfu();

        if ((sink_dev->sco_state == SCO_SETUP_STATE_IDLE) && (!sink_dev->hfp_codec_selected))
        {
            bt_sniff_mode_enable(sink_dev->remote_info.bd_addr, 784, 816, 0, 0);
        }
    }

    //stop stream if no streaming device exist
    if (src_get_stream_dev_num() == 0)
    {
        //app_src_sbc_voice_stop_capture();
        //app_usb_audio_music_destroy();
        bt_avrcp_play_status_change_req(bd_addr, BT_AVRCP_PLAY_STATUS_PAUSED);
    }
}

static void src_handle_a2dp_stream_close(uint8_t *bd_addr)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        a2dp_channel_ready = false;
        sink_dev = &sink_dev_list[index];

        sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_IDLE;
        sink_dev->stream_info.sbc_offset = 0;
        sink_dev->stream_frame_count = 0;
        sink_dev->stream_idle_count = 0;
        sink_dev->stream_info.credits = 0;
        sink_dev->stream_info.a2dp_seq_num = 0;
        sink_dev->stream_info.a2dp_timestamp = 0;
#ifdef BTDONGLE_BQB_MODE_ENABLE
        if (app_is_bqb_mode())
        {
            app_a2dp_opened = false;
        }
#endif
        app_stop_timer(&sink_dev->timer_idx_stream_idle);

        if (sink_dev->stream_info.buffer != NULL)
        {
            os_mem_free(sink_dev->stream_info.buffer);
            sink_dev->stream_info.buffer = NULL;
        }

//        src_stream_state_notify_cfu();
    }

    //stop stream if no streaming device exist
    if (src_get_stream_dev_num() == 0)
    {
        //app_src_sbc_voice_stop_capture();
        app_usb_audio_music_destroy();
        bt_avrcp_play_status_change_req(bd_addr, BT_AVRCP_PLAY_STATUS_STOPPED);
    }
}

void src_handle_a2dp_stream_suspend(void)
{
    T_APP_DEV *sink_dev = NULL;
    sink_dev = &sink_dev_list[0];
    T_APP_BR_LINK *p_link = app_find_br_link(sink_dev->remote_info.bd_addr);
    if (!p_link)
    {
        return;
    }

    APP_PRINT_INFO1("src_handle_a2dp_stream_suspend: stream state %d",
                    sink_dev->stream_info.stream_state);

    if ((p_link->connected_profile & A2DP_PROFILE_MASK) &&
        (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_STARTED))
    {

        silent_detect_finished = false;
        app_stop_timer(&timer_idx_usb_stream_restart);
        APP_PRINT_INFO0("src_handle_a2dp_stream_suspend send a2dp suspend");
        if (bt_a2dp_stream_suspend_req(sink_dev->remote_info.bd_addr))
        {
            sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STOPPING;
        }
    }
}

void src_handle_profile_disconn_cmpl(uint8_t *bd_addr, uint16_t cause, uint32_t profile_mask)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);

    del_connected_profile(bd_addr, profile_mask);
    if (profile_mask == HFP_PROFILE_MASK)
    {
        src_led_set_hfp_disconnected();
    }

    APP_PRINT_INFO3("src_handle_profile_disconn_cmpl: %s, profile_mask %08x, cause %04x",
                    TRACE_BDADDR(bd_addr), profile_mask, cause);

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        APP_PRINT_INFO2("src_handle_profile_disconn_cmpl: connected_mask %08x, dev_state %02x",
                        p_link != NULL ? p_link->connected_profile : 0, sink_dev->dev_state);

        if (profile_mask == A2DP_PROFILE_MASK)
        {
            if (sink_dev->stream_info.buffer != NULL)
            {
                os_mem_free(sink_dev->stream_info.buffer);
                sink_dev->stream_info.buffer = NULL;
            }
#ifdef BTDONGLE_BQB_MODE_ENABLE
            if (app_is_bqb_mode())
            {
                app_a2dp_opened = false;
            }
#endif
            memset(&sink_dev->stream_info, 0, sizeof(T_APP_SRC_STREAM_INFO));
            bt_avrcp_play_status_change_req(bd_addr, BT_AVRCP_PLAY_STATUS_STOPPED);
        }

        if (profile_mask == AVRCP_PROFILE_MASK)
        {
            avrcp_is_connected = false;
        }

        switch (sink_dev->dev_state)
        {
        case STATE_DEV_RECONNECTING:
        case STATE_DEV_CONNECTING:
            {
                if ((cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                    (cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)) ||
                    (cause == (HCI_ERR | HCI_ERR_AUTHEN_FAIL)))
                {
                    APP_PRINT_INFO0("src_handle_profile_disconn_cmpl: wait for disconnected");
                }
#if (MS_SURFACE == 1)
                else if (cause == (L2C_ERR | L2C_ERR_SECURITY_BLOCK))
#else
                else if ((cause == (L2C_ERR | L2C_ERR_SECURITY_BLOCK)) ||
                         (cause == (L2C_ERR | L2C_ERR_VND_TIMEOUT)))
#endif
                {
                    gap_br_send_acl_disconn_req(bd_addr);
                }
                else if (sink_dev->profile_mask_doing == profile_mask)
                {
                    src_profile_conn_finish(index);
                    src_dev_profile_conn(index);
                }
            }
            break;

        case STATE_DEV_CONNECTED:
            {
//                app_usb_hid_connection_state_notify(DEV_SLOT_HS,
//                                                    src_get_audio_device_conn_state(),
//                                                    src_get_audio_device_profile_state());
                app_bt_connection_state_report_event(src_get_audio_device_conn_state(),
                                                     src_get_audio_device_profile_state());
            }
            break;

        case STATE_DEV_CLEANUP:
            {
                if ((cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT)) ||
                    (cause == (HCI_ERR | HCI_ERR_LMP_RESPONSE_TIMEOUT)))
                {
                    APP_PRINT_INFO0("src_handle_profile_disconn_cmpl: wait for disconnected");
                }
                else if (cause == (L2C_ERR | L2C_ERR_SECURITY_BLOCK))
                {
                    gap_br_send_acl_disconn_req(bd_addr);
                }
                else
                {
                    if ((p_link != NULL) && (p_link->connected_profile == 0))
                    {
                        gap_br_send_acl_disconn_req(bd_addr);
                    }
                }
            }
            break;

        default:
            break;
        }
    }

    if (src_get_stream_dev_num() == 0)
    {
        //app_src_sbc_voice_stop_capture();
        app_usb_audio_music_destroy();
    }
}

static void app_src_handle_sco_connected(T_BT_EVENT_PARAM_SCO_CONN_CMPL param)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;
    T_APP_BR_LINK *p_link = NULL;

    APP_PRINT_INFO6("app_src_handle_sco_connected: %s %04x-%04x-%02x-%02x-%02x",
                    TRACE_BDADDR(param.bd_addr),
                    param.handle, param.cause, param.air_mode,
                    param.rx_pkt_len, param.tx_pkt_len);

    p_link = app_find_br_link(param.bd_addr);
    if (p_link != NULL)
    {
        if (param.cause == 0)
        {
            p_link->sco_handle = param.handle;
        }
    }

    if (src_policy_find_dev_index_by_addr(param.bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];
        sink_dev->sco_state = SCO_SETUP_STATE_CONNECTED;

        if (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_STARTED)
        {
            if (bt_a2dp_stream_suspend_req(sink_dev->remote_info.bd_addr))
            {
                sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STOPPING;
            }
        }

        app_usb_audio_music_destroy();

        app_usb_audio_msbc_ds_create();
        app_usb_audio_msbc_us_create();

//        src_stream_state_notify_cfu();

#ifdef SRC_ENABLE_AUDIO_US_PKT_LOGS_STAT
        src_enable_audio_us_pkt_loss_count(true);
#endif
    }

#ifdef  BTDONGLE_BQB_MODE_ENABLE
    if (app_is_bqb_mode())
    {
        extern void app_bqb_ring_usb_send(uint8_t *buf, uint16_t size);
        uint8_t sco_con_buf[] = "SCO Connected!";
        uint8_t sco_rjt_buf[] = "SCO Connect Failed!";
        if (param.cause)
        {
            app_bqb_ring_usb_send(sco_rjt_buf, sizeof(sco_rjt_buf));
        }
        else
        {
            app_bqb_ring_usb_send(sco_con_buf, sizeof(sco_con_buf));
        }
    }
#endif
}

#ifdef  BTDONGLE_BQB_MODE_ENABLE
static void app_src_handle_sco_conn_ind(T_BT_EVENT_PARAM_SCO_CONN_IND param)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;

    if (src_policy_find_dev_index_by_addr(param.bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        APP_PRINT_INFO1("app_src_handle_sco_conn_ind: %d", sink_dev->sco_state);
        if (sink_dev->sco_state == SCO_SETUP_STATE_CONNECTED)
        {
            bt_hfp_ag_audio_connect_cfm(param.bd_addr, false);
        }
        else
        {
            bt_hfp_ag_audio_connect_cfm(param.bd_addr, true);
            //sink_dev->sco_state = true;
        }
    }
}
#endif

static void app_src_handle_sco_disconnected(T_BT_EVENT_PARAM_SCO_DISCONNECTED param)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;
    T_APP_BR_LINK *p_link = NULL;

    APP_PRINT_INFO2("app_src_handle_sco_disconnected: %s %04x",
                    TRACE_BDADDR(param.bd_addr), param.cause);

    //app_bt_sniffing_stop(param->bt_sco_disconnected.bd_addr, BT_SNIFFING_TYPE_SCO);
    p_link = app_find_br_link(param.bd_addr);
    if (p_link != NULL)
    {
        p_link->sco_handle = 0;
        //app_resume_a2dp_link_stream(p_link->id);
    }

    if (src_policy_find_dev_index_by_addr(param.bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];
        sink_dev->sco_state = SCO_SETUP_STATE_IDLE;
        sink_dev->hfp_codec_selected = false;
        sink_dev->voice_data.h2_header_offset = 0;

        bt_sniff_mode_enable(sink_dev->remote_info.bd_addr, 784, 816, 0, 0);

        app_usb_audio_msbc_ds_destroy();
        app_usb_audio_msbc_us_destroy();
        if (app_get_usb_ds_state())
        {
            app_handle_music_data_notify();
        }
//        src_stream_state_notify_cfu();
#ifdef SRC_ENABLE_AUDIO_US_PKT_LOGS_STAT
        src_enable_audio_us_pkt_loss_count(false);
#endif
    }
#ifdef  BTDONGLE_BQB_MODE_ENABLE
    if (app_is_bqb_mode())
    {
        extern void app_bqb_ring_usb_send(uint8_t *buf, uint16_t size);
        uint8_t buf[] = "SCO Disconnected!";
        app_bqb_ring_usb_send(buf, sizeof(buf));
    }
#endif
}

static void src_handle_sco_data_ind(T_BT_EVENT_PARAM_SCO_DATA_IND param)
{
    uint8_t index = 0;

    uint8_t *p_sco = param.p_data;

    APP_PRINT_INFO3("src_handle_sco_data_ind len %d, status %d data %b", param.length, param.status,
                    TRACE_BINARY(5, param.p_data));
    if ((p_sco == NULL) || (param.length != 57))
    {
        return;
    }

    if (src_policy_find_dev_index_by_addr(param.bd_addr, &index))
    {
        if (param.status != BT_SCO_PKT_STATUS_OK)
        {

#ifdef SRC_ENABLE_AUDIO_US_PKT_LOGS_STAT
            if (pkt_loss_count_flag)
            {
                if (param.status == BT_SCO_PKT_STATUS_INVALID)
                {
                    pkt_invalid++;
                    APP_PRINT_INFO1("src_handle_sco_data_ind: invalid %d", pkt_invalid);
                }
                else if (param.status == BT_SCO_PKT_STATUS_NO_DATA)
                {
                    pkt_no_data++;
                    APP_PRINT_INFO1("src_handle_sco_data_ind: no_data %d", pkt_no_data);
                }
                else if (param.status == BT_SCO_PKT_STATUS_PARTIAL_LOST)
                {
                    pkt_partial_lost++;
                    APP_PRINT_INFO1("src_handle_sco_data_ind: partial_lost %d", pkt_partial_lost);
                }
            }
#endif
        }
        app_usb_audio_msbc_fill_us(p_sco, param.length, param.status);
    }
}

#ifdef SRC_ENABLE_AUDIO_US_PKT_LOGS_STAT
static void src_enable_audio_us_pkt_loss_count(bool flag)
{
    pkt_loss_count_flag = flag;
    APP_PRINT_INFO4("src_enable_audio_us_pkt_loss_count: flag %d, invalid %d, no_data %d, partial_lost %d",
                    pkt_loss_count_flag, pkt_invalid, pkt_no_data, pkt_partial_lost);

    pkt_invalid = 0;
    pkt_no_data = 0;
    pkt_partial_lost = 0;
}
#endif


static void src_handle_hfp_codec_selected(T_BT_EVENT_PARAM_HFP_AG_CODEC_TYPE_SELECTED param)
{
    T_APP_DEV *sink_dev = NULL;
    uint8_t index = 0;

    if (src_policy_find_dev_index_by_addr(param.bd_addr, &index))
    {
        sink_dev = &sink_dev_list[index];

        sink_dev->hfp_codec_selected = true;

        bt_sniff_mode_disable(sink_dev->remote_info.bd_addr);

        if (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_STARTED)
        {
            if (bt_a2dp_stream_suspend_req(param.bd_addr))
            {
                sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STOPPING;
            }
        }

        //app_usb_audio_music_destroy();
    }
}

#if 0
static uint32_t get_profile_mask_from_bond_flag(uint16_t bond_flag)
{
    uint32_t profile_mask = 0;

    if (bond_flag & APP_SRC_BOND_FLAG_A2DP)
    {
        profile_mask |= A2DP_PROFILE_MASK;
    }

    if (bond_flag & APP_SRC_BOND_FLAG_SPP)
    {
        profile_mask |= SPP_PROFILE_MASK;
    }

    if (bond_flag & APP_SRC_BOND_FLAG_HFP)
    {
        profile_mask |= HFP_PROFILE_MASK;
    }
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    if (bond_flag & APP_SRC_BOND_FLAG_ASP)
    {
        profile_mask |= ASP_PROFILE_MASK;
    }
#endif
    return profile_mask;
}
#endif

static uint32_t get_bond_flag_from_profile_mask(uint32_t profile_mask)
{
    uint32_t bond_flag = 0;

    if (profile_mask & A2DP_PROFILE_MASK)
    {
        bond_flag |= APP_SRC_BOND_FLAG_A2DP;
    }

    if (profile_mask & SPP_PROFILE_MASK)
    {
        bond_flag |= APP_SRC_BOND_FLAG_SPP;
    }

    if (profile_mask & HFP_PROFILE_MASK)
    {
        bond_flag |= APP_SRC_BOND_FLAG_HFP;
    }

    return bond_flag;
}

static uint32_t load_doing_profile(uint32_t remain_profile_mask)
{
    int i;

    for (i = 0; i < sizeof(src_supported_profiles) / sizeof(src_supported_profiles[0]); i++)
    {
        if (src_supported_profiles[i] & remain_profile_mask)
        {
            return src_supported_profiles[i];
        }
    }

    return 0;
}

static bool get_profile_uuid_from_mask(uint32_t profile_mask,
                                       T_GAP_UUID_DATA *uuid,
                                       T_GAP_UUID_TYPE *type)
{
    if (!uuid || !type)
    {
        APP_PRINT_ERROR0("get_profile_uuid_from_mask(): Invalid paramter");
        return false;
    }

    switch (profile_mask)
    {
    case A2DP_PROFILE_MASK:
        {
            uuid->uuid_16 = UUID_AUDIO_SINK;
            *type = GAP_UUID16;
        }
        break;

    case AVRCP_PROFILE_MASK:
        {
            uuid->uuid_16 = UUID_AV_REMOTE_CONTROL_TARGET;
            *type = GAP_UUID16;
        }
        break;

    case SPP_PROFILE_MASK:
        {
            uuid->uuid_16 = UUID_SERIAL_PORT;
            *type = GAP_UUID16;
        }
        break;

    case HFP_PROFILE_MASK:
        {
            uuid->uuid_16 = UUID_HANDSFREE;
            *type = GAP_UUID16;
        }
        break;
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    case ASP_PROFILE_MASK:
        {
            memcpy(uuid->uuid_128, UUID_ASP, 16);
            *type = GAP_UUID128;
        }
        break;
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
    case SPP_VENDER_PASSTHROUGH_PROFILE_MASK:
        {
            memcpy(uuid->uuid_128, UUID_VENDER_PASSTHROUGH, 16);
            *type = GAP_UUID128;
        }
        break;
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
    case SPP_AUDIO_PROFILE_MASK:
        {
            memcpy(uuid->uuid_128, UUID_SPP_AUDIO, 16);
            *type = GAP_UUID128;
        }
        break;
#endif
#if 0
    case MANUFACTURER_DATA_MASK:
        {
            memcpy(uuid->uuid_128, UUID_MANUFACTURER_DATA, 16);
            *type = GAP_UUID128;
        }
        break;
#endif
    default:
        return false;
    }

    switch (*type)
    {
    case GAP_UUID16:
        APP_PRINT_INFO1("profile uuid: 0x%04x", uuid->uuid_16);
        break;
    case GAP_UUID32:
        APP_PRINT_INFO1("profile uuid: 0x%08x", uuid->uuid_32);
        break;
    case GAP_UUID128:
        APP_PRINT_INFO1("profile uuid: %b", TRACE_BINARY(16, uuid->uuid_128));
        break;
    }

    return true;
}


static bool get_connected_profile(uint8_t *bd_addr, uint32_t *profile_masks)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (profile_masks != NULL)
        {
            *profile_masks = p_link->connected_profile;
        }
        return true;
    }

    return false;
}

static void add_conneted_profile(uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile |= profile_mask;
    }
}

static void del_connected_profile(uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_BR_LINK *p_link;

    p_link = app_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->connected_profile &= ~profile_mask;
    }
}

static void src_profile_conn_finish(uint8_t index)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    sink_dev->profile_mask_remain &= ~sink_dev->profile_mask_doing;
    sink_dev->profile_mask_doing = 0;
    sink_dev->sdp_info.is_found = false;
}

static void src_profile_conn_exit(uint8_t index)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    p_link = app_find_br_link(sink_dev->remote_info.bd_addr);
    if (p_link != NULL)
    {
        if ((p_link->connected_profile & sink_dev->profile_mask_plan) != 0)
        {
            //TBD: conn init from remote

            if (src_is_cur_dev(index))
            {
                src_handle_action_result(index, true);
            }
            else
            {
                //Not possible
                src_dev_state_set(index, STATE_DEV_CONNECTED);
            }
        }
        else
        {
            //cleanup when disconnected
            gap_br_send_acl_disconn_req(p_link->bd_addr);
        }
    }
}

static bool src_profile_search_start(uint8_t index)
{
    bool ret = false;
    T_GAP_UUID_DATA uuid;
    T_GAP_UUID_TYPE uuid_type = GAP_UUID16;
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    if (sink_dev->profile_mask_doing != DEVICE_INFO_MASK)
    {
        ret = get_profile_uuid_from_mask(sink_dev->profile_mask_doing, &uuid,
                                         &uuid_type);
        if (ret)
        {
            if (gap_br_start_sdp_discov(sink_dev->remote_info.bd_addr, uuid_type, uuid) == GAP_CAUSE_SUCCESS)
            {
                sink_dev->sdp_info.is_busy = true;
                ret = true;
            }
        }
    }
    else
    {
        if (gap_br_start_did_discov(sink_dev->remote_info.bd_addr) == GAP_CAUSE_SUCCESS)
        {
            sink_dev->sdp_info.is_busy = true;
            ret = true;
        }
    }

    return ret;
}

static void src_dev_profile_conn(uint8_t index)
{
    uint32_t connected_profiles;
    T_APP_DEV *sink_dev = &sink_dev_list[index];
    T_APP_BR_LINK *p_link = app_find_br_link(sink_dev->remote_info.bd_addr);

    APP_PRINT_INFO2("src_dev_profile_conn: start, %08x, %d",
                    sink_dev->profile_mask_remain,
                    sink_dev->profiles_retry_count);

RETRY:
    if (sink_dev->profile_mask_remain == 0)
    {
        if ((p_link != NULL) &&
            (p_link->connected_profile != TAR_CONN_PROFILES_MASK) &&
            (sink_dev->profiles_retry_count > 0))
        {
            src_profile_conn_finish(index);
            sink_dev->profile_mask_remain = p_link->connected_profile ^ TAR_CONN_PROFILES_MASK;
            sink_dev->profiles_retry_count--;
        }
        else
        {
            src_profile_conn_exit(index);
            return;
        }
    }
#ifdef BTDONGLE_BQB_MODE_ENABLE
    if (app_is_bqb_mode())
    {
        if (get_connected_profile(sink_dev->remote_info.bd_addr, &connected_profiles))
        {
            if (connected_profiles)
            {
                APP_PRINT_INFO1("src_dev_profile_conn: bqb mode connected profile %x", connected_profiles);
                src_profile_conn_exit(index);
                return;
            }
        }
    }
#endif
    sink_dev->profile_mask_doing = load_doing_profile(sink_dev->profile_mask_remain);

    if (get_connected_profile(sink_dev->remote_info.bd_addr, &connected_profiles))
    {
        if (connected_profiles & sink_dev->profile_mask_doing)
        {
            src_profile_conn_finish(index);
            goto RETRY;
        }
    }
#if 0
    if ((!sink_dev->a2dp_conn_setup_delay_once) && (sink_dev->profile_mask_doing == A2DP_PROFILE_MASK))
    {
        app_start_timer(&sink_dev->timer_idx_a2dp_conn_setup_delay, "a2dp_conn_delay",
                        src_policy_timer_id, SRC_TIMER_ID_A2DP_CONN_SETUP_DELAY_SINK0 + index, 0, false,
                        SRC_A2DP_CONN_SETUP_DELAY_TIMEOUT);

        sink_dev->a2dp_conn_setup_delay_once = true;

        return;
    }
#endif

    if (!src_profile_search_start(index))
    {
        src_profile_conn_finish(index);
        goto RETRY;
    }
}

static bool src_profile_conn(uint8_t *bd_addr, uint32_t profile_mask, T_APP_SRC_SDP_INFO sdp_info)
{
    bool ret = true;

    switch (profile_mask)
    {
    case A2DP_PROFILE_MASK:
        ret = bt_a2dp_connect_req(bd_addr, sdp_info.protocol_version, BT_A2DP_ROLE_SNK);
        break;

    case AVRCP_PROFILE_MASK:
        ret = bt_avrcp_connect_req(bd_addr);
        break;

    case SPP_PROFILE_MASK:
        ret = bt_spp_connect_req(bd_addr,
                                 sdp_info.server_channel,
                                 1012,
                                 7,
                                 sdp_info.local_server_channel);
        break;

    case HFP_PROFILE_MASK:
#ifdef  BTDONGLE_BQB_MODE_ENABLE
        if (app_is_bqb_mode())
        {
            extern bool wbs_test_case;
            if (!wbs_test_case)
            {
                ret = bt_hfp_ag_connect_req(bd_addr, sdp_info.server_channel, true);
            }
            wbs_test_case = false;
        }
        else
#endif
        {
            APP_PRINT_INFO1("bt_hfp_ag_connect_req ch %x", sdp_info.server_channel);
            ret = bt_hfp_ag_connect_req(bd_addr, sdp_info.server_channel, true);
        }
        break;
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    case ASP_PROFILE_MASK:
        /* profile conn cmpl is called in asp cback */
        ret = bt_rfc_conn_req(bd_addr, RFC_ASP_CHANN_NUM,
                              sdp_info.server_channel, 1012, 7);
        break;
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
    case SPP_VENDER_PASSTHROUGH_PROFILE_MASK:
        /* profile conn cmpl is called in gaia cback */
        ret = bt_rfc_conn_req(bd_addr, RFC_VENDER_PASSTHROUGH_CHANN_NUM,
                              sdp_info.server_channel, 1012, 7);
        break;
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
    case SPP_AUDIO_PROFILE_MASK:
        ret = bt_rfc_conn_req(bd_addr, 17,
                              sdp_info.server_channel, 1012, 100);
        break;
#endif
#if 0
    case MANUFACTURER_DATA_MASK:
        ret = false;
        break;
#endif
    case DEVICE_INFO_MASK:
        ret = false;
        break;

    default:
        ret = false;
        break;
    }

    return ret;
}

static void src_profile_disconn(uint8_t *bd_addr, uint32_t profile_mask)
{
    APP_PRINT_INFO2("src_profile_disconn: address %s, profile 0x%08x",
                    TRACE_BDADDR(bd_addr), profile_mask);

    if (profile_mask & A2DP_PROFILE_MASK)
    {
        bt_a2dp_disconnect_req(bd_addr);
    }

    if (profile_mask & AVRCP_PROFILE_MASK)
    {
        bt_avrcp_disconnect_req(bd_addr);
    }

    if (profile_mask & SPP_PROFILE_MASK)
    {
        bt_spp_disconnect_all_req(bd_addr);
    }

    if (profile_mask & HFP_PROFILE_MASK)
    {
        bt_hfp_ag_disconnect_req(bd_addr);
    }
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    if (profile_mask & ASP_PROFILE_MASK)
    {
        bt_rfc_disconn_req(bd_addr, RFC_ASP_CHANN_NUM);
    }
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
    if (profile_mask & SPP_VENDER_PASSTHROUGH_PROFILE_MASK)
    {
        bt_rfc_disconn_req(bd_addr, RFC_VENDER_PASSTHROUGH_CHANN_NUM);
    }
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
    if (profile_mask & SPP_AUDIO_PROFILE_MASK)
    {
        bt_rfc_disconn_req(bd_addr, 17);
    }
#endif
}

static void src_dev_conn(uint8_t index, uint8_t *bd_addr, uint32_t profile_mask)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_dev_conn: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO2("src_dev_conn: address %s, profile %08x",
                    TRACE_BDADDR(bd_addr), profile_mask);

    if (sink_dev->bt_state == STATE_BT_ACL_CONNECTED)
    {
        APP_PRINT_INFO2("src_dev_conn: index %d, addr %s, acl already conn, wait timeout",
                        index, TRACE_BDADDR(bd_addr));
        return;
    }

    memcpy(sink_dev->remote_info.bd_addr, bd_addr, 6);

    src_bt_state_set(index, STATE_BT_PAGING);

    sink_dev->profile_mask_plan = profile_mask;
    sink_dev->profile_mask_remain = profile_mask;
    sink_dev->profiles_retry_count = SRC_PROFILES_RETRY_COUNT;
    //gap_br_start_did_discov(bd_addr);
    src_dev_profile_conn(index);
}

static void src_start_reconn(uint8_t index)
{
    uint8_t bd_addr[6] = {0, 0, 0, 0, 0, 0};
    //uint32_t bond_flag = 0;
    //uint32_t profile_mask = 0;

    src_set_active_sink_dev(index);

    src_check_set_bt_mode();

    src_is_reconnecting = true;

    if (!memcmp(sink_dev_list[index].remote_info.bd_addr, bd_addr, 6))
    {
        app_src_legacy_get_bond_addr_by_index(index, bd_addr);
        memcpy(sink_dev_list[index].remote_info.bd_addr, bd_addr, 6);
    }

#ifdef BTDONGLE_BQB_MODE_ENABLE
    if (app_is_bqb_mode())
    {
        src_dev_conn(index, sink_dev_list[index].remote_info.bd_addr,
                     A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK |
                     HFP_PROFILE_MASK);
    }
    else
#endif
    {
        src_dev_conn(index, sink_dev_list[index].remote_info.bd_addr,
                     A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK | HFP_PROFILE_MASK |
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
                     ASP_PROFILE_MASK |
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
                     SPP_VENDER_PASSTHROUGH_PROFILE_MASK |
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
                     SPP_AUDIO_PROFILE_MASK |
#endif
#if 0
                     MANUFACTURER_DATA_MASK |
#endif
                     DEVICE_INFO_MASK);
    }
}

static void src_start_conn(uint8_t index)
{
    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_start_conn: index %d is invalid", index);
        return;
    }

    src_set_active_sink_dev(index);

    src_check_set_bt_mode();
    src_bt_state_set(index, STATE_BT_INQUIRY);
    gap_br_start_inquiry(false, 10);
    app_scan_state_change_event(LEGACY_SCAN_ON);
}

static bool src_start_disconn(uint8_t *bd_addr)
{
    T_APP_BR_LINK *p_link = app_find_br_link(bd_addr);

    if (p_link != NULL)
    {
        if (p_link->connected_profile != 0)
        {
            src_profile_disconn(p_link->bd_addr, p_link->connected_profile);
            //start timer here
        }
        else
        {
            gap_br_send_acl_disconn_req(p_link->bd_addr);
        }

        return true;
    }

    return false;
}

static void src_handle_dev_pending_evt(uint8_t index, T_APP_SRC_KEY_EVENT pending_evt)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_handle_dev_pending_evt: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];
    APP_PRINT_INFO2("src_handle_dev_pending_evt pending_evt %d, state %d", pending_evt,
                    sink_dev->dev_state);
    switch (pending_evt)
    {
    case KEY_NON:
        {
            src_dev_cleanup(index);
        }
        break;

    case KEY_NEW_TARGET:
        {
            src_dev_cleanup_restore_pending_info(index);

            memcpy(sink_dev->remote_info.bd_addr, sink_dev->pending_info.remote_bd, 6);
            sink_dev->remote_info.cod = sink_dev->pending_info.cod;
            key_short_press_handle(index);
        }
        break;

    case KEY_SHORT:
        {
            key_short_press_handle(index);
        }
        break;

    case KEY_LONG:
        {
            key_long_press_handle(index);
        }
        break;

    default:
        break;
    }
}

static void src_handle_dev_cleanup(uint8_t index)
{
    T_APP_DEV *sink_dev;
    T_APP_SRC_KEY_EVENT pending_evt = KEY_NON;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("src_handle_dev_cleanup: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];
    pending_evt = sink_dev->pending_event;

    APP_PRINT_INFO3("src_handle_dev_cleanup: index %d, dev_state %04x, bt_state %04x",
                    index, sink_dev->dev_state, sink_dev->bt_state);

    switch (sink_dev->dev_state)
    {
    case STATE_DEV_IDLE:
        {
            if (sink_dev->bt_state == STATE_BT_ACL_CONNECTED)
            {
                src_dev_state_set(index, STATE_DEV_CLEANUP);
                gap_br_send_acl_disconn_req(sink_dev->remote_info.bd_addr);
            }
            else
            {
                src_handle_dev_pending_evt(index, sink_dev->pending_event);
                src_check_set_bt_mode();
            }
        }
        break;

    case STATE_DEV_RECONNECTING:
        {
            if (sink_dev->bt_state == STATE_BT_PAGING)
            {
                //wait for cancel complete
                src_dev_state_set(index, STATE_DEV_CLEANUP);
                gap_br_stop_sdp_discov(sink_dev->remote_info.bd_addr);
            }
            else if (sink_dev->bt_state == STATE_BT_ACL_CONNECTED)
            {
                //wait for disconnected
                src_dev_state_set(index, STATE_DEV_CLEANUP);
                gap_br_send_acl_disconn_req(sink_dev->remote_info.bd_addr);
            }
            else
            {
                //bt state idle, handle here
                src_handle_dev_pending_evt(index, pending_evt);
            }
        }
        break;

    case STATE_DEV_CONNECTING:
        {
            if (sink_dev->bt_state == STATE_BT_INQUIRY)
            {
                gap_br_stop_inquiry();
                app_scan_state_change_event(LEGACY_SCAN_OFF);
                src_dev_state_set(index, STATE_DEV_CLEANUP);
            }
            else if (sink_dev->bt_state == STATE_BT_PAGING)
            {
                //wait for cancel complete
                gap_br_stop_sdp_discov(sink_dev->remote_info.bd_addr);
                src_dev_state_set(index, STATE_DEV_CLEANUP);
            }
            else if (sink_dev->bt_state == STATE_BT_ACL_CONNECTED)
            {
                //wait for disconnected
                gap_br_send_acl_disconn_req(sink_dev->remote_info.bd_addr);
                src_dev_state_set(index, STATE_DEV_CLEANUP);
            }
            else
            {
                //bt state idle, handle here
                src_handle_dev_pending_evt(index, pending_evt);
            }
        }
        break;

    case STATE_DEV_CONNECTED:
        {
            if (src_start_disconn(sink_dev->remote_info.bd_addr))
            {
                //no need to set active device here
                src_dev_state_set(index, STATE_DEV_CLEANUP);

                //app_usb_hid_connection_state_notify(DEV_SLOT_HS, BT_CONN_STATE_DISCONNECTED, 0);
                app_bt_connection_state_report_event(BT_CONN_STATE_DISCONNECTED, 0);
            }
            else
            {
                APP_PRINT_ERROR0("src_handle_dev_cleanup: no device found");
            }
        }
        break;

    case STATE_DEV_CLEANUP:
        {
            APP_PRINT_WARN0("src_handle_dev_cleanup: already cleanup");
        }
        break;

    default:
        break;
    }
}

static void active_dev_handle_short_press_event(uint8_t index)
{
    T_APP_DEV *sink_dev;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("active_dev_handle_short_press_event: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("active_dev_handle_short_press_event: index %d, dev_state %04x, bt_state %04x",
                    index, sink_dev->dev_state, sink_dev->bt_state);

    switch (sink_dev->dev_state)
    {
    case STATE_DEV_IDLE:
        {
#if 0
            if (src_dev_is_bonded(index))
#endif
            {
                src_enter_reconnecting_state(index);
                src_start_reconn(index);
            }
        }
        break;

    case STATE_DEV_RECONNECTING:
        {
            APP_PRINT_WARN0("active_dev_handle_short_press_event: already reconnecting");
        }
        break;

    case STATE_DEV_CONNECTING:
        {
            //only handled if dev is bonded
            if (src_dev_is_bonded(index))
            {
                sink_dev->pending_event = KEY_SHORT;
                src_handle_dev_cleanup(index);
            }
            else
            {
                APP_PRINT_INFO0("active_dev_handle_short_press_event: no dev bonded, still connecting");
            }
        }
        break;

    case STATE_DEV_CONNECTED:
        {
            APP_PRINT_WARN0("active_dev_handle_short_press_event: disconnecting");
            src_handle_dev_cleanup(index);
        }
        break;

    case STATE_DEV_CLEANUP:
        {
            APP_PRINT_INFO1("active_dev_handle_short_press_event: current pending event %d",
                            sink_dev->pending_event);

            sink_dev->pending_event = KEY_SHORT;
        }
        break;

    default:
        break;
    }
}

static void non_active_dev_handle_short_press_event(uint8_t index)
{
    T_APP_DEV *sink_dev;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("non_active_dev_handle_short_press_event: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("non_active_dev_handle_short_press_event: index %d, dev_state %04x, bt_state %04x",
                    index, sink_dev->dev_state, sink_dev->bt_state);

    switch (sink_dev->dev_state)
    {
    case STATE_DEV_IDLE:
        {
            if (src_dev_is_bonded(index))
            {
                src_enter_reconnecting_state(index);
            }
        }
        break;

    case STATE_DEV_RECONNECTING:
        {
            APP_PRINT_WARN0("non_active_dev_handle_short_press_event: already reconnecting");
        }
        break;

    case STATE_DEV_CONNECTING:
        {
            //only handled if dev is bonded
            if (src_dev_is_bonded(index))
            {
                sink_dev->pending_event = KEY_SHORT;
                src_handle_dev_cleanup(index);
            }
            else
            {
                APP_PRINT_INFO0("active_dev_handle_short_press_event: no dev bonded, still connecting");
            }
        }
        break;

    case STATE_DEV_CONNECTED:
        {
            APP_PRINT_WARN0("active_dev_handle_short_press_event: disconnecting");
            src_handle_dev_cleanup(index);
        }
        break;

    case STATE_DEV_CLEANUP:
        {
            APP_PRINT_INFO1("active_dev_handle_short_press_event: current pending event %d",
                            sink_dev->pending_event);

            sink_dev->pending_event = KEY_SHORT;
        }
        break;

    default:
        break;
    }
}

void key_short_press_handle(uint8_t index)
{
    if (shutdown)
    {
        APP_PRINT_WARN1("key_short_press_handle: index %d not handled in shutdown", index);
        return;
    }

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_INFO1("key_short_press_handle: invalid %d", index);
        return;
    }

    if (!src_is_busy() || src_is_cur_dev(index))
    {
        active_dev_handle_short_press_event(index);
    }
    else
    {
        non_active_dev_handle_short_press_event(index);
    }
}

static void active_dev_handle_long_press_event(uint8_t index)
{
    T_APP_DEV *sink_dev;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("active_dev_handle_long_press_event: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("active_dev_handle_long_press_event: index %d, dev_state %04x, bt_state %04x",
                    index, sink_dev->dev_state, sink_dev->bt_state);

    switch (sink_dev->dev_state)
    {
    case STATE_DEV_IDLE:
        {
            src_enter_connecting_state(index);
            src_start_conn(index);
        }
        break;

    case STATE_DEV_RECONNECTING:
        {
            sink_dev->pending_event = KEY_LONG;
            src_handle_dev_cleanup(index);
        }
        break;

    case STATE_DEV_CONNECTING:
        {
            APP_PRINT_WARN0("active_dev_handle_long_press_event: already connecting");
        }
        break;

    case STATE_DEV_CONNECTED:
        {
            APP_PRINT_WARN0("active_dev_handle_long_press_event: disconnecting");
            sink_dev->pending_event = KEY_LONG;
            src_handle_dev_cleanup(index);
        }
        break;

    case STATE_DEV_CLEANUP:
        {
            APP_PRINT_INFO1("active_dev_handle_long_press_event: current pending event %d",
                            sink_dev->pending_event);

            sink_dev->pending_event = KEY_LONG;
        }
        break;

    default:
        break;
    }
}

static void non_active_dev_handle_long_press_event(uint8_t index)
{
    T_APP_DEV *sink_dev;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("non_active_dev_handle_long_press_event: index %d is invalid", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO3("non_active_dev_handle_long_press_event: index %d, dev_state %04x, bt_state %04x",
                    index, sink_dev->dev_state, sink_dev->bt_state);

    switch (sink_dev->dev_state)
    {
    case STATE_DEV_IDLE:
        {
            src_enter_connecting_state(index);
        }
        break;

    case STATE_DEV_RECONNECTING:
        {
            sink_dev->pending_event = KEY_LONG;
            src_handle_dev_cleanup(index);
        }
        break;

    case STATE_DEV_CONNECTING:
        {
            APP_PRINT_WARN0("non_active_dev_handle_long_press_event: already connecting");
        }
        break;

    case STATE_DEV_CONNECTED:
        {
            APP_PRINT_WARN0("non_active_dev_handle_long_press_event: disconnecting");
            sink_dev->pending_event = KEY_LONG;
            src_handle_dev_cleanup(index);
        }
        break;

    case STATE_DEV_CLEANUP:
        {
            APP_PRINT_INFO1("non_active_dev_handle_long_press_event: current pending event %d",
                            sink_dev->pending_event);

            sink_dev->pending_event = KEY_LONG;
        }
        break;

    default:
        break;
    }
}


void key_long_press_handle(uint8_t index)
{
    if (shutdown)
    {
        APP_PRINT_WARN1("key_long_press_handle: index %d not handled in shutdown", index);
        return;
    }

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_INFO1("key_short_press_handle: invalid %d", index);
        return;
    }

    if (!src_is_busy() || src_is_cur_dev(index))
    {
        active_dev_handle_long_press_event(index);
    }
    else
    {
        non_active_dev_handle_long_press_event(index);
    }
}

void app_src_handle_usb_audio_in(void)
{
    uint8_t i;

    //app_usb_audio_music_create();
    //app_usb_audio_msbc_ds_create();
    //app_usb_audio_msbc_us_create();

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].stream_info.stream_state == A2DP_STREAM_STATE_IDLE)
        {
            //bt_a2dp_stream_start_req(sink_dev_list[i].remote_info.bd_addr);
        }
    }
}

void app_src_handle_usb_audio_out(void)
{
    uint8_t i;

    //app_usb_audio_music_destroy();
    //app_usb_audio_msbc_ds_destroy();
    //app_usb_audio_msbc_us_destroy();

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].stream_info.stream_state == A2DP_STREAM_STATE_STARTED)
        {
            //bt_a2dp_stream_suspend_req(sink_dev_list[i].remote_info.bd_addr);
        }
    }
}

static void app_handle_us_audio_status(uint8_t index, bool active)
{
    T_APP_DEV *sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO2("app_handle_us_audio_status: index %d, active %d",
                    index, active);

    if (active)
    {
        if (!timer_idx_sco_conn_delay)
        {
            app_start_timer(&timer_idx_sco_conn_delay,
                            "sco_conn_delay",
                            src_policy_timer_id,
                            SRC_TIMER_ID_SCO_CONN_SETUP_DELAY,
                            0,
                            false,
                            SRC_SCO_CONN_SETUP_DELAY_TIMEOUT);
        }
    }
    else
    {
        if (sink_dev->sco_state == SCO_SETUP_STATE_CONNECTED)
        {
            bt_hfp_ag_audio_disconnect_req(sink_dev->remote_info.bd_addr);
            sink_dev->sco_state = SCO_SETUP_STATE_DISCONNECTING;
            if (non_teams_is_inCall)
            {
                bt_hfp_ag_call_indicator_send(sink_dev->remote_info.bd_addr, BT_HFP_AG_NO_CALL_IN_PROGRESS);
                non_teams_is_inCall = false;
            }
        }

        app_usb_audio_msbc_ds_destroy();
        app_usb_audio_msbc_us_destroy();

        //wait for stream data to start
        /*
        if (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_IDLE)
        {
            if (bt_a2dp_stream_start_req(sink_dev->remote_info.bd_addr))
            {
                sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STARTING;
            }
        }
        */

        //app_usb_audio_music_create(sink_dev->codec_info);
    }
}

void app_handle_music_data_notify(void)
{
    T_APP_DEV *sink_dev = &sink_dev_list[0];
    T_APP_BR_LINK *p_link = app_find_br_link(sink_dev->remote_info.bd_addr);

    if (p_link != NULL)
    {
#ifdef BTDONGLE_BQB_MODE_ENABLE
        if (app_is_bqb_mode())
        {
            if ((sink_dev->dev_state == STATE_DEV_CONNECTED) &&
                (p_link->connected_profile & A2DP_PROFILE_MASK) &&
                (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_IDLE) &&
                (app_a2dp_opened == true) && avrcp_is_connected)
            {
                if ((sink_dev->sco_state == SCO_SETUP_STATE_IDLE) && (!sink_dev->hfp_codec_selected) &&
                    bt_a2dp_stream_start_req(sink_dev->remote_info.bd_addr))
                {
                    app_stop_timer(&timer_idx_usb_stream_restart);
                    sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STARTING;
                }
            }
        }
        else
#endif
        {
            APP_PRINT_INFO1("app_handle_music_data_notify, a2dp_channel_ready %d", a2dp_channel_ready);
            if (a2dp_channel_ready && (sink_dev->dev_state == STATE_DEV_CONNECTED) &&
                (p_link->connected_profile & A2DP_PROFILE_MASK) &&
                (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_IDLE) && avrcp_is_connected)
            {
                if ((sink_dev->sco_state == SCO_SETUP_STATE_IDLE) && (!sink_dev->hfp_codec_selected) &&
                    bt_a2dp_stream_start_req(sink_dev->remote_info.bd_addr))
                {
                    APP_PRINT_INFO1("app_handle_music_data_notify addr %b",
                                    TRACE_BDADDR(sink_dev->remote_info.bd_addr));
                    app_stop_timer(&timer_idx_usb_stream_restart);
                    sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STARTING;
                    bt_sniff_mode_disable(sink_dev->remote_info.bd_addr);
                }
            }
        }

    }
}

void app_notify_policy_us_audio_status(bool active)
{
    uint8_t i;
    T_APP_BR_LINK *p_link = NULL;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].dev_state == STATE_DEV_CONNECTED)
        {
            p_link = app_find_br_link(sink_dev_list[i].remote_info.bd_addr);

            if ((p_link != NULL) && (p_link->connected_profile & HFP_PROFILE_MASK))
            {
                app_handle_us_audio_status(i, active);
            }
        }
    }
}

void src_handle_poweron(void)
{
    shutdown = false;

    //dm_power_off_disable(0xff);

    //force to set IDLE bt mode
    bt_device_mode_set(BT_DEVICE_MODE_IDLE);

    bool ret = false;

    ret = app_start_timer(&timer_idx_src_poweron_delay, "src_poweron_delay",
                          src_policy_timer_id, SRC_TIMER_ID_POWERON_DELAY, 0, false,
                          SRC_POWERON_DELAY_TIMEOUT);
    APP_PRINT_INFO2("src_handle_poweron ret %d delay handle %x", ret, timer_idx_src_poweron_delay);

}

void src_poweroff_cleanup(void)
{
    uint8_t i;

    src_set_bt_mode(BT_DEVICE_MODE_IDLE);
    shutdown = true;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev_list[i].pending_event = KEY_NON;
        src_handle_dev_cleanup(i);
    }

    //see if need to reset to switch a2dp role
    src_check_product_role_switch();
}

void src_stop_poweron_reconn(void)
{
    T_APP_DEV *sink_dev = &sink_dev_list[0];

    if ((!report_pair_state) && (sink_dev->dev_state == STATE_DEV_RECONNECTING))
    {
        //if paging stop it
        if (sink_dev->bt_state == STATE_BT_PAGING)
        {
            gap_br_stop_sdp_discov(sink_dev->remote_info.bd_addr);
        }

        //for acl connected state, it will not block inquiry
        //but this should be the last time retry reconn
        sink_dev->retry_count = 1;
    }
}

void src_usb_reset_reconn(void)
{
    T_APP_DEV *sink_dev = &sink_dev_list[0];

    APP_PRINT_INFO3("src_usb_reset_reconn: %d, %d, %d",
                    src_dev_ready,
                    sink_dev->in_use,
                    sink_dev->dev_state);

    if (src_dev_ready && sink_dev->in_use &&
        (sink_dev->dev_state == STATE_DEV_IDLE))
    {
        src_is_reconnecting = true;

        key_short_press_handle(0);
    }
}

/* MS APIs begin*/
uint8_t src_get_audio_device_pair_state(void)
{
    T_APP_DEV *sink_dev = &sink_dev_list[0];

    if (sink_dev->in_use)
    {
        if (sink_dev->dev_state == STATE_DEV_CONNECTED)
        {
            return PAIR_STATE_SUCCESS;
        }
        else if ((sink_dev->dev_state != STATE_DEV_IDLE) ||
                 (sink_dev->bt_state == STATE_BT_ACL_CONNECTED))
        {
            return PAIR_STATE_IN_PROCESS;
        }
        else
        {
            return PAIR_STATE_IDLE;
        }
    }
    else
    {
        return PAIR_STATE_IDLE;
    }
}

bool src_get_audio_connected_dev_addr(uint8_t *bd_addr)
{
    T_APP_DEV *sink_dev;

    if (bd_addr == NULL)
    {
        return false;
    }

    sink_dev = &sink_dev_list[0];

    if (sink_dev->in_use && (sink_dev->bt_state == STATE_BT_ACL_CONNECTED))
    {
        memcpy(bd_addr, sink_dev->remote_info.bd_addr, 6);
    }

    return true;
}

bool src_connect_audio_device(uint8_t *bd_addr, uint32_t cod)
{
    T_APP_DEV *sink_dev = NULL;

    if (bd_addr == NULL)
    {
        APP_PRINT_ERROR0("src_connect_audio_device: invalid BD address");
        return false;
    }

    sink_dev = &sink_dev_list[0];

    if (sink_dev->in_use &&
        (sink_dev->dev_state != STATE_DEV_IDLE) &&
        (sink_dev->dev_state != STATE_DEV_CONNECTED))
    {
        APP_PRINT_WARN1("src_connect_audio_device: bad state %d", sink_dev->dev_state);
        return false;
    }

    report_pair_state = true;

    src_is_reconnecting = false;

    memcpy(sink_dev->pending_info.remote_bd, bd_addr, 6);
    sink_dev->pending_info.cod = cod;
    sink_dev->pending_event = KEY_NEW_TARGET;
    sink_dev->in_use = true;
    src_handle_dev_cleanup(0);

    return true;
}

bool src_remove_audio_device(void)
{
    T_APP_DEV *sink_dev = &sink_dev_list[0];

    //no target device
    if (sink_dev->in_use)
    {
        src_handle_dev_cleanup(0);
        //general_bt_disconnect_by_bdaddr(sink_dev->remote_info.bd_addr);
        //sink_dev->pending_event = KEY_NON;
        //src_dev_cleanup(0);
#if F_BT_LEGACY_APP_KEY_MANAGER
        app_src_legacy_delete_bond_by_index(0);
#else
        bt_bond_delete(sink_dev->remote_info.bd_addr);
#endif
        src_is_reconnecting = false;
    }

    return true;
}

/*void src_register_audio_pair_state_nofity(APP_SRC_AUDIO_CONN_RESULT_CB pair_state_cb)
{
    src_pair_state_cb = pair_state_cb;
}*/

void src_register_pair_state_nofity(APP_SRC_PAIR_RESULT_CB pair_state_cb)
{
    src_pair_state = pair_state_cb;
}

uint8_t src_get_audio_device_conn_state(void)
{
    T_APP_DEV *sink_dev = &sink_dev_list[0];

    //no target device
    if (sink_dev->in_use && (sink_dev->dev_state == STATE_DEV_CONNECTED))
    {
        return BT_CONN_STATE_CONNECTED;
    }

    return BT_CONN_STATE_DISCONNECTED;
}

uint8_t src_get_audio_device_profile_state(void)
{
    T_APP_DEV *sink_dev = &sink_dev_list[0];
    T_APP_BR_LINK *p_link = NULL;
    uint8_t profile_state = 0;

    if (sink_dev->in_use)
    {
        p_link = app_find_br_link(sink_dev_list[0].remote_info.bd_addr);

        if (p_link != NULL)
        {
            if (p_link->connected_profile & HFP_PROFILE_MASK)
            {
                profile_state |= 0x01;
            }

            if (p_link->connected_profile & A2DP_PROFILE_MASK)
            {
                profile_state |= 0x02;
            }

            if (p_link->connected_profile & AVRCP_PROFILE_MASK)
            {
                profile_state |= 0x04;
            }
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
            if (p_link->connected_profile & SPP_VENDER_PASSTHROUGH_PROFILE_MASK)
            {
                profile_state |= 0x20;
            }
#endif
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
            if (p_link->connected_profile & ASP_PROFILE_MASK)
            {
                profile_state |= 0x10;
            }
#endif
        }
    }

    return profile_state;
}


bool src_connect(void)
{
    T_APP_DEV *sink_dev = &sink_dev_list[0];

    if (sink_dev->in_use)
    {
        if (sink_dev->dev_state == STATE_DEV_IDLE)
        {
            key_short_press_handle(0);
            return true;
        }
    }

    return false;
}

bool src_disconnect(void)
{
    T_APP_DEV *sink_dev = &sink_dev_list[0];

    if (sink_dev->in_use)
    {
        if (sink_dev->dev_state == STATE_DEV_CONNECTED)
        {
            gap_br_send_acl_disconn_req(sink_dev->remote_info.bd_addr);
            return true;
        }
    }

    return false;
}

uint8_t src_get_disconn_reason(void)
{
    return disconn_reason;
}

/* MS APIs end*/

#if MS_RF_SCHEDULER_ENABLE
static bool rf_scheduler_enable(bool enable)
{
    uint8_t params[2];

    params[0] = 0x17;
    params[1] = enable ? 1 : 0;

    return gap_vendor_cmd_req(0xfd80, 2, params);
}
#endif

static void src_handle_poweron_delay_timeout(void)
{
    T_APP_SRC_TARGET_DEV temp_dev;
#if 0
    uint8_t i;

#ifdef CUSTOMER_FIXED_SETTINGS
    poweron_autopair = true;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        src_led_set_idle(i);
        key_long_press_handle(i);
    }
#else
    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        src_led_set_idle(i);
        key_short_press_handle(i);
    }
#endif
#else

#if MS_RF_SCHEDULER_ENABLE
    if (app_vendor_cfg_is_audio_hid())
    {
        rf_scheduler_enable(true);
    }
#endif

    src_dev_ready = true;

    //src_led_set_idle(0);

    src_check_set_bt_mode();
//    T_APP_DONGLE_NAME target_name;
//    if ((app_storage_get_dongle_usb_name(&target_name) == 0) && (target_name.is_valid == 1))
//    {
//        gap_br_set_dev_name(target_name.name, target_name.length);
//    }
    APP_PRINT_INFO0("src_handle_poweron_delay_timeout");
    if (app_src_storage_get_target_dev(TARGET_DEVICE_TYPE_HEADSET, &temp_dev))
    {
        sink_dev_list[0].in_use = true;
        memcpy(sink_dev_list[0].remote_info.bd_addr, temp_dev.remote_bd, 6);
        sink_dev_list[0].remote_info.cod = temp_dev.ms_dev_type;

        //only discoverable during reconn when poweron
        src_is_reconnecting = true;

        key_short_press_handle(0);
        //src_check_set_bt_mode();
    }
#ifndef F_BT_LE_HIDS_CLIENT_BQB_SUPPORT
    if (app_vendor_cfg_is_audio_hid())
    {
//        le_conn_target_devices();
    }
#endif
#endif
}

static void src_handle_acl_exist_timeout(uint8_t index)
{
    T_APP_BR_LINK *p_link;
    T_APP_DEV *sink_dev = NULL;
    uint32_t target_profile_mask = A2DP_PROFILE_MASK | HFP_PROFILE_MASK |
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
                                   ASP_PROFILE_MASK |
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
                                   SPP_VENDER_PASSTHROUGH_PROFILE_MASK |
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
                                   SPP_AUDIO_PROFILE_MASK |
#endif
#if 0
                                   MANUFACTURER_DATA_MASK |
#endif
                                   DEVICE_INFO_MASK;
#ifdef BTDONGLE_BQB_MODE_ENABLE
    if (app_is_bqb_mode())
    {
        target_profile_mask = A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK | HFP_PROFILE_MASK;
    }
#endif
    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_handle_acl_exist_timeout: invalid index %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    APP_PRINT_INFO2("src_handle_acl_exist_timeout: index %d, bt state %d",
                    index, sink_dev->dev_state);

    p_link = app_find_br_link(sink_dev->remote_info.bd_addr);
    if (sink_dev->dev_state != STATE_DEV_IDLE)
    {
        if (p_link != NULL)
        {
#if 0
            if (p_link->connected_profile & A2DP_PROFILE_MASK)
            {
                src_bt_state_set(index, STATE_BT_ACL_CONNECTED);
                src_handle_action_result(index, true);
            }
            else
            {
                gap_br_send_acl_disconn_req(sink_dev->remote_info.bd_addr);
                //src_dev_profile_conn(index);
            }
#endif
            if (p_link->connected_profile != target_profile_mask)
            {
                sink_dev->profile_mask_plan = target_profile_mask;
                sink_dev->profile_mask_remain = p_link->connected_profile ^ target_profile_mask;

                //signal channel connected, no discover cmd received
                //start avdtp discover by open api
                /*
                if (!sink_dev->a2dp_codec_configured &&
                    (p_link->connected_profile & A2DP_PROFILE_MASK))
                {
                    bt_a2dp_stream_open_req(sink_dev->remote_info.bd_addr, BT_A2DP_ROLE_SNK);
                }
                */

                src_dev_profile_conn(index);
            }
            else
            {
                src_handle_action_result(index, true);
            }
        }
        else
        {
            if (src_is_cur_dev(index))
            {
                src_handle_action_result(index, false);
            }
        }
    }
    else
    {
        if (p_link != NULL)
        {
            APP_PRINT_INFO2("src_handle_acl_exist_timeout, target %08x, connected %08x",
                            target_profile_mask, p_link->connected_profile);

            if (p_link->connected_profile != target_profile_mask)
            {
                sink_dev->profile_mask_plan = target_profile_mask;
                sink_dev->profile_mask_remain = p_link->connected_profile ^ target_profile_mask;
                sink_dev->profiles_retry_count = SRC_PROFILES_RETRY_COUNT;

                //signal channel connected, no discover cmd received
                //start avdtp discover by open api
                /*
                if (!sink_dev->a2dp_codec_configured &&
                    (p_link->connected_profile & A2DP_PROFILE_MASK))
                {
                    bt_a2dp_stream_open_req(sink_dev->remote_info.bd_addr, BT_A2DP_ROLE_SNK);
                }
                */

                src_dev_state_set(index, STATE_DEV_RECONNECTING);
                sink_dev->retry_count = 1;

                src_set_active_sink_dev(index);

                src_dev_profile_conn(index);
            }
            else
            {
                src_handle_action_result(index, true);
            }
        }
        else
        {
            if (src_is_cur_dev(index))
            {
                src_handle_action_result(index, false);
            }
        }
    }
}

static void src_handle_stream_idle_timeout(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_handle_stream_idle_timeout: invalid index %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];
    T_APP_BR_LINK *p_link = app_find_br_link(sink_dev->remote_info.bd_addr);

    APP_PRINT_INFO2("src_handle_stream_idle_timeout: index %d, dev state %d",
                    index, sink_dev->dev_state);

    //if ((sink_dev->dev_state == STATE_DEV_CONNECTED) &&
    if ((p_link->connected_profile & A2DP_PROFILE_MASK) &&
        (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_STARTED))
    {
        if (sink_dev->stream_frame_count > 0)
        {
            sink_dev->stream_idle_count = 0;
        }
        else
        {
            sink_dev->stream_idle_count++;
        }

        sink_dev->stream_frame_count = 0;

        if (sink_dev->stream_idle_count >= SRC_STREAMING_IDLE_DETECT_MAX_COUNT)
        {
            if (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_STARTED)
            {
                APP_PRINT_INFO1("src_handle_stream_idle_timeout send a2dp suspend, usb ds state is %d",
                                app_get_usb_ds_state());
                if (app_get_usb_ds_state())
                {
                    sink_dev->stream_idle_count = 0;
                }
                else
                {
                    silent_detect_finished = false;
                    app_stop_timer(&timer_idx_usb_stream_restart);
                    if (bt_a2dp_stream_suspend_req(sink_dev->remote_info.bd_addr))
                    {
                        sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STOPPING;
                    }
                }
            }
        }
        else
        {
            app_start_timer(&sink_dev->timer_idx_stream_idle, "stream_idle",
                            src_policy_timer_id, SRC_TIMER_ID_STREAM_IDLE_SINK0 + index, 0, false,
                            SRC_STREAMING_IDLE_DETECT_TIMEOUT);
        }
    }
}

static void src_hanlde_pair_suc_report_delay_timeout(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;

    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_hanlde_pair_suc_report_delay_timeout: invalid index %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];

    if ((src_pair_state != NULL) && report_pair_state)
    {
        src_pair_state(sink_dev->remote_info.bd_addr, PAIR_STATE_SUCCESS);
        report_pair_state = false;
    }
}

static void src_handle_a2dp_conn_setup_delay_timeout(uint8_t index)
{
    T_APP_DEV *sink_dev = NULL;
    T_APP_BR_LINK *p_link;
    uint32_t target_profile_mask = A2DP_PROFILE_MASK |
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
                                   ASP_PROFILE_MASK |
#endif
#ifdef SPP_VENDER_PASSTHROUGH_FEATURE_SUPPORT
                                   SPP_VENDER_PASSTHROUGH_PROFILE_MASK |
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT ==1)
                                   SPP_AUDIO_PROFILE_MASK |
#endif
                                   HFP_PROFILE_MASK;
    if (index >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_WARN1("src_handle_a2dp_conn_setup_delay_timeout: invalid index %d", index);
        return;
    }

    sink_dev = &sink_dev_list[index];
    p_link = app_find_br_link(sink_dev->remote_info.bd_addr);

    if (p_link != NULL)
    {
        APP_PRINT_INFO2("src_handle_a2dp_conn_setup_delay_timeout: %d, %08x",
                        sink_dev->dev_state, p_link->connected_profile);

        if (sink_dev->dev_state != STATE_DEV_IDLE)
        {
            if (p_link->connected_profile != target_profile_mask)
            {
                sink_dev->profile_mask_plan = target_profile_mask;
                sink_dev->profile_mask_remain = p_link->connected_profile ^ target_profile_mask;

                src_dev_profile_conn(index);
            }
            else
            {
                src_handle_action_result(index, true);
            }
        }
        else
        {
            if (p_link->connected_profile != target_profile_mask)
            {
                sink_dev->profile_mask_plan = target_profile_mask;
                sink_dev->profile_mask_remain = p_link->connected_profile ^ target_profile_mask;
                sink_dev->profiles_retry_count = SRC_PROFILES_RETRY_COUNT;

                src_dev_state_set(index, STATE_DEV_RECONNECTING);
                sink_dev->retry_count = 1;

                src_set_active_sink_dev(index);

                src_dev_profile_conn(index);
            }
            else
            {
                src_handle_action_result(index, true);
            }
        }
    }
}

void src_detect_silent_data_start_timer(void)
{
    if ((!timer_idx_silent_detect) && (!silent_detect_finished))
    {
        APP_PRINT_INFO0("detected silent data!");
        app_start_timer(&timer_idx_silent_detect,
                        "silent_detect_timer",
                        src_policy_timer_id,
                        SRC_TIMER_ID_SILENT_DETECT,
                        0,
                        false,
                        SRC_SILENT_DETECT_TIMEOUT);
    }
}

void src_detect_silent_data_stop_timer(void)
{
    if (timer_idx_silent_detect)
    {
        app_stop_timer(&timer_idx_silent_detect);
        timer_idx_silent_detect = NULL;
    }
    else
    {
        if (silent_detect_finished)
        {
            silent_detect_finished = false;
        }
        T_APP_DEV *sink_dev = &sink_dev_list[0];
        T_APP_BR_LINK *p_link = app_find_br_link(sink_dev->remote_info.bd_addr);

        APP_PRINT_INFO1("src_detect_silent_data_stop_timer, stream state: %d",
                        sink_dev->stream_info.stream_state);
        if (a2dp_channel_ready && (sink_dev->dev_state == STATE_DEV_CONNECTED) &&
            (p_link->connected_profile & A2DP_PROFILE_MASK) && avrcp_is_connected &&
            (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_IDLE))
        {
            if ((sink_dev->sco_state == SCO_SETUP_STATE_IDLE) && (!sink_dev->hfp_codec_selected))
            {
                bt_a2dp_stream_start_req(sink_dev->remote_info.bd_addr);
                app_stop_timer(&timer_idx_usb_stream_restart);
                sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STARTING;
                bt_sniff_mode_disable(sink_dev->remote_info.bd_addr);
            }
        }
    }
}

static void src_policy_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO2("src_policy_timeout_cb: timer_evt %d, param %d",
                    timer_evt, param);

    switch (timer_evt)
    {
    case SRC_TIMER_ID_POWERON_DELAY:
        {
            app_stop_timer(&timer_idx_src_poweron_delay);
            src_handle_poweron_delay_timeout();
        }
        break;

    case SRC_TIMER_ID_PAIRING_MODE:
        {
            ota_mode = false;
            app_stop_timer(&timer_idx_src_pairing_mode);

            //app_led_set_mode(APP_LED_MODE_POWER_ON);

            src_check_set_bt_mode();
        }
        break;

    case SRC_TIMER_ID_ACL_EXIST_SINK0:
        {
            app_stop_timer(&sink_dev_list[0].timer_idx_acl_exist_delay);
            src_handle_acl_exist_timeout(0);
        }
        break;

    case SRC_TIMER_ID_STREAM_IDLE_SINK0:
        {
            app_stop_timer(&sink_dev_list[0].timer_idx_stream_idle);
            src_handle_stream_idle_timeout(0);
        }
        break;

    case SRC_TIMER_ID_PAIR_SUC_REPORT_DELAY_SINK0:
        {
            app_stop_timer(&sink_dev_list[0].timer_idx_pair_suc_report_delay);
            src_hanlde_pair_suc_report_delay_timeout(0);
        }
        break;

    case SRC_TIMER_ID_A2DP_CONN_SETUP_DELAY_SINK0:
        {
            app_stop_timer(&sink_dev_list[0].timer_idx_a2dp_conn_setup_delay);
            src_handle_a2dp_conn_setup_delay_timeout(0);
        }
        break;

#if SINK_DEV_NUM_MAX > 1
    case SRC_TIMER_ID_ACL_EXIST_SINK1:
        {
            app_stop_timer(&sink_dev_list[1].timer_idx_acl_exist_delay);
            src_handle_acl_exist_timeout(1);
        }
        break;

    case SRC_TIMER_ID_STREAM_IDLE_SINK1:
        {
            app_stop_timer(&sink_dev_list[1].timer_idx_stream_idle);
            src_handle_stream_idle_timeout(1);
        }
        break;

    case SRC_TIMER_ID_PAIR_SUC_REPORT_DELAY_SINK1:
        {
            app_stop_timer(&sink_dev_list[1].timer_idx_pair_suc_report_delay);
            src_hanlde_pair_suc_report_delay_timeout(1);
        }
        break;

    case SRC_TIMER_ID_A2DP_CONN_SETUP_DELAY_SINK1:
        {
            app_stop_timer(&sink_dev_list[1].timer_idx_a2dp_conn_setup_delay);
            src_handle_a2dp_conn_setup_delay_timeout(1);
        }
        break;
#endif
#ifdef BTDONGLE_BQB_MODE_ENABLE
    case SRC_TIMER_ID_BQB_USB_SET_REPORT:
        app_bqb_ring_handle_msg();
        break;
#endif
    case SRC_TIMER_ID_SILENT_DETECT:
        {
            app_stop_timer(&timer_idx_silent_detect);
            timer_idx_silent_detect = NULL;
            silent_detect_finished = true;
            APP_PRINT_INFO1("silent detect finished, stream state: %d",
                            sink_dev_list[0].stream_info.stream_state);
            if (sink_dev_list[0].stream_info.stream_state == A2DP_STREAM_STATE_STARTED)
            {
                if (bt_a2dp_stream_suspend_req(sink_dev_list[0].remote_info.bd_addr))
                {
                    sink_dev_list[0].stream_info.stream_state = A2DP_STREAM_STATE_STOPPING;
                }
            }
        }
        break;
    case SRC_TIMER_ID_SCO_CONN_SETUP_DELAY:
        {
            app_stop_timer(&timer_idx_sco_conn_delay);
            timer_idx_sco_conn_delay = NULL;
            if (app_usb_audio_is_us_streaming())
            {
                T_APP_DEV *sink_dev = &sink_dev_list[0];

                if (sink_dev->stream_info.stream_state == A2DP_STREAM_STATE_STARTED)
                {
                    if (bt_a2dp_stream_suspend_req(sink_dev->remote_info.bd_addr))
                    {
                        sink_dev->stream_info.stream_state = A2DP_STREAM_STATE_STOPPING;
                    }
                }

                app_usb_audio_music_destroy();

                if (sink_dev->sco_state == SCO_SETUP_STATE_IDLE)
                {
                    sink_dev->sco_state = SCO_SETUP_STATE_CONNECTING;
                    bt_hfp_ag_audio_connect_req(sink_dev->remote_info.bd_addr);
                    if ((!app_hfp_isInCall) && (curr_call_status != BT_HFP_AG_CURRENT_CALL_STATUS_HELD))
                    {
                        bt_hfp_ag_call_indicator_send(sink_dev->remote_info.bd_addr, BT_HFP_AG_CALL_IN_PROGRESS);
                        non_teams_is_inCall = true;
                        curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_ACTIVE;
                    }
                }
                app_usb_audio_msbc_ds_create();
                app_usb_audio_msbc_us_create();
            }
        }
        break;
    case SRC_TIMER_ID_USB_STREAM_RESTART:
        {
            app_stop_timer(&timer_idx_usb_stream_restart);
            T_APP_DEV *sink_dev = &sink_dev_list[0];
            bt_avrcp_play_status_change_req(sink_dev->remote_info.bd_addr, BT_AVRCP_PLAY_STATUS_PLAYING);
        }
        break;
    default:
        break;
    }
}

static void src_handle_avrcp_play_status_changed_reg_req(uint8_t *bd_addr)
{
    uint8_t index = 0;
    T_BT_AVRCP_PLAY_STATUS play_status;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        if (sink_dev_list[index].stream_info.stream_state == A2DP_STREAM_STATE_STARTED)
        {
            play_status = BT_AVRCP_PLAY_STATUS_PLAYING;
        }
        else
        {
            play_status = BT_AVRCP_PLAY_STATUS_PAUSED;
        }
        APP_PRINT_INFO2("avrcp_play_status_changed_reg_req %b status %d", TRACE_BDADDR(bd_addr),
                        play_status);
        bt_avrcp_play_status_change_register_rsp(bd_addr, play_status);
    }
}

static void src_handle_avrcp_track_changed_reg_req(uint8_t *bd_addr)
{
    uint8_t index = 0;
    uint64_t track_id = 1;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        T_APP_DEV *sink_dev = &sink_dev_list[index];
        uint32_t connected_profiles;
        if (get_connected_profile(sink_dev->remote_info.bd_addr, &connected_profiles))
        {
            if (connected_profiles & AVRCP_PROFILE_MASK)
            {
                APP_PRINT_INFO2("avrcp_track_changed_reg_req %b track_id %d",
                                TRACE_BDADDR(bd_addr), track_id);
                bt_avrcp_track_change_register_rsp(bd_addr, track_id);
            }
        }
    }
}

static void src_handle_avrcp_addressed_player_changed_reg_req(uint8_t *bd_addr)
{
    uint8_t index = 0;
    uint16_t player_id = 1;
    uint16_t uid_counter = 0;

    if (src_policy_find_dev_index_by_addr(bd_addr, &index))
    {
        T_APP_DEV *sink_dev = &sink_dev_list[index];
        uint32_t connected_profiles;
        if (get_connected_profile(sink_dev->remote_info.bd_addr, &connected_profiles))
        {
            if (connected_profiles & AVRCP_PROFILE_MASK)
            {
                APP_PRINT_INFO3("avrcp_addressed_player_change_register_rsp %b player_id %d uid_counter %d",
                                TRACE_BDADDR(bd_addr), player_id, uid_counter);
                bt_avrcp_addressed_player_change_register_rsp(bd_addr, player_id, uid_counter);
            }
        }
    }
}

static void src_bt_mgr_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;
    T_BT_PARAM bt_param;

    if (event_type != BT_EVENT_SCO_DATA_IND)
    {
        APP_PRINT_INFO1("src_bt_mgr_cback: event_type 0x%04x", event_type);
    }

    switch (event_type)
    {

    case BT_EVENT_READY:
        {
            //check for bonded device to re-connect
            //app_mmi_handle_action(MMI_DEV_POWER_ON);
            //bt_device_mode_set(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
        }
        break;

    case BT_EVENT_INQUIRY_RESULT:
        {
            src_handle_inquiry_res(param->inquiry_result);
        }
        break;

    case BT_EVENT_INQUIRY_RSP:
        {
            src_handle_inquiry_rsp(param->inquiry_rsp.cause);
        }
        break;

    case BT_EVENT_INQUIRY_CMPL:
        {
            src_handle_inquiry_cmpl(param->inquiry_cmpl.cause);
        }
        break;

    case BT_EVENT_INQUIRY_CANCEL_RSP:
        {
            src_handle_inquiry_cancel_cmpl(param->inquiry_cancel_rsp.cause);
        }
        break;

    case BT_EVENT_ACL_CONN_IND:
        {
            src_handle_acl_conn_req(param->acl_conn_ind.bd_addr);
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            bt_param.bd_addr = param->acl_conn_disconn.bd_addr;
            bt_param.cause = param->acl_conn_disconn.cause;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_CONN_FAIL:
        {
            bt_param.bd_addr = param->acl_conn_fail.bd_addr;
            bt_param.cause = param->acl_conn_fail.cause;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_CONN_SUCCESS:
        {
            bt_param.bd_addr = param->acl_conn_success.bd_addr;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_CONN_READY:
        {
            bt_param.bd_addr = param->acl_conn_active.bd_addr;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_SUCCESS:
        {
            bt_param.bd_addr = param->acl_authen_success.bd_addr;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_AUTHEN_FAIL:
        {
            bt_param.bd_addr = param->acl_authen_fail.bd_addr;
            bt_param.cause = param->acl_authen_fail.cause;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_ROLE_MASTER:
        {
            bt_param.bd_addr = param->acl_role_slave.bd_addr;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_ACL_ROLE_SLAVE:
        {
            bt_param.bd_addr = param->acl_role_slave.bd_addr;
            src_handle_acl_link_status(&bt_param, event_type);
        }
        break;

    case BT_EVENT_REMOTE_NAME_RSP:
        {
            src_handle_acl_remote_name_rsp(param->remote_name_rsp);
        }
        break;

    case BT_EVENT_ACL_ROLE_SWITCH_FAIL:
        {
            src_handle_role_switch_fail(param->acl_role_switch_fail.bd_addr,
                                        param->acl_role_switch_fail.cause);
        }
        break;

    case BT_EVENT_LINK_KEY_INFO:
        {
            uint8_t bdaddr[6] = {0, 0, 0, 0, 0, 0};
            if (!memcmp(sink_dev_list[0].remote_info.bd_addr, bdaddr, 6))
            {
                memcpy(sink_dev_list[0].remote_info.bd_addr, param->link_key_info.bd_addr, 6);
            }
            src_handle_link_key_info(param->link_key_info.bd_addr,
                                     param->link_key_info.link_key,
                                     param->link_key_info.key_type);
        }
        break;

    case BT_EVENT_LINK_KEY_REQ:
        {
            src_handle_link_key_req(param->link_key_req.bd_addr);
        }
        break;

    case BT_EVENT_SDP_ATTR_INFO:
        {
            src_handle_sdp_attr_info(param->sdp_attr_info.bd_addr,
                                     param->sdp_attr_info.info);
        }
        break;

    case BT_EVENT_DID_ATTR_INFO:
        {
            src_handle_did_attr_info(param->did_attr_info);
        }
        break;

    case BT_EVENT_SDP_DISCOV_CMPL:
        {
            src_handle_sdp_cmpl(param->sdp_discov_cmpl.bd_addr,
                                param->sdp_discov_cmpl.cause);
        }
        break;

    case BT_EVENT_A2DP_CONN_IND:
        {
            src_handle_a2dp_conn_ind(param->a2dp_conn_ind.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_CONN_CMPL:
        {
            src_handle_profile_conn_cmpl(param->a2dp_conn_cmpl.bd_addr,
                                         A2DP_PROFILE_MASK);
        }
        break;

    case BT_EVENT_A2DP_CONFIG_CMPL:
        {
            src_handle_a2dp_config_cmpl(param->a2dp_config_cmpl);
        }
        break;

    case BT_EVENT_A2DP_STREAM_OPEN:
        {
            src_handle_a2dp_open(param->a2dp_stream_open.bd_addr,
                                 param->a2dp_stream_open.max_pkt_len);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            src_handle_a2dp_stream_start_ind(param->a2dp_stream_start_ind.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_RSP:
        {
            src_handle_a2dp_stream_start_rsp(param->a2dp_stream_start_rsp.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_STREAM_DATA_RSP:
        {
            src_handle_a2dp_stream_data_rsp(param->a2dp_stream_data_rsp.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            src_handle_a2dp_stream_stop(param->a2dp_stream_stop.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_STREAM_CLOSE:
        {
            src_handle_a2dp_stream_close(param->a2dp_stream_close.bd_addr);
        }
        break;

    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
            src_handle_profile_disconn_cmpl(param->a2dp_disconn_cmpl.bd_addr,
                                            param->a2dp_disconn_cmpl.cause,
                                            A2DP_PROFILE_MASK);
        }
        break;

    case BT_EVENT_AVRCP_CONN_CMPL:
        src_handle_profile_conn_cmpl(param->avrcp_conn_cmpl.bd_addr,
                                     AVRCP_PROFILE_MASK);
        break;

    case BT_EVENT_AVRCP_DISCONN_CMPL:
        src_handle_profile_disconn_cmpl(param->avrcp_disconn_cmpl.bd_addr,
                                        param->avrcp_disconn_cmpl.cause,
                                        AVRCP_PROFILE_MASK);
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED_REG_REQ:
        src_handle_avrcp_play_status_changed_reg_req(param->avrcp_reg_play_status_changed.bd_addr);
        break;

    case BT_EVENT_AVRCP_TRACK_CHANGED_REG_REQ:
        src_handle_avrcp_track_changed_reg_req(param->avrcp_track_changed.bd_addr);
        break;

    case BT_EVENT_AVRCP_ADDRESSED_PLAYER_CHANGED_REG_REQ:
        src_handle_avrcp_addressed_player_changed_reg_req(param->avrcp_addressed_player_changed.bd_addr);
        break;

    case BT_EVENT_SPP_CONN_CMPL:
        {
            src_handle_profile_conn_cmpl(param->spp_conn_cmpl.bd_addr,
                                         SPP_PROFILE_MASK);
        }
        break;

    case BT_EVENT_SPP_DISCONN_CMPL:
        {
            src_handle_profile_disconn_cmpl(param->spp_disconn_cmpl.bd_addr,
                                            param->spp_disconn_cmpl.cause,
                                            SPP_PROFILE_MASK);
        }
        break;

    case BT_EVENT_HFP_AG_CONN_CMPL:
        {
            src_handle_profile_conn_cmpl(param->hfp_ag_conn_cmpl.bd_addr,
                                         HFP_PROFILE_MASK);
            /*
            if (app_usb_audio_is_us_streaming())
            {
                app_notify_policy_us_audio_status(true);
            }
            */
        }
        break;

    case BT_EVENT_HFP_AG_DISCONN_CMPL:
        {
            src_handle_profile_disconn_cmpl(param->hfp_ag_disconn_cmpl.bd_addr,
                                            param->hfp_ag_disconn_cmpl.cause,
                                            HFP_PROFILE_MASK);
        }
        break;

    case BT_EVENT_HFP_AG_CODEC_TYPE_SELECTED:
        {
            src_handle_hfp_codec_selected(param->hfp_ag_codec_type_selected);
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            app_src_handle_sco_connected(param->sco_conn_cmpl);

#ifdef AUDIO_DELAY_TEST_SCO_US
            first_sco_us_enc_pkt = true;
#endif
#ifdef AUDIO_DELAY_TEST_SCO_DS
            first_sco_ds_enc_pkt = true;
#endif
        }
        break;
#ifdef  BTDONGLE_BQB_MODE_ENABLE
    case BT_EVENT_SCO_CONN_IND:
        {
            if (app_is_bqb_mode())
            {
                app_src_handle_sco_conn_ind(param->bt_sco_conn_ind);
            }
        }
        break;
#endif
    case BT_EVENT_SCO_DISCONNECTED:
        {
            app_src_handle_sco_disconnected(param->sco_disconnected);
        }
        break;

    case BT_EVENT_SCO_DATA_IND:
        {
#ifdef AUDIO_DELAY_TEST_SCO_US
            if (first_sco_us_enc_pkt)
            {
                Pad_Config(P1_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
                Pad_Config(P1_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
                first_sco_us_enc_pkt = false;
            }
#endif
            src_handle_sco_data_ind(param->sco_data_ind);
        }
        break;

    case BT_EVENT_AVRCP_STOP:
    case BT_EVENT_AVRCP_PAUSE:
        {
            APP_PRINT_INFO2("app_avrcp_bt_cback: opcode %x %s",
                            event_type, TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_start_timer(&timer_idx_usb_stream_restart, "usb_stream_restart",
                            src_policy_timer_id, SRC_TIMER_ID_USB_STREAM_RESTART, 0, false,
                            SRC_USB_STREAM_RESTART_TIMEOUT);
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if ((handle == true) && (event_type != BT_EVENT_SCO_DATA_IND))
    {
        //APP_PRINT_INFO1("src_bt_mgr_cback: event_type 0x%04x", event_type);
    }
}

void app_src_policy_init(void)
{
    src_dev_ready = false;

    memset(sink_dev_list, 0, sizeof(sink_dev_list));
    memset(&cur_active_dev_info, 0, sizeof(cur_active_dev_info));
    bt_mgr_cback_register(src_bt_mgr_cback);

    app_timer_reg_cb(src_policy_timeout_cb, &src_policy_timer_id);

    APP_PRINT_INFO1("app_src_policy_init timer_id %d", src_policy_timer_id);
}


static bool src_bt_is_busy(void)
{
    int i;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
#if 0
        if (sink_dev_list[i].connecting)
        {
            return true;
        }
#endif
        if (sink_dev_list[i].bt_state == STATE_BT_INQUIRY)
        {
            return true;
        }
        if (sink_dev_list[i].bt_state == STATE_BT_PAGING)
        {
            return true;
        }
    }

    /* No inquiry or paging or connecting */
    return false;
}

int general_bt_init(void)
{
    app_src_policy_init();

    return 0;
}

//void gaming_set_cfg(uint8_t type, uint8_t *value, uint8_t len);

int general_bt_enable(void)
{
    shutdown = false;

#if 0
    src_auto_link_cfg.single_link = 0;
    src_auto_link_cfg.autopair = 0;
    src_auto_link_cfg.autopair_timeout = 0;
    src_auto_link_cfg.autopair_general = 1;
    src_auto_link_cfg.autopair_rssi = -60;
    src_general_cfg.sbc_frames_per_pkt = 0;
    src_general_cfg.pairing_timeout = 0;
    src_general_cfg.linkback_timeout = 0;
    src_general_cfg.auto_power_off_timeout = 0;
#endif
    /* TODO: Handle autopair and single link configs. */

    return 0;
}

void general_bt_disable(void)
{
    src_poweroff_cleanup();
}

int general_bt_connect(uint8_t id, uint8_t *bdaddr)
{
    T_APP_DEV *dev;
    uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

    if (shutdown)
    {
        APP_PRINT_ERROR2("general_bt_connect: host is down, dev %u (%b)", id,
                         TRACE_BDADDR(bdaddr));
        return -1;
    }

    if (id >= SINK_DEV_NUM_MAX)
    {
        return -2;
    }

    if (!bdaddr)
    {
        return -3;
    }

    dev = &sink_dev_list[id];

    APP_PRINT_INFO4("general_bt_connect: dev %u (%b), dev_state %04x, bt_state %04x",
                    id, TRACE_BDADDR(bdaddr), dev->dev_state, dev->bt_state);

    switch (dev->dev_state)
    {
    case STATE_DEV_IDLE:
        if (!memcmp(bdaddr, ba_any, 6))
        {
            /* Connect to any found device. */
            src_enter_connecting_state(id);
            if (!src_bt_is_busy())
            {
                src_start_conn(id);
            }
            return 0;
        }

        if (src_dev_is_bonded(id))
        {
            uint8_t ba[6] = { 0, 0, 0, 0, 0, 0 };

            /* The result is definitely true because of the protection from the
             * above condition.
             * */
            app_src_legacy_get_bond_addr_by_index(id, ba);

            if (!memcmp(bdaddr, ba, 6))
            {
                src_enter_reconnecting_state(id);
                /* Maybe remote is connecting to us */
                if (!src_bt_is_busy())
                {
                    src_start_reconn(id);
                }
                return 0;
            }

            if (!app_src_legacy_delete_bond_by_index(id))
            {
                APP_PRINT_ERROR1("general_bt_connect: rm bond for dev %u err", id);
            }
        }

        src_enter_connecting_state(id);
        memcpy(dev->remote_info.bd_addr, bdaddr, 6);
        if (!src_bt_is_busy())
        {
            src_start_conn(id);
        }
        break;

    case STATE_DEV_RECONNECTING:
        APP_PRINT_WARN0("general_bt_connect: reconnecting in progress");
        break;

    case STATE_DEV_CONNECTING:
        APP_PRINT_WARN0("general_bt_connect: connecting in progress");
        break;

    case STATE_DEV_CLEANUP:
        APP_PRINT_WARN1("general_bt_connect: cleaning up in progress, next event %u",
                        dev->pending_event);
        break;

    case STATE_DEV_CONNECTED:
        APP_PRINT_WARN2("general_bt_connect: dev %u (%b) is already connected",
                        id, TRACE_BDADDR(dev->remote_info.bd_addr));
        break;

    default:
        break;
    }

    return 0;
}

int general_bt_disconnect_by_bdaddr(uint8_t *bdaddr)
{
    uint8_t i;

    if (!bdaddr)
    {
        return -1;
    }

    if (shutdown)
    {
        APP_PRINT_ERROR1("general_bt_disconnect_by_bdaddr: host is down, dev %b",
                         TRACE_BDADDR(bdaddr));
        return -2;
    }


    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (!memcmp(sink_dev_list[i].remote_info.bd_addr, bdaddr, 6))
        {
            break;
        }
    }

    if (i >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("general_bt_disconnect_by_bdaddr: No id for dev %b found",
                         TRACE_BDADDR(bdaddr));
        return -3;
    }

    src_handle_dev_cleanup(i);

    return 0;
}

int general_bt_disconnect_by_id(uint8_t id)
{
    if (shutdown)
    {
        APP_PRINT_ERROR1("general_bt_disconnect_by_id: host is down dev %u", id);
        return -1;
    }

    if (id >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("general_bt_disconnect_by_id: Invaild id %u", id);
        return -2;
    }

    src_handle_dev_cleanup(id);

    return 0;
}

void general_bt_remove_bond_by_bdaddr(uint8_t *bdaddr)
{
    if (shutdown)
    {
        APP_PRINT_ERROR1("general_bt_remove_bond_by_bdaddr: host is down, dev %b",
                         TRACE_BDADDR(bdaddr));
        return;
    }

    if (!app_src_legacy_delete_bond_by_addr(bdaddr))
        APP_PRINT_ERROR1("general_bt_remove_bond_by_bdaddr: rm bond err, dev %b",
                         TRACE_BDADDR(bdaddr));

    return;
}

void general_bt_remove_bond_by_id(uint8_t id)
{
    if (shutdown)
    {
        APP_PRINT_ERROR1("general_bt_remove_bond_by_id: host is down, id %u", id);
        return;
    }

    if (id >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("general_bt_remove_bond_by_id: Invalid id %u", id);
        return;
    }

    if (!app_src_legacy_delete_bond_by_index(id))
        APP_PRINT_ERROR1("general_bt_remove_bond_by_id: rm bond for dev %u err",
                         id);

    return;
}

int general_bt_start_discovery(void)
{
    uint8_t i;

    if (shutdown)
    {
        APP_PRINT_ERROR0("general_bt_start_discovery: host is down");
        return -1;
    }

    if (client_discovering)
    {
        APP_PRINT_ERROR0("general_bt_start_discovery: It is discovering");
        return -2;
    }

    switch (discov_state)
    {
    case DISCOVERY_STARTING:
        client_discovering = 1;
        /* src_handle_inquiry_rsp() will send STARTED event to upper layer. */
        return 0;
    case DISCOVERY_FINDING:
        client_discovering = 1;
        return 0;
    case DISCOVERY_STOPPING:
        /* If it is stopping, src_handle_inquiry_cancel_cmpl() will start
         * inquiry again. And src_handle_inquiry_rsp() will send STARTED event
         * to upper layer.
         * */
        client_discovering = 1;
        return 0;
    case DISCOVERY_STOPPED:
        break;
    default:
        APP_PRINT_ERROR1("general_bt_start_discovery: Invalid discov state %u",
                         discov_state);
        return -3;
    }

    /* Here, discov_state is definitely DISCOVERY_STOPPED. */

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].bt_state == STATE_BT_PAGING)
        {
            APP_PRINT_ERROR2("general_bt_start_discovery: Connecting to %u (%b)",
                             i, TRACE_BDADDR(sink_dev_list[i].remote_info.bd_addr));
            client_discovering = 0;
            return -4;
        }
        else if (sink_dev_list[i].bt_state == STATE_BT_INQUIRY)
        {
            /* FIXME: Is it possible that local dev is inquiring? */
            client_discovering = 1;
            APP_PRINT_ERROR0("general_bt_start_discovery: It is inquiring");
            /* Later, inquiry rsp is received, and this state will be set to
             * DISCOVERY_FINDING.
             * */
            return 0;
        }
    }

    discov_state = DISCOVERY_STARTING;

    if (gap_br_start_inquiry(false, 10))
    {
        APP_PRINT_ERROR0("general_bt_start_discovery: Start inquiry err");
        discov_state = DISCOVERY_STOPPED;
        client_discovering = 0;
        return -5;
    }

    client_discovering = 1;

    return 0;
}

int general_bt_stop_discovery(void)
{
    uint8_t i;
    uint8_t pre_state;
    uint8_t pre_client_state;

    if (shutdown)
    {
        APP_PRINT_ERROR0("general_bt_stop_discovery: host is down");
        return -1;
    }

    if (!client_discovering)
    {
        APP_PRINT_ERROR0("general_bt_stop_discovery: Discovery already stopped");
        return -2;
    }

    pre_client_state = client_discovering;
    client_discovering = 0;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        if (sink_dev_list[i].bt_state == STATE_BT_INQUIRY)
        {
            APP_PRINT_INFO1("general_bt_stop_discovery: inquiring for conn dev %u", i);
            /* Discovery will be stopped because of false client_discovering
             * */
            return 0;
        }
    }

    switch (discov_state)
    {
    case DISCOVERY_STARTING:
    case DISCOVERY_FINDING:
        pre_state = discov_state;
        discov_state = DISCOVERY_STOPPING;

        if (gap_br_stop_inquiry())
        {
            APP_PRINT_ERROR0("general_bt_stop_discovery: Stop inquiry err");
            discov_state = pre_state;
            client_discovering = pre_client_state;
            return -4;
        }
        return 0;

    case DISCOVERY_STOPPING:
        return 0;
    case DISCOVERY_STOPPED:
        return 0;
    default:
        APP_PRINT_ERROR1("general_bt_stop_discovery: Unknown discov state %u",
                         discov_state);
        client_discovering = pre_client_state;
        return -3;
    }
}


int general_bt_get_bond_bdaddr(uint8_t id, uint8_t *bdaddr)
{
    if (id >= SINK_DEV_NUM_MAX)
    {
        return -1;
    }

    if (app_src_legacy_get_bond_addr_by_index(id, bdaddr))
    {
        return 0;
    }
    else
    {
        return -2;
    }
}

bool general_bt_is_connected(void)
{
    uint8_t i = 0;
    bool connected = false;
    T_APP_DEV *sink_dev = NULL;
    T_APP_BR_LINK *p_link = NULL;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];
        p_link = app_find_br_link(sink_dev->remote_info.bd_addr);
        if (!p_link)
        {
            continue;
        }
        if (sink_dev->dev_state == STATE_DEV_CONNECTED &&
            (p_link->connected_profile & A2DP_PROFILE_MASK))
        {
            connected = true;
            break;
        }
    }

    return connected;
}

void general_bt_profile_conn_cmpl(uint8_t *bd_addr, uint32_t profile_mask)
{
    src_handle_profile_conn_cmpl(bd_addr, profile_mask);
}

void general_bt_profile_disconn_cmpl(uint8_t *bd_addr, uint16_t cause,
                                     uint32_t profile_mask)
{
    src_handle_profile_disconn_cmpl(bd_addr, cause, profile_mask);
}
void general_bt_try_start_stream(void);
void general_bt_set_volume(uint32_t vol, uint8_t mute);
void general_bt_audio_capture_switch(bool active);

bool general_bt_is_idle(uint8_t id)
{
    if (id >= SINK_DEV_NUM_MAX)
    {
        APP_PRINT_ERROR1("general_bt_is_idle: Incorrect id %u", id);
        return true;
    }

    switch (sink_dev_list[id].dev_state)
    {
    case STATE_DEV_CONNECTED:
    case STATE_DEV_CONNECTING:
    case STATE_DEV_RECONNECTING:
    case STATE_DEV_CLEANUP:
        return false;
    default:
        return true;
    }
}
#endif
