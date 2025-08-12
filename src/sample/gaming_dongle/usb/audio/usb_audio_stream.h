/**
 * @copyright Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 * @file usb_audio_stream.h
 * @version 1.0
 *
 */

#ifndef __USB_AUDIO_STREAM_H__
#define __USB_AUDIO_STREAM_H__
#include "audio_type.h"

/**
 * @defgroup UAS
 * @brief module for universal usb audio process
 * @{
 *
 * |Terms         |Details                                               |
 * |--------------|------------------------------------------------------|
 * |\b USB        |Universal Serial Bus.                                 |
 * |\b UAS        |USB Audio Streaming                                   |
 *
 **/

/**
 * usb_audio_stream.h
 *
 * @brief   USB Audio stream control command
 *
 * @ingroup UAS
 */
/** @brief  The format type of USB Audio Stream.*/
#define T_STREAM_ATTR   T_AUDIO_PCM_ATTR
/** @brief  The state of start/stop cb in @ref T_UAS_FUNC is synchronous.*/
#define SYNC    true
/** @brief  The state of start/stop cb in @ref T_UAS_FUNC is asynchronous.*/
#define ASYNC   false
/** @brief  The label of USB Audio Stream.*/
typedef enum
{
    USB_AUDIO_STREAM_LABEL_1,
    USB_AUDIO_STREAM_LABEL_2,
    USB_AUDIO_STREAM_NUM,
} T_UAC_LABEL;

/**
 * usb_audio_stream.h
 *
 * @brief   USB Audio stream control command
 *
 * @ingroup UAS
 */
typedef enum
{
    CTRL_CMD_VOL_CHG,       /**<  A volume change control command. */
    CTRL_CMD_MUTE,          /**<  A mute set control command. */
    CTRL_CMD_STOP,          /**<  A usb stop control command. */
} T_CTRL_CMD;

/**
 * usb_audio_stream.h
 *
 * @brief   USB Audio stream direction
 *
 * @ingroup UAS
 */
typedef enum
{
    USB_AUDIO_STREAM_TYPE_OUT,      /**<  PC direction is OUT. */
    USB_AUDIO_STREAM_TYPE_IN,       /**<  PC direction is IN. */
    USB_AUDIO_STREAM_TYPE_MAX,
} T_USB_AUDIO_STREAM_TYPE;

/**
 * usb_audio_stream.h
 *
 * @brief   USB Audio stream volume
 *
 * @ingroup UAS
 */
typedef struct _uas_vol
{
    uint32_t range: 16;         /**<  The range of the volume. */
    uint32_t cur: 16;           /**<  The current volume value. */
} T_UAS_VOL;

/**
 * usb_audio_stream.h
 *
 * @brief   USB Audio stream control ,vol/mute .etc
 *
 * @ingroup UAS
 */
typedef struct _uas_ctrl
{
    T_UAS_VOL vol;          /**<  The volume control of USB Audio stream. */
    bool mute;              /**<  The mute control of USB Audio stream. */
} T_UAS_CTRL;

#define RTP_PLC_PCM_THRESHOLD   384 // 48k/16bit/4ms/mono -> 48*2*4*1

uint16_t usb_audio_get_us_pcm_buf_data_size(void);

/**
 * usb_audio_stream.h
 *
 * @brief   bind user application layer cbs to usb audio stream
 *
 * @param[in] stream_type @ref T_USB_AUDIO_STREAM_TYPE
 * @param[in] label the pipe of @ref T_USB_AUDIO_STREAM
 *
 * @return user application id
 * @par Example
 * Please refer to "Initialize the USB Audio stream" in @ref UAS_Usage_Chapter.
 * @ingroup UAS
 */
uint32_t usb_audio_stream_ual_bind(uint8_t stream_type, uint8_t label);

/**
 * usb_audio_stream.h
 *
 * @brief   usb audio stream remaining data length
 *
 * @param[in] id return value of @ref usb_audio_stream_ual_bind
 * @par Example
 * Please refer to "Initialize the USB Audio stream" in @ref UAS_Usage_Chapter.
 *
 * @ingroup UAS
 */
uint32_t usb_audio_stream_get_data_len(uint32_t id);

