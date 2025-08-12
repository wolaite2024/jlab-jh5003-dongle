/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "gap_le.h"
#include "btm.h"

#include "engage.h"
#include "bt_bond.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_bt_policy_api.h"
#include "app_bt_policy_int.h"
#include "app_linkback.h"
#include "app_bond.h"
#include "app_relay.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_multilink.h"
#if F_APP_TEAMS_BT_POLICY
#include "app_teams_hid.h"
#endif
#if F_APP_CFU_FEATURE_SUPPORT
#include "app_common_cfu.h"
#endif
#if F_APP_SINGLE_MUTLILINK_SCENERIO_1
#include "app_single_multilink_customer.h"
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
#include "app_dongle_common.h"
#include "app_dongle_dual_mode.h"
#endif

#if F_APP_SAMPLE_CONFIG_TEAMS_SINGLE_SUPPORT
#define TPOLL_IDLE                (18)
#elif F_APP_GAMING_DONGLE_SUPPORT
#define TPOLL_IDLE                (30)
#else
#define TPOLL_IDLE                (40)
#endif

#define TPOLL_LINKBACK_ACTIVE     (16)
#define TPOLL_LINKBACK_INACTIVE   (120)

#if F_APP_SAMPLE_CONFIG_TEAMS_SINGLE_SUPPORT
#define TPOLL_A2DP_ACTIVE         (12)
#else
#define TPOLL_A2DP_ACTIVE         (26)
#endif
#define TPOLL_A2DP_INACTIVE       (28)

#define TPOLL_LEA_A2DP_ACTIVE     (110)

#define TPOLL_SCO_ACTIVE          (40)
#define TPOLL_SCO_INACTIVE        (28)

#define TPOLL_DONGLE_ACTIVE       (12)
#define TPOLL_FLASH_UPDATE_ACTIVE (16)

extern T_LINKBACK_ACTIVE_NODE linkback_active_node;

extern T_BP_STATE bp_state;
extern T_EVENT cur_event;

extern T_BT_DEVICE_MODE radio_mode;

extern bool first_connect_sync_default_volume_to_src;

T_BP_TPOLL_CONTEXT tpoll_ctx = {0};

void app_bt_policy_startup(T_BP_IND_FUN fun, bool at_once_trigger)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.startup_param.ind_fun = fun;
    bt_param.startup_param.at_once_trigger = at_once_trigger;

    app_bt_policy_state_machine(EVENT_STARTUP, &bt_param);
}

void app_bt_policy_shutdown(void)
{
    app_bt_policy_state_machine(EVENT_SHUTDOWN, NULL);
}

void app_bt_policy_stop(void)
{
    app_bt_policy_state_machine(EVENT_STOP, NULL);
}

void app_bt_policy_restore(void)
{
    app_bt_policy_state_machine(EVENT_RESTORE, NULL);
}

void app_bt_policy_prepare_for_roleswap(void)
{
    app_bt_policy_state_machine(EVENT_PREPARE_FOR_ROLESWAP, NULL);
}

void app_bt_policy_msg_prof_conn(uint8_t *bd_addr, uint32_t prof)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = prof;

    app_bt_policy_state_machine(EVENT_PROFILE_CONN_SUC, &bt_param);
}

void app_bt_policy_msg_prof_disconn(uint8_t *bd_addr, uint32_t prof, uint16_t cause)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = prof;
    bt_param.cause = cause;

    app_bt_policy_state_machine(EVENT_PROFILE_DISCONN, &bt_param);
}

void app_bt_policy_enter_pairing_mode(bool force, bool visiable)
{
    T_BT_PARAM bt_param;

#if F_APP_GAMING_DONGLE_SUPPORT
    if (app_dongle_is_streaming())
    {
        APP_PRINT_TRACE0("disallow pairing when dongle streaming");
        return;
    }
#endif

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.is_force = force;
    bt_param.is_visiable = visiable;

    app_bt_policy_state_machine(EVENT_DEDICATED_ENTER_PAIRING_MODE, &bt_param);
}

void app_bt_policy_exit_pairing_mode(void)
{
    app_bt_policy_state_machine(EVENT_DEDICATED_EXIT_PAIRING_MODE, NULL);
}

void app_bt_policy_enter_dut_test_mode(void)
{
    app_bt_policy_state_machine(EVENT_ENTER_DUT_TEST_MODE, NULL);
}

#if (F_APP_OTA_TOOLING_SUPPORT == 1)
void app_bt_policy_start_ota_shaking(void)
{
    app_bt_policy_state_machine(EVENT_START_OTA_SHAKING, NULL);
}

void app_bt_policy_enter_ota_mode(bool connectable)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.is_connectable = connectable;

    app_bt_policy_state_machine(EVENT_ENTER_OTA_MODE, &bt_param);
}
#endif

