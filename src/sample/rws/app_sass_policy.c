#if CONFIG_REALTEK_GFPS_SASS_SUPPORT
#include <stdlib.h>
#include <string.h>
#include "bt_bond.h"
#include "trace.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_link_util.h"
#include "app_bt_policy_api.h"
#include "app_multilink.h"
#include "app_mmi.h"
#include "app_sass_policy.h"
#include "app_bond.h"
#include "bt_hfp.h"
#include "app_gfps.h"
#include "app_ota.h"
#include "gfps_sass_conn_status.h"

typedef enum
{
    LINK_SWITCH_IDLE,
    LINK_SWITCH_SWITCHING,
} T_APP_SASS_LINK_SWITCH_STATE;

typedef enum
{
    SWITCH_BACK = 0x01,
    SWITCH_BACK_AND_RESUME_PLAYING = 0x02,
} T_APP_SASS_SWITCH_BACK_EVENT;

static uint8_t original_enable_multi_link = 0xff;
static T_APP_SASS_LINK_SWITCH_STATE link_switch_state = LINK_SWITCH_IDLE;
static bool link_switch_resume = false;
static uint8_t prev_conn_addr[6] = {0};
static uint8_t prev_disc_addr[6] = {0};

/* Switching active audio source event flags */
#define GFPS_SWITCH_TO_THIS_DEVICE  0x80    // Bit 0: 1 switch to this device, 0 switch to second connected device
#define GFPS_RESUME_PLAYING         0x40    // Bit 1: 1 resume playing on switch to device after switching, 0 otherwise.
#define GFPS_REJECT_SCO             0x20    // Bit 2: 1 reject SCO on switched away device, 0 otherwise
#define GFPS_DISCONN_BLUETOOTH      0x10    // Bit 3: 1 disconnect Bluetooth on switch away device, 0 otherwise.

void app_sass_policy_sync_conn_bit_map(uint8_t bitmap)
{
    app_db.conn_bitmap = bitmap;
    APP_PRINT_TRACE1("app_sass_policy_sync_conn_bit_map: %d",  app_db.conn_bitmap);
}

uint8_t app_sass_policy_get_conn_bit_map(void)
{
    APP_PRINT_TRACE2("app_sass_policy_get_conn_bit_map: %d %d", app_cfg_nv.sass_bitmap,
                     app_db.conn_bitmap);
    return app_db.conn_bitmap;
}

bool app_sass_policy_update_conn_bit_map(uint8_t *bd_addr, bool is_conn)
{
    bool ret = false;
    uint8_t peer_addr[6] = {0};
    remote_peer_addr_get(peer_addr);

    if (memcmp(peer_addr, bd_addr, 6))
    {
        uint8_t idx = 0;

        if (app_bond_get_pair_idx_mapping(bd_addr, &idx))
        {
            if (is_conn)
            {
                app_db.conn_bitmap |= (1 << idx);
            }
            else
            {
                app_db.conn_bitmap &= ~(1 << idx);
            }

            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK,
                                       APP_REMOTE_MSG_SASS_DEVICE_BITMAP_SYNC);
            }

            ret = true;
        }
    }

    return ret;
}

static void app_sass_policy_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    bool handle = true;
    T_APP_BR_LINK *p_link = NULL;
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_ACL_CONN_SUCCESS:
    case BT_EVENT_LINK_KEY_INFO:
        {
            if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
                (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE))
            {
                if (app_sass_policy_update_conn_bit_map(param->acl_conn_success.bd_addr, true))
                {
                    app_gfps_notify_conn_status();
                }
            }
        }
        break;

    case BT_EVENT_ACL_CONN_DISCONN:
        {
            if (param->acl_conn_disconn.cause != (HCI_ERR | HCI_ERR_CONN_ROLESWAP))
            {
                if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY) ||
                    (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE))
                {
                    uint8_t idx;

                    if (app_sass_policy_update_conn_bit_map(param->acl_conn_success.bd_addr, false))
                    {
                        app_gfps_notify_conn_status();
                    }

                    app_bond_get_pair_idx_mapping(param->acl_conn_disconn.bd_addr, &idx);

                    if (link_switch_state == LINK_SWITCH_SWITCHING)
                    {
                        app_bt_policy_default_connect(prev_disc_addr,
                                                      (A2DP_PROFILE_MASK | AVRCP_PROFILE_MASK | HFP_PROFILE_MASK), true);
                    }
                    else
                    {
                        memcpy(prev_disc_addr, param->acl_conn_disconn.bd_addr, 6);
                    }
                }
            }
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_sass_policy_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_sass_policy_link_back_end(void)
{
    link_switch_state = LINK_SWITCH_IDLE;
}

