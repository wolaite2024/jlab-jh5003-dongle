/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_USB_AUDIO_H_
#define _APP_USB_AUDIO_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_USB_AUDIO App Usb Audio
  * @brief modulization for usb plug or unplug module for simple application usage.
  * @{
  */
/*============================================================================*
  *                                Functions
  *============================================================================*/
/** @defgroup APP_USB_AUDIO_Exported_Functions usb audio Exported Functions
    * @{
    */

///**
//    * @brief  App check usb audio plug. And send io message to app for follow-up processing.
//    * @param  audio_path indicate usb audio decode or encode path
//    * @param  bit_res usb audio DA/AD sample depth
//    * @param  sf usb audio DA/AD sampling frequency
//    * @param  chann_mode usb audio channel mode, mono or stereo.
//    * @return void
//    */
//bool app_usb_audio_plug(uint8_t audio_path, uint8_t bit_res, uint8_t sf, uint8_t chann_mode);

///**
//    * @brief  App check usb audio unplug. And send io message to app for follow-up processing.
//    * @param  audio_path indicate usb audio decode or encode path
//    * @return void
//    */
//bool app_usb_audio_unplug(uint8_t audio_path);

///**
//    * @brief  usb audio device plug handle. Use handle usb audio message, according to message,
//              make usb audio work in audio AD or audio DA or USB_BC12 mode.
//    * @param io_driver_msg_recv indicate usb audio messge.
//    * @return void
//    */
//void app_usb_audio_plug_handle_msg(T_IO_MSG *io_driver_msg_recv);

///**
//    * @brief  usb audio device unplug handle.  Make usb audio reset and stop.
//    * @param  io_driver_msg_recv indicate usb audio messge.
//    * @return void
//    */
//void app_usb_audio_unplug_handle_msg(T_IO_MSG *io_driver_msg_recv);

///**
//    * @brief  usb audio device check handle. Start USB Audio downstreaming.
//    * @param  io_driver_msg_recv indicate usb audio messge.
//    * @return void
//    */
//void app_usb_audio_check_handle_msg(T_IO_MSG *io_driver_msg_recv);

//void app_usb_audio_data_trans_handle_msg(T_IO_MSG *io_driver_msg_recv);

void app_usb_audio_music_create(T_AUDIO_FORMAT_INFO cfg_snk_info);

void app_usb_audio_music_destroy(void);

void app_usb_audio_msbc_ds_create(void);

void app_usb_audio_msbc_ds_destroy(void);

void app_usb_audio_msbc_us_create(void);

void app_usb_audio_msbc_us_destroy(void);

bool app_usb_audio_is_us_streaming(void);

bool app_usb_audio_is_ds_streaming(void);

bool app_usb_connected(void);

void app_usb_handle_msg(T_IO_MSG *io_driver_msg_recv);

/**
    * @brief  usb start, and register callback function.
    * @return void
    */
void app_usb_audio_init(void);

void app_usb_audio_start(void);

/**
    * @brief  usb audio deinit, and register callback function.
    * @return void
    */
void app_usb_audio_deinit(void);

/** @} */ /* End of group APP_USB_AUDIO_Exported_Functions */

/** @} */ /* End of group APP_USB_AUDIO */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_USB_AUDIO_H_ */
