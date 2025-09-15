/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "app_cfg.h"
#include "bt_types.h"
#include "btm.h"
#include "bt_sdp.h"
#include "gap_br.h"
#include "app_sdp_a2dp_src.h"
#include "app_main.h"
#include "app_vendor_cfg.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VID_SOURCE_BT       0x0001
#define VID_SOURCE_USB      0x0002

#define RTK_COMPANY_ID_BT   0x005D
#define RTK_COMPANY_ID_USB  0x0BDA

#define RTK_PRODUCT_ID      0x875C

static const uint8_t app_customer_sdp_record_audio_hid[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x43,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID128_HDR,
    0x40, 0xbf, 0xe9, 0x57, 0x1d, 0x78, 0x49, 0x6a, 0x8b, 0x50, 0x96, 0xd5, 0xe7, 0xc9, 0x64, 0x60,

    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,

    SDP_UNSIGNED_TWO_BYTE,
    0x02, 0x10,           //attr id
    SDP_DATA_ELEM_SEQ_HDR,
    0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01,
    0x01,
    0x00, 0x00, 0x01, 0xff,
    0x00, 0x00, 0x01, 0xff
};

static const uint8_t app_customer_sdp_record_audio_only[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x43,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID128_HDR,
    0x40, 0xbf, 0xe9, 0x57, 0x1d, 0x78, 0x49, 0x6a, 0x8b, 0x50, 0x96, 0xd5, 0xe7, 0xc9, 0x64, 0x60,

    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,

    SDP_UNSIGNED_TWO_BYTE,
    0x02, 0x10,           //attr id
    SDP_DATA_ELEM_SEQ_HDR,
    0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01,
    0x01,
    0x00, 0x00, 0x00, 0x1f,
    0x00, 0x00, 0x00, 0x79
};

static const uint8_t app_src_did_sdp_record_audio_hid[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x4D,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PNP_INFORMATION >> 8),
    (uint8_t)(UUID_PNP_INFORMATION),

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,

    //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)SDP_LANG_ENGLISH,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)SDP_CHARACTER_UTF8,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)SDP_BASE_LANG_OFFSET,

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PNP_INFORMATION >> 8),
    (uint8_t)UUID_PNP_INFORMATION,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,//version 1.3
    0x03,

    //attribute SDP_ATTR_DIP_SPECIFICATION_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_SPECIFICATION_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_SPECIFICATION_ID,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x03,

    //attribute SDP_ATTR_DIP_VENDOR_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_VENDOR_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_VENDOR_ID,
    SDP_UNSIGNED_TWO_BYTE,
#ifdef DEVICE_CUSTOMER_VID
    (uint8_t)(DEVICE_CUSTOMER_VID >> 8),
    (uint8_t)DEVICE_CUSTOMER_VID,
#else
#ifdef VENDER_ID_SOURCE_USB
    (uint8_t)(RTK_COMPANY_ID_USB >> 8),
    (uint8_t)(RTK_COMPANY_ID_USB),
#else
    (uint8_t)(RTK_COMPANY_ID_BT >> 8),
    (uint8_t)RTK_COMPANY_ID_BT,
#endif
#endif

    //attribute SDP_ATTR_DIP_PRODUCT_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRODUCT_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_PRODUCT_ID,
    SDP_UNSIGNED_TWO_BYTE,
#ifdef DEVICE_CUSTOMER_PID_AUDIO_HID
    (uint8_t)(DEVICE_CUSTOMER_PID_AUDIO_HID >> 8),
    (uint8_t)DEVICE_CUSTOMER_PID_AUDIO_HID,
#else
    (uint8_t)(RTK_PRODUCT_ID >> 8),
    (uint8_t)RTK_PRODUCT_ID,
