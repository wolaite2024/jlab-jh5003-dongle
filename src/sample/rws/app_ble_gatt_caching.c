/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if F_APP_GATT_CACHING_SUPPORT
#include "ftl.h"
#include "string.h"
#include "trace.h"
#include "app_cfg.h"
#include "app_main.h"
#include "bt_types.h"
#include "app_link_util.h"
#include "bt_bond_le_sync.h"
#include "gatt_builtin_services.h"
#include "app_ble_gatt_caching.h"
#include "gatt_server_service_change.h"

#define GATT_CACHING_DATABASE_HASH_SIZE 16
#define GATT_CACHING_MAX_BOND_DEVICE_NUM 8

typedef struct
{
    uint8_t is_used;
    uint8_t bond_entry_idx;
    uint8_t service_change_state;
    uint8_t rsv;
} T_SERVICE_CHANGE_STATE_INFO;

typedef struct
{
    uint8_t database_hash[GATT_CACHING_DATABASE_HASH_SIZE];
    T_SERVICE_CHANGE_STATE_INFO service_change_info[GATT_CACHING_MAX_BOND_DEVICE_NUM];
} T_GATT_CACHING_INFO_STORE;

static T_GATT_CACHING_INFO_STORE gatt_chaching_info;
extern bool le_get_conn_local_addr(uint16_t conn_handle, uint8_t *bd_addr, uint8_t *bd_type);

static void app_ble_gatt_caching_save_service_change_state(uint8_t idx,
                                                           uint8_t service_change_state)
{
    APP_PRINT_INFO2("app_ble_gatt_caching_save_service_change_state: idx %d, service_change_state %d",
                    idx, service_change_state);

    gatt_chaching_info.service_change_info[idx].is_used = true;
    gatt_chaching_info.service_change_info[idx].bond_entry_idx = idx;
    gatt_chaching_info.service_change_info[idx].service_change_state = service_change_state;

    uint16_t addr_offset = APP_RW_GATT_CACHING_INFO_ADDR + GATT_CACHING_DATABASE_HASH_SIZE +
                           (idx * sizeof(T_SERVICE_CHANGE_STATE_INFO));

    ftl_save_to_storage(&gatt_chaching_info.service_change_info[idx], addr_offset,
                        sizeof(T_SERVICE_CHANGE_STATE_INFO));
}

static void app_ble_gatt_caching_save_database_hash(uint8_t *p_database_hash)
{
    APP_PRINT_INFO2("app_ble_gatt_caching_save_database_hash: %b, offset 0x%x",
                    TRACE_BINARY(16, p_database_hash), APP_RW_GATT_CACHING_INFO_ADDR);

    memcpy(gatt_chaching_info.database_hash, p_database_hash, 16);
    ftl_save_to_storage(gatt_chaching_info.database_hash, APP_RW_GATT_CACHING_INFO_ADDR, 16);
}

static void app_ble_gatt_caching_load_service_change_state(uint8_t idx,
                                                           T_SERVICE_CHANGE_STATE_INFO *service_change_info)
{
    uint16_t addr_offset = APP_RW_GATT_CACHING_INFO_ADDR + GATT_CACHING_DATABASE_HASH_SIZE +
                           (idx * sizeof(T_SERVICE_CHANGE_STATE_INFO));

    ftl_load_from_storage(service_change_info, addr_offset, sizeof(T_SERVICE_CHANGE_STATE_INFO));
    gatt_chaching_info.service_change_info[idx] = *service_change_info;

    APP_PRINT_INFO2("app_ble_gatt_caching_load_service_change_state: idx %d, service_change_info %b",
                    idx, TRACE_BINARY(sizeof(T_SERVICE_CHANGE_STATE_INFO), service_change_info));
}

static void app_ble_gatt_caching_load_database_hash(uint8_t *p_database_hash)
{
    ftl_load_from_storage(p_database_hash, APP_RW_GATT_CACHING_INFO_ADDR, 16);
    memcpy(gatt_chaching_info.database_hash, p_database_hash, 16);
    APP_PRINT_INFO2("app_ble_gatt_caching_load_database_hash: %b, offset 0x%x",
                    TRACE_BINARY(16, p_database_hash), APP_RW_GATT_CACHING_INFO_ADDR);
}

static void app_ble_gatt_caching_clear_service_change_state_by_idx(uint8_t idx)
{
    gatt_chaching_info.service_change_info[idx].is_used = false;
    gatt_chaching_info.service_change_info[idx].bond_entry_idx = 0xFF;
    gatt_chaching_info.service_change_info[idx].service_change_state = 0xFF;

    uint16_t addr_offset = APP_RW_GATT_CACHING_INFO_ADDR + GATT_CACHING_DATABASE_HASH_SIZE +
                           (idx * sizeof(T_SERVICE_CHANGE_STATE_INFO));

    ftl_save_to_storage(&gatt_chaching_info.service_change_info[idx], addr_offset,
                        sizeof(T_SERVICE_CHANGE_STATE_INFO));

    APP_PRINT_INFO2("app_ble_gatt_caching_clear_service_change_state_by_idx: idx %d, offset 0x%x",
                    idx, addr_offset);
}

void app_ble_gatt_caching_clear_service_change_state_all(void)
{
    APP_PRINT_INFO0("app_ble_gatt_caching_clear_service_change_state_all");

    for (uint8_t idx = 0; idx < GATT_CACHING_MAX_BOND_DEVICE_NUM; idx++)
    {
        app_ble_gatt_caching_clear_service_change_state_by_idx(idx);
    }
}

void app_ble_gatt_caching_clear_database_hash(void)
{
    APP_PRINT_INFO0("app_ble_gatt_caching_clear_database_hash");
    memset(gatt_chaching_info.database_hash, 0, 16);
    ftl_save_to_storage(gatt_chaching_info.database_hash, APP_RW_GATT_CACHING_INFO_ADDR, 16);
}

static void app_ble_gatt_caching_handle_set_service_change_state(T_GAP_CB_DATA cb_data)
{
    uint8_t status_code = 0;
    uint8_t service_change;
    uint8_t service_change_state;

    T_GATT_SERVER_SERVICE_CHANGE_CB *p_gatt_server_service_change_info = cb_data.p_gap_cb_data;

    service_change = p_gatt_server_service_change_info-> \
                     data.p_gatt_server_service_change_set_service_change_state_ind->service_change;

    service_change_state = p_gatt_server_service_change_info-> \
                           data.p_gatt_server_service_change_set_service_change_state_ind->service_change_state;

    if (service_change == 0)
    {
        // Service is not changed
        uint16_t conn_handle;
        uint8_t remote_addr[6];
        uint8_t remote_addr_type;
        uint8_t resolved_addr[6];
        uint8_t resolved_bd_type;
        uint8_t local_bd[GAP_BD_ADDR_LEN];
        uint8_t local_bd_type = LE_BOND_LOCAL_ADDRESS_ANY;

        conn_handle = p_gatt_server_service_change_info-> \
                      data.p_gatt_server_service_change_set_service_change_state_ind->conn_handle;

        remote_addr_type = p_gatt_server_service_change_info-> \
                           data.p_gatt_server_service_change_set_service_change_state_ind->remote_addr_type;

        memcpy(remote_addr, p_gatt_server_service_change_info-> \
               data.p_gatt_server_service_change_set_service_change_state_ind->remote_bd_addr, 6);

        le_get_conn_local_addr(conn_handle, local_bd, &local_bd_type);

        if (bt_le_resolve_random_address(remote_addr, resolved_addr,
                                         (T_GAP_IDENT_ADDR_TYPE *)&resolved_bd_type) == true)
        {
            T_LE_BOND_ENTRY *p_key_entry = NULL;
            p_key_entry = bt_le_find_key_entry(resolved_addr, (T_GAP_REMOTE_ADDR_TYPE)resolved_bd_type,
                                               local_bd, local_bd_type);

            if (p_key_entry != NULL)
            {
                // Specific device is bonded, APP save @ref service_change_state
                app_ble_gatt_caching_save_service_change_state(p_key_entry->idx, service_change_state);
                status_code = 1;
            }
            else
            {
                // Specific device is not bonded, APP update temporary service change state with @ref service_change_state
                T_APP_LE_LINK *p_le_link = app_link_find_le_link_by_conn_handle(conn_handle);
                p_le_link->service_change_state = service_change_state;
                status_code = 2;
            }
        }
        else
        {
            T_LE_BOND_ENTRY *p_key_entry = NULL;
            p_key_entry = bt_le_find_key_entry(remote_addr, (T_GAP_REMOTE_ADDR_TYPE)remote_addr_type,
                                               local_bd, local_bd_type);

            if (p_key_entry != NULL)
            {
                // Specific device is bonded, APP save @ref service_change_state
                app_ble_gatt_caching_save_service_change_state(p_key_entry->idx, service_change_state);
                status_code = 3;
            }
            else
            {
                // Specific device is not bonded, APP update temporary service change state with @ref service_change_state
                T_APP_LE_LINK *p_le_link = app_link_find_le_link_by_conn_handle(conn_handle);
                p_le_link->service_change_state = service_change_state;
                status_code = 4;
            }
        }
    }
    else if (service_change == 1)
    {
        // Service is changed
        // Step 1: For each bonded device, APP save "new service change state".
        uint8_t bond_num = bt_le_get_max_le_paired_device_num();
        uint8_t new_service_change_state;
        status_code = 5;

        for (uint8_t i = 0; i < bond_num; i++)
        {
            T_LE_BOND_ENTRY *p_entry = bt_le_find_key_entry_by_priority(i + 1);

            if (p_entry != NULL)
            {
                uint16_t info_len;
                T_BT_LE_DEV_DATA *p_info = NULL;

                if (bt_le_dev_info_get(p_entry, &p_info, &info_len))
                {
                    if (p_info->dev_info.client_supported_feature[0] &&
                        GATTS_CLIENT_SUPPORTED_FEATURES_ROBUST_CACHING_BIT)
                    {
                        // Support Robust Caching: "new service change state" is @ref service_change_state .
                        new_service_change_state = service_change_state;
                    }
                    else
                    {
                        // Not support Robust Caching: "new service change state" is (service_change_state & (~(1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_CHANGE_UNAWARE)))
                        new_service_change_state = (service_change_state &
                                                    (~(1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_CHANGE_UNAWARE)));
                    }
                }

                // APP save "new service change state"
                app_ble_gatt_caching_save_service_change_state(p_entry->idx, new_service_change_state);
            }
        }

        // Step 2: Start
        // For each connected device
        uint8_t le_link_num = app_link_get_le_link_num();

        for (uint8_t i = 0; i < le_link_num; i++)
        {
            // Check whether device support Robust Caching (Bit 0 of Octet 0 in the Client Supported Features characteristic value is 1)
            if (app_db.le_link[i].robust_caching_support)
            {
                // Support Robust Caching: "new service change state" is @ref service_change_state .
                new_service_change_state = service_change_state;
            }
            else
            {
                // Not support Robust Caching: "new service change state" is (@ref service_change_state & (~(1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_CHANGE_UNAWARE))).
                new_service_change_state = (service_change_state &
                                            (~(1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_CHANGE_UNAWARE)));
            }

            // APP update temporary service change state with "new service change state"
            app_db.le_link[i].service_change_state = new_service_change_state;

            // APP call @ref gatt_server_service_change_update_service_change_state with "new service change state"
            T_GATT_SERVER_SERVICE_CHANGE_UPDATE_SERVICE_CHANGE_STATE_PARAM param;

            param.conn_handle = app_db.le_link[i].conn_handle;
            param.service_change_state = new_service_change_state;

            gatt_server_service_change_update_service_change_state(&param);

            if (param.service_change_state & (1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_SEND_INDICATION))
            {
                // APP send Service Changed Indication
                uint16_t start_handle = 1;
                uint16_t end_handle = 0xFFFF;

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
                gatts_ext_service_changed_indicate(app_db.le_link[i].conn_handle, L2C_FIXED_CID_ATT, start_handle,
                                                   end_handle);
#else
                gatts_service_changed_indicate(app_db.le_link[i].conn_id, start_handle, end_handle);
#endif
            }
        }
    }

    APP_PRINT_INFO3("app_ble_gatt_caching_handle_set_service_change_state: state %d, service_change %d, status_code %d",
                    service_change_state, service_change, -status_code);
}

