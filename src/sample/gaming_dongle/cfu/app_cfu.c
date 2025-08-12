#include <string.h>
#include <bt_types.h>
#include "trace.h"
#include "app_cfg.h"
#include "os_mem.h"
#include "rtl876x.h"
//#include "flash_device.h"
#include "app_cfu.h"
//#include "rom_ext.h"
#include "app_msg.h"
#include "app_timer.h"
//#include "usb_lib_ext.h"
#include "rom_uuid.h"
#include "dfu_api.h"
//#include "flash_nor_device.h"
#include "compiler_abstraction.h"
#include "fmc_api.h"
#include "dfu_common.h"
#include "app_usb_hid.h"
#include "ota_ext.h"

#pragma pack(1)
typedef union _COMPONENT_VERSION
{
    uint32_t AsUInt32;
    struct
    {
        uint8_t Variant;
        uint16_t MinorVersion;
        uint8_t MajorVersion;
    };
} COMPONENT_VERSION;

typedef union _COMPONENT_PROPERTY
{
    uint32_t AsUInt32;
    struct
    {
        uint8_t Bank : 2;
        uint8_t Reserved: 2;
        uint8_t VendorSpecific0: 4;
        uint8_t ComponentId;
        uint16_t VendorSpecific1;
    };
} COMPONENT_PROPERTY;

typedef struct _COMPONENT_VERSION_AND_PROPERTY
{
    COMPONENT_VERSION ComponentVersion;
    COMPONENT_PROPERTY ComponentProperty;
} COMPONENT_VERSION_AND_PROPERTY;

typedef struct _GET_FWVERSION_RESPONSE
{
    uint8_t ReportId;
    struct
    {
        uint8_t ComponentCount;
        uint16_t Reserved0;
        uint8_t ProtocolRevision : 4;
        uint8_t Reserved1 : 3;
        uint8_t ExtensionFlag : 1;
    } header;
    COMPONENT_VERSION_AND_PROPERTY componentVersionsAndProperty[7];
} GET_FWVERSION_RESPONSE;

typedef struct
{
    uint8_t ReportId;
    struct
    {
        uint8_t SegmentNumber;
        uint8_t Reserved0 : 6;
        uint8_t ForceImmediateReset : 1;
        uint8_t ForceIgnoreVersion : 1;
        uint8_t ComponentId;
        uint8_t Token;
    } ComponentInfo;
    COMPONENT_VERSION Version;
    uint16_t LastImgID;
    uint8_t  DongleEarphone: 2;
    uint8_t  ForceIgnorePlateformID: 1;
    uint8_t  RSV: 5;
    uint8_t  VendorRsv;
    struct
    {
        uint8_t ProtocolVersion : 4;
        uint8_t bank : 2;
        uint8_t reserved0 : 2;
        uint8_t milestone : 4;
        uint8_t Reserved1 : 4;
        uint16_t PlatformID; // for CFU V4 change
    } MiscAndProtocolVersion;
} FWUPDATE_OFFER_COMMAND;

typedef struct
{
    struct
    {
        uint8_t infoCode;
        uint8_t reserved0;
        uint8_t shouldBe0xFF;
        uint8_t token;
    } componentInfo;

    uint32_t reserved0[3];

} FWUPDATE_OFFER_INFO_ONLY_COMMAND;

typedef struct
{
    struct
    {
        uint8_t commandCode;
        uint8_t reserved0;
        uint8_t shouldBe0xFE;
        uint8_t token;
    } componentInfo;

    uint32_t reserved0[3];

} FWUPDATE_SPECIAL_OFFER_COMMAND;

typedef struct _FWUPDATE_CONTENT_COMMAND
{
    uint8_t ReportId;
    uint8_t Flags;
    uint8_t Length;
    uint16_t SequenceNumber;
    uint32_t Address;
    uint8_t Data[52];
} FWUPDATE_CONTENT_COMMAND;

typedef enum _RESPONSE_TYPE
{
    OFFER,
    CONTENT
} RESPONSE_TYPE;

typedef enum _COMPONENT_FIRMWARE_UPDATE_FLAG
{
    COMPONENT_FIRMWARE_UPDATE_FLAG_DEFAULT = 0x00,
    COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK = 0x80,        // Denotes the first block of a firmware payload.
    COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK = 0x40,         // Denotes the last block of a firmware payload.
    COMPONENT_FIRMWARE_UPDATE_FLAG_VERIFY = 0x08,             // If set, the firmware verifies the byte array in the upper block at the specified address.
} COMPONENT_FIRMWARE_UPDATE_FLAG;

enum FwUpdateOfferStatus
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
};

enum FwUpdateOfferRejectReason
{
    // The offer was rejected by the product due to the offer
    // version being older than the currently downloaded / existing firmware.
    FIRMWARE_OFFER_REJECT_OLD_FW = 0x00, //The offer was rejected by the product due to the offer version being older than the currently downloaded / existing firmware.

    // The offer was rejected due to it not being applicable to
    // the product?s primary MCU(Component ID).
    FIRMWARE_OFFER_REJECT_INV_MCU = 0x01,

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
};

typedef enum _COMPONENT_FIRMWARE_UPDATE_PAYLOAD_RESPONSE
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
} COMPONENT_FIRMWARE_UPDATE_PAYLOAD_RESPONSE;

#define REPORT_ID_LENGTH            0x01

#define CFU_OFFER_RESPONSE_LENGTH_BYTES             16
#define HID_CFU_OFFER_RESPONSE_LENGTH_BYTES         (CFU_OFFER_RESPONSE_LENGTH_BYTES + REPORT_ID_LENGTH)

typedef union
{
    uint8_t AsBytes[HID_CFU_OFFER_RESPONSE_LENGTH_BYTES];
    struct HidCfuOfferResponse
    {
        uint8_t ReportId;
        struct CfuOfferResponse
        {
            uint8_t Reserved0[3];
            uint8_t Token;
            uint32_t Reserved1;
            uint8_t RejectReasonCode;
            uint8_t Reserved2[3];
            uint8_t Status;
            uint8_t Reserved3[2];
        } CfuOfferResponse;
    } HidCfuOfferResponse;
} FWUPDATE_OFFER_RESPONSE;

#define CFU_CONTENT_RESPONSE_LENGTH_BYTES           16
#define HID_CFU_CONTENT_RESPONSE_LENGTH_BYTES       (CFU_CONTENT_RESPONSE_LENGTH_BYTES + REPORT_ID_LENGTH)

typedef union
{
    uint8_t AsBytes[HID_CFU_CONTENT_RESPONSE_LENGTH_BYTES];
    struct
    {
        struct HidCfuContentResponse
        {
            uint8_t ReportId;
            struct CfuContentResponse
            {
                uint16_t SequenceNumber;
                uint16_t Reserved0;
                uint8_t Status;
                uint8_t Reserved1[3];
                uint32_t Reserved2[1];
            } CfuContentResponse;
        } HidCfuContentResponse;
    };
} FWUPDATE_CONTENT_RESPONSE;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _T_OFFER_INFO
{
    struct
    {
        uint8_t SegmentNumber;
        uint8_t last_offer_process : 6;
        uint8_t ForceImmediateReset : 1;
        uint8_t ForceIgnoreVersion : 1;
        uint8_t ComponentId;
    } ComponentInfo;
    uint32_t VendorSpecific;
} OFFER_INFO;

typedef struct _T_CFU_STRUCT
{
//#if !UES_OFFER_NO_SKIP
    OFFER_INFO offer_info[MAX_ACCEPTED_OFFER_NUM];
//#endif
    COMPONENT_VERSION update_cfu_version;
    uint8_t update_in_progress: 1;
    uint8_t is_bank_swap_pending: 1;
    uint8_t update_if_segment_verion_equal: 1;
    uint8_t update_result;
    uint32_t addr_count;
    uint32_t current_flashing_offset;
    uint32_t last_transmit_SequenceNumber;
    uint8_t current_updating_compomentID;
    uint8_t current_updating_immediate_reset;
    uint8_t current_updating_ignore_version;
    uint8_t bp_level;
    uint32_t current_image_ID;
    uint32_t current_image_start_addr;
    uint32_t current_image_end_addr;
    uint32_t current_image_length;
} T_CFU_STRUCT;

