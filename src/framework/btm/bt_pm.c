/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "trace.h"
#include "gap_br.h"
#include "sys_timer.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"

void bt_pm_timeout_cback(T_SYS_TIMER_HANDLE handle)
{
    T_BT_LINK *link;

    link = (void *)sys_timer_id_get(handle);

    BTM_PRINT_TRACE2("bt_pm_timeout_cback: handle %p, link %p",
                     handle, link);

    if (link != NULL)
    {
        if (link->acl_link_state == BT_LINK_STATE_CONNECTED)
        {
            bt_pm_sm(link->bd_addr, BT_PM_EVENT_SNIFF_ENTER_REQ);
        }
    }
}

bool bt_pm_cback_register(uint8_t       bd_addr[6],
                          P_BT_PM_CBACK cback)
{
    if (cback != NULL)
    {
        T_BT_LINK *link;

        link = bt_link_find(bd_addr);
        if (link)
        {
            T_BT_PM_CBACK_ITEM *item;

            item = (T_BT_PM_CBACK_ITEM *)link->pm_cback_list.p_first;
            while (item != NULL)
            {
                if (item->cback == cback)
                {
                    return false;
                }

                item = item->p_next;
            }

            item = os_mem_alloc2(sizeof(T_BT_PM_CBACK_ITEM));
            if (item != NULL)
            {
                item->cback = cback;
                os_queue_in(&link->pm_cback_list, item);
                return true;
            }
        }
    }

    return false;
}

bool bt_pm_cback_unregister(uint8_t       bd_addr[6],
                            P_BT_PM_CBACK cback)
{
    if (cback != NULL)
    {
        T_BT_LINK *link;

        link = bt_link_find(bd_addr);
        if (link)
        {
            T_BT_PM_CBACK_ITEM *item;

            item = (T_BT_PM_CBACK_ITEM *)link->pm_cback_list.p_first;
            while (item != NULL)
            {
                if (item->cback == cback)
                {
                    os_queue_delete(&link->pm_cback_list, item);
                    os_mem_free(item);
                    return true;
                }

                item = item->p_next;
            }
        }
    }

    return false;
}

bool bt_pm_state_get(uint8_t             bd_addr[6],
                     T_BT_LINK_PM_STATE *pm_state)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link)
    {
        *pm_state = link->pm_state;
        return true;
    }

    return false;
}

bool bt_sniff_mode_config(uint8_t  bd_addr[6],
                          uint16_t sniff_interval,
                          uint16_t sniff_attempt,
                          uint16_t sniff_timeout,
                          uint32_t pm_timeout)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        link->pm_enable     = true;
        link->pm_timeout    = pm_timeout;
        link->max_interval  = sniff_interval + BT_SNIFF_INTERVAL_OFFSET / 2;
        link->min_interval  = sniff_interval - BT_SNIFF_INTERVAL_OFFSET / 2;
        link->sniff_attempt = sniff_attempt;
        link->sniff_timeout = sniff_timeout;
        link->timer_sniff   = sys_timer_create("bt_sniff",
                                               SYS_TIMER_TYPE_LOW_PRECISION,
                                               (uint32_t)link,
                                               pm_timeout,
                                               false,
                                               bt_pm_timeout_cback);
        os_queue_init(&link->pm_cback_list);

        return true;
    }

    return false;
}

bool bt_sniff_mode_enable(uint8_t  bd_addr[6],
                          uint16_t min_interval,
                          uint16_t max_interval,
                          uint16_t sniff_attempt,
                          uint16_t sniff_timeout)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        bool exit = false;

        if (link->pm_enable == false)
        {
            link->pm_enable = true;
            bt_link_policy_set(bd_addr, link->acl_link_policy | GAP_LINK_POLICY_SNIFF_MODE);
        }

        if (min_interval != 0 && max_interval != 0)
        {
            link->min_interval  = min_interval;
            link->max_interval  = max_interval;
            exit = true;
        }

        if (sniff_attempt != 0)
        {
            link->sniff_attempt = sniff_attempt;
            exit = true;
        }

        if (sniff_timeout != 0)
        {
            link->sniff_timeout = sniff_timeout;
            exit = true;
        }

        if (exit == true)
        {
            bt_pm_sm(link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_REQ);
        }

        return true;
    }

    return false;
}

bool bt_sniff_mode_disable(uint8_t bd_addr[6])
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        if (link->pm_enable == true)
        {
            link->pm_enable = false;

            if (link->pm_state == BT_LINK_PM_STATE_ACTIVE)
            {
                bt_link_policy_set(bd_addr, link->acl_link_policy & (~GAP_LINK_POLICY_SNIFF_MODE));
            }
            else
            {
                bt_pm_sm(link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_REQ);
            }
        }

        return true;
    }

    return false;
}

