/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if F_APP_DURIAN_SUPPORT

#include <string.h>
#include "trace.h"
#include "app_durian_att.h"
#include "profile_server_ext.h"

T_SERVER_ID simp_ext_service_id;
typedef bool(*P_FUN_AVP_ATT_GENERAL_CB)(uint16_t conn_handle, uint8_t service_id, uint16_t cid,
                                        uint8_t *data, uint16_t length);

static P_FUN_AVP_ATT_GENERAL_CB pfn_simp_ext_service_cb = NULL;

static uint8_t avp_att_read_value2[] = {0x01, 0x01, 0x00, 0x04, 0x07, 0x02, 0x01, 0x01};
static uint8_t avp_att_read_value19[] = {0x00, 0x00, 0xC0, 0x42};
static uint8_t avp_att_read_value21[] = {0x00, 0x00, 0xC8, 0xC2, 0x00, 0x00, 0x7E, 0xC2, 0x00, 0x00,
                                         0x62, 0xC2, 0x00, 0x00, 0x4A, 0xC2, 0x00, 0x00, 0x34, 0xC2,
                                         0x00, 0x00, 0x20, 0xC2, 0x00, 0x00, 0x0E, 0xC2, 0x00, 0x00,
                                         0xF8, 0xC1, 0x00, 0x00, 0xD8, 0xC1, 0x00, 0x00, 0xB8, 0xC1,
                                         0x00, 0x00, 0x98, 0xC1, 0x00, 0x00, 0x78, 0xC1, 0x00, 0x00,
                                         0x40, 0xC1, 0x00, 0x00, 0x08, 0xC1, 0x00, 0x00, 0xB0, 0xC0,
                                         0x00, 0x00, 0x20, 0xC0, 0x00, 0x00, 0x00, 0x00
                                        };

//Attribute Handle 1-17
//UUID 9BD708D7-64C7-4E9F-9DED-F6B6C4551967
static const uint8_t avp_att_service1_uuid128[16] =
{
    0x67, 0x19, 0x55, 0xC4, 0xB6, 0xF6, 0xED, 0x9D,
    0x9F, 0x4E, 0xC7, 0x64, 0xD7, 0x08, 0xD7, 0x9B
};

//Attribute Handle 18-24
static const uint8_t avp_att_service2_uuid128[16] =
{
    0x4E, 0xE0, 0xEF, 0x92, 0x34, 0x56, 0x33, 0x99,
    0xA6, 0x45, 0xB7, 0xB7, 0x2B, 0x08, 0x98, 0x77
};

