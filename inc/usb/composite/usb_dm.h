#ifndef __USB_DM__
#define __USB_DM__
#include <stdbool.h>
#include <stdint.h>

/** @defgroup 87x3d_USB_DTM USB DTM
  * @brief app usb module.
  * @{
  */

/**
 * usb_dm.h
 *
 * \brief   usb device manager event used in \ref USB_DM_CB
 *
 */
typedef enum
{
    USB_DM_EVT_STATUS_IND                   = 0,
    USB_DM_EVT_BC12_DETECT                  = 1,
    USB_DM_EVT_EP_IN_NAK                    = 2,
    USB_DM_EVT_SETUP_PEEK                   = 3,
} T_USB_DM_EVT;

typedef union
{
    uint16_t d16;

    struct
    {
        uint16_t status_ind: 1;
        uint16_t bc12_det: 1;
        uint16_t in_nak: 1;
        uint16_t setup_peek: 1;
        uint16_t rsv: 12;
    } b;

} T_USB_DM_EVT_MSK;

/**
 * usb_dm.h
 *
 * \brief   usb power state obtained from event \ref T_USB_DM_EVT
 *
 */
typedef enum  {USB_PDN = 0,
               USB_ATTACHED,
               USB_POWERED,
               USB_DEFAULT,
               USB_ADDRESSED,
               USB_CONFIGURED,
               USB_SUSPENDED,
              } T_USB_POWER_STATE;

/**
 * \brief usb dm event parameter status ind.
 * \param state: current state
 * \param info: specific information depending on \ref state. \n
 *              if state is \ref USB_DEFAULT, it means actual enum speed
 *
 */
typedef struct _usb_dm_evt_param_status_ind
{
    T_USB_POWER_STATE state;
    union
    {
        uint8_t speed;
    } info;
} T_USB_DM_EVT_PARAM_STATUS_IND;

/**
 * \brief usb dm event parameter bc12 detect.
 * \param type: bc12 detect result
 */
typedef struct _usb_dm_evt_param_bc12_det
{
    uint8_t type;
} T_USB_DM_EVT_PARAM_BC12_DET;

/**
 * \brief usb dm event parameter in endpoint nak.
 * \param addr: endpoint address
 */
typedef struct _usb_dm_evt_param_ep_in_nak
{
    uint8_t ep_num;
} T_USB_DM_EVT_PARAM_EP_IN_NAK;

/**
 * \brief usb dm event parameter setup packet
 * \param pkt: setup packet
 */
typedef struct _usb_dm_evt_param_setup_peek
{
    uint8_t *pkt;
} T_USB_DM_EVT_PARAM_SETUP_PEEK;

/**
 * \brief The event parameters.
 *
 */
typedef union _usb_dm_evt_param
{
    T_USB_DM_EVT_PARAM_STATUS_IND status_ind;
    T_USB_DM_EVT_PARAM_BC12_DET bc12_det;
    T_USB_DM_EVT_PARAM_EP_IN_NAK ep_in_nak;
    T_USB_DM_EVT_PARAM_SETUP_PEEK setup_peek;
} T_USB_DM_EVT_PARAM;

/**
 * usb_dm.h
 *
 * \brief   USB device manager callback
 *
 * \param[in] T_USB_DM_EVT USB DM EVT defined in \ref T_USB_DM_EVT
 *
 * \param[in] uint32_t Optional parameter depending on different event
 *
 */
typedef bool (*USB_DM_CB)(T_USB_DM_EVT, T_USB_DM_EVT_PARAM *);

/**
 * usb_dm.h
 *
 * \brief   USB settings such as speed \ref USB_SPEED /interface .etc
 *
 */
typedef struct _t_usb_core_config
{
    uint8_t speed;
    struct
    {
        uint8_t uac_enable: 1;
        uint8_t hid_enable: 1;
        uint8_t gip_enable: 1;
        uint8_t rsv: 5;
    } class_set;
} T_USB_CORE_CONFIG;

/**
 * usb_dm.h
 *
 * \brief   USB speed definition
 *
 */
#define USB_SPEED_FULL  0
#define USB_SPEED_HIGH  1

/**
 * usb_dm.h
 *
 * \brief   USB core init
 *
 * \param[in] config USB core settings in \ref T_USB_CORE_CONFIG
 *
 */
void usb_dm_core_init(T_USB_CORE_CONFIG config);

/**
 * usb_dm.h
 *
 * \brief   USB core start, this api will start USB task
 *
 * \param[in] bc12_detect do bc12 detect if true.
 *
 */
void usb_dm_start(bool bc12_detect);

/**
 * usb_dm.h
 *
 * \brief   USB core start, this api will stop USB task
 *
 */
void usb_dm_stop(void);

/**
 * usb_dm.h
 *
 * \brief   register USB dm callback
 *
 * \param evt_msk USB dm event mask \ref T_USB_DM_EVT_MSK
 * \param cb USB dm callback \ref USB_DM_CB
 *
 */
void usb_dm_cb_register(T_USB_DM_EVT_MSK evt_msk, USB_DM_CB cb);

/**
 * usb_dm.h
 *
 * \brief   change event mask
 *
 * \param cb USB dm callback \ref USB_DM_CB
 * \param evt_msk USB dm event mask \ref T_USB_DM_EVT_MSK
 *
 */
void usb_dm_evt_msk_change(T_USB_DM_EVT_MSK evt_msk, USB_DM_CB cb);

/**
 * usb_dm.h
 *
 * \brief   unregister USB dm callback
 *
 * \param cb USB dm callback \ref USB_DM_CB
 */
void usb_dm_cb_unregister(USB_DM_CB cb);


/** @}*/
/** End of 87x3d_USB_DTM
*/
#endif

