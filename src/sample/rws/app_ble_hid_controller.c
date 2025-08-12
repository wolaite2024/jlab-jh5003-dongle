/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     app_ble_hid_controller.c
  * @brief    Source file for using Human Interface Device Service.
  * @details  Global data and function implement.
  * @author   Bill Zhao
  * @date     2025-02-26
  * @version  v1.0
  * *************************************************************************************
  */
#if F_APP_BLE_HID_CONTROLLER_SUPPORT
#include <string.h>
#include "trace.h"
#include "profile_server.h"
#include "profile_server_ext.h"
#include "app_ble_hid_controller.h"
#include "ble_ext_adv.h"
#include "app_link_util.h"
#include "app_ble_rand_addr_mgr.h"
#include "gap.h"
#include "gap_conn_le.h"
#include "app_cfg.h"
#include "app_dongle_dual_mode.h"
#include "app_bt_policy_api.h"
#include "app_timer.h"

#define GATT_UUID_HID                           0x1812
#define GATT_UUID_CHAR_REPORT                   0x2A4D
#define GATT_UUID_CHAR_REPORT_MAP               0x2A4B
#define GATT_UUID_CHAR_HID_INFO                 0x2A4A
#define GATT_UUID_CHAR_HID_CONTROL_POINT        0x2A4C

#define BLE_HID_REPORT_ID_GAME_PAD_INPUT        1
#define BLE_HID_REPORT_ID_GAME_PAD_OUTPUT       2

#define HID_FAST_ADV_INTERVAL                   0x30
#define HID_SLOW_ADV_INTERVAL                   0x140
#define HID_SLOW_ADV_TIMEOUT                    180

typedef enum
{
    APP_TIMER_SLOW_HID_ADV            = 0x00,
} T_APP_BLE_HID_TIMER;

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
static P_FUN_EXT_SERVER_GENERAL_CB pfn_hids_cb = NULL;
#else
static P_FUN_SERVER_GENERAL_CB pfn_hids_cb = NULL;
#endif

T_HID_INFO hid_info = {0, 0, 0};
uint8_t hid_suspand_mode = 0;
static uint8_t ble_controller_adv_handle = 0xff;
static T_SERVER_ID ble_controller_hid_service_id = 0xff;
static uint8_t ble_controller_conn_id = 0xff;
static uint8_t app_ble_hid_timer_id = 0;
static uint8_t timer_idx_slow_hid_adv_interval = 0;

static uint8_t ble_controller_adv_data[GAP_MAX_LEGACY_ADV_LEN] =
{
    /*Flags*/
    0x02,
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    /*Appearance*/
    0x03,
    GAP_ADTYPE_APPEARANCE,
    LO_WORD(GAP_GATT_APPEARANCE_GAMEPAD),
    HI_WORD(GAP_GATT_APPEARANCE_GAMEPAD),

    /*Complete List of 16-bit Service Class UUIDs*/
    0x03,
    GAP_ADTYPE_16BIT_COMPLETE,
    LO_WORD(GATT_UUID_HID),
    HI_WORD(GATT_UUID_HID),

    /*Complete Local Name*/
    0x0F,
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    0x52, 0x54, 0x4B, 0x20, 0x43, 0x6F, 0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x6C, 0x65, 0x72
};

