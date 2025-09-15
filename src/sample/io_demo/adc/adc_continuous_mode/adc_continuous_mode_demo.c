/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     adc_continuous_mode_demo.c
* @brief    This file provides demo code of adc in continuous mode.
* @details
* @author   arthur
* @date     2022-04-02
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "rtl876x_adc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_nvic.h"
#include "rtl876x_gdma.h"
#include "dma_channel.h"
#include "hal_adp.h"
#include "vector_table.h"
#include "adc_manager.h"
#include "charger_api.h"
#include "os_timer.h"
#include "trace.h"

/** @defgroup  ADC_CONTINUOUS_MODE_DEMO ADC Continuous Mode Demo
    * @brief  adc implementation demo code
    * @{
    */

static uint8_t adc_dma_ch_num = 0xa5;
#define ADC_DMA_CHANNEL_NUM            adc_dma_ch_num
#define ADC_DMA_CHANNEL                DMA_CH_BASE(adc_dma_ch_num)
#define ADC_DMA_IRQ                    DMA_CH_IRQ(adc_dma_ch_num)
#define ADC_DMA_BUFFER_SIZE            80
static uint16_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE];

static void adc_dma_handler(void);
static void adc_handler(void);

#define ADC_MANAGER_QUEUE 4

#define ADC_TIMER_PERIOD  2000  /*unit  ms*/

static void *adc_test_timer_handle = NULL;
static void adc_timer_finish_handler(void *xTimer);

/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_adc_init(void)
{
    Pad_Config(ADC_0, PAD_SW_MODE, PAD_SHUTDOWN, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pinmux_Config(ADC_0, IDLE_MODE);
}

/**
  * @brief  Initialize ADC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_adc_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, ENABLE);

    ADC_InitTypeDef adc_init_struct;
    ADC_StructInit(&adc_init_struct);

    adc_init_struct.schIndex[0] = EXT_SINGLE_ENDED(0);
    adc_init_struct.bitmap = 0x01;
    adc_init_struct.adcBurstSize = 8;
    ADC_Init(ADC, &adc_init_struct);

    RamVectorTableUpdate(ADC_VECTORn, (IRQ_Fun)adc_handler);
    ADC_INTConfig(ADC, ADC_INT_FIFO_RD_ERR, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  Initialize GDMA peripheral.
  * @param   No parameter.
  * @return  void
  */
static void adc_gdma_init(void)
{
    uint16_t i = 0;

    for (i = 0; i < ADC_DMA_BUFFER_SIZE; i++)
    {
        adc_dma_buffer[i] = 0;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    if (!GDMA_channel_request(&adc_dma_ch_num, adc_dma_handler, true))
    {
        IO_PRINT_ERROR0("adc_gdma_init: adc gdma request fail");
        return;
    }

    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);

    GDMA_InitStruct.GDMA_ChannelNum      = ADC_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;
    GDMA_InitStruct.GDMA_BufferSize      = 80;  //determine total transfer size
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_8;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_8;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)(&(ADC->FIFO));
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(&adc_dma_buffer[0]);
    GDMA_InitStruct.GDMA_SourceHandshake    = GDMA_Handshake_ADC;

    GDMA_Init(ADC_DMA_CHANNEL, &GDMA_InitStruct);

    GDMA_INTConfig(ADC_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = ADC_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

/**
  * @brief   Deinitialize ADC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_adc_deinit(void)
{
    /* Stop adc_test_timer */
    if (!os_timer_stop(&adc_test_timer_handle))
    {
        IO_PRINT_ERROR0("driver_adc_deinit: adc_test_timer Stop Fail");
    }

    /* Delete adc_test_timer */
    if (!os_timer_delete(&adc_test_timer_handle))
    {
        IO_PRINT_ERROR0("driver_adc_deinit: adc_test_timer Delete Fail");
    }
    else
    {
        adc_test_timer_handle = NULL;
    }

    RCC_PeriphClockCmd(APBPeriph_ADC, APBPeriph_ADC_CLOCK, DISABLE);
    Pinmux_Deinit(ADC_0);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);

    GDMA_Cmd(ADC_DMA_CHANNEL_NUM, DISABLE);
    GDMA_channel_release(ADC_DMA_CHANNEL_NUM);
}

/**
  * @brief  Initialize ADC timer.
  * @param   No parameter.
  * @return  void
  */
static void adc_test_timer_init(void)
{
    /* Create adc_test_timer */
    if (adc_test_timer_handle == NULL)
    {
        if (!os_timer_create(&adc_test_timer_handle, "adc_test_timer", 1,
                             ADC_TIMER_PERIOD, false, adc_timer_finish_handler))
        {
            IO_PRINT_ERROR0("adc_test_timer_init: adc_test_timer Create Fail");
        }
    }
}

