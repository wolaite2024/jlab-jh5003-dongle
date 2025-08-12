#ifndef _APP_SRC_POLICY_H_
#define _APP_SRC_POLICY_H_
#include <stdbool.h>
#include <stdint.h>
#include "gap.h"
#include "audio_type.h"
#include "legacy_gaming.h"

#include "ring_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup APP_SRC_POLICY APP Src Policy
  * @brief APP policy about legacy gaming bt.
  * @{
  */


/* #define A2DP_STREAM_LATENCY_MONITOR */
/* #define SPP_STREAM_LATENCY_MONITOR */


#define SINK_DEV_NUM_MAX    2
#define CONFIG_FIX_BITPOOL

#ifdef CONFIG_FIX_BITPOOL
#define FIXED_BITPOOL   37
#endif

#define AVDTP_STREAM_DATA_FLUSH_TIMEOUT          (10)
#define AVDTP_STREAM_LONG_DATA_FLUSH_TIMEOUT     (30)

/* The maximum user payload of 2-DH5 is 679 */
/* The first sbc frame offset from beginning of l2cap pkt is 18.
 * If there is no content protection, the offset is 17
 */
#define SRC_2M_ACL_AVDTP_STREAM_OFFSET      (18)
#define SRC_2M_ACL_2DH3_PAYLOAD_SIZE    367
#define SRC_2M_ACL_2DH5_PAYLOAD_SIZE    679

typedef struct
{
    bool (*up_stream_data_write)(uint8_t *data, uint16_t length);
    uint16_t (*up_stream_get_data_len)(void);
} SRC_POLICY_UAC_CB_F;

/**
* app_src_policy.h
*
* \brief Define APP device state.
*/
typedef enum
{
    STATE_DEV_IDLE              = 0x00,
    STATE_DEV_RECONNECTING      = 0x01,
    STATE_DEV_CONNECTING        = 0x02,
    STATE_DEV_CLEANUP           = 0x03,
    STATE_DEV_CONNECTED         = 0x04,
    STATE_DEV_SHUTDOWN          = 0x05,
    STATE_DEV_OTA               = 0x06,
    STATE_DEV_MAX               = 0x07,
} T_APP_DEV_STATE;

/**
* app_src_policy.h
*
* \brief Define APP bt state.
*/
typedef enum
{
    STATE_BT_IDLE               = 0x00,
    STATE_BT_INQUIRY            = 0x01,
    STATE_BT_PAGING             = 0x02,
    STATE_BT_ACL_CONNECTED      = 0x03,
    STATE_BT_MAX                = 0x04,
} T_APP_BT_STATE;

/**
* app_src_policy.h
*
* \brief Remote device information.
*/
typedef struct
{
    uint8_t bd_addr[6];
    uint8_t gaming_bdaddr[6];
    uint32_t cod;
    uint8_t rtk_feature[8];
    uint8_t name[GAP_DEVICE_NAME_LEN];
} T_APP_SRC_REMOTE_INFO;

/**
* app_src_policy.h
*
* \brief A2DP stream information.
*/
typedef struct
{
    bool is_streaming;
    uint8_t codec_type;
    uint8_t *buffer;
    uint16_t buf_size;
    uint16_t sbc_frame_size;
    uint16_t sbc_max_frame_num;
    uint16_t sbc_offset;
    uint16_t mtu;
} T_APP_SRC_STREAM_INFO;

/**
* app_src_policy.h
*
* \brief SDP information.
*/
typedef struct
{
    uint32_t sdp_mask;
    bool is_found;
    bool is_busy;
    uint16_t protocol_version;
    uint8_t server_channel;
    uint8_t local_server_channel;
    uint8_t feature;
} T_APP_SRC_SDP_INFO;

/**
* app_src_policy.h
*
* \brief APP key event.
*/
typedef enum
{
    KEY_NONE        = 0x00,
    KEY_SHORT       = 0x01,
    KEY_LONG        = 0x02,
} T_APP_SRC_KEY_EVENT;

/**
* app_src_policy.h
*
* \brief APP device information.
*/

typedef enum
{
    TWS_HS_NORMAL       = 0x00,
    TWS_HS_LL           = 0x01,        /* low latency */
    STEREO_HS_NORMAL    = 0x02,
    STEREO_HS_LL        = 0x03,        /* low latency */
} T_APP_HEADSET_TYPE;

typedef struct
{
    uint16_t dev_state;
    uint16_t bt_state;
    bool connecting;

    bool inquiry_dev_found;

    bool sco_connected;

    T_APP_SRC_REMOTE_INFO remote_info;

    uint32_t profile_mask_plan;
    uint32_t profile_mask_remain;
    uint32_t profile_mask_doing;

    T_APP_SRC_SDP_INFO  sdp_info;

    bool codec_configured;
    T_AUDIO_FORMAT_INFO codec_info;

    T_APP_SRC_STREAM_INFO stream_info;

    T_APP_SRC_KEY_EVENT pending_event;

    uint8_t timer_idx_acl_exist_delay;

    uint8_t attempt;
    uint8_t pending_req;
    uint8_t mic_open_pending;
    uint8_t mic_open;

    bool headset_info_valid;
    uint8_t headset_info[4];

    uint32_t spp_sbc_frames;
    uint16_t sbcenc_packets;
    uint16_t stream_idle_count;
    uint32_t spp_audio_seqno;
    uint8_t a2dp_opened;
    uint8_t a2dp_stream_credits;
    uint16_t a2dp_ll_seq; /* Track the framework seq num */
    uint32_t a2dp_timestamp;
#ifdef A2DP_STREAM_LATENCY_MONITOR
    uint16_t a2dp_seq_synchronized;
    uint32_t a2dp_seq;
    uint32_t a2dp_anchor_seq;
    uint32_t a2dp_anchor_point; /* Timestamp at the first media packet. */
#endif
#ifdef SPP_STREAM_LATENCY_MONITOR
    uint16_t spp_seq_synchronized;
    uint32_t spp_seq;
    uint32_t spp_anchor_seq;
    uint32_t spp_anchor_point;
#endif
    T_APP_HEADSET_TYPE headset_type;
} T_APP_DEV;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_Src_Policy_Exported_Functions APP Src Policy Functions
  * @{
  */


/**
 * app_src_policy.h
 *
 * \brief   USB audio function related register.
 *
 * \param[in] p_func uac function related structure.
 *
 */
void app_src_policy_uac_register(SRC_POLICY_UAC_CB_F *p_func);

/**
 * app_src_policy.h
 *
 * \brief   USB audio function unregister.
 *
 */
void app_src_policy_uac_unregister(void);

/**
 * app_src_policy.h
 *
 * \brief   Calculate sbc frame size according to sbc parameters.
 *
 * \param[in] chann_mode    Sbc channel mode.
 * \param[in] blocks        Block length.
 * \param[in] subbands      Subbands.
 * \param[in] bitpool       Bitpool.
 *
 * \return The value of sbc frame size.
 * \retval frame_size
 */
uint8_t src_sbc_frame_size(T_AUDIO_SBC_CHANNEL_MODE chann_mode,
                           uint8_t blocks, uint8_t subbands,
                           uint8_t bitpool);

/**
 * app_src_policy.h
 *
 * \brief   Adjust stream codec.
 *
 * \param[in] bitpool           Bitpool.
 * \param[in] sbc_frame_num     Sbc frame number.
 * \param[in] sbc_frame_size    Sbc frame size.
 */
void src_adjust_stream_codec(uint8_t bitpool, uint8_t sbc_frame_num,
                             uint8_t sbc_frame_size);

/**
 * gaming_bt.h
 *
 * \brief   Find device index by address.
 *
 * \param[in] bd_addr    Remote BT device address.
 * \param[in] index      Link index(can be 0x00: link1, or 0x01: link2).
 *
 * \return The status of finding device index by address.
 * \retval true     Device index was found successfully.
 * \retval false    Device index was failed to find.
 */
bool src_policy_find_dev_index_by_addr(uint8_t *bd_addr, uint8_t *index);

/**
 * gaming_bt.h
 *
 * \brief   Check whether the profile is conected.
 *
 * \param[in] bd_addr       Remote BT device address.
 * \param[in] profile_mask  Profile mask.
 *
 * \return The status of finding device index by address.
 * \retval true     Device index was found successfully.
 * \retval false    Device index was failed to find.
 */
bool profile_is_connected(uint8_t *bd_addr, uint32_t profile_mask);

/**
 * gaming_bt.h
 *
 * \brief   Announce enter or leave low latency mode.
 *
 * \param[in] ll    Enter or leave low latency.(bool value)
 *
 * \return The status of announcing enter or leave low latency.
 * \retval 1     Announce enter or leave low latency mode successfully.
 * \retval 0     Announce enter or leave low latency mode failed.
 */
uint8_t src_announce_enter_or_leave_ll(bool ll);

/**
 * app_src_policy.h
 *
 * \brief   Get spp connected link number.
 *
 * \return The value of spp connected link number.
 * \retval Spp connected link number.
 */
uint8_t src_get_spp_connected_link_num(void);

bool all_src_a2dp_stream_stop(void);
void src_handle_headset_linkback_tpoll(void);

/**
* \brief   force to use 3M or 2M
*
* \param[in] addr       peer addr
* \param[in] enable_3M  enable 3M
*
* \return               true if it is ready
*/
void src_handle_pkt_type(uint8_t *addr, bool enable_3M);

/**
 * \brief   Check if spp audio prequeue ready and start to decode
 *
 * \return               true if it is ready
 */
bool src_spp_audio_prequeue_ready(void);

/**
 * \brief   Check if legacy gaming is ready
 *
 * \return               true if it is ready
 */
bool src_legacy_gaming_is_ready(void);

/**
 * \brief   Legacy gaming event callback function
 *
 * \param[in] event       legacy gaming event
 * \param[in] addr       peer addr
 *
 * \return               none
 */
void src_legacy_gaming_event_cback(T_LEGACY_GAMING_EVENT event, uint8_t *addr);

/**
 * \brief   Send a2dp stream suspend req.
 *
 * \return            true if it sends successfully
 */
bool all_src_a2dp_stream_stop(void);

extern bool rtp_voice_prequeue_ready;
extern bool rtp_voice_transmitting;
extern T_APP_DEV sink_dev_list[SINK_DEV_NUM_MAX];

/** @} */ /* End of group APP_Src_Policy_Exported_Functions */

/** End of APP_SRC_POLICY
* @}
*/

#ifdef __cplusplus
}
#endif

#endif
