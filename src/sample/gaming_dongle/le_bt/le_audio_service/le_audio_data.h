#ifndef _LE_AUDIO_DATA_H_
#define _LE_AUDIO_DATA_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "os_queue.h"
#include "codec_def.h"
#include "app_msg.h"
#include "bt_direct_msg.h"

#define LE_AUDIO_USB_PATH       0x00
#define LE_AUDIO_AUX_PATH       0x01
#define LE_AUDIO_AUX_MIC_PATH   0x02

#define LE_AUDIO_AUX_IN       0x01
#define LE_AUDIO_AUX_OUT      0x02

#define LE_AUDIO_AUX_DISABLE  0x00
#define LE_AUDIO_AUX_ENABLE   0x01

#define CIG_ISO_MODE        0x01
#define BIG_ISO_MODE        0x02

#define AUDIO_LEFT_OUTPUT_MASK    0x01
#define AUDIO_RIGHT_OUTPUT_MASK   0x02

typedef struct t_iso_data_ind
{
    struct t_iso_data_ind   *p_next;
    uint16_t  conn_handle;
    T_ISOCH_DATA_PKT_STATUS pkt_status_flag;
    uint32_t  time_stamp;
    uint16_t  iso_sdu_len;
    uint16_t  pkt_seq_num;
    uint8_t   *p_buf;
} T_ISO_DATA_IND;


typedef struct
{
    uint8_t                     audio_path;

    T_OS_QUEUE                  output_path_queue;       //controller to host
    bool                        output_send_to_tk;
    bool                        output_left_ready;
    bool                        output_right_ready;
    uint8_t                     output_src_num;
    uint8_t                     output_chnl_mask;
    uint8_t                     output_block_num;
    uint16_t                    output_left_handle;
    uint8_t                     output_left_len;
    T_OS_QUEUE                  output_left_queue;
    uint16_t                    output_right_handle;
    uint8_t                     output_right_len;
    T_OS_QUEUE                  output_right_queue;
    uint32_t                    output_sync_timestamp;

    T_OS_QUEUE                  input_path_queue;        //host to controller
    uint16_t                    input_total_len;
    uint16_t                    input_block_len;
    uint8_t                     input_blocks_per_sdu;
    uint16_t                    input_codec_frame_len;
    uint32_t                    input_frame_duration;
    uint8_t                     *p_input_buffer;
#if (LEA_BIS_DUAL_UAC_SUPPORT == 1)
    uint8_t                     *p_input_uac2;
#endif
    bool                        input_timer_start;
} T_LE_AUDIO_CO_DB;

typedef struct t_iso_chnl_info_db
{
    struct t_iso_chnl_info_db   *p_next;
    bool                        first_iso_is_send;
    uint8_t                     conn_id;
    uint8_t                     iso_mode;
    uint8_t                     path_direction;   //DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t                    iso_conn_handle;
    uint16_t                    packet_seq_num;
    uint16_t                    last_packet_seq_num;
    T_CODEC_CFG                 codec_data;
} T_ISO_CHNL_INFO_DB;


typedef struct
{
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t cis_conn_handle;
    T_CODEC_CFG codec_parsed_data;
} T_CIS_SETUP_DATA_PATH;

typedef struct
{
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t cis_conn_handle;
} T_CIS_REMOVE_DATA_PATH;


typedef struct
{
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t bis_conn_handle;
    T_CODEC_CFG codec_parsed_data;
} T_BIG_SETUP_DATA_PATH;

typedef struct
{
    uint8_t path_direction;//DATA_PATH_INPUT_FLAG or DATA_PATH_OUTPUT_FLAG
    uint16_t bis_conn_handle;
} T_BIG_REMOVE_DATA_PATH;

void le_audio_data_start_encode_path(T_CODEC_CFG *p_codec, uint32_t pd_delay);
void le_audio_data_stop_encode_path(void);

void handle_cis_data_path_setup_cmplt_msg(T_CIS_SETUP_DATA_PATH *p_data);
void handle_cis_data_path_remove_complt_msg(T_CIS_REMOVE_DATA_PATH *p_data);
void handle_bis_data_path_setup_complt_msg(T_BIG_SETUP_DATA_PATH *p_data);
void handle_bis_data_path_remove_complt_msg(T_BIG_REMOVE_DATA_PATH *p_data);
T_LE_AUDIO_CO_DB *le_audio_get_audio_db(void);
void le_audio_data_init(void);
void le_audio_uac_handle(T_IO_MSG *msg);
void le_audio_set_audio_path(uint8_t audio_path);
uint8_t le_audio_get_audio_path(void);
void le_audio_data_direct_callback(uint8_t cb_type, void *p_cb_data);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
