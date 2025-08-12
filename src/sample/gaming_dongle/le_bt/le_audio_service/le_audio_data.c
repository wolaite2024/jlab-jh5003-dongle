#include <string.h>
#include <stdlib.h>
#include "section.h"
#include "trace.h"
#include "hw_tim.h"
#include "le_audio_data.h"
#include "gap.h"
#include "gap_iso_data.h"
#include "codec_def.h"
#include "vcs_def.h"
#include "app_usb_layer.h"
#include "bt_syscall.h"
#include "ble_isoch_def.h"
#include "codec_qos.h"
#include "app_cyclic_buffer.h"
#include "le_unicast_src_service.h"

#include "rtl876x_tim.h"
#include "app_audio_path.h"
#include "app_timer.h"
#include "app_usb_uac.h"

#if APP_DEBUG_REPORT
#include "app_status_report.h"
#endif
#include "app_cfg.h"

#include "gaming_bt.h"
#include "app_line_in.h"

#include "app_downstream_encode.h"

#include "app_upstream_decode.h"

#if UAL_LATENCY_TEST
#include "rtl876x.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
static uint8_t signal = 0;
static bool first_data_send = false;
bool signal_float = false;
static uint8_t iso_data_test[] = {0x33, 0xb7, 0xa6, 0x51, 0x61, 0xa0, 0x4e, 0x36, 0xdd, 0xbe,
                                  0x66, 0x91, 0x0d, 0xce, 0x95, 0x2a, 0x40, 0x75, 0x22, 0x74,
                                  0xc5, 0x8a, 0xbf, 0x86, 0x58, 0xaa, 0xc1, 0xeb, 0xe3, 0x0b,
                                  0x99, 0x6b, 0x99, 0xd6, 0x68, 0x2e, 0x23, 0x51, 0xc8, 0x9b
                                 };
#endif

#if (LE_AUDIO_ISO_REF_CLK == 1)
#include "audio_probe.h"
#include "bt_syscall.h"
#endif

/* Timer load count 10ms that equals to bis/cis iso interval. */
#define LEAUDIO_SYNC_TIMER_LOADCOUNT    (10000)
#if (LE_AUDIO_ISO_REF_CLK == 1)
#define LEA_REF_HEADER_LEN (3 + sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO)/sizeof(uint32_t))
typedef enum
{
    UPLINK_SYNCREF_STAMP        = 0x5E,
} T_LEA_DSP_CMD;
#endif

#if (LE_AUDIO_REF_CLK ==1)
#include "syncclk_driver.h"
#define LEA_SYNC_CLK_REF SYNCCLK_ID4
#define REF_GUARD_TIME_US       (3000)  //lowerstack use 625
uint32_t g_session_id = 0;
bool first_codec_data = false;
#endif

#define USB_ISO_AP_GUARD_TIME_US    (3000)
#define ISO_DATA_CHNL_DELTA_TIMESTAP   3000
#define ISO_DATA_RING_BUFFER_DEPTH      4


void le_audio_data_direct_callback(uint8_t cb_type, void *p_cb_data);
static void app_rcv_and_send_anchor_point(uint32_t session_id);
static T_LE_AUDIO_CO_DB *p_audio_co_db;
static bool audio_path_lc3enc_created = false;

static bool app_lea_get_next_ap(T_BT_LE_ISO_SYNC_REF_AP_INFO *info, uint32_t *p_ap);

static T_ISO_CHNL_INFO_DB *find_iso_channel_db(uint16_t iso_conn_handle, uint8_t direction)
{
    int i;
    T_ISO_CHNL_INFO_DB *iso_elem = NULL;
    if (direction == DATA_PATH_INPUT_FLAG)
    {
        for (i = 0; i < p_audio_co_db->input_path_queue.count; i++)
        {
            iso_elem = (T_ISO_CHNL_INFO_DB *)os_queue_peek(&p_audio_co_db->input_path_queue, i);

            if (iso_elem == NULL)
            {
                return NULL;
            }

            if (iso_elem->iso_conn_handle == iso_conn_handle)
            {
                return iso_elem;
            }
        }
    }
    else
    {
        for (i = 0; i < p_audio_co_db->output_path_queue.count; i++)
        {
            iso_elem = (T_ISO_CHNL_INFO_DB *)os_queue_peek(&p_audio_co_db->output_path_queue, i);

            if (iso_elem == NULL)
            {
                return NULL;
            }

            if (iso_elem->iso_conn_handle == iso_conn_handle)
            {
                return iso_elem;
            }
        }
    }
    return NULL;

}

