/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_BOND_H_
#define _BT_BOND_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    BT_BOND_X3E BT Bond
 *
 * \brief   Manipulate BT bond information.
 */

/**
 * \brief   Get BT bond maximum number prototype.
 *
 * \return  The BT bond maximum number.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef uint8_t (*P_BT_MAX_BOND_NUM_GET)(void);

/**
 * \brief   Get BT bond number prototype.
 *
 * \return  The BT bond number.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef uint8_t (*P_BT_BOND_NUM_GET)(void);

/**
 * \brief   Get BT address by BT bond priority.
 *
 * \param[in] priority   Bond priority.
 * \param[in] bd_addr    BT address.
 *
 * \return  The status of get BT bond address.
 * \retval true    Get BT address successfully.
 * \retval false   Get BT address was failed.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef bool (*P_BT_BOND_ADDR_GET)(uint8_t priority, uint8_t *bd_addr);

/**
 * \brief   Get BT bond index by BT address.
 *
 * \param[in] bd_addr    BT address.
 * \param[in] index      BT bond index.
 *
 * \return  The status of get BT bond index.
 * \retval true    Get BT bond index successfully.
 * \retval false   Get BT bond index was failed.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef bool (*P_BT_BOND_INDEX_GET)(uint8_t *bd_addr, uint8_t *index);

/**
 * \brief   Get BT bond flag by BT address.
 *
 * \param[in] bd_addr    BT address.
 * \param[in] bond_flag  BT bond flag.
 *
 * \return  The status of get BT bond flag.
 * \retval true    Get BT bond flag successfully.
 * \retval false   Get BT bond flag was failed.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef bool (*P_BT_BOND_FLAG_GET)(uint8_t *bd_addr, uint32_t *bond_flag);

/**
 * \brief   Set BT bond flag by BT address.
 *
 * \param[in] bd_addr    BT address.
 * \param[in] bond_flag  BT bond flag.
 *
 * \return  The status of set BT bond flag.
 * \retval true    Set BT bond flag successfully.
 * \retval false   Get BT bond flag was failed.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef bool (*P_BT_BOND_FLAG_SET)(uint8_t *bd_addr, uint32_t bond_flag);

/**
 * \brief   Initialize BT bond table
 *
 * \ingroup  BT_BOND_X3E
 */
typedef void (*P_BT_BOND_KEY_INIT)(void);

/**
 * \brief   Get BT bond link key
 *
 * \param[in] bd_addr    BT address.
 * \param[in] link_key   BT bond link key.
 * \param[in] key_type   BT bond link key type.
 *
 * \return  The status of get BT bond link key.
 * \retval true    Get BT bond link key successfully.
 * \retval false   Get BT bond link key was failed.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef bool (*P_BT_BOND_KEY_GET)(uint8_t *bd_addr, uint8_t *link_key, uint8_t *key_type);

/**
 * \brief   Set BT bond link key
 *
 * \param[in] bd_addr    BT address.
 * \param[in] link_key   BT bond link key.
 * \param[in] key_type   BT bond link key type.
 *
 * \return  The status of set BT bond link key.
 * \retval true    Set BT bond link key successfully.
 * \retval false   Set BT bond link key was failed.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef bool (*P_BT_BOND_KEY_SET)(uint8_t *bd_addr, uint8_t *linkkey, uint8_t key_type);

/**
 * \brief   Set BT bond information to highest priority.
 *
 * \param[in] bd_addr    BT address.
 *
 * \return  The status of set BT bond information priority.
 * \retval true    Set BT bond information priority successfully.
 * \retval false   Get BT bond information priority was failed.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef bool (*P_BT_BOND_PRIORITY_SET)(uint8_t *bd_addr);

/**
 * \brief   Delete BT bond information.
 *
 * \param[in] bd_addr    BT address.
 *
 * \return  The status of delete BT bond information.
 * \retval true    Delete BT bond information successfully.
 * \retval false   Delete BT bond information was failed.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef bool (*P_BT_BOND_DELETE)(uint8_t *bd_addr);

/**
 * \brief   Delete all BT bond informations.
 *
 * \ingroup  BT_BOND_X3E
 */
typedef void (*P_BT_BOND_CLEAR)(void);

/**
 * \brief   BT bond manager function struct.
 *
 * \ingroup BT_BOND_X3E
 */
typedef struct
{
    P_BT_MAX_BOND_NUM_GET p_bt_max_bond_num_get;
    P_BT_BOND_NUM_GET p_bt_bond_num_get;
    P_BT_BOND_ADDR_GET p_bt_bond_addr_get;
    P_BT_BOND_INDEX_GET p_bt_bond_index_get;
    P_BT_BOND_FLAG_GET p_bt_bond_flag_get;
    P_BT_BOND_FLAG_SET p_bt_bond_flag_set;
    P_BT_BOND_KEY_INIT p_bt_bond_key_init;
    P_BT_BOND_KEY_GET p_bt_bond_key_get;
    P_BT_BOND_KEY_SET p_bt_bond_key_set;
    P_BT_BOND_PRIORITY_SET p_bt_bond_priority_set;
    P_BT_BOND_DELETE p_bt_bond_delete;
    P_BT_BOND_CLEAR p_bt_bond_clear;
} T_LEGACY_BOND_HAL;

