/**
 * @copyright Copyright (C) 2023 Realtek Semiconductor Corporation.
 *
 * @file usb_msc.h
 * @version 1.0
 * @brief
 *
 * @note:
 */
#ifndef __USB_MSC_BOT_H__
#define __USB_MSC_BOT_H__
#include <stdint.h>

#define USB_MS_REQ_GET_MAX_LUN          (0xFE)
#define USB_MS_REQ_BO_RESET             (0xFF)

#pragma pack(push,1)

typedef struct _t_cbw
{
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN: 4;
    uint8_t rsv0: 4;
    uint8_t bCBWCBLength: 5;
    uint8_t rsv1: 3;
    uint8_t CBWCB[16];
} T_CBW;

typedef struct _t_csw
{
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
} T_CSW;

#pragma pack(pop)

typedef enum
{
    CB_STATUS_OK,
    CB_STATUS_FAILED,
    CB_STATUS_PHASE_ERROR,
} T_CB_STATUS;
#endif
