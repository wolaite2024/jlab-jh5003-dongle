/**
*****************************************************************************************
*     Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     hids_gatt_client.h
  * @brief
  * @details
  * @author   cen_zhang
  * @date     2023-9-20
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _HIDS_GATT_CLIENT_H_
#define _HIDS_GATT_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include <profile_client.h>
#include <stdint.h>
#include <stdbool.h>
#include "bt_gatt_client.h"

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup HIDS_GATT_CLIENT_Exported_Macros Hids GATT Client Exported Macros
  * @brief
  * @{
  */


#define GATT_UUID_HIDS                          0x1812
#define GATT_UUID_CHAR_PROTOCOL_MODE            0x2A4E
#define GATT_UUID_CHAR_REPORT                   0x2A4D
#define GATT_UUID_CHAR_REPORT_MAP               0x2A4B
#define GATT_UUID_CHAR_HID_INFO                 0x2A4A
#define GATT_UUID_CHAR_HID_CONTROL_POINT        0x2A4C

/** @defgroup HIDS_CLT_CB_MSG
  * @brief HIDS client callback messages
  * @{
  */
#define GATT_MSG_HIDS_CLIENT_DIS_DONE            0x00
#define GATT_MSG_HIDS_CLIENT_READ_RESULT         0x01
#define GATT_MSG_HIDS_CLIENT_WRITE_RESULT        0x02
#define GATT_MSG_HIDS_CLIENT_NOTIFY_IND          0x03
/** @} End of HIDS_CLT_CB_MSG */

/** End of HIDS_GATT_CLIENT_Exported_Macros
* @}
*/

typedef T_APP_RESULT(*P_FUN_HIDS_CLIENT_APP_CB)(uint16_t conn_handle, uint8_t type,
                                                void *p_data);


/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup HIDS_GATT_CLIENT_Exported_Types Hids GATT Client Exported Types
  * @brief
  * @{
  */

/**
 * @brief hid report type
 *
 */
typedef enum
{
    HID_INPUT_TYPE   = 1,
    HID_OUTPUT_TYPE  = 2,
    HID_FEATURE_TYPE = 3
} T_HID_REPORT_TYPE;

/** @brief BLE hid client discovery done*/
typedef struct
{
    bool is_found;
    bool load_from_ftl;
    uint8_t srv_instance_num;
} T_HIDS_CLIENT_DIS_DONE;

/** @brief HIDS client report characteristic*/
typedef struct
{
    uint8_t  report_id;
    uint8_t  *p_hids_report_value;
    uint16_t hids_report_value_len;
} T_HIDS_CLIENT_REPORT;

/** @brief HIDS client report map characteristic*/
typedef struct
{
    uint8_t     *p_hids_report_map;
    uint16_t    hids_report_map_len;
} T_HIDS_CLIENT_REPORT_MAP;

/** @brief BLE hid client read result*/
typedef struct
{
    uint8_t srv_instance_id;
    uint16_t char_uuid;
    uint16_t cause;
    union
    {
        T_HIDS_CLIENT_REPORT_MAP    hids_report_map;
        T_HIDS_CLIENT_REPORT        hids_report;
    } data;
} T_HIDS_CLIENT_READ_RESULT;

/** @brief HIDS client write result
 *         The message data for GATT_MSG_HIDS_CLIENT_WRITE_RESULT
*/
typedef struct
{
    uint8_t srv_instance_id;
    uint16_t char_uuid;
    uint16_t cause;
} T_HIDS_CLIENT_WRITE_RESULT;

/** @brief HIDS client notify data*/
typedef struct
{
    uint8_t  srv_instance_id;
    uint8_t  report_id;
    uint16_t data_len;
    uint8_t *p_data;
} T_HIDS_CLIENT_NOTIFY_DATA;

/** End of HIDS_CLIENT_Exported_Types
* @}
*/


/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup HIDS_CLIENT_Exported_Functions HIDS Client Exported Functions
  * @brief
  * @{
  */

/**
 * @brief api of get hids client service number
 *
 * @param conn_handle le link connect handle
 * @return service number
 */
uint8_t hids_client_get_service_num(uint16_t conn_handle);

/**
 * @brief api of read hids report map value
 *
 * @param conn_handle le link connect handle
 * @param srv_instance_id service instance id
 * @return true
 * @return false
 */
bool hids_client_read_report_map_value(uint16_t conn_handle, uint8_t srv_instance_id);

/**
  * @brief           Add hid service client to application.
  * @param[in]       app_cb pointer of app callback function to handle specific client module data.
  * @param[in]       link_num initialize link num.
  * @return true     add client success
  * @return false    add client fail
  */
bool hids_add_client(P_FUN_HIDS_CLIENT_APP_CB app_cb, uint8_t link_num);

/**
 * @brief api of hids read report
 *
 * @param conn_handle le link connect handle
 * @param srv_instance_id service instance id
 * @param report_id  report id
 * @param report_type  report type
 * @return true
 * @return false
 */
bool hids_client_read_report(uint8_t conn_handle, uint8_t srv_instance_id, uint8_t report_id,
                             T_HID_REPORT_TYPE report_type);

/**
 * @brief api of hids write report
 *
 * @param conn_handle le link connect handle
 * @param srv_instance_id service instance id
 * @param report_id  report id
 * @param report_type  report type
 * @param length data length
 * @param p_data data pointer
 * @return true
 * @return false
 */
bool hids_client_write_report(uint16_t conn_handle, uint8_t srv_instance_id, uint8_t report_id,
                              T_HID_REPORT_TYPE report_type, uint16_t length,
                              uint8_t *p_data);

/** @} End of HIDS_GATT_CLIENT_Exported_Functions */

/** @} End of HIDS_GATT_CLIENT */




#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _HIDS_GATT_CLIENT_H_ */
