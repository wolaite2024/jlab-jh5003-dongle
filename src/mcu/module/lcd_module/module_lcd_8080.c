#include "app_gui.h"
#if (LCD_INTERFACE == LCD_INTERFACE_8080)
#include "trace.h"
#include "rtl876x_rcc.h"
#include "rtl876x_if8080.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "module_lcd_8080.h"
#include "platform_utils.h"
#if (TARGET_LCD_DEVICE == LCD_DEVICE_NT35110)
#include "lcd_nt35510_480_8080.h"
#endif
#include "rtk_hal_lcd.h"

static uint8_t lcd_8080_dma_ch_num = 0xa5;

#define LCD_DMA_CHANNEL_NUM              lcd_8080_dma_ch_num
#define LCD_DMA_CHANNEL_INDEX            DMA_CH_BASE(lcd_8080_dma_ch_num)
#define LCD_DMA_CHANNEL_IRQ              DMA_CH_IRQ(lcd_8080_dma_ch_num)

#define LCD_8080_D0                      P2_6
#define LCD_8080_D1                      P2_7
#define LCD_8080_D2                      P4_0
#define LCD_8080_D3                      P4_1
#define LCD_8080_D4                      P4_2
#define LCD_8080_D5                      P4_3
#define LCD_8080_D6                      P4_4
#define LCD_8080_D7                      P4_5

#define LCD_8080_CS                      P2_3
#define LCD_8080_DCX                     P2_4

#if (CONFIG_REALTEK_TARGET_RTL8763EWE_VP == 1)
#define LCD_8080_RD                      P9_0
#else
#define LCD_8080_RD                      P2_1
#endif
#define LCD_8080_WR                      P2_5

typedef struct _t_lcd_pin
{
    uint8_t lcd_rst;
    uint8_t lcd_power;
    uint8_t lcd_bl;
} T_LCD_PIN;

T_LCD_PIN lcd_pin;

static GDMA_LLIDef GDMA_LLIStruct_LAST;

void lcd_pin_config(uint8_t lcd_rst, uint8_t lcd_power, uint8_t lcd_bl)
{
    lcd_pin.lcd_rst = lcd_rst;
    lcd_pin.lcd_power = lcd_power;
    lcd_pin.lcd_bl = lcd_bl;
}

void lcd_set_dma_ch_num(uint8_t num)
{
    lcd_8080_dma_ch_num = num;
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
    lcd_set_backlight(!reset);
    lcd_set_reset(reset);
}

void lcd_pad_init(void)
{
    /* Power for LDO of LCD */
    Pad_Config(lcd_pin.lcd_power, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pad_Config(LCD_8080_D0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D4, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D5, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D6, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D7, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    /* CS */
    Pad_Config(LCD_8080_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    /* DCX */
    Pad_Config(LCD_8080_DCX, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    /* RD */
//    Pad_Config(LCD_8080_RD, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    /* WR */
    Pad_Config(LCD_8080_WR, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    /* 8080 interface: D0~D7 */
    Pinmux_Config(LCD_8080_D0, IDLE_MODE);
    Pinmux_Config(LCD_8080_D1, IDLE_MODE);
    Pinmux_Config(LCD_8080_D2, IDLE_MODE);
    Pinmux_Config(LCD_8080_D3, IDLE_MODE);
    Pinmux_Config(LCD_8080_D4, IDLE_MODE);
    Pinmux_Config(LCD_8080_D5, IDLE_MODE);
    Pinmux_Config(LCD_8080_D6, IDLE_MODE);
    Pinmux_Config(LCD_8080_D7, IDLE_MODE);

    /* CS */
    Pinmux_Config(LCD_8080_CS, IDLE_MODE);
    /* DCX */
    Pinmux_Config(LCD_8080_DCX, IDLE_MODE);
    /* RD */
//    Pinmux_Config(LCD_8080_RD, IDLE_MODE);
    /* WR */
    Pinmux_Config(LCD_8080_WR, IDLE_MODE);

    /*BL AND RESET ARE NOT FIX*/
    Pad_Config(lcd_pin.lcd_bl, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);

}

/**
  * @brief  writband lcd device init set IO config here
  * @param  None
  * @retval None
  */
void lcd_device_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_IF8080, APBPeriph_IF8080_CLOCK, DISABLE);
    RCC_PeriphClockCmd(APBPeriph_IF8080, APBPeriph_IF8080_CLOCK, ENABLE);

    IF8080_PinGroupConfig();
    IF8080_InitTypeDef IF8080_InitStruct;
    IF8080_StructInit(&IF8080_InitStruct);
    extern uint32_t SystemCpuClock;
    if (SystemCpuClock == 100000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_5;
    }
    else if (SystemCpuClock == 90000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_5;
    }
    else if (SystemCpuClock == 80000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_4;
    }
    else if (SystemCpuClock == 40000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_2;
    }
    IF8080_InitStruct.IF8080_Mode              = IF8080_MODE_MANUAL;
    IF8080_InitStruct.IF8080_AutoModeDirection = IF8080_Auto_Mode_Direction_WRITE;
    IF8080_InitStruct.IF8080_GuardTimeCmd      = IF8080_GUARD_TIME_DISABLE;
    IF8080_InitStruct.IF8080_GuardTime         = IF8080_GUARD_TIME_2T;
    IF8080_InitStruct.IF8080_8BitSwap          = IF8080_8BitSwap_DISABLE;
    IF8080_InitStruct.IF8080_16BitSwap         = IF8080_16BitSwap_DISABLE;
    IF8080_InitStruct.IF8080_TxThr             = 10;
    IF8080_InitStruct.IF8080_TxDMACmd          = IF8080_TX_DMA_ENABLE;
    IF8080_InitStruct.IF8080_VsyncCmd          = IF8080_VSYNC_DISABLE;
    IF8080_InitStruct.IF8080_VsyncPolarity     = IF8080_VSYNC_POLARITY_FALLING;
    IF8080_Init(&IF8080_InitStruct);
    lcd_pad_init();
}

void lcd_driver_init(void)
{
    lcd_device_init();
    lcd_set_reset(false);
    platform_delay_ms(100);
    lcd_set_reset(true);
    platform_delay_ms(50);
    lcd_set_reset(false);
    platform_delay_ms(50);
}
/**
  * @brief  lcd init
  * @param  None
  * @retval None
  */
void rtk_lcd_hal_init(void)
{
    lcd_driver_init();
    lcd_nt35510_init();
    lcd_nt35510_power_on();
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
#if (TARGET_LCD_DEVICE == LCD_DEVICE_NT35110)
    lcd_nt35510_set_window(xStart, yStart, xStart + w - 1, yStart + h - 1);
#endif
}

void lcd_set_backlight(uint32_t percent)
{
    if (percent)
    {
        Pad_Config(lcd_pin.lcd_bl, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE,
                   PAD_OUT_HIGH);
    }
    else
    {
        Pad_Config(lcd_pin.lcd_bl, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    return;
}

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
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

    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_I8080;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)((uint32_t)(&(IF8080->FIFO))));
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

    IF8080_SwitchMode(IF8080_MODE_MANUAL);
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

    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_I8080;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    rtk_lcd_hal_set_window(xStart, yStart, w, h);

    static uint32_t color_buf = 0;
    color_buf = (color >> 8) | (color << 8);
    color_buf = color_buf | color_buf << 16;

    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)&color_buf);
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)((uint32_t)(&(IF8080->FIFO))));

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

    IF8080_SwitchMode(IF8080_MODE_MANUAL);
}

void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
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

    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_I8080;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len >> 1);     // len: unit -> 2 bytes
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)((uint32_t)(&(IF8080->FIFO))));
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
    IF8080_SwitchMode(IF8080_MODE_MANUAL);
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

    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_I8080;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    rtk_lcd_hal_set_window(xStart, yStart, w, h);

    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)((uint32_t)(&(IF8080->FIFO))));
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);

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
    IF8080_SwitchMode(IF8080_MODE_MANUAL);
}
#endif
