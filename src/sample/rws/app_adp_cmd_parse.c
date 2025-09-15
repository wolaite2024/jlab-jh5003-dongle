#if F_APP_ADP_5V_CMD_SUPPORT
#include "trace.h"
#include "stdlib.h"
#include <string.h>
#include "app_dlps.h"
#include "hw_tim.h"
#include "app_timer.h"
#include "app_io_msg.h"
#include "section.h"
#include "hal_adp.h"
#include "app_adp_cmd_parse.h"
#include "app_cfg.h"

// Guard BIT, now we just use(6 bits), Spec define 8 bits.
#define GUARD_BIT 0x3F

/*40Mhz 40 devide, set 1ms period=1000 */
#define PERIOD_ADP_TIM      2000000 // 2s

static T_ADP_CMD_PARSE_STRUCT *p_adp_cmd_data = NULL;
static T_HW_TIMER_HANDLE adp_hw_timer_handle = NULL;
static T_ADP_CMD_PARSE_PARAMETER cmd_parse_para;
static P_ADP_CMD_RAW_DATA_PARSE_CBACK adp_cmd_raw_data_parse_callback;

static uint8_t app_adp_cmd_parse_timer_id = 0;
static uint8_t timer_idx_adp_cmd_protect = 0;

typedef struct
{
    uint32_t stop_bit: 1;
    uint32_t parity_bit: 1;
    uint32_t cmd_index: 6;
    uint32_t start_bit: 1;
    uint32_t reserved_bit: 23;
} T_APP_ADP_CMDPACK09;

typedef struct
{
    uint32_t stop_bit: 1;
    uint32_t parity_bit: 1;
    uint32_t payload: 8;
    uint32_t cmd_index: 4;
    uint32_t start_bit: 1;
    uint32_t reserved_bit: 17;
} T_APP_ADP_CMDPACK15;

typedef union
{
    uint32_t bit_data;
    T_APP_ADP_CMDPACK09 cmdpack09;
    T_APP_ADP_CMDPACK15 cmdpack15;
} T_APP_ADP_CMD_DECODE;

typedef enum
{
    APP_TIMER_ADP_CMD_PROTECT,
} T_APP_ADP_CMD_PARSE_TIMER;

void app_adp_cmd_parse_handle_msg(uint32_t cmd_data)
{
    if (adp_cmd_raw_data_parse_callback)
    {
        adp_cmd_raw_data_parse_callback(cmd_data); // to app_smart_chargerbox_handle_cmd_raw_data
    }
}

static uint8_t app_adp_cmd_parse_parity_count(uint32_t data, uint8_t bit_num)
{
    uint8_t parity_count = 0;

    for (int i = 0; i < bit_num; i++)
    {
        if (((data >> i) & BIT(0)) == 1)
        {
            parity_count++;
        }
    }
    return parity_count;
}

