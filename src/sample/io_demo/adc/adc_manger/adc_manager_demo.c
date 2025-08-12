/**
*****************************************************************************************
*     Copyright(c) 2019, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file     adc_manager_demo.c
   * @brief    This file provides demo code of adc in one-shot mode.
   * @details
   * @author   sandy
   * @date     2019-03-14
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2019 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_adc.h"
#include "trace.h"
#include "adc_manager.h"
#include "os_timer.h"


/** @defgroup  ADC_MANAGER_DEMO ADC Manager Demo
    * @brief  adc implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup ADC_MANAGER_DEMO_Exported_Macros Adc Manager Exported Macros
    * @{
    */

/**  @brief  The  number of read back and read cycle*/
#define TEST_COUNT (100)
#define TEST_CYCLE (3)


/** End of ADC_MANAGER_DEMO_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup ADC_MANAGER_DEMO_Private_Variables  Private Variables
    * @{
    */

static uint8_t
test_adc_channel_handler;/*hander  for every adc read which register to the adc manger*/
static int32_t test_count = TEST_COUNT;
static int32_t test_cycle = TEST_CYCLE;
static void *test_timer;

/** End of ADC_MANAGER_DEMO_Private_Variables
    * @}
    */


/*============================================================================*
 *                              Private Functions
 *============================================================================*/
/** @defgroup ADC_MANAGER_DEMO_Functions  Private Functions
    * @{
    */

/**
  * @brief  adc callback function execute when adc_mgr_enable_req be called  .
  * @param   No used.
  * @return  void
  */
static void adc_cb(void *pvPara, uint32_t int_status)
{
    uint16_t adc_data[2] = {0};
    uint16_t sched_bit_map = 0x0003;

    adc_mgr_read_data_req(test_adc_channel_handler, adc_data, sched_bit_map);
    IO_PRINT_INFO3("adc_cb: seq %d, adc_data[0] %d, adc_data[1] %d",
                   TEST_COUNT - test_count,
                   adc_data[0], adc_data[1]);
    if (test_count > 0)
    {
        test_count--;
    }
    if (test_count == 0)
    {
        adc_mgr_free_chann(test_adc_channel_handler);
    }
}

/**
  * @brief  os timer handler function , enable the adc channel every timer count  to end.
  * @param   No used.
  * @return  void
  */
static void test_timer_handler(void *xTimer)
{
    adc_mgr_enable_req(test_adc_channel_handler);
    if (test_count == 0)
    {
        test_cycle--;
        if (test_cycle > 0)
        {
            test_count = TEST_COUNT;
            ADC_InitTypeDef ADC_InitStruct;
            ADC_StructInit(&ADC_InitStruct);
            ADC_InitStruct.adcClock = ADC_CLK_39K;
            ADC_InitStruct.bitmap = 0x0003;
            ADC_InitStruct.schIndex[0] = INTERNAL_VBAT_MODE;
            ADC_InitStruct.schIndex[1] = INTERNAL_VADPIN_MODE;

            if (!adc_mgr_register_req(&ADC_InitStruct, (adc_callback_function_t)adc_cb,
                                      &test_adc_channel_handler))
            {
                IO_PRINT_ERROR0("test_timer_handler: adc_manager Register Request Fail");
            }
        }
        else
        {
            os_timer_delete(&test_timer);
        }

    }
}
/** @} */ /* End of group ADC_MANAGER_DEMO_Private_Functions */


/*============================================================================*
 *                              Public Functions
 *============================================================================*/
/**
  * @brief  demo code of operation about ADC through adc manger.
            creat a timer task for read adc when timer count to end and regesiter one adc read event.
  * @param   No parameter.
  * @return  void
  */
void adc_manager_demo(void)
{
    os_timer_create(&test_timer, "TEST_TIMER", 2, 100, true, test_timer_handler);
    /* Start Carger Timer */
    if (!os_timer_start(&test_timer))
    {
        IO_PRINT_ERROR0("adc_manager_demo: TEST_TIMER Start Fail");
    }

    ADC_InitTypeDef ADC_InitStruct;
    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.adcClock = ADC_CLK_39K;
    ADC_InitStruct.bitmap = 0x0003;
    ADC_InitStruct.schIndex[0] = INTERNAL_VBAT_MODE;
    ADC_InitStruct.schIndex[1] = INTERNAL_VADPIN_MODE;

    if (!adc_mgr_register_req(&ADC_InitStruct, (adc_callback_function_t)adc_cb,
                              &test_adc_channel_handler))
    {
        IO_PRINT_ERROR0("adc_manager_demo: adc_manager Register Request Fail");
    }
}

/** @} */ /* End of group ADC_MANAGER_DEMO */
