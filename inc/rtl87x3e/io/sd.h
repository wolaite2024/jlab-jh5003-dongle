/**
*********************************************************************************************************
*               Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     sd.h
* @brief    This file provides application functions for sd card or emmc card library.
* @details
* @author   qinyuan_hu
* @date     2025-04-29
* @version  v1.2
*********************************************************************************************************
*/

#ifndef __SD_H
#define __SD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "sd_define.h"

/** @addtogroup 87x3e_SD SD
  * @brief SD driver module.
  * @{
  */

/* Defines ------------------------------------------------------------------*/
/** @defgroup 87x3e_SD_Exported_Types SD Exported Types
  * @{
  */

typedef void(*RW_DONE_CBACK)(void *); //!< Read and write done callback.

typedef enum
{
    GROUP_0,   //!< Select the SD pin group0.
    GROUP_1,   //!< Select the SD pin group1.
    GROUP_MAX, //!< SD pin group selection need less than the max value.
} T_SDIO_PIN_GROUP;

typedef enum
{
    SD_IF_NONE      = 0x00, //!< No card is selected.
    SD_IF_SD_CARD   = 0x01, //!< SD card is selected.
    SD_IF_MMC       = 0x02, //!< MMC card is selected.
} T_SD_IF_TYPE;

typedef enum
{
    SD_BUS_WIDTH_1B,     //!< SD bus width is 1bit mode.
    SD_BUS_WIDTH_4B = 2, //!< SD bus width is 4bit mode.
    SD_BUS_WIDTH_MAX     //!< SD bus width selection need less than the max value.
} T_SD_BUS_WIDTH;

typedef enum
{
    /*80M 40M only 8773do support*/
    SD_BUS_CLK_80M = 0x100,   //!< Set SD output clk is 80MHz.
    SD_BUS_CLK_40M = 0x101,   //!< Set SD output clk is 40MHz.

    SD_BUS_CLK_20M = 0x00,    //!< Set SD output clk is 20MHz.
    SD_BUS_CLK_10M = 0x01,    //!< Set SD output clk is 10MHz.
    SD_BUS_CLK_5M = 0x02,     //!< Set SD output clk is 5MHz.
    SD_BUS_CLK_2M5 = 0x04,    //!< Set SD output clk is 2.5MHz.
    SD_BUS_CLK_1M25 = 0x08,   //!< Set SD output clk is 1.25MHz.
    SD_BUS_CLK_625K = 0x10,   //!< Set SD output clk is 625KHz.
    SD_BUS_CLK_312K5 = 0x20,  //!< Set SD output clk is 312.5KHz.
    SD_BUS_CLK_156K25 = 0x40, //!< Set SD output clk is 156.25KHz.
    SD_BUS_CLK_78K125 = 0x80, //!< Set SD output clk is 78.125KHz.

} T_SD_BUS_CLK_FREQ;

/**
  * @brief  SD configure structure definition.
  */
typedef struct
{
    uint8_t     sd_if_type: 2;           //!< Config the SD card type.
    uint8_t     rsv0: 2;                 //!< Reserved.
    uint8_t     sd_power_en: 1;          //!< Enable or disable the SD power pin config.
    uint8_t     sd_power_high_active: 1; //!< Config the SD power pin to high or low level.
    uint8_t     sdh_bus_width: 2;        //!< Config the SD bus width.

    uint8_t     sdh_group: 2;            //!< Config the SD pin group.
    uint8_t     sd_debug_en: 1;          //!< Config the SD debug enable.
    uint8_t     sd_mutex_en: 1;          //!< Config the SD mutex semaphores for SD multi-task.
    uint8_t     rsv1: 4;                 //!< Reserved.
    uint16_t    sd_bus_clk_sel;          //!< Config the SD bus output clk.
    uint8_t     sd_power_pin;            //!< Config the SD power pin.
    RW_DONE_CBACK rw_cback;              //!< Config the SD read/write callback.
    void       *rw_user_data;            //!< Config the SD user data of read/write callback.
} T_SD_CONFIG;

typedef enum
{
    SDH_CLK,      //!< SD clock pin.
    SDH_CMD,      //!< SD command pin.
    SDH_D0,       //!< SD data0 pin.
    SDH_D1,       //!< SD data1 pin.
    SDH_D2,       //!< SD data2 pin.
    SDH_D3,       //!< SD data3 pin.
    SDH_WT_PROT,  //!< SD write protect pin.
    SDH_CD,       //!< SD card detect pin.
} T_SD_PIN_NUM;

/** End of group 87x3e_SD_Exported_Types
  * @}
  */

/* Exported functions -------------------------------------------------------*/

/** @defgroup 87x3e_SD_Exported_Functions SD Exported Functions
  * @{
  */

