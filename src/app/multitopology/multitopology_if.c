/**
*****************************************************************************************
*     Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      multitopology_ctrl.c
   * @brief     This file handles Multi-Topology controller (MTC) Interface routines.
   * @author    mj.mengjie.han
   * @date      2022-03-03
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2021 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
*                              Header Files
*============================================================================*/
#include "trace.h"
#include "multitopology_if.h"
/*============================================================================*
 *                              Constants
 *============================================================================*/

/*============================================================================*
 *                              Variables
 *============================================================================*/

/*============================================================================*
 *                              Functions
 *============================================================================*/



/*============================================================================*
 *                              Interface
 *============================================================================*/
T_MTC_RESULT mtc_if_routine(T_MTC_IF_INFO *para);
T_MTC_RESULT mtc_if_routine_reg(uint8_t if_index, P_MTC_IF_CB para);
T_MTC_RESULT mtc_if_fm_ap_handle(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_fm_ml_handle(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_fm_lcis_handle(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
T_MTC_RESULT mtc_if_fm_lbis_handle(T_MTC_IF_MSG msg, void *inbuf, void *outbuf);
/*============================================================================*
 *                              Sub Module
 *============================================================================*/
/*Audio Policy*/
T_MTC_RESULT mtc_if_fm_ap(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO fm_ap_info;
    fm_ap_info.if_index = MTC_IF_FM_AUDIO_POLICY;
    fm_ap_info.msg = msg;
    fm_ap_info.inbuf = inbuf;
    fm_ap_info.outbuf = outbuf;

    return mtc_if_routine(&fm_ap_info);
}

T_MTC_RESULT mtc_if_to_ap(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO to_ap_info;
    to_ap_info.if_index = MTC_IF_TO_AUDIO_POLICY;
    to_ap_info.msg = msg;
    to_ap_info.inbuf = inbuf;
    to_ap_info.outbuf = outbuf;
    return mtc_if_routine(&to_ap_info);
}

T_MTC_RESULT mtc_if_ap_reg(P_MTC_IF_CB para)
{
    T_MTC_RESULT result = MTC_RESULT_SUCCESS;
    //From audio policy
    result =  mtc_if_routine_reg(MTC_IF_FM_AUDIO_POLICY, mtc_if_fm_ap_handle);
    //TO audio policy
    result =  mtc_if_routine_reg(MTC_IF_TO_AUDIO_POLICY, para);
    return result;
}
/*Multi-link*/
T_MTC_RESULT mtc_if_fm_ml(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO fm_ml_info;
    fm_ml_info.if_index = MTC_IF_FM_MULTILINK;
    fm_ml_info.msg = msg;
    fm_ml_info.inbuf = inbuf;
    fm_ml_info.outbuf = outbuf;
    return mtc_if_routine(&fm_ml_info);
}

T_MTC_RESULT mtc_if_to_ml(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO to_ml_info;
    to_ml_info.if_index = MTC_IF_TO_MULTILINK;
    to_ml_info.msg = msg;
    to_ml_info.inbuf = inbuf;
    to_ml_info.outbuf = outbuf;
    return mtc_if_routine(&to_ml_info);
}

T_MTC_RESULT mtc_if_ml_reg(P_MTC_IF_CB para)
{
    T_MTC_RESULT result = MTC_RESULT_SUCCESS;
    //From multi link
    result =  mtc_if_routine_reg(MTC_IF_FM_MULTILINK, mtc_if_fm_ml_handle);
    //TO multi link
    result =  mtc_if_routine_reg(MTC_IF_TO_MULTILINK, para);
    return result;
}

/*LEA CIS*/
T_MTC_RESULT mtc_if_fm_lcis(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO fm_lcis_info;
    fm_lcis_info.if_index = MTC_IF_FM_LEA_CIS;
    fm_lcis_info.msg = msg;
    fm_lcis_info.inbuf = inbuf;
    fm_lcis_info.outbuf = outbuf;
    return mtc_if_routine(&fm_lcis_info);
}

T_MTC_RESULT mtc_if_to_lcis(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO to_lcis_info;
    to_lcis_info.if_index = MTC_IF_TO_LEA_CIS;
    to_lcis_info.msg = msg;
    to_lcis_info.inbuf = inbuf;
    to_lcis_info.outbuf = outbuf;
    return mtc_if_routine(&to_lcis_info);
}

T_MTC_RESULT mtc_if_lcis_reg(P_MTC_IF_CB para)
{
    T_MTC_RESULT result = MTC_RESULT_SUCCESS;
    result =  mtc_if_routine_reg(MTC_IF_FM_LEA_CIS, mtc_if_fm_lcis_handle);
    result =  mtc_if_routine_reg(MTC_IF_TO_LEA_CIS, para);
    return result;
}
/*LEA BIS*/
T_MTC_RESULT mtc_if_fm_lbis(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO fm_lbis_info;
    fm_lbis_info.if_index = MTC_IF_FM_LEA_BIS;
    fm_lbis_info.msg = msg;
    fm_lbis_info.inbuf = inbuf;
    fm_lbis_info.outbuf = outbuf;
    return mtc_if_routine(&fm_lbis_info);
}

T_MTC_RESULT mtc_if_to_lbis(T_MTC_IF_MSG msg, void *inbuf, void *outbuf)
{
    T_MTC_IF_INFO to_lbis_info;
    to_lbis_info.if_index = MTC_IF_TO_LEA_BIS;
    to_lbis_info.msg = msg;
    to_lbis_info.inbuf = inbuf;
    to_lbis_info.outbuf = outbuf;
    return mtc_if_routine(&to_lbis_info);
}

T_MTC_RESULT mtc_if_lbis_reg(P_MTC_IF_CB para)
{
    T_MTC_RESULT result = MTC_RESULT_SUCCESS;
    result =  mtc_if_routine_reg(MTC_IF_FM_LEA_BIS, mtc_if_fm_lbis_handle);
    result =  mtc_if_routine_reg(MTC_IF_TO_LEA_BIS, para);
    return result;
}

