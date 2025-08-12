#include <stdlib.h>
#include "trace.h"
#include "app_usb_hid.h"
#include "app_gaming_ctrl_cfg.h"
#include "app_usb_layer.h"
#include "test_mode.h"
#include "app_bond.h"
#include "btm.h"
#include "ual_bluetooth.h"
#include "app_main.h"
#include "app_cfg.h"
#include "gaming_bt.h"
#include "app_link_util.h"
#include "app_timer.h"
#include "app_mmi.h"
#include "gap_vendor.h"
#include "legacy_audio_wrapper.h"
#include "app_adapter_service.h"
#include "app_le_audio.h"

#ifdef LEGACY_BT_GAMING

#define SINK_DEV_NUM   2
#define HCI_VENDOR_MP_OPCODE        0xFCEB

#define CONNECTION_EVENT_TIMEOUT    150000

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
#define LOCK_ACK_TIMEOUT                   (5000)
#define HEADSET_LOCK_ACK_SUCCESS            0x00
#define HEADSET_LOCK_ACK_FAIL               0x01
#define HEADSET_LOCK_ACK_TIMEOUT            0x02

#define ADDR_TYPE_PUBLIC                    0x00
#define ADDR_TYPE_RANDOM                    0x01
#define ADDR_TYPE_UNKNOW                    0x02

static uint8_t       headset_lock_ack_timer = 0;
static uint8_t       new_read_data[11];
#endif

typedef struct
{
    uint8_t conn_state;
    uint8_t profile_state;
    uint8_t remote_addr[6];
} T_GAMING_CTRL;

typedef enum
{
    GAMING_BT_STATE_DISCONNECTED = 0x00,
    GAMING_BT_STATE_CONNECTED,
} T_GAMING_CONN_STATE;

static T_GAMING_CTRL t_gaming_ctrl[SINK_DEV_NUM];
static bool usb_cmd_discovery = false;
static bool usb_cmd_disconnect = false;
static uint8_t usb_cmd_connection_count;

static uint8_t      usb_cmd_control_timer_queue_id = 0;
static uint8_t      usb_cmd_connection_event_timer[SINK_DEV_NUM] = { 0 };
extern bool         force_enter_pairing;

/*command event start*/
static void legacy_gaming_cmd_complete_event(uint16_t opcode, uint8_t status)
{
    uint8_t report[5];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, LEGACY_GAMING_COMMAND_COMPLETE_EVENT);
    UINT16_TO_STREAM(pp, opcode);
    UINT8_TO_STREAM(pp, status);

    app_usb_hid_send_bt_ctrl_data(5, report);
}

static uint8_t gaming_get_eir_data_significant_len(uint8_t *data, uint8_t total_len)
{
    uint8_t significant_len = 0;

    if ((data == NULL) || (total_len == 0) || (total_len > 0xF0))
    {
        return 0;
    }
    else
    {
        while (significant_len < total_len)
        {
            if (data[significant_len] > 0)
            {
                significant_len += data[significant_len] + 1;
            }
            else
            {
                break;
            }
        }

        return significant_len;
    }
}

static void gaming_app_discovery_result_report_event(T_BT_EVENT_PARAM_INQUIRY_RESULT inquiry_result)
{
    uint8_t report[60];
    uint8_t *pp = report;
    uint8_t significant_len = 0;
    uint8_t eir_offset = 0;

    significant_len = gaming_get_eir_data_significant_len(inquiry_result.p_eir, inquiry_result.eir_len);
    UINT16_TO_STREAM(pp, GAMING_BT_DISCOVERY_REPORT_EVENT);
    UINT8_TO_STREAM(pp, eir_offset);
    UINT8_TO_STREAM(pp, significant_len);
    memcpy(pp, inquiry_result.bd_addr, 6);
    pp += 6;
    UINT8_TO_STREAM(pp, (uint8_t)((inquiry_result.cod & 0x00FF0000) >> 16));
    UINT8_TO_STREAM(pp, (uint8_t)((inquiry_result.cod & 0x0000FF00) >> 8));
    UINT8_TO_STREAM(pp, (uint8_t)(inquiry_result.cod & 0x000000FF));
    UINT8_TO_STREAM(pp, inquiry_result.rssi);

    if (significant_len <= 46)
    {
        memcpy(pp, inquiry_result.p_eir, significant_len);
        app_usb_hid_send_bt_ctrl_data(significant_len + 14, report);
    }
    else
    {
        uint8_t eir_len_left = 0;

        memcpy(pp, inquiry_result.p_eir, 46);
        app_usb_hid_send_bt_ctrl_data(60, report);

        eir_len_left = significant_len - 46;
        while (eir_len_left > 57)
        {
            memset(report, 0, 60);
            pp = report;
            UINT16_TO_STREAM(pp, GAMING_BT_DISCOVERY_REPORT_EVENT);
            UINT8_TO_STREAM(pp, (significant_len - eir_len_left));
            memcpy(pp, (inquiry_result.p_eir + (significant_len - eir_len_left)), 57);
            app_usb_hid_send_bt_ctrl_data(60, report);

            eir_len_left -= 57;
        }

        if (eir_len_left > 0)
        {
            memset(report, 0, 60);
            pp = report;
            UINT16_TO_STREAM(pp, GAMING_BT_DISCOVERY_REPORT_EVENT);
            UINT8_TO_STREAM(pp, (significant_len - eir_len_left));
            memcpy(pp, (inquiry_result.p_eir + (significant_len - eir_len_left)), eir_len_left);
            app_usb_hid_send_bt_ctrl_data(60, report);
        }
    }
}

