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
#if (LCD_INTERFACE == LCD_INTERFACE_QSPI)
#include "trace.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "module_lcd_qspi.h"
#include "platform_utils.h"
#include "fmc_api.h"
#include "fmc_api_ext.h"
#include "rtk_hal_lcd.h"
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST7789)
#include "lcd_st7789_240_spi.h"
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_RM69330)
#include "lcd_rm69330_454_qspi.h"
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
#include "lcd_sh8601z_454_qspi.h"
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
#include "lcd_st77916_320_385_qspi.h"
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
#include "lcd_st7801_368_448_qspi.h"
#endif

#define LCD_SPIC_CLK                     P9_4
#define LCD_SPIC_CSN                     P9_2
#define LCD_SPIC_SIO0                    P9_3
#define LCD_SPIC_SIO1                    P9_1
#define LCD_SPIC_SIO2                    P9_0
#define LCD_SPIC_SIO3                    P9_5
#define SPIC_ID                          FMC_SPIC_ID_2
#define SPIC_GDMA_TX_HANDSHAKE           GDMA_Handshake_SPIC2_TX
#define SPIC_GDMA_DESTINATION_ADDR       ((uint32_t)(&(SPIC2->DR[0].WORD)))

static uint8_t lcd_qspi_dma_ch_num = 0xa5;

#define LCD_DMA_CHANNEL_NUM              lcd_qspi_dma_ch_num
#define LCD_DMA_CHANNEL_INDEX            DMA_CH_BASE(lcd_qspi_dma_ch_num)
#define LCD_DMA_CHANNEL_IRQ              DMA_CH_IRQ(lcd_qspi_dma_ch_num)

static uint8_t lcd_rst_pin;

void lcd_pin_config(uint8_t lcd_rst)
{
    lcd_rst_pin = lcd_rst;
}

void lcd_set_dma_ch_num(uint8_t num)
{
    lcd_qspi_dma_ch_num = num;
}

void lcd_set_reset(bool reset)
{
    if (reset)
    {
        Pad_Config(lcd_rst_pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(lcd_rst_pin, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}

void rtk_lcd_hal_set_reset(bool reset)
{
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
    lcd_set_backlight(!reset);
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
    lcd_set_vci_en(false);
#endif
    lcd_set_reset(reset);
#if (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
    lcd_set_avdd_en(false);
#endif
}

/**
  * @brief  wristband driver init
  * @param  None
  * @retval None
  */
void lcd_driver_init(void)
{
    lcd_device_init();
#if (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
    lcd_set_avdd_en(true);
#endif
    lcd_set_reset(true);
    platform_delay_ms(120);
    lcd_set_reset(false);
    platform_delay_ms(50);
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
    lcd_set_vci_en(true);
#endif
}

void lcd_pad_init(void)
{
    Pinmux_Config(LCD_SPIC_SIO2, IDLE_MODE); //SIO2
    Pinmux_Config(LCD_SPIC_SIO1, IDLE_MODE); //SIO1
    Pinmux_Config(LCD_SPIC_CSN, IDLE_MODE); //CS
    Pinmux_Config(LCD_SPIC_SIO0, IDLE_MODE); //SIO0
    Pinmux_Config(LCD_SPIC_CLK, IDLE_MODE); //CLK
    Pinmux_Config(LCD_SPIC_SIO3, IDLE_MODE); //SIO3

    Pad_Config(LCD_SPIC_SIO2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(LCD_SPIC_SIO1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(LCD_SPIC_CSN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(LCD_SPIC_SIO0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(LCD_SPIC_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(LCD_SPIC_SIO3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
#if (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
    lcd_backlight_init();
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
    lcd_vci_en_init();
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
    lcd_avdd_en_init();
#endif
#if (ENABLE_TE_FOR_LCD == 1)
    rtk_lcd_hal_set_TE_type(LCDC_TE_TYPE_SW_TE);
    lcd_te_device_init();
#endif
}

/**
  * @brief  writband lcd device init set IO config here
  * @param  None
  * @retval None
  */
void lcd_device_init(void)
{
    lcd_pad_init();
    fmc_spic_init(SPIC_ID);
    uint32_t spic2_freq = 0;
    fmc_spic_clock_switch(SPIC_ID, 100, &spic2_freq);
    fmc_spic_set_baud(SPIC_ID, 1);
}

void rtk_lcd_hal_init(void)
{
    lcd_driver_init();
#if (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
    sh8601a_init();
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
    st77916_init();
    lcd_set_backlight(true);
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
    st7801_init();
#endif
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    //APP_PRINT_TRACE2("rtk_lcd_hal_set_window w:%d, h:%d", w, h);
#if (TARGET_LCD_DEVICE == LCD_DEVICE_SH8601Z)
    lcd_SH8601A_qspi_454_set_window(xStart, yStart, xStart + w - 1, yStart + h - 1);
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST77916)
    lcd_ST77916_qspi_454_set_window(xStart, yStart, xStart + w - 1, yStart + h - 1);
#elif (TARGET_LCD_DEVICE == LCD_DEVICE_ST7801)
    lcd_ST7801_qspi_368_448_set_window(xStart, yStart, xStart + w - 1, yStart + h - 1);
#endif
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

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;

    GDMA_InitStruct.GDMA_DestHandshake       = SPIC_GDMA_TX_HANDSHAKE;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_DestinationAddr     = SPIC_GDMA_DESTINATION_ADDR;
    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);

    len = len >> 1;
    uint32_t dma_max_block_size = 65535;
    uint32_t left_count = len % dma_max_block_size;
    uint32_t count = len / dma_max_block_size;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < count; i++)
    {
        GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(buf + offset));
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, dma_max_block_size);
        offset += (dma_max_block_size << 2);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);

        rtk_lcd_hal_transfer_done();
    }
    if (left_count != 0)
    {
        GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(buf + offset));
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, left_count);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
        rtk_lcd_hal_transfer_done();
    }
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

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;

    GDMA_InitStruct.GDMA_DestHandshake       = SPIC_GDMA_TX_HANDSHAKE;
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
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, SPIC_GDMA_DESTINATION_ADDR);

    uint32_t section_hight = 10;
    uint32_t left_line = h % section_hight;

    for (uint32_t i = 0; i < h / section_hight; i++)
    {
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * section_hight * 2) >> 2);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);

        rtk_lcd_hal_transfer_done();
    }
    if (left_line != 0)
    {
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * left_line * 2) >> 2);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
        rtk_lcd_hal_transfer_done();
    }
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

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_4;

    GDMA_InitStruct.GDMA_DestHandshake       = SPIC_GDMA_TX_HANDSHAKE;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len >> 1);

    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, SPIC_GDMA_DESTINATION_ADDR);
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
}

