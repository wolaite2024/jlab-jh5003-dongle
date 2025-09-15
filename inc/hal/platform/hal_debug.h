#ifndef HAL_DEBUG_H
#define HAL_DEBUG_H

#include "platform_ext.h"

/** @defgroup  HAL_DEBUG    HAL Debug
    * @brief HAL debug API.
    * @{
    */

/** @defgroup HAL_DEBUG_Exported_Macros HAL Debug Exported Macros
    * @brief
    * @{
    */
#define F_HAL_DEBUG_TASK_SCHEDULING         1
#define F_HAL_DEBUG_HIT_RATE_PRINT          1
#define F_HAL_DEBUG_HEAP_USAGE_INFO         1
#define F_HAL_DEBUG_PC_SAMPLING             1
#define F_HAL_DEBUG_QUEUE_USAGE             1
#define F_HAL_DEBUG_TASK_TIME_PROPORTION    1
#define F_HAL_DEBUG_HW_TIMER_IRQ            1
#define F_HAL_DEBUG_MALLOC_DEBUG_INFO       0
#define F_HAL_DEBUG_DBG_MONITOR_CB_ENABLE   1
/** End of HAL_DEBUG_Exported_Macros
    * @}
    */
/** @defgroup HAL_Debug_Exported_Functions HAL Debug Exported Functions
    * @brief
    * @{
    */
#ifdef __cplusplus
extern "C" {
#endif

/** @brief This CB is used for registering cb in debug monitor isr. */
typedef void (* HAL_DEBUG_DEBUG_MONITOR_CB)(void);

/**
    * @brief  Register app debug timer callback function.
    * @note   App_init_timer should be called before hal_debug_init.
    * @param  void
    * @return void
    */
void hal_debug_init(void);
/**
    * @brief  Allocate task information record buffer.
    * @param  task_num Specify the max task count to record task information.
    * @return void
    */
void hal_debug_task_schedule_init(uint32_t task_num);
/**
    * @brief  Print task name and task context switch out time.
    * @param  void
    * @return void
    */
void hal_debug_print_task_info(void);
/**
    * @brief Init cache hit and debug timer.
    * @param  period_ms Specify cache hit rate period.
    * @return void
    */
void hal_debug_cache_hit_count_init(uint32_t period_ms);
/**
    * @brief  Trigger raw memory dump.
    * @param  void
    * @return void
    */
void hal_debug_memory_dump(void);
/**
    * @brief  Set queue handler to monitor and init debug timer.
    * @param  period_ms Specify queue usage statistic period.
    * @param  queue_handle1 Specify the monitor message queue handle 1.
    * @param  queue_handle2 Specify the monitor message queue handle 2.
    * @param  queue_handle3 Specify the monitor message queue handle 3.
    * @return void
    */
void hal_debug_msg_queue_usage_monitor(uint32_t period_ms, void *queue_handle1, void *queue_handle2,
                                       void *queue_handle3);
/**
    * @brief  Init hal debug task time statistic proportion environment.
    * @param  period_ms Specify task statistic proportion period.
    * @return void
    */
void hal_debug_task_time_proportion_init(uint32_t period_ms);
/**
    * @brief  Print pc/lr sampling record data.
    * @param  void
    * @return void
    */
void hal_debug_print_pc_sampling(void);
/**
    * @brief  Init hal debug pc sampling environment.
    * @param  num Specify the pc sampling count.
    * @param  period_ms Specify pc sampling period.
    * @return void
    */
void hal_debug_pc_sampling_init(uint8_t num, uint32_t period_ms);
/**
    * @brief  Debug the hw timer timeout time.
    * @param  void
    * @return void
    */
void hal_debug_hw_timer_irq_init(void);

/**
    * @brief  Register cb in debug monitor isr.
    * @param  cb Register in debug monitor isr.
    * @return void
    */
void hal_debug_debug_monitor_cb_register(HAL_DEBUG_DEBUG_MONITOR_CB cb);

/**
    * @brief  Set debug monitor for task stackoverflow
    * @param  index Debug monitor idex.
    * @param  task_handle Handle of task to be detected.
    * @return void
    */
void hal_debug_stackoverflow_debug_monitor_set(T_WATCH_POINT_INDEX index, void *task_handle);


#ifdef __cplusplus
}
#endif
/** @} */ /* End of group HAL_Debug_Exported_Functions */
/** @} */ /* End of group HAL_DEBUG */
#endif
