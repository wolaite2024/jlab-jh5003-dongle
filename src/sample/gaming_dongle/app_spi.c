/*
 *  Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "app_spi.h"
#include "rtl876x_rcc.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "app_cyclic_buffer.h"
#include "compiler_abstraction.h"
#include "section.h"

#ifdef ENABLE_SPI0_SLAVE

#define TIME_CALCULATE_START(x)                         \
    static uint32_t stat_count = 0;             \
    static uint32_t stat_accum = 0;             \
    uint32_t tmp_start = 0;                     \
    uint32_t tmp_end = 0;                       \
    extern uint32_t (*log_timestamp_get)(void); \
    tmp_start = log_timestamp_get();            \
    if (!stat_count)                            \
    {                                           \
        stat_accum = 0;                         \
    }

/* ident: "func_name: time consumption %u" */
#define TIME_CALCULATE_END(ident)                    \
    tmp_end = log_timestamp_get();           \
    if (tmp_end >= tmp_start)                \
    {                                        \
        stat_accum += (tmp_end - tmp_start); \
    }                                        \
    else                                     \
    {                                        \
        stat_count = 0;                      \
        return;                              \
    }                                        \
    stat_count++;                            \
    if (stat_count == 1000)                  \
    {                                        \
        stat_accum /= stat_count;            \
        APP_PRINT_INFO1(ident, stat_accum);  \
        stat_count = 0;                      \
    }

#define SPI_HID_DATA_MAX_LEN    5
#define SPI_HID_DATA_LEN        5
#define SPI_PKT_START   0
#define SPI_PKT_DATA    1
#define SPI_READ_MOUSE_DATA_FLAG    0x7F

/* For slave role, clk rate has no meaning. */
#define     CLCK_RATE              (40000000)

#define     SPI0_CS         P0_0
#define     SPI0_SCK        P0_1
#define     SPI0_MOSI       P2_1
#define     SPI0_MISO       P2_2

T_CYCLIC_BUF spi_read_rbuf;

struct spi_hid
{
    uint8_t state;
    uint16_t pos;
    uint16_t pkt_len;
    uint8_t hdr[1];
    uint8_t buf[SPI_HID_DATA_MAX_LEN];
};
struct spi_hid shid;

extern bool usb_hid_interrupt_in_high_duty(uint8_t *data, uint8_t len);

__STATIC_ALWAYS_INLINE void rx_complete_pkt(uint8_t *data, uint16_t len)
{
    /* APP_PRINT_INFO2("rx_complete_pkt: type %02x, len %u", data[0], len); */
    data[0] = 3; /* app_usb_hid.h REPORT_ID_MOUSE_INPUT */
    usb_hid_interrupt_in_high_duty(data, len);
}

RAM_TEXT_SECTION static void spi_data_parser(uint8_t *data, uint16_t len)
{
    uint16_t i = 0;
    uint16_t tlen = 0;

    if (!len)
    {
        return;
    }
    while (len > 0)
    {
        switch (shid.state)
        {
        case SPI_PKT_START:
            if (data[i] == SPI_READ_MOUSE_DATA_FLAG)
            {
                shid.buf[shid.pos] = data[i];
                shid.pkt_len = SPI_HID_DATA_LEN;
                shid.state = PKT_DATA;
                shid.pos++;
                i++;
                len--;
            }
            else
            {
                APP_PRINT_ERROR2("spi_data_parser: Unknown pkt type %u, i %u",
                                 data[i], i);
                i++;
                len--;
                continue;
            }
            break;
        case SPI_PKT_DATA:
            if (shid.pos + len < shid.pkt_len)
            {
                tlen = len;
            }
            else
            {
                tlen = shid.pkt_len - shid.pos;
            }
            memcpy(shid.buf + shid.pos, data + i, tlen);
            len -= tlen;
            shid.pos += tlen;
            i += tlen;
            if (shid.pos == shid.pkt_len)
            {
                /* Process the complete pkt. */
                rx_complete_pkt(shid.buf, shid.pos);
                shid.pos = 0;
                shid.state = SPI_PKT_START;
            }
            break;
        default:
            APP_PRINT_ERROR1("spi_data_parser: Unknown state %u", shid.state);
            break;
        }
    }
}

static void RCC_Configuration(void)
{
    /* Turn on SPI clock */
    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
}

