#ifndef _APP_BLE_SERVICE_H_
#define _APP_BLE_SERVICE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_BLE_SERVICE App Ble Service
  * @brief App Ble Service
  * @{
  */
enum
{
    LE_SEND_OK       = 0x00,
    LE_NOT_CONN      = 0x01,
    LE_NOT_READY     = 0x02,
    LE_SEND_FAIL     = 0x03,
};
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_BLE_SERVICE_Exported_Macros App Ble Service Macros
   * @{
   */
#define TX_ENABLE_CCCD_BIT                  0x01
#define TX_ENABLE_AUTHEN_BIT                0x02
#define TX_ENABLE_READY                     (TX_ENABLE_CCCD_BIT|TX_ENABLE_AUTHEN_BIT)
#define BLE_SERVICE_MAX_NUM                 0x16
/** End of APP_BLE_SERVICE_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_BLE_SERVICE_Exported_Functions App Ble Service Functions
    * @{
    */
/**
    * @brief  Ble Service module init
    * @param  void
    * @return void
    */
void app_ble_service_init(void);

uint8_t app_ble_service_transfer(uint8_t app_idx, uint8_t *data, uint32_t len);
/** @} */ /* End of group APP_BLE_SERVICE_Exported_Functions */

/** End of APP_BLE_SERVICE
* @}
*/



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_BLE_SERVICE_H_ */
