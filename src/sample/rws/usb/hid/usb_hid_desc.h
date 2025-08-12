#ifndef __HID_DESC_H__
#define __HID_DESC_H__
#include <stdint.h>

#define USB_CLASS_CODE_HID                  0x03

#define DESC_TYPE_HID                       0x21
#define DESC_TYPE_REPORT                    0x22

#define HID_REPORT_ID_AUDIO_CONTROL         0x01
#define REPORT_ID_GAME_PAD_INPUT            0x07
#define HID_REPORT_DESC_CONSUMER_CONTROL  \
    0x05, 0x0C,                         /* Usage Page (Consumer)            */ \
          0x09, 0x01,                         /* Usage (Consumer Control)         */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
          0x85, HID_REPORT_ID_AUDIO_CONTROL,  /* Report ID (1)                    */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x01,                         /* Logical Maximum (1)              */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x01,                         /* Report Count (1)                 */ \
          0x09, 0xE9,                         /* Usage (Volume Increment)         */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x09, 0xEA,                         /* Usage (Volume Decrement)         */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x09, 0xCD,                         /* Usage (Play/Pause)               */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x09, 0xB5,                         /* Usage (Scan Next Track)          */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x09, 0xB6,                         /* Usage (Scan Previous Track)      */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0x95, 0x03,                         /* Report Count (3)                 */ \
          0x81, 0x01,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/ \
          0xC0,                               /* END_COLLECTION                    */

#if F_APP_GAMING_CONTROLLER_SUPPORT
#define GAME_PAD_HID_DESC_ATTRIB  \
    0x05, 0x01,             /* USAGE_PAGE       (Generic Desktop Controls)      */ \
          0x09, 0x05,             /* USAGE            (Game Pad)                      */ \
          0xA1, 0x01,             /* COLLECTION       (Application)           */ \
          0x85, REPORT_ID_GAME_PAD_INPUT, /* REPORT_ID        (0x10)                  */ \
          0x09, 0x30,             /* Usage (X)                                        */ \
          0x09, 0x31,             /* Usage (Y)                                        */ \
          0x09, 0x32,             /* Usage (Z)                                        */ \
          0x09, 0x35,             /* Usage (Rz)                                       */ \
          0x09, 0x33,             /* Usage (Rx)                                       */ \
          0x09, 0x34,             /* Usage (Ry)                                       */ \
          0x15, 0x00,             /* LOGICAL_MINIMUM  (0)                     */ \
          0x26, 0xFF, 0x00,       /* LOGICAL_MAXIMUM  (0xff)                  */ \
          0x75, 0x08,             /* REPORT_SIZE      (8)                     */ \
          0x95, 0x06,             /* REPORT_COUNT     (6)                     */ \
          0x81, 0x02,             /* INPUT            (Data,Var,Abs)          */ \
          0x06, 0x00, 0xFF,       /* USAGE            (vendor)                         */ \
          0x09, 0x20,             /* Usage (0x20)                                     */ \
          0x95, 0x01,             /* REPORT_COUNT     (1)                     */ \
          0x81, 0x02,             /* INPUT            (Data,Var,Abs)          */ \
          0x05, 0x01,             /* USAGE_PAGE       (Generic Desktop Controls)      */ \
          0x09, 0x39,             /* USAGE            (Hat switc)                      */ \
          0x15, 0x00,             /* LOGICAL_MINIMUM  (0)                     */ \
          0x25, 0x07,             /* LOGICAL_MAXIMUM  (0x07)                  */ \
          0x35, 0x00,             /* PHYSICAL_MINIMUM  (0)                     */ \
          0x46, 0x3B, 0x01,       /* PHYSICAL_MAXIMUM  (0x013B)                  */ \
          0x65, 0x14,             /* UNIT  (0x14)                               */ \
          0x75, 0x04,             /* REPORT_SIZE      (4)                     */ \
          0x95, 0x01,             /* REPORT_COUNT     (1)                     */ \
          0x81, 0x42,             /* INPUT(Data, Variable, Absolute, No Wrap, Linear, Preferred State, Null State, Bit Field)*/ \
          0x65, 0x00,             /* UNIT  (0x0)                               */ \
          0x05, 0x09,             /* USAGE            (Button)                         */ \
          0x19, 0x01,             /* USAGE_MINIMUM    (1)                     */ \
          0x29, 0x0F,             /* USAGE_MAXIMUM    (0x0F)                  */ \
          0x15, 0x00,             /* LOGICAL_MINIMUM  (0)                     */ \
          0x25, 0x01,             /* LOGICAL_MAXIMUM  (0x01)                  */ \
          0x75, 0x01,             /* REPORT_SIZE      (1)                     */ \
          0x95, 0x0F,             /* REPORT_COUNT     (F)                     */ \
          0x81, 0x02,             /* INPUT            (Data,Var,Abs)          */ \
          0x95, 0x05,             /* REPORT_COUNT     (5)                     */ \
          0x81, 0x01,             /* INPUT            (Data,Var,Abs)          */ \
          0xC0                    /* END_COLLECTION                           */
#endif

#define HID_REPORT_DESCS        HID_REPORT_DESC_CONSUMER_CONTROL

#define CONSUMER_CTRL_MAX_TRANSMISSION_UNIT                   0x02
#define CONSUMER_CTRL_MAX_PENDING_REQ_NUM                     0x0A

#endif
