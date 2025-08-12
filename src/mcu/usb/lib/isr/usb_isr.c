#include <string.h>
#include "rtl876x_nvic.h"
#include "vector_table.h"
#include "hal_usb.h"
#include "usb_dm_int.h"
#include "section.h"
#include "trace.h"
#include "usb_isr.h"

#ifdef CONFIG_SOC_SERIES_RTL8763E
#define USB_IP_IRQn         USB_IRQn
#define USB_IP_VECTORn      USB_VECTORn
#endif

static uint8_t setup_pkt[8] = {0,};
static T_USB_DEVICE_DRIVER *device_driver = NULL;
static uint8_t usb_isr_pending = 0;

USB_ISR_VOID_FUNC usb_isr_isoc_init = NULL;

typedef enum
{
    USB_ISR_MSG_UNDEFINED,
    USB_ISR_MSG_RESET,
    USB_ISR_MSG_SPEED_ENUM_DONE,
    USB_ISR_MSG_SETUP,
    USB_ISR_MSG_SUSPEND,
    USB_ISR_MSG_RESUME,

    USB_ISR_MSG_XFER_DONE,
    USB_ISR_MSG_IN_NAK,
} T_USB_ISR_MSG;

void usb_isr_device_driver_register(T_USB_DEVICE_DRIVER *driver)
{
    device_driver = driver;
}

void usb_isr_device_driver_unregister(T_USB_DEVICE_DRIVER *driver)
{
    device_driver = NULL;
}

USB_USER_SPEC_SECTION
static void usb_common_isr_enter(void)
{
    NVIC_DisableIRQ(USB_IP_IRQn);
}

static void usb_common_isr_handler(T_HAL_USB_COMMON_ISR isr, T_HAL_USB_ISR_PARAM *param)
{
    T_USB_TASK_MSG msg = {.group = USB_TASK_MSG_GROUP_HAL, .type = USB_ISR_MSG_UNDEFINED, };
    bool msg_send = true;

    switch (isr)
    {
    case HAL_USB_COMMON_ISR_RESET:
        {
            msg.type = USB_ISR_MSG_RESET;
        }
        break;

    case HAL_USB_COMMON_ISR_ENUM_DONE:
        {
            msg.type = USB_ISR_MSG_SPEED_ENUM_DONE;
            msg.var[0] = param->enum_done.speed;
        }
        break;

    case HAL_USB_COMMON_ISR_SETUP:
        {
            msg.type = USB_ISR_MSG_SETUP;
            memcpy(setup_pkt, param->setup.setup_pkt, 8);
            msg.complex.buf = setup_pkt;
            msg.complex.len = sizeof(setup_pkt);
        }
        break;

    case HAL_USB_COMMON_ISR_SUSPEND:
        {
            usb_dm_suspend_enter();
            msg.type = USB_ISR_MSG_SUSPEND;
        }
        break;

    case HAL_USB_COMMON_ISR_RESUME:
        {
            msg.type = USB_ISR_MSG_RESUME;
        }
        break;

    case HAL_USB_COMMON_ISR_XFER_DONE:
        {
            T_HAL_USB_REQUEST_BLOCK *urb = param->xfer_done.urb;
            if (urb->complete)
            {
                msg.type = USB_ISR_MSG_XFER_DONE;
                msg.complex.buf = (uint8_t *)param->xfer_done.urb;
                msg.complex.len = sizeof(T_HAL_USB_REQUEST_BLOCK);
            }
            else
            {
                msg_send = false;
            }
        }
        break;

    case HAL_USB_COMMON_ISR_IN_NAK:
        {
            T_USB_DM_EVT_PARAM evt_param;
            memset(&evt_param, 0, sizeof(T_USB_DM_EVT_PARAM));
            evt_param.ep_in_nak.ep_num = param->in_nak.ep_num;
            usb_dm_evt_dispatch(USB_DM_EVT_EP_IN_NAK, &evt_param);
        }
        break;

    default:
        msg_send = false;
        break;
    }

    if (msg_send)
    {
        USB_PRINT_INFO1("usb_common_isr_handler:%d", msg.type);
        usb_task_msg_send(&msg);
        usb_isr_pending++;
    }
}

