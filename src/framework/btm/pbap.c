/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_PBAP_SUPPORT == 1)

#include <string.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "pbap.h"
#include "obex.h"

//Application parameters(TLV format) Tag for PBAP
#define PBAP_TAG_ID_ORDER                        0x01
#define PBAP_TAG_ID_SEARCH_VALUE                 0x02
#define PBAP_TAG_ID_SEARCH_PROP                  0x03
#define PBAP_TAG_ID_MAX_LIST_COUNT               0x04
#define PBAP_TAG_ID_START_OFFSET                 0x05
#define PBAP_TAG_ID_FILTER                       0x06
#define PBAP_TAG_ID_FORMAT                       0x07
#define PBAP_TAG_ID_PB_SIZE                      0x08
#define PBAP_TAG_ID_NEW_MISS_CALL                0x09
#define PBAP_TAG_ID_PRIMARY_VERSION_COUNTER      0x0A
#define PBAP_TAG_ID_SECONDARY_VERSION_COUNTER    0x0B
#define PBAP_TAG_ID_VCARD_SELECTOR               0x0C
#define PBAP_TAG_ID_DATABASE_IDENTIFIER          0x0D
#define PBAP_TAG_ID_VCARD_SELECTOR_OPERATOR      0x0E
#define PBAP_TAG_ID_RESET_NEW_MISSED_CALLS       0x0F
#define PBAP_TAG_ID_PBAP_FEATURE                 0x10

#define PBAP_TAG_ID_ORDER_LEN                        1
#define PBAP_TAG_ID_SEARCH_PROP_LEN                  1
#define PBAP_TAG_ID_MAX_LIST_COUNT_LEN               2
#define PBAP_TAG_ID_START_OFFSET_LEN                 2
#define PBAP_TAG_ID_FILTER_LEN                       8
#define PBAP_TAG_ID_FORMAT_LEN                       1
#define PBAP_TAG_ID_PB_SIZE_LEN                      2
#define PBAP_TAG_ID_NEW_MISS_CALL_LEN                1
#define PBAP_TAG_ID_PRIMARY_VERSION_COUNTER_LEN      16
#define PBAP_TAG_ID_SECONDARY_VERSION_COUNTER_LEN    16
#define PBAP_TAG_ID_VCARD_SELECTOR_LEN               8
#define PBAP_TAG_ID_DATABASE_IDENTIFIER_LEN          16
#define PBAP_TAG_ID_VCARD_SELECTOR_OPERATOR_LEN      1
#define PBAP_TAG_ID_RESET_NEW_MISSED_CALLS_LEN       1
#define PBAP_TAG_ID_PBAP_FEATURE_LEN                 4

#define PBAP_TAG_ID_PBAP_FEAT_DOWNLOAD                           0x00000001
#define PBAP_TAG_ID_PBAP_FEAT_BROWSING                           0x00000002
#define PBAP_TAG_ID_PBAP_FEAT_DB_ID                              0x00000004
#define PBAP_TAG_ID_PBAP_FEAT_FOLDER_VER_COUNTERS                0x00000008
#define PBAP_TAG_ID_PBAP_FEAT_VCARD_SELECTING                    0x00000010
#define PBAP_TAG_ID_PBAP_FEAT_ENHANCED_MISSED_CALLS              0x00000020
#define PBAP_TAG_ID_PBAP_FEAT_X_BT_UCI_VCARD_PROPERTY            0x00000040
#define PBAP_TAG_ID_PBAP_FEAT_X_BT_UID_VCARD_PROPERTY            0x00000080
#define PBAP_TAG_ID_PBAP_FEAT_CONTACT_REFERENCING                0x00000100
#define PBAP_TAG_ID_PBAP_FEAT_DEFAULT_CONTACT_IMAGE_FORMAT       0x00000200

#define PBAP_TARGET_LEN                 0x13//include TLV
#define PBAP_PHONE_BOOK_TYPE_LEN        0x12//include TLV
#define PBAP_VCARD_LISTING_TYPE_LEN     0x16//include TLV
#define PBAP_VCARD_TYPE_LEN             0x0E//include TLV

/********seach attribute*********/
#define PBAP_TAG_SEARCH_ATTR_LEN         0x03
#define PBAP_TAG_LIST_SATRT_OFFSET_LEN   0x04
#define PBAP_TAG_MAX_LIST_COUNT_LEN      0x04

#define PBAP_PATH_SIM1_LEN           0x0A
#define PBAP_PATH_TELECOM_LEN        0x10
#define PBAP_PATH_PB_LEN             0x06
#define PBAP_PATH_CALL_HISTORY_LEN   0x08

#define PBAP_NAME_TELECOM_LEN         0x10
#define PBAP_NAME_SIM1_TELECOM_LEN    0x1A
#define PBAP_NAME_PHONE_BOOK_LEN      0x0E
#define PBAP_NAME_CALL_HISTORY_LEN    0x10

#define PBAP_MAX_QUEUED_CMD 10

typedef enum
{
    PBAP_STATE_DISCONNECTED        = 0x00,
    PBAP_STATE_ALLOCATED           = 0x01,
    PBAP_STATE_CONNECTING          = 0x02,
    PBAP_STATE_CONNECTED           = 0x03,
    PBAP_STATE_ABORTING            = 0x04,
    PBAP_STATE_GET_VCARD_LISTING   = 0x05,
    PBAP_STATE_GET_VCARD_ENTRY     = 0x06,
    PBAP_STATE_GET_PHONE_BOOK      = 0x07,
    PBAP_STATE_GET_PHONE_BOOK_SIZE = 0x08,
} T_PBAP_STATE;

typedef enum
{
    PBAP_OP_SET_PHONE_BOOK     = 0x00,
    PBAP_OP_PULL_PHONE_BOOK    = 0x01,
    PBAP_OP_PULL_VCARD_LISTING = 0x02,
    PBAP_OP_PULL_VCARD_ENTRY   = 0x03,
    PBAP_OP_PULL_CONTINUE      = 0x04,
    PBAP_OP_PULL_ABORT         = 0x05,
} T_PBAP_OPCODE;

typedef struct
{
    struct T_PBAP_CMD          *p_next;
    uint8_t                     opcode;
    union
    {
        uint8_t                       pbap_path;
        T_PBAP_REQ_PULL_PHONE_BOOK    pull_phone_book;
        T_PBAP_REQ_PULL_VCARD_LISTING pull_vcard_listing;
        T_PBAP_REQ_PULL_VCARD_ENTRY   pull_vcard_entry;
    } cmd_param;
} T_PBAP_CMD;

typedef struct
{
    uint8_t       bd_addr[6];
    uint8_t       state;
    uint8_t       cmd_credits;
    T_OS_QUEUE    cmd_queue;
    T_OBEX_HANDLE handle;
    bool          feat_flag;
    bool          obex_over_l2c;
    T_PBAP_PATH   last_path;
    uint8_t       srm_status;
} T_PBAP_LINK;

typedef struct
{
    P_PBAP_CBACK   cback;
    T_PBAP_LINK   *p_link;

    uint8_t con_tout;
    uint8_t obex_over_rfc_idx;
    uint8_t bex_over_l2c_idx;
    uint8_t link_num;
} T_PBAP;

typedef struct
{
    uint8_t remote_bd[6];
    uint8_t accept_flag;
} T_PBAP_ACPT_DATA;

T_PBAP *p_pbap;

const uint8_t pbap_target[PBAP_TARGET_LEN + 1] =
{
    PBAP_TARGET_LEN,//indicate total length
    OBEX_HI_TARGET,
    (uint8_t)(PBAP_TARGET_LEN >> 8),
    (uint8_t)PBAP_TARGET_LEN,
    //target uuid
    0x79, 0x61, 0x35, 0xf0, 0xf0, 0xc5, 0x11, 0xd8, 0x09, 0x66, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};

const uint8_t pbap_target_n_feature[PBAP_TARGET_LEN + 9 + 1] =
{
    (PBAP_TARGET_LEN + 9), //indicate total length
    OBEX_HI_TARGET,
    (uint8_t)(PBAP_TARGET_LEN >> 8),
    (uint8_t)(PBAP_TARGET_LEN),
    //target uuid
    0x79, 0x61, 0x35, 0xf0, 0xf0, 0xc5, 0x11, 0xd8, 0x09, 0x66, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66,
    //application parameter with Tag
    OBEX_HI_APP_PARAMETER,
    //length : 0x09
    0x00,
    0x09,
    PBAP_TAG_ID_PBAP_FEATURE,
    0x04,
    0x00,
    0x00,
    0x00,
    0x83//DOWNLOAD,BROWSING, X-BT-UID VCARD
};

//NULL terminated ASCII : x-bt/phonebook
const uint8_t phone_book_type[PBAP_PHONE_BOOK_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(PBAP_PHONE_BOOK_TYPE_LEN >> 8),
    (uint8_t)PBAP_PHONE_BOOK_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x70, 0x68, 0x6f, 0x6e, 0x65, 0x62, 0x6f, 0x6f, 0x6b, 0x00
};

//NULL terminated ASCII : x-bt/vcard
const uint8_t vcard_type[PBAP_VCARD_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(PBAP_VCARD_TYPE_LEN >> 8),
    (uint8_t)PBAP_VCARD_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x76, 0x63, 0x61, 0x72, 0x64, 0x00
};

//NULL terminated ASCII : x-bt/vcard-listing
const uint8_t vcard_listing_type[PBAP_VCARD_LISTING_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(PBAP_VCARD_LISTING_TYPE_LEN >> 8),
    (uint8_t)PBAP_VCARD_LISTING_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x76, 0x63, 0x61, 0x72, 0x64, 0x2d, 0x6c, 0x69, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x00
};

//NULL terminated UNICODE : telecom
const uint8_t path_telecom[PBAP_PATH_TELECOM_LEN] =
{
    0x00, 0x74, 0x00, 0x65, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x63, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x00
};

//NULL terminated UNICODE : SIM1
const uint8_t path_sim1[PBAP_PATH_SIM1_LEN] =
{
    0x00, 0x53, 0x00, 0x49, 0x00, 0x4d, 0x00, 0x31, 0x00, 0x00
};

//NULL terminated UNICODE : pb
const uint8_t path_pb[PBAP_PATH_PB_LEN] =
{
    0x00, 0x70, 0x00, 0x62, 0x00, 0x00
};

//NULL terminated UNICODE : ich
const uint8_t path_ich[PBAP_PATH_CALL_HISTORY_LEN] =
{
    0x00, 0x69, 0x00, 0x63, 0x00, 0x68, 0x00, 0x00
};

//NULL terminated UNICODE : och
const uint8_t path_och[PBAP_PATH_CALL_HISTORY_LEN] =
{
    0x00, 0x6f, 0x00, 0x63, 0x00, 0x68, 0x00, 0x00
};

//NULL terminated UNICODE : mch
const uint8_t path_mch[PBAP_PATH_CALL_HISTORY_LEN] =
{
    0x00, 0x6d, 0x00, 0x63, 0x00, 0x68, 0x00, 0x00
};

//NULL terminated UNICODE : cch
const uint8_t path_cch[PBAP_PATH_CALL_HISTORY_LEN] =
{
    0x00, 0x63, 0x00, 0x63, 0x00, 0x68, 0x00, 0x00
};

//UNICODE : telecom/
const uint8_t name_telecom[PBAP_NAME_TELECOM_LEN] =
{
    0x00, 0x74, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x2F
};

//UNICODE : SIM1/telecom/
const uint8_t name_sim1_telecom[PBAP_NAME_SIM1_TELECOM_LEN] =
{
    0x00, 0x53, 0x00, 0x49, 0x00, 0x4D, 0x00, 0x31, 0x00, 0x2F,
    0x00, 0x74, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x2F
};

