#include "trace.h"
#include "app_usb_hid.h"
#include "app_ctrl_cfg.h"
#include "gap_br.h"
#include "gap_scan.h"
#include "gap.h"
#include "app_io_msg.h"
#include "btm.h"
#include "ftl.h"
#include "app_usb_dongle_test.h"
#include "app_general_policy.h"
#include "app_src_ble.h"
#include "app_src_storage_target_dev.h"
#include "bt_bond.h"
#include "app_cfg.h"
#include "app_led_ctl.h"
#include "ual_bluetooth.h"

#include "app_vendor_cfg.h"
#include "app_usb_layer.h"
#include "app_main.h"
#include "app_bond.h"
#include "test_mode.h"
#include "app_mmi.h"

#ifdef LEGACY_BT_GENERAL
#define USB_HID_CTRL_TEST_CMD_ENABLE        0

#define USB_HID_CTRL_CMD_ACCEPT             0x00
#define USB_HID_CTRL_CMD_NOT_ACCEPT         0x01
#define USB_HID_CTRL_CMD_PARAM_ERR          0x02

#define USB_HID_CTRL_CMD_SCAN_TYPE_BOTH     0x00
#define USB_HID_CTRL_CMD_SCAN_TYPE_LEGACY   0x01
#define USB_HID_CTRL_CMD_SCAN_TYPE_LE       0x02

#define LE_ADV_REPORT_NON_CONNECTABLE       0

bool inquiry_start = false;
bool scan_start = false;

static bool legacy_in_scan = false;
static bool le_in_scan = false;

extern bool teams_is_off;

uint8_t rf_test_mode = 0;

#define APP_CTRL_CFG_DONGLE_USB_NAME_OFFSET 4740

static bool pair2dev_received[3] = {false, false, false};
T_APP_CTRL_PAIR_STATE pair_state[3] = {PAIR_STATE_IDLE, PAIR_STATE_IDLE, PAIR_STATE_IDLE};

T_APP_LE_SCAN_INFO le_adv_report_info;
T_APP_LE_SCAN_INFO_REPORT_STATE le_adv_report_state;


bool app_usb_hid_ctrl_cmd_get_version(void)
{
    uint8_t rdata[5] = {MS_CMD_ID_GET_VERSION, USB_HID_CTRL_CMD_ACCEPT, 0x02};
    rdata[3] = (uint8_t)RTL8763EAU_VER_MAJOR;
    rdata[4] = (uint8_t)RTL8763EAU_VER_MINOR;

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);

    return true;
}

bool app_usb_hid_ctrl_cmd_legacy_scan_start(void)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    uint8_t rdata[3] = {MS_CMD_ID_LEGACY_START_SCAN, USB_HID_CTRL_CMD_NOT_ACCEPT, 0x00};

    if (legacy_in_scan)
    {
        APP_PRINT_WARN0("app_usb_hid_ctrl_cmd_legacy_scan_start: already in scan");

        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);

        return false;
    }

    //poweron reconn can should be cancelled before inquiry
    src_stop_poweron_reconn();

    ret = gap_br_start_inquiry(false, 10);
    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("app_usb_hid_ctrl_cmd_legacy_scan_start: error %d", ret);

        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);

        return false;
    }

    //wait for inquiry rsp to send back result

    return true;
}

bool app_usb_hid_ctrl_cmd_legacy_scan_stop(void)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    uint8_t rdata[3] = {MS_CMD_ID_LEGACY_STOP_SCAN, USB_HID_CTRL_CMD_NOT_ACCEPT, 0x00};

    ret = gap_br_stop_inquiry();

    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("app_usb_hid_ctrl_cmd_legacy_scan_stop: error %d", ret);

        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
    }

    //wait for inquiry cancel complete event to send back result

    return true;
}

bool app_usb_hid_ctrl_cmd_le_scan_start(void)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    uint8_t rdata[3] = {MS_CMD_ID_LE_START_SCAN, USB_HID_CTRL_CMD_NOT_ACCEPT, 0x00};

    if (!app_vendor_cfg_is_audio_hid())
    {
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return false;
    }

    if (le_in_scan)
    {
        APP_PRINT_WARN0("app_usb_hid_ctrl_cmd_le_scan_start: already in scan");

        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);

        return false;
    }

    ret = le_scan_start();
    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("app_usb_hid_ctrl_cmd_le_scan_start: error %d", ret);

        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);

        return false;
    }

    //wait for gap scan state changed event to send back result

    return true;
}

bool app_usb_hid_ctrl_cmd_le_scan_stop(void)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
    uint8_t rdata[3] = {MS_CMD_ID_LE_SCAN_STOP, USB_HID_CTRL_CMD_NOT_ACCEPT, 0x00};

    if (!app_vendor_cfg_is_audio_hid())
    {
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return false;
    }

    ret = le_scan_stop();
    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("app_usb_hid_ctrl_cmd_le_scan_stop: error %d", ret);

        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
    }

    //wait for gap scan state changed event to send back result

    return true;
}

static bool app_flash_save_dongle_usb_name(uint8_t *name, uint8_t len)
{
    uint16_t offset = APP_CTRL_CFG_DONGLE_USB_NAME_OFFSET;
    T_APP_DONGLE_NAME temp;
    uint32_t ret = 0;

    temp.is_valid = 1;
    temp.length = len;
    memcpy(temp.name, name, len);
    temp.name[len] = '\0';

    ret = ftl_save_to_storage(&temp, offset, sizeof(T_APP_DONGLE_NAME));

    if (ret)
    {
        APP_PRINT_WARN1("app_flash_save_dongle_usb_name: error %08x", ret);
        return false;
    }

    return true;
}

bool app_storage_get_dongle_usb_name(T_APP_DONGLE_NAME *target_name)
{
    uint16_t offset;
    uint32_t has_error;
    uint8_t size;

    offset = APP_CTRL_CFG_DONGLE_USB_NAME_OFFSET;
    size = sizeof(T_APP_DONGLE_NAME);
    has_error = ftl_load_from_storage(target_name, offset, size);
    if (has_error)
    {
        memset(target_name, 0, size);
        return false;
    }

    return true;
}