/**
* @brief: Process bit data, currently we use odd parity check
*
*   Payload of CMD_9
*   |bit 8  |bit 7  |bit 6  |bit 5  |bit 4  |bit 3  |bit 2  |bit 1  |bit 0  |
*   |START  |                     Data(6bits)               |PARITY |END    |
*
*   Payload of CMD_15
*   |bit 14 |bit 13 |               ~               |bit 2  |bit 1  |bit 0  |
*   |START  |                     Data(12bits)              |PARITY |END    |
*
*/
bool app_adp_cmd_parse_process(uint32_t cmd_data, uint8_t *p_cmd_index, uint8_t *p_cmd_payload)
{
    uint8_t cmd_index = 0;
    uint8_t payload = 0;
    uint8_t cmd_parity_cnt = 0;
    uint32_t cmd_guard = 0;
    bool ret = false;

    /* Handle adp_dat to app cmd here. */
    switch (app_cfg_const.smart_charger_box_cmd_set)
    {
    case CHARGER_BOX_CMD_SET_9BITS:
        {
            T_APP_ADP_CMDPACK09 cmd_pack09;

            memcpy(&cmd_pack09, &cmd_data, sizeof(uint32_t));

            cmd_index = cmd_pack09.cmd_index;
            payload = 0;
            cmd_parity_cnt += app_adp_cmd_parse_parity_count(cmd_index, 6);
            cmd_parity_cnt += cmd_pack09.parity_bit;

            if ((cmd_pack09.start_bit == 0) && (cmd_pack09.stop_bit == 0) &&
                (app_adp_cmd_parse_parity_count(cmd_parity_cnt, 1) == 1))
            {
                cmd_guard = (cmd_data >> 9) & GUARD_BIT;

                if (cmd_guard == GUARD_BIT)
                {
                    *p_cmd_index = cmd_pack09.cmd_index;
                    *p_cmd_payload = payload;
                    ret = true;
                }
            }
        }
        break;

    case CHARGER_BOX_CMD_SET_15BITS:
        {
            T_APP_ADP_CMDPACK15 cmd_pack15;

            memcpy(&cmd_pack15, &cmd_data, sizeof(uint32_t));
            APP_PRINT_INFO5("app_adp_cmd_parse_process: cmd_pack15: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
                            cmd_pack15.start_bit, cmd_pack15.cmd_index, cmd_pack15.payload,
                            cmd_pack15.parity_bit, cmd_pack15.stop_bit);

            cmd_index = cmd_pack15.cmd_index;
            payload = cmd_pack15.payload;
            cmd_parity_cnt += app_adp_cmd_parse_parity_count(cmd_index, 4);
            cmd_parity_cnt += app_adp_cmd_parse_parity_count(payload, 8);
            cmd_parity_cnt += cmd_pack15.parity_bit;

            if ((cmd_pack15.start_bit == 0) && (cmd_pack15.stop_bit == 0) &&
                (app_adp_cmd_parse_parity_count(cmd_parity_cnt, 1) == 1))
            {
                cmd_guard = (cmd_data >> 15) & GUARD_BIT;

                if (cmd_guard == GUARD_BIT)
                {
                    *p_cmd_index = cmd_pack15.cmd_index;
                    *p_cmd_payload = cmd_pack15.payload;
                    ret = true;
                }
            }
        }
        break;

    default:
        break;
    }

    APP_PRINT_INFO6("app_adp_cmd_parse_process: cmd_bits: 0x%x, cmd_data: 0x%x, cmd_guard: 0x%08x, cmd_index:0x%x payload: 0x%x, ret:%d",
                    app_cfg_const.smart_charger_box_cmd_set, cmd_data, cmd_guard, *p_cmd_index, *p_cmd_payload, ret);

    if (*p_cmd_index == 0xFF)
    {
        ret = false;
    }

    return ret;
}

/**
* @brief: Reset bit process data
*
*/
RAM_TEXT_SECTION static void app_adp_cmd_parse_data_parse_reset(bool usb_dis)
{
    APP_PRINT_INFO1("app_adp_cmd_parse_data_parse_reset usb_dis = %d", usb_dis);
    memset(p_adp_cmd_data, 0, sizeof(T_ADP_CMD_PARSE_STRUCT));
    p_adp_cmd_data->cmd_out = 0xFFFFFFFF;

    if (!usb_dis)
    {
        p_adp_cmd_data->usb_in_cmd_det = true ;
        p_adp_cmd_data->usb_start_wait_en = 1;
        p_adp_cmd_data->usb_cmd_rec = true;
    }
}

bool app_adp_cmd_parse_get_usb_cmd_rec(void)
{
    return p_adp_cmd_data->usb_cmd_rec;
}

