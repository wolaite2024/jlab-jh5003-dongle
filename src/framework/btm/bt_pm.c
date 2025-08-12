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
    T_BT_BR_LINK *p_link;

    p_link = (void *)sys_timer_id_get(handle);

    BTM_PRINT_TRACE2("bt_pm_timeout_cback: handle %p, p_link %p",
                     handle, p_link);

    if (p_link != NULL)
    {
        if (p_link->acl_link_state == BT_LINK_STATE_CONNECTED)
        {
            bt_pm_sm(p_link, BT_PM_EVENT_SNIFF_ENTER_REQ);
        }
    }
}

bool bt_pm_cback_register(uint8_t       bd_addr[6],
                          P_BT_PM_CBACK cback)
{
    T_BT_PM_CBACK_ITEM *item;
    T_BT_BR_LINK *p_link;

    if (cback != NULL)
    {
        p_link = bt_find_br_link(bd_addr);
        if (p_link)
        {
            item = (T_BT_PM_CBACK_ITEM *)p_link->pm_cback_list.p_first;
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
                os_queue_in(&p_link->pm_cback_list, item);
                return true;
            }
        }
    }

    return false;
}

bool bt_pm_cback_unregister(uint8_t       bd_addr[6],
                            P_BT_PM_CBACK cback)
{
    T_BT_PM_CBACK_ITEM *item;
    T_BT_BR_LINK *p_link;

    if (cback != NULL)
    {
        p_link = bt_find_br_link(bd_addr);
        if (p_link)
        {
            item = (T_BT_PM_CBACK_ITEM *)p_link->pm_cback_list.p_first;
            while (item != NULL)
            {
                if (item->cback == cback)
                {
                    os_queue_delete(&p_link->pm_cback_list, item);
                    os_mem_free(item);
                    return true;
                }

                item = item->p_next;
            }
        }
    }

    return false;
}

T_BT_LINK_PM_STATE bt_pm_state_get(T_BT_BR_LINK *p_link)
{
    return p_link->pm_state;
}

bool bt_sniff_mode_config(uint8_t  bd_addr[6],
                          uint16_t sniff_interval,
                          uint16_t sniff_attempt,
                          uint16_t sniff_timeout,
                          uint32_t pm_timeout)
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        p_link->pm_enable = true;
        p_link->pm_timeout = pm_timeout;
        p_link->max_interval = sniff_interval + BT_SNIFF_INTERVAL_OFFSET / 2;
        p_link->min_interval = sniff_interval - BT_SNIFF_INTERVAL_OFFSET / 2;
        p_link->sniff_attempt = sniff_attempt;
        p_link->sniff_timeout = sniff_timeout;
        p_link->timer_enter_sniff = sys_timer_create("bt_sniff_enter",
                                                     SYS_TIMER_TYPE_LOW_PRECISION,
                                                     (uint32_t)p_link,
                                                     pm_timeout,
                                                     false,
                                                     bt_pm_timeout_cback);
        os_queue_init(&p_link->pm_cback_list);

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
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        bool exit = false;

        if (p_link->pm_enable == false)
        {
            p_link->pm_enable = true;
            bt_link_policy_set(bd_addr, p_link->acl_link_policy | GAP_LINK_POLICY_SNIFF_MODE);
        }

        if (min_interval != 0 && max_interval != 0)
        {
            p_link->min_interval  = min_interval;
            p_link->max_interval  = max_interval;
            exit = true;
        }

        if (sniff_attempt != 0)
        {
            p_link->sniff_attempt = sniff_attempt;
            exit = true;
        }

        if (sniff_timeout != 0)
        {
            p_link->sniff_timeout = sniff_timeout;
            exit = true;
        }

        if (exit == true)
        {
            bt_pm_sm(p_link, BT_PM_EVENT_SNIFF_EXIT_REQ);
        }

        return true;
    }

    return false;
}

bool bt_sniff_mode_disable(uint8_t bd_addr[6])
{
    T_BT_BR_LINK *p_link;

    p_link = bt_find_br_link(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->pm_enable == true)
        {
            p_link->pm_enable = false;

            if (p_link->pm_state == BT_LINK_PM_STATE_ACTIVE)
            {
                bt_link_policy_set(bd_addr, p_link->acl_link_policy & (~GAP_LINK_POLICY_SNIFF_MODE));
            }
            else
            {
                bt_pm_sm(p_link, BT_PM_EVENT_SNIFF_EXIT_REQ);
            }
        }

        return true;
    }

    return false;
}

