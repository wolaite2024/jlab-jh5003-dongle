/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      ancs_sample.h
* @brief     ANCS
* @details   ANCS
* @author    ranhui
* @date      2015-03-27
* @version   v0.1
* *********************************************************************************************************
*/

#ifndef _ANCS_SAMPLE_H__
#define _ANCS_SAMPLE_H__

#ifdef __cplusplus
extern "C" {
#endif
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <gap_le.h>

/** @defgroup PERIPH_ANCS Peripheral ANCS
  * @brief Peripheral ANCS
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup PERIPH_ANCS_Exported_Macros ANCS Exported Macros
   * @{
   */
#define ANCS_MAX_ATTR_LEN 256 //!< Max ANCS attribute length

#if F_BT_ANCS_GET_APP_ATTR
#define ANCS_APP_IDENTIFIER_MAX_LEN 30  //!< Max app identifier length
#endif


/** End of PERIPH_ANCS_Exported_Macros
    * @}
    */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup PERIPH_ANCS_Exported_Types ANCS Exported Types
    * @{
    */

/**  @brief  App parse ANCS notification attribute state */
typedef enum
{
    DS_PARSE_NOT_START = 0x00,
    DS_PARSE_GET_NOTIFICATION_COMMAND_ID = 0x01,
    DS_PARSE_UID1,
    DS_PARSE_UID2,
    DS_PARSE_UID3,
    DS_PARSE_UID4,
    DS_PARSE_ATTRIBUTE_ID,
    DS_PARSE_ATTRIBUTE_LEN1,
    DS_PARSE_ATTRIBUTE_LEN2,
    DS_PARSE_ATTRIBUTE_READY
} T_DS_NOTIFICATION_ATTR_PARSE_STATE;

/**  @brief  App parse ANCS attribute state */
typedef enum
{
    DS_PARSE_GET_APP_COMMAND_ID = 0x10,
    DS_PARSE_APP_IDENTIFIER_START,
    DS_PARSE_APP_IDENTIFIER_END,
    DS_PARSE_APP_ATTRIBUTE_ID,
    DS_PARSE_APP_ATTRIBUTE_LEN1,
    DS_PARSE_APP_ATTRIBUTE_LEN2,
    DS_PARSE_APP_ATTRIBUTE_READY
} T_DS_APP_ATTR_PARSE_STATE;

/**  @brief  Define notification attribute details data */
/**          App can acquire details information by attribute id */
typedef struct
{
    uint8_t    command_id;
    uint8_t    notification_uid[4];
    uint8_t    attribute_id;
    uint16_t   attribute_len;
    uint8_t    data[ANCS_MAX_ATTR_LEN];
} T_DS_NOTIFICATION_ATTR;

/**  @brief  Local app record notification attribute information */
typedef struct
{
    uint8_t    command_id;
    uint8_t    attribute_id;
    uint16_t   attribute_len;
    uint8_t    data[ANCS_MAX_ATTR_LEN];
} T_DS_APP_ATTR;

/** End of PERIPH_ANCS_Exported_Types
    * @}
    */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** @defgroup PERIPH_ANCS_Exported_Functions ANCS Exported Functions
    * @{
    */
/**
 * @brief  App register ANCS client to bluetooth host.
 *
 * @return void
 */
void app_ancs_client_init(void);

/** @} */ /* End of group PERIPH_ANCS_Exported_Functions */
/** @} */ /* End of group PERIPH_ANCS */
#ifdef __cplusplus
}
#endif

#endif

