/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_rcc.h
* @brief     The header file of reset and clock control driver.
* @details
* @author    tifnan_ge
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _RTL876X_RCC_H_
#define _RTL876X_RCC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x.h"



/** @addtogroup 87x3e_RCC RCC
  * @brief RCC driver module.
  * @{
  */



/** @defgroup 87x3e_RCC_Exported_Constants RCC Exported Constants
  * @{
  */


/** @defgroup 87x3e_RCC_Peripheral_Clock  RCC Peripheral Clock
  * @{
  */
/*start  offset address  28| --> bit28, (0x01 << 29) --> adress 0x230,(0x02 << 29) -->0x234, 0x00 << 10) -->0x00 sleep clock cfg?  yes*/
#define APBPeriph_CAP_CLOCK               (0) //!< CAP clock.

#define APBPeriph_I2S0_CLOCK              ((uint32_t)((1 << 5) | (1 << 8) | (0x00 << 29))) //!< I2S0 clock.
#define APBPeriph_I2S1_CLOCK              ((uint32_t)((1 << 6) | (1 << 8) | (0x00 << 29))) //!< I2S1 clock.
#define APBPeriph_CODEC_CLOCK             ((uint32_t)((1 << 4) | (0x00 << 29))) //!< CODEC clock.
#define APBPeriph_USB_CLOCK                 ((uint32_t)( 28 | (0x01 << 29) | 0x00 << 10)) //!< USB clock.
#define APBPeriph_SD_HOST_CLOCK             ((uint32_t)( 26 | (0x01 << 29) | 0x00 << 10)) //!< SD host clock.

#define APBPeriph_GPIOA_CLOCK               ((uint32_t)( 24 | (0x01 << 29) | 0x00 << 10)) //!< GPIOA clock.
#define APBPeriph_GPIO_CLOCK                APBPeriph_GPIOA_CLOCK //!< GPIO clock.
#define APBPeriph_GPIOB_CLOCK               ((uint32_t)( 22 | (0x01 << 29) | 0x00 << 10)) //!< GPIOB clock.
#define APBPeriph_FLASH2_CLOCK              ((uint32_t)( 20 | (0x01 << 29) | 0x00 << 10)) //!< FLASH2 clock.
#define APBPeriph_FLASH1_CLOCK              ((uint32_t)( 18 | (0x01 << 29) | 0x00 << 10)) //!< FLASH1 clock.
#define APBPeriph_GDMA_CLOCK                ((uint32_t)( 16 | (0x01 << 29) | 0x00 << 10)) //!< GDMA clock.
#define APBPeriph_TIMER_CLOCK               ((uint32_t)( 14 | (0x01 << 29) | 0x00 << 10)) //!< Timer clock.
#define APBPeriph_TIMERA_CLOCK              APBPeriph_TIMER_CLOCK //!< Timer clock.
#define APBPeriph_UART1_CLOCK               ((uint32_t)( 12 | (0x01 << 29) | 0x00 << 10)) //!< UART1 clock.
#define APBPeriph_UART2_CLOCK               ((uint32_t)( 10 | (0x01 << 29) | 0x00 << 10)) //!< UART2 clock.
#define APBPeriph_FLASH_CLOCK               ((uint32_t)(  8 | (0x01 << 29) | 0x00 << 10)) //!< FLASH clock.
#define APBPeriph_VENDOR_REG_CLOCK          ((uint32_t)(  6 | (0x01 << 29) | 0x00 << 10)) //!< VENDOR clock.
#define APBPeriph_CKE_BTV_CLOCK             ((uint32_t)(  5 | (0x01 << 29) | 0x01 << 10)) //!< CKE BTV clock.
#define APBPeriph_BUS_RAM_SLP_CLOCK         ((uint32_t)(  4 | (0x01 << 29) | 0x01 << 10)) //!< BUS RAM SLP clock.
#define APBPeriph_CKE_CTRLAP_CLOCK          ((uint32_t)(  3 | (0x01 << 29) | 0x01 << 10)) //!< CKE CTRLAP clock.
#define APBPeriph_CKE_PLFM_CLOCK            ((uint32_t)(  2 | (0x01 << 29) | 0x01 << 10)) //!< CKE PLFM clock.
#define APBPeriph_CKE_CORDIC_CLOCK          ((uint32_t)(  1 | (0x01 << 29) | 0x01 << 10)) //!< CKE CORDIC clock.
#define APBPeriph_HWSPI_CLOCK               ((uint32_t)(  0 | (0x01 << 29) | 0x01 << 10)) //!< HWSPI clock.

