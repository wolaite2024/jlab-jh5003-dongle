/**
*********************************************************************************************************
*               Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file         ir_detect_pulse_demo.c
* @brief        This file provides IR demo code to pulse detection with ir detect module.
* @details
* @author       colin_lu
* @date         2025-03-28
* @version      v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "rtl876x.h"
#include "ir_detect_pulse.h"
#include "trace.h"
#include "pm.h"
#include "io_dlps.h"
#include "os_timer.h"


/** @defgroup  IR_DETECT_PULSE_DEMO  IR DETECT PULSE DEMO
    * @brief  Ir detect pulse implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
#define DETECT_PIN                     P2_1
#define MAX_DETECT_PULSE_CNT           100

/*============================================================================*
 *                              Variables
 *============================================================================*/
/* Buffer which store pulse data */
static IRDetectPulse_TypeDef data[MAX_DETECT_PULSE_CNT] = {0};
static void *ir_detect_timer_handle = NULL;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup IR_DETECT_PULSE_DEMO_Exported_Functions IR Pulse Detection Demo Exported Functions
  * @brief
  * @{
  */

void ir_detect_callback(IRDetectPulse_TypeDef *pulse_data, uint32_t len)
{
    memcpy(data, pulse_data, sizeof(IRDetectPulse_TypeDef) * len);

    for (uint32_t i = 0; i < len; i++)
    {
        IO_PRINT_INFO3("ir_detect_callback index %d level %d time %d", i, data[i].pulse, data[i].time);
    }

    /* When IR detection ends, DLPS will be enabled. */
    ir_detect_pulse_stop();
}

static void ir_detect_timer_callback(void *xTimer)
{
    IO_PRINT_TRACE0("ir_detect_timer_callback");

    /* When IR detection begins, DLPS will be disabled. */
    ir_detect_pulse_start();
}

/**
  * @brief  demo code of IR send data.
  * @param   No parameter.
  * @return  void
  */
void ir_detect_pulse_demo(void)
{
    IO_PRINT_INFO0("ir_detect_pulse_demo");

    ir_detect_pulse_init(DETECT_PIN, MAX_DETECT_PULSE_CNT, IR_DETECT_BOTH_EDGE,
                         IR_DETECT_IDLE_LEVEL_LOW, 20000);
    ir_detect_pulse_register_callback(ir_detect_callback);

    /* Create test_timer */
    if (ir_detect_timer_handle == NULL)
    {
        if (!os_timer_create(&ir_detect_timer_handle, "ir_detect", 1,
                             100, true, ir_detect_timer_callback))
        {
            IO_PRINT_ERROR0("ir_detect_pulse_demo: ir_detect_timer Create Fail");
        }
    }

    if (!os_timer_start(&ir_detect_timer_handle))
    {
        IO_PRINT_ERROR0("ir_detect_timer Start Fail");
    }

    io_dlps_register();

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    power_mode_set(POWER_DLPS_MODE);

}

/** @} */ /* End of group IR_DETECT_PULSE_DEMO_Exported_Functions */
/** @} */ /* End of group IR_DETECT_PULSE_DEMO */


/******************* (C) COPYRIGHT 2025 Realtek Semiconductor Corporation *****END OF FILE****/

