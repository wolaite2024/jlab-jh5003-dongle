/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_TRACK_H_
#define _AUDIO_TRACK_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_TRACK Audio Track
 *
 * \brief   Create, control and destroy Audio Track sessions.
 * \details Each Audio Track represents an audio stream path from the low level hardware, which shall
 *          be unidirectional or bidirectional for the input (record) or output (playback) streams,
 *          and be attached to the audio devices. The application can pass data in and drain data out
 *          by writing to and reading from the Audio Track, or adjust the gain level by controlling
 *          the Audio Track volumes.
 */

/**
 * \brief Define the Audio Track states.
 *
 * \ingroup AUDIO_TRACK
 */
typedef enum t_audio_track_state
{
    AUDIO_TRACK_STATE_RELEASED      = 0x00, /**< Audio Track that was released or not created yet. */
    AUDIO_TRACK_STATE_CREATED       = 0x01, /**< Audio Track that was created. */
    AUDIO_TRACK_STATE_STARTED       = 0x02, /**< Audio Track that was started. */
    AUDIO_TRACK_STATE_STOPPED       = 0x03, /**< Audio Track that was stopped. */
    AUDIO_TRACK_STATE_PAUSED        = 0x04, /**< Audio Track that was paused. */
    AUDIO_TRACK_STATE_RESTARTED     = 0x05, /**< Audio Track that was restarted. */
} T_AUDIO_TRACK_STATE;

/**
 * \brief Define the Audio Track causes.
 *
 * \ingroup AUDIO_TRACK
 */
typedef enum t_audio_track_cause
{
    AUDIO_TRACK_CAUSE_NONE                  = 0x00, /**< Audio Track that was handled successfully. */
    AUDIO_TRACK_CAUSE_HW_ERROR              = 0x01, /**< Audio Track that encountered low layer hardware error. */
    AUDIO_TRACK_CAUSE_BUFFER_EMPTY          = 0x02, /**< Audio Track that encountered buffer empty. */
    AUDIO_TRACK_CAUSE_REMOTE_JOIN_FAILED    = 0x03, /**< Audio Track that encountered remote join failure. */
    AUDIO_TRACK_CAUSE_JOIN_PACKET_LOST      = 0x04, /**< Audio Track that encountered too many packets lost. */
    AUDIO_TRACK_CAUSE_UNKNOWN_ERROR         = 0xFF, /**< Audio Track that encountered unknown error. */
} T_AUDIO_TRACK_CAUSE;

/**
 * \brief Define the Audio Track policy.
 *
 * \details The Audio Track can set its stream policy.
 *          \ref AUDIO_TRACK_POLICY_SINGLE_STREAM is the default policy where only one running Audio
 *          Track instance is allowed for either the decoding path or the encoding path. The Audio
 *          Track instance with the stream type \ref AUDIO_STREAM_TYPE_VOICE will preempt the Audio
 *          Track instance with the stream type \ref AUDIO_STREAM_TYPE_PLAYBACK. The Audio Track
 *          instance with the stream type \ref AUDIO_STREAM_TYPE_VOICE cannot coexist with the Audio
 *          Track instance with the stream type \ref AUDIO_STREAM_TYPE_RECORD.
 *          \ref AUDIO_TRACK_POLICY_MULTI_STREAM is the policy that multi Audio Track instances for
 *          either decoding path or encoding path can coexist. Applications can control each Audio
 *          Track instance's lifecycle individually.
 *
 * \ingroup AUDIO_TRACK
 */
typedef enum t_audio_track_policy
{
    AUDIO_TRACK_POLICY_SINGLE_STREAM = 0x00, /**< Audio Track single stream policy */
    AUDIO_TRACK_POLICY_MULTI_STREAM  = 0x01, /**< Audio Track multi stream policy */
} T_AUDIO_TRACK_POLICY;

/**
 * \brief Define the Audio Track session handle.
 *
 * \ingroup AUDIO_TRACK
 */
typedef void *T_AUDIO_TRACK_HANDLE;

