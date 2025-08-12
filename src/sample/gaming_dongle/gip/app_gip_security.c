#if F_APP_USB_GIP_SUPPORT
#include "stdlib.h"
#include <string.h>
#include "os_mem.h"
#include "bt_types.h"
#include "trace.h"
#include "platform_utils.h"
#include "rtl876x_i2c.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_gpio.h"
#include "app_timer.h"
#include "app_cfg.h"
#include "app_gip.h"
#include "app_sensor.h"
#include "app_sensor_i2c.h"
#include "app_io_msg.h"
#include "Gip.h"

#define I2C_DATA_PIN        P2_3
#define I2C_CLK_PIN         P2_1
#define RESET_PIN           P0_0

/* Register */
#define REG_DATA            0x80
#define REG_DATA_LEN        0x81
#define REG_I2C_STATE       0x82
#define REG_BASE_ADDR       0x83
#define REG_MAX_SCL_FREQU   0x84
#define REG_APP_STATE       0x90

/* Parameters */
#define BASE_ADDR           0x20
#define MAX_PACKET_SIZE     285
#define WIN_SIZE            1
#define MAX_SCL_FREQU       400
#define APP_STATE           0       //Delayed Response Time
#define I2C_STATE           0x1

/* I2C state */
#define I2C_STATE_BUSY              BIT(31)
#define I2C_STATE_RESP_RDY          BIT(30)
#define I2C_STATE_SOFT_RESET        BIT(27)
#define I2C_STATE_CONT_READ         BIT(26)
#define I2C_STATE_REP_START         BIT(25)
#define I2C_STATE_CLK_STRETCHING    BIT(24)

/* PCTR Chain flag */
#define SINGLE_PACKET       0x00
#define FIRST_PACKET        0x01
#define INTERMEDIATE_PACKET 0x02
#define LAST_PACKET         0x04

/* FCTR */
#define CONTROL_FRAME       0x80

typedef struct
{
    uint8_t ACKNR : 2;
    uint8_t FRNR : 2;
    uint8_t RFU : 1;
    uint8_t SEQCTR : 2;
    uint8_t FTYPE : 1;
} T_APP_GIP_PSCC_FCTR;


/* Protocol Timing Limits */
#define RESPONSE_TIME_OUT           100 //ms
#define IMMEDIATE_RESPONSE_TIME     RESPONSE_TIME_OUT - 2 //ms
#define GUARD_TIME                  1000 //us
#define TRANS_TIMEOUT               10 //ms
#define TRANS_REPEAT                2

/* Standard Request */
#define Set_Data         0x01
#define Get_Data         0x02
#define Set_Sleep        0x03
#define Get_Parameter    0x05
#define Get_Prod_ID      0x07
#define Get_Chip_ID      0x09
#define Set_Derived_Key  0x0C
#define Get_Derived_Key  0x0D

/* Security (TLS) Request */
#define Set_TLS          0x41
#define Get_TLS          0x42

/* Response */
#define Get_Parameter_Response   0x85
#define Get_Prod_ID_Response     0x87

/* TLS Param */
#define HOST_HELLO           0x0001
#define DEVICE_HELLO         0x0002
#define DEVICE_CERT          0x0003
#define HOST_KEY_EXCHANGE    0x0005
#define HOST_FINISH          0x0007
#define EVICE_FINISH         0x0008

typedef enum
{
    GIP_SECURITY_TIMER_ID_DELAYED_RESPONSE_TIME         = 0x01,
    GIP_SECURITY_TIMER_ID_TRANS_TIMEOUT                 = 0x02,
} T_GIP_SECURITY_TIMER_ID;

static uint8_t app_gip_security_timer_queue_id = 0;
static uint8_t delayed_response_time_timer = 0;
static uint8_t trans_time_timer = 0;

uint8_t *data_to_xhc = NULL;
uint16_t pos_data_to_xhc = 0;

bool is_pscc_sleep = false;

T_APP_GIP_PSCC_FCTR WRITE_FCTR;

uint8_t gip_puid[20] = {0x00};
uint16_t gip_vid = 0x0000;
uint16_t gip_pid = 0x0000;

static void app_gip_pscc_read(uint8_t read_addr, uint16_t read_len);


static uint16_t calcFast(uint16_t seed, uint8_t c)
{
    uint32_t h1, h2, h3, h4;
    h1 = (seed ^ c) & 0xFF;
    h2 = h1 & 0x0F;
    h3 = (h2 << 4) ^ h1;
    h4 = h3 >> 4;
    return (((((h3 << 1) ^ h4) << 4) ^ h2) << 3) ^ h4 ^ (seed >> 8);
}