/**
 * \brief   Register BT bond manager function.
 *
 * \ingroup BT_BOND_X3E
 */
void bt_bond_register_hal(const T_LEGACY_BOND_HAL *p_hal);

/**
 * \brief   Get value of supported maximum bond device num.
 *
 * \return  The value of supported maximum bond device num.
 *
 * \ingroup BT_BOND_X3E
 */
uint8_t bt_max_bond_num_get(void);

/**
 * \brief   Get bond device num from bond storage.
 *
 * \return  The bond device num.
 *
 * \ingroup BT_BOND_X3E
 */
uint8_t bt_bond_num_get(void);

/**
 * \brief   Get Bluetooth device address from bond storage by priority.
 *
 * \param[in]  priority      The priority of bonded device.
 * \param[out] bd_addr       The Bluetooth device address got by priority.
 *
 * \note    Bond priority number starts at 1, rather than 0. Bond priority is descending
 *          with the corresponding number increasing.
 *
 * \return          The status of getting Bluetooth device address.
 * \retval true     The Bluetooth device address was got successfully.
 * \retval false    The Bluetooth device address was failed to get.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_addr_get(uint8_t priority, uint8_t *bd_addr);

/**
 * \brief   Get bond index from device address.
 *
 * \param[in]  bd_addr       The Bluetooth device address.
 * \param[out] index         The bond index.
 *
 *  \note    Bond index starts at 0.
 *
 * \return          The status of getting bond index.
 * \retval true     The bond index was got successfully.
 * \retval false    The bond index was failed to get.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_index_get(uint8_t *bd_addr, uint8_t *index);

/**
 * \brief   Get bond flag with Bluetooth device address from bond storage.
 *
 * \param[in]  bd_addr       The Bluetooth device address.
 * \param[out] bond_flag    The obtained bond flag.
 *
 * \return          The status of getting bond flag.
 * \retval true     The bond flag was got successfully.
 * \retval false    The bond flag was failed to get.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_flag_get(uint8_t *bd_addr, uint32_t *bond_flag);

/**
 * \brief   Save bond flag.
 *
 * \param[in] bd_addr       The Bluetooth device address.
 * \param[in] bond_flag     The bond flag to be saved.
 *
 * \return          The status of saving bond flag.
 * \retval true     The bond flag was saved successfully.
 * \retval false    The bond flag was failed to save.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_flag_set(uint8_t *bd_addr, uint32_t bond_flag);

/**
 * \brief   Add bond flag.
 *
 * \param[in] bd_addr       The Bluetooth device address.
 * \param[in] bond_flag     The bond flag to be added.
 *
 * \return          The status of adding bond flag.
 * \retval true     The bond flag was added successfully.
 * \retval false    The bond flag was failed to add.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_flag_add(uint8_t *bd_addr, uint32_t bond_flag);

/**
 * \brief   Remove bond flag.
 *
 * \param[in] bd_addr       The Bluetooth device address.
 * \param[in] bond_flag     The bond flag to be removed.
 *
 * \return          The status of removing bond flag.
 * \retval true     The bond flag was removed successfully.
 * \retval false    The bond flag was failed to remove.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_flag_remove(uint8_t *bd_addr, uint32_t bond_flag);

/**
 * \brief   Init link key.
 *
 *
 * \ingroup BT_BOND_X3E
 */
void bt_bond_key_init(void);

/**
 * \brief   Get link key with Bluetooth device address from bond storage.
 *
 * \param[in]  bd_addr       The Bluetooth device address.
 * \param[out] link_key      The link key.
 * \param[out] key_type      The type of link key.
 *
 * \return          The status of getting link key.
 * \retval true     The link key was got successfully.
 * \retval false    The link key was failed to get.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_key_get(uint8_t *bd_addr, uint8_t *link_key, uint8_t *key_type);

/**
 * \brief   Save link key.
 *
 * \param[in] bd_addr       The Bluetooth device address.
 * \param[in] linkkey       The link key to be saved.
 * \param[in] key_type      The type of link key.
 *
 * \return          The status of saving link key.
 * \retval true     The link key was saved successfully.
 * \retval false    The link key was failed to save.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_key_set(uint8_t *bd_addr, uint8_t *linkkey, uint8_t key_type);

/**
 * \brief   Adjust priority to highest in bond storage.
 *
 * \param[in] bd_addr       The Bluetooth device address that need to adjust priority.
 *
 * \return          The status of adjusting priority.
 * \retval true     The priority was adjusted successfully.
 * \retval false    The priority was failed to adjust.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_priority_set(uint8_t *bd_addr);

/**
 * \brief   Delete bond information in bond storage.
 *
 * \param[in] bd_addr       The Bluetooth device address that need to delete bond information.
 *
 * \return          The status of deleting bond information.
 * \retval true     The bond information was deleted successfully.
 * \retval false    The bond information was failed to delete.
 *
 * \ingroup BT_BOND_X3E
 */
bool bt_bond_delete(uint8_t *bd_addr);

/**
 * \brief   Delete all bond information in bond storage.
 *
 * \ingroup BT_BOND_X3E
 */
void bt_bond_clear(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_BOND_H_ */