void app_adp_cmd_parse_handle_usb_cmd(uint8_t cmd, uint8_t payload)
{
    APP_PRINT_INFO2("app_adp_cmd_parse_handle_usb_cmd AD2B_CMD_USB(in=0x0a05) = 0x%x usb_in_cmd_det_already =%d",
                    (cmd << 8) | payload, p_adp_cmd_data->usb_in_cmd_det);

    if (payload == ADP_USB_IN_PLAYLOAD) //usb_in
    {
        if ((p_adp_cmd_data->usb_in_cmd_det == true) ||
            (p_adp_cmd_data->tim_cnt_finish == true)) //second detect usb in command and wait adp timer finished
        {
            APP_PRINT_INFO0("app_adp_cmd_parse_handle_usb_cmd USB START"); // second usb in command get usb start
            /*start usb clear flag*/
            app_adp_cmd_parse_data_parse_reset(1);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);

            p_adp_cmd_data->usb_started = true;

#if F_APP_LOCAL_PLAYBACK_SUPPORT //& F_APP_ERWS_SUPPORT
            if (app_cfg_const.local_playback_support && (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SINGLE))
            {
                app_adp_usb_start_handle();
            }
#endif
        }
        else
        {
            /*first usb cmd get clear flag*/
            app_adp_cmd_parse_data_parse_reset(0);

        }
    }
    else
    {
#if F_APP_LOCAL_PLAYBACK_SUPPORT //& F_APP_ERWS_SUPPORT
        if (app_cfg_const.local_playback_support && (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SINGLE))
        {
            p_adp_cmd_data->usb_started = false;
            app_adp_usb_stop_handle();
        }
#endif
    }

    //@lemon add usb start code here,payload 0x05:usb in   0x70:usb out
}

static void app_adp_cmd_parse_send_io_msg(uint32_t adp_cmd_raw_data)
{
    T_IO_MSG adp_msg;

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_ADAPTOR_DAT;
    adp_msg.u.param = adp_cmd_raw_data;

    app_io_msg_send(&adp_msg);
}

RAM_TEXT_SECTION void app_adp_cmd_parse_protect_send_msg(void)
{
    T_IO_MSG adp_msg;

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_SMARTBOX_COMMAND_PROTECT;
    adp_msg.u.param = 0;

    app_io_msg_send(&adp_msg);
}

void app_adp_cmd_parse_protect(void)
{
    app_start_timer(&timer_idx_adp_cmd_protect, "adp_cmd_protect",
                    app_adp_cmd_parse_timer_id, APP_TIMER_ADP_CMD_PROTECT, 0, false,
                    6000);
}

void app_adp_cmd_parse_int_handle(T_IO_MSG *io_driver_msg_recv)
{
    T_ADP_INT_DATA adp_data;
    uint8_t bit_cnt = 0;

    memcpy(&adp_data, &io_driver_msg_recv->u.param, sizeof(adp_data));

    if (adp_data.tim_delta_value <= (14 * 1000)) // pules 10ms  15bit 9bit plus is 10ms
    {
        hw_timer_stop(adp_hw_timer_handle);
        app_adp_cmd_parse_data_parse_reset(1);
        app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);
        return;
    }
    else
    {
        bit_cnt = (adp_data.tim_delta_value + cmd_parse_para.cmd_max_inaccuracy) /
                  (cmd_parse_para.cmd_bit_width * 1000);

        if (bit_cnt >= MAX_BIT_CNT)
        {
            bit_cnt = 16;
        }

        for (uint8_t i = 0; i < bit_cnt; i++)
        {
            p_adp_cmd_data->cmd_out <<= 1;
            p_adp_cmd_data->cmd_out |= (!adp_data.bit_data);
            p_adp_cmd_data->total_bit_pushed++;
        }

        app_adp_cmd_parse_send_io_msg(p_adp_cmd_data->cmd_out);

#if (ADP_CMD_DBG == 1)
        APP_PRINT_INFO3("app_adp_cmd_parse_int_handle: push [%d] x %d, total_bit_pushed %d",
                        (~adp_data.bit_data) & 0x01,
                        bit_cnt,
                        p_adp_cmd_data->total_bit_pushed);
#endif

    }
}
/**
 * @brief ADP DET peripheral interrupt handler function.
 *
 */
