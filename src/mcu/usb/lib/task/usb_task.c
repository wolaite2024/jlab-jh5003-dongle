#include <stddef.h>
#include "errno.h"
#include "os_msg.h"
#include "os_task.h"
#include "trace.h"
#include "usb_task.h"
#include "usb_isr.h"
#include "usb_dm_int.h"
#include "os_sync.h"
#include "section.h"

static void *usb_task_handle = NULL;
static void *usb_task_msg_queue = NULL;

typedef enum
{
    USB_MISC_MSG_PENDING_CALL
} T_USB_MISC_MSG;

RAM_TEXT_SECTION
int usb_task_msg_send(T_USB_TASK_MSG *msg)
{
    return os_msg_send(usb_task_msg_queue, msg, 0);
}

static void usb_task_msg_handle(T_USB_TASK_MSG *msg)
{
    uint8_t group = msg->group;

    if (group == USB_TASK_MSG_GROUP_HAL)
    {
        usb_isr_msg_handle(msg);
    }
    else if (group == USB_TASK_MSG_GROUP_DM)
    {
        usb_dm_msg_handle(msg);
    }
    else if (group == USB_TASK_MSG_GROUP_MISC)
    {
        T_USB_MISC_MSG type = (T_USB_MISC_MSG)msg->type;
        T_USB_TASK_PENDING_CALL func = (T_USB_TASK_PENDING_CALL)msg->var[0];
        void *param = (void *)msg->var[1];

        switch (type)
        {
        case USB_MISC_MSG_PENDING_CALL:
            {
                func(param);
            }
            break;
        }
    }
#if FOR_SS_DONGLE_HID_USING_EP4
    else if (group == USB_TASK_MSG_GROUP_HAL_EP4)
    {
        usb_isr_msg_handle_ep4(msg);
    }
#endif
    else
    {
        USB_PRINT_ERROR1("usb_task_msg_handle, invalid group:0x%x", group);
    }
}

static void usb_task(void *param)
{
    T_USB_TASK_MSG usb_msg;

    while (1)
    {
        if (os_msg_recv(usb_task_msg_queue, &usb_msg, 0xFFFFFFFFUL) == true)
        {
            usb_task_msg_handle(&usb_msg);
        }
    }
}

int usb_task_create(void)
{
    os_msg_queue_create(&usb_task_msg_queue, "usb task queue", 0x20, sizeof(T_USB_TASK_MSG));
    return (int)os_task_create(&usb_task_handle, "usb_task", usb_task, NULL,
                               1024 * 2, 1);
}

int usb_task_release(void)
{
    int ret = (int)os_task_delete(usb_task_handle);
    usb_task_handle = NULL;
    return ret;
}

int usb_task_pending_call(T_USB_TASK_PENDING_CALL func, void *param)
{
    int ret = ESUCCESS;

    T_USB_TASK_MSG msg = {.group = USB_TASK_MSG_GROUP_MISC, .type = USB_MISC_MSG_PENDING_CALL, };
    msg.var[0] = (uint32_t)func;
    msg.var[1] = (uint32_t)param;
    usb_task_msg_send(&msg);

    return ret;
}
