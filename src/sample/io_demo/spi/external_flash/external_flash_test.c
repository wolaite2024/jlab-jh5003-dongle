/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     external_flash_test.c
* @brief    This file provides test code for external flash.
* @details
* @author   Colin
* @date     2023-06-05
* @version  v1.0
*********************************************************************************************************
*/


/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "os_task.h"
#include "os_msg.h"
#include "os_mem.h"
#include "external_flash.h"
#include "external_flash_test.h"

/** @addtogroup SPI_EXTERNAL_FLASH_TEST External Flash Test
  * @brief External flash test module
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup External_Flash_TEST_Macros External flash Test Code Macros
    * @brief
    * @{
    */
/**
  * @brief  External flash test msg definition
  */

typedef struct
{
    EXT_FLASH_TEST_CASE test_case;
    EXT_FLASH_TEST_EVENT event;
} T_FLASH_TEST_MSG;

/** @} */ /* End of group External_Flash_TEST_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup External_Flash_TEST_Variables External Flash Test Variables
    * @brief
    * @{
    */
static uint8_t sendBuf[256];
static uint8_t recvBuf[256];
static void *flash_task_handle;
static void *flash_evt_queue_handle;
static T_FLASH_TEST_MSG flash_test_msg;

/** @} */ /* End of group External_Flash_TEST_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/

/** @defgroup External_Flash_TEST_Exported_Functions External Flash Test Functions
    * @brief
    * @{
    */
void flash_test_send_msg(EXT_FLASH_TEST_CASE test_case, EXT_FLASH_TEST_EVENT event)
{
    flash_test_msg.test_case = test_case;
    flash_test_msg.event = event;

    if (os_msg_send(flash_evt_queue_handle, &flash_test_msg, 0) == false)
    {
        IO_PRINT_ERROR0("flash_test_send_msg: failed");
    }
}

static void ext_flash_spi_test_polling_mode(void)
{
    bool test_result = true;
    uint16_t index = 0;

    for (index = 0; index < 200; index++)
    {
        sendBuf[index] = index + 2;
    }

    for (index = 0; index < 256; index++)
    {
        recvBuf[index] = 0;
    }

    ext_flash_spi_erase(0, EXT_FLASH_BLOCK_ERASE_32_CMD);
    ext_flash_spi_page_program(0, sendBuf, 200);
    ext_flash_spi_read(0, recvBuf, 200);
    for (uint8_t dbg_index = 0; dbg_index < 200; dbg_index++)
    {
        if (sendBuf[dbg_index] != recvBuf[dbg_index])
        {
            IO_PRINT_INFO3("ext_flash_spi_test_polling_mode: Failed dbg_index %d, sendBuf %d, recvBuf %d",
                           dbg_index, sendBuf[dbg_index],
                           recvBuf[dbg_index]);
            test_result = false;
            break;
        }
    }
    if (test_result)
    {
        IO_PRINT_INFO0("ext_flash_spi_test_polling_mode: success");
        flash_test_send_msg(FLASH_TEST_INTERRUPT_MODE, FLASH_TEST_START);
    }
}

