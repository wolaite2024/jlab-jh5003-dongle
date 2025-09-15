/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdlib.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "rfc.h"
#include "obex.h"

#define OBEX_SUCCESS            0x00
#define OBEX_FAIL               0x01

#define FLAG_CONNECTION_ID      0x01
#define FLAG_BODY               0x02
#define FLAG_AUTHEN_CHALLENGE   0x04
#define FLAG_SRM_ENABLE         0x08
#define FLAG_SRMP_WAIT          0x10

#define OBEX_VERSION            0x10 /* version 1.0 */

/************opcode definition**************/
#define OBEX_OPCODE_CONNECT             0x80
#define OBEX_OPCODE_DISCONNECT          0x81
#define OBEX_OPCODE_PUT                 0x02
#define OBEX_OPCODE_FPUT                0x82
#define OBEX_OPCODE_GET                 0x03
#define OBEX_OPCODE_FGET                0x83
#define OBEX_OPCODE_SET_PATH            0x85
#define OBEX_OPCODE_ACTION              0x06
#define OBEX_OPCODE_FACTION             0x86
#define OBEX_OPCODE_SESSION             0x87
#define OBEX_OPCODE_ABORT               0xFF

#define OBEX_BASE_LEN     3  /* 1 byte opcode + 2 byte pkt_len*/
#define OBEX_CONNECT_LEN  7
#define OBEX_SET_PATH_LEN 5

#define OBEX_OVER_RFC_MAX_PKT_LEN       1011    /* 1021-4(l2c hdr)-5(rfc hdr)-1(crc) */
#define OBEX_OVER_L2C_MAX_PKT_LEN       1015    /* 1021-6(l2c ertm hdr/fcs) */

/* F, G and H are basic MD5 functions: selection, majority, parity */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
    {(a) += F ((b), (c), (d)) + (x) + (uint32_t)(ac); \
        (a) = ROTATE_LEFT ((a), (s)); \
        (a) += (b); \
    }
#define GG(a, b, c, d, x, s, ac) \
    {(a) += G ((b), (c), (d)) + (x) + (uint32_t)(ac); \
        (a) = ROTATE_LEFT ((a), (s)); \
        (a) += (b); \
    }
#define HH(a, b, c, d, x, s, ac) \
    {(a) += H ((b), (c), (d)) + (x) + (uint32_t)(ac); \
        (a) = ROTATE_LEFT ((a), (s)); \
        (a) += (b); \
    }
#define II(a, b, c, d, x, s, ac) \
    {(a) += I ((b), (c), (d)) + (x) + (uint32_t)(ac); \
        (a) = ROTATE_LEFT ((a), (s)); \
        (a) += (b); \
    }

typedef struct t_obex_md5
{
    uint32_t    buf[4];
    uint32_t    bytes[2];
    uint8_t     in[64];
    uint8_t     digest[16];
} T_OBEX_MD5;

typedef struct t_obex_connect
{
    uint8_t     opcode;
    uint16_t    packet_len;
    uint8_t     obex_version;
    uint8_t     flags;
    uint16_t    max_packet_len;
} T_OBEX_CONNECT;

typedef struct t_obex_set_path
{
    uint8_t     opcode;
    uint16_t    packet_len;
    uint8_t     flags;
    uint8_t     constants;
} T_OBEX_SET_PATH;

typedef struct t_obex_link
{
    struct t_obex_link *next;
    uint8_t             bd_addr[6];
    uint16_t            cid;
    uint16_t            psm;
    uint8_t             dlci;
    uint8_t             server_chann;
    uint8_t             queue_id;
    T_OBEX_STATE        state;
    T_OBEX_ROLE         role;
    uint8_t             srm_status;
    uint32_t            conn_id;
    const uint8_t      *conn_param;
    uint8_t            *p_rx_data;
    uint16_t            rx_len;
    uint16_t            total_len;
    uint8_t             wait_credit_flag;
    P_OBEX_CBACK        cback;
    uint16_t            local_max_pkt_len;
    uint16_t            remote_max_pkt_len;
} T_OBEX_LINK;

typedef struct t_obex_rfc_cback
{
    struct t_obex_rfc_cback *next;
    P_OBEX_CBACK             cback;
    uint8_t                  service_id;
    uint8_t                  server_chann;
} T_OBEX_RFC_CBACK;

typedef struct t_obex_l2c_cback
{
    struct t_obex_l2c_cback *next;
    P_OBEX_CBACK             cback;
    uint16_t                 psm;
    uint8_t                  queue_id;
} T_OBEX_L2C_CBACK;

typedef struct t_obex
{
    T_OS_QUEUE rfc_cback_list;
    T_OS_QUEUE l2c_cback_list;
    T_OS_QUEUE link_queue;
    uint8_t    service_id;
    uint16_t   ds_data_offset;
} T_OBEX;

static T_OBEX *obex = NULL;

