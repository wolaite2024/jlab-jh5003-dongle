/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _MULTIPRO_IF_H_
#define _MULTIPRO_IF_H_


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @brief  MTC Return Result List */
typedef enum
{
    MTC_RESULT_SUCCESS                    = 0x00,
    MTC_RESULT_NOT_REGISTER               = 0x01,
    MTC_RESULT_PENDING                    = 0x02,
    MTC_RESULT_ACCEPT                     = 0x03,
    MTC_RESULT_REJECT                     = 0x04,
    MTC_RESULT_NOT_RELEASE                = 0x05,

} T_MTC_RESULT;

typedef enum
{
    MTC_IF_TO_AUDIO_POLICY              = 0x00,
    MTC_IF_FM_AUDIO_POLICY              = 0x01,
    MTC_IF_TO_MULTILINK                 = 0x02,
    MTC_IF_FM_MULTILINK                 = 0x03,
    MTC_IF_TO_LEA_CIS                   = 0x04,
    MTC_IF_FM_LEA_CIS                   = 0x05,
    MTC_IF_TO_LEA_BIS                   = 0x06,
    MTC_IF_FM_LEA_BIS                   = 0x07,
    MTC_IF_MAX                          = 0x08,
} T_MTC_IF_IDX;

/** @brief  Module List */
typedef enum
{
    MTC_IF_GROUP_AUDIO_POLICY           = 0x0000,
    MTC_IF_GROUP_MULTILINK              = 0x0100,
    MTC_IF_GROUP_LEA_CIS                = 0x0200,
    MTC_IF_GROUP_LEA_BIS                = 0x0300,
} T_MTC_IF_GROUP;

/** @brief  Interface List */
typedef enum
{
    AP_FM_MTC_RESUME_SCO                = MTC_IF_GROUP_AUDIO_POLICY | 0x00,
    AP_FM_MTC_RESUME_A2DP               = MTC_IF_GROUP_AUDIO_POLICY | 0x01,
    AP_FM_MTC_STOP_A2DP                 = MTC_IF_GROUP_AUDIO_POLICY | 0x02,
    AP_TO_MTC_SCO_CMPL                  = MTC_IF_GROUP_AUDIO_POLICY | 0x80,
    AP_TO_MTC_SNIFFING_CHECK            = MTC_IF_GROUP_AUDIO_POLICY | 0x81,
    AP_TO_MTC_SHUTDOWN                  = MTC_IF_GROUP_AUDIO_POLICY | 0x82,

    ML_FM_MTC_LEA_START                 = MTC_IF_GROUP_MULTILINK | 0x00,
    ML_FM_MTC_LEA_STOP                  = MTC_IF_GROUP_MULTILINK | 0x01,
    ML_TO_MTC_CH_SNIFFING               = MTC_IF_GROUP_MULTILINK | 0x80,
    ML_TO_MTC_CH_LEA_CALL               = MTC_IF_GROUP_MULTILINK | 0x81,

    LCIS_FM_MTC_LEA_TRACK               = MTC_IF_GROUP_LEA_CIS | 0x00,
    LCIS_FM_MTC_LEA_PACS_CHANGE         = MTC_IF_GROUP_LEA_CIS | 0x01,

    LCIS_TO_MTC_ASCS_CP_ENABLE          = MTC_IF_GROUP_LEA_CIS | 0x80,
    LCIS_TO_MTC_TER_CIS                 = MTC_IF_GROUP_LEA_CIS | 0x81,

    LBIS_FM_MTC_                        = MTC_IF_GROUP_LEA_BIS | 0x00,
    LBIS_TO_MTC_CHECK_RESUME            = MTC_IF_GROUP_LEA_BIS | 0x80,
} T_MTC_IF_MSG;

typedef struct
{
    uint8_t if_index;
    void *inbuf;
    void *outbuf;
    T_MTC_IF_MSG msg;
} T_MTC_IF_INFO;
typedef T_MTC_RESULT(*P_MTC_IF_CB)(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);

/*============================================================================*
 *                              Functions
 *============================================================================*/
T_MTC_RESULT mtc_if_to_ap(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_fm_ap(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_ap_reg(P_MTC_IF_CB para);
T_MTC_RESULT mtc_if_to_ml(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_fm_ml(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_ml_reg(P_MTC_IF_CB para);
T_MTC_RESULT mtc_if_to_lcis(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_fm_lcis(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_lcis_reg(P_MTC_IF_CB para);
T_MTC_RESULT mtc_if_to_lbis(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_fm_lbis(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_lbis_reg(P_MTC_IF_CB para);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