#if FOR_SS_DONGLE_HID_USING_EP4
RAM_TEXT_SECTION
static void usb_ep4_isr_handler(T_HAL_USB_EP4_ISR isr, T_HAL_USB_ISR_PARAM *param)
{
    T_USB_TASK_MSG msg = {.group = USB_TASK_MSG_GROUP_HAL_EP4, .type = USB_ISR_MSG_UNDEFINED, };
    bool msg_send = true;

    switch (isr)
    {
    case HAL_USB_EP4_ISR_XFER_DONE:
        {
            T_HAL_USB_REQUEST_BLOCK *urb = param->xfer_done.urb;
            if (urb->complete)
            {
                msg.type = USB_ISR_MSG_XFER_DONE;
                msg.complex.buf = (uint8_t *)param->xfer_done.urb;
                msg.complex.len = sizeof(T_HAL_USB_REQUEST_BLOCK);
            }
            else
            {
                msg_send = false;
            }
        }
        break;

    default:
        msg_send = false;
        break;
    }

    if (msg_send)
    {
        USB_PRINT_INFO1("usb_ep4_isr_handler:%d", msg.type);
        usb_task_msg_send(&msg);
    }
}

void usb_isr_msg_handle_ep4(T_USB_TASK_MSG *usb_msg)
{
    T_USB_ISR_MSG type = (T_USB_ISR_MSG)usb_msg->type;

    if (usb_dm_state_get() == USB_PDN)
    {
        USB_PRINT_INFO1("usb_isr_msg_handle_ep4, usb already power down:%d", usb_isr_pending);
        return;
    }

    switch (type)
    {
    case USB_ISR_MSG_XFER_DONE:
        {
            T_HAL_USB_REQUEST_BLOCK *urb = (T_HAL_USB_REQUEST_BLOCK *)usb_msg->complex.buf;
            if (urb && urb->complete)
            {
                urb->complete(urb);
            }
        }
        break;

    default:
        break;
    }
    USB_PRINT_INFO1("usb_isr_msg_handle_ep4:%d, %d", type);
}
#endif

USB_USER_SPEC_SECTION
static void usb_common_isr_exit(void)
{
    if (usb_isr_pending == 0)
    {
        NVIC_EnableIRQ(USB_IP_IRQn);
    }
}

void usb_isr_msg_handle(T_USB_TASK_MSG *usb_msg)
{
    T_USB_ISR_MSG type = (T_USB_ISR_MSG)usb_msg->type;

    if (usb_dm_state_get() == USB_PDN)
    {
        USB_PRINT_INFO1("usb_isr_msg_handle, usb already power down:%d", usb_isr_pending);
        usb_isr_pending--;
        return;
    }

    switch (type)
    {
    case USB_ISR_MSG_RESET:
        {
            if (device_driver && device_driver->reset)
            {
                device_driver->reset();
            }
        }
        break;

    case USB_ISR_MSG_SPEED_ENUM_DONE:
        {
            uint8_t speed = (uint8_t)usb_msg->var[0];
            if (device_driver && device_driver->speed_enum_done)
            {
                device_driver->speed_enum_done(speed);
            }
        }
        break;

    case USB_ISR_MSG_SETUP:
        {
            uint8_t *setup_pkt = usb_msg->complex.buf;
            if (device_driver && device_driver->setup)
            {
                device_driver->setup(setup_pkt);
            }
            T_USB_DM_EVT_PARAM param = {.setup_peek.pkt = setup_pkt};
            usb_dm_evt_dispatch(USB_DM_EVT_SETUP_PEEK, &param);
        }
        break;

    case USB_ISR_MSG_SUSPEND:
        {
            if (device_driver && device_driver->suspend)
            {
                device_driver->suspend();
            }
        }
        break;

    case USB_ISR_MSG_RESUME:
        {
            if (device_driver && device_driver->resume)
            {
                device_driver->resume();
            }
        }
        break;

    case USB_ISR_MSG_XFER_DONE:
        {
            T_HAL_USB_REQUEST_BLOCK *urb = (T_HAL_USB_REQUEST_BLOCK *)usb_msg->complex.buf;
            if (urb && urb->complete)
            {
                urb->complete(urb);
            }
        }
        break;

    default:
        break;
    }
    USB_PRINT_INFO2("usb_isr_msg_handle:%d, %d", type, usb_isr_pending);
    usb_isr_pending--;
    if (usb_isr_pending == 0)
    {
        NVIC_EnableIRQ(USB_IP_IRQn);
    }
}


