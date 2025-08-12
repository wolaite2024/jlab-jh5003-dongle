/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __INQUIRY_H__
#define __INQUIRY_H__
#include "ual_adapter.h"
#include "gap_callback_le.h"
#include "adapter_int.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void ual_handle_inquiry_rsp(T_BT_ADAPTER *adapter, uint16_t cause);
void ual_handle_inquiry_cancel_cmpl(T_BT_ADAPTER *adapter, uint16_t cause);
void ual_handle_inquiry_cmpl(T_BT_ADAPTER *adapter, uint16_t cause);
void ual_handle_inquiry_result(T_BT_ADAPTER *adapter,
                               T_BT_EVENT_PARAM_INQUIRY_RESULT inq_res);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
