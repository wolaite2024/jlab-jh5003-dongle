/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     ancs_gatt_client.h
  * @brief    Head file for using battery service client.
  * @details  ANCS client data structs and external functions declaration
  *           developed based on bt_gatt_client.h.
  * @author   jane
  * @date     2016-02-18
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _ANCS_GATT_CLIENT_H_
#define _ANCS_GATT_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include <stdint.h>
#include <stdbool.h>
#include "bt_gatt_client.h"

/** @defgroup ANCS_GATT_CLIENT ANCS GATT Client
  * @brief ANCS client
  * @details
     Application shall register ancs client when initialization through @ref ancs_client_init function.

     Application can config cccd data through @ref ancs_client_cfg_cccd function.

     Application shall handle callback function registered by ancs_client_init.
  * \code{.c}
    T_APP_RESULT app_ancs_client_callback(uint16_t conn_handle, uint8_t type, void *p_data)
    {
        T_APP_RESULT  result = APP_RESULT_SUCCESS;
        switch(type)
        {
            case GATT_MSG_ANCS_CLIENT_DIS_DONE:
                {
                    T_ANCS_CLIENT_DIS_DONE *p_dis_done = (T_ANCS_CLIENT_DIS_DONE *)p_data;
                }
                break;
            ......
        }
    }
  * \endcode
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup ANCS_GATT_CLIENT_Exported_Macros ANCS GATT Client Exported Macros
  * @brief
  * @{
  */

/** @defgroup ANCS_CLT_CB_MSG
  * @brief ANCS client callback messages
  * @{
  */
#define GATT_MSG_ANCS_CLIENT_DIS_DONE                   0x00
#define GATT_MSG_ANCS_CLIENT_WRITE_CP_RESULT            0x01
#define GATT_MSG_ANCS_CLIENT_DATA_SOURCE                0x02
#define GATT_MSG_ANCS_CLIENT_NOTIFICATION_SOURCE        0x03
#define GATT_MSG_ANCS_CLIENT_CCCD_CFG_RESULT            0x04
#define GATT_MSG_ANCS_CLIENT_CONN_DEL                   0x05


/**  @brief  ANCS event flag */
#define NS_EVENT_FLAG_SILENT          (1 << 0)
#define NS_EVENT_FLAG_IMPORTANT       (1 << 1)
#define NS_EVENT_FLAG_PRE_EXISTING    (1 << 2)
#define NS_EVENT_FLAG_POSITIVE_ACTION (1 << 3) //!<A positive action exists and is associated with this iOS notification
#define NS_EVENT_FLAG_NEGATIVE_ACTION (1 << 4) //!<A negative action exists and is associated with this iOS notification.

/**  @brief  Smart Phone App attribute id. */
typedef enum
{
    DS_APP_ATTR_ID_DISPLAY_NAME = 0,
    DS_APP_ATTR_ID_RESERVED = 255
} T_DS_APP_ATTR_ID;

/**  @brief  ANCS action id state */
typedef enum
{
    CP_ACTION_ID_POSITIVE = 0,
    CP_ACTION_ID_NEGATIVE = 1,
    CP_ACTION_ID_RESERVED  = 255
} T_CP_ACTION_ID_VALUES;

/** @brief ANCS client control command id*/
typedef enum
{
    CP_CMD_ID_GET_NOTIFICATION_ATTR = 0,
    CP_CMD_ID_GET_APP_ATTR = 1,
    CP_CMD_ID_PERFORM_NOTIFICATION_ACTION = 2,
    CP_CMD_ID_RESERVED = 255
} T_ANCS_CP_CMD_ID;

/**  @brief  ANCS notification attribute id */
typedef enum
{
    DS_NOTIFICATION_ATTR_ID_APP_IDENTIFIER = 0, /**< Format: UTF-8 strings */
    DS_NOTIFICATION_ATTR_ID_TITLE = 1, /**<Format: UTF-8 strings. Needs to be followed by a 2-bytes max length parameter */
    DS_NOTIFICATION_ATTR_ID_SUB_TITLE = 2, /**<Format: UTF-8 strings. Needs to be followed by a 2-bytes max length parameter */
    DS_NOTIFICATION_ATTR_ID_MESSAGE = 3, /**<Format: UTF-8 strings. Needs to be followed by a 2-bytes max length parameter */
    DS_NOTIFICATION_ATTR_ID_MESSAGE_SIZE = 4, /**<The format of the DS_NOTIFICATION_ATTR_ID_MESSAGE_SIZE constant is a string
    that represents the integral value of the message size. */
    DS_NOTIFICATION_ATTR_ID_DATE = 5, /**<The format of the DS_NOTIFICATION_ATTR_ID_DATE constant is a string
    that uses the Unicode Technical Standard (UTS) #35 date format pattern yyyyMMdd'T'HHmmSS. */
    DS_NOTIFICATION_ATTR_ID_POSITIVE_ACTION_LABEL = 6, /**<Format: UTF-8 strings. The label used to describe
    the positive action that can be performed on the iOS notification. */
    DS_NOTIFICATION_ATTR_ID_NEGATIVE_ACTION_LABEL = 7, /**<Format: UTF-8 strings. The label used to describe
    the negative action that can be performed on the iOS notification. */
    DS_NOTIFICATION_ATTR_ID_RESERVED = 255
} T_DS_NOTIFICATION_ATTR_ID;

