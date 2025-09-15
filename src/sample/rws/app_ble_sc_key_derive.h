#ifndef _APP_BLE_SC_KEY_DERIVE_
#define _APP_BLE_SC_KEY_DERIVE_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//Distribution of keys
#define SMP_DIST_ENC_KEY        0x01/*distribute LTK*/
#define SMP_DIST_ID_KEY         0x02/*distribute IRK*/
#define SMP_DIST_SIGN_KEY       0x04/*distribute CSRK*/
#define SMP_DIST_LINK_KEY       0x08/*distribute link key*/

void app_ble_key_derive_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
