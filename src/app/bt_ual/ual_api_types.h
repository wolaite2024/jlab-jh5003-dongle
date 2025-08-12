/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __UAL_API_TYPES_H__
#define __UAL_API_TYPES_H__

/****************************
 * minor device class field
 ****************************/

/* 0x00 is used as unclassified for all minor device classes */
#define BTM_COD_MINOR_UNCLASSIFIED 0x00

/* minor device class field for Computer Major Class */
/* #define BTM_COD_MINOR_UNCLASSIFIED       0x00    */
#define BTM_COD_MINOR_DESKTOP_WORKSTATION 0x04
#define BTM_COD_MINOR_SERVER_COMPUTER 0x08
#define BTM_COD_MINOR_LAPTOP 0x0C
#define BTM_COD_MINOR_HANDHELD_PC_PDA 0x10 /* clam shell */
#define BTM_COD_MINOR_PALM_SIZE_PC_PDA 0x14
#define BTM_COD_MINOR_WEARABLE_COMPUTER 0x18 /* watch sized */

/* minor device class field for Phone Major Class */
/* #define BTM_COD_MINOR_UNCLASSIFIED       0x00    */
#define BTM_COD_MINOR_CELLULAR 0x04
#define BTM_COD_MINOR_CORDLESS 0x08
#define BTM_COD_MINOR_SMART_PHONE 0x0C
/* wired modem or voice gatway */
#define BTM_COD_MINOR_WIRED_MDM_V_GTWY 0x10
#define BTM_COD_MINOR_ISDN_ACCESS 0x14

/* minor device class field for LAN Access Point Major Class */
/* Load Factor Field bit 5-7 */
#define BTM_COD_MINOR_FULLY_AVAILABLE 0x00
#define BTM_COD_MINOR_1_17_UTILIZED 0x20
#define BTM_COD_MINOR_17_33_UTILIZED 0x40
#define BTM_COD_MINOR_33_50_UTILIZED 0x60
#define BTM_COD_MINOR_50_67_UTILIZED 0x80
#define BTM_COD_MINOR_67_83_UTILIZED 0xA0
#define BTM_COD_MINOR_83_99_UTILIZED 0xC0
#define BTM_COD_MINOR_NO_SERVICE_AVAILABLE 0xE0
/* sub-Field bit 2-4 */
/* #define BTM_COD_MINOR_UNCLASSIFIED       0x00    */

/* minor device class field for Audio/Video Major Class */
/* #define BTM_COD_MINOR_UNCLASSIFIED       0x00    */
#define BTM_COD_MINOR_CONFM_HEADSET 0x04
#define BTM_COD_MINOR_CONFM_HANDSFREE 0x08
#define BTM_COD_MINOR_MICROPHONE 0x10
#define BTM_COD_MINOR_LOUDSPEAKER 0x14
#define BTM_COD_MINOR_HEADPHONES 0x18
#define BTM_COD_MINOR_PORTABLE_AUDIO 0x1C
#define BTM_COD_MINOR_CAR_AUDIO 0x20
#define BTM_COD_MINOR_SET_TOP_BOX 0x24
#define BTM_COD_MINOR_HIFI_AUDIO 0x28
#define BTM_COD_MINOR_VCR 0x2C
#define BTM_COD_MINOR_VIDEO_CAMERA 0x30
#define BTM_COD_MINOR_CAMCORDER 0x34
#define BTM_COD_MINOR_VIDEO_MONITOR 0x38
#define BTM_COD_MINOR_VIDDISP_LDSPKR 0x3C
#define BTM_COD_MINOR_VIDEO_CONFERENCING 0x40
#define BTM_COD_MINOR_GAMING_TOY 0x48

/* minor device class field for Peripheral Major Class */
/* Bits 6-7 independently specify mouse, keyboard, or combo mouse/keyboard */
#define BTM_COD_MINOR_KEYBOARD 0x40
#define BTM_COD_MINOR_POINTING 0x80
#define BTM_COD_MINOR_COMBO 0xC0
/* Bits 2-5 OR'd with selection from bits 6-7 */
/* #define BTM_COD_MINOR_UNCLASSIFIED       0x00    */
#define BTM_COD_MINOR_JOYSTICK 0x04
#define BTM_COD_MINOR_GAMEPAD 0x08
#define BTM_COD_MINOR_REMOTE_CONTROL 0x0C
#define BTM_COD_MINOR_SENSING_DEVICE 0x10
#define BTM_COD_MINOR_DIGITIZING_TABLET 0x14
#define BTM_COD_MINOR_CARD_READER 0x18 /* e.g. SIM card reader */
#define BTM_COD_MINOR_DIGITAL_PAN 0x1C
#define BTM_COD_MINOR_HAND_SCANNER 0x20
#define BTM_COD_MINOR_HAND_GESTURAL_INPUT 0x24