// game controller descriptor
const uint8_t hids_report_descriptor[] =
{
    0x05, 0x01,             /* USAGE_PAGE       (Generic Desktop Controls)      */ \
    0x09, 0x05,             /* USAGE            (Game Pad)                      */ \
    0xA1, 0x01,             /* COLLECTION       (Application)           */ \

    0x85, BLE_HID_REPORT_ID_GAME_PAD_INPUT,             /* REPORT_ID        (0x10)                  */ \

    0x09, 0x30,             /* Usage (X)                                        */ \
    0x09, 0x31,             /* Usage (Y)                                        */ \
    0x09, 0x32,             /* Usage (Z)                                        */ \
    0x09, 0x35,             /* Usage (Rz)                                       */ \
    0x09, 0x33,             /* Usage (Rx)                                       */ \
    0x09, 0x34,             /* Usage (Ry)                                       */ \
    0x15, 0x00,             /* LOGICAL_MINIMUM  (0)                     */ \
    0x26, 0xFF, 0x00,       /* LOGICAL_MAXIMUM  (0xff)                  */ \
    0x75, 0x08,             /* REPORT_SIZE      (8)                     */ \
    0x95, 0x06,             /* REPORT_COUNT     (6)                     */ \
    0x81, 0x02,             /* INPUT            (Data,Var,Abs)          */ \

    0x06, 0x00, 0xFF,       /* USAGE            (vendor)                         */ \
    0x09, 0x20,             /* Usage (0x20)                                     */ \
    0x75, 0x08,             /* REPORT_SIZE      (8)                     */ \
    0x95, 0x01,             /* REPORT_COUNT     (1)                     */ \
    0x81, 0x02,             /* INPUT            (Data,Var,Abs)          */ \

    0x05, 0x01,             /* USAGE_PAGE       (Generic Desktop Controls)      */ \
    0x09, 0x39,             /* USAGE            (Hat switc)                      */ \
    0x15, 0x00,             /* LOGICAL_MINIMUM  (0)                     */ \
    0x25, 0x07,             /* LOGICAL_MAXIMUM  (0x07)                  */ \
    0x35, 0x00,             /* PHYSICAL_MINIMUM  (0)                     */ \
    0x46, 0x3B, 0x01,       /* PHYSICAL_MAXIMUM  (0x013B)                  */ \
    0x65, 0x14,             /* UNIT  (0x14)                               */ \
    0x75, 0x04,             /* REPORT_SIZE      (4)                     */ \
    0x95, 0x01,             /* REPORT_COUNT     (1)                     */ \
    0x81, 0x42,             /* INPUT(Data, Variable, Absolute, No Wrap, Linear, Preferred State, Null State, Bit Field)*/ \
    0x65, 0x00,             /* UNIT  (0x0)                               */ \

    0x05, 0x09,             /* USAGE            (Button)                         */ \
    0x19, 0x01,             /* USAGE_MINIMUM    (1)                     */ \
    0x29, 0x0F,             /* USAGE_MAXIMUM    (0x0F)                  */ \
    0x15, 0x00,             /* LOGICAL_MINIMUM  (0)                     */ \
    0x25, 0x01,             /* LOGICAL_MAXIMUM  (0x01)                  */ \
    0x75, 0x01,             /* REPORT_SIZE      (1)                     */ \
    0x95, 0x0F,             /* REPORT_COUNT     (F)                     */ \
    0x81, 0x02,             /* INPUT            (Data,Var,Abs)          */ \

    0x95, 0x05,             /* REPORT_COUNT     (5)                     */ \
    0x81, 0x01,             /* INPUT            (Data,Var,Abs)          */ \

    0xC0                    /* END_COLLECTION                           */
};

