/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "ascs_mgr.h"
#include "bas.h"
#include "bass_mgr.h"
#include "ble_conn.h"
#include "bt_direct_msg.h"
#include "bt_types.h"
#include "bt_gatt_client.h"
#include "bt_syscall.h"
#include "gap_iso_data.h"
#include "ccp_client.h"
#include "mcp_client.h"
#include "vcs_mgr.h"
#include "audio_probe.h"
#include "app_lea_broadcast_audio.h"
#include "app_lea_ccp.h"
#include "app_cfg.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_io_msg.h"
#include "app_lea_adv.h"
#include "app_lea_ascs.h"
#include "app_lea_mcp.h"
#include "app_lea_mgr.h"
#include "app_lea_scan.h"
#include "app_lea_profile.h"
#include "app_lea_vcs.h"
#include "app_lea_vol_def.h"
#include "app_link_util.h"
#include "app_main.h"
#include "app_mmi.h"
#include "app_multilink.h"
#include "app_sniff_mode.h"
#include "app_auto_power_off.h"
#include "app_audio_policy.h"
#include "app_ipc.h"
#include "app_ota.h"
#include "app_lea_pacs.h"
#include "app_report.h"
#include "app_lea_csis.h"
#include "app_lea_unicast_audio.h"
#include "app_lea_vol_policy.h"
#include "app_bud_loc.h"
#include "app_sensor.h"
#include "app_roleswap_control.h"
#include "app_vendor.h"

#if F_APP_QOL_MONITOR_SUPPORT
#include "app_qol.h"
#endif

#if (F_APP_AIRPLANE_SUPPORT == 1)
#include "app_airplane.h"
#endif

#if F_APP_TUYA_SUPPORT
#include "rtk_tuya_ble_ota.h"
#endif

#if F_APP_24G_BT_AUDIO_SOURCE_CTRL_SUPPORT
#include "app_dongle_source_ctrl.h"
#endif

#if F_APP_LISTENING_MODE_SUPPORT
#include "app_listening_mode.h"
#endif

#if F_APP_LEA_REALCAST_SUPPORT
#include "app_cmd.h"
#include "app_lea_realcast.h"
#include "app_lea_vol_def.h"
#endif

#if F_APP_LEA_DONGLE_BINDING
#include "app_dongle_dual_mode.h"
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
#include "app_dongle_common.h"
#include "app_usb_vol_control.h"
#include "app_dongle_data_ctrl.h"
#endif
/*============================================================================*
 *                              Variables
 *============================================================================*/

#if F_APP_LEA_SUPPORT
#define DATA_PATH_MASK          (DATA_PATH_INPUT_FLAG|DATA_PATH_OUTPUT_FLAG)

#define LEA_REF_GUARD_TIME_US   3000

//For two CISes, pixel sometimes sends 6 packets for one handle and exceeds iso queue size
#define LEA_ISO_QUEUE_MAX       8

#define LEA_ISO_DATA_DELTA_TIMESTAMP 3000

static T_ISOCH_DATA_PKT_STATUS app_lea_mgr_iso_data_status = ISOCH_DATA_PKT_STATUS_LOST_DATA;
static uint8_t lea_mgr_current_mcp_state = MCS_MEDIA_STATE_INACTIVE;
static bool lea_mgr_media_suspend_by_out_ear = false;
static bool lea_mgr_gaming_streaming = false;
uint8_t lea_bas_id = 0;

static void app_lea_mgr_legacy_mode(bool *result);
static bool app_lea_mgr_sniff_check(T_MTC_AUDIO_MODE mode);

/*============================================================================*
 *                              Functions
 *============================================================================*/
bool app_lea_mgr_is_media_streaming(void)
{
    bool ret = false;

    if (mtc_get_btmode() == MULTI_PRO_BT_CIS)
    {
#if F_APP_MCP_SUPPORT
        T_APP_LE_LINK *p_link;
        uint16_t conn_handle = app_lea_mcp_get_active_conn_handle();

        p_link = app_link_find_le_link_by_conn_handle(conn_handle);

        if (p_link != NULL)
        {
            if (p_link->gmcs)
            {
                if (p_link->media_state == MCS_MEDIA_STATE_PLAYING)
                {
                    ret = true;
                }
            }
            else
            {
                if ((p_link->cis_left_ch_conn_handle != 0) ||
                    (p_link->cis_right_ch_conn_handle != 0))
                {
                    ret = true;
                }
            }
        }
#endif
    }
    else if (mtc_get_btmode() == MULTI_PRO_BT_BIS)
    {
        if (app_lea_bca_state() == LEA_BCA_STATE_STREAMING)
        {
            ret = true;
        }
    }

    return ret;
}

bool app_lea_mgr_is_streaming(void)
{
    for (uint8_t idx = 0; idx < MAX_BLE_LINK_NUM; idx++)
    {
        if (app_db.le_link[idx].used &&
            app_db.le_link[idx].lea_link_state == LEA_LINK_STREAMING)
        {
            return true;
        }
    }

    if (app_lea_bca_state() == LEA_BCA_STATE_STREAMING)
    {
        return true;
    }

    return false;
}

bool app_lea_mgr_is_upstreaming(void)
{
    for (uint8_t idx = 0; idx < MAX_BLE_LINK_NUM; idx++)
    {
        if (app_db.le_link[idx].used &&
            app_db.le_link[idx].lea_link_state == LEA_LINK_STREAMING)
        {
            if (app_lea_ascs_find_ase_entry_by_direction(&app_db.le_link[idx], DATA_PATH_INPUT_FLAG) != NULL)
            {
                return true;
            }
        }
    }

    return false;
}

void app_lea_mgr_update_current_mcp_state(void *p_link_info, uint8_t mcp_prev_state)
{
    T_APP_LE_LINK *p_link = (T_APP_LE_LINK *)p_link_info;

    APP_PRINT_TRACE3("app_lea_mgr_update_current_mcp_state: media_state %d, mcp_prev_state %d, lea_mgr_media_suspend_by_out_ear %d",
                     p_link->media_state, mcp_prev_state, lea_mgr_media_suspend_by_out_ear);

    if (((mcp_prev_state == MCS_MEDIA_STATE_PAUSED &&
          p_link->media_state == MCS_MEDIA_STATE_PLAYING)) &&
        lea_mgr_media_suspend_by_out_ear)
    {
        app_lea_mgr_update_media_suspend_by_out_ear(false);
    }

    lea_mgr_current_mcp_state = p_link->media_state;
}

void app_lea_mgr_clear_iso_queue(T_OS_QUEUE *p_queue)
{
    while (p_queue->count > 0)
    {
        audio_probe_media_buffer_free(os_queue_out(p_queue));
    }
}

void app_lea_mgr_set_gaming_state(bool state)
{
    lea_mgr_gaming_streaming = state;
    APP_PRINT_INFO1("app_lea_mgr_set_gaming_state: 0x%x", lea_mgr_gaming_streaming);
}

