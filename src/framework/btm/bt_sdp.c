/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "trace.h"
#include "gap.h"
#include "gap_br.h"
#include "bt_mgr.h"
#include "bt_mgr_int.h"
#include "bt_sdp.h"


typedef struct
{
    bool         registered;
    void        *p_buf;
} T_SDP_RECORD;

typedef struct
{
    T_SDP_RECORD   *p_record;
    uint8_t         record_num;
    bool            enable;
    bool            lock;
} T_BT_SDP_DB;

T_BT_SDP_DB bt_sdp_db;

bool bt_sdp_init(void)
{
    bt_sdp_db.record_num = gap_br_get_max_sdp_record_num();
    bt_sdp_db.lock = false;
    bt_sdp_db.enable = false;

    bt_sdp_db.p_record = os_mem_zalloc2(bt_sdp_db.record_num * sizeof(T_SDP_RECORD));
    if (bt_sdp_db.p_record == NULL)
    {
        return false;
    }

    return true;
}

void bt_sdp_enable(bool enable)
{
    bt_sdp_db.enable = enable;
}

bool bt_sdp_discov_start(uint8_t             bd_addr[6],
                         T_BT_SDP_UUID_TYPE  uuid_type,
                         T_BT_SDP_UUID_DATA  uuid_data)
{
    T_GAP_UUID_DATA data;

    memcpy(&data, &uuid_data, sizeof(T_GAP_UUID_DATA));
    if (gap_br_start_sdp_discov(bd_addr, (T_GAP_UUID_TYPE)uuid_type, data) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_sdp_discov_stop(uint8_t bd_addr[6])
{
    if (gap_br_stop_sdp_discov(bd_addr) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bt_did_discov_start(uint8_t bd_addr[6])
{
    if (gap_br_start_did_discov(bd_addr) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void bt_sdp_record_register(void)
{
    if (bt_sdp_db.lock == false)
    {
        uint8_t i;
        for (i = 0; i < bt_sdp_db.record_num; i++)
        {
            if (bt_sdp_db.p_record[i].p_buf != NULL &&
                bt_sdp_db.p_record[i].registered == false)
            {
                if (gap_br_reg_sdp_record(bt_sdp_db.p_record[i].p_buf))
                {
                    bt_sdp_db.lock = true;
                    break;
                }
            }
        }
    }
}

bool bt_sdp_record_add(void *p_buf)
{
    uint8_t i;

    if (p_buf != NULL)
    {
        for (i = 0; i < bt_sdp_db.record_num; i++)
        {
            if (bt_sdp_db.p_record[i].p_buf == NULL)
            {
                bt_sdp_db.p_record[i].p_buf = p_buf;
                bt_sdp_db.p_record[i].registered = false;
                break;
            }
        }
    }

    if (bt_sdp_db.enable)
    {
        bt_sdp_record_register();
    }

    return true;
}

bool bt_sdp_record_delete(void *p_buf)
{
    uint8_t i;

    if (p_buf != NULL)
    {
        for (i = 0; i < bt_sdp_db.record_num; i++)
        {
            if (bt_sdp_db.p_record[i].p_buf == p_buf)
            {
                bt_sdp_db.p_record[i].p_buf = NULL;
                bt_sdp_db.p_record[i].registered = false;
                break;
            }
        }

        if (i == bt_sdp_db.record_num)
        {
            BTM_PRINT_ERROR0("bt_sdp_record_del: sdp record not registered");
            return true;
        }
    }
    else
    {
        return false;
    }


    if (bt_sdp_db.enable)
    {
        if (gap_br_del_sdp_record(p_buf) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
        else
        {
            return false;
        }

    }

    return true;
}

uint8_t *bt_sdp_attr_find(uint8_t  *buf,
                          uint16_t  len,
                          uint32_t  attr_value)
{
    return gap_br_find_sdp_attr(buf, buf + len, attr_value);
}

uint32_t bt_sdp_value_get(uint8_t  *buf,
                          uint16_t  len)
{
    return gap_br_get_sdp_value(buf, buf + len);
}

bool bt_sdp_uuid_value_get(uint8_t            *buf,
                           uint16_t            len,
                           T_BT_SDP_UUID_TYPE *p_type,
                           T_BT_SDP_UUID_DATA *p_data)
{
    return gap_br_get_sdp_uuid_value(buf, buf + len, (T_GAP_UUID_TYPE *)p_type,
                                     (T_GAP_UUID_DATA *)p_data);
}

uint8_t *bt_sdp_elem_access(uint8_t  *buf,
                            uint16_t  len,
                            uint16_t  index)
{
    return gap_br_access_sdp_elem(buf, buf + len, index);
}

void bt_handle_add_sdp_rsp(void *buf)
{
    uint8_t i;
    T_GAP_ADD_SDP_RECORD_RSP *p_rsp = (T_GAP_ADD_SDP_RECORD_RSP *)buf;

    bt_sdp_db.lock = false;

    for (i = 0; i < bt_sdp_db.record_num; i++)
    {
        if (bt_sdp_db.p_record[i].p_buf == p_rsp->p_buf)
        {
            if (p_rsp->cause)
            {
                BTM_PRINT_ERROR2("bt_handle_sdp_reg_rsp: reg fail 0x%04x for addr %p",
                                 p_rsp->cause, p_rsp->p_buf);

                bt_sdp_db.p_record[i].p_buf = NULL;
                bt_sdp_db.p_record[i].registered = false;
            }
            else
            {
                bt_sdp_db.p_record[i].registered = true;
            }
            break;
        }
    }

    for (i = 0; i < bt_sdp_db.record_num; i++)
    {
        if (bt_sdp_db.p_record[i].p_buf != NULL &&
            bt_sdp_db.p_record[i].registered == false)
        {
            if (gap_br_reg_sdp_record(bt_sdp_db.p_record[i].p_buf))
            {
                bt_sdp_db.lock = true;
                break;
            }
        }
    }
}

void bt_handle_del_sdp_rsp(void *buf)
{
    uint8_t i;
    T_GAP_DEL_SDP_RECORD_RSP *p_rsp = (T_GAP_DEL_SDP_RECORD_RSP *)buf;

    if (!p_rsp->cause)
    {
        for (i = 0; i < bt_sdp_db.record_num; i++)
        {
            if (bt_sdp_db.p_record[i].p_buf == p_rsp->p_buf)
            {
                bt_sdp_db.p_record[i].p_buf = NULL;
                bt_sdp_db.p_record[i].registered = false;
            }
        }
    }
    else
    {
        BTM_PRINT_ERROR2("bt_handle_del_sdp_rsp: reg fail 0x%04x for addr %p",
                         p_rsp->cause, p_rsp->p_buf);
    }
}

uint8_t *bt_sdp_elem_decode(uint8_t  *buf,
                            uint16_t  len,
                            uint32_t *p_len,
                            uint8_t  *p_type)
{
    uint8_t size_index;
    uint8_t type;
    uint32_t elem_len = 0;
    bool err = false;
    uint8_t *p_start = buf;

    BE_STREAM_TO_UINT8(size_index, p_start);
    type = (size_index >> 3) & 0x1f;        /*high 5 bit: type*/
    size_index = size_index & 0x7;          /*low 3 bit: size index*/

    /* first check correct typ vs siz settings */
    switch (type)
    {
    case 0:
    case 5:
        if (size_index != 0)    /*this two types, size index need to be zero*/
        {
            err = true;
        }
        break;

    case 1:
    case 2:
        if (size_index > 4)     /*this two types: 1, 2, 3, 4*/
        {
            err = true;
        }
        break;

    case 3:          /*uuid: 1, 2, 4*/
        if (size_index != 1 && size_index != 2 && size_index != 4)
        {
            err = true;
        }
        break;

    case 4:
    case 6:
    case 7:
    case 8:
        if (size_index < 5)
        {
            err = true;
        }
        break;

    default:
        err = true;
        break;
    }

    if (err)
    {
        return NULL;
    }

    /* second extract length info */
    switch (size_index)
    {
    case 0:
        if (type == 0)       /* for null type , elem_len is zero */
        {
            elem_len = 0;
        }
        else                            /* otherwise, lelem_len is 1 */
        {
            elem_len = 1;
        }
        break;

    case 1:
        elem_len = 2;
        break;

    case 2:
        elem_len = 4;
        break;

    case 3:
        elem_len = 8;
        break;

    case 4:
        elem_len = 16;
        break;

    case 5:
        BE_STREAM_TO_UINT8(elem_len, p_start);
        break;

    case 6:
        BE_STREAM_TO_UINT16(elem_len, p_start);
        break;

    case 7:
        BE_STREAM_TO_UINT32(elem_len, p_start);
        break;
    }

    if (elem_len > len)
    {
        return NULL;
    }

    if (p_len)
    {
        *p_len = elem_len;
    }

    if (p_type)
    {
        *p_type = type;
    }

    return p_start;
}
