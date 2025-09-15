/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     transmit_gatt_client.h
  * @brief    Head file for using transmit gatt Client.
  * @details  transmit data structs and external functions declaration.
  * @author   cen
  * @date     2024-05-31
  * @version  v1.1
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _TRANSMIT_GATT_CLIENT_H_
#define _TRANSMIT_GATT_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include "bt_gatt_client.h"

/** @defgroup TRANSMIT_Client transmit service client
  * @brief transmit service client
  * @details
     transmit Profile is a customized BLE-based Profile. transmit ble service please refer to @ref TRANSMIT_Service .
  * @{
  */
/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup TRANSMIT_Client_Exported_Macros TRANSMIT Client Exported Macros
  * @brief
  * @{
  */

/** @brief  Define links number. range: 0-4 */
#define TRANSMIT_MAX_LINKS  2
#define TRANSMIT_DATA_MIN_LEN 12
/** End of TRANSMIT_Client_Exported_Macros
  * @}
  */

typedef T_APP_RESULT(*P_FUN_CLIENT_GENERAL_APP_CB)(uint16_t conn_handle, uint8_t *p_srv_uuid,
                                                   void *p_data);

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup TRANSMIT_Client_Exported_Types TRANSMIT Client Exported Types
  * @brief
  * @{
  */
/** @brief TRANSMIT client discovery done*/
typedef struct
{
    uint16_t conn_handle;
    bool is_found;
    bool load_from_ftl;
} T_TRANSMIT_DISC_DONE;

/** @brief TRANSMIT client write type*/
typedef enum
{
    TRANSMIT_WRITE_DATA,
} T_TRANSMIT_WRTIE_TYPE;

/** @brief TRANSMIT client write result*/
typedef struct
{
    T_TRANSMIT_WRTIE_TYPE type;
    uint16_t cause;

} T_TRANSMIT_WRITE_RESULT;

/** @brief TRANSMIT client cccd cfg*/
typedef struct
{
    uint16_t conn_handle;
    uint16_t cause;
} T_TRANSMIT_CCCD_CFG;

/** @brief TRANSMIT client notify indication*/
typedef struct
{
    uint16_t conn_handle;
    uint16_t value_size;
    uint8_t *p_value;
} T_TRANSMIT_NOTIFY_IND;

/** @brief TRANSMIT client callback type*/
typedef enum
{
    TRANSMIT_CLIENT_CB_TYPE_DISC_DONE,          //!< Discovery procedure state, done or pending.
    TRANSMIT_CLIENT_CB_TYPE_WRITE_RESULT,        //!< Write request result, success or fail.
    TRANSMIT_CLIENT_CB_TYPE_CCCD_CFG,        //!< Write request result, success or fail.
    TRANSMIT_CLIENT_CB_TYPE_NOTIFY_IND,        //!< Write request result, success or fail.
    TRANSMIT_CLIENT_CB_TYPE_INVALID              //!< Invalid callback type, no practical usage.
} T_TRANSMIT_CLIENT_CB_TYPE;

/** @brief TRANSMIT client callback content*/
typedef union
{
    T_TRANSMIT_DISC_DONE      disc_done;
    T_TRANSMIT_WRITE_RESULT    write_result;
    T_TRANSMIT_CCCD_CFG        cccd_cfg;
    T_TRANSMIT_NOTIFY_IND      notify_ind;
} T_TRANSMIT_CLIENT_CB_CONTENT;

/** @brief TRANSMIT client callback data*/
typedef struct
{
    T_TRANSMIT_CLIENT_CB_TYPE     cb_type;
    T_TRANSMIT_CLIENT_CB_CONTENT  cb_content;
} T_TRANSMIT_CLT_CB_DATA;


/** End of TRANSMIT_Client_Exported_Types * @} */

/** @defgroup TRANSMIT_Client_Exported_Functions TRANSMIT Client Exported Functions
  * @{
  */

/**
  * @brief           Add transmit service client to application.
  * @param[in]       app_cb pointer of app callback function to handle specific client module data.
  * @return true     add client to upper stack success
  * @return false    add client to upper stack fail
  */
bool transmit_client_init(P_FUN_CLIENT_GENERAL_APP_CB app_cb);

/**
  * @brief  Used by application, to write char to transmit server.
  * @param conn_handle     connection handle.
  * @param p_transmit_data the pointer of data for writing to server.
  * @param data_len        the len of data for writing to server
  * @retval true           send data success.
  * @retval false          send data failed.
  */
bool transmit_data_write(uint16_t conn_handle, uint8_t *p_transmit_data, uint16_t data_len);

/** @} End of TRANSMIT_Client_Exported_Functions */

/** @} End of TRANSMIT_Client */

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _TRANSMIT_GATT_CLIENT_H_ */
