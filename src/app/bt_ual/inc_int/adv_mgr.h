/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __ADV_MGR_H__
#define __ADV_MGR_H__
#include "ual_adapter.h"
#include "gap_callback_le.h"
#include "adapter_int.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool adv_suspend_req(void);
void adv_resume_req(void);
void ble_adv_mgr_init(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
