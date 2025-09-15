/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_PBAP_H_
#define _BT_PBAP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * \defgroup    BT_PBAP BT PBAP Profile
 *
 * \brief   Provide BT PBAP profile interfaces.
 */

/**
 * \defgroup BT_PBAP_PROPERTY_MASK BT PBAP Property Mask
 *
 * \brief  Define BT PBAP property mask.
 *
 * \ingroup BT_PBAP
 */
#define BT_PBAP_PROPERTY_VERSION     (1 << 0)    /**< vCard version. */
#define BT_PBAP_PROPERTY_FN          (1 << 1)    /**< Formatted name. */
#define BT_PBAP_PROPERTY_N           (1 << 2)    /**< Structured presentation of name. */
#define BT_PBAP_PROPERTY_PHOTO       (1 << 3)    /**< Associated image or photo. */
#define BT_PBAP_PROPERTY_BDAY        (1 << 4)    /**< Birthday. */
#define BT_PBAP_PROPERTY_ADR         (1 << 5)    /**< Delivery address. */
#define BT_PBAP_PROPERTY_LABEL       (1 << 6)    /**< Delivery. */
#define BT_PBAP_PROPERTY_TEL         (1 << 7)    /**< Telephone number. */
#define BT_PBAP_PROPERTY_EMAIL       (1 << 8)    /**< Electronic mail address. */
#define BT_PBAP_PROPERTY_MAILER      (1 << 9)    /**< Electronic mail. */
#define BT_PBAP_PROPERTY_TZ          (1 << 10)   /**< Time zone. */
#define BT_PBAP_PROPERTY_GEO         (1 << 11)   /**< Geographic position. */
#define BT_PBAP_PROPERTY_TITLE       (1 << 12)   /**< Job. */
#define BT_PBAP_PROPERTY_ROLE        (1 << 13)   /**< Role within the organization. */
#define BT_PBAP_PROPERTY_LOGO        (1 << 14)   /**< Organization logo. */
#define BT_PBAP_PROPERTY_AGENT       (1 << 15)   /**< vCard of person representing. */
#define BT_PBAP_PROPERTY_ORG         (1 << 16)   /**< Name of organization. */
#define BT_PBAP_PROPERTY_NOTE        (1 << 17)   /**< Comments. */
#define BT_PBAP_PROPERTY_REV         (1 << 18)   /**< Revision. */
#define BT_PBAP_PROPERTY_SOUND       (1 << 19)   /**< Pronunciation of name. */
#define BT_PBAP_PROPERTY_URL         (1 << 20)   /**< Uniform resource locator. */
#define BT_PBAP_PROPERTY_UID         (1 << 21)   /**< Unique ID. */
#define BT_PBAP_PROPERTY_KEY         (1 << 22)   /**< Public encryption key. */
#define BT_PBAP_PROPERTY_NICKNAME    (1 << 23)   /**< Nickname. */
#define BT_PBAP_PROPERTY_CATEGORIES  (1 << 24)   /**< Categories. */
#define BT_PBAP_PROPERTY_PROID       (1 << 25)   /**< Product ID. */
#define BT_PBAP_PROPERTY_CLASS       (1 << 26)   /**< Class information. */
#define BT_PBAP_PROPERTY_SORTSTRING  (1 << 27)   /**< String used for sorting operations. */
#define BT_PBAP_PROPERTY_DATETIME    (1 << 28)   /**< Time stamp. */

/**
 * \brief  Define BT PBAP repository.
 *
 * \ingroup BT_PBAP
 */
typedef enum t_bt_pbap_repository
{
    BT_PBAP_REPOSITORY_LOCAL   = (1 << 0),   /**< Phone's local memory. */
    BT_PBAP_REPOSITORY_SIM1    = (1 << 1),   /**< Phone's SIM card. */
} T_BT_PBAP_REPOSITORY;

/**
 * \brief  Define BT PBAP path.
 *
 * \ingroup BT_PBAP
 */
typedef enum t_bt_pbap_path
{
    BT_PBAP_PATH_ROOT           = 0x00,
    BT_PBAP_PATH_TELECOM        = 0x01,
    BT_PBAP_PATH_PB             = 0x02,
    BT_PBAP_PATH_ICH            = 0x03,
    BT_PBAP_PATH_OCH            = 0x04,
    BT_PBAP_PATH_MCH            = 0x05,
    BT_PBAP_PATH_CCH            = 0x06,
    BT_PBAP_PATH_SIM1           = 0x07,
} T_BT_PBAP_PATH;

/**
 * \brief  Define BT PBAP phone book.
 *
 * \ingroup BT_PBAP
 */
typedef enum t_bt_pbap_phone_book
{
    BT_PBAP_PHONE_BOOK_PB        = 0x00,    /**< Main phone book object */
    BT_PBAP_PHONE_BOOK_ICH       = 0x01,    /**< Incoming calls history object */
    BT_PBAP_PHONE_BOOK_OCH       = 0x02,    /**< Outgoing calls history object */
    BT_PBAP_PHONE_BOOK_MCH       = 0x03,    /**< Missed calls history object */
    BT_PBAP_PHONE_BOOK_CCH       = 0x04,    /**< Combined (incoming, outgoing, and missed) calls history object */
    BT_PBAP_PHONE_BOOK_SPD       = 0x05,    /**< Speed-Dial object */
    BT_PBAP_PHONE_BOOK_FAV       = 0x06,    /**< Favorite contacts object */
} T_BT_PBAP_PHONE_BOOK;

