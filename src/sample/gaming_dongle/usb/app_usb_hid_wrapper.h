#ifndef _APP_USB_HID_WRAPPER_H_
#define _APP_USB_HID_WRAPPER_H_
#include <stdint.h>
#include <stdbool.h>

/** @defgroup USB_HID_WRAPPER USB Hid wrapper
  * @brief USB hid data pipe usage
  * @{
  */


/**
 * app_usb_hid_wrapper.h
 *
 * \brief   hid pipe send data
 *
 * \details data is sent serially, which means data is not sent actually until previous data transmission is complete.
 *
 * \param[in]  buf data will be sent
 * \param[in]  len length of data
 *
 * \return true data will be sent now or after all previous transmissions are complete, false will never be sent
 *
 * \ingroup USB_HID_WRAPPER
 */
bool app_usb_hid_interrupt_pipe_send(void *data, uint16_t length);

/**
 * app_usb_hid_wrapper.h
 *
 * \brief   usb hid init wrapper
 *
 * \ingroup USB_HID_WRAPPER
 */
void app_usb_hid_wrapper_init(void);

/** @}*/
/** End of USB_HID_WRAPPER
*/

#endif
