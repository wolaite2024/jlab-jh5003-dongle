/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    fmc_api.h
* @brief   This file provides fmc api wrapper for sdk customer.
* @author  yuhungweng
* @date    2020-11-26
* @version v1.0
* *************************************************************************************
*/

#ifndef __FMC_API_H_
#define __FMC_API_H_

#include <stdint.h>
#include "storage.h"

/** @defgroup  87x3e_FMC    FMC
    * @brief FMC wrapper.
    * @{
    */

/*============================================================================*
  *                                   Macros
  *============================================================================*/
/** @defgroup 87x3e_FMC_Exported_Macros FMC Exported Macros
    * @brief
    * @{
    */
#define FMC_SEC_SECTION_LEN 0x1000
#define FMC_SEC_BLOCK_LEN   0x10000

#define IS_FLASH_SIZE_LARGER_THAN_2MB()            (fmc_flash_get_flash_size(0) > (1 << 21))
/** End of 87x3e_FMC_Exported_Macros
    * @}
    */

/*============================================================================*
  *                                   Types
  *============================================================================*/
/** @defgroup 87x3e_FMC_TYPE FMC Exported Types
   * @brief
   * @{
   */

typedef enum
{
    FMC_FLASH_NOR_ERASE_CHIP   = 1,
    FMC_FLASH_NOR_ERASE_SECTOR = 2,
    FMC_FLASH_NOR_ERASE_BLOCK  = 4,
} FMC_FLASH_NOR_ERASE_MODE;


typedef enum
{
    PARTITION_FLASH_OCCD,
    PARTITION_FLASH_OTA_BANK_0,
    PARTITION_FLASH_OTA_BANK_1,
    PARTITION_FLASH_OTA_TMP,
    PARTITION_FLASH_BKP_DATA1,
    PARTITION_FLASH_BKP_DATA2,
    PARTITION_FLASH_FTL,
    PARTITION_FLASH_HARDFAULT_RECORD,

    PARTITION_FLASH_TOTAL,

} T_FLASH_PARTITION_NAME;

typedef enum _FLASH_IMG_ID
{
    FLASH_IMG_OTA         = 0, /* OTA header */
    FLASH_IMG_SBL         = 1,
    FLASH_IMG_MCUPATCH    = 2,
    FLASH_IMG_MCUAPP      = 3,
    FLASH_IMG_DSPSYSTEM   = 4,
    FLASH_IMG_DSPAPP      = 5,
    FLASH_IMG_MCUCONFIG   = 6,
    FLASH_IMG_DSPCONFIG   = 7,
    FLASH_IMG_ANC         = 8,
    FLASH_IMG_EXT1        = 9,
    FLASH_IMG_EXT2        = 10,
    FLASH_IMG_ADSP        = 11,
    FLASH_IMG_SYSPATCH    = 12,
    FLASH_IMG_STACKPATCH  = 13,
    FLASH_IMG_UPPERSTACK  = 14,
    FLASH_IMG_FRAMEWORK   = 15,
    FLASH_IMG_SYSDATA     = 16,
    FLASH_IMG_MAX,
} FLASH_IMG_ID;

typedef void (*FMC_FLASH_NOR_ASYNC_CB)(void);

typedef enum
{
    FMC_SPIC_ID_0,
    FMC_SPIC_ID_1,
    FMC_SPIC_ID_2,
    FMC_SPIC_ID_3,
    FMC_SPIC_ID_4,
    FMC_SPIC_ID_MAX,
} FMC_SPIC_ID;

typedef enum
{
    DSP_CFG_IMG_1,
    DSP_CFG_IMG_2,
} T_DSP_CFG_IMG_ID;

/** End of 87x3e_FMC_TYPE
    * @}
    */

/*============================================================================*
  *                                Functions
  *============================================================================*/
/** @defgroup 87x3e_FMC_Exported_Flash_Functions FMC Exported Flash Functions
    * @brief
    * @{
    */

