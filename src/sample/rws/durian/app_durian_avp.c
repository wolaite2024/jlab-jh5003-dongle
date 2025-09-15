/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if F_APP_DURIAN_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "os_mem.h"
#include "trace.h"
#include "vad.h"
#include "gap_chann.h"
#include "gap_lib_common.h"
#include "bt_att.h"
#include "app_durian_att.h"
#include "app_durian.h"
#include "app_main.h"
#include "app_roleswap_control.h"
#include "app_cfg.h"
#include "app_dsp_cfg.h"
#include "app_relay.h"
#include "app_hfp.h"
#include "app_a2dp.h"
#include "app_multilink.h"
#include "app_timer.h"
#include "app_audio_policy.h"
#include "app_report.h"

#if (F_APP_SENSOR_MEMS_SUPPORT == 1)
#include "app_sensor_mems.h"
#endif

static T_AUDIO_TRACK_HANDLE avp_opus_handle = NULL;
static bool key_voice_reg_enable = false;
static uint8_t compactness_bd_addr[6] = {0};
uint8_t siri_seq = 0x00;

#define AVP_ATT_CONNECT_MS                1000
#define AVP_COMPACTNESS_MS                4000
#define AVP_VOICE_CALL_VP_DELAY_MS        200

typedef enum
{
    APP_TIMER_ATT_CONNECT,
    APP_TIMER_AVP_COMPACTNESS,
    APP_TIMER_VOICE_CALL_VP_DELAY,
} T_APP_AVP_TIMER;

static uint8_t app_durian_avp_timer_id = 0;

static uint8_t timer_idx_att_connnect = 0;
static uint8_t timer_idx_avp_compactness = 0;
static uint8_t timer_idx_voice_call_vp_delay = 0;

static bool app_durian_avp_track_async_read(T_AUDIO_TRACK_HANDLE   handle,
                                            uint32_t              *timestamp,
                                            uint16_t              *seq_num,
                                            T_AUDIO_STREAM_STATUS *status,
                                            uint8_t               *frame_num,
                                            void                  *buf,
                                            uint16_t               required_len,
                                            uint16_t              *actual_len)
{
    uint8_t app_idx = app_hfp_get_active_idx();

    if ((app_db.br_link[app_idx].connected_profile & AVP_PROFILE_MASK) &&
        (app_db.br_link[app_idx].audio_opus_status == AVP_VIOCE_RECOGNITION_ENCODE_START))
    {
        uint8_t sniffing_state = app_db.br_link[app_idx].bt_sniffing_state;

        if ((sniffing_state == APP_BT_SNIFFING_STATE_IDLE) ||
            (sniffing_state == APP_BT_SNIFFING_STATE_SNIFFING) ||
            (sniffing_state == APP_BT_SNIFFING_STATE_READY))
        {
            app_durian_avp_voice_recognition_data_send(app_db.br_link[app_idx].bd_addr, buf, required_len);
            *actual_len = required_len;
            return true;
        }
    }

    return false;
}

void app_durian_avp_voice_start(uint8_t app_idx)
{
    if (app_db.br_link[app_idx].connected_profile & AVP_PROFILE_MASK)
    {
        app_start_timer(&timer_idx_voice_call_vp_delay, "voice_call_vp_delay",
                        app_durian_avp_timer_id, APP_TIMER_VOICE_CALL_VP_DELAY, 0, false,
                        AVP_VOICE_CALL_VP_DELAY_MS);
        app_durian_avp_voice_recognition_enable_req(app_db.br_link[app_idx].bd_addr);
        key_voice_reg_enable = true;
    }
}

void app_durian_avp_voice_recognition_check(void)
{
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        audio_track_release(avp_opus_handle);
        avp_opus_handle = NULL;
    }
    else if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
    {
        for (uint8_t app_idx = 0; app_idx < MAX_BR_LINK_NUM; app_idx++)
        {
            if (app_link_check_b2s_link_by_id(app_idx))
            {
                if ((app_db.br_link[app_idx].audio_opus_status != AVP_VIOCE_RECOGNITION_IDLE) &&
                    ((app_db.br_link[app_idx].connected_profile & AVP_PROFILE_MASK) != 0))
                {
                    T_AUDIO_FORMAT_INFO opus_info;

                    durian_avp_opus_info(&opus_info, AUDIO_FORMAT_TYPE_OPUS);

                    avp_opus_handle = audio_track_create(AUDIO_STREAM_TYPE_RECORD,
                                                         AUDIO_STREAM_MODE_NORMAL,
                                                         AUDIO_STREAM_USAGE_LOCAL,
                                                         opus_info,
                                                         0,
                                                         app_dsp_cfg_vol.record_volume_default,
                                                         AUDIO_DEVICE_IN_MIC,
                                                         NULL,
                                                         app_durian_avp_track_async_read);

                    if (app_db.br_link[app_idx].audio_opus_status == AVP_VIOCE_RECOGNITION_ENCODE_START)
                    {
                        if (app_cfg_const.enable_vad == 1)
                        {
                            vad_disable();
                        }

#if (F_APP_LISTENING_MODE_SUPPORT == 1)
                        app_durain_anc_siri_start_check();
#endif
                        audio_track_start(avp_opus_handle);
                    }
                }
            }
        }
    }
}

T_AVP_VIOCE_RECOGNITION_STATUS app_durian_avp_get_opus_status(void)
{
    uint8_t app_idx = app_a2dp_get_active_idx();

    return (T_AVP_VIOCE_RECOGNITION_STATUS)app_db.br_link[app_idx].audio_opus_status;
}

static bool app_durian_avp_compact_test_report(uint8_t *bd_addr, bool right_ear_result,
                                               bool left_ear_result)
{
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);

    if (p_link != NULL)
    {
        bt_avp_compactness_test_report(bd_addr, right_ear_result, left_ear_result);
    }

    return false;
}

bool app_durian_avp_voice_recognition_enable_req(uint8_t *bd_addr)
{
    uint16_t conn_handle;
    uint16_t cid;
    uint8_t cid_num;
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t buf[5] = {0x01, 0x00, 0x01, 0x01, 0x02};

        gap_chann_get_handle(bd_addr, 0x10/*BTIF_REMOTE_ADDR_CLASSIC*/, &conn_handle);
        gap_chann_get_cid(conn_handle, 1, &cid, &cid_num);

        return avp_att_service_send_notify3(conn_handle, cid, buf, sizeof(buf));
    }

    return false;
}

bool app_durian_avp_voice_recognition_disable_req(uint8_t *bd_addr)
{
    uint16_t conn_handle;
    uint16_t cid;
    uint8_t cid_num;
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t buf[4] = {0x01, 0x00, 0x05, 0x00};

        gap_chann_get_handle(bd_addr, 0x10/*BTIF_REMOTE_ADDR_CLASSIC*/, &conn_handle);
        gap_chann_get_cid(conn_handle, 1, &cid, &cid_num);

        return avp_att_service_send_notify3(conn_handle, cid, buf, sizeof(buf));
    }

    return false;
}


bool app_durian_avp_voice_recognition_encode_start(uint8_t *bd_addr)
{
    uint16_t conn_handle;
    uint16_t cid;
    uint8_t cid_num;
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t buf[2] = {0x01, 0x00};
        gap_chann_get_handle(bd_addr, 0x10/*BTIF_REMOTE_ADDR_CLASSIC*/, &conn_handle);
        gap_chann_get_cid(conn_handle, 1, &cid, &cid_num);

        return avp_att_service_send_notify2(conn_handle, cid, buf, sizeof(buf));
    }

    return false;
}

bool app_durian_avp_voice_recognition_encode_stop(uint8_t *bd_addr)
{
    uint16_t conn_handle;
    uint16_t cid;
    uint8_t cid_num;
    T_APP_BR_LINK *p_link;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        uint8_t buf[4] = {0x01, 0x00, 0x03, 0x00};
        gap_chann_get_handle(bd_addr, 0x10/*BTIF_REMOTE_ADDR_CLASSIC*/, &conn_handle);
        gap_chann_get_cid(conn_handle, 1, &cid, &cid_num);

        return avp_att_service_send_notify3(conn_handle, cid, buf, sizeof(buf));
    }

    return false;
}

bool app_durian_avp_voice_recognition_data_send(uint8_t *bd_addr, uint8_t *p_data,
                                                uint16_t data_len)
{
    T_APP_BR_LINK *p_link;
    uint16_t conn_handle;
    uint16_t cid;
    uint8_t cid_num;
    bool ret = false;
    uint8_t *p_buf;
    uint8_t *p;

    p_link = app_link_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_buf = os_mem_alloc2(data_len + 10);
        if (p_buf)
        {
            p = p_buf;
            LE_UINT8_TO_STREAM(p, 0x01);
            LE_UINT8_TO_STREAM(p, 0x00);
            LE_UINT8_TO_STREAM(p, siri_seq);
            LE_UINT16_TO_STREAM(p, data_len + 5);
            LE_UINT16_TO_STREAM(p, 0x0000);
            LE_UINT8_TO_STREAM(p, siri_seq);
            LE_UINT8_TO_STREAM(p, 0x00);
            LE_UINT8_TO_STREAM(p, (uint8_t)data_len);
            memcpy(p, p_data, data_len);

            gap_chann_get_handle(bd_addr, 0x10/*BTIF_REMOTE_ADDR_CLASSIC*/, &conn_handle);
            gap_chann_get_cid(conn_handle, 1, &cid, &cid_num);
            ret = avp_att_service_send_notify1(conn_handle,  cid, p_buf, data_len + 10);

            os_mem_free(p_buf);

            if (siri_seq == 0xff)
            {
                siri_seq = 0;
            }
            else
            {
                siri_seq++;
            }
        }
    }

    return ret;
}

void app_durian_avp_compactness_state_machine(bool force_pass)
{
    bool right_compactness, left_compactness;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }
    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED &&
        durian_db.remote_compactness.compactness_result == AVP_EARFIT_RESULT_UNKOWN &&
        !force_pass)
    {
        APP_PRINT_TRACE0("app_durian_avp_compactness_state_machine: wait for sec-bud testing result");
        return;
    }

    if (app_cfg_const.bud_side == DEVICE_ROLE_LEFT)
    {
        left_compactness = (durian_db.local_compactness.compactness_result == AVP_EARFIT_RESULT_GOOD) ?
                           true :
                           false;
        right_compactness = (durian_db.remote_compactness.compactness_result == AVP_EARFIT_RESULT_GOOD) ?
                            true : false;
    }
    else
    {
        right_compactness = (durian_db.local_compactness.compactness_result == AVP_EARFIT_RESULT_GOOD) ?
                            true :
                            false;
        left_compactness = (durian_db.remote_compactness.compactness_result == AVP_EARFIT_RESULT_GOOD) ?
                           true :
                           false;
    }
    if (force_pass)
    {
        left_compactness = true;
        right_compactness = true;
        if (app_db.remote_session_state != REMOTE_SESSION_STATE_CONNECTED)
        {
            if (app_cfg_const.bud_side == DEVICE_ROLE_LEFT)
            {
                right_compactness = false;
            }
            else
            {
                left_compactness = false;
            }
        }
    }
    app_durian_avp_compact_test_report(compactness_bd_addr, right_compactness,
                                       left_compactness);

    //CONTROL_ANC_APT
    if (!app_cfg_const.disable_fake_earfit_verification)
    {
        return;
    }
    //app_avp_compactness_anc_apt_ctrl(false);
}

#if (F_APP_SENSOR_MEMS_SUPPORT == 1)
void app_durian_avp_atti_start(uint8_t app_idx)
{
    if (app_db.br_link[app_idx].mems_is_start)
    {
        return;
    }

#if (MEMS_EVB == 0)
    bool is_left = (app_cfg_const.bud_side == DEVICE_ROLE_LEFT) ? true : false;
#if TARGET_RTL8753EFE_VS_AVP&DURIAN_FOUR
    durian_atti_pos_set(PROD_ID_AIRPODS3, (gsensor_pos_e)app_cfg_const.mems_pos, is_left);
#endif
#if (TARGET_RTL8763ESE_AVP&DURIAN_PRO)|(TARGET_RTL8763ESE_AVP&DURIAN_PRO2)
    durian_atti_pos_set(PROD_ID_AIRPODS_PRO, (gsensor_pos_e)app_cfg_const.mems_pos, is_left);
#endif
#if TARGET_RTL8763ESE_AVP&DURIAN_BEATS_FIT
    app_cfg_const.mems_pos = GS_Z_UP_X_LEFT;
    durian_atti_pos_set(PROD_ID_BEATS_FIT_PRO, (gsensor_pos_e)app_cfg_const.mems_pos, is_left);
#endif
#endif

}
#endif

