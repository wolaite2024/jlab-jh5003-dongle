/**
*****************************************************************************************
*     Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file      audio_test.c
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

#include "trace.h"
#include "os_queue.h"
#include "app_mmi.h"
#include "audio_test.h"
#include "app_cfg.h"
#include "app_usb_layer.h"
#include "app_audio_path.h"

#if(AUDIO_TEST_ENABLE == 1)
/*============================================================================*
 *                         Macros
 *============================================================================*/


/*============================================================================*
 *                         Types
 *============================================================================*/


/*============================================================================*
 *                         global variable
 *============================================================================*/


/*============================================================================*
 *                         local variable
 *============================================================================*/
#if (LE_AUDIO_REF_CLK ==1)
#include "syncclk_driver.h"
#define LEA_SYNC_CLK_REF SYNCCLK_ID4
#endif


/*============================================================================*
 *                         Functions
 *============================================================================*/
static uint32_t data_int_cnt = 0;

static bool line_in_callback(uint8_t id, uint8_t event, void *p_buf,
                             uint16_t len, uint16_t frame_num)
{
    APP_PRINT_INFO3("line_in_callback: event %x, len %d,  frame_num 0x%x", event, len,
                    frame_num);
    if (event != EVENT_AUDIO_PATH_DATA_IND)
    {
        return true;
    }

    app_audio_path_fill_async(IT_LC3FRM, p_buf, len, 0, data_int_cnt * 10);

    return true;
}

void audio_test_lc3_track_line_in_init(void)
{
    struct path_iovec iv[1];
    uint8_t ids[1];
    T_AUDIO_FORMAT_INFO src;
    T_AUDIO_FORMAT_INFO snk;
    uint8_t ivn;
    memset(iv, 0, sizeof(iv));

    src.type = AUDIO_FORMAT_TYPE_PCM;
    snk.attr.pcm.sample_rate = 48000;
    snk.attr.pcm.frame_length = 240;
    snk.attr.pcm.chann_num = 2;
    snk.attr.pcm.bit_width = 16;


    snk.type = AUDIO_FORMAT_TYPE_LC3;
    snk.attr.lc3.sample_rate = 48000;
    snk.attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_FL | AUDIO_CHANNEL_LOCATION_FR;
    snk.attr.lc3.frame_length = 120;
    snk.attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
    snk.attr.lc3.presentation_delay = 10;


    iv[0].it = IT_AUX;
    iv[0].ot = OT_LC3FRM;
    iv[0].ident = "it_x_ot_lc3frm";
    iv[0].ifmt = &src;
    iv[0].ofmt = &snk;
    iv[0].uapi_cback = line_in_callback;
    iv[0].priority = 0;
    ivn = 1;

    data_int_cnt = 0;
    app_audio_path_createv(iv, ivn, ids);

#if (LE_AUDIO_REF_CLK ==1)
    syncclk_drv_timer_start(LEA_SYNC_CLK_REF, CONN_HANDLE_TYPE_FREERUN_CLOCK, 0xFF, 0);
#endif
}


static bool spk_out_callback(uint8_t id, uint8_t event, void *buf,
                             uint16_t len, uint16_t frm_num)
{
    if (event == EVENT_AUDIO_PATH_DATA_IND)
    {
        if (len)
        {
            APP_PRINT_ERROR1("spk_out_callback get len %d", len);
        }
        return true;
    }

    APP_PRINT_INFO2("spk_out_callback: id %u event %x", id, event);
    switch (event)
    {
    case EVENT_AUDIO_PATH_STREAM_STARTED:
        break;
    case EVENT_AUDIO_PATH_STREAM_STOPPED:
        break;
    case EVENT_AUDIO_PATH_READY:
        break;
    case EVENT_AUDIO_PATH_RELEASED:
        break;
    }

    return true;
}

void audio_test_lc3_track_spk_out_init(void)
{
    struct path_iovec iv[1];
    uint8_t ids[1];
    T_AUDIO_FORMAT_INFO src;
    T_AUDIO_FORMAT_INFO snk;
    uint8_t ivn;
    memset(iv, 0, sizeof(iv));

    src.type = AUDIO_FORMAT_TYPE_LC3;
    src.attr.lc3.sample_rate = 48000;
    src.attr.lc3.chann_location = AUDIO_CHANNEL_LOCATION_FL | AUDIO_CHANNEL_LOCATION_FR;
    src.attr.lc3.frame_length = 120;
    src.attr.lc3.frame_duration = AUDIO_LC3_FRAME_DURATION_10_MS;
    src.attr.lc3.presentation_delay = 10;

    snk.type = AUDIO_FORMAT_TYPE_PCM;
    snk.attr.pcm.sample_rate = 48000;
    snk.attr.pcm.frame_length = 240;
    snk.attr.pcm.chann_num = 2;
    snk.attr.pcm.bit_width = 16;


    iv[0].it = IT_LC3FRM;
    iv[0].ot = OT_SPK;
    iv[0].ident = "it_lc3_ot_udev";
    iv[0].ifmt = &src;
    iv[0].ofmt = &snk;
    iv[0].uapi_cback = spk_out_callback;
    iv[0].priority = 0;
    ivn = 1;
    app_audio_path_createv(iv, ivn, ids);
}

#endif
