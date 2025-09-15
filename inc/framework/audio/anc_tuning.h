/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _ANC_TUNING_H_
#define _ANC_TUNING_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    AUDIO_ANC_TUNING
 *
 * \brief   Provide Active Noise Cancellation (ANC) control tools.
 */

/**
 * \brief ANC response measurement mode.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
typedef enum t_anc_resp_meas_mode
{
    ANC_RESP_MEAS_MODE_NONE,
    ANC_RESP_MEAS_MODE_ENTER,
    ANC_RESP_MEAS_MODE_EXIT,
} T_ANC_RESP_MEAS_MODE;

/**
 * \brief   Start to load ANC scenario image.
 *
 * \param[in]  image_sub_type    ANC image sub-type, only available for LLAPT image.
 * \param[in]  scenario_id       Scenario id for ANC image type.
 *
 * \return  The status of starting to load ANC scenario image.
 * \retval  true     Loading was started successfully.
 * \retval  false    Loading was fail to start.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
bool anc_tool_load_scenario(uint8_t image_sub_type, uint8_t scenario_id);

/**
 * \brief   Set ANC feature map for control.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  feature_map    ANC control feature map.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_set_feature_map(uint32_t feature_map);

/**
 * \brief   Get ANC feature map for ANC control.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \return  Feature map of ANC control.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint32_t anc_tool_get_feature_map(void);

/**
 * \brief   Apply enable ANC settings directly.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_enable(void);

/**
 * \brief   Apply disable ANC settings directly.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_disable(void);

/**
 * \brief   Set ANC register or PALU register.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  anc_cmd_ptr    ANC command which follow HCI CMD format.
 *                            This command will indicate register offset and content,
 *                             and parse by ANC driver.
 *
 * \return  The error code.
 * \retval  0x00      ANC/PALU register was written successfully.
 * \retval  others    ANC/PALU register was failed to be written.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint8_t anc_tool_set_para(void *anc_cmd_ptr);

/**
 * \brief   Get ANC/PALU register content.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  reg_addr    The register address by word.
 *
 * \return  The value of the register.
 * \retval  0xDEADBEEF          Cannot get content by target register address.
 * \retval  others              Content by target register address.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint32_t anc_tool_read_reg(uint32_t reg_addr);

/**
 * \brief   Get the length of ANC data log.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \return  The length of ANC data log.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint32_t anc_tool_get_data_log_length(void);

/**
 * \brief   Enable/Disable playing ringtone by MCU.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  enable       Enable/Disable MCU play ringtone.
 * \param[in]  ch_sel       L/R channel select for measurement.
 * \param[in]  tx_freq      User specific frequencies.
 * \param[in]  freq_num     Number of frequencies.
 * \param[in]  amp_ratio    Tone Amplitude, must be 0 ~ 128.
 *
 * \return  The status of settings.
 * \retval  true    The ringtone was played successfully.
 * \retval  false   The ringtone was failed to play.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
bool anc_tool_response_measure_enable(uint8_t enable, uint8_t ch_sel, uint32_t *tx_freq,
                                      uint8_t freq_num, uint8_t amp_ratio);

/**
 * \brief   Set ANC data log configuration and dump log.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  src0_sel         Log source 0 selection.
 * \param[in]  src1_sel         Log source 1 selection.
 * \param[in]  log_len          Log length (by word).
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_config_data_log(uint8_t src0_sel, uint8_t src1_sel, uint16_t log_len);

/**
 * \brief   Unlock RAM settings to read log directly.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_load_data_log(void);

/**
 * \brief   Convert destination address of ANC data log.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[out]  log_dest_addr    Destination address of ANC data log.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_convert_data_log_addr(uint32_t *log_dest_addr);

/**
 * \brief   Get current response measurement mode.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \return  The mode of response measurement \ref T_ANC_RESP_MEAS_MODE.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
T_ANC_RESP_MEAS_MODE anc_tool_check_resp_meas_mode(void);

/**
 * \brief   Set specific response measurement mode.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  resp_meas_mode    Response measurement mode \ref T_ANC_RESP_MEAS_MODE.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_set_resp_meas_mode(T_ANC_RESP_MEAS_MODE resp_meas_mode);

/**
 * \brief   Set gain mismatch to RAM.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  gain_src    Gain source selection, 0: external MIC, 2: internal MIC.
 * \param[in]  l_gain      Gain value for Lch.
 * \param[in]  r_gain      Gain value for Rch.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_set_gain_mismatch(uint8_t gain_src, uint32_t l_gain, uint32_t r_gain);

/**
 * \brief   Read gain mismatch from RAM or Flash.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]   gain_src      Gain source selection, 0: external MIC, 1: internal MIC.
 * \param[in]   read_flash    Read from Flash or not, 0: from RAM, 1: from Flash.
 * \param[out]  l_gain        Buffer to store Lch gain value.
 * \param[out]  r_gain        Buffer to store Rch gain value.
 *
 * \return  The status of reading gain mismatch.
 * \retval  true    The gain mismatch was read successfully.
 * \retval  false   The gain mismatch was failed to read.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
bool anc_tool_read_gain_mismatch(uint8_t gain_src, uint8_t read_flash,
                                 uint32_t *l_gain, uint32_t *r_gain);

/**
 * \brief   Read MP extend data from Flash.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[out]  mp_ext_data    Buffer to store MP extend data.
 *
 * \return  The status of reading extend data.
 * \retval  true    MP extend data was read successfully.
 * \retval  false   MP extend data was failed to read.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
bool anc_tool_read_mp_ext_data(uint32_t *mp_ext_data);

/**
 * \brief   Read LLAPT extend data from Flash.
 *
 * \param[out]  llapt_ext_data    Buffer to store LLAPT extend data.
 *
 * \return  The status of reading extend LLAPT extend data.
 * \retval  true    LLAPT extend data was read successfully.
 * \retval  false   LLAPT extend data was failed to read.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
bool anc_tool_read_llapt_ext_data(uint32_t *llapt_ext_data);

/**
 * \brief   Burn gain mismatch and MP extend data from RAM to Flash.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  mp_ext_data   MP extend data.
 *
 * \return  The status of burn gain.
 * \retval  true    MP extend data was burned successfully.
 * \retval  false   MP extend data was failed to burn.
 *
 * \ingroup AUDIO_ANC_TUNING
 */

