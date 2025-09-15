/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      power_debug.h
* @brief
* @details
* @author
* @date      2024-10-11
* @version   v1.1
* *********************************************************************************************************
*/


#ifndef _POWER_DEBUG_
#define _POWER_DEBUG_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup POWER_DEBUG Power Debug
  * @brief Power debug function module.
  * @{
  */

/** @defgroup POWER_DEBUG_Exported_Types Power Debug Exported Types
  * @{
  */

typedef enum
{
    POWER_DEBUG_STAGE_BOOT,
    POWER_DEBUG_STAGE_A2DP_START,
    POWER_DEBUG_STAGE_A2DP_STOP,
    POWER_DEBUG_STAGE_HFP_START,
    POWER_DEBUG_STAGE_HFP_STOP,
} T_POWER_DEBUG_STAGE;

/** End of Group POWER_DEBUG_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup POWER_DEBUG_Exported_Functions Power Debug Exported Functions
  * @{
  */

/**
 * @brief  Dump register for power test.
 * \xrefitem Added_API_2_12_0_0 "Added Since 2.12.0.0" "Added API"
 * @param[in]  stage: The stage (boot/a2dp start/a2dp stop/HFP start/HFP stop) that requires dump power related registers.
 * @return None.
*/
void power_test_dump_register(T_POWER_DEBUG_STAGE stage);

/** @} */ /* End of group POWER_DEBUG_Exported_Functions */
/** @} */ /* End of group POWER_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* _POWER_DEBUG_ */

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/

