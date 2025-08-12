/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "os_mem.h"
#include "console.h"
#include "app_util.h"
#include "app_cmd.h"
#include "app_main.h"
#include "app_link_util.h"
#include "app_transfer.h"
#include "app_report.h"
#include "app_transfer.h"
#include "app_ble_gap.h"
#include "app_cfg.h"
#if F_APP_APT_SUPPORT
#include "app_audio_passthrough.h"
#endif
#if F_APP_ANC_SUPPORT
#include "app_anc.h"
#endif
#include "eq_utils.h"
#include "app_eq.h"

#if F_APP_ONE_WIRE_UART_SUPPORT
#include "app_one_wire_uart.h"
#endif

#if F_APP_GAMING_DONGLE_SUPPORT
#include "app_dongle_spp.h"
#endif

typedef enum t_gaming_mode_info_type
{
    GAMING_MODE_STATUS     = 0x00,
} T_GAMING_MODE_INFO_TYPE;

static uint8_t uart_tx_seqn = 0;

#if F_APP_ONE_WIRE_UART_SUPPORT
static bool app_report_one_wire_uart_event_check(uint16_t event_id)
{
    if ((event_id == EVENT_ACK) ||
        (event_id == EVENT_MP_TEST_RESULT) ||
        (event_id == EVENT_FORCE_ENGAGE) ||
        (event_id == EVENT_AGING_TEST_CONTROL))
    {
        return true;
    }

#if F_APP_CHARGER_CASE_SUPPORT
    if ((event_id == EVENT_CHARGER_CASE_REPORT_STATUS) ||
        (event_id == EVENT_CHARGER_CASE_ENTER_OTA_MODE) ||
        (event_id == EVENT_CHAGRER_CASE_REPORT_BT_ADDR) ||
        (event_id == EVENT_CHARGER_CASE_FIND_CHARGER_CASE))
    {
        return true;
    }
#endif

    return false;
}
#endif

static void app_report_uart_event(uint16_t event_id, uint8_t *data, uint16_t len)
{
#if F_APP_CONSOLE_SUPPORT
    if (console_get_mode() != CONSOLE_MODE_BINARY)
    {
        return;
    }
#endif

    uint8_t *buf;
    uint16_t total_len;

    total_len = len + 7;

    buf = malloc(total_len);
    if (buf == NULL)
    {
        return;
    }

    uart_tx_seqn++;
    if (uart_tx_seqn == 0)
    {
        uart_tx_seqn = 1;
    }

    buf[0] = CMD_SYNC_BYTE;
    buf[1] = uart_tx_seqn;
    buf[2] = (uint8_t)(total_len - 5);
    buf[3] = (uint8_t)((total_len - 5) >> 8);
    buf[4] = (uint8_t)event_id;
    buf[5] = (uint8_t)(event_id >> 8);
    if (len)
    {
        memcpy(&buf[6], data, len);
    }
    buf[total_len - 1] = app_util_calc_checksum(&buf[1], total_len - 2);

    if (app_transfer_push_data_queue(CMD_PATH_UART, buf, total_len, 0) == false)
    {
        free(buf);
    }
}

static void app_report_le_event(T_APP_LE_LINK *p_link, uint16_t event_id, uint8_t *data,
                                uint16_t len)
{
    if (p_link->state == LE_LINK_STATE_CONNECTED)
    {
        uint8_t *buf;

        buf = malloc(len + 6);
        if (buf == NULL)
        {
            return;
        }

        buf[0] = CMD_SYNC_BYTE;
        p_link->tx_event_seqn++;
        if (p_link->tx_event_seqn == 0)
        {
            p_link->tx_event_seqn = 1;
        }
        buf[1] = p_link->tx_event_seqn;
        buf[2] = (uint8_t)(len + 2);
        buf[3] = (uint8_t)((len + 2) >> 8);
        buf[4] = (uint8_t)event_id;
        buf[5] = (uint8_t)(event_id >> 8);

        if (len)
        {
            memcpy(&buf[6], data, len);
        }

        if (app_transfer_push_data_queue(CMD_PATH_LE, buf, len + 6, p_link->id) == false)
        {
            free(buf);
        }
    }
}

static void app_report_spp_iap_event(T_APP_BR_LINK *p_link, uint16_t event_id, uint8_t *data,
                                     uint16_t len, uint8_t cmd_path)
{
    uint32_t check_prof = 0;

    if (cmd_path == CMD_PATH_SPP)
    {
        check_prof = SPP_PROFILE_MASK;
    }
    else if (cmd_path == CMD_PATH_IAP)
    {
        check_prof = IAP_PROFILE_MASK;
    }

    if (p_link->connected_profile & check_prof)
    {
        uint8_t *buf;

        buf = malloc(len + 6);
        if (buf == NULL)
        {
            return;
        }

        buf[0] = CMD_SYNC_BYTE;
        p_link->tx_event_seqn++;
        if (p_link->tx_event_seqn == 0)
        {
            p_link->tx_event_seqn = 1;
        }
        buf[1] = p_link->tx_event_seqn;
        buf[2] = (uint8_t)(len + 2);
        buf[3] = (uint8_t)((len + 2) >> 8);
        buf[4] = (uint8_t)event_id;
        buf[5] = (uint8_t)(event_id >> 8);

        if (len)
        {
            memcpy(&buf[6], data, len);
        }

#if F_APP_GAMING_DONGLE_SUPPORT
        if (app_link_check_dongle_link(p_link->bd_addr) && (check_prof == SPP_PROFILE_MASK))
        {
            app_dongle_report_pass_through_event(buf, len + 6);
            free(buf);
        }
        else
#endif
        {
            if (app_transfer_push_data_queue(cmd_path, buf, len + 6, p_link->id) == false)
            {
                free(buf);
            }
        }
    }
}

static bool app_report_spp_iap_raw_data(T_APP_BR_LINK *p_link, uint8_t *data, uint16_t len,
                                        uint8_t cmd_path)
{
    uint32_t check_prof = 0;
    bool ret = false;

    if (cmd_path == CMD_PATH_SPP)
    {
        check_prof = SPP_PROFILE_MASK;
    }
    else if (cmd_path == CMD_PATH_IAP)
    {
        check_prof = IAP_PROFILE_MASK;
    }

    if (p_link->connected_profile & check_prof)
    {
        uint8_t *buf;

        buf = malloc(len);
        if (buf == NULL)
        {
            return false;
        }

        memcpy(buf, data, len);

        ret = app_transfer_push_data_queue(cmd_path, buf, len, p_link->id);
        if (ret == false)
        {
            free(buf);
        }
    }

    return ret;
}

static bool app_report_le_raw_data(T_APP_LE_LINK *p_link, uint8_t *data, uint16_t len)
{
    uint8_t *buf;
    bool ret = false;

    buf = malloc(len);
    if (buf == NULL)
    {
        return false;
    }

    memcpy(buf, data, len);

    ret = app_transfer_push_data_queue(CMD_PATH_LE, buf, len, p_link->id);
    if (ret == false)
    {
        free(buf);
    }

    return ret;
}

void app_report_event(uint8_t cmd_path, uint16_t event_id, uint8_t app_index, uint8_t *data,
                      uint16_t len)
{
    APP_PRINT_INFO4("app_report_event: cmd_path %d, event_id 0x%04x, app_index %d, data_len %d",
                    cmd_path, event_id, app_index, len);

    switch (cmd_path)
    {
    case CMD_PATH_UART:
        {
            if ((app_cfg_const.enable_data_uart)
#if F_APP_ONE_WIRE_UART_SUPPORT
                || (app_cfg_const.one_wire_uart_support &&
                    (one_wire_state == ONE_WIRE_STATE_IN_ONE_WIRE) &&
                    app_report_one_wire_uart_event_check(event_id))
#endif
               )
            {
                app_report_uart_event(event_id, data, len);
            }
        }
        break;

    case CMD_PATH_LE:
        if (app_index < MAX_BLE_LINK_NUM)
        {
            app_report_le_event(&app_db.le_link[app_index], event_id, data, len);
        }
        break;

    case CMD_PATH_SPP:
    case CMD_PATH_IAP:
        if (app_index < MAX_BR_LINK_NUM)
        {
            app_report_spp_iap_event(&app_db.br_link[app_index], event_id, data, len, cmd_path);
        }
        break;

    default:
        break;
    }
}

void app_report_event_broadcast(uint16_t event_id, uint8_t *buf, uint16_t len)
{
    T_APP_BR_LINK *br_link;
    T_APP_LE_LINK *le_link;
    uint8_t        i;

    app_report_event(CMD_PATH_UART, event_id, 0, buf, len);

    for (i = 0; i < MAX_BR_LINK_NUM; i ++)
    {
        br_link = &app_db.br_link[i];

        if (br_link->cmd_set_enable == true)
        {
            if (br_link->connected_profile & SPP_PROFILE_MASK)
            {
                app_report_event(CMD_PATH_SPP, event_id, i, buf, len);
            }

            if (br_link->connected_profile & IAP_PROFILE_MASK)
            {
                app_report_event(CMD_PATH_IAP, event_id, i, buf, len);
            }
        }
    }

    for (i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        le_link = &app_db.le_link[i];

        if (le_link->state == LE_LINK_STATE_CONNECTED)
        {
            if (le_link->cmd_set_enable == true)
            {
                app_report_event(CMD_PATH_LE, event_id, i, buf, len);
            }
        }
    }
}

