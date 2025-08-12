/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#include "ftl.h"
#include <string.h>
#include <stdint.h>
#include <trace.h>
#include <os_mem.h>
#include "app_bond.h"

#define APP_SRC_FLASH_LINK_INFO_START_OFFSET          4100
#define APP_SRC_FLASH_LEGACY_LINK_INFO_BD_OFFSET      4100
#define APP_SRC_FLASH_LEGACY_LINK_INFO_LINKKEY_OFFSET (APP_SRC_FLASH_LEGACY_LINK_INFO_BD_OFFSET + sizeof(T_APP_SRC_LEGACY_REMOTE_BD))
#define APP_SRC_FLASH_LEGACY_LINK_INFO_SIZE           (sizeof(T_APP_SRC_LEGACY_REMOTE_BD)+ sizeof(T_APP_SRC_LEGACY_LINK_KEY))

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
#define APP_SRC_FLASH_LEGACY_LOCK_INFO_OFFSET         (APP_SRC_FLASH_LEGACY_LINK_INFO_BD_OFFSET + 2 * APP_SRC_FLASH_LEGACY_LINK_INFO_SIZE)
#define APP_SRC_FLASH_LEGACY_LOCK_INFO_SIZE           (sizeof(T_APP_SRC_LEGACY_LOCK_INFO))
#endif

typedef struct
{
    uint8_t         remote_bd[6];
    uint8_t         is_valid;
    uint8_t         key_type;
    uint32_t        bond_flag;
    uint8_t         hs_info[4];
} T_APP_SRC_LEGACY_REMOTE_BD;

typedef struct
{
    uint8_t key[16];
} T_APP_SRC_LEGACY_LINK_KEY;

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
typedef struct
{
    uint8_t locked_remote_bd[6];
    uint16_t lock_headset_flag;
} T_APP_SRC_LEGACY_LOCK_INFO;
#endif

static uint8_t  legacy_bond_num = 0;
static T_APP_SRC_LEGACY_REMOTE_BD *legacy_bond_tbl = NULL;

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
static T_APP_SRC_LEGACY_LOCK_INFO *legacy_lock_info = NULL;
#endif

uint32_t app_src_flash_save_legacy_remote_bd(T_APP_SRC_LEGACY_REMOTE_BD *p_data, uint8_t idx)
{
    uint16_t offset = APP_SRC_FLASH_LEGACY_LINK_INFO_BD_OFFSET + idx *
                      APP_SRC_FLASH_LEGACY_LINK_INFO_SIZE;
    uint8_t size;

    offset = APP_SRC_FLASH_LEGACY_LINK_INFO_BD_OFFSET + idx * APP_SRC_FLASH_LEGACY_LINK_INFO_SIZE;
    size = sizeof(T_APP_SRC_LEGACY_REMOTE_BD);

    GAP_PRINT_TRACE4("app_src_flash_save_legacy_remote_bd: idx %d, addr %s, bond_flag 0x%08x, key_type 0x%02x",
                     idx, TRACE_BDADDR(p_data->remote_bd), p_data->bond_flag, p_data->key_type);

    return ftl_save_to_storage(p_data, offset, size);
}

uint32_t app_src_flash_load_legacy_remote_bd(T_APP_SRC_LEGACY_REMOTE_BD *p_data, uint8_t idx)
{
    uint16_t offset;
    uint32_t has_error;
    uint8_t size;

    GAP_PRINT_TRACE1("app_src_flash_load_legacy_remote_bd: idx %d", idx);

    offset = APP_SRC_FLASH_LEGACY_LINK_INFO_BD_OFFSET + idx * APP_SRC_FLASH_LEGACY_LINK_INFO_SIZE;
    size = sizeof(T_APP_SRC_LEGACY_REMOTE_BD);

    has_error =  ftl_load_from_storage(p_data, offset, size);

    if (has_error)
    {
        memset(p_data, 0, size);
    }

    return has_error;
}

uint32_t app_src_flash_save_legacy_link_key(T_APP_SRC_LEGACY_LINK_KEY *p_data, uint8_t idx)
{
    uint16_t offset;
    uint8_t size;

    GAP_PRINT_TRACE1("app_src_flash_save_legacy_link_key: idx %d", idx);

    offset = APP_SRC_FLASH_LEGACY_LINK_INFO_LINKKEY_OFFSET + idx * APP_SRC_FLASH_LEGACY_LINK_INFO_SIZE;
    size = sizeof(T_APP_SRC_LEGACY_LINK_KEY);

    return ftl_save_to_storage(p_data, offset, size);
}

