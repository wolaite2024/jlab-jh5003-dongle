/**
 * @copyright Copyright (C) 2023 Realtek Semiconductor Corporation.
 *
 * @file usb_task.h
 * @version 1.0
 * @brief
 *
 * @note:
 */
#ifndef __USB_TASK_H__
#define __USB_TASK_H__
#include <stdint.h>

/** @defgroup USB_Task USB Task
  * @{
  */

/**
 * @brief msg group that send from isr defined in \ref T_HAL_USB_COMMON_ISR
 *
 */
#define USB_TASK_MSG_GROUP_HAL      0
#define USB_TASK_MSG_GROUP_DM       1
#define USB_TASK_MSG_GROUP_HAL_EP4      2
#define USB_TASK_MSG_GROUP_MISC     3

/**
 * @brief USB task message
 *
 */
typedef struct _usb_task_msg
{
    uint8_t group;
    uint8_t type;

    union
    {
        struct
        {
            uint8_t *buf;
            uint16_t len;
        } complex;
        uint32_t var[2];
    };

} T_USB_TASK_MSG;

/**
 * @brief USB pending request type
 *
 */
typedef int (*T_USB_TASK_PENDING_CALL)(void *);

/**
 * @brief send message to USB task
 *
 * @param msg: msg to send
 * @return int result, refer to "errno.h"
 */
int usb_task_msg_send(T_USB_TASK_MSG *msg);

/**
 * @brief create USB task
 *
 * @return int
 */
int usb_task_create(void);

/**
 * @brief release USB task
 *
 * @return int
 */
int usb_task_release(void);

/**
 * @brief pending request to USB task
 *
 * @return int
 */
int usb_task_pending_call(T_USB_TASK_PENDING_CALL func, void *param);

/** @}*/
/** End of USB_Task
*/
#endif // !__USB_TASK_H__