bool app_usb_hid_ctrl_cmd_get_dongle_name(void)
{
    T_APP_DONGLE_NAME name_info;
    uint8_t rdata[53] = {MS_CMD_ID_GET_DONGLE_NAME, 0x00};

    if (app_storage_get_dongle_usb_name(&name_info) && (name_info.is_valid == 1))
    {
        rdata[2] = name_info.length;
        memcpy(&rdata[3], name_info.name, name_info.length);
    }
    else
    {
        if (app_vendor_cfg_is_audio_hid())
        {
            rdata[2] = sizeof(DEVICE_CUSTOMER_NAME_AUDIO_HID);
            memcpy(&rdata[3], DEVICE_CUSTOMER_NAME_AUDIO_HID, rdata[2]);
        }
        else
        {
            rdata[2] = sizeof(DEVICE_CUSTOMER_NAME_AUDIO_ONLY);
            memcpy(&rdata[3], DEVICE_CUSTOMER_NAME_AUDIO_ONLY, rdata[2]);
        }
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, rdata[2] + 3, 0);

    return true;
}

bool app_usb_hid_ctrl_cmd_change_dongle_name(uint8_t *data)
{
    uint8_t len = data[2];
    uint8_t name[USB_NAME_MAX_LEN];
    uint8_t rdata[3] = {MS_CMD_ID_SET_DONGLE_NAME, USB_HID_CTRL_CMD_NOT_ACCEPT, 0x00};

    if (len >= USB_NAME_MAX_LEN)
    {
        len = USB_NAME_MAX_LEN - 1;
    }

    memcpy(name, &(data[3]), len);
    name[len] = '\0';

    if (app_flash_save_dongle_usb_name(name, len))
    {
        rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
    return true;
}

static void ctrl_cmd_handle_inquiry_rsp(uint16_t cause)
{
    uint8_t rdata[3] = {MS_CMD_ID_LEGACY_START_SCAN, USB_HID_CTRL_CMD_NOT_ACCEPT, 0x00};

    if (!legacy_in_scan)
    {
        if (!cause)
        {
            inquiry_start = true;
            legacy_in_scan = true;

            rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
        }
        else
        {
            APP_PRINT_ERROR1("ctrl_cmd_handle_inquiry_rsp: %d", cause);
            inquiry_start = false;
        }

        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
    }
}

static uint8_t get_eir_data_significant_len(uint8_t *data, uint8_t total_len)
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

/*
*   1st fragment event of one inquiry result report:
*       cmd_id(1) + payload_len(1) + EIR_offset(1, =0) + EIR_total_len(1)   \
*       bd_addr(6) + COD(3) + RSSI(1) + EIR_data(n)
*
*   subsequent fragment event of one inquiry result report:
*       cmd_id(1) + payload_len(1) + EIR_offset(1, 46~217) + EIR_data(n)
*
*/

/*static void ctrl_cmd_legacy_scan_result_report(T_BT_EVENT_PARAM_INQUIRY_RESULT inquiry_result)
{
    uint8_t i = 0;
    uint8_t rdata[60] = {MS_CMD_ID_LEGACY_SCAN_LIST_UPDATE, 0x00, 0x00};
    uint8_t significant_len = 0;
    uint8_t eir_len_left = 0;

    APP_PRINT_INFO4("ctrl_cmd_legacy_scan_result_report: %s, %08x, %d, %02x",
                    TRACE_BDADDR(inquiry_result.bd_addr),
                    inquiry_result.cod,
                    inquiry_result.rssi,
                    inquiry_result.eir_len);

    significant_len = get_eir_data_significant_len(inquiry_result.p_eir, inquiry_result.eir_len);

    //if (significant_len > 0)
    {
        //total eir len
        rdata[3] = significant_len;

        for (i = 0; i < 6; i++)
        {
            rdata[4 + i] = inquiry_result.bd_addr[5 - i];
        }

        rdata[10] = (uint8_t)((inquiry_result.cod & 0x00FF0000) >> 16);
        rdata[11] = (uint8_t)((inquiry_result.cod & 0x0000FF00) >> 8);
        rdata[12] = (uint8_t)(inquiry_result.cod & 0x000000FF);

        rdata[13] = (uint8_t)(inquiry_result.rssi);

        if (significant_len <= 46)
        {
            rdata[1] = rdata[3] + 2;
            memcpy(rdata + 14, inquiry_result.p_eir, significant_len);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, rdata[1] + 2, 0);
        }
        else
        {
            rdata[1] = 58;
            memcpy(rdata + 14, inquiry_result.p_eir, 46);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, rdata[1] + 2, 0);

            eir_len_left = significant_len - 46;

            while (eir_len_left > 57)
            {
                rdata[1] = 58;
                rdata[2] = significant_len - eir_len_left;
                memcpy(rdata + 3, inquiry_result.p_eir + rdata[2], 57);
                app_usb_hid_send_report(HID_IF_TEST, 0, rdata, rdata[1] + 2, 0);

                eir_len_left -= 57;
            }

            if (eir_len_left > 0)
            {
                rdata[1] = eir_len_left + 1;
                rdata[2] = significant_len - eir_len_left;
                memcpy(rdata + 3, inquiry_result.p_eir + rdata[2], eir_len_left);
                app_usb_hid_send_report(HID_IF_TEST, 0, rdata, rdata[1] + 2, 0);
            }
        }
    }
}*/

static void ctrl_cmd_handle_inquiry_cancel_cmpl(uint16_t cause)
{
    uint8_t rdata[3] = {MS_CMD_ID_LEGACY_STOP_SCAN, USB_HID_CTRL_CMD_NOT_ACCEPT, 0x00};

    if (!cause)
    {
        inquiry_start = false;
        legacy_in_scan = false;

        rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
    }
    else
    {
        APP_PRINT_ERROR1("ctrl_cmd_handle_inquiry_cancel_cmpl: %d", cause);

        inquiry_start = true;
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
}

static void ctrl_cmd_bt_mgr_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_BT_EVENT_PARAM *param = event_buf;

    switch (event_type)
    {
    case BT_EVENT_INQUIRY_RSP:
        {
            ctrl_cmd_handle_inquiry_rsp(param->inquiry_rsp.cause);
        }
        break;

    case BT_EVENT_INQUIRY_RESULT:
        {
            //ctrl_cmd_handle_inquiry_res(param->bt_inquiry_result);
            //ctrl_cmd_legacy_scan_result_report(param->inquiry_result);
            app_scan_result_report_event(param->inquiry_result);
        }
        break;

    case BT_EVENT_INQUIRY_CMPL:
        {
            gap_br_start_inquiry(false, 10);
        }
        break;

    case BT_EVENT_INQUIRY_CANCEL_RSP:
        {
            ctrl_cmd_handle_inquiry_cancel_cmpl(param->inquiry_cancel_rsp.cause);
        }
        break;
    default:
        break;
    }
}

void app_ble_handle_scan_start_stop(T_GAP_DEV_STATE old_state, T_GAP_DEV_STATE new_state,
                                    uint16_t cause)
{
    uint8_t rdata[3] = {0x00, 0x00, 0x00};


    if (old_state.gap_scan_state == GAP_SCAN_STATE_START)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            scan_start = true;
            le_in_scan = true;

            rdata[0] = MS_CMD_ID_LE_START_SCAN;
            rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);

            le_adv_report_state = LE_SCAN_INFO_REPORT_STATE_IDLE;
        }
        else
        {
            scan_start = false;

            rdata[0] = MS_CMD_ID_LE_START_SCAN;
            rdata[1] = USB_HID_CTRL_CMD_NOT_ACCEPT;
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        }
    }

    if (old_state.gap_scan_state == GAP_SCAN_STATE_STOP)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            scan_start = false;
            le_in_scan = false;

            rdata[0] = MS_CMD_ID_LE_SCAN_STOP;
            rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        }
        else
        {
            scan_start = true;

            rdata[0] = MS_CMD_ID_LE_SCAN_STOP;
            rdata[1] = USB_HID_CTRL_CMD_NOT_ACCEPT;
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        }
    }
}

