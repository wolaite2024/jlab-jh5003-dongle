/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#ifndef _APP_HID_REPORT_DESC_H_
#define _APP_HID_REPORT_DESC_H_

//****************************************************************************
//
//                                  DEFINES
//
//****************************************************************************

// Microsoft Corp/Device Info
#define MS_VENDOR_ID            (0x045E)
#define MS_VENDOR_ID_SOURCE     (0x0002)
#define MS_PRODUCT_ID           (0x0852)
#define MS_PRODUCT_VERSION      (0x0112)
#define MS_USB_BCD_VERSION      (0x0200)
#define MS_DEVICE_PLATFORM_ID   (0x5F)
#define MS_RTL8763_COMPONENT_ID (0x10)
#define MS_RK2108_COMPONENT_ID  (0x11)

// Generic Desktop (0x01)
#define REPORT_ID_KEYBOARD_FEATURE          0x01
#define REPORT_ID_KEYBOARD_INPUT            REPORT_ID_KEYBOARD_FEATURE
#define REPORT_ID_KEYBOARD_OUTPUT           REPORT_ID_KEYBOARD_FEATURE
#define REPORT_ID_MOUSE_INPUT               0x03
#define REPORT_ID_WIRELESS_CONSUMER         0x05

// Consumer (0x0C)
#define REPORT_ID_CONSUMER_HOT_KEY_INPUT    0x02
#define REPORT_ID_GAME_PAD_INPUT            0x07

// Telephony Headset (0x0B)
#define REPORT_ID_TELEPHONY_INPUT           0x0B

// Microsoft CFU (0xFF0B:0104)
#define REPORT_ID_CFU_FEATURE               0x2A
#define REPORT_ID_CFU_FEATURE_EX            0x2B
#define REPORT_ID_CFU_OFFER_INPUT           0x2D
#define REPORT_ID_CFU_OFFER_OUTPUT          REPORT_ID_CFU_OFFER_INPUT
#define REPORT_ID_CFU_PAYLOAD_INPUT         0x2C
#define REPORT_ID_CFU_PAYLOAD_OUTPUT        REPORT_ID_CFU_FEATURE

#define USAGE_ID_CFU_FEATURE                0x62
#define USAGE_ID_CFU_FEATURE_EX             0x65
#define USAGE_ID_CFU_OFFER_OUTPUT_MIN       0x8E
#define USAGE_ID_CFU_OFFER_OUTPUT_MAX       0x91
#define USAGE_ID_CFU_OFFER_INPUT_MIN        0x8A
#define USAGE_ID_CFU_OFFER_INPUT_MAX        0x8D
#define USAGE_ID_CFU_PAYLOAD_INPUT_MIN      0x66
#define USAGE_ID_CFU_PAYLOAD_INPUT_MAX      0x69
#define USAGE_ID_CFU_PAYLOAD_OUTPUT         0x61

// PASSTHROUGH CFU (0xFF07:0212)
#define REPORT_ID_CTRL_DATA_OUT_REQUEST     0x24
#define REPORT_ID_CTRL_DATA_IN_REQUEST      0x27

#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
#define REPORT_ID_PASSTHROUGH_CMD_OUTPUT    0x80
#define REPORT_ID_PASSTHROUGH_CMD_INPUT     0x81
#endif

#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)
#define REPORT_ID_PASSTHROUGH_TEST_FEATURE              0x34
#define REPORT_ID_PASSTHROUGH_TEST_INPUT                0x37
#define REPORT_ID_PASSTHROUGH_CFU_FEATURE               0x3A
#define REPORT_ID_PASSTHROUGH_CFU_FEATURE_EX            0x3B
#define REPORT_ID_PASSTHROUGH_CFU_OFFER_INPUT           0x3D
#define REPORT_ID_PASSTHROUGH_CFU_OFFER_OUTPUT          REPORT_ID_PASSTHROUGH_CFU_OFFER_INPUT
#define REPORT_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT         0x3C
#define REPORT_ID_PASSTHROUGH_CFU_PAYLOAD_OUTPUT        REPORT_ID_PASSTHROUGH_CFU_FEATURE

#define USAGE_ID_PASSTHROUGH_CFU_FEATURE                0xd6
#define USAGE_ID_PASSTHROUGH_CFU_FEATURE_EX             0xd7
#define USAGE_ID_PASSTHROUGH_CFU_OFFER_OUTPUT_MIN       0xec
#define USAGE_ID_PASSTHROUGH_CFU_OFFER_OUTPUT_MAX       0xef
#define USAGE_ID_PASSTHROUGH_CFU_OFFER_INPUT_MIN        0xe8
#define USAGE_ID_PASSTHROUGH_CFU_OFFER_INPUT_MAX        0xeb
#define USAGE_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT_MIN      0xe0
#define USAGE_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT_MAX      0xe3
#define USAGE_ID_PASSTHROUGH_CFU_PAYLOAD_OUTPUT         0xd5
#endif

// Microsoft Teams ASP - Teams button (0xff99:01)
#define REPORT_ID_ASP_FEATURE               0x9A
#define REPORT_ID_ASP_INPUT                 0x9B

#define USAGE_ID_ASP_FEATURE                0x03
#define USAGE_ID_ASP_INPUT                  0x04

// Microsoft App HID Cmd (0xFF07:0222)



