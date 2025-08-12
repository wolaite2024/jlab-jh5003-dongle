#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "app_src_policy.h"
#include "app_src_gaming.h"
#include "gap_vendor.h"
#include "gaming_bt.h"
#include "app_link_util.h"
#include "app_cfg.h"

#define SRC_MAX_SBC_FRAME_NUM       7
#define SRC_TWS_SBC_FRAME_NUM       4
#define SRC_STEREO_SBC_FRAME_NUM    2

extern uint8_t low_latency_mode;


/* The headset EIR data will be saved in headset_info[4].
 * headset_info[0] - headset type.
 *      bit0: 0- stereo headset 1- TWS headset.
 *      bit1: 1- support LowLatency with Gaming Dongle, 0- not support.
 *
 * headset_info[1] - sbc_frame_num.
 *      bit3~0: 0 - sbc frame nums in each avdtp packet depend on dongle side.
 *
 * headset_info[2] - headset feature.
 *      bit1~0: Set SPP Voice Sample Rate. 0x00-16K(default), 0x01-32K, others rsv.
 *      bit  2: Set Multilink feature bit. 0 - Not support Multilink, 1 - support.
 *      bit  3: Set A2DP LC3 Codec. 0 - Not support, 1 - Support.
 *      bit  4: Set Dongle role.  0 - dongle master, 1 - dongle slave.
 */

