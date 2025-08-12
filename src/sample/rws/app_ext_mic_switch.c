#if F_APP_EXT_MIC_SWITCH_SUPPORT && (F_APP_EXT_MIC_PLUG_IN_GPIO_DETECT || F_APP_EXT_MIC_SWITCH_IC_SUPPORT)
#include "trace.h"
#include "app_main.h"
#include "string.h"
#include "app_dlps.h"
#include "app_cfg.h"
#include "app_ext_mic_switch.h"
#include "app_dsp_cfg.h"
#include "app_io_msg.h"
#include "section.h"
#include "hw_tim.h"
#include "fmc_api.h"
#include "rtl876x_rcc.h"
#include "rtl876x_gpio.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "hal_gpio_int.h"
#include "vector_table.h"
#include "app_timer.h"
#include "hal_gpio.h"

#if F_APP_ERWS_SUPPORT
#include "app_audio_policy.h"
#include "app_relay.h"
#endif

#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
#define MIC_SEL_PIN             P6_2
//#define DIO_MUTE_PIN            P6_3
#endif

#if F_APP_EXT_MIC_PLUG_IN_GPIO_DETECT
#define BOOM_IN_DETECT_PIN      P6_1

static uint8_t app_gpio_ext_mic_timer_id = 0;
static uint8_t timer_idx_ext_mic_io_debounce = 0;

typedef enum
{
    APP_TIMER_EXT_MIC_DEBOUNCE,
} T_APP_EXT_MIC_TIMER;
#endif

#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
//DIO32276 mute: 1, unmute: 0
void app_ext_mic_switch_ic_mute(bool mute)
{
#ifdef DIO_MUTE_PIN
    hal_gpio_set_level(DIO_MUTE_PIN, (T_GPIO_LEVEL)mute);
#endif
}

//DIO32276 mic select internal mic: 1, boom mic: 0
void app_ext_mic_switch_ic_mic_sel(bool internal_mic)
{
    hal_gpio_set_level(MIC_SEL_PIN, (T_GPIO_LEVEL)internal_mic);
}
#endif

#if F_APP_EXT_MIC_PLUG_IN_GPIO_DETECT
static uint8_t app_ext_mic_gpio_detect_switch()
{
    //Read BOOM_IN status to set MIC_SEL to High : Internal Mic
    //                                      Low  : Boom Mic
    uint8_t boom_in_level;
    uint8_t mic_plugged = false;

    boom_in_level = hal_gpio_get_input_level(BOOM_IN_DETECT_PIN);

#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
    app_ext_mic_switch_ic_mute(APP_EXT_MIC_IC_MUTE);
#endif

    if (boom_in_level)
    {
#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
        //MIC_SEL_PIN High Level : Boom Mic is not plugged
        app_ext_mic_switch_ic_mic_sel(APP_EXT_MIC_IC_INTERNAL_MIC);
#endif
        mic_plugged = false;
    }
    else
    {
#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
        //MIC_SEL_PIN Low Level : Boom Mic is Plugged
        app_ext_mic_switch_ic_mic_sel(APP_EXT_MIC_IC_BOOM_MIC);
#endif
        mic_plugged = true;
    }

    //switch dsp config image
    if (app_db.ext_mic_plugged != mic_plugged)
    {
        app_db.ext_mic_plugged = mic_plugged;

        if (app_db.ext_mic_plugged)
        {
            flash_dsp_cfg_switch(DSP_CFG_IMG_2);
        }
        else
        {
            flash_dsp_cfg_switch(DSP_CFG_IMG_1);
        }

        app_dsp_cfg_apply();

#if F_APP_ERWS_SUPPORT && F_APP_EXT_MIC_SWITCH_SUPPORT
        app_db.local_boom_mic_in = app_db.ext_mic_plugged;
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_BOOM_MIC_IN);
#endif
    }

#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
    app_ext_mic_switch_ic_mute(APP_EXT_MIC_IC_UNMUTE);
#endif

    return boom_in_level;
}

