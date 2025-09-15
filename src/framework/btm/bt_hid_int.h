/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_HID_INT_H_
#define _BT_HID_INT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BT_HID_BOOT_MODE_KEYBOARD_ID 1
#define BT_HID_BOOT_MODE_MOUSE_ID    2

#define BT_HID_CONTROL_MTU_SIZE     672
#define BT_HID_INTERRUPT_MTU_SIZE   672

#define BT_HID_HDR_LENGTH       1

/* HID Message Type Definitions */
#define BT_HID_MSG_TYPE_HANDSHAKE           0x00
#define BT_HID_MSG_TYPE_HID_CONTROL         0x01
#define BT_HID_MSG_TYPE_GET_REPORT          0x04
#define BT_HID_MSG_TYPE_SET_REPORT          0x05
#define BT_HID_MSG_TYPE_GET_PROTOCOL        0x06
#define BT_HID_MSG_TYPE_SET_PROTOCOL        0x07
#define BT_HID_MSG_TYPE_GET_IDLE            0x08
#define BT_HID_MSG_TYPE_SET_IDLE            0x09
#define BT_HID_MSG_TYPE_DATA                0x0A

/* HID Handshake Message Parameter Definition */
#define BT_HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL               0x00
#define BT_HID_MSG_PARAM_HANDSHAKE_RESULT_NOT_READY                0x01
#define BT_HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID    0x02
#define BT_HID_MSG_PARAM_HANDSHAKE_ERR_UNSUPPORTED_REQUEST         0x03
#define BT_HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER           0x04
#define BT_HID_MSG_PARAM_HANDSHAKE_ERR_UNKNOWN                     0x05
#define BT_HID_MSG_PARAM_HANDSHAKE_ERR_FATAL                       0x06

/* HID Control Message Parameter Definition */
#define BT_HID_MSG_PARAM_CONTROL_NOP                   0x00
#define BT_HID_MSG_PARAM_CONTROL_HARD_RESET            0x01
#define BT_HID_MSG_PARAM_CONTROL_SOFT_RESET            0x02
#define BT_HID_MSG_PARAM_CONTROL_SUSPEND               0x03
#define BT_HID_MSG_PARAM_CONTROL_EXIT_SUSPEND          0x04
#define BT_HID_MSG_PARAM_CONTROL_VIRTUAL_CABLE_UNPLUG  0x05

/* HID Set/Get Protocol Message Parameter Definition */
#define BT_HID_MSG_PARAM_BOOT_PROTOCOL_MODE    0x00
#define BT_HID_MSG_PARAM_REPORT_PROTOCOL_MODE  0x01

/* HID Data Message Parameter Definition */
#define BT_HID_MSG_PARAM_REPORT_TYPE_OTHER     0x00
#define BT_HID_MSG_PARAM_REPORT_TYPE_INPUT     0x01
#define BT_HID_MSG_PARAM_REPORT_TYPE_OUTPUT    0x02
#define BT_HID_MSG_PARAM_REPORT_TYPE_FEATURE   0x03

typedef enum t_bt_hid_state
{
    BT_HID_STATE_DISCONNECTED  = 0x00,
    BT_HID_STATE_CONNECTING    = 0x01,
    BT_HID_STATE_CONNECTED     = 0x02,
    BT_HID_STATE_DISCONNECTING = 0x03,
} T_BT_HID_STATE;

typedef struct t_bt_hid_control_chann
{
    T_BT_HID_STATE       state;
    uint8_t              report_status;
    uint16_t             cid;
    uint16_t             remote_mtu;
    uint16_t             ds_data_offset;
} T_BT_HID_CONTROL_CHANN;

typedef struct t_bt_hid_interrupt_chann
{
    T_BT_HID_STATE       state;
    uint16_t             cid;
    uint16_t             remote_mtu;
    uint16_t             ds_data_offset;
} T_BT_HID_INTERRUPT_CHANN;

typedef struct t_roleswap_hid_device_info
{
    uint8_t     bd_addr[6];
    uint8_t     proto_mode;
    uint16_t    control_cid;
    uint8_t     control_state;
    uint16_t    control_remote_mtu;
    uint16_t    control_data_offset;
    uint16_t    interrupt_cid;
    uint8_t     interrupt_state;
    uint16_t    interrupt_remote_mtu;
    uint16_t    interrupt_data_offset;
} T_ROLESWAP_HID_DEVICE_INFO;

