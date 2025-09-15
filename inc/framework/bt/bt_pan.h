/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_PAN_H_
#define _BT_PAN_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    BT_PAN BT Personal Area Networking Profile (PAN)
 *
 * \brief   Provide BT PAN profile interfaces.
 */

/**
 * \brief  BT PAN event type for \ref T_BT_PAN_CBACK.
 *
 * \ingroup BT_PAN
 */
typedef enum t_bt_pan_event
{
    /**
     * \brief Indicate PAN connection indication \ref T_BT_PAN_EVENT_PARAM_CONN_IND.
     */
    BT_PAN_EVENT_CONN_IND                             = 0x00,

    /**
     * \brief Indicate PAN connection complete \ref T_BT_PAN_EVENT_PARAM_CONN_CMPL.
     */
    BT_PAN_EVENT_CONN_CMPL                            = 0x01,

    /**
     * \brief Indicate PAN connection fail \ref T_BT_PAN_EVENT_PARAM_CONN_FAIL.
     */
    BT_PAN_EVENT_CONN_FAIL                            = 0x02,

    /**
     * \brief Indicate PAN disconnection complete \ref T_BT_PAN_EVENT_PARAM_DISCONN_CMPL.
     */
    BT_PAN_EVENT_DISCONN_CMPL                         = 0x03,

    /**
     * \brief Indicate PAN setup connection indication \ref T_BT_PAN_EVENT_PARAM_SETUP_CONN_IND.
     */
    BT_PAN_EVENT_SETUP_CONN_IND                       = 0x04,

    /**
     * \brief Indicate PAN setup connection response \ref T_BT_PAN_EVENT_PARAM_SETUP_CONN_RSP.
     */
    BT_PAN_EVENT_SETUP_CONN_RSP                       = 0x05,

    /**
     * \brief Indicate PAN filter network type set indication \ref T_BT_PAN_EVENT_PARAM_FILTER_NET_TYPE_SET_IND.
     */
    BT_PAN_EVENT_FILTER_NET_TYPE_SET_IND              = 0x06,

    /**
     * \brief Indicate PAN filter network type set response \ref T_BT_PAN_EVENT_PARAM_FILTER_NET_TYPE_SET_RSP.
     */
    BT_PAN_EVENT_FILTER_NET_TYPE_SET_RSP              = 0x07,

    /**
     * \brief Indicate PAN filter multicast address set indication \ref T_BT_PAN_EVENT_PARAM_FILTER_MULTI_ADDR_SET_IND.
     */
    BT_PAN_EVENT_FILTER_MULTI_ADDR_SET_IND            = 0x08,

    /**
     * \brief Indicate PAN filter multicast address set response \ref T_BT_PAN_EVENT_PARAM_FILTER_MULTI_ADDR_SET_RSP.
     */
    BT_PAN_EVENT_FILTER_MULTI_ADDR_SET_RSP            = 0x09,

    /**
     * \brief Indicate PAN ethernet packet indication \ref T_BT_PAN_EVENT_PARAM_ETHERNET_PACKET_IND.
     */
    BT_PAN_EVENT_ETHERNET_PACKET_IND                  = 0x0a,
} T_BT_PAN_EVENT;

/**
 * \brief  BT PAN event parameter PAN connection indication.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_conn_ind
{
    uint8_t bd_addr[6];
} T_BT_PAN_EVENT_PARAM_CONN_IND;

/**
 * \brief  BT PAN event parameter PAN connection completed.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_conn_cmpl
{
    uint8_t bd_addr[6];
} T_BT_PAN_EVENT_PARAM_CONN_CMPL;

/**
 * \brief  BT PAN event parameter PAN connection fail.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_conn_fail
{
    uint8_t  bd_addr[6];
    uint16_t cause;
} T_BT_PAN_EVENT_PARAM_CONN_FAIL;

/**
 * \brief  BT PAN event parameter PAN disconnection completed.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_disconn_cmpl
{
    uint8_t  bd_addr[6];
    uint16_t cause;
} T_BT_PAN_EVENT_PARAM_DISCONN_CMPL;

/**
 * \brief  BT PAN event parameter PAN setup connection indication.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_setup_conn_ind
{
    uint8_t  bd_addr[6];
    uint16_t dst_uuid;
    uint16_t src_uuid;
} T_BT_PAN_EVENT_PARAM_SETUP_CONN_IND;

/**
 * \brief  BT PAN event parameter PAN setup connection response.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_setup_conn_rsp
{
    uint8_t  bd_addr[6];
    uint16_t rsp_msg;
} T_BT_PAN_EVENT_PARAM_SETUP_CONN_RSP;

/**
 * \brief  BT PAN event parameter PAN filter network type set indication.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_filter_net_type_set_ind
{
    uint8_t   bd_addr[6];
    uint16_t  filter_num;
    uint16_t *filter_start;
    uint16_t *filter_end;
} T_BT_PAN_EVENT_PARAM_FILTER_NET_TYPE_SET_IND;

/**
 * \brief  BT PAN event parameter PAN filter network type set response.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_filter_net_type_set_rsp
{
    uint8_t  bd_addr[6];
    uint16_t rsp_msg;
} T_BT_PAN_EVENT_PARAM_FILTER_NET_TYPE_SET_RSP;

/**
 * \brief  BT PAN event parameter PAN filter multicast address set indication.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_filter_multi_addr_set_ind
{
    uint8_t   bd_addr[6];
    uint16_t  filter_num;
    uint8_t (*filter_start)[6];
    uint8_t (*filter_end)[6];
} T_BT_PAN_EVENT_PARAM_FILTER_MULTI_ADDR_SET_IND;

/**
 * \brief  BT PAN event parameter PAN filter multicast address set response.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_filter_multi_addr_set_rsp
{
    uint8_t  bd_addr[6];
    uint16_t rsp_msg;
} T_BT_PAN_EVENT_PARAM_FILTER_MULTI_ADDR_SET_RSP;

/**
 * \brief  BT PAN event parameter PAN ethernet packet indication.
 *
 * \ingroup BT_PAN
 */
