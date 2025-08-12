#ifndef _APP_CTRL_CFG_H_
#define _APP_CTRL_CFG_H_

#include <stdint.h>
#include "app_msg.h"
#include "gap.h"
#include "gap_msg.h"
#include "gap_le.h"
#include "btm.h"

#define USB_NAME_MAX_LEN                      40
#define HARDWARE_ID_MAX_LENGTH                20

#define APPEARANCE_ID_KEYBOARD               961
#define APPEARANCE_ID_MOUSE                  962

#define LEGACY_COMMAND_COMPLETE_SUCCESS      0x00
#define LEGACY_UNKNOWN_COMMAND               0x01
#define LEGACY_INVALID_COMMAND_LENGTH        0x02
#define LEGACY_INVALID_COMMAND_PARAM         0x03
#define LEGACY_COMMAND_OPERATION_FAIL        0x04

#define GENERAL_RTK_START_OPCODE             0x2000
// General CMD
#define GET_VERSION_OPCODE                   0x2001
#define LEGACY_START_SCAN_OPCODE             0x2002
#define LEGACY_STOP_SCAN_OPCODE              0x2003
#define LEGACY_PAIR_TO_DEVICE_OPCODE         0x2004
#define LEGACY_GET_PAIR_STATE_OPCODE         0x2005
#define GET_PAIRED_DEVICE_INFO_OPCODE        0x2006
#define LEGACY_REMOVE_DEVICE_OPCODE          0x2007
#define LEGACY_GET_BT_CONN_STATE_OPCODE      0x2008
#define LEGACY_SET_BT_CONN_STATE_OPCODE      0x2009
#define LEGACY_GET_DONGLE_NAME_OPCODE        0x200A
#define GET_PUB_BT_ADDR_OPCODE               0x200B
#define LEGACY_GET_LINK_KEY_OPCODE           0x200C
#define LEGACY_ENTER_DUT_MODE                0x200D
#define LEGACY_FACTORY_RESET                 0x200E

// event opcode
#define LEGACY_SCAN_STATE_CHANGE_EVENT       0x2E00
#define FIRMWARE_VERSION_REPORT_EVENT        0x2E01
#define LEGACY_SCAN_RESULT_REPORT_EVENT      0x2E02
#define PAIR_STATE_REPORT_EVENT              0x2E03
#define PAIRED_DEVICE_INFO_REPORT_EVENT      0x2E04
#define BT_CONNECTION_STATE_REPORT_EVENT     0x2E05
#define DONGLE_NAME_REPORT_EVENT             0x2E06
#define PUBLIC_BT_ADDRESS_REPORT_EVENT       0x2E07
#define LINK_KEY_REPORT_EVENT                0x2E08
#define REALTIME_PAIR_STATE_REPORT_EVENT     0x2E09

// internal event

#define LEGACY_COMMAND_COMPLETE_EVENT 0x2EFF

typedef enum
{
    LEGACY_SCAN_OFF,
    LEGACY_SCAN_ON,
} T_APP_SCAN_STATE;

typedef enum
{
    DEV_MODE_LEGACY = 0x01,
    DEV_MODE_LE,
} T_APP_DEV_MODE;

typedef enum
{
    DEV_SLOT_HS = 0x01,
    DEV_SLOT_HOGP_1,
    DEV_SLOT_HOGP_2,
} T_APP_CTRL_DEV_SLOT;

#if 0
typedef enum
{
    BT_CONN_STATE_DICONNECTED = 0x00,
    BT_CONN_STATE_CONNECTED,
} T_APP_CTRL_BT_CONN_STATE;
#endif

typedef enum
{
    PAIR_STATE_IDLE = 0x00,
    PAIR_STATE_IN_PROCESS,
    PAIR_STATE_SUCCESS,
    PAIR_STATE_FAIL,
    PAIR_STATE_WAIT_PIN_CODE,
} T_APP_CTRL_PAIR_STATE;

typedef enum
{
    LE_SCAN_INFO_REPORT_STATE_IDLE = 0x00,
    LE_SCAN_INFO_REPORT_STATE_WAIT_SCAN_RSP,
} T_APP_LE_SCAN_INFO_REPORT_STATE;

