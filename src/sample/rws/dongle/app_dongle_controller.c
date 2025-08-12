#if F_APP_GAMING_DONGLE_SUPPORT

/**
*****************************************************************************************
*     Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file      app_dongle_controller.c
  * @brief
  * @details
  * @author
  * @date
  * @version
  ***************************************************************************************
  * @attention
  ***************************************************************************************
  */

/*============================================================================*
*                        Header Files
*============================================================================*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "gap_br.h"
#include "section.h"
#include "ring_buffer.h"
#include "trace.h"
#include "hw_tim.h"
#include "app_main.h"
#include "app_msg.h"
#include "app_io_msg.h"
#include "app_timer.h"
#include "app_dongle_data_ctrl.h"
#include "app_dongle_common.h"
#include "app_dongle_dual_mode.h"
#include "app_report.h"
#include "app_cfg.h"

/*============================================================================*
*                         Macros
*============================================================================*/
#define CONTROLLER_HID_DEBUG        1

/* size and flushable */
#define HID_EXTRA_PARAM_SIZE        2
#define HID_MAX_DATA_SIZE           30
#define HID_TMP_BUF_SIZE            128

#define VOICE_TMP_BUF_SIZE          64

/* hid data num contain in one pkt */
#define HID_DATA_NUM_PER_PACKET     2
#define HID_REPORT_TO_DONGLE_INTERVAL           4000 //us

#define CONTROLLER_BUF_SIZE         256
#define HID_SEND_BUF_SIZE           768

#define CONTROLLER_AUDIO_JACK_PIN   P9_2
#define AUDIO_JACK_UNPLUGGED(x)     ((x) == 0)

/*============================================================================*
*                         Types
*============================================================================*/

typedef enum
{
    APP_TIMER_AUDIO_JACK_DEBOUNCE,
} T_APP_CONTROLLER_TIMER;

typedef struct
{
    uint16_t size;
    uint8_t  payload[0];
} T_APP_VOICE_DATA;

typedef struct
{
    bool     flushable;
    uint8_t  size;
    uint8_t  payload[0];
} T_APP_HID_DATA;

/*============================================================================*
*                         global variable
*============================================================================*/


/*============================================================================*
*                         local variable
*============================================================================*/
static uint8_t *hid_data_buf = NULL;
static uint8_t *voice_data_buf = NULL;

static T_RING_BUFFER hid_data_ring_buf;
static T_RING_BUFFER voice_data_ring_buf;
static T_HW_TIMER_HANDLE hid_data_timer_handle = NULL;
static uint8_t *hid_send_buf = NULL;
static uint8_t app_controller_timer_id = 0;

/*============================================================================*
*                         Functions
*============================================================================*/
RAM_TEXT_SECTION static void hid_data_timer_isr_cb(T_HW_TIMER_HANDLE handle)
{
    T_IO_MSG hid_msg;

    if (ring_buffer_get_data_count(&hid_data_ring_buf) > 0)
    {
        hid_msg.type = IO_MSG_TYPE_CONTROLLER;
        app_io_msg_send(&hid_msg);
    }
    else
    {
        hw_timer_stop(hid_data_timer_handle);
    }
}

bool app_dongle_controller_is_transmitting(void)
{
    bool hw_timer_active = false;

    hw_timer_is_active(hid_data_timer_handle, &hw_timer_active);

    return hw_timer_active;
}

bool app_dongle_controller_save_voice_data(uint8_t *data, uint16_t size)
{
    uint8_t ret = false;
    uint8_t cause = 0;
    uint8_t voice_tmp_buf[VOICE_TMP_BUF_SIZE] = {0};
    T_APP_VOICE_DATA *voice_data = (T_APP_VOICE_DATA *)voice_tmp_buf;

    if (ring_buffer_get_remaining_space(&voice_data_ring_buf) < (sizeof(T_APP_VOICE_DATA) + size))
    {
        cause = 1;
        goto exit;
    }

    if (sizeof(T_APP_VOICE_DATA) + size > VOICE_TMP_BUF_SIZE)
    {
        cause = 2;
        goto exit;
    }

    voice_data->size = size;
    memcpy(voice_data->payload, data, size);

    ring_buffer_write(&voice_data_ring_buf, (uint8_t *)voice_data, sizeof(T_APP_VOICE_DATA) + size);

    ret = true;

exit:
    if (ret == false)
    {
        APP_PRINT_ERROR1("app_dongle_controller_save_voice_data failed: -%d", cause);
    }

    return ret;
}

