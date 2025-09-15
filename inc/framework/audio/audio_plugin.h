/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _AUDIO_PLUGIN_H_
#define _AUDIO_PLUGIN_H_

#include <stdint.h>
#include <stdbool.h>
#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_PLUGIN Audio Plugin
 *
 * \brief   Customize the Audio Plugin configurations.
 */

/**
 * \brief Define the Audio Plugin instance handle, created by \ref audio_plugin_create.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef void *T_AUDIO_PLUGIN_HANDLE;

/**
 * \brief Define the Audio Plugin parameter type, used in \ref P_AUDIO_PLUGIN_ACTION_HANDLER.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef struct t_audio_plugin_param
{
    uint32_t dac_sample_rate;
    uint32_t adc_sample_rate;
} T_AUDIO_PLUGIN_PARAM;

/**
 * \brief Define the Audio Plugin action handler prototype.
 *
 * \details Each Audio Plugin instance has the capability to perform customized actions at specific
 *          occasions as defined by \ref T_AUDIO_PLUGIN_OCCASION. When creating an Audio Plugin
 *          instance using \ref audio_plugin_create, the action handler within \ref T_AUDIO_PLUGIN_POLICY
 *          is registered to Audio Plugin.
 *
 * \note    The Audio Plugin action is always treated as an asynchronous procedure, regardless of its
 *          implementation details. This means that the Audio Plugin cannot determine when the action
 *          is completed until the application returns the same context through the \ref audio_plugin_msg_send
 *          interface.
 *
 * \param[in]     handle       The Audio Plugin instance handle \ref T_AUDIO_PLUGIN_HANDLE.
 * \param[in]     param        The parameter for the Audio Plugin action handler.
 * \param[in]     context      The context for the Audio Plugin instance to use in \ref audio_plugin_msg_send.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef void (*P_AUDIO_PLUGIN_ACTION_HANDLER)(T_AUDIO_PLUGIN_HANDLE handle,
                                              T_AUDIO_PLUGIN_PARAM  param,
                                              void                 *context);

/**
 * \brief Define the Audio Plugin occasions.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef enum t_audio_plugin_occasion
{
    AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_ON,
    AUDIO_PLUGIN_OCCASION_ANALOG_DOMAIN_OFF,
    AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_ON,
    AUDIO_PLUGIN_OCCASION_DIGITAL_DOMAIN_OFF,
    AUDIO_PLUGIN_OCCASION_NUM,
} T_AUDIO_PLUGIN_OCCASION;

/**
 * \brief Define the Audio Plugin policies.
 *
 * \ingroup AUDIO_PLUGIN
 */
typedef struct t_audio_plugin_policy
{
    T_AUDIO_CATEGORY              category;
    T_AUDIO_PLUGIN_OCCASION       occasion;
    P_AUDIO_PLUGIN_ACTION_HANDLER handler;
} T_AUDIO_PLUGIN_POLICY;

/**
 * \brief   Create an Audio Plugin instance.
 *
 * \param[in] policies    The array of customized usage policies \ref T_AUDIO_PLUGIN_POLICY.
 * \param[in] count       The number of array elements.
 * \return  The handle of the created Audio Plugin instance. If the returned handle is NULL, the
 *          Audio Plugin instance was failed to create.
 *
 * \ingroup AUDIO_PLUGIN
 */
T_AUDIO_PLUGIN_HANDLE audio_plugin_create(const T_AUDIO_PLUGIN_POLICY *policies,
                                          uint32_t                     count);

/**
 * \brief   Release the specific Audio Plugin instance.
 *
 * \param[in] handle       The Audio Plugin instance handle returned by \ref audio_plugin_create.
 * \return                 The result of releasing the Audio Plugin instance.
 * \retval    true         The Audio Plugin instance was released successfully.
 * \retval    false        The Audio Plugin instance was failed to release.
 *
 * \ingroup AUDIO_PLUGIN
 */
bool audio_plugin_release(T_AUDIO_PLUGIN_HANDLE handle);

/**
 * \brief   The application sends a message to the Audio Plugin instance to indicate that the
 *          customized action has been completed.
 *
 * \param[in] handle        The Audio Plugin instance handle \ref T_AUDIO_PLUGIN_HANDLE.
 * \param[in] context       The context parameter in \ref P_AUDIO_PLUGIN_ACTION_HANDLER.
 *
 * \ingroup AUDIO_PLUGIN
 */
void audio_plugin_msg_send(T_AUDIO_PLUGIN_HANDLE   handle,
                           void                   *context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_PLUGIN_H_ */