T_DSP_CFG_IMG_ID flash_dsp_cfg_id_get(void);

void flash_dsp_cfg_switch(T_DSP_CFG_IMG_ID img_id);

uint32_t flash_dsp_cfg_addr_get(void);

/**
 * @brief           Task-safe NOR flash read.
 * @param addr      The RAM address mapping of NOR flash going to be read.
 * @param data      The buffer to be read into.
 * @param len       The length of read data.
 * @return          True if read successful, otherwise false.
 */
bool fmc_flash_nor_read(uint32_t addr, void *data, uint32_t len);

/**
 * @brief           Task-safe NOR flash write.
 * @param addr      The RAM address mapping of NOR flash going to be written.
 * @param data      The buffer to be written into.
 * @param len       The length of write data.
 * @return          True if write successful, otherwise false.
 */
bool fmc_flash_nor_write(uint32_t addr, void *data, uint32_t len);

/**
 * @brief           Task-safe NOR flash erase.
 * @param addr      The RAM address mapping of NOR flash going to be erased.
 * @param mode      Erase mode defined as @ref FMC_FLASH_NOR_ERASE_MODE.
 * @return          True if erase successful, otherwise false.
 */
bool fmc_flash_nor_erase(uint32_t addr, FMC_FLASH_NOR_ERASE_MODE mode);

/**
 * @brief           Task-safe NOR flash auto DMA read.
 * @param src       The RAM address mapping of NOR flash going to be read from.
 * @param dst       The RAM address going to be written to.
 * @param len       DMA data length.
 * @param cb        Callback function to be executed when DMA finishes.
 * @return          True if trigger auto DMA read successful, otherwise false.
 */
bool fmc_flash_nor_auto_dma_read(uint32_t src, uint32_t dst, uint32_t len,
                                 FMC_FLASH_NOR_ASYNC_CB cb);
/**
 * @brief           Switch NOR flash clock.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 * @param idx       The index of SPIC.
 * @param required_mhz Clock need to be set.
 * @param actual_mhz Used to pass back actual flash clock.
 * @return          True if switch clock successful, otherwise false.
 */
bool fmc_flash_nor_clock_switch(FMC_SPIC_ID idx, uint32_t required_mhz, uint32_t *actual_mhz);

/**
 * @brief           Get the address of a flash partition.
 * @param name      The partition name in @ref T_FLASH_PARTITION_NAME.
 * @return          The address of the flash partition.
 */
uint32_t flash_partition_addr_get(T_FLASH_PARTITION_NAME name);

/**
 * @brief           Get the size of a flash partition.
 * @param name      The partition in @ref T_FLASH_PARTITION_NAME.
 * @return          The size of the flash partition.
 */
uint32_t flash_partition_size_get(T_FLASH_PARTITION_NAME name);

/**
 * @brief           Get the payload address of an image.
 * @param id        The image id in @ref FLASH_IMG_ID.
 * @return          The payload address of the image.
 */
uint32_t flash_cur_bank_img_payload_addr_get(FLASH_IMG_ID id);

/**
 * @brief           Get the header address of an image.
 * @param id        The image id in @ref FLASH_IMG_ID.
 * @return          The header address of the image.
 */
uint32_t flash_cur_bank_img_header_addr_get(FLASH_IMG_ID id);

/**
 * @brief           NOR flash set block protect level.
 * @param addr      The RAM address mapping of NOR flash.
 * @param bp_lv     The block protect level to be set.
 * @return          True if set block protect level is successful, otherwise false.
 */
bool fmc_flash_nor_set_bp_lv(uint32_t addr, uint8_t bp_lv);

/**
 * @brief           Get NOR flash block protect level.
 * @param addr      The RAM address mapping of NOR flash.
 * @param bp_lv     Used to pass back the block protect level.
 * @return          True if get bp level is successful, otherwise false.
 */
