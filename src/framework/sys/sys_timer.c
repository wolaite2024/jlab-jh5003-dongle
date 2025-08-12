/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "os_msg.h"
#include "os_timer.h"
#include "os_queue.h"
#include "os_sched.h"
#include "trace.h"
#include "section.h"
#include "pm.h"
#include "hw_tim.h"
#include "sys_timer.h"

/* TODO Remove Start */
#include "sys_mgr.h"
#include "app_msg.h"
extern void *hEventQueueHandleAu;
/* TODO Remove End */

#define SYS_MSG_MAX_NUM         0x20

#define SYS_MSG_TYPE_LP_TIMER   0x0000
#define SYS_MSG_TYPE_HP_TIMER   0x0001

typedef struct
{
    uint8_t     type;
    uint8_t     subtype;
    uint16_t    len;
    void       *buf;
} T_SYS_QUEUE_MSG;

typedef struct t_sys_timer
{
    struct t_sys_timer *p_next;             /* timer pointed to next item in the pending list */
    const char         *timer_name;         /* timer name string */
    uint32_t            timer_id;           /* timer parameter for callback */
    uint32_t            timeout_interval;   /* timer period in microseconds */
    uint32_t            timeout_left;       /* timer period remained in microseconds */
    T_SYS_TIMER_TYPE    timer_type;         /* low or high precision timer type */
    bool                reload;             /* true if it is periodic timer */
    P_SYS_TIMER_CBACK   cback;              /* timer expiring callback */
    void               *sw_timer_handle;    /* software timer handle for low precision timer */
} T_SYS_TIMER;

typedef struct t_sys_timer_db
{
    T_OS_QUEUE         lp_timer_list;
    T_OS_QUEUE         hp_timer_list;
    void              *msg_queue;
    T_HW_TIMER_HANDLE  hw_timer_handle;
    uint32_t           timeout_interval;
    uint32_t           hp_timeout_time;
} T_SYS_TIMER_DB;

static T_SYS_TIMER_DB sys_timer_db;

ISR_TEXT_SECTION void sys_sw_timer_handler(void *handle)
{
    uint8_t         event;
    T_SYS_QUEUE_MSG msg;

    event       = EVENT_SYS_MSG;
    msg.type    = SYS_MSG_TYPE_LP_TIMER;
    msg.subtype = 0;
    msg.len     = sizeof(void *);
    msg.buf     = handle; /* OS layer software timer instance */

    if (hEventQueueHandleAu != NULL)
    {
        if (os_msg_send(sys_timer_db.msg_queue, &msg, 0) == true)
        {
            os_msg_send(hEventQueueHandleAu, &event, 0);
        }
    }
}

ISR_TEXT_SECTION void sys_hw_timer_handler(void *handle)
{
    uint8_t         event;
    T_SYS_QUEUE_MSG msg;

    event       = EVENT_SYS_MSG;
    msg.type    = SYS_MSG_TYPE_HP_TIMER;
    msg.subtype = 0;
    msg.len     = sizeof(void *);
    msg.buf     = handle; /* Hardware timer instance */

    if (hEventQueueHandleAu != NULL)
    {
        if (os_msg_send(sys_timer_db.msg_queue, &msg, 0) == true)
        {
            os_msg_send(hEventQueueHandleAu, &event, 0);
        }
    }

    sys_timer_db.hp_timeout_time = os_hp_time_get();
}

