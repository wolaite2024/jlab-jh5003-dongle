/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "stdint.h"
#include "stdbool.h"

#include "trace.h"
#include "os_mem.h"
#include "sys_event.h"

typedef struct t_sys_event_group
{
    void *p_next;
    T_OS_QUEUE  wait_list;
    uint32_t flags;
} T_SYS_EVENT_GROUP;

typedef struct t_sys_event_node
{
    void                     *next;
    void                     *owner;
    void                     *group;
    P_SYS_EVENT_NODE_CBACK  cback;
    uint32_t                 flags;
    T_SYS_EVENT_FLAG_TYPE    type;
    bool                     consume;
} T_SYS_EVENT_NODE;

typedef struct t_sys_event_db
{
    T_OS_QUEUE group_queue;
} T_SYS_EVENT_DB;

static T_SYS_EVENT_DB *sys_event_db = NULL;

bool sys_event_init(void)
{
    sys_event_db = os_mem_zalloc2(sizeof(T_SYS_EVENT_DB));
    if (sys_event_db != NULL)
    {
        os_queue_init(&sys_event_db->group_queue);
        return true;
    }

    return false;
}

void sys_event_deinit(void)
{
    if (sys_event_db != NULL)
    {
        os_queue_init(&sys_event_db->group_queue);
        os_mem_free(sys_event_db);
        sys_event_db = NULL;
    }
}

T_SYS_EVENT_GROUP_HANDLE sys_event_group_create(uint32_t flags)
{
    T_SYS_EVENT_GROUP *pgrp;

    pgrp = os_mem_alloc2(sizeof(T_SYS_EVENT_GROUP));
    if (pgrp != NULL)
    {
        pgrp->flags = flags;
        os_queue_init(&pgrp->wait_list);
        os_queue_in(&sys_event_db->group_queue, pgrp);
    }

    return (T_SYS_EVENT_GROUP_HANDLE)pgrp;
}

bool sys_event_group_delete(T_SYS_EVENT_GROUP_HANDLE handle)
{
    T_SYS_EVENT_NODE *pnode;
    T_SYS_EVENT_GROUP  *pgrp;

    pgrp = (T_SYS_EVENT_GROUP *)handle;
    if (pgrp == NULL)
    {
        return false;
    }

    pnode = (T_SYS_EVENT_NODE *)os_queue_peek(&pgrp->wait_list, 0);
    while (pnode != NULL)
    {
        T_SYS_EVENT_NODE *pnode_next = (T_SYS_EVENT_NODE *)pnode->next;
        sys_event_flag_abort((T_SYS_EVENT_NODE_HANDLE)pnode);
        pnode = pnode_next;
    }

    os_queue_delete(&sys_event_db->group_queue, pgrp);
    os_mem_free(pgrp);

    return true;
}

T_SYS_EVENT_NODE_HANDLE sys_event_flag_wait(T_SYS_EVENT_GROUP_HANDLE handle,
                                            P_SYS_EVENT_NODE_CBACK  cback,
                                            void                     *owner,
                                            uint32_t                 flags,
                                            T_SYS_EVENT_FLAG_TYPE    type)
{
    T_SYS_EVENT_GROUP *pgrp = (T_SYS_EVENT_GROUP *)handle;
    if (pgrp == NULL)
    {
        return NULL;
    }

    T_SYS_EVENT_NODE *pnode;
    pnode = os_mem_zalloc2(sizeof(T_SYS_EVENT_NODE));
    if (pnode == NULL)
    {
        return NULL;
    }

    pnode->owner      = owner;
    pnode->flags      = flags;
    pnode->type       = type;
    pnode->consume    = false;
    pnode->cback      = cback;
    pnode->group      = handle;
    os_queue_in(&pgrp->wait_list, pnode);

    SYS_PRINT_TRACE4("sys_event_flag_wait:group %p, pnode = %p, flags = %d, owner %p",
                     pgrp, pnode, pnode->flags, pnode->owner);

    return pnode;
}

bool sys_event_flag_abort(T_SYS_EVENT_NODE_HANDLE handle)
{
    if (handle == NULL)
    {
        return false;
    }

    T_SYS_EVENT_NODE *pnode = (T_SYS_EVENT_NODE *)handle;
    if (pnode->group != NULL)
    {
        T_SYS_EVENT_GROUP *pgrp = (T_SYS_EVENT_GROUP *)pnode->group;
        os_queue_delete(&pgrp->wait_list, pnode);
    }
    os_mem_free(pnode);

    return true;
}

