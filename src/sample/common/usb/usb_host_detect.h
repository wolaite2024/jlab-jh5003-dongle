#ifndef _HID_HOST_DETECT_H_
#define _HID_HOST_DETECT_H_
#include <stdint.h>

/**
 * \brief   Operation system type.
 */
typedef enum
{
    OS_TYPE_UNDEF,
    OS_TYPE_WINDOWS,
    OS_TYPE_IOS,
    OS_TYPE_ANDROID,
    OS_TYPE_PS,      /* The volume control of consumer control may be disturbed by the vendor IN report.*/
    OS_TYPE_SWITCH,
} T_OS_TYPE;

/**
 * \brief   The status is used to indicate whether the HID volume operation has been completed.
 */
typedef enum
{
    DETECT_STATUS_NOT_DETECTED,   /* The HID volume operation has not been deteted yet. */
    DETECT_STATUS_IN_PROGRESS,    /* The HID volume operation is in progress. */
    DETECT_STATUS_COMPLETE,       /* The HID volume operation is complete. */
} T_DETECT_STATUS;

/**
 * \brief   The callback to inform the application of the operation system type.
 */
typedef int (* HOST_DETECT_CB)(T_OS_TYPE);

/**
 * \brief   The callback to execute USB HID pipe.
 */
typedef void (* USB_HID_CB)(void);

/**
 * \brief   HID infomation.
 */
typedef struct _usb_host_detect_hid_info
{
    uint8_t intf_num;          /* Interface number of HID. */
    uint8_t ep_num;            /* endpoint number of HID. */
    USB_HID_CB pipe_open;      /* Open HID pipe. */
    USB_HID_CB volume_up;      /* Volume up by sending HID in report of consumer control. */
    USB_HID_CB volume_down;    /* Volume down by sending HID in report of consumer control. */
    USB_HID_CB volume_release; /* Volume release by sending HID in report of consumer control. */
} T_USB_HOST_DETECT_HID_INFO;
/**
 *
 * \brief   Handle USB host detect event.
 *
 * \param evt USB host detect event.
 * \param param Optional parameter.
 * \par Example
 * \code
 *  void app_usb_msg_handle (void)
 *  {
 *      subtype = msg->subtype;
 *      group = USB_MSG_GROUP(subtype);
 *      switch(group)
 *      {
 *      case  USB_EVT:
 *          {
 *              usb_host_detect_evt_handle();
 *          }
 *          break;
 *      }
 *  }
 * \endcode
 *
 */
void usb_host_detect_evt_handle(uint8_t evt, uint32_t param);

/**
 *
 * \brief   Get USB host detect status.
 *
 * \return  Refer to \ref T_DETECT_STATUS.
 *
 */
T_DETECT_STATUS usb_host_detect_status_peek(void);

/**
 *
 * \brief   Get host OS type.
 *
 * \return  Refer to \ref T_OS_TYPE.
 *
 */
T_OS_TYPE usb_host_detect_os_type_peek(void);

/**
 *
 * \brief   Register HID infomation.
 * \param   HID related infomation.
 *
 */
void usb_host_detect_hid_info_register(T_USB_HOST_DETECT_HID_INFO info);

/**
 *
 * \brief   Initialize USB host detect.
 */
void usb_host_detect_init(HOST_DETECT_CB cb);

#endif