static void app_durian_avp_bt_cback(T_BT_AVP_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_AVP_EVENT_PARAM *param = event_buf;
    bool handle = true;
    uint8_t active_hf_idx = app_hfp_get_active_idx();

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        APP_PRINT_TRACE0("app_durian_avp_bt_cback: role is sec");
        return;
    }

    switch (event_type)
    {
    case BT_AVP_EVENT_CONTROL_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;
            uint8_t avp_is_conn = 0;

            p_link = app_link_find_br_link(param->control_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                app_bt_policy_msg_prof_conn(p_link->bd_addr, AVP_PROFILE_MASK);
                app_start_timer(&timer_idx_att_connnect, "att_connect",
                                app_durian_avp_timer_id, APP_TIMER_ATT_CONNECT, 0, false,
                                AVP_ATT_CONNECT_MS);
                durian_db.click_speed_rec = false;
                durian_db.long_press_time_rec = false;
                durian_avp_id_set();
                avp_is_conn = 1;
                app_durian_sync_report_event_broadcast(EVENT_AVP_PROFILE_CONNECT, &avp_is_conn, 1);
            }
        }
        break;

    case BT_AVP_EVENT_SET_NAME:
        {
            app_durian_cfg_set_name(&param->set_name);
            app_durian_adp_batt_report();
            app_durian_sync_report_event_broadcast(EVENT_AVP_NAME_SET, app_cfg_nv.device_name_legacy, 40);
        }
        break;

#if DURIAN_TWO
    case BT_AVP_EVENT_CONTROL_SETTINGS: //pod1&2
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->control_settings.bd_addr);
            if (p_link != NULL)
            {
                app_durian_key_control_set(param->control_settings.left_ear_control,
                                           param->control_settings.right_ear_control);


                uint8_t ear_control[2];
                ear_control[0] = param->control_settings.left_ear_control;
                ear_control[1] = param->control_settings.right_ear_control;
                app_durian_sync_report_event_broadcast(EVENT_AVP_CONTROL_SET, ear_control, 2);
            }
        }
        break;
#endif

#if DURIAN_PRO|DURIAN_BEATS_FIT|DURIAN_PRO2
    case BT_AVP_EVENT_CLICK_SETTINGS: //pro
        {
            uint8_t left_right_action;
            APP_PRINT_INFO2("app_avp_bt_cback: left_ear_longpress_control 0x%2x,right_ear_longpress_control 0x%2x",
                            param->click_settings.left_ear_longpress_control,
                            param->click_settings.right_ear_longpress_control);

            left_right_action = param->click_settings.left_ear_longpress_control << 4;
            left_right_action |= param->click_settings.right_ear_longpress_control;
            /*ref T_BT_AVP_LONGPRESS_CONTROL for action def; If action is anc switch:
              0x03: 1&3
              0x05: 2&3
              0x06: 1&2
              0x07: 1&2&3
            1:anc
            2.parent
            3.close
            param->bt_avp_click_settings.right_ear_control_anc;
            param->bt_avp_click_settings.left_ear_control_anc;
            param->bt_avp_click_settings.setting;*/
            app_durian_key_click_set(left_right_action);
            app_durian_sync_report_event_broadcast(EVENT_AVP_CLICK_SET, &left_right_action, 1);
        }
        break;
#endif

#if (F_APP_LISTENING_MODE_SUPPORT == 1)
    case BT_AVP_EVENT_ANC_APT_CYCLE:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->cycle_settings.bd_addr);
            if (p_link != NULL)
            {
                uint8_t cycle_set = param->cycle_settings.setting;
                app_durian_anc_cylce_set(cycle_set);
                app_durian_sync_report_event_broadcast(EVENT_AVP_ANC_APT_CYCLE, &cycle_set, 1);
            }
        }
        break;

#if DURIAN_PRO2
    case BT_AVP_EVENT_AUTO_APT_SETTINGS:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->auto_apt_settings.bd_addr);
            if (p_link != NULL)
            {
                uint8_t auto_apt_en = (uint8_t)param->auto_apt_settings.enable;
                app_durian_anc_auto_apt_cfg(auto_apt_en);
            }
        }
        break;

    case BT_AVP_EVENT_VOLUME_CONTROL_SETTINGS:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->volume_control_settings.bd_addr);
            if (p_link != NULL)
            {
                uint8_t vol_control_en = (uint8_t)param->volume_control_settings.is_enable;
                app_durian_audio_vol_control(vol_control_en);
            }
        }
        break;
#endif

    case BT_AVP_EVENT_ANC_SETTINGS:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->anc_settings.bd_addr);
            if (p_link != NULL)
            {
                app_durian_anc_set(param->anc_settings.setting);
            }
        }
        break;

    case BT_AVP_EVENT_ONE_BUD_ANC:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->one_bud_anc.bd_addr);
            if (p_link != NULL)
            {
                APP_PRINT_INFO1("app_avp_bt_cback: one bud anc is open %d", param->one_bud_anc.is_open);
                durian_db.anc_one_bud_enabled = param->one_bud_anc.is_open;
                app_relay_async_single(APP_MODULE_TYPE_AVP, SYNC_EVENT_DURIAN_ANC_ONE_BUD_ENABLE);
            }
        }
        break;
#endif

    case BT_AVP_EVENT_IN_EAR_DETECTION:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->in_ear_detection.bd_addr);

            if (p_link != NULL)
            {


                uint8_t ear_detect_en = (uint8_t)param->in_ear_detection.open;
                app_durian_loc_in_ear_detect(ear_detect_en);
                app_durian_sync_report_event_broadcast(EVENT_EAR_DETECTION_STATUS, &ear_detect_en, 1);
            }
        }
        break;

#if (F_APP_ERWS_SUPPORT == 1)
    case BT_AVP_EVENT_MIC_SETTINGS:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->mic_settings.bd_addr);
            if (p_link != NULL)
            {
                APP_PRINT_INFO1("app_avp_bt_cback: avp_mic_select %d", param->mic_settings.setting);
                durian_db.mic_setting = param->mic_settings.setting;

                app_relay_async_single(APP_MODULE_TYPE_AVP, SYNC_EVENT_DURIAN_MIC_SETTING);
                app_roleswap_ctrl_check(APP_ROLESWAP_CTRL_EVENT_DURIAN_MIC_CHANGED);
            }
        }
        break;
#endif

#if DURIAN_AIRMAX
    case BT_AVP_EVENT_DIGITAL_CROWN_SETTINGS:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->digital_crown_settings.bd_addr);
            if (p_link != NULL)
            {
                APP_PRINT_INFO1("app_avp_bt_cback: is_clockwise %d", param->digital_crown_settings.is_clockwise);
                app_cfg_nv.is_clockwise = param->digital_crown_settings.is_clockwise;
            }
        }
        break;
