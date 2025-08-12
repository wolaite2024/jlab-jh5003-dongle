/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <string.h>
#include <os_mem.h>
#include "ual_types.h"
#include "ual_list.h"
#include "ual_dev_mgr.h"
#include "ual_errno.h"
#include "ual_adapter.h"
#include "dev_mgr.h"
#include <trace.h>
#include "ual_bluetooth.h"
#include "gap_storage_le.h"
#include "aes_api.h"
#include "ual_api_types.h"
#include "connection_mgr.h"
#include "ble_privacy.h"
#include "bt_gatt_client.h"
#include "gap_bond_le.h"

#define UAL_DISC_LIST_LEN_MAX   (200)
typedef struct bt_device_mgr
{
    T_UALIST_HEAD sec_dev_list;
    T_UALIST_HEAD connect_list;
    T_UALIST_HEAD disc_list;
} T_BT_DEVICE_MGR;

static T_BT_DEVICE_MGR device_mgr;
static bool is_support_dongle_dual_mode = false;

static void bond_state_changed(T_BT_STATUS status, uint8_t *bd_addr, T_BLE_BD_TYPE bd_type,
                               T_BT_BOND_STATE state)
{
    APP_PRINT_INFO4("bond_state_changed: status %x bd_addr %s bd_type %x state %x", status,
                    TRACE_BDADDR(bd_addr), bd_type, state);

    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR1("bond_state_changed: can not find %s", TRACE_BDADDR(bd_addr));
        return;
    }

    if ((state == BT_BOND_STATE_BONDING) && (p_dev_rec->bond_state == BT_BOND_STATE_BONDING))
    {
        APP_PRINT_WARN2("bond_state_changed: bd_addr %s, same state %x", TRACE_BDADDR(bd_addr), state);
        return;
    }

    p_dev_rec->bond_state = state;

    //HAL_CBACK
    if (bt_hal_cbacks)
    {
        T_BT_BOND_INFO bond_info;
        bond_info.state = state;
        memcpy(bond_info.bd_addr, bd_addr, BD_ADDR_LEN);
        bond_info.bd_type = bd_type;
        bt_hal_cbacks(UAL_DEV_BOND_STATE_CHANGE, (uint8_t *)&bond_info, sizeof(bond_info));
    }
}

int bt_dev_mgr_init()
{
    init_ualist_head(&device_mgr.sec_dev_list);
    init_ualist_head(&device_mgr.connect_list);
    init_ualist_head(&device_mgr.disc_list);

    /* TODO load bonded devices */

    return 0;
}

static bool rpa_matches_irk(uint8_t rpa[6], uint8_t irk[16])
{
    uint8_t buffer[16];
    uint8_t irk_temp[16];
    uint8_t encrypt_buffer[16];
    uint8_t j;

    memset(buffer, 0x00, 16);
    buffer[13] = rpa[5];
    buffer[14] = rpa[4];
    buffer[15] = rpa[3];
    for (j = 0; j < 16; j++)
    {
        irk_temp[j] = irk[16 - 1 - j];
    }

    aes128_ecb_encrypt(buffer, irk_temp, encrypt_buffer);

    if ((encrypt_buffer[13] == rpa[2])
        && (encrypt_buffer[14] == rpa[1])
        && (encrypt_buffer[15] == rpa[0]))
    {
        // match
        return true;
    }
    // not a match
    return false;
}

bool ble_init_pseudo_addr(T_BT_DEVICE *p_dev_rec, uint8_t *new_pseudo_addr,
                          T_BLE_BD_TYPE new_bd_type)
{
    uint8_t empty_addr[] = {0, 0, 0, 0, 0, 0};
    if (memcmp(p_dev_rec->pseudo_addr, empty_addr, BD_ADDR_LEN) == 0)
    {
        memcpy(p_dev_rec->pseudo_addr, new_pseudo_addr, BD_ADDR_LEN);
        p_dev_rec->bd_type = new_bd_type;
        return true;
    }

    return false;
}

static bool ble_addr_resolvable(uint8_t *rpa, T_BT_DEVICE *p_dev_rec)
{
    if (!BLE_IS_RESOLVE_BDA(rpa))
    {
        return false;
    }

    if (p_dev_rec->has_irk)
    {
        APP_PRINT_INFO0("ble_addr_resolvable try to resolve");

        if (rpa_matches_irk(rpa, p_dev_rec->irk))
        {
            ble_init_pseudo_addr(p_dev_rec, rpa, BLE_ADDR_RANDOM);
            return true;
        }
    }
    return false;
}

