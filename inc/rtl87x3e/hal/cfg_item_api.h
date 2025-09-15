/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     cfg_item_api.h
* @brief    This file provides API for config item operation.
* @details
* @author
* @date     2021-12-9
* @version  v1.0
*****************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __CFG_ITEM_API_H_
#define __CFG_ITEM_API_H_


/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/** @defgroup HAL_87x3E_CFG_ITEM_API Config Item
    * @brief Config item.
    * @{
    */
#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
 *                              Variables
*============================================================================*/

/** @addtogroup HAL_87x3E_CFG_Item_API_Exported_Types Config Item Exported Types
  * @{
  */

/** @defgroup HAL_87x3E_CFG_SERACH_RESULT_E  CFG_SERACH_RESULT_E
* @{
*/
typedef enum
{
    CFG_SERACH_ENTRY_NOT_EXIST = -1,
    CFG_SERACH_ENTRY_SUCCESS = 0,
    CFG_SERACH_ENTRY_SIZE_MISMATCH = 1,
    CFG_SERACH_MODULE_SIZE_MISMATCH = 2,
    CFG_SERACH_MODULE_SIG_MISMATCH = 3,
    CFG_SEARCH_ENTRY_SIZE_INVALID = 4,
} CFG_SERACH_RESULT_E;
/** End of HAL_87x3E_CFG_SERACH_RESULT_E
  * @}
  */
/** @defgroup HAL_87x3E_CFG_UPDATE_RESULT_E  CFG_UPDATE_RESULT_E
* @{
*/
typedef enum
{
    CFG_UPDATE_SUCCESS = 0,
    CFG_UPDATE_UNLOCK_BP_FAIL = 1,
    CFG_UPDATE_ERASE_FAIL = 2,
    CFG_UPDATE_WRITE_FAIL = 3,
    CFG_UPDATE_LOCK_BP_FAIL = 4,
} CFG_UPDATE_RESULT_E;
/** End of HAL_87x3E_CFG_UPDATE_RESULT_E
  * @}
  */
/** @defgroup HAL_87x3E_CFG_OP_CODE_E  CFG_OP_CODE_E
* @{
*/
typedef enum
{
    CFG_READ   = 1,
    CFG_UPDATE = 2,
} CFG_OP_CODE_E;
/** End of HAL_87x3E_CFG_OP_CODE_E
 * @}
 */
/** @defgroup HAL_87x3E_CFG_OP_CODE_E  CFG_OP_CODE_E
* @{
*/
typedef enum
{
    CFG_READ_NONE      = 0, //!< don't need to read
    CFG_READ_SUCCESS   = 1,
    CFG_READ_NOT_EXIST = 2,
    CFG_READ_FAIL      = 3,
} CFG_READ_RESULT_E;
/** End of HAL_87x3E_CFG_OP_CODE_E
 * @}
 */
/** @defgroup HAL_87x3E_LBT_CFG_T  LBT_CFG_t
* @{
*/
typedef struct
{
    /*Adaptivity (LBT)*/
    /* LBTEnableFlag:
     * Update(1: set the LBTEnable, 0: don't set LBTEnable).
     * Read request(1: read the LBTEnable, 0: don't read LBTEnable).
     * Read response(CFG_READ_RESULT_E).
    */
    uint8_t LBTEnableFlag;
    uint8_t LBTEnable;     //!< 0:Disable 1:Enable

    /* LBTModeFlag:
     * Update(1: set the LBTMode, 0: don't set LBTMode).
     * Read request(1: read the LBTMode, 0: don't read LBTMode).
     * Read response(CFG_READ_RESULT_E).
    */
    uint8_t LBTModeFlag;
    uint8_t LBTMode;      //!< 0: No Tx, 1: Reduce Power Tx

    /* LBTThresholdFlag:
     * Update(1: set the LBTThreshold, 0: don't set LBTThreshold).
     * Read request(1: read the LBTThreshold, 0: don't read LBTThreshold).
     * Read response(CFG_READ_RESULT_E).
    */
    uint8_t LBTThresholdFlag;
    union
    {
        uint8_t LBTThresholdIndex; //!< set LBTThreshold with index value
        int8_t  LBTThresholdDBI; //!< set LBTThreshold with dbi value
    } LBTThreshold;

    /* LBTAntennaGainFlag:
     * Update(1: set the LBTAntennaGain, 0: don't set LBTAntennaGain).
     * Read request(1: read the LBTAntennaGain, 0: don't read LBTAntennaGain).
     * Read response(CFG_READ_RESULT_E).
    */
    uint8_t LBTAntennaGainFlag;
    union
    {
        uint8_t LBTAntennaGainIndex; //!< set LBTAntennaGain with index value
        int8_t LBTAntennaGainDBM; //!< set LBTAntennaGain with dbm value
    } LBTAntennaGain;
} LBT_CFG_t;
/** End of HAL_87x3E_LBT_CFG_t
  * @}
  */