static uint16_t crc16_gen(uint16_t len, uint8_t *data)
{
    uint16_t seed = 0;
    uint16_t crc16 = 0;

    for (uint16_t i = 0; i < len; i++)
    {
        seed = calcFast(seed, data[i]);
    }
    crc16 = seed;

    return crc16;
}

static void app_gip_pscc_send_msg(uint16_t subtype, uint32_t param)
{
    T_IO_MSG msg;

    msg.type = IO_MSG_TYPE_DONGLE_APP;
    msg.subtype = subtype;
    msg.u.param = param;

    app_io_msg_send(&msg);
}

static void app_gip_pscc_timer_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_INFO2("app_gip_pscc_timer_cback: timer_id %d, timer_channel %d",
                    timer_id, timer_chann);

    switch (timer_id)
    {
    case GIP_SECURITY_TIMER_ID_DELAYED_RESPONSE_TIME:
        {
            app_stop_timer(&delayed_response_time_timer);

            app_gip_pscc_send_msg(GIP_READ_I2C_STATE, 4);
        }
        break;

    case GIP_SECURITY_TIMER_ID_TRANS_TIMEOUT:
        {
            app_stop_timer(&trans_time_timer);

#if 0
            if (repeat_trans_time < TRANS_REPEAT)
            {
                /* re-transmit */
                repeat_trans_time++;
            }
            else
            {
                app_gip_pscc_warm_reset();
                repeat_trans_time = 0;
            }
#endif
        }
        break;
    }
}

void app_gip_pscc_start_trans_time_timer(void)
{
    app_stop_timer(&trans_time_timer);
    app_start_timer(&trans_time_timer, "trans_time",
                    app_gip_security_timer_queue_id, GIP_SECURITY_TIMER_ID_TRANS_TIMEOUT, 0, false,
                    TRANS_TIMEOUT);
}

