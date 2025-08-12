#ifndef _APP_STORAGE_FLASH_
#define _APP_STORAGE_FLASH_

#include <stdint.h>

typedef struct
{
    uint8_t         remote_bd[6];
    uint8_t         is_valid;
    uint8_t         key_type;
    uint32_t        bond_flag;
} T_APP_SRC_LEGACY_REMOTE_BD;

typedef struct
{
    uint8_t key[16];
} T_APP_SRC_LEGACY_LINK_KEY;

uint32_t app_src_flash_save_legacy_remote_bd(T_APP_SRC_LEGACY_REMOTE_BD *p_data, uint8_t idx);

uint32_t app_src_flash_load_legacy_remote_bd(T_APP_SRC_LEGACY_REMOTE_BD *p_data, uint8_t idx);

uint32_t app_src_flash_save_legacy_link_key(T_APP_SRC_LEGACY_LINK_KEY *p_data, uint8_t idx);

uint32_t app_src_flash_load_legacy_link_key(T_APP_SRC_LEGACY_LINK_KEY *p_data, uint8_t idx);


#endif
