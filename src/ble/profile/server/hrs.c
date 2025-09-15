/*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     hrs.c
* @brief    Heart Rate service source file.
* @details  Interfaces to access Heart Rate service.
* @author
* @date     2017-9-21
* @version  v1.0
*********************************************************************************************************
*/
#include "stdint.h"
#include "gatt.h"
#include <string.h>
#include "trace.h"
#include "profile_server.h"
#include "gap_conn_le.h"
#include "os_mem.h"
#include "hrs.h"


/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/
#define HRS_BODY_SENSOR_LOCATION_CHAR_SUPPORT
#define HRS_ENERGY_EXPENDED_FEATURE_SUPPORT

#define HRS_HEART_RATE_MEASUREMENT_VALUE_INDEX              2//notify
#define HRS_BODY_SENSOR_LOCATION_VALUE_INDEX                5//read
#define HRS_HEART_RATE_CP_VALUE_INDEX                       7//write

#define HRS_HEART_RATE_MEASUREMENT_CCCD_INDEX               3

#define GATT_UUID_SERVICE_HEART_RATE                        0x180D
#define GATT_UUID_CHAR_HRS_HEART_RATE_MEASUREMENT           0x2A37
#define GATT_UUID_CHAR_HRS_BODY_SENSOR_LOCATION             0x2A38
#define GATT_UUID_CHAR_HRS_HEART_RATE_CP                    0x2A39

#define HRS_CP_RSPCODE_RESERVED                             0x00
#define HRS_CP_RSPCODE_SUCCESS                              0x01
#define HRS_CP_RSPCODE_OPCODE_UNSUPPORT                     0x02
#define HRS_CP_RSPCODE_INVALID_PARAMETER                    0x03
#define HRS_CP_RSPCODE_OPERATION_FAILED                     0x04

#define HRS_CTL_PNT_OPERATE_ACTIVE(x)                     \
    (x == HRS_HEART_RATE_CP_OPCODE_RESET_ENERGY_EXPENDED)

#define HEART_RATE_VALUE_FORMAT_UINT8                       0
#define HEART_RATE_VALUE_FORMAT_UINT16                      1

HRS_NOTIFY_INDICATE_FLAG hrs_notify_indicate_flag = {0};
T_HRS_CONTROL_POINT hrs_ctl_pnt = {0};


/**<  Function pointer used to send event to application from location and navigation profile. */

static P_FUN_SERVER_GENERAL_CB pfn_hrs_app_cb = NULL;

/**< @brief  profile/service definition.  */
static const T_ATTRIB_APPL hrs_attr_tbl[] =
{
    /*----------------- Heart Rate Service -------------------*/
    /* <<Primary Service>>, .. 0,*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),   /* wFlags     */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_SERVICE_HEART_RATE),              /* service UUID */
            HI_WORD(GATT_UUID_SERVICE_HEART_RATE)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* pValueContext */
        GATT_PERM_READ                              /* wPermissions  */
    },

    /* <<Characteristic>>, Heart Rate Measurement*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_NOTIFY/* characteristic properties */
            )
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
    /* Temperature Measurement value 2,*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HRS_HEART_RATE_MEASUREMENT),
            HI_WORD(GATT_UUID_CHAR_HRS_HEART_RATE_MEASUREMENT)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_NOTIF_IND/* wPermissions */
    },

    /* client characteristic configuration */
    {
        ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL,                   /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            /* NOTE: this value has an instantiation for each client, a write to */
            /* this attribute does not modify this default value:                */
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* wPermissions */
    }
#ifdef HRS_BODY_SENSOR_LOCATION_CHAR_SUPPORT
    ,
    /* <<Characteristic>>Body Sensor Location*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ/* characteristic properties */
            )
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HRS_BODY_SENSOR_LOCATION),
            HI_WORD(GATT_UUID_CHAR_HRS_BODY_SENSOR_LOCATION)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    }
#endif

