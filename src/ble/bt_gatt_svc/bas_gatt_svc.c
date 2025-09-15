/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     bas_gatt_svc.c
* @brief    Battery service source file.
* @details  Interfaces to access Battery service developed based on bt_gatt_svc.h.
* @author
* @date
* @version  v1.0
*********************************************************************************************************
*/
#include "string.h"
#include "trace.h"
#include "bas_gatt_svc.h"
/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

#define GATT_SVC_BAS_BATTERY_LEVEL_INDEX        2 /**< @brief Index for battery level chars's value */
#define GATT_SVC_BAS_CHAR_CCCD_INDEX            3 /**< @brief CCCD Index for battery level chars's value */

/**<  BatteryLevel value. */
/**<  Function pointer used to send event to application from bas profile. */
static P_FUN_BAS_SERVER_APP_CB  pfn_bas_cb = NULL;
/**< @brief  profile/service definition.  */
static const T_ATTRIB_APPL bas_attr_tbl[] =
{
    /*----------------- Battery Service -------------------*/
    /* <<Primary Service>>, .. */
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),   /* flags     */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_BATTERY),              /* service UUID */
            HI_WORD(GATT_UUID_BATTERY)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* p_value_context */
        GATT_PERM_READ                              /* permissions  */
    },

    /* <<Characteristic>>, .. */
    {
        ATTRIB_FLAG_VALUE_INCL,                           /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
#if BAS_BATTERY_LEVEL_NOTIFY_SUPPORT
            (GATT_CHAR_PROP_READ |                    /* characteristic properties */
             GATT_CHAR_PROP_NOTIFY)
#else
            GATT_CHAR_PROP_READ
#endif/* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    /* Battery Level value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_BAS_LEVEL),
            HI_WORD(GATT_UUID_CHAR_BAS_LEVEL)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    }
#if BAS_BATTERY_LEVEL_NOTIFY_SUPPORT
    ,
    /* client characteristic configuration */
    {
        ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL,                   /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            /* NOTE: this value has an instantiation for each client, a write to */
            /* this attribute does not modify this default value:                */
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* permissions */
    }
#endif
};
/**< @brief  Battery service size definition.  */
const static uint16_t bas_attr_tbl_size = sizeof(bas_attr_tbl);

#if BAS_BATTERY_LEVEL_NOTIFY_SUPPORT
/**
 * @brief       Send notify battery level notification data .
 *
 * @param[in]   conn_handle    Connection handle of the ACL link.
 * @param[in]   service_id  Service id.
 * @param[in]   battery_level  Battery level value.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t battery_level = 90;
        bas_send_battery_level_notify(conn_handle, bas_id, battery_level);
    }
 * \endcode
 */
bool bas_send_battery_level_notify(uint16_t conn_handle,
                                   uint8_t service_id, uint8_t battery_level)
{
    return gatt_svc_send_data(conn_handle, L2C_FIXED_CID_ATT, service_id,
                              GATT_SVC_BAS_BATTERY_LEVEL_INDEX,
                              &battery_level, sizeof(battery_level), GATT_PDU_TYPE_NOTIFICATION);
}
#endif

/**
 * @brief       Confirm for read battery level value request.
 *              If application accepts the read request, application shall return APP_RESULT_PENDING or
 *              APP_RESULT_SUCCESS when receive GATT_MSG_BAS_SERVER_READ_BATTERY_LEVEL_IND,
 *              and call this API later.
 *
 * @param[in]   conn_handle    Connection handle of the ACL link.
 * @param[in]   cid            Local CID assigned by Bluetooth stack.
 * @param[in]   service_id     Service id.
 * @param[in]   battery_level  Battery level.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t battery_level = 90;
        bas_battery_level_read_confirm(conn_handle, cid, bas_id, battery_level);
    }
 * \endcode
 */
bool bas_battery_level_read_confirm(uint16_t conn_handle, uint16_t cid,
                                    uint8_t service_id, uint8_t battery_level)
{
    return gatt_svc_read_confirm(conn_handle, cid, service_id,
                                 GATT_SVC_BAS_BATTERY_LEVEL_INDEX,
                                 &battery_level, sizeof(battery_level),
                                 sizeof(battery_level), APP_RESULT_SUCCESS);
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
T_APP_RESULT bas_attr_read_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                              uint16_t attrib_index,
                              uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    *p_length = 0;


    PROFILE_PRINT_INFO2("bas_attr_read_cb: attrib_index %d offset %d", attrib_index, offset);

    switch (attrib_index)
    {
    default:
        {
            PROFILE_PRINT_ERROR0("bas_attr_read_cb: attr not found");
            cause  = APP_RESULT_ATTR_NOT_FOUND;
        }
        break;

    case GATT_SVC_BAS_BATTERY_LEVEL_INDEX:
        {
            T_BAS_SERVER_READ_BATTERY_LEVEL_IND read_ind = {0};
            read_ind.service_id = service_id;

            if (pfn_bas_cb)
            {
                cause = pfn_bas_cb(conn_handle, cid, GATT_MSG_BAS_SERVER_READ_BATTERY_LEVEL_IND, (void *)&read_ind);
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
 * @brief update CCCD bits from stack.
 *
 * @param conn_handle       Connection handle of the ACL link.
 * @param cid               Local CID assigned by Bluetooth stack.
 * @param service_id        Service ID.
 * @param index             Attribute index of characteristic data.
 * @param ccc_bits          CCCD bits from stack.
 * @return None
*/
void bas_cccd_update_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id, uint16_t index,
                        uint16_t ccc_bits)
{
    T_BAS_SERVER_CCCD_UPDATE cccd_info = {0};
    cccd_info.service_id = service_id;

    bool handle = true;

    PROFILE_PRINT_INFO2("bas_cccd_update_cb: index %d ccc_bits 0x%04x", index, ccc_bits);

    switch (index)
    {
    case GATT_SVC_BAS_CHAR_CCCD_INDEX:
        {

            cccd_info.char_uuid = GATT_UUID_CHAR_BAS_LEVEL;
            cccd_info.cccd_cfg = ccc_bits;
        }
        break;

    default:
        {
            handle = false;
            break;
        }

    }

    if (pfn_bas_cb && (handle == true))
    {
        pfn_bas_cb(conn_handle, cid, GATT_MSG_BAS_SERVER_CCCD_UPDATE, (void *)&cccd_info);
    }

    return;
}

/**
 * @brief BAS Service Callbacks.
*/
const T_FUN_GATT_EXT_SERVICE_CBS  bas_cbs =
{
    bas_attr_read_cb,  // Read callback function pointer
    NULL, // Write callback function pointer
    bas_cccd_update_cb  // CCCD update callback function pointer
};

/**
 * @brief       Add battery service to the BLE stack database.
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
         bas_id = bas_reg_srv(app_bas_service_callback);
    }
 * \endcode
 */
T_SERVER_ID bas_reg_srv(P_FUN_BAS_SERVER_APP_CB app_cb)
{
    T_SERVER_ID service_id;
    if (false == gatt_svc_add(&service_id,
                              (uint8_t *)bas_attr_tbl,
                              bas_attr_tbl_size,
                              &bas_cbs, NULL))
    {
        PROFILE_PRINT_ERROR1("bas_reg_srv: service_id %d", service_id);
        service_id = 0xff;
    }

    pfn_bas_cb = app_cb;
    return service_id;
}

