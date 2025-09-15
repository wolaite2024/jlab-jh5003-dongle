/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_nvic.h
* @brief
* @details
* @author    elliot chen
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef __RTL876X_NVIC_H
#define __RTL876X_NVIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"


/** @addtogroup 87x3e_NVIC NVIC
  * @brief NVIC driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup 87x3e_NVIC_Exported_Types NVIC Exported Types
  * @{
  */

/**
  * @brief  NVIC init structure definition.
  */

typedef struct
{
    IRQn_Type NVIC_IRQChannel;                  /*!< Specifies the IRQ channel. This parameter can be a value of @ref IRQn_Type. */

    uint32_t NVIC_IRQChannelPriority;           /*!< Specifies the priority for the IRQ channel. This parameter can be a value
                                                                    between 0 and x as described in the table. */

    FunctionalState
    NVIC_IRQChannelCmd;         /*!< Specifies whether the IRQ channel defined in NVIC_IRQChannel
                                                                     will be enabled or disabled. */
} NVIC_InitTypeDef;

/**
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_NVIC_Exported_Functions NVIC Exported Functions
  * @{
  */

/**
 *
 * \brief     Initializes the NVIC peripheral according to the specified
 *            parameters in the NVIC_InitStruct.
 *
 * \param[in] NVIC_InitStruct: Pointer to a \ref NVIC_InitTypeDef structure that contains
 *            the configuration information for the specified NVIC peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void i2c0_init(void)
 * {
 *     //add code here.
 *     RamVectorTableUpdate(I2C0_VECTORn, (IRQ_Fun)I2C0_Handler);
 *
 *     NVIC_InitTypeDef NVIC_InitStruct;
 *     NVIC_InitStruct.NVIC_IRQChannel = I2C0_IRQn;
 *     NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
 *     NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
 *     NVIC_Init(&NVIC_InitStruct);
 * }
 * \endcode
 */
extern void (*NVIC_Init)(NVIC_InitTypeDef *NVIC_InitStruct);

#ifdef __cplusplus
}
#endif

#endif /*__RTL876X_NVIC_H*/

/** @} */ /* End of group 87x3e_NVIC_Exported_Functions */
/** @} */ /* End of group 87x3e_NVIC */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

