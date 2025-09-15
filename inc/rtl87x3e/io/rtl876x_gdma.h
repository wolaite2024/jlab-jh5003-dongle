/**
*********************************************************************************************************
*               Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_gdma.h
* @brief
* @details
* @author    colin
* @date      2025-03-17
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef __RTL876X_GDMA_H
#define __RTL876X_GDMA_H



#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"

/** @addtogroup 87x3e_GDMA GDMA
  * @brief GDMA driver module.
  * @{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/


/** @defgroup 87x3e_GDMA_Exported_Types GDMA Exported Types
  * @{
  */

/**
  * @brief  GDMA Init structure definition.
  */
typedef struct _X3E_GDMA_InitTypeDef
{
    uint8_t GDMA_ChannelNum;         /*!< Specifies the channel number for GDMA.
                                                    This parameter can be set from 0 to 8. */

    uint8_t GDMA_DIR;                /*!< Specifies if the peripheral is the source or destination.
                                                    This parameter can be a value of @ref x3e_GDMA_data_transfer_direction. */

    uint32_t GDMA_BufferSize;        /*!< Specifies the buffer size. The data unit is equal to the configuration set in GDMA_SourceDataSize.
                                                    This parameter can be set from 1 to 65535. */

    uint8_t GDMA_SourceInc;          /*!< Specifies whether the source address register is incremented or not.
                                                    This parameter can be a value of @ref x3e_GDMA_source_incremented_mode. */

    uint8_t GDMA_DestinationInc;     /*!< Specifies whether the destination address register is incremented or not.
                                                    This parameter can be a value of @ref x3e_GDMA_destination_incremented_mode. */

    uint32_t GDMA_SourceDataSize;    /*!< Specifies the source data width.
                                                    This parameter can be a value of @ref x3e_GDMA_data_size. */

    uint32_t GDMA_DestinationDataSize;/*!< Specifies the destination data width.
                                                    This parameter can be a value of @ref x3e_GDMA_data_size. */

    uint32_t GDMA_SourceMsize;      /*!< Specifies the burst transaction length.
                                                    This parameter can be a value of @ref x3e_GDMA_Msize. */

    uint32_t GDMA_DestinationMsize; /*!< Specifies  the burst transaction length.
                                                    This parameter can be a value of @ref x3e_GDMA_Msize. */

    uint32_t GDMA_SourceAddr;       /*!< Specifies the source address for GDMA channelx. */

    uint32_t GDMA_DestinationAddr;  /*!< Specifies the destination address for GDMA channelx. */

    uint32_t GDMA_ChannelPriority;   /*!< Specifies the software priority for the GDMA channelx.
                                                    This parameter can be set from 0 to 8, and 0 is the highest priority value. */

    uint32_t GDMA_Multi_Block_Struct; /*!< Specifies the pointer to the first struct of LLI. */

    uint8_t  GDMA_Multi_Block_En;     /*!< Specifies whether to enable the multi-block function.
                                                    This parameter can be a value of ENABLE or DISABLE. */

    uint8_t  GDMA_Scatter_En;         /*!< Specifies whether to enable the scatter function.
                                                    This parameter can be a value of ENABLE or DISABLE. */

    uint8_t  GDMA_Gather_En;          /*!< Specifies whether to enable the gather function.
                                                    This parameter can be a value of ENABLE or DISABLE. */

    uint32_t GDMA_GatherCount;        /*!< Specifies the source gather count.
                                                    This parameter can be set from 0x0 to 0xffff. */

    uint32_t GDMA_GatherInterval;     /*!< Specifies the source gather interval.
                                                    This parameter can be set from 0x0 to 0xfffff. */

    uint32_t GDMA_Source_Cir_Gather_Num; /*!< Specifies the source circular gather number.
                                                    This parameter can be set from 0x1 to 0xf. */

    uint32_t GDMA_ScatterCount;          /*!< Specifies the destination scatter count.
                                                    This parameter can be set from 0x0 to 0xffff. */

    uint32_t GDMA_ScatterInterval;      /*!< Specifies the destination scatter interval.
                                                    This parameter can be set from 0x0 to 0xfffff. */

    uint32_t GDMA_Dest_Cir_Sca_Num;  /*!< Specifies the destination circular scatter number.
                                                    This parameter can be set from 0x1 to 0xf. */

    uint32_t GDMA_Multi_Block_Mode;      /*!< Specifies the multi block transfer mode.
                                                    This parameter can be a value of @ref x3e_GDMA_Multiblock_Mode. */

    uint8_t  GDMA_SourceHandshake;       /*!< Specifies the handshake index in source.
                                                    This parameter can be a value of @ref x3e_GDMA_Handshake_Type. */

    uint8_t  GDMA_DestHandshake;          /*!< Specifies the handshake index in destination.
                                                    This parameter can be a value of @ref x3e_GDMA_Handshake_Type. */

} GDMA_InitTypeDef;

/**
  * @brief  GDMA Link List Item structure definition.
  */
typedef struct _X3E_GDMA_LLIDef
{
    __IO uint32_t SAR;          /*!< Specifies the source address for LLI. */
    __IO uint32_t DAR;          /*!< Specifies the destination address for LLI. */
    __IO uint32_t LLP;          /*!< Specifies the next pointer to the struct of LLI. */
    __IO uint32_t CTL_LOW;      /*!< Specifies the configure for LLI. */
    __IO uint32_t CTL_HIGH;     /*!< Specifies the configure for LLI. */
} GDMA_LLIDef;

/** End of Group 87x3e_GDMA_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/

/** @defgroup 87x3e_GDMA_Exported_Constants GDMA Exported Constants
  * @{
  */

/** @defgroup 87x3e_GDMA_ALL_PERIPH Check GDMA Channel
  * @{
  */
#define IS_GDMA_ALL_PERIPH(PERIPH) (((PERIPH) == GDMA_Channel0) || \
                                    ((PERIPH) == GDMA_Channel1) || \
                                    ((PERIPH) == GDMA_Channel2) || \
                                    ((PERIPH) == GDMA_Channel3) || \
                                    ((PERIPH) == GDMA_Channel4) || \
                                    ((PERIPH) == GDMA_Channel5) || \
                                    ((PERIPH) == GDMA_Channel6) || \
                                    ((PERIPH) == GDMA_Channel7) || \
                                    ((PERIPH) == GDMA_Channel8))    //!< Check whether is the GDMA channel.
#define IS_GDMA_ChannelNum(NUM) ((NUM) < 9) //!< The max channel num of GDMA is 8.
#define GDMA_TOTAL_CH_COUNT (9)     //!< The total GDMA channel count for MCU.
#define GDMA_MAX_HP_CH_COUNT (2)    //!< The GDMA high performance channel count.