/* minor device class field for Imaging Major Class */
/* Bits 5-7 independently specify display, camera, scanner, or printer */
#define BTM_COD_MINOR_DISPLAY 0x10
#define BTM_COD_MINOR_CAMERA 0x20
#define BTM_COD_MINOR_SCANNER 0x40
#define BTM_COD_MINOR_PRINTER 0x80
/* Bits 2-3 Reserved */
/* #define BTM_COD_MINOR_UNCLASSIFIED       0x00    */

/* minor device class field for Wearable Major Class */
/* Bits 2-7 meaningful    */
#define BTM_COD_MINOR_WRIST_WATCH 0x04
#define BTM_COD_MINOR_PAGER 0x08
#define BTM_COD_MINOR_JACKET 0x0C
#define BTM_COD_MINOR_HELMET 0x10
#define BTM_COD_MINOR_GLASSES 0x14

/* minor device class field for Toy Major Class */
/* Bits 2-7 meaningful    */
#define BTM_COD_MINOR_ROBOT 0x04
#define BTM_COD_MINOR_VEHICLE 0x08
#define BTM_COD_MINOR_DOLL_ACTION_FIGURE 0x0C
#define BTM_COD_MINOR_CONTROLLER 0x10
#define BTM_COD_MINOR_GAME 0x14

/* minor device class field for Health Major Class */
/* Bits 2-7 meaningful    */
#define BTM_COD_MINOR_BLOOD_MONITOR 0x04
#define BTM_COD_MINOR_THERMOMETER 0x08
#define BTM_COD_MINOR_WEIGHING_SCALE 0x0C
#define BTM_COD_MINOR_GLUCOSE_METER 0x10
#define BTM_COD_MINOR_PULSE_OXIMETER 0x14
#define BTM_COD_MINOR_HEART_PULSE_MONITOR 0x18
#define BTM_COD_MINOR_HEALTH_DATA_DISPLAY 0x1C
#define BTM_COD_MINOR_STEP_COUNTER 0x20
#define BTM_COD_MINOR_BODY_COM_ANALYZER 0x24
#define BTM_COD_MINOR_PEAK_FLOW_MONITOR 0x28
#define BTM_COD_MINOR_MEDICATION_MONITOR 0x2C
#define BTM_COD_MINOR_KNEE_PROSTHESIS 0x30
#define BTM_COD_MINOR_ANKLE_PROSTHESIS 0x34

/***************************
 * major device class field
 ***************************/
#define BTM_COD_MAJOR_MISCELLANEOUS 0x00
#define BTM_COD_MAJOR_COMPUTER 0x01
#define BTM_COD_MAJOR_PHONE 0x02
#define BTM_COD_MAJOR_LAN_ACCESS_PT 0x03
#define BTM_COD_MAJOR_AUDIO 0x04
#define BTM_COD_MAJOR_PERIPHERAL 0x05
#define BTM_COD_MAJOR_IMAGING 0x06
#define BTM_COD_MAJOR_WEARABLE 0x07
#define BTM_COD_MAJOR_TOY 0x08
#define BTM_COD_MAJOR_HEALTH 0x09
#define BTM_COD_MAJOR_UNCLASSIFIED 0x1F

/***************************
 * service class fields
 ***************************/
#define BTM_COD_SERVICE_LMTD_DISCOVER 0x0020
#define BTM_COD_SERVICE_POSITIONING 0x0100
#define BTM_COD_SERVICE_NETWORKING 0x0200
#define BTM_COD_SERVICE_RENDERING 0x0400
#define BTM_COD_SERVICE_CAPTURING 0x0800
#define BTM_COD_SERVICE_OBJ_TRANSFER 0x1000
#define BTM_COD_SERVICE_AUDIO 0x2000
#define BTM_COD_SERVICE_TELEPHONY 0x4000
#define BTM_COD_SERVICE_INFORMATION 0x8000