static uint16_t construct_hid_packet(uint8_t **buf_input, bool *is_pkt_flush)
{
    uint8_t *hid_data_num_in_buf = NULL;
    uint8_t hid_total_data_size = 0;
    uint8_t *buf = *buf_input;
    uint8_t *buf_start = buf;
    uint16_t total_pkt_size = 0;
    uint8_t data_num = 0;

    /* default is flush */
    *is_pkt_flush = true;

    /* construct hid packet: size | data | size | data */
    if (ring_buffer_get_data_count(&hid_data_ring_buf) != 0)
    {
        *buf++ = DONGLE_FORMAT_START_BIT;
        *buf++ = DONGLE_TYPE_CONTROLLER_HID_DATA;
        buf++; /* buf[2]: payload len */
        buf++; /* buf[3]: data_cnt */

        while (ring_buffer_get_data_count(&hid_data_ring_buf) > 0 && data_num <= HID_DATA_NUM_PER_PACKET)
        {
            uint8_t hid_tmp_buf[HID_TMP_BUF_SIZE] = {0};
            T_APP_HID_DATA *hid_tmp = (T_APP_HID_DATA *)hid_tmp_buf;
            T_APP_HID_DATA hid_header = {0};

            ring_buffer_peek(&hid_data_ring_buf, sizeof(T_APP_HID_DATA), (uint8_t *)&hid_header);

            if (hid_header.flushable == false)
            {
                *is_pkt_flush = false;
            }

            if (sizeof(T_APP_HID_DATA) + hid_header.size < HID_TMP_BUF_SIZE)
            {
                ring_buffer_read(&hid_data_ring_buf, sizeof(T_APP_HID_DATA) + hid_header.size, (uint8_t *)hid_tmp);

                *buf++ = hid_tmp->size;
                memcpy(buf, hid_tmp->payload, hid_tmp->size);

                buf += hid_tmp->size;
            }

            data_num++;
            hid_total_data_size += hid_tmp->size;
        }

        *buf++ = DONGLE_FORMAT_STOP_BIT;

        buf_start[2] = hid_total_data_size + data_num; /* hid_data_cnt for data size storage */
        buf_start[3] = data_num;

        total_pkt_size += hid_total_data_size + sizeof(T_GAMING_SYNC_HDR) + 1 + data_num;
    }

    *buf_input += total_pkt_size;

    return total_pkt_size;
}

bool app_dongle_controller_send_hid_buf_data(void)
{
    uint8_t cause = 0;
    uint8_t *buf = hid_send_buf;
    uint8_t *buf_start = hid_send_buf;
    bool ret = false;
    uint16_t total_pkt_size = 0;
    uint16_t hid_pkt_size = 0;
    uint16_t voice_pkt_size = 0;
    bool is_pkt_flush = false;
    uint8_t tmp_buf[64] = {0};
    T_APP_VOICE_DATA *voice_data = (T_APP_VOICE_DATA *)tmp_buf;

    if (ring_buffer_get_data_count(&voice_data_ring_buf) == 0 &&
        ring_buffer_get_data_count(&hid_data_ring_buf) == 0)
    {
        cause = 1;
        goto exit;
    }

    hid_pkt_size = construct_hid_packet(&buf, &is_pkt_flush);

    /* construct voice pkt */
    if (ring_buffer_get_data_count(&voice_data_ring_buf) > 0)
    {
        T_APP_VOICE_DATA tmp_data = {0};

        ring_buffer_peek(&voice_data_ring_buf, sizeof(T_APP_VOICE_DATA), (uint8_t *)&tmp_data);

        if ((tmp_data.size + sizeof(T_APP_VOICE_DATA)) < sizeof(tmp_buf) &&
            (tmp_data.size + hid_pkt_size) < HID_SEND_BUF_SIZE)
        {
            ring_buffer_read(&voice_data_ring_buf, sizeof(T_APP_VOICE_DATA) + tmp_data.size,
                             (uint8_t *)voice_data);

            memcpy(buf, voice_data->payload, voice_data->size);

            voice_pkt_size += voice_data->size;

            buf += voice_data->size;
        }
    }

    total_pkt_size = hid_pkt_size + voice_pkt_size;

    if (total_pkt_size == 0)
    {
        cause = 2;
        goto exit;
    }

    if (app_dongle_send_fix_channel_data(app_cfg_nv.dongle_addr, FIX_CHANNEL_CID, buf_start,
                                         total_pkt_size, is_pkt_flush))
    {
        ret = true;
    }
exit:
    if (ret == false)
    {
        APP_PRINT_ERROR1("app_dongle_controller_send_hid_buf_data failed: cause -%d", cause);
    }

    return ret;
}

RAM_TEXT_SECTION static bool write_hid_data_to_buf(uint8_t *data, uint8_t size, bool flushable)
{
    bool ret = false;
    uint8_t cause = 0;
    bool hw_timer_active = false;
    uint8_t tmp[HID_TMP_BUF_SIZE] = {0};
    T_APP_HID_DATA *hid_data = (T_APP_HID_DATA *)tmp;

    if (size > HID_MAX_DATA_SIZE)
    {
        cause = 1;
        goto exit;
    }

    if (ring_buffer_get_remaining_space(&hid_data_ring_buf) < size + sizeof(T_APP_HID_DATA))
    {
        cause = 2;
        goto exit;
    }

    if (sizeof(T_APP_HID_DATA) + size > HID_TMP_BUF_SIZE)
    {
        cause = 3;
        goto exit;
    }

    hid_data->flushable = flushable;
    hid_data->size = size;
    memcpy(hid_data->payload, data, size);

    ring_buffer_write(&hid_data_ring_buf, (uint8_t *)hid_data, sizeof(T_APP_HID_DATA) + size);

    hw_timer_is_active(hid_data_timer_handle, &hw_timer_active);

    if (hw_timer_active == false)
    {
        hw_timer_start(hid_data_timer_handle);
    }

    ret = true;

exit:
    if (ret == false)
    {
        APP_PRINT_ERROR1("write_hid_data_to_buf failed: cause -%d", cause);
    }
    else
    {
        APP_PRINT_TRACE2("write_hid_data_to_buf: size %d flush %d", size, flushable);
    }

    return ret;
}

RAM_TEXT_SECTION bool app_dongle_controller_hid_data(uint8_t *data, uint8_t size, bool flushable)
{
    return write_hid_data_to_buf(data, size, flushable);
}

static void app_controller_timer_cback(uint8_t timer_evt, uint16_t param)
{
}

void app_dongle_controller_init(void)
{
    if (hid_data_buf == NULL)
    {
        hid_data_buf = malloc(CONTROLLER_BUF_SIZE);

        ring_buffer_init(&hid_data_ring_buf, hid_data_buf, CONTROLLER_BUF_SIZE);
    }

    if (voice_data_buf == NULL)
    {
        voice_data_buf = malloc(CONTROLLER_BUF_SIZE);

        ring_buffer_init(&voice_data_ring_buf, voice_data_buf, CONTROLLER_BUF_SIZE);
    }

    if (hid_send_buf == NULL)
    {
        hid_send_buf = malloc(HID_SEND_BUF_SIZE);
    }

    hid_data_timer_handle = hw_timer_create("hid_data_timer_handle", HID_REPORT_TO_DONGLE_INTERVAL,
                                            true,
                                            hid_data_timer_isr_cb);

    app_timer_reg_cb(app_controller_timer_cback, &app_controller_timer_id);

}

#endif