typedef struct t_bt_pan_event_param_ethernet_packet_ind
{
    uint8_t   bd_addr[6];
    uint8_t  *buf;
    uint16_t  len;
} T_BT_PAN_EVENT_PARAM_ETHERNET_PACKET_IND;

/**
 * \brief  BT PAN event parameter.
 *
 * \ingroup BT_PAN
 */
typedef union t_bt_pan_event_param
{
    T_BT_PAN_EVENT_PARAM_CONN_IND                   pan_conn_ind;
    T_BT_PAN_EVENT_PARAM_CONN_CMPL                  pan_conn_cmpl;
    T_BT_PAN_EVENT_PARAM_CONN_FAIL                  pan_conn_fail;
    T_BT_PAN_EVENT_PARAM_DISCONN_CMPL               pan_disconn_cmpl;
    T_BT_PAN_EVENT_PARAM_SETUP_CONN_IND             pan_setup_conn_ind;
    T_BT_PAN_EVENT_PARAM_SETUP_CONN_RSP             pan_setup_conn_rsp;
    T_BT_PAN_EVENT_PARAM_FILTER_NET_TYPE_SET_IND    pan_filter_net_type_set_ind;
    T_BT_PAN_EVENT_PARAM_FILTER_NET_TYPE_SET_RSP    pan_filter_net_type_set_rsp;
    T_BT_PAN_EVENT_PARAM_FILTER_MULTI_ADDR_SET_IND  pan_filter_multi_addr_set_ind;
    T_BT_PAN_EVENT_PARAM_FILTER_MULTI_ADDR_SET_RSP  pan_filter_multi_addr_set_rsp;
    T_BT_PAN_EVENT_PARAM_ETHERNET_PACKET_IND        pan_ethernet_packet_ind;
} T_BT_PAN_EVENT_PARAM;

/**
 * \brief  BT PAN supported features.
 *
 * \ingroup BT_PAN
 */
typedef enum t_bt_pan_role
{
    BT_PAN_ROLE_PANU = 0x00,
    BT_PAN_ROLE_NAP  = 0x01,
    BT_PAN_ROLE_GN   = 0x02,
} T_BT_PAN_ROLE;

/**
 * \brief  BT PAN event callback prototype.
 *
 * \param[in] event_type   BT PAN event type.
 * \param[in] event_buf    Event buffer address.
 * \param[in] buf_len      Event buffer length.
 *
 * \ingroup  BT_PAN
 */
typedef void (*T_BT_PAN_CBACK)(T_BT_PAN_EVENT  event_type,
                               void           *event_buf,
                               uint16_t        buf_len);

/**
 * \brief  Initialize PAN profile.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return          The status of initializing PAN profile.
 * \retval true     PAN profile was initialized successfully.
 * \retval false    PAN profile was failed to initialize.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_init(void);

/**
 * \brief  De-initialize PAN profile.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \ingroup BT_PAN
 */
void bt_pan_deinit(void);