#define APBPeriph_SPI2_CLOCK               ((uint32_t)( 22 | (0x02 << 29) | 0x00 << 10)) //!< SPI2 clock.
#define APBPeriph_IR_CLOCK                 ((uint32_t)( 20 | (0x02 << 29) | 0x00 << 10)) //!< IR clock.
#define APBPeriph_SPI1_CLOCK               ((uint32_t)( 18 | (0x02 << 29) | 0x00 << 10)) //!< SPI1 clock.
#define APBPeriph_SPI0_CLOCK               ((uint32_t)( 16 | (0x02 << 29) | 0x00 << 10)) //!< SPI0 clock.
#define APBPeriph_CKE_CAP_CLOCK            ((uint32_t)( 11 | (0x02 << 29) | 0x01 << 10)) //!< CKE_CAP clock.
#define APBPeriph_CKE_AAC_XTAL_CLOCK       ((uint32_t)( 10 | (0x02 << 29) | 0x01 << 10)) //!< CKE AAC XTAL clock.
#define APBPeriph_CKE_PDCK_CLOCK           ((uint32_t)(  9 | (0x02 << 29) | 0x01 << 10)) //!< CKE PDCK clock.
#define APBPeriph_RNG_CLOCK                ((uint32_t)(  8 | (0x02 << 29) | 0x01 << 10)) //!< RNG clock.
#define APBPeriph_SWR_SS_CLOCK             ((uint32_t)(  6 | (0x02 << 29) | 0x01 << 10)) //!< SWR SS clock.
#define APBPeriph_CAL_32K_CLOCK            ((uint32_t)(  5 | (0x02 << 29) | 0x01 << 10)) //!< CAL 32K clock.
#define APBPeriph_CKE_MODEM_CLOCK          ((uint32_t)(  4 | (0x02 << 29) | 0x01 << 10)) //!< CKE MODEM clock.
#define APBPeriph_UART1_HCI_CLOCK          ((uint32_t)(  2 | (0x02 << 29) | 0x00 << 10)) //!< UART1 HCI clock.
#define APBPeriph_UART0_CLOCK              ((uint32_t)(  0 | (0x02 << 29) | 0x00 << 10)) //!< UART0 clock.

#define APBPeriph_EFUSE_CLOCK             ((uint32_t)( 31 | (0x03 << 29) | 0x01 << 10)) //!< EFUSE clock.
#define APBPeriph_CKE_DSP_WDT_CLOCK       ((uint32_t)( 30 | (0x03 << 29) | 0x01 << 10)) //!< CKE DSP WDT clock.

#define APBPeriph_CKE_DSP_CLOCK           ((uint32_t)( 28 | (0x03 << 29) | 0x00 << 10)) //!< CKE DSP clock.
#define APBPeriph_CKE_H2D_D2H             ((uint32_t)( 26 | (0x03 << 29) | 0x00 << 10)) //!< CKE H2D clock.
#define APBPeriph_ADC_CLOCK               ((uint32_t)( 24 | (0x03 << 29) | 0x00 << 10)) //!< ADC clock.
#define APBPeriph_DSP_MEM_CLOCK           ((uint32_t)( 22 | (0x03 << 29) | 0x00 << 10)) //!< DSP MEM clock.
#define APBPeriph_ASRC_CLOCK              ((uint32_t)( 20 | (0x03 << 29) | 0x00 << 10)) //!< ASRC clock.
#define APBPeriph_LCD_CLOCK               ((uint32_t)( 18 | (0x03 << 29) | 0x00 << 10)) //!< LCD clock.
#define APBPeriph_IF8080_CLOCK            ((uint32_t)( 18 | (0x03 << 29) | 0x00 << 10)) //!< IF8080 clock.
#define APBPeriph_SPI2W_CLOCK             ((uint32_t)( 16 | (0x03 << 29) | 0x00 << 10)) //!< SPI2W clock.
#define APBPeriph_SYS_RAM_CLOCK           ((uint32_t)( 14 | (0x03 << 29) | 0x00 << 10)) //!< SYS RAM clock.
#define APBPeriph_I2C2_CLOCK              ((uint32_t)( 12 | (0x03 << 29) | 0x00 << 10)) //!< I2C2 clock.
#define APBPeriph_SIMC_CLOCK              ((uint32_t)( 10 | (0x03 << 29) | 0x00 << 10)) //!< SIMC clock.
#define APBPeriph_AES_CLOCK               ((uint32_t)(  8 | (0x03 << 29) | 0x00 << 10)) //!< AES clock.
#define APBPeriph_KEYSCAN_CLOCK           ((uint32_t)(  6 | (0x03 << 29) | 0x00 << 10)) //!< KEYSCAN clock.
#define APBPeriph_QDEC_CLOCK              ((uint32_t)(  4 | (0x03 << 29) | 0x00 << 10)) //!< QDEC clock.
#define APBPeriph_I2C0_CLOCK              ((uint32_t)(  0 | (0x03 << 29) | 0x00 << 10)) //!< I2C0 clock.
#define APBPeriph_I2C1_CLOCK              ((uint32_t)(  2 | (0x03 << 29) | 0x00 << 10)) //!< I2C1 clock.

#define IS_APB_PERIPH_CLOCK(CLOCK) (((CLOCK) == APBPeriph_USB_CLOCK) || ((CLOCK) == APBPeriph_SD_HOST_CLOCK)\
                                    || ((CLOCK) == APBPeriph_GPIOA_CLOCK)|| ((CLOCK) == APBPeriph_GPIO_CLOCK) \
                                    || ((CLOCK) == APBPeriph_GPIOB_CLOCK)|| ((CLOCK) == APBPeriph_FLASH2_CLOCK)\
                                    || ((CLOCK) == APBPeriph_FLASH1_CLOCK) || ((CLOCK) == APBPeriph_GDMA_CLOCK)\
                                    || ((CLOCK) == APBPeriph_TIMER_CLOCK) || ((CLOCK) == APBPeriph_UART1_CLOCK)\
                                    || ((CLOCK) == APBPeriph_UART2_CLOCK) || ((CLOCK) == APBPeriph_FLASH_CLOCK)\
                                    || ((CLOCK) == APBPeriph_VENDOR_REG_CLOCK) || ((CLOCK) == APBPeriph_CKE_BTV_CLOCK)\
                                    || ((CLOCK) == APBPeriph_BUS_RAM_SLP_CLOCK) || ((CLOCK) == APBPeriph_CKE_CTRLAP_CLOCK)\
                                    || ((CLOCK) == APBPeriph_CKE_PLFM_CLOCK)|| ((CLOCK) == APBPeriph_CKE_CORDIC_CLOCK)\
                                    || ((CLOCK) == APBPeriph_HWSPI_CLOCK)\
                                    || ((CLOCK) == APBPeriph_SPI2_CLOCK)|| ((CLOCK) == APBPeriph_IR_CLOCK)\
                                    || ((CLOCK) == APBPeriph_SPI1_CLOCK)||(CLOCK) == (APBPeriph_SPI0_CLOCK)\
                                    || ((CLOCK) ==APBPeriph_CKE_CAP_CLOCK ) || (CLOCK) == (APBPeriph_CKE_AAC_XTAL_CLOCK)\
                                    || ((CLOCK) == APBPeriph_CKE_PDCK_CLOCK)||((CLOCK) == APBPeriph_RNG_CLOCK)\
                                    || ((CLOCK) == APBPeriph_SWR_SS_CLOCK)|| ((CLOCK) == APBPeriph_CAL_32K_CLOCK)\
                                    || ((CLOCK) == APBPeriph_CKE_MODEM_CLOCK) || ((CLOCK) == APBPeriph_UART1_HCI_CLOCK)\
                                    || ((CLOCK) == APBPeriph_UART0_CLOCK)||((CLOCK) == APBPeriph_EFUSE_CLOCK)\
                                    || ((CLOCK) == APBPeriph_CKE_DSP_WDT_CLOCK) || ((CLOCK) == APBPeriph_CKE_DSP_CLOCK)\
                                    || ((CLOCK) == APBPeriph_CKE_H2D_D2H)) || ((CLOCK) == APBPeriph_ADC_CLOCK)\
|| ((CLOCK) == APBPeriph_DSP_MEM_CLOCK) || ((CLOCK) == APBPeriph_ASRC_CLOCK)\
|| ((CLOCK) == APBPeriph_LCD_CLOCK) || ((CLOCK) == APBPeriph_IF8080_CLOCK)\
|| ((CLOCK) == APBPeriph_SPI2W_CLOCK)\
|| ((CLOCK) == APBPeriph_SYS_RAM_CLOCK) || ((CLOCK) == APBPeriph_I2C2_CLOCK)\
|| ((CLOCK) == APBPeriph_SIMC_CLOCK)||(CLOCK) == (APBPeriph_AES_CLOCK)\
|| ((CLOCK) == APBPeriph_KEYSCAN_CLOCK)||((CLOCK) ==APBPeriph_QDEC_CLOCK )\
|| ((CLOCK) == APBPeriph_I2C1_CLOCK) || ((CLOCK) ==APBPeriph_I2C0_CLOCK )\
|| ((CLOCK) == APBPeriph_CAP_CLOCK )\
) //!< Check if the input parameter is valid.

/** End of group 87x3e_RCC_Peripheral_Clock
  * @}
  */

/** @defgroup 87x3e_APB_Peripheral_Define APB Peripheral Define
  * @{
  */
#define APBPeriph_CAP                   ((uint32_t)( 21 | (0x00 << 26))) //!< CAP peripheral.
#define APBPeriph_AAC_XTAL              ((uint32_t)( 20 | (0x00 << 26))) //!< AAC XTAL peripheral.
#define APBPeriph_SWR_SS                ((uint32_t)( 18 | (0x00 << 26))) //!< SWR SS peripheral.
#define APBPeriph_TIMER                 ((uint32_t)( 16 | (0x00 << 26))) //!< Timer peripheral.
#define APBPeriph_TIMERA                APBPeriph_TIMER                  //!< Timer peripheral.
#define APBPeriph_USB                   ((uint32_t)( 15 | (0x00 << 26))) //!< USB peripheral.
#define APBPeriph_SD_HOST               ((uint32_t)( 14 | (0x00 << 26))) //!< SD host peripheral.
#define APBPeriph_GDMA                  ((uint32_t)( 13 | (0x00 << 26))) //!< GDMA peripheral.
#define APBPeriph_UART1                 ((uint32_t)( 12 | (0x00 << 26))) //!< UART1 peripheral.
#define APBPeriph_FLASH                 ((uint32_t)( 4  | (0x00 << 26))) //!< FLASH peripheral.
#define APBPeriph_FLASH3                ((uint32_t)( 7  | (0x00 << 26))) //!< FLASH3 peripheral.
#define APBPeriph_FLASH2                ((uint32_t)( 6  | (0x00 << 26))) //!< FLASH2 peripheral.
#define APBPeriph_LCD                   ((uint32_t)( 5  | (0x02 << 26))) //!< LCD peripheral.
#define APBPeriph_I2C0                  ((uint32_t)( 16 | (0x02 << 26))) //!< I2C0 peripheral.
#define APBPeriph_FLASH1                ((uint32_t)( 5  | (0x00 << 26))) //!< FLASH1 peripheral.
#define APBPeriph_BTBUS                 ((uint32_t)( 2  | (0x00 << 26))) //!< BTBUS peripheral.


