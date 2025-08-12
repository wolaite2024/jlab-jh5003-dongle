#include "string.h"
#include "trace.h"
#include "app_general_policy.h"
#include "app_led_ctl.h"
#include "bt_hfp_ag.h"
#include "app_hfp_ag.h"
#include "app_usb_hid.h"
#include "app_src_asp.h"
#include "app_usb_call_control.h"
#include "app_src_audio.h"

#ifdef LEGACY_BT_GENERAL
static uint8_t last_data;
static uint8_t pre_hook = 0;
static uint8_t hold_flag = false;
static bool mute_status = false;
static T_BT_HFP_AG_CURRENT_CALL_STATUS prev_call_status;
static bool sfb_resume_mute_flag = false;
static bool not_resume_flag = false;

bool isMute = false;
bool teams_is_mute = false;
bool hs_is_mute = false;
bool isOutgoingCall = false;
bool isIncomingCall = false;
bool app_hfp_isInCall = false;
bool is_hold_mute = false;
bool is_hold_call = false;
bool mute_ignore = false;
uint8_t call_dir = 0;
uint8_t call_count = 0;
uint8_t hold_count = 0;
T_BT_HFP_AG_CURRENT_CALL_STATUS curr_call_status;

extern bool hook_switch_flag;
extern bool save_mic_flag;
extern uint8_t save_mic;
extern T_APP_LED_MODE current_mode;
extern uint8_t timer_idx_started_meeting_or_incoming_meeting;

void app_usb_telephony_message_sendback(uint8_t *data, uint8_t length)
{
    app_usb_hid_send_report(HID_IF_TELEPHONY, 0, data, length, 0);
}

static void app_resume_notify_mute_status(uint8_t *bd_addr)
{
    APP_PRINT_TRACE6("app_resume_notify_mute_status, mute_status: %d, is_hold_mute: %d, isMute: %d, teams_is_mute: %d, hs_is_mute: %d, sfb_resume_mute_flag: %d",
                     mute_status, is_hold_mute, isMute, teams_is_mute, hs_is_mute, sfb_resume_mute_flag);
    if (mute_status || is_hold_mute || sfb_resume_mute_flag)
    {
        if (!isMute)
        {
            if (!teams_is_mute)
            {
                uint8_t data1[2] = {0x01, 0x00};
                uint8_t data2[2] = {0x09, 0x00};
                app_usb_hid_send_report(HID_IF_TELEPHONY, 0, data1, sizeof(data1), 0);
                app_usb_hid_send_report(HID_IF_TELEPHONY, 0, data2, sizeof(data2), 0);
            }
        }
        if (!hs_is_mute)
        {
            bt_hfp_ag_microphone_gain_set(bd_addr, 0);
            hs_is_mute = true;
        }
        isMute = true;
        mute_status = false;
        sfb_resume_mute_flag = false;
    }
    else
    {
        // notify host in call state
        uint8_t data1[2] = {0x01, 0x00};
        app_usb_hid_send_report(HID_IF_TELEPHONY, 0, data1, sizeof(data1), 0);
    }
}

static void app_notify_hs_unmute_status(uint8_t *bd_addr)
{
    APP_PRINT_TRACE2("app_notify_hs_unmute_status, hs_is_mute: %d, isMute: %d", hs_is_mute,
                     isMute);
    if (hs_is_mute)
    {
        bt_hfp_ag_microphone_gain_set(bd_addr, 15);
        hs_is_mute = false;
    }
    isMute = false;
}

static void app_notify_host_save_mic(uint8_t *bd_addr)
{
    APP_PRINT_TRACE3("app_notify_host_save_mic, teams_is_mute: %d, isMute: %d, hs_is_mute: %d",
                     teams_is_mute, isMute, hs_is_mute);
    if (save_mic == 0)
    {
        if (!teams_is_mute)
        {
            uint8_t data[2] = {0x09, 0x00};
            app_usb_hid_send_report(HID_IF_TELEPHONY, 0, data, sizeof(data), 0);
        }
        if (!hs_is_mute)
        {
            bt_hfp_ag_microphone_gain_set(bd_addr, 0);
            hs_is_mute = true;
        }
        isMute = true;
        app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
    }
    else
    {
        if (teams_is_mute)
        {
            uint8_t data[2] = {0x01, 0x00};
            app_usb_hid_send_report(HID_IF_TELEPHONY, 0, data, sizeof(data), 0);
        }
        if (hs_is_mute)
        {
            bt_hfp_ag_microphone_gain_set(bd_addr, 15);
            hs_is_mute = false;
        }
        isMute = false;
        app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
    }
}