static void remove_iso_channel_db(uint16_t iso_conn_handle, uint8_t direction)
{
    int i;
    T_ISO_CHNL_INFO_DB *iso_elem = NULL;
    if (direction == DATA_PATH_INPUT_FLAG)
    {
        for (i = 0; i < p_audio_co_db->input_path_queue.count; i++)
        {
            iso_elem = (T_ISO_CHNL_INFO_DB *)os_queue_peek(&p_audio_co_db->input_path_queue, i);

            if (iso_elem == NULL)
            {
                return;
            }

            if (iso_elem->iso_conn_handle == iso_conn_handle)
            {
                APP_PRINT_INFO2("remove_iso_channel_db: remove path handle %u curr_num %x",
                                iso_elem->iso_conn_handle,
                                p_audio_co_db->input_path_queue.count);
                os_queue_delete(&p_audio_co_db->input_path_queue, iso_elem);
                free(iso_elem);
                return;
            }
        }
    }
    else
    {
        for (i = 0; i < p_audio_co_db->output_path_queue.count; i++)
        {
            iso_elem = (T_ISO_CHNL_INFO_DB *)os_queue_peek(&p_audio_co_db->output_path_queue, i);

            if (iso_elem == NULL)
            {
                return;
            }
            if (iso_elem->iso_conn_handle == iso_conn_handle)
            {
                os_queue_delete(&p_audio_co_db->output_path_queue, iso_elem);
                APP_PRINT_INFO1("remove_iso_channel_db cnt %x", p_audio_co_db->output_path_queue.count);
                free(iso_elem);
                return;
            }
        }
        if (p_audio_co_db->output_path_queue.count == 0)
        {
            p_audio_co_db->output_block_num = 0;
#if APP_DEBUG_REPORT
            app_status_report.total_decode_pkts = 0;
            app_status_report.first_decode_pkt_seq = 0;
            app_status_report.last_decode_pkt_seq = 0;
            app_status_report.total_decode_pkts = 0;
#endif

        }
    }
}

static void clear_iso_channel_queue(uint8_t output_mask)
{
    int i;
    T_ISO_DATA_IND *iso_elem = NULL;
    T_OS_QUEUE *p_queue = NULL;

    if (AUDIO_LEFT_OUTPUT_MASK & output_mask)
    {
        p_queue = &(p_audio_co_db->output_left_queue);
        for (i = 0; i < p_queue->count; i++)
        {
            iso_elem = (T_ISO_DATA_IND *)os_queue_peek(p_queue, i);
            os_queue_delete(p_queue, iso_elem);
            free(iso_elem);
        }
    }

    if (AUDIO_RIGHT_OUTPUT_MASK & output_mask)
    {
        p_queue = &(p_audio_co_db->output_right_queue);
        for (i = 0; i < p_queue->count; i++)
        {
            iso_elem = (T_ISO_DATA_IND *)os_queue_peek(p_queue, i);
            os_queue_delete(p_queue, iso_elem);
            free(iso_elem);
        }
    }
}

