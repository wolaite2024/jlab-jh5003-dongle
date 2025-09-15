/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     has_client.h
  * @brief    Header file for Hearing Aid Client.
  * @details  This file defines Hearing Aid Client related API.
  * @author
  * @date
  * @version
  * *************************************************************************************
  */

#ifndef _HAS_CLIENT_H_
#define _HAS_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include <stdbool.h>
#include "has_def.h"

/**
 * \defgroup    LEA_USE_CASE_HAP_Client Hearing Aid Client
 *
 * \brief   Control and interact with hearing aid
 */

/**
 * \defgroup HAP_Client_Exported_Macros Hearing Aid Client Exported Macros
 *
 * \ingroup LEA_USE_CASE_HAP_Client
 * @{
 */

/**
 * has_client.h
 *
 * \defgroup  HAS_CFG_FLAGS HAS Service Characteristic Flags
 *
 * \brief  Define HAS service characteristic flags.
 *
 * \ingroup LEA_USE_CASE_HAP_Client
 * @{
 */
#define HAS_HEARING_AID_FEATURE_FLAG     0x01  /**< Hearing Aid Feature flag for CCCD configuration. */
#define HAS_HEARING_AID_PRESET_CP_FLAG   0x02  /**< Hearing Aid Preset Control Point flag for CCCD configuration. */
#define HAS_ACTIVE_PRESET_IDX_FLAG       0x04  /**< Active Preset Index flag for CCCD configuration. */
/**
 * End of HAS_CFG_FLAGS
 * @}
 */

/**
 * End of HAP_Client_Exported_Macros
 * @}
 */

/**
 * \defgroup HAP_Client_Exported_Types Hearing Aid Client Exported Types
 *
 * \ingroup LEA_USE_CASE_HAP_Client
 * @{
 */

/**
 * has_client.h
 *
 * \brief  HAS CCCD configuration opcode.
 *
 * \ingroup HAP_Client_Exported_Types
 */
typedef enum
{
    HAS_OP_ENABLE_ALL = 0x00
} T_HAS_CCCD_OP_TYPE;

/**
 * has_client.h
 *
 * \brief  HAS client discover HAS service result.
 *
 * The message data for @ref LE_AUDIO_MSG_HAS_CLIENT_DIS_DONE.
 *
 * \ingroup HAP_Client_Exported_Types
 */
typedef struct
{
    uint16_t conn_handle;
    bool     is_found;       /**< Whether to find service. */
    bool     load_from_ftl;  /**< Whether the service table is loaded from FTL. */
    bool     is_ha_cp_exist;
    bool     is_ha_cp_notify;
    bool     is_ha_features_notify_support;
} T_HAS_CLIENT_DIS_DONE;

/**
 * has_client.h
 *
 * \brief  HAS client read Hearing Aid Features result.
 *
 * The message data for @ref LE_AUDIO_MSG_HAS_CLIENT_READ_HA_FEATURES_RESULT.
 *
 * \ingroup HAP_Client_Exported_Types
 */
typedef struct
{
    uint16_t conn_handle;
    uint16_t cause;
    T_HAS_HA_FEATURES has_feature;
} T_HAS_CLIENT_READ_HA_FEATURES_RESULT;

/**
 * has_client.h
 *
 * \brief  HAS client read Active Preset Index result.
 *
 * The message data for @ref LE_AUDIO_MSG_HAS_CLIENT_READ_ACTIVE_PRESET_IDX_RESULT.
 *
 * \ingroup HAP_Client_Exported_Types
 */
typedef struct
{
    uint16_t conn_handle;
    uint16_t cause;
    uint8_t active_preset_idx;
} T_HAS_CLIENT_READ_ACTIVE_PRESET_IDX_RESULT;

/**
 * has_client.h
 *
 * \brief  HAS client write Control Point result.
 *
 * The message data for @ref LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT.
 *
 * \ingroup HAP_Client_Exported_Types
 */
typedef struct
{
    uint16_t conn_handle;
    uint16_t cause;
} T_HAS_CLIENT_CP_RESULT;

/**
 * has_client.h
 *
 * \brief  HAS client receive Hearing Aid Features notify data.
 *
 * The message data for @ref LE_AUDIO_MSG_HAS_CLIENT_HA_FEATURES_NOTIFY.
 *
 * \ingroup HAP_Client_Exported_Types
 */
