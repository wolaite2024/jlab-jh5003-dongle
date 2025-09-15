/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     hids_gatt_svc.h
  * @brief    Head file for using Human Interface Device Service.
  * @details  HIDS data structs and external functions declaration developed based on bt_gatt_svc.h.
  * @author   Carrie
  * @date     2022-9-21
  * @version  v1.0
  * *************************************************************************************
  */

#ifndef _HIDS_GATT_SVC_H_
#define _HIDS_GATT_SVC_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "bt_gatt_svc.h"
#include "hids_gatt_def.h"

/** @defgroup HIDS_GATT_SVC Human Interface Device GATT Service
  * @brief  Human Interface Device Service
   * @details

    The HID Service exposes data and associated formatting for HID Devices and HID Hosts.

  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup HIDS_GATT_Exported_Macros HIDS GATT Exported Macros
  * @brief
  * @{
  */

#define HIDS_CHAR_BOOT_KB_REPORT_SUPPORT              1
#define HIDS_CHAR_BOOT_MS_REPORT_SUPPORT              1
#define HIDS_CHAR_INPUT_REPORT_NUM                    2 //0~2
#define HIDS_CHAR_OUTPUT_REPORT_NUM                   2 //0~2
#define HIDS_CHAR_FEATURE_REPORT_NUM                  2 //0~2

#define HIDS_CHAR_BOOT_KB_INPUT_REPORT_WRITE_SUPPORT  1
#define HIDS_CHAR_INPUT_REPORT_WRITE_SUPPORT          1

/** @defgroup HIDS_SVC_CB_MSG
  * @brief HIDS server callback messages
  * @{
  */
#define GATT_MSG_HIDS_SERVER_READ_IND           0x00
#define GATT_MSG_HIDS_SERVER_WRITE_IND          0x01
#define GATT_MSG_HIDS_SERVER_CCCD_UPDATE        0x02
/** @} End of HIDS_SVC_CB_MSG  */

/** End of HIDS_GATT_Exported_Macros
* @}
*/


