#include <string.h>
#include <bt_types.h>
#include "trace.h"
//#include "os_sched.h"
#include "os_mem.h"
#include "os_queue.h"
#include "os_timer.h"
#include "rtl876x.h"
//#include "flash_device.h"
#include "stdlib.h"
#include "app_usb_hid.h"
#include "usb_lib_ext.h"
#include "app_cfu.h"
#include "app_ctrl_cfg.h"
#include "app_cfg.h"
#include "app_io_msg.h"
#include "app_src_avrcp.h"
#include "app_src_policy.h"
#include "app_src_asp.h"


const char report_desc_attrib[] =
{
    0x05, 0x0C,                             // Usage Page (Consumer)
    0x09, 0x01,                             // Usage (Consumer Control)
    0xA1, 0x01,                             // Collection (Application)
    0x85, REPORT_ID_CONSUMER_HOT_KEY_INPUT, //   Report ID (2)
    0x15, 0x00,                             //   Logical Minimum (0)
    0x25, 0x01,                             //   Logical Maximum (1)
    0x75, 0x01,                             //   Report Size (1)
    0x95, 0x01,                             //   Report Count (1)
    0x09, 0xE9,                             //   Usage (Volume Increment)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xEA,                             //   Usage (Volume Decrement)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xCD,                             //   Usage (Play/Pause)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB5,                             //   Usage (Scan Next Track)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB6,                             //   Usage (Scan Previous Track)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB7,                             //   Usage (Stop)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB3,                             //   Usage (Fast Forward)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB4,                             //   Usage (Rewind)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x0B,                             //   Usage Page (Telephony)
    0x09, 0x24,                             //   Usage (Redial)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x20,                             //   Usage (Hook Switch)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x2F,                             //   Usage (Phone Mute)
    0x81, 0x06,                             //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,                             //   Report Count (5)
    0x81, 0x01,                             //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                                   // End Collection

#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    0x06, 0x99, 0xFF,            // Usage Page (Vendor Defined 0xFF99)
    0x09, 0x01,                  // Usage (0x01)
    0xA1, 0x01,                  // Collection (Application)
    0x09, 0x03,                  //   Usage (0x03)
    0x85, REPORT_ID_ASP_FEATURE, //   Report ID (REPORT_ID_ASP_FEATURE)
    0x15, 0x00,                  //   Logical Minimum (0)
    0x26, 0xFF, 0x00,            //   Logical Maximum (255)
    0x75, 0x08,                  //   Report Size (8)
    0x95, 0x3E,                  //   Report Count (62)
    0xB2, 0x02, 0x01,            //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)
    0x09, 0x04,                  //   Usage (0x04)
    0x85, REPORT_ID_ASP_INPUT,   //   Report ID (REPORT_ID_ASP_INPUT)
    0x15, 0x00,                  //   Logical Minimum (0)
    0x25, 0x01,                  //   Logical Maximum (1)
    0x75, 0x01,                  //   Report Size (1)
    0x95, 0x01,                  //   Report Count (1)
    0x81, 0x02,                  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,                  //   Report Size (1)
    0x95, 0x0F,                  //   Report Count (15)
    0x81, 0x01,                  //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                        // End Collection

    // 44 bytes
#endif
    0x05, 0x0B,                      // Usage Page (Telephony)
    0x09, 0x05,                      // Usage (Headset)
    0xA1, 0x01,                      // Collection (Application)
    0x85, REPORT_ID_TELEPHONY_INPUT, //   Report ID (REPORT_ID_TELEPHONY_INPUT)
    0x05, 0x0B,                      //   Usage Page (Telephony)
    0x15, 0x00,                      //   Logical Minimum (0)
    0x25, 0x01,                      //   Logical Maximum (1)
    0x09, 0x20,                      //   Usage (Hook Switch)
    0x09, 0x97,                      //   Usage (Line Busy Tone)
    0x09, 0x2A,                      //   Usage (Line)
    0x09, 0x2F,                      //   Usage (Phone Mute)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x04,                      //   Report Count (3)
    0x81, 0x23,                      //   Input (Const,Var,Abs,No Wrap,Linear,No Preferred State,No Null Position)
    0x09, 0x21,                      //   Usage (Flash)
    0x09, 0x24,                      //   Usage (Redial)
    0x09, 0x50,                      //   Usage (Speed Dial)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x03,                      //   Report Count (4)
    0x81, 0x07,                      //   Input (Const,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x06,                      //   Usage (Telephony Key Pad)
    0xA1, 0x02,                      //   Collection (Logical)
    0x19, 0xB0,                      //     Usage Minimum (Phone Key 0)
    0x29, 0xBB,                      //     Usage Maximum (Phone Key Pound)
    0x15, 0x00,                      //     Logical Minimum (0)
    0x25, 0x0C,                      //     Logical Maximum (12)
    0x75, 0x04,                      //     Report Size (4)
    0x95, 0x01,                      //     Report Count (1)
    0x81, 0x40,                      //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,Null State)
    0xC0,                            //   End Collection
    0x09, 0x07,                      //   Usage (Programmable Button)
    0x15, 0x00,                      //   Logical Minimum (0)
    0x25, 0x01,                      //   Logical Maximum (1)
    0x05, 0x09,                      //   Usage Page (Button)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x01,                      //   Report Count (1)
    0x81, 0x02,                      //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x04,                      //   Report Count (4)
    0x81, 0x01,                      //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x08,                      //   Usage Page (LEDs)
    0x15, 0x00,                      //   Logical Minimum (0)
    0x25, 0x01,                      //   Logical Maximum (1)
    0x09, 0x17,                      //   Usage (Off-Hook)
    0x09, 0x1E,                      //   Usage (Speaker)
    0x09, 0x09,                      //   Usage (Mute)
    0x09, 0x18,                      //   Usage (Ring)
    0x09, 0x20,                      //   Usage (Hold)
    0x09, 0x21,                      //   Usage (Microphone)
    0x09, 0x2A,                      //   Usage (On-Line)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x07,                      //   Report Count (7)
    0x91, 0x22,                      //   Output (Data,Var,Abs,No Wrap,Linear,No Preferred State,No Null Position,Non-volatile)
    0x05, 0x0B,                      //   Usage Page (Telephony)
    0x15, 0x00,                      //   Logical Minimum (0)
    0x25, 0x01,                      //   Logical Maximum (1)
    0x09, 0x9E,                      //   Usage (Ringer)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x01,                      //   Report Count (1)
    0x91, 0x22,                      //   Output (Data,Var,Abs,No Wrap,Linear,No Preferred State,No Null Position,Non-volatile)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x08,                      //   Report Count (8)
    0x91, 0x01,                      //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,                            // End Collection

    // 126 bytes

    0x06, 0x0B, 0xFF, // Usage Page (Vendor Defined 0xFF0B)
    0x0A, 0x04, 0x01, // Usage (0x0104)
    0xA1, 0x01,       // Collection (Application)
    // 8-bit data
    0x15, 0x00,                        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,                  //   Logical Maximum (255)
    0x75, 0x08,                        //   Report Size (8)
    0x95, 0x3c,                        //   Report Count (60)
    0x85, REPORT_ID_CFU_FEATURE,       //   Report ID (REPORT_ID_CFU_FEATURE)
    0x09, 0x60,                        //   Usage (0x60)
    0x82, 0x02, 0x01,                  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Buffered Bytes)
    0x09, USAGE_ID_CFU_PAYLOAD_OUTPUT, //   Usage (0x61)
    0x92, 0x02, 0x01,                  //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)
    0x09, USAGE_ID_CFU_FEATURE,        //   Usage (0x62)
    0xB2, 0x02, 0x01,                  //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)
    0x85, REPORT_ID_CFU_FEATURE_EX,    //   Report ID (REPORT_ID_CFU_FEATURE_EX)
    0x09, USAGE_ID_CFU_FEATURE_EX,     //   Usage (0x65)
    0xB2, 0x02, 0x01,                  //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)
    // 32-bit data
    0x17, 0x00, 0x00, 0x00, 0x80,         //   Logical Minimum (-2147483649)
    0x27, 0xFF, 0xFF, 0xFF, 0x7F,         //   Logical Maximum (2147483646)
    0x75, 0x20,                           //   Report Size (32)
    0x95, 0x04,                           //   Report Count (4)
    0x85, REPORT_ID_CFU_PAYLOAD_INPUT,    //   Report ID (REPORT_ID_CFU_OFFER_INPUT)
    0x19, USAGE_ID_CFU_PAYLOAD_INPUT_MIN, //   Usage Minimum (0x66)
    0x29, USAGE_ID_CFU_PAYLOAD_INPUT_MAX, //   Usage Maximum (0x69)
    0x81, 0x02,                           //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, REPORT_ID_CFU_OFFER_INPUT,      //   Report ID (45)
    0x19, USAGE_ID_CFU_OFFER_INPUT_MIN,   //   Usage Minimum (0x8A)
    0x29, USAGE_ID_CFU_OFFER_INPUT_MAX,   //   Usage Maximum (0x8D)
    0x81, 0x02,                           //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x19, USAGE_ID_CFU_OFFER_OUTPUT_MIN,  //   Usage Minimum (0x8E)
    0x29, USAGE_ID_CFU_OFFER_OUTPUT_MAX,  //   Usage Maximum (0x91)
    0x91, 0x02,                           //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,                                 // End Collection

    // 77 bytes

    0x06, 0x00, 0xFF,                    // Usage Page (Vendor Defined 0xFF07)
    0x0A, 0x12, 0x02,                    // Usage (0x0212)
    0xA1, 0x01,                          // Collection (Application)
    0x85, REPORT_ID_DONGLE_TEST_FEATURE, //   Report ID (30)
    0x09, 0xc6,                          //   Usage (0xc6)
    0x75, 0x08,                          //   Report Size (8)
    0x95, 0x3C,                          //   Report Count (60)
    0x15, 0x00,                          //   Logical Minimum (0)
    0x26, 0xFF, 0x00,                    //   Logical Maximum (255)
    0xB1, 0x02,                          //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, REPORT_ID_DONGLE_TEST_INPUT,   //   Report ID (31)
    0x09, 0xc9,                          //   Usage (0xc9)
    0x81, 0x02,                          //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                                // End Collection

    // 29 bytes
};

#if 0

const char report_desc_attrib[] =
{
    0x05, 0x0C,                             // Usage Page (Consumer)
    0x09, 0x01,                             // Usage (Consumer Control)
    0xA1, 0x01,                             // Collection (Application)
    0x85, REPORT_ID_CONSUMER_HOT_KEY_INPUT, //   Report ID (2)
    0x15, 0x00,                             //   Logical Minimum (0)
    0x25, 0x01,                             //   Logical Maximum (1)
    0x75, 0x01,                             //   Report Size (1)
    0x95, 0x01,                             //   Report Count (1)
    0x09, 0xE9,                             //   Usage (Volume Increment)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xEA,                             //   Usage (Volume Decrement)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xCD,                             //   Usage (Play/Pause)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB5,                             //   Usage (Scan Next Track)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB6,                             //   Usage (Scan Previous Track)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB7,                             //   Usage (Stop)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB3,                             //   Usage (Fast Forward)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB4,                             //   Usage (Rewind)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x0B,                             //   Usage Page (Telephony)
    0x09, 0x24,                             //   Usage (Redial)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x20,                             //   Usage (Hook Switch)
    0x81, 0x02,                             //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x2F,                             //   Usage (Phone Mute)
    0x81, 0x06,                             //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,                             //   Report Count (5)
    0x81, 0x01,                             //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                                   // End Collection

#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    0x06, 0x99, 0xFF,            // Usage Page (Vendor Defined 0xFF99)
    0x09, 0x01,                  // Usage (0x01)
    0xA1, 0x01,                  // Collection (Application)
    0x09, 0x03,                  //   Usage (0x03)
    0x85, REPORT_ID_ASP_FEATURE, //   Report ID (REPORT_ID_ASP_FEATURE)
    0x15, 0x00,                  //   Logical Minimum (0)
    0x26, 0xFF, 0x00,            //   Logical Maximum (255)
    0x75, 0x08,                  //   Report Size (8)
    0x95, 0x3E,                  //   Report Count (62)
    0xB2, 0x02, 0x01,            //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)
    0x09, 0x04,                  //   Usage (0x04)
    0x85, REPORT_ID_ASP_INPUT,   //   Report ID (REPORT_ID_ASP_INPUT)
    0x15, 0x00,                  //   Logical Minimum (0)
    0x25, 0x01,                  //   Logical Maximum (1)
    0x75, 0x01,                  //   Report Size (1)
    0x95, 0x01,                  //   Report Count (1)
    0x81, 0x02,                  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,                  //   Report Size (1)
    0x95, 0x0F,                  //   Report Count (15)
    0x81, 0x01,                  //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                        // End Collection

    // 44 bytes
#endif
    0x05, 0x0B,                      // Usage Page (Telephony)
    0x09, 0x05,                      // Usage (Headset)
    0xA1, 0x01,                      // Collection (Application)
    0x85, REPORT_ID_TELEPHONY_INPUT, //   Report ID (REPORT_ID_TELEPHONY_INPUT)
    0x05, 0x0B,                      //   Usage Page (Telephony)
    0x15, 0x00,                      //   Logical Minimum (0)
    0x25, 0x01,                      //   Logical Maximum (1)
    0x09, 0x20,                      //   Usage (Hook Switch)
    0x09, 0x97,                      //   Usage (Line Busy Tone)
    0x09, 0x2A,                      //   Usage (Line)
    0x09, 0x2F,                      //   Usage (Phone Mute)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x04,                      //   Report Count (4)
    0x81, 0x23,                      //   Input (Const,Var,Abs,No Wrap,Linear,No Preferred State,No Null Position)
    0x09, 0x21,                      //   Usage (Flash)
    0x09, 0x24,                      //   Usage (Redial)
    0x09, 0x50,                      //   Usage (Speed Dial)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x03,                      //   Report Count (3)
    0x81, 0x07,                      //   Input (Const,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x06,                      //   Usage (Telephony Key Pad)
    0xA1, 0x02,                      //   Collection (Logical)
    0x19, 0xB0,                      //     Usage Minimum (Phone Key 0)
    0x29, 0xBB,                      //     Usage Maximum (Phone Key Pound)
    0x15, 0x00,                      //     Logical Minimum (0)
    0x25, 0x0C,                      //     Logical Maximum (12)
    0x75, 0x04,                      //     Report Size (4)
    0x95, 0x01,                      //     Report Count (1)
    0x81, 0x40,                      //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,Null State)
    0xC0,                            //   End Collection
    0x09, 0x07,                      //   Usage (Programmable Button)
    0x15, 0x00,                      //   Logical Minimum (0)
    0x25, 0x01,                      //   Logical Maximum (1)
    0x05, 0x09,                      //   Usage Page (Button)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x01,                      //   Report Count (1)
    0x81, 0x02,                      //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x04,                      //   Report Count (4)
    0x81, 0x01,                      //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x08,                      //   Usage Page (LEDs)
    0x15, 0x00,                      //   Logical Minimum (0)
    0x25, 0x01,                      //   Logical Maximum (1)
    0x09, 0x17,                      //   Usage (Off-Hook)
    0x09, 0x1E,                      //   Usage (Speaker)
    0x09, 0x09,                      //   Usage (Mute)
    0x09, 0x18,                      //   Usage (Ring)
    0x09, 0x20,                      //   Usage (Hold)
    0x09, 0x21,                      //   Usage (Microphone)
    0x09, 0x2A,                      //   Usage (On-Line)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x07,                      //   Report Count (7)
    0x91, 0x22,                      //   Output (Data,Var,Abs,No Wrap,Linear,No Preferred State,No Null Position,Non-volatile)
    0x05, 0x0B,                      //   Usage Page (Telephony)
    0x15, 0x00,                      //   Logical Minimum (0)
    0x25, 0x01,                      //   Logical Maximum (1)
    0x09, 0x9E,                      //   Usage (Ringer)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x01,                      //   Report Count (1)
    0x91, 0x22,                      //   Output (Data,Var,Abs,No Wrap,Linear,No Preferred State,No Null Position,Non-volatile)
    0x75, 0x01,                      //   Report Size (1)
    0x95, 0x08,                      //   Report Count (8)
    0x91, 0x01,                      //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,                            // End Collection

    // 126 bytes

    0x06, 0x0B, 0xFF, // Usage Page (Vendor Defined 0xFF0B)
    0x0A, 0x04, 0x01, // Usage (0x0104)
    0xA1, 0x01,       // Collection (Application)
    // 8-bit data
    0x15, 0x00,                        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,                  //   Logical Maximum (255)
    0x75, 0x08,                        //   Report Size (8)
    0x95, 0x3c,                        //   Report Count (60)
    0x85, REPORT_ID_CFU_FEATURE,       //   Report ID (REPORT_ID_CFU_FEATURE)
    0x09, 0x60,                        //   Usage (0x60)
    0x82, 0x02, 0x01,                  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Buffered Bytes)
    0x09, USAGE_ID_CFU_PAYLOAD_OUTPUT, //   Usage (0x61)
    0x92, 0x02, 0x01,                  //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)
    0x09, USAGE_ID_CFU_FEATURE,        //   Usage (0x62)
    0xB2, 0x02, 0x01,                  //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)
    0x85, REPORT_ID_CFU_FEATURE_EX,    //   Report ID (REPORT_ID_CFU_FEATURE_EX)
    0x09, USAGE_ID_CFU_FEATURE_EX,     //   Usage (0x65)
    0xB2, 0x02, 0x01,                  //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)
    // 32-bit data
    0x17, 0x00, 0x00, 0x00, 0x80,         //   Logical Minimum (-2147483649)
    0x27, 0xFF, 0xFF, 0xFF, 0x7F,         //   Logical Maximum (2147483646)
    0x75, 0x20,                           //   Report Size (32)
    0x95, 0x04,                           //   Report Count (4)
    0x85, REPORT_ID_CFU_PAYLOAD_INPUT,    //   Report ID (REPORT_ID_CFU_OFFER_INPUT)
    0x19, USAGE_ID_CFU_PAYLOAD_INPUT_MIN, //   Usage Minimum (0x66)
    0x29, USAGE_ID_CFU_PAYLOAD_INPUT_MAX, //   Usage Maximum (0x69)
    0x81, 0x02,                           //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, REPORT_ID_CFU_OFFER_INPUT,      //   Report ID (45)
    0x19, USAGE_ID_CFU_OFFER_INPUT_MIN,   //   Usage Minimum (0x8A)
    0x29, USAGE_ID_CFU_OFFER_INPUT_MAX,   //   Usage Maximum (0x8D)
    0x81, 0x02,                           //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x19, USAGE_ID_CFU_OFFER_OUTPUT_MIN,  //   Usage Minimum (0x8E)
    0x29, USAGE_ID_CFU_OFFER_OUTPUT_MAX,  //   Usage Maximum (0x91)
    0x91, 0x02,                           //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,                                 // End Collection

    // 77 bytes

    0x06, 0x00, 0xFF,                    // Usage Page (Vendor Defined 0xFF07)
    0x0A, 0x12, 0x02,                    // Usage (0x0212)
    0xA1, 0x01,                          // Collection (Application)
    0x85, REPORT_ID_DONGLE_TEST_FEATURE, //   Report ID (30)
    0x09, 0xc6,                          //   Usage (0xc6)
    0x75, 0x08,                          //   Report Size (8)
    0x95, 0x3C,                          //   Report Count (60)
    0x15, 0x00,                          //   Logical Minimum (0)
    0x26, 0xFF, 0x00,                    //   Logical Maximum (255)
    0xB1, 0x02,                          //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, REPORT_ID_DONGLE_TEST_INPUT,   //   Report ID (31)
    0x09, 0xc9,                          //   Usage (0xc9)
    0x81, 0x02,                          //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                                // End Collection

    // 29 bytes
};

#endif

uint16_t report_desc_attrib_length = sizeof(report_desc_attrib);

#ifdef CUSTOMER_FIXED_SETTINGS

const char *manufacturer = "Realtek";
uint8_t app_usb_hid_interrupt_in_buff[APP_USB_HID_MAX_IN_SIZE];

#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
const uint8_t usb_string_desc_id_addin = 33;
const char *usb_string_desc_str_addin = "UCQ01001000101000";
#endif

const uint8_t iSerialNumber = 3;
#define USB_MAX_NAME_STR  0x46
char usb_product_name_str_buf[USB_MAX_NAME_STR] = DEVICE_CUSTOMER_NAME_AUDIO_HID;
const char *product_desc = usb_product_name_str_buf;
#define USB_MAX_SN_STR  0x21
char usb_serial_number_str_buf[USB_MAX_SN_STR] = "Not Assigned";
const char *usb_serial_number_str = usb_serial_number_str_buf;
uint16_t ibcdDevice = 0;

void app_usb_update_show_fw_ver() //must called before usb init
{
    extern uint32_t app_cfu_get_version(bool active);
    uint32_t version = app_cfu_get_version(1);
    uint16_t bcd = version >> 24;
    ibcdDevice = ((bcd / 10) << 4 | (bcd % 10)) << 8;
    bcd = (version >> 8) & 0xFFFF;
    ibcdDevice |= (bcd / 10) << 4 | (bcd % 10);
    USB_PRINT_INFO2("app_usb_update_show_fw_ver version %d, ibcdDevice %x", version, ibcdDevice);
}

void app_usb_update_product_name(uint8_t *name, uint8_t length) //must called before usb start
{
    uint8_t i;
    USB_PRINT_INFO2("app_usb update product name length %d, name %b", length, TRACE_BINARY(length,
                    name));
    for (i = 0; i < length && i < USB_MAX_NAME_STR - 1; i++)
    {
        usb_product_name_str_buf[i] = name[i];
    }
    usb_product_name_str_buf[i] = 0;
    USB_PRINT_INFO1("app_usb product name str %b", TRACE_BINARY(USB_MAX_NAME_STR,
                                                                usb_product_name_str_buf));
}

#include "app_usb_dongle_test.h"
void app_usb_update_SN() //must called before usb start
{
    uint8_t length = *(uint8_t *)MSFT_SN_ADDR;
    uint8_t *data = (uint8_t *)(MSFT_SN_ADDR + 2);
    uint8_t type = *(uint8_t *)(MSFT_SN_ADDR + 1);
    uint8_t i;
    USB_PRINT_INFO3("app_usb SN length %d, type %d, data %b", length, type, TRACE_BINARY(length, data));
    if (type == 1) //ASCII
    {
        memcpy(usb_serial_number_str_buf, data, MIN(length, 32));
    }
    else if (type == 2) //BCD
    {
        for (i = 0; i < length && i < USB_MAX_SN_STR / 2; i++)
        {
            usb_serial_number_str_buf[i * 2] = ((data[i] & 0xF0) >> 4) + '0';
            usb_serial_number_str_buf[i * 2 + 1] = (data[i] & 0x0F) + '0';
        }
    }
    USB_PRINT_INFO1("app_usb SN str %b", TRACE_BINARY(USB_MAX_SN_STR, usb_serial_number_str));
}
#endif

typedef struct _HID_RID_MAP
{
    uint8_t reportID;
    uint8_t maptoID;
} HID_RID_MAP;

const HID_RID_MAP mouse_0x0212082fid_map[] =
{
    {1, 3},
    {2, 1},
    {0, 0}
};

uint8_t app_usb_hid_match_id(const HID_RID_MAP *map, uint8_t id)
{
    uint8_t i = 0;
    if (map == NULL)
    {
        return id;
    }
    do
    {
        if (map[i].reportID == id)
        {
            return map[i].maptoID;
        }
    }
    while (map[++i].reportID != 0);
    USB_PRINT_ERROR1("app_usb_hid_match_id not find id %d", id);
    return id;
}

