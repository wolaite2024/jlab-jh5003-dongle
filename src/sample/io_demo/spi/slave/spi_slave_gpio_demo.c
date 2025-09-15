/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     spi_slave_gpio_demo.c
* @brief    This file provides demo code of SPI master rx and slave tx by gpio in interrupt mode.
* @details  Because the SPI Slave does not support sending and receiving data when the FIFO is empty,
            the demo toggles a GPIO pin to notify the master after the slave sends SPI data.
* @author
* @date
* @version
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "vector_table.h"
#include "trace.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"
#include "section.h"

/** @defgroup  SPI_SLAVE_GPIO_DEMO  SPI SLAVE GPIO DEMO
    * @brief  Spi work in slave mode implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup SPI_Demo_Exported_Macros SPI Demo Exported Macros
  * @brief
  * @{
  */


/* Master Macros*/
#define SPI1_SCK                P1_4
#define SPI1_MOSI               P1_5
#define SPI1_MISO               P1_6
#define SPI1_CS                 P1_7

/* Slave Macros*/
#define SPI0_SCK                P1_0
#define SPI0_MOSI               P1_1
#define SPI0_MISO               P1_2
#define SPI0_CS                 P1_3

#define GPIO_PIN1               P2_3 //input
#define GPIO_PIN0               P2_4 //output

static uint8_t SPI_WriteBuf[30] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

static void spi0_handler(void);
static void spi1_handler(void);

ISR_TEXT_SECTION
static void gpio_isr_cb(uint32_t context)
{
    uint8_t pin_index = (uint32_t)context;
    T_GPIO_LEVEL gpio_level = hal_gpio_get_input_level(pin_index);

    IO_PRINT_INFO2("gpio_isr_cb: pin_name %s, gpio_level %d", TRACE_STRING(Pad_GetPinName(pin_index)),
                   gpio_level);

    SPI_SendBuffer(SPI1, SPI_WriteBuf, 6);
    SPI_INTConfig(SPI1, SPI_INT_TXE, ENABLE);
}

static void gpio_input_mode_test(void)
{
    IO_PRINT_TRACE0("gpio_input_mode_test");

    hal_gpio_init();
    hal_gpio_int_init();
    hal_gpio_set_debounce_time(30);

    hal_gpio_init_pin(GPIO_PIN0, GPIO_TYPE_AUTO, GPIO_DIR_OUTPUT, GPIO_PULL_UP);
    hal_gpio_init_pin(GPIO_PIN1, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_UP);
    hal_gpio_set_up_irq(GPIO_PIN1, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
    hal_gpio_register_isr_callback(GPIO_PIN1, gpio_isr_cb, GPIO_PIN1);
    hal_gpio_irq_enable(GPIO_PIN1);
}

static void board_spi_init(void)
{
    Pinmux_Config(SPI1_SCK, SPI1_CLK_MASTER);
    Pinmux_Config(SPI1_MOSI, SPI1_MO_MASTER);
    Pinmux_Config(SPI1_MISO, SPI1_MI_MASTER);
    Pinmux_Config(SPI1_CS, SPI1_SS_N_0_MASTER);

    Pad_Config(SPI1_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI1_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pinmux_Config(SPI0_SCK, SPI0_CLK_SLAVE);
    Pinmux_Config(SPI0_MOSI, SPI0_SI_SLAVE);
    Pinmux_Config(SPI0_MISO, SPI0_SO_SLAVE);
    Pinmux_Config(SPI0_CS, SPI0_SS_N_0_SLAVE);

    Pad_Config(SPI0_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

static void spi_master_init(void)
{
    /* turn on SPI clock */
    RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 400;

    SPI_InitStructure.SPI_TxThresholdLevel  = 0;
    SPI_InitStructure.SPI_RxThresholdLevel  = 0; //the receive FIFO Threshold
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);

    /* detect receive data */
    SPI_INTConfig(SPI1, SPI_INT_RXF, ENABLE);
    RamVectorTableUpdate(SPI1_VECTORn, (IRQ_Fun)spi1_handler);
    /* Config SPI interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = SPI1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

static void spi_slave_init(void)
{
    /* turn on SPI clock */
    SPI_DeInit(SPI0);
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Slave;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 400;

    SPI_InitStructure.SPI_TxThresholdLevel  = 0;
    SPI_InitStructure.SPI_RxThresholdLevel  = 0;
    /* cause SPI_INT_RXF interrupt if data length in receive FIFO  >= SPI_RxThresholdLevel + 1*/
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_Init(SPI0, &SPI_InitStructure);

    SPI_Cmd(SPI0, ENABLE);

    /* detect receive data */
    SPI_INTConfig(SPI0, SPI_INT_TUF | SPI_INT_RXF, ENABLE);
    RamVectorTableUpdate(SPI0_VECTORn, (IRQ_Fun)spi0_handler);

    /* Config SPI interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = SPI0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

void spi_slave_gpio_demo(void)
{
    /* Initialize GPIO peripheral */
    gpio_input_mode_test();

    /* Configure PAD and pinmux firstly! */
    board_spi_init();

    /* Initialize SPI peripheral */
    spi_master_init(); //SPI1

    spi_slave_init(); //SPI0

    /*---------------send demo buffer--------------*/
    SPI_SendBuffer(SPI0, SPI_WriteBuf, 6); //SPI0 send data
    SPI_INTConfig(SPI0, SPI_INT_TXE, ENABLE);

    hal_gpio_set_level(GPIO_PIN0, GPIO_LEVEL_LOW);
}

static void spi0_handler(void)
{
    uint8_t len = 0;
    uint8_t idx = 0;
    uint8_t SPI_ReadINTBuf[30] = {0};

    if (SPI_GetINTStatus(SPI0, SPI_INT_RXF) == SET)
    {
        /* It is recommended to post the os msg to the task thread for data processing. */
        len = SPI_GetRxFIFOLen(SPI0);
        IO_PRINT_INFO1("spi0_handler: len %d", len);

        for (idx = 0; idx < len; idx++)
        {
            /* must read all data in receive FIFO , otherwise cause SPI_INT_RXF interrupt again */
            SPI_ReadINTBuf[idx] = SPI_ReceiveData(SPI0);

            IO_PRINT_INFO2("spi0_handler: SPI_ReadINTBuf[%d] 0x%x", idx, SPI_ReadINTBuf[idx]);
        }
    }

    if (SPI_GetINTStatus(SPI0, SPI_INT_TUF) == SET)
    {
        SPI_INTConfig(SPI0, SPI_INT_TUF, DISABLE);
        IO_PRINT_INFO0("spi0_handler: SPI0 TX Underflow");
    }

    if (SPI_GetINTStatus(SPI0, SPI_INT_TXE) == SET)
    {
        SPI_INTConfig(SPI0, SPI_INT_TXE, DISABLE);
        IO_PRINT_INFO0("spi0_handler: SPI0 TX FIFO Empty");
    }
}

static void spi1_handler(void)
{
    uint8_t len1 = 0;
    uint8_t idx1 = 0;
    uint8_t SPI1_ReadINTBuf[30] = {0};

    if (SPI_GetINTStatus(SPI1, SPI_INT_TXE) == SET)
    {
        SPI_INTConfig(SPI1, SPI_INT_TXE, DISABLE);
        /* It is recommended to post the os msg to the task thread for data processing. */
        IO_PRINT_INFO0("spi1_handler: SPI1 TX FIFO Empty");
    }

    if (SPI_GetINTStatus(SPI1, SPI_INT_RXF) == SET)
    {
        /* It is recommended to post the os msg to the task thread for data processing. */
        len1 = SPI_GetRxFIFOLen(SPI1);
        IO_PRINT_INFO1("spi1_handler: len1 %d", len1);

        for (idx1 = 0; idx1 < len1; idx1++)
        {
            /* must read all data in receive FIFO , otherwise cause SPI_INT_RXF interrupt again */
            SPI1_ReadINTBuf[idx1] = SPI_ReceiveData(SPI1);

            IO_PRINT_INFO2("spi1_handler: SPI1_ReadINTBuf[%d] 0x%x", idx1, SPI1_ReadINTBuf[idx1]);
        }
    }
}

/** @} */ /* End of group SPI_Slave_GPIO_Demo_Exported_Functions */
/** @} */ /* End of group SPI_SLAVE_GPIO_DEMO */
