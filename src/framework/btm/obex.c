/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "mpa.h"
#include "rfc.h"
#include "obex.h"

#define OBEX_PROFILE_NUM                4
#define OBEX_OVER_RFC_MAX_PKT_LEN       1011    // 1021-4(l2c hdr)-5(rfc hdr)-1(crc)
#define OBEX_OVER_L2C_MAX_PKT_LEN       1015    // 1021-6(l2c ertm hdr/fcs)

typedef struct t_obex_link
{
    struct t_obex_link *p_next;

    const uint8_t      *con_param_ptr;
    uint32_t            connection_id;

    uint8_t            *p_rx_data;
    uint16_t            rx_len;
    uint16_t            total_len;

    uint8_t             bd_addr[6];
    uint16_t            cid;
    uint16_t            psm;
    uint8_t             dlci;
    T_OBEX_STATE        state;
    uint8_t             wait_credit_flag;
    uint8_t             srm_status;
    uint8_t             server_chann;
    uint8_t             queue_id;
    P_OBEX_PROFILE_CB   cb;

    uint16_t            local_max_pkt_len;
    uint16_t            remote_max_pkt_len;
    T_OBEX_ROLE         role;
} T_OBEX_LINK;

typedef struct t_obex
{
    T_OBEX_RFC_PROFILE *rfc_prof_cb;
    T_OBEX_L2C_PROFILE *l2c_prof_cb;
    T_OS_QUEUE          link_queue;

    uint16_t            ds_data_offset;
    uint8_t             rfc_index;
} T_OBEX;

T_OBEX *p_obex;

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

void md5_update(T_MD5         *md5_context,
                const uint8_t *in_ptr,
                uint16_t       in_len)
{
    uint32_t         temp[16];
    uint16_t         mdi;
    uint16_t         i, j;

    /* compute number of bytes mod 64 */
    mdi = (int)((md5_context->bytes[0] >> 3) & 0x3F);

    /* update number of bits */
    if ((md5_context->bytes[0] + ((uint32_t)in_len << 3)) < md5_context->bytes[0])
    {
        md5_context->bytes[1]++;
    }
    md5_context->bytes[0] += ((uint32_t)in_len << 3);
    //md5_context->bytes[1] += ((uint32_t)in_len >> 29);

    while (in_len--)
    {
        /* add new character to buffer, increment mdi */
        md5_context->in[mdi] = *in_ptr++;
        mdi++;

        /* transform if necessary */
        if (mdi == 0x40)
        {
            for (i = 0, j = 0; i < 16; i++, j += 4)
            {
                temp[i] = (((uint32_t)md5_context->in[j + 3]) << 24) | (((uint32_t)md5_context->in[j + 2]) << 16)
                          | (((uint32_t)md5_context->in[j + 1]) << 8) | ((uint32_t)md5_context->in[j]);
            }
            transfrom(md5_context->buf, temp);
            mdi = 0;
        }
    }
}

void md5_final(T_MD5 *md5_context)
{
    uint32_t         in[16];
    uint16_t         mdi;
    uint32_t         i, j;
    uint16_t         pad_len;

    /* save number of bits */
    in[14] = md5_context->bytes[0];
    in[15] = md5_context->bytes[1];

    /* compute number of bytes mod 64 */
    mdi = (uint16_t)((md5_context->bytes[0] >> 3) & 0x3F);

    /* pad out to 56 mod 64 */
    pad_len = (mdi < 56) ? (56 - mdi) : (120 - mdi);
    md5_update(md5_context, md5_padding, pad_len);

    /* append length in bits and transform */
    for (i = 0, j = 0; i < 14; i++, j += 4)
    {
        in[i] = (((uint32_t)md5_context->in[j + 3]) << 24) | (((uint32_t)md5_context->in[j + 2]) << 16)
                | (((uint32_t)md5_context->in[j + 1]) << 8) | ((uint32_t)md5_context->in[j]);
    }
    transfrom(md5_context->buf, in);

    /* store buffer in digest */
    for (i = 0, j = 0; i < 4; i++, j += 4)
    {
        md5_context->digest[j] = (uint8_t)(md5_context->buf[i]);
        md5_context->digest[j + 1] = (uint8_t)(md5_context->buf[i] >> 8);
        md5_context->digest[j + 2] = (uint8_t)(md5_context->buf[i] >> 16);
        md5_context->digest[j + 3] = (uint8_t)(md5_context->buf[i] >> 24);
    }
}

T_OBEX_LINK *obex_alloc_link(uint8_t bd_addr[6])
{
    T_OBEX_LINK *p_link;

    p_link = os_mem_zalloc2(sizeof(T_OBEX_LINK));
    if (p_link != NULL)
    {
        memcpy(p_link->bd_addr, bd_addr, 6);
        p_link->state = OBEX_STATE_IDLE;

        os_queue_in(&p_obex->link_queue, p_link);
    }

    return p_link;
}

void obex_free_link(T_OBEX_LINK *p_link)
{
    if (p_link != NULL)
    {
        os_queue_delete(&p_obex->link_queue, p_link);
        os_mem_free(p_link);
    }
}

T_OBEX_LINK *obex_find_link_by_server_chann(uint8_t     bd_addr[6],
                                            uint8_t     server_chann,
                                            T_OBEX_ROLE role)
{
    T_OBEX_LINK *p_link;

    p_link = os_queue_peek(&p_obex->link_queue, 0);
    while (p_link != NULL)
    {
        if (memcmp(p_link->bd_addr, bd_addr, 6) == 0 &&
            p_link->server_chann == server_chann &&
            p_link->role == role)
        {
            return p_link;
        }
        p_link = p_link->p_next;
    }

    return NULL;
}

T_OBEX_LINK *obex_find_link_by_dlci(uint8_t bd_addr[6],
                                    uint8_t dlci)
{
    T_OBEX_LINK *p_link;

    p_link = os_queue_peek(&p_obex->link_queue, 0);
    while (p_link != NULL)
    {
        if (memcmp(p_link->bd_addr, bd_addr, 6) == 0 &&
            p_link->dlci == dlci)
        {
            return p_link;
        }
        p_link = p_link->p_next;
    }

    return NULL;
}

T_OBEX_LINK *obex_find_link_by_queue_id(uint8_t bd_addr[6],
                                        uint8_t queue_id)
{
    T_OBEX_LINK *p_link;

    p_link = os_queue_peek(&p_obex->link_queue, 0);
    while (p_link != NULL)
    {
        if (memcmp(p_link->bd_addr, bd_addr, 6) == 0 && p_link->queue_id == queue_id)
        {
            return p_link;
        }
        p_link = p_link->p_next;
    }

    return NULL;
}

T_OBEX_LINK *obex_find_link_by_cid(uint16_t cid)
{
    T_OBEX_LINK *p_link;

    p_link = os_queue_peek(&p_obex->link_queue, 0);
    while (p_link != NULL)
    {
        if (p_link->cid == cid)
        {
            return p_link;
        }
        p_link = p_link->p_next;
    }

    return NULL;
}