#endif

    case BT_AVP_EVENT_COMPACTNESS_TEST:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->compactness_test.bd_addr);
            if ((p_link != NULL) && (timer_idx_avp_compactness == 0))
            {
                memcpy(compactness_bd_addr, p_link->bd_addr, 6);
                if (app_cfg_const.disable_fake_earfit_verification)
                {
                    //Wait next version update, replace below flow
                    app_start_timer(&timer_idx_avp_compactness, "avp_compactness",
                                    app_durian_avp_timer_id, APP_TIMER_AVP_COMPACTNESS, 0, false,
                                    AVP_COMPACTNESS_MS);
                }
                else
                {
                    app_start_timer(&timer_idx_avp_compactness, "avp_compactness",
                                    app_durian_avp_timer_id, APP_TIMER_AVP_COMPACTNESS, 0, false,
                                    AVP_COMPACTNESS_MS);
                }

            }
        }
        break;

    case BT_AVP_EVENT_CLICK_SPEED:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->click_speed_settings.bd_addr);
            if (p_link != NULL)
            {
                durian_db.click_speed = param->click_speed_settings.speed;
                app_cfg_const.key_multi_click_interval = app_durian_key_multiclick_get(durian_db.click_speed);
                durian_db.click_speed_rec = true;
                app_relay_async_single(APP_MODULE_TYPE_AVP, SYNC_EVENT_DURIAN_KEY_CLICK_SPEED);
            }
        }
        break;

    case BT_AVP_EVENT_LONG_RESS_TIME:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->long_press_time_settings.bd_addr);
            if (p_link != NULL)
            {
                durian_db.long_press_time = param->long_press_time_settings.time;
                app_cfg_const.key_long_press_interval = app_durian_key_long_press_get(durian_db.long_press_time);

                durian_db.long_press_time_rec = true;
                app_relay_async_single(APP_MODULE_TYPE_AVP, SYNC_EVENT_DURIAN_KEY_LONG_PRESS_TIME);
            }
        }
        break;

#if F_APP_TWO_GAIN_TABLE
    case BT_AVP_EVENT_AUDIO_SHARING:
        {
            T_APP_BR_LINK *p_link = app_link_find_br_link(param->audio_sharing.bd_addr);

            if (p_link != NULL)
            {
                p_link->audio_sharing = param->audio_sharing.is_enable;
                app_durian_sync_amplify_gain(p_link->bd_addr);
            }
        }
        break;
#endif

#if (F_APP_SENSOR_MEMS_SUPPORT == 1)
    case BT_AVP_EVENT_SPATIAL_AUDIO:
        {
            if (app_cfg_const.mems_support)
            {
                T_APP_BR_LINK *p_link = app_link_find_br_link(param->spatial_audio.bd_addr);
                if (p_link != NULL)
                {
                    uint8_t attitude_ret = durian_atti_hdl(param->spatial_audio.bd_addr, param->spatial_audio.data,
                                                           param->spatial_audio.len);

                    APP_PRINT_INFO1("app_avp_bt_cback: attitude_ret %d", attitude_ret);

                    if (attitude_ret == 1)
                    {
                        app_sensor_mems_spatial_start(p_link->id);
                    }
                    else if (attitude_ret == 2)
                    {
                        app_sensor_mems_spatial_stop(p_link->id);
                    }
                }
            }
        }
        break;
#endif

    case BT_AVP_EVENT_VERSION_SYNC:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->version_sync.bd_addr);
            if (p_link != NULL)
            {
                T_VER_SYNC ver_sync = {0};
                ver_sync.bd_addr = param->version_sync.bd_addr;
                ver_sync.version = p_link->remote_device_vendor_version;
                durian_avp_version_sync(&ver_sync);
            }

            app_durian_adp_batt_report();
        }
        break;

    case BT_AVP_EVENT_AUDIO_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->audio_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                app_bt_policy_msg_prof_conn(p_link->bd_addr, GATT_PROFILE_MASK);
                p_link->audio_opus_status = AVP_VIOCE_RECOGNITION_CREATE;

                if (avp_opus_handle == NULL)
                {
                    T_AUDIO_FORMAT_INFO opus_info;

                    durian_avp_opus_info(&opus_info, AUDIO_FORMAT_TYPE_OPUS);
                    avp_opus_handle = audio_track_create(AUDIO_STREAM_TYPE_RECORD,
                                                         AUDIO_STREAM_MODE_NORMAL,
                                                         AUDIO_STREAM_USAGE_LOCAL,
                                                         opus_info,
                                                         0,
                                                         app_dsp_cfg_vol.record_volume_default,
                                                         AUDIO_DEVICE_IN_MIC,
                                                         NULL,
                                                         app_durian_avp_track_async_read);
                }
            }
        }
        break;

    case BT_AVP_EVENT_VOICE_RECOGNITION_START:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->voice_recognition_start.bd_addr);
            if (p_link != NULL)
            {
                if (p_link->call_status == APP_CALL_IDLE)
                {
                    p_link->call_status = APP_VOICE_ACTIVATION_ONGOING;
                }

                app_hfp_update_call_status();

                if (p_link->id != active_hf_idx)
                {
                    break;
                }

                app_multi_a2dp_active_link_set(app_db.br_link[p_link->id].bd_addr);
                app_multi_pause_inactive_a2dp_link_stream(p_link->id, false);
                app_bond_set_priority(app_db.br_link[p_link->id].bd_addr);

                if (key_voice_reg_enable)
                {
                    p_link->audio_opus_status = AVP_VIOCE_RECOGNITION_ENCODE_START;

#if (F_APP_LISTENING_MODE_SUPPORT == 1)
                    app_durain_anc_siri_start_check();
#endif

                    audio_track_start(avp_opus_handle);
                    app_durian_avp_voice_recognition_encode_start(p_link->bd_addr);
                    key_voice_reg_enable = false;
                }
            }
        }
        break;

    case BT_AVP_EVENT_VOICE_RECOGNITION_ENCODE_START:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->voice_recognition_encode_start.bd_addr);
            if (p_link != NULL)
            {
                if (p_link->call_status == APP_CALL_IDLE)
                {
                    p_link->call_status = APP_VOICE_ACTIVATION_ONGOING;
                }

                app_hfp_update_call_status();

                if (p_link->id != active_hf_idx)
                {
                    break;
                }

                app_multi_a2dp_active_link_set(app_db.br_link[p_link->id].bd_addr);
                app_multi_pause_inactive_a2dp_link_stream(p_link->id, false);
                app_bond_set_priority(app_db.br_link[p_link->id].bd_addr);

                if (app_cfg_const.enable_vad == 1 && app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
                {
                    vad_disable();
                }
                p_link->audio_opus_status = AVP_VIOCE_RECOGNITION_ENCODE_START;

#if (F_APP_LISTENING_MODE_SUPPORT == 1)
                app_durain_anc_siri_start_check();
#endif

                audio_track_start(avp_opus_handle);
            }
        }
        break;

    case BT_AVP_EVENT_VOICE_RECOGNITION_ENCODE_STOP:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->voice_recognition_encode_stop.bd_addr);
            if (p_link != NULL)
            {
                if (p_link->call_status == APP_VOICE_ACTIVATION_ONGOING)
                {
                    p_link->call_status = APP_CALL_IDLE;
                }

                app_hfp_update_call_status();

                if (p_link->id != active_hf_idx)
                {
                    break;
                }

                audio_track_stop(avp_opus_handle);
                p_link->audio_opus_status = AVP_VIOCE_RECOGNITION_ENCODE_STOP;
                app_durian_avp_voice_recognition_encode_stop(p_link->bd_addr);
            }

            if (app_cfg_const.enable_vad == 1 && app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                vad_enable();
            }
        }
        break;

    case BT_AVP_EVENT_VOICE_RECOGNITION_STOP:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_link_find_br_link(param->voice_recognition_stop.bd_addr);
            if (p_link != NULL)
            {
                if (p_link->call_status == APP_VOICE_ACTIVATION_ONGOING)
                {
                    p_link->call_status = APP_CALL_IDLE;
                    app_hfp_update_call_status();
                }

                if (p_link->id != active_hf_idx)
                {
                    break;
                }

                if (p_link->audio_opus_status == AVP_VIOCE_RECOGNITION_ENCODE_START)
                {
                    audio_track_stop(avp_opus_handle);
                    app_durian_avp_voice_recognition_encode_stop(p_link->bd_addr);
                }

                p_link->audio_opus_status = AVP_VIOCE_RECOGNITION_STOP;

#if (F_APP_LISTENING_MODE_SUPPORT == 1)
                app_durain_anc_siri_stop_check();
#endif

            }
        }
        break;

    default:
        break;
    }

    APP_PRINT_INFO1("app_durian_avp_handle_event: event_type 0x%04x", event_type);
}