bool bt_sniff_mode_enter(uint8_t  bd_addr[6],
                         uint16_t min_interval,
                         uint16_t max_interval,
                         uint16_t sniff_attempt,
                         uint16_t sniff_timeout)
{
    T_BT_LINK *link;
    bool       ret = false; /* false if failed or pending */

    link = bt_link_find(bd_addr);
    if (link)
    {
        T_BT_LINK_PM_STATE   prev_pm_state = link->pm_state;
        T_BT_LINK_PM_ACTION  prev_pm_action = link->pm_action;

        switch (link->pm_state)
        {
        case BT_LINK_PM_STATE_ACTIVE:
            if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                /* Enter sniff mode if no action pending. */
                if (gap_br_enter_sniff_mode(link->bd_addr, min_interval, max_interval,
                                            sniff_attempt, sniff_timeout) == GAP_CAUSE_SUCCESS)
                {
                    link->pm_state = BT_LINK_PM_STATE_SNIFF_PENDING;

                    link->min_interval  = min_interval;
                    link->max_interval  = max_interval;
                    link->sniff_attempt = sniff_attempt;
                    link->sniff_timeout = sniff_timeout;
                }
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                /* Clear the duplicated sniff-enter pending action and enter sniff mode directly.
                * This case will not happen in reality.
                */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;

                if (gap_br_enter_sniff_mode(link->bd_addr, min_interval, max_interval,
                                            sniff_attempt, sniff_timeout) == GAP_CAUSE_SUCCESS)
                {
                    link->pm_state = BT_LINK_PM_STATE_SNIFF_PENDING;

                    link->min_interval  = min_interval;
                    link->max_interval  = max_interval;
                    link->sniff_attempt = sniff_attempt;
                    link->sniff_timeout = sniff_timeout;
                }
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                /* Override the pending sniff-exit action and enter sniff mode directly.
                * This case will not happen in reality.
                */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;

                if (gap_br_enter_sniff_mode(link->bd_addr, min_interval, max_interval,
                                            sniff_attempt, sniff_timeout) == GAP_CAUSE_SUCCESS)
                {
                    link->pm_state = BT_LINK_PM_STATE_SNIFF_PENDING;

                    link->min_interval  = min_interval;
                    link->max_interval  = max_interval;
                    link->sniff_attempt = sniff_attempt;
                    link->sniff_timeout = sniff_timeout;
                }
            }
            break;

        case BT_LINK_PM_STATE_SNIFF_PENDING:
            if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                /* Still sniff pending when re-entering sniff mode. Use previous sniff parameters. */
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                /* Clear the pending sniff-enter action. Use previous sniff parameters.
                * This case will not happen in reality.
                */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                /* Clear the sniff-exit pending action. Use previous sniff parameters. */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;
            }
            break;

        case BT_LINK_PM_STATE_SNIFF:
            if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                /* Nothing to do when re-entering sniff mode. Use previous sniff parameters. */
                ret = true;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                /* Clear the pending sniff-enter action. Use previous sniff parameters.
                * This case will not happen in reality.
                */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;
                ret = true;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                /* Clear the sniff-exit pending action. Use previous sniff parameters.
                * This case will not happen in reality.
                */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;
                ret = true;
            }
            break;

        case BT_LINK_PM_STATE_ACTIVE_PENDING:
            if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                link->pm_action = BT_LINK_PM_ACTION_SNIFF_ENTER;

                link->min_interval  = min_interval;
                link->max_interval  = max_interval;
                link->sniff_attempt = sniff_attempt;
                link->sniff_timeout = sniff_timeout;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                /* Still active pending when re-entering sniff mode. */
                link->min_interval  = min_interval;
                link->max_interval  = max_interval;
                link->sniff_attempt = sniff_attempt;
                link->sniff_timeout = sniff_timeout;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                /* Override the pending sniff-exit action. This case will not happen in reality. */
                link->pm_action = BT_LINK_PM_ACTION_SNIFF_ENTER;

                link->min_interval  = min_interval;
                link->max_interval  = max_interval;
                link->sniff_attempt = sniff_attempt;
                link->sniff_timeout = sniff_timeout;
            }
            break;
        }

        BTM_PRINT_TRACE6("bt_sniff_mode_enter: bd_addr %s, prev_pm_state %u, prev_pm_action %u, "
                         "pm_state %u, pm_action %u, ret %u",
                         TRACE_BDADDR(link->bd_addr), prev_pm_state, prev_pm_action,
                         link->pm_state, link->pm_action, ret);
    }

    return ret;
}