void app_gip_pscc_handle_data_from_pscc(uint16_t len, uint8_t *data)
{
    APP_PRINT_INFO2("app_gip_pscc_handle_data_from_pscc: len %d, data %b", len, TRACE_BINARY(len,
                    data));

    /* Data Link Layer */
    T_APP_GIP_PSCC_FCTR READ_FCTR;
    uint16_t LEN = (uint16_t)data[1] << 8 | (uint16_t)data[2];
    uint16_t CSUM = (uint16_t)data[len - 2] << 8 | (uint16_t)data[len - 1];
    uint8_t ack_to_pscc[6];

    /* Transport Layer */
    uint8_t PCTR = data[3];

    /* Application Layer */
    //uint8_t bCmd = 0x00;
    //uint16_t wParam = 0x0000;
    uint16_t wLength = 0x0000;
    uint8_t *p_data_payload = NULL;
    uint16_t data_payload_len = 0;

    uint16_t data_to_xhc_total_len = 0;
    bool ready_to_send_to_xhc = false;

    if (crc16_gen(len - 2, data) != CSUM)
    {
        APP_PRINT_ERROR0("app_gip_pscc_handle_data_from_pscc: checksum error!");
        return;
    }

    /* check frame type */
    memcpy(&READ_FCTR, &data[0], 1);

#if 0
    APP_PRINT_TRACE5("app_gip_pscc_handle_data_from_pscc: FCTR 0x%02x, LEN 0x%04x, PCTR 0x%02x, FTYPE %d, CONTROL_FRAME 0x%02x",
                     READ_FCTR, LEN, PCTR, READ_FCTR.FTYPE, data[0] & CONTROL_FRAME);
#endif

    if (data[0] & CONTROL_FRAME)
    {
        app_gip_pscc_read(REG_APP_STATE, 0);
        return;
    }

    /* Ack to PSCC */
    WRITE_FCTR.ACKNR = READ_FCTR.FRNR;

    ack_to_pscc[0] = REG_DATA;
    ack_to_pscc[1] = CONTROL_FRAME | READ_FCTR.FRNR;
    ack_to_pscc[2] = 0x00;
    ack_to_pscc[3] = 0x00;
    CSUM = crc16_gen(3, &ack_to_pscc[1]);
    ack_to_pscc[4] = (CSUM >> 8) & 0xff;
    ack_to_pscc[5] = CSUM & 0xff;

    platform_delay_us(GUARD_TIME); // GT: Gaurd Time at lease 250us

    app_sensor_i2c_write(BASE_ADDR, ack_to_pscc, sizeof(ack_to_pscc));

#if 0
    APP_PRINT_TRACE2("app_gip_pscc_handle_data_from_pscc: len %d, ack_to_pscc %b", sizeof(ack_to_pscc),
                     TRACE_BINARY(sizeof(ack_to_pscc), ack_to_pscc));
#endif

    /* check the packet is single or multi */
    if (PCTR == SINGLE_PACKET)
    {
        if (data[4] == Get_Prod_ID_Response)
        {
            uint8_t *p_APDU = &data[9];

            memcpy(gip_puid, &p_APDU[0], 20);
            gip_vid = p_APDU[20] << 8 | p_APDU[21];
            gip_pid = p_APDU[22] << 8 | p_APDU[23];

            app_gip_handle_security_info(gip_pid, gip_vid, gip_puid);

#if 0
            APP_PRINT_TRACE3("app_gip_pscc_handle_data_from_pscc: gip_vid 0x%x, gip_pid 0x%x, gip_puid %b",
                             gip_vid, gip_pid, TRACE_BINARY(20,
                                                            gip_puid));
#endif
            return;
        }

        if (LEN)
        {
            p_data_payload = &data[3];

            data_to_xhc = (uint8_t *)malloc(LEN);
            memcpy(&data_to_xhc[0], p_data_payload, LEN);
            pos_data_to_xhc += LEN;
        }

        ready_to_send_to_xhc = true;
    }
    else if (PCTR == FIRST_PACKET)
    {
        //bCmd = data[4];
        //wParam = data[5] << 8 | data[6];
        wLength = data[7] << 8 | data[8];
        p_data_payload = &data[4];
        data_payload_len = LEN - 1; //LEN - APDU overhead(6)(PCTR, bCmd, wParam, wLength)

        data_to_xhc = (uint8_t *)malloc(wLength + 6);

        data_to_xhc[0] = 0x00;
        pos_data_to_xhc++;

        memcpy(&data_to_xhc[1], p_data_payload, data_payload_len);
        pos_data_to_xhc += data_payload_len;
    }
    else if (PCTR == INTERMEDIATE_PACKET)
    {
        p_data_payload = &data[4];
        data_payload_len = LEN - 1; //LEN - PCTR(1)

        memcpy(&data_to_xhc[pos_data_to_xhc], p_data_payload, data_payload_len);
        pos_data_to_xhc += data_payload_len;
    }
    else if (PCTR == LAST_PACKET)
    {
        p_data_payload = &data[4];
        data_payload_len = LEN - 1; //LEN - PCTR(1)

        memcpy(&data_to_xhc[pos_data_to_xhc], p_data_payload, data_payload_len);
        pos_data_to_xhc += data_payload_len;

        ready_to_send_to_xhc = true;
    }
    else
    {
        /* error case */
    }

    /* Response data to USB interface */
    if (ready_to_send_to_xhc)
    {
        data_to_xhc_total_len = pos_data_to_xhc;

        if (data_to_xhc_total_len)
        {
            APP_PRINT_TRACE2("app_gip_pscc_handle_data_from_pscc: send to usb interface, len %d, data %b",
                             data_to_xhc_total_len,
                             TRACE_BINARY(data_to_xhc_total_len, data_to_xhc));

            GipSysSendSecurity(data_to_xhc, data_to_xhc_total_len);
        }

        pos_data_to_xhc = 0;
    }
    else
    {
        /* Read I2C_STATE again */
        app_gip_pscc_send_msg(GIP_READ_I2C_STATE, 4);
    }


    /* Read I2C_STATE again */
    //app_gip_pscc_send_msg(GIP_READ_I2C_STATE, 4);
}

