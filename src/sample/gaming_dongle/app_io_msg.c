/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "app_io_msg.h"
#include "app_console_msg.h"
#include "app_charger.h"
#include "app_key_process.h"
#include "app_cfg.h"
#include "app_audio_path.h"
#include "app_usb_uac.h"
#include "app_ble_gap.h"

#if LOCAL_PLAYBACK_FEATURE_SUPPORT
#include "app_playback_update_file.h"
#endif

#include "ual_adapter.h"
#include "ble_audio.h"
#include "le_audio_data.h"
#include "app_usb_layer.h"
#include "app_main.h"
#include "os_msg.h"
#ifdef LEGACY_BT_GENERAL
#include "app_general_policy.h"
#endif
#if DONGLE_LE_AUDIO
#include "app_le_audio.h"
#endif
#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
#include "uac_silence_detect.h"
#endif
#include "app_usb.h"
#include "usb_dm.h"
#include "dev_mgr.h"
#include "section.h"
#include "app_console.h"
#include "le_unicast_src_service.h"

#if F_APP_USB_GIP_SUPPORT
#include "app_gip.h"
#include "app_gip_security.h"
#endif
#include "app_usb_hid_report.h"
#include "app_upstream_decode.h"
#include "app_downstream_encode.h"

#if F_APP_GAMING_CONTROLLER_SUPPORT
#include "app_usb_controller.h"
#endif

void (*app_rtk_charger_box_dat_msg_hook)(T_IO_MSG *) = NULL;

RAM_TEXT_SECTION
bool app_io_msg_send(T_IO_MSG *io_msg)
{
    T_EVENT_TYPE event = EVENT_IO_TO_APP;
    bool ret = false;

    if (os_msg_send(audio_io_queue_handle, io_msg, 0) == true)
    {
        ret = os_msg_send(audio_evt_queue_handle, &event, 0);
    }

    if (ret == false)
    {
        APP_PRINT_ERROR3("app_io_msg_send failed, type = %x, subtype = %x, param = %x",
                         io_msg->type, io_msg->subtype, io_msg->u.param);
    }

    return ret;
}

static void app_handle_dongle_app_msg(T_IO_MSG *io_driver_msg_recv)
{
    switch (io_driver_msg_recv->subtype)
    {
    case REMOVE_BOND_EVENT:
        {
            T_RMV_BOND_DEV *p_rmv_dev = (T_RMV_BOND_DEV *)io_driver_msg_recv->u.buf;
            if (p_rmv_dev != NULL)
            {
                //le_unicast_src_release_group();
                bt_dev_clear_bond_info(p_rmv_dev->bd_addr);
                T_BT_DEVICE *p_dev_rec = ual_find_alloc_device_by_addr(p_rmv_dev->bd_addr, p_rmv_dev->bd_type);
                if (p_dev_rec)
                {
                    le_bond_pair(p_dev_rec->le_conn_id);
                }
            }
        }
        break;
    case APP_LE_AUDIO_MSG_SEND_EVENT:
        {
#if DONGLE_LE_AUDIO
            app_le_audio_handle_msg(io_driver_msg_recv->u.param);
#endif
        }
        break;
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
    case SILENCE_DETECT_EVENT:
        {
            app_handle_silence_detect_event(io_driver_msg_recv);
        }
        break;
#if (ENABLE_UAC2 == 1)
    case UAC2_SILENCE_DETECT_EVENT:
        {
            app_handle_uac2_silence_detect_event(io_driver_msg_recv);
        }
        break;
#endif
#endif
#if F_APP_USB_GIP_SUPPORT
    case GIP_READ_I2C_STATE:
    case GIP_READ_DATA:
        app_gip_pscc_handle_app_msg(io_driver_msg_recv);
        break;
#endif

    default:
        break;
    }
}

