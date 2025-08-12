/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     dis_gatt_svc.h
  * @brief    Head file for using device information service.
  * @details  DIS data structs and external functions declaration developed based on bt_gatt_svc.h.
  * @author
  * @date
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _DIS_GATT_SVC_H_
#define _DIS_GATT_SVC_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include "bt_gatt_svc.h"


/** @defgroup DIS_GATT_SVC DIS Device Information GATT Service
  * @brief  Device Information service
   * @details

     The Device Information Service exposes manufacturer and/or vendor information about a device.

     The Device Information Service may expose one or more of the characteristics. It is possible that
     none of the characteristics below are included.
     The specific configuration process can be achieved by modifying Macro @ref DIS_SVC_FEATURE_SUPPORT.

    * <b>Example usage</b>
    * \code{.c}

        #define DIS_CHAR_MANUFACTURER_NAME_SUPPORT          1
        #define DIS_CHAR_MODEL_NUMBER_SUPPORT               1
        #define DIS_CHAR_SERIAL_NUMBER_SUPPORT              1

    * \endcode

     Application shall register device information service when initialization through @ref dis_reg_srv function.

     Application can reply the characteristic value through @ref dis_char_read_confirm function when receive a read request from dis client.

  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup DIS_Exported_Macros DIS Exported Macros
  * @brief
  * @{
  */
/** @defgroup DIS_SVC_FEATURE_SUPPORT
  * @brief DIS server feature support
  * @{
  */
/** @details
    Set DIS_CHAR_MANUFACTURER_NAME_SUPPORT to 1 to support Manufacturer Name String characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_MANUFACTURER_NAME_SUPPORT          1

/** @details
    Set DIS_CHAR_MODEL_NUMBER_SUPPORT to 1 to support Model Number String characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_MODEL_NUMBER_SUPPORT               1

/** @details
    Set DIS_CHAR_SERIAL_NUMBER_SUPPORT to 1 to support Serial Number String characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_SERIAL_NUMBER_SUPPORT              1

/** @details
    Set DIS_CHAR_FIRMWARE_REVISION_SUPPORT to 1 to support Firmware Revision String characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_FIRMWARE_REVISION_SUPPORT          1

/** @details
    Set DIS_CHAR_HARDWARE_REVISION_SUPPORT to 1 to support Hardware Revision String characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_HARDWARE_REVISION_SUPPORT          1

/** @details
    Set DIS_CHAR_SOFTWARE_REVISION_SUPPORT to 1 to support Software Revision String characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_SOFTWARE_REVISION_SUPPORT          1

/** @details
    Set DIS_CHAR_SYSTEM_ID_SUPPORT to 1 to support System ID characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_SYSTEM_ID_SUPPORT                  1

/** @details
    Set DIS_CHAR_IEEE_CERTIF_DATA_LIST_SUPPORT to 1 to support IEEE 11073-20601 Regulatory Certification Data List characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_IEEE_CERTIF_DATA_LIST_SUPPORT      1

/** @details
    Set DIS_CHAR_PNP_ID_SUPPORT to 1 to support PnP ID characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_PNP_ID_SUPPORT                     1

/** @details
    Set DIS_CHAR_UDI_MEDICAL_DEVICES_SUPPORT to 1 to support UDI for Medical Devices characteristic,
    otherwise set it to 0.
*/
#define DIS_CHAR_UDI_MEDICAL_DEVICES_SUPPORT        1
/** @} End of DIS_SVC_FEATURE_SUPPORT */

/** @defgroup DIS_SVC_CB_MSG
  * @brief DIS server callback messages
  * @{
  */
#define GATT_MSG_DIS_SERVER_READ_CHAR_IND      0x00
/** @} End of DIS_SVC_CB_MSG */

/** @defgroup DIS_SVC_UUID
  * @brief DIS service UUID
  * @{
  */
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
#define GATT_UUID_CHAR_UDI_MEDICAL_DEVICES     0x2BFF
/** @} End of DIS_SVC_UUID */