/**
 * \brief   Initialize PBAP profile.
 *
 * \return          The status of initializing PBAP profile.
 * \retval true     PBAP profile was initialized successfully.
 * \retval false    PBAP profile was failed to initialize.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_init(void);

/**
 * \brief  Send a PBAP connection over RFCOMM request.
 *
 * \param[in]  bd_addr         Remote BT address.
 * \param[in]  server_chann    The remote server_channel which can be found from the SDP info.
 * \param[in]  feat_flag       The PSE PBAP SupportedFeatures attribute present flag.
 * \arg    true    The PSE PBAP SupportedFeatures attribute is not present in SDP info.
 * \arg    false   The PSE PBAP SupportedFeatures attribute is present in SDP info.
 *
 * \return         The status of sending the PBAP connection request.
 * \retval true    PBAP connection request was sent successfully.
 * \retval false   PBAP connection request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_connect_over_rfc_req(uint8_t bd_addr[6],
                                  uint8_t server_chann,
                                  bool    feat_flag);

/**
 * \brief  Send a PBAP connection over L2CAP request.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in]  bd_addr         Remote BT address.
 * \param[in]  l2c_psm         The remote L2CAP PSM which can be found from the SDP info.
 * \param[in]  feat_flag       The PSE PBAP SupportedFeatures attribute present flag.
 * \arg    true    The PSE PBAP SupportedFeatures attribute is not present in SDP info.
 * \arg    false   The PSE PBAP SupportedFeatures attribute is present in SDP info.
 *
 * \return         The status of sending the PBAP connection request.
 * \retval true    PBAP connection request was sent successfully.
 * \retval false   PBAP connection request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_connect_over_l2c_req(uint8_t  bd_addr[6],
                                  uint16_t l2c_psm,
                                  bool     feat_flag);

/**
 * \brief  Send a PBAP disconnection request.
 *
 * \param[in]  bd_addr    Remote BT address.
 *
 * \return         The status of sending the PBAP disconnection request.
 * \retval true    PBAP disconnection request was sent successfully.
 * \retval false   PBAP disconnection request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_disconnect_req(uint8_t bd_addr[6]);

/**
 * \brief  Send a request to pull the entire phone book.
 *
 * \param[in]  bd_addr         Remote BT address.
 * \param[in]  repos           Repository containing the phone book \ref T_BT_PBAP_REPOSITORY.
 * \param[in]  phone_book      The phone book object to be retrieved \ref T_BT_PBAP_PHONE_BOOK.
 * \param[in]  start_offset    The offset of the first entry to be retrieved compared to
 *                             the first entry of the phone book object.
 * \param[in]  max_list_count  The maximum number of entries to be retrieved.
 * \param[in]  filter          The filter attribute mask of 64 bits, and 0 for disabling filter \ref BT_PBAP_PROPERTY_MASK.
 *
 * \return         The status of sending pull phone book message.
 * \retval true    Request was sent successfully.
 * \retval false   Request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_phone_book_pull(uint8_t              bd_addr[6],
                             T_BT_PBAP_REPOSITORY repos,
                             T_BT_PBAP_PHONE_BOOK phone_book,
                             uint16_t             start_offset,
                             uint16_t             max_list_count,
                             uint64_t             filter);

/**
 * \brief  Send a request to get the number of indexes in the phone book of interest.
 *
 * \param[in]  bd_addr     Remote BT address.
 * \param[in]  repos       Repository containing the phone book \ref T_BT_PBAP_REPOSITORY.
 * \param[in]  phone_book  The phone book object to be retrieved \ref T_BT_PBAP_PHONE_BOOK.
 *
 * \return         The status of sending get phone book size message.
 * \retval true    Request was sent successfully.
 * \retval false   Request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_phone_book_size_get(uint8_t              bd_addr[6],
                                 T_BT_PBAP_REPOSITORY repos,
                                 T_BT_PBAP_PHONE_BOOK phone_book);

/**
 * \brief  Send a request to pull more data from the remote server.
 *
 * \param[in]  bd_addr     Remote BT address.
 *
 * \return         The status of sending pull continue message.
 * \retval true    Request was sent successfully.
 * \retval false   Request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_pull_continue(uint8_t bd_addr[6]);

/**
 * \brief  Send a request to terminate the ongoing pull operation.
 *
 * \param[in]  bd_addr     Remote BT address.
 *
 * \return         The status of sending pull abort message.
 * \retval true    Request was sent successfully.
 * \retval false   Request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_pull_abort(uint8_t bd_addr[6]);

/**
 * \brief  Send a request to set active phone book.
 *
 * \param[in]  bd_addr    Remote BT address.
 * \param[in]  repos      Repository containing the phone book \ref T_BT_PBAP_REPOSITORY.
 * \param[in]  path       The phone book to be set \ref T_BT_PBAP_PATH.
 *
 * \return         The status of sending set phone book message.
 * \retval true    Request was sent successfully.
 * \retval false   Request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_phone_book_set(uint8_t              bd_addr[6],
                            T_BT_PBAP_REPOSITORY repos,
                            T_BT_PBAP_PATH       path);

/**
 * \brief  Send a request to pull the vCard listing in current phone book by phone number.
 *
 * \param[in]  bd_addr        Remote BT address.
 * \param[in]  phone_number   The start address of string buffer that stores phone number.
 *
 * \return         The status of sending pull_vCard_listing message.
 * \retval true    Request was sent successfully.
 * \retval false   Request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_vcard_listing_by_number_pull(uint8_t  bd_addr[6],
                                          char    *phone_number);

/**
 * \brief  Send a request to pull an individual vCard entry from the phone book object of interest.

 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in]  bd_addr        Remote BT address.
 * \param[in]  p_name         The object name of vCard .
 * \param[in]  name_len       The length of name.
 * \param[in]  filter         The properties contained in the vCard object.
 *
 * \return         The status of sending the command.
 * \retval true    Request was sent successfully.
 * \retval false   Request was failed to send.
 *
 * \ingroup BT_PBAP
 */
bool bt_pbap_pull_vcard_entry(uint8_t   bd_addr[6],
                              uint8_t  *p_name,
                              uint8_t   name_len,
                              uint64_t  filter);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_PBAP_H_ */
