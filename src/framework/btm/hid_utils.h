/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _HID_UTILS_H_
#define _HID_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#include "os_queue.h"
#include "hid_parser.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define HID_BOOT_MODE_KEYBOARD_ID 1
#define HID_BOOT_MODE_MOUSE_ID 2

#define HID_CONTROL_MTU_SIZE     672
#define HID_INTERRUPT_MTU_SIZE   672

#define HID_HDR_LENGTH       1

/* HID Message Type Definitions */
#define HID_MSG_TYPE_HANDSHAKE           0x00
#define HID_MSG_TYPE_HID_CONTROL         0x01
#define HID_MSG_TYPE_GET_REPORT          0x04
#define HID_MSG_TYPE_SET_REPORT          0x05
#define HID_MSG_TYPE_GET_PROTOCOL        0x06
#define HID_MSG_TYPE_SET_PROTOCOL        0x07
#define HID_MSG_TYPE_GET_IDLE            0x08
#define HID_MSG_TYPE_SET_IDLE            0x09
#define HID_MSG_TYPE_DATA                0x0A

/* HID Handshake Message Parameter Definition */
#define HID_MSG_PARAM_HANDSHAKE_RESULT_SUCCESSFUL               0x00
#define HID_MSG_PARAM_HANDSHAKE_RESULT_NOT_READY                0x01
#define HID_MSG_PARAM_HANDSHAKE_RESULT_ERR_INVALID_REPORT_ID    0x02
#define HID_MSG_PARAM_HANDSHAKE_ERR_UNSUPPORTED_REQUEST         0x03
#define HID_MSG_PARAM_HANDSHAKE_ERR_INVALID_PARAMETER           0x04
#define HID_MSG_PARAM_HANDSHAKE_ERR_UNKNOWN                     0x05
#define HID_MSG_PARAM_HANDSHAKE_ERR_FATAL                       0x06

/* HID Control Message Parameter Definition */
#define HID_MSG_PARAM_CONTROL_NOP                   0x00
#define HID_MSG_PARAM_CONTROL_HARD_RESET            0x01
#define HID_MSG_PARAM_CONTROL_SOFT_RESET            0x02
#define HID_MSG_PARAM_CONTROL_SUSPEND               0x03
#define HID_MSG_PARAM_CONTROL_EXIT_SUSPEND          0x04
#define HID_MSG_PARAM_CONTROL_VIRTUAL_CABLE_UNPLUG  0x05

/* HID Set/Get Protocol Message Parameter Definition */
#define HID_MSG_PARAM_BOOT_PROTOCOL_MODE    0x00
#define HID_MSG_PARAM_REPORT_PROTOCOL_MODE  0x01

/* HID Data Message Parameter Definition */
#define HID_MSG_PARAM_REPORT_TYPE_OTHER     0x00
#define HID_MSG_PARAM_REPORT_TYPE_INPUT     0x01
#define HID_MSG_PARAM_REPORT_TYPE_OUTPUT    0x02
#define HID_MSG_PARAM_REPORT_TYPE_FEATURE   0x03

typedef enum t_hid_state
{
    HID_STATE_DISCONNECTED  = 0x00,
    HID_STATE_ALLOCATED     = 0x01,
    HID_STATE_CONNECTING    = 0x02,
    HID_STATE_CONNECTED     = 0x03,
    HID_STATE_DISCONNECTING = 0x04,
} T_HID_STATE;

typedef struct t_hid_control_chann
{
    T_HID_STATE       state;
    uint8_t           report_status;
    uint16_t          cid;
    uint16_t          remote_mtu;
    uint16_t          ds_data_offset;
} T_HID_CONTROL_CHANN;

typedef struct t_hid_interrupt_chann
{
    T_HID_STATE       state;
    uint16_t          cid;
    uint16_t          remote_mtu;
    uint16_t          ds_data_offset;
} T_HID_INTERRUPT_CHANN;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HID_UTILS_H_ */