/** @defgroup HAL_87x3E_TX_POWER_CFG_t  TX_POWER_CFG_t
* @{
*/
typedef struct
{
    /* txgain_br_1M_flag:
     * Update(1: set the txgain_br_1M, 0: don't set txgain_br_1M).
     * Read request(1: set the txgain_br_1M, 0: don't set txgain_br_1M).
     * Read response(CFG_READ_RESULT_E).
     */
    uint8_t txgain_br_1M_flag; //!< 1: set the txgain_br_1M, 0: don't set txgain_br_1M
    union
    {
        uint8_t txgain_br_1M_index; //!< set txgain_br_1M with index value
        int8_t  txgain_br_1M_dbm; //!< set txgain_br_1M with dbm value
    } txgain_br_1M;

    /* txgain_edr_2M_flag:
     * Update(1: set the txgain_edr_2M, 0: don't set txgain_edr_2M).
     * Read request(1: set the txgain_edr_2M, 0: don't set txgain_edr_2M).
     * Read response(CFG_READ_RESULT_E).
     */
    uint8_t txgain_edr_2M_flag;
    union
    {
        uint8_t txgain_edr_2M_index; //!< set txgain_edr_2M with index value
        int8_t txgain_edr_2M_dbm; //!< set txgain_edr_2M with dbm value
    } txgain_edr_2M;

    /* txgain_edr_3M_flag:
     * Update(1: set the txgain_edr_3M, 0: don't set txgain_edr_3M).
     * Read request(1: set the txgain_edr_3M, 0: don't set txgain_edr_3M).
     * Read response(CFG_READ_RESULT_E).
     */
    uint8_t txgain_edr_3M_flag;
    union
    {
        uint8_t txgain_edr_3M_index; //!< set txgain_edr_3M with index value
        int8_t txgain_edr_3M_dbm; //!< set txgain_edr_3M with dbm value
    } txgain_edr_3M;

    /* txgain_br_LE1M_flag:
     * Update(1: set the txgain_br_LE1M, 0: don't set txgain_br_LE1M).
     * Read request(1: set the txgain_br_LE1M, 0: don't set txgain_br_LE1M).
     * Read response(CFG_READ_RESULT_E).
    */
    uint8_t txgain_br_LE1M_flag;
    union
    {
        uint8_t txgain_edr_LE1M_index; //!< set txgain_br_LE1M with index value
        int8_t txgain_edr_LE1M_dbm; //!< set txgain_br_LE1M with dbm value
    } txgain_edr_LE1M;

    /* txgain_br_LE2M_flag:
     * Update(1: set the txgain_br_LE2M, 0: don't set txgain_br_LE2M).
     * Read request(1: set the txgain_br_LE2M, 0: don't set txgain_br_LE2M).
     * Read response(CFG_READ_RESULT_E).
    */
    uint8_t txgain_br_LE2M_flag;
    union
    {
        uint8_t txgain_edr_LE2M_index; //!< set txgain_br_LE2M with index value
        int8_t txgain_edr_LE2M_dbm; //!< set txgain_br_LE2M with dbm value
    } txgain_edr_LE2M;
} TX_POWER_CFG_t;
/** End of HAL_87x3E_TX_POWER_CFG_T
  * @}
  */

/** End of HAL_87x3E_Cfg_Item_API_Exported_Types
  * @}
  */
/*============================================================================*
 *                              Functions
*============================================================================*/
/** @addtogroup HAL_87x3E_Cfg_Item_Ext_Exported_Functions Config Item Exported Functions
  * @{
  */

/**
 * @brief Get the config payload length.
 * @warning This API is only supported in RTL87x3E.
 *          It is NOT supported in RTL87x3D and RTL87x3G.
 * \param p_cfg_payload   Set NULL to get the config module total length on flash OCCD_ADDRESS.
 * Set a valid RAM address to get the config module total length on the RAM buffer.
 *
 * @return The total config module length.
 */
uint32_t cfg_get_size(void *p_cfg_payload);
/**
 * @brief Read back the config data on the flash to the RAM buffer.
 * @warning This API is only supported in RTL87x3E.
 *          It is NOT supported in RTL87x3D and RTL87x3G.
 * \param address   Specify the sytem config read address,default set as OCCD_ADDRESS.
 * \param backup_len  Specify the backup config data length.
 *
 * @return The config buffer backup pointer on the heap.
 */
void *cfg_backup(uint32_t address, uint32_t backup_len);
/**
 * @brief Write the config data on the RAM to the flash.
 * @warning This API is only supported in RTL87x3E.
 *          It is NOT supported in RTL87x3D and RTL87x3G.
 * \param p_new_cfg_buf   Pointer to the occd config payload data backup on RAM.
 * \param backup_len      Specify the config buffer length.
 *
 * @return The result of config buffer writing operation.
 * @retval true              Success.
 * @retval false             Fail.
 */
bool cfg_write_to_flash(void *p_new_cfg_buf, uint32_t backup_len);
/**
 * @brief Update the config entry item in the config data buffer
 * @warning This API is only supported in RTL87x3E.
 *          It is NOT supported in RTL87x3D and RTL87x3G.
 * \param p_new_cfg_buf   Pointer to the occd config payload data backup on RAM.
 * \param module_id       Specify the config item module id.
 * \param p_cfg_entry     Pointer to the config entry item to update.
 * @return The result of updating the config item in p_new_cfg_buf.
 * @retval true              Success.
 * @retval false             Fail.
 */
bool cfg_update_item_in_store(uint8_t *p_new_cfg_buf, uint16_t module_id,
                              void *p_cfg_entry);

/**
 * @brief Update the config entry item on the occd flash.
 * @warning This API is only supported in RTL87x3E.
 *          It is NOT supported in RTL87x3D and RTL87x3G.
 * \param module_id       Specify the config item module id.
 * \param p_cfg_entry     Pointer to the config entry item to update.
 * @return The result of updating the config item on the flash of system config.
 * @retval true              Success.
 * @retval false             Fail.
 */
bool cfg_add_item(uint16_t module_id, void *p_cfg_entry);

/**
  * @brief Write MAC address to config, this is mainly used on production line.
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] p_mac_addr  The buffer hold MAC address (6 bytes).
  * @return Write MAC to config fail or success.
  * @retval true    Write MAC to config success.
  * @retval false   Write MAC to config fails or not write existed MAC.
 */
bool cfg_update_mac(uint8_t *p_mac_addr);
/**
  * @brief Write 40M XTAL calibration data to config sc_xi_40m and sc_xo_40m.
  *        This is mainly used on production line.
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] xtal               The value of 40M XTAL calibration data
  * @return Write calibration data to config fail or success.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_update_xtal(uint8_t xtal);

/**
  * @brief Write tx gain k calibration data to config txgaink_module and set txgaink_module_valid as 1.
    *      This is mainly used on production line.
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] tx_gain_k        The value of tx_gain_k calibration data.
  * @return Write calibration data to config fail or success.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_update_txgaink(int8_t tx_gain_k);

/**
  * @brief Write txgaink_module_valid as 0 or 1.
  *        This is mainly used on production line.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] txgaink_module_valid   Set true to update txgaink module valid.
  *                                   Set false to update txgaink module invalid.
  * @return Write txgaink_module_valid to config fail or success.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_update_txgaink_vaild(bool txgaink_module_valid);

/**
  * @brief Write tx gain k calibration data to config txgain_flatk_module and set txgain_flatk_module_valid as 1.
  *        This is mainly used on production line.
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] tx_flatk_0        The value of txgain_flatk_module[0] calibration data.
  * @param[in] tx_flatk_1        The value of txgain_flatk_module[1] calibration data.
  * @param[in] tx_flatk_2        The value of txgain_flatk_module[2] calibration data.
  * @param[in] tx_flatk_3        The value of txgain_flatk_module[3] calibration data.
  * @return Write calibration data to config fail or success.
  * @retval true              Success.
  * @retval false             Fail.

  */
