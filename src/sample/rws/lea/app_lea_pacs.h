#ifndef _APP_LEA_PACS_H_
#define _APP_LEA_PACS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/**
 * @brief  Initialize Audio Capabilities parameter include PAC record,
 *         Audio Location, Context Type,...etc. Register ble audio
 *         callback to handle pacs message.
 * @param  No parameter.
 * @return void
 */
void app_lea_pacs_init(void);

/**@brief Get Sink audio context.
 *
 * @param   No parameter.
 * @return  uint16_t available contexts of sink.
 */
uint16_t app_lea_pacs_get_sink_available_contexts(void);

/**@brief Get Source audio context.
 *
 * @param   No parameter.
 * @return  uint16_t available contexts of source.
 */
uint16_t app_lea_pacs_get_source_available_contexts(void);

/**@brief A set of parameter values that denote low latency is using or not.
 *
 * @param[in] low_latency true:low latency, false:high reliability
 * @return    void.
 */
void app_lea_pacs_update_low_latency(bool low_latency);

/**@brief Initialize available context.
 *
 * @param   conn_handle ble connection handle.
 * \retval  true  success.
 * \retval  false fail.
 */
bool app_lea_pacs_init_available_context(uint16_t conn_handle);

/**@brief Require changing sink audio context.
 *
 * @param   conn_handle ble connection handle.
 * @param   sink_contexts using sink audio context.
 * @param   enable true: enable available context; false: disable available context.
 * \retval  true  success.
 * \retval  false fail.
 */
bool app_lea_pacs_set_sink_available_contexts(uint16_t conn_handle, uint16_t sink_contexts,
                                              bool enable);

/**@brief Require changing source audio context.
 *
 * @param   conn_handle ble connection handle.
 * @param   source_contexts using source audio context.
 * @param   enable true: enable available context; false: disable available context.
 * \retval  true  success.
 * \retval  false fail.
 */
bool app_lea_pacs_set_source_available_contexts(uint16_t conn_handle, uint16_t source_contexts,
                                                bool enable);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif

