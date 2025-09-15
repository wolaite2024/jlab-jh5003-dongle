/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    crash_handle_ext.h
* @brief   This file provides crash handle wrapper for sdk customer.
* @author
* @date
* @version v1.0
* *************************************************************************************
*/
#ifndef __CRASH_HANDLE_EXT_H__
#define __CRASH_HANDLE_EXT_H__
#include "stdint.h"

/** @defgroup  HAL_87x3E_CRASH_HANDLE    Crash Handle
    * @brief Crash handle.
    * @{
    */
/** @defgroup 87x3E_CRASH_HANDLE_EXPORTED_FUNCTIONS Crash Handle Exported Functions
    * @brief
    * @{
    */

#ifdef __cplusplus
extern "C" {
#endif
/**
    * @brief  Set the crash dump flash space.
    * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @warning This API is supported in RTL87x3E and RTL87x3D.
    *          It is NOT supported in RTL87x3G.
    * @param crash_dump_flash_addr  Specify the flash address to save the crash dump data.
    * @param crash_dump_flash_size  Specify the flash size to save the crash dump data.
    * @return 0.
    */
int32_t hal_crash_dump_set(uint32_t crash_dump_flash_addr, uint32_t crash_dump_flash_size);

#ifdef __cplusplus
}
#endif
/** @} */ /* End of group 87x3E_CRASH_HANDLE_EXPORTED_FUNCTIONS */
/** @} */ /* End of group HAL_87x3E_CRASH_HANDLE */
#endif //__CRASH_HANDLE_EXT_H__