static T_CFU_STRUCT m_cfu_struct = {0};
static uint8_t app_cfu_dongle_rush_level; // idle 0; connect 1; A2DP run 2
static bool app_cfu_is_image_signed;
static uint8_t app_cfu_response_buffer[17];
static uint8_t app_cfu_timer_id = 0;
static uint8_t timer_idx_cfu_content = 0;
static uint8_t timer_idx_cfu_reset = 0;

#if USE_4K_KEEP_BUFFER
void   *app_cfu_update_progress_buffer;
static uint16_t app_cfu_buffer_used_count;
static uint32_t app_cfu_inneed_write_sector_offset;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t get_active_ota_bank_addr(void);
//////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t app_cfu_update_data(uint32_t img_id, uint32_t offset, uint32_t length, void *p_void,
                             bool last_one)
{
    uint32_t result = 0;
#if (DFU_COMMON_DEBUG == 1)
    DFU_PRINT_TRACE4("app_cfu_update_data image id %x, offset %x, length %x last_one %x",
                     img_id, offset, length, last_one);
#endif
#if USE_4K_KEEP_BUFFER
    uint32_t base_addr;
    memcpy((void *)((uint32_t)app_cfu_update_progress_buffer + app_cfu_buffer_used_count),
           p_void, length);
    app_cfu_buffer_used_count += length;
    if (app_cfu_buffer_used_count >= FMC_SEC_SECTION_LEN)
    {
#if USE_CUSTOMER_SIGN
        if ((img_id == IMG_MCUAPP) && (offset < FMC_SEC_SECTION_LEN) && (!customer_cfu_sign_flag))
        {
            base_addr = get_temp_ota_bank_addr_by_img_id((IMG_ID)IMG_EXT3);
            img_id = IMG_EXT3;
        }
        else
        {
            base_addr = get_temp_ota_bank_addr_by_img_id((IMG_ID)img_id);
        }
#else
        base_addr = get_temp_ota_bank_addr_by_img_id((IMG_ID)img_id);
#endif

        if (memcmp((void *)(base_addr + app_cfu_inneed_write_sector_offset),
                   app_cfu_update_progress_buffer, FMC_SEC_SECTION_LEN))
        {
            result = dfu_write_data_to_flash(img_id, app_cfu_inneed_write_sector_offset, FMC_SEC_SECTION_LEN,
                                             app_cfu_update_progress_buffer);
            DFU_PRINT_TRACE3("app_cfu_update_data write image %x sector %x result %d", img_id,
                             base_addr + app_cfu_inneed_write_sector_offset, result);
        }
        else
        {
            DFU_PRINT_TRACE2("app_cfu_update_data write image %x sector %x no data changed", img_id,
                             base_addr + app_cfu_inneed_write_sector_offset);
        }
        memmove(app_cfu_update_progress_buffer,
                (void *)((uint32_t)app_cfu_update_progress_buffer + FMC_SEC_SECTION_LEN),
                MIN(app_cfu_buffer_used_count, CFU_CONTENT_DATA_LENGTH));
        app_cfu_buffer_used_count -= FMC_SEC_SECTION_LEN;
        app_cfu_inneed_write_sector_offset += FMC_SEC_SECTION_LEN;
        if (last_one)
        {
            if (app_cfu_buffer_used_count != 0)
            {
                DFU_PRINT_TRACE0("app_cfu_update_data last one with more data");
                if (memcmp((void *)(base_addr + app_cfu_inneed_write_sector_offset),
                           app_cfu_update_progress_buffer, app_cfu_buffer_used_count))
                {
                    result = dfu_write_data_to_flash(img_id, app_cfu_inneed_write_sector_offset,
                                                     app_cfu_buffer_used_count,
                                                     app_cfu_update_progress_buffer);
                    DFU_PRINT_TRACE3("app_cfu_update_data write image %x sector %x result %d", img_id,
                                     base_addr + app_cfu_inneed_write_sector_offset, result);
                }
                else
                {
                    DFU_PRINT_TRACE2("app_cfu_update_data write image %x sector %x no data changed", img_id,
                                     base_addr + app_cfu_inneed_write_sector_offset);
                }
            }
            app_cfu_buffer_used_count = 0;
            app_cfu_inneed_write_sector_offset = 0;
#if USE_CUSTOMER_SIGN
            if (img_id != IMG_MCUAPP)
#endif
            {
                result = !dfu_checksum(img_id, 0);
                DFU_PRINT_TRACE2("dfu_check_checksum image id %x, cfu_result %x", img_id,
                                 result);
            }
        }
        DFU_PRINT_TRACE2("app_cfu_update_data buffer used %x, next %x", app_cfu_buffer_used_count,
                         base_addr + app_cfu_inneed_write_sector_offset);
    }
    else if (last_one)
    {
        base_addr = get_temp_ota_bank_addr_by_img_id((IMG_ID)img_id);
        if (memcmp((void *)(base_addr + app_cfu_inneed_write_sector_offset),
                   app_cfu_update_progress_buffer, app_cfu_buffer_used_count))
        {
            result = dfu_write_data_to_flash(img_id, app_cfu_inneed_write_sector_offset,
                                             app_cfu_buffer_used_count,
                                             app_cfu_update_progress_buffer);
            DFU_PRINT_TRACE4("app_cfu_update_data write image %x last sector %x length %x result %d", img_id,
                             base_addr + app_cfu_inneed_write_sector_offset, app_cfu_buffer_used_count, result);
        }
        else
        {
            DFU_PRINT_TRACE2("app_cfu_update_data write image %x last sector %x no data changed", img_id,
                             base_addr + app_cfu_inneed_write_sector_offset);
        }
        app_cfu_buffer_used_count = 0;
        app_cfu_inneed_write_sector_offset = 0;
#if USE_CUSTOMER_SIGN
        if (img_id != IMG_MCUAPP)
#endif
        {
            result = !dfu_checksum(img_id, 0);
            DFU_PRINT_TRACE2("dfu_check_checksum image id %x, cfu_result %x", img_id,
                             result);
        }
    }
    return result;
#else

    result = dfu_write_data_to_flash(img_id, offset, 0, length, p_void);
    if (result)
    {
        return result;
    }
    if (last_one)
    {
        result = !dfu_checksum((IMG_ID)img_id, 0);
        DFU_PRINT_TRACE2("dfu_check_checksum image id %x, cfu_result %x", img_id,
                         result);
    }
    return result;
#endif
}

uint8_t app_cfu_get_bank_index(bool active)// 0 inactive; 1 active
{
    uint32_t ota_bank0_addr = flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0);
    uint32_t active_bank_addr = get_active_ota_bank_addr();
    if (ota_bank0_addr == get_active_ota_bank_addr())
    {
        return active ? APP_CFU_IMAGE_LOCATION_BANK0 : APP_CFU_IMAGE_LOCATION_BANK1;
    }
    else
    {
        return active ? APP_CFU_IMAGE_LOCATION_BANK1 : APP_CFU_IMAGE_LOCATION_BANK0;
    }
}

uint32_t app_cfu_get_version(bool active)
{
    uint32_t verion = 0;
//    ftl_load_from_storage(&verion, APP_RW_DATA_ADDR + sizeof(T_APP_CFG_NV), sizeof(uint32_t));
    uint32_t addr = 0;
    if (active)
    {
        addr = get_header_addr_by_img_id(CFU_VERSION_SECTION);
    }
    else
    {
        addr = get_temp_ota_bank_addr_by_img_id(CFU_VERSION_SECTION);
    }
    if (addr != 0)
    {
        verion =  *(uint32_t *)(addr + 0x3f4);
    }
    DFU_PRINT_INFO2("app_cfu_get_cfu_version at addr %x ver %x", addr, verion);
    return verion;
}

uint8_t app_cfu_get_component_ID()
{
    uint8_t ID = 0;
    uint32_t addr;
    addr = get_header_addr_by_img_id(CFU_VERSION_SECTION);
    if (addr != 0)
    {
        ID = *(uint8_t *)(addr + 0x3f2);
    }
    DFU_PRINT_INFO2("app_cfu_get_component_ID at addr %x ver %x", addr, ID);
    return ID;
}

uint16_t app_cfu_get_platform_ID()
{
    uint16_t ID = 0;
    uint32_t addr;
    addr = get_header_addr_by_img_id(CFU_VERSION_SECTION);
    if (addr != 0)
    {
        ID = *(uint16_t *)(addr + 0x3fe);
    }
    DFU_PRINT_INFO2("app_cfu_get_platform_ID at addr %x plateform ID %x", addr, ID);
    return ID;
}

void app_cfu_offer_parse(uint8_t *data, uint16_t length)
{
    DFU_PRINT_TRACE1("app_cfu_offer_parse %b", TRACE_BINARY(17, data));
    int i;
    uint8_t __UNUSED offer_process = OFFER_PROCESS_NOT_GOT;
    FWUPDATE_OFFER_COMMAND *offerCommand = (FWUPDATE_OFFER_COMMAND *)data;
    FWUPDATE_OFFER_RESPONSE *offerResponse = (FWUPDATE_OFFER_RESPONSE *)app_cfu_response_buffer;
    offerResponse->HidCfuOfferResponse.ReportId = REPORT_ID_CFU_OFFER_INPUT;
    DFU_PRINT_TRACE8("Received Offer: Component {Id 0x%x, V 0x%x, I 0x%x, Seg 0x%x, Token 0x%x, Bank %d, P %x} Version { 0x%x }\n",
                     offerCommand->ComponentInfo.ComponentId,
                     offerCommand->ComponentInfo.ForceIgnoreVersion,
                     offerCommand->ComponentInfo.ForceImmediateReset,
                     offerCommand->ComponentInfo.SegmentNumber,
                     offerCommand->ComponentInfo.Token,
                     offerCommand->MiscAndProtocolVersion.bank,
                     offerCommand->ForceIgnorePlateformID,
                     offerCommand->Version.AsUInt32);
    offerResponse->HidCfuOfferResponse.CfuOfferResponse.Token = offerCommand->ComponentInfo.Token;

    if (m_cfu_struct.update_in_progress || (app_cfu_dongle_rush_level > 1))
    {
        DFU_PRINT_WARN0("Offer reject update_in_progress");
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_BUSY;
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.RejectReasonCode = FIRMWARE_UPDATE_OFFER_BUSY;
        return;
    }
    if (offerCommand->ComponentInfo.ComponentId == CFU_SPECIAL_OFFER_CMD)
    {
        FWUPDATE_SPECIAL_OFFER_COMMAND *pCommand = (FWUPDATE_SPECIAL_OFFER_COMMAND *)offerCommand;
        DFU_PRINT_TRACE1("Received extend Offer: commandCode %x", pCommand->componentInfo.commandCode);
        if (pCommand->componentInfo.commandCode == CFU_SPECIAL_OFFER_GET_STATUS)
        {
            offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_COMMAND_READY;
            return;
        }
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_REJECT;
        return;
    }
    if (offerCommand->ComponentInfo.ComponentId == CFU_OFFER_METADATA_INFO_CMD)
    {
        FWUPDATE_OFFER_INFO_ONLY_COMMAND *pCommand = (FWUPDATE_OFFER_INFO_ONLY_COMMAND *)offerCommand;
        DFU_PRINT_TRACE1("Received info Offer: infoCode %x", pCommand->componentInfo.infoCode);
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_REJECT;
        return;
    }
    if (m_cfu_struct.is_bank_swap_pending)
    {
        DFU_PRINT_WARN0("Offer reject is_bank_swap_pending");
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_REJECT;
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.RejectReasonCode =
            FIRMWARE_UPDATE_OFFER_SWAP_PENDING;
        return;
    }

    /* FIXME: Do we need to check working bank's component_ID*/
    if (offerCommand->ComponentInfo.ComponentId != CFU_DONGLE_COMPONENT_ID &&
        !customer_cfu_sign_flag)
    {
        DFU_PRINT_WARN0("Offer reject FIRMWARE_OFFER_REJECT_INV_MCU");
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_REJECT;
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.RejectReasonCode =
            FIRMWARE_OFFER_REJECT_INV_MCU;
        return;
    }
    if (!offerCommand->ForceIgnorePlateformID &&
        offerCommand->MiscAndProtocolVersion.PlatformID != app_cfu_get_platform_ID() &&
        !customer_cfu_sign_flag)
    {
        DFU_PRINT_WARN0("Offer reject FIRMWARE_OFFER_REJECT_PLATFORM");
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_REJECT;
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.RejectReasonCode =
            FIRMWARE_OFFER_REJECT_PLATFORM;
        return;
    }

    if (offerCommand->MiscAndProtocolVersion.bank != app_cfu_get_bank_index(false))
    {
        DFU_PRINT_WARN0("Offer reject FIRMWARE_OFFER_REJECT_BANK");
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_REJECT;
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.RejectReasonCode = FIRMWARE_OFFER_REJECT_BANK;
        return;
    }

    for (i = 0; i < MAX_ACCEPTED_OFFER_NUM &&
         m_cfu_struct.offer_info[i].ComponentInfo.ComponentId != 0; i++)
    {
        DFU_PRINT_INFO2("ComponentId %d %x", i, m_cfu_struct.offer_info[i].ComponentInfo.ComponentId);
        if (m_cfu_struct.offer_info[i].ComponentInfo.ComponentId == offerCommand->ComponentInfo.ComponentId)
        {
            DFU_PRINT_INFO2("Offer matched i %d, ID %x", i,
                            m_cfu_struct.offer_info[i].ComponentInfo.ComponentId);
            offer_process = m_cfu_struct.offer_info[i].ComponentInfo.last_offer_process;
            m_cfu_struct.last_transmit_SequenceNumber = 0xFFFFFFFF;
            m_cfu_struct.update_result = 1;
            app_start_timer(&timer_idx_cfu_content, "app_cfu_timer_trans",
                            app_cfu_timer_id, CFU_TIMER_CONTENT_TRANS, 0, false,
                            3000);
            m_cfu_struct.current_updating_compomentID = offerCommand->ComponentInfo.ComponentId;
            m_cfu_struct.current_updating_immediate_reset = offerCommand->ComponentInfo.ForceImmediateReset;
            break;
        }
    }
    if (offer_process == OFFER_PROCESS_NOT_GOT && i < MAX_ACCEPTED_OFFER_NUM)
    {
        if (!offerCommand->ComponentInfo.ForceIgnoreVersion &&
            offerCommand->Version.AsUInt32 <= app_cfu_get_version(true))
        {
            DFU_PRINT_INFO0("Offer reject FIRMWARE_OFFER_REJECT_OLD_FW");
            offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_REJECT;
            offerResponse->HidCfuOfferResponse.CfuOfferResponse.RejectReasonCode = FIRMWARE_OFFER_REJECT_OLD_FW;
            return;
        }
        if (offerCommand->Version.AsUInt32 < m_cfu_struct.update_cfu_version.AsUInt32)
        {
            DFU_PRINT_INFO0("Offer reject FIRMWARE_OFFER_REJECT_MISMATCH");
            offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_REJECT;
            offerResponse->HidCfuOfferResponse.CfuOfferResponse.RejectReasonCode =
                FIRMWARE_OFFER_REJECT_MISMATCH;
            return;
        }
        DFU_PRINT_INFO4("Offer skipped CID %x, seg %x, ver %x, bank %x",
                        offerCommand->ComponentInfo.ComponentId,
                        offerCommand->ComponentInfo.SegmentNumber, offerCommand->Version.AsUInt32,
                        offerCommand->MiscAndProtocolVersion.bank);
        m_cfu_struct.update_if_segment_verion_equal = true;
        m_cfu_struct.update_cfu_version.AsUInt32 = offerCommand->Version.AsUInt32;
        m_cfu_struct.offer_info[i].ComponentInfo.ComponentId = offerCommand->ComponentInfo.ComponentId;
        m_cfu_struct.offer_info[i].ComponentInfo.ForceIgnoreVersion =
            offerCommand->ComponentInfo.ForceIgnoreVersion;
        m_cfu_struct.offer_info[i].ComponentInfo.ForceImmediateReset =
            offerCommand->ComponentInfo.ForceImmediateReset;
        m_cfu_struct.offer_info[i].ComponentInfo.last_offer_process = OFFER_PROCESS_SKIPPED;
        m_cfu_struct.offer_info[i].ComponentInfo.SegmentNumber = offerCommand->ComponentInfo.SegmentNumber;
        //m_cfu_struct.offer_info[i].VendorSpecific = offerCommand->VendorSpecific;
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_SKIP;
        return;
    }
    if (offer_process == OFFER_PROCESS_SKIPPED && i < MAX_ACCEPTED_OFFER_NUM)
    {
        DFU_PRINT_INFO4("Offer accepted CID %x, seg %x, ver %x, bank %x",
                        offerCommand->ComponentInfo.ComponentId,
                        offerCommand->ComponentInfo.SegmentNumber, offerCommand->Version.AsUInt32,
                        offerCommand->MiscAndProtocolVersion.bank);
        m_cfu_struct.update_in_progress = true;
#if USE_4K_KEEP_BUFFER
        DFU_PRINT_TRACE6("heap memory scan main end:data on %d, data off %d, buffer on %d, DTCM0 %d ITCM1 %d DSPSHARE %d",
                         os_mem_peek(RAM_TYPE_DATA_ON), os_mem_peek(RAM_TYPE_DATA_OFF),
                         os_mem_peek(RAM_TYPE_BUFFER_ON), os_mem_peek(RAM_TYPE_DTCM0),
                         os_mem_peek(RAM_TYPE_ITCM1), os_mem_peek(RAM_TYPE_DSPSHARE));
        app_cfu_update_progress_buffer = malloc(FMC_SEC_SECTION_LEN + CFU_CONTENT_DATA_LENGTH);
        DFU_PRINT_TRACE6("heap memory scan main end:data on %d, data off %d, buffer on %d, DTCM0 %d ITCM1 %d DSPSHARE %d",
                         os_mem_peek(RAM_TYPE_DATA_ON), os_mem_peek(RAM_TYPE_DATA_OFF),
                         os_mem_peek(RAM_TYPE_BUFFER_ON), os_mem_peek(RAM_TYPE_DTCM0),
                         os_mem_peek(RAM_TYPE_ITCM1), os_mem_peek(RAM_TYPE_DSPSHARE));
        if (app_cfu_update_progress_buffer == NULL)
        {
            offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_CMD_NOT_SUPPORTED;
        }
#endif
        m_cfu_struct.offer_info[i].ComponentInfo.last_offer_process = OFFER_PROCESS_ACCEPTED;
        m_cfu_struct.current_updating_ignore_version =
            m_cfu_struct.offer_info[i].ComponentInfo.ForceIgnoreVersion;
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_ACCEPT;
//        dfu_init();
    }
    if (offer_process == OFFER_PROCESS_ACCEPTED && i < MAX_ACCEPTED_OFFER_NUM)
    {
        DFU_PRINT_WARN4("Offer resend CID %x, seg %x, ver %x, bank %x",
                        offerCommand->ComponentInfo.ComponentId,
                        offerCommand->ComponentInfo.SegmentNumber, offerCommand->Version.AsUInt32,
                        offerCommand->MiscAndProtocolVersion.bank);
        m_cfu_struct.offer_info[i].ComponentInfo.last_offer_process = OFFER_PROCESS_SKIPPED;
        offerResponse->HidCfuOfferResponse.CfuOfferResponse.Status = FIRMWARE_UPDATE_OFFER_SKIP;
    }
//#endif
    fmc_flash_nor_get_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0),
                            &m_cfu_struct.bp_level);
    DFU_PRINT_TRACE1("app_cfu_offer_parse flash bp level %d", m_cfu_struct.bp_level);
    fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0), 0);
}

