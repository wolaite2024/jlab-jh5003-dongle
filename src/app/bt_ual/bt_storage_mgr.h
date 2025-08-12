/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __BT_STORAGE_MGR_H__
#define __BT_STORAGE_MGR_H__

#include <stdint.h>
#include "ual_types.h"
#include "gap_bond_le.h"
#include "gap_conn_le.h"
#include "ual_bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void bt_storage_load_devices(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UAL_DEV_MGR_H__ */