static void gaming_app_bt_connection_state_event(uint8_t idx, uint8_t conn_state,
                                                 uint8_t profile_mask)
{
    uint8_t report[11];
    uint8_t *pp = report;
    UINT16_TO_STREAM(pp, GAMING_BT_CONNECTION_STATE_REPORT_EVENT);
    UINT8_TO_STREAM(pp, idx);
    memcpy(pp, t_gaming_ctrl[idx].remote_addr, 6);
    pp += 6;
    UINT8_TO_STREAM(pp, conn_state);
    UINT8_TO_STREAM(pp, profile_mask);
    APP_PRINT_INFO4("gaming_app_bt_connection_state_event: idx %d,bddr %b,conn_state %d,profile_mask %d",
                    idx, TRACE_BDADDR(t_gaming_ctrl[idx].remote_addr), conn_state, profile_mask);
    app_usb_hid_send_bt_ctrl_data(11, report);
    if (usb_cmd_disconnect)
    {
        usb_cmd_disconnect = false;
    }
    else
    {
        usb_cmd_connection_count--;
    }
}

static void gaming_app_get_version_event(void)
{
    uint8_t report[4];
    uint8_t *pp = report;
    UINT16_TO_STREAM(pp, GAMING_BT_DONGLE_VERSION_REPORT_EVENT);
    UINT8_TO_STREAM(pp, (uint8_t)RTL8763EAU_VER_MAJOR);
    UINT8_TO_STREAM(pp, (uint8_t)RTL8763EAU_VER_MINOR);

    app_usb_hid_send_bt_ctrl_data(4, report);
}

extern T_APP_CFG_NV app_cfg_nv;
static void gaming_app_get_dongle_name_event(void)
{
    uint8_t name_len;
    name_len = strlen((char *)app_cfg_nv.device_name_legacy);

    uint8_t *report = calloc(1, name_len + 2);
    if (report == NULL)
    {
        return;
    }
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, GAMING_BT_DONGLE_NAME_REPORT_EVENT);
    memcpy(pp, app_cfg_nv.device_name_legacy, name_len);
    app_usb_hid_send_bt_ctrl_data(name_len + 2, report);

    free(report);
}

static void gaming_app_paired_device_info_event(uint8_t index)
{
    uint8_t report[10];
    uint8_t remote_addr[6];
    uint8_t remote_addr_type = 0x00;
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, GAMING_BT_PAIRED_DEVICE_INFO_REPORT_EVENT);
    UINT8_TO_STREAM(pp, index);
    UINT8_TO_STREAM(pp, remote_addr_type);

    if (app_src_legacy_get_bond_addr_by_index(index, remote_addr))
    {
        memcpy(pp, remote_addr, 6);
    }
    else
    {
        memset(pp, 0x00, 6);
    }
    app_usb_hid_send_bt_ctrl_data(10, report);
}

static void gaming_app_get_public_bt_addr_event(void)
{
    uint8_t report[8];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, GAMING_BT_PUBLIC_ADDRESS_REPORT_EVENT);
    memcpy(pp, app_db.factory_addr, 6);

    app_usb_hid_send_bt_ctrl_data(8, report);
}

