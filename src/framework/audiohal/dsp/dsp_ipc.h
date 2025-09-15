/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#ifndef _DSP_IPC_H_
#define _DSP_IPC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/* DIPC Opcode Indicator Field (1 bit) */
#define DIPC_OIF_LEGACY             (0x00 << 15)
#define DIPC_OIF_CURRENT            (0x01 << 15)
#define DIPC_OIF_GET(opcode)        ((opcode) & 0x8000)

/* DIPC Opcode Domain Field (3-bit mask) */
#define DIPC_ODF_UNSPECIFIED        (0x00 << 12)
#define DIPC_ODF_ENTITY_1           (0x01 << 12)
#define DIPC_ODF_ENTITY_2           (0x02 << 12)
#define DIPC_ODF_ENTITY_3           (0x04 << 12)
#define DIPC_ODF_GET(opcode)        ((opcode) & 0x7000)

/* DIPC Opcode Group Field (4 bits) */
#define DIPC_OGF_INFORMATION        (0x00 << 8)
#define DIPC_OGF_CONFIGURATION      (0x01 << 8)
#define DIPC_OGF_TESTING            (0x02 << 8)
#define DIPC_OGF_SYNCHRONIZATION    (0x03 << 8)
#define DIPC_OGF_CODEC_PIPE         (0x04 << 8)
#define DIPC_OGF_CODER              (0x05 << 8)
#define DIPC_OGF_NOTIFICATION       (0x06 << 8)
#define DIPC_OGF_SIGNAL_PROCESS     (0x07 << 8)
#define DIPC_OGF_AUDIO_EFFECT       (0x08 << 8)
#define DIPC_OGF_VENDOR_SPECIFIC    (0x0F << 8)
#define DIPC_OGF_GET(opcode)        ((opcode) & 0x0F00)

/* DIPC Opcode Command Field (8 bits) */
#define DIPC_OCF_GET(opcode)        ((opcode) & 0x00FF)

/* DIPC H2D Information Command Codes */
#define DIPC_H2D_CONTROLLER_VERSION_READ        (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_INFORMATION | 0x00)
#define DIPC_H2D_CONTROLLER_CAPABILITIES_READ   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_INFORMATION | 0x01)
#define DIPC_H2D_HOST_VERSION_REPLY             (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_INFORMATION | 0x02)

/* DIPC H2D Configuration Command Codes */
#define DIPC_H2D_GATE_SET                       (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CONFIGURATION | 0x00)
#define DIPC_H2D_GATE_CLEAR                     (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CONFIGURATION | 0x01)
#define DIPC_H2D_GATE_START                     (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CONFIGURATION | 0x02)
#define DIPC_H2D_GATE_STOP                      (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CONFIGURATION | 0x03)
#define DIPC_H2D_MCPS_REPLY                     (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CONFIGURATION | 0x04)

/* DIPC H2D Testing Command Codes */
/* TBD: DIPC Spec r03 */

/* DIPC H2D Synchronization Command Codes */
#define DIPC_H2D_TIMESTAMP_SET                  (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SYNCHRONIZATION | 0x00)
#define DIPC_H2D_SESSION_COUPLE                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SYNCHRONIZATION | 0x01)
#define DIPC_H2D_SESSION_DECOUPLE               (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SYNCHRONIZATION | 0x02)
#define DIPC_H2D_SESSION_ROLE_SET               (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SYNCHRONIZATION | 0x03)
#define DIPC_H2D_SESSION_JOIN                   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SYNCHRONIZATION | 0x04)
#define DIPC_H2D_INFORMATION_RELAY_REPLY        (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SYNCHRONIZATION | 0x05)
#define DIPC_H2D_INFORMATION_RELAY_APPLY        (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SYNCHRONIZATION | 0x06)

/* DIPC H2D Codec Pipe Command Codes */
#define DIPC_H2D_CODEC_PIPE_CREATE              (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x00)
#define DIPC_H2D_CODEC_PIPE_DESTROY             (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x01)
#define DIPC_H2D_CODEC_PIPE_START               (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x02)
#define DIPC_H2D_CODEC_PIPE_STOP                (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x03)
#define DIPC_H2D_CODEC_PIPE_GAIN_SET            (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x04)
#define DIPC_H2D_CODEC_PIPE_ASRC_SET            (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x05)
#define DIPC_H2D_CODEC_PIPE_PRE_MIXER_ADD       (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x10)
#define DIPC_H2D_CODEC_PIPE_POST_MIXER_ADD      (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x11)
#define DIPC_H2D_CODEC_PIPE_MIXER_REMOVE        (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x12)
#define DIPC_H2D_CODEC_PIPE_PRE_TEE_SPLIT       (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x20)
#define DIPC_H2D_CODEC_PIPE_POST_TEE_SPLIT      (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x21)
#define DIPC_H2D_CODEC_PIPE_TEE_MERGE           (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODEC_PIPE | 0x22)

/* DIPC H2D Coder (Decoder and Encoder) Command Codes */
#define DIPC_H2D_DECODER_SET                      (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x00)
#define DIPC_H2D_DECODER_CLEAR                    (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x01)
#define DIPC_H2D_DECODER_START                    (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x02)
#define DIPC_H2D_DECODER_STOP                     (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x03)
#define DIPC_H2D_DECODER_GAIN_SET                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x04)
#define DIPC_H2D_DECODER_GAIN_RAMP                (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x05)
#define DIPC_H2D_DECODER_ASRC_SET                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x06)
#define DIPC_H2D_DECODER_SIGNAL_MONITORING_START  (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x07)
#define DIPC_H2D_DECODER_SIGNAL_MONITORING_STOP   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x08)
#define DIPC_H2D_ENCODER_SET                      (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x80)
#define DIPC_H2D_ENCODER_CLEAR                    (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x81)
#define DIPC_H2D_ENCODER_START                    (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x82)
#define DIPC_H2D_ENCODER_STOP                     (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x83)
#define DIPC_H2D_ENCODER_GAIN_SET                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x84)
#define DIPC_H2D_ENCODER_SIGNAL_MONITORING_START  (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x85)
#define DIPC_H2D_ENCODER_SIGNAL_MONITORING_STOP   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_CODER | 0x86)

/* DIPC H2D Notification Command Codes */
#define DIPC_H2D_RINGTONE_SET                   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x00)
#define DIPC_H2D_RINGTONE_CLEAR                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x01)
#define DIPC_H2D_RINGTONE_START                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x02)
#define DIPC_H2D_RINGTONE_STOP                  (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x03)
#define DIPC_H2D_RINGTONE_GAIN_SET              (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x04)
#define DIPC_H2D_VOICE_PROMPT_SET               (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x80)
#define DIPC_H2D_VOICE_PROMPT_CLEAR             (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x81)
#define DIPC_H2D_VOICE_PROMPT_START             (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x82)
#define DIPC_H2D_VOICE_PROMPT_STOP              (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x83)
#define DIPC_H2D_VOICE_PROMPT_GAIN_SET          (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_NOTIFICATION | 0x84)

/* DIPC H2D Signal Process Command Codes */
#define DIPC_H2D_LINE_IN_SET                    (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x00)
#define DIPC_H2D_LINE_IN_CLEAR                  (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x01)
#define DIPC_H2D_LINE_IN_START                  (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x02)
#define DIPC_H2D_LINE_IN_STOP                   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x03)
#define DIPC_H2D_LINE_IN_DAC_GAIN_SET           (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x04)
#define DIPC_H2D_LINE_IN_ADC_GAIN_SET           (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x05)
#define DIPC_H2D_LINE_IN_GAIN_RAMP              (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x06)
#define DIPC_H2D_APT_SET                        (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x20)
#define DIPC_H2D_APT_CLEAR                      (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x21)
#define DIPC_H2D_APT_START                      (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x22)
#define DIPC_H2D_APT_STOP                       (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x23)
#define DIPC_H2D_APT_DAC_GAIN_SET               (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x24)
#define DIPC_H2D_APT_ADC_GAIN_SET               (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x25)
#define DIPC_H2D_APT_GAIN_RAMP                  (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x26)
#define DIPC_H2D_VAD_SET                        (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x40)
#define DIPC_H2D_VAD_CLEAR                      (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x41)
#define DIPC_H2D_VAD_START                      (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x42)
#define DIPC_H2D_VAD_STOP                       (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_SIGNAL_PROCESS | 0x43)

/* DIPC H2D Audio Effect Command Codes */
#define DIPC_H2D_AUDIO_EQ_SET                   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x00)
#define DIPC_H2D_AUDIO_EQ_CLEAR                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x01)
#define DIPC_H2D_VOICE_EQ_SET                   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x02)
#define DIPC_H2D_VOICE_EQ_CLEAR                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x03)
#define DIPC_H2D_RECORD_EQ_SET                  (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x04)
#define DIPC_H2D_RECORD_EQ_CLEAR                (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x05)
#define DIPC_H2D_APT_EQ_SET                     (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x06)
#define DIPC_H2D_APT_EQ_CLEAR                   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x07)
#define DIPC_H2D_EFFECT_CONTROL                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x0A)
#define DIPC_H2D_SIDETONE_SET                   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x10)
#define DIPC_H2D_SIDETONE_CLEAR                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x11)
#define DIPC_H2D_VOICE_NREC_SET                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x20)
#define DIPC_H2D_VOICE_NREC_CLEAR               (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x21)
#define DIPC_H2D_APT_NREC_SET                   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x22)
#define DIPC_H2D_APT_NREC_CLEAR                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x23)
#define DIPC_H2D_APT_NREC_MODE_SET              (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x24)
#define DIPC_H2D_APT_NREC_LEVEL_SET             (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x25)
#define DIPC_H2D_AUDIO_WDRC_SET                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x40)
#define DIPC_H2D_AUDIO_WDRC_CLEAR               (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x41)
#define DIPC_H2D_VOICE_WDRC_SET                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x42)
#define DIPC_H2D_VOICE_WDRC_CLEAR               (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x43)
#define DIPC_H2D_APT_WDRC_SET                   (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x44)
#define DIPC_H2D_APT_WDRC_CLEAR                 (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x45)
#define DIPC_H2D_APT_OVP_SET                    (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x50)
#define DIPC_H2D_APT_OVP_CLEAR                  (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x51)
#define DICP_H2D_APT_BEAMFORMING_SET            (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x60)
#define DICP_H2D_APT_BEAMFORMING_CLEAR          (DIPC_OIF_CURRENT | DIPC_ODF_UNSPECIFIED | DIPC_OGF_AUDIO_EFFECT | 0x60)

/* DIPC Event Indicator Field (1 bit) */
#define DIPC_EIF_LEGACY             (0x00 << 15)
#define DIPC_EIF_CURRENT            (0x01 << 15)
#define DIPC_EIF_GET(opcode)        ((opcode) & 0x8000)

/* DIPC Event Domain Field (3-bit mask) */
#define DIPC_EDF_UNSPECIFIED        (0x00 << 12)
#define DIPC_EDF_ENTITY_1           (0x01 << 12)
#define DIPC_EDF_ENTITY_2           (0x02 << 12)
#define DIPC_EDF_ENTITY_3           (0x04 << 12)
#define DIPC_EDF_GET(opcode)        ((opcode) & 0x7000)

/* DIPC Event Group Field (4 bits) */
#define DIPC_EGF_INFORMATION        (0x00 << 8)
#define DIPC_EGF_CONFIGURATION      (0x01 << 8)
#define DIPC_EGF_TESTING            (0x02 << 8)
#define DIPC_EGF_SYNCHRONIZATION    (0x03 << 8)
#define DIPC_EGF_CODEC_PIPE         (0x04 << 8)
#define DIPC_EGF_CODER              (0x05 << 8)
#define DIPC_EGF_NOTIFICATION       (0x06 << 8)
#define DIPC_EGF_SIGNAL_PROCESS     (0x07 << 8)
#define DIPC_EGF_AUDIO_EFFECT       (0x08 << 8)
#define DIPC_EGF_AUDIO_STATUS       (0x09 << 8)
#define DIPC_EGF_VENDOR_SPECIFIC    (0x0F << 8)
#define DIPC_EGF_GET(opcode)        ((opcode) & 0x0F00)

