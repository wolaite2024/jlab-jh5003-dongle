/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      io_debug.h
* @brief
* @details
* @author
* @date      2024-7-18
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _IO_DEBUG_
#define _IO_DEBUG_

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl876x.h"

/** @addtogroup IO_DEBUG IO DEBUG
  * @brief IO debug function module.
  * @{
  */

/** @defgroup IO_DEBUG_Exported_Types IO Debug Exported Types
  * @{
  */

/**
  * @brief  GPIO configure structure definition.
  */
typedef struct
{
    uint8_t mode: 1;                /* GPIO operating mode, 0:input mode, 1:output mode. */
    uint8_t in_value: 1;            /* Input level, 0:low level, 1:high level. */
    uint8_t out_value: 1;           /* Output level, 0:low level, 1:high level. */
    uint8_t int_en: 1;              /* GPIO interrupt enable or disable, 0: disable, 1:enable. */
    uint8_t int_en_mask: 1;         /* GPIO interrupt mask, 0:unmask, 1:mask. */
    uint8_t int_polarty: 1;         /* GPIO interrupt polarity, 0:low active, 1:high active. */
    uint8_t int_type: 1;            /* GPIO interrupt type, 0:level trigger, 1:edge trigger. */
    uint8_t int_type_edg_both: 1;   /* Both edge interrupt, 0:disable, 1:enable. */
    uint8_t int_status: 1;          /* Interrupt status, 0:reset, 1:set. */
    uint8_t debounce: 1;            /* GPIO hardware debounce function, 0:disable, 1:enable. */

} T_GPIO_SETTING;

/** End of group IO_DEBUG_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup IO_DEBUG_Exported_Functions IO Debug Exported Functions
  * @{
  */

/**
  * @brief  Set all the PAD to shut down mode, for power saving bubug only.
  * @param  None.
  * @return  None.
  */
void pad_set_all_shut_down(void);

/**
  * @brief  Dump the PAD setting of the specific pin, for IO PAD bubug only.
  * @param  pin_num: The pin function to dump.
  * @return  None.
  */
int32_t pad_print_setting(uint8_t pin_num);

/**
  * @brief  Dump all the PAD setting.
  * @param  None.
  * @return  None.
  */
void pad_print_all_pin_setting(void);

/**
  * @brief  Get the key name by the key_mask.
  * @param  key_mask: The specific key mask.
  * @return  None.
  */
const char *key_get_name(uint8_t key_mask);

/**
  * @brief  Convert the key status string.
  * @param  active: The polarity of key active.
  * @param  key_status: The current key status.
  * @return  None.
  */
const char *key_get_stat_str(uint32_t active, uint8_t key_status);

/**
  * @brief  Dump the GPIO setting of the specific pin, for IO PAD bubug only.
  * @param  pin_num: The pin function to dump.
  * @return  None.
  */
void gpio_print_pin_setting(uint8_t pin_num);

/**
 * io_debug.h
 *
 * \brief   Get the GPIO config.
 *
 * \xrefitem Added_API_2_12_0_0 "Added Since 2.12.0.0" "Added API"
 *
 * \param[in]   pin_num           Pin number.
 * \param[in]   gpio_setting      GPIO configuration parameters.
 * @return      Operation result.
 * @retval      0  Operation success.
 * @retval      -1 Operation failure.
 */
int32_t gpio_get_pin_setting(uint8_t pin_num, T_GPIO_SETTING *gpio_setting);

/** @} */ /* End of group IO_DEBUG_Exported_Functions */

#ifdef __cplusplus
}
#endif

#endif /* _IO_DEBUG_ */

/** @} */ /* End of group IO_DEBUG */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/

