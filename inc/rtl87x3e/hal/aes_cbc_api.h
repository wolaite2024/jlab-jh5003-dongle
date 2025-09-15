/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     aes_cbc_api.c
* @brief    This file provides aes cbc API wrapper for sdk customer.
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

/** @defgroup  HAL_87x3E_AES_CBC    AES CBC
    * @brief AES CBC.
    * @{
    */
/*============================================================================*
 *                              Variables
*============================================================================*/


/*============================================================================*
 *                              Functions
*============================================================================*/
/** @defgroup HAL_87x3E_AES_CBC_EXPORTED_FUNCTIONS AES CBC Exported Functions
    * @brief
    * @{
    */
#ifdef __cplusplus
extern "C" {
#endif
/**
    * @brief  Encrypt the speicified plaintext by AES CBC mode with a 128-bit key.
    * @param  plaintext    Specify the plaintext to be encypted.
    * @param  key          Specify the 128-bit key to encrypt the plaintext.
    * @param  encrypted    Specify the output buffer to store the encrypted data.
    * @param  p_iv         Specify the initialization vector (IV) for AES CBC mode.
    * @param  data_word_len  Specify the word length of the data. The data length must be a multiple of 4.
    * @return Encryption result.
    * @retval true      Success.
    * @retval false     Fail.
    * @note   The least significant octet of encrypted data corresponds to encypted[0].
    */
bool aes128_cbc_encrypt(uint8_t *plaintext, const uint8_t key[16], uint8_t *encrypted,
                        uint32_t *p_iv, uint32_t data_word_len);
/**
    * @brief  Decrypt the speicified data by AES CBC mode with a 128-bit key.
    * @param  input    Specify the encypted data to be decypted.
    * @param  key      Specify the 128-bit key to decrypt the data.
    * @param  output   Specify the output buffer to store the plain data.
    * @param  p_iv     Specify the initialization vector (IV) for AES CBC mode.
    * @param  data_word_len  Specify the word length of the data. The data length must be a multiple of 4.
    * @return Decryption result.
    * @retval true      Success.
    * @retval false     Fail.
    * @note   The least significant octet of decrypted data corresponds to output[0].
    */
bool aes128_cbc_decrypt(uint8_t *input, const uint8_t key[16], uint8_t *output, uint32_t *p_iv,
                        uint32_t data_word_len);
/**
    * @brief  Encrypt the speicified plaintext (MSB) by AES CBC mode with a 128-bit key.
    * @param  plaintext    Specify the plaintext (MSB) to be encypted.
    * @param  key          Specify the 128-bit key to encrypt the plaintext.
    * @param  encrypted    Specify the output buffer to store the encrypted data.
    * @param  p_iv         Specify the initialization vector (IV) for AES CBC mode.
    * @param  data_word_len  Specify the word length of the data. The data length must be a multiple of 4.
    * @return Encryption result.
    * @retval true      Success
    * @retval false     Fail.
    * @note   The most significant octet of encrypted data corresponds to encypted[0].
    */
bool aes128_cbc_encrypt_msb2lsb(uint8_t plaintext[16], const uint8_t key[16], uint8_t *encrypted,
                                uint32_t *p_iv, uint32_t data_word_len);

/**
    * @brief  Decrypt the speicified data (MSB) by AES CBC mode with a 128-bit key.
    * @param  input    Specify the encypted data (MSB) to be decypted.
    * @param  key      Specify the 128-bit key to decrypt the data.
    * @param  output   Specify the output buffer to store the plain data.
    * @param  p_iv     Specify the initialization vector (IV) for AES CBC mode.
    * @param  data_word_len  Specify the word length of the data. The data length must be a multiple of 4.
    * @return Decryption result.
    * @retval true      Success.
    * @retval false     Fail.
    * @note   The most significant octet of encrypted data corresponds to output[0].
    */
bool aes128_cbc_decrypt_msb2lsb(uint8_t *input, const uint8_t key[16], uint8_t *output,
                                uint32_t *p_iv, uint32_t data_word_len);

/**
    * @brief  Encrypt the speicified plaintext by AES CBC mode with a 256-bit key.
    * @param  plaintext    Specify the plaintext to be encypted.
    * @param  key          Specify the 256-bit key to encrypt the plaintext.
    * @param  encrypted    Specify the output buffer to store the encrypted data.
    * @param  p_iv         Specify the initialization vector (IV) for AES CBC mode.
    * @param  data_word_len  Specify the word length of the data. The data length must be a multiple of 4.
    * @return Encryption result.
    * @retval true      Success.
    * @retval false     Fail.
    * @note   The least significant octet of encrypted data corresponds to encypted[0].
    */
bool aes256_cbc_encrypt(uint8_t *plaintext, const uint8_t key[32], uint8_t *encrypted,
                        uint32_t *p_iv, uint32_t data_word_len);

/**
    * @brief  Decrypt the speicified data by AES CBC mode with a 256-bit key.
    * @param  input    Specify the encypted data to be decypted.
    * @param  key      Specify the 256-bit key to decrypt the data.
    * @param  output   Specify the output buffer to store the plain data.
    * @param  p_iv     Specify the initialization vector (IV) for AES CBC mode.
    * @param  data_word_len  Specify the word length of the data. The data length must be a multiple of 4.
    * @return Decryption result.
    * @retval true      Success.
    * @retval false     Fail.
    * @note   The least significant octet of decrypted data corresponds to output[0].
    */
bool aes256_cbc_decrypt(uint8_t *input, const uint8_t key[32], uint8_t *output, uint32_t *p_iv,
                        uint32_t data_word_len);
/**
    * @brief  it's called when the decryption for AES CBC mode by DMA with 128-bit key has finished.
    * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
    * @param  parameter Specify the HW AES callback parameter.
    * @return void
    * @note RX DMA channel and TX DMA channel are released in this function.
    */
void aes128_cbc_dma_done(uint32_t parameter);
/**
    * @brief  Decrypt data by AES CBC DMA mode with 128-bit key.
    * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
    * @param  input     Specify the source address of the data to be decypted.
    * @param  output    Specify the output buffer to store the plain data.
    * @param  data_word_len  Specify the word length of the data. The data length must be a multiple of 4.
    * @param  key       Specify the 128-bit key to decrypt the data.
    * @param  p_iv      Specify the initialization vector (IV) for AES CBC mode.
    * @param  cback     Register the callback function when the total AES DMA operation has been finished.
    * @return The AES CBC decryption result by DMA mode.
    * @retval true      Success.
    * @retval false     Fail.
    * @note   The RX DMA channel and TX DMA channel are requested and used by HW AES DMA operation.
    *         The RX DMA channel and TX DMA channel should be released in the HW AES DMA done callback.
    */
bool aes128_cbc_decrypt_by_dma(uint32_t *input, uint32_t *output, uint32_t data_word_len,
                               const uint8_t key[16], uint32_t *p_iv,
                               void (*cback)(void *));
#ifdef __cplusplus
}
#endif  // __cplusplus
/** @} */ /* End of group HAL_87x3E_AES_CBC_EXPORTED_FUNCTIONS */
/** @} */ /* End of group HAL_87x3E_AES_CBC */
#endif //__AES_CBC_API_H_