static void app_durian_avp_bt_cback(T_BT_AVP_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_AVP_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        APP_PRINT_TRACE0("app_durian_avp_bt_cback: role is sec");
        return;
    }

    switch (event_type)
    {
    case BT_AVP_EVENT_CONN_CMPL:
        {
            uint8_t avp_is_conn = 0;

            p_link = app_link_find_br_link(param->conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                app_bt_policy_msg_prof_conn(p_link->bd_addr, AVP_PROFILE_MASK);
                app_start_timer(&timer_idx_att_connnect, "att_connect",
                                app_durian_avp_timer_id, APP_TIMER_ATT_CONNECT, 0, false,
                                AVP_ATT_CONNECT_MS);
                durian_db.click_speed_rec = false;
                durian_db.long_press_time_rec = false;
                durian_avp_id_set();
                avp_is_conn = 1;
                app_durian_sync_report_event_broadcast(EVENT_AVP_PROFILE_CONNECT, &avp_is_conn, 1);
            }
        }
        break;

    case BT_AVP_EVENT_DISCONN_CMPL:
        {
            p_link = app_link_find_br_link(param->disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                uint8_t avp_is_conn = 0;
                app_bt_policy_msg_prof_disconn(p_link->bd_addr, AVP_PROFILE_MASK, 0);
                app_durian_sync_report_event_broadcast(EVENT_AVP_PROFILE_CONNECT, &avp_is_conn, 1);
            }
        }
        break;

    case BT_AVP_EVENT_DATA_IND:
        {
            p_link = app_link_find_br_link(param->data_ind.bd_addr);
            if (p_link != NULL)
            {
                T_APP_DURIAN_AVP_EVENT_PARAM event_param;
                uint8_t   *data = param->data_ind.data;
                uint16_t   len = param->data_ind.len;

                /*sync packet*/
                if ((data[0] == 0x00) && (data[1] == 0x00) &&
                    (data[2] == 0x04) && (data[3] == 0x00))
                {
                    uint8_t durian_pro[18] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00,
                                              0x02, 0x00, 0x05, 0x00, 0x49, 0x4E, 0x05, 0x00,
                                              0xA5, 0x4F
                                             };

                    bt_avp_data_send(p_link->bd_addr, durian_pro, sizeof(durian_pro), false);

                    memcpy(event_param.version_sync.bd_addr, p_link->bd_addr, 6);
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_VERSION_SYNC, &event_param, sizeof(event_param));
                }
                /*iphone set name*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x1A) && (data[5] == 0x00) &&
                         (data[6] == 0x01))
                {
                    event_param.set_name.len = data[7] + (data[8] << 8);
                    event_param.set_name.data = &data[9];
                    memcpy(event_param.set_name.bd_addr, p_link->bd_addr, 6);
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_SET_NAME, &event_param, sizeof(event_param));
                }
                /*pod1&2 click settings*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x0C))
                {
                    memcpy(event_param.control_settings.bd_addr, p_link->bd_addr, 6);
                    event_param.control_settings.right_ear_control = (T_APP_DURIAN_AVP_CONTROL)data[7];
                    event_param.control_settings.left_ear_control = (T_APP_DURIAN_AVP_CONTROL)data[8];
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_CONTROL_SETTINGS, &event_param,
                                                sizeof(event_param));
                }
                /*in_ear detection*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x0A))
                {
                    memcpy(event_param.in_ear_detection.bd_addr, p_link->bd_addr, 6);

                    if (data[7] == 0x01)
                    {
                        event_param.in_ear_detection.open = false;
                    }
                    else if (data[7] == 0x02)
                    {
                        event_param.in_ear_detection.open = true;
                    }

                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_IN_EAR_DETECTION, &event_param,
                                                sizeof(event_param));
                }
                /*mic settings*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x01))
                {
                    memcpy(event_param.mic_settings.bd_addr, p_link->bd_addr, 6);
                    event_param.mic_settings.setting = (T_APP_DURIAN_AVP_MIC)data[7];
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_MIC_SETTINGS, &event_param, sizeof(event_param));
                }
                /*anc settings*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x0D))
                {
                    memcpy(event_param.anc_settings.bd_addr, p_link->bd_addr, 6);
                    if (data[7] == 0x01)
                    {
                        event_param.anc_settings.setting = APP_DURIAN_AVP_ANC_CLOSE;
                    }
                    else if (data[7] == 0x02)
                    {
                        event_param.anc_settings.setting = APP_DURIAN_AVP_ANC_OPEN;
                    }
                    else if (data[7] == 0x03)
                    {
                        event_param.anc_settings.setting = APP_DURIAN_AVP_ANC_TRANSPARENCY_MODE;
                    }

                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_ANC_SETTINGS, &event_param, sizeof(event_param));
                }

                /*pro click settings: anc or voice recg*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x16))
                {
                    if (data[7] == 0x05)
                    {
                        event_param.click_settings.right_ear_longpress_control = APP_DURIAN_AVP_CONTROL_ANC;
                    }
                    else if (data[7] == 0x01)
                    {
                        event_param.click_settings.right_ear_longpress_control = APP_DURIAN_AVP_CONTROL_VOICE_RECOGNITION;
                    }
                    else if (data[7] == 0x06)
                    {
                        event_param.click_settings.right_ear_longpress_control = APP_DURIAN_AVP_CONTROL_VOL_UP;
                    }
                    else if (data[7] == 0x07)
                    {
                        event_param.click_settings.right_ear_longpress_control = APP_DURIAN_AVP_CONTROL_VOL_DOWN;
                    }

                    if (data[8] == 0x05)
                    {
                        event_param.click_settings.left_ear_longpress_control = APP_DURIAN_AVP_CONTROL_ANC;
                    }
                    else if (data[8] == 0x01)
                    {
                        event_param.click_settings.left_ear_longpress_control = APP_DURIAN_AVP_CONTROL_VOICE_RECOGNITION;
                    }
                    else if (data[8] == 0x06)
                    {
                        event_param.click_settings.left_ear_longpress_control = APP_DURIAN_AVP_CONTROL_VOL_UP;
                    }
                    else if (data[8] == 0x07)
                    {
                        event_param.click_settings.left_ear_longpress_control = APP_DURIAN_AVP_CONTROL_VOL_DOWN;
                    }

                    memcpy(event_param.click_settings.bd_addr, p_link->bd_addr, 6);
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_CLICK_SETTINGS, &event_param, sizeof(event_param));
                }
                /*pro click settings: anc apt cycle*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x1A))
                {
                    event_param.cycle_settings.setting = data[7];

                    memcpy(event_param.cycle_settings.bd_addr, p_link->bd_addr, 6);
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_ANC_APT_CYCLE, &event_param, sizeof(event_param));
                }
                /*compactness test*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x21) && (data[5] == 0x00))
                {
                    memcpy(event_param.compactness_test.bd_addr, p_link->bd_addr, 6);
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_COMPACTNESS_TEST, &event_param,
                                                sizeof(event_param));
                }
                /*click speed*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x17))
                {
                    memcpy(event_param.click_speed_settings.bd_addr, p_link->bd_addr, 6);
                    event_param.click_speed_settings.speed = (T_APP_DURIAN_AVP_CLICK_SPEED)data[7];
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_CLICK_SPEED, &event_param, sizeof(event_param));
                }
                /*long press time*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x18))
                {
                    memcpy(event_param.long_press_time_settings.bd_addr, p_link->bd_addr, 6);
                    event_param.long_press_time_settings.time = (T_APP_DURIAN_AVP_LONG_PRESS_TIME)data[7];
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_LONG_RESS_TIME, &event_param, sizeof(event_param));
                }
                /*one bud in ear to open anc*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x1B))
                {
                    memcpy(event_param.one_bud_anc.bd_addr, p_link->bd_addr, 6);

                    if (data[7] == 0x01)
                    {
                        event_param.one_bud_anc.is_open = true;
                    }
                    else
                    {
                        event_param.one_bud_anc.is_open = false;
                    }

                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_ONE_BUD_ANC, &event_param, sizeof(event_param));
                }
                /*air max*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x1C))
                {
                    memcpy(event_param.digital_crown_settings.bd_addr, p_link->bd_addr, 6);

                    if (data[7] == 0x01)
                    {
                        event_param.digital_crown_settings.is_clockwise = true;
                    }
                    else if (data[7] == 0x02)
                    {
                        event_param.digital_crown_settings.is_clockwise = false;
                    }

                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_DIGITAL_CROWN_SETTINGS, &event_param,
                                                sizeof(event_param));
                }
                /*share audio*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x1f) && (data[5] == 0x00))
                {
                    memcpy(event_param.audio_sharing.bd_addr, p_link->bd_addr, 6);

                    if (data[6] == data[7])
                    {
                        event_param.audio_sharing.is_enable = true;
                    }
                    else
                    {
                        event_param.audio_sharing.is_enable = false;
                    }

                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_AUDIO_SHARING, &event_param, sizeof(event_param));
                }
                /*spatial audio*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x17) && (data[5] == 0x00))
                {
                    memcpy(event_param.spatial_audio.bd_addr, p_link->bd_addr, 6);
                    event_param.spatial_audio.data = &data[0];
                    event_param.spatial_audio.len = len;
                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_SPATIAL_AUDIO, &event_param, sizeof(event_param));
                }
                /*volume control setting*/
                else if ((data[0] == 0x04) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x09) && (data[5] == 0x00) &&
                         (data[6] == 0x25))
                {
                    memcpy(event_param.volume_control_settings.bd_addr, p_link->bd_addr, 6);

                    if (data[7] == 0x01)
                    {
                        event_param.volume_control_settings.is_enable = true;
                    }
                    else if (data[7] == 0x02)
                    {
                        event_param.volume_control_settings.is_enable = false;
                    }

                    app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_VOLUME_CONTROL_SETTINGS, &event_param,
                                                sizeof(event_param));
                }
                /*discon event*/
                else if ((data[0] == 0x02) && (data[1] == 0x00) &&
                         (data[2] == 0x04) && (data[3] == 0x00) &&
                         (data[4] == 0x00) && (data[5] == 0x00))
                {
                    uint8_t disconn_ack[6] = {0x03, 0x00, 0x04, 0x00, 0x00, 0x00};
                    bt_avp_data_send(p_link->bd_addr, disconn_ack, sizeof(disconn_ack), false);
                }
            }
        }
        break;
    }

    if (handle == true)
    {
        APP_PRINT_INFO1("app_durian_avp_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_durian_att_bt_cback(uint8_t *bd_addr, T_BT_ATT_MSG_TYPE msg_type, void *p_msg)
{
    T_APP_BR_LINK *p_link;

    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        return;
    }

    switch (msg_type)
    {
    case BT_ATT_MSG_CONN_CMPL:
        {
            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                app_bt_policy_msg_prof_conn(p_link->bd_addr, GATT_PROFILE_MASK);
                p_link->audio_opus_status = AVP_VIOCE_RECOGNITION_CREATE;  //opus audio_path created

                if (avp_opus_handle == NULL)
                {
                    T_AUDIO_FORMAT_INFO opus_info;

                    durian_avp_opus_info(&opus_info, AUDIO_FORMAT_TYPE_OPUS);

                    avp_opus_handle = audio_track_create(AUDIO_STREAM_TYPE_RECORD,
                                                         AUDIO_STREAM_MODE_NORMAL,
                                                         AUDIO_STREAM_USAGE_LOCAL,
                                                         opus_info,
                                                         0,
                                                         app_cfg_const.record_volume_default,
                                                         AUDIO_DEVICE_IN_MIC,
                                                         NULL,
                                                         app_durian_avp_track_async_read);
                }
            }
        }
        break;

    case BT_ATT_MSG_DISCONN_CMPL:
        {
            p_link = app_link_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                if (app_link_get_b2s_link_num_with_profile(GATT_PROFILE_MASK) == 1)
                {
                    audio_track_release(avp_opus_handle);
                    avp_opus_handle = NULL;
                }

                p_link->audio_opus_status = AVP_VIOCE_RECOGNITION_IDLE; //opus audio_path released
                app_bt_policy_msg_prof_disconn(p_link->bd_addr, GATT_PROFILE_MASK, 0);
                app_start_timer(&timer_idx_att_connnect, "att_connect",
                                app_durian_avp_timer_id, APP_TIMER_ATT_CONNECT, 0, false,
                                AVP_ATT_CONNECT_MS);
            }
        }
        break;

    default:
        break;
    }
}