/**  @brief  ANCS event id for the peer device notification */
typedef enum
{
    NS_EVENT_ID_NOTIFICATION_ADD = 0,      /**< The arrival of a new iOS notification on the NP */
    NS_EVENT_ID_NOTIFICATION_MODIFIED = 1, /**< The modification of an iOS notification on the NP */
    NS_EVENT_ID_NOTIFICATION_REMOVED = 2,  /**< The removal of an iOS notification on the NP */
    NS_EVENT_ID_NOTIFICATION_RESERVED = 0xff
} T_NS_EVENT_ID;

/**  @brief  ANCS category id */
typedef enum
{
    NS_CATEGORY_ID_OTHER = 0,
    NS_CATEGORY_ID_INCOMING_CALL = 1,
    NS_CATEGORY_ID_MISSED_CALL = 2,
    NS_CATEGORY_ID_VOICE_MAIL = 3,
    NS_CATEGORY_ID_SOCIAL = 4,
    NS_CATEGORY_ID_SCHEDULE = 5,
    NS_CATEGORY_ID_EMAIL = 6,
    NS_CATEGORY_ID_NEWS = 7,
    NS_CATEGORY_ID_HEALTH_AND_FITNESS = 8,
    NS_CATEGORY_ID_BUSINESS_ADN_FINANCE = 9,
    NS_CATEGORY_ID_LOCATION = 10,
    NS_CATEGORY_ID_ENTERTAINMENT = 11,
    NS_CATEGORY_ID_RESERVED = 255
} T_NS_CATEGORY_ID;

/** @} End of ANCS_CLT_CB_MSG */
/** End of ANCS_GATT_CLIENT_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup ANCS_GATT_CLIENT_Exported_Types ANCS GATT Client Exported Types
  * @brief
  * @{
  */
/**
 * @brief P_FUN_ANCS_CLIENT_APP_CB ANCS Client Callback Function Point Definition Function
 *        pointer used in ANCS client module, to send events to specific client module.
 *        The events @ref ANCS_CLT_CB_MSG.
 */
typedef T_APP_RESULT(*P_FUN_ANCS_CLIENT_APP_CB)(uint16_t conn_handle, uint8_t type, void *p_data);

/** @brief ANCS source type*/
typedef enum
{
    ANCS_DATA_SOURCE,
    ANCS_NOTIFICATION_SOURCE,
} T_ANCS_SOURCE;

/** @brief ANCS client discovery result
 *         The message data for GATT_MSG_ANCS_CLIENT_DIS_DONE.
*/
typedef struct
{
    bool     is_found;
    bool     load_from_ftl;
} T_ANCS_CLIENT_DIS_DONE;

/** @brief ANCS client write control point result
 *         The message data for GATT_MSG_ANCS_CLIENT_WRITE_CP_RESULT.
*/
typedef struct
{
    uint16_t cause;
} T_ANCS_CLIENT_WRITE_CP_RESULT;

/** @brief ANCS client notify info
 *         The message data for GATT_MSG_ANCS_CLIENT_DATA_SOURCE.
*/
typedef struct
{
    uint16_t      value_size;
    uint8_t      *p_value;
} T_ANCS_CLIENT_DATA_SOURCE;

/** @brief ANCS client notification source data
 *         The message data for GATT_MSG_ANCS_CLIENT_NOTIFICATION_SOURCE.
*/
typedef struct
{
    uint8_t event_id;          /**< This field informs the accessory whether the given iOS notification
    was added, modified, or removed. The enumerated values for this field are defined in @ref T_NS_EVENT_ID*/
    uint8_t event_flags;       /**< A bitmask whose set bits inform an NC of specificities with the iOS notification. */
    uint8_t category_id;       /**< A numerical value providing a category in which the iOS notification
    can be classified. The enumerated values for this field are defined in @ref T_NS_CATEGORY_ID*/
    uint8_t category_count;    /**< The current number of active iOS notifications in the given category. */
    uint32_t notification_uid; /**< A 32-bit numerical value that is the unique identifier (UID) for the iOS notification.*/
} T_ANCS_CLIENT_NOTIFICATION_SOURCE;

