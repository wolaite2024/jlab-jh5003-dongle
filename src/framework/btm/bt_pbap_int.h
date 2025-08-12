/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_PBAP_INT_H_
#define _BT_PBAP_INT_H_

#include <stdint.h>
#include "bt_pbap.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct
{
    uint8_t path;
    T_BT_PBAP_REPOSITORY repos;
} T_PBAP_LINK_DATA;

typedef struct
{
    uint8_t *name_ptr;
    uint16_t name_len;
} T_BT_PBAP_CALLER_ID_NAME;

typedef struct
{
    bool           result;
    T_BT_PBAP_PATH path;
} T_BT_PBAP_SET_PHONE_BOOK_CMPL;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_PBAP_INT_H_ */
