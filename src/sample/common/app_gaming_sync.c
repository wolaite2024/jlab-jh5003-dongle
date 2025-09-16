#if F_APP_GAMING_DONGLE_SUPPORT || TARGET_LEGACY_GAMING_DONGLE || TARGET_LE_AUDIO_GAMING_DONGLE
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "trace.h"
#include "gap_br.h"
#include "ring_buffer.h"
#include "app_gaming_sync.h"
#include "app_timer.h"
#include "os_mem.h"

#define CTRL_DATA_MAX_SIZE                   384
#define CTRL_DATA_SEND_BUF_SIZE              1024
#define CTRL_DATA_ACK_BUF_SIZE               128

typedef enum
{
    GAMING_SYNC_TIMER_WAIT_DATA_ACK,
    GAMING_SYNC_TIMER_STREAMING_TO_PEER,
    GAMING_SYNC_TIMER_SEND_DATA_RETRY,
} T_GAMING_SYNC_TIMER;

typedef struct
{
    bool flushable;
    bool wait_for_ack;
    uint16_t build_len;
} T_CTRL_PKT_BUILD_INFO;

typedef struct
{
    uint8_t last_rcv_pkt_seq;
    bool    ctrl_pkt_rcvd;
} T_CTRL_DATA_INFO;

#define calloc(num, size) os_mem_zalloc(RAM_TYPE_BUFFER_ON, num * size)
#define free(ptr)         os_mem_free(ptr)

static uint8_t          app_gaming_sync_timer_id = 0;
static uint8_t          timer_idx_wait_data_ack = 0;
static uint8_t          timer_idx_send_data_retry = 0;
static uint8_t          timer_idx_streaming_to_peer = 0;
static uint8_t          *ctrl_data_send_buf;
static uint8_t          *ctrl_data_ack_buf;
static T_RING_BUFFER    ctrl_data_send_ring_buf;
static T_RING_BUFFER    ctrl_data_ack_ring_buf;
static T_CTRL_DATA_INFO ctrl_data_info;
static bool             ctrl_data_link_connected;
static uint8_t          ctrl_data_le_link_id = LE_LINK_ID_INVALID;
static bool             ctrl_data_append_to_streaming;
static uint8_t          ctrl_data_link_addr[6] = {0};
static uint16_t         ctrl_data_max_len = 0;

static P_GAMING_SYNC_PKT_RCV_CB     gaming_sync_rcv_cb;
static P_GAMING_SYNC_LEGACY_PKT_SEND_CB gaming_sync_legacy_send_cb;
static P_GAMING_SYNC_LE_PKT_SEND_CB gaming_sync_le_send_cb;
static P_GAMING_SYNC_LEGACY_CMD_CB  gaming_sync_legacy_cmd_cb;
static P_GAMING_SYNC_LE_CMD_CB      gaming_sync_le_cmd_cb;

T_DONGLE_STATUS dongle_status;
T_HEADSET_STATUS headset_status;

static uint16_t ctrl_data_len_peek(void)
{
    uint16_t data_size = ring_buffer_get_data_count(&ctrl_data_send_ring_buf);
    T_APP_CTRL_DATA header;
    uint16_t len = 0;

    if (data_size != 0)
    {
        ring_buffer_peek(&ctrl_data_send_ring_buf, sizeof(T_APP_CTRL_DATA), (uint8_t *)&header);

        len = sizeof(T_APP_CTRL_DATA) + header.data_size;
    }

    return len;
}

static bool ctrl_data_ack_write(uint16_t seq)
{
    T_APP_CTRL_DATA ctrl_data = {0};

    ctrl_data.seq = seq;
    ctrl_data.is_ack = true;

    if (ring_buffer_get_remaining_space(&ctrl_data_ack_ring_buf) < sizeof(T_APP_CTRL_DATA))
    {
        APP_PRINT_ERROR0("ctrl_data_ack_write failed: buf full");
        return false;
    }

    ring_buffer_write(&ctrl_data_ack_ring_buf, (uint8_t *)&ctrl_data, sizeof(T_APP_CTRL_DATA));

    return true;
}

static void ctrl_data_ack_remove()
{
    uint16_t data_size = ring_buffer_get_data_count(&ctrl_data_ack_ring_buf);

    if (data_size != 0)
    {
        ring_buffer_remove(&ctrl_data_ack_ring_buf, sizeof(T_APP_CTRL_DATA));
    }
}

static void ctrl_data_remove()
{
    uint16_t ctrl_data_len = 0;
    uint16_t data_size = ring_buffer_get_data_count(&ctrl_data_send_ring_buf);

    if ((ctrl_data_len = ctrl_data_len_peek()) != 0)
    {
        ring_buffer_remove(&ctrl_data_send_ring_buf, ctrl_data_len);
    }
}

static uint16_t ctrl_data_ack_len_peek(void)
{
    uint16_t data_size = ring_buffer_get_data_count(&ctrl_data_ack_ring_buf);
    uint16_t len = 0;

    if (data_size != 0)
    {
        len = sizeof(T_APP_CTRL_DATA);
    }

    return len;
}

static bool ctrl_data_ack_peek(T_APP_CTRL_DATA *ctrl_data)
{
    uint16_t data_size = ring_buffer_get_data_count(&ctrl_data_ack_ring_buf);
    bool ret = false;

    if (data_size != 0)
    {
        ring_buffer_peek(&ctrl_data_ack_ring_buf, sizeof(T_APP_CTRL_DATA), (uint8_t *)ctrl_data);

        ret = true;
    }

    return ret;
}

static bool ctrl_data_peek(T_APP_CTRL_DATA *ctrl_data, uint16_t len)
{
    uint16_t data_size = ring_buffer_get_data_count(&ctrl_data_send_ring_buf);
    bool ret = false;

    if (data_size != 0)
    {
        ring_buffer_peek(&ctrl_data_send_ring_buf, len, (uint8_t *)ctrl_data);

        ret = true;
    }

    return ret;
}

static bool ctrl_data_send_check(void)
{
    bool ret = false;

    if (ctrl_data_link_connected || ctrl_data_le_link_id != LE_LINK_ID_INVALID)
    {
        if (ctrl_data_ack_len_peek() ||
            (ctrl_data_len_peek() && timer_idx_wait_data_ack == NULL))
        {
            ret = true;
        }
    }

    return ret;
}

static void ctrl_data_get_buf_info(uint16_t *total_len, bool *send_ctrl_data,
                                   bool *send_ctrl_data_ack,
                                   uint16_t *ctrl_data_len, uint16_t *ack_data_len)
{
    if (timer_idx_wait_data_ack == NULL)
    {
        if ((*ctrl_data_len = ctrl_data_len_peek()) != 0)
        {
            *send_ctrl_data = true;

            *total_len += *ctrl_data_len + sizeof(T_GAMING_SYNC_HDR) + 1;
        }
    }

    if ((*ack_data_len = ctrl_data_ack_len_peek()) != 0)
    {
        *send_ctrl_data_ack = true;

        *total_len += *ack_data_len + sizeof(T_GAMING_SYNC_HDR) + 1;
    }
}

static uint16_t ctrl_data_assemble(uint8_t *buf, uint16_t data_len)
{
    T_GAMING_SYNC_HDR *spp_hdr = (T_GAMING_SYNC_HDR *)buf;
    uint16_t total_len = 0;

    spp_hdr->sync = DONGLE_FORMAT_START_BIT;
    spp_hdr->type = DONGLE_TYPE_CMD;
    spp_hdr->pl = data_len + 1;
    spp_hdr->cmd = DONGLE_CMD_CTRL_RAW_DATA;

    buf[sizeof(T_GAMING_SYNC_HDR) + data_len] = DONGLE_FORMAT_STOP_BIT;

    total_len += sizeof(T_GAMING_SYNC_HDR);
    total_len += data_len;
    total_len += 1;

    return total_len;
}

