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
#include "dev_mgr.h"
#include <trace.h>
#include "ual_bluetooth.h"
#include "gap_storage_le.h"
#include "gap_bond_le.h"
#include "bt_storage_mgr.h"
#include "ble_privacy.h"

void bt_storage_load_devices(void)
{
    uint8_t le_devs_num;
    uint8_t devs_pri_table[20] = {0};
    if (!le_get_bond_priority(&le_devs_num, devs_pri_table))
    {
        APP_PRINT_INFO0("bt_storage_load_devices: get bonded device fail");
        return;
    }
    APP_PRINT_INFO1("bt_storage_load_devices: le_devs_num %d", le_devs_num);
    for (int i = 0; i < le_devs_num; i++)
    {
        T_LE_KEY_ENTRY *p_entry = le_find_key_entry_by_idx(devs_pri_table[i]);
        T_BT_DEVICE *p_dev_rec = NULL;
        if (p_entry)
        {
            APP_PRINT_INFO4("bt_storage_load_devices: idx %d, addr %s, remote_bd_type %d, flags 0x%02x",
                            i, TRACE_BDADDR(p_entry->remote_bd.addr), p_entry->remote_bd.remote_bd_type, p_entry->flags);
            APP_PRINT_INFO2("bt_storage_load_devices: resolved_addr %s, remote_bd_type %d",
                            TRACE_BDADDR(p_entry->resolved_remote_bd.addr), p_entry->resolved_remote_bd.remote_bd_type);

            if (p_entry->flags & LE_KEY_STORE_REMOTE_LTK_BIT)
            {
                p_dev_rec = ual_find_alloc_device_by_addr(p_entry->remote_bd.addr,
                                                          p_entry->remote_bd.remote_bd_type);
                if (p_dev_rec)
                {
                    ble_init_pseudo_addr(p_dev_rec, p_entry->remote_bd.addr, p_entry->remote_bd.remote_bd_type);
                    if (le_get_dev_ltk(p_entry, true, &p_dev_rec->ltk_len, p_dev_rec->ltk))
                    {
                        p_dev_rec->le_bonded = true;
                    }

                    if (p_entry->flags & LE_KEY_STORE_REMOTE_IRK_BIT)
                    {
                        if (le_get_dev_irk(p_entry, true, p_dev_rec->irk))
                        {
                            p_dev_rec->has_irk = true;
                            APP_PRINT_INFO1("bt_storage_load_devices: addr %s, found irk",
                                            TRACE_BDADDR(p_entry->remote_bd.addr));
                        }
                        memcpy(p_dev_rec->identity_addr, p_entry->resolved_remote_bd.addr, BD_ADDR_LEN);
                        p_dev_rec->identity_bd_type = p_entry->resolved_remote_bd.remote_bd_type | BLE_BD_TYPE_ID_BIT;
#if BLE_PRIVACY_SPT
                        ble_resolving_list_load_dev(p_dev_rec);
#endif
                    }
                }
            }
        }
    }
}