void app_lea_mgr_enqueue_iso_data(T_OS_QUEUE *p_queue, T_BT_DIRECT_ISO_DATA_IND *p_iso_data,
                                  uint16_t output_handle)
{
    T_ISO_DATA_IND *p_iso_buf = NULL;

    if (output_handle != p_iso_data->conn_handle)
    {
        APP_PRINT_TRACE2("app_lea_mgr_enqueue_iso_data: output_handle 0x%02X not match data_handle 0x%02X",
                         output_handle, p_iso_data->conn_handle);
        return;
    }

    p_iso_buf = calloc(1, sizeof(T_ISO_DATA_IND) + p_iso_data->iso_sdu_len);

    if (p_iso_buf == NULL)
    {
        return;
    }

    p_iso_buf->conn_handle = p_iso_data->conn_handle;
    p_iso_buf->time_stamp = p_iso_data->time_stamp;
    p_iso_buf->pkt_seq_num = p_iso_data->pkt_seq_num;
    p_iso_buf->pkt_status_flag = p_iso_data->pkt_status_flag;
    p_iso_buf->p_buf = (uint8_t *)(p_iso_buf + 1);

    if (p_iso_data->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
    {
        memcpy(p_iso_buf->p_buf, p_iso_data->p_buf + p_iso_data->offset, p_iso_data->iso_sdu_len);
        p_iso_buf->iso_sdu_len = p_iso_data->iso_sdu_len;
    }
    else
    {
        p_iso_buf->iso_sdu_len = 0;
    }

    if (p_queue->count > LEA_ISO_QUEUE_MAX)
    {
        T_ISO_DATA_IND *p_iso_elem = (T_ISO_DATA_IND *)os_queue_out(p_queue);

        if (p_iso_elem != NULL)
        {
            free(p_iso_elem);
        }
        APP_PRINT_TRACE0("app_lea_mgr_enqueue_iso_data: more than LEA_ISO_QUEUE_MAX");
    }

    os_queue_in(p_queue, (void *)p_iso_buf);
    return;
}

static bool app_lea_mgr_is_iso_queue_timestamp_valid(T_BT_DIRECT_ISO_DATA_IND *p_direct_iso,
                                                     T_ISO_DATA_IND *p_iso_elem)
{
    if ((p_direct_iso == NULL) || (p_iso_elem == NULL))
    {
        return false;
    }

    if (p_direct_iso->time_stamp >= p_iso_elem->time_stamp)
    {
        if (p_direct_iso->time_stamp - p_iso_elem->time_stamp < LEA_ISO_DATA_DELTA_TIMESTAMP)
        {
            return true;
        }
        else if (0xFFFFFFFF - p_direct_iso->time_stamp + p_iso_elem->time_stamp <
                 LEA_ISO_DATA_DELTA_TIMESTAMP)
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
        if (p_iso_elem->time_stamp - p_direct_iso->time_stamp < LEA_ISO_DATA_DELTA_TIMESTAMP)
        {
            return true;
        }
        else if (0xFFFFFFFF - p_iso_elem->time_stamp + p_direct_iso->time_stamp <
                 LEA_ISO_DATA_DELTA_TIMESTAMP)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}


T_ISO_DATA_IND *app_lea_mgr_find_iso_elem(T_OS_QUEUE *p_queue,
                                          T_BT_DIRECT_ISO_DATA_IND *p_direct_iso)
{
    if ((p_queue == NULL) || (p_direct_iso == NULL))
    {
        return NULL;
    }

    while (p_queue->count)
    {
        T_ISO_DATA_IND *p_iso_elem = (T_ISO_DATA_IND *)os_queue_peek(p_queue, 0);

        if (p_iso_elem != NULL &&
            app_lea_mgr_is_iso_queue_timestamp_valid(p_direct_iso, p_iso_elem))
        {
            return p_iso_elem;
        }
        else
        {
            if (p_iso_elem == NULL)
            {
                APP_PRINT_TRACE0("app_lea_mgr_find_iso_elem: not find iso elem");
                return NULL;
            }

            APP_PRINT_TRACE6("app_lea_mgr_find_iso_elem: iso time 0x%02X, \
queue time 0x%02X, iso handle 0x%02X, queue handle 0x%02X, iso seq 0x%02X, queue seq 0x%02X",
                             p_direct_iso->time_stamp, p_iso_elem->time_stamp,
                             p_direct_iso->conn_handle, p_iso_elem->conn_handle,
                             p_direct_iso->pkt_seq_num, p_iso_elem->pkt_seq_num);

            if (p_direct_iso->time_stamp > p_iso_elem->time_stamp)
            {
                os_queue_delete(p_queue, p_iso_elem);
                free(p_iso_elem);
            }
            else
            {
                return NULL;
            }
        }
    }

    return NULL;
}

bool app_lea_mgr_dev_ctrl(uint8_t para, uint8_t *data)
{
    bool result = false;

    switch (para)
    {
    case LEA_DEV_CTRL_SET_IDLE:
        {
            app_lea_mgr_legacy_mode(&result);
        }
        break;

    case LEA_DEV_CTRL_GET_LEGACY:
        {
            if (app_cfg_const.legacy_enable)
            {
                result = true;
            }
        }
        break;

    case LEA_DEV_CTRL_SET_RADIO:
        {
            T_BT_DEVICE_MODE device_mode = *((T_BT_DEVICE_MODE *)data);

            if (app_cfg_const.legacy_enable ||
                //For B2B legacy connection, primary needs to change to connectable mode
                ((app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED) &&
                 (device_mode != BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)))
            {
                result = bt_device_mode_set(device_mode);
            }

            if (device_mode == BT_DEVICE_MODE_CONNECTABLE ||
                device_mode == BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE)
            {
                app_lea_adv_start();
            }
        }
        break;

    case LEA_DEV_CTRL_GET_CIS_POLICY:
        {
            if ((app_cfg_const.iso_mode == LE_AUDIO_CIS) || (app_cfg_const.iso_mode == LE_AUDIO_ALL))
            {
                result = true;
            }
        }
        break;
    }
    return result;
}

void app_lea_mgr_set_media_state(uint8_t para)
{
    app_lea_mgr_iso_data_status = (T_ISOCH_DATA_PKT_STATUS)para;
}

bool app_lea_mgr_handle_vol_mmi(uint8_t action)
{
    bool ret = false;
    T_APP_LE_LINK *p_lea_link = NULL;
    p_lea_link = app_dongle_get_le_audio_dongle_link();

    if (p_lea_link == NULL)
    {
        goto exit;
    }

    T_LEA_ASE_ENTRY *p_ase_entry;
    p_ase_entry = app_lea_ascs_find_ase_entry_by_direction(p_lea_link, DATA_PATH_OUTPUT_FLAG);

    if (action == MMI_DEV_SPK_VOL_UP)
    {
        if (p_lea_link)
        {
            app_dongle_send_vol_ctrl_cmd(SPK_VOL_UP);
            app_usb_audio_volume_control_handle(SPK_VOL_UP, p_ase_entry->track_handle);
            app_usb_play_vol_max_min_tone(SPK_VOL_UP);
            ret = true;
        }
    }
    else if (action == MMI_DEV_SPK_VOL_DOWN)
    {
        if (p_lea_link)
        {
            app_dongle_send_vol_ctrl_cmd(SPK_VOL_DOWN);
            app_usb_audio_volume_control_handle(SPK_VOL_DOWN, p_ase_entry->track_handle);
            app_usb_play_vol_max_min_tone(SPK_VOL_DOWN);
            ret = true;
        }
    }

exit:
    return ret;
}

void app_lea_mgr_tri_mmi_handle_action(uint8_t action, bool inter)
{
    if (!app_cfg_const.iso_mode)
    {
        return;
    }
    APP_PRINT_TRACE5("app_lea_mgr_tri_mmi_handle_action: action 0x%02X, b2s %d, b2b %d, local %d, lea_mode %d",
                     action, mtc_get_b2s_sniff_status(), mtc_get_b2d_sniff_status(), inter, app_cfg_const.iso_mode);
    switch (action)
    {
#if F_APP_TMAP_BMR_SUPPORT
    case MMI_BIG_START:
        {
            bool need_return = false;

            if (!(app_cfg_const.iso_mode & LE_AUDIO_BIS)
#if (F_APP_AIRPLANE_SUPPORT == 1)
                || app_airplane_mode_get()
#endif
               )
            {
                break;
            }

            if (app_hfp_get_call_status() != APP_CALL_IDLE &&
                (mtc_if_fm_lbis(LBIS_TO_MTC_CHECK_RESUME, NULL, &need_return) == MTC_RESULT_SUCCESS) &&
                !need_return)
            {
                break;
            }
#if F_APP_QOL_MONITOR_SUPPORT
            app_qol_link_monitor(app_cfg_nv.bud_peer_addr, false);
            app_qol_link_monitor(app_db.br_link[app_a2dp_get_active_idx()].bd_addr, false);
#endif
            if (app_lea_bca_state() == LEA_BCA_STATE_IDLE)
            {
                app_lea_bca_sm(LEA_BCA_MMI, NULL);
            }
            else if (!inter)
            {
                app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_AUDIO);
                break;
            }

            if (app_lea_mgr_sniff_check(LE_AUDIO_BIS))
            {
                break;
            }
            mtc_stream_switch(false);
            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_LEA);
            app_sniff_mode_b2b_disable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_LEA);
            app_lea_bca_sm(LEA_BCA_BIS_SYNC, NULL);
            app_auto_power_off_disable(AUTO_POWER_OFF_MASK_BLE_AUDIO);
        }
        break;

    case MMI_BIG_STOP:
        {
            if (!(app_cfg_const.iso_mode & LE_AUDIO_BIS)
#if (F_APP_AIRPLANE_SUPPORT == 1)
                || app_airplane_mode_get()
#endif
               )
            {
                break;
            }

            if (app_lea_bca_state() == LEA_BCA_STATE_IDLE)
            {
                mtc_set_beep(MTC_PRO_BEEP_PROMPTLY);
                break;
            }

            if (mtc_get_beep() == MTC_PRO_BEEP_PROMPTLY)
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_audio_tone_type_play(TONE_BIS_STOP, false, false);
                }
                else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    app_audio_tone_type_play(TONE_BIS_STOP, false, true);
                }
            }
            app_lea_bca_bmr_terminate();
        }
        break;
#endif

    default:
        break;
    }

}

static bool app_lea_mgr_check_pri_only_action(uint8_t action, T_MTC_BT_MODE bt_mode)
{
    bool is_pri_only_action = true;

    switch (action)
    {
    case MMI_DEV_SPK_VOL_UP:
    case MMI_DEV_SPK_VOL_DOWN:
    case MMI_DEV_SPK_MUTE:
    case MMI_DEV_SPK_UNMUTE:
        {
            is_pri_only_action = false;
#if F_APP_VCS_SUPPORT
            if (app_lea_vcs_find_vc_link())
            {
                is_pri_only_action = true;
            }
#endif
        }
        break;

    case MMI_DEV_GAMING_MODE_SWITCH:
    case MMI_DEV_MIC_MUTE_UNMUTE:
    case MMI_DEV_MIC_MUTE:
    case MMI_DEV_MIC_UNMUTE:
        {
            is_pri_only_action = false;
        }
        break;

    default:
        break;
    }

    return is_pri_only_action;
}