typedef struct
{
    uint16_t conn_handle;
    T_HAS_HA_FEATURES has_feature;
} T_HAS_CLIENT_HA_FEATURES_NOTIFY;

/**
 * has_client.h
 *
 * \brief  HAS client receive Active Preset Index notify data.
 *
 * The message data for @ref LE_AUDIO_MSG_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY.
 *
 * \ingroup HAP_Client_Exported_Types
 */
typedef struct
{
    uint16_t conn_handle;
    uint8_t active_preset_idx;
} T_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY;

/**
 * has_client.h
 *
 * \brief  Control point notify or indication data.
 *
 * \ingroup HAP_Client_Exported_Types
 */
typedef struct
{
    T_HAS_CP_OP cp_op;
    uint8_t change_id;
    uint8_t is_last;
    uint8_t pre_idx;
    uint8_t name_length;
    T_HAS_PRESET_FORMAT preset;
} T_HAS_CP_NOTIFY_IND_DATA;

/**
 * has_client.h
 *
 * \brief  HAS client receive control point notify or indication data.
 *
 * The message data for @ref LE_AUDIO_MSG_HAS_CLIENT_CP_NOTIFY_IND_DATA.
 *
 * \ingroup HAP_Client_Exported_Types
 */
typedef struct
{
    uint16_t conn_handle;
    bool notify;
    T_HAS_CP_NOTIFY_IND_DATA cp_data;
} T_HAS_CLIENT_CP_NOTIFY_IND_DATA;
/**
 * End of HAP_Client_Exported_Types
 * @}
 */

/**
 * \defgroup HAP_Client_Exported_Functions Hearing Aid Client Exported Functions
 *
 * \ingroup LEA_USE_CASE_HAP_Client
 * @{
 */

/**
 * has_client.h
 *
 * \brief  Initialize HAS client.
 *
 * \return         The result of initializing HAS client.
 * \retval true    Initializing HAS client is successful.
 * \retval false   Initializing HAS client failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_lea_profile_init(void)
    {
        ble_audio_cback_register(has_client_ts_handle_msg);
        has_client_init();
    }
 * \endcode
 *
 * \ingroup HAP_Client_Exported_Functions
 */
bool has_client_init(void);

/**
 * has_client.h
 *
 * \brief  Configure HAS CCCD.
 *
 * \param[in]  conn_handle    Connection handle.
 * \param[in]  cfg_flags      HAS CCCD configuration flags: @ref HAS_CFG_FLAGS.
 * \param[in]  enable         Whether to enable CCCD.
 * \arg    true    Enable CCCD.
 * \arg    false   Disable CCCD.
 *
 * \return  void.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        ble_audio_cback_register(has_client_ts_handle_msg);
    }

    uint16_t has_client_ts_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
    {
        uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

        switch (msg)
        {
        case LE_AUDIO_MSG_HAS_CLIENT_DIS_DONE:
            {
                T_HAS_CLIENT_DIS_DONE *p_data = (T_HAS_CLIENT_DIS_DONE *)buf;

                if (p_data->is_found)
                {
                    uint16_t cfg_flags = 0;

                    if (p_data->is_ha_features_notify_support)
                    {
                        cfg_flags |= HAS_HEARING_AID_FEATURE_FLAG;
                    }
                    if (p_data->is_ha_cp_exist)
                    {
                        cfg_flags |= HAS_HEARING_AID_PRESET_CP_FLAG;
                        cfg_flags |= HAS_ACTIVE_PRESET_IDX_FLAG;
                    }

                    has_cfg_cccd(p_data->conn_handle, cfg_flags, true);
                }
            }
            break;

        default:
            break;
        }
    }
 * \endcode
 *
 * \ingroup HAP_Client_Exported_Functions
 */
void has_cfg_cccd(uint16_t conn_handle, uint8_t cfg_flags, bool enable);

/**
 * has_client.h
 *
 * \brief  Read Hearing Aid Features value.
 *
 * \param[in]  conn_handle    Connection handle.
 *
 * \return         The result of reading Hearing Aid Features.
 * \retval true    Reading Hearing Aid Features is successful.
 * \retval false   Reading Hearing Aid Features failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(uint16_t conn_handle)
    {
        has_read_ha_features(conn_handle);
    }

    uint16_t has_client_ts_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
    {
        uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

        switch (msg)
        {
        case LE_AUDIO_MSG_HAS_CLIENT_READ_HA_FEATURES_RESULT:
            {
                T_HAS_CLIENT_READ_HA_FEATURES_RESULT *p_data = (T_HAS_CLIENT_READ_HA_FEATURES_RESULT *)buf;
                APP_PRINT_INFO3("LE_AUDIO_MSG_HAS_CLIENT_READ_HA_FEATURES_RESULT: Hearing Aid Features 0x%x, conn_handle 0x%x, cause 0x%x",
                                p_data->has_feature, p_data->conn_handle, p_data->cause);
            }
            break;

        default:
            break;
        }
    }
 * \endcode
 *
 * \ingroup HAP_Client_Exported_Functions
 */
