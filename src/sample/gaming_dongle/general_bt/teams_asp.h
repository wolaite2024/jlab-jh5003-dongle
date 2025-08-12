#ifndef _TEAMS_ASP_H_
#define _TEAMS_ASP_H_

#include "stdint.h"
#define MAX_ASP_CACHE      60

typedef void (*TEAMS_ASP_SEND_DATA)(uint8_t *data, uint8_t length);
void teams_asp_tx_handle(uint8_t *data, uint8_t len);
void teams_asp_rx_handle(uint8_t *data, uint16_t len);
void teams_asp_init(TEAMS_ASP_SEND_DATA send_cb);
void teams_asp_deinit(void);
#endif /* _TEAMS_ASP_H_ */