bool bt_sniff_mode_exit(uint8_t bd_addr[6],
                        bool    refresh)
{
    T_BT_LINK *link;
    bool       ret = false; /* false if failed or pending */

    link = bt_link_find(bd_addr);
    if (link)
    {
        T_BT_LINK_PM_STATE   prev_pm_state = link->pm_state;
        T_BT_LINK_PM_ACTION  prev_pm_action = link->pm_action;

        switch (link->pm_state)
        {
        case BT_LINK_PM_STATE_ACTIVE:
            if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                /* Nothing to do when re-exiting sniff mode. */
                ret = true;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                /* Clear the sniff-enter pending action. This case will not happen in reality. */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;
                ret = true;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                /* Clear the sniff-exit pending action. This case will not happen in reality. */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;
                ret = true;
            }

            if (refresh)
            {
                sys_timer_restart(link->timer_sniff, link->pm_timeout);
            }
            break;

        case BT_LINK_PM_STATE_SNIFF_PENDING:
            if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                link->pm_action = BT_LINK_PM_ACTION_SNIFF_EXIT;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                /* Override the sniff-enter pending action. This case will not happen in reality. */
                link->pm_action = BT_LINK_PM_ACTION_SNIFF_EXIT;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                /* Still sniff pending when re-exiting sniff mode. */
                link->pm_action = BT_LINK_PM_ACTION_SNIFF_EXIT;
            }
            break;

        case BT_LINK_PM_STATE_SNIFF:
            if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                /* Exit sniff mode if no action pending. */
                if (gap_br_exit_sniff_mode(link->bd_addr) == GAP_CAUSE_SUCCESS)
                {
                    link->pm_state = BT_LINK_PM_STATE_ACTIVE_PENDING;
                }
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                /* Override the pending sniff-enter action and exit sniff mode directly.
                * This case will not happen in reality.
                */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;

                if (gap_br_exit_sniff_mode(link->bd_addr) == GAP_CAUSE_SUCCESS)
                {
                    link->pm_state = BT_LINK_PM_STATE_ACTIVE_PENDING;
                }
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                /* Clear the duplicated sniff-exit pending action and exit sniff mode directly.
                * This case will not happen in reality.
                */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;

                if (gap_br_exit_sniff_mode(link->bd_addr) == GAP_CAUSE_SUCCESS)
                {
                    link->pm_state = BT_LINK_PM_STATE_ACTIVE_PENDING;
                }
            }
            break;

        case BT_LINK_PM_STATE_ACTIVE_PENDING:
            if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                /* Still active pending when re-exiting sniff mode. */
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                /* Clear the sniff-enter pending action. */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;
            }
            else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                /* Clear the pending sniff-enter action. This case will not happen in reality. */
                link->pm_action = BT_LINK_PM_ACTION_IDLE;
            }
            break;
        }

        if (prev_pm_state == BT_LINK_PM_STATE_SNIFF ||
            prev_pm_state == BT_LINK_PM_STATE_SNIFF_PENDING ||
            refresh == true)
        {
            BTM_PRINT_TRACE7("bt_sniff_mode_exit: bd_addr %s, prev_pm_state %u, prev_pm_action %u, "
                             "pm_state %u, pm_action %u, refresh %u, ret %u",
                             TRACE_BDADDR(link->bd_addr), prev_pm_state, prev_pm_action,
                             link->pm_state, link->pm_action, refresh, ret);
        }
    }

    return ret;
}

