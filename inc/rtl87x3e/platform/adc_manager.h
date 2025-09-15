/**
*****************************************************************************************
*     Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file      adc_manager.h
* @brief     API and related structures defined for multi-user for ADC driver.
* @details   The ADC manager module is useful for multi-user for one-shot ADC.
*            Please call adc_mgr_deinit when continuous mode ADC is in use.
* @author    Sandy
* @date      2024-10-11
* @version   v1.1
**************************************************************************************
*/

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/

#ifndef __ADC_MANAGER_H_
#define __ADC_MANAGER_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_adc.h"


/** @defgroup  87x3e_ADC_MANAGER ADC Manager
    * @brief   Arbitrator to access ADC. This module will overall manage charger and user ADC requests cocurrently.
    * @{
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup 87x3e_ADC_MANAGER_Exported_Types ADC Manager Exported Types
    * @{
    */

/**  @brief  The callback definition for ADC interrupt notify. */
typedef void (*adc_callback_function_t)(void *, uint32_t);

/** End of 87x3e_ADC_MANAGER_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup 87x3e_ADC_MANAGER_Exported_Functions ADC Manager Exported Functions
    * @{
    */

/**
    * @brief    Initialize ADC Manager module.
    * @note     Don't call this function unless call @ref adc_mgr_deinit before.
    * @param    req_num: The number of ADC manager channels that need to be requested.
    * @return   The initialization status of ADC manager.
    * @retval   true  ADC manager initialization successful.
    * @retval   false ADC manager initialization failed.
    */
extern bool (*adc_mgr_init)(uint8_t req_num);

/**
  * @brief      Close ADC Manager module, disable the ADC peripheral clock, and restore registers to their default values.
  */
void adc_mgr_deinit(void);

/**
  * @brief      Fills each p_adc_init member with its default value.
  * @param      p_adc_init: Pointer to a ADC_InitTypeDef structure which will be initialized.
  */
void adc_struct_init_req(ADC_InitTypeDef *p_adc_init);

/**
  * @brief      Request for a channel in ADC manager.
  * @note       If return successful, this channel will be reserved before free.
  * @param      p_adc_init: Pointer to a ADC_InitTypeDef structure which will be initialized.
  * @param      cb: Register callback for ADC interrupt notify.
  * @param      adc_mgr_index: The pointer to the return index corresponding to a channel.
  * @return     The status of the channel request.
  * @retval     true            Request successful, and adc_mgr_index is valid.
  * @retval     false           Request failed, and adc_mgr_index is not defined.
  */
extern bool (*adc_mgr_register_req)(ADC_InitTypeDef *p_adc_init, adc_callback_function_t cb,
                                    uint8_t *adc_mgr_index);

/**
  * @brief      Free the index corresponding channel.
  * @param      index: The number corresponding to a channel.
  */
extern void (*adc_mgr_free_chann)(uint8_t index);

/**
  * @brief      Enable the specified ADC interrupts.
  * @note       Make sure the index corresponding channel is valid.
  * @param      index: The number corresponding to a channel.
  * @param      enable_intr_bit_map: Specifies the ADC interrupt sources to be enabled.
  *             This parameter can be one of the following values:
  *             - ADC_INT_ONE_SHOT_DONE: ADC one shot mode done interrupt.
  *             - ADC_INT_FIFO_OVERFLOW: ADC FIFO full interrupt.
  *             - ADC_INT_FIFO_RD_REQ: ADC GDMA request interrupt.
  *             - ADC_INT_FIFO_RD_ERR: ADC FIFO read error interrupt.
  *             - ADC_INT_FIFO_TH: ADC FIFO threshold interrupt.
  * @return   The status of ADC interrupts enable.
  * @retval   true  Enable ADC interrupts successful.
  * @retval   false Enable ADC interrupts failed.
  */
bool adc_mgr_set_int_para(uint8_t index, uint32_t enable_intr_bit_map);

/**
  * @brief      Start up a one shot mode ADC.
  * @note       Please call @ref adc_mgr_read_data_req when callback function is called.
  * @param      index: The number corresponding to a channel.
  * @return     The status of the ADC manager enable.
  * @retval     true    Enable ADC manager successfully.
  * @retval     false   Enable ADC manager failed.
  */
extern bool (*adc_mgr_enable_req)(uint8_t index);

/**
  * @brief      Read ADC data according to the ADC schedule table bitmap.
  * @note       Please call @ref adc_mgr_read_data_req when callback function is called.
  * @param      index: The number corresponding to a channel.
  * @param      output_data: The pointer to the output data buffer.
  * @param      sched_bit_map: ADC schedule table bitmap.
  * @return     The status of data reading.
  * @retval     true    Read data successfully.
  * @retval     false   Reading data failed.
  */
bool adc_mgr_read_data_req(uint8_t index, uint16_t *output_data, uint16_t sched_bit_map);

/** @} */ /* End of group 87x3e_ADC_MANAGER_Exported_Functions */

/** @} */ /* End of group 87x3e_ADC_MANAGER */

#ifdef __cplusplus
}
#endif

#endif
