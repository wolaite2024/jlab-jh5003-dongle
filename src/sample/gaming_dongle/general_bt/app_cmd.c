/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "trace.h"
#include "console.h"
#include "gap_scan.h"
#include "app_timer.h"
#include "os_mem.h"
#include "ancs_client.h"
#include "gap_br.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_audio_policy.h"
#include "app_transfer.h"
#include "app_report.h"
#include "app_ble_gap.h"
#include "app_bt_policy_api.h"
#include "mp_test.h"
#include "app_mmi.h"
#include "app_cfg.h"
#include "bt_hfp.h"
#include "bt_iap.h"
#include "tts.h"
#include "btm.h"
#include "bt_bond.h"
#include "app_ota.h"
#include "remote.h"
#include "voice_prompt.h"
#include "eq.h"
#include "app_eq.h"
#include "stdlib.h"
#include "patch_header_check.h"
#include "flash_device.h"
#include "app_relay.h"
#include "app_anc.h"
#include "test_mode.h"
#include "platform_utils.h"
#include "app_roleswap.h"
#include "rtl876x_pinmux.h"
#include "rom_ext.h"

//for CMD_TTS
#define TTS_TYPE_START                  0x00
#define TTS_TYPE_SEND_DATA              0x01

#define TTS_DATA_SINGLE                 0x00
#define TTS_DATA_START                  0x01
#define TTS_DATA_CONTINUE               0x02
#define TTS_DATA_END                    0x03

#define TTS_HEADER_LEN                  0x000A
#define TTS_SYNC_BYTE                   0xB8
#define TTS_INIT_SEQ                    0x00
#define TTS_FRAME_LEN                   0x02

//for CMD_SET_CONFIGURATION
#define CFG_TYPE_LE_NAME                0x00
#define CFG_TYPE_LEGACY_NAME            0x01
#define CFG_TYPE_AUDIO_LATENCY          0x02
#define CFG_TYPE_SUPPORT_CODEC          0x03
#define CFG_TYPE_MAX                    0x04

//for CMD_LINE_IN_CTRL
#define CFG_LINE_IN_STOP                0x00
#define CFG_LINE_IN_START               0x01


/* uart receive sequence number */
static uint8_t uart_rx_seqn = 0;

typedef enum
{
    APP_TIMER_SWITCH_TO_HCI_DOWNLOAD_MODE,
} T_APP_CMD_TIMER;

static uint8_t app_cmd_timer_id = 0;
static uint8_t timer_idx_switch_to_hci_mode = 0;

static void app_cmd_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_SWITCH_TO_HCI_DOWNLOAD_MODE:
        {
            app_stop_timer(&timer_idx_switch_to_hci_mode);
            platform_set_hci_download_mode_flag(true);
            set_hci_mode_flag(true);
            WDG_SystemReset(RESET_ALL_EXCEPT_AON);
        }
        break;

    default:
        break;
    }
}

void app_cmd_init(void)
{
    app_timer_reg_cb(app_cmd_timeout_cb, &app_cmd_timer_id);
}

void app_cmd_set_event_ack(uint8_t cmd_path, uint8_t app_idx, uint8_t *buf)
{
    if (cmd_path == CMD_PATH_SPP)
    {
        app_report_spp_event(&app_db.br_link[app_idx], EVENT_ACK, buf, 3);
    }
    else if (cmd_path == CMD_PATH_LE)
    {
        app_report_le_event(&app_db.le_link[app_idx], EVENT_ACK, buf, 3);
    }
    else
    {
        app_report_uart_event(EVENT_ACK, buf, 3);
    }
}

void app_cmd_set_event_broadcast(uint16_t event_id, uint8_t *buf, uint16_t len)
{
    T_APP_BR_LINK *br_link;
    T_APP_LE_LINK *le_link;
    uint8_t        i;

    for (i = 0; i < MAX_BR_LINK_NUM; i ++)
    {
        br_link = &app_db.br_link[i];

        if (br_link->cmd_set_enable == true)
        {
            app_report_spp_event(br_link, event_id, buf, len);
        }
    }

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        le_link = &app_db.le_link[i];

        if (le_link->cmd_set_enable == true)
        {
            app_report_le_event(le_link, event_id, buf, len);
        }
    }
}

T_FLASH_DATA flash_data;
void app_read_flash(uint32_t start_addr, uint8_t cmd_path, uint8_t app_idx)
{
    uint32_t start_addr_tmp;
    uint16_t data_send_len;

    data_send_len = 0x200;// in case assert fail
    start_addr_tmp = start_addr;

    if (cmd_path == CMD_PATH_SPP)
    {
        APP_PRINT_TRACE1("app_read_flash: rfc_frame_size %d", app_db.br_link[app_idx].rfc_frame_size);
        if (app_db.br_link[app_idx].rfc_frame_size - 12 < data_send_len)
        {
            data_send_len = app_db.br_link[app_idx].rfc_frame_size - 12;
        }
    }
    else if (cmd_path == CMD_PATH_LE)
    {
        APP_PRINT_TRACE1("app_read_flash: mtu_size %d", app_db.le_link[app_idx].mtu_size);
        if (app_db.le_link[app_idx].mtu_size - 15 < data_send_len)
        {
            data_send_len = app_db.le_link[app_idx].mtu_size - 15;
        }
    }

    uint8_t *data = malloc(data_send_len + 6);

    if (data != NULL)
    {
        if (start_addr + data_send_len >= flash_data.flash_data_start_addr + flash_data.flash_data_size)
        {
            data_send_len = flash_data.flash_data_start_addr + flash_data.flash_data_size - start_addr;
            data[0] = END_TRANS_DATA;
        }
        else
        {
            data[0] = CONTINUE_TRANS_DATA;
        }

        data[1] = flash_data.flash_data_type;
        data[2] = (uint8_t)(start_addr_tmp);
        data[3] = (uint8_t)(start_addr_tmp >> 8);
        data[4] = (uint8_t)(start_addr_tmp >> 16);
        data[5] = (uint8_t)(start_addr_tmp >> 24);

        if (flash_read_locked(start_addr_tmp, data_send_len, &data[6]))// read flash data
        {
            if (cmd_path == CMD_PATH_UART)
            {
                app_report_uart_event(EVENT_REPORT_FLASH_DATA, data, data_send_len + 6);

            }
            else if (cmd_path == CMD_PATH_LE)
            {
                app_report_le_event(&app_db.le_link[app_idx], EVENT_REPORT_FLASH_DATA, data, data_send_len + 6);

            }
            else if (cmd_path == CMD_PATH_SPP)
            {
                app_report_spp_event(&app_db.br_link[app_idx], EVENT_REPORT_FLASH_DATA, data, data_send_len + 6);
            }
        }

        flash_data.flash_data_start_addr_tmp += data_send_len;
        os_mem_free(data);
    }
}