bool cfg_update_txgain_flatk(int8_t tx_flatk_0, int8_t tx_flatk_1, int8_t tx_flatk_2,
                             int8_t tx_flatk_3);

/**
  * @brief Write txgain_flatk_module_valid as 0 or 1.
  *        This is mainly used on production line.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] txgain_flatk_module_valid  Set true to update the txgain flatk module valid.
  *                                       Set false to update the txgain flatk module invalid.
  * @return Write txgain_flatk_module_valid to config fail or success.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_update_txgain_flatk_vaild(bool txgain_flatk_module_valid);
/**
  * @brief Get calibration status of xtal cap.
  * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
  * @param[in] void
  * @return  Calibration status for xtal cap.
  *     @retval true              calibrated.
  *     @retval false             not calibrated.
  */
bool cfg_get_xtal_cap_is_cal(void);
/**
  * @brief Update LBT config data into the system config.
  *        This is mainly used on production line.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] lbt_cfg      The pointer to @ref LBT_CFG_t structure data.
  * @return The result of writing LBT config data to system config.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_update_LBT(LBT_CFG_t *lbt_cfg);
/**
  * @brief Update thermal default txgaink config data into the system config.
  *        This is mainly used on production line.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] thermal_default_txgaink      Specify the value to set for thermal default txgaink.
  * @return The result of writing thermal default txgaink config data to system config.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_update_thermal(uint8_t thermal_default_txgaink);
/**
  * @brief Update TX Power config data into the system config.
  *        This is mainly used on production line.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] tx_power_cfg        The pointer to @ref TX_POWER_CFG_t structure data.
  * @return The result of writing TX power config data to system config.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_update_txpower(TX_POWER_CFG_t *tx_power_cfg);
/**
 * @brief Read the config entry item on the sys config image.
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
 * @warning This API is only supported in RTL87x3E.
 *          It is NOT supported in RTL87x3D and RTL87x3G.
 * \param module_id       Specify the config item module id.
 * \param p_cfg_entry     Pointer to the config entry item to read, input the item offset, len and mask, output data.
 * @return  @ref CFG_SERACH_RESULT_E The result of search the config item.
 */
CFG_SERACH_RESULT_E cfg_read_item(uint16_t module_id, void *p_cfg_entry);
/**
  * @brief Read MAC address.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[in] p_mac_addr  Specify the buffer to read back the MAC address (6 bytes).
  * @return Read MAC result.
  * @retval true    Read MAC success.
  * @retval false   Read MAC fail or not existed.
 */
bool cfg_read_mac(uint8_t *p_mac_addr);
/**
  * @brief Read 40M XTAL XI(sc_xi_40m) and XO(sc_xo_40m) data.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[out] p_xtal_xi    Get the value of 40M XTAL XI calibration data.
  * @param[out] p_xtal_xo    Get the value of 40M XTAL XO calibration data.
  * @return The result of reading 40M XTAL calibration data.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_read_xtal(uint8_t *p_xtal_xi, uint8_t *p_xtal_xo);
/**
  * @brief Read tx gain k calibration data.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[out] p_tx_gain_k_valid  Get the value of txgaink_module_valid.
  *                                1: valid, 0:invalid.
  * @param[out] p_tx_gain_k        Get the value of tx_gain_k calibration data,
  *                                It's valid when tx_gain_k_valid is 1.
  * @return The result of reading txgaink_module_valid and tx_gain_k data.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_read_txgaink(uint8_t *p_tx_gain_k_valid, int8_t *p_tx_gain_k);
/**
  * @brief Read tx gain k calibration data.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[out] p_tx_gain_flatk_valid   Get the value of txgain_flatk_module_valid.
  *                                 All of the tx flatk data are valid when txgain_flatk_module_valid is 1.
  * @param[out] p_tx_flatk_0        Get the value of txgain_flatk_module[0] calibration data.
  * @param[out] p_tx_flatk_1        Get the value of txgain_flatk_module[1] calibration data.
  * @param[out] p_tx_flatk_2        Get the value of txgain_flatk_module[2] calibration data.
  * @param[out] p_tx_flatk_3        Get the value of txgain_flatk_module[3] calibration data.
  * @return The result of reading txgain flatk calibration data.
  * @retval true              Success
  * @retval false             Fail.
  */
