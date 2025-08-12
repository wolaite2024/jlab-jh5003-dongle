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

typedef struct t_hid_link_data
{
    uint8_t proto_mode;
    uint8_t initiator;
    bool    control_chann_connected;
    bool    interrupt_chann_connected;
} T_HID_LINK_DATA;

typedef struct t_bt_hid_device_set_idle_ind
{
    uint8_t    report_status;
} T_BT_HID_DEVICE_SET_IDLE_IND;

typedef struct t_bt_hid_device_get_report_ind
{
    uint8_t    report_type;
    uint16_t   report_id;
    uint16_t   report_size;
    uint8_t   *p_data;
} T_BT_HID_DEVICE_GET_REPORT_IND;

typedef struct t_bt_hid_device_set_report_ind
{
    uint8_t    report_type;
    uint16_t   report_id;
    uint16_t   report_size;
    uint8_t   *p_data;
} T_BT_HID_DEVICE_SET_REPORT_IND;

typedef struct t_bt_hid_device_data_ind
{
    uint8_t    report_type;
    uint16_t   report_id;
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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_HID_INT_H_ */