#endif

    //attribute SDP_ATTR_DIP_PRODUCT_VERSION
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRODUCT_VERSION >> 8),
    (uint8_t)SDP_ATTR_DIP_PRODUCT_VERSION,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,// 1.0.0
    0x00,

    //attribute SDP_ATTR_DIP_PRIMARY_RECORD
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRIMARY_RECORD >> 8),
    (uint8_t)SDP_ATTR_DIP_PRIMARY_RECORD,
    SDP_BOOL_ONE_BYTE,
    true,

    //attribute SDP_ATTR_DIP_VENDOR_ID_SOURCE
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_VENDOR_ID_SOURCE >> 8),
    (uint8_t)SDP_ATTR_DIP_VENDOR_ID_SOURCE,
    SDP_UNSIGNED_TWO_BYTE,
#ifdef VENDER_ID_SOURCE_USB
    (uint8_t)(VID_SOURCE_USB >> 8),//USB SIG
    (uint8_t)VID_SOURCE_USB
#else
    (uint8_t)(VID_SOURCE_BT >> 8),//Bluetooth SIG
    (uint8_t)VID_SOURCE_BT
#endif
};

static const uint8_t app_src_did_sdp_record_audio_only[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x4D,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PNP_INFORMATION >> 8),
    (uint8_t)(UUID_PNP_INFORMATION),

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,

    //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)SDP_LANG_ENGLISH,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)SDP_CHARACTER_UTF8,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)SDP_BASE_LANG_OFFSET,

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PNP_INFORMATION >> 8),
    (uint8_t)UUID_PNP_INFORMATION,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,//version 1.3
    0x03,

    //attribute SDP_ATTR_DIP_SPECIFICATION_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_SPECIFICATION_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_SPECIFICATION_ID,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x03,

    //attribute SDP_ATTR_DIP_VENDOR_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_VENDOR_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_VENDOR_ID,
    SDP_UNSIGNED_TWO_BYTE,
#ifdef DEVICE_CUSTOMER_VID
    (uint8_t)(DEVICE_CUSTOMER_VID >> 8),
    (uint8_t)DEVICE_CUSTOMER_VID,
#else
#ifdef VENDER_ID_SOURCE_USB
    (uint8_t)(RTK_COMPANY_ID_USB >> 8),
    (uint8_t)(RTK_COMPANY_ID_USB),
#else
    (uint8_t)(RTK_COMPANY_ID_BT >> 8),
    (uint8_t)RTK_COMPANY_ID_BT,
#endif
#endif

    //attribute SDP_ATTR_DIP_PRODUCT_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRODUCT_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_PRODUCT_ID,
    SDP_UNSIGNED_TWO_BYTE,
#ifdef DEVICE_CUSTOMER_PID_AUDIO_ONLY
    (uint8_t)(DEVICE_CUSTOMER_PID_AUDIO_ONLY >> 8),
    (uint8_t)DEVICE_CUSTOMER_PID_AUDIO_ONLY,
#else
    (uint8_t)(RTK_PRODUCT_ID >> 8),
    (uint8_t)RTK_PRODUCT_ID,
#endif

    //attribute SDP_ATTR_DIP_PRODUCT_VERSION
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRODUCT_VERSION >> 8),
    (uint8_t)SDP_ATTR_DIP_PRODUCT_VERSION,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,// 1.0.0
    0x00,

    //attribute SDP_ATTR_DIP_PRIMARY_RECORD
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRIMARY_RECORD >> 8),
    (uint8_t)SDP_ATTR_DIP_PRIMARY_RECORD,
    SDP_BOOL_ONE_BYTE,
    true,

    //attribute SDP_ATTR_DIP_VENDOR_ID_SOURCE
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_VENDOR_ID_SOURCE >> 8),
    (uint8_t)SDP_ATTR_DIP_VENDOR_ID_SOURCE,
    SDP_UNSIGNED_TWO_BYTE,
#ifdef VENDER_ID_SOURCE_USB
    (uint8_t)(VID_SOURCE_USB >> 8),//USB SIG
    (uint8_t)VID_SOURCE_USB
#else
    (uint8_t)(VID_SOURCE_BT >> 8),//Bluetooth SIG
    (uint8_t)VID_SOURCE_BT
#endif
};