void app_sass_policy_init(void)
{
    APP_PRINT_TRACE2("app_sass_policy_init: enable_multi_link %x gfps_sass_support %x",
                     app_cfg_const.enable_multi_link, extend_app_cfg_const.gfps_sass_support);

    original_enable_multi_link = app_cfg_const.enable_multi_link;
    app_cfg_const.enable_multi_link = app_cfg_nv.sass_multipoint_state;
    bt_mgr_cback_register(app_sass_policy_bt_cback);
}

void app_sass_policy_reset(void)
{
    app_cfg_nv.sass_switching_preference = 0;

    if (!app_cfg_const.disable_multilink_preemptive)
    {
        app_cfg_nv.sass_switching_preference |= SASS_A2DP_A2DP;
    }

    app_cfg_nv.sass_switching_preference |= SASS_SCO_A2DP;
    app_cfg_nv.sass_switching_preference |= SASS_VA_A2DP;

    if (original_enable_multi_link == 0xff)
    {
        app_cfg_nv.sass_multipoint_state = app_cfg_const.enable_multi_link;
    }
    else
    {
        app_cfg_nv.sass_multipoint_state = original_enable_multi_link;
        app_cfg_const.enable_multi_link = original_enable_multi_link;
    }

    app_cfg_nv.sass_bitmap = 0;
}

uint8_t app_sass_policy_switch_active_audio_source(uint8_t *bd_addr, uint8_t switch_flag, bool self)
{
    // Switch active audio source (to connected device)
    T_APP_BR_LINK *p_link;
    p_link = app_link_find_br_link(bd_addr);
    uint8_t ret = SWITCH_RET_OK;

    if (!self)
    {
        // self == 0: switch to second connected device
        uint8_t app_idx = app_multi_find_other_idx(p_link->id);

        if (app_idx == MAX_BR_LINK_NUM)
        {
            ret = SWITCH_RET_FAILED;
            return ret;
        }

        p_link = app_link_find_br_link(app_db.br_link[app_idx].bd_addr);
    }

    APP_PRINT_TRACE2("app_sass_policy_switch_active: %s, %x", TRACE_BDADDR(p_link->bd_addr),
                     switch_flag);

    if ((switch_flag & GFPS_SWITCH_TO_THIS_DEVICE) || !self)
    {
        if (app_multi_get_active_idx() == p_link->id)
        {
            ret = SWITCH_RET_REDUNDANT;
        }

        app_multi_preemptive_judge(p_link->id, MULTILINK_SASS_FORCE_PREEM);
    }

    if (switch_flag & GFPS_RESUME_PLAYING)
    {
        bt_avrcp_play(p_link->bd_addr);
    }

    if (switch_flag & GFPS_REJECT_SCO &&
        !(switch_flag & GFPS_SWITCH_TO_THIS_DEVICE) &&
        !(switch_flag & GFPS_DISCONN_BLUETOOTH))
    {
        uint8_t app_idx = app_multi_find_other_idx(p_link->id);

        if (app_db.br_link[app_idx].sco_handle)
        {
            bt_hfp_audio_disconnect_req(app_db.br_link[app_idx].bd_addr);
        }
    }

    if (switch_flag & GFPS_DISCONN_BLUETOOTH)
    {
        uint8_t app_idx = app_multi_find_other_idx(p_link->id);
        app_bt_policy_disconnect(app_db.br_link[app_idx].bd_addr, ALL_PROFILE_MASK);
    }

    return ret;
}

void app_sass_policy_set_multipoint_state(uint8_t enable)
{
    // Multipoint state:    0: switch off multipoint  1: switch on multipoint
    if (app_cfg_nv.sass_multipoint_state == enable)
    {
        return;
    }

    if (enable)
    {
        app_cfg_const.enable_multi_link = 1;
        app_cfg_nv.sass_multipoint_state = 1;
        app_cfg_const.max_legacy_multilink_devices = 2;
        app_db.b2s_connected_num_max = app_cfg_const.max_legacy_multilink_devices;
        app_mmi_handle_action(MMI_DEV_ENTER_PAIRING_MODE);
    }
    else
    {
        bool need_disc = app_sass_get_available_connection_num() == 0;
        app_cfg_const.enable_multi_link = 0;
        app_cfg_nv.sass_multipoint_state = 0;
        app_cfg_const.max_legacy_multilink_devices = 1;
        app_db.b2s_connected_num_max = app_cfg_const.max_legacy_multilink_devices;

        if (need_disc)
        {
            app_multi_disconnect_inactive_link(MAX_BR_LINK_NUM);
        }
    }

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_SASS_PREEM_BIT_SYNC);
    }
}

