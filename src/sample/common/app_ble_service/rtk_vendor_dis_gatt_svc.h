

#ifndef _REALTEK_VENDOR_DIS_GATT_SVC_H_
#define _REALTEK_VENDOR_DIS_GATT_SVC_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include "bt_gatt_svc.h"

#define RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_LE      0x01 //BIT0
#define RVDIS_PREFERRED_GATT_TRANSPORT_TYPE_BREDR   0x02 //BIT1

#define GATT_UUID_CHAR_PREFERRED_GATT_TRANSPORT_TYPE    0xF001
#define GATT_CHAR_PREFERRED_GATT_TRANSPORT_TYPE_LEN     1
#define GATT_UUID_CHAR_CURRENT_GATT_TRANSPORT_TYPE      0xF002
#define GATT_CHAR_CURRENT_GATT_TRANSPORT_TYPE_LEN       1

#define GATT_MSG_RVDIS_SERVER_READ_CHAR_IND      0x00

typedef T_APP_RESULT(*P_FUN_RVDIS_SERVER_APP_CB)(uint16_t conn_handle, uint16_t cid,
                                                 uint8_t type, void *p_data);

typedef struct
{
    T_SERVER_ID service_id;
    uint16_t    char_uuid;
    uint16_t    offset;
} T_RVDIS_SERVER_READ_CHAR_IND;

T_SERVER_ID rvdis_add_service(P_FUN_RVDIS_SERVER_APP_CB app_cb);

bool rvdis_char_read_confirm(uint16_t conn_handle, uint16_t cid, uint8_t service_id,
                             uint16_t char_uuid, uint16_t offset,
                             uint16_t value_len, uint8_t *p_value);


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _REALTEK_VENDOR_DIS_GATT_SVC_H_ */
