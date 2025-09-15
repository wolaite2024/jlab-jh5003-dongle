/**
*****************************************************************************************
*     Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     app_cfu.h
  * @brief
  * @details
  * @author   michael_lin
  * @date     2020-9-1
  * @version  v1.0
  * *************************************************************************************
  */

#ifndef _APP_COMMON_CFU_H_
#define _APP_COMMON_CFU_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "app_cfu_transfer.h"

#if F_APP_TEAMS_CFU_SUPPORT
#include "CfuInfoStructs.h"
#endif
#if (F_APP_CFU_HID_SUPPORT || F_APP_TEAMS_HID_SUPPORT || F_APP_CFU_SPP_SUPPORT || F_APP_CFU_BLE_CHANNEL_SUPPORT)
#include "app_link_util.h"
#endif

/** @defgroup APP_COMMON_CFU APP COMMON CFU
    * @brief app common cfu api
    * @{
    */

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup APP_COMMON_CFU_Exported_Macros APP COMMON CFU Exported Macros
    * @brief
    * @{
    */

#define MS_CFU_IMAGE_LOCATION_BANK0            0
#define MS_CFU_IMAGE_LOCATION_BANK1            1
#define MS_CFU_NOT_SUPPORT_BANK_SWITCH         3

#define MAX_ACCEPTED_OFFER_NUM                 2

#define OFFER_PROCESS_NOT_GOT           0
#define OFFER_PROCESS_SKIPPED           1
#define OFFER_PROCESS_REJECTED          2
#define OFFER_PROCESS_ACCEPTED          3

#define CFU_CONTENT_DATA_LENGTH         52

// NOTE - defines should match CFU Protocol Spec definitions
#define CFU_OFFER_METADATA_INFO_CMD                        (0xFF)
#define CFU_SPECIAL_OFFER_CMD                              (0xFE)
#define CFU_SPECIAL_OFFER_GET_STATUS                       (0x03)
#define CFU_SPECIAL_OFFER_NONCE                            (0x02)
#define CFU_SPECIAL_OFFER_NOTIFY_ON_READY                  (0x01)
#define CFW_UPDATE_PACKET_MAX_LENGTH                       (sizeof(FW_UPDATE_CONTENT_COMMAND))
#define FIRMWARE_OFFER_TOKEN_DRIVER                        (0xA0)
#define FIRMWARE_OFFER_TOKEN_SPEEDFLASHER                  (0xB0)
#define FIRMWARE_UPDATE_FLAG_FIRST_BLOCK                   (0x80)
#define FIRMWARE_UPDATE_FLAG_LAST_BLOCK                    (0x40)
#define FIRMWARE_UPDATE_FLAG_VERIFY                        (0x08)
#define FIRMWARE_UPDATE_STATUS_ERROR_COMPLETE              (0x03)
#define FIRMWARE_UPDATE_STATUS_ERROR_CRC                   (0x05)
#define FIRMWARE_UPDATE_STATUS_ERROR_INVALID               (0x0B)
#define FIRMWARE_UPDATE_STATUS_ERROR_INVALID_ADDR          (0x09)
#define FIRMWARE_UPDATE_STATUS_ERROR_NO_OFFER              (0x0A)
#define FIRMWARE_UPDATE_STATUS_ERROR_PENDING               (0x08)
#define FIRMWARE_UPDATE_STATUS_ERROR_PREPARE               (0x01)
#define FIRMWARE_UPDATE_STATUS_ERROR_SIGNATURE             (0x06)
#define FIRMWARE_UPDATE_STATUS_ERROR_VERIFY                (0x04)
#define FIRMWARE_UPDATE_STATUS_ERROR_VERSION               (0x07)
#define FIRMWARE_UPDATE_STATUS_ERROR_WRITE                 (0x02)
#define FIRMWARE_UPDATE_STATUS_SUCCESS                     (0x00)

#define OFFER_INFO_START_ENTIRE_TRANSACTION                (0x00)
#define OFFER_INFO_START_OFFER_LIST                        (0x01)
#define OFFER_INFO_END_OFFER_LIST                          (0x02)

#define APP_TEAMS_CFU_SUPPORT_EXTERNAL_DSP      0
#define CFU_PLATFORM_ID_OFFSET 1022
#define CFU_COMPONENT_ID_OFFSET 1010

#if F_APP_TEAMS_CFU_SUPPORT
#define APP_TEAMS_SUPPORT_SIGNED_IMAGE          1
#define  CFU_MILESTONE_NUM  6
#define  CFU_PLATFORM_ID    0x4C
#define  CFU_VARIANT_MASK   1
#define  CFU_PRI_COMPONENT_ID   0x10
#define  CFU_DSP_COMPONENT_ID   0x11
#define  CFU_SEC_COMPONENT_ID   0x12
#define  CFU_MILESTONE_COUNT  2
#else
#define APP_TEAMS_SUPPORT_SIGNED_IMAGE          0
#define  CFU_MILESTONE_NUM  0
#define  CFU_PLATFORM_ID    0
#define  CFU_PRI_COMPONENT_ID      0xa1
#define  CFU_DONGLE_COMPONENT_ID   0xa2
#endif

#define BUILD_TYPE_DEBUG                                   0
#define BUILD_TYPE_RELEASE                                 2

#define BUILD_SIGNING_TEST                                 1
#define BUILD_SIGNING_MSFT                                 3

#define BUILD_VERSION_IGNORE                               0
#define BUILD_VERSION_HIGH                                 1
#define BUILD_VERSION_SAME                                 2
#define BUILD_VERSION_LOW                                  3

#define PACKET_SIZE         0x34
#define CFU_HEADER_CONTENT_LEN  (sizeof(T_CFU_HEADER) + 1) / PACKET_SIZE * PACKET_SIZE
#define CFU_HEADER_LEN      0x66c
#define CFU_SINATURE_LEN   CFU_HEADER_LEN - sizeof(CFU_HEADER) - sizeof(CFU_INFO) - sizeof(CFU_HW_COMPAT) - sizeof(CFU_PKI)

#define  CFU_PROTOCOL_VER   4
#define  CFU_SFUA_VER       0

#if APP_TEAMS_CFU_SUPPORT_EXTERNAL_DSP
#define  CFU_COMPONENT_COUNT  0x2
#else
#define  CFU_COMPONENT_COUNT  0x1
#endif

#define MAX_BUFFER_SIZE     4096

#define CFU_HEADER_OFFSET   0

#define REPORT_ID_LENGTH    0x01

#define CFU_OFFER_RESPONSE_LENGTH_BYTES             16
#define HID_CFU_OFFER_RESPONSE_LENGTH_BYTES         (CFU_OFFER_RESPONSE_LENGTH_BYTES + REPORT_ID_LENGTH)

#define CFU_CONTENT_RESPONSE_LENGTH_BYTES           16
#define HID_CFU_CONTENT_RESPONSE_LENGTH_BYTES       (CFU_CONTENT_RESPONSE_LENGTH_BYTES + REPORT_ID_LENGTH)

/** End of APP_COMMON_CFU_Exported_Macros
    * @}
    */

typedef enum t_cfu_rsp_type
{
    CFU_CUR_COMPONENT_RSP,
    CFU_OTH_COMPONENT_RSP,
} T_CFU_RSP_TYPE;

typedef union t_cfu_flag
{
    uint8_t value;
    struct
    {
        uint8_t cfu_in_process: 1;
        uint8_t bank_swap_pending: 1;
        uint8_t force_reset: 1;
        uint8_t ignore_ver: 1;
        uint8_t is_sec: 1;
        uint8_t rsvd: 3;
    };
} T_CFU_FLAG;

typedef enum t_component_firmware_update
{
    COMPONENT_FIRMWARE_UPDATE_FLAG_DEFAULT = 0x00,
    COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK = 0x80,        // Denotes the first block of a firmware payload.
    COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK = 0x40,         // Denotes the last block of a firmware payload.
    COMPONENT_FIRMWARE_UPDATE_FLAG_VERIFY = 0x08,             // If set, the firmware verifies the byte array in the upper block at the specified address.
} T_COMPONENT_FIRMWARE_UPDATE;

typedef enum t_fw_offer_status
{
    // The offer needs to be skipped at this time indicating to
    // the host to please offer again during next applicable period.
    FIRMWARE_UPDATE_OFFER_SKIP = 0x00,

    // Once FIRMWARE_UPDATE_FLAG_LAST_BLOCK has been issued,
    // the accessory can then determine if the offer contents
    // apply to it.
    FIRMWARE_UPDATE_OFFER_ACCEPT = 0x01,

    // Once FIRMWARE_UPDATE_FLAG_LAST_BLOCK has been issued,
    // the accessory can then determine if the offer block contents apply to it.
    FIRMWARE_UPDATE_OFFER_REJECT = 0x02,

    // The offer needs to be delayed at this time.  The device has
    // nowhere to put the incoming blob.
    FIRMWARE_UPDATE_OFFER_BUSY = 0x03,

    // Used with the Offer Other response for the OFFER_NOTIFY_ON_READY
    // request, when the Accessory is ready to accept additional Offers.
    FIRMWARE_UPDATE_OFFER_COMMAND_READY = 0x04,

    // Response applicable to when the Offer request is not recognized.
    FIRMWARE_UPDATE_CMD_NOT_SUPPORTED = 0xFF
} T_FW_OFFER_STATUS;

typedef enum t_fw_update_offer_reject_reason
{
    // The offer was rejected by the product due to the offer
    // version being older than the currently downloaded / existing firmware.
    FIRMWARE_OFFER_REJECT_OLD_FW = 0x00, //The offer was rejected by the product due to the offer version being older than the currently downloaded / existing firmware.

    // The offer was rejected due to it not being applicable to
    // the product?s primary MCU(Component ID).
    FIRMWARE_OFFER_REJECT_INV_COMPONENT = 0x01,

    // MCU Firmware has been updated and a swap is currently pending.
    // No further Firmware Update processing can occur until the
    // target has been reset.
    FIRMWARE_UPDATE_OFFER_SWAP_PENDING = 0x02,

    // The offer was rejected due to a Version mismatch(Debug / Release for example)
    FIRMWARE_OFFER_REJECT_MISMATCH = 0x03,

    // The bank being offered for the component is currently in use.
    FIRMWARE_OFFER_REJECT_BANK = 0x04,

    // The offer's Platform ID does not correlate to the receiving
    // hardware product.
    FIRMWARE_OFFER_REJECT_PLATFORM = 0x05,

    // The offer's Milestone does not correlate to the receiving
    // hardware's Build ID.
    FIRMWARE_OFFER_REJECT_MILESTONE = 0x06,

    // The offer indicates an interface Protocol Revision that
    // the receiving product does not support.
    FIRMWARE_OFFER_REJECT_INV_PCOL_REV = 0x07,

    // The combination of Milestone & Compatibility Variants Mask did
    // not match the HW.
    FIRMWARE_OFFER_REJECT_VARIANT = 0x08
} T_FW_UPDATE_OFFER_REJECT_REASON;

typedef enum t_component_firmware_update_payload_response
{
    COMPONENT_FIRMWARE_UPDATE_SUCCESS = 0x00,                             // No Error, the requested function(s) succeeded.
    COMPONENT_FIRMWARE_UPDATE_ERROR_PREPARE = 0x01,                       // Could not either: 1) Erase the upper block; 2) Initialize the swap command scratch block; 3) Copy the configuration data to the upper block.
    COMPONENT_FIRMWARE_UPDATE_ERROR_WRITE = 0x02,                         // Could not write the bytes.
    COMPONENT_FIRMWARE_UPDATE_ERROR_COMPLETE = 0x03,                      // Could not set up the swap, in response to COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK.
    COMPONENT_FIRMWARE_UPDATE_ERROR_VERIFY = 0x04,                        // Verification of the DWord failed, in response to COMPONENT_FIRMWARE_UPDATE_FLAG_VERIFY.
    COMPONENT_FIRMWARE_UPDATE_ERROR_CRC = 0x05,                           // CRC of the image failed, in response to COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK.
    COMPONENT_FIRMWARE_UPDATE_ERROR_SIGNATURE = 0x06,                     // Firmware signature verification failed, in response to COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK.
    COMPONENT_FIRMWARE_UPDATE_ERROR_VERSION = 0x07,                       // Firmware version verification failed, in response to COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK.
    COMPONENT_FIRMWARE_UPDATE_ERROR_SWAP_PENDING = 0x08,                  // Firmware has already been updated and a swap is pending.  No further Firmware Update commands can be accepted until the device has been reset.
    COMPONENT_FIRMWARE_UPDATE_ERROR_INVALID_ADDR = 0x09,                  // Firmware has detected an invalid destination address within the message data content.
    COMPONENT_FIRMWARE_UPDATE_ERROR_NO_OFFER = 0x0A,                      // The Firmware Update Content Command was received without first receiving a valid & accepted FW Update Offer.
    COMPONENT_FIRMWARE_UPDATE_ERROR_INVALID = 0x0B                        // General error for the Firmware Update Content command, such as an invalid applicable Data Length.
} T_COMPONENT_FIRMWARE_UPDATE_PAYLOAD_RESPONSE;

typedef enum t_content_status
{
    CFU_CONTENT_IDLE,
    CFU_CONTENT_RECEIVE_CFU_HEADER,
    CFU_CONTENT_RECEIVE_IMG_HEADER,
    CFU_CONTENT_RECEIVE_IMG_BLOCKS,
} T_CONTENT_STATUS;

typedef enum t_cfu_remote_msg
{
    APP_REMOTE_MSG_CFU_ACK                    = 0x00,
    APP_REMOTE_MSG_CFU_P2S_OFFER              = 0x01,
    APP_REMOTE_MSG_CFU_P2S_CONTENT            = 0x02,
    APP_REMOTE_MSG_CFU_S2P_OFFER_RSP          = 0x03,
    APP_REMOTE_MSG_CFU_S2P_CONTENT_RSP        = 0x04,

    APP_REMOTE_MSG_CFU_TOTAL                  = 0x05,
} T_CFU_REMOTE_MSG;

typedef union t_cfu_offer_version
{
    uint8_t value[4];
    struct
    {
        uint8_t signer: 2;
        uint8_t build_type: 2;
        uint8_t rsvd0: 3;
        uint8_t official: 1;
        uint8_t minor_ver[2];
        uint8_t major_ver;
    } ver;
} T_CFU_OFFER_VERSION;

typedef struct t_fw_update_standard_offer
{
    uint8_t rsvd0;
    uint8_t rsvd1: 6;
    uint8_t force_reset: 1;
    uint8_t force_ignore: 1;
    uint8_t component_id;
    uint8_t token;
    T_CFU_OFFER_VERSION version;
    uint16_t last_image_id;
    uint8_t bud_role: 2;
    uint8_t force_ignore_platform_id: 1;
    uint8_t rsvd2: 5;
    uint8_t rsvd3;
    uint8_t cfu_ver: 4;
    uint8_t bank: 2;
    uint8_t rsvd4: 2;
    uint8_t milestone: 4;
    uint8_t sfua_ver: 4;
    uint16_t platform_id;
} T_FW_UPDATE_STANDARD_OFFER;

typedef struct t_fw_update_special_offer
{
    uint8_t cmdCode;
    uint8_t rsvd0;
    uint8_t component_id;
    uint8_t token;
    uint8_t rsvd1[12];
} T_FW_UPDATE_SPECIAL_OFFER;


typedef struct t_fw_update_content_command
{
    uint8_t flags;
    uint8_t length;
    uint16_t seq_num;
    uint8_t offset[4];
    uint8_t data[52];
} T_FW_UPDATE_CONTENT_COMMAND;

typedef struct t_cfu_struct
{
    uint32_t cur_img_offset;
    uint32_t cur_img_total_len;
    uint32_t cfu_header_offset;
#if (F_APP_CFU_HID_SUPPORT || F_APP_TEAMS_HID_SUPPORT || F_APP_CFU_SPP_SUPPORT || F_APP_CFU_BLE_CHANNEL_SUPPORT)
    T_APP_BR_LINK *p_link;
#endif
    uint16_t last_seq_num;
    uint16_t record_seq_num;
    uint16_t cur_img_id;
    uint8_t content_state;
    uint8_t signing;
    uint8_t ver_cmp;
    uint8_t cur_component_id;
    uint8_t cur_hid_source_type;
    uint8_t report_type;
    T_APP_CFU_REPORT_ACTION_TYPE report_action;
    T_CFU_FLAG flag;
    uint8_t bp_level;
} T_CFU_STRUCT;

extern T_CFU_STRUCT *ms_cfu;
typedef struct _FW_UPDATE_VER_RESPONSE
{
    uint8_t component_count;
    uint8_t rsvd0[2];
    uint8_t cfu_ver: 4;
    uint8_t rsvd1: 3;
    uint8_t extension: 1;

    struct
    {
        uint32_t version;
        uint8_t bank: 2;
        uint8_t bud_role: 2; //bud role:0x00 single,0x01 pri, 0x02 sec
        uint8_t milestone: 4;
        uint8_t component_id;
        uint16_t platform_id;
    } component_info[7];
} FW_UPDATE_VER_RESPONSE;

typedef struct _FW_UPDATE_OFFER_RESPONSE
{
    uint8_t rsvd0[3];
    uint8_t token;
    uint8_t challenge[4];
    uint8_t rejectReason;
    uint8_t rsvd1[3];
    uint8_t status;
    uint8_t rsvd2[3];
} FW_UPDATE_OFFER_RESPONSE;

typedef struct _FW_UPDATE_CONTENT_RESPONSE
{
    uint16_t seq_num;
    uint16_t rsvd0;
    uint8_t status;
    uint8_t rsvd1[11];
} FW_UPDATE_CONTENT_RESPONSE;

/*============================================================================*
 *                              Functions
 *============================================================================*/

/** @defgroup APP_COMMON_CFU_Exported_Functions APP COMMON CFU Exported Functions
    * @brief
    * @{
    */

/**
    * @brief initialize app cfu.
    *\xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void
    * @return void
    */
void app_cfu_init(void);

/**
    * @brief parse get version report.
    *\xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  ver_rsp version response
    * @return void
    */
void app_cfu_get_version_parse(FW_UPDATE_VER_RESPONSE *ver_rsp);

/**
    * @brief parse offer.
    *\xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param data offer data
    * @param length offer length
    * @param offer_rsp offer response
    * @return current component response or not
    */
T_CFU_RSP_TYPE app_cfu_offer_parse(uint8_t *data, uint16_t length,
                                   FW_UPDATE_OFFER_RESPONSE *offer_rsp);

/**
    * @brief parse content.
    *\xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param data content data
    * @param length content length
    * @param content_rsp content response
    * @return current component response or not
    */
T_CFU_RSP_TYPE app_cfu_content_parse(uint8_t *data, uint8_t length,
                                     FW_UPDATE_CONTENT_RESPONSE *content_rsp);

#if (F_APP_CFU_HID_SUPPORT || F_APP_TEAMS_HID_SUPPORT || F_APP_CFU_SPP_SUPPORT || F_APP_CFU_BLE_CHANNEL_SUPPORT)
/**
    * @brief save link info.
    *\xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  bd_addr bd address
    * @return current component response or not
    */
void app_cfu_save_link_info(uint8_t *bd_addr);

/**
    * @brief check wether app cfu is in process.
    *\xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  p_link b2s link
    * @return True: is doing cfu; False: is not doing cfu
    */
bool app_cfu_is_process_check(T_APP_BR_LINK **p_link);
#endif

/**
    * @brief check wether app cfu is in process.
    *\xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
    * @param  void
    * @return True: is doing cfu; False: is not doing cfu
    */
bool app_cfu_is_in_process(void);

/** @} */ /* End of group APP_COMMON_CFU_Exported_Functions */
/** @} */ /* End of group APP_COMMON_CFU */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_COMMON_CFU_H_ */