static T_BT_DEVICE *ble_find_dev_by_identity_addr(uint8_t bd_addr[6], uint8_t bd_type)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_BT_DEVICE *device;

    if (ualist_empty(&device_mgr.sec_dev_list))
    {
        return NULL;
    }
    ualist_for_each_safe(pos, n, &device_mgr.sec_dev_list)
    {
        device = ualist_entry(pos, struct bt_device, list);
        if (!memcmp(device->identity_addr, bd_addr, BD_ADDR_LEN))
        {
            if ((device->identity_bd_type & (~BLE_BD_TYPE_ID_BIT)) !=
                (bd_type & (~BLE_BD_TYPE_ID_BIT)))
                APP_PRINT_INFO2("find pseudo->random match with diff addr type: %d vs %d",
                                device->identity_bd_type, bd_type);

            return device;
        }
    }
    return NULL;
}

T_BT_DEVICE *ble_resolve_random_addr(uint8_t *bd_addr)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;

    T_BT_DEVICE *device;
    /* start to resolve random address */
    /* check for next security record */

    ualist_for_each_safe(pos, n, &device_mgr.sec_dev_list)
    {
        device = ualist_entry(pos, struct bt_device, list);
        if (ble_addr_resolvable(bd_addr, device))
        {
            return device;
        }
    }

    return NULL;
}

bool ble_identity_addr_to_random_pseudo(uint8_t **bd_addr, T_BLE_BD_TYPE *p_bd_type,
                                        bool refresh)
{
    T_BT_DEVICE *device = ble_find_dev_by_identity_addr(*bd_addr, *p_bd_type);

    /* evt reported on static address, map static address to random pseudo */
    if (device != NULL)
    {
        /* if RPA offloading is supported, or 4.2 controller, do RPA refresh */

        //FIX TODO
        //if (refresh &&
        //controller_get_interface()->get_ble_resolving_list_max_size() != 0)
        //btm_ble_read_resolving_list_entry(p_dev_rec);

        /* assign the original address to be the current report address */
        if (!ble_init_pseudo_addr(device, *bd_addr, *p_bd_type))
        {
            memcpy(*bd_addr, device->pseudo_addr, BD_ADDR_LEN);
        }

        *p_bd_type = device->bd_type;
        return true;
    }

    return false;
}

T_BT_DEVICE *ual_get_first_sec_dev(void)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_BT_DEVICE *device;

    if (ualist_empty(&device_mgr.sec_dev_list))
    {
        APP_PRINT_ERROR0("ual_get_first_sec_dev list is empty");
        return NULL;
    }

    ualist_for_each_safe(pos, n, &device_mgr.sec_dev_list)
    {
        device = ualist_entry(pos, struct bt_device, list);
        break;
    }

    return device;
}

T_BT_DEVICE *ual_get_next_sec_dev(T_BT_DEVICE *p_sec_dev)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_BT_DEVICE *device;
    bool found_next = false;

    if (ualist_empty(&device_mgr.sec_dev_list))
    {
        APP_PRINT_ERROR0("ual_get_next_sec_dev list is empty");
        return NULL;
    }

    ualist_for_each_safe(pos, n, &device_mgr.sec_dev_list)
    {
        device = ualist_entry(pos, struct bt_device, list);
        if (!memcmp(device->bd_addr, p_sec_dev->bd_addr, BD_ADDR_LEN))
        {
            found_next = true;
        }
        else if (found_next)
        {
            return device;
        }
    }

    APP_PRINT_INFO0("ual_get_next_sec_dev list no next device");
    return NULL;
}


T_BT_DEVICE *ual_find_device_by_conn_id(uint8_t conn_id)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_BT_DEVICE *device;

    if (ualist_empty(&device_mgr.sec_dev_list))
    {
        APP_PRINT_ERROR0("ual_find_device_by_conn_id list is empty");
        return NULL;
    }

    ualist_for_each_safe(pos, n, &device_mgr.sec_dev_list)
    {
        device = ualist_entry(pos, struct bt_device, list);
        if (device->le_conn_id == conn_id)
        {
            return device;
        }
    }

    APP_PRINT_ERROR1("ual_find_device_by_conn_id list can't find device conn id %d", conn_id);
    return NULL;
}

T_BT_DEVICE *ual_find_device_by_conn_handle(uint16_t conn_handle)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_BT_DEVICE *device;

    if (ualist_empty(&device_mgr.sec_dev_list))
    {
        APP_PRINT_ERROR0("ual_find_device_by_conn_id list is empty");
        return NULL;
    }

    ualist_for_each_safe(pos, n, &device_mgr.sec_dev_list)
    {
        device = ualist_entry(pos, struct bt_device, list);
        if (device->le_conn_handle == conn_handle)
        {
            return device;
        }
    }

    APP_PRINT_ERROR1("ual_find_device_by_conn_handle list can't find device conn_handle 0x%x",
                     conn_handle);
    return NULL;
}


