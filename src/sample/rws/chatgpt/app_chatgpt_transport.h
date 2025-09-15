/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CHATGPT_TRANSPORT_H_
#define _APP_CHATGPT_TRANSPORT_H_

#define TRANS_HEADER_MAGIC   (0xAB)
#define TRANS_HEADER_VERSION (0x00)

#define TRANS_CHAT_VOICE_ID   0x0F02
#define TRANS_CHAT_BUFF_CTRL  0x0F0B

typedef enum
{
    TRANS_RX_TYPE_NULL                = 0x00,
    TRANS_RX_TYPE_SEPARATE_DATA       = 0x01,
    TRANS_RX_TYPE_MAGIC_ERR           = 0x02,
    TRANS_RX_TYPE_ACK                 = 0x03,
    TRANS_RX_TYPE_LEN_ERR             = 0x04,
    TRANS_RX_TYPE_FULL                = 0x05,
    TRANS_RX_TYPE_SEPARATE            = 0x06,
} CHATGPT_TRANS_RX_TYPE;

typedef struct
{
    uint8_t magic;

    uint8_t version: 4;
    uint8_t ack: 1;
    uint8_t err: 1;
    uint8_t rsv: 2;

    uint8_t len_high;
    uint8_t len_low;
    uint8_t crc_high;
    uint8_t crc_low;
    uint8_t seq_high;
    uint8_t seq_low;
} T_CHATGPT_TRANS_L1_HEADER;

typedef struct
{
    uint8_t opcode;
    uint8_t version: 4;
    uint8_t rsv: 4;
    uint8_t key;
    uint8_t len_high;
    uint8_t len_low;
} T_CHATGPT_TRANS_L2_HEADER;

typedef struct
{
    uint8_t is_rec;
    uint16_t total_len;
    uint16_t remain_len;
    uint16_t offset;
    uint8_t *buff;
} T_CHATGPT_TRANS_SEPARATE;

typedef enum
{
    FIRMWARE_UPDATE_CMD_ID       = 0x01,
    SET_CONFIG_TRANSAND_ID       = 0x02,
    BOND_TRANSAND_ID             = 0x03,
    NOTIFY_TRANSAND_ID           = 0x04,
    HEALTH_DATA_TRANSAND_ID      = 0x05,
    FACTORY_TEST_TRANSAND_ID     = 0x06,
    CONTROL_TRANSAND_ID          = 0X07,
    BLUETOOTH_LOG_TRANSAND_ID    = 0x0A,
    WEATHER_INFORMATION_ID       = 0X0B,
    TRANS_CMD_CHATGPT            = 0x0F,
    GET_STACK_DUMP               = 0x10,
    TEST_FLASH_READ_WRITE        = 0xFE,
    TEST_TRANSAND_ID             = 0xFF,
} T_CHATGPT_TRANS_CMD;

#define TRANS_L1_HEADER_LEN      sizeof(T_CHATGPT_TRANS_L1_HEADER)
#define TRANS_L2_HEADER_LEN      sizeof(T_CHATGPT_TRANS_L2_HEADER)
#define TRANS_CHAT_ACK_LEN       sizeof(T_CHATGPT_TRANS_L1_HEADER)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool app_chatgpt_transport_head_check(uint8_t *data);
void app_chatgpt_transport_transmit(uint8_t key, uint8_t *data, uint16_t len);
void app_chatgpt_transport_receive(uint8_t *data, uint16_t len);
uint16_t app_chatgpt_transport_get_seq(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
