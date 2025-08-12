/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#ifndef _APP_BOND_H_
#define _APP_BOND_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BOND App Bond
  * @brief Storage of bond information.
  * @{
  */

#define APP_SRC_BOND_FLAG_HFP                   0x01    /**< GAP legacy bond flag HFP support. */
#define APP_SRC_BOND_FLAG_A2DP                  0x02    /**< GAP legacy bond flag A2DP support. */
#define APP_SRC_BOND_FLAG_SPP                   0x04    /**< GAP legacy bond flag SPP support. */
#define APP_SRC_BOND_FLAG_PBAP                  0x08    /**< GAP legacy bond flag PBAP support. */
#define APP_SRC_BOND_FLAG_IAP                   0x10    /**< GAP legacy bond flag IAP support. */
#define APP_SRC_BOND_FLAG_HSP                   0x20    /**< GAP legacy bond flag HSP support. */
#define APP_SRC_BOND_FLAG_RDTP                  0x40    /**< GAP legacy bond flag RDTP support. */
#define APP_SRC_BOND_FLAG_PAIRED                0x80    /**< GAP legacy bond flag device already paired. */
#define APP_SRC_BOND_FLAG_HS_INFO               0x100   /* Headset info */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_BOND_Functions APP Bond Functions
  * @{
  */

/**
* app_bond.h
*
* \brief   Link Key init. Load link key and remote bond information from storage.
*/
void app_src_legacy_key_init(void);

/**
* app_bond.h
*
* \brief   Clear all the bound devices and link key information.
*/
void app_src_legacy_clear_all_keys(void);

/**
* app_bond.h
*
* \brief   Find the link index of bound device according to the address.
*
* \param[in] *bd_addr    Remote BT device address.
* \param[in] *p_idx      Local BT link index(can be 0x00: link1, or 0x01: link2).
*
* \return The status of getting paired index.
* \retval true     Get paired index succeed.
* \retval false    Get paired index failed.
*/
bool app_src_legacy_get_paired_idx(uint8_t *bd_addr, uint8_t *p_idx);

/**
* app_bond.h
*
* \brief   Set and save bond flag according to the link index.
*
* \param[in] index       Local BT link index(can be 0x00: link1, or 0x01: link2).
* \param[in] *bd_addr    Remote BT device address.
* \param[in] value       Bond flag.
*
* \return The status of seting bond flag according to the link index.
* \retval true     Set bond flag by index succeed.
* \retval false    Set bond flag by index failed.
*/
bool app_src_legacy_set_bond_flag_by_index(uint8_t index, uint8_t *bd_addr, uint32_t value);

/**
* app_bond.h
*
* \brief   Set and save bond flag according to the remote bt address.
*
* \param[in] *bd_addr    Remote BT device address.
* \param[in] value       Bond flag.
*
* \return The status of seting bond flag according to the remote bt address.
* \retval true     Set bond flag by address succeed.
* \retval false    Set bond flag by address failed.
*/
bool app_src_legacy_set_bond_flag_by_addr(uint8_t *bd_addr, uint32_t value);

/**
* app_bond.h
*
* \brief   Get bond flag according to the link index.
*
* \param[in] index       Local BT link index(can be 0x00: link1, or 0x01: link2).
*
* \return The value of bond flag.
* \retval bond_flag
*/
uint32_t app_src_legacy_get_bond_flag_by_index(uint8_t index);

/**
* app_bond.h
*
* \brief   Get bond flag according to the remote bt address.
*
* \param[in] bd_addr     Remote BT device address.
*
* \return The value of bond flag.
* \retval bond_flag
*/
uint32_t app_src_legacy_get_bond_flag_by_addr(uint8_t *bd_addr);

/**
* app_bond.h
*
* \brief   Get the bond device address according to the link index.
*
* \param[in] index       Local BT link index(can be 0x00: link1, or 0x01: link2).
* \param[in] *bd_addr    Remote BT device address.
*
* \return The status of get bond device address according to the link index.
* \retval true     Get bond address by link index succeed.
* \retval false    Get bond address by link index failed.
*/
bool app_src_legacy_get_bond_addr_by_index(uint8_t index, uint8_t *bd_addr);

/**
* app_bond.h
*
* \brief   Save bond device information and link key.
*
* \param[in] index       Local BT link index(can be 0x00: link1, or 0x01: link2).
* \param[in] *bd_addr    Remote BT device address.
* \param[in] *linkkey    Link key.
* \param[in] key_type    Link key type.
*
* \return The status of saving bond device information and link key.
* \retval true     Save bond device information and link key succeed.
* \retval false    Save bond device information and link key failed.
*/
bool app_src_legacy_save_bond(uint8_t index, uint8_t *bd_addr, uint8_t *linkkey, uint8_t key_type);

/**
* app_bond.h
*
* \brief   Add and save bond flag.
*
* \param[in] *bd_addr    Remote BT device address.
* \param[in] bond_mask   Bond mask.
*
* \return The status of adding and saving bond flag.
* \retval true     Add and save bond flag succeed.
* \retval false    Add and save bond flag failed.
*/
bool app_src_legacy_add_bond_flag(uint8_t *bd_addr, uint32_t bond_mask);

/**
* app_bond.h
*
* \brief   Remove bond flag.
*
* \param[in] *bd_addr    Remote BT device address.
* \param[in] bond_mask   Bond mask.
*
* \return The status of removing bond flag.
* \retval true     Remove bond flag succeed.
* \retval false    Remove save bond flag failed.
*/
bool app_src_legacy_remove_bond_flag(uint8_t *bd_addr, uint32_t bond_mask);

/**
* app_bond.h
*
* \brief   Get bond link key infromation according to the link index.
*
* \param[in] index       Local BT link index(can be 0x00: link1, or 0x01: link2).
* \param[in] *linkkey    Link key.
* \param[in] key_type    Link key type.
*
* \return The status of geting bond link key information by index.
* \retval true     Get bond link key information succeed.
* \retval false    Get bond link key information failed.
*/
bool app_src_legacy_get_bond_by_index(uint8_t index, uint8_t *link_key, uint8_t *key_type);

/**
* app_bond.h
*
* \brief   Get bond link key infromation according to the remote bt address.
*
* \param[in] *bd_addr    Remote BT device address.
* \param[in] *linkkey    Link key.
* \param[in] key_type    Link key type.
*
* \return The status of geting bond link key information by address.
* \retval true     Get bond link key information succeed.
* \retval false    Get bond link key information failed.
*/
bool app_src_legacy_get_bond_by_addr(uint8_t *bd_addr, uint8_t *link_key, uint8_t *key_type);

/**
* app_bond.h
*
* \brief   Delete bond device information according to the link index.
*
* \param[in] index       Local BT link index(can be 0x00: link1, or 0x01: link2).
*
* \return The status of deleting bond device information by link index.
* \retval true     Delete bond device information by link index succeed.
* \retval false    Delete bond device information by link index failed.
*/
bool app_src_legacy_delete_bond_by_index(uint8_t index);

/**
* app_bond.h
*
* \brief   Delete bond device information according to the address.
*
* \param[in] *bd_addr    Remote BT device address.
*
* \return The status of get bond device address according to the address.
* \retval true     Delete bond device information by address succeed.
* \retval false    Delete bond device information by address failed.
*/
bool app_src_legacy_delete_bond_by_addr(uint8_t *bd_addr);

/**
* app_bond.h
*
* \brief   Get the maximum number of bound devices.
*
* \return The number of maximum bound devices.
* \retval legacy_bond_num
*/
uint8_t app_src_legacy_get_max_bond_num(void);

/**
* app_bond.h
*
* \brief   Add and save headset information.
*
* \param[in] index       Local BT link index(can be 0x00: link1, or 0x01: link2).
* \param[in] *bd_addr    Remote BT device address.
* \param[in] *info       Headset information.
*
* \return The status of adding headset information.
* \retval true     Add and save headset information succeed.
* \retval false    Add and save headset information failed.
*/
bool app_src_legacy_add_hs_info(uint8_t index, uint8_t *bd_addr, uint8_t *info);

/**
* app_bond.h
*
* \brief   Get headset information.
*
* \param[in] *bd_addr    Remote BT device address.
* \param[in] *info       Headset information.
*
* \return The status of getting headset information.
* \retval true     Get headset information succeed.
* \retval false    Get headset information failed.
*/
bool app_src_legacy_get_hs_info(uint8_t *bd_addr, uint8_t *info);

/**
* app_bond.h
*
* \brief   Get the flag of whether the headset is lock to dongle .
*
* \return The flag of whether the headset is lock to dongle.
* \retval lock_flag.
*/
uint8_t app_src_legacy_get_lock_flag_by_index(uint8_t index);

/**
* app_bond.h
*
* \brief   Save 1v1 lock address and flag.
*
* \param[in] index       Local BT link index(can be 0x00: link1, or 0x01: link2).
* \param[in] *bd_addr    Remote BT device address.
* \param[in] lock_flag   Lock flag.
*
* \return The status of saving lock information.
* \retval true     Save lock information succeed.
* \retval false    Save lock information failed.
*/
bool app_src_legacy_save_lock_info(uint8_t index, uint8_t *bd_addr, uint8_t lock_flag);

/**
* app_bond.h
*
* \brief   Get the locked device address according to the link index.
*
* \param[in] index       Local BT link index(can be 0x00: link1, or 0x01: link2).
* \param[in] *bd_addr    Locked BT device address.
*
* \return The status of get locked device address according to the link index.
* \retval true     Get locked address by link index succeed.
* \retval false    Get locked address by link index failed.
*/
bool app_src_legacy_get_lock_addr_by_index(uint8_t index, uint8_t *bd_addr);

/**
* app_bond.h
*
* \brief   Lock information init. Load locked information from storage.
*/
void app_src_legacy_lock_info_init(void);

/**
* app_bond.h
*
* \brief   Clear locked information.
*/
void app_src_legacy_clear_lock_info(void);


/** @} */ /* End of group APP_BOND_Functions */

/** End of APP_BOND
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BOND_H_ */