static const T_ATTRIB_APPL hids_attr_tbl[] =
{
    /* <<Primary Service>>, .. 0*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /* wFlags     */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_HID),               /* service UUID */
            HI_WORD(GATT_UUID_HID)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* pValueContext */
        GATT_PERM_READ                              /* wPermissions  */
    },

    /* <<Characteristic>>, .. 1*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ,                    /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /*  HID Information characteristic value .. 2*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HID_INFO),
            HI_WORD(GATT_UUID_CHAR_HID_INFO)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /* <<Characteristic>>, .. 3*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_WRITE_NO_RSP,                      /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /*  HID controlPoint characteristic value .. 4*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HID_CONTROL_POINT),
            HI_WORD(GATT_UUID_CHAR_HID_CONTROL_POINT)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ | GATT_PERM_WRITE            /* wPermissions */
    },

    /* <<Characteristic>>, .. 5*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ,                  /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /*  HID report map characteristic value .. 6*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_MAP),
            HI_WORD(GATT_UUID_CHAR_REPORT_MAP)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /* <<Characteristic>>, .. 7*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_NOTIFY,           /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 8*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT),
            HI_WORD(GATT_UUID_CHAR_REPORT)
        },
        0,                                        /* variable size */
        (void *)NULL,
        GATT_PERM_READ | GATT_PERM_WRITE /* wPermissions */
    },

    /* client characteristic configuration .. 9*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL),                  /* wFlags */
        {                                         /* bTypeValue */
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
    },

    /*report ID map reference descriptor .. 10*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL),                  /* wFlags */
        {                                         /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            HI_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            BLE_HID_REPORT_ID_GAME_PAD_INPUT,
            HID_INPUT_TYPE,
        },
        2,                                          /* bValueLen */
        NULL,//(void*)&cPointerInputReportIdMap,
        (GATT_PERM_READ)        /* wPermissions */
    },

    /* <<Characteristic>>, .. 11*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                         /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE | GATT_CHAR_PROP_WRITE_NO_RSP,   /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 12*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                         /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT),
            HI_WORD(GATT_UUID_CHAR_REPORT)
        },
        0,                                        /* variable size */
        (void *)NULL,
        GATT_PERM_READ | GATT_PERM_WRITE                          /* wPermissions */
    },

    /*report ID map reference descriptor .. 13*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL),                   /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            HI_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            BLE_HID_REPORT_ID_GAME_PAD_OUTPUT,
            HID_OUTPUT_TYPE
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ)          /* wPermissions */
    }
};

/**
 * @brief       Set a HID service parameter.
 *
 *              NOTE: You can call this function with a HID service parameter type and it will set the
 *                      HID service parameter.  HID service parameters are defined in @ref T_HIDS_PARAM_TYPE.
 *
 * @param[in]   param_type  HID service parameter type: @ref T_HIDS_PARAM_TYPE
 * @param[in]   len         Length of data to write
 * @param[in]   p_value Pointer to data to write.  This is dependent on
 *                      the parameter type and WILL be cast to the appropriate
 *                      data type
 *
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t mode = 1;
        hids_set_parameter(HID_PROTOCOL_MODE, 1, &mode);
    }
 * \endcode
 */
bool hids_set_parameter(T_HIDS_PARAM_TYPE param_type, uint8_t length, void *value_ptr)
{
    bool ret = true;

    APP_PRINT_TRACE2("hids_set_parameter: param_type 0x%x, value %b", param_type, TRACE_BINARY(length,
                     value_ptr));

    switch (param_type)
    {
    case HID_REPORT_INPUT:
        break;

    case HID_REPORT_OUTPUT:
        break;

    case HID_REPORT_FEATURE:
        break;

    case HID_REPORT_MAP:
        break;

    case HID_INFO:
        {
            memcpy((void *)&hid_info, value_ptr, length);
        }
        break;

    case HID_CONTROL_POINT:
        hid_suspand_mode = *((uint8_t *)value_ptr);
        break;

    default:
        ret = false;
        break;
    }
    return ret;
}

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
static T_APP_RESULT hids_attr_read_cb(uint16_t conn_handle, uint16_t cid,
                                      T_SERVER_ID service_id,
                                      uint16_t attrib_index,
                                      uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    T_HID_CALLBACK_DATA callback_data;
    callback_data.conn_handle = conn_handle;
    callback_data.cid = cid;

    uint8_t conn_id;
    le_get_conn_id_by_handle(conn_handle, &conn_id);

    callback_data.conn_id = conn_id;
    callback_data.msg_type = SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE;

    APP_PRINT_TRACE1("hids_attr_read_cb: attrib_index %d", attrib_index);

    switch (attrib_index)
    {
    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;

    case GATT_SVC_HID_REPORT_INPUT_INDEX:
        break;

    case GATT_SVC_HID_REPORT_OUTPUT_INDEX:
        break;

    case GATT_SVC_HID_REPORT_MAP_INDEX:
        *pp_value = (uint8_t *)hids_report_descriptor;
        *p_length = sizeof(hids_report_descriptor);
        break;

    case GATT_SVC_HID_INFO_INDEX:
        callback_data.msg_data.read_value_index = GATT_SVC_HID_INFO_INDEX;
        cause = pfn_hids_cb(service_id, (void *)&callback_data);
        *pp_value = (uint8_t *)&hid_info;
        *p_length = sizeof(hid_info);
        break;

    case GATT_SVC_HID_CONTROL_POINT_INDEX:
        break;
    }

    return cause;
}