RAM_TEXT_SECTION bool sys_hp_timer_pm_check(void)
{
    T_SYS_TIMER *item;

    item = os_queue_peek(&sys_timer_db.hp_timer_list, 0);
    if (item == NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

T_SYS_TIMER *sys_lp_timer_list_find(void *handle)
{
    T_SYS_TIMER *item;

    item = os_queue_peek(&sys_timer_db.lp_timer_list, 0);
    while (item != NULL)
    {
        if (item->sw_timer_handle == handle)
        {
            break;
        }

        item = item->p_next;
    }

    return item;
}

void sys_lp_timer_list_insert(T_SYS_TIMER *p_timer)
{
    if (os_queue_search(&sys_timer_db.lp_timer_list, p_timer) == false)
    {
        /* Insert the timer in the pending timer list. */
        os_queue_in(&sys_timer_db.lp_timer_list, p_timer);
    }

    os_timer_restart(&p_timer->sw_timer_handle, p_timer->timeout_interval / 1000);
}

void sys_lp_timer_list_remove(T_SYS_TIMER *p_timer)
{
    if (os_queue_search(&sys_timer_db.lp_timer_list, p_timer) == true)
    {
        /* Remove OS layer software timer instance first. */
        os_timer_stop(&p_timer->sw_timer_handle);

        /* Remove the timer from the pending list. */
        os_queue_delete(&sys_timer_db.lp_timer_list, p_timer);
    }
}

void sys_lp_timer_list_free(T_SYS_TIMER *p_timer)
{
    /* Free OS layer software timer instance first. */
    os_timer_delete(&p_timer->sw_timer_handle);

    /* Remove the timer from the pending list. */
    os_queue_delete(&sys_timer_db.lp_timer_list, p_timer);

    /* Free the timer instance. */
    os_mem_free(p_timer);
}

void sys_hp_timer_elapsed_time_update(void)
{
    T_SYS_TIMER *item;
    uint32_t     timeout_elapsed;

    hw_timer_get_elapsed_time(sys_timer_db.hw_timer_handle, &timeout_elapsed);
    if (timeout_elapsed == 0)
    {
        timeout_elapsed = sys_timer_db.timeout_interval + os_hp_time_get() - sys_timer_db.hp_timeout_time;
    }
    else
    {
        sys_timer_db.hp_timeout_time = os_hp_time_get();
        sys_timer_db.timeout_interval = 0;
    }

    /* Update the elapsed time in the pending list. */
    item = os_queue_peek(&sys_timer_db.hp_timer_list, 0);
    while (item != NULL &&
           timeout_elapsed != 0)
    {
        if (item->timeout_left >= timeout_elapsed)
        {
            item->timeout_left -= timeout_elapsed;
            timeout_elapsed     = 0;
            break;
        }
        else
        {
            timeout_elapsed    -= item->timeout_left;
            item->timeout_left  = 0;
        }

        item = item->p_next;
    }
}

void sys_hp_timer_list_insert(T_SYS_TIMER *p_timer)
{
    T_SYS_TIMER *item;
    T_SYS_TIMER *prev_item;

    /* In case the active timer is restarted */
    p_timer->timeout_left = p_timer->timeout_interval;

    item = os_queue_peek(&sys_timer_db.hp_timer_list, 0);
    if (item == NULL)
    {
        /* Insert the timer in the pending timer list. */
        os_queue_in(&sys_timer_db.hp_timer_list, p_timer);

        sys_timer_db.timeout_interval = p_timer->timeout_left;
        hw_timer_restart(sys_timer_db.hw_timer_handle, p_timer->timeout_left);
    }
    else
    {
        bool     timer_active;

        sys_hp_timer_elapsed_time_update();
        hw_timer_is_active(sys_timer_db.hw_timer_handle, &timer_active);
        hw_timer_stop(sys_timer_db.hw_timer_handle);

        /* Insert the timer into the pending list in a relative interval order. */
        prev_item = NULL;
        item = os_queue_peek(&sys_timer_db.hp_timer_list, 0);
        while (item != NULL)
        {
            if (item->timeout_left <= p_timer->timeout_left)
            {
                p_timer->timeout_left -= item->timeout_left;
                prev_item              = item;
                item                   = item->p_next;
            }
            else
            {
                item->timeout_left -= p_timer->timeout_left;
                break;
            }
        }

        if (item == NULL)
        {
            os_queue_in(&sys_timer_db.hp_timer_list, p_timer);
        }
        else
        {
            os_queue_insert(&sys_timer_db.hp_timer_list, prev_item, p_timer);
        }

        /* Restart hw timer with the updated timeout_left value. Even when the
         * timeout_left is 0, we shall not invoke the callback directly due to
         * recursive issues.
         */
        if (timer_active == true)
        {
            item = os_queue_peek(&sys_timer_db.hp_timer_list, 0);
            if (item != NULL)
            {
                sys_timer_db.timeout_interval = item->timeout_left;
                hw_timer_restart(sys_timer_db.hw_timer_handle, item->timeout_left);
            }
        }
    }
}

void sys_hp_timer_list_remove(T_SYS_TIMER *p_timer)
{
    T_SYS_TIMER *item;

    if (os_queue_search(&sys_timer_db.hp_timer_list, p_timer) == true)
    {
        bool     timer_active;

        sys_hp_timer_elapsed_time_update();
        hw_timer_is_active(sys_timer_db.hw_timer_handle, &timer_active);
        hw_timer_stop(sys_timer_db.hw_timer_handle);

        /* Update timeout_left value of the timer that follows the removed timer. */
        item = p_timer->p_next;
        if (item != NULL)
        {
            item->timeout_left += p_timer->timeout_left;
        }

        /* Remove the timer from the pending list. */
        os_queue_delete(&sys_timer_db.hp_timer_list, p_timer);

        /* Restart hw timer with the updated timeout_left value. Even when the
         * timeout_left is 0, we shall not invoke the callback directly due to
         * recursive issues.
         */
        if (timer_active == true)
        {
            item = os_queue_peek(&sys_timer_db.hp_timer_list, 0);
            if (item != NULL)
            {
                sys_timer_db.timeout_interval = item->timeout_left;
                hw_timer_restart(sys_timer_db.hw_timer_handle, item->timeout_left);
            }
        }
    }
}

void sys_lp_timer_expire(void *handle)
{
    T_SYS_TIMER *p_timer;

    p_timer = sys_lp_timer_list_find(handle);
    if (p_timer != NULL)
    {
        /* Here it does not need to check if it is a periodic timer, as OS layer software
         * timer will handle this case.
         */
        if (p_timer->cback != NULL)
        {
            p_timer->cback((T_SYS_TIMER_HANDLE)p_timer);
        }
    }
}

void sys_hp_timer_expire(void)
{
    T_SYS_TIMER *p_timer;
    T_SYS_TIMER *item;

    sys_timer_db.timeout_interval = 0;
    hw_timer_stop(sys_timer_db.hw_timer_handle);

    /* First item will be the expired timer in the pending list. */
    p_timer = os_queue_out(&sys_timer_db.hp_timer_list);
    if (p_timer != NULL)
    {
        bool timer_active;

        /* It is safe to start, stop or delete the expired timer in its callback. */
        if (p_timer->cback != NULL)
        {
            /* Insert the reloading timer before invoking its previous callback. */
            if (p_timer->reload == true)
            {
                sys_hp_timer_list_insert(p_timer);
            }

            p_timer->cback((T_SYS_TIMER_HANDLE)p_timer);
        }

        /* Avoid re-enabling hw timer when starting timer in previous callback. */
        hw_timer_is_active(sys_timer_db.hw_timer_handle, &timer_active);
        if (timer_active == false)
        {
            item = os_queue_peek(&sys_timer_db.hp_timer_list, 0);
            if (item != NULL)
            {
                sys_hp_timer_elapsed_time_update();
                sys_timer_db.timeout_interval = item->timeout_left;
                hw_timer_restart(sys_timer_db.hw_timer_handle, item->timeout_left);
            }
        }
    }
}

void sys_msg_handler(void)
{
    T_SYS_QUEUE_MSG msg;

    if (os_msg_recv(sys_timer_db.msg_queue, &msg, 0) == true)
    {
        switch (msg.type)
        {
        case SYS_MSG_TYPE_LP_TIMER:
            sys_lp_timer_expire(msg.buf);
            break;

        case SYS_MSG_TYPE_HP_TIMER:
            sys_hp_timer_expire();
            break;

        default:
            break;
        }
    }
}

bool sys_timer_init(void)
{
    int32_t ret = 0;

    os_queue_init(&sys_timer_db.lp_timer_list);
    os_queue_init(&sys_timer_db.hp_timer_list);

    sys_timer_db.hw_timer_handle = hw_timer_create("SYS Timer",
                                                   0,
                                                   false,
                                                   sys_hw_timer_handler);
    if (sys_timer_db.hw_timer_handle == NULL)
    {
        ret = 1;
        goto fail_create_hw_timer;
    }

    if (os_msg_queue_create(&sys_timer_db.msg_queue,
                            "sys_timerQ",
                            SYS_MSG_MAX_NUM,
                            sizeof(T_SYS_QUEUE_MSG)) == false)
    {
        ret = 2;
        goto fail_create_msg_queue;
    }

    sys_mgr_event_register(EVENT_SYS_MSG, sys_msg_handler);

    power_check_cb_register(sys_hp_timer_pm_check);
    return true;

fail_create_msg_queue:
    hw_timer_delete(sys_timer_db.hw_timer_handle);
    sys_timer_db.hw_timer_handle = NULL;
fail_create_hw_timer:
    SYS_PRINT_ERROR1("sys_timer_init: failed %d", -ret);
    return false;
}

uint32_t sys_timer_id_get(T_SYS_TIMER_HANDLE handle)
{
    T_SYS_TIMER *p_timer;

    p_timer = (T_SYS_TIMER *)handle;

    return p_timer->timer_id;
}

T_SYS_TIMER_HANDLE sys_timer_create(const char *timer_name,
                                    T_SYS_TIMER_TYPE timer_type,
                                    uint32_t timer_id,
                                    uint32_t timeout_us,
                                    bool reload,
                                    P_SYS_TIMER_CBACK cback)
{
    T_SYS_TIMER *p_timer;

    p_timer = os_mem_alloc2(sizeof(T_SYS_TIMER));
    if (p_timer != NULL)
    {
        p_timer->timer_name         = timer_name;
        p_timer->timer_id           = timer_id;
        p_timer->timeout_interval   = timeout_us;
        p_timer->timeout_left       = timeout_us;
        p_timer->timer_type         = timer_type;
        p_timer->reload             = reload;
        p_timer->cback              = cback;
        p_timer->sw_timer_handle    = NULL;

        if (p_timer->timer_type == SYS_TIMER_TYPE_LOW_PRECISION)
        {
            /* Low precision sys timer is based on OS layer software timer. */
            if (os_timer_create(&p_timer->sw_timer_handle,
                                p_timer->timer_name,
                                p_timer->timer_id,
                                p_timer->timeout_interval / 1000,
                                p_timer->reload,
                                sys_sw_timer_handler) == false)
            {
                os_mem_free(p_timer);
                p_timer = NULL;
            }
        }
    }

    SYS_PRINT_TRACE6("sys_timer_create: handle %p, name %s, type %u, id 0x%08x, reload %u, timeout 0x%08x",
                     p_timer, TRACE_STRING(timer_name), timer_type, timer_id, reload, timeout_us);

    return (T_SYS_TIMER_HANDLE)p_timer;
}

bool sys_timer_start(T_SYS_TIMER_HANDLE handle)
{
    T_SYS_TIMER *p_timer;

    SYS_PRINT_TRACE1("sys_timer_start: handle %p", handle);

    p_timer = (T_SYS_TIMER *)handle;
    if (p_timer != NULL)
    {
        if (p_timer->timer_type == SYS_TIMER_TYPE_LOW_PRECISION)
        {
            sys_lp_timer_list_insert(p_timer);
        }
        else
        {
            /* Remove the timer from the pending timer list if started. */
            sys_hp_timer_list_remove(p_timer);

            /* Insert the timer in the pending timer list in the ascending order. */
            sys_hp_timer_list_insert(p_timer);
        }

        return true;
    }

    return false;
}

bool sys_timer_restart(T_SYS_TIMER_HANDLE handle, uint32_t timeout_us)
{
    T_SYS_TIMER *p_timer;

    SYS_PRINT_TRACE2("sys_timer_restart: handle %p, timeout 0x%08x", handle, timeout_us);

    p_timer = (T_SYS_TIMER *)handle;
    if (p_timer != NULL)
    {
        if (p_timer->timer_type == SYS_TIMER_TYPE_LOW_PRECISION)
        {
            /* Update timeout period before restarting. */
            p_timer->timeout_interval = timeout_us;
            p_timer->timeout_left     = timeout_us;

            sys_lp_timer_list_insert(p_timer);
        }
        else
        {
            /* Remove the timer from the pending timer list if started. */
            sys_hp_timer_list_remove(p_timer);

            /* Update timeout period before restarting. */
            p_timer->timeout_interval = timeout_us;

            /* Insert the timer in the pending timer list in the ascending order. */
            sys_hp_timer_list_insert(p_timer);
        }

        return true;
    }

    return false;
}

bool sys_timer_stop(T_SYS_TIMER_HANDLE handle)
{
    T_SYS_TIMER *p_timer;

    SYS_PRINT_TRACE1("sys_timer_stop: handle %p", handle);

    p_timer = (T_SYS_TIMER *)handle;
    if (p_timer != NULL)
    {
        /* Remove the timer from the pending timer list if started. */
        if (p_timer->timer_type == SYS_TIMER_TYPE_LOW_PRECISION)
        {
            sys_lp_timer_list_remove(p_timer);
        }
        else
        {
            sys_hp_timer_list_remove(p_timer);
        }

        return true;
    }

    return false;
}

bool sys_timer_delete(T_SYS_TIMER_HANDLE handle)
{
    T_SYS_TIMER *p_timer;

    SYS_PRINT_TRACE1("sys_timer_delete: handle %p", handle);

    p_timer = (T_SYS_TIMER *)handle;
    if (p_timer != NULL)
    {
        /* Remove the timer from the pending timer list if started. */
        if (p_timer->timer_type == SYS_TIMER_TYPE_LOW_PRECISION)
        {
            sys_lp_timer_list_free(p_timer);
        }
        else
        {
            sys_hp_timer_list_remove(p_timer);
            os_mem_free(p_timer);
        }

        return true;
    }

    return false;
}
