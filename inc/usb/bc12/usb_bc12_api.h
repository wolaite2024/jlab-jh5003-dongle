#ifndef __USB_BC12_H__
#define __USB_BC12_H__
#include <stdint.h>

/** @defgroup 87x3e_USB_BC12 USB bc1.2
  * @brief provide bc12 api to external charger
  * @{
  */

/**
 * usb_bc12.h
 *
 * \brief   definition of usb bc12 type
 *
 * \ingroup 87x3e_USB_BC12
 */
typedef enum
{
    BC12_TYPE_SDP_0P5A = 0,               /**< BC12 type is SDP, the maximum charger current is 0.5A. */
    BC12_TYPE_DCP_1P5A,                   /**< BC12 type is DCP, the maximum charger current is 1.5A. */
    BC12_TYPE_CDP_1P5A,                   /**< BC12 type is CDP, the maximum charger current is 1.5A. */
    BC12_TYPE_OTHERS_USER_DEFINED_0P5A,   /**< BC12 type is others, the maximum charger current is user-defined. */
    BC12_TYPE_ADP_ERROR = 0xFF,           /**< error value. */
} T_USB_BC12_TYPE;

/**
 * usb_bc12.h
 *
 * \brief   get USB bc12 type
 *
 *\details the bc12 type is defined in \ref T_USB_BC12_TYPE.
           If the type is BC12_TYPE_OTHERS_USER_DEFINED_0P5A, max charger current is user-defined,
           and the proposed value is less-than-0.5A for safety.
           If the type is BC12_TYPE_ADP_ERROR, please DO NOT charge the battery, and call
           \ref usb_bc12_type_get to get the bc12 type once again.
 *
 *\return  bc12 type defined in \ref T_USB_BC12_TYPE

 * \ingroup  87x3e_USB_BC12
 */
uint8_t usb_bc12_type_get(void);

/**
 * usb_bc12.h
 *
 * \brief   power down bc12
 *
 *\details If the type is DCP, the power will be keeping(BC1.2 spec chapter4.4.1). When unplug usb, it need to power down.
 *
 * \ingroup  87x3e_USB_BC12
 */
void usb_bc12_power_down(void);

/** @}*/
/** End of 87x3e_USB_BC12
*/
#endif