/*
*   1st fragment event of one LE ADV report:
*       cmd_id(1) + payload_len(1) + EIR_offset(1, =0) + ADV_data_total_len(1)   \
*       addr_type(1) + bd_addr(6) + RSSI(1) + ADV_data(n)
*
*   2nd fragment event of one LE ADV report:
*       cmd_id(1) + payload_len(1) + ADV_offset(1, 48) + EIR_data(n)
*
*/

static void app_src_report_le_adv(T_APP_LE_SCAN_INFO *adv_info)
{
    uint8_t i = 0;
    uint8_t rdata[60] = {MS_CMD_ID_LE_SCAN_LIST_UPDATE, 0x00, 0x00};

    if (adv_info != NULL)
    {
        rdata[3] = adv_info->data_len;
        rdata[4] = adv_info->addr_type;

        for (i = 0; i < 6; i++)
        {
            rdata[5 + i] = adv_info->addr[5 - i];
        }

        rdata[11] = (uint8_t)adv_info->rssi;

        if (adv_info->data_len <= 48)
        {
            rdata[1] = 10 + adv_info->data_len;
            memcpy(rdata + 12, adv_info->data, adv_info->data_len);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, rdata[1] + 2, 0);
        }
        else
        {
            rdata[1] = 58;
            memcpy(rdata + 12, adv_info->data, 48);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, rdata[1] + 2, 0);

            rdata[1] = 1 + adv_info->data_len - 48;
            rdata[2] = 48;
            memcpy(rdata + 3, adv_info->data + 48, adv_info->data_len - 48);
            app_usb_hid_send_report(HID_IF_TEST, 0, rdata, rdata[1] + 2, 0);
        }
    }
}

static void app_src_le_adv_append_scan_rsp(T_APP_LE_SCAN_INFO *adv_report_info,
                                           T_LE_SCAN_INFO *scan_info)
{
    memcpy(adv_report_info->data + adv_report_info->data_len,
           scan_info->data, scan_info->data_len);

    adv_report_info->data_len += scan_info->data_len;
}

static void app_src_store_new_le_adv_report(T_APP_LE_SCAN_INFO *adv_report_info,
                                            T_LE_SCAN_INFO *scan_info)
{
    memset(adv_report_info, 0, sizeof(T_APP_LE_SCAN_INFO));

    memcpy(adv_report_info->addr, scan_info->bd_addr, 6);
    adv_report_info->addr_type = scan_info->remote_addr_type;
    adv_report_info->rssi = scan_info->rssi;
    adv_report_info->data_len = scan_info->data_len;

    memcpy(adv_report_info->data, scan_info->data, 31);
}

static void le_adv_report_handle_by_path(T_APP_LE_SCAN_INFO_HANDLE_PATH handle_path,
                                         T_APP_LE_SCAN_INFO *adv_report_info,
                                         T_LE_SCAN_INFO *scan_info)
{
    switch (handle_path)
    {
    case LE_SCAN_INFO_REPORT_NEW:
        {
            app_src_store_new_le_adv_report(adv_report_info, scan_info);
            app_src_report_le_adv(adv_report_info);
            le_adv_report_state = LE_SCAN_INFO_REPORT_STATE_IDLE;
        }
        break;

    case LE_SCAN_INFO_STORE_NEW:
        {
            app_src_store_new_le_adv_report(adv_report_info, scan_info);
            le_adv_report_state = LE_SCAN_INFO_REPORT_STATE_WAIT_SCAN_RSP;
        }
        break;

    case LE_SCAN_INFO_REPORT_OLD_STORE_NEW:
        {
            app_src_report_le_adv(adv_report_info);
            app_src_store_new_le_adv_report(adv_report_info, scan_info);
            le_adv_report_state = LE_SCAN_INFO_REPORT_STATE_WAIT_SCAN_RSP;
        }
        break;

    case LE_SCAN_INFO_REPORT_OLD_IGNORE_NEW:
        {
            app_src_report_le_adv(adv_report_info);
            le_adv_report_state = LE_SCAN_INFO_REPORT_STATE_IDLE;
        }
        break;

    case LE_SCAN_INFO_REPORT_APPEND_REPORT:
        {
            app_src_le_adv_append_scan_rsp(adv_report_info, scan_info);
            app_src_report_le_adv(adv_report_info);
            le_adv_report_state = LE_SCAN_INFO_REPORT_STATE_IDLE;
        }
        break;

    default:
        break;
    }
}