const uint8_t md5_padding[64] =
{
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void transfrom(uint32_t *buf,  uint32_t *in)
{
    uint32_t a = buf[0], b = buf[1], c = buf[2], d = buf[3];

    /* Round 1 */
    FF(a, b, c, d, in[ 0], 7, 3614090360);   /* 1 */
    FF(d, a, b, c, in[ 1], 12, 3905402710);   /* 2 */
    FF(c, d, a, b, in[ 2], 17,  606105819);   /* 3 */
    FF(b, c, d, a, in[ 3], 22, 3250441966);   /* 4 */
    FF(a, b, c, d, in[ 4], 7, 4118548399);   /* 5 */
    FF(d, a, b, c, in[ 5], 12, 1200080426);   /* 6 */
    FF(c, d, a, b, in[ 6], 17, 2821735955);   /* 7 */
    FF(b, c, d, a, in[ 7], 22, 4249261313);   /* 8 */
    FF(a, b, c, d, in[ 8], 7, 1770035416);   /* 9 */
    FF(d, a, b, c, in[ 9], 12, 2336552879);   /* 10 */
    FF(c, d, a, b, in[10], 17, 4294925233);   /* 11 */
    FF(b, c, d, a, in[11], 22, 2304563134);   /* 12 */
    FF(a, b, c, d, in[12], 7, 1804603682);   /* 13 */
    FF(d, a, b, c, in[13], 12, 4254626195);   /* 14 */
    FF(c, d, a, b, in[14], 17, 2792965006);   /* 15 */
    FF(b, c, d, a, in[15], 22, 1236535329);   /* 16 */

    /* Round 2 */
    GG(a, b, c, d, in[ 1], 5, 4129170786);   /* 17 */
    GG(d, a, b, c, in[ 6], 9, 3225465664);   /* 18 */
    GG(c, d, a, b, in[11], 14,  643717713);   /* 19 */
    GG(b, c, d, a, in[ 0], 20, 3921069994);   /* 20 */
    GG(a, b, c, d, in[ 5], 5, 3593408605);   /* 21 */
    GG(d, a, b, c, in[10], 9,   38016083);   /* 22 */
    GG(c, d, a, b, in[15], 14, 3634488961);   /* 23 */
    GG(b, c, d, a, in[ 4], 20, 3889429448);   /* 24 */
    GG(a, b, c, d, in[ 9], 5,  568446438);   /* 25 */
    GG(d, a, b, c, in[14], 9, 3275163606);   /* 26 */
    GG(c, d, a, b, in[ 3], 14, 4107603335);   /* 27 */
    GG(b, c, d, a, in[ 8], 20, 1163531501);   /* 28 */
    GG(a, b, c, d, in[13], 5, 2850285829);   /* 29 */
    GG(d, a, b, c, in[ 2], 9, 4243563512);   /* 30 */
    GG(c, d, a, b, in[ 7], 14, 1735328473);   /* 31 */
    GG(b, c, d, a, in[12], 20, 2368359562);   /* 32 */

    /* Round 3 */
    HH(a, b, c, d, in[ 5], 4, 4294588738);   /* 33 */
    HH(d, a, b, c, in[ 8], 11, 2272392833);   /* 34 */
    HH(c, d, a, b, in[11], 16, 1839030562);   /* 35 */
    HH(b, c, d, a, in[14], 23, 4259657740);   /* 36 */
    HH(a, b, c, d, in[ 1], 4, 2763975236);   /* 37 */
    HH(d, a, b, c, in[ 4], 11, 1272893353);   /* 38 */
    HH(c, d, a, b, in[ 7], 16, 4139469664);   /* 39 */
    HH(b, c, d, a, in[10], 23, 3200236656);   /* 40 */
    HH(a, b, c, d, in[13], 4,  681279174);   /* 41 */
    HH(d, a, b, c, in[ 0], 11, 3936430074);   /* 42 */
    HH(c, d, a, b, in[ 3], 16, 3572445317);   /* 43 */
    HH(b, c, d, a, in[ 6], 23,   76029189);   /* 44 */
    HH(a, b, c, d, in[ 9], 4, 3654602809);   /* 45 */
    HH(d, a, b, c, in[12], 11, 3873151461);   /* 46 */
    HH(c, d, a, b, in[15], 16,  530742520);   /* 47 */
    HH(b, c, d, a, in[ 2], 23, 3299628645);   /* 48 */

    /* Round 4 */
    II(a, b, c, d, in[ 0], 6, 4096336452);   /* 49 */
    II(d, a, b, c, in[ 7], 10, 1126891415);   /* 50 */
    II(c, d, a, b, in[14], 15, 2878612391);   /* 51 */
    II(b, c, d, a, in[ 5], 21, 4237533241);   /* 52 */
    II(a, b, c, d, in[12], 6, 1700485571);   /* 53 */
    II(d, a, b, c, in[ 3], 10, 2399980690);   /* 54 */
    II(c, d, a, b, in[10], 15, 4293915773);   /* 55 */
    II(b, c, d, a, in[ 1], 21, 2240044497);   /* 56 */
    II(a, b, c, d, in[ 8], 6, 1873313359);   /* 57 */
    II(d, a, b, c, in[15], 10, 4264355552);   /* 58 */
    II(c, d, a, b, in[ 6], 15, 2734768916);   /* 59 */
    II(b, c, d, a, in[13], 21, 1309151649);   /* 60 */
    II(a, b, c, d, in[ 4], 6, 4149444226);   /* 61 */
    II(d, a, b, c, in[11], 10, 3174756917);   /* 62 */
    II(c, d, a, b, in[ 2], 15,  718787259);   /* 63 */
    II(b, c, d, a, in[ 9], 21, 3951481745);   /* 64 */

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

void md5_update(T_OBEX_MD5    *context,
                const uint8_t *data,
                uint16_t       len)
{
    uint32_t         temp[16];
    uint16_t         mdi;
    uint16_t         i, j;

    /* compute number of bytes mod 64 */
    mdi = (int)((context->bytes[0] >> 3) & 0x3F);

    /* update number of bits */
    if ((context->bytes[0] + ((uint32_t)len << 3)) < context->bytes[0])
    {
        context->bytes[1]++;
    }
    context->bytes[0] += ((uint32_t)len << 3);
    //context->bytes[1] += ((uint32_t)len >> 29);

    while (len--)
    {
        /* add new character to buffer, increment mdi */
        context->in[mdi] = *data++;
        mdi++;

        /* transform if necessary */
        if (mdi == 0x40)
        {
            for (i = 0, j = 0; i < 16; i++, j += 4)
            {
                temp[i] = (((uint32_t)context->in[j + 3]) << 24) | (((uint32_t)context->in[j + 2]) << 16)
                          | (((uint32_t)context->in[j + 1]) << 8) | ((uint32_t)context->in[j]);
            }
            transfrom(context->buf, temp);
            mdi = 0;
        }
    }
}

void md5_final(T_OBEX_MD5 *context)
{
    uint32_t         in[16];
    uint16_t         mdi;
    uint32_t         i, j;
    uint16_t         pad_len;

    /* save number of bits */
    in[14] = context->bytes[0];
    in[15] = context->bytes[1];

    /* compute number of bytes mod 64 */
    mdi = (uint16_t)((context->bytes[0] >> 3) & 0x3F);

    /* pad out to 56 mod 64 */
    pad_len = (mdi < 56) ? (56 - mdi) : (120 - mdi);
    md5_update(context, md5_padding, pad_len);

    /* append length in bits and transform */
    for (i = 0, j = 0; i < 14; i++, j += 4)
    {
        in[i] = (((uint32_t)context->in[j + 3]) << 24) | (((uint32_t)context->in[j + 2]) << 16)
                | (((uint32_t)context->in[j + 1]) << 8) | ((uint32_t)context->in[j]);
    }
    transfrom(context->buf, in);

    /* store buffer in digest */
    for (i = 0, j = 0; i < 4; i++, j += 4)
    {
        context->digest[j] = (uint8_t)(context->buf[i]);
        context->digest[j + 1] = (uint8_t)(context->buf[i] >> 8);
        context->digest[j + 2] = (uint8_t)(context->buf[i] >> 16);
        context->digest[j + 3] = (uint8_t)(context->buf[i] >> 24);
    }
}

T_OBEX_LINK *obex_alloc_link(uint8_t bd_addr[6])
{
    T_OBEX_LINK *link;

    link = calloc(1, sizeof(T_OBEX_LINK));
    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);
        link->state = OBEX_STATE_IDLE;

        os_queue_in(&obex->link_queue, link);
    }

    return link;
}

void obex_free_link(T_OBEX_LINK *link)
{
    if (link != NULL)
    {
        os_queue_delete(&obex->link_queue, link);
        free(link);
    }
}

T_OBEX_LINK *obex_find_link_by_server_chann(uint8_t     bd_addr[6],
                                            uint8_t     server_chann,
                                            T_OBEX_ROLE role)
{
    T_OBEX_LINK *link;

    link = os_queue_peek(&obex->link_queue, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6) &&
            link->server_chann == server_chann &&
            link->role == role)
        {
            return link;
        }

        link = link->next;
    }

    return NULL;
}

T_OBEX_LINK *obex_find_link_by_dlci(uint8_t bd_addr[6],
                                    uint8_t dlci)
{
    T_OBEX_LINK *link;

    link = os_queue_peek(&obex->link_queue, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6) &&
            link->dlci == dlci)
        {
            return link;
        }

        link = link->next;
    }

    return NULL;
}

T_OBEX_LINK *obex_find_link_by_queue_id(uint8_t bd_addr[6],
                                        uint8_t queue_id)
{
    T_OBEX_LINK *link;

    link = os_queue_peek(&obex->link_queue, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6) &&
            link->queue_id == queue_id)
        {
            return link;
        }

        link = link->next;
    }

    return NULL;
}

T_OBEX_LINK *obex_find_link_by_cid(uint16_t cid)
{
    T_OBEX_LINK *link;

    link = os_queue_peek(&obex->link_queue, 0);
    while (link != NULL)
    {
        if (link->cid == cid)
        {
            return link;
        }

        link = link->next;
    }

    return NULL;
}

T_OBEX_LINK *obex_find_link_by_conn_id(uint32_t conn_id)
{
    T_OBEX_LINK *link;

    link = os_queue_peek(&obex->link_queue, 0);
    while (link != NULL)
    {
        if (link->conn_id == conn_id)
        {
            return link;
        }

        link = link->next;
    }

    return NULL;
}

uint32_t obex_get_free_conn_id(void)
{
    // now do not distinguish local or remote connection id
    uint32_t i;

    for (i = 0x00000001; i < 0xFFFFFF; i++)
    {
        if (obex_find_link_by_conn_id(i) == NULL)
        {
            return i;
        }
    }

    return 0;
}

bool obex_send_data(T_OBEX_LINK *link,
                    uint8_t     *p_data,
                    uint16_t     data_len,
                    bool         ack)
{
    if (link->psm)
    {
        uint8_t *p_buf;

        p_buf = mpa_get_l2c_buf(link->queue_id, link->cid, 0, data_len, obex->ds_data_offset, ack);
        if (p_buf != NULL)
        {
            memcpy(&p_buf[obex->ds_data_offset], p_data, data_len);
            mpa_send_l2c_data_req(p_buf, obex->ds_data_offset, link->cid, data_len, false);

            return true;
        }
    }
    else
    {
        return rfc_data_req(link->bd_addr, link->dlci, p_data, data_len, ack);
    }

    return false;
}

void obex_connect(T_OBEX_LINK *link)
{
    uint16_t    pkt_len;
    uint8_t    *pkt_ptr;
    uint16_t    param_len;
    uint8_t     index;

    param_len = link->conn_param[0];
    pkt_len = OBEX_CONNECT_LEN + param_len;

    pkt_ptr = calloc(1, pkt_len);
    if (pkt_ptr != NULL)
    {
        pkt_ptr[0] = OBEX_OPCODE_CONNECT;
        pkt_ptr[1] = (uint8_t)(pkt_len >> 8);
        pkt_ptr[2] = (uint8_t)pkt_len;
        pkt_ptr[3] = OBEX_VERSION;
        pkt_ptr[4] = 0x00;
        pkt_ptr[5] = (uint8_t)(link->local_max_pkt_len >> 8);
        pkt_ptr[6] = (uint8_t)link->local_max_pkt_len;

        index = OBEX_CONNECT_LEN;
        memcpy(&pkt_ptr[index], &link->conn_param[1], param_len);

        obex_send_data(link, pkt_ptr, pkt_len, false);
        free(pkt_ptr);
        link->state = OBEX_STATE_OBEX_CONNECTING;
    }
}