/**
 * usb_audio_stream.h
 *
 * @brief   peek usb audio stream data, note the data is not removed from usb audio stream buffer until @ref usb_audio_stream_data_flush
 *          is called
 *
 * @param[in] id return value of @ref usb_audio_stream_ual_bind
 * @param[in] buf data buffer
 * @param[in] len length of data to peek
 *
 * @return actual length of peeked data
 *
 * @par Example
 * Please refer to "Initialize the USB Audio stream" in @ref UAS_Usage_Chapter.
 *
 * @ingroup UAS
 */
uint32_t usb_audio_stream_data_peek(uint32_t id, void *buf, uint32_t len);

/**
 * usb_audio_stream.h
 *
 * @brief   remove data from usb audio stream buffer
 *
 * @param[in] id return value of @ref usb_audio_stream_ual_bind
 * @param[in] len length of data to remove
 *
 * @return actual length of removed data
 * @par Example
 * Please refer to "Initialize the USB Audio stream" in @ref UAS_Usage_Chapter.
 *
 * @ingroup UAS
 */
uint32_t usb_audio_stream_data_flush(uint32_t id, uint32_t len);

/**
 * usb_audio_stream.h
 *
 * @brief   write usb audio stream data
 *          is called
 *
 * @param[in] id return value of @ref usb_audio_stream_ual_bind
 * @param[in] buf data buffer
 * @param[in] len length of data to write
 *
 * @return actual length of written data
 * @par Example
 * Sample code:
 * @code
 *      void mic_read()
 *      {
 *          if (usb_audio_stream_get_remaining_pool_size(id) < required_len)
 *          {
 *              usb_audio_stream_data_flush(id, required_len);
 *          }
 *          usb_audio_stream_data_write(id, buf, required_len);
 *      }
 * @endcode
 *
 * @ingroup UAS
 */
uint32_t usb_audio_stream_data_write(uint32_t id, void *buf, uint32_t len);

/**
 * usb_audio_stream.h
 *
 * @brief   read usb audio stream data, note the data will be removed from usb audio stream buffer after read
 *
 * @param[in] id return value of @ref usb_audio_stream_ual_bind
 * @param[in] buf data buffer
 * @param[in] len length of data to read
 *
 * @return actual length of read data
 *
 * @ingroup UAS
 */
uint32_t usb_audio_stream_data_read(uint32_t id, void *buf, uint32_t len);

/**
 * usb_audio_stream.h
 *
 * @brief   get usb audio remaining pool size
 *
 * @param[in] id return value of @ref usb_audio_stream_ual_bind
 *
 * @return remaining pool size
 * @par Example
 * Please refer to @ref usb_audio_stream_data_write.
 *
 * @ingroup UAS
 */
uint32_t usb_audio_stream_get_remaining_pool_size(uint32_t id);

/**
 * usb_audio_stream.h
 *
 * @brief   clear usb audio pool
 *
 * @param[in] id return value of @ref usb_audio_stream_ual_bind
 *
 * @return void
 *
 * @ingroup UAS
 */
void usb_audio_stream_buf_clear(uint32_t id);

/**
 * usb_audio_stream.h
 *
 * @brief   handle usb audio stream event
 *
 * @param[in] evt event
 * @param[in] param optional parameter
 * @par Example
 * Sample code:
 * @code
 *  void app_usb_msg_handle (void)
 *  {
 *      subtype = msg->subtype;
 *      group = USB_MSG_GROUP(subtype);
 *      switch(group)
 *      {
 *      case  USB_EVT:
 *          {
 *              usb_audio_stream_evt_handle();
 *          }
 *          break;
 *      }
 *  }
 * @endcode
 *
 * @ingroup UAS
 */
void usb_audio_stream_evt_handle(uint8_t evt, uint32_t param);

void usb_audio_stream_data_trans_msg(uint32_t label);

/**
 * usb_audio_stream.h
 *
 * @brief   init usb audio stream
 * @par Example
 * Sample code:
 * @code
 * void usb_init (void)
 * {
 *     usb_audio_stream_init();
 * }
 * @endcode
 *
 * @ingroup UAS
 */
void usb_audio_stream_init(void);

/** @}*/
/** End of UAS
*/

#endif
