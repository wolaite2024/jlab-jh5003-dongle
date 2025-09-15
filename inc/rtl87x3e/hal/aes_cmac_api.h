/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     aes_cmac_api.c
* @brief    This file provides aes cmac API wrapper for sdk customer.
* @details
* @author   jane_zhang
* @date
* @version  v1.0
*********************************************************************************************************
*/
/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __AES_CMAC_API_H_
#define __AES_CMAC_API_H_

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdbool.h>
#include <stdint.h>

/** @defgroup  HAL_87x3E_AES_CMAC    AES CMAC
    * @brief AES CMAC.
    * @{
    */
/*============================================================================*
 *                              Variables
*============================================================================*/
/** @defgroup HAL_87x3E_AES_CMAC_EXPORTED_MACROS AES CMAC Exported Macros
    * @brief
    * @{
    */
#define AES_CMAC128_LSB_MSG_LEN_MAX (255)

/** End of HAL_87x3E_AES_CMAC_EXPORTED_MACROS
    * @}
    */
/*============================================================================*
 *                              Functions
*============================================================================*/

/** @defgroup HAL_87x3E_AES_CMAC_EXPORTED_FUNCTIONS AES CMAC Exported Functions
    * @brief
    * @{
    */
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Calculate the AES CMAC for the input data (MSB) with 128-bit key.
 *
 * @param[in] key    Specify the 128-bit AES CMAC key (MSB).
 * @param[in] input  Specify the input data buffer (MSB).
 * @param[in] length Specify the input data length.
 * @param[out] mac   Specify the data buffer to store the CMAC result.
 *
 * @return Whether the AES CMAC calculation is successful.
 * @retval true      Success.
 * @retval false     Fail.
 */
bool aes_cmac128_msb(unsigned char *key, unsigned char *input, int length,
                     unsigned char *mac);

/**
 * @brief Calculate the AES CMAC for the input data (LSB) with 128-bit key.
 *
 * @param[in] key     Specify the 128-bit AES CMAC key (LSB).
 * @param[in] msg     Specify the input data buffer (LSB).
 * @param[in] msg_len Specify the input data length.It should be less than AES_CMAC128_LSB_MSG_LEN_MAX.
 * @param[out] mac    Specify the data buffer to store the CMAC result.
 *
 * @return Whether the AES CMAC calculation is successful.
 * @retval true      Success.
 * @retval false     Fail.
 */
bool aes_cmac128_lsb(unsigned char key[16], unsigned char *msg,
                     int msg_len, unsigned char mac[16]);

#ifdef __cplusplus
}
#endif
/** @} */ /* End of group HAL_87x3E_AES_CMAC_EXPORTED_FUNCTIONS */
/** @} */ /* End of group HAL_87x3E_AES_CMAC */
#endif