bool app_durian_gatt_cback(uint16_t conn_handle, uint8_t service_id, uint16_t cid, uint8_t *data,
                           uint16_t length)
{
    uint8_t bd_addr[6];
    T_APP_BR_LINK *p_link;
    uint8_t addr_type;
    T_APP_DURIAN_AVP_EVENT_PARAM event_param;

    gap_chann_get_addr(conn_handle, bd_addr, &addr_type);
    p_link = app_link_find_br_link(bd_addr);

    if (p_link != NULL)
    {
        if (length == 0x05)
        {
            //voice recognition start
            if ((data[0] == 0x01) &&
                (data[1] == 0x00) && (data[2] == 0x04) &&
                (data[3] == 0x01) && (data[4] == 0x07))
            {
                memcpy(event_param.voice_recognition_start.bd_addr, bd_addr, 6);
                app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_VOICE_RECOGNITION_START, &event_param,
                                            sizeof(event_param));
            }
            //voice recognition stop
            else if ((data[0] == 0x01) &&
                     (data[1] == 0x00) && (data[2] == 0x04) &&
                     (data[3] == 0x01) && (data[4] == 0x00))
            {
                memcpy(event_param.voice_recognition_stop.bd_addr, bd_addr, 6);
                app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_VOICE_RECOGNITION_STOP, &event_param,
                                            sizeof(event_param));
            }
        }
        else if (length == 0x02)
        {
            //encode start
            if ((data[0] == 0x01) && (data[1] == 0x00))
            {
                memcpy(event_param.voice_recognition_encode_start.bd_addr, bd_addr, 6);
                app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_VOICE_RECOGNITION_ENCODE_START, &event_param,
                                            sizeof(event_param));
            }
        }
        else if (length == 0x03)
        {
            //encode stop
            if ((data[0] == 0x01) &&
                (data[1] == 0x00) && (data[2] == 0x00))
            {
                siri_seq = 0;

                memcpy(event_param.voice_recognition_encode_stop.bd_addr, bd_addr, 6);
                app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_VOICE_RECOGNITION_ENCODE_STOP, &event_param,
                                            sizeof(event_param));
            }
        }
        else if (length == 0x01)
        {
            if (data[0] == 0)
            {
                memcpy(event_param.auto_apt_settings.bd_addr, bd_addr, 6);
                event_param.auto_apt_settings.enable = false;
                app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_AUTO_APT_SETTINGS, &event_param,
                                            sizeof(event_param));
            }
            else if (data[0] == 0x01)
            {
                memcpy(event_param.auto_apt_settings.bd_addr, bd_addr, 6);
                event_param.auto_apt_settings.enable = true;
                app_durian_avp_handle_event(APP_DURIAN_AVP_EVENT_AUTO_APT_SETTINGS, &event_param,
                                            sizeof(event_param));
            }
        }

        //report start
        /*else if ((data[0] == 0x01) &&
                    (data[1] == 0x00) && (data[2] == 0x04) &&
                    (data[3] == 0x01) && (data[4] == 0x03))
        {

        }

        //report stop
        else if ((data[0] == 0x01) &&
                    (data[1] == 0x00) && (data[2] == 0x04) &&
                    (data[3] == 0x01) && (data[4] == 0x02))
        {

        }*/

        //error event
        /*else if ((data[0] == 0x01) &&
                    (data[1] == 0x00) && (data[2] == 0x06))
        {

        }*/

        //eq & apt settings
        /*else if ((data[0] == 0x00) && (data[1] == 0x00))
        {
            memcpy(event_param.apt_gain_settings.bd_addr, bd_addr, 6);
            event_param.apt_gain_settings.left_ear_gian = data[36] +
                                                    (data[37] << 8) +
                                                    (data[38] << 16) +
                                                    (data[39] << 24);

            event_param.apt_gain_settings.right_ear_gian = data[84] +
                                                        (data[85] << 8) +
                                                        (data[86] << 16) +
                                                        (data[87] << 24);

            app_avp_cback(APP_DURIAN_AVP_EVENT_APT_GAIN_SETTINGS, &event_param, sizeof(event_param));

            memcpy(event_param.apt_tone_settings.bd_addr, bd_addr, 6);
            event_param.apt_tone_settings.left_ear_tone = data[40] +
                                                    (data[41] << 8) +
                                                    (data[42] << 16) +
                                                    (data[43] << 24);

            event_param.apt_tone_settings.right_ear_tone = data[88] +
                                                        (data[89] << 8) +
                                                        (data[90] << 16) +
                                                        (data[91] << 24);

            app_avp_cback(APP_DURIAN_AVP_EVENT_APT_TONE_SETTINGS, &event_param, sizeof(event_param));

            if ((data[4] == 0x08) && (data[5] == 0xac) &&
                (data[6] == 0x8e))
            {
                memcpy(event_param.balanced_tone_slight.bd_addr, bd_addr, 6);
                app_avp_cback(APP_DURIAN_AVP_EVENT_BALANCED_TONE_SLIGHT, &event_param, sizeof(event_param));
            }
            else if ((data[4] == 0x62) && (data[5] == 0x10) &&
                        (data[6] == 0xda))
            {
                memcpy(event_param.balanced_tone_moderate.bd_addr, bd_addr, 6);
                app_avp_cback(APP_DURIAN_AVP_EVENT_BALANCED_TONE_MODERATE, &event_param, sizeof(event_param));
            }
            else if ((data[4] == 0xac) && (data[5] == 0x9c) &&
                        (data[6] == 0x3b))
            {
                memcpy(event_param.balanced_tone_strong.bd_addr, bd_addr, 6);
                app_avp_cback(APP_DURIAN_AVP_EVENT_BALANCED_TONE_STRONG, &event_param, sizeof(event_param));
            }
            else if ((data[4] == 0x42) && (data[5] == 0x60) &&
                        (data[6] == 0x13))
            {
                memcpy(event_param.vocal_range_slight.bd_addr, bd_addr, 6);
                app_avp_cback(APP_DURIAN_AVP_EVENT_VOCAL_RANGE_SLIGHT, &event_param, sizeof(event_param));
            }
            else if ((data[4] == 0x25) && (data[5] == 0x06) &&
                        (data[6] == 0x3d))
            {
                memcpy(event_param.vocal_range_moderate.bd_addr, bd_addr, 6);
                app_avp_cback(APP_DURIAN_AVP_EVENT_VOCAL_RANGE_MODERATE, &event_param, sizeof(event_param));
            }
            else if ((data[4] == 0xb8) && (data[5] == 0x1e) &&
                        (data[6] == 0xce))
            {
                memcpy(event_param.vocal_range_strong.bd_addr, bd_addr, 6);
                app_avp_cback(APP_DURIAN_AVP_EVENT_VOCAL_RANGE_STRONG, &event_param, sizeof(event_param));
            }
            else if ((data[4] == 0x39) && (data[5] == 0xb4) &&
                        (data[6] == 0x14))
            {
                memcpy(event_param.brightness_slight.bd_addr, bd_addr, 6);
                app_avp_cback(APP_DURIAN_AVP_EVENT_BRIGHTNESS_SLIGHT, &event_param, sizeof(event_param));
            }
            else if ((data[4] == 0x17) && (data[5] == 0xd9) &&
                        (data[6] == 0xaf))
            {
                memcpy(event_param.brightness_moderate.bd_addr, bd_addr, 6);
                app_avp_cback(APP_DURIAN_AVP_EVENT_BRIGHTNESS_MODERATE, &event_param, sizeof(event_param));
            }
            else if ((data[4] == 0x10) && (data[5] == 0xd8) &&
                        (data[6] == 0x25))
            {
                memcpy(event_param.brightness_strong.bd_addr, bd_addr, 6);
                app_avp_cback(APP_DURIAN_AVP_EVENT_BRIGHTNESS_STRONG, &event_param, sizeof(event_param));
            }
            else
            {
                BTM_PRINT_TRACE0("bt_att_cback: unknown eq settings");
            }
        }*/
    }


    return false;
}