static const T_ATTRIB_APPL avp_att_attr_tbl[] =
{
    /*----------------- service1 -------------------*/
    /* <<Primary Service>>, .. */
    {
        (ATTRIB_FLAG_VOID | ATTRIB_FLAG_BREDR),           /* flags     */
        {
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),              /* type_value */
        },
        UUID_128BIT_SIZE,                                    /* bValueLen     */
        (void *)avp_att_service1_uuid128,                    /* p_value_context */
        GATT_PERM_READ                                      /* permissions  */
    },

    /* <<Characteristic 1>>, .. */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0xA8, 0x28, 0x69, 0xA0, 0x87, 0x2F, 0x8B, 0x85,
            0x01, 0x44, 0xE2, 0xA9, 0xFB, 0xB8, 0x95, 0xF1
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },

    /* <<Characteristic 2>>, .. */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_WRITE_NO_RSP)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0x7E, 0x8F, 0x4B, 0x57, 0x8E, 0xC6, 0x94, 0xAF,
            0x3D, 0x41, 0x47, 0x7E, 0x35, 0xB8, 0xF9, 0xE1
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_WRITE                              /* permissions */
    },

    /* <<Characteristic 3>>, .. */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_WRITE_NO_RSP |                    /* characteristic properties */
             GATT_CHAR_PROP_NOTIFY)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0x45, 0x38, 0x0B, 0xC0, 0x3A, 0x7F, 0x80, 0xB9,
            0xA2, 0x42, 0x8A, 0x69, 0xEF, 0xE5, 0x8C, 0xA0
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_WRITE | GATT_PERM_NOTIF_IND       /* permissions */
    },
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
    },

    /* <<Characteristic 4>>, .. */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_WRITE_NO_RSP |                    /* characteristic properties */
             GATT_CHAR_PROP_NOTIFY)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0x58, 0xFC, 0x3C, 0xBF, 0xA6, 0x9F, 0x0B, 0x89,
            0xAD, 0x47, 0x89, 0x7B, 0x2D, 0xEA, 0x88, 0x62
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_WRITE | GATT_PERM_NOTIF_IND       /* permissions */
    },
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
    },

    /* <<Characteristic 5>>, .. */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_NOTIFY)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },

    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0xC6, 0x80, 0x07, 0x61, 0x27, 0x6D, 0xF5, 0x91,
            0x46, 0x47, 0x73, 0x64, 0x1D, 0x16, 0x1C, 0x3F
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_NOTIF_IND                              /* permissions */
    },
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
    },

    /* <<Characteristic 6>>, .. */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_WRITE_NO_RSP |                    /* characteristic properties */
             GATT_CHAR_PROP_NOTIFY)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0x31, 0xB5, 0x6C, 0x89, 0x28, 0xB3, 0xAF, 0x8E,
            0xCB, 0x4B, 0x65, 0x31, 0x7D, 0x94, 0xC6, 0xC7
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_WRITE | GATT_PERM_NOTIF_IND       /* permissions */
    },
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
    },


    /*----------------- service2 -------------------*/
    /* <<Primary Service>>, .. */
    {
        (ATTRIB_FLAG_VOID | ATTRIB_FLAG_BREDR),           /* flags     */
        {
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),              /* type_value */
        },
        UUID_128BIT_SIZE,                                    /* bValueLen     */
        (void *)avp_att_service2_uuid128,                    /* p_value_context */
        GATT_PERM_READ                                      /* permissions  */
    },

    /* <<Characteristic 1>>, .. */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0x9A, 0x46, 0xE3, 0x90, 0x58, 0xEC, 0xC9, 0xAF,
            0x73, 0x48, 0xCA, 0x1B, 0x0A, 0xBC, 0xF6, 0x82
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0xB4, 0x35, 0x62, 0xBE, 0x4A, 0xF5, 0xE6, 0xA7,
            0xBB, 0x41, 0x2C, 0x2F, 0xFA, 0x6A, 0xF9, 0xD5
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE_NO_RSP)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0x90, 0x53, 0x4D, 0x74, 0xAF, 0x9E, 0x03, 0x94,
            0xDB, 0x45, 0xAB, 0xF7, 0xC1, 0x1C, 0x62, 0xD5
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE_NO_RSP)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0xfb, 0xb4, 0xeb, 0xe8, 0xbc, 0xaf, 0xaa, 0x82,
            0xdb, 0x43, 0x21, 0x39, 0x08, 0xec, 0x8f, 0xa4
        },
        0,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)                              /* permissions */
    },
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE_NO_RSP)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {
            0x66, 0x31, 0x6b, 0xe6, 0xbd, 0x2a, 0xfe, 0xaa,
            0x52, 0x4b, 0x3f, 0xdc, 0x56, 0xb7, 0xd0, 0xa1
        },
        0,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* permissions */
    }
};

T_APP_RESULT  avp_att_service_attr_read_cb(uint16_t conn_handle,  uint16_t cid,
                                           T_SERVER_ID service_id,
                                           uint16_t attrib_index, uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT  cause  = APP_RESULT_SUCCESS;

    PROFILE_PRINT_TRACE1("avp_att_service_attr_read_cb, index 0x%02x", attrib_index);

    switch (attrib_index)
    {
    case AVP_ATT_SERVICE_CHAR_READ_INDEX2:
        {
            *pp_value = avp_att_read_value2;
            *p_length = sizeof(avp_att_read_value2);
        }
        break;

    case AVP_ATT_SERVICE_CHAR_READ_INDEX19:
        {
            *pp_value = avp_att_read_value19;
            *p_length = sizeof(avp_att_read_value19);
        }
        break;

    case AVP_ATT_SERVICE_CHAR_READ_INDEX21:
        {
            *pp_value = avp_att_read_value21;
            *p_length = sizeof(avp_att_read_value21);
        }
        break;

    case AVP_ATT_SERVICE_CHAR_READ_INDEX25:
        {
            static uint8_t value[] = {0x01};
            *pp_value = value;
            *p_length = sizeof(value);
        }
        break;

    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;
    }

    return cause;
}


