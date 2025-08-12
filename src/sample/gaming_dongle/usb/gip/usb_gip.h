#ifndef __USB_GIP_H__
#define __USB_GIP_H__

#include "usb_gip_driver.h"
#include "usb_audio.h"

/** @defgroup APP_USB APP USB
  * @brief app usb module.
  * @{
  */

/** @defgroup USB_GIP_IF USB GIP Interface
  * @brief USB GIP interface
  * @{
  */

#define GIP_INT_OUT_ENDPOINT_ADDRESS                0x01
#define GIP_INT_IN_ENDPOINT_ADDRESS                 0x81
#define GIP_ISO_OUT_ENDPOINT_ADDRESS                0x03
#define GIP_ISO_IN_ENDPOINT_ADDRESS                 0x83

#define GIP_CONGESTION_CTRL_DROP_CUR    GIP_DRIVER_CONGESTION_CTRL_DROP_CUR
#define GIP_CONGESTION_CTRL_DROP_FIRST  GIP_DRIVER_CONGESTION_CTRL_DROP_FIRST

typedef uint32_t (*USB_GIP_DATA_PIPE_CB)(void *handle, void *buf, uint32_t len, int status);

/**
 * usb_gip.h
 *
 * \brief   USB GIP control attribute.
 *
 * \ingroup USB_GIP_IF
 */
typedef struct _usb_gip_ctrl_attr
{
    uint8_t dir;
    union
    {
        uint32_t d32;
        struct
        {
            uint32_t      type: 16;
            uint32_t      value: 16;
        } vol;

        struct
        {
            uint32_t value;
        } mute;

        struct
        {
            uint32_t bit_width: 6;
            uint32_t channels: 4;
            uint32_t sample_rate: 22;
        } audio;
    } content;

} T_USB_GIP_PIPE_ATTR;

struct _usb_gip_pipes;
typedef int (*USB_GIP_PIPE_CB_STREAM)(struct _usb_gip_pipes *pipe, void *buf, uint32_t len);
typedef int (*USB_GIP_PIPE_CB_CTRL)(struct _usb_gip_pipes *pipe, T_USB_AUDIO_CTRL_EVT evt,
                                    T_USB_GIP_PIPE_ATTR ctrl);

/**
 * usb_gip.h
 *
 * \brief   USB GIP PIPE callbacks to transmit audio data or control infomation to upper sw \n
 *          \ref ctrl: transmit vol and mute control infomation, param \ref pipe is defined \n
 *          in \ref T_USB_GIP_PIPES, \ref event is defined in \ref T_USB_GIP_CTRL_EVT, \ref \n
 *          ctrl is defined in T_USB_GIP_CTRL_ATTR.
 *          \ref upstream: transmit data from device to host, param \ref pipe is defined in \n
 *          \ref T_USB_GIP_PIPES, \ref buf is audio data will be sent, \ref len is length of \n
 *          \ref buf \n
 *          \ref downstream: transmit data from host to device, param \ref pipe is defined in \n
 *          \ref T_USB_GIP_PIPES, \ref buf is audio data has been received, \ref len is length \n
 *          of \ref buf \n
 *
 * \ingroup USB_GIP_IF
 */
typedef struct _usb_gip_pipes
{
    uint32_t label;
    USB_GIP_PIPE_CB_CTRL ctrl;
    USB_GIP_PIPE_CB_STREAM upstream;
    USB_GIP_PIPE_CB_STREAM downstream;
} T_USB_GIP_PIPES;

/**
 * usb_gip.h
 *
 * \brief   USB GIP pipe attr
 *          \ref zlp: zero length packet
 *          \ref high_throughput: if it is set to 1, it can be be executed in interrupt, else it excute in task.
 *          \ref congestion_ctrl: if it is set to 0, drop current data, else drop the first data in the queue.
 *          \ref rsv: reserved
 *          \ref mtu: maximum transfer unit
 *
 * \ingroup USB_GIP
 */
typedef struct _usb_gip_attr
{
    uint16_t zlp: 1;
    uint16_t high_throughput: 1;
    uint16_t congestion_ctrl: 2;
    uint16_t rsv: 12;
    uint16_t mtu;
} T_USB_GIP_ATTR;

/**
 * usb_gip.h
 *
 * \brief   open gip data pipe
 *
 * \param[in]  ep_addr ep address
 * \param[in]  attr GIP pipe attribute of \ref T_USB_GIP_ATTR
 * \param[in]  pending_req_num supported pending request number
 * \param[in] cb callback of \ref USB_GIP_DATA_PIPE_CB, which will be called after data is sent over
 *
 * \return handle
 *
 * \ingroup USB_GIP
 */
void *usb_gip_data_pipe_open(uint8_t ep_addr, T_USB_GIP_ATTR attr, uint8_t pending_req_num,
                             USB_GIP_DATA_PIPE_CB cb);

/**
 * usb_gip.h
 *
 * \brief   gip pipe send data
 *
 * \details data is sent serially, which means data is not sent actually until previous data transmission is complete.
 *
 * \param[in]  handle return value of \ref usb_gip_data_pipe_open
 * \param[in]  buf data will be sent
 * \param[in]  len length of data
 *
 * \return true data will be sent now or after all previous transmissions are complete, false will never be sent
 *
 * \ingroup USB_GIP
 */
bool usb_gip_data_pipe_send(void *handle, void *buf, uint32_t len);

/**
 * usb_gip.h
 *
 * \brief   close gip data pipe
 *
 * \param[in]  handle return value of \ref usb_gip_data_pipe_open
 *
 * \return int result, refer to "errno.h"
 *
 * \ingroup USB_GIP
 */
int usb_gip_data_pipe_close(void *handle);

/**
 * usb_gip.h
 *
 * \brief   USB GIP init
 *
 * \ingroup USB_GIP_IF
 */
void usb_gip_init(T_USB_GIP_PIPES *pipe);

/** @}*/
/** End of USB_GIP_IF
*/

/** @}*/
/** End of APP_USB
*/
#endif
