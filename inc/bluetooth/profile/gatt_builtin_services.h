/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     gatt_builtin_services.h
  * @brief    Header file for using built-in services, including GAP service and GATT service.
  * @details  GAPS data structures and external functions declaration.
  * @author   Jane
  * @date     2015-5-12
  * @version  v0.1
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _BUILTIN_SERVICES_H_
#define _BUILTIN_SERVICES_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include <profile_server.h>

/** @defgroup GAP_GATT_SERVICE GAP and GATT Inbox Services
  * @brief GAP and GATT inbox services
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup GAP_GATT_SERVICE_Exported_Macros GAP and GATT Service Exported Macros
  * @brief
  * @{
  */

/** @defgroup GAPS_Write_PROPERTY GAP Service Write Property
  * @brief  GAP service write property.
  * @{
  */
#define GAPS_PROPERTY_WRITE_DISABLE                   0
#define GAPS_PROPERTY_WRITE_ENABLE                    1
/** @} */


/** @defgroup GAPS_WRITE_TYPE GAP and GATT Service Write Type
  * @brief  GAP and GATT Service Write Type.
  * @{
  */
#define GAPS_WRITE_DEVICE_NAME            1
#define GAPS_WRITE_APPEARANCE             2
#define GATT_SERVICE_CHANGE_CCCD_ENABLE   3
#define GATT_SERVICE_CHANGE_CCCD_DISABLE  4
#define GATT_SERVICE_WRITE_CLIENT_SUPPORTED_FEATURES 5
/** @} */


