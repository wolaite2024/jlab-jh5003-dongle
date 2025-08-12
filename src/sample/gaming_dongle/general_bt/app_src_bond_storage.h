#ifndef _APP_SRC_BOND_STORAGE_
#define _APP_SRC_BOND_STORAGE_

#include <stdint.h>
#include <stdbool.h>

#define APP_SRC_BOND_FLAG_HFP                   0x01    /**< GAP legacy bond flag HFP support. */
#define APP_SRC_BOND_FLAG_A2DP                  0x02    /**< GAP legacy bond flag A2DP support. */
#define APP_SRC_BOND_FLAG_SPP                   0x04    /**< GAP legacy bond flag SPP support. */
#define APP_SRC_BOND_FLAG_PBAP                  0x08    /**< GAP legacy bond flag PBAP support. */
#define APP_SRC_BOND_FLAG_IAP                   0x10    /**< GAP legacy bond flag IAP support. */
#define APP_SRC_BOND_FLAG_HSP                   0x20    /**< GAP legacy bond flag HSP support. */
#define APP_SRC_BOND_FLAG_RDTP                  0x40    /**< GAP legacy bond flag RDTP support. */
#define APP_SRC_BOND_FLAG_PAIRED                0x80    /**< GAP legacy bond flag device already paired. */

void app_src_legacy_key_init(void);

void app_src_legacy_clear_all_keys(void);

bool app_src_legacy_get_paired_idx(uint8_t *bd_addr, uint8_t *p_idx);

bool app_src_legacy_set_bond_flag_by_index(uint8_t index, uint8_t *bd_addr, uint32_t value);

bool app_src_legacy_set_bond_flag_by_addr(uint8_t *bd_addr, uint32_t value);

uint32_t app_src_legacy_get_bond_flag_by_index(uint8_t index);

uint32_t app_src_legacy_get_bond_flag_by_addr(uint8_t *bd_addr);

bool app_src_legacy_get_bond_addr_by_index(uint8_t index, uint8_t *bd_addr);

bool app_src_legacy_save_bond(uint8_t index, uint8_t *bd_addr, uint8_t *linkkey, uint8_t key_type);

bool app_src_legacy_add_bond_flag(uint8_t *bd_addr, uint32_t bond_mask);

bool app_src_legacy_remove_bond_flag(uint8_t *bd_addr, uint32_t bond_mask);

bool app_src_legacy_get_bond_by_index(uint8_t index, uint8_t *link_key, uint8_t *key_type);

bool app_src_legacy_get_bond_by_addr(uint8_t *bd_addr, uint8_t *link_key, uint8_t *key_type);

bool app_src_legacy_delete_bond_by_index(uint8_t index);

bool app_src_legacy_delete_bond_by_addr(uint8_t *bd_addr);

uint8_t app_src_legacy_get_max_bond_num(void);

#endif