bool fmc_flash_nor_get_bp_lv(uint32_t addr, uint8_t *bp_lv);

/**
 * @brief           Get NOR flash OTP lock status.
 * @param is_lock   Used to pass back the lock status.
 * @return          True if get lock status is successful, otherwise false.
 */
bool fmc_flash_nor_get_otp(bool *is_lock);

/**
 * @brief           Set NOR flash OTP lock status.
 * @param is_lock   The lock status to be set.
 * @return          True if set lock status is successful, otherwise false.
 */
bool fmc_flash_nor_set_otp(bool is_lock);

/**
 * @brief           NOR flash write OTP
 * @param addr      The address mapping of OTP to be written.
 * @param otp_data  Data buffer to be written into.
 * @param size      Write data length.
 * @return          True if write is successful, otherwise false.
 */
bool fmc_flash_nor_write_otp(uint32_t addr, uint8_t *otp_data, uint32_t size);

/**
 * @brief           Erase flash OTP.
 * @param addr      The address mapping of OTP going to be erased.
 * @return          True if erase is successful, otherwise false.
 */
bool fmc_flash_nor_erase_otp(uint32_t addr);

/**
 * @brief           Read flash OTP.
 * @param addr      The address mapping of OTP going to be read.
 * @param otp_data  Data buffer to be read into.
 * @param size      Read data length.
 * @return          True if read is successful, otherwise false.
 */
bool fmc_flash_nor_read_otp(uint32_t addr, uint8_t *otp_data, uint32_t size);

/**
 * @brief           Get the UUID of NOR flash.
 * @param uuid      Used to pass back the UUID of NOR flash.
 * @param size      The size of UUID.
 * @return          True if getting the UUID is successful, otherwise false.
 */
bool fmc_flash_nor_get_uuid(uint8_t *uuid, uint8_t size);

/**
 * @brief           Init flash OTP (One Time Programmable).
 * @return          True if init is successful, otherwise false.
 */
bool fmc_flash_nor_otp_init(void);

/**
 * @brief           Get flash size.
 *\xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
 * @param idx       SPIC channel number.
 * @return          The Byte size of current flash.
 */
uint32_t fmc_flash_get_flash_size(uint8_t idx);

/** @} */ /* End of group 87x3e_FMC_Exported_Flash_Functions */



/** @defgroup 87x3e_FMC_Exported_PSRAM_Functions FMC Exported PSRAM Functions
    * @brief
    * @{
    */


/**
 * @brief           Init Winbond OPI PSRAM (W955D8MBYA).
 * @return          True if init is successful, otherwise false.
 */
bool fmc_psram_winbond_opi_init(void);

/**
 * @brief           Init APMemory QPI PSRAM (APS1604M-SQR) which only supports quad mode
 * @return          True if init is successful, otherwise false.
 */
bool fmc_psram_ap_memory_qpi_init(void);

/**
 * @brief           Init APMemory OPI PSRAM (APS6408L-OBx).
 * @return          True if init is successful, otherwise false.
 */
bool fmc_psram_ap_memory_opi_init(void);

/**
 * @brief           PSRAM mode switch.
 * @return          True if the mode switch is successful, otherwise false.
 */
bool fmc_psram_mode_switch_quad(void);
/**
 * @brief           SPIC set clock.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * @param spic_id       The index of SPIC.
 * @param  require_mhz  Required SPIC frequency.
 * @param  actual_mhz     The frequency of current SPIC.
 * @return              True if switching the clock is successful, otherwise false.
 */
bool fmc_spic_clock_switch(FMC_SPIC_ID spic_id, uint32_t required_mhz, uint32_t *actual_mhz);


//bool fmc_psram_clock_switch(CLK_FREQ_TYPE clk);

/** @} */ /* End of group 87x3e_FMC_Exported_PSRAM_Functions */

/** @} */ /* End of group 87x3e_FMC */

#endif  // __FMC_API_H_



