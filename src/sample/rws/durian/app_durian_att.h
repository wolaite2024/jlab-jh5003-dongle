/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifdef __cplusplus
extern "C"  {
#endif      /* __cplusplus */

#define AVP_ATT_SERVICE_CHAR_READ_INDEX2                0x02

#define AVP_ATT_SERVICE_CHAR_READ_INDEX19               0x13
#define AVP_ATT_SERVICE_CHAR_READ_INDEX21               0x15
#define AVP_ATT_SERVICE_CHAR_READ_INDEX25               0x19

#define AVP_ATT_SERVICE_CHAR_NOTIFY_INDEX1             0x0c
#define AVP_ATT_SERVICE_CHAR_NOTIFY_INDEX2             0x06
#define AVP_ATT_SERVICE_CHAR_NOTIFY_INDEX3             0x0f

uint8_t avp_att_add_service(void *p_func);

bool avp_att_service_send_notify1(uint16_t conn_handle,  uint16_t cid,
                                  void *p_value, uint16_t length);

bool avp_att_service_send_notify2(uint16_t conn_handle,  uint16_t cid,
                                  void *p_value, uint16_t length);

bool avp_att_service_send_notify3(uint16_t conn_handle,  uint16_t cid,
                                  void *p_value, uint16_t length);

#ifdef __cplusplus
}
#endif
