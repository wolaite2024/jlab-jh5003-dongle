#ifndef _APP_ADAPTER_SERVICE_H_
#define _APP_ADAPTER_SERVICE_H_

#include "ual_types.h"
#include "ual_dev_mgr.h"
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "app_gaming_sync.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
typedef enum
{
    /* headset disconnected due to power off */
    ADAPTER_EVENT_HEADSET_PHONE_UNKNOWN         = 0x00,
    ADAPTER_EVENT_HEADSET_PHONE_CONNECTED       = 0x01,
    ADAPTER_EVENT_HEADSET_PHONE_DISCONNECTED    = 0x02,
    ADAPTER_EVENT_BLE_CONNECTED                 = 0x03,
    ADAPTER_EVENT_BLE_DISCONNECTED              = 0x04,
    ADAPTER_EVENT_A2DP_CONNECTED                = 0x05,
    ADAPTER_EVENT_A2DP_DISCONNECTED             = 0x06,
} T_ADAPTER_LINK_EVENT;
#endif

typedef struct t_dev_res_info
{
    struct t_dev_res_info   *p_next;
    uint8_t                 adv_sid;
    uint8_t                 bd_addr[BD_ADDR_LEN];  /**< remote BD */
    uint8_t                 bd_type;              /**< remote BD type*/
} T_DEV_RES_INFO;

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
bool adapter_ble_is_connected(void);
bool adapter_acl_is_connected(void);
bool adapter_a2dp_is_connected(void);
void adapter_link_handle_headset_conn_status(T_HEADSET_CONN_STATUS headset_conn_status);
void adapter_dual_mode_link_mgr(T_ADAPTER_LINK_EVENT event, uint8_t ble_conn_id);
#endif
T_DEV_RES_INFO *disc_mgr_find_device_by_idx(uint8_t idx);
void adapter_msg_cback(uint16_t msg, uint8_t *msg_data, uint32_t msg_dlen);
uint32_t adapter_start_le_scan(uint16_t uuid, bool has_white_list);
void adapter_stop_le_scan(void);

#if F_APP_LEA_DONGLE_BINDING
void adapter_set_scan_by_mmi(bool flag);
bool adapter_get_scan_by_mmi(void);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */
#endif
