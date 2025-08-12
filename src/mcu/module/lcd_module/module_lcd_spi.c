/**
*****************************************************************************************
*     Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file    module_lcd_qspi.c
* @brief   This file provides HAL QSPI functions
* @author
* @date    2023-09-15
* @version v1.0
* *************************************************************************************
*/
#include "app_gui.h"
#if (LCD_INTERFACE == LCD_INTERFACE_SPI)
#include "trace.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "module_lcd_spi.h"
#include "platform_utils.h"
#include "rtl876x_spi.h"
#include "rtk_hal_lcd.h"
#include "hal_gpio.h"
#include "lcd_sh8601z_454_spi.h"

static uint8_t lcd_spi_dma_ch_num = 0xa5;

#define LCD_DMA_CHANNEL_NUM              lcd_spi_dma_ch_num
#define LCD_DMA_CHANNEL_INDEX            DMA_CH_BASE(lcd_spi_dma_ch_num)
#define LCD_DMA_CHANNEL_IRQ              DMA_CH_IRQ(lcd_spi_dma_ch_num)

#define SPI_CLK                          SPI0_CLK_MASTER
#define SPI_MOSI                         SPI0_MO_MASTER
#define LCD_SPI_BUS                      SPI0
#define APBPeriph_SPI                    APBPeriph_SPI0
#define APBPeriph_SPI_CLOCK              APBPeriph_SPI0_CLOCK
#define GDMA_HANDSHAKE_SPI_TX            GDMA_Handshake_SPI0_TX

typedef struct _t_lcd_pin
{
    uint8_t lcd_rst;
    uint8_t lcd_dcx;
    uint8_t lcd_spi_cs;
    uint8_t lcd_spi_clk;
    uint8_t lcd_spi_mosi;
} T_LCD_PIN;

T_LCD_PIN lcd_pin;

void lcd_pin_config(uint8_t lcd_rst, uint8_t lcd_dcx, uint8_t lcd_spi_cs, uint8_t lcd_spi_clk,
                    uint8_t lcd_spi_mosi)
{
    lcd_pin.lcd_rst = lcd_rst;
    lcd_pin.lcd_dcx = lcd_dcx;
    lcd_pin.lcd_spi_cs = lcd_spi_cs;
    lcd_pin.lcd_spi_clk = lcd_spi_clk;
    lcd_pin.lcd_spi_mosi = lcd_spi_mosi;
}

void lcd_set_dma_ch_num(uint8_t num)
{
    lcd_spi_dma_ch_num = num;
}

void lcd_dcx_init(void)
{
    hal_gpio_init();
    hal_gpio_init_pin(lcd_pin.lcd_dcx, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT, GPIO_PULL_NONE);
    hal_gpio_set_level(lcd_pin.lcd_dcx, GPIO_LEVEL_HIGH);
}

void lcd_dcx_set(void)
{
    hal_gpio_set_level(lcd_pin.lcd_dcx, GPIO_LEVEL_HIGH);
}

void lcd_dcx_reset(void)
{
    hal_gpio_set_level(lcd_pin.lcd_dcx, GPIO_LEVEL_LOW);
}