void app_sass_policy_profile_conn_handle(uint8_t idx)
{
    APP_PRINT_TRACE1("app_sass_policy_profile_conn_handle %s", TRACE_BDADDR(prev_conn_addr));

    if (memcmp(prev_conn_addr, app_db.br_link[idx].bd_addr, 6) == 0)
    {
        if (link_switch_resume &&
            app_db.br_link[idx].connected_profile & A2DP_PROFILE_MASK &&
            app_db.br_link[idx].connected_profile & AVRCP_PROFILE_MASK)
        {
            link_switch_resume = false;
            bt_avrcp_play(app_db.br_link[idx].bd_addr);
            memset(prev_conn_addr, 0x00, 6);
        }
    }
}

uint8_t app_sass_policy_get_disc_link(void)
{
    uint8_t count = 0;
    uint8_t bit = app_sass_policy_get_conn_bit_map();
    uint8_t disc_link = MAX_BR_LINK_NUM;
    uint8_t max_link_num = app_cfg_const.enable_multi_link ?
                           app_cfg_const.max_legacy_multilink_devices : 1;

    for (uint8_t i = 0; i < 8; i++)
    {
        if (1 << i & bit)
        {
            count++;
        }
    }

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_sass_policy_is_target_drop_device(i))
        {
            disc_link = i;
            goto ret;
        }
    }

    if ((count == 0) || (count == max_link_num)) //all sass or no sass disc inactive device
    {
        disc_link = MAX_BR_LINK_NUM;
    }
    else
    {
        T_APP_BR_LINK *p_link;
        T_APP_BR_LINK *other_link;

        p_link = app_link_find_br_link(app_db.br_link[app_multi_get_active_idx()].bd_addr);

        if (app_sass_policy_is_sass_device(p_link->bd_addr))
        {
            //active SASS, disconnect non sass if SASS is streaming or call
            if (p_link->streaming_fg || p_link->call_status != BT_HFP_CALL_IDLE)
            {
                disc_link = MAX_BR_LINK_NUM;
            }
            else
            {
                disc_link = p_link->id;
            }
        }
        else //active is Non SASS, or SASS active bur idle, means always disconnect inactive(SASS)
        {
            disc_link = MAX_BR_LINK_NUM;
        }
    }

ret:
    APP_PRINT_TRACE5("app_sass_policy_get_disc_link %d %d %d %d %d", bit, count, max_link_num,
                     disc_link, app_multi_get_active_idx());
    return disc_link;
}

void app_sass_policy_switch_back(uint8_t event)
{
    bool resume = false;
    uint8_t resume_idx = MAX_BR_LINK_NUM;

    if (event ==  SWITCH_BACK_AND_RESUME_PLAYING)
    {
        resume = true;
    }

    APP_PRINT_TRACE2("app_sass_policy_switch_back: %s, %d", TRACE_BDADDR(prev_disc_addr), resume);

    if (link_switch_state != LINK_SWITCH_IDLE)
    {
        return;
    }

    link_switch_state = LINK_SWITCH_SWITCHING;

    if (resume)
    {
        if (app_cfg_const.enable_multi_link)
        {
            if (app_link_get_b2s_link_num() > 1)
            {
                for (uint8_t i = 0 ; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_link_check_b2b_link_by_id(i) &&
                        (memcmp(prev_disc_addr, app_db.br_link[i].bd_addr, 6) != 0))
                    {
                        resume_idx = i;
                        break;
                    }
                }
            }
        }
        else
        {
            memcpy(prev_conn_addr, prev_disc_addr, 6);
            link_switch_resume = resume;
        }
    }

    if (resume && (resume_idx != MAX_BR_LINK_NUM))
    {
        bt_avrcp_play(app_db.br_link[resume_idx].bd_addr);
        app_multi_preemptive_judge(resume_idx, MULTILINK_SASS_FORCE_PREEM);
    }
}

void app_sass_policy_set_capability(uint8_t *addr)
{
    T_APP_BR_LINK *p_link = app_link_find_br_link(addr);
    uint8_t idx;

    app_bond_get_pair_idx_mapping(addr, &idx);
    app_cfg_nv.sass_bitmap |= (1 << idx);
    app_gfps_notify_conn_status();

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_SASS_DEVICE_SUPPORT_SYNC);
    }

    APP_PRINT_TRACE1("app_sass_policy_set_capability %x", app_cfg_nv.sass_bitmap);
}