void app_usb_hid_send_report(SEND_TYPE type, uint8_t id, uint8_t *data, uint8_t length,
                             uint32_t pnpinfo)
{
    uint8_t sendcount = MIN(length + 1, APP_USB_HID_MAX_IN_SIZE);
    memset(app_usb_hid_interrupt_in_buff, 0, sizeof(app_usb_hid_interrupt_in_buff));
    memcpy(&app_usb_hid_interrupt_in_buff[1], data, sendcount - 1);
    switch (type)
    {
    case HID_IF_KEYBOARD:
    case HID_IF_MOUSE:
        if (hid_is_suspend())
        {
            usb_remotewakeup();
        }
        switch (pnpinfo)
        {
        case 0x0212082f:
            app_usb_hid_interrupt_in_buff[0] = app_usb_hid_match_id(mouse_0x0212082fid_map, id);
            break;
        default:
            app_usb_hid_interrupt_in_buff[0] = id;
            break;
        }
        break;
    case HID_IF_CONSUMER:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_CONSUMER_HOT_KEY_INPUT;
        sendcount = 3;
        break;
    case HID_IF_HIDSYSCTRL:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_SYS_CTRL;
        sendcount = 3;
        break;
    case HID_IF_TELEPHONY:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_TELEPHONY_INPUT;
        sendcount = 3;
        USB_PRINT_TRACE2("USB HID send telephony, length %x, %b", sendcount, TRACE_BINARY(sendcount,
                         app_usb_hid_interrupt_in_buff));
        break;
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    case HID_IF_ASP:
        if (!app_usb_hid_asp_host_exist())
        {
            return;
        }
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_ASP_INPUT;
//        USB_PRINT_TRACE2("USB HID send asp, length %x, %b", sendcount, TRACE_BINARY(sendcount,
//                                                                                    app_usb_hid_interrupt_in_buff));
        sendcount = 3;
        break;
#endif
//    case HID_IF_GATT:
//        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_GATT_INPUT;
//        sendcount = 61;
//        break;
    case HID_IF_GAIA:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_GAIA_INPUT;
//        USB_PRINT_TRACE2("USB HID send gaia, length %x, %b", sendcount, TRACE_BINARY(sendcount,
//                                                                                     app_usb_hid_interrupt_in_buff));
        sendcount = 61;
        break;
//    case HID_IF_CTRL:
//        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_DONGLE_CTRL_INPUT;
//        sendcount = 61;
//        break;
    case HID_IF_TEST:
        app_usb_hid_interrupt_in_buff[0] = REPORT_ID_DONGLE_TEST_INPUT;
        sendcount = 61;
        break;
    default:
        break;
    }

//    USB_PRINT_TRACE3("USB HID send type %x, length %x, %b", type, sendcount, TRACE_BINARY(sendcount,
//                     app_usb_hid_interrupt_in_buff));
    app_hid_interrupt_in(app_usb_hid_interrupt_in_buff, sendcount);
}

__weak void app_usb_hid_handle_set_Keyboard_cmd(uint8_t *data, uint8_t length) {};
__weak void app_usb_hid_handle_set_Telephony_cmd(uint8_t *data, uint8_t length) {};
__weak void app_usb_hid_handle_set_ASP_cmd(uint8_t *data, uint8_t length) {};
__weak void app_usb_hid_handle_set_test_cmd(uint8_t *data, uint8_t length) {};
__weak void app_usb_hid_handle_set_praha_cmd(uint8_t *data, uint8_t length) {};

void hid_handle_set_report_msg(uint8_t *data, uint16_t length)
{
    USB_PRINT_TRACE3("hid_handle_set_report_msg ID %x, length 0x%x, %b", data[0], 10,
                     TRACE_BINARY(8, data));
    switch (data[0])
    {
    case REPORT_ID_KEYBOARD_OUTPUT:
        app_usb_hid_handle_set_Keyboard_cmd(data, length);
        break;
    case REPORT_ID_CFU_FEATURE_EX:
    case REPORT_ID_CFU_OFFER_OUTPUT:
    case REPORT_ID_CFU_PAYLOAD_OUTPUT:
        app_cfu_handle_set_report(data, length);
        break;
    case REPORT_ID_TELEPHONY_INPUT:
        app_usb_hid_handle_set_Telephony_cmd(data, length);
        break;
    case REPORT_ID_ASP_FEATURE:
        app_usb_hid_handle_set_ASP_cmd(data, length);
        break;
    case REPORT_ID_DONGLE_TEST_FEATURE:
        app_usb_hid_handle_set_test_cmd(data, length);
        break;
    default:
        USB_PRINT_ERROR3("app_usb_hid_set_report unknow ID %x, length 0x%x, %b", data[0], length,
                         TRACE_BINARY(length, data));
        break;
    }
}

void hid_handle_set_report(uint8_t *data, uint16_t length)
{
    USB_PRINT_TRACE2("hid_handle_set_report ID %x, length 0x%x", data[0], 10);
    switch (data[0])
    {
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
    case REPORT_ID_ASP_FEATURE:
        {
            extern uint8_t asp_set_message_id;
            asp_set_message_id = data[1];
            break;
        }
#endif
    default:
        break;
    }
    T_IO_MSG gpio_msg = {0};
    gpio_msg.type = IO_MSG_TYPE_USB_HID;
    gpio_msg.subtype = USB_HID_MSG_TYPE_HID_SET_REPORT | (length << 8);
    gpio_msg.u.buf = malloc(MIN(length, 0x3f));
    if (gpio_msg.u.buf == NULL)
    {
        APP_PRINT_ERROR0("hid_handle_set_report: malloc fail");
        return;
    }
    memcpy(gpio_msg.u.buf, data, MIN(length, 0x3f));
    if (app_io_msg_send(&gpio_msg) == false)
    {
        free(gpio_msg.u.buf);
        APP_PRINT_ERROR0("hid_handle_set_report: msg send fail");
    }
}

__weak uint8_t app_usb_hid_handle_get_ASP_cmd(uint8_t *data, uint16_t *length) {return 0;};
__weak uint8_t app_usb_hid_handle_get_praha_cmd(uint8_t *data, uint16_t *length) {return 0;};

uint8_t hid_handle_get_report(uint8_t reportID, uint8_t *data, uint16_t *length)
{
    uint8_t result = true;
    switch (reportID)
    {
    case REPORT_ID_CFU_FEATURE:
    case REPORT_ID_CFU_FEATURE_EX:
        app_cfu_handle_get_report(data, length);
        break;
    case REPORT_ID_ASP_FEATURE:
        result = app_usb_hid_handle_get_ASP_cmd(data, length);
        break;
    default:
        USB_PRINT_ERROR1("app_usb_hid_get_report unknow report ID %x", reportID);
        result = false;
        break;
    }
//    USB_PRINT_TRACE4("hid_handle_get_report ID %x, result %d, length 0x%x, %b", reportID, result,
//                     *length, TRACE_BINARY(*length, data));
    return result;
}