uint32_t sys_event_flag_post(T_SYS_EVENT_GROUP_HANDLE handle,
                             T_SYS_EVENT_FLAG_OPT     opt,
                             uint32_t              flags)
{
    T_SYS_EVENT_NODE *pnode;
    T_SYS_EVENT_NODE *pnode_next;
    uint32_t      flags_cur;
    uint32_t      flags_rdy;
    T_SYS_EVENT_GROUP *pgrp;

    pgrp = (T_SYS_EVENT_GROUP *)handle;
    if (pgrp == NULL)
    {
        return ((uint32_t)0);
    }

    switch (opt)
    {
    case SYS_EVENT_FLAG_OPT_CLEAR:
        pgrp->flags &= (uint32_t)~flags;
        break;

    case SYS_EVENT_FLAG_OPT_SET:
        pgrp->flags |= flags;
        break;

    default:
        return ((uint32_t)0);
    }

    SYS_PRINT_TRACE3("sys_event_flag_post: group %p, opt = %d, flags = %d",
                     pgrp, opt, flags);

    pnode = (T_SYS_EVENT_NODE *)os_queue_peek(&pgrp->wait_list, 0);
    while (pnode != NULL)
    {
        pnode_next = (T_SYS_EVENT_NODE *)pnode->next;
        switch (pnode->type)
        {
        case SYS_EVENT_FLAG_TYPE_SET_AND:
            {
                flags_rdy = (uint32_t)(pgrp->flags & pnode->flags);

                if ((flags_rdy == pnode->flags) && ((flags & pnode->flags) != 0))
                {
                    if (pnode->consume == true)
                    {
                        pgrp->flags &= (uint32_t)~flags_rdy;
                    }
                    pnode->cback(pnode->owner);
                    sys_event_flag_abort((T_SYS_EVENT_NODE_HANDLE)pnode);
                }
            }
            break;

        case SYS_EVENT_FLAG_TYPE_SET_OR:
            {
                flags_rdy = (uint32_t)(pgrp->flags & pnode->flags);
                if ((flags_rdy != (uint32_t)0) && ((flags & pnode->flags) != 0))
                {
                    if (pnode->consume == true)
                    {
                        pgrp->flags &= (uint32_t)~flags_rdy;
                    }
                    pnode->cback(pnode->owner);
                    sys_event_flag_abort((T_SYS_EVENT_NODE_HANDLE)pnode);
                }
            }
            break;

        case SYS_EVENT_FLAG_TYPE_CLEAR_AND:
            {
                flags_rdy = (uint32_t)~pgrp->flags & pnode->flags;
                if ((flags_rdy == pnode->flags) && ((flags & pnode->flags) != 0))
                {
                    if (pnode->consume == true)
                    {
                        pgrp->flags |= flags_rdy;
                    }
                    pnode->cback(pnode->owner);
                    sys_event_flag_abort((T_SYS_EVENT_NODE_HANDLE)pnode);
                }
            }
            break;

        case SYS_EVENT_FLAG_TYPE_CLEAR_OR:
            {
                flags_rdy = (uint32_t)~pgrp->flags & pnode->flags;
                if ((flags_rdy != (uint32_t)0) && ((flags & pnode->flags) != 0))
                {
                    if (pnode->consume == true)
                    {
                        pgrp->flags |= flags_rdy;
                    }
                    pnode->cback(pnode->owner);
                    sys_event_flag_abort((T_SYS_EVENT_NODE_HANDLE)pnode);
                }
            }
            break;

        default:
            return ((uint32_t)0);
        }
        pnode = pnode_next;
    }

    flags_cur = pgrp->flags;
    return (flags_cur);
}

bool sys_event_flag_check(T_SYS_EVENT_GROUP_HANDLE handle, uint32_t flags)
{
    uint32_t      flags_check;
    T_SYS_EVENT_GROUP *pgrp;

    pgrp = (T_SYS_EVENT_GROUP *)handle;
    if (pgrp == NULL)
    {
        return false;
    }

    flags_check = pgrp->flags;
    flags_check &= flags;

    if (flags_check != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
