/**
*****************************************************************************************
*     Copyright(c) 2019, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_ota.h
   * @brief     Head file for using OTA service
   * @author    Michael
   * @date      2019-11-25
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2019 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/
#ifndef _DFU_COMMON_H_
#define _DFU_COMMON_H_

#include "patch_header_check.h"
#include "rtl876x.h"
#include "wdg.h"
/** @defgroup  APP_OTA_SERVICE APP OTA handle
    * @brief APP OTA Service to implement OTA feature
    * @{
    */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_OTA_SERVICE_Exported_Macros App OTA service Exported Macros
    * @brief
    * @{
    */
#define DFU_COMMON_DEBUG                           0
#define FLASH_OFFSET_TO_NO_CACHE                   0x02000000
// if there is no cache, then UNCACHEABLE_ADDR should equal to FMC_MAIN0_ADDR
#define FMC_MAIN0_ADDR                             (0x02000000)
#define FMC_MAIN0_UNCACHEABLE_ADDR                 (0x02000000)
#define FLASH_TABLE_MAGIC_PATTERN                  0x5a5a12a5
#define FLASH_SECTOR_SIZE                          0x1000
#define FMC_MAIN0_NON_CACHE_ADDR(cache_addr)       ((cache_addr) & ~(FMC_MAIN0_ADDR) | (FMC_MAIN0_UNCACHEABLE_ADDR))

#define OTA_HEADER_SIZE                            1024

#define PLATFORM_STATIC_ASSERT(condition, identifier) typedef char PALStaticAssert_##identifier[(condition) ? 1 : -1]

#define SET_VALID_BITMAP(image_id)      (valid_bitmap |= BIT(image_id - IMG_OTA))
#define GET_VALID_BITMAP(image_id)      (valid_bitmap >> (image_id - IMG_OTA))

/** End of APP_OTA_SERVICE_Exported_Macros
    * @}
    */


/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_OTA_SERVICE_Exported_Types APP OTA Service Exported Types
    * @brief
    * @{
    */
typedef enum
{
    DFU_ACTIVE_RESET = 0xd0,
    DFU_SYSTEM_RESET = 0xd1,
    DFU_TIMEOUT,
    DFU_SWITCH_TO_OTA_MODE,
    DFU_LINK_LOST,
} T_RESET_REASON;

typedef enum
{
    USER_DATA_SUCCESS = 0,
    USER_DATA_NOT_SUPPORT_OTA,
    USER_DATA_TYPE_ERROR,
} T_USER_DATA_ERROR_TYPE;
/** End of APP_OTA_SERVICE_Exported_Types
    * @}
    */
extern uint32_t valid_bitmap;
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_OTA_SERVICE_Exported_Functions APP OTA service Functions
    * @brief
    * @{
    */
uint32_t get_bank_size_by_img_id(IMG_ID image_id);

void dfu_fw_reboot(T_WDG_MODE reset_mode, T_RESET_REASON reason);

bool dfu_check_ota_mode_flag(void);

void dfu_set_ota_mode_flag(bool enable);

void dfu_switch_to_ota_mode(void);

/**
    * @brief    get the ic type of current fw
    * @param    void
    * @return   ic type
    */
uint8_t dfu_get_ic_type(void);


/**
    * @brief    check if ota header exist
    * @param    header_addr    ota header address
    * @return   true: exist; false: not exist
    */
bool check_ota_header(uint32_t header_addr);

/**
    * @brief    get inactive bank's image address
    * @param    image_id   image id
    * @return   image address
    */
uint32_t get_temp_ota_bank_img_addr_by_img_id(IMG_ID image_id);

/**
    * @brief    get inactive bank's image size
    * @param    image_id   image id
    * @return   image size
    */
uint32_t get_temp_ota_bank_img_size_by_img_id(IMG_ID image_id);

/**
* @brief calculate checksum of lenth of buffer in flash.
*
* @param  signature          signature to identify FW.
* @param  offset             offset of the image.
* @param  length             length of data.
* @param  crcValue          ret crc value point.
* @return  0 if buffer checksum calcs successfully, error line number otherwise
*/

uint32_t dfu_check_bufcrc(uint8_t *buf, uint32_t length, uint16_t mCrcVal);

/**
 * @brief erase a sector of the flash.
 *
 * @param  signature          signature to identify FW.
 * @param  offset             offset of the image.
 * @return  0 if erase successfully, error line number otherwise
*/
uint32_t dfu_flash_erase(IMG_ID image_id, uint32_t offset);

/**
    * @brief    write data to flash
    * @param    img_id  image id
    * @param    offset  image offset
    * @param    total_offset  total offset when ota temp mode
    * @param    p_void  point of data
    * @return   0: success; other: fail
    */
uint32_t dfu_write_data_to_flash(uint16_t img_id, uint32_t offset, uint32_t total_offset,
                                 uint32_t length, void *p_void);

/**
    * @brief    check the integrity of the image
    * @param    image_id    image id
    * @param    offset  address offset
    * @return   ture:success ; false: fail
    */
bool dfu_checksum(IMG_ID image_id, uint32_t offset);

/**
    * @brief    clear not ready flag of specific image
    * @param    addr    address of the image
    * @return   void
    */
void dfu_set_ready(T_IMG_HEADER_FORMAT *p_header);

/**
    * @brief    clear not obsolete flag of specific image
    * @param    addr    address of the image
    * @return   void
    */
void dfu_set_obsolete(T_IMG_HEADER_FORMAT *p_header);
#if 0
T_USER_DATA_ERROR_TYPE dfu_get_user_data_info(USER_IMG_ID image_id,
                                              uint32_t *img_info, bool is_addr);
#endif
/** @} */ /* End of group APP_OTA_SERVICE_Exported_Functions */

/** @} */ /* End of group APP_OTA_SERVICE */
#endif
