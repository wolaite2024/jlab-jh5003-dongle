#ifndef _APP_GAMING_SYNC_H_
#define _APP_GAMING_SYNC_H_

#include "app_cfg.h"

#define DONGLE_FORMAT_START_BIT         0x52
#define DONGLE_FORMAT_STOP_BIT          0x54

#define FIX_CHANNEL_CID                 0x0008
#define LE_FIX_CHANNEL_ID               0x0A

#define LE_LINK_ID_INVALID              0xff

#define GAMING_SBC_FRAME_NUM            2
#define GAMING_SBC_MAX_FRAME_NUM        4
#define GAMING_LC3_FRAME_NUM            1
#define GAMING_LC3_MAX_FRAME_NUM        2
#define GAMING_SBC_FRAME_SIZE           87
#define GAMING_SBC_BITPOOL              37

#define GAMING_SYNC_STREAMING_TIMEOUT       250
#define GAMING_SYNC_STREAM_DETECT_INTERVAL  150

typedef enum
{
    GAMING_CODEC_NONE,
    PCM_16K_16BIT_MONO_10MS,
    PCM_48K_16BIT_MONO_10MS,
    PCM_48K_16BIT_STEREO_0_5MS,
    PCM_48K_16BIT_STEREO_1MS,
    PCM_48K_16BIT_STEREO_2MS,
    PCM_48K_24BIT_STEREO_0_5MS,
    PCM_48K_24BIT_STEREO_1MS,
    PCM_48K_24BIT_STEREO_2MS,
    PCM_96K_24BIT_STEREO_0_5MS,
    SBC_48K_16BIT_STEREO,
    LC3_16K_16BIT_MONO_7_5MS,
    LC3_32K_16BIT_MONO_7_5MS,
    LC3_16K_16BIT_MONO_10MS,
    LC3_32K_16BIT_MONO_10MS,
    LC3_48K_16BIT_MONO_10MS,
    LC3_16K_16BIT_STEREO_7_5MS,
    LC3_32K_16BIT_STEREO_7_5MS,
    LC3_48K_16BIT_STEREO_7_5MS,
    LC3_16K_16BIT_STEREO_10MS,
    LC3_32K_16BIT_STEREO_10MS,
    LC3_48K_16BIT_STEREO_10MS,
} T_GAMING_CODEC;

typedef enum
{
    PHONE_STREAM_IDLE,
    PHONE_STREAM_A2DP,
    PHONE_STREAM_HFP,
    PHONE_STREAM_LEA_CALL,
} T_PHONE_STREAM_STATUS;

typedef enum
{
    HEADSET_SOURCE_DONT_CARE    = 0x00,
    HEADSET_SOURCE_24G          = 0x01,
    HEADSET_SOURCE_BT           = 0x02,
} T_HEADSET_SOURCE;

typedef enum
{
    LC3PLUS_LOW_LATENCY_MODE,
    LC3PLUS_ULTRA_LOW_LATENCY_MODE,
    LC3PLUS_HIRES_MODE,
} T_LC3PLUS_ENCODE_MODE;

typedef enum
{
    HEADSET_PHONE_DISCONNECTED,
    HEADSET_PHONE_CONNECTED,
} T_HEADSET_CONN_STATUS;

typedef struct
{
    T_PHONE_STREAM_STATUS phone_status;
    bool audio_mixing_support;
    bool headset_linkback;
    bool rtp_enable;
    bool enable_3M;
    T_HEADSET_SOURCE headset_source;
    bool audio_jack_unplugged;
    T_LC3PLUS_ENCODE_MODE lc3plus_mode;
    T_HEADSET_CONN_STATUS conn_status;
    bool upstream_enable;
    T_GAMING_CODEC upstream_codec;
    bool increase_a2dp_interval;
} T_HEADSET_STATUS;