static void app_durian_avp_bt_mgr_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_A2DP_DISCONN_CMPL:
        {
#if (F_APP_SENSOR_MEMS_SUPPORT == 1)
            T_APP_BR_LINK *p_link;
            p_link = app_link_find_br_link(param->a2dp_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                app_sensor_mems_spatial_stop(p_link->id);
            }
#endif
        }
        break;

    case BT_EVENT_HFP_CALL_STATUS:
        {
#if (F_APP_LISTENING_MODE_SUPPORT == 1)
            if (param->hfp_call_status.curr_status == BT_HFP_CALL_IDLE)
            {
                app_durian_anc_call_stop_check();
            }
            else
            {
                app_durian_anc_call_start_check();
            }
#endif
        }
        break;

    case BT_EVENT_A2DP_STREAM_STOP:
        {
            T_APP_BR_LINK *p_link;
            p_link = app_link_find_br_link(param->a2dp_stream_stop.bd_addr);

            if (p_link != NULL)
            {
                durian_avp_stream_stop(param->a2dp_stream_stop.bd_addr);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_START_IND:
        {
            T_APP_BR_LINK *p_link;
            p_link = app_link_find_br_link(param->a2dp_stream_start_ind.bd_addr);

            if (p_link != NULL)
            {
                durian_avp_stream_start(param->a2dp_stream_start_ind.bd_addr);
            }
        }
        break;

    case BT_EVENT_A2DP_STREAM_DATA_IND:
        {
#if (F_APP_SENSOR_MEMS_SUPPORT == 1)
            if (app_cfg_const.mems_support)
            {
                T_APP_BR_LINK *p_link;
                p_link = app_link_find_br_link(param->a2dp_stream_data_ind.bd_addr);

                if (p_link != NULL)
                {
                    if (p_link->mems_is_start)
                    {
                        durian_atti_a2dp_ts(param->a2dp_stream_data_ind.timestamp);
                    }
                }
            }
#endif
        }
        break;

    case BT_EVENT_DID_ATTR_INFO:
        {
            T_APP_BR_LINK *p_link = NULL;

            p_link = app_link_find_br_link(param->did_attr_info.bd_addr);

            if (p_link != NULL)
            {
                if (param->did_attr_info.vendor_id == IOS_VENDOR_ID)
                {
                    p_link->remote_device_vendor_id = APP_REMOTE_DEVICE_IOS;
                    p_link->remote_device_vendor_version = param->did_attr_info.version;
                }
                else
                {
                    p_link->remote_device_vendor_id = APP_REMOTE_DEVICE_OTHERS;
                }

                T_DID_SYNC did_sync = {0};
                did_sync.version = param->did_attr_info.version;
                did_sync.bd_addr = param->did_attr_info.bd_addr;

                durian_avp_did_sync(&did_sync);
            }

        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        if (event_type != BT_EVENT_A2DP_STREAM_DATA_IND)
        {
            APP_PRINT_INFO1("app_avp_bt_mgr_cback: event_type 0x%04x", event_type);
        }
    }
}

static void app_durian_avp_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_avp_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_ATT_CONNECT:
        {
            app_stop_timer(&timer_idx_att_connnect);
            //connect att no matter opus status
            {
                for (uint8_t i = 0; i < MAX_BR_LINK_NUM; i++)
                {
                    if (app_db.br_link[i].connected_profile & AVP_PROFILE_MASK)
                    {
                        APP_PRINT_TRACE0("app_avp_timeout_cb: bt_att_connect_req");
                        bt_att_connect_req(app_db.br_link[i].bd_addr);
                    }
                }
            }

#if(F_APP_LISTENING_MODE_SUPPORT == 1)
            app_durian_anc_report(durian_db.anc_cur_setting);
#endif
        }
        break;

    case APP_TIMER_AVP_COMPACTNESS:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                app_durian_avp_compactness_state_machine(true);
            }
            app_stop_timer(&timer_idx_avp_compactness);

        }
        break;

    case APP_TIMER_VOICE_CALL_VP_DELAY:
        {
            app_stop_timer(&timer_idx_voice_call_vp_delay);
            app_audio_tone_type_play(TONE_HF_CALL_VOICE_DIAL, false, true);
        }
        break;

    default:
        break;
    }
}

void app_durian_avp_init(void)
{
    if (durian_cfg.one_trig_need)
    {
        bt_avp_init(app_durian_avp_bt_cback);
        bt_att_init(app_durian_att_bt_cback);
        avp_att_add_service(app_durian_gatt_cback);
    }
    bt_mgr_cback_register(app_durian_avp_bt_mgr_cback);
    app_timer_reg_cb(app_durian_avp_timeout_cb, &app_durian_avp_timer_id);
}
#endif