static void gaming_app_get_link_key_event(uint8_t *bd_addr)
{
    uint8_t report[25];
    uint8_t *pp = report;
    uint8_t bd_addr_reverse[6];

    UINT16_TO_STREAM(pp, GAMING_BT_LINK_KEY_REPORT_EVENT);

    for (uint8_t i = 0; i < 6; i++)
    {
        bd_addr_reverse[5 - i] = bd_addr[i];
    }
    APP_PRINT_INFO1("app_get_link_key(), bd_addr %b", TRACE_BDADDR(bd_addr_reverse));

    memcpy(pp, bd_addr_reverse, 6);
    pp += BD_ADDR_LEN;

    if (app_src_legacy_get_bond_by_addr(bd_addr_reverse, pp + 1, pp))
    {
        app_usb_hid_send_bt_ctrl_data(25, report);
    }
    else
    {
        APP_PRINT_ERROR0("app_get_link_key():cant not find !");
    }
}

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
static void gaming_app_get_addr_lock_status_event(void)
{
    uint8_t report[18];
    uint8_t lock_addr[6];
    uint8_t remote_addr[6];
    uint8_t lock_flag;
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, GAMING_LOCK_DETECT_REPORT_EVENT);
    UINT8_TO_STREAM(pp, ADDR_TYPE_PUBLIC);
    memcpy(pp, app_db.factory_addr, 6);
    pp += BD_ADDR_LEN;
    lock_flag = app_src_legacy_get_lock_flag_by_index(0);
    if (lock_flag)
    {
        UINT8_TO_STREAM(pp, ADDR_TYPE_PUBLIC);
        if (app_src_legacy_get_lock_addr_by_index(0, lock_addr) != 0)
        {
            memcpy(pp, lock_addr, 6);
        }
    }
    else
    {
        if (t_gaming_ctrl[0].conn_state)
        {
            UINT8_TO_STREAM(pp, ADDR_TYPE_PUBLIC)
            if (app_src_legacy_get_bond_addr_by_index(0, remote_addr))
            {
                memcpy(pp, remote_addr, 6);
            }
        }
        else
        {
            UINT8_TO_STREAM(pp, ADDR_TYPE_UNKNOW);
            memset(pp, 0x00, 6);
        }

    }
    pp += BD_ADDR_LEN;
    UINT8_TO_STREAM(pp, lock_flag);
    UINT8_TO_STREAM(pp, t_gaming_ctrl[0].conn_state);
    app_usb_hid_send_bt_ctrl_data(18, report);
}

static void gaming_app_ctrl_lock_or_unlock_event(uint8_t cmd)
{
    uint8_t report[11];
    uint8_t remote_addr[6];
    uint8_t *pp = report;
    extern void app_src_dongle_lock_headset(bool lock);
    UINT16_TO_STREAM(pp, GAMING_LOCK_HEADSET_REPORT_EVENT);
    UINT8_TO_STREAM(pp, ADDR_TYPE_PUBLIC);
    if (app_src_legacy_get_bond_addr_by_index(0, remote_addr))
    {
        memcpy(pp, remote_addr, 6);
    }
    pp += BD_ADDR_LEN;
    switch (cmd)
    {
    case 0:
        {
            UINT8_TO_STREAM(pp, 0x00);
            app_src_dongle_lock_headset(false);
            memcpy(new_read_data, report, 11);
            app_start_timer(&headset_lock_ack_timer,
                            "wait_headset_lock_ack",
                            usb_cmd_control_timer_queue_id,
                            HEADSET_LOCK_ACK_TIMER_ID,
                            false,
                            0,
                            LOCK_ACK_TIMEOUT);
            break;
        }
    case 1:
        {
            UINT8_TO_STREAM(pp, 0x01);
            app_src_dongle_lock_headset(true);
            memcpy(new_read_data, report, 11);
            app_start_timer(&headset_lock_ack_timer,
                            "wait_headset_lock_ack",
                            usb_cmd_control_timer_queue_id,
                            HEADSET_LOCK_ACK_TIMER_ID,
                            false,
                            0,
                            LOCK_ACK_TIMEOUT);
            break;
        }
    }
}
#endif
static void gaming_app_device_type_report_event(void)
{
    uint8_t report[3];
    uint8_t *pp = report;
    uint8_t device_type = 0x00;

    UINT16_TO_STREAM(pp, GAMING_BT_DEVICE_TYPE_REPORT_EVENT);
    UINT8_TO_STREAM(pp, device_type);
    app_usb_hid_send_bt_ctrl_data(3, report);
}