static T_APP_RESULT hids_attr_write_cb(uint16_t conn_handle, uint16_t cid,
                                       T_SERVER_ID service_id,
                                       uint16_t attrib_index, T_WRITE_TYPE write_type,
                                       uint16_t length, uint8_t *p_value, P_FUN_EXT_WRITE_IND_POST_PROC *p_write_post_proc)
{
    T_APP_RESULT cause  = APP_RESULT_SUCCESS;
    T_HID_CALLBACK_DATA callback_data;
    callback_data.msg_type = SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE;

    if (!p_value)
    {
        cause = APP_RESULT_INVALID_PDU;
        return cause;
    }

    switch (attrib_index)
    {
    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;

    case GATT_SVC_HID_REPORT_INPUT_INDEX:
        break;

    case GATT_SVC_HID_REPORT_OUTPUT_INDEX:
        break;

    case GATT_SVC_HID_REPORT_MAP_INDEX:
        break;

    case GATT_SVC_HID_INFO_INDEX:
        break;

    case GATT_SVC_HID_CONTROL_POINT_INDEX:
        break;
    }

    if (pfn_hids_cb && (cause == APP_RESULT_SUCCESS))
    {
        pfn_hids_cb(service_id, (void *)&callback_data);
    }

    return cause;

}

void hids_cccd_update_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                         uint16_t attrib_index, uint16_t ccc_bits)
{
    bool cause = true;
    T_HID_CALLBACK_DATA callback_data;
    callback_data.conn_handle = conn_handle;
    callback_data.cid = cid;

    uint8_t conn_id;
    le_get_conn_id_by_handle(conn_handle, &conn_id);
    callback_data.conn_id = conn_id;
    callback_data.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;

    PROFILE_PRINT_INFO2("hids_cccd_update_cb index = %d ccc_bits %x", attrib_index, ccc_bits);

    switch (attrib_index)
    {
    default:
        cause = false;
        break;

    case GATT_SVC_HID_REPORT_INPUT_CCCD_INDEX:
        {
            callback_data.msg_data.not_ind_data.index = GATT_SVC_HID_REPORT_INPUT_CCCD_INDEX;
            if (ccc_bits & GATT_PDU_TYPE_NOTIFICATION)
            {
                callback_data.msg_data.not_ind_data.value = NOTIFY_ENABLE;
            }
            else
            {
                callback_data.msg_data.not_ind_data.value = NOTIFY_DISABLE;
            }
            break;
        }
    }

    if (pfn_hids_cb && (cause == true))
    {
        pfn_hids_cb(service_id, (void *)&callback_data);
    }

    return;
}
#else
static T_APP_RESULT hids_attr_read_cb(uint8_t conn_id, T_SERVER_ID service_id,
                                      uint16_t attrib_index,
                                      uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    T_HID_CALLBACK_DATA callback_data;
    callback_data.msg_type = SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE;
    callback_data.conn_id = conn_id;

    callback_data.msg_type = SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE;

    switch (attrib_index)
    {
    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;

    case GATT_SVC_HID_REPORT_INPUT_INDEX:
        break;

    case GATT_SVC_HID_REPORT_OUTPUT_INDEX:
        break;

    case GATT_SVC_HID_REPORT_MAP_INDEX:
        *pp_value = (uint8_t *)hids_report_descriptor;
        *p_length = sizeof(hids_report_descriptor);
        break;

    case GATT_SVC_HID_INFO_INDEX:
        callback_data.msg_data.read_value_index = GATT_SVC_HID_INFO_INDEX;
        cause = pfn_hids_cb(service_id, (void *)&callback_data);
        *pp_value = (uint8_t *)&hid_info;
        *p_length = sizeof(hid_info);
        break;

    case GATT_SVC_HID_CONTROL_POINT_INDEX:
        break;
    }

    return cause;
}