/** End of Group 87x3e_GDMA_ALL_PERIPH
  * @}
  */

/** @defgroup 87x3e_GDMA_data_size GDMA Data Size
  * @{
  */

#define GDMA_DataSize_Byte            ((uint32_t)0x00000000)    //!< Configure the source or destination data size as byte.
#define GDMA_DataSize_HalfWord        ((uint32_t)0x00000001)    //!< Configure the source or destination data size as half word.
#define GDMA_DataSize_Word            ((uint32_t)0x00000002)    //!< Configure the source or destination data size as word.
#define IS_GDMA_DATA_SIZE(SIZE) (((SIZE) == GDMA_DataSize_Byte) || \
                                 ((SIZE) == GDMA_DataSize_HalfWord) || \
                                 ((SIZE) == GDMA_DataSize_Word))    //!< Check whether is the GDMA data size.

/** End of Group 87x3e_GDMA_data_size
  * @}
  */

/** @defgroup 87x3e_GDMA_Msize GDMA Msize
  * @{
  */

#define GDMA_Msize_1            ((uint32_t)0x00000000)          //!< Configure the burst transaction length as 1.
#define GDMA_Msize_4            ((uint32_t)0x00000001)          //!< Configure the burst transaction length as 4.
#define GDMA_Msize_8            ((uint32_t)0x00000002)          //!< Configure the burst transaction length as 8.
#define GDMA_Msize_16           ((uint32_t)0x00000003)          //!< Configure the burst transaction length as 16.
#define GDMA_Msize_32           ((uint32_t)0x00000004)          //!< Configure the burst transaction length as 32.
#define GDMA_Msize_64           ((uint32_t)0x00000005)          //!< Configure the burst transaction length as 64.
#define GDMA_Msize_128          ((uint32_t)0x00000006)          //!< Configure the burst transaction length as 128.
#define GDMA_Msize_256          ((uint32_t)0x00000007)          //!< Configure the burst transaction length as 256.
#define IS_GDMA_MSIZE(SIZE) (((SIZE) == GDMA_Msize_1) || \
                             ((SIZE) == GDMA_Msize_4) || \
                             ((SIZE) == GDMA_Msize_8) || \
                             ((SIZE) == GDMA_Msize_16) || \
                             ((SIZE) == GDMA_Msize_32) || \
                             ((SIZE) == GDMA_Msize_64) || \
                             ((SIZE) == GDMA_Msize_128) || \
                             ((SIZE) == GDMA_Msize_256))        //!< Check whether is the burst transaction length.

/** End of Group 87x3e_GDMA_Msize
  * @}
  */

/** @defgroup 87x3e_GDMA_Handshake_Type GDMA Handshake Type
  * @{
  */
#define GDMA_Handshake_UART0_TX          (0)        //!< The handshake for UART0 TX.
#define GDMA_Handshake_UART0_RX          (1)        //!< The handshake for UART0 RX.
#define GDMA_Handshake_UART2_TX          (2)        //!< The handshake for UART2 TX.
#define GDMA_Handshake_UART2_RX          (3)        //!< The handshake for UART2 RX.
#define GDMA_Handshake_SPI0_TX           (4)        //!< The handshake for SPI0 TX.
#define GDMA_Handshake_SPI0_RX           (5)        //!< The handshake for SPI0 RX.
#define GDMA_Handshake_SPI1_TX           (6)        //!< The handshake for SPI1 TX.
#define GDMA_Handshake_SPI1_RX           (7)        //!< The handshake for SPI1 RX.
#define GDMA_Handshake_I2C0_TX           (8)        //!< The handshake for I2C0 TX.
#define GDMA_Handshake_I2C0_RX           (9)        //!< The handshake for I2C0 RX.
#define GDMA_Handshake_I2C1_TX           (10)       //!< The handshake for I2C1 TX.
#define GDMA_Handshake_I2C1_RX           (11)       //!< The handshake for I2C1 RX.
#define GDMA_Handshake_ADC               (12)       //!< The handshake for ADC.
#define GDMA_Handshake_AES_TX            (13)       //!< The handshake for AES TX.
#define GDMA_Handshake_AES_RX            (14)       //!< The handshake for AES RX.
#define GDMA_Handshake_UART1_TX          (15)       //!< The handshake for UART1 TX.
#define GDMA_Handshake_SPORT0_TX         (16)       //!< The handshake for SPORT0 TX.
#define GDMA_Handshake_SPORT0_RX         (17)       //!< The handshake for SPORT0 RX.
#define GDMA_Handshake_SPORT1_TX         (18)       //!< The handshake for SPORT1 TX.
#define GDMA_Handshake_SPORT1_RX         (19)       //!< The handshake for SPORT1 RX.
#define GDMA_Handshake_UART1_RX          (20)       //!< The handshake for UART1 RX.
#define GDMA_Handshake_SPIC0_TX          (21)       //!< The handshake for SPIC0 TX.
#define GDMA_Handshake_SPIC0_RX          (22)       //!< The handshake for SPIC0 RX.
#define GDMA_Handshake_TIM0              (24)       //!< The handshake for TIM0.
#define GDMA_Handshake_TIM1              (25)       //!< The handshake for TIM1.
#define GDMA_Handshake_TIM2              (26)       //!< The handshake for TIM2.
#define GDMA_Handshake_TIM3              (27)       //!< The handshake for TIM3.
#define GDMA_Handshake_TIM4              (28)       //!< The handshake for TIM4.
#define GDMA_Handshake_TIM5              (29)       //!< The handshake for TIM5.
#define GDMA_Handshake_TIM6              (30)       //!< The handshake for TIM6.
#define GDMA_Handshake_TIM7              (31)       //!< The handshake for TIM7.
#define GDMA_Handshake_SPIC1_TX          (32)       //!< The handshake for SPIC1 TX.
#define GDMA_Handshake_SPIC1_RX          (33)       //!< The handshake for SPIC1 RX.
#define GDMA_Handshake_SPIC2_TX          (34)       //!< The handshake for SPIC2 TX.
#define GDMA_Handshake_SPIC2_RX          (35)       //!< The handshake for SPIC2 RX.
#define GDMA_Handshake_I2C2_TX           (36)       //!< The handshake for I2C2 TX.
#define GDMA_Handshake_I2C2_RX           (37)       //!< The handshake for I2C2 RX.
#define GDMA_Handshake_SPI2_TX           (38)       //!< The handshake for SPIC2 TX.
#define GDMA_Handshake_SPI2_RX           (39)       //!< The handshake for SPIC2 RX.
#define GDMA_Handshake_AUDIO_RX          (40)       //!< The handshake for AUDIO RX.
#define GDMA_Handshake_I8080             (41)       //!< The handshake for I8080.
#define GDMA_Handshake_SPORT2_RX         (42)       //!< The handshake for SPORT2 RX.
#define GDMA_Handshake_SPIC3_TX          (43)       //!< The handshake for SPIC3 TX.
#define GDMA_Handshake_SPIC3_RX          (44)       //!< The handshake for SPIC3 RX.
#define GDMA_Handshake_IR_TX             (62)       //!< The handshake for IR TX.
#define GDMA_Handshake_IR_RX             (63)       //!< The handshake for IR RX.

