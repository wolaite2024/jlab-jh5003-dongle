/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _PBAP_H_
#define _PBAP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PBAP_FILTER_ATTR_MASK_VERSION                0x00000001
#define PBAP_FILTER_ATTR_MASK_FN                     0x00000002
#define PBAP_FILTER_ATTR_MASK_N                      0x00000004
#define PBAP_FILTER_ATTR_MASK_PHOTO                  0x00000008
#define PBAP_FILTER_ATTR_MASK_BDAY                   0x00000010
#define PBAP_FILTER_ATTR_MASK_ADR                    0x00000020
#define PBAP_FILTER_ATTR_MASK_LABEL                  0x00000040
#define PBAP_FILTER_ATTR_MASK_TEL                    0x00000080
#define PBAP_FILTER_ATTR_MASK_EMAIL                  0x00000100
#define PBAP_FILTER_ATTR_MASK_MAILER                 0x00000200
#define PBAP_FILTER_ATTR_MASK_TZ                     0x00000400
#define PBAP_FILTER_ATTR_MASK_GEO                    0x00000800
#define PBAP_FILTER_ATTR_MASK_TITLE                  0x00001000
#define PBAP_FILTER_ATTR_MASK_ROLE                   0x00002000
#define PBAP_FILTER_ATTR_MASK_LOGO                   0x00004000
#define PBAP_FILTER_ATTR_MASK_AGENT                  0x00008000
#define PBAP_FILTER_ATTR_MASK_ORG                    0x00010000
#define PBAP_FILTER_ATTR_MASK_NOTE                   0x00020000
#define PBAP_FILTER_ATTR_MASK_REV                    0x00040000
#define PBAP_FILTER_ATTR_MASK_SOUND                  0x00080000
#define PBAP_FILTER_ATTR_MASK_URL                    0x00100000
#define PBAP_FILTER_ATTR_MASK_UID                    0x00200000
#define PBAP_FILTER_ATTR_MASK_KEY                    0x00400000
#define PBAP_FILTER_ATTR_MASK_NICKNAME               0x00800000
#define PBAP_FILTER_ATTR_MASK_CATEGORIES             0x01000000
#define PBAP_FILTER_ATTR_MASK_PROID                  0x02000000
#define PBAP_FILTER_ATTR_MASK_CLASS                  0x04000000
#define PBAP_FILTER_ATTR_MASK_SORT                   0x08000000
#define PBAP_FILTER_ATTR_MASK_TIME_STAMP             0x10000000
#define PBAP_FILTER_ATTR_MASK_RSV29                  0x20000000
#define PBAP_FILTER_ATTR_MASK_RSV30                  0x40000000
#define PBAP_FILTER_ATTR_MASK_RSV31                  0x80000000
#define PBAP_FILTER_ATTR_MASK_RSV32                 (uint64_t)0x100000000
#define PBAP_FILTER_ATTR_MASK_RSV33                 (uint64_t)0x200000000
#define PBAP_FILTER_ATTR_MASK_RSV34                 (uint64_t)0x400000000
#define PBAP_FILTER_ATTR_MASK_RSV35                 (uint64_t)0x800000000
#define PBAP_FILTER_ATTR_MASK_RSV36                (uint64_t)0x1000000000
#define PBAP_FILTER_ATTR_MASK_RSV37                (uint64_t)0x2000000000
#define PBAP_FILTER_ATTR_MASK_RSV38                (uint64_t)0x4000000000
#define PBAP_FILTER_ATTR_MASK_Proprietary_Filter   (uint64_t)0x8000000000//BIT39

typedef enum
{
    PBAP_REPOSITORY_LOCAL   = (1 << 0),
    PBAP_REPOSITORY_SIM1    = (1 << 1),
} T_PBAP_REPOSITORY;

typedef enum
{
    PBAP_PATH_ROOT         = 0x00,
    PBAP_PATH_TELECOM      = 0x01,
    PBAP_PATH_PB           = 0x02,
    PBAP_PATH_ICH          = 0x03,
    PBAP_PATH_OCH          = 0x04,
    PBAP_PATH_MCH          = 0x05,
    PBAP_PATH_CCH          = 0x06,
    PBAP_PATH_SIM1         = 0x07,
} T_PBAP_PATH;

