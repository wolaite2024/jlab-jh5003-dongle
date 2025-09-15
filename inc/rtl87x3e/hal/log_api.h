/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    log_api.h
* @brief   This file provides log API wrapper for sdk customer.
* @author  Sandy
* @date    2021-05-20
* @version v1.0
* *************************************************************************************
*/

#ifndef __LOG_API_H_
#define __LOG_API_H_
#include "stdbool.h"
#include "stdint.h"
/** @defgroup  HAL_87x3E_LOG_API    Log Control
    * @brief Log control.
    * @{
    */
/** @defgroup HAL_87x3E_LOG_CONTROL_EXPORTED_TYPE Log Control Exported Types
   * @brief
   * @{
   */
typedef void (*LOG_1W_UART_POLLING_FUNC)(uint8_t *, uint32_t);
typedef void (*LOG_1W_TX_DONE_CB)(void);
typedef void (*LOG_1W_UART_SEND_FUNC)(uint8_t *, uint16_t, LOG_1W_TX_DONE_CB);
/** End of HAL_87x3E_LOG_CONTROL_EXPORTED_TYPE
    * @}
    */
/** @defgroup HAL_87x3e_LOG_CONTROL_EXPORTED_FUNCTIONS Log Control Exported Functions
    * @brief
    * @{
    */
#ifdef __cplusplus
extern "C" {
#endif
/**
    * @brief  Get the log enable status.
    * @return The log enable status.
    * @retval true   Log enable.
    * @retval false  Log disable.
    */
bool log_enable_get(void);
/**
    * @brief  Set the log enable status.
    * @warning This API is nonly supported in the RTL87x3E.
    *          It is NOT supported in the RTL87x3D and RTL87x3G.
    * @param enable  Log enable status, false: log disable, true: log enable.
    */
void log_enable_set(bool enable);

/**
    * @brief  Set the trace string log enable status.
    * \xrefitem Added_API_2_12_0_0 "Added Since 2.12.0.0" "Added API"
    * @warning This API is supported in the RTL87x3E and RTL87x3D.
    *          It is NOT supported in the RTL87x3G.
    * @param enable  whether to output trace string, false: don't output, true: output.
    */
void log_enable_trace_string(bool enable);

/**
    * @brief  Get the trace string log enable status.
    *\xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
    * @warning This API is supported in the RTL87x3E and RTL87x3D.
    *          It is NOT supported in the RTL87x3G.
    * @return  The status whether to output trace string, false: don't output, true: output.
    */
bool log_enable_trace_string_get(void);
/**
    * @brief  Enable LEVEL_CRITICAL log.
    * @note   LEVEL_CRITICAL log is disabled by default. If LEVEL_CRITICAL log is enabled, it will be output ad LEVEL_INFO.
    *\xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
    * @warning This API is supported in the RTL87x3E and RTL87x3D.
    *          It is NOT supported in the RTL87x3G.
    * @param enable  Whether to output LEVEL_CRITICAL log, false: don't output, true: output.
    * @return void
    */
void log_enable_critical_level(bool enable);

/**
    * @brief  Get the LEVEL_CRITICAL log enable status.
    *\xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
    * @warning This API is supported in the RTL87x3E and RTL87x3D.
    *          It is NOT supported in the RTL87x3G.
    * @return The output status for LEVEL_CRITICAL log, true: output, false: don't output.
    */
bool log_enable_critical_level_get(void);

/**
    * @brief  Get trace_mask value.
    *\xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
    * @warning This API is supported in the RTL87x3E and RTL87x3D.
    *          It is NOT supported in the RTL87x3G.
    * @param[out] p_trace_mask_buf  Pointer to the buffer to get the trace_mask value.
    * @param[in] buf_size  Specified the size of p_trace_mask_buf. User can get the size of trace_mask by log_trace_mask_size_get API.
    * @return The operation status for getting trace_mask value.
    * @retval  true  Get the trace_mask value by p_trace_mask_buf successfully.
    * @retval  false Get trace_mask value  fail for parameter checking invalid.
    */
bool log_trace_mask_get(void *p_trace_mask_buf, uint32_t buf_size);

/**
    * @brief  Get trace_mask size, uint: bytes.
    *\xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
    * @warning This API is supported in the RTL87x3E and RTL87x3D.
    *          It is NOT supported in the RTL87x3G.
    * @return The trace_mask size, uint: bytes.
    */
uint32_t log_trace_mask_size_get(void);

/**
    * @brief  Enable one wire log.
    *\xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
    * @warning This API is supported in the RTL87x3E and RTL87x3D.
    *          It is NOT supported in the RTL87x3EP.
    * @param   polling_tx_func  One wire uart polling tx data function.
    * @param   dma_send_func One wire uart async DMA send data function.
    */
void log_enable_one_wire(LOG_1W_UART_POLLING_FUNC polling_tx_func,
                         LOG_1W_UART_SEND_FUNC dma_send_func);
#ifdef __cplusplus
}
#endif
/** @} */ /* End of group HAL_87x3e_LOG_CONTROL_EXPORTED_FUNCTIONS */
/** @} */ /* End of group HAL_87x3E_LOG_API */
#endif

