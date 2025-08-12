/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     ias_gatt_svc.c
* @brief    immediate alert level service source file.
* @details  Interfaces to access immediate alert level service developed based on bt_gatt_svc.h.
* @author
* @date
* @version  v1.0
*********************************************************************************************************
*/

#include "stdint.h"
#include "string.h"
#include "trace.h"
#include "ias_gatt_svc.h"

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/

///@cond
/** @brief IAS related services UUIDs */
#define GATT_UUID_IMMEDIATE_ALERT_SERVICE       0x1802
#define GATT_UUID_CHAR_ALERT_LEVEL              0x2A06

/** @brief Index defines for Characteristic's value */
#define GATT_SVC_PXP_IMMEDIATE_AlERT_VALUE_INDEX   2
///@endcond

static P_FUN_IAS_SERVER_APP_CB pfn_ias_cb = NULL;

/**< @brief  profile/service definition.  */
const T_ATTRIB_APPL ias_attr_tbl[] =
{
    /*----------------- Immediate Alert Service -------------------*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /* wFlags     */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_IMMEDIATE_ALERT_SERVICE),    /* service UUID */
            HI_WORD(GATT_UUID_IMMEDIATE_ALERT_SERVICE)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* pValueContext */
        GATT_PERM_READ                              /* wPermissions  */
    },

    /* Alert Level Characteristic */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_WRITE_NO_RSP,              /* characteristic properties */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /* Alert Level Characteristic value  */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_ALERT_LEVEL),
            HI_WORD(GATT_UUID_CHAR_ALERT_LEVEL)
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_WRITE                             /* wPermissions */
    }
};
/**< @brief  IAS service size definition.  */
const int ias_attr_tbl_size = sizeof(ias_attr_tbl);

/**
 * @brief write characteristic data from service.
 *
 * @param[in] conn_handle       Connection handle.
 * @param[in] cid               Local CID assigned by Bluetooth stack.
 * @param[in] service_id        ServiceID to be written.
 * @param[in] attrib_index      Attribute index of characteristic.
 * @param[in] write_type        Write type.
 * @param[in] length            Length of writing characteristic data.
 * @param[in] p_value           Pointer to characteristic data.
 * @param[in] p_write_ind_post_proc      Function pointer after ias_attr_write_cb.
 * @return TProfileResult
*/
T_APP_RESULT ias_attr_write_cb(uint16_t conn_handle, uint16_t cid, uint8_t service_id,
                               uint16_t attrib_index, T_WRITE_TYPE write_type,
                               uint16_t length, uint8_t *p_value,
                               P_FUN_EXT_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
    T_APP_RESULT cause  = APP_RESULT_SUCCESS;

    if (p_value == NULL)
    {
        PROFILE_PRINT_ERROR2("ias_attr_write_cb: p_value %p length= 0x%x", p_value, length);
        cause = APP_RESULT_INVALID_PDU;
        return cause;
    }

    switch (attrib_index)
    {
    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;

    case GATT_SVC_PXP_IMMEDIATE_AlERT_VALUE_INDEX:
        {
            T_IAS_SERVER_WRITE_ALERT_LEVEL write_ind = {0};
            write_ind.service_id = service_id;

            if (length != sizeof(uint8_t))
            {
                cause = APP_RESULT_INVALID_VALUE_SIZE;
            }
            else
            {
                write_ind.alert_level = p_value[0];
            }

            if (pfn_ias_cb && (cause == APP_RESULT_SUCCESS))
            {
                cause = pfn_ias_cb(conn_handle, cid, GATT_MSG_IAS_SERVER_WRITE_ALERT_LEVEL, (void *)&write_ind);
            }
        }
        break;
    }

    return cause;
}

/*********************************************************************
 * SERVICE CALLBACKS
 */
// IAS related Service Callbacks
const T_FUN_GATT_EXT_SERVICE_CBS ias_cbs =
{
    NULL,  // Read callback function pointer
    ias_attr_write_cb, // Write callback function pointer
    NULL  // CCCD update callback function pointer
};

/**
 * @brief Add immediate alert service to the BLE stack database.
 *
 * @param[in]   app_cb  Callback when service attribute was read, write or cccd update.
 * @return Service id generated by the BLE stack: @ref T_SERVER_ID.
 * @retval 0xFF Operation failure.
 * @retval others Service id assigned by stack.
 *
 * <b>Example usage</b>
 * \code{.c}
    void profile_init(uint16_t mode, uint8_t svc_num)
    {
        gatt_svc_init(mode, svc_num);
        ias_id = ias_reg_srv(app_ias_service_callback);
    }
 * \endcode
 */
T_SERVER_ID ias_reg_srv(P_FUN_IAS_SERVER_APP_CB app_cb)
{
    T_SERVER_ID service_id;
    if (false == gatt_svc_add(&service_id,
                              (uint8_t *)ias_attr_tbl,
                              ias_attr_tbl_size,
                              &ias_cbs, NULL))
    {
        PROFILE_PRINT_ERROR1("ias_reg_srv: service_id %d", service_id);
        service_id = 0xff;
    }

    pfn_ias_cb = app_cb;
    return service_id;
}