#define APBPeriph_SHA                   ((uint32_t)( 31 | (0x02 << 26))) //!< SHA peripheral.
#define APBPeriph_EFUSE                 ((uint32_t)( 30 | (0x02 << 26))) //!< EFUSE peripheral.
#define APBPeriph_DSP_WDT               ((uint32_t)( 29 | (0x02 << 26))) //!< DSP WDT peripheral.
#define APBPeriph_ASRC                  ((uint32_t)( 28 | (0x02 << 26))) //!< ASRC peripheral.
#define APBPeriph_DSP_MEM               ((uint32_t)( 27 | (0x02 << 26))) //!< DSP MEM peripheral.
#define APBPeriph_DSP_H2D_D2H           ((uint32_t)( 26 | (0x02 << 26))) //!< DSP H2D D2H peripheral.
#define APBPeriph_DSP_CORE              ((uint32_t)( 25 | (0x02 << 26))) //!< DSP CORE peripheral.
#define APBPeriph_SPI2W                 ((uint32_t)( 24 | (0x02 << 26))) //!< SPI2W peripheral.
#define APBPeriph_PSRAM                 ((uint32_t)( 22 | (0x02 << 26))) //!< PSRAM peripheral.
#define APBPeriph_I2C2                  ((uint32_t)( 20 | (0x02 << 26))) //!< I2C2 peripheral.
#define APBPeriph_KEYSCAN               ((uint32_t)( 19 | (0x02 << 26))) //!< KEYSCAN peripheral.
#define APBPeriph_QDEC                  ((uint32_t)( 18 | (0x02 << 26))) //!< QDEC peripheral.
#define APBPeriph_I2C1                  ((uint32_t)( 17 | (0x02 << 26))) //!< I2C1 peripheral.

#define APBPeriph_SPI0                  ((uint32_t)( 8  | (0x02 << 26))) //!< SPI0 peripheral.
#define APBPeriph_SPI1                  ((uint32_t)( 9  | (0x02 << 26))) //!< SPI1 peripheral.
#define APBPeriph_SPI2                  ((uint32_t)( 11 | (0x02 << 26))) //!< SPI2 peripheral.
#define APBPeriph_LCD                   ((uint32_t)( 5  | (0x02 << 26))) //!< LCD peripheral.
#define APBPeriph_IF8080                ((uint32_t)( 5  | (0x02 << 26))) //!< IF8080 peripheral.
#define APBPeriph_UART0                 ((uint32_t)( 0  | (0x02 << 26))) //!< UART0 peripheral.
#define APBPeriph_UART2                 ((uint32_t)( 1  | (0x02 << 26))) //!< UART2 peripheral.
#define APBPeriph_GPIOA                 ((uint32_t)( 8  | (0x03 << 26))) //!< GPIOA peripheral.
#define APBPeriph_GPIO                   APBPeriph_GPIOA                 //!< GPIO peripheral.
#define APBPeriph_GPIOB                 ((uint32_t)( 9  | (0x03 << 26))) //!< GPIOB peripheral.
#define APBPeriph_ADC                   ((uint32_t)( 0  | (0x03 << 26))) //!< ADC peripheral.
#define APBPeriph_IR                    ((uint32_t)( 10 | (0x02 << 26))) //!< IR peripheral.
#define APBPeriph_SIMC                  ((uint32_t)( 4  | (0x02 << 26))) //!< SIMC peripheral.
#define APBPeriph_RNG                   ((uint32_t)( 3  | (0x02 << 26))) //!< RNG peripheral.
#define APBPeriph_AES                   ((uint32_t)( 2  | (0x02 << 26))) //!< AES peripheral.

#define APBPeriph_I2S0                  ((uint32_t)((1 << 1) | (0x00 << 26))) //!< I2S0 peripheral.
#define APBPeriph_I2S1                  ((uint32_t)((1 << 2) | (0x00 << 26))) //!< I2S1 peripheral.
#define APBPeriph_CODEC                 ((uint32_t)((1 << 0) | (0x00 << 26))) //!< CODEC peripheral.
#define APBPeriph_UART1_HCI             (0) //!< UART1 HCI peripheral.
#define APBPeriph_CKE_MODEM             (0) //!< CKE MODEM peripheral.