void app_cfu_error_process()
{
    DFU_PRINT_TRACE0("app_cfu_error_process");
#if USE_4K_KEEP_BUFFER
    if (app_cfu_update_progress_buffer)
    {
        free(app_cfu_update_progress_buffer);
        app_cfu_update_progress_buffer = NULL;
        app_cfu_buffer_used_count = 0;
        app_cfu_inneed_write_sector_offset = 0;
    }
#endif

    if (m_cfu_struct.bp_level)
    {
        fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0),
                                m_cfu_struct.bp_level);
    }

    memset(&m_cfu_struct, 0, sizeof(T_CFU_STRUCT));
}

void app_cfu_content_parse(uint8_t *data, uint16_t length)
{
    FWUPDATE_CONTENT_RESPONSE *contentResponse = (FWUPDATE_CONTENT_RESPONSE *)app_cfu_response_buffer;
    FWUPDATE_CONTENT_COMMAND *contentCommand = (FWUPDATE_CONTENT_COMMAND *)
                                               data;//app_cfu_receive_buffer;
    DFU_PRINT_TRACE4("Content Received: { SeqNo = 0x%x Addr = 0x%x, L = 0x%x, flag %x}\n",
                     contentCommand->SequenceNumber,
                     contentCommand->Address,
                     contentCommand->Length,
                     contentCommand->Flags);
    contentResponse->HidCfuContentResponse.ReportId = REPORT_ID_CFU_PAYLOAD_INPUT;
    contentResponse->HidCfuContentResponse.CfuContentResponse.SequenceNumber =
        contentCommand->SequenceNumber;
    //check cfu seq num
    if ((contentCommand->Flags & COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK) ==
        COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK)
    {
        m_cfu_struct.last_transmit_SequenceNumber = contentCommand->SequenceNumber - 1;
    }
    if (m_cfu_struct.last_transmit_SequenceNumber == contentCommand->SequenceNumber)
    {
        DFU_PRINT_TRACE1("received seg %x again", contentCommand->SequenceNumber);
        contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
            COMPONENT_FIRMWARE_UPDATE_SUCCESS;
        return;
    }
    else if ((m_cfu_struct.last_transmit_SequenceNumber + 1 != contentCommand->SequenceNumber) &&
             m_cfu_struct.update_in_progress)
    {
        DFU_PRINT_TRACE2("received seg %x error, last %x", contentCommand->SequenceNumber,
                         m_cfu_struct.last_transmit_SequenceNumber);
        contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
            COMPONENT_FIRMWARE_UPDATE_ERROR_PREPARE;
        return;
    }
    m_cfu_struct.last_transmit_SequenceNumber = contentCommand->SequenceNumber;

    //get CFU header for new image
    if (((contentCommand->Flags & COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK) ==
         COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK))
    {
        DFU_PRINT_TRACE2("cfu header start length %d, %b", contentCommand->Length,
                         TRACE_BINARY(20, contentCommand->Data));
        CFU_HEADER *header = (CFU_HEADER *)contentCommand->Data;
        {
            m_cfu_struct.current_flashing_offset = 0;
            m_cfu_struct.current_image_ID = header->image_id;
            m_cfu_struct.current_image_start_addr = 0;
            m_cfu_struct.current_image_end_addr = header->end_addr;
            m_cfu_struct.current_image_length = header->length;
            m_cfu_struct.addr_count += contentCommand->Length;
            DFU_PRINT_TRACE4("Start Image info id %x start %x end %x length %x",
                             m_cfu_struct.current_image_ID,
                             m_cfu_struct.current_image_start_addr,
                             m_cfu_struct.current_image_end_addr,
                             m_cfu_struct.current_image_length);
            contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                COMPONENT_FIRMWARE_UPDATE_SUCCESS;
            return;
        }
#if USE_CUSTOMER_SIGN
        else
        {
            if ((contentCommand->Flags & COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK) ==
                COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK)
            {
                if (!cfu_MS_sign_prepare_header((MSCFU_HEADER *)(contentCommand->Data)))
                {
                    DFU_PRINT_ERROR0("Keep MS header erase fail");
                    contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                        COMPONENT_FIRMWARE_UPDATE_ERROR_PREPARE;
                    return;
                }
                m_cfu_struct.current_image_start_addr = 0xffffffff;
            }
            if (!cfu_MS_sign_keep_header(contentCommand->Data, contentCommand->Length))
            {
                DFU_PRINT_ERROR0("Keep MS header write fail");
                contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                    COMPONENT_FIRMWARE_UPDATE_ERROR_WRITE;
                return;
            }
            if (contentCommand->Address >= MS_HEADER_LENGTH)
            {
//                memcpy(&ms_cfu_aliment_buffer[ms_cfu_aliment_pointer], contentCommand->Data,
//                       CFU_CONTENT_DATA_LENGTH - ms_cfu_aliment_pointer);
//                CFU_HEADER *header = (CFU_HEADER *)ms_cfu_aliment_buffer;
                if (header->signature != 0x48554643) //CFUH
                {
                    DFU_PRINT_ERROR1("MS image header error, header %b", TRACE_BINARY(14, header));
                    contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                        COMPONENT_FIRMWARE_UPDATE_ERROR_SIGNATURE;
                    return;
                }
                m_cfu_struct.current_flashing_offset = 0;
                m_cfu_struct.current_image_ID = header->image_id & 0xFFFF;
                m_cfu_struct.current_image_start_addr = header->start_addr;
                m_cfu_struct.current_image_end_addr = header->end_addr;
                m_cfu_struct.current_image_length = header->length;
                DFU_PRINT_TRACE4("MS Image info id %x start %x end %x length %x",
                                 m_cfu_struct.current_image_ID,
                                 m_cfu_struct.current_image_start_addr,
                                 m_cfu_struct.current_image_end_addr,
                                 m_cfu_struct.current_image_length);
                m_cfu_struct.addr_count = 0;
                if (!cfu_MS_sign_hash_header())
                {
                    DFU_PRINT_ERROR0("MS image header sign error");
                    contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                        COMPONENT_FIRMWARE_UPDATE_ERROR_SIGNATURE;
                    return;
                }
            }
            m_cfu_struct.addr_count += contentCommand->Length;
            contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                COMPONENT_FIRMWARE_UPDATE_SUCCESS;
            return;
        }
#endif
    }
    else if (m_cfu_struct.addr_count > m_cfu_struct.current_image_end_addr)
    {
        DFU_PRINT_TRACE2("Continue header start length %d, %b", contentCommand->Length,
                         TRACE_BINARY(20, contentCommand->Data));
        CFU_HEADER *header = (CFU_HEADER *)contentCommand->Data;

        m_cfu_struct.current_flashing_offset = 0;
        m_cfu_struct.current_image_ID = header->image_id & 0xFFFF;
        //m_cfu_struct.current_image_start_addr = header->start_addr;
        m_cfu_struct.current_image_end_addr = header->end_addr;
        m_cfu_struct.current_image_length = header->length;
        DFU_PRINT_TRACE4("Continue Image info id %x start %x end %x length %x",
                         m_cfu_struct.current_image_ID,
                         m_cfu_struct.current_image_start_addr,
                         m_cfu_struct.current_image_end_addr,
                         m_cfu_struct.current_image_length);
        m_cfu_struct.addr_count += contentCommand->Length;
#if USE_CUSTOMER_SIGN
        cfu_MS_sign_hash_content(contentCommand->Data, contentCommand->Length);
#endif
        contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
            COMPONENT_FIRMWARE_UPDATE_SUCCESS;
        return;
    }
    else if (m_cfu_struct.current_image_ID != 0)
    {
#if (DFU_COMMON_DEBUG == 1)
        DFU_PRINT_TRACE2("m_cfu_struct.addr_count %x, end_addr %x", m_cfu_struct.addr_count,
                         m_cfu_struct.current_image_end_addr);
#endif
        uint32_t cfu_result = 0;
#if USE_CUSTOMER_SIGN
        cfu_MS_sign_hash_content(contentCommand->Data, contentCommand->Length);
#endif
        if (m_cfu_struct.addr_count < m_cfu_struct.current_image_end_addr)
        {
            //image pre header message
            if (m_cfu_struct.addr_count == m_cfu_struct.current_image_start_addr + CFU_CONTENT_DATA_LENGTH)
            {
                DFU_PRINT_TRACE2("Image header %b UUID %b",
                                 TRACE_BINARY(14, contentCommand->Data),
                                 TRACE_BINARY(16, contentCommand->Data + sizeof(T_IMG_CTRL_HEADER_FORMAT)));
                uint8_t uuid[] = DEFINE_symboltable_uuid;
                if (memcmp(uuid, contentCommand->Data + sizeof(T_IMG_CTRL_HEADER_FORMAT), 16))
                {
                    DFU_PRINT_ERROR1("Image not signed, header %b", TRACE_BINARY(14, contentCommand->Data));
                    contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                        COMPONENT_FIRMWARE_UPDATE_ERROR_SIGNATURE;
                    //return; //not all bb2 image are signatured
                }
                if (app_cfu_is_image_signed && !(((T_IMG_CTRL_HEADER_FORMAT *)(contentCommand->Data))->crc16))
                {
                    DFU_PRINT_ERROR1("Image not appliable, header %b", TRACE_BINARY(14, contentCommand->Data));
                    contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                        COMPONENT_FIRMWARE_UPDATE_ERROR_SIGNATURE;
                    return;
                }
            }
            cfu_result = app_cfu_update_data(m_cfu_struct.current_image_ID,
                                             m_cfu_struct.current_flashing_offset,
                                             CFU_CONTENT_DATA_LENGTH,
                                             contentCommand->Data,
                                             false);
            if (cfu_result == 0)
            {
                m_cfu_struct.current_flashing_offset += CFU_CONTENT_DATA_LENGTH;
                m_cfu_struct.current_image_length -= CFU_CONTENT_DATA_LENGTH;
                m_cfu_struct.addr_count += CFU_CONTENT_DATA_LENGTH;
                contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                    COMPONENT_FIRMWARE_UPDATE_SUCCESS;
                return;
            }
            else
            {
                contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                    COMPONENT_FIRMWARE_UPDATE_ERROR_WRITE;
                return;
            }
        }
        else if (m_cfu_struct.addr_count == m_cfu_struct.current_image_end_addr)
        {
            cfu_result = app_cfu_update_data(m_cfu_struct.current_image_ID,
                                             m_cfu_struct.current_flashing_offset,
                                             m_cfu_struct.current_image_length,
                                             contentCommand->Data, true);
            if (cfu_result == 0)
            {
                m_cfu_struct.current_flashing_offset = 0;
                m_cfu_struct.current_image_length = 0;
                m_cfu_struct.update_result &= (!cfu_result);
                if (((contentCommand->Flags & COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK) ==
                     COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK) && m_cfu_struct.update_result)
                {
                    app_stop_timer(&timer_idx_cfu_content);
                    timer_idx_cfu_content = 0;
                    DFU_PRINT_ERROR0("last block finished");
#if USE_CUSTOMER_SIGN && USE_VERIFY_AFTER_RESET
                    if (!cfu_MS_sign_keep_hash_result(m_cfu_struct.current_updating_ignore_version))
                    {
                        contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                            COMPONENT_FIRMWARE_UPDATE_ERROR_VERIFY;
                        return;
                    }
#endif
                    DFU_PRINT_ERROR5("Version from %x to %x, current bank %x Ignore Ver %d, reset %d",
                                     app_cfu_get_version(true),
                                     app_cfu_get_version(false),
                                     app_cfu_get_bank_index(true),
                                     m_cfu_struct.current_updating_ignore_version,
                                     m_cfu_struct.current_updating_immediate_reset);
                    if (m_cfu_struct.current_updating_ignore_version)
                    {
#if USE_CUSTOMER_SIGN && USE_VERIFY_AFTER_RESET
//                        if(!cfu_MS_sign_keep_flag(m_cfu_struct.current_updating_ignore_version))
//                        {
//                            contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
//                                COMPONENT_FIRMWARE_UPDATE_SUCCESS;
//                            app_cfu_error_process();
//                            return;
//                        }
#else
                        uint32_t header_addr = get_header_addr_by_img_id(IMG_MCUAPP);
                        DFU_PRINT_ERROR2("Erase sector %x address %x", IMG_MCUAPP, header_addr);
                        fmc_flash_nor_erase(header_addr, FMC_FLASH_NOR_ERASE_SECTOR);
#endif
                    }
                    m_cfu_struct.update_in_progress = false;
#if USE_4K_KEEP_BUFFER
                    if (app_cfu_update_progress_buffer)
                    {
                        free(app_cfu_update_progress_buffer);
                        app_cfu_update_progress_buffer = NULL;
                    }
#endif
                    m_cfu_struct.is_bank_swap_pending = true;

                    app_ota_clear_notready_flag();

                    if (m_cfu_struct.bp_level)
                    {
                        fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0),
                                                m_cfu_struct.bp_level);
                    }

                    app_start_timer(&timer_idx_cfu_reset, "app_cfu_timer_reset",
                                    app_cfu_timer_id, CFU_TIMER_RESET_ACTIVE, 0, false,
                                    3000);
                }
                else
                {
                    m_cfu_struct.current_image_start_addr = m_cfu_struct.current_image_end_addr +
                                                            CFU_CONTENT_DATA_LENGTH;
                    DFU_PRINT_TRACE1("move to next image, start addr %x", m_cfu_struct.current_image_start_addr);
                    m_cfu_struct.addr_count += CFU_CONTENT_DATA_LENGTH;
                }
                contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                    COMPONENT_FIRMWARE_UPDATE_SUCCESS;
                return;
            }
            else
            {
                DFU_PRINT_ERROR2("app_cfu_update_data image id %x, result %x", m_cfu_struct.current_image_ID,
                                 cfu_result);
                if ((contentCommand->Flags & COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK) ==
                    COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK)
                {
                    contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                        COMPONENT_FIRMWARE_UPDATE_ERROR_VERIFY;
                }
                else
                {
                    contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                        COMPONENT_FIRMWARE_UPDATE_ERROR_WRITE;
                }
            }
            return;
        }
        else
        {
            contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                COMPONENT_FIRMWARE_UPDATE_ERROR_INVALID;
            DFU_PRINT_ERROR0("CFU content parse error INVALID addr>end");
            return;
        }
    }
    contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
        COMPONENT_FIRMWARE_UPDATE_ERROR_INVALID;
    DFU_PRINT_ERROR0("CFU content parse error INVALID imageID == 0");
    return;
}