void obex_connect_with_auth_rsp(T_OBEX_LINK *link,
                                uint8_t     *auth_rsp)
{
    uint16_t pkt_len;
    uint8_t *pkt_ptr;
    uint32_t index;
    uint8_t  param_len;

    param_len = link->conn_param[0];
    pkt_len = OBEX_CONNECT_LEN + OBEX_HDR_AUTHEN_RSP_LEN + param_len;

    pkt_ptr = malloc(pkt_len);
    if (pkt_ptr != NULL)
    {
        pkt_ptr[0] = OBEX_OPCODE_CONNECT;
        pkt_ptr[1] = (uint8_t)(pkt_len >> 8);
        pkt_ptr[2] = (uint8_t)pkt_len;
        pkt_ptr[3] = OBEX_VERSION;
        pkt_ptr[4] = 0x00;
        pkt_ptr[5] = (uint8_t)(link->local_max_pkt_len >> 8);
        pkt_ptr[6] = (uint8_t)link->local_max_pkt_len;

        index = OBEX_CONNECT_LEN;
        pkt_ptr[index] = OBEX_HI_AUTHEN_RSP;
        pkt_ptr[index + 1] = (uint8_t)OBEX_HDR_AUTHEN_RSP_LEN >> 8;
        pkt_ptr[index + 2] = (uint8_t)OBEX_HDR_AUTHEN_RSP_LEN;
        pkt_ptr[index + 3] = 0x00;
        pkt_ptr[index + 4] = 0x10;
        memcpy(&pkt_ptr[index + 5], auth_rsp, 16);
        index += OBEX_HDR_AUTHEN_RSP_LEN;
        memcpy(&pkt_ptr[index], &link->conn_param[1], param_len);

        obex_send_data(link, pkt_ptr, pkt_len, false);
        free(pkt_ptr);
    }
}

bool obex_set_path(T_OBEX_HANDLE  handle,
                   uint8_t        flag,
                   const uint8_t *path_ptr,
                   uint16_t       path_len)
{
    T_OBEX_LINK *link;
    uint8_t *pkt_ptr;
    uint16_t pkt_len;
    bool ret;

    link = (T_OBEX_LINK *)handle;
    if (link == NULL)
    {
        return false;
    }

    pkt_len = OBEX_SET_PATH_LEN + OBEX_HDR_CONN_ID_LEN + OBEX_HDR_PREFIXED_LEN_BASE + path_len;
    pkt_ptr = os_mem_alloc2(pkt_len);
    if (pkt_ptr == NULL)
    {
        return false;
    }

    pkt_ptr[0] = OBEX_OPCODE_SET_PATH;
    pkt_ptr[1] = (uint8_t)(pkt_len >> 8);
    pkt_ptr[2] = (uint8_t)pkt_len;
    pkt_ptr[3] = flag;  // Bit_1: Don't create directory if it does not exist, return an error instead
    pkt_ptr[4] = 0x00;
    pkt_ptr[5] = OBEX_HI_CONNECTION_ID;
    pkt_ptr[6] = (uint8_t)(link->conn_id >> 24);
    pkt_ptr[7] = (uint8_t)(link->conn_id >> 16);
    pkt_ptr[8] = (uint8_t)(link->conn_id >> 8);
    pkt_ptr[9] = (uint8_t)(link->conn_id);
    pkt_ptr[10] = OBEX_HI_NAME;
    pkt_ptr[11] = (uint8_t)((OBEX_HDR_PREFIXED_LEN_BASE + path_len) >> 8);
    pkt_ptr[12] = (uint8_t)(OBEX_HDR_PREFIXED_LEN_BASE + path_len);
    if (path_len)
    {
        memcpy(&pkt_ptr[13], path_ptr, path_len);
    }

    ret = obex_send_data(link, pkt_ptr, pkt_len, false);

    if (ret == true)
    {
        link->state = OBEX_STATE_SET_PATH;
    }

    os_mem_free(pkt_ptr);
    return ret;
}

bool obex_put_data(T_OBEX_HANDLE  handle,
                   const uint8_t *header_ptr,
                   uint16_t       header_len,
                   bool           more_data,
                   uint8_t       *body_ptr,
                   uint16_t       body_len,
                   bool           ack)
{
    T_OBEX_LINK *link;
    uint8_t *p_buf;
    uint16_t buf_len;
    uint8_t *p;
    bool ret;

    link = (T_OBEX_LINK *)handle;
    if (link == NULL)
    {
        return false;
    }

    buf_len = OBEX_HDR_PUT_LEN + header_len;

    if (link->conn_id != 0)
    {
        buf_len += OBEX_HDR_CONN_ID_LEN;
    }

    if (body_ptr != NULL)
    {
        buf_len += (OBEX_HDR_BODY_LEN + body_len);
    }

    p_buf = os_mem_alloc2(buf_len);
    if (p_buf == NULL)
    {
        return false;
    }

    p = p_buf;
    if (more_data)
    {
        BE_UINT8_TO_STREAM(p, OBEX_OPCODE_PUT);
    }
    else
    {
        BE_UINT8_TO_STREAM(p, OBEX_OPCODE_FPUT);
    }
    BE_UINT16_TO_STREAM(p, buf_len);

    if (link->conn_id != 0)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_CONNECTION_ID);
        BE_UINT32_TO_STREAM(p, link->conn_id);
    }

    ARRAY_TO_STREAM(p, header_ptr, header_len);

    if (body_ptr != NULL)
    {
        if (more_data)
        {
            BE_UINT8_TO_STREAM(p, OBEX_HI_BODY);
        }
        else
        {
            BE_UINT8_TO_STREAM(p, OBEX_HI_END_BODY);
        }
        BE_UINT16_TO_STREAM(p, body_len + OBEX_HDR_BODY_LEN);
        ARRAY_TO_STREAM(p, body_ptr, body_len);
    }

    ret = obex_send_data(link, p_buf, buf_len, ack);

    if (ret == true)
    {
        link->state = OBEX_STATE_PUT;
    }

    os_mem_free(p_buf);

    return ret;
}

bool obex_get_object(T_OBEX_HANDLE  handle,
                     uint8_t       *content_ptr,
                     uint16_t       content_len)
{
    T_OBEX_LINK *link;
    uint8_t     *pkt_ptr;
    uint16_t     pkt_len;
    uint32_t     index;
    bool ret;

    link = (T_OBEX_LINK *)handle;
    if (link == NULL)
    {
        return false;
    }

    //GET, connection id, name, type, application parameters
    pkt_len = OBEX_BASE_LEN + OBEX_HDR_CONN_ID_LEN + content_len;

    pkt_ptr = os_mem_alloc2(pkt_len);
    if (pkt_ptr == NULL)
    {
        return false;
    }

    pkt_ptr[0] = OBEX_OPCODE_FGET;
    pkt_ptr[1] = (uint8_t)(pkt_len >> 8);
    pkt_ptr[2] = (uint8_t)pkt_len;
    index = OBEX_BASE_LEN;
    pkt_ptr[index] = OBEX_HI_CONNECTION_ID;
    pkt_ptr[index + 1] = (uint8_t)(link->conn_id >> 24);
    pkt_ptr[index + 2] = (uint8_t)(link->conn_id >> 16);
    pkt_ptr[index + 3] = (uint8_t)(link->conn_id >> 8);
    pkt_ptr[index + 4] = (uint8_t)(link->conn_id);
    index += OBEX_HDR_CONN_ID_LEN;
    link->srm_status = SRM_DISABLE;
    memcpy(&pkt_ptr[index], content_ptr, content_len);

    ret = obex_send_data(link, pkt_ptr, pkt_len, false);
    free(pkt_ptr);

    if (ret)
    {
        link->state = OBEX_STATE_GET;
    }

    return ret;
}

bool obex_send_rsp(T_OBEX_HANDLE handle,
                   uint8_t       rsp_code)
{
    T_OBEX_LINK *link;
    uint8_t pkt_ptr[3];

    link = (T_OBEX_LINK *)handle;
    if (link == NULL)
    {
        return false;
    }

    pkt_ptr[0] = rsp_code;
    pkt_ptr[1] = 0x00;
    pkt_ptr[2] = 0x03;

    return obex_send_data(link, pkt_ptr, 3, false);
}