#ifdef HRS_ENERGY_EXPENDED_FEATURE_SUPPORT
    ,
    /* <<Characteristic>> Heart Rate Control Point*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_WRITE/* characteristic properties */
            )
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HRS_HEART_RATE_CP),
            HI_WORD(GATT_UUID_CHAR_HRS_HEART_RATE_CP)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_WRITE/* wPermissions */
    }
#endif
};

/**< @brief  Heart Rate service size definition.  */
const uint16_t hrs_attr_tbl_size = sizeof(hrs_attr_tbl);

bool hrs_body_sensor_location_read_confirm(uint8_t conn_id, T_SERVER_ID service_id,
                                           uint8_t hrs_body_sensor_location, T_APP_RESULT cause)
{
    return server_attr_read_confirm(conn_id, service_id, HRS_BODY_SENSOR_LOCATION_VALUE_INDEX,
                                    &hrs_body_sensor_location, sizeof(uint8_t), cause);
}

bool hrs_heart_rate_measurement_value_notify(uint8_t conn_id, T_SERVER_ID service_id,
                                             T_HEART_RATE_MEASUREMENT_VALUE heart_rate_measurement_value)
{
    uint16_t value_len = sizeof(uint8_t);  /* The length of heart_rate_measurement_value.flag */
    uint16_t mtu_size = 23;
    bool ret = false;

    le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu_size, conn_id);

    if (heart_rate_measurement_value.flag.heart_rate_value_format_bit ==
        HEART_RATE_VALUE_FORMAT_UINT8)
    {
        value_len += sizeof(uint8_t);
    }
    else
    {
        value_len += sizeof(uint16_t);
    }

    if (heart_rate_measurement_value.flag.energy_expended_status_bit)
    {
        value_len += sizeof(uint16_t);
    }

    if (heart_rate_measurement_value.flag.rr_interval_bit)
    {
        value_len += heart_rate_measurement_value.rr_interval_len;
    }


    if (value_len > mtu_size - 3)
    {
        PROFILE_PRINT_ERROR1("hrs_heart_rate_measurement_value_notify: value_len %d is too long! value_len",
                             value_len);
        return false;
    }

    uint8_t *p_heart_rate_measurement_value = os_mem_zalloc(RAM_TYPE_DATA_ON, value_len);

    if (p_heart_rate_measurement_value)
    {
        uint16_t cur_offset = 0;
        memcpy(p_heart_rate_measurement_value, &heart_rate_measurement_value.flag, 1);

        cur_offset += 1;

        if (heart_rate_measurement_value.flag.heart_rate_value_format_bit ==
            HEART_RATE_VALUE_FORMAT_UINT8)
        {
            memcpy(&p_heart_rate_measurement_value[cur_offset],
                   &heart_rate_measurement_value.heart_rate_measurement_value, 1);
            cur_offset += 1;
        }
        else
        {
            memcpy(&p_heart_rate_measurement_value[cur_offset],
                   &heart_rate_measurement_value.heart_rate_measurement_value, 2);
            cur_offset += 2;
        }

        if (heart_rate_measurement_value.flag.energy_expended_status_bit)
        {
            memcpy(&p_heart_rate_measurement_value[cur_offset],
                   &heart_rate_measurement_value.energy_expended, 2);
            cur_offset += 2;
        }

        if (heart_rate_measurement_value.flag.rr_interval_bit)
        {
            memcpy(&p_heart_rate_measurement_value[cur_offset],
                   heart_rate_measurement_value.p_rr_interval,
                   heart_rate_measurement_value.rr_interval_len);
        }

        PROFILE_PRINT_INFO0("hrs_heart_rate_measurement_value_notify");
        ret = server_send_data(conn_id, service_id, HRS_HEART_RATE_MEASUREMENT_VALUE_INDEX,
                               p_heart_rate_measurement_value, value_len, GATT_PDU_TYPE_NOTIFICATION);

        os_mem_free(p_heart_rate_measurement_value);
    }
    else
    {
        PROFILE_PRINT_ERROR0("hrs_heart_rate_measurement_value_notify: alloc buf fail");
    }

    return ret;
}

