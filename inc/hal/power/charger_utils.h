/**
************************************************************************************************************
*            Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file      charger_utils.h
* @brief     Charger utils API head file.
* @author
* @date
* @version   v1.1
*************************************************************************************************************
*/

#ifndef _CHARGER_UTILS_H_
#define _CHARGER_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

/** @addtogroup CHARGER_UTILS Charger Utils
  * @brief Charger function module.
  * @{
  */

/** @defgroup CHARGER_UTILS_Exported_Constants Charger Utils Exported Constants
  * @{
  */

/**  @brief RTK charger function return general error code. */
typedef enum
{
    CHARGER_UTILS_SUCCESS,
    CHARGER_UTILS_NOT_SUPPROTED,
    CHARGER_UTILS_NOT_ENABLED,
    CHARGER_UTILS_INVALID_PARAM,
} T_CHARGER_UTILS_ERROR;

typedef struct _charger_utils_config
{
    uint16_t pre_charge_current;      /* Charge current of pre-charge state. Unit: mA. Range: 5 ~ 50 (mA). */
    uint16_t pre_charge_timeout;      /* The timeout time of pre-charge stage. Unit: minutes. Range: 1 ~ 65535(minutes). */
    uint16_t fast_charge_current;     /* Charge current of fast-charge state. Unit: mA. For RTL87X3E, range: 20 ~ 400 (mA). For RTL87X3D, range of internal charger: 30 ~ 400 (mA), range of external BJT charger: 405 ~ 1000 (mA). */
    uint16_t fast_charge_timeout;     /* The timeout time of fast-charge stage. Unit: minutes. Range: 3 ~ 65535(minutes). */
    uint16_t full_voltage;            /* Voltage Limit of Battery. Unit: mV. Range: 4000 ~ 4400(mV). */
    uint16_t pre_charge_voltage;      /* Pre-charge voltage. The value must be a multiple of 10. Unit: mV. Range: 2680 ~ 3310(mV). */
    uint16_t re_charge_voltage;       /* Re-charge voltage. The value must be a multiple of 10. Unit: mV. Range: 3680 ~ 4310(mV). */
    uint16_t finish_current;          /* Charge finish current. Unit: mA. Range: 5 ~ 50 (mA). */
} T_CHARGER_UTILS_CONFIG;

/** End of group CHARGER_UTILS_Exported_Constants
  * @}
  */

/** @defgroup CHARGER_UTILS_Exported_Functions Charger Utils Exported Functions
  * @{
  */

/**
 * charger_utils.h
 *
 * \brief   Get charging voltage.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[out]   volt: Battery voltage, unit: mV.
 *
 * \return          The status of getting voltage.
 * \retval  CHARGER_UTILS_SUCCESS           Current charging info is getting successfully.
 * \retval  CHARGER_UTILS_NOT_SUPPROTED     Charging info getting failed.
 * \retval  CHARGER_UTILS_NOT_ENABLED       Charger is not enabled. could not get battery information from charger module.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void charger_utils_demo(void)
 * {
 *     T_CHARGER_UTILS_ERROR status;
 *     uint16_t voltage;
 *
 *     status = charger_utils_get_batt_volt(&voltage);
 *     if(status == CHARGER_UTILS_SUCCESS)
 *     {
 *          APP_PRINT_INFO1("charger_utils_demo: voltage %d", voltage);
 *     }
 *     else
 *     {
 *          APP_PRINT_ERROR1("charger_utils_demo: error code %d", status);
 *     }
 * }
 * \endcode
 *
 * \ingroup CHARGER_UTILS_Exported_Functions
 */
T_CHARGER_UTILS_ERROR charger_utils_get_batt_volt(uint16_t *volt);

