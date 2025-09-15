/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dma_multiblock_demo.c
* @brief    This file provides demo code of meomory to memory transfer by GDMA Multiblock.
* @details
* @author   renee
* @date     2017-01-23
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "dma_channel.h"
#include "rtl876x_gdma.h"
#include "rtl876x_rcc.h"
#include "rtl876x_nvic.h"
#include "trace.h"

/** @defgroup  GDMA_MULTIBLOCK_DEMO  GDMA multi-block Demo
    * @brief  Gdma multiblock implementation demo code
    * @{
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup GDMA_Multiblock_Demo_Exported_Variables Gdma multiblock Demo Exported Variables
  * @brief
  * @{
  */

#define GDMA_TRANSFER_SIZE              100                 /*< Set this macro to modify the transfer size. */
#define GDMA_MULTIBLOCK_SIZE            12                  /*< Set this macro to modify the block size. */

static uint8_t GDMA_SendBuffer[GDMA_MULTIBLOCK_SIZE][GDMA_TRANSFER_SIZE];
static uint8_t GDMA_RecvBuffer[GDMA_MULTIBLOCK_SIZE][GDMA_TRANSFER_SIZE];

static GDMA_LLIDef GDMA_LLIStruct[GDMA_MULTIBLOCK_SIZE];
static uint8_t multi_block_dma_ch_num = 0xa5;
static uint8_t block_cnt = 0;

#define MULTI_BLOCK_DMA_CHANNEL_NUM     multi_block_dma_ch_num
#define MULTI_BLOCK_DMA_CHANNEL         DMA_CH_BASE(multi_block_dma_ch_num)
#define MULTI_BLOCK_DMA_IRQ             DMA_CH_IRQ(multi_block_dma_ch_num)

/** @} */ /* End of group GDMA_MULTIBLOCK_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup GDMA_Multiblock_Demo_Exported_Functions Gdma multiblock Demo Exported Functions
  * @brief
  * @{
  */