void app_io_handle_msg(T_IO_MSG io_driver_msg_recv)
{
    uint16_t msgtype = io_driver_msg_recv.type;
    uint16_t subtype = io_driver_msg_recv.subtype;

    switch (msgtype)
    {
    case IO_MSG_TYPE_GPIO:
        {
            switch (io_driver_msg_recv.subtype)
            {
            case IO_MSG_GPIO_KEY:
                app_key_handle_msg(&io_driver_msg_recv);
                break;

            case IO_MSG_GPIO_UART_WAKE_UP:
                {
                    app_console_uart_handle_msg(&io_driver_msg_recv);
                }
                break;

            case IO_MSG_GPIO_CHARGER:
                app_charger_handle_msg(&io_driver_msg_recv);
                break;

#if LOCAL_PLAYBACK_FEATURE_SUPPORT
            case IO_MSG_GPIO_PLAYBACK_TRANS_FILE_ACK:
                app_playback_trans_write_file_ack_handle_msg(&io_driver_msg_recv);
                break;
#endif

            case IO_MSG_GPIO_ADAPTOR_PLUG:
                APP_PRINT_TRACE0("IO_MSG_GPIO_ADAPTOR_PLUG");
                //app_charger_plug_handle_msg(&io_driver_msg_recv);
                //if (app_cfg_const.enable_external_mcu_reset)
                //{
                //    app_io_output_ctrl_ext_mcu_pin(ENABLE);
                //}
                break;

            case IO_MSG_GPIO_ADAPTOR_UNPLUG:
                APP_PRINT_TRACE0("IO_MSG_GPIO_ADAPTOR_UNPLUG");
                //app_charger_unplug_handle_msg(&io_driver_msg_recv);
                //if (app_cfg_const.enable_external_mcu_reset)
                //{
                //    APP_PRINT_TRACE0("adp unplug disable output 10hz");
                //    app_io_output_ctrl_ext_mcu_pin(DISABLE);
                //}
                break;

            case IO_MSG_GPIO_ADAPTOR_DAT:
                //app_adp_handle_msg(&io_driver_msg_recv);
                break;

            case IO_MSG_GPIO_LINE_IN:
                app_line_in_detect_msg_handler(&io_driver_msg_recv);
                break;

#if F_APP_USB_GIP_SUPPORT
            case IO_MSG_GPIO_SLIDE_SWITCH_0:
                {
                    app_gip_mode_switch_handle_msg(&io_driver_msg_recv);
                }
                break;
#endif

            default:
                break;
            }
        }
        break;

    case IO_MSG_TYPE_USB_HID:
        {
#if F_APP_USB_HID_SUPPORT
#include "app_usb_hid.h"
            app_usb_hid_handle_msg(&io_driver_msg_recv);
#endif
        }
        break;

    case IO_MSG_TYPE_BT_STATUS:
        {
            ual_handle_ble_gap_msg(&io_driver_msg_recv);
            app_ble_gap_handle_gap_msg(&io_driver_msg_recv);
            le_unicast_handle_gap_msg(&io_driver_msg_recv);
        }
        break;

    case IO_MSG_TYPE_LE_AUDIO:
        {
            ble_audio_handle_msg(&io_driver_msg_recv);
        }
        break;

    case IO_MSG_TYPE_APP_USB_LAYER:
        {
            usb_layer_signal_handle(&io_driver_msg_recv);
        }
        break;

    case IO_MSG_TYPE_CONSOLE:
        app_console_handle_msg(io_driver_msg_recv);
        break;

    case IO_MSG_TYPE_USB_DEV:
        app_usb_msg_handle(&io_driver_msg_recv);
        break;

#if F_APP_USB_GIP_SUPPORT
    case IO_MSG_TYPE_USB_GIP:
        app_gip_usb_msg_handle(&io_driver_msg_recv);
        break;
#endif

    case IO_MSG_TYPE_DONGLE_APP:
        app_handle_dongle_app_msg(&io_driver_msg_recv);
        break;

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    case IO_MSG_TYPE_US_PKT_DECODE:
        {
            us_decode_pkt_from_buf();
        }
        break;
#endif

    case IO_MSG_TYPE_DS_PCM_ENCODE:
        {
            if (subtype == DS_MSG_SUBTYPE_PCM_DECODE)
            {
                app_usb_ds_pcm_encode();
            }
#if F_APP_HANDLE_DS_PIPE_IN_ISR
            else if (subtype == DS_MSG_SUBTYPE_DS_STREAMING)
            {
                app_usb_ds_streaming_handle();
            }
#endif
        }
        break;

#if F_APP_GAMING_CONTROLLER_SUPPORT
    case IO_MSG_TYPE_CONTROLLER:
        {
            app_usb_controller_io_msg(&io_driver_msg_recv);
        }
        break;
#endif

    default:
        break;
    }
}
