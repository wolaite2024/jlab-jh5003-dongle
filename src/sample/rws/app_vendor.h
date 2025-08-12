#ifndef _APP_VENDOR_H_
#define _APP_VENDOR_H_

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT || F_APP_B2B_HTPOLL_CONTROL
typedef enum
{
    B2S_HTPOLL_EVENT_ACL_DISC                = 0x00,
    B2S_HTPOLL_EVENT_ACL_CONN                = 0x01,
    B2S_HTPOLL_EVENT_A2DP_START              = 0x02,
    B2S_HTPOLL_EVENT_A2DP_STOP               = 0x03,
    B2S_HTPOLL_EVENT_A2DP_DISC               = 0x04,
    B2S_HTPOLL_EVENT_LINKBACK_START          = 0x05,
    B2S_HTPOLL_EVENT_LINKBACK_STOP           = 0x06,
    B2S_HTPOLL_EVENT_SCO_CONNECTED           = 0x07,
    B2S_HTPOLL_EVENT_SCO_DISCONNECTED        = 0x08,
    B2S_HTPOLL_EVENT_ACTIVE_A2DP_IDX_CHANGED = 0x09,
    B2S_HTPOLL_EVENT_BLE_CONNECTED           = 0x0a,
    B2S_HTPOLL_EVENT_BLE_DISCONNECTED        = 0x0b,
    B2S_HTPOLL_EVENT_COMMON_ADV_ENABLE       = 0x0c,
    B2S_HTPOLL_EVENT_COMMON_ADV_DISABLE      = 0x0d,
    B2S_HTPOLL_EVENT_GAMING_MODE_SWITCH      = 0x0e,
    B2S_HTPOLL_EVENT_DONGLE_MIC_STATUS_CHANGED = 0x0f,
    B2S_HTPOLL_EVENT_LEA_SET_PREFER_QOS      = 0x10,
    B2S_HTPOLL_EVENT_LEA_SET_QOS_TIMEOUT     = 0x11,

    B2B_HTPOLL_EVENT_B2B_CONNECTED           = 0x20,
    B2B_HTPOLL_EVENT_B2B_DISCONNECTED        = 0x21,
    B2B_HTPOLL_EVENT_CIS_CONNECTED           = 0x22,
    B2B_HTPOLL_EVENT_CIS_DISCONNECTED        = 0x23,
    B2B_HTPOLL_EVENT_REMOTE_CIS_STATUS       = 0x24,
    B2B_HTPOLL_EVENT_REMOTE_BIS_STATUS       = 0x25,
    B2B_HTPOLL_EVENT_BIS_CONNECTED           = 0x26,
    B2B_HTPOLL_EVENT_BIS_DISCONNECTED        = 0x27,
    B2B_HTPOLL_EVENT_B2S_CONNECTED           = 0x28,
    B2B_HTPOLL_EVENT_B2S_DISCONNECTED        = 0x29,
    B2B_HTPOLL_EVENT_ROLESWAP_CMPL           = 0x2A,
} T_APP_HTPOLL_EVENT;

#if F_APP_GAMING_B2S_HTPOLL_SUPPORT
/**
 * @brief Get latency plus value when htpoll enable
 *
 * @param void
 * @return latency
 */
uint8_t app_vendor_get_htpoll_latency_plus(void);
#endif

/**@brief     Handle htpoll control for b2b & b2s htpoll
 *
 * @param[in] event event check for b2b & b2s htpoll.
 * @return    void.
 */
void app_vendor_htpoll_control(T_APP_HTPOLL_EVENT event);

#if F_APP_B2B_HTPOLL_CONTROL
/**@brief     Check if htpoll execing
 *
 * @param[in] void
 * @return    true if execing
 */
bool app_vendor_htpoll_execing(void);
#endif
#endif

/**@brief     init vendor process
 *
 * @param     void
 * @return    void.
 */
void app_vendor_init(void);

/**
 * @brief Set Advertising Extended Misc Command
 *
 * @param fix_channel  Aux ADV Fix Channel:
 * 0 Extended Advertising auxiliary packet channel unfixed.
 * 1 Extended Advertising auxiliary packet channel fixed to 1 (2406MHz).
 * @param offset  Aux ADV offset
 * 0~0xFF (Unit: slot)
 * The minimum value of the offset from a Extended Advertising packet to its auxiliary packet
 * The actual offset value might larger than Aux ADV offset because of collision with other protocol.
 * @return true  success
 * @return false fail
 */
bool app_vendor_set_adv_extend_misc(uint8_t fix_channel, uint8_t offset);

/**
 * @brief Set Afh Policy Priority Command
 *
 * @param lea_conn_handle(2 byte)  CIS Connect Handle
 * @param afh policy priority (1 byte)
 * remote first = 0, local first = 1
 * @return true  success
 * @return false fail
 */
bool app_vendor_send_psd_policy(uint16_t lea_conn_handle);

/**
 * @brief RF XTAL Auto K Command
 *
 * @param channel(1 byte)
 * @param upperbound (1 byte)
 * @param lowerbound (1 byte)
 * @param offset (1 byte)
 * @return void
 */
void app_vendor_rf_xtak_k(uint8_t channel, uint8_t upperbound, uint8_t lowerbound, uint8_t offset);

/**
 * @brief Get RF XTAL K result Command
 *
 * @param void
 * @return void
 */
void app_vendor_get_xtak_k_result(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _APP_VENDOR_H_ */