static const uint8_t a2dp_src_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x38,//0x55,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AUDIO_SOURCE >> 8),
    (uint8_t)(UUID_AUDIO_SOURCE),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVDTP >> 8),
    (uint8_t)(PSM_AVDTP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVDTP >> 8),
    (uint8_t)(UUID_AVDTP),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x03,

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP),

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_ADVANCED_AUDIO_DISTRIBUTION >> 8),
    (uint8_t)(UUID_ADVANCED_AUDIO_DISTRIBUTION),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,//version 1.3
    0x03,

    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES >> 8),
    (uint8_t)SDP_ATTR_SUPPORTED_FEATURES,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,
    0x03
};

#if 0
static const uint8_t avrcp_ct_src_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x3b,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Attribute length: 6 bytes
    //Service Class #0: A/V Remote Control
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER & 0xff),
    //Service Class #1: A/V Remote Control Controller

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 16 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0003 >> 8), //Category 1 Player / Recorder, Category 2
    (uint8_t)(0x0003 & 0xff),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
};

static const uint8_t avrcp_tg_src_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x38,//0x46,//0x5F,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03, //Attribute length: 3 bytes
    //Service Class #0: A/V Remote Control Target
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 16 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0001 >> 8), //Category 1
    (uint8_t)(0x0001 & 0xff),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
};
#endif
static const uint8_t avrcp_ct_src_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x3B,//0x49,//0x62,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Attribute length: 6 bytes
    //Service Class #0: A/V Remote Control
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Service Class #1: A/V Remote Control Controller
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 12 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0001 >> 8), //Category 1 Player / Recorder
    (uint8_t)(0x0001),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
    /*
        //Attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST...it is used for SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
        (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x09,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_LANG_ENGLISH >> 8),
        (uint8_t)SDP_LANG_ENGLISH,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
        (uint8_t)SDP_CHARACTER_UTF8,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
        (uint8_t)SDP_BASE_LANG_OFFSET,

        //Attribute SDP_ATTR_PROVIDER_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x07, //Attribute length: 7 bytes
        0x52, 0x65, 0x61, 0x6C, 0x54, 0x65, 0x6B, //RealTek

        //Attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x08, //Attribute length: 8 bytes
        0x41, 0x56, 0x52, 0x43, 0x50, 0x20, 0x43, 0x54, //AVRCP CT
    */
};

static const uint8_t avrcp_tg_src_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x38,//0x46,//0x5F,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03, //Attribute length: 6 bytes
    //Service Class #0: A/V Remote Control Target
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 12 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0002 >> 8), //Category 2 Amplifier
    (uint8_t)(0x0002),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
    /*
        //Attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST...it is used for SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
        (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x09,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_LANG_ENGLISH >> 8),
        (uint8_t)SDP_LANG_ENGLISH,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
        (uint8_t)SDP_CHARACTER_UTF8,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
        (uint8_t)SDP_BASE_LANG_OFFSET,

        //Attribute SDP_ATTR_PROVIDER_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_PROVIDER_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x07, //Attribute length: 7 bytes
        0x52, 0x65, 0x61, 0x6C, 0x54, 0x65, 0x6B, //RealTek

        //Attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x08, //Attribute length: 8 bytes
        0x41, 0x56, 0x52, 0x43, 0x50, 0x20, 0x54, 0x47, //AVRCP TG
    */
};

const uint8_t app_src_spp_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x4D,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_SERIAL_PORT >> 8),
    (uint8_t)(UUID_SERIAL_PORT),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x0c,
    SDP_DATA_ELEM_SEQ_HDR,
    03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)UUID_L2CAP,
    SDP_DATA_ELEM_SEQ_HDR,
    0x05,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_RFCOMM >> 8),
    (uint8_t)UUID_RFCOMM,
    SDP_UNSIGNED_ONE_BYTE,
    RFC_SPP_CHANN_NUM,

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,

    //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)SDP_LANG_ENGLISH,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)SDP_CHARACTER_UTF8,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)SDP_BASE_LANG_OFFSET,

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_SERIAL_PORT >> 8),
    (uint8_t)UUID_SERIAL_PORT,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,//version 1.2
    0x02,

    //attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0B,
    0x73, 0x65, 0x72, 0x69, 0x61, 0x6c, 0x20, 0x70, 0x6f, 0x72, 0x74 //"serial port"
};

