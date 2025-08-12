#include <string.h>
#include <bt_types.h>
#include "trace.h"

#include "os_mem.h"
#include "rtl876x.h"
#include "app_usb_hid.h"
#include "app_usb_dongle_test.h"
#include "rtl876x_wdg.h"
#include "app_src_storage_target_dev.h"
#include "app_main.h"
#include "app_ctrl_cfg.h"

#ifdef BTDONGLE_BQB_MODE_ENABLE
#include "bt_a2dp.h"
#include "bt_avrcp.h"
#include "app_src_policy.h"
#include "bt_hfp_ag.h"
#include "bt_bond.h"

#include "app_bqb_mode_switch.h"
#endif

#ifdef BTDONGLE_BQB_MODE_ENABLE
typedef enum
{
    BQB_PROFILE_A2DP,
    BQB_PROFILE_AVRCP,
    BQB_PROFILE_HOGP,
    BQB_PROFILE_HFP,
    BQB_PROFILE_IOPT,
    BQB_MODE_SWITCH = 0x80,
} BQB_PROFILE_TYPE;

typedef enum
{
    BQB_A2DP_RELEASE,
    BQB_A2DP_CONNECT,
} BQB_A2DP_CMD_T;

typedef enum
{
    BQB_AVRCP_TG_CONNECT,
    BQB_AVRCP_TG_DISCONNECT,

} BQB_AVRCP_CMD_T;

typedef enum
{
    BQB_HOGP_CONNECT,
    BQB_HOGP_DISCOVER,
} BQB_HOGP_CMD_T;

typedef enum
{
    BQB_HFP_CALL_INCOMING       = 0x0,
    BQB_HFP_SCO_CONNECT         = 0x1,
    BQB_HFP_SCO_DISCONNECT      = 0x2,
    BQB_HFP_PSI                 = 0x3,
    BQB_HFP_DISCONNECT          = 0x4,
    BQB_HFP_CALL_END            = 0x5,
    BQB_HFP_CALL_ANSWER         = 0x6,
    BQB_HFP_CALL_OUT            = 0x7,
    BQB_HFP_CLEAR_CALL_OUT      = 0x8,
    BQB_HFP_SECOND_CALL         = 0x9,
    BQB_HFP_CALL_IN_UNCONNECTED = 0x0A,
    BQB_HFP_WBS_TEST_CASE       = 0x0B,
    BQB_HFP_CLCC_SET            = 0x0C,
    BQB_HFP_HOLD_AND_ACTIVE     = 0x0D,
    BQB_HFP_SET_RINGTONE        = 0x0E,
    BQB_HFP_INQUIRY_PTS         = 0x0F,
} BQB_HFP_CMD_T;


bool    wbs_test_case = false;
uint8_t iut_call_status = 0;
bool    bqb_open_inquiry = false;

uint8_t pts_addr[6] = {0x00};
#endif

void app_led_set_custom_config(uint8_t led, uint8_t *config);
void app_usb_hid_test_loop_back(uint8_t length, uint8_t *data);

#ifdef BTDONGLE_BQB_MODE_ENABLE
static void bqb_cmd_profile_a2dp_proc(uint8_t *data, uint8_t length)
{
    uint8_t addr[6];
    for (uint8_t i = 0; i < 6; i++)
    {
        addr[i] = data[6 - i];
    }
    USB_PRINT_TRACE1("bqb_cmd_profile_a2dp_proc cmd %x", data[0]);
    if (data[0] == BQB_A2DP_RELEASE)
    {
        bt_a2dp_disconnect_req(addr);
    }
    else if (data[0] == BQB_A2DP_CONNECT)
    {
        key_short_press_handle(0);
    }
}

static void bqb_cmd_profile_avrcp_proc(uint8_t *data, uint8_t length)
{
    USB_PRINT_TRACE1("bqb_cmd_profile_avrcp_proc cmd %x", data[0]);

    uint8_t addr[6];
    for (uint8_t i = 0; i < 6; i++)
    {
        addr[i] = data[6 - i];
    }

    if (data[0] == BQB_AVRCP_TG_CONNECT)
    {
        bt_avrcp_connect_req(addr);
    }
    else if (data[0] == BQB_AVRCP_TG_DISCONNECT)
    {
        bt_avrcp_disconnect_req(addr);
    }
}

