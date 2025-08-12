#ifndef _APP_SRC_BLE_H_
#define _APP_SRC_BLE_H_

#include <stdint.h>
#include <string.h>

#include "app_msg.h"
#include "gap.h"
#include "app_ctrl_cfg.h"
#include "ual_bluetooth.h"

typedef enum
{
    LE_PENDING_EVT_NON = 0x00,
    LE_PENDING_EVT_CLEANUP,
    LE_PENDING_EVT_RECONN,
    LE_PENDING_EVT_NEW_TARGET,
} T_APP_SRC_LE_PENDING_EVT;

typedef struct
{
    T_APP_SRC_LE_PENDING_EVT pending_evt;
    uint8_t addr_type;
    uint8_t remote_bd[6];
    uint32_t appearance;
} T_APP_SRC_LE_PENDING_INFO;

typedef struct
{
    uint8_t     in_use;
    uint8_t     le_dev_type;
    uint8_t     bd_addr[6];
    uint8_t     addr_type;
    uint8_t     conn_id;
    uint8_t     in_wl;
    uint8_t     acl_exist;
    uint8_t     host_cmd_connect;
    uint8_t     hogp_connected;
    uint32_t    appearance;
    uint16_t    vid;
    uint16_t    pid;
    uint8_t     remote_name[GAP_DEVICE_NAME_LEN];

    T_APP_SRC_LE_PENDING_INFO pending_info;
} T_APP_SRC_LE_TARGET_DEV;


void app_src_ble_handle_gap_msg(T_IO_MSG *io_msg);

void le_conn_target_devices(void);

void app_src_ble_init(void);

typedef void (*APP_SRC_HOGP_CONN_RESULT_CB)(uint8_t index, uint8_t *bd_addr,
                                            uint8_t addr_type,
                                            T_APP_CTRL_PAIR_STATE pair_state,
                                            uint8_t *pin_code);
void src_register_hogp_pair_state_nofity(APP_SRC_HOGP_CONN_RESULT_CB pair_state_cb);

bool src_ble_connect_hogp_device(uint8_t index, uint8_t *bd_addr,
                                 uint8_t addr_type, uint32_t appearance);

bool src_ble_remove_hogp_device(uint8_t index);

bool src_get_hogp_paired_device(uint8_t index, uint8_t *bd_addr, uint8_t *dev_name);

bool src_get_hogp_device_conn_state(uint8_t index, T_BT_CONN_STATE *conn_state);


//only to reconn/disconn current devices
bool src_ble_connect(uint8_t index);
bool src_ble_disconnect(uint8_t index);

#endif