#define DIS_SYSTEM_ID_LENGTH                   8
#define DIS_PNP_ID_LENGTH                      7


/** End of DIS_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup DIS_Exported_Types DIS Exported Types
  * @brief
  * @{
  */
/**
 * @brief P_FUN_DIS_SERVER_APP_CB DIS Server Callback Function Point Definition Function
 *        pointer used in DIS server module, to send events to specific server module.
 *        The events @ref DIS_SVC_CB_MSG.
 */
typedef T_APP_RESULT(*P_FUN_DIS_SERVER_APP_CB)(uint16_t conn_handle, uint16_t cid,
                                               uint8_t type, void *p_data);


/** @brief DIS server read characteristic value ind
 *         The message data for GATT_MSG_DIS_SERVER_READ_CHAR_IND.
*/
typedef struct
{
    T_SERVER_ID service_id;
    uint16_t    char_uuid;
    uint16_t    offset;
} T_DIS_SERVER_READ_CHAR_IND;
/** End of DIS_Exported_Types
* @}
*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup DIS_Exported_Functions DIS Exported Functions
  * @brief
  * @{
  */

/**
 * @brief       Add device information service to the BLE stack database.
 *
 *
 * @param[in]   app_cb            Callback when service attribute was read, write or cccd update.
 * @return Service id generated by the BLE stack: @ref T_SERVER_ID.
 * @retval 0xFF Operation failure.
 * @retval Others Service id assigned by stack.
 *
 * <b>Example usage</b>
 * \code{.c}
    void profile_init(uint16_t mode, uint8_t svc_num)
    {
         gatt_svc_init(mode, svc_num);
         dis_gatt_srv_id = dis_reg_srv(app_dis_gatt_svc_callback);
    }

    T_APP_RESULT app_dis_gatt_svc_callback(uint16_t conn_handle, uint16_t cid, uint8_t type,
                                           void *p_data)
    {
        if (type == GATT_MSG_DIS_SERVER_READ_CHAR_IND)
        {
            T_DIS_SERVER_READ_CHAR_IND *p_read = (T_DIS_SERVER_READ_CHAR_IND *)p_data;

            APP_PRINT_INFO3("app_dis_gatt_svc_callback: service id %d, char_uuid 0x%x, offset %d",
                            p_read->service_id, p_read->char_uuid, p_read->offset);
        }

        return APP_RESULT_SUCCESS;
    }
 * \endcode
 */
T_SERVER_ID dis_reg_srv(P_FUN_DIS_SERVER_APP_CB app_cb);