T_BT_DEVICE *ual_find_device_by_addr(uint8_t *bd_addr)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_BT_DEVICE *device;

    if (ualist_empty(&device_mgr.sec_dev_list))
    {
        //APP_PRINT_ERROR0("ual_find_device_by_addr list is empty");
        return NULL;
    }

    ualist_for_each_safe(pos, n, &device_mgr.sec_dev_list)
    {
        device = ualist_entry(pos, struct bt_device, list);
        if (!memcmp(device->bd_addr, bd_addr, BD_ADDR_LEN))
        {
            return device;
        }
        if (!memcmp(device->pseudo_addr, bd_addr, BD_ADDR_LEN))
        {
            return device;
        }
        if (!memcmp(device->identity_addr, bd_addr, BD_ADDR_LEN))
        {
            return device;
        }
        if (!memcmp(device->cur_rand_addr, bd_addr, BD_ADDR_LEN))
        {
            return device;
        }
        if (ble_addr_resolvable(bd_addr, device))
        {
            return device;
        }
    }

    APP_PRINT_ERROR1("ual_find_device_by_addr: list can't find device %s", TRACE_BDADDR(bd_addr));
    return NULL;
}

T_BT_DEVICE *ual_alloc_device_by_addr(uint8_t *bd_addr, T_BLE_BD_TYPE bd_type)
{
    struct bt_device *p_dev = NULL;
    T_LE_KEY_ENTRY *p_entry =  NULL;
    bool ret = false;
    T_DISC_RESULT *p_result = ual_find_disc_db_by_addr(bd_addr);

    APP_PRINT_TRACE2("ual_alloc_device_by_addr %s type : %d", TRACE_BDADDR(bd_addr), bd_type);

    p_dev = os_mem_zalloc(RAM_TYPE_BT_UAL, sizeof(T_BT_DEVICE));
    if (p_dev == NULL)
    {
        APP_PRINT_ERROR0("ual_alloc_device_by_addr alloc device fail");
        return NULL;
    }


    init_ualist_head(&p_dev->list);
    memcpy(p_dev->bd_addr, bd_addr, BD_ADDR_LEN);

    if (p_result != NULL)
    {
        p_dev->cod = p_result->cod;
        ble_init_pseudo_addr(p_dev, p_result->bd_addr, p_result->bd_type);
        APP_PRINT_WARN0("ual_alloc_device_by_addr use inquiry db address as pseudo_addr");
    }

    if ((bd_type == BLE_ADDR_PUBLIC_ID) ||
        (bd_type == BLE_ADDR_RANDOM_ID))
    {
        ble_init_pseudo_addr(p_dev, bd_addr, bd_type & (~BLE_BD_TYPE_ID_BIT));
    }
    else
    {
        ble_init_pseudo_addr(p_dev, bd_addr, bd_type);
    }

    if (bd_type & BLE_BD_TYPE_ID_BIT)
    {
        memcpy(p_dev->identity_addr, bd_addr, BD_ADDR_LEN);
        p_dev->identity_bd_type = bd_type;
    }

    p_entry = le_find_key_entry(bd_addr, (T_GAP_REMOTE_ADDR_TYPE)bd_type);
    if (p_entry != NULL)
    {
        p_dev->le_bonded = true;
        if (p_entry->flags & LE_KEY_STORE_REMOTE_LTK_BIT)
        {
            ret = le_get_dev_ltk(p_entry, true, &p_dev->ltk_len, p_dev->ltk);
            if (!ret)
            {
                APP_PRINT_INFO0("dev_mgr_handle_le_bond_info le_get_dev_ltk return NULL");
            }
        }
        if (p_entry->flags & LE_KEY_STORE_REMOTE_IRK_BIT)
        {
            if (le_get_dev_irk(p_entry, true, p_dev->irk))
            {
                p_dev->has_irk = true;
                p_dev->identity_bd_type = p_entry->resolved_remote_bd.remote_bd_type | BLE_BD_TYPE_ID_BIT;
                memcpy(p_dev->identity_addr, p_entry->resolved_remote_bd.addr, BD_ADDR_LEN);
            }
        }
    }

    p_dev->le_conn_id = 0xFF;
    ualist_add_tail(&p_dev->list, &device_mgr.sec_dev_list);
    return p_dev;
}


T_BT_DEVICE *ual_find_alloc_device_by_addr(uint8_t *bd_addr, T_BLE_BD_TYPE bd_type)
{
    T_BT_DEVICE *device = ual_find_device_by_addr(bd_addr);
    if (device != NULL)
    {
        return device;
    }

    device = ual_alloc_device_by_addr(bd_addr, bd_type);
    return device;
}

