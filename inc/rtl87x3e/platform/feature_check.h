/**
*****************************************************************************************
*     Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    feature_check.h
  * @brief   This file provides API to check different IC feature.
  * @author  colin
  * @date    2024-03-08
  * @version v1.0
  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2024 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __FEATURE_CHECK_H_
#define __FEATURE_CHECK_H_


/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup HAL_87x3e_FEATURE_CHECK Feature Check API
  * @{
  */

/*============================================================================*
 *                               Types
*============================================================================*/
/** @defgroup HAL_87x3e_FEATURE_CHECK_Exported_Types Feature Check Exported Types
  * @{
  */

typedef enum
{
    /* RTL8753EFE,RTL8753EFE_VS,RTL8763EFL,RTL8763EFL_VM_CG,RTL8763EF,RTL8753EFH */
    RTL87X3E_PKG_TYPE_QFN_40         = 1,
    /* RTL8773EFE,RTL8773EFL */
    RTL87X3E_PKG_TYPE_QFN_46         = 2,
    RTL87X3E_PKG_TYPE_QFN_40_3MIC    = 3,
    /* RTL8763EW,RTL8763EW_VC,RTL8763EW_VP */
    RTL87X3E_PKG_TYPE_BGA_149        = 4,
    /* RTL8763EAU,RTL8763ESE,RTL8773ESL */
    RTL87X3E_PKG_TYPE_QFN_50         = 5,
    /*RTL8763EWE,RTL8763EWE_VP */
    RTL87X3E_PKG_TYPE_QFN_68         = 6,
    /* RTL8763EWM */
    RTL87X3E_PKG_TYPE_aQFN_76        = 7,
    /* RTL87X3D and other chip*/
    OTHER_PKG_TYPE                   = 8,
} PackageType_Typedef;

/** @} */ /* End of group HAL_87x3e_FEATURE_CHECK_Exported_Types */

/*============================================================================*
 *                              Variables
*============================================================================*/
/** @defgroup HAL_87x3e_FEATURE_CHECK_Exported_Variables HAL Feature Check Exported Variables
  * @{
  */
#define RTK_BT_CHIP_ID_RTL87X3D      (26)
#define RTK_BT_CHIP_ID_RTL87X3E      (27)

/** @} */ /* End of group HAL_87x3e_FEATURE_CHECK_Exported_Variables */

/*============================================================================*
 *                              Functions
*============================================================================*/

/** @defgroup HAL_87x3e_FEATURE_CHECK_Exported_Functions HAL Feature Check Exported Functions
  * @{
  */

/**
 * \brief   Get chip id.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 * \param   None.
 * \return  Chip id.
 */
uint8_t feature_check_get_chip_id(void);

/**
 * \brief   Get package id.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 * \param   None.
 * \return  Package id.
 */
uint8_t feature_check_get_pkg_id(void);

/**
 * \brief   Get package type.
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 * \param   None.
 * \return  Package type.
 */
PackageType_Typedef feature_check_get_pkg_type(void);

/** @} */ /* End of group HAL_87x3e_FEATURE_CHECK_Exported_Functions */
/** End of HAL_87x3e_FEATURE_CHECK
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif
