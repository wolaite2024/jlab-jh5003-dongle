#ifndef _STDLIB_CORECRT_H
#define _STDLIB_CORECRT_H
#include <stdlib.h>
/** @defgroup  HAL_87x3e_STDLIB_CORECRT    Stdlib Corecrt
    * @brief Some C standard library corecrt functions.
    * @{
    */

/** @defgroup HAL_87x3e_Stdlib_Corecrt_Exported_Functions Stdlib Corecrt Exported Functions
    * @brief
    * @{
    */
#ifdef __cplusplus
extern "C" {
#endif
/**
    * @brief  Security check mechanism.
    * @param  dest Destination buffer pointer.
    * @param  destsz The size of destination buffer.
    * @param  src   Source buffer pointer.
    * @param  count Source data size to memcpy.
    * @return The error code.
    * @retval 0      Success.
    * @retval others   Error code.
    */
extern int memcpy_s(void *dest, size_t destsz, const void *src, size_t count);
/**
    * @brief  Security check mechanism.
    * @param  dest Destination buffer pointer.
    * @param  destsz The size of destination buffer.
    * @param  ch   Memory set value.
    * @param  count Data size to memset with ch.
    * @return The error code.
    * @retval 0      Success.
    * @retval others   Error code.
    */
int memset_s(void *dest, size_t destsz, int ch, size_t count);
#ifdef __cplusplus
}
#endif
/** @} */ /* End of group HAL_87x3e_Stdlib_Corecrt_Exported_Functions */
/** @} */ /* End of group HAL_87x3e_STDLIB_CORECRT */
#endif /* stdlib_corecrt.h */