//T_FLASH_DATA initialization
void app_flash_data_set_param(uint8_t flash_type, uint8_t cmd_path, uint8_t app_idx)
{
    flash_data.flash_data_type = flash_type;
    flash_data.flash_data_start_addr = 0x800000;
    flash_data.flash_data_size = 0x00;

    switch (flash_type)
    {
    case FLASH_ALL:
        {
            flash_data.flash_data_start_addr = 0x800000;
            flash_data.flash_data_size = 0x100000;
        }
        break;

    case SYSTEM_CONFIG:
        {
            flash_data.flash_data_start_addr = flash_get_bank_addr(FLASH_OCCD);
            flash_data.flash_data_size = flash_get_bank_size(FLASH_OCCD) & 0x00FFFFFF;
        }
        break;

    case ROM_PATCH_IMAGE:
        {
            flash_data.flash_data_start_addr = get_header_addr_by_img_id(IMG_MCUPATCH);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_MCUPATCH);
        }
        break;

    case APP_IMAGE:
        {
            flash_data.flash_data_start_addr = get_header_addr_by_img_id(IMG_MCUAPP);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_MCUAPP);
        }
        break;

    case DSP_SYSTEM_IMAGE:
        {
            flash_data.flash_data_start_addr = get_header_addr_by_img_id(IMG_DSPSYSTEM);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_DSPSYSTEM);
        }
        break;

    case DSP_APP_IMAGE:
        {
            flash_data.flash_data_start_addr = get_header_addr_by_img_id(IMG_DSPAPP);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_DSPAPP);
        }
        break;

    case FTL_DATA:
        {
            flash_data.flash_data_start_addr = flash_get_bank_addr(FLASH_FTL);
            flash_data.flash_data_size = flash_get_bank_size(FLASH_FTL) & 0x00FFFFFF;
        }
        break;

    case ANC_IMAGE:
        {
            flash_data.flash_data_start_addr = get_header_addr_by_img_id(IMG_ANC);
            flash_data.flash_data_size = get_bank_size_by_img_id(IMG_ANC);
        }
        break;

    case LOG_PARTITION:
        {
            //add later;
        }
        break;

    case CORE_DUMP_PARTITION:
        {
            flash_data.flash_data_start_addr = flash_get_bank_addr(FLASH_HARDFAULT_RECORD);
            flash_data.flash_data_size = flash_get_bank_size(FLASH_HARDFAULT_RECORD);
        }
        break;

    default:
        break;
    }

    flash_data.flash_data_start_addr_tmp = flash_data.flash_data_start_addr;

    //report TRANS_DATA_INFO param
    uint8_t paras[10];

    paras[0] = TRANS_DATA_INFO;
    paras[1] = flash_data.flash_data_type;

    paras[2] = (uint8_t)(flash_data.flash_data_size);
    paras[3] = (uint8_t)(flash_data.flash_data_size >> 8);
    paras[4] = (uint8_t)(flash_data.flash_data_size >> 16);
    paras[5] = (uint8_t)(flash_data.flash_data_size >> 24);

    paras[6] = (uint8_t)(flash_data.flash_data_start_addr);
    paras[7] = (uint8_t)(flash_data.flash_data_start_addr >> 8);
    paras[8] = (uint8_t)(flash_data.flash_data_start_addr >> 16);
    paras[9] = (uint8_t)(flash_data.flash_data_start_addr >> 24);

    if (cmd_path == CMD_PATH_UART)
    {
        app_report_uart_event(EVENT_REPORT_FLASH_DATA, paras, sizeof(paras));

    }
    else if (cmd_path == CMD_PATH_LE)
    {
        app_report_le_event(&app_db.le_link[app_idx], EVENT_REPORT_FLASH_DATA, paras, sizeof(paras));

    }
    else if (cmd_path == CMD_PATH_SPP)
    {
        app_report_spp_event(&app_db.br_link[app_idx], EVENT_REPORT_FLASH_DATA, paras, sizeof(paras));
    }
}