// for compatible with BBPRO
#define GDMA_Handshake_UART_TX           GDMA_Handshake_UART0_TX    //!< The handshake for UART0 TX.
#define GDMA_Handshake_UART_RX           GDMA_Handshake_UART0_RX    //!< The handshake for UART0 RX.
#define GDMA_Handshake_LOG_UART1_TX      GDMA_Handshake_UART2_TX    //!< The handshake for UART2 TX.
#define GDMA_Handshake_LOG_UART1_RX      GDMA_Handshake_UART2_RX    //!< The handshake for UART2 RX.
#define GDMA_Handshake_LOG_UART_TX       GDMA_Handshake_UART1_TX    //!< The handshake for UART1 TX.
#define GDMA_Handshake_LOG_UART_RX       GDMA_Handshake_UART1_RX    //!< The handshake for UART1 RX.

#define IS_GDMA_TransferType(Type) (((Type) == GDMA_Handshake_UART0_TX) || \
                                    ((Type) == GDMA_Handshake_UART0_RX) || \
                                    ((Type) == GDMA_Handshake_UART2_TX) || \
                                    ((Type) == GDMA_Handshake_UART2_RX) || \
                                    ((Type) == GDMA_Handshake_SPI0_TX) || \
                                    ((Type) == GDMA_Handshake_SPI0_RX) || \
                                    ((Type) == GDMA_Handshake_I2C0_TX) || \
                                    ((Type) == GDMA_Handshake_I2C0_RX) || \
                                    ((Type) == GDMA_Handshake_I2C1_TX) || \
                                    ((Type) == GDMA_Handshake_I2C1_RX) || \
                                    ((Type) == GDMA_Handshake_ADC) || \
                                    ((Type) == GDMA_Handshake_AES_TX) || \
                                    ((Type) == GDMA_Handshake_AES_RX) || \
                                    ((Type) == GDMA_Handshake_UART1_TX) || \
                                    ((Type) == GDMA_Handshake_SPORT0_TX) || \
                                    ((Type) == GDMA_Handshake_SPORT0_RX) || \
                                    ((Type) == GDMA_Handshake_SPORT1_TX) || \
                                    ((Type) == GDMA_Handshake_SPORT1_RX) || \
                                    ((Type) == GDMA_Handshake_SPDIF_TX) || \
                                    ((Type) == GDMA_Handshake_Other) ||\
                                    ((Type) == GDMA_Handshake_TIM0)||\
                                    ((Type) == GDMA_Handshake_TIM1)||\
                                    ((Type) == GDMA_Handshake_TIM2)||\
                                    ((Type) == GDMA_Handshake_TIM3)||\
                                    ((Type) == GDMA_Handshake_TIM4)||\
                                    ((Type) == GDMA_Handshake_TIM5)||\
                                    ((Type) == GDMA_Handshake_TIM6)||\
                                    ((Type) == GDMA_Handshake_TIM7))    //!< Check whether is the GDMA handshake.

/** End of Group 87x3e_GDMA_Handshake_Type
  * @}
  */

/** @defgroup 87x3e_GDMA_data_transfer_direction GDMA Data Transfer Direction
  * @{
  */

#define GDMA_DIR_MemoryToMemory              ((uint32_t)0x00000000)         //!< Configure the direction of GDMA as memory to memory.
#define GDMA_DIR_MemoryToPeripheral          ((uint32_t)0x00000001)         //!< Configure the direction of GDMA as memory to peripheral.
#define GDMA_DIR_PeripheralToMemory          ((uint32_t)0x00000002)         //!< Configure the direction of GDMA as peripheral to memory.
#define GDMA_DIR_PeripheralToPeripheral      ((uint32_t)0x00000003)         //!< Configure the direction of GDMA as peripheral to peripheral.

#define IS_GDMA_DIR(DIR) (((DIR) == GDMA_DIR_MemoryToMemory) || \
                          ((DIR) == GDMA_DIR_MemoryToPeripheral) || \
                          ((DIR) == GDMA_DIR_PeripheralToMemory) ||\
                          ((DIR) == GDMA_DIR_PeripheralToPeripheral))       //!< Check whether is the direction of GDMA.

/** End of Group 87x3e_GDMA_data_transfer_direction
  * @}
  */

/** @defgroup 87x3e_GDMA_source_incremented_mode GDMA Source Incremented Mode
  * @{
  */

#define DMA_SourceInc_Inc          ((uint32_t)0x00000000)                   //!< Configure the source address as incremented.
#define DMA_SourceInc_Fix          ((uint32_t)0x00000002)                   //!< Configure the source address as fixed.

#define IS_GDMA_SourceInc(STATE) (((STATE) == DMA_SourceInc_Inc) || \
                                  ((STATE) == DMA_SourceInc_Fix))           //!< Check whether is the source address mode.

/** End of Group 87x3e_GDMA_source_incremented_mode
  * @}
  */

/** @defgroup 87x3e_GDMA_destination_incremented_mode GDMA Destination Incremented Mode
  * @{
  */

#define DMA_DestinationInc_Inc          ((uint32_t)0x00000000)              //!< Configure the destination address as incremented.
#define DMA_DestinationInc_Fix          ((uint32_t)0x00000002)              //!< Configure the destination address as fixed.

#define IS_GDMA_DestinationInc(STATE) (((STATE) == DMA_DestinationInc_Inc) || \
                                       ((STATE) == DMA_DestinationInc_Fix)) //!< Check whether is the destination address mode.

/** End of Group 87x3e_GDMA_destination_incremented_mode
  * @}
  */

/** @defgroup 87x3e_DMA_interrupts_definition GDMA Interrupts Definition
  * @{
  */

#define GDMA_INT_Transfer                               ((uint32_t)0x00000001)  //!< The interrupt is generated on GDMA transfer completion to the destination.
#define GDMA_INT_Block                                  ((uint32_t)0x00000002)  //!< The interrupt is generated on GDMA block transfer completion to the destination.
#define GDMA_INT_SrcTransfer                            ((uint32_t)0x00000004)  //!< The interrupt is generated after completion of the last OCP transfer of the requested single/burst transaction from the handshaking interface.
#define GDMA_INT_DstTransfer                            ((uint32_t)0x00000008)  //!< The interrupt is generated on GDMA transfer completion to the destination peripheral.
#define GDMA_INT_Error                                  ((uint32_t)0x00000010)  //!< The interrupt is generated when an ERROR response is received from the slave on the SRESP bus during a GDMA transfer.
#define IS_GDMA_CONFIG_IT(IT) ((((IT) & 0xFFFFFE00) == 0x00) && ((IT) != 0x00)) //!< Check whether is the GDMA interrupt.