bool cfg_read_txgain_flatk(uint8_t *p_tx_gain_flatk_valid,
                           int8_t *p_tx_flatk_0,
                           int8_t *p_tx_flatk_1,
                           int8_t *p_tx_flatk_2,
                           int8_t *p_tx_flatk_3);
/**
  * @brief Read thermal default txgaink config data.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[out] p_thermal_default_txgaink   Get the value of thermal default txgaink.
  * @return The result of reading thermal default txgaink config data.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_read_thermal(uint8_t *p_thermal_default_txgaink);
/**
  * @brief Read TX Power config data.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[inout] tx_power_cfg   The pointer to @ref TX_POWER_CFG_t structure data
  * @return The result of reading TX power config data.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_read_txpower(TX_POWER_CFG_t *tx_power_cfg);
/**
  * @brief Read LBT config data.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL87x3E.
  *          It is NOT supported in RTL87x3D and RTL87x3G.
  * @param[inout] lbt_cfg    The pointer to LBT_CFG_t structure data.
  * @return The result of reading LBT config data.
  * @retval true             Success.
  * @retval false            Fail.
  */
bool cfg_read_LBT(LBT_CFG_t *lbt_cfg);
/**
  * @brief Update low power frequency offset config data into the system config.
  *        This is mainly used on production line.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL8763E and RTL8773E.
  *          It is NOT supported in RTL87x3D.
  * @param[in] low_power_freq_offset     It's the value to set for xtal_ppm_plus and xtal_ppm_err.
  * @return The result of writing low power frequency offset config data to system config.
  * @retval True              Success.
  * @retval false             Fail.
  */
bool cfg_update_low_power_freq_offset(uint16_t low_power_freq_offset);
/**
  * @brief Read low power frequency offset config data.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL8763E and RTL8773E.
  *          It is NOT supported in RTL87x3D.
  * @param[out] p_low_freq_offset   Get the value of low power frequency offset.
  * @return The result of reading low power frequency offset config data.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_read_low_power_freq_offset(uint16_t *p_low_freq_offset);
/**
  * @brief Update esco link config data into the system config.
  *        This is mainly used on production line.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL8763E and RTL8773E.
  *          It is NOT supported in RTL87x3D.
  * @param[in] esco_link     It's the value to set for esco_link, 0~3.
  * @return The result of writing esco link config data to system config.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_update_esco_link(uint8_t esco_link);
/**
  * @brief Read esco link config data.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL8763E and RTL8773E.
  *          It is NOT supported in RTL87x3D.
  * @param[out] p_esco link   Get the value of esco link.
  * @return The result of reading esco link config data.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_read_esco_link(uint8_t *p_esco_link);
/**
  * @brief Update enable expa config data into the system config.
  *        This is mainly used on production line.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL8763E and RTL8773E.
  *          It is NOT supported in RTL87x3D.
  * @param[in] enable expa     It's the value to set for enable expa, 0 or 1.
  * @return The result of writing enable expa config data to system config.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_update_enable_expa(uint8_t enable_expa);
/**
  * @brief Read enable expa config data.
  * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Added API"
  * @warning This API is only supported in RTL8763E and RTL8773E.
  *          It is NOT supported in RTL87x3D.
  * @param[out] p_enable_expa   Get the value of enable expa.
  * @return The result of reading enable expa config data.
  * @retval true              Success.
  * @retval false             Fail.
  */
bool cfg_read_enable_expa(uint8_t *p_enable_expa);
/** End of HAL_87x3E_CFG_ITEM_EXT_EXPORTED_FUNCTIONS
  * @}
  */
#ifdef __cplusplus
}
#endif
/** @} */ /* End of group HAL_87x3E_CFG_ITEM_API */
#endif