/**
 * @brief  handle control point write (request).
 *
 * @param[in] conn_id       Connection id.
 * @param[in] service_id    Service ID.
 * @param write_length      Write request data length.
 * @param value_ptr         Pointer to write request data.
 * @return none
 * @retval  void
*/
static T_APP_RESULT hrs_hanlde_ctl_pnt_proc(uint8_t conn_id, T_SERVER_ID service_id,
                                            uint16_t write_length, uint8_t *p_value)
{
    T_HRS_CALLBACK_DATA callback_data;
    T_APP_RESULT cause  = APP_RESULT_SUCCESS;
    uint16_t parameter_length = 0;
    memcpy(hrs_ctl_pnt.value, p_value, write_length);
    if (write_length >= 1)
    {
        parameter_length = write_length - 1;
    }

    PROFILE_PRINT_INFO1("hrs_hanlde_ctl_pnt_proc request: OpCode=0x%x", hrs_ctl_pnt.value[0]);

    callback_data.msg_type = SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE;
    callback_data.msg_data.write.opcode = (T_HRS_HEART_RATE_CP_OPCODE)hrs_ctl_pnt.value[0];


    switch (hrs_ctl_pnt.value[0])
    {
    case HRS_HEART_RATE_CP_OPCODE_RESET_ENERGY_EXPENDED:
        {
            if (!hrs_notify_indicate_flag.heart_rate_measurement_notify_enable)
            {
                cause = APP_RESULT_PROC_ALREADY_IN_PROGRESS;
            }
            else if (parameter_length != 0)
            {
                cause = APP_RESULT_INVALID_PDU;
            }
        }
        break;


    default:
        {
            cause = APP_RESULT_APP_ERR;
        }
        break;
    }

    pfn_hrs_app_cb(service_id, (void *)&callback_data);
    return cause;
}


/**
 * @brief read characteristic data from service.
 *
 * @param[in] conn_id       Connection id.
 * @param[in] service_id    Service ID.
 * @param[in] attrib_index          Attribute index of getting characteristic data.
 * @param[in] offset                Used for Blob Read.
 * @param[in,out] p_length            length of getting characteristic data.
 * @param[in,out] pp_value            data got from service.
 * @return Profile procedure result
*/
T_APP_RESULT hrs_attr_read_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t attrib_index,
                              uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause  = APP_RESULT_SUCCESS;

    PROFILE_PRINT_INFO2("hrs_attr_read_cb iAttribIndex = %d iOffset %x", attrib_index, offset);

    *p_length = 0;

    switch (attrib_index)
    {
    case HRS_BODY_SENSOR_LOCATION_VALUE_INDEX:
        {
            T_HRS_CALLBACK_DATA callback_data;
            callback_data.msg_type = SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE;
            callback_data.msg_data.read_value_index = HRS_READ_BODY_SENSOR_LOCATION_VALUE;
            cause = pfn_hrs_app_cb(service_id, (void *)&callback_data);

            if (cause == APP_RESULT_SUCCESS)
            {
                cause = APP_RESULT_PENDING;
            }
        }
        break;

    default:
        {
            PROFILE_PRINT_ERROR1("hrs_attr_read_cb iAttribIndex = %d not found", attrib_index);
            cause  = APP_RESULT_ATTR_NOT_FOUND;
        }
        break;
    }

    return cause;
}