bool obex_get_object_continue(T_OBEX_HANDLE handle)
{
    T_OBEX_LINK *link;
    uint8_t pkt_ptr[3];

    link = (T_OBEX_LINK *)handle;
    if (link == NULL)
    {
        return false;
    }

    pkt_ptr[0] = OBEX_OPCODE_FGET;
    pkt_ptr[1] = 0x00;
    pkt_ptr[2] = 0x03;

    return obex_send_data(link, pkt_ptr, 3, false);
}

bool obex_abort(T_OBEX_HANDLE handle)
{
    T_OBEX_LINK *link;
    uint8_t pkt_ptr[3];
    bool ret;

    link = (T_OBEX_LINK *)handle;
    if (link == NULL)
    {
        return false;
    }

    pkt_ptr[0] = OBEX_OPCODE_ABORT;
    pkt_ptr[1] = 0x00;
    pkt_ptr[2] = 0x03;
    ret = obex_send_data(link, pkt_ptr, 3, false);

    if (ret == true)
    {
        link->state = OBEX_STATE_ABORT;
    }

    return ret;
}

bool obex_find_hdr_in_pkt(uint8_t   *p_pkt,
                          uint16_t   pkt_len,
                          uint8_t    tgt_hdr,
                          uint8_t  **pp_tgt,
                          uint16_t  *p_tgt_len)
{
    uint8_t hdr;
    uint16_t len;
    uint16_t hdr_len;

    while (pkt_len)
    {
        hdr = *p_pkt;

        switch (hdr & 0xC0)     // bit 7 6
        {
        case 0x00:  //prefixed with 2 byte
        case 0x40:  //prefixed with 2 byte
            len = (*(p_pkt + 1) << 8) | *(p_pkt + 2);
            hdr_len = 3;
            break;

        case 0x80:  //one byte quality
            len = 2;
            hdr_len = 1;
            break;

        case 0xC0:  //four byte quality
            len = 5;
            hdr_len = 1;
            break;

        default:
            return false;
        }

        if (pkt_len < len)
        {
            return false;
        }

        if (hdr == tgt_hdr)
        {
            if (pp_tgt)
            {
                *pp_tgt = p_pkt + hdr_len;
            }

            if (p_tgt_len)
            {
                *p_tgt_len = len - hdr_len;
            }
            return true;
        }

        pkt_len -= len;
        p_pkt += len;
    }

    return false;
}

bool obex_find_value_in_app_param(uint8_t   *p_param,
                                  uint16_t   param_len,
                                  uint8_t    tag,
                                  uint8_t  **pp_value,
                                  uint16_t  *p_value_len)
{
    uint8_t type;
    uint16_t len;

    while (param_len)
    {
        type = *p_param++;
        param_len--;

        len = *p_param++;
        param_len--;

        if (param_len < len)
        {
            return false;
        }

        if (type == tag)
        {
            *pp_value = p_param;
            *p_value_len = len;
            return true;
        }

        param_len -= len;
        p_param += len;
    }

    return false;
}

void obex_handle_req_pkt(T_OBEX_LINK *link,
                         uint8_t     *p_pkt,
                         uint16_t     pkt_len)
{
    uint8_t *p = p_pkt;
    uint8_t opcode;

    BE_STREAM_TO_UINT8(opcode, p);

    PROTOCOL_PRINT_INFO2("obex_handle_req_pkt: opcode 0x%x, pkt len %d", opcode, pkt_len);

    switch (opcode)
    {
    case OBEX_OPCODE_CONNECT:
        if (link->cback)
        {
            uint8_t *p_hdr_data;
            uint16_t hdr_data_len;
            T_OBEX_CONN_IND_DATA data;

            if (obex_find_hdr_in_pkt(p_pkt + OBEX_HDR_CONNECT_LEN,
                                     pkt_len - OBEX_HDR_CONNECT_LEN,
                                     OBEX_HI_SRM,
                                     &p_hdr_data,
                                     &hdr_data_len))
            {
                if (*p_hdr_data == 0x01)
                {
                    link->srm_status = SRM_ENABLE;
                }
            }

            data.handle = link;
            memcpy(data.bd_addr, link->bd_addr, 6);

            p += 4; //pkt len(2), version(1), flag(1)
            BE_STREAM_TO_UINT16(link->remote_max_pkt_len, p);

            link->cback(OBEX_MSG_CONN_IND, &data);
        }
        break;

    case OBEX_OPCODE_DISCONNECT:
        {
            uint8_t pkt_ptr[3];

            pkt_ptr[0] = OBEX_RSP_SUCCESS;
            pkt_ptr[1] = 0x00;
            pkt_ptr[2] = 0x03;
            obex_send_data(link, pkt_ptr, 3, false);
        }
        break;

    case OBEX_OPCODE_PUT:
        {
            if (link->psm && link->srm_status == SRM_DISABLE)
            {
                uint8_t *p_hdr_data;
                uint16_t hdr_data_len;
                uint8_t  pkt_ptr[5];

                if (obex_find_hdr_in_pkt(p_pkt + OBEX_HDR_PUT_LEN,
                                         pkt_len - OBEX_HDR_PUT_LEN,
                                         OBEX_HI_SRM,
                                         &p_hdr_data,
                                         &hdr_data_len))
                {
                    if (*p_hdr_data == 0x01)
                    {
                        pkt_ptr[0] = OBEX_RSP_CONTINUE;
                        pkt_ptr[1] = 0x00;
                        pkt_ptr[2] = 0x05;
                        pkt_ptr[3] = OBEX_HI_SRM;
                        pkt_ptr[4] = 0x01;

                        obex_send_data(link, pkt_ptr, 5, false);
                        link->srm_status = SRM_ENABLE;
                    }
                }
            }

            if (link->cback)
            {
                T_OBEX_REMOTE_PUT_DATA data;

                data.handle = link;
                memcpy(data.bd_addr, link->bd_addr, 6);
                data.data_len = pkt_len - OBEX_HDR_PUT_LEN;
                data.p_data = p_pkt + OBEX_HDR_PUT_LEN;
                data.srm_status = link->srm_status;

                link->cback(OBEX_MSG_REMOTE_PUT, &data);
            }
        }
        break;

    case OBEX_OPCODE_FPUT:
        if (link->cback)
        {
            T_OBEX_REMOTE_PUT_DATA data;

            data.handle = link;
            memcpy(data.bd_addr, link->bd_addr, 6);
            data.data_len = pkt_len - OBEX_HDR_PUT_LEN;
            data.p_data = p_pkt + OBEX_HDR_PUT_LEN;
            data.srm_status = link->srm_status;

            link->cback(OBEX_MSG_REMOTE_PUT, &data);
        }
        break;

    case OBEX_OPCODE_ABORT:
        break;

    case OBEX_OPCODE_ACTION:
    case OBEX_OPCODE_FACTION:
    case OBEX_OPCODE_SESSION:
        {
            obex_send_rsp(link, OBEX_RSP_NOT_IMPLEMENT);
        }
        break;

    default :
        break;
    }
}