void app_gip_pscc_handle_data_from_xhc(uint16_t len, uint8_t *data_from_xhc)
{
    uint8_t cmd = data_from_xhc[1];
    uint16_t payload_len = 0;

    if (cmd == Set_TLS)
    {
        payload_len = (uint16_t)data_from_xhc[4] << 8 | (uint16_t)data_from_xhc[5];
        len = payload_len + 6;
    }
    else if (cmd == Get_TLS)
    {
        payload_len = 0;
        len = 6;
    }

    uint16_t frame_len = len + 6; // REG(1) + FCTR(1) + LEN(2) + CSUM(2)
    uint8_t *frame_to_pscc = (uint8_t *)malloc(frame_len);

    uint16_t CSUM = 0x0000;

    frame_to_pscc[0] = REG_DATA;
    memcpy(&frame_to_pscc[1], &WRITE_FCTR, 1);
    frame_to_pscc[2] = (len >> 8) & 0xff;
    frame_to_pscc[3] = len & 0xff;
    memcpy(&frame_to_pscc[4], data_from_xhc, len);

    CSUM = crc16_gen(frame_len - 3, &frame_to_pscc[1]); // from FCTR
    frame_to_pscc[frame_len - 2] = (CSUM >> 8) & 0xff;
    frame_to_pscc[frame_len - 1] = CSUM & 0xff;

    APP_PRINT_TRACE2("app_gip_pscc_handle_data_from_xhc: frame_len %d, frame_to_pscc %b", frame_len,
                     TRACE_BINARY(frame_len, frame_to_pscc));

    /* Write frame to PSCC */
    if (app_sensor_i2c_write(BASE_ADDR, frame_to_pscc, frame_len) != I2C_Success)
    {
        APP_PRINT_ERROR0("app_gip_pscc_handle_data_from_xhc: i2c write fail !");
        goto EXIT;
    }

    WRITE_FCTR.FRNR++;

    /* Read I2C_STATE */
    app_gip_pscc_send_msg(GIP_READ_I2C_STATE, 4);

    /* Start trans time timer */
    app_gip_pscc_start_trans_time_timer();

EXIT:
    os_mem_free(frame_to_pscc);
}

static void app_gip_pscc_read(uint8_t read_addr, uint16_t read_len)
{
    platform_delay_us(GUARD_TIME); // GT: Gaurd Time at lease 250us

    int ret = 0;

    //APP_PRINT_TRACE2("app_gip_pscc_read: read_addr 0x%02x, read_len 0x%04x", read_addr, read_len);

    switch (read_addr)
    {
    case REG_I2C_STATE:
        {
            uint8_t read_buf[4] = {0};
            uint32_t i2c_state;
            bool BUSY = false;
            bool RESP_RDY = false;
            uint16_t RESP_LEN = 0x0000;

            /* Read I2C_STATE register */
            if (app_sensor_i2c_read_32(BASE_ADDR, REG_I2C_STATE, read_buf) != I2C_Success)
            {
                ret = -1;
                goto EXIT;
            }

            /* stop transmit timeout timer */
            app_stop_timer(&trans_time_timer);

            BE_ARRAY_TO_UINT32(i2c_state, read_buf);

            BUSY = (i2c_state & I2C_STATE_BUSY) ? 1 : 0;
            RESP_RDY = (i2c_state & I2C_STATE_RESP_RDY) ? 1 : 0;
            RESP_LEN = (uint16_t)i2c_state;

            APP_PRINT_TRACE3("app_gip_pscc_read: BUSY %d, RESP_RDY %d, RESP_LEN %d", BUSY, RESP_RDY, RESP_LEN);

            if (BUSY == 1 && RESP_RDY == 0)
            {
                /* PSCC busy, keep polling I2C_STATE */
                app_gip_pscc_send_msg(GIP_READ_I2C_STATE, 4);
            }
            else if (BUSY == 1 && RESP_RDY == 1)
            {
                /* PSCC still busy, but PSCC want to ack */
                //app_gip_pscc_read(REG_DATA, RESP_LEN);

                app_gip_pscc_send_msg(GIP_READ_DATA, (uint32_t)RESP_LEN);
            }
            else if (BUSY == 0 && RESP_RDY == 1)
            {
                /* PSCC process complete, read data_reg */
                //app_gip_pscc_read(REG_DATA, RESP_LEN);

                app_gip_pscc_send_msg(GIP_READ_DATA, (uint32_t)RESP_LEN);
            }
            else
            {
                /* Initial state */
                static uint8_t cnt = 0;

                if (cnt < 5)
                {
                    app_gip_pscc_send_msg(GIP_READ_I2C_STATE, 4);
                    cnt++;
                }
                else
                {
                    cnt = 0;
                }
            }
        }
        break;

    case REG_APP_STATE:
        {
            uint32_t delayed_response_time = 0;
            uint8_t read_buf[4] = {0};

            if (app_sensor_i2c_read_32(BASE_ADDR, REG_APP_STATE,
                                       read_buf) != I2C_Success)
            {
                ret = -2;
                goto EXIT;
            }

            BE_ARRAY_TO_UINT32(delayed_response_time, read_buf);

            APP_PRINT_TRACE1("app_gip_pscc_read: delayed_response_time %d ms", delayed_response_time);

            //if (app_state & BIT(31)) //valid register data contents
            if (delayed_response_time)
            {
                app_start_timer(&delayed_response_time_timer, "delayed_response_time",
                                app_gip_security_timer_queue_id, GIP_SECURITY_TIMER_ID_DELAYED_RESPONSE_TIME, 0, false,
                                delayed_response_time);
            }
            else
            {
                app_gip_pscc_send_msg(GIP_READ_I2C_STATE, 4);
            }
        }
        break;

    case REG_DATA:
        {
            if (read_len)
            {
                uint8_t *read_buf = (uint8_t *)malloc(read_len);

                if (app_sensor_i2c_read(BASE_ADDR, REG_DATA, read_buf, read_len) != I2C_Success)
                {
                    ret = -3;
                    goto EXIT;
                }

                app_gip_pscc_handle_data_from_pscc(read_len, read_buf);

                os_mem_free(read_buf);
            }
        }
        break;

    default:
        {
            ret = -4;
        }
        break;
    }

EXIT:
    if (ret)
    {
        APP_PRINT_ERROR1("app_gip_pscc_read: read data fail, ret %d", ret);
    }
}

