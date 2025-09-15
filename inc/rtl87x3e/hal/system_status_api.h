/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    system_status_api.h
  * @brief   This file provides API wrapper for bbpro compatibility..
  * @author  sandy_jiang
  * @date    2018-11-29
  * @version v1.0
  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __SYSTEM_STATUS_API_H_
#define __SYSTEM_STATUS_API_H_


/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup HAL_87x3E_SYSTEM_STATUS_API System Status API
  * @{
  */

/*============================================================================*
 *                              Variables
*============================================================================*/
/** @defgroup HAL_87x3E_SYSTEM_STATUS_API_EXPORTED_VARIABLES HAL System Status Exported Variables
  * @{
  */


/** @} */ /* End of group HAL_87x3E_SYSTEM_STATUS_API_EXPORTED_VARIABLES */

/*============================================================================*
 *                              Functions
*============================================================================*/

/** @defgroup HAL_87x3E_SYSTEM_STATUS_API_EXPORTED_FUNCTIONS HAL System Status API
  * @{
  */
/**
    * @brief  Get the reset status to tell apart whether the mcu reboot from software reset or hardware reset.
    * @param  void
    * @return Whether reboot from software reset.
    * @retval  true   Reboot from software reset.
    * @retval  false  Reboot from hardware reset.
    */
bool sys_hall_get_reset_status(void);
/**
    * @brief  Print wake up reason after mcu power down.
    * @param  void
    * @return void
    */
void sys_hall_get_power_down_info(void);
/**
    * @brief  Get adpater level
    * @param  void
    * @return The result to get adpater level.
    * @retval true  Fail to get adpater level.
    * @retval false Success to get adpater level.
    */
bool sys_hall_adp_read_adp_level(void);

/**
    * @brief  Share 80k ram from dsp to mcu.
    * @note   Temporarily unavailable.
    * @param is_off_ram  Whether the memory shared from dsp is off ram or not.
    * @return void
    */
void sys_hall_set_dsp_share_memory_80k(bool is_off_ram);
/**
    * @brief  Read register value of aon register safely.
    * @param  input_info Offerset of aon register.
    * @param  output_info The read value to aon register.
    * @return void
    */
void sys_hall_btaon_fast_read_safe(uint16_t *input_info, uint16_t *output_info);


/**
    * @brief  Store register value of aon register safely.
    * @param  offset Offerset of aon register.
    * @param  input_info The value store to aon register.
    * @return void
    */
void sys_hall_btaon_fast_write_safe(uint16_t offset, uint16_t *input_info);

/**
    * @brief  Get package id of IC.
    * @note   Temporarily unavailable.
    * @param  enable Eable or disable charger.
    * @return void
    */
void sys_hall_charger_auto_enable(bool enable);
/**
    * @brief  Get package id of IC.
    * @param  void
    * @return Chip id of IC.
    */
uint8_t sys_hall_read_package_id(void);
/**
    * @brief  Get chip id of IC.
    * @param  void
    * @return Chip id of IC
    */
uint8_t sys_hall_read_chip_id(void);
/**
    * @brief  Get rom version of IC.
    * @param  void
    * @return Rom version of IC.
    */
uint8_t sys_hall_read_rom_version(void);
/**
    * @brief  Get 14 bytes euid of IC.
    * @param  void
    * @return Euid of IC.
    */
uint8_t *sys_hall_get_ic_euid(void);

/**
    * @brief  Init upperstack
    * @note   Temporarily unavailable.
    * @param  upperstack_compile_stamp  The time compile upperstack.
    * @return void
    */
void sys_hall_upperstack_ini(uint8_t *upperstack_compile_stamp);

/**
    * @brief  Enable or disable auto sleep in idle task
    * @note   Temporarily unavailable.
    * @param  flag  Specify the flag as true or false to enable or disable auto sleep in idle.
    * @return void
    */
void sys_hall_auto_sleep_in_idle(bool flag);

/**
    * @brief  Read efuse data on ram.
    * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
    * @note   Prepare enough data space to read the efuse on ram, and the reading space should be valid in the efuse space.
    * @param  offset  Specify the efuse offset to read.
    * @param  length  Specify the length to read.
    * @param  data    Specify the data buffer to store the efuse data.
    * @return The result to read efuse.
    * @retval  true  Read efuse successfully, refer the efuse data by the data parameter.
    * @retval  false Check the parameter fail before reading efuse data.
    */
bool  read_efuse_on_ram(uint16_t offset, uint16_t length, uint8_t *data);

/**
    * @brief  get IC secure state
    * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
    * @param  void
    * @return Whether secure is enabled.
    * @retval true  Secure is enabled.
    * @retval false Secure is disabled.
    */
bool sys_hall_get_secure_state(void);
/** @} */ /* End of group HAL_87x3E_SYSTEM_STATUS_API_EXPORTED_FUNCTIONS */
/** End of HAL_87x3E_SYSTEM_STATUS_API
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif
