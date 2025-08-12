/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     app_ble_hid_controller.h
  * @brief    Head file for using Human Interface Device Service.
  * @details  HIDS data structs and external functions declaration.
  * @author   Bill Zhao
  * @date     2025-02-26
  * @version  v1.0
  * *************************************************************************************
  */

#ifndef _APP_BLE_HID_CONTROLLER_H_
#define _APP_BLE_HID_CONTROLLER_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "profile_server.h"

/** @defgroup APP_BLE_HID_CONTROLLER Human Interface Device Service of Gamepad
  * @brief  Human Interface Device Service
   * @details

    The HID Service exposes data and associated formatting for HID Devices and HID Hosts.

    Application shall register HID service when initialization through @ref hids_add_service function.

    Application can set the HID service through @ref hids_set_parameter function.

    Application can send report data of HID service to the client with a notification through @ref hids_send_report function.

  * @{
  */


/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup APP_BLE_HID_CONTROLLER_Exported_Macros HIDS Gamepad Exported Macros
  * @brief
  * @{
  */
#define GATT_SVC_HID_INFO_INDEX                     (2)
#define GATT_SVC_HID_CONTROL_POINT_INDEX            (4)
#define GATT_SVC_HID_REPORT_MAP_INDEX               (6)
#define GATT_SVC_HID_REPORT_INPUT_INDEX             (8)
#define GATT_SVC_HID_REPORT_INPUT_CCCD_INDEX        (GATT_SVC_HID_REPORT_INPUT_INDEX+1)
#define GATT_SVC_HID_REPORT_OUTPUT_INDEX            (12)

#define BLE_HID_CONTROLLER_VENDOR_ID                0x005D
#define BLE_HID_CONTROLLER_PRODUCT_ID               0x1234
#define BLE_HID_CONTROLLER_PRODUCT_VERSION          0x0100

/** End of APP_BLE_HID_CONTROLLER_Exported_Macros
* @}
*/


/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup APP_BLE_HID_CONTROLLER_Exported_Types HIDS Gamepad Exported Types
  * @brief
  * @{
  */

/**
*  @brief Human Interface Device Service  parameter type
*/
typedef enum
{
    HID_REPORT_INPUT,
    HID_REPORT_OUTPUT,
    HID_REPORT_FEATURE,
    HID_REPORT_MAP,
    HID_EXTERNAL_REPORT_REFER,
    HID_INFO,
    HID_CONTROL_POINT,
} T_HIDS_PARAM_TYPE;

/**
*  @brief Human Interface Device Service information
*/
typedef struct
{
    uint8_t  b_country_code;
    uint8_t  flags;
    uint16_t bcd_hid;
} T_HID_INFO;

/**
*  @brief Human Interface Device Service report type
*/
typedef enum
{
    HID_INPUT_TYPE   = 1,
    HID_OUTPUT_TYPE  = 2,
    HID_FEATURE_TYPE = 3
} T_PROFILE_HID_REPORT_TYPE;

/**
*  @brief Human Interface Device Service control point
*/
typedef enum
{
    HID_SUSPEND         = 0,
    HID_EXIT_SUSPEND    = 1,
} T_HID_CTL_POINT;

/**
*  @brief Human Interface Device Service protocol mode
*/
typedef enum
{
    BOOT_PROTOCOL_MODE = 0,
    REPORT_PROCOCOL_MODE = 1
} T_HID_PROTOCOL_MODE;

/** @defgroup APP_BLE_HID_CONTROLLER_Upstream_Message HIDS Gamepad Upstream Message
  * @brief  Upstream message used to inform application.
  * @{
  */
typedef enum
{
    NOTIFY_ENABLE,
    NOTIFY_DISABLE
} T_HID_NOTIFY;

typedef union
{
    uint8_t voice_enable;
    uint8_t protocol_mode;
    uint8_t suspend_mode;
    struct
    {
        uint8_t reportLen;
        uint8_t *report;
    } report_data;
} T_HID_WRITE_PARAMETER;

typedef struct
{
    uint8_t write_type;
    T_HID_WRITE_PARAMETER write_parameter;
} T_HID_WRITE_MSG;

typedef struct
{
    uint8_t index;
    T_HID_NOTIFY value;
} T_HID_NOT_IND_DATA;

typedef union
{
    uint8_t read_value_index;
    T_HID_WRITE_MSG write_msg;
    T_HID_NOT_IND_DATA not_ind_data;
} T_HID_UPSTREAM_MSG_DATA;

typedef struct
{
#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    uint16_t                    conn_handle;
    uint16_t                    cid;
#endif
    uint8_t                     conn_id;
    T_SERVICE_CALLBACK_TYPE     msg_type;
    T_HID_UPSTREAM_MSG_DATA     msg_data;
} T_HID_CALLBACK_DATA;

//--------------------------------------------------------------------------------
// Generic Desktop Page inputReport (Device --> Host)
//--------------------------------------------------------------------------------

typedef struct
{
    uint8_t X;
    uint8_t Y;
    uint8_t Z;
    uint8_t Rx;
    uint8_t Ry;
    uint8_t Rz;

    uint8_t vnd_data;
    uint8_t hat_switch : 4;
    uint8_t button_1 : 1;
    uint8_t button_2 : 1;
    uint8_t button_3 : 1;
    uint8_t button_4 : 1;

    uint8_t button_5 : 1;
    uint8_t button_6 : 1;
    uint8_t button_7 : 1;
    uint8_t button_8 : 1;
    uint8_t button_9 : 1;
    uint8_t button_10 : 1;
    uint8_t button_11 : 1;
    uint8_t button_12 : 1;

    uint8_t button_13 : 1;
    uint8_t button_14 : 1;
    uint8_t button_15 : 1;
    uint8_t resv : 5;
} BLE_CONTROLLER_HID_INPUT_REPORT;

/** @} End of APP_BLE_HID_CONTROLLER_Upstream_Message */


/** End of APP_BLE_HID_CONTROLLER_Exported_Types
* @}
*/


/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup APP_BLE_HID_CONTROLLER_Exported_Functions HIDS Gamepad Exported Functions
  * @brief
  * @{
  */

/**
  * @brief       Add HID service to the BLE stack database.
  *
  *
  * @param[in]   p_func  Callback when service attribute was read, write or cccd update.
  * @return Service id generated by the BLE stack: @ref T_SERVER_ID.
  * @retval 0xFF Operation failure.
  * @retval Others Service id assigned by stack.
  *
  * <b>Example usage</b>
  * \code{.c}
     void profile_init()
     {
         server_init(1);
         hids_id = hids_add_service(app_handle_profile_message);
     }
  * \endcode
  */
T_SERVER_ID hids_add_service(void *p_func);

/**
 * @brief       Set a HID service parameter.
 *
 *              NOTE: You can call this function with a HID service parameter type and it will set the
 *                      HID service parameter.  HID service parameters are defined in @ref T_HIDS_PARAM_TYPE.
 *
 * @param[in]   param_type  HID service parameter type: @ref T_HIDS_PARAM_TYPE
 * @param[in]   len         Length of data to write
 * @param[in]   p_value Pointer to data to write.  This is dependent on
 *                      the parameter type and WILL be cast to the appropriate
 *                      data type
 *
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t mode = 1;
        hids_set_parameter(HID_PROTOCOL_MODE, 1, &mode);
    }
 * \endcode
 */
bool hids_set_parameter(T_HIDS_PARAM_TYPE param_type, uint8_t len, void *p_value);

/**
 * @brief       Send HIDS notification data .
 *
 *
 * @param[in]   conn_id  Connection id.
 * @param[in]   service_id  Service id.
 * @param[in]   index  hids characteristic index.
 * @param[in]   p_data report value pointer.
 * @param[in]   data_len length of report data.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t conn_id = 0;
        T_SERVER_ID service_id = hids_id;
        uint8_t hid_report_input[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
        hids_send_report(conn_id, service_id, GATT_SVC_HID_REPORT_INPUT_INDEX, hid_report_input, sizeof(hid_report_input));
    }
 * \endcode
 */
bool hids_send_report(uint8_t conn_id, T_SERVER_ID service_id, uint16_t index, uint8_t *p_data,
                      uint16_t data_len);


void app_ble_controller_adv_stop(void);
void app_ble_controller_adv_start(bool is_swift_pair_adv);
void app_ble_controller_adv_init(void);
#if BLE_CONTROLLER_TEST
void app_ble_controller_test(void);
#endif
/** @} End of APP_BLE_HID_CONTROLLER_Exported_Functions */

/** @} End of APP_BLE_HID_CONTROLLER */

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
