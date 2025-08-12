/*
 *  Copyright (C) 2020 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "trace.h"
#include "app_timer.h"
#include "gap.h"
#include "app_cfg.h"
#include "app_link_util.h"
#include "app_src_avrcp.h"
#include "app_usb_hid.h"
#include "bt_avrcp.h"
//#include "bt_a2dp.h"
#include "hal_gpio.h"
#ifdef BTDONGLE_BQB_MODE_ENABLE
#include "app_bqb_mode_switch.h"
#endif

// #define APP_SRC_AVRCP_VOL_ADJUSTING_TIMEOUT         (50)
// #define APP_SRC_AVRCP_TIMER_ID_AVC_PRESS            0x05
// #define APP_SRC_AVRCP_AVC_PRESS_TIMEOUT             (2000)
#define CHARSET_UTF8    0x6a

uint16_t player_id;
static uint8_t pressed_key;
static uint8_t avrcp_timer_started;
// static uint8_t timer_idx_avrcp = 0;
// uint8_t src_avrcp_timer_id = 0;
int16_t app_src_avrcp_headset_vol;
static T_APP_BR_LINK *current_avrcp_link;

extern void app_hid_interrupt_in(uint8_t *data, uint8_t data_size);

bool player_cb(uint8_t type, void *user_data)
{
    return true;
}

// static void app_src_avrcp_timeout_cb(uint8_t timer_evt, uint16_t param)
// {
// //    APP_PRINT_INFO2("app_src_avrcp_timer_cback: timer_evt %d, param %d",
// //                    timer_evt, param);
//     switch (timer_evt)
//     {
//     case APP_SRC_AVRCP_TIMER_ID_AVC_PRESS:
//         {
//             uint8_t data[3] = {0x02, 0x00, 0x00};

//             APP_PRINT_INFO1("AV/C key %u press timeout", pressed_key);
//             app_stop_timer(&timer_idx_avrcp);
//             avrcp_timer_started = 0;
//             app_hid_interrupt_in(data, 3);
//         }
//         break;
//     default:
//         break;
//     }
// }

static void src_bt_avrcp_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    switch (event_type)
    {
    case BT_EVENT_AVRCP_CONN_IND:
        {
            p_link = app_find_br_link(param->avrcp_conn_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_avrcp_connect_cfm(p_link->bd_addr, true);
            }
        }
        break;

    case BT_EVENT_AVRCP_VOLUME_CHANGED:
        {
            p_link = app_find_br_link(param->avrcp_volume_changed.bd_addr);
            if (p_link != NULL)
            {
                uint16_t vol = param->avrcp_volume_changed.volume;
                vol *= 100;
                vol /= 0x7f;
                APP_PRINT_INFO2("volume changed %02x, %u percent",
                                param->avrcp_volume_changed.volume, vol);
                if (p_link->vol_set_needed)
                {
                    p_link->vol_set_needed = 0;
//                    app_dongle_set_avrcp_vol();
                }
                else
                {
//                    app_dongle_handle_a2u_set_vol(param->avrcp_volume_changed.volume);
                }
            }
        }
        break;

    case BT_EVENT_AVRCP_VOLUME_UP:
        {
            p_link = app_find_br_link(param->avrcp_volume_up.bd_addr);
            if (p_link != NULL)
            {
                APP_PRINT_INFO1("app_avrcp_bt_cback: vol up %s",
                                TRACE_BDADDR(param->avrcp_volume_up.bd_addr));
                app_usb_hid_send_consumer_ctrl_key_down(KEY_VOL_UP);
                app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
            }
        }
        break;

    case BT_EVENT_AVRCP_VOLUME_DOWN:
        {
            p_link = app_find_br_link(param->avrcp_volume_down.bd_addr);
            if (p_link != NULL)
            {
                APP_PRINT_INFO1("app_avrcp_bt_cback: vol down %s",
                                TRACE_BDADDR(param->avrcp_volume_down.bd_addr));
                app_usb_hid_send_consumer_ctrl_key_down(KEY_VOL_DOWN);
                app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
            }
        }
        break;

    case BT_EVENT_AVRCP_REG_VOLUME_CHANGED:
        {
            p_link = app_find_br_link(param->avrcp_reg_volume_changed.bd_addr);
            if (p_link != NULL)
            {
                /* might call
                 * bt_avrcp_volume_change_register_rsp(p_link->bd_addr, vol)
                 */
            }
        }
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED_REG_REQ:
        if (!src_is_streaming(param->avrcp_reg_play_status_changed.bd_addr))
            bt_avrcp_play_status_change_register_rsp(param->avrcp_reg_play_status_changed.bd_addr,
                                                     BT_AVRCP_PLAY_STATUS_PAUSED);
        else
            bt_avrcp_play_status_change_register_rsp(param->avrcp_reg_play_status_changed.bd_addr,
                                                     BT_AVRCP_PLAY_STATUS_PLAYING);

        break;

    case BT_EVENT_AVRCP_CONN_CMPL:
        {
            p_link = app_find_br_link(param->avrcp_conn_cmpl.bd_addr);
            if (p_link)
            {
                p_link->vol_set_needed = 1;
            }
        }
        break;

    case BT_EVENT_AVRCP_DISCONN_CMPL:
        {
            p_link = app_find_br_link(param->avrcp_disconn_cmpl.bd_addr);
            if (p_link)
            {
                p_link->vol_set_needed = 0;
            }
        }
        break;

    case BT_EVENT_AVRCP_PLAY_STATUS_CHANGED:
        {
        }
        break;

    case BT_EVENT_AVRCP_PLAY:
        p_link = app_find_br_link(param->avrcp_play.bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_INFO1("app_avrcp_bt_cback: play %s",
                            TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
        }
        break;

    case BT_EVENT_AVRCP_STOP:
        p_link = app_find_br_link(param->avrcp_stop.bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_INFO1("app_avrcp_bt_cback: stop %s",
                            TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_usb_hid_send_consumer_ctrl_key_down(KEY_STOP);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
        }
        break;

    case BT_EVENT_AVRCP_PAUSE:
        p_link = app_find_br_link(param->avrcp_pause.bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_INFO1("app_avrcp_bt_cback: pause %s",
                            TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PAUSE);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
        }
        break;

    case BT_EVENT_AVRCP_REWIND_START:
        p_link = app_find_br_link(param->avrcp_rewind_start.bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_INFO1("app_avrcp_bt_cback: rewind start %s",
                            TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_usb_hid_send_consumer_ctrl_key_down(KEY_REWIND);
        }
        break;
    case BT_EVENT_AVRCP_REWIND_STOP:
        p_link = app_find_br_link(param->avrcp_rewind_stop.bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_INFO1("app_avrcp_bt_cback: rewind stop %s",
                            TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
        }
        break;

    case BT_EVENT_AVRCP_FAST_FORWARD_START:
        p_link = app_find_br_link(param->avrcp_fast_forward_start.bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_INFO1("app_avrcp_bt_cback: fast forward start %s",
                            TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_usb_hid_send_consumer_ctrl_key_down(KEY_FORWARD);
        }
        break;

    case BT_EVENT_AVRCP_FAST_FORWARD_STOP:
        p_link = app_find_br_link(param->avrcp_fast_forward_stop.bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_INFO1("app_avrcp_bt_cback: fast forward stop %s",
                            TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
        }
        break;

    case BT_EVENT_AVRCP_FORWARD: /* next song */
        p_link = app_find_br_link(param->avrcp_forward.bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_INFO1("app_avrcp_bt_cback: %s",
                            TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_usb_hid_send_consumer_ctrl_key_down(KEY_NEXT_TK);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
        }
        break;

    case BT_EVENT_AVRCP_BACKWARD: /* previous song */
        p_link = app_find_br_link(param->avrcp_backward.bd_addr);
        if (p_link != NULL)
        {
            APP_PRINT_INFO1("app_avrcp_bt_cback: %s",
                            TRACE_BDADDR(param->avrcp_play.bd_addr));
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PREV_TK);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
        }
        break;


    default:
        {
            handle = false;
        }
        break;
    }
    if (handle)
    {
        APP_PRINT_INFO1("app_avrcp_bt_cback: event_type %s", event_type);
    }
}

extern bool bt_avrcp_set_absolute_volume(uint8_t *bd_addr, uint8_t vol);
bool app_src_avrcp_set_spk_vol(uint8_t vol)
{
    bool rt = true;

    APP_PRINT_TRACE1("app_src_avrcp_set_spk_vol %x", vol);
    if (current_avrcp_link != 0)
    {
        if (app_src_avrcp_headset_vol != vol)
        {
#ifdef BTDONGLE_BQB_MODE_ENABLE
            if (app_is_bqb_mode())
            {
                rt = bt_avrcp_notify_volume_change_req(current_avrcp_link->bd_addr, vol);
            }
            else
#endif
            {
                rt = bt_avrcp_set_absolute_volume(current_avrcp_link->bd_addr, vol);
            }
            APP_PRINT_TRACE2("bt_avrcp_set_absolute_volume: vol %x return %x", vol, rt);

            if (rt)
            {
                app_src_avrcp_headset_vol = vol;
            }
        }
        APP_PRINT_TRACE3("app_src_avrcp_handle_set_vol: vol %x hs %x rt %x", vol, app_src_avrcp_headset_vol,
                         rt);
    }

    return rt;
}

void app_src_avrcp_init(void)
{
    char *title = "usb audio";

    if (app_cfg_const.supported_profile_mask & AVRCP_PROFILE_MASK)
    {
        bt_avrcp_init(app_cfg_const.avrcp_link_number);
        bt_mgr_cback_register(src_bt_avrcp_cback);
        player_id = bt_avrcp_register_player(player_cb);
        bt_avrcp_set_attr(player_id, BT_AVRCP_ELEM_ATTR_TITLE, CHARSET_UTF8,
                          strlen(title), (uint8_t *)title);
        // app_timer_reg_cb(app_src_avrcp_timeout_cb, &src_avrcp_timer_id);
    }
}