typedef enum
{
    PBAP_PHONE_BOOK_PB        = 0x00,
    PBAP_PHONE_BOOK_ICH       = 0x01,
    PBAP_PHONE_BOOK_OCH       = 0x02,
    PBAP_PHONE_BOOK_MCH       = 0x03,
    PBAP_PHONE_BOOK_CCH       = 0x04,
    PBAP_PHONE_BOOK_SPD       = 0x05,
    PBAP_PHONE_BOOK_FAV       = 0x06,
} T_PBAP_PHONE_BOOK;

typedef enum
{
    PBAP_TAG_ID_ORDER_VALUE_INDEXED      = 0x00,
    PBAP_TAG_ID_ORDER_VALUE_ALPHANUMERIC = 0x01,
    PBAP_TAG_ID_ORDER_VALUE_PHONETIC     = 0x02,
} T_PBAP_TAG_ID_ORDER_VALUE;

typedef enum
{
    PBAP_TAG_ID_SEARCH_PROP_VALUE_NAME   = 0x00,
    PBAP_TAG_ID_SEARCH_PROP_VALUE_NUMBER = 0x01,
    PBAP_TAG_ID_SEARCH_PROP_VALUE_SOUND  = 0x02,
} T_PBAP_TAG_ID_SEARCH_PROP_VALUE;

typedef enum
{
    PBAP_TAG_ID_FORMAT_VALUE_21 = 0x00,
    PBAP_TAG_ID_FORMAT_VALUE_30 = 0x01,
} T_PBAP_TAG_ID_FORMAT_VALUE;

typedef enum
{
    PBAP_TAG_ID_VCARD_SEL_OP_VALUE_OR    = 0x00,
    PBAP_TAG_ID_VCARD_SEL_OP_VALUE_AND   = 0x01,
} T_PBAP_TAG_ID_VCARD_SEL_OP_VALUE;

#define PBAP_TAG_ID_RESET_NEW_MISSED_CALLS_VALUE_RESET 0x01

typedef struct
{
    uint16_t    cause;
} T_PBAP_DISCONN_INFO;

typedef struct
{
    bool        result;
    T_PBAP_PATH path;
} T_PBAP_SET_PHONE_BOOK_CMPL;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    uint16_t    pb_size;
    uint8_t     new_missed_calls;
    bool        data_end;
} T_PBAP_GET_PHONE_BOOK_MSG_DATA;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    uint16_t    pb_size;
    uint8_t     new_missed_calls;
    bool        data_end;
} T_PBAP_GET_VCARD_LISTING_MSG_DATA;

typedef struct
{
    uint8_t    *p_data;
    uint16_t    data_len;
    bool        data_end;
} T_PBAP_GET_VCARD_ENTRY_MSG_DATA;

typedef enum
{
    PBAP_MSG_CONNECTED                = 0x00,
    PBAP_MSG_CONNECTION_FAIL          = 0x01,
    PBAP_MSG_DISCONNECTED             = 0x02,
    PBAP_MSG_SET_PATH_DONE            = 0x03,
    PBAP_MSG_GET_VCARD_LISTING_CMPL   = 0x04,
    PBAP_MSG_GET_VCARD_ENTRY_CMPL     = 0x05,
    PBAP_MSG_CONN_ACPT_APP            = 0x06,
    PBAP_MSG_GET_PHONE_BOOK_CMPL      = 0x07,
    PBAP_MSG_GET_PHONE_BOOK_SIZE_CMPL = 0x08,
} T_PBAP_MSG;

typedef struct
{
    uint8_t             bd_addr[6];
    uint16_t            l2c_cid;
    uint32_t            obex_conn_id;
    uint16_t            obex_psm;
    uint16_t            local_max_pkt_len;
    uint16_t            remote_max_pkt_len;
    uint8_t             obex_state;
    uint8_t             rfc_dlci;

    uint8_t             pbap_state;
    bool                feat_flag;

    uint8_t             data_offset;

    uint8_t             path;
    T_PBAP_REPOSITORY   repos;
} T_ROLESWAP_PBAP_INFO;