/* DIPC Event Command Field (8 bits) */
#define DIPC_ECF_GET(opcode)        ((opcode) & 0x00FF)

/* DIPC D2H Information Command Codes */
#define D2H_INFORMATION_EXCHANGE_COMMAND_ACK        (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_INFORMATION | 0x00)
#define D2H_INFORMATION_EXCHANGE_COMMAND_COMPLETE   (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_INFORMATION | 0x01)
#define D2H_HOST_VERSION_REQUEST                    (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_INFORMATION | 0x02)

/* DIPC D2H Configuration Command Codes */
#define D2H_CONFIGURE_CONTROL_COMMAND_ACK           (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CONFIGURATION | 0x00)
#define D2H_CONFIGURE_CONTROL_COMMAND_COMPLETE      (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CONFIGURATION | 0x01)
#define D2H_MCPS_REQUEST                            (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CONFIGURATION | 0x02)

/* DIPC D2H Testing Command Codes */
/* TBD: DIPC Spec r03 */

/* DIPC D2H Synchronization Command Codes */
#define D2H_SYNCHRONIZATION_COMMAND_ACK             (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SYNCHRONIZATION | 0x00)
#define D2H_SYNCHRONIZATION_COMMAND_COMPLETE        (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SYNCHRONIZATION | 0x01)
#define D2H_TIMESTAMP_LOSE                          (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SYNCHRONIZATION | 0x02)
#define D2H_SYNCHRONIZATION_LATCH                   (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SYNCHRONIZATION | 0x03)
#define D2H_SYNCHRONIZATION_UNLATCH                 (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SYNCHRONIZATION | 0x04)
#define D2H_JOIN_INFO_REPORT                        (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SYNCHRONIZATION | 0x05)
#define D2H_INFO_RELAY_REQUEST                      (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SYNCHRONIZATION | 0x06)

/* DIPC D2H Codec Pipe Command Codes */
#define D2H_CODEC_PIPE_COMMAND_ACK                  (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODEC_PIPE | 0x00)
#define D2H_CODEC_PIPE_COMMAND_COMPLETE             (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODEC_PIPE | 0x01)
#define D2H_CODEC_PIPE_DATA_ACK                     (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODEC_PIPE | 0x02)
#define D2H_CODEC_PIPE_DATA_INDICATE                (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODEC_PIPE | 0x03)

/* DIPC D2H Coder (Decoder and Encoder) Command Codes */
#define D2H_DECODER_COMMAND_ACK                     (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODER | 0x00)
#define D2H_DECODER_COMMAND_COMPLETE                (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODER | 0x01)
#define D2H_DECODER_DATA_ACK                        (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODER | 0x02)
#define D2H_DECODER_DATA_COMPLETE                   (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODER | 0x03)
#define D2H_ENCODER_COMMAND_ACK                     (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODER | 0x10)
#define D2H_ENCODER_COMMAND_COMPLETE                (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODER | 0x11)
#define D2H_ENCODER_DATA_COMPLETE                   (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODER | 0x12)
#define D2H_ENCODER_SIGNAL_REFRESH                  (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_CODER | 0x13)

/* DIPC D2H Notification Command Codes */
#define D2H_RINGTONE_COMMAND_ACK                    (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_NOTIFICATION | 0x00)
#define D2H_RINGTONE_COMMAND_COMPLETE               (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_NOTIFICATION | 0x01)
#define D2H_VOICE_PROMPT_COMMAND_ACK                (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_NOTIFICATION | 0x10)
#define D2H_VOICE_PROMPT_COMMAND_COMPLETE           (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_NOTIFICATION | 0x11)
#define D2H_VOICE_PROMPT_DATA_ACK                   (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_NOTIFICATION | 0x12)
#define D2H_VOICE_PROMPT_DATA_COMPLETE              (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_NOTIFICATION | 0x13)

/* DIPC D2H Signal Process Command Codes */
#define D2H_LINE_IN_COMMAND_ACK                     (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SIGNAL_PROCESS | 0x00)
#define D2H_LINE_IN_COMMAND_COMPLETE                (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SIGNAL_PROCESS | 0x01)
#define D2H_APT_COMMAND_ACK                         (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SIGNAL_PROCESS | 0x02)
#define D2H_APT_COMMAND_COMPLETE                    (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SIGNAL_PROCESS | 0x03)
#define D2H_VAD_COMMAND_ACK                         (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SIGNAL_PROCESS | 0x04)
#define D2H_VAD_COMMAND_COMPLETE                    (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_SIGNAL_PROCESS | 0x05)

/* DIPC D2H Status Report Command Codes */
#define D2H_COMMAND_ACK                             (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_AUDIO_STATUS | 0x00)
#define D2H_COMMAND_COMPLETE                        (DIPC_EIF_CURRENT | DIPC_EDF_UNSPECIFIED | DIPC_EGF_AUDIO_STATUS | 0x01)

/* DIPC Event Error Codes (8 bits) */
#define DIPC_ERROR_SUCCESS                  0x00
#define DIPC_ERROR_HARDWARE_FAILURE         0x01
#define DIPC_ERROR_UNKNOWN_COMMAND          0x02
#define DIPC_ERROR_UNKNOWN_SESSION_ID       0x03
#define DIPC_ERROR_INVALID_COMMAND_PARAM    0x04
#define DIPC_ERROR_COMMAND_DISALLOWED       0x05
#define DIPC_ERROR_MEM_CAPACITY_EXCEEDED    0x06
#define DIPC_ERROR_UNSUPPORTED_FEATURE      0x07
#define DIPC_ERROR_CONTROLLER_BUSY          0x08
#define DIPC_ERROR_INVALID_OPERATION_MODE   0x09
#define DIPC_ERROR_UNSPECIFIED_REASON       0xFF

/* DIPC Audio Category (8 bits) */
#define DIPC_AUDIO_CATEGORY_AUDIO           0x00
#define DIPC_AUDIO_CATEGORY_VOICE           0x01
#define DIPC_AUDIO_CATEGORY_RECORD          0x02
#define DIPC_AUDIO_CATEGORY_LINE_IN         0x03
#define DIPC_AUDIO_CATEGORY_RINGTONE        0x04
#define DIPC_AUDIO_CATEGORY_VOICE_PROMPT    0x05
#define DIPC_AUDIO_CATEGORY_APT             0x06
#define DIPC_AUDIO_CATEGORY_LLAPT           0x07
#define DIPC_AUDIO_CATEGORY_ANC             0x08
#define DIPC_AUDIO_CATEGORY_VAD             0x09

/* DIPC Coder ID (8 bits) */
#define DIPC_CODER_ID_PCM                   0x00
#define DIPC_CODER_ID_CVSD                  0x01
#define DIPC_CODER_ID_MSBC                  0x02
#define DIPC_CODER_ID_SBC                   0x03
#define DIPC_CODER_ID_AAC                   0x04
#define DIPC_CODER_ID_OPUS                  0x05
#define DIPC_CODER_ID_FLAC                  0x06
#define DIPC_CODER_ID_MP3                   0x07
#define DIPC_CODER_ID_LC3                   0x08
#define DIPC_CODER_ID_LDAC                  0x09
#define DIPC_CODER_ID_LHDC                  0x0A
#define DIPC_CODER_ID_G729                  0x0B

/* DIPC Audio Route Logical IO (8 bits) */
#define DIPC_LOGICAL_IO_AUDIO_PRIMARY_SPK           0x00
#define DIPC_LOGICAL_IO_AUDIO_SECONDARY_SPK         0x01
#define DIPC_LOGICAL_IO_AUDIO_REFERENCE_SPK         0x02

#define DIPC_LOGICAL_IO_VOICE_PRIMARY_SPK           0x10
#define DIPC_LOGICAL_IO_VOICE_SECONDARY_SPK         0x11
#define DIPC_LOGICAL_IO_VOICE_REFERENCE_SPK         0x12
#define DIPC_LOGICAL_IO_VOICE_REFERENCE_MIC         0x13
#define DIPC_LOGICAL_IO_VOICE_PRIMARY_MIC           0x14
#define DIPC_LOGICAL_IO_VOICE_SECONDARY_MIC         0x15
#define DIPC_LOGICAL_IO_VOICE_FUSION_MIC            0x16
#define DIPC_LOGICAL_IO_VOICE_BONE_MIC              0x17

#define DIPC_LOGICAL_IO_RECORD_REFERENCE_MIC        0x20
#define DIPC_LOGICAL_IO_RECORD_PRIMARY_MIC          0x21
#define DIPC_LOGICAL_IO_RECORD_SECONDARY_MIC        0x22
#define DIPC_LOGICAL_IO_RECORD_LEFT_AUX             0x23
#define DIPC_LOGICAL_IO_RECORD_RIGHT_AUX            0x24

#define DIPC_LOGICAL_IO_LINE_IN_PRIMARY_SPK         0x30
#define DIPC_LOGICAL_IO_LINE_IN_SECONDARY_SPK       0x31
#define DIPC_LOGICAL_IO_LINE_IN_REFERENCE_SPK       0x32
#define DIPC_LOGICAL_IO_LINE_IN_REFERENCE_MIC       0x33
#define DIPC_LOGICAL_IO_LINE_IN_LEFT_AUX_IN         0x34
#define DIPC_LOGICAL_IO_LINE_IN_RIGHT_AUX_IN        0x35

#define DIPC_LOGICAL_IO_RINGTONE_PRIMARY_SPK        0x40
#define DIPC_LOGICAL_IO_RINGTONE_SECONDARY_SPK      0x41
#define DIPC_LOGICAL_IO_RINGTONE_REFERENCE_SPK      0x42

#define DIPC_LOGICAL_IO_VOICE_PROMPT_PRIMARY_SPK    0x50
#define DIPC_LOGICAL_IO_VOICE_PROMPT_SECONDARY_SPK  0x51
#define DIPC_LOGICAL_IO_VOICE_PROMPT_REFERENCE_SPK  0x52

#define DIPC_LOGICAL_IO_APT_PRIMARY_SPK             0x60
#define DIPC_LOGICAL_IO_APT_SECONDARY_SPK           0x61
#define DIPC_LOGICAL_IO_APT_REFERENCE_SPK           0x62
#define DIPC_LOGICAL_IO_APT_REFERENCE_MIC           0x63
#define DIPC_LOGICAL_IO_APT_PRIMARY_LEFT_MIC        0x64
#define DIPC_LOGICAL_IO_APT_PRIMARY_RIGHT_MIC       0x65
#define DIPC_LOGICAL_IO_APT_SECONDARY_LEFT_MIC      0x66
#define DIPC_LOGICAL_IO_APT_SECONDARY_RIGHT_MIC     0x67

#define DIPC_LOGICAL_IO_LLAPT_PRIMARY_SPK           0x70
#define DIPC_LOGICAL_IO_LLAPT_SECONDARY_SPK         0x71
#define DIPC_LOGICAL_IO_LLAPT_REFERENCE_SPK         0x72
#define DIPC_LOGICAL_IO_LLAPT_REFERENCE_MIC         0x73
#define DIPC_LOGICAL_IO_LLAPT_LEFT_MIC              0x74
#define DIPC_LOGICAL_IO_LLAPT_RIGHT_MIC             0x75

