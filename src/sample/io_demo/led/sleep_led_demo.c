/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     sleep_led_demo.c
* @brief    sleep led demo, LED Pin: P2_0, P2_1, P2_2, ADC_0, ADC_1, ADC_6, ADC_7, P1_0, P1_1, P1_4
* @details
* @author   renee
* @date     2022-01-23
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_pinmux.h"
#include "rtl876x_sleep_led.h"

/** @defgroup  LED_DEMO_LED  LED DEMO
    * @brief  Led work in breathe mode and blink implementation demo code
    * @{
    */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Led_Demo_Exported_Macros Led Demo Exported Macros
  * @brief
  * @{
  */

#define LED_OUT_0       ADC_1
#define LED_OUT_1       P2_1
#define LED_OUT_2       P2_2

/** @} */ /* End of group Led_Demo_Exported_Macros */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Led_Demo_Exported_Functions Led Demo Exported Functions
  * @brief
  * @{
  */

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_led_init(void)
{
    Pad_Config(LED_OUT_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LED_OUT_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LED_OUT_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pad_FunctionConfig(LED_OUT_0, LED0);
    Pad_FunctionConfig(LED_OUT_1, LED1);
    Pad_FunctionConfig(LED_OUT_2, LED2);
}

/**
  * @brief  Initialize sleep led peripheral in breahe mode.
  * @param   No parameter.
  * @return  void
  */
static void driver_led_init_breathe(void)
{
    SleepLed_Reset();
    SLEEP_LED_InitTypeDef Led_Initsturcture;
    SleepLed_StructInit(&Led_Initsturcture);
    Led_Initsturcture.prescale                    = 320;
    Led_Initsturcture.mode                        = LED_BREATHE_MODE;
    Led_Initsturcture.polarity                    = LED_OUTPUT_NORMAL;

    Led_Initsturcture.phase_uptate_rate[0]              = 0;
    Led_Initsturcture.phase_phase_tick[0]               = 50;
    Led_Initsturcture.phase_initial_duty[0]             = 0;
    Led_Initsturcture.phase_increase_duty[0]            = 1;
    Led_Initsturcture.phase_duty_step[0]                = 1;

    Led_Initsturcture.phase_uptate_rate[1]              = 0;
    Led_Initsturcture.phase_phase_tick[1]               = 50;
    Led_Initsturcture.phase_initial_duty[1]             = 100;
    Led_Initsturcture.phase_increase_duty[1]            = 1;
    Led_Initsturcture.phase_duty_step[1]                = 2;

    Led_Initsturcture.phase_uptate_rate[2]              = 0;
    Led_Initsturcture.phase_phase_tick[2]               = 50;
    Led_Initsturcture.phase_initial_duty[2]             = 200;
    Led_Initsturcture.phase_increase_duty[2]            = 1;
    Led_Initsturcture.phase_duty_step[2]                = 3;

    Led_Initsturcture.phase_uptate_rate[3]              = 0;
    Led_Initsturcture.phase_phase_tick[3]               = 50;
    Led_Initsturcture.phase_initial_duty[3]             = 300;
    Led_Initsturcture.phase_increase_duty[3]            = 1;
    Led_Initsturcture.phase_duty_step[3]                = 4;

    Led_Initsturcture.phase_uptate_rate[4]              = 0;
    Led_Initsturcture.phase_phase_tick[4]               = 50;
    Led_Initsturcture.phase_initial_duty[4]             = 300;
    Led_Initsturcture.phase_increase_duty[4]            = 0;
    Led_Initsturcture.phase_duty_step[4]                = 1;

    Led_Initsturcture.phase_uptate_rate[5]              = 0;
    Led_Initsturcture.phase_phase_tick[5]               = 50;
    Led_Initsturcture.phase_initial_duty[5]             = 200;
    Led_Initsturcture.phase_increase_duty[5]            = 0;
    Led_Initsturcture.phase_duty_step[5]                = 2;

    Led_Initsturcture.phase_uptate_rate[6]              = 0;
    Led_Initsturcture.phase_phase_tick[6]               = 50;
    Led_Initsturcture.phase_initial_duty[6]             = 100;
    Led_Initsturcture.phase_increase_duty[6]            = 0;
    Led_Initsturcture.phase_duty_step[6]                = 3;

    Led_Initsturcture.phase_uptate_rate[7]              = 0;
    Led_Initsturcture.phase_phase_tick[7]               = 50;
    Led_Initsturcture.phase_initial_duty[7]             = 0;
    Led_Initsturcture.phase_increase_duty[7]            = 0;
    Led_Initsturcture.phase_duty_step[7]                = 0;

    SleepLed_Init(LED_CHANNEL_0, &Led_Initsturcture);
    SleepLed_Init(LED_CHANNEL_1, &Led_Initsturcture);
    SleepLed_Init(LED_CHANNEL_2, &Led_Initsturcture);
    SleepLed_Cmd(LED_CHANNEL_1 | LED_CHANNEL_0 | LED_CHANNEL_2, ENABLE);
}

/**
  * @brief  Initialize sleep led peripheral blink mode.
  * @param   No parameter.
  * @return  void
  */
static void driver_led_init_blink(void)
{
    SleepLed_Reset();
    SLEEP_LED_InitTypeDef Led_Initsturcture;
    SleepLed_StructInit(&Led_Initsturcture);
    Led_Initsturcture.mode                        = LED_BLINK_MODE;
    Led_Initsturcture.polarity                    = LED_OUTPUT_NORMAL;
    Led_Initsturcture.prescale                    = 32;
    Led_Initsturcture.period_high[0]                = 200;
    Led_Initsturcture.period_low[0]                  = 100;
    Led_Initsturcture.period_high[1]                = 200;
    Led_Initsturcture.period_low[1]                  = 100;
    Led_Initsturcture.period_high[2]                = 200;
    Led_Initsturcture.period_low[2]                  = 100;
    SleepLed_Init(LED_CHANNEL_0, &Led_Initsturcture);
    Led_Initsturcture.period_high[2]                = 0;
    Led_Initsturcture.period_low[2]                  = 0;
    SleepLed_Init(LED_CHANNEL_1, &Led_Initsturcture);
    Led_Initsturcture.period_high[1]                = 0;
    Led_Initsturcture.period_low[1]                  = 0;
    SleepLed_Init(LED_CHANNEL_2, &Led_Initsturcture);
    SleepLed_Cmd(LED_CHANNEL_1 | LED_CHANNEL_0 | LED_CHANNEL_2, ENABLE);
}

/**
  * @brief  demo code of operation about LED breathe mode.
  * @param   No parameter.
  * @return  void
  */
void sleep_led_breathe_demo(void)
{
    /* Configure PAD firstly! */
    board_led_init();

    /* Initialize LED peripheral */
    driver_led_init_breathe();
}

/**
  * @brief  demo code of operation about LED blink mode.
  * @param   No parameter.
  * @return  void
  */
void sleep_led_blink_demo(void)
{
    /* Configure PAD firstly! */
    board_led_init();

    /* Initialize LED peripheral */
    driver_led_init_blink();
}

/** @} */ /* End of group Led_Demo_Exported_Functions */
/** @} */ /* End of group LED_DEMO_LED */
