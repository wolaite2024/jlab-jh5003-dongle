#ifndef __UAL_LIST_H__
#define __UAL_LIST_H__

#include <stdint.h>
#include "ual_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ualist_entry(ptr, type, member) \
    ((type *)((uint8_t *)(ptr) - (size_t)(&((type *)0)->member)))

#define ualist_first_entry(ptr, type, member) \
    ualist_entry((ptr)->next, type, member)

#define ualist_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

static inline void init_ualist_head(T_UALIST_HEAD *list)
{
    list->next = list;
    list->prev = list;
}

/* Add new item to the head */
static inline void ualist_add(T_UALIST_HEAD *new, T_UALIST_HEAD *head)
{
    T_UALIST_HEAD *prev = head;
    T_UALIST_HEAD *next = head->next;

    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/* Add new item to the tail */
static inline void ualist_add_tail(T_UALIST_HEAD *new, T_UALIST_HEAD *head)
{
    T_UALIST_HEAD *prev = head->prev;
    T_UALIST_HEAD *next = head;

    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void ualist_del(T_UALIST_HEAD *entry)
{
    T_UALIST_HEAD *prev = entry->prev;
    T_UALIST_HEAD *next = entry->next;

    next->prev = prev;
    prev->next = next;

    entry->next = entry;
    entry->prev = entry;
}

static inline int ualist_empty(const T_UALIST_HEAD *head)
{
    return head->next == head;
}

static inline int ualist_len(const T_UALIST_HEAD *head)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    int count = 0;

    ualist_for_each_safe(pos, n, head)
    {
        count++;
    }

    return count;
}

static inline void ualist_join(const T_UALIST_HEAD *list,
                               T_UALIST_HEAD *prev,
                               T_UALIST_HEAD *next)
{
    T_UALIST_HEAD *first = list->next;
    T_UALIST_HEAD *last = list->prev;

    first->prev = prev;
    prev->next = first;

    last->next = next;
    next->prev = last;
}

static inline void ualist_join_tail(const T_UALIST_HEAD *list,
                                    T_UALIST_HEAD *head)
{
    T_UALIST_HEAD *first;
    T_UALIST_HEAD *last;
    T_UALIST_HEAD *hprev;

    if (ualist_empty(list))
    {
        return;
    }

    hprev = head->prev;
    first = list->next;
    last  = list->prev;

    first->prev = hprev;
    hprev->next = first;
    last->next  = head;
    head->prev  = last;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UAL_LIST_H__ */