static void gaming_app_write_address_status_event(uint8_t save_status)
{
    uint8_t report[3];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, GAMING_WRITE_ADDRESS_STATUS_EVENT);
    UINT8_TO_STREAM(pp, save_status);

    app_usb_hid_send_bt_ctrl_data(3, report);
}

static void gaming_app_read_address_report_event(void)
{
    uint8_t report[5];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, GAMING_READ_ADDRESS_REPORT_EVENT);
    memcpy(pp, app_cfg_nv.saved_id, 3);

    app_usb_hid_send_bt_ctrl_data(5, report);
}

void gaming_app_cis_pair_status_report_event(uint8_t connect_status)
{
    uint8_t report[3];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, GAMING_PAIR_STATUS_REPORT_EVENT);
    UINT8_TO_STREAM(pp, connect_status);
    app_usb_hid_send_bt_ctrl_data(3, report);
}

uint8_t app_usb_hid_handle_gaming_cmd(uint8_t *p_data, uint16_t len)
{
    uint8_t status = GAMING_COMMAND_COMPLETE_SUCCESS;
    uint8_t *pp = p_data;
    uint16_t length = len;
    uint16_t opcode;
    STREAM_TO_UINT16(opcode, pp);
    length -= 2;

    APP_PRINT_INFO0("app_usb_hid_handle_gaming_cmd(): start");

    switch (opcode)
    {
    case GAMING_BT_START_DISCOVERY_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_bt_start_discovery();
            usb_cmd_discovery = true;
        }
        break;

    case GAMING_BT_STOP_DISCOVERY_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_bt_stop_discovery();
            usb_cmd_discovery = false;
        }
        break;

    case GAMING_BT_CONNECT_OPCODE:
        {
            uint8_t bd_addr[6];
            uint8_t index;
            if (length != 7)
            {
                status = GAMING_INVALID_COMMAND_LENGTH;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            if ((*pp != 0) && (*pp != 1))
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            index = *pp;
            pp++;
            for (uint8_t i = 0; i < 6; i++)
            {
                bd_addr[5 - i] = *pp++;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            APP_PRINT_INFO2("app_usb_hid_handle_gaming_cmd:id %d, bd_addr %b", index, TRACE_BDADDR(bd_addr));
            gaming_bt_connect(index, bd_addr);
            usb_cmd_connection_count++;
            app_start_timer(&usb_cmd_connection_event_timer[index],
                            "usb_cmd_connection_event",
                            usb_cmd_control_timer_queue_id,
                            CONNECTION_STATE_EVENT_TIMER_ID + index,
                            false,
                            0,
                            CONNECTION_EVENT_TIMEOUT);
        }
        break;

    case GAMING_BT_DISCONNECT_BY_ID_OPCODE:
        {
            if ((*pp != 0) && (*pp != 1))
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            if (t_gaming_ctrl[*pp].conn_state == GAMING_BT_STATE_DISCONNECTED)
            {
                APP_PRINT_WARN0("app_usb_hid_handle_gaming_cmd(): already disconncet!");
                status = GAMING_COMMAND_OPERATION_FAIL;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_bt_disconnect_by_id(*pp);
            usb_cmd_disconnect = true;
        }
        break;

    case GAMING_BT_DISCONNECT_BY_ADDRESS_OPCODE:
        {
            uint8_t bd_addr[6];
            uint8_t index;
            if (length != 6)
            {
                status = GAMING_INVALID_COMMAND_LENGTH;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            for (uint8_t i = 0; i < 6; i++)
            {
                bd_addr[5 - i] = *pp++;
            }
            if (app_src_legacy_get_paired_idx(bd_addr, &index) == false)
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            if (t_gaming_ctrl[index].conn_state == GAMING_BT_STATE_DISCONNECTED)
            {
                APP_PRINT_WARN0("app_usb_hid_handle_gaming_cmd(): already disconncet!");
                status = GAMING_COMMAND_OPERATION_FAIL;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_bt_disconnect_by_bdaddr(bd_addr);
            usb_cmd_disconnect = true;
        }
        break;

    case GAMING_BT_REMOVE_BOND_BY_ID_OPCODE:
        {
            if ((*pp != 0) && (*pp != 1))
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_bt_remove_bond_by_id(*pp);
        }
        break;

    case GAMING_BT_REMOVE_BOND_BY_ADDRESS_OPCODE:
        {
            uint8_t bd_addr[6];
            uint8_t index;
            if (length != 6)
            {
                status = GAMING_INVALID_COMMAND_LENGTH;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            for (uint8_t i = 0; i < 6; i++)
            {
                bd_addr[5 - i] = *pp++;
            }
            if (app_src_legacy_get_paired_idx(bd_addr, &index) == false)
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_bt_remove_bond_by_bdaddr(bd_addr);
        }
        break;

    case GAMING_BT_QUERY_CONNECTION_STATE_OPCODE:
        {
            if ((*pp != 0) && (*pp != 1))
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            usb_cmd_connection_count++;
            gaming_app_bt_connection_state_event(*pp, t_gaming_ctrl[*pp].conn_state,
                                                 t_gaming_ctrl[*pp].profile_state);
        }
        break;

    case GAMING_BT_GET_VERSION_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_app_get_version_event();
        }
        break;

    case GAMING_BT_GET_DONGLE_NAME_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_app_get_dongle_name_event();
        }
        break;

    case GAMING_BT_GET_PAIRED_DEVICE_INFO_OPCODE:
        {
            if (length != 1)
            {
                status = GAMING_INVALID_COMMAND_LENGTH;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            if ((*pp != 0) && (*pp != 1))
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_app_paired_device_info_event(*pp);
        }
        break;

    case GAMING_BT_GET_PUBLIC_ADDRESS_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_app_get_public_bt_addr_event();
        }
        break;

    case GAMING_BT_GET_LINK_KEY_OPCODE:
        {
            if (length != 6)
            {
                status = GAMING_INVALID_COMMAND_LENGTH;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_app_get_link_key_event(pp);
        }
        break;

    case GAMING_BT_ENTER_DUT_MODE_OPCODE:
        {
            APP_PRINT_INFO0("Enter dut test mode");
            legacy_gaming_cmd_complete_event(opcode, status);
            switch_into_single_tone_test_mode();
        }
        break;

    case GAMING_BT_FACTORY_RESET_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
            app_src_legacy_clear_lock_info();
#endif
            app_mmi_handle_action(MMI_DEV_FACTORY_RESET);
        }
        break;

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
    case GAMING_LOCK_DETECT_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_app_get_addr_lock_status_event();
        }
        break;
    case GAMING_LOCK_HEADSET_OPCODE:
        {
            uint8_t bd_addr[6];
            uint8_t remote_addr[6];
            pp++;
            if (length != 8)
            {
                status = GAMING_INVALID_COMMAND_LENGTH;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            for (uint8_t i = 0; i < 6; i++)
            {
                bd_addr[i] = *pp++;
            }
            if ((*pp != 0) && (*pp != 1))
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            if (app_src_legacy_get_bond_addr_by_index(0, remote_addr))
            {
                if ((t_gaming_ctrl[0].profile_state & 0x01) && (memcmp(bd_addr, remote_addr, 6) == 0))
                {
                    legacy_gaming_cmd_complete_event(opcode, status);
                    gaming_app_ctrl_lock_or_unlock_event(*pp);
                }
                else
                {
                    status = GAMING_INVALID_COMMAND_PARAM;
                    legacy_gaming_cmd_complete_event(opcode, status);
                }
            }
            else
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
            }
        }
        break;
#endif

    case GAMING_DUAL_MODE_CONNECT_OPCODE:
        {
            uint8_t bd_addr[6];
            uint8_t saved_addr[3];
            uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };
            uint8_t index;
            if (length != 7)
            {
                status = GAMING_INVALID_COMMAND_LENGTH;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            if ((*pp != 0) && (*pp != 1))
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            index = *pp;
            pp++;
            for (uint8_t i = 0; i < 6; i++)
            {
                bd_addr[5 - i] = *pp++;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            memcpy(saved_addr, app_cfg_nv.saved_id, 3);

            if (!memcmp(bd_addr, ba_any, 6))
            {
                switch (index)
                {
                case 0:
                    app_mmi_handle_action(MMI_BT_LEGACY_LINK1_SCAN);
                    break;
                //FIXME: Currently, 2.4G Mode only support in single link.
                case 1:
                    break;
                default:
                    break;
                }
            }
            else
            {
                app_device_save_pairing_id(bd_addr, 3);

                switch (index)
                {
                case 0:
                    if (!memcmp(app_cfg_nv.saved_id, saved_addr, 3) &&
                        t_gaming_ctrl[index].conn_state == GAMING_BT_STATE_CONNECTED)
                    {
                        APP_PRINT_WARN0("app_usb_hid_handle_gaming_cmd: alredy connected!");
                    }
                    else if (t_gaming_ctrl[index].conn_state == GAMING_BT_STATE_CONNECTED)
                    {
                        /* force disconnect and reconnect after dev disconnect */
                        gaming_bt_disconnect_by_id(0);
                        force_enter_pairing = true;
                    }
                    else
                    {
                        gaming_bt_force_connect(0, ba_any);
                    }
                    break;
                case 1:
                    break;
                default:
                    break;
                }
            }

            APP_PRINT_INFO4("app_usb_hid_handle_gaming_cmd:index %d, bd_addr %b,saved_id %b,already_saved_id %b",
                            index, TRACE_BDADDR(bd_addr), TRACE_BINARY(3, app_cfg_nv.saved_id), TRACE_BINARY(3, saved_addr));
            usb_cmd_connection_count++;
        }
        break;

    case GAMING_BT_MP_HCI_CMD_PASSTHROUGH_OPCODE:
        {
            uint16_t hci_opcode;
            uint8_t cmd_pl_len = length - 2;
            STREAM_TO_UINT16(hci_opcode, pp);
            APP_PRINT_INFO3("HCI MP CMD START: hci_opcode %x, data %b, len %d", hci_opcode,
                            TRACE_BINARY(cmd_pl_len, pp), cmd_pl_len);
            //usb cmd formate: big-endian and little-endian about byte order.
            if (hci_opcode == 0xebfc)
            {
                legacy_audio_exit();
                bt_device_mode_set(BT_DEVICE_MODE_IDLE);
                legacy_gaming_cmd_complete_event(opcode, status);
                gap_vendor_cmd_req(HCI_VENDOR_MP_OPCODE, cmd_pl_len, pp);
            }
            else
            {
                status = GAMING_INVALID_COMMAND_PARAM;
                legacy_gaming_cmd_complete_event(opcode, status);
            }
        }
        break;

    case GAMING_DEVICE_IDENTIFY_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_app_device_type_report_event();
        }
        break;

    case GAMING_WRITE_ADDRESS_OPCODE:
        {
            uint8_t bd_addr[6];
            uint8_t save_status;
            if (length != 6)
            {
                status = GAMING_INVALID_COMMAND_LENGTH;
                legacy_gaming_cmd_complete_event(opcode, status);
                break;
            }
            legacy_gaming_cmd_complete_event(opcode, status);
            for (uint8_t i = 0; i < 6; i++)
            {
                bd_addr[5 - i] = *pp++;
            }
            if (app_device_save_pairing_id(bd_addr, 3))
            {
                save_status = 0;
            }
            else
            {
                save_status = 1;
            }

            gaming_app_write_address_status_event(save_status);
        }
        break;

    case GAMING_READ_ADDRESS_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
            gaming_app_read_address_report_event();
        }
        break;

    case GAMING_BT_PAIR_OPCODE:
        {
            uint8_t bd_addr[6];
            uint8_t recv_id[3];
            uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };
            uint8_t null_id[3] = { 0, 0, 0};

            legacy_gaming_cmd_complete_event(opcode, status);

            for (uint8_t i = 0; i < 6; i++)
            {
                bd_addr[5 - i] = *pp++;
            }
            memcpy(recv_id, bd_addr, 3);

            APP_PRINT_INFO2("recv_id %b, saved_id %b", TRACE_BINARY(3, recv_id), TRACE_BINARY(3,
                            app_cfg_nv.saved_id));

            if (app_get_cur_bt_mode() == DONGLE_BT_MODE_CIS)
            {
#if F_APP_LEA_DONGLE_BINDING
                if (!memcmp(recv_id, null_id, 3))
                {
                    // recv_id is empty, enter pairing mode
                    adapter_set_scan_by_mmi(true);
                }
                else
                {
                    // recv_id is not empty, enter link back mode
                    adapter_set_scan_by_mmi(false);
                    app_device_save_pairing_id(recv_id, 3);
                }

                app_le_audio_start_auto_pair();
#endif
            }
            else if (app_get_cur_bt_mode() == DONGLE_BT_MODE_LEGACY)
            {
                if (!memcmp(recv_id, null_id, 3))
                {
                    // recv_id is empty, enter pairing mode
                    legacy_audio_mmi(MMI_BT_LEGACY_LINK1_SCAN, NULL);
                }
                else
                {
                    // recv_id is not empty, enter link back mode
                    app_device_save_pairing_id(recv_id, 3);

                    gaming_bt_connect(0, ba_any);
                }
            }
        }
        break;

    case GAMING_BT_STOP_PAIR_OPCODE:
        {
            legacy_gaming_cmd_complete_event(opcode, status);
            if (app_get_cur_bt_mode() == DONGLE_BT_MODE_CIS)
            {
#if F_APP_LEA_DONGLE_BINDING
                if (adapter_get_scan_by_mmi())
                {
                    adapter_set_scan_by_mmi(false);
                }
                app_le_usb_cmd_handle_stop_scan();
#endif
            }
            else if (app_get_cur_bt_mode() == DONGLE_BT_MODE_LEGACY)
            {
                legacy_audio_mmi(MMI_BT_LEGACY_LINK1_DISCONNECT, NULL);
                extern void src_stop_le_scan(void);
                src_stop_le_scan();
            }
        }
        break;

    default:
        status = GAMING_UNKNOWN_COMMAND;
        break;
    }
    APP_PRINT_INFO3("app_usb_hid_handle_gaming_cmd opcode 0x%x, data %b, status %d", opcode,
                    TRACE_BINARY(len, p_data), status);
    return status;
}