//others
#define REPORT_ID_TELEMETRY_INPUT           0x04
#define REPORT_ID_MKC_EXECUTION_INPUT       0x04
#define REPORT_ID_SYS_CTRL                  0x08
#define REPORT_ID_DEVICE_PROPERTIES_INPUT   0x39
#define REPORT_ID_EMOJI                     0x10
#define REPORT_ID_GAIA_FEATURE              0x1A
#define REPORT_ID_GAIA_INPUT                0x1B
#define REPORT_ID_DONGLE_CTRL_FEATURE       0x1E
#define REPORT_ID_DONGLE_CTRL_INPUT         0x1F



#define APP_CFU_HID_DESC_ATTRIB  \
    0x06, 0x0b, 0xff,                   /* USAGE_PAGE       Vendor          */ \
          0x0a, 0x04, 0x01,                   /* USAGE                            */ \
          0xa1, 0x01,                         /* COLLECTION        (Application)  */ \
          /* 8-bit data */ \
          0x15, 0x00,                         /* LOGICAL_MINIMUM   (0)            */ \
          0x26, 0xFF, 0x00,                   /* LOGICAL_MAXIMUM   (255)          */ \
          0x75, 0x08,                         /* REPORT_SIZE       (8)            */ \
          0x95, 0x3c,                         /* REPORT_COUNT      (60)           */ \
          0x85, REPORT_ID_CFU_FEATURE,        /* REPORT_ID         (42)           */ \
          0x09, 0x60,                         /* USAGE             (0x60)         */ \
          0x82, 0x02, 0x01,                   /* INPUT             (Data,Var,Abs) */ \
          0x09, USAGE_ID_CFU_PAYLOAD_OUTPUT,  /* USAGE             (0x61)         */ \
          0x92, 0x02, 0x01,                   /* OUTPUT            (Data,Var,Abs) */ \
          0x09, USAGE_ID_CFU_FEATURE,         /* USAGE             (0x62)         */ \
          0xb2, 0x02, 0x01,                   /* FEATURE           (Data,Var,Abs) */ \
          0x85, REPORT_ID_CFU_FEATURE_EX,     /* Report ID         (0x2B)         */ \
          0x09, USAGE_ID_CFU_FEATURE_EX,      /* Usage             (0x65)         */ \
          0xB2, 0x02, 0x01,                   /* Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes) */ \
          /* 32-bit data */ \
          0x17, 0x00, 0x00, 0x00, 0x80,       /* LOGICAL_MINIMUM                 */ \
          0x27, 0xff, 0xff, 0xff, 0x7f,       /* LOGICAL_MAXIMUM                 */ \
          0x75, 0x20,                         /* REPORT_SIZE      (32)           */ \
          0x95, 0x04,                         /* REPORT_COUNT     (4)            */ \
          0x85, REPORT_ID_CFU_PAYLOAD_INPUT,  /* REPORT_ID        (44)           */ \
          0x19, USAGE_ID_CFU_PAYLOAD_INPUT_MIN, /* USAGE_MINIMUM    (0x66)         */ \
          0x29, USAGE_ID_CFU_PAYLOAD_INPUT_MAX, /* USAGE_MAXIMUM    (0x69)         */ \
          0x81, 0x02,                         /* INPUT            (Data,Var,Abs) */ \
          0x85, REPORT_ID_CFU_OFFER_INPUT,    /* REPORT_ID        (45)           */ \
          0x19, USAGE_ID_CFU_OFFER_INPUT_MIN, /* USAGE_MINIMUM    (0x8a)         */ \
          0x29, USAGE_ID_CFU_OFFER_INPUT_MAX, /* USAGE_MAXIMUM    (0x8d)         */ \
          0x81, 0x02,                         /* INPUT            (Data,Var,Abs) */ \
          0x19, USAGE_ID_CFU_OFFER_OUTPUT_MIN, /* USAGE_MINIMUM    (0x8e)         */ \
          0x29, USAGE_ID_CFU_OFFER_OUTPUT_MAX, /* USAGE_MAXIMUM    (0x91)         */ \
          0x91, 0x02,                         /* OUTPUT           (Data,Var,Abs) */ \
          0xc0                                /* END_COLLECTION                  */
// HID_USAGE_PAGE_16(0x0B, 0xFF),              // USAGE PAGE (Vendor 0xFF0B)
// HID_USAGE_16(0x04,0x01),                    // USAGE(0x0104)
// HID_COLLECTION(Application),                // COLLECTION (Application)
//     // 8-bit data
//     HID_LOGICAL_MIN_8(0x00),                    // LOGICAL_MINIMUM (0x00)
//     HID_LOGICAL_MAX_8(0xFF),                    // LOGICAL_MAXIMUM (0xFF)
//     HID_REPORT_SIZE(8),                         // REPORT SIZE (8)
//     HID_REPORT_COUNT(60),                       // REPORT COUNT (60)
//     HID_REPORT_ID(0x2A),                        // REPORT ID (0x2A)
//     HID_USAGE_8(0x60),                          // USAGE (60)
//     HID_INPUT_16(Data_Var_Abs, BuffBytes),      // Unused, CFU Challenge
//     HID_USAGE_8(0x61),                          // USAGE (0x61)
//     HID_OUTPUT_16(Data_Var_Abs, BuffBytes),     // CFU content
//     HID_USAGE_8(0x62),                          // USAGE (0x62)
//     HID_FEATURE_16(Data_Var_Abs, BuffBytes),    // CFU version
//     // 32-bit data
//     HID_LOGICAL_MIN_32(0x00, 0x00, 0x00, 0x80), // LOGICAL_MINIMUM (0x80000000)
//     HID_LOGICAL_MAX_32(0xFF, 0xFF, 0xFF, 0x7F), // LOGICAL_MAXIMUM (0x7F000000)
//     HID_REPORT_SIZE(32),                        // REPORT SIZE (32)
//     HID_REPORT_COUNT(4),                        // REPORT COUNT (4)
//     HID_REPORT_ID(0x2C),                        // REPORT ID (0x2C)
//     HID_USAGE_MIN_8(0x66),                      // USAGE MIN (0x66)
//     HID_USAGE_MAX_8(0x69),                      // USAGE MAX (0x69)
//     HID_INPUT_8(Data_Var_Abs),                  // CFU status report
//     HID_REPORT_ID(0x2D),                        // REPORT ID (0x2D)
//     HID_USAGE_MIN_8(0x8A),                      // USAGE MIN (0x8A)
//     HID_USAGE_MAX_8(0x8D),                      // USAGE MAX (0x8D)
//     HID_INPUT_8(Data_Var_Abs),                  // CFU offer response
//     HID_USAGE_MIN_8(0x8E),                      // USAGE MIN (0x8E)
//     HID_USAGE_MAX_8(0x91),                      // USAGE MAX (0x91)
//     HID_OUTPUT_8(Data_Var_Abs),                 // CFU offer
// HID_END_COLLECTION,                         // Application collection

