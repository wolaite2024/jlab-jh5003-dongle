#ifndef BT_BOND_MGR_H
#define BT_BOND_MGR_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "gap.h"

/** @defgroup BT_BOND_COMMON Bluetooth Bond Common
  * @brief Bluetooth Bond Common Module
  * @{
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/**
 * @defgroup BT_BOND_COMMON_Exported_Functions Bluetooth Bond Common Exported Functions
 *
 * @{
 */
/**
 * @brief Initialize parameters of Bluetooth bond manager.
 *
 * <b>Example usage</b>
 * \code{.c}
    static void app_bt_gap_init(void)
    {
        ......
        #if CONFIG_REALTEK_APP_BOND_MGR_SUPPORT
            bt_bond_init();
        #if ISOC_AUDIO_SUPPORT
            ble_audio_bond_init();
        #endif
        #endif
        ......
    }
 * \endcode
 */
void bt_bond_init(void);

/**
 * @brief GAP message handler.
 *
 * Handle GAP message when receive GAP_MSG_APP_BOND_MANAGER_INFO
 *
 * @param[in] p_data   Pointer to GAP message data.
 * @return T_APP_RESULT
 *
 * <b>Example usage</b>
 * \code{.c}
    static T_APP_RESULT app_ble_gap_cb(uint8_t cb_type, void *p_cb_data)
    {
        ......
        switch (cb_type)
        {
        case GAP_MSG_APP_BOND_MANAGER_INFO:
            {
                result = bt_bond_mgr_handle_gap_msg(cb_data.p_le_cb_data);
            }
            break;
        }
    }
 * \endcode
 */
T_APP_RESULT bt_bond_mgr_handle_gap_msg(void *p_data);

/** End of BT_BOND_COMMON_Exported_Functions
  * @}
  */
/** End of BT_BOND_COMMON
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif
