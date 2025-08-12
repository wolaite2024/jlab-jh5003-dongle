/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _BQB_H_
#define _BQB_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * @defgroup APP_RWS_BQB App bqb
 * @brief Provides BQB interfaces for RWS BQB test.
 * @{
 */


/** @brief BQB command */
typedef enum
{
    BQB_CMD_NULL            = 0x0000,
    BQB_CMD_ENTER           = 0x0001,
    BQB_CMD_EXIT            = 0x0002,
    BQB_CMD_RESET           = 0x0003,
    BQB_CMD_POWER           = 0x0004,
    BQB_CMD_PAIR            = 0x0005,
    BQB_CMD_SDP             = 0x0006,
    BQB_CMD_AVDTP           = 0x0007,
    BQB_CMD_AVRCP           = 0x0008,
    BQB_CMD_RFCOMM          = 0x0009,
    BQB_CMD_HFHS            = 0x000A,
    BQB_CMD_PBAP            = 0x000B,
} T_BQB_CMD;


#define BQB_ACTION_POWER_ON                 0x01
#define BQB_ACTION_POWER_OFF                0x02

#define BQB_ACTION_PAIR_START               0x01
#define BQB_ACTION_PAIR_STOP                0x02

#define BQB_ACTION_SDP_SEARCH               0x01

#define BQB_ACTION_AVDTP_DISCOVER           0x01
#define BQB_ACTION_AVDTP_GET_CPBS           0x02
#define BQB_ACTION_AVDTP_SET_CFG            0x03
#define BQB_ACTION_AVDTP_GET_CFG            0x04
#define BQB_ACTION_AVDTP_RECFG              0x05
#define BQB_ACTION_AVDTP_OPEN               0x06
#define BQB_ACTION_AVDTP_START              0x07
#define BQB_ACTION_AVDTP_CLOSE              0x08
#define BQB_ACTION_AVDTP_SUSPEND            0x09
#define BQB_ACTION_AVDTP_ABORT              0x0A
#define BQB_ACTION_AVDTP_SECURITY_CONTROL   0x0B
#define BQB_ACTION_AVDTP_GET_ALL_CPBS       0x0C
#define BQB_ACTION_AVDTP_DELAY_REPORT       0x0D
#define BQB_ACTION_AVDTP_CONNECT_SIGNAL     0x20
#define BQB_ACTION_AVDTP_CONNECT_STREAM     0x21
#define BQB_ACTION_AVDTP_DISCONNECT         0x22

#define BQB_ACTION_AVRCP_CONNECT            0x01
#define BQB_ACTION_AVRCP_CONNECT_CONTROLLER 0x02
#define BQB_ACTION_AVRCP_CONNECT_TARGET     0x03
#define BQB_ACTION_AVRCP_GET_PLAY_STATUS    0x04
#define BQB_ACTION_AVRCP_GET_ELEMENT_ATTR   0x05
#define BQB_ACTION_AVRCP_PLAY               0x06
#define BQB_ACTION_AVRCP_PAUSE              0x07
#define BQB_ACTION_AVRCP_STOP               0x08
#define BQB_ACTION_AVRCP_REWIND             0x09
#define BQB_ACTION_AVRCP_FASTFORWARD        0x0A
#define BQB_ACTION_AVRCP_FORWARD            0x0B
#define BQB_ACTION_AVRCP_BACKWARD           0x0C
#define BQB_ACTION_AVRCP_NOTIFY_VOLUME      0x0D
#define BQB_ACTION_AVRCP_DISCONNECT         0x0E

#define BQB_ACTION_RFCOMM_CONNECT_SPP       0x01
#define BQB_ACTION_RFCOMM_CONNECT_HFP       0x02
#define BQB_ACTION_RFCOMM_CONNECT_HSP       0x03
#define BQB_ACTION_RFCOMM_CONNECT_PBAP      0x04
#define BQB_ACTION_RFCOMM_DISCONNECT_SPP    0x05
#define BQB_ACTION_RFCOMM_DISCONNECT_HFP    0x06
#define BQB_ACTION_RFCOMM_DISCONNECT_HSP    0x07
#define BQB_ACTION_RFCOMM_DISCONNECT_PBAP   0x08
#define BQB_ACTION_RFCOMM_DISCONNECT_ALL    0x09

#define BQB_ACTION_HFHS_CONNECT_SCO                   0x01
#define BQB_ACTION_HFHS_DISCONNECT_SCO                0x02
#define BQB_ACTION_HFHS_CALL_ANSWER                   0x03
#define BQB_ACTION_HFHS_CALL_ACTIVE                   0x04
#define BQB_ACTION_HFHS_CALL_END                      0x05
#define BQB_ACTION_HFHS_CALL_REJECT                   0x06
#define BQB_ACTION_HFHS_CALL_REDIAL                   0x07
#define BQB_ACTION_HFHS_SPK_GAIN_LEVEL_REPORT         0x08
#define BQB_ACTION_HFHS_SPK_UP                        0x09
#define BQB_ACTION_HFHS_SPK_DOWN                      0x0A
#define BQB_ACTION_HFHS_MIC_UP                        0x0B
#define BQB_ACTION_HFHS_MIC_DOWN                      0x0C
#define BQB_ACTION_HFHS_VOICE_RECOGNITION_ACTIVATE    0x0D
#define BQB_ACTION_HFHS_VOICE_RECOGNITION_DEACTIVATE  0x0E

#define BQB_ACTION_PBAP_VCARD_SRM               0x01
#define BQB_ACTION_PBAP_VCARD_NOSRM             0x02
#define BQB_ACTION_PBAP_VCARD_ENTRY             0x03


/**
  * @brief  Register bqb command.
  * @param  None
  * @return The status of registering command.
  * @retval true    Command has been registered successfully.
  * @retval false   Command was fail to register.
  */
bool bqb_cmd_register(void);


/** End of APP_RWS_BQB
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BQB_H_ */
