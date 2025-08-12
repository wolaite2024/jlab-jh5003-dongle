/*
 * Copyright (c) 2017, Realtek Semiconductor Corporation. All rights reserved.
 */

#ifndef __APP_SYSTEM_CALL__
#define __APP_SYSTEM_CALL__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  esco_record_evt_param
{
    uint16_t instant_times;
    uint16_t rcv_ack_tx_time[4];
    uint16_t no_rcv_ack;
    uint16_t tx_poll_or_null;
    uint16_t rcv_data_rx_time[4];
    uint16_t no_rcv_data;
    uint16_t rx_poll_or_null;
} ESCO_RECORD_EVT_PARAM;

typedef enum
{
    APP_TIMER_SYSTEM_CALL_ACL_PACKET_STATISTICS,
    APP_TIMER_SYSTEM_CALL_ESCO_PACKET_STATISTICS,
} T_APP_SYSTEM_CALL_TIMER;

typedef struct
{
    uint8_t commandId;
    uint8_t payLength;
    uint16_t type;      //0:HFP TX  1:HFP:RX  2: ACL TX
    uint16_t total_num;
    uint16_t acked_num;
    uint16_t no_ack_num;
} APP_SYSTEM_CALL_REPORT_STATISTICS;

/**
 * app_system_call.h
 *
 * \brief    get acl tx and acked packets num.
 * \param[in]  acl_tx_num          acl tx packets num.
 * \param[in]  acl_tx_acked_num    acl tx acked packets num.
 *
 * \return     void.
 *
 * <b>Example usage</b>
   \code{.c}
   void test(void)
   {
       int16_t acl_tx_num = 0;
       int16_t acl_tx_acked_num = 0;

       acl_tx_and_acked_num_get(&acl_tx_num, &acl_tx_acked_num);
   }
   \endcode
 *
 * \ingroup  APP_SYSTEM_CALL
 */
extern void acl_tx_and_acked_num_get(uint16_t *acl_tx_num, uint16_t *acl_tx_acked_num);

/**
 * app_system_call.h
 *
 * \brief    get acl tx and acked packets num.
 * \param[in]  conn_handle         esco connection handle.
 *
 * \return     bool.
 *
 * <b>Example usage</b>
   \code{.c}
   ESCO_RECORD_EVT_PARAM esco_record;

   // BT manager callback function
   static void app_hfp_ag_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
   {
        case BT_EVENT_HFP_AG_SCO_DATA_IND:
            {
                T_APP_BR_LINK *p_link;
                bool ret;

                p_link = app_find_br_link(param->bt_hfp_ag_sco_data_ind.bd_addr);
                if (p_link != NULL)
                {
                    ret = esco_tx_and_rx_record_get(&esco_record, p_link->sco_handle);
                    if(ret)
                    {
                        APP_PRINT_INFO7("app_hfp_ag_bt_cback: instant_times:%d rcv_ack_tx_time[0]:%d rcv_ack_tx_time[1]:%d \
                                        rcv_ack_tx_time[2]:%d rcv_ack_tx_time[3]:%d no_rcv_ack:%d tx_poll_or_null:%d",
                                        esco_record.instant_times, esco_record.rcv_ack_tx_time[0], esco_record.rcv_ack_tx_time[1],
                                        esco_record.rcv_ack_tx_time[2], esco_record.rcv_ack_tx_time[3], esco_record.no_rcv_ack, esco_record.tx_poll_or_null);
                        APP_PRINT_INFO6("app_hfp_ag_bt_cback: rcv_data_rx_time[0]:%d rcv_data_rx_time[1]:%d \
                                        rcv_data_rx_time[2]:%d rcv_data_rx_time[3]:%d no_rcv_data:%d rx_poll_or_null:%d",
                                        esco_record.rcv_data_rx_time[0], esco_record.rcv_data_rx_time[1], esco_record.rcv_data_rx_time[2],
                                        esco_record.rcv_data_rx_time[3], esco_record.no_rcv_data, esco_record.rx_poll_or_null);
                                    }
                    else
                    {
                        memset(&esco_record, 0, sizeof(ESCO_RECORD_EVT_PARAM));
                    }
                }
            }
            break;
   }
   \endcode
 *
 * \ingroup  APP_SYSTEM_CALL
 */
bool esco_tx_and_rx_record_get(ESCO_RECORD_EVT_PARAM *esco_record, int16_t conn_handle);

void app_system_call_init(void);


#ifdef __cplusplus
}
#endif    /*  __cplusplus */
#endif    /*  __APP_SYSTEM_CALL__*/

