/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    cache.h
* @brief   This file provides cache API wrapper for sdk customer.
* @author  huan_yang
* @date    2020-11-26
* @version v1.0
* *************************************************************************************
*/
#ifndef __CACHE_H
#define __CACHE_H

#include <stdint.h>


/** @defgroup  HAL_87x3e_CACHE    Cache
    * @brief Cache wrapper.
    * @{
    */
/*============================================================================*
 *                              Variables
*============================================================================*/


/*============================================================================*
 *                              Functions
*============================================================================*/
/** @defgroup HAL_87x3e_CACHE_Exported_Functions Cache Exported Functions
    * @brief
    * @{
    */
#ifdef __cplusplus
extern "C" {
#endif

/**
    * @brief Enable cache.
    * @return void
    */
void cache_enable(void);

/**
    * @brief Disable cache.
    * @return void
    */
void cache_disable(void);

/**
    * @brief Disable cache.
    * @warning This API is supported in RTL87x3E and RTL87x3G.
    *          It is NOT supported in RTL87x3D.
    * @return Refer to errno.h.
    */
void cache_flush_by_addr(uint32_t *addr, uint32_t length);
/**
 * @brief Flash cache init.
 * @param do_cache_flush Not used just for 3in1.
 * @return void
*/
void cache_hit_init(bool do_cache_flush);

/**
 * @brief Get cache hit rate *100.
 *
 * @return Cache hit rate *100.
*/
uint32_t cache_hit_get(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
/** @} */ /* End of group HAL_87x3e_CACHE_Exported_Functions */
/** @} */ /* End of group HAL_87x3e_CACHE */
#endif /* __CACHE_H_ */


