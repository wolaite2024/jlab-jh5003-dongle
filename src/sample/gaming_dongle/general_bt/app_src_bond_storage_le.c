#include "app_src_bond_storage_le.h"
#include "app_src_storage_flash_le.h"
#include <trace.h>
#include <string.h>

static uint8_t  le_bond_num = 0;
static T_APP_SRC_LE_REMOTE_BD le_bond_tbl[4];

void app_src_le_key_init(void)
{
    //uint8_t size;
    uint8_t i;

    le_bond_num = 4;
    //size = le_bond_num * sizeof(T_APP_SRC_LE_REMOTE_BD);
    //le_bond_tbl = os_mem_zalloc(RAM_TYPE_DATA_ON, size);

    for (i = 0; i < le_bond_num; i++)
    {
        app_src_flash_load_le_remote_bd(&le_bond_tbl[i], i);
        GAP_PRINT_INFO5("app_src_le_key_init: idx %d, addr %s, addr_type %d, valid %d, bond_flag 0x%02x",
                        i, TRACE_BDADDR(le_bond_tbl[i].remote_bd),
                        le_bond_tbl[i].addr_type,
                        le_bond_tbl[i].is_valid,
                        le_bond_tbl[i].bond_flag);
    }
}

void app_src_le_clear_all_keys(void)
{
    uint8_t   i;
    memset(le_bond_tbl, 0x00,
           le_bond_num * sizeof(T_APP_SRC_LE_REMOTE_BD));

    for (i = 0; i < le_bond_num; i++)
    {
        app_src_flash_save_le_remote_bd(&le_bond_tbl[i], i);
    }
}

bool app_src_le_get_paired_idx(uint8_t *bd_addr, uint8_t addr_type, uint8_t *p_idx)
{
    uint8_t i;

    for (i = 0; i < le_bond_num; i++)
    {
        if ((le_bond_tbl[i].bond_flag & APP_SRC_BOND_FLAG_LE_PAIRED) &&
            (memcmp(le_bond_tbl[i].remote_bd, bd_addr, 6) == 0) &&
            (le_bond_tbl[i].addr_type == addr_type) &&
            (le_bond_tbl[i].is_valid != 0))
        {
            break;
        }
    }

    if (i == le_bond_num)
    {
        return false;
    }
    else
    {
        *p_idx = i;
        return true;
    }
}

bool app_src_le_set_bond_flag_by_index(uint8_t index, uint32_t value)
{
    if (index >= le_bond_num)
    {
        return false;
    }

    le_bond_tbl[index].bond_flag = value;
    app_src_flash_save_le_remote_bd(&le_bond_tbl[index], index);

    return true;
}

bool app_src_le_set_bond_flag_by_addr(uint8_t *bd_addr, uint8_t addr_type, uint32_t value)
{
    uint8_t index;

    if (app_src_le_get_paired_idx(bd_addr, addr_type, &index) == false)
    {
        return false;
    }

    return app_src_le_set_bond_flag_by_index(index, value);
}

uint32_t app_src_le_get_bond_flag_by_index(uint8_t index)
{
    if (index >= le_bond_num)
    {
        return 0;
    }

    if (le_bond_tbl[index].is_valid == 0)
    {
        return 0;
    }

    return le_bond_tbl[index].bond_flag;
}

uint32_t app_src_le_get_bond_flag_by_addr(uint8_t *bd_addr, uint8_t addr_type)
{
    uint8_t index;

    if (app_src_le_get_paired_idx(bd_addr, addr_type, &index) == false)
    {
        return 0;
    }

    return app_src_le_get_bond_flag_by_index(index);
}

bool app_src_le_get_bond_addr_by_index(uint8_t index, uint8_t *bd_addr, uint8_t *addr_type)
{
    if (index >= le_bond_num)
    {
        return false;
    }

    if (le_bond_tbl[index].is_valid != 0)
    {
        if (bd_addr != 0)
        {
            memcpy(bd_addr, le_bond_tbl[index].remote_bd, 6);
            *addr_type = le_bond_tbl[index].addr_type;
            return true;
        }
    }

    return false;
}

bool app_src_le_save_ltk(uint8_t index, uint8_t *bd_addr, uint8_t addr_type, uint8_t *ltk,
                         uint8_t link_key_length)
{
    T_APP_SRC_LE_REMOTE_LTK temp_ltk;

    memset(&le_bond_tbl[index], 0x00, sizeof(T_APP_SRC_LE_REMOTE_BD));

    memcpy(le_bond_tbl[index].remote_bd, bd_addr, 6);
    le_bond_tbl[index].addr_type = addr_type;
    le_bond_tbl[index].bond_flag = APP_SRC_BOND_FLAG_LE_PAIRED;
    le_bond_tbl[index].is_valid = 1;

    if (app_src_flash_save_le_remote_bd(&le_bond_tbl[index], index) != 0)
    {
        return false;
    }

    memset(&temp_ltk, 0, sizeof(T_APP_SRC_LE_REMOTE_LTK));
    temp_ltk.link_key_length = link_key_length;
    memcpy(&temp_ltk.key, ltk, link_key_length);

    if (app_src_flash_save_le_ltk((T_APP_SRC_LE_REMOTE_LTK *)&temp_ltk, index) != 0)
    {
        return false;
    }

    le_bond_tbl[index].is_valid = 1;

    return true;
}