void app_lea_mgr_mmi_handle(uint8_t action)
{
    uint16_t conn_handle = 0;
    bool handle = true;
    T_APP_LE_LINK *p_link = NULL;
    T_MTC_BT_MODE bt_mode = mtc_get_btmode();
#if F_APP_MCP_SUPPORT
    T_MCP_CLIENT_WRITE_MEDIA_CP_PARAM param;
#endif
#if F_APP_CCP_SUPPORT
    T_CCP_CLIENT_WRITE_CALL_CP_PARAM write_call_cp_param = {0};
#endif

    if (action == MMI_BIG_START ||
        action == MMI_BIG_STOP)
    {
        app_lea_mgr_tri_mmi_handle_action(action, false);
        return;
    }

    if (action == MMI_DEV_POWER_OFF)
    {
        mtc_set_dev(2);
        if (bt_mode  == MULTI_PRO_BT_BIS)
        {
            mtc_set_beep(MTC_PRO_BEEP_NONE);
            app_lea_mgr_tri_mmi_handle_action(MMI_BIG_STOP, false);
        }
        return;
    }

    //secondary bud handle the action or not
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY &&
        app_lea_mgr_check_pri_only_action(action, bt_mode))
    {
        return;
    }

    if (bt_mode == MULTI_PRO_BT_CIS &&
        action != MMI_DEV_GAMING_MODE_SWITCH)
    {
        if (action <= MMI_HF_RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL)
        {
#if F_APP_CCP_SUPPORT
            conn_handle = app_lea_ccp_get_active_conn_handle();
#endif
            p_link = app_link_find_le_link_by_conn_handle(conn_handle);
        }
        else if ((action >= MMI_AV_PLAY_PAUSE && action <= MMI_AV_REWIND_STOP) ||
                 action == MMI_AV_PAUSE)
        {
#if F_APP_MCP_SUPPORT
            conn_handle = app_lea_mcp_get_active_conn_handle();
#endif
            p_link = app_link_find_le_link_by_conn_handle(conn_handle);
        }
        else if ((action == MMI_DEV_SPK_VOL_UP) ||
                 (action == MMI_DEV_SPK_VOL_DOWN) ||
                 (action == MMI_DEV_SPK_MUTE) ||
                 (action == MMI_DEV_SPK_UNMUTE) ||
                 (action == MMI_DEV_MIC_UNMUTE) ||
                 (action == MMI_DEV_MIC_MUTE) ||
                 (action == MMI_DEV_MIC_MUTE_UNMUTE))
        {
            if (app_lea_ccp_get_call_status() != APP_CALL_IDLE)
            {
#if F_APP_CCP_SUPPORT
                conn_handle = app_lea_ccp_get_active_conn_handle();
#endif
                p_link = app_link_find_le_link_by_conn_handle(conn_handle);
            }
            else
            {
#if F_APP_MCP_SUPPORT
                conn_handle = app_lea_mcp_get_active_conn_handle();
#endif
                p_link = app_link_find_le_link_by_conn_handle(conn_handle);
            }
        }
        else
        {
            APP_PRINT_TRACE1("app_lea_mgr_mmi_handle: not support action 0x%02X", action);
            return;
        }

        if (p_link == NULL)
        {
            if ((action == MMI_HF_LAST_NUMBER_REDIAL) || (action == MMI_AV_PLAY_PAUSE))
            {
                //TODO: change to target and direct adv to linkback last device
            }

            return;
        }
    }

    switch (action)
    {
#if F_APP_MCP_SUPPORT
    case MMI_AV_PLAY_PAUSE:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                if (p_link->media_state == MCS_MEDIA_STATE_PLAYING)
                {
                    // CIS will effect A2DP stutter when switch to A2DP
                    param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PAUSE;
                }
                else
                {
                    param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PLAY;
                }

                mcp_client_write_media_cp(conn_handle, 0, p_link->gmcs, &param, true);
            }
            else
            {
                mtc_if_fm_lcis(LCIS_TO_MTC_TER_CIS, NULL, NULL);
            }
        }
        break;

    case MMI_AV_PAUSE:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                if (p_link->media_state == MCS_MEDIA_STATE_PLAYING)
                {
                    param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PAUSE;
                    mcp_client_write_media_cp(conn_handle, 0, p_link->gmcs, &param, true);
                }
            }
        }
        break;

    case MMI_AV_STOP:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                if (p_link->media_state == MCS_MEDIA_STATE_PLAYING)
                {
                    param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_STOP;
                    mcp_client_write_media_cp(conn_handle, 0, p_link->gmcs, &param, true);
                }
            }
        }
        break;

    case MMI_AV_FWD:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_NEXT_TRACK;
                mcp_client_write_media_cp(conn_handle, 0, p_link->gmcs, &param, true);
            }
        }
        break;

    case MMI_AV_BWD:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PREVIOUS_TRACK;
                mcp_client_write_media_cp(conn_handle, 0, p_link->gmcs, &param, true);
            }
        }
        break;

    case MMI_AV_REWIND:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_FAST_REWIND;
                mcp_client_write_media_cp(conn_handle, 0, p_link->gmcs, &param, true);
            }
        }
        break;

    case MMI_AV_FASTFORWARD:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_FAST_FORWARD;
                mcp_client_write_media_cp(conn_handle, 0, p_link->gmcs, &param, true);
            }
        }
        break;

    case MMI_AV_REWIND_STOP:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PLAY;
                mcp_client_write_media_cp(conn_handle, 0, p_link->gmcs, &param, true);
            }
        }
        break;

    case MMI_AV_FASTFORWARD_STOP:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                param.opcode = MCS_MEDIA_CONTROL_POINT_CHAR_OPCODE_PLAY;
                mcp_client_write_media_cp(conn_handle, 0, p_link->gmcs, &param, true);
            }
        }
        break;
#endif

#if F_APP_CCP_SUPPORT
    case MMI_HF_END_OUTGOING_CALL:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                T_LEA_CALL_ENTRY *p_call_entry;

                p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, p_link->active_call_index);
                if (p_call_entry != NULL)
                {
                    if ((p_call_entry->call_state == TBS_CALL_STATE_DIALING) ||
                        (p_call_entry->call_state == TBS_CALL_STATE_ALERTING))
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE;
                        write_call_cp_param.param.terminate_opcode_call_index = p_call_entry->call_index;

                        ccp_client_write_call_cp(p_link->conn_handle, 0, p_link->gtbs, false, &write_call_cp_param);
                    }
                }
            }
        }
        break;

    case MMI_HF_ANSWER_CALL:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                T_LEA_CALL_ENTRY *p_call_entry;

                p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, p_link->active_call_index);
                if (p_call_entry != NULL)
                {
                    if (p_call_entry->call_state == TBS_CALL_STATE_INCOMING)
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ACCEPT;
                        write_call_cp_param.param.accept_opcode_call_index = p_call_entry->call_index;

                        ccp_client_write_call_cp(p_link->conn_handle, 0, p_link->gtbs, false, &write_call_cp_param);
                    }
                }
            }
        }
        break;

    case MMI_HF_REJECT_CALL:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                T_LEA_CALL_ENTRY *p_call_entry;

                p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, p_link->active_call_index);
                if (p_call_entry != NULL)
                {
                    if (p_call_entry->call_state == TBS_CALL_STATE_INCOMING)
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE;
                        write_call_cp_param.param.terminate_opcode_call_index = p_call_entry->call_index;

                        if (ccp_client_write_call_cp(p_link->conn_handle, 0, p_link->gtbs, false, &write_call_cp_param))
                        {
                            app_db.reject_call_by_key = true;
                        }
                    }
                }
            }
        }
        break;

    case MMI_HF_END_ACTIVE_CALL:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                T_LEA_CALL_ENTRY *p_call_entry;

                p_call_entry = app_lea_ccp_find_call_entry_by_idx(p_link->conn_id, p_link->active_call_index);
                if (p_call_entry != NULL)
                {
                    write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE;
                    write_call_cp_param.param.terminate_opcode_call_index = p_call_entry->call_index;

                    ccp_client_write_call_cp(p_link->conn_handle, 0, p_link->gtbs, false, &write_call_cp_param);
                }
            }
        }
        break;

    case MMI_HF_LAST_NUMBER_REDIAL:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                if (p_link->active_call_uri != NULL)
                {
                    write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ORIGINATE;
                    write_call_cp_param.param.originate_opcode_param.p_uri = p_link->active_call_uri;
                    write_call_cp_param.param.originate_opcode_param.uri_len = strlen((char *)p_link->active_call_uri);

                    ccp_client_write_call_cp(p_link->conn_handle, 0, p_link->gtbs, false, &write_call_cp_param);
                    app_audio_tone_type_play(TONE_HF_CALL_REDIAL, false, true);
                }
            }
        }
        break;

    case MMI_HF_SWITCH_TO_SECOND_CALL:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                uint8_t i;
                int32_t ret = 0;
                T_APP_LE_LINK *p_active_link = NULL;
                T_APP_LE_LINK *p_inactive_link = NULL;
                T_LEA_CALL_ENTRY *p_active_call_entry = NULL;
                T_LEA_CALL_ENTRY *p_inactive_call_entry = NULL;
                T_APP_CALL_STATUS call_status = app_lea_ccp_get_call_status();

                for (i = 0; i < MAX_BLE_LINK_NUM; i++)
                {
                    if (app_db.le_link[i].used == true)
                    {
                        if (app_db.le_link[i].conn_handle == app_lea_ccp_get_active_conn_handle())
                        {
                            p_active_link = &app_db.le_link[i];
                        }
                        else
                        {
                            p_inactive_link = &app_db.le_link[i];
                        }
                    }
                }

                if ((call_status == APP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT) ||
                    (call_status == APP_MULTILINK_CALL_ACTIVE_WITH_CALL_HELD))
                {
                    if (p_inactive_link == NULL)
                    {
                        ret = 1;
                        goto fail_switch_to_second_call;
                    }

                    for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
                    {
                        if (p_inactive_link->lea_call_entry[i].used == true &&
                            p_inactive_link->lea_call_entry[i].call_index == p_inactive_link->active_call_index)
                        {
                            p_active_call_entry = &p_inactive_link->lea_call_entry[i];
                            break;
                        }
                    }

                    if (p_active_call_entry == NULL)
                    {
                        ret = 2;
                        goto fail_switch_to_second_call;
                    }

                    if ((p_active_call_entry->call_state == TBS_CALL_STATE_LOCALLY_HELD) ||
                        (p_active_call_entry->call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD))
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_RETRIEVE;
                        write_call_cp_param.param.local_retrieve_opcode_call_index = p_active_call_entry->call_index;

                        ccp_client_write_call_cp(p_inactive_link->conn_handle, 0, p_inactive_link->gtbs, false,
                                                 &write_call_cp_param);
                    }
                }
                else
                {
                    if (p_active_link == NULL)
                    {
                        ret = 3;
                        goto fail_switch_to_second_call;
                    }

                    for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
                    {
                        if (p_active_link->lea_call_entry[i].used == true &&
                            p_active_link->lea_call_entry[i].call_index == p_active_link->active_call_index)
                        {
                            p_active_call_entry = &p_active_link->lea_call_entry[i];
                            break;
                        }
                    }

                    if (p_active_call_entry == NULL)
                    {
                        ret = 4;
                        goto fail_switch_to_second_call;
                    }

                    if (p_active_call_entry->call_state == TBS_CALL_STATE_ACTIVE)
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_HOLD;
                        write_call_cp_param.param.local_hold_opcode_call_index = p_active_call_entry->call_index;

                        ccp_client_write_call_cp(p_active_link->conn_handle, 0, p_active_link->gtbs, false,
                                                 &write_call_cp_param);
                    }

                    for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
                    {
                        if (p_active_link->lea_call_entry[i].used == true &&
                            p_active_link->lea_call_entry[i].call_index != p_active_link->active_call_index)
                        {
                            p_inactive_call_entry = &p_active_link->lea_call_entry[i];
                            break;
                        }
                    }

                    if (p_inactive_call_entry == NULL)
                    {
                        ret = 5;
                        goto fail_switch_to_second_call;
                    }

                    if ((p_inactive_call_entry->call_state == TBS_CALL_STATE_LOCALLY_HELD) ||
                        (p_inactive_call_entry->call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD))
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_RETRIEVE;
                        write_call_cp_param.param.local_retrieve_opcode_call_index = p_inactive_call_entry->call_index;

                        ccp_client_write_call_cp(p_active_link->conn_handle, 0, p_active_link->gtbs, false,
                                                 &write_call_cp_param);
                    }
                    else if (p_inactive_call_entry->call_state == TBS_CALL_STATE_INCOMING)
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ACCEPT;
                        write_call_cp_param.param.accept_opcode_call_index = p_inactive_call_entry->call_index;

                        ccp_client_write_call_cp(p_active_link->conn_handle, 0, p_active_link->gtbs, false,
                                                 &write_call_cp_param);
                    }
                }