/**
 * \brief Define the Audio Track asynchronous read/write IO operations.
 *
 * \details Each Audio Track session can be unidirectional or bidirectional. When the stream type is
 *          set as \ref AUDIO_STREAM_TYPE_PLAYBACK in \ref audio_track_create, \ref P_AUDIO_TRACK_ASYNC_IO
 *          can be implemented as an asynchronous write operation. When the stream type is set as
 *          \ref AUDIO_STREAM_TYPE_RECORD, \ref P_AUDIO_TRACK_ASYNC_IO can be implemented as an
 *          asynchronous read operation. When the stream type is set as \ref AUDIO_STREAM_TYPE_VOICE,
 *          \ref P_AUDIO_TRACK_ASYNC_IO can be implemented as either an asynchronous write or an
 *          asynchronous read operation.
 *
 * \note    If \ref P_AUDIO_TRACK_ASYNC_IO is set to NULL in \ref audio_track_create, the Audio Track
 *          shall use the synchronous read interface \ref audio_track_read to drain data, or use the
 *          synchronous write interface \ref audio_track_write to pass data in.
 *
 * \param[in]     handle        The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in,out] timestamp     The timestamp of the buffer passed into or passed out from Audio Track.
 * \param[in,out] seq_num       The sequence number of the buffer passed into or passed out from Audio Track.
 * \param[in,out] status        The frame status of the buffer passed into or passed out from Audio Track.
 * \param[in,out] frame_num     The frame number of the buffer passed into or passed out from Audio Track.
 * \param[in,out] buf           The buffer that holds the audio data written to or read from the Audio Track.
 * \param[in]     required_len  The required size in bytes written to or read from the Audio Track.
 * \param[out]    actual_len    The actual size in bytes written to or read from the Audio Track.
 *
 * \ingroup AUDIO_TRACK
 */
typedef bool (*P_AUDIO_TRACK_ASYNC_IO)(T_AUDIO_TRACK_HANDLE   handle,
                                       uint32_t              *timestamp,
                                       uint16_t              *seq_num,
                                       T_AUDIO_STREAM_STATUS *status,
                                       uint8_t               *frame_num,
                                       void                  *buf,
                                       uint16_t               required_len,
                                       uint16_t              *actual_len);

/**
 * \brief   Set the Audio Track stream policy.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] policy    The Audio Track policy \ref T_AUDIO_TRACK_POLICY.
 *
 * \return  The status of setting Audio Track policy.
 * \retval  true    Audio Track policy was set successfully.
 * \retval  false   Audio Track policy was failed to set.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_policy_set(T_AUDIO_TRACK_POLICY policy);

/**
 * \brief   Create an Audio Track session instance.
 *
 * \details The Audio Subsystem may have multiple coexisting or exclusive low-level audio data paths.
 *          If the application wants to control a specific audio data path, it should first create an
 *          Audio Track that is abstracted from the low-level hardware device routing. Operations on
 *          the Audio Track shall focus on the corresponding Audio Track handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \note    This is an asynchronous function, and the Audio Track session state will be transformed
 *          into the <b>created</b> state when the current state \ref AUDIO_TRACK_STATE_CREATED in
 *          the audio event \ref AUDIO_EVENT_TRACK_STATE_CHANGED is received.
 *
 * \param[in] stream_type   The stream type \ref T_AUDIO_STREAM_TYPE of the Audio Track.
 * \param[in] mode          The stream mode \ref T_AUDIO_STREAM_MODE of the Audio Track.
 * \param[in] usage         The stream usage \ref T_AUDIO_STREAM_USAGE of the Audio Track.
 * \param[in] format_info   The encoding/decoding format information \ref T_AUDIO_FORMAT_INFO of the
 *                          Audio Track.
 * \param[in] volume_out    The volume out level of the Audio Track stream, and the range is between
 *                          \ref audio_volume_out_min_get and \ref audio_volume_out_max_get.
 * \param[in] volume_in     The volume in level of the Audio Track stream, and the range is between
 *                          \ref audio_volume_in_min_get and \ref audio_volume_in_max_get.
 * \param[in] device        The device bitmask \ref AUDIO_DEVICE_BITMASK of the Audio Track.
 * \param[in] async_write   The asynchronous write operation \ref P_AUDIO_TRACK_ASYNC_IO of the Audio
 *                          Track.
 * \param[in] async_read    The asynchronous read operation \ref P_AUDIO_TRACK_ASYNC_IO of the Audio
 *                          Track.
 *
 * \return  The instance handle of the Audio Track session. If the returned handle is NULL, the Audio
 *          Track session instance failed to create.
 *
 * \ingroup AUDIO_TRACK
 */
