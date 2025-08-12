#include "app_src_bond_storage.h"
#include "app_src_storage_flash.h"
#include <trace.h>
#include <string.h>

static uint8_t  legacy_bond_num = 0;
static T_APP_SRC_LEGACY_REMOTE_BD legacy_bond_tbl[2];

void app_src_legacy_key_init(void)
{
    //uint8_t size;
    uint8_t i;

    legacy_bond_num = 2;
    //size = legacy_bond_num * sizeof(T_APP_SRC_LEGACY_REMOTE_BD);
    //legacy_bond_tbl = os_mem_zalloc(RAM_TYPE_DATA_ON, size);

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