T_OBEX_LINK *obex_find_link_by_conn_id(uint32_t conn_id)
{
    T_OBEX_LINK *p_link;

    p_link = os_queue_peek(&p_obex->link_queue, 0);
    while (p_link != NULL)
    {
        if (p_link->connection_id == conn_id)
        {
            return p_link;
        }
        p_link = p_link->p_next;
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

bool obex_send_data(T_OBEX_LINK *p_link,
                    uint8_t     *p_data,
                    uint16_t     data_len,
                    bool         ack)
{
    if (p_link->psm)
    {
        uint8_t *p_buf;

        p_buf = mpa_get_l2c_buf(p_link->queue_id, p_link->cid, 0, data_len, p_obex->ds_data_offset, ack);
        if (p_buf != NULL)
        {
            memcpy(&p_buf[p_obex->ds_data_offset], p_data, data_len);
            mpa_send_l2c_data_req(p_buf, p_obex->ds_data_offset, p_link->cid, data_len, false);

            return true;
        }
    }
    else
    {
        return rfc_data_req(p_link->bd_addr, p_link->dlci, p_data, data_len, ack);
    }

    return false;
}

void obex_connect(T_OBEX_LINK *p_link)
{
    uint16_t    pkt_len;
    uint8_t    *pkt_ptr;
    uint16_t    param_len;
    uint8_t     index;

    param_len = p_link->con_param_ptr[0];
    pkt_len = OBEX_CONNECT_LEN + param_len;

    pkt_ptr = os_mem_zalloc2(pkt_len);
    if (pkt_ptr == NULL)
    {
        return;
    }

    pkt_ptr[0] = OBEX_OPCODE_CONNECT;
    pkt_ptr[1] = (uint8_t)(pkt_len >> 8);
    pkt_ptr[2] = (uint8_t)pkt_len;
    pkt_ptr[3] = OBEX_VERSION;
    pkt_ptr[4] = 0x00;
    pkt_ptr[5] = (uint8_t)(p_link->local_max_pkt_len >> 8);
    pkt_ptr[6] = (uint8_t)p_link->local_max_pkt_len;

    index = OBEX_CONNECT_LEN;
    memcpy(&pkt_ptr[index], &p_link->con_param_ptr[1], param_len);

    obex_send_data(p_link, pkt_ptr, pkt_len, false);
    os_mem_free(pkt_ptr);
    p_link->state = OBEX_STATE_OBEX_CONNECTING;
}

void obex_connect_with_auth_rsp(T_OBEX_LINK *p_link,
                                uint8_t     *auth_rsp)
{
    uint16_t pkt_len;
    uint8_t *pkt_ptr;
    uint32_t index;
    uint8_t  param_len;

    param_len = p_link->con_param_ptr[0];
    pkt_len = OBEX_CONNECT_LEN + OBEX_HDR_AUTHEN_RSP_LEN + param_len;
    pkt_ptr = os_mem_alloc2(pkt_len);
    if (pkt_ptr == NULL)
    {
        return;
    }

    pkt_ptr[0] = OBEX_OPCODE_CONNECT;
    pkt_ptr[1] = (uint8_t)(pkt_len >> 8);
    pkt_ptr[2] = (uint8_t)pkt_len;
    pkt_ptr[3] = OBEX_VERSION;
    pkt_ptr[4] = 0x00;
    pkt_ptr[5] = (uint8_t)(p_link->local_max_pkt_len >> 8);
    pkt_ptr[6] = (uint8_t)p_link->local_max_pkt_len;

    index = OBEX_CONNECT_LEN;
    pkt_ptr[index] = OBEX_HI_AUTHEN_RSP;
    pkt_ptr[index + 1] = (uint8_t)OBEX_HDR_AUTHEN_RSP_LEN >> 8;
    pkt_ptr[index + 2] = (uint8_t)OBEX_HDR_AUTHEN_RSP_LEN;
    pkt_ptr[index + 3] = 0x00;
    pkt_ptr[index + 4] = 0x10;
    memcpy(&pkt_ptr[index + 5], auth_rsp, 16);
    index += OBEX_HDR_AUTHEN_RSP_LEN;
    memcpy(&pkt_ptr[index], &p_link->con_param_ptr[1], param_len);

    obex_send_data(p_link, pkt_ptr, pkt_len, false);
    os_mem_free(pkt_ptr);
}

bool obex_set_path(T_OBEX_HANDLE  handle,
                   uint8_t        flag,
                   const uint8_t *path_ptr,
                   uint16_t       path_len)
{
    T_OBEX_LINK *p_link;
    uint8_t *pkt_ptr;
    uint16_t pkt_len;
    bool ret;

    p_link = (T_OBEX_LINK *)handle;
    if (p_link == NULL)
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
    pkt_ptr[6] = (uint8_t)(p_link->connection_id >> 24);
    pkt_ptr[7] = (uint8_t)(p_link->connection_id >> 16);
    pkt_ptr[8] = (uint8_t)(p_link->connection_id >> 8);
    pkt_ptr[9] = (uint8_t)(p_link->connection_id);
    pkt_ptr[10] = OBEX_HI_NAME;
    pkt_ptr[11] = (uint8_t)((OBEX_HDR_PREFIXED_LEN_BASE + path_len) >> 8);
    pkt_ptr[12] = (uint8_t)(OBEX_HDR_PREFIXED_LEN_BASE + path_len);
    if (path_len)
    {
        memcpy(&pkt_ptr[13], path_ptr, path_len);
    }

    ret = obex_send_data(p_link, pkt_ptr, pkt_len, false);

    if (ret == true)
    {
        p_link->state = OBEX_STATE_SET_PATH;
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
    T_OBEX_LINK *p_link;
    uint8_t *p_buf;
    uint16_t buf_len;
    uint8_t *p;
    bool ret;

    p_link = (T_OBEX_LINK *)handle;
    if (p_link == NULL)
    {
        return false;
    }

    buf_len = OBEX_HDR_PUT_LEN + header_len;

    if (p_link->connection_id != 0)
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

    if (p_link->connection_id != 0)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_CONNECTION_ID);
        BE_UINT32_TO_STREAM(p, p_link->connection_id);
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

    ret = obex_send_data(p_link, p_buf, buf_len, ack);

    if (ret == true)
    {
        p_link->state = OBEX_STATE_PUT;
    }

    os_mem_free(p_buf);

    return ret;
}

bool obex_get_object(T_OBEX_HANDLE  handle,
                     uint8_t       *content_ptr,
                     uint16_t       content_len)
{
    T_OBEX_LINK *p_link;
    uint8_t     *pkt_ptr;
    uint16_t     pkt_len;
    uint32_t     index;
    bool ret;

    p_link = (T_OBEX_LINK *)handle;
    if (p_link == NULL)
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
    pkt_ptr[index + 1] = (uint8_t)(p_link->connection_id >> 24);
    pkt_ptr[index + 2] = (uint8_t)(p_link->connection_id >> 16);
    pkt_ptr[index + 3] = (uint8_t)(p_link->connection_id >> 8);
    pkt_ptr[index + 4] = (uint8_t)(p_link->connection_id);
    index += OBEX_HDR_CONN_ID_LEN;
    p_link->srm_status = SRM_DISABLE;
    memcpy(&pkt_ptr[index], content_ptr, content_len);

    ret = obex_send_data(p_link, pkt_ptr, pkt_len, false);
    os_mem_free(pkt_ptr);

    if (ret)
    {
        p_link->state = OBEX_STATE_GET;
    }

    return ret;
}

bool obex_send_rsp(T_OBEX_HANDLE handle,
                   uint8_t       rsp_code)
{
    T_OBEX_LINK *p_link;
    uint8_t pkt_ptr[3];

    p_link = (T_OBEX_LINK *)handle;
    if (p_link == NULL)
    {
        return false;
    }

    pkt_ptr[0] = rsp_code;
    pkt_ptr[1] = 0x00;
    pkt_ptr[2] = 0x03;

    return obex_send_data(p_link, pkt_ptr, 3, false);
}

bool obex_get_object_continue(T_OBEX_HANDLE handle)
{
    T_OBEX_LINK *p_link;
    uint8_t pkt_ptr[3];

    p_link = (T_OBEX_LINK *)handle;
    if (p_link == NULL)
    {
        return false;
    }

    pkt_ptr[0] = OBEX_OPCODE_FGET;
    pkt_ptr[1] = 0x00;
    pkt_ptr[2] = 0x03;

    return obex_send_data(p_link, pkt_ptr, 3, false);
}

bool obex_abort(T_OBEX_HANDLE handle)
{
    T_OBEX_LINK *p_link;
    uint8_t pkt_ptr[3];
    bool ret;

    p_link = (T_OBEX_LINK *)handle;
    if (p_link == NULL)
    {
        return false;
    }

    pkt_ptr[0] = OBEX_OPCODE_ABORT;
    pkt_ptr[1] = 0x00;
    pkt_ptr[2] = 0x03;
    ret = obex_send_data(p_link, pkt_ptr, 3, false);

    if (ret == true)
    {
        p_link->state = OBEX_STATE_ABORT;
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

void obex_handle_req_pkt(T_OBEX_LINK *p_link,
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
        if (p_link->cb)
        {
            uint8_t *p_hdr_data;
            uint16_t hdr_data_len;
            T_OBEX_CONN_IND_DATA data;

            if (obex_find_hdr_in_pkt(p_pkt + OBEX_HDR_CONNECT_LEN, pkt_len - OBEX_HDR_CONNECT_LEN,
                                     OBEX_HI_SRM, &p_hdr_data, &hdr_data_len))
            {
                if (*p_hdr_data == 0x01)
                {
                    p_link->srm_status = SRM_ENABLE;
                }
            }

            data.handle = p_link;
            memcpy(data.bd_addr, p_link->bd_addr, 6);

            p += 4; //pkt len(2), version(1), flag(1)
            BE_STREAM_TO_UINT16(p_link->remote_max_pkt_len, p);

            p_link->cb(OBEX_CB_MSG_CONN_IND, &data);
        }
        break;

    case OBEX_OPCODE_DISCONNECT:
        {
            uint8_t pkt_ptr[3];

            pkt_ptr[0] = OBEX_RSP_SUCCESS;
            pkt_ptr[1] = 0x00;
            pkt_ptr[2] = 0x03;
            obex_send_data(p_link, pkt_ptr, 3, false);
        }
        break;

    case OBEX_OPCODE_PUT:
        {
            if (p_link->psm && p_link->srm_status == SRM_DISABLE)
            {
                uint8_t *p_hdr_data;
                uint16_t hdr_data_len;
                uint8_t  pkt_ptr[5];

                if (obex_find_hdr_in_pkt(p_pkt + OBEX_HDR_PUT_LEN, pkt_len - OBEX_HDR_PUT_LEN,
                                         OBEX_HI_SRM, &p_hdr_data, &hdr_data_len))
                {
                    if (*p_hdr_data == 0x01)
                    {
                        pkt_ptr[0] = OBEX_RSP_CONTINUE;
                        pkt_ptr[1] = 0x00;
                        pkt_ptr[2] = 0x05;
                        pkt_ptr[3] = OBEX_HI_SRM;
                        pkt_ptr[4] = 0x01;

                        obex_send_data(p_link, pkt_ptr, 5, false);
                        p_link->srm_status = SRM_ENABLE;
                    }
                }
            }

            if (p_link->cb)
            {
                T_OBEX_REMOTE_PUT_DATA data;

                data.handle = p_link;
                memcpy(data.bd_addr, p_link->bd_addr, 6);
                data.data_len = pkt_len - OBEX_HDR_PUT_LEN;
                data.p_data = p_pkt + OBEX_HDR_PUT_LEN;
                data.srm_status = p_link->srm_status;

                p_link->cb(OBEX_CB_MSG_REMOTE_PUT, &data);
            }
        }
        break;

    case OBEX_OPCODE_FPUT:
        if (p_link->cb)
        {
            T_OBEX_REMOTE_PUT_DATA data;

            data.handle = p_link;
            memcpy(data.bd_addr, p_link->bd_addr, 6);
            data.data_len = pkt_len - OBEX_HDR_PUT_LEN;
            data.p_data = p_pkt + OBEX_HDR_PUT_LEN;
            data.srm_status = p_link->srm_status;

            p_link->cb(OBEX_CB_MSG_REMOTE_PUT, &data);
        }
        break;

    case OBEX_OPCODE_ABORT:
        break;

    case OBEX_OPCODE_ACTION:
    case OBEX_OPCODE_FACTION:
    case OBEX_OPCODE_SESSION:
        {
            obex_send_rsp(p_link, OBEX_RSP_NOT_IMPLEMENT);
        }
        break;

    default :
        break;
    }
}

void obex_handle_rsp_pkt(T_OBEX_LINK *p_link,
                         uint8_t     *p_pkt,
                         uint16_t     pkt_len)
{
    uint8_t *p = p_pkt;
    uint8_t rsp_code;
    uint8_t *p_hdr_data;
    uint16_t hdr_data_len;

    BE_STREAM_TO_UINT8(rsp_code, p);

    PROTOCOL_PRINT_INFO3("obex_handle_rsp_pkt: response code 0x%x, pkt len %d, link state %d",
                         rsp_code, pkt_len, p_link->state);

    switch (p_link->state)
    {
    case OBEX_STATE_OBEX_CONNECTING:
        {
            uint8_t ret_flag = 0;
            uint8_t status;

            status = OBEX_SUCCESS;

            if (rsp_code == OBEX_RSP_SUCCESS)
            {
                p += 4;
                BE_STREAM_TO_UINT16(p_link->remote_max_pkt_len, p);

                if (obex_find_hdr_in_pkt(p_pkt + OBEX_CONNECT_LEN, pkt_len - OBEX_CONNECT_LEN,
                                         OBEX_HI_CONNECTION_ID, &p_hdr_data, &hdr_data_len))
                {
                    BE_STREAM_TO_UINT32(p_link->connection_id, p_hdr_data);
                }

                PROTOCOL_PRINT_INFO2("obex_handle_rsp_pkt: obex connected, remote max len %d, connection id 0x%x",
                                     p_link->remote_max_pkt_len, p_link->connection_id);

                if (p_link->cb)
                {
                    T_OBEX_CONN_CMPL_DATA data;

                    data.handle = p_link;
                    data.max_pkt_len = p_link->remote_max_pkt_len;
                    memcpy(data.bd_addr, p_link->bd_addr, 6);

                    if (p_link->psm)
                    {
                        data.obex_over_l2c = true;
                    }
                    else
                    {
                        data.obex_over_l2c = false;
                    }

                    p_link->state = OBEX_STATE_CONNECTED;

                    p_link->cb(OBEX_CB_MSG_CONN_CMPL, (void *)&data);
                }
            }
            else if (rsp_code == OBEX_RSP_UNAUTHORIZED)
            {
                if (obex_find_hdr_in_pkt(p_pkt + OBEX_CONNECT_LEN, pkt_len - OBEX_CONNECT_LEN,
                                         OBEX_HI_AUTHEN_CHALLENGE, &p_hdr_data, &hdr_data_len))
                {
                    T_MD5     *md5_context;
                    uint8_t    password[5] = {0x3A, 0x30, 0x30, 0x30, 0x30};

                    md5_context = os_mem_alloc2(sizeof(T_MD5));
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
                            obex_connect_with_auth_rsp(p_link, md5_context->digest);
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
                if (p_link->psm)
                {
                    mpa_send_l2c_disconn_req(p_link->cid);
                }
                else
                {
                    rfc_disconn_req(p_link->bd_addr, p_link->dlci);
                }
                p_link->state = OBEX_STATE_IDLE;
            }
        }
        break;

    case OBEX_STATE_SET_PATH:
        {
            T_OBEX_SET_PATH_CMPL_DATA data;

            data.handle = p_link;
            memcpy(data.bd_addr, p_link->bd_addr, 6);
            data.cause = rsp_code;
            if (p_link->cb)
            {
                p_link->cb(OBEX_CB_MSG_SET_PATH_DONE, (void *)&data);
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
                    p_link->state = OBEX_STATE_IDLE;
                }
                else if (rsp_code == OBEX_RSP_CONTINUE)
                {
                    uint8_t ret_flag = 0;

                    if (obex_find_hdr_in_pkt(p_pkt + OBEX_BASE_LEN, pkt_len - OBEX_BASE_LEN,
                                             OBEX_HI_SRM, &p_hdr_data, &hdr_data_len))
                    {
                        if (*p_hdr_data == 0x01)
                        {
                            ret_flag |= FLAG_SRM_ENABLE;
                        }
                    }
                    if (obex_find_hdr_in_pkt(p_pkt + OBEX_BASE_LEN, pkt_len - OBEX_BASE_LEN,
                                             OBEX_HI_SRMP, &p_hdr_data, &hdr_data_len))
                    {
                        if (*p_hdr_data == 0x01)
                        {
                            ret_flag |= FLAG_SRMP_WAIT;
                        }
                    }

                    if (p_link->srm_status == SRM_DISABLE)
                    {
                        if (ret_flag & FLAG_SRM_ENABLE)
                        {
                            if (ret_flag & FLAG_SRMP_WAIT)
                            {
                                p_link->srm_status = SRM_WAIT;
                            }
                            else
                            {
                                p_link->srm_status = SRM_ENABLE;
                            }
                        }
                    }
                    else if (p_link->srm_status == SRM_WAIT)
                    {
                        if ((ret_flag & FLAG_SRMP_WAIT) == 0)
                        {
                            p_link->srm_status = SRM_ENABLE;
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

            data.handle = p_link;
            memcpy(data.bd_addr, p_link->bd_addr, 6);
            data.cause = rsp_code;
            data.srm_status = p_link->srm_status;

            if (p_link->cb)
            {
                p_link->cb(OBEX_CB_MSG_PUT_DONE, (void *)&data);
            }
        }
        break;

    case OBEX_STATE_GET:
        {
            if (rsp_code == OBEX_RSP_SUCCESS || rsp_code == OBEX_RSP_CONTINUE)
            {
                if (rsp_code == OBEX_RSP_SUCCESS)
                {
                    p_link->state = OBEX_STATE_IDLE;
                }
                else if (rsp_code == OBEX_RSP_CONTINUE)
                {
                    uint8_t ret_flag = 0;

                    if (obex_find_hdr_in_pkt(p_pkt + OBEX_BASE_LEN, pkt_len - OBEX_BASE_LEN,
                                             OBEX_HI_SRM, &p_hdr_data, &hdr_data_len))
                    {
                        if (*p_hdr_data == 0x01)
                        {
                            ret_flag |= FLAG_SRM_ENABLE;
                        }
                    }
                    if (obex_find_hdr_in_pkt(p_pkt + OBEX_BASE_LEN, pkt_len - OBEX_BASE_LEN,
                                             OBEX_HI_SRMP, &p_hdr_data, &hdr_data_len))
                    {
                        if (*p_hdr_data == 0x01)
                        {
                            ret_flag |= FLAG_SRMP_WAIT;
                        }
                    }

                    if (p_link->srm_status == SRM_DISABLE)
                    {
                        if (ret_flag & FLAG_SRM_ENABLE)
                        {
                            if (ret_flag & FLAG_SRMP_WAIT)
                            {
                                p_link->srm_status = SRM_WAIT;
                            }
                            else
                            {
                                p_link->srm_status = SRM_ENABLE;
                            }
                        }
                    }
                    else if (p_link->srm_status == SRM_WAIT)
                    {
                        if ((ret_flag & FLAG_SRMP_WAIT) == 0)
                        {
                            p_link->srm_status = SRM_ENABLE;
                        }
                    }
                }

                if (p_link->cb)
                {
                    T_OBEX_GET_OBJECT_CMPL_DATA data;

                    data.handle = p_link;
                    memcpy(data.bd_addr, p_link->bd_addr, 6);
                    data.data_len = pkt_len - OBEX_BASE_LEN;
                    data.p_data = p_pkt + OBEX_BASE_LEN;
                    data.rsp_code = rsp_code;
                    data.srm_status = p_link->srm_status;

                    p_link->cb(OBEX_CB_MSG_GET_OBJECT, &data);
                }
            }
            else
            {
                p_link->state = OBEX_STATE_IDLE;

                if (p_link->cb)
                {
                    T_OBEX_GET_OBJECT_CMPL_DATA data;

                    data.handle = p_link;
                    memcpy(data.bd_addr, p_link->bd_addr, 6);
                    data.data_len = 0;
                    data.p_data = NULL;
                    data.rsp_code = rsp_code;

                    p_link->cb(OBEX_CB_MSG_GET_OBJECT, &data);
                }
            }
        }
        break;

    case OBEX_STATE_DISCONNECT:
        if (p_link->psm)
        {
            mpa_send_l2c_disconn_req(p_link->cid);
        }
        else
        {
            rfc_disconn_req(p_link->bd_addr, p_link->dlci);
        }
        break;

    case OBEX_STATE_ABORT:
        {
            T_OBEX_ABORT_CMPL_DATA data;

            p_link->state = OBEX_STATE_IDLE;

            data.handle = p_link;
            memcpy(data.bd_addr, p_link->bd_addr, 6);
            data.cause = rsp_code;
            if (p_link->cb)
            {
                p_link->cb(OBEX_CB_MSG_ABORT_DONE, (void *)&data);
            }
        }
        break;

    default:
        break;
    }
}

void obex_handle_data_ind(T_OBEX_LINK *p_link,
                          uint8_t     *p_pkt,
                          uint16_t     pkt_len)
{
    if (p_link->p_rx_data != NULL)
    {
        if (pkt_len <= p_link->total_len - p_link->rx_len)
        {
            memcpy(p_link->p_rx_data + p_link->rx_len, p_pkt, pkt_len);
            p_link->rx_len += pkt_len;

            if (p_link->rx_len == p_link->total_len)
            {
                if (p_link->role == OBEX_ROLE_SERVER)
                {
                    obex_handle_req_pkt(p_link, p_link->p_rx_data, p_link->total_len);
                }
                else
                {
                    obex_handle_rsp_pkt(p_link, p_link->p_rx_data, p_link->total_len);
                }

                os_mem_free(p_link->p_rx_data);
                p_link->p_rx_data = NULL;
                p_link->rx_len = 0;
                p_link->total_len = 0;
            }
        }
        else
        {
            PROTOCOL_PRINT_ERROR3("obex_handle_rcv_data: error pkt len %d, received data len %d, total data len %d",
                                  pkt_len, p_link->rx_len, p_link->total_len);
            os_mem_free(p_link->p_rx_data);
            p_link->p_rx_data = NULL;
            p_link->rx_len = 0;
            p_link->total_len = 0;
        }
    }
    else
    {
        p_link->total_len = (p_pkt[1] << 8) | (p_pkt[2]);

        if (p_link->total_len > pkt_len)
        {
            // this should not hanppen since we limmitted max pkt len
            PROTOCOL_PRINT_ERROR3("obex_handle_rcv_data: max pkt len %d, received pkt len %d, total data len %d",
                                  p_link->local_max_pkt_len, pkt_len, p_link->total_len);

            p_link->p_rx_data = os_mem_alloc2(p_link->total_len);
            if (p_link->p_rx_data == NULL)
            {
                p_link->total_len = 0;
                return;
            }

            memcpy(p_link->p_rx_data, p_pkt, pkt_len);
            p_link->rx_len = pkt_len;
        }
        else
        {
            if (p_link->role == OBEX_ROLE_SERVER)
            {
                obex_handle_req_pkt(p_link, p_pkt, p_link->total_len);
            }
            else
            {
                obex_handle_rsp_pkt(p_link, p_pkt, p_link->total_len);
            }
        }
    }
}

void obex_rfcomm_callback(T_RFC_MSG_TYPE  msg_type,
                          void           *p_msg)
{
    T_OBEX_LINK *p_link;
    PROTOCOL_PRINT_TRACE1("obex_rfcomm_callback: msg_type = %d", msg_type);

    switch (msg_type)
    {
    case RFC_CONN_IND:
        {
            T_RFC_CONN_IND *p_ind = (T_RFC_CONN_IND *)p_msg;
            uint8_t idx;
            T_OBEX_RFC_PROFILE *p_profile_cb;

            p_link = obex_find_link_by_dlci(p_ind->bd_addr, p_ind->dlci);
            if (p_link)
            {
                rfc_conn_cfm(p_ind->bd_addr, p_ind->dlci, RFC_REJECT, p_ind->frame_size, RFC_DEFAULT_CREDIT);
                break;
            }

            p_link = obex_alloc_link(p_ind->bd_addr);
            if (p_link)
            {
                p_link->psm = 0;
                p_link->dlci = p_ind->dlci;
                p_link->role = OBEX_ROLE_SERVER;

                for (idx = 0; idx < OBEX_PROFILE_NUM; idx++)
                {
                    p_profile_cb = &p_obex->rfc_prof_cb[idx];
                    if (p_profile_cb->server_chann == ((p_link->dlci >> 1) & 0x1F))
                    {
                        p_link->cb = p_profile_cb->cb;
                    }
                }

                if (p_ind->frame_size < OBEX_OVER_RFC_MAX_PKT_LEN)
                {
                    rfc_conn_cfm(p_ind->bd_addr, p_ind->dlci, RFC_ACCEPT, p_ind->frame_size, RFC_DEFAULT_CREDIT);
                }
                else
                {
                    rfc_conn_cfm(p_ind->bd_addr, p_ind->dlci, RFC_ACCEPT, OBEX_OVER_RFC_MAX_PKT_LEN,
                                 RFC_DEFAULT_CREDIT);
                }
            }
            else
            {
                rfc_conn_cfm(p_ind->bd_addr, p_ind->dlci, RFC_REJECT, p_ind->frame_size, RFC_DEFAULT_CREDIT);
            }
        }
        break;

    case RFC_CONN_CMPL:
        {
            T_RFC_CONN_CMPL *p_cmpl = (T_RFC_CONN_CMPL *)p_msg;

            p_link = obex_find_link_by_dlci(p_cmpl->bd_addr, p_cmpl->dlci);

            if (p_link)
            {
                p_link->state = OBEX_STATE_RFC_CONNECTED;
                p_link->local_max_pkt_len = p_cmpl->frame_size;

                if (p_link->role == OBEX_ROLE_CLIENT)
                {
                    if (p_cmpl->remain_credits)
                    {
                        obex_connect(p_link);
                    }
                    else
                    {
                        p_link->wait_credit_flag = 1;
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

            p_link = obex_find_link_by_dlci(p_disc->bd_addr, p_disc->dlci);
            if (p_link)
            {
                T_OBEX_DISCONN_CMPL_DATA data;

                data.handle = p_link;
                data.cause = p_disc->cause;
                memcpy(data.bd_addr, p_link->bd_addr, 6);

                if (p_link->cb)
                {
                    p_link->cb(OBEX_CB_MSG_DISCONN, &data);
                }

                obex_free_link(p_link);
            }
        }
        break;

    case RFC_CREDIT_INFO:
        {
            T_RFC_CREDIT_INFO *p_info = (T_RFC_CREDIT_INFO *)p_msg;

            p_link = obex_find_link_by_dlci(p_info->bd_addr, p_info->dlci);
            if (p_link)
            {
                if ((p_info->remain_credits) && (p_link->wait_credit_flag))
                {
                    obex_connect(p_link);
                    p_link->wait_credit_flag = 0;
                }
            }
        }
        break;

    case RFC_DATA_IND:
        {
            T_RFC_DATA_IND *p_ind = (T_RFC_DATA_IND *)p_msg;

            p_link = obex_find_link_by_dlci(p_ind->bd_addr, p_ind->dlci);
            if (p_link)
            {
                if ((p_ind->remain_credits) && (p_link->wait_credit_flag))
                {
                    obex_connect(p_link);
                    p_link->wait_credit_flag = 0;
                }

                if (p_ind->length)
                {
                    obex_handle_data_ind(p_link, p_ind->buf, p_ind->length);
                }
            }

            rfc_data_cfm(p_ind->bd_addr, p_ind->dlci, 1);
        }
        break;

    case RFC_DLCI_CHANGE:
        {
            T_RFC_DLCI_CHANGE_INFO *p_info = (T_RFC_DLCI_CHANGE_INFO *)p_msg;

            p_link = obex_find_link_by_dlci(p_info->bd_addr, p_info->pre_dlci);
            if (p_link)
            {
                p_link->dlci = p_info->curr_dlci;
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
    T_OBEX_LINK *p_link;
    PROTOCOL_PRINT_TRACE1("obex_l2cap_callback: msg_type = %d", l2c_msg);

    switch (l2c_msg)
    {
    case L2C_CONN_IND:
        {
            uint8_t index;
            T_MPA_L2C_CONN_IND *p_ind = (T_MPA_L2C_CONN_IND *)p_buf;
            T_OBEX_L2C_PROFILE *p_profile_cb;

            p_link = obex_alloc_link(p_ind->bd_addr);
            if (p_link == NULL)
            {
                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_NO_RESOURCE, p_ind->cid, 672, MPA_L2C_MODE_ERTM, 0xFFFF);
            }
            else
            {
                p_link->cid = p_ind->cid;
                p_link->state = OBEX_STATE_IDLE;
                p_link->role = OBEX_ROLE_SERVER;

                for (index = 0; index < OBEX_PROFILE_NUM; index++)
                {
                    p_profile_cb = &p_obex->l2c_prof_cb[index];

                    if (p_profile_cb->queue_id == p_ind->proto_id)
                    {
                        p_link->cb = p_profile_cb->cb;
                        p_link->psm = p_profile_cb->psm;
                    }
                }

                mpa_send_l2c_conn_cfm(MPA_L2C_CONN_ACCEPT, p_ind->cid, OBEX_OVER_L2C_MAX_PKT_LEN, MPA_L2C_MODE_ERTM,
                                      0xFFFF);
            }
        }
        break;

    case L2C_DATA_RSP:
        {
            T_MPA_L2C_DATA_RSP *p_info = (T_MPA_L2C_DATA_RSP *)p_buf;

            p_link = obex_find_link_by_cid(p_info->cid);
            if (p_link)
            {
                if (p_link->state == OBEX_STATE_PUT)
                {
                    if (p_link->cb)
                    {
                        T_OBEX_PUT_DATA_RSP rsp;

                        memcpy(rsp.bd_addr, p_link->bd_addr, 6);
                        rsp.handle = p_link;
                        p_link->cb(OBEX_CB_MSG_PUT_DATA_RSP, &rsp);
                    }
                }
            }
        }
        break;

    case L2C_CONN_RSP:
        {
            T_MPA_L2C_CONN_RSP *p_rsp = (T_MPA_L2C_CONN_RSP *)p_buf;

            p_link = obex_find_link_by_queue_id(p_rsp->bd_addr, p_rsp->proto_id);
            if (p_link)
            {
                if (p_rsp->cause == 0)
                {
                    p_link->cid = p_rsp->cid;
                }
                else
                {
                    T_OBEX_DISCONN_CMPL_DATA data;

                    data.handle = p_link;
                    data.cause = p_rsp->cause;
                    memcpy(data.bd_addr, p_link->bd_addr, 6);

                    if (p_link->cb)
                    {
                        p_link->cb(OBEX_CB_MSG_DISCONN, &data);
                    }

                    obex_free_link(p_link);
                }
            }
        }
        break;

    case L2C_CONN_CMPL:
        {
            T_MPA_L2C_CONN_CMPL_INFO *p_info = (T_MPA_L2C_CONN_CMPL_INFO *)p_buf;

            p_link = obex_find_link_by_cid(p_info->cid);
            if (p_link)
            {
                if (p_info->cause)
                {
                    T_OBEX_DISCONN_CMPL_DATA data;

                    data.handle = p_link;
                    data.cause = p_info->cause;
                    memcpy(data.bd_addr, p_link->bd_addr, 6);

                    if (p_link->cb)
                    {
                        p_link->cb(OBEX_CB_MSG_DISCONN, &data);
                    }

                    obex_free_link(p_link);
                }
                else
                {
                    p_obex->ds_data_offset = p_info->ds_data_offset;
                    p_link->state = OBEX_STATE_L2C_CONNECTED;
                    p_link->local_max_pkt_len = OBEX_OVER_L2C_MAX_PKT_LEN;

                    if (p_link->role == OBEX_ROLE_CLIENT)
                    {
                        obex_connect(p_link);
                    }
                }
            }
        }
        break;

    case L2C_DATA_IND:
        {
            T_MPA_L2C_DATA_IND *p_ind = (T_MPA_L2C_DATA_IND *)p_buf;

            p_link = obex_find_link_by_cid(p_ind->cid);
            if (p_link && p_ind->length)
            {
                obex_handle_data_ind(p_link, p_ind->data + p_ind->gap, p_ind->length);
            }
        }
        break;

    case L2C_DISCONN_RSP:
        {
            T_MPA_L2C_DISCONN_RSP *p_rsp = (T_MPA_L2C_DISCONN_RSP *)p_buf;

            p_link = obex_find_link_by_cid(p_rsp->cid);
            if (p_link)
            {
                T_OBEX_DISCONN_CMPL_DATA data;

                data.handle = p_link;
                data.cause = p_rsp->cause;
                memcpy(data.bd_addr, p_link->bd_addr, 6);

                if (p_link->cb)
                {
                    p_link->cb(OBEX_CB_MSG_DISCONN, &data);
                }
                obex_free_link(p_link);
            }
        }
        break;

    case L2C_DISCONN_IND:
        {
            T_MPA_L2C_DISCONN_IND *p_ind = (T_MPA_L2C_DISCONN_IND *)p_buf;

            p_link = obex_find_link_by_cid(p_ind->cid);
            if (p_link)
            {
                T_OBEX_DISCONN_CMPL_DATA data;

                data.handle = p_link;
                data.cause = p_ind->cause;
                memcpy(data.bd_addr, p_link->bd_addr, 6);

                if (p_link->cb)
                {
                    p_link->cb(OBEX_CB_MSG_DISCONN, &data);
                }

                obex_free_link(p_link);
            }

            mpa_send_l2c_disconn_cfm(p_ind->cid);
        }
        break;

    default:
        break;
    }
}

bool obex_conn_req_over_l2c(uint8_t            bd_addr[6],
                            uint8_t           *p_param,
                            uint16_t           psm,
                            P_OBEX_PROFILE_CB  cb,
                            T_OBEX_HANDLE     *p_handle)
{
    T_OBEX_LINK *p_link;

    p_link = obex_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (mpa_reg_l2c_proto(psm, obex_l2cap_callback, &p_link->queue_id) == false)
    {
        obex_free_link(p_link);
        PROTOCOL_PRINT_ERROR1("obex_conn_req_over_l2c: reg l2c callback for psm 0x%x fail", psm);
        return false;
    }

    mpa_send_l2c_conn_req(psm, psm, p_link->queue_id, OBEX_OVER_L2C_MAX_PKT_LEN, bd_addr,
                          MPA_L2C_MODE_ERTM, 0xFFFF);

    p_link->con_param_ptr = p_param;
    p_link->state = OBEX_STATE_IDLE;
    p_link->cb = cb;
    p_link->role = OBEX_ROLE_CLIENT;
    p_link->psm = psm;

    *p_handle = p_link;

    return true;
}

bool obex_conn_cfm(T_OBEX_HANDLE  handle,
                   bool           accept,
                   uint8_t       *p_cfm_data,
                   uint16_t       data_len)
{
    T_OBEX_LINK *p_link;
    uint8_t *p_buf;
    uint16_t buf_len;
    uint8_t *p;
    uint32_t conn_id;
    bool ret;

    p_link = (T_OBEX_LINK *)handle;
    if (p_link == NULL)
    {
        return false;
    }

    conn_id = obex_get_free_conn_id();
    if (conn_id == 0)
    {
        return false;
    }

    buf_len = OBEX_CONNECT_LEN;
    if (accept)
    {
        buf_len += OBEX_HDR_CONN_ID_LEN + data_len;
    }

    p_buf = os_mem_zalloc2(buf_len);
    if (p_buf == NULL)
    {
        return false;
    }

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
    BE_UINT16_TO_STREAM(p, p_link->local_max_pkt_len);

    if (accept)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_CONNECTION_ID);
        BE_UINT32_TO_STREAM(p, conn_id);
        ARRAY_TO_STREAM(p, p_cfm_data, data_len);

        p_link->connection_id = conn_id;
    }

    ret = obex_send_data(p_link, p_buf, buf_len, false);
    os_mem_free(p_buf);

    if (accept && ret)
    {
        if (p_link->cb)
        {
            T_OBEX_CONN_CMPL_DATA data;

            data.handle = p_link;
            data.max_pkt_len = p_link->local_max_pkt_len;
            memcpy(data.bd_addr, p_link->bd_addr, 6);

            if (p_link->psm)
            {
                data.obex_over_l2c = true;
            }
            else
            {
                data.obex_over_l2c = false;
            }

            p_link->state = OBEX_STATE_CONNECTED;

            p_link->cb(OBEX_CB_MSG_CONN_CMPL, (void *)&data);
        }
    }

    return ret;
}

bool obex_conn_req_over_rfc(uint8_t            bd_addr[6],
                            uint8_t           *p_param,
                            uint8_t            server_chann,
                            P_OBEX_PROFILE_CB  cb,
                            T_OBEX_HANDLE     *p_handle)
{
    T_OBEX_LINK *p_link;
    uint8_t dlci;

    p_link = obex_find_link_by_server_chann(bd_addr, server_chann, OBEX_ROLE_CLIENT);
    if (p_link)
    {
        return false;
    }

    p_link = obex_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_link->con_param_ptr = p_param;
    p_link->psm = 0;
    p_link->server_chann = server_chann;
    p_link->cb = cb;
    p_link->role = OBEX_ROLE_CLIENT;

    if (rfc_conn_req(bd_addr, server_chann, OBEX_OVER_RFC_MAX_PKT_LEN,
                     7, p_obex->rfc_index, &dlci) == true)
    {
        *p_handle = p_link;
        p_link->dlci = dlci;
        return true;
    }
    else
    {
        obex_free_link(p_link);
        return false;
    }
}

bool obex_disconn_req(T_OBEX_HANDLE handle)
{
    T_OBEX_LINK *p_link;

    p_link = (T_OBEX_LINK *)handle;
    if (p_link != NULL)
    {
        if ((p_link->state != OBEX_STATE_L2C_CONNECTED) &&
            (p_link->state != OBEX_STATE_RFC_CONNECTED) &&
            (p_link->state != OBEX_STATE_OBEX_CONNECTING))
        {
            if (p_link->connection_id != 0)
            {
                uint8_t pkt_ptr[8];     //OBEX_BASE_LEN + OBEX_HDR_CONN_ID_LEN

                pkt_ptr[0] = OBEX_OPCODE_DISCONNECT;
                pkt_ptr[1] = 0x00;
                pkt_ptr[2] = 0x08;
                pkt_ptr[3] = OBEX_HI_CONNECTION_ID;
                pkt_ptr[4] = (uint8_t)(p_link->connection_id >> 24);
                pkt_ptr[5] = (uint8_t)(p_link->connection_id >> 16);
                pkt_ptr[6] = (uint8_t)(p_link->connection_id >> 8);
                pkt_ptr[7] = (uint8_t)(p_link->connection_id);
                obex_send_data(p_link, pkt_ptr, 8, false);
            }
            else
            {
                uint8_t pkt_ptr[3];     //OBEX_BASE_LEN

                pkt_ptr[0] = OBEX_OPCODE_DISCONNECT;
                pkt_ptr[1] = 0x00;
                pkt_ptr[2] = 0x03;
                obex_send_data(p_link, pkt_ptr, 3, false);
            }

            p_link->state = OBEX_STATE_DISCONNECT;
        }
        else if (p_link->psm)
        {
            mpa_send_l2c_disconn_req(p_link->cid);
        }
        else
        {
            rfc_disconn_req(p_link->bd_addr, p_link->dlci);
        }

        return true;
    }

    return false;
}

bool obex_init(void)
{
    int32_t ret = 0;

    if (p_obex == NULL)
    {
        p_obex = os_mem_zalloc2(sizeof(T_OBEX));
        if (p_obex == NULL)
        {
            ret = 1;
            goto fail_alloc_obex;
        }

        os_queue_init(&p_obex->link_queue);

        p_obex->rfc_prof_cb = os_mem_zalloc2(OBEX_PROFILE_NUM * sizeof(T_OBEX_RFC_PROFILE));
        if (p_obex->rfc_prof_cb == NULL)
        {
            ret = 2;
            goto fail_alloc_rfc_cb;
        }

        p_obex->l2c_prof_cb = os_mem_zalloc2(OBEX_PROFILE_NUM * sizeof(T_OBEX_L2C_PROFILE));
        if (p_obex->l2c_prof_cb == NULL)
        {
            ret = 3;
            goto fail_alloc_l2c_cb;
        }

        // register an invalid server channel to get an index which can be used when connect
        if (rfc_reg_cb(0xFF, obex_rfcomm_callback, &p_obex->rfc_index) == false)
        {
            ret = 4;
            goto fail_reg_rfc_cb;
        }
    }

    return true;

fail_reg_rfc_cb:
    os_mem_free(p_obex->l2c_prof_cb);
fail_alloc_l2c_cb:
    os_mem_free(p_obex->rfc_prof_cb);
fail_alloc_rfc_cb:
    os_mem_free(p_obex);
    p_obex = NULL;
fail_alloc_obex:
    PROFILE_PRINT_ERROR1("obex_init: failed %d", -ret);
    return false;
}

bool obex_reg_cb_over_rfc(uint8_t chann_num, P_OBEX_PROFILE_CB callback, uint8_t *p_idx)
{
    uint8_t index = 0;
    T_OBEX_RFC_PROFILE *p_profile_cb;

    if (p_obex == NULL)
    {
        if (!obex_init())
        {
            PROTOCOL_PRINT_ERROR0("obex_reg_cb_over_rfc: init obex fail");
            return false;
        }
    }

    for (index = 0; index < OBEX_PROFILE_NUM; index++)
    {
        p_profile_cb = &p_obex->rfc_prof_cb[index];

        if (p_profile_cb->server_chann == chann_num && p_profile_cb->cb != NULL)
        {
            PROTOCOL_PRINT_WARN1("obex_reg_cb_over_rfc: channel number %d is used", chann_num);
            return false;
        }
    }

    for (index = 0; index < OBEX_PROFILE_NUM; index++)
    {
        p_profile_cb = &p_obex->rfc_prof_cb[index];

        if (p_profile_cb->cb == NULL)
        {
            break;
        }
    }

    if (index == OBEX_PROFILE_NUM)
    {
        return false;
    }

    if (rfc_reg_cb(chann_num, obex_rfcomm_callback, &p_profile_cb->rfc_index) == false)
    {
        PROTOCOL_PRINT_ERROR1("obex_reg_cb_over_rfc: register rfc cb for chann num %d fail", chann_num);
        return false;
    }

    p_profile_cb->server_chann = chann_num;
    p_profile_cb->cb = callback;
    *p_idx = index;

    return true;
}

bool obex_reg_cb_over_l2c(uint16_t           psm,
                          P_OBEX_PROFILE_CB  callback,
                          uint8_t           *p_idx)
{
    uint8_t index = 0;
    T_OBEX_L2C_PROFILE *p_profile_cb;

    if (p_obex == NULL)
    {
        if (!obex_init())
        {
            PROTOCOL_PRINT_ERROR0("obex_reg_cb_over_l2c: init obex fail");
            return false;
        }
    }

    for (index = 0; index < OBEX_PROFILE_NUM; index++)
    {
        p_profile_cb = &p_obex->l2c_prof_cb[index];

        if (p_profile_cb->cb != NULL && p_profile_cb->psm == psm)
        {
            PROTOCOL_PRINT_WARN1("obex_reg_cb_over_l2c: psm 0x%x is used", psm);
            return false;
        }
    }

    for (index = 0; index < OBEX_PROFILE_NUM; index++)
    {
        p_profile_cb = &p_obex->l2c_prof_cb[index];

        if (p_profile_cb->cb == NULL)
        {
            break;
        }
    }

    if (index == OBEX_PROFILE_NUM)
    {
        return false;
    }

    if (mpa_reg_l2c_proto(psm, obex_l2cap_callback, &p_profile_cb->queue_id) == false)
    {
        PROTOCOL_PRINT_ERROR1("obex_reg_cb_over_l2c: reg l2c callback for psm 0x%x fail", psm);
        return false;
    }

    p_profile_cb->psm = psm;
    p_profile_cb->cb  = callback;
    *p_idx = index;

    return true;
}

bool obex_get_roleswap_info(T_OBEX_HANDLE         handle,
                            T_OBEX_ROLESWAP_INFO *p_info)
{
    T_OBEX_LINK *p_link;

    p_link = (T_OBEX_LINK *)handle;
    if (p_link == NULL)
    {
        return false;
    }

    p_info->connection_id = p_link->connection_id;
    p_info->local_max_pkt_len = p_link->local_max_pkt_len;
    p_info->remote_max_pkt_len = p_link->remote_max_pkt_len;
    p_info->role = p_link->role;
    p_info->state = p_link->state;
    p_info->psm = p_link->psm;
    p_info->cid = p_link->cid;

    if (p_info->psm)
    {
        p_info->data_offset = p_obex->ds_data_offset;
        return true;
    }
    else
    {
        p_info->rfc_dlci = p_link->dlci;
        return rfc_get_cid(p_link->bd_addr, p_info->rfc_dlci, &p_info->cid);
    }
}

bool obex_set_roleswap_info(uint8_t               bd_addr[6],
                            P_OBEX_PROFILE_CB     cb,
                            T_OBEX_ROLESWAP_INFO *p_info,
                            T_OBEX_HANDLE        *p_handle)
{
    T_OBEX_LINK *p_link;

    if (p_info->psm)
    {
        p_link = obex_find_link_by_cid(p_info->cid);
    }
    else
    {
        p_link = obex_find_link_by_dlci(bd_addr, p_info->rfc_dlci);
    }

    if (p_link == NULL)
    {
        p_link = obex_alloc_link(bd_addr);
    }

    if (p_link == NULL)
    {
        return false;
    }

    p_link->con_param_ptr = p_info->con_param_ptr;
    p_link->connection_id = p_info->connection_id;
    p_link->cid = p_info->cid;
    p_link->state = p_info->state;
    p_link->cb = cb;
    p_link->role = p_info->role;
    p_link->remote_max_pkt_len = p_info->remote_max_pkt_len;
    p_link->local_max_pkt_len = p_info->local_max_pkt_len;
    p_link->psm = p_info->psm;

    *p_handle = p_link;

    if (p_link->psm)
    {
        p_obex->ds_data_offset = p_info->data_offset;

        if (mpa_reg_l2c_proto(p_link->psm, obex_l2cap_callback, &p_link->queue_id) == false)
        {
            goto FAIL;
        }
    }
    else
    {
        p_link->dlci = p_info->rfc_dlci;
    }

    return true;

FAIL:
    obex_free_link(p_link);
    return false;
}

bool obex_del_roleswap_info(T_OBEX_HANDLE handle)
{
    T_OBEX_LINK *p_link;

    p_link = (T_OBEX_LINK *)handle;
    if (p_link == NULL)
    {
        return false;
    }

    obex_free_link(p_link);

    return true;
}

uint8_t obex_get_rfc_profile_idx(void)
{
    return p_obex->rfc_index;
}