RAM_TEXT_SECTION static void app_adp_cmd_parse_of_chargerbox_handler(void)
{
    if ((app_cfg_const.ntc_power_by_pad_direct) && (app_cfg_const.thermistor_power_pinmux != 0xFF))
    {
        Pad_OutputControlValue(app_cfg_const.thermistor_power_pinmux, PAD_OUT_HIGH);
    }

    // This is an protection, to avoid cmd receive failed.
    if (!(app_dlps_get_dlps_bitmap() & APP_DLPS_ENTER_CHECK_CMD_PROTECT))
    {
        app_dlps_disable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);
        app_adp_cmd_parse_protect_send_msg();
    }

    uint32_t tim_current_value;
    T_ADP_INT_DATA adp_data;

    adp_data.bit_data = adp_get_level(ADP_DETECT_5V);

    /* The HW timer must be started in this interrupt handler,
       otherwise the first bit of cmd will not be detected */
    if (p_adp_cmd_data->hw_timer_started == false)
    {
        if (adp_data.bit_data == 0) //start HW timer when first high to low interrupt
        {
            if (p_adp_cmd_data->usb_in_cmd_det == true)
            {
                app_adp_cmd_parse_data_parse_reset(0);
            }
            else
            {
                app_adp_cmd_parse_data_parse_reset(1);
            }

            p_adp_cmd_data->hw_timer_started = true;
            hw_timer_restart(adp_hw_timer_handle, cmd_parse_para.cmd_detect_timeout);
            p_adp_cmd_data->tim_prev_value = cmd_parse_para.cmd_detect_timeout;

            APP_PRINT_INFO0("app_adp_cmd_parse_of_chargerbox_handler: adp timer start");
        }
        else
        {
            app_adp_cmd_parse_data_parse_reset(1);
            APP_PRINT_INFO0("app_adp_cmd_parse_of_chargerbox_handler: not valid start signal");
        }
        return;
    }
#if (ADP_CMD_DBG == 1)
    APP_PRINT_INFO3("app_adp_cmd_parse_of_chargerbox_handler:adp_hw_timer_handle  0x %x, cmd_parse_para.cmd_detect_timeout %d p_adp_cmd_data->tim_prev_value =%d",
                    adp_hw_timer_handle,
                    cmd_parse_para.cmd_detect_timeout,
                    p_adp_cmd_data->tim_prev_value);
#endif

    /* caculate deltaT between two interrupt */
    hw_timer_get_current_count(adp_hw_timer_handle, &tim_current_value);

    adp_data.tim_delta_value = p_adp_cmd_data->tim_prev_value - tim_current_value;

    APP_PRINT_INFO4("app_adp_cmd_parse_of_chargerbox_handler: bit_data [%d], timer = %d->%d, deltaT = %d",
                    adp_data.bit_data, p_adp_cmd_data->tim_prev_value, tim_current_value, adp_data.tim_delta_value);

    p_adp_cmd_data->tim_prev_value = tim_current_value;

    if ((adp_data.tim_delta_value >= cmd_parse_para.cmd_guard_time) &&
        (adp_data.bit_data == 0)) //guard_bit_got
    {
        p_adp_cmd_data->hw_timer_started = true;
        hw_timer_restart(adp_hw_timer_handle, cmd_parse_para.cmd_detect_timeout);
        p_adp_cmd_data->tim_prev_value = cmd_parse_para.cmd_detect_timeout;
    }

    /* send adp level and deltaT to app task */
    T_IO_MSG adp_msg;

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_ADP_INT;
    memcpy(&adp_msg.u.param, &adp_data, sizeof(adp_msg.u.param));

    app_io_msg_send(&adp_msg);
}

