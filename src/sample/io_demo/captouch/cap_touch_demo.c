/**
************************************************************************************************************
*            Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file      cap_touch_demo.c
* @brief     Cap Touch Demonstration.
* @author    js.lin
* @date      2020-11-24
* @version   v1.0
*************************************************************************************************************
*/
#include <string.h>

#include "trace.h"
#include "os_timer.h"
#include "vector_table.h"

#include "rtl876x_captouch.h"

#include "pmu_api.h"


/** @defgroup  CapTouch_DEMO CapTouch
    * @brief  CapTouch
    * @{
    */

#define CTC_TIMER_PERIOD  1000  /*unit  ms*/

static void *ctc_test_timer_handle = NULL;
static void ctc_timer_finish_handler(void *xTimer);

static void CapTouch_status(void);
static uint8_t ctc_timer_cnt = 0;

static void ctc_test_timer_init(void)
{
    /* Create ctc_test_timer */
    if (ctc_test_timer_handle == NULL)
    {
        if (!os_timer_create(&ctc_test_timer_handle, "ctc_test_timer", 1,
                             CTC_TIMER_PERIOD, true, ctc_timer_finish_handler))
        {
            IO_PRINT_ERROR0("ctc_test_timer_init: ctc_test_timer Create Fail");
        }
    }
}

/**
  * @brief  CapTouch Interrupt handler.
  * @retval None
  */
