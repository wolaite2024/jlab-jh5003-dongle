/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtc_in_dlps.c
* @brief    This file provides demo code to realize RTC work in dlps mode.
* @details
* @author   colin
* @date     2024-03-28
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
#include "io_dlps.h"
#include "pm.h"

/** @defgroup  RTC_IN_DLPS_MODE  RTC IN DLPS MODE
    * @brief  RTC work in dlps mode implementation demo code
    * @{
    */


/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup RTC_IN_DLPS_MODE_Exported_Macros RTC In DLPS Mode Exported Macros
  * @brief
  * @{
  */

#define RTC_PRESCALER_VALUE         4095
#define RTC_COMP_INDEX              COMP0_INDEX
#define RTC_COMP_INT_INDEX          RTC_INT_CMP0
#define RTC_COMP_WK_INDEX           RTC_CMP0_WK_INT

/** @} */ /* End of group RTC_IN_DLPS_MODE_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup RTC_IN_DLPS_MODE_Exported_Functions RTC In DLPS Mode Exported Functions
  * @brief
  * @{
  */
static void rtc_handler(void);

/**
  * @brief  Initialize RTC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_rtc_init(void)
{
    RTC_DeInit();
    RTC_SetPrescaler(RTC_PRESCALER_VALUE);

    RamVectorTableUpdate(RTC_VECTORn, rtc_handler);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    /* Start RTC */
    RTC_RunCmd(ENABLE);
}

/**
  * @brief  Initialize RTC dlps wake up time.
  * @param  wakeup_time_s: RTC wake up time in dlps mode.
  * @return  void
  */
static void app_dlps_system_wakeup_by_rtc(uint32_t wakeup_time_s)
{
    uint32_t prescaler_value = RTC_PRESCALER_VALUE; /* 1 counter : (prescaler_value + 1)/32000  sec*/
    uint32_t comparator_value = (uint32_t)(((uint64_t)wakeup_time_s * 32000) / (prescaler_value + 1));
    uint32_t current_value = 0;

    RTC_CompINTConfig(RTC_COMP_INT_INDEX | RTC_COMP_WK_INDEX, ENABLE);
    RTC_MaskINTConfig(RTC_COMP_INT_INDEX | RTC_COMP_WK_INDEX, DISABLE);

    RTC_SystemWakeupConfig(ENABLE);

    current_value = RTC_GetCounter();
    RTC_SetComp(RTC_COMP_INDEX, current_value + comparator_value);
}

/**
  * @brief   DLPS enter callback.
  * @param   No parameter.
  * @return  void
  */
static void app_dlps_enter_callback(void)
{
    app_dlps_system_wakeup_by_rtc(10);
    DBG_DIRECT("app_dlps_enter_callback: 10 sec");
}

/**
  * @brief   DLPS exit callback.
  * @param   No parameter.
  * @return  void
  */
static void app_dlps_exit_callback(void)
{
    IO_PRINT_TRACE0("app_dlps_exit_callback");
}

/**
  * @brief  demo code of operation about RTC.
  * @param   No parameter.
  * @return  void
  */
void rtc_in_dlps_mode(void)
{
    driver_rtc_init();

    bt_power_mode_set(BTPOWER_DEEP_SLEEP);

    io_dlps_register();

    io_dlps_register_enter_cb(app_dlps_enter_callback);
    io_dlps_register_exit_cb(app_dlps_exit_callback);

    power_mode_set(POWER_DLPS_MODE);
}

/**
  * @brief  RTC interrupt handle function.
  * @param  None.
  * @return None.
  */
static void rtc_handler(void)
{
    if (RTC_GetINTStatus(RTC_COMP_INT_INDEX) == SET)
    {
        RTC_ClearCompINT(RTC_COMP_INDEX);
        /* clear rtc wake up event */
        RTC_ClearCompWkINT(RTC_COMP_INDEX);
        IO_PRINT_TRACE0("rtc_handler: RTC_INT_CMP0");
    }
}

/** @} */ /* End of group RTC_IN_DLPS_MODE_Exported_Functions */
/** @} */ /* End of group RTC_IN_DLPS_MODE */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor Corporation *****END OF FILE****/

