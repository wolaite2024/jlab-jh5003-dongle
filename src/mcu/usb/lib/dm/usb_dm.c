#include <string.h>
#include <stdlib.h>
#include "os_task.h"
#include "rtl876x_nvic.h"
#include "errno.h"
#include "hal_usb.h"
#include "usb_dm.h"
#include "section.h"
#include "trace.h"
#include "usb_isr.h"
#include "usb_task.h"
#include "usb_dm_int.h"
#include "os_sync.h"
#include "usb_bc12_api.h"
#include "usb_ver.h"
#include "usb_composite_dev.h"
#include "hal_adp.h"
#include "pm.h"
#include "usb_utils.h"

typedef struct _usb_dm_cb_item
{
    struct _usb_dm_cb_item *p_next;
    T_USB_DM_EVT_MSK msk;
    USB_DM_CB cb;
} T_USB_DM_CB_ITEM;

static T_USB_POWER_STATE usb_pwr_state = USB_PDN;
static T_USB_POWER_STATE usb_pwr_state_before_suspend = USB_POWERED;
static T_USB_UTILS_LIST usb_dm_cb_list = {.p_first = NULL, .p_last = NULL, .count = 0, .flags = 0};
static uint8_t s_usb_dm_bc12_type = BC12_TYPE_ADP_ERROR;

static const char *const usb_commit = TO_STR(COMMIT);
static const char *const usb_build_date = TO_STR(BUILDING_TIME);
static const char *const usb_builder = TO_STR(BUILDER);

typedef enum
{
    USB_DM_MSG_START,
    USB_DM_MSG_STOP,
} T_USB_DM_MSG;

RAM_TEXT_SECTION
int usb_dm_state_set(T_USB_POWER_STATE state)
{
    if (state != usb_pwr_state)
    {
        USB_PRINT_INFO2("usb_dm_state_set, pre:%d, new:%d", usb_pwr_state, state);
        usb_pwr_state = state;

        T_USB_DM_EVT_PARAM evt_param;
        memset(&evt_param, 0, sizeof(T_USB_DM_EVT_PARAM));
        evt_param.status_ind.state = state;
        /*reset done*/
        if (state == USB_DEFAULT)
        {
            uint8_t speed = (usb_composite_dev_enum_speed_get() == HAL_USB_SPEED_FULL) ? USB_SPEED_FULL :
                            USB_SPEED_HIGH;
            evt_param.status_ind.info.speed = speed;
        }
        usb_dm_evt_dispatch(USB_DM_EVT_STATUS_IND, &evt_param);
    }

    return ESUCCESS;
}

RAM_TEXT_SECTION
T_USB_POWER_STATE usb_dm_state_get(void)
{
    return usb_pwr_state;
}

static bool usb_dm_lpm_mode_valid(void)
{
    bool ret = true;
#if CONFIG_SOC_SERIES_RTL87X3D
    ret = ((power_mode_get() == POWER_LPS_MODE) || (power_mode_get() == POWER_DLPS_MODE));
#elif CONFIG_SOC_SERIES_RTL8763E
    ret = (power_mode_get() == POWER_LPS_MODE);
#endif
    return ret;
}

bool usb_dm_lpm_check(void)
{
    uint8_t usb_dm_state = usb_dm_state_get();

    if ((usb_dm_state == USB_SUSPENDED && usb_dm_lpm_mode_valid()) || usb_dm_state == USB_PDN)
    {
        return true;
    }

    return false;
}

void usb_dm_lpm_store(void)
{
#if CONFIG_SOC_SERIES_RTL87X3D
    hal_usb_wakeup_status_clear();
#elif CONFIG_SOC_SERIES_RTL8763E
    if (usb_dm_state_get() > USB_PDN)
    {
        usb_isr_disable();
    }
#endif
}

void usb_dm_lpm_restore(void)
{
#if CONFIG_SOC_SERIES_RTL87X3D
    if (usb_dm_state_get() > USB_PDN)
    {
        USB_PRINT_TRACE1("usb_dm_lpm_restore, usb_wakeup_status %d", hal_usb_wakeup_status_get());
    }

    if (hal_usb_wakeup_status_get())
    {
        usb_dm_suspend_exit();
    }
#endif
    if (usb_dm_state_get() > USB_PDN)
    {
        usb_isr_enable();
    }

}

void usb_dm_register_pm_cb(void)
{
    power_check_cb_register(usb_dm_lpm_check);// register cb
    power_stage_cb_register(usb_dm_lpm_store, POWER_STAGE_STORE);
    power_stage_cb_register(usb_dm_lpm_restore, POWER_STAGE_RESTORE);// register cb
}

static void usb_dm_ver_print(void)
{
    USB_PRINT_INFO3("usb lib info, commit: %s, build date:%s, builder:%s",
                    TRACE_STRING(usb_commit), TRACE_STRING(usb_build_date), TRACE_STRING(usb_builder));
}

static void usb_dm_adp_state_change_cb(T_ADP_PLUG_EVENT event, void *user_data)
{
    if (event == ADP_EVENT_PLUG_OUT)
    {
        if (s_usb_dm_bc12_type == BC12_TYPE_DCP_1P5A)
        {
            usb_bc12_power_down();
        }
        s_usb_dm_bc12_type = BC12_TYPE_ADP_ERROR;
    }
}

void usb_dm_start(bool bc12_detect)
{
    T_USB_TASK_MSG msg = {.group = USB_TASK_MSG_GROUP_DM, .type = USB_DM_MSG_START, .var[0] = bc12_detect};
    usb_task_msg_send(&msg);
}

