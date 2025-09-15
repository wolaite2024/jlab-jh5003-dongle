/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include "trace.h"
#include "btm.h"
#include "app_cfg.h"
#include "app_main.h"
#include "bt_hfp_ag.h"
#include "remote.h"
#include "app_link_util.h"
#include "app_hfp_ag.h"
#include "string.h"
#include "app_led_ctl.h"
#include "app_io_msg.h"
#include "app_dongle_vol.h"
#include "app_usb_hid.h"
#include "app_sdp_a2dp_src.h"

#ifdef LEGACY_BT_GENERAL
static uint8_t app_src_hfp_headset_gain;
static uint8_t data[2] = {0x00, 0x00};
static APP_USB_TELEPHONY_CB src_telephony_cb;
static T_BT_HFP_AG_CALL_STATUS call_status;
static T_APP_BR_LINK *current_hfp_link;

bool app_src_hfp_sco_connected;
bool hook_switch_flag = false;
bool save_mic_flag = false;
uint8_t save_mic = 15;

extern bool mute_ignore;
extern bool app_hfp_isInCall;
extern bool hs_is_mute;
extern bool isMute;
extern bool teams_is_mute;
extern bool isOutgoingCall;
extern bool isIncomingCall;
extern bool is_hold_mute;
extern bool is_hold_call;
extern uint8_t call_count;
extern uint8_t hold_count;
extern uint8_t call_dir;
extern T_BT_HFP_AG_CURRENT_CALL_STATUS curr_call_status;

bool app_src_hfp_set_spk_vol(int16_t vol)
{
    bool rt = true;
    uint8_t hfpvol = 0;

    APP_PRINT_TRACE1("app_src_hfp_set_spk_vol %x", vol);
    if (current_hfp_link != 0)
    {
        if (vol == 0)
        {
            hfpvol = 0;
        }
        else if (vol > 0 && vol < 4)
        {
            hfpvol = 1;
        }
        else if (vol < 7)
        {
            hfpvol = 2;
        }
        else if (vol < 9)
        {
            hfpvol = 3;
        }
        else if (vol < 13)
        {
            hfpvol = 4;
        }
        else if (vol < 18)
        {
            hfpvol = 5;
        }
        else if (vol < 23)
        {
            hfpvol = 6;
        }
        else if (vol < 31)
        {
            hfpvol = 7;
        }
        else if (vol < 39)
        {
            hfpvol = 8;
        }
        else if (vol < 46)
        {
            hfpvol = 9;
        }
        else if (vol < 56)
        {
            hfpvol = 10;
        }
        else if (vol < 67)
        {
            hfpvol = 11;
        }
        else if (vol < 79)
        {
            hfpvol = 12;
        }
        else if (vol < 92)
        {
            hfpvol = 13;
        }
        else if (vol < 107)
        {
            hfpvol = 14;
        }
        else if (vol <= 127)
        {
            hfpvol = 15;
        }

        if (app_src_hfp_headset_gain != hfpvol)
        {
            rt = bt_hfp_ag_speaker_gain_set(current_hfp_link->bd_addr, hfpvol);

            if (rt)
            {
                app_src_hfp_headset_gain = hfpvol;
            }
        }
        APP_PRINT_TRACE3("app_src_hfp_handle_set_vol: vol %x hs %x rt %x", hfpvol, app_src_hfp_headset_gain,
                         rt);
    }

    return rt;
}