void app_cfu_handle_set_report(uint8_t *data, uint16_t length)
{
    memset(app_cfu_response_buffer, 0, sizeof(FWUPDATE_OFFER_RESPONSE));
    switch (data[0])
    {
    case REPORT_ID_CFU_FEATURE_EX:
        {
#if CFU_DEBUG || USE_CUSTOMER_SIGN
            const uint8_t god_mode_code[] = {REPORT_ID_CFU_FEATURE_EX, 0x46, 0x4f, 0x52, 0x43, 0x45, 0x20, 0x49, 0x4e, 0x54, 0x4f, 0x20, 0x47, 0x4f, 0x44, 0x20, 0x4d, 0x4f, 0x44};
#endif
#if USE_CUSTOMER_SIGN && ENABLE_GOD_MODE
            if (!memcmp(data, god_mode_code, sizeof(god_mode_code)))
            {
                customer_cfu_sign_flag = !customer_cfu_sign_flag;
                DFU_PRINT_WARN1("app_cfu FORCE INTO GOD MOD %d", customer_cfu_sign_flag);
            }
#endif
#if CFU_DEBUG
            if (!memcmp(data, god_mode_code, 6))
            {
                //app_cfu_test(app_cfu_receive_buffer, m_cfu_struct.usb_receive_length);
                app_cfu_test(data + 6, length);
                return;
            }
#endif
            break;
        }
    case REPORT_ID_CFU_OFFER_OUTPUT:
        {
//            app_cfu_offer_parse(app_cfu_receive_buffer, m_cfu_struct.usb_receive_length);
            app_cfu_offer_parse(data, length);
            break;
        }
    case REPORT_ID_CFU_PAYLOAD_OUTPUT:
        {
//            app_cfu_content_parse(app_cfu_receive_buffer, m_cfu_struct.usb_receive_length);
#if CFU_DEBUG && defined SYS_UPDATE_BY_CFU && NEED_CHANGE_SYS_CONFIG
            bool app_cfu_test_change_sys_cfg(uint16_t offset, uint8_t *data, uint8_t length);
            if (app_cfu_test_change_sys_cfg(0, data, length))
            {
                return;
            }
#endif
            app_cfu_content_parse(data, length);
            if (((FWUPDATE_CONTENT_RESPONSE *)
                 app_cfu_response_buffer)->HidCfuContentResponse.CfuContentResponse.Status !=
                COMPONENT_FIRMWARE_UPDATE_SUCCESS)
            {
                app_cfu_error_process();
                break;
            }
            if (app_cfu_dongle_rush_level > 1)
            {
                return;
            }
            break;
        }
    default:
        return;
    }
    app_usb_hid_interrupt_in(app_cfu_response_buffer, sizeof(FWUPDATE_OFFER_RESPONSE));
    return;
}