/** End of Group 87x3e_DMA_interrupts_definition
  * @}
  */

/** @defgroup 87x3e_DMA_Channel_Base GDMA Channel Base Address
  * @{
  */
#define DMA_CH_BASE(ChNum) ((GDMA_ChannelTypeDef *)((ChNum >= 8) ?(GDMA_Channel8_BASE + (ChNum - 8)*0x0058) : (GDMA_CHANNEL_REG_BASE + ChNum * 0x0058)))    //!< The GDMA channel address.
/** End of Group 87x3e_DMA_Channel_Base
  * @}
  */

/** @defgroup 87x3e_DMA_Channel_IRQ GDMA Channel IRQ Number
  * @{
  */
#define DMA_CH_IRQ(ChNum) ((IRQn_Type)((ChNum >= 6) ? (GDMA0_Channel6_IRQn + ChNum - 6) : (GDMA0_Channel0_IRQn + ChNum)))   //!< The GDMA channel irq.
/** End of Group 87x3e_DMA_Channel_IRQ
  * @}
  */

/** @defgroup 87x3e_DMA_Channel_Vector GDMA Channel Vector
  * @{
  */
#define DMA_CH_VECTOR(ChNum) ((VECTORn_Type)((ChNum >= 6) ? (GDMA0_Channel6_VECTORn + ChNum - 6) : (GDMA0_Channel0_VECTORn + ChNum)))   //!< The GDMA channel vector table number.
#define DMA_VECTOR_TO_CH_NUM(vector) (((vector) <= GDMA0_Channel5_VECTORn )? (vector) - GDMA0_Channel0_VECTORn:(vector) - GDMA0_Channel6_VECTORn + 6)   //!< Obtain the GDMA channel number through the vector table.
#define DMA_MAX_VECTORN               GDMA0_Channel8_VECTORn    //!< The max GDMA channel vector table number.
/** End of Group 87x3e_DMA_Channel_Vector
  * @}
  */

/** @defgroup 87x3e_DMA_Suspend_definition GDMA Suspend Definition
  * @{
  */

#define GDMA_SUSPEND_TRANSMISSSION                  (BIT(8))                //!< The bit for GDMA suspend transmisssion.
#define GDMA_FIFO_STATUS                            (BIT(9))                //!< The bit for GDMA FIFO status.
#define GDMA_SUSPEND_CHANNEL_STATUS                 (BIT(0))                //!< The bit for GDMA suspend channel status.
#define GDMA_SUSPEND_CMD_STATUS                     (BIT(2) | BIT(1))       //!< The bit for GDMA suspend command.

/** End of Group 87x3e_DMA_Suspend_definition
  * @}
  */

/** @defgroup 87x3e_GDMA_Multiblock_Mode GDMA Multi-block Mode
  * @{
  */

#define AUTO_RELOAD_WITH_CONTIGUOUS_SAR                            (BIT31)  //!< Configure the multi-block transfer mode with the source address set to continuous and the destination address set to auto-reload.
#define AUTO_RELOAD_WITH_CONTIGUOUS_DAR                            (BIT30)  //!< Configure the multi-block transfer mode with the source address set to auto-reload and the destination address set to continuous.
#define AUTO_RELOAD_TRANSFER                                       (BIT30 | BIT31)  //!< Configure the multi-block transfer mode with the source address set to auto-reload and the destination address set to auto-reload.
#define LLI_WITH_CONTIGUOUS_SAR                                    (BIT27)  //!< Configure the multi-block transfer mode with the source address set to continuous and the destination address set to LLI.
#define LLI_WITH_AUTO_RELOAD_SAR                                   (BIT27 | BIT30)  //!< Configure the multi-block transfer mode with the source address set to auto-reload and the destination address set to LLI.
#define LLI_WITH_CONTIGUOUS_DAR                                    (BIT28)  //!< Configure the multi-block transfer mode with the source address set to LLI and the destination address set to continuous.
#define LLI_WITH_AUTO_RELOAD_DAR                                   (BIT28 | BIT31)  //!< Configure the multi-block transfer mode with the source address set to LLI and the destination address set to auto-reload.
#define LLI_TRANSFER                                               (BIT27 | BIT28)  //!< Configure the multi-block transfer mode with the source address set to LLI and the destination address set to LLI.

#define IS_GDMA_MULTIBLOCKMODE(MODE) (((MODE) == AUTO_RELOAD_WITH_CONTIGUOUS_SAR) || ((MODE) == AUTO_RELOAD_WITH_CONTIGUOUS_DAR)\
                                      ||((MODE) == AUTO_RELOAD_TRANSFER) || ((MODE) == LLI_WITH_CONTIGUOUS_SAR)\
                                      ||((MODE) == LLI_WITH_AUTO_RELOAD_SAR) || ((MODE) == LLI_WITH_CONTIGUOUS_DAR)\
                                      ||((MODE) == LLI_WITH_AUTO_RELOAD_DAR) || ((MODE) == LLI_TRANSFER))   //!< Check whether is the GDMA multi-block transfer mode.

/** End of Group 87x3e_GDMA_Multiblock_Mode
  * @}
  */

/** @cond private
  * @defgroup 87x3e_GDMA_Multiblock_Select_Bit GDMA Multi-block Select Bit
  * @{
  */

#define AUTO_RELOAD_SELECTED_BIT                                   (uint32_t)(0xC0000000)   //!< The bit for GDMA auto-reload.
#define LLP_SELECTED_BIT                                           (uint32_t)(0x18000000)   //!< The bit for GDMA LLI.
/** End of Group 87x3e_GDMA_Multiblock_Select_Bit
  * @}
  * @endcond
  */

/** @defgroup 87x3e_GDMA_Channel_BIT GDMA Channel Bit
  * @{
  */
#define CHANNEL_BIT(ch)    ((ch < 8) ? BIT(ch) : BIT(ch + 8))     //!< The GDMA channel bit.
/** End of Group 87x3e_GDMA_Channel_BIT
  * @}
  */

/** @defgroup 87x3e_GDMA_Declaration GDMA Declaration
  * @{
  */
#define GDMA_Channel0                   ((GDMA_ChannelTypeDef      *) GDMA_Channel0_BASE) //!< The GDMA channel 0 base.
#define GDMA_Channel1                   ((GDMA_ChannelTypeDef      *) GDMA_Channel1_BASE) //!< The GDMA channel 1 base.
#define GDMA_Channel2                   ((GDMA_ChannelTypeDef      *) GDMA_Channel2_BASE) //!< The GDMA channel 2 base.
#define GDMA_Channel3                   ((GDMA_ChannelTypeDef      *) GDMA_Channel3_BASE) //!< The GDMA channel 3 base.
#define GDMA_Channel4                   ((GDMA_ChannelTypeDef      *) GDMA_Channel4_BASE) //!< The GDMA channel 4 base.
#define GDMA_Channel5                   ((GDMA_ChannelTypeDef      *) GDMA_Channel5_BASE) //!< The GDMA channel 5 base.
#define GDMA_Channel6                   ((GDMA_ChannelTypeDef      *) GDMA_Channel6_BASE) //!< The GDMA channel 6 base.
#define GDMA_Channel7                   ((GDMA_ChannelTypeDef      *) GDMA_Channel7_BASE) //!< The GDMA channel 7 base.
#define GDMA_Channel8                   ((GDMA_ChannelTypeDef      *) GDMA_Channel8_BASE) //!< The GDMA channel 8 base.
/** End of group 87x3e_GDMA_Declaration
  * @}
  */

/** End of Group 87x3e_GDMA_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/


/** @defgroup 87x3e_GDMA_Exported_Functions GDMA Exported Functions
  * @{
  */

/**
 *
 * \brief  Deinitializes the GDMA registers to their default reset values and GDMA clock will be closed.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_gdma_init(void)
 * {
 *     GDMA_DeInit();
 * }
 * \endcode
 */
void GDMA_DeInit(void);


/**
 *
 * \brief     Initializes the GDMA channelx according to the specified
 *            parameters in the GDMA_InitStruct.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 * \param[in] GDMA_InitStruct: Pointer to a \ref _X3E_GDMA_InitTypeDef structure that
 *            contains the configuration information for the specified GDMA Channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t uart_tx_dma_ch_num = 0xa5;
 * uint8_t UART_TX_DMA_CHANNEL_NUM = uart_tx_dma_ch_num;
 * #define UART_TX_DMA_CHANNEL    DMA_CH_BASE(uart_tx_dma_ch_num)
 * #define UART_TX_DMA_IRQ        DMA_CH_IRQ(uart_tx_dma_ch_num)
 * uint8_t GDMA_SendBuffer[100];
 *
 * void driver_gdma_init(void)
 * {
 *     uint16_t  strLen = 0;
 *     char *demoStr = "### Welcome to use RealTek Bumblebee ###\r\n";
 *     strLen = strlen(demoStr);
 *     memcpy(GDMA_SendBuffer, demoStr, strLen);
 *
 *     GDMA_InitTypeDef GDMA_InitStruct;
 *     GDMA_StructInit(&GDMA_InitStruct);
 *     GDMA_InitStruct.GDMA_ChannelNum      = UART_TX_DMA_CHANNEL_NUM;
 *     GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToPeripheral;
 *     GDMA_InitStruct.GDMA_BufferSize      = strLen;//determine total transfer size
 *     GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
 *     GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
 *     GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
 *     GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
 *     GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
 *     GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
 *     GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)GDMA_SendBuffer;
 *     GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(&(UART->RB_THR));
 *     GDMA_InitStruct.GDMA_DestHandshake   = GDMA_Handshake_UART0_TX;
 *     GDMA_Init(UART_TX_DMA_CHANNEL, &GDMA_InitStruct);
 * }
 * \endcode
 */
extern void (*GDMA_Init)(GDMA_ChannelTypeDef *GDMA_Channelx, GDMA_InitTypeDef *GDMA_InitStruct);


/**
 *
 * \brief     Fills each GDMA_InitStruct member with its default value.
 *
 * \param[in] GDMA_InitStruct: Pointer to a \ref _X3E_GDMA_InitTypeDef structure which will be initialized.
 *
 * \note   The default settings for the GDMA_InitStruct member are shown in the following table:
 *         | GDMA_InitStruct Member      | Default Value                      |
 *         |:---------------------------:|:----------------------------------:|
 *         | GDMA_ChannelNum             | 0                                  |
 *         | GDMA_DIR                    | \ref GDMA_DIR_PeripheralToMemory   |
 *         | GDMA_BufferSize             | 200                                |
 *         | GDMA_SourceInc              | \ref DMA_SourceInc_Fix             |
 *         | GDMA_DestinationInc         | \ref DMA_DestinationInc_Inc        |
 *         | GDMA_SourceDataSize         | \ref GDMA_DataSize_Byte            |
 *         | GDMA_DestinationDataSize    | \ref GDMA_DataSize_Byte            |
 *         | GDMA_SourceMsize            | \ref GDMA_Msize_1                  |
 *         | GDMA_DestinationMsize       | \ref GDMA_Msize_1                  |
 *         | GDMA_SourceAddr             | 0                                  |
 *         | GDMA_DestinationAddr        | 0                                  |
 *         | GDMA_ChannelPriority        | 0                                  |
 *         | GDMA_Multi_Block_Mode       | \ref LLI_TRANSFER                  |
 *         | GDMA_SourceHandshake        | 0                                  |
 *         | GDMA_DestHandshake          | 0                                  |
 *         | GDMA_Multi_Block_En         | DISABLE                            |
 *         | GDMA_Scatter_En             | DISABLE                            |
 *         | GDMA_Gather_En              | DISABLE                            |
 *         | GDMA_Source_Cir_Gather_Num  | 1                                  |
 *         | GDMA_Dest_Cir_Sca_Num       | 1                                  |
 *         | GDMA_Multi_Block_Struct     | 0                                  |
 *         | GDMA_ScatterCount           | 0                                  |
 *         | GDMA_ScatterInterval        | 0                                  |
 *         | GDMA_GatherCount            | 0                                  |
 *         | GDMA_GatherInterval         | 0                                  |
 *
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t uart_tx_dma_ch_num = 0xa5;
 * uint8_t UART_TX_DMA_CHANNEL_NUM = uart_tx_dma_ch_num;
 * #define UART_TX_DMA_CHANNEL    DMA_CH_BASE(uart_tx_dma_ch_num)
 * #define UART_TX_DMA_IRQ        DMA_CH_IRQ(uart_tx_dma_ch_num)
 * uint8_t GDMA_SendBuffer[100];
 *
 * void driver_gdma_init(void)
 * {
 *     uint16_t  strLen = 0;
 *     char *demoStr = "### Welcome to use RealTek Bumblebee ###\r\n";
 *     strLen = strlen(demoStr);
 *     memcpy(GDMA_SendBuffer, demoStr, strLen);
 *
 *     GDMA_InitTypeDef GDMA_InitStruct;
 *     GDMA_StructInit(&GDMA_InitStruct);
 *     GDMA_InitStruct.GDMA_ChannelNum      = UART_TX_DMA_CHANNEL_NUM;
 *     GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToPeripheral;
 *     GDMA_InitStruct.GDMA_BufferSize      = strLen;//determine total transfer size
 *     GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
 *     GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
 *     GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
 *     GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
 *     GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
 *     GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
 *     GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)GDMA_SendBuffer;
 *     GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(&(UART->RB_THR));
 *     GDMA_InitStruct.GDMA_DestHandshake   = GDMA_Handshake_UART0_TX;
 *     GDMA_Init(UART_TX_DMA_CHANNEL, &GDMA_InitStruct);
 * }
 * \endcode
 */