void obex_handle_rsp_pkt(T_OBEX_LINK *link,
                         uint8_t     *p_pkt,
                         uint16_t     pkt_len)
{
    uint8_t *p = p_pkt;
    uint8_t rsp_code;
    uint8_t *p_hdr_data;
    uint16_t hdr_data_len;

    BE_STREAM_TO_UINT8(rsp_code, p);

    PROTOCOL_PRINT_INFO3("obex_handle_rsp_pkt: response code 0x%x, pkt len %d, link state %d",
                         rsp_code, pkt_len, link->state);

    switch (link->state)
    {
    case OBEX_STATE_OBEX_CONNECTING:
        {
            uint8_t ret_flag = 0;
            uint8_t status;

            status = OBEX_SUCCESS;

            if (rsp_code == OBEX_RSP_SUCCESS)
            {
                p += 4;
                BE_STREAM_TO_UINT16(link->remote_max_pkt_len, p);

                if (obex_find_hdr_in_pkt(p_pkt + OBEX_CONNECT_LEN,
                                         pkt_len - OBEX_CONNECT_LEN,
                                         OBEX_HI_CONNECTION_ID,
                                         &p_hdr_data,
                                         &hdr_data_len))
                {
                    BE_STREAM_TO_UINT32(link->conn_id, p_hdr_data);
                }

                if (link->cback)
                {
                    T_OBEX_CONN_CMPL_DATA data;

                    data.handle = link;
                    data.max_pkt_len = link->remote_max_pkt_len;
                    memcpy(data.bd_addr, link->bd_addr, 6);

                    if (link->psm)
                    {
                        data.obex_over_l2c = true;
                    }
                    else
                    {
                        data.obex_over_l2c = false;
                    }

                    link->state = OBEX_STATE_CONNECTED;

                    link->cback(OBEX_MSG_CONN_CMPL, (void *)&data);
                }
            }
            else if (rsp_code == OBEX_RSP_UNAUTHORIZED)
            {
                if (obex_find_hdr_in_pkt(p_pkt + OBEX_CONNECT_LEN,
                                         pkt_len - OBEX_CONNECT_LEN,
                                         OBEX_HI_AUTHEN_CHALLENGE,
                                         &p_hdr_data,
                                         &hdr_data_len))
                {
                    T_OBEX_MD5 *md5_context;
                    uint8_t     password[5] = {0x3A, 0x30, 0x30, 0x30, 0x30};

                    md5_context = malloc(sizeof(T_OBEX_MD5));
                    if (md5_context == NULL)
                    {
                        status = OBEX_FAIL;
                    }
                    else
                    {
                        while (hdr_data_len)
                        {
                            uint8_t para_len;

                            if (*p_hdr_data == 0x00)
                            {
                                md5_context->buf[0] = 0x67452301;
                                md5_context->buf[1] = 0xefcdab89;
                                md5_context->buf[2] = 0x98badcfe;
                                md5_context->buf[3] = 0x10325476;
                                md5_context->bytes[0] = 0;
                                md5_context->bytes[1] = 0;
                                md5_update(md5_context, (p_hdr_data + 2), 16);
                                md5_update(md5_context, password, 5);
                                md5_final(md5_context);
                                ret_flag |= FLAG_AUTHEN_CHALLENGE;
                                break;
                            }
                            para_len = *(p_hdr_data + 1);
                            hdr_data_len -= (para_len + 2);
                            p_hdr_data += (para_len + 2);
                        }

                        if (ret_flag & FLAG_AUTHEN_CHALLENGE)
                        {
                            obex_connect_with_auth_rsp(link, md5_context->digest);
                        }
                        os_mem_free(md5_context);
                    }
                }
                else
                {
                    status = OBEX_FAIL;
                }
            }
            else
            {
                status = OBEX_FAIL;
            }

            if (status == OBEX_FAIL)
            {
                if (link->psm)
                {
                    mpa_send_l2c_disconn_req(link->cid);
                }
                else
                {
                    rfc_disconn_req(link->bd_addr, link->dlci);
                }
                link->state = OBEX_STATE_IDLE;
            }
        }
        break;

    case OBEX_STATE_SET_PATH:
        {
            T_OBEX_SET_PATH_CMPL_DATA data;

            data.handle = link;
            memcpy(data.bd_addr, link->bd_addr, 6);
            data.cause = rsp_code;
            if (link->cback)
            {
                link->cback(OBEX_MSG_SET_PATH_DONE, (void *)&data);
            }
        }
        break;

    case OBEX_STATE_PUT:
        {
            T_OBEX_PUT_CMPL_DATA data = {0};
            uint8_t *p_hdr_data;
            uint16_t hdr_data_len;
            bool ret;

            if (rsp_code == OBEX_RSP_SUCCESS || rsp_code == OBEX_RSP_CONTINUE)
            {
                if (rsp_code == OBEX_RSP_SUCCESS)
                {
                    link->state = OBEX_STATE_IDLE;
                }
                else if (rsp_code == OBEX_RSP_CONTINUE)
                {
                    uint8_t ret_flag = 0;

                    if (obex_find_hdr_in_pkt(p_pkt + OBEX_BASE_LEN,
                                             pkt_len - OBEX_BASE_LEN,
                                             OBEX_HI_SRM,
                                             &p_hdr_data,
                                             &hdr_data_len))
                    {
                        if (*p_hdr_data == 0x01)
                        {
                            ret_flag |= FLAG_SRM_ENABLE;
                        }
                    }
                    if (obex_find_hdr_in_pkt(p_pkt + OBEX_BASE_LEN,
                                             pkt_len - OBEX_BASE_LEN,
                                             OBEX_HI_SRMP,
                                             &p_hdr_data,
                                             &hdr_data_len))
                    {
                        if (*p_hdr_data == 0x01)
                        {
                            ret_flag |= FLAG_SRMP_WAIT;
                        }
                    }

                    if (link->srm_status == SRM_DISABLE)
                    {
                        if (ret_flag & FLAG_SRM_ENABLE)
                        {
                            if (ret_flag & FLAG_SRMP_WAIT)
                            {
                                link->srm_status = SRM_WAIT;
                            }
                            else
                            {
                                link->srm_status = SRM_ENABLE;
                            }
                        }
                    }
                    else if (link->srm_status == SRM_WAIT)
                    {
                        if ((ret_flag & FLAG_SRMP_WAIT) == 0)
                        {
                            link->srm_status = SRM_ENABLE;
                        }
                    }
                }

                ret = obex_find_hdr_in_pkt(p_pkt, pkt_len, OBEX_HI_NAME, &p_hdr_data, &hdr_data_len);
                if (ret)
                {
                    data.p_name = p_hdr_data;
                    data.name_len = hdr_data_len;
                }
            }

            data.handle = link;
            memcpy(data.bd_addr, link->bd_addr, 6);
            data.cause = rsp_code;
            data.srm_status = link->srm_status;

            if (link->cback)
            {
                link->cback(OBEX_MSG_PUT_DONE, (void *)&data);
            }
        }
        break;

    case OBEX_STATE_GET:
        {
            if (rsp_code == OBEX_RSP_SUCCESS || rsp_code == OBEX_RSP_CONTINUE)
            {
                if (rsp_code == OBEX_RSP_SUCCESS)
                {
                    link->state = OBEX_STATE_IDLE;
                }
                else if (rsp_code == OBEX_RSP_CONTINUE)
                {
                    uint8_t ret_flag = 0;

                    if (obex_find_hdr_in_pkt(p_pkt + OBEX_BASE_LEN,
                                             pkt_len - OBEX_BASE_LEN,
                                             OBEX_HI_SRM,
                                             &p_hdr_data,
                                             &hdr_data_len))
                    {
                        if (*p_hdr_data == 0x01)
                        {
                            ret_flag |= FLAG_SRM_ENABLE;
                        }
                    }
                    if (obex_find_hdr_in_pkt(p_pkt + OBEX_BASE_LEN,
                                             pkt_len - OBEX_BASE_LEN,
                                             OBEX_HI_SRMP,
                                             &p_hdr_data,
                                             &hdr_data_len))
                    {
                        if (*p_hdr_data == 0x01)
                        {
                            ret_flag |= FLAG_SRMP_WAIT;
                        }
                    }

                    if (link->srm_status == SRM_DISABLE)
                    {
                        if (ret_flag & FLAG_SRM_ENABLE)
                        {
                            if (ret_flag & FLAG_SRMP_WAIT)
                            {
                                link->srm_status = SRM_WAIT;
                            }
                            else
                            {
                                link->srm_status = SRM_ENABLE;
                            }
                        }
                    }
                    else if (link->srm_status == SRM_WAIT)
                    {
                        if ((ret_flag & FLAG_SRMP_WAIT) == 0)
                        {
                            link->srm_status = SRM_ENABLE;
                        }
                    }
                }

                if (link->cback)
                {
                    T_OBEX_GET_OBJECT_CMPL_DATA data;

                    data.handle = link;
                    memcpy(data.bd_addr, link->bd_addr, 6);
                    data.data_len = pkt_len - OBEX_BASE_LEN;
                    data.p_data = p_pkt + OBEX_BASE_LEN;
                    data.rsp_code = rsp_code;
                    data.srm_status = link->srm_status;

                    link->cback(OBEX_MSG_GET_OBJECT, &data);
                }
            }
            else
            {
                link->state = OBEX_STATE_IDLE;

                if (link->cback)
                {
                    T_OBEX_GET_OBJECT_CMPL_DATA data;

                    data.handle = link;
                    memcpy(data.bd_addr, link->bd_addr, 6);
                    data.data_len = 0;
                    data.p_data = NULL;
                    data.rsp_code = rsp_code;

                    link->cback(OBEX_MSG_GET_OBJECT, &data);
                }
            }
        }
        break;

    case OBEX_STATE_DISCONNECT:
        if (link->psm)
        {
            mpa_send_l2c_disconn_req(link->cid);
        }
        else
        {
            rfc_disconn_req(link->bd_addr, link->dlci);
        }
        break;

    case OBEX_STATE_ABORT:
        {
            T_OBEX_ABORT_CMPL_DATA data;

            link->state = OBEX_STATE_IDLE;

            data.handle = link;
            memcpy(data.bd_addr, link->bd_addr, 6);
            data.cause = rsp_code;
            if (link->cback)
            {
                link->cback(OBEX_MSG_ABORT_DONE, (void *)&data);
            }
        }
        break;

    default:
        break;
    }
}