void app_cfu_handle_get_report(uint8_t *data, uint16_t *length)
{
    DFU_PRINT_TRACE0("app_cfu_handle_get_report");
    //*data = (uint8_t *)&firmwareVersionResponse;
    memset(data, 0, sizeof(GET_FWVERSION_RESPONSE));
    GET_FWVERSION_RESPONSE *response = (GET_FWVERSION_RESPONSE *)data;
    response->ReportId = REPORT_ID_CFU_FEATURE;
    response->header.ComponentCount = 1;
    response->header.ProtocolRevision = 4;
    response->componentVersionsAndProperty[0].ComponentVersion.AsUInt32 = app_cfu_get_version(true);
    response->componentVersionsAndProperty[0].ComponentProperty.ComponentId =
        app_cfu_get_component_ID();
    response->componentVersionsAndProperty[0].ComponentProperty.Bank = app_cfu_get_bank_index(1);
    response->componentVersionsAndProperty[0].ComponentProperty.VendorSpecific1 =
        app_cfu_get_platform_ID();
    *length = sizeof(GET_FWVERSION_RESPONSE);
}

void app_cfu_handle_get_reportEx(uint8_t *data, uint16_t *length)
{
}

void app_cfu_set_dongle_rush_level(uint8_t level)
{
    app_cfu_dongle_rush_level = level;
    return;
}

void app_cfu_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    DFU_PRINT_TRACE2("app_cfu_timeout_cb: timer_evt 0x%02X param 0x%x", timer_evt, param);
    static uint32_t last_seg = 0;
    app_stop_timer(&timer_idx_cfu_content);
    timer_idx_cfu_content = 0;
    switch (timer_evt)
    {
    case CFU_TIMER_CONTENT_TRANS:
        {

            DFU_PRINT_TRACE2("CFU_TIMER_CONTENT_TRANS: ls 0x%x nx 0x%x", last_seg,
                             m_cfu_struct.last_transmit_SequenceNumber);
            if (m_cfu_struct.last_transmit_SequenceNumber != last_seg)
            {
                last_seg = m_cfu_struct.last_transmit_SequenceNumber;
                app_start_timer(&timer_idx_cfu_content, "app_cfu_timer_trans",
                                app_cfu_timer_id, CFU_TIMER_CONTENT_TRANS, 0, false,
                                3000);
            }
            else
            {
                DFU_PRINT_ERROR1("CFU_TIMER_CONTENT_TRANS: stopped at segment %x",
                                 m_cfu_struct.last_transmit_SequenceNumber);
                app_cfu_error_process();

            }
            break;
        }
    case CFU_TIMER_RESET_ACTIVE:
        {
            if (m_cfu_struct.current_updating_immediate_reset && !app_cfu_dongle_rush_level)
            {
                DFU_PRINT_ERROR3("CFU_TIMER_RESET_ACTIVE: set version from '%x bank %x' to %x, reset device!",
                                 app_cfu_get_version(true), app_cfu_get_bank_index(true),
                                 app_cfu_get_version(false));
                //dfu_fw_active_reset();
//                fmc_flash_nor_set_bp_lv(flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0), 0);
//                uint32_t header_addr = get_active_ota_bank_addr();
//                fmc_flash_nor_erase(header_addr, FMC_FLASH_NOR_ERASE_SECTOR);
                chip_reset(RESET_ALL);
            }
            else
            {
                DFU_PRINT_WARN0("CFU_TIMER_RESET_ACTIVE: not reset, for next round");
                app_start_timer(&timer_idx_cfu_reset, "app_cfu_timer_reset",
                                app_cfu_timer_id, CFU_TIMER_RESET_ACTIVE, 0, false,
                                10000);
            }
        }
    }
}

void app_cfu_init()
{
    T_IMG_HEADER_FORMAT *addr;
    //dfu_init();
    addr = (T_IMG_HEADER_FORMAT *)get_header_addr_by_img_id(IMG_MCUAPP);
    app_cfu_is_image_signed = addr->ctrl_header.crc16;
    DFU_PRINT_TRACE1("app_cfu_init image signed state %d", app_cfu_is_image_signed);
    app_timer_reg_cb(app_cfu_timeout_cb, &app_cfu_timer_id);
#if CFU_DEBUG_LOG_ONLY
    log_module_bitmap_trace_set(0xffffffffffffffff, LEVEL_TRACE, false);
    log_module_bitmap_trace_set(0xffffffffffffffff, LEVEL_INFO, false);
    log_module_bitmap_trace_set(0xffffffffffffffff, LEVEL_WARN, false);
    log_module_bitmap_trace_set(0xffffffffffffffff, LEVEL_ERROR, false);
    log_module_trace_set(MODULE_DFU, LEVEL_TRACE, true);
    log_module_trace_set(MODULE_DFU, LEVEL_INFO, true);
    log_module_trace_set(MODULE_DFU, LEVEL_WARN, true);
    log_module_trace_set(MODULE_DFU, LEVEL_ERROR, true);
//    log_module_trace_set(MODULE_USB, LEVEL_TRACE, true);
//    log_module_trace_set(MODULE_USB, LEVEL_INFO, true);
//    log_module_trace_set(MODULE_USB, LEVEL_WARN, true);
//    log_module_trace_set(MODULE_USB, LEVEL_ERROR, true);
#endif
}