uint32_t app_src_flash_load_legacy_link_key(T_APP_SRC_LEGACY_LINK_KEY *p_data, uint8_t idx)
{
    uint16_t offset;
    uint8_t size;
    uint32_t has_error;

    GAP_PRINT_TRACE1("app_src_flash_load_legacy_link_key: idx %d", idx);

    offset = APP_SRC_FLASH_LEGACY_LINK_INFO_LINKKEY_OFFSET + idx * APP_SRC_FLASH_LEGACY_LINK_INFO_SIZE;
    size = sizeof(T_APP_SRC_LEGACY_LINK_KEY);

    has_error = ftl_load_from_storage(p_data, offset, size);

    if (has_error)
    {
        memset(p_data, 0, size);
    }

    return has_error;
}

void app_src_legacy_key_init(void)
{
    uint8_t size;
    uint8_t i;

    legacy_bond_num = 2;
    size = legacy_bond_num * sizeof(T_APP_SRC_LEGACY_REMOTE_BD);
    legacy_bond_tbl = os_mem_zalloc(RAM_TYPE_DATA_ON, size);

    for (i = 0; i < legacy_bond_num; i++)
    {
        app_src_flash_load_legacy_remote_bd(&legacy_bond_tbl[i], i);
        GAP_PRINT_INFO5("app_src_legacy_key_init: idx %d, addr %s, valid %d, bond_flag 0x%02x, key_type 0x%02x",
                        i, TRACE_BDADDR(legacy_bond_tbl[i].remote_bd),
                        legacy_bond_tbl[i].is_valid,
                        legacy_bond_tbl[i].bond_flag,
                        legacy_bond_tbl[i].key_type);
    }
}

void app_src_legacy_clear_all_keys(void)
{
    uint8_t   i;
    memset(legacy_bond_tbl, 0x00,
           legacy_bond_num * sizeof(T_APP_SRC_LEGACY_REMOTE_BD));

    for (i = 0; i < legacy_bond_num; i++)
    {
        app_src_flash_save_legacy_remote_bd(&legacy_bond_tbl[i], i);
    }
}

bool app_src_legacy_get_paired_idx(uint8_t *bd_addr, uint8_t *p_idx)
{
    uint8_t i;

    for (i = 0; i < legacy_bond_num; i++)
    {
        if ((legacy_bond_tbl[i].bond_flag & APP_SRC_BOND_FLAG_PAIRED) &&
            (memcmp(legacy_bond_tbl[i].remote_bd, bd_addr, 6) == 0) &&
            (legacy_bond_tbl[i].is_valid != 0))
        {
            break;
        }
    }

    if (i == legacy_bond_num)
    {
        return false;
    }
    else
    {
        *p_idx = i;
        return true;
    }
}

bool app_src_legacy_set_bond_flag_by_index(uint8_t index, uint8_t *bd_addr, uint32_t value)
{
    if (index >= legacy_bond_num)
    {
        return false;
    }

    legacy_bond_tbl[index].bond_flag = value;
    app_src_flash_save_legacy_remote_bd(&legacy_bond_tbl[index], index);

    return true;
}

bool app_src_legacy_set_bond_flag_by_addr(uint8_t *bd_addr, uint32_t value)
{
    uint8_t index;

    if (app_src_legacy_get_paired_idx(bd_addr, &index) == false)
    {
        return false;
    }

    return app_src_legacy_set_bond_flag_by_index(index, bd_addr, value);
}

uint32_t app_src_legacy_get_bond_flag_by_index(uint8_t index)
{
    if (index >= legacy_bond_num)
    {
        return 0;
    }

    if (legacy_bond_tbl[index].is_valid == 0)
    {
        return 0;
    }

    return legacy_bond_tbl[index].bond_flag;
}

uint32_t app_src_legacy_get_bond_flag_by_addr(uint8_t *bd_addr)
{
    uint8_t index;

    if (app_src_legacy_get_paired_idx(bd_addr, &index) == false)
    {
        return 0;
    }

    return app_src_legacy_get_bond_flag_by_index(index);
}

bool app_src_legacy_get_bond_addr_by_index(uint8_t index, uint8_t *bd_addr)
{
    if (index >= legacy_bond_num)
    {
        return false;
    }

    if (legacy_bond_tbl[index].is_valid != 0)
    {
        if (bd_addr != 0)
        {
            memcpy(bd_addr, legacy_bond_tbl[index].remote_bd, 6);
            return true;
        }
    }

    return false;
}

