/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      bt_bond_common.h
* @brief     Key storage function.
* @details
* @author    Jane
* @date      2016-02-18
* @version   v0.1
* *********************************************************************************************************
*/

#ifndef     BT_BOND_COMMON_H
#define     BT_BOND_COMMON_H

#include "bt_bond_le.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @defgroup BT_BOND_LE Bluetooth Bond LE
  * @brief Bluetooth Bond LE Module
  * @{
  */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup BT_BOND_LE_Exported_Macros Bluetooth Bond LE Exported Macros
  * @{
  */

/** @defgroup BT_BOND_INFO_FLAG Bluetooth Bond Info Flag Bits
* @{
 */
/*Bond state: indicate bond information exist or not. */
#define BT_BOND_INFO_EXIST_FLAG 0x01
/*Bond state: indicate bond information need update or not. */
#define BT_BOND_INFO_NEED_UPDATE_FLAG 0x02
/*Bond state: indicate re-pair or not. */
#define BT_BOND_INFO_REPAIR_FLAG      0x04
/**
  * @}
  */

/** @defgroup BT_BOND_MSG_TYPE Bluetooth Bond Message Types
* @{
 */
#define BT_BOND_MSG_LE_BOND_ADD       0x00
#define BT_BOND_MSG_LE_BOND_REMOVE    0x01
#define BT_BOND_MSG_LE_BOND_CLEAR     0x02
#define BT_BOND_MSG_LE_BOND_UPDATE    0x03
//#define BT_BOND_MSG_LE_SET_HIGH_PRIORITY    0x04
#define BT_BOND_MSG_LE_BOND_FULL      0x05

#define BT_BOND_MSG_LE_BOND_GET       0x10
/**
  * @}
  */

/** End of BT_BOND_LE_Exported_Macros
  * @}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup BT_BOND_LE_Exported_Types Bluetooth Bond LE Exported Types
  * @{
  */
typedef struct
{
    uint8_t         bond_state;
    uint8_t         key_type;
    uint8_t         remote_bd_type;
    uint8_t         local_bd_type;
    uint8_t         remote_bd[6];
    uint8_t         local_bd[6];
    uint32_t        bond_flag;
} T_BT_BOND_INFO;

typedef void(*P_FUN_BT_BOND_CB)(uint8_t cb_type, void *p_cb_data);

/** End of BT_BOND_LE_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/**
 * @defgroup BT_BOND_LE_Exported_Functions Bluetooth Bond LE Exported Functions
 *
 * @{
 */

/**
 * @brief bt_bond_register_app_cb
 *
 * APP can call bt_bond_register_app_cb to register callback to get Bluetooth Bond modify information.
 * APP can register more than one callback function using bt_bond_register_app_cb.
 * APP will not send message BT_BOND_MSG_LE_BOND_GET.
 *
 * @param app_callback Callback function: @ref P_FUN_BT_BOND_CB.
 * @return   Operation result.
 * @retval   true   Success.
 * @retval   false  Failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void ble_bond_sync_init(void)
    {
        ......
        bt_bond_register_app_cb(ble_bond_sync_cb);
        ......
    }

    void ble_bond_sync_cb(uint8_t cb_type, void *p_cb_data)
    {
        ......
        switch (cb_type)
        {
        case BT_BOND_MSG_LE_BOND_ADD:
            {

            }
            break;

        case BT_BOND_MSG_LE_BOND_REMOVE:
            {

            }
            break;

        case BT_BOND_MSG_LE_BOND_CLEAR:
            {

            }
            break;

        case BT_BOND_MSG_LE_BOND_UPDATE:
            {

            }
            break;

        default:
            break;
        }
        return;
    }
 * \endcode
 */
bool bt_bond_register_app_cb(P_FUN_BT_BOND_CB app_callback);

/**
 * @brief bt_bond_unregister_app_cb
 *
 * APP can call bt_bond_unregister_app_cb to unregister callback to get Bluetooth Bond modify information.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 * @param app_callback Callback function: @ref P_FUN_BT_BOND_CB.
 * @return   Operation result.
 * @retval   true   Success.
 * @retval   false  Failed.
 */
bool bt_bond_unregister_app_cb(P_FUN_BT_BOND_CB app_callback);

/**
 * @brief bt_bond_cfg_send_bond_full
 *
 * APP can call bt_bond_cfg_send_bond_full to configure whether to send BT_BOND_MSG_LE_BOND_FULL when the number of bonds is full.
 * If the APP deletes unnecessary bond information when APP receives the message BT_BOND_MSG_LE_BOND_FULL, the current pairing procedure can continue.
 * If the APP does not delete bond information when APP receives the message BT_BOND_MSG_LE_BOND_FULL, the current pairing procedure will fail.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 * @param enable Whether to send BT_BOND_MSG_LE_BOND_FULL when the number of bonds is full.
 * \arg    true    When the number of bonds is full, Bluetooth Bond module will send BT_BOND_MSG_LE_BOND_FULL.
 * \arg    false   When the number of bonds is full, Bluetooth Bond module will auto delete low priority bond information.
 * @return void.
 *
 * <b>Example usage</b>
 * \code{.c}
    void ble_bond_sync_init(void)
    {
        ......
        bt_bond_register_app_cb(ble_bond_sync_cb);
        bt_bond_cfg_send_bond_full(true);
        ......
    }

    void ble_bond_sync_cb(uint8_t cb_type, void *p_cb_data)
    {
        ......
        switch (cb_type)
        {
        case BT_BOND_MSG_LE_BOND_FULL:
            {
                T_LE_BOND_ENTRY *p_entry = NULL;
                p_entry = bt_le_get_low_priority_bond();
                if (p_entry != NULL)
                {
                    bt_le_delete_bond(p_entry);
                }
            }
            break;

        default:
            break;
        }
        return;
    }
 * \endcode
 */
void bt_bond_cfg_send_bond_full(bool enable);

/**
 * @brief bt_bond_register_bond_get_cb
 *
 * APP can call bt_bond_register_bond_get_cb to register callback to handle message BT_BOND_MSG_LE_BOND_GET.
 * APP can only register one callback function.
 * APP will send message BT_BOND_MSG_LE_BOND_GET when no matching key entry is found.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 * @param app_callback Callback function: @ref P_FUN_BT_BOND_CB.
 * @return   Operation result.
 * @retval   true   Success.
 * @retval   false  Failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        ......
        bt_bond_register_bond_get_cb(ble_bond_get_cb);
    }

    void ble_bond_get_cb(uint8_t cb_type, void *p_cb_data)
    {
        ......
        switch (cb_type)
        {
        case BT_BOND_MSG_LE_BOND_GET:
            {
                T_BT_LE_BOND_CB_DATA cb_data;
                T_LE_BOND_ENTRY *p_entry = NULL;

                memcpy(&cb_data, p_cb_data, sizeof(T_BT_LE_BOND_CB_DATA));
                APP_PRINT_INFO5("BT_BOND_MSG_LE_BOND_GET: bd_addr %s, bd_type %d, local_bd_addr %s, local_bd_type %d, key_type %d",
                                TRACE_BDADDR(cb_data.p_le_bond_get->bd_addr),
                                cb_data.p_le_bond_get->bd_type,
                                TRACE_BDADDR(cb_data.p_le_bond_get->local_bd_addr),
                                cb_data.p_le_bond_get->local_bd_type,
                                cb_data.p_le_bond_get->key_type);

                p_entry = bt_le_find_key_entry_by_priority(1);
                if (p_entry != NULL)
                {
                    T_BT_LE_LTK le_ltk;
                    if(bt_le_dev_info_get_local_ltk(p_entry, (uint8_t *)&le_ltk))
                    {
                        cb_data.p_le_bond_get->cfm_cause = GAP_CFM_CAUSE_ACCEPT;
                        cb_data.p_le_bond_get->key_len = le_ltk.link_key_length;
                        memcpy(cb_data.p_le_bond_get->key_data,le_ltk.key, 28);
                    }
                }
            }
            break;

        default:
            break;
        }
        return;
    }
 * \endcode
 */
bool bt_bond_register_bond_get_cb(P_FUN_BT_BOND_CB app_callback);
/** End of BT_BOND_LE_Exported_Functions
  * @}
  */
/** End of BT_BOND_LE
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* BT_BOND_COMMON_H */
