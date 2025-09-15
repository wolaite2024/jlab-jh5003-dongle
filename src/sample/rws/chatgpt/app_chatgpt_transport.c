/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if F_APP_CHATGPT_SUPPORT
#include "app_main.h"
#include "app_chatgpt.h"
#include "app_transfer.h"
#include "app_cmd.h"

static uint16_t transport_seq = 0;
static T_CHATGPT_TRANS_SEPARATE transport_separate = {0};

static void app_chatgpt_transport_ack(uint8_t err, uint16_t seq)
{
    uint8_t *buf;
    T_CHATGPT_TRANS_L1_HEADER l1_header_ack = {0};

    buf = malloc(TRANS_L1_HEADER_LEN);

    if (buf == NULL)
    {
        return;
    }

    l1_header_ack.magic = TRANS_HEADER_MAGIC;
    l1_header_ack.version = TRANS_HEADER_VERSION;
    l1_header_ack.err = err;
    l1_header_ack.ack = 1;
    l1_header_ack.seq_high = (seq >> 8) & 0xff;
    l1_header_ack.seq_low = (seq >> 0) & 0xff;

    memcpy(buf, (uint8_t *)&l1_header_ack, TRANS_L1_HEADER_LEN);

    if (app_transfer_push_data_queue(CMD_PATH_LE, buf,  TRANS_L1_HEADER_LEN, 0) == false)
    {
        free(buf);
    }
}

static void app_chatgpt_transport_receive_parse(uint8_t *data, uint16_t len)
{
    T_CHATGPT_TRANS_L1_HEADER l1_header = {0};
    T_CHATGPT_TRANS_L2_HEADER l2_header = {0};
    uint8_t ret = 0;
    uint16_t seq = 0, crc16 = 0;
    uint8_t need_ack = 1;

    if (data[0] != TRANS_HEADER_MAGIC)
    {
        ret = 1;
        goto transport_receive_parse;
    }

    if (len < TRANS_L1_HEADER_LEN)
    {
        ret = 2;
        goto transport_receive_parse;
    }

    memcpy((uint8_t *)&l1_header, data, TRANS_L1_HEADER_LEN);

    if (len < (TRANS_L1_HEADER_LEN + TRANS_L2_HEADER_LEN))
    {
        ret = 3;
        goto transport_receive_parse;
    }

    memcpy((uint8_t *)&l2_header, data + TRANS_L1_HEADER_LEN, TRANS_L2_HEADER_LEN);

    seq = (l1_header.seq_high << 8) + l1_header.seq_low;
    // data_len = (l2_header.len_high << 8) + l1_header.len_low;
    crc16 = (l1_header.crc_high << 8) + l1_header.crc_low;
    if (crc16 != btxfcs(0, data + TRANS_L1_HEADER_LEN, len - TRANS_L1_HEADER_LEN))
    {
        ret = 4;
        goto transport_receive_parse;
    }

    switch (l2_header.opcode)
    {
    case TRANS_CMD_CHATGPT:
        {
            app_chatgpt_process_event_handle(data + TRANS_L1_HEADER_LEN, len - TRANS_L1_HEADER_LEN,
                                             &need_ack);
        }
        break;

    default:
        {
            ret = 5;
        }
        break;
    }

transport_receive_parse:
    APP_PRINT_INFO3("app_chatgpt_transport_receive_parse: len %d ret %d data %b", len, ret,
                    TRACE_BINARY(len,
                                 data));

    if (need_ack != 0)
    {
        app_chatgpt_transport_ack((ret == 0) ? 0 : 1, seq);
    }
}

static void app_transport_separate_reset(void)
{
    free(transport_separate.buff);
    memset((uint8_t *)&transport_separate, 0, sizeof(T_CHATGPT_TRANS_SEPARATE));
}

static void app_chatgpt_transport_head_load(T_CHATGPT_TRANS_L1_HEADER *header, uint16_t crc16,
                                            uint16_t len,
                                            uint16_t seq)
{
    header->magic = TRANS_HEADER_MAGIC;
    header->version = TRANS_HEADER_VERSION;

    header->len_high = (len >> 8) & 0xff;
    header->len_low = (len >> 0) & 0xff;
    header->crc_high = (crc16 >> 8) & 0xff;
    header->crc_low = (crc16 >> 0) & 0xff;
    header->seq_high = (seq >> 8) & 0xff;
    header->seq_low = (seq >> 0) & 0xff;
}

static void app_transport_separate_check(uint8_t *data, uint16_t data_len, uint16_t len)
{
    uint8_t ret = 0;
    transport_separate.is_rec = true;
    transport_separate.total_len = data_len + TRANS_L1_HEADER_LEN;

    if (transport_separate.buff != NULL)
    {
        ret = 1;
        goto  transport_separate_check;
    }

    transport_separate.buff = malloc(transport_separate.total_len);

    if (transport_separate.buff == NULL)
    {
        ret = 2;
        goto  transport_separate_check;
    }

    memcpy(transport_separate.buff, data, len);
    transport_separate.offset = len;
    transport_separate.remain_len = transport_separate.total_len - len;

transport_separate_check:
    APP_PRINT_INFO4("app_transport_separate_check: total_len %d offset %d remain_len %d ret %d",
                    transport_separate.total_len, transport_separate.offset, transport_separate.remain_len, ret);

    if (ret != 0)
    {
        app_transport_separate_reset();
    }
}

