/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      ext_buck.h
* @brief
* @details
* @author
* @date      2024-10-11
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef EXT_BUCK_H
#define EXT_BUCK_H

#include <stdbool.h>
#include <stdint.h>

/** @addtogroup EXT_BUCK External Buck
  * @brief Exteranl buck function module.
  * @{
  */

/*============================================================================*
*                         Types
*============================================================================*/

/** @defgroup EXT_BUCK_Exported_Types External Buck Exported Types
  * @{
  */

/**
  * @brief  External buck configure structure definition.
  */
typedef struct _EXT_BUCK_T
{
    bool (*ext_buck_hw_init)(void);                     /*!< Specifies ext_buck hardware init API.
                                                             Realize the ext_buck hardware init function. */
    bool (*ext_buck_set_voltage)(uint32_t volt_h_uv);   /*!< Specifies ext_buck set voltage API.
                                                             Realize the ext_buck changing voltage function. */
    bool (*ext_buck_enable)(void);                      /*!< Specifies ext_buck enable API.
                                                             Realize the ext_buck enable function. */
    bool (*ext_buck_disable)(void);                     /*!< Specifies ext_buck disable API.
                                                             Realize the ext_buck disable function. */
} EXT_BUCK_T;

/** End of group EXT_BUCK_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup EXT_BUCK_Exported_Functions External Buck Exported Functions
  * @{
  */

/**
  * @brief  Register the buck driver functions.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
  * @param  ext_buck: Register the ext_buck to set voltage/ enable / disable function.
  * @return None.
  *
  * <b>Example usage</b>
  * \code{.c}
  *
  * void app_buck_init(void)
  * {
  *     EXT_BUCK_T buck_ops;
  *     buck_ops.ext_buck_set_voltage = app_buck_set_vo1;
  *     buck_ops.ext_buck_enable = app_buck_enable;
  *     buck_ops.ext_buck_disable = app_buck_disable;
  *     ext_buck_init(&buck_ops);
  *
  *     app_buck_hw_init();
  * }
  * \endcode
  */
void ext_buck_init(EXT_BUCK_T *ext_buck);

/**
  * @brief  Enable the vore2 and external buck, this function is reserved for DSP2 driver.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
  * @retval true: Enable the vcore2 success.
  * @retval false: Enable the vcore2 failed.
  */
bool ext_buck_vcore2_enable(void);

/**
  * @brief  Disable the vcore2 and external buck, this function is reserved for DSP2 driver.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
  * @retval true: Disable the vcore2 success.
  * @retval false: Disable the vcore2 failed.
  */
bool ext_buck_vcore2_disable(void);

/** @} */ /* End of group EXT_BUCK_Exported_Functions */
/** @} */ /* End of group EXT_BUCK */

#endif /* EXT_BUCK_H */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/
