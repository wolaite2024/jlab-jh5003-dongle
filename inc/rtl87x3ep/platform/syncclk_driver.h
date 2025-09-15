/**
*********************************************************************************************************
*               Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     syncclk_driver.h
* \brief    The header file of the sync clock (freerun clock) driver.
* \details  This file provides all sync clock firmware functions.
* \author   youyanwu
* \date     2022-10-25
* \version  v1.0
* *********************************************************************************************************
 */
#ifndef _SYNCCLK_DRIVER_H_
#define _SYNCCLK_DRIVER_H_
#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup 87x3ep_PLATFORM_UTILS SYNC_CLOCK
  * @brief Manage the sync clock (freerun clock) functions
  * @{
  */

/*============================================================================*
 *                         Includes
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup SYNCCLK_ID_Types Export SYNCCLK ID
  * @brief
  * @{
  */
typedef enum t_syncclk_id
{
    SYNCCLK_ID_EMPTY,
    SYNCCLK_ID1,
    SYNCCLK_ID2,
    SYNCCLK_ID3,
    SYNCCLK_ID4,
    SYNCCLK_ID5,
    SYNCCLK_ID6,
    SYNCCLK_ID7,
    SYNCCLK_ID8,
    SYNCCLK_ID9,
    SYNCCLK_ID10,
    SYNCCLK_ID_MAX,
} T_SYNCCLK_ID;

/** @defgroup SYNCCLK_Exported_Types Export Param Struct
  * @brief
  * @{
  */

#define CONN_HANDLE_TYPE_FREERUN_CLOCK      (0xFE)

typedef union t_syncclk_latch_info
{
    uint32_t d32[6];
    struct
    {
        uint8_t  conn_role;
        uint8_t  conn_type;
        uint8_t  role;
        uint8_t  net_id;
        uint32_t exp_sync_clock;
    };
} T_SYNCCLK_LATCH_INFO_TypeDef;
/** \} */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/**
 * \defgroup    SYNCCLK_Exported_Functions Peripheral APIs
 * \{
 */
/**
 * \brief   Initializes the SYNCCLK registers to their default values.
 * \xrefitem Added_API_2_14_1_0 "Added Since 2.14.1.0" "Added API"
 *
 * \param   None.
 * \return  None.
 *
 * <b>Example usage</b>
 * \code{.c}
 * bool syncclk_init(void)
 * {
 *     syncclk_drv_init();
 *
 *     return syncclk_drv_timer_start(SYNCCLK_ID4, CONN_HANDLE_TYPE_FREERUN_CLOCK, 0xFF, 1);
 * }
 * \endcode
 */

void syncclk_drv_init(void);

/**
 * \brief   Start the SYNCCLK
 * \xrefitem Added_API_2_14_1_0 "Added Since 2.14.1.0" "Added API"
 *
 * \param[in] timer_id: Selected SYNCCLK timer ID, refer to enum T_SYNCCLK_ID
 * \param[in] conn_type: fixed as CONN_HANDLE_TYPE_FREERUN_CLOCK (0xFE),
 * \          others are reserved for future function
 * \param[in] role: fixed as 0xFF, reserved for future function
 * \param[in] net_id: fixed as 0x01, reserved for future function
 * \return  The state for success(true) or fail(false).
 *
 * <b>Example usage</b>
 * \code{.c}
 * bool syncclk_init(void)
 * {
 *     syncclk_drv_init();
 *
 *     return syncclk_drv_timer_start(SYNCCLK_ID4, CONN_HANDLE_TYPE_FREERUN_CLOCK, 0xFF, 1);
 * }
 * \endcode
 */
bool syncclk_drv_timer_start(T_SYNCCLK_ID timer_id, uint8_t conn_type, uint8_t role,
                             uint8_t net_id);

/**
 * \brief   Stop the SYNCCLK
 * \xrefitem Added_API_2_14_1_0 "Added Since 2.14.1.0" "Added API"
 *
 * \param[in] timer_id: Selected SYNCCLK timer ID, refer to enum T_SYNCCLK_ID
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void syncclk_stop(void)
 * {
 *     syncclk_drv_timer_stop(SYNCCLK_ID4);
 * }
 * \endcode
 */
void syncclk_drv_timer_stop(T_SYNCCLK_ID timer_id);

/**
 * \brief   Get the SYNCCLK counter value
 * \xrefitem Added_API_2_14_1_0 "Added Since 2.14.1.0" "Added API"
 *
 * \param[in] timer_id: Selected SYNCCLK timer ID, refer to enum T_SYNCCLK_ID
 * \return T_SYNCCLK_LATCH_INFO_TypeDef structure of syncclk counter.
 *
 * <b>Example usage</b>
 * \code{.c}
 * T_SYNCCLK_LATCH_INFO_TypeDef syncclk_get(void)
 * {
 *     return synclk_drv_time_get(SYNCCLK_ID4);
 * }
 * \endcode
 */
T_SYNCCLK_LATCH_INFO_TypeDef *synclk_drv_time_get(T_SYNCCLK_ID id);

/** \} */ /*End of group SYNCCLK_Exported_Functions */
/** @} */ /* End of group 87x3ep_PLATFORM_UTILS */
#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYNCCLK_DRIVER_H_ */