/**
 * charger_utils.h
 *
 * \brief   Get charging current, unit: mA.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[out]   current: Charging current, positive in charging mode, negative in discharging mode.
 *
 * \return          The status of getting current.
 * \retval  CHARGER_UTILS_SUCCESS           Current charging info is getting successfully.
 * \retval  CHARGER_UTILS_NOT_SUPPROTED     Charging info getting failed.
 * \retval  CHARGER_UTILS_NOT_ENABLED       Charger is not enabled. could not get battery information from charger module.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void charger_utils_demo(void)
 * {
 *     T_CHARGER_UTILS_ERROR status;
 *     int16_t current;
 *
 *     status = charger_utils_get_batt_curr(&current);
 *     if(status == CHARGER_UTILS_SUCCESS)
 *     {
 *          APP_PRINT_INFO1("charger_utils_demo: current %d", current);
 *     }
 *     else
 *     {
 *          APP_PRINT_ERROR1("charger_utils_demo: error code %d", status);
 *     }
 * }
 * \endcode
 *
 * \ingroup CHARGER_UTILS_Exported_Functions
 */
T_CHARGER_UTILS_ERROR charger_utils_get_batt_curr(int16_t *current);

/**
 * \brief   Get charging temperature1 and temperature2.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[out]   temperature1: Temperature1, unit: mV.
 * \param[out]   temperature2: Temperature2, unit: mV.
 *
 * \return          The status of getting temperature.
 * \retval  CHARGER_UTILS_SUCCESS           Temperature obtained successfully.
 * \retval  CHARGER_UTILS_NOT_SUPPROTED     Getting temperature is not supported.
 * \retval  CHARGER_UTILS_NOT_ENABLED       Charger is not enabled. Could not get temperature information from charger module.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void charger_utils_demo(void)
 * {
 *     T_CHARGER_UTILS_ERROR status;
 *     uint16_t temperature1, temperature2;
 *
 *     status = charger_utils_get_batt_temp(&temperature1, &temperature2);
 *     if(status == CHARGER_UTILS_SUCCESS)
 *     {
 *          APP_PRINT_INFO2("charger_utils_demo: temperature1 %d, temperature2 %d", temperature1, temperature2);
 *     }
 *     else
 *     {
 *          APP_PRINT_ERROR1("charger_utils_demo: error code %d", status);
 *     }
 * }
 * \endcode
 *
 * \ingroup CHARGER_UTILS_Exported_Functions
 */
T_CHARGER_UTILS_ERROR charger_utils_get_batt_temp(uint16_t *temperature1, uint16_t *temperature2);

/**
 * charger_utils.h
 *
 * \brief   Get charging adapter voltage, unit: mV.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[out]   volt: Adapter voltage.
 *
 * \return          The status of getting adapter voltage.
 * \retval  CHARGER_UTILS_SUCCESS           Adapter voltage obtained successfully.
 * \retval  CHARGER_UTILS_NOT_SUPPROTED     Getting adapter voltage is not supported.
 * \retval  CHARGER_UTILS_NOT_ENABLED       Charger is not enabled. Could not get adapter voltage information from charger module.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void charger_utils_demo(void)
 * {
 *     T_CHARGER_UTILS_ERROR status;
 *     uint16_t voltage;
 *
 *     status = charger_utils_get_adapter_volt(&voltage);
 *     if(status == CHARGER_UTILS_SUCCESS)
 *     {
 *          APP_PRINT_INFO1("charger_utils_demo: voltage %d", voltage);
 *     }
 *     else
 *     {
 *          APP_PRINT_ERROR1("charger_utils_demo: error code %d", status);
 *     }
 * }
 * \endcode
 *
 * \ingroup CHARGER_UTILS_Exported_Functions
 */
T_CHARGER_UTILS_ERROR charger_utils_get_adapter_volt(uint16_t *volt);

/**
 * charger_utils.h
 *
 * \brief   Enable or disable charger.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[in]   enable: Enable or disable charger.
 *              This parameter can be one of the following values:
 *              \arg true: Enable charger.
 *              \arg false: Disable charger.
 * @return      None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void charger_utils_demo(void)
 * {
 *     charger_utils_charger_auto_enable(true);
 * }
 * \endcode
 *
 * \ingroup CHARGER_UTILS_Exported_Functions
 */
void charger_utils_charger_auto_enable(bool enable);

/**
 * charger_utils.h
 *
 * \brief   Get charger thermistor detection enable status.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[in]   None.
 * @return      Charger thermistor detection enable state.
 * @retval      true    Charger thermistor detection is supported.
 * @retval      false   Charger thermistor detection is not supported.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void charger_utils_demo(void)
 * {
 *     bool status = charger_utils_get_thermistor_enable_state();
 * }
 * \endcode
 *
 * \ingroup CHARGER_UTILS_Exported_Functions
 */
