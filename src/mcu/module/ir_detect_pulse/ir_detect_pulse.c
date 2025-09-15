/*********************************************************************************************************
*               Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     ir_detect_pulse.c
* @brief
* @details
* @author   colin
* @date     2025-03-27
* @version  v1.0
*********************************************************************************************************
*/

#include <string.h>
#include "ir_detect_pulse.h"
#include "vector_table.h"
#include "trace.h"
#include "os_mem.h"
#include "pm.h"

#include "rtl876x_pinmux.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"
#include "rtl876x_ir.h"

#define IR_SAMPLING_FREQUENCY       10000       //kHZ
#define IR_DETECT_MAX_IDLE_TIME     (0x7fffffff / IR_SAMPLING_FREQUENCY * 1000)   //us

static pIRDetectCB ir_detect_callback = NULL;
static bool ir_detect_allow_enter_dlps = true;
/* Buffer which store receiving data */
uint32_t *ir_rawdata = NULL;
IRDetectPulse_TypeDef *gpio_pulse = NULL;
/* Number of data which has been receoved */
uint32_t pulse_count = 0;
uint32_t ir_detect_pulse_max_cnt = 0;

static void ir_detect_pulse_clear_data(void)
{
    memset(ir_rawdata, 0, 4 * ir_detect_pulse_max_cnt);
    memset(gpio_pulse, 0, sizeof(IRDetectPulse_TypeDef) * ir_detect_pulse_max_cnt);
    pulse_count = 0;
}

static void ir_detect_pulse_convert_data_to_pulse(void)
{
    uint32_t cnt = 0;

    /* High/low information. */
    for (uint32_t i = 0; i < pulse_count; i++)
    {
        if (ir_rawdata[i] & BIT(31))
        {
            /* High level time cnt */
            cnt = (ir_rawdata[i] & 0x7FFFFFFF) + 1;
            gpio_pulse[i].time = cnt / 10;      // high_cnt * 1 / 10000000 * 1000000 us
            gpio_pulse[i].pulse = 1;
        }
        else
        {
            /* Low level time cnt */
            cnt = ir_rawdata[i] + 1;
            gpio_pulse[i].time = cnt / 10;      // high_cnt * 1 / 10000000 * 1000000 us
            gpio_pulse[i].pulse = 0;
        }
    }
}

static void ir_detect_pulse_update_data(void)
{
    uint16_t len = 0;
    uint32_t new_count = 0;

    len = IR_GetRxDataLen();

    if (len == 0)
    {
        return;
    }
    new_count = pulse_count + len;

    /* Coming here indicates that the initialization settings parameters are incorrect. */
    if (new_count > ir_detect_pulse_max_cnt)
    {
        IO_PRINT_ERROR1("ir_detect_pulse_handler: pulse overflow, data will be lost, total cnt %d",
                        new_count);
        if (pulse_count == ir_detect_pulse_max_cnt)
        {
            uint32_t temp_buf[64];
            IR_ReceiveBuf(temp_buf, len);
        }
        else
        {
            uint32_t temp_buf[64];
            IR_ReceiveBuf(ir_rawdata + pulse_count, ir_detect_pulse_max_cnt - pulse_count);
            IR_ReceiveBuf(temp_buf, new_count - ir_detect_pulse_max_cnt);
            pulse_count = ir_detect_pulse_max_cnt;
        }
    }
    else
    {
        IR_ReceiveBuf(ir_rawdata + pulse_count, len);
        pulse_count = new_count;
    }
}

static void ir_detect_pulse_handler(void)
{
    /* Receive by interrupt */
    if (IR_GetINTStatus(IR_INT_RF_LEVEL) == SET)
    {
        IR_MaskINTConfig(IR_INT_RF_LEVEL, ENABLE);
        ir_detect_pulse_update_data();
        IR_ClearINTPendingBit(IR_INT_RF_LEVEL_CLR);
        IR_MaskINTConfig(IR_INT_RF_LEVEL, DISABLE);
    }

    /* Stop to receive IR data */
    if (IR_GetINTStatus(IR_INT_RX_CNT_THR) == SET)
    {
        IR_MaskINTConfig(IR_INT_RX_CNT_THR, ENABLE);
        /* Read remaining data */
        ir_detect_pulse_update_data();
        IR_ClearINTPendingBit(IR_INT_RX_CNT_THR_CLR);
        IR_MaskINTConfig(IR_INT_RX_CNT_THR, DISABLE);
        IR_Cmd(IR_MODE_RX, DISABLE);

        ir_detect_pulse_convert_data_to_pulse();
        ir_detect_callback(gpio_pulse, pulse_count);
    }
}

