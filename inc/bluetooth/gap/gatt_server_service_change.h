/**
*****************************************************************************************
*     Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    gatt_server_service_change.h
  * @brief   This file contains all the functions prototypes for GATT Service service change
  *          related functions.
  * @details
  * @author
  * @date
  * @version
  * *************************************************************************************
  */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef GATT_SERVER_SERVICE_CHANGE_H
#define GATT_SERVER_SERVICE_CHANGE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "gatt.h"
#include "gap.h"

/** @addtogroup GATT_SERVER_API GATT Server API
  * @{
  */

/** @defgroup GATT_SERVER_SERVICE_CHANGE GATT Server Service Change
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup GATT_SERVER_SERVICE_CHANGE_Exported_Macros GATT Server Service Change Exported Macros
  * @{
  */
/** @defgroup GATT_SERVER_SERVICE_CHANGE_STATE_BIT_Def GATT Server Service Change State Bit definitions
  * @{
  */
#define GATT_SERVER_SERVICE_CHANGE_STATE_BIT_SEND_INDICATION                         0 /**< Bit 0: Whether APP should send Service Changed Indication
                                                                                            * \arg 0 : APP should not send Service Changed Indication
                                                                                            * \arg 1 : APP should send Service Changed Indication */
#define GATT_SERVER_SERVICE_CHANGE_STATE_BIT_CHANGE_UNAWARE                          1 /**< Bit 1: Ignore if Server on local device does not support GATT Caching.
                                                                                            *      From the perspective of Server, Client is either change-aware or change-unaware
                                                                                            * \arg 0 : change-aware
                                                                                            * \arg 1 : change-unaware */
/** End of GATT_SERVER_SERVICE_CHANGE_STATE_BIT_Def
  * @}
  */

/** @defgroup GATT_SERVER_SERVICE_CHANGE_MSG_Opcodes GATT Server Service Change Message Opcodes
  * @{
  */
#define GATT_SERVER_SERVICE_CHANGE_SET_SERVICE_CHANGE_STATE_IND               0x0001   /**< * Inform APP to handle and save service change state.
                                                                                            * Usage refer to @ref service_change and @ref service_change_state in @ref T_GATT_SERVER_SERVICE_CHANGE_SET_SERVICE_CHANGE_STATE_IND */

#define GATT_SERVER_SERVICE_CHANGE_SET_DATABASE_HASH_INFO                     0x0010   /**< * Inform APP to save database hash. */

#define GATT_SERVER_SERVICE_CHANGE_GET_SERVICE_CHANGE_STATE_IND               0x0020   /**< * Inform APP to supply service change state by @ref gatt_server_service_change_update_service_change_state
                                                                                            * If device is bonded, service change state is previous state that is saved by APP.
                                                                                            * If device is not bonded, service change state is 0 . */

#define GATT_SERVER_SERVICE_CHANGE_GET_DATABASE_HASH_IND                      0x0030   /**< * Inform APP to supply database hash by @ref gatt_server_service_change_update_database_hash */
/** End of GATT_SERVER_SERVICE_CHANGE_MSG_Opcodes
  * @}
  */

/** End of GATT_SERVER_SERVICE_CHANGE_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup GATT_SERVER_SERVICE_CHANGE_Exported_Types GATT Server Service Change Exported Types
  * @{
  */

typedef struct
{
    uint16_t                    conn_handle;
    uint8_t
    service_change_state; /**< @ref GATT_SERVER_SERVICE_CHANGE_STATE_BIT_Def */
} T_GATT_SERVER_SERVICE_CHANGE_UPDATE_SERVICE_CHANGE_STATE_PARAM;

typedef struct
{
    uint8_t                     database_hash[GATTS_DATABASE_HASH_LEN];
} T_GATT_SERVER_SERVICE_CHANGE_UPDATE_DATABASE_HASH_PARAM;

/** @brief  Data for @ref GATT_SERVER_SERVICE_CHANGE_SET_SERVICE_CHANGE_STATE_IND */
typedef struct
{
    uint8_t service_change;             /**< Whether service is changed
                                          * \arg 0 : Service is not changed.
                                          *              If specific device is bonded, APP save @ref service_change_state .
                                          *              If specific device is not bonded, APP update temporary service change state with @ref service_change_state .
                                          * \arg 1 : Service is changed.
                                          *             Step 1: For each bonded device, APP save "new service change state".
                                          *             Step 2: For each connected device, APP update temporary service change state with "new service change state",
                                          *                     call @ref gatt_server_service_change_update_service_change_state with "new service change state",
                                          *                     and then send Service Changed Indication.
                                          *
                                          *             If device support Robust Caching (Bit 0 of Octet 0 in the Client Supported Features characteristic value
                                          *             is 1), "new service change state" is @ref service_change_state .
                                          *             If device does not support Robust Caching (Bit 0 of Octet 0 in the Client Supported Features characteristic value
                                          *             is 0), "new service change state" is (@ref service_change_state & (~(1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_CHANGE_UNAWARE))). */

    uint8_t service_change_state;                   /**< APP should not use @ref service_change_state as "new service change state".
                                                         * Usage refer to @ref service_change .
                                                         * @ref GATT_SERVER_SERVICE_CHANGE_STATE_BIT_Def . */

    uint16_t svc_changed_char_cccd_handle;          /**< Ignore if @ref service_change is 0. 0x0000: Invalid handle. */

    uint16_t conn_handle;                           /**< Ignore if @ref service_change is 1. */
    uint8_t  remote_bd_addr[GAP_BD_ADDR_LEN];       /**< Ignore if @ref service_change is 1. */
    T_GAP_REMOTE_ADDR_TYPE      remote_addr_type;   /**< Ignore if @ref service_change is 1. */
} T_GATT_SERVER_SERVICE_CHANGE_SET_SERVICE_CHANGE_STATE_IND;