/**
  * @brief  Adp plug callback function
  * @param  event: adp plug event
  *     This parameter can be one of the following values:
  *     @arg ADP_EVENT_PLUG_IN: adp plug in.
  *     @arg ADP_EVENT_PLUG_OUT: adp plug out.
  * @param  user_data: user data
  * @retval None
  */
static void adc_continuous_mode_callback(T_ADP_PLUG_EVENT event, void *user_data)
{
    if (event == ADP_EVENT_PLUG_IN)
    {
        /* if you need to charger, please disable ADC continuous mode. */
        driver_adc_deinit();

        /* Initiate ADC Manager module */
        adc_mgr_init(ADC_MANAGER_QUEUE);

        /* Enable charger. */
        charger_api_enable_charger();
    }
    else if (event == ADP_EVENT_PLUG_OUT)
    {
        /* If you need to use ADC continuous mode, you need to turn off
           Charger auto enable and Battery detection support on the McuConfig Tool. */

        /* Disable charger. */
        charger_api_disable_charger();

        /* Close ADC Manager module */
        adc_mgr_deinit();

        /* Initialize ADC timer */
        adc_test_timer_init();

        /* Configure PAD and pinmux */
        board_adc_init();

        /* Initialize ADC peripheral */
        driver_adc_init();

        /* Initialize GDMA peripheral */
        adc_gdma_init();

        GDMA_Cmd(ADC_DMA_CHANNEL_NUM, ENABLE);
        ADC_Cmd(ADC, ADC_Continuous_Mode, ENABLE);
    }
}

/**
  * @brief  demo code of operation about ADC.
  * @param   No parameter.
  * @return  void
  */
void adc_continuous_mode_demo(void)
{
    /* Register adp 5V callback */
    adp_register_state_change_cb(ADP_DETECT_5V, &adc_continuous_mode_callback, NULL);

    /* Initialize ADC timer */
    adc_test_timer_init();

    /* Configure PAD and pinmux */
    board_adc_init();

    /* Initialize ADC peripheral */
    driver_adc_init();

    /* Initialize GDMA peripheral */
    adc_gdma_init();

    GDMA_Cmd(ADC_DMA_CHANNEL_NUM, ENABLE);
    ADC_Cmd(ADC, ADC_Continuous_Mode, ENABLE);
}

/**
* @brief  ADC interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void adc_handler(void)
{
    if (ADC_GetIntFlagStatus(ADC, ADC_INT_FIFO_RD_ERR) == SET)
    {
        IO_PRINT_TRACE0("adc_handler: ADC_INT_FIFO_RD_ERR");
        ADC_ClearINTPendingBit(ADC, ADC_INT_FIFO_RD_ERR);
    }
}

/**
* @brief  GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void adc_dma_handler(void)
{
    uint16_t adc_dma_count = 0;
    uint16_t i = 0;
    int32_t result[ADC_DMA_BUFFER_SIZE] = {0, 0, 0};

    GDMA_ClearINTPendingBit(ADC_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    ADC_Cmd(ADC, ADC_Continuous_Mode, DISABLE);

    adc_dma_count = GDMA_GetTransferLen(ADC_DMA_CHANNEL) / 2;
    for (i = 0; i < adc_dma_count; i++)
    {
        result[i] = ADC_GetRes(adc_dma_buffer[i], EXT_SINGLE_ENDED(0));
        IO_PRINT_TRACE4("adc_dma_handler: adc_dma_buffer[%d] %d, result[%d] %d", i, adc_dma_buffer[i], i,
                        result[i]);
    }

    ADC_ClearFifo(ADC);

    /* Start adc_test_timer */
    if (!os_timer_start(&adc_test_timer_handle))
    {
        IO_PRINT_ERROR0("adc_dma_handler: adc_test_timer Start Fail");
    }
}

/**
 * @brief  adc timer finish handler
 * @param  xTimer: timer
 * @retval None
 */
static void adc_timer_finish_handler(void *xTimer)
{
    IO_PRINT_TRACE0("adc_timer_finish_handler");

    GDMA_SetDestinationAddress(ADC_DMA_CHANNEL, (uint32_t)&adc_dma_buffer[0]);
    GDMA_Cmd(ADC_DMA_CHANNEL_NUM, ENABLE);
    ADC_Cmd(ADC, ADC_Continuous_Mode, ENABLE);
}


/** @} */ /* End of group ADC_CONTINUOUS_MODE_DEMO */

