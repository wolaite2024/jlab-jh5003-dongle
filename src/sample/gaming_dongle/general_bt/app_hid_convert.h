/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_HID_CONVERT_H_
#define _APP_HID_CONVERT_H_

#include "bt_hfp_ag.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define HIDS_MAX_REPORT_DATA_SIZE       8

typedef struct
{
    uint8_t report_id;
    uint8_t size;
    uint8_t data[HIDS_MAX_REPORT_DATA_SIZE];
} hids_report_t;
/** @defgroup APP_HFP_AG App HFP AG
  * @brief This file handle HFP AG profile related process
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
bool input_convert_from_hogp_to_usb(hids_report_t *p_hogp, hids_report_t *p_usb);

bool hogp_write_proc(uint8_t *data, uint8_t size);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_HID_CONVERT_H_ */
