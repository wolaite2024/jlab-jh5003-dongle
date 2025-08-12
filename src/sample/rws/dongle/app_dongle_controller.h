/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef __APP_DONGLE_CONTROLLER_H__
#define __APP_DONGLE_CONTROLLER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 *
 * \brief  check hid data transmit to 2.4g or not
 * @param  void
 * @return transmit status
 *
 */
bool app_dongle_controller_is_transmitting(void);

/**
 *
 * \brief  save voice data to ring buf
 * @param  data: data to save
 * @param  size: data size
 * @return true if save success
 *
 */
bool app_dongle_controller_save_voice_data(uint8_t *data, uint16_t size);

/**
 *
 * \brief  send hid data from buf to 2.4g
 * @param  void
 *
 */
void app_dongle_controller_send_hid_buf_data(void);

/**
 *
 * \brief  send flushable hid data to dongle
 * @param  data: data to send
 * @param  size: data size
 * @param  flushable: data can be flushable or not
 * @return true if send success
 *
 */
bool app_dongle_controller_hid_data(uint8_t *data, uint8_t size, bool flushable);

/**
 *
 * \brief  init controller setting
 *
 */
void app_dongle_controller_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_EQ_H_ */

