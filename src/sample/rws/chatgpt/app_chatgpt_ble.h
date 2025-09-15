#ifndef _APP_CHATGPT_BLE_H_
#define _APP_CHATGPT_BLE_H_

#define BLE_UUID_CHATGPT_SERVICE               0xFF01
#define BLE_UUID_CHATGPT_TX_CHARACTERISTIC     0xFF02
#define BLE_UUID_CHATGPT_RX_CHARACTERISTIC     0xFF03
#define BLE_UUID_CHATGPT_DEVICE_NAME           0xFF04
#define BLE_UUID_CHATGPT_LED_CONTROL           0xFF05
#define BLE_UUID_CHATGPT_DEBUG_PATH            0xFF06

#define CHATGPT_RX_DATA_INDEX                  0x02
#define CHATGPT_TX_DATA_INDEX                  0x04
#define CHATGPT_DATA_CCCD_INDEX                0x05

typedef struct
{
    uint16_t len;
    uint8_t *p_value;
} T_CHATGPT_RX_DATA;
/** Message content: @ref TRANSMIT_SERVICE_Upstream_Message */
typedef union
{
    uint8_t value;
    T_CHATGPT_RX_DATA rx_data;
} T_CHATGPT_SRV_MSG_DATA;

typedef struct
{
    T_SERVICE_CALLBACK_TYPE  msg_type;

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    uint16_t                 conn_handle;
    uint16_t                 cid;
#endif

    uint8_t                  conn_id;
    uint8_t                  attr_index;
    T_CHATGPT_SRV_MSG_DATA     msg_data;
} T_CHATGPT_SRV_RX;

void app_chatgpt_load_uuid(uint8_t *data);
bool app_chatgpt_transmit_srv_tx_data(uint8_t conn_id, uint16_t len, uint8_t *p_value);
uint8_t app_chatgpt_ble_get_idx(void);
bool app_chatgpt_ble_tx_ready(void);
uint8_t app_chatgpt_ble_init(void);
#endif

