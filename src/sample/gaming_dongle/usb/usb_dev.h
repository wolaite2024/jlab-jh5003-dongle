#ifndef __USB_DEV_H__
#define __USB_DEV_H__

/** @defgroup USB_DEV USB Device
  * @brief USB Device related descriptors
  * @{
  */

typedef enum
{
    STRING_ID_UNDEFINED,
    STRING_ID_MANUFACTURER  = 1,
    STRING_ID_PRODUCT       = 2,
    STRING_ID_SERIALNUM     = 3,
    STRING_ID_UAC_1st       = 4,
    STRING_ID_UAC_2st       = 5,

    STRING_ID_TEAMS_UCQ     = 33,
} T_STRING_ID;

typedef enum
{
    STRING_IDX_MANUFACTURER,
    STRING_IDX_PRODUCT,
    STRING_IDX_SERIALNUM,
    STRING_IDX_TEAMS_UCQ,
    STRING_IDX_UAC_1st,
    STRING_IDX_UAC_2st,

    STRING_IDX_MAX
} T_STRING_IDX;

/**
 * usb_dev.h
 *
 * \brief   usb device-related strings.
 *
 * \param idVendor Vendor ID
 * \param idProduct  Product ID
 * \param bcdDevice  Device release number in binary-coded decimal
 *
 * \ingroup USB_DEV
 */
typedef struct _usb_dev_desc_cfg
{
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
} T_USB_DEV_DESC_CFG;

/**
 * usb_dev.h
 *
 * \brief   configure usb device descriptors.
 *
 * \details configure USB device descriptors.
* \param[in] dev_desc_cfg \ref T_USB_DEV_DESC_CFG
 *
 * \ingroup USB_DEV
 */
void usb_dev_desc_rcfg(T_USB_DEV_DESC_CFG dev_desc_cfg);

/**
 * usb_dev_driver.h
 *
 * \brief   USB audio interface callback register.
 *
 * \param[in] index  in \ref T_STRING_IDX
 * \param[in] *s descriptor string
 *
 */
bool usb_dev_desc_string_rcfg(T_STRING_IDX index, const char *s);

/**
 * usb_dev.h
 *
 * \brief   usb device init.
 *
 * \details register USB device/configuration/string descriptors
 *
 * \ingroup USB_DEV
 */
void usb_dev_init(void);

/** @}*/
/** End of USB_DEV
*/

#endif
