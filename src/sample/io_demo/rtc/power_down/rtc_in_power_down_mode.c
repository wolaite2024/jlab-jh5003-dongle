/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtc_in_power_down_mode.c
* @brief    This file provides demo code to realize RTC work in power down mode.
* @details
* @author   colin
* @date     2023-1-11
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_rcc.h"
#include "rtl876x_rtc.h"
#include "rtl876x_nvic.h"
#include "trace.h"
#include "vector_table.h"
#include "dlps_util.h"
#include "io_dlps.h"
#include "pm.h"

/** @defgroup  RTC_IN_POWER_DOWN_MODE  RTC IN POWER DOWN MODE
    * @brief  RTC work in power down mode implementation demo code
    * @{
    */


/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup RTC_IN_POWER_DOWN_MODE_Exported_Macros RTC In Power Down Mode Exported Macros
  * @brief
  * @{
  */

#define RTC_PRESCALER_VALUE         4095
#define RTC_COMP_INDEX              COMP1_INDEX
#define RTC_INT_CMP_1               RTC_INT_CMP1
#define RTC_COMP_VALUE              7//(0xC000)

#define RTC_WAKE_UP_COMP_INDEX      COMP0_INDEX

/** @} */ /* End of group RTC_IN_POWER_DOWN_MODE_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup RTC_IN_POWER_DOWN_MODE_Exported_Functions RTC In Power Down Mode Exported Functions
  * @brief
  * @{
  */
static void rtc_handler(void);
extern void (*set_clk_32k_power_in_powerdown)(bool);

/**
  * @brief  Initialize RTC NVIC.
  * @param   No parameter.
  * @return  void
  */
static void driver_rtc_nvic_init(void)
{
    /* power down mode use RTC need to open 32k */
    set_clk_32k_power_in_powerdown(true);

    RamVectorTableUpdate(RTC_VECTORn, rtc_handler);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
  * @brief  Initialize RTC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_rtc_init(void)
{
    RTC_DeInit();
    RTC_SetPrescaler(RTC_PRESCALER_VALUE);
    RTC_SetComp(RTC_COMP_INDEX, RTC_COMP_VALUE);
    RTC_MaskINTConfig(RTC_INT_CMP_1, DISABLE);
    RTC_CompINTConfig(RTC_INT_CMP_1, ENABLE);

    driver_rtc_nvic_init();

    /* Start RTC */
    RTC_RunCmd(ENABLE);
}

/**
  * @brief  Initialize RTC power down wake up time.
  * @param  wakeup_time_s: RTC wake up time in power down mode.
  * @return  void
  */
static void app_dlps_system_wakeup_by_rtc(uint32_t wakeup_time_s)
{
    uint32_t prescaler_value = RTC_PRESCALER_VALUE; /* 1 counter : (prescaler_value + 1)/32000  sec*/
    uint32_t comparator_value = (uint32_t)(((uint64_t)wakeup_time_s * 32000) / (prescaler_value + 1));
    uint32_t current_value = 0;

    RTC_CompINTConfig(RTC_INT_CMP0 | RTC_CMP0_WK_INT, ENABLE);
    RTC_MaskINTConfig(RTC_INT_CMP0 | RTC_CMP0_WK_INT, DISABLE);

    RTC_SystemWakeupConfig(ENABLE);

    current_value = RTC_GetCounter();
    RTC_SetComp(RTC_WAKE_UP_COMP_INDEX, current_value + comparator_value);
}

/**
  * @brief   DLPS enter callback.
  * @param   No parameter.
  * @return  void
  */
static void app_dlps_enter_callback(void)
{
    POWERMode lps_mode = power_mode_get();

    if (lps_mode == POWER_POWERDOWN_MODE)
    {
        app_dlps_system_wakeup_by_rtc(10);
        DBG_DIRECT("app_dlps_enter_callback: 10 sec");
    }
}

/**
  * @brief  demo code of operation about RTC.
  * @param   No parameter.
  * @return  void
  */
void rtc_in_power_down_mode(void)
{
    uint8_t pm_wakeup_reason = 0;

    pm_wakeup_reason = power_down_check_wake_up_reason();

    /* not wake up from power down */
    if (pm_wakeup_reason == 0)
    {
        IO_PRINT_INFO0("rtc_in_power_down_mode: rtc first init");
        /* init RTC first time from reset */
        driver_rtc_init();
    }
    else
    {
        driver_rtc_nvic_init();
    }

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);

    io_dlps_register();

    io_dlps_register_enter_cb(app_dlps_enter_callback);

    power_mode_set(POWER_POWERDOWN_MODE);
}

/**
  * @brief  RTC interrupt handle function.
  * @param  None.
  * @return None.
  */
static void rtc_handler(void)
{
    if (RTC_GetINTStatus(RTC_INT_CMP0) == SET)
    {
        RTC_ClearCompINT(RTC_WAKE_UP_COMP_INDEX);
        /* clear rtc wake up event */
        RTC_ClearCompWkINT(RTC_WAKE_UP_COMP_INDEX);
        IO_PRINT_TRACE0("rtc_handler: RTC_INT_CMP0");
    }

    if (RTC_GetINTStatus(RTC_INT_CMP_1) == SET)
    {
        RTC_SetComp(RTC_COMP_INDEX, RTC_GetCounter() + RTC_COMP_VALUE);
        RTC_ClearCompINT(RTC_COMP_INDEX);
        IO_PRINT_TRACE0("rtc_handler: RTC_INT_CMP_1");
    }
}

/** @} */ /* End of group RTC_IN_POWER_DOWN_MODE_Exported_Functions */
/** @} */ /* End of group RTC_IN_POWER_DOWN_MODE */


/******************* (C) COPYRIGHT 2016 Realtek Semiconductor Corporation *****END OF FILE****/