static void ctrl_pkt_build(uint8_t *buf, bool send_ctrl_data, bool send_ctrl_data_ack,
                           uint16_t ctrl_data_len, uint16_t ack_data_len, T_CTRL_PKT_BUILD_INFO *info)
{
    T_APP_CTRL_DATA *ctrl_data = (T_APP_CTRL_DATA *)(buf + sizeof(T_GAMING_SYNC_HDR));

    if (send_ctrl_data && send_ctrl_data_ack == false)
    {
        ctrl_data_peek(ctrl_data, ctrl_data_len);

        info->build_len += ctrl_data_assemble(buf, ctrl_data_len);

        info->flushable = ctrl_data->flushable;

        if (ctrl_data_append_to_streaming && ctrl_data->flushable == false)
        {
            info->wait_for_ack = true;
        }
    }
    else if (send_ctrl_data == false && send_ctrl_data_ack)
    {
        ctrl_data_ack_peek(ctrl_data);

        info->build_len += ctrl_data_assemble(buf, sizeof(T_APP_CTRL_DATA));

        info->flushable = false;
    }
    else if (send_ctrl_data && send_ctrl_data_ack)
    {
        ctrl_data_peek(ctrl_data, ctrl_data_len);

        info->build_len += ctrl_data_assemble(buf, ctrl_data_len);

        if (ctrl_data_append_to_streaming && ctrl_data->flushable == false)
        {
            info->wait_for_ack = true;
        }

        uint8_t *next_start_offset = buf + sizeof(T_GAMING_SYNC_HDR) + ctrl_data_len + 1;

        ctrl_data = (T_APP_CTRL_DATA *)(next_start_offset + sizeof(T_GAMING_SYNC_HDR));

        ctrl_data_ack_peek(ctrl_data);

        info->build_len += ctrl_data_assemble(next_start_offset, sizeof(T_APP_CTRL_DATA));

        info->flushable = false;
    }
}

static void ctrl_pkt_send_post_handle(bool send_ctrl_data, bool send_ctrl_data_ack,
                                      bool wait_for_ack)
{
    if (send_ctrl_data)
    {
        if (ctrl_data_append_to_streaming && wait_for_ack)
        {
            app_start_timer(&timer_idx_wait_data_ack, "wait_data_ack",
                            app_gaming_sync_timer_id, GAMING_SYNC_TIMER_WAIT_DATA_ACK, 0, false,
                            150);
        }
        else
        {
            ctrl_data_remove();
        }
    }

    if (send_ctrl_data_ack)
    {
        ctrl_data_ack_remove();
    }
}

static bool ctrl_data_send_data_in_buf(void)
{
    uint8_t *tmp = NULL;
    uint8_t failed_cause = 0;
    uint16_t total_len = 0;
    uint16_t ctrl_data_len = 0;
    uint16_t ack_data_len = sizeof(T_APP_CTRL_DATA);
    bool send_ctrl_data = false;
    bool send_ctrl_data_ack = false;
    T_CTRL_PKT_BUILD_INFO pkt_build_info = {0};
    bool ret = false;

    if (ctrl_data_link_connected == false && ctrl_data_le_link_id == LE_LINK_ID_INVALID)
    {
        failed_cause = 1;
        goto exit;
    }

    if (gaming_sync_legacy_send_cb == NULL && gaming_sync_le_send_cb == NULL)
    {
        failed_cause = 2;
        goto exit;
    }

    ctrl_data_get_buf_info(&total_len, &send_ctrl_data, &send_ctrl_data_ack, &ctrl_data_len,
                           &ack_data_len);

    if (total_len == 0)
    {
        failed_cause = 3;
        goto exit;
    }

    tmp = calloc(1, total_len);

    if (tmp == NULL)
    {
        failed_cause = 4;
        goto exit;
    }

    ctrl_pkt_build(tmp, send_ctrl_data, send_ctrl_data_ack, ctrl_data_len, ack_data_len,
                   &pkt_build_info);

    if (gaming_sync_legacy_send_cb)
    {
        ret = gaming_sync_legacy_send_cb(ctrl_data_link_addr, tmp, pkt_build_info.build_len,
                                         pkt_build_info.flushable);

        if (ret == false)
        {
            failed_cause = 5;
            goto exit;
        }
    }

    if (gaming_sync_le_send_cb)
    {
        ret = gaming_sync_le_send_cb(ctrl_data_le_link_id, tmp, pkt_build_info.build_len);

        if (ret == false)
        {
            failed_cause = 6;
            goto exit;
        }
    }

    ctrl_pkt_send_post_handle(send_ctrl_data, send_ctrl_data_ack, pkt_build_info.wait_for_ack);

exit:
    if (tmp != NULL)
    {
        free(tmp);
    }

    if (failed_cause != 0)
    {
        APP_PRINT_ERROR1("send_data_and_ack failed: cause %d", failed_cause);
    }

    return ret;
}