void app_src_handle_le_adv_report(T_LE_SCAN_INFO *scan_info)
{
    T_APP_LE_SCAN_INFO_HANDLE_PATH handle_path = LE_SCAN_INFO_REPORT_MAX;

    if (scan_info != NULL)
    {
        if (le_adv_report_state == LE_SCAN_INFO_REPORT_STATE_IDLE)
        {
            switch (scan_info->adv_type)
            {
            case GAP_ADV_EVT_TYPE_UNDIRECTED:
                handle_path = LE_SCAN_INFO_STORE_NEW;
                break;

#if (LE_ADV_REPORT_NON_CONNECTABLE == 1)
            case GAP_ADV_EVT_TYPE_SCANNABLE:
                handle_path = LE_SCAN_INFO_STORE_NEW;
                break;

            case GAP_ADV_EVT_TYPE_NON_CONNECTABLE:
                handle_path = LE_SCAN_INFO_REPORT_NEW;
                break;
#endif

            default:
                break;
            }
        }
        else if (le_adv_report_state == LE_SCAN_INFO_REPORT_STATE_WAIT_SCAN_RSP)
        {
            switch (scan_info->adv_type)
            {
            case GAP_ADV_EVT_TYPE_UNDIRECTED:
                handle_path = LE_SCAN_INFO_REPORT_OLD_STORE_NEW;
                break;

#if (LE_ADV_REPORT_NON_CONNECTABLE == 1)
            case GAP_ADV_EVT_TYPE_SCANNABLE:
                handle_path = LE_SCAN_INFO_REPORT_OLD_STORE_NEW;
                break;

            case GAP_ADV_EVT_TYPE_NON_CONNECTABLE:
                handle_path = LE_SCAN_INFO_REPORT_OLD_STORE_NEW;
                break;
#endif

            case GAP_ADV_EVT_TYPE_SCAN_RSP:
                if (!memcmp(le_adv_report_info.addr, scan_info->bd_addr, 6) &&
                    (le_adv_report_info.addr_type == scan_info->remote_addr_type))
                {
                    handle_path = LE_SCAN_INFO_REPORT_APPEND_REPORT;
                }
                else
                {
                    handle_path = LE_SCAN_INFO_REPORT_OLD_IGNORE_NEW;
                }
                break;

            default:
                break;
            }
        }

        le_adv_report_handle_by_path(handle_path, &le_adv_report_info, scan_info);
    }
}

void app_usb_hid_ctrl_cmd_pair_to_device(uint8_t *data)
{
    uint8_t bd_addr[6];
    uint8_t device_slot = data[3];
//    uint8_t addr_type = data[4];
    uint8_t rdata[3] = {MS_CMD_ID_PAIR_TO_DEVICE, USB_HID_CTRL_CMD_NOT_ACCEPT, 0x00};

    if (!app_vendor_cfg_is_audio_hid() && (device_slot != DEV_SLOT_HS))
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return;
    }

    for (int i = 0; i < 6; i++)
    {
        bd_addr[i] = data[10 - i];
    }

    if (device_slot <= DEV_SLOT_HOGP_2)
    {
        pair2dev_received[device_slot - 1] = true;
    }
    else
    {
        //err input param
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
    }

    if (device_slot == DEV_SLOT_HS)
    {
#ifdef LEGACY_BT_GENERAL
        if (src_connect_audio_device(bd_addr, 0))
        {
            rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
            pair_state[device_slot - 1] = PAIR_STATE_IN_PROCESS;
        }
#endif
        //app_src_storage_set_target_dev(DEV_SLOT_HS, DEV_MODE_LEGACY, 0x00, bd_addr, 0);
    }
    else if (device_slot <= DEV_SLOT_HOGP_2)
    {
//        if (src_ble_connect_hogp_device(device_slot - 2, bd_addr,
//                                        addr_type, 0))
        {
            rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
            pair_state[device_slot - 1] = PAIR_STATE_IN_PROCESS;
        }
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
}

void app_usb_hid_ctrl_cmd_get_pair_state(uint8_t *data)
{
    uint8_t device_slot = data[3];

    uint8_t rdata[5] = {MS_CMD_ID_GET_PAIR_STATE, 0x00, 0x02, device_slot, 0x02};

    if (!app_vendor_cfg_is_audio_hid() && (device_slot != DEV_SLOT_HS))
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return;
    }

    if (device_slot <= DEV_SLOT_HOGP_2)
    {
        if (pair2dev_received[device_slot - 1])
        {
            rdata[4] = pair_state[device_slot - 1];
        }
        else
        {
            rdata[4] = PAIR_STATE_IDLE;
        }
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
}

/* format:
    commandID(1) + status(1) + payload_length(1) + dev_slot(1) + addr_type(1) + \
    addr(6) + vid(2) + pid(2) + name_length(1) + name(n)

    total length: 16 + n, n <= 40
*/
void app_usb_hid_ctrl_cmd_get_paired_device_list(uint8_t *data)
{
    uint8_t device_slot = data[3];
    T_APP_SRC_TARGET_DEV dev_info;
    uint8_t rdata[60];
    uint8_t *p_data = NULL;
    uint8_t name_len = 0;
    uint8_t total_len = 13;     //payload len exclude name string

    memset(rdata, 0, 60);

    rdata[0] = MS_CMD_ID_GET_PAIRED_DEVICE_LIST;    //command ID
    rdata[1] = USB_HID_CTRL_CMD_ACCEPT;             //status
    rdata[2] = total_len;                           //payload length
    rdata[3] = device_slot;

    if (!app_vendor_cfg_is_audio_hid() && (device_slot != DEV_SLOT_HS))
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, total_len + 3, 0);
        return;
    }

    if ((device_slot >= DEV_SLOT_HS) && (device_slot <= DEV_SLOT_HOGP_2))
    {
        if (app_src_storage_get_target_dev(device_slot, &dev_info))
        {
            rdata[4] = dev_info.addr_type;

            //addr
            for (int i = 0; i < 6 ; i++)
            {
                rdata[i + 5] = dev_info.remote_bd[5 - i];
            }

            p_data = rdata + 11;

            //vid, pid
            BE_UINT16_TO_STREAM(p_data, dev_info.vid);
            BE_UINT16_TO_STREAM(p_data, dev_info.pid);

            name_len = strlen((const char *)dev_info.remote_name);
            BE_UINT8_TO_STREAM(p_data, name_len);

            memcpy(p_data, dev_info.remote_name, name_len);
            total_len += name_len;

            //payload length
            rdata[2] = total_len;
        }
    }
    else
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
    }

    //total_length = payload_length + 3
    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, total_len + 3, 0);
}