static bool app_eir_parse_is_support_lowlatency(uint8_t index)
{
    if (sink_dev_list[index].headset_info[0] & 0x02)
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint32_t app_eir_parse_spp_voice_data(uint8_t index)
{
    uint32_t spp_sample_rate;
    if ((sink_dev_list[index].headset_info[2] & 0x03) == 0x01)
    {
        spp_sample_rate = 32000;
    }
    else
    {
        spp_sample_rate = 16000;
    }
    return spp_sample_rate;
}

bool app_eir_parse_is_support_multilink(uint8_t index)
{
    if (sink_dev_list[index].headset_info[2] & 0x04)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool app_eir_parse_is_support_lc3_codec(uint8_t index)
{
    if (sink_dev_list[index].headset_info[2] & 0x08)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool app_eir_parse_dongle_need_to_be_slave(uint8_t index)
{
    if (sink_dev_list[index].headset_info[2] & 0x10)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void src_headset_type_parse(T_APP_DEV *sink_dev, uint8_t headset_type_byte)
{
    T_APP_HEADSET_TYPE type;

    if (headset_type_byte & 0x01)
    {
        if (headset_type_byte & 0x02)
        {
            type = TWS_HS_LL;
        }
        else
        {
            type = TWS_HS_NORMAL;
        }
    }
    else
    {
        if (headset_type_byte & 0x02)
        {
            type = STEREO_HS_LL;
        }
        else
        {
            type = STEREO_HS_NORMAL;
        }
    }

    APP_PRINT_TRACE1("src_headset_type_parse: type %d", type);

    sink_dev->headset_type = type;
}

void src_eir_parse(T_APP_DEV *sink_dev, uint8_t *eir_data, uint8_t eir_len)
{
    uint8_t *p = eir_data;
    uint8_t *end = NULL;
    uint8_t field_len;
    /* uint8_t type; */

    if (!eir_data || !eir_len)
    {
        return;
    }

    end = eir_data + eir_len;
    /* L + T + Data, L + T + Data, ... */
    for (; p < end && *p; p += (1 + *p))
    {
        /* Data exceeds */
        if (p + (1 + *p) > end)
        {
            break;
        }
        field_len = *p;
        /* type = *(p + 1); */

        /* APP_PRINT_INFO2("src_eir_parse(): len %u, type %02x", field_len, type); */
        /* Manufacture-specific data */
        if (*(p + 1) == 0xFF && field_len > 5)
        {
            uint16_t company_id = (uint16_t)p[3] << 8 | p[2];
            uint8_t cmd = p[4];

            if (cmd == 0x08 && field_len > 5)
            {
                src_headset_type_parse(sink_dev, p[5]);
            }

            if (company_id == 0x005d && cmd == 0x08 && field_len > 5)
            {
                if (p[5] & 0x01)
                {
                    APP_PRINT_INFO0("src_eir_parse(): Found Realtek TWS HS");
                }
                else
                {
                    APP_PRINT_INFO0("src_eir_parse(): Found Realtek Stereo HS");
                }
                if (p[5] & 0x02)
                {
                    APP_PRINT_INFO0("src_eir_parse(): Low latency mode");
                }
                else
                {
                    APP_PRINT_INFO0("src_eir_parse(): Normal mode");
                }

                sink_dev->headset_info_valid = true;
                memcpy(sink_dev->headset_info, p + 5, 2);

                if (field_len >= 9)
                {
                    sink_dev->headset_info[2] = *(p + 9);
                    APP_PRINT_INFO1("src_eir_parse(): headset feature: spp sample rate and multi enable %x",
                                    sink_dev->headset_info[2]);
                }
            }
        }
    }
}

bool src_low_latency_allowed(void)
{
    uint8_t i;
    uint8_t link_num = 0;
    uint8_t tws_num = 0;
    uint8_t stereo_num = 0;
    T_APP_DEV *sink_dev;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];

        if (sink_dev->bt_state != STATE_BT_ACL_CONNECTED)
        {
            continue;
        }
        link_num++;
        if (sink_dev->headset_info_valid)
        {
            if (!app_eir_parse_is_support_lowlatency(i))
            {
                APP_PRINT_INFO1("src_low_latency_allowed(): %u doesn't support ll",
                                i);
                return false;
            }
            if (sink_dev->headset_info[0] & 0x01)
            {
                tws_num++;
            }
            else
            {
                stereo_num++;
            }
        }
        else
        {
            APP_PRINT_INFO1("src_low_latency_allowed(): idx %u Normal headset exists", i);
            return false;
        }
    }

    /* Only TWS or STEREO headset connected */

    if (tws_num)
    {
        if (link_num < 2)
        {
            return true;
        }
    }
    else if (stereo_num)
    {
        if (link_num <= 2)
        {
            return true;
        }
    }

    return false;
}

static uint8_t src_get_proper_frame_num(uint8_t index)
{
    uint8_t frame_num = 0;
    uint8_t hs_num;
    uint8_t stereo_cfg_num = app_cfg_const.dongle_stereo_sbcs_per_pkt;
    uint8_t tws_cfg_num = app_cfg_const.dongle_tws_sbcs_per_pkt;

    APP_PRINT_INFO4("src_get_proper_frame_num(): %u headset info %d, %02x %02x",
                    index, sink_dev_list[index].headset_info_valid,
                    sink_dev_list[index].headset_info[0],
                    sink_dev_list[index].headset_info[1]);
    if (sink_dev_list[index].headset_info_valid)
    {
        if (!app_eir_parse_is_support_lowlatency(index))
        {
            /* Not support low latency */
            return 0;
        }

        hs_num = sink_dev_list[index].headset_info[1] & 0x0f;
        if (!hs_num)
        {
            /* Get num from app cfg */
            if (!(sink_dev_list[index].headset_info[0] & 0x01))
            {
                APP_PRINT_INFO0("src_get_proper_frame_num(): Stereo");
                /* Stereo */
                if (stereo_cfg_num != 0)
                {
                    frame_num = stereo_cfg_num;
                }
                else
                {
                    frame_num = SRC_STEREO_SBC_FRAME_NUM;
                }
            }
            else
            {
                APP_PRINT_INFO0("src_get_proper_frame_num(): TWS");
                /* TWS */
                if (tws_cfg_num != 0)
                {
                    frame_num = tws_cfg_num;
                }
                else
                {
                    frame_num = SRC_TWS_SBC_FRAME_NUM;
                }
            }
        }
        else
        {
            /* Headset info contains valid frame num */
            frame_num = hs_num;
        }
    }
    /* If headset has no low latency info in EIR, we set frame num to zero. */
    /* else if ((cfg_num & 0x0f) != 0)
     * {
     *     frame_num = cfg_num & 0x0f;
     * }
     */

    return frame_num;
}

uint8_t src_get_proper_frame_num2(void)
{
    uint8_t i;
    uint8_t link_num = 0;
    uint8_t tws_num = 0;
    uint8_t stereo_num = 0;
    uint8_t max_sbc_num = 0;
    uint8_t tmp;
    T_APP_DEV *sink_dev;

    for (i = 0; i < SINK_DEV_NUM_MAX; i++)
    {
        sink_dev = &sink_dev_list[i];

        if (sink_dev->bt_state != STATE_BT_ACL_CONNECTED)
        {
            continue;
        }
        link_num++;
        if (sink_dev->headset_info_valid)
        {
            if (!app_eir_parse_is_support_lowlatency(i))
            {
                APP_PRINT_INFO1("src_get_proper_frame_num2(): %u doesn't support ll",
                                i);
                return SRC_MAX_SBC_FRAME_NUM;
            }
            if (sink_dev->headset_info[0] & 0x01)
            {
                tws_num++;
            }
            else
            {
                stereo_num++;
            }
        }
        else
        {
            APP_PRINT_INFO1("src_get_proper_frame_num2(): idx %u Normal headset exists", i);
            return SRC_MAX_SBC_FRAME_NUM;
        }

        tmp = src_get_proper_frame_num(i);
        if (tmp == 0)
        {
            tmp = SRC_MAX_SBC_FRAME_NUM;
        }
        if (max_sbc_num < tmp)
        {
            max_sbc_num = tmp;
        }
    }

    /* Only TWS or STEREO headset connected */

    if (tws_num)
    {
        if (link_num < 2)
        {
            return max_sbc_num;
        }
    }
    else if (stereo_num)
    {
        if (link_num == 2)
        {
            if (max_sbc_num < 2 * SRC_STEREO_SBC_FRAME_NUM)
            {
                return 2 * SRC_STEREO_SBC_FRAME_NUM;
            }
            else
            {
                return max_sbc_num;
            }
        }
        else if (link_num == 1)
        {
            if (max_sbc_num < SRC_STEREO_SBC_FRAME_NUM)
            {
                return SRC_STEREO_SBC_FRAME_NUM;
            }
            else
            {
                return max_sbc_num;
            }
        }
    }

    return SRC_MAX_SBC_FRAME_NUM;
}

bool src_enter_low_latency(uint8_t index, uint8_t custom_sbc_num,
                           uint8_t *rc_sbc_num)
{
    if (gaming_a2dp_codec_is_lc3())
    {
        if (!src_low_latency_allowed())
        {
            return false;
        }
        if (!sink_dev_list[index].a2dp_opened)
        {
            return false;
        }
        low_latency_mode = 1;
        return true;
    }
    else
    {
        T_AUDIO_FORMAT_INFO *codec;
        uint8_t num;
        uint8_t bitpool;
        uint16_t acl_pkt_len = 0;
        uint16_t sbc_frame_size;

        if (!src_low_latency_allowed())
        {
            APP_PRINT_WARN0("src_enter_low_latency(): ll is not allowed");
            return false;
        }

        num = src_get_proper_frame_num2();

        if (!sink_dev_list[index].a2dp_opened)
        {
            APP_PRINT_WARN0("src_enter_low_latency: a2dp not opened");
            return false;
        }

        codec = &sink_dev_list[index].codec_info;
#ifdef CONFIG_FIX_BITPOOL
        bitpool = FIXED_BITPOOL;
#else
        bitpool = src_sbc_bitpool(codec, num, &acl_pkt_len);
#endif
        sbc_frame_size = src_sbc_frame_size(codec->attr.sbc.chann_mode,
                                            codec->attr.sbc.block_length,
                                            codec->attr.sbc.subband_num,
                                            bitpool);
        APP_PRINT_INFO4("src_enter_low_latency(): bitpool %u -> %u, num %u, acl %u",
                        codec->attr.sbc.bitpool, bitpool, num, acl_pkt_len);

        src_adjust_stream_codec(bitpool, num, sbc_frame_size);
        if (rc_sbc_num)
        {
            *rc_sbc_num = num;
        }

        low_latency_mode = 1;

        return true;
    }
}

void src_leave_low_latency(uint8_t *bdaddr)
{
    if (gaming_a2dp_codec_is_lc3())
    {
        uint8_t index = 0;
        if (!src_policy_find_dev_index_by_addr(bdaddr, &index))
        {
            return;
        }

        if (!profile_is_connected(bdaddr, A2DP_PROFILE_MASK))
        {
            return;
        }

        /* In single link mode, we enable Tpoll for normal mode. */
        if (!app_cfg_const.dongle_link_num_max)
        {
            legacy_gaming_xfer_stop(bdaddr, LEGACY_GAMING_XFER_TYPE);
        }
        low_latency_mode = 0;
    }
    else
    {
        uint8_t bitpool;
        T_AUDIO_FORMAT_INFO *codec;
        uint8_t index = 0;
        uint16_t acl_pkt_len = 0;
        uint8_t num;
        uint16_t sbc_frame_size;

        if (!src_policy_find_dev_index_by_addr(bdaddr, &index))
        {
            APP_PRINT_ERROR1("src_leave_low_latency(): Not find device %b",
                             TRACE_BDADDR(bdaddr));
            return;
        }

        if (!profile_is_connected(bdaddr, A2DP_PROFILE_MASK))
        {
            APP_PRINT_ERROR0("src_leave_low_latency(): a2dp not connected");
            return;
        }

        /* we enable Tpoll for normal mode. */
        if ((!app_cfg_const.dongle_link_num_max))
        {
            legacy_gaming_xfer_stop(bdaddr, LEGACY_GAMING_XFER_TYPE);
        }

        codec = &sink_dev_list[index].codec_info;
        num = SRC_MAX_SBC_FRAME_NUM;
#ifdef CONFIG_FIX_BITPOOL
        bitpool = FIXED_BITPOOL;
#else
        bitpool = src_sbc_bitpool(codec, num, &acl_pkt_len);
#endif
        sbc_frame_size = src_sbc_frame_size(codec->attr.sbc.chann_mode,
                                            codec->attr.sbc.block_length,
                                            codec->attr.sbc.subband_num,
                                            bitpool);

        APP_PRINT_INFO4("src_leave_low_latency(): idx %u, bitpool %u -> %u, acl %u",
                        index, codec->attr.sbc.bitpool, bitpool, acl_pkt_len);
        src_adjust_stream_codec(bitpool, num, sbc_frame_size);

        low_latency_mode = 0;
    }
}

void src_spp_req_enter_ll(uint8_t *bdaddr, uint8_t custom_sbc_num)
{
    uint8_t index = 0;
    uint8_t rc_num = 0;

    APP_PRINT_INFO1("src_spp_req_enter_ll(): %b ++", TRACE_BDADDR(bdaddr));

    if (!src_policy_find_dev_index_by_addr(bdaddr, &index))
    {
        APP_PRINT_ERROR1("src_spp_req_enter_ll(): Not find device %b",
                         TRACE_BDADDR(bdaddr));
        return;
    }

    if (!profile_is_connected(bdaddr, A2DP_PROFILE_MASK))
    {
        APP_PRINT_ERROR0("src_spp_req_enter_ll(): a2dp not connected");
        return;
    }

    if (src_announce_enter_or_leave_ll(true))
    {
        if (!src_enter_low_latency(index, custom_sbc_num, &rc_num))
        {
            APP_PRINT_ERROR0("src_spp_req_enter_ll: failed to enter low latency");
            return;
        }
    }

    /* In single link mode, we enable always-poll feature for gaming mode. */
    if (!app_cfg_const.dongle_link_num_max)
    {
        uint8_t i;
        uint8_t tws_num = 0;
        uint8_t stereo_num = 0;
        uint8_t link_num = 0;
        T_APP_DEV *sink_dev = NULL;

        for (i = 0; i < SINK_DEV_NUM_MAX; i++)
        {
            sink_dev = &sink_dev_list[i];

            if (sink_dev->bt_state != STATE_BT_ACL_CONNECTED)
            {
                continue;
            }
            link_num++;
            if (sink_dev->headset_info_valid)
            {
                if (!app_eir_parse_is_support_lowlatency(i))
                {
                    /* Headset doesn't support low latency. */
                    return;
                }
                if (sink_dev->headset_info[0] & 0x01)
                {
                    tws_num++;
                }
                else
                {
                    stereo_num++;
                }
            }
        }
        /* Enable always-tpoll when dongle and headset are connected and dongle
         * is in single link mode. when headset is multilink mode,no need enable this function.
         * */
        if (link_num == 1 && stereo_num == 1)
        {
            if (src_legacy_gaming_is_ready() && gaming_bt_a2dp_is_streaming())
            {
                legacy_gaming_xfer_start(bdaddr, LEGACY_GAMING_XFER_TYPE);
            }
        }
    }
}
void src_spp_req_leave_ll(uint8_t *bdaddr, uint8_t custom_sbc_num)
{

    uint8_t spp_num = src_get_spp_connected_link_num();

    /* only all link leave low latency, dongle leave latency */
    if (src_announce_enter_or_leave_ll(false) == spp_num)
    {
        APP_PRINT_INFO1("src_spp_req_leave_ll(): %b ++", TRACE_BDADDR(bdaddr));
        src_leave_low_latency(bdaddr);
    }
}