static void app_transport_separate_receive(uint8_t *data, uint16_t len)
{
    uint8_t ret = 0;
    if (len > transport_separate.remain_len)
    {
        ret = 1;
        goto  transport_separate_receive;
    }

    if (transport_separate.buff == NULL)
    {
        ret = 2;
        goto  transport_separate_receive;
    }

    memcpy(transport_separate.buff + transport_separate.offset, data, len);
    transport_separate.offset += len;
    transport_separate.remain_len -= len;

    if (transport_separate.remain_len == 0)
    {
        app_chatgpt_transport_receive_parse(transport_separate.buff, transport_separate.total_len);
    }

transport_separate_receive:
    APP_PRINT_INFO4("app_transport_separate_receive: total_len %d offset %d remain_len %d ret %d",
                    transport_separate.total_len, transport_separate.offset, transport_separate.remain_len, ret);

    if ((transport_separate.remain_len == 0) || (ret != 0))
    {
        app_transport_separate_reset();
    }
}

uint16_t app_chatgpt_transport_get_seq(void)
{
    return (transport_seq + 1);
}

bool app_chatgpt_transport_head_check(uint8_t *data)
{
    if ((data[0] == TRANS_HEADER_MAGIC) && (data[1] == TRANS_HEADER_VERSION))
    {
        return true;
    }

    return false;
}

void app_chatgpt_transport_transmit(uint8_t key, uint8_t *data, uint16_t len)
{
    uint8_t *buf;
    T_CHATGPT_TRANS_L1_HEADER l1_header = {0};
    uint16_t crc16 = btxfcs(0, data, len);

    buf = malloc(len + TRANS_L1_HEADER_LEN);

    if (buf == NULL)
    {
        return;
    }

    transport_seq++;
    app_chatgpt_transport_head_load(&l1_header, crc16, len, transport_seq);

    memcpy(buf, (uint8_t *)&l1_header, TRANS_L1_HEADER_LEN);
    memcpy(&buf[TRANS_L1_HEADER_LEN], data, len);

    if (app_transfer_push_data_queue(CMD_PATH_LE, buf, (len + TRANS_L1_HEADER_LEN),
                                     app_chatgpt_ble_get_idx()) == false)
    {
        free(buf);
    }
}

void app_chatgpt_transport_receive(uint8_t *data, uint16_t len)
{
    T_CHATGPT_TRANS_L1_HEADER l1_header = {0};
    uint16_t seq = 0, data_len = 0;
    CHATGPT_TRANS_RX_TYPE rx_type = TRANS_RX_TYPE_NULL;

    if (transport_separate.is_rec)
    {
        if ((data[0] != TRANS_HEADER_MAGIC) || (len < TRANS_L1_HEADER_LEN))
        {
            {
                app_transport_separate_receive(data, len);

                rx_type = TRANS_RX_TYPE_SEPARATE_DATA;
                goto transport_receive;
            }
        }
    }

    if (len >= TRANS_L1_HEADER_LEN)
    {
        memcpy((uint8_t *)&l1_header, data, TRANS_L1_HEADER_LEN);
    }

    if (l1_header.magic != TRANS_HEADER_MAGIC)
    {
        rx_type = TRANS_RX_TYPE_MAGIC_ERR;
        goto transport_receive;
    }

    seq = (l1_header.seq_high << 8) + l1_header.seq_low;

    if ((len == TRANS_L1_HEADER_LEN) && (l1_header.ack == 1))
    {
        if (l1_header.err == 0)
        {
            app_transfer_queue_recv_ack_check(seq, CMD_PATH_LE);
            app_chatgpt_process_ack_check(seq);
        }

        rx_type = TRANS_RX_TYPE_ACK;
        goto transport_receive;
    }

    if (len < (TRANS_L1_HEADER_LEN + TRANS_L2_HEADER_LEN))
    {
        rx_type = TRANS_RX_TYPE_LEN_ERR;
        goto transport_receive;
    }

    data_len = (l1_header.len_high << 8) + l1_header.len_low;

    if (data_len > (len - TRANS_L1_HEADER_LEN))
    {
        app_transport_separate_check(data, data_len, len);
        rx_type = TRANS_RX_TYPE_SEPARATE;
    }
    else
    {
        app_transport_separate_reset();
        app_chatgpt_transport_receive_parse(data, len);
        rx_type = TRANS_RX_TYPE_FULL;
    }

transport_receive:
    APP_PRINT_INFO2("app_chatgpt_transport_receive: rx_type %d len %d ", rx_type, len);
}
#endif
