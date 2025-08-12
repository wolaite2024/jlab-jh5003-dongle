/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __UAL_BLUETOOTH_H__
#define __UAL_BLUETOOTH_H__

#include <stdint.h>
#include "ual_types.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TRANSPORT_TYPE_UNKNOW   0
#define TRANSPORT_TYPE_BREDR    (1 << 0)
#define TRANSPORT_TYPE_LE       (1 << 1)
#define TRANSPORT_TYPE_DUAL     (TRANSPORT_TYPE_BREDR | TRANSPORT_TYPE_LE)
typedef uint8_t T_BT_TRANSPORT_TYPE;

#define BD_ADDR_LEN               6

typedef enum
{
    BT_BOND_STATE_NONE,
    BT_BOND_STATE_BONDING,
    BT_BOND_STATE_BONDED
} T_BT_BOND_STATE;

typedef enum
{
    BT_CONN_STATE_DISCONNECTED, //!< Disconnected.
    BT_CONN_STATE_CONNECTING,   //!< Connecting.
    BT_CONN_STATE_CONNECTED,    //!< Connected.
    BT_CONN_STATE_DISCONNECTING //!< Disconnecting.
} T_BT_CONN_STATE;


/** Bluetooth Error Status */
/** We need to build on this */

typedef enum
{
    BT_STATUS_SUCCESS,
    BT_STATUS_FAIL,
    BT_STATUS_NOT_READY,
    BT_STATUS_NOMEM,
    BT_STATUS_BUSY,
    BT_STATUS_DONE, /* request already completed */
    BT_STATUS_UNSUPPORTED,
    BT_STATUS_PARM_INVALID,
    BT_STATUS_UNHANDLED,
    BT_STATUS_RMT_DEV_DOWN,
    BT_STATUS_AUTH_FAILURE,
    BT_STATUS_AUTH_REJECTED,
    BT_STATUS_AUTH_KEY_MISSING,
    BT_STATUS_AUTH_COMPLETE,    /**< auth key complete for bluetooth */
} T_BT_STATUS;

/** Bluetooth Adapter State */
typedef enum { BT_STATE_OFF, BT_STATE_ON } T_BT_STATE;

typedef enum
{
    INQ_OFF,
    INQ_ON,
    LE_SCAN_OFF,
    LE_SCAN_ON
} T_BT_DISC_STATE;


typedef struct
{
    T_BT_BOND_STATE  state;
    uint8_t          bd_addr[6];
    uint8_t          bd_type;
} T_BT_BOND_INFO;

typedef struct
{
    uint32_t         passkey;
    uint8_t          bd_addr[6];
} T_BT_DISPLAY_INFO;

typedef struct
{
    uint8_t          bd_addr[6];
} T_PASSKEY_REQ;

typedef struct
{
    uint8_t                 conn_id;
    T_BT_CONN_STATE         state;
    uint8_t                 bd_addr[6];
    uint8_t                 bd_type;
    T_BT_TRANSPORT_TYPE     type;
    uint16_t                disc_cause;
} T_BT_CONN_INFO;

typedef struct
{
    uint8_t          transport_type;
    T_BT_STATUS      status;
    uint8_t          bd_addr[6];
    uint8_t          bd_type;
} T_BT_AUTH_INFO;

#define UAL_ADP_GROUP                   0x0100
#define UAL_ADP_STATE_CHANGE            0x0101
#define UAL_ADP_LE_SCAN_STATE_CHANGE    0x0102
#define UAL_ADP_BOND_PASSKEY_DISPLAY    0x0103
#define UAL_ADP_BOND_USER_CONFIRMATION  0x0104
#define UAL_ADP_BOND_PASSKEY_INPUT      0x0105
#define UAL_ADP_DEV_AUTH_CMPLT          0x0106

#define UAL_DEV_GROUP                   0x0200
#define UAL_DEV_BOND_STATE_CHANGE       0x0201
#define UAL_DEV_CONN_STATE_CHANGE       0x0202

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UAL_ADAPTER_H__ */
