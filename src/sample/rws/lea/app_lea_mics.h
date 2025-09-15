#ifndef _APP_LEA_MICS_H_
#define _APP_LEA_MICS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/** @defgroup APP_LEA_MICS App LE Audio MICS
  * @brief this file handle MICS profile related process
  * @{
  */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_LEA_MICS_Exported_Functions App LE Audio MICS Functions
    * @{
    */

/**
 * @brief  Check microphone controller exist or not
 * @param  No parameter.
 * @return The link of microphone controller
 */
T_APP_LE_LINK *app_lea_mics_find_mc_link(void);

/**
    * @brief  Initialize parameter of MICS
    * @param  No parameter.
    * @return void
    */
void app_lea_mics_init(void);

/** @} */ /* End of group APP_LEA_MICS_Exported_Functions */
/** End of APP_LEA_MICS
* @}
*/

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
