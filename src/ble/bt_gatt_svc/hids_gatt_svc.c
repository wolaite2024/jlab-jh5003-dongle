/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     hids_gatt_svc.c
  * @brief    Source file for using Human Interface Device Service.
  * @details  Global data and function implement.
  * @author   Leon_liu
  * @date     2022-9-21
  * @version  v1.0
  * *************************************************************************************
  */

#include <string.h>
#include "trace.h"
#include "hids_gatt_svc.h"

#define HIDS_REPORT_MAP_MAX_LEN      512
#define HIDS_CHAR_REPORT_NUM         (HIDS_CHAR_INPUT_REPORT_NUM + \
                                      HIDS_CHAR_OUTPUT_REPORT_NUM + \
                                      HIDS_CHAR_FEATURE_REPORT_NUM)

typedef struct
{
    uint16_t attr_idx;
    uint8_t report_index;
    T_HIDS_REPORT_TYPE report_type;
} T_HIDS_REPORT_IDX;

T_HIDS_REPORT_IDX hid_report_idx[HIDS_CHAR_REPORT_NUM] = {0};
static P_FUN_HIDS_SERVER_APP_CB pfn_hids_cb = NULL;

static const T_ATTRIB_APPL hids_attr_tbl[] =
{
    /* <<Primary Service>>, .. 0*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /* wFlags     */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_HIDS),               /* service UUID */
            HI_WORD(GATT_UUID_HIDS)
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
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE_NO_RSP                   /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /*  HID Protocol Mode characteristic value ..2*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_PROTOCOL_MODE),
            HI_WORD(GATT_UUID_CHAR_PROTOCOL_MODE)
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_READ | GATT_PERM_WRITE                        /* wPermissions */
    },

#if (HIDS_CHAR_INPUT_REPORT_NUM >= 1)
    /* <<Characteristic>>, .. 3*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_NOTIFY          /* characteristic properties */
#if HIDS_CHAR_INPUT_REPORT_WRITE_SUPPORT
            | GATT_CHAR_PROP_WRITE