void app_usb_hid_ctrl_cmd_remove_device(uint8_t *data)
{
    uint8_t device_slot = data[3];

    if (!app_vendor_cfg_is_audio_hid() && (device_slot != DEV_SLOT_HS))
    {
        uint8_t rdata[3] = {MS_CMD_ID_REMOVE_DEVICE, USB_HID_CTRL_CMD_PARAM_ERR, 0x00};
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return;
    }

    if (device_slot == DEV_SLOT_HS)
    {
        pair2dev_received[device_slot - 1] = false;
        pair_state[device_slot - 1] = PAIR_STATE_IDLE;
        src_remove_audio_device();
        app_src_storage_clear_target_dev(device_slot);
    }
    else if (device_slot <= DEV_SLOT_HOGP_2)
    {
        pair2dev_received[device_slot - 1] = false;
        pair_state[device_slot - 1] = PAIR_STATE_IDLE;
//        src_ble_remove_hogp_device(device_slot - 2);
        app_src_storage_clear_target_dev(device_slot);
    }

    uint8_t rdata[3] = {MS_CMD_ID_REMOVE_DEVICE, 0x00, 0x00};
    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
}

void app_usb_hid_ctrl_cmd_get_link_key(uint8_t *data)
{
#if USB_HID_CTRL_TEST_CMD_ENABLE
    uint8_t device_slot = data[3];
    uint8_t rdata[20] = {MS_CMD_ID_GET_LINK_KEY, 0xFF, 0x11, device_slot};
    uint8_t link_key[16];
    T_APP_SRC_TARGET_DEV temp_dev;

    if (device_slot > DEV_SLOT_HOGP_2)
    {
        memset(&rdata[4], 0x00, 16);
    }
    else if (device_slot == DEV_SLOT_HS)
    {
        T_BT_LINK_KEY_TYPE type;
        bool status;

        if (app_src_storage_get_target_dev(TARGET_DEVICE_TYPE_HEADSET, &temp_dev))
        {
            status = bt_bond_key_get(temp_dev.remote_bd, link_key, (uint8_t *)&type);
            if (status)
            {
                rdata[1] = 0x00;
                memcpy(&rdata[4], link_key, 16);
            }
            else
            {
                memset(&rdata[4], 0x00, 16);
            }
        }
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
#endif
}

void app_usb_hid_ctrl_cmd_enable_rf_test_mode(uint8_t *data)
{
#if USB_HID_CTRL_TEST_CMD_ENABLE
    rf_test_mode = data[3];

    uint8_t rdata[3] = {MS_CMD_ID_RF_TEST_MODE, 0x00, 0x00};
    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
#endif
}

void app_usb_hid_ctrl_cmd_get_disconn_reason(uint8_t *data)
{
#if USB_HID_CTRL_TEST_CMD_ENABLE
    uint8_t device_slot = data[3];
    uint8_t rdata[20] = {MS_CMD_ID_GET_DISCONN_REASON, USB_HID_CTRL_CMD_PARAM_ERR, 0x01, 0x00};

    if (device_slot == DEV_SLOT_HS)
    {
        rdata[1] = 0x00;
        rdata[3] = src_get_disconn_reason();
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
#endif
}

void app_usb_hid_audio_device_pair_state_cb(uint8_t *bd_addr, T_APP_CTRL_PAIR_STATE new_pair_state,
                                            uint8_t *pin_code)
{
    uint8_t rdata[11] = {MS_CMD_ID_GET_PAIR_STATE, 0x00};
    uint8_t len = 5;

    if ((bd_addr == NULL) ||
        ((pin_code == NULL) &&
         (new_pair_state == PAIR_STATE_WAIT_PIN_CODE)))
    {
        APP_PRINT_ERROR0("app_usb_hid_audio_device_pair_state_cb: error input params");
        return;
    }

    rdata[3] = DEV_SLOT_HS;
    rdata[4] = (uint8_t)new_pair_state;

    if (new_pair_state == PAIR_STATE_WAIT_PIN_CODE)
    {
        rdata[2] = 0x08;    //param length
        memcpy(&rdata[5], pin_code, 6);
        len += 6;
    }
    else
    {
        rdata[2] = 0x02;
    }
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    extern uint8_t asp_new_pair_state;
    if (new_pair_state == PAIR_STATE_SUCCESS)
    {
        asp_new_pair_state = true;
        T_IO_MSG gpio_msg = {0};
        gpio_msg.type = IO_MSG_TYPE_USB_HID;
        gpio_msg.subtype = USB_HID_MSG_TYPE_ASP;
        gpio_msg.u.param = 0xd0;
        app_io_msg_send(&gpio_msg);
    }
#endif
    if ((new_pair_state == PAIR_STATE_SUCCESS) ||
        (new_pair_state == PAIR_STATE_FAIL))
    {
        pair_state[0] = new_pair_state;
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, len, 0);
}

void app_usb_hid_hogp_device_pair_state_cb(uint8_t index, uint8_t *bd_addr,
                                           uint8_t addr_type,
                                           T_APP_CTRL_PAIR_STATE new_pair_state,
                                           uint8_t *pin_code)
{
    uint8_t rdata[11] = {MS_CMD_ID_GET_PAIR_STATE, 0x00};
    uint8_t len = 5;

    if ((bd_addr == NULL) ||
        ((pin_code == NULL) &&
         (new_pair_state == PAIR_STATE_WAIT_PIN_CODE)))
    {
        APP_PRINT_ERROR0("app_usb_hid_audio_device_pair_state_cb: error input params");
        return;
    }

    if (index == 0)
    {
        rdata[3] = DEV_SLOT_HOGP_1;
    }
    else if (index == 1)
    {
        rdata[3] = DEV_SLOT_HOGP_2;
    }

    rdata[4] = (uint8_t)new_pair_state;

    if (new_pair_state == PAIR_STATE_WAIT_PIN_CODE)
    {
        rdata[2] = 0x08;    //param length
        memcpy(&rdata[5], pin_code, 6);
        len += 6;
    }
    else
    {
        rdata[2] = 0x02;
    }

    if ((new_pair_state == PAIR_STATE_SUCCESS) ||
        (new_pair_state == PAIR_STATE_FAIL))
    {
        pair_state[0] = new_pair_state;
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, len, 0);
}


/* format:
    commandID(1) + status(1) + payload_length(1) + dev_slot(1) + conn_state(1)
*/
void app_usb_hid_connection_state_notify(uint8_t dev_slot, uint8_t conn_state, uint8_t profile_mask)
{
    uint8_t rdata[6];

    memset(rdata, 0, 5);

    rdata[0] = MS_CMD_ID_GET_BT_CONN_STATE;
    rdata[2] = 3;

    if ((dev_slot >= DEV_SLOT_HS) && (dev_slot <= DEV_SLOT_HOGP_2))
    {
        rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
        rdata[3] = dev_slot;
        rdata[4] = conn_state;
        rdata[5] = profile_mask;
    }
    else
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, rdata[2] + 3, 0);
}

void app_usb_hid_ctrl_cmd_get_bt_conn_state(uint8_t *data)
{
    uint8_t dev_slot = data[3];
    uint8_t conn_state = 0;
    uint8_t profile_mask = 0;

    if (!app_vendor_cfg_is_audio_hid() && (dev_slot != DEV_SLOT_HS))
    {
        //force app_usb_hid_connection_state_notify to notify param error
        dev_slot = DEV_SLOT_HOGP_2 + 1;
    }

    switch (dev_slot)
    {
    case DEV_SLOT_HS:
        {
            conn_state = src_get_audio_device_conn_state();
            profile_mask = src_get_audio_device_profile_state();
        }
        break;

    case DEV_SLOT_HOGP_1:
    case DEV_SLOT_HOGP_2:
        {
//            src_get_hogp_device_conn_state(dev_slot - 2, &conn_state);
        }
        break;

    default:
        break;
    }

    app_usb_hid_connection_state_notify(dev_slot, conn_state, profile_mask);
}

void app_usb_hid_ctrl_cmd_set_bt_conn_state(uint8_t *data)
{
    uint8_t rdata[3];
    bool ret = false;

    memset(rdata, 0, 3);

    uint8_t dev_slot = data[3];

    rdata[0] = MS_CMD_ID_SET_BT_CONN_STATE;
    rdata[2] = 0x00;

    if (!app_vendor_cfg_is_audio_hid() && (dev_slot != DEV_SLOT_HS))
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return;
    }

    if ((dev_slot < DEV_SLOT_HS) || (dev_slot > DEV_SLOT_HOGP_2) || data[2] != 2 || data[4] > 1)
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return;
    }

    switch (dev_slot)
    {
    case DEV_SLOT_HS:
        {
            if (data[4] == BT_CONN_STATE_DISCONNECTED)
            {
                //ret = src_disconnect();
                ret = true;
            }
            else
            {
#ifdef LEGACY_BT_GENERAL
                ret = src_connect();
            }
        }
        break;

    case DEV_SLOT_HOGP_1:
    case DEV_SLOT_HOGP_2:
        {
            if (data[4] == BT_CONN_STATE_DISCONNECTED)
            {
//                ret = src_ble_disconnect(dev_slot - 2);
            }
            else
            {
//                ret = src_ble_connect(dev_slot - 2);
            }
        }
        break;

        // default:
        //     break;
    }

    if (ret)
    {
        rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
    }
    else
    {
        rdata[1] = USB_HID_CTRL_CMD_NOT_ACCEPT;
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
}

