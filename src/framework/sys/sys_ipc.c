/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "os_queue.h"
#include "os_mem.h"
#include "trace.h"
#include "sys_ipc.h"

#define TOPIC_CHANN_NUM     8

typedef struct t_sys_ipc_topic_item
{
    struct t_sys_ipc_topic_item *p_next;
    P_SYS_IPC_CBACK              cback;
} T_SYS_IPC_TOPIC_ITEM;

typedef struct
{
    const char *topic;
    T_OS_QUEUE  subscriber_list;
    bool        used;
} T_SYS_IPC_TOPIC_CHANN;

typedef struct
{
    T_SYS_IPC_TOPIC_CHANN topic_chann[TOPIC_CHANN_NUM];
} T_SYS_IPC;

static T_SYS_IPC sys_ipc;

bool sys_ipc_init(void)
{
    memset(&sys_ipc, 0, sizeof(sys_ipc));

    return true;
}

bool sys_ipc_publish(const char *topic, uint32_t id, void *msg)
{
    T_SYS_IPC_TOPIC_ITEM *item;
    uint8_t i;

    for (i = 0; i < TOPIC_CHANN_NUM; i++)
    {
        if (sys_ipc.topic_chann[i].used == true &&
            !strcmp(sys_ipc.topic_chann[i].topic, topic))
        {
            item = os_queue_peek(&sys_ipc.topic_chann[i].subscriber_list, 0);
            while (item != NULL)
            {
                if (item->cback != NULL)
                {
                    item->cback(id, msg);
                }

                item = item->p_next;
            }

            break;
        }
    }

    return true;
}

T_SYS_IPC_HANDLE sys_ipc_subscribe(const char *topic, P_SYS_IPC_CBACK cback)
{
    T_SYS_IPC_TOPIC_ITEM *item;
    uint8_t i;

    for (i = 0; i < TOPIC_CHANN_NUM; i++)
    {
        if (sys_ipc.topic_chann[i].used == true &&
            !strcmp(sys_ipc.topic_chann[i].topic, topic))
        {
            item = os_mem_alloc2(sizeof(T_SYS_IPC_TOPIC_ITEM));
            if (item != NULL)
            {
                item->cback = cback;
                os_queue_in(&sys_ipc.topic_chann[i].subscriber_list, item);
                return (T_SYS_IPC_HANDLE)item;
            }
            else
            {
                return NULL;
            }
        }
    }

    for (i = 0; i < TOPIC_CHANN_NUM; i++)
    {
        if (sys_ipc.topic_chann[i].used == false)
        {
            item = os_mem_alloc2(sizeof(T_SYS_IPC_TOPIC_ITEM));
            if (item != NULL)
            {
                sys_ipc.topic_chann[i].used = true;
                sys_ipc.topic_chann[i].topic = topic;

                item->cback = cback;
                os_queue_in(&sys_ipc.topic_chann[i].subscriber_list, item);
                return (T_SYS_IPC_HANDLE)item;
            }
            else
            {
                return NULL;
            }
        }
    }

    return NULL;
}

bool sys_ipc_unsubscribe(T_SYS_IPC_HANDLE handle)
{
    T_SYS_IPC_TOPIC_ITEM *item;
    uint8_t i;

    item = (T_SYS_IPC_TOPIC_ITEM *)handle;

    for (i = 0; i < TOPIC_CHANN_NUM; i++)
    {
        if (sys_ipc.topic_chann[i].used == true)
        {
            if (os_queue_delete(&sys_ipc.topic_chann[i].subscriber_list, item) == true)
            {
                os_mem_free(item);

                if (sys_ipc.topic_chann[i].subscriber_list.count == 0)
                {
                    sys_ipc.topic_chann[i].used = false;
                    sys_ipc.topic_chann[i].topic = NULL;
                }

                return true;
            }
        }
    }

    return false;
}
