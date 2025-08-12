/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     bas_gatt_client.h
  * @brief    Head file for using battery service client.
  * @details  Battery service client data structs and external functions declaration
  *           developed based on bt_gatt_client.h.
  * @author   jane
  * @date     2016-02-18
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _BAS_GATT_CLIENT_H_
#define _BAS_GATT_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include <stdint.h>
#include <stdbool.h>
#include "bt_gatt_client.h"



/** @defgroup BAS_GATT_CLIENT Battery Service GATT Client
  * @brief BAS client
  * @details
     Application shall register bas client when initialization through @ref bas_client_init function.

     Application can read battery level characteristic value through @ref bas_client_read_battery_level function.

     Application can config cccd data through @ref bas_client_cfg_cccd function.

     Application shall handle callback function registered by bas_client_init.
  * \code{.c}
    T_APP_RESULT app_bas_client_callback(uint16_t conn_handle, uint8_t type, void *p_data)
    {
        T_APP_RESULT  result = APP_RESULT_SUCCESS;
        switch(type)
        {
            case GATT_MSG_BAS_CLIENT_DIS_DONE:
                {
                    T_BAS_CLIENT_DIS_DONE *p_dis_done = (T_BAS_CLIENT_DIS_DONE *)p_data;
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
/** @addtogroup BAS_CLIENT_Exported_Macros BAS Client Exported Macros
  * @brief
  * @{
  */

/** @defgroup BAS_CLT_CB_MSG
  * @brief BAS client callback messages
  * @{
  */
#define GATT_MSG_BAS_CLIENT_DIS_DONE                   0x00
#define GATT_MSG_BAS_CLIENT_READ_BATTERY_LEVEL_RESULT  0x01
#define GATT_MSG_BAS_CLIENT_BATTERY_LEVEL_NOTIFY       0x02
#define GATT_MSG_BAS_CLIENT_CCCD_CFG_RESULT            0x03
/** @} End of BAS_CLT_CB_MSG */
/** End of BAS_CLIENT_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup BAS_CLIENT_Exported_Types BAS Client Exported Types
  * @brief
  * @{
  */
/**
 * @brief P_FUN_BAS_CLIENT_APP_CB BAS Client Callback Function Point Definition Function
 *        pointer used in BAS client module, to send events to specific client module.
 *        The events @ref BAS_CLT_CB_MSG.
 */
typedef T_APP_RESULT(*P_FUN_BAS_CLIENT_APP_CB)(uint16_t conn_handle, uint8_t type, void *p_data);

/** @brief BAS client discovery result
 *         The message data for GATT_MSG_BAS_CLIENT_DIS_DONE.
*/
typedef struct
{
    bool     is_found;
    bool     load_from_ftl;
    uint8_t  srv_instance_num;
} T_BAS_CLIENT_DIS_DONE;

/** @brief BAS client read result
 *         The message data for GATT_MSG_BAS_CLIENT_READ_BATTERY_LEVEL_RESULT.
*/
typedef struct
{
    uint8_t   srv_instance_id;
    uint16_t  cause;
    uint8_t   battery_level;
} T_BAS_CLIENT_READ_BATTERY_LEVEL_RESULT;

/** @brief BAS client notify info
 *         The message data for GATT_MSG_BAS_CLIENT_BATTERY_LEVEL_NOTIFY.
*/
typedef struct
{
    uint8_t   srv_instance_id;
    uint8_t   battery_level;
} T_BAS_CLIENT_BATTERY_LEVEL_NOTIFY;

/** @brief BAS client configure cccd data info
 *         The message data for GATT_MSG_BAS_CLIENT_CCCD_CFG_RESULT.
*/
typedef struct
{
    uint8_t    srv_instance_id;
    uint16_t   cause;
    bool       enable;
} T_BAS_CLIENT_CCCD_CFG_RESULT;
/** End of BAS_CLIENT_Exported_Types
* @}
*/


/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup BAS_CLIENT_Exported_Functions BAS Client Exported Functions
  * @brief
  * @{
  */

/**
  * @brief      Add bas client to application.
  * @param[in]  app_cb   Pointer of app callback function to handle specific client module data.
  * @return Result of add the specific client module.
  * @retval true  Add client module success.
  * @retval false Add client module failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        if(bas_client_init(app_client_callback))
        {
            APP_PRINT_ERROR0("app_le_profile_init: add bas client fail!");
        }
    }
  * \endcode
  */
bool bas_client_init(P_FUN_BAS_CLIENT_APP_CB app_cb);

/**
  * @brief      Config the BAS service CCCD data.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @param[in]  srv_instance_id Service instance id.
  * @param[in]  enable          Whether to enable CCCD.
  * \arg    true    Enable CCCD.
  * \arg    false   Disable CCCD.
  * @return Result of config the BAS service CCCD data.
  * @retval true  Config the BAS service CCCD data success.
  * @retval false Config the BAS service CCCD data failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void test(uint16_t conn_handle, uint8_t srv_instance_id)
    {
        bas_client_cfg_cccd(conn_handle, srv_instance_id, true);
    }
  * \endcode
  */
bool bas_client_cfg_cccd(uint16_t conn_handle, uint8_t srv_instance_id, bool enable);

/**
  * @brief  Used by application, to read battery level.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @param[in]  srv_instance_id Service instance id.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    static T_USER_CMD_PARSE_RESULT cmd_basread(T_USER_CMD_PARSED_VALUE *p_parse_value)
    {
        uint16_t conn_handle = p_parse_value->dw_param[0];
        uint8_t srv_instance_id = p_parse_value->dw_param[1];
        bool ret = false;
        ret = bas_client_read_battery_level(conn_handle, srv_instance_id);
        ......
    }
  * \endcode
  */
bool bas_client_read_battery_level(uint16_t conn_handle, uint8_t srv_instance_id);

/** @} End of BAS_CLIENT_Exported_Functions */

/** @} End of BAS_GATT_CLIENT */


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* BAS_GATT_CLIENT_H */