static void ctrl_data_send()
{
    if (ctrl_data_append_to_streaming && timer_idx_streaming_to_peer != 0)
    {
        APP_PRINT_TRACE0("handle ctrl data in streaming data");

        return;
    }

    while (ctrl_data_send_check() && (timer_idx_send_data_retry == 0))
    {
        if (ctrl_data_send_data_in_buf() == false)
        {
            app_start_timer(&timer_idx_send_data_retry, "send_data_retry",
                            app_gaming_sync_timer_id, GAMING_SYNC_TIMER_SEND_DATA_RETRY, 0, false,
                            500);
        }
    }
}

static bool ctrl_data_write(uint8_t *data, uint16_t size, bool flushable)
{
    uint16_t remain_size = ring_buffer_get_remaining_space(&ctrl_data_send_ring_buf);
    uint8_t cause = 0;
    T_APP_CTRL_DATA *ctrl_data = NULL;
    static uint8_t ctrl_data_seq = 0;
    uint16_t write_size = sizeof(T_APP_CTRL_DATA) + size;
    bool ret = false;

    ctrl_data = calloc(1, ctrl_data_max_len + sizeof(T_APP_CTRL_DATA));
    if (ctrl_data == NULL)
    {
        cause = 1;
        goto exit;
    }

    if (remain_size < write_size)
    {
        cause = 2;
        goto exit;
    }

    ctrl_data->seq = ctrl_data_seq++;
    ctrl_data->is_ack = false;
    ctrl_data->flushable = flushable;
    ctrl_data->data_size = size;
    memcpy(ctrl_data->payload, data, size);

    ring_buffer_write(&ctrl_data_send_ring_buf, (uint8_t *)ctrl_data, write_size);

    ret = true;

exit:
    if (ctrl_data != NULL)
    {
        free(ctrl_data);
    }

    if (cause != 0)
    {
        APP_PRINT_ERROR1("ctrl_data_write failed: cause %d", cause);
    }

    return ret;
}

static void ctrl_pkt_handle(uint8_t *bd_addr, uint8_t *data, uint16_t size)
{
    T_APP_CTRL_DATA *ctrl_data = (T_APP_CTRL_DATA *)data;

    if (ctrl_data_append_to_streaming == false)
    {
        if (gaming_sync_rcv_cb)
        {
            gaming_sync_rcv_cb(ctrl_data->payload, ctrl_data->data_size);
        }
    }
    else
    {
        if (ctrl_data->is_ack)
        {
            T_APP_CTRL_DATA tmp = {0};

            if (ctrl_data_peek(&tmp, sizeof(T_APP_CTRL_DATA)) && tmp.seq == ctrl_data->seq)
            {
                ctrl_data_remove();

                app_stop_timer(&timer_idx_wait_data_ack);

                ctrl_data_send();
            }
        }
        else
        {
            if (ctrl_data->flushable == false)
            {
                if (ctrl_data_ack_write(ctrl_data->seq))
                {
                    ctrl_data_send();
                }

                if (ctrl_data_info.ctrl_pkt_rcvd == false ||
                    (ctrl_data_info.ctrl_pkt_rcvd && ctrl_data->seq == ((ctrl_data_info.last_rcv_pkt_seq + 1) & 0xff)))
                {
                    if (gaming_sync_rcv_cb)
                    {
                        gaming_sync_rcv_cb(ctrl_data->payload, ctrl_data->data_size);
                    }

                    ctrl_data_info.last_rcv_pkt_seq = ctrl_data->seq;
                    ctrl_data_info.ctrl_pkt_rcvd = true;
                }

            }
            else
            {
                if (gaming_sync_rcv_cb)
                {
                    gaming_sync_rcv_cb(ctrl_data->payload, ctrl_data->data_size);
                }
            }
        }
    }
}

