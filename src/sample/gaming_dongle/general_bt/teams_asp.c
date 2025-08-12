#include <stdint.h>
#include <string.h>
#include "bt_rfc.h"
#include "trace.h"
#include "app_link_util.h"
#include "teams_asp.h"
#include "app_src_asp.h"
#include "bt_types.h"

#define META_HDR_LEN         2
#define PKT_START            0
#define PKT_HEADER           1
#define PKT_DATA             2
#define PKT_DATA_META_HEADER 3
#define PKT_META_DATA        4

struct rfc_packet_assembler
{
    uint8_t cache[MAX_ASP_CACHE];
    uint8_t header[4];
    uint8_t header_len;
    uint8_t state;
    uint8_t meta_header[META_HDR_LEN];
    uint8_t meta_offset;
    uint8_t meta;
    uint16_t pos;
    uint16_t pkt_len;
};
struct rfc_packet_assembler asp_assembler;

static TEAMS_ASP_SEND_DATA func_asp_send = NULL;

static void asp_assemble_packet(uint8_t *buf, uint16_t len)
{
    struct rfc_packet_assembler *as = &asp_assembler;
    uint16_t i = 0;
    uint16_t tlen = 0;

    if (!buf || !len)
    {
        APP_PRINT_ERROR2("asp_assemble_packet(): invalid params %p, %u",
                         buf, len);
        return;
    }

    while (len > 0)
    {
        //        APP_PRINT_ERROR3("asp_assemble_packet() current %u, %u, %u",
        //                         buf[i], len, as->state);
        switch (as->state)
        {
        case PKT_START:
            as->state = PKT_HEADER;
            as->header_len = 4;
            as->header[0] = buf[i];
            as->pos++;
            i++;
            len--;
            break;
        case PKT_HEADER:
            as->header[as->pos++] = buf[i++];
            len--;
            if (as->pos == as->header_len)
            {
                BE_ARRAY_TO_UINT16(tlen, &(as->header[2]));
                as->meta = as->header[1] & 0x0f;
                as->pkt_len = as->header_len + tlen;
                memcpy(as->cache, as->header, as->header_len);
                as->state = PKT_DATA;
                //        APP_PRINT_ERROR3("asp_assemble_packet(1) current %u, %u, %u",
                //                         as->meta, as->pkt_len, as->state);
                /* No payload */
                if (as->pkt_len == as->header_len)
                {
                    //                    app_usb_hid_send_report(HID_IF_ASP, 0, &(as->cache[1]),
                    //                                            as->pkt_len - 1, 0);
                    app_asp_received_proc(as->cache, as->pkt_len);
                    as->pos = 0;
                    as->state = PKT_START;
                }
            }
            break;
        case PKT_DATA:
            if (as->pos + len < as->pkt_len)
            {
                tlen = len;
            }
            else
            {
                tlen = as->pkt_len - as->pos;
            }
            //        APP_PRINT_ERROR3("asp_assemble_packet(2) current %u, %u, %u",
            //                         as->meta, as->pkt_len, as->state);
            memcpy(as->cache + as->pos, buf + i, tlen);
            len -= tlen;
            as->pos += tlen;
            i += tlen;
            if (as->pos == as->pkt_len)
            {
                if (as->meta == 0)
                {
                    //                    app_usb_hid_send_report(HID_IF_ASP, 0, &(as->cache[1]),
                    //                                            as->pkt_len - 1, 0);
                    app_asp_received_proc(as->cache, as->pkt_len);
                    as->pos = 0;
                    as->state = PKT_START;
                }
                else
                {
                    as->state = PKT_DATA_META_HEADER;
                }
            }
            break;
        case PKT_DATA_META_HEADER:
            if (as->meta == 0)
            {
                APP_PRINT_ERROR0("asp_assemble_packet(): Meta is zero");
                as->pos = 0;
                as->meta_offset = 0;
                as->state = PKT_START;
                break;
            }
            as->meta_header[as->meta_offset++] = buf[i++];
            len--;
            if (as->meta_offset == META_HDR_LEN)
            {
                memcpy(as->cache + as->pos, as->meta_header, as->meta_offset);
                as->pos += as->meta_offset;
                BE_ARRAY_TO_UINT16(tlen, &(as->meta_header[0]));
                as->pkt_len += as->meta_offset + tlen;
                as->state = PKT_META_DATA;
                as->meta_offset = 0;
            }
            break;
        case PKT_META_DATA:
            as->meta--;
            as->state = PKT_DATA;
            break;
        }
    }
}

void teams_asp_tx_handle(uint8_t *data, uint8_t len)
{
    if (!data || !len)
    {
        APP_PRINT_INFO2("teams_asp_tx_handle Invalid param %p %u", data, len);
        return;
    }
    func_asp_send(data, len);
}

void teams_asp_rx_handle(uint8_t *data, uint16_t len)
{
    asp_assemble_packet(data, len);
}

void teams_asp_init(TEAMS_ASP_SEND_DATA send_cb)
{
    func_asp_send = send_cb;
}

void teams_asp_deinit(void)
{
    func_asp_send = NULL;
}