#endif
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 4*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT),
            HI_WORD(GATT_UUID_CHAR_REPORT)
        },
        0,                                        /* variable size */
        NULL,
        GATT_PERM_READ /* wPermissions */
    },

    /* client characteristic configuration .. 5*/
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

    /*report ID map reference descriptor .. 6*/
    {
        (ATTRIB_FLAG_VALUE_APPL | ATTRIB_FLAG_CCCD_APPL),                  /* wFlags */
        {                                         /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            HI_WORD(GATT_UUID_CHAR_REPORT_REFERENCE)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
#endif
#if (HIDS_CHAR_INPUT_REPORT_NUM == 2)
    /* <<Characteristic>>, .. 7*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_NOTIFY           /* characteristic properties */
#if HIDS_CHAR_INPUT_REPORT_WRITE_SUPPORT
            | GATT_CHAR_PROP_WRITE
#endif
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
        NULL,
        GATT_PERM_READ /* wPermissions */
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
        (ATTRIB_FLAG_VALUE_APPL | ATTRIB_FLAG_CCCD_APPL),                  /* wFlags */
        {                                         /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            HI_WORD(GATT_UUID_CHAR_REPORT_REFERENCE)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
#endif
#if (HIDS_CHAR_OUTPUT_REPORT_NUM >= 1)
    /* <<Characteristic>>, .. 11*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE |
            GATT_CHAR_PROP_WRITE_NO_RSP           /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 12*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT),
            HI_WORD(GATT_UUID_CHAR_REPORT)
        },
        0,                                        /* variable size */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*report ID map reference descriptor .. 13*/
    {
        (ATTRIB_FLAG_VALUE_APPL | ATTRIB_FLAG_CCCD_APPL),                  /* wFlags */
        {                                         /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            HI_WORD(GATT_UUID_CHAR_REPORT_REFERENCE)
        },
        0,                                          /* bValueLen */
        NULL,//(void*)&cPointerInputReportIdMap,
        GATT_PERM_READ                              /* wPermissions */
    },
#endif
#if (HIDS_CHAR_OUTPUT_REPORT_NUM == 2)
    /* <<Characteristic>>, .. 14*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE |
            GATT_CHAR_PROP_WRITE_NO_RSP           /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 15*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT),
            HI_WORD(GATT_UUID_CHAR_REPORT)
        },
        0,                                        /* variable size */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*report ID map reference descriptor .. 16*/
    {
        (ATTRIB_FLAG_VALUE_APPL | ATTRIB_FLAG_CCCD_APPL),                  /* wFlags */
        {                                         /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            HI_WORD(GATT_UUID_CHAR_REPORT_REFERENCE)
        },
        0,                                          /* bValueLen */
        NULL,//(void*)&cPointerInputReportIdMap,
        GATT_PERM_READ                              /* wPermissions */
    },
#endif
#if (HIDS_CHAR_FEATURE_REPORT_NUM >= 1)
    /* <<Characteristic>>, .. 17*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE           /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 18*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT),
            HI_WORD(GATT_UUID_CHAR_REPORT)
        },
        0,                                        /* variable size */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*report ID map reference descriptor .. 19*/
    {
        (ATTRIB_FLAG_VALUE_APPL | ATTRIB_FLAG_CCCD_APPL),                  /* wFlags */
        {                                         /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            HI_WORD(GATT_UUID_CHAR_REPORT_REFERENCE)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
#endif
#if (HIDS_CHAR_FEATURE_REPORT_NUM == 2)
    /* <<Characteristic>>, .. 20*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE           /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 21*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT),
            HI_WORD(GATT_UUID_CHAR_REPORT)
        },
        0,                                        /* variable size */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*report ID map reference descriptor .. 22*/
    {
        (ATTRIB_FLAG_VALUE_APPL | ATTRIB_FLAG_CCCD_APPL),                  /* wFlags */
        {                                         /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_REFERENCE),
            HI_WORD(GATT_UUID_CHAR_REPORT_REFERENCE)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },
#endif
    /* <<Characteristic>>, .. 23*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                    /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /*  HID report map characteristic value .. 24*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_REPORT_MAP),
            HI_WORD(GATT_UUID_CHAR_REPORT_MAP)
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /* <<EXTERNAL_REPORT_REFERENCE>>, .. 25*/
    {
        ATTRIB_FLAG_VALUE_APPL,                                 /* flags */
        {                                                       /* type_value */
            LO_WORD(GATT_UUID_CHAR_EXTERNAL_REPORT_REFERENCE),
            HI_WORD(GATT_UUID_CHAR_EXTERNAL_REPORT_REFERENCE),
        },
        0,                                                      /* bValueLen */
        NULL,
        GATT_PERM_READ                                          /* permissions */
    },
#if HIDS_CHAR_BOOT_KB_REPORT_SUPPORT
    /* <<Characteristic>>, .. 26*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_NOTIFY           /* characteristic properties */
#if HIDS_CHAR_BOOT_KB_INPUT_REPORT_WRITE_SUPPORT
            | GATT_CHAR_PROP_WRITE
#endif
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 27*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_BOOT_KB_IN_REPORT),
            HI_WORD(GATT_UUID_CHAR_BOOT_KB_IN_REPORT)
        },
        0,                                        /* variable size */
        NULL,
        GATT_PERM_READ /* wPermissions */
    },

    /* client characteristic configuration .. 28*/
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

    /* <<Characteristic>>, .. 29*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE |
            GATT_CHAR_PROP_WRITE_NO_RSP           /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 30*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_BOOT_KB_OUT_REPORT),
            HI_WORD(GATT_UUID_CHAR_BOOT_KB_OUT_REPORT)
        },
        0,                                        /* variable size */
        NULL,
        GATT_PERM_READ /* wPermissions */
    },
#endif
#if HIDS_CHAR_BOOT_MS_REPORT_SUPPORT
    /* <<Characteristic>>, .. 31*/
    {
        ATTRIB_FLAG_VALUE_INCL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE          /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                        /* bValueLen */
        NULL,
        GATT_PERM_READ                            /* wPermissions */
    },

    /*  HID Report characteristic value .. 32*/
    {
        ATTRIB_FLAG_VALUE_APPL,                   /* wFlags */
        {                                       /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_BOOT_MS_IN_REPORT),
            HI_WORD(GATT_UUID_CHAR_BOOT_MS_IN_REPORT)
        },
        0,                                        /* variable size */
        NULL,
        GATT_PERM_READ /* wPermissions */
    },