void app_handle_cmd_set(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t rx_seqn,
                        uint8_t app_idx)
{
    uint16_t cmd_id;
    uint8_t  ack_pkt[3];

    cmd_id     = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    ack_pkt[0] = cmd_ptr[0];
    ack_pkt[1] = cmd_ptr[1];
    ack_pkt[2] = CMD_SET_STATUS_COMPLETE;

    /* check duplicated seq num */
    if (cmd_id != CMD_ACK && rx_seqn != 0)
    {
        if (cmd_path == CMD_PATH_UART)
        {
            if (uart_rx_seqn == rx_seqn)
            {
                app_report_uart_event(EVENT_ACK, &ack_pkt[0], 3);
                return;
            }
            uart_rx_seqn = rx_seqn;
        }
        else if (cmd_path == CMD_PATH_LE)
        {
            if (app_db.le_link[app_idx].rx_cmd_seqn == rx_seqn)
            {
                app_report_le_event(&app_db.le_link[app_idx], EVENT_ACK, &ack_pkt[0], 3);
                return;
            }
            app_db.le_link[app_idx].rx_cmd_seqn = rx_seqn;
        }
        else if (cmd_path == CMD_PATH_SPP)
        {
            if (app_db.br_link[app_idx].rx_cmd_seqn == rx_seqn)
            {
                app_report_spp_event(&app_db.br_link[app_idx], EVENT_ACK, &ack_pkt[0], 3);
                return;
            }
            app_db.br_link[app_idx].rx_cmd_seqn = rx_seqn;
        }
    }

    switch (cmd_id)
    {
    case CMD_ACK:
        {
            bool is_uart;

            if (cmd_path == CMD_PATH_UART)
            {
                is_uart = true;
                app_transfer_data(is_uart);
            }
            else if ((cmd_path == CMD_PATH_LE) || (cmd_path == CMD_PATH_SPP))
            {
                uint16_t event_id = (uint16_t)(cmd_ptr[2] | (cmd_ptr[3] << 8));
                uint8_t status = cmd_ptr[4];

                is_uart = false;
                app_transfer_data(is_uart);

                if (event_id == EVENT_AUDIO_EQ_PARAM_REPORT)
                {
                    uint16_t max_frame_len;

                    if (cmd_path == CMD_PATH_LE)
                    {
                        T_APP_LE_LINK *p_link;

                        p_link = &app_db.le_link[app_idx];

                        if (status != CMD_SET_STATUS_COMPLETE)
                        {
                            app_eq_report_abort_frame(p_link, cmd_path);

                            break;
                        }

                        /*3 is att header,  6 is trasmint service header*/
                        if (p_link->eq_data_len > 0 && p_link->eq_data_buf != NULL)
                        {
                            max_frame_len = p_link->mtu_size - 3 - EQ_CONTINUE_FRAME_HEADER_LEN - 6 ;

                            if (p_link->eq_data_len <= max_frame_len)
                            {
                                app_eq_report_end_frame(p_link, CMD_PATH_LE);
                            }
                            else
                            {
                                app_eq_report_continue_frame(p_link, CMD_PATH_LE);
                            }
                        }
                    }
                }
                else if (event_id == EVENT_OTA_ACTIVE_ACK)
                {
                    if (cmd_path == CMD_PATH_SPP || cmd_path == CMD_PATH_IAP)
                    {
                        app_ota_cmd_ack_handle(event_id, status);
                    }
                }
            }
        }
        break;

    case CMD_BT_READ_LINK_INFO:
        {
            uint8_t app_index;

            app_index = cmd_ptr[2];
            if (app_index < MAX_BR_LINK_NUM)
            {
                uint8_t event_buff[3];

                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                event_buff[0] = app_index;
                event_buff[1] = app_db.br_link[app_index].connected_profile;
                event_buff[2] = 0;
                app_report_uart_event(EVENT_REPLY_LINK_INFO, &event_buff[0], 3);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            }
        }
        break;

    case CMD_BT_CREATE_CONNECTION:
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        app_bt_policy_default_connect(&cmd_ptr[3], cmd_ptr[2], false);
        break;

    case CMD_BT_DISCONNECT:
        {
            T_APP_BR_LINK *p_link;
            uint8_t bd_addr[6];

            memcpy(bd_addr, &cmd_ptr[2], 6);
            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                app_bt_policy_disconnect(p_link->bd_addr, cmd_ptr[8]);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_MMI:
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

        if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
        {
            //single mode
            app_mmi_handle_action(cmd_ptr[3]);
        }
        else
        {
            if (cmd_ptr[3] == MMI_DEV_FACTORY_RESET)
            {
                app_mmi_handle_action(cmd_ptr[3]);
            }
            else if ((cmd_ptr[3] == MMI_DEV_SPK_VOL_UP) || (cmd_ptr[3] == MMI_DEV_SPK_VOL_DOWN) ||
                     (cmd_ptr[3] == MMI_DEV_POWER_OFF))
            {
                if (app_cfg_nv.bud_role == DEVICE_ROLE_PRIMARY)
                {
                    remote_cmd_relay(app_db.relay_handle, APP_REMOTE_MSG_MMI_SYNC, &cmd_ptr[3], 1,
                                     REMOTE_TIMER_HIGH_PRECISION, DEFAULT_SYNC_TIMEOUT_MS, false);
                }
                else
                {
                    remote_cmd_relay(app_db.relay_handle, APP_REMOTE_MSG_MMI_SYNC, &cmd_ptr[3], 1,
                                     REMOTE_TIMER_HIGH_PRECISION, DEFAULT_SYNC_TIMEOUT_MS, true);
                }

            }
            else
            {
                remote_cmd_relay(app_db.relay_handle, APP_REMOTE_MSG_MMI_SYNC, &cmd_ptr[3], 1,
                                 REMOTE_TIMER_NONE, 0, false);
                app_mmi_handle_action(cmd_ptr[3]);
            }
        }
        break;

    case CMD_LEGACY_DATA_TRANSFER:
        {
            uint16_t total_len;
            uint16_t pkt_len;
            uint8_t app_index;
            uint8_t pkt_type;
            uint8_t *pkt_ptr;

            app_index = cmd_ptr[2];
            pkt_type = cmd_ptr[3];
            total_len = (cmd_ptr[4] | (cmd_ptr[5] << 8));
            pkt_len = (cmd_ptr[6] | (cmd_ptr[7] << 8));
            pkt_ptr = &cmd_ptr[8];
            if ((app_db.br_link[app_index].connected_profile & SPP_PROFILE_MASK) ||
                (app_db.br_link[app_index].connected_profile & IAP_PROFILE_MASK))
            {
                if (cmd_path == CMD_PATH_UART)
                {
                    if (pkt_len)
                    {
                        if ((pkt_type == PKT_TYPE_SINGLE) || (pkt_type == PKT_TYPE_START))
                        {
                            if (app_transfer_check_active() == 0)
                            {
                                if (app_db.br_link[app_index].uart_rx_dt_pkt_ptr)
                                {
                                    os_mem_free(app_db.br_link[app_index].uart_rx_dt_pkt_ptr);
                                }
                                app_db.br_link[app_index].uart_rx_dt_pkt_ptr = os_mem_alloc(RAM_TYPE_DATA_ON,
                                                                                            total_len);
                                memcpy(app_db.br_link[app_index].uart_rx_dt_pkt_ptr, pkt_ptr, pkt_len);
                                app_db.br_link[app_index].uart_rx_dt_pkt_len = pkt_len;
                            }
                            else
                            {
                                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                            }
                        }
                        else
                        {
                            if (app_db.br_link[app_index].uart_rx_dt_pkt_ptr)
                            {
                                uint8_t *temp_ptr;

                                temp_ptr = app_db.br_link[app_index].uart_rx_dt_pkt_ptr +
                                           app_db.br_link[app_index].uart_rx_dt_pkt_len;
                                memcpy(temp_ptr, pkt_ptr, pkt_len);
                                app_db.br_link[app_index].uart_rx_dt_pkt_len += pkt_len;
                            }
                            else//maybe start packet been lost
                            {
                                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                            }
                        }

                        if ((pkt_type == PKT_TYPE_SINGLE) || (pkt_type == PKT_TYPE_END))
                        {
                            if (ack_pkt[2] == CMD_SET_STATUS_COMPLETE)
                            {
                                if (app_push_data_transfer_queue(DT_PATH_SPP, app_index,
                                                                 app_db.br_link[app_index].uart_rx_dt_pkt_ptr,
                                                                 app_db.br_link[app_index].uart_rx_dt_pkt_len) == false)
                                {
                                    os_mem_free(app_db.br_link[app_index].uart_rx_dt_pkt_ptr);
                                }
                                app_db.br_link[app_index].uart_rx_dt_pkt_ptr = NULL;
                                if (app_transfer_check_active() != 0)
                                {
                                    ack_pkt[2] = CMD_SET_STATUS_BUSY;
                                    app_db.br_link[app_index].resume_fg = 0x01;
                                }
                            }
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_ASSIGN_BUFFER_SIZE:
        {
            app_db.external_mcu_mtu = (cmd_ptr[4] | (cmd_ptr[5] << 8));
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_TONE_GEN:
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_BT_IAP_LAUNCH_APP:
        {
            uint8_t app_index;

            app_index = cmd_ptr[2];
            if (app_index < MAX_BR_LINK_NUM)
            {
                if ((app_db.br_link[app_index].connected_profile & IAP_PROFILE_MASK)
                    && (app_db.br_link[app_index].iap_authen_flag))
                {
                    char boundle_id[] = "com.realtek.EADemo2";
                    BT_IAP_APP_LAUNCH_METHOD method = BT_IAP_APP_LAUNCH_WITH_USER_ALERT;
                    bt_iap_app_launch(app_db.br_link[app_index].bd_addr, boundle_id, sizeof(boundle_id), method);
                }
                else
                {
                    ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_AUDIO_EQ_QUERY:
        {
            uint8_t query_type;
            uint8_t buf[2];

            query_type = cmd_ptr[2];

            if (query_type == AUDIO_EQ_QUERY_STATE)
            {
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                buf[0] = query_type;
                buf[1] = app_db.eq_enable;
            }
            else if (query_type == AUDIO_EQ_QUERY_NUM)
            {
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                buf[0] = query_type;
                buf[1] = eq_num_get();
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                break;
            }
            if (cmd_path == CMD_PATH_SPP)
            {
                app_report_spp_event(&app_db.br_link[app_idx], EVENT_AUDIO_EQ_REPLY, buf, 2);
            }
            else if (cmd_path == CMD_PATH_LE)
            {
                app_report_le_event(&app_db.le_link[app_idx], EVENT_AUDIO_EQ_REPLY, buf, 2);
            }
        }
        break;

    case CMD_AUDIO_EQ_ENABLE:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if (app_db.eq_enable == false)
            {
                app_db.eq_enable = true;
                eq_enable();
                eq_index_set(app_cfg_nv.eq_index);
            }
        }
        break;

    case CMD_AUDIO_EQ_DISABLE:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if (app_db.eq_enable == true)
            {
                app_db.eq_enable = false;
                eq_disable();
            }
        }
        break;

    case CMD_AUDIO_EQ_PARAM_SET:
        {
            uint8_t eq_index;
            uint8_t type;
            uint8_t seq;
            uint16_t eq_len;
            uint16_t frame_len;

            eq_index = cmd_ptr[2];
            type = cmd_ptr[3];
            seq = cmd_ptr[4];

            if (type == AUDIO_EQ_FRAME_SINGLE)
            {
                frame_len = (uint16_t)(cmd_ptr[5] | cmd_ptr[6] << 8);

                if (seq != EQ_INIT_SEQ)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    break;
                }

                if (cmd_path == CMD_PATH_SPP)
                {
                    if (eq_param_set(eq_index, &cmd_ptr[7], frame_len) == true)
                    {
                        if (app_cfg_nv.bud_role == DEVICE_ROLE_PRIMARY)
                        {
                            remote_cmd_relay(app_db.relay_handle, APP_REMOTE_MSG_EQ_DATA, &cmd_ptr[7], frame_len,
                                             REMOTE_TIMER_HIGH_PRECISION, 0, false);
                        }
                        /* setting eq param will update active eq index */
                        app_cfg_nv.eq_index = eq_index;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    }
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    app_db.le_link[app_idx].eq_state = AUDIO_EQ_FRAME_SINGLE;

                    if (eq_param_set(eq_index, &cmd_ptr[7], frame_len) == true)
                    {
                        if (app_cfg_nv.bud_role == DEVICE_ROLE_PRIMARY)
                        {
                            remote_cmd_relay(app_db.relay_handle, APP_REMOTE_MSG_EQ_DATA, &cmd_ptr[7], frame_len,
                                             REMOTE_TIMER_HIGH_PRECISION, 0, false);
                        }                        /* setting eq param will update active eq index */
                        app_cfg_nv.eq_index = eq_index;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    }
                }
            }
            else if (type == AUDIO_EQ_FRAME_START)
            {
                eq_len  = (uint16_t)(cmd_ptr[5] | cmd_ptr[6] << 8);
                frame_len = (uint16_t)(cmd_ptr[7] | cmd_ptr[8] << 8);

                if (seq != EQ_INIT_SEQ)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    break;
                }

                if (cmd_path == CMD_PATH_SPP)
                {

                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    T_APP_LE_LINK *p_link;

                    p_link = &app_db.le_link[app_idx];
                    p_link->eq_data_offset = 0;
                    p_link->eq_state = AUDIO_EQ_FRAME_START;
                    p_link->eq_seq = EQ_INIT_SEQ;
                    p_link->eq_data_len = eq_len;

                    if (p_link->eq_data_buf != NULL)
                    {
                        os_mem_free(p_link->eq_data_buf);
                    }

                    p_link->eq_data_buf = os_mem_alloc(RAM_TYPE_DATA_ON, p_link->eq_data_len);

                    if (p_link->eq_data_buf != NULL)
                    {
                        memcpy(p_link->eq_data_buf, &cmd_ptr[9], frame_len);
                        p_link->eq_data_offset += frame_len;
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
            else if (type == AUDIO_EQ_FRAME_CONTINUE)
            {
                frame_len = (uint16_t)(cmd_ptr[5] | cmd_ptr[6] << 8);

                if (cmd_path == CMD_PATH_SPP)
                {

                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    T_APP_LE_LINK *p_link;

                    p_link = &app_db.le_link[app_idx];

                    if (seq == p_link->eq_seq + 1)
                    {
                        p_link->eq_seq = seq;
                        p_link->eq_state = AUDIO_EQ_FRAME_CONTINUE;
                        memcpy(p_link->eq_data_buf + p_link->eq_data_offset, &cmd_ptr[7], frame_len);
                        p_link->eq_data_offset += frame_len;
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
            else if (type == AUDIO_EQ_FRAME_END)
            {
                frame_len = (uint16_t)(cmd_ptr[5] | cmd_ptr[6] << 8);

                if (cmd_path == CMD_PATH_SPP)
                {

                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    T_APP_LE_LINK *p_link;

                    p_link = &app_db.le_link[app_idx];
                    if (seq == p_link->eq_seq + 1)
                    {
                        p_link->eq_seq = seq;
                        p_link->eq_state = AUDIO_EQ_FRAME_END;
                        memcpy(p_link->eq_data_buf + p_link-> eq_data_offset, &cmd_ptr[7], frame_len);
                        p_link->eq_data_offset += frame_len;

                        if (eq_param_set(eq_index, p_link->eq_data_buf, p_link->eq_data_len) == true)
                        {
                            if (app_cfg_nv.bud_role == DEVICE_ROLE_PRIMARY)
                            {
                                remote_cmd_relay(app_db.relay_handle, APP_REMOTE_MSG_EQ_DATA, &cmd_ptr[7], frame_len,
                                                 REMOTE_TIMER_HIGH_PRECISION, 0, false);
                            }
                            /* setting eq param will update active eq index */
                            app_cfg_nv.eq_index = eq_index;
                            os_mem_free(p_link->eq_data_buf);
                            p_link->eq_data_buf = NULL;
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
        }
        break;

    case CMD_AUDIO_EQ_PARAM_GET:
        {
            uint16_t max_frame_len;

            if (cmd_path == CMD_PATH_SPP)
            {
                T_APP_BR_LINK *p_link;

                p_link = &app_db.br_link[app_idx];

                if (p_link->eq_data_buf != NULL)
                {
                    os_mem_free(p_link->eq_data_buf);
                }

                p_link->eq_data_buf = os_mem_alloc(RAM_TYPE_DATA_ON, eq_param_len_get());

                if (p_link->eq_data_buf == NULL)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    break;
                }
                else
                {
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                    p_link->eq_index = cmd_ptr[2];
                    p_link->eq_type = AUDIO_EQ_FRAME_SINGLE;
                    p_link->eq_seq = EQ_INIT_SEQ;
                    p_link->eq_data_len = eq_param_get(p_link->eq_index, p_link->eq_data_buf, eq_param_len_get());

                    app_eq_report_single_frame(p_link, CMD_PATH_SPP);
                }
            }
            else if (cmd_path == CMD_PATH_LE)
            {
                T_APP_LE_LINK *p_link;

                p_link = &app_db.le_link[app_idx];

                if (p_link->eq_data_buf != NULL)
                {
                    os_mem_free(p_link->eq_data_buf);
                }

                p_link->eq_data_buf = os_mem_alloc(RAM_TYPE_DATA_ON, eq_param_len_get());

                if (p_link->eq_data_buf == NULL)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    break;
                }
                else
                {
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                    p_link->eq_index = cmd_ptr[2];
                    p_link->eq_seq = EQ_INIT_SEQ;
                    p_link->eq_data_len = eq_param_get(p_link->eq_index, p_link->eq_data_buf,
                                                       eq_param_len_get());
                    if (p_link->eq_data_len != 0)
                    {
                        p_link->eq_data_offset = 0;
                        /*EQ: 3 is att header,  6 is trasmint service header*/
                        max_frame_len = p_link->mtu_size - 3 - EQ_SINGLE_FRAME_HEADER_LEN - 6 ;
                        if (p_link->eq_data_len <= max_frame_len)
                        {
                            app_eq_report_single_frame(p_link, CMD_PATH_LE);
                        }
                        else
                        {
                            app_eq_report_start_frame(p_link, CMD_PATH_LE);
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                        if (p_link->eq_data_buf != NULL)
                        {
                            os_mem_free(p_link->eq_data_buf);
                            p_link->eq_data_buf = NULL;
                        }
                    }
                }
            }
        }
        break;

    case CMD_AUDIO_EQ_INDEX_SET:
        {
            uint8_t eq_index;

            eq_index = cmd_ptr[2];

            if (eq_index_set(eq_index) == true)
            {
                app_cfg_nv.eq_index = eq_index;
                app_roleswap_event_info_remote(ROLESWAP_EVENT_SYNC_EQ_INDEX, &app_cfg_nv.eq_index, 1);
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            }
        }
        break;

    case CMD_AUDIO_EQ_INDEX_GET:
        {
            uint8_t buf[1];

            buf[0] = app_cfg_nv.eq_index;

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            if (cmd_path == CMD_PATH_SPP)
            {
                app_report_spp_event(&app_db.br_link[app_idx], EVENT_AUDIO_EQ_INDEX_REPORT, buf, 1);
            }
            else if (cmd_path == CMD_PATH_LE)
            {
                app_report_le_event(&app_db.le_link[app_idx], EVENT_AUDIO_EQ_INDEX_REPORT, buf, 1);
            }
        }
        break;

    case CMD_TTS:
        {
            uint8_t   type;
            uint8_t   seq;
            uint16_t  len;
            uint16_t  frame_len;

            type = cmd_ptr[2];
            seq  = cmd_ptr[3];
            len  = (uint16_t)(cmd_ptr[4] | (cmd_ptr[5] << 8)); /* little endian */

            if (type == TTS_SESSION_OPEN)
            {
                uint8_t   sync_byte;
                uint32_t  sess_len;
                uint16_t  frame_num;
                uint32_t  tts_cfg;
                void     *tts_handle;

                sync_byte = cmd_ptr[6]; /* big endian */
                sess_len = (uint32_t)((cmd_ptr[7] << 16) | (cmd_ptr[8] << 8) | cmd_ptr[9]); /* big endian */
                frame_num = (uint16_t)((cmd_ptr[10] << 8) | cmd_ptr[11]); /* big endian */
                tts_cfg = (uint32_t)((cmd_ptr[12] << 24) | (cmd_ptr[13] << 16) |
                                     (cmd_ptr[14] << 8) | cmd_ptr[15]); /* big endian */

                if (sync_byte != TTS_SYNC_BYTE ||
                    seq       != TTS_INIT_SEQ  ||
                    len       != TTS_HEADER_LEN)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    break;
                }

                app_audio_set_tts_path(cmd_path);

                if (cmd_path == CMD_PATH_SPP)
                {
                    if (app_db.br_link[app_idx].tts_handle != NULL)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                        break;
                    }

                    if (app_db.br_link[app_idx].call_status == APP_HFP_CALL_INCOMING)
                    {
                        tts_handle = tts_create(sess_len - TTS_HEADER_LEN, frame_num, tts_cfg);
                        if (tts_handle != NULL)
                        {
                            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                            app_db.br_link[app_idx].tts_handle = tts_handle;
                            app_db.br_link[app_idx].tts_state  = TTS_SESSION_OPEN;
                            tts_start(tts_handle, false, false);
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    }
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    if (app_db.le_link[app_idx].tts_handle != NULL)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                        break;
                    }

                    tts_handle = tts_create(sess_len - TTS_HEADER_LEN, frame_num, tts_cfg);
                    if (tts_handle != NULL)
                    {
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                        app_db.le_link[app_idx].tts_handle = tts_handle;
                        app_db.le_link[app_idx].tts_state  = TTS_SESSION_OPEN;
                        tts_start(app_db.le_link[app_idx].tts_handle, false, false);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    }
                }
            }
            else if (type == TTS_SESSION_SEND_SINGLE_FRAME)
            {
                frame_len = (uint16_t)(cmd_ptr[6] << 8 | cmd_ptr[7]); /* big endian */

                if (seq != TTS_INIT_SEQ ||
                    len != frame_len + TTS_FRAME_LEN)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    break;
                }

                if (cmd_path == CMD_PATH_SPP)
                {
                    if (app_db.br_link[app_idx].tts_handle != NULL)
                    {
                        app_db.br_link[app_idx].tts_state = TTS_SESSION_SEND_SINGLE_FRAME;
                        tts_play(app_db.br_link[app_idx].tts_handle, &cmd_ptr[6], len);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    if (app_db.le_link[app_idx].tts_handle != NULL)
                    {
                        app_db.le_link[app_idx].tts_state = TTS_SESSION_SEND_SINGLE_FRAME;
                        tts_play(app_db.le_link[app_idx].tts_handle, &cmd_ptr[6], len);

                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }

                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            }
            else if (type == TTS_SESSION_SEND_START_FRAME)
            {
                frame_len = (uint16_t)(cmd_ptr[6] << 8 | cmd_ptr[7]); /* big endian */

                if (seq != TTS_INIT_SEQ)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    break;
                }

                if (cmd_path == CMD_PATH_SPP)
                {
                    T_APP_BR_LINK *p_link;

                    p_link = &app_db.br_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        p_link->tts_data_offset = 0;
                        p_link->tts_state = TTS_SESSION_SEND_START_FRAME;
                        p_link->tts_seq = TTS_INIT_SEQ;
                        p_link->tts_frame_len = frame_len + TTS_FRAME_LEN;

                        if (p_link->tts_frame_buf != NULL)
                        {
                            os_mem_free(p_link->tts_frame_buf);
                        }

                        p_link->tts_frame_buf = os_mem_alloc(RAM_TYPE_DATA_ON, frame_len + TTS_FRAME_LEN);
                        if (p_link->tts_frame_buf != NULL)
                        {
                            memcpy(p_link->tts_frame_buf, &cmd_ptr[6], len);
                            p_link->tts_data_offset += len;
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    T_APP_LE_LINK *p_link;

                    p_link = &app_db.le_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        p_link->tts_data_offset = 0;
                        p_link->tts_state = TTS_SESSION_SEND_START_FRAME;
                        p_link->tts_seq = TTS_INIT_SEQ;
                        p_link->tts_frame_len = frame_len + TTS_FRAME_LEN;

                        if (p_link->tts_frame_buf != NULL)
                        {
                            os_mem_free(p_link->tts_frame_buf);
                        }

                        p_link->tts_frame_buf = os_mem_alloc(RAM_TYPE_DATA_ON, frame_len + TTS_FRAME_LEN);
                        if (p_link->tts_frame_buf != NULL)
                        {
                            memcpy(p_link->tts_frame_buf, &cmd_ptr[6], len);
                            p_link->tts_data_offset += len;
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
            else if (type == TTS_SESSION_SEND_CONTINUE_FRAME)
            {
                if (cmd_path == CMD_PATH_SPP)
                {
                    T_APP_BR_LINK *p_link;

                    p_link = &app_db.br_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        if (seq == p_link->tts_seq + 1)
                        {
                            p_link->tts_seq = seq;
                            p_link->tts_state = TTS_SESSION_SEND_CONTINUE_FRAME;
                            memcpy(p_link->tts_frame_buf + p_link->tts_data_offset, &cmd_ptr[6], len);
                            p_link->tts_data_offset += len;
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    T_APP_LE_LINK *p_link;

                    p_link = &app_db.le_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        if (seq == p_link->tts_seq + 1)
                        {
                            p_link->tts_seq = seq;
                            p_link->tts_state = TTS_SESSION_SEND_CONTINUE_FRAME;
                            memcpy(p_link->tts_frame_buf + p_link->tts_data_offset, &cmd_ptr[6], len);
                            p_link->tts_data_offset += len;
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
            else if (type == TTS_SESSION_SEND_END_FRAME)
            {
                if (cmd_path == CMD_PATH_SPP)
                {
                    T_APP_BR_LINK *p_link;

                    p_link = &app_db.br_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        if (seq == p_link->tts_seq + 1)
                        {
                            p_link->tts_seq = seq;
                            p_link->tts_state = TTS_SESSION_SEND_END_FRAME;
                            memcpy(p_link->tts_frame_buf + p_link->tts_data_offset, &cmd_ptr[6], len);
                            p_link->tts_data_offset = 0;
                            tts_play(p_link->tts_handle, p_link->tts_frame_buf, p_link->tts_frame_len);
                            os_mem_free(p_link->tts_frame_buf);
                            p_link->tts_frame_buf = NULL;
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    T_APP_LE_LINK *p_link;

                    p_link = &app_db.le_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        if (seq == p_link->tts_seq + 1)
                        {
                            p_link->tts_seq = seq;
                            p_link->tts_state = TTS_SESSION_SEND_END_FRAME;
                            memcpy(p_link->tts_frame_buf + p_link->tts_data_offset, &cmd_ptr[6], len);
                            p_link->tts_data_offset = 0;
                            tts_play(p_link->tts_handle, p_link->tts_frame_buf, p_link->tts_frame_len);
                            os_mem_free(p_link->tts_frame_buf);
                            p_link->tts_frame_buf = NULL;
                        }
                        else
                        {
                            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                        }
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
            else if (type == TTS_SESSION_CLOSE)
            {
                if (seq != TTS_INIT_SEQ  ||
                    len != TTS_FRAME_LEN)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    break;
                }

                if (cmd_path == CMD_PATH_SPP)
                {
                    T_APP_BR_LINK *p_link;

                    p_link = &app_db.br_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        p_link->tts_state = TTS_SESSION_CLOSE;
                        tts_play(p_link->tts_handle, &cmd_ptr[6], len);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    T_APP_LE_LINK *p_link;

                    p_link = &app_db.le_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        p_link->tts_state = TTS_SESSION_CLOSE;
                        tts_play(p_link->tts_handle, &cmd_ptr[6], len);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
            else if (type == TTS_SESSION_ABORT)
            {
                if (seq != TTS_INIT_SEQ)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    break;
                }

                if (cmd_path == CMD_PATH_SPP)
                {
                    T_APP_BR_LINK *p_link;

                    p_link = &app_db.br_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        p_link->tts_state = TTS_SESSION_ABORT;
                        tts_stop(p_link->tts_handle);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    T_APP_LE_LINK *p_link;

                    p_link = &app_db.le_link[app_idx];
                    if (p_link->tts_handle != NULL)
                    {
                        p_link->tts_state = TTS_SESSION_ABORT;
                        tts_stop(p_link->tts_handle);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            }
        }
        break;

    case CMD_GET_BD_ADDR:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            if (cmd_path == CMD_PATH_UART)
            {
                app_report_uart_event(EVENT_GET_BD_ADDR, app_db.factory_addr,
                                      sizeof(app_db.factory_addr));
            }
            else if (cmd_path == CMD_PATH_LE)
            {
                app_report_le_event(&app_db.le_link[app_idx], EVENT_GET_BD_ADDR,
                                    app_db.factory_addr, sizeof(app_db.factory_addr));
            }
            else if (cmd_path == CMD_PATH_SPP)
            {
                app_report_spp_event(&app_db.br_link[app_idx], EVENT_GET_BD_ADDR,
                                     app_db.factory_addr, sizeof(app_db.factory_addr));
            }
        }
        break;

    case CMD_INFO_REQ:
        {
            uint8_t info_type;
            uint8_t buf[4];

            info_type = cmd_ptr[2];

            if (info_type == CMD_SET_INFO_TYPE_VERSION)
            {
                ack_pkt[2] = CMD_SET_STATUS_COMPLETE;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if (info_type == CMD_SET_INFO_TYPE_VERSION)
            {
                buf[0] = info_type;
                buf[1] = 0;
                buf[2] = CMD_SET_VER_MAJOR;
                buf[3] = CMD_SET_VER_MINOR;

                if (cmd_path == CMD_PATH_SPP)
                {
                    app_db.br_link[app_idx].cmd_set_enable = true;
                    app_report_spp_event(&app_db.br_link[app_idx], EVENT_INFO_RSP, buf, 4);
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    app_db.le_link[app_idx].cmd_set_enable = true;
                    app_report_le_event(&app_db.le_link[app_idx], EVENT_INFO_RSP, buf, 4);
                }
            }
        }
        break;

    case CMD_BT_SEND_AT_CMD:
        {
            uint8_t app_index;

            app_index = cmd_ptr[2];
            if (bt_hfp_send_at_cmd_req(app_db.br_link[app_index].bd_addr, (char *)&cmd_ptr[3]) == false)
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_SET_CONFIGURATION:
        if (cmd_ptr[2] == CFG_TYPE_LE_NAME)
        {
            le_set_gap_param(GAP_PARAM_DEVICE_NAME, cmd_ptr[3], &cmd_ptr[4]);
            if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_LE))
            {
                uint8_t name_len;

                name_len = cmd_ptr[3];
                if (name_len >= GAP_DEVICE_NAME_LEN)
                {
                    name_len = GAP_DEVICE_NAME_LEN - 1;
                }
                memcpy(app_cfg_nv.device_name_le, &cmd_ptr[4], name_len);
                app_cfg_nv.device_name_le[name_len] = 0;
            }
        }
        else if (cmd_ptr[2] == CFG_TYPE_LEGACY_NAME)
        {
            gap_br_set_dev_name(&cmd_ptr[4], cmd_ptr[3]);
            if ((cmd_path == CMD_PATH_SPP) || (cmd_path == CMD_PATH_LE))
            {
                uint8_t name_len;

                name_len = cmd_ptr[3];
                if (name_len >= GAP_DEVICE_NAME_LEN)
                {
                    name_len = GAP_DEVICE_NAME_LEN - 1;
                }
                memcpy(app_cfg_nv.device_name_legacy, &cmd_ptr[4], name_len);
                app_cfg_nv.device_name_legacy[name_len] = 0;
            }
        }
        else if (cmd_ptr[2] == CFG_TYPE_AUDIO_LATENCY)
        {
        }
        else if (cmd_ptr[2] == CFG_TYPE_SUPPORT_CODEC)
        {
        }
        else
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
        }

        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        break;

    case CMD_GET_CONFIGURATION:
        {
            uint8_t get_type = cmd_ptr[2];

            if (get_type >= CFG_TYPE_MAX)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if ((get_type == CFG_TYPE_LE_NAME) || (get_type == CFG_TYPE_LEGACY_NAME))
            {
                uint8_t p_name[40 + 2];
                uint8_t *p_buf;
                uint8_t name_len;

                if (get_type == CFG_TYPE_LEGACY_NAME)
                {
                    name_len = strlen((const char *)app_cfg_nv.device_name_legacy);
                    p_buf = app_cfg_nv.device_name_legacy;
                }
                else
                {
                    name_len = strlen((const char *)app_cfg_nv.device_name_le);
                    p_buf = app_cfg_nv.device_name_le;
                }
                p_name[0] = get_type;
                p_name[1] = name_len;
                memcpy(&p_name[2], p_buf, name_len);

                if (cmd_path == CMD_PATH_SPP)
                {
                    app_report_spp_event(&app_db.br_link[app_idx], EVENT_REPORT_CFG_TYPE, &p_name[0], name_len + 2);
                }
                else if (cmd_path == CMD_PATH_LE)
                {
                    app_report_le_event(&app_db.le_link[app_idx], EVENT_REPORT_CFG_TYPE, &p_name[0], name_len + 2);
                }
            }
            else if (cmd_ptr[2] == CFG_TYPE_AUDIO_LATENCY)
            {
            }
            else if (cmd_ptr[2] == CFG_TYPE_SUPPORT_CODEC)
            {
            }
        }
        break;

    case CMD_INDICATION:
        if (cmd_ptr[2] == 0)//report MAC address of smart phone
        {
            memcpy(app_db.le_link[app_idx].bd_addr, &cmd_ptr[3], 6);
        }
        else
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
        }

        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        break;

    case CMD_LANGUAGE_GET:
        {
            uint8_t buf[2];

            buf[0] = (uint8_t)app_db.vp_language;
            buf[1] = voice_prompt_supported_languages_get();

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if (cmd_path == CMD_PATH_SPP)
            {
                app_report_spp_event(&app_db.br_link[app_idx], EVENT_LANGUAGE_REPORT, buf, 2);
            }
            else if (cmd_path == CMD_PATH_LE)
            {
                app_report_le_event(&app_db.le_link[app_idx], EVENT_LANGUAGE_REPORT, buf, 2);
            }
        }
        break;

    case CMD_LANGUAGE_SET:
        if (voice_prompt_language_set((T_VOICE_PROMPT_LANGUAGE_ID)cmd_ptr[2]) == true)
        {
            app_db.vp_language = (T_VOICE_PROMPT_LANGUAGE_ID)cmd_ptr[2];
        }
        else
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
        }

        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        break;

    case CMD_GET_STATUS:
        {
            uint8_t buf[3];

            buf[0] = cmd_ptr[2]; //status_index
            switch (cmd_ptr[2])
            {
            case GET_STATUS_RWS_STATE:
                buf[1] = app_db.remote_session_state;
                break;

            case GET_STATUS_RWS_CHANNEL:
                //buf[1] = sys_cfg_nv.rws_current_channel;
                break;

            case GET_STATUS_BATTERY_STATUS:
                buf[1] = app_db.local_batt_level;
                buf[2] = app_db.remote_batt_level;
                break;

            case GET_STATUS_AMB_STATUS:
                if (app_db.current_anc_amb_state == ANC_OFF_AMB_ON)
                {
                    buf[1] = 1;
                }
                else
                {
                    buf[1] = 0;
                }
                break;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_event(cmd_path, EVENT_REPORT_STATUS, app_idx, buf, 3);
        }
        break;

    case CMD_LE_START_ADVERTISING:
        if (cmd_ptr[1] <= 31)
        {
            //app_ble_gap_start_advertising(APP_ADV_PURPOSE_VENDOR, cmd_ptr[0], cmd_ptr[1], &cmd_ptr[2]);
            //fixme later
        }
        else
        {
            ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
        }
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        break;

    case CMD_LE_START_SCAN:
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        le_scan_start();
        break;

    case CMD_LE_STOP_SCAN:
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        le_scan_stop();
        break;

    case CMD_ANCS_REGISTER:
        {
            uint8_t le_index;

            le_index = cmd_ptr[2];

            if (app_db.le_link[le_index].state == LE_LINK_STATE_CONNECTED)
            {
                if (ancs_start_discovery(app_db.le_link[le_index].conn_id) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_ANCS_GET_NOTIFICATION_ATTR:
        {
            uint8_t le_index;
            uint32_t  notification_uid;

            le_index = cmd_ptr[2];
            notification_uid = *((uint32_t *)&cmd_ptr[3]);

            if (app_db.le_link[le_index].state == LE_LINK_STATE_CONNECTED)
            {
                if (ancs_get_notification_attr(app_db.le_link[le_index].conn_id, notification_uid,
                                               &cmd_ptr[8],
                                               cmd_ptr[7]) == false)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        }
        break;

    case CMD_STRING_MODE:
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        console_set_mode(CONSOLE_MODE_STRING);
        break;

    case CMD_BT_GET_LOCAL_ADDR:
        {
            uint8_t temp_buff[6];
            memcpy(&temp_buff[0], app_cfg_nv.bud_local_addr, 6);
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_uart_event(EVENT_LOCAL_ADDR, temp_buff, sizeof(temp_buff));
        }
        break;

    case CMD_BT_BOND_INFO_CLEAR:
        {
            uint8_t temp_buff[1];
            temp_buff[0] = 1;

            if (cmd_ptr[2] == 0) //clear BR/EDR bond info
            {
                uint8_t bd_addr[6];
                memcpy(bd_addr, &cmd_ptr[3], 6);
                if (bt_bond_delete(bd_addr) == true)
                {
                    temp_buff[0] = 0;  //success
                }
            }

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_report_uart_event(EVENT_BT_BOND_INFO_CLEAR, temp_buff, sizeof(temp_buff));
        }
        break;

    case CMD_MP_TEST:
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        mp_test_handle_cmd(app_idx, cmd_path, cmd_ptr[2], cmd_ptr[3], &cmd_ptr[4], cmd_len - 4);
        break;

    case CMD_OTA_DEV_INFO :
    case CMD_OTA_ACTIVE_BANK_VER:
    case CMD_OTA_INACTIVE_BANK_VER:
    case CMD_OTA_START:
    case CMD_OTA_PACKET:
    case CMD_OTA_VALID:
    case CMD_OTA_RESET:
    case CMD_OTA_ACTIVE_RESET:
    case CMD_OTA_BUFFER_CHECK_ENABLE:
    case CMD_OTA_BUFFER_CHECK:
    case CMD_OTA_IMG_INFO:
    case CMD_OTA_SECTION_SIZE:
    case CMD_OTA_DEV_EXTRA_INFO:
    case CMD_OTA_PROTOCOL_TYPE:
    case CMD_OTA_GET_RELEASE_VER:
    case CMD_OTA_COPY_IMG:
    case CMD_OTA_CHECK_SHA256:
        {
            app_ota_cmd_handle(cmd_len, cmd_ptr, app_idx);
        }
        break;

    case CMD_HCI:
        {
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            uint8_t *anc_tool_cmd_ptr;
            anc_tool_cmd_ptr = malloc(cmd_ptr[4] + 5);
            anc_tool_cmd_ptr[0] = cmd_path;
            anc_tool_cmd_ptr[1] = app_idx;
            memcpy(&anc_tool_cmd_ptr[2], &cmd_ptr[2], cmd_ptr[4] + 3);
            app_anc_handle_anc_tool_cmd(anc_tool_cmd_ptr);
            free(anc_tool_cmd_ptr);
        }
        break;

    case CMD_WDG_RESET:
        {
            uint8_t wdg_status = 0x00;

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if (cmd_path == CMD_PATH_SPP)
            {
                app_report_spp_event(&app_db.br_link[app_idx], EVENT_WDG_RESET, &wdg_status, 1);
            }
            else if (cmd_path == CMD_PATH_UART)
            {
                app_report_uart_event(EVENT_WDG_RESET, &wdg_status, 1);
            }

            app_anc_ini_wdg_reset(cmd_ptr[2]);
        }
        break;

    //for ANC command
    case CMD_ANC_READ_REGISTER:
    case CMD_ANC_WRITE_REGISTER:
    case CMD_ANC_TEST_MODE:
    case CMD_ANC_WRITE_GAIN:
    case CMD_ANC_READ_GAIN:
    case CMD_ANC_BURN_GAIN:
    case CMD_ANC_COMPARE:
    case CMD_ANC_GEN_TONE:
    case CMD_ANC_CONFIG_DATA_LOG:
    case CMD_ANC_READ_DATA_LOG:
        {
            uint16_t anc_cmd_len = cmd_len - 2;
            app_anc_operate(cmd_id, &cmd_ptr[2], anc_cmd_len, cmd_path, app_idx);
        }
        break;

    case CMD_GET_FLASH_DATA:
        {
            switch (cmd_ptr[2])
            {
            case START_TRANS:
                {
                    if ((0x01 << cmd_ptr[3]) & ALL_DUMP_IAMGE_MASK)
                    {
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                        app_flash_data_set_param(cmd_ptr[3], cmd_path, app_idx);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    }
                }
                break;

            case CONTINUE_TRANS:
                {
                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                    app_read_flash(flash_data.flash_data_start_addr_tmp, cmd_path, app_idx);
                }
                break;

            case SUPPORT_IAMGE_TYPE:
                {
                    uint8_t paras[5];

                    paras[0] = SUPPORT_IAMGE_TYPE_INFO;
                    paras[1] = (uint8_t)(ALL_DUMP_IAMGE_MASK);
                    paras[2] = (uint8_t)(ALL_DUMP_IAMGE_MASK >> 8);
                    paras[3] = (uint8_t)(ALL_DUMP_IAMGE_MASK >> 16);
                    paras[4] = (uint8_t)(ALL_DUMP_IAMGE_MASK >> 24);

                    app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

                    if (cmd_path == CMD_PATH_UART)
                    {
                        app_report_uart_event(EVENT_REPORT_FLASH_DATA, paras, sizeof(paras));

                    }
                    else if (cmd_path == CMD_PATH_LE)
                    {
                        app_report_le_event(&app_db.le_link[app_idx], EVENT_REPORT_FLASH_DATA, paras, sizeof(paras));

                    }
                    else if (cmd_path == CMD_PATH_SPP)
                    {
                        app_report_spp_event(&app_db.br_link[app_idx], EVENT_REPORT_FLASH_DATA, paras, sizeof(paras));
                    }
                }
                break;

            default:
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
                break;
            }
        }
        break;

    case CMD_MIC_SWITCH:
        {
            uint8_t param = app_audio_mic_switch(cmd_ptr[2]);

            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);

            if (cmd_path == CMD_PATH_SPP)
            {
                app_report_spp_event(&app_db.br_link[app_idx], EVENT_MIC_SWITCH, &param, 1);
            }
        }
        break;

    case CMD_SWITCH_TO_HCI_DOWNLOAD_MODE:
        {
            //if uart tx shares the same pin with 3pin gpio, set uart tx pin when receive cmd
            if (app_cfg_const.gpio_box_detect_pinmux == app_cfg_const.data_uart_tx_pinmux)
            {
                Pinmux_Config(app_cfg_const.data_uart_tx_pinmux, UART0_TX);
                Pad_Config(app_cfg_const.data_uart_tx_pinmux,
                           PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
            }
            app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
            app_start_timer(&timer_idx_switch_to_hci_mode, "switch_to_hci_mode",
                            app_cmd_timer_id, APP_TIMER_SWITCH_TO_HCI_DOWNLOAD_MODE, app_idx, false,
                            100);
        }
        break;

    default:
        ack_pkt[2] = CMD_SET_STATUS_UNKNOW_CMD;
        app_cmd_set_event_ack(cmd_path, app_idx, ack_pkt);
        break;
    }

    APP_PRINT_TRACE5("app_handle_cmd_set: cmd_id 0x%04x, cmd_len 0x%04x, cmd_path %u, rx_seqn 0x%02x, ack_status 0x%02x",
                     cmd_id, cmd_len, cmd_path, rx_seqn, ack_pkt[2]);
}