/* EIR data information carried by headset. */
typedef struct
{
    uint8_t length;

    /* 0xff: manufacturer data */
    uint8_t type;

    /* 0x5d, 0x00, 0x08 */
    uint8_t magic_data[3];

    /* bit0: 0- stereo headset 1- TWS headset
       bit1: 1- support LowLatency with Gaming Dongle
             0- not support LowLatency with Gaming Dongle */
    uint8_t headset_type;

    /* bit 3~0:
       0: sbc frame nums in each avdtp packet depend on dongle side */
    uint8_t sbc_frame_num;

    /* (app_cfg_const.rws_custom_uuid >> 8) & 0xFF; */
    uint8_t pairing_id_1;
    /* app_cfg_const.rws_custom_uuid & 0xFF; */
    uint8_t pairing_id_2;

    /*
        bit 1~0: Set SPP Voice Sample Rate.
        bit   2: Set Multilink feature bit.
        bit 7~3: rsv.
    */
    uint8_t feature_set;
} __attribute__((packed)) T_DONGLE_EIR_DATA;

/* brief Information carried by headset in BLE data(2.4G legacy mode) */
typedef struct
{
    uint8_t length;
    uint8_t type;

    uint8_t pairing_bit : 1;
    uint8_t rsv2 : 7;

    T_DONGLE_EIR_DATA eir_data;

    uint8_t rsv[18];
} __attribute__((packed)) T_LEGACY_DONGLE_BLE_DATA;

/* brief Information carried by headset in BLE data(2.4G lea mode) */
typedef struct
{
    uint8_t length;
    uint8_t ad_type;
    uint8_t ad_type_flag;

    uint8_t pairing_bit : 1;
    uint8_t low_latency : 1;
    uint8_t bud_side : 1;
    uint8_t is_stereo : 1;
    uint8_t rsv2 : 4;

    /* (app_cfg_const.rws_custom_uuid >> 8) & 0xFF; */
    uint8_t pairing_id_HI;
    /* app_cfg_const.rws_custom_uuid & 0xFF; */
    uint8_t pairing_id_LO;

    uint8_t pri_bud_mac[3];

    uint8_t psri_data[6];

    uint8_t public_addr[6];
    uint8_t rsv[10];
} __attribute__((packed)) T_DONGLE_LEA_ADV_DATA;

enum
{
    SYNC_PKT_START = 0,
    SYNC_PKT_HEADER,
    SYNC_PKT_DATA,
};

typedef enum
{
    DONGLE_TYPE_UPSTREAM_VOICE = 0x00,
    DONGLE_TYPE_CMD  = 0x01,
    DONGLE_TYPE_CONTROLLER_HID_DATA = 0x02,
} T_APP_DONGLE_DATA_TYPE;

typedef enum
{
    LEFT_BUD_LE_CONNECTED  = 0x01,
    RIGHT_BUD_LE_CONNECTED = 0x02,
    ALL_BUDS_LE_CONNECTED  = (LEFT_BUD_LE_CONNECTED | RIGHT_BUD_LE_CONNECTED),
} T_APP_BUD_LE_CONN_STATUS;

typedef enum
{
    DONGLE_CMD_SET_GAMING_MOE           = 0x01,
    DONGLE_CMD_REQUEST_GAMING_MOE       = 0x02,
    DONGLE_CMD_REQ_OPEN_MIC             = 0x03,
    DONGLE_CMD_SET_VOL_BALANCE          = 0x06,
    DONGLE_CMD_CTRL_RAW_DATA            = 0x08,
    DONGLE_CMD_PASS_THROUGH_DATA        = 0x10,
    DONGLE_CMD_CFU_DATA                 = 0x11,
    DONGLE_CMD_FORCE_SUSPEND            = 0x14,
    DONGLE_CMD_INTERNAL_USE             = 0x15, // rsv for internal use
    DONGLE_CMD_MIC_MUTE_CTRL            = 0x16,
    DONGLE_CMD_ERASE_FLASH              = 0xA0,
    DONGLE_CMD_WRITE_CUSTOMIZED_VP      = 0xA1,
    DONGLE_CMD_WRITE_VP_FINISH          = 0xA2,
    DONGLE_CMD_TRANS_SPK_VOL_INFO       = 0xA3,
    DONGLE_CMD_SPK_VOL_CTRL             = 0xA4,
    DONGLE_CMD_B2B_RELAY_ENGAGE_ADV     = 0xFC,
    DONGLE_CMD_B2B_RELAY_DATA           = 0xFD,
    DONGLE_CMD_SYNC_STATUS              = 0xFE,
} T_APP_DONGLE_CMD;