#define DIPC_LOGICAL_IO_ANC_PRIMARY_SPK             0x80
#define DIPC_LOGICAL_IO_ANC_SECONDARY_SPK           0x81
#define DIPC_LOGICAL_IO_ANC_REFERENCE_SPK           0x82
#define DIPC_LOGICAL_IO_ANC_REFERENCE_MIC           0x83
#define DIPC_LOGICAL_IO_ANC_FF_LEFT_MIC             0x84
#define DIPC_LOGICAL_IO_ANC_FF_RIGHT_MIC            0x85
#define DIPC_LOGICAL_IO_ANC_FB_LEFT_MIC             0x86
#define DIPC_LOGICAL_IO_ANC_FB_RIGHT_MIC            0x87

#define DIPC_LOGICAL_IO_VAD_REFERENCE_MIC           0x90
#define DIPC_LOGICAL_IO_VAD_PRIMARY_MIC             0x91
#define DIPC_LOGICAL_IO_VAD_SECONDARY_MIC           0x92

/* DIPC Header Sync Word (32 bits) */
#define DIPC_SYNC_WORD              0x3F3F3F3F

/* DIPC Header Tail (32 bits) */
#define DIPC_TAIL                   0xFFFFFFFF

/* DIPC Session ID Type (2 bits) */
#define DIPC_SESSION_TYPE_CODEC_PIPE        0x00
#define DIPC_SESSION_TYPE_DECODER           0x01
#define DIPC_SESSION_TYPE_ENCODER           0x02
#define DIPC_SESSION_TYPE_SIGNAL_PROCESS    0x03

/* DIPC Data Status (8 bits) */
#define DIPC_DATA_STATUS_CORRECT            0x00
#define DIPC_DATA_STATUS_ERROR              0x01
#define DIPC_DATA_STATUS_LOST               0x02
#define DIPC_DATA_STATUS_DUMMY              0x03

/* DIPC Data Mode (8 bits) */
#define DIPC_DATA_MODE_NORMAL       0x00
#define DIPC_DATA_MODE_DIRECT       0x01

/* DIPC GATE DIRECTION */
#define DIPC_DIRECTION_TX           (0x0)
#define DIPC_DIRECTION_RX           (0x1)
#define DIPC_DIRECTION_MAX          (0x2)

/* DIPC GATE ID */
#define DIPC_GATE_ID0              (0x0)
#define DIPC_GATE_ID1              (0x1)
#define DIPC_GATE_ID2              (0x2)
#define DIPC_GATE_MAX              (0x3)

typedef struct t_dipc_gate_status
{
    uint8_t gate_id;
    uint8_t dir_bit;
    bool    ready;
} T_DIPC_GATE_STATUS;

typedef struct t_dipc_pcm_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
} T_DIPC_PCM_CODER_FORMAT;

typedef struct t_dipc_cvsd_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
} T_DIPC_CVSD_CODER_FORMAT;

typedef struct t_dipc_msbc_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
    uint8_t  chann_mode;
    uint8_t  block_length;
    uint8_t  subband_num;
    uint8_t  allocation_method;
    uint8_t  bitpool;
    uint8_t  paddings[3];
} T_DIPC_MSBC_CODER_FORMAT;

typedef struct t_dipc_sbc_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
    uint8_t  chann_mode;
    uint8_t  block_length;
    uint8_t  subband_num;
    uint8_t  allocation_method;
    uint8_t  bitpool;
    uint8_t  paddings[3];
} T_DIPC_SBC_CODER_FORMAT;

typedef struct t_dipc_aac_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
    uint8_t  transport_format;
    uint8_t  object_type;
    uint8_t  vbr;
    uint8_t  paddings;
    uint32_t bitrate;
} T_DIPC_AAC_CODER_FORMAT;

typedef struct t_dipc_opus_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
    uint8_t  cbr;
    uint8_t  cvbr;
    uint8_t  mode;
    uint8_t  complexity;
    uint32_t bitrate;
} T_DIPC_OPUS_CODER_FORMAT;

typedef struct t_dipc_mp3_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
    uint8_t  chann_mode;
    uint8_t  version;
    uint8_t  layer;
    uint32_t bitrate;
} T_DIPC_MP3_CODER_FORMAT;

typedef struct t_dipc_lc3_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
    uint8_t  frame_length;
    uint16_t paddings;
    uint32_t chann_location;
    uint32_t presentation_delay;
} T_DIPC_LC3_CODER_FORMAT;

typedef struct t_dipc_ldac_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
    uint8_t  chann_mode;
    uint8_t  paddings[3];
} T_DIPC_LDAC_CODER_FORMAT;

typedef struct t_dipc_flac_coder_format
{
    uint32_t paddings;
} T_DIPC_FLAC_CODER_FORMAT;

typedef struct t_dipc_lhdc_coder_format
{
    uint32_t sample_rate;
    uint16_t frame_size;
    uint8_t  chann_num;
    uint8_t  bit_width;
} T_DIPC_LHDC_CODER_FORMAT;

typedef struct t_dipc_g729_coder_format
{
    uint32_t paddings;
} T_DIPC_G729_CODER_FORMAT;


typedef struct t_dipc_codec_pipe_create_cmpl
{
    uint8_t  status;
    uint32_t session_id;
    uint32_t src_transport_address;
    uint32_t src_transport_size;
    uint32_t snk_transport_address;
    uint32_t snk_transport_size;
} T_DIPC_CODEC_PIPE_CREATE_CMPL;

bool dipc_gate_start(uint8_t id, uint8_t dir_bit);
bool dipc_gate_stop(uint8_t id, uint8_t dir_bit);

bool dipc_codec_pipe_create(uint32_t session_id,
                            uint8_t  data_mode,
                            uint8_t  src_coder_id,
                            uint8_t  src_frame_num,
                            uint16_t src_coder_format_size,
                            uint8_t *src_coder_format,
                            uint8_t  snk_coder_id,
                            uint8_t  snk_frame_num,
                            uint16_t snk_coder_format_size,
                            uint8_t *snk_coder_format);
bool dipc_codec_pipe_destroy(uint32_t session_id);
bool dipc_codec_pipe_start(uint32_t session_id);
bool dipc_codec_pipe_stop(uint32_t session_id);
bool dipc_codec_pipe_gain_set(uint32_t session_id,
                              int16_t  gain_step_left,
                              int16_t  gain_step_right);
bool dipc_codec_pipe_pre_mixer_add(uint32_t prime_session_id,
                                   uint32_t auxiliary_session_id);
bool dipc_codec_pipe_post_mixer_add(uint32_t prime_session_id,
                                    uint32_t auxiliary_session_id);
bool dipc_codec_pipe_mixer_remove(uint32_t prime_session_id,
                                  uint32_t auxiliary_session_id);
bool dipc_pipe_asrc_set(uint32_t session_id,
                        int32_t  ratio);

/* OLD */
#define PKT_TYPE_SCO                0
#define PKT_TYPE_A2DP               1
#define PKT_TYPE_VOICE_PROMPT       2
#define PKT_TYPE_LOST_PACKET        3
#define PKT_TYPE_IOT_DATA           4
#define PKT_TYPE_RAW_AUDIO_DATA     5
#define PKT_TYPE_ZERO_PACKET        6
#define PKT_TYPE_DUMMY              0xFF

#define AAC_TYPE_LATM_NORMAL        0x00
#define AAC_TYPE_LATM_SIMPLE        0x01
#define AAC_TYPE_ADTS               0x02
#define AAC_TYPE_ADIF               0x03
#define AAC_TYPE_RAW                0x04

#define STREAM_CHANNEL_OUTPUT_MONO          0
#define STREAM_CHANNEL_OUTPUT_STEREO        1

#define MSBC_FRAME_SIZE                 120
#define SBC_FRAME_SIZE                  256     // L+R
#define AACLC_FRAME_SIZE                2048    // L+R
#define MP3_FRAME_SIZE                  (1152*2)

#define DSP_SPORT0_READY                    0x10
#define DSP_SPORT0_STOP                     0x11
#define DSP_SPORT1_READY                    0x12
#define DSP_SPORT1_STOP                     0x13

#define D2H_FADE_OUT_COMPLETE_SIZE          0
#define D2H_CODEC_STATE_SIZE                1
#define D2H_BOOT_DONE_SIZE                  0
#define D2H_BOOT_QUERY_SIZE                 1

//for D2H_DSP_STATUS_IND type
#define D2H_IND_TYPE_RINGTONE               0
#define D2H_IND_TYPE_FADE_IN                1
#define D2H_IND_TYPE_FADE_OUT               2
#define D2H_IND_TYPE_DECODE_EMPTY           3
#define D2H_IND_TYPE_PROMPT                 4
#define D2H_IND_TYPE_KEYWORD                6
#define D2H_IND_TYPE_SEG_SEND               10

//for D2H_IND_TYPE_RINGTONE status
#define D2H_IND_STATUS_RINGTONE_STOP        0
//for D2H_IND_TYPE_FADE_IN status
#define D2H_IND_STATUS_FADE_IN_COMPLETE     0
//for D2H_IND_TYPE_FADEOUT status
#define D2H_IND_STATUS_FADE_OUT_COMPLETE    0
//for D2H_IND_TYPE_PROMPT status
#define D2H_IND_STATUS_PROMPT_REQUEST       0
#define D2H_IND_STATUS_PROMPT_FINISH        1
#define D2H_IND_STATUS_PROMPT_DECODE_ERROR  2
//for D2H_IND_TYPE_SEG_SEND
#define D2H_IND_STATUS_SEG_SEND_REQ_DATA    0
#define D2H_IND_STATUS_SEG_SEND_ERROR       1

#define DIPC_CH_MODE_MONO                   (0)
#define DIPC_CH_MODE_STEREO                 (1)

/** @brief  DSP log output path.*/
typedef enum
{
    DSP_OUTPUT_LOG_NONE = 0x0,          //!< no DSP log.
    DSP_OUTPUT_LOG_BY_UART = 0x1,       //!< DSP log by uart directly.
    DSP_OUTPUT_LOG_BY_MCU = 0x2,        //!< DSP log by MCU.
} T_DSP_OUTPUT_LOG;

typedef enum
{
    DSP_IPC_DUMMY_PKT_STOP,
    DSP_IPC_DUMMY_PKT_START,
} T_DUMMY_PKT_CMD;

typedef struct t_dsp_ipc_action_ack
{
    uint32_t h2d_id: 16;
    uint32_t result: 8;
    uint32_t rsvd : 8;
} T_DSP_IPC_ACTION_ACK;

typedef enum
{
    DSP_IPC_ACTION_STOP   = 0x0,
    DSP_IPC_ACTION_START  = 0x1,
} T_DSP_IPC_ACTION;

typedef enum
{
    //D2H mailbox
    MAILBOX_D2H_SHARE_QUEUE_ADDRESS     = 0x01,
    MAILBOX_D2H_DSP_LOG_BUFFER          = 0x02,
    MAILBOX_D2H_DSP_ADCDAC_DATA0        = 0x04,
    MAILBOX_D2H_DSP_ADCDAC_DATA1        = 0X05,
    MAILBOX_D2H_CHECK                   = 0x06,

    MAILBOX_D2H_WATCHDOG_TIMEOUT        = 0xFE,
    MAILBOX_D2H_DSP_EXCEPTION           = 0xFF
} T_SHM_MAILBOX_D2H;

typedef enum
{
    DSP_STREAM_REC_PLAY_STOP,
    DSP_STREAM_REC_START,
    DSP_STREAM_PLAY_START,
    DSP_STREAM_REC_PLAY_START,
    DSP_STREAM_REC_STOP,
    DSP_STREAM_PLAY_STOP,
} T_STREAM_COMMAND;