extern T_APP_DEV sink_dev_list[];
void app_usb_hid_handle_set_Telephony_cmd(uint8_t *data, uint8_t length)
{
    uint8_t bd_addr[6] = {0};
    T_APP_DEV *sink_dev = &sink_dev_list[0];
    memcpy(bd_addr, sink_dev->remote_info.bd_addr, 6);

    uint8_t off_hook = data[1] & 0x01;
    uint8_t mute = data[1] & 0x04;
    APP_PRINT_INFO4("app_usb_hid_handle_set_Telephony_cmd data %b, last_data %d, isOutgoingCall %d save_mic_flag %d",
                    TRACE_BINARY(length, data), last_data, isOutgoingCall, save_mic_flag);
    if (data[1] != last_data)
    {
        if (data[1] == 0x11 || data[1] == 0x15)
        {
            app_hfp_isInCall = false;
            isIncomingCall = false;
            if (!app_led_set_before_white())
            {
                app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
            }
        }

        uint8_t ring = data[1] & 0x08;
        uint8_t hold = data[1] & 0x10;

        uint8_t last_hook = last_data & 0x01;
        uint8_t last_mute = last_data & 0x04;
        uint8_t last_ring = last_data & 0x08;
        uint8_t last_hold = last_data & 0x10;

        if (hold != last_hold)
        {
            if (hold == 0x10)
            {
                pre_hook = 0;
                is_hold_call = true;
                if (isMute && hold_count == 0)
                {
                    is_hold_mute = true;
                }
                hold_count++;
                APP_PRINT_TRACE2("hold changed, hold_count: %d, is_hold_mute: %d", hold_count, is_hold_mute);
                if (!teams_connect_complete && off_hook == 1)
                {
                    app_led_set_mode(
                        APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT);
                }
                if (!teams_connect_complete && mute == 0x04 && off_hook == 0x00)
                {
                    sfb_resume_mute_flag = true;
                }
            }
            else
            {
                if (data[1] == 0x00)
                {
                    bt_hfp_ag_held_call_release(bd_addr);
                    isIncomingCall = false;
                    isOutgoingCall = false;
                    app_hfp_isInCall = false;
                    call_count = 0;
                    hold_flag = false;
                    hold_count--;
                    is_hold_mute = false;
                    not_resume_flag = false;
                    isMute = false;
                    teams_is_mute = false;
                    hs_is_mute = false;
                    uint8_t data[2] = {0x00, 0x00};
                    app_usb_hid_send_report(HID_IF_TELEPHONY, 0, data, sizeof(data), 0);
                    hook_switch_flag = false;
                }
                else if (off_hook == 0x01)
                {
                    app_hfp_isInCall = true;
                    if (mute == 0x04)
                    {
                        app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
                    }
                    else
                    {
                        app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
                    }
                }
            }
            last_data = data[1];
            return;
        }

        app_stop_flash_on_and_off_timer();
        APP_PRINT_TRACE1("app_usb_hid_handle_set_Telephony_cmd, current_call_num: %d", call_count);
        if (off_hook != last_hook)
        {
            APP_PRINT_TRACE4("is_hold_call: %d, hold: %d, is_hold_mute:%d, hold_count: %d", is_hold_call, hold,
                             is_hold_mute, hold_count);
            if (off_hook == 0x01)
            {
                if (ring == 0x00)
                {
                    if (is_hold_call)
                    {
                        if (hold == 0x10) // call-2 outgoing
                        {
                            bt_hfp_ag_call_setup_indicator_send(bd_addr, BT_HFP_AG_CALL_SETUP_STATUS_OUTGOING_CALL);
                            if (sink_dev->sco_state == SCO_SETUP_STATE_IDLE)
                            {
                                sink_dev->sco_state = SCO_SETUP_STATE_CONNECTING;
                                bt_hfp_ag_audio_connect_req(bd_addr);
                            }
                            mute_status = isMute;
                            isOutgoingCall = true;
                            hold_count = 1;
                            is_hold_call = true;
                            hold_flag = false;
                            mute_ignore = true;
                            call_dir = 0;
                            curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_DIALING;
                            if (!teams_connect_complete)
                            {
                                app_led_set_mode(
                                    APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT);
                            }
                        }
                        else if (!not_resume_flag)
                        {
                            APP_PRINT_TRACE2("call-1 resumed, call_count: %d, mute_status: %d", call_count, mute_status);
                            bt_hfp_ag_call_indicator_send(bd_addr, BT_HFP_AG_CALL_IN_PROGRESS);
                            bt_hfp_ag_call_setup_indicator_send(bd_addr, BT_HFP_AG_CALL_SETUP_STATUS_IDLE);
                            bt_hfp_ag_audio_connect_req(bd_addr);
                            curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_ACTIVE;
                            app_resume_notify_mute_status(bd_addr);
                            if (mute == 0x00)
                            {
                                app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
                            }
                            else
                            {
                                app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
                            }
                            hold_count = 0;
                            is_hold_call = false;
                            is_hold_mute = false;
                            hold_flag = false;
                            call_count = 1;
                            app_hfp_isInCall = true;
                            mute_ignore = false;
                        }
                    }
                    else // call-1 outgoing
                    {
                        bt_hfp_ag_call_dial(bd_addr);
                        bt_hfp_ag_audio_connect_req(bd_addr);
                        curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_DIALING;
                        call_dir = 0;
                        mute_ignore = true;
                        app_led_set_mode(
                            APP_LED_MODE_OUTGOING_CALLS_VIA_DIAL_PAD_OR_TEAMS_CONTACT_OR_CELLPHONE_CONTACT);
                        isOutgoingCall = true;
                    }
                }
            }
            else
            {
                if ((hold == 0x10) && (hold_count == 1)) // call-1 hold
                {
                    if (!teams_connect_complete)
                    {
                        // hang up 2nd call or 2nd outgoing call is rejected by far end
                        if (call_count > 0)
                        {
                            call_count--;
                        }
                    }
                    hold_flag = true;
                    hs_is_mute = false;
                    curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_HELD;
                    bt_hfp_ag_call_hold(bd_addr);
                    if (!app_usb_audio_is_us_streaming())
                    {
                        bt_hfp_ag_audio_disconnect_req(bd_addr);
                    }
                    app_hfp_isInCall = false;
                    if (!app_led_set_before_white())
                    {
                        app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
                    }
                }
                else if ((hold == 0x00) || ((hold == 0x10) && (call_count == 2)))
                {
                    isMute = false;
                    bt_hfp_ag_call_terminate(bd_addr);
                    if (!app_usb_audio_is_us_streaming())
                    {
                        bt_hfp_ag_audio_disconnect_req(bd_addr);
                    }

                    uint8_t send_data[2] = {0x00, 0x00};
                    app_usb_hid_send_report(HID_IF_TELEPHONY, 0, send_data, sizeof(send_data), 0);
                    hook_switch_flag = false;
                    app_hfp_isInCall = false;

                    APP_PRINT_TRACE3("call terminated, hold_count: %d, call_count: %d, is_hold_mute: %d", hold_count,
                                     call_count, is_hold_mute);

                    if (hold_count > 0)
                    {
                        hold_count--;
                    }
                    if (call_count > 0)
                    {
                        call_count--;
                    }

                    if (call_count == 0)
                    {
                        is_hold_mute = false;
                        is_hold_call = false;
                        hold_count = 0;
                        mute_status = false;
                        not_resume_flag = false;
                        teams_is_mute = false;
                        hs_is_mute = false;
                    }
                    else
                    {
                        not_resume_flag = true;
                        curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_HELD;
                    }

                    if (isOutgoingCall)
                    {
                        isOutgoingCall = false;
                    }

                    if (!timer_idx_started_meeting_or_incoming_meeting)
                    {
                        if (!app_led_set_before_white())
                        {
                            app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
                        }
                    }
                    else
                    {
                        uint8_t rconfig[] = {4, 100, 100, 1, 0, 0};
                        uint8_t bconfig[] = {4, 100, 100, 1, 0, 0};
                        uint8_t gconfig[] = {0, 0, 0, 0, 0, 0};
                        app_led_set_custom_config(0, rconfig);
                        app_led_set_custom_config(1, bconfig);
                        app_led_set_custom_config(2, gconfig);
                    }
                }
            }
        }

        if (mute != last_mute)
        {
            APP_PRINT_TRACE5("come here, isMute: %d, hold_flag: %d, isOutgoingCall: %d, hs_is_mute: %d, mute: %d",
                             isMute, hold_flag, isOutgoingCall, hs_is_mute, mute);
            if (off_hook == 0x01)
            {
                app_hfp_isInCall = true;
            }
            else
            {
                app_hfp_isInCall = false;
            }
            if (mute == 0x04)
            {
                teams_is_mute = true;
                isMute = true;
                if (isOutgoingCall)
                {
                    bt_hfp_ag_call_answer(bd_addr);
                    curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_ACTIVE;
                    mute_ignore = false;
                    save_mic_flag = false;
                    isOutgoingCall = false;
                }
                if (!hold_flag && off_hook == 0x01)
                {
                    app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
                }
                else
                {
                    app_led_set_mode(current_mode);
                }
                if ((hs_is_mute != isMute) && (off_hook == 0x01))
                {
                    bt_hfp_ag_microphone_gain_set(bd_addr, 0);
                    hs_is_mute = true;
                }
            }
            else
            {
                teams_is_mute = false;
                isMute = false;
                if (!hold_flag && off_hook == 0x01)
                {
                    app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
                }
                else
                {
                    app_led_set_mode(current_mode);
                }
                if ((hs_is_mute != isMute) && (off_hook == 0x01))
                {
                    bt_hfp_ag_microphone_gain_set(bd_addr, 15);
                    hs_is_mute = false;
                }
            }
        }

        if (ring != last_ring)
        {
            if (ring == 0x08)
            {
                if (call_count > 0)
                {
                    bt_hfp_ag_call_terminate(bd_addr);
                    bt_hfp_ag_call_setup_indicator_send(bd_addr, BT_HFP_AG_CALL_SETUP_STATUS_INCOMING_CALL);
                    if (sink_dev->sco_state == SCO_SETUP_STATE_IDLE)
                    {
                        sink_dev->sco_state = SCO_SETUP_STATE_CONNECTING;
                        bt_hfp_ag_audio_connect_req(bd_addr);
                    }
                    mute_status = isMute;
                    hold_flag = false;
                }
                else
                {
                    bt_hfp_ag_call_incoming(bd_addr, "10086", 6, 129);
                }
                if (hook_switch_flag && off_hook == 0)
                {
                    uint8_t data[2] = {0x00, 0x00};
                    app_usb_hid_send_report(HID_IF_TELEPHONY, 0, data, sizeof(data), 0);
                    hook_switch_flag = false;
                }
                call_dir = 1;
                prev_call_status = curr_call_status;
                curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_INCOMING;
                isIncomingCall = true;
                mute_ignore = true;
                pre_hook = off_hook;
                app_led_set_mode(APP_LED_MODE_INCOMING_CALLS_VIA_CELL_OR_TEAMS);
            }
            else
            {
                if (off_hook == 0x01)
                {
                    APP_PRINT_TRACE4("incoming call , app_hfp_isInCall: %d, pre_hook: %d, call_count: %d, mute_status: %d",
                                     app_hfp_isInCall, pre_hook, call_count, mute_status);
                    if (app_hfp_isInCall && pre_hook)
                    {
                        bt_hfp_ag_call_indicator_send(bd_addr, BT_HFP_AG_CALL_IN_PROGRESS);
                        bt_hfp_ag_call_setup_indicator_send(bd_addr, BT_HFP_AG_CALL_SETUP_STATUS_IDLE);
                        isIncomingCall = false;
                        curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_ACTIVE;
                        mute_ignore = false;
                        if (mute == 0x04)
                        {
                            bt_hfp_ag_microphone_gain_set(bd_addr, 0);
                            hs_is_mute = true;
                            app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
                        }
                        else
                        {
                            app_notify_hs_unmute_status(bd_addr);
                            app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
                        }
                    }
                    else
                    {
                        if (call_count == 0)
                        {
                            bt_hfp_ag_call_answer(bd_addr);
                        }
                        else
                        {
                            bt_hfp_ag_call_indicator_send(bd_addr, BT_HFP_AG_CALL_IN_PROGRESS);
                            bt_hfp_ag_call_setup_indicator_send(bd_addr, BT_HFP_AG_CALL_SETUP_STATUS_IDLE);
                        }
                        curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_ACTIVE;
                        uint8_t send_data[2] = {0x01, 0x00};
                        app_usb_hid_send_report(HID_IF_TELEPHONY, 0, send_data, sizeof(send_data), 0);
                        app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
                        mute_ignore = false;
                        if (call_count < 2)
                        {
                            call_count++;
                        }
                        app_hfp_isInCall = true;
                        isIncomingCall = false;
                    }
                }
                else
                {
                    APP_PRINT_TRACE3("incoming call rejected, call_count: %d, is_hold_call: %d, is_hold_mute: %d",
                                     call_count, is_hold_call, is_hold_mute);
                    bt_hfp_ag_call_terminate(bd_addr);
                    isIncomingCall = false;
                    isMute = false;
                    mute_ignore = false;
                    save_mic_flag = false;
                    if (!app_usb_audio_is_us_streaming())
                    {
                        bt_hfp_ag_audio_disconnect_req(bd_addr);
                    }

                    if (call_count == 0)
                    {
                        app_hfp_isInCall = false;
                    }
                    else
                    {
                        curr_call_status = prev_call_status;
                    }

                    if (!timer_idx_started_meeting_or_incoming_meeting)
                    {
                        if (!app_led_set_before_white())
                        {
                            app_led_set_mode(APP_LED_MODE_TEAMS_IS_ACTIVE_IDLE_STATE);
                        }
                    }
                    else
                    {
                        uint8_t rconfig[] = {4, 100, 100, 1, 0, 0};
                        uint8_t bconfig[] = {4, 100, 100, 1, 0, 0};
                        uint8_t gconfig[] = {0, 0, 0, 0, 0, 0};
                        app_led_set_custom_config(0, rconfig);
                        app_led_set_custom_config(1, bconfig);
                        app_led_set_custom_config(2, gconfig);
                    }
                }
            }
        }
        last_data = data[1];
    }
    else
    {
        if (off_hook == 0x01 && isOutgoingCall)
        {
            APP_PRINT_TRACE1("outgoing call answered, call_count: %d", call_count);
            if (call_count == 0)
            {
                bt_hfp_ag_call_answer(bd_addr);
            }
            else
            {
                bt_hfp_ag_call_indicator_send(bd_addr, BT_HFP_AG_CALL_IN_PROGRESS);
                bt_hfp_ag_call_setup_indicator_send(bd_addr, BT_HFP_AG_CALL_SETUP_STATUS_IDLE);
            }
            call_dir = 0;
            curr_call_status = BT_HFP_AG_CURRENT_CALL_STATUS_ACTIVE;
            uint8_t send_data[2] = {0x01, 0x00};
            app_usb_hid_send_report(HID_IF_TELEPHONY, 0, send_data, sizeof(send_data), 0);
            app_stop_flash_on_and_off_timer();
            if (save_mic_flag)
            {
                app_notify_host_save_mic(bd_addr);
                save_mic_flag = false;
            }
            else
            {
                if (mute == 0x04)
                {
                    app_led_set_mode(APP_LED_MODE_TEAMS_STATUS_MUTE);
                }
                else
                {
                    app_notify_hs_unmute_status(bd_addr);
                    app_led_set_mode(APP_LED_MODE_TEAMS_IN_CALL);
                }
            }
            mute_ignore = false;
            if (call_count < 2)
            {
                call_count++;
            }
            app_hfp_isInCall = true;
            isOutgoingCall = false;
        }
        else if (data[1] == 0x01 && last_data == 0x01 && save_mic_flag)
        {
            app_notify_host_save_mic(bd_addr);
            save_mic_flag = false;
        }
    }
}

void app_usb_call_control_init(void)
{
    app_usb_telephony_register_cb(app_usb_telephony_message_sendback);
}
#endif