extern void GDMA_StructInit(GDMA_InitTypeDef *GDMA_InitStruct);


/**
 *
 * \brief  Enables or disables the selected GDMA channel.
 *
 * \param[in]  GDMA_Channel_Num: GDMA channel number, which can be 0~8.
 * \param[in]  NewState: New state of the selected GDMA channel.
 *             This parameter can be one of the following values:
 *             - ENABLE: Enable the selected GDMA channel.
 *             - DISABLE: Disable the selected GDMA channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t uart_tx_dma_ch_num = 0xa5;
 * uint8_t UART_TX_DMA_CHANNEL_NUM = uart_tx_dma_ch_num;
 *
 * void driver_gdma_init(void)
 * {
 *     GDMA_Cmd(UART_TX_DMA_CHANNEL_NUM, ENABLE);
 * }
 * \endcode
 */
extern uint8_t (*GDMA_Cmd)(uint8_t GDMA_Channel_Num, FunctionalState NewState);

/**
 *
 * \brief   Enable or disable the specified GDMA channelx interrupt source.
 *
 * \param[in] GDMA_Channel_Num: GDMA channel number, which can be 0~8.
 * \param[in] GDMA_IT: Specifies the GDMA interrupt source to be enabled or disabled. \ref x3e_DMA_interrupts_definition.
 *            This parameter can be any combination of the following values:
 *            - GDMA_INT_Transfer: The interrupt is generated on GDMA transfer completion to the destination.
 *            - GDMA_INT_Block: The interrupt is generated on GDMA block transfer completion to the destination.
 *            - GDMA_INT_SrcTransfer: The interrupt is generated after completion of the last OCP transfer of the requested single/burst transaction from the handshaking interface.
 *            - GDMA_INT_DstTransfer: The interrupt is generated on GDMA transfer completion to the destination peripheral.
 *            - GDMA_INT_Error: The interrupt is generated when an ERROR response is received from the slave on the SRESP bus during a GDMA transfer.
 * \param[in] NewState: New state of the specified GDMA interrupt source.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified GDMA channelx interrupt source.
 *            - DISABLE: Disable the specified GDMA channelx interrupt source.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t uart_tx_dma_ch_num = 0xa5;
 * uint8_t UART_TX_DMA_CHANNEL_NUM = uart_tx_dma_ch_num;
 * #define UART_TX_DMA_CHANNEL    DMA_CH_BASE(uart_tx_dma_ch_num)
 * #define UART_TX_DMA_IRQ        DMA_CH_IRQ(uart_tx_dma_ch_num)
 * uint8_t GDMA_SendBuffer[100];
 *
 * void driver_gdma_init(void)
 * {
 *     GDMA_INTConfig(UART_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
 *
 *     NVIC_InitTypeDef NVIC_InitStruct;
 *     NVIC_InitStruct.NVIC_IRQChannel = UART_TX_DMA_IRQ;
 *     NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
 *     NVIC_InitStruct.NVIC_IRQChannelCmd = (FunctionalState)ENABLE;
 *     NVIC_Init(&NVIC_InitStruct);
 * }
 * \endcode
 */
extern void GDMA_INTConfig(uint8_t GDMA_Channel_Num, uint32_t GDMA_IT, FunctionalState NewState);

/**
 *
 * \brief  Clear the specified GDMA channelx interrupt pending bit.
 *
 * \param[in] GDMA_Channel_Num: GDMA channel number, which can be 0~8.
 *
 * \param[in] GDMA_IT: Specifies the GDMA interrupt source to be enabled or disabled. \ref x3e_DMA_interrupts_definition.
 *            This parameter can be any combination of the following values:
 *            - GDMA_INT_Transfer: The interrupt is generated on GDMA transfer completion to the destination.
 *            - GDMA_INT_Block: The interrupt is generated on GDMA block transfer completion to the destination.
 *            - GDMA_INT_SrcTransfer: The interrupt is generated after completion of the last OCP transfer of the requested single/burst transaction from the handshaking interface.
 *            - GDMA_INT_DstTransfer: The interrupt is generated on GDMA transfer completion to the destination peripheral.
 *            - GDMA_INT_Error: The interrupt is generated when an ERROR response is received from the slave on the SRESP bus during a GDMA transfer.
 *
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t uart_tx_dma_ch_num = 0xa5;
 * uint8_t UART_TX_DMA_CHANNEL_NUM = uart_tx_dma_ch_num;
 *
 * void UART_TX_GDMA_Handler(void)
 * {
 *     GDMA_ClearINTPendingBit(UART_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
 *     //Add user code here.
 * }
 * \endcode
 */
extern void GDMA_ClearINTPendingBit(uint8_t GDMA_Channel_Num, uint32_t GDMA_IT);


/**
 *
 * \brief  Suspend GDMA transmission safe from the source. Please check GDMA FIFO empty to guarnatee without losing data.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 *
 * \return The result of suspend GDMA transmission.
 * \retval true: Suspend GDMA transmission success.
 * \retval false: Suspend GDMA transmission failed.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t uart_rx_dma_ch_num = 0xa5;
 * #define UART_RX_DMA_CHANNEL_NUM   uart_rx_dma_ch_num
 * #define UART_RX_DMA_CHANNEL       DMA_CH_BASE(uart_rx_dma_ch_num)
 *
 * void gdma_demo(void)
 * {
 *     GDMA_SafeSuspend(UART_RX_DMA_CHANNEL);
 *     //Add user code here.
 * }
 * \endcode
 */
bool GDMA_SafeSuspend(GDMA_ChannelTypeDef *GDMA_Channelx);


/**
 *
 * \brief   Get the selected GDMA channel status.
 *
 * \param[in] GDMA_Channel_Num: GDMA channel number, which can be 0~8.
 *
 * \return  The status of GDMA channel.
 * \retval  SET: Channel is be used.
 * \retval  RESET: Channel is free.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void Data_Uart_Handler(void)
 * {
 *     if (UART_GetFlagState(UART, UART_FLAG_RX_IDLE) == SET)
 *     {
 *         UART_INTConfig(UART, UART_INT_IDLE, DISABLE);
 *         if (GDMA_GetChannelStatus(UART_RX_DMA_CHANNEL_NUM))
 *         {
 *             GDMA_SuspendCmd(UART_RX_DMA_CHANNEL, ENABLE);
 *         }
 *     }
 * }
 * \endcode
 */
static __forceinline FlagStatus GDMA_GetChannelStatus(uint8_t GDMA_Channel_Num)
{
    FlagStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_GDMA_ChannelNum(GDMA_Channel_Num));

    if ((GDMA_BASE->ChEnReg & CHANNEL_BIT(GDMA_Channel_Num)) != (uint32_t)RESET)
    {
        bit_status = SET;
    }

    /* Return the selected channel status */
    return  bit_status;
}


/**
 *
 * \brief  Check whether GDMA Channel transfer interrupt is set.
 *
 * \param[in] GDMA_Channel_Num: GDMA channel number, which can be 0~8.
 *
 * \return  Transfer interrupt status, SET or RESET.
 * \retval SET: GDMA Channel transfer interrupt is set.
 * \retval RESET: GDMA Channel transfer interrupt is unset.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * #define LCD_RST                          P4_0
 * #define LCD_DMA_CHANNEL_NUM              lcd_dma_ch_num
 *
 * void lcd_wait_dma_transfer(void)
 * {
 *     while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
 *     GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
 * }
 * \endcode
 */
static __forceinline ITStatus GDMA_GetTransferINTStatus(uint8_t GDMA_Channel_Num)
{
    ITStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_GDMA_ChannelNum(GDMA_Channel_Num));

    if ((GDMA_BASE->STATUS_TFR & BIT(GDMA_Channel_Num)) != (uint32_t)RESET)
    {

        bit_status = SET;
    }

    /* Return the transfer interrupt status */
    return  bit_status;
}


/**
 *
 * \brief     Clear GDMA channelx all type interrupt.
 *
 * \param[in] GDMA_Channel_Num: GDMA channel number, which can be 0~8.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t uart_tx_dma_ch_num = 0xa5;
 * uint8_t UART_TX_DMA_CHANNEL_NUM = uart_tx_dma_ch_num;
 *
 * void gdma_demo(void)
 * {
 *     GDMA_ClearAllTypeINT(UART_TX_DMA_CHANNEL_NUM);
 * }
 * \endcode
 */
static __forceinline void GDMA_ClearAllTypeINT(uint8_t GDMA_Channel_Num)
{
    /* Check the parameters */
    assert_param(IS_GDMA_ChannelNum(GDMA_Channel_Num));

    GDMA_BASE->CLEAR_TFR = BIT(GDMA_Channel_Num);
    GDMA_BASE->CLEAR_BLOCK = BIT(GDMA_Channel_Num);
    GDMA_BASE->CLEAR_DST_TRAN = BIT(GDMA_Channel_Num);
    GDMA_BASE->CLEAR_SRC_TRAN = BIT(GDMA_Channel_Num);
    GDMA_BASE->CLEAR_ERR = BIT(GDMA_Channel_Num);
}


/**
 *
 * \brief     Set GDMA transmission source address.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 * \param[in] Address: Source address.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t console_tx_dma_ch_num = 0xa5;
 * #define UART_TX_DMA_CHANNEL         DMA_CH_BASE(console_tx_dma_ch_num)
 *
 * void gdma_demo(void)
 * {
 *     uint32_t data_buf[10] = {0};
 *     GDMA_SetSourceAddress(UART_TX_DMA_CHANNEL, (uint32_t)data_buf);
 * }
 * \endcode
 */
static __forceinline void GDMA_SetSourceAddress(GDMA_ChannelTypeDef *GDMA_Channelx,
                                                uint32_t Address)
{
    /* Check the parameters */
    assert_param(IS_GDMA_ALL_PERIPH(GDMA_Channelx));

    GDMA_Channelx->SAR = Address;
}


/**
 *
 * \brief     Set GDMA transmission destination address.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 * \param[in] Address: Destination address.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t console_rx_dma_ch_num = 0xa5;
 * #define UART_RX_DMA_CHANNEL    DMA_CH_BASE(console_rx_dma_ch_num)
 *
 * void gdma_demo(void)
 * {
 *     uint32_t data_buf[10] = {0};
 *     GDMA_SetDestinationAddress(UART_RX_DMA_CHANNEL, (uint32_t)data_buf);
 * }
 * \endcode
 */
static __forceinline void GDMA_SetDestinationAddress(GDMA_ChannelTypeDef *GDMA_Channelx,
                                                     uint32_t Address)
{
    /* Check the parameters */
    assert_param(IS_GDMA_ALL_PERIPH(GDMA_Channelx));

    GDMA_Channelx->DAR = Address;
}


/**
 *
 * \brief     Set GDMA buffer size.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 * \param[in] buffer_size: Set GDMA BufferSize, max size is 0xffff.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * uint8_t console_tx_dma_ch_num = 0xa5;
 * #define UART_TX_DMA_CHANNEL         DMA_CH_BASE(console_tx_dma_ch_num)
 *
 * void gdma_demo(void)
 * {
 *     uint32_t data_buf_size = 4095;
 *     GDMA_SetBufferSize(UART_TX_DMA_CHANNEL, data_buf_size);
 * }
 * \endcode
 */
static __forceinline void GDMA_SetBufferSize(GDMA_ChannelTypeDef *GDMA_Channelx,
                                             uint32_t buffer_size)
{
    /* Check the parameters */
    assert_param(IS_GDMA_ALL_PERIPH(GDMA_Channelx));

    /* configure high 32 bit of CTL register */
    GDMA_Channelx->CTL_HIGH = (buffer_size & 0xFFFF);
}


/**
 *
 * \brief     Suspend GDMA transmission from the source. Please check GDMA FIFO empty to guarnatee without losing data.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 * \param[in] NewState: Enable or disable suspend GDMA transmission.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable suspend GDMA transmission.
 *            - DISABLE: Disable suspend GDMA transmission.
 *
 * \note      To prevent data loss, it is necessary to check whether FIFO data transmission is completed
 *            after suspend, and judge by checking whether GDMA FIFO is empty.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void Data_Uart_Handler(void)
 * {
 *     uint16_t rx_count;
 *     if (UART_GetFlagState(UART, UART_FLAG_RX_IDLE) == SET)
 *     {
 *         //restart GDMA receive
 *         GDMA_SetDestinationAddress(UART_RX_DMA_CHANNEL, (uint32_t)uart_receive_buf);
 *         GDMA_SuspendCmd(UART_RX_DMA_CHANNEL, DISABLE);
 *         GDMA_Cmd(UART_RX_DMA_CHANNEL_NUM, ENABLE);
 *
 *         UART_INTConfig(UART0, UART_INT_IDLE, ENABLE);
 *     }
 * }
 * \endcode
 */
