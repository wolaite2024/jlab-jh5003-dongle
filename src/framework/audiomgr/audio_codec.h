/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_CODEC_H_
#define _AUDIO_CODEC_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

uint8_t audio_codec_chann_num_get(T_AUDIO_FORMAT_TYPE type, void *attr);

uint16_t audio_codec_frame_size_get(T_AUDIO_FORMAT_TYPE type, void *attr);

uint32_t audio_codec_frame_duration_get(T_AUDIO_FORMAT_TYPE type, void *attr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_CODEC_H_ */