T_OS_QUEUE  hid_in_queue;
uint8_t app_hid_interrupt_in_state;
void app_hid_interrupt_in(uint8_t *data, uint8_t data_size)
{
    USB_PRINT_TRACE2("app_hid_interrupt_in data_size %x %b", data_size, TRACE_BINARY(MIN(data_size, 7),
                     data));
    uint8_t *buf = malloc(data_size + 1 + sizeof(T_OS_QUEUE_ELEM));
    buf[sizeof(T_OS_QUEUE_ELEM)] = data_size;
    memcpy(&buf[sizeof(T_OS_QUEUE_ELEM) + 1], data, data_size);
    os_queue_in(&hid_in_queue, buf);
    if (!app_hid_interrupt_in_state || hid_in_queue.count > 10)
    {
        T_IO_MSG gpio_msg;
        gpio_msg.type = IO_MSG_TYPE_USB_HID;
        gpio_msg.subtype = USB_HID_MSG_TYPE_HID_IN_REQUEST;
        gpio_msg.u.param = 0;
        app_hid_interrupt_in_state = 1;
        if (app_io_msg_send(&gpio_msg) == false)
        {
            APP_PRINT_ERROR0("app_hid_interrupt_in: msg send fail");
            app_hid_interrupt_in_state = 0;
            os_queue_delete(&hid_in_queue, buf);
            free(buf);
        }
    }
}

void hid_interrupt_in_complete_result(int result, uint8_t *buf)
{
    T_IO_MSG gpio_msg = {0};
    gpio_msg.type = IO_MSG_TYPE_USB_HID;
    gpio_msg.subtype = USB_HID_MSG_TYPE_HID_IN_COMPLETE;
    gpio_msg.u.buf = buf;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        free(--buf);
        APP_PRINT_ERROR0("hid_interrupt_in_complete_result: msg send fail");
    }
}

///////////////////////////patch for interrupt in lost // to slow down
static void *hid_in_timer_handle;
void hid_in_timer_cb(void *p)
{
//    USB_PRINT_TRACE2("hid_in_timer_cb state %x count %d",app_hid_interrupt_in_state, hid_in_queue.count);
    app_hid_interrupt_in_state = 0;
    if (hid_in_queue.count)
    {
        T_IO_MSG gpio_msg;
        gpio_msg.type = IO_MSG_TYPE_USB_HID;
        gpio_msg.subtype = USB_HID_MSG_TYPE_HID_IN_REQUEST;
        gpio_msg.u.param = 0;
        if (app_io_msg_send(&gpio_msg) == false)
        {
            APP_PRINT_ERROR0("USB_HID_MSG_TYPE_HID_IN_COMPLETE: msg send fail");
        }
        app_hid_interrupt_in_state = 1;
    }
}

void usb_low_level_resume_app_cb(void)
{
    T_IO_MSG gpio_msg;
    gpio_msg.type = IO_MSG_TYPE_USB_HID;
    gpio_msg.subtype = USB_HID_MSG_TYPE_LOW_LEVEL_RESUME;
    gpio_msg.u.param = 0;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("USB_HID_MSG_TYPE_LOW_LEVEL_RESUME: msg send fail");
    }
};

__weak void app_usb_hid_handle_io_asp_msg(T_IO_MSG *msg) {return ;};
__weak void app_usb_hid_handle_io_gaia_msg(T_IO_MSG *msg) {return ;};
void app_usb_hid_handle_msg(T_IO_MSG *msg)
{
    uint16_t hid_msg_type = msg->subtype & 0xff;
//    USB_PRINT_TRACE1("app_usb_hid_handle_msg %x",hid_msg_type);
    switch (hid_msg_type)
    {
    case USB_HID_MSG_TYPE_ASP:
        app_usb_hid_handle_io_asp_msg(msg);
        break;
    case USB_HID_MSG_TYPE_GAIA:
        app_usb_hid_handle_io_gaia_msg(msg);
        break;
    case USB_HID_MSG_TYPE_HID_SET_REPORT:
        {
            hid_handle_set_report_msg(msg->u.buf, (msg->subtype) >> 8);
            free(msg->u.buf);
        }
        break;
    case USB_HID_MSG_TYPE_CONSUMER_CRTL:
        {
            void app_usb_hid_request_uac_set_vol(uint16_t vol);
            app_usb_hid_request_uac_set_vol(msg->u.param);
        }
        break;
    case USB_HID_MSG_TYPE_HID_SUSPEND_RESUME:
        {
            T_IO_MSG gpio_msg = {0};
            gpio_msg.type = IO_MSG_TYPE_USB_HID;
            gpio_msg.subtype = USB_HID_MSG_TYPE_ASP;
            gpio_msg.u.param = 0xd0;
            if (app_io_msg_send(&gpio_msg) == false)
            {
                APP_PRINT_ERROR0("hid_handle_device_resume: msg send fail");
            }
            if (msg->u.param == 1)
            {
                USB_PRINT_TRACE0("hid_handle_device_suspend");
            }
            else if (msg->u.param == 2)
            {
                USB_PRINT_TRACE0("hid_handle_device_resume");
            }
        }
        break;
    case USB_HID_MSG_TYPE_HID_IN_REQUEST:
        {
//            USB_PRINT_TRACE2("USB_HID_MSG_TYPE_HID_IN_REQUEST state %x count %d",app_hid_interrupt_in_state, hid_in_queue.count);
            if (app_hid_interrupt_in_state)
            {
                if (hid_in_queue.count)
                {
                    uint8_t *buf = os_queue_out(&hid_in_queue);
                    if (hid_interrupt_in(&buf[sizeof(T_OS_QUEUE_ELEM) + 1], buf[sizeof(T_OS_QUEUE_ELEM)]))
                    {
                        USB_PRINT_TRACE2("USB_HID_MSG_TYPE_HID_IN_REQUEST addr %x %b", buf, TRACE_BINARY(4,
                                         &buf[sizeof(T_OS_QUEUE_ELEM)]));
                    }
                    else
                    {
                        USB_PRINT_WARN2("USB_HID_MSG_TYPE_HID_IN_REQUEST fail %x %b", buf,
                                        TRACE_BINARY(4, &buf[sizeof(T_OS_QUEUE_ELEM)]));
                        free(buf);
                        app_hid_interrupt_in_state = 0;
                        if (hid_in_queue.count)
                        {
                            T_IO_MSG gpio_msg;
                            gpio_msg.type = IO_MSG_TYPE_USB_HID;
                            gpio_msg.subtype = USB_HID_MSG_TYPE_HID_IN_REQUEST;
                            gpio_msg.u.param = 0;
                            if (app_io_msg_send(&gpio_msg) == false)
                            {
                                APP_PRINT_ERROR0("USB_HID_MSG_TYPE_HID_IN_COMPLETE: msg send fail");
                            }
                            app_hid_interrupt_in_state = 1;
                        }
                    }
                }
            }
            break;
        }
    case USB_HID_MSG_TYPE_HID_IN_COMPLETE:
        {
            uint8_t *buf = msg->u.buf;
            if (buf[0] == 2 && buf[1] == 0 && buf[2] == 0)
            {
                // add for patch host response 02 00 00 too late
                T_IO_MSG gpio_msg;
                gpio_msg.type = IO_MSG_TYPE_USB_HID;
                gpio_msg.subtype = USB_HID_MSG_TYPE_CONSUMER_CRTL;
                gpio_msg.u.param = 0xfe;
                if (app_io_msg_send(&gpio_msg) == false)
                {
                    APP_PRINT_ERROR0("app_usb_hid_handle_uac_set_volume: io msg send fail");
                }
            }
            buf -= (sizeof(T_OS_QUEUE_ELEM) + 1);
            USB_PRINT_TRACE2("USB_HID_MSG_TYPE_HID_IN_COMPLETE buf addr %x %b", buf, TRACE_BINARY(4,
                             &buf[sizeof(T_OS_QUEUE_ELEM)]));
            free(buf);
            os_timer_start(&hid_in_timer_handle);
            break;
        }
    case USB_HID_MSG_TYPE_LOW_LEVEL_RESUME:
        {
            APP_PRINT_ERROR0("USB_HID_MSG_TYPE_LOW_LEVEL_RESUME");
            src_usb_reset_reconn();
            break;
        }
    default:
        USB_PRINT_TRACE1("app_usb_hid_handle_msg unknow %x", hid_msg_type);
        break;
    }
}

