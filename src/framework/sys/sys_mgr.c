/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "os_mem.h"
#include "trace.h"
#include "sysm.h"
#include "sys_ipc.h"
#include "sys_timer.h"
#include "sys_event.h"
#include "sys_mgr.h"

/* TODO Remove Start */
void *hEventQueueHandleAu;
/* TODO Remove End */

typedef enum t_sys_state
{
    SYS_STATE_OFF           = 0x00,
    SYS_STATE_ON            = 0x01,
} T_SYS_STATE;

typedef struct t_sys_cback_item
{
    struct t_sys_cback_item *p_next;
    P_SYS_MGR_CBACK          cback;
} T_SYS_CBACK_ITEM;

typedef struct t_sys_sched_item
{
    bool        used;
    uint8_t     event;
    void (*entry)(void);
} T_SYS_SCHED_ITEM;

#define SYS_SCHED_ITEM_SIZE     10

typedef struct t_sys_db
{
    T_SYS_STATE         state;      /* system manager state */
    T_OS_QUEUE          cback_list; /* system manager callback list */
    T_SYS_SCHED_ITEM    sched_item[SYS_SCHED_ITEM_SIZE];
} T_SYS_DB;

static T_SYS_DB sys_db;

bool sys_mgr_event_post(T_SYS_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    T_SYS_CBACK_ITEM *p_item;

    SYS_PRINT_INFO1("sys_mgr_event_post: event_type 0x%04x", event_type);

    p_item = (T_SYS_CBACK_ITEM *)sys_db.cback_list.p_first;
    while (p_item != NULL)
    {
        p_item->cback(event_type, event_buf, buf_len);
        p_item = p_item->p_next;
    }

    return true;
}

void sys_mgr_power_on(void)
{
    SYS_PRINT_INFO1("sys_mgr_power_on: state 0x%02x", sys_db.state);

    if (sys_db.state == SYS_STATE_OFF)
    {
        T_SYS_EVENT_PARAM param;

        sys_db.state = SYS_STATE_ON;
        sys_ipc_publish(SYS_IPC_TOPIC, SYS_IPC_POWER_ON, NULL);

        sys_mgr_event_post(SYS_EVENT_POWER_ON, &param.power_on, sizeof(param.power_on));
    }
}

void sys_mgr_power_off(void)
{
    SYS_PRINT_INFO1("sys_mgr_power_off: state 0x%02x", sys_db.state);

    if (sys_db.state == SYS_STATE_ON)
    {
        T_SYS_EVENT_PARAM param;

        sys_db.state = SYS_STATE_OFF;
        sys_ipc_publish(SYS_IPC_TOPIC, SYS_IPC_POWER_OFF, NULL);

        sys_mgr_event_post(SYS_EVENT_POWER_OFF, &param.power_off, sizeof(param.power_off));
    }
}

void sys_mgr_event_handle(uint8_t event)
{
    uint8_t i;

    for (i = 0; i < SYS_SCHED_ITEM_SIZE; i++)
    {
        if (sys_db.sched_item[i].used == true &&
            sys_db.sched_item[i].event == event)
        {
            if (sys_db.sched_item[i].entry != NULL)
            {
                sys_db.sched_item[i].entry();
            }
            break;
        }
    }
}

bool sys_mgr_event_register(uint8_t event, void (*entry)(void))
{
    uint8_t i;

    for (i = 0; i < SYS_SCHED_ITEM_SIZE; i++)
    {
        if (sys_db.sched_item[i].used == false)
        {
            sys_db.sched_item[i].used  = true;
            sys_db.sched_item[i].event = event;
            sys_db.sched_item[i].entry = entry;
            return true;
        }
    }

    return false;
}

bool sys_mgr_cback_register(P_SYS_MGR_CBACK cback)
{
    T_SYS_CBACK_ITEM *p_item;

    p_item = (T_SYS_CBACK_ITEM *)sys_db.cback_list.p_first;
    while (p_item != NULL)
    {
        /* Sanity check if callback already registered */
        if (p_item->cback == cback)
        {
            return true;
        }

        p_item = p_item->p_next;
    }

    p_item = os_mem_alloc2(sizeof(T_SYS_CBACK_ITEM));
    if (p_item != NULL)
    {
        p_item->cback = cback;
        os_queue_in(&sys_db.cback_list, p_item);
        return true;
    }

    return false;
}

bool sys_mgr_cback_unregister(P_SYS_MGR_CBACK cback)
{
    T_SYS_CBACK_ITEM *p_item;
    bool              ret = false;

    p_item = (T_SYS_CBACK_ITEM *)sys_db.cback_list.p_first;
    while (p_item != NULL)
    {
        if (p_item->cback == cback)
        {
            os_queue_delete(&sys_db.cback_list, p_item);
            os_mem_free(p_item);
            ret = true;
            break;
        }

        p_item = p_item->p_next;
    }

    return ret;
}

bool sys_mgr_init(void *evt_handle)
{
    hEventQueueHandleAu = evt_handle;

    sys_db.state = SYS_STATE_OFF;
    os_queue_init(&sys_db.cback_list);
    memset(sys_db.sched_item, 0, sizeof(sys_db.sched_item));

    sys_ipc_init();
    sys_timer_init();
    sys_event_init();

    return true;
}