typedef struct
{
    uint8_t             bd_addr[6];
    uint8_t             obex_state;
    uint8_t             pbap_state;
    uint8_t             path;
    T_PBAP_REPOSITORY   repos;
} T_ROLESWAP_PBAP_TRANSACT;

typedef struct
{
    uint64_t                   filter;
    T_PBAP_TAG_ID_FORMAT_VALUE format;
    uint16_t                   max_list_count;
    uint16_t                   start_offset;
} T_PBAP_PULL_PHONE_BOOK_PARAM;

typedef struct
{
    T_PBAP_REPOSITORY            repo;
    T_PBAP_PHONE_BOOK            phone_book;
    T_PBAP_PULL_PHONE_BOOK_PARAM param;
} T_PBAP_REQ_PULL_PHONE_BOOK;

typedef struct
{
    uint8_t                        *p_folder;
    uint16_t                        folder_len;
    T_PBAP_TAG_ID_ORDER_VALUE       order;
    uint8_t                        *p_search_value;
    uint8_t                         value_len;
    T_PBAP_TAG_ID_SEARCH_PROP_VALUE search_attr;
    uint16_t                        max_list_count;
    uint16_t                        start_offset;
} T_PBAP_REQ_PULL_VCARD_LISTING;

typedef struct
{
    uint8_t                   *p_name;
    uint8_t                    name_len;
    uint64_t                   filter;
    T_PBAP_TAG_ID_FORMAT_VALUE format;
} T_PBAP_REQ_PULL_VCARD_ENTRY;

typedef void(*P_PBAP_CBACK)(uint8_t     bd_addr[6],
                            T_PBAP_MSG  msg_type,
                            void       *msg_buf);

bool pbap_init(uint8_t      link_num,
               P_PBAP_CBACK cback,
               uint32_t     support_feat);

bool pbap_init(uint8_t      link_num,
               P_PBAP_CBACK cback,
               uint32_t     support_feat);

bool pbap_conn_over_rfc(uint8_t bd_addr[6],
                        uint8_t server_chann,
                        bool    feat_flag);

bool pbap_conn_over_l2c(uint8_t  bd_addr[6],
                        uint16_t l2c_psm,
                        bool     feat_flag);

bool pbap_disconnect_req(uint8_t p_bd_addr[6]);

bool pbap_pull_phone_book(uint8_t                       bd_addr[6],
                          T_PBAP_REPOSITORY             repo,
                          T_PBAP_PHONE_BOOK             phone_book,
                          T_PBAP_PULL_PHONE_BOOK_PARAM *p_param);

bool pbap_pull_continue(uint8_t bd_addr[6]);

bool pbap_pull_abort(uint8_t bd_addr[6]);

bool pbap_set_phone_book(uint8_t     bd_addr[6],
                         T_PBAP_PATH path);

bool pbap_pull_vcard_listing(uint8_t                          bd_addr[6],
                             uint8_t                         *folder,
                             uint16_t                         folder_len,
                             T_PBAP_TAG_ID_ORDER_VALUE        order,
                             uint8_t                         *search_value,
                             uint8_t                          value_len,
                             T_PBAP_TAG_ID_SEARCH_PROP_VALUE  search_attr,
                             uint16_t                         max_list_count,
                             uint16_t                         start_offset);

bool pbap_pull_vcard_listing_by_number(uint8_t  bd_addr[6],
                                       char    *phone_number);

bool pbap_pull_vcard_entry(uint8_t                     bd_addr[6],
                           uint8_t                    *p_name,
                           uint8_t                     name_len,
                           uint64_t                    filter,
                           T_PBAP_TAG_ID_FORMAT_VALUE  format);

bool pbap_get_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_PBAP_INFO *p_info);

bool pbap_set_roleswap_info(uint8_t               bd_addr[6],
                            T_ROLESWAP_PBAP_INFO *p_info);

bool pbap_del_roleswap_info(uint8_t bd_addr[6]);

uint8_t pbap_get_rfc_profile_idx(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PBAP_H_ */