#endif
    /* <<Characteristic>>, .. 33*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /*  HID Information characteristic value .. 34*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HID_INFO),
            HI_WORD(GATT_UUID_CHAR_HID_INFO)
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /* <<Characteristic>>, .. 35*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_WRITE_NO_RSP             /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /*  HID controlPoint characteristic value .. 36*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {                                           /* bTypeValue */
            LO_WORD(GATT_UUID_CHAR_HID_CONTROL_POINT),
            HI_WORD(GATT_UUID_CHAR_HID_CONTROL_POINT)
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_WRITE                             /* wPermissions */
    }
};

uint16_t hids_attr_num = sizeof(hids_attr_tbl) / sizeof(T_ATTRIB_APPL);

bool hids_get_report_idx(uint16_t attr_idx, uint8_t *p_report_idx,
                         T_HIDS_REPORT_TYPE *p_report_type)
{
    for (uint8_t i = 0; i < HIDS_CHAR_REPORT_NUM ; i++)
    {
        if (hid_report_idx[i].attr_idx == attr_idx)
        {
            *p_report_idx = hid_report_idx[i].report_index;
            *p_report_type = hid_report_idx[i].report_type;
            return true;
        }
    }
    PROFILE_PRINT_ERROR1("hids_get_report_idx: not found report, attr_idx %d", attr_idx);
    return false;
}

bool hids_get_report_attr_idx(uint8_t report_index, uint8_t report_type,
                              uint16_t *p_attr_idx)
{
    for (uint8_t i = 0; i < HIDS_CHAR_REPORT_NUM; i++)
    {
        if ((hid_report_idx[i].report_index == report_index) &&
            (hid_report_idx[i].report_type == report_type))
        {
            *p_attr_idx = hid_report_idx[i].attr_idx;
            return true;
        }
    }
    PROFILE_PRINT_ERROR2("hids_get_report_attr_idx: not found attr_idx, report idx %d, report type %d",
                         report_index, report_type);
    return false;
}

bool hids_send_input_report_notify(uint8_t conn_handle, uint8_t service_id,
                                   T_HIDS_CHAR_REPORT report_data)
{
    uint16_t attr_idx = 0;

    if (!hids_get_report_attr_idx(report_data.report_index, report_data.report_type, &attr_idx))
    {
        return false;
    }

    return gatt_svc_send_data(conn_handle, L2C_FIXED_CID_ATT, service_id, attr_idx,
                              report_data.p_value, report_data.value_len, GATT_PDU_TYPE_NOTIFICATION);
}

bool hids_send_boot_kb_input_report_notify(uint8_t conn_handle, uint8_t service_id,
                                           T_HIDS_CHAR_KB_MS_REPORT report_data)
{
    int16_t attr_idx = gatt_svc_find_char_index_by_uuid16(hids_attr_tbl,
                                                          GATT_UUID_CHAR_BOOT_KB_IN_REPORT,
                                                          hids_attr_num);

    if (attr_idx == 0)
    {
        PROFILE_PRINT_ERROR0("hids_send_boot_kb_input_report_notify: char not found");
        return false;
    }

    return gatt_svc_send_data(conn_handle, L2C_FIXED_CID_ATT, service_id, attr_idx,
                              report_data.p_value, report_data.value_len, GATT_PDU_TYPE_NOTIFICATION);
}

