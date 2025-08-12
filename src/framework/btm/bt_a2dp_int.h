/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_A2DP_INT_H_
#define _BT_A2DP_INT_H_

#include <stdint.h>
#include <stdbool.h>
#include "a2dp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct t_a2dp_link_data
{
    uint8_t           role;
    uint8_t           codec_type;
    bool              streaming_fg;
    uint8_t           a2dp_delay_report;
    uint8_t           a2dp_content_protect;
    uint16_t          last_seq_num;
    T_A2DP_CODEC_INFO codec_info;
} T_A2DP_LINK_DATA;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_A2DP_INT_H_ */
