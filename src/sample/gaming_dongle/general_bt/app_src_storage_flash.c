#include "app_src_storage_flash.h"
#include "ftl.h"
#include <string.h>
#include <stdint.h>
#include <trace.h>

#define APP_SRC_FLASH_LINK_INFO_START_OFFSET          4100
#define APP_SRC_FLASH_LEGACY_LINK_INFO_BD_OFFSET      4100
#define APP_SRC_FLASH_LEGACY_LINK_INFO_LINKKEY_OFFSET (APP_SRC_FLASH_LEGACY_LINK_INFO_BD_OFFSET + sizeof(T_APP_SRC_LEGACY_REMOTE_BD))
#define APP_SRC_FLASH_LEGACY_LINK_INFO_SIZE           (sizeof(T_APP_SRC_LEGACY_REMOTE_BD)+ sizeof(T_APP_SRC_LEGACY_LINK_KEY))

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
