#include "app_src_storage_flash_le.h"
#include "ftl.h"
#include <string.h>
#include <trace.h>

#define APP_SRC_FLASH_LE_LINK_INFO_BD_OFFSET        4200
#define APP_SRC_FLASH_LE_LINK_INFO_LTK_OFFSET       (APP_SRC_FLASH_LE_LINK_INFO_BD_OFFSET + sizeof(T_APP_SRC_LE_REMOTE_BD))
#define APP_SRC_FLASH_LE_LINK_INFO_IRK_OFFSET       (APP_SRC_FLASH_LE_LINK_INFO_LTK_OFFSET + sizeof(T_APP_SRC_LE_REMOTE_LTK))
#define APP_SRC_FLASH_LE_LINK_INFO_SIZE             (sizeof(T_APP_SRC_LE_REMOTE_BD) + sizeof(T_APP_SRC_LE_REMOTE_LTK) + sizeof(T_APP_SRC_LE_REMOTE_IRK))

uint32_t app_src_flash_save_le_remote_bd(T_APP_SRC_LE_REMOTE_BD *p_data, uint8_t idx)
{
    uint16_t offset = APP_SRC_FLASH_LE_LINK_INFO_BD_OFFSET + idx *
                      APP_SRC_FLASH_LE_LINK_INFO_SIZE;
    uint8_t size;

    size = sizeof(T_APP_SRC_LE_REMOTE_BD);

    GAP_PRINT_TRACE4("app_src_flash_save_le_remote_bd: idx %d, addr %s, addr_type %d, bond_flag 0x%08x",
                     idx, TRACE_BDADDR(p_data->remote_bd), p_data->addr_type, p_data->bond_flag);

    return ftl_save_to_storage(p_data, offset, size);
}

uint32_t app_src_flash_load_le_remote_bd(T_APP_SRC_LE_REMOTE_BD *p_data, uint8_t idx)
{
    uint16_t offset;
    uint32_t has_error;
    uint8_t size;

    GAP_PRINT_TRACE1("app_src_flash_load_le_remote_bd: idx %d", idx);

    offset = APP_SRC_FLASH_LE_LINK_INFO_BD_OFFSET + idx * APP_SRC_FLASH_LE_LINK_INFO_SIZE;
    size = sizeof(T_APP_SRC_LE_REMOTE_BD);

    has_error =  ftl_load_from_storage(p_data, offset, size);

    if (has_error)
    {
        memset(p_data, 0, size);
    }

    return has_error;
}

uint32_t app_src_flash_save_le_ltk(T_APP_SRC_LE_REMOTE_LTK *p_data, uint8_t idx)
{
    uint16_t offset;
    uint8_t size;

    GAP_PRINT_TRACE1("app_src_flash_save_le_ltk: idx %d", idx);

    offset = APP_SRC_FLASH_LE_LINK_INFO_LTK_OFFSET + idx * APP_SRC_FLASH_LE_LINK_INFO_SIZE;
    size = sizeof(T_APP_SRC_LE_REMOTE_LTK);

    return ftl_save_to_storage(p_data, offset, size);
}

uint32_t app_src_flash_load_le_ltk(T_APP_SRC_LE_REMOTE_LTK *p_data, uint8_t idx)
{
    uint16_t offset;
    uint8_t size;
    uint32_t has_error;

    APP_PRINT_INFO1("app_src_flash_load_le_ltk: idx %d", idx);

    offset = APP_SRC_FLASH_LE_LINK_INFO_LTK_OFFSET + idx * APP_SRC_FLASH_LE_LINK_INFO_SIZE;
    size = sizeof(T_APP_SRC_LE_REMOTE_LTK);

    has_error = ftl_load_from_storage(p_data, offset, size);

    if (has_error)
    {
        APP_PRINT_ERROR1("app_src_flash_load_le_ltk: failed to load %d", has_error);
        memset(p_data, 0, size);
    }

    return has_error;
}

uint32_t app_src_flash_save_le_irk(T_APP_SRC_LE_REMOTE_IRK *p_data, uint8_t idx)
{
    uint16_t offset;
    uint8_t size;

    GAP_PRINT_TRACE1("app_src_flash_save_le_irk: idx %d", idx);

    offset = APP_SRC_FLASH_LE_LINK_INFO_IRK_OFFSET + idx * APP_SRC_FLASH_LE_LINK_INFO_SIZE;
    size = sizeof(T_APP_SRC_LE_REMOTE_IRK);

    return ftl_save_to_storage(p_data, offset, size);
}

uint32_t app_src_flash_load_le_irk(T_APP_SRC_LE_REMOTE_IRK *p_data, uint8_t idx)
{
    uint16_t offset;
    uint8_t size;
    uint32_t has_error;

    GAP_PRINT_TRACE1("app_src_flash_load_le_irk: idx %d", idx);

    offset = APP_SRC_FLASH_LE_LINK_INFO_IRK_OFFSET + idx * APP_SRC_FLASH_LE_LINK_INFO_SIZE;
    size = sizeof(T_APP_SRC_LE_REMOTE_IRK);

    has_error = ftl_load_from_storage(p_data, offset, size);

    if (has_error)
    {
        memset(p_data, 0, size);
    }

    return has_error;
}