/** @brief ANCS client configure cccd data info
 *         The message data for GATT_MSG_ANCS_CLIENT_CCCD_CFG_RESULT.
*/
typedef struct
{
    T_ANCS_SOURCE type;
    uint16_t      cause;
    bool          enable;
} T_ANCS_CLIENT_CCCD_CFG_RESULT;

/** @brief ANCS client ACL disconnected
 *         The message data for GATT_MSG_ANCS_CLIENT_CONN_DEL.
*/
typedef struct
{
    bool transport_le;
} T_ANCS_CLIENT_CONN_DEL;
/** End of ANCS_GATT_CLIENT_Exported_Types
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup ANCS_GATT_CLIENT_Exported_Functions ANCS GATT Client Exported Functions
  * @brief
  * @{
  */

/**
  * @brief      Add ancs client to application.
  * @param[in]  app_cb   Pointer of app callback function to handle specific client module data.
  * @return Result of add the specific client module.
  * @retval true  Add client module success.
  * @retval false Add client module failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        if(ancs_client_init(app_client_callback) == false)
        {
            APP_PRINT_ERROR0("app_le_profile_init: add ancs client fail!");
        }
    }
  * \endcode
  */
bool ancs_client_init(P_FUN_ANCS_CLIENT_APP_CB app_cb);

/**
  * @brief      Config the ANCS service CCCD data.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @param[in]  type            Source type.
  * @param[in]  enable          Whether to enable CCCD.
  * \arg    true    Enable CCCD.
  * \arg    false   Disable CCCD.
  * @return Result of config the ANCS service CCCD data.
  * @retval true  Config the ANCS service CCCD data success.
  * @retval false Config the ANCS service CCCD data failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void test(uint16_t conn_handle, T_ANCS_SOURCE type)
    {
        ancs_client_cfg_cccd(conn_handle, type, true);
    }
  * \endcode
  */
bool ancs_client_cfg_cccd(uint16_t conn_handle, T_ANCS_SOURCE type, bool enable);

/**
  * @brief  Used by application, to get the notification attribute.
  * @param[in]  conn_handle       connection handle.
  * @param[in]  notification_uid  value to enable or disable notify.
  * @param[in]  p_attribute_ids   Pointer to attribute ids.
  * @param[in]  attribute_ids_len Length of attribute ids.
  * @retval true send request to bluetooth host success.
  * @retval false send request to bluetooth host failed.
  */
bool ancs_client_cp_get_notification_attr(uint16_t conn_handle, uint32_t notification_uid,
                                          uint8_t *p_attribute_ids, uint8_t attribute_ids_len);

/**
  * @brief  Used by application, to get the app attribute.
  * @param[in]  conn_handle       connection handle.
  * @param[in]  p_app_identifier  value to enable or disable notify.
  * @param[in]  p_attribute_ids   Pointer to attribute ids.
  * @param[in]  attribute_ids_len Length of attribute ids.
  * @retval true send request to bluetooth host success.
  * @retval false send request to bluetooth host failed.
  */
bool ancs_client_cp_get_app_attr(uint16_t conn_handle, char *p_app_identifier,
                                 uint8_t *p_attribute_ids, uint8_t attribute_ids_len);

/**
  * @brief  Used by application, to perform the notification action.
  * @param[in]  conn_handle       connection handle.
  * @param[in]  notification_uid Notification UUID.
  * @param[in]  action_id        Action id.
  * @retval true send request to bluetooth host success.
  * @retval false send request to bluetooth host failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void test(void)
    {
        uint8_t attr_id_list[14];
        uint8_t cur_index = 0;

        attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_APP_IDENTIFIER;
        attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_TITLE;
        attr_id_list[cur_index++] = LO_WORD(ANCS_MAX_ATTR_LEN);
        attr_id_list[cur_index++] = HI_WORD(ANCS_MAX_ATTR_LEN);

        attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_SUB_TITLE;
        attr_id_list[cur_index++] = LO_WORD(ANCS_MAX_ATTR_LEN);
        attr_id_list[cur_index++] = HI_WORD(ANCS_MAX_ATTR_LEN);

        attr_id_list[cur_index++] = DS_NOTIFICATION_ATTR_ID_DATE;

        ancs_client_cp_get_app_attr(conn_handle, (char *)"com.tencent.mqq",
                                 attr_id_list, cur_index);
    }
  * \endcode
  */
bool ancs_client_cp_perform_notification_action(uint16_t conn_handle, uint32_t notification_uid,
                                                uint8_t action_id);
/** @} End of ANCS_GATT_CLIENT_Exported_Functions */

/** @} End of ANCS_GATT_CLIENT */


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* ANCS_GATT_CLIENT_H */