void app_gip_pscc_handle_app_msg(T_IO_MSG *io_driver_msg_recv)
{
    uint16_t event = io_driver_msg_recv->subtype;
    uint32_t read_len = io_driver_msg_recv->u.param;

    if (event == GIP_READ_DATA)
    {
        app_gip_pscc_read(REG_DATA, read_len);
    }
    else if (event == GIP_READ_I2C_STATE)
    {
        app_gip_pscc_read(REG_I2C_STATE, read_len);
    }
}

void app_gip_pscc_enter_sleep(void)
{
    /* stop clock */
    RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, DISABLE);

    is_pscc_sleep = true;
}

void app_gip_pscc_wake_up()
{
    /* start clock */
    RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);

    is_pscc_sleep = false;
}

void app_gip_pscc_warm_reset(void)
{
    APP_PRINT_ERROR0("app_gip_pscc_warm_reset");

    /* stop clock */
    RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, DISABLE);

    /* reset pin pull low 800us */
    Pad_Config(RESET_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    platform_delay_us(800); // t_RST
    Pad_Config(RESET_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    /* t_STARTUP */
    platform_delay_ms(10);

    /* start clock again */
    RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);
}

void app_gip_pscc_board_init(void)
{
    Pinmux_Config(I2C_DATA_PIN, I2C0_DAT);
    Pinmux_Config(I2C_CLK_PIN, I2C0_CLK);

    Pad_Config(I2C_DATA_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(I2C_CLK_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(RESET_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pad_PullConfigValue(I2C_DATA_PIN, PAD_STRONG_PULL);
    Pad_PullConfigValue(I2C_CLK_PIN, PAD_STRONG_PULL);
    Pad_PullConfigValue(RESET_PIN, PAD_STRONG_PULL);
}

uint16_t app_gip_pscc_get_vendor_id(void)
{
    return gip_vid;
}

uint16_t app_gip_pscc_get_product_id(void)
{
    return gip_pid;
}

uint8_t *app_gip_pscc_get_puid(void)
{
    return gip_puid;
}

void app_gip_pscc_read_prod_id(void)
{
    //DBG_DIRECT("app_gip_pscc_read_prod_id");

    uint8_t read_prod_id_cmd[] = {0x00, Get_Prod_ID, 0x00, 0x00, 0x00, 0x1B};
    app_gip_pscc_handle_data_from_xhc(sizeof(read_prod_id_cmd), read_prod_id_cmd);
}

void app_gip_pscc_init(void)
{
    if (app_gip_get_switch_mode())
    {
        app_cfg_const.i2c_0_dat_pinmux = I2C_DATA_PIN;
        app_cfg_const.i2c_0_clk_pinmux = I2C_CLK_PIN;

        app_gip_pscc_warm_reset();

        app_gip_pscc_board_init();

        app_sensor_i2c_init(BASE_ADDR, DEF_I2C_CLK_SPD, false);

        app_timer_reg_cb(app_gip_pscc_timer_cback, &app_gip_security_timer_queue_id);

        GipSysSecurityExternalHandlerRegister(app_gip_pscc_handle_data_from_xhc);

        platform_delay_ms(15); //first transmission may start earliest 10 ms after power-up

        app_gip_pscc_read_prod_id();
    }
}
#endif