typedef enum
{
    LE_SCAN_INFO_REPORT_NEW = 0,
    LE_SCAN_INFO_STORE_NEW,
    LE_SCAN_INFO_REPORT_OLD_STORE_NEW,
    LE_SCAN_INFO_REPORT_OLD_IGNORE_NEW,
    LE_SCAN_INFO_REPORT_APPEND_REPORT,
    LE_SCAN_INFO_REPORT_MAX,
} T_APP_LE_SCAN_INFO_HANDLE_PATH;

typedef struct
{
    uint8_t is_valid;
    uint8_t length;
    uint8_t name[USB_NAME_MAX_LEN];
    uint8_t padding[2];
} T_APP_DONGLE_NAME;

typedef struct
{
    uint8_t device_type;  //0:headset, 1: mouse, 2:keyboard
    uint8_t mode;         //0:BR/EDR, 1:BLE, 2:dual mode
    uint8_t addr_type;    //0:public, 1:random
    uint8_t bd_addr[6];
} T_APP_DEV_INFO;

typedef struct
{
    uint8_t commandId;
    uint8_t status;
    uint8_t payloadLength;
    uint8_t dev_slot;
    uint8_t bd_addr[6];
    uint8_t dev_type[4];
    uint8_t name[GAP_DEVICE_NAME_LEN];
} T_APP_DEV_LIST_INFO;

typedef struct
{
    uint8_t addr[6];
    T_GAP_REMOTE_ADDR_TYPE addr_type;
    int8_t rssi;
    uint8_t data_len;
    uint8_t data[62];
} T_APP_LE_SCAN_INFO;


//void app_usb_hid_handle_set_dongle_CTRL_cmd(uint8_t *data, uint8_t length);
void app_ble_handle_scan_start_stop(T_GAP_DEV_STATE old_state, T_GAP_DEV_STATE new_state,
                                    uint16_t cause);
//void app_ble_handle_scan_info(T_LE_CB_DATA *p_cb_data);
void app_src_handle_le_adv_report(T_LE_SCAN_INFO *scan_info);
void app_ctrl_cfg_init(void);
bool app_storage_get_dongle_usb_name(T_APP_DONGLE_NAME *target_name);

bool app_usb_hid_ctrl_cmd_get_version(void);

bool app_usb_hid_ctrl_cmd_legacy_scan_start(void);
bool app_usb_hid_ctrl_cmd_legacy_scan_stop(void);
bool app_usb_hid_ctrl_cmd_le_scan_start(void);
bool app_usb_hid_ctrl_cmd_le_scan_stop(void);

bool app_usb_hid_ctrl_cmd_get_dongle_name(void);
bool app_usb_hid_ctrl_cmd_change_dongle_name(uint8_t *data);
void app_usb_hid_ctrl_cmd_pair_to_device(uint8_t *data);
void app_usb_hid_ctrl_cmd_get_pair_state(uint8_t *data);
void app_usb_hid_ctrl_cmd_get_paired_device_list(uint8_t *data);
void app_usb_hid_ctrl_cmd_remove_device(uint8_t *data);
void app_usb_hid_ctrl_cmd_enable_rf_test_mode(uint8_t *data);
void app_usb_hid_ctrl_cmd_get_link_key(uint8_t *data);
void app_usb_hid_ctrl_cmd_get_disconn_reason(uint8_t *data);

void app_usb_hid_connection_state_notify(uint8_t dev_slot, uint8_t conn_state,
                                         uint8_t profile_mask);
void app_usb_hid_ctrl_cmd_get_bt_conn_state(uint8_t *data);
void app_usb_hid_ctrl_cmd_set_bt_conn_state(uint8_t *data);

void app_usb_hid_ctrl_cmd_query_bt_conn_state(uint8_t *data);

void app_usb_hid_ctrl_cmd_set_led_brightness(uint8_t *data);
void app_usb_hid_ctrl_cmd_get_led_brightness(uint8_t *data);
void app_usb_hid_ctrl_cmd_get_dongle_capability(void);

void app_scan_state_change_event(T_APP_SCAN_STATE state);
void app_scan_result_report_event(T_BT_EVENT_PARAM_INQUIRY_RESULT inquiry_result);
void app_bt_connection_state_report_event(uint8_t conn_state, uint8_t profile_mask);
uint8_t app_usb_hid_handle_general_cmd(uint8_t *p_data, uint16_t len);
#endif