bool app_report_raw_data(uint8_t cmd_path, uint8_t app_index, uint8_t *data,
                         uint16_t len)
{
    bool ret = false;

    APP_PRINT_TRACE3("app_report_raw_data: cmd_path %d, app_index %d, data_len %d", cmd_path, app_index,
                     len);

    switch (cmd_path)
    {
    case CMD_PATH_SPP:
    case CMD_PATH_IAP:
        if ((app_index < MAX_BR_LINK_NUM) && data)
        {
            ret = app_report_spp_iap_raw_data(&app_db.br_link[app_index], data, len, cmd_path);
        }
        break;

    case CMD_PATH_LE:
        if ((app_index < MAX_BLE_LINK_NUM) && data)
        {
            ret = app_report_le_raw_data(&app_db.le_link[app_index], data, len);
        }
        break;

    default:
        break;
    }

    return ret;
}

void app_report_eq_idx(T_EQ_DATA_UPDATE_EVENT eq_data_update_event)
{
    if (!app_db.eq_ctrl_by_src)
    {
        return;
    }

    uint8_t buf[3];

    if (app_db.spk_eq_mode == GAMING_MODE)
    {
        buf[0] = app_cfg_nv.eq_idx_gaming_mode_record;
    }
    else if (app_db.spk_eq_mode == ANC_MODE)
    {
        buf[0] = app_cfg_nv.eq_idx_anc_mode_record;
    }
#if F_APP_VOICE_SPK_EQ_SUPPORT
    else if (app_db.spk_eq_mode == VOICE_SPK_MODE)
    {
        buf[0] = 0;
    }
#endif
    else
    {
        buf[0] = app_cfg_nv.eq_idx_normal_mode_record;
    }

    buf[1]  = app_db.spk_eq_mode;
    buf[2]  = eq_data_update_event;
    app_report_event_broadcast(EVENT_AUDIO_EQ_INDEX_REPORT, buf, 3);
}

void app_report_rws_state(void)
{
    uint8_t buf[2];
    buf[0] = GET_STATUS_RWS_STATE;
    buf[1] = app_db.remote_session_state;
    app_report_event_broadcast(EVENT_REPORT_STATUS, buf, 2);
}

void app_report_rws_bud_side(void)
{
    uint8_t buf[2];
    buf[0] = GET_STATUS_RWS_BUD_SIDE;
    buf[1] = app_cfg_const.bud_side;
    app_report_event_broadcast(EVENT_REPORT_STATUS, buf, 2);
}

#if F_APP_APT_SUPPORT
void app_report_apt_eq_idx(T_APT_EQ_DATA_UPDATE_EVENT apt_eq_data_update_event)
{
    if (!app_db.eq_ctrl_by_src)
    {
        return;
    }

    uint8_t buf[2];

    buf[0]  = app_cfg_nv.apt_eq_idx;
    buf[1]  = apt_eq_data_update_event;

    app_report_event_broadcast(EVENT_APT_EQ_INDEX_INFO, buf, 2);
}
#endif