void app_bt_policy_default_connect(uint8_t *bd_addr, uint32_t plan_profs, bool check_bond_flag)
{
    T_BT_PARAM bt_param;
    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = plan_profs;
    bt_param.is_special = false;
    bt_param.check_bond_flag = check_bond_flag;
    app_bt_policy_state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
}

void app_bt_policy_special_connect(uint8_t *bd_addr, uint32_t plan_prof,
                                   T_LINKBACK_SEARCH_PARAM *search_param)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = plan_prof;
    bt_param.is_special = true;
    bt_param.search_param = search_param;
    bt_param.check_bond_flag = false;
    app_bt_policy_state_machine(EVENT_DEDICATED_CONNECT, &bt_param);
}

void app_bt_policy_disconnect(uint8_t *bd_addr, uint32_t plan_profs)
{
    T_BT_PARAM bt_param;

    memset(&bt_param, 0, sizeof(T_BT_PARAM));

    bt_param.bd_addr = bd_addr;
    bt_param.prof = plan_profs;
    app_bt_policy_state_machine(EVENT_DEDICATED_DISCONNECT, &bt_param);
}

void app_bt_policy_disc_all_b2s(void)
{
    uint32_t plan_profs = 0;

    for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
    {
        if (app_link_check_b2s_link_by_id(i))
        {
            plan_profs = (app_db.br_link[i].connected_profile & (~RDTP_PROFILE_MASK));
            app_bt_policy_disconnect(app_db.br_link[i].bd_addr, plan_profs);
        }
    }
}

void app_bt_policy_conn_all_b2s(void)
{
    uint8_t bd_addr[6] = {0};
    uint8_t bond_num = bt_bond_num_get();
    uint32_t bond_flag = 0, plan_profs = 0;
    uint8_t max_load_num = (app_cfg_const.maximum_linkback_number_high_bit << 3) |
                           app_cfg_const.maximum_linkback_number;
    uint8_t num = 0;

    for (uint8_t i = 1; i <= bond_num; i++)
    {
        bond_flag = 0;
        if (bt_bond_addr_get(i, bd_addr))
        {
            bt_bond_flag_get(bd_addr, &bond_flag);

#if F_APP_GAMING_DONGLE_SUPPORT
            if (app_link_check_dongle_link(bd_addr))
            {
                continue;
            }
#endif

            if (bond_flag & (BOND_FLAG_HFP | BOND_FLAG_HSP | BOND_FLAG_A2DP))
            {
                plan_profs = app_bt_policy_get_profs_by_bond_flag(bond_flag);
                app_bt_policy_default_connect(bd_addr, plan_profs, false);
                if (++num >= max_load_num)
                {
                    break;
                }
            }
        }
    }
}

void app_bt_policy_disconnect_all_link(void)
{
    app_bt_policy_state_machine(EVENT_DISCONNECT_ALL, NULL);
}

T_BP_STATE app_bt_policy_get_state(void)
{
    return bp_state;
}

T_BT_DEVICE_MODE app_bt_policy_get_radio_mode(void)
{
    return radio_mode;
}

void app_bt_policy_sync_b2s_connected(void)
{
    APP_PRINT_TRACE1("app_bt_policy_sync_b2s_connected: b2s_connected_num %d",
                     app_link_get_b2s_link_num());

    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
    {
        app_relay_async_single(APP_MODULE_TYPE_MULTI_LINK, APP_REMOTE_MSG_PHONE_CONNECTED);
    }
}

bool app_bt_policy_get_first_connect_sync_default_vol_flag(void)
{
    return first_connect_sync_default_volume_to_src;
}

void app_bt_policy_set_first_connect_sync_default_vol_flag(bool flag)
{
    first_connect_sync_default_volume_to_src = flag;
}

#if F_APP_GAMING_DONGLE_SUPPORT
#if F_APP_GAMING_CHAT_MIXING_SUPPORT
void app_bt_policy_set_idle_zone(void)
{
    if ((app_db.remote_is_dongle) && (app_link_get_b2s_link_num() == MULTILINK_SRC_CONNECTED))
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SINGLE)
        {
            uint8_t dongle_addr[6] = {0};

            if (app_dongle_get_connected_dongle_addr(dongle_addr))
            {
                T_APP_BR_LINK *dongle_link = app_link_find_br_link(dongle_addr);

                APP_PRINT_TRACE2("app_bt_policy_set_idle_zone: %s role %d", TRACE_BDADDR(dongle_addr),
                                 dongle_link->acl_link_role);
                bt_link_idle_zone_set(dongle_addr, IDLE_ZONE_INTERVAL, IDLE_ZONE_IDLE_SLOT, IDLE_ZONE_IDLE_SKIP);
            }
        }
    }
}
#endif
#endif