static void bqb_cmd_profile_hogp_proc(uint8_t *data, uint8_t length)
{
    USB_PRINT_TRACE1("bqb_cmd_profile_hogp_proc cmd %x", data[0]);

    if (data[0] == BQB_HOGP_CONNECT)
    {
    }
    else if (data[0] == BQB_HOGP_DISCOVER)
    {
    }
}

static void bqb_cmd_profile_hfp_proc(uint8_t *data, uint8_t length)
{
    USB_PRINT_TRACE1("bqb_cmd_profile_hfp_proc cmd %x", data[0]);

    uint8_t addr[6];
    for (uint8_t i = 0; i < 6; i++)
    {
        addr[i] = data[6 - i];
    }
    if (data[0] == BQB_HFP_CALL_INCOMING)
    {
        bt_hfp_ag_call_incoming(addr, "1234567", 8, 129);
    }
    else if (data[0] == BQB_HFP_SCO_CONNECT)
    {
        uint8_t ret = bt_hfp_ag_bqb_audio_connect_req(addr);
    }
    else if (data[0] == BQB_HFP_SCO_DISCONNECT)
    {
        bt_hfp_ag_audio_disconnect_req(addr);
    }
    else if (data[0] == BQB_HFP_PSI)
    {
        bt_hfp_ag_indicator_event_report(addr, data[7], data[8]);
    }
    else if (data[0]  == BQB_HFP_DISCONNECT)
    {
        bt_hfp_ag_disconnect_req(addr);
    }
    else if (data[0] == BQB_HFP_CALL_END)
    {
        bt_hfp_ag_call_terminate(addr);
    }
    else if (data[0] == BQB_HFP_CALL_ANSWER)
    {
        bt_hfp_ag_call_answer(addr);
    }
    else if (data[0] == BQB_HFP_CALL_OUT)
    {
        char call_num[] = "1234567";
        bt_hfp_ag_call_out(addr, call_num, sizeof(call_num));
    }
    else if (data[0] == BQB_HFP_CLEAR_CALL_OUT)
    {
        bt_hfp_ag_clear_call_out(addr);
    }
    else if (data[0] == BQB_HFP_SECOND_CALL)
    {
        bt_hfp_ag_second_call_incoming(addr, "7654321", 8, 129);
    }
    else if (data[0] == BQB_HFP_CALL_IN_UNCONNECTED)
    {
        iut_call_status = data[7];
    }
    else if (data[0] == BQB_HFP_WBS_TEST_CASE)
    {
        wbs_test_case = true;
    }
    else if (data[0] == BQB_HFP_CLCC_SET)
    {
        //bt_audio_info_set(data + 7, 6);
    }
    else if (data[0] == BQB_HFP_HOLD_AND_ACTIVE)
    {
        bt_hfp_ag_hold_and_active(addr);
    }
    else if (data[0] == BQB_HFP_SET_RINGTONE)
    {
        bt_hfp_ag_in_band_ringing_set(addr, false);
    }
    else if (data[0] == BQB_HFP_INQUIRY_PTS)
    {
        memcpy(pts_addr, addr, 6);
        bqb_open_inquiry = true;
        gap_br_start_inquiry(false, 10);
    }
}

static void bqb_cmd_profile_iopt_proc(uint8_t *data, uint8_t length)
{
    USB_PRINT_TRACE1("bqb_cmd_profile_iopt_proc cmd %x", data[0]);

    uint8_t addr[6];
    for (uint8_t i = 0; i < 6; i++)
    {
        addr[i] = data[6 - i];
    }

    if (data[0] == 0)
    {
        bt_bond_delete(addr);
    }
    else if (data[0] == 1)
    {
        if (data[7] == 0)
        {
            gap_br_set_radio_mode(GAP_RADIO_MODE_CONNECTABLE, false, 0);
        }
        else
        {
            gap_br_set_radio_mode(GAP_RADIO_MODE_VISIBLE_CONNECTABLE, false, 0);
        }
    }
    else if (data[0] == 2)
    {
        T_GAP_UUID_DATA uuid;

        uuid.uuid_16 = UUID_HANDSFREE;
        gap_br_start_sdp_discov(addr, GAP_UUID16, uuid);
    }

}
static void bqb_cmd_mode_switch(uint8_t *data, uint8_t length)
{
    uint8_t bqb_mode = data[0]; //bqb_mode + "BQBMOD"
    uint8_t mode_sync[6] = {'B', 'Q', 'B', 'M', 'O', 'D'};
    for (uint8_t i = 0; i < sizeof(mode_sync); i++)
    {
        if (data[i + 1] != mode_sync[i])
        {
            return;
        }
    }

    if (bqb_mode == 1)
    {
        app_enable_bqb_mode();
    }
    else if (bqb_mode == 0)
    {
        app_disable_bqb_mode();
    }

}