void app_adp_cmd_parse_hw_timer_handler(void)
{
    APP_PRINT_TRACE2("app_adp_cmd_parse_hw_timer_handler: total_bit_pushed %d, cmd_bit_num %d",
                     p_adp_cmd_data->total_bit_pushed,
                     cmd_parse_para.cmd_bit_num);

    if (p_adp_cmd_data->total_bit_pushed < cmd_parse_para.cmd_bit_num)
    {
        uint8_t need_fill_bit_cnt = cmd_parse_para.cmd_bit_num - p_adp_cmd_data->total_bit_pushed;

        p_adp_cmd_data->cmd_out = (p_adp_cmd_data->cmd_out << need_fill_bit_cnt) |
                                  ((1 << need_fill_bit_cnt) - 1);

        app_adp_cmd_parse_send_io_msg(p_adp_cmd_data->cmd_out);
    }

    app_adp_cmd_parse_data_parse_reset(1);
}

/**
 * @brief ADP DET TIMx interrupt handler function.
 *
 */
RAM_TEXT_SECTION void app_adp_cmd_parse_timer_isr_cb(T_HW_TIMER_HANDLE handle)
{
    hw_timer_stop(adp_hw_timer_handle);

    T_IO_MSG adp_msg;

    adp_msg.type = IO_MSG_TYPE_GPIO;
    adp_msg.subtype = IO_MSG_GPIO_ADP_HW_TIMER_HANDLER;

    app_io_msg_send(&adp_msg);

#if F_APP_LOCAL_PLAYBACK_SUPPORT //& F_APP_ERWS_SUPPORT
    if (app_cfg_const.local_playback_support && (app_cfg_const.bud_role != REMOTE_SESSION_ROLE_SINGLE))
    {
        if (p_adp_cmd_data->usb_start_wait_en == 1)
        {
            // second usb in command not get  start usb
            APP_PRINT_INFO0("app_adp_cmd_parse_timer_isr_cb: USB START");
            p_adp_cmd_data->usb_start_wait_en = 0;
            p_adp_cmd_data->tim_cnt_finish = true;
            T_IO_MSG adp_msg;
            adp_msg.type = IO_MSG_TYPE_GPIO;
            adp_msg.subtype = IO_MSG_GPIO_ADAPTOR_DAT;
            adp_msg.u.param = ADP_USB_IN_PRAR;

            app_io_msg_send(&adp_msg);
            hw_timer_restart(adp_hw_timer_handle, PERIOD_ADP_TIM);
            return;
        }
    }
#endif
}

static void app_adp_cmd_parse_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_ADP_CMD_PROTECT:
        {
            app_stop_timer(&timer_idx_adp_cmd_protect);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);
        }
        break;

    default:
        break;
    }
}

void app_adp_cmd_parse_para_set(uint8_t cmd_bit_num, uint8_t cmd_bit_length,
                                P_ADP_CMD_RAW_DATA_PARSE_CBACK cback)
{
    cmd_parse_para.cmd_bit_num = cmd_bit_num;
    cmd_parse_para.cmd_bit_width = cmd_bit_length;

    if (cmd_bit_length == 20)
    {
        cmd_parse_para.cmd_guard_time = (8 * cmd_bit_length * 1000) - GUART_BIT_TIM_TOLERANCE / 2;
    }
    else if (cmd_bit_length == 40)
    {
        if (cmd_bit_num == 9)
        {
            cmd_parse_para.cmd_guard_time = (8 * cmd_bit_length * 1000) - GUART_BIT_TIM_TOLERANCE / 2;
        }
        else if (cmd_bit_num == 15)
        {
            cmd_parse_para.cmd_guard_time = 2 * (8 * cmd_bit_length * 1000) - GUART_BIT_TIM_TOLERANCE;
        }
    }

    cmd_parse_para.cmd_detect_timeout = (cmd_bit_num + 2) * cmd_bit_length * 1000;
    /*ota tooling cmd one bit meet less than 16ms */
    cmd_parse_para.cmd_max_inaccuracy = 5000;

    adp_cmd_raw_data_parse_callback = cback;
#if (ADP_CMD_DBG == 1)
    APP_PRINT_INFO3("app_adp_cmd_parse_para_set:cmd_bit_num  = %d,cmd_bit_length =%d, cmd_parse_para.cmd_detect_timeout =%d ",
                    cmd_bit_num, cmd_bit_length,
                    cmd_parse_para.cmd_detect_timeout);
#endif
}