void avp_att_write_post_callback(uint16_t conn_handle,  uint16_t cid, T_SERVER_ID service_id,
                                 uint16_t attrib_index,
                                 uint16_t length, uint8_t *p_value)
{

}

T_APP_RESULT avp_att_service_attr_write_cb(uint16_t conn_handle,  uint16_t cid,
                                           T_SERVER_ID service_id,
                                           uint16_t attrib_index, T_WRITE_TYPE write_type, uint16_t length, uint8_t *p_value,
                                           P_FUN_EXT_WRITE_IND_POST_PROC *p_write_post_proc)
{
    T_APP_RESULT  cause = APP_RESULT_SUCCESS;

    PROFILE_PRINT_INFO3("avp_att_service_attr_write_cb write_type = 0x%x, attrib_index 0x%x, p_value[0] 0x%x",
                        write_type, attrib_index, p_value[0]);

    *p_write_post_proc = avp_att_write_post_callback;
    //attrib_index need check according to durian service table

    /* Notify avp. */
    if (pfn_simp_ext_service_cb)
    {
        pfn_simp_ext_service_cb(conn_handle, (uint8_t)service_id, cid, p_value, length);
    }

    return cause;
}

void avp_att_service_cccd_update_cb(uint16_t conn_handle,  uint16_t cid, T_SERVER_ID service_id,
                                    uint16_t index,
                                    uint16_t cccbits)
{

}

//opus data
bool avp_att_service_send_notify1(uint16_t conn_handle,  uint16_t cid,
                                  void *p_value, uint16_t length)
{
    return server_ext_send_data(conn_handle, cid, simp_ext_service_id,
                                AVP_ATT_SERVICE_CHAR_NOTIFY_INDEX1,
                                p_value, length, GATT_PDU_TYPE_ANY);
}

//voice_recognition_encode_start
bool avp_att_service_send_notify2(uint16_t conn_handle,  uint16_t cid,
                                  void *p_value, uint16_t length)
{
    return server_ext_send_data(conn_handle, cid, simp_ext_service_id,
                                AVP_ATT_SERVICE_CHAR_NOTIFY_INDEX2,
                                p_value, length, GATT_PDU_TYPE_ANY);
}

//voice_recognition_enable_req && voice_recognition_disable_req && voice_recognition_encode_stop
bool avp_att_service_send_notify3(uint16_t conn_handle,  uint16_t cid,
                                  void *p_value, uint16_t length)
{
    return server_ext_send_data(conn_handle, cid, simp_ext_service_id,
                                AVP_ATT_SERVICE_CHAR_NOTIFY_INDEX3,
                                p_value, length, GATT_PDU_TYPE_ANY);
}

const T_FUN_GATT_EXT_SERVICE_CBS simp_ext_service_cbs =
{
    avp_att_service_attr_read_cb,  // Read callback function pointer
    avp_att_service_attr_write_cb, // Write callback function pointer
    avp_att_service_cccd_update_cb // CCCD update callback function pointer
};

uint8_t avp_att_add_service(void *p_func)
{
    if (false == server_ext_add_service(&simp_ext_service_id,
                                        (uint8_t *)avp_att_attr_tbl,
                                        sizeof(avp_att_attr_tbl),
                                        &simp_ext_service_cbs))
    {
        PROFILE_PRINT_ERROR0("avp_att_add_service: fail");
        simp_ext_service_id = 0xff;
        return (uint8_t)simp_ext_service_id;
    }

    pfn_simp_ext_service_cb = (P_FUN_AVP_ATT_GENERAL_CB)p_func;
    return (uint8_t)simp_ext_service_id;
}
#endif