bool hids_read_char_confirm(uint8_t conn_handle, uint16_t cid, uint8_t service_id,
                            uint16_t offset, T_HIDS_CHAR_VALUE char_value, T_APP_RESULT cause)
{
    uint16_t attr_idx = gatt_svc_find_char_index_by_uuid16(hids_attr_tbl, char_value.char_uuid,
                                                           hids_attr_num);
    uint8_t error_idx = 0;
    uint8_t *p_data = NULL;
    uint16_t data_len = 0;

    if (attr_idx == 0)
    {
        PROFILE_PRINT_ERROR0("hids_char_read_confirm: char not found");
        error_idx = 1;
        goto error;
    }

    switch (char_value.char_uuid)
    {
    case GATT_UUID_CHAR_PROTOCOL_MODE:
        {
            data_len = sizeof(uint8_t);
            p_data = (uint8_t *)&char_value.data.protocol_mode;
        }
        break;

    case GATT_UUID_CHAR_REPORT_MAP:
        {
            if (char_value.data.report_map.report_map_len > HIDS_REPORT_MAP_MAX_LEN)
            {
                error_idx = 2;
                goto error;
            }
            data_len = char_value.data.report_map.report_map_len;
            p_data = char_value.data.report_map.p_report_map;
        }
        break;

    case GATT_UUID_CHAR_HID_INFO:
        {
            data_len = sizeof(T_HIDS_INFO);
            p_data = (uint8_t *)&char_value.data.hid_info;
        }
        break;

    case GATT_UUID_CHAR_EXTERNAL_REPORT_REFERENCE:
        {
            data_len = sizeof(uint16_t);
            p_data = (uint8_t *)&char_value.data.hid_external_report_ref;
        }
        break;

#if HIDS_CHAR_BOOT_KB_REPORT_SUPPORT
    case GATT_UUID_CHAR_BOOT_KB_IN_REPORT:
    case GATT_UUID_CHAR_BOOT_KB_OUT_REPORT:
        {
            data_len = char_value.data.kb_ms_report.value_len;
            p_data = char_value.data.kb_ms_report.p_value + offset;
        }
        break;
#endif

#if HIDS_CHAR_BOOT_MS_REPORT_SUPPORT
    case GATT_UUID_CHAR_BOOT_MS_IN_REPORT:
        {
            data_len = char_value.data.kb_ms_report.value_len;
            p_data = char_value.data.kb_ms_report.p_value + offset;
        }
        break;
#endif

    case GATT_UUID_CHAR_REPORT_REFERENCE:
        {
            if (!hids_get_report_attr_idx(char_value.data.report_ref.report_index,
                                          char_value.data.report_ref.report.report_type, &attr_idx))
            {
                error_idx = 3;
                goto error;
            }

            if (char_value.data.report_ref.report.report_type == HID_INPUT_TYPE)
            {
                attr_idx += 2;
            }
            else
            {
                attr_idx += 1;
            }

            data_len = sizeof(T_HIDS_REPORT_REFERENCE);
            p_data = (uint8_t *)&char_value.data.report_ref.report;
        }
        break;

    default:
        {
            error_idx = 4;
            goto error;
        }
    }

    return gatt_svc_read_confirm(conn_handle, cid, service_id,
                                 attr_idx, p_data, data_len - offset, data_len, cause);

error:
    {
        PROFILE_PRINT_ERROR2("hids_char_read_confirm: fail, error idx %d, char_uuid 0x%x", error_idx,
                             char_value.char_uuid);
        return false;
    }
}

bool hids_read_report_confirm(uint8_t conn_handle, uint16_t cid, uint8_t service_id,
                              uint16_t offset, T_HIDS_CHAR_REPORT report_data, T_APP_RESULT cause)
{
    uint16_t attr_idx = 0;
    if (!hids_get_report_attr_idx(report_data.report_index, report_data.report_type, &attr_idx))
    {
        return false;
    }

    return gatt_svc_read_confirm(conn_handle, cid, service_id, attr_idx, report_data.p_value + offset,
                                 report_data.value_len - offset, report_data.value_len, cause);
}

static T_APP_RESULT hids_attr_read_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                                      uint16_t attrib_index,
                                      uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    T_HIDS_SERVER_READ_IND read_ind = {0};
    T_CHAR_UUID char_uuid = gatt_svc_find_char_uuid_by_index(hids_attr_tbl, attrib_index,
                                                             hids_attr_num);

    read_ind.service_id = service_id;
    read_ind.offset = offset;
    read_ind.char_uuid = char_uuid.uu.char_uuid16;

    switch (char_uuid.uu.char_uuid16)
    {
    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;

    case GATT_UUID_CHAR_PROTOCOL_MODE:
    case GATT_UUID_CHAR_REPORT_MAP:
    case GATT_UUID_CHAR_HID_INFO:
#if HIDS_CHAR_BOOT_KB_REPORT_SUPPORT
    case GATT_UUID_CHAR_BOOT_KB_IN_REPORT:
    case GATT_UUID_CHAR_BOOT_KB_OUT_REPORT:
#endif
#if HIDS_CHAR_BOOT_MS_REPORT_SUPPORT
    case GATT_UUID_CHAR_BOOT_MS_IN_REPORT:
#endif
    case GATT_UUID_CHAR_EXTERNAL_REPORT_REFERENCE:
        {

        }
        break;

    case GATT_UUID_CHAR_REPORT_REFERENCE:
        {
            if (!hids_get_report_idx(attrib_index - 1, &read_ind.data.report_info.report_index,
                                     &read_ind.data.report_info.report_type))
            {
                if (!hids_get_report_idx(attrib_index - 2, &read_ind.data.report_info.report_index,
                                         &read_ind.data.report_info.report_type))
                {
                    cause = APP_RESULT_ATTR_NOT_FOUND;
                }
            }
        }
        break;

    case GATT_UUID_CHAR_REPORT:
        {
            if (!hids_get_report_idx(attrib_index, &read_ind.data.report_info.report_index,
                                     &read_ind.data.report_info.report_type))
            {
                cause = APP_RESULT_ATTR_NOT_FOUND;
            }
        }
        break;
    }

    if (pfn_hids_cb)
    {
        cause = pfn_hids_cb(conn_handle, cid, GATT_MSG_HIDS_SERVER_READ_IND, (void *)&read_ind);
    }

    if (cause == APP_RESULT_SUCCESS)
    {
        cause = APP_RESULT_PENDING;
    }

    return cause;
}