void obex_handle_data_ind(T_OBEX_LINK *link,
                          uint8_t     *p_pkt,
                          uint16_t     pkt_len)
{
    if (link->p_rx_data != NULL)
    {
        if (pkt_len <= link->total_len - link->rx_len)
        {
            memcpy(link->p_rx_data + link->rx_len, p_pkt, pkt_len);
            link->rx_len += pkt_len;

            if (link->rx_len == link->total_len)
            {
                if (link->role == OBEX_ROLE_SERVER)
                {
                    obex_handle_req_pkt(link, link->p_rx_data, link->total_len);
                }
                else
                {
                    obex_handle_rsp_pkt(link, link->p_rx_data, link->total_len);
                }

                free(link->p_rx_data);
                link->p_rx_data = NULL;
                link->rx_len = 0;
                link->total_len = 0;
            }
        }
        else
        {
            PROTOCOL_PRINT_ERROR3("obex_handle_rcv_data: error pkt len %d, received data len %d, total data len %d",
                                  pkt_len, link->rx_len, link->total_len);
            free(link->p_rx_data);
            link->p_rx_data = NULL;
            link->rx_len = 0;
            link->total_len = 0;
        }
    }
    else
    {
        link->total_len = (p_pkt[1] << 8) | (p_pkt[2]);

        if (link->total_len > pkt_len)
        {
            // this should not hanppen since we limmitted max pkt len
            PROTOCOL_PRINT_ERROR3("obex_handle_rcv_data: max pkt len %d, received pkt len %d, total data len %d",
                                  link->local_max_pkt_len, pkt_len, link->total_len);

            link->p_rx_data = malloc(link->total_len);
            if (link->p_rx_data == NULL)
            {
                link->total_len = 0;
                return;
            }

            memcpy(link->p_rx_data, p_pkt, pkt_len);
            link->rx_len = pkt_len;
        }
        else
        {
            if (link->role == OBEX_ROLE_SERVER)
            {
                obex_handle_req_pkt(link, p_pkt, link->total_len);
            }
            else
            {
                obex_handle_rsp_pkt(link, p_pkt, link->total_len);
            }
        }
    }
}

void obex_rfc_cback(T_RFC_MSG_TYPE  msg_type,
                    void           *p_msg)
{
    T_OBEX_LINK *link;

    PROTOCOL_PRINT_TRACE1("obex_rfc_cback: msg_type 0x%02x", msg_type);

    switch (msg_type)
    {
    case RFC_CONN_IND:
        {
            T_RFC_CONN_IND *p_ind = (T_RFC_CONN_IND *)p_msg;

            link = obex_find_link_by_dlci(p_ind->bd_addr, p_ind->dlci);
            if (link)
            {
                rfc_conn_cfm(p_ind->bd_addr,
                             p_ind->dlci,
                             RFC_REJECT,
                             p_ind->frame_size,
                             RFC_DEFAULT_CREDIT);
                break;
            }

            link = obex_alloc_link(p_ind->bd_addr);
            if (link)
            {
                T_OBEX_RFC_CBACK *rfc_cback;

                link->psm = 0;
                link->dlci = p_ind->dlci;
                link->role = OBEX_ROLE_SERVER;

                rfc_cback = os_queue_peek(&obex->rfc_cback_list, 0);
                while (rfc_cback != NULL)
                {
                    if (rfc_cback->server_chann == ((link->dlci >> 1) & 0x1F))
                    {
                        link->cback = rfc_cback->cback;
                        break;
                    }

                    rfc_cback = rfc_cback->next;
                }

                if (p_ind->frame_size < OBEX_OVER_RFC_MAX_PKT_LEN)
                {
                    rfc_conn_cfm(p_ind->bd_addr,
                                 p_ind->dlci,
                                 RFC_ACCEPT,
                                 p_ind->frame_size,
                                 RFC_DEFAULT_CREDIT);
                }
                else
                {
                    rfc_conn_cfm(p_ind->bd_addr,
                                 p_ind->dlci,
                                 RFC_ACCEPT,
                                 OBEX_OVER_RFC_MAX_PKT_LEN,
                                 RFC_DEFAULT_CREDIT);
                }
            }
            else
            {
                rfc_conn_cfm(p_ind->bd_addr,
                             p_ind->dlci,
                             RFC_REJECT,
                             p_ind->frame_size,
                             RFC_DEFAULT_CREDIT);
            }
        }
        break;

    case RFC_CONN_CMPL:
        {
            T_RFC_CONN_CMPL *p_cmpl = (T_RFC_CONN_CMPL *)p_msg;

            link = obex_find_link_by_dlci(p_cmpl->bd_addr, p_cmpl->dlci);

            if (link)
            {
                link->state = OBEX_STATE_RFC_CONNECTED;
                link->local_max_pkt_len = p_cmpl->frame_size;

                if (link->role == OBEX_ROLE_CLIENT)
                {
                    if (p_cmpl->remain_credits)
                    {
                        obex_connect(link);
                    }
                    else
                    {
                        link->wait_credit_flag = 1;
                    }
                }
            }
            else
            {
                rfc_disconn_req(p_cmpl->bd_addr, p_cmpl->dlci);
            }
        }
        break;

    case RFC_DISCONN_CMPL:
        {
            T_RFC_DISCONN_CMPL *p_disc = (T_RFC_DISCONN_CMPL *)p_msg;

            link = obex_find_link_by_dlci(p_disc->bd_addr, p_disc->dlci);
            if (link)
            {
                T_OBEX_DISCONN_CMPL_DATA data;

                data.handle = link;
                data.cause = p_disc->cause;
                memcpy(data.bd_addr, link->bd_addr, 6);

                if (link->cback)
                {
                    link->cback(OBEX_MSG_DISCONN, &data);
                }

                obex_free_link(link);
            }
        }
        break;

    case RFC_CREDIT_INFO:
        {
            T_RFC_CREDIT_INFO *p_info = (T_RFC_CREDIT_INFO *)p_msg;

            link = obex_find_link_by_dlci(p_info->bd_addr, p_info->dlci);
            if (link)
            {
                if ((p_info->remain_credits) && (link->wait_credit_flag))
                {
                    obex_connect(link);
                    link->wait_credit_flag = 0;
                }
            }
        }
        break;

    case RFC_DATA_IND:
        {
            T_RFC_DATA_IND *p_ind = (T_RFC_DATA_IND *)p_msg;

            link = obex_find_link_by_dlci(p_ind->bd_addr, p_ind->dlci);
            if (link)
            {
                if ((p_ind->remain_credits) && (link->wait_credit_flag))
                {
                    obex_connect(link);
                    link->wait_credit_flag = 0;
                }

                if (p_ind->length)
                {
                    obex_handle_data_ind(link, p_ind->buf, p_ind->length);
                }
            }

            rfc_data_cfm(p_ind->bd_addr, p_ind->dlci, 1);
        }
        break;

    case RFC_DLCI_CHANGE:
        {
            T_RFC_DLCI_CHANGE_INFO *p_info = (T_RFC_DLCI_CHANGE_INFO *)p_msg;

            link = obex_find_link_by_dlci(p_info->bd_addr, p_info->prev_dlci);
            if (link)
            {
                link->dlci = p_info->curr_dlci;
            }
        }
        break;

    default:
        break;
    }
}