typedef enum
{
    DSP_STREAM_AD_DA_READY,
    DSP_STREAM_AD_READY,
    DSP_STREAM_DA_READY,
} T_STREAM_STATUS;

typedef enum
{
    ALGORITHM_PROPRIETARY_VOICE,    //00
    ALGORITHM_G711_ALAW,            //01
    ALGORITHM_CVSD,                 //02
    ALGORITHM_MSBC,                 //03
    ALGORITHM_OPUS_VOICE,           //04
    ALGORITHM_LC3_VOICE,            //05
    ALGORITHM_USB_SPEECH,           //06
    ALGORITHM_SBC,                  //07
    ALGORITHM_AAC,                  //08
    ALGORITHM_PROPRIETARY_AUDIO1,   //09
    ALGORITHM_PROPRIETARY_AUDIO2,   //10
    ALGORITHM_MP3,                  //11
    ALGORITHM_USB_AUDIO,            //12
    ALGORITHM_LC3,                  //13
    ALGORITHM_LDAC,                 //14 SONY
    ALGORITHM_UHQ,                  //15 Samsung
    ALGORITHM_LHDC,                 //16 Savitech
    ALGORITHM_OPUS_AUDIO,           //17
    ALGORITHM_FLAC,                 //18
    ALGORITHM_PURE_STREAM,          //19, ALGORITHM_PROPRIETARY_AUDIO4
    ALGORITHM_LINE_IN,              //20, MUST before ALGORITHM_END
    ALGORITHM_END                   //21
} T_ALGORITHM_TYPE;

typedef enum
{
    DSP_DATA_NORMAL_MODE,
    DSP_DATA_LOW_LATENCY_MODE,
    DSP_DATA_HIGH_STABILITY,
    DSP_DATA_DIRECT_MODE,
    DSP_DATA_ULTRA_LOW_LATENCY_MODE,
} T_DSP_DATA_MODE;

typedef enum
{
    DSP_FIFO_TX_DIRECTION,
    DSP_FIFO_RX_DIRECTION,
} T_DSP_FIFO_TRX_DIRECTION;

typedef struct
{
    uint16_t    data_len;
    uint8_t     *p_data;
} T_DSP_MAILBOX_DATA;

typedef enum
{
    FADE_ACTION_NONE,
    FADE_ACTION_IN,
    FADE_ACTION_OUT,
} T_FADE_ACTION;

typedef struct
{
    uint32_t version        : 4;
    uint32_t action         : 2;
    uint32_t fade_action    : 2;
    uint32_t vad_hw         : 1;
    uint32_t vad_kws        : 1;
    uint32_t rsvd           : 22;
} T_ACTION_CONFIG;

typedef struct t_dsp_ipc_logic_io_tlv
{
    uint32_t category    : 4;
    uint32_t logic_io_id : 8;
    uint32_t location    : 4;
    uint32_t sport_id    : 4;
    uint32_t rtx         : 4;
    uint32_t channel     : 4;
    uint32_t polarity    : 1;
    uint32_t rsvd        : 3;
} T_DSP_IPC_LOGIC_IO_TLV;

typedef struct t_dsp_ipc_logic_io
{
    uint32_t version : 4;
    uint32_t tlv_cnt : 8;
    uint32_t rsvd    : 20;
    T_DSP_IPC_LOGIC_IO_TLV *tlv;
} T_DSP_IPC_LOGIC_IO;

typedef struct t_dsp_ipc_sport_cfg
{
    uint8_t version             ;
    uint8_t sport_id            ;
    uint8_t rtx                 ;
    uint8_t channel_count       ;
    uint8_t data_length         ;
    uint8_t role                ;
    uint8_t channel_mode        ;
    uint8_t bridge              ;
    uint32_t sample_rate        ;
} T_DSP_IPC_SPORT_CFG;

/**
  * @brief D2H command IDs
*/
typedef enum
{
    D2H_RESERVED = 0x00, //Command id: 0x00

    D2H_DSP_STATE = 0x01, //Command id: 0x01
    /* 1 word: Reporting current DSP/codec state
     * 4' d0: DSP_RESET_STATE_READY // analog codec is power off
     * 4' d1: DSP_SLEEP_STATE_READY
     * 4' d2: DSP_STANDBY_STATE_READY // I2S interface is disabled MCU can load codes to DSP to swtich application scenarios, such as SCO ??A2DP, SCO ??LineIn.
     * 4' d3: DSP_ACTIVE_STATE_READY // DSP is active, playing/recording SPK/MIC signal
     *
     */

    D2H_DSP_STATUS_IND = 0x02, //Command id: 0x02
    /*
    bit[7:0] : Indication Type
    0: Ringtone
    1: Fade in
    2: Fade out
    3: Decode empty
    4: Voice prompt

    bit[15:8] : Indication Status
    */

    D2H_SILENCE_DETECT_REPORT = 0x03, //Command id: 0x03
    /*
     * Assume initial state not_silence
     * DSP report silence status when condition is changed
     * bit 0: 1 for Spk path is silence, 0 for Spk path is not silence
     *
     */

    D2H_DSP_POWER_DOWN_ACK = 0x04, //Command id: 0x04
    /*
     * No data report
     */

    D2H_RWS_SLAVE_SYNC_STATUS = 0x07, //Command id: 0x07
    /*
     *  Word 0: [31:0] Loop_n
        Word 1: [31:0] error of samples (Q4)
        Word 2: [31:0] :
                (-3)RWS  reSync
                (-2)Spk1 timestamp lose
                (-1)I2S FIFO empty
                (0) initial condition
                (1) rws sync. unlock
                (2) rws sync. Lock
                (3) rws RWSV2 Seamless OK
     */
    D2H_DSP_FW_ROM_VERSION_ACK = 0x08, //Command id: 0x08
    /*
     * 2 words for version report
     * Word1: Bit[31:8]: IC part number
     * Word1: Bit[7:0]: FW ROM version
     * Word2: Bit{31:0]: 0
     */

    D2H_DSP_FW_RAM_VERSION_ACK = 0x09, //Command id: 0x09
    /*
     * 2 words for version report
     * Word1: Bit[31:8]: IC part number
     * Word1: Bit[7:0]: FW ROM version
     * Word2: Bit[31:16]: FW RAM version
     */

    D2H_DSP_FW_PATCH_VERSION_ACK = 0x0A, //Command id: 0x0A
    D2H_DSP_FW_SDK_VERSION_ACK = 0x0B, //Command id: 0x0B
    /*
     * 1 word for version report
     * Word0: Bit[31:0]: SDK Version number
     */

    D2H_DSP_MIPS_REQIURED = 0x0C, //Command id: 0x0C
    /* Word 0: [31:0] mips required
    */

    D2H_DSP_SET_DAC_GAIN = 0x11, //Command id:0x11
    /*
     * Byte 0: Ch0 DAC gain
     * Byte 1: Ch1 DAC gain
     * Byte 2: Reserved (Ch2 DAC gain)
     * Byte 3: Reserved (Ch3 DAC gain)
     */

    D2H_DSP_SET_ADC_GAIN = 0x12, //Command id:0x12
    /*
     * Byte 0: Ch0 ADC gain
     * Byte 1: Ch1 ADC gain
     * Byte 2: Reserved (Ch2 ADC gain)
     * Byte 3: Reserved (Ch3 ADC gain)
     * Byte 4: Reserved (Ch4 ADC gain)
     * Byte 5: Reserved (Ch5 ADC gain)
     * Byte 6: Reserved (Ch6 ADC gain)
     * Byte 7: Reserved (Ch7 ADC gain)
     */

    D2H_B2BMSG_INTERACTION_SEND = 0x14, //Command id: 0x14

    D2H_EARTIP_FIT_RESULT = 0x16,  //Command id: 0x16
    /*
    * DSP return Ear Tip fitting result to MCU
    *
    * Byte[0]: 0:Unknown  1:EarTip_Fit_Good  2:EarTip_Fit_Bad
    */

    D2H_RWS_SEAMLESS_RETURN_INFO = 0x17,  //Command id: 0x17
    /*
     * Word0:Secondary Next I2S Start Btclock (Unit:312.5us)
     * Word1:Secondary Next I2S Start Seq 0~65535. StartSeq = CurrentSeq + Interval
     * Word2:Secondary Current Seq 0~255
     */

    D2H_AUDIOPLAY_VOLUME_INFO = 0x18,   //Command id: 0x18
    /*
     *  //D2H_AUDIOPLAY_VOLUME_INFO
     *  Word[0]: [0 :15]  :L_channel_volume (dB)
     *  Word[0]: [16:32]  :R_channle_volume (dB)
     *
     */

    D2H_DSP_PLC_FRAME_NUM = 0x19,                  //Command id: 0x19
    /*
     * return currently number of PLC frame
     *
     * Word[0]: Sum of PLC frame  (uint32_t) [0~2^31]
     * Word[1]: currently frame Number (uint16_t)
     * Word[2]: currently AVDTP Sequence Number (uint16_t)
     * Word[3]: Number of frames in currently PLC Pkt
     */

    D2H_DSP2_MIPS_REQIURED = 0x1F, //

    /* Word 0: [31:0] DSP2 mips required

    */

    D2H_ACTION_ACK = 0x20,

    D2H_REPORT_STREAM_LATENCY = 0x30,
    /*
     *    report Stream latency to MCU
     *
     *    element:
     *    0. received X packets. X comes from 0x5B_H2D parameter
     *    1. every_packet_latency = (decode_current_clock - packet_received_timestamp), Unit is Slot(625us)
     *    2. normal_packet_count =  X - (AudioPlc packet_cnt, zero packet_cnt ...),
     *    3. average_packet_latency = sum( every_packet_latency(1:normal packet count) ) / normal packet count
     *
     *    return Word[0]
     *    Word[0]:Bit[0 :15]: uint16: normal_packet_count.
     *    Word[0]:Bit[16:31]: u
     *    Word[1]:Bit[0 :31]:  uint32: average fifo queuing,
     */

    D2H_UPLINK_SYNCREF_REQUEST = 0x31, //request Sync Reference depend on session id
    /*
     *   word0: session id, let mcu to find correspond conn_handle
     */

    D2H_DECODER_PLC_NOTIFY = 0x32,
    /*
     *    report PLC Status to MCU
     *
     *    element:
     *    0. PlcSampleNum: The sample count of DSP Plced in the interval set by MCU
     *    1. TotalSampleNum: The total sample count played in the interval set by MCU
     *    2. ContinueSampleNum: The sample count of the continuous PLC
     *
     *    return Word[0]
     *    Word[2]:Bit[0 :31]: uint32: PlcSampleNum.
     *    Word[2]:Bit[0 :31]: uint32: TotalSampleNum.
     *    Word[2]:Bit[0 :31]: uint32: ContinueSampleNum.
     */

    D2H_OPEN_AIR_AVC = 0x7E,

    D2H_HA_VER_INFO = 0xB0,

    D2H_DSP_SDK_GENERAL_CMD = 0xC0,
    /* Word 0: Command payload pointer
    */

    D2H_SPORT_VERIFY_RESULT = 0xD0,

    D2H_SCENARIO_STATE = 0xE0,

    D2H_DSP_BOOT_DONE = 0xF0,
    D2H_BOOT_READY = 0xF1,
    D2H_DSP_DBG = 0xF2,
    BID_SHM_LOOPBACK_TEST_ACK = 0xF3,
    BID_SHM_LOOPBACK_MPC_TEST_ACK = 0xF4,
    D2H_FT_TEST_REPORT = 0xF5,
    D2H_TEST_RESP = 0xF6,
} T_SHM_CMD_D2H;

