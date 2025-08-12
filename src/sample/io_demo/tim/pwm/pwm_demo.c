/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     pwm_demo.c
* @brief    tim + pwm demo and Deadzone demo
* @details
* @author   renee
* @date     2017-01-23
* @version  v0.1
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include "rtl876x.h"
#include "trace.h"
#include "pwm.h"


/** @defgroup  PWM_DEMO  PWM DEMO
    * @brief  Pwm implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup PWM_Demo_Exported_Macros PWM Demo Exported Macros
  * @brief
  * @{
  */

#define PWM_OUT_PIN              ADC_0
#define PWM_OUT_PIN_P            ADC_2
#define PWM_OUT_PIN_N            ADC_3
#define PWM_LOW_LEVEL_CNT                       2000    //LOW LEVEL count ,count frequnce is 1Mhz
#define PWM_HIGH_LEVEL_CNT                      2000    //High LEVEL count ,count frequnce is 1Mhz
#define PWM_DEADZONE_SIZE_CNT                   0x10    //Deadzone size count,count frequnce is 32K

/** @} */ /* End of group PWM_Demo_Exported_Macros */

static T_PWM_HANDLE demo_pwm_handle;
static T_PWM_HANDLE demo_pwm_deadzone_handle;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup PWM_Demo_Exported_Functions PWM Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  Initialize PWM peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_pwm_init(void)
{
    T_PWM_CONFIG demo_pwm_deadzone_para;

    demo_pwm_handle = pwm_create("demo_pwm", PWM_HIGH_LEVEL_CNT, PWM_LOW_LEVEL_CNT, false);
    if (demo_pwm_handle == NULL)
    {
        IO_PRINT_ERROR0("driver_pwm_init: Fail to create pwm handle");
        return;
    }

    pwm_pin_config(demo_pwm_handle, PWM_OUT_PIN, PWM_FUNC);
    pwm_start(demo_pwm_handle);

    demo_pwm_deadzone_handle = pwm_create("demo_pwm_deadzone", PWM_HIGH_LEVEL_CNT, PWM_LOW_LEVEL_CNT,
                                          true);
    if (demo_pwm_deadzone_handle == NULL)
    {
        IO_PRINT_ERROR0("driver_pwm_init: Fail to create pwm deadzone handle");
        return;
    }

    demo_pwm_deadzone_para.pwm_high_count = PWM_HIGH_LEVEL_CNT;
    demo_pwm_deadzone_para.pwm_low_count = PWM_LOW_LEVEL_CNT;
    demo_pwm_deadzone_para.pwm_deadzone_enable = ENABLE;
    demo_pwm_deadzone_para.clock_source = PWM_CLOCK_1M;
    demo_pwm_deadzone_para.pwm_deadzone_size = PWM_DEADZONE_SIZE_CNT;
    demo_pwm_deadzone_para.pwm_p_stop_state = PWM_DEAD_ZONE_STOP_LOW;
    demo_pwm_deadzone_para.pwm_n_stop_state = PWM_DEAD_ZONE_STOP_HIGH;
    pwm_config(demo_pwm_deadzone_handle, &demo_pwm_deadzone_para);

    pwm_pin_config(demo_pwm_deadzone_handle, PWM_OUT_PIN_P, PWM_FUNC_P);
    pwm_pin_config(demo_pwm_deadzone_handle, PWM_OUT_PIN_N, PWM_FUNC_N);
    pwm_start(demo_pwm_deadzone_handle);
}

/**
  * @brief  demo code of operation about PWM.
  * @param   No parameter.
  * @return  void
  */
void pwm_demo(void)
{
    /* Initialize PWM peripheral */
    driver_pwm_init();
}

/** @} */ /* End of group PWM_Demo_Exported_Functions */
/** @} */ /* End of group PWM_DEMO */