//NULL terminated UNICODE : pb.vcf
const uint8_t name_pb[PBAP_NAME_PHONE_BOOK_LEN] =
{
    0x00, 0x70, 0x00, 0x62, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

//NULL terminated UNICODE : ich.vcf
const uint8_t name_ich[PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x69, 0x00, 0x63, 0x00, 0x68, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

//NULL terminated UNICODE : och.vcf
const uint8_t name_och[PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x6F, 0x00, 0x63, 0x00, 0x68, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

//NULL terminated UNICODE : mch.vcf
const uint8_t name_mch[PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x6D, 0x00, 0x63, 0x00, 0x68, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

//NULL terminated UNICODE : cch.vcf
const uint8_t name_cch[PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x63, 0x00, 0x63, 0x00, 0x68, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

//NULL terminated UNICODE : spd.vcf
const uint8_t name_spd[PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x73, 0x00, 0x70, 0x00, 0x64, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

//NULL terminated UNICODE : fav.vcf
const uint8_t name_fav[PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x66, 0x00, 0x61, 0x00, 0x76, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

#define BE_UINT64_TO_STREAM(s, u64) {                   \
        *s++ = (uint8_t)((u64) >> 56);                  \
        *s++ = (uint8_t)((u64) >> 48);                  \
        *s++ = (uint8_t)((u64) >> 40);                  \
        *s++ = (uint8_t)((u64) >> 32);                  \
        *s++ = (uint8_t)((u64) >> 24);                  \
        *s++ = (uint8_t)((u64) >> 16);                  \
        *s++ = (uint8_t)((u64) >>  8);                  \
        *s++ = (uint8_t)((u64) >>  0);                  \
    }

T_PBAP_LINK *pbap_alloc_link(uint8_t bd_addr[6])
{
    uint8_t i;
    T_PBAP_LINK *p_link;

    for (i = 0; i < p_pbap->link_num; i++)
    {
        p_link = &p_pbap->p_link[i];
        if (p_link->state == PBAP_STATE_DISCONNECTED)
        {
            p_link->handle = NULL;
            memcpy(p_link->bd_addr, bd_addr, 6);
            p_link->state = PBAP_STATE_ALLOCATED;

            return p_link;
        }
    }

    return NULL;
}

void pbap_free_link(T_PBAP_LINK *p_link)
{
    if (p_link != NULL)
    {
        memset(p_link, 0, sizeof(T_PBAP_LINK));
    }
}

T_PBAP_LINK *pbap_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t i;
    T_PBAP_LINK *p_link;

    for (i = 0; i < p_pbap->link_num; i++)
    {
        p_link = &p_pbap->p_link[i];

        if ((p_link->state != PBAP_STATE_DISCONNECTED) && (memcmp(p_link->bd_addr, bd_addr, 6) == 0))
        {
            return p_link;
        }
    }

    return NULL;
}

bool pbap_send_cmd_set_phone_book(T_PBAP_LINK *p_link,
                                  T_PBAP_PATH  path)
{
    const uint8_t *path_ptr;
    uint16_t path_len;

    switch (path)
    {
    case PBAP_PATH_ROOT:
        path_len = 0;
        path_ptr = NULL;
        break;

    case PBAP_PATH_TELECOM:
        path_len = PBAP_PATH_TELECOM_LEN;
        path_ptr = path_telecom;
        break;

    case PBAP_PATH_SIM1:
        path_len = PBAP_PATH_SIM1_LEN;
        path_ptr = path_sim1;
        break;

    case PBAP_PATH_PB:
        path_len = PBAP_PATH_PB_LEN;
        path_ptr = path_pb;
        break;

    case PBAP_PATH_ICH:
        path_len = PBAP_PATH_CALL_HISTORY_LEN;
        path_ptr = path_ich;
        break;

    case PBAP_PATH_OCH:
        path_len = PBAP_PATH_CALL_HISTORY_LEN;
        path_ptr = path_och;
        break;

    case PBAP_PATH_MCH:
        path_len = PBAP_PATH_CALL_HISTORY_LEN;
        path_ptr = path_mch;
        break;

    case PBAP_PATH_CCH:
        path_len = PBAP_PATH_CALL_HISTORY_LEN;
        path_ptr = path_cch;
        break;

    default:
        path_ptr = NULL;
        path_len = 0;
        break;
    }
    return obex_set_path(p_link->handle, 0x02, path_ptr, path_len);
}

bool pbap_send_cmd_pull_phone_book(T_PBAP_LINK                  *p_link,
                                   T_PBAP_REPOSITORY             repo,
                                   T_PBAP_PHONE_BOOK             phone_book,
                                   T_PBAP_PULL_PHONE_BOOK_PARAM *p_param)
{
    uint8_t *p_data;
    uint8_t *p;
    uint8_t name_len;
    bool ret = true;

    PROFILE_PRINT_INFO3("pbap_pull_phone_book: bd_addr %s, repo %d, phone_book %d",
                        TRACE_BDADDR(p_link->bd_addr), repo, phone_book);

    if (repo == PBAP_REPOSITORY_LOCAL)
    {
        name_len = PBAP_NAME_TELECOM_LEN;
    }
    else
    {
        name_len = PBAP_NAME_SIM1_TELECOM_LEN;
    }

    if (phone_book == PBAP_PHONE_BOOK_PB)
    {
        name_len += PBAP_NAME_PHONE_BOOK_LEN;
    }
    else
    {
        name_len += PBAP_NAME_CALL_HISTORY_LEN;
    }

    uint16_t pkt_len = (OBEX_HDR_NAME_LEN + name_len) + PBAP_PHONE_BOOK_TYPE_LEN;
    if (p_param != NULL)
    {
        pkt_len = pkt_len + (3 + 10 + 3 + 4 + 4);
    }
    if (p_link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    p_data = os_mem_alloc2(pkt_len);
    if (p_data == NULL)
    {
        return false;
    }
    p = p_data;

    if (p_link->obex_over_l2c)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
        BE_UINT8_TO_STREAM(p, 0x01); //ENABLE
    }

    BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
    BE_UINT16_TO_STREAM(p, OBEX_HDR_NAME_LEN + name_len);
    if (repo == PBAP_REPOSITORY_LOCAL)
    {
        ARRAY_TO_STREAM(p, name_telecom, PBAP_NAME_TELECOM_LEN);
    }
    else
    {
        ARRAY_TO_STREAM(p, name_sim1_telecom, PBAP_NAME_SIM1_TELECOM_LEN);
    }
    switch (phone_book)
    {
    case PBAP_PHONE_BOOK_PB:
        ARRAY_TO_STREAM(p, name_pb, PBAP_NAME_PHONE_BOOK_LEN);
        break;

    case PBAP_PHONE_BOOK_ICH:
        ARRAY_TO_STREAM(p, name_ich, PBAP_NAME_CALL_HISTORY_LEN);
        break;

    case PBAP_PHONE_BOOK_OCH:
        ARRAY_TO_STREAM(p, name_och, PBAP_NAME_CALL_HISTORY_LEN);
        break;

    case PBAP_PHONE_BOOK_MCH:
        ARRAY_TO_STREAM(p, name_mch, PBAP_NAME_CALL_HISTORY_LEN);
        break;

    case PBAP_PHONE_BOOK_CCH:
        ARRAY_TO_STREAM(p, name_cch, PBAP_NAME_CALL_HISTORY_LEN);
        break;

    case PBAP_PHONE_BOOK_SPD:
        ARRAY_TO_STREAM(p, name_spd, PBAP_NAME_CALL_HISTORY_LEN);
        break;

    case PBAP_PHONE_BOOK_FAV:
        ARRAY_TO_STREAM(p, name_fav, PBAP_NAME_CALL_HISTORY_LEN);
        break;

    default:
        PROFILE_PRINT_ERROR0("pbap_pull_phone_book: unknown phone_book");
        os_mem_free(p_data);
        return false;
    }

    ARRAY_TO_STREAM(p, phone_book_type, PBAP_PHONE_BOOK_TYPE_LEN);

    if (p_param != NULL)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
        BE_UINT16_TO_STREAM(p, 3 + 10 + 3 + 4 + 4);

        BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_FILTER);
        BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_FILTER_LEN);
        BE_UINT64_TO_STREAM(p, p_param->filter);

        BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_FORMAT);
        BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_FORMAT_LEN);
        BE_UINT8_TO_STREAM(p, p_param->format);

        BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_MAX_LIST_COUNT);
        BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_MAX_LIST_COUNT_LEN);
        BE_UINT16_TO_STREAM(p, p_param->max_list_count);

        BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_START_OFFSET);
        BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_START_OFFSET_LEN);
        BE_UINT16_TO_STREAM(p, p_param->start_offset);
    }

    if (p_param != NULL && p_param->max_list_count == 0)
    {
        p_link->state = PBAP_STATE_GET_PHONE_BOOK_SIZE;
    }
    else
    {
        p_link->state = PBAP_STATE_GET_PHONE_BOOK;
    }
    ret = obex_get_object(p_link->handle, p_data, p - p_data);
    os_mem_free(p_data);

    return ret;
}