/** End of GAP_GATT_SERVICE_Exported_Macros
* @}
*/

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup GAP_GATT_SERVICE_Exported_Types GAP and GATT Service Exported Types
  * @brief
  * @{
  */

/** @brief GAPS parameter type */
typedef enum
{
    GAPS_PARAM_DEVICE_NAME = 0x00,                  //!< GAPS parameter device name, range of value length is from 0 to (GAP_DEVICE_NAME_LEN - 1).
    GAPS_PARAM_APPEARANCE  = 0x01,                  //!< GAPS parameter appearance, value length is 2.
    GAPS_PARAM_CENTRAL_ADDRESS_RESOLUTION = 0x02,   //!< GAPS parameter central address resolution, value length is 1.
    GAPS_PARAM_DEVICE_NAME_PROPERTY = 0x03,         //!< GAPS parameter device name property, value length is 1.
    GAPS_PARAM_APPEARANCE_PROPERTY = 0x04,          //!< GAPS parameter appearance property, value length is 1.
} T_GAPS_PARAM_TYPE;

/** @brief Builtin services data struct for notification data to application. */
typedef struct
{
    uint8_t  opcode; //!<  @ref GAPS_WRITE_TYPE.
    uint16_t len;
    uint8_t  *p_value;
} T_GAPS_UPSTREAM_MSG_DATA;

/** @brief Builtin services callback data to inform application. */
typedef struct
{
    T_SERVICE_CALLBACK_TYPE     msg_type;
    uint8_t
    conn_id;     //!< This parameter can use when parameter use_ext of the server_cfg_use_ext_api is false.
#if F_BT_GATT_SERVER_EXT_API
    uint16_t
    conn_handle; //!< This parameter can use when parameter use_ext of the server_cfg_use_ext_api is true.
    uint16_t
    cid;         //!< This parameter can use when parameter use_ext of the server_cfg_use_ext_api is true.
#endif
    T_GAPS_UPSTREAM_MSG_DATA    msg_data;
} T_GAPS_CALLBACK_DATA;

/** End of GAP_GATT_SERVICE_Exported_Types
* @}
*/
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup GAP_GATT_SERVICE_Exported_Functions GAP and GATT Service Exported Functions
  * @brief
  * @{
  */

/**
 * @brief  Register callback to builtin services.
 *
 * @param[in] p_func   Callback to notify APP.
 * @return void.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t appearance_prop = GAPS_PROPERTY_WRITE_ENABLE;
        uint8_t device_name_prop = GAPS_PROPERTY_WRITE_ENABLE;
        gaps_set_parameter(GAPS_PARAM_APPEARANCE_PROPERTY, sizeof(appearance_prop), &appearance_prop);
        gaps_set_parameter(GAPS_PARAM_DEVICE_NAME_PROPERTY, sizeof(device_name_prop), &device_name_prop);
        gatt_register_callback(gap_service_callback);
    }
    T_APP_RESULT gap_service_callback(T_SERVER_ID service_id, void *p_para)
    {
        T_APP_RESULT  result = APP_RESULT_SUCCESS;
        T_GAPS_CALLBACK_DATA *p_gap_data = (T_GAPS_CALLBACK_DATA *)p_para;
        APP_PRINT_INFO2("gap_service_callback conn_id %d msg_type %d", p_gap_data->conn_id,
                        p_gap_data->msg_type);
        if (p_gap_data->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
        {
            switch (p_gap_data->msg_data.opcode)
            {
            case GAPS_WRITE_DEVICE_NAME:
                {
                    T_LOCAL_NAME device_name;
                    memcpy(device_name.local_name, p_gap_data->msg_data.p_value, p_gap_data->msg_data.len);
                    device_name.local_name[p_gap_data->msg_data.len] = 0;
                    flash_save_local_name(&device_name);
                }
                break;

            case GAPS_WRITE_APPEARANCE:
                {
                    uint16_t appearance_val;
                    T_LOCAL_APPEARANCE appearance;

                    LE_ARRAY_TO_UINT16(appearance_val, p_gap_data->msg_data.p_value);
                    appearance.local_appearance = appearance_val;
                    flash_save_local_appearance(&appearance);
                }
                break;

            default:
                break;
            }
        }
        else if (p_gap_data->msg_type == SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION)
        {
            if (p_gap_data->msg_data.opcode == GATT_SERVICE_CHANGE_CCCD_ENABLE)
            {
                APP_PRINT_INFO0("GATT_SERVICE_CHANGE_CCCD_ENABLE");
            }
        }
        return result;
    }
 * \endcode
 */
void gatt_register_callback(void *p_func);

/**
 * @brief  Set GAP service parameter.
 *
 * @param[in] param_type   Parameter type to set: @ref T_GAPS_PARAM_TYPE.
 * @param[in] length       Value length to be set.
 * @param[in] p_value      Value to set.
 * @return The result of set parameter operation.
 * @retval true Set parameter operation is successful.
 * @retval false Set parameter operation is failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t appearance_prop = GAPS_PROPERTY_WRITE_ENABLE;
        uint8_t device_name_prop = GAPS_PROPERTY_WRITE_ENABLE;
        gaps_set_parameter(GAPS_PARAM_APPEARANCE_PROPERTY, sizeof(appearance_prop), &appearance_prop);
        gaps_set_parameter(GAPS_PARAM_DEVICE_NAME_PROPERTY, sizeof(device_name_prop), &device_name_prop);
    }
 * \endcode
 */
bool gaps_set_parameter(T_GAPS_PARAM_TYPE param_type, uint8_t length, void *p_value);


/**
  * @brief  Set the preferred connection parameter.
  *
  * @param[in] conn_interval_min   Defines minimum value for the connection interval in the
                                    following manner:
                                    connIntervalmin = Conn_Interval_Min * 1.25 ms.
                                    Conn_Interval_Min range: 0x0006 to 0x0C80.
                                    Value of 0xFFFF indicates no specific minimum.
                                    Values outside the range (except 0xFFFF) are reserved for
                                    future use.
  * @param[in] conn_interval_max   Defines maximum value for the connection interval in the
                                    following manner:
                                    connIntervalmax = Conn_Interval_Max * 1.25 ms.
                                    Conn_Interval_Max range: 0x0006 to 0x0C80.
                                    Shall be equal to or greater than the Conn_Interval_Min.
                                    Value of 0xFFFF indicates no specific maximum.
                                    Values outside the range (except 0xFFFF) are reserved for
                                    future use.
  * @param[in] slave_latency        Defines the slave latency for the connection in number of
                                    connection events.
                                    Slave latency range: 0x0000 to 0x01F3.
                                    Values outside the range are reserved for future use.
  * @param[in] supervision_timeout  Defines the connection supervisor timeout multiplier as a multiple of 10ms.
                                    @arg Range: 0xFFFF indicates no specific value requested.
                                    @arg Range: 0x000A to 0x0C80.
                                    @arg Time = N * 10 ms.
                                    @arg Time Range: 100 ms to 32 seconds.
                                    Values outside the range (except 0xFFFF) are reserved for
                                    future use.
  * @return void.
  */
void gaps_set_peripheral_preferred_conn_param(uint16_t conn_interval_min,
                                              uint16_t conn_interval_max,
                                              uint16_t slave_latency,
                                              uint16_t supervision_timeout);

/**
  * @brief    Get service handle range of GAP service.
  *
  * Applications can only call this API after the Bluetooth Host is ready. \n
  *                  Explanation: If the Bluetooth Host is ready, the application will be notified by message @ref GAP_MSG_LE_DEV_STATE_CHANGE
  *                               with new_state about gap_init_state, which is configured as @ref GAP_INIT_STATE_STACK_READY.
  *
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
  *
  * @param[in,out] p_starting_handle      Pointer to location to get starting handle.
  * @param[in,out] p_ending_handle        Pointer to location to get ending handle.
  *
  * @return Get result.
  * @retval true Success.
  * @retval false Get failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void test(void)
    {
        bool ret = gaps_get_service_handle_range(&starting_handle, &ending_handle);
    }
  * \endcode
  */
bool gaps_get_service_handle_range(uint16_t *p_starting_handle, uint16_t *p_ending_handle);

/**
  * @brief    Get service handle range of GATT service.
  *
  * Applications can only call this API after the Bluetooth Host is ready. \n
  *                  Explanation: If the Bluetooth Host is ready, the application will be notified by message @ref GAP_MSG_LE_DEV_STATE_CHANGE
  *                               with new_state about gap_init_state, which is configured as @ref GAP_INIT_STATE_STACK_READY.
  *
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
  *
  * @param[in,out] p_starting_handle      Pointer to location to get starting handle.
  * @param[in,out] p_ending_handle        Pointer to location to get ending handle.
  *
  * @return Get result.
  * @retval true Success.
  * @retval false Get failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void test(void)
    {
        bool ret = gatts_get_service_handle_range(&starting_handle, &ending_handle);
    }
  * \endcode
  */
bool gatts_get_service_handle_range(uint16_t *p_starting_handle, uint16_t *p_ending_handle);

/**
 * @brief  Send service changed indication.
 *         Applications can use this API when parameter use_ext of the server_cfg_use_ext_api is false.
 *
 * @param[in] conn_id      Connection ID.
 * @param[in] start_handle Start of Affected Attribute Handle Range.
 * @param[in] end_handle   End of Affected Attribute Handle Range.
 * @return The result of sending operation.
 * @retval true Sending request is successful.
 * @retval false Sending request is failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint16_t start_handle = 0x0001;
        uint16_t end_handle = 0xFFFF;
        bool ret = gatts_service_changed_indicate(conn_id, start_handle, end_handle);
    }
 * \endcode
 */
bool gatts_service_changed_indicate(uint8_t conn_id, uint16_t start_handle, uint16_t end_handle);

/**
 * @brief  Send service changed indication.
 *         Applications can use this API when parameter use_ext of the server_cfg_use_ext_api is true.
 *
 * @param[in] conn_handle  Connection handle of the ACL link.
 * @param[in] cid          Local Channel Identifier assigned by Bluetooth Host.
 * @param[in] start_handle Start of Affected Attribute Handle Range.
 * @param[in] end_handle   End of Affected Attribute Handle Range.
 * @return The result of sending operation.
 * @retval true Sending request is successful.
 * @retval false Sending request is failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint16_t start_handle = 0x0001;
        uint16_t end_handle = 0xFFFF;
        bool ret = gatts_ext_service_changed_indicate(conn_handle, cid, start_handle, end_handle);
    }
 * \endcode
 */
bool gatts_ext_service_changed_indicate(uint16_t conn_handle, uint16_t cid, uint16_t start_handle,
                                        uint16_t end_handle);

/** End of GAP_GATT_SERVICE_Exported_Functions
* @}
*/

/** End of GAP_GATT_SERVICE
* @}
*/


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _BUILTIN_SERVICES_H_ */
