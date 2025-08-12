/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     dis_gatt_client.h
  * @brief
  * @details
  * @author   jane
  * @date     2016-02-18
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _DIS_GATT_CLIENT_H_
#define _DIS_GATT_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include <stdint.h>
#include <stdbool.h>
#include "bt_gatt_client.h"

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @addtogroup DIS_CLIENT_Exported_Macros DIS Client Exported Macros
  * @brief
  * @{
  */

/** @brief DIS related UUIDs */
#define GATT_UUID_DEVICE_INFORMATION_SERVICE   0x180A
#define GATT_UUID_CHAR_SYSTEM_ID               0x2A23
#define GATT_UUID_CHAR_MODEL_NUMBER            0x2A24
#define GATT_UUID_CHAR_SERIAL_NUMBER           0x2A25
#define GATT_UUID_CHAR_FIRMWARE_REVISION       0x2A26
#define GATT_UUID_CHAR_HARDWARE_REVISION       0x2A27
#define GATT_UUID_CHAR_SOFTWARE_REVISION       0x2A28
#define GATT_UUID_CHAR_MANUFACTURER_NAME       0x2A29
#define GATT_UUID_CHAR_IEEE_CERTIF_DATA_LIST   0x2A2A
#define GATT_UUID_CHAR_PNP_ID                  0x2A50
#define GATT_UUID_CHAR_UDI_FOR_MEDICAL_DEVICES 0x2BFF

/** @defgroup DIS_CLT_CB_MSG
  * @brief DIS client callback messages
  * @{
  */
#define GATT_MSG_DIS_CLIENT_DIS_DONE           0x00
#define GATT_MSG_DIS_CLIENT_READ_RESULT        0x01

/** @} End of DIS_CLT_CB_MSG */
/** End of DIS_CLIENT_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup DIS_CLIENT_Exported_Types DIS Client Exported Types
  * @brief
  * @{
  */

/**
 * @brief P_FUN_DIS_CLIENT_APP_CB DIS Client Callback Function Point Definition Function
 *        pointer used in DIS client module, to send events to specific client module.
 *        The events @ref DIS_CLT_CB_MSG.
 */
typedef T_APP_RESULT(*P_FUN_DIS_CLIENT_APP_CB)(uint16_t conn_handle, uint8_t type, void *p_data);

/** @brief DIS client discovery result
 *         The message data for GATT_MSG_DIS_CLIENT_DIS_DONE.
*/
typedef struct
{
    bool     is_found;
    bool     load_from_ftl;
    uint8_t  srv_instance_num;
} T_DIS_CLIENT_DIS_DONE;

/** @brief DIS client System ID characteristic*/
typedef struct
{
    uint8_t     mid[5];      /**< manufacturer-defined identifier */
    uint8_t     oui[3];      /**< Organizationally Unique Identifier */
} T_HIDS_CLIENT_SYSTEM_ID;

/** @brief DIS client PnP ID characteristic*/
typedef struct
{
    uint8_t     vendor_id_source;
    uint16_t    vendor_id;
    uint16_t    product_id;
    uint16_t    product_version;
} T_HIDS_CLIENT_PNP_ID;

/** @brief DIS client read result
 *         The message data for GATT_MSG_DIS_CLIENT_READ_RESULT.
*/
typedef struct
{
    uint8_t srv_instance_id;
    uint16_t char_uuid;
    uint16_t cause;
    uint16_t value_size;
    union
    {
        uint8_t                   *char_utf8_string;     /**<
                                                          * Contains Manufacturer Name, Model Number, Serial Number,
                                                          * Hardware Revision, Firmware Revision, Software Revision. */
        T_HIDS_CLIENT_SYSTEM_ID   system_id;
        uint8_t
        *ieee_certif_data_list;    /**< IEEE 11073-20601 Regulatory Certification Data List */
        T_HIDS_CLIENT_PNP_ID      pnp_id;
        uint8_t                   *udi_for_medical_dev;    /**< UDI for Medical Devices */
    } data;
} T_DIS_CLIENT_READ_RESULT;
/** End of DIS_CLIENT_Exported_Types
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup DIS_CLIENT_Exported_Functions DIS Client Exported Functions
  * @brief
  * @{
  */

/**
  * @brief      Add dis client to application.
  * @param[in]  app_cb   Pointer of app callback function to handle specific client module data.
  * @return Result of add the specific client module.
  * @retval true  Add client module success.
  * @retval false Add client module failed.
  *
  * <b>Example usage</b>
  * \code{.c}
    void app_le_profile_init(void)
    {
        if(dis_client_init(app_client_callback))
        {
            APP_PRINT_ERROR0("app_le_profile_init: add dis client fail!");
        }
    }
  * \endcode
  */
bool dis_client_init(P_FUN_DIS_CLIENT_APP_CB app_cb);

/**
  * @brief  Used by application, to read characteristic from server by using UUIDs.
  * @param[in]  conn_handle     Connection handle of the ACL link.
  * @param[in]  srv_instance_id Service instance id.
  * @param[in]  uuid       characteristics uuid.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool dis_client_read_char(uint16_t conn_handle, uint8_t srv_instance_id, uint16_t char_uuid);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* DIS_GATT_CLIENT_H */