static void ual_check_disc_list_len(void)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_DISC_RESULT *device;
    T_BT_DEVICE *p_dev = NULL;

    if (ualist_len(&device_mgr.disc_list) < UAL_DISC_LIST_LEN_MAX)
    {
        return;
    }

    /*When disc list bigger than UAL_DISC_LIST_LEN_MAX, */
    /* release all devices except the device in sec list */
    ualist_for_each_safe(pos, n, &device_mgr.disc_list)
    {
        device = ualist_entry(pos, T_DISC_RESULT, list);
        p_dev = ual_find_device_by_addr(device->bd_addr);
        if (p_dev)
        {
            continue;
        }

        if (device->p_data)
        {
            os_mem_free(device->p_data);
            device->p_data = NULL;
            device->data_len = 0;
        }
        ualist_del(&device->list);
        os_mem_free(device);
    }
}

T_DISC_RESULT *ual_find_or_alloc_disc_db(uint8_t *bd_addr)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_DISC_RESULT *device;

    ualist_for_each_safe(pos, n, &device_mgr.disc_list)
    {
        device = ualist_entry(pos, T_DISC_RESULT, list);
        if (!memcmp(device->bd_addr, bd_addr, BD_ADDR_LEN))
        {
            return device;
        }
    }

    device = os_mem_zalloc(RAM_TYPE_BT_UAL, sizeof(T_DISC_RESULT));
    if (device == NULL)
    {
        APP_PRINT_ERROR1("ual_find_or_alloc_disc_db: %s alloc fail", TRACE_BDADDR(bd_addr));
        return NULL;
    }
    memcpy(device->bd_addr, bd_addr, 6);
    ual_check_disc_list_len();
    ualist_add_tail(&device->list, &device_mgr.disc_list);

    return device;
}

T_DISC_RESULT *ual_find_disc_db_by_addr(uint8_t *bd_addr)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_DISC_RESULT *device;

    if (ualist_empty(&device_mgr.disc_list))
    {
        return NULL;
    }

    ualist_for_each_safe(pos, n, &device_mgr.disc_list)
    {
        device = ualist_entry(pos, T_DISC_RESULT, list);
        if (!memcmp(device->bd_addr, bd_addr, BD_ADDR_LEN))
        {
            return device;
        }
    }

    APP_PRINT_ERROR0("ual_find_disc_db_by_addr find device fail");
    return NULL;
}


void clear_disc_db(void)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_DISC_RESULT *device;

    ualist_for_each_safe(pos, n, &device_mgr.disc_list)
    {
        ualist_del(pos);
        device = ualist_entry(pos, T_DISC_RESULT, list);
        if (device->p_data)
        {
            os_mem_free(device->p_data);
            device->p_data = NULL;
            device->data_len = 0;
        }
        os_mem_free(device);
    }
}

int bt_dev_create_bond(uint8_t *bd_addr, T_BLE_BD_TYPE bd_type)
{
    T_BT_DEVICE *p_dev_rec = ual_find_alloc_device_by_addr(bd_addr, bd_type);
    APP_PRINT_TRACE2("bt_dev_create_bond: bd_addr %s type %x", TRACE_BDADDR(bd_addr), bd_type);

    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("bt_dev_create_bond: alloc device fail");
        return BT_STATUS_FAIL;
    }

    if (p_dev_rec->bond_state == BT_BOND_STATE_BONDING)
    {
        return BT_STATUS_BUSY;
    }

    if (p_dev_rec->ble_state == BT_CONN_STATE_CONNECTED)
    {
        if (p_dev_rec->le_bonded)
        {
            bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                               BT_BOND_STATE_BONDED);
        }
        else
        {
            if (p_dev_rec->bond_state != BT_BOND_STATE_BONDING)
            {
                bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                                   BT_BOND_STATE_BONDING);
                le_bond_pair(p_dev_rec->le_conn_id);
            }
        }
    }
    else
    {
        if (bt_dev_le_gatt_connect(bd_addr, bd_type, DEV_MGR_APP_ID) != BT_STATUS_SUCCESS)
        {
            APP_PRINT_ERROR0("bt_dev_le_gatt_connect failed");
            return BT_STATUS_BUSY;
        }
    }

    return BT_STATUS_SUCCESS;
}


int bt_dev_clear_bond_info(uint8_t *bd_addr)
{
    struct bt_device *p_dev_rec = NULL;

    /* look up device */
    p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (!p_dev_rec)
    {
        return -ENODEV;
    }
    APP_PRINT_INFO1("bt_dev_remove_bond : bd_addr  %s", TRACE_BDADDR(bd_addr));

    APP_PRINT_INFO4("bt_dev_remove_bond : bd %s, p %s, i %s, r %s",
                    TRACE_BDADDR(p_dev_rec->bd_addr),
                    TRACE_BDADDR(p_dev_rec->pseudo_addr),
                    TRACE_BDADDR(p_dev_rec->identity_addr),
                    TRACE_BDADDR(p_dev_rec->cur_rand_addr));

    if (le_bond_delete_by_bd(p_dev_rec->pseudo_addr,
                             (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type) == GAP_CAUSE_SUCCESS)
    {

    }
    else if (p_dev_rec->has_irk)
    {
        le_bond_delete_by_bd(p_dev_rec->identity_addr,
                             (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->identity_bd_type);
    }
    else if (le_find_key_entry(p_dev_rec->pseudo_addr,
                               (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type) != NULL)
    {
        le_bond_delete_by_bd(p_dev_rec->pseudo_addr, (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type);
    }
    else if (le_find_key_entry(p_dev_rec->cur_rand_addr,
                               (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type) != NULL)
    {
        le_bond_delete_by_bd(p_dev_rec->cur_rand_addr, (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type);
    }

    p_dev_rec->le_bonded = false;
    p_dev_rec->has_irk = false;
    p_dev_rec->ltk_len = 0;
    return BT_STATUS_SUCCESS;
}

int bt_dev_remove_bond(uint8_t *bd_addr)
{
    struct bt_device *p_dev_rec = NULL;

    /* look up device */
    p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (!p_dev_rec)
    {
        return -ENODEV;
    }
    APP_PRINT_INFO1("bt_dev_remove_bond : bd_addr %s", TRACE_BDADDR(bd_addr));

    if (le_bond_delete_by_bd(p_dev_rec->pseudo_addr,
                             (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type) == GAP_CAUSE_SUCCESS)
    {

    }
    else if (p_dev_rec->has_irk)
    {
        le_bond_delete_by_bd(p_dev_rec->identity_addr,
                             (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->identity_bd_type);
    }
    else if (le_find_key_entry(p_dev_rec->pseudo_addr,
                               (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type) != NULL)
    {
        le_bond_delete_by_bd(p_dev_rec->pseudo_addr, (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type);
    }
    else if (le_find_key_entry(p_dev_rec->cur_rand_addr,
                               (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type) != NULL)
    {
        le_bond_delete_by_bd(p_dev_rec->cur_rand_addr, (T_GAP_REMOTE_ADDR_TYPE)p_dev_rec->bd_type);
    }

    p_dev_rec->le_bonded = false;
    p_dev_rec->has_irk = false;
    p_dev_rec->ltk_len = 0;

    /* disconnect if it's connected */
    APP_PRINT_INFO1("bt_dev_remove_bond : le_connect_mask 0x%x", p_dev_rec->le_connect_mask);
    T_APP_ID app_id = 0;
    uint32_t le_connect_mask = p_dev_rec->le_connect_mask;
    while (le_connect_mask != 0)
    {
        if (le_connect_mask & 0x01)
        {
            bt_dev_le_gatt_disconnect(p_dev_rec->pseudo_addr, app_id);
        }
        le_connect_mask = le_connect_mask >> 1;
        app_id++;
    }
    bt_dev_le_gatt_disconnect(p_dev_rec->pseudo_addr, DEV_MGR_APP_ID);

    if (p_dev_rec->ble_state == GAP_CONN_STATE_DISCONNECTED)
    {
        if (!p_dev_rec->le_bonded)
        {
            bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                               BT_BOND_STATE_NONE);
            ualist_del(&p_dev_rec->list);
            os_mem_free(p_dev_rec);
        }
    }

    return BT_STATUS_SUCCESS;
}

void bt_le_passkey_display_confirm(bool accept, uint8_t *bd_addr)
{
    uint8_t conn_id;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (p_dev_rec && le_get_conn_id_by_handle(p_dev_rec->le_conn_handle, &conn_id))
    {
        le_bond_passkey_display_confirm(conn_id, accept ? GAP_CFM_CAUSE_ACCEPT : GAP_CFM_CAUSE_REJECT);
    }
}

void bt_le_num_comp_confirm(bool accept, uint8_t *bd_addr)
{
    uint8_t conn_id;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (p_dev_rec && le_get_conn_id_by_handle(p_dev_rec->le_conn_handle, &conn_id))
    {
        le_bond_user_confirm(conn_id, accept ? GAP_CFM_CAUSE_ACCEPT : GAP_CFM_CAUSE_REJECT);
    }
}

void bt_le_passkey_input_confirm(bool accept, uint8_t *bd_addr, uint32_t passky)
{
    uint8_t conn_id;
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(bd_addr);
    if (p_dev_rec && le_get_conn_id_by_handle(p_dev_rec->le_conn_handle, &conn_id))
    {
        le_bond_passkey_input_confirm(conn_id, passky,
                                      accept ? GAP_CFM_CAUSE_ACCEPT : GAP_CFM_CAUSE_REJECT);
    }
}

void dev_mgr_handle_le_pairing_confirm(uint8_t conn_id, uint16_t subtype)
{
    T_GAP_CONN_INFO conn_info;
    if (!le_get_conn_info(conn_id, &conn_info))
    {
        if (subtype == GAP_MSG_LE_BOND_PASSKEY_DISPLAY)
        {
            le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_REJECT);
        }
        else if (subtype == GAP_MSG_LE_BOND_USER_CONFIRMATION)
        {
            le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_REJECT);
        }
        else if (subtype == GAP_MSG_LE_BOND_PASSKEY_INPUT)
        {
            le_bond_passkey_input_confirm(conn_id, 0, GAP_CFM_CAUSE_REJECT);
        }
        return;
    }
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_addr(conn_info.remote_bd);
    if (p_dev_rec == NULL || bt_hal_cbacks == NULL)
    {
        if (subtype == GAP_MSG_LE_BOND_PASSKEY_DISPLAY)
        {
            le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_REJECT);
        }
        else if (subtype == GAP_MSG_LE_BOND_USER_CONFIRMATION)
        {
            le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_REJECT);
        }
        else if (subtype == GAP_MSG_LE_BOND_PASSKEY_INPUT)
        {
            le_bond_passkey_input_confirm(conn_id, 0, GAP_CFM_CAUSE_REJECT);
        }

        APP_PRINT_ERROR0("dev_mgr_handle_le_pairing_confirm : can't find device reject");
        return;
    }


    if (subtype == GAP_MSG_LE_BOND_PASSKEY_DISPLAY)
    {
        T_BT_DISPLAY_INFO display_info;
        memcpy(display_info.bd_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
        le_bond_get_display_key(conn_id, &display_info.passkey);
        bt_hal_cbacks(UAL_ADP_BOND_PASSKEY_DISPLAY, (uint8_t *)&display_info, sizeof(display_info));
    }
    else if (subtype == GAP_MSG_LE_BOND_USER_CONFIRMATION)
    {
        T_BT_DISPLAY_INFO display_info;
        memcpy(display_info.bd_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
        le_bond_get_display_key(conn_id, &display_info.passkey);
        bt_hal_cbacks(UAL_ADP_BOND_USER_CONFIRMATION, (uint8_t *)&display_info, sizeof(display_info));
    }
    else if (subtype == GAP_MSG_LE_BOND_PASSKEY_INPUT)
    {
        T_PASSKEY_REQ req;
        memcpy(req.bd_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
        bt_hal_cbacks(UAL_ADP_BOND_PASSKEY_INPUT, (uint8_t *)&req, sizeof(req));
    }
}

void dev_mgr_handle_le_bond_info(T_LE_BOND_MODIFY_INFO *p_modify_info)
{
    T_UALIST_HEAD *pos;
    T_UALIST_HEAD *n;
    T_BT_DEVICE *p_dev_rec;
    uint8_t ret;

    T_LE_KEY_ENTRY *p_entry;

    if (p_modify_info->type == LE_BOND_ADD ||
        p_modify_info->type == LE_BOND_DELETE ||
        p_modify_info->type == LE_BOND_KEY_MISSING)
    {
        p_entry = p_modify_info->p_entry;
        if (p_entry == NULL)
        {
            APP_PRINT_ERROR1("dev_mgr_handle_le_bond_info type : %d, entry is NULL", p_modify_info->type);
            return;
        }
        p_dev_rec = ual_find_alloc_device_by_addr(p_modify_info->p_entry->remote_bd.addr,
                                                  p_modify_info->p_entry->remote_bd.remote_bd_type);
        if (p_dev_rec == NULL)
        {
            APP_PRINT_ERROR1("dev_mgr_handle_le_bond_info type : %d, device record is NULL",
                             p_modify_info->type);
            return;
        }
    }


    APP_PRINT_INFO1("dev_mgr_handle_le_bond_info type : %d", p_modify_info->type);
    if (p_modify_info->type == LE_BOND_ADD)
    {
        bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                           BT_BOND_STATE_BONDED);
        p_dev_rec->le_bonded = true;
        if (p_entry->flags & LE_KEY_STORE_REMOTE_LTK_BIT)
        {
            ret = le_get_dev_ltk(p_entry, true, &p_dev_rec->ltk_len, p_dev_rec->ltk);
            if (!ret)
            {
                APP_PRINT_INFO0("dev_mgr_handle_le_bond_info le_get_dev_ltk return NULL");
            }
        }
        if (p_entry->flags & LE_KEY_STORE_REMOTE_IRK_BIT)
        {
            if (le_get_dev_irk(p_entry, true, p_dev_rec->irk))
            {
                p_dev_rec->has_irk = true;
                p_dev_rec->identity_bd_type = p_entry->resolved_remote_bd.remote_bd_type | BLE_BD_TYPE_ID_BIT;
                memcpy(p_dev_rec->identity_addr, p_entry->resolved_remote_bd.addr, BD_ADDR_LEN);
            }
        }
#if BLE_PRIVACY_SPT
        ble_resolving_list_load_dev(p_dev_rec);
#endif
    }
    else if (p_modify_info->type == LE_BOND_DELETE)
    {
#if BLE_PRIVACY_SPT
        ble_resolving_list_remove_dev(p_dev_rec);
#endif
        p_dev_rec->has_irk = 0;
        memset(p_dev_rec->ltk, 0, 16);
        memset(p_dev_rec->irk, 0, 16);
        bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                           BT_BOND_STATE_NONE);
        p_dev_rec->le_bonded = false;
    }
    else if (p_modify_info->type == LE_BOND_CLEAR)
    {
        ualist_for_each_safe(pos, n, &device_mgr.sec_dev_list)
        {
            p_dev_rec = ualist_entry(pos, struct bt_device, list);
#if BLE_PRIVACY_SPT
            ble_resolving_list_remove_dev(p_dev_rec);
#endif
            p_dev_rec->le_bonded = false;
            p_dev_rec->has_irk = 0;
            memset(p_dev_rec->ltk, 0, 16);
            memset(p_dev_rec->irk, 0, 16);


            bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                               BT_BOND_STATE_NONE);
            if (p_dev_rec->ble_state == GAP_CONN_STATE_DISCONNECTED)
            {
                ualist_del(&p_dev_rec->list);
                os_mem_free(p_dev_rec);
            }
        }
    }
    else if (p_modify_info->type == LE_BOND_KEY_MISSING)
    {
        APP_PRINT_INFO0("dev_mgr_handle_le_bond_info: KEY missing");
    }
}

void dev_mgr_handle_le_conn_state(T_BT_DEVICE *p_dev_rec, T_GAP_CONN_STATE new_state,
                                  uint16_t disc_cause)
{
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("dev_mgr_handle_le_conn_state: device is NULL");
        return;
    }

    APP_PRINT_INFO3("dev_mgr_handle_le_conn_state: bd_addr %s, conn_state %d, disc_cause 0x%x",
                    TRACE_BDADDR(p_dev_rec->bd_addr), new_state, disc_cause);
    switch (new_state)
    {
    case GAP_CONN_STATE_CONNECTED:
        {
            APP_PRINT_INFO2("dev_mgr_handle_le_conn_state: pair %s le_bonded %d",
                            TRACE_BDADDR(p_dev_rec->pseudo_addr), p_dev_rec->le_bonded);
            if (p_dev_rec->le_bonded)
            {
                bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr,
                                   p_dev_rec->bd_type, BT_BOND_STATE_BONDED);
            }
            else
            {
                bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr,
                                   p_dev_rec->bd_type, BT_BOND_STATE_BONDING);
            }
            le_bond_pair(p_dev_rec->le_conn_id);

        }
        break;
    case GAP_CONN_STATE_DISCONNECTED:
        {
            if (!p_dev_rec->le_bonded)
            {
                bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr,
                                   p_dev_rec->bd_type, BT_BOND_STATE_NONE);
                ualist_del(&p_dev_rec->list);
                os_mem_free(p_dev_rec);
            }
        }
        break;
    default:
        break;
    }

}

void dev_mgr_handle_le_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    T_GAP_CONN_INFO conn_info;
    T_BT_DEVICE *p_dev_rec;
    APP_PRINT_INFO3("dev_mgr_handle_le_authen_state_evt:conn_id %d, state %x, cause 0x%x",
                    conn_id, new_state, cause);

    if (!le_get_conn_info(conn_id, &conn_info))
    {
        APP_PRINT_ERROR1("dev_mgr_handle_le_authen_state_evt: can't find conn info %d", conn_id);
        return;
    }

    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
        {
            APP_PRINT_INFO0("dev_mgr_handle_le_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
        }
        break;
    case GAP_AUTHEN_STATE_COMPLETE:
        {
            p_dev_rec = ual_find_alloc_device_by_addr(conn_info.remote_bd, conn_info.remote_bd_type);
            if (p_dev_rec == NULL)
            {
                break;
            }
            if (cause == GAP_SUCCESS)
            {
                p_dev_rec->le_conn_handle = le_get_conn_handle(conn_id);
                p_dev_rec->ble_state = GAP_CONN_STATE_CONNECTED;
                p_dev_rec->auth_cmpl = true;
                if (p_dev_rec->mtu_received)
                {
                    gatt_client_start_discovery_all(p_dev_rec->le_conn_handle, ble_gatt_client_discover_cb);
                }

                if (bt_hal_cbacks)
                {
                    T_BT_AUTH_INFO auth_ifo;
                    auth_ifo.status = BT_STATUS_AUTH_COMPLETE;
                    memcpy(auth_ifo.bd_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
                    auth_ifo.bd_type = p_dev_rec->bd_type;
                    auth_ifo.transport_type = TRANSPORT_TYPE_LE;
                    bt_hal_cbacks(UAL_ADP_DEV_AUTH_CMPLT, (uint8_t *)&auth_ifo, sizeof(auth_ifo));
                }
            }
            else
            {
                bond_state_changed(BT_STATUS_SUCCESS, p_dev_rec->pseudo_addr, p_dev_rec->bd_type,
                                   BT_BOND_STATE_NONE);
                p_dev_rec->auth_cmpl = false;
                if (cause == (HCI_ERR | HCI_ERR_KEY_MISSING))
                {
                    if (bt_hal_cbacks)
                    {
                        T_BT_AUTH_INFO auth_info;
                        auth_info.status = BT_STATUS_AUTH_KEY_MISSING;
                        memcpy(auth_info.bd_addr, p_dev_rec->pseudo_addr, BD_ADDR_LEN);
                        auth_info.bd_type = p_dev_rec->bd_type;
                        auth_info.transport_type = TRANSPORT_TYPE_LE;
                        bt_hal_cbacks(UAL_ADP_DEV_AUTH_CMPLT, (uint8_t *)&auth_info, sizeof(auth_info));
                    }
                }
            }
        }
        break;

    default:
        {
            APP_PRINT_ERROR1("dev_mgr_handle_le_authen_state_evt: unknown newstate %d", new_state);
        }
        break;
    }
}

bool bt_dev_register_le_conn_callback(T_APP_ID app_id, T_LE_CONN_CBACK callback)
{
    return ble_register_conn_callback_by_id(app_id, callback);
}

void bt_dev_unregister_le_conn_callback(T_APP_ID app_id)
{
    ble_unregister_conn_callback_by_id(app_id);
}


int bt_dev_le_gatt_connect(uint8_t *bd_addr, T_BLE_BD_TYPE bd_type, T_APP_ID app_id)
{
    T_BT_DEVICE *p_dev_rec = ual_find_alloc_device_by_addr(bd_addr, bd_type);
#if F_CONNECT_BY_WHITELIST
    bool add_to_wl = true;
#else
    bool add_to_wl = false;
#endif
    if (is_support_dongle_dual_mode)
    {
        add_to_wl = false;
    }

    APP_PRINT_INFO2("bt_dev_le_gatt_connect: app_id %d, add_to_wl %d", app_id, add_to_wl);
    if (p_dev_rec == NULL)
    {
        APP_PRINT_ERROR0("bt_dev_le_gatt_connect: alloc device fail");
        return BT_STATUS_FAIL;
    }

    ble_init_pseudo_addr(p_dev_rec, bd_addr, bd_type);
    bt_dev_register_le_conn_callback(app_id, le_conn_cback);
    if (app_id > APP_CONN_ID_MAX)
    {
        APP_PRINT_ERROR1("bt_dev_le_gatt_connect: app_id too large %d", app_id);
        return BT_STATUS_FAIL;
    }
    if (!ble_conn_mgr_connect(p_dev_rec->pseudo_addr, p_dev_rec->bd_type, add_to_wl))
    {
        APP_PRINT_ERROR0("bt_dev_le_gatt_connect: connect fail");
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

int bt_dev_le_gatt_disconnect(uint8_t *bd_addr, T_APP_ID app_id)
{
    if (!ble_conn_mgr_disconnect(bd_addr, app_id))
    {
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}


void le_conn_cback(uint8_t conn_id, uint8_t *bd_addr, uint8_t state)
{
    APP_PRINT_TRACE3("le_conn_cback: conn_id %d, bd_addr %s, state %d", conn_id, TRACE_BDADDR(bd_addr),
                     state);
    T_BT_DEVICE *p_dev_rec = ual_find_device_by_conn_id(conn_id);
    if (!p_dev_rec)
    {
        APP_PRINT_ERROR2("le_conn_cback: can not find conn_id %x bd_addr %s", conn_id,
                         TRACE_BDADDR(bd_addr));
        return;
    }
}

void bt_dev_set_support_dongle_dual_mode(void)
{
    is_support_dongle_dual_mode = true;
}


