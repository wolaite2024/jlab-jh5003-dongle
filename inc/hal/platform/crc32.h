#ifndef _CRC32_H_
#define _CRC32_H_

/** @defgroup  HAL_CRC32    CRC32
    * @brief This file introduces the CRC32 APIs.
    * @{
    */

/** @defgroup HAL_CRC32_Exported_Functions CRC32 Exported Functions
    * @brief
    * @{
    */
#include <stdint.h>

/**
    * @brief  Calculate crc32.
    * @note   Poly 0xedb88320(x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1).
    * @param  crc The checksum to pack.
    * @param  data The pointer of input data.
    * @param  data_len  The size of input data.
    * @return The 32 bits CRC value of given data.
    */
uint32_t crc32_calu(uint32_t crc, const void *data, uint32_t data_len);



/** @} */ /* End of group HAL_CRC32_Exported_Functions */
/** @} */ /* End of group HAL_CRC32 */
#endif