/**
  * @brief H2D command IDs
*/
typedef enum
{
    H2D_EMPTY = 0x00, //Command id: 0x00

    H2D_DSP_CONFIG = 0x01, //Command id: 0x01
    /*
     * 1 byte : DSP_CLK_Rate (MHz)
     * Parameter length unit : 32 bit
    */

    H2D_TEST_MODE = 0x02, //Command id: 0x02
    /*
     * Byte[0]: test_type
     * 0: Normal mode
     * 1: MIC TEST (1Mic/2Mic both can use)
     * 2: Dual Mic Beamforming TEST (only for 2Mic test)
     * 3: MIC with effect off TEST (1Mic/2Mic both can use)
     *
     * Byte[1]: para_len, 0~255. Currently not used
     *
     * Byte[2~257]: para
     *  test_type = 2, Dual Mic Beamforming TEST
     *      byte[2] = 0, Normal mode (Enable all effect except the effect bypass by DSP config tool)
     *      byte[2] = 1, Only 2MIC Beamforming and disable other effect
     *  test_type = 3, MIC with effect off TEST
     *      byte[2] = 0, Normal mode(Enable all effect except the effect bypass by DSP config tool)
     *      byte[2] = 1, MIC0 Only and disable all effect
     *      byte[2] = 2, MIC1 Only and disable all effect
    */

    H2D_TONE_GEN_CONFIG = 0x08, //Command id: 0x08
    /* Command Packet
        This message command is to deliver the default ringtone prompt
        The ringtone can be a mixture of two sinusoid tone with individual amplitude weighting.
        MCU provide desired tone frequency and DSP shall generate its sine tone based on various DAC sampling rate setting.
        3 Words:
          Word 0:
        Bit[15:0]: tone1 frequency(Hz). i.e. 0x200 denotes 512Hz.
        Bit[31:16]: tone1 amplitude weighting ranging from 1 to 0. 0x7FFF (0.9999)~
        0x0000 (0.0)
          Word 1:
        Bit[15:0]: tone2 frequency(Hz). i.e. 0x200 denotes 512Hz.
        Bit[31:16]: tone2 amplitude weighting ranging from 1 to 0. 0x7FFF(0.9999)~
        0x0000 (0.0)
          Word 2:
            Bit [15:0]: ringtone period in msec. DSP shall convert this setting according to DAC
        sampling rate.
            Bit [23:16]: Fade-In time in msec.
            Bit [23:16]: Fade-Out time in msec.
    */

    H2D_ADC_GAIN = 0x0D, //Command id: 0x0D
    /* Command Packet
        MCU informs DSP to change the ADC gain level. This action only to configure
        1 Word:
          Byte[0] : ADC gain
    */
    //  #define ICODEC_ATTdb(db,pdb)    ((db*10+pdb)/15)

    H2D_FADE_IN_OUT_CONTROL = 0x0E, //Command id: 0x0E
    /* Command Packet
      00 byte : Command
      01 byte : Tx Path Fade In Out Step (AY0 > 0 Fade In, AY0<0 Fad Out)
         Default Value :
            txFadeStep  = 0  (Disable)
         Noted :
            txFadeStep  ==> 0 = Diable
                        1 = 683 mSec FadeIn @ 48K
                        2 = 341 mSec FadeIn @ 48K
                        3 = 227 mSec FadeIn @ 48K
                          :
                        6 = 114 mSec FadeIn @ 48K   (recommanded)
                          :
                       -1 = 683 msec FadeOut @ 48K
                       -2 = 341 msec FadeOut @ 48K
                       -3 = 227 msec FadeOut @ 48K
                          :
                       -6 = 114 mSec FadeOut @ 48K  (recommanded)
                          :
                          ==> 32768/SampleRate/txFadeStep
    */

    H2D_SBC_ENCODE_HDR_CONFIG = 0x0F, //Command id: 0x0F
    /*
     * 1 byte : HDR
     * 2 byte : bitpool
     * 3 byte : frame number
     *
     * sample frequency : HDR bit 7:6
     * 00 : 16k
     * 01 : 32k
     * 10 : 44.1k
     * 11 : 48k
     * ===========================
     * blocks : HDR bit 5:4
     * 00 : 4
     * 01 : 8
     * 10 : 12
     * 11 : 16
     * ============================
     * channel mode : HDR bit 3:2
     * 00 : mono
     * 01 : dual channel
     * 10 : stereo
     * 11 : joint stereo
     * ============================
     * allocation method : HDR bit 1
     * 0 : loundness
     * 1 : SNR
     * ============================
     * subbands : HDR bit 0
     * 0 : 4
     * 1 : 8
    */

    H2D_CUSTOMER_INFO_CONFIG = 0x13, //Command id: 0x13
    /* customer command
    */

    H2D_DSP_AUDIO_SPORT0_ACTION = 0x15, //Command id: 0x15
    /*
    bit[3:0]
    4'd0: DSP Audio stop
    This action informs DSP to stop current audio/speech application activity.
    DSP does the following thing
    1.  Start the fade-out process
    2.  At the end of fade-out process, clear queued packet buffer and intermediate FIFO.

    4'd1: REC_START
    This action informs DSP to configure MIC path only for BT audio dongle or MIC applications.
    DSP does the following things:
    1.  Configure the FIFO
    2.  Configure SBC encoder
    3.  Configure signal processing function.
    4.  Configure the packet aggregation configuration.
    5.  Configure settings of analog audio codec
    6.  Configure I2S interface

    4'd2: CMD_PLAY_START
    This action informs DSP to configure SPK path only for BT audio music playback applications.
    DSP does the following things:
    1.  Configure the FIFO
    2.  Configure audio decoder
    3.  Configure signal processing function.
    4.  Configure the packet aggregation configuration.
    5.  Configure settings of analog audio codec
    6.  Configure I2S interface

    4'd3: CMD_REC_PLAY_SART
    This action informs DSP to configure SPK path only for BT speech/IoT applications.
    DSP does the following things:
    1.  Configure the FIFO
    2.  Configure decoder/encoder
    3.  Configure signal processing function.
    4.  Configure the packet aggregation configuration.
    5.  Configure settings of analog audio codec
    6.  Configure I2S interface
    */

    H2D_TX_PATH_RAMP_GAIN_CONTROL = 0x16, //Command id: 0x16
    /*
    bit[7:0]
         TX Path Gain idx (0~15)
         [0  -2  -4  -6 -9 -12 -15 -18 -21 -27 -33 -39 -45 -70 -200]; Attenuation dB % 0:15
    bit[15:8] : Ramp Gain Time
        0: 50mSec
        1: 100mSec
        2: 150mSec
    */

    H2D_CMD_ACTION_CTRL = 0x17, //Command id: 0x17
    /*
    bit[7:0]
    0: Ringtone Ctrl

    bit[15:8] : action
    0: stop ringtone
    */

    H2D_CMD_LINE_IN_START = 0x18, //Command id: 0x18

    H2D_CMD_PROMPT_CONFIG = 0x19, //Command id: 0x19
    /*
    total length bit[31:0]

    default: 0x331800 -> Mix disable
    default: 0x331810 -> Mix enable

    bit[3:0] : Prompt_Coder_type
    0: AAC Voice Prompt
    1: SBC Voice Prompt
    2: MP3 Voice Prompt
    other reserved

    bit[7:4] : Mix option
    0: Mix_disable
    1: Mix_enable
    other reserved

    bit[11:8]: Prompt file Frequency
    0: 96000;
    1: 88200;
    2: 64000;
    3: 48000;
    4: 44100;
    5: 32000;
    6: 24000;
    7: 22050;
    8: 16000;
    9: 12000;
    10:11025;
    11:8000;
    other reserved

    bit[15:12]: Channel number;
    0: ignore decode
    1: Mono;
    2: Stereo;
    other reserved

    bit[19:16]: Prompt attenuation gain;
    0: normal
    1: attenuation 1dB
    2: attenuation 2dB
    ...
    15: attenuation 15dB
    other reserved

    bit[23:20]: Path attenuation gain;
    0: normal
    1: attenuation 1dB
    2: attenuation 2dB
    ...
    15: attenuation 15dB
    other reserved
    */

    H2D_CMD_PROMPT_ACTION = 0x1A, //Command id: 0x1A
    /*
      Bit[15:0]: Frame Length byte N, and indicate Prompt active
         => If  Frame Length N = 0xFFFF, indicate Prompt File already End
      Bit[N+15: 16]: Raw data, max 800 byte
         => If  Frame Length N = 0xFFFF, Raw data =>empty
    */

    H2D_CMD_LOG_OUTPUT_SEL = 0x1B, //Command id: 0x1B
    /*
    Bit[7:0]: DSP Log output selection
            0: DSP Log disabled
            1: DSP Log output to UART directly
            2: DSP Log is forward to MCU to output

    Bit[15:8]: valid if Bit[7:0] == 0x0
            0: DSP send log to SHM, then use DMA output the log
            1: DSP send log to Uart Register directly

    Bit[23:16]: valid if Bit[15:8] == 0x1
            0: DSP use DMA to output log
            1: DSP write uart Register directly
    */

    H2D_DSP_POWER_DOWN = 0x1E, //Command id: 0x1E
    /*
     * Null to ask DSP ack only
     */

    H2D_CMD_DSP_DAC_ADC_DATA_TO_MCU = 0x1F, //Command id: 0x1F
    /*
    Bit[ 7: 0]: DAC DATA TO MCU or Not
            0: DSP Don't Send DAC DATA to MCU
            1: DSP Send DAC DATA to MCU
    Bit[15: 8]: ADC DATA TO MCU or Not
            0: DSP Don't Send ADC DATA to MCU
            1: DSP Send ADC DATA to MCU
    */

    H2D_CALL_STATUS = 0x21, //Command id: 0x21
    /*
    Byte 0: SCO call status
            0: Call not active
            1: Call active
     */

    H2D_DSP_FW_ROM_VERSION = 0x22, //Command id: 0x22
    /*
     */

    H2D_DSP_FW_RAM_VERSION = 0x23, //Command id: 0x23
    /*
     */

    H2D_DSP_FW_PATCH_VERSION = 0x24, //Command id: 0x24
    /*
     */

    H2D_RWS_INIT = 0x26, //Command id: 0x26
    /*
     * Reset RWS state and set Stamp index
        Word0:
        [3:0]: (0) Native
               (1) Piconet0
               (2) Piconet1
               (3) Piconet2
               (4) Piconet3
        Word1: [31:0] :I2S Sampling Rate idx(0~6)
           (0)8000,  (1)16000, (2)32000, (3)44100,  (4)48000, (5)88200, (6)96000,
     *
     */

    H2D_RWS_SET_ASRC_RATIO = 0x28, //Command id: 0x28
    /*  Word 0:
            Bit [31:0]: ASRC Dejitter ratio;
    */

    H2D_RWS_SET = 0x29, //Command: 0x29
    /*
     * Set device is Master Speaker or Slave Speaker
        Word0:
        [0]: (1) Master Speaker enable; (0)Disable
        [4]: (1) Slave  Speaker enable; (0)Disable
    */

    H2D_SEND_BT_ADDR = 0x2E, //Command id: 0x2E
    /*
     *   Send BT Address to DSP
     *   Byte [5:0]: BT Address
     *
     */

    H2D_RWS_SEAMLESS = 0x2F, //Command id: 0x2F
    /*   Word0: [31:0] (0) disable Seamless
     *                   (1) enable  Seamless  Master     *                   (2) enable  Seamless  Slave
     */

    H2D_ENCODER_SET = 0x30,//Command id: 0x30
    /*
        Word 0: Encoder and Rx ASRC configuration [default: 0x0078, default at mSBC mode]
          Bit [11:0]: Encoder frame size (unit : sample)
          Bit 12:rsvd
          Bit 13: channel mode
          Bit [20:16]: Encoder algorithm
            ALGORITHM_PROPRIETARY_VOICE,   //00
            ALGORITHM_G711_ALAW,           //01
            ALGORITHM_CVSD,                //02
            ALGORITHM_MSBC,                //03
            ALGORITHM_OPUS_VOICE,          //04
            ALGORITHM_UHQ_VOICE,           //05
            ALGORITHM_USB_SPEECH,          //06
            ALGORITHM_SBC,                 //07
            ALGORITHM_AAC,                 //08
            ALGORITHM_PROPRIETARY_AUDIO1,  //09
            ALGORITHM_PROPRIETARY_AUDIO2,  //10
            ALGORITHM_MP3,                 //11
            ALGORITHM_USB_AUDIO,           //12
            ALGORITHM_SUB_WOOFER,          //13
            ALGORITHM_LDAC,                //14 SONY
            ALGORITHM_UHQ,                 //15 Samsung
            ALGORITHM_LHDC,                //16 Savitech
            ALGORITHM_OPUS_AUDIO,          //17
            ALGORITHM_LINE_IN,             //18
            ALGORITHM_END                  //19

         Bit [22:21]: Bit width
                0x0: 16Bit
                0x1: 24Bit
                0x2: Reserved
                0x3: Reserved

         Bit [26:23]: encode sampling rate
         Bit [29:27]: AAC type
             0x0: AAC_TYPE_LATM_NORMAL
             0x1: AAC_TYPE_LATM_SIMPLE
             0x2: AAC_TYPE_ADTS
             0x3: AAC_TYPE_ADIF
             0x4: AAC_TYPE_RAW

         Bit 30: pass_through_enable
    */
    H2D_DECODER_SET = 0x31,//Command id: 0x31
    /*
        Word 0: Decoder and Tx ASRC configuration [default: 0x0178, default at mSBC mode]
          Bit[11:0] : Decoder frame size (unit : sample)
             Speech mode: CVSD: 30 or 60 samples, mSBC: 120 samples.
             Audio mode: SBC: 128 samples, LDAC 256 samples, AAC-LC : 1024 samples
          Bit 12: rsvd
          Bit 13:channel mode
          Bit [20:16]: Decoder algorithm
            ALGORITHM_PROPRIETARY_VOICE,   //00
            ALGORITHM_G711_ALAW,           //01
            ALGORITHM_CVSD,                //02
            ALGORITHM_MSBC,                //03
            ALGORITHM_OPUS_VOICE,          //04
            ALGORITHM_UHQ_VOICE,           //05
            ALGORITHM_USB_SPEECH,          //06
            ALGORITHM_SBC,                 //07
            ALGORITHM_AAC,                 //08
            ALGORITHM_PROPRIETARY_AUDIO1,  //09
            ALGORITHM_PROPRIETARY_AUDIO2,  //10
            ALGORITHM_MP3,                 //11
            ALGORITHM_USB_AUDIO,           //12
            ALGORITHM_SUB_WOOFER,          //13
            ALGORITHM_LDAC,                //14 SONY
            ALGORITHM_UHQ,                 //15 Samsung
            ALGORITHM_LHDC,                //16 Savitech
            ALGORITHM_OPUS_AUDIO,          //17
            ALGORITHM_LINE_IN,             //18
            ALGORITHM_END                  //19

         Bit [22:21]: Bit width
                0x0: 16Bit
                0x1: 24Bit
                0x2: Reserved
                0x3: Reserved

         Bit [26:23]: decode sampling rate
         Bit [29:27]: AAC type
             0x0: AAC_TYPE_LATM_NORMAL
             0x1: AAC_TYPE_LATM_SIMPLE
             0x2: AAC_TYPE_ADTS
             0x3: AAC_TYPE_ADIF
             0x4: AAC_TYPE_RAW
    */

    H2D_DAC_GAIN_STEREO = 0x32, //Command id: 0x32
    /*
     * Byte 0: L channel DAC gain
     * Byte 1: R channel DAC gain
     */

    H2D_TONE_GAIN = 0x33, //Command id: 0x33
    /* Command Packet
        MCU informs DSP to change the tone gain level. This action only to configure
        1 Word:
          Byte[0] : Codec gain
    */
    H2D_DECODER_SET2 = 0x38, //command id: 0x38
    H2D_DECODER_ACTION = 0x39,
    H2D_RESERVED3 = 0x3A, //command id: 0x3A
    H2D_ENCODER_SET2 = 0x3B, //command id: 0x3B
    H2D_ENCODER_ACTION = 0x3C,
    H2D_RESERVED1 = 0x3D, //command id: 0x3D
    /*
     * Request DSP get data from SPORT and send to MCU
     */
    H2D_B2BMSG_INTERACTION_RECEIVED = 0x3F,  //command id: 0x3F
    H2D_B2BMSG_INTERACTION_TIMEOUT = 0x40,    //command id: 0x40

    H2D_SIGNAL_PROC_START = 0x44,        //command id: 0x44
    /*
     * After H2D_DSP_AUDIO_ACTION 0x15 setting.
     * The parameter mean which timing to enable I2S.
     * Primary and Secondary should send this command to DSP announce enable time.
     * DSP will use BT_Isr to do this.
     *
     * Word0 [27:0]: bt_clock (unit 312.5ms for MCU)
     * If Word0 = 0xffff_ffff mean enable right now
     */

    H2D_SPORT_SET = 0x45,                //command id: 0x45  //BBpro2
    /*
     *  Before Audio_Action, should set sport Sampling Rate and Channel number,
     *  Ch0_Virtual_IOName reference  T_Virtual_IOName_TYPE
     *
     *  Payload:
     *  Word 0~2
     *
     *  typedef struct ==> reference SportCfg_T
     *  sizeof(SportCfg_T) = 12Byte
     */

    H2D_DSP_VOICE_TRIGGER_SET = 0x49, //command id: 0x49
    /*
     * 2 byte parameters
     * bit[0]    : 1: HW VAD enable, 0: disable
     * bit[1]    : 1: stereo TX (Neckband type), 0: mono TX (RWS type)
     * bit[2~7]  : reserved
     * bit[8]    : 1: KWS enable, 0:disable
     * bit[9~15] : reserved
     */

    H2D_DSP_VOICE_TRIGGER_ACTION = 0x4A,       //command id: 0x4A
    /*
     * 1 byte parameters
     * bit[0]    : 1: Voice Trigger continued, 0: Voice Trigger suspended
     * bit[1~7]  : reserved
     */

    H2D_HANDOVER_INFO = 0x4B, //command id: 0x4B
    /*
    * 2 byte parameters
    * bit[0~7]    : 2: role sec, 1:  role pri
    * bit[7~15]   : 1: handover start , 0: handover stop
    */

    H2D_APT_DAC_GAIN = 0x4C,                        //command id: 0x4C

    H2D_EARTIP_FIT_TEST_STATUS = 0x4D,    //command id: 0x4D
    /*
     * MCU request DSP to start Ear Tip fitting detection
     * Byte 0~3 : gain mismatch, response information
     *            Byte 0-> Bit 0: 1 if gain mismatch of L-ch is valid
     *                     B1: 1 if response of L-ch is valid
     *            Byte 2-> B0: 1 if gain mismatch of R-ch is valid, for BFR, B0=0
     *                     B1: 1 if response of R-ch is valid, for BFR, B1=0
     * Byte 4~135  : 4 bytes gain mismatch and 128 bytes response of LCH
     * Byte 136~267: 4 bytes gain mismatch and 128 bytes response of RCH
     *
     */

    H2D_FORCE_DUMMY_PKT = 0x4E,                        //command id: 0x4E
    /*
     * MCU request DSP to start put dummy packet output
     *
     * Byte[0]: 0:Stop   1:Start
     */

    H2D_CURRENTLY_VOLUME_REQUEST = 0x51,            //command id: 0x51
    /*
     *  return Audio Volume (dB) by D2H_AUDIOPLAY_VOLUME_INFO
     *
     *  //D2H_AUDIOPLAY_VOLUME_INFO
     *  Word[0]: [0 :15]  :L_channel_volume (dB)
     *  Word[0]: [16:32]  :R_channle_volume (dB)
     *
     */

    H2D_DSP_AUDIO_SPORT1_ACTION = 0x52,             //command id: 0x52
    /*
     * DSP configuration control for SPORT1
     *
     * Byte[0]:
     *     DSP_STREAM_REC_PLAY_STOP   = 0,
     *     DSP_STREAM_REC_START       = 1,
     *     DSP_STREAM_PLAY_START      = 2,
     *     DSP_STREAM_REC_PLAY_START  = 3,
     *     DSP_STREAM_REC_STOP        = 4,
     *     DSP_STREAM_PLAY_STOP       = 5,
     *
     */

    H2D_LOW_LATENCY_MODE = 0x53,                     //command id: 0x53
    /*
    * Word[0]: Bit[0:3]: (1) entering low latency mode, upgrade DSP clock
    *                                   (0) entering normal mode
    * Word[0]: Bit[4:7]: (1)enable Audio Plc,
    *                                   (0):off Audio Plc
    *
    * Should send this command after Audio Action, and before than 1st send audio stream.
    *
    */

    H2D_DSP_AUDIO_PASSTHROUGH_ACTION = 0x57,                //command id: 0x55  //BBpro2
    /*
     *  Set DSP Enable/Disabl DSP Audio Passthrough
     *  DSP will use Sport0 Rx loop back and add Tx stream
     *
     *  ex:
     *  H2D_DSP_AUDIO_PASSTHROUGH_ACTION(1) => H2D_DSP_AUDIO_ACTION(record_start)
     *  APT will starting mixed when record_start
     *
     *  ex:
     *  H2D_DSP_AUDIO_ACTION(record_start) => H2D_SIGNAL_LOOP_BACK_MIX(1)
     *  SportRx stream will feed to DSP, then APT starting mixed when LOOP_BACK_MIX(1)
     *
     *  Payload:
     *  Word[0]:  0: Off
     *
     *            1: DSP APT enable 1ch in, mixed to TxLch
     *            2: DSP APT enable 1ch in, mixed to TxLch and TxRch both
     *            3: DSP APT enable 1ch in, mixed to TxRch
     *            4: DSP APT enable 2ch in, mixed to Lch after processing to 1ch
     *            5: DSP APT enable 2ch in, RxLch mixed TxLch, RxRch mixed TxRch, for headset application.

     *
     */

    H2D_DSP_LINE_IN_ACTION = 0x56,                        //command id: 0x56  //BBpro2
    /*
     *  Set DSP Enable/Disabl DSP LineIn
     *  DSP will use Sport0 Rx loop back and add Tx stream
     *
     *  ex:
     *  H2D_DSP_LINE_IN_ACTION(1) => H2D_DSP_AUDIO_ACTION(record_start)
     *  APT will starting mixed when record_start
     *
     *  ex:
     *  H2D_DSP_AUDIO_ACTION(record_start) => H2D_DSP_LINE_IN_ACTION(1)
     *  SportRx stream will feed to DSP, then APT starting mixed when LOOP_BACK_MIX(1)
     *
     *  Payload:
     *  Word[0]:  0: Off
     *
     *            1: LineIn enable 2ch in,  RxLch mixed TxLch, RxRch mixed TxRch
     *
     */

    H2D_DSP_SDK_INIT = 0x58,

    H2D_AUX_ADC_GAIN = 0x5A,
    /*
     *   for AUX(LineIn) MicIn gain
     *
     *   int16_t: L_channel_gain (XdB*128) Q7
     *   int16_t: R_channle_gain (XdB*128) Q7
     *
     *   XdB = -128~30, -128 is mute, 0dB = multiply one(unmute).
     *
     */

    H2D_SET_STREAM_LATENCY_REPORT = 0x5b,  //
    /*
     *   Should send this command after Audio Action, and before than 1st send audio stream
     *   This command will use (0x30) D2H_REPORT_STREAM_LATENCY report latency.
     *   Word[0]: Bit[0:7]:
     *             (0): off latency report
     *             (x)(1~255): enable latency report, (x)latency report every x packets,
     *
     *
     */

    H2D_SET_DSP2_CLK = 0x5c,
    /*
     *   Word[0]: DSP2 clock set value
     *
     *
     */

    H2D_VOICE_ADC_POST_GAIN = 0x5D,
    /*
     *   For record spp uplink / Hfp Voice uplink post gain
     *
     *   int16_t: L_channel_gain (XdB*128) Q7
     *   int16_t: R_channle_gain (XdB*128) Q7
     *
     *   XdB = -128~30, -128 is mute, 0dB = multiply one(unmute).
     */

    H2D_UPLINK_SYNCREF_STAMP = 0x5E, //LeAudio send Sync Reference stamp to DSP
    /*
     *  word0: session id, let mcu to find correspond conn_handle
     *  Payload structure defined on Rws2_LeAudio.h
     *  word0: session id
     *  word1~: structure of LeIso_SyncRef_AppInfo_T or LeAcl_AnchorPoint_AppInfo_T
     */

    H2D_RECORD_ADC_GAIN = 0x5F,
    /*
     *   for record MicIn/AuxIn gain, ex: AuxIn-->LC3 encode(dongle)
     *
     *   int16_t: L_channel_gain (XdB*128) Q7
     *   int16_t: R_channle_gain (XdB*128) Q7
     *
     *   XdB = -128~30, -128 is mute, 0dB = multiply one(unmute).
     *
     */

    H2D_MIC_NEAREND_PROC_SET = 0x60,
    /*
     * MCU set Echo Cancellation & Noise Reduction
     *
     * Word 0
     * Byte[0] : 0  (Disable EC & NR)
     *           1  (Enable EC & NR)
     */

    H2D_VOICE_SIDETONE_SET = 0x62,

    H2D_SEG_SEND = 0x63,

    H2D_HA_PARA = 0x70,

    H2D_FIFO_REQUEST = 0x81,
    /*
     *   uint16_t[0]
     *     0: FIFO_TYPE_TX_FIFO
     *     1: FIFO_TYPE_RX_FIFO
     *
     *   uint16_t[1]
     *     0: FIFO_FEATURE_LEN_NORMAL
     *     1: FIFO_FEATURE_LEN_LOW
     *
     */

    H2D_FIFO_EMPTY_REPORT_SET = 0x82,
    /*
     *   uint8_t[0]
     *     0: disable empty report
     *     1: enalbe empty report
     *
     */

    H2D_DATA_MODE = 0x83,
    /*
     *   uint8_t[0]
     *     0: Normal data mode
     *     1: Low Latency data mode
     *     2: High Stability data mode
     *     3: Direct data mode
     *     4: Ultra Low Latency data mode
     *
     *   uint8_t[1]
     *     0: AUDIO_STREAM_TYPE_PLAYBACK
     *     1: AUDIO_STREAM_TYPE_VOICE
     *     2: AUDIO_STREAM_TYPE_RECORD
     *
     */

    H2D_DECODER_PLC_SET = 0x85,
    /*
     *   for DSP PLC Control
     *
     *   uint8_t: enable (1) or disable (0) PLC
     *   uint16_t: interval for PLC Notify, unit: ms
     */

    H2D_EFFECT_CONTROL = 0x84,

    H2D_STREAM_CHANNEL_OUT_CONFIG = 0xD0,
    /*
     * byte[0] :
     * 0 : No LR Mixing at the SPK path, L->L, R->R
     * 1 : L->L, L->R
     * 2 : R->L, R->R
     * 3 : (L+R)/2->L, (L+R)/2->R
     * 4 : R->L, L->R
     */

    H2D_STREAM_CHANNEL_IN_CONFIG = 0xD1,
    /*
     * byte[0] :
     * 0 : No LR Mixing at the MIC path, L->L, R->R
     * 1 : L->L, L->R
     * 2 : R->L, R->R
     * 3 : (L+R)/2->L, (L+R)/2->R
     * 4 : R->L, L->R
     */

    H2D_DUMMY_PKT = 0xEF,

    H2D_BOOT_QUERY = 0xF0,
    H2D_DEBUG_QUERY = 0xF1,
    H2D_BOOT_CONFIG = 0xF2,

    H2D_SPPCAPTURE_SET = 0x0F01,

    //0x1000, Command for scenario 0 //A2DP
    H2D_SCENARIO0_PARA_PARSING = 0x1000,
    H2D_S0_GENERAL_EQ_PARA_MIC = 0x1001,
    H2D_S0_GENERAL_EQ_PARA_SPK = 0x1002,
    H2D_S0_CODEC_PARA_PARSING = 0x1003,
    H2D_S0_PARAMETRIC_EQ_MIC = 0x1004,
    H2D_S0_PARAMETRIC_EQ_SPK = 0x1005,
    // command content defined in dsp_sys_scenario_config.h
    H2D_S0_SYS_CONFIG = 0x1006,
    H2D_S0_PARAMETRIC_EQ_SPK_2 = 0x1007,

    H2D_S0_PARAMETRIC_NEWEQ_SPK = 0x1008, //0x1008
    H2D_S0_PARAMETRIC_NEWEQ_MIC = 0x1009, //0x1009

    H2D_AUDIO_EQ_PARA = 0x100A,
    H2D_RECORD_EQ_PARA = 0x100F,
    H2D_APT_EQ_PARA = 0x100C,

    //0x1100, Command for scenario 1 //SCO
    H2D_SCENARIO1_PARA_PARSING = 0x1100,
    H2D_S1_GENERAL_EQ_PARA_MIC = 0x1101,
    H2D_S1_GENERAL_EQ_PARA_SPK = 0x1102,
    H2D_S1_CODEC_PARA_PARSING = 0x1103,

    H2D_VOICE_EQ_PARA = 0x1106,

    //0x1200, Command for scenario 2
    H2D_SCENARIO2_PARA_PARSING = 0x1200,
    H2D_S2_GENERAL_EQ_PARA_MIC = 0x1201,
    H2D_S2_GENERAL_EQ_PARA_SPK = 0x1202,
    H2D_S2_CODEC_PARA_PARSING = 0x1203,

    //0x1300, Command for scenario 3
    H2D_SCENARIO3_PARA_PARSING = 0x1300,

    H2D_MALLEUS_LICENSE_PARA = 0x1400,
    H2D_LHDC_LICENSE_PARA = 0x1401,

    //0x1F00, Command for common command
    H2D_SYSTEM_PARA_PARSING = 0x1F00,
    H2D_SYSTEM_PARA_SILENCE_DETECT = 0x1F01,
} T_SHM_CMD_H2D;