void handle_cis_data_path_setup_cmplt_msg(T_CIS_SETUP_DATA_PATH *p_data)
{
#if TARGET_LE_AUDIO_GAMING_DONGLE
    uint8_t prev_input_cnt = p_audio_co_db->input_path_queue.count;
    uint8_t prev_output_cnt = p_audio_co_db->output_path_queue.count;
#endif

    APP_PRINT_INFO3("handle_cis_data_path_setup_cmplt_msg: cis handle %04x, "
                    "dir %u, audio_channel_allocation %08x",
                    p_data->cis_conn_handle,
                    p_data->path_direction,
                    p_data->codec_parsed_data.audio_channel_allocation);

    T_ISO_CHNL_INFO_DB *iso_cb = find_iso_channel_db(p_data->cis_conn_handle,
                                                     p_data->path_direction);
    if (iso_cb != NULL)
    {
        APP_PRINT_WARN0("handle_cis_data_path_setup_cmplt_msg iso channl already exist");
    }
    else
    {
        iso_cb = calloc(1, sizeof(T_ISO_CHNL_INFO_DB));
        if (iso_cb == NULL)
        {
            APP_PRINT_ERROR0("handle_cis_data_path_setup_cmplt_msg alloc iso_cb fail");
            return;
        }
        iso_cb->path_direction = p_data->path_direction;
        iso_cb->iso_mode = CIG_ISO_MODE;
        if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
        {
            os_queue_in(&p_audio_co_db->input_path_queue, (void *)iso_cb);
        }
        else
        {
            os_queue_in(&p_audio_co_db->output_path_queue, (void *) iso_cb);
            APP_PRINT_INFO1("handle_cis_data_path_setup_cmplt_msg num %x",
                            p_audio_co_db->output_path_queue.count);
        }
    }
    iso_cb->codec_data = p_data->codec_parsed_data;
    iso_cb->packet_seq_num = 0;
    iso_cb->first_iso_is_send = false;
    iso_cb->iso_conn_handle = p_data->cis_conn_handle;

#if TARGET_LE_AUDIO_GAMING_DONGLE
    if (prev_input_cnt == 0 && p_audio_co_db->input_path_queue.count != 0)
    {
        app_usb_ds_lea_pipe_create(&p_data->codec_parsed_data);
    }
    else if (prev_output_cnt == 0 && p_audio_co_db->output_path_queue.count != 0)
    {
        app_upstream_lea_pipe_create(&p_data->codec_parsed_data);
    }
#endif

    if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
    {
        if (!audio_path_lc3enc_created)
        {
            le_audio_data_start_encode_path(&p_data->codec_parsed_data, 5000);
        }

        app_rcv_and_send_anchor_point(g_session_id);
    }
    else
    {
        uint8_t chnl_cnt;
        if (p_data->codec_parsed_data.audio_channel_allocation == AUDIO_LOCATION_MONO)
        {
            chnl_cnt = 1;
        }
        else
        {
            chnl_cnt = __builtin_popcount(p_data->codec_parsed_data.audio_channel_allocation);
        }

        uint8_t codec_len = 0;
        uint8_t blocks_num = 1;
        if (p_data->codec_parsed_data.type_exist & CODEC_CFG_OCTET_PER_CODEC_FRAME_EXIST)
        {
            codec_len = p_data->codec_parsed_data.octets_per_codec_frame;
        }
        if (p_data->codec_parsed_data.type_exist & CODEC_CFG_TYPE_BLOCKS_PER_SDU_EXIST)
        {
            blocks_num = p_data->codec_parsed_data.codec_frame_blocks_per_sdu;
        }

        if (p_data->codec_parsed_data.frame_duration == FRAME_DURATION_CFG_7_5_MS)
        {
            p_audio_co_db->input_frame_duration = 7500;
        }
        else
        {
            p_audio_co_db->input_frame_duration = 10000;
        }

        if (chnl_cnt == 2)
        {
            p_audio_co_db->output_chnl_mask = AUDIO_LEFT_OUTPUT_MASK | AUDIO_RIGHT_OUTPUT_MASK;
            p_audio_co_db->output_left_handle = iso_cb->iso_conn_handle;
            p_audio_co_db->output_right_handle = iso_cb->iso_conn_handle;
            p_audio_co_db->output_left_len = p_audio_co_db->output_right_len = codec_len;
            p_audio_co_db->output_block_num = blocks_num;
        }
        else
        {
            APP_PRINT_INFO2("handle_cis_data_path_setup_cmplt_msg: codec_len %x block_num %x", codec_len,
                            blocks_num);
            if (p_data->codec_parsed_data.audio_channel_allocation & AUDIO_CHANNEL_LOCATION_FL)
            {
                p_audio_co_db->output_chnl_mask |= AUDIO_LEFT_OUTPUT_MASK;
                p_audio_co_db->output_left_handle = iso_cb->iso_conn_handle;
                p_audio_co_db->output_left_len = codec_len;
                p_audio_co_db->output_block_num = blocks_num;
            }
            else
            {
                p_audio_co_db->output_chnl_mask |= AUDIO_RIGHT_OUTPUT_MASK;     //all left use right channel
                p_audio_co_db->output_right_handle = iso_cb->iso_conn_handle;
                p_audio_co_db->output_right_len = codec_len;
            }

            if (p_audio_co_db->output_left_handle != 0 && p_audio_co_db->output_right_handle != 0)
            {
                T_ISO_CHNL_INFO_DB *p_left_iso_cb = find_iso_channel_db(p_audio_co_db->output_left_handle,
                                                                        DATA_PATH_OUTPUT_FLAG);
                T_ISO_CHNL_INFO_DB *p_right_iso_cb = find_iso_channel_db(p_audio_co_db->output_right_handle,
                                                                         DATA_PATH_OUTPUT_FLAG);
                if (p_left_iso_cb->codec_data.sample_frequency != p_right_iso_cb->codec_data.sample_frequency ||
                    p_left_iso_cb->codec_data.codec_frame_blocks_per_sdu !=
                    p_right_iso_cb->codec_data.codec_frame_blocks_per_sdu ||
                    p_left_iso_cb->codec_data.octets_per_codec_frame !=
                    p_right_iso_cb->codec_data.octets_per_codec_frame)
                {
                    APP_PRINT_ERROR0("app_unicast_start_stream: mis-match left and right, use only left");
                    p_audio_co_db->output_chnl_mask = AUDIO_LEFT_OUTPUT_MASK;
                    codec_len = p_audio_co_db->output_left_len;
                    blocks_num = p_audio_co_db->output_block_num;
                }
            }

            p_audio_co_db->output_block_num = blocks_num;
        }

        clear_iso_channel_queue(p_audio_co_db->output_chnl_mask);
    }
}

