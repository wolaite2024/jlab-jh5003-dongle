/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dis_gatt_svc.c
* @brief    Device Information service source file.
* @details  Interfaces to access Device Information service developed based on bt_gatt_svc.h.
* @author
* @date
* @version  v1.0
*********************************************************************************************************
*/
#include "string.h"
#include "trace.h"
#include "dis_gatt_svc.h"
/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

/**<  Function pointer used to send event to application from dis profile. */
static P_FUN_DIS_SERVER_APP_CB  pfn_dis_cb = NULL;
/**< @brief  profile/service definition.  */
static const T_ATTRIB_APPL dis_attr_tbl[] =
{
    /*----------------- Device Information Service -------------------*/
    /* <<Primary Service>> */
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE), /* flags     */
        {                                         /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_DEVICE_INFORMATION_SERVICE),  /* service UUID */
            HI_WORD(GATT_UUID_DEVICE_INFORMATION_SERVICE)
        },
        UUID_16BIT_SIZE,                              /* bValueLen     */
        NULL,                                     /* p_value_context */
        GATT_PERM_READ                            /* permissions  */
    }

#if DIS_CHAR_MANUFACTURER_NAME_SUPPORT
    ,
    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                       /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* Manufacturer Name String characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_MANUFACTURER_NAME),
            HI_WORD(GATT_UUID_CHAR_MANUFACTURER_NAME)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif

#if DIS_CHAR_MODEL_NUMBER_SUPPORT
    ,
    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                       /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* Model Number characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_MODEL_NUMBER),
            HI_WORD(GATT_UUID_CHAR_MODEL_NUMBER)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif

#if DIS_CHAR_SERIAL_NUMBER_SUPPORT
    ,
    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* Serial Number String String characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_SERIAL_NUMBER),
            HI_WORD(GATT_UUID_CHAR_SERIAL_NUMBER)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif

#if DIS_CHAR_HARDWARE_REVISION_SUPPORT
    ,
    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* Hardware Revision String characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_HARDWARE_REVISION),
            HI_WORD(GATT_UUID_CHAR_HARDWARE_REVISION)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif

#if DIS_CHAR_FIRMWARE_REVISION_SUPPORT
    ,
    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* Firmware revision String characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_FIRMWARE_REVISION),
            HI_WORD(GATT_UUID_CHAR_FIRMWARE_REVISION)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif

#if DIS_CHAR_SOFTWARE_REVISION_SUPPORT
    ,
    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* Software Revision String characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_SOFTWARE_REVISION),
            HI_WORD(GATT_UUID_CHAR_SOFTWARE_REVISION)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif

#if DIS_CHAR_SYSTEM_ID_SUPPORT
    ,
    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                       /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* System ID String characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_SYSTEM_ID),
            HI_WORD(GATT_UUID_CHAR_SYSTEM_ID)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif

#if DIS_CHAR_IEEE_CERTIF_DATA_LIST_SUPPORT
    ,

    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* IEEE 11073-20601 Regulatory Certification Data List characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_IEEE_CERTIF_DATA_LIST),
            HI_WORD(GATT_UUID_CHAR_IEEE_CERTIF_DATA_LIST)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif

#if DIS_CHAR_PNP_ID_SUPPORT
    ,
    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* PnP ID characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_PNP_ID),
            HI_WORD(GATT_UUID_CHAR_PNP_ID)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif

#if DIS_CHAR_UDI_MEDICAL_DEVICES_SUPPORT
    ,
    /* <<Characteristic>> */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* UDI for Medical Devices characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_UDI_MEDICAL_DEVICES),
            HI_WORD(GATT_UUID_CHAR_UDI_MEDICAL_DEVICES)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }
#endif
};
/**< @brief  DIS service size definition.  */
const uint16_t dis_char_num = sizeof(dis_attr_tbl) / sizeof(T_ATTRIB_APPL);

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
                           uint16_t value_len, uint8_t *p_value)
{
    bool ret = false;
    uint16_t attrib_index = gatt_svc_find_char_index_by_uuid16(dis_attr_tbl, char_uuid,
                                                               dis_char_num);
    if (attrib_index == 0)
    {
        PROFILE_PRINT_ERROR0("dis_char_read_confirm: attr not found");
        return ret;
    }

    if ((char_uuid == GATT_UUID_CHAR_SYSTEM_ID && value_len != DIS_SYSTEM_ID_LENGTH) ||
        (char_uuid == GATT_UUID_CHAR_PNP_ID && value_len != DIS_PNP_ID_LENGTH))
    {
        PROFILE_PRINT_ERROR1("dis_char_read_confirm: invalid value length char_uuid 0x%x", char_uuid);
        return ret;
    }

    uint8_t *p_data = p_value;

    return gatt_svc_read_confirm(conn_handle, cid, service_id, attrib_index, p_data + offset,
                                 value_len - offset, value_len, APP_RESULT_SUCCESS);
}

/**
 * @brief read characteristic data from service.
 *
 * @param conn_handle       Connection handle of the ACL link.
 * @param cid               Local CID assigned by Bluetooth stack.
 * @param service_id        ServiceID to be read.
 * @param attrib_index      Attribute index of getting characteristic data.
 * @param offset            Offset of characteritic to be read.
 * @param p_length          Length of getting characteristic data.
 * @param pp_value          Pointer to pointer of characteristic value to be read.
 * @return T_APP_RESULT
*/
T_APP_RESULT dis_attr_read_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                              uint16_t attrib_index,
                              uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    *p_length = 0;
    T_CHAR_UUID char_uuid = gatt_svc_find_char_uuid_by_index(dis_attr_tbl, attrib_index,
                                                             dis_char_num);
    T_DIS_SERVER_READ_CHAR_IND read_ind = {0};
    read_ind.service_id = service_id;
    read_ind.offset = offset;
    read_ind.char_uuid = char_uuid.uu.char_uuid16;

    PROFILE_PRINT_INFO4("dis_attr_read_cb: conn_handle 0x%x, service_id 0x%x, char_uuid %d offset %d",
                        conn_handle, service_id, read_ind.char_uuid, offset);

    switch (read_ind.char_uuid)
    {
    default:
        {
            PROFILE_PRINT_ERROR0("dis_attr_read_cb: attr not found");
            cause  = APP_RESULT_ATTR_NOT_FOUND;
        }
        break;

    case GATT_UUID_CHAR_SYSTEM_ID:
    case GATT_UUID_CHAR_MODEL_NUMBER:
    case GATT_UUID_CHAR_SERIAL_NUMBER:
    case GATT_UUID_CHAR_FIRMWARE_REVISION:
    case GATT_UUID_CHAR_HARDWARE_REVISION:
    case GATT_UUID_CHAR_SOFTWARE_REVISION:
    case GATT_UUID_CHAR_MANUFACTURER_NAME:
    case GATT_UUID_CHAR_IEEE_CERTIF_DATA_LIST:
    case GATT_UUID_CHAR_PNP_ID:
    case GATT_UUID_CHAR_UDI_MEDICAL_DEVICES:
        {
            if (pfn_dis_cb)
            {
                cause = pfn_dis_cb(conn_handle, cid, GATT_MSG_DIS_SERVER_READ_CHAR_IND, (void *)&read_ind);
            }

            if (cause == APP_RESULT_SUCCESS)
            {
                cause = APP_RESULT_PENDING;
            }
        }
        break;
    }

    return (cause);
}

/**
 * @brief DIS Service Callbacks.
*/
const T_FUN_GATT_EXT_SERVICE_CBS  dis_cbs =
{
    dis_attr_read_cb,  // Read callback function pointer
    NULL, // Write callback function pointer
    NULL  // CCCD update callback function pointer
};

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
         dis_id = dis_reg_srv(app_dis_service_callback);
    }
 * \endcode
 */
T_SERVER_ID dis_reg_srv(P_FUN_DIS_SERVER_APP_CB app_cb)
{
    T_SERVER_ID service_id;
    if (false == gatt_svc_add(&service_id,
                              (uint8_t *)dis_attr_tbl,
                              sizeof(dis_attr_tbl),
                              &dis_cbs, NULL))
    {
        PROFILE_PRINT_ERROR1("dis_reg_srv: service_id %d", service_id);
        service_id = 0xff;
    }

    pfn_dis_cb = app_cb;
    return service_id;
}