typedef enum
{
    DSP_IPC_EVT_NONE                      = 0x00,
    DSP_IPC_EVT_SPK_SAMPLES               = 0x01,
    DSP_IPC_EVT_NOTIFICATION_FINISH       = 0x03,
    DSP_IPC_EVT_VP_REQUEST_DATA           = 0x05,
    DSP_IPC_EVT_FADE_IN_FINISH            = 0x06,
    DSP_IPC_EVT_FADE_OUT_FINISH           = 0x07,
    DSP_IPC_EVT_DECODE_EMPTY              = 0x08,
    DSP_IPC_EVT_DETECT_SILENCE            = 0x09,
    DSP_IPC_EVT_POWER_OFF_ACK             = 0x0A,
    DSP_IPC_EVT_VOL_UP                    = 0x0D,
    DSP_IPC_EVT_VOL_DOWN                  = 0x0E,
    DSP_IPC_EVT_VERSION                   = 0x0F,
    DSP_IPC_EVT_CLK_REQUEST               = 0x10,
    DSP_IPC_EVT_B2BMSG                    = 0x11,
    DSP_IPC_EVT_JOIN_CLK                  = 0x12,
    DSP_IPC_EVT_KEYWORD                   = 0x13,
    DSP_IPC_EVT_SET_DAC_GAIN              = 0x14,
    DSP_IPC_EVT_SET_ADC_GAIN              = 0x15,
    DSP_IPC_EVT_PLC_NUM                   = 0x16,
    DSP_IPC_EVT_OPEN_AIR_AVC              = 0x18,
    DSP_IPC_EVT_PROBE                     = 0x19,/*reservation of probe class */
    DSP_IPC_EVT_PROBE_EARFIT              = 0x1A,
    DSP_IPC_EVT_PROBE_SDK_GEN_CMD         = 0x1B,
    DSP_IPC_EVT_PROBE_SDK_BOOT            = 0x1C,
    DSP_IPC_EVT_HA_VER_INFO               = 0x1D,
    DSP_IPC_EVT_PROBE_SYNC_REF_REQUEST    = 0x1E, /* FIXME https://jira.realtek.com/browse/BB2RD-656 */
    DSP_IPC_EVT_APT_ACTION_ACK            = 0x1F,
    DSP_IPC_EVT_SPORT0_READY              = 0x20,
    DSP_IPC_EVT_SPORT0_STOP               = 0x21,
    DSP_IPC_EVT_SPORT1_READY              = 0x22,
    DSP_IPC_EVT_SPORT1_STOP               = 0x23,
    DSP_IPC_EVT_DECODER_ACTION_ACK        = 0x24,
    DSP_IPC_EVT_ENCODER_ACTION_ACK        = 0x25,
    DSP_IPC_EVT_DSP_SYNC_V2_SUCC          = 0x26,
    DSP_IPC_EVT_DSP_UNSYNC                = 0x27,
    DSP_IPC_EVT_DSP_SYNC_UNLOCK           = 0x28,
    DSP_IPC_EVT_DSP_SYNC_LOCK             = 0x29,
    DSP_IPC_EVT_SYNC_EMPTY                = 0x2A,
    DSP_IPC_EVT_SYNC_LOSE_TIMESTAMP       = 0x2B,
    DSP_IPC_EVT_BTCLK_EXPIRED             = 0x2C,
    DSP_IPC_EVT_LATENCY_REPORT            = 0x30,
    DSP_IPC_EVT_SEG_SEND_REQ_DATA         = 0x31,
    DSP_IPC_EVT_SEG_SEND_ERROR            = 0x32,
    DSP_IPC_EVT_AUDIOPLAY_VOLUME_INFO     = 0x33,
    DSP_IPC_EVT_DSP2_CLK_REQUEST          = 0x35,
    DSP_IPC_EVT_DECODER_PLC_NOTIFY        = 0x36,
    DSP_IPC_EVT_GATE_STATUS               = 0x40,

    DSP_IPC_EVT_CODEC_PIPE_CREATE         = 0x50,
    DSP_IPC_EVT_CODEC_PIPE_DESTROY        = 0x51,
    DSP_IPC_EVT_CODEC_PIPE_START          = 0x52,
    DSP_IPC_EVT_CODEC_PIPE_STOP           = 0x53,
    DSP_IPC_EVT_CODEC_PIPE_DATA_ACK       = 0x54,
    DSP_IPC_EVT_CODEC_PIPE_DATA_IND       = 0x55,
    DSP_IPC_EVT_CODEC_PIPE_PRE_MIXER_ADD  = 0x56,
    DSP_IPC_EVT_CODEC_PIPE_POST_MIXER_ADD = 0x57,
    DSP_IPC_EVT_CODEC_PIPE_MIXER_REMOVE   = 0x58,
} T_DSP_IPC_EVENT;

