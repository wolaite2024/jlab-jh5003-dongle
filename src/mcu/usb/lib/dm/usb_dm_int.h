/**
 * @copyright Copyright (C) 2023 Realtek Semiconductor Corporation.
 *
 * @file usb_dm_int.h
 * @version 1.0
 * @brief
 *
 * @note:
 */
#ifndef __USB_DM_INT_H__
#define __USB_DM_INT_H__
#include "usb_dm.h"
#include "usb_task.h"

/**
 * usb_dm_int.h
 *
 * \brief   usb device descriptor event used in \ref USB_DM_CB
 *
 */
typedef enum
{
    USB_DM_EVT_INT_GET_DEV_QUALIFIER_DESC    = 3,
    USB_DM_EVT_INT_ENDPOINT_HALT  = 4,
} T_USB_DM_EVT_INT;

T_USB_POWER_STATE usb_dm_state_get(void);
int usb_dm_state_set(T_USB_POWER_STATE state);
int usb_dm_suspend_enter(void);
int usb_dm_suspend_exit(void);
int usb_dm_msg_handle(T_USB_TASK_MSG *msg);
void usb_dm_evt_dispatch(T_USB_DM_EVT evt, T_USB_DM_EVT_PARAM *param);
#endif