bool pbap_send_cmd_pull_vcard_listing(T_PBAP_LINK                   *p_link,
                                      T_PBAP_REQ_PULL_VCARD_LISTING *param)
{
    uint8_t *p_data;
    uint8_t *p;
    bool ret = true;

    uint16_t pkt_len = OBEX_HDR_NAME_LEN + param->folder_len + PBAP_VCARD_LISTING_TYPE_LEN +
                       (3 + 3 + 2 + param->value_len + 3 + 4 + 4);
    if (p_link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    p_data = os_mem_alloc2(pkt_len);
    if (p_data == NULL)
    {
        return false;
    }

    p = p_data;

    if (p_link->obex_over_l2c)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
        BE_UINT8_TO_STREAM(p, 0x01); //ENABLE
    }

    BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
    BE_UINT16_TO_STREAM(p, OBEX_HDR_NAME_LEN + param->folder_len);
    ARRAY_TO_STREAM(p, param->p_folder, param->folder_len);

    ARRAY_TO_STREAM(p, vcard_listing_type, PBAP_VCARD_LISTING_TYPE_LEN);

    BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
    BE_UINT16_TO_STREAM(p, 3 + 3 + 2 + param->value_len + 3 + 4 + 4);

    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_ORDER);
    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_ORDER_LEN);
    BE_UINT8_TO_STREAM(p, param->order);

    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_SEARCH_VALUE);
    BE_UINT8_TO_STREAM(p, param->value_len);
    ARRAY_TO_STREAM(p, param->p_search_value, param->value_len);

    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_SEARCH_PROP);
    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_SEARCH_PROP_LEN);
    BE_UINT8_TO_STREAM(p, param->search_attr);

    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_MAX_LIST_COUNT);
    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_MAX_LIST_COUNT_LEN);
    BE_UINT16_TO_STREAM(p, param->max_list_count);

    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_START_OFFSET);
    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_START_OFFSET_LEN);
    BE_UINT16_TO_STREAM(p, param->start_offset);

    p_link->state = PBAP_STATE_GET_VCARD_LISTING;

    ret = obex_get_object(p_link->handle, p_data, p - p_data);
    os_mem_free(p_data);

    return ret;
}

bool pbap_send_cmd_pull_vcard_entry(T_PBAP_LINK                 *p_link,
                                    T_PBAP_REQ_PULL_VCARD_ENTRY *p_param)
{
    uint8_t *p_data;
    uint8_t *p;
    bool ret = true;

    PROFILE_PRINT_INFO4("pbap_send_cmd_pull_vcard_entry: bd_addr %s, p_name %s, filter 0x%016lx, format %d",
                        TRACE_BDADDR(p_link->bd_addr), TRACE_STRING(p_param->p_name), p_param->filter,
                        p_param->format);

    uint16_t pkt_len = (OBEX_HDR_NAME_LEN + p_param->name_len) + PBAP_VCARD_TYPE_LEN + 0x0010;
    if (p_link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    p_data = os_mem_alloc2(pkt_len);
    if (p_data == NULL)
    {
        return false;
    }
    p = p_data;

    if (p_link->obex_over_l2c)
    {
        BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
        BE_UINT8_TO_STREAM(p, 0x01); //ENABLE
    }

    BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
    BE_UINT16_TO_STREAM(p, OBEX_HDR_NAME_LEN + p_param->name_len);
    ARRAY_TO_STREAM(p, p_param->p_name, p_param->name_len);

    ARRAY_TO_STREAM(p, vcard_type, PBAP_VCARD_TYPE_LEN);

    BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
    BE_UINT16_TO_STREAM(p, 0x0010);

    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_FILTER);
    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_FILTER_LEN);
    BE_UINT64_TO_STREAM(p, p_param->filter);

    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_FORMAT);
    BE_UINT8_TO_STREAM(p, PBAP_TAG_ID_FORMAT_LEN);
    BE_UINT8_TO_STREAM(p, p_param->format);

    p_link->state = PBAP_STATE_GET_VCARD_ENTRY;
    ret = obex_get_object(p_link->handle, p_data, p - p_data);
    os_mem_free(p_data);

    return ret;
}

void pbap_cmd_process(T_PBAP_LINK *p_link)
{
    T_PBAP_CMD *p_cmd;
    bool status = false;
    int32_t ret = 0;

    if (p_link->handle == NULL)
    {
        ret = 1;
        goto fail_check_handle;
    }

    p_cmd = os_queue_peek(&p_link->cmd_queue, 0);
    if (p_cmd == NULL)
    {
        ret = 2;
        goto fail_queue_empty;
    }

    PROFILE_PRINT_INFO1("pbap_cmd_process: opcode 0x%02x", p_cmd->opcode);

    if (p_link->cmd_credits == 0)
    {
        ret = 3;
        goto fail_no_credits;
    }

    switch (p_cmd->opcode)
    {
    case PBAP_OP_SET_PHONE_BOOK:
        if (p_link->state == PBAP_STATE_CONNECTED)
        {
            status = pbap_send_cmd_set_phone_book(p_link,
                                                  (T_PBAP_PATH)p_cmd->cmd_param.pbap_path);
            if (status == true)
            {
                p_link->last_path = (T_PBAP_PATH)p_cmd->cmd_param.pbap_path;
            }
        }
        else
        {
            ret = 4;
            goto fail_invalid_state;
        }
        break;

    case PBAP_OP_PULL_PHONE_BOOK:
        if (p_link->state == PBAP_STATE_CONNECTED)
        {
            status = pbap_send_cmd_pull_phone_book(p_link,
                                                   p_cmd->cmd_param.pull_phone_book.repo,
                                                   p_cmd->cmd_param.pull_phone_book.phone_book,
                                                   &p_cmd->cmd_param.pull_phone_book.param);
        }
        else
        {
            ret = 4;
            goto fail_invalid_state;
        }
        break;

    case PBAP_OP_PULL_VCARD_LISTING:
        if (p_link->state == PBAP_STATE_CONNECTED)
        {
            status = pbap_send_cmd_pull_vcard_listing(p_link, &p_cmd->cmd_param.pull_vcard_listing);
            if (p_cmd->cmd_param.pull_vcard_listing.p_folder)
            {
                os_mem_free(p_cmd->cmd_param.pull_vcard_listing.p_folder);
                p_cmd->cmd_param.pull_vcard_listing.p_folder = NULL;
            }
            if (p_cmd->cmd_param.pull_vcard_listing.p_search_value)
            {
                os_mem_free(p_cmd->cmd_param.pull_vcard_listing.p_search_value);
                p_cmd->cmd_param.pull_vcard_listing.p_search_value = NULL;
            }
        }
        else
        {
            ret = 4;
            goto fail_invalid_state;
        }
        break;

    case PBAP_OP_PULL_VCARD_ENTRY:
        if (p_link->state == PBAP_STATE_CONNECTED)
        {
            status = pbap_send_cmd_pull_vcard_entry(p_link, &p_cmd->cmd_param.pull_vcard_entry);
            if (p_cmd->cmd_param.pull_vcard_entry.p_name != NULL)
            {
                os_mem_free(p_cmd->cmd_param.pull_vcard_entry.p_name);
                p_cmd->cmd_param.pull_vcard_entry.p_name = NULL;
            }
        }
        else
        {
            ret = 4;
            goto fail_invalid_state;
        }
        break;

    case PBAP_OP_PULL_CONTINUE:
        {
            status = obex_get_object_continue(p_link->handle);
        }
        break;

    case PBAP_OP_PULL_ABORT:
        {
            status = obex_abort(p_link->handle);
            if (status == true)
            {
                p_link->state = PBAP_STATE_ABORTING;
            }
        }
        break;

    default:
        break;
    }

    if (status == false)
    {
        ret = 5;
        goto fail_send_cmd;
    }

    p_link->cmd_credits = 0;
    os_queue_delete(&p_link->cmd_queue, p_cmd);
    os_mem_free(p_cmd);

    return;

fail_send_cmd:
fail_invalid_state:
    os_queue_delete(&p_link->cmd_queue, p_cmd);
    os_mem_free(p_cmd);
fail_queue_empty:
fail_no_credits:
fail_check_handle:
    PROFILE_PRINT_ERROR1("pbap_cmd_process: failed %d", -ret);
}