#define IS_APB_PERIPH(PERIPH) (((PERIPH) == APBPeriph_CAP) || ((PERIPH) == APBPeriph_AAC_XTAL)\
                               || ((PERIPH) == APBPeriph_SWR_SS)\
                               || ((PERIPH) == APBPeriph_TIMER) || ((PERIPH) == APBPeriph_USB)\
                               || ((PERIPH) == APBPeriph_SD_HOST) || ((PERIPH) == APBPeriph_GDMA)\
                               || ((PERIPH) == APBPeriph_UART1) || ((PERIPH) == APBPeriph_FLASH3)\
                               || ((PERIPH) == APBPeriph_FLASH2) || ((PERIPH) == APBPeriph_FLASH1)\
                               || ((PERIPH) == APBPeriph_FLASH)|| ((PERIPH) == APBPeriph_BTBUS)\
                               || ((PERIPH) == APBPeriph_SHA) || ((PERIPH) == APBPeriph_EFUSE)\
                               || ((PERIPH) == APBPeriph_DSP_WDT) || ((PERIPH) == APBPeriph_ASRC)\
                               || ((PERIPH) == APBPeriph_DSP_MEM) || ((PERIPH) == APBPeriph_DSP_H2D_D2H)\
                               || ((PERIPH) == APBPeriph_DSP_CORE) || ((PERIPH) == APBPeriph_SPI2W)\
                               || ((PERIPH) == APBPeriph_PSRAM) || ((PERIPH) == APBPeriph_I2C2)\
                               || ((PERIPH) == APBPeriph_KEYSCAN) || ((PERIPH) == APBPeriph_QDEC)\
                               || ((PERIPH) == APBPeriph_I2C1) || ((PERIPH) == APBPeriph_I2C0)\
                               || ((PERIPH) == APBPeriph_SPI2)|| ((PERIPH) == APBPeriph_IR)\
                               || ((PERIPH) == APBPeriph_SPI1) || ((PERIPH) == APBPeriph_SPI0)\
                               || ((PERIPH) == APBPeriph_LCD) || ((PERIPH) == APBPeriph_SIMC)\
                               || ((PERIPH) == APBPeriph_IF8080)\
                               || ((PERIPH) == APBPeriph_RNG) || ((PERIPH) == APBPeriph_AES)\
                               || ((PERIPH) == APBPeriph_UART2) || ((PERIPH) == APBPeriph_UART0)\
                               || ((PERIPH) == APBPeriph_GPIOB) || ((PERIPH) == APBPeriph_GPIOA)\
                               || ((PERIPH) == APBPeriph_GPIO)|| ((PERIPH) == APBPeriph_ADC)\
                               || ((PERIPH) == APBPeriph_DSP_PERI) || ((PERIPH) == APBPeriph_DSP_DMA)\
                               || (PERIPH == APBPeriph_UART1_HCI) || (PERIPH == APBPeriph_CKE_MODEM)\
                              ) //!< Check if the input parameter is valid.

/** End of group 87x3e_APB_Peripheral_Define
  * @}
  */

/** @defgroup 87x3e_RCC_Clock_Source_Gating  RCC Clock Source Gating
  * @{
  */

#define CLOCK_GATE_20M                         ((uint32_t)(0x01 << 27)) //!< 20M clock source for 2wspi and QDEC.
#define CLOCK_GATE_10M                         ((uint32_t)(0x01 << 28)) //!< 10M clock source for bluewiz.
#define CLOCK_GATE_5M                          ((uint32_t)(0x01 << 29)) //!< 5M clock source for ADC and KeyScan.
#define IS_CLOCK_GATE(CLOCK) (((CLOCK) == CLOCK_GATE_5M) || ((CLOCK) == CLOCK_GATE_20M)\
                              || ((CLOCK) == CLOCK_GATE_10M)) //!< Check if the input parameter is valid.

/** End of group 87x3e_RCC_Clock_Source_Gating
  * @}
  */

/** @defgroup 87x3e_RCC_TIM_Clock_Source RCC TIM Clock Source
  * @{
  */

#define TIM_CLOCK_SOURCE_SYSTEM_CLOCK                              ((uint16_t) BIT7) //!< TIM clock source is set to system clock.
#define TIM_CLOCK_SOURCE_40MHZ                                     ((uint16_t)(BIT5 | BIT7)) //!< TIM clock source is set to 40MHz.
#define TIM_CLOCK_SOURCE_PLL                                       ((uint16_t)(BIT6 | BIT7)) //!< TIM clock source is set to PLL.
#define IS_TIM_CLOCK_SOURCE(HZ)                                    (((HZ) == TIM_CLOCK_SOURCE_SYSTEM_CLOCK) || \
                                                                    ((HZ) == TIM_CLOCK_SOURCE_40MHZ) || ((HZ) == TIM_CLOCK_SOURCE_PLL)) //!< Check if the input parameter is valid.
/** End of group 87x3e_RCC_TIM_Clock_Source
  * @}
  */

/** @defgroup 87x3e_TIM2TO7_CLOCK_DIV TIM2TO7 Clock Divider
  * @{
  */

#define TIM2TO7_CLOCK_DIV_2                    ((uint16_t)0x1) //!< TIM2 to TIM7 clock divider is set to 2.
#define TIM2TO7_CLOCK_DIV_3                    ((uint16_t)0x2) //!< TIM2 to TIM7 clock divider is set to 3.
#define TIM2TO7_CLOCK_DIV_4                    ((uint16_t)0x3) //!< TIM2 to TIM7 clock divider is set to 4.
#define TIM2TO7_CLOCK_DIV_6                    ((uint16_t)0x4) //!< TIM2 to TIM7 clock divider is set to 6.
#define TIM2TO7_CLOCK_DIV_8                    ((uint16_t)0x5) //!< TIM2 to TIM7 clock divider is set to 8.
#define TIM2TO7_CLOCK_DIV_16                   ((uint16_t)0x6) //!< TIM2 to TIM7 clock divider is set to 16.
#define TIM2TO7_CLOCK_DIV_1                    ((uint16_t)0x8) //!< TIM2 to TIM7 clock divider is set to 1.
#define TIM2TO7_TIM_DIV(DIV)              (((DIV) == TIM2TO7_CLOCK_DIV_1) || \
                                           ((DIV) == TIM2TO7_CLOCK_DIV_3) || \
                                           ((DIV) == TIM2TO7_CLOCK_DIV_4) || \
                                           ((DIV) == TIM2TO7_CLOCK_DIV_8) || \
                                           ((DIV) == TIM2TO7_CLOCK_DIV_16)) //!< Check if the input parameter is valid.
