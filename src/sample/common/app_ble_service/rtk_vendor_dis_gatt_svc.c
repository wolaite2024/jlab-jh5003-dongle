

#include "string.h"
#include "trace.h"
#include "rtk_vendor_dis_gatt_svc.h"


const uint8_t GATT_UUID128_RVDIS[16] = { 0xe1, 0x26, 0xd7, 0x98, 0xd0, 0xcd, 0x43, 0xca, 0x98, 0x6d, 0x55, 0x9d, 0x47, 0x76, 0x41, 0x0f};


static P_FUN_RVDIS_SERVER_APP_CB  pfn_rvdis_cb = NULL;

static const T_ATTRIB_APPL rvdis_attr_tbl[] =
{
    /*----------------- Transmit Service -------------------*/
    /* <<Primary Service>>, .. */
    {
#if F_APP_GATT_OVER_BREDR_SUPPORT
        (ATTRIB_FLAG_VOID | ATTRIB_FLAG_LE | ATTRIB_FLAG_BREDR),   /* flags     */
#else
        (ATTRIB_FLAG_VOID | ATTRIB_FLAG_LE),        /* flags     */
#endif
        {                                           /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
        },
        UUID_128BIT_SIZE,                            /* bValueLen     */
        (void *)GATT_UUID128_RVDIS,                 /* p_value_context */
        GATT_PERM_READ                              /* permissions  */
    },
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
    /* Preferred gatt transport type characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_PREFERRED_GATT_TRANSPORT_TYPE),
            HI_WORD(GATT_UUID_CHAR_PREFERRED_GATT_TRANSPORT_TYPE)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    },

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
    /* Current gatt transport type characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_CURRENT_GATT_TRANSPORT_TYPE),
            HI_WORD(GATT_UUID_CHAR_CURRENT_GATT_TRANSPORT_TYPE)
        },
        0,                                          /* variable size */
        (void *)NULL,
        GATT_PERM_READ                              /* permissions */
    }

};

const uint16_t rvdis_char_num = sizeof(rvdis_attr_tbl) / sizeof(T_ATTRIB_APPL);

bool rvdis_char_read_confirm(uint16_t conn_handle, uint16_t cid, uint8_t service_id,
                             uint16_t char_uuid, uint16_t offset,
                             uint16_t value_len, uint8_t *p_value)
{
    bool ret = false;
    uint16_t attrib_index = gatt_svc_find_char_index_by_uuid16(rvdis_attr_tbl, char_uuid,
                                                               rvdis_char_num);
    if (attrib_index == 0)
    {
        PROFILE_PRINT_ERROR0("rvdis_char_read_confirm: attr not found");
        return ret;
    }

    if (((char_uuid == GATT_UUID_CHAR_PREFERRED_GATT_TRANSPORT_TYPE) &&
         (value_len != GATT_CHAR_PREFERRED_GATT_TRANSPORT_TYPE_LEN)) ||
        ((char_uuid == GATT_UUID_CHAR_CURRENT_GATT_TRANSPORT_TYPE) &&
         (value_len != GATT_CHAR_CURRENT_GATT_TRANSPORT_TYPE_LEN)))
    {
        PROFILE_PRINT_ERROR1("rvdis_char_read_confirm: invalid value length char_uuid 0x%x", char_uuid);
        return ret;
    }

    uint8_t *p_data = p_value;

    return gatt_svc_read_confirm(conn_handle, cid, service_id, attrib_index, p_data + offset,
                                 value_len - offset, value_len, APP_RESULT_SUCCESS);
}

T_APP_RESULT rvdis_attr_read_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                                uint16_t attrib_index,
                                uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    *p_length = 0;
    T_CHAR_UUID char_uuid = gatt_svc_find_char_uuid_by_index(rvdis_attr_tbl, attrib_index,
                                                             rvdis_char_num);
    T_RVDIS_SERVER_READ_CHAR_IND read_ind = {0};
    read_ind.service_id = service_id;
    read_ind.offset = offset;
    read_ind.char_uuid = char_uuid.uu.char_uuid16;

    PROFILE_PRINT_INFO4("rvdis_attr_read_cb: conn_handle 0x%x, service_id 0x%x, char_uuid %d offset %d",
                        conn_handle, service_id, read_ind.char_uuid, offset);

    switch (read_ind.char_uuid)
    {
    case GATT_UUID_CHAR_PREFERRED_GATT_TRANSPORT_TYPE:
        {
            if (pfn_rvdis_cb)
            {
                cause = pfn_rvdis_cb(conn_handle, cid, GATT_MSG_RVDIS_SERVER_READ_CHAR_IND, (void *)&read_ind);
            }

            if (cause == APP_RESULT_SUCCESS)
            {
                cause = APP_RESULT_PENDING;
            }
        }
        break;

    case GATT_UUID_CHAR_CURRENT_GATT_TRANSPORT_TYPE:
        {
            if (pfn_rvdis_cb)
            {
                cause = pfn_rvdis_cb(conn_handle, cid, GATT_MSG_RVDIS_SERVER_READ_CHAR_IND, (void *)&read_ind);
            }

            if (cause == APP_RESULT_SUCCESS)
            {
                cause = APP_RESULT_PENDING;
            }
        }
        break;
    default:
        {
            PROFILE_PRINT_ERROR0("rvdis_attr_read_cb: attr not found");
            cause  = APP_RESULT_ATTR_NOT_FOUND;
        }
        break;
    }

    return (cause);
}

/**
 * @brief DIS Service Callbacks.
*/
const T_FUN_GATT_EXT_SERVICE_CBS  rvdis_cbs =
{
    rvdis_attr_read_cb,  // Read callback function pointer
    NULL, // Write callback function pointer
    NULL  // CCCD update callback function pointer
};


T_SERVER_ID rvdis_add_service(P_FUN_RVDIS_SERVER_APP_CB app_cb)
{
    T_SERVER_ID service_id;
    if (false == gatt_svc_add(&service_id,
                              (uint8_t *)rvdis_attr_tbl,
                              sizeof(rvdis_attr_tbl),
                              &rvdis_cbs, NULL))
    {
        PROFILE_PRINT_ERROR1("rvdis_add_service: service_id %d", service_id);
        service_id = 0xff;
    }

    pfn_rvdis_cb = app_cb;
    return service_id;
}
