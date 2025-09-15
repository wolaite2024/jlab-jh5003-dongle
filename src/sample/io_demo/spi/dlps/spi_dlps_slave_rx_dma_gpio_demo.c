/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_dlps_slave_rx_dma_gpio_demo.c
* @brief    This file provides demo code of SPI slave receive data by dma mode in DLPS situation.
* @details  Wake up by gpio when enter dlps, master toggle gpio before tx enable.
* @author
* @date     2024-3-4
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "string.h"
#include "trace.h"
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "vector_table.h"
#include "rtl876x_gdma.h"
#include "rtl876x_pinmux.h"
#include "dma_channel.h"
#include "os_task.h"
#include "os_msg.h"
#include "pm.h"
#include "io_dlps.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"

/*============================================================================*
 *                              Macros
 *============================================================================*/

#define TEST_SIZE                        6

#define SPI_SLAVE_RX_DMA_CHANNEL_NUM     spi_slave_rx_ch_num
#define SPI_SLAVE_RX_DMA_CHANNEL         DMA_CH_BASE(spi_slave_rx_ch_num)
#define SPI_SLAVE_RX_DMA_IRQ             DMA_CH_IRQ(spi_slave_rx_ch_num)
#define SPI_SLAVE_RX_DMA_VECTOR          DMA_CH_VECTOR(spi_slave_rx_ch_num)

#define PIN_SPI_SCK                      P2_1
#define PIN_SPI_MOSI                     P2_2
#define PIN_SPI_MISO                     P2_3
#define PIN_SPI_CS                       P0_1

#define PIN_GPIO_WAKEUP                  P1_1

#define SPI_CLK_SLAVE                    SPI0_CLK_SLAVE
#define SPI_MO_SLAVE                     SPI0_SI_SLAVE
#define SPI_MI_SLAVE                     SPI0_SO_SLAVE
#define SPI_SS_N_0_SLAVE                 SPI0_SS_N_0_SLAVE

#define SPI_SLAVE                        SPI0
#define SPI_SLAVE_VECTORn                SPI0_VECTORn
#define SPI_SLAVE_IRQn                   SPI0_IRQn
#define APBPeriph_SPI_CLOCK              APBPeriph_SPI0_CLOCK
#define APBPeriph_SPI                    APBPeriph_SPI0

#define DMA_Handshake_SPI_SLAVE_RX       GDMA_Handshake_SPI0_RX

/* queue size */
#define IO_DEMO_EVENT_QUEUE_SIZE         0x10
#define IO_DEMO_EVENT_SPI_RX             0x01
/*============================================================================*
 *                              Variables
 *============================================================================*/
static uint8_t read_buf[TEST_SIZE];
static uint8_t spi_slave_rx_ch_num = 0xa5;
static bool allowedSystemEnterDlps = true;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/* task handle & queue handle */
static void *io_queue_handle;
static void *iodemo_app_task_handle;

static void io_demo_task(void *param);
static void spi_slave_rx_dma_handler(void);

void spi_dlps_slave_rx_dma_gpio_demo(void)
{
    os_task_create(&iodemo_app_task_handle, "app", io_demo_task, NULL, 384 * 4, 2);

    os_msg_queue_create(&io_queue_handle, "spiRxQ", IO_DEMO_EVENT_QUEUE_SIZE, sizeof(uint8_t));
}

static void spi_slave_gpio_cb(uint32_t context)
{
    uint8_t pin_index = (uint32_t)context;
    uint8_t event = IO_DEMO_EVENT_SPI_RX;
    T_GPIO_LEVEL gpio_level = hal_gpio_get_input_level(pin_index);
    IO_PRINT_INFO1("spi_slave_gpio_cb: gpio_level %d", gpio_level);

    if (gpio_level == GPIO_LEVEL_HIGH)
    {
        allowedSystemEnterDlps = false;
        hal_gpio_irq_change_polarity(PIN_GPIO_WAKEUP, GPIO_IRQ_ACTIVE_LOW);

        if (os_msg_send(io_queue_handle, &event, 0) == false)
        {
            IO_PRINT_ERROR0("spi_slave_gpio_cb: Send Queue Error");
        }
    }
    else
    {
        allowedSystemEnterDlps = true;
        hal_gpio_irq_change_polarity(PIN_GPIO_WAKEUP, GPIO_IRQ_ACTIVE_HIGH);
    }
}

static void driver_gpio_init(void)
{
    hal_gpio_init();
    hal_gpio_int_init();

    hal_gpio_init_pin(PIN_GPIO_WAKEUP, GPIO_TYPE_CORE, GPIO_DIR_INPUT, GPIO_PULL_DOWN);
    hal_gpio_set_up_irq(PIN_GPIO_WAKEUP, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_HIGH, false);
    hal_gpio_register_isr_callback(PIN_GPIO_WAKEUP, spi_slave_gpio_cb, PIN_GPIO_WAKEUP);
    hal_gpio_irq_enable(PIN_GPIO_WAKEUP);
}

