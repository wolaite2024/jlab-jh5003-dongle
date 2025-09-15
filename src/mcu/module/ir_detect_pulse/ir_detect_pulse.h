/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      ir_detect_pulse.h
* @brief     This file provides api to use ir detect pulse.
* @details
* @author    colin
* @date      2025-03-27
* @version   v1.0
* *********************************************************************************************************
*/

#ifndef _IR_DETECT_PULSE_H_
#define _IR_DETECT_PULSE_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
/*============================================================================*
 *                         Types
 *============================================================================*/

typedef struct
{
    uint8_t pulse;    //!< The edge of the pulse, with 1 representing the rising edge and 0 representing the falling edge.
    uint32_t time;    //!< Duration of the level, unit: µs.
} IRDetectPulse_TypeDef;

typedef enum
{
    IR_DETECT_FALLING_EDGE = 0,     //!< Configure IR detection on the falling edge.
    IR_DETECT_RISING_EDGE = 1,      //!< Configure IR detection on the rising edge.
    IR_DETECT_BOTH_EDGE = 2,        //!< Configure IR detection on the both edge.
} IRDetectEdgeType_TypeDef;

typedef enum
{
    IR_DETECT_IDLE_LEVEL_LOW = 0,    //!< When the low-level duration exceeds the specified time, it indicates the end of detection.
    IR_DETECT_IDLE_LEVEL_HIGH = 1,   //!< When the high-level duration exceeds the specified time, it indicates the end of detection.
} IRDetectIdleLevel_TypeDef;

typedef void (* pIRDetectCB)(IRDetectPulse_TypeDef *pulse_data,
                             uint32_t len);    //!< IR detect callback type.

/*============================================================================*
 *                         Functions
 *============================================================================*/

/**
 * \brief   Init IR detct pulse.
 *
 * \param[in]  pin_index: The pin index, please refer to pin_def.h 'Pin_Number' part.
 * \param[in]  max_pulse_cnt: The maximum number of pulses in one IR detection.
 * \param[in]  type: The IR detect pulse edge type. \ref IRDetectEdgeType_TypeDef
 * \param[in]  idle_level: The level at the end of IR detection. \ref IRDetectIdleLevel_TypeDef
 * \param[in]  idle_time_us: How long the idle level lasts to indicate the end of detection.
 * \return The result of init IR detect.
 * \retval true: Init IR detect success.
 * \retval false: Init IR detect failed.
 */
bool ir_detect_pulse_init(uint8_t pin_index, uint32_t max_pulse_cnt, IRDetectEdgeType_TypeDef type,
                          IRDetectIdleLevel_TypeDef idle_level, uint32_t idle_time_us);

/**
 * \brief   Start IR detect pulse and DLPS will be disabled.
 */
void ir_detect_pulse_start(void);

/**
 * \brief   Stop IR detect pulse and DLPS will be enabled.
 */
void ir_detect_pulse_stop(void);

/**
 * \brief  Register IR detect callback.
 * \param[in]  cb: IR detect callback. \ref pIRDetectCB
 */
void ir_detect_pulse_register_callback(pIRDetectCB cb);

/**
 * \brief  Unregister IR detect callback.
 */
void ir_detect_pulse_unregister_callback(void);

#ifdef __cplusplus
}
#endif


#endif //_IR_DETECT_PULSE_H_

/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/