static void app_ble_gatt_caching_handle_get_service_change_state(T_GAP_CB_DATA cb_data)
{
    uint16_t conn_handle;
    uint8_t remote_addr[6];
    uint8_t remote_addr_type;
    uint8_t resolved_addr[6];
    uint8_t resolved_bd_type;
    uint8_t local_bd[GAP_BD_ADDR_LEN];
    uint8_t local_bd_type = LE_BOND_LOCAL_ADDRESS_ANY;
    uint8_t status_code = 0;

    T_GATT_SERVER_SERVICE_CHANGE_CB *p_gatt_server_service_change_info = cb_data.p_gap_cb_data;

    conn_handle = p_gatt_server_service_change_info-> \
                  data.p_gatt_server_service_change_get_service_change_state_ind->conn_handle;

    remote_addr_type = p_gatt_server_service_change_info-> \
                       data.p_gatt_server_service_change_get_service_change_state_ind->remote_addr_type;

    memcpy(remote_addr, p_gatt_server_service_change_info-> \
           data.p_gatt_server_service_change_get_service_change_state_ind->remote_bd_addr, 6);

    le_get_conn_local_addr(conn_handle, local_bd, &local_bd_type);

    T_GATT_SERVER_SERVICE_CHANGE_UPDATE_SERVICE_CHANGE_STATE_PARAM param;
    param.conn_handle = conn_handle;

    if (bt_le_resolve_random_address(remote_addr, resolved_addr,
                                     (T_GAP_IDENT_ADDR_TYPE *)&resolved_bd_type) == true)
    {
        T_LE_BOND_ENTRY *p_key_entry = NULL;
        p_key_entry = bt_le_find_key_entry(resolved_addr, (T_GAP_REMOTE_ADDR_TYPE)resolved_bd_type,
                                           local_bd, local_bd_type);

        if (p_key_entry != NULL)
        {
            // If device is bonded, service change state is previous state that is saved by APP.
            T_SERVICE_CHANGE_STATE_INFO service_change_info;

            app_ble_gatt_caching_load_service_change_state(p_key_entry->idx, &service_change_info);

            if (service_change_info.is_used)
            {
                param.service_change_state = service_change_info.service_change_state;
                status_code = 1;
            }
            else
            {
                param.service_change_state = 0;
                status_code = 2;
            }

            gatt_server_service_change_update_service_change_state(&param);
        }
        else
        {
            // If device is not bonded, service change state is 0 .
            param.service_change_state = 0;
            status_code = 3;

            gatt_server_service_change_update_service_change_state(&param);
        }
    }
    else
    {
        T_LE_BOND_ENTRY *p_key_entry = NULL;
        p_key_entry = bt_le_find_key_entry(remote_addr, (T_GAP_REMOTE_ADDR_TYPE)remote_addr_type,
                                           local_bd, local_bd_type);

        if (p_key_entry != NULL)
        {
            // If device is bonded, service change state is previous state that is saved by APP.
            T_SERVICE_CHANGE_STATE_INFO service_change_info;
            app_ble_gatt_caching_load_service_change_state(p_key_entry->idx, &service_change_info);

            if (service_change_info.is_used)
            {
                param.service_change_state = service_change_info.service_change_state;
                status_code = 4;
            }
            else
            {
                param.service_change_state = 0;
                status_code = 5;
            }

            gatt_server_service_change_update_service_change_state(&param);
        }
        else
        {
            // If device is not bonded, service change state is 0 .
            param.service_change_state = 0;
            status_code = 6;
            gatt_server_service_change_update_service_change_state(&param);
        }
    }

    if (param.service_change_state & (1 << GATT_SERVER_SERVICE_CHANGE_STATE_BIT_SEND_INDICATION))
    {
        // APP send Service Changed Indication
        uint16_t start_handle = 1;
        uint16_t end_handle = 0xFFFF;
        status_code = 7;

#if F_APP_GATT_SERVER_EXT_API_SUPPORT
        gatts_ext_service_changed_indicate(conn_handle, L2C_FIXED_CID_ATT, start_handle, end_handle);
#else
        uint8_t conn_id;
        le_get_conn_id_by_handle(conn_handle, &conn_id);
        gatts_service_changed_indicate(conn_id, start_handle, end_handle);
#endif
    }

    APP_PRINT_INFO1("app_ble_gatt_caching_handle_get_service_change_state: status_code %d",
                    -status_code);
}

static void app_ble_gatt_caching_handle_set_database_hash_info(T_GAP_CB_DATA cb_data)
{
    // APP save database hash
    APP_PRINT_INFO0("app_ble_gatt_caching_handle_set_database_hash_info");

    uint8_t database_hash[16];
    T_GATT_SERVER_SERVICE_CHANGE_CB *p_gatt_server_service_change_info = cb_data.p_gap_cb_data;

    memcpy(database_hash, p_gatt_server_service_change_info-> \
           data.p_gatt_server_service_change_set_database_hash_info->database_hash, 16);

    app_ble_gatt_caching_save_database_hash(database_hash);
}

static void app_ble_gatt_caching_handle_get_database_hash_info(void)
{
    APP_PRINT_INFO0("app_ble_gatt_caching_handle_get_database_hash_info");

    T_GATT_SERVER_SERVICE_CHANGE_UPDATE_DATABASE_HASH_PARAM param;

    app_ble_gatt_caching_load_database_hash(param.database_hash);
    gatt_server_service_change_update_database_hash(&param);
}