static uint16_t ctrl_pkt_len_get(void)
{
    uint16_t total_len = 0;
    bool send_ctrl_data = false;
    bool send_ctrl_data_ack = false;
    uint16_t ctrl_data_len = 0;
    uint16_t ack_data_len = 0;

    ctrl_data_get_buf_info(&total_len, &send_ctrl_data, &send_ctrl_data_ack, &ctrl_data_len,
                           &ack_data_len);

    if (ctrl_data_len != 0)
    {
        total_len += (ctrl_data_len + sizeof(T_GAMING_SYNC_HDR) + 1);
    }

    if (ack_data_len != 0)
    {
        total_len += (ctrl_data_len + sizeof(T_GAMING_SYNC_HDR) + 1);
    }

    return total_len;
}

static void ctrl_pkt_clear(void)
{
    app_stop_timer(&timer_idx_wait_data_ack);
    app_stop_timer(&timer_idx_streaming_to_peer);
    app_stop_timer(&timer_idx_send_data_retry);

    ring_buffer_clear(&ctrl_data_send_ring_buf);
    ring_buffer_clear(&ctrl_data_ack_ring_buf);

    memset(&ctrl_data_info, 0, sizeof(ctrl_data_info));
}

bool app_gaming_set_sync_data_in_streaming(void)
{
    static uint32_t last_detect_time = 0;
    uint32_t curr_time = sys_timestamp_get();
    bool restart_timer = false;

    if (timer_idx_streaming_to_peer == 0)
    {
        /* dongle streaming start */
        last_detect_time = 0;
    }

    if (curr_time > last_detect_time)
    {
        if ((curr_time - last_detect_time) > GAMING_SYNC_STREAM_DETECT_INTERVAL)
        {
            restart_timer = true;
        }
    }
    else
    {
        if ((0xffffffff - last_detect_time + curr_time) > GAMING_SYNC_STREAM_DETECT_INTERVAL)
        {
            restart_timer = true;
        }
    }

    if (restart_timer)
    {
        app_start_timer(&timer_idx_streaming_to_peer, "streaming_to_peer",
                        app_gaming_sync_timer_id, GAMING_SYNC_TIMER_STREAMING_TO_PEER, 0, false,
                        GAMING_SYNC_STREAMING_TIMEOUT);

        last_detect_time = curr_time;
    }

    return true;
}

uint16_t app_gaming_sync_get_data_len_for_streaming(void)
{
    uint16_t data_len = 0;

    if (timer_idx_streaming_to_peer)
    {
        data_len = ctrl_pkt_len_get();
    }

    return data_len;
}

void app_gaming_sync_append_data_to_streaming(uint8_t *start_offset, uint16_t len)
{
    uint16_t total_len = 0;
    uint16_t ctrl_data_len = 0;
    uint16_t ack_data_len = sizeof(T_APP_CTRL_DATA);
    bool send_ctrl_data = false;
    bool send_ctrl_data_ack = false;
    T_CTRL_PKT_BUILD_INFO pkt_build_info = {0};
    uint16_t ctrl_pkt_len = ctrl_pkt_len_get();
    uint8_t cause = 0;

    if (ctrl_pkt_len > len)
    {
        cause = 1;
        goto exit;
    }

    if (timer_idx_wait_data_ack != 0)
    {
        cause = 2;
        goto exit;
    }

    ctrl_data_get_buf_info(&total_len, &send_ctrl_data, &send_ctrl_data_ack, &ctrl_data_len,
                           &ack_data_len);

    ctrl_pkt_build(start_offset, send_ctrl_data, send_ctrl_data_ack, ctrl_data_len, ack_data_len,
                   &pkt_build_info);

    ctrl_pkt_send_post_handle(send_ctrl_data, send_ctrl_data_ack, pkt_build_info.wait_for_ack);

exit:
    if (cause != 0)
    {
        APP_PRINT_ERROR1("app_gaming_sync_append_data_to_streaming failed: cause %d", cause);
    }
}