/**
 * @brief       Confirm for read dis characteristic value request.
 *              When application receives GATT_MSG_DIS_SERVER_READ_CHAR_IND, if the read request is accepted,
 *              application shall return APP_RESULT_PENDING or APP_RESULT_SUCCESS, and call this API later.
 *
 * @param[in]   conn_handle    Connection handle of the ACL link.
 * @param[in]   cid            Local CID assigned by Bluetooth stack.
 * @param[in]   service_id     Service id.
 * @param[in]   char_uuid      Characteristic UUID.
 * @param[in]   offset         Offset.
 * @param[in]   value_len      Characteristic value length.
 * @param[in]   p_value        Characteristic value.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    T_APP_RESULT app_dis_gatt_svc_callback(uint16_t conn_handle, uint16_t cid, uint8_t type,
                                           void *p_data)
    {
        bool ret = true;
        if (type == GATT_MSG_DIS_SERVER_READ_CHAR_IND)
        {
            T_DIS_SERVER_READ_CHAR_IND *p_read = (T_DIS_SERVER_READ_CHAR_IND *)p_data;

            APP_PRINT_INFO3("app_dis_gatt_svc_callback: service id %d, char_uuid 0x%x, offset %d",
                            p_read->service_id, p_read->char_uuid, p_read->offset);

            switch (p_read->char_uuid)
            {
            case GATT_UUID_CHAR_SYSTEM_ID:
                {
                    const uint8_t DISSystemID[DIS_SYSTEM_ID_LENGTH] = {0, 1, 2, 0, 0, 3, 4, 5};
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                DIS_SYSTEM_ID_LENGTH, (uint8_t *)DISSystemID);
                }
                break;

            case GATT_UUID_CHAR_MODEL_NUMBER:
                {
                    const uint8_t DISModelNumber[] = "Model Nbr 0.9";
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                sizeof(DISModelNumber), (uint8_t *)DISModelNumber);
                }
                break;

            case GATT_UUID_CHAR_SERIAL_NUMBER:
                {
                    const uint8_t DISSerialNumber[] = "RTKBeeSerialNum";
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                sizeof(DISSerialNumber), (uint8_t *)DISSerialNumber);
                }
                break;

            case GATT_UUID_CHAR_FIRMWARE_REVISION:
                {
                    const uint8_t DISFirmwareRev[] = "RTKBeeFirmwareRev";
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                sizeof(DISFirmwareRev), (uint8_t *)DISFirmwareRev);
                }
                break;

            case GATT_UUID_CHAR_HARDWARE_REVISION:
                {
                    const uint8_t DISHardwareRev[] = "RTKBeeHardwareRev";
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                sizeof(DISHardwareRev), (uint8_t *)DISHardwareRev);
                }
                break;

            case GATT_UUID_CHAR_SOFTWARE_REVISION:
                {
                    const uint8_t DISSoftwareRev[] = "RTKBeeSoftwareRev";
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                sizeof(DISSoftwareRev), (uint8_t *)DISSoftwareRev);
                }
                break;

            case GATT_UUID_CHAR_MANUFACTURER_NAME:
                {
                    const uint8_t DISManufacturerName[] = "Realtek BT";
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                sizeof(DISManufacturerName), (uint8_t *)DISManufacturerName);
                }
                break;

            case GATT_UUID_CHAR_IEEE_CERTIF_DATA_LIST:
                {
                    const uint8_t DISIEEEDataList[] = "RTKBeeIEEEDatalist";
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                sizeof(DISIEEEDataList), (uint8_t *)DISIEEEDataList);
                }
                break;

            case GATT_UUID_CHAR_PNP_ID:
                {
                    uint16_t version = 0x01; //VERSION_BUILD;
                    uint8_t DISPnpID[DIS_PNP_ID_LENGTH] = {0x01, 0x5D, 0x00, 0x01, 0x00,
                                                           (uint8_t)version, (uint8_t)(version >> 8)}; //VID_005D&PID_0001?
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                DIS_PNP_ID_LENGTH, DISPnpID);
                }
                break;

            case GATT_UUID_CHAR_UDI_MEDICAL_DEVICES:
                {
                    uint8_t UDIMedicalDevices[] = {0x01, 'R', 'T', 'K', 'U', 'D', 'I', 'L', 'a', 'b', 'e', 'l', 0x00};
                    ret = dis_char_read_confirm(conn_handle, cid, p_read->service_id, p_read->char_uuid, p_read->offset,
                                                sizeof(UDIMedicalDevices), UDIMedicalDevices);
                }
                break;

            default:
                break;
            }
        }

        if (ret)
        {
            return APP_RESULT_SUCCESS;
        }
        else
        {
            return APP_RESULT_APP_ERR;
        }
    }
 * \endcode
 */
bool dis_char_read_confirm(uint16_t conn_handle, uint16_t cid, uint8_t service_id,
                           uint16_t char_uuid, uint16_t offset,
                           uint16_t value_len, uint8_t *p_value);

/** @} End of DIS_Exported_Functions */

/** @} End of DIS_GATT_SVC */


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _DIS_GATT_SVC_H_ */