void pbap_obex_cb(uint8_t  msg_type,
                  void    *p_msg)
{
    T_PBAP_LINK *p_link;

    p_link = NULL;
    PROFILE_PRINT_TRACE1("pbap_obex_cb: msg_type=0x%x", msg_type);

    switch (msg_type)
    {
    case OBEX_CB_MSG_CONN_IND:
        break;

    case OBEX_CB_MSG_CONN_CMPL:
        {
            T_OBEX_CONN_CMPL_DATA *p_data = (T_OBEX_CONN_CMPL_DATA *)p_msg;

            p_link = pbap_find_link_by_addr(p_data->bd_addr);
            if (p_link != NULL && (p_link->handle == p_data->handle))
            {
                p_link->state = PBAP_STATE_CONNECTED;
                os_queue_init(&p_link->cmd_queue);
                p_link->cmd_credits = 1;

                if (p_pbap->cback)
                {
                    p_pbap->cback(p_link->bd_addr, PBAP_MSG_CONNECTED, NULL);
                }
            }
            else
            {
                obex_disconn_req(p_data->handle);
            }
        }
        break;

    case OBEX_CB_MSG_DISCONN:
        {
            T_OBEX_DISCONN_CMPL_DATA *p_data = (T_OBEX_DISCONN_CMPL_DATA *)p_msg;

            p_link = pbap_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                if (p_data->handle == p_link->handle)
                {
                    T_PBAP_CMD *p_cmd;
                    T_PBAP_CMD *p_next;

                    p_cmd = (T_PBAP_CMD *)p_link->cmd_queue.p_first;
                    while (p_cmd)
                    {
                        p_next = (T_PBAP_CMD *)p_cmd->p_next;

                        os_queue_delete(&p_link->cmd_queue, p_cmd);
                        if (p_cmd->opcode == PBAP_OP_PULL_VCARD_ENTRY)
                        {
                            if (p_cmd->cmd_param.pull_vcard_entry.p_name != NULL)
                            {
                                os_mem_free(p_cmd->cmd_param.pull_vcard_entry.p_name);
                            }
                        }
                        if (p_cmd->opcode == PBAP_OP_PULL_VCARD_LISTING)
                        {
                            if (p_cmd->cmd_param.pull_vcard_listing.p_folder != NULL)
                            {
                                os_mem_free(p_cmd->cmd_param.pull_vcard_listing.p_folder);
                            }
                            if (p_cmd->cmd_param.pull_vcard_listing.p_search_value != NULL)
                            {
                                os_mem_free(p_cmd->cmd_param.pull_vcard_listing.p_search_value);
                            }
                        }
                        os_mem_free(p_cmd);

                        p_cmd = p_next;
                    }

                    p_link->handle = NULL;
                    if (p_pbap->cback)
                    {
                        if (p_link->state == PBAP_STATE_CONNECTING)
                        {
                            p_pbap->cback(p_link->bd_addr, PBAP_MSG_CONNECTION_FAIL, &p_data->cause);
                        }
                        else
                        {
                            T_PBAP_DISCONN_INFO info;
                            info.cause = p_data->cause;
                            p_pbap->cback(p_link->bd_addr, PBAP_MSG_DISCONNECTED, &info);
                        }
                    }
                    pbap_free_link(p_link);
                }
            }
        }
        break;

    case OBEX_CB_MSG_SET_PATH_DONE:
        {
            T_OBEX_SET_PATH_CMPL_DATA *p_data = (T_OBEX_SET_PATH_CMPL_DATA *)p_msg;

            p_link = pbap_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                if (p_link->handle == p_data->handle)
                {
                    T_PBAP_SET_PHONE_BOOK_CMPL msg_buf;

                    if (p_data->cause == OBEX_RSP_SUCCESS)
                    {
                        msg_buf.result = true;
                    }
                    else
                    {
                        msg_buf.result = false;
                    }

                    msg_buf.path = p_link->last_path;

                    if (p_pbap->cback)
                    {
                        p_pbap->cback(p_link->bd_addr, PBAP_MSG_SET_PATH_DONE, &msg_buf);
                    }
                }

                p_link->cmd_credits = 1;
                pbap_cmd_process(p_link);
            }
        }
        break;

    case OBEX_CB_MSG_ABORT_DONE:
        {
            T_OBEX_ABORT_CMPL_DATA *p_data = (T_OBEX_ABORT_CMPL_DATA *)p_msg;

            p_link = pbap_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                p_link->state = PBAP_STATE_CONNECTED;
                p_link->cmd_credits = 1;
                p_link->srm_status = SRM_DISABLE;
                pbap_cmd_process(p_link);
            }
        }
        break;

    case OBEX_CB_MSG_GET_OBJECT:
        {
            T_OBEX_GET_OBJECT_CMPL_DATA *p_data = (T_OBEX_GET_OBJECT_CMPL_DATA *)p_msg;
            uint8_t *p_hdr_data;
            uint16_t hdr_data_len;
            bool ret;

            p_link = pbap_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                p_link->cmd_credits = 1;
                if (p_link->handle != p_data->handle || p_pbap->cback == NULL)
                {
                    pbap_cmd_process(p_link);
                    break;
                }

                if (p_data->rsp_code != OBEX_RSP_SUCCESS && p_data->rsp_code != OBEX_RSP_CONTINUE)
                {
                    PROFILE_PRINT_ERROR1("OBEX_CB_MSG_GET_OBJECT: error rsp_code=0x%x", p_data->rsp_code);
                }

                p_link->srm_status = p_data->srm_status;

                if (p_link->state == PBAP_STATE_GET_PHONE_BOOK)
                {
                    T_PBAP_GET_PHONE_BOOK_MSG_DATA data = {0};
                    uint8_t *p_tag_data;
                    uint16_t tag_data_len;

                    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_APP_PARAMETER,
                                               &p_hdr_data, &hdr_data_len);
                    if (ret)
                    {
                        ret = obex_find_value_in_app_param(p_hdr_data, hdr_data_len, PBAP_TAG_ID_PB_SIZE,
                                                           &p_tag_data, &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.pb_size, p_tag_data);
                        }

                        ret = obex_find_value_in_app_param(p_hdr_data, hdr_data_len, PBAP_TAG_ID_NEW_MISS_CALL,
                                                           &p_tag_data, &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.new_missed_calls, p_tag_data);
                        }
                    }

                    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY,
                                               &p_hdr_data, &hdr_data_len);
                    if (ret == false)
                    {
                        ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY,
                                                   &p_hdr_data, &hdr_data_len);
                    }

                    if (ret)
                    {
                        data.p_data = p_hdr_data;
                        data.data_len = hdr_data_len;
                    }

                    if (p_data->rsp_code != OBEX_RSP_CONTINUE)
                    {
                        data.data_end = true;
                        p_link->state = PBAP_STATE_CONNECTED;
                        p_link->srm_status = SRM_DISABLE;
                    }

                    p_pbap->cback(p_link->bd_addr, PBAP_MSG_GET_PHONE_BOOK_CMPL, (void *)&data);
                }
                else if (p_link->state == PBAP_STATE_GET_PHONE_BOOK_SIZE)
                {
                    T_PBAP_GET_PHONE_BOOK_MSG_DATA data = {0};
                    uint8_t *p_tag_data;
                    uint16_t tag_data_len;

                    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_APP_PARAMETER,
                                               &p_hdr_data, &hdr_data_len);
                    if (ret)
                    {
                        ret = obex_find_value_in_app_param(p_hdr_data, hdr_data_len, PBAP_TAG_ID_PB_SIZE,
                                                           &p_tag_data, &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.pb_size, p_tag_data);
                        }

                        ret = obex_find_value_in_app_param(p_hdr_data, hdr_data_len, PBAP_TAG_ID_NEW_MISS_CALL,
                                                           &p_tag_data, &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.new_missed_calls, p_tag_data);
                        }
                    }

                    data.data_end = true;
                    p_link->state = PBAP_STATE_CONNECTED;

                    p_pbap->cback(p_link->bd_addr, PBAP_MSG_GET_PHONE_BOOK_SIZE_CMPL, (void *)&data);
                }
                else if (p_link->state == PBAP_STATE_GET_VCARD_LISTING)
                {
                    T_PBAP_GET_VCARD_LISTING_MSG_DATA data = {0};
                    uint8_t *p_tag_data;
                    uint16_t tag_data_len;

                    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_APP_PARAMETER, &p_hdr_data,
                                               &hdr_data_len);
                    if (ret)
                    {
                        ret = obex_find_value_in_app_param(p_hdr_data, hdr_data_len, PBAP_TAG_ID_PB_SIZE, &p_tag_data,
                                                           &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.pb_size, p_tag_data);
                        }

                        ret = obex_find_value_in_app_param(p_hdr_data, hdr_data_len, PBAP_TAG_ID_NEW_MISS_CALL, &p_tag_data,
                                                           &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.new_missed_calls, p_tag_data);
                        }
                    }

                    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &p_hdr_data,
                                               &hdr_data_len);
                    if (ret == false)
                    {
                        ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &p_hdr_data,
                                                   &hdr_data_len);
                    }

                    if (ret)
                    {
                        data.p_data = p_hdr_data;
                        data.data_len = hdr_data_len;
                    }

                    if (p_data->rsp_code != OBEX_RSP_CONTINUE)
                    {
                        data.data_end = true;
                        p_link->state = PBAP_STATE_CONNECTED;
                    }

                    p_pbap->cback(p_link->bd_addr, PBAP_MSG_GET_VCARD_LISTING_CMPL, (void *)&data);
                }
                else if (p_link->state == PBAP_STATE_GET_VCARD_ENTRY)
                {
                    T_PBAP_GET_VCARD_ENTRY_MSG_DATA data = {0};

                    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY, &p_hdr_data,
                                               &hdr_data_len);
                    if (ret == false)
                    {
                        ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &p_hdr_data,
                                                   &hdr_data_len);
                    }

                    if (ret)
                    {
                        data.p_data = p_hdr_data;
                        data.data_len = hdr_data_len;
                    }

                    if (p_data->rsp_code != OBEX_RSP_CONTINUE)
                    {
                        data.data_end = true;
                        p_link->state = PBAP_STATE_CONNECTED;
                    }

                    p_pbap->cback(p_link->bd_addr, PBAP_MSG_GET_VCARD_ENTRY_CMPL, (void *)&data);
                }

                pbap_cmd_process(p_link);
            }
        }
        break;

    default:
        break;
    }
}