static void app_hfp_ag_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_HFP_AG_CONN_IND:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_ag_conn_ind.bd_addr);
            if (p_link == NULL)
            {
                APP_PRINT_ERROR0("app_hfp_ag_bt_cback: no acl link found");
                return;
            }

            bt_hfp_ag_connect_cfm(p_link->bd_addr, true);
        }
        break;

    case BT_EVENT_HFP_AG_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_ag_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                p_link->call_id_type_check = true;
                p_link->call_id_type_num = false;
                current_hfp_link = p_link;
            }
        }
        break;

    case BT_EVENT_HFP_AG_DISCONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->hfp_ag_disconn_cmpl.bd_addr);
            if (p_link != NULL)
            {
            }
            current_hfp_link = NULL;
        }
        break;

    case BT_EVENT_SCO_CONN_CMPL:
        {
            T_APP_BR_LINK *p_link;

            p_link = app_find_br_link(param->sco_conn_cmpl.bd_addr);
            if (p_link != NULL)
            {
                bt_sco_link_switch(param->sco_conn_cmpl.bd_addr);
            }

            app_src_hfp_sco_connected = true;
            if (!app_hfp_isInCall)
            {
                hs_is_mute = false;
            }
            // SCO connect, set pc vol to sco
            app_src_hfp_set_spk_vol(get_dongle_host_vol());
        }

        break;

    case BT_EVENT_SCO_DISCONNECTED:
        {
            // Notify app to stop audio stream.
            APP_PRINT_TRACE0("app_hfp_ag_bt_cback: BT_EVENT_SCO_DISCONNECTED");
            app_src_hfp_sco_connected = false;
            if (!app_hfp_isInCall)
            {
                hs_is_mute = false;
            }
        }
        break;

    case BT_EVENT_HFP_AG_MIC_VOLUME_CHANGED:
        {
            // Update AG mic volume.
            APP_PRINT_TRACE4("app_hfp_ag_bt_cback: hf mic volume:%d, isMute: %d, teams_is_mute: %d, mute_ignore: %d",
                             param->hfp_ag_mic_volume_changed.volume, isMute, teams_is_mute, mute_ignore);
            if (!mute_ignore)
            {
                if (param->hfp_ag_mic_volume_changed.volume == 0)
                {
                    hs_is_mute = true;
                    if (hs_is_mute != isMute)
                    {
                        if (app_hfp_isInCall)
                        {
                            if (src_telephony_cb)
                            {
                                data[0] = 0x01;
                                src_telephony_cb(data, sizeof(data));
                                data[0] = 0x09;
                                src_telephony_cb(data, sizeof(data));
                            }
                        }
                        else
                        {
                            save_mic_flag = true;
                            save_mic = param->hfp_ag_mic_volume_changed.volume;
                        }
                        isMute = true;
                    }
                }

                if (param->hfp_ag_mic_volume_changed.volume == 15)
                {
                    hs_is_mute = false;
                    if (hs_is_mute != isMute)
                    {
                        if (app_hfp_isInCall)
                        {
                            if (src_telephony_cb)
                            {
                                data[0] = 0x09;
                                src_telephony_cb(data, sizeof(data));
                                data[0] = 0x01;
                                src_telephony_cb(data, sizeof(data));
                            }
                        }
                        else
                        {
                            save_mic_flag = true;
                            save_mic = param->hfp_ag_mic_volume_changed.volume;
                        }
                        isMute = false;
                    }
                }
            }
            else
            {
                if (isOutgoingCall)
                {
                    if (param->hfp_ag_mic_volume_changed.volume == 0)
                    {
                        if (src_telephony_cb)
                        {
                            data[0] = 0x01;
                            src_telephony_cb(data, sizeof(data));
                            data[0] = 0x09;
                            src_telephony_cb(data, sizeof(data));
                        }
                        app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
                    }

                    if (param->hfp_ag_mic_volume_changed.volume == 15)
                    {
                        if (src_telephony_cb)
                        {
                            data[0] = 0x09;
                            src_telephony_cb(data, sizeof(data));
                            data[0] = 0x01;
                            src_telephony_cb(data, sizeof(data));
                        }
                        app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
                    }
                }
                save_mic_flag = true;
                save_mic = param->hfp_ag_mic_volume_changed.volume;
                if (save_mic == 0)
                {
                    hs_is_mute = true;
                }
                else
                {
                    hs_is_mute = false;
                }
            }
        }
        break;

    case BT_EVENT_HFP_AG_SPK_VOLUME_CHANGED:
        {
            APP_PRINT_TRACE1("hfp vol change %x", param->avrcp_volume_changed.volume);
            app_src_hfp_headset_gain = param->hfp_ag_spk_volume_changed.volume;
            if (app_src_hfp_sco_connected)
            {
                T_IO_MSG gpio_msg;
                gpio_msg.type = IO_MSG_TYPE_USB_HID;
                gpio_msg.subtype = USB_HID_MSG_TYPE_CONSUMER_CRTL;
                uint8_t sp[16] = {0, 2, 5, 10, 12, 17, 22, 30, 37, 45, 55, 65, 78, 91, 108, 127};
                gpio_msg.u.param = sp[param->hfp_ag_spk_volume_changed.volume];
                // gpio_msg.u.param =  param->bt_hfp_ag_spk_volume_changed.volume * 127;
                // gpio_msg.u.param =  gpio_msg.u.param / 15;
                app_io_msg_send(&gpio_msg);
            }
        }
        break;

    case BT_EVENT_HFP_AG_CODEC_TYPE_SELECTED:
        // App start CVSD/MSBC codec handling according to param->bt_hfp_ag_codec_type_selected.codec_type.
        APP_PRINT_INFO1("app_hfp_ag_bt_cback: codec_type_selected:%d",
                        param->hfp_ag_codec_type_selected.codec_type);
        break;

    case BT_EVENT_HFP_AG_CURR_CALLS_LIST_QUERY:
        {
            APP_PRINT_INFO2("app_hfp_ag_bt_cback: curr_call_status: %d, call_dir: %d", curr_call_status,
                            call_dir);
            if ((app_hfp_isInCall || isIncomingCall || isOutgoingCall) &&
                (curr_call_status != BT_HFP_AG_CURRENT_CALL_STATUS_HELD))
            {
                uint8_t call_idx = 1;
                T_BT_HFP_AG_CURRENT_CALL_MODE call_mode = BT_HFP_AG_CURRENT_CALL_MODE_VOICE;
                uint8_t is_multi_party = 0;
                char *call_num = NULL;
                uint8_t call_type = 128;

                bt_hfp_ag_current_calls_list_send(param->hfp_ag_curr_calls_list_query.bd_addr, call_idx, call_dir,
                                                  curr_call_status,
                                                  call_mode, is_multi_party, call_num, call_type);
            }
            bt_hfp_ag_ok_send(param->hfp_ag_curr_calls_list_query.bd_addr);
        }
        break;

    case BT_EVENT_HFP_AG_CALL_ANSWER_REQ:
        {
            APP_PRINT_INFO1("app_hfp_ag_bt_cback BT_EVENT_HFP_AG_CALL_ANSWER_REQ, call_status: %d",
                            call_status);
            if (call_status == BT_HFP_AG_CALL_INCOMING)
            {
                if (call_count == 0)
                {
                    if (src_telephony_cb)
                    {
                        data[0] = 0x01;
                        src_telephony_cb(data, sizeof(data));
                    }
                }
                else if (hold_count > 0)
                {
                    if (src_telephony_cb)
                    {
                        data[0] = 0x00;
                        src_telephony_cb(data, sizeof(data));
                        data[0] = 0x01;
                        src_telephony_cb(data, sizeof(data));
                        hook_switch_flag = false;
                    }
                }
                else
                {
                    if (src_telephony_cb)
                    {
                        data[0] = 0x11;
                        src_telephony_cb(data, sizeof(data));
                        data[0] = 0x01;
                        src_telephony_cb(data, sizeof(data));
                    }
                }
                app_hfp_isInCall = true;
            }
            else
            {
                if (src_telephony_cb)
                {
                    data[0] = 0x00;
                    src_telephony_cb(data, sizeof(data));
                    data[0] = 0x01;
                    src_telephony_cb(data, sizeof(data));
                    hook_switch_flag = true;
                }
            }
        }
        break;

    case BT_EVENT_HFP_AG_CALL_TERMINATE_REQ:
        {
            APP_PRINT_INFO1("app_hfp_ag_bt_cback BT_EVENT_HFP_AG_CALL_TERMINATE, call_status: %d", call_status);
            if (call_status == BT_HFP_AG_CALL_ACTIVE)
            {
                if (src_telephony_cb)
                {
                    data[0] = 0x01;
                    src_telephony_cb(data, sizeof(data));
                    data[0] = 0x00;
                    src_telephony_cb(data, sizeof(data));
                }
                hook_switch_flag = false;
            }

            if (call_status == BT_HFP_AG_CALL_INCOMING)
            {
                uint8_t send_data[2] = {0};
                data[1] = 0x08;
                if (call_count == 0)
                {
                    data[0] = 0x00;
                }
                else
                {
                    if (hold_count == 0)
                    {
                        if (teams_is_mute)
                        {
                            data[0] = 0x09;
                            send_data[0] = 0x09;
                        }
                        else
                        {
                            data[0] = 0x01;
                            send_data[0] = 0x01;
                        }
                    }
                    else
                    {
                        if (is_hold_mute)
                        {
                            data[0] = 0x08;
                            send_data[0] = 0x08;
                        }
                        else
                        {
                            data[0] = 0x00;
                            send_data[0] = 0x00;
                            hook_switch_flag = false;
                        }
                    }
                }
                if (src_telephony_cb)
                {
                    src_telephony_cb(data, sizeof(data));
                    src_telephony_cb(send_data, sizeof(send_data));
                }
                data[1] = 0x00;
            }
        }
        break;

    case BT_EVENT_HFP_AG_CALL_STATUS_CHANGED:
        {
            // Update AG call_status.
            call_status = (T_BT_HFP_AG_CALL_STATUS)param->hfp_ag_call_status_changed.curr_status;
            APP_PRINT_INFO3("app_hfp_ag_bt_cback call_status: pre_call_status:%d, call_status:%d, call_count:%d",
                            param->hfp_ag_call_status_changed.prev_status, call_status, call_count);
        }
        break;

    case BT_EVENT_HFP_AG_BATTERY_LEVEL:
        {
            // Update the battery level.
            APP_PRINT_INFO1("app_hfp_ag_bt_cback: hf battery_level:%d",
                            param->hfp_ag_battery_level.battery_level);
        }
        break;

    case BT_EVENT_SCO_DATA_IND:
        {
            // SCO data recieved handling.
        }
        break;

    case BT_EVENT_HFP_AG_INDICATORS_STATUS_REQ:
        {
            // App provide current network status.
            T_BT_HFP_AG_SERVICE_INDICATOR service_status = BT_HFP_AG_SERVICE_STATUS_UNAVAILABLE;
            // Bt_hfp_ag provide current call status.
            T_BT_HFP_AG_CALL_INDICATOR call_status;
            T_BT_HFP_AG_CALL_SETUP_INDICATOR call_setup_status;
            if (app_hfp_isInCall)
            {
                call_setup_status = BT_HFP_AG_CALL_SETUP_STATUS_IDLE;
                call_status = BT_HFP_AG_CALL_IN_PROGRESS;
            }
            else if (isIncomingCall)
            {
                call_setup_status = BT_HFP_AG_CALL_SETUP_STATUS_INCOMING_CALL;
                call_status = BT_HFP_AG_NO_CALL_IN_PROGRESS;
            }
            else if (isOutgoingCall)
            {
                call_setup_status = BT_HFP_AG_CALL_SETUP_STATUS_IDLE;
                call_status = BT_HFP_AG_NO_CALL_IN_PROGRESS;
            }
            else
            {
                call_setup_status = BT_HFP_AG_CALL_SETUP_STATUS_IDLE;
                call_status = BT_HFP_AG_NO_CALL_IN_PROGRESS;
            }
            // App provide current signal status.
            uint8_t signal_status = 0;
            // App provide current roam status.
            T_BT_HFP_AG_ROAMING_INDICATOR roam_status = BT_HFP_AG_ROAMING_STATUS_INACTIVE;
            // App provide current battery status.
            uint8_t batt_chg_status = 0;
#ifdef BTDONGLE_BQB_MODE_ENABLE
            extern bool app_is_bqb_mode();
            if (app_is_bqb_mode())
            {
                service_status = BT_HFP_AG_SERVICE_STATUS_AVAILABLE;
                extern uint8_t iut_call_status;
                call_status = (T_BT_HFP_AG_CALL_STATUS)iut_call_status;
                iut_call_status = 0; // reset for next testcase
                signal_status = 3;
                batt_chg_status = 3;
            }
#endif

            T_BT_HFP_AG_CALL_HELD_INDICATOR call_held_status;
            if ((is_hold_call) && (!is_hold_mute))
            {
                call_held_status = BT_HFP_AG_CALL_HELD_STATUS_HOLD_AND_ACTIVE_CALL;
            }
            else if ((is_hold_call) && (is_hold_mute))
            {
                call_held_status = BT_HFP_AG_CALL_HELD_STATUS_HOLD_NO_ACTIVE_CALL;
            }
            else
            {
                call_held_status = BT_HFP_AG_CALL_HELD_STATUS_IDLE;
            }
            bt_hfp_ag_indicators_send(param->hfp_ag_indicators_status_req.bd_addr,
                                      service_status, call_status, call_setup_status, call_held_status,
                                      signal_status, roam_status, batt_chg_status);
            bt_hfp_ag_ok_send(param->hfp_ag_indicators_status_req.bd_addr);
        }
        break;

    case BT_EVENT_HFP_AG_DIAL_WITH_MEMORY:
        {
            bt_hfp_ag_error_send(param->hfp_ag_dial_with_memory.bd_addr, BT_HFP_AG_ERR_OP_NOT_SUPPORTED);
        }
        break;

    case BT_EVENT_HFP_AG_DIAL_LAST_NUMBER:
        {
            bt_hfp_ag_error_send(param->hfp_ag_dial_last_number.bd_addr, BT_HFP_AG_ERR_OP_NOT_SUPPORTED);
        }
        break;

    case BT_EVENT_HFP_AG_NREC_STATUS:
        {
            bt_hfp_ag_ok_send(param->hfp_ag_nrec_status.bd_addr);
        }
        break;

    case BT_EVENT_HFP_AG_VENDOR_CMD:
        {
            bt_hfp_ag_error_send(param->hfp_ag_vendor_cmd.bd_addr, BT_HFP_AG_ERR_INVALID_CHAR_IN_TSTR);
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
        APP_PRINT_INFO1("app_hfp_ag_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_usb_telephony_register_cb(APP_USB_TELEPHONY_CB telephony_state_cb)
{
    src_telephony_cb = telephony_state_cb;
}

void app_hfp_ag_init(void)
{
    if (app_cfg_const.supported_profile_mask & (HFP_PROFILE_MASK | HSP_PROFILE_MASK))
    {
        bt_hfp_ag_init(RFC_HFP_AG_CHANN_NUM, RFC_HSP_AG_CHANN_NUM, 0x268,
                       BT_HFP_AG_CODEC_TYPE_CVSD | BT_HFP_AG_CODEC_TYPE_MSBC);
        bt_mgr_cback_register(app_hfp_ag_bt_cback);
    }
}
#endif