T_AUDIO_TRACK_HANDLE audio_track_create(T_AUDIO_STREAM_TYPE    stream_type,
                                        T_AUDIO_STREAM_MODE    mode,
                                        T_AUDIO_STREAM_USAGE   usage,
                                        T_AUDIO_FORMAT_INFO    format_info,
                                        uint8_t                volume_out,
                                        uint8_t                volume_in,
                                        uint32_t               device,
                                        P_AUDIO_TRACK_ASYNC_IO async_write,
                                        P_AUDIO_TRACK_ASYNC_IO async_read);

/**
 * \brief   Start playing the Audio Track session.
 *
 * \details The Audio Track must be started before writing to or reading from the Audio Track session;
 *          otherwise, data may be dropped when writing or be empty when reading. When the Audio Track
 *          session is stopped by \ref audio_track_stop, this API can resume the Audio Track session.
 *
 * \note    This is an asynchronous function, and the Audio Track session state will be transformed
 *          into the <b>started</b> state when the current state \ref AUDIO_TRACK_STATE_STARTED in
 *          the audio event \ref AUDIO_EVENT_TRACK_STATE_CHANGED is received.
 *          Starting an Audio Track session already in the <b>started</b> state will be ignored.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of starting the Audio Track.
 * \retval true     Audio Track was started successfully.
 * \retval false    Audio Track was failed to start.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_start(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Stop the Audio Track session.
 *
 * \details The Audio Track will be paused, and the data queued in the track buffer will be discarded.
 *          Any subsequent \ref audio_track_write or \ref audio_track_read operations on the stopped
 *          Audio Track session will be ignored.
 *
 * \note    This is an asynchronous function, and the Audio Track session state will be transformed
 *          into the <b>stopped</b> state when the current state \ref AUDIO_TRACK_STATE_STOPPED in
 *          the audio event \ref AUDIO_EVENT_TRACK_STATE_CHANGED is received.
 *          Stopping an Audio Track session already in <b>stopped</b> state will be ignored.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of stopping the Audio Track.
 * \retval true     Audio Track was stopped successfully.
 * \retval false    Audio Track was failed to stop.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_stop(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Pause the Audio Track session.
 *
 * \details The Audio Track will be paused, but the data queued in the track buffer will not be
 *          discarded. Any subsequent \ref audio_track_write or \ref audio_track_read operations on
 *          the paused Audio Track session will be ignored.
 *
 * \note    This is an asynchronous function, and the Audio Track session state will be transformed
 *          into the <b>paused</b> state when the \ref AUDIO_TRACK_STATE_PAUSED event in the audio
 *          event \ref AUDIO_EVENT_TRACK_STATE_CHANGED is received.
 *          Pausing an Audio Track session already in the <b>paused</b> state will be ignored.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of pausing the Audio Track.
 * \retval true     Audio Track was paused successfully.
 * \retval false    Audio Track was failed to pause.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_pause(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Restart the Audio Track session.
 *
 * \details The Audio Track will be restarted, and the data queued in the track buffer will be
 *          discarded. Any subsequent \ref audio_track_write or \ref audio_track_read operations on
 *          the restarting Audio Track session will be ignored.
 *
 * \note    This is an asynchronous function, and the Audio Track session state will be transformed
 *          into the <b>started</b> state when the \ref AUDIO_TRACK_STATE_RESTARTED event in the
 *          audio event \ref AUDIO_EVENT_TRACK_STATE_CHANGED is received.
 *          Restarting an Audio Track session already in the <b>restarting</b> state will be ignored.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of restarting the Audio Track.
 * \retval true     Audio Track was restarted successfully.
 * \retval false    Audio Track was failed to restart.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_restart(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Flush the data of the Audio Track session.
 *
 * \details The data currently queued in the track buffer will be flushed, and any subsequent \ref
 *          audio_track_write or \ref audio_track_read operations on the recently flushed Audio Track
 *          session will be valid.
 *
 * \note    Flushing an Audio Track in the <b>started</b> state is forbidden and will return a
 *          failure. To purge the Audio Track session safely, the Audio Track should be flushed
 *          immediately after the Audio Track session is created.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of flushing the Audio Track.
 * \retval true     Audio Track was flushed successfully.
 * \retval false    Audio Track was failed to flush.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_flush(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Release the Audio Track session.
 *
 * \details All resources and low-level hardware configurations associated with the Audio Track
 *          session will be released.
 *
 * \note    This is an asynchronous function, and the Audio Track session state will be transformed
 *          into the <b>released</b> state when the \ref AUDIO_TRACK_STATE_RELEASE event in the
 *          audio event \ref AUDIO_EVENT_TRACK_STATE_CHANGED is received.
 *          It is recommended that the Audio Track session be stopped before being released.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of releasing the Audio Track.
 * \retval true     Audio Track was released successfully.
 * \retval false    Audio Track was failed to release.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_release(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Write the data to the Audio Track session.
 *
 * \details When the Audio Track session is started by \ref audio_track_start, the application can
 *          write data to the Audio Track session. Refer to \ref P_AUDIO_TRACK_ASYNC_IO for
 *          asynchronous IO operation details.
 *
 * \note    If the returned status is false, the application can stop and release the Audio Track
 *          session. If the returned status is true, but the actual written buffer length is zero or
 *          any positive number less than the required buffer length, the application can retry or
 *          discard the remaining buffer data.
 *
 * \param[in]  handle       The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in]  timestamp    The timestamp of the buffer passed into Audio Subsystem.
 * \param[in]  seq_num      The sequence number of the buffer passed into Audio Subsystem.
 * \param[in]  status       The frame status of the buffer passed into Audio Subsystem.
 * \param[in]  frame_num    The frame number of the buffer passed into Audio Subsystem.
 * \param[in]  buf          The buffer that holds the audio data.
 * \param[in]  len          The required size in bytes written from the buffer.
 * \param[out] written_len  The actual size in bytes written from the buffer.
 *
 * \return          The status of writing the audio data.
 * \retval true     Audio data was written successfully.
 * \retval false    Audio data was failed to write.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_write(T_AUDIO_TRACK_HANDLE   handle,
                       uint32_t               timestamp,
                       uint16_t               seq_num,
                       T_AUDIO_STREAM_STATUS  status,
                       uint8_t                frame_num,
                       void                  *buf,
                       uint16_t               len,
                       uint16_t              *written_len);

/**
 * \brief   Read the data from the Audio Track session.
 *
 * \details When the Audio Track session is started by \ref audio_track_start, the application can
 *          read data from the Audio Track session. Refer to \ref P_AUDIO_TRACK_ASYNC_IO for
 *          asynchronous IO operation details.
 *
 * \note    If the returned status is false, the application can stop and release the Audio Track
 *          session. If the returned status is true, but the actual read buffer length is zero or
 *          any positive number less than the required buffer length, the application can retry or
 *          terminate the read operation.
 *
 * \param[in]  handle       The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] timestamp    The timestamp of the buffer passed out from Audio Subsystem.
 * \param[out] seq_num      The sequence number of the buffer passed out from Audio Subsystem.
 * \param[out] status       The frame status of the buffer passed out from Audio Subsystem.
 * \param[out] frame_num    The frame number of the buffer passed out from Audio Subsystem.
 * \param[out] buf          The buffer that holds the audio data.
 * \param[in]  len          The required size in bytes read from the Audio Track.
 * \param[out] read_len     The actual size in bytes read from the Audio Track.
 *
 * \return          The status of reading the audio data.
 * \retval true     Audio data was read successfully.
 * \retval false    Audio data was failed to read.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_read(T_AUDIO_TRACK_HANDLE   handle,
                      uint32_t              *timestamp,
                      uint16_t              *seq_num,
                      T_AUDIO_STREAM_STATUS *status,
                      uint8_t               *frame_num,
                      void                  *buf,
                      uint16_t               len,
                      uint16_t              *read_len);

/**
 * \brief   Set the threshold of the specific Audio Track.
 *
 * \details The application will receive the audio events \ref AUDIO_EVENT_TRACK_BUFFER_LOW and
 *          \ref AUDIO_EVENT_TRACK_BUFFER_HIGH by setting the thresholds appropriately. The upper
 *          threshold and the lower threshold both range from 0 to the maximum Audio Track latency.
 *          The upper threshold must be larger than the lower threshold, and the initial buffer
 *          level in milliseconds must be greater than the lower threshold and less than the upper
 *          threshold.
 *
 * \note    The application can set both the upper threshold and the lower threshold to 0 if it does
 *          not need the audio events.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in]  upper_threshold  The upper threshold of the Audio Track buffer in milliseconds.
 * \param[in]  lower_threshold  The lower threshold of the Audio Track buffer in milliseconds.
 *
 * \return          The status of setting the Audio Track buffer threshold.
 * \retval true     Audio Track buffer threshold was set successfully.
 * \retval false    Audio Track buffer threshold was failed to set.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_threshold_set(T_AUDIO_TRACK_HANDLE handle,
                               uint16_t             upper_threshold,
                               uint16_t             lower_threshold);

/**
 * \brief   Get the state of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] state    The current state \ref T_AUDIO_TRACK_STATE of the Audio Track session.
 *
 * \return          The status of getting the Audio Track state.
 * \retval true     Audio Track state was got successfully.
 * \retval false    Audio Track state was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_state_get(T_AUDIO_TRACK_HANDLE  handle,
                           T_AUDIO_TRACK_STATE  *state);

/**
 * \brief   Get the stream type of the specific Audio Track session.
 *
 * \param[in]  handle       The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] stream_type  The stream type \ref T_AUDIO_STREAM_TYPE of the Audio Track session.
 *
 * \return          The status of getting the Audio Track stream type.
 * \retval true     Audio Track stream type was got successfully.
 * \retval false    Audio Track stream type was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_stream_type_get(T_AUDIO_TRACK_HANDLE  handle,
                                 T_AUDIO_STREAM_TYPE  *stream_type);

/**
 * \brief   Get the encoding/decoding format information of the specific Audio Track session.
 *
 * \param[in]  handle       The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] format_info  The encoding/decoding format information \ref T_AUDIO_FORMAT_TYPE of the
 *                          Audio Track session.
 *
 * \return          The status of getting the Audio Track encoding/decoding format information.
 * \retval true     Audio Track encoding/decoding format information was got successfully.
 * \retval false    Audio Track encoding/decoding format information was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_format_info_get(T_AUDIO_TRACK_HANDLE  handle,
                                 T_AUDIO_FORMAT_INFO  *format_info);

/**
 * \brief   Get the device bitmask of the specific Audio Track session.
 *
 * \details The device shall be treated as the output sink endpoint of the playback stream type
 *          \ref AUDIO_STREAM_TYPE_PLAYBACK, the input source endpoint of the record stream type
 *          \ref AUDIO_STREAM_TYPE_RECORD, or the output and input endpoints of the voice stream
 *          type \ref AUDIO_STREAM_TYPE_VOICE.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] device   The device bitmask \ref AUDIO_DEVICE_BITMASK of the Audio Track session.
 *
 * \return          The status of getting the Audio Track device bitmask.
 * \retval true     Audio Track device bitmask was got successfully.
 * \retval false    Audio Track device bitmask was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_device_get(T_AUDIO_TRACK_HANDLE  handle,
                            uint32_t             *device);

/**
 * \brief   Get the stream mode of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] mode     The stream mode \ref T_AUDIO_STREAM_MODE of the Audio Track session.
 *
 * \return          The status of getting the Audio Track stream mode.
 * \retval true     Audio Track stream mode was got successfully.
 * \retval false    Audio Track stream mode was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_mode_get(T_AUDIO_TRACK_HANDLE  handle,
                          T_AUDIO_STREAM_MODE  *mode);

/**
 * \brief   Get the usage of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] usage    The usage \ref T_AUDIO_STREAM_USAGE of the Audio Track session.
 *
 * \return          The status of getting the Audio Track usage.
 * \retval true     Audio Track usage was got successfully.
 * \retval false    Audio Track usage was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_usage_get(T_AUDIO_TRACK_HANDLE  handle,
                           T_AUDIO_STREAM_USAGE *usage);

/**
 * \brief   Get the maximum stream latency of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] latency  The stream latency in milliseconds of the Audio Track session.
 *
 * \return          The status of getting the Audio Track maximum stream latency.
 * \retval true     Audio Track maximum stream latency was got successfully.
 * \retval false    Audio Track maximum stream latency was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_latency_max_get(T_AUDIO_TRACK_HANDLE  handle,
                                 uint16_t             *latency);

/**
 * \brief   Get the minimum stream latency of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] latency  The stream latency in milliseconds of the Audio Track session.
 *
 * \return          The status of getting the Audio Track minimum stream latency.
 * \retval true     Audio Track minimum stream latency was got successfully.
 * \retval false    Audio Track minimum stream latency was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_latency_min_get(T_AUDIO_TRACK_HANDLE  handle,
                                 uint16_t             *latency);

/**
 * \brief   Get the current stream latency of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] latency  The stream latency in milliseconds of the Audio Track session.
 *
 * \return          The status of getting the Audio Track current stream latency.
 * \retval true     Audio Track current stream latency was got successfully.
 * \retval false    Audio Track current stream latency was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_latency_get(T_AUDIO_TRACK_HANDLE  handle,
                             uint16_t             *latency);

/**
 * \brief   Set the current stream latency of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] latency   The stream latency in milliseconds of the Audio Track session.
 * \param[in] fixed     The latency is fixed and cannot be adjusted dynamically.
 *
 * \return          The status of setting the Audio Track current stream latency.
 * \retval true     Audio Track current stream latency was set successfully.
 * \retval false    Audio Track current stream latency was failed to set.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_latency_set(T_AUDIO_TRACK_HANDLE handle,
                             uint16_t             latency,
                             bool                 fixed);

/**
 * \brief   Enable DSP PLC and set the plc-notify-interval of the specific Audio Track session.
 *
 * \param[in] handle     The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] interval   The plc-notify-interval in milliseconds of the Audio Track session.
 * \param[in] threshold  The preset threshold for the continuous PLC samples for the session.
 *                       If  the continuous PLC samples exceed the threshold, the Controller shall stop PLC process.
 *                       Note that if the parameter enable is 0x01, a threshold of 0x00000000 indicates that PLC threshold shall be determined by the Controller;
 *                       while if the parameter enable is 0x00, the threshold shall be fixed at 0x00000000 by the Host.
 * \param[in] enable     Enable or disable DSP PLC.
 *
 * \return          The status of setting the Audio Track current stream latency.
 * \retval true     Audio Track plc notify was set successfully.
 * \retval false    Audio Track plc notify was failed to set.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_plc_notify_set(T_AUDIO_TRACK_HANDLE handle,
                                uint16_t             interval,
                                uint32_t             threshold,
                                bool                 enable);

/**
 * \brief   Get the current buffer level of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] level     The buffer level in milliseconds of the Audio Track session.
 *
 * \return          The status of getting the Audio Track current stream buffer level.
 * \retval true     Audio Track current stream buffer level was get successfully.
 * \retval false    Audio Track current stream buffer level was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_buffer_level_get(T_AUDIO_TRACK_HANDLE  handle,
                                  uint16_t             *level);


/**
 * \brief   Get the maximum volume out level of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] volume   The volume out level of the Audio Track session.
 *
 * \return          The status of getting the Audio Track maximum volume out level.
 * \retval true     Audio Track maximum volume out level was got successfully.
 * \retval false    Audio Track maximum volume out level was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_out_max_get(T_AUDIO_TRACK_HANDLE  handle,
                                    uint8_t              *volume);

/**
 * \brief   Get the minimum volume out level of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] volume   The volume out level of the Audio Track session.
 *
 * \return          The status of getting the Audio Track minimum volume out level.
 * \retval true     Audio Track minimum volume out level was got successfully.
 * \retval false    Audio Track minimum volume out level was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_out_min_get(T_AUDIO_TRACK_HANDLE  handle,
                                    uint8_t              *volume);

/**
 * \brief   Get the current volume out level of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] volume   The volume out level of the Audio Track session.
 *
 * \return          The status of getting the current volume out level of the Audio Track.
 * \retval true     Audio Track current volume out level was got successfully.
 * \retval false    Audio Track current volume out level was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_out_get(T_AUDIO_TRACK_HANDLE  handle,
                                uint8_t              *volume);

/**
 * \brief   Set the current volume out level of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] volume    The volume out level of the Audio Track session, whose range is between
 *                      \ref audio_volume_out_min_get and \ref audio_volume_out_max_get.
 *
 * \return          The status of setting the current volume out level of the Audio Track.
 * \retval true     Audio Track current volume out level was set successfully.
 * \retval false    Audio Track current volume out level was failed to set.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_out_set(T_AUDIO_TRACK_HANDLE handle,
                                uint8_t              volume);

/**
 * \brief   Mute the current volume out level of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of muting the current volume out level of the Audio Track.
 * \retval  true    Audio Track current volume out level was muted successfully.
 * \retval  false   Audio Track current volume out level was failed to mute.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_out_mute(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Unmute the current volume out level of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of unmuting the current volume out level of the Audio Track.
 * \retval  true    Audio Track current volume out level was unmuted successfully.
 * \retval  false   Audio Track current volume out level was failed to unmute.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_out_unmute(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Get the current volume out mute status of the specific Audio Track session.
 *
 * \param[in]  handle     The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] is_muted   The volume out mute status of the Audio Track session.
 *
 * \return          The result of getting the Audio Track current volume out mute status.
 * \retval true     Audio Track current volume out mute status was got successfully.
 * \retval false    Audio Track current volume out mute status was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_out_is_muted(T_AUDIO_TRACK_HANDLE  handle,
                                     bool                 *is_muted);

/**
 * \brief   Start monitoring the signal out strength of the specific Audio Track session.
 *
 * \param[in] handle            The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] refresh_interval  The refresh interval in milliseconds.
 *
 * \return          The result of starting the Audio Track signal out strength monitoring.
 * \retval  true    Audio Track signal out strength monitoring was started successfully.
 * \retval  false   Audio Track signal out strength monitoring was failed to start.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_signal_out_monitoring_start(T_AUDIO_TRACK_HANDLE handle,
                                             uint16_t             refresh_interval);

/**
 * \brief   Stop monitoring the signal out strength of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The result of stopping the Audio Track signal out strength monitoring.
 * \retval  true    Audio Track signal out strength monitoring was stopped successfully.
 * \retval  false   Audio Track signal out strength monitoring was failed to stop.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_signal_out_monitoring_stop(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Start monitoring the signal in strength of the specific Audio Track session.
 *
 * \note    The frequency number ranges from 0 to 10, specifying the size of the frequency table. If
 *          the frequency number is set to 0, the frequency table shall be empty, and the whole band
 *          of the signal shall be monitored.
 *
 * \param[in] handle            The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] refresh_interval  The refresh interval in milliseconds.
 *
 * \return          The result of starting the Audio Track signal in strength monitoring.
 * \retval  true    Audio Track signal in strength monitoring was started successfully.
 * \retval  false   Audio Track signal in strength monitoring was failed to start.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_signal_in_monitoring_start(T_AUDIO_TRACK_HANDLE handle,
                                            uint16_t             refresh_interval);

/**
 * \brief   Stop monitoring the signal in strength of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The result of stopping the Audio Track signal in strength monitoring.
 * \retval  true    Audio Track signal in strength monitoring was stopped successfully.
 * \retval  false   Audio Track signal in strength monitoring was failed to stop.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_signal_in_monitoring_stop(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Get the maximum volume in level of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] volume   The volume in level of the Audio Track session.
 *
 * \return          The status of getting the maximum volume in level of the Audio Track.
 * \retval true     Audio Track maximum volume in level was got successfully.
 * \retval false    Audio Track maximum volume in level was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_in_max_get(T_AUDIO_TRACK_HANDLE  handle,
                                   uint8_t              *volume);

/**
 * \brief   Get the minimum volume in level of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] volume   The volume in level of the Audio Track session.
 *
 * \return          The status of getting the minimum volume in level the Audio Track.
 * \retval true     Audio Track minimum volume in level was got successfully.
 * \retval false    Audio Track minimum volume in level was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_in_min_get(T_AUDIO_TRACK_HANDLE  handle,
                                   uint8_t              *volume);

/**
 * \brief   Get the current volume in level of the specific Audio Track session.
 *
 * \param[in]  handle   The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] volume   The volume in level of the Audio Track session.
 *
 * \return          The status of getting the Audio Track current volume in level.
 * \retval true     Audio Track current volume in level was got successfully.
 * \retval false    Audio Track current volume in level was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_in_get(T_AUDIO_TRACK_HANDLE  handle,
                               uint8_t              *volume);

/**
 * \brief   Set the current volume in level of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] volume    The volume in level of the Audio Track session, and the range is between
 *                      \ref audio_volume_in_min_get and \ref audio_volume_in_max_get.
 *
 * \return          The status of setting the Audio Track current volume in level.
 * \retval true     Audio Track current volume in level was set successfully.
 * \retval false    Audio Track current volume in level was failed to set.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_in_set(T_AUDIO_TRACK_HANDLE handle,
                               uint8_t              volume);

/**
 * \brief   Mute the current volume in level of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of muting the Audio Track current volume in level.
 * \retval  true    Audio Track current volume in level was muted successfully.
 * \retval  false   Audio Track current volume in level was failed to mute.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_in_mute(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Unmute the current volume in level of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 *
 * \return          The status of unmuting the Audio Track current volume in level.
 * \retval  true    Audio Track current volume in level was unmuted successfully.
 * \retval  false   Audio Track current volume in level was failed to unmute.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_in_unmute(T_AUDIO_TRACK_HANDLE handle);

/**
 * \brief   Get the current volume in mute status of the specific Audio Track session.
 *
 * \param[in]  handle     The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[out] is_muted   The volume in mute status of the Audio Track session.
 *
 * \return          The result of getting the Audio Track current volume in mute status.
 * \retval true     Audio Track current volume in mute status was got successfully.
 * \retval false    Audio Track current volume in mute status was failed to get.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_volume_in_is_muted(T_AUDIO_TRACK_HANDLE  handle,
                                    bool                 *is_muted);

/**
 * \brief   Control all attached Audio Effects of the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] action    The control action of the Audio Track session.
 * \arg \c    0         Apply all attached Audio Effects (default action).
 * \arg \c    1         Bypass all attached Audio Effects.
 *
 * \return          The status of controlling all attached Audio Effects.
 * \retval  true    All attached Audio Effects were controlled successfully.
 * \retval  false   All attached Audio Effects were failed to control.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_effect_control(T_AUDIO_TRACK_HANDLE handle,
                                uint8_t              action);

/**
 * \brief   Attach the Audio Effect instance to the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] instance  The Audio Effect instance \ref T_AUDIO_EFFECT_INSTANCE.
 *
 * \return          The status of attaching the Audio Effect instance.
 * \retval  true    Audio Effect instance was attached successfully.
 * \retval  false   Audio Effect instance was failed to attach.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_effect_attach(T_AUDIO_TRACK_HANDLE    handle,
                               T_AUDIO_EFFECT_INSTANCE instance);

/**
 * \brief   Detach the Audio Effect instance from the specific Audio Track session.
 *
 * \param[in] handle    The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] instance  The Audio Effect instance \ref T_AUDIO_EFFECT_INSTANCE.
 *
 * \return          The status of detaching the Audio Effect instance.
 * \retval  true    Audio Effect instance was detached successfully.
 * \retval  false   Audio Effect instance was failed to detach.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_effect_detach(T_AUDIO_TRACK_HANDLE    handle,
                               T_AUDIO_EFFECT_INSTANCE instance);

bool audio_remote_join_set(bool enable, bool force_join);

/**
 * audio_track.h
 *
 * \brief   Scale the left/right out volume of audio stream
 *
 * \param[in] handle        The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] scale_left    The left channel volume scale ranges from 0.0 to 1.0.
 * \param[in] scale_right   The right channel volume scale ranges from 0.0 to 1.0.
 *
 * \return          The status of scaling the current audio stream volume out.
 * \retval true     The current volume out level was scaled successfully.
 * \retval false    The current volume out level was failed to scale.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_out_scale_set(T_AUDIO_TRACK_HANDLE handle,
                               float                scale_left,
                               float                scale_right);

/**
 * audio_track.h
 *
 * \brief   Set the left/right out volume of audio stream
 *
 * \param[in] handle        The Audio Track session handle \ref T_AUDIO_TRACK_HANDLE.
 * \param[in] left_db       The left channel volume ranges from -128 to 0.
 * \param[in] right_db      The right channel volume ranges from -128 to 0.
 *
 * \return          The status of setting the current audio stream volume out.
 * \retval true     The current volume out db value was set successfully.
 * \retval false    The current volume out db value was failed to set.
 *
 * \ingroup AUDIO_TRACK
 */
bool audio_track_out_db_set(T_AUDIO_TRACK_HANDLE handle,
                            int16_t              left_db,
                            int16_t              right_db);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_TRACK_H_ */