/**
 *
 * \brief   Config the SD card type information for SD card.
 *
 * \param[in]  app_sd_cfg    Point to SD card configuration parameters, please refer to \ref T_SD_CONFIG.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * static const T_SD_CONFIG    sd_card_cfg =
 * {
 *     .sd_if_type = SD_IF_SD_CARD,
 *     .sdh_group = GROUP_0,
 *     .sdh_bus_width = SD_BUS_WIDTH_4B,
 *     .sd_bus_clk_sel = SD_BUS_CLK_20M
 * };
 *
 * void sd_init(void)
 * {
 *    sd_config_init((T_SD_CONFIG *)&sd_card_cfg);
 * }
 *
 * \endcode
 *
 */
void sd_config_init(T_SD_CONFIG *app_sd_cfg);

/**
 *
 * \brief   Config the PAD and PINMUX for SD card.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_init(void)
 * {
 *    sd_config_init((T_SD_CONFIG *)&sd_card_cfg);
 *    sd_board_init(SDHC_ID0);
 * }
 *
 * \endcode
 *
 */
void sd_board_init(void);

/**
 *
 * \brief   Initial the SD card.
 *
 * \return The SD card init status, 0 is SD_OK. Please refer to \ref T_SD_STATUS.
 * \retval 0         The SD card was initialized successfully.
 * \retval 0x1-0xFF  The SD card was failed to initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_init(void)
 * {
 *    sd_config_init((T_SD_CONFIG *)&sd_card_cfg);
 *    sd_board_init(SDHC_ID0);
 *    sd_card_init(SDHC_ID0);
 * }
 *
 * \endcode
 *
 */
T_SD_STATUS sd_card_init(void);

/**
 *
 * \brief   Suspend the SD host by disable SD CLK, when SD card is not powered off. Use sd_resume can be restored.
 *          At this time, SD card does not need to be powered on and initialized again.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_test(void)
 * {
 *    sd_suspend(SDHC_ID0);
 * }
 *
 * \endcode
 *
 */
void sd_suspend(void);

/**
 *
 * \brief   Resume the PAD config and SD host controller power on.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_test(void)
 * {
 *    sd_resume(SDHC_ID0);
 * }
 *
 * \endcode
 *
 */
void sd_resume(void);

/**
 *
 * \brief   Initialize the SD PAD, SD host and SD card, when SD card is powered off.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * \return The SD card init status, 0 is SD_OK. Please refer to \ref T_SD_STATUS.
 * \retval 0         The SD card was initialized successfully.
 * \retval 0x1-0xFF  The SD card was failed to initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_test(void)
 * {
 *    sd_init(SDHC_ID0);
 * }
 *
 * \endcode
 *
 */
T_SD_STATUS sd_init(void);

/**
 *
 * \brief   Print the SD card binary data by TRACE_BINARY.
 *
 * \param[in]  p    Point to test buffer for SD read or write. This parameter must range from 0x0 to 0xFF.
 * \param[in]  len  The data length to be printed. This parameter must range from 0x0 to 0xFFFFFFFF.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define OPER_SD_CARD_ADDR       ((uint32_t)0x8000)
 * uint8_t *test_buf = NULL;
 *
 * void sd_test(void)
 * {
 *    test_buf = os_mem_alloc(OS_MEM_TYPE_BUFFER, 512);
 *    sd_read(OPER_SD_CARD_ADDR, (uint32_t)test_buf, 512, 1);
 *    sd_print_binary_data(test_buf, 512);
 * }
 * \endcode
 *
 */
void sd_print_binary_data(uint8_t *p, uint32_t len);

/**
 *
 * \brief   Erase SD card from the specified start address to end address.
 *
 * \param[in]  start_addr    The SD card start address to be erased. This parameter must range from 0x0 to 0xFFFFFFFF.
 *
 * \param[in]  end_addr     The SD card end address to be erased. This parameter must range from 0x0 to 0xFFFFFFFF.
 *
 * \return The SD card erase status, 0 is SD_OK. Please refer to \ref T_SD_STATUS.
 * \retval 0         The SD card was erased successfully.
 * \retval 0x1-0xFF  The SD card was failed to erased.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_test(void)
 * {
 *     sd_erase(start_addr, end_addr);
 * }
 * \endcode
 *
 */
T_SD_STATUS sd_erase(uint32_t start_addr, uint32_t end_addr);

/**
 *
 * \brief   Read data from a specified address in SD card.
 *
 * \param[in]  sector    The specified SD card address to read. This parameter must range from 0x0 to 0xFFFFFFFF.
 *
 * \param[in]  buf       The buffer of SD card to read data. The buf address must be 4 bytes aligned and must range from 0x0 to 0xFF.
 *
 * \param[in]  blk_size   The block size of SD card to read. This parameter is recommended to set 512.
 *
 * \param[in]  blk_num   The block number of SD card to read. This parameter must range from 0x0 to 0xFFFF.
 *
 * \return The SD card read status, 0 is SD_OK. Please refer to \ref T_SD_STATUS.
 * \retval 0         The SD card was read successfully.
 * \retval 0x1-0xFF  The SD card was failed to read.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_test(void)
 * {
 *     uint32_t sd_status = 0;
 *     sd_status = sd_read(OPER_SD_CARD_ADDR, (uint32_t)test_buf, SINGLE_BLOCK_SIZE, BLOCK_NUM);
 *     if (sd_status != 0)
 *     {
 *        IO_PRINT_ERROR0("sd_read fail");
 *        return ;
 *     }
 * }
 * \endcode
 *
 */