void app_report_get_bud_info(uint8_t *data)
{
    uint8_t buf[6];
    uint8_t spk_channel = app_cfg_nv.spk_channel;

    if (app_cfg_const.bud_role == REMOTE_SESSION_ROLE_SINGLE)
    {
        buf[0] = BUD_TYPE_SINGLE;           // bud type
        buf[1] = INVALID_VALUE;             // primary bud side
        buf[2] = INVALID_VALUE;             // RWS channel
        buf[3] = app_db.local_batt_level;   // battery status
        buf[4] = 0;                         // invalid value
    }
    else
    {
        // bud type
        buf[0] = BUD_TYPE_RWS;

        // primary bud side
        if ((app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED) &&
            (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY))
        {
            // should not go here (report normally by primary bud)
            buf[1] = CHECK_IS_LCH ? R_CH_PRIMARY : L_CH_PRIMARY;
        }
        else
        {
            buf[1] = CHECK_IS_LCH ? L_CH_PRIMARY : R_CH_PRIMARY;
        }

        if (app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
        {
            spk_channel = app_cfg_const.solo_speaker_channel;

            if (CHECK_IS_LCH)
            {
                buf[2] = (spk_channel << 4);
            }
            else
            {
                buf[2] = spk_channel;
            }
        }
        else
        {
            if (CHECK_IS_LCH)
            {
                buf[2] = (spk_channel << 4) | app_db.remote_spk_channel;
            }
            else
            {
                buf[2] = (app_db.remote_spk_channel << 4) | spk_channel;
            }
        }

        // battery status
        buf[3] = L_CH_BATT_LEVEL;
        buf[4] = R_CH_BATT_LEVEL;
    }

    // case battery volume equals to (case_battery & 0x7F)
    buf[5] = (app_cfg_const.enable_rtk_charging_box) ? (app_cfg_nv.case_battery & 0x7F) : INVALID_VALUE;

    memcpy(data, &buf[0], sizeof(buf));
}

void app_report_rws_bud_info(void)
{
    uint8_t buf[6];

    app_report_get_bud_info(&buf[0]);
    app_report_event_broadcast(EVENT_REPORT_BUD_INFO, buf, sizeof(buf));
}

void app_report_gaming_mode_info(void)
{
    uint8_t buf[4];

    buf[0] = 3;  // LTV total length

    buf[1] = 2;
    buf[2] = GAMING_MODE_STATUS;
    buf[3] = app_db.gaming_mode;

    app_report_event_broadcast(EVENT_GAMING_MODE_INFO, buf, sizeof(buf));
}

#if F_APP_CHARGER_CASE_SUPPORT
void app_report_bud_loc_to_charger_case(void)
{
    uint8_t loc_status[2];

    if (CHECK_IS_LCH)
    {
        loc_status[0] = app_db.local_loc;
        loc_status[1] = app_db.remote_loc;
    }
    else
    {
        loc_status[0] = app_db.remote_loc;
        loc_status[1] = app_db.local_loc;
    }

    app_report_status_to_charger_case(CHARGER_CASE_GET_BUD_LOCATION_STATUS, loc_status);

    if (app_db.local_loc == BUD_LOC_IN_CASE)
    {
        app_report_status_to_charger_case(CHARGER_CASE_GET_BUD_INFO, NULL);
    }
}

void app_report_status_to_charger_case(uint8_t type, uint8_t *param)
{
    switch (type)
    {
    case CHARGER_CASE_GET_BUD_LOCATION_STATUS:
    case CHARGER_CASE_GET_BUD_BATTERY_STATUS:
        {
            if (app_db.charger_case_connected &&
                app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                uint8_t evt_buf[3];

                evt_buf[0] = type;
                evt_buf[1] = param[0];
                evt_buf[2] = param[1];
                app_report_event(CMD_PATH_LE, EVENT_CHARGER_CASE_REPORT_STATUS, app_db.charger_case_link_id,
                                 evt_buf,
                                 sizeof(evt_buf));
            }
        }
        break;

    case CHARGER_CASE_GET_MUTE_STATUS:
    case CHARGER_CASE_GET_CONNECT_STATUS:
        {
            if (app_db.charger_case_connected &&
                app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
            {
                uint8_t evt_buf[2];

                evt_buf[0] = type;
                evt_buf[1] = param[0];
                app_report_event(CMD_PATH_LE, EVENT_CHARGER_CASE_REPORT_STATUS, app_db.charger_case_link_id,
                                 evt_buf,
                                 sizeof(evt_buf));
            }
        }
        break;

    case CHARGER_CASE_GET_BUD_INFO:
        {
            uint8_t evt_buf[49];

            evt_buf[0] = type;
            memcpy(&evt_buf[1], &param[0], 40);
            memcpy(&evt_buf[41], app_cfg_nv.bud_local_addr, 6);
#if F_APP_SC_KEY_DERIVE_SUPPORT
            evt_buf[47] = GAP_LOCAL_ADDR_LE_PUBLIC;
#else
            evt_buf[47] = GAP_LOCAL_ADDR_LE_RANDOM;
#endif
            evt_buf[48] = app_cfg_const.bud_side;

            if (app_db.local_loc == BUD_LOC_IN_CASE)
            {
                app_report_event(CMD_PATH_UART, EVENT_CHARGER_CASE_REPORT_STATUS, 0,
                                 evt_buf,
                                 sizeof(evt_buf));
            }
            else
            {
                app_report_event(CMD_PATH_LE, EVENT_CHARGER_CASE_REPORT_STATUS, app_db.charger_case_link_id,
                                 evt_buf,
                                 sizeof(evt_buf));
            }
        }
        break;

    default:
        break;
    }
}
#endif

