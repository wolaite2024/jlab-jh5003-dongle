/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    hmac.h
  * @brief   SHA256HKDF algorithm
  * @author
  * @date    2023-4-27
  * @version v1.0
  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

#ifndef HMAC_H
#define HMAC_H

/** @defgroup  HAL_SHA256HKDF    SHA256HKDF
    * @brief This file introduces SHA256HKDF APIs
    * @{
    */

/** @defgroup HAL_SHA256HKDF_Exported_Functions SHA256HKDF Exported Functions
    * @brief
    * @{
    */

#include <string.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * \brief          Output = Generic_HMAC( hmac key, input buffer ).
 * \xrefitem       Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 * \param md_info  Message digest info.
 * \param key      HMAC secret key.
 * \param keylen   Length of the HMAC key in bytes.
 * \param input    Buffer holding the  data.
 * \param ilen     Length of the input data.
 * \param output   Generic HMAC-result.
 *
 * \returns        0 on success, MBEDTLS_ERR_MD_BAD_INPUT_DATA if parameter
 *                 verification fails.
 */
uint32_t sha256_hkdf(const uint8_t *key, uint32_t key_len, const uint8_t *salt, uint32_t salt_len,
                     const uint8_t *info, uint32_t info_len,
                     uint8_t *out, uint32_t out_len);


#ifdef __cplusplus
}
#endif
/** @} */ /* End of group HAL_SHA256HKDF_Exported_Functions */
/** @} */ /* End of group HAL_SHA256HKDF */

#endif /* hmac.h */
