/**
*****************************************************************************************
*     Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      multitopology_pro.c
   * @brief     This file handles Multi-Topology setup process routines.
   * @author    mj.mengjie.han
   * @date      2021-11-18
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2021 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
*                              Header Files
*============================================================================*/
#include "trace.h"
#include "mem_types.h"
#include "os_mem.h"
#include "app_timer.h"
#include "app_main.h"
#include "app_mmi.h"
#include "app_auto_power_off.h"
#include "app_cfg.h"
#include "multitopology_ctrl.h"
#include "app_sniff_mode.h"
#include "app_relay.h"
#include "app_lea_broadcast_audio.h"

/*============================================================================*
 *                              Constants
 *============================================================================*/
#define mtc_pairing      "cis_pairing"
#define mtc_linkback     "cis_linkback"
#define mtc_lossback     "cis_linkloss_linkback"
#define mtc_scan         "bis_scan"
#define mtc_resync       "bis_resync"
#define mtc_totalback    "total_link_back"
#define mtc_gap(name) mtc_##name

typedef enum
{
    MTC_TIMER_CH_BIS,
    MTC_TIMER_CH_LINKBACK_ALL,
    MTC_TIMER_CH_CHANNELID_MAX,
} T_MTC_TIMER_CH_CHANNELID;

typedef void (*MTC_TMR)(uint8_t timer_id);

typedef struct
{
    MTC_TMR tmr_callback;
    MTC_GAP_HANDLER handler;
    //char **p_name;
    const char *const *p_name;
} T_MTC_GAP_MAP;

typedef struct
{
    MTC_GAP_HANDLER handler;
    uint16_t timer_chann;
} T_MTC_GAP;

typedef struct
{
    T_MTC_GAP cis_gap_handler;
    T_MTC_GAP bis_gap_handler;
    T_MTC_GAP linkback_total;
    uint8_t         id;
} T_MTC_GAP_DB;

static void mtc_bis_timer(uint8_t timer_id);
static void mtc_timer(uint8_t timer_id);
/*============================================================================*
 *                              Variables
 *============================================================================*/

static T_MTC_GAP_DB mtc_setup_db =
{
    .bis_gap_handler = {0, 0},
    .linkback_total = {0, 0},
    .id = 0,
};

const static char *const p_bis_name_table[] = {"bis_scan", "bis_resync"};
const static char *const p_other_name_table[] = {"total_link_back"};
static T_MTC_GAP_MAP mtc_tmr_table[2];

/*============================================================================*
 *                              Functions
 *============================================================================*/

static void mtc_device_poweroff(void)
{
    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
    {
        app_db.power_off_cause = POWER_OFF_CAUSE_EXIT_PAIRING_MODE;
        app_mmi_handle_action(MMI_DEV_POWER_OFF);
    }
    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        uint8_t action = MMI_DEV_POWER_OFF;
        app_relay_sync_single(APP_MODULE_TYPE_MMI, action, REMOTE_TIMER_HIGH_PRECISION,
                              0, false);
    }
}

bool mtc_device_idle(void)
{
    if (!app_lea_bca_state() &&
        app_link_get_lea_link_num() == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void mtc_device_sniff(bool allow_poweroff, bool allow_sniff)
{
    if (allow_sniff)
    {
        app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_LEA);
        app_sniff_mode_b2b_enable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_LEA);
    }
    else
    {
        app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_LEA);
        app_sniff_mode_b2b_disable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_LEA);
    }
    if (allow_poweroff)
    {
        app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BLE_AUDIO, app_cfg_const.timer_auto_power_off);
    }
}

static void mtc_check_device_state(void)
{
    if (app_lea_mgr_is_streaming())
    {
        mtc_device_sniff(false, false);
    }
    else if (app_link_get_lea_link_num())
    {
        mtc_device_sniff(false, true);
    }
    else
    {
        mtc_device_sniff(true, true);
    }
}

bool mtc_device_poweroff_check(T_MTC_EVENT event)
{
    bool result = true;
    switch (event)
    {
    case MTC_EVENT_BIS_SCAN_TO:
    case MTC_EVENT_BIS_RESYNC_TO:
        {
            mtc_check_device_state();

            if (app_cfg_const.power_off_bis_to &&
                app_bt_policy_listening_allow_poweroff() &&
                (app_link_get_b2s_link_num() == 0) &&
                !app_bt_policy_is_pairing() &&
                mtc_device_idle() &&
                !app_lea_mgr_is_streaming())
            {
                mtc_device_poweroff();
            }
            mtc_topology_dm(MTC_TOPO_EVENT_BIS_STOP);
        }
        break;

    case MTC_EVENT_LEGACY_PAIRING_TO:
        {
            if (mtc_device_idle())
            {
                result = true;
            }
            else
            {
                result = false;
            }
        }
        break;

    default:
        result = false;
        break;
    }
    return result;
}

