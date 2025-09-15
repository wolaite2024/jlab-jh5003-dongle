#if F_APP_CHATGPT_SUPPORT
#include "app_main.h"
#include "app_ble_service.h"
#include "app_chatgpt.h"

// static const uint8_t app_chatgpt_ble_adv_data[31] =
// {
//     /* This modification is used to solve issue: BLE name shown on Android list */
//     /* TX Power Level */
//     0x02,/* length */
//     GAP_ADTYPE_POWER_LEVEL,
//     0x00, /* Don't care now */

//     /* Service */
//     17,/* length */
//     GAP_ADTYPE_128BIT_COMPLETE,/* "Complete 128-bit UUIDs available"*/
//     /*transmit service uuid*/
//     0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFF, 0x01, 0x00,
//     0x00,/*bud role:0x00 single,0x01 pri, 0x02 sec*/

//     /* Manufacturer Specific Data */
//     9,/* length */
//     0xFF,
//     0x5D, 0x00 /* Company ID */
//     /*local address*/
// };

static const uint8_t chatgpt_uuid[] = {0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFF, 0x01, 0x00, 0x00};

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
static P_FUN_EXT_SERVER_GENERAL_CB chatgpt_srv_rx_cb = NULL;
#else
static P_FUN_SERVER_GENERAL_CB chatgpt_srv_rx_cb = NULL;
#endif

static T_SERVER_ID chatgpt_srv_id = 0xff;
static uint8_t chatgpt_app_idx = 0;
static const uint8_t GATT_UUID128_CHATGPT_SRV[16] = {0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFF, 0x01, 0x00, 0x00};

static T_ATTRIB_APPL chatgpt_bwps_attr_tbl[] =
{
    /*--------------------------BWPS(bee wristband private protocol) Service ---------------------------*/
    /* <<Primary Service>>, .. 0*/
    {
        (ATTRIB_FLAG_VOID | ATTRIB_FLAG_LE),        /* wFlags     */
        {
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
        },
        UUID_128BIT_SIZE,                           /* bValueLen     */
        (void *)GATT_UUID128_CHATGPT_SRV,                  /* pValueContext */
        GATT_PERM_READ                              /* wPermissions  */
    },

    /* <<Characteristic>>, .. 1*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {   /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_WRITE_NO_RSP | GATT_CHAR_PROP_WRITE, /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /*  Tx characteristic value 2*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {   /* bTypeValue */
            LO_WORD(BLE_UUID_CHATGPT_TX_CHARACTERISTIC),
            HI_WORD(BLE_UUID_CHATGPT_TX_CHARACTERISTIC),
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_WRITE                             /* wPermissions */
    },

    /* <<Characteristic>>, .. 3*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {   /* bTypeValue */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_NOTIFY,                  /* characteristic properties */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /* <<RX Characteristic>>, .. 4*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {   /* bTypeValue */
            LO_WORD(BLE_UUID_CHATGPT_RX_CHARACTERISTIC),
            HI_WORD(BLE_UUID_CHATGPT_RX_CHARACTERISTIC),
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_NOTIF_IND                         /* wPermissions */
    },

    /* client characteristic configuration 5*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL),    /* wFlags */
        {
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* wPermissions */
    },

    /* used for device name update */
    /* <<Characteristic>>, .. 6*/
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* wFlags */
        {
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_WRITE | GATT_CHAR_PROP_READ,                 /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* wPermissions */
    },

    /*<<Characteristic>>, .. 7*/
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* wFlags */
        {   /* bTypeValue */
            LO_WORD(BLE_UUID_CHATGPT_DEVICE_NAME),
            HI_WORD(BLE_UUID_CHATGPT_DEVICE_NAME),
        },
        0,                                          /* variable size */
        NULL,
        GATT_PERM_READ | GATT_PERM_WRITE            /* wPermissions */
    },
};
#if F_APP_GATT_SERVER_EXT_API_SUPPORT
T_APP_RESULT app_chatgpt_ble_attr_write_cb(uint16_t conn_handle, uint16_t cid,
                                           T_SERVER_ID service_id,
                                           uint16_t attrib_idx,
                                           T_WRITE_TYPE write_type, uint16_t len,
                                           uint8_t *p_value, P_FUN_EXT_WRITE_IND_POST_PROC *p_write_post_proc)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    T_CHATGPT_SRV_RX srv_rx;

    APP_PRINT_TRACE3("app_chatgpt_ble_attr_write_cb: attrib_idx %d, service_id %d, len %d",
                     attrib_idx,
                     service_id, len);


    if (p_value == NULL)
    {
        APP_PRINT_ERROR0("app_chatgpt_ble_attr_write_cb: p_value is NULL");
        cause = APP_RESULT_APP_ERR;
        return cause;
    }

    switch (attrib_idx)
    {
    default:
        {
            cause = APP_RESULT_ATTR_NOT_FOUND;
        }
        break;

    case CHATGPT_RX_DATA_INDEX:
        {
            uint8_t conn_id = 0;

            /* Notify Application. */
            srv_rx.msg_type = SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE;
            srv_rx.conn_handle  = conn_handle;
            srv_rx.cid = cid;
            srv_rx.attr_index = attrib_idx;
            srv_rx.msg_data.rx_data.len = len;
            srv_rx.msg_data.rx_data.p_value = p_value;

            le_get_conn_id_by_handle(conn_handle, &conn_id);
            srv_rx.conn_id = conn_id;

            if (chatgpt_srv_rx_cb)
            {
                chatgpt_srv_rx_cb(service_id, (void *)&srv_rx);
            }
        }
        break;
    }

    return cause;
}