bool anc_tool_burn_gain_mismatch(uint32_t mp_ext_data);

/**
* anc_tuning.h
*
* \brief   Get ANC scenario mode.
*
* \param[out]  scenario_mode    Get ANC scenario mode.
*
* \return  The number of ANC scenarios.
*
* \ingroup AUDIO_ANC_TUNING
*/
uint8_t anc_tool_get_anc_scenario_info(uint8_t *scenario_mode);

/**
* anc_tuning.h
*
* \brief   Get LLAPT scenario information.
*
* \param[out]  scenario_mode          Get LLAPT scenario mode.
* \param[out]  scenario_apt_effect    Get LLAPT effect.
*
* \return   The number of ANC scenarios.
*
* \ingroup AUDIO_ANC_TUNING
*/
uint8_t anc_tool_get_llapt_scenario_info(uint8_t *scenario_mode, uint8_t *scenario_apt_effect);

/**
 * \brief   Set LLAPT gain mismatch to RAM.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  l_gain    Gain value for Lch.
 * \param[in]  r_gain    Gain value for Rch.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_set_llapt_gain_mismatch(uint32_t l_gain, uint32_t r_gain);

/**
 * \brief   Read LLAPT gain mismatch from RAM or Flash.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]   read_flash Read from Flash or not, 0: from RAM, 1: from Flash.
 * \param[out]  l_gain     Buffer to store Lch gain value.
 * \param[out]  r_gain     Buffer to store Rch gain value.
 *
 * \return  The status of reading LLAPT gain.
 * \retval  true    LLAPT gain mismatch was read successfully.
 * \retval  false   LLAPT gain mismatch was failed to read.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
bool anc_tool_read_llapt_gain_mismatch(uint8_t read_flash, uint32_t *l_gain, uint32_t *r_gain);

/**
 * \brief   Burn LLAPT gain mismatch from RAM to Flash.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  llapt_ext_data    LLAPT extend data.
 *
 * \return  The status of burn LLAPT gain.
 * \retval  true    LLAPT gain mismatch was burned successfully.
 * \retval  false   LLAPT gain mismatch was failed to burn.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
bool anc_tool_burn_llapt_gain_mismatch(uint32_t llapt_ext_data);

/**
 * \brief   Switch ANC/LLAPT limiter WNS on/off depend on feature map.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_limiter_wns_switch(void);

/**
 * \brief   Set ANC MP backup buffer.
 *
 * \param[in]  param1    Control parameters.
 * \param[in]  param2    Control parameters.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_anc_mp_bak_buf_config(uint32_t param1, uint32_t param2);

/**
 * \brief   Ramp data write for ramp tool.
 *
 * \param[in]  wdata    Data to be written.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_ramp_data_write(uint32_t wdata);

/**
 * \brief   Get ramp scenario address for ramp tool.
 *
 * \param[in]  param    Command buffer address send to driver.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_get_scenario_addr(uint32_t param);

/**
 * \brief   Set scenario image to ANC module.
 *
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 *
 * \param[in] image_buf    Image scenario.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
void anc_tool_set_scenario(void *image_buf);

/**
* anc_tuning.h
*
* \brief   Get scenario image.
*
* \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
*
* \param[in]    sub_type       ANC or LLAPT type.
* \param[in]    scenario_id    Scenario index.
* \param[out]   length         ANC or LLAPT length.
*
* \return The buffer of scenario image.
*
* \ingroup AUDIO_ANC_TUNING
*/
void *anc_tool_get_scenario_img(uint8_t sub_type, uint8_t scenario_id, uint32_t *length);