void obex_l2cap_callback(void        *p_buf,
                         T_PROTO_MSG  l2c_msg)
{
    T_OBEX_LINK *link;

    PROTOCOL_PRINT_TRACE1("obex_l2cap_callback: msg_type 0x%02x", l2c_msg);

    switch (l2c_msg)
    {
    case L2C_CONN_IND:
        {
            T_MPA_L2C_CONN_IND *p_ind = (T_MPA_L2C_CONN_IND *)p_buf;

            link = obex_alloc_link(p_ind->bd_addr);
            if (link == NULL)
            {
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE,
                                      p_ind->cid,
                                      672,
                                      MPA_L2C_MODE_ERTM,
                                      0xFFFF);
            }
            else
            {
                T_OBEX_L2C_CBACK *l2c_cback;

                link->cid = p_ind->cid;
                link->role = OBEX_ROLE_SERVER;

                l2c_cback = os_queue_peek(&obex->l2c_cback_list, 0);
                while (l2c_cback != NULL)
                {
                    if (l2c_cback->queue_id == p_ind->proto_id)
                    {
                        link->cback = l2c_cback->cback;
                        link->psm = l2c_cback->psm;
                        break;
                    }

                    l2c_cback = l2c_cback->next;
                }

                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT,
                                      p_ind->cid,
                                      OBEX_OVER_L2C_MAX_PKT_LEN,
                                      MPA_L2C_MODE_ERTM,
                                      0xFFFF);
            }
        }
        break;

    case L2C_CONN_RSP:
        {
            T_MPA_L2C_CONN_RSP *p_rsp = (T_MPA_L2C_CONN_RSP *)p_buf;

            link = obex_find_link_by_queue_id(p_rsp->bd_addr, p_rsp->proto_id);
            if (link)
            {
                if (p_rsp->cause == 0)
                {
                    link->cid = p_rsp->cid;
                }
                else
                {
                    T_OBEX_DISCONN_CMPL_DATA data;

                    data.handle = link;
                    data.cause = p_rsp->cause;
                    memcpy(data.bd_addr, link->bd_addr, 6);

                    if (link->cback)
                    {
                        link->cback(OBEX_MSG_DISCONN, &data);
                    }

                    obex_free_link(link);
                }
            }
        }
        break;

    case L2C_CONN_CMPL:
        {
            T_MPA_L2C_CONN_CMPL_INFO *p_info = (T_MPA_L2C_CONN_CMPL_INFO *)p_buf;

            link = obex_find_link_by_cid(p_info->cid);
            if (link)
            {
                if (p_info->cause)
                {
                    T_OBEX_DISCONN_CMPL_DATA data;

                    data.handle = link;
                    data.cause = p_info->cause;
                    memcpy(data.bd_addr, link->bd_addr, 6);

                    if (link->cback)
                    {
                        link->cback(OBEX_MSG_DISCONN, &data);
                    }

                    obex_free_link(link);
                }
                else
                {
                    obex->ds_data_offset = p_info->ds_data_offset;
                    link->state = OBEX_STATE_L2C_CONNECTED;
                    link->local_max_pkt_len = OBEX_OVER_L2C_MAX_PKT_LEN;

                    if (link->role == OBEX_ROLE_CLIENT)
                    {
                        obex_connect(link);
                    }
                }
            }
        }
        break;

    case L2C_DATA_IND:
        {
            T_MPA_L2C_DATA_IND *p_ind = (T_MPA_L2C_DATA_IND *)p_buf;

            link = obex_find_link_by_cid(p_ind->cid);
            if (link && p_ind->length)
            {
                obex_handle_data_ind(link, p_ind->data + p_ind->gap, p_ind->length);
            }
        }
        break;

    case L2C_DATA_RSP:
        {
            T_MPA_L2C_DATA_RSP *p_info = (T_MPA_L2C_DATA_RSP *)p_buf;

            link = obex_find_link_by_cid(p_info->cid);
            if (link)
            {
                if (link->state == OBEX_STATE_PUT)
                {
                    if (link->cback)
                    {
                        T_OBEX_PUT_DATA_RSP rsp;

                        memcpy(rsp.bd_addr, link->bd_addr, 6);
                        rsp.handle = link;
                        link->cback(OBEX_MSG_PUT_DATA_RSP, &rsp);
                    }
                }
            }
        }
        break;

    case L2C_DISCONN_RSP:
        {
            T_MPA_L2C_DISCONN_RSP *p_rsp = (T_MPA_L2C_DISCONN_RSP *)p_buf;

            link = obex_find_link_by_cid(p_rsp->cid);
            if (link)
            {
                T_OBEX_DISCONN_CMPL_DATA data;

                data.handle = link;
                data.cause = p_rsp->cause;
                memcpy(data.bd_addr, link->bd_addr, 6);

                if (link->cback)
                {
                    link->cback(OBEX_MSG_DISCONN, &data);
                }
                obex_free_link(link);
            }
        }
        break;

    case L2C_DISCONN_IND:
        {
            T_MPA_L2C_DISCONN_IND *p_ind = (T_MPA_L2C_DISCONN_IND *)p_buf;

            link = obex_find_link_by_cid(p_ind->cid);
            if (link)
            {
                T_OBEX_DISCONN_CMPL_DATA data;

                data.handle = link;
                data.cause = p_ind->cause;
                memcpy(data.bd_addr, link->bd_addr, 6);

                if (link->cback)
                {
                    link->cback(OBEX_MSG_DISCONN, &data);
                }

                obex_free_link(link);
            }

            mpa_send_l2c_disconn_cfm(p_ind->cid);
        }
        break;

    default:
        break;
    }
}

bool obex_conn_cfm(T_OBEX_HANDLE  handle,
                   bool           accept,
                   uint8_t       *p_cfm_data,
                   uint16_t       data_len)
{
    T_OBEX_LINK *link;

    link = (T_OBEX_LINK *)handle;
    if (link != NULL)
    {
        uint32_t conn_id;

        conn_id = obex_get_free_conn_id();
        if (conn_id != 0)
        {
            uint8_t  *p_buf;
            uint16_t  buf_len;

            buf_len = OBEX_CONNECT_LEN;
            if (accept)
            {
                buf_len += OBEX_HDR_CONN_ID_LEN + data_len;
            }

            p_buf = calloc(1, buf_len);
            if (p_buf != NULL)
            {
                uint8_t *p;

                p = p_buf;

                if (accept)
                {
                    BE_UINT8_TO_STREAM(p, OBEX_RSP_SUCCESS);
                }
                else
                {
                    BE_UINT8_TO_STREAM(p, OBEX_RSP_FORBIDDEN);
                }
                BE_UINT16_TO_STREAM(p, buf_len);
                BE_UINT8_TO_STREAM(p, OBEX_VERSION);
                BE_UINT8_TO_STREAM(p, 0x00);
                BE_UINT16_TO_STREAM(p, link->local_max_pkt_len);

                if (accept)
                {
                    BE_UINT8_TO_STREAM(p, OBEX_HI_CONNECTION_ID);
                    BE_UINT32_TO_STREAM(p, conn_id);
                    ARRAY_TO_STREAM(p, p_cfm_data, data_len);

                    link->conn_id = conn_id;
                }

                if (obex_send_data(link, p_buf, buf_len, false) == true)
                {
                    if (accept)
                    {
                        if (link->cback)
                        {
                            T_OBEX_CONN_CMPL_DATA data;

                            data.handle = link;
                            data.max_pkt_len = link->local_max_pkt_len;
                            memcpy(data.bd_addr, link->bd_addr, 6);
                            if (link->psm)
                            {
                                data.obex_over_l2c = true;
                            }
                            else
                            {
                                data.obex_over_l2c = false;
                            }

                            link->state = OBEX_STATE_CONNECTED;

                            link->cback(OBEX_MSG_CONN_CMPL, (void *)&data);
                        }
                    }

                    free(p_buf);

                    return true;
                }

                free(p_buf);
            }
        }
    }

    return false;
}

bool obex_disconn_req(T_OBEX_HANDLE handle)
{
    T_OBEX_LINK *link;

    link = (T_OBEX_LINK *)handle;
    if (link != NULL)
    {
        if ((link->state != OBEX_STATE_L2C_CONNECTED) &&
            (link->state != OBEX_STATE_RFC_CONNECTED) &&
            (link->state != OBEX_STATE_OBEX_CONNECTING))
        {
            if (link->conn_id != 0)
            {
                uint8_t pkt_ptr[8];     //OBEX_BASE_LEN + OBEX_HDR_CONN_ID_LEN

                pkt_ptr[0] = OBEX_OPCODE_DISCONNECT;
                pkt_ptr[1] = 0x00;
                pkt_ptr[2] = 0x08;
                pkt_ptr[3] = OBEX_HI_CONNECTION_ID;
                pkt_ptr[4] = (uint8_t)(link->conn_id >> 24);
                pkt_ptr[5] = (uint8_t)(link->conn_id >> 16);
                pkt_ptr[6] = (uint8_t)(link->conn_id >> 8);
                pkt_ptr[7] = (uint8_t)(link->conn_id);
                obex_send_data(link, pkt_ptr, 8, false);
            }
            else
            {
                uint8_t pkt_ptr[3];     //OBEX_BASE_LEN

                pkt_ptr[0] = OBEX_OPCODE_DISCONNECT;
                pkt_ptr[1] = 0x00;
                pkt_ptr[2] = 0x03;
                obex_send_data(link, pkt_ptr, 3, false);
            }

            link->state = OBEX_STATE_DISCONNECT;
        }
        else if (link->psm)
        {
            mpa_send_l2c_disconn_req(link->cid);
        }
        else
        {
            rfc_disconn_req(link->bd_addr, link->dlci);
        }

        return true;
    }

    return false;
}

bool obex_init(void)
{
    int32_t ret = 0;

    if (obex == NULL)
    {
        obex = calloc(1, sizeof(T_OBEX));
        if (obex == NULL)
        {
            ret = 1;
            goto fail_alloc_obex;
        }

        if (rfc_cback_register(0xFF, obex_rfc_cback, &obex->service_id) == false)
        {
            ret = 2;
            goto fail_register_rfc;
        }

        os_queue_init(&obex->link_queue);
        os_queue_init(&obex->rfc_cback_list);
        os_queue_init(&obex->l2c_cback_list);
    }

    return true;

fail_register_rfc:
    free(obex);
    obex = NULL;
fail_alloc_obex:
    PROFILE_PRINT_ERROR1("obex_init: failed %d", -ret);
    return false;
}

