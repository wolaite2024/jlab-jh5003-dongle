/*
 *  Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 */
#ifndef __GAMING_BT_H__
#define __GAMING_BT_H__

#include "btm.h"
#include "gap_callback_le.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup GAMING_BT Gaming BT
  * @brief Define status and APIs for Legacy Gaming bt.
  * @{
  */


/**
* gaming_bt.h
*
* \brief Define Gaming bt some configs. Currently, only UAC2 config is used.
*/
enum gaming_cfg_type
{
    GAMING_CFG_LOW_LATENCY = 0,     /**< Actively enter the gaming mode after connecting the Headset. */
    GAMING_CFG_AUTOPAIR_ENABLE,     /**< Enable automatic pairing after power on. */
    GAMING_CFG_AUTOPAIR_GENERAL,    /**< General headset can be connected during automatic pairing. */
    GAMING_CFG_AUTOPAIR_TIMEOUT,    /**< Configure the timeout for automatic pairing. */
    GAMING_CFG_AUTOPAIR_RSSI,       /**< Configure the RSSI for automatic pairing. */
    GAMING_CFG_UAC2,                /**< Enable dual uac mode. */
};

/**
* gaming_bt.h
*
* \brief Events reported during inquiry and connection.
*/
enum gaming_bt_event
{
    EVENT_GAMING_DEVICE_CONNECTED = 0,  /**< Device acl connect successfully. */
    EVENT_GAMING_DEVICE_DISCONNECTED,   /**< Device acl disconnect successfully. */
    EVENT_GAMING_PROFILE_CONNECTED,     /**< Device profiles connect successfully. */
    EVENT_GAMING_PROFILE_DISCONNECTED,  /**< Device profiles disconnect successfully. */
    EVENT_GAMING_DEVICE_FOUND,          /**< Device found. */
    EVENT_GAMING_DISCOVERY_STOPPED,     /**< Discovery stopped. */
    EVENT_GAMING_DISCOVERY_STARTING,    /**< Discovery starting. */
    EVENT_GAMING_DISCOVERY_STARTED,     /**< Discovery started. */
    EVENT_GAMING_DISCOVERY_STOPPING,    /**< Discovery stopping. */
    EVENT_GAMING_LED_INDICATOR,         /**< LED indicators in different states. */
};

/**
* gaming_bt.h
*
* \brief LED indicators in different states.
*/
enum gaming_led_state
{
    GAMING_LED_BT_IDLE,             /**< LED indicator in idle or standby state. */
    GAMING_LED_BT_CONNECTING,       /**< LED indicator in connecting state. */
    GAMING_LED_BT_RECONNECTING,     /**< LED indicator in reconnecting state. */
    GAMING_LED_BT_CONNECTED,        /**< LED indicator in connected state. */
};

/**
* gaming_bt.h
*
* \brief Legacy gaming bt event parameter device connected.
*/
struct gaming_ev_device_connected
{
    uint8_t id;         /**< Link id(can be 0x00: link1, or 0x01: link2.) Two links can be connected at most. */
    uint8_t reserved;   /**< Reserved. */
    uint8_t ba[6];      /**< Remote bt address. */
};

/**
* gaming_bt.h
*
* \brief Legacy gaming bt event parameter device disconnected.
*/
struct gaming_ev_device_disconnected
{
    uint8_t id;         /**< Link id(can be 0x00: link1, or 0x01: link2.) Two links can be connected at most. */
    uint8_t reserved;   /**< Reserved. */
    uint8_t ba[6];      /**< Remote bt address. */
};

/**
* gaming_bt.h
*
* \brief Legacy gaming bt event parameter profile connected.
*/
struct gaming_ev_profile_connected
{
    uint8_t id;         /**< Link id(can be 0x00: link1, or 0x01: link2.) Two links can be connected at most. */
    uint8_t reserved;   /**< Reserved. */
    uint8_t ba[6];      /**< Remote bt address. */
    uint32_t profile;   /**< See app_link_util.h */
    uint32_t profile_data;
};

/**
* gaming_bt.h
*
* \brief Legacy gaming bt event parameter profile disconnected.
*/
struct gaming_ev_profile_disconnected
{
    uint8_t id;         /**< Link id(can be 0x00: link1, or 0x01: link2.) Two links can be connected at most. */
    uint8_t reserved;   /**< Reserved. */
    uint8_t ba[6];      /**< Remote bt address. */
    uint32_t profile;   /**< See app_link_util.h */
    uint32_t profile_data;
};

/**
* gaming_bt.h
*
* \brief Legacy gaming bt event parameter inquiry result.
*/
struct gaming_ev_device_found
{
    T_BT_EVENT_PARAM_INQUIRY_RESULT result; /**< Inquiry result. */
};

/**
* gaming_bt.h
*
* \brief Legacy gaming bt event parameter led state.
*/
struct gaming_ev_led_state
{
    uint8_t id;         /**< Link id(can be 0x00: link1, or 0x01: link2.) Two links can be connected at most. */
    uint8_t led_state;  /**< Led state. */
};

typedef bool (*gaming_bt_cback_t)(uint8_t evt, uint8_t *data, uint16_t len);
typedef void (*spp_cmd_cback_t)(uint8_t *bdaddr, uint8_t *cmd, uint16_t len);

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Gaming_BT_Exported_Functions Gaming BT Functions
  * @{
  */

/**
 * gaming_bt.h
 *
 * \brief   Initialize gaming bt manager.
 *
 * <b>Example usage</b>
 * \code{.c}
    int gaming_init(void)
    {
        gaming_bt_init();
        ...
        return 0;
    }
 * \endcode
 */
int gaming_bt_init(void);

/**
 * gaming_bt.h
 *
 * \brief   Gaming bt set configuration.
 *
 * \param[in] type    Configuration type.
 * \param[in] value   Configuration value.
 * \param[in] len     Configuration length.
 *
 * <b>Example usage</b>
 * \code{.c}
    int legacy_audio_init(void *evt_queue, void *msg_queue)
    {
        uint8_t uac2 = 1;
        gaming_set_cfg(GAMING_CFG_UAC2, &uac2, sizeof(uac2));
        ...
        return 0;
    }
 * \endcode
 */
void gaming_set_cfg(uint8_t type, uint8_t *value, uint8_t len);

/**
 * gaming_bt.h
 *
 * \brief   Gaming bt enable.
 *
 * <b>Example usage</b>
 * \code{.c}
    int legacy_audio_enter(void)
    {
        gaming_bt_enable();
        ...
        return 0;
    }
 * \endcode
 */
int gaming_bt_enable(void);

/**
 * gaming_bt.h
 *
 * \brief   Gaming bt disable.
 *
 * <b>Example usage</b>
 * \code{.c}
    int legacy_audio_exit(void)
    {
        gaming_bt_disable();
        ...
        return 0;
    }
 * \endcode
 */
void gaming_bt_disable(void);

/**
 * gaming_bt.h
 *
 * \brief   Gaming bt manager event callback register.
 *
 * \param[in] cback    Callback function
 *
 * \return             The status of gaming bt manager event callback register.
 * \retval    0        Gaming bt manager event callback was registered successfully.
 * \retval    -1       Gaming bt manager event callback was failed to register.
 *
 * <b>Example usage</b>
 * \code{.c}
    int gaming_init(void)
    {
        gaming_bt_reg_cback(gaming_bt_cback);
        ...
        return 0;
    }
 * \endcode
 */
int gaming_bt_reg_cback(gaming_bt_cback_t cback);

/**
 * gaming_bt.h
 *
 * \brief   Connecting remote device.
 *
 * \param[in] id        Local BT link id(can be 0x00: link1, or 0x01: link2). Two links can be connected at most.
 * \param[in] bdaddr    Remote BT device address. If the address is 0, any devices found will be connected.
 *                      If the address is not 0, it will check whether the address has been bound. If yes, it
 *                      will reconnect this address. If not, the currently bound information will be deleted,
 *                      and start searching for this device and connect.
 *
 * \return The status of connecting remote device.
 * \retval 0            Connecting remote device successfully.
 * \retval others       Connecting remote device failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    int legacy_audio_mmi(uint8_t mmi_action, void *params)
    {
        switch (mmi_action)
        {
        case MMI_DONGLE_LINK1_KEY_LONG_PRESS:
            {
                //KEY_MMI execution: link index 0 will inquiry and connect any qualified devices.
                uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };
                gaming_bt_connect(0, ba_any);
            }
            break;
        case MMI_DONGLE_LINK2_KEY_LONG_PRESS:
            {
                //KEY_MMI execution: link index 1 will first check whether the address has been bound, if yes will
                reconnect it. If not will delete bond info and start searching for this device and connect.
                uint8_t ba_addr[6] = { 01, 02, 03, 04, 05, 06 };
                gaming_bt_connect(1, ba_any);
            }
            break;
        }
    }
 * \endcode
 */
int gaming_bt_connect(uint8_t id, uint8_t *bdaddr);

/**
 * gaming_bt.h
 *
 * \brief   Disconnect the device by address.
 *
 * \param[in] bdaddr    Remote BT device address.
 *
 * \return The status of disconnecting remote device.
 * \retval 0            Disconnecting remote device successfully.
 * \retval others       Disconnecting remote device failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    uint8_t app_usb_hid_handle_gaming_cmd(uint8_t *p_data, uint16_t len)
    {
        ...
        switch (opcode)
        {
        //Executed by usb command
        case GAMING_BT_DISCONNECT_BY_ADDRESS_OPCODE:
            {
                uint8_t bd_addr[6];
                uint8_t index;
                for (uint8_t i = 0; i < 6; i++)
                {
                    bd_addr[5 - i] = *pp++;
                }
                if (app_src_legacy_get_paired_idx(bd_addr, &index) == false)
                {
                    status = GAMING_INVALID_COMMAND_PARAM;
                    legacy_gaming_cmd_complete_event(opcode, status);
                    break;
                }
                legacy_gaming_cmd_complete_event(opcode, status);
                gaming_bt_disconnect_by_bdaddr(bd_addr);
            }
            break;
        }
    }
 * \endcode
 */
int gaming_bt_disconnect_by_bdaddr(uint8_t *bdaddr);

/**
 * gaming_bt.h
 *
 * \brief   Disconnect the device by link id.
 *
 * \param[in] id        Local BT link id(can be 0x00: link1, or 0x01: link2). Two links can be connected at most.
 *
 * \return The status of disconnecting remote device.
 * \retval 0            Disconnecting remote device successfully.
 * \retval others       Disconnecting remote device failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    int legacy_audio_mmi(uint8_t mmi_action, void *params)
    {
        switch (mmi_action)
        {
        case MMI_DONGLE_LINK1_KEY_SHORT_PRESS:
            {
                //KEY_MMI execution: disconnect index 0 link.
                if (!gaming_bt_is_idle(0))
                {
                    gaming_bt_disconnect_by_id(0);
                    return 0;
                }
            }
            break;
        }
    }
 * \endcode
 */
int gaming_bt_disconnect_by_id(uint8_t id);

/**
 * gaming_bt.h
 *
 * \brief   Remove bond information by address.
 *
 * \param[in] bdaddr    Remote BT device address.
 *
 * <b>Example usage</b>
 * \code{.c}
    uint8_t app_usb_hid_handle_gaming_cmd(uint8_t *p_data, uint16_t len)
    {
        ...
        switch (opcode)
        {
        //Executed by usb command
        case GAMING_BT_REMOVE_BOND_BY_ADDRESS_OPCODE:
            {
                for (uint8_t i = 0; i < 6; i++)
                {
                    bd_addr[5 - i] = *pp++;
                }
                if (app_src_legacy_get_paired_idx(bd_addr, &index) == false)
                {
                    status = GAMING_INVALID_COMMAND_PARAM;
                    legacy_gaming_cmd_complete_event(opcode, status);
                    break;
                }
                legacy_gaming_cmd_complete_event(opcode, status);
                gaming_bt_remove_bond_by_bdaddr(bd_addr);
            }
            break;
        }
    }
 * \endcode
 */
void gaming_bt_remove_bond_by_bdaddr(uint8_t *bdaddr);

/**
 * gaming_bt.h
 *
 * \brief   Remove bond information by link id.
 *
 * \param[in] id        Local BT link id(can be 0x00: link1, or 0x01: link2). Two links can be connected at most.
 *
 * <b>Example usage</b>
 * \code{.c}
    uint8_t app_usb_hid_handle_gaming_cmd(uint8_t *p_data, uint16_t len)
    {
        ...
        switch (opcode)
        {
        //Executed by usb command
        case GAMING_BT_REMOVE_BOND_BY_ID_OPCODE:
            {
                if ((*pp != 0) && (*pp != 1))
                {
                    status = GAMING_INVALID_COMMAND_PARAM;
                    legacy_gaming_cmd_complete_event(opcode, status);
                    break;
                }
                legacy_gaming_cmd_complete_event(opcode, status);
                gaming_bt_remove_bond_by_id(*pp);
            }
            break;
        }
    }
 * \endcode
 */
void gaming_bt_remove_bond_by_id(uint8_t id);

/**
 * gaming_bt.h
 *
 * \brief   Gaming bt start discovery.
 *
 * \return The status of starting discovery.
 * \retval 0            Start discovery successfully.
 * \retval others       Start discovery failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    uint8_t app_usb_hid_handle_gaming_cmd(uint8_t *p_data, uint16_t len)
    {
        ...
        switch (opcode)
        {
        //Executed by usb command.
        //You can call this function where you want it.
        case GAMING_BT_START_DISCOVERY_OPCODE:
            {
                legacy_gaming_cmd_complete_event(opcode, status);
                gaming_bt_start_discovery();
            }
            break;
        }
    }
 * \endcode
 */
int gaming_bt_start_discovery(void);

/**
 * gaming_bt.h
 *
 * \brief   Gaming bt stop discovery.
 *
 * \return The status of stopping discovery.
 * \retval 0            Stop discovery successfully.
 * \retval others       Stop discovery failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    uint8_t app_usb_hid_handle_gaming_cmd(uint8_t *p_data, uint16_t len)
    {
        ...
        switch (opcode)
        {
        //Executed by usb command.
        //Usually, this function is called after the function gaming_bt_start_discovery().
        case GAMING_BT_STOP_DISCOVERY_OPCODE:
            {
                legacy_gaming_cmd_complete_event(opcode, status);
                gaming_bt_stop_discovery();
            }
            break;
        }
    }
 * \endcode
 */
int gaming_bt_stop_discovery(void);

/**
 * gaming_bt.h
 *
 * \brief   Get bond device address.
 *
 * \param[in] id        Local BT link id(can be 0x00: link1, or 0x01: link2). Two links can be connected at most.
 * \param[in] bdaddr    Remote BT device address.
 *
 * \return The status of geting bond device address.
 * \retval 0            Geting bond device address successfully.
 * \retval others       Geting bond device address failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    int app_test_sample_code(void)
    {
        //Usually used to check bond address.
        uint8_t bdaddr[6];
        if (gaming_bt_get_bond_bdaddr(0, bdaddr) < 0)
        {
            APP_PRINT_WARN0("app_test_sample_code: No bond for dev 0");
            return -2;
        }
    }
 * \endcode
 */
int gaming_bt_get_bond_bdaddr(uint8_t id, uint8_t *bdaddr);

/**
 * gaming_bt.h
 *
 * \brief   Check whether it is connected.
 *
 * \return The status of whether it is connected.
 * \retval true     It is connected.
 * \retval false    It is not connected.
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_test_sample_code(void)
    {
        //Returns true whenever there is a connection.
        if (gaming_bt_is_connected())
        {
            gaming_bt_disconnect_by_id(0);
        }
    }
 * \endcode
 */
bool gaming_bt_is_connected(void);

/**
 * gaming_bt.h
 *
 * \brief   Profile connected complete event.
 *
 * \param[in] bdaddr        Remote BT device address.
 * \param[in] profile_mask  Profile mask.
 *
 * <b>Example usage</b>
 * \code{.c}
    static void app_spp_audio_cback(uint8_t *bd_addr, T_BT_RFC_MSG_TYPE msg_type, void *msg_buf)
    {
        switch (msg_type)
        {
        case BT_RFC_MSG_CONN_CMPL:
        {
            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                gaming_bt_profile_conn_cmpl(bd_addr, SPP_AUDIO_PROFILE_MASK);
            }
        }
        break;
        }
    }
 * \endcode
 */
void gaming_bt_profile_conn_cmpl(uint8_t *bd_addr, uint32_t profile_mask);

/**
 * gaming_bt.h
 *
 * \brief   Profile disconnected complete event.
 *
 * \param[in] bdaddr        Remote BT device address.
 * \param[in] cause         Profile disconnect cause.
 * \param[in] profile_mask  Profile mask.
 *
 * <b>Example usage</b>
 * \code{.c}
    static void app_spp_audio_cback(uint8_t *bd_addr, T_BT_RFC_MSG_TYPE msg_type, void *msg_buf)
    {
        switch (msg_type)
        {
        case BT_RFC_MSG_DISCONN_CMPL:
        {
            p_link = app_find_br_link(bd_addr);
            if (p_link != NULL)
            {
                gaming_bt_profile_disconn_cmpl(bd_addr, p_info->cause, SPP_AUDIO_PROFILE_MASK);
            }
        }
        break;
        }
    }
 * \endcode
 */
void gaming_bt_profile_disconn_cmpl(uint8_t *bd_addr, uint16_t cause,
                                    uint32_t profile_mask);

/**
 * gaming_bt.h
 *
 * \brief   Try to send a2dp stream start request.
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_usb_uac_handle_msg(T_IO_MSG *msg)
    {
        uint8_t subtype = msg->subtype & 0xff;
        switch (subtype)
        {
        case MSG_TYPE_UAC_DATA_TRANS_DS:
            {
                gaming_bt_try_start_stream();
            }
            break;
        }
    }
 * \endcode
 */
void gaming_bt_try_start_stream(void);

/**
 * gaming_bt.h
 *
 * \brief   Set bt avrcp absolute volume.
 *
 * \param[in] vol       Avrcp volume(range: 0x00 ~ 0x7F).
 * \param[in] mute      Mute or not(1: mute, 0: unmute).
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_dongle_set_avrcp_vol(void)
    {
        if (g_dongle_vol.host_state == DONGLE_HOST_STATE_CHECKED)
        {
            if ((g_dongle_vol.host_type == DONGLE_HOST_HID_INVALID)
            {
                gaming_bt_set_volume(AVRCP_VOL_MAX, PC_VOL_UNMUTE);
            }
            else
            {
                gaming_bt_set_volume(get_dongle_host_vol(), get_dongle_host_mute());
            }
        }
    }
 * \endcode
 */
void gaming_bt_set_volume(uint32_t vol, uint8_t mute);

/**
 * gaming_bt.h
 *
 * \brief   Uac up stream state capture.
 *
 * \param[in] active    Up stream active or inactive.
 *
 * <b>Example usage</b>
 * \code{.c}
    void dongle_uac_cback_chnl_state(uint8_t chnl)
    {
        if ((chnl & UAC_US_ACTIVE) != (dongle_chnl_state & UAC_US_ACTIVE))
        {
            if (chnl & UAC_US_ACTIVE)
            {
                gaming_bt_audio_capture_switch(true);
            }
            else
            {
                gaming_bt_audio_capture_switch(false);
            }
        }
    }
 * \endcode
 */
void gaming_bt_audio_capture_switch(bool active);

/**
 * gaming_bt.h
 *
 * \brief   Check whether it is in idle state.
 *
 * \param[in] id    Local BT link id(can be 0x00: link1, or 0x01: link2). Two links can be connected at most.
 *
 * \return The status of whether it is in idle state.
 * \retval true     It is in idle state.
 * \retval false    It is not in idle state.
 *
 * <b>Example usage</b>
 * \code{.c}
    int legacy_audio_mmi(uint8_t mmi_action, void *params)
    {
        switch (mmi_action)
        {
        case MMI_DONGLE_LINK1_KEY_SHORT_PRESS:
            {
                //Check bt state.
                if (!gaming_bt_is_idle(0))
                {
                    gaming_bt_disconnect_by_id(0);
                    return 0;
                }
            }
            break;
        }
    }
 * \endcode
 */
bool gaming_bt_is_idle(uint8_t id);

/**
 * gaming_bt.h
 *
 * \brief   Check whether a2dp is streaming.
 *
 * \return The status of whether a2dp is streaming.
 * \retval true     It is streaming.
 * \retval false    It is not streaming.
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_test_sample_code(void)
    {
        ...
        //check streaming state
        if (!gaming_bt_a2dp_is_streaming())
        {
            ...
        }
    }
 * \endcode
 */
bool gaming_bt_a2dp_is_streaming(void);

/**
 * gaming_bt.h
 *
 * \brief   Set bt mode.
 *
 * \param[in] mode    T_BT_DEVICE_MODE mode(see btm.h).
 *
 * <b>Example usage</b>
 * \code{.c}
    int legacy_audio_enter(void)
    {
        ...
        //Set bt mode discoverable and connectable,after power on, it can be reconn by headset
        //or can be connected by testing instrument.
        gaming_set_bt_mode(BT_DEVICE_MODE_DISCOVERABLE_CONNECTABLE);
        return 0;
    }
 * \endcode
 */
void gaming_set_bt_mode(T_BT_DEVICE_MODE mode);

/**
 * gaming_bt.h
 *
 * \brief   Check whether a2dp uses lc3 codec.
 *
 * \return The result of whether a2dp uses lc3 codec.
 * \retval true     A2dp uses lc3 codec.
 * \retval false    A2dp does not use lc3 codec.
 *
 * <b>Example usage</b>
 * \code{.c}
    static bool app_usb_uac_set_gain(uint8_t it, uint8_t level)
    {
        switch (it)
        {
        case IT_UDEV_IN1:
            //check current codec whether is lc3 codec.
            if (gaming_a2dp_codec_is_lc3())
            {
                rc = app_audio_path_set_gain(IT_UDEV_IN1, OT_LC3FRM, gain);
            }
            else
            {
                rc = app_audio_path_set_gain(IT_UDEV_IN1, OT_SBC, gain);
            }
            return rc;
        }
    }
 * \endcode
 */
bool gaming_a2dp_codec_is_lc3(void);

/**
 * gaming_bt.h
 *
 * \brief   Check whether there is bond record with id saved.
 *
 * \return The result of whether there is bond record with id saved.
 * \retval true     Yes. Dongle has a bud link key record and last 3 byte of bud bdaddr are the same as
 *                       the saved id.
 * \retval false    No. There is not bond record with id saved.
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_test_sample_code(void)
    {
        if (gaming_bt_have_saved_id_bond_record())
        {
            //This function is used when enabling dual mode(2.4g mode),
              In this mode, the last three digits of the headphone address are saved.
            ...
        }
    }
 * \endcode
 */
bool gaming_bt_have_saved_id_bond_record(void);

/**
 * gaming_bt.h
 *
 * \brief   Set scan by mmi flag.
 *
 * \param[in] flag    yes or not sacn by mmi.
 *
 * <b>Example usage</b>
 * \code{.c}
    int legacy_audio_mmi(uint8_t mmi_action, void *params)
    {
        switch (mmi_action)
        {
        case MMI_DONGLE_LINK1_KEY_LONG_PRESS:
            {
                uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };

                if (app_cfg_const.enable_dongle_dual_mode)
                {
                    //Customized function, long press the key to pair,and the flag will set to true.
                    gaming_bt_set_scan_by_mmi(true);
                    ...
                }
            }
            break;
        }
    }
 * \endcode
 */
void gaming_bt_set_scan_by_mmi(bool flag);

/**
 * gaming_bt.h
 *
 * \brief   Get scan by mmi flag.
 *
 * \return The result of geting sacn by mmi flag.
 * \retval true     Scan is by mmi.
 * \retval false    Scan is not by mmi.
 *
 * <b>Example usage</b>
 * \code{.c}
    int legacy_audio_mmi(uint8_t mmi_action, void *params)
    {
        switch (mmi_action)
        {
        case MMI_DONGLE_LINK1_KEY_SHORT_PRESS:
            {
                if (app_cfg_const.enable_dongle_dual_mode)
                {
                    //Customized function, get scan by mmi flag.
                    if (gaming_bt_get_scan_by_mmi() == true)
                    {
                        gaming_bt_set_scan_by_mmi(false);
                        ...
                    }
                }
            }
            break;
        }
    }
 * \endcode
 */
bool gaming_bt_get_scan_by_mmi(void);

/**
 * gaming_bt.h
 *
 * \brief   Handle le extended advertising report information.
 *
 * \param[in] cb_type    Callback type.
 * \param[in] result     Advertising report information.
 *
 * <b>Example usage</b>
 * \code{.c}
    void src_start_le_scan(void)
    {
        scan_id = bt_adap_start_discovery(DISCOVERY_TYPE_LE, GAP_SCAN_FILTER_ANY,
                                          gaming_handle_ext_adv_report_info);

    }
 * \endcode
 */
void gaming_handle_ext_adv_report_info(uint8_t cb_type, void *result);

/**
 * gaming_bt.h
 *
 * \brief   Check whether it is in low latency mode and is streaming.
 *
 * \return The result of whether it is in low latency mode and is streaming.
 * \retval true     It is in low latency mode and is streaming.
 * \retval false    It is not in low latency mode or is not streaming.
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_test_sample_code(void)
    {
        if (gaming_bt_low_latency_streaming())
        {
            //send ctrl data via a2dp
            ...
        }
    }
 * \endcode
 */
bool gaming_bt_low_latency_streaming(void);

/**
 * gaming_bt.h
 *
 * \brief   Send spp command.
 *
 * \param[in] index    Link index(can be 0x00: link1, or 0x01: link2). Two links can be connected at most.
 * \param[in] cmd      Spp command.
 * \param[in] data     Spp payload.
 * \param[in] len      Spp payload length.
 *
 * \return The state of sending spp command.
 * \retval true     Spp command was set successfully.
 * \retval false    Spp command was failed to set.
 *
 * <b>Example usage</b>
 * \code{.c}
    static void send_headset_ctrl_data_by_spp_cmd(uint8_t *data, uint16_t size)
    {
        app_spp_cmd_send(0, DONGLE_SPP_CMD_CTRL_RAW_DATA, data, size);
    }
 * \endcode
 */
bool app_spp_cmd_send(uint8_t index, uint8_t cmd, uint8_t *data, uint16_t len);

/**
 * gaming_bt.h
 *
 * \brief   Spp command receive callback register.
 *
 * \param[in] cback    Callback function
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_spp_cmd_init(void)
    {
        ...
        register_rcv_ctrl_pkt_cback(app_ctrl_pkt_rcv_cback);
        spp_cmd_register_recv_cback(app_spp_cmd_received);
    }
 * \endcode
 */
void spp_cmd_register_recv_cback(spp_cmd_cback_t cback);

/**
 * gaming_bt.h
 *
 * \brief   Connecting any discovered remote device.
 *
 * \param[in] id       Local BT link id(can be 0x00: link1, or 0x01: link2). Two links can be connected at most.
 * \param[in] bdaddr   Remote BT device address.(Executed only when the address is 0.)
 *
 * \return The state of connecting any discovered remote device.
 * \retval true     Connecting any discovered remote device successfully.
 * \retval false    Connecting any discovered remote device failed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_test_sample_code(void)
    {
        if (app_cfg_const.enable_dongle_dual_mode && force_enter_pairing)
        {
            //Customized function. Executed only when the address is 0.
            uint8_t ba_any[6] = { 0, 0, 0, 0, 0, 0 };
            gaming_bt_force_connect(0, ba_any);
            force_enter_pairing = false;
        }
    }
 * \endcode
 */
int gaming_bt_force_connect(uint8_t id, uint8_t *bdaddr);

/**
 * gaming_bt.h
 *
 * \brief   Get a2dp packet sequence number.
 *
 * \param[in] index    Local BT link index(can be 0x00: link1, or 0x01: link2). Two links can be connected at most.
 *
 * \return The sequence number of the a2dp packet.
 * \retval sequence_number
 *
 * <b>Example usage</b>
 * \code{.c}
    void app_test_sample_code(void)
    {
        uint16_t sdu_seq_num;
        sdu_seq_num = gaming_sync_app_seq_number(0);
        ...
    }
 * \endcode
 */
uint16_t gaming_sync_app_seq_number(uint8_t index);

/**
 * gaming_bt.h
 *
 * \brief   Check whether the current status is A2DP force suspend, in this state,drop downstream data.
 *
 * \return The result of whether the current status is A2DP force suspend.
 * \retval true     The current status is A2DP force suspend.
 * \retval false    The current status is not A2DP force suspend.
 */
bool gaming_a2dp_is_force_suspend(void);

/** @} */ /* End of group Gaming_BT_Exported_Functions */

/** End of Gaming_BT
* @}
*/

#ifdef __cplusplus
}
#endif

#endif /* __GAMING_BT_H__ */