/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup HIDS_GATT_Exported_Types HIDS GATT Exported Types
  * @brief
  * @{
  */

typedef struct
{
    uint8_t report_id;
    T_HIDS_REPORT_TYPE report_type;
} T_HIDS_REPORT_REFERENCE;

typedef struct
{
    T_HIDS_REPORT_TYPE report_type;
    uint8_t report_index;
} T_HIDS_REPORT_INFO;

typedef struct
{
    uint16_t value_len;
    uint8_t *p_value;
} T_HIDS_CHAR_KB_MS_REPORT;

typedef struct
{
    uint16_t report_map_len;
    uint8_t *p_report_map;
} T_HIDS_CHAR_REPORT_MAP;

typedef struct
{
    uint8_t report_index;
    T_HIDS_REPORT_REFERENCE report;
} T_HIDS_CHAR_REPORT_REF;

typedef struct
{
    uint16_t char_uuid;
    union
    {
        T_HIDS_PROTOCOL_MODE protocol_mode;
        T_HIDS_INFO hid_info;
        T_HIDS_CHAR_REPORT_REF report_ref;
        uint16_t hid_external_report_ref;    //EXTERNAL_REPORT_REFERENCE
        T_HIDS_CHAR_KB_MS_REPORT kb_ms_report;
        T_HIDS_CHAR_REPORT_MAP report_map;
    } data;
} T_HIDS_CHAR_VALUE;

typedef struct
{
    T_HIDS_REPORT_TYPE report_type;
    uint8_t report_index;
    uint16_t value_len;
    uint8_t *p_value;
} T_HIDS_CHAR_REPORT;

/** @brief HIDS server read characteristic data indication
 *         The message data for GATT_MSG_HIDS_SERVER_READ_IND.
*/
typedef struct
{
    T_SERVER_ID service_id;
    uint16_t offset;
    uint16_t char_uuid;
    union
    {
        T_HIDS_REPORT_INFO report_info;
    } data;
} T_HIDS_SERVER_READ_IND;

/** @brief HIDS server write data indication
 *         The message data for GATT_MSG_HIDS_SERVER_WRITE_IND.
*/
typedef struct
{
    T_SERVER_ID service_id;
    uint16_t char_uuid;
    T_WRITE_TYPE write_type;
    union
    {
        T_HIDS_PROTOCOL_MODE protocol_mode;
        T_HIDS_CP_VALUE hid_cp;
        T_HIDS_CHAR_KB_MS_REPORT kb_ms_report;
        T_HIDS_CHAR_REPORT report;
    } data;
} T_HIDS_SERVER_WRITE_IND;

/** @brief HIDS server cccd data update info
 *         The message data for GATT_MSG_HIDS_SERVER_CCCD_UPDATE.
*/
typedef struct
{
    T_SERVER_ID service_id;
    uint16_t char_uuid;
    uint16_t cccd_cfg;
    union
    {
        T_HIDS_REPORT_INFO report_info;
    } data;
} T_HIDS_SERVER_CCCD_UPDATE;

/** End of HIDS_GATT_Exported_Types
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup HIDS_GATT_Exported_Functions HIDS GATT Exported Functions
  * @brief
  * @{
  */
/**
 * @brief P_FUN_HIDS_SERVER_APP_CB HIDS Server Callback Function Point Definition Function
 *        pointer used in HIDS server module, to send events to specific server module.
 *        The events @ref HIDS_SVC_CB_MSG.
 */
typedef T_APP_RESULT(*P_FUN_HIDS_SERVER_APP_CB)(uint16_t conn_handle, uint16_t cid, uint8_t type,
                                                void *p_data);

/**
 * @brief       Add HID service to the BLE stack database.
 *
 *
 * @param[in]   app_cb  Callback when service attribute was read, write or cccd update.
 * @return Service id generated by the BLE stack: @ref T_SERVER_ID.
 * @retval 0xFF Operation failure.
 * @retval Others Service id assigned by stack.
 *
 * <b>Example usage</b>
 * \code{.c}
    void profile_init(void)
    {
        gatt_svc_init(mode, svc_num);
        hids_id = hids_reg_srv(app_hids_service_callback);
    }
 * \endcode
 */
T_SERVER_ID hids_reg_srv(P_FUN_HIDS_SERVER_APP_CB app_cb);

/**
 * @brief       Confirm for read charactereistic value request.
 *
 *
 * @param[in]   conn_handle   Connection handle of the ACL link.
 * @param[in]   cid           Local CID assigned by Bluetooth stack.
 * @param[in]   service_id    Service id.
 * @param[in]   offset        Offset.
 * @param[in]   char_value    Charactereistic value: @ref T_HIDS_CHAR_VALUE.
 * @param[in]   cause         Cause: @ref T_APP_RESULT.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 */
bool hids_read_char_confirm(uint8_t conn_handle, uint16_t cid, uint8_t service_id,
                            uint16_t offset, T_HIDS_CHAR_VALUE char_value, T_APP_RESULT cause);
/**
 * @brief       Confirm for read report value request.
 *
 *
 * @param[in]   conn_handle   Connection handle of the ACL link.
 * @param[in]   cid           Local CID assigned by Bluetooth stack.
 * @param[in]   service_id    Service id.
 * @param[in]   offset        Offset.
 * @param[in]   report_data   Report related data: @ref T_HIDS_CHAR_REPORT.
 * @param[in]   cause         Cause: @ref T_APP_RESULT.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 */
bool hids_read_report_confirm(uint8_t conn_handle, uint16_t cid, uint8_t service_id,
                              uint16_t offset, T_HIDS_CHAR_REPORT report_data, T_APP_RESULT cause);

/**
 * @brief       Send HIDS input report notification data.
 *
 *
 * @param[in]   conn_handle   Connection handle of the ACL link.
 * @param[in]   service_id    Service id.
 * @param[in]   report_data   Report related data.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 */
bool hids_send_input_report_notify(uint8_t conn_handle, uint8_t service_id,
                                   T_HIDS_CHAR_REPORT report_data);

#if HIDS_CHAR_BOOT_KB_REPORT_SUPPORT
/**
 * @brief       Send HIDS boot keyboard input report notification data.
 *
 *
 * @param[in]   conn_handle   Connection handle of the ACL link.
 * @param[in]   service_id   Service id.
 * @param[in]   report_data  Report related data.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 */
bool hids_send_boot_kb_input_report_notify(uint8_t conn_handle, uint8_t service_id,
                                           T_HIDS_CHAR_KB_MS_REPORT report_data);
#endif
/** @} End of HIDS_GATT_Exported_Functions */

/** @} End of HIDS_GATT_SVC */

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
