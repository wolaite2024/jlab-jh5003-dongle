/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#ifndef __ADAPTER_INT_H__
#define __ADAPTER_INT_H__

#include <stdint.h>
#include "ual_types.h"
#include "ual_dev_mgr.h"
#include "ual_adapter.h"
#include "gap_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PAGESCAN_WINDOW_DEF         0x12
#define PAGESCAN_INTERVAL_DEF       0x800
#define PAGE_TIMEOUT_DEF            0x2000
#define SUPVISIONTIMEOUT_DEF        0x0C80
#define INQUIRYSCAN_WINDOW_DEF      0x12
#define INQUIRYSCAN_INTERVAL_DEF    0x1000


#define  BTA_FL_HAS_IRK    (1 << 0)
extern T_ADAPTET_CBACK bt_hal_cbacks;

struct bt_adv_struct
{
    uint8_t      instance;

    uint8_t      pa; /* PA (periodic advertising) or non-PA */
    uint32_t     adv_min_interval;
    uint32_t     adv_max_interval;

    uint8_t      *adv_data_ltv;
    uint16_t     adv_data_len;

    uint8_t      *scan_rsp_ltv;
    uint16_t     scan_rsp_len;
};

struct bt_scan_client
{
    T_UALIST_HEAD list;

    uint8_t  id;
    uint8_t  discovery_type;
    T_SCAN_RESULT_CBACK cback;

    /* filter ? */
};

typedef struct bt_adapter
{
    T_UALIST_HEAD         list; /* future use */

    uint32_t                 flags; /* BTA_FL_XXX */
    uint8_t                  bd_addr[6];
    uint8_t                  le_addr[6];
    T_BLE_BD_TYPE      bd_type;
    uint32_t                 cod;
    /* FIXME: How long is the real name? */
    char                le_name[GAP_DEVICE_NAME_LEN];
    char                legacy_name[GAP_DEVICE_NAME_LEN];
    uint32_t                 discoverable_timeout;

    T_UALIST_HEAD  connect_list;   /* devices to connect */
    T_UALIST_HEAD  device_list; /* bonded devices */

    /****** discovery ******/
    uint8_t      inquiring;   /* the state of low-level inquirer */
    uint8_t      le_scanning; /* the state of low-level le scanning */
    //uint8_t      discovery_type;
    uint8_t      max_devices;
    uint8_t      inquiry_refcnt;
    uint8_t      lescan_refcnt;
    T_UALIST_HEAD scan_clients; /* users who want to scan */
    T_UALIST_HEAD devices; /* general device info */

    /****** advertising ******/
    uint8_t advertising;
    T_UALIST_HEAD adv_list;

    uint8_t      connectable;
    uint8_t      discoverable;

    uint8_t      irk[16];
} T_BT_ADAPTER;

typedef void (*T_UAL_LE_GAP_MSG_CBACK)(uint16_t subtype, T_LE_GAP_MSG gap_msg);
uint8_t ble_get_conn_st(void);
uint8_t ble_get_scan_st(void);
T_BLE_BD_TYPE ble_get_local_bd_type(void);
uint8_t *get_adapter_rpa(void);
void ual_le_register_gap_msg_cb(T_UAL_LE_GAP_MSG_CBACK cback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UAL_ADAPTER_H__ */