#define UUID_SERVCLASS_IMMEDIATE_ALERT 0x1802 /* immediate alert */
#define UUID_SERVCLASS_LINKLOSS 0x1803        /* Link Loss Alert */
#define UUID_SERVCLASS_TX_POWER 0x1804        /* TX power */
#define UUID_SERVCLASS_CURRENT_TIME 0x1805    /* Link Loss Alert */
#define UUID_SERVCLASS_DST_CHG 0x1806         /* DST Time change */
#define UUID_SERVCLASS_REF_TIME_UPD 0x1807    /* reference time update */
#define UUID_SERVCLASS_THERMOMETER 0x1809     /* Thermometer UUID */
#define UUID_SERVCLASS_DEVICE_INFO 0x180A     /* device info service */
#define UUID_SERVCLASS_NWA 0x180B             /* Network availability */
#define UUID_SERVCLASS_HEART_RATE 0x180D      /* Heart Rate service */
#define UUID_SERVCLASS_PHALERT 0x180E         /* phone alert service */
#define UUID_SERVCLASS_BATTERY 0x180F         /* battery service */
#define UUID_SERVCLASS_BPM 0x1810             /*  blood pressure service */
#define UUID_SERVCLASS_ALERT_NOTIFICATION 0x1811
#define UUID_SERVCLASS_LE_HID 0x1812     /*  HID over LE */
#define UUID_SERVCLASS_SCAN_PARAM 0x1813 /* Scan Parameter service */
#define UUID_SERVCLASS_GLUCOSE 0x1808    /* Glucose Meter Service */
#define UUID_SERVCLASS_RSC 0x1814 /* RUNNERS SPEED AND CADENCE SERVICE      */
#define UUID_SERVCLASS_CSC 0x1816 /* Cycling SPEED AND CADENCE SERVICE      */


/* connection parameter boundary values */
#define BLE_SCAN_INT_MIN 0x0004
#define BLE_SCAN_INT_MAX 0x4000
#define BLE_SCAN_WIN_MIN 0x0004
#define BLE_SCAN_WIN_MAX 0x4000
#define BLE_EXT_SCAN_INT_MAX 0x00FFFFFF
#define BLE_EXT_SCAN_WIN_MAX 0xFFFF
#define BLE_CONN_INT_MIN 0x0006
#define BLE_CONN_INT_MAX 0x0C80
#define BLE_CONN_LATENCY_MAX 500
#define BLE_CONN_SUP_TOUT_MIN 0x000A
#define BLE_CONN_SUP_TOUT_MAX 0x0C80
/* use this value when a specific value not to be overwritten */
#define BLE_CONN_PARAM_UNDEF 0xffff
#define BLE_SCAN_PARAM_UNDEF 0xffff

/* default connection parameters if not configured, use GAP recommended value
 * for auto/selective connection */
#if 0
/* default scan interval */
#define BLE_SCAN_FAST_INT 96 /* 30 ~ 60 ms (use 60)  = 96 *0.625 */
#else
#define BLE_SCAN_FAST_INT 80 /* 30 ~ 60 ms (use 50)  = 80 *0.625 */
#endif

/* default scan window for background connection, applicable for auto connection
 * or selective connection */
#define BLE_SCAN_FAST_WIN 48 /* 30 ms = 48 *0.625 */

/* default scan paramter used in reduced power cycle (background scanning) */
#define BLE_SCAN_SLOW_INT_1 144 /* 90ms   = 144 *0.625 */
#define BLE_SCAN_SLOW_WIN_1 80 /* 50 ms = 80 *0.625 */

/* default scan paramter used in reduced power cycle (background scanning) */
#define BLE_SCAN_SLOW_INT_2 2048 /* 1.28 s   = 2048 *0.625 */
#define BLE_SCAN_SLOW_WIN_2 48 /* 30 ms = 48 *0.625 */

#define BLE_CONN_FAST_CI_DEF    (0x0006)
#define BLE_CONN_GAMING_CI_DEF  (0x0020)
#define BLE_CONN_NORMAL_CI_DEF  (0x0020)

/* default connection interval min */
/* recommended min: 30ms  = 24 * 1.25 */
#define BLE_CONN_INT_MIN_DEF 24

/* default connectino interval max */
/* recommended max: 50 ms = 56 * 1.25 */
#define BLE_CONN_INT_MAX_DEF 40

/* default slave latency */
#define BLE_CONN_SLAVE_LATENCY_DEF 0 /* 0 */

/* default supervision timeout */
#define BLE_CONN_TIMEOUT_DEF 500

/* minimum supervision timeout */
#define BLE_CONN_TIMEOUT_MIN_DEF 100

/* minimum acceptable connection interval */
#define BLE_CONN_INT_MIN_LIMIT 0x0009

/*  most significant bit, bit7, bit6 is 01 to be resolvable random */
#define BLE_RESOLVE_ADDR_MSB 0x40
/* bit 6, and bit7 */
#define BLE_RESOLVE_ADDR_MASK 0xc0

