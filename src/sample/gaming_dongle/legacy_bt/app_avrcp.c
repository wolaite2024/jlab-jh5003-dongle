/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "btm.h"
#include "bt_avrcp.h"
#include "app_cfg.h"
#include "app_link_util.h"
#include "app_avrcp.h"
#include "app_dongle_vol.h"
#include "app_usb_hid.h"
#include "usb_host_detect.h"
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
#include "uac_silence_detect.h"
#endif


static void app_avrcp_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;
    T_APP_BR_LINK *p_link;
    bool handle = true;

    if (event_type != BT_EVENT_SDP_ATTR_INFO)
    {
        APP_PRINT_INFO2("app_avrcp_bt_cback: event %x bd_addr %b", event_type,
                        TRACE_BDADDR(param->avrcp_conn_ind.bd_addr));
    }

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
#ifndef ENABLE_UAC2
#if F_APP_LEGACY_DISABLE_ABSOLUTE_VOLUME || F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
            /* dongle don't handle abs vol changed */
#else
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
                    APP_PRINT_INFO2("get_dongle_host_vol %02x,get_dongle_host_mute %d", get_dongle_host_vol(),
                                    get_dongle_host_mute());
                    if (usb_host_detect_os_type_peek() == OS_TYPE_PS)
                    {
                        app_dongle_set_host_type(DONGLE_HOST_HID_INVALID);
                    }
                    app_dongle_set_avrcp_vol();
                }
                else
                {
                    app_dongle_handle_a2u_set_vol(param->avrcp_volume_changed.volume, false);
                }
            }
#endif
#endif
        }
        break;

    case BT_EVENT_AVRCP_VOLUME_UP:
        {
            p_link = app_find_br_link(param->avrcp_volume_up.bd_addr);
            if (p_link != NULL)
            {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
                app_usb_hid_send_consumer_ctrl_key_down(KEY_VOL_UP);
                app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
            }
        }
        break;

    case BT_EVENT_AVRCP_VOLUME_DOWN:
        {
            p_link = app_find_br_link(param->avrcp_volume_down.bd_addr);
            if (p_link != NULL)
            {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
                app_usb_hid_send_consumer_ctrl_key_down(KEY_VOL_DOWN);
                app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
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
        /* NOTICE: gaming lib also deals with this event internally. */
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

#if (DEVICE_CMD_SUPPORT == 1)
    case BT_EVENT_SDP_ATTR_INFO:
        {
            T_BT_SDP_ATTR_INFO *sdp_info = &param->bt_sdp_attr_info.info;

            if (sdp_info->srv_class_uuid_data.uuid_16 == UUID_AV_REMOTE_CONTROL_TARGET)
            {
                if (app_cmd_get_report_attr_info_flag())
                {
                    uint8_t temp_buff[5];

                    temp_buff[0] = GET_AVRCP_ATTR_INFO;
                    memcpy(&temp_buff[1], &(sdp_info->profile_version), 2);
                    memcpy(&temp_buff[3], &(sdp_info->supported_feat), 2);

                    app_cmd_set_report_attr_info_flag(false);
                    app_report_event(CMD_PATH_UART, EVENT_REPORT_REMOTE_DEV_ATTR_INFO, 0, temp_buff,
                                     sizeof(temp_buff));
                }
            }
        }
        break;
#endif

    case BT_EVENT_AVRCP_PLAY:
        p_link = app_find_br_link(param->avrcp_play.bd_addr);
        if (p_link != NULL)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY_PAUSE);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
            silence_detect_set_hid_state(false);
#endif
        }
        break;

    case BT_EVENT_AVRCP_STOP:
        p_link = app_find_br_link(param->avrcp_stop.bd_addr);
        if (p_link != NULL)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_STOP);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
        }
        break;

    case BT_EVENT_AVRCP_PAUSE:
        p_link = app_find_br_link(param->avrcp_pause.bd_addr);
        if (p_link != NULL)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY_PAUSE);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
            silence_detect_set_hid_state(true);
#endif
        }
        break;

    case BT_EVENT_AVRCP_REWIND_START:
        p_link = app_find_br_link(param->avrcp_rewind_start.bd_addr);
        if (p_link != NULL)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_REWIND);
#endif
        }
        break;
    case BT_EVENT_AVRCP_REWIND_STOP:
        p_link = app_find_br_link(param->avrcp_rewind_stop.bd_addr);
        if (p_link != NULL)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
        }
        break;

    case BT_EVENT_AVRCP_FAST_FORWARD_START:
        p_link = app_find_br_link(param->avrcp_fast_forward_start.bd_addr);
        if (p_link != NULL)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_FORWARD);
#endif
        }
        break;

    case BT_EVENT_AVRCP_FAST_FORWARD_STOP:
        p_link = app_find_br_link(param->avrcp_fast_forward_stop.bd_addr);
        if (p_link != NULL)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
        }
        break;

    case BT_EVENT_AVRCP_FORWARD: /* next song */
        p_link = app_find_br_link(param->avrcp_forward.bd_addr);
        if (p_link != NULL)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_NEXT_TK);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
        }
        break;

    case BT_EVENT_AVRCP_BACKWARD: /* previous song */
        p_link = app_find_br_link(param->avrcp_backward.bd_addr);
        if (p_link != NULL)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            app_usb_hid_send_consumer_ctrl_key_down(KEY_PREV_TK);
            app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
#endif
        }
        break;

    case BT_EVENT_AVRCP_VENDOR_CMD_IND:
        {
            p_link = app_find_br_link(param->avrcp_vendor_cmd_ind.bd_addr);
            if (p_link != NULL)
            {
                bt_avrcp_vendor_rsp_send(p_link->bd_addr, BT_AVRCP_RESPONSE_NOT_IMPLEMENTED,
                                         param->avrcp_vendor_cmd_ind.company_id, NULL, 0);
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
        APP_PRINT_INFO1("app_avrcp_bt_cback: event_type 0x%04x", event_type);
    }
}

void app_avrcp_init(void)
{
    if (app_cfg_const.supported_profile_mask & AVRCP_PROFILE_MASK)
    {
        bt_avrcp_init(app_cfg_const.avrcp_link_number);
        bt_avrcp_supported_features_set(BT_AVRCP_FEATURE_CATEGORY_1 | BT_AVRCP_FEATURE_CATEGORY_2,
                                        BT_AVRCP_FEATURE_CATEGORY_1);
        bt_mgr_cback_register(app_avrcp_bt_cback);
    }
}