void gaming_usb_ctrl_cmd_msg(uint8_t event_type, uint8_t *event_buf, uint16_t buf_len)
{
    switch (event_type)
    {
    case EVENT_GAMING_DEVICE_FOUND:
        {
            struct gaming_ev_device_found *ev = (void *)event_buf;

            if (!ev)
            {
                APP_PRINT_ERROR0("gaming_ctrl_cmd_bt_mgr_cback: invalid dev disconn ev");
                return ;
            }
            if (usb_cmd_discovery)
            {
                gaming_app_discovery_result_report_event(ev->result);
            }
        }
        break;

    case EVENT_GAMING_DEVICE_CONNECTED:
        {
            struct gaming_ev_device_connected *ev = (void *)event_buf;
            if (!ev)
            {
                APP_PRINT_ERROR0("gaming_ctrl_cmd_bt_mgr_cback: invalid dev conn ev");
                return ;
            }
            t_gaming_ctrl[ev->id].conn_state = GAMING_BT_STATE_CONNECTED;
        }
        break;

    case EVENT_GAMING_DEVICE_DISCONNECTED:
        {
            struct gaming_ev_device_disconnected *ev = (void *)event_buf;

            if (!ev)
            {
                APP_PRINT_ERROR0("gaming_ctrl_cmd_bt_mgr_cback: invalid dev disconn ev");
                return ;
            }
            t_gaming_ctrl[ev->id].conn_state = GAMING_BT_STATE_DISCONNECTED;

            if (usb_cmd_disconnect)
            {
                gaming_app_bt_connection_state_event(ev->id, t_gaming_ctrl[ev->id].conn_state,
                                                     t_gaming_ctrl[ev->id].profile_state);
            }
        }
        break;
    case EVENT_GAMING_PROFILE_CONNECTED:
        {
            struct gaming_ev_profile_connected *ev = (void *)event_buf;

            if (!ev)
            {
                APP_PRINT_ERROR0("gaming_ctrl_cmd_bt_mgr_cback: invalide profile conn ev");
                return ;
            }
            memcpy(t_gaming_ctrl[ev->id].remote_addr, ev->ba, 6);

            if (ev->profile == SPP_AUDIO_PROFILE_MASK)
            {
                t_gaming_ctrl[ev->id].profile_state |= 0x01;
            }
            if (ev->profile == A2DP_PROFILE_MASK)
            {
                t_gaming_ctrl[ev->id].profile_state |= 0x02;
            }
            if (ev->profile & AVRCP_PROFILE_MASK)
            {
                t_gaming_ctrl[ev->id].profile_state |= 0x04;

                if (usb_cmd_connection_count)
                {
                    app_stop_timer(&usb_cmd_connection_event_timer[ev->id]);
                    gaming_app_bt_connection_state_event(ev->id, t_gaming_ctrl[ev->id].conn_state,
                                                         t_gaming_ctrl[ev->id].profile_state);
                }
            }
        }
        break;

    case EVENT_GAMING_PROFILE_DISCONNECTED:
        {
            struct gaming_ev_profile_disconnected *ev = (void *)event_buf;

            if (!ev)
            {
                APP_PRINT_ERROR0("gaming_ctrl_cmd_bt_mgr_cback: invalid profile disc ev");
                return ;
            }

            memcpy(t_gaming_ctrl[ev->id].remote_addr, ev->ba, 6);

            if (ev->profile == SPP_AUDIO_PROFILE_MASK)
            {
                t_gaming_ctrl[ev->id].profile_state &= 0x0E;
            }
            if (ev->profile == A2DP_PROFILE_MASK)
            {
                t_gaming_ctrl[ev->id].profile_state &= 0x0D;
            }
            if (ev->profile & AVRCP_PROFILE_MASK)
            {
                t_gaming_ctrl[ev->id].profile_state &= 0x0B;
            }
        }
        break;

    default:
        break;
    }
    return ;
}

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
void app_ctrl_spp_receive_ack(uint8_t *params, uint8_t *bd_addr)
{
    uint8_t lock_flag = 0;
    uint8_t lock_addr[6];
    memset(lock_addr, 0, 6);
    if (params[0] == 0x01)
    {
        if (params[1] == 0x01)
        {
            app_stop_timer(&headset_lock_ack_timer);
            APP_PRINT_WARN0("src_spp_audio_packet_received: lock Fail");
            new_read_data[10] = HEADSET_LOCK_ACK_FAIL;
            app_usb_hid_send_bt_ctrl_data(11, new_read_data);

        }
        else
        {
            APP_PRINT_INFO0("src_spp_audio_packet_received: lock success");
            app_stop_timer(&headset_lock_ack_timer);
            lock_flag = 1;
            if (!app_src_legacy_save_lock_info(0, bd_addr, lock_flag))
            {
                APP_PRINT_WARN0("app_src_legacy_save_lock_info: ERROR!");
            }
            new_read_data[10] = HEADSET_LOCK_ACK_SUCCESS;
            app_usb_hid_send_bt_ctrl_data(11, new_read_data);

        }
    }
    else
    {
        if (params[1] == 0x01)
        {
            app_stop_timer(&headset_lock_ack_timer);
            APP_PRINT_WARN0("src_spp_audio_packet_received: unlock Fail");
            new_read_data[10] = HEADSET_LOCK_ACK_FAIL;
            app_usb_hid_send_bt_ctrl_data(11, new_read_data);

        }
        else
        {
            APP_PRINT_WARN0("src_spp_audio_packet_received: unlock success");
            app_stop_timer(&headset_lock_ack_timer);
            lock_flag = 0;
            if (!app_src_legacy_save_lock_info(0, lock_addr, lock_flag))
            {
                APP_PRINT_WARN0("app_src_legacy_save_lock_info: ERROR!");
            }
            new_read_data[10] = HEADSET_LOCK_ACK_SUCCESS;
            app_usb_hid_send_bt_ctrl_data(11, new_read_data);

        }
    }
}
#endif

