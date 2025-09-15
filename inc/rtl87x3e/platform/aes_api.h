/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    aes_api.h
  * @brief   aes apis abstracted for encryption related.
  * @details AES encryption APIs which delivers HW/SW implemented reliable and safe AES solution.
  * @author  Abel
  * @date    2017.5.17
  * @version v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */


/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/
#ifndef _AES_API_H_
#define _AES_API_H_

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>

#define AES256 1
#define ECB    1
#define CBC    1
/** @defgroup 87x3E_AES_ECB_API  AES ECB API Sets
  * @brief API sets for AES ECB encryption and decryption implementation.
  * @{
  */

/*============================================================================*
  *                                Functions
  *============================================================================*/
/** @defgroup 87x3E_AES_ECB_API_EXPORTED_FUNCTIONS AES ECB API Sets Exported Functions
    * @{
    */
/**
    * @brief  Swap bytes for the data buffer.
    * @param  src          Source address.
    * @param  dst          Destination address.
    * @param  len          Swap buffer size.
    */

void swap_buf(const uint8_t *src, uint8_t *dst, uint16_t len);

/**
    * @brief  Encrypt the speicified 16 bytes plaintext by AES ECB mode with a 128-bit key.
    * @note   The least significant octet of encrypted data corresponds to encypted[0].
    * @param  plaintext    Specify the 16 bytes plaintext to be encypted.
    * @param  key          Specify the 128-bit key to encrypt the plaintext.
    * @param  encrypted    Specify the output buffer to store the encrypted data.
    * @return Encryption result.
    * @retval true      Success.
    * @retval false     Fail.
    */
bool aes128_ecb_encrypt(uint8_t plaintext[16], const uint8_t key[16], uint8_t *encrypted);

/**
    * @brief  Encrypt the speicified 16 bytes plaintext (MSB) by AES ECB mode with a 128-bit key (MSB).
    * @note   The most significant octet of encrypted data corresponds to encypted[0].
    * @param  plaintext    Specify the 16 bytes plaintext (MSB) to be encypted.
    * @param  key          Specify the 128-bit key (MSB) to encrypt the plaintext.
    * @param  encrypted    Specify the output buffer to store the encrypted data.
    * @return Encryption result.
    * @retval true      Success.
    * @retval false     Fail.
    */
bool aes128_ecb_encrypt_msb2lsb(uint8_t plaintext[16], const uint8_t key[16],
                                uint8_t *encrypted);

/**
    * @brief  Decrypt the speicified 16 bytes encrypted data by AES ECB mode with a 128-bit key.
    * @note   The least significant octet of decrypted data corresponds to output[0].
    * @param  input    Specify the 16 bytes encrypted data to be decypted.
    * @param  key      Specify the 128-bit key to decrypt the data.
    * @param  output   Specify the output buffer to store the decrypted data.
    * @return Decryption result.
    * @retval true      Success.
    * @retval false     Fail.
    */
bool aes128_ecb_decrypt(uint8_t *input, const uint8_t *key, uint8_t *output);

/**
    * @brief  Decrypt the speicified 16 bytes encrypted data (MSB) by AES ECB mode with a 128-bit key (MSB).
    * @note   The most significant octet of decrypted data corresponds to output[0].
    * @param  input    Specify the 16 bytes encrypted data (MSB) to be decypted.
    * @param  key      Specify the 128-bit key (MSB) to decrypt the data.
    * @param  output   Specify the output buffer to store the decrypted data.
    * @return Decryption result.
    * @retval true      Success.
    * @retval false     Fail.
    */
bool aes128_ecb_decrypt_msb2lsb(uint8_t *input, const uint8_t *key, uint8_t *output);

/**
    * @brief  Encrypt the speicified 16 bytes plaintext by AES ECB mode with a 256-bit key.
    * @note   The least significant octet of encrypted data corresponds to encypted[0].
    * @param  plaintext    Specify the 16 bytes plaintext to be encypted.
    * @param  key          Specify the 256-bit key to encrypt the plaintext.
    * @param  encrypted    Specify the output buffer to store the encrypted data.
    * @return Encryption result.
    * @retval true      Success.
    * @retval false     Fail.
    */
bool aes256_ecb_encrypt(uint8_t plaintext[16], const uint8_t key[32], uint8_t *encrypted);

/**
    * @brief  Encrypt the speicified 16 bytes plaintext (MSB) by AES ECB mode with a 256-bit key (MSB).
    * @note   The most significant octet of encrypted data corresponds to encypted[0].
    * @param  plaintext    Specify the 16 bytes plaintext (MSB) to be encypted.
    * @param  key          Specify the 256-bit key (MSB) to encrypt the plaintext.
    * @param  encrypted    Specify the output buffer to store the encrypted data.
    * @return Encryption result.
    * @retval true      Success.
    * @retval false     Fail.
    */
bool aes256_ecb_encrypt_msb2lsb(uint8_t plaintext[16], const uint8_t key[32],
                                uint8_t *encrypted);

/**
    * @brief  Decrypt the speicified 16 bytes encrypted data by AES ECB mode with a 256-bit key.
    * @note   The least significant octet of decrypted data corresponds to output[0].
    * @param  input    Specify the 16 bytes encrypted data to be decypted.
    * @param  key      Specify the 256-bit key to decrypt the data.
    * @param  output   Specify the output buffer to store the decrypted data.
    * @return Decryption result.
    * @retval true      Success.
    * @retval false     Fail.
    */
bool aes256_ecb_decrypt(uint8_t *input, const uint8_t *key, uint8_t *output);

/**
    * @brief  Decrypt the speicified 16 bytes encrypted data (MSB) by AES ECB mode with a 256-bit key (MSB).
    * @note   The most significant octet of decrypted data corresponds to output[0].
    * @param  input    Specify the 16 bytes encrypted data (MSB) to be decypted.
    * @param  key      Specify the 256-bit key (MSB) to decrypt the data.
    * @param  output   Specify the output buffer to store the decrypted data.
    * @return Decryption result.
    * @retval true      Success.
    * @retval false     Fail.
    */
bool aes256_ecb_decrypt_msb2lsb(uint8_t *input, const uint8_t *key, uint8_t *output);

/** @} */ /* End of group 87x3E_AES_ECB_API_EXPORTED_FUNCTIONS */

/** @} */ /* End of group 87x3E_AES_ECB_API */


#endif
