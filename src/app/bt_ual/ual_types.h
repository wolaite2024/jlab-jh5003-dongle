/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __UAL_TYPES_H__
#define __UAL_TYPES_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct ualist_head
{
    struct ualist_head *next;
    struct ualist_head *prev;
} T_UALIST_HEAD;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UAL_TYPES_H__ */
