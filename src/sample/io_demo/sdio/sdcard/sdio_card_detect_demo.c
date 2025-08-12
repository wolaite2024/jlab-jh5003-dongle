/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     sdio_card_detect_demo.c
* @brief    This file provides SD card detect demo code.
* @details
* @author   qinyuan_hu
* @date     2023-12-7
* @version  v1.0
*********************************************************************************************************
*/

/* Includes -----------------------------------------------------------------*/
#include <string.h>
#include "trace.h"
#include "sd.h"
#include "os_mem.h"
#include "rtl876x.h"
#include "os_task.h"
#include "os_msg.h"
#include "hal_gpio.h"
#include "hal_gpio_int.h"

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

#define SD_CARD_DETECT_PIN           P0_3
#define SD_EVENT_QUEUE_SIZE          0x10
#define SD_EVENT_CARD_INSERT         0x01
#define SD_EVENT_CARD_REMOVE         0x02

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
static void *sd_evt_queue_handle;
static void *sd_task_handle;

static void gpio_isr_cb(uint32_t context)
{
    uint8_t event = 0;
    uint8_t pin_index = (uint32_t)context;

    T_GPIO_LEVEL gpio_level = hal_gpio_get_input_level(pin_index);

    if (gpio_level == GPIO_LEVEL_LOW)
    {
        event = SD_EVENT_CARD_INSERT;
        IO_PRINT_INFO0("gpio_isr_cb: card insert!");
        hal_gpio_irq_change_polarity(pin_index, GPIO_IRQ_ACTIVE_HIGH);

        if (os_msg_send(sd_evt_queue_handle, &event, 0) == false)
        {
            IO_PRINT_ERROR0("gpio_isr_cb: send card insert msg fail");
        }
    }
    else
    {
        event = SD_EVENT_CARD_REMOVE;
        IO_PRINT_INFO0("gpio_isr_cb: card remove!");
        hal_gpio_irq_change_polarity(pin_index, GPIO_IRQ_ACTIVE_LOW);

        if (os_msg_send(sd_evt_queue_handle, &event, 0) == false)
        {
            IO_PRINT_ERROR0("gpio_isr_cb: send card remove msg fail");
        }
    }
}

static void sd_card_read(void)
{
    uint32_t sd_status = 0;

    memset(test_buf, 0, BLOCK_DATA_SIZE);

    sd_status = sd_read(OPER_SD_CARD_ADDR, (uint32_t)test_buf, SINGLE_BLOCK_SIZE, BLOCK_NUM);

    if (sd_status != 0)
    {
        return ;
    }
    sd_print_binary_data(test_buf, BLOCK_DATA_SIZE);
}

static void sd_cd_test_task(void *param)
{
    uint8_t event;

    while (true)
    {
        if (os_msg_recv(sd_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == SD_EVENT_CARD_INSERT)
            {
                sd_init();
                sd_card_read();
            }
            else if (event == SD_EVENT_CARD_REMOVE)
            {
                sd_deinit();
            }
        }
    }
}

static void sd_card_detect_init(void)
{
    os_task_create(&sd_task_handle, "sd_task", sd_cd_test_task, NULL, 384 * 4, 2);
    os_msg_queue_create(&sd_evt_queue_handle, "sd evtQ", SD_EVENT_QUEUE_SIZE, sizeof(uint8_t));

    hal_gpio_init();
    hal_gpio_int_init();
    hal_gpio_set_debounce_time(20);

    hal_gpio_init_pin(SD_CARD_DETECT_PIN, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_UP);
    hal_gpio_set_up_irq(SD_CARD_DETECT_PIN, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true);
    hal_gpio_register_isr_callback(SD_CARD_DETECT_PIN, gpio_isr_cb, SD_CARD_DETECT_PIN);
    hal_gpio_irq_enable(SD_CARD_DETECT_PIN);
}

void sdio_card_detect_demo(void)
{
    IO_PRINT_TRACE0("sdio_card_detect_demo: enter");

    if (test_buf == NULL)
    {
        test_buf = os_mem_alloc(RAM_TYPE_BUFFER_ON, BLOCK_DATA_SIZE);
        if (test_buf == NULL)
        {
            return ;
        }
    }

    sd_config_init((T_SD_CONFIG *)&sd_card_cfg);
    sd_card_detect_init();
}

/** @} */ /* End of group SDIO_DEMO_SDIO */

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