void handle_cis_data_path_remove_complt_msg(T_CIS_REMOVE_DATA_PATH *p_data)
{
    APP_PRINT_INFO2("handle_cis_data_path_remove_complt_msg: cis handle %04x, dir %u",
                    p_data->cis_conn_handle,
                    p_data->path_direction);
    T_ISO_CHNL_INFO_DB *iso_cb = find_iso_channel_db(p_data->cis_conn_handle,
                                                     p_data->path_direction);

    if (iso_cb == NULL)
    {
        APP_PRINT_ERROR2("handle_cis_data_path_remove_complt_msg: handle %x dir %x",
                         p_data->cis_conn_handle,
                         p_data->path_direction);
        return;
    }
    remove_iso_channel_db(p_data->cis_conn_handle, p_data->path_direction);

    if (p_audio_co_db->input_path_queue.count == 0)
    {
        app_usb_ds_pipe_release();
    }
    else if (p_audio_co_db->output_path_queue.count == 0)
    {
        upstream_pipe_release();
    }

    if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
    {
        if (p_audio_co_db->input_path_queue.count == 0)
        {
            if (audio_path_lc3enc_created)
            {
                le_audio_data_stop_encode_path();
            }
        }
    }
    else
    {
        if (iso_cb->iso_conn_handle == p_audio_co_db->output_left_handle)
        {
            if (p_audio_co_db->output_left_handle == p_audio_co_db->output_right_handle)
            {
                p_audio_co_db->output_chnl_mask = 0;
            }
            else
            {
                p_audio_co_db->output_chnl_mask &= (~AUDIO_LEFT_OUTPUT_MASK);
            }
            p_audio_co_db->output_left_handle = 0;
            p_audio_co_db->output_left_ready = false;
            clear_iso_channel_queue(AUDIO_LEFT_OUTPUT_MASK);
        }
        else if (iso_cb->iso_conn_handle == p_audio_co_db->output_right_handle)
        {
            p_audio_co_db->output_chnl_mask &= (~AUDIO_RIGHT_OUTPUT_MASK);
            p_audio_co_db->output_right_handle = 0;
            p_audio_co_db->output_right_ready = false;
            clear_iso_channel_queue(AUDIO_RIGHT_OUTPUT_MASK);
        }
    }

}

void handle_bis_data_path_setup_complt_msg(T_BIG_SETUP_DATA_PATH *p_data)
{
    T_ISO_CHNL_INFO_DB *iso_cb = find_iso_channel_db(p_data->bis_conn_handle,
                                                     p_data->path_direction);
    if (iso_cb != NULL)
    {
        APP_PRINT_WARN0("handle_bis_data_path_setup_complt_msg iso channl already exist");
    }
    else
    {
        iso_cb = calloc(1, sizeof(T_ISO_CHNL_INFO_DB));
        if (iso_cb == NULL)
        {
            APP_PRINT_ERROR0("handle_bis_data_path_setup_complt_msg alloc iso_cb fail");
            return;
        }
        iso_cb->path_direction = p_data->path_direction;
        iso_cb->iso_mode = BIG_ISO_MODE;

        iso_cb->packet_seq_num = 0;
        if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
        {
            os_queue_in(&p_audio_co_db->input_path_queue, (void *) iso_cb);
        }
        else
        {
            os_queue_in(&p_audio_co_db->output_path_queue, (void *) iso_cb);
        }
    }
    iso_cb->codec_data = p_data->codec_parsed_data;
    iso_cb->iso_conn_handle = p_data->bis_conn_handle;

    APP_PRINT_TRACE3("handle_bis_data_path_setup_complt_msg: iso_conn_handle 0x%x path_direction %d,  channle location 0x%x",
                     p_data->bis_conn_handle, p_data->path_direction, iso_cb->codec_data.audio_channel_allocation);

    if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
    {
        if (!audio_path_lc3enc_created)
        {
            le_audio_data_start_encode_path(&p_data->codec_parsed_data, 5000);
        }

        app_rcv_and_send_anchor_point(g_session_id);
    }
}

void handle_bis_data_path_remove_complt_msg(T_BIG_REMOVE_DATA_PATH *p_data)
{
    remove_iso_channel_db(p_data->bis_conn_handle, p_data->path_direction);

    //FIX TODO
    if (p_data->path_direction == DATA_PATH_INPUT_FLAG)
    {
        if (audio_path_lc3enc_created)
        {
            le_audio_data_stop_encode_path();
        }
    }
}

void le_audio_set_audio_path(uint8_t audio_path)
{
    audio_path = audio_path > LE_AUDIO_AUX_MIC_PATH ? LE_AUDIO_USB_PATH : audio_path;
    if (app_line_in_plug_in())
    {
        audio_path = LE_AUDIO_AUX_PATH;
    }
    APP_PRINT_INFO1("le_audio_set_audio_path %d", audio_path);
    p_audio_co_db->audio_path = audio_path;
}

uint8_t le_audio_get_audio_path(void)
{
    return p_audio_co_db->audio_path;
}

void le_audio_data_start_encode_path(T_CODEC_CFG *p_codec, uint32_t pd_delay)
{
    APP_PRINT_INFO2("le_audio_data_start_encode_path: lc3enc %d pd_delay %d", audio_path_lc3enc_created,
                    pd_delay);
    if ((p_codec == NULL) || (audio_path_lc3enc_created))
    {
        return;
    }

    APP_PRINT_INFO1("le_audio_data_start_encode_path:  pd_delay %d", pd_delay);

    if (p_audio_co_db->p_input_buffer)
    {
        APP_PRINT_WARN0("le_audio_data_start_encode_path:  already playing");
        return;
    }

    if (p_codec->type_exist & CODEC_CFG_TYPE_BLOCKS_PER_SDU_EXIST)
    {
        p_audio_co_db->input_blocks_per_sdu = p_codec->codec_frame_blocks_per_sdu;
        p_audio_co_db->input_total_len = p_codec->octets_per_codec_frame *
                                         p_codec->codec_frame_blocks_per_sdu * 2
                                         * ISO_DATA_RING_BUFFER_DEPTH + 1;
        p_audio_co_db->input_block_len = p_codec->octets_per_codec_frame *
                                         p_codec->codec_frame_blocks_per_sdu *
                                         2;
    }
    else
    {
        p_audio_co_db->input_blocks_per_sdu = 1;
        p_audio_co_db->input_total_len = p_codec->octets_per_codec_frame * 2 * ISO_DATA_RING_BUFFER_DEPTH +
                                         1;
        p_audio_co_db->input_block_len = p_codec->octets_per_codec_frame * 2;
    }

    p_audio_co_db->input_codec_frame_len = p_codec->octets_per_codec_frame;
    APP_PRINT_INFO1("le_audio_data_start_encode_path:  total_len %d", p_audio_co_db->input_total_len);

    if (p_audio_co_db->p_input_buffer != NULL)
    {
        free(p_audio_co_db->p_input_buffer);
    }
    p_audio_co_db->p_input_buffer = calloc(1, p_codec->octets_per_codec_frame * 2);
    if (p_audio_co_db->p_input_buffer == NULL)
    {
        APP_PRINT_ERROR0("le_audio_data_start_encode_path: alloc iso buffer fail");
        return;
    }

#if APP_DEBUG_REPORT
    app_status_report.codec_up_ringbuffer_bytes = 0;
#endif

    audio_path_lc3enc_created = true;
#if (LE_AUDIO_REF_CLK ==1)
    syncclk_drv_timer_start(LEA_SYNC_CLK_REF, CONN_HANDLE_TYPE_FREERUN_CLOCK, 0xFF, 0);
    first_codec_data = false;
#endif

}