#define VENDOR_0xFF07_HID_DESC_PART_1  \
    0x06, 0x07, 0xFF,                       /* Usage Page (Vendor Defined 0xFF07)                                                   */ \
          0x0A, 0x12, 0x02,                       /* Usage (0x0212)                                                                       */ \
          0xA1, 0x01,                             /* Collection (Application)                                                             */ \
          0x75, 0x08,                             /*   Report Size (8)                                                                    */ \
          0x95, 0x3e,                             /*   Report Count (62)                                                                  */ \
          0x15, 0x00,                             /*   Logical Minimum (0)                                                                */ \
          0x26, 0xFF, 0x00,                       /*   Logical Maximum (255)                                                              */ \
          0x85, REPORT_ID_CTRL_DATA_OUT_REQUEST,  /*   Report ID (30)                                                                     */ \
          0x09, 0xc6,                             /*   Usage (0xc6)                                                                       */ \
          0xB1, 0x02,                             /*   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/ \
          0x85, REPORT_ID_CTRL_DATA_IN_REQUEST,   /*   Report ID (31)                                                                     */ \
          0x09, 0xc9,                             /*   Usage (0xc9)                                                                       */ \
          0x81, 0x02                              /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)               */
#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
#define VENDOR_0xFF07_HID_DESC_PART_2  \
    0x85, REPORT_ID_PASSTHROUGH_CMD_OUTPUT, /*   Report ID (30)                                                                      */ \
          0x09, 0xf4,                             /*   Usage (0xc6)                                                                        */ \
          0xB1, 0x02,                             /*   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile) */ \
          0x85, REPORT_ID_PASSTHROUGH_CMD_INPUT,  /*   Report ID (31)                                                                      */ \
          0x09, 0xf5,                             /*   Usage (0xc9)                                                                        */ \
          0x81, 0x02                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)                */
#endif
#if (USB_PASSTHROUGH_CFU_SUPPORT == 1)
#define VENDOR_0xFF07_HID_DESC_PART_3  \
    0x85, REPORT_ID_PASSTHROUGH_TEST_FEATURE,   /* Report ID (52)                                                                                            */ \
          0x09, 0xCA,                                 /* Usage (0xCA)                                                                                              */ \
          0xB1, 0x02,                                 /* Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)                       */ \
          0x85, REPORT_ID_PASSTHROUGH_TEST_INPUT,     /* Report ID (55)                                                                                            */ \
          0x09, 0xCB,                                 /* Usage (0xCB)                                                                                              */ \
          0x81, 0x02,                                 /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)                                      */ \
          0x15, 0x00,                                 /* Logical Minimum (0)                                                                                       */ \
          0x26, 0xFF, 0x00,                           /* Logical Maximum (255)                                                                                     */ \
          0x75, 0x08,                                 /* Report Size (8)                                                                                           */ \
          0x95, 0x3C,                                 /* Report Count (60)                                                                                         */ \
          0x85, REPORT_ID_PASSTHROUGH_CFU_FEATURE,    /* Report ID (58)                                                                                            */ \
          0x09, 0x60,                                 /* Usage (0x60)                                                                                              */ \
          0x82, 0x02, 0x01,                           /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Buffered Bytes)                       */ \
          0x09, USAGE_ID_PASSTHROUGH_CFU_PAYLOAD_OUTPUT,  /* Usage (0xD5)                                                                                          */ \
          0x92, 0x02, 0x01,                           /* Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)         */ \
          0x09, USAGE_ID_PASSTHROUGH_CFU_FEATURE,     /* Usage (0xD6)                                                                                              */ \
          0xB2, 0x02, 0x01,                           /* Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)        */ \
          0x85, REPORT_ID_PASSTHROUGH_CFU_FEATURE_EX, /* Report ID (59)                                                                                            */ \
          0x09, USAGE_ID_PASSTHROUGH_CFU_FEATURE_EX,  /* Usage (0xD7)                                                                                              */ \
          0xB2, 0x02, 0x01,                           /* Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)        */ \
          0x17, 0x00, 0x00, 0x00, 0x80,               /* Logical Minimum (-2147483649)                                                                             */ \
          0x27, 0xFF, 0xFF, 0xFF, 0x7F,               /* Logical Maximum (2147483646)                                                                              */ \
          0x75, 0x20,                                 /* Report Size (32)                                                                                          */ \
          0x95, 0x04,                                 /* Report Count (4)                                                                                          */ \
          0x85, REPORT_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT,      /* Report ID (60)                                                                                    */ \
          0x19, USAGE_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT_MIN,   /* Usage Minimum (0xE0)                                                                              */ \
          0x29, USAGE_ID_PASSTHROUGH_CFU_PAYLOAD_INPUT_MAX,   /* Usage Maximum (0xE3)                                                                              */ \
          0x81, 0x02,                                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)                              */ \
          0x85, REPORT_ID_PASSTHROUGH_CFU_OFFER_INPUT,        /* Report ID (61)                                                                                    */ \
          0x19, USAGE_ID_PASSTHROUGH_CFU_OFFER_INPUT_MIN,     /* Usage Maximum (0xEB)                                                                              */ \
          0x29, USAGE_ID_PASSTHROUGH_CFU_OFFER_INPUT_MAX,     /* Usage Minimum (0xE8)                                                                              */ \
          0x81, 0x02,                                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)                              */ \
          0x19, USAGE_ID_PASSTHROUGH_CFU_OFFER_OUTPUT_MIN,    /* Usage Minimum (0xEC)                                                                              */ \
          0x29, USAGE_ID_PASSTHROUGH_CFU_OFFER_OUTPUT_MAX - 1, /* Usage Maximum (0xEE)                                                                             */ \
          0x91, 0x02,                                         /* Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)                */ \
          0x09, USAGE_ID_PASSTHROUGH_CFU_OFFER_OUTPUT_MAX,    /* Usage (0xEF)                                                                                      */ \
          0xB2, 0x02, 0x01                                    /* Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)*/