RAM_TEXT_SECTION
void usb_suspendn_isr_handler(void)
{
    usb_dm_suspend_exit();
}

void usb_isr_suspendn_enable(void)
{
#if defined(CONFIG_SOC_SERIES_RTL87X3D)
    uint32_t peripheral_mode = PERIPHINT->MODE;
    peripheral_mode |= BIT29;
    PERIPHINT->MODE = peripheral_mode;
    PERIPHINT->EDGE_MODE &= 0xDFFFFFFF;
#elif defined(CONFIG_SOC_SERIES_RTL8763E)
    SoC_VENDOR->REG_LOW_PRI_INT_MODE |= BIT2;
    SoC_VENDOR->Interrupt_edge_option &= (~BIT2);
#endif
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = USB_UTMI_SUSPEND_N_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 6;
    NVIC_Init(&nvic_init_struct);
}

void usb_isr_suspendn_disable(void)
{
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = USB_UTMI_SUSPEND_N_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)DISABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 6;
    NVIC_Init(&nvic_init_struct);
}

USB_USER_SPEC_SECTION
static const HAL_USB_COMMON_ISR_HOOKS usb_common_isr_hooks =
{
    .enter = usb_common_isr_enter,
    .handler = usb_common_isr_handler,
    .exit = usb_common_isr_exit,
};

#if FOR_SS_DONGLE_HID_USING_EP4
static const HAL_USB_EP4_ISR_HOOKS usb_ep4_isr_hooks =
{
    .handler = usb_ep4_isr_handler,
};
#endif

static HAL_USB_SUSPENDN_ISR_HOOKS usb_suspendn_isr_hooks =
{
    .enter = NULL,
    .handler = usb_suspendn_isr_handler,
    .exit = NULL,
};

void usb_isr_enable(void)
{
    hal_usb_common_isr_handler_update((HAL_USB_COMMON_ISR_HOOKS *)&usb_common_isr_hooks);
#if FOR_SS_DONGLE_HID_USING_EP4
    hal_usb_ep4_isr_handler_update((HAL_USB_EP4_ISR_HOOKS *)&usb_ep4_isr_hooks);
#endif
    hal_usb_suspendn_isr_handler_update((HAL_USB_SUSPENDN_ISR_HOOKS *)&usb_suspendn_isr_hooks);
    NVIC_SetPriority(USB_IP_IRQn, 4);
    NVIC_SetPriority(USB_ISOC_IRQn, 3);
    NVIC_EnableIRQ(USB_IP_IRQn);
    NVIC_EnableIRQ(USB_ISOC_IRQn);
    if (usb_isr_isoc_init)
    {
        usb_isr_isoc_init();
    }
    usb_isr_suspendn_enable();
    hal_usb_global_isr_enable();
}

void usb_isr_disable(void)
{
    hal_usb_global_isr_disable();
    usb_isr_suspendn_disable();
    NVIC_DisableIRQ(USB_ISOC_IRQn);
    NVIC_DisableIRQ(USB_IP_IRQn);
}