/**
 * @brief   Gap Timer call back .
 *
 * @param[timer_id] num Limitation of link quantity.
 * @param[timer_chann] num Limitation of link quantity.
 */

static void mtc_bis_timer(uint8_t timer_id)
{
    APP_PRINT_TRACE1("mtc_bis_timer: timer_id 0x%02x", timer_id);
    switch (timer_id)
    {
#if F_APP_TMAP_BMR_SUPPORT
    case MTC_BIS_TMR_SCAN:
        {
            mtc_device_poweroff_check(MTC_EVENT_BIS_SCAN_TO);
            app_lea_bca_sm(LEA_BCA_SCAN_TIMEOUT, NULL);

            // mtc_resume_a2dp(mtc_get_resume_a2dp_idx());
            //app_sniff_mode_b2s_enable_all(SNIFF_DISABLE_MASK_LEA);
            //app_sniff_mode_b2b_enable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_LEA);
            //app_auto_power_off_enable(AUTO_POWER_OFF_MASK_BLE_AUDIO, app_cfg_const.timer_auto_power_off);

        }
        break;

    case MTC_BIS_TMR_RESYNC:
        {
            mtc_device_poweroff_check(MTC_EVENT_BIS_RESYNC_TO);
            app_lea_bca_sm(LEA_BCA_SCAN_TIMEOUT, NULL);
        }
        break;
#endif

    default:
        break;
    }
}

static void mtc_timer(uint8_t timer_id)
{
    APP_PRINT_TRACE1("mtc_timer: timer_id 0x%02x", timer_id);
    switch (timer_id)
    {
    case T_MTC_OTHER_TMR_LINKBACK_TOTAL:
        {
        }
        break;

    default:
        break;
    }
}

void mtc_stop_timer(T_MTC_TMR tmt)
{
    if (tmt >= MTC_TMR_MAX)
    {
        APP_PRINT_TRACE0("mtc_stop_timer: no timer handler");
        return;
    }
    app_stop_timer(&mtc_tmr_table[tmt].handler);
    mtc_tmr_table[tmt].handler = NULL;
}

static void mtc_gap_timer_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE1("mtc_gap_timer_cb: timer_chann %d", timer_chann);
    if (timer_chann >= MTC_TIMER_CH_CHANNELID_MAX)
    {
        return;
    }
    mtc_stop_timer((T_MTC_TMR)timer_chann);
    ((MTC_TMR)mtc_tmr_table[timer_chann].tmr_callback)(timer_id);

}

bool mtc_exist_handler(T_MTC_TMR tmt)
{
    if (mtc_tmr_table[tmt].handler)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//Add out side user gap
void mtc_start_timer(T_MTC_TMR tmt, uint8_t opt, uint32_t timeout)
{
    if (tmt >= MTC_TMR_MAX  || timeout <= 0)
    {
        APP_PRINT_TRACE0("mtc_start_timer: no timer handler or timeout zero");
        return;
    }
    APP_PRINT_TRACE3("mtc_start_timer: tmt %d, opt=%d, %d", tmt, opt, timeout);

    if (tmt == MTC_TMR_BIS)
    {
        if ((T_MTC_BIS_TMR)opt >= MTC_BIS_TMR_MAX)
        {
            return;
        }
    }
    else if (tmt == MTC_TMR_OTHER)
    {
        if ((T_MTC_OTHER_TMR)opt >= T_MTC_OTHER_TMR_MAX)
        {
            return;
        }
    }

    app_start_timer(&mtc_tmr_table[tmt].handler, mtc_tmr_table[tmt].p_name[opt], mtc_setup_db.id,
                    opt, (T_MTC_TIMER_CH_CHANNELID)tmt, false, timeout);
}



void mct_gap_timer_init(void)
{
    app_timer_reg_cb(mtc_gap_timer_cb, &mtc_setup_db.id);

    mtc_tmr_table[0].tmr_callback = mtc_bis_timer;
    mtc_tmr_table[0].handler = mtc_setup_db.bis_gap_handler.handler;
    mtc_tmr_table[0].p_name = p_bis_name_table;

    mtc_tmr_table[1].tmr_callback = mtc_timer;
    mtc_tmr_table[1].handler = mtc_setup_db.linkback_total.handler;
    mtc_tmr_table[1].p_name = p_other_name_table;
}
