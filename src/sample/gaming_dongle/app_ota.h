/**
*****************************************************************************************
*     Copyright(c) 2019, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_ota.h
   * @brief     Head file for using OTA service
   * @author    Michael
   * @date      2019-11-25
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2019 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/
#ifndef _APP_OTA_H_
#define _APP_OTA_H_

#include "patch_header_check.h"
#include "app_relay.h"
/** @defgroup  APP_OTA_SERVICE APP OTA handle
    * @brief APP OTA Service to implement OTA feature
    * @{
    */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_OTA_SERVICE_Exported_Macros App OTA service Exported Macros
    * @brief
    * @{
    */
#define SPP_PROTOCOL_INFO      0x0011
#define BLE_PROTOCOL_INFO      0x0010

/*ota version num*/
#define BLE_OTA_VERSION     0x4
#define SPP_OTA_VERSION     0x6

/** @brief  spp ota command length. */
#define OTA_LENGTH_OTA_GET_INFO         0
#define OTA_LENGTH_OTA_GET_IMG_VER      1
#define OTA_LENGTH_START_OTA            16
#define OTA_LENGTH_VALID_FW             3
#define OTA_LENGTH_IMAGE_INFO           2
#define OTA_LENGTH_BUFFER_CHECK_EN      0
#define OTA_LENGTH_BUFFER_CRC           4
#define OTA_LRNGTH_RWS_INFO             0
#define OTA_LRNGTH_OTHER_INFO           0
#define OTA_LENGTH_SECTION_SIZE         0

/** @brief image exit indicator. */
#define IMAGE_LOCATION_BANK0        1
#define IMAGE_LOCATION_BANK1        2
#define NOT_SUPPORT_BANK_SWITCH     3

/*bit set of device info data2*/
#define OTA_DEVICE_FEATURE_SUPPORT_BUFFER_CHECK     (1 << 0)
#define OTA_DEVICE_FEATURE_ENABLE_AES_ENCRYPT       (1 << 1)
#define OTA_DEVICE_FEATURE_ENCRYPT_MODE_16N         (1 << 2)
#define OTA_DEVICE_FEATURE_SUPPORT_MULTIIMAGE       (1 << 3)

#define DFU_OPCODE_COPY_IMG                 0x0c
#define DFU_OPCODE_GET_IMAGE_VER            0x0d
#define DFU_OPCODE_GET_SECTION_SIZE         0x0e
#define DFU_OPCODE_CHECK_SHA256             0x0f
#define DFU_OPCODE_GET_RELEASE_VER          0x10
#define DFU_OPCODE_TEST_EN                  0x11

#define UINT_4K             4096
#define MAX_BUFFER_SIZE     4096
#define MTU_SIZE            512
#define DATA_PADDING        4
#define OTA_HEADER_SIZE     0x1000
#define CRC_OFFSET          6
#define CRC_LEN             2
#define SHA256_OFFSET       372
#define RELEASE_VER_OFFSET  (SHA256_OFFSET + 32)
#define SHA256_LEN          32
#define BAT_30_PERCENT      30
#define FLASH_SECTOR_SIZE   0x1000
#define FLASH_OFFSET_TO_NO_CACHE    0x01000000
#define FLASH_TABLE_MAGIC_PATTERN   0x5a5a12a5
#define READBACK_BUFFER_SIZE    64
#define BTAON_FAST_AON_GPR_15   0x1a2

#define DEVICE_INFO_LEN     12
#define MAX_IMG_NUM         15
#define IMG_INFO_LEN        MAX_IMG_NUM * 6 + 2
#define CHECK_SHA256_LEN    MAX_IMG_NUM * 3
#define TARGET_INFO_LEN     9
#define BUFFER_CHECK_LEN    5
#define EXTRA_INFO_LEN      6
#define RELEASE_VER_LEN     6
#define SPEC_VER_LEN        3

#define UPDATE_FAIL         0
#define UPDATE_SUCCESS      1

#define ACTIVE_BANK         0
#define INACTIVE_BANK       1

#define CHECK_SHA256_FAIL      0

#define FORCE_OTA_TEMP  1
/** End of APP_OTA_SERVICE_Exported_Macros
    * @}
    */


/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_OTA_SERVICE_Exported_Types APP OTA Service Exported Types
    * @brief
    * @{
    */
/** @brief  OTA timer callback */
typedef enum
{
    OTA_TIMER_DELAY_RESET,
    OTA_TIMER_TOTAL_TIME,
    OTA_TIMER_IMAGE_TRANS,
    OTA_TIMER_RWS_SYNC,
    OTA_TIMER_TOTAL,
} T_OTA_TIMER;

/** @brief  OTA mode */
typedef enum
{
    NONE,
    BLE_OTA_MODE,
    SPP_OTA_MODE,
} T_OTA_MODE;

/** @brief  rws ota sync cmd */
typedef enum
{
    RWS_OTA_ACK,
    RWS_OTA_UPDATE_RET,
    RWS_OTA_HANDSHAKE,
    RWS_OTA_MAX,
} T_RWS_OTA_CMD;

typedef enum
{
    SINGLE_DEFAULT,
    SINGLE_B2B_DISC,
    RWS_B2B_CONNECT,
} T_RWS_MODE;

typedef enum
{
    OTA_SUCCESS_REBOOT,
    OTA_SPP_DISC,
    OTA_BLE_DISC,
    OTA_IMAGE_TRANS_TIMEOUT,
    OTA_IMAGE_TOTAL_TIMEOUT,
    OTA_RESET_CMD,
    OTA_B2B_DISC,
    OTA_OTHBUD_FAIL,
} T_CLEAR_CAUSE;

typedef struct
{
    uint8_t ota_mode;
    uint8_t cmd_id;
    uint8_t data;
} RWS_MESSAGE_FORMAT;

typedef union
{
    uint8_t value;
    struct
    {
        uint8_t cur_bud: 1 ;
        uint8_t oth_bud: 1 ; /* data */
    } ;
} VALID_RET;

typedef struct
{
    VALID_RET valid_ret;
    uint8_t retry_times;
    RWS_MESSAGE_FORMAT msg;
} OTA_RWS;

typedef union
{
    uint8_t value;
    struct
    {
        uint8_t buffer_check_en: 1;
        uint8_t is_ota_process: 1;
        uint8_t is_rws: 1;
        uint8_t roleswap: 1;
        uint8_t is_devinfo: 1;
        uint8_t skip_flag: 1;
        uint8_t RSVD: 2;
    };
} OTA_FLAG;

typedef union
{
    uint8_t value;
    struct
    {
        uint8_t t_aes_en: 1;
        uint8_t t_stress_test: 1;
        uint8_t t_copy_fail: 1;
        uint8_t t_skip_fail: 1;
        uint8_t t_buffercheck_disable: 1;
        uint8_t rsvd: 3;
    };
} TEST_ENABLE;


/** @brief  Table used to store Extended Device Information */
typedef struct
{
    void *timer_handle_ota_reset;
    void *timer_handle_ota_total_time;
    void *timer_handle_ota_transfer;
    void *timer_handle_ota_rws_sync;
    uint8_t *p_ota_temp_buf_head;
    uint32_t image_indicator;
    uint32_t image_total_length;
    uint32_t cur_offset;
    uint32_t next_subimage_offset;
    uint16_t image_id;
    uint16_t ota_temp_buf_used_size;
    uint16_t buffer_size;
    uint8_t bd_addr[6];
    uint8_t bp_level;
    uint8_t ota_timer_queue_id;
    uint8_t force_temp_mode;
    OTA_RWS rws_mode;
    OTA_FLAG ota_flag;
    TEST_ENABLE test;
} OTA_FUNCTION_STRUCT;

typedef struct
{
    IMG_ID img_id;
    uint8_t sha256[32];
} SHA256_CHECK;

typedef struct
{
    uint16_t img_id;
    uint8_t data[4];
} IMG_INFO;

typedef union
{
    uint8_t value;
    struct
    {
        uint8_t buffercheck_en: 1;
        uint8_t aes_en: 1;
        uint8_t aes_mode: 1; /*1:16*N bytes, 0:first 16byte*/
        uint8_t support_multiimage: 1;
        uint8_t rsvd: 4;
    };
} DEVICE_INFO_MODE;

typedef union
{
    uint8_t value;
    struct
    {
        uint8_t b2b_status: 2;
        uint8_t fc_bud_role: 2;
        uint8_t rsvd: 4;
    };
} DEVICE_INFO_BUD_STATUS;

typedef union
{
    uint8_t value[12];
    struct
    {
        uint8_t ic_type;
        uint8_t spec_ver;
        DEVICE_INFO_MODE mode;
        DEVICE_INFO_BUD_STATUS status;
        uint8_t ota_temp_size;
        uint8_t banknum;
        uint16_t mtu_size;
        uint8_t rsvd[4];
    };
} DEVICE_INFO;

typedef union
{
    uint32_t value;
    struct
    {
        uint32_t enc_en: 1;
        uint32_t sha256_en: 1;
        uint32_t link_loss_stop: 1;
        uint32_t rsvd: 28;
    };
} OTA_SETTING;
/** End of APP_OTA_SERVICE_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_OTA_SERVICE_Exported_Functions APP OTA service Functions
    * @brief
    * @{
    */
/**
    * @brief  The main function to handle all the spp ota command
    * @param  length length of command id and data
    * @param  p_value data addr
    * @param  app_idx received rx command device index
    * @return void
    */
void app_ota_cmd_handle(uint8_t path, uint16_t length, uint8_t *p_value, uint8_t app_idx);
/**
    * @brief  handle the active reset event ack
    * @param  event_id  the ack is which event's
    * @param  status  the status of the ack
    * @return void
    */
void app_ota_cmd_ack_handle(uint16_t event_id, uint8_t status);

/**
    * @brief    Handle written request on DFU packet characteristic
    * @param    conn_id     ID to identify the connection
    * @param    length      Length of value to be written
    * @param    p_value     Value to be written
    * @return   T_APP_RESULT
    * @retval   Handle result of this request
    */
T_APP_RESULT app_ota_ble_handle_packet(uint8_t conn_id, uint16_t length, uint8_t *p_value);

/**
    * @brief    Handle written request on DFU control point characteristic
    * @param    conn_id     ID to identify the connection
    * @param    length      Length of value to be written
    * @param    p_value     Value to be written
    * @return   T_APP_RESULT
    * @retval   Handle result of this request
    */
T_APP_RESULT app_ota_ble_handle_cp_req(uint8_t conn_id, uint16_t length, uint8_t *p_value);

/**
    * @brief  ota callback register
    * @param  void
    * @return void
    */
void app_ota_init(void);

/**
    * @brief    Used to get device information
    * @param    info_length    point of image length
    * @return   address of the device info buffer
    */
void app_ota_get_device_info(DEVICE_INFO *p_data);

/**
    * @brief    Used to get image section size
    * @param    *info_length  point of the buffer
    * @return   addres of the buffer
    */
void app_ota_get_section_size(uint8_t *p_data);

/**
    * @brief    Used to get image version
    * @param    *table_length   point of buffer length
    * @return   address of the buffer
    */
void app_ota_get_image_version(uint8_t *p_data, uint8_t bank);

/**
    * @brief  get image size of bank area
    * @param  image_id image ID
    * @return size
    */
uint32_t get_bank_size_by_img_id(IMG_ID image_id);

/**
    * \brief  Handle remote msg from remote device.
    * \param[in] msg   Message type.
    * \param[in] buf   Message buffer.
    * \param[in] buf   Message buffer.
    */
void app_ota_handle_remote_msg(T_APP_REMOTE_MSG msg, void *buf, uint16_t len);

void app_ota_le_disconnect_cb(uint8_t conn_id, uint8_t local_disc_cause, uint16_t disc_cause);

uint8_t app_ota_get_active_bank(void);
/**
    * @brief  get ota status
    * @return True:is doing ota; False: is not doing ota
    */
bool app_ota_dfu_is_busy(void);

/**
    * @brief  Reset local variables and inactive bank ota header
    * @return void
    */
void app_ota_error_clear_local(uint8_t cause);

/**
    * @brief  link loss handle flag
    * @return True:is doing ota; False: is not doing ota
    */
bool app_ota_link_loss_stop(void);
/** @} */ /* End of group APP_OTA_SERVICE_Exported_Functions */

/** @} */ /* End of group APP_OTA_SERVICE */
#endif