void bt_pm_sm(uint8_t        bd_addr[6],
              T_BT_PM_EVENT  event)
{
    T_BT_LINK *link;

    link = bt_link_find(bd_addr);
    if (link != NULL)
    {
        BTM_PRINT_TRACE2("bt_pm_sm: bd_addr %s, event 0x%02x", TRACE_BDADDR(bd_addr), event);

        switch (event)
        {
        case BT_PM_EVENT_LINK_CONNECTED:
            {
                link->pm_state = BT_LINK_PM_STATE_ACTIVE;

                sys_timer_start(link->timer_sniff);
            }
            break;

        case BT_PM_EVENT_LINK_DISCONNECTED:
            /* Link free will free related resources. */
            break;

        case BT_PM_EVENT_SNIFF_ENTER_SUCCESS:
            {
                T_BT_PM_CBACK_ITEM *item;

                sys_timer_stop(link->timer_sniff);

                link->pm_state = BT_LINK_PM_STATE_SNIFF;

                item = (T_BT_PM_CBACK_ITEM *)link->pm_cback_list.p_first;
                while (item != NULL)
                {
                    item->cback(link->bd_addr, BT_PM_EVENT_SNIFF_ENTER_SUCCESS);
                    item = item->p_next;
                }

                if (link->pm_enable == false)
                {
                    link->pm_action = BT_LINK_PM_ACTION_SNIFF_EXIT;
                }

                if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
                {
                    /* Nothing to do. */
                }
                else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
                {
                    link->pm_action = BT_LINK_PM_ACTION_IDLE;
                }
                else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
                {
                    link->pm_action = BT_LINK_PM_ACTION_IDLE;
                    bt_sniff_mode_exit(link->bd_addr, false);
                }
            }
            break;

        case BT_PM_EVENT_SNIFF_ENTER_FAIL:
            {
                T_BT_PM_CBACK_ITEM *item;

                if (link->pm_state == BT_LINK_PM_STATE_SNIFF_PENDING)
                {
                    /* Restart timer */
                    sys_timer_start(link->timer_sniff);

                    link->pm_state = BT_LINK_PM_STATE_ACTIVE;

                    item = (T_BT_PM_CBACK_ITEM *)link->pm_cback_list.p_first;
                    while (item != NULL)
                    {
                        item->cback(link->bd_addr, BT_PM_EVENT_SNIFF_ENTER_FAIL);
                        item = item->p_next;
                    }

                    if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
                    {
                        /* Nothing to do. */
                    }
                    else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
                    {
                        link->pm_action = BT_LINK_PM_ACTION_IDLE;
                        bt_sniff_mode_enter(link->bd_addr,
                                            link->min_interval,
                                            link->max_interval,
                                            link->sniff_attempt,
                                            link->sniff_timeout);
                    }
                    else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
                    {
                        link->pm_action = BT_LINK_PM_ACTION_IDLE;
                    }
                }
            }
            break;

        case BT_PM_EVENT_SNIFF_ENTER_REQ:
            {
                T_BT_PM_CBACK_ITEM *item;
                bool                ret = true;

                sys_timer_stop(link->timer_sniff);

                item = (T_BT_PM_CBACK_ITEM *)link->pm_cback_list.p_first;
                while (item != NULL)
                {
                    ret = item->cback(link->bd_addr, BT_PM_EVENT_SNIFF_ENTER_REQ);
                    if (ret == false)
                    {
                        /* Failed to request sniff-enter */
                        break;
                    }

                    item = item->p_next;
                }

                if (ret == true)
                {
                    ret = bt_sniff_mode_enter(link->bd_addr,
                                              link->min_interval,
                                              link->max_interval,
                                              link->sniff_attempt,
                                              link->sniff_timeout);
                }

                if (ret == false)
                {
                    sys_timer_start(link->timer_sniff);
                }
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_SUCCESS:
            {
                T_BT_PM_CBACK_ITEM *item;

                if (link->pm_enable == false)
                {
                    bt_link_policy_set(link->bd_addr, link->acl_link_policy & (~GAP_LINK_POLICY_SNIFF_MODE));
                }
                link->pm_state = BT_LINK_PM_STATE_ACTIVE;

                sys_timer_start(link->timer_sniff);

                item = (T_BT_PM_CBACK_ITEM *)link->pm_cback_list.p_first;
                while (item != NULL)
                {
                    item->cback(link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_SUCCESS);
                    item = item->p_next;
                }

                if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
                {
                    /* Nothing to do. */
                }
                else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
                {
                    link->pm_action = BT_LINK_PM_ACTION_IDLE;
                    bt_sniff_mode_enter(link->bd_addr,
                                        link->min_interval,
                                        link->max_interval,
                                        link->sniff_attempt,
                                        link->sniff_timeout);
                }
                else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
                {
                    link->pm_action = BT_LINK_PM_ACTION_IDLE;
                }
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_FAIL:
            {
                T_BT_PM_CBACK_ITEM *item;

                if (link->pm_state == BT_LINK_PM_STATE_ACTIVE_PENDING)
                {
                    sys_timer_stop(link->timer_sniff);

                    link->pm_state = BT_LINK_PM_STATE_SNIFF;

                    item = (T_BT_PM_CBACK_ITEM *)link->pm_cback_list.p_first;
                    while (item != NULL)
                    {
                        item->cback(link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_FAIL);
                        item = item->p_next;
                    }

                    if (link->pm_action == BT_LINK_PM_ACTION_IDLE)
                    {
                        /* Nothing to do. */
                    }
                    else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
                    {
                        link->pm_action = BT_LINK_PM_ACTION_IDLE;
                    }
                    else if (link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
                    {
                        link->pm_action = BT_LINK_PM_ACTION_IDLE;
                        bt_sniff_mode_exit(link->bd_addr, false);
                    }
                }
            }
            break;

        case BT_PM_EVENT_SNIFF_EXIT_REQ:
            {
                T_BT_PM_CBACK_ITEM *item;
                bool                ret = true;

                item = (T_BT_PM_CBACK_ITEM *)link->pm_cback_list.p_first;
                while (item != NULL)
                {
                    ret = item->cback(link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_REQ);
                    if (ret == false)
                    {
                        /* Failed to request sniff-exit */
                        break;
                    }

                    item = item->p_next;
                }

                if (ret == true)
                {
                    bt_sniff_mode_exit(link->bd_addr, false);
                }
            }
            break;
        }
    }
}