static __forceinline void GDMA_SuspendCmd(GDMA_ChannelTypeDef *GDMA_Channelx,
                                          FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_GDMA_ALL_PERIPH(GDMA_Channelx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState == DISABLE)
    {
        /* Not suspend transmission*/
        GDMA_Channelx->CFG_LOW &= ~(GDMA_SUSPEND_TRANSMISSSION);
    }
    else
    {
        /* Suspend transmission */
        GDMA_Channelx->CFG_LOW |= GDMA_SUSPEND_TRANSMISSSION;
    }
}


/**
 *
 * \brief     Check whether GDMA FIFO is empty.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 *
 * \return The status of GDMA FIFO.
 * \retval SET: GDMA FIFO is empty.
 * \retval RESET: GDMA FIFO is not empty.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void Data_Uart_Handler(void)
 * {
 *     uint16_t rx_count;
 *     if (UART_GetFlagState(UART, UART_FLAG_RX_IDLE) == SET)
 *     {
 *         UART_INTConfig(UART, UART_INT_IDLE, DISABLE);
 *         if (GDMA_GetChannelStatus(UART_RX_DMA_CHANNEL_NUM))
 *         {
 *             GDMA_SuspendCmd(UART_RX_DMA_CHANNEL, ENABLE);
 *         }
 *         while (GDMA_GetFIFOStatus(UART_RX_DMA_CHANNEL) != SET);
 *     }
 * }
 * \endcode
 */
static __forceinline FlagStatus GDMA_GetFIFOStatus(GDMA_ChannelTypeDef *GDMA_Channelx)
{
    FlagStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_GDMA_ALL_PERIPH(GDMA_Channelx));

    if ((GDMA_Channelx->CFG_LOW & GDMA_FIFO_STATUS) != (uint32_t)RESET)
    {
        if ((GDMA_Channelx->CFG_LOW & BIT1) && (GDMA_Channelx->CFG_LOW & BIT2))
        {
            bit_status = SET;
        }
    }

    /* Return the selected channel status */
    return  bit_status;
}


/**
 *
 * \brief     Get GDMA transfer data length.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 *
 * \return    GDMA transfer data length.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void Data_Uart_Handler(void)
 * {
 *     uint16_t rx_count;
 *     if (UART_GetFlagState(UART, UART_FLAG_RX_IDLE) == SET)
 *     {
 *         //read GDMA total transfer count
 *         rx_count = GDMA_GetTransferLen(UART_RX_DMA_CHANNEL);
 *         APP_PRINT_INFO1("rx_count:%d", rx_count);
 *     }
 * }
 * \endcode
 */
static __forceinline uint16_t GDMA_GetTransferLen(GDMA_ChannelTypeDef *GDMA_Channelx)
{
    /* Check the parameters */
    assert_param(IS_GDMA_ALL_PERIPH(GDMA_Channelx));

    return (uint16_t)(GDMA_Channelx->CTL_HIGH & 0xFFFF);
}


/**
 *
 * \brief     Set GDMA LLP stucture address.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 * \param[in] Address: The address of LLP stucture.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void gdma_demo(void)
 * {
 *     GDMA_LLIDef GDMA_LLIStruct[4000];
 *     GDMA_SetLLPAddress(GDMA2_Channel0,(uint32_t)GDMA_LLIStruct);
 * }
 * \endcode
 */
static __forceinline void GDMA_SetLLPAddress(GDMA_ChannelTypeDef *GDMA_Channelx, uint32_t Address)
{
    /* Check the parameters */
    assert_param(IS_GDMA_ALL_PERIPH(GDMA_Channelx));

    GDMA_Channelx->LLP = Address;
}


/**
 *
 * \brief  Check GDMA suspend channel status.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 *
 * \return GDMA suspend channel status.
 * \retval SET: The GDMA suspend channel is inactive.
 * \retval RESET: The GDMA suspend channel is active.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void gdma_demo(void)
 * {
 *     //add user code here.
 *     while (GDMA_GetSuspendChannelStatus(GDMA_Channel0) != SET);
 * }
 * \endcode
 */
static __forceinline FlagStatus GDMA_GetSuspendChannelStatus(GDMA_ChannelTypeDef *GDMA_Channelx)
{
    FlagStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_GDMA_ALL_PERIPH(GDMA_Channelx));

    if ((GDMA_Channelx->CFG_LOW & GDMA_SUSPEND_CHANNEL_STATUS) == GDMA_SUSPEND_CHANNEL_STATUS)
    {
        bit_status = SET;
    }

    /* Return the selected channel suspend status */
    return  bit_status;
}


/**
 *
 * \brief  Check GDMA suspend command status.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 *
 * \return GDMA suspend command status.
 * \retval SET: The GDMA channel is suspended.
 * \retval RESET: The GDMA channel is not suspended.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void gdma_demo(void)
 * {
 *     FlagStatus flag_status = GDMA_GetSuspendCmdStatus(GDMA2_Channel0);
 * }
 * \endcode
 */
static __forceinline FlagStatus GDMA_GetSuspendCmdStatus(GDMA_ChannelTypeDef *GDMA_Channelx)
{
    FlagStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_GDMA_ALL_PERIPH(GDMA_Channelx));

    if ((GDMA_Channelx->CFG_LOW & GDMA_SUSPEND_CMD_STATUS) == GDMA_SUSPEND_CMD_STATUS)
    {
        bit_status = SET;
    }

    /* Return the selected channel suspend status */
    return  bit_status;
}

/**
 *
 * \brief  Update GDMA LLP mode in multi-block.
 *
 * \param[in] GDMA_Channelx: Where x can be 0 to 8 to select the GDMA Channel. \ref x3e_GDMA_Declaration.
 * \param[in] GDMA LLP mode. \ref x3e_GDMA_Multiblock_Mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void gdma_demo(void)
 * {
 *     GDMA_SetLLPMode(LLI_TRANSFER);
 * }
 * \endcode
 */
static __forceinline void GDMA_SetLLPMode(GDMA_ChannelTypeDef *GDMA_Channelx, uint32_t mode)
{
    GDMA_Channelx->CTL_LOW = ((GDMA_Channelx->CTL_LOW & (~LLI_TRANSFER)) | mode);
}

#ifdef __cplusplus
}
#endif

#endif /*__RTL876X_GDMA_H*/

/** @} */ /* End of group 87x3e_GDMA_Exported_Functions */
/** @} */ /* End of group 87x3e_GDMA */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