static void board_spi_init(void)
{
    Pinmux_Config(PIN_SPI_SCK, SPI_CLK_SLAVE);
    Pinmux_Config(PIN_SPI_MOSI, SPI_MO_SLAVE);
    Pinmux_Config(PIN_SPI_MISO, SPI_MI_SLAVE);
    Pinmux_Config(PIN_SPI_CS, SPI_SS_N_0_SLAVE);

    Pad_Config(PIN_SPI_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(PIN_SPI_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

static void driver_spi_slave_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_SPI, APBPeriph_SPI_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction          = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode               = SPI_Mode_Slave;
    SPI_InitStructure.SPI_DataSize           = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL               = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA               = SPI_CPHA_1Edge;

    SPI_InitStructure.SPI_RxWaterlevel       = 7;

    SPI_Init(SPI_SLAVE, &SPI_InitStructure);
    SPI_Cmd(SPI_SLAVE, ENABLE);
}

static void spi_slave_rx_dma_init(void)
{
    for (uint16_t i = 0; i < TEST_SIZE; i++)
    {
        read_buf[i] = 0;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    /*--------------DMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = SPI_SLAVE_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_PeripheralToMemory;
    GDMA_InitStruct.GDMA_BufferSize          = TEST_SIZE;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)SPI_SLAVE->DR;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)read_buf;
    GDMA_InitStruct.GDMA_SourceHandshake     = DMA_Handshake_SPI_SLAVE_RX;

    GDMA_Init(SPI_SLAVE_RX_DMA_CHANNEL, &GDMA_InitStruct);
    GDMA_INTConfig(SPI_SLAVE_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------DMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_SLAVE_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);
}

static void driver_gdma_init(void)
{
    /* get free gdma channel */
    if (!GDMA_channel_request(&spi_slave_rx_ch_num, spi_slave_rx_dma_handler, true))
    {
        return;
    }

    spi_slave_rx_dma_init();
}

static void spi_dma_rx_data(uint8_t *p_recv_buf, uint16_t len)
{
    GDMA_SetBufferSize(SPI_SLAVE_RX_DMA_CHANNEL, len);
    GDMA_SetDestinationAddress(SPI_SLAVE_RX_DMA_CHANNEL, (uint32_t)p_recv_buf);

    /* Enable SPI GDMA Rx Function */
    SPI_GDMACmd(SPI_SLAVE, SPI_GDMAReq_Rx, DISABLE);
    SPI_GDMACmd(SPI_SLAVE, SPI_GDMAReq_Rx, ENABLE);
    GDMA_Cmd(SPI_SLAVE_RX_DMA_CHANNEL_NUM, ENABLE);
}

/**
  * @brief  IO Enter dlps call back function.
  * @param   No parameter.
  * @return  void
  */
static void spi_dlps_enter(void)
{
    System_WakeUpPinEnable(PIN_GPIO_WAKEUP, PAD_WAKEUP_POL_HIGH);

    /* To Debug */
    DBG_DIRECT("spi_dlps_enter");
}

/**
  * @brief  IO Exit dlps call back function.
  * @param   No parameter.
  * @return  void
  */
static void spi_dlps_exit(void)
{
    if (System_WakeUpInterruptValue(PIN_GPIO_WAKEUP) == SET)
    {
        /* recall gpio interrupt handler for enter dlps IO interrupt lose*/
        spi_slave_gpio_cb(PIN_GPIO_WAKEUP);
    }

    IO_PRINT_INFO0("spi_dlps_exit");
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
static bool io_dlps_check(void)
{
    return allowedSystemEnterDlps;
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
static void power_spi_init(void)
{
    power_check_cb_register(io_dlps_check);
    io_dlps_register();

    io_dlps_register_enter_cb(spi_dlps_enter);
    io_dlps_register_exit_cb(spi_dlps_exit);

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    power_mode_set(POWER_DLPS_MODE);
}

static void io_demo_task(void *param)
{
    uint8_t event = 0;
    driver_gpio_init();

    board_spi_init();
    driver_spi_slave_init();
    driver_gdma_init();

    /* Power Setting */
    power_spi_init();

    while (1)
    {
        if (os_msg_recv(io_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == IO_DEMO_EVENT_SPI_RX)
            {
                /* GDMA should reinitialize when exit from DLPS */
                spi_slave_rx_dma_init();
                spi_dma_rx_data(read_buf, TEST_SIZE);
            }

        }
    }
}

static void spi_slave_rx_dma_handler(void)
{
    uint16_t rx_len_all = GDMA_GetTransferLen(SPI_SLAVE_RX_DMA_CHANNEL);
    IO_PRINT_INFO1("spi_slave_rx_dma_handler! rx_len_all %d", rx_len_all);

    GDMA_INTConfig(SPI_SLAVE_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, DISABLE);
    GDMA_ClearINTPendingBit(SPI_SLAVE_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    /* It is recommended to post the os msg to the task thread for data processing. */

    for (uint16_t i = 0; i < TEST_SIZE; i++)
    {
        IO_PRINT_INFO2("spi_slave_rx_dma_handler: read_buf[%d] = 0x%x", i, read_buf[i]);
    }
}

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/
