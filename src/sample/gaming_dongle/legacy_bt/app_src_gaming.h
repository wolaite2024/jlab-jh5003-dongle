#ifndef _APP_SRC_GAMING_H_
#define _APP_SRC_GAMING_H_
#include <stdbool.h>
#include <stdint.h>
#include "gap.h"
#include "audio_type.h"
#include "app_src_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * app_src_gaming.h
 *
 * \brief some APIs for gaming mode(low latency).
 */

#if F_APP_GAMING_CONTROLLER_SUPPORT
#define LEGACY_GAMING_XFER_TYPE     LEGACY_GAMING_XFER_1
#else
#define LEGACY_GAMING_XFER_TYPE     LEGACY_GAMING_XFER_2
#endif

/**
 * app_src_gaming.h
 *
 * \brief   Analyze the spp voice sample rate according to the headset's EIR data.
 *
 * \param[in] index    Sink remote device link id.
 *
 * \return The sample rate information carried by the headset.
 */
uint32_t app_eir_parse_spp_voice_data(uint8_t index);

/**
 * app_src_gaming.h
 *
 * \brief   Check whether the Headset is enable multilink.
 *
 * \param[in] index    Sink remote device link id.
 *
 * \return Whether the headset is enable multilink.
 * \retval true     The headset is enable multilink.
 * \retval false    The headset is not enble multilink.
 */
bool app_eir_parse_is_support_multilink(uint8_t index);

/**
 * app_src_gaming.h
 *
 * \brief   Check whether the Headset is use LC3 codec.
 *
 * \param[in] index    Sink remote device link id.
 *
 * \return Whether the headset is use LC3 codec.
 * \retval true     The headset is use LC3 codec.
 * \retval false    The headset is not use LC3 codec.
 */
bool app_eir_parse_is_support_lc3_codec(uint8_t index);

/**
 * app_src_gaming.h
 *
 * \brief   Check whether dongle need to be slave.
 *
 * \param[in] index    Sink remote device link id.
 *
 * \return whether dongle need to be slave.
 * \retval true     Dongle slave(Paired with earphones audio mixing).
 * \retval false    Dongle master(General situation).
 */
bool app_eir_parse_dongle_need_to_be_slave(uint8_t index);

/**
 * app_src_gaming.h
 *
 * \brief   Parse and save the headset EIR data.
 *
 * \param[in] sink_dev   Sink device.
 * \param[in] eir_data   EIR data.
 * \param[in] eir_len    EIR data length.
 */
void src_eir_parse(T_APP_DEV *sink_dev, uint8_t *eir_data, uint8_t eir_len);

/**
 * app_src_gaming.h
 *
 * \brief   Check whether it is allowed to enter the gaming mode.
 *
 * \return The result of this function.
 * \retval true     Allow to enter gaming mode.
 * \retval false    Not allow to enter gaming mode.
 */
bool src_low_latency_allowed(void);

/**
 * app_src_gaming.h
 *
 * \brief   Determine frame number.
 *
 * \return  The frame number.
 */
uint8_t src_get_proper_frame_num2(void);

/**
 * app_src_gaming.h
 *
 * \brief   Enter gaming mode.
 *
 * \return The status of enter gaming mode.
 * \retval true     Enter gaming mode successfully.
 * \retval false    Enter gaming mode failed.
 */
bool src_enter_low_latency(uint8_t index, uint8_t custom_sbc_num,
                           uint8_t *rc_sbc_num);

/**
 * app_src_gaming.h
 *
 * \brief   Leave gaming mode.
 *
 * \param[in] bd_addr    Bluetooth device address.
 */
void src_leave_low_latency(uint8_t *bdaddr);

/**
 * app_src_gaming.h
 *
 * \brief   Headset request enter gaming mode.
 */
void src_spp_req_enter_ll(uint8_t *bdaddr, uint8_t custom_sbc_num);

/**
 * app_src_gaming.h
 *
 * \brief   Headset request leave gaming mode.
 */
void src_spp_req_leave_ll(uint8_t *bdaddr, uint8_t custom_sbc_num);

#ifdef __cplusplus
}
#endif

#endif