static void app_adp_cmd_parse_int_init(void)
{
    adp_update_isr_cb(ADP_DETECT_5V, (P_ADP_ISR_CBACK)app_adp_cmd_parse_of_chargerbox_handler);
}

/**
 * @brief contains the initialization of adaptor det timer and ADP_DET_IRQn settings.
 *
 */
static void app_adp_cmd_parse_timer_init(void)
{
    adp_hw_timer_handle = hw_timer_create("app_adp_cmd_parse", cmd_parse_para.cmd_detect_timeout, false,
                                          app_adp_cmd_parse_timer_isr_cb);
#if (ADP_CMD_DBG == 1)
    APP_PRINT_INFO2("app_adp_cmd_parse_timer_init:adp_hw_timer_handle  0x %x, cmd_parse_para.cmd_detect_timeout =%d ",
                    adp_hw_timer_handle,
                    cmd_parse_para.cmd_detect_timeout);
#endif
    if (adp_hw_timer_handle == NULL)
    {
        APP_PRINT_ERROR0("adp cmd HW timer create failed !");
    }
    else
    {
        hw_timer_lpm_set(adp_hw_timer_handle, true);
    }
}

static void app_adp_cmd_parse_para_init(P_ADP_CMD_RAW_DATA_PARSE_CBACK cmd_handle)
{
    uint8_t cmd_bit_num = 0;
    uint8_t cmd_bit_length = 0;

    /* bit number */
    if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_9BITS)
    {
        cmd_bit_num = 9;
    }
    else if (app_cfg_const.smart_charger_box_cmd_set == CHARGER_BOX_CMD_SET_15BITS)
    {
        cmd_bit_num = 15;
    }

    /* bit length */
    if (app_cfg_const.smart_charger_box_bit_length == CHARGER_BOX_BIT_LENGTH_20MS)
    {
        cmd_bit_length = 20;
    }
    else if (app_cfg_const.smart_charger_box_bit_length == CHARGER_BOX_BIT_LENGTH_40MS)
    {
        cmd_bit_length = 40;
    }
    app_adp_cmd_parse_para_set(cmd_bit_num, cmd_bit_length, cmd_handle);
}

static bool app_adp_cmd_parse_enable(void)
{
    bool ret = false;

    if (app_cfg_const.enable_rtk_charging_box)
    {
        ret = true;
    }
    else
    {
#if F_APP_OTA_TOOLING_SUPPORT
        ret = true;
#endif
    }

    return ret;
}

void app_adp_cmd_parse_init(P_ADP_CMD_RAW_DATA_PARSE_CBACK cmd_handle)
{
    if (app_adp_cmd_parse_enable())
    {
        app_adp_cmd_parse_para_init(cmd_handle);

        if (p_adp_cmd_data == NULL)
        {
            p_adp_cmd_data = (T_ADP_CMD_PARSE_STRUCT *)calloc(1, sizeof(T_ADP_CMD_PARSE_STRUCT));
        }
        else
        {
            memset(p_adp_cmd_data, 0, sizeof(T_ADP_CMD_PARSE_STRUCT));
        }

        app_adp_cmd_parse_timer_init();
        app_adp_cmd_parse_int_init();
        app_adp_cmd_parse_data_parse_reset(1);

        app_timer_reg_cb(app_adp_cmd_parse_timeout_cb, &app_adp_cmd_parse_timer_id);
        app_dlps_enable(APP_DLPS_ENTER_CHECK_CMD_PROTECT);
    }
}
#endif