#endif
#define VENDOR_0xFF07_HID_DESC_END_COLLECTION  \
    0xC0                                               /* End Collection    */


#define APP_CMD_HID_DESC_ATTRIB  \
    0x06, 0x07, 0xff,                   /* USAGE_PAGE       Vendor          */ \
          0x0a, 0x22, 0x02,                   /* USAGE                            */ \
          0xa1, 0x01,                         /* COLLECTION        (Application)  */ \
          /* 8-bit data */ \
          0x85, REPORT_ID_APP_CMD_FEATURE,    /* REPORT_ID         (24)           */ \
          0x75, 0x08,                         /* REPORT_SIZE       (8)            */ \
          0x95, 0x3e,                         /* REPORT_COUNT      (62)           */ \
          0x15, 0x00,                         /* LOGICAL_MINIMUM   (0)            */ \
          0x26, 0xFF, 0x00,                   /* LOGICAL_MAXIMUM   (255)          */ \
          0x09, 0x01,                         /* USAGE             (0x01)         */ \
          0xb1, 0x02,                         /* FEATURE           (Data,Var,Abs) */ \
          0x85, REPORT_ID_APP_CMD_INPUT,      /* REPORT_ID         (27)           */ \
          0x95, 0x3e,                         /* REPORT_COUNT      (62)           */ \
          0x09, 0x02,                         /* USAGE             (0x02)         */ \
          0x81, 0x02,                         /* INPUT             (Data,Var,Abs) */ \
          0x85, REPORT_ID_CFU_FEATURE_5A,     /* REPORT_ID         (5A)           */ \
          0x95, 0x3F,                         /* REPORT_COUNT      (63)           */ \
          0x09, 0x04,                         /* USAGE             (0x04)         */ \
          0xB1, 0x02,                         /* FEATURE           (Data,Var,Abs) */ \
          0x85, REPORT_ID_CFU_INPUT_5B,      /* REPORT_ID         (5B)           */ \
          0x95, 0x13,                         /* REPORT_COUNT      (19)           */ \
          0x09, 0x05,                         /* USAGE             (0x05)         */ \
          0x81, 0x02,                         /* INPUT             (Data,Var,Abs) */ \
          0x85, REPORT_ID_VP_FEATURE,         /* Report ID (0xF1)                 */ \
          0x95, 0x3c,                         /* REPORT_COUNT      (60)           */ \
          0x09, 0xf4,                         /* USAGE             (0xF4)         */ \
          0xB1, 0x02,                         /* FEATURE           (Data,Var,Abs) */ \
          0x85, REPORT_ID_VP_INPUT,           /* Report ID (0xF2)                 */ \
          0x09, 0xf5,                         /* USAGE             (0xF5)         */ \
          0x81, 0x02,                         /* INPUT             (Data,Var,Abs) */ \
          0xc0                                /* END_COLLECTION                   */
// HID_USAGE_PAGE_16(0x07, 0xFF),      // USAGE PAGE (Vendor 0xFF07)
// HID_USAGE_16(0x22, 0x02),           // USAGE(0x0222)
// HID_COLLECTION(Application),        // COLLECTION (Application)
//     //Generic Host to Device Packet
//     HID_REPORT_ID(0x24),                // REPORT ID (0x24)
//     HID_REPORT_SIZE(0x08),              // REPORT SIZE (8)
//     HID_REPORT_COUNT(62 ),               // REPORT COUNT (62)
//     HID_LOGICAL_MIN_8(0x00),            // LOGICAL_MINIMUM (00)
//     HID_LOGICAL_MAX_8(0xFF),            // LOGICAL_MAXIMUM (FF)
//     HID_USAGE_8(0x01),                  // USAGE (Generic Value)
//     HID_FEATURE_8(Data_Var_Abs),        // FEATURE (Data Variable Absolute)
//     //Generic Device to Host Packet
//     HID_REPORT_ID(0x27),                // REPORT ID (0x27)
//     HID_REPORT_COUNT(62),               // REPORT COUNT (62)
//     HID_USAGE_8(0x02),                  // USAGE (Generic Value)
//     HID_INPUT_8(Data_Var_Abs),          //INPUT (Data Variable Absolute)

//     Feature 5A CFU feature report
//     HID_REPORT_ID(0x5A),                // REPORT ID (0x5A)
//     HID_REPORT_COUNT(63),               // REPORT COUNT (62)
//     HID_USAGE_8(0x04),                  // USAGE (Generic Value)
//     HID_INPUT_8(Data_Var_Abs),          //INPUT (Data Variable Absolute)

//     Report 5B - in put report
//     HID_REPORT_ID(0x5B),                // REPORT ID (0x5B)
//     HID_REPORT_COUNT(19),               // REPORT COUNT (19)
//     HID_USAGE_8(0x05),                  // USAGE (Generic Value)
//     HID_INPUT_8(Data_Var_Abs),          //INPUT (Data Variable Absolute)

// HID_END_COLLECTION,                  // END_COLLECTION (Application)



#define TEAMS_ASP_HID_DESC_ATTRIB \
    0x06, 0x99, 0xFF,  /* Usage Page (0xFF99)       */ \
          0x09, 0x01,        /* Usage (1)                 */ \
          0xA1, 0x01,        /* Collection (Application)  */ \
          0x09, USAGE_ID_ASP_FEATURE,        /*   Usage (0x03)            */ \
          0x85, REPORT_ID_ASP_FEATURE, /* Report ID (0x9A)*/ \
          0x15, 0x00,        /*   Logical Minimum (0)     */ \
          0x26, 0xFF, 0x00,  /*   Logical Maximum (0xFF)  */ \
          0x75, 0x08,        /*   Report Size (8)         */ \
          0x95, 0x3E,        /*   Report Count (62)       */ \
          0xB2, 0x02, 0x01,  /*   Feature(Data, Variable, Absolute, No Wrap, Linear, Preferred State, No Null Position, Buffered Bytes) */ \
          0x09, USAGE_ID_ASP_INPUT, /*   Usage (0x04)            */ \
          0x85, REPORT_ID_ASP_INPUT, /* Report ID (0x9B)  */ \
          0x15, 0x00,        /*   Logical Minimum (0)     */ \
          0x25, 0x01,        /*   Logical Maximum (1)     */ \
          0x75, 0x01,        /*   Report Size (1)         */ \
          0x95, 0x01,        /*   Report Count (1)        */ \
          0x81, 0x02,        /*   Input(Data, Variable, Relative, No Wrap, Linear, Preferred State, No Null Position, Bit Field) */ \
          0x75, 0x01,        /*  Report Size (1)          */ \
          0x95, 0x0F,        /*   Report Count (15)        */ \
          0x81, 0x01,        /*   Input(Constant, Array, Absolute, No Wrap, Linear, Preferred State, No Null Position, Bit Field) */ \
          0xC0               /* End Collection            */
// Usage Page(0xFF99 )
// Usage(3)
// Collection(Application )
//       Report ID(0x9A )
//       Logical Minimum(0x0 )
//       Logical Maximum(0xFF )
//       Usage Minimum(0x0 )
//       Usage Maximum(0xFF )
//       Report Size(0x8 )
//       Report Count(0x3E)
//       Feature(Data, Variable, Absolute, No Wrap, Linear, Preferred State, No Null Position, Non VolatileBit Field)
//       Report ID(0x9B )
//       Usage(4)
//       Logical Minimum(0x0 )
//       Logical Maximum(0x1 )
//       Report Size(0x1 )
//       Report Count(0x1 )
//       Input(Data, Variable, Relative, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
//       Report Size(0x1 )
//       Report Count(0x7 )
//       Input(Constant, Array, Absolute, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
// End Collection


#define APP_TELEMETRY_HID_DESC_ATTRIB  \
    0x06, 0x00, 0xff,                   /* USAGE_PAGE       Vendor          */ \
          0x0a, 0x39, 0xff,                   /* USAGE                            */ \
          0xa1, 0x01,                         /* COLLECTION        (Application)  */ \
          0x85, REPORT_ID_TELEMETRY_INPUT,    /* REPORT ID         (0x39)         */ \
          0x75, 0x08,                         /* REPORT_SIZE       (8)            */ \
          0x95, 0x07,                         /* REPORT_COUNT      (7)            */ \
          0x09, 0x04,                         /* USAGE             (0x04)         */ \
          0x81, 0x02,                         /* INPUT             (Data,Var,Abs) */ \
          0xc0                                /* END_COLLECTION                   */

// HID_USAGE_PAGE_16(0x00, 0xFF),                               // USAGE PAGE (Vendor 0xFF00)
// HID_USAGE_16(0x39, 0xFF),                                    // USAGE(0xFF39)
// HID_COLLECTION(Application),                              // COLLECTION (Application)
//     HID_REPORT_ID(INPUT_REP_REF_PROPERTY_TELEMETRY_ID),      //REPORT ID (0x39)
//     HID_REPORT_SIZE(0x08),                                   //REPORT SIZE (8)
//     HID_REPORT_COUNT(INPUT_REP_ PROPERTY_TELEMETRY_LEN),     //REPORT COUNT (7)
//     HID_USAGE_8(0x04),                                       //USAGE(Generic Value)
//     HID_INPUT_8(Data_Var_Abs),                               //INPUT (Data Variable Absolute)
// HID_END_COLLECTION,                                          // END_COLLECTION (Application)