bool app_src_legacy_save_bond(uint8_t index, uint8_t *bd_addr, uint8_t *linkkey, uint8_t key_type)
{
    T_APP_SRC_LEGACY_LINK_KEY link_key;

    memset(&legacy_bond_tbl[index], 0x00, sizeof(T_APP_SRC_LEGACY_REMOTE_BD));
    memcpy(legacy_bond_tbl[index].remote_bd, bd_addr, 6);
    memcpy(link_key.key, linkkey, 16);
    legacy_bond_tbl[index].bond_flag = APP_SRC_BOND_FLAG_PAIRED;
    legacy_bond_tbl[index].key_type = key_type;
    legacy_bond_tbl[index].is_valid = 1;

    if (app_src_flash_save_legacy_remote_bd(&legacy_bond_tbl[index], index) != 0)
    {
        return false;
    }

    if (app_src_flash_save_legacy_link_key(&link_key, index) != 0)
    {
        return false;
    }

    return true;
}

bool app_src_legacy_add_bond_flag(uint8_t *bd_addr, uint32_t bond_mask)
{
    uint32_t bond_flag;

    bond_flag = app_src_legacy_get_bond_flag_by_addr(bd_addr);

    if ((bond_flag & APP_SRC_BOND_FLAG_PAIRED) &&
        ((bond_flag & bond_mask) != bond_mask))
    {
        bond_flag |= bond_mask;
    }

    return app_src_legacy_set_bond_flag_by_addr(bd_addr, bond_flag);
}

bool app_src_legacy_add_hs_info(uint8_t index, uint8_t *bd_addr, uint8_t *info)
{
    uint32_t bond_flag;
    uint32_t bond_mask = APP_SRC_BOND_FLAG_HS_INFO;

    if (index >= legacy_bond_num)
    {
        return false;
    }

    if (!legacy_bond_tbl[index].is_valid)
    {
        return false;
    }

    if (memcmp(bd_addr, legacy_bond_tbl[index].remote_bd, 6))
    {
        return false;
    }

    bond_flag = legacy_bond_tbl[index].bond_flag;

    if (bond_flag & APP_SRC_BOND_FLAG_PAIRED)
    {
        legacy_bond_tbl[index].bond_flag = bond_flag | bond_mask;
        memcpy(legacy_bond_tbl[index].hs_info, info, 4);
        app_src_flash_save_legacy_remote_bd(&legacy_bond_tbl[index], index);
        return true;
    }
    else
    {
        return false;
    }
}

bool app_src_legacy_get_hs_info(uint8_t *bd_addr, uint8_t *info)
{
    uint8_t i;

    for (i = 0; i < legacy_bond_num; i++)
    {
        if (!memcmp(bd_addr, legacy_bond_tbl[i].remote_bd, 6))
        {
            if (!legacy_bond_tbl[i].is_valid)
            {
                return false;
            }
            if (info)
            {
                memcpy(info, legacy_bond_tbl[i].hs_info, 4);
            }
            return true;
        }
    }
    return false;
}

bool app_src_legacy_remove_bond_flag(uint8_t *bd_addr, uint32_t bond_mask)
{
    uint32_t bond_flag;

    bond_flag = app_src_legacy_get_bond_flag_by_addr(bd_addr);

    if (bond_flag & bond_mask)
    {
        bond_flag &= ~bond_mask;
    }

    return app_src_legacy_set_bond_flag_by_addr(bd_addr, bond_flag);
}


bool app_src_legacy_get_bond_by_index(uint8_t index, uint8_t *link_key, uint8_t *key_type)
{
    if (index >= legacy_bond_num)
    {
        return false;
    }

    if (legacy_bond_tbl[index].is_valid == 0)
    {
        return false;
    }

    if (app_src_flash_load_legacy_link_key((T_APP_SRC_LEGACY_LINK_KEY *)link_key, index) == 0)
    {
        *key_type = legacy_bond_tbl[index].key_type;
        return true;
    }
    else
    {
        return false;
    }
}

bool app_src_legacy_get_bond_by_addr(uint8_t *bd_addr, uint8_t *link_key, uint8_t *key_type)
{
    uint8_t index;

    if (app_src_legacy_get_paired_idx(bd_addr, &index) == false)
    {
        return false;
    }

    return app_src_legacy_get_bond_by_index(index, link_key, key_type);
}