bool obex_conn_req_over_rfc(uint8_t            bd_addr[6],
                            uint8_t           *p_param,
                            uint8_t            server_chann,
                            P_OBEX_CBACK       cback,
                            T_OBEX_HANDLE     *p_handle)
{
    T_OBEX_LINK *link;
    uint8_t      dlci;
    int32_t      ret = 0;

    if (obex == NULL)
    {
        if (obex_init() == false)
        {
            ret = 1;
            goto fail_init_obex;
        }
    }

    link = obex_find_link_by_server_chann(bd_addr, server_chann, OBEX_ROLE_CLIENT);
    if (link != NULL)
    {
        ret = 2;
        goto fail_link_exist;
    }

    link = obex_alloc_link(bd_addr);
    if (link == NULL)
    {
        ret = 3;
        goto fail_alloc_link;
    }

    link->conn_param = p_param;
    link->psm = 0;
    link->server_chann = server_chann;
    link->cback = cback;
    link->role = OBEX_ROLE_CLIENT;

    if (rfc_conn_req(bd_addr,
                     server_chann,
                     obex->service_id,
                     OBEX_OVER_RFC_MAX_PKT_LEN,
                     7,
                     &dlci) == false)
    {
        ret = 4;
        goto fail_send_rfc_conn_req;
    }

    *p_handle = link;
    link->dlci = dlci;
    return true;

fail_send_rfc_conn_req:
    obex_free_link(link);
fail_alloc_link:
fail_link_exist:
fail_init_obex:
    PROTOCOL_PRINT_ERROR2("obex_conn_req_over_rfc: failed %d, server_chann %d", -ret, server_chann);
    return false;
}

bool obex_register_over_rfc(uint8_t      chann_num,
                            P_OBEX_CBACK callback)
{
    T_OBEX_RFC_CBACK *rfc_cback;
    int32_t           ret = 0;

    if (obex == NULL)
    {
        if (obex_init() == false)
        {
            ret = 1;
            goto fail_init_obex;
        }
    }

    rfc_cback = os_queue_peek(&obex->rfc_cback_list, 0);
    while (rfc_cback != NULL)
    {
        if (rfc_cback->server_chann == chann_num)
        {
            ret = 2;
            goto fail_invalid_chann_num;
        }
        rfc_cback = rfc_cback->next;
    }

    rfc_cback = calloc(1, sizeof(T_OBEX_RFC_CBACK));
    if (rfc_cback == NULL)
    {
        ret = 3;
        goto fail_alloc_cb;
    }

    if (rfc_cback_register(chann_num, obex_rfc_cback, &rfc_cback->service_id) == false)
    {
        ret = 4;
        goto fail_rfc_reg_cb;
    }

    rfc_cback->server_chann = chann_num;
    rfc_cback->cback = callback;
    os_queue_in(&obex->rfc_cback_list, rfc_cback);

    return true;

fail_rfc_reg_cb:
    free(rfc_cback);
fail_alloc_cb:
fail_invalid_chann_num:
fail_init_obex:
    PROTOCOL_PRINT_ERROR2("obex_register_over_rfc: failed %d, chann_num %d", -ret, chann_num);
    return false;
}

bool obex_conn_req_over_l2c(uint8_t        bd_addr[6],
                            uint8_t       *p_param,
                            uint16_t       psm,
                            P_OBEX_CBACK   cback,
                            T_OBEX_HANDLE *p_handle)
{
    T_OBEX_LINK *link;
    int32_t      ret = 0;

    if (obex == NULL)
    {
        if (obex_init() == false)
        {
            ret = 1;
            goto fail_init_obex;
        }
    }

    link = obex_alloc_link(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (mpa_reg_l2c_proto(psm, obex_l2cap_callback, &link->queue_id) == false)
    {
        ret = 3;
        goto fail_reg_l2c_proto;
    }

    mpa_send_l2c_conn_req(psm,
                          psm,
                          link->queue_id,
                          OBEX_OVER_L2C_MAX_PKT_LEN,
                          bd_addr,
                          MPA_L2C_MODE_ERTM,
                          0xFFFF);

    link->conn_param = p_param;
    link->cback = cback;
    link->role = OBEX_ROLE_CLIENT;
    link->psm = psm;

    *p_handle = link;

    return true;

fail_reg_l2c_proto:
    obex_free_link(link);
fail_alloc_link:
fail_init_obex:
    PROTOCOL_PRINT_ERROR2("obex_conn_req_over_l2c: failed %d, psm 0x%04x", -ret, psm);
    return false;
}

bool obex_register_over_l2c(uint16_t     psm,
                            P_OBEX_CBACK callback)
{
    T_OBEX_L2C_CBACK *l2c_cback;
    int32_t           ret = 0;

    if (obex == NULL)
    {
        if (obex_init() == false)
        {
            ret = 1;
            goto fail_init_obex;
        }
    }

    l2c_cback = os_queue_peek(&obex->l2c_cback_list, 0);
    while (l2c_cback != NULL)
    {
        if (l2c_cback->psm == psm)
        {
            ret = 2;
            goto fail_invalid_psm;
        }
        l2c_cback = l2c_cback->next;
    }

    l2c_cback = calloc(1, sizeof(T_OBEX_L2C_CBACK));
    if (l2c_cback == NULL)
    {
        ret = 3;
        goto fail_alloc_cb;
    }

    if (mpa_reg_l2c_proto(psm, obex_l2cap_callback, &l2c_cback->queue_id) == false)
    {
        ret = 4;
        goto fail_reg_l2c_proto;
    }

    l2c_cback->psm = psm;
    l2c_cback->cback = callback;
    os_queue_in(&obex->l2c_cback_list, l2c_cback);

    return true;

fail_reg_l2c_proto:
    free(l2c_cback);
fail_alloc_cb:
fail_invalid_psm:
fail_init_obex:
    PROTOCOL_PRINT_ERROR2("obex_register_over_l2c: failed %d, psm 0x%04x", -ret, psm);
    return false;
}

bool obex_roleswap_info_get(T_OBEX_HANDLE         handle,
                            T_OBEX_ROLESWAP_INFO *info)
{
    T_OBEX_LINK *link;

    link = (T_OBEX_LINK *)handle;
    if (link != NULL)
    {
        info->conn_id = link->conn_id;
        info->local_max_pkt_len = link->local_max_pkt_len;
        info->remote_max_pkt_len = link->remote_max_pkt_len;
        info->role = link->role;
        info->state = link->state;
        info->psm = link->psm;
        info->cid = link->cid;

        if (info->psm)
        {
            info->data_offset = obex->ds_data_offset;
            return true;
        }
        else
        {
            info->rfc_dlci = link->dlci;
            return rfc_get_cid(link->bd_addr, info->rfc_dlci, &info->cid);
        }
    }

    return false;
}

bool obex_roleswap_info_set(uint8_t               bd_addr[6],
                            P_OBEX_CBACK          cback,
                            T_OBEX_ROLESWAP_INFO *info,
                            T_OBEX_HANDLE        *handle)
{
    T_OBEX_LINK *link;

    if (info->psm)
    {
        link = obex_find_link_by_cid(info->cid);
    }
    else
    {
        link = obex_find_link_by_dlci(bd_addr, info->rfc_dlci);
    }

    if (link == NULL)
    {
        link = obex_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        link->conn_id = info->conn_id;
        link->cid = info->cid;
        link->state = info->state;
        link->cback = cback;
        link->role = info->role;
        link->remote_max_pkt_len = info->remote_max_pkt_len;
        link->local_max_pkt_len = info->local_max_pkt_len;
        link->psm = info->psm;

        *handle = link;

        if (link->psm)
        {
            obex->ds_data_offset = info->data_offset;

            if (mpa_reg_l2c_proto(link->psm, obex_l2cap_callback, &link->queue_id) == false)
            {
                obex_free_link(link);
                return false;
            }
        }
        else
        {
            link->dlci = info->rfc_dlci;
        }

        return true;
    }

    return false;
}

bool obex_roleswap_info_del(T_OBEX_HANDLE handle)
{
    T_OBEX_LINK *link;

    link = (T_OBEX_LINK *)handle;
    if (link != NULL)
    {
        obex_free_link(link);
        return true;
    }

    return false;
}

uint8_t obex_service_id_get(void)
{
    return obex->service_id;
}
