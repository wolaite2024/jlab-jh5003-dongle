#ifndef _PLATFORM_EXT_H_
#define _PLATFORM_EXT_H_

#include "vector_table.h"

/** @defgroup  HAL_PLATFORM_EXT    Platform EXT
    * @brief This file introduces the platform EXT APIs.
    * @{
    */

/*============================================================================*
 *                              Variables
*============================================================================*/
/** @defgroup HAL_PLATFORM_EXT_Exported_Variables Platform EXT Exported Variables
  * @{
  */

#include "vector_table.h"
typedef enum
{

    WATCH_POINT_INDEX0 = 0, // Configure Watchpoint 0
    WATCH_POINT_INDEX1 = 1, // Configure Watchpoint 1
    WATCH_POINT_INDEX2 = 2, // Configure Watchpoint 2
    WATCH_POINT_INDEX3 = 3, // Configure Watchpoint 3
    WATCH_POINT_NUMBER = 4
} T_WATCH_POINT_INDEX;

typedef enum
{
    WATCH_POINT_BYTE  = 0, //Detect 1 byte access
    WATCH_POINT_HALFWORD,  //Detect 2 byte access
    WATCH_POINT_WORD,      //Detect 4 byte access
} T_WATCH_POINT_ACCESS_SIZE;

#if  TARGET_RTL8773DO || TARGET_RTL8773DFL
typedef enum
{
    WATCH_POINT_FUNCTION_DISABLED = 0, // Disabled. Never generates a match.
    WATCH_POINT_FUNCTION_DADDR_R = 5,  // Detect Read access
    WATCH_POINT_FUNCTION_DADDR_W = 6,  // Detect Write access
    WATCH_POINT_FUNCTION_DADDR_RW = 7, // Detect Read or Write access
} T_WATCH_POINT_WATCH_TYPE;
#else
typedef enum
{
    WATCH_POINT_FUNCTION_DISABLED = 0, // Disabled. Never generates a match.
    WATCH_POINT_FUNCTION_DADDR_RW = 4, // Detect Read or Write access
    WATCH_POINT_FUNCTION_DADDR_W = 5,  // Detect Read access
    WATCH_POINT_FUNCTION_DADDR_R = 6,  // Detect Write access
} T_WATCH_POINT_WATCH_TYPE;
#endif


typedef enum
{
    WATCH_POINT_WATCH_SIZE_1BYTE  = 0,
    WATCH_POINT_WATCH_SIZE_2BYTE,
    WATCH_POINT_WATCH_SIZE_4BYTE,
    WATCH_POINT_WATCH_SIZE_8BYTE,
    WATCH_POINT_WATCH_SIZE_16BYTE,
    WATCH_POINT_WATCH_SIZE_32BYTE,
    WATCH_POINT_WATCH_SIZE_64BYTE,
    WATCH_POINT_WATCH_SIZE_128BYTE,
    WATCH_POINT_WATCH_SIZE_256BYTE,
    WATCH_POINT_WATCH_SIZE_512BYTE,
    WATCH_POINT_WATCH_SIZE_1024BYTE,
    WATCH_POINT_WATCH_SIZE_2048BYTE,
    WATCH_POINT_WATCH_SIZE_4096BYTE,
    WATCH_POINT_WATCH_SIZE_8192BYTE,
    WATCH_POINT_WATCH_SIZE_16384BYTE,
    WATCH_POINT_WATCH_SIZE_32768BYTE,
} T_WATCH_POINT_WATCH_SIZE;

/** @} */ /* End of group HAL_PLATFORM_EXT_Exported_Variables */
/*============================================================================*
  *                                Functions
  *============================================================================*/

/** @defgroup HAL_PLATFORM_EXT_Exported_Functions Platform EXT  Exported Functions
    * @brief
    * @{
    */

/**
 * @brief     Update APP defined handlers.
 * @param     pAppVector The vector table address of app.
 * @param     size The size of the vector table.
 * @param     Default_Handler  APP defined hanlders.
 * @return    void
 */
void vector_table_update(IRQ_Fun *pAppVector, uint32_t size, void *Default_Handler);

/**
 * @brief     Enable debug monitor.
 * @note      The debug_monitor_enable must be called before debug_monitor_point_set.
 * @param     void
 * @return    void
 */
void debug_monitor_enable(void);

/**
 * @brief     Disable debug monitor.
 * @param     void
 * @return    void
 */
void debug_monitor_disable(void);

/**
 * @brief     Check if debug monitor is enbale.
 * @param     void
 * @retval    true Debug monitor has been enabled.
 * @retval    false debug monitor is not enabled.
 */
bool debug_monitor_is_enable(void);

/**
 * @brief     Save debug monitor setting before enter dlps.
 * @warning   This API is supported in RTL87x3G and RTL87x3E.
 *            It is NOT supported in RTL87x3D.
 * \xrefitem Added_API_2_13_1_0 "Added Since 2.13.1.0" "Added API
 * @param     void
 * @return    void
 */
void debug_monitor_save_site(void);

/**
 * @brief     Restore debug monitor setting after exist dlps.
 * @warning   This API is supported in RTL87x3G and RTL87x3E.
 *            It is NOT supported in RTL87x3D.
 * \xrefitem Added_API_2_13_1_0 "Added Since 2.13.1.0" "Added API
 * @param     void
 * @return    void
 */
void debug_monitor_restore_site(void);

/**
 * @brief     Check if debug monitor is enbale.
 * @note      The debug_monitor_enable must be called before debug_monitor_point_set.
 * @param     index The index of WatchPoint x @ref T_WATCH_POINT_INDEX.
 * @param     watch_address The address of variable which is being watched.
 * @param     access_size The size of variable which is being watched. @ref T_WATCH_POINT_ACCESS_SIZE.
 * @param     read_write_func The detecting type of Watchpoint @ref T_WATCH_POINT_WATCH_TYPE.
 * @return    void
 */
void debug_monitor_point_set(T_WATCH_POINT_INDEX index, uint32_t watch_address,
                             T_WATCH_POINT_ACCESS_SIZE access_size, T_WATCH_POINT_WATCH_TYPE read_write_func);

/**
 * @brief     Stop memory monitor if the range of address set by memory_monitor_start do not need to be detected.
 * @param     void
 * @return    void
 */
void memory_monitor_stop(void);

/**
 * @brief Memory_monitor_start can specifies start address and end address with restriction.
 * @param watch_address The start address of variable which is being watched.
 * @param watch_size The size of address range which is being watched. @ref T_WATCH_POINT_WATCH_SIZE.
 * @param read_write_func The detecting type of Watchpoint. @ref T_WATCH_POINT_ACCESS_SIZE.
 * @return void
 */
int memory_monitor_start(uint32_t address, T_WATCH_POINT_WATCH_SIZE size,
                         T_WATCH_POINT_WATCH_TYPE read_write_func);

/** @} */ /* End of group HAL_PLATFORM_EXT_Exported_Functions */
/** @} */ /* End of group HAL_PLATFORM_EXT */
#endif