static void captouch_handler(void)
{
    uint32_t int_status = 0;
    int_status = CapTouch_GetINTStatus();
    IO_PRINT_INFO1("captouch_handler: int_status 0x%x", int_status);
    /* Channel 0 interrupts */
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_TOUCH_PRESS_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_TOUCH_PRESS_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_TOUCH_RELEASE_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_TOUCH_RELEASE_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH0, CTC_FALSE_TOUCH_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH0, CTC_FALSE_TOUCH_INT);
    }
    /* Channel 1 interrupts */
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH1, CTC_TOUCH_PRESS_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH1, CTC_TOUCH_PRESS_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH1, CTC_TOUCH_RELEASE_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH1, CTC_TOUCH_RELEASE_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH1, CTC_FALSE_TOUCH_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH1, CTC_FALSE_TOUCH_INT);
    }
    /* Channel 2 interrupts */
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH2, CTC_TOUCH_PRESS_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH2, CTC_TOUCH_PRESS_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH2, CTC_TOUCH_RELEASE_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH2, CTC_TOUCH_RELEASE_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH2, CTC_FALSE_TOUCH_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH2, CTC_FALSE_TOUCH_INT);
    }
    /* Channel 3 interrupts */
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH3, CTC_TOUCH_PRESS_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH3, CTC_TOUCH_PRESS_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH3, CTC_TOUCH_RELEASE_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH3, CTC_TOUCH_RELEASE_INT);
    }
    if (CapTouch_IsChINTTriggered(int_status, CTC_CH3, CTC_FALSE_TOUCH_INT))
    {
        /* do something */
        CapTouch_ChINTClearPendingBit(CTC_CH3, CTC_FALSE_TOUCH_INT);
    }
    /* Noise Interrupt */
    if (CapTouch_IsNoiseINTTriggered(int_status, CTC_OVER_N_NOISE_INT))
    {
        /* do something */
        CapTouch_NoiseINTClearPendingBit(CTC_OVER_N_NOISE_INT);
    }
    if (CapTouch_IsNoiseINTTriggered(int_status, CTC_OVER_P_NOISE_INT))
    {
        IO_PRINT_ERROR0("captouch_handler: OVER_P_NOISE_INT baseline error, need to reset Cap-touch system");
        CapTouch_NoiseINTClearPendingBit(CTC_OVER_P_NOISE_INT);
    }
}
static void CapTouch_status(void)
{
    CTC_CH_TYPE i;
    CTC_CH_TYPE max_channel;
    max_channel = (CTC_CH_TYPE)CapTouch_GetMaxChannel();

    for (i = CTC_CH0; i <= max_channel; i++)
    {
        IO_PRINT_INFO5("CapTouch_status: channel %d, AveData %4d, Baseline %4d, AveData-Baseline %4d, TouchCnt %2d",
                       i,
                       CapTouch_GetChAveData(i), CapTouch_GetChBaseline(i),
                       CapTouch_GetChAveData(i) - CapTouch_GetChBaseline(i),
                       CapTouch_GetChTouchCnt(i));
    }
    IO_PRINT_INFO2("CapTouch_status: fs %d, TouchStatus %d", CapTouch_IsFastMode(),
                   CapTouch_GetChTouchStatus());
}
void cap_touch_demo(void)
{
    CapTouch_SystemEnable(true);
    CapTouch_SysReset();
    IO_PRINT_INFO2("cap_touch_demo: Start, max_channel %d, cap_touch_enable %d",
                   CapTouch_GetMaxChannel(), CapTouch_IsSystemEnable());

    /* Register new interrupt handler to vector table */
    RamVectorTableUpdate(TOUCH_VECTORn, (IRQ_Fun)captouch_handler);

    /* Channel enable */
    CapTouch_ChCmd(CTC_CH0, ENABLE);
    CapTouch_ChCmd(CTC_CH1, ENABLE);
    CapTouch_ChCmd(CTC_CH2, ENABLE);
    CapTouch_ChCmd(CTC_CH3, ENABLE);

    /* Interrupt enable */
    CapTouch_ChINTConfig(CTC_CH0, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                    CTC_FALSE_TOUCH_INT), ENABLE);
    CapTouch_ChINTConfig(CTC_CH1, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                    CTC_FALSE_TOUCH_INT), ENABLE);
    CapTouch_ChINTConfig(CTC_CH2, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                    CTC_FALSE_TOUCH_INT), ENABLE);
    CapTouch_ChINTConfig(CTC_CH3, (CTC_CH_INT_TYPE)(CTC_TOUCH_PRESS_INT | CTC_TOUCH_RELEASE_INT |
                                                    CTC_FALSE_TOUCH_INT), ENABLE);
    CapTouch_NoiseINTConfig(CTC_OVER_P_NOISE_INT, ENABLE);


    /* Set scan interval */
    if (!CapTouch_SetScanInterval(0x3B, CTC_SLOW_MODE))
    {
        IO_PRINT_WARN0("cap_touch_demo: Slow mode scan interval overange");
    }
    if (!CapTouch_SetScanInterval(0x1D, CTC_FAST_MODE))
    {
        IO_PRINT_WARN0("cap_touch_demo: Fast mode scan interval overange");
    }


    /* Enable touch wakeup from LPS, DLPS, PowerDown */
    CapTouch_ChWakeupCmd(CTC_CH0, (FunctionalState)ENABLE);
    CapTouch_ChWakeupCmd(CTC_CH1, (FunctionalState)ENABLE);
    CapTouch_ChWakeupCmd(CTC_CH2, (FunctionalState)ENABLE);
    CapTouch_ChWakeupCmd(CTC_CH3, (FunctionalState)ENABLE);

    /*should keep 32k clk if enable wakeup from power down mode*/
    pmu_set_clk_32k_power_in_powerdown(true);

    /* Cap Touch start start after 3s later */
    CapTouch_Cmd(ENABLE, ENABLE);

    ctc_test_timer_init();

    /* Start ctc_test_timer */
    if (!os_timer_start(&ctc_test_timer_handle))
    {
        IO_PRINT_ERROR0("cap_touch_demo: ctc_test_timer Start Fail");
    }
}

/**
 * @brief  ctc timer finish handler
 * @param  xTimer: timer
 * @retval None
 */
static void ctc_timer_finish_handler(void *xTimer)
{
    ctc_timer_cnt++;
    IO_PRINT_INFO1("ctc_timer_finish_handler: ctc_timer_cnt %d", ctc_timer_cnt);
    CapTouch_status();
    os_timer_stop(&ctc_test_timer_handle);
}

/** @} */ /* End of group CapTouch_DEMO */

