/**
****************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
****************************************************************************************************
* @file      ftl.h
* @brief     Flash transport layer is used as an abstraction layer for user applications to save read/write
*            parameters in flash.
* @note      FTL is a dedicated block in flash, used for saving data. If the value is only for one-time read,
*            refer to fmc APIs.
* @author    Brenda_li
* @date      2016-12-27
* @version   v1.0
* **************************************************************************************************
*/

#ifndef _FTL_H_
#define _FTL_H_

#include <stdint.h>
#include <stdbool.h>
#include "errno.h"
#include "storage.h"

#ifdef  __cplusplus
extern  "C" {
#endif  // __cplusplus


/** @defgroup  HAL_FTL    Flash Transport Layer
    * @brief Simple implementation of file system for flash.
    * @{
    */

/*============================================================================*
  *                                   Macros
  *============================================================================*/
/** @defgroup FTL_Exported_Macros Flash Transport Layer Exported Macros
    * @brief
    * @{
    */

#define FTL_POOL_ENABLE 1    /**< Indicate that the current version supports FTL POOL, to facilitate compatibility handling for users. */


/** End of FTL_Exported_Macros
    * @}
    */
/*============================================================================*
  *                                   Types
  *============================================================================*/


/*============================================================================*
  *                                Functions
  *============================================================================*/
/** @defgroup FTL_Exported_Functions Flash Transport Layer Exported Functions
    * @brief
    * @{
    */
/**
    * @brief    Save specified value to specified FTL offset.
    * @param    pdata  Specify data buffer.
    * @param    offset Specify FTL offset to store.
    * @param    size   Size to store.
    * @return   Refer to errno.h.
    * @note     FTL offset is pre-defined and no conflict with ROM.
    */
extern uint32_t(*ftl_save_to_storage)(void *pdata, uint16_t offset, uint16_t size);

/**
    * @brief    Load specified FTL offset parameter to specified buffer.
    * @param    pdata  Specify data buffer.
    * @param    offset Specify FTL offset to load.
    * @param    size   Size to load.
    * @return   Refer to errno.h.
    * @note     FTL offset is pre-defined and no conflict with ROM.
    */
extern uint32_t(*ftl_load_from_storage)(void *pdata, uint16_t offset, uint16_t size);

/**
    * @brief    Init FTL.
    * @param    info  FTL partition info.
    * @return   Refer to errno.h.
    */
int ftl_init(const T_STORAGE_PARTITION_INFO *info);

/**
    * @brief    Init an ext FTL module.
    * @param    module_name  Specify FTL module name, and the first 4 bytes must be unique.
    * @param    malloc_size  FTL module logical size.
    * @param    block_len    The minimum access unit for the FTL module must be an integral multiple of 4 and cannot exceed 128.
    * @return   Refer to errno.h.
    * @note     Up to 6 modules can be applied.
    */
int32_t ftl_init_module(char *module_name, uint16_t malloc_size, uint8_t block_len);

/**
    * @brief    Set V1 FTL information for the FTL upgrade.
    * @param    module_name  V1 ext FTL name.
    * @param    u32PageStartAddr  V1 FTL ext start address.
    * @param    pagenum  V1 ext FTL pagenum.
    * @param    value_size  V1 ext FTL value size.
    * @param    offset_min  V1 ext FTL min offset.
    */
void ftl_v1_module_info(char *module_name, uint32_t u32PageStartAddr, uint8_t pagenum,
                        uint32_t value_size, uint16_t offset_min);

/**
    * @brief    Save specified value to FTL module.
    * @param    module_name  Specify FTL module name.
    * @param    pdata  Specify data buffer.
    * @param    offset  FTL offset to store.
    * @param    size   Size to store, needs to be an integer multiple of block_len in ftl_init_module.
    * @return   Refer to errno.h.
    */
int32_t ftl_save_to_module(char *module_name, void *pdata, uint16_t offset, uint16_t size);

/**
    * @brief    Load specified FTL module offset parameter to specified buffer.
    * @param    module_name  Specify FTL module name.
    * @param    pdata  Specify data buffer.
    * @param    offset  Specify FTL offset to load, needs to be an integer multiple of block_len in ftl_init_module.
    * @param    size   Size to load.
    * @return   Refer to errno.h.
    */
int32_t ftl_load_from_module(char *module_name, void *pdata, uint16_t offset, uint16_t size);

/**
    * @brief    Init FTL cache and set FTL cache enable.
    * @param    cache_size  FTL cache size.
    * @return   True if init is successful, otherwise false.
    */
bool ftl_cache_init(uint16_t cache_size);

/**
    * @brief    Set ROM FTL module size.
    * @param    rom_size  FTL module logic size.
    * @return   Refer to errno.h.
    */
int32_t ftl_set_rom_module_size(uint16_t rom_size);

/** @} */ /* End of group FTL_Exported_Functions */

/** @} */ /* End of group HAL_FTL */

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // _FTL_H_