void lcd_set_reset(bool reset)
{
    if (reset)
    {
        Pad_Config(lcd_pin.lcd_rst, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(lcd_pin.lcd_rst, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}

void rtk_lcd_hal_set_reset(bool reset)
{
    lcd_set_reset(reset);
}

/**
  * @brief  wristband driver init
  * @param  None
  * @retval None
  */
void lcd_driver_init(void)
{
    lcd_device_init();
    lcd_set_reset(true);
    platform_delay_ms(120);
    lcd_set_reset(false);
    platform_delay_ms(50);
}

static void lcd_cs_pin_init(void)
{
    hal_gpio_init();
    hal_gpio_init_pin(lcd_pin.lcd_spi_cs, GPIO_TYPE_AON, GPIO_DIR_OUTPUT, GPIO_PULL_NONE);
    hal_gpio_set_level(lcd_pin.lcd_spi_cs, GPIO_LEVEL_HIGH);
}

void lcd_pad_init(void)
{
    Pinmux_Config(lcd_pin.lcd_spi_clk, SPI_CLK);
    Pinmux_Config(lcd_pin.lcd_spi_mosi, SPI_MOSI);

    Pad_Config(lcd_pin.lcd_spi_clk, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(lcd_pin.lcd_spi_mosi, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);

    lcd_cs_pin_init();
    lcd_dcx_init();
#if (ENABLE_TE_FOR_LCD == 1)
    rtk_lcd_hal_set_TE_type(LCDC_TE_TYPE_SW_TE);
    lcd_te_device_init();
#endif
}

void lcd_cs_pin_no_active(void)
{
    hal_gpio_set_level(lcd_pin.lcd_spi_cs, GPIO_LEVEL_HIGH);
}

void lcd_cs_pin_active(void)
{
    hal_gpio_set_level(lcd_pin.lcd_spi_cs, GPIO_LEVEL_LOW);
}

/**
  * @brief  writband lcd device init set IO config here
  * @param  None
  * @retval None
  */
void lcd_device_init(void)
{
    lcd_pad_init();

    SPI_DeInit(LCD_SPI_BUS);
    RCC_PeriphClockCmd(APBPeriph_SPI, APBPeriph_SPI_CLOCK, ENABLE);
    RCC_SPIClkDivConfig(LCD_SPI_BUS, SPI_CLOCK_DIV_1);
    SPI_InitTypeDef  SPI_InitStructure;

    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction   = SPI_Direction_TxOnly;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 4;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_NDF         = 0;
    SPI_InitStructure.SPI_TxWaterlevel = 30;
    SPI_InitStructure.SPI_TxDmaEn = ENABLE;

    SPI_Init(LCD_SPI_BUS, &SPI_InitStructure);
    SPI_Cmd(LCD_SPI_BUS, ENABLE);
}

void rtk_lcd_hal_init(void)
{
    lcd_driver_init();
    sh8601a_init();
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    //APP_PRINT_TRACE2("rtk_lcd_hal_set_window w:%d, h:%d", w, h);
    lcd_SH8601A_spi_454_set_window(xStart, yStart, xStart + w - 1, yStart + h - 1);
}

static void lcd_dma_transfer_done(void)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
}

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_4;

    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_HANDSHAKE_SPI_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)(LCD_SPI_BUS->DR);
    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);

    uint32_t dma_max_block_size = 65535;
    uint32_t left_count = len % dma_max_block_size;
    uint32_t count = len / dma_max_block_size;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < count; i++)
    {
        GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(buf + offset));
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, dma_max_block_size);
        offset += (dma_max_block_size << 1);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);

        lcd_dma_transfer_done();
    }
    if (left_count != 0)
    {
        GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(buf + offset));
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, left_count);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
        lcd_dma_transfer_done();
    }
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY) ||
           SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_TFE) == false);
    lcd_cs_pin_no_active();
}

void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;

    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_HANDSHAKE_SPI_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    rtk_lcd_hal_set_window(xStart, yStart, w, h);

    static uint32_t color_buf = 0;
    color_buf = (color >> 8) | (color << 8);
    color_buf = color_buf | color_buf << 16;

    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)&color_buf);
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(LCD_SPI_BUS->DR));

    uint32_t section_hight = 10;
    uint32_t left_line = h % section_hight;

    for (uint32_t i = 0; i < h / section_hight; i++)
    {
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * section_hight));
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);

        lcd_dma_transfer_done();
    }
    if (left_line != 0)
    {
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * left_line));
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
        lcd_dma_transfer_done();
    }
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY) ||
           SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_TFE) == false);
    lcd_cs_pin_no_active();
}

void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
    //APP_PRINT_TRACE2("rtk_lcd_hal_start_transfer: buf 0x%p, len %d", buf, len);
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_4;

    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_HANDSHAKE_SPI_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len);

    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(LCD_SPI_BUS->DR));
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
}

void rtk_lcd_hal_transfer_done(void)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY) ||
           SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_TFE) == false);
    lcd_cs_pin_no_active();
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return LCD_WIDTH;
}

uint32_t rtk_lcd_hal_get_height(void)
{
    return LCD_HIGHT;
}

uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return RGB16BIT_565;
}
#endif
