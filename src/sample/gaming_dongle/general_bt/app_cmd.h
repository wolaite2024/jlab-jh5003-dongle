/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CMD_H_
#define _APP_CMD_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_CMD App Cmd
  * @brief App Cmd
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_CMD_Exported_Macros App Cmd Macros
   * @{
   */
#define CMD_SET_VER_MAJOR                   0x01
#define CMD_SET_VER_MINOR                   0x01

#define CMD_SYNC_BYTE                       0xAA

#define CMD_PATH_UART                       0x00
#define CMD_PATH_SPP                         0x01
#define CMD_PATH_LE                         0x02
#define CMD_PATH_IAP                        0x03

#define GET_STATUS_RWS_STATE            0x00
#define GET_STATUS_RWS_CHANNEL          0x01
#define GET_STATUS_BATTERY_STATUS       0x02
#define GET_STATUS_AMB_STATUS           0x03

//for CMD_GET_FLASH_DATA and EVENT_REPORT_FLASH_DATA
#define START_TRANS                 0x00
#define CONTINUE_TRANS              0x01
#define SUPPORT_IAMGE_TYPE          0x02

#define TRANS_DATA_INFO             0x00
#define CONTINUE_TRANS_DATA         0x01
#define END_TRANS_DATA              0x02
#define SUPPORT_IAMGE_TYPE_INFO     0x03

#define SYSTEM_CONFIG               0x00
#define ROM_PATCH_IMAGE             0x01
#define APP_IMAGE                   0x02
#define DSP_SYSTEM_IMAGE            0x03
#define DSP_APP_IMAGE               0x04
#define FTL_DATA                    0x05
#define ANC_IMAGE                   0x06
#define LOG_PARTITION               0x07
#define CORE_DUMP_PARTITION         0x08

#define FLASH_ALL                   0xFF
#define ALL_DUMP_IAMGE_MASK         ((0x01 << SYSTEM_CONFIG) | (0x01 << ROM_PATCH_IMAGE) | (0x01 << APP_IMAGE) \
                                     | (0x01 << DSP_SYSTEM_IMAGE) | (0x01 << DSP_APP_IMAGE) \
                                     | (0x01 << FTL_DATA) |(0x01 << CORE_DUMP_PARTITION))

/** End of APP_DEVICE_Exported_Macros
    * @}
    */
