#ifndef _CRC8_H_
#define _CRC8_H_

/** @defgroup  HAL_CRC8    CRC8
    * @brief  This file introduces the CRC8 APIs.
    * @{
    */

/** @defgroup HAL_CRC8_Exported_Functions CRC8 Exported Functions
    * @brief
    * @{
    */

#include <stdint.h>

/**
    * @brief  Calculate crc8.
    * @note   Poly 0x7(x^8 + x^2 + x + 1).
    * @param  crc The checksum to pack.
    * @param  data The pointer of input data.
    * @param  data_len The size of data.
    * @return The 8 bits CRC value of given data.
    */
uint8_t crc8_calu(uint8_t crc, const void *data, uint32_t data_len);

/** @} */ /* End of group HAL_CRC8_Exported_Functions */
/** @} */ /* End of group HAL_CRC8 */


#endif
