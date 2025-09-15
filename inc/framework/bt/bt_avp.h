/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _BT_AVP_H_
#define _BT_AVP_H_


#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum t_bt_avp_bud_location
{
    BT_AVP_BUD_UNKNOWN,
    BT_AVP_BUD_IN_EAR,
    BT_AVP_BUD_OUT_OF_CASE,
    BT_AVP_BUD_IN_CASE,
} T_BT_AVP_BUD_LOCATION;

typedef enum t_bt_avp_control
{
    BT_AVP_CONTROL_NONE                 = 0x00,
    BT_AVP_CONTROL_SIRI                 = 0x01,
    BT_AVP_CONTROL_PLAY_PAUSE           = 0x02,
    BT_AVP_CONTROL_FORWARD              = 0x03,
    BT_AVP_CONTROL_BACKWARD             = 0x04,
} T_BT_AVP_CONTROL;

typedef enum
{
    BT_AVP_CONTROL_VOICE_RECOGNITION    = 0x00,
    BT_AVP_CONTROL_ANC                  = 0x01,
    BT_AVP_CONTROL_VOL_DOWN             = 0x02,
    BT_AVP_CONTROL_VOL_UP               = 0x03,
} T_BT_AVP_LONGPRESS_CONTROL;

typedef enum t_bt_avp_anc
{
    BT_AVP_ANC_CLOSE                  = 0x01,
    BT_AVP_ANC_OPEN                   = 0x02,
    BT_AVP_ANC_TRANSPARENCY_MODE      = 0x03,
} T_BT_AVP_ANC;

typedef enum t_bt_avp_mic
{
    BT_AVP_MIC_AUTO                  = 0x00,
    BT_AVP_MIC_ALLWAYS_RIGHT         = 0x01,
    BT_AVP_MIC_ALLWAYS_LEFT          = 0x02,
} T_BT_AVP_MIC;

typedef enum t_bt_avp_click_speed
{
    BT_AVP_CLICK_SPEED_DEFAULT        = 0x00,
    BT_AVP_CLICK_SPEED_SLOW           = 0x01,
    BT_AVP_CLICK_SPEED_SLOWEST        = 0x02,
} T_BT_AVP_CLICK_SPEED;

typedef enum t_bt_avp_long_press_time
{
    BT_AVP_LONG_PRESS_TIME_DEFAULT    = 0x00,
    BT_AVP_LONG_PRESS_TIME_SHORT      = 0x01,
    BT_AVP_LONG_PRESS_TIME_SHORTEST   = 0x02,
} T_BT_AVP_LONG_PRESS_TIME;

typedef enum t_bt_avp_event
{
    BT_AVP_EVENT_CONN_CMPL       = 0x00,
    BT_AVP_EVENT_DISCONN_CMPL    = 0x01,
    BT_AVP_EVENT_DATA_IND        = 0x02,
} T_BT_AVP_EVENT;

typedef struct t_bt_avp_event_conn_cmpl
{
    uint8_t bd_addr[6];
} T_BT_AVP_EVENT_CONN_CMPL;

typedef struct t_bt_avp_event_disconn_cmpl
{
    uint8_t bd_addr[6];
} T_BT_AVP_EVENT_DISCONN_CMPL;

typedef struct t_bt_avp_event_data_ind
{
    uint8_t    bd_addr[6];
    uint8_t   *data;
    uint16_t   len;
} T_BT_AVP_EVENT_DATA_IND;

typedef union t_bt_avp_event_param
{
    T_BT_AVP_EVENT_CONN_CMPL       conn_cmpl;
    T_BT_AVP_EVENT_DISCONN_CMPL    disconn_cmpl;
    T_BT_AVP_EVENT_DATA_IND        data_ind;
} T_BT_AVP_EVENT_PARAM;

typedef void (* P_BT_AVP_CBACK)(T_BT_AVP_EVENT  event_type,
                                void           *event_buf,
                                uint16_t        buf_len);

bool bt_avp_init(P_BT_AVP_CBACK cback);

void bt_avp_deinit(void);

bool bt_avp_connect_req(uint8_t bd_addr[6]);

bool bt_avp_disconnect_req(uint8_t bd_addr[6]);

bool bt_avp_data_send(uint8_t   bd_addr[6],
                      uint8_t  *data,
                      uint16_t  data_len,
                      bool      flush);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