/** End of group 87x3e_TIM2TO7_CLOCK_DIV
  * @}
  */
/** @defgroup 87x3e_SPI_Clock_Divider SPI Clock Divider
  * @{
  */

#define SPI_CLOCK_DIV_1                    ((uint16_t)0x0) //!< SPI clock divider is set to 1.
#define SPI_CLOCK_DIV_2                    ((uint16_t)0x1) //!< SPI clock divider is set to 2.
#define SPI_CLOCK_DIV_4                    ((uint16_t)0x2) //!< SPI clock divider is set to 4.
#define SPI_CLOCK_DIV_8                    ((uint16_t)0x3) //!< SPI clock divider is set to 8.
#define SPI_CLOCK_DIV_3_4                  ((uint16_t)0x4) //!< SPI clock divider is set to 3/4.

#define IS_SPI_DIV(DIV)              (((DIV) == SPI_CLOCK_DIV_1) || \
                                      ((DIV) == SPI_CLOCK_DIV_2) || \
                                      ((DIV) == SPI_CLOCK_DIV_4) || \
                                      ((DIV) == SPI_CLOCK_DIV_8) || \
                                      ((DIV) == SPI_CLOCK_DIV_3_4)) //!< Check if the input parameter is valid.
/** End of group 87x3e_SPI_Clock_Divider
  * @}
  */

/** @defgroup 87x3e_I2C_Clock_Divider I2C Clock Divider
  * @{
  */

#define I2C_CLOCK_DIV_1                    ((uint16_t)0x0) //!< I2C clock divider is set to 1.
#define I2C_CLOCK_DIV_2                    ((uint16_t)0x1) //!< I2C clock divider is set to 2.
#define I2C_CLOCK_DIV_4                    ((uint16_t)0x2) //!< I2C clock divider is set to 4.
#define I2C_CLOCK_DIV_8                    ((uint16_t)0x3) //!< I2C clock divider is set to 8.
#define IS_I2C_DIV(DIV)              (((DIV) == I2C_CLOCK_DIV_1) || \
                                      ((DIV) == I2C_CLOCK_DIV_2) || \
                                      ((DIV) == I2C_CLOCK_DIV_4) || \
                                      ((DIV) == I2C_CLOCK_DIV_8)) //!< Check if the input parameter is valid.
/** End of group 87x3e_I2C_Clock_Divider
  * @}
  */

/** @defgroup 87x3e_UART_Clock_Divider UART Clock Divider
  * @{
  */

#define UART_CLOCK_DIV_1                    ((uint16_t)0x0) //!< UART clock divider is set to 1.
#define UART_CLOCK_DIV_2                    ((uint16_t)0x1) //!< UART clock divider is set to 2.
#define UART_CLOCK_DIV_4                    ((uint16_t)0x2) //!< UART clock divider is set to 4.
#define UART_CLOCK_DIV_16                   ((uint16_t)0x3) //!< UART clock divider is set to 16.
#define IS_UART_DIV(DIV)              (((DIV) == UART_CLOCK_DIV_1) || \
                                       ((DIV) == UART_CLOCK_DIV_2) || \
                                       ((DIV) == UART_CLOCK_DIV_4) || \
                                       ((DIV) == UART_CLOCK_DIV_16)) //!< Check if the input parameter is valid.
/** End of group 87x3e_UART_Clock_Divider
  * @}
  */


/** @defgroup 87x3e_RCC_TIM_Clock_Divider TIM Clock Divider
  * @{
  */

#define TIM_CLOCK_DIV_1                    ((uint16_t)0x0) //!< TIM clock divider is set to 1.
#define TIM_CLOCK_DIV_2                    ((uint16_t)0x4) //!< TIM clock divider is set to 2.
#define TIM_CLOCK_DIV_4                    ((uint16_t)0x5) //!< TIM clock divider is set to 4.
#define TIM_CLOCK_DIV_8                    ((uint16_t)0x6) //!< TIM clock divider is set to 8.
#define TIM_CLOCK_DIV_FIX_1MHZ             ((uint16_t)0x7) //!< TIM clock is fixed at 1MHz.
#define IS_TIM_DIV(DIV)              (((DIV) == TIM_CLOCK_DIV_1) || \
                                      ((DIV) == TIM_CLOCK_DIV_2) || \
                                      ((DIV) == TIM_CLOCK_DIV_4) || \
                                      ((DIV) == TIM_CLOCK_DIV_8) || \
                                      ((DIV) == TIM_CLOCK_DIV_FIXED )) //!< Check if the input parameter is valid.
/** End of group 87x3e_RCC_TIM_Clock_Divider
  * @}
  */

/** End of group 87x3e_RCC_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup 87x3e_RCC_Exported_Functions RCC Exported Functions
  * @{
  */