void hid_handle_device_suspend(void)
{
    T_IO_MSG gpio_msg = {0};
    gpio_msg.type = IO_MSG_TYPE_USB_HID;
    gpio_msg.subtype = USB_HID_MSG_TYPE_HID_SUSPEND_RESUME;
    gpio_msg.u.param = 1;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("hid_handle_set_report: msg send fail");
    }
}

void hid_handle_device_resume(void)
{
    T_IO_MSG gpio_msg = {0};
    gpio_msg.type = IO_MSG_TYPE_USB_HID;
    gpio_msg.subtype = USB_HID_MSG_TYPE_HID_SUSPEND_RESUME;
    gpio_msg.u.param = 2;
    if (app_io_msg_send(&gpio_msg) == false)
    {
        APP_PRINT_ERROR0("hid_handle_set_report: msg send fail");
    }
}

/////////////////////////audio volume control
int16_t app_src_os_current_vol = 0x20;
int16_t app_src_uac_target_vol;
int16_t app_src_uac_target_vol_new;
int8_t app_src_uac_adjusting_vol;
int8_t app_src_os_spk_is_mute;
static void *app_src_uac_vol_timer_handle;
static uint8_t app_src_uac_adjusting_vol_host_count;
static uint8_t app_src_uac_adjusting_vol_host_count_check;

static bool host_sync_status_checked = false;
static bool host_synced = true;
static uint16_t vol_sync_timeout = 200;
extern bool app_src_hfp_sco_connected;
extern bool app_src_hfp_set_spk_vol(int16_t vol);
static void app_usb_hid_set_hs_volume(int16_t vol);

void app_src_uac_vol_timer_cb(void *p)
{
    APP_PRINT_ERROR4("app_src_uac_vol_timer_cb out of sync os %x, tar %x, tarn %x, adj %x,",
                     app_src_os_current_vol, app_src_uac_target_vol, app_src_uac_target_vol_new,
                     app_src_uac_adjusting_vol);
    uint8_t data[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};
    app_hid_interrupt_in(data, 3);

    if (app_src_uac_adjusting_vol == 0)
    {
        return;
    }

    app_src_uac_adjusting_vol = 0;

    if (!host_sync_status_checked)
    {
        host_sync_status_checked = true;
        host_synced = false;
    }

    if (!host_synced)
    {
        if (app_src_hfp_sco_connected)
        {
            app_src_os_current_vol = 0x5B;
        }
        else
        {
            app_src_os_current_vol = 0x70;
        }
        app_usb_hid_set_hs_volume(app_src_os_current_vol);
        vol_sync_timeout = 200;
    }
}

static void *app_src_bt_vol_set_retry_timer_handle;
static void app_usb_hid_set_hs_volume(int16_t vol);

void app_src_bt_vol_set_retrytimer_cb(void *p)
{
    if (app_src_uac_adjusting_vol == 0)
    {
        app_usb_hid_set_hs_volume(app_src_os_current_vol);
    }
}

static void app_usb_hid_set_hs_volume(int16_t vol)
{
    bool ret = true;

    if (app_src_hfp_sco_connected)
    {
        ret = app_src_hfp_set_spk_vol(vol);
    }
    else
    {
        ret = app_src_avrcp_set_spk_vol(vol);
    }

    if (!ret)
    {
        os_timer_start(&app_src_bt_vol_set_retry_timer_handle);
    }
}