static T_APP_RESULT hids_attr_write_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id,
                                       uint16_t attrib_index, T_WRITE_TYPE write_type,
                                       uint16_t length, uint8_t *p_value, P_FUN_EXT_WRITE_IND_POST_PROC *p_write_post_proc)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;

    T_HIDS_SERVER_WRITE_IND write_ind = {0};

    T_CHAR_UUID char_uuid = gatt_svc_find_char_uuid_by_index(hids_attr_tbl, attrib_index,
                                                             hids_attr_num);

    write_ind.char_uuid = char_uuid.uu.char_uuid16;
    write_ind.service_id = service_id;
    write_ind.write_type = write_type;

    if (!p_value)
    {
        cause = APP_RESULT_INVALID_PDU;
        return cause;
    }

    switch (write_ind.char_uuid)
    {
    default:
        cause = APP_RESULT_ATTR_NOT_FOUND;
        break;

    case GATT_UUID_CHAR_PROTOCOL_MODE:
        {
            if (length == sizeof(uint8_t))
            {
                memcpy(&write_ind.data.protocol_mode, p_value, length);
            }
            else
            {
                cause = APP_RESULT_INVALID_VALUE_SIZE;
            }
        }
        break;

    case GATT_UUID_CHAR_HID_CONTROL_POINT:
        {
            if (length == sizeof(uint8_t))
            {
                memcpy(&write_ind.data.hid_cp, p_value, length);
            }
            else
            {
                cause = APP_RESULT_INVALID_VALUE_SIZE;
            }
        }
        break;

#if HIDS_CHAR_BOOT_KB_REPORT_SUPPORT
#if HIDS_CHAR_BOOT_KB_INPUT_REPORT_WRITE_SUPPORT
    case GATT_UUID_CHAR_BOOT_KB_IN_REPORT:
#endif
    case GATT_UUID_CHAR_BOOT_KB_OUT_REPORT:
#endif
#if HIDS_CHAR_BOOT_MS_REPORT_SUPPORT
    case GATT_UUID_CHAR_BOOT_MS_IN_REPORT:
#endif
    case GATT_UUID_CHAR_REPORT:
        {
            if (write_ind.char_uuid == GATT_UUID_CHAR_REPORT)
            {
                if (hids_get_report_idx(attrib_index, &write_ind.data.report.report_index,
                                        &write_ind.data.report.report_type))
                {
                    write_ind.data.report.value_len = length;
                    write_ind.data.report.p_value = p_value;
                }
                else
                {
                    cause = APP_RESULT_ATTR_NOT_FOUND;
                }
            }
            else
            {
                write_ind.data.kb_ms_report.value_len = length;
                write_ind.data.kb_ms_report.p_value = p_value;
            }
        }
        break;
    }

    if (pfn_hids_cb && (cause == APP_RESULT_SUCCESS))
    {
        cause = pfn_hids_cb(conn_handle, cid, GATT_MSG_HIDS_SERVER_WRITE_IND, (void *)&write_ind);
    }

    return cause;

}

void hids_cccd_update_cb(uint16_t conn_handle, uint16_t cid, T_SERVER_ID service_id, uint16_t index,
                         uint16_t ccc_bits)
{
    bool cause = true;

    T_CHAR_UUID char_uuid = gatt_svc_find_char_uuid_by_index(hids_attr_tbl, index, hids_attr_num);

    T_HIDS_SERVER_CCCD_UPDATE cccd_update = {0};

    cccd_update.service_id = service_id;
    cccd_update.char_uuid = char_uuid.uu.char_uuid16;
    cccd_update.cccd_cfg = ccc_bits;

    PROFILE_PRINT_INFO2("hids_cccd_update_cb index = %d ccc_bits %x", index, ccc_bits);

    if (cccd_update.char_uuid == GATT_UUID_CHAR_REPORT)
    {
        if (!hids_get_report_idx(index - 1, &cccd_update.data.report_info.report_index,
                                 &cccd_update.data.report_info.report_type))
        {
            cause = false;
        }
    }
    else if (cccd_update.char_uuid == GATT_UUID_CHAR_BOOT_KB_IN_REPORT)
    {

    }
    else
    {
        cause = false;
    }

    if (pfn_hids_cb && (cause == true))
    {
        pfn_hids_cb(conn_handle, cid, GATT_MSG_HIDS_SERVER_CCCD_UPDATE, (void *)&cccd_update);
    }

    return;
}

const T_FUN_GATT_EXT_SERVICE_CBS hids_cbs =
{
    hids_attr_read_cb,      // Read callback function pointer
    hids_attr_write_cb,     // Write callback function pointer
    hids_cccd_update_cb,    // Authorization callback function pointer
};

bool hids_create_report_table(void)
{
    T_CHAR_UUID char_uuid ;
    char_uuid.uuid_size = UUID_16BIT_SIZE;
    char_uuid.uu.char_uuid16 = GATT_UUID_CHAR_REPORT;
    uint8_t in_out_reprot_num = HIDS_CHAR_INPUT_REPORT_NUM + HIDS_CHAR_OUTPUT_REPORT_NUM;
    uint16_t attr_idx = 0;

    for (uint8_t i = 0; i < HIDS_CHAR_REPORT_NUM; i++)
    {
        char_uuid.index = i + 1;

        if (i < HIDS_CHAR_INPUT_REPORT_NUM)
        {
            hid_report_idx[i].report_type = HID_INPUT_TYPE;
            hid_report_idx[i].report_index = i;
        }
        else if (i >= HIDS_CHAR_INPUT_REPORT_NUM && i < in_out_reprot_num)
        {
            hid_report_idx[i].report_type = HID_OUTPUT_TYPE;
            hid_report_idx[i].report_index = i - HIDS_CHAR_INPUT_REPORT_NUM;
        }
        else if (i >= in_out_reprot_num)
        {
            hid_report_idx[i].report_type = HID_FEATURE_TYPE;
            hid_report_idx[i].report_index = i - in_out_reprot_num;
        }

        if (!gatt_svc_find_char_index_by_uuid(hids_attr_tbl, char_uuid,
                                              hids_attr_num, &attr_idx))
        {
            PROFILE_PRINT_ERROR0("hids_create_report_table: fail");
            return false;
        }
        hid_report_idx[i].attr_idx = attr_idx;
    }

    return true;
}

T_SERVER_ID hids_reg_srv(P_FUN_HIDS_SERVER_APP_CB app_cb)
{
    T_SERVER_ID service_id = 0xff;

    if (HIDS_CHAR_INPUT_REPORT_NUM > 2 || HIDS_CHAR_OUTPUT_REPORT_NUM > 2 ||
        HIDS_CHAR_FEATURE_REPORT_NUM > 2)
    {
        PROFILE_PRINT_ERROR3("hids_reg_srv: report number exceeds the range: input report %d, output report %d, feature report %d",
                             HIDS_CHAR_INPUT_REPORT_NUM, HIDS_CHAR_OUTPUT_REPORT_NUM,
                             HIDS_CHAR_FEATURE_REPORT_NUM);
        return service_id;
    }

    if (hids_create_report_table() == false)
    {
        service_id = 0xff;
    }

    if (false == gatt_svc_add(&service_id, (uint8_t *)hids_attr_tbl,
                              hids_attr_num * sizeof(T_ATTRIB_APPL),
                              &hids_cbs, NULL))
    {
        PROFILE_PRINT_ERROR1("hids_reg_srv: service_id %d", service_id);
        service_id = 0xff;
    }

    pfn_hids_cb = app_cb;

    return service_id;
}