static void app_bt_policy_b2s_tpoll_set(uint8_t idx, uint8_t tpoll_value)
{
    if (app_link_check_b2s_link_by_id(idx))
    {
        if (tpoll_value == TPOLL_IDLE)
        {
            tpoll_value += 2 * idx;
        }
        if (app_db.br_link[idx].tpoll_value != tpoll_value)
        {
            APP_PRINT_TRACE2("app_bt_policy_b2s_tpoll_set: bd_addr %s, tpoll_value %d",
                             TRACE_BDADDR(app_db.br_link[idx].bd_addr), tpoll_value);
            bt_link_qos_set(app_db.br_link[idx].bd_addr, BT_QOS_TYPE_GUARANTEED, tpoll_value);
            app_db.br_link[idx].tpoll_value = tpoll_value;
        }
    }
}

static void app_bt_policy_b2s_tpoll_set_both(uint8_t active_idx, uint8_t active_tpoll_value,
                                             uint8_t inactive_tpoll_value)
{
    if (app_link_get_b2s_link_num() >= 1)
    {
        app_bt_policy_b2s_tpoll_set(active_idx, active_tpoll_value);
        if (app_link_get_b2s_link_num() == 2)
        {
            for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
            {
                if (app_link_check_b2s_link_by_id(i) && (i != active_idx))
                {
                    app_bt_policy_b2s_tpoll_set(i, inactive_tpoll_value);
                    break;
                }
            }
        }
    }
}

static bool app_bt_policy_b2s_tpoll_check_state(uint8_t *bd_addr, T_BP_TPOLL_EVENT event)
{
    bool changed = false;
    T_APP_BR_LINK *p_link = NULL;
    T_BP_TPOLL_CONTEXT pre_tpoll_ctx;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        memset(&tpoll_ctx, 0, sizeof(tpoll_ctx));
        return changed;
    }

    tpoll_ctx.need_reset = false;

    memcpy(&pre_tpoll_ctx, &tpoll_ctx, sizeof(tpoll_ctx));

    switch (event)
    {
    case BP_TPOLL_EVENT_ACL_CONN:
        {
            if (linkback_active_node_judge_cur_conn_addr(bd_addr))
            {
                changed = true;
            }
        }
        break;

    case BP_TPOLL_EVENT_ACL_DISCONN:
        {
            if (app_link_get_b2s_link_num() == 0)
            {
                memset(&tpoll_ctx, 0, sizeof(tpoll_ctx));
            }
        }
        break;

#if F_APP_HID_SUPPORT
    case BP_TPOLL_EVENT_HID_CONN:
        {
            tpoll_ctx.need_reset = true;
        }
        break;
#endif

#if F_APP_TEAMS_BT_POLICY
    case BP_TPOLL_EVENT_VP_UPDATE_START:
        {
            app_teams_hid_vp_update_is_process_check(&p_link);
            if (p_link != NULL)
            {
                tpoll_ctx.in_flash_update = true;
                tpoll_ctx.flash_update_idx = p_link->id;
            }
        }
        break;

    case BP_TPOLL_EVENT_VP_UPDATE_STOP:
        {
            tpoll_ctx.in_flash_update = false;
        }
        break;
#endif

#if (F_APP_CFU_FEATURE_SUPPORT &&(F_APP_CFU_HID_SUPPORT || F_APP_TEAMS_HID_SUPPORT || F_APP_CFU_SPP_SUPPORT || F_APP_CFU_BLE_CHANNEL_SUPPORT))
    case BP_TPOLL_EVENT_CFU_START:
        {
            app_cfu_is_process_check(&p_link);
            if (p_link != NULL)
            {
                tpoll_ctx.in_flash_update = true;
                tpoll_ctx.flash_update_idx = p_link->id;
            }
        }
        break;

    case BP_TPOLL_EVENT_CFU_STOP:
        {
            tpoll_ctx.in_flash_update = false;
        }
        break;
#endif

#if F_APP_DATA_CAPTURE_SUPPORT
    case BP_TPOLL_EVENT_DATA_CAPTURE_START:
        {
            p_link = app_link_find_b2s_link(bd_addr);
            if (p_link != NULL)
            {
                tpoll_ctx.in_data_capture = true;
                tpoll_ctx.data_capture_idx = p_link->id;
            }
        }
        break;

    case BP_TPOLL_EVENT_DATA_CAPTURE_STOP:
        {
            tpoll_ctx.in_data_capture = false;
        }
        break;
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
    case BP_TPOLL_EVENT_DONGLE_SPP_START:
        {
            p_link = app_dongle_get_connected_dongle_link();
            if ((p_link != NULL) && (p_link->id == app_hfp_get_active_idx()))
            {
                tpoll_ctx.in_dongle_spp = true;
                tpoll_ctx.dongle_idx = p_link->id;
            }
        }
        break;

    case BP_TPOLL_EVENT_DONGLE_SPP_STOP:
        {
            tpoll_ctx.in_dongle_spp = false;
        }
        break;
#endif

#if F_APP_LEA_SUPPORT
    case BP_TPOLL_EVENT_LEA_START:
        {
            tpoll_ctx.in_lea = true;
        }
        break;

    case BP_TPOLL_EVENT_LEA_STOP:
        {
            tpoll_ctx.in_lea = false;
        }
        break;
#endif

    case BP_TPOLL_EVENT_SCO_START:
    case BP_TPOLL_EVENT_SCO_STOP:
        {
            tpoll_ctx.in_sco = app_hfp_sco_is_connected();
        }
        break;

    case BP_TPOLL_EVENT_A2DP_START:
    case BP_TPOLL_EVENT_A2DP_STOP:
        {
            tpoll_ctx.in_a2dp = app_a2dp_is_streaming();
        }
        break;

    case BP_TPOLL_EVENT_LINKBACK_START:
        {
            tpoll_ctx.in_linkback = true;
            changed = true;
        }
        break;

    case BP_TPOLL_EVENT_LINKBACK_STOP:
        {
            tpoll_ctx.in_linkback = false;
        }
        break;

    default:
        break;
    }

    if (memcmp(&pre_tpoll_ctx, &tpoll_ctx, sizeof(tpoll_ctx)))
    {
        changed = true;
    }

    if (changed)
    {
        APP_PRINT_TRACE3("app_bt_policy_b2s_tpoll_check_state: bd_addr %s, event 0x%x, tpoll_ctx 0x%x",
                         TRACE_BDADDR(bd_addr), event, (*(uint16_t *)&tpoll_ctx));
    }

    return changed;
}