bool app_src_le_save_irk(uint8_t *bd_addr, uint8_t addr_type, uint8_t *irk)
{
    uint8_t index;

    if (app_src_le_get_paired_idx(bd_addr, addr_type, &index) == false)
    {
        return 0;
    }


    if (!app_src_flash_save_le_irk((T_APP_SRC_LE_REMOTE_IRK *)irk, index))
    {
        return true;
    }

    return false;
}

bool app_src_le_add_bond_flag(uint8_t *bd_addr, uint8_t addr_type, uint32_t bond_mask)
{
    uint32_t bond_flag;

    bond_flag = app_src_le_get_bond_flag_by_addr(bd_addr, addr_type);

    if ((bond_flag & APP_SRC_BOND_FLAG_LE_PAIRED) &&
        ((bond_flag & bond_mask) != bond_mask))
    {
        bond_flag |= bond_mask;
    }

    return app_src_le_set_bond_flag_by_addr(bd_addr, addr_type, bond_flag);
}

bool app_src_le_remove_bond_flag(uint8_t *bd_addr, uint8_t addr_type, uint32_t bond_mask)
{
    uint32_t bond_flag;

    bond_flag = app_src_le_get_bond_flag_by_addr(bd_addr, addr_type);

    if (bond_flag & bond_mask)
    {
        bond_flag &= ~bond_mask;
    }

    return app_src_le_set_bond_flag_by_addr(bd_addr, addr_type, bond_flag);
}

bool app_src_le_get_ltk_by_index(uint8_t index, uint8_t *ltk, uint8_t *key_len)
{
    T_APP_SRC_LE_REMOTE_LTK temp_remote_ltk;

    if (index >= le_bond_num)
    {
        return false;
    }

    if (le_bond_tbl[index].is_valid == 0)
    {
        return false;
    }

    memset(&temp_remote_ltk, 0, sizeof(T_APP_SRC_LE_REMOTE_LTK));

    if (app_src_flash_load_le_ltk(&temp_remote_ltk, index) == 0)
    {
        *key_len = temp_remote_ltk.link_key_length;
        memcpy(ltk, temp_remote_ltk.key, temp_remote_ltk.link_key_length);
        return true;
    }
    else
    {
        return false;
    }
}

bool app_src_le_get_ltk_by_addr(uint8_t *bd_addr, uint8_t addr_type, uint8_t *ltk, uint8_t *key_len)
{
    uint8_t index;

    if (app_src_le_get_paired_idx(bd_addr, addr_type, &index) == false)
    {
        return false;
    }

    return app_src_le_get_ltk_by_index(index, ltk, key_len);
}

bool app_src_le_get_irk_by_index(uint8_t index, uint8_t *irk)
{
    if (index >= le_bond_num)
    {
        return false;
    }

    if (le_bond_tbl[index].is_valid == 0)
    {
        return false;
    }

    if (app_src_flash_load_le_irk((T_APP_SRC_LE_REMOTE_IRK *)irk, index) == 0)
    {
        return true;
    }

    return false;
}

bool app_src_le_get_irk_by_addr(uint8_t *bd_addr, uint8_t addr_type, uint8_t *irk)
{
    uint8_t index;

    if (app_src_le_get_paired_idx(bd_addr, addr_type, &index) == false)
    {
        return false;
    }

    return app_src_le_get_irk_by_index(index, irk);
}

bool app_src_le_delete_bond_by_index(uint8_t index)
{
    if (index < le_bond_num)
    {
        memset(&le_bond_tbl[index], 0x00, sizeof(T_APP_SRC_LE_REMOTE_BD));
        app_src_flash_save_le_remote_bd(&le_bond_tbl[index], index);

        return true;
    }

    return false;
}

bool app_src_le_delete_bond_by_addr(uint8_t *bd_addr, uint8_t addr_type)
{
    uint8_t index;

    if (app_src_le_get_paired_idx(bd_addr, addr_type, &index) == false)
    {
        return false;
    }

    app_src_le_delete_bond_by_index(index);

    return true;
}

uint8_t app_src_le_get_max_bond_num(void)
{
    return le_bond_num;
}