#define TELEPHONY_HID_DESC_ATTRIB  \
    0x05, 0x0B,                         /* Usage Page (Telephony)           */ \
          0x09, 0x05,                         /* Usage (Headset)                  */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
          0x85, REPORT_ID_TELEPHONY_INPUT,    /* Report ID (0x0B)                 */ \
          0x05, 0x0B,                         /* Usage Page (Telephony)           */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x01,                         /* Logical Minimum (1)              */ \
          0x09, 0x20,                         /* Usage (Hook Switch)              */ \
          0x09, 0x97,                         /* Usage (Line Busy Tone)           */ \
          0x09, 0x2A,                         /* Usage (Line)                     */ \
          0x09, 0x2F,                         /* Usage (Phone Mute)               */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x04,                         /* Report Count (4)                 */ \
          0x81, 0x23,                         /* Input (Const,Var,Abs,No Wrap,Linear,No Preferred State,No Null Position) */ \
          0x09, 0x21,                         /* Usage (Flash)                    */ \
          0x09, 0x23,                         /* Usage (hold)                    */ \
          0x09, 0x24,                         /* Usage (Redial)               */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x03,                         /* Report Count (3)                 */ \
          0x81, 0x07,                         /* Input (Const,Var,Rel,No Wrap,Linear,Preferred State,No Null Position) */ \
          0x09, 0x06,                         /* Usage (Telephony Key Pad)        */ \
          0xA1, 0x02,                         /*  Collection (Logical)            */ \
          0x19, 0xB0,                         /*   Usage Minimum (Phone Key 0)    */ \
          0x29, 0xBB,                         /*   Usage Maximum (Phone Key Pound)  */ \
          0x15, 0x00,                         /*   Logical Minimum (0)            */ \
          0x25, 0x0C,                         /*   Logical Maximum (12)           */ \
          0x75, 0x04,                         /*   Report Size (4)                */ \
          0x95, 0x01,                         /*   Report Count (1)               */ \
          0x81, 0x40,                         /*   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,Null State) */ \
          0xC0,                               /*  End Collection                  */ \
          0x09, 0x07,                         /* Usage (Programmable Button)      */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x01,                         /* Logical Maximum (1)              */ \
          0x05, 0x09,                         /* Usage Page (Button)              */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x01,                         /* Report Count (1)                 */ \
          0x81, 0x02,                         /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position) */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x04,                         /* Report Count (4)                 */ \
          0x81, 0x01,                         /* Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)  */ \
          0x05, 0x08,                         /* Usage Page (LEDs)                */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x01,                         /* Logical Maximum (1)              */ \
          0x09, 0x17,                         /* Usage (Off-Hook)                 */ \
          0x09, 0x1E,                         /* Usage (Speaker)                  */ \
          0x09, 0x09,                         /* Usage (Mute)                     */ \
          0x09, 0x18,                         /* Usage (Ring)                     */ \
          0x09, 0x20,                         /* Usage (Hold)                     */ \
          0x09, 0x21,                         /* Usage (Microphone)               */ \
          0x09, 0x2A,                         /* Usage (On-Line)                  */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x07,                         /* Report Count (7)                 */ \
          0x91, 0x22,                         /* Output (Data,Var,Abs,No Wrap,Linear,No Preferred State,No Null Position,Non-volatile) */ \
          0x05, 0x0B,                         /* Usage Page (Telephony)           */ \
          0x15, 0x00,                         /* Logical Minimum (0)              */ \
          0x25, 0x01,                         /* Logical Maximum (1)              */ \
          0x09, 0x9E,                         /* Usage (Ringer)                   */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x01,                         /* Report Count (1)                 */ \
          0x91, 0x22,                         /* Output (Data,Var,Abs,No Wrap,Linear,No Preferred State,No Null Position,Non-volatile) */ \
          0x75, 0x01,                         /* Report Size (1)                  */ \
          0x95, 0x08,                         /* Report Count (8)                 */ \
          0x91, 0x01,                         /* Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile) */ \
          0xC0                                /* END_COLLECTION                   */

// #define LED_OUTPUT_HID_DESC_ATTRIB
// Usage Page(Telephony )
// Usage(Headset)
// Collection(Application )
//         Logical Minimum(0x0 )
//         Logical Maximum(0x1 )
//         Report Size(0x1 )
//           Usage Page(LEDs )
//           Report ID(0x9 )
//           Usage(Mute)
//           Usage(Off-Hook)
//           Usage(Ring)
//           Usage(Microphone)
//           Usage(On-Line)
//           Usage(Hold)
//           Report Count(0x6 )
//           Output(Data, Variable, Absolute, No Wrap, Linear, No Preferred, No Null Position, Non VolatileBit Field)
//           Report Count(0x2 )
//          Output(Constant, Array, Absolute, No Wrap, Linear, Preferred State, No Null Position, Non VolatileBit Field)
// End Collection