static T_APP_RESULT hids_attr_write_cb(uint8_t conn_id, T_SERVER_ID service_id,
                                       uint16_t attrib_index, T_WRITE_TYPE write_type,
                                       uint16_t length, uint8_t *p_value, P_FUN_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
    T_APP_RESULT cause  = APP_RESULT_SUCCESS;
    T_HID_CALLBACK_DATA callback_data;
    callback_data.msg_type = SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE;

    if (!p_value)
    {
        cause = APP_RESULT_INVALID_PDU;
        return cause;
    }

    switch (attrib_index)
    {
    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;

    case GATT_SVC_HID_PROTOCOL_MODE_INDEX:
        callback_data.msg_data.write_msg.write_type = write_type;
        callback_data.msg_data.write_msg.write_parameter.protocol_mode = *p_value;
        hids_set_parameter(HID_PROTOCOL_MODE, length, p_value);
        break;

    case GATT_SVC_HID_REPORT_INPUT_INDEX:
        break;

    case GATT_SVC_HID_REPORT_OUTPUT_INDEX:
        break;

    case GATT_SVC_HID_REPORT_FEATURE_INDEX:
        break;

    case GATT_SVC_HID_REPORT_MAP_INDEX:
        break;

    case GATT_SVC_HID_BOOT_MS_IN_REPORT_INDEX:
        break;

    case GATT_SVC_HID_INFO_INDEX:
        break;

    case GATT_SVC_HID_CONTROL_POINT_INDEX:
        break;
    }

    if (pfn_hids_cb && (cause == APP_RESULT_SUCCESS))
    {
        pfn_hids_cb(service_id, (void *)&callback_data);
    }

    return cause;

}

void hids_cccd_update_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t index, uint16_t ccc_bits)
{
    bool cause = true;
    T_HID_CALLBACK_DATA callback_data;
    callback_data.conn_id = conn_id;
    callback_data.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;

    PROFILE_PRINT_INFO2("hids_cccd_update_cb index = %d ccc_bits %x", index, ccc_bits);

    switch (index)
    {
    default:
        cause = false;
        break;

    case GATT_SVC_HID_REPORT_INPUT_CCCD_INDEX:
        {
            callback_data.msg_data.not_ind_data.index = GATT_SVC_HID_REPORT_INPUT_CCCD_INDEX;
            if (ccc_bits & GATT_PDU_TYPE_NOTIFICATION)
            {
                callback_data.msg_data.not_ind_data.value = NOTIFY_ENABLE;
            }
            else
            {
                callback_data.msg_data.not_ind_data.value = NOTIFY_DISABLE;
            }
            break;
        }

    case GATT_SVC_HID_BOOT_MS_IN_REPORT_CCCD_INDEX:
        {
            callback_data.msg_data.not_ind_data.index = GATT_SVC_HID_BOOT_MS_IN_REPORT_CCCD_INDEX;
            if (ccc_bits & GATT_PDU_TYPE_NOTIFICATION)
            {
                callback_data.msg_data.not_ind_data.value = NOTIFY_ENABLE;
            }
            else
            {
                callback_data.msg_data.not_ind_data.value = NOTIFY_DISABLE;
            }
            break;
        }
    }

    if (pfn_hids_cb && (cause == true))
    {
        pfn_hids_cb(service_id, (void *)&callback_data);
    }

    return;
}
#endif

/**
 * @brief       Send HIDS notification data .
 *
 *
 * @param[in]   conn_id  Connection id.
 * @param[in]   service_id  Service id.
 * @param[in]   index  hids characteristic index.
 * @param[in]   p_data report value pointer.
 * @param[in]   data_len length of report data.
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t conn_id = 0;
        T_SERVER_ID service_id = hids_id;
        uint8_t hid_report_input[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
        hids_send_report(conn_id, service_id, GATT_SVC_HID_REPORT_INPUT_INDEX, hid_report_input, sizeof(hid_report_input));
    }
 * \endcode
 */
