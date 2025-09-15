/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_DONGLE_RECORD_H_
#define _APP_DONGLE_RECORD_H_


#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
  *                           Header Files
  *============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/** @defgroup APP_RWS_DONGLE
  * @brief
  * @{
  */

#define BD_ADDR_LENGTH       6

typedef enum
{
    RECORD_SAMPLE_RATE_16K,
    RECORD_SAMPLE_RATE_32K,
    RECORD_SAMPLE_RATE_48K,
    RECORD_SAMPLE_RATE_MAX,
} T_RECORD_SAMPLE_RATE;

/**
    * @brief        get rtp voice data len
    * @return       voice data len
    */
uint16_t app_dongle_get_voice_data_len(void);

/**
    * @brief        assemble rtp voice pkt
    * @pram[in]     allocate buf size
    * @pram[in]     voice data len
    * @return       actual voice data len in ring buf
    */
uint16_t app_dongle_assemble_rtp_voice(uint8_t *buf, uint16_t buf_size, uint16_t voice_data_len);

/**
    * @brief        This function can stop the record.
    * @return       void
    */
void app_dongle_stop_recording(void);

/**
    * @brief        This function can start the record.
    * @return       void
    */
void app_dongle_start_recording(void);

/**
    * @brief        This function can get current dongle record sample rate.
    * @return       dongle record sample rate
    */
T_RECORD_SAMPLE_RATE app_dongle_get_record_sample_rate(void);

void app_dongle_record_init(void);

void app_dongle_volume_in_mute(void);

void app_dongle_volume_in_unmute(void);
bool app_dongle_get_record_status(void);


bool app_dongle_get_record_state(void);

/** @} End of APP_RWS_DONGLE */

#ifdef __cplusplus
}
#endif

#endif //_VOICE_SPP_H_
