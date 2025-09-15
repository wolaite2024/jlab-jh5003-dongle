/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     task_log_api.h
* @brief    This file provides log task API declaration.
* @details
* @author
* @date
* @version  v1.0
*********************************************************************************************************
*/

#ifndef _TASK_LOG_API_H_
#define _TASK_LOG_API_H_
#include <stdint.h>
#include "mem_types.h"
/** @defgroup  HAL_87x3E_TASK_LOG    Save Log to Flash
    * @brief Save log to flash.
    * @{
    */
/**
 * @addtogroup HAL_87x3E_TASK_LOG Save Log to Flash
 * @{
 * @section LOG2FLASH_API_Usage_Chapter How to use this module?
 *
 * Prepare log_task_init parameter value:
 * - Assign the log partition space in the flash layout aligned by 4Kbytes as log_part_addr and log_part_size. \n
 * - Select the RAM type to alloc the ring buffer for log2flash as rb_ram_type. \n
 * - Check the avaliable RAM size for ring buffer on the selected RAM type as log_rb_size. \n
 * - Check whether there is DMA channel available as use_dma. If use DMA for flash write operation, an available DMA channel will be requested and freed dynamically.
 *
 * Before calling log_task_init API:
 * - If the log partition space is under flash block pretection region, please call fmc_flash_nor_set_bp_lv to
 *   unlock flash BP level.
 * - If the log partition address is more than 16Mbytes, please call fmc_flash_set_4_byte_address_mode to enable
 *   4-byte address mode for flash driver.
 *
 * Sample code:
 * @code
 *    //use inactive bank space as log partition space
 *
 *   uint32_t log_temp_addr = 0x02056000;
 *   uint32_t log_temp_length = 0x001AA000;
 *
 *   //alloc ring buffer 4000 bytes on data RAM, and use flash write DMA
 *   int init_ret = log_task_init(log_temp_addr, log_temp_length, 4000, OS_MEM_TYPE_DATA, true);
 *   if(init_ret != 0)
 *   {
 *      APP_PRINT_ERROR1("log_task_init return %d",init_ret);
 *   }
 *
 *   // unlock flash BP level before calling log_task_init if the log partition space
 *   // is under flash block protection region
 *   fmc_flash_nor_set_bp_lv(log_temp_addr, 0);
 *
 *   // enable 4-byte address mode for flash before calling log_task_init if the log
 *   // partition address is larger than 16Mbytes.
 *   fmc_flash_set_4_byte_address_mode(FMC_FLASH_NOR_ID0, true);
 *
 *   // If need to disable log2flash function, call enable_log_to_flash with parameter "false"
 *   enable_log_to_flash(false);
 * @endcode
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup HAL_87x3E_TASK_LOG_EXPORTED_MACROS Save Log to Flash Exported Macros
    * @brief
    * @{
    */

#define LOG_PARTITION_NAME "LOGGING_PART"
#define LOG_TASK_PRIORITY       (1)
#define LOG_TASK_STACK_MAX_SIZE (1024)

/*Adjust the log buffer size according to the available heap size.*/
#define LOG_BUFFER_SIZE        (1024*30)
/** End of HAL_87x3E_TASK_LOG_EXPORTED_MACROS
    * @}
    */
/** @defgroup HAL_87x3E_TASK_LOG_EXPORTED_FUNCTIONS Save Log to Flash Exported Functions
    * @brief
    * @{
    */
/**
    * @brief  Setup log2flash task.
    * \xrefitem Added_API_2_12_0_0 "Added Since 2.12.0.0" "Added API"
    * @note   Specify the log partition space on the flash through log_part_addr
    *         and log_part_size, specify the log2flash cache ring buffer size
    *         through the log_rb_size, LOG_BUFFER_SIZE is the reference value.
    *         Please keep the log partition space aligned by 4Kbytes. If the log
    *         partition space is under the flash block protection region, please
    *         unlock the flash BP level before using it as log partition. If the
    *         log partition region is assigned at the address more than 16 Mbytes,
    *         please consider to use 4-byte address mode for the flash driver,
    *         refer to the API fmc_flash_set_4_byte_address_mode.
    * @param  log_part_addr Log partition address on the flash.
    * @param  log_part_size Log partition size on the flash.
    * @param  log_rb_size   Log2flash cache ring buffer size on the heap.
    * @param  rb_ram_type   ref RAM_TYPE definition, RAM_TYPE_DSPSHARE, RAM_TYPE_ITCM1
    *                       or other invalid RAM type.
    * @param  use_dma Set true to use flash DMA write mode.
    *                 Set false to use flash user write mode.
    * @return The result of log2flash task init and create.
    * @retval 0      Success.
    * @retval !=0   Error code.
    */
int log_task_init(size_t log_part_addr, size_t log_part_size, size_t log_rb_size,
                  RAM_TYPE rb_ram_type,  bool use_dma);
/**
    * @brief  Enable or disable log to flash.
    * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
    * @note  When user call log_task_int API to init the log to flash function, the log
    *        to flash status is enabled by default. If user wants to disable the log to
    *        flash operation, use enable_log_to_flash(false) to stop writting log data to
    *        flash, but the log task will still run without writting flash.
    * @warning This API is supported in RTL87x3E and RTL87x3D.
    *          It is NOT supported in RTL87x3G.
    * @param  enable_status Specify the log to flash function enable status.
    *                       Set true to enable log to flash operation.
    *                       Set false to disable log to flash operation.
    */
void enable_log_to_flash(bool enable_status);

/**
    * @brief  Format log flash circular block to the init state.
    * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
    * @note  If user needs to format the log flash partition when the log2flash task is running, log_flash_format can be used.
    *        When log2flash task is running ok, the flash BP level should be unlocked by fmc_flash_nor_set_bp_lv already.
    *        So it doesn't need to unlock the flash BP level before calling log_flash_format.
    * @warning This API can't be used before log_task_init.
    *          This API is supported in RTL87x3E and RTL87x3D.
    *          It is NOT supported in RTL87x3G.
    * @return Error code 0: success, others: error code.
    */
int log_flash_format(void);

/** @} */ /* End of group HAL_87x3E_TASK_LOG_EXPORTED_FUNCTIONS */
#ifdef __cplusplus
}
#endif

/** @} */ /* End of group HAL_87x3E_TASK_LOG */
#endif