bool hids_send_report(uint8_t conn_id, T_SERVER_ID service_id, uint16_t index, uint8_t *p_data,
                      uint16_t data_len)
{
    PROFILE_PRINT_INFO1("hids_send_report data_len %d", data_len);

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    return server_ext_send_data(le_get_conn_handle(conn_id), L2C_FIXED_CID_ATT, service_id,
                                index, p_data, data_len, GATT_PDU_TYPE_ANY);
#else
    return server_send_data(conn_id, service_id, index, p_data, data_len, GATT_PDU_TYPE_NOTIFICATION);
#endif
}

void app_ble_controller_send_data(uint8_t *data, uint16_t data_len)
{
    APP_PRINT_TRACE3("app_ble_controller_send_data: conn_id %d, service_id %d, data %b",
                     ble_controller_conn_id, ble_controller_hid_service_id, TRACE_BINARY(data_len, data));

    hids_send_report(ble_controller_conn_id, ble_controller_hid_service_id,
                     GATT_SVC_HID_REPORT_INPUT_INDEX, data, data_len);
}

#if BLE_CONTROLLER_TEST
void app_ble_controller_test(void)
{
    static uint8_t s_cnt = 0;
    s_cnt++;

    BLE_CONTROLLER_HID_INPUT_REPORT gd_data;
    memset(&gd_data, 0, sizeof(BLE_CONTROLLER_HID_INPUT_REPORT));

    if (s_cnt == 1)
    {
        gd_data.X = 0x82;
        gd_data.Y = 0x80;
        gd_data.Z = 0x7F;
        gd_data.Rz = 0x7F;
        gd_data.hat_switch = 7;
        gd_data.button_1 = 1;
    }
    else if (s_cnt == 2)
    {
        gd_data.X = 0x70;
        gd_data.Y = 0x60;
        gd_data.Z = 0x7;
        gd_data.Rz = 0x7F;
        gd_data.Rx = 0x02;
        gd_data.Ry = 0x08;
        gd_data.hat_switch = 6;
        gd_data.button_3 = 1;
        gd_data.button_5 = 1;
    }
    else
    {
        gd_data.X = 0x0;
        gd_data.Y = 0x0;
        gd_data.Z = 0x88;
        gd_data.Rz = 0x72;
        gd_data.Rx = 0x72;
        gd_data.Ry = 0x78;
        s_cnt = 0;
    }
    app_ble_controller_send_data((uint8_t *)&gd_data, sizeof(gd_data));
}
#endif

uint16_t hids_attr_tbl_len = sizeof(hids_attr_tbl);

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
const T_FUN_GATT_EXT_SERVICE_CBS hids_cbs =
#else
const T_FUN_GATT_SERVICE_CBS hids_cbs =
#endif
{
    hids_attr_read_cb,      // Read callback function pointer
    hids_attr_write_cb,     // Write callback function pointer
    hids_cccd_update_cb,    // Authorization callback function pointer
};

/**
  * @brief       Add HID service to the BLE stack database.
  *
  *
  * @param[in]   p_func  Callback when service attribute was read, write or cccd update.
  * @return Service id generated by the BLE stack: @ref T_SERVER_ID.
  * @retval 0xFF Operation failure.
  * @retval Others Service id assigned by stack.
  *
  * <b>Example usage</b>
  * \code{.c}
     void profile_init()
     {
         server_init(1);
         hids_id = hids_add_service(app_handle_profile_message);
     }
  * \endcode
  */
T_SERVER_ID hids_add_service(void *p_func)
{
    T_SERVER_ID service_id;

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    if (false == server_ext_add_service(&service_id,
                                        (uint8_t *)hids_attr_tbl,
                                        hids_attr_tbl_len,
                                        &hids_cbs))
    {
        DBG_DIRECT("hids_add_service: 1 fail, srv_id %d", service_id);
        service_id = 0xff;
    }

    pfn_hids_cb = (P_FUN_EXT_SERVER_GENERAL_CB)p_func;
#else
    if (false == server_add_service(&service_id, (uint8_t *)hids_attr_tbl, hids_attr_tbl_len, hids_cbs))
    {
        DBG_DIRECT("hids_add_service: 2 fail, ServiceId %d", service_id);
        service_id = 0xff;
    }

    pfn_hids_cb = (P_FUN_SERVER_GENERAL_CB)p_func;
#endif

    ble_controller_hid_service_id = service_id;

    DBG_DIRECT("hids_add_service: ServiceId %d", service_id);

    return service_id;
}

