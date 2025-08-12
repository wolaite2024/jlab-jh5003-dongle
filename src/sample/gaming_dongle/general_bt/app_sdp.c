/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "app_cfg.h"
#include "bt_types.h"
#include "bt_sdp.h"
#include "btm.h"
#include "gap_br.h"
#include "app_sdp.h"
#include "app_main.h"



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define RTK_COMPANY_ID 0x005D

static const uint8_t did_sdp_record[] =
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
    //0x00,//0x005D : RealTek
    //0x5D,
    (uint8_t)(RTK_COMPANY_ID >> 8),
    (uint8_t)RTK_COMPANY_ID,

    //attribute SDP_ATTR_DIP_PRODUCT_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRODUCT_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_PRODUCT_ID,
    SDP_UNSIGNED_TWO_BYTE,
    0x22,//8763
    0x3B,

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
    0x00,//Bluetooth SIG
    0x01
};

static const uint8_t avrcp_ct_sdp_record[] =
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

static const uint8_t avrcp_tg_sdp_record[] =
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

static const uint8_t hsp_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x39,//0x58,

    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x09,                                   //6bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HEADSET >> 8),       //0x1108
    (uint8_t)(UUID_HEADSET),
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HEADSET_HS >> 8),    //0x1131
    (uint8_t)(UUID_HEADSET_HS),
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
    RFC_HSP_CHANN_NUM,      //0x01

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
    (uint8_t)(0x0102),

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,
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
    */
    //attribute SDP_ATTR_RMT_AUDIO_VOL_CTRL
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_RMT_AUDIO_VOL_CTRL >> 8),
    (uint8_t)SDP_ATTR_RMT_AUDIO_VOL_CTRL,
    SDP_BOOL_ONE_BYTE,                       //0x28
    0x01                                    //True
};

static const uint8_t hfp_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x37,//0x59,

    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x06,                                   //6bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HANDSFREE >> 8), //0x111E
    (uint8_t)(UUID_HANDSFREE),
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
    RFC_HFP_CHANN_NUM,  //0x02

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

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,
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
    (uint8_t)(0x013F >> 8),
    (uint8_t)(0x013F)
};

static const uint8_t a2dp_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x39,//0x55,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AUDIO_SINK >> 8),
    (uint8_t)(UUID_AUDIO_SINK),

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
    /*
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
    */
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
    0x01,//version 1.4
    0x04,

    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES >> 8),
    (uint8_t)SDP_ATTR_SUPPORTED_FEATURES,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,
    0x03
    /*
        //attribute SDP_ATTR_SRV_NAME
        SDP_UNSIGNED_TWO_BYTE,
        (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
        (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
        SDP_STRING_HDR,
        0x09,
        0x61, 0x32, 0x64, 0x70, 0x5f, 0x73, 0x69, 0x6e, 0x6b //a2dp_sink
    */
};

const uint8_t spp_sdp_record[] =
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

static const uint8_t iap_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x4F,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID128_HDR,
    0x00, 0x00, 0x00, 0x00, 0xde, 0xca, 0xfa, 0xde, 0xde, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xca, 0xff,

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
    RFC_IAP_CHANN_NUM,

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

    //attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0C,
    0x57, 0x69, 0x72, 0x65, 0x6c, 0x65, 0x73, 0x73, 0x20, 0x69, 0x41, 0x50 //"Wireless iAP"
};

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
static const uint8_t gfps_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x4F,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID128_HDR,
    0xdf, 0x21, 0xfe, 0x2c, 0x25, 0x15, 0x4f, 0xdb, 0x88, 0x86, 0xf1, 0x2c, 0x4d, 0x67, 0x92, 0x7c,

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
    RFC_GFPS_CHANN_NUM,

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

    //attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x09,
    0x67, 0x66, 0x70, 0x73, 0x20, 0x74, 0x65, 0x73, 0x74 //"gfps test"
};
#endif

#if AMA_FEATURE_SUPPORT
const uint8_t ama_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x5E,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x14,
    SDP_UUID128_HDR,
    0x93, 0x1C, 0x7E, 0x8A, 0x54, 0x0F, 0x46, 0x86, 0xB7, 0x98, 0xE8, 0xDF, 0x0A, 0x2A, 0xD9, 0xF7,
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
    RFC_SPP_AMA_CHANN_NUM,

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
#endif

#if XM_XIAOAI_FEATURE_SUPPORT
static const uint8_t xm_xiaoai_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x5C,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID128_HDR,
    0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x85, 0x84, 0xd0, 0x18, 0x10,

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
    RFC_XM_XIAOAI_CHANN_NUM,

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
    0x09,
    'x', 'm', '_', 'x', 'i', 'a', 'o', 'a', 'i' //"xm_xiaoai"
};
#endif

#if BISTO_FEATURE_SUPPORT

const uint8_t bisto_control_spp_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x5B,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID128_HDR,
    BISTO_BT_CONTROL_SDP_UUID,
    //22


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
    RFC_SPP_BISTO_CTRL_CHANN_NUM,
    //17

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,
    //8

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
    //14

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
    //13

    //attribute SDP_ATTR_SRV_NAME
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET) >> 8),
    (uint8_t)(SDP_ATTR_SRV_NAME + SDP_BASE_LANG_OFFSET),
    SDP_STRING_HDR,
    0x0B,
    0x73, 0x65, 0x72, 0x69, 0x61, 0x6c, 0x20, 0x70, 0x6f, 0x72, 0x74 //"serial port"
};


const uint8_t bisto_audio_spp_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x5B,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x11,
    SDP_UUID128_HDR,
    BISTO_BT_AUDIO_SDP_UUID,

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
    RFC_SPP_BISTO_AU_CHANN_NUM,

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
#endif

void app_sdp_init(void)
{
    bt_sdp_record_add((void *)did_sdp_record);

    if (app_cfg_const.supported_profile_mask & A2DP_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)a2dp_sdp_record);
    }
    if (app_cfg_const.supported_profile_mask & AVRCP_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)avrcp_ct_sdp_record);
        bt_sdp_record_add((void *)avrcp_tg_sdp_record);
    }
    if (app_cfg_const.supported_profile_mask & HFP_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)hfp_sdp_record);
    }
    if (app_cfg_const.supported_profile_mask & HSP_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)hsp_sdp_record);
    }
    if (app_cfg_const.supported_profile_mask & SPP_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)spp_sdp_record);
    }

    if (app_cfg_const.supported_profile_mask & IAP_PROFILE_MASK)
    {
        bt_sdp_record_add((void *)iap_sdp_record);
    }
}