/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_CMD_Exported_Types App Cmd Types
  * @{
  */
/**  @brief  embedded uart, spp or le vendor command type.
  *    <b> Only <b> valid when BT SOC connects to external MCU via data uart, spp or le.
  *    refer to SDK audio sample code for definition
  */
typedef enum
{
    CMD_ACK                             = 0x0000,
    CMD_BT_READ_PAIRED_RECORD           = 0x0001,
    CMD_BT_CREATE_CONNECTION            = 0x0002,
    CMD_BT_DISCONNECT                   = 0x0003,
    CMD_MMI                             = 0x0004,
    CMD_LEGACY_DATA_TRANSFER            = 0x0005,
    CMD_ASSIGN_BUFFER_SIZE              = 0x0006,
    CMD_BT_READ_LINK_INFO               = 0x0007,
    CMD_TONE_GEN                        = 0x0008,
    CMD_BT_GET_REMOTE_NAME              = 0x0009,
    CMD_BT_IAP_LAUNCH_APP               = 0x000A,
    CMD_TTS                             = 0x000B,
    CMD_INFO_REQ                        = 0x000C,

    CMD_DAC_GAIN_CTRL                   = 0x000F,
    CMD_ADC_GAIN_CTRL                   = 0x0010,
    CMD_BT_SEND_AT_CMD                  = 0x0011,
    CMD_SET_CONFIGURATION               = 0x0012,
    CMD_INDICATION                      = 0x0013,
    CMD_LINE_IN_CTRL                    = 0x0014,
    CMD_LANGUAGE_GET                    = 0x0015,
    CMD_LANGUAGE_SET                    = 0x0016,
    CMD_GET_CONFIGURATION               = 0x0017,

    CMD_GET_STATUS                      = 0x0018,

    CMD_BT_HFP_DIAL_WITH_NUMBER         = 0x001B,
    CMD_GET_BD_ADDR                     = 0x001C,
    CMD_STRING_MODE                     = 0x001D,

    CMD_LE_START_ADVERTISING            = 0x0100,
    CMD_LE_STOP_ADVERTISING             = 0x0101,
    CMD_LE_DATA_TRANSFER                = 0x0102,
    CMD_LE_START_SCAN                   = 0x0103,
    CMD_LE_STOP_SCAN                    = 0x0104,

    CMD_ANCS_REGISTER                   = 0x0110,
    CMD_ANCS_GET_NOTIFICATION_ATTR      = 0x0111,
    CMD_ANCS_GET_APP_ATTR               = 0x0112,
    CMD_ANCS_PERFORM_NOTIFICATION_ACTION = 0x0113,

    CMD_AUDIO_EQ_QUERY                  = 0x0200,
    CMD_AUDIO_EQ_ENABLE                 = 0x0201,
    CMD_AUDIO_EQ_DISABLE                = 0x0202,
    CMD_AUDIO_EQ_PARAM_SET              = 0x0203,
    CMD_AUDIO_EQ_PARAM_GET              = 0x0204,
    CMD_AUDIO_EQ_INDEX_SET              = 0x0205,
    CMD_AUDIO_EQ_INDEX_GET              = 0x0206,

    CMD_BT_GET_LOCAL_ADDR               = 0x0302,
    CMD_BT_BOND_INFO_CLEAR              = 0x0309,
    CMD_GET_FLASH_DATA                  = 0x030D,
    CMD_MIC_SWITCH                      = 0x030E,
    CMD_SWITCH_TO_HCI_DOWNLOAD_MODE     = 0x0310,

    CMD_OTA_DEV_INFO                    = 0x0600,
    CMD_OTA_ACTIVE_BANK_VER             = 0x0601,
    CMD_OTA_START                       = 0x0602,
    CMD_OTA_PACKET                      = 0x0603,
    CMD_OTA_VALID                       = 0x0604,
    CMD_OTA_RESET                       = 0x0605,
    CMD_OTA_ACTIVE_RESET                = 0x0606,
    CMD_OTA_BUFFER_CHECK_ENABLE         = 0x0607,
    CMD_OTA_BUFFER_CHECK                = 0x0608,
    CMD_OTA_IMG_INFO                    = 0x0609,
    CMD_OTA_SECTION_SIZE                = 0x060A,
    CMD_OTA_DEV_EXTRA_INFO              = 0x060B,
    CMD_OTA_PROTOCOL_TYPE               = 0x060C,
    CMD_OTA_GET_RELEASE_VER             = 0x060D,
    CMD_OTA_INACTIVE_BANK_VER           = 0x060E,
    CMD_OTA_COPY_IMG                    = 0x060F,
    CMD_OTA_CHECK_SHA256                = 0x0610,


    CMD_VENDOR_SEPC                     = 0x0800, //It has been reserved for vendor customer A, please dont't use this value.

    CMD_DFU_START                       = 0x0900,

    //for HCI command
    CMD_HCI = 0x0B00,                   //0x0B00
    CMD_WDG_RESET,

    //for ANC command
    CMD_ANC_TEST_MODE = 0x0C00,                  //0x0C00
    CMD_ANC_WRITE_GAIN,
    CMD_ANC_READ_GAIN,
    CMD_ANC_BURN_GAIN,
    CMD_ANC_COMPARE,
    CMD_ANC_GEN_TONE,
    CMD_ANC_CONFIG_DATA_LOG,
    CMD_ANC_READ_DATA_LOG,
    CMD_ANC_READ_REGISTER  = 0x0C0A,
    CMD_ANC_WRITE_REGISTER = 0x0C0B,

    CMD_MP_TEST                         = 0x0F00,
} T_CMD_ID;

/** @brief  packet type for legacy transfer*/
typedef enum t_pkt_type
{
    PKT_TYPE_SINGLE = 0x00,
    PKT_TYPE_START = 0x01,
    PKT_TYPE_CONT = 0x02,
    PKT_TYPE_END = 0x03
} T_PKT_TYPE;


/**  @brief CMD Set Info Request type. */
typedef enum
{
    CMD_SET_INFO_TYPE_VERSION = 0x00,
} T_CMD_SET_INFO_TYPE;

/**  @brief  phone send this cmd to soc.*/
typedef enum
{
    TTS_SESSION_OPEN = 0x00,
    TTS_SESSION_PAUSE = 0x01,
    TTS_SESSION_RESUME = 0x02,
    TTS_SESSION_ABORT = 0x03,
    TTS_SESSION_CLOSE = 0x04,
    TTS_SESSION_SEND_SINGLE_FRAME = 0x05,
    TTS_SESSION_SEND_START_FRAME = 0x06,
    TTS_SESSION_SEND_CONTINUE_FRAME = 0x07,
    TTS_SESSION_SEND_END_FRAME = 0x08
} T_TTS_SESSION_CMD_TYPE;

/**  @brief  soc send this event to phone.*/
typedef enum
{
    TTS_SESSION_OPEN_REQ = 0x00,
    TTS_SESSION_PAUSE_REQ = 0x01,
    TTS_SESSION_RESEME_REQ = 0x02,
    TTS_SESSION_ABORT_REQ = 0x03,
    TTS_SESSION_CLOSE_REQ = 0x04,
    TTS_SESSION_SEND_ENCODE_DATA = 0x05
} T_TTS_SESSION_EVENT_TYPE;

/**  @brief  caller id type
  */
typedef enum
{
    CALLER_ID_NUMBER = 0x00,
    CALLER_ID_NAME = 0x01
} T_CALLER_ID_TYPE;

/**
 * \brief Audio EQ query type
 */
typedef enum
{
    AUDIO_EQ_QUERY_STATE    = 0x00,
    AUDIO_EQ_QUERY_NUM      = 0x01,
} T_AUDIO_EQ_QUERY_TYPE;

/**
 * \brief Audio EQ sample rate
 */
typedef enum
{
    AUDIO_EQ_SAMPLE_RATE_44_1KHZ    = 0x03,
    AUDIO_EQ_SAMPLE_RATE_48KHZ      = 0x04,
} T_AUDIO_EQ_SAMPLE_RATE;

/**  @brief  Audio EQ Frame Type.*/
typedef enum
{
    AUDIO_EQ_FRAME_SINGLE = 0x00,
    AUDIO_EQ_FRAME_START = 0x01,
    AUDIO_EQ_FRAME_CONTINUE = 0x02,
    AUDIO_EQ_FRAME_END = 0x03,
    AUDIO_EQ_FRAME_ABORT = 0x04
} T_AUDIO_EQ_FRAME_TYPE;

/**  @brief  cmd set status to phone
  */
typedef enum
{
    CMD_SET_STATUS_COMPLETE = 0x00,
    CMD_SET_STATUS_DISALLOW = 0x01,
    CMD_SET_STATUS_UNKNOW_CMD = 0x02,
    CMD_SET_STATUS_PARAMETER_ERROR = 0x03,
    CMD_SET_STATUS_BUSY = 0x04,
    CMD_SET_STATUS_PROCESS_FAIL = 0x05,
    CMD_SET_STATUS_SCENARIO_ERROR = 0x0A,
} T_AU_CMD_SET_STATUS;

typedef struct
{
    uint32_t flash_data_start_addr_tmp;
    uint32_t flash_data_start_addr;
    uint32_t flash_data_size;
    uint8_t flash_data_type;
} T_FLASH_DATA;
/** End of APP_CMD_Exported_Types
    * @}
    */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_CMD_Exported_Functions App Cmd Functions
    * @{
    */
/**
    * @brief  App process uart or embedded spp vendor command.
    * @param  cmd_ptr command type
    * @param  cmd_len command length
    * @param  cmd_path command path use for distinguish uart,or le,or spp channel.
    * @param  rx_seqn recieved command sequence
    * @param  app_idx received rx command device index
    * @return void
    */
void app_handle_cmd_set(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t rx_seqn,
                        uint8_t app_idx);

/**
    * @brief  send event packet via br link or le link.
    * @param  event_id @ref T_EVENT_ID
    * @param  buf content in the event packet
    * @param  len buf length.
    * @return void
    */
void app_cmd_set_event_broadcast(uint16_t event_id, uint8_t *buf, uint16_t len);

/* @brief  app cmd init
*
* @param  void
* @return none
*/
void app_cmd_init(void);

/** @} */ /* End of group APP_CMD_Exported_Functions */
/** End of APP_CMD
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_CMD_H_ */