static void app_ext_mic_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_EXT_MIC_DEBOUNCE:
        {
            app_stop_timer(&timer_idx_ext_mic_io_debounce);
            app_ext_mic_gpio_detect_switch();
            app_dlps_enable(APP_DLPS_ENTER_CHECK_GPIO);
        }
        break;

    default:
        break;
    }
}

void app_ext_mic_gpio_detect_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    app_stop_timer(&timer_idx_ext_mic_io_debounce);
    app_start_timer(&timer_idx_ext_mic_io_debounce, "sensor_ld_io_debounce",
                    app_gpio_ext_mic_timer_id, APP_TIMER_EXT_MIC_DEBOUNCE, 0, false,
                    500);
}

void app_ext_mic_gpio_detect_init(void)
{
    app_timer_reg_cb(app_ext_mic_timeout_cb, &app_gpio_ext_mic_timer_id);
}

void app_ext_mic_gpio_detect_enter_dlps_pad_set(void)
{
    app_dlps_pad_wake_up_enable(BOOM_IN_DETECT_PIN);
}

void app_ext_mic_gpio_detect_exit_dlps_pad_set(void)
{
    app_dlps_restore_pad(BOOM_IN_DETECT_PIN);
}

//GPIO interrupt handler function
ISR_TEXT_SECTION void app_ext_mic_gpio_detect_intr_handler(uint32_t param)
{
    T_IO_MSG gpio_msg;
    uint8_t pinmux = BOOM_IN_DETECT_PIN;
    uint8_t gpio_level = hal_gpio_get_input_level(pinmux);
    /* Control of entering DLPS */
    app_dlps_disable(APP_DLPS_ENTER_CHECK_GPIO);

    /* Disable GPIO interrupt */
    hal_gpio_irq_disable(pinmux);

    /* Change GPIO Interrupt Polarity */
    if (gpio_level)
    {
        hal_gpio_irq_change_polarity(pinmux, GPIO_IRQ_ACTIVE_LOW); //Polarity Low
    }
    else
    {
        hal_gpio_irq_change_polarity(pinmux, GPIO_IRQ_ACTIVE_HIGH); //Polarity High
    }

    gpio_msg.type = IO_MSG_TYPE_GPIO;
    gpio_msg.subtype = IO_MSG_GPIO_EXT_MIC_IO_DETECT;

    app_io_msg_send(&gpio_msg);

    /* Enable GPIO interrupt */
    hal_gpio_irq_enable(pinmux);
}
#endif

//initialization of pinmux settings and pad settings
void app_ext_mic_gpio_board_init(void)
{
#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
    //MIC_SEL pinmux gpio
    //DIO_MUTE_PIN pinmux gpio
#endif

}

void app_ext_mic_gpio_driver_init(void)
{
#if F_APP_EXT_MIC_SWITCH_IC_SUPPORT
    hal_gpio_init_pin(MIC_SEL_PIN, GPIO_TYPE_AON, GPIO_DIR_OUTPUT, GPIO_PULL_DOWN);
    hal_gpio_set_level(MIC_SEL_PIN, GPIO_LEVEL_HIGH);

#ifdef DIO_MUTE_PIN
    hal_gpio_init_pin(DIO_MUTE_PIN, GPIO_TYPE_AON, GPIO_DIR_OUTPUT, GPIO_PULL_DOWN);
    hal_gpio_set_level(DIO_MUTE_PIN, GPIO_LEVEL_LOW);
#endif
#endif

#if F_APP_EXT_MIC_PLUG_IN_GPIO_DETECT
    //init BOOM_IN as an interrupt pin
    /* GPIO Parameter Config */
    {
        hal_gpio_init_pin(BOOM_IN_DETECT_PIN, GPIO_TYPE_AUTO, GPIO_DIR_INPUT, GPIO_PULL_UP);

        hal_gpio_set_up_irq(BOOM_IN_DETECT_PIN, GPIO_IRQ_EDGE, GPIO_IRQ_ACTIVE_LOW, true); //Polarity Low

        hal_gpio_register_isr_callback(BOOM_IN_DETECT_PIN,
                                       app_ext_mic_gpio_detect_intr_handler, 0);

        //enable int
        hal_gpio_irq_enable(BOOM_IN_DETECT_PIN);
    }
#endif
}

#endif