static void app_gaming_sync_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case GAMING_SYNC_TIMER_WAIT_DATA_ACK:
        {
            app_stop_timer(&timer_idx_wait_data_ack);

            ctrl_data_send();
        }
        break;

    case GAMING_SYNC_TIMER_STREAMING_TO_PEER:
        {
            app_stop_timer(&timer_idx_streaming_to_peer);

            ctrl_data_send();
        }
        break;

    case GAMING_SYNC_TIMER_SEND_DATA_RETRY:
        {
            app_stop_timer(&timer_idx_send_data_retry);

            ctrl_data_send();
        }
        break;

    default:
        break;
    }
}

bool app_gaming_sync_data_send(uint8_t *data, uint16_t size, bool flushable)
{
    uint8_t cause = 0;
    uint8_t ret = true;

    if (ctrl_data_max_len > CTRL_DATA_MAX_SIZE)
    {
        cause = 1;
        goto exit;
    }

    if (size > ctrl_data_max_len)
    {
        cause = 2;
        goto exit;
    }

    if (ctrl_data_link_connected == false && ctrl_data_le_link_id == LE_LINK_ID_INVALID)
    {
        /* dongle not connected */
        cause = 3;
        goto exit;
    }

    if (gaming_sync_legacy_send_cb == NULL && gaming_sync_le_send_cb == NULL)
    {
        cause = 4;
        goto exit;
    }

    if (ctrl_data_write(data, size, flushable) == false)
    {
        cause = 5;
        goto exit;
    }

    ctrl_data_send();

    ret = true;
exit:

    if (cause != 0)
    {
        ret = false;
    }


    APP_PRINT_TRACE3("app_dongle_send_ctrl_data: size %d ret %d cause %d", size, ret, cause);

    return ret;
}

void app_gaming_sync_disassemble_data(uint8_t *bd_addr, uint8_t *buf, uint16_t len)
{
    T_APP_CTRL_PKT_ASSEMBLE *as = NULL;
    uint16_t i = 0;
    uint16_t tlen = 0;
    T_GAMING_SYNC_HDR *spp_hdr = NULL;

    if (buf == NULL || len == 0)
    {
        return;
    }

    as = calloc(1, sizeof(T_APP_CTRL_PKT_ASSEMBLE));
    if (as == NULL)
    {
        return;
    }

    while (len > 0)
    {
        switch (as->state)
        {
        case SYNC_PKT_START:
            {
                if (buf[i] != DONGLE_FORMAT_START_BIT)
                {
                    i++;
                    len--;
                    continue;
                }

                spp_hdr = (T_GAMING_SYNC_HDR *)(buf + i);
                as->state = SYNC_PKT_HEADER;

                T_APP_DONGLE_DATA_TYPE data_type = (T_APP_DONGLE_DATA_TYPE)((spp_hdr->type) & 0x07);

                if (data_type == DONGLE_TYPE_UPSTREAM_VOICE || data_type == DONGLE_TYPE_CONTROLLER_HID_DATA)
                {
                    as->header_len = sizeof(T_GAMING_SYNC_HDR);
                }
                else
                {
                    as->header_len = sizeof(T_GAMING_SYNC_HDR) - 1; /* not include cmd */
                }

                as->header[0] = spp_hdr->sync; /* DONGLE_FORMAT_START_BIT */
                as->pos++;
                i++;
                len--;
            }
            break;

        case SYNC_PKT_HEADER:
            {
                as->header[as->pos++] = buf[i++];
                len--;
                if (as->pos == as->header_len)
                {
                    if ((spp_hdr != NULL) && (((spp_hdr->type) & 0x07) == DONGLE_TYPE_UPSTREAM_VOICE))
                    {
                        tlen = (as->header[offsetof(T_GAMING_SYNC_HDR, cmd)] | ((as->header[offsetof(T_GAMING_SYNC_HDR,
                                                                                                     pl)] & 0x03) << 8));
                    }
                    else
                    {
                        tlen = as->header[offsetof(T_GAMING_SYNC_HDR, pl)];
                    }

                    /* include the ending DONGLE_FORMAT_STOP_BIT */
                    as->pkt_len = as->header_len + tlen + 1;
                    memcpy(as->cache, as->header, as->header_len);
                    as->state = SYNC_PKT_DATA;

                    /* No payload */
                    if (as->pkt_len == as->header_len + 1)
                    {
                        APP_PRINT_ERROR0("app_gaming_sync_disassemble_data: Received pkt without payload");
                        as->pos = 0;
                        as->state = SYNC_PKT_START;
                    }
                }
            }
            break;

        case SYNC_PKT_DATA:
            {
                if (as->pos + len < as->pkt_len)
                {
                    tlen = len;
                }
                else
                {
                    tlen = as->pkt_len - as->pos;
                }
                memcpy(as->cache + as->pos, buf + i, tlen);
                len -= tlen;
                as->pos += tlen;
                i += tlen;
                if (as->pos == as->pkt_len)
                {
                    if (as->cache[as->pkt_len - 1] == DONGLE_FORMAT_STOP_BIT)
                    {
                        if ((spp_hdr != NULL) && (spp_hdr->cmd == DONGLE_CMD_CTRL_RAW_DATA))
                        {
                            ctrl_pkt_handle(bd_addr, as->cache + sizeof(T_GAMING_SYNC_HDR), spp_hdr->pl);
                        }
                        else
                        {
                            if (gaming_sync_legacy_cmd_cb)
                            {
                                gaming_sync_legacy_cmd_cb(bd_addr, as->cache, as->pkt_len);
                            }

                            if (gaming_sync_le_cmd_cb)
                            {
                                gaming_sync_le_cmd_cb(as->cache, as->pkt_len);
                            }
                        }
                    }
                    else
                    {
                        APP_PRINT_ERROR0("app_gaming_sync_disassemble_data: rcv bad data");
                    }
                    as->pos = 0;
                    as->state = SYNC_PKT_START;
                }
            }
            break;
        }
    }

    free(as);
}