typedef struct
{
    T_DEVICE_BUD_SIDE bud_side;
    uint16_t len;
    uint8_t  data[0];
} T_DONGLE_RELAY_B2B_DATA;

typedef struct
{
    T_DEVICE_BUD_SIDE bud_side;
    uint8_t  addr[6];
    uint16_t adv_len;
    uint8_t  adv_data[0];
} T_DONGLE_RELAY_B2B_ADV;

typedef struct
{
    uint8_t usb_is_streaming;
    uint8_t snk_audio_loc;
    uint8_t src_conn_num;
    uint8_t silence_stream_for_a_while;
    uint8_t dongle_addr[6];
    uint8_t streaming_to_peer;
    uint8_t volume_sync_to_headset;
    T_APP_BUD_LE_CONN_STATUS bud_le_conn_status;
    uint8_t usb_host_type;
} T_DONGLE_STATUS;

typedef struct
{
    uint8_t sync;
    uint8_t type;
    uint8_t pl; /* payload len: including cmd and cmd payload */
    uint8_t cmd;
    uint8_t payload[0];
} __attribute__((packed)) T_GAMING_SYNC_HDR;

typedef struct
{
    uint8_t len;
    uint8_t payload[0];
} T_GAMING_HID_SYNC_HDR;

#define SRC_SPP_HDR_SIZE (sizeof(T_GAMING_SYNC_HDR))

typedef struct
{
    uint8_t seq;
    union
    {
        uint8_t data_byte;
        struct
        {
            uint8_t is_ack : 1;
            uint8_t flushable : 1;
            uint8_t bud_side : 2;
            uint8_t rsv : 4;
        };
    };
    uint16_t data_size;
    uint8_t payload[0];
} T_APP_CTRL_DATA;

typedef struct
{
    uint8_t cache[512];
    uint8_t header[sizeof(T_GAMING_SYNC_HDR)];
    uint8_t header_len;
    uint8_t state;
    uint16_t pos;
    uint16_t pkt_len;
} T_APP_CTRL_PKT_ASSEMBLE;

typedef void (*P_GAMING_SYNC_PKT_RCV_CB)(uint8_t *data, uint16_t len);
typedef bool (*P_GAMING_SYNC_LEGACY_PKT_SEND_CB)(uint8_t *addr, uint8_t *data, uint16_t len,
                                                 bool flushable);
typedef bool (*P_GAMING_SYNC_LE_PKT_SEND_CB)(uint8_t link_id, uint8_t *data, uint16_t len);
typedef void (*P_GAMING_SYNC_LEGACY_CMD_CB)(uint8_t *addr, uint8_t *data, uint16_t len);
typedef void (*P_GAMING_SYNC_LE_CMD_CB)(uint8_t *data, uint16_t len);

uint16_t app_gaming_sync_get_data_len_for_streaming(void);
bool app_gaming_set_sync_data_in_streaming(void);
void app_gaming_sync_append_data_to_streaming(uint8_t *start_offset, uint16_t len);
void app_gaming_sync_disassemble_data(uint8_t *addr, uint8_t *buf, uint16_t len);
void app_gaming_sync_set_le_link_id(uint8_t link_id);
void app_gaming_sync_set_link_connected(bool connected, uint8_t *addr);
void app_gaming_sync_le_send_register(P_GAMING_SYNC_LE_PKT_SEND_CB cb);
void app_gaming_sync_legacy_send_register(P_GAMING_SYNC_LEGACY_PKT_SEND_CB cb);
void app_gaming_sync_rcv_register(P_GAMING_SYNC_PKT_RCV_CB cb);
void app_gaming_sync_legacy_cmd_register(P_GAMING_SYNC_LEGACY_CMD_CB cb);
void app_gaming_sync_le_cmd_register(P_GAMING_SYNC_LE_CMD_CB cb);
bool app_gaming_sync_data_send(uint8_t *data, uint16_t size, bool flushable);
void app_gaming_sync_init(bool append_to_streaming, uint16_t max_data_len);


extern T_DONGLE_STATUS dongle_status;
extern T_HEADSET_STATUS headset_status;

#endif