static void flash_test_task(void *pvParameters)
{
    uint16_t index = 0;
    uint8_t status = 0;
    uint8_t dbg_index = 0;
    bool test_result = true;
    T_FLASH_TEST_MSG msg;

    ext_flash_spi_init();

    status = ext_flash_spi_read_status();

    if (status & EXT_FLASH_SECTION_PROTECT)
    {
        IO_PRINT_INFO1("flash_test_task: flash section protect status 0x%x", status);
    }

    flash_test_send_msg(FLASH_TEST_POLLING_MODE, FLASH_TEST_START);

    while (true)
    {
        if (os_msg_recv(flash_evt_queue_handle, &msg, 0xFFFFFFFF) == true)
        {
            IO_PRINT_INFO2("flash_test_task: test_case %d, event %d", msg.test_case, msg.event);
            switch (msg.test_case)
            {
            case FLASH_TEST_POLLING_MODE:
                {
                    if (msg.event == FLASH_TEST_START)
                    {
                        ext_flash_spi_test_polling_mode();
                    }
                }
                break;
            case FLASH_TEST_INTERRUPT_MODE:
                {
                    if (msg.event == FLASH_TEST_START)
                    {
                        test_result = true;

                        for (index = 0; index < 200; index++)
                        {
                            sendBuf[index] = index + 3;
                        }

                        for (index = 0; index < 256; index++)
                        {
                            recvBuf[index] = 0;
                        }
                        ext_flash_spi_erase(0, EXT_FLASH_BLOCK_ERASE_32_CMD);
                        ext_flash_spi_page_program_by_interrupt(0, sendBuf, 200);
                    }
                    else if (msg.event == FLASH_TEST_WRITE_FINISH)
                    {
                        ext_flash_spi_read_by_interrupt(0, recvBuf, 200);
                    }
                    else if (msg.event == FLASH_TEST_READ_FINISH)
                    {
                        for (dbg_index = 0; dbg_index < 200; dbg_index++)
                        {
                            if (sendBuf[dbg_index] != recvBuf[dbg_index])
                            {
                                test_result = false;
                                goto result;
                            }
                        }
                        flash_test_send_msg(FLASH_TEST_GDMA_MODE, FLASH_TEST_START);
                        goto result;
                    }
                }
                break;
            case FLASH_TEST_GDMA_MODE:
                {
                    if (msg.event == FLASH_TEST_START)
                    {
                        test_result = true;

                        for (index = 0; index < 200; index++)
                        {
                            sendBuf[index] = index + 4;
                        }

                        for (index = 0; index < 256; index++)
                        {
                            recvBuf[index] = 0;
                        }
                        ext_flash_spi_erase(0, EXT_FLASH_BLOCK_ERASE_32_CMD);
                        ext_flash_spi_page_program_by_dma(0, sendBuf, 200);
                    }
                    else if (msg.event == FLASH_TEST_WRITE_FINISH)
                    {
                        ext_flash_spi_read_by_dma(0, recvBuf, 200);
                    }
                    else if (msg.event == FLASH_TEST_READ_FINISH)
                    {
                        for (dbg_index = 0; dbg_index < 200; dbg_index++)
                        {
                            if (sendBuf[dbg_index] != recvBuf[dbg_index])
                            {
                                test_result = false;
                                goto result;
                            }
                        }
                        flash_test_send_msg(FLASH_TEST_ERASE, FLASH_TEST_START);
                        goto result;
                    }
                }
                break;
            case FLASH_TEST_ERASE:
                {
                    if (msg.event == FLASH_TEST_START)
                    {
                        test_result = true;

                        ext_flash_spi_erase_chip();
                        ext_flash_spi_read(0, recvBuf, 200);
                        for (dbg_index = 0; dbg_index < 200; dbg_index++)
                        {
                            if (0xFF != recvBuf[dbg_index])
                            {
                                test_result = false;
                                goto result;
                            }
                        }
                        goto result;
                    }
                }
                break;
            default:
                break;
            }
            continue;
result:
            if (test_result)
            {
                IO_PRINT_INFO1("flash_test_task: test_case %d success", msg.test_case);
            }
            else
            {
                IO_PRINT_INFO4("flash_test_task: test_case %d failed dbg_index %d, sendBuf %d, recvBuf %d",
                               msg.test_case, dbg_index, sendBuf[dbg_index],
                               recvBuf[dbg_index]);
            }

        }
    }
}

/**
  * @brief  demo code of operation about Flash.
  * @param  No parameter.
  * @return  void
  */
void ext_flash_spi_test_code(void)
{
    os_msg_queue_create(&flash_evt_queue_handle, "ioQ", 10, sizeof(T_FLASH_TEST_MSG));

    /* This task is only for users to run flash test */
    os_task_create(&flash_task_handle, "flash_task", flash_test_task, NULL, 3 * 1024, 2);
}

/** @} */ /* End of group External_Flash_TEST_Exported_Functions */

/** @} */ /* End of group SPI_EXTERNAL_FLASH_TEST */

/******************* (C) COPYRIGHT 2021 Realtek Semiconductor Corporation *****END OF FILE****/

