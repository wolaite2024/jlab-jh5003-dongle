/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     aes_cbc_api.c
* @brief    This file provides aes cbc api wrapper for sdk customer.
* @details
* @author   jane_zhang
* @date
* @version  v1.0
*********************************************************************************************************
*/
/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __AES_CBC_API_H_
#define __AES_CBC_API_H_

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdbool.h>
#include <stdint.h>
#include "rtl876x_hw_aes.h"

/** @defgroup  HAL_87x3e_AES_CBC    AES CBC
    * @brief aes cbc api
    * @{
    */
/*============================================================================*
 *                              Variables
*============================================================================*/


/*============================================================================*
 *                              Functions
*============================================================================*/
/** @defgroup HAL_87x3e_AES_CBC_Exported_Functions AES CBC Exported Functions
    * @brief
    * @{
    */
#ifdef __cplusplus
extern "C" {
#endif
/**
    * @brief  128 bit AES CBC encryption on speicified plaintext and keys
    * @param  plaintext    specifed plain text to be encypted
    * @param  key          keys to encrypt the plaintext
    * @param  encrypted    output buffer to store encrypted data
    * @param  p_iv         initialization vector (IV) for CBC mode
    * @param  data_word_len    word length of the data to be descrypted, must be multiples of 4
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    */
bool aes128_cbc_encrypt(uint8_t *plaintext, const uint8_t key[16], uint8_t *encrypted,
                        uint32_t *p_iv, uint32_t data_word_len);
/**
    * @brief  128 bit AES CBC decryption on speicified plaintext and keys
    * @param  input    specifed encypted data to be decypted
    * @param  key          keys to decrypt the data
    * @param  output    output buffer to store plain data
    * @param  p_iv         initialization vector (IV) for CBC mode
    * @param  data_word_len    word length of the data to be descrypted, must be multiples of 4
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    */
bool aes128_cbc_decrypt(uint8_t *input, const uint8_t key[16], uint8_t *output, uint32_t *p_iv,
                        uint32_t data_word_len);
/**
    * @brief  128 bit AES CBC encryption on speicified plaintext and keys
    * @param  plaintext    specifed plain text to be encypted
    * @param  key          keys to encrypt the plaintext
    * @param  encrypted    output buffer to store encrypted data
    * @param  p_iv         initialization vector (IV) for CBC mode
    * @param  data_word_len    word length of the data to be descrypted, must be multiples of 4
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note   most significant octet of encrypted data corresponds to encypted[0]
    */
bool aes128_cbc_encrypt_msb2lsb(uint8_t plaintext[16], const uint8_t key[16], uint8_t *encrypted,
                                uint32_t *p_iv, uint32_t data_word_len);

/**
    * @brief  128 bit AES CBC decryption on speicified plaintext and keys
    * @param  input    specifed encypted data to be decypted
    * @param  key          keys to decrypt the data
    * @param  output    output buffer to store plain data
    * @param  p_iv         initialization vector (IV) for CBC mode
    * @param  data_word_len    word length of the data to be descrypted, must be multiples of 4
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note   most significant octet of encrypted data corresponds to encypted[0]
    */
bool aes128_cbc_decrypt_msb2lsb(uint8_t *input, const uint8_t key[16], uint8_t *output,
                                uint32_t *p_iv, uint32_t data_word_len);

/**
    * @brief  256 bit AES CBC encryption on speicified plaintext and keys
    * @param  plaintext    specifed plain text to be encypted
    * @param  key          keys to encrypt the plaintext
    * @param  encrypted    output buffer to store encrypted data
    * @param  p_iv         initialization vector (IV) for CBC mode
    * @param  data_word_len    word length of the data to be descrypted, must be multiples of 4
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    */
bool aes256_cbc_encrypt(uint8_t *plaintext, const uint8_t key[32], uint8_t *encrypted,
                        uint32_t *p_iv, uint32_t data_word_len);

/**
    * @brief  256 bit AES CBC decryption on speicified plaintext and keys
    * @param  input    specifed encypted data to be decypted
    * @param  key          keys to decrypt the data
    * @param  output    output buffer to store plain data
    * @param  p_iv         initialization vector (IV) for CBC mode
    * @param  data_word_len    word length of the data to be descrypted, must be multiples of 4
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    */
bool aes256_cbc_decrypt(uint8_t *input, const uint8_t key[32], uint8_t *output, uint32_t *p_iv,
                        uint32_t data_word_len);
/**
    * @brief  it's called when 128 bit AES CBC used on flash data by dma has finished
    * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
    * @param  rx_dma_ch pointer to the rx dma channel variable
    * @param  tx_dma_ch pointer to the tx dma channel variable
    * @return void
    */
void aes128_cbc_flash_dma_done(uint8_t *rx_dma_ch, uint8_t *tx_dma_ch);
/**
    * @brief  128 bit AES CBC decryption by dma on flash data
    * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
    * @param  input    specifed flash address of the data to be decypted
    * @param  output    output buffer to store plain data
    * @param  data_word_len    word length of the data to be decrypted, must be multiples of 4
    * @param  key       keys to decrypt the data
    * @param  p_iv      initialization vector (IV) for CBC mode
    * @param  rx_dma_ch pointer to the rx dma channel variable, rx dma channel will be requested in
    *                   this api, don't call dma channel request before calling this api.
    * @param  tx_dma_ch pointer to the tx dma channel variable, tx dma channel will be requested in
    *                   this api, don't call dma channel request before calling this api.
    * @param  cb      register the callback function when the total aes dma operation has been finished.
    * @return aes dma operation trigger result
    * @retval true      successful
    * @retval false     fail
    */
bool aes128_cbc_decrypt_flash_data_by_dma(uint32_t *input, uint32_t *output, uint32_t data_word_len,
                                          const uint8_t key[16], uint32_t *p_iv,
                                          uint8_t *rx_dma_ch, uint8_t *tx_dma_ch,
                                          AES_DMA_CB cb);

#ifdef __cplusplus
}
#endif  // __cplusplus
/** @} */ /* End of group HAL_87x3e_AES_CBC_Exported_Functions */
/** @} */ /* End of group HAL_87x3e_AES_CBC */
#endif //__AES_CBC_API_H_
