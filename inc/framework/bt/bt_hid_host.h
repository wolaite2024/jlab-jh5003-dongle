/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_HID_HOST_H_
#define _BT_HID_HOST_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup    BT_HID_HOST BT HID Profile
 *
 * \brief   Provide BT HID profile interfaces.
 */

/**
 * \brief  BT HID protocol mode.
 *
 * \ingroup BT_HID_HOST
 */
typedef enum t_bt_hid_host_protocol_mode
{
    BT_HID_HOST_BOOT_PROTO_MODE             = 0x00,
    BT_HID_HOST_REPORT_PROTO_MODE           = 0x01,
} T_BT_HID_HOST_PROTOCOL_MODE;

/**
 * \brief  BT HID report type.
 *
 * \ingroup BT_HID_HOST
 */
typedef enum t_bt_hid_host_report_type
{
    BT_HID_HOST_REPORT_TYPE_RESERVED          = 0x00,
    BT_HID_HOST_REPORT_TYPE_INPUT             = 0x01,
    BT_HID_HOST_REPORT_TYPE_OUTPUT            = 0x02,
    BT_HID_HOST_REPORT_TYPE_FEATURE           = 0x03,
} T_BT_HID_HOST_REPORT_TYPE;

/**
 * \brief  BT HID control operation.
 *
 * \ingroup BT_HID_HOST
 */
typedef enum t_bt_hid_host_control_operation
{
    BT_HID_HOST_CONTROL_OPERATION_NOP                     = 0x00,
    BT_HID_HOST_CONTROL_OPERATION_HARD_RESET              = 0x01,
    BT_HID_HOST_CONTROL_OPERATION_SOFT_RESET              = 0x02,
    BT_HID_HOST_CONTROL_OPERATION_SUSPEND                 = 0x03,
    BT_HID_HOST_CONTROL_OPERATION_EXIT_SUSPEND            = 0x04,
    BT_HID_HOST_CONTROL_OPERATION_VIRTUAL_CABLE_UNPLUG    = 0x05,
} T_BT_HID_HOST_CONTROL_OPERATION;