/**
 * \brief   BT PAN event callback register.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] cback       Callback function \ref T_BT_PAN_CBACK
 *
 * \return                The status of BT PAN event callback register.
 * \retval    true        BT PAN event callback was registered successfully.
 * \retval    false       BT PAN event callback was failed to register.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_cback_register(T_BT_PAN_CBACK cback);

/**
 * \brief   BT PAN event callback unregister.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] cback       Callback function \ref T_BT_PAN_CBACK
 *
 * \return                The status of BT PAN event callback unregister.
 * \retval    true        BT PAN event callback was unregistered successfully.
 * \retval    false       BT PAN event callback was failed to unregister.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_cback_unregister(T_BT_PAN_CBACK cback);

/**
 * \brief  Send a PAN connection request.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] local_addr    Local BT address.
 * \param[in] remote_addr   Remote BT address.
 * \param[in] src_role      Source PAN role \ref T_BT_PAN_ROLE.
 * \param[in] dst_role      Destination PAN role \ref T_BT_PAN_ROLE.
 *
 * \return          The status of sending the PAN connection request.
 * \retval true     PAN connection request was sent successfully.
 * \retval false    PAN connection request was failed to send.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_connect_req(uint8_t       local_addr[6],
                        uint8_t       remote_addr[6],
                        T_BT_PAN_ROLE src_role,
                        T_BT_PAN_ROLE dst_role);

/**
 * \brief  Send a PAN connection confirmation.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] local_addr    Local BT address.
 * \param[in] remote_addr   Remote BT address.
 * \param[in] accept        Accept or reject the connection indication.
 * \arg    true     Accept the connection indication.
 * \arg    false    Reject the connection indication.
 *
 * \return          The status of sending the PAN connection confirmation.
 * \retval true     PAN connection confirmation was sent successfully.
 * \retval false    PAN connection confirmation was failed to send.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_connect_cfm(uint8_t local_addr[6],
                        uint8_t remote_addr[6],
                        bool    accept);

/**
 * \brief  Send a PAN disconnection request.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr   Remote BT address.
 *
 * \return          The status of sending the PAN disconnection request.
 * \retval true     PAN disconnection request was sent successfully.
 * \retval false    PAN disconnection request was failed to send.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_disconnect_req(uint8_t bd_addr[6]);

/**
 * \brief  Send a PAN setup connection request.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr     Remote BT address.
 * \param[in] src_role    Source PAN role \ref T_BT_PAN_ROLE.
 * \param[in] dst_role    Destination PAN role \ref T_BT_PAN_ROLE.
 *
 * \return          The status of sending the PAN setup connection request.
 * \retval true     PAN setup connection request was sent successfully.
 * \retval false    PAN setup connection request was failed to send.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_setup_connection_req(uint8_t       bd_addr[6],
                                 T_BT_PAN_ROLE src_role,
                                 T_BT_PAN_ROLE dst_role);

/**
 * \brief  Send a PAN setup connection response.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr    Remote BT address.
 * \param[in] rsp_msg    Response message.
 *
 * \return          The status of sending the PAN setup connection response.
 * \retval true     PAN setup connection response was sent successfully.
 * \retval false    PAN setup connection response was failed to send.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_setup_connection_rsp(uint8_t  bd_addr[6],
                                 uint16_t rsp_msg);

/**
 * \brief  Send a PAN network type filter set message.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr        Remote BT address.
 * \param[in] filter_num     Number of filters.
 * \param[in] start_array    Network protocol type range start array.
 * \param[in] end_array      Network protocol type range end array.
 *
 * \return          The status of sending the PAN network type filter set message.
 * \retval true     PAN network type filter set message was sent successfully.
 * \retval false    PAN network type filter set message was failed to send.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_filter_net_type_set(uint8_t   bd_addr[6],
                                uint16_t  filter_num,
                                uint16_t *start_array,
                                uint16_t *end_array);

/**
 * \brief  Send a PAN multicast address filter set message.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr        Remote BT address.
 * \param[in] filter_num     Number of filters.
 * \param[in] start_array    Multicast address start array.
 * \param[in] end_array      Multicast address end array.
 *
 * \return          The status of sending the PAN multicast address filter set message.
 * \retval true     PAN multicast address filter set message was sent successfully.
 * \retval false    PAN multicast address filter set message was failed to send.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_filter_multi_addr_set(uint8_t  bd_addr[6],
                                  uint16_t filter_num,
                                  uint8_t  start_array[][6],
                                  uint8_t  end_array[][6]);

/**
 * \brief  Send a PAN ethernet packet.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr    Remote BT address.
 * \param[in] buf        Ethernet packet buffer.
 * \param[in] len        Length of ethernet packet.
 *
 * \return          The status of sending the PAN ethernet packet.
 * \retval true     PAN ethernet packet was sent successfully.
 * \retval false    PAN ethernet packet was failed to send.
 *
 * \ingroup BT_PAN
 */
bool bt_pan_send(uint8_t   bd_addr[6],
                 uint8_t  *buf,
                 uint16_t  len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_PAN_H_ */