/**
 * @brief write characteristic data from service.
 *
 * @param[in] conn_id          Connection id.
 * @param[in] service_id          ServiceID to be written.
 * @param[in] attrib_index          Attribute index of characteristic.
 * @param[in] length            length of value to be written.
 * @param[in] p_value            value to be written.
 * @return Profile procedure result
*/
T_APP_RESULT hrs_attr_write_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t attrib_index,
                               T_WRITE_TYPE write_type,
                               uint16_t length, uint8_t *p_value, P_FUN_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
    T_APP_RESULT  cause  = APP_RESULT_SUCCESS;
    PROFILE_PRINT_INFO2("hrs_attr_write_cb  iAttribIndex = %d wLength %x", attrib_index, length);

    switch (attrib_index)
    {
    case HRS_HEART_RATE_CP_VALUE_INDEX:
        {
            /* Attribute value has variable size, make sure written value size is valid. */
            if ((length > HRS_MAX_CTL_PNT_VALUE) || (p_value == NULL))
            {
                cause  = APP_RESULT_INVALID_VALUE_SIZE;
            }
            /* Make sure Control Point is not "Process already in progress". */
            else if (HRS_CTL_PNT_OPERATE_ACTIVE(hrs_ctl_pnt.value[0]))
            {
                cause  = APP_RESULT_PROC_ALREADY_IN_PROGRESS;
            }
            /* Make sure Control Point is configured indication enable. */
            else if (!hrs_notify_indicate_flag.heart_rate_measurement_notify_enable)
            {
                cause = APP_RESULT_CCCD_IMPROPERLY_CONFIGURED;
            }
            else
            {
                cause = hrs_hanlde_ctl_pnt_proc(conn_id, service_id, length, p_value);
            }

        }
        break;

    default:
        {
            PROFILE_PRINT_ERROR1("hrs_attr_write_cb  iAttribIndex = %d not found", attrib_index);
            cause  = APP_RESULT_ATTR_NOT_FOUND;
        }
        break;
    }
    hrs_ctl_pnt.value[0] = HRS_HEART_RATE_CP_OPCODE_RESERVED;

    return cause;
}

/**
 * @brief update CCCD bits from stack.
 *
 * @param[in] conn_id       Connection id.
 * @param[in] service_id          Service ID.
 * @param[in] index          Attribute index of characteristic data.
 * @param[in] ccc_bits         CCCD bits from stack.
 * @return None
*/
void hrs_cccd_update_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t index, uint16_t ccc_bits)
{
    T_HRS_CALLBACK_DATA hrs_callback_data;
    bool bHandle = true;

    PROFILE_PRINT_INFO2("hrs_cccd_update_cb  Index = %d wCCCDBits %x", index, ccc_bits);

    switch (index)
    {
    case HRS_HEART_RATE_MEASUREMENT_CCCD_INDEX:
        {
            if (ccc_bits & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
            {
                // Enable Notification
                hrs_notify_indicate_flag.heart_rate_measurement_notify_enable = 1;
                hrs_callback_data.msg_data.notification_indification_index =
                    HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_ENABLE;
            }
            else
            {
                hrs_notify_indicate_flag.heart_rate_measurement_notify_enable = 0;
                hrs_callback_data.msg_data.notification_indification_index =
                    HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_DISABLE;
            }
        }
        break;


    default:
        {
            bHandle = false;
            PROFILE_PRINT_ERROR1("hrs_cccd_update_cb  Index = %d not found", index);
        }
        break;

    }
    /* Notify Application. */
    if (pfn_hrs_app_cb && (bHandle == true))
    {
        pfn_hrs_app_cb(service_id, (void *)&hrs_callback_data);
    }

    return;
}

/**
 * @brief Heart Rate Service Callbacks.
*/
const T_FUN_GATT_SERVICE_CBS hrs_cbs =
{
    hrs_attr_read_cb,  // Read callback function pointer
    hrs_attr_write_cb, // Write callback function pointer
    hrs_cccd_update_cb  // CCCD update callback function pointer
};

T_SERVER_ID hrs_add_service(void *p_func)
{
    T_SERVER_ID service_id;
    if (false == server_add_service(&service_id,
                                    (uint8_t *)hrs_attr_tbl,
                                    hrs_attr_tbl_size,
                                    hrs_cbs))
    {
        PROFILE_PRINT_ERROR1("hrs_add_service: ServiceId %d", service_id);
        service_id = 0xff;
    }
    pfn_hrs_app_cb = (P_FUN_SERVER_GENERAL_CB)p_func;

    return service_id;
}