/**
 * \brief  Initialize HID profile.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] boot_proto_mode   Support boot protocol mode.
 *
 * \return          The status of initializing HID profile.
 * \retval true     HID profile was initialized successfully.
 * \retval false    HID profile was failed to initialize.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_init(bool boot_proto_mode);

/**
 * \brief  Set HID descriptor.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr             Remote BT address.
 * \param[in] descriptor          HID report descriptor.
 * \param[in] len                 The length of HID report descriptor.
 *
 * \return          The status of set HID descriptor.
 * \retval true     HID descriptor was set successfully.
 * \retval false    HID descriptor was failed to set.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_descriptor_set(uint8_t   bd_addr[6],
                                uint8_t  *descriptor,
                                uint16_t  len);

/**
 * \brief  Send HID connection request.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr      Remote BT address.
 * \param[in] proto_mode   Protocol mode.
 *
 * \return          The status of sending the HID connection request.
 * \retval true     HID connection request was sent successfully.
 * \retval false    HID connection request was failed to send.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_connect_req(uint8_t bd_addr[6],
                             uint8_t proto_mode);

/**
 * \brief  Send HID connection confirmation.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr   Remote BT address.
 * \param[in] accept    Accept or reject the connection indication.
 * \arg    true     Accept the connection indication.
 * \arg    false    Reject the connection indication.
 *
 * \return          The status of sending the HID connection confirmation.
 * \retval true     HID connection confirmation was sent successfully.
 * \retval false    HID connection confirmation was failed to send.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_connect_cfm(uint8_t bd_addr[6],
                             bool    accept);

/**
 * \brief  Send HID disconnection request.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr   Remote BT address.
 *
 * \return          The status of sending the HID disconnection request.
 * \retval true     HID disconnection request was sent successfully.
 * \retval false    HID disconnection request was failed to send.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_disconnect_req(uint8_t bd_addr[6]);

/**
 * \brief  Send HID control message over control channel.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr    Remote BT address.
 * \param[in] operation  Report Type \ref T_BT_HID_HOST_CONTROL_OPERATION.
 *
 * \return          The status of sending HID control message over control channel.
 * \retval true     HID control message was sent successfully.
 * \retval false    HID control message was failed to send.
 *
 * \note            A HID_CONTROL message with a parameter of VIRTUAL_CABLE_UNPLUG is the only
 *                  HID_CONTROL message a Bluetooth HID device may send to a Bluetooth HID Host.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_control_req(uint8_t                          bd_addr[6],
                             T_BT_HID_HOST_CONTROL_OPERATION  operation);

/**
 * \brief  Send HID GET_REPORT message over control channel. This message is used by the Bluetooth
 *         HID Host to request the transfer of a HID report from the Bluetooth HID device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr            Remote BT address.
 * \param[in] report_type        Report Type \ref T_BT_HID_HOST_REPORT_TYPE.
 * \param[in] report_id          Report ID.
 * \param[in] buffer_size        Buffer size.
 *
 * \return          The status of sending HID GET_REPORT message over control channel.
 * \retval true     HID GET_REPORT message was sent successfully.
 * \retval false    HID GET_REPORT message was failed to send.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_get_report_req(uint8_t                   bd_addr[6],
                                T_BT_HID_HOST_REPORT_TYPE report_type,
                                uint8_t                   report_id,
                                uint16_t                  buffer_size);

/**
 * \brief  Send HID SET_REPORT message over control channel. This message is used by a Bluetooth
 *         HID Host to initiate the transfer of a report to a Bluetooth HID device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr            Remote BT address.
 * \param[in] report_type        Report type \ref T_BT_HID_HOST_REPORT_TYPE.
 * \param[in] buf                Message buffer.
 * \param[in] len                Message length.
 *
 * \return          The status of sending HID SET_REPORT message over control channel.
 * \retval true     HID SET_REPORT message was sent successfully.
 * \retval false    HID SET_REPORT message was failed to send.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_set_report_req(uint8_t                    bd_addr[6],
                                T_BT_HID_HOST_REPORT_TYPE  report_type,
                                uint8_t                   *buf,
                                uint16_t                   len);

/**
 * \brief  Send HID GET_PROTOCOL message over control channel. This message is used by a Bluetooth
 *         HID Host to retrieve the protocol mode of the Bluetooth HID device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr            Remote BT address.
 *
 * \return          The status of sending HID GET_PROTOCOL message over control channel.
 * \retval true     HID GET_PROTOCOL message was sent successfully.
 * \retval false    HID GET_PROTOCOL message was failed to send.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_get_protocol_req(uint8_t bd_addr[6]);


/**
 * \brief  Send HID SET_PROTOCOL message over control channel. This message is used by a Bluetooth
 *         HID Host to set protocol mode on the Bluetooth HID device.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr            Remote BT address.
 * \param[in] proto_mode         Protocol mode \ref T_BT_HID_HOST_PROTOCOL_MODE.
 *
 * \return          The status of sending HID SET_PROTOCOL message over control channel.
 * \retval true     HID SET_PROTOCOL message was sent successfully.
 * \retval false    HID SET_PROTOCOL message was failed to send.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_set_protocol_req(uint8_t                     bd_addr[6],
                                  T_BT_HID_HOST_PROTOCOL_MODE proto_mode);


/**
 * \brief  Send HID DATA message over interrupt channel.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param[in] bd_addr           Remote BT address.
 * \param[in] report_type       Report Type \ref T_BT_HID_HOST_REPORT_TYPE.
 * \param[in] buf               Message buffer.
 * \param[in] len               Message length.
 *
 * \return          The status of sending HID DATA message over interrupt channel.
 * \retval true     HID DATA message was sent successfully.
 * \retval false    HID DATA message was failed to send.
 *
 * \ingroup BT_HID_HOST
 */
bool bt_hid_host_interrupt_data_send(uint8_t                    bd_addr[6],
                                     T_BT_HID_HOST_REPORT_TYPE  report_type,
                                     uint8_t                   *buf,
                                     uint16_t                   len);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BT_HID_HOST_H_ */