void app_usb_hid_handle_uac_set_volume(uint32_t vol)
{
    if ((vol >= 2) && (vol <= 0x81))
    {
        app_src_os_current_vol = vol - 2;
    }
    else if (vol < 2)
    {
        app_src_os_spk_is_mute = vol;
        APP_PRINT_INFO1("app_usb_hid_handle_uac_set_volume mute %x", vol);
        return;
    }
    else
    {
        APP_PRINT_ERROR1("app_usb_hid_handle_uac_set_volume unsupported %x", vol);
    }
    APP_PRINT_TRACE4("app_usb_hid_handle_uac_set_volume os %x, tar %x, tar new %x, adj %x",
                     app_src_os_current_vol, app_src_uac_target_vol,
                     app_src_uac_target_vol_new, app_src_uac_adjusting_vol);
    app_src_uac_adjusting_vol_host_count++;

    if (app_src_uac_adjusting_vol > 0)
    {
        host_sync_status_checked = true;
        host_synced = true;
        vol_sync_timeout = 1000;
    }


    if ((app_src_uac_adjusting_vol == 2) && (app_src_os_current_vol < app_src_uac_target_vol))
    {
        uint8_t data[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x01, 0x00};
        app_hid_interrupt_in(data, 3);
        //uint8_t data2[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x01, 0x00};
        //app_hid_interrupt_in(data2, 3);
    }
    else if ((app_src_uac_adjusting_vol == 1) && (app_src_os_current_vol > app_src_uac_target_vol))
    {
        uint8_t data[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x02, 0x00};
        app_hid_interrupt_in(data, 3);
        //uint8_t data2[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x02, 0x00};
        //app_hid_interrupt_in(data2, 3);
    }
    else if (app_src_uac_adjusting_vol > 0)
    {
        if (app_src_uac_target_vol_new != app_src_uac_target_vol)
        {
            app_src_uac_target_vol = app_src_uac_target_vol_new;
            if (app_src_os_current_vol < app_src_uac_target_vol)
            {
                uint8_t data[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x01, 0x00};
                app_hid_interrupt_in(data, 3);
                app_src_uac_adjusting_vol = 2;
            }
            else if (app_src_os_current_vol > app_src_uac_target_vol)
            {
                uint8_t data[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x02, 0x00};
                app_hid_interrupt_in(data, 3);
                app_src_uac_adjusting_vol = 1;
            }
        }
        else
        {
            uint8_t data[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};
            app_hid_interrupt_in(data, 3);
            // remove for patch host response 02 00 00 too late
//            T_IO_MSG gpio_msg;
//            gpio_msg.type = IO_MSG_TYPE_USB_HID;
//            gpio_msg.subtype = USB_HID_MSG_TYPE_CONSUMER_CRTL;
//            gpio_msg.u.param = 0xfe;
//            if (app_io_msg_send(&gpio_msg) == false)
//            {
//                APP_PRINT_ERROR0("app_usb_hid_handle_uac_set_volume: io msg send fail");
//            }
        }
    }
    else if (app_src_uac_adjusting_vol == 0)
    {
        app_usb_hid_set_hs_volume(app_src_os_current_vol);
        // extern bool app_src_hfp_sco_connected;
        // if (app_src_hfp_sco_connected)
        // {
        //     extern void app_src_hfp_set_spk_vol(int16_t vol);
        //     app_src_hfp_set_spk_vol(app_src_os_current_vol);
        // }
        // else
        // {
        //     app_src_avrcp_set_spk_vol(app_src_os_current_vol);
        // }
    }
    else if (app_src_uac_adjusting_vol < 0)
    {
        app_src_uac_adjusting_vol = 0;
    }
    return ;
}

void app_usb_hid_request_uac_set_vol(uint16_t vol)
{
    APP_PRINT_TRACE4("app_usb_hid_request_uac_set_vol %x, adj %x, tar %x, tar new %x",
                     vol, app_src_uac_adjusting_vol, app_src_uac_target_vol, app_src_uac_target_vol_new);
    uint8_t vol_adjust = 0;

    if ((vol & 0x0100) && (!host_synced))
    {
        app_src_os_current_vol = 0x5B;
        app_usb_hid_set_hs_volume(app_src_os_current_vol);
    }
    else if ((vol & 0x0200) && (!host_synced))
    {
        app_src_os_current_vol = 0x70;
        app_usb_hid_set_hs_volume(app_src_os_current_vol);
    }
    else if ((vol & 0xff) == 0xff)
    {
        app_src_avrcp_set_spk_vol(app_src_os_current_vol);
    }
    else if ((vol & 0xff) == 0xfe)
    {
        app_src_uac_adjusting_vol = 0;
        os_timer_stop(&app_src_uac_vol_timer_handle);
        if (app_src_uac_target_vol_new != app_src_uac_target_vol)
        {
            vol_adjust = 1;
            app_src_uac_target_vol = app_src_uac_target_vol_new;
        }
    }
    else
    {
        os_timer_restart(&app_src_uac_vol_timer_handle, vol_sync_timeout);
        app_src_uac_target_vol_new = vol & 0xff;

        if (app_src_uac_adjusting_vol == 0)
        {
            app_src_uac_target_vol = app_src_uac_target_vol_new;
            vol_adjust = 1;
        }
        else
        {
            if (app_src_uac_target_vol_new > app_src_os_current_vol)
            {
                app_src_uac_target_vol = app_src_uac_target_vol_new;
                app_src_uac_adjusting_vol = 2;
            }
            else if (app_src_uac_target_vol_new < app_src_os_current_vol)
            {
                app_src_uac_target_vol = app_src_uac_target_vol_new;
                app_src_uac_adjusting_vol = 1;
            }

            if (app_src_uac_adjusting_vol_host_count != app_src_uac_adjusting_vol_host_count_check)
            {
                app_src_uac_adjusting_vol_host_count_check = app_src_uac_adjusting_vol_host_count;
            }
            else
            {
                vol_adjust = 1;
                app_src_uac_adjusting_vol = 0;
            }
        }
    }
    if (vol_adjust)
    {
        uint8_t data[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};
        uint8_t data2[3] = {REPORT_ID_CONSUMER_HOT_KEY_INPUT, 0x00, 0x00};
        app_src_uac_adjusting_vol_host_count_check = app_src_uac_adjusting_vol_host_count;
        if (app_src_uac_adjusting_vol == 0)
        {
            if (app_src_os_current_vol > app_src_uac_target_vol_new)
            {
                app_src_uac_adjusting_vol = 1;
                data[1] = 0x02;
                app_hid_interrupt_in(data, 3);
                data2[1] = 0x02;
                app_hid_interrupt_in(data2, 3);
            }
            else if (app_src_os_current_vol < app_src_uac_target_vol_new)
            {
                app_src_uac_adjusting_vol = 2;
                data[1] = 0x01;
                app_hid_interrupt_in(data, 3);
                data2[1] = 0x01;
                app_hid_interrupt_in(data2, 3);
            }
        }
        APP_PRINT_TRACE3("app_usb_hid_request_uac_set_vol cur %x, tar_new %x, adj %x",
                         app_src_os_current_vol,
                         app_src_uac_target_vol_new, app_src_uac_adjusting_vol);
    }
    return ;
}

void app_usb_hid_init(void)
{
    os_queue_init(&hid_in_queue);
    os_timer_create(&hid_in_timer_handle, "hid_in_timer", 0xfafa, 1, false, hid_in_timer_cb);
    os_timer_create(&app_src_uac_vol_timer_handle, "vol_in_timer",
                    0xfafb, vol_sync_timeout, false, app_src_uac_vol_timer_cb);
    os_timer_create(&app_src_bt_vol_set_retry_timer_handle, "vol_retry_timer",
                    0xfafc, 10, false, app_src_bt_vol_set_retrytimer_cb);
}