bool has_read_ha_features(uint16_t conn_handle);

/**
 * has_client.h
 *
 * \brief  Read Active Preset Index.
 *
 * \param[in]  conn_handle     Connection handle.
 *
 * \return         The result of reading Active Preset Index.
 * \retval true    Reading Active Preset Index is successful.
 * \retval false   Reading Active Preset Index failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(uint16_t conn_handle)
    {
        has_read_active_preset_idx(conn_handle);
    }

    uint16_t has_client_ts_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
    {
        uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

        switch (msg)
        {
        case LE_AUDIO_MSG_HAS_CLIENT_READ_ACTIVE_PRESET_IDX_RESULT:
            {
                T_HAS_CLIENT_READ_ACTIVE_PRESET_IDX_RESULT *p_data = (T_HAS_CLIENT_READ_ACTIVE_PRESET_IDX_RESULT *)
                                                                     buf;
                APP_PRINT_INFO3("LE_AUDIO_MSG_HAS_CLIENT_READ_ACTIVE_PRESET_IDX_RESULT: conn_handle 0x%x, cause 0x%x, active preset idx 0x%x",
                                p_data->conn_handle,
                                p_data->cause,
                                p_data->active_preset_idx);
            }
            break;

        default:
            break;
        }
    }
 * \endcode
 *
 * \ingroup HAP_Client_Exported_Functions
 */
bool has_read_active_preset_idx(uint16_t conn_handle);

/**
 * has_client.h
 *
 * \brief  Write Read Preset Request control point.
 *
 * \param[in]  conn_handle        Connection handle.
 * \param[in]  start_preset_idx   The start preset index to read.
 * \param[in]  preset_num         The preset number to read.
 *
 * \return         The result of writing Read Preset Request control point.
 * \retval true    Writing Read Preset Request control point is successful.
 * \retval false   Writing Read Preset Request control point failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(uint16_t conn_handle, uint8_t start_preset_idx, uint8_t preset_num)
    {
        has_cp_read_presets(conn_handle, start_preset_idx, preset_num);
    }

    uint16_t has_client_ts_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
    {
        uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

        switch (msg)
        {
        case LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT:
            {
                T_HAS_CLIENT_CP_RESULT *p_data = (T_HAS_CLIENT_CP_RESULT *)buf;
                APP_PRINT_INFO2("LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT: conn_handle 0x%x, cause 0x%x",
                                p_data->conn_handle, p_data->cause);
            }
            break;

        case LE_AUDIO_MSG_HAS_CLIENT_CP_NOTIFY_IND_DATA:
            {
                T_HAS_CLIENT_CP_NOTIFY_IND_DATA *p_data = (T_HAS_CLIENT_CP_NOTIFY_IND_DATA *)buf;

                if (p_data->notify)
                {
                    APP_PRINT_INFO0("LE_AUDIO_MSG_HAS_CLIENT_CP_NOTIFY_IND_DATA: has sent cp notification");
                }
                else
                {
                    APP_PRINT_INFO0("LE_AUDIO_MSG_HAS_CLIENT_CP_NOTIFY_IND_DATA: has sent cp indication");
                }

                if (p_data->cp_data.cp_op == HAS_CP_OP_READ_PRESET_RSP)
                {
                    APP_PRINT_INFO4("HAS_CP_OP_READ_PRESET_RSP: is_last 0x%x, idx 0x%x, properties 0x%x, name %s",
                                    p_data->cp_data.is_last,
                                    p_data->cp_data.preset.index,
                                    p_data->cp_data.preset.properties,
                                    TRACE_STRING(p_data->cp_data.preset.p_name));
                }
            }
            break;

        default:
            break;
        }
    }
 * \endcode
 *
 * \ingroup HAP_Client_Exported_Functions
 */
bool has_cp_read_presets(uint16_t conn_handle, uint8_t start_preset_idx, uint8_t preset_num);