void app_chatgpt_ble_cccd_update_cb(uint16_t conn_handle,  uint16_t cid, T_SERVER_ID service_id,
                                    uint16_t attrib_idx,
                                    uint16_t cccd_bits)
{
    T_CHATGPT_SRV_RX srv_rx;
    uint8_t conn_id;

    srv_rx.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;
    srv_rx.conn_handle = conn_handle;
    srv_rx.cid = cid;


    le_get_conn_id_by_handle(conn_handle, &conn_id);
    srv_rx.conn_id = conn_id;

    APP_PRINT_INFO2("app_chatgpt_ble_cccd_update_cb: attrib_idx %d, cccd_bits 0x%04x", attrib_idx,
                    cccd_bits);

    switch (attrib_idx)
    {
    case CHATGPT_DATA_CCCD_INDEX:
        {
            if (cccd_bits & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
            {
                // Enable Notification
                srv_rx.msg_data.value = TRANSMIT_SVC_TX_DATA_CCCD_ENABLE;
            }
            else
            {
                srv_rx.msg_data.value = TRANSMIT_SVC_TX_DATA_CCCD_DISABLE;
            }

            srv_rx.attr_index = CHATGPT_DATA_CCCD_INDEX;

            if (chatgpt_srv_rx_cb)
            {
                chatgpt_srv_rx_cb(service_id, (void *)&srv_rx);
            }
            break;
        }
    default:
        break;
    }
    return;
}
#else
static void app_chatgpt_ble_cccd_update_cb(uint8_t conn_id, uint8_t service_id, uint16_t attrib_idx,
                                           uint16_t ccc_bits)
{
    APP_PRINT_TRACE3("app_chatgpt_ble_cccd_update_cb: attrib_idx %d, service_id %d, ccc_bits 0x%x",
                     attrib_idx,
                     service_id,
                     ccc_bits);

    switch (attrib_idx)
    {
    case CHATGPT_DATA_CCCD_INDEX:
        {
            if (ccc_bits & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
            {

            }
            else
            {

            }
        }
        break;

    default:
        break;
    }
}

static T_APP_RESULT app_chatgpt_ble_attr_write_cb(uint8_t conn_id, uint8_t service_id,
                                                  uint16_t attrib_idx,
                                                  T_WRITE_TYPE write_type,
                                                  uint16_t len, uint8_t *p_value, P_FUN_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
    T_APP_RESULT cause = APP_RESULT_SUCCESS;
    T_CHATGPT_SRV_RX srv_rx;

    APP_PRINT_TRACE3("app_chatgpt_ble_attr_write_cb: attrib_idx %d, service_id %d, len %d",
                     attrib_idx,
                     service_id, len);


    if (p_value == NULL)
    {
        APP_PRINT_ERROR0("app_chatgpt_ble_attr_write_cb: p_value is NULL");
        cause = APP_RESULT_APP_ERR;
        return cause;
    }

    switch (attrib_idx)
    {
    default:
        {
            cause = APP_RESULT_ATTR_NOT_FOUND;
        }
        break;

    case CHATGPT_RX_DATA_INDEX:
        {
            /* Notify Application. */
            srv_rx.msg_type = SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE;
            srv_rx.conn_id  = conn_id;
            srv_rx.attr_index = attrib_idx;
            srv_rx.msg_data.rx_data.len = len;
            srv_rx.msg_data.rx_data.p_value = p_value;

            if (chatgpt_srv_rx_cb)
            {
                chatgpt_srv_rx_cb(service_id, (void *)&srv_rx);
            }
        }
        break;
    }

    return cause;
}

// static T_APP_RESULT app_chatgpt_ble_attr_read_cb(uint8_t conn_id, T_SERVER_ID service_id,
//                                                uint16_t attrib_index,
//                                                uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
// {
//     T_APP_RESULT cause = APP_RESULT_SUCCESS;

//     APP_PRINT_TRACE0("app_chatgpt_ble_attr_read_cb");

//     return cause;
// }
#endif

static T_APP_RESULT app_chatgpt_ble_srv_rx_cb(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_APP_LE_LINK *p_link;
    T_CHATGPT_SRV_RX *srv_rx = (T_CHATGPT_SRV_RX *)p_data;

    if (srv_rx == NULL)
    {
        return APP_RESULT_APP_ERR;
    }

    APP_PRINT_INFO4("app_chatgpt_ble_srv_rx_cb: service_id %d conn_id %d, msg_type %d attr_index %d",
                    service_id, srv_rx->conn_id,
                    srv_rx->msg_type, srv_rx->attr_index);

    p_link = app_link_find_le_link_by_conn_id(srv_rx->conn_id);

    if (p_link == NULL)
    {
        return APP_RESULT_APP_ERR;
    }

    chatgpt_app_idx = p_link->id;

    if (srv_rx->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
    {
        uint8_t         *p_data;
        uint16_t        data_len;
        uint16_t        total_len;

        if ((srv_rx->msg_data.rx_data.p_value == NULL) || (srv_rx->msg_data.rx_data.len == 0))
        {
            return APP_RESULT_APP_ERR;
        }

        if (srv_rx->attr_index == CHATGPT_RX_DATA_INDEX)
        {
            app_chatgpt_transport_receive(srv_rx->msg_data.rx_data.p_value, srv_rx->msg_data.rx_data.len);
        }
    }
    else if (srv_rx->msg_type == SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION)
    {
        if (srv_rx->attr_index == CHATGPT_DATA_CCCD_INDEX)
        {
            if (srv_rx->msg_data.value == TRANSMIT_SVC_TX_DATA_CCCD_ENABLE)
            {
                p_link->chatgpt_transmit_srv_tx_enable_fg |= TX_ENABLE_CCCD_BIT;
            }
            else if (srv_rx->msg_data.value == TRANSMIT_SVC_TX_DATA_CCCD_DISABLE)
            {
                p_link->chatgpt_transmit_srv_tx_enable_fg &= ~TX_ENABLE_CCCD_BIT;
            }
        }
    }

    return app_result;
}

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
const T_FUN_GATT_EXT_SERVICE_CBS chatgpt_service_cbs =
#else
static const T_FUN_GATT_SERVICE_CBS chatgpt_service_cbs =
#endif
{
    NULL,
    app_chatgpt_ble_attr_write_cb,
    app_chatgpt_ble_cccd_update_cb,
};

static uint8_t app_chatgpt_ble_service_add(void *p_func)
{
#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    if (false == server_ext_add_service(&chatgpt_srv_id,
                                        (uint8_t *)chatgpt_bwps_attr_tbl,
                                        sizeof(chatgpt_bwps_attr_tbl),
                                        &chatgpt_service_cbs))
    {
        chatgpt_srv_id = 0xff;
    }

    chatgpt_srv_rx_cb = (P_FUN_EXT_SERVER_GENERAL_CB)p_func;
#else
    if (false == server_add_service(&chatgpt_srv_id,
                                    (uint8_t *)chatgpt_bwps_attr_tbl,
                                    sizeof(chatgpt_bwps_attr_tbl),
                                    chatgpt_service_cbs))
    {
        chatgpt_srv_id = 0xFF;
    }

    chatgpt_srv_rx_cb = (P_FUN_SERVER_GENERAL_CB)p_func;
#endif

    // APP_PRINT_TRACE1("app_chatgpt_ble_service_add: chatgpt_srv_id %d", chatgpt_srv_id);
    DBG_DIRECT("app_chatgpt_ble_service_add: chatgpt_srv_id %d", chatgpt_srv_id);

    return chatgpt_srv_id;
}

uint8_t app_chatgpt_ble_get_idx(void)
{
    return chatgpt_app_idx;
}

bool app_chatgpt_ble_tx_ready(void)
{
    if (app_db.le_link[chatgpt_app_idx].chatgpt_transmit_srv_tx_enable_fg & TX_ENABLE_CCCD_BIT)
    {
        return true;
    }

    return false;
}

bool app_chatgpt_transmit_srv_tx_data(uint8_t conn_id, uint16_t len, uint8_t *p_value)
{
    APP_PRINT_INFO2("app_chatgpt_transmit_srv_tx_data: conn_id %d chatgpt_srv_id %d", conn_id,
                    chatgpt_srv_id);

    if (chatgpt_srv_id == 0xFF)
    {
        return false;
    }

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
    return server_ext_send_data(le_get_conn_handle(conn_id), L2C_FIXED_CID_ATT, chatgpt_srv_id,
                                CHATGPT_TX_DATA_INDEX, p_value,
                                len, GATT_PDU_TYPE_ANY);
#else
    return server_send_data(conn_id, chatgpt_srv_id, CHATGPT_TX_DATA_INDEX, p_value,
                            len, GATT_PDU_TYPE_ANY);
#endif
}

void app_chatgpt_load_uuid(uint8_t *data)
{
    memcpy(data, chatgpt_uuid, sizeof(chatgpt_uuid));
}

uint8_t app_chatgpt_ble_init(void)
{
    return app_chatgpt_ble_service_add(app_chatgpt_ble_srv_rx_cb);
}
#endif