/**
 *
 * \brief  Enable or disable the APB peripheral clock.
 *
 * \param[in] APBPeriph: Specifies the APB peripheral to gates its clock. This parameter can refer to \ref x3e_APB_Peripheral_Define.
 *            This parameter can be one of the following values:
 *            - APBPeriph_TIMERx: The APB peripheral of TIMER, where x can be A.
 *            - APBPeriph_GDMA: The APB peripheral of GDMA.
 *            - APBPeriph_SPI2W: The APB peripheral of SPI2W.
 *            - APBPeriph_KEYSCAN: The APB peripheral of KEYSCAN.
 *            - APBPeriph_QDEC: The APB peripheral of QDEC.
 *            - APBPeriph_I2Cx: The APB peripheral of I2C, where x can be 0 to 2.
 *            - APBPeriph_IR: The APB peripheral of IR.
 *            - APBPeriph_SPIx: The APB peripheral of SPI, where x can be 0 to 2.
 *            - APBPeriph_GPIOx: The APB peripheral of GPIO, where x can be A or B.
 *            - APBPeriph_UARTx: The APB peripheral of UART, where x can be 0 to 2.
 *            - APBPeriph_ADC: The APB peripheral of ADC.
 *            - APBPeriph_CODEC: The APB peripheral of CODEC.
 * \param[in] APBPeriph_Clock: Specifies the APB peripheral clock config. This parameter can refer to \ref x3e_RCC_Peripheral_Clock.
 *            This parameter can be one of the following values(must be the same with APBPeriph):
 *            - APBPeriph_TIMERx_CLOCK: The APB peripheral clock of TIMER, where x can be A.
 *            - APBPeriph_GDMA_CLOCK: The APB peripheral clock of GDMA.
 *            - APBPeriph_SPI2W_CLOCK: The APB peripheral clock of SPI2W.
 *            - APBPeriph_KEYSCAN_CLOCK: The APB peripheral clock of KEYSCAN.
 *            - APBPeriph_QDEC_CLOCK: The APB peripheral clock of QDEC.
 *            - APBPeriph_I2Cx_CLOCK: The APB peripheral clock of I2C, where x can be 0 to 2.
 *            - APBPeriph_IR_CLOCK: The APB peripheral clock of IR.
 *            - APBPeriph_SPIx_CLOCK: The APB peripheral clock of SPI, where x can be 0 to 2.
 *            - APBPeriph_GPIOx_CLOCK: The APB peripheral clock of GPIO, where x can be A or B.
 *            - APBPeriph_UARTx_CLOCK: The APB peripheral clock of UART, where x can be 0 to 2.
 *            - APBPeriph_ADC_CLOCK: The APB peripheral clock of ADC.
 *            - APBPeriph_CODEC_CLOCK: The APB peripheral clock of CODEC.
 * \param[in] NewState: New state of the specified peripheral clock.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified peripheral clock.
 *            - DISABLE: Disable the specified peripheral clock.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_spi_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
 * }
 * \endcode
 */
void RCC_PeriphClockCmd(uint32_t APBPeriph, uint32_t APBPeriph_Clock, FunctionalState NewState);

/**
 *
 * \brief  Enable or disable the specified APB peripheral clock.
 *
 * \param[in] APBPeriph_Clock: Specifies the APB peripheral clock config. This parameter can refer to \ref x3e_RCC_Peripheral_Clock.
 *            This parameter can be one of the following values(must be the same with APBPeriph):
 *            - APBPeriph_TIMERx_CLOCK: The APB peripheral clock of TIMER, where x can be A.
 *            - APBPeriph_GDMA_CLOCK: The APB peripheral clock of GDMA.
 *            - APBPeriph_SPI2W_CLOCK: The APB peripheral clock of SPI2W.
 *            - APBPeriph_KEYSCAN_CLOCK: The APB peripheral clock of KEYSCAN.
 *            - APBPeriph_QDEC_CLOCK: The APB peripheral clock of QDEC.
 *            - APBPeriph_I2Cx_CLOCK: The APB peripheral clock of I2C, where x can be 0 to 2.
 *            - APBPeriph_IR_CLOCK: The APB peripheral clock of IR.
 *            - APBPeriph_SPIx_CLOCK: The APB peripheral clock of SPI, where x can be 0 to 2.
 *            - APBPeriph_GPIOx_CLOCK: The APB peripheral clock of GPIO, where x can be A or B.
 *            - APBPeriph_UARTx_CLOCK: The APB peripheral clock of UART, where x can be 0 to 2.
 *            - APBPeriph_ADC_CLOCK: The APB peripheral clock of ADC.
 *            - APBPeriph_CODEC_CLOCK: The APB peripheral clock of CODEC.
 * \param[in] NewState: New state of the specified APB peripheral clock.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified APB peripheral clock.
 *            - DISABLE: Disable the specified APB peripheral clock.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_spi_init(void)
 * {
 *     RCC_PeriClockConfig(APBPeriph_SPI0_CLOCK, ENABLE);
 * }
 * \endcode
 */
extern void RCC_PeriClockConfig(uint32_t APBPeriph_Clock, FunctionalState NewState);

/**
 *
 * \brief  Enable or disable the APB peripheral function.
 *
 * \param[in] APBPeriph: Specifies the APB peripheral. This parameter can refer to \ref x3e_APB_Peripheral_Define.
 *            This parameter can be one of the following values:
 *            - APBPeriph_TIMERx: The APB peripheral of TIMER, where x can be A.
 *            - APBPeriph_GDMA: The APB peripheral of GDMA.
 *            - APBPeriph_SPI2W: The APB peripheral of SPI2W.
 *            - APBPeriph_KEYSCAN: The APB peripheral of KEYSCAN.
 *            - APBPeriph_QDEC: The APB peripheral of QDEC.
 *            - APBPeriph_I2Cx: The APB peripheral of I2C, where x can be 0 to 2.
 *            - APBPeriph_IR: The APB peripheral of IR.
 *            - APBPeriph_SPIx: The APB peripheral of SPI, where x can be 0 to 2.
 *            - APBPeriph_GPIOx: The APB peripheral of GPIO, where x can be A or B.
 *            - APBPeriph_UARTx: The APB peripheral of UART, where x can be 0 to 2.
 *            - APBPeriph_ADC: The APB peripheral of ADC.
 *            - APBPeriph_CODEC: The APB peripheral of CODEC.
 * \param[in] NewState: New state of the specified peripheral.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified peripheral function.
 *            - DISABLE: Disable the specified peripheral function.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_spi_init(void)
 * {
 *     RCC_PeriFunctionConfig(APBPeriph_SPI0, ENABLE);
 * }
 * \endcode
 */