// #define BUTTON_HOOK_MUTE_HID_DESC_ATTRIB
// Usage Page(Telephony )
// Usage(Headset)
// Collection(Application )
//       Logical Minimum(0x0 )
//       Logical Maximum(0x1 )
//       Report Size(0x1 )
//       Report ID(0x8 )
//       Usage(Hook Switch)
//       Report Count(0x1 )
//       Input(Data, Variable, Absolute, No Wrap, Linear, No Preferred, No Null Position, Bit Field)
//       Usage(Phone Mute)
//       Report Count(0x1 )
//       Input(Data, Variable, Relative, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
//       Usage(Redial)
//       Usage(Flash)
//       Usage(Drop)
//       Report Count(0x3 )
//       Input(Data, Variable, Relative, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
//       Usage(Programmable Button)
//       Usage Page(Button )
//       Usage(Button 1)
//       Report Size(0x1 )
//       Report Count(0x1 )
//       Input(Data, Variable, Absolute, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
//       Report Count(0x2 )
//       Input(Constant, Array, Absolute, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
// End Collection

#define BUTTON_VOL_PLAY_CTRL_HID_DESC_ATTRIB  \
    0x05, 0x0C,                         /* Usage Page (Consumer)            */ \
          0x09, 0x01,                         /* Usage (Consumer Control)         */ \
          0xA1, 0x01,                         /* Collection (Application)         */ \
          0x85, 0x01,                         /* Report ID (1)                    */ \
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
          0xC0                               /* END_COLLECTION                    */
// #define BUTTON_VOL_PLAY_CTRL_HID_DESC_ATTRIB
// Usage Page(Consumer )
// Usage(Consumer Control)
// Collection(Application )
//       Report ID(0x1 )
//       Logical Minimum(0x0 )
//       Logical Maximum(0x1 )
//       Usage(Volume Increment)
//       Usage(Volume Decrement)
//       Usage(Play/Pause)
//       Report Size(0x1 )
//       Report Count(0x3 )
//       Input(Data, Variable, Absolute, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
//       Report Count(0x5 )
//       Usage(Unassigned)
//       Input(Constant, Array, Absolute, No Wrap, Linear, Preferred State, No Null Position, Bit Field)
// End Collection

#define BUTTON_VOL_PLAY_CTRL_HID_DESC_ATTRIB_2  \
    0x05, 0x0C,                             /* Usage Page (Consumer)                                                     */ \
          0x09, 0x01,                             /* Usage (Consumer Control)                                                  */ \
          0xA1, 0x01,                             /* Collection (Application)                                                  */ \
          0x85, REPORT_ID_CONSUMER_HOT_KEY_INPUT, /*   Report ID (2)                                                           */ \
          0x15, 0x00,                             /*   Logical Minimum (0)                                                     */ \
          0x25, 0x01,                             /*   Logical Maximum (1)                                                     */ \
          0x75, 0x01,                             /*   Report Size (1)                                                         */ \
          0x95, 0x01,                             /*   Report Count (1)                                                        */ \
          0x09, 0xE9,                             /*   Usage (Volume Increment)                                                */ \
          0x81, 0x02,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0x09, 0xEA,                             /*   Usage (Volume Decrement)                                                */ \
          0x81, 0x02,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0x09, 0xCD,                             /*   Usage (Play/Pause)                                                      */ \
          0x81, 0x02,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0x09, 0xB5,                             /*   Usage (Scan Next Track)                                                 */ \
          0x81, 0x02,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0x09, 0xB6,                             /*   Usage (Scan Previous Track)                                             */ \
          0x81, 0x02,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0x09, 0xB7,                             /*   Usage (Stop)                                                            */ \
          0x81, 0x02,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0x09, 0xB3,                             /*   Usage (Fast Forward)                                                    */ \
          0x81, 0x02,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0x09, 0xB4,                             /*   Usage (Rewind)                                                          */ \
          0x81, 0x02,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0x09, 0xE2,                             /*   Usage (Mute)                                                            */ \
          0x81, 0x02,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0x95, 0x07,                             /*   Report Count (7)                                                        */ \
          0x81, 0x01,                             /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)    */ \
          0xC0                                   /* End Collection                                                             */


#define MOUSE_HID_DESC_ATTRIB  \
    0x05, 0x01,                     /* Usage Page (Generic Desktop)             */ \
          0x09, 0x02,                     /* Usage (Mouse)                            */ \
          0xA1, 0x01,                     /* Collection (Application)                 */ \
          0x85, REPORT_ID_MOUSE_INPUT,    /* Report ID (3)                            */ \
          0x09, 0x01,                     /* Usage (Pointer)                          */ \
          0xA1, 0x00,                     /* Collection (Physical)                    */ \
          0x05, 0x09,                     /* Usage Page (Buttons)                     */ \
          0x19, 0x01,                     /* Usage Minimum (01)                       */ \
          0x29, 0x05,                     /* Usage Maximum (05)                       */ \
          0x15, 0x00,                     /* Logical Minimum (0)                      */ \
          0x25, 0x01,                     /* Logical Maximum (1)                      */ \
          0x95, 0x05,                     /* Report Count (5)                         */ \
          0x75, 0x01,                     /* Report Size (1)                          */ \
          0x81, 0x02,                     /* Input (Data, Variable, Absolute)         */ \
          0x95, 0x01,                     /* Report Count (1)                         */ \
          0x75, 0x03,                     /* Report Size (3)                          */ \
          0x81, 0x03,                     /* Input (Constant, Variable, Absolute) ;5 bit padding */ \
          0x05, 0x01,                     /* Usage Page (Generic Desktop)             */ \
          0x09, 0x30,                     /* Usage (X)                                */ \
          0x09, 0x31,                     /* Usage (Y)                                */ \
          0x09, 0x38,                     /* Usage (Wheel)                            */ \
          0x15, 0x81,                     /* Logical Minimum (-127)                   */ \
          0x25, 0x7F,                     /* Logical Maximum (127)                    */ \
          0x75, 0x08,                     /* Report Size (8)                          */ \
          0x95, 0x03,                     /* Report Count (3)                         */ \
          0x81, 0x06,                     /* Input (Data, Variable, Relative)         */ \
          0xC0,                           /* End Collection                           */ \
          0xC0                           /* End Collection                           */

