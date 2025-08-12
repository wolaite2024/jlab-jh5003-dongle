#ifndef _APP_SRC_STORAGE_FLASH_LE_H_
#define _APP_SRC_STORAGE_FLASH_LE_H_

#include <stdint.h>

typedef struct
{
    uint8_t     remote_bd[6];
    uint8_t     addr_type;
    uint8_t     is_valid;
    uint32_t    bond_flag;
} T_APP_SRC_LE_REMOTE_BD;

typedef struct
{
    uint8_t     key[28];
    uint8_t     link_key_length;
    uint8_t     padding[3];
} T_APP_SRC_LE_REMOTE_LTK;

typedef struct
{
    uint8_t     key[16];
    uint8_t     addr[6];
    uint8_t     addr_type;
    uint8_t     key_exist;
} T_APP_SRC_LE_REMOTE_IRK;

uint32_t app_src_flash_save_le_remote_bd(T_APP_SRC_LE_REMOTE_BD *p_data, uint8_t idx);

uint32_t app_src_flash_load_le_remote_bd(T_APP_SRC_LE_REMOTE_BD *p_data, uint8_t idx);

uint32_t app_src_flash_save_le_ltk(T_APP_SRC_LE_REMOTE_LTK *p_data, uint8_t idx);

uint32_t app_src_flash_load_le_ltk(T_APP_SRC_LE_REMOTE_LTK *p_data, uint8_t idx);

uint32_t app_src_flash_save_le_irk(T_APP_SRC_LE_REMOTE_IRK *p_data, uint8_t idx);

uint32_t app_src_flash_load_le_irk(T_APP_SRC_LE_REMOTE_IRK *p_data, uint8_t idx);

#endif