void app_usb_hid_ctrl_cmd_query_bt_conn_state(uint8_t *data)
{
    uint8_t dev_slot = data[3];
    uint8_t conn_state = 0;
    uint8_t profile_mask = 0;
    uint8_t rdata[6] = {MS_CMD_ID_QUERY_BT_CONN_STATE, 0x00, 0x03, dev_slot, 0x00, 0x00};

    if (!app_vendor_cfg_is_audio_hid() && (dev_slot != DEV_SLOT_HS))
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return;
    }

    switch (dev_slot)
    {
    case DEV_SLOT_HS:
        {
            conn_state = src_get_audio_device_conn_state();
            profile_mask = src_get_audio_device_profile_state();
        }
        break;

    case DEV_SLOT_HOGP_1:
    case DEV_SLOT_HOGP_2:
        {
//            src_get_hogp_device_conn_state(dev_slot - 2, &conn_state);
        }
        break;

    default:
        {
            rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
        }
        break;
    }

    if (!rdata[1])
    {
        rdata[4] = conn_state;
        rdata[5] = profile_mask;
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
}

void app_usb_hid_ctrl_cmd_set_led_brightness(uint8_t *data)
{
    uint8_t rdata[3];
    memset(rdata, 0, 3);

    rdata[0] = MS_CMD_ID_SET_LED_BRIGHTNESS;
    rdata[2] = 0x00;

    if (data[2] != 0x0A || data[3] > 1)
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return;
    }

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
}

void app_usb_hid_ctrl_cmd_get_led_brightness(uint8_t *data)
{
    uint8_t rdata[12];

    memset(rdata, 0, 12);
    rdata[0] = MS_CMD_ID_GET_LED_BRIGHTNESS;
    rdata[2] = 0x09;

    if (data[2] != 0)
    {
        rdata[1] = USB_HID_CTRL_CMD_PARAM_ERR;
        app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
        return;
    }
    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
}

void app_usb_hid_ctrl_cmd_get_dongle_capability(void)
{
    uint8_t rdata[35];

    memset(rdata, 0, 35);

    rdata[0] = MS_CMD_ID_GET_DONGLE_CAPABILITY;
    rdata[1] = USB_HID_CTRL_CMD_ACCEPT;
    rdata[2] = 0x20;

    app_vendor_cfg_get_capability(&rdata[3]);

    app_usb_hid_send_report(HID_IF_TEST, 0, rdata, sizeof(rdata), 0);
}

/*command event start*/

static void legacy_cmd_complete_event(uint16_t opcode, uint8_t status)
{
    uint8_t report[5];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, LEGACY_COMMAND_COMPLETE_EVENT);
    UINT16_TO_STREAM(pp, opcode);
    UINT8_TO_STREAM(pp, status);

    app_usb_hid_send_bt_ctrl_data(5, report);
}

void app_scan_state_change_event(T_APP_SCAN_STATE state)
{
    uint8_t report[3];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, LEGACY_SCAN_STATE_CHANGE_EVENT);
    UINT8_TO_STREAM(pp, (uint8_t)state);

    app_usb_hid_send_bt_ctrl_data(3, report);
}

void app_version_report_event(void)
{
    uint8_t report[4];
    uint8_t *pp = report;
    UINT16_TO_STREAM(pp, FIRMWARE_VERSION_REPORT_EVENT);
    UINT8_TO_STREAM(pp, (uint8_t)RTL8763EAU_VER_MAJOR);
    UINT8_TO_STREAM(pp, (uint8_t)RTL8763EAU_VER_MINOR);

    app_usb_hid_send_bt_ctrl_data(4, report);
}

void app_scan_result_report_event(T_BT_EVENT_PARAM_INQUIRY_RESULT inquiry_result)
{
    uint8_t report[60];
    uint8_t *pp = report;
    uint8_t significant_len = 0;
    uint8_t eir_offset = 0;

    significant_len = get_eir_data_significant_len(inquiry_result.p_eir, inquiry_result.eir_len);
    UINT16_TO_STREAM(pp, LEGACY_SCAN_RESULT_REPORT_EVENT);
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
            UINT16_TO_STREAM(pp, LEGACY_SCAN_RESULT_REPORT_EVENT);
            UINT8_TO_STREAM(pp, (significant_len - eir_len_left));
            memcpy(pp, (inquiry_result.p_eir + (significant_len - eir_len_left)), 57);
            app_usb_hid_send_bt_ctrl_data(60, report);

            eir_len_left -= 57;
        }

        if (eir_len_left > 0)
        {
            memset(report, 0, 60);
            pp = report;
            UINT16_TO_STREAM(pp, LEGACY_SCAN_RESULT_REPORT_EVENT);
            UINT8_TO_STREAM(pp, (significant_len - eir_len_left));
            memcpy(pp, (inquiry_result.p_eir + (significant_len - eir_len_left)), eir_len_left);
            app_usb_hid_send_bt_ctrl_data(60, report);
        }
    }
}

void app_pair_state_report_event(void)
{
    uint8_t report[3];
    uint8_t *pp = report;
    uint8_t cur_pair_state;
    UINT16_TO_STREAM(pp, PAIR_STATE_REPORT_EVENT);

    cur_pair_state = pair_state[0];
    UINT8_TO_STREAM(pp, cur_pair_state);
    app_usb_hid_send_bt_ctrl_data(3, report);
}

void app_paired_device_info_report_event(void)
{
    uint8_t name_len = 0;
    T_APP_SRC_TARGET_DEV dev_info;
    if (app_src_storage_get_target_dev(DEV_SLOT_HS, &dev_info))
    {
        name_len = strlen((const char *)dev_info.remote_name);
        uint8_t *report = calloc(1, name_len + 14);
        if (report == NULL)
        {
            return;
        }
        uint8_t *pp = report;

        UINT16_TO_STREAM(pp, PAIRED_DEVICE_INFO_REPORT_EVENT);
        UINT8_TO_STREAM(pp, dev_info.addr_type);
        memcpy(pp, dev_info.remote_bd, 6);
        pp += BD_ADDR_LEN;
        UINT16_TO_STREAM(pp, dev_info.vid);
        UINT16_TO_STREAM(pp, dev_info.pid);
        UINT8_TO_STREAM(pp, name_len);
        memcpy(pp, dev_info.remote_name, name_len);

        app_usb_hid_send_bt_ctrl_data(name_len + 14, report);

        free(report);
    }
}

void app_bt_connection_state_report_event(uint8_t conn_state, uint8_t profile_mask)
{
    uint8_t report[4];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, BT_CONNECTION_STATE_REPORT_EVENT);
    UINT8_TO_STREAM(pp, conn_state);
    UINT8_TO_STREAM(pp, profile_mask);

    app_usb_hid_send_bt_ctrl_data(4, report);
}

extern T_APP_CFG_NV app_cfg_nv;
void app_dongle_name_report_event(void)
{
    uint8_t name_len;
    name_len = strlen((char *)app_cfg_nv.device_name_legacy);

    uint8_t *report = calloc(1, name_len + 2);
    if (report == NULL)
    {
        return;
    }
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, DONGLE_NAME_REPORT_EVENT);
    memcpy(pp, app_cfg_nv.device_name_legacy, name_len);
    app_usb_hid_send_bt_ctrl_data(name_len + 2, report);

    free(report);
}

extern T_APP_DB app_db;
void app_public_bt_addr_report_event(void)
{
    uint8_t report[8];
    uint8_t *pp = report;

    UINT16_TO_STREAM(pp, PUBLIC_BT_ADDRESS_REPORT_EVENT);
    memcpy(pp, app_db.factory_addr, 6);

    app_usb_hid_send_bt_ctrl_data(8, report);
}

void app_link_key_report_event(void)
{
    uint8_t report[18];
    uint8_t *pp = report;
    uint8_t link_key[16];
    T_APP_SRC_TARGET_DEV temp_dev;
    bool status;
    T_BT_LINK_KEY_TYPE type;

    UINT16_TO_STREAM(pp, LINK_KEY_REPORT_EVENT);

    if (app_src_storage_get_target_dev(TARGET_DEVICE_TYPE_HEADSET, &temp_dev))
    {
        status = app_src_legacy_get_bond_by_addr(temp_dev.remote_bd, link_key, &type);
        APP_PRINT_INFO1("app_link_key_report_event, status: %d", status);
        if (status)
        {
            memcpy(pp, link_key, 16);
        }
        else
        {
            memset(pp, 0x00, 16);
        }
    }

    app_usb_hid_send_bt_ctrl_data(18, report);
}

void app_realtime_pair_state_report_event(uint8_t *bd_addr, T_APP_CTRL_PAIR_STATE new_pair_state)
{
    uint8_t report[3];
    uint8_t *pp = report;

    if (bd_addr == NULL)
    {
        APP_PRINT_ERROR0("app_realtime_pair_state_report_event: error input params");
        return;
    }

    UINT16_TO_STREAM(pp, REALTIME_PAIR_STATE_REPORT_EVENT);
    UINT8_TO_STREAM(pp, (uint8_t)new_pair_state);

    if ((new_pair_state == PAIR_STATE_SUCCESS) ||
        (new_pair_state == PAIR_STATE_FAIL))
    {
        pair_state[0] = new_pair_state;
    }

    app_usb_hid_send_bt_ctrl_data(3, report);
}

