#include "stdint.h"
#include "stdbool.h"
#include "patch_header_check.h"
//#include "data_storage.h"
#include "flash_map.h"
#include "app_cfg.h"

#ifndef _APP_CFU_H_
#define _APP_CFU_H_

#define CFU_DEBUG 1
#define CFU_DEBUG_LOG_ONLY 0

#ifdef CUSTOMER_FIXED_SETTINGS
#define USE_CUSTOMER_SIGN       0
#define UES_OFFER_NO_SKIP       1
#if USE_CUSTOMER_SIGN
#define USE_4K_KEEP_BUFFER      1
#define ENABLE_GOD_MODE         1
#define USE_VERIFY_AFTER_RESET  1
#endif
#include "CfuMSignProcess.h"
#define MS_HEADER_TEMP_AREA_ID      IMG_MCUAPP
#define MS_HEADER_TEMP_AREA_ADDR    error
#define MS_HEADER_TEMP_AREA_SIZE    0x1000
#define MS_HEADER_HASH_HEADER_SIZE  72
#define MS_HEADER_SIG_OFFSET        0x400
#endif

/** @brief image exit indicator. */
#define APP_CFU_IMAGE_LOCATION_BANK0            0
#define APP_CFU_IMAGE_LOCATION_BANK1            1
#define APP_CFU_NOT_SUPPORT_BANK_SWITCH         3

#define MAX_ACCEPTED_OFFER_NUM                  2

#define OFFER_PROCESS_NOT_GOT           0
#define OFFER_PROCESS_SKIPPED           1
#define OFFER_PROCESS_REJECTED          2
#define OFFER_PROCESS_ACCEPTED          3

#define CFU_CONTENT_DATA_LENGTH         52

#define CFU_TIMER_CONTENT_TRANS         1
#define CFU_TIMER_RESET_ACTIVE          2

// NOTE - defines should match CFU Protocol Spec definitions
#define CFU_OFFER_METADATA_INFO_CMD                        (0xFF)
#define CFU_SPECIAL_OFFER_CMD                              (0xFE)
#define CFU_SPECIAL_OFFER_GET_STATUS                       (0x03)
#define CFU_SPECIAL_OFFER_NONCE                            (0x02)
#define CFU_SPECIAL_OFFER_NOTIFY_ON_READY                  (0x01)
#define CFW_UPDATE_PACKET_MAX_LENGTH                       (sizeof(FWUPDATE_CONTENT_COMMAND))
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

#define CFU_VERSION_SECTION     IMG_OTA
#define CFU_DONGLE_COMPONENT_ID   0xA2

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

#pragma pack(1)
typedef struct _CFU_HEADER
{
    uint16_t image_id;
    uint32_t version;
    uint32_t end_addr;
    uint32_t length;
} CFU_HEADER;
#pragma pack()

#if ENABLE_GOD_MODE
extern uint8_t customer_cfu_sign_flag;
#else
#define customer_cfu_sign_flag 0
#endif

void app_cfu_init(void);
void app_cfu_test(uint8_t *data, uint16_t length);
void app_cfu_handle_get_report(uint8_t *data, uint16_t *length);
void app_cfu_handle_set_report(uint8_t *data, uint16_t length);
uint32_t app_cfu_get_version(bool active);
void app_cfu_set_dongle_rush_level(uint8_t level);
void app_cfu_dump_image_ver(void);
#endif
