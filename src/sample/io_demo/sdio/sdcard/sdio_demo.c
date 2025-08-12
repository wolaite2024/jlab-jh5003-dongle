/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     sdio_demo.c
* @brief    This file provides SDcard demo code.
* @details
* @author   justin kang
* @date     2022-03-24
* @version  v1.0
*********************************************************************************************************
*/

/* Includes -----------------------------------------------------------------*/
#include <string.h>
#include "trace.h"
#include "sd.h"
#include "os_mem.h"
#include "rtl876x.h"

/** @defgroup  SDIO_DEMO_SDIO  SDIO DEMO
    * @brief
    * @{
    */


/* Declaration --------------------------------------------------------------*/

/* Defines ------------------------------------------------------------------*/
#define SINGLE_BLOCK_SIZE       ((uint32_t)512)
#define BLOCK_NUM               ((uint32_t)1)
#define OPER_SD_CARD_ADDR       ((uint32_t)0x8000)
#define BLOCK_DATA_SIZE         (SINGLE_BLOCK_SIZE * BLOCK_NUM)

static const T_SD_CONFIG    sd_card_cfg =
{
    .sd_if_type = SD_IF_SD_CARD,
    .sdh_group = GROUP_0,
    .sdh_bus_width = SD_BUS_WIDTH_4B,
    .sd_bus_clk_sel = SD_BUS_CLK_20M,
#if defined (TARGET_RTL87X3E)
    .sd_power_en = 1,
    .sd_power_high_active = 1,
    .sd_power_pin = P5_6
#endif
};

/* Globals ------------------------------------------------------------------*/
static uint8_t *test_buf = NULL;

void sdio_demo(void)
{
    IO_PRINT_TRACE0("sdio_demo: enter");

    uint32_t sd_status = 0;

    if (test_buf == NULL)
    {
        test_buf = os_mem_alloc(RAM_TYPE_BUFFER_ON, BLOCK_DATA_SIZE);
        if (test_buf == NULL)
        {
            return ;
        }
    }

    sd_config_init((T_SD_CONFIG *)&sd_card_cfg);
    sd_board_init();
    sd_card_init();

    memset(test_buf, 0, BLOCK_DATA_SIZE);
    sd_status = sd_read(OPER_SD_CARD_ADDR, (uint32_t)test_buf, SINGLE_BLOCK_SIZE, BLOCK_NUM);

    if (sd_status != 0)
    {
        return ;
    }
    sd_print_binary_data(test_buf, BLOCK_DATA_SIZE);

    for (uint32_t i = 0; i < BLOCK_DATA_SIZE; i++)
    {
        test_buf[i] = i & 0xff;
    }

    sd_print_binary_data(test_buf, BLOCK_DATA_SIZE);
    sd_status = sd_write(OPER_SD_CARD_ADDR, (uint32_t)test_buf, SINGLE_BLOCK_SIZE, BLOCK_NUM);

    if (sd_status != 0)
    {
        return ;
    }

    memset(test_buf, 0, BLOCK_DATA_SIZE);

    sd_status = sd_read(OPER_SD_CARD_ADDR, (uint32_t)test_buf, SINGLE_BLOCK_SIZE, BLOCK_NUM);

    if (sd_status != 0)
    {
        return ;
    }
    sd_print_binary_data(test_buf, BLOCK_DATA_SIZE);

}

/** @} */ /* End of group SDIO_DEMO_SDIO */

/******************* (C) COPYRIGHT 2021 Realtek Semiconductor Corporation *****END OF FILE****/