typedef struct t_dsp_ipc_decoder2
{
    uint8_t     version;
    uint8_t     decode_algorithm;      /*!< Base on T_ALGORITHM_TYPE  */
    uint16_t    reserved;
    uint32_t    session_id;
    uint32_t    sample_rate;
    uint16_t    frame_size_per_ch;
    uint8_t     channel_num_per_block;  /*!< decode channel number per block*/
    uint8_t     bit_width;              /*!< 0x00:16bit 0x01:24bit */
} T_DSP_IPC_DECODER2;

typedef struct t_dsp_ipc_lc3_encoder2
{
    uint8_t     version;
    uint8_t     encode_algorithm;      /*!< Base on T_ALGORITHM_TYPE  */
    uint16_t    reserved;
    uint32_t    session_id;
    uint32_t    sample_rate;
    uint16_t    frame_size_per_ch;
    uint8_t     channel_num_per_block;
    uint8_t     bit_width;
} T_DSP_IPC_ENCODER2;

typedef struct t_dsp_ipc_lc3_decoder
{
    uint8_t     version;                     /*!< default 0 */
    uint8_t     frame_duration;
    uint8_t     blocks_per_sdu;
    uint8_t     reserved0;
    uint32_t    channel_allocation;
    uint16_t    octets_per_frame;            /*!< Number of octets(byte) used per frame */
    uint16_t    max_transport_latency_ms;    /*!< Maximum trasport latency with ms Unit */
    uint32_t    sdu_interval;                /*!< Unit us, typical values are between 7500 to 60000 */
    uint32_t    presentation_delay_us;
    uint8_t     retransmission_num;
    uint8_t     framed_unframed;             /*!< 0x00: unframed, 0x01: framed   */
    uint8_t     bis_cis;                     /*!< 0x00 CIS, 0x01 BIS */
    uint8_t     plc_method;
    uint32_t    reserved1;
} T_DSP_IPC_LC3_DECODER;