void app_cfu_dump_image_ver(void)
{
    T_IMG_HEADER_FORMAT *addr;
    IMG_ID i;
    IMG_CHECK_ERR_TYPE ret = IMG_CHECK_PASS;
    for (i = IMG_OCCD; i <= IMAGE_MAX; i++)
    {
        addr = (T_IMG_HEADER_FORMAT *)get_header_addr_by_img_id((IMG_ID)i);
        //addr = (T_IMG_HEADER_FORMAT *)get_temp_ota_bank_addr_by_img_id((IMG_ID)i);
        //DFU_PRINT_TRACE2("app_cfu_get_active_bank_version id %x, addr %x", i, addr);
        if (addr != 0)
        {
            ret = IMG_CHECK_PASS;
            if (i > IMG_OTA)
            {
                ret = check_header_valid((uint32_t)addr, (IMG_ID)i);
            }
            DFU_PRINT_TRACE4("app_cfu_get_active_bank_version id %x, ver %x git_ver %x ret %x", i,
                             addr->ver_val, addr->git_ver.version, ret);
        }
    }
    for (i = IMG_OCCD; i <= IMAGE_MAX; i++)
    {
        //addr = (T_IMG_HEADER_FORMAT *)get_header_addr_by_img_id((IMG_ID)i);
        addr = (T_IMG_HEADER_FORMAT *)get_temp_ota_bank_addr_by_img_id((IMG_ID)i);
        //DFU_PRINT_TRACE2("app_cfu_get_inactive_bank_version id %x, addr %x", i, addr);
        if (addr != 0)
        {
            ret = IMG_CHECK_PASS;
            if (i > IMG_OTA)
            {
                ret = check_header_valid((uint32_t)addr, (IMG_ID)i);
            }
            DFU_PRINT_TRACE4("app_cfu_get_inactive_bank_version id %x, ver %x git_ver %x ret %x", i,
                             addr->ver_val, addr->git_ver.version, ret);
        }
    }
}
#if CFU_DEBUG
void app_cfu_test(uint8_t *data, uint16_t length)
{
    // 2b 46 4f 52 43 45 00 26 00 d0 9f 00 20
    //DFU_PRINT_TRACE1("app_cfu_test %b", TRACE_BINARY(length, data));
    uint8_t *test_data = data + 2;
    switch (data[1])
    {
    case 0:
        {
            app_cfu_get_version(true);
            break;
        }
    case 1:
        {
            //uint32_t addr = *(uint32_t *)(test_data);
            //uint8_t length = *(uint8_t *)(test_data + 4);
            //DFU_PRINT_TRACE3("addr %x, length %x, data %b", addr, length, TRACE_BINARY(length, addr));
            break;
        }
    case 2:
        {
            //bool aes_rslt = false;
            //uint8_t bsize;
            //ota_get_enc_setting(aes_rslt);
            //bsize = flash_partition_size_get(PARTITION_FLASH_OTA_TMP) / 4096;
            //DFU_PRINT_TRACE2("app_cfu_get_device_info aes_rslt %d, bsize %x", aes_rslt, bsize);
            //uint32_t ota_bank0_addr = flash_partition_size_get(PARTITION_FLASH_OTA_BANK_0);
            //uint32_t active_addr = get_active_ota_bank_addr();
            //DFU_PRINT_TRACE2("app_cfu_get_device_info ota_bank0_addr %x, active_addr %x", ota_bank0_addr,
            //                 active_addr);
            break;
        }
    case 3:
        {
            app_cfu_dump_image_ver();
            break;
        }
    case 4:
        {
            uint16_t img_id = *(uint16_t *)(test_data);
            uint32_t offset = *(uint32_t *)(test_data + 2);
            uint8_t length = *(uint8_t *)(test_data + 6);
            //uint32_t dfu_result = 0;
            dfu_write_data_to_flash(img_id, offset, 0, length, (test_data + 7));
            //DFU_PRINT_TRACE4("dfu_update image id %x, offset %x, length %x, value %b", img_id, offset,
            //                 length,
            //                 TRACE_BINARY(length, (test_data + 7)));
            //DFU_PRINT_TRACE1("dfu_update result %x", dfu_result);
        }
        break;
    case 5:
        {
            uint16_t img_id = *(uint16_t *)(test_data);
            //uint32_t cfu_result = 0;
            dfu_checksum((IMG_ID)img_id, 0);
            //DFU_PRINT_TRACE2("dfu_check_checksum image id %x, cfu_result %x", img_id, cfu_result);
        }
        break;
    case 6:
        {
            //uint16_t img_id = *(uint16_t *)(test_data);
            //uint32_t address = get_temp_ota_bank_addr_by_img_id((IMG_ID)img_id);
            //uint32_t offset = *(uint32_t *)(test_data + 2);
            //uint8_t length = *(uint8_t *)(test_data + 6);
            //DFU_PRINT_TRACE4("image %x address %x length %x data is %b", img_id, address + offset, length,
            //                 TRACE_BINARY(length,
            //                              address + offset));
        }
        break;
    case 7:
        {
            //uint16_t img_id = *(uint16_t *)(test_data);
            //uint32_t address = get_header_addr_by_img_id((IMG_ID)img_id);
            //uint32_t offset = *(uint32_t *)(test_data + 2);
            //uint8_t length = *(uint8_t *)(test_data + 6);
            //DFU_PRINT_TRACE4("image %x address %x length %x data is %b", img_id, address + offset, length,
            //                 TRACE_BINARY(length,
            //                              address + offset));
        }
        break;
    case 8:
        {
            uint8_t length = *(uint8_t *)(test_data);
            //DFU_PRINT_TRACE2("interrupt in length %x, %b", length, TRACE_BINARY(length, (test_data + 2)));
            app_usb_hid_interrupt_in((test_data + 1), length);
        }
        break;
    case 0x20:
        {
            uint16_t img_id = *(uint16_t *)(test_data);
            uint32_t header_addr = get_temp_ota_bank_addr_by_img_id((IMG_ID)img_id);
            dfu_erase_img_flash_area(header_addr, BANK0_OTA_HDR_SIZE);
            break;
        }
    case 0x21:
        {
            uint16_t img_id = *(uint16_t *)(test_data);
            uint32_t header_addr = get_header_addr_by_img_id((IMG_ID)img_id);
            dfu_erase_img_flash_area(header_addr, BANK0_OTA_HDR_SIZE);
            break;
        }
    case 0x22:
        {
            uint16_t img_id = *(uint16_t *)(test_data);
            uint32_t address = get_header_addr_by_img_id((IMG_ID)img_id);
            uint32_t offset = *(uint32_t *)(test_data + 2);
            uint8_t length = *(uint8_t *)(test_data + 6);
            uint8_t *pdata = (uint8_t *)(test_data + 7);
            dfu_erase_img_flash_area(address, BANK0_OTA_HDR_SIZE);
            fmc_flash_nor_write(address + offset, pdata, length);
            break;
        }
    case 0x23:
        {
            uint16_t img_id = *(uint16_t *)(test_data);
            uint32_t address = get_header_addr_by_img_id((IMG_ID)img_id);
            uint32_t offset = *(uint32_t *)(test_data + 2);
            uint8_t length = *(uint8_t *)(test_data + 6);
            uint8_t pdata[20] = {0};
            fmc_flash_nor_read(address + offset, pdata, length);
            break;
        }
    case 0x24:
        {
            uint32_t result;
            uint32_t address = *(uint32_t *)(test_data);
            uint8_t length = *(uint8_t *)(test_data + 4);
            uint8_t pdata[61] = {0};
            memcpy(pdata, test_data + 5, length);
            result = fmc_flash_nor_write(address, pdata, length);
            pdata[0] = 0x2a;
            pdata[1] = result;
            app_usb_hid_interrupt_in(pdata, 61);
            break;
        }
    case 0x25:
        {
            uint32_t address = *(uint32_t *)(test_data);
            uint8_t length = *(uint8_t *)(test_data + 4);
            uint8_t pdata[61] = {0x2a, 0};
            fmc_flash_nor_read(address, &pdata[1], length);
            app_usb_hid_interrupt_in(pdata, 61);
            break;
        }
    case 0x26:
        {
            uint32_t result;
            uint32_t address = *(uint32_t *)(test_data);
            result = fmc_flash_nor_erase(address, FMC_FLASH_NOR_ERASE_SECTOR);
            uint8_t pdata[61] = {0x2a, 0};
            pdata[1] = result;
            app_usb_hid_interrupt_in(pdata, 61);
            break;
        }
    case 15:
        {
#if USE_CUSTOMER_SIGN && 0
            uint64_t version = 0;
            version = CryptoLibVersion();
#endif
        }
        break;
    case 16:
        {
#if USE_CUSTOMER_SIGN && 1
            cfu_MS_sign_hash_header();
#endif
        }
        break;
    case 0xff:
        {
//            dfu_fw_active_reset();
        }
        break;
    default:
        memset(app_cfu_response_buffer, 0, sizeof(FWUPDATE_OFFER_RESPONSE));
        for (int i = 0; i < length; i++)
        {
            app_cfu_response_buffer[i] = data[i];
        }
        app_usb_hid_interrupt_in(app_cfu_response_buffer, sizeof(FWUPDATE_OFFER_RESPONSE));
        break;
    }
}

