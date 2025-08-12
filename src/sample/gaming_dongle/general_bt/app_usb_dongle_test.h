#include "stdint.h"
#include "app_cfg.h"

#define MSFT_SN_ADDR        0x9FE000
#define MSFT_COLOR_ADDR     (MSFT_SN_ADDR+0x20)
#define MSFT_CONFIG_ADDR    (MSFT_COLOR_ADDR+0x20)
#define MSFT_HW_VERSION_ADDR    (MSFT_CONFIG_ADDR+0x20)

#define TEST_CMD_ID_LOOP_BACK           0xc0
#define TEST_CMD_ID_SET_LED             0xc1
#define TEST_CMD_ID_HID_INPUT           0xc2
#define TEST_CMD_ID_DEV_INFO            0xc3
#define TEST_CMD_ID_ACL_ESCO_STATIS     0xc4
#define TEST_CMD_ID_RESET               0xc5
#define TEST_CMD_ID_TEST_MOD            0xc6
#define TEST_CMD_ID_CLEAR_DEV_INFO      0xc7
#define TEST_CMD_ID_CHANGE_SYS_CFG      0xc8
#ifdef BTDONGLE_BQB_MODE_ENABLE
#define TEST_CMD_ID_BQB                 0xc9
#endif
//////////////////////////////////////////////////////
//Microsoft CMD
#define TEST_CMD_ID_MS_SET_LED              0x87
#define TEST_CMD_ID_MS_GET_FW_VER           0x2F
#define TEST_CMD_ID_GET_BT_ADDR             0x1B
#define TEST_CMD_ID_MS_GET_COLOR_CODE       0x1C
#define TEST_CMD_ID_MS_GET_SERIAL_NUM       0x18
#define TEST_CMD_ID_MS_GET_HW_VERSION       0x19

#define MS_CMD_ID_GET_VERSION               0x01
#define MS_CMD_ID_LEGACY_START_SCAN         0x02
#define MS_CMD_ID_LEGACY_SCAN_LIST_UPDATE   0x03
#define MS_CMD_ID_LEGACY_STOP_SCAN          0x04
#define MS_CMD_ID_LE_START_SCAN             0x05
#define MS_CMD_ID_LE_SCAN_LIST_UPDATE       0x06
#define MS_CMD_ID_LE_SCAN_STOP              0x07
#define MS_CMD_ID_PAIR_TO_DEVICE            0x08
#define MS_CMD_ID_GET_PAIR_STATE            0x09
#define MS_CMD_ID_GET_DONGLE_NAME           0x0A
#define MS_CMD_ID_SET_DONGLE_NAME           0x0B
#define MS_CMD_ID_GET_PAIRED_DEVICE_LIST    0x0C
#define MS_CMD_ID_REMOVE_DEVICE             0x0D
#define MS_CMD_ID_GET_BT_CONN_STATE         0x0E
#define MS_CMD_ID_SET_BT_CONN_STATE         0x0F
#define MS_CMD_ID_GET_LED_BRIGHTNESS        0x10
#define MS_CMD_ID_SET_LED_BRIGHTNESS        0x11

#define MS_CMD_ID_QUERY_BT_CONN_STATE       0x1A

#define MS_CMD_ID_GET_DONGLE_CAPABILITY     0x1D

#define MS_CMD_ID_GET_LINK_KEY              0xF0
#define MS_CMD_ID_RF_TEST_MODE              0xF1
#define MS_CMD_ID_GET_DISCONN_REASON        0xF2
inline void app_usb_hid_send_test_data(uint8_t length, uint8_t *data);
void app_usb_hid_handle_dev_info(uint8_t *data);