static void app_usb_hid_bqb_cmd_proc(uint8_t *data, uint8_t length)
{
    if (!app_is_bqb_mode())
    {
        if (data[0] != BQB_MODE_SWITCH)
        {
            return;
        }
    }
    switch (data[0])
    {
    case BQB_PROFILE_A2DP:
        bqb_cmd_profile_a2dp_proc(data + 1, length - 1);
        break;
    case BQB_PROFILE_AVRCP:
        bqb_cmd_profile_avrcp_proc(data + 1, length - 1);
        break;
    case BQB_PROFILE_HOGP:
        bqb_cmd_profile_hogp_proc(data + 1, length - 1);
        break;
    case BQB_PROFILE_HFP:
        bqb_cmd_profile_hfp_proc(data + 1, length - 1);
        break;
    case BQB_PROFILE_IOPT:
        bqb_cmd_profile_iopt_proc(data + 1, length - 1);
        break;
    case BQB_MODE_SWITCH:
        bqb_cmd_mode_switch(data + 1, length - 1);
        break;
    default:
        break;
    }
}
#endif

void app_usb_hid_handle_set_test_cmd(uint8_t *data, uint8_t length)
{
    USB_PRINT_TRACE3("app_usb_hid_handle_set_test_cmd ID %x, length 0x%x, %b", data[0], length,
                     TRACE_BINARY(length, data));
    switch (data[1])
    {
    case TEST_CMD_ID_LOOP_BACK:
        app_usb_hid_test_loop_back(length, data);
        break;
    case TEST_CMD_ID_SET_LED:
        app_led_set_custom_config(data[2], &data[3]);
        break;
    case TEST_CMD_ID_HID_INPUT:
        app_usb_hid_send_report((SEND_TYPE)data[2], data[3], &data[9], data[4],
                                *(uint32_t *)&data[5]); //2tp, 3id,4length,5pnp,9data
        break;
    case TEST_CMD_ID_DEV_INFO:
        app_usb_hid_handle_dev_info(data);
        break;
    case TEST_CMD_ID_RESET:
        WDG_SystemReset(RESET_ALL);
        break;
    case TEST_CMD_ID_TEST_MOD:
        {

            uint8_t rdata[] = {TEST_CMD_ID_TEST_MOD, data[2], 0x00};
            USB_PRINT_TRACE0("Enter test mode");
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);

            switch (data[2])
            {
            case 0 :
                {
                    extern void switch_into_single_tone_test_mode(void);
                    switch_into_single_tone_test_mode();
                    break;
                }
            case 2 :
                {
                    extern void dut_hci_cmd_write_scan_enable(uint8_t);
                    dut_hci_cmd_write_scan_enable(3);
                    break;
                }
            case 3 :
                {
                    extern void dut_hci_cmd_set_event_filter(void);
                    dut_hci_cmd_set_event_filter();
                    break;
                }
            case 4 :
                {
                    extern void dut_hci_cmd_write_page_scan_activity(uint16_t, uint16_t);
                    dut_hci_cmd_write_page_scan_activity(0x0200, 0x0012);
                    break;
                }
            case 5 :
                {
                    extern void dut_hci_cmd_enable_dut_mode(void);
                    dut_hci_cmd_enable_dut_mode();
                    break;
                }
            case 6 :
                {
                    extern void dut_hci_cmd_write_inquiry_scan_type(uint8_t);
                    dut_hci_cmd_write_inquiry_scan_type(0);
                    break;
                }
            case 7 :
                {
                    extern void dut_hci_cmd_write_inquiry_scan_activity(uint16_t, uint16_t);
                    dut_hci_cmd_write_inquiry_scan_activity(0x0200, 0x0012);
                    break;
                }
            default:
                {
                    USB_PRINT_TRACE1("Enter test mode: unsupported cmd %d", data[2]);
                    break;
                }
            }
        }
        break;
    case TEST_CMD_ID_CLEAR_DEV_INFO:
        {
            uint8_t dev_type = data[2];
            app_src_storage_clear_target_dev(dev_type);
            break;
        }
    case TEST_CMD_ID_CHANGE_SYS_CFG:
        {
            bool app_cfu_test_change_sys_cfg(uint16_t offset, uint8_t *data, uint8_t length);
            uint16_t offset = *(uint16_t *)(&data[2]);
            uint8_t length = *(uint8_t *)(&data[4]);
            if (app_cfu_test_change_sys_cfg(offset, &data[5], length))
            {
                WDG_SystemReset(RESET_ALL);
            }
            break;
        }
#ifdef BTDONGLE_BQB_MODE_ENABLE
    case TEST_CMD_ID_BQB:
        {
            app_usb_hid_bqb_cmd_proc(data + 2, length - 2);
            app_usb_hid_send_report(HID_IF_TEST, 0, &data[1], 60, 0);
            break;
        }
#endif
    //////////////////////////////////////////////////////
    //Microsoft CMD
    case TEST_CMD_ID_MS_SET_LED:
        {
            uint8_t rdata[] = {TEST_CMD_ID_MS_SET_LED, 0x00, 0x00};
            app_led_set_custom_config(data[3], &data[4]);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
            break;
        }
    case TEST_CMD_ID_MS_GET_FW_VER:
        {
            uint8_t rdata[9] = {TEST_CMD_ID_MS_GET_FW_VER, 0x00, 0x06}, bcd;
            uint32_t version = 0;
            uint32_t app_cfu_get_version(bool active);
            version = app_cfu_get_version(true);
            bcd = version >> 24;
            rdata[3] = (bcd / 10) << 4 | (bcd % 10);
            bcd = (version >> 8) & 0xFFFF;
            rdata[4] = (bcd / 10) << 4 | (bcd % 10);
            memcpy(&rdata[5], &version, 4);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
            break;
        }
    case TEST_CMD_ID_GET_BT_ADDR:
        {
            uint8_t rdata[9] = {TEST_CMD_ID_GET_BT_ADDR, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            uint8_t bd0[6] = {0, 0, 0, 0, 0, 0};
            if (memcmp(app_db.factory_addr, bd0, 6) == 0)
            {
                rdata[1] = 0x01;
            }
            memcpy(&rdata[3], app_db.factory_addr, 6);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
            break;
        }
    case TEST_CMD_ID_MS_GET_SERIAL_NUM:
        {
            uint8_t rdata[33] = {TEST_CMD_ID_MS_GET_SERIAL_NUM, 0x00};
            uint8_t length = MIN(*((uint8_t *)MSFT_SN_ADDR), 30);
            rdata[2] = length;
            memcpy(&rdata[3], (uint8_t *)(MSFT_SN_ADDR + 2), length);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, length + 3, 0);
            break;
        }
    case TEST_CMD_ID_MS_GET_COLOR_CODE:
        {
            uint8_t rdata[33] = {TEST_CMD_ID_MS_GET_COLOR_CODE, 0x00};
            uint8_t length = MIN(*((uint8_t *)MSFT_COLOR_ADDR), 30);
            rdata[2] = length;
            memcpy(&rdata[3], (uint8_t *)(MSFT_COLOR_ADDR + 2), length);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, length + 3, 0);
            break;
        }
    case TEST_CMD_ID_MS_GET_HW_VERSION:
        {
            uint8_t rdata[7] = {TEST_CMD_ID_MS_GET_HW_VERSION, 0x00, 0x04};
            memcpy(&rdata[3], (uint8_t *)MSFT_HW_VERSION_ADDR, 4);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
            break;
        }

    case MS_CMD_ID_GET_VERSION:
        app_usb_hid_ctrl_cmd_get_version();
        break;

    case MS_CMD_ID_LEGACY_START_SCAN:
        app_usb_hid_ctrl_cmd_legacy_scan_start();
        break;

    case MS_CMD_ID_LEGACY_STOP_SCAN:
        app_usb_hid_ctrl_cmd_legacy_scan_stop();
        break;

    case MS_CMD_ID_LE_START_SCAN:
        app_usb_hid_ctrl_cmd_le_scan_start();
        break;

    case MS_CMD_ID_LE_SCAN_STOP:
        app_usb_hid_ctrl_cmd_le_scan_stop();
        break;

    case MS_CMD_ID_GET_DONGLE_NAME:
        app_usb_hid_ctrl_cmd_get_dongle_name();
        break;
    case MS_CMD_ID_SET_DONGLE_NAME:
        app_usb_hid_ctrl_cmd_change_dongle_name(data);
        break;
    case MS_CMD_ID_PAIR_TO_DEVICE:
        app_usb_hid_ctrl_cmd_pair_to_device(data);
        break;
    case MS_CMD_ID_GET_PAIR_STATE:
        app_usb_hid_ctrl_cmd_get_pair_state(data);
        break;
    case MS_CMD_ID_GET_PAIRED_DEVICE_LIST:
        app_usb_hid_ctrl_cmd_get_paired_device_list(data);
        break;
    case MS_CMD_ID_REMOVE_DEVICE:
        app_usb_hid_ctrl_cmd_remove_device(data);
        break;
    case MS_CMD_ID_GET_BT_CONN_STATE:
        app_usb_hid_ctrl_cmd_get_bt_conn_state(data);
        break;
    case MS_CMD_ID_SET_BT_CONN_STATE:
        app_usb_hid_ctrl_cmd_set_bt_conn_state(data);
        break;
    case MS_CMD_ID_GET_LED_BRIGHTNESS:
        app_usb_hid_ctrl_cmd_get_led_brightness(data);
        break;
    case MS_CMD_ID_SET_LED_BRIGHTNESS:
        app_usb_hid_ctrl_cmd_set_led_brightness(data);
        break;

    case MS_CMD_ID_QUERY_BT_CONN_STATE:
        app_usb_hid_ctrl_cmd_query_bt_conn_state(data);
        break;

    case MS_CMD_ID_GET_DONGLE_CAPABILITY:
        app_usb_hid_ctrl_cmd_get_dongle_capability();
        break;

    case MS_CMD_ID_GET_LINK_KEY:
        app_usb_hid_ctrl_cmd_get_link_key(data);
        break;
    case MS_CMD_ID_RF_TEST_MODE:
        app_usb_hid_ctrl_cmd_enable_rf_test_mode(data);
        break;

    case MS_CMD_ID_GET_DISCONN_REASON:
        app_usb_hid_ctrl_cmd_get_disconn_reason(data);
        break;

    default:
        DFU_PRINT_ERROR1("test cmd unknown %x", data[1]);
        break;
    }

}

inline void app_usb_hid_send_test_data(uint8_t length, uint8_t *data)
{
    app_usb_hid_send_report(HID_IF_TEST, 0, data, length, 0);
}

extern void app_hid_interrupt_in(uint8_t *data, uint8_t data_size);
extern uint8_t app_usb_hid_interrupt_in_buff[APP_USB_HID_MAX_IN_SIZE];
void app_usb_hid_test_loop_back(uint8_t length, uint8_t *data)
{
    memcpy(app_usb_hid_interrupt_in_buff, &data[3], data[2]);
    app_hid_interrupt_in(app_usb_hid_interrupt_in_buff, data[2]);
}

void app_usb_hid_handle_dev_info(uint8_t *data)
{
    T_APP_SRC_TARGET_DEV save_dev;

    memset(&save_dev, 0, sizeof(save_dev));

    save_dev.dev_type = data[2];
    save_dev.bt_mode = data[3];
    save_dev.addr_type = data[4];
    memcpy(save_dev.remote_bd, data + 5, 6);

    app_src_storage_set_target_dev(save_dev);
}
