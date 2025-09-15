#ifndef _APP_LEA_AICS_H_
#define _APP_LEA_AICS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define AICS_MICS_SRV_ID 0

/** @defgroup APP_LEA_AICS App LE Audio AICS
  * @brief this file handle AICS profile related process
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_LEA_AICS_Exported_Functions App LE Audio AICS Functions
    * @{
    */

/**
    * @brief  Initialize parameter of AICS
    * @param  No parameter.
    * @return void
    */
void app_lea_aics_init(void);

/** @} */ /* End of group APP_LEA_AICS_Exported_Functions */
/** End of APP_LEA_AICS
* @}
*/

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