typedef struct t_roleswap_hid_device_transact
{
    uint8_t     bd_addr[6];
    uint8_t     proto_mode;
    uint8_t     control_state;
    uint8_t     interrupt_state;
} T_ROLESWAP_HID_DEVICE_TRANSACT;

typedef struct t_roleswap_hid_host_info
{
    uint8_t     bd_addr[6];
    uint8_t     proto_mode;
    uint16_t    control_cid;
    uint8_t     control_state;
    uint16_t    control_remote_mtu;
    uint16_t    control_data_offset;
    uint16_t    interrupt_cid;
    uint8_t     interrupt_state;
    uint16_t    interrupt_remote_mtu;
    uint16_t    interrupt_data_offset;
} T_ROLESWAP_HID_HOST_INFO;

typedef struct t_roleswap_hid_host_transact
{
    uint8_t     bd_addr[6];
    uint8_t     proto_mode;
    uint8_t     control_state;
    uint8_t     interrupt_state;
} T_ROLESWAP_HID_HOST_TRANSACT;

typedef struct t_bt_hid_device_set_idle_ind
{
    uint8_t    report_status;
} T_BT_HID_DEVICE_SET_IDLE_IND;

typedef struct t_bt_hid_device_get_report_ind
{
    uint8_t    report_type;
    uint8_t    report_id;
    uint16_t   report_size;
    uint8_t   *p_data;
} T_BT_HID_DEVICE_GET_REPORT_IND;

typedef struct t_bt_hid_device_set_report_ind
{
    uint8_t    report_type;
    uint8_t    report_id;
    uint16_t   report_size;
    uint8_t   *p_data;
} T_BT_HID_DEVICE_SET_REPORT_IND;

typedef struct t_bt_hid_device_data_ind
{
    uint8_t    report_type;
    uint8_t    report_id;
    uint16_t   report_size;
    uint8_t   *p_data;
} T_BT_HID_DEVICE_DATA_IND;

typedef struct t_bt_hid_device_get_protocol_ind
{
    uint8_t    proto_mode;
} T_BT_HID_DEVICE_GET_PROTOCOL_IND;

typedef struct t_bt_hid_device_set_protocol_ind
{
    uint8_t    proto_mode;
} T_BT_HID_DEVICE_SET_PROTOCOL_IND;

typedef struct t_bt_hid_device_control_ind
{
    uint8_t    control_operation;
} T_BT_HID_DEVICE_CONTROL_IND;

typedef struct t_bt_hid_host_disconn_cmpl
{
    uint8_t cause;
    bool    virtual_unplug;
} T_BT_HID_HOST_DISCONN_CMPL;

typedef struct t_bt_hid_host_hid_control_ind
{
    uint8_t    control_operation;
} T_BT_HID_HOST_CONTROL_IND;

typedef struct t_bt_hid_host_get_report_rsp
{
    uint8_t    report_type;
    uint8_t    report_id;
    uint16_t   report_size;
    uint8_t   *p_data;
} T_BT_HID_HOST_GET_REPORT_RSP;

typedef struct t_bt_hid_host_set_report_rsp
{
    uint8_t    result_code;
} T_BT_HID_HOST_SET_REPORT_RSP;

typedef struct t_bt_hid_host_data_ind
{
    uint8_t    report_type;
    uint8_t    report_id;
    uint16_t   report_size;
    uint8_t   *p_data;
} T_BT_HID_HOST_DATA_IND;

typedef struct t_bt_hid_host_get_protocol_rsp
{
    uint8_t    proto_mode;
} T_BT_HID_HOST_GET_PROTOCOL_RSP;

typedef struct t_bt_hid_host_set_protocol_rsp
{
    uint8_t    result_code;
} T_BT_HID_HOST_SET_PROTOCOL_RSP;

bool bt_hid_device_roleswap_info_get(uint8_t                     bd_addr[6],
                                     T_ROLESWAP_HID_DEVICE_INFO *info);

bool bt_hid_device_roleswap_info_set(uint8_t                     bd_addr[6],
                                     T_ROLESWAP_HID_DEVICE_INFO *info);

bool bt_hid_device_roleswap_info_del(uint8_t bd_addr[6]);

bool bt_hid_host_roleswap_info_get(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_HID_HOST_INFO *info);

bool bt_hid_host_roleswap_info_set(uint8_t                   bd_addr[6],
                                   T_ROLESWAP_HID_HOST_INFO *info);

bool bt_hid_host_roleswap_info_del(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_HID_INT_H_ */