void ir_detect_pulse_register_callback(pIRDetectCB cb)
{
    ir_detect_callback = cb;
}

void ir_detect_pulse_unregister_callback(void)
{
    ir_detect_callback = NULL;
}

static bool ir_detect_dlps_check(void)
{
    return ir_detect_allow_enter_dlps;
}

void ir_detect_pulse_stop(void)
{
    IR_Cmd(IR_MODE_RX, DISABLE);
    IR_ClearRxFIFO();
    IR_MaskINTConfig(IR_INT_RF_LEVEL | IR_INT_RX_CNT_THR, ENABLE);
    IR_INTConfig(IR_INT_RF_LEVEL | IR_INT_RX_CNT_THR, DISABLE);

    /* Enable DLPS after IR detection completes. */
    ir_detect_allow_enter_dlps = true;
}

void ir_detect_pulse_start(void)
{
    /* Disable DLPS when IR detection is activated. */
    ir_detect_allow_enter_dlps = false;

    ir_detect_pulse_clear_data();
    IR_INTConfig(IR_INT_RF_LEVEL | IR_INT_RX_CNT_THR, ENABLE);
    IR_MaskINTConfig(IR_INT_RF_LEVEL | IR_INT_RX_CNT_THR, DISABLE);
    IR_ClearRxFIFO();
    IR_Cmd(IR_MODE_RX, ENABLE);
}

bool ir_detect_pulse_init(uint8_t pin, uint32_t max_pulse_cnt, IRDetectEdgeType_TypeDef type,
                          IRDetectIdleLevel_TypeDef idle_level, uint32_t idle_time_us)
{
    pulse_count = 0;
    ir_detect_pulse_max_cnt = max_pulse_cnt;

    ir_rawdata = (uint32_t *)os_mem_zalloc(RAM_TYPE_DATA_ON, 4 * ir_detect_pulse_max_cnt);

    if (ir_rawdata == NULL)
    {
        IO_PRINT_ERROR0("ir_detect_pulse_init: malloc ir_rawdata failed");
        return false;
    }

    gpio_pulse = (IRDetectPulse_TypeDef *)os_mem_zalloc(RAM_TYPE_DATA_ON,
                                                        sizeof(IRDetectPulse_TypeDef) * ir_detect_pulse_max_cnt);
    if (gpio_pulse == NULL)
    {
        IO_PRINT_ERROR0("ir_detect_pulse_init: malloc gpio_pulse failed");
        return false;
    }

    if (idle_time_us > IR_DETECT_MAX_IDLE_TIME)
    {
        idle_time_us = IR_DETECT_MAX_IDLE_TIME;
    }

    Pinmux_Deinit(pin);
    Pad_Config(pin, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pinmux_Config(pin, IRDA_RX);

    RCC_PeriphClockCmd(APBPeriph_IR, APBPeriph_IR_CLOCK, ENABLE);

    /* Initialize IR */
    IR_InitTypeDef IR_InitStruct;
    IR_StructInit(&IR_InitStruct);

    IR_InitStruct.IR_Freq               = IR_SAMPLING_FREQUENCY;
    IR_InitStruct.IR_Mode               = IR_MODE_RX;
    IR_InitStruct.IR_RxStartMode        = IR_RX_AUTO_MODE;
    IR_InitStruct.IR_RxFIFOThrLevel     = 2;
    IR_InitStruct.IR_RxFIFOFullCtrl     = IR_RX_FIFO_FULL_DISCARD_NEWEST;
    IR_InitStruct.IR_RxTriggerMode      = ((uint32_t)((type) << IR_TRIGGER_MODE_Pos));
    IR_InitStruct.IR_RxFilterTime       = IR_RX_FILTER_TIME_200ns;
    IR_InitStruct.IR_RxCntThrType       = ((uint32_t)((idle_level) << IR_RX_CNT_THR_TYPE_Pos));
    IR_InitStruct.IR_RxCntThr           = idle_time_us * 10;   //1 / 10000000 * cnt * 1000000 us

    IR_Init(&IR_InitStruct);

    RamVectorTableUpdate(IR_VECTORn, (IRQ_Fun)ir_detect_pulse_handler);

    /* Configure NVIC */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = IR_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    power_check_cb_register(ir_detect_dlps_check);

    return true;
}