#define KEYBOARD_HID_DESC_ATTRIB  \
    0x05, 0x01,                     /* USAGE_PAGE       (Generic Desktop)       */ \
          0x09, 0x06,                     /* USAGE            (Keyboard)              */ \
          0xa1, 0x01,                     /* COLLECTION       (Application)           */ \
          0x85, REPORT_ID_KEYBOARD_FEATURE,    /* REPORT_ID        (1)                */ \
          0x75, 0x01,                     /* REPORT_SIZE      (1)                     */ \
          0x95, 0x08,                     /* REPORT_COUNT     (8)                     */ \
          0x05, 0x07,                     /* USAGE_PAGE       (Keyboard)              */ \
          0x19, 0xe0,                     /* USAGE_MINIMUM    (Keyboard Left Control) */ \
          0x29, 0xe7,                     /* USAGE_MAXIMUM    (Keyboard Right GUI)    */ \
          0x15, 0x00,                     /* LOGICAL_MINIMUM  (0)                     */ \
          0x25, 0x01,                     /* LOGICAL_MAXIMUM  (1)                     */ \
          0x81, 0x02,                     /* input (Data,variable,Abs)                */ \
          0x75, 0x01,                     /* REPORT_SIZE      (1)                     */ \
          0x95, 0x05,                     /* REPORT_COUNT     (5)                     */ \
          0x05, 0x08,                     /* USAGE_PAGE       (LEDs)                  */ \
          0x19, 0x01,                     /* USAGE_MINIMUM    (Num Lock)              */ \
          0x29, 0x05,                     /* USAGE_MAXIMUM    (Kana)                  */ \
          0x91, 0x02,                     /* OUTPUT           (Data,Var,Abs)          */ \
          0x75, 0x03,                     /* REPORT_SIZE      (3)                     */ \
          0x95, 0x01,                     /* REPORT_COUNT     (1)                     */ \
          0x91, 0x03,                     /* OUTPUT           (cons,Var,Abs)          */ \
          0x75, 0x08,                     /* REPORT_SIZE      (8)                     */ \
          0x95, 0x03,                     /* REPORT_COUNT     (6)                     */ \
          0x15, 0x00,                     /* LOGICAL_MINIMUM  (0)                     */ \
          0x25, 0xFF,                     /* LOGICAL_MAXIMUM  (164)  Can be 255       */ \
          0x05, 0x07,                     /* USAGE_PAGE       (Keyboard)              */ \
          0x19, 0x00,                     /* USAGE_MINIMUM    (Reserved-no event indicated) */ \
          0x29, 0xFF,                     /* USAGE_MAXIMUM    (Keyboard Application)  */ \
          0x81, 0x00,                     /* INPUT            (Data,Ary,Abs)          */ \
          0xc0                           /* END_COLLECTION                           */

#define WIRELESS_CONSUMER_HID_DESC_ATTRIB  \
    0x05, 0x0c,                     /* USAGE_PAGE       (Consumer)              */ \
          0x09, 0x01,                     /* USAGE            (Consumer Control)      */ \
          0xa1, 0x01,                     /* COLLECTION       (Application)           */ \
          0x85, REPORT_ID_WIRELESS_CONSUMER,  /* REPORT_ID        (5)                 */ \
          0x15, 0x00,                     /* LOGICAL_MINIMUM  (0)                     */ \
          0x26, 0x80, 0x03,               /* LOGICAL_MAXIMUM  (896)                   */ \
          0x19, 0x00,                     /* USAGE_MINIMUM                            */ \
          0x2a, 0x80, 0x03,               /* USAGE_MAXIMUM                            */ \
          0x75, 0x10,                     /* REPORT_SIZE      (16)                    */ \
          0x95, 0x01,                     /* REPORT_COUNT     (1)                     */ \
          0x81, 0x00,                     /* INPUT            (Data,Ary,Abs)          */ \
          0x95, 0x01,                     /* REPORT_COUNT     (1)  test               */ \
          0x75, 0x10,                     /* REPORT_SIZE      (16)                    */ \
          0x81, 0x03,                     /* input (cons,variable,Abs)                */ \
          0xc0                           /* END_COLLECTION                           */

#define EMOJI_HID_DESC_ATTRIB  \
    0x06, 0x01, 0xff,               /* USAGE_PAGE       (vendor)                */ \
          0x09, 0x01,                     /* USAGE            (vendor)                */ \
          0xa1, 0x01,                     /* COLLECTION       (Application)           */ \
          0x85, REPORT_ID_EMOJI,          /* REPORT_ID        (0x10)                  */ \
          0x19, 0x00,                     /* USAGE_MINIMUM    (0)                     */ \
          0x29, 0xff,                     /* USAGE_MAXIMUM    (0xff)                  */ \
          0x15, 0x00,                     /* LOGICAL_MINIMUM  (0)                     */ \
          0x25, 0xff,                     /* LOGICAL_MAXIMUM  (0xff)                  */ \
          0x75, 0x08,                     /* REPORT_SIZE      (8)                     */ \
          0x95, 0x04,                     /* REPORT_COUNT     (4)                     */ \
          0x81, 0x02,                     /* INPUT            (Data,Var,Abs)          */ \
          0xc0                           /* END_COLLECTION                           */

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

#endif
