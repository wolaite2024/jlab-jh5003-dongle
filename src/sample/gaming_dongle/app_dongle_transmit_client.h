
#ifndef _APP_DONGLE_TRANSMIT_CLIENT_H_
#define _APP_DONGLE_TRANSMIT_CLIENT_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>
#include "app_ctrl_pkt_policy.h"

#define SYNC_WORD 0xAA
#define CMD_LE_DATA_TRANSFER  0x0102
#define CMD_ACK  0x0000
#define EVENT_LEA_GAMING_MODE_SWITCH 0x001D

#define DONGLE_TRANSMIT_DEVICE_NAME_MAX_LENGTH (40)
#define DONGLE_TRANSMIT_DATA_MIN_LEN     (12)
#define DONGLE_TRANSMIT_GAMING_MIN_LEN   (6)
#define DONGLE_TRANSMIT_ACK_LEN          (5)
/**
 * @brief packet type for data transfer
 */

typedef enum
{
    BT_AUDIO_GAMING_MODE = 0x00,
    TRANSMIT_TYPE_END    = 0xff,
} T_TRANSMIT_TYPE;

typedef enum
{
    GAMING_MODE_ENABLE      = 0,
    GAMING_MODE_DISABLE     = 1,
} T_GAMING_MODE;
typedef enum
{
    GAMING_TYPE_LEA         = 0,
    GAMING_TYPE_LEGACY      = 1,
} T_GAMING_TYPE;
typedef enum
{
    DONGLE_PACKET_TYPE_SINGLE                 = 0,
    DONGLE_PACKET_TYPE_FRAGMENT_START         = 1,
    DONGLE_PACKET_TYPE_FRAGMENT_CONTINUE      = 2,
    DONGLE_PACKET_TYPE_FRAGMENT_END           = 3,
} T_DONGLE_PACKET_TYPE;

typedef enum
{
    DONGLE_TRANSMIT_DEVICE_DEFAULT,
    DONGLE_TRANSMIT_DEVICE_DONGLE,
} T_DONGLE_TRANSMIT_DEVICE_TYPE; //The device type seq must be same with realtek headset/speaker

typedef struct
{
    uint8_t  bd_addr[6];
    uint8_t  device_type;
    uint8_t  res;
    uint8_t  device_name[DONGLE_TRANSMIT_DEVICE_NAME_MAX_LENGTH];
} T_DONGLE_TRANSMIT_DEV_INFO;  //The msg struct must be same with realtek headset/speaker

/**
 * @brief app dongle_transmit service client init
 *
 */
void app_dongle_transmit_client_init(void);

/**
 * @brief
 *
 * @param p_data   the pointer of data of dongle_transmit service server's notification/client's read_result.
 * @param data_len the len of data of dongle_transmit service server's notification/client's read_result.
 * @param p_para   the poniter of data in spp format.
 * @param para_len the len of  data in spp format.
 * @return true    parse success.
 * @return false   parse fail.
 */
bool app_dongle_transmit_client_data_parse(uint8_t *p_data, uint16_t data_len, bool *p_mode,
                                           uint16_t *p_event_id, uint8_t **p_para, uint16_t *para_len);

/**
 * @brief
 *
 * @param bud_side  the bud side of rws
 * @param data_len  the len of data without spp header
 * @param p_data    the ponter of data without spp header
 * @return true     dongle_transmit success
 * @return false    dongle_transmit fail
 */
bool app_dongle_transmit_client_start(T_EARBUD_SIDE bud_side, uint16_t data_len, uint8_t *p_data);
bool app_cmd_send_by_le(uint8_t cmd, uint8_t *data, uint16_t len, T_EARBUD_SIDE side);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif

