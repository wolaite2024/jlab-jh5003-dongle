/**
*****************************************************************************************
*     Copyright (C) 2021 Realtek Semiconductor Corporation.
*****************************************************************************************
  * @file
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
 *                      Define to prevent recursive inclusion
 *============================================================================*/

#ifndef _UAC_SILENCE_DETECT_
#define _UAC_SILENCE_DETECT_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include "string.h"
#include "trace.h"
#include "app_io_msg.h"

/*============================================================================*
 *                         Macros
 *============================================================================*/
#if (UAC_SILENCE_DETECT_SUPPORT == 1)

/*============================================================================*
 *                         Types
 *============================================================================*/
enum
{
    UAC_SILENCE_STATE_MUTE,
    UAC_SILENCE_STATE_UNMUTE,
};
/*============================================================================*
 *                         Functions
 *============================================================================*/
void uac_silence_detect_proc(uint8_t *data, uint16_t length);
void silence_detect_set_hid_state(bool hid_state);
void uac_set_silence_threhold(uint32_t value);
void app_handle_silence_detect_event(T_IO_MSG *io_msg);
bool uac_get_silence_state(void);
void silence_detect_set_hid_state(bool state);
void app_silence_detect_init(void);
bool uac_stream_is_stream_suspend(void);
void slience_set_stream_suspend(bool flag);
void uac2_silence_detect_proc(uint8_t *data, uint16_t length);
void app_handle_uac2_silence_detect_event(T_IO_MSG *io_msg);
bool uac2_get_silence_state(void);

#endif
#ifdef __cplusplus
}
#endif

#endif  // _UAC_SILENCE_DETECT_

