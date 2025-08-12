/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_GAP_H_
#define _BT_GAP_H_

#include "gap_br.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bt_gap_cback(void         *p_buf,
                  T_GAP_BR_MSG  br_msg);

void bt_gap_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_GAP_H_ */