T_SD_STATUS sd_read(uint32_t sector, uint32_t buf, uint16_t blk_size, uint16_t blk_num);

/**
 *
 * \brief   Write data from a specified address in SD card.
 *
 * \param[in]  sector    The specified SD card address to write. This parameter must range from 0x0 to 0xFFFFFFFF.
 *
 * \param[in]  buf       The buffer of SD card to write data. The buf address must be 4 bytes aligned and must range from 0x0 to 0xFF.
 *
 * \param[in]  blk_size   The block size of SD card to write. This parameter is recommended to set 512.
 *
 * \param[in]  blk_num   The block number of SD card to write. This parameter must range from 0x0 to 0xFFFF.
 *
 * \return The SD card write status, 0 is SD_OK. Please refer to \ref T_SD_STATUS.
 * \retval 0         The SD card was write successfully.
 * \retval 0x1-0xFF  The SD card was failed to write.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_test(void)
 * {
 *     uint32_t sd_status = 0;
 *     sd_status = sd_write(OPER_SD_CARD_ADDR, (uint32_t)test_buf, SINGLE_BLOCK_SIZE, BLOCK_NUM);
 *     if (sd_status != 0)
 *     {
 *        IO_PRINT_ERROR0("sd_write fail");
 *        return ;
 *     }
 * }
 * \endcode
 *
 */
T_SD_STATUS sd_write(uint32_t sector, uint32_t buf, uint16_t blk_size, uint16_t blk_num);

/**
 *
 * \brief   Set block length for standard capacity SD card.
 *
 * \param[in]   block_len   The block length to set. This parameter must range from 0x0 to 0xFFFFFFFF.
 *
 * \return The SD card set block length status, 0 is SD_OK. Please refer to \ref T_SD_STATUS.
 * \retval 0         The SD card block length was set successfully.
 * \retval 0x1-0xFF  The SD card block length was failed to set.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void sd_test(void)
 * {
 *     sd_set_block_len(block_len);
 * }
 * \endcode
 *
 */
T_SD_STATUS sd_set_block_len(uint32_t block_len);

/**
 *
 * \brief   Start or stop the SD host operation clock.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in]  NewState   New state of the SD host operation clock, can be set to true or false.
 *                        - true   Start the SD host operation clock.
 *                        - false  Stop the SD host operation clock.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_init(void)
 * {
 *    sd_sdh_clk_cmd(true);
 * }
 *
 * \endcode
 *
 */
void sd_sdh_clk_cmd(bool NewState);

/**
 *
 * \brief   Check if the SD card is in programming state.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return The SD card check program status, 0 is SD_OK. Please refer to \ref T_SD_STATUS.
 * \retval 0         The SD card was checked successfully.
 * \retval 0x1-0xFF  The SD card was failed to check.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void sd_test(void)
 * {
 *     uint32_t sd_status = 0;
 *     sd_status = sd_check_program_status(SDHC_ID0);
 * }
 * \endcode
 *
 */
T_SD_STATUS sd_check_program_status(void);

/**
 *
 * \brief   Get the device block size.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return The block size of device.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void test(void)
 * {
 *     uint32_t block_size = 0;
 *     block_size = sd_get_dev_block_size(SDHC_ID0);
 * }
 * \endcode
 *
 */
uint32_t sd_get_dev_block_size(void);

/**
 *
 * \brief   Get the device capacity in bytes.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return The capacity of device.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void test(void)
 * {
 *     uint64_t capacity = 0;
 *     capacity = sd_get_dev_capacity(SDHC_ID0);
 * }
 * \endcode
 *
 */
uint64_t sd_get_dev_capacity(void);

/**
 *
 * \brief   De-initialize the SD host and SD card. Use sd_init can be restored.
 * \xrefitem Experimental_Added_API_2_13_0_0 " Experimental Added Since 2.13.0.0" "Added API"
 *
 * <b>Example usage</b>
 * \code{.c}
 * void test(void)
 * {
 *     sd_deinit(SDHC_ID0);
 * }
 * \endcode
 *
 */
void sd_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /*__SD_H*/

/** @} */ /* End of group 87x3e_SD_Exported_Functions */
/** @} */ /* End of group 87x3e_SD */

/******************* (C) COPYRIGHT 2025 Realtek Semiconductor Corporation *****END OF FILE****/