void app_ble_gatt_caching_common_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_CB_DATA));
    APP_PRINT_INFO1("app_ble_gatt_caching_common_callback: cb_type = 0x%x", cb_type);

    switch (cb_type)
    {
    case GAP_MSG_GATT_SERVER_SERVICE_CHANGE_INFO:
        {
            T_GATT_SERVER_SERVICE_CHANGE_CB *p_gatt_server_service_change_info = cb_data.p_gap_cb_data;

            switch (p_gatt_server_service_change_info->opcode)
            {
            case GATT_SERVER_SERVICE_CHANGE_SET_SERVICE_CHANGE_STATE_IND:
                {
                    app_ble_gatt_caching_handle_set_service_change_state(cb_data);
                }
                break;

            case GATT_SERVER_SERVICE_CHANGE_GET_SERVICE_CHANGE_STATE_IND:
                {
                    app_ble_gatt_caching_handle_get_service_change_state(cb_data);
                }
                break;

            case GATT_SERVER_SERVICE_CHANGE_SET_DATABASE_HASH_INFO:
                {
                    app_ble_gatt_caching_handle_set_database_hash_info(cb_data);
                }
                break;

            case GATT_SERVER_SERVICE_CHANGE_GET_DATABASE_HASH_IND:
                {
                    app_ble_gatt_caching_handle_get_database_hash_info();
                }
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }
    return;
}

T_APP_RESULT app_ble_gatt_caching_service_callback(T_SERVER_ID service_id, void *p_para)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GAPS_CALLBACK_DATA *p_gap_data = (T_GAPS_CALLBACK_DATA *)p_para;

    APP_PRINT_INFO2("app_ble_gatt_caching_service_callback conn_id = %d msg_type = %d\n",
                    p_gap_data->conn_id, p_gap_data->msg_type);

    if (p_gap_data->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
    {
        switch (p_gap_data->msg_data.opcode)
        {
        case GATT_SERVICE_WRITE_CLIENT_SUPPORTED_FEATURES:
            {
                T_APP_LE_LINK *p_le_link = app_link_find_le_link_by_conn_id(p_gap_data->conn_id);

                if (p_gap_data->msg_data.p_value[0] && GATTS_CLIENT_SUPPORTED_FEATURES_ROBUST_CACHING_BIT)
                {
                    p_le_link->robust_caching_support = true;
                }
                else
                {
                    p_le_link->robust_caching_support = false;
                }

                APP_PRINT_TRACE2("app_ble_gatt_caching_service_callback: length %d, client supported features %s",
                                 p_gap_data->msg_data.len, TRACE_BINARY(p_gap_data->msg_data.len, p_gap_data->msg_data.p_value));
            }
            break;

        default:
            break;
        }
    }
    return result;
}

void app_ble_gatt_caching_bond_sync_cb(uint8_t cb_type, void *p_cb_data)
{
    T_BT_LE_BOND_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_BT_LE_BOND_CB_DATA));
    APP_PRINT_INFO2("app_ble_gatt_caching_bond_sync_cb: cb_type = %d, app_cfg_nv.bud_role %d",
                    cb_type, app_cfg_nv.bud_role);

    switch (cb_type)
    {
    case BT_BOND_MSG_LE_BOND_REMOVE:
        {
            app_ble_gatt_caching_clear_service_change_state_by_idx(cb_data.p_le_bond_remove->p_entry->idx);
        }
        break;

    case BT_BOND_MSG_LE_BOND_CLEAR:
        {
            app_ble_gatt_caching_clear_service_change_state_all();
        }
        break;

    default:
        break;
    }
    return;
}

void app_ble_gatt_caching_init(void)
{
    APP_PRINT_INFO1("app_ble_gatt_caching_init: offset 0x%x", APP_RW_GATT_CACHING_INFO_ADDR);
    uint32_t read_result = ftl_load_from_storage(&gatt_chaching_info, APP_RW_GATT_CACHING_INFO_ADDR,
                                                 sizeof(T_GATT_CACHING_INFO_STORE));

    if (read_result == ENOF)
    {
        memset(&gatt_chaching_info, 0, sizeof(T_GATT_CACHING_INFO_STORE));
        ftl_save_to_storage(&gatt_chaching_info, APP_RW_GATT_CACHING_INFO_ADDR,
                            sizeof(T_GATT_CACHING_INFO_STORE));
    }
    else
    {
        APP_PRINT_INFO1("app_ble_gatt_caching_init: database_hash 0x%b",
                        TRACE_BINARY(16, gatt_chaching_info.database_hash));
    }

    gatt_register_callback(app_ble_gatt_caching_service_callback);
    bt_bond_register_app_cb(app_ble_gatt_caching_bond_sync_cb);
}
#endif