bool bt_sniff_mode_enter(T_BT_BR_LINK *p_link,
                         uint16_t      min_interval,
                         uint16_t      max_interval,
                         uint16_t      sniff_attempt,
                         uint16_t      sniff_timeout)
{
    T_BT_LINK_PM_STATE  prev_pm_state = p_link->pm_state;
    T_BT_LINK_PM_ACTION prev_pm_action = p_link->pm_action;
    bool                ret = false; /* false if failed or pending */

    switch (p_link->pm_state)
    {
    case BT_LINK_PM_STATE_ACTIVE:
        if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
        {
            /* Enter sniff mode if no action pending. */
            if (gap_br_enter_sniff_mode(p_link->bd_addr, min_interval, max_interval,
                                        sniff_attempt, sniff_timeout) == GAP_CAUSE_SUCCESS)
            {
                p_link->pm_state = BT_LINK_PM_STATE_SNIFF_PENDING;

                p_link->min_interval  = min_interval;
                p_link->max_interval  = max_interval;
                p_link->sniff_attempt = sniff_attempt;
                p_link->sniff_timeout = sniff_timeout;
            }
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
        {
            /* Clear the duplicated sniff-enter pending action and enter sniff mode directly.
             * This case will not happen in reality.
             */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;

            if (gap_br_enter_sniff_mode(p_link->bd_addr, min_interval, max_interval,
                                        sniff_attempt, sniff_timeout) == GAP_CAUSE_SUCCESS)
            {
                p_link->pm_state = BT_LINK_PM_STATE_SNIFF_PENDING;

                p_link->min_interval  = min_interval;
                p_link->max_interval  = max_interval;
                p_link->sniff_attempt = sniff_attempt;
                p_link->sniff_timeout = sniff_timeout;
            }
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
        {
            /* Override the pending sniff-exit action and enter sniff mode directly.
             * This case will not happen in reality.
             */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;

            if (gap_br_enter_sniff_mode(p_link->bd_addr, min_interval, max_interval,
                                        sniff_attempt, sniff_timeout) == GAP_CAUSE_SUCCESS)
            {
                p_link->pm_state = BT_LINK_PM_STATE_SNIFF_PENDING;

                p_link->min_interval  = min_interval;
                p_link->max_interval  = max_interval;
                p_link->sniff_attempt = sniff_attempt;
                p_link->sniff_timeout = sniff_timeout;
            }
        }
        break;

    case BT_LINK_PM_STATE_SNIFF_PENDING:
        if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
        {
            /* Still sniff pending when re-entering sniff mode. Use previous sniff parameters. */
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
        {
            /* Clear the pending sniff-enter action. Use previous sniff parameters.
             * This case will not happen in reality.
             */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
        {
            /* Clear the sniff-exit pending action. Use previous sniff parameters. */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
        }
        break;

    case BT_LINK_PM_STATE_SNIFF:
        if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
        {
            /* Nothing to do when re-entering sniff mode. Use previous sniff parameters. */
            ret = true;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
        {
            /* Clear the pending sniff-enter action. Use previous sniff parameters.
             * This case will not happen in reality.
             */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
            ret = true;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
        {
            /* Clear the sniff-exit pending action. Use previous sniff parameters.
             * This case will not happen in reality.
             */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
            ret = true;
        }
        break;

    case BT_LINK_PM_STATE_ACTIVE_PENDING:
        if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
        {
            p_link->pm_action = BT_LINK_PM_ACTION_SNIFF_ENTER;

            p_link->min_interval  = min_interval;
            p_link->max_interval  = max_interval;
            p_link->sniff_attempt = sniff_attempt;
            p_link->sniff_timeout = sniff_timeout;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
        {
            /* Still active pending when re-entering sniff mode. */
            p_link->min_interval  = min_interval;
            p_link->max_interval  = max_interval;
            p_link->sniff_attempt = sniff_attempt;
            p_link->sniff_timeout = sniff_timeout;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
        {
            /* Override the pending sniff-exit action. This case will not happen in reality. */
            p_link->pm_action = BT_LINK_PM_ACTION_SNIFF_ENTER;

            p_link->min_interval  = min_interval;
            p_link->max_interval  = max_interval;
            p_link->sniff_attempt = sniff_attempt;
            p_link->sniff_timeout = sniff_timeout;
        }
        break;
    }

    BTM_PRINT_TRACE6("bt_sniff_mode_enter: bd_addr %s, prev_pm_state %u, prev_pm_action %u, "
                     "pm_state %u, pm_action %u, ret %u",
                     TRACE_BDADDR(p_link->bd_addr), prev_pm_state, prev_pm_action,
                     p_link->pm_state, p_link->pm_action, ret);

    return ret;
}

bool bt_sniff_mode_exit(T_BT_BR_LINK *p_link,
                        bool          refresh)
{
    T_BT_LINK_PM_STATE  prev_pm_state = p_link->pm_state;
    T_BT_LINK_PM_ACTION prev_pm_action = p_link->pm_action;
    bool                ret = false; /* false if failed or pending */

    switch (p_link->pm_state)
    {
    case BT_LINK_PM_STATE_ACTIVE:
        if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
        {
            /* Nothing to do when re-exiting sniff mode. */
            ret = true;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
        {
            /* Clear the sniff-enter pending action. This case will not happen in reality. */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
            ret = true;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
        {
            /* Clear the sniff-exit pending action. This case will not happen in reality. */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
            ret = true;
        }

        if (refresh)
        {
            sys_timer_restart(p_link->timer_enter_sniff, p_link->pm_timeout);
        }
        break;

    case BT_LINK_PM_STATE_SNIFF_PENDING:
        if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
        {
            p_link->pm_action = BT_LINK_PM_ACTION_SNIFF_EXIT;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
        {
            /* Override the sniff-enter pending action. This case will not happen in reality. */
            p_link->pm_action = BT_LINK_PM_ACTION_SNIFF_EXIT;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
        {
            /* Still sniff pending when re-exiting sniff mode. */
            p_link->pm_action = BT_LINK_PM_ACTION_SNIFF_EXIT;
        }
        break;

    case BT_LINK_PM_STATE_SNIFF:
        if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
        {
            /* Exit sniff mode if no action pending. */
            if (gap_br_exit_sniff_mode(p_link->bd_addr) == GAP_CAUSE_SUCCESS)
            {
                p_link->pm_state = BT_LINK_PM_STATE_ACTIVE_PENDING;
            }
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
        {
            /* Override the pending sniff-enter action and exit sniff mode directly.
             * This case will not happen in reality.
             */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;

            if (gap_br_exit_sniff_mode(p_link->bd_addr) == GAP_CAUSE_SUCCESS)
            {
                p_link->pm_state = BT_LINK_PM_STATE_ACTIVE_PENDING;
            }
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
        {
            /* Clear the duplicated sniff-exit pending action and exit sniff mode directly.
             * This case will not happen in reality.
             */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;

            if (gap_br_exit_sniff_mode(p_link->bd_addr) == GAP_CAUSE_SUCCESS)
            {
                p_link->pm_state = BT_LINK_PM_STATE_ACTIVE_PENDING;
            }
        }
        break;

    case BT_LINK_PM_STATE_ACTIVE_PENDING:
        if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
        {
            /* Still active pending when re-exiting sniff mode. */
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
        {
            /* Clear the sniff-enter pending action. */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
        }
        else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
        {
            /* Clear the pending sniff-enter action. This case will not happen in reality. */
            p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
        }
        break;
    }

    if (prev_pm_state == BT_LINK_PM_STATE_SNIFF ||
        prev_pm_state == BT_LINK_PM_STATE_SNIFF_PENDING ||
        refresh == true)
    {
        BTM_PRINT_TRACE7("bt_sniff_mode_exit: bd_addr %s, prev_pm_state %u, prev_pm_action %u, "
                         "pm_state %u, pm_action %u, refresh %u, ret %u",
                         TRACE_BDADDR(p_link->bd_addr), prev_pm_state, prev_pm_action,
                         p_link->pm_state, p_link->pm_action, refresh, ret);
    }

    return ret;
}

void bt_pm_sm(T_BT_BR_LINK  *p_link,
              T_BT_PM_EVENT  event)
{
    BTM_PRINT_TRACE2("bt_pm_sm: link %p, event 0x%02x", p_link, event);

    switch (event)
    {
    case BT_PM_EVENT_LINK_CONNECTED:
        {
            p_link->pm_state = BT_LINK_PM_STATE_ACTIVE;

            sys_timer_start(p_link->timer_enter_sniff);
        }
        break;

    case BT_PM_EVENT_LINK_DISCONNECTED:
        /* Link free will free related resources. */
        break;

    case BT_PM_EVENT_SNIFF_ENTER_SUCCESS:
        {
            T_BT_PM_CBACK_ITEM *item;

            sys_timer_stop(p_link->timer_enter_sniff);

            p_link->pm_state = BT_LINK_PM_STATE_SNIFF;

            item = (T_BT_PM_CBACK_ITEM *)p_link->pm_cback_list.p_first;
            while (item != NULL)
            {
                item->cback(p_link->bd_addr, BT_PM_EVENT_SNIFF_ENTER_SUCCESS);
                item = item->p_next;
            }

            if (p_link->pm_enable == false)
            {
                p_link->pm_action = BT_LINK_PM_ACTION_SNIFF_EXIT;
            }

            if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                /* Nothing to do. */
            }
            else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
            }
            else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
                bt_sniff_mode_exit(p_link, false);
            }
        }
        break;

    case BT_PM_EVENT_SNIFF_ENTER_FAIL:
        {
            T_BT_PM_CBACK_ITEM *item;

            if (p_link->pm_state == BT_LINK_PM_STATE_SNIFF_PENDING)
            {
                /* Restart timer */
                sys_timer_start(p_link->timer_enter_sniff);

                p_link->pm_state = BT_LINK_PM_STATE_ACTIVE;

                item = (T_BT_PM_CBACK_ITEM *)p_link->pm_cback_list.p_first;
                while (item != NULL)
                {
                    item->cback(p_link->bd_addr, BT_PM_EVENT_SNIFF_ENTER_FAIL);
                    item = item->p_next;
                }

                if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
                {
                    /* Nothing to do. */
                }
                else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
                {
                    p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
                    bt_sniff_mode_enter(p_link,
                                        p_link->min_interval,
                                        p_link->max_interval,
                                        p_link->sniff_attempt,
                                        p_link->sniff_timeout);
                }
                else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
                {
                    p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
                }
            }
        }
        break;

    case BT_PM_EVENT_SNIFF_ENTER_REQ:
        {
            T_BT_PM_CBACK_ITEM *item;
            bool                ret = true;

            sys_timer_stop(p_link->timer_enter_sniff);

            item = (T_BT_PM_CBACK_ITEM *)p_link->pm_cback_list.p_first;
            while (item != NULL)
            {
                ret = item->cback(p_link->bd_addr, BT_PM_EVENT_SNIFF_ENTER_REQ);
                if (ret == false)
                {
                    /* Failed to request sniff-enter */
                    break;
                }

                item = item->p_next;
            }

            if (ret == true)
            {
                ret = bt_sniff_mode_enter(p_link,
                                          p_link->min_interval,
                                          p_link->max_interval,
                                          p_link->sniff_attempt,
                                          p_link->sniff_timeout);
            }

            if (ret == false)
            {
                sys_timer_start(p_link->timer_enter_sniff);
            }
        }
        break;

    case BT_PM_EVENT_SNIFF_EXIT_SUCCESS:
        {
            T_BT_PM_CBACK_ITEM *item;

            if (p_link->pm_enable == false)
            {
                bt_link_policy_set(p_link->bd_addr, p_link->acl_link_policy & (~GAP_LINK_POLICY_SNIFF_MODE));
            }
            p_link->pm_state = BT_LINK_PM_STATE_ACTIVE;

            sys_timer_start(p_link->timer_enter_sniff);

            item = (T_BT_PM_CBACK_ITEM *)p_link->pm_cback_list.p_first;
            while (item != NULL)
            {
                item->cback(p_link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_SUCCESS);
                item = item->p_next;
            }

            if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
            {
                /* Nothing to do. */
            }
            else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
            {
                p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
                bt_sniff_mode_enter(p_link,
                                    p_link->min_interval,
                                    p_link->max_interval,
                                    p_link->sniff_attempt,
                                    p_link->sniff_timeout);
            }
            else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
            {
                p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
            }
        }
        break;

    case BT_PM_EVENT_SNIFF_EXIT_FAIL:
        {
            T_BT_PM_CBACK_ITEM *item;

            if (p_link->pm_state == BT_LINK_PM_STATE_ACTIVE_PENDING)
            {
                sys_timer_stop(p_link->timer_enter_sniff);

                p_link->pm_state = BT_LINK_PM_STATE_SNIFF;

                item = (T_BT_PM_CBACK_ITEM *)p_link->pm_cback_list.p_first;
                while (item != NULL)
                {
                    item->cback(p_link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_FAIL);
                    item = item->p_next;
                }

                if (p_link->pm_action == BT_LINK_PM_ACTION_IDLE)
                {
                    /* Nothing to do. */
                }
                else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_ENTER)
                {
                    p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
                }
                else if (p_link->pm_action == BT_LINK_PM_ACTION_SNIFF_EXIT)
                {
                    p_link->pm_action = BT_LINK_PM_ACTION_IDLE;
                    bt_sniff_mode_exit(p_link, false);
                }
            }
        }
        break;

    case BT_PM_EVENT_SNIFF_EXIT_REQ:
        {
            T_BT_PM_CBACK_ITEM *item;
            bool                ret = true;

            item = (T_BT_PM_CBACK_ITEM *)p_link->pm_cback_list.p_first;
            while (item != NULL)
            {
                ret = item->cback(p_link->bd_addr, BT_PM_EVENT_SNIFF_EXIT_REQ);
                if (ret == false)
                {
                    /* Failed to request sniff-exit */
                    break;
                }

                item = item->p_next;
            }

            if (ret == true)
            {
                bt_sniff_mode_exit(p_link, false);
            }
        }
        break;
    }
}