#if NEED_CHANGE_SYS_CONFIG //NOTE: !!Important !! this is a very dangous function!! not enable if no need
bool app_cfu_test_change_sys_cfg(uint16_t offset, uint8_t *data, uint8_t length)
{
#define SYS_AREA_ADDR           0x802000
#define SYS_KEEP_ADDR           0x9FC000
#ifdef SYS_KEEP_BY_STEP
#define SYS_KEEP_STEP_LENGTH           0x40
    uint16_t sector_length = *(uint16_t *)(SYS_AREA_ADDR + 4);
    uint32_t result;
    uint8_t pdata[SYS_KEEP_STEP_LENGTH];
    uint32_t keep_offset;
    result = flash_erase_locked(FLASH_ERASE_SECTOR, SYS_KEEP_ADDR);
    for (keep_offset = 0; keep_offset < 0x1000; keep_offset += SYS_KEEP_STEP_LENGTH)
    {
        memcpy(pdata, (void *)(SYS_AREA_ADDR + keep_offset), SYS_KEEP_STEP_LENGTH);
        result = flash_write_locked((SYS_KEEP_ADDR + keep_offset), SYS_KEEP_STEP_LENGTH, pdata);
    }
    result = flash_erase_locked(FLASH_ERASE_SECTOR, SYS_AREA_ADDR);
    for (keep_offset = 0; keep_offset < 0x1000; keep_offset += SYS_KEEP_STEP_LENGTH)
    {
        memcpy(pdata, (void *)(SYS_KEEP_ADDR + keep_offset), SYS_KEEP_STEP_LENGTH);
        result = flash_write_locked((SYS_AREA_ADDR + keep_offset), SYS_KEEP_STEP_LENGTH, pdata);
    }
    return result;
#elif defined SYS_KEEP_ALL_MEMORY
#define SYS_KEEP_ALL_LENGTH             0x300
#define SYS_AREA_CHANGE_ADDR            SYS_AREA_ADDR+0x1000
    uint32_t result = 0;
    uint8_t pdata[SYS_KEEP_ALL_LENGTH];
    uint16_t point = 6;

    uint16_t sector_length = *(uint16_t *)(SYS_AREA_CHANGE_ADDR + 4);
    struct partten_head
    {
        uint16_t offset;
        uint8_t length;
    }*head;
    memcpy(pdata, (void *)(SYS_AREA_CHANGE_ADDR), SYS_KEEP_ALL_LENGTH);
    while (point < sector_length)
    {
        head = (struct partten_head *)&pdata[point];
        //DFU_PRINT_TRACE3("offset %04x, length %x, data %b", head->offset, head->length,
        //                 TRACE_BINARY(head->length, (&(head->length) + 1)));
        if (offset == head->offset && length == head->length)
        {
            memcpy(&(head->length) + 1, data, head->length);
            result = flash_erase_locked(FLASH_ERASE_SECTOR, SYS_AREA_CHANGE_ADDR);
            result = flash_write_locked((SYS_AREA_CHANGE_ADDR), SYS_KEEP_ALL_LENGTH, pdata);
            return result;
        }
        point = point + sizeof(struct partten_head) + head->length * 2;
    }
    return result;
#elif defined SYS_UPDATE_BY_CFU
    uint32_t result = 0;
    static uint32_t addr_count;
    FWUPDATE_CONTENT_RESPONSE *contentResponse = (FWUPDATE_CONTENT_RESPONSE *)app_cfu_response_buffer;
    FWUPDATE_CONTENT_COMMAND *contentCommand = (FWUPDATE_CONTENT_COMMAND *)
                                               data;//app_cfu_receive_buffer;
    //DFU_PRINT_TRACE1("app_cfu_content_parse command %b", TRACE_BINARY(9, data));
    if (contentCommand->Address != 0xFFFFFFFF)
    {
        return 0;
    }
    if (*(uint32_t *)contentCommand->Data == 0x44414552)
    {
        app_cfu_response_buffer[0] = REPORT_ID_CFU_PAYLOAD_INPUT;
        memcpy(&app_cfu_response_buffer[1], &contentCommand->Data[4], 4);
        memcpy(&app_cfu_response_buffer[5], (void *) * (uint32_t *)(&contentCommand->Data[4]), 12);
        app_hid_interrupt_in(app_cfu_response_buffer, sizeof(FWUPDATE_OFFER_RESPONSE));
        return 1;
    }
    //DFU_PRINT_TRACE4("Content Received: { SeqNo = 0x%x Addr = 0x%x, L = 0x%x, flag %x}\n",
    //                 contentCommand->SequenceNumber,
    //                 contentCommand->Address,
    //                 contentCommand->Length,
    //                 contentCommand->Flags);
    //DFU_PRINT_TRACE1("data %b", TRACE_BINARY(contentCommand->Length, contentCommand->Data));
    if (((contentCommand->Flags & COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK) ==
         COMPONENT_FIRMWARE_UPDATE_FLAG_FIRST_BLOCK))
    {
        if (contentCommand->Data[4] != 0x8e || contentCommand->Data[5] != 0x27)
        {
            contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
                COMPONENT_FIRMWARE_UPDATE_ERROR_SIGNATURE;
            app_hid_interrupt_in(app_cfu_response_buffer, sizeof(FWUPDATE_OFFER_RESPONSE));
            return 0;
        }
        addr_count = 0;
        result = flash_erase_locked(FLASH_ERASE_SECTOR, SYS_AREA_ADDR);
        result = flash_erase_locked(FLASH_ERASE_SECTOR, SYS_AREA_ADDR + 0x1000);
    }
    result = flash_write_locked((SYS_AREA_ADDR + addr_count), 52, contentCommand->Data);
    addr_count += 52;
    contentResponse->HidCfuContentResponse.ReportId = REPORT_ID_CFU_PAYLOAD_INPUT;
    contentResponse->HidCfuContentResponse.CfuContentResponse.SequenceNumber =
        contentCommand->SequenceNumber;
    contentResponse->HidCfuContentResponse.CfuContentResponse.Status =
        COMPONENT_FIRMWARE_UPDATE_SUCCESS;
    if (((contentCommand->Flags & COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK) ==
         COMPONENT_FIRMWARE_UPDATE_FLAG_LAST_BLOCK))
    {
        app_start_timer(&timer_idx_cfu_reset, "app_cfu_timer_reset",
                        app_cfu_timer_id, CFU_TIMER_RESET_ACTIVE, 0, false,
                        1000);
    }
    app_hid_interrupt_in(app_cfu_response_buffer, sizeof(FWUPDATE_OFFER_RESPONSE));
    return true;
#else
    return false;
#endif
    return true;
}
#endif // NEED_CHANGE_SYS_CONFIG
#endif // CFU debug