/**
 * has_client.h
 *
 * \brief  Send Write Preset Name control point.
 *
 * \param[in]  conn_handle     Connection handle.
 * \param[in]  preset_idx      Preset index.
 * \param[in]  name_len        Preset name length.
 * \param[in]  p_name          Pointer to preset name.
 *
 * \return         The result of send Write Preset Name control point.
 * \retval true    Sending Write Preset Name control point is successful.
 * \retval false   Sending Write Preset Name control point failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(uint16_t conn_handle, uint8_t preset_idx,
              uint8_t name_len, char *p_name)
    {
        has_cp_write_preset_name(conn_handle, preset_idx, name_len, p_name);
    }

    uint16_t has_client_ts_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
    {
        uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

        switch (msg)
        {
        case LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT:
            {
                T_HAS_CLIENT_CP_RESULT *p_data = (T_HAS_CLIENT_CP_RESULT *)buf;
                APP_PRINT_INFO2("LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT: conn_handle 0x%x, cause 0x%x",
                                p_data->conn_handle, p_data->cause);
            }
            break;

        case LE_AUDIO_MSG_HAS_CLIENT_CP_NOTIFY_IND_DATA:
            {
                T_HAS_CLIENT_CP_NOTIFY_IND_DATA *p_data = (T_HAS_CLIENT_CP_NOTIFY_IND_DATA *)buf;

                if (p_data->notify)
                {
                    APP_PRINT_INFO0("LE_AUDIO_MSG_HAS_CLIENT_CP_NOTIFY_IND_DATA: has send cp notification");
                }
                else
                {
                    APP_PRINT_INFO0("LE_AUDIO_MSG_HAS_CLIENT_CP_NOTIFY_IND_DATA: has send cp indication");
                }

                if (p_data->cp_data.cp_op == HAS_CP_OP_PRESET_CHANGED)
                {
                    switch (p_data->cp_data.change_id)
                    {
                    case GENERIC_UPDATE:
                        {
                            APP_PRINT_INFO6("HAS_CP_OP_PRESET_CHANGED: change_id 0x%x, is_last 0x%x, pre_idx 0x%x, idx 0x%x, properties 0x%x, name %s",
                                            p_data->cp_data.change_id,
                                            p_data->cp_data.is_last,
                                            p_data->cp_data.pre_idx,
                                            p_data->cp_data.preset.index,
                                            p_data->cp_data.preset.properties,
                                            TRACE_STRING(p_data->cp_data.preset.p_name));
                        }
                        break;

                    case PRESET_RECORD_DELETED:
                    case PRESET_RECORD_AVAILABLE:
                    case PRESET_RECORD_UNAVAILABLE:
                        {
                            APP_PRINT_INFO3("HAS_CP_OP_PRESET_CHANGED: change_id 0x%x, is_last 0x%x, idx 0x%x",
                                            p_data->cp_data.change_id,
                                            p_data->cp_data.is_last,
                                            p_data->cp_data.preset.index);
                        }
                        break;

                    default:
                        break;
                    }
                }
            }
            break;

        default:
            break;
        }
    }
 * \endcode
 *
 * \ingroup HAP_Client_Exported_Functions
 */
bool has_cp_write_preset_name(uint16_t conn_handle, uint8_t preset_idx,
                              uint8_t name_len, char *p_name);

/**
 * has_client.h
 *
 * \brief  Write Set Active Preset control point.
 *
 * \param[in]  conn_handle     Connection handle.
 * \param[in]  preset_idx      Preset index.
 * \param[in]  is_sync_local   Whether the opcode is sync locally.
 * \arg    true  : The control point opcode is @ref HAS_CP_OP_SET_ACTIVE_PRESET_SYNC_LOCAL.
 * \arg    false : The control point opcode is @ref HAS_CP_OP_SET_ACTIVE_PRESET.
 *
 * \return         The result of writing Set Active Preset control point.
 * \retval true    Writing Set Active Preset control point is successful.
 * \retval false   Writing Set Active Preset control point failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(uint16_t conn_handle, uint8_t preset_idx, bool is_sync_local)
    {
        has_cp_set_active_preset(conn_handle, preset_idx, is_sync_local);
    }

    uint16_t has_client_ts_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
    {
        uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

        switch (msg)
        {
        case LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT:
            {
                T_HAS_CLIENT_CP_RESULT *p_data = (T_HAS_CLIENT_CP_RESULT *)buf;
                APP_PRINT_INFO2("LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT: conn_handle 0x%x, cause 0x%x",
                                p_data->conn_handle, p_data->cause);
            }
            break;

        case LE_AUDIO_MSG_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY:
            {
                T_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY *p_data = (T_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY *)buf;
                APP_PRINT_INFO2("LE_AUDIO_MSG_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY: conn_handle 0x%x, active_preset_idx 0x%x",
                                p_data->conn_handle,
                                p_data->active_preset_idx);
            }
            break;

        default:
            break;
        }
    }
 * \endcode
 *
 * \ingroup HAP_Client_Exported_Functions
 */