void rtk_lcd_hal_transfer_done(void)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
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


void rtk_hal_lcd_fill(uint8_t *addr)
{
    uint32_t section_hight = 50;
    uint32_t left_line = (LCD_HIGHT - 1) % section_hight;
    uint32_t offset = 0;
    uint32_t secton_count = 0;
    uint32_t transfer_len_16 = 0;

    rtk_lcd_hal_set_window(0, 0,  LCD_WIDTH, section_hight);
    transfer_len_16 = LCD_WIDTH * section_hight;
    rtk_lcd_hal_start_transfer((uint8_t *)addr, transfer_len_16);
    offset += transfer_len_16 * 2;

    for (secton_count = 1; secton_count < (LCD_HIGHT - 1) / section_hight; secton_count++)
    {
        rtk_lcd_hal_transfer_done();
        rtk_lcd_hal_set_window(0, secton_count * section_hight, LCD_WIDTH, section_hight);
        rtk_lcd_hal_start_transfer((uint8_t *)(addr + offset), transfer_len_16);
        offset += (transfer_len_16 * 2);
    }
    if (left_line != 0)
    {
        rtk_lcd_hal_transfer_done();
        rtk_lcd_hal_set_window(0, secton_count * section_hight, LCD_WIDTH, left_line);
        rtk_lcd_hal_start_transfer((uint8_t *)(addr + offset), left_line * LCD_WIDTH);
        rtk_lcd_hal_transfer_done();
    }
}

void rtk_lcd_hal_start_transfer_test(uint8_t *buf, uint16_t xStart, uint16_t yStart, uint16_t w,
                                     uint16_t h)
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

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;

    GDMA_InitStruct.GDMA_DestHandshake       = SPIC_GDMA_TX_HANDSHAKE;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, SPIC_GDMA_DESTINATION_ADDR);
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);

    rtk_lcd_hal_set_window(0, 0, w, h);

    uint32_t section_hight = 10;
    uint32_t left_line = h % section_hight;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < h / section_hight; i++)
    {
        GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(buf + offset));
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * section_hight * 2) >> 2);
        offset += (w * section_hight * 2);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);

        rtk_lcd_hal_transfer_done();
    }
    if (left_line != 0)
    {
        GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(buf + offset));
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * left_line * 2) >> 2);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
        rtk_lcd_hal_transfer_done();
    }
}
#endif