fail_switch_to_second_call:
                APP_PRINT_ERROR1("app_lea_mgr_mmi_handle: switch to second call fail %d", -ret);
                return;
            }
        }
        break;

    case MMI_HF_JOIN_TWO_CALLS:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                uint8_t i, j = 0;
                uint8_t call_index[CCP_CALL_ENTRY_NUM];

                for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
                {
                    if (p_link->lea_call_entry[i].used == true &&
                        p_link->lea_call_entry[i].call_index != 0)
                    {
                        call_index[j++] = p_link->lea_call_entry[i].call_index;
                    }
                }

                if (j == CCP_CALL_ENTRY_NUM)
                {
                    write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_JOIN;
                    write_call_cp_param.param.join_opcode_param.p_list_of_call_indexes = call_index;
                    write_call_cp_param.param.join_opcode_param.list_of_call_indexes_len = CCP_CALL_ENTRY_NUM;

                    ccp_client_write_call_cp(p_link->conn_handle, 0, p_link->gtbs, false, &write_call_cp_param);
                }
            }
        }
        break;

    case MMI_HF_RELEASE_HELD_OR_WAITING_CALL:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                uint8_t i;
                int32_t ret = 0;
                T_APP_LE_LINK *p_active_link = NULL;
                T_APP_LE_LINK *p_inactive_link = NULL;
                T_LEA_CALL_ENTRY *p_active_call_entry = NULL;
                T_LEA_CALL_ENTRY *p_inactive_call_entry = NULL;
                T_APP_CALL_STATUS call_status = app_lea_ccp_get_call_status();

                for (i = 0; i < MAX_BLE_LINK_NUM; i++)
                {
                    if (app_db.le_link[i].used == true)
                    {
                        if (app_db.le_link[i].conn_handle == app_lea_ccp_get_active_conn_handle())
                        {
                            p_active_link = &app_db.le_link[i];
                        }
                        else
                        {
                            p_inactive_link = &app_db.le_link[i];
                        }
                    }
                }

                if ((call_status == APP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT) ||
                    (call_status == APP_MULTILINK_CALL_ACTIVE_WITH_CALL_HELD))
                {
                    if (p_inactive_link == NULL)
                    {
                        ret = 1;
                        goto fail_release_held_or_waiting_call;
                    }

                    for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
                    {
                        if (p_inactive_link->lea_call_entry[i].used == true &&
                            p_inactive_link->lea_call_entry[i].call_index == p_inactive_link->active_call_index)
                        {
                            p_active_call_entry = &p_inactive_link->lea_call_entry[i];
                            break;
                        }
                    }

                    if (p_active_call_entry == NULL)
                    {
                        ret = 2;
                        goto fail_release_held_or_waiting_call;
                    }

                    if ((p_active_call_entry->call_state < TBS_CALL_STATE_ACTIVE) ||
                        (p_active_call_entry->call_state == TBS_CALL_STATE_LOCALLY_HELD) ||
                        (p_active_call_entry->call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD))
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE;
                        write_call_cp_param.param.local_retrieve_opcode_call_index = p_active_call_entry->call_index;

                        ccp_client_write_call_cp(p_inactive_link->conn_handle, 0, p_inactive_link->gtbs, false,
                                                 &write_call_cp_param);
                    }
                }
                else
                {
                    if (p_active_link == NULL)
                    {
                        ret = 3;
                        goto fail_release_held_or_waiting_call;
                    }

                    for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
                    {
                        if (p_active_link->lea_call_entry[i].used == true &&
                            p_active_link->lea_call_entry[i].call_index != p_active_link->active_call_index)
                        {
                            p_inactive_call_entry = &p_active_link->lea_call_entry[i];
                            break;
                        }
                    }

                    if (p_inactive_call_entry == NULL)
                    {
                        ret = 4;
                        goto fail_release_held_or_waiting_call;
                    }

                    if ((p_inactive_call_entry->call_state < TBS_CALL_STATE_ACTIVE) ||
                        (p_inactive_call_entry->call_state == TBS_CALL_STATE_LOCALLY_HELD) ||
                        (p_inactive_call_entry->call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD))
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE;
                        write_call_cp_param.param.local_retrieve_opcode_call_index = p_inactive_call_entry->call_index;

                        ccp_client_write_call_cp(p_active_link->conn_handle, 0, p_active_link->gtbs, false,
                                                 &write_call_cp_param);
                    }
                }

