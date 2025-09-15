/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CHATGPT_PROCESS_H_
#define _APP_CHATGPT_PROCESS_H_

typedef enum
{
    CHATGPT_NONE                            = 0x00,
    CHATGPT_CMD_VOICE_START                 = 0x01,
    CHATGPT_CMD_VOICE_DATA                  = 0x02,
    CHATGPT_CMD_VOICE_END                   = 0x03,
    CHATGPT_EVT_WHISPER_DATA                = 0x04,
    CHATGPT_EVT_TEXT_DATA                   = 0x05,
    CHATGPT_EVT_VOICE_DATA_START            = 0x06,
    CHATGPT_EVT_VOICE_DATA                  = 0x07,
    CHATGPT_EVT_VOICE_DATA_STOP             = 0x08,
    CHATGPT_EVT_RX_BUFFER_CTRL              = 0x0B,
} T_CHATGPT_ID;

typedef enum
{
    CHATGPT_STATUS_IDLE                     = 0x00,
    CHATGPT_STATUS_OPUS_START               = 0x01,
    CHATGPT_STATUS_OPUS_CREATE              = 0x02,
    CHATGPT_STATUS_OPUS_ENCODE_START        = 0x03,
    CHATGPT_STATUS_OPUS_ENCODE_STOP         = 0x04,
    CHATGPT_STATUS_OPUS_STOP                = 0x05,
    CHATGPT_STATUS_VOICE_START              = 0x06,
    CHATGPT_STATUS_VOICE_STOP               = 0x07,
} T_CHATGPT_STATUS;

typedef struct
{
    uint8_t codec_type;
    uint8_t msg_id;
    uint8_t channel;
    uint8_t sample_rate[4];

    uint8_t whisper: 1;
    uint8_t text: 1;
    uint8_t ble: 1;
    uint8_t a2dp: 1;
    uint8_t rsv: 4;
} T_CHATGPT_START_VOICE;

typedef struct
{
    uint8_t codec_type;
    uint8_t msg_id;
    uint8_t time_stamp[6];
    uint8_t sample_rate[4];

    uint8_t block_num;
    uint8_t channel_mode;
    uint8_t alloc_method;
    uint8_t subband_num;
} T_CHATGPT_VOICE_DAT_START;

typedef struct
{
    uint8_t voice;
} T_CHATGPT_STOP_VOICE;

typedef struct
{
    uint8_t method;
    uint8_t frame_low;
    uint8_t frame_high;
} T_CHATGPT_FRAME_CTRL;

typedef union t_chatgpt_evt_dat
{
    T_CHATGPT_VOICE_DAT_START                voice_data_start;
} T_CHATGPT_EVT_DAT;


void app_chatgpt_process_start(void);
void app_chatgpt_process_stop(void);
void app_chatgpt_process_ack_check(uint16_t seq);
void app_chatgpt_process_event_handle(uint8_t *data, uint16_t len, uint8_t *need_ack);
void app_chatgpt_process_init(void);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