static void multi_block_dma_handler(void);
/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
void dma_multiblock_demo(void)
{
    uint16_t i, j = 0;
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;

    if (!GDMA_channel_request(&multi_block_dma_ch_num, multi_block_dma_handler, true))
    {
        return;
    }

    /*--------------initialize test buffer---------------------*/
    for (i = 0; i < GDMA_TRANSFER_SIZE; i++)
    {
        GDMA_SendBuffer[0][i] = (i & 0xff);
        GDMA_SendBuffer[1][i] = (i + 1) & 0xff;
        GDMA_SendBuffer[2][i] = (i + 2) & 0xff;
        GDMA_SendBuffer[3][i] = (i + 3) & 0xff;
        GDMA_SendBuffer[4][i] = (i + 4) & 0xff;
        GDMA_SendBuffer[5][i] = (i + 5) & 0xff;
        GDMA_SendBuffer[6][i] = ((i + 6) & 0xff);
        GDMA_SendBuffer[7][i] = (i + 7) & 0xff;
        GDMA_SendBuffer[8][i] = (i + 8) & 0xff;
        GDMA_SendBuffer[9][i] = (i + 9) & 0xff;
        GDMA_SendBuffer[10][i] = (i + 10) & 0xff;
        GDMA_SendBuffer[11][i] = (i + 11) & 0xff;
    }
    for (i = 0; i < GDMA_TRANSFER_SIZE; i++)
    {
        for (j = 0; j < GDMA_MULTIBLOCK_SIZE; j++)
        {
            GDMA_RecvBuffer[j][i] = 0;
        }
    }

    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = MULTI_BLOCK_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToMemory;
    GDMA_InitStruct.GDMA_BufferSize      = GDMA_TRANSFER_SIZE;//determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)GDMA_SendBuffer;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)GDMA_RecvBuffer;
    GDMA_InitStruct.GDMA_Multi_Block_Mode = LLI_TRANSFER;
    GDMA_InitStruct.GDMA_Multi_Block_En = 1;
    GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)GDMA_LLIStruct;

    for (int i = 0; i < GDMA_MULTIBLOCK_SIZE; i++)
    {
        if (i == (GDMA_MULTIBLOCK_SIZE - 1))
        {
            //GDMA_LLIStruct[i].LLP=0;
            GDMA_LLIStruct[i].SAR = (uint32_t)GDMA_SendBuffer[i];
            GDMA_LLIStruct[i].DAR = (uint32_t)GDMA_RecvBuffer[i];
            GDMA_LLIStruct[i].LLP = 0;
            /* configure low 32 bit of CTL register */
            GDMA_LLIStruct[i].CTL_LOW = BIT(0)
                                        | (GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                        | (GDMA_InitStruct.GDMA_SourceDataSize << 4)
                                        | (GDMA_InitStruct.GDMA_DestinationInc << 7)
                                        | (GDMA_InitStruct.GDMA_SourceInc << 9)
                                        | (GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                        | (GDMA_InitStruct.GDMA_SourceMsize << 14)
                                        | (GDMA_InitStruct.GDMA_DIR << 20);
            /* configure high 32 bit of CTL register */
            GDMA_LLIStruct[i].CTL_HIGH = GDMA_InitStruct.GDMA_BufferSize;
        }
        else
        {
            GDMA_LLIStruct[i].SAR = (uint32_t)GDMA_SendBuffer[i];
            GDMA_LLIStruct[i].DAR = (uint32_t)GDMA_RecvBuffer[i];
            GDMA_LLIStruct[i].LLP = (uint32_t)&GDMA_LLIStruct[i + 1];
            /* configure low 32 bit of CTL register */
            GDMA_LLIStruct[i].CTL_LOW = BIT(0)
                                        | (GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                        | (GDMA_InitStruct.GDMA_SourceDataSize << 4)
                                        | (GDMA_InitStruct.GDMA_DestinationInc << 7)
                                        | (GDMA_InitStruct.GDMA_SourceInc << 9)
                                        | (GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                        | (GDMA_InitStruct.GDMA_SourceMsize << 14)
                                        | (GDMA_InitStruct.GDMA_DIR << 20)
                                        | (GDMA_InitStruct.GDMA_Multi_Block_Mode & LLP_SELECTED_BIT);
            /* configure high 32 bit of CTL register */
            GDMA_LLIStruct[i].CTL_HIGH = GDMA_InitStruct.GDMA_BufferSize;
        }
    }
    GDMA_Init(MULTI_BLOCK_DMA_CHANNEL, &GDMA_InitStruct);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = MULTI_BLOCK_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_INTConfig(MULTI_BLOCK_DMA_CHANNEL_NUM, GDMA_INT_Block, ENABLE);
    GDMA_Cmd(MULTI_BLOCK_DMA_CHANNEL_NUM, ENABLE);
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void multi_block_dma_handler(void)
{
    GDMA_ClearAllTypeINT(MULTI_BLOCK_DMA_CHANNEL_NUM);
    block_cnt++;

    if (block_cnt == GDMA_MULTIBLOCK_SIZE)
    {
        /* Compare whether the destination data which transported by GDMA is equal to the source data*/
        for (uint32_t i = 0; i < GDMA_MULTIBLOCK_SIZE; i++)
        {
            for (uint32_t j = 0; j < GDMA_TRANSFER_SIZE; j++)
            {
                if (GDMA_SendBuffer[i][j] != GDMA_RecvBuffer[i][j])
                {
                    IO_PRINT_ERROR4("multi_block_dma_handler: Data transmission error! index %d %d GDMA_SendBuffer = %d, GDMA_RecvBuffer = %d",
                                    i, j, GDMA_SendBuffer[i][j], GDMA_RecvBuffer[i][j]);
                }
            }
        }

        IO_PRINT_INFO0("multi_block_dma_handler: Data transmission completion!");
    }
}

/** @} */ /* End of group GDMA_MULTIBLOCK_Exported_Functions */
/** @} */ /* End of group GDMA_MULTIBLOCK_DEMO */