static const uint8_t hsp_ag_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x29,//41bytes,

    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x06,                                   //6bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HEADSET_AUDIO_GATEWAY >> 8),       //0x1112
    (uint8_t)(UUID_HEADSET_AUDIO_GATEWAY),
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_GENERIC_AUDIO >> 8),  //0x1203
    (uint8_t)(UUID_GENERIC_AUDIO),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x0C,                                   //12 bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x03,                               ///3 bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_L2CAP >> 8),     //0x0100
    (uint8_t)(UUID_L2CAP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x05,                               //5 bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_RFCOMM >> 8),    //0x0003
    (uint8_t)(UUID_RFCOMM),
    SDP_UNSIGNED_ONE_BYTE,           //0x08
    RFC_HSP_AG_CHANN_NUM,      //0x12

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x08,                                   //8 bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x06,                               //6 bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HEADSET >> 8),   //0x1108
    (uint8_t)(UUID_HEADSET),
    SDP_UNSIGNED_TWO_BYTE,               //0x09
    (uint8_t)(0x0102 >> 8),         //version number default hs1.2
    (uint8_t)(0x0102)
    /*
        //attribute SDP_ATTR_BROWSE_GROUP_LIST
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
        (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x03,
        SDP_UUID16_HDR,
        (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
        (uint8_t)UUID_PUBLIC_BROWSE_GROUP,
    */
    /*
        //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST...it is used for SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
        (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x09,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_LANG_ENGLISH >> 8),
        (uint8_t)SDP_LANG_ENGLISH,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
        (uint8_t)SDP_CHARACTER_UTF8,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
        (uint8_t)SDP_BASE_LANG_OFFSET,

        //attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,                             //0x25 text string
        0x0C,                               //12 bytes
        0x48, 0x65, 0x61, 0x64, 0x73, 0x65, 0x74, 0x20, 0x75, 0x6e,
        0x69, 0x74, //"Headset unit"
    */                                //True
};

static const uint8_t hfp_ag_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x2f,//47 bytes belowed.

    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x06,                                   //6bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HANDSFREE_AUDIO_GATEWAY >> 8), //0x111F
    (uint8_t)(UUID_HANDSFREE_AUDIO_GATEWAY),
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_GENERIC_AUDIO >> 8),  //0x1203
    (uint8_t)(UUID_GENERIC_AUDIO),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x0C,                                   //12bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x03,                               //3bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_L2CAP >> 8),     //0x0100
    (uint8_t)(UUID_L2CAP),
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x05,                               //5bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_RFCOMM >> 8),   //0x0003
    (uint8_t)(UUID_RFCOMM),
    SDP_UNSIGNED_ONE_BYTE,           //0x08
    RFC_HFP_AG_CHANN_NUM,  //0x11

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x08,                                   //8 bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x06,                               //6 bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_HANDSFREE >> 8), //0x111E
    (uint8_t)(UUID_HANDSFREE),
    SDP_UNSIGNED_TWO_BYTE,           //0x09
    (uint8_t)(0x0109 >> 8),     //version number default hf1.9
    (uint8_t)(0x0109),
    /*
        //attribute SDP_ATTR_BROWSE_GROUP_LIST
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
        (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x03,
        SDP_UUID16_HDR,
        (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
        (uint8_t)UUID_PUBLIC_BROWSE_GROUP,
    */
    /*
        //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST...it is used for SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
        (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x09,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_LANG_ENGLISH >> 8),
        (uint8_t)SDP_LANG_ENGLISH,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
        (uint8_t)SDP_CHARACTER_UTF8,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
        (uint8_t)SDP_BASE_LANG_OFFSET,

        //attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,                             //0x25 text string
        0x0F,                                   //15 bytes
        0x48, 0x61, 0x6e, 0x64, 0x73, 0x2d, 0x66, 0x72, 0x65, 0x65,
        0x20, 0x75, 0x6e, 0x69, 0x74, //"Hands-free unit"
    */
    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x012F >> 8),
    (uint8_t)(0x012F)
};