void app_bt_policy_b2s_tpoll_update(uint8_t *bd_addr, T_BP_TPOLL_EVENT event)
{
    bool changed;
    T_APP_BR_LINK *p_link;

    changed = app_bt_policy_b2s_tpoll_check_state(bd_addr, event);
    if (!changed)
    {
        return;
    }

    if (tpoll_ctx.need_reset)
    {
        p_link = app_link_find_b2s_link(bd_addr);
        if (p_link != NULL)
        {
            app_bt_policy_b2s_tpoll_set(p_link->id, p_link->tpoll_value);
        }
        return;
    }

    if (tpoll_ctx.in_flash_update)
    {
        app_bt_policy_b2s_tpoll_set_both(tpoll_ctx.flash_update_idx, TPOLL_FLASH_UPDATE_ACTIVE, TPOLL_IDLE);
    }
#if F_APP_DATA_CAPTURE_SUPPORT
    else if (tpoll_ctx.in_data_capture)
    {
        app_bt_policy_b2s_tpoll_set_both(tpoll_ctx.data_capture_idx, app_db.data_capture_tpoll, TPOLL_IDLE);
    }
#endif
#if F_APP_GAMING_DONGLE_SUPPORT
    else if (tpoll_ctx.in_dongle_spp)
    {
        app_bt_policy_b2s_tpoll_set_both(tpoll_ctx.dongle_idx, TPOLL_DONGLE_ACTIVE, TPOLL_IDLE);
    }
#endif
    else if (tpoll_ctx.in_sco)
    {
        app_bt_policy_b2s_tpoll_set_both(app_hfp_get_active_idx(), TPOLL_SCO_ACTIVE, TPOLL_SCO_INACTIVE);
    }
    else if (tpoll_ctx.in_a2dp)
    {
#if F_APP_LEA_SUPPORT
        if (tpoll_ctx.in_lea)
        {
            app_bt_policy_b2s_tpoll_set_both(app_a2dp_get_active_idx(), TPOLL_LEA_A2DP_ACTIVE, TPOLL_IDLE);
        }
        else
#endif
        {
            app_bt_policy_b2s_tpoll_set_both(app_a2dp_get_active_idx(), TPOLL_A2DP_ACTIVE, TPOLL_A2DP_INACTIVE);
        }
    }
    else if (tpoll_ctx.in_linkback)
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_link_check_b2s_link_by_id(i))
            {
                if (linkback_active_node_judge_cur_conn_addr(app_db.br_link[i].bd_addr))
                {
                    app_bt_policy_b2s_tpoll_set(i, TPOLL_LINKBACK_ACTIVE);
                }
                else
                {
                    app_bt_policy_b2s_tpoll_set(i, TPOLL_LINKBACK_INACTIVE);
                }
            }
        }
    }
    else
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_link_check_b2s_link_by_id(i))
            {
                app_bt_policy_b2s_tpoll_set(i, TPOLL_IDLE);
            }
        }
    }
}

uint8_t *app_bt_policy_get_linkback_device(void)
{
    if (linkback_active_node.is_valid)
    {
        return linkback_active_node.linkback_node.bd_addr;
    }
    else
    {
        return NULL;
    }
}