/** @brief  Data for @ref GATT_SERVER_SERVICE_CHANGE_SET_DATABASE_HASH_INFO */
typedef struct
{
    uint8_t                     database_hash[GATTS_DATABASE_HASH_LEN];
} T_GATT_SERVER_SERVICE_CHANGE_SET_DATABASE_HASH_INFO;

/** @brief  Data for @ref GATT_SERVER_SERVICE_CHANGE_GET_SERVICE_CHANGE_STATE_IND */
typedef struct
{
    uint16_t                    conn_handle;
    uint8_t                     remote_bd_addr[GAP_BD_ADDR_LEN];
    T_GAP_REMOTE_ADDR_TYPE      remote_addr_type;
} T_GATT_SERVER_SERVICE_CHANGE_GET_SERVICE_CHANGE_STATE_IND;

typedef union
{
    T_GATT_SERVER_SERVICE_CHANGE_SET_SERVICE_CHANGE_STATE_IND
    *p_gatt_server_service_change_set_service_change_state_ind;

    T_GATT_SERVER_SERVICE_CHANGE_SET_DATABASE_HASH_INFO
    *p_gatt_server_service_change_set_database_hash_info;

    T_GATT_SERVER_SERVICE_CHANGE_GET_SERVICE_CHANGE_STATE_IND
    *p_gatt_server_service_change_get_service_change_state_ind;
} T_GATT_SERVER_SERVICE_CHANGE_CB_DATA;

/** @brief  Data for @ref GAP_MSG_GATT_SERVER_SERVICE_CHANGE_INFO */
typedef struct
{
    uint16_t                              opcode;   /**< @ref GATT_SERVER_SERVICE_CHANGE_MSG_Opcodes */
    T_GATT_SERVER_SERVICE_CHANGE_CB_DATA  data;
} T_GATT_SERVER_SERVICE_CHANGE_CB;
/** End of GATT_SERVER_SERVICE_CHANGE_Exported_Types
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup GATT_SERVER_SERVICE_CHANGE_Exported_Functions GATT Server Service Change Exported Functions
  * @{
  */

