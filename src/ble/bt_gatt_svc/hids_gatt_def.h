/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     hids_gatt_def.h
  * @brief    Definition for using Human Interface Device Service.
  * @details  HIDS data structs and external functions declaration developed based on bt_gatt_svc.h.
  * @author   Carrie
  * @date     2022-9-21
  * @version  v1.0
  * *************************************************************************************
  */

#ifndef _HIDS_GATT_DEF_H_
#define _HIDS_GATT_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include <stdint.h>

/** @defgroup HIDS_GATT_DEF Human Interface Device Service
  * @brief  Human Interface Device Service
  * @details
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup HIDS_GATT_DEF_Exported_Macros HIDS GATT Exported Macros
  * @brief
  * @{
  */

/** @defgroup HIDS_SVC_UUID
  * @brief HIDS service UUID
  * @{
  */
#define GATT_UUID_HIDS                          0x1812

#define GATT_UUID_CHAR_BOOT_KB_IN_REPORT        0x2A22
#define GATT_UUID_CHAR_BOOT_KB_OUT_REPORT       0x2A32
#define GATT_UUID_CHAR_BOOT_MS_IN_REPORT        0x2A33
#define GATT_UUID_CHAR_HID_INFO                 0x2A4A
#define GATT_UUID_CHAR_REPORT_MAP               0x2A4B
#define GATT_UUID_CHAR_HID_CONTROL_POINT        0x2A4C
#define GATT_UUID_CHAR_REPORT                   0x2A4D
#define GATT_UUID_CHAR_PROTOCOL_MODE            0x2A4E
/** @} */

/** End of HIDS_GATT_DEF_Exported_Macros
* @}
*/


/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup HIDS_GATT_DEF_Exported_Types HIDS GATT Exported Types
  * @brief
  * @{
  */

/**
 * @brief Human Interface Device Service report type
*/

typedef enum
{
    HID_INPUT_TYPE   = 1,
    HID_OUTPUT_TYPE  = 2,
    HID_FEATURE_TYPE = 3
} T_HIDS_REPORT_TYPE;

typedef struct
{
    uint16_t bcd_hid;
    uint8_t  b_country_code;
    uint8_t  flags;
} T_HIDS_INFO;

/**
 * @brief HIDS protocol mode
*/
typedef enum
{
    BOOT_PROTOCOL_MODE = 0,
    REPORT_PROCOCOL_MODE = 1
} T_HIDS_PROTOCOL_MODE;

typedef enum
{
    SUSPEND = 0x00,
    EXIT_SUSPEND = 0x01
} T_HIDS_CP_VALUE;

/** End of HIDS_GATT_DEF_Exported_Types
* @}
*/

/** @} End of HIDS_GATT_DEF */

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