bool charger_utils_get_thermistor_enable_state(void);

/**
 * charger_utils.h
 *
 * \brief   Set charging current and full voltage, restart charger fsm if charger is running.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  p_charger_config     The configuration structure of charging current and full voltage.
 *
 * \return          The status of setting charging current and full voltage.
 * \retval  CHARGER_UTILS_SUCCESS           The charging current and full voltage are set successfully.
 * \retval  CHARGER_UTILS_INVALID_PARAM     Invalid charging current and full voltage parameters.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void charger_utils_demo(void)
 * {
 *     T_CHARGER_UTILS_CONFIG charger_config;
 *
 *     if(charger_utils_get_all_param(&charger_config) == CHARGER_UTILS_SUCCESS)
 *     {
 *          charger_config.fast_charge_current = 50;
 *          if(charger_utils_set_all_param(&charger_config) != CHARGER_UTILS_SUCCESS)
 *          {
 *              APP_PRINT_ERROR0("charger_utils_demo: set param fail!");
 *          }
 *     }
 * }
 * \endcode
 *
 * \ingroup CHARGER_UTILS_Exported_Functions
 */
T_CHARGER_UTILS_ERROR charger_utils_set_all_param(T_CHARGER_UTILS_CONFIG *p_charger_config);

/**
 * charger_utils.h
 *
 * \brief   Get charging current and full voltage configurations.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  p_charger_config     The configuration structure of charging current and full voltage.
 *
 * \return          The status of getting charging current and full voltage configurations.
 * \retval  CHARGER_UTILS_SUCCESS           The charging current and full voltage configurations are obtained successfully.
 * \retval  CHARGER_UTILS_INVALID_PARAM     Invalid parameter.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void charger_utils_demo(void)
 * {
 *     T_CHARGER_UTILS_CONFIG charger_config;
 *
 *     if(charger_utils_get_all_param(&charger_config) == CHARGER_UTILS_SUCCESS)
 *     {
 *          charger_config.fast_charge_current = 50;
 *          if(charger_utils_set_all_param(&charger_config) != CHARGER_UTILS_SUCCESS)
 *          {
 *              APP_PRINT_ERROR0("charger_utils_demo: set param fail!");
 *          }
 *     }
 * }
 * \endcode
 *
 * \ingroup CHARGER_UTILS_Exported_Functions
 */
T_CHARGER_UTILS_ERROR charger_utils_get_all_param(T_CHARGER_UTILS_CONFIG *p_charger_config);

/**
 * charger_utils.h
 *
 * \brief   Get charging thermistor1 adc channel configurations.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \param[in]  p_thermistor_adc_channel: Charging thermistor1 adc channel configurations.
 *
 * \return          The status of getting charging thermistor1 adc channel configurations.
 * \retval  CHARGER_UTILS_SUCCESS           The charging thermistor1 adc channel configurations are obtained successfully.
 * \retval  CHARGER_UTILS_INVALID_PARAM     Invalid parameter.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void charger_utils_demo(void)
 * {
 *     T_CHARGER_UTILS_ERROR status;
 *     uint8_t thermistor_adc_channel;
 *
 *     status = charger_utils_get_thermistor_1_pin(&thermistor_adc_channel);
 *     if(status == CHARGER_UTILS_SUCCESS)
 *     {
 *          APP_PRINT_INFO1("charger_utils_demo: thermistor_adc_channel %d", thermistor_adc_channel);
 *     }
 *     else
 *     {
 *          APP_PRINT_ERROR1("charger_utils_demo: error code %d", status);
 *     }
 * }
 * \endcode
 *
 * \ingroup CHARGER_UTILS_Exported_Functions
 */
T_CHARGER_UTILS_ERROR charger_utils_get_thermistor_1_pin(uint8_t *p_thermistor_adc_channel);

/** @} */ /* End of group CHARGER_UTILS_Exported_Functions */
/** @} */ /* End of group CHARGER_UTILS */

#endif