void app_sass_policy_initiated_connection(uint8_t *addr, bool triggered_by_audio_switch)
{
    T_APP_BR_LINK *p_link = app_link_find_br_link(addr);

    APP_PRINT_TRACE3("app_sass_policy_initiated_connection %d, %x, %x", p_link->streaming_fg,
                     app_cfg_nv.sass_bitmap, triggered_by_audio_switch);

    if (triggered_by_audio_switch && p_link->streaming_fg)
    {
        app_multi_preemptive_judge(p_link->id, MULTILINK_SASS_FORCE_PREEM);
    }
}

bool app_sass_policy_is_sass_device(uint8_t *addr)
{
    bool ret = true;
    uint8_t idx;

    if (app_multi_get_active_idx() < MAX_BR_LINK_NUM)
    {
        app_bond_get_pair_idx_mapping(addr, &idx);

        if (!(app_cfg_nv.sass_bitmap & (1 << idx)))
        {
            ret = false;
        }
    }

    APP_PRINT_TRACE2("app_sass_policy_is_sass_device: %d %d", idx, ret);

    return ret;
}

bool app_sass_policy_is_target_drop_device(uint8_t idx)
{
    bool ret = false;

    if ((app_db.br_link[idx].used) &&
        (!memcmp(app_db.br_link[idx].bd_addr, app_db.sass_target_drop_device, 6)))
    {
        APP_PRINT_TRACE1("app_sass_policy_is_target_drop_device %b",
                         TRACE_BDADDR(app_db.br_link[idx].bd_addr));
        ret = true;
    }

    return ret;
}

void app_sass_policy_set_switch_preference(uint8_t flag)
{
    uint16_t bit_mask = 0b1111111111110000;

    app_cfg_nv.sass_switching_preference &= bit_mask;
    app_cfg_nv.sass_switching_preference |= (uint16_t)(flag & ~bit_mask);

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_SASS_PREEM_BIT_SYNC);
    }
}

uint8_t app_sass_policy_get_switch_preference(void)
{
    uint8_t bit_mask = 0b00001111;
    return (uint8_t) app_cfg_nv.sass_switching_preference & (bit_mask);
}

void app_sass_policy_set_advanced_switching_setting(uint8_t flag)
{
    app_cfg_nv.sass_advanced_switching_setting = flag;

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_SASS_SWITCH_SYNC);
    }
}

uint8_t app_sass_policy_get_advanced_switching_setting(void)
{
    return app_cfg_nv.sass_advanced_switching_setting;
}

T_SASS_CONN_STATE app_sass_policy_get_connection_state(void)
{
    if (extend_app_cfg_const.gfps_sass_support == 0)
    {
        APP_PRINT_TRACE0("Not support sass");
        return SASS_DISABLE_CONN_SWITCH;
    }

    T_SASS_CONN_STATE ret = SASS_DISABLE_CONN_SWITCH;

    uint8_t active_idx = app_multi_get_active_idx();
    T_APP_BR_LINK *p_link;
    p_link = app_link_find_br_link(app_db.br_link[active_idx].bd_addr);

    if (app_link_get_b2s_link_num() == 0)
    {
        if (linkback_todo_queue_node_num())
        {
            ret = SASS_PAGING;
        }

        else if (app_ota_dfu_is_busy())
        {
            ret = SASS_DISABLE_CONN_SWITCH;
        }
        else
        {
            ret = SASS_NO_CONN;
            //check LE audio
        }
    }
    else
    {
        if (app_ota_dfu_is_busy())
        {
            ret = SASS_DISABLE_CONN_SWITCH;
        }
        else if (p_link->call_status != BT_HFP_CALL_IDLE)
        {
            ret = SASS_HFP;
        }
        else if (p_link->streaming_fg)
        {
            if (p_link->avrcp_play_status == BT_AVRCP_PLAY_STATUS_PLAYING &&
                !app_multi_get_stream_only(active_idx))
            {
                ret = SASS_A2DP_WITH_AVRCP;
            }
            else
            {
                ret = SASS_A2DP_ONLY;
            }
        }
        else
        {
            ret = SASS_CONN_NO_DATA_TRANS;
            //check LE audio
        }
    }

    APP_PRINT_TRACE1("app_sass_policy_get_connection_state ret 0x%x", ret);
    return ret;
}

bool app_sass_policy_get_original_enable_multi_link(void)
{
    return original_enable_multi_link;
}

uint8_t app_sass_get_available_connection_num(void)
{
    uint8_t b2s_link_num = app_link_get_b2s_link_num();
    uint8_t max_link_num = app_cfg_const.enable_multi_link ?
                           app_cfg_const.max_legacy_multilink_devices : 1;
    uint8_t ret = max_link_num - b2s_link_num;

    APP_PRINT_TRACE1("app_sass_get_available_connection_num %d", ret);

    return ret;
}
#endif
