/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     ota_ext.h
* @brief    This file provides OTA extern API for sdk customer.
* @details
* @author   huan_yang
* @date
* @version  v1.0
*********************************************************************************************************
*/
/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/

#ifndef __OTA_EXT_H_
#define __OTA_EXT_H_


/*============================================================================*
 *                               Header Files
*============================================================================*/

#include <stdint.h>
/** @defgroup  HAL_87x3e_OTA_EXT    OTA Extern
    * @brief  OTA extern API.
    * @{
    */
/*============================================================================*
 *                              Variables
*============================================================================*/
/** @defgroup HAL_87x3e_OTA_EXT_EXPORTED_MACROS OTA Extern Exported Macros
    * @brief
    * @{
    */
#define IMG_HEADER_SIZE     0x400

/** End of HAL_87x3e_OTA_EXT_EXPORTED_MACROS
    * @}
    */
/*============================================================================*
 *                              Functions
*============================================================================*/
/** @defgroup HAL_87x3e_OTA_EXT_Exported_Functions OTA Extern Exported Functions
    * @brief
    * @{
    */

#ifdef __cplusplus
extern "C" {
#endif


/**
    * @brief    Get active bank's image address.
    * @param    image_id   Image ID.
    * @return   Image address.
    */
uint32_t app_get_active_bank_addr(uint16_t image_id);

/**
    * @brief    Get inactive bank's image address.
    * @param    image_id   Image ID.
    * @return   Image address.
    */
uint32_t app_get_inactive_bank_addr(uint16_t image_id);

/**
    * @brief    Check image integrity.
    * @param    base_addr Image start address.
    * @return       Check result.
    * @retval true  Success.
    * @retval false Fail.
    */
bool check_image_sum(uint32_t base_addr);

/**
    * @brief    Check section size before ota.
    * @param    image_total_length Image total length.
    * @param    image_id Image ID.
    * @return       Check result.
    * @retval true  Success.
    * @retval false Fail.
    */
bool app_ota_check_section_size(uint32_t image_total_length, uint16_t image_id);

/**
    * @brief    Check image ID.
    * @param    img_id    Image ID.
    * @return       Check result.
    * @retval true  Success.
    * @retval false Fail.
    */
bool check_image_id(uint16_t img_id);

/**
    * @brief    Clear not ready flag of specific image.
    * @param    addr    address of the image.
    * @return   void
    */
void app_ota_set_ready(uint32_t addr);

/**
    * @brief    Clear not ready flag of all the image.
    * @param    void
    * @return   void
    */
void app_ota_clear_notready_flag(void);

/**
    * @brief    Change the sequence of app config bin and dsp config bin.
    * @param    image_id   Image ID.
    * @return   Image ID.
    */
IMG_ID app_change_seq_appdata_dspdata(IMG_ID image_id);

/**
    * @brief    Used to get image version.
    * @param    p_data   Point of image version.
    * @param    bank   0:active bank    1:inactive bank.
    * @return   void
    */
void app_ota_get_img_version(uint8_t *p_data, uint8_t bank);

/**
    * @brief    Used to get image section size.
    * @param    p_data  Point of the section size.
    * @return   void
    */
void app_ota_get_section_size(uint8_t *p_data);

/**
    * @brief    Check copy image ID.
    * @param    img_id    Image ID.
    * @return       Check result.
    * @retval true  Success.
    * @retval false Fail.
    */
bool check_copy_image_id(uint16_t img_id);

/**
    * @brief    Decrypt with 16byte input.
    * @param    input    Data need to be decrypted.
    * @return   void
    */
void aes256_decrypt_16byte(uint8_t *input);

/**
 * @brief Erase a sector of the flash.
 *
 * @param  signature          Signature to identify FW.
 * @param  offset             Offset of the image.
 * @return  0 if erase successfully, error line number otherwise.
*/
uint32_t dfu_flash_erase(uint16_t signature, uint32_t offset);

/**
* @brief calculate checksum of lenth of buffer in flash.
*
* @param  signature          Signature to identify FW.
* @param  offset             Offset of the image.
* @param  length             Length of data.
* @param  crcValue           Ret crc value point.
* @return  0 if buffer checksum calcs successfully, error line number otherwise
*/

uint32_t dfu_checkbufcrc(uint8_t *buf, uint32_t length, uint16_t mCrcVal);

/**
 * @brief  Erase specific flash area.
 * @param  start_addr       Start addr of the erea.
 * @param  size             Length of area to erase, input NULL if erase the whole area.
 * @return       Erase result.
 * @retval true  Success.
 * @retval false Fail.
*/
bool dfu_erase_img_flash_area(uint32_t start_addr, uint32_t size);

/**
 * @brief  Use flash dma mode to write.
 *\xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 * @param  dest_addr       Destination of the copy address.
 * @param  buf             The pointer of data to be copied to flash.
 * @param  len             The length of data to be copied to flash.
 * @return 0 if succeed, error number if failed.
*/
int ota_flash_dma_write(size_t dest_addr, void *buf, size_t len);

/**
 * @brief  Init OTA flash dma write.
 *\xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 * @param  void
 * @return 0 if succeed, error number if failed.
*/
int ota_flash_dma_write_init(void);

/**
 * @brief  Deinit OTA flash dma write.
 *\xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 * @param  void
 * @return void
*/
void ota_flash_dma_write_deinit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
/** @} */ /* End of group HAL_87x3e_OTA_EXT_Exported_Functions */
/** @} */ /* End of group HAL_87x3e_OTA_EXT */
#endif /* __OTA_EXT_H_ */