uint8_t app_usb_hid_handle_general_cmd(uint8_t *p_data, uint16_t len)
{
    uint8_t status = LEGACY_COMMAND_COMPLETE_SUCCESS;
    uint8_t *pp = p_data;
    uint16_t length = len;
    uint16_t opcode;
    STREAM_TO_UINT16(opcode, pp);
    length -= 2;

    APP_PRINT_INFO1("app_usb_hid_handle_general_cmd, opcode: 0x%04x", opcode);
    switch (opcode)
    {
    case GET_VERSION_OPCODE:
        {
            legacy_cmd_complete_event(opcode, status);
            app_version_report_event();
        }
        break;
    case LEGACY_START_SCAN_OPCODE:
        {
            T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;
            // poweron reconn can should be cancelled before inquiry
            src_stop_poweron_reconn();

            ret = gap_br_start_inquiry(false, 10);
            if (ret != GAP_CAUSE_SUCCESS)
            {
                status = LEGACY_COMMAND_OPERATION_FAIL;
                legacy_cmd_complete_event(opcode, status);
            }
            else
            {
                status = LEGACY_COMMAND_COMPLETE_SUCCESS;
                legacy_cmd_complete_event(opcode, status);
                app_scan_state_change_event(LEGACY_SCAN_ON);
            }
        }
        break;
    case LEGACY_STOP_SCAN_OPCODE:
        {
            if (gap_br_stop_inquiry())
            {
                status = LEGACY_COMMAND_OPERATION_FAIL;
                legacy_cmd_complete_event(opcode, status);
            }
            else
            {
                status = LEGACY_COMMAND_COMPLETE_SUCCESS;
                legacy_cmd_complete_event(opcode, status);
                app_scan_state_change_event(LEGACY_SCAN_OFF);
            }
        }
        break;
    case LEGACY_PAIR_TO_DEVICE_OPCODE:
        {
            uint8_t bd_addr[6];
            if (length != 7)
            {
                status = LEGACY_INVALID_COMMAND_LENGTH;
                legacy_cmd_complete_event(opcode, status);
                break;
            }
            pp++; // Addr Type
            // memcpy(bd_addr, pp + 1, BD_ADDR_LEN);
            for (uint8_t i = 0; i < 6; i++)
            {
                bd_addr[5 - i] = *pp++;
            }
            APP_PRINT_TRACE1("paired_to_device, bd_addr: %s", TRACE_BDADDR(bd_addr));
            if (src_connect_audio_device(bd_addr, 0))
            {
                pair_state[0] = PAIR_STATE_IN_PROCESS;
            }
            else
            {
                status = LEGACY_COMMAND_OPERATION_FAIL;
            }
            legacy_cmd_complete_event(opcode, status);
        }
        break;
    case LEGACY_GET_PAIR_STATE_OPCODE:
        {
            legacy_cmd_complete_event(opcode, status);
            app_pair_state_report_event();
        }
        break;
    case GET_PAIRED_DEVICE_INFO_OPCODE:
        {
            legacy_cmd_complete_event(opcode, status);
            app_paired_device_info_report_event();
        }
        break;
    case LEGACY_REMOVE_DEVICE_OPCODE:
        {
            legacy_cmd_complete_event(opcode, status);
            pair_state[0] = PAIR_STATE_IDLE;
            src_remove_audio_device();
            app_src_storage_clear_target_dev(DEV_SLOT_HS);
        }
        break;
    case LEGACY_GET_BT_CONN_STATE_OPCODE:
        {
            legacy_cmd_complete_event(opcode, status);

            uint8_t conn_state = 0;
            uint8_t profile_mask = 0;

            conn_state = src_get_audio_device_conn_state();
            profile_mask = src_get_audio_device_profile_state();
            app_bt_connection_state_report_event(conn_state, profile_mask);
        }
        break;
    case LEGACY_SET_BT_CONN_STATE_OPCODE:
        {
            uint8_t conn_state;
            uint8_t ret;
            STREAM_TO_UINT8(conn_state, pp);
            if (length != 1)
            {
                status = LEGACY_INVALID_COMMAND_LENGTH;
            }
            else if (conn_state > 1)
            {
                status = LEGACY_INVALID_COMMAND_PARAM;
            }
            else if (conn_state == 0x00)
            {
                ret = src_disconnect();
                if (!ret)
                {
                    status = LEGACY_COMMAND_OPERATION_FAIL;
                }
            }
            else if (conn_state == 0x01)
            {
                ret = src_connect();
                if (!ret)
                {
                    status = LEGACY_COMMAND_OPERATION_FAIL;
                }
            }
            legacy_cmd_complete_event(opcode, status);
        }
        break;
    case LEGACY_GET_DONGLE_NAME_OPCODE:
        {
            legacy_cmd_complete_event(opcode, status);
            app_dongle_name_report_event();
        }
        break;
    case GET_PUB_BT_ADDR_OPCODE:
        {
            legacy_cmd_complete_event(opcode, status);
            app_public_bt_addr_report_event();
        }
        break;
    case LEGACY_GET_LINK_KEY_OPCODE:
        {
            legacy_cmd_complete_event(opcode, status);
            app_link_key_report_event();
        }
        break;
    case LEGACY_ENTER_DUT_MODE:
        {
            legacy_cmd_complete_event(opcode, status);
            switch_into_single_tone_test_mode();
        }
        break;
    case LEGACY_FACTORY_RESET:
        {
            legacy_cmd_complete_event(opcode, status);
            app_mmi_handle_action(MMI_DEV_FACTORY_RESET);
        }
        break;
    }
    return status;
}

void app_ctrl_cfg_init(void)
{
    bt_mgr_cback_register(ctrl_cmd_bt_mgr_cback);
    src_register_pair_state_nofity(app_realtime_pair_state_report_event);
    //src_register_audio_pair_state_nofity(app_usb_hid_audio_device_pair_state_cb);
    //src_register_hogp_pair_state_nofity(app_usb_hid_hogp_device_pair_state_cb);
}
#endif
#endif