bool app_src_legacy_delete_bond_by_index(uint8_t index)
{
    //TBD: check index

    if (index < legacy_bond_num)
    {
        memset(&legacy_bond_tbl[index], 0x00, sizeof(T_APP_SRC_LEGACY_REMOTE_BD));
        app_src_flash_save_legacy_remote_bd(&legacy_bond_tbl[index], index);

        return true;
    }

    return false;
}

bool app_src_legacy_delete_bond_by_addr(uint8_t *bd_addr)
{
    uint8_t index;

    if (app_src_legacy_get_paired_idx(bd_addr, &index) == false)
    {
        return false;
    }

    app_src_legacy_delete_bond_by_index(index);

    return true;
}

uint8_t app_src_legacy_get_max_bond_num(void)
{
    return legacy_bond_num;
}

#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
static uint32_t app_src_flash_save_legacy_lock_info(T_APP_SRC_LEGACY_LOCK_INFO *p_data, uint8_t idx)
{
    uint16_t offset;
    uint8_t size;
    offset = APP_SRC_FLASH_LEGACY_LOCK_INFO_OFFSET + idx * APP_SRC_FLASH_LEGACY_LOCK_INFO_SIZE;
    size = APP_SRC_FLASH_LEGACY_LOCK_INFO_SIZE;
    GAP_PRINT_TRACE3("app_src_flash_save_legacy_lock_info: idx %d, addr %b, lock_flag %d",
                     idx, TRACE_BDADDR(p_data->locked_remote_bd), p_data->lock_headset_flag);
    return ftl_save_to_storage(p_data, offset, size);
}

static uint32_t app_src_flash_load_legacy_lock_info(T_APP_SRC_LEGACY_LOCK_INFO *p_data, uint8_t idx)
{
    uint16_t offset;
    uint8_t size;
    uint32_t has_error;
    GAP_PRINT_TRACE2("app_src_flash_load_legacy_lock_info: addr %b, lock_flag %d",
                     p_data->locked_remote_bd, p_data->lock_headset_flag);
    offset = APP_SRC_FLASH_LEGACY_LOCK_INFO_OFFSET + idx * APP_SRC_FLASH_LEGACY_LOCK_INFO_SIZE;
    size = APP_SRC_FLASH_LEGACY_LOCK_INFO_SIZE;
    has_error = ftl_load_from_storage(p_data, offset, size);
    if (has_error)
    {
        memset(p_data, 0, size);
    }
    return has_error;
}

bool app_src_legacy_save_lock_info(uint8_t index, uint8_t *bd_addr, uint8_t lock_flag)
{
    memset(&legacy_lock_info[index], 0x00, sizeof(T_APP_SRC_LEGACY_LOCK_INFO));
    memcpy(legacy_lock_info[index].locked_remote_bd, bd_addr, 6);
    legacy_lock_info[index].lock_headset_flag = lock_flag;
    if (app_src_flash_save_legacy_lock_info(&legacy_lock_info[index], index) != 0)
    {
        return false;
    }
    return true;
}

void app_src_legacy_lock_info_init(void)
{
    uint8_t size;
    uint8_t i;
    legacy_bond_num = 2;
    size = legacy_bond_num * sizeof(T_APP_SRC_LEGACY_LOCK_INFO);
    legacy_lock_info = os_mem_zalloc(RAM_TYPE_DATA_ON, size);
    for (i = 0; i < legacy_bond_num; i++)
    {
        app_src_flash_load_legacy_lock_info(&legacy_lock_info[i], i);
        GAP_PRINT_INFO3("app_src_legacy_lock_info_init: idx %d, addr %s, bond_flag %d",
                        i, TRACE_BDADDR(legacy_lock_info[i].locked_remote_bd),
                        legacy_lock_info[i].lock_headset_flag);
    }
}

uint8_t app_src_legacy_get_lock_flag_by_index(uint8_t index)
{
    if (index >= legacy_bond_num)
    {
        return 0;
    }
    return legacy_lock_info[index].lock_headset_flag;
}

bool app_src_legacy_get_lock_addr_by_index(uint8_t index, uint8_t *bd_addr)
{
    if (index >= legacy_bond_num)
    {
        return false;
    }
    memcpy(bd_addr, legacy_lock_info[index].locked_remote_bd, 6);
    return true;
}

void app_src_legacy_clear_lock_info(void)
{
    uint8_t   i;
    memset(legacy_lock_info, 0x00,
           legacy_bond_num * sizeof(T_APP_SRC_LEGACY_LOCK_INFO));
    for (i = 0; i < legacy_bond_num; i++)
    {
        app_src_flash_save_legacy_lock_info(&legacy_lock_info[i], i);
    }
}
#endif