fail_release_held_or_waiting_call:
                APP_PRINT_ERROR1("app_lea_mgr_mmi_handle: release held or waiting call fail %d", -ret);
                return;
            }
        }
        break;

    case MMI_HF_RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                uint8_t i;
                int32_t ret = 0;
                T_APP_LE_LINK *p_active_link = NULL;
                T_APP_LE_LINK *p_inactive_link = NULL;
                T_LEA_CALL_ENTRY *p_active_call_entry = NULL;
                T_LEA_CALL_ENTRY *p_inactive_call_entry = NULL;
                T_APP_CALL_STATUS call_status = app_lea_ccp_get_call_status();

                for (i = 0; i < MAX_BLE_LINK_NUM; i++)
                {
                    if (app_db.le_link[i].used == true)
                    {
                        if (app_db.le_link[i].conn_handle == app_lea_ccp_get_active_conn_handle())
                        {
                            p_active_link = &app_db.le_link[i];
                        }
                        else
                        {
                            p_inactive_link = &app_db.le_link[i];
                        }
                    }
                }

                if ((call_status == APP_MULTILINK_CALL_ACTIVE_WITH_CALL_WAIT) ||
                    (call_status == APP_MULTILINK_CALL_ACTIVE_WITH_CALL_HELD))
                {
                    app_lea_ccp_release_active_call(p_active_link);
                    app_lea_ccp_accept_hold_or_wait_call(p_inactive_link);
                }
                else
                {
                    if (p_active_link == NULL)
                    {
                        ret = 1;
                        goto fail_release_active_call_accept_held_or_waiting_call;
                    }

                    for (i = 0; i < CCP_CALL_ENTRY_NUM; i++)
                    {
                        if (p_active_link->lea_call_entry[i].used == true)
                        {
                            if (p_active_link->lea_call_entry[i].call_index == p_active_link->active_call_index)
                            {
                                p_active_call_entry = &p_active_link->lea_call_entry[i];
                            }
                            else
                            {
                                p_inactive_call_entry = &p_active_link->lea_call_entry[i];
                            }
                        }
                    }

                    if (p_active_call_entry == NULL)
                    {
                        ret = 2;
                        goto fail_release_active_call_accept_held_or_waiting_call;
                    }

                    if (p_active_call_entry->call_state == TBS_CALL_STATE_ACTIVE)
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE;
                        write_call_cp_param.param.local_retrieve_opcode_call_index = p_active_call_entry->call_index;

                        ccp_client_write_call_cp(p_active_link->conn_handle, 0, p_active_link->gtbs, false,
                                                 &write_call_cp_param);
                    }

                    if (p_inactive_call_entry == NULL)
                    {
                        ret = 3;
                        goto fail_release_active_call_accept_held_or_waiting_call;
                    }

                    if ((p_inactive_call_entry->call_state == TBS_CALL_STATE_LOCALLY_HELD) ||
                        (p_inactive_call_entry->call_state == TBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD))
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_RETRIEVE;
                        write_call_cp_param.param.local_retrieve_opcode_call_index = p_inactive_call_entry->call_index;

                        ccp_client_write_call_cp(p_active_link->conn_handle, 0, p_active_link->gtbs, false,
                                                 &write_call_cp_param);
                    }
                    else if (p_inactive_call_entry->call_state < TBS_CALL_STATE_ACTIVE)
                    {
                        write_call_cp_param.opcode = TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ACCEPT;
                        write_call_cp_param.param.local_retrieve_opcode_call_index = p_inactive_call_entry->call_index;

                        ccp_client_write_call_cp(p_active_link->conn_handle, 0, p_active_link->gtbs, false,
                                                 &write_call_cp_param);
                    }
                }

fail_release_active_call_accept_held_or_waiting_call:
                APP_PRINT_ERROR1("app_lea_mgr_mmi_handle: release active call accept held or waiting call fail %d",
                                 -ret);
            }
        }
        break;

#endif

    case MMI_DEV_SPK_VOL_UP:
        {
            T_LEA_VOL_CHG_RESULT change_result;

            change_result = app_lea_vol_local_volume_change(LEA_SPK_UP);
            if (change_result == LEA_VOL_LEVEL_CHANGE)
            {
                app_lea_vol_update_track_volume();

#if F_APP_LEA_REALCAST_SUPPORT
                if (bt_mode == MULTI_PRO_BT_BIS)
                {
                    uint8_t volume_out = VOLUME_LEVEL(app_cfg_nv.lea_vol_setting);

                    app_lea_realcast_state_notify(CMD_LEA_CTRL_VOL, CMD_STATUS_SUCCESS, VOL_VALUE_STATE,
                                                  PKT_TYPE_COMPLETE, &volume_out, 1);
                }
#endif
            }

            if ((change_result == LEA_VOL_LEVEL_LIMIT) ||
                ((change_result == LEA_VOL_LEVEL_CHANGE) && (app_cfg_nv.lea_vol_setting == MAX_VOLUME_SETTING)))
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_audio_tone_type_play(TONE_VOL_MAX, false, false);
                }
                else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    app_audio_tone_type_play(TONE_VOL_MAX, false, true);
                }
            }
        }
        break;

    case MMI_DEV_SPK_VOL_DOWN:
        {
            T_LEA_VOL_CHG_RESULT change_result;

            change_result = app_lea_vol_local_volume_change(LEA_SPK_DOWN);
            if (change_result == LEA_VOL_LEVEL_CHANGE)
            {
                app_lea_vol_update_track_volume();

#if F_APP_LEA_REALCAST_SUPPORT
                if (bt_mode == MULTI_PRO_BT_BIS)
                {
                    uint8_t volume_out = VOLUME_LEVEL(app_cfg_nv.lea_vol_setting);

                    app_lea_realcast_state_notify(CMD_LEA_CTRL_VOL, CMD_STATUS_SUCCESS, VOL_VALUE_STATE,
                                                  PKT_TYPE_COMPLETE, &volume_out, 1);
                }
#endif
            }

            if ((change_result == LEA_VOL_LEVEL_LIMIT) ||
                ((change_result == LEA_VOL_LEVEL_CHANGE) && (app_cfg_nv.lea_vol_setting == 0)))
            {
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    app_audio_tone_type_play(TONE_VOL_MIN, false, false);
                }
                else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                {
                    app_audio_tone_type_play(TONE_VOL_MIN, false, true);
                }
            }
        }
        break;

    case MMI_DEV_GAMING_MODE_SWITCH:
        {
#if F_APP_GAMING_DONGLE_SUPPORT && F_APP_LEA_SUPPORT
            APP_PRINT_ERROR0("app_lea_mgr_mmi_handle: MMI_DEV_GAMING_MODE_SWITCH: LE gaming dongle does not support switch gaming mode");
#else
            if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY &&
                app_link_get_lea_link_num() == 0)
            {
                break;
            }

            APP_PRINT_TRACE1("app_lea_mgr_mmi_handle: MMI_DEV_GAMING_MODE_SWITCH mode %d",
                             app_db.gaming_mode);

            if (app_link_get_b2s_link_num() == 0)
            {
                app_db.gaming_mode ^= 1;

                app_lea_pacs_update_low_latency(app_db.gaming_mode);
                app_report_gaming_mode_info();

                if (app_db.gaming_mode)
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                    {
                        app_audio_tone_type_play(TONE_ENTER_GAMING_MODE, false, false);
                    }
                    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        app_audio_tone_type_play(TONE_ENTER_GAMING_MODE, false, true);
                    }
                }
                else
                {
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                    {
                        app_audio_tone_type_play(TONE_EXIT_GAMING_MODE, false, false);
                    }
                    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
                    {
                        app_audio_tone_type_play(TONE_EXIT_GAMING_MODE, false, true);
                    }
                }
            }
#endif
        }
        break;

    case MMI_DEV_SPK_MUTE:
        {
            if (app_lea_vol_local_volume_change(LEA_SPK_MUTE) == LEA_VOL_MUTE_CHANGE)
            {
                app_lea_vol_update_track_volume();
            }
        }
        break;

    case MMI_DEV_SPK_UNMUTE:
        {
            if (app_lea_vol_local_volume_change(LEA_SPK_UNMUTE) == LEA_VOL_MUTE_CHANGE)
            {
                app_lea_vol_update_track_volume();
            }
        }
        break;

    case MMI_DEV_MIC_MUTE_UNMUTE:
    case MMI_DEV_MIC_MUTE:
    case MMI_DEV_MIC_UNMUTE:
        {
            if (bt_mode == MULTI_PRO_BT_CIS)
            {
                app_lea_uca_set_mic_mute(p_link, action, true);
            }
        }
        break;

    default:
        {
            handle = false;
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE2("app_lea_mgr_mmi_handle: action 0x%02X, conn_handle 0x%02X", action,
                         conn_handle);
    }
}

bool app_lea_mgr_ase_exist_conn(uint16_t conn_handle)
{
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CONN, conn_handle, NULL);
    if (p_ase_entry != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void app_lea_mgr_update_sniff_mask(void)
{
    if (app_lea_mgr_is_streaming())
    {
        app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_LEA);
        app_sniff_mode_b2b_disable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_LEA);
    }
}

bool app_lea_mgr_get_link_info(uint8_t *sets, uint8_t *active_index, uint8_t *para)
{
    T_APP_LE_LINK *p_link = NULL;
    uint8_t offset = 0;
    bool ret = false;
    uint16_t active_conn_handle = app_lea_mcp_get_active_conn_handle();

    *sets = 0;

    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        p_link = &app_db.le_link[i];

        if (p_link->used)
        {
            (*sets) ++;
            if (p_link->conn_handle == active_conn_handle)
            {
                *active_index = *sets;
            }

            memcpy(&para[offset], p_link->bd_addr, 6);
            offset += 6;
        }
    }
    return ret;
}

/*============================================================================*
 *                              Internal Functions
 *============================================================================*/
static void app_lea_mgr_legacy_mode(bool *result)
{
    *result = false;
    if (!app_cfg_const.legacy_enable)
    {
        for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY &&
                app_db.br_link[i].connected_profile == RDTP_PROFILE_MASK)
            {
                bt_device_mode_set(BT_DEVICE_MODE_IDLE);
                *result = true;
                break;
            }
        }
    }
}

static bool app_lea_mgr_sniff_check(T_MTC_AUDIO_MODE mode)
{
    if ((mtc_get_b2d_sniff_status() != MTC_BUD_STATUS_ACTIVE) &&
        (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED))
    {
        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
        {
            app_sniff_mode_b2s_disable_all(SNIFF_DISABLE_MASK_LEA);
            app_sniff_mode_b2b_disable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_LEA);

        }
        mtc_set_pending(mode);
        return true;
    }
    return false;
}

