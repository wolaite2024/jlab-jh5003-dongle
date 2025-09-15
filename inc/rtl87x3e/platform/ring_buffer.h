#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

/*============================================================================*
 *                               Header Files
 *============================================================================*/

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup 87x3e_RING_BUFFER Ring Buffer
  * @brief API for user application to use ring buffer.
  * @{
  */

/*============================================================================*
 *                                   Types
 *============================================================================*/
/** @defgroup 87x3e_RING_BUFFER_Exported_Types Ring Buffer Exported Types
  * @{
  */

/**
 * @brief Data structure to control the ring buffer.
 */
typedef struct
{
    uint8_t    *buf;  //!< Memory buffer to store data.
    uint32_t
    head;  //!< Indicates the distance between the current read pointer and the buffer starting address
    uint32_t
    tail;  //!< Indicates the distance between the current write pointer and the buffer starting address
    uint32_t   size;  //!< Size of memory buffer.
} T_RING_BUFFER;

/** End of 87x3e_RING_BUFFER_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup 87x3e_RING_BUFFER_Exported_Functions Ring Buffer Exported Functions
 * @{
 */

/**
 * @brief      Initialize the ring buffer.
 * @note       This API will use the ring buffer to manage an existing memory.
 * @param[in]  rb    Pointer to ring buffer structure.
 * @param[in]  buf   An existing memory buffer.
 * @param[in]  size  Size of the memory buffer.
 * @return     True if initialization succeeds, false otherwise.
 */
bool ring_buffer_init(T_RING_BUFFER *rb, void *buf, uint32_t size);

/**
 * @brief      Write data to ring buffer.
 * @note       This API supports partial write, it means that if the size of the
 *             data written is larger than the remaining space of the ring
 *             buffer, it will write the data of the remaining space size.
 * @param[in]  rb    Pointer to ring buffer structure.
 * @param[in]  data  Data written to ring buffer.
 * @param[in]  len   Length of data to be written.
 * @return     The length of data that is successfully written to ring buffer.
 */
uint32_t ring_buffer_write(T_RING_BUFFER *rb, const uint8_t *data, uint32_t len);

/**
 * @brief      Read data from ring buffer.
 * @note       This API supports partial read, it means that if data to be read
 *             from ring buffer is longer than the data in ring buffer, it will
 *             read all data in the ring buffer and return the corresponding data length.
 *             After reading the data, the data will be removed from the ring buffer.
 * @param[in]  rb    Pointer to ring buffer structure.
 * @param[in]  len   Length of data to read.
 * @param[out] data  Data that is read from the ring buffer.
 * @return     The length of data that is successfully read from ring buffer.
 */
uint32_t ring_buffer_read(T_RING_BUFFER *rb, uint32_t len, uint8_t *data);

/**
 * @brief      Get the remaining space in the ring buffer.
 * @param[in]  rb    Pointer to ring buffer structure.
 * @return     The remaining space in the ring buffer.
 */
uint32_t ring_buffer_get_remaining_space(T_RING_BUFFER *rb);

/**
 * @brief      Peek data from ring buffer.
 * @note       This API supports partial peek, it means that if data to be peeked
 *             from ring buffer is longer than the data in ring buffer, it will
 *             peek all data in the ring buffer and return the corresponding data length.
 *             Unlike reading data, peeking data does not remove data from ring buffer.
 * @param[in]  rb    Pointer to ring buffer structure.
 * @param[in]  len   Length of data to peek.
 * @param[out] data  Data that is peeked from the ring buffer.
 * @return     The length of data that is successfully peeked from ring buffer.
 */
uint32_t ring_buffer_peek(T_RING_BUFFER *rb, uint32_t len, uint8_t *data);

/**
 * @brief      Remove data from ring buffer.
 * @note       This API supports partial remove, it means that if data to be removed
 *             from ring buffer is longer than the data in ring buffer, it will
 *             remove all data in the ring buffer and return the corresponding data length.
 * @param[in]  rb    Pointer to ring buffer structure.
 * @param[in]  len   Length of data to be removed.
 * @return     The length of data that is successfully removed from ring buffer.
 */
uint32_t ring_buffer_remove(T_RING_BUFFER *rb, uint32_t len);

/**
 * @brief      Get the length of data in the ring buffer.
 * @param[in]  rb    Pointer to ring buffer structure.
 * @return     The length of data in ring buffer.
 */
uint32_t ring_buffer_get_data_count(T_RING_BUFFER *rb);

/**
 * @brief      Clear data in the ring buffer.
 * @param[in]  rb    Pointer to ring buffer structure.
 */
void ring_buffer_clear(T_RING_BUFFER *rb);

/**
 * @brief      Deinitialize the ring buffer.
 * @note       This API will deinitialize the ring buffer, but the memory buffer
 *             needs to be freed manually.
 * @param[in]  rb    Pointer to ring buffer structure.
 */
void ring_buffer_deinit(T_RING_BUFFER *rb);

/** @} */ /* End of group 87x3e_RING_BUFFER_Exported_Functions */

/** @} */ /* End of group 87x3e_RING_BUFFER */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _RING_BUFFER_H_ */


