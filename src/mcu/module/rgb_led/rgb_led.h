/*
 * Copyright (c) 2023, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _RGB_LED_H_
#define _RGB_LED_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup MODULE_RGB_LED  RGB LED
  * @brief modulization for spi rgb led module for application usage.
  * @{
  */

typedef void (*P_RGB_LED_CB)();

/**
  * @brief     Initializes the SPI1 led driver for RGB LED.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
  *
  * @param[in] rgb_data_num  rgb data number.
  * @param[in] rgb_callback  user rgb callback.
  *
  * @return  None.
  */
void rgb_led_init(uint32_t rgb_data_num, P_RGB_LED_CB rgb_callback);

/**
  * @brief     Send buffer according to RGB data for RGB LED.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
  *
  * @param[in] rgb_data_num   rgb data number.
  * @param[in] buf   rgb data buffer.
  *
  * @return  None.
  */
void rgb_led_data_send(uint32_t *buf, uint32_t rgb_data_num);

/** @} */ /* End of group MODULE_RGB_LED */

#ifdef __cplusplus
}
#endif


#endif //_RGB_LED_H_
