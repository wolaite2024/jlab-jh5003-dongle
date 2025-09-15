/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file
* @brief
* @details
* @author    qinyuan_hu
* @date      2024-10-11
* @version   v1.2
* *********************************************************************************************************
*/

#ifndef __GDMA_CHANNEL_MANAGER_H
#define __GDMA_CHANNEL_MANAGER_H



#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"
#include "errno.h"
#include <stdbool.h>
/** @addtogroup 87x3e_PLATFORM_GDMA DMA Channel
  * @brief DMA channel driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup 87x3e_PLATFORM_GDMA_Exported_Types DMA Channel Exported Types
  * @{
  */
typedef struct
{
    uint32_t lr;
} GDMA_ChInfoTypeDef;

/** End of Group 87x3e_PLATFORM_GDMA_Exported_Types
  * @}
  */

/** @defgroup 87x3e_PLATFORM_GDMA_Exported_Constants DMA Channel Exported Constants
  * @{
  */

#define DMA_CHANNEL_DSP_RESERVED (BIT2|BIT3|BIT4|BIT5)      /* Indicates the default value. APP do not modify it. */
#define DMA_CHANNEL_LOG_RESERVED (BIT7)                     /* Indicates the default value. APP do not modify it. */

/** End of Group 87x3e_PLATFORM_GDMA_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup 87x3e_PLATFORM_GDMA_Exported_Functions DMA Channel Exported Functions
  * @{
  */
/**
  * @brief  Request an unused DMA channel.
  * @param  ch: Requested channel number. If there's no unused channel, ch is set to 0xa5.
  * @param  isr: Requested channel's isr. If isr is NULL, ignore ram vector table update.
  * @param  is_hp_ch: 1: request high performance channel, 0: normal DMA channel.
  * @retval   true  Status successful.
  * @retval   false Status fail.
  */
extern bool (*GDMA_channel_request)(uint8_t *ch, void *isr, bool is_hp_ch);

/**
  * @brief  Release an used DMA channel.
  * @param  ch: Released channel number.
  * @return None.
  */
void GDMA_channel_release(uint8_t ch);

/**
  * @brief  Get assigned DMA channel mask.
  * @param  None.
  * @return Mask of assigned DMA channel.
  */
uint16_t GDMA_channel_get_active_mask(void);
/**
  * @brief  Config mandatory DSP allocation DMA channel.
  * @param  dsp_dma_channel_mask: Mask of assigned DSP DMA channel.
  * @note   Rom default set DSP DMA channel 2\3\4\5\8, use it will
              reconfig DSP fixed channel, unused DMA channel will release.
  * @return None.
  */
void dma_dsp_channel_set(uint32_t dsp_dma_channel_mask);

/**
  * @brief  Config mandatory MCU allocation DMA channel.
  * @param  fixed_channel_mask: Mask of assigned DMA channel.
  * @note   Could called repeatedly.
  * @return Refer to errno.h.
  */
int32_t dma_fixed_channel_set(uint32_t fixed_channel_mask);

/**
  * @brief  Print the allocated DMA channel information.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
  */
void dma_print_allocated_channel(void);

#ifdef __cplusplus
}
#endif

#endif /*__GDMA_CHANNEL_MANAGER_H*/

/** @} */ /* End of group 87x3e_PLATFORM_GDMA_Exported_Functions */
/** @} */ /* End of group 87x3e_PLATFORM_GDMA */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