static void SPI_PINMUXConfiguration(void)
{

    /* Pinmux_Config(SPI0_SCK, SPI0_CLK_MASTER);
     * Pinmux_Config(SPI0_MOSI, SPI0_MO_MASTER);
     * Pinmux_Config(SPI0_MISO, SPI0_MI_MASTER);
     * Pinmux_Config(SPI0_CS, SPI0_SS_N_0_MASTER);
     */

    Pinmux_Config(SPI0_SCK, SPI0_CLK_SLAVE);
    Pinmux_Config(SPI0_MOSI, SPI0_SI_SLAVE);
    Pinmux_Config(SPI0_MISO, SPI0_SO_SLAVE);
    Pinmux_Config(SPI0_CS, SPI0_SS_N_0_SLAVE);
}

static void SPI_PADConfiguration(void)
{
    Pad_Config(SPI0_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
}

static void SPI_Configuration(uint32_t baudrate)
{
    SPI_InitTypeDef  SPI_InitStructure;
    SPI_StructInit(&SPI_InitStructure);


    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    /* FIXME: We are in slave mode. */
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Slave;
    /* SPI_InitStructure.SPI_Mode        = SPI_Mode_Master; */
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High; /* high in IDLE */
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_2Edge;
    /* For slave role, clk rate has no meaning. */
    SPI_InitStructure.SPI_BaudRatePrescaler  = CLCK_RATE / baudrate;

    /* Trigger SPI_INT_RXF interrupt if data length in receive FIFO  >=
     * SPI_RxThresholdLevel + 1
     * */
    SPI_InitStructure.SPI_RxThresholdLevel = 4;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola; /* SPI */

    SPI_Init(SPI0, &SPI_InitStructure);
    SPI_Cmd(SPI0, ENABLE);

    /* Enable RXF interrupt */
    SPI_INTConfig(SPI0, SPI_INT_RXF, ENABLE);
    /* Config SPI interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = SPI0_IRQn;
    /* Let the irq prio is lower than USB isoc irq, because we are gaming
     * dongle. USB audio transfer has the highest priority.
     * */
    NVIC_InitStruct.NVIC_IRQChannelPriority = 4;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

void app_spi_init(uint32_t baudrate)
{
    //p_read_cback = cback;
    //cyclic_buf_init(&spi_read_rbuf, SPI_READ_RBUF_LEN);
    SPI_DeInit(SPI0);
    RCC_Configuration();
    SPI_PINMUXConfiguration();
    SPI_PADConfiguration();
    SPI_Configuration(baudrate);
}

bool spi_send_msg(uint16_t len)
{
#if 0
    uint8_t  event;
    T_IO_MSG msg;

    if (len > SPI_PKG_LEN_MAX)
    {
        APP_PRINT_ERROR1("spi_send_msg pkg len %d too big", len);
        return false;
    }

    event = EVENT_IO_TO_APP;
    msg.type    = IO_MSG_TYPE_SPI;
    msg.subtype = len;
    if (os_msg_send(audio_evt_queue_handle, &event, 0) == true)
    {
        return os_msg_send(audio_io_queue_handle, &msg, 0);
    }
#endif

    return false;
}

RAM_TEXT_SECTION void SPI0_Handler(void)
{
    uint8_t len = 0;
    uint8_t i = 0;
    uint8_t tbuf[16]; /* FIXME: the real fifo depth */

    if (SPI_GetINTStatus(SPI0, SPI_INT_RXF) == SET)
    {
        /* while(SPI_GetFlagState(SPI0, SPI_FLAG_RFNE) == RESET); */
        /* Disable interrupt first */
        SPI_INTConfig(SPI0, SPI_INT_RXF, DISABLE);

        TIME_CALCULATE_START(1)
        /* Read FIFO will clear SPI_INT_RXF.
         * Read all data in receive FIFO, otherwise SPI_INT_RXF interrupt
         * happens again
         * */
        len = SPI_GetRxFIFOLen(SPI0);
        if (len > sizeof(tbuf))
        {
            len = sizeof(tbuf);
        }
        for (i = 0; i < len; i++)
        {
            tbuf[i] = SPI_ReceiveData(SPI0);
        }

#ifdef APP_SPI_DEBUG
        APP_PRINT_INFO2("SPI0_Handler %b, len %d", TRACE_BINARY(i, tbuf), i);
#endif

        spi_data_parser(tbuf, i);
        TIME_CALCULATE_END("SPI0_Handler: time consumption %u");

        /* Enable interrupt */
        SPI_INTConfig(SPI0, SPI_INT_RXF, ENABLE);
    }
}

#endif /* ENABLE_SPI0_SLAVE */