#ifdef  BTDONGLE_BQB_MODE_ENABLE
static const uint8_t hfp_ag_bqb_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x34,//47 bytes belowed.

    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x06,                                   //6bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HANDSFREE_AUDIO_GATEWAY >> 8), //0x111F
    (uint8_t)(UUID_HANDSFREE_AUDIO_GATEWAY),
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_GENERIC_AUDIO >> 8),  //0x1203
    (uint8_t)(UUID_GENERIC_AUDIO),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x0C,                                   //12bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x03,                               //3bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_L2CAP >> 8),     //0x0100
    (uint8_t)(UUID_L2CAP),
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x05,                               //5bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_RFCOMM >> 8),   //0x0003
    (uint8_t)(UUID_RFCOMM),
    SDP_UNSIGNED_ONE_BYTE,           //0x08
    RFC_HFP_AG_CHANN_NUM,  //0x11

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x08,                                   //8 bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x06,                               //6 bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_HANDSFREE >> 8), //0x111E
    (uint8_t)(UUID_HANDSFREE),
    SDP_UNSIGNED_TWO_BYTE,           //0x09
    (uint8_t)(0x0107 >> 8),     //version number default hf1.7
    (uint8_t)(0x0107),
    /*
        //attribute SDP_ATTR_BROWSE_GROUP_LIST
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
        (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x03,
        SDP_UUID16_HDR,
        (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
        (uint8_t)UUID_PUBLIC_BROWSE_GROUP,
    */
    /*
        //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST...it is used for SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
        (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
        SDP_DATA_ELEM_SEQ_HDR,
        0x09,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_LANG_ENGLISH >> 8),
        (uint8_t)SDP_LANG_ENGLISH,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
        (uint8_t)SDP_CHARACTER_UTF8,
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
        (uint8_t)SDP_BASE_LANG_OFFSET,

        //attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,                             //0x25 text string
        0x0F,                                   //15 bytes
        0x48, 0x61, 0x6e, 0x64, 0x73, 0x2d, 0x66, 0x72, 0x65, 0x65,
        0x20, 0x75, 0x6e, 0x69, 0x74, //"Hands-free unit"
    */
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_EXT_NETWORK) >> 8),
    (uint8_t)(SDP_ATTR_EXT_NETWORK),
    SDP_UNSIGNED_ONE_BYTE,
    (uint8_t)(0x01),
    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0028 >> 8),
    (uint8_t)(0x0028)
};
#endif

void app_sdp_a2dp_src_init(void)
{
    if (app_vendor_cfg_is_audio_hid())
    {
        bt_sdp_record_add((void *)app_src_did_sdp_record_audio_hid);
    }
    else
    {
        bt_sdp_record_add((void *)app_src_did_sdp_record_audio_only);
    }

    if (app_vendor_cfg_is_audio_hid())
    {
        bt_sdp_record_add((void *)app_customer_sdp_record_audio_hid);
    }
    else
    {
        bt_sdp_record_add((void *)app_customer_sdp_record_audio_only);
    }

    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)a2dp_src_sdp_record);
    }

    if (app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)app_src_spp_sdp_record);
    }
#ifdef  BTDONGLE_BQB_MODE_ENABLE
    extern bool app_is_bqb_mode();
    if (app_is_bqb_mode())
    {
        gap_br_reg_sdp_record((void *)hfp_ag_bqb_sdp_record);
    }
    else
#else
    {
        bt_sdp_record_add((void *)hsp_ag_sdp_record);
        bt_sdp_record_add((void *)hfp_ag_sdp_record);
    }
#endif

        APP_PRINT_INFO1("supported_profile_mask %08x", app_cfg_const.supported_profile_mask);
    if (app_cfg_const.supported_profile_mask & AVRCP_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)avrcp_ct_src_sdp_record);
        bt_sdp_record_add((void *)avrcp_tg_src_sdp_record);
    }
}
