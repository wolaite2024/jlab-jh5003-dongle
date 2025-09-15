/**
************************************************************************************************************
*            Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file      charger_api.h
* @brief     Charger api head file.
* @author    Kellan Ho
* @date      2024-10-11
* @version   v1.1
*************************************************************************************************************
*/

#ifndef __CHARGER_API_H
#define __CHARGER_API_H
#include <stdint.h>


/** @defgroup 87x3e_CHARGER_API Charger API Sets
  * @brief Charger module API sets.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup 87x3e_CHARGER_API_Exported_Types Charger API Sets Exported Types
  * @{
  */

/**  @brief  Charger states. */
typedef enum
{
    STATE_CHARGER_START = 0,
    STATE_CHARGER_PRE_CHARGE,
    STATE_CHARGER_FAST_CHARGE,
    STATE_CHARGER_FINISH,
    STATE_CHARGER_ERROR,
    STATE_CHARGER_END,
} T_CHARGER_STATE;

/**  @brief  Charger error code. */
typedef enum
{
    CHARGER_NO_ERROR = 0,
    CHARGER_ERROR_OPEN,
    CHARGER_ERROR_SHORT,
    CHARGER_ERROR_PRE_CHARGE_TIMEOUT,
    CHARGER_ERROR_FAST_CHARGE_TIMEOUT,
    CHARGER_ERROR_CURRENT,
    CHARGER_ERROR_VOLTAGE,
    CHARGER_ERROR_ADC_INTERRUPT,
    CHARGER_ERROR_THERMISTOR_1,
    CHARGER_ERROR_THERMISTOR_2,
    CHARGER_ERROR_FW_OTP
} T_CHARGER_ERROR_CODE;

/**  @brief  Charger state change callback function prototype. */
typedef void (*CHARGER_STATE_CALLBACK)(T_CHARGER_STATE);

/**  @brief  Charge state change callback function prototype. */
typedef void (*STATE_OF_CHARGE_CALLBACK)(uint8_t);

/** End of 87x3e_CHARGER_API_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/**
 * @defgroup 87x3e_CHARGER_API_Exported_Functions Charger API Sets Exported Functions
 * @{
 */

/**
    * @brief      Register the user-defined callback function, which will be called by charger module when charge state changes.
    * @param      callback_func: Callback function to be registered.
    * @return     None.
    */
extern void (*charger_api_reg_state_of_charge_callback)(STATE_OF_CHARGE_CALLBACK callback_func);

/**
    * @brief      Unregister the user-defined @ref STATE_OF_CHARGE_CALLBACK callback function.
    * @param      None.
    * @return     None.
    */
extern void (*charger_api_unreg_state_of_charge_callback)(void);

/**
    * @brief      Register the user-defined callback function, which will be called by charger moudle when charger module state changes.
    * @param      callback_func: Callback function to be registered.
    * @return     None.
    */
extern void (*charger_api_reg_charger_state_callback)(CHARGER_STATE_CALLBACK callback_func);

/**
    * @brief      Unregister the user-defined @ref CHARGER_STATE_CALLBACK callback function.
    * @param      None.
    * @return     None.
    */
extern void (*charger_api_unreg_charger_state_callback)(void);

/**
    * @brief      Return state of charge.
    * @param      None.
    * @return     Charge state.
    */
extern uint8_t (*charger_api_get_state_of_charge)(void);

/**
    * @brief      Return charger module state.
    * @param      None.
    * @return     Charger state @ref T_CHARGER_STATE.
    */
extern T_CHARGER_STATE(*charger_api_get_charger_state)(void);

/**
    * @brief      Return error code of charger module.
    * @param      None.
    * @return     Charger module error code @ref T_CHARGER_ERROR_CODE.
    */
extern T_CHARGER_ERROR_CODE(*charger_api_get_error_code)(void);

/**
    * @brief      Return maximum current of adapter.
    * @param      None.
    * @return     Maximum current of adapter.
    */
extern uint16_t (*charger_api_get_adapter_current)(void);

/**
    * @brief      Return full charge current (1C).
    * @param      None.
    * @return     Full charge current.
    */
extern uint16_t (*charger_api_get_full_current)(void);

/**
    * @brief      Set maximum current of adapter.
    * @note       It will cause soft reset of charger module.
    * @param      adapter_current: Adapter current to te set.
    * @return     None.
    */
extern void (*charger_api_set_adapter_current)(uint16_t adapter_current);

/**
    * @brief      Set full charge current (1C).
    * @note       It will cause soft reset of charger module.
    * @param      full_current: Full current to te set.
    * @return     None.
    */
extern void (*charger_api_set_full_current)(uint16_t full_current);

/**
    * @brief      Enable charger module manually.
    * @note       It will cause soft reset of charger module and force discharger module to be disable if discharger module is running.
    * @param      None.
    * @return     None.
    */
extern void (*charger_api_enable_charger)(void);

/**
    * @brief      Disable charger module manually.
    * @param      None.
    * @return     None.
    */
extern void (*charger_api_disable_charger)(void);

/**
    * @brief      Enable discharger module manually.
    * @note       It will cause soft reset of discharger module and force charger module to be disable if charger module is running.
    * @param      None.
    * @return     None.
    */
extern void (*charger_api_enable_discharger)(void);

/**
    * @brief      Disable discharger module manually.
    * @param      None.
    * @return     None.
    */
extern void (*charger_api_disable_discharger)(void);

/** @} */ /* End of group 87x3e_CHARGER_API_Exported_Functions */
/** @} */ /* End of group 87x3e_CHARGER_API */


#endif  /* __CHARGER_API_H */