bool pbap_conn_over_rfc(uint8_t bd_addr[6],
                        uint8_t server_chann,
                        bool    feat_flag)
{
    T_PBAP_LINK *p_link;
    uint8_t *p_param;
    PROFILE_PRINT_TRACE2("pbap_conn_over_rfc: server_chann=0x%x, feat_flag %d", server_chann,
                         feat_flag);

    ///TODO: support only one mas per link now
    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link)
    {
        PROFILE_PRINT_TRACE0("pbap_conn_over_rfc: error connection exist");
        return false;
    }

    p_link = pbap_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_link->feat_flag = feat_flag;
    if (feat_flag)
    {
        p_param = (uint8_t *)pbap_target_n_feature;
    }
    else
    {
        p_param = (uint8_t *)pbap_target;
    }

    if (obex_conn_req_over_rfc(bd_addr, p_param, server_chann, pbap_obex_cb, &p_link->handle))
    {
        p_link->state = PBAP_STATE_CONNECTING;
        p_link->obex_over_l2c = false;
        return true;
    }
    else
    {
        pbap_free_link(p_link);
        return false;
    }
}

bool pbap_conn_over_l2c(uint8_t  bd_addr[6],
                        uint16_t l2c_psm,
                        bool     feat_flag)
{
    T_PBAP_LINK *p_link;
    uint8_t *p_param;

    ///TODO: support only one mas per link now
    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link)
    {
        return false;
    }

    p_link = pbap_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_link->feat_flag = feat_flag;
    if (feat_flag)
    {
        p_param = (uint8_t *)pbap_target_n_feature;
    }
    else
    {
        p_param = (uint8_t *)pbap_target;
    }

    p_link->state = PBAP_STATE_CONNECTING;

    if (obex_conn_req_over_l2c(bd_addr, p_param, l2c_psm, pbap_obex_cb, &p_link->handle))
    {
        p_link->obex_over_l2c = true;
        return true;
    }
    else
    {
        pbap_free_link(p_link);
        return false;
    }
}

bool pbap_disconnect_req(uint8_t bd_addr[6])
{
    T_PBAP_LINK *p_link;

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link == NULL || p_link->handle == NULL)
    {
        return false;
    }

    return obex_disconn_req(p_link->handle);
}