void app_gaming_sync_le_send_register(P_GAMING_SYNC_LE_PKT_SEND_CB cb)
{
    gaming_sync_le_send_cb = cb;
}

void app_gaming_sync_legacy_send_register(P_GAMING_SYNC_LEGACY_PKT_SEND_CB cb)
{
    gaming_sync_legacy_send_cb = cb;
}

void app_gaming_sync_rcv_register(P_GAMING_SYNC_PKT_RCV_CB cb)
{
    gaming_sync_rcv_cb = cb;
}

void app_gaming_sync_legacy_cmd_register(P_GAMING_SYNC_LEGACY_CMD_CB cb)
{
    gaming_sync_legacy_cmd_cb = cb;
}

void app_gaming_sync_le_cmd_register(P_GAMING_SYNC_LE_CMD_CB cb)
{
    gaming_sync_le_cmd_cb = cb;
}

void app_gaming_sync_set_le_link_id(uint8_t link_id)
{
    ctrl_data_le_link_id = link_id;
}

void app_gaming_sync_set_link_connected(bool connected, uint8_t *addr)
{
    uint8_t null_addr[6] = {0};

    APP_PRINT_TRACE1("app_gaming_sync_set_link_connected: %d", connected);

    ctrl_data_link_connected = connected;

    if (connected)
    {
        memcpy(ctrl_data_link_addr, addr, 6);
    }
    else
    {
        memcpy(ctrl_data_link_addr, null_addr, 6);

        ctrl_pkt_clear();
    }
}

void app_gaming_sync_init(bool append_to_streaming, uint16_t max_data_len)
{
    app_timer_reg_cb(app_gaming_sync_timeout_cb, &app_gaming_sync_timer_id);

    ctrl_data_append_to_streaming = append_to_streaming;
    ctrl_data_max_len = max_data_len;

    if (ctrl_data_send_buf == NULL)
    {
        ctrl_data_send_buf = calloc(1, CTRL_DATA_SEND_BUF_SIZE);

        ring_buffer_init(&ctrl_data_send_ring_buf, ctrl_data_send_buf, CTRL_DATA_SEND_BUF_SIZE);
    }

    if (ctrl_data_ack_buf == NULL)
    {
        ctrl_data_ack_buf = calloc(1, CTRL_DATA_ACK_BUF_SIZE);

        ring_buffer_init(&ctrl_data_ack_ring_buf, ctrl_data_ack_buf, CTRL_DATA_ACK_BUF_SIZE);
    }
}

bool app_gaming_get_connect_state(void)
{
	return ctrl_data_link_connected;
}

#endif
