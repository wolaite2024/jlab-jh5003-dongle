#ifndef _APP_SRC_BOND_STORAGE_LE_H_
#define _APP_SRC_BOND_STORAGE_LE_H_

#include <stdint.h>
#include <stdbool.h>

#define APP_SRC_BOND_FLAG_LE_GATT               0x01    /**< GAP le bond flag GATT support. */
#define APP_SRC_BOND_FLAG_LE_HOGP               0x02    /**< GAP le bond flag HOGP support. */
#define APP_SRC_BOND_FLAG_LE_PAIRED             0x80    /**< GAP le bond flag le device already paired. */

void app_src_le_key_init(void);

void app_src_le_clear_all_keys(void);

bool app_src_le_get_paired_idx(uint8_t *bd_addr, uint8_t addr_type, uint8_t *p_idx);

bool app_src_le_set_bond_flag_by_index(uint8_t index, uint32_t value);

bool app_src_le_set_bond_flag_by_addr(uint8_t *bd_addr, uint8_t addr_type, uint32_t value);

uint32_t app_src_le_get_bond_flag_by_index(uint8_t index);

uint32_t app_src_le_get_bond_flag_by_addr(uint8_t *bd_addr, uint8_t addr_type);

bool app_src_le_get_bond_addr_by_index(uint8_t index, uint8_t *bd_addr, uint8_t *addr_type);

bool app_src_le_save_ltk(uint8_t index, uint8_t *bd_addr, uint8_t addr_type, uint8_t *ltk,
                         uint8_t link_key_length);

bool app_src_le_save_irk(uint8_t *bd_addr, uint8_t addr_type, uint8_t *irk);

bool app_src_le_add_bond_flag(uint8_t *bd_addr, uint8_t addr_type, uint32_t bond_mask);

bool app_src_le_remove_bond_flag(uint8_t *bd_addr, uint8_t addr_type, uint32_t bond_mask);

bool app_src_le_get_ltk_by_index(uint8_t index, uint8_t *ltk, uint8_t *key_len);

bool app_src_le_get_ltk_by_addr(uint8_t *bd_addr, uint8_t addr_type, uint8_t *ltk,
                                uint8_t *key_len);

bool app_src_le_get_irk_by_index(uint8_t index, uint8_t *irk);

bool app_src_le_get_irk_by_addr(uint8_t *bd_addr, uint8_t addr_type, uint8_t *irk);

bool app_src_le_delete_bond_by_index(uint8_t index);

bool app_src_le_delete_bond_by_addr(uint8_t *bd_addr, uint8_t addr_type);

uint8_t app_src_le_get_max_bond_num(void);

#endif

