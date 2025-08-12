/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_SDP_A2DP_SRC_H_
#define _APP_SDP_A2DP_SRC_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_SDP App Sdp
  * @brief App Sdp
  * @{
  */

#define RFC_HFP_CHANN_NUM               1
#define RFC_HSP_CHANN_NUM               2
#define RFC_IAP_CHANN_NUM               3
#define RFC_SPP_CHANN_NUM               6
#define RFC_GFPS_CHANN_NUM              7
#define RFC_SPECIFIC2_CHANN_NUM         8
#define RFC_PBAP_CHANN_NUM              10
#define RFC_XM_XIAOAI_CHANN_NUM         11
#define RFC_SPP_AMA_CHANN_NUM           12
#define RFC_HSP_AG_CHANN_NUM            15
#define RFC_HFP_AG_CHANN_NUM            16
#define RFC_ASP_CHANN_NUM               17
#define RFC_PRAHA_DELEGATE_CHANN_NUM    18

/**
    * @brief  sdp module init
    * @param  void
    * @return void
    */
void app_sdp_a2dp_src_init(void);


/** End of APP_SDP
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SDP_H_ */