/**
* anc_tuning.h
*
* \brief   Get PALU filter information.
*
* \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
*
* \param[out]  length    The filter information length.
*
* \return   The buffer of filter information.
*
* \ingroup AUDIO_ANC_TUNING
*/
void *anc_tool_get_filter_info_img(uint32_t *length);

/**
* anc_tuning.h
*
* \brief   Encrypt image.
*
* \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
*
* \param[in]  scenario_img    Scenario image.
* \param[in]  length          The length of image.
*
* \ingroup AUDIO_ANC_TUNING
*/
void anc_tool_para_aes_encrypt(uint32_t scenario_img, uint32_t length);

/**
 * \brief   Query ADSP information.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[out]  adsp_info   The information of ADSP.
 *
 * \return  The status of getting ADSP information.
 * \retval  0x00     The information was got successfully.
 * \retval  others   The information was failed to get.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint8_t anc_tool_query_para(uint8_t *adsp_info);

/**
 * \brief   Config ADSP transfer parameters.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  crc_check        The CRC16 checksum for all ADSP parameter payload.
 * \param[in]  adsp_para_len    The length of adsp parameter payload.
 * \param[in]  segment_total    The total number of segment.
 *
 * \return  The status of config.
 * \retval  0x00    The config was got successfully.
 * \retval  others  The config was failed to get.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint8_t anc_tool_config_adsp_para(uint16_t crc_check, uint32_t adsp_para_len,
                                  uint8_t segment_total);

/**
 * \brief   Enable or disable ADSP.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  enable    1: enable ADSP, 0: disable ADSP.
 *
 * \return  The status of config.
 * \retval  0x00    Enabled or disabled ADSP successfully.
 * \retval  others  ADSP was fail to enable or disable.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint8_t anc_tool_turn_on_adsp(uint8_t enable);

/**
 * \brief   Set ADSP paramters.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  para_ptr         ADSP paramters array.
 * \param[in]  segment_index    The maximun is (adsp_para_len / mtusize + 1).
 *
 * \return  The status of setting ADSP paramters.
 * \retval  0x00    ADSP register was written successfully.
 * \retval  others  ADSP register was failed to write.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint8_t anc_tool_load_adsp_para(uint8_t *para_ptr, uint8_t segment_index);

/**
 * \brief   Set adaptive ANC enable or disable.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  enable  1:enable, 0: disable.
 *
 * \return  The status of adaptive ANC enable or disable.
 * \retval  0x00    ANC/PALU register enabled or disabled successfully.
 * \retval  others  ANC/PALU register was failed to enabled or disabled.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint8_t anc_tool_enable_adaptive_anc(uint8_t enable, uint8_t grp_idx);

/**
 * \brief   Start to load ADSP image.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  encode  The config from tool to indicate which image should be loaded.
 *
 * \return  The status of starting to load ADSP image.
 * \retval  0x00     The ADSP image loading was started successfully.
 * \retval  others   The ADSP image loading was failed to start.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint8_t anc_tool_load_adsp_image(uint32_t encode);

/**
 * \brief   Check ADSP image load done.
 *          Only provided for tool usage, cannot invoke in normal uasge.
 *
 * \param[in]  encode The config from tool to indicate which image should be loaded
 *
 * \return  The status of ADSP image loading.
 * \retval  0x01   The ADSP image was loaded successfully.
 * \retval  0x00   The ADSP image wad failed to load.
 *
 * \ingroup AUDIO_ANC_TUNING
 */
uint8_t anc_tool_get_adsp_image_load_done(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ANC_TUNING_H_ */
