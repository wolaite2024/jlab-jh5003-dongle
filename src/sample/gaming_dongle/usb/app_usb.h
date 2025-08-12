#include "stdbool.h"
#include "stdint.h"
#include "app_msg.h"
#include "usb_dm.h"
#include "usb_msg.h"
#ifndef APP_USB_H
#define APP_USB_H

#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

/**
 * usb_dm.h
 *
 * \brief   USB GIP enum done callback
 *
 * \ingroup  USB_CORE
 */
typedef void (*GIP_CB_ENUM_DONE)(void);

/**
 * usb_dm.h
 *
 * \brief   USB GIP start enum callback
 *
 * \ingroup  USB_CORE
 */
typedef void (*GIP_CB_START_ENUM)(void);

/**
 * usb_dm.h
 *
 * \brief   USB GIP suspend callback
 *
 * \ingroup  USB_CORE
 */
typedef void (*GIP_CB_SUSPEND)(void);

/**
 * app_usb.h
 *
 * \brief   app usb init
 *
 * \ingroup APP_USB
 */
void app_usb_init(void);

/**
 * app_usb.h
 *
 * \brief   handle USB-related message.
 *
 * \ingroup APP_USB
 */
void app_usb_msg_handle(T_IO_MSG *msg);

/**
 * app_usb.h
 *
 * \brief   register USB GIP enum done callback
 *
 * \ingroup  APP_USB
 */
void app_usb_enum_done_register(GIP_CB_ENUM_DONE cb);

/**
 * app_usb.h
 *
 * \brief   register USB GIP start enum callback
 *
 * \ingroup  APP_USB
 */
void app_usb_start_enum_register(GIP_CB_START_ENUM cb);

/**
 * app_usb.h
 *
 * \brief   gip callback to modify gip dev and string descs
 *
 * \ingroup  APP_USB
 */
void app_usb_gip_cfg_desc(uint16_t idProduct, uint16_t idVendor, char *serialNum);

/**
 * @brief usb state currently
 *
 * @return T_USB_POWER_STATE
 */
T_USB_POWER_STATE app_usb_power_state(void);

/**
 * @brief handle suspend event
 *
 */
void app_usb_dm_evt_suspend_handle(void);

/**
 * @brief start USB
 *
 */
int app_usb_start(void);

/**
 * @brief set GIP xbox mode flag
 *
 */
void app_usb_set_gip_flag(uint8_t value);

/**
 * app_usb.h
 *
 * \brief   whether device is suspend or resume
 *
 * \return   true: suspend
 * \ingroup   APP_USB
 */
bool app_usb_is_suspend(void);
void app_usb_suspend_register(GIP_CB_SUSPEND cb);

/**
 * @brief usb notify app task and other mode
 *
 */
bool app_usb_other_trigger_evt(T_USB_OTHER_MODE_MSG evt,  uint32_t param);

#endif