void app_ble_controller_le_disconnect_cb(uint8_t conn_id, uint8_t local_disc_cause,
                                         uint16_t disc_cause)
{
    APP_PRINT_TRACE3("app_ble_controller_le_disconnect_cb: conn_id %d, local_disc_cause %d, disc_cause 0x%x",
                     conn_id, local_disc_cause, disc_cause);

    if (app_cfg_nv.dongle_rf_mode == DONGLE_RF_MODE_BT)
    {
        app_ble_controller_adv_start(false);
    }
}

static void app_ble_controller_adv_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BLE_EXT_ADV_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BLE_EXT_ADV_CB_DATA));

    switch (cb_type)
    {
    case BLE_EXT_ADV_STATE_CHANGE:
        {
            if (cb_data.p_ble_state_change->state == BLE_EXT_ADV_MGR_ADV_ENABLED)
            {
                APP_PRINT_TRACE1("app_ble_controller_adv_callback: BLE_EXT_ADV_MGR_ADV_ENABLED adv_handle %d",
                                 cb_data.p_ble_state_change->adv_handle);
            }
            else if (cb_data.p_ble_state_change->state == BLE_EXT_ADV_MGR_ADV_DISABLED)
            {
                APP_PRINT_TRACE3("app_ble_controller_adv_callback: BLE_EXT_ADV_MGR_ADV_DISABLED adv_handle %d, stop_cause %d, app_cause 0x%02x",
                                 cb_data.p_ble_state_change->adv_handle,
                                 cb_data.p_ble_state_change->stop_cause,
                                 cb_data.p_ble_state_change->app_cause);
            }
        }
        break;

    case BLE_EXT_ADV_SET_CONN_INFO:
        {
            APP_PRINT_TRACE1("app_ble_controller_adv_callback: BLE_EXT_ADV_SET_CONN_INFO conn_id 0x%x",
                             cb_data.p_ble_conn_info->conn_id);
            app_link_reg_le_link_disc_cb(cb_data.p_ble_conn_info->conn_id, app_ble_controller_le_disconnect_cb);
            ble_controller_conn_id = cb_data.p_ble_conn_info->conn_id;

            app_audio_tone_type_play(TONE_LINK_CONNECTED, false, true);

            app_bt_policy_exit_pairing_mode();
        }
        break;

    default:
        break;
    }
    return;
}

static void app_ble_controller_gen_adv_data(bool is_swift_pair_adv)
{
    uint8_t idx = 0;

    memset(ble_controller_adv_data, 0, GAP_MAX_LEGACY_ADV_LEN);

    /*Flags*/
    ble_controller_adv_data[idx++] = 0x02;
    ble_controller_adv_data[idx++] = GAP_ADTYPE_FLAGS;

    if (is_swift_pair_adv)
    {
        ble_controller_adv_data[idx++] = GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
    }
    else
    {
        ble_controller_adv_data[idx++] = GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
    }

    /*Appearance*/
    ble_controller_adv_data[idx++] = 0x03;
    ble_controller_adv_data[idx++] = GAP_ADTYPE_APPEARANCE;
    ble_controller_adv_data[idx++] = LO_WORD(GAP_GATT_APPEARANCE_GAMEPAD);
    ble_controller_adv_data[idx++] = HI_WORD(GAP_GATT_APPEARANCE_GAMEPAD);

    if (is_swift_pair_adv)
    {
        ble_controller_adv_data[idx++] = 0x06;
        ble_controller_adv_data[idx++] = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
        ble_controller_adv_data[idx++] = 0x06;
        ble_controller_adv_data[idx++] = 0x00;
        ble_controller_adv_data[idx++] = 0x03;
        ble_controller_adv_data[idx++] = 0x00;
        ble_controller_adv_data[idx++] = 0x80;
    }
    else
    {
        ble_controller_adv_data[idx++] = 0x04;
        ble_controller_adv_data[idx++] = GAP_ADTYPE_MANUFACTURER_SPECIFIC;
        ble_controller_adv_data[idx++] = 0x06;
        ble_controller_adv_data[idx++] = 0x00;
        ble_controller_adv_data[idx++] = 0x00;

        /*Complete List of 16-bit Service Class UUIDs*/
        ble_controller_adv_data[idx++] = 0x03;
        ble_controller_adv_data[idx++] = GAP_ADTYPE_16BIT_COMPLETE;
        ble_controller_adv_data[idx++] = LO_WORD(GATT_UUID_HID);
        ble_controller_adv_data[idx++] = HI_WORD(GATT_UUID_HID);
    }

    uint8_t name_max_len = GAP_MAX_LEGACY_ADV_LEN - idx - 2;

    /*Complete Local Name*/
    ble_controller_adv_data[idx++] = name_max_len + 1;
    ble_controller_adv_data[idx++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&ble_controller_adv_data[idx], app_cfg_nv.device_name_le, name_max_len);
    idx = idx + name_max_len;
}