bool has_cp_set_active_preset(uint16_t conn_handle, uint8_t preset_idx, bool is_sync_local);

/**
 * has_client.h
 *
 * \brief  Write Set Next Preset control point.
 *
 * \param[in]  conn_handle     Connection handle.
 * \param[in]  is_sync_local   Whether the opcode is sync locally.
 * \arg    true  : The control point opcode is @ref HAS_CP_OP_SET_NEXT_PRESET_SYNC_LOCAL.
 * \arg    false : The control point opcode is @ref HAS_CP_OP_SET_NEXT_PRESET.
 *
 * \return         The result of writing Set Next Preset control point.
 * \retval true    Writing Set Next Preset control point is successful.
 * \retval false   Writing Set Next Preset control point failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(uint16_t conn_handle, bool is_sync_local)
    {
        has_cp_set_next_preset(conn_handle, is_sync_local);
    }

    uint16_t has_client_ts_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
    {
        uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

        switch (msg)
        {
        case LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT:
            {
                T_HAS_CLIENT_CP_RESULT *p_data = (T_HAS_CLIENT_CP_RESULT *)buf;
                APP_PRINT_INFO2("LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT: conn_handle 0x%x, cause 0x%x",
                                p_data->conn_handle, p_data->cause);
            }
            break;

        case LE_AUDIO_MSG_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY:
            {
                T_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY *p_data = (T_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY *)buf;
                APP_PRINT_INFO2("LE_AUDIO_MSG_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY: conn_handle 0x%x, active_preset_idx 0x%x",
                                p_data->conn_handle,
                                p_data->active_preset_idx);
            }
            break;

default:
            break;
        }
    }
 * \endcode
 *
 * \ingroup HAP_Client_Exported_Functions
 */
bool has_cp_set_next_preset(uint16_t conn_handle, bool is_sync_local);

/**
 * has_client.h
 *
 * \brief  Write Set Previous Preset control point.
 *
 * \param[in]  conn_handle     Connection handle.
 * \param[in]  is_sync_local   Whether the opcode is synced locally.
 * \arg    true  : The control point opcode is @ref HAS_CP_OP_SET_PREVIOUS_PRESET_SYNC_LOCAL.
 * \arg    false : The control point opcode is @ref HAS_CP_OP_SET_PREVIOUS_PRESET.
 *
 * \return         The result of writing the Set Previous Preset control point.
 * \retval true    Writing the Set Previous Preset control point is successful.
 * \retval false   Writing the Set Previous Preset control point failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(uint16_t conn_handle, bool is_sync_local)
    {
        has_cp_set_previous_preset(conn_handle, is_sync_local);
    }

    uint16_t has_client_ts_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
    {
        uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

        switch (msg)
        {
        case LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT:
            {
                T_HAS_CLIENT_CP_RESULT *p_data = (T_HAS_CLIENT_CP_RESULT *)buf;
                APP_PRINT_INFO2("LE_AUDIO_MSG_HAS_CLIENT_CP_RESULT: conn_handle 0x%x, cause 0x%x",
                                p_data->conn_handle, p_data->cause);
            }
            break;

        case LE_AUDIO_MSG_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY:
            {
                T_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY *p_data = (T_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY *)buf;
                APP_PRINT_INFO2("LE_AUDIO_MSG_HAS_CLIENT_ACTIVE_PRESET_IDX_NOTIFY: conn_handle 0x%x, active_preset_idx 0x%x",
                                p_data->conn_handle,
                                p_data->active_preset_idx);
            }
            break;

        default:
            break;
        }
    }
 * \endcode
 *
 * \ingroup HAP_Client_Exported_Functions
 */
bool has_cp_set_previous_preset(uint16_t conn_handle, bool is_sync_local);
/**
 * End of HAP_Client_Exported_Functions
 * @}
 */

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