/**
 * @brief       Update service change state.
 *
 *              This API shall be used if any of the following conditions is true:
 *                  \arg cb_type is @ref GAP_MSG_GATT_SERVER_SERVICE_CHANGE_INFO, opcode is
 *                       @ref GATT_SERVER_SERVICE_CHANGE_SET_SERVICE_CHANGE_STATE_IND and
 *                       @ref service_change is 1 :
 *                       APP shall call this API for each connected device with state refer
 *                       to @ref service_change in @ref T_GATT_SERVER_SERVICE_CHANGE_SET_SERVICE_CHANGE_STATE_IND
 *                  \arg cb_type is @ref GAP_MSG_GATT_SERVER_SERVICE_CHANGE_INFO and opcode is
 *                       @ref GATT_SERVER_SERVICE_CHANGE_GET_SERVICE_CHANGE_STATE_IND :
 *                       APP shall call this API for specific device with state refer to @ref GATT_SERVER_SERVICE_CHANGE_GET_SERVICE_CHANGE_STATE_IND
 *
 * @xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * @param[in] p_param   Point to parameter: @ref T_GATT_SERVER_SERVICE_CHANGE_UPDATE_SERVICE_CHANGE_STATE_PARAM
 *
 * @return The result of operation.
 * @retval GAP_CAUSE_SUCCESS  Operation is successful.
 * @retval Others             Operation is failed.
 *
 * <b>Example usage</b>
 * \code{.c}
   // Callback registered by @ref gap_register_app_cb
   void app_gap_common_callback(uint8_t cb_type, void *p_cb_data)
   {
       T_GAP_CB_DATA cb_data;

       memcpy(&cb_data, p_cb_data, sizeof(T_GAP_CB_DATA));

       switch (cb_type)
       {
       ...
        case GAP_MSG_GATT_SERVER_SERVICE_CHANGE_INFO:
            {
                T_GATT_SERVER_SERVICE_CHANGE_CB *p_gatt_server_service_change_info = cb_data.p_gap_cb_data;

                switch (p_gatt_server_service_change_info->opcode)
                {
                case GATT_SERVER_SERVICE_CHANGE_SET_SERVICE_CHANGE_STATE_IND:
                    {
                        if (p_gatt_server_service_change_info->data.p_gatt_server_service_change_set_service_change_state_ind->service_change == 0)
                        {
                            // Service is not changed

                            if // Check whether specific device is bonded
                            {
                                // Specific device is bonded, APP save @ref service_change_state
                            }
                            else
                            {
                                // Specific device is not bonded, APP update temporary service change state with @ref service_change_state
                            }
                        }
                        else if (p_gatt_server_service_change_info->data.p_gatt_server_service_change_set_service_change_state_ind->service_change == 1)
                        {
                            // Service is changed

                            // Step 1: For each bonded device, APP save "new service change state".

                            // For each bonded device
                            {
                                if // Check whether device support Robust Caching (Bit 0 of Octet 0 in the Client Supported Features characteristic value is 1)
                                {
                                    // Support Robust Caching: "new service change state" is @ref service_change_state .
                                }
                                else
                                {
                                    // Not support Robust Caching: "new service change state" is (@ref service_change_state & (~(1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_CHANGE_UNAWARE))).
                                }

                                // APP save "new service change state"
                            }
                            ...
                            // Step 1: End

                            // Step 2: Start
                            ...
                            // For each connected device
                            {
                                if // Check whether device support Robust Caching (Bit 0 of Octet 0 in the Client Supported Features characteristic value is 1)
                                {
                                    // Support Robust Caching: "new service change state" is @ref service_change_state .
                                }
                                else
                                {
                                    // Not support Robust Caching: "new service change state" is (@ref service_change_state & (~(1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_CHANGE_UNAWARE))).
                                }

                                // APP update temporary service change state with "new service change state"
                                ...

                                // APP call @ref gatt_server_service_change_update_service_change_state with "new service change state"
                                gatt_server_service_change_update_service_change_state(p_param);

                                if (p_param->service_change_state & (1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_SEND_INDICATION))
                                {
                                    // APP send Service Changed Indication
                                }
                            }
                            ...
                            // Step 2: End
                        }
                    }
                    break;

                case GATT_SERVER_SERVICE_CHANGE_GET_SERVICE_CHANGE_STATE_IND:
                    // p_gatt_server_service_change_info->data.p_gatt_server_service_change_get_service_change_state_ind->conn_handle

                    // If device is bonded, service change state is previous state that is saved by APP.
                    // If device is not bonded, service change state is 0 .
                    gatt_server_service_change_update_service_change_state(p_param);

                    if (p_param->service_change_state & (1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_SEND_INDICATION))
                    {
                        // APP send Service Changed Indication
                    }
                    break;
                ...
                }
            }
            break;
       }
    ...
   }
 * \endcode
 */
T_GAP_CAUSE gatt_server_service_change_update_service_change_state(
    T_GATT_SERVER_SERVICE_CHANGE_UPDATE_SERVICE_CHANGE_STATE_PARAM *p_param);

/**
 * @brief       Update database hash.
 *
 *              This API shall be used if following condition is true:
 *                  \arg cb_type is @ref GAP_MSG_GATT_SERVER_SERVICE_CHANGE_INFO and
 *                       opcode is @ref GATT_SERVER_SERVICE_CHANGE_GET_DATABASE_HASH_IND .
 *
 * @xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * @param[in] p_param   Point to parameter: @ref T_GATT_SERVER_SERVICE_CHANGE_UPDATE_DATABASE_HASH_PARAM
 *
 * @return The result of operation.
 * @retval GAP_CAUSE_SUCCESS  Operation is successful.
 * @retval Others             Operation is failed.
 *
 * <b>Example usage</b>
 * \code{.c}
   // Callback registered by @ref gap_register_app_cb
   void app_gap_common_callback(uint8_t cb_type, void *p_cb_data)
   {
       T_GAP_CB_DATA cb_data;

       memcpy(&cb_data, p_cb_data, sizeof(T_GAP_CB_DATA));

       switch (cb_type)
       {
       ...
        case GAP_MSG_GATT_SERVER_SERVICE_CHANGE_INFO:
            {
                T_GATT_SERVER_SERVICE_CHANGE_CB *p_gatt_server_service_change_info = cb_data.p_gap_cb_data;

                switch (p_gatt_server_service_change_info->opcode)
                {
                case GATT_SERVER_SERVICE_CHANGE_SET_DATABASE_HASH_INFO:
                    // APP save database hash
                    break;

                case GATT_SERVER_SERVICE_CHANGE_GET_DATABASE_HASH_IND:
                    gatt_server_service_change_update_database_hash(p_param);
                    break;
                ...
                }
            }
            break;
       }
        ...
   }
 * \endcode
 */
T_GAP_CAUSE gatt_server_service_change_update_database_hash(
    T_GATT_SERVER_SERVICE_CHANGE_UPDATE_DATABASE_HASH_PARAM *p_param);

/** End of GATT_SERVER_SERVICE_CHANGE_Exported_Functions
  * @}
  */

/** End of GATT_SERVER_SERVICE_CHANGE
  * @}
  */

/** End of GATT_SERVER_API
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif
