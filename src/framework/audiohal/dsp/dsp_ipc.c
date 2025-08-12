/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "dsp_shm.h"
#include "dsp_ipc.h"
#include "dsp_driver.h"

#define BIT(_n)             (uint32_t)(1U << (_n))

#define DIPC_H2D_HEAD_LEN                   (4)

/* DIPC Stream skip len */
#define DIPC_STREAM_SKIP_LEN(s, len)     {              \
        s += len;                                       \
    }

/* DIPC Stream to array */
#define DIPC_STREAM_TO_ARRAY(a, s, len)  {              \
        uint32_t ii;                                    \
        for (ii = 0; ii < len; ii++)                    \
        {                                               \
            *((uint8_t *)(a) + ii) = *s++;              \
        }                                               \
    }

/* DIPC Array to stream */
#define DIPC_ARRAY_TO_STREAM(s, a, len)  {              \
        uint32_t ii;                                    \
        for (ii = 0; ii < len; ii++)                    \
        {                                               \
            *s++ = *((uint8_t *)(a) + ii);              \
        }                                               \
    }

/* DIPC Little Endian stream to uint8 */
#define DIPC_LE_STREAM_TO_UINT8(u8, s)   {              \
        u8  = (uint8_t)(*s);                            \
        s  += 1;                                        \
    }

/* DIPC Little Endian stream to uint16 */
#define DIPC_LE_STREAM_TO_UINT16(u16, s) {              \
        u16  = ((uint16_t)(*(s + 0)) << 0) +            \
               ((uint16_t)(*(s + 1)) << 8);             \
        s   += 2;                                       \
    }

/* DIPC Little Endian stream to uint24 */
#define DIPC_LE_STREAM_TO_UINT24(u24, s) {              \
        u24  = ((uint32_t)(*(s + 0)) <<  0) +           \
               ((uint32_t)(*(s + 1)) <<  8) +           \
               ((uint32_t)(*(s + 2)) << 16);            \
        s   += 3;                                       \
    }

/* DIPC Little Endian stream to uint32 */
#define DIPC_LE_STREAM_TO_UINT32(u32, s) {              \
        u32  = ((uint32_t)(*(s + 0)) <<  0) +           \
               ((uint32_t)(*(s + 1)) <<  8) +           \
               ((uint32_t)(*(s + 2)) << 16) +           \
               ((uint32_t)(*(s + 3)) << 24);            \
        s   += 4;                                       \
    }

/* DIPC Little Endian uint8 to stream */
#define DIPC_LE_UINT8_TO_STREAM(s, u8)   {              \
        *s++ = (uint8_t)(u8);                           \
    }

/* DIPC Little Endian uint16 to stream */
#define DIPC_LE_UINT16_TO_STREAM(s, u16) {              \
        *s++ = (uint8_t)((u16) >> 0);                   \
        *s++ = (uint8_t)((u16) >> 8);                   \
    }

/* DIPC Little Endian uint24 to stream */
#define DIPC_LE_UINT24_TO_STREAM(s, u24) {              \
        *s++ = (uint8_t)((u24) >>  0);                  \
        *s++ = (uint8_t)((u24) >>  8);                  \
        *s++ = (uint8_t)((u24) >> 16);                  \
    }

/* DIPC Little Endian uint32 to stream */
#define DIPC_LE_UINT32_TO_STREAM(s, u32) {              \
        *s++ = (uint8_t)((u32) >>  0);                  \
        *s++ = (uint8_t)((u32) >>  8);                  \
        *s++ = (uint8_t)((u32) >> 16);                  \
        *s++ = (uint8_t)((u32) >> 24);                  \
    }

/* DIPC Little Endian array to uint8 */
#define DIPC_LE_ARRAY_TO_UINT8(u8, a)    {              \
        u8  = (uint8_t)(*(a + 0));                      \
    }

/* DIPC Little Endian array to uint16 */
#define DIPC_LE_ARRAY_TO_UINT16(u16, a)  {              \
        u16 = ((uint16_t)(*(a + 0)) << 0) +             \
              ((uint16_t)(*(a + 1)) << 8);              \
    }

/* DIPC Little Endian array to uint24 */
#define DIPC_LE_ARRAY_TO_UINT24(u24, a)  {              \
        u24 = ((uint32_t)(*(a + 0)) <<  0) +            \
              ((uint32_t)(*(a + 1)) <<  8) +            \
              ((uint32_t)(*(a + 2)) << 16);             \
    }

/* DIPC Little Endian array to uint32 */
#define DIPC_LE_ARRAY_TO_UINT32(u32, a) {               \
        u32 = ((uint32_t)(*(a + 0)) <<  0) +            \
              ((uint32_t)(*(a + 1)) <<  8) +            \
              ((uint32_t)(*(a + 2)) << 16) +            \
              ((uint32_t)(*(a + 3)) << 24);             \
    }

/* DIPC Little Endian uint8 to array */
#define DIPC_LE_UINT8_TO_ARRAY(a, u8)    {              \
        *((uint8_t *)(a) + 0) = (uint8_t)(u8);          \
    }

/* DIPC Little Endian uint16 to array */
#define DIPC_LE_UINT16_TO_ARRAY(a, u16)  {              \
        *((uint8_t *)(a) + 0) = (uint8_t)((u16) >> 0);  \
        *((uint8_t *)(a) + 1) = (uint8_t)((u16) >> 8);  \
    }

/* DIPC Little Endian uint24 to array */
#define DIPC_LE_UINT24_TO_ARRAY(a, u24) {               \
        *((uint8_t *)(a) + 0) = (uint8_t)((u24) >>  0); \
        *((uint8_t *)(a) + 1) = (uint8_t)((u24) >>  8); \
        *((uint8_t *)(a) + 2) = (uint8_t)((u24) >> 16); \
    }

/* DIPC Little Endian uint32 to array */
#define DIPC_LE_UINT32_TO_ARRAY(a, u32) {               \
        *((uint8_t *)(a) + 0) = (uint8_t)((u32) >>  0); \
        *((uint8_t *)(a) + 1) = (uint8_t)((u32) >>  8); \
        *((uint8_t *)(a) + 2) = (uint8_t)((u32) >> 16); \
        *((uint8_t *)(a) + 3) = (uint8_t)((u32) >> 24); \
    }

/* DIPC Big Endian stream to uint8 */
#define DIPC_BE_STREAM_TO_UINT8(u8, s)   {              \
        u8   = (uint8_t)(*(s + 0));                     \
        s   += 1;                                       \
    }

/* DIPC Big Endian stream to uint16 */
#define DIPC_BE_STREAM_TO_UINT16(u16, s) {              \
        u16  = ((uint16_t)(*(s + 0)) << 8) +            \
               ((uint16_t)(*(s + 1)) << 0);             \
        s   += 2;                                       \
    }

/* DIPC Big Endian stream to uint24 */
#define DIPC_BE_STREAM_TO_UINT24(u24, s) {              \
        u24  = ((uint32_t)(*(s + 0)) << 16) +           \
               ((uint32_t)(*(s + 1)) <<  8) +           \
               ((uint32_t)(*(s + 2)) <<  0);            \
        s   += 3;                                       \
    }

/* DIPC Big Endian stream to uint32 */
#define DIPC_BE_STREAM_TO_UINT32(u32, s) {              \
        u32  = ((uint32_t)(*(s + 0)) << 24) +           \
               ((uint32_t)(*(s + 1)) << 16) +           \
               ((uint32_t)(*(s + 2)) <<  8) +           \
               ((uint32_t)(*(s + 3)) <<  0);            \
        s   += 4;                                       \
    }

/* DIPC Big Endian uint8 to stream */
#define DIPC_BE_UINT8_TO_STREAM(s, u8)   {              \
        *s++ = (uint8_t)(u8);                           \
    }

/* DIPC Big Endian uint16 to stream */
#define DIPC_BE_UINT16_TO_STREAM(s, u16) {              \
        *s++ = (uint8_t)((u16) >> 8);                   \
        *s++ = (uint8_t)((u16) >> 0);                   \
    }

/* DIPC Big Endian uint24 to stream */
#define DIPC_BE_UINT24_TO_STREAM(s, u24) {              \
        *s++ = (uint8_t)((u24) >> 16);                  \
        *s++ = (uint8_t)((u24) >>  8);                  \
        *s++ = (uint8_t)((u24) >>  0);                  \
    }

/* DIPC Big Endian uint32 to stream */
#define DIPC_BE_UINT32_TO_STREAM(s, u32) {              \
        *s++ = (uint8_t)((u32) >> 24);                  \
        *s++ = (uint8_t)((u32) >> 16);                  \
        *s++ = (uint8_t)((u32) >>  8);                  \
        *s++ = (uint8_t)((u32) >>  0);                  \
    }

/* DIPC Big Endian array to uint8 */
#define DIPC_BE_ARRAY_TO_UINT8(u8, a)    {              \
        u8  = (uint8_t)(*(a + 0));                      \
    }

/* DIPC Big Endian array to uint16 */
#define DIPC_BE_ARRAY_TO_UINT16(u16, a)  {              \
        u16 = ((uint16_t)(*(a + 0)) << 8) +             \
              ((uint16_t)(*(a + 1)) << 0);              \
    }

/* DIPC Big Endian array to uint24 */
#define DIPC_BE_ARRAY_TO_UINT24(u24, a)  {              \
        u24 = ((uint32_t)(*(a + 0)) << 16) +            \
              ((uint32_t)(*(a + 1)) <<  8) +            \
              ((uint32_t)(*(a + 2)) <<  0);             \
    }

/* DIPC Big Endian array to uint32 */
#define DIPC_BE_ARRAY_TO_UINT32(u32, a)  {              \
        u32 = ((uint32_t)(*(a + 0)) << 24) +            \
              ((uint32_t)(*(a + 1)) << 16) +            \
              ((uint32_t)(*(a + 2)) <<  8) +            \
              ((uint32_t)(*(a + 3)) <<  0);             \
    }

/* DIPC Big Endian uint8 to array */
#define DIPC_BE_UINT8_TO_ARRAY(a, u8)    {              \
        *((uint8_t *)(a) + 0) = (uint8_t)(u8);          \
    }

/* DIPC Big Endian uint16 to array */
#define DIPC_BE_UINT16_TO_ARRAY(a, u16)  {              \
        *((uint8_t *)(a) + 0) = (uint8_t)((u16) >> 8);  \
        *((uint8_t *)(a) + 1) = (uint8_t)((u16) >> 0);  \
    }

/* DIPC Big Endian uint24 to array */
#define DIPC_BE_UINT24_TO_ARRAY(a, u24)  {              \
        *((uint8_t *)(a) + 0) = (uint8_t)((u24) >> 16); \
        *((uint8_t *)(a) + 1) = (uint8_t)((u24) >>  8); \
        *((uint8_t *)(a) + 2) = (uint8_t)((u24) >>  0); \
    }

/* DIPC Big Endian uint32 to array */
#define DIPC_BE_UINT32_TO_ARRAY(a, u32)  {              \
        *((uint8_t *)(a) + 0) = (uint8_t)((u32) >> 24); \
        *((uint8_t *)(a) + 1) = (uint8_t)((u32) >> 16); \
        *((uint8_t *)(a) + 2) = (uint8_t)((u32) >>  8); \
        *((uint8_t *)(a) + 3) = (uint8_t)((u32) >>  0); \
    }

#define WORD_TO_BYTE_LENGTH         4
#define H2D_HEADER_LEN              (4)
#define MAX_H2D_PAYLOAD_LEN         (508)

#define RWS_SPK2_AUDIO_SYNC_INITIAL         0x00
#define RWS_SPK2_AUDIO_SYNC_UNLOCK          0x01
#define RWS_SPK2_AUDIO_SYNC_LOCK            0x02
#define RWS_SPK2_AUDIO_SYNC_V2_SUCCESSFUL   0x03
#define RWS_SPK2_AUDIO_SYNC_EMPTY           0xFF
#define RWS_SPK2_AUDIO_SYNC_LOSE_TIMESTAMP  0xFE
#define RWS_SPK2_AUDIO_SYNC_FAIL            0xFD
#define RWS_START_BTCLK_EXPIRED             0xF2

typedef struct t_phy_codec_state
{
    uint8_t state;
    uint8_t path;
    uint8_t rsv[2];
} T_PHY_CODEC_STATE;

typedef struct t_dsp_ipc_cback_elem
{
    struct t_dsp_ipc_cback_elem *next;
    P_DSP_IPC_CBACK              cback;
} T_DSP_IPC_CBACK_ELEM;

typedef struct t_dsp_ipc_db
{
    T_OS_QUEUE cback_list;
} T_DSP_IPC_DB;

static bool dsp_ipc_event_parser(uint16_t cmd_id, uint8_t *param, uint16_t payload_length);
static bool dipc_event_parser(uint16_t event, uint8_t *payload, uint16_t payload_length);

static T_DSP_IPC_DB *dsp_ipc_db = NULL;

static bool dsp_ipc_h2d_cmd_send(uint8_t *cmd_buf, uint32_t cmd_buf_len, bool flush)
{
    return h2d_cmd_send(cmd_buf, cmd_buf_len, flush);
}

bool dsp_ipc_set_latency_report(uint8_t param)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_SET_STREAM_LATENCY_REPORT);
    cmd_buf[1] = (uint8_t)(H2D_SET_STREAM_LATENCY_REPORT >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_plc_notify(uint16_t interval, uint32_t threshold, bool enable)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 12;
    uint8_t cmd_buf[12] = {0};

    cmd_buf[0] = (uint8_t)(H2D_DECODER_PLC_SET);
    cmd_buf[1] = (uint8_t)(H2D_DECODER_PLC_SET >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = enable;
    cmd_buf[5] = (uint8_t)(interval);
    cmd_buf[6] = (uint8_t)(interval >> 8);
    cmd_buf[7] = (uint8_t)(threshold);
    cmd_buf[8] = (uint8_t)(threshold >> 8);
    cmd_buf[9] = (uint8_t)(threshold >> 16);
    cmd_buf[10] = (uint8_t)(threshold >> 24);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}
bool dsp_ipc_set_low_latency_mode(uint8_t param)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_LOW_LATENCY_MODE);
    cmd_buf[1] = (uint8_t)(H2D_LOW_LATENCY_MODE >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_fifo_len(T_DSP_FIFO_TRX_DIRECTION direction, uint8_t lower_len)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_FIFO_REQUEST);
    cmd_buf[1] = (uint8_t)(H2D_FIFO_REQUEST >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = direction;
    cmd_buf[5] = 0;
    cmd_buf[6] = lower_len;
    cmd_buf[7] = 0;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_decode_emtpy(bool enable)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_FIFO_EMPTY_REPORT_SET);
    cmd_buf[1] = (uint8_t)(H2D_FIFO_EMPTY_REPORT_SET >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = enable;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_data_mode(T_DSP_DATA_MODE data_mode, uint8_t stream_type)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_DATA_MODE);
    cmd_buf[1] = (uint8_t)(H2D_DATA_MODE >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = data_mode;
    cmd_buf[5] = stream_type;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_boot_config(uint8_t param)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_BOOT_CONFIG);
    cmd_buf[1] = (uint8_t)(H2D_BOOT_CONFIG >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_tone_gain(int16_t left_gain, int16_t right_gain)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8];

    cmd_buf[0] = (uint8_t)(H2D_TONE_GAIN);
    cmd_buf[1] = (uint8_t)(H2D_TONE_GAIN >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)left_gain;
    cmd_buf[5] = (uint8_t)(left_gain >> 8);
    cmd_buf[6] = (uint8_t)right_gain;
    cmd_buf[7] = (uint8_t)(right_gain >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_dac_gain(int16_t left_gain, int16_t right_gain)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8];

    cmd_buf[0] = (uint8_t)(H2D_DAC_GAIN_STEREO);
    cmd_buf[1] = (uint8_t)(H2D_DAC_GAIN_STEREO >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)(left_gain);
    cmd_buf[5] = (uint8_t)(left_gain >> 8);
    cmd_buf[6] = (uint8_t)(right_gain);
    cmd_buf[7] = (uint8_t)(right_gain >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_apt_dac_gain(int16_t left_gain, int16_t right_gain)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8];

    cmd_buf[0] = (uint8_t)(H2D_APT_DAC_GAIN);
    cmd_buf[1] = (uint8_t)(H2D_APT_DAC_GAIN >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)left_gain;
    cmd_buf[5] = (uint8_t)(left_gain >> 8);
    cmd_buf[6] = (uint8_t)(right_gain);
    cmd_buf[7] = (uint8_t)(right_gain >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_voice_adc_gain(int16_t left_gain, int16_t right_gain)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8];

    cmd_buf[0] = (uint8_t)(H2D_VOICE_ADC_POST_GAIN);
    cmd_buf[1] = (uint8_t)(H2D_VOICE_ADC_POST_GAIN >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)left_gain;
    cmd_buf[5] = (uint8_t)(left_gain >> 8);
    cmd_buf[6] = (uint8_t)(right_gain);
    cmd_buf[7] = (uint8_t)(right_gain >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_record_adc_gain(int16_t left_gain, int16_t right_gain)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8];

    //cmd_buf[0] = (uint8_t)(H2D_RECORD_ADC_GAIN);
    //cmd_buf[1] = (uint8_t)(H2D_RECORD_ADC_GAIN >> 8);
    cmd_buf[0] = (uint8_t)(H2D_VOICE_ADC_POST_GAIN);
    cmd_buf[1] = (uint8_t)(H2D_VOICE_ADC_POST_GAIN >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)left_gain;
    cmd_buf[5] = (uint8_t)(left_gain >> 8);
    cmd_buf[6] = (uint8_t)(right_gain);
    cmd_buf[7] = (uint8_t)(right_gain >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_aux_adc_gain(int16_t left_gain, int16_t right_gain)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8];

    cmd_buf[0] = (uint8_t)(H2D_AUX_ADC_GAIN);
    cmd_buf[1] = (uint8_t)(H2D_AUX_ADC_GAIN >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)left_gain;
    cmd_buf[5] = (uint8_t)(left_gain >> 8);
    cmd_buf[6] = (uint8_t)(right_gain);
    cmd_buf[7] = (uint8_t)(right_gain >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_synchronization_data_send(uint8_t *buf, uint16_t len)
{
    uint32_t cmd_length = len / WORD_TO_BYTE_LENGTH;
    uint32_t cmd_buf_len = (cmd_length + 1) * WORD_TO_BYTE_LENGTH;
    uint8_t cmd_buf[24] = {0};

    cmd_buf[0] = (uint8_t)(H2D_B2BMSG_INTERACTION_RECEIVED);
    cmd_buf[1] = (uint8_t)(H2D_B2BMSG_INTERACTION_RECEIVED >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    memcpy(&cmd_buf[4], buf, len);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_b2bmsg_interaction_timeout(uint8_t param)
{
    uint32_t cmd_length = 0;
    uint32_t cmd_buf_len = 4;
    uint8_t cmd_buf[4];

    cmd_buf[0] = (uint8_t)(H2D_B2BMSG_INTERACTION_TIMEOUT);
    cmd_buf[1] = (uint8_t)(H2D_B2BMSG_INTERACTION_TIMEOUT >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_power_down(uint8_t param)
{
    uint32_t cmd_length = 0;
    uint32_t cmd_buf_len = 4;
    uint8_t cmd_buf[4];

    cmd_buf[0] = (uint8_t)(H2D_DSP_POWER_DOWN);
    cmd_buf[1] = (uint8_t)(H2D_DSP_POWER_DOWN >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_dsp_config(uint32_t param, bool flush)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_DSP_CONFIG);
    cmd_buf[1] = (uint8_t)(H2D_DSP_CONFIG >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)(param);
    cmd_buf[5] = (uint8_t)(param >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, flush);
}

bool dsp_ipc_set_dsp2_config(uint32_t param, bool flush)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 0;
    uint8_t cmd_buf[8];

    cmd_buf_len = ((cmd_length + 1) * WORD_TO_BYTE_LENGTH);

    memset(cmd_buf, 0, cmd_buf_len);
    cmd_buf[0] = (uint8_t)(H2D_SET_DSP2_CLK);
    cmd_buf[1] = (uint8_t)(H2D_SET_DSP2_CLK >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)(param);
    cmd_buf[5] = (uint8_t)(param >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, flush);
}

bool dsp_ipc_set_decoder(T_DSP_IPC_DECODER param, bool flush)
{
    uint32_t cmd_length = 2;
    uint32_t cmd_buf_len = 12;
    uint8_t cmd_buf[12] = {0};

    DIPC_PRINT_TRACE6("dsp_ipc_set_decoder: algorithm 0x%02X sub_type 0x%02X chann_mode 0x%02X sample_rate %d bit_res 0x%02X samples_per_frame 0x%04X",
                      param.algorithm, param.sub_type, param.chann_mode, param.sample_rate, param.bit_res,
                      param.samples_per_frame);

    cmd_buf[0] = (uint8_t)H2D_DECODER_SET;
    cmd_buf[1] = (uint8_t)(H2D_DECODER_SET >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);

    *((uint32_t *)(&cmd_buf[4])) = ((param.samples_per_frame)
                                    | (param.chann_mode << 13)
                                    | (param.algorithm << 16)
                                    | (param.bit_res << 21)
                                    | (param.sub_type << 27));

    cmd_buf[8] = (uint8_t)(param.sample_rate);
    cmd_buf[9] = (uint8_t)(param.sample_rate >> 8);
    cmd_buf[10] = (uint8_t)(param.sample_rate >> 16);
    cmd_buf[11] = (uint8_t)(param.sample_rate >> 24);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, flush);
}

bool dsp_ipc_set_encoder(T_DSP_IPC_ENCODER param, bool flush)
{
    uint32_t cmd_length;
    uint32_t cmd_buf_len;
    uint8_t cmd_buf[16] = {0};

    DIPC_PRINT_TRACE6("dsp_ipc_set_encoder: algorithm 0x%02X, sub_type 0x%02X, chann_mode 0x%02X,"
                      " sample_rate %d, bit_res 0x%02X, samples_per_frame 0x%04X",
                      param.algorithm, param.sub_type, param.chann_mode, param.sample_rate, param.bit_res,
                      param.samples_per_frame);

    if (param.algorithm == ALGORITHM_OPUS_AUDIO)
    {
        cmd_length = 3;
        cmd_buf_len = 16;

        cmd_buf[0] = (uint8_t)H2D_ENCODER_SET;
        cmd_buf[1] = (uint8_t)(H2D_ENCODER_SET >> 8);
        cmd_buf[2] = (uint8_t)(cmd_length);
        cmd_buf[3] = (uint8_t)(cmd_length >> 8);

        *((uint32_t *)(&cmd_buf[4])) = ((param.samples_per_frame)
                                        | (param.chann_mode << 13)
                                        | (param.algorithm << 16)
                                        | (param.bit_res << 21));

        cmd_buf[8] = (uint8_t)(param.sample_rate);
        cmd_buf[9] = (uint8_t)(param.sample_rate >> 8);
        cmd_buf[10] = (uint8_t)(param.sample_rate >> 16);
        cmd_buf[11] = (uint8_t)(param.sample_rate >> 24);

        *((uint32_t *)(&cmd_buf[12])) = ((param.opus_bitrate)
                                         | (param.opus_cbr << 20)
                                         | (param.opus_cvbr << 21)
                                         | (param.opus_complexity << 22)
                                         | (param.opus_mode << 26)
                                         | (param.opus_application << 28));
    }
    else
    {
        cmd_length = 2;
        cmd_buf_len = 12;

        cmd_buf[0] = (uint8_t)H2D_ENCODER_SET;
        cmd_buf[1] = (uint8_t)(H2D_ENCODER_SET >> 8);
        cmd_buf[2] = (uint8_t)(cmd_length);
        cmd_buf[3] = (uint8_t)(cmd_length >> 8);

        *((uint32_t *)(&cmd_buf[4])) = (param.samples_per_frame)
                                       | (param.chann_mode << 13)
                                       | (param.algorithm << 16)
                                       | (param.bit_res << 21)
                                       | ((param.sub_type & 0x03) << 27);

        cmd_buf[8] = (uint8_t)(param.sample_rate);
        cmd_buf[9] = (uint8_t)(param.sample_rate >> 8);
        cmd_buf[10] = (uint8_t)(param.sample_rate >> 16);
        cmd_buf[11] = (uint8_t)(param.sample_rate >> 24);
    }

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, flush);
}

bool dsp_ipc_set_lc3_decoder(T_DSP_IPC_DECODER2 param, T_DSP_IPC_LC3_DECODER lc3_param)
{
    uint32_t cmd_length = 11;
    uint32_t cmd_buf_len = 48;
    uint8_t cmd_buf[48];

    DIPC_PRINT_TRACE3("dsp_ipc_set_lc3_decoder: frame_duration 0x%02X, octets_per_frame 0x%04X, "
                      " plc_method 0x%02X", lc3_param.frame_duration, lc3_param.octets_per_frame,
                      lc3_param.plc_method);

    cmd_buf[0] = (uint8_t)H2D_DECODER_SET2;
    cmd_buf[1] = (uint8_t)(H2D_DECODER_SET2 >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    memcpy(&cmd_buf[4], &param, 16);
    memcpy(&cmd_buf[20], &lc3_param, 28);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_lc3_encoder(T_DSP_IPC_ENCODER2 param, T_DSP_IPC_LC3_ENCODER lc3_param)
{
    uint32_t cmd_length = 11;
    uint32_t cmd_buf_len = 48;
    uint8_t cmd_buf[48];

    DIPC_PRINT_TRACE2("dsp_ipc_set_lc3_encoder: frame_duration 0x%02X, octets_per_frame 0x%04X",
                      lc3_param.frame_duration, lc3_param.octets_per_frame);

    cmd_buf[0] = (uint8_t)H2D_ENCODER_SET2;
    cmd_buf[1] = (uint8_t)(H2D_ENCODER_SET2 >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    memcpy(&cmd_buf[4], &param, 16);
    memcpy(&cmd_buf[20], &lc3_param, 28);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_send_h2d_param(uint16_t cmd_id, uint8_t *payload_data, uint16_t payload_len)
{
    uint8_t  *cmd_buf;
    uint16_t offset = 0;
    uint16_t cmd_len = 0;
    uint16_t pedding_len = 0;

    cmd_buf = os_mem_zalloc2(512);

    if (cmd_buf == NULL)
    {
        return false;
    }

    while (offset + MAX_H2D_PAYLOAD_LEN < payload_len)
    {
        cmd_len = MAX_H2D_PAYLOAD_LEN;

        cmd_buf[0] = cmd_id & 0xFF;
        cmd_buf[1] = (cmd_id & 0xFF00) >> 8;
        cmd_buf[2] = (cmd_len / WORD_TO_BYTE_LENGTH) & 0xFF;
        cmd_buf[3] = ((cmd_len / WORD_TO_BYTE_LENGTH) & 0xFF00) >> 8;

        memcpy(cmd_buf + H2D_HEADER_LEN, payload_data + offset, MAX_H2D_PAYLOAD_LEN);
        offset += MAX_H2D_PAYLOAD_LEN;

        if ((dsp_ipc_h2d_cmd_send(cmd_buf, H2D_HEADER_LEN + MAX_H2D_PAYLOAD_LEN, true)) != true)
        {
            os_mem_free(cmd_buf);
            return false;
        }
    }

    cmd_len = payload_len - offset;
    pedding_len = (cmd_len % 4) ? (4 - (cmd_len % 4)) : 0;
    cmd_len += pedding_len;

    cmd_buf[0] = cmd_id & 0xFF;
    cmd_buf[1] = (cmd_id & 0xFF00) >> 8;
    cmd_buf[2] = (cmd_len / WORD_TO_BYTE_LENGTH) & 0xFF;
    cmd_buf[3] = ((cmd_len / WORD_TO_BYTE_LENGTH) & 0xFF00) >> 8;

    memset(cmd_buf + H2D_HEADER_LEN, 0, cmd_len);
    memcpy(cmd_buf + H2D_HEADER_LEN, payload_data + offset, cmd_len - pedding_len);

    if ((dsp_ipc_h2d_cmd_send(cmd_buf, H2D_HEADER_LEN + cmd_len, true)) != true)
    {
        os_mem_free(cmd_buf);
        return false;
    }

    os_mem_free(cmd_buf);
    return true;
}

bool dsp_ipc_wdrc_effect_set(uint8_t *payload_data,
                             uint16_t payload_len)
{
    uint8_t  *cmd_buf;
    uint16_t  cmd_len;

    cmd_len = (H2D_HEADER_LEN + payload_len + 3) / 4;

    cmd_buf = os_mem_zalloc2(cmd_len * 4);
    if (cmd_buf == NULL)
    {
        return false;
    }

    cmd_buf[0] = H2D_HA_PARA & 0xFF;
    cmd_buf[1] = (H2D_HA_PARA & 0xFF00) >> 8;
    cmd_buf[2] = cmd_len;
    cmd_buf[3] = cmd_len >> 8;
    memcpy(&cmd_buf[4], payload_data, payload_len);

    if (dsp_ipc_h2d_cmd_send(cmd_buf, cmd_len * 4, true) == false)
    {
        os_mem_free(cmd_buf);
        return false;
    }

    os_mem_free(cmd_buf);
    return true;
}

bool dsp_ipc_audio_wdrc_set(uint8_t *payload_data,
                            uint16_t payload_len)
{
    return dsp_ipc_wdrc_effect_set(payload_data, payload_len);
}

bool dsp_ipc_voice_wdrc_set(uint8_t *payload_data,
                            uint16_t payload_len)
{
    return dsp_ipc_wdrc_effect_set(payload_data, payload_len);
}

bool dsp_ipc_apt_wdrc_set(uint8_t *payload_data,
                          uint16_t payload_len)
{
    return dsp_ipc_wdrc_effect_set(payload_data, payload_len);
}

bool dsp_ipc_audio_eq_set(void *p_data, uint16_t len)
{
    uint8_t *p_cmd;
    uint16_t cmd_len = (len + H2D_HEADER_LEN + 3) / 4;

    DIPC_PRINT_TRACE2("dsp_ipc_audio_eq_set: p_data %p, len 0x%04x", p_data, len);

    p_cmd = os_mem_zalloc2(cmd_len * 4);
    if (p_cmd != NULL)
    {
        p_cmd[0] = (uint8_t)H2D_AUDIO_EQ_PARA;
        p_cmd[1] = (uint8_t)(H2D_AUDIO_EQ_PARA >> 8);
        p_cmd[2] = cmd_len; //eq_data + dsp_definition
        p_cmd[3] = cmd_len >> 8;

        memcpy(&p_cmd[H2D_HEADER_LEN], p_data, len);

        if (dsp_ipc_h2d_cmd_send(p_cmd, cmd_len * 4, true) == false)
        {
            os_mem_free(p_cmd);
            return false;
        }

        os_mem_free(p_cmd);
        return true;
    }

    return false;
}

bool dsp_ipc_audio_eq_clear(void)
{
    uint8_t cmd_buf[H2D_HEADER_LEN];

    cmd_buf[0] = (uint8_t)H2D_AUDIO_EQ_PARA;
    cmd_buf[1] = (uint8_t)(H2D_AUDIO_EQ_PARA >> 8);
    cmd_buf[2] = 0x01;
    cmd_buf[3] = 0;

    return dsp_ipc_h2d_cmd_send(cmd_buf, H2D_HEADER_LEN, true);
}

bool dsp_ipc_voice_eq_set(void *p_data, uint16_t len)
{
    uint8_t *p_cmd;
    uint16_t cmd_len = (len + H2D_HEADER_LEN + 3) / 4;

    DIPC_PRINT_TRACE2("dsp_ipc_voice_eq_set: p_data %p, len 0x%04x", p_data, len);

    p_cmd = os_mem_zalloc2(cmd_len * 4);
    if (p_cmd != NULL)
    {
        p_cmd[0] = (uint8_t)H2D_VOICE_EQ_PARA;
        p_cmd[1] = (uint8_t)(H2D_VOICE_EQ_PARA >> 8);
        p_cmd[2] = cmd_len; //eq_data + dsp_definition
        p_cmd[3] = cmd_len >> 8;

        memcpy(&p_cmd[H2D_HEADER_LEN], p_data, len);

        if (dsp_ipc_h2d_cmd_send(p_cmd, cmd_len * 4, true) == false)
        {
            os_mem_free(p_cmd);
            return false;
        }

        os_mem_free(p_cmd);
        return true;
    }

    return false;
}

bool dsp_ipc_voice_eq_clear(void)
{
    uint8_t cmd_buf[H2D_HEADER_LEN];

    cmd_buf[0] = (uint8_t)H2D_VOICE_EQ_PARA;
    cmd_buf[1] = (uint8_t)(H2D_VOICE_EQ_PARA >> 8);
    cmd_buf[2] = 0x01;
    cmd_buf[3] = 0;

    return dsp_ipc_h2d_cmd_send(cmd_buf, H2D_HEADER_LEN, true);
}

bool dsp_ipc_record_eq_set(void *p_data, uint16_t len)
{
    uint8_t *p_cmd;
    uint16_t cmd_len = (len + H2D_HEADER_LEN + 3) / 4;

    DIPC_PRINT_TRACE2("dsp_ipc_record_eq_set: p_data %p, len 0x%04x", p_data, len);

    p_cmd = os_mem_zalloc2(cmd_len * 4);
    if (p_cmd != NULL)
    {
        p_cmd[0] = (uint8_t)H2D_RECORD_EQ_PARA;
        p_cmd[1] = (uint8_t)(H2D_RECORD_EQ_PARA >> 8);
        p_cmd[2] = cmd_len; //eq_data + dsp_definition
        p_cmd[3] = cmd_len >> 8;

        memcpy(&p_cmd[H2D_HEADER_LEN], p_data, len);

        if (dsp_ipc_h2d_cmd_send(p_cmd, cmd_len * 4, true) == false)
        {
            os_mem_free(p_cmd);
            return false;
        }

        os_mem_free(p_cmd);
        return true;
    }

    return false;
}

bool dsp_ipc_record_eq_clear(void)
{
    uint8_t cmd_buf[H2D_HEADER_LEN];

    cmd_buf[0] = (uint8_t)H2D_RECORD_EQ_PARA;
    cmd_buf[1] = (uint8_t)(H2D_RECORD_EQ_PARA >> 8);
    cmd_buf[2] = 0x01;
    cmd_buf[3] = 0;

    return dsp_ipc_h2d_cmd_send(cmd_buf, H2D_HEADER_LEN, true);
}

bool dsp_ipc_apt_eq_set(void *p_data, uint16_t len)
{
    uint8_t *p_cmd;
    uint16_t cmd_len = (len + H2D_HEADER_LEN + 3) / 4;

    DIPC_PRINT_TRACE2("dsp_ipc_apt_eq_set: p_data %p, len 0x%04x", p_data, len);

    p_cmd = os_mem_zalloc2(cmd_len * 4);
    if (p_cmd != NULL)
    {
        p_cmd[0] = (uint8_t)H2D_APT_EQ_PARA;
        p_cmd[1] = (uint8_t)(H2D_APT_EQ_PARA >> 8);
        p_cmd[2] = cmd_len; //eq_data + dsp_definition
        p_cmd[3] = cmd_len >> 8;

        memcpy(&p_cmd[H2D_HEADER_LEN], p_data, len);

        if (dsp_ipc_h2d_cmd_send(p_cmd, cmd_len * 4, true) == false)
        {
            os_mem_free(p_cmd);
            return false;
        }

        os_mem_free(p_cmd);
        return true;
    }

    return false;
}

bool dsp_ipc_apt_eq_clear(void)
{
    uint8_t cmd_buf[H2D_HEADER_LEN];

    cmd_buf[0] = (uint8_t)H2D_APT_EQ_PARA;
    cmd_buf[1] = (uint8_t)(H2D_APT_EQ_PARA >> 8);
    cmd_buf[2] = 0x01;
    cmd_buf[3] = 0;

    return dsp_ipc_h2d_cmd_send(cmd_buf, H2D_HEADER_LEN, true);
}

bool dsp_ipc_voice_nrec_set(uint8_t enable)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t  cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)H2D_MIC_NEAREND_PROC_SET;
    cmd_buf[1] = (uint8_t)(H2D_MIC_NEAREND_PROC_SET >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = enable;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_apt_nrec_set(uint8_t enable, uint8_t mode, uint8_t level)
{
    uint16_t  cmd_length = 2;
    uint16_t  cmd_buf_len = 12;
    uint8_t   cmd_buf[12] = {0};

    cmd_buf[0] = H2D_HA_PARA & 0xFF;
    cmd_buf[1] = (H2D_HA_PARA & 0xFF00) >> 8;
    cmd_buf[2] = cmd_length;
    cmd_buf[3] = cmd_length >> 8;
    cmd_buf[4] = 0xb4;
    cmd_buf[8] = enable;
    cmd_buf[9] = level;
    cmd_buf[10] = mode;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_apt_ovp_set(uint8_t enable, uint8_t level)
{
    uint16_t  cmd_length = 2;
    uint16_t  cmd_buf_len = 12;
    uint8_t   cmd_buf[12] = {0};

    cmd_buf[0] = H2D_HA_PARA & 0xFF;
    cmd_buf[1] = (H2D_HA_PARA & 0xFF00) >> 8;
    cmd_buf[2] = cmd_length;
    cmd_buf[3] = cmd_length >> 8;
    cmd_buf[4] = 0xb5;
    cmd_buf[8] = enable;
    cmd_buf[9] = level;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_apt_beamforming_set(uint8_t enable, uint8_t direction)
{
    uint16_t  cmd_length = 2;
    uint16_t  cmd_buf_len = 12;
    uint8_t   cmd_buf[12] = {0};

    cmd_buf[0] = H2D_HA_PARA & 0xFF;
    cmd_buf[1] = (H2D_HA_PARA & 0xFF00) >> 8;
    cmd_buf[2] = cmd_length;
    cmd_buf[3] = cmd_length >> 8;
    cmd_buf[4] = 0xb6;
    cmd_buf[8] = enable;
    cmd_buf[9] = direction;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_stream_channel_out_config(uint8_t param, bool flush)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_STREAM_CHANNEL_OUT_CONFIG);
    cmd_buf[1] = (uint8_t)(H2D_STREAM_CHANNEL_OUT_CONFIG >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, flush);
}

bool dsp_ipc_set_stream_channel_in_config(uint8_t param)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_STREAM_CHANNEL_IN_CONFIG);
    cmd_buf[1] = (uint8_t)(H2D_STREAM_CHANNEL_IN_CONFIG >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

static bool dsp_ipc_set_audio_sport_action(uint16_t id, uint8_t param, bool flush)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(id);
    cmd_buf[1] = (uint8_t)(id >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;

    DIPC_PRINT_TRACE3("dsp_ipc_set_audio_sport0_action: cmd id 0x%02x action %d, flush %d",
                      id, param, flush);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, flush);
}

bool dsp_ipc_set_audio_sport0_action(uint8_t param)
{
    return dsp_ipc_set_audio_sport_action(H2D_DSP_AUDIO_SPORT0_ACTION, param, true);
}

bool dsp_ipc_set_audio_sport1_action(uint8_t param)
{
    return dsp_ipc_set_audio_sport_action(H2D_DSP_AUDIO_SPORT1_ACTION, param, true);
}

bool dsp_ipc_set_fade_in_out_control(uint8_t param)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_FADE_IN_OUT_CONTROL);
    cmd_buf[1] = (uint8_t)(H2D_FADE_IN_OUT_CONTROL >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_action_control(void)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_CMD_ACTION_CTRL);
    cmd_buf[1] = (uint8_t)(H2D_CMD_ACTION_CTRL >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_sbc_encoder_hdr_config(T_DSP_SBC_ENCODE_PARAM param)
{
    uint32_t cmd_length = 3;
    uint32_t cmd_buf_len = 16;
    uint8_t cmd_buf[16] = {0};

    cmd_buf[0] = (uint8_t)(H2D_SBC_ENCODE_HDR_CONFIG);
    cmd_buf[1] = (uint8_t)(H2D_SBC_ENCODE_HDR_CONFIG >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)(param.sample_rate);
    cmd_buf[5] = (uint8_t)(param.sample_rate >> 8);
    cmd_buf[6] = (uint8_t)(param.sample_rate >> 16);
    cmd_buf[7] = (uint8_t)(param.sample_rate >> 24);
    cmd_buf[8] = param.chann_mode;
    cmd_buf[9] = param.block_length;
    cmd_buf[10] = param.subband_num;
    cmd_buf[11] = param.allocation_method;
    cmd_buf[12] = param.bitpool;
    cmd_buf[13] = param.frame_num;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, false);
}

bool dsp_ipc_set_rws_init(uint8_t bt_clk_index, uint32_t sample_rate, uint8_t type)
{
    uint32_t cmd_length = 2;
    uint32_t cmd_buf_len = 16;
    uint8_t cmd_buf[16] = {0};

    cmd_buf[0] = (uint8_t)(H2D_RWS_INIT);
    cmd_buf[1] = (uint8_t)(H2D_RWS_INIT >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = bt_clk_index;
    cmd_buf[8] = (uint8_t)sample_rate;
    cmd_buf[9] = (uint8_t)(sample_rate >> 8);
    cmd_buf[10] = (uint8_t)(sample_rate >> 16);
    cmd_buf[11] = (uint8_t)(sample_rate >> 24);
    cmd_buf[12] = type;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_rws_seamless(uint8_t param)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_RWS_SEAMLESS);
    cmd_buf[1] = (uint8_t)(H2D_RWS_SEAMLESS >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_rws(uint8_t param, uint8_t type)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_RWS_SET);
    cmd_buf[1] = (uint8_t)(H2D_RWS_SET >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;
    cmd_buf[5] = type;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_tx_path_ramp_gain_control(uint16_t gain_step, uint8_t time_index)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_TX_PATH_RAMP_GAIN_CONTROL);
    cmd_buf[1] = (uint8_t)(H2D_TX_PATH_RAMP_GAIN_CONTROL >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)(gain_step);
    cmd_buf[5] = (uint8_t)(gain_step >> 8);
    cmd_buf[6] = time_index;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_log_output_sel(uint8_t param)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_CMD_LOG_OUTPUT_SEL);
    cmd_buf[1] = (uint8_t)(H2D_CMD_LOG_OUTPUT_SEL >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_rws_asrc_ratio(uint32_t *param)
{
    uint32_t cmd_length = 2;
    uint32_t cmd_buf_len = 12;
    uint8_t cmd_buf[12];

    cmd_buf[0] = (uint8_t)(H2D_RWS_SET_ASRC_RATIO);
    cmd_buf[1] = (uint8_t)(H2D_RWS_SET_ASRC_RATIO >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = param[0];
    cmd_buf[5] = (param[0] >> 8);
    cmd_buf[6] = (param[0] >> 16);
    cmd_buf[7] = (param[0] >> 24);

    cmd_buf[8] = (uint8_t)(param[1]);
    cmd_buf[9] = (uint8_t)(param[1] >> 8);
    cmd_buf[10] = (uint8_t)(param[1] >> 16);
    cmd_buf[11] = (uint8_t)(param[1] >> 24);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_voice_prompt_end(void)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_CMD_PROMPT_ACTION);
    cmd_buf[1] = (uint8_t)(H2D_CMD_PROMPT_ACTION >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)DSP_IPC_STOP_VP;
    cmd_buf[5] = (uint8_t)(DSP_IPC_STOP_VP >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_signal_proc_start(uint32_t param, uint8_t sport_id)
{
    uint32_t cmd_length = 2;
    uint32_t cmd_buf_len = 12;
    uint8_t cmd_buf[12] = {0};

    cmd_buf[0] = (uint8_t)(H2D_SIGNAL_PROC_START);
    cmd_buf[1] = (uint8_t)(H2D_SIGNAL_PROC_START >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t) param;
    cmd_buf[5] = (uint8_t)(param >> 8);
    cmd_buf[6] = (uint8_t)(param >> 16);
    cmd_buf[7] = (uint8_t)(param >> 24);
    cmd_buf[8] = sport_id;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_handover_info(uint8_t role, bool start)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_HANDOVER_INFO);
    cmd_buf[1] = (uint8_t)(H2D_HANDOVER_INFO >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)start;
    cmd_buf[5] = (uint8_t)role;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_voice_trigger(uint32_t param)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_DSP_VOICE_TRIGGER_SET);
    cmd_buf[1] = (uint8_t)(H2D_DSP_VOICE_TRIGGER_SET >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)param;
    cmd_buf[5] = (uint8_t)(param >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_set_force_dummy_pkt(uint32_t param)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8] = {0};

    cmd_buf[0] = (uint8_t)(H2D_FORCE_DUMMY_PKT);
    cmd_buf[1] = (uint8_t)(H2D_FORCE_DUMMY_PKT >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = (uint8_t)param;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_sport_set(T_DSP_IPC_SPORT_CFG *param)
{
    uint32_t cmd_length;
    uint32_t cmd_buf_len;
    uint8_t cmd_buf[16];

    cmd_length = ((sizeof(T_DSP_IPC_SPORT_CFG) + 3) / WORD_TO_BYTE_LENGTH);
    cmd_buf_len = ((cmd_length + 1) * WORD_TO_BYTE_LENGTH);

    DIPC_PRINT_TRACE7("dsp_ipc_sport_set: sport_id %d, rtx %d, channel_count %d,"
                      " data_length %d, role %d, bridge %d, sample_rate %d", param->sport_id,
                      param->rtx, param->channel_count, param->data_length, param->role, param->bridge,
                      param->sample_rate);

    cmd_buf[0] = (uint8_t)(H2D_SPORT_SET);
    cmd_buf[1] = (uint8_t)(H2D_SPORT_SET >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    memcpy(&cmd_buf[4], param, sizeof(T_DSP_IPC_SPORT_CFG));

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

static bool dsp_ipc_hal_cback(uint32_t event, void *msg)
{
    if (event == DSP_HAL_EVT_D2H)
    {
        uint8_t rcv_buffer[288] = {0};
        uint32_t rxlen = d2h_cmd_recv(rcv_buffer, 288);

        if (rxlen)
        {
            uint16_t event;
            uint16_t payload_length;
            uint8_t *p;

            p = rcv_buffer;

            DIPC_LE_STREAM_TO_UINT16(event, p);
            DIPC_LE_STREAM_TO_UINT16(payload_length, p);

            DIPC_PRINT_TRACE2("dsp_ipc_hal_cback: event %#x, payload_length %#x",
                              event, payload_length);

            if (DIPC_EIF_GET(event) == DIPC_EIF_CURRENT)
            {
                dipc_event_parser(event, p, payload_length);
            }
            else
            {
                dsp_ipc_event_parser(event, p, payload_length);
            }

            dsp_send_msg(DSP_MSG_INTR_D2H_CMD, 0, NULL, 0);
        }
    }

    return true;
}

bool dsp_ipc_composite_data_send(void *p_data, uint32_t len)
{
    uint32_t  word_size;
    uint8_t *cmd_buffer;
    uint32_t cmd_buffer_len;

    word_size = (len + 3) >> 2;
    cmd_buffer_len = (word_size + 1) * WORD_TO_BYTE_LENGTH;

    cmd_buffer = os_mem_zalloc2(cmd_buffer_len);
    if (cmd_buffer != NULL)
    {
        cmd_buffer[0] = H2D_TONE_GEN_CONFIG;
        cmd_buffer[2] = word_size;
        memcpy(&cmd_buffer[4], p_data, len);

        if ((dsp_ipc_h2d_cmd_send(cmd_buffer, cmd_buffer_len, true)) != true)
        {
            os_mem_free(cmd_buffer);
            return false;
        }

        os_mem_free(cmd_buffer);
        return true;
    }
    return false;
}

bool dsp_ipc_voice_prompt_start(uint32_t cfg, uint32_t cfg_bt_clk_mix)
{
    uint32_t cmd_length = 2;
    uint32_t cmd_buf_len = 12;
    uint8_t  cmd_buf[12];

    cmd_buf[0] = (uint8_t)(H2D_CMD_PROMPT_CONFIG);
    cmd_buf[1] = 0;
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = 0;
    cmd_buf[4] = (uint8_t)cfg;
    cmd_buf[5] = (uint8_t)(cfg >> 8);
    cmd_buf[6] = (uint8_t)(cfg >> 16);
    cmd_buf[7] = (uint8_t)(cfg >> 24);

    cmd_buf[8] = (uint8_t)cfg_bt_clk_mix;
    cmd_buf[9] = (uint8_t)(cfg_bt_clk_mix >> 8);
    cmd_buf[10] = (uint8_t)(cfg_bt_clk_mix >> 16);
    cmd_buf[11] = (uint8_t)(cfg_bt_clk_mix >> 24);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_init(void)
{
    int32_t ret = 0;

    dsp_ipc_db = os_mem_zalloc2(sizeof(T_DSP_IPC_DB));
    if (dsp_ipc_db == NULL)
    {
        ret = 1;
        goto fail_alloc_db;
    }

    os_queue_init(&dsp_ipc_db->cback_list);

    dsp_hal_register_cback(DSP_HAL_PRIORITY_LOW, dsp_ipc_hal_cback);

    return true;

fail_alloc_db:
    DIPC_PRINT_ERROR1("dsp_ipc_init: failed %d", -ret);
    return false;
}

void dsp_ipc_deinit(void)
{
    if (dsp_ipc_db != NULL)
    {
        os_mem_free(dsp_ipc_db);
        dsp_ipc_db = NULL;
    }
}

bool dsp_ipc_voice_prompt_send(uint8_t *p_data, uint32_t len)
{
    uint16_t word_length;
    uint8_t *cmd_ptr;
    word_length = (len + 3) / 4;
    cmd_ptr = os_mem_zalloc2((word_length + 1) * WORD_TO_BYTE_LENGTH);
    if (cmd_ptr == NULL)
    {
        return false;
    }
    cmd_ptr[0] = H2D_CMD_PROMPT_ACTION;
    cmd_ptr[1] = (uint8_t)(H2D_CMD_PROMPT_ACTION >> 8);
    cmd_ptr[2] = (uint8_t)word_length;
    cmd_ptr[3] = (uint8_t)(word_length >> 8);
    memcpy(&cmd_ptr[4], p_data, len);
    if (h2d_cmd_send(cmd_ptr, (word_length + 1)*WORD_TO_BYTE_LENGTH, true) == false)
    {
        os_mem_free(cmd_ptr);
        return false;
    }
    os_mem_free(cmd_ptr);

    return true;
}

static bool dsp_ipc_event_parser(uint16_t cmd_id, uint8_t *param, uint16_t payload_length)
{
    T_DSP_IPC_EVENT event = DSP_IPC_EVT_NONE;
    uint32_t parameter = 0;
    uint32_t temp_param[3] = {0};
    T_DIPC_GATE_STATUS gate_state;

    if (dsp_ipc_db != NULL)
    {
        switch (cmd_id)
        {
        case D2H_OPEN_AIR_AVC:
            {
                event = DSP_IPC_EVT_OPEN_AIR_AVC;
                temp_param[0] = (uint32_t)payload_length;
                temp_param[1] = (uint32_t)param;
                parameter = (uint32_t)(&temp_param[0]);
            }
            break;

        case D2H_UPLINK_SYNCREF_REQUEST:
            {
                /* FIXME https://jira.realtek.com/browse/BB2RD-656 */
                event = DSP_IPC_EVT_PROBE_SYNC_REF_REQUEST;
                parameter = (uint32_t)param; /* session id */
            }
            break;

        case D2H_REPORT_STREAM_LATENCY:
            {
                event = DSP_IPC_EVT_LATENCY_REPORT;
                parameter = (uint32_t)param;
            }
            break;

        case D2H_AUDIOPLAY_VOLUME_INFO:
            {
                event = DSP_IPC_EVT_AUDIOPLAY_VOLUME_INFO;
                parameter = (uint32_t)param;
            }
            break;

        case D2H_DSP_PLC_FRAME_NUM:
            {
                event = DSP_IPC_EVT_PLC_NUM;
                parameter = (uint32_t)param;
            }
            break;

        case D2H_SILENCE_DETECT_REPORT:
            {
                event = DSP_IPC_EVT_DETECT_SILENCE;
                parameter = param[0];
            }
            break;

        case D2H_DSP_POWER_DOWN_ACK:
            {
                event = DSP_IPC_EVT_POWER_OFF_ACK;
            }
            break;

        case D2H_DSP_MIPS_REQIURED:
            {
                event = DSP_IPC_EVT_CLK_REQUEST;
                parameter = param[0] + (param[1] << 8)
                            + (param[2] << 16) + (param[3] << 24);
            }
            break;

        case D2H_DSP2_MIPS_REQIURED:
            {
                event = DSP_IPC_EVT_DSP2_CLK_REQUEST;
                parameter = param[0] + (param[1] << 8)
                            + (param[2] << 16) + (param[3] << 24);
            }
            break;

        case D2H_DSP_STATUS_IND:
            {
                uint8_t ind_type;
                uint8_t ind_status;

                ind_type = param[0];
                ind_status = param[1];
                if ((ind_type == D2H_IND_TYPE_RINGTONE) && (ind_status == D2H_IND_STATUS_RINGTONE_STOP))
                {
                    event = DSP_IPC_EVT_NOTIFICATION_FINISH;
                }
                else if (ind_type == D2H_IND_TYPE_PROMPT)
                {
                    if (ind_status == D2H_IND_STATUS_PROMPT_FINISH)
                    {
                        event = DSP_IPC_EVT_NOTIFICATION_FINISH;
                    }
                    else if ((ind_status == D2H_IND_STATUS_PROMPT_REQUEST) ||
                             (ind_status == D2H_IND_STATUS_PROMPT_DECODE_ERROR))
                    {
                        event = DSP_IPC_EVT_VP_REQUEST_DATA;
                    }
                }
                else if ((ind_type == D2H_IND_TYPE_FADE_IN) && (ind_status == D2H_IND_STATUS_FADE_IN_COMPLETE))
                {
                    event = DSP_IPC_EVT_FADE_IN_FINISH;
                }
                else if ((ind_type == D2H_IND_TYPE_FADE_OUT) && (ind_status == D2H_IND_STATUS_FADE_OUT_COMPLETE))
                {
                    event = DSP_IPC_EVT_FADE_OUT_FINISH;
                }
                else if (ind_type == D2H_IND_TYPE_DECODE_EMPTY)
                {
                    event = DSP_IPC_EVT_DECODE_EMPTY;
                }
                else if (ind_type == D2H_IND_TYPE_KEYWORD)
                {
                    event = DSP_IPC_EVT_KEYWORD;
                }
                else if (ind_type == D2H_IND_TYPE_SEG_SEND)
                {
                    if (ind_status == D2H_IND_STATUS_SEG_SEND_REQ_DATA)
                    {
                        event = DSP_IPC_EVT_SEG_SEND_REQ_DATA;
                    }
                    else if (ind_status == D2H_IND_STATUS_SEG_SEND_ERROR)
                    {
                        event = DSP_IPC_EVT_SEG_SEND_ERROR;
                    }
                }
            }
            break;

        case D2H_DSP_STATE:   // only for DSP control state now
            {
                T_PHY_CODEC_STATE *dsp_state;

                gate_state.dir_bit = 0;

                dsp_state = (T_PHY_CODEC_STATE *)param;

                if (dsp_state->state == DSP_SPORT0_READY)
                {
                    gate_state.gate_id = DIPC_GATE_ID0;
                    gate_state.ready = true;
                }
                else if (dsp_state->state == DSP_SPORT0_STOP)
                {
                    gate_state.gate_id = DIPC_GATE_ID0;
                    gate_state.ready = false;
                }
                else if (dsp_state->state == DSP_SPORT1_READY)
                {
                    gate_state.gate_id = DIPC_GATE_ID1;
                    gate_state.ready = true;
                }
                else if (dsp_state->state == DSP_SPORT1_STOP)
                {
                    gate_state.gate_id = DIPC_GATE_ID1;
                    gate_state.ready = false;
                }

                if (dsp_state->path == DSP_STREAM_AD_DA_READY)
                {
                    gate_state.dir_bit |= BIT(DIPC_DIRECTION_RX);
                    gate_state.dir_bit |= BIT(DIPC_DIRECTION_TX);
                }
                else if (dsp_state->path == DSP_STREAM_DA_READY)
                {
                    gate_state.dir_bit |= BIT(DIPC_DIRECTION_TX);
                }
                else if (dsp_state->path == DSP_STREAM_AD_READY)
                {
                    gate_state.dir_bit |= BIT(DIPC_DIRECTION_RX);
                }

                event = DSP_IPC_EVT_GATE_STATUS;

                parameter = (uint32_t) & (gate_state);
            }
            break;

        case D2H_RWS_SLAVE_SYNC_STATUS:
            {
                uint32_t *pt = (uint32_t *)&param[0];
                uint8_t sync_state;

                parameter = (uint8_t)pt[2];
                sync_state = (uint8_t)pt[2];

                if (sync_state == RWS_SPK2_AUDIO_SYNC_V2_SUCCESSFUL)
                {
                    event = DSP_IPC_EVT_DSP_SYNC_V2_SUCC;
                }
                else if (sync_state == RWS_SPK2_AUDIO_SYNC_FAIL)
                {
                    event = DSP_IPC_EVT_DSP_UNSYNC;
                }
                else if (sync_state == RWS_START_BTCLK_EXPIRED)
                {
                    event = DSP_IPC_EVT_BTCLK_EXPIRED;
                }
                else if (sync_state == RWS_SPK2_AUDIO_SYNC_UNLOCK)
                {
                    event = DSP_IPC_EVT_DSP_SYNC_UNLOCK;
                }
                else if (sync_state == RWS_SPK2_AUDIO_SYNC_LOCK)
                {
                    event = DSP_IPC_EVT_DSP_SYNC_LOCK;
                }
                else if (sync_state == RWS_SPK2_AUDIO_SYNC_EMPTY)
                {
                    event = DSP_IPC_EVT_SYNC_EMPTY;
                }
                else if (sync_state == RWS_SPK2_AUDIO_SYNC_LOSE_TIMESTAMP)
                {
                    event = DSP_IPC_EVT_SYNC_LOSE_TIMESTAMP;
                }
            }
            break;

        case  D2H_B2BMSG_INTERACTION_SEND:
            {
                event = DSP_IPC_EVT_B2BMSG;
                parameter = (uint32_t)param;
            }
            break;

        case D2H_RWS_SEAMLESS_RETURN_INFO:
            {
                event = DSP_IPC_EVT_JOIN_CLK;
                uint8_t i;
                for (i = 0; i < 3; i++)
                {
                    temp_param[i] |= param[3 + 4 * i];
                    temp_param[i] = temp_param[i] << 8;

                    temp_param[i] |= param[2 + 4 * i];
                    temp_param[i] = temp_param[i] << 8;

                    temp_param[i] |= param[1 + 4 * i];
                    temp_param[i] = temp_param[i] << 8;

                    temp_param[i] |= param[0 + 4 * i];
                }
                parameter = (uint32_t)(&(temp_param[0]));
            }
            break;

        case D2H_SCENARIO_STATE:
            {
                event = DSP_IPC_EVT_PROBE;
                parameter = (uint32_t)param;
            }
            break;

        case D2H_EARTIP_FIT_RESULT:
            {
                event = DSP_IPC_EVT_PROBE_EARFIT;
                parameter = (uint32_t)param;
            }
            break;

        case D2H_DSP_SDK_GENERAL_CMD:
            {
                event = DSP_IPC_EVT_PROBE_SDK_GEN_CMD;
                parameter = (uint32_t)param;
            }
            break;

        case D2H_DSP_BOOT_DONE:
            {
                event = DSP_IPC_EVT_PROBE_SDK_BOOT;
                parameter = (uint32_t)param;

            }
            break;

        case D2H_HA_VER_INFO:
            {
                event = DSP_IPC_EVT_HA_VER_INFO;
                parameter = (uint32_t)param[0];
            }
            break;

        case D2H_ACTION_ACK:
            {
                T_DSP_IPC_ACTION_ACK *action_ack;

                action_ack = (T_DSP_IPC_ACTION_ACK *)param;

                if (action_ack->h2d_id == H2D_DSP_AUDIO_PASSTHROUGH_ACTION)
                {
                    event = DSP_IPC_EVT_APT_ACTION_ACK;
                }
                else if (action_ack->h2d_id == H2D_DECODER_ACTION)
                {
                    event = DSP_IPC_EVT_DECODER_ACTION_ACK;
                }
                else
                {
                    event = DSP_IPC_EVT_ENCODER_ACTION_ACK;
                }
                parameter = action_ack->result;
            }
            break;

        case D2H_DECODER_PLC_NOTIFY:
            {
                event = DSP_IPC_EVT_DECODER_PLC_NOTIFY;
                parameter = (uint32_t)param;
            }
            break;

        default:
            break;
        }

        if (event != DSP_IPC_EVT_NONE)
        {
            T_DSP_IPC_CBACK_ELEM *elem;

            elem = os_queue_peek(&dsp_ipc_db->cback_list, 0);

            while (elem != NULL)
            {
                elem->cback(event, parameter);
                elem = elem->next;
            }
        }
    }

    return true;
}

bool dsp_ipc_cback_register(P_DSP_IPC_CBACK cback)
{
    T_DSP_IPC_CBACK_ELEM *elem;

    elem = os_mem_zalloc2(sizeof(T_DSP_IPC_CBACK_ELEM));
    if (elem != NULL)
    {
        elem->cback = cback;
        os_queue_in(&dsp_ipc_db->cback_list, elem);
        return true;
    }

    return false;
}

bool dsp_ipc_cback_unregister(P_DSP_IPC_CBACK cback)
{
    T_DSP_IPC_CBACK_ELEM *elem;

    elem = os_queue_peek(&dsp_ipc_db->cback_list, 0);
    while (elem != NULL)
    {
        if (elem->cback == cback)
        {
            return os_queue_delete(&dsp_ipc_db->cback_list, elem);
        }

        elem = elem->next;
    }

    return false;
}

static bool dsp_ipc_action_send(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io,
                                uint16_t cmd_id, bool flush)
{
    uint32_t cmd_length;
    uint32_t cmd_buf_len;
    uint8_t *cmd_buf;

    cmd_length  = logic_io->tlv_cnt + 2;
    cmd_buf_len = ((cmd_length + 1) * WORD_TO_BYTE_LENGTH);
    cmd_buf = os_mem_zalloc2(cmd_buf_len);

    if (cmd_buf == NULL)
    {
        return false;
    }

    cmd_buf[0] = (uint8_t)(cmd_id);
    cmd_buf[1] = (uint8_t)(cmd_id >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    memcpy(&cmd_buf[4], &action, sizeof(T_ACTION_CONFIG));
    memcpy(&cmd_buf[8], logic_io, sizeof(T_DSP_IPC_LOGIC_IO) - WORD_TO_BYTE_LENGTH);
    memcpy(&cmd_buf[12], logic_io->tlv, sizeof(T_DSP_IPC_LOGIC_IO_TLV)*logic_io->tlv_cnt);

    for (uint8_t i = 0; i < cmd_buf_len; i = i + 8)
    {
        DIPC_PRINT_TRACE8("dsp_ipc_action_send: %#x %#x %#x %#x %#x %#x %#x %#x",
                          cmd_buf[i],  cmd_buf[i + 1], cmd_buf[i + 2],  cmd_buf[i + 3],
                          cmd_buf[i + 4],  cmd_buf[i + 5], cmd_buf[i + 6],  cmd_buf[i + 7]);
    }

    if (h2d_cmd_send(cmd_buf, cmd_buf_len, flush) == false)
    {
        os_mem_free(cmd_buf);
        return false;
    }
    os_mem_free(cmd_buf);
    return true;
}

bool dsp_ipc_apt_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io)
{
    return dsp_ipc_action_send(action, logic_io, H2D_DSP_AUDIO_PASSTHROUGH_ACTION, true);
}

bool dsp_ipc_decoder_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io)
{
    return dsp_ipc_action_send(action, logic_io, H2D_DECODER_ACTION, true);
}

bool dsp_ipc_encoder_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io)
{
    return dsp_ipc_action_send(action, logic_io, H2D_ENCODER_ACTION, true);
}

bool dsp_ipc_vad_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io)
{
    return dsp_ipc_action_send(action, logic_io, H2D_DSP_VOICE_TRIGGER_ACTION, true);
}

bool dsp_ipc_line_in_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io)
{
    return dsp_ipc_action_send(action, logic_io, H2D_DSP_LINE_IN_ACTION, true);
}

bool dsp_ipc_init_dsp_sdk(bool flush, uint8_t scenario)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8];

    cmd_buf[0] = (uint8_t)(H2D_DSP_SDK_INIT);
    cmd_buf[1] = (uint8_t)(H2D_DSP_SDK_INIT >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = scenario;
    cmd_buf[5] = 0;
    cmd_buf[6] = 0;
    cmd_buf[7] = 0;

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, flush);
}

bool dsp_ipc_sidetone_set(uint8_t enable, int16_t gain, uint8_t level)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 8;
    uint8_t cmd_buf[8];

    cmd_buf[0] = (uint8_t)(H2D_VOICE_SIDETONE_SET);
    cmd_buf[1] = (uint8_t)(H2D_VOICE_SIDETONE_SET >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = enable;
    cmd_buf[5] = gain;
    cmd_buf[6] = (gain << 8);
    cmd_buf[7] = 0;

    return h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

static bool dipc2_cmd_send(uint8_t *cmd_buf, uint32_t cmd_buf_len, bool flush)
{
    bool ret;

    ret = h2d_cmd_send(cmd_buf, cmd_buf_len, flush);

    DIPC_PRINT_TRACE6("dipc2_cmd_send: cmd_buf %p, cmd_buf_len %d, ret %d, flush %d, %#x %#x",
                      cmd_buf, cmd_buf_len, ret, flush,
                      *((uint16_t *)(&cmd_buf[0])), *((uint16_t *)(&cmd_buf[2])));

    return ret;
}

const static T_STREAM_COMMAND start_cmd[] =
{
    DSP_STREAM_PLAY_START,          /* for None direction */
    DSP_STREAM_PLAY_START,          /* for DIPC_DIRECTION_TX */
    DSP_STREAM_REC_START,           /* for DIPC_DIRECTION_RX */
    DSP_STREAM_REC_PLAY_START,      /* for DIPC_DIRECTION_TX DIPC_DIRECTION_RX */
};

static bool dipc_gate_start_core(uint8_t sport_id, uint8_t sport_direction)
{
    uint8_t cmd_buf[8];
    uint8_t *p;

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_GATE_START);
    DIPC_LE_UINT16_TO_STREAM(p, 4);
    DIPC_LE_UINT8_TO_STREAM(p,  sport_id);
    DIPC_LE_UINT16_TO_STREAM(p, 0x0000);
    DIPC_LE_UINT8_TO_STREAM(p,  sport_direction);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}

bool dipc_gate_start(uint8_t id, uint8_t dir_bit)
{
    bool ret;
    T_STREAM_COMMAND cmd;

    ret = false;

    cmd = start_cmd[dir_bit];

    if (id == DIPC_GATE_ID0)
    {
        ret = dsp_ipc_set_audio_sport0_action(cmd);
    }
    else if (id == DIPC_GATE_ID1)
    {
        ret = dsp_ipc_set_audio_sport1_action(cmd);
    }
    else if (id == DIPC_GATE_ID2)
    {
        if ((dir_bit & BIT(DIPC_DIRECTION_TX)) == BIT(DIPC_DIRECTION_TX))
        {
            ret = dipc_gate_start_core(id, DIPC_DIRECTION_TX);
        }
        else
        {
            ret = dipc_gate_start_core(id, DIPC_DIRECTION_RX);
        }
    }

    return ret;
}

const static T_STREAM_COMMAND stop_cmd[] =
{
    DSP_STREAM_PLAY_STOP,          /* for None direction */
    DSP_STREAM_PLAY_STOP,          /* for DIPC_DIRECTION_TX */
    DSP_STREAM_REC_STOP,           /* for DIPC_DIRECTION_RX */
    DSP_STREAM_REC_PLAY_STOP,      /* for DIPC_DIRECTION_TX DIPC_DIRECTION_RX */
};

static bool dipc_gate_stop_core(uint8_t sport_id, uint8_t sport_direction)
{
    uint8_t cmd_buf[8];
    uint8_t *p;

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_GATE_STOP);
    DIPC_LE_UINT16_TO_STREAM(p, 4);
    DIPC_LE_UINT8_TO_STREAM(p,  sport_id);
    DIPC_LE_UINT8_TO_STREAM(p,  sport_direction);
    DIPC_LE_UINT16_TO_STREAM(p, 0x0000);

    DIPC_PRINT_TRACE2("dipc_gate_start: sport_id %d, sport_direction %#x", sport_id, sport_direction);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}

bool dipc_gate_stop(uint8_t id, uint8_t dir_bit)
{
    bool ret;
    T_STREAM_COMMAND cmd;

    ret = false;

    cmd = stop_cmd[dir_bit];

    if (id == DIPC_GATE_ID0)
    {
        ret = dsp_ipc_set_audio_sport0_action(cmd);
    }
    else if (id == DIPC_GATE_ID1)
    {
        ret = dsp_ipc_set_audio_sport1_action(cmd);
    }
    else if (id == DIPC_GATE_ID2)
    {
        if ((dir_bit & BIT(DIPC_DIRECTION_TX)) == BIT(DIPC_DIRECTION_TX))
        {
            ret = dipc_gate_stop_core(id, DIPC_DIRECTION_TX);
        }
        else
        {
            ret = dipc_gate_stop_core(id, DIPC_DIRECTION_RX);
        }
    }

    return ret;
}

bool dsp_ipc_signal_out_monitoring_set(uint8_t enable, uint16_t refresh_interval)
{
    uint32_t cmd_length = 1;
    uint32_t cmd_buf_len = 0;
    uint8_t cmd_buf[8];

    cmd_buf_len = ((cmd_length + 1) * WORD_TO_BYTE_LENGTH);
    memset(cmd_buf, 0, cmd_buf_len);
    cmd_buf[0] = (uint8_t)(H2D_CURRENTLY_VOLUME_REQUEST);
    cmd_buf[1] = (uint8_t)(H2D_CURRENTLY_VOLUME_REQUEST  >> 8);
    cmd_buf[2] = (uint8_t)(cmd_length);
    cmd_buf[3] = (uint8_t)(cmd_length >> 8);
    cmd_buf[4] = enable;
    cmd_buf[6] = (uint8_t)refresh_interval;
    cmd_buf[7] = (uint8_t)(refresh_interval >> 8);

    return dsp_ipc_h2d_cmd_send(cmd_buf, cmd_buf_len, true);
}

bool dsp_ipc_signal_in_monitoring_set(uint8_t enable, uint16_t refresh_interval)
{
    DIPC_PRINT_TRACE0("dsp_ipc_signal_in_monitoring_set: DIPC2.0 required");
    return false;
}

void dsp_ipc_download_algo_param(uint8_t *cmd_buffer, uint16_t algo_cmd_length)
{
    h2d_cmd_send(cmd_buffer, algo_cmd_length, true);
}

///dipc2.0
static void dipc_event_post(uint32_t event, void *param)
{
    T_DSP_IPC_CBACK_ELEM *elem;

    if (event != DSP_IPC_EVT_NONE)
    {
        elem = os_queue_peek(&dsp_ipc_db->cback_list, 0);
        while (elem != NULL)
        {
            elem->cback((T_DSP_IPC_EVENT)event, (uint32_t)param);
            elem = elem->next;
        }
    }
}

static bool dipc_event_codec_pipe_ack_handler(uint8_t *payload, uint16_t payload_length)
{
    uint8_t *p;
    uint8_t status;
    uint16_t opcode;
    uint32_t session_id;

    p = payload;

    DIPC_LE_STREAM_TO_UINT16(opcode, p);
    DIPC_LE_STREAM_TO_UINT8(status, p);
    DIPC_LE_STREAM_TO_UINT32(session_id, p);

    DIPC_PRINT_TRACE3("dipc_event_codec_pipe_ack_handler: opcode %#x, status %#x, session_id %#x",
                      opcode, status, session_id);

    return true;
}

static bool dipc_event_codec_pipe_complete_handler(uint8_t *payload, uint16_t payload_length)
{
    uint8_t *p;
    uint8_t status;
    uint16_t opcode;
    uint32_t session_id;

    p = payload;

    DIPC_LE_STREAM_TO_UINT16(opcode, p);
    DIPC_LE_STREAM_TO_UINT8(status, p);
    DIPC_LE_STREAM_TO_UINT32(session_id, p);

    DIPC_PRINT_TRACE3("dipc_event_codec_pipe_complete_handler: opcode %#x, status %#x, session_id %#x",
                      opcode, status, session_id);

    switch (opcode)
    {
    case DIPC_H2D_CODEC_PIPE_CREATE:
        {
            T_DIPC_CODEC_PIPE_CREATE_CMPL codec_pipe_create_cmpl;

            codec_pipe_create_cmpl.status = status;
            codec_pipe_create_cmpl.session_id = session_id;
            if (status == DIPC_ERROR_SUCCESS)
            {
                DIPC_LE_STREAM_TO_UINT32(codec_pipe_create_cmpl.src_transport_address, p);
                DIPC_LE_STREAM_TO_UINT32(codec_pipe_create_cmpl.src_transport_size, p);
                DIPC_LE_STREAM_TO_UINT32(codec_pipe_create_cmpl.snk_transport_address, p);
                DIPC_LE_STREAM_TO_UINT32(codec_pipe_create_cmpl.snk_transport_size, p);

                DIPC_PRINT_TRACE4("dipc_event_codec_pipe_complete_handler: src_addr 0x%x, src_size 0x%x "
                                  "snk_addr 0x%x, snk_size 0x%x",
                                  codec_pipe_create_cmpl.src_transport_address, codec_pipe_create_cmpl.src_transport_size,
                                  codec_pipe_create_cmpl.snk_transport_address, codec_pipe_create_cmpl.snk_transport_size);
            }

            dipc_event_post(DSP_IPC_EVT_CODEC_PIPE_CREATE, &codec_pipe_create_cmpl);
        }
        break;

    case DIPC_H2D_CODEC_PIPE_DESTROY:
        {
            dipc_event_post(DSP_IPC_EVT_CODEC_PIPE_DESTROY, (void *)session_id);
        }
        break;

    case DIPC_H2D_CODEC_PIPE_START:
        {
            dipc_event_post(DSP_IPC_EVT_CODEC_PIPE_START, (void *)session_id);
        }
        break;

    case DIPC_H2D_CODEC_PIPE_STOP:
        {
            dipc_event_post(DSP_IPC_EVT_CODEC_PIPE_STOP, (void *)session_id);
        }
        break;

    case DIPC_H2D_CODEC_PIPE_PRE_MIXER_ADD:
        {
            dipc_event_post(DSP_IPC_EVT_CODEC_PIPE_PRE_MIXER_ADD, (void *)session_id);
        }
        break;

    case DIPC_H2D_CODEC_PIPE_POST_MIXER_ADD:
        {
            dipc_event_post(DSP_IPC_EVT_CODEC_PIPE_POST_MIXER_ADD, (void *)session_id);
        }
        break;

    case DIPC_H2D_CODEC_PIPE_MIXER_REMOVE:
        {
            dipc_event_post(DSP_IPC_EVT_CODEC_PIPE_MIXER_REMOVE, (void *)session_id);
        }
        break;

    default:
        break;
    }

    return true;
}

static bool dipc_event_codec_pipe_data_ack_handler(uint8_t *payload, uint16_t payload_length)
{
    uint8_t *p;
    uint8_t status;
    uint16_t seq;
    uint32_t session_id;

    p = payload;

    DIPC_LE_STREAM_TO_UINT8(status, p);
    DIPC_LE_STREAM_TO_UINT32(session_id, p);
    DIPC_LE_STREAM_TO_UINT16(seq, p);

    if (status == DIPC_ERROR_SUCCESS)
    {
        dipc_event_post(DSP_IPC_EVT_CODEC_PIPE_DATA_ACK, (void *)session_id);
    }
    else
    {
        DIPC_PRINT_ERROR3("dipc_event_codec_pipe_data_ack_handler: status 0x%02x, session_id 0x%08x, seq 0x%04x",
                          status, session_id, seq);
    }

    return true;
}

static bool dipc_event_codec_pipe_data_indicate_handler(uint8_t *payload, uint16_t payload_length)
{
    uint8_t *p;
    uint8_t status;
    uint16_t seq;
    uint32_t session_id;

    p = payload;

    DIPC_LE_STREAM_TO_UINT8(status, p);
    DIPC_LE_STREAM_TO_UINT32(session_id, p);
    DIPC_LE_STREAM_TO_UINT16(seq, p);

    if (status == DIPC_ERROR_SUCCESS)
    {
        dipc_event_post(DSP_IPC_EVT_CODEC_PIPE_DATA_IND, (void *)session_id);
    }
    else
    {
        DIPC_PRINT_ERROR3("dipc_event_codec_pipe_data_indicate_handler: status 0x%02x, session_id 0x%08x, seq 0x%04x",
                          status, session_id, seq);
    }

    return true;
}

static bool dipc_event_parser(uint16_t event, uint8_t *payload, uint16_t payload_length)
{
    bool ret;

    ret = false;

    switch (event)
    {
    case D2H_INFORMATION_EXCHANGE_COMMAND_ACK      :
        {
            //ret = dipc_event_info_ack_handler(payload, payload_length);
        }
        break;

    case D2H_INFORMATION_EXCHANGE_COMMAND_COMPLETE :
        {
            //ret = dipc_event_info_complete_handler(payload, payload_length);
        }
        break;

    case D2H_HOST_VERSION_REQUEST                  :
        {

        }
        break;

    case D2H_CONFIGURE_CONTROL_COMMAND_ACK                   :
        {
            //ret = dipc_event_gateway_ack_handler(payload, payload_length);
        }
        break;

    case D2H_CONFIGURE_CONTROL_COMMAND_COMPLETE              :
        {
            //ret = dipc_event_gateway_complete_handler(payload, payload_length);
        }
        break;

    case D2H_SYNCHRONIZATION_COMMAND_ACK           :
        {
            //ret = dipc_event_sync_ack_handler(payload, payload_length);
        }
        break;

    case D2H_SYNCHRONIZATION_COMMAND_COMPLETE      :
        {
            //ret = dipc_event_sync_complete_handler(payload, payload_length);
        }
        break;

    case D2H_TIMESTAMP_LOSE                        :
        {
            //ret = dipc_event_timestamp_lose_handler(payload, payload_length);
        }
        break;

    case D2H_SYNCHRONIZATION_LATCH        :
        {
            //ret = dipc_event_sync_latch_handler(payload, payload_length);
        }
        break;

    case D2H_SYNCHRONIZATION_UNLATCH      :
        {
            //ret = dipc_event_sync_unlatch_handler(payload, payload_length);
        }
        break;

    case D2H_JOIN_INFO_REPORT             :
        {
            //ret = dipc_event_info_report_handler(payload, payload_length);
        }
        break;

    case D2H_INFO_RELAY_REQUEST           :
        {
            //ret = dipc_event_relay_request_handler(payload, payload_length);
        }
        break;

    case D2H_CODEC_PIPE_COMMAND_ACK       :
        {
            ret = dipc_event_codec_pipe_ack_handler(payload, payload_length);
        }
        break;

    case D2H_CODEC_PIPE_COMMAND_COMPLETE  :
        {
            ret = dipc_event_codec_pipe_complete_handler(payload, payload_length);
        }
        break;

    case D2H_CODEC_PIPE_DATA_ACK          :
        {
            ret = dipc_event_codec_pipe_data_ack_handler(payload, payload_length);
        }
        break;

    case D2H_CODEC_PIPE_DATA_INDICATE     :
        {
            ret = dipc_event_codec_pipe_data_indicate_handler(payload, payload_length);
        }
        break;

    case D2H_DECODER_COMMAND_ACK          :
        {
            //ret = dipc_event_decoder_ack_handler(payload, payload_length);
        }
        break;

    case D2H_DECODER_COMMAND_COMPLETE     :
        {
            //ret = dipc_event_decoder_complete_handler(payload, payload_length);
        }
        break;

    case D2H_DECODER_DATA_ACK             :
        {
            //ret = dipc_event_decoder_data_ack_handler(payload, payload_length);
        }
        break;

    case D2H_DECODER_DATA_COMPLETE        :
        {
            //ret = dipc_event_decoder_data_complete_handler(payload, payload_length);
        }
        break;

    case D2H_ENCODER_COMMAND_ACK          :
        {
            //ret = dipc_event_encoder_ack_handler(payload, payload_length);
        }
        break;

    case D2H_ENCODER_COMMAND_COMPLETE     :
        {
            //ret = dipc_event_encoder_complete_handler(payload, payload_length);
        }
        break;

    case D2H_ENCODER_DATA_COMPLETE:
        {
            //ret = dipc_event_encoder_data_complete_handler(payload, payload_length);
        }
        break;

    case D2H_RINGTONE_COMMAND_ACK:
        {
            //ret = dipc_event_ringtone_ack_handler(payload, payload_length);
        }
        break;

    case D2H_RINGTONE_COMMAND_COMPLETE:
        {
            //ret = dipc_event_ringtone_complete_handler(payload, payload_length);
        }
        break;

    case D2H_VOICE_PROMPT_COMMAND_ACK:
        {
            //ret = dipc_event_voice_prompt_ack_handler(payload, payload_length);
        }
        break;

    case D2H_VOICE_PROMPT_COMMAND_COMPLETE:
        {
            //ret = dipc_event_voice_prompt_complete_handler(payload, payload_length);
        }
        break;

    case D2H_VOICE_PROMPT_DATA_ACK        :
        {
            //ret = dipc_event_voice_prompt_data_ack_handler(payload, payload_length);
        }
        break;

    case D2H_VOICE_PROMPT_DATA_COMPLETE   :
        {
            //ret = dipc_event_voice_prompt_data_complete_handler(payload, payload_length);
        }
        break;

    case D2H_LINE_IN_COMMAND_ACK          :
        {
            //ret = dipc_event_line_in_ack_handler(payload, payload_length);
        }
        break;

    case D2H_LINE_IN_COMMAND_COMPLETE     :
        {
            //ret = dipc_event_line_in_complete_handler(payload, payload_length);
        }
        break;

    case D2H_APT_COMMAND_ACK              :
        {
            //ret = dipc_event_apt_ack_handler(payload, payload_length);
        }
        break;

    case D2H_APT_COMMAND_COMPLETE         :
        {
            //ret = dipc_event_apt_complete_handler(payload, payload_length);
        }
        break;

    case D2H_VAD_COMMAND_ACK              :
        {
            //ret = dipc_event_vad_ack_handler(payload, payload_length);
        }
        break;

    case D2H_VAD_COMMAND_COMPLETE         :
        {
            //ret = dipc_event_vad_complete_handler(payload, payload_length);
        }
        break;

    case D2H_COMMAND_ACK                  :
        {

        }
        break;

    case D2H_COMMAND_COMPLETE             :
        {

        }
        break;

    default :
        {}
        break;
    }

    return ret;
}

bool dipc_codec_pipe_create(uint32_t session_id,
                            uint8_t  data_mode,
                            uint8_t  src_coder_id,
                            uint8_t  src_frame_num,
                            uint16_t src_coder_format_size,
                            uint8_t *src_coder_format,
                            uint8_t  snk_coder_id,
                            uint8_t  snk_frame_num,
                            uint16_t snk_coder_format_size,
                            uint8_t *snk_coder_format)
{
    uint8_t *cmd_buf;
    uint8_t *p;
    uint16_t cmd_size;
    bool ret;

    cmd_size = DIPC_H2D_HEAD_LEN + 9 + src_coder_format_size + 4 + snk_coder_format_size;

    cmd_buf = os_mem_zalloc2(cmd_size);

    if (cmd_buf == NULL)
    {
        return false;
    }

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_CODEC_PIPE_CREATE);
    DIPC_LE_UINT16_TO_STREAM(p, cmd_size - DIPC_H2D_HEAD_LEN);
    DIPC_LE_UINT32_TO_STREAM(p, session_id);
    DIPC_LE_UINT8_TO_STREAM(p,  data_mode);
    DIPC_LE_UINT8_TO_STREAM(p,  src_coder_id);
    DIPC_LE_UINT8_TO_STREAM(p,  src_frame_num);
    DIPC_LE_UINT16_TO_STREAM(p, src_coder_format_size);

    memcpy(p, src_coder_format, src_coder_format_size);
    p += src_coder_format_size;

    DIPC_LE_UINT8_TO_STREAM(p,  snk_coder_id);
    DIPC_LE_UINT8_TO_STREAM(p,  snk_frame_num);
    DIPC_LE_UINT16_TO_STREAM(p, snk_coder_format_size);

    memcpy(p, snk_coder_format, snk_coder_format_size);
    p += snk_coder_format_size;

    // DIPC_PRINT_TRACE2("dipc_codec_pipe_create: len: 0x%x, cmd_buf(%b)", cmd_size, TRACE_BINARY(cmd_size,
    //                   cmd_buf));
    ret = dipc2_cmd_send(cmd_buf, p - cmd_buf, true);

    os_mem_free(cmd_buf);

    cmd_buf = NULL;

    return ret;
}

bool dipc_codec_pipe_destroy(uint32_t session_id)
{
    uint8_t cmd_buf[8];
    uint8_t *p;

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_CODEC_PIPE_DESTROY);
    DIPC_LE_UINT16_TO_STREAM(p, 4);
    DIPC_LE_UINT32_TO_STREAM(p, session_id);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}

bool dipc_codec_pipe_start(uint32_t session_id)
{
    uint8_t cmd_buf[8];
    uint8_t *p;

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_CODEC_PIPE_START);
    DIPC_LE_UINT16_TO_STREAM(p, 4);
    DIPC_LE_UINT32_TO_STREAM(p, session_id);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}

bool dipc_codec_pipe_stop(uint32_t session_id)
{
    uint8_t cmd_buf[8];
    uint8_t *p;

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_CODEC_PIPE_STOP);
    DIPC_LE_UINT16_TO_STREAM(p, 4);
    DIPC_LE_UINT32_TO_STREAM(p, session_id);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}

bool dipc_codec_pipe_gain_set(uint32_t session_id,
                              uint16_t gain_step_left,
                              uint16_t gain_step_right)
{
    uint8_t cmd_buf[12];
    uint8_t *p;

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_CODEC_PIPE_GAIN_SET);
    DIPC_LE_UINT16_TO_STREAM(p, 8);
    DIPC_LE_UINT32_TO_STREAM(p, session_id);
    DIPC_LE_UINT16_TO_STREAM(p, gain_step_left);
    DIPC_LE_UINT16_TO_STREAM(p, gain_step_right);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}

bool dipc_codec_pipe_pre_mixer_add(uint32_t prime_session_id,
                                   uint32_t auxiliary_session_id)
{
    uint8_t cmd_buf[12];
    uint8_t *p;

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_CODEC_PIPE_PRE_MIXER_ADD);
    DIPC_LE_UINT16_TO_STREAM(p, 8);
    DIPC_LE_UINT32_TO_STREAM(p, prime_session_id);
    DIPC_LE_UINT32_TO_STREAM(p, auxiliary_session_id);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}

bool dipc_codec_pipe_post_mixer_add(uint32_t prime_session_id,
                                    uint32_t auxiliary_session_id)
{
    uint8_t cmd_buf[12];
    uint8_t *p;

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_CODEC_PIPE_POST_MIXER_ADD);
    DIPC_LE_UINT16_TO_STREAM(p, 8);
    DIPC_LE_UINT32_TO_STREAM(p, prime_session_id);
    DIPC_LE_UINT32_TO_STREAM(p, auxiliary_session_id);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}

bool dipc_codec_pipe_mixer_remove(uint32_t prime_session_id,
                                  uint32_t auxiliary_session_id)
{
    uint8_t cmd_buf[12];
    uint8_t *p;

    p = cmd_buf;

    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_CODEC_PIPE_MIXER_REMOVE);
    DIPC_LE_UINT16_TO_STREAM(p, 8);
    DIPC_LE_UINT32_TO_STREAM(p, prime_session_id);
    DIPC_LE_UINT32_TO_STREAM(p, auxiliary_session_id);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}

bool dipc_pipe_asrc_set(uint32_t session_id,
                        int32_t  ratio)
{
    uint8_t  cmd_buf[12];
    uint8_t *p;

    p = cmd_buf;
    DIPC_LE_UINT16_TO_STREAM(p, DIPC_H2D_CODEC_PIPE_ASRC_SET);
    DIPC_LE_UINT16_TO_STREAM(p, 8);
    DIPC_LE_UINT32_TO_STREAM(p, session_id);
    DIPC_LE_UINT32_TO_STREAM(p, ratio);

    return dipc2_cmd_send(cmd_buf, p - cmd_buf, true);
}
