/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_RDTP_INT_H_
#define _BT_RDTP_INT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool bt_rdtp_set_roleswap_info(uint8_t prev_bd_addr[6],
                               uint8_t curr_bd_addr[6]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_RDTP_INT_H_ */