void app_ble_controller_adv_stop(void)
{
    APP_PRINT_TRACE0("app_ble_controller_adv_stop");

    ble_ext_adv_mgr_disable(ble_controller_adv_handle, 0);
}

void app_ble_controller_adv_start(bool is_swift_pair_adv)
{
    APP_PRINT_TRACE1("app_ble_controller_adv_start: is_swift_pair_adv %d", is_swift_pair_adv);

    app_ble_controller_gen_adv_data(is_swift_pair_adv);

    ble_ext_adv_mgr_set_adv_data(ble_controller_adv_handle,
                                 sizeof(ble_controller_adv_data), (uint8_t *)&ble_controller_adv_data);

    ble_ext_adv_mgr_change_adv_interval(ble_controller_adv_handle, (uint16_t)HID_FAST_ADV_INTERVAL);

    ble_ext_adv_mgr_enable(ble_controller_adv_handle, 0);

    app_start_timer(&timer_idx_slow_hid_adv_interval, "slow_hid_adv_interval",
                    app_ble_hid_timer_id, APP_TIMER_SLOW_HID_ADV, 0, false,
                    HID_SLOW_ADV_TIMEOUT * 1000);
}

static void app_ble_hid_timer_cback(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_SLOW_HID_ADV:
        {
            app_stop_timer(&timer_idx_slow_hid_adv_interval);

            ble_ext_adv_mgr_change_adv_interval(ble_controller_adv_handle, (uint16_t)HID_SLOW_ADV_INTERVAL);
        }
        break;

    default:
        break;
    }
}

void app_ble_controller_adv_init(void)
{
    DBG_DIRECT("app_ble_controller_adv_init");

    T_LE_EXT_ADV_LEGACY_ADV_PROPERTY adv_event_prop = LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint16_t adv_interval_min = HID_FAST_ADV_INTERVAL;
    uint16_t adv_interval_max = HID_FAST_ADV_INTERVAL;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_RANDOM;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[6] = {0, 0, 0, 0, 0, 0};
    T_GAP_ADV_FILTER_POLICY filter_policy = GAP_ADV_FILTER_ANY;
    uint8_t random_addr[6] = {0};

    app_timer_reg_cb(app_ble_hid_timer_cback, &app_ble_hid_timer_id);

    app_ble_rand_addr_get(random_addr);

    app_ble_controller_gen_adv_data(false);

    ble_ext_adv_mgr_init_adv_params(&ble_controller_adv_handle, adv_event_prop, adv_interval_min,
                                    adv_interval_max, own_address_type, peer_address_type, peer_address,
                                    filter_policy, sizeof(ble_controller_adv_data), ble_controller_adv_data, 0, NULL, random_addr);

    if (ble_ext_adv_mgr_register_callback(app_ble_controller_adv_callback,
                                          ble_controller_adv_handle) == GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_INFO1("app_ble_controller_adv_init: ble_controller_adv_handle %d",
                        ble_controller_adv_handle);
    }
}
#endif
