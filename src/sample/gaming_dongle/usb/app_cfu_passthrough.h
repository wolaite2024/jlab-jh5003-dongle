#include "stdint.h"
#include "app_msg.h"
#include "stdbool.h"
#ifndef _APP_CFU_PASSTHROUGH_H_
#define _APP_CFU_PASSTHROUGH_H_



typedef bool(*P_CFU_PT_CB)(uint8_t *data, uint16_t length);


void cfu_pt_register_cb(P_CFU_PT_CB cb);

void app_usb_hid_handle_cfu_passthrough_set_report(uint8_t *data, uint16_t length);

uint8_t app_usb_hid_handle_cfu_passthrough_get_report(uint8_t *data, uint16_t *length);

bool app_usb_hid_send_cfu_passthrough(uint8_t *data, uint16_t length);

bool app_cfu_pt_handle_spp_received(uint8_t *data, uint16_t len);

void app_usb_hid_cfu_pt_init(void);
#endif