typedef struct t_dsp_ipc_lc3_encoder
{
    uint8_t     version;                     /*!< default 0 */
    uint8_t     frame_duration;
    uint8_t     blocks_per_sdu;
    uint8_t     reserved0;
    uint32_t    channel_allocation;
    uint16_t    octets_per_frame;            /*!< Number of octets(byte) used per frame */
    uint16_t    max_transport_latency_ms;    /*!< Maximum trasport latency with ms Unit */
    uint32_t    sdu_interval;                /*!< Unit us, typical values are between 7500 to 60000 */
    uint32_t    presentation_delay_us;
    uint8_t     retransmission_num;
    uint8_t     framed_unframed;             /*!< 0x00: unframed, 0x01: framed   */
    uint8_t     bis_cis;                     /*!< 0x00 CIS, 0x01 BIS */
    uint8_t     encode_packet_format;        /*!< Packing LC3 package format 0x00: default */
    uint32_t    reserved1;
} T_DSP_IPC_LC3_ENCODER;

typedef struct t_dsp_ipc_decoder
{
    uint8_t algorithm;
    uint8_t sub_type;
    uint8_t chann_mode;
    uint8_t bit_res;
    uint32_t sample_rate;
    uint16_t samples_per_frame;
    T_DSP_IPC_DECODER2 comm_header;           /*codec set common header*/
    T_DSP_IPC_LC3_DECODER decoder;            /*LC3 using, replace all in ipc2.0 */
} T_DSP_IPC_DECODER;

typedef struct t_dsp_ipc_encoder
{
    uint8_t     algorithm;
    uint8_t     sub_type;
    uint8_t     chann_mode;
    uint8_t     bit_res;
    uint32_t    sample_rate;
    uint8_t     opus_cbr;
    uint8_t     opus_cvbr;
    uint8_t     opus_complexity;
    uint8_t     opus_mode;
    uint8_t     opus_application;
    uint16_t    samples_per_frame;
    uint32_t    opus_bitrate;
    T_DSP_IPC_ENCODER2 comm_header;           /*codec set common header*/
    T_DSP_IPC_LC3_ENCODER encoder;            /*LC3 using, replace all in ipc2.0 */
} T_DSP_IPC_ENCODER;


#define IPC_FADE_IN         (0x1)
#define IPC_FADE_OUT        (0x0)

#define DSP_IPC_STOP_VP     (0xFFFFF)

#define DSP_IPC_VP_CFG_SIZE     (4)
#define DSP_IPC_TTS_CFG_SIZE    (4)

/**
 * \brief define for \ref H2D_SBC_ENCODE_HDR_CONFIG
 *
 */
typedef struct t_dsp_sbc_encode_param
{
    uint32_t sample_rate;
    uint8_t chann_mode;
    uint8_t block_length;
    uint8_t subband_num;
    uint8_t allocation_method;
    uint8_t bitpool;
    uint8_t frame_num;
} T_DSP_SBC_ENCODE_PARAM;

/**
 * \brief define for \ref H2D_DSP_VOICE_TRIGGER_SET_SIZE
 *
 */
typedef struct t_dsp_voice_triger_set
{
    uint8_t vad_setting;
    uint8_t kws_setting;
} T_DSP_VOICE_TRIGER_SET;

/**
 * \brief This is not an API
 *
 * \param p_data Pointer to the VP data
 * \param len  The length of the VP data
 * \return true Send Success
 * \return false Send Fail
 */
bool dsp_ipc_voice_prompt_send(uint8_t *p_data, uint32_t len);

typedef bool (*P_DSP_IPC_CBACK)(T_DSP_IPC_EVENT event, uint32_t param);

bool dsp_ipc_init(void);
void dsp_ipc_deinit(void);
bool dsp_ipc_cback_register(P_DSP_IPC_CBACK cback);
bool dsp_ipc_cback_unregister(P_DSP_IPC_CBACK cback);
bool dsp_ipc_set_decoder(T_DSP_IPC_DECODER param, bool flush);
bool dsp_ipc_set_encoder(T_DSP_IPC_ENCODER param, bool flush);
bool dsp_ipc_set_lc3_decoder(T_DSP_IPC_DECODER2 param, T_DSP_IPC_LC3_DECODER lc3_param);
bool dsp_ipc_set_lc3_encoder(T_DSP_IPC_ENCODER2 param, T_DSP_IPC_LC3_ENCODER lc3_param);
bool dsp_ipc_set_low_latency_mode(uint8_t param);
bool dsp_ipc_set_fifo_len(T_DSP_FIFO_TRX_DIRECTION direction, uint8_t lower_len);
bool dsp_ipc_set_decode_emtpy(bool enable);
bool dsp_ipc_set_data_mode(T_DSP_DATA_MODE data_mode, uint8_t stream_type);
bool dsp_ipc_set_latency_report(uint8_t param);
bool dsp_ipc_set_plc_notify(uint16_t interval, uint32_t threshold, bool enable);
bool dsp_ipc_set_boot_config(uint8_t param);
bool dsp_ipc_set_tone_gain(int16_t left_gain, int16_t right_gain);
bool dsp_ipc_set_dac_gain(int16_t left_gain, int16_t right_gain);
bool dsp_ipc_set_apt_dac_gain(int16_t left_gain, int16_t right_gain);
bool dsp_ipc_set_voice_adc_gain(int16_t left_gain, int16_t right_gain);
bool dsp_ipc_set_record_adc_gain(int16_t left_gain, int16_t right_gain);
bool dsp_ipc_set_aux_adc_gain(int16_t left_gain, int16_t right_gain);
bool dsp_ipc_synchronization_data_send(uint8_t *buf, uint16_t len);
bool dsp_ipc_set_b2bmsg_interaction_timeout(uint8_t param);
bool dsp_ipc_set_power_down(uint8_t param);
bool dsp_ipc_set_dsp_config(uint32_t param, bool flush);
bool dsp_ipc_set_dsp2_config(uint32_t param, bool flush);
bool dsp_ipc_audio_eq_set(void *p_data, uint16_t len);
bool dsp_ipc_voice_eq_set(void *p_data, uint16_t len);
bool dsp_ipc_record_eq_set(void *p_data, uint16_t len);
bool dsp_ipc_apt_eq_set(void *p_data, uint16_t len);
bool dsp_ipc_audio_eq_clear(void);
bool dsp_ipc_voice_eq_clear(void);
bool dsp_ipc_record_eq_clear(void);
bool dsp_ipc_apt_eq_clear(void);
bool dsp_ipc_set_stream_channel_out_config(uint8_t param, bool flush);
bool dsp_ipc_set_stream_channel_in_config(uint8_t param);
bool dsp_ipc_set_audio_sport0_action(uint8_t param);
bool dsp_ipc_set_audio_sport1_action(uint8_t param);
bool dsp_ipc_set_fade_in_out_control(uint8_t param);
bool dsp_ipc_set_action_control(void);
bool dsp_ipc_set_sbc_encoder_hdr_config(T_DSP_SBC_ENCODE_PARAM param);
bool dsp_ipc_set_rws_init(uint8_t bt_clk_index, uint32_t sample_rate, uint8_t type);
bool dsp_ipc_set_rws_seamless(uint8_t param);
bool dsp_ipc_set_rws(uint8_t param, uint8_t type);
bool dsp_ipc_set_tx_path_ramp_gain_control(uint16_t gain_step, uint8_t time_index);
bool dsp_ipc_set_log_output_sel(uint8_t param);
bool dsp_ipc_set_rws_asrc_ratio(uint32_t *param);
bool dsp_ipc_voice_prompt_end(void);
bool dsp_ipc_set_signal_proc_start(uint32_t param, uint8_t sport_id);
bool dsp_ipc_set_handover_info(uint8_t role, bool start);
bool dsp_ipc_set_voice_trigger(uint32_t param);
bool dsp_ipc_set_force_dummy_pkt(uint32_t param);

bool dsp_ipc_sport_set(T_DSP_IPC_SPORT_CFG *param);

bool dsp_ipc_composite_data_send(void *p_data, uint32_t len);
bool dsp_ipc_voice_prompt_start(uint32_t cfg, uint32_t cfg_bt_clk_mix);
bool dsp_ipc_send_h2d_param(uint16_t cmd_id, uint8_t *payload_data, uint16_t payload_len);

bool dsp_ipc_apt_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io);
bool dsp_ipc_decoder_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io);
bool dsp_ipc_encoder_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io);
bool dsp_ipc_vad_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io);
bool dsp_ipc_line_in_action(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io);

bool dsp_ipc_init_dsp_sdk(bool flush, uint8_t scenario);
bool dsp_ipc_sidetone_set(uint8_t enable, int16_t gain, uint8_t level);

bool dipc_decoder_effect_control(uint8_t category, uint8_t action);

bool dipc_encoder_effect_control(uint8_t category, uint8_t action);

bool dsp_ipc_wdrc_effect_set(uint8_t *payload_data,
                             uint16_t payload_len);
bool dsp_ipc_audio_wdrc_set(uint8_t *payload_data,
                            uint16_t payload_len);
bool dsp_ipc_voice_wdrc_set(uint8_t *payload_data,
                            uint16_t payload_len);
bool dsp_ipc_apt_wdrc_set(uint8_t *payload_data,
                          uint16_t payload_len);

bool dsp_ipc_voice_nrec_set(uint8_t enable);

bool dsp_ipc_apt_nrec_set(uint8_t enable, uint8_t mode, uint8_t level);

bool dsp_ipc_apt_ovp_set(uint8_t enable, uint8_t level);

bool dsp_ipc_apt_beamforming_set(uint8_t enable, uint8_t direction);

bool dsp_ipc_signal_out_monitoring_set(uint8_t enable, uint16_t refresh_interval);

bool dsp_ipc_signal_in_monitoring_set(uint8_t enable, uint16_t refresh_interval);

void dsp_ipc_download_algo_param(uint8_t *cmd_buffer, uint16_t algo_cmd_length);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif   /* _DSP_IPC_H_ */