bool pbap_cmd_enqueue(T_PBAP_LINK *p_link,
                      uint8_t      opcode,
                      void        *param)
{
    T_PBAP_CMD *p_cmd;
    int32_t ret = 0;

    PROFILE_PRINT_INFO1("pbap_cmd_enqueue: opcode 0x%02x", opcode);

    if (p_link->cmd_queue.count == PBAP_MAX_QUEUED_CMD)
    {
        ret = 1;
        goto fail_cmd_enqueue;
    }

    p_cmd = os_mem_zalloc2(sizeof(T_PBAP_CMD));
    if (p_cmd == NULL)
    {
        ret = 2;
        goto fail_alloc_cmd;
    }

    p_cmd->opcode = opcode;
    switch (opcode)
    {
    case PBAP_OP_SET_PHONE_BOOK:
        p_cmd->cmd_param.pbap_path = *(uint8_t *)param;
        break;

    case PBAP_OP_PULL_PHONE_BOOK:
        memcpy(&p_cmd->cmd_param.pull_phone_book, param, sizeof(T_PBAP_REQ_PULL_PHONE_BOOK));
        break;

    case PBAP_OP_PULL_VCARD_LISTING:
        {
            T_PBAP_REQ_PULL_VCARD_LISTING *tmp = (T_PBAP_REQ_PULL_VCARD_LISTING *)param;

            if (tmp->folder_len > 0)
            {
                p_cmd->cmd_param.pull_vcard_listing.p_folder = os_mem_alloc2(tmp->folder_len);
                if (p_cmd->cmd_param.pull_vcard_listing.p_folder == NULL)
                {
                    ret = 3;
                    goto fail_build_cmd;
                }

                memcpy(p_cmd->cmd_param.pull_vcard_listing.p_folder, tmp->p_folder, tmp->folder_len);
            }

            p_cmd->cmd_param.pull_vcard_listing.p_search_value = os_mem_alloc2(tmp->value_len);
            if (p_cmd->cmd_param.pull_vcard_listing.p_search_value == NULL)
            {
                if (p_cmd->cmd_param.pull_vcard_listing.p_folder != NULL)
                {
                    os_mem_free(p_cmd->cmd_param.pull_vcard_listing.p_folder);
                }
                ret = 4;
                goto fail_build_cmd;
            }

            p_cmd->cmd_param.pull_vcard_listing.folder_len = tmp->folder_len;
            p_cmd->cmd_param.pull_vcard_listing.order = tmp->order;
            memcpy(p_cmd->cmd_param.pull_vcard_listing.p_search_value, tmp->p_search_value, tmp->value_len);
            p_cmd->cmd_param.pull_vcard_listing.value_len = tmp->value_len;
            p_cmd->cmd_param.pull_vcard_listing.search_attr = tmp->search_attr;
            p_cmd->cmd_param.pull_vcard_listing.max_list_count = tmp->max_list_count;
            p_cmd->cmd_param.pull_vcard_listing.start_offset = tmp->start_offset;
        }
        break;

    case PBAP_OP_PULL_VCARD_ENTRY:
        {
            T_PBAP_REQ_PULL_VCARD_ENTRY *tmp = (T_PBAP_REQ_PULL_VCARD_ENTRY *)param;

            p_cmd->cmd_param.pull_vcard_entry.p_name = os_mem_alloc2(tmp->name_len);
            if (p_cmd->cmd_param.pull_vcard_entry.p_name == NULL)
            {
                ret = 5;
                goto fail_build_cmd;
            }

            memcpy(p_cmd->cmd_param.pull_vcard_entry.p_name, tmp->p_name, tmp->name_len);
            p_cmd->cmd_param.pull_vcard_entry.name_len = tmp->name_len;
            p_cmd->cmd_param.pull_vcard_entry.filter = tmp->filter;
            p_cmd->cmd_param.pull_vcard_entry.format = tmp->format;
        }
        break;

    case PBAP_OP_PULL_CONTINUE:
        break;

    case PBAP_OP_PULL_ABORT:
        break;

    default:
        ret = 6;
        goto fail_build_cmd;
    }

    os_queue_in(&p_link->cmd_queue, p_cmd);
    pbap_cmd_process(p_link);

    return true;

fail_build_cmd:
    os_mem_free(p_cmd);
fail_alloc_cmd:
fail_cmd_enqueue:
    PROFILE_PRINT_ERROR1("pbap_cmd_enqueue: failed %d", -ret);
    return false;
}

bool pbap_pull_phone_book(uint8_t                       bd_addr[6],
                          T_PBAP_REPOSITORY             repo,
                          T_PBAP_PHONE_BOOK             phone_book,
                          T_PBAP_PULL_PHONE_BOOK_PARAM *p_param)
{
    T_PBAP_LINK *p_link;

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        T_PBAP_REQ_PULL_PHONE_BOOK tmp;

        tmp.repo = repo;
        tmp.phone_book = phone_book;
        tmp.param = *p_param;

        return pbap_cmd_enqueue(p_link, PBAP_OP_PULL_PHONE_BOOK, &tmp);
    }

    return false;
}

bool pbap_pull_continue(uint8_t bd_addr[6])
{
    T_PBAP_LINK *p_link;

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        if (p_link->srm_status == SRM_ENABLE)
        {
            return true;
        }

        return pbap_cmd_enqueue(p_link, PBAP_OP_PULL_CONTINUE, NULL);
    }

    return false;
}

bool pbap_pull_abort(uint8_t bd_addr[6])
{
    T_PBAP_LINK *p_link;

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        return pbap_cmd_enqueue(p_link, PBAP_OP_PULL_ABORT, NULL);
    }

    return false;
}

bool pbap_set_phone_book(uint8_t     bd_addr[6],
                         T_PBAP_PATH path)
{
    T_PBAP_LINK *p_link;

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        uint8_t pbap_path = path;

        return pbap_cmd_enqueue(p_link, PBAP_OP_SET_PHONE_BOOK, &pbap_path);
    }

    return false;
}

bool pbap_pull_vcard_listing(uint8_t                          bd_addr[6],
                             uint8_t                         *folder,
                             uint16_t                         folder_len,
                             T_PBAP_TAG_ID_ORDER_VALUE        order,
                             uint8_t                         *search_value,
                             uint8_t                          value_len,
                             T_PBAP_TAG_ID_SEARCH_PROP_VALUE  search_attr,
                             uint16_t                         max_list_count,
                             uint16_t                         start_offset)
{
    T_PBAP_LINK *p_link;
    bool ret = false;

    PROFILE_PRINT_INFO5("pbap_pull_vcard_listing: bd_addr %s, search_value %b, search_attr %d, max_list_count %d, start_offset %d",
                        TRACE_BDADDR(bd_addr), TRACE_BINARY(value_len, search_value), search_attr, max_list_count,
                        start_offset);

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        T_PBAP_REQ_PULL_VCARD_LISTING tmp;

        tmp.p_folder = folder;
        tmp.folder_len = folder_len;
        tmp.order = order;
        tmp.p_search_value = search_value;
        tmp.value_len = value_len;
        tmp.search_attr = search_attr;
        tmp.max_list_count = max_list_count;
        tmp.start_offset = start_offset;

        ret = pbap_cmd_enqueue(p_link, PBAP_OP_PULL_VCARD_LISTING, &tmp);
    }

    return ret;
}

bool pbap_pull_vcard_listing_by_number(uint8_t  bd_addr[6],
                                       char    *phone_number)
{
    return pbap_pull_vcard_listing(bd_addr, NULL, 0, PBAP_TAG_ID_ORDER_VALUE_INDEXED,
                                   (uint8_t *)phone_number, strlen(phone_number) + 1,
                                   PBAP_TAG_ID_SEARCH_PROP_VALUE_NUMBER, 4, 0);
}