void gaming_app_ctrl_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_INFO2("gaming_app_ctrl_cback: timer_id %d, timer_channel %d", timer_id, timer_chann);
    switch (timer_id)
    {
    case CONNECTION_STATE_EVENT_TIMER_ID:
        {
            app_stop_timer(&usb_cmd_connection_event_timer[0]);
            gaming_app_bt_connection_state_event(0, t_gaming_ctrl[0].conn_state,
                                                 t_gaming_ctrl[0].profile_state);
        }
        break;
    case CONNECTION_STATE_EVENT_TIMER_ID + 1:
        {
            app_stop_timer(&usb_cmd_connection_event_timer[1]);
            gaming_app_bt_connection_state_event(1, t_gaming_ctrl[1].conn_state,
                                                 t_gaming_ctrl[1].profile_state);
        }
        break;
#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
    case HEADSET_LOCK_ACK_TIMER_ID:
        {
            app_stop_timer(&headset_lock_ack_timer);
            new_read_data[10] = HEADSET_LOCK_ACK_TIMEOUT;
            app_usb_hid_send_bt_ctrl_data(11, new_read_data);
        }
        break;
#endif
    default:
        break;
    }
}

void gaming_app_ctrl_cfg_init(void)
{
    app_timer_reg_cb(gaming_app_ctrl_cback, &usb_cmd_control_timer_queue_id);
}

#endif