void usb_dm_core_init(T_USB_CORE_CONFIG config)
{
    usb_dm_ver_print();

    hal_usb_speed_set((T_HAL_USB_SPEED)config.speed);
    hal_usb_init();
    usb_dm_register_pm_cb();
    usb_task_create();
    adp_register_state_change_cb(ADP_DETECT_5V, (P_ADP_PLUG_CBACK)usb_dm_adp_state_change_cb, NULL);
}

int usb_dm_suspend_enter(void)
{
    uint32_t s;
    s = os_lock();
    hal_usb_suspend_enter();
    usb_pwr_state_before_suspend = usb_dm_state_get();
    usb_dm_state_set(USB_SUSPENDED);
    os_unlock(s);

    return ESUCCESS;
}

RAM_TEXT_SECTION
int usb_dm_suspend_exit(void)
{
    uint32_t s;
    s = os_lock();
    if (usb_dm_state_get() == USB_SUSPENDED)
    {
        usb_dm_state_set(usb_pwr_state_before_suspend);
        if (hal_usb_suspend_exit() != 0)
        {
            usb_dm_state_set(USB_PDN);
        }
    }
    os_unlock(s);

    return ESUCCESS;
}

void usb_dm_stop(void)
{
    T_USB_TASK_MSG msg = {.group = USB_TASK_MSG_GROUP_DM, .type = USB_DM_MSG_STOP, };
    usb_task_msg_send(&msg);
}

int usb_dm_msg_handle(T_USB_TASK_MSG *usb_msg)
{
    T_USB_DM_MSG type = (T_USB_DM_MSG)usb_msg->type;
    switch (type)
    {
    case USB_DM_MSG_START:
        {
            bool bc12_detect = (bool)usb_msg->var[0];
            if (bc12_detect)
            {
                uint8_t bc12_type = usb_bc12_type_get();
                s_usb_dm_bc12_type = bc12_type;
                T_USB_DM_EVT_PARAM evt_param;
                memset(&evt_param, 0, sizeof(T_USB_DM_EVT_PARAM));
                evt_param.bc12_det.type = bc12_type;
                usb_dm_evt_dispatch(USB_DM_EVT_BC12_DETECT, &evt_param);
                if ((bc12_type  != BC12_TYPE_SDP_0P5A) && (bc12_type  != BC12_TYPE_CDP_1P5A)
                    && (bc12_type  != BC12_TYPE_OTHERS_USER_DEFINED_0P5A))
                {
                    return -EFAULT;
                }
            }

            if (usb_dm_state_get() == USB_PDN)
            {
                usb_dm_state_set(USB_ATTACHED);
                hal_usb_phy_power_on();
                usb_dm_state_set(USB_POWERED);
                hal_usb_mac_init();
                usb_isr_enable();
                hal_usb_soft_attach();
            }
        }
        break;

    case USB_DM_MSG_STOP:
        {
            if (usb_dm_state_get() >= USB_POWERED)
            {
                hal_usb_soft_detach();
                usb_isr_disable();
                hal_usb_mac_deinit();
                hal_usb_phy_power_down();
                usb_dm_state_set(USB_PDN);
                usb_pwr_state_before_suspend = USB_PDN;
            }
        }
        break;

    default:
        break;
    }
    USB_PRINT_INFO1("usb_dm_msg_handle: %d", type);

    return 0;
}

void usb_dm_evt_msk_change(T_USB_DM_EVT_MSK evt_msk, USB_DM_CB cb)
{
    int s = 0;
    T_USB_DM_CB_ITEM *item = NULL;
    usb_dm_cb_list.flags = 0;

    s = os_lock();
    USB_UTILS_LIST_FOREACH(&usb_dm_cb_list, T_USB_DM_CB_ITEM *,  item)
    {
        if (cb == item->cb)
        {
            item->msk.d16 = evt_msk.d16;
        }
        usb_dm_cb_list.flags |= item->msk.d16;
    }
    os_unlock(s);
}

void usb_dm_cb_register(T_USB_DM_EVT_MSK evt_msk, USB_DM_CB cb)
{
    int s = 0;
    T_USB_DM_CB_ITEM *item = malloc(sizeof(T_USB_DM_CB_ITEM));

    if (item)
    {
        item->msk.d16 = evt_msk.d16;
        item->cb = cb;
        s = os_lock();
        USB_UTILS_LIST_INSERT_TAIL(&usb_dm_cb_list, item);
        usb_dm_cb_list.flags |= evt_msk.d16;
        os_unlock(s);
    }
}

void usb_dm_cb_unregister(USB_DM_CB cb)
{
    T_USB_DM_CB_ITEM *item = NULL;
    int s = 0;
    usb_dm_cb_list.flags = 0;

    s = os_lock();
    USB_UTILS_LIST_FOREACH(&usb_dm_cb_list, T_USB_DM_CB_ITEM *,  item)
    {
        if (item->cb == cb)
        {
            USB_UTILS_LIST_REMOVE(&usb_dm_cb_list, item);
            free(item);
        }
        else
        {
            usb_dm_cb_list.flags |= item->msk.d16;
        }
    }
    os_unlock(s);

}

RAM_TEXT_SECTION
void usb_dm_evt_dispatch(T_USB_DM_EVT evt, T_USB_DM_EVT_PARAM *param)
{
    T_USB_DM_CB_ITEM *item = NULL;
    USB_DM_CB cb = NULL;
    int s = 0;

    if ((usb_dm_cb_list.flags & (1 << evt)) == 0)
    {
        return;
    }
    s = os_lock();
    USB_UTILS_LIST_FOREACH(&usb_dm_cb_list, T_USB_DM_CB_ITEM *,  item)
    {
        cb = item->cb;
        os_unlock(s);
        if ((item->msk.d16 & (1 << evt)) && cb)
        {
            cb(evt, param);
        }
        os_lock();
    }
    os_unlock(s);
}

