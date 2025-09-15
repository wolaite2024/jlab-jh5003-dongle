/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dma_scrgar_demo.c
* @brief    This file provides demo code of meomory to memory transfer by Scatter/Gather GDMA.
* @details
* @author   renee
* @date     2017-02-17
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "dma_channel.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"
#include "trace.h"

/** @defgroup  GDMA_SCRGAR_DEMO  GDMA SARGAR Demo
    * @brief  Gdma scatter/gather transfer implementation demo code
    * @{
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup GDMA_SCRGAR_Exported_Variables Gdma Scatter/Gather transfer Variables
  * @brief
  * @{
  */

#define USE_GATHER      0       /*< Set this macro to 1 to use gather function. */
#define USE_SCATTER     1       /*< Set this macro to 1 to use scatter function. */

#define SCATTER_GATHER_BUFFER_SIZE              (20)
#define SCATTER_GATHER_COUNT                    (4)
#define SCATTER_GATHER_INTERVAL                 (1)
#define SCATTER_DEST_SIZE                       ((SCATTER_GATHER_BUFFER_SIZE / SCATTER_GATHER_COUNT) * (SCATTER_GATHER_COUNT + SCATTER_GATHER_INTERVAL))
#define GATHER_SOUR_SIZE                        ((SCATTER_GATHER_BUFFER_SIZE / SCATTER_GATHER_COUNT) * (SCATTER_GATHER_COUNT + SCATTER_GATHER_INTERVAL))

#if (USE_SCATTER == 1)
static uint8_t GDMA_SendBuffer[SCATTER_GATHER_BUFFER_SIZE];
static uint8_t GDMA_RecvBuffer[SCATTER_DEST_SIZE];
#endif

#if (USE_GATHER == 1)
static uint8_t GDMA_SendBuffer[GATHER_SOUR_SIZE];
static uint8_t GDMA_RecvBuffer[SCATTER_GATHER_BUFFER_SIZE];
#endif

static uint8_t scatter_gather_dma_ch_num = 0xa5;

#define SCATTER_GATHER_DMA_CHANNEL_NUM     scatter_gather_dma_ch_num
#define SCATTER_GATHER_DMA_CHANNEL         DMA_CH_BASE(scatter_gather_dma_ch_num)
#define SCATTER_GATHER_DMA_IRQ             DMA_CH_IRQ(scatter_gather_dma_ch_num)
/** @} */ /* End of group GDMA_SCRGAR_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup GDMA_SCRGAR_Exported_Functions Gdma Scatter/Gather transfer Functions
  * @brief
  * @{
  */


static void scatter_gather_check_data(void)
{

#if (USE_SCATTER == 1)
    uint32_t scatter_cnt = SCATTER_GATHER_BUFFER_SIZE / SCATTER_GATHER_COUNT;
    uint8_t send_value = 0;
    uint8_t recv_value = 0;

    for (uint32_t i = 0; i < scatter_cnt; i++)
    {
        for (uint32_t j = 0; j < SCATTER_GATHER_COUNT; j++)
        {
            send_value = GDMA_SendBuffer[i * SCATTER_GATHER_COUNT + j];
            recv_value = GDMA_RecvBuffer[i * (SCATTER_GATHER_COUNT + SCATTER_GATHER_INTERVAL) + j];

            if (send_value != recv_value)
            {
                IO_PRINT_ERROR4("scatter_gather_check_data: scatter check data failed i %d j %d source %d dest %d",
                                i, j, send_value,
                                recv_value);
            }
        }
    }
#endif

#if (USE_GATHER == 1)
    uint32_t gather_cnt = SCATTER_GATHER_BUFFER_SIZE / SCATTER_GATHER_COUNT;
    uint8_t send_value = 0;
    uint8_t recv_value = 0;

    for (uint32_t i = 0; i < gather_cnt; i++)
    {
        for (uint32_t j = 0; j < SCATTER_GATHER_COUNT; j++)
        {
            send_value = GDMA_SendBuffer[i * (SCATTER_GATHER_COUNT + SCATTER_GATHER_INTERVAL) + j];
            recv_value = GDMA_RecvBuffer[i * SCATTER_GATHER_COUNT + j];

            if (send_value != recv_value)
            {
                IO_PRINT_ERROR4("scatter_gather_check_data: gather check data failed i %d j %d source %d dest %d",
                                i, j, send_value,
                                recv_value);
            }
        }
    }
#endif
}
/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void scatter_gather_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SCATTER_GATHER_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    scatter_gather_check_data();

    IO_PRINT_INFO0("scatter_gather_dma_handler: Data transmission completion!");
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
void dma_scrgar_demo(void)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef GDMA_InitStruct;

    if (!GDMA_channel_request(&scatter_gather_dma_ch_num, scatter_gather_dma_handler, true))
    {
        return;
    }

#if (USE_SCATTER == 1)
    for (uint32_t i = 0; i < SCATTER_GATHER_BUFFER_SIZE; i++)
    {
        GDMA_SendBuffer[i] = i % 256;
    }

    for (uint32_t i = 0; i < SCATTER_DEST_SIZE; i++)
    {
        GDMA_RecvBuffer[i] = 0;
    }
#endif

#if (USE_GATHER == 1)
    uint32_t gather_cnt = SCATTER_GATHER_BUFFER_SIZE / SCATTER_GATHER_COUNT;

    for (uint32_t i = 0; i < gather_cnt; i++)
    {
        for (uint32_t j = 0; j < SCATTER_GATHER_COUNT; j++)
        {
            GDMA_SendBuffer[i * (SCATTER_GATHER_COUNT + SCATTER_GATHER_INTERVAL) + j] = i;
        }
    }

    for (uint32_t i = 0; i < SCATTER_GATHER_BUFFER_SIZE; i++)
    {
        GDMA_RecvBuffer[i] = 0;
    }
#endif

    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SCATTER_GATHER_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToMemory;
    GDMA_InitStruct.GDMA_BufferSize      = SCATTER_GATHER_BUFFER_SIZE;//determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)GDMA_SendBuffer;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)GDMA_RecvBuffer;
#if (USE_SCATTER == 1)
    GDMA_InitStruct.GDMA_Scatter_En = ENABLE;
    GDMA_InitStruct.GDMA_ScatterCount = SCATTER_GATHER_COUNT;
    GDMA_InitStruct.GDMA_ScatterInterval = SCATTER_GATHER_INTERVAL;
#endif

#if (USE_GATHER == 1)
    GDMA_InitStruct.GDMA_Gather_En = ENABLE;
    GDMA_InitStruct.GDMA_GatherCount = SCATTER_GATHER_COUNT;
    GDMA_InitStruct.GDMA_GatherInterval = SCATTER_GATHER_INTERVAL;
#endif
    /*  Enable GDMA IRQ  */
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SCATTER_GATHER_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    GDMA_Init(SCATTER_GATHER_DMA_CHANNEL, &GDMA_InitStruct);

    GDMA_INTConfig(SCATTER_GATHER_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    GDMA_Cmd(SCATTER_GATHER_DMA_CHANNEL_NUM, ENABLE);
}

/** @} */ /* End of group GDMA_SCRGAR_Exported_Functions */
/** @} */ /* End of group GDMA_SCRGAR_DEMO */