bool app_lea_mgr_get_media_suspend_by_out_ear(void)
{
    return lea_mgr_media_suspend_by_out_ear;
}

void app_lea_mgr_update_media_suspend_by_out_ear(bool out_ear)
{
    lea_mgr_media_suspend_by_out_ear = out_ear;

    APP_PRINT_TRACE1("app_lea_mgr_update_media_suspend_by_out_ear: out_ear 0x%02X", out_ear);

    if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
        (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY))
    {
        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_LEA,
                                            LEA_REMOTE_MEDIA_SUSPEND_SYNC,
                                            (uint8_t *)&lea_mgr_media_suspend_by_out_ear, 1);
    }
}

void app_lea_mgr_sync_media_suspend_by_remote(uint8_t remote_media_suspend)
{
    uint8_t local_media_suspend = app_lea_mgr_get_media_suspend_by_out_ear();

    APP_PRINT_TRACE4("app_lea_mgr_handle_media_resume: remote suspend %d, local suspend %d, mcp_state %d, suspend %d",
                     remote_media_suspend, local_media_suspend, lea_mgr_current_mcp_state,
                     lea_mgr_media_suspend_by_out_ear);

    if (remote_media_suspend == true || local_media_suspend == true)
    {
        lea_mgr_media_suspend_by_out_ear = true;

        if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY && app_db.is_lea_device_connected)
        {
            if (app_db.local_loc == BUD_LOC_IN_EAR &&
                app_db.remote_loc == BUD_LOC_IN_EAR)
            {
                if (lea_mgr_current_mcp_state == MCS_MEDIA_STATE_PAUSED ||
                    lea_mgr_current_mcp_state == MCS_MEDIA_STATE_INACTIVE)
                {
                    app_mmi_handle_action(MMI_AV_PLAY_PAUSE);
                    app_lea_mgr_update_media_suspend_by_out_ear(false);
                }
            }
        }
    }
}

static void app_lea_mgr_spk_mute_ctrl(bool muted)
{
    if (muted)
    {
        app_db.playback_muted = true;
        app_db.voice_muted = true;

        audio_volume_out_mute(AUDIO_STREAM_TYPE_PLAYBACK);
        audio_volume_out_mute(AUDIO_STREAM_TYPE_VOICE);
        audio_volume_out_mute(AUDIO_STREAM_TYPE_RECORD);
    }
    else
    {
        app_db.playback_muted = false;
        app_db.voice_muted = false;

        audio_volume_out_unmute(AUDIO_STREAM_TYPE_PLAYBACK);
        audio_volume_out_unmute(AUDIO_STREAM_TYPE_VOICE);
        audio_volume_out_unmute(AUDIO_STREAM_TYPE_RECORD);
    }
}

static uint8_t app_lea_mgr_out_ear_action(bool from_remote, uint8_t cause_action)
{
    uint8_t action = LEA_LOC_ACTION_AUDIO_NOTHING;
    T_APP_LE_LINK *p_link = NULL;
    T_MTC_BT_MODE bt_mode = mtc_get_btmode();
    uint8_t result = 0;

    if (bt_mode == MULTI_PRO_BT_CIS)
    {
        if (app_lea_ccp_get_call_status() != APP_CALL_IDLE)
        {
            p_link = app_link_find_le_link_by_conn_handle(app_lea_ccp_get_active_conn_handle());

            if (p_link == NULL)
            {
                result = 1;
                goto RESULT;
            }

            if (!app_lea_uca_get_mic_mute_state() && !app_lea_uca_check_enable_mic())
            {
                app_lea_uca_set_mic_mute(p_link, MMI_DEV_MIC_MUTE, false);
            }
        }
        else
        {
            p_link = app_link_find_le_link_by_conn_handle(app_lea_mcp_get_active_conn_handle());

            if (p_link == NULL)
            {
                result = 2;
                goto RESULT;
            }
            else if (p_link->media_state == MCS_MEDIA_STATE_PLAYING)
            {
                action = LEA_LOC_ACTION_AUDIO_PAUSE;
            }
            APP_PRINT_TRACE2("app_lea_mgr_out_ear_action: conn_handle 0x%02X, media_state %d",
                             p_link->conn_handle, p_link->media_state);
        }
    }

RESULT:
    APP_PRINT_TRACE4("app_lea_mgr_out_ear_action: from_remote %d, cause_action %d, result %d, action %d",
                     from_remote, cause_action, result, action);
    return action;
}

static uint8_t app_lea_mgr_in_ear_action(bool from_remote, uint8_t cause_action)
{
    uint8_t action = LEA_LOC_ACTION_AUDIO_NOTHING;
    T_APP_LE_LINK *p_link = NULL;
    T_MTC_BT_MODE bt_mode = mtc_get_btmode();
    uint8_t result = 0;

    if (bt_mode == MULTI_PRO_BT_CIS)
    {
        if (app_lea_ccp_get_call_status() != APP_CALL_IDLE)
        {
            if (from_remote)
            {
                return action;
            }

            p_link = app_link_find_le_link_by_conn_handle(app_lea_ccp_get_active_conn_handle());

            if (p_link == NULL)
            {
                result = 1;
                goto RESULT;
            }

            if (!app_lea_uca_get_mic_mute_state() && app_lea_uca_check_enable_mic())
            {
                app_lea_uca_set_mic_mute(p_link, MMI_DEV_MIC_UNMUTE, false);
            }
        }
        else
        {
            p_link = app_link_find_le_link_by_conn_handle(app_lea_mcp_get_active_conn_handle());

            if (p_link == NULL)
            {
                result = 2;
                goto RESULT;
            }
            else if ((p_link->media_state == MCS_MEDIA_STATE_INACTIVE ||
                      p_link->media_state == MCS_MEDIA_STATE_PAUSED) &&
                     lea_mgr_media_suspend_by_out_ear)
            {
                action = LEA_LOC_ACTION_AUDIO_RESUME;
            }
            APP_PRINT_TRACE2("app_lea_mgr_in_ear_action: conn_handle 0x%02X, media_state %d",
                             p_link->conn_handle, p_link->media_state);
        }
    }

RESULT:
    APP_PRINT_TRACE4("app_lea_mgr_in_ear_action: from_remote %d, cause_action %d, result %d, action %d",
                     from_remote, cause_action, result, action);
    return action;
}

static uint8_t app_lea_mgr_audio_in_case_action(void)
{
    uint8_t action = LEA_LOC_ACTION_AUDIO_NOTHING;
    T_APP_LE_LINK *p_link = NULL;
    T_MTC_BT_MODE bt_mode = mtc_get_btmode();
    uint8_t result = 0;

    if (bt_mode == MULTI_PRO_BT_CIS &&
        ((app_db.local_loc == BUD_LOC_IN_CASE) && (app_db.remote_loc == BUD_LOC_IN_CASE)))
    {
        if (app_lea_ccp_get_call_status() != APP_CALL_IDLE)
        {
            p_link = app_link_find_le_link_by_conn_handle(app_lea_ccp_get_active_conn_handle());

            if (p_link == NULL)
            {
                result = 1;
                goto RESULT;
            }

            app_lea_uca_set_mic_mute(p_link, MMI_DEV_MIC_UNMUTE, false);
        }
    }

RESULT:
    APP_PRINT_TRACE3("app_lea_mgr_audio_in_case_action: local_loc %d, remote_loc %d, result %d",
                     app_db.local_loc, app_db.remote_loc, result);

    return action;
}

static void app_lea_mgr_action_when_bud_loc_changed(uint8_t action)
{
    APP_PRINT_TRACE3("app_lea_mgr_action_when_bud_loc_changed: action 0x%02X, lea_mgr_current_mcp_state 0x%02X, roleswap status 0x%02X",
                     action, lea_mgr_current_mcp_state, app_roleswap_ctrl_get_status());

    if ((app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY) ||
        (app_roleswap_ctrl_get_status() != APP_ROLESWAP_STATUS_IDLE))
    {
        return;
    }

    switch (action)
    {
    case LEA_LOC_ACTION_AUDIO_PAUSE:
        {
            if (lea_mgr_current_mcp_state == MCS_MEDIA_STATE_PLAYING)
            {
                app_mmi_handle_action(MMI_AV_PLAY_PAUSE);
                app_lea_mgr_update_media_suspend_by_out_ear(true);
            }
        }
        break;

    case LEA_LOC_ACTION_AUDIO_RESUME:
        {
            if (lea_mgr_current_mcp_state == MCS_MEDIA_STATE_PAUSED ||
                lea_mgr_current_mcp_state == MCS_MEDIA_STATE_INACTIVE)
            {
                app_mmi_handle_action(MMI_AV_PLAY_PAUSE);
            }
            app_lea_mgr_update_media_suspend_by_out_ear(false);
        }
        break;

    default:
        break;
    }
}

static void app_lea_mgr_bud_loc_event_cback(uint32_t event, void *msg)
{
    bool handle = true;
    uint8_t action = 0;
    uint8_t *p_info = msg;
    bool from_remote = p_info[0];

    if (mtc_get_btmode() == MULTI_PRO_BT_BREDR)
    {
        return;
    }

#if (F_APP_SENSOR_SUPPORT == 1)
    uint8_t cause_action = p_info[1];
    //clear action flag after sync
    app_bud_loc_cause_action_flag_set(false);
#endif

    /*loc changed action and spk mute/unmte*/
    if (app_db.local_loc == BUD_LOC_IN_CASE)
    {
        app_lea_mgr_spk_mute_ctrl(true);
    }
    else
    {
        app_lea_mgr_spk_mute_ctrl(false);
    }

    switch (event)
    {
#if (F_APP_SENSOR_SUPPORT == 1)
    case EVENT_OUT_EAR:
        {
            action = app_lea_mgr_out_ear_action(from_remote, cause_action);
        }
        break;

    case EVENT_IN_EAR:
        {
            if ((!from_remote) && app_sensor_is_play_in_ear_tone())
            {
                if (app_cfg_const.play_in_ear_tone_regardless_of_phone_connection ||
                    (app_link_get_le_link_num() > 0))
                {
                    app_audio_tone_type_play(TONE_IN_EAR_DETECTION, false, false);
                }
            }

            action = app_lea_mgr_in_ear_action(from_remote, cause_action);
        }
        break;
#endif

    case EVENT_OUT_CASE:
        {
            //NA
        }
        break;

    case EVENT_IN_CASE:
        {
            action = app_lea_mgr_audio_in_case_action();
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE3("app_lea_mgr_bud_loc_event_cback: event_type 0x%02X, action 0x%02X, from_remote 0x%02X",
                         event, action, from_remote);
        app_lea_mgr_action_when_bud_loc_changed(action);
    }
}

#if F_APP_CCP_SUPPORT
T_LEA_ASE_ENTRY *app_lea_mgr_get_active_ccp_up_ase_handle(void)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(
                                app_lea_ccp_get_active_conn_handle());
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    if (p_link)
    {
        p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_UP_DIRECT, p_link->conn_handle, NULL);

        if (p_ase_entry != NULL)
        {
            if (p_ase_entry->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
            {
                return p_ase_entry;
            }
        }
    }

    return NULL;
}

T_LEA_ASE_ENTRY *app_lea_mgr_get_active_ccp_down_ase_handle(void)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(
                                app_lea_ccp_get_active_conn_handle());
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    if (p_link)
    {
        p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CONN_DOWN_DIRECT, p_link->conn_handle, NULL);

        if (p_ase_entry != NULL)
        {
            if (p_ase_entry->audio_context == AUDIO_CONTEXT_CONVERSATIONAL)
            {
                return p_ase_entry;
            }
        }
    }

    return NULL;
}
#endif

#if F_APP_MCP_SUPPORT
T_LEA_ASE_ENTRY *app_lea_mgr_get_active_mcp_ase_handle(void)
{
    T_APP_LE_LINK *p_link = app_link_find_le_link_by_conn_handle(
                                app_lea_mcp_get_active_conn_handle());
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    if (p_link)
    {
        p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CONN, p_link->conn_handle, NULL);

        if (p_ase_entry != NULL)
        {
            if (p_ase_entry->audio_context == AUDIO_CONTEXT_MEDIA ||
                p_ase_entry->audio_context == AUDIO_CONTEXT_UNSPECIFIED)
            {
                return p_ase_entry;
            }
        }
    }

    return NULL;
}
#endif

static void app_lea_mgr_direct_cback(uint8_t cb_type, void *p_cb_data)
{
    T_BT_DIRECT_CB_DATA *p_data = (T_BT_DIRECT_CB_DATA *)p_cb_data;

    switch (cb_type)
    {
    case BT_DIRECT_MSG_ISO_DATA_IND:
        {
            T_SYNCCLK_LATCH_INFO_TypeDef *app_lea_mgr_syncclk_latch_info;
            bool handle_data_in_stack = false;

            if (p_data->p_bt_direct_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA &&
                p_data->p_bt_direct_iso->iso_sdu_len != 0)
            {
                app_lea_mgr_iso_data_status = p_data->p_bt_direct_iso->pkt_status_flag;
            }

            if (app_lea_mgr_iso_data_status != ISOCH_DATA_PKT_STATUS_VALID_DATA)
            {
                goto iso_data_cfm;
            }

            app_lea_mgr_syncclk_latch_info = synclk_drv_time_get(LEA_SYNC_CLK_REF);

            if (lea_mgr_gaming_streaming)
            {
                handle_data_in_stack = true;
            }

            APP_PRINT_TRACE6("app_lea_mgr_direct_cback: syncclk latch result \
net_id 0x%02X, conn_type 0x%02X role 0x%02X exp_sync_clock 0x%02X status 0x%02x stack %d",
                             app_lea_mgr_syncclk_latch_info->net_id,
                             app_lea_mgr_syncclk_latch_info->conn_type,
                             app_lea_mgr_syncclk_latch_info->role,
                             app_lea_mgr_syncclk_latch_info->exp_sync_clock,
                             p_data->p_bt_direct_iso->pkt_status_flag,
                             handle_data_in_stack);

            if (handle_data_in_stack)
            {
                if (app_lea_uca_get_stream_link()) //TODO: Need to modify condition when support CIS+BIS mix
                {
                    app_lea_uca_handle_iso_data(p_data);
                }
#if F_APP_TMAP_BMR_SUPPORT
                else
                {
                    app_lea_bca_handle_iso_data(p_data);
                }
#endif
            }
            else
            {
                uint16_t len;
                T_IO_MSG msg;
                T_BT_DIRECT_ISO_DATA_IND *p_ind = malloc(sizeof(T_BT_DIRECT_ISO_DATA_IND));

                if (p_ind == NULL)
                {
                    goto iso_data_cfm;
                }

                memcpy(p_ind, p_data->p_bt_direct_iso, sizeof(T_BT_DIRECT_ISO_DATA_IND));

                len = p_data->p_bt_direct_iso->iso_sdu_len + p_data->p_bt_direct_iso->offset;
                p_ind->p_buf = malloc(len);
                if (p_ind->p_buf == NULL)
                {
                    free(p_ind);
                    goto iso_data_cfm;
                }

                memcpy(p_ind->p_buf, p_data->p_bt_direct_iso->p_buf, len);

                msg.type = IO_MSG_TYPE_LEA_SNK;
                msg.subtype = IO_MSG_LEA_ISO_DATA_DOWNSTREAM;
                msg.u.buf = p_ind;
                if (app_io_msg_send(&msg) == false)
                {
                    free(p_ind->p_buf);
                    free(p_ind);
                    goto iso_data_cfm;
                }
            }

iso_data_cfm:
            gap_iso_data_cfm(p_data->p_bt_direct_iso->p_buf);
        }
        break;

    default:
        break;
    }
}

void app_lea_mgr_msg_handle(T_IO_MSG *io_msg)
{
    T_IO_MSG_LEA sub_type = (T_IO_MSG_LEA)io_msg->subtype;

    switch (sub_type)
    {
    case IO_MSG_LEA_ISO_DATA_DOWNSTREAM:
        {
            T_BT_DIRECT_CB_DATA p_data;

            p_data.p_bt_direct_iso = (T_BT_DIRECT_ISO_DATA_IND *)(io_msg->u.buf);
            if (app_lea_uca_get_stream_link()) //TODO: Need to modify condition when support CIS+BIS mix
            {
                app_lea_uca_handle_iso_data(&p_data);
            }
#if F_APP_TMAP_BMR_SUPPORT
            else
            {
                app_lea_bca_handle_iso_data(&p_data);
            }
#endif
            free(p_data.p_bt_direct_iso->p_buf);
            free(p_data.p_bt_direct_iso);
        }
        break;

    default:
        break;
    }
}

#if F_APP_ERWS_SUPPORT
uint16_t app_lea_mgr_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    uint16_t payload_len = 0;
    uint8_t *msg_ptr = NULL;
    bool skip = true;

    switch (msg_type)
    {
    case LEA_REMOTE_SYNC_SIRK:
        {
            payload_len = CSIS_SIRK_LEN;
            msg_ptr = (uint8_t *)app_cfg_nv.lea_sirk;
        }
        break;

    default:
        break;

    }

    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_LEA, payload_len, msg_ptr, skip, total);
}

static void app_lea_mgr_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                    T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("app_lea_mgr_parse_cback: msg_type 0x%02X, status 0x%02X", msg_type, status);
    switch (msg_type)
    {
    case LEA_REMOTE_MMI_SWITCH_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                T_MTC_BT_MODE bt_mode = *(T_MTC_BT_MODE *)buf;

                mtc_set_btmode(bt_mode);
                mtc_set_active_bt_mode(bt_mode);
#if F_APP_LISTENING_MODE_SUPPORT
                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    if (bt_mode != MULTI_PRO_BT_BIS)
                    {
                        app_listening_mode_call_status_update();
                    }
                }
#endif
                APP_PRINT_TRACE2("app_lea_mgr_parse_cback: le_audio_switch %d, %d",
                                 bt_mode, mtc_get_btmode());
            }
        }
        break;

#if F_APP_TMAP_BMR_SUPPORT
    case LEA_REMOTE_PA_INFO_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
                    app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
                {
                    app_lea_bca_bs_update_device_info((T_LEA_BRS_INFO *)buf);
                }
                else
                {
                    app_lea_bca_tgt(APP_BIS_BS_FIND_RECORD_ALIGN);
                }
                app_lea_bca_bs_handle_pa_info_sync((T_LEA_BRS_INFO *)buf);
            }
        }
        break;
#endif

    case LEA_REMOTE_SEC_SYNC_CIS_STREAMING:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                bool is_streaming = *(bool *)buf;

                // only primary bud would receive
                APP_PRINT_TRACE2("app_lea_mgr_parse_cback: LEA_REMOTE_SEC_SYNC_CIS_STREAMING, remote_cis_link_exists = 0x%02X -> 0x%02X",
                                 app_db.remote_cis_link_is_streaming, is_streaming);

                if (app_db.remote_cis_link_is_streaming != is_streaming)
                {
                    app_db.remote_cis_link_is_streaming = is_streaming;

                    if (app_db.remote_cis_link_is_streaming)
                    {
                        app_sniff_mode_b2b_disable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_LEA);

#if F_APP_GAMING_LE_AUDIO_24G_STREAM_FIRST
                        app_dongle_handle_stream_preempt(EVENT_2_4G_STREAM_START);
#endif
                    }
                    else if (!mtc_is_lea_cis_stream(NULL))
                    {
                        app_sniff_mode_b2b_enable(app_cfg_nv.bud_peer_addr, SNIFF_DISABLE_MASK_LEA);
                    }
                }
            }
        }
        break;

    case LEA_REMOTE_MEDIA_SUSPEND_SYNC:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_lea_mgr_update_media_suspend_by_out_ear(*(bool *)buf);
            }
        }
        break;

    case LEA_REMOTE_SYNC_SIRK:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                if (memcmp(app_cfg_nv.lea_sirk, buf, CSIS_SIRK_LEN))
                {
                    memcpy(app_cfg_nv.lea_sirk, buf, CSIS_SIRK_LEN);
                    app_lea_csis_update_sirk();
                    app_lea_adv_update();
                    app_cfg_store(app_cfg_nv.lea_sirk, CSIS_SIRK_LEN);
                }
            }
        }
        break;

    case LEA_REMOTE_SYNC_PRI_MIC_STATE:
        {
            if (status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                app_lea_uca_set_mic_mute_state(*(bool *)buf);
            }
        }
        break;

    default:
        break;
    }
}
#endif

T_MTC_RESULT app_lea_mgr_cis_mtc_if_handle(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_RESULT app_result = MTC_RESULT_SUCCESS;

    switch (msg)
    {
    case LCIS_FM_MTC_LEA_TRACK:
        {
            uint8_t i, result, audio_context = 0;
            T_LEA_ASE_ENTRY *p_ase_entry = NULL;
            T_APP_LE_LINK *p_link = NULL;

            for (i = 0; i < MAX_BLE_LINK_NUM; i++)
            {
                p_link = &app_db.le_link[i];

                if (p_link->used == true &&
                    p_link->lea_link_state == LEA_LINK_STREAMING)
                {
                    audio_context = AUDIO_CONTEXT_MEDIA;
                    p_ase_entry = app_lea_ascs_find_ase_entry(LEA_ASE_CONTEXT, p_link->conn_handle, &audio_context);

                    if ((p_ase_entry != NULL) &&
                        (p_ase_entry->track_handle != NULL))
                    {
                        audio_track_start(p_ase_entry->track_handle);
                        break;
                    }
                }
            }
        }
        break;

    case LCIS_FM_MTC_LEA_PACS_CHANGE:
        {
            uint8_t i, result = 0;
            T_LEA_ASE_ENTRY *p_ase_entry = NULL;
            T_APP_LE_LINK *p_link = NULL;

            for (i = 0; i < MAX_BLE_LINK_NUM; i++)
            {
                p_link = &app_db.le_link[i];

                if (p_link->used == true &&
                    p_link->lea_link_state >= LEA_LINK_CONNECTED &&
                    (p_link->conn_handle == app_lea_mcp_get_active_conn_handle()))
                {
                    bool enable = *((bool *)inbuf);
                    bool ret = false;

                    ret = app_lea_pacs_set_sink_available_contexts(p_link->conn_handle,
                                                                   app_lea_pacs_get_sink_available_contexts(), enable);
#if F_APP_CCP_SUPPORT
                    //hfp call switch to idle and sco disc will read ccp call, check it when mtc optimize
                    if (enable == true && ret == true)
                    {
                        ccp_client_read_char_value(p_link->conn_handle, 0, TBS_UUID_CHAR_CALL_STATE, p_link->gtbs);
                    }
#endif
                    break;
                }
            }
        }
        break;

    default:
        break;
    }
    return app_result;
}

void app_lea_mgr_bas_batt_level_report(uint16_t conn_handle)
{
    T_APP_LE_LINK *p_link;

    p_link = app_link_find_le_link_by_conn_handle(conn_handle);

    // current lea phone not enable bas CCCD, remove p_link->bas_report_batt check condition
    if (p_link != NULL)
    {
        bas_battery_level_value_notify(p_link->conn_id, lea_bas_id, app_db.local_batt_level);
    }
}

static void app_lea_mgr_send_ref(T_BT_LE_ISO_SYNC_REF_AP_INFO *info, uint32_t guard_time_us)
{
    uint8_t ref_info_len = sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO) + sizeof(guard_time_us);
    uint8_t cmd_len = ref_info_len + 4; // First 4 bytes parameters are session id
    uint8_t *cmd_buf = calloc(1, cmd_len + 4); // 4 bytes dsp command header: id, length

    if (cmd_buf != NULL)
    {
        cmd_buf[0] = AUDIO_PROBE_UPLINK_SYNCREF;
        cmd_buf[2] = cmd_len >> 2;
        cmd_buf[8] = guard_time_us;
        cmd_buf[9] = guard_time_us >> 8;
        cmd_buf[10] = guard_time_us >> 16;
        cmd_buf[11] = guard_time_us >> 24;
        memcpy(&cmd_buf[12], info, sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO));
        audio_probe_dsp_send(cmd_buf, cmd_len + 4);

        free(cmd_buf);
    }
}

static void app_lea_mgr_probe_cback(T_AUDIO_PROBE_EVENT event, void *buf)
{
    uint32_t *event_buff = (uint32_t *)buf;
    T_BT_LE_ISO_SYNC_REF_AP_INFO p_info;
    T_LEA_ASE_ENTRY *p_ase_entry = NULL;

    if (event != PROBE_SYNC_REF_REQUEST)
    {
        return;
    }

    if (app_lea_uca_get_diff_path())
    {
        p_ase_entry = app_lea_ascs_find_ase_entry_non_conn(LEA_ASE_UP_DIRECT, NULL, NULL);
    }
    else
    {
        p_ase_entry = app_lea_ascs_find_ase_entry_non_conn(LEA_ASE_DEVICE_INOUT, NULL, NULL);
    }

    if (p_ase_entry == NULL)
    {
        APP_PRINT_TRACE0("app_lea_mgr_probe_cback: error ase empty");
        return;
    }

    p_info.dir = 1;
    p_info.conn_handle = p_ase_entry->cis_conn_handle;
    bt_get_le_iso_sync_ref_ap_info(&p_info);
    app_lea_mgr_send_ref(&p_info, LEA_REF_GUARD_TIME_US);
}

void app_lea_mgr_init(void)
{
    T_BLE_AUDIO_PARAMS ble_audio_param;

    ble_audio_param.evt_queue_handle = audio_evt_queue_handle;
    ble_audio_param.io_queue_handle = audio_io_queue_handle;
#if F_APP_EATT_SUPPORT
    ble_audio_param.bt_gatt_client_init = GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT |
                                          GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT |
                                          GATT_CLIENT_DISCOV_MODE_GATT_SVC;
    gatt_client_cfg_client_supported_feature(GATT_SVC_CLIENT_SUPPORTED_FEATURES_EATT_BEARER_BIT);
#else
    ble_audio_param.bt_gatt_client_init = GATT_CLIENT_DISCOV_MODE_REG_SVC_BIT |
                                          GATT_CLIENT_DISCOV_MODE_USE_EXT_CLIENT;
#endif
    ble_audio_param.acl_link_num = le_get_max_link_num();
    ble_audio_param.io_event_type = IO_MSG_TYPE_LE_AUDIO;

    gap_register_direct_cb(app_lea_mgr_direct_cback);
#if F_APP_ERWS_SUPPORT
    app_relay_cback_register(app_lea_mgr_relay_cback, app_lea_mgr_parse_cback,
                             APP_MODULE_TYPE_LEA, LEA_REMOTE_MSG_TOTAL);
#endif
    app_ipc_subscribe(BUD_LOCATION_IPC_TOPIC, app_lea_mgr_bud_loc_event_cback);

    ble_audio_init(&ble_audio_param);
    app_lea_adv_init();
    mtc_if_lcis_reg(app_lea_mgr_cis_mtc_if_handle);
    mtc_if_lbis_reg(NULL);//just register mtc handle bis event
    //LEA are going to combine uca and bca handle
    mtc_init();
    audio_probe_dsp_cback_register(app_lea_mgr_probe_cback);
}
#endif