extern void RCC_PeriFunctionConfig(uint32_t APBPeriph, FunctionalState NewState);

/**
 *
 * \brief     Config the SPI clock divider.
 *
 * \param[in] SPIx: Where x can be 0 to 2 to select the SPI peripheral \ref x3e_SPI_Declaration.
 * \param[in] ClockDiv: Specifies the SPI clock divider \ref x3e_SPI_Clock_Divider.
 *            This parameter can be one of the following values:
 *            - SPI_CLOCK_DIV_x: Where x can be 1, 2, 4, 8, 3/4 to select the specified clock divider.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_spi_init(void)
 * {
 *     RCC_SPIClkDivConfig(SPI0, SPI_CLOCK_DIV_1);
 * }
 * \endcode
 */
extern void RCC_SPIClkDivConfig(SPI_TypeDef *SPIx, uint16_t ClockDiv);

/**
 *
 * \brief  Config the I2C clock divider.
 *
 * \param[in] I2Cx: Where x can be 0 to 2 to select the I2C peripheral \ref x3e_I2C_Declaration.
 * \param[in] ClockDiv: Specifies the I2C clock divider \ref x3e_I2C_Clock_Divider.
 *            This parameter can be one of the following values:
 *            - I2C_CLOCK_DIV_x: Where x can be 1, 2, 4, 8 to select the specified clock divider.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2c_init(void)
 * {
 *     RCC_I2CClkDivConfig(I2C0, I2C_CLOCK_DIV_1);
 * }
 * \endcode
 */
extern void RCC_I2CClkDivConfig(I2C_TypeDef *I2Cx, uint16_t ClockDiv);

/**
 *
 * \brief  Config the UART clock divider.
 *
 * \param[in] UARTx: UART peripheral selected, x can be 0 ~ 2 \ref x3e_UART_Declaration.
 * \param[in] ClockDiv: Specifies the UART clock divider \ref x3e_UART_Clock_Divider.
 *            This parameter can be one of the following values:
 *            - UART_CLOCK_DIV_x: Where x can be 1, 2, 4, 16 to select the specified clock divider.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_i2c_init(void)
 * {
 *     RCC_UARTClkDivConfig(UART0, UART_CLOCK_DIV_1);
 * }
 * \endcode
 */
extern void RCC_UARTClkDivConfig(UART_TypeDef *UARTx, uint16_t ClockDiv);

/**
 *
 * \brief  Config the timer clock divider.
 *
 * \param[in] TIMx: Where x can be 0 to 7 to select the TIMx peripheral \ref x3e_TIM_Declaration.
 * \param[in] ClockDiv: Specifies the timer clock divider \ref x3e_RCC_TIM_Clock_Divider.
 *            This parameter can be one of the following values:
 *            - TIM_CLOCK_DIV_x: Where x can be 1, 2, 4, 8 and FIX_1MHz to select the specified clock divider.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_timer_init(void)
 * {
 *     RCC_TIMClkDivConfig(TIM0, TIM_CLOCK_DIV_1);
 * }
 * \endcode
 */
extern void RCC_TIMClkDivConfig(TIM_TypeDef *TIMx, uint16_t ClockDiv);

/**
 *
 * \brief     Select the specified timer clock divider and source.
 *
 * \param[in] clocklevel: Timer clock divider \ref x3e_TIM_DIV.
 *            This parameter can be one of the following values:
 *            - TIM2TO7_CLOCK_DIV_x: TIM2TO7 clock divider, where x can be 1 ~ 4, 6, 8, 16.
 * \param[in] clocksource: Timer clock source \ref x3e_RCC_TIM_Clock_Source_Config.
 *            This parameter can be one of the following values:
 *            - TIM_CLOCK_SOURCE_SYSTEM_CLOCK: Select timer clock source of system clock.
 *            - TIM_CLOCK_SOURCE_40MHZ: Select timer clock source of 40MHz.
 *            - TIM_CLOCK_SOURCE_PLL: Select timer clock source of PLL.
 * \param[in] NewState: New state of the specified RCC clock source.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the specified RCC clock source.
 *            - DISABLE: Disable the specified RCC clock source.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_xx_init(void)
 * {
 *     RCC_TimSourceConfig(TIM2TO7_CLOCK_DIV_1, TIM_CLOCK_SOURCE_40MHZ, ENABLE);
 * }
 * \endcode
 */
extern void RCC_TimSourceConfig(uint16_t clocklevel, uint16_t clocksource,
                                FunctionalState NewState);
#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_RCC_H_ */

/**End of group 87x3e_RCC_Exported_Functions
  * @}
  */

/**End of group 87x3e_RCC
  * @}
  */




/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/



