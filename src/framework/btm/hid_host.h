/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _HID_HOST_H_
#define _HID_HOST_H_

#include <stdint.h>
#include <stdbool.h>
#include "hid_utils.h"
#include "hid_parser.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_hid_host_msg
{
    HID_HOST_MSG_CONTROL_CONN_IND         = 0x00,
    HID_HOST_MSG_CONTROL_CONN_RSP         = 0x01,
    HID_HOST_MSG_CONTROL_CONN_CMPL        = 0x02,
    HID_HOST_MSG_CONTROL_CONN_FAIL        = 0x03,
    HID_HOST_MSG_CONTROL_DISCONNECTED     = 0x04,
    HID_HOST_MSG_HID_CONTROL_IND          = 0x05,
    HID_HOST_MSG_GET_REPORT_RSP           = 0x06,
    HID_HOST_MSG_SET_REPORT_RSP           = 0x07,
    HID_HOST_MSG_GET_PROTOCOL_RSP         = 0x08,
    HID_HOST_MSG_SET_PROTOCOL_RSP         = 0x09,
    HID_HOST_MSG_INTERRUPT_CONN_IND       = 0x10,
    HID_HOST_MSG_INTERRUPT_CONN_RSP       = 0x11,
    HID_HOST_MSG_INTERRUPT_CONN_CMPL      = 0x12,
    HID_HOST_MSG_INTERRUPT_CONN_FAIL      = 0x13,
    HID_HOST_MSG_INTERRUPT_DISCONNECTED   = 0x14,
    HID_HOST_MSG_INTERRUPT_DATA_IND       = 0x15,
} T_HID_HOST_MSG;

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

typedef struct t_hid_host_conn_cmpl_info
{
    uint8_t     bd_addr[6];
    uint16_t    cause;
} T_HID_HOST_CONN_CMPL_INFO;

typedef struct t_hid_host_disconn_rsp_info
{
    uint8_t     bd_addr[6];
    uint16_t    cause;
} T_HID_HOST_DISCONN_RSP_INFO;

typedef struct t_hid_host_control_ind
{
    uint8_t    control_operation;
} T_HID_HOST_CONTROL_IND;

typedef struct t_hid_host_get_report_rsp
{
    uint8_t    report_type;
    uint8_t    report_id;
    uint16_t   report_size;
    uint8_t   *p_data;
} T_HID_HOST_GET_REPORT_RSP;

typedef struct t_hid_host_set_report_rsp
{
    uint8_t    result_code;
} T_HID_HOST_SET_REPORT_RSP;

typedef struct t_hid_host_data_ind
{
    uint8_t    report_type;
    uint8_t    report_id;
    uint16_t   report_size;
    uint8_t   *p_data;
} T_HID_HOST_DATA_IND;

typedef struct t_hid_host_get_protocol_rsp
{
    uint8_t    proto_mode;
} T_HID_HOST_GET_PROTOCOL_RSP;

typedef struct t_hid_host_set_protocol_rsp
{
    uint8_t    result_code;
} T_HID_HOST_SET_PROTOCOL_RSP;

typedef void(*P_HID_HOST_CBACK)(uint8_t         bd_addr[6],
                                T_HID_HOST_MSG  msg_type,
                                void           *msg_buf);

bool hid_host_init(uint8_t          link_num,
                   bool             boot_proto_mode,
                   P_HID_HOST_CBACK cback);

bool hid_host_descriptor_set(const uint8_t *descriptor,
                             uint16_t       len);

bool hid_host_control_connect_req(uint8_t bd_addr[6]);

bool hid_host_control_disconnect_req(uint8_t bd_addr[6]);

bool hid_host_control_connect_cfm(uint8_t bd_addr[6],
                                  bool    accept);

bool hid_host_control_msg_send(uint8_t  bd_addr[6],
                               uint8_t  msg_type,
                               uint8_t  msg_param,
                               uint8_t *buf,
                               uint8_t  len);

bool hid_host_interrupt_connect_req(uint8_t bd_addr[6]);

bool hid_host_interrupt_disconnect_req(uint8_t bd_addr[6]);

bool hid_host_interrupt_connect_cfm(uint8_t bd_addr[6],
                                    bool    accept);

bool hid_host_interrupt_msg_send(uint8_t   bd_addr[6],
                                 uint8_t   msg_type,
                                 uint8_t   msg_param,
                                 uint8_t  *buf,
                                 uint16_t  len);

bool hid_host_get_roleswap_info(uint8_t                   bd_addr[6],
                                T_ROLESWAP_HID_HOST_INFO *p_info);

bool hid_host_set_roleswap_info(uint8_t                   bd_addr[6],
                                T_ROLESWAP_HID_HOST_INFO *p_info);

bool hid_host_del_roleswap_info(uint8_t bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HID_HOST_H_ */