#define GAP_ADTYPE_FLAGS                        0x01 //!< The Flags data type contains one bit Boolean flags. Please reference @ref ADV_TYPE_FLAGS for details.
#define GAP_ADTYPE_16BIT_MORE                   0x02 //!< Service: More 16-bit UUIDs available
#define GAP_ADTYPE_16BIT_COMPLETE               0x03 //!< Service: Complete list of 16-bit UUIDs
#define GAP_ADTYPE_32BIT_MORE                   0x04 //!< Service: More 32-bit UUIDs available
#define GAP_ADTYPE_32BIT_COMPLETE               0x05 //!< Service: Complete list of 32-bit UUIDs
#define GAP_ADTYPE_128BIT_MORE                  0x06 //!< Service: More 128-bit UUIDs available
#define GAP_ADTYPE_128BIT_COMPLETE              0x07 //!< Service: Complete list of 128-bit UUIDs
#define GAP_ADTYPE_LOCAL_NAME_SHORT             0x08 //!< Shortened local name
#define GAP_ADTYPE_LOCAL_NAME_COMPLETE          0x09 //!< Complete local name
#define GAP_ADTYPE_POWER_LEVEL                  0x0A //!< TX Power Level: 0xXX: -127 to +127 dBm
#define GAP_ADTYPE_OOB_CLASS_OF_DEVICE          0x0D //!< Simple Pairing OOB Tag: Class of device (3 octets)
#define GAP_ADTYPE_OOB_SIMPLE_PAIRING_HASHC     0x0E //!< Simple Pairing OOB Tag: Simple Pairing Hash C (16 octets)
#define GAP_ADTYPE_OOB_SIMPLE_PAIRING_RANDR     0x0F //!< Simple Pairing OOB Tag: Simple Pairing Randomizer R (16 octets)
#define GAP_ADTYPE_SM_TK                        0x10 //!< Security Manager TK Value
#define GAP_ADTYPE_SM_OOB_FLAG                  0x11 //!< Secutiry Manager OOB Flags
#define GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE    0x12 //!< Min and Max values of the connection interval (2 octets Min, 2 octets Max) (0xFFFF indicates no conn interval min or max)
#define GAP_ADTYPE_SIGNED_DATA                  0x13 //!< Signed Data field
#define GAP_ADTYPE_SERVICES_LIST_16BIT          0x14 //!< Service Solicitation: list of 16-bit Service UUIDs
#define GAP_ADTYPE_SERVICES_LIST_128BIT         0x15 //!< Service Solicitation: list of 128-bit Service UUIDs
#define GAP_ADTYPE_SERVICE_DATA                 0x16 //!< Service Data
#define GAP_ADTYPE_PUBLIC_TGT_ADDR              0x17 //!< Public Target Address
#define GAP_ADTYPE_RANDOM_TGT_ADDR              0x18 //!< Random Target Address
#define GAP_ADTYPE_APPEARANCE                   0x19 //!< Appearance
#define GAP_ADTYPE_ADV_INTERVAL                 0x1A //!< Advertising Interval
#define GAP_ADTYPE_LE_BT_ADDR                   0x1B //!< LE Bluetooth Device Address
#define GAP_ADTYPE_LE_ROLE                      0x1C //!< LE Role
#define GAP_ADTYPE_SP_HASH_C256                 0x1D //!< Simple Pairing Hash C-256
#define GAP_ADTYPE_SP_RAND_R256                 0x1E //!< Simple Pairing Randomizer R-256
#define GAP_ADTYPE_LIST_32BIT_SILI              0x1F //!< List of 32-bit Service Solicitation UUIDs
#define GAP_ADTYPE_SERVICE_DATA_32BIT           0x20 //!< Service Data - 32-bit UUID
#define GAP_ADTYPE_SERVICE_DATA_128BIT          0x21 //!< Service Data - 128-bit UUID
#define GAP_ADTYPE_SC_CONF_VALUE                0x22 //!< LE Secure Connections Confirmation Value
#define GAP_ADTYPE_SC_RAND_VALUE                0x23 //!< LE Secure Connections Random Value
#define GAP_ADTYPE_URI                          0x24 //!< URI
#define GAP_ADTYPE_INDOOR_POSITION              0x25 //!< Indoor Positioning
#define GAP_ADTYPE_TRANSPORT_DISCOVERY_DATA     0x26 //!< Transport Discovery Data
#define GAP_ADTYPE_LE_SUPPORTED_FEATURES        0x27 //!< LE Supported Features
#define GAP_ADTYPE_CHAN_MAP_UPDATE_IND          0x28 //!< Channel Map Update Indication
#define GAP_ADTYPE_MESH_PB_ADV                  0x29 //!< Mesh Pb-Adv
#define GAP_ADTYPE_MESH_PACKET                  0x2A //!< Mesh Packet
#define GAP_ADTYPE_MESH_BEACON                  0x2B //!< Mesh Beacon
#define GAP_ADTYPE_3D_INFO_DATA                 0x3D //!< 3D Information Data
#define GAP_ADTYPE_MANUFACTURER_SPECIFIC        0xFF //!< Manufacturer Specific Data: first 2 octets contain the Company Identifier Code followed by the additional manufacturer specific data

static inline bool BLE_IS_RESOLVE_BDA(uint8_t x[6])
{
    return (x[5] & BLE_RESOLVE_ADDR_MASK) == BLE_RESOLVE_ADDR_MSB;
}

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UAL_API_TYPES_H__ */