bool pbap_pull_vcard_entry(uint8_t                     bd_addr[6],
                           uint8_t                    *p_name,
                           uint8_t                     name_len,
                           uint64_t                    filter,
                           T_PBAP_TAG_ID_FORMAT_VALUE  format)
{
    T_PBAP_LINK *p_link;
    bool ret = false;

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        T_PBAP_REQ_PULL_VCARD_ENTRY tmp;

        tmp.p_name = p_name;
        tmp.name_len = name_len;
        tmp.filter = filter;
        tmp.format = format;

        ret = pbap_cmd_enqueue(p_link, PBAP_OP_PULL_VCARD_ENTRY, &tmp);
    }

    return ret;
}

bool pbap_init(uint8_t      link_num,
               P_PBAP_CBACK cback,
               uint32_t     support_feat)
{
    int32_t ret = 0;

    p_pbap = os_mem_zalloc2(sizeof(T_PBAP));
    if (p_pbap == NULL)
    {
        ret = 1;
        goto fail_alloc_pbap;
    }

    p_pbap->link_num = link_num;

    // OTP has no map config, use pbap now
    p_pbap->p_link = os_mem_zalloc2(p_pbap->link_num * sizeof(T_PBAP_LINK));
    if (p_pbap->p_link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (support_feat)
    {
        BE_UINT32_TO_ARRAY(&pbap_target_n_feature[PBAP_TARGET_LEN + 5], support_feat);
    }

    p_pbap->cback = cback;
    p_pbap->con_tout = 30; //uint is second

    if (obex_init() == false)
    {
        ret = 3;
        goto fail_alloc_link;
    }

    return true;

fail_alloc_link:
    os_mem_free(p_pbap);
    p_pbap = NULL;
fail_alloc_pbap:
    PROFILE_PRINT_ERROR1("pbap_init: failed %d", -ret);
    return false;
}

bool pbap_get_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_PBAP_INFO *p_info)
{
    T_PBAP_LINK *p_link;
    T_OBEX_ROLESWAP_INFO obex_info;

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    p_info->feat_flag = p_link->feat_flag;
    p_info->pbap_state = p_link->state;

    if (obex_get_roleswap_info(p_link->handle, &obex_info) == false)
    {
        return false;
    }

    p_info->obex_conn_id = obex_info.connection_id;
    p_info->l2c_cid = obex_info.cid;
    p_info->local_max_pkt_len = obex_info.local_max_pkt_len;
    p_info->remote_max_pkt_len = obex_info.remote_max_pkt_len;
    p_info->obex_state = (uint8_t)obex_info.state;
    p_info->obex_psm = obex_info.psm;
    if (p_info->obex_psm)
    {
        p_info->data_offset = obex_info.data_offset;
    }
    else
    {
        p_info->rfc_dlci = obex_info.rfc_dlci;
    }

    return true;
}

bool pbap_set_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_PBAP_INFO *p_info)
{
    T_PBAP_LINK *p_link;
    T_OBEX_ROLESWAP_INFO obex_info;

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = pbap_alloc_link(bd_addr);
    }

    if (p_link == NULL)
    {
        return false;
    }

    p_link->state = (T_PBAP_STATE)p_info->pbap_state;
    p_link->feat_flag = p_info->feat_flag;

    // set client obex info
    obex_info.connection_id = p_info->obex_conn_id;
    obex_info.cid = p_info->l2c_cid;
    obex_info.local_max_pkt_len = p_info->local_max_pkt_len;
    obex_info.remote_max_pkt_len = p_info->remote_max_pkt_len;
    obex_info.role = OBEX_ROLE_CLIENT;
    obex_info.state = (T_OBEX_STATE)p_info->obex_state;
    obex_info.psm = p_info->obex_psm;

    if (obex_info.psm)
    {
        obex_info.data_offset = p_info->data_offset;
    }
    else
    {
        obex_info.rfc_dlci = p_info->rfc_dlci;
    }

    if (p_info->feat_flag)
    {
        obex_info.con_param_ptr = pbap_target_n_feature;
    }
    else
    {
        obex_info.con_param_ptr = (uint8_t *)pbap_target;
    }

    if (obex_set_roleswap_info(bd_addr, pbap_obex_cb, &obex_info, &p_link->handle) == false)
    {
        pbap_free_link(p_link);
        return false;
    }

    return true;

}

bool pbap_del_roleswap_info(uint8_t bd_addr[6])
{
    T_PBAP_LINK *p_link;
    T_OBEX_HANDLE obex_handle;

    p_link = pbap_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    obex_handle = p_link->handle;
    pbap_free_link(p_link);

    return obex_del_roleswap_info(obex_handle);
}

uint8_t pbap_get_rfc_profile_idx(void)
{
    return obex_get_rfc_profile_idx();
}
#else
#include <stdint.h>
#include <stdbool.h>
#include "pbap.h"

bool pbap_init(uint8_t      link_num,
               P_PBAP_CBACK cback,
               uint32_t     support_feat)
{
    return false;
}

bool pbap_conn_over_rfc(uint8_t bd_addr[6],
                        uint8_t server_chann,
                        bool    feat_flag)
{
    return false;
}

bool pbap_conn_over_l2c(uint8_t  bd_addr[6],
                        uint16_t l2c_psm,
                        bool     feat_flag)
{
    return false;
}

bool pbap_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool pbap_pull_phone_book(uint8_t                       bd_addr[6],
                          T_PBAP_REPOSITORY             repo,
                          T_PBAP_PHONE_BOOK             phone_book,
                          T_PBAP_PULL_PHONE_BOOK_PARAM *p_param)
{
    return false;
}

bool pbap_pull_continue(uint8_t bd_addr[6])
{
    return false;
}

bool pbap_pull_abort(uint8_t bd_addr[6])
{
    return false;
}

bool pbap_set_phone_book(uint8_t     bd_addr[6],
                         T_PBAP_PATH path)
{
    return false;
}

bool pbap_pull_vcard_listing(uint8_t                          bd_addr[6],
                             uint8_t                         *folder,
                             uint16_t                         folder_len,
                             T_PBAP_TAG_ID_ORDER_VALUE        order,
                             uint8_t                         *search_value,
                             uint8_t                          value_len,
                             T_PBAP_TAG_ID_SEARCH_PROP_VALUE  search_attr,
                             uint16_t                         max_list_count,
                             uint16_t                         start_offset)
{
    return false;
}

bool pbap_pull_vcard_listing_by_number(uint8_t  bd_addr[6],
                                       char    *phone_number)
{
    return false;
}

bool pbap_pull_vcard_entry(uint8_t                     bd_addr[6],
                           uint8_t                    *p_name,
                           uint8_t                     name_len,
                           uint64_t                    filter,
                           T_PBAP_TAG_ID_FORMAT_VALUE  format)
{
    return false;
}

bool pbap_get_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_PBAP_INFO *p_info)
{
    return false;
}

bool pbap_set_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_PBAP_INFO *p_info)
{
    return false;
}

bool pbap_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}

uint8_t pbap_get_rfc_profile_idx(void)
{
    return 0;
}
#endif