void le_audio_data_stop_encode_path(void)
{
    APP_PRINT_INFO1("le_audio_data_stop_encode_path: lc3enc %d", audio_path_lc3enc_created);
    p_audio_co_db->input_total_len = 0;
    p_audio_co_db->input_blocks_per_sdu = p_audio_co_db->input_codec_frame_len = 0;
    if (p_audio_co_db->p_input_buffer != NULL)
    {
        free(p_audio_co_db->p_input_buffer);
    }
    p_audio_co_db->p_input_buffer = NULL;
#if (LEA_BIS_DUAL_UAC_SUPPORT == 1)
    if (p_audio_co_db->p_input_uac2 != NULL)
    {
        free(p_audio_co_db->p_input_uac2);
    }
    p_audio_co_db->p_input_uac2 = NULL;
#endif

    audio_path_lc3enc_created = false;
#if (LE_AUDIO_REF_CLK ==1)
    syncclk_drv_timer_stop(LEA_SYNC_CLK_REF);
    first_codec_data = false;
#endif
}

void le_audio_data_direct_callback(uint8_t cb_type, void *p_cb_data)
{
    T_BT_DIRECT_CB_DATA *p_data = (T_BT_DIRECT_CB_DATA *)p_cb_data;
    if (!p_data || !(p_data->p_bt_direct_iso))
    {
        return;
    }

    T_BT_DIRECT_ISO_DATA_IND *p_iso = p_data->p_bt_direct_iso;
    T_LE_AUDIO *p_link = NULL;

    switch (cb_type)
    {
    case BT_DIRECT_MSG_ISO_DATA_IND:
        {
            APP_PRINT_TRACE5("le_audio_data_direct_callback: conn_handle 0x%x, iso_sdu_len %d, pkt_seq_num 0x%x, time_stamp 0x%x, pkt_status_flag 0x%x",
                             p_iso->conn_handle, p_iso->iso_sdu_len,
                             p_iso->pkt_seq_num, p_iso->time_stamp,
                             p_iso->pkt_status_flag);

            if (p_audio_co_db->output_left_handle == p_iso->conn_handle)
            {
                if (p_iso->iso_sdu_len != p_audio_co_db->output_left_len)
                {
                    p_iso->pkt_status_flag = ISOCH_DATA_PKT_STATUS_POSSIBLE_ERROR_DATA;
                }
                if (!p_audio_co_db->output_left_ready)
                {
                    if (p_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
                    {
                        APP_PRINT_INFO1("lea_data_handle_multi_src_direct pkt_seq_num %x left ready",
                                        p_iso->pkt_seq_num);
                        p_audio_co_db->output_left_ready = true;
#if TARGET_LE_AUDIO_GAMING_DONGLE
                        iso_queue_clear(ISO_LEFT_QUEUE);
#endif
                    }
                }
            }
            else
            {
                if (p_iso->iso_sdu_len != p_audio_co_db->output_right_len)
                {
                    p_iso->pkt_status_flag = ISOCH_DATA_PKT_STATUS_POSSIBLE_ERROR_DATA;
                }
                if (!p_audio_co_db->output_right_ready)
                {
                    if (p_iso->pkt_status_flag == ISOCH_DATA_PKT_STATUS_VALID_DATA)
                    {
                        APP_PRINT_INFO1("lea_data_handle_multi_src_direct pkt_seq_num %x right ready",
                                        p_iso->pkt_seq_num);
                        p_audio_co_db->output_right_ready = true;
#if TARGET_LE_AUDIO_GAMING_DONGLE
                        iso_queue_clear(ISO_RIGHT_QUEUE);
#endif
                    }
                }
            }

            p_link = ble_audio_find_by_conn_handle(p_iso->conn_handle);
            if (p_link)
            {
                if ((p_link->mic_mute == 1) && (p_iso->iso_sdu_len))
                {
                    memset(p_iso->p_buf + p_iso->offset, 0, p_iso->iso_sdu_len);
                }
            }

#if TARGET_LE_AUDIO_GAMING_DONGLE
            us_handle_lea_data(p_data->p_bt_direct_iso);
#endif
            gap_iso_data_cfm(p_iso->p_buf);
        }
        break;

    default:
        APP_PRINT_ERROR1("le_audio_data_direct_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
}

static bool app_lea_get_next_ap(T_BT_LE_ISO_SYNC_REF_AP_INFO *info, uint32_t *p_ap)
{
    volatile uint32_t anchor_point = 0;
    uint32_t iso_interval = 0;
    volatile uint32_t freerun_clk = 0;
    uint32_t cnt = 0;
    T_SYNCCLK_LATCH_INFO_TypeDef *p_latch_info = NULL;

    p_latch_info = synclk_drv_time_get(LEA_SYNC_CLK_REF);

    anchor_point = info->group_anchor_point;
    iso_interval = info->iso_interval_us;
    freerun_clk = p_latch_info->exp_sync_clock;

    if (!iso_interval)
    {
        APP_PRINT_ERROR0("app_lea_get_next_ap: iso_interval invalid");
        return false;
    }

    if ((freerun_clk == 0) || (anchor_point == 0))
    {
        APP_PRINT_ERROR2("app_lea_get_next_ap: freerun_clk %x anchor_point %x",
                         freerun_clk, anchor_point);
        return false;
    }

    while (freerun_clk > anchor_point)
    {
        cnt ++;
        if (cnt % 100 == 0)
        {
            APP_PRINT_ERROR3("app_lea_get_next_ap: error freerun_clk %x anchor_point %x cnt %d",
                             freerun_clk, info->group_anchor_point, cnt);
        }
        if (0xFFFFFFFF - freerun_clk + anchor_point <= iso_interval)
        {
            break;
        }

        anchor_point += iso_interval;
    }

    *p_ap = anchor_point;
    return true;
}

#if (LE_AUDIO_ISO_REF_CLK == 1)
static void app_lea_send_ref(T_BT_LE_ISO_SYNC_REF_AP_INFO *info, uint32_t session_id,
                             uint32_t guard_time_us)
{
    APP_PRINT_INFO1("app_lea_send_ref: session_id 0x%x", session_id);
    uint32_t cmd_buf[LEA_REF_HEADER_LEN] = {0};
    cmd_buf[0] = UPLINK_SYNCREF_STAMP | ((LEA_REF_HEADER_LEN - 1) << 16);
    cmd_buf[1] = session_id;     //session_id
    cmd_buf[2] = guard_time_us;
    memcpy(&cmd_buf[3], info, sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO));
    audio_probe_dsp_send((uint8_t *)&cmd_buf[0], (LEA_REF_HEADER_LEN * 4));
}

#ifdef LEGACY_BT_GAMING
static void legacy_lc3_auxin_get_anchor_point(uint32_t session_id)
{
    T_BT_LE_ISO_SYNC_REF_AP_INFO info;
    memset(&info, 0, sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO));
    info.dir = 0x01;
    info.iso_interval_us = 0x2710; //10ms(us)
    info.sdu_seq_num = gaming_sync_app_seq_number(0);
    T_SYNCCLK_LATCH_INFO_TypeDef *p_latch_info  = synclk_drv_time_get(LEA_SYNC_CLK_REF);
    info.group_anchor_point = p_latch_info->exp_sync_clock;
    APP_PRINT_INFO4("ref_ap_info: dir:%x, sdu_seq_num:%x, iso_interval_us:%x group_anchor_point:%x ",
                    info.dir,
                    info.sdu_seq_num,
                    info.iso_interval_us,
                    info.group_anchor_point);
    app_lea_send_ref(&info, session_id, REF_GUARD_TIME_US);
}
#endif

static void app_rcv_and_send_anchor_point(uint32_t session_id)
{
    T_BT_LE_ISO_SYNC_REF_AP_INFO info;
    T_ISO_CHNL_INFO_DB *iso_elem = NULL;
    uint32_t anchor_point = 0;

    memset(&info, 0, sizeof(T_BT_LE_ISO_SYNC_REF_AP_INFO));
    /* FIXME: what to do if get multi active conn_handle */
    for (uint8_t i = 0; i < p_audio_co_db->input_path_queue.count; i++)
    {
        iso_elem = (T_ISO_CHNL_INFO_DB *)os_queue_peek(&p_audio_co_db->input_path_queue, i);
        if (iso_elem == NULL)
        {
            return;
        }

        if (iso_elem->path_direction != DATA_PATH_INPUT_FLAG)
        {
            continue;
        }

        info.dir = 1;
        info.conn_handle = iso_elem->iso_conn_handle;
        bt_get_le_iso_sync_ref_ap_info(&info);

        if (info.group_anchor_point == 0)
        {
            APP_PRINT_WARN1("app_rcv_and_send_anchor_point: handle %x group_anchor_point invalid",
                            iso_elem->iso_conn_handle);
            continue;
        }

        if (app_lea_get_next_ap(&info, &anchor_point) == false)
        {
            continue;
        }

        info.group_anchor_point = anchor_point;
        APP_PRINT_TRACE2("app_rcv_and_send_anchor_point: 0x%08x, ap %d",
                         iso_elem->iso_conn_handle, info.group_anchor_point);
        app_lea_send_ref(&info, session_id, REF_GUARD_TIME_US);
        break;
    }
}

static void app_lea_probe_cback(T_AUDIO_PROBE_EVENT event, void *buf)
{
    APP_PRINT_INFO1("app_lea_probe_cback: event 0x%x", event);

    if (event != PROBE_SYNC_REF_REQUEST)
    {
        return;
    }

    g_session_id = (uint32_t)buf;
    if (g_session_id == 0)
    {
        return;
    }

    extern uint8_t app_get_cur_bt_mode(void);
    if (app_get_cur_bt_mode() == 0)
    {
#ifdef LEGACY_BT_GAMING
        legacy_lc3_auxin_get_anchor_point(g_session_id);
#endif
    }
    else
    {
        app_rcv_and_send_anchor_point(g_session_id);
    }
}
#endif

RAM_TEXT_SECTION T_LE_AUDIO_CO_DB *le_audio_get_audio_db(void)
{
    return p_audio_co_db;
}

void le_audio_data_init(void)
{
    p_audio_co_db = calloc(1, sizeof(T_LE_AUDIO_CO_DB));
    if (p_audio_co_db == NULL)
    {
        APP_PRINT_ERROR0("le_audio_data_init: alloc fail!");
        return;
    }
    p_audio_co_db->output_chnl_mask = 0;
    os_queue_init(&p_audio_co_db->input_path_queue);
    os_queue_init(&p_audio_co_db->output_path_queue);
    gap_register_direct_cb(le_audio_data_direct_callback);

    le_audio_set_audio_path(app_cfg_const.dongle_media_device);

#if 0
#if UAL_LATENCY_TEST
    p_audio_co_db->audio_path = LE_AUDIO_AUX_PATH;
#endif
#endif

#if (LE_AUDIO_ISO_REF_CLK == 1)
    audio_probe_dsp_cback_register(app_lea_probe_cback);
#endif
}
