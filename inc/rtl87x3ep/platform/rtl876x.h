
/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    rtl876x.h
  * @brief   CMSIS Cortex-M4 Peripheral Access Layer Header File for
  *          RTL876X from Realtek Semiconductor.
  * @date    2016.3.3
  * @version v1.0

 * @date     3. March 2015
 *
 * @note     Generated with SVDConv Vx.xxp
 *           from CMSIS SVD File 'RTL876X.xml' Version x.xC,
 *
 * @par      Copyright (c) 2015 Realtek Semiconductor. All Rights Reserved.
 *
 *           The information contained herein is property of Realtek Semiconductor.
 *           Terms and conditions of usage are described in detail in Realtek
 *           SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 *           Licensees are granted free, non-transferable use of the information. NO
 *           WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 *           the file.
 *
 *

  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

#ifndef RTL876X_H
#define RTL876X_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup RTL876X Rtl876x
  * @brief   CMSIS Cortex-M4 peripheral access layer header file for
  *          RTL876X from Realtek Semiconductor
  * @{
  */


/** @defgroup Configuration_of_CMSIS Configuration of CMSIS
  * @brief   Configuration of the cm4 Processor and Core Peripherals
  * @{
  */
/* ----------------Configuration of the cm4 Processor and Core Peripherals---------------- */
#define __CM4_REV                      0x0001U    //!< Core revision r0p1
#define __MPU_PRESENT                  1          //!< MPU present or not
#define __FPU_PRESENT                  1          //!< FPU present
#define __NVIC_PRIO_BITS               3          //!< Number of Bits used for Priority Levels
#define __Vendor_SysTickConfig         0          //!< Set to 1 if different SysTick Config is used
/** @} */ /* End of group Configuration_of_CMSIS */

/*============================================================================*
 *                              Types
*============================================================================*/
/** @defgroup RTL876x_Exported_types RTL876X Exported types
  * @{
  */

//compatible define
#define UART                            UART0

/** brief Interrupt Number Definition */
typedef enum IRQn
{
    /* -------------------  Cortex-M4 Processor Exceptions Numbers  ------------------- */
    NonMaskableInt_IRQn           = -14,      /**< 2 Non Maskable Interrupt */
    HardFault_IRQn                = -13,      /**< 3 HardFault Interrupt */
    MemoryManagement_IRQn         = -12,      /**< 4 Memory Management Interrupt */
    BusFault_IRQn                 = -11,      /**< 5 Bus Fault Interrupt */
    UsageFault_IRQn               = -10,      /**< 6 Usage Fault Interrupt */
    SVCall_IRQn                   =  -5,      /**< 11 SV Call Interrupt */
    DebugMonitor_IRQn             =  -4,      /**< 12 Debug Monitor Interrupt */
    PendSV_IRQn                   =  -2,      /**< 14 Pend SV Interrupt */
    SysTick_IRQn                  =  -1,      /**< 15 System Tick Interrupt */
    /* INT_NUMBER = 64 */
    System_IRQn = 0,
    WDT_IRQn,
    BTMAC_IRQn,
    DSP_IRQn,
    RXI300_IRQn,
    SPI0_IRQn,
    I2C0_IRQn,
    ADC_IRQn,
    SPORT0_RX_IRQn,
    SPORT0_TX_IRQn,
    TIM2_IRQn,
    TIM3_IRQn,
    TIM4_IRQn,
    RTC_IRQn,
    UART0_IRQn,
    UART1_IRQn,
    UART2_IRQn,
    Peripheral_IRQn,
    GPIO_A0_IRQn,
    GPIO_A1_IRQn,
    GPIO_A_2_7_IRQn,
    GPIO_A_8_15_IRQn,
    GPIO_A_16_23_IRQn,
    GPIO_A_24_31_IRQn,
    SPORT1_RX_IRQn,
    SPORT1_TX_IRQn,
    ADP_DET_NVIC_IRQn,
    VBAT_DET_NVIC_IRQn,
    GDMA0_Channel0_IRQn,
    GDMA0_Channel1_IRQn,
    GDMA0_Channel2_IRQn,
    GDMA0_Channel3_IRQn,
    GDMA0_Channel4_IRQn,
    GDMA0_Channel5_IRQn,
    GDMA0_Channel6_IRQn,
    GDMA0_Channel7_IRQn,
    GDMA0_Channel8_IRQn,
    GPIO_B_0_7_IRQn,
    GPIO_B_8_15_IRQn,
    GPIO_B_16_23_IRQn,
    GPIO_B_24_31_IRQn,
    SPI1_IRQn,
    SPI2_IRQn,
    I2C1_IRQn,
    I2C2_IRQn,
    KeyScan_IRQn,
    QDEC_IRQn,
    USB_IRQn,
    USB_ISOC_IRQn,
    SPIC0_IRQn,
    SPIC1_IRQn,
    SPIC2_IRQn,
    SPIC3_IRQn,
    TIM5_IRQn,
    TIM6_IRQn,
    TIM7_IRQn,
    ASRC0_IRQn,
    ASRC1_IRQn,
    I8080_IRQn,
    ISO7816_IRQn,
    SDIO0_IRQn,
    SPORT2_RX_IRQn,
    ANC_IRQn,
    TOUCH_IRQn,
    GDMA0_Channel9_IRQn,
    GDMA0_Channel10_IRQn,
    GDMA0_Channel11_IRQn,
    DISPLAY_IRQn,
    PPE_IRQn,
    IMDC_IRQn,
    SLAVE_PORT_MONITOR_IRQn,
    RTK_Timer0_IRQn,
    RTK_Timer1_IRQn,
    RTK_Timer2_IRQn,
    RTK_Timer3_IRQn,
    CAN_IRQn,
    BTMAC_WRAP_AROUND_IRQn,
    SHA256_IRQn,
    Public_Key_Engine_IRQn,
    SPI_PHY1_INTR_IRQn,

    /* second level interrupt (Peripheral_IRQn), not directly connect to NVIC */
    MFB_DET_L_IRQn,
    PTA_Mailbox_IRQn,
    USB_UTMI_SUSPEND_N_IRQn,
    IR_IRQn,
    TRNG_IRQn,
    PSRAMC_IRQn,
    LPCOMP_IRQn,
    Timer5_Peri_IRQn,
    Timer6_Peri_IRQn,
    Timer7_Peri_IRQn,
    Peri_10_IRQn,
    Peri_11_IRQn,
    VBAT_DET_IRQn,
    ADP_DET_IRQn,
} IRQn_Type, *PIRQn_Type;

#define GPIO0_IRQn              GPIO_A0_IRQn
#define GPIO1_IRQn              GPIO_A1_IRQn
#define GPIO2_IRQn              GPIO_A_2_7_IRQn
#define GPIO3_IRQn              GPIO_A_2_7_IRQn
#define GPIO4_IRQn              GPIO_A_2_7_IRQn
#define GPIO5_IRQn              GPIO_A_2_7_IRQn
#define GPIO6_IRQn              GPIO_A_2_7_IRQn
#define GPIO7_IRQn              GPIO_A_2_7_IRQn
#define GPIO8_IRQn              GPIO_A_8_15_IRQn
#define GPIO9_IRQn              GPIO_A_8_15_IRQn
#define GPIO10_IRQn             GPIO_A_8_15_IRQn
#define GPIO11_IRQn             GPIO_A_8_15_IRQn
#define GPIO12_IRQn             GPIO_A_8_15_IRQn
#define GPIO13_IRQn             GPIO_A_8_15_IRQn
#define GPIO14_IRQn             GPIO_A_8_15_IRQn
#define GPIO15_IRQn             GPIO_A_8_15_IRQn
#define GPIO16_IRQn             GPIO_A_16_23_IRQn
#define GPIO17_IRQn             GPIO_A_16_23_IRQn
#define GPIO18_IRQn             GPIO_A_16_23_IRQn
#define GPIO19_IRQn             GPIO_A_16_23_IRQn
#define GPIO20_IRQn             GPIO_A_16_23_IRQn
#define GPIO21_IRQn             GPIO_A_16_23_IRQn
#define GPIO22_IRQn             GPIO_A_16_23_IRQn
#define GPIO23_IRQn             GPIO_A_16_23_IRQn
#define GPIO24_IRQn             GPIO_A_24_31_IRQn
#define GPIO25_IRQn             GPIO_A_24_31_IRQn
#define GPIO26_IRQn             GPIO_A_24_31_IRQn
#define GPIO27_IRQn             GPIO_A_24_31_IRQn
#define GPIO28_IRQn             GPIO_A_24_31_IRQn
#define GPIO29_IRQn             GPIO_A_24_31_IRQn
#define GPIO30_IRQn             GPIO_A_24_31_IRQn
#define GPIO31_IRQn             GPIO_A_24_31_IRQn

#define  GPIO32_IRQn            GPIO_B_0_7_IRQn
#define  GPIO33_IRQn            GPIO_B_0_7_IRQn
#define  GPIO34_IRQn            GPIO_B_0_7_IRQn
#define  GPIO35_IRQn            GPIO_B_0_7_IRQn
#define  GPIO36_IRQn            GPIO_B_0_7_IRQn
#define  GPIO37_IRQn            GPIO_B_0_7_IRQn
#define  GPIO38_IRQn            GPIO_B_0_7_IRQn
#define  GPIO39_IRQn            GPIO_B_0_7_IRQn
#define  GPIO40_IRQn            GPIO_B_8_15_IRQn
#define  GPIO41_IRQn            GPIO_B_8_15_IRQn
#define  GPIO42_IRQn            GPIO_B_8_15_IRQn
#define  GPIO43_IRQn            GPIO_B_8_15_IRQn
#define  GPIO44_IRQn            GPIO_B_8_15_IRQn
#define  GPIO45_IRQn            GPIO_B_8_15_IRQn
#define  GPIO46_IRQn            GPIO_B_8_15_IRQn
#define  GPIO47_IRQn            GPIO_B_8_15_IRQn
#define  GPIO48_IRQn            GPIO_B_16_23_IRQn
#define  GPIO49_IRQn            GPIO_B_16_23_IRQn
#define  GPIO50_IRQn            GPIO_B_16_23_IRQn
#define  GPIO51_IRQn            GPIO_B_16_23_IRQn
#define  GPIO52_IRQn            GPIO_B_16_23_IRQn
#define  GPIO53_IRQn            GPIO_B_16_23_IRQn
#define  GPIO54_IRQn            GPIO_B_16_23_IRQn
#define  GPIO55_IRQn            GPIO_B_16_23_IRQn
#define  GPIO56_IRQn            GPIO_B_24_31_IRQn
#define  GPIO57_IRQn            GPIO_B_24_31_IRQn
#define  GPIO58_IRQn            GPIO_B_24_31_IRQn
#define  GPIO59_IRQn            GPIO_B_24_31_IRQn
#define  GPIO60_IRQn            GPIO_B_24_31_IRQn
#define  GPIO61_IRQn            GPIO_B_24_31_IRQn
#define  GPIO62_IRQn            GPIO_B_24_31_IRQn
#define  GPIO63_IRQn            GPIO_B_24_31_IRQn

/** @} */ /* End of group RTL876x_Exported_types */

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include "core_cm4.h"                       /* Processor and core peripherals */

/*============================================================================*
 *                              Types
*============================================================================*/
/** @addtogroup RTL876x_Exported_types RTL876X Exported types
  * @{
  */

typedef enum
{
    RESET = 0,
    SET = !RESET
} FlagStatus, ITStatus;

typedef enum
{
    DISABLE = 0,
    ENABLE = !DISABLE
} FunctionalState;

#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))
//typedef enum {ERROR = 0, SUCCESS = !ERROR} ErrorStatus;

/** @} */ /* End of group RTL876x_Exported_types */



/*============================================================================*
 *                              RTL876X Pin Number
*============================================================================*/
/** @defgroup RTL876X_Pin_Number RTL876X Pin Number
  * @{
  */
#define P0_0        0       /**< GPIO0   0x0280*/
#define P0_1        1       /**< GPIO1   */
#define P0_2        2       /**< GPIO2   */
#define P0_3        3       /**< GPIO3   */

#define P3_2        4       /**< GPIO4   */
#define P3_3        5       /**< GPIO5   */
#define P3_4        6       /**< GPIO6   */
#define P3_5        7       /**< GPIOB6  */

#define P1_0        8       /**< GPIO7   */
#define P1_1        9       /**< GPIO8  */
#define P1_2        10      /**< GPIO9   */
#define P1_3        11      /**< GPIO10  */

#define P1_4        12      /**< GPIO11  */
#define P1_5        13      /**< GPIO12  */
#define P1_6        14      /**< GPIO13  */
#define P1_7        15      /**< GPIO14  */

#define P2_0        16      /**< GPIO15  */
#define P2_1        17      /**< GPIO16  */
#define P2_2        18      /**< GPIO17  */
#define P2_3        19      /**< GPIO18  */

#define P2_4        20      /**< GPIO19  */
#define P2_5        21      /**< GPIO20  */
#define P2_6        22      /**< GPIO21  */
#define P2_7        23      /**< GPIO22  */

#define P3_0        24      /**< GPIO23  */
#define P3_1        25      /**< GPIO24  */
#define DAOUT_P     26      /**< DAOUT_P GPIOA30  */
#define DAOUT_N     27      /**< DAOUT_N GPIOA31  */

#define MIC1_P      28      /**< MIC1_P GPIO27  */
#define MIC1_N      29      /**< MIC1_N GPIO28  */
#define MIC2_P      30      /**< MIC2_P GPIO25  */
#define MIC2_N      31      /**< MIC2_N GPIO26  */

#define MICBIAS     32      /**< MICBIAS GPIO29  */
//#define LOUT_P      33      /**< LOUT_P  GPIO30  */
//#define LOUT_N      34      /**< LOUT_N  GPIO31  */
//#define ROUT_P      35      /**< ROUT_P  GPIOB0  *///  0x2A0

//#define ROUT_N      36      /**< ROUT_N GPIOB1  */
//#define MIC3_P      37      /**< MIC3_P GPIOB4  */
//#define MIC3_N      38      /**< MIC3_N GPIOB5  */   //  0x2A4

#define P4_0        40      /**< GPIOB7   */ //offset 0x3B0
#define P4_1        41      /**< GPIOB8   */
#define P4_2        42      /**< GPIOB9   */
#define P4_3        43      /**< GPIOB10  */

#define P4_4        44      /**< GPIOB11  */
#define P4_5        45      /**< GPIOB12  */
#define P4_6        46      /**< GPIOB13  */
#define P4_7        47      /**< GPIOB14  */

#define P5_0        48      /**< GPIOB15   */ //offset 0x3B0
#define P5_1        49      /**< GPIOB16   */
#define P5_2        50      /**< GPIOB17   */
#define P5_3        51      /**< GPIOB18   */

#define P5_4        52      /**< GPIOB0   */
#define P5_5        53      /**< GPIOB1   */
#define P5_6        54      /**< GPIOB2   */
//#define P5_7        55      /**< GPIOB3  ------ */

#define P6_0        56      /**< GPIOA25   */
#define P6_1        57      /**< GPIOA26   */
#define P6_2        58      /**< GPIOA27   */
#define P6_3        59      /**< GPIOA28   */

#define P6_4        60      /**< GPIOA29   */
#define P6_5        61      /**< GPIOA30   */
#define P6_6        62      /**< GPIOA31   */
//#define P6_7      63      /**< GPIO18   */

#define P7_0        64      /**< GPIOB19   */
#define P7_1        65      /**< GPIOB20   */
#define P7_2        66      /**< GPIOB21   */
#define P7_3        67      /**< GPIOB22   */

#define P7_4        68      /**< GPIOB23   */
#define P7_5        69      /**< GPIOB24   */
#define P7_6        70      /**< GPIOB25   */
//#define P7_7        71      /**< GPIO  */

#define P8_0        72      /**< GPIOB26  */
#define P8_1        73      /**< GPIOB27   */
#define P8_2        74      /**< GPIOB28  */
#define P8_3        75      /**< GPIOB29   */

#define P8_4        76      /**< GPIOB30   */
#define P8_5        77      /**< GPIOB31   */
#define P8_6        78      /**< GPIOB19   ++++++++++*/
#define P8_7        79      /**< GPIOB20   ++++++++++*/

#define P9_0        80      /**< GPIOB0   */
#define P9_1        81      /**< GPIOB1   */
#define P9_2        82      /**< GPIOB2   */
#define P9_3        83      /**< GPIOB3   */

#define P9_4        84      /**< GPIOB4   */
#define P9_5        85      /**< GPIOB5   */
#define P10_0     86      /**< GPIO      */
#define P9_6        87      /**< GPIOB6   ++++++++++*/


#define  TOTAL_PIN_NUM                      88
#define  PINMUX_REG1_ST_PIN                P4_0
#define  PINMUX_REG0_NUM                    10
#define  PINMUX_REG1_NUM                    12






#define ADC_0       P0_0    /**< GPIO0  */
#define ADC_1       P0_1    /**< GPIO1  */
#define ADC_2       P0_2    /**< GPIO2  */
#define ADC_3       P0_3    /**< GPIO3  */
#define ADC_7       P3_5    /**< GPIO29 */

/** @} */ /* End of group RTL876X_Pin_Number */


/* ================================================================================ */
/* ================    Peripheral Registers Structures Section     ================ */
/* ================================================================================ */

/** @defgroup RTL876X_Peripheral_Registers_Structures RTL876X Register Structure
  * @{
  */

/* ================================================================================ */
/* ================                      UART                      ================ */
/* ================================================================================ */

/**
  * @brief Universal Asynchronous Receiver/Transmitter, version 1.0. (UART)
  */

typedef struct                                      /*!< UART Structure                                                        */
{
    __IO  uint32_t DLL;                             /*!< 0x00 */
    __IO  uint32_t DLH_INTCR;                       /*!< 0x04 */
    __IO  uint32_t INTID_FCR;                       /*!< 0x08 */
    __IO  uint32_t LCR;                             /*!< 0x0C */
    __IO  uint32_t MCR;                             /*!< 0x10 */
    __I   uint32_t LSR;                             /*!< 0x14 */
    __I   uint32_t MSR;                             /*!< 0x18 */
    __IO  uint32_t SPR;                             /*!< 0x1C */
    __IO  uint32_t STSR;                            /*!< 0x20 */
    __IO  uint32_t RB_THR;                          /*!< 0x24 */
    __IO  uint32_t MISCR;                           /*!< 0x28 may not be seen for client*/
    __IO  uint32_t TXPLSR;                          /*!< 0x2C */
    __IO  uint32_t RSVD0;                           /*!< 0x30 */
    __IO  uint32_t BaudMONR;                        /*!< 0x34 */
    __IO  uint32_t RSVD1;                           /*!< 0x38 */
    __I   uint32_t DBG_UART;                        /*!< 0x3C */
    __IO  uint32_t RX_IDLE_TOCR;                    /*!< 0x40 */
    __IO  uint32_t RX_IDLE_SR;                      /*!< 0x44 */
    __IO  uint32_t RXIDLE_INTCR;                    /*!< 0x48 */
    __I   uint32_t FIFO_LEVEL;                      /*!< 0x4C */
    __IO  uint32_t REG_INT_MASK;                    /*!< offset 0x50,  */
    __IO  uint32_t REG_TXDONE_INT;                  /*!< offset 0x54,  */
    __IO  uint32_t REG_TX_THD_INT;                  /*!< offset 0x58,  */
    __IO  uint32_t REG_UART_CTRL2;                  /*!< offset 0x5C,  */
    __IO  uint32_t REG_RTL_VERSION;                 /*!< offset 0x60,  */
} UART_TypeDef;

typedef enum
{
    BAUD_RATE_1200,
    BAUD_RATE_9600,
    BAUD_RATE_14400,
    BAUD_RATE_19200,
    BAUD_RATE_28800,
    BAUD_RATE_38400,
    BAUD_RATE_57600,
    BAUD_RATE_76800,
    BAUD_RATE_115200,
    BAUD_RATE_128000,
    BAUD_RATE_153600,
    BAUD_RATE_230400,
    BAUD_RATE_460800,
    BAUD_RATE_500000,
    BAUD_RATE_921600,
    BAUD_RATE_1000000,
    BAUD_RATE_1382400,
    BAUD_RATE_1444400,
    BAUD_RATE_1500000,
    BAUD_RATE_1843200,
    BAUD_RATE_2000000,
    BAUD_RATE_3000000,
    BAUD_RATE_4000000,
    BAUD_RATE_6000000
} UartBaudRate_TypeDef;

typedef enum
{
    LOG_CHANNEL_UART0,
    LOG_CHANNEL_UART1,
    LOG_CHANNEL_UART2
} LogChannel_TypeDef;

//typedef enum
//{
//    LOG_CHANNEL_LOG0_UART,
//    LOG_CHANNEL_LOG1_UART,
//    LOG_CHANNEL_DATA_UART
//} LogChannel_TypeDef;

/* ================================================================================ */
/* ================                 2WIRE_SPI                      ================ */
/* ================================================================================ */

/**
  * @brief 2wire spi, mostly used with mouse sensor. (2WIRE_SPI)
  */

typedef struct                                      /*!< 3WIRE_SPI Structure */
{
    __IO  uint32_t RSVD0[12]; // 0x00 -- 0x2C for Q-decode
    __IO  uint32_t CFGR;      // 0x30
    __IO  uint32_t CR;
    __IO  uint32_t INTCR;
    __I   uint32_t SR;
    __IO  uint32_t RD0;       // 0x40
    __IO  uint32_t RD1;
    __IO  uint32_t RD2;
    __IO  uint32_t RD3;
} SPI3WIRE_TypeDef;

/* ================================================================================ */
/* ================                 IR                             ================ */
/* ================================================================================ */

/**
  * @brief IR.
  */

typedef struct                                      /*!< IR Structure  */
{
    __IO  uint32_t CLK_DIV;                 /*!< 0x00 */
    __IO  uint32_t TX_CONFIG;               /*!< 0x04 */
    __IO  uint32_t TX_SR;                   /*!< 0x08 */
    __IO  uint32_t RESERVER;                /*!< 0x0c */
    __IO  uint32_t TX_INT_CLR;              /*!< 0x10 */
    __IO  uint32_t TX_FIFO;                 /*!< 0x14 */
    __IO  uint32_t RX_CONFIG;               /*!< 0x18 */
    __IO  uint32_t RX_SR;                   /*!< 0x1c */
    __IO  uint32_t RX_INT_CLR;              /*!< 0x20 */
    __IO  uint32_t RX_CNT_INT_SEL;          /*!< 0x24 */
    __IO  uint32_t RX_FIFO;                 /*!< 0x28 */
    __IO  uint32_t IR_VERSION;              /*!< 0x2c */
    __IO  uint32_t REVD[4];
    __IO  uint32_t TX_TIME;                 /*!< 0x40 */
    __IO  uint32_t RX_TIME;                 /*!< 0x44 */
    __IO  uint32_t IR_DBG;                  /*!< 0x48 */
    __IO  uint32_t RSVD2;                   /*!< 0x4C */
    __IO  uint32_t DMA_CONFIG;              /*!< 0x50 */
} IR_TypeDef;

/* ================================================================================ */
/* ================                       SPI                      ================ */
/* ================================================================================ */

/**
  * @brief SPI master 0. (SPI)
  */
typedef struct                              /*!< SPI Structure  */
{
    __IO  uint32_t    CTRLR0;               /*!< 0x00 */
    __IO  uint32_t    CTRLR1;               /*!< 0x04 */
    __IO  uint32_t    SSIENR;               /*!< 0x08 */
    __IO  uint32_t    RSVD_0C;              /*!< 0x0C */
    __IO  uint32_t    SER;                  /*!< 0x10 */
    __IO  uint32_t    BAUDR;                /*!< 0x14 */
    __IO  uint32_t    TXFTLR;               /*!< 0x18 */
    __IO  uint32_t    RXFTLR;               /*!< 0x1C */
    __I  uint32_t     TXFLR;                /*!< 0x20 */
    __I  uint32_t     RXFLR;                /*!< 0x24 */
    __I  uint32_t     SR;                   /*!< 0x28 */
    __IO  uint32_t    IMR;                  /*!< 0x2C */
    __I  uint32_t     ISR;                  /*!< 0x30 */
    __I  uint32_t     RISR;                 /*!< 0x34 */
    __I  uint32_t     TXOICR;               /*!< 0x38 */
    __I  uint32_t     RXOICR;               /*!< 0x3C */
    __I  uint32_t     RXUICR;               /*!< 0x40 */
    __I  uint32_t     FAEICR;               /*!< 0x44 */
    __I  uint32_t     ICR;                  /*!< 0x48 */
    __IO  uint32_t    DMACR;                /*!< 0x4C */
    __IO  uint32_t    DMATDLR;              /*!< 0x50 */
    __IO  uint32_t    DMARDLR;              /*!< 0x54 */
    __I  uint32_t     TXUICR;               /*!< 0x58 */
    __I  uint32_t     SSRICR;               /*!< 0x5C */
    __IO  uint32_t    DR[36];               /*!< 0x60 - 0xEC */
    __IO  uint32_t    RX_SAMPLE_DLY;        /*!< 0xF0 */
} SPI_TypeDef;

/* ================================================================================ */
/* ================                      SD host Interface                    ================= */
/* ================================================================================ */

typedef struct
{
    __IO uint32_t SDMA_SYS_ADDR;            /*!< 0x00 */
    __IO uint16_t BLK_SIZE;                 /*!< 0x04 */
    __IO uint16_t BLK_CNT;                  /*!< 0x06 */
    __IO uint32_t ARG;                      /*!< 0x08 */
    __IO uint16_t TF_MODE;                  /*!< 0x0C */
    __IO uint16_t CMD;                      /*!< 0x0E */
    __I  uint32_t RSP0;                     /*!< 0x10 */
    __I  uint32_t RSP2;                     /*!< 0x14 */
    __I  uint32_t RSP4;                     /*!< 0x18 */
    __I  uint32_t RSP6;                     /*!< 0x1C */
    __IO uint32_t BUF_DATA_PORT;            /*!< 0x20 */
    __I  uint32_t PRESENT_STATE;            /*!< 0x24 */
    __IO uint8_t  HOST_CTRL;                /*!< 0x28 */
    __IO uint8_t  PWR_CTRL;                 /*!< 0x29 */
    __IO uint8_t  BLK_GAP_CTRL;             /*!< 0x2A */
    __IO uint8_t  WAKEUP_CTRL;              /*!< 0x2B */
    __IO uint16_t CLK_CTRL;                 /*!< 0x2C */
    __IO uint8_t  TIMEOUT_CTRL;             /*!< 0x2E */
    __IO uint8_t  SW_RESET;                 /*!< 0x2F */
    __IO uint16_t NORMAL_INTR_SR;           /*!< 0x30 */
    __IO uint16_t ERR_INTR_SR;              /*!< 0x32 */
    __IO uint16_t NORMAL_INTR_SR_EN;        /*!< 0x34 */
    __IO uint16_t ERR_INTR_SR_EN;           /*!< 0x36 */
    __IO uint16_t NORMAL_INTR_SIG_EN;       /*!< 0x38 */
    __IO uint16_t ERR_INTR_SIG_EN;          /*!< 0x3A */
    __I  uint16_t CMD12_ERR_SR;             /*!< 0x3C */
    __I  uint16_t RSVD0;                    /*!< 0x3E */
    __IO uint32_t CAPABILITIES_L;           /*!< 0x40 */
    __IO uint32_t CAPABILITIES_H;           /*!< 0x44 */
    __I  uint32_t RSVD1[4];                 /*!< 0x48 */
    __IO uint32_t ADMA_SYS_ADDR_L;          /*!< 0x58 */
    __IO uint32_t ADMA_SYS_ADDR_H;          /*!< 0x5C */
} SDIO_TypeDef;

/* ================================================================================ */
/* ================                      I2C                      ================= */
/* ================================================================================ */

/**
  * @brief I2C
  */
typedef struct
{
    __IO uint32_t IC_CON;                   /*!< 0x00 */
    __IO uint32_t IC_TAR;                   /*!< 0x04 */
    __IO uint32_t IC_SAR;                   /*!< 0x08 */
    __IO uint32_t IC_HS_MADDR;              /*!< 0x0C */
    __IO uint32_t IC_DATA_CMD;              /*!< 0x10 */
    __IO uint32_t IC_SS_SCL_HCNT;           /*!< 0x14 */
    __IO uint32_t IC_SS_SCL_LCNT;           /*!< 0x18 */
    __IO uint32_t IC_FS_SCL_HCNT;           /*!< 0x1C */
    __IO uint32_t IC_FS_SCL_LCNT;           /*!< 0x20 */
    __IO uint32_t IC_HS_SCL_HCNT;           /*!< 0x24 */
    __IO uint32_t IC_HS_SCL_LCNT;           /*!< 0x28 */
    __I uint32_t IC_INTR_STAT;              /*!< 0x2C */
    __IO uint32_t IC_INTR_MASK;             /*!< 0x30 */
    __I uint32_t IC_RAW_INTR_STAT;          /*!< 0x34 */
    __IO uint32_t IC_RX_TL;                 /*!< 0x38 */
    __IO uint32_t IC_TX_TL;                 /*!< 0x3C */
    __I uint32_t IC_CLR_INTR;               /*!< 0x40 */
    __I uint32_t IC_CLR_RX_UNDER;           /*!< 0x44 */
    __I uint32_t IC_CLR_RX_OVER;            /*!< 0x48 */
    __I uint32_t IC_CLR_TX_OVER;            /*!< 0x4C */
    __I uint32_t IC_CLR_RD_REQ;             /*!< 0x50 */
    __I uint32_t IC_CLR_TX_ABRT;            /*!< 0x54 */
    __I uint32_t IC_CLR_RX_DONE;            /*!< 0x58 */
    __I uint32_t IC_CLR_ACTIVITY;           /*!< 0x5C */
    __I uint32_t IC_CLR_STOP_DET;           /*!< 0x60 */
    __I uint32_t IC_CLR_START_DET;          /*!< 0x64 */
    __I uint32_t IC_CLR_GEN_CALL;           /*!< 0x68 */
    __IO uint32_t IC_ENABLE;                /*!< 0x6C */
    __I uint32_t IC_STATUS;                 /*!< 0x70 */
    __I uint32_t IC_TXFLR;                  /*!< 0x74 */
    __I uint32_t IC_RXFLR;                  /*!< 0x78 */
    __IO uint32_t IC_SDA_HOLD;              /*!< 0x7C */
    __I uint32_t IC_TX_ABRT_SOURCE;         /*!< 0x80 */
    __IO uint32_t IC_SLV_DATA_NACK_ONLY;    /*!< 0x84 */
    __IO uint32_t IC_DMA_CR;                /*!< 0x88 */
    __IO uint32_t IC_DMA_TDLR;              /*!< 0x8C */
    __IO uint32_t IC_DMA_RDLR;              /*!< 0x90 */
    __IO uint32_t IC_SDA_SETUP;             /*!< 0x94 */
    __IO uint32_t IC_ACK_GENERAL_CALL;      /*!< 0x98 */
    __IO uint32_t IC_ENABLE_STATUS;         /*!< 0x9C */
    __IO uint32_t IC_FS_SPKLEN;           /*!< offset 0xA0, I2C SS and FS Spike Suppression
                                                            Limit Register */
} I2C_TypeDef;

/* ================================================================================ */
/* ================                       ADC                      ================ */
/* ================================================================================ */

/**
  * @brief Analog to digital converter. (ADC)
  */

typedef struct                              /*!< ADC Structure */
{
    __O  uint32_t FIFO;                     //!<0x00
    __IO uint32_t CR;                       //!<0x04
    __IO uint32_t SCHCR;                    //!<0x08
    __IO uint32_t INTCR;                    //!<0x0C
    __IO uint32_t SCHTAB0;                  //!<0x10
    __IO uint32_t SCHTAB1;                  //!<0x14
    __IO uint32_t SCHTAB2;                  //!<0x18
    __IO uint32_t SCHTAB3;                  //!<0x1C
    __IO uint32_t SCHTAB4;                  //!<0x20
    __IO uint32_t SCHTAB5;                  //!<0x24
    __IO uint32_t SCHTAB6;                  //!<0x28
    __IO uint32_t SCHTAB7;                  //!<0x2C
    __IO uint32_t SCHD0;                    //!<0x30
    __IO uint32_t SCHD1;                    //!<0x34
    __IO uint32_t SCHD2;                    //!<0x38
    __IO uint32_t SCHD3;                    //!<0x3C
    __IO uint32_t SCHD4;                    //!<0x40
    __IO uint32_t SCHD5;                    //!<0x44
    __IO uint32_t SCHD6;                    //!<0x48
    __IO uint32_t SCHD7;                    //!<0x4C
    __IO uint32_t PWRDLY;                   //!<0x50
    __IO uint32_t DATCLK;                   //!<0x54
    __IO uint32_t ANACTL;                   //!<0x58
    __IO uint32_t REG_5C_CLK;             //!<0x5C
    __IO uint32_t RTL_VER;                  //!<0x60
} ADC_TypeDef;

/* ================================================================================ */
/* ================                      TIM                      ================ */
/* ================================================================================ */

/**
  * @brief TIM
  */
typedef struct
{
    __IO uint32_t LoadCount;                /*!< 0x00*/
    __I  uint32_t CurrentValue;             /*!< 0x04*/
    __IO uint32_t ControlReg;               /*!< 0x08*/
    __I  uint32_t EOI;                      /*!< 0x0C*/
    __I  uint32_t IntStatus;                /*!< 0x10*/
} TIM_TypeDef;

typedef struct
{
    __I uint32_t TimersIntStatus;        /*!< 0xa0, Timers Interrupt Status Register */
    __I uint32_t TimersEOI;              /*!< 0xa4, Timers End-of-Interrupt Register */
    __I uint32_t TimersRawIntStatus;     /*!< 0xa8, Timers Raw Interrupt Status Register */
} TIM_ChannelsTypeDef;

/* ================================================================================ */
/* ================                   RTK   TIM                      ================ */
/* ================================================================================ */

typedef struct
{
    __I  uint32_t
    REG_ENH_TIMER_x_CUR_CNT;     /*!< 0x00,0x50,0xA0 ,0xF0 , Timer 0~3 channel 0~3 current count value */
    __IO uint32_t REG_ENH_TIMER_x_MODE_CFG ;   /*!< 0x04,  */
    __IO uint32_t REG_ENH_TIMER_x_MAX_CNT;     /*!< 0x08,  */
    __I  uint32_t REG_ENH_TIMER_x_CCR;         /*!< 0x0C,  RESERVED*/
    __I  uint32_t REG_ENH_TIMER_x_CCR_FIFO ;   /*!< 0x10,  RESERVED*/
    __I  uint32_t REG_ENH_TIMER_x_PWM_CFG;     /*!< 0x14,  RESERVED*/
    __I  uint32_t REG_ENH_TIMER_x_PWM_SHIFT_CNT; /*!< 0x18,  RESERVED*/
    __I  uint32_t REG_ENH_TIMER_x_LAT_CNT[9];    /*!< 0x1C ~ 0x3C,  RESERVED*/
    __I  uint32_t REG_ENH_TIMER_x_DMA_CFG;       /*!< 0x40,  RESERVED*/
    __I  uint32_t REG_ENH_TIMER_x_DMY[3];        /*!< 0x44 ~0x4C, RESERVED */
} ENH_TIM_TypeDef;

typedef struct
{
    __IO uint32_t REG_ENH_TIMER_EN_CTRL;         /*!< 0xA00, Enhtimer enable control register */
    __IO uint32_t REG_ENH_TIMER_ONESHOT_GO_CTRL; /*!< 0xA04, Enhtimer one-shot go control register */
    __I  uint32_t REG_ENH_TIMER_PWM_STATE ;       /*!< 0xA08, RESERVED */
    __IO uint32_t
    REG_ENH_TIMER_INTR_EN_CTRL;    /*!< 0xA0C, (Enhtimer interrupt enable control register) */
    __IO uint32_t REG_ENH_TIMER_INTR_STS;        /*!< 0xA10, (Enhtimer interrupt status register)*/
    __I  uint32_t REG_ENH_TIMER_INTR;            /*!< 0xA14, (Enhtimer interrupt register)*/
    __I  uint32_t REG_ENH_TIMER_CCR_FIFO_EMPTY_STS;/*!< 0xA18,  RESERVED */
    __I  uint32_t REG_ENH_TIMER_CCR_FIFO_FULL_STS; /*!< 0xA1C, RESERVED */
    __I  uint32_t REG_ENH_TIMER_LAT_CNT[15];        /*!< 0xA20 ~0xA58  RESERVED*/

    __I  uint32_t REG_ENH_TIMER_IP_VER
    ;                       /*!< 0xA5C, (Enhtimer IP version register) */
} ENH_TIM_CTRL_TypeDef;

/* ================================================================================ */
/* ================                      GDMA                      ================ */
/* ================================================================================ */
/**
  * @brief GDMA interrupt registers
  */
typedef struct
{
    __I uint32_t RAW_TFR;                   /*!< offset 0x2C0*/
    uint32_t RSVD0;
    __I uint32_t RAW_BLOCK;                 /*!< offset 0x2C8*/
    uint32_t RSVD1;
    __I uint32_t RAW_SRC_TRAN;              /*!< offset 0x2D0*/
    uint32_t RSVD2;
    __I uint32_t RAW_DST_TRAN;              /*!< offset 0x2D8*/
    uint32_t RSVD3;
    __I uint32_t RAW_ERR;                   /*!< offset 0x2E0*/
    uint32_t RSVD4;

    __I uint32_t STATUS_TFR;                /*!< offset 0x2E8*/
    uint32_t RSVD5;
    __I uint32_t STATUS_BLOCK;              /*!< offset 0x2F0*/
    uint32_t RSVD6;
    __I uint32_t STATUS_SRC_TRAN;           /*!< offset 0x2F8*/
    uint32_t RSVD7;
    __I uint32_t STATUS_DST_TRAN;           /*!< offset 0x300*/
    uint32_t RSVD8;
    __I uint32_t STATUS_ERR;                /*!< offset 0x308*/
    uint32_t RSVD9;

    __IO uint32_t MASK_TFR;                 /*!< offset 0x310*/
    uint32_t RSVD10;
    __IO uint32_t MASK_BLOCK;               /*!< offset 0x318*/
    uint32_t RSVD11;
    __IO uint32_t MASK_SRC_TRAN;            /*!< offset 0x320*/
    uint32_t RSVD12;
    __IO uint32_t MASK_DST_TRAN;            /*!< offset 0x328*/
    uint32_t RSVD13;
    __IO uint32_t MASK_ERR;                 /*!< offset 0x330*/
    uint32_t RSVD14;

    __O uint32_t CLEAR_TFR;                 /*!< offset 0x338*/
    uint32_t RSVD15;
    __O uint32_t CLEAR_BLOCK;               /*!< offset 0x340*/
    uint32_t RSVD16;
    __O uint32_t CLEAR_SRC_TRAN;            /*!< offset 0x348*/
    uint32_t RSVD17;
    __O uint32_t CLEAR_DST_TRAN;            /*!< offset 0x350*/
    uint32_t RSVD18;
    __O uint32_t CLEAR_ERR;                 /*!< offset 0x358*/
    uint32_t RSVD19;
    __O uint32_t StatusInt;                 /*!< offset 0x360, reserved in RTK DMAC IP*/
    uint32_t RSVD191;

    __IO uint32_t ReqSrcReg;                /*!< offset 0x368, reserved in RTK DMAC IP*/
    uint32_t RSVD20;
    __IO uint32_t ReqDstReg;                /*!< offset 0x370, reserved in RTK DMAC IP*/
    uint32_t RSVD21;
    __IO uint32_t SglReqSrcReg;             /*!< offset 0x378, reserved in RTK DMAC IP*/
    uint32_t RSVD22;
    __IO uint32_t SglReqDstReg;             /*!< offset 0x380, reserved in RTK DMAC IP*/
    uint32_t RSVD23;
    __IO uint32_t LstSrcReg;                /*!< offset 0x388, reserved in RTK DMAC IP*/
    uint32_t RSVD24;
    __IO uint32_t LstDstReg;                /*!< offset 0x390, reserved in RTK DMAC IP*/
    uint32_t RSVD25;

    __IO uint32_t DmaCfgReg;                /*!< offset 0x398 */
    uint32_t RSVD26;
    __IO uint32_t ChEnReg;                  /*!< offset 0x3A0 */
    uint32_t RSVD27;
    __I uint32_t DmaIdReg;                  /*!< offset 0x3A8 */
    uint32_t RSVD28;
    __IO uint32_t DmaTestReg;               /*!< offset 0x3B0 */
    uint32_t RSVD29;
} GDMA_TypeDef;

typedef struct
{
    __IO uint32_t SAR;                  /*!< offset 0x00*/
    uint32_t RSVD0;
    __IO uint32_t DAR;                  /*!< offset 0x08*/
    uint32_t RSVD1;
    __IO uint32_t LLP;                  /*!< offset 0x10*/
    uint32_t RSVD2;
    __IO uint32_t CTL_LOW;              /*!< offset 0x18*/
    __IO uint32_t CTL_HIGH;             /*!< offset 0x1C*/
    __IO uint32_t SSTAT;                /*!< offset 0x20*/
    uint32_t RSVD4;
    __IO uint32_t DSTAT;                /*!< offset 0x28*/
    uint32_t RSVD5;
    __IO uint32_t SSTATAR;              /*!< offset 0x30*/
    uint32_t RSVD6;
    __IO uint32_t DSTATAR;              /*!< offset 0x38*/
    uint32_t RSVD7;
    __IO uint32_t CFG_LOW;              /*!< offset 0x40*/
    __IO uint32_t CFG_HIGH;
    __IO uint32_t SGR_LOW;                  /*!< offset 0x48*/
    __IO uint32_t SGR_HIGH;
    __IO uint32_t DSR_LOW;                  /*!< offset 0x50*/
    __IO uint32_t DSR_HIGH;
} GDMA_ChannelTypeDef;

/* ================================================================================ */
/* ================                     I8080                      ================ */
/* ================================================================================ */

/**
  * @brief IF8080
  */
typedef struct
{
    __IO uint32_t CTRL0;            /*!< 0x00 */
    __IO uint32_t CTRL1;            /*!< 0x04 */
    __IO uint32_t IMR;              /*!< 0x08 */
    __I  uint32_t SR;               /*!< 0x0C */
    __O  uint32_t ICR;              /*!< 0x10 */
    __IO uint32_t CFG;              /*!< 0x14 */
    __IO uint32_t FIFO;             /*!< 0x18 */
    __I  uint32_t RESV0;            /*!< 0x1C */
    __I  uint32_t RXDATA;           /*!< 0x20 */
    __IO uint32_t TX_LEN;           /*!< 0x24 */
    __I  uint32_t TX_CNT;           /*!< 0x28 */
    __I  uint32_t RESV1;            /*!< 0x2C */
    __IO uint32_t RX_LEN;           /*!< 0x30 */
    __I  uint32_t RX_CNT;           /*!< 0x34 */
    __IO uint32_t CMD1;             /*!< 0x38 */
    __IO uint32_t CMD2;             /*!< 0x3C */
    __IO uint32_t CMD3;             /*!< 0x40 */
} IF8080_TypeDef;

typedef struct
{
    __IO uint32_t SAR;
    __IO uint32_t DAR;
    __IO uint32_t LLP;
    __IO uint32_t CTL_LOW;
    __IO uint32_t CTL_HIGH;
} IF8080_GDMALLITypeDef;

typedef struct
{
    __IO uint32_t CR;
    __IO uint32_t LLI;
} IF8080_GDMATypeDef;

typedef struct
{
    __IO uint32_t SAR_OFT;
    __IO uint32_t DAR_OFT;
} IF8080_GDMALLIOFTTypeDef;

/* ================================================================================ */
/* ================                      SmartCard                 ================ */
/* ================================================================================ */

/**
  * @brief SmartCard Controller, version 1.0.0. (SmartCard)
  */

typedef struct                  /*!< SmartCard Structure */
{
    __IO  uint32_t CR;          /*!<0x00 */
    __IO  uint32_t GCR;
    __IO  uint32_t TCR;
    __IO  uint32_t RCR;
    __IO  uint32_t THR;         /*!<0x10 */
    __I   uint32_t ISR;
    __IO  uint32_t IER;
    __IO  uint32_t TX_FIFO;
    __IO  uint32_t RX_FIFO;     /*!<0x20 */
    __I   uint32_t TSR;
    __I   uint32_t RSR;
    __IO  uint32_t ESR;         /*!<0x2C */
    __IO  uint32_t LP_CLKG;     /*!<0x30 */
} ISO7816_TypeDef;

/* ================================================================================ */
/* ================                       RTC                      ================ */
/* ================================================================================ */

/**
  * @brief Real time counter 0. (RTC)
  */

typedef struct
{
    __IO uint32_t CR0;                      /**< 0x00  */
    __IO uint32_t INT_MASK;                 /**< 0x04  */
    __IO uint32_t INT_SR;                   /**< 0x08  */
    __IO uint32_t PRESCALER;                /**< 0x0C  */
    __IO uint32_t COMP0;                    /**< 0x10  */
    __IO uint32_t COMP1;                    /**< 0x14  */
    __IO uint32_t COMP2;                    /**< 0x18  */
    __IO uint32_t COMP3;                    /**< 0x1C  */
    __IO uint32_t COMP0GT;                  /**< 0x20  */
    __IO uint32_t COMP1GT;                  /**< 0x24  */
    __IO uint32_t COMP2GT;                  /**< 0x28  */
    __IO uint32_t COMP3GT;                  /**< 0x2C  */
    __I  uint32_t CNT;                      /**< 0x30  */
    __I  uint32_t PRESCALE_CNT;             /**< 0x34  */
    __IO uint32_t PRESCALE_CMP;             /**< 0x38  */
    __IO uint32_t BACKUP;                   /**< 0x3C  */

} RTC_TypeDef;

/**
  * @brief Real time for lp (RTC)
  */
typedef struct                              // offset from 0x40000000
{
    __IO uint32_t LPC_CR0;                  /*!< 0x180 */
    __I  uint32_t LPC_SR;                   /*!< 0x184 */
    __IO uint32_t LPC_CMP_LOAD;             /*!< 0x188 */
    __IO uint32_t LPC_CMP_CNT;              /*!< 0x18C */
    //__IO uint32_t LPC_SR_IN_SLEEP_MODE;   // removed in BBLite
} LPC_TypeDef;

/**
  * @brief AON Watch Dog (RTC)
  */

/* 0x190 (AON_WDT_CRT)
    01:00    rw  00/01/11: turn on AON Watchdog 10: turn off AON Watchdog                                   2'b10
    02       rw  1: WDG countine count in DLPS 0:WDG stop count in DLPS                                     1'b0
    03       rw  1: reset whole chip            0: reset whole chip - (AON register and RTC)                1'b0
    04       rw  when  reg_aon_wdt_cnt_ctl == 0                                                             1'b0
                 1:relaod counter when exit DLPS 0:not reload counter when exit DLPS
    07:04    rw
    25:08    rw  Set the period of AON watchdog (unit:1/450Hz ~ 1/2.3KHz)                                   8'hff
    31:26    rw
*/
typedef union
{
    uint32_t d32;
    struct
    {
        uint32_t reg_aon_wdt_en: 2;            // 01:00
        uint32_t reg_aon_wdt_cnt_ctl: 1;       // 02
        uint32_t reg_aon_wdt_rst_lv_sel: 1;    // 03
        uint32_t reg_aon_wdt_cnt_reload: 1;    // 04
        uint32_t rsvd0: 3;                     // 07:05
        uint32_t reg_aon_wdt_comp: 18;         // 25:08
        uint32_t rsvd1: 6;                     // 31:26
    };
} RTC_AON_WDT_CRT_TYPE;

/* 0x198 & 0x1a4(AON_WDT_CRT)
    00    rw  write_protect 1:enable 0:disable               1'b0
    01    rw  wdt_disable                                    1'b0                                 8'hff
    31:02    rw
*/
typedef struct
{
    uint32_t write_protect : 1;
    uint32_t wdt_disable : 1;
    uint32_t reserved: 30;
} BTAON_FAST_RTC_AON_WDT_CONTROL;
typedef struct
{
    RTC_AON_WDT_CRT_TYPE aon_wdg_crt;
    uint32_t aon_wdg_clr;
    BTAON_FAST_RTC_AON_WDT_CONTROL aon_wdg_wp;
} BTAON_FAST_RTC_AON_WDT;

/**
  * @brief SLEEP MODE LED (RTC)
  */

typedef struct
{
    __IO uint32_t S_LED_CR;                 //!<0x00
    __IO uint32_t S_LED_CH0_CR0;            //!<0x04
    __IO uint32_t S_LED_CH0_CR1;            //!<0x08
    __IO uint32_t S_LED_CH0_P1_CR;          //!<0x0C
    __IO uint32_t S_LED_CH0_P2_CR;          //!<0x10
    __IO uint32_t S_LED_CH0_P3_CR;          //!<0x14
    __IO uint32_t S_LED_CH0_P4_CR;          //!<0x18
    __IO uint32_t S_LED_CH0_P5_CR;          //!<0x1C
    __IO uint32_t S_LED_CH0_P6_CR;          //!<0x20
    __IO uint32_t S_LED_CH0_P7_CR;          //!<0x24
    __IO uint32_t S_LED_CH0_P8_CR;          //!<0x28

    __IO uint32_t RSV0;                     //!<0x2C

    __IO uint32_t S_LED_CH1_CR0;            //!<0x30
    __IO uint32_t S_LED_CH1_CR1;            //!<0x34
    __IO uint32_t S_LED_CH1_P1_CR;          //!<0x38
    __IO uint32_t S_LED_CH1_P2_CR;          //!<0x3C
    __IO uint32_t S_LED_CH1_P3_CR;          //!<0x40
    __IO uint32_t S_LED_CH1_P4_CR;          //!<0x44
    __IO uint32_t S_LED_CH1_P5_CR;          //!<0x48
    __IO uint32_t S_LED_CH1_P6_CR;          //!<0x4C
    __IO uint32_t S_LED_CH1_P7_CR;          //!<0x50
    __IO uint32_t S_LED_CH1_P8_CR;          //!<0x54

    __IO uint32_t RSV1;                     //!<0x58
    __IO uint32_t RSV2;                     //!<0x5C

    __IO uint32_t S_LED_CH2_CR0;            //!<0x60
    __IO uint32_t S_LED_CH2_CR1;            //!<0x64
    __IO uint32_t S_LED_CH2_P1_CR;          //!<0x68
    __IO uint32_t S_LED_CH2_P2_CR;          //!<0x6C
    __IO uint32_t S_LED_CH2_P3_CR;          //!<0x70
    __IO uint32_t S_LED_CH2_P4_CR;          //!<0x74
    __IO uint32_t S_LED_CH2_P5_CR;          //!<0x78
    __IO uint32_t S_LED_CH2_P6_CR;          //!<0x7C
    __IO uint32_t S_LED_CH2_P7_CR;          //!<0x80
    __IO uint32_t S_LED_CH2_P8_CR;          //!<0x84
} RTC_LED_TypeDef;

/* ================================================================================ */
/* ================                      QDEC                      ================ */
/* ================================================================================ */

/**
  * @brief Rotary decoder. (QDEC)
  */

typedef struct                              /*!< QDEC Structure */
{
    __IO uint32_t   REG_DIV;                /*!< 0x00 */
    __IO uint32_t   REG_CR_X;               /*!< 0x04 */
    __IO uint32_t   REG_SR_X;               /*!< 0x08 */
    __IO uint32_t   REG_CR_Y;               /*!< 0x0C */
    __IO uint32_t   REG_SR_Y;               /*!< 0x10 */
    __IO uint32_t   REG_CR_Z;               /*!< 0x14 */
    __IO uint32_t   REG_SR_Z;               /*!< 0x18 */
    __IO uint32_t   INT_MASK;               /*!< 0x1C, Reserved */
    __IO uint32_t   INT_SR;                 /*!< 0x20 */
    __IO uint32_t   INT_CLR;                /*!< 0x24 */
    __IO uint32_t   REG_DBG;                /*!< 0x28 */
    __IO uint32_t   REG_VERSION;            /*!< 0x2C  */
} QDEC_TypeDef;

/* ================================================================================ */
/* ================                     Watch Dog                     ================ */
/* ================================================================================ */

/**
  * @brief Watch Dog. (WDG)
  */

typedef struct                              /*!< WDG Structure */
{
    __IO uint32_t WDG_CTL;                  /*!< 0x00 */
}   WDG_TypeDef;

/**
  * @brief AON Watchdog
  */
typedef struct
{
    union
    {
        __IO uint32_t CRT; /*!< 0x00 (R/W) Control Register                                       */
        struct
        {
            uint32_t EN: 2;     /*!< R/W WDT2 Enable, 2'b10: OFF, the other value: ON             */
            uint32_t CNT_CTL: 1; /*!< R/W Counter Control,
                                     1: WDG continue count in DLPS/Power Down/Hibernate,
                                     0: WDG stop count in DLPS/Power Down/Hibernate               */
            uint32_t RST_LVL: 1; /*!< R/W Reset Level,
                                   1: Level 2 reset (reset whole chip),
                                   0: Level 1 reset (reset whole chip except partial AON and RTC) */
            uint32_t CNT_RELOAD: 1; /*!< R/W Reload Counter,
                                        1: Reload counter when exit DLPS/Power Down/Hibernate,
                                        0: Not reload counter when exit DLPS/Power Down/Hibernate */
            uint32_t RSVD0: 3;
            uint32_t COMP: 18;  /*!< R/W Set the period of AON WDG : 1/450Hz ~ 1/2.3KHz           */
            uint32_t RSVD1: 6;
        } CRT_BITS;
    } u_00;
    __IO uint32_t CNT_CLR; /*!< 0x04 (R/W) Clear Register, write 1 to reload AON WDG counter      */
    __IO uint32_t WP;      /*!< 0x08 (R/W) Write Protect Register, write 1 to enable write CRT    */
} AON_WDG_TypeDef;

/* ================================================================================ */
/* ================                     random generator           ================ */
/* ================================================================================ */

/**
  * @brief random generator. (RAN_GEN)
  */

typedef struct                              /*!< RAN_GEN Structure */
{
    union
    {
        __IO uint32_t CTL;                  /*!< 0x00              */
        struct
        {
            __IO uint32_t rand_gen_en: 1;
            __IO uint32_t seed_upd: 1;
            __IO uint32_t random_req: 1;
            __IO uint32_t opt_rand_upd: 1;
            __IO uint32_t soft_rst: 1;
            __IO uint32_t rsvd: 27;
        } CTL_BITS;
    } u_00;
    __IO uint32_t POLYNOMIAL;               /*!< 0x04              */
    __IO uint32_t SEED;                     /*!< 0x08              */
    __IO uint32_t RAN_NUM;                  /*!< 0x0C              */
} RAN_GEN_TypeDef;


/* ================================================================================ */
/* ================            System Block Control            ================ */
/* ================================================================================ */

/* Auto gen based on BB2Plus_PERION_register_table_20230822_v0.xlsx */

typedef struct
{
    /* 0x0200       0x4000_0200
        0       R/W    r_cpu_slow_en                           1'b0
        1       R/W    r_cpu_slow_opt_wfi                      1'b0
        2       R/W    r_cpu_slow_opt_dsp                      1'b0
        3       R/W    r_dsp_slow_en                           1'b0
        4       R/W    r_dsp_slow_opt_dsp                      1'b0
        5       R/W    r_auto_dsp_fast_clk_en                  1'b0
        6       R/W    r_clk_cpu_f1m_en                        1'b0
        7       R/W    r_clk_cpu_32k_en                        1'b0
        8       R/W    r_aon_rd_opt                            1'b1
        10:9    R/W    r_bus_slow_sel                          2'b0
        18:11   R/W    r_dsp_fast_clk_ext_num                  8'b0
        24:19   R/W    r_bt_ahb_wait_cnt                       6'b010001
        25      R/W    r_btaon_acc_no_block                    1'b0
        26      R/W    r_cpu_slow_opt_at_tx                    1'b1
        27      R/W    r_cpu_slow_opt_at_rx                    1'b1
        31:28   R/W    r_cpu_low_rate_valid_num                4'h3
    */
    union
    {
        __IO uint32_t SYS_CLK_SEL;
        struct
        {
            __IO uint32_t r_cpu_slow_en: 1;
            __IO uint32_t r_cpu_slow_opt_wfi: 1;
            __IO uint32_t r_cpu_slow_opt_dsp: 1;
            __IO uint32_t r_dsp_slow_en: 1;
            __IO uint32_t r_dsp_slow_opt_dsp: 1;
            __IO uint32_t r_auto_dsp_fast_clk_en: 1;
            __IO uint32_t r_clk_cpu_f1m_en: 1;
            __IO uint32_t r_clk_cpu_32k_en: 1;
            __IO uint32_t r_aon_rd_opt: 1;
            __IO uint32_t r_bus_slow_sel: 2;
            __IO uint32_t r_dsp_fast_clk_ext_num: 8;
            __IO uint32_t r_bt_ahb_wait_cnt: 6;
            __IO uint32_t r_btaon_acc_no_block: 1;
            __IO uint32_t r_cpu_slow_opt_at_tx: 1;
            __IO uint32_t r_cpu_slow_opt_at_rx: 1;
            __IO uint32_t r_cpu_low_rate_valid_num: 4;
        } BITS_200;
    } u_200;

    /* 0x0204       0x4000_0204
        3:0     R      RF_RL_ID                                4'h0
        7:4     R      RF_RTL_ID                               4'h0
        8       R/W    reserved0204_08                         1'b0
        9       R/W    r_cpu_slow_opt_spic0                    1'b0
        10      R/W    r_cpu_slow_opt_spic1                    1'b0
        11      R/W    r_cpu_slow_opt_spic2                    1'b0
        12      R/W    r_cpu_slow_opt_spi0                     1'b0
        13      R/W    r_dsp_clk_src_pll_sel                   1'b0
        14      R/W    r_dsp_auto_slow_filter_en               1'b1
        15      R/W    bzdma_autoslow_eco_disable              1'b0
        16      R/W    reserved0204_16                         1'b0
        17      R/W    reserved0204_17                         1'b0
        18      R/W    r_auto_slow_opt                         1'b0
        19      R/W    r_cpu_slow_opt_dma                      1'b0
        20      R/W    r_cpu_slow_opt_sdio0                    1'b0
        21      R/W    reserved0204_21                         1'b0
        22      R/W    r_cpu_slow_opt_bt_sram_1                1'b0
        23      R/W    r_cpu_slow_opt_bt_sram_2                1'b0
        24      R/W    r_dsp_slow_opt_dspram_wbuf              1'b0
        25      R/W    r_cpu_slow_opt_dspram_wbuf              1'b0
        26      R/W    r_dsp_slow_opt_at_tx                    1'b1
        27      R/W    r_dsp_slow_opt_at_rx                    1'b1
        31:28   R/W    r_dsp_low_rate_valid_num                4'h3
    */
    union
    {
        __IO uint32_t SLOW_CTRL;
        struct
        {
            __I uint32_t RF_RL_ID: 4;
            __I uint32_t RF_RTL_ID: 4;
            __IO uint32_t reserved0204_08: 1;
            __IO uint32_t r_cpu_slow_opt_spic0: 1;
            __IO uint32_t r_cpu_slow_opt_spic1: 1;
            __IO uint32_t r_cpu_slow_opt_spic2: 1;
            __IO uint32_t r_cpu_slow_opt_spi0: 1;
            __IO uint32_t r_dsp_clk_src_pll_sel: 1;
            __IO uint32_t r_dsp_auto_slow_filter_en: 1;
            __IO uint32_t bzdma_autoslow_eco_disable: 1;
            __IO uint32_t reserved0204_16: 1;
            __IO uint32_t reserved0204_17: 1;
            __IO uint32_t r_auto_slow_opt: 1;
            __IO uint32_t r_cpu_slow_opt_dma: 1;
            __IO uint32_t r_cpu_slow_opt_sdio0: 1;
            __IO uint32_t reserved0204_21: 1;
            __IO uint32_t r_cpu_slow_opt_bt_sram_1: 1;
            __IO uint32_t r_cpu_slow_opt_bt_sram_2: 1;
            __IO uint32_t r_dsp_slow_opt_dspram_wbuf: 1;
            __IO uint32_t r_cpu_slow_opt_dspram_wbuf: 1;
            __IO uint32_t r_dsp_slow_opt_at_tx: 1;
            __IO uint32_t r_dsp_slow_opt_at_rx: 1;
            __IO uint32_t r_dsp_low_rate_valid_num: 4;
        } BITS_204;
    } u_204;

    /* 0x0208       0x4000_0208
        3:0     R/W    r_cpu_div_sel                           4'h4
        7:4     R/W    r_cpu_div_sel_slow                      4'h4
        8       R/W    r_cpu_div_en                            1'b1
        9       R/W    r_CPU_CLK_SRC_EN                        1'b1
        10      R/W    r_cpu_auto_slow_filter_en               1'b1
        11      R/W    r_cpu_auto_slow_force_update            1'b0
        12      R/W    r_cpu_pll_clk_cg_en                     1'b1
        13      R/W    r_cpu_xtal_clk_cg_en                    1'b1
        14      R/W    r_cpu_osc40_clk_cg_en                   1'b1
        15      R/W    r_cpu_div_en_slow                       1'b1
        19:16   R/W    r_dsp_div_sel                           4'h0
        23:20   R/W    r_dsp_div_sel_slow                      4'h0
        24      R/W    r_dsp_div_en                            1'b0
        25      R/W    r_DSP_CLK_SRC_EN                        1'b1
        26      R/W    r_dsp_clk_src_sel_1                     1'b1
        27      R/W    r_dsp_clk_src_sel_0                     1'b0
        28      R/W    r_dsp_pll_clk_cg_en                     1'b0
        29      R/W    r_dsp_xtal_clk_cg_en                    1'b0
        30      R/W    r_dsp_osc40_clk_cg_en                   1'b0
        31      R/W    r_dsp_div_en_slow                       1'b0
    */
    union
    {
        __IO uint32_t REG_0x208;
        struct
        {
            __IO uint32_t r_cpu_div_sel: 4;
            __IO uint32_t r_cpu_div_sel_slow: 4;
            __IO uint32_t r_cpu_div_en: 1;
            __IO uint32_t r_CPU_CLK_SRC_EN: 1;
            __IO uint32_t r_cpu_auto_slow_filter_en: 1;
            __IO uint32_t r_cpu_auto_slow_force_update: 1;
            __IO uint32_t r_cpu_pll_clk_cg_en: 1;
            __IO uint32_t r_cpu_xtal_clk_cg_en: 1;
            __IO uint32_t r_cpu_osc40_clk_cg_en: 1;
            __IO uint32_t r_cpu_div_en_slow: 1;
            __IO uint32_t r_dsp_div_sel: 4;
            __IO uint32_t r_dsp_div_sel_slow: 4;
            __IO uint32_t r_dsp_div_en: 1;
            __IO uint32_t r_DSP_CLK_SRC_EN: 1;
            __IO uint32_t r_dsp_clk_src_sel_1: 1;
            __IO uint32_t r_dsp_clk_src_sel_0: 1;
            __IO uint32_t r_dsp_pll_clk_cg_en: 1;
            __IO uint32_t r_dsp_xtal_clk_cg_en: 1;
            __IO uint32_t r_dsp_osc40_clk_cg_en: 1;
            __IO uint32_t r_dsp_div_en_slow: 1;
        } BITS_208;
    } u_208;

    /* 0x020C       0x4000_020c
        3:0     R/W    r_flash_div_sel                         4'h0
        4       R/W    r_flash_div_en                          1'h0
        5       R/W    r_FLASH_CLK_SRC_EN                      1'h1
        6       R/W    r_flash_clk_src_sel_1                   1'h0
        7       R/W    r_flash_clk_src_sel_0                   1'h0
        8       R/W    r_flash_mux_1_clk_cg_en                 1'h1
        9       R/W    r_rng_sfosc_sel                         1'h0
        12:10   R/W    r_rng_sfosc_div_sel                     3'h0
        13      R/W    r_flash_clk_src_pll_sel                 1'h0
        15:14   R/W    RESERVED020C_15_14                      2'h0
        18:16   R/W    r_40m_div_sel                           3'h0
        19      R/W    RESERVED020C_19                         1'h0
        20      R/W    r_40m_div_en                            1'h0
        21      R/W    r_CLK_40M_DIV_CG_EN                     1'h1
        22      R/W    r_CLK_40M_SRC_EN                        1'h1
        23      R/W    r_40m_clk_src_sel_1                     1'h0
        24      R/W    r_40m_clk_src_sel_0                     1'h1
        25      R/W    r_f40m_pll_clk_cg_en                    1'h0
        26      R/W    r_CLK_40M_SRC_DIV_EN                    1'h1
        27      R/W    r_CLK_20M_SRC_EN                        1'h0
        28      R/W    r_CLK_10M_SRC_EN                        1'h1
        29      R/W    r_CLK_5M_SRC_EN                         1'h0
        31:30   R/W    RESERVED020C_31_30                      2'h0
    */
    union
    {
        __IO uint32_t REG_0x20C;
        struct
        {
            __IO uint32_t r_flash_div_sel: 4;
            __IO uint32_t r_flash_div_en: 1;
            __IO uint32_t r_FLASH_CLK_SRC_EN: 1;
            __IO uint32_t r_flash_clk_src_sel_1: 1;
            __IO uint32_t r_flash_clk_src_sel_0: 1;
            __IO uint32_t r_flash_mux_1_clk_cg_en: 1;
            __IO uint32_t r_rng_sfosc_sel: 1;
            __IO uint32_t r_rng_sfosc_div_sel: 3;
            __IO uint32_t r_flash_clk_src_pll_sel: 1;
            __IO uint32_t RESERVED020C_15_14: 2;
            __IO uint32_t r_40m_div_sel: 3;
            __IO uint32_t RESERVED020C_19: 1;
            __IO uint32_t r_40m_div_en: 1;
            __IO uint32_t r_CLK_40M_DIV_CG_EN: 1;
            __IO uint32_t r_CLK_40M_SRC_EN: 1;
            __IO uint32_t r_40m_clk_src_sel_1: 1;
            __IO uint32_t r_40m_clk_src_sel_0: 1;
            __IO uint32_t r_f40m_pll_clk_cg_en: 1;
            __IO uint32_t r_CLK_40M_SRC_DIV_EN: 1;
            __IO uint32_t r_CLK_20M_SRC_EN: 1;
            __IO uint32_t r_CLK_10M_SRC_EN: 1;
            __IO uint32_t r_CLK_5M_SRC_EN: 1;
            __IO uint32_t RESERVED020C_31_30: 2;
        } BITS_20C;
    } u_20C;

    /* 0x0210       0x4000_0210
        0       R/W    RESERVED0210_00                         1'h0
        1       R/W    RESERVED0210_01                         1'h0
        2       R/W    BIT_SOC_BTBUS_EN                        1'b1
        3       R/W    RESERVED0210_03                         1'h0
        4       R/W    BIT_SOC_FLASH_EN                        1'b1
        5       R/W    BIT_SOC_FLASH_1_EN                      1'h0
        6       R/W    BIT_SOC_FLASH_2_EN                      1'h0
        7       R/W    reserved0210_07                         1'h0
        8       R/W    RESERVED0210_08                         1'h0
        10:9    R/W    RESERVED0210_10_09                      2'h0
        11      R/W    RESERVED0210_11                         1'h0
        12      R/W    BIT_SOC_UART1_EN                        1'h0
        13      R/W    BIT_SOC_GDMA0_EN                        1'h0
        14      R/W    BIT_SOC_SDH_EN                          1'h0
        15      R/W    BIT_SOC_USB_EN                          1'h0
        16      R/W    BIT_SOC_GTIMER_EN                       1'h0
        17      R/W    RESERVED0210_17                         1'h0
        18      R/W    BIT_SOC_SWR_SS_EN                       1'h0
        19      R/W    RESERVED0210_19                         1'h0
        20      R/W    BIT_SOC_AAC_XTAL_EN                     1'h0
        21      R/W    RESERVED0210_21                         1'h0
        22      R/W    BIT_SOC_ETIMER_EN                       1'h0
        23      R/W    BIT_SOC_CAN_EN                          1'h0
        24      R/W    BIT_SOC_PPE_EN                          1'h0
        25      R/W    BIT_SOC_PKE_EN                          1'h0
        26      R/W    BIT_SOC_IMDC_EN                        1'h0
        31:27   R/W    RESERVED0210_31_27                      5'h0
    */
    union
    {
        __IO uint32_t SOC_FUNC_EN;
        struct
        {
            __IO uint32_t RESERVED0210_00: 1;
            __IO uint32_t RESERVED0210_01: 1;
            __IO uint32_t BIT_SOC_BTBUS_EN: 1;
            __IO uint32_t RESERVED0210_03: 1;
            __IO uint32_t BIT_SOC_FLASH_EN: 1;
            __IO uint32_t BIT_SOC_FLASH_1_EN: 1;
            __IO uint32_t BIT_SOC_FLASH_2_EN: 1;
            __IO uint32_t reserved0210_07: 1;
            __IO uint32_t RESERVED0210_08: 1;
            __IO uint32_t RESERVED0210_10_09: 2;
            __IO uint32_t RESERVED0210_11: 1;
            __IO uint32_t BIT_SOC_UART1_EN: 1;
            __IO uint32_t BIT_SOC_GDMA0_EN: 1;
            __IO uint32_t BIT_SOC_SDH_EN: 1;
            __IO uint32_t BIT_SOC_USB_EN: 1;
            __IO uint32_t BIT_SOC_GTIMER_EN: 1;
            __IO uint32_t RESERVED0210_17: 1;
            __IO uint32_t BIT_SOC_SWR_SS_EN: 1;
            __IO uint32_t RESERVED0210_19: 1;
            __IO uint32_t BIT_SOC_AAC_XTAL_EN: 1;
            __IO uint32_t RESERVED0210_21: 1;
            __IO uint32_t BIT_SOC_ETIMER_EN: 1;
            __IO uint32_t BIT_SOC_CAN_EN: 1;
            __IO uint32_t BIT_SOC_PPE_EN: 1;
            __IO uint32_t BIT_SOC_PKE_EN: 1;
            __IO uint32_t BIT_SOC_IMDC_EN: 1;
            __IO uint32_t RESERVED0210_31_27: 5;
        } BITS_210;
    } u_210;

    /* 0x0214       0x4000_0214
        2:0     R/W    sdio0_clk_div                           3'h0
        3       R/W    sdio0_clk_div_en                        1'h0
        4       R/W    sdio0_clk_src_en                        1'h0
        5       R/W    sdio0_clk_sel0                          1'h0
        6       R/W    sdio0_clk_sel1                          1'h0
        7       R/W    RESERVED0214_07                         1'h0
        10:8    R/W    disp_clk_div                            3'h0
        11      R/W    disp_clk_div_en                         1'h0
        12      R/W    disp_clk_src_en                         1'h0
        13      R/W    disp_clk_sel0                           1'h0
        14      R/W    disp_clk_sel1                           1'h0
        15      R/W    RESERVED0214_15                         1'h0
        31:16   R/W    RESERVED0214_31_16                      16'h0
    */
    union
    {
        __IO uint32_t REG_0x0214;
        struct
        {
            __IO uint32_t sdio0_clk_div: 3;
            __IO uint32_t sdio0_clk_div_en: 1;
            __IO uint32_t sdio0_clk_src_en: 1;
            __IO uint32_t sdio0_clk_sel0: 1;
            __IO uint32_t sdio0_clk_sel1: 1;
            __IO uint32_t RESERVED0214_07: 1;
            __IO uint32_t disp_clk_div: 3;
            __IO uint32_t disp_clk_div_en: 1;
            __IO uint32_t disp_clk_src_en: 1;
            __IO uint32_t disp_clk_sel0: 1;
            __IO uint32_t disp_clk_sel1: 1;
            __IO uint32_t RESERVED0214_15: 1;
            __IO uint32_t RESERVED0214_31_16: 16;
        } BITS_214;
    } u_214;

    /* 0x0218       0x4000_0218
        0       R/W    BIT_PERI_UART0_EN                       1'h0
        1       R/W    BIT_PERI_UART2_EN                       1'h0
        2       R/W    BIT_PERI_AES_EN                         1'h0
        3       R/W    BIT_PERI_RNG_EN                         1'h0
        4       R/W    BIT_PERI_SIMC_EN                        1'h0
        5       R/W    BIT_PERI_LCD_EN                         1'h0
        6       R/W    BIT_PERI_SHA256_EN                      1'h0
        7       R/W    BIT_PERI_SM3_EN                         1'h0
        8       R/W    BIT_PERI_SPI0_EN                        1'h0
        9       R/W    BIT_PERI_SPI1_EN                        1'h0
        10      R/W    BIT_PERI_IRRC_EN                        1'h0
        11      R/W    BIT_PERI_SPI2_EN                        1'h0
        15:12   R/W    RESERVED0218_15_12                      4'h0
        16      R/W    BIT_PERI_I2C0_EN                        1'h0
        17      R/W    BIT_PERI_I2C1_EN                        1'h0
        18      R/W    BIT_PERI_QDEC_EN                        1'h0
        19      R/W    BIT_PERI_KEYSCAN_EN                     1'h0
        20      R/W    BIT_PERI_I2C2_EN                        1'h0
        21      R/W    RESERVED0218_21                         1'h0
        22      R/W    BIT_PERI_PSRAM_EN                       1'h0
        23      R/W    RESERVED0218_23                         1'h0
        24      R/W    BIT_PERI_SPI2W_EN                       1'h0
        25      R/W    BIT_DSP_CORE_EN                         1'b1
        26      R/W    BIT_DSP_H2D_D2H                         1'b1
        27      R/W    BIT_DSP_MEM_EN                          1'b1
        28      R/W    BIT_ASRC_EN                             1'h0
        29      R/W    BIT_DSP_WDT_EN                          1'h0
        30      R/W    BIT_EFUSE_EN                            1'h0
        31      R/W    BIT_DATA_MEM_EN                         1'h0
    */
    union
    {
        __IO uint32_t SOC_PERI_FUNC0_EN;
        struct
        {
            __IO uint32_t BIT_PERI_UART0_EN: 1;
            __IO uint32_t BIT_PERI_UART2_EN: 1;
            __IO uint32_t BIT_PERI_AES_EN: 1;
            __IO uint32_t BIT_PERI_RNG_EN: 1;
            __IO uint32_t BIT_PERI_SIMC_EN: 1;
            __IO uint32_t BIT_PERI_LCD_EN: 1;
            __IO uint32_t BIT_PERI_SHA256_EN: 1;
            __IO uint32_t BIT_PERI_SM3_EN: 1;
            __IO uint32_t BIT_PERI_SPI0_EN: 1;
            __IO uint32_t BIT_PERI_SPI1_EN: 1;
            __IO uint32_t BIT_PERI_IRRC_EN: 1;
            __IO uint32_t BIT_PERI_SPI2_EN: 1;
            __IO uint32_t RESERVED0218_15_12: 4;
            __IO uint32_t BIT_PERI_I2C0_EN: 1;
            __IO uint32_t BIT_PERI_I2C1_EN: 1;
            __IO uint32_t BIT_PERI_QDEC_EN: 1;
            __IO uint32_t BIT_PERI_KEYSCAN_EN: 1;
            __IO uint32_t BIT_PERI_I2C2_EN: 1;
            __IO uint32_t RESERVED0218_21: 1;
            __IO uint32_t BIT_PERI_PSRAM_EN: 1;
            __IO uint32_t RESERVED0218_23: 1;
            __IO uint32_t BIT_PERI_SPI2W_EN: 1;
            __IO uint32_t BIT_DSP_CORE_EN: 1;
            __IO uint32_t BIT_DSP_H2D_D2H: 1;
            __IO uint32_t BIT_DSP_MEM_EN: 1;
            __IO uint32_t BIT_ASRC_EN: 1;
            __IO uint32_t BIT_DSP_WDT_EN: 1;
            __IO uint32_t BIT_EFUSE_EN: 1;
            __IO uint32_t BIT_DATA_MEM_EN: 1;
        } BITS_218;
    } u_218;

    /* 0x021C       0x4000_021c
        0       R/W    BIT_PERI_ADC_EN                         1'h0
        7:1     R/W    RESERVED021C_07_01                      7'h0
        8       R/W    BIT_PERI_GPIO_EN                        1'h0
        9       R/W    BIT_PERI_GPIO1_EN                       1'h0
        31:10   R/W    RESERVED021C_31_10                      22'h0
    */
    union
    {
        __IO uint32_t SOC_PERI_FUNC1_EN;
        struct
        {
            __IO uint32_t BIT_PERI_ADC_EN: 1;
            __IO uint32_t RESERVED021C_07_01: 7;
            __IO uint32_t BIT_PERI_GPIO_EN: 1;
            __IO uint32_t BIT_PERI_GPIO1_EN: 1;
            __IO uint32_t RESERVED021C_31_10: 22;
        } BITS_21C;
    } u_21C;

    /* 0x0220       0x4000_0220
        0       R/W    r_PON_FEN_AUDIO                         1'h0
        1       R/W    r_PON_FEN_SPORT0                        1'h0
        2       R/W    r_PON_FEN_SPORT1                        1'h0
        3       R/W    RESERVED0220_03                         1'h0
        4       R/W    r_CLK_EN_AUDIO                          1'h0
        5       R/W    r_CLK_EN_SPORT0                         1'h0
        6       R/W    r_CLK_EN_SPORT1                         1'h0
        7       R/W    RESERVED0220_07                         1'h0
        8       R/W    r_CLK_EN_SPORT_40M                      1'h0
        9       R/W    r_CLK_EN_SI                             1'h1
        10      R/W    r_PON_FEN_SPORT2                        1'h1
        11      R/W    RESERVED0220_11                         1'h0
        12      R/W    r_CLK_EN_SPORT2                         1'h1
        31:13   R/W    RESERVED0220_31_13                      19'h0
    */
    union
    {
        __IO uint32_t SOC_AUDIO_IF_EN;
        struct
        {
            __IO uint32_t r_PON_FEN_AUDIO: 1;
            __IO uint32_t r_PON_FEN_SPORT0: 1;
            __IO uint32_t r_PON_FEN_SPORT1: 1;
            __IO uint32_t RESERVED0220_03: 1;
            __IO uint32_t r_CLK_EN_AUDIO: 1;
            __IO uint32_t r_CLK_EN_SPORT0: 1;
            __IO uint32_t r_CLK_EN_SPORT1: 1;
            __IO uint32_t RESERVED0220_07: 1;
            __IO uint32_t r_CLK_EN_SPORT_40M: 1;
            __IO uint32_t r_CLK_EN_SI: 1;
            __IO uint32_t r_PON_FEN_SPORT2: 1;
            __IO uint32_t RESERVED0220_11: 1;
            __IO uint32_t r_CLK_EN_SPORT2: 1;
            __IO uint32_t RESERVED0220_31_13: 19;
        } BITS_220;
    } u_220;

    /* 0x0224       0x4000_0224
        2:0     R/W    r_SPORT0_PLL_CLK_SEL                    3'b100
        3       R/W    r_SPORT0_EXT_CODEC                      1'h0
        6:4     R/W    r_SPORT1_PLL_CLK_SEL                    3'b001
        7       R/W    r_CODEC_STANDALONE                      1'h0
        15:8    R/W    r_PLL_DIV0_SETTING                      8'h0
        23:16   R/W    r_PLL_DIV1_SETTING                      8'h0
        31:24   R/W    r_PLL_DIV2_SETTING                      8'h0
    */
    union
    {
        __IO uint32_t SOC_AUDIO_CLK_CTRL_A;
        struct
        {
            __IO uint32_t r_SPORT0_PLL_CLK_SEL: 3;
            __IO uint32_t r_SPORT0_EXT_CODEC: 1;
            __IO uint32_t r_SPORT1_PLL_CLK_SEL: 3;
            __IO uint32_t r_CODEC_STANDALONE: 1;
            __IO uint32_t r_PLL_DIV0_SETTING: 8;
            __IO uint32_t r_PLL_DIV1_SETTING: 8;
            __IO uint32_t r_PLL_DIV2_SETTING: 8;
        } BITS_224;
    } u_224;

    /* 0x0228       0x4000_0228
        2:0     R/W    RESERVED0228_02_00                      3'h1
        3       R/W    r_SPORT0_MCLK_OUT                       1'h0
        4       R/W    r_SPORT1_MCLK_OUT                       1'h0
        5       R/W    r_SPORT2_MCLK_OUT                       1'h0
        6       R/W    RESERVED0228_06                         1'h0
        7       R/W    r_AUDIO_CLK_FROM_PLL                    1'h0
        8       R/W    r_SPORT1_EXT_CODEC                      1'h0
        12:9    R/W    RESERVED0228_12_09                      4'b0
        15:13   R/W    r_SPORT2_PLL_CLK_SEL                    3'b0
        18:16   R/W    RESERVED0228_18_16                      3'b0
        19      R/W    r_SPORT2_EXT_CODEC                      1'h0
        31:20   R/W    RESERVED0228_31_20                      12'b0
    */
    union
    {
        __IO uint32_t SOC_AUDIO_CLK_CTRL_B;
        struct
        {
            __IO uint32_t RESERVED0228_02_00: 3;
            __IO uint32_t r_SPORT0_MCLK_OUT: 1;
            __IO uint32_t r_SPORT1_MCLK_OUT: 1;
            __IO uint32_t r_SPORT2_MCLK_OUT: 1;
            __IO uint32_t RESERVED0228_06: 1;
            __IO uint32_t r_AUDIO_CLK_FROM_PLL: 1;
            __IO uint32_t r_SPORT1_EXT_CODEC: 1;
            __IO uint32_t RESERVED0228_12_09: 4;
            __IO uint32_t r_SPORT2_PLL_CLK_SEL: 3;
            __IO uint32_t RESERVED0228_18_16: 3;
            __IO uint32_t r_SPORT2_EXT_CODEC: 1;
            __IO uint32_t RESERVED0228_31_20: 12;
        } BITS_228;
    } u_228;

    /* 0x022C       0x4000_022c
        31:0    R/W    RESERVED022C_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x022C;
        struct
        {
            __IO uint32_t RESERVED022C_31_00: 32;
        } BITS_22C;
    } u_22C;

    /* 0x0230       0x4000_0230
        0       R/W    RESERVED0230_00                         1'h1
        1       R/W    BIT_CKE_CORDIC                          1'h0
        2       R/W    BIT_SOC_CKE_PLFM                        1'h1
        3       R/W    r_CKE_CTRLAP                            1'h1
        4       R/W    BIT_CKE_BUS_RAM_SLP                     1'h0
        5       R/W    BIT_CKE_BT_VEN                          1'h1
        6       R/W    BIT_SOC_ACTCK_VENDOR_REG_EN             1'h0
        7       R/W    BIT_SOC_SLPCK_VENDOR_REG_EN             1'h0
        8       R/W    BIT_SOC_ACTCK_FLASH_EN                  1'h1
        9       R/W    BIT_SOC_SLPCK_FLASH_EN                  1'h0
        10      R/W    BIT_SOC_ACTCK_UART2_EN                  1'h0
        11      R/W    BIT_SOC_SLPCK_UART2_EN                  1'h0
        12      R/W    BIT_SOC_ACTCK_UART1_EN                  1'h0
        13      R/W    BIT_SOC_SLPCK_UART1_EN                  1'h0
        14      R/W    BIT_SOC_ACTCK_TIMER_EN                  1'h0
        15      R/W    BIT_SOC_SLPCK_TIMER_EN                  1'h0
        16      R/W    BIT_SOC_ACTCK_GDMA0_EN                  1'h0
        17      R/W    BIT_SOC_SLPCK_GDMA0_EN                  1'h0
        18      R/W    BIT_SOC_ACTCK_FLASH1_EN                 1'h0
        19      R/W    BIT_SOC_SLPCK_FLASH1_EN                 1'h0
        20      R/W    BIT_SOC_ACTCK_FLASH2_EN                 1'h0
        21      R/W    BIT_SOC_SLPCK_FLASH2_EN                 1'h0
        22      R/W    BIT_SOC_ACTCK_GPIO1_EN                  1'h0
        23      R/W    BIT_SOC_SLPCK_GPIO1_EN                  1'h0
        24      R/W    BIT_SOC_ACTCK_GPIO_EN                   1'h0
        25      R/W    BIT_SOC_SLPCK_GPIO_EN                   1'h0
        26      R/W    BIT_SOC_ACTCK_SDH_EN                    1'h0
        27      R/W    BIT_SOC_SLPCK_SDH_EN                    1'h0
        28      R/W    BIT_SOC_ACTCK_USB_EN                    1'h0
        29      R/W    BIT_SOC_SLPCK_USB_EN                    1'h0
        31:30   R/W    RESERVED0230_31_30                      2'h0
    */
    union
    {
        __IO uint32_t PESOC_CLK_CTRL;
        struct
        {
            __IO uint32_t RESERVED0230_00: 1;
            __IO uint32_t BIT_CKE_CORDIC: 1;
            __IO uint32_t BIT_SOC_CKE_PLFM: 1;
            __IO uint32_t r_CKE_CTRLAP: 1;
            __IO uint32_t BIT_CKE_BUS_RAM_SLP: 1;
            __IO uint32_t BIT_CKE_BT_VEN: 1;
            __IO uint32_t BIT_SOC_ACTCK_VENDOR_REG_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_VENDOR_REG_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_FLASH_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_FLASH_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_UART2_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_UART2_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_UART1_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_UART1_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_TIMER_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_TIMER_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_GDMA0_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_GDMA0_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_FLASH1_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_FLASH1_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_FLASH2_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_FLASH2_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_GPIO1_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_GPIO1_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_GPIO_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_GPIO_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_SDH_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_SDH_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_USB_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_USB_EN: 1;
            __IO uint32_t RESERVED0230_31_30: 2;
        } BITS_230;
    } u_230;

    /* 0x0234       0x4000_0234
        0       R/W    BIT_SOC_ACTCK_UART0_EN                  1'h0
        1       R/W    BIT_SOC_SLPCK_UART0_EN                  1'h0
        2       R/W    BIT_SOC_ACTCK_HCI                       1'h0
        3       R/W    BIT_SOC_SLPCK_HCI                       1'h0
        4       R/W    BIT_CKE_MODEM                           1'h1
        5       R/W    BIT_CKE_CAL32K                          1'h0
        6       R/W    BIT_CKE_SWR_SS                          1'h0
        7       R/W    RESERVED0234_07                         1'h0
        8       R/W    BIT_CKE_RNG                             1'h0
        9       R/W    BIT_CKE_PDCK                            1'h0
        10      R/W    BIT_CKE_AAC_XTAL                        1'h0
        11      R/W    BIT_CKE_SHA256                          1'h0
        12      R/W    BIT_CKE_SM3                             1'h0
        13      R/W    RESERVED0234_13                         1'h0
        14      R/W    BIT_SOC_ACTCK_ETIMER                    1'h0
        15      R/W    BIT_SOC_SLPCK_ETIMER                    1'h0
        16      R/W    BIT_SOC_ACTCK_SPI0_EN                   1'h0
        17      R/W    BIT_SOC_SLPCK_SPI0_EN                   1'h0
        18      R/W    BIT_SOC_ACTCK_SPI1_EN                   1'h0
        19      R/W    BIT_SOC_SLPCK_SPI1_EN                   1'h0
        20      R/W    BIT_SOC_ACTCK_IRRC                      1'h0
        21      R/W    BIT_SOC_SLPCK_IRRC                      1'h0
        22      R/W    BIT_SOC_ACTCK_SPI2_EN                   1'h0
        23      R/W    BIT_SOC_SLPCK_SPI2_EN                   1'h0
        24      R/W    BIT_SOC_ACTCK_CAN                       1'h0
        25      R/W    BIT_SOC_SLPCK_CAN                       1'h0
        26      R/W    BIT_SOC_ACTCK_PPE                       1'h0
        27      R/W    BIT_SOC_SLPCK_PPE                       1'h0
        28      R/W    BIT_SOC_ACTCK_PKE                       1'h0
        29      R/W    BIT_SOC_SLPCK_PKE                       1'h0
        30      R/W    BIT_SOC_ACTCK_IMDC                     1'h0
        31      R/W    BIT_SOC_SLPCK_IMDC                     1'h0
    */
    union
    {
        __IO uint32_t PESOC_PERI_CLK_CTRL0;
        struct
        {
            __IO uint32_t BIT_SOC_ACTCK_UART0_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_UART0_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_HCI: 1;
            __IO uint32_t BIT_SOC_SLPCK_HCI: 1;
            __IO uint32_t BIT_CKE_MODEM: 1;
            __IO uint32_t BIT_CKE_CAL32K: 1;
            __IO uint32_t BIT_CKE_SWR_SS: 1;
            __IO uint32_t RESERVED0234_07: 1;
            __IO uint32_t BIT_CKE_RNG: 1;
            __IO uint32_t BIT_CKE_PDCK: 1;
            __IO uint32_t BIT_CKE_AAC_XTAL: 1;
            __IO uint32_t BIT_CKE_SHA256: 1;
            __IO uint32_t BIT_CKE_SM3: 1;
            __IO uint32_t RESERVED0234_13: 1;
            __IO uint32_t BIT_SOC_ACTCK_ETIMER: 1;
            __IO uint32_t BIT_SOC_SLPCK_ETIMER: 1;
            __IO uint32_t BIT_SOC_ACTCK_SPI0_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_SPI0_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_SPI1_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_SPI1_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_IRRC: 1;
            __IO uint32_t BIT_SOC_SLPCK_IRRC: 1;
            __IO uint32_t BIT_SOC_ACTCK_SPI2_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_SPI2_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_CAN: 1;
            __IO uint32_t BIT_SOC_SLPCK_CAN: 1;
            __IO uint32_t BIT_SOC_ACTCK_PPE: 1;
            __IO uint32_t BIT_SOC_SLPCK_PPE: 1;
            __IO uint32_t BIT_SOC_ACTCK_PKE: 1;
            __IO uint32_t BIT_SOC_SLPCK_PKE: 1;
            __IO uint32_t BIT_SOC_ACTCK_IMDC: 1;
            __IO uint32_t BIT_SOC_SLPCK_IMDC: 1;
        } BITS_234;
    } u_234;

    /* 0x0238       0x4000_0238
        0       R/W    BIT_SOC_ACTCK_I2C0_EN                   1'h0
        1       R/W    BIT_SOC_SLPCK_I2C0_EN                   1'h0
        2       R/W    BIT_SOC_ACTCK_I2C1_EN                   1'h0
        3       R/W    BIT_SOC_SLPCK_I2C1_EN                   1'h0
        4       R/W    BIT_SOC_ACTCK_QDEC_EN                   1'h0
        5       R/W    BIT_SOC_SLPCK_QDEC_EN                   1'h0
        6       R/W    BIT_SOC_ACTCK_KEYSCAN_EN                1'h0
        7       R/W    BIT_SOC_SLPCK_KEYSCAN_EN                1'h0
        8       R/W    BIT_SOC_ACTCK_AES_EN                    1'h0
        9       R/W    BIT_SOC_SLPCK_AES_EN                    1'h0
        10      R/W    BIT_SOC_ACTCK_SIMC_EN                   1'h0
        11      R/W    BIT_SOC_SLPCK_SIMC_EN                   1'h0
        12      R/W    BIT_SOC_ACTCK_I2C2_EN                   1'h0
        13      R/W    BIT_SOC_SLPCK_I2C2_EN                   1'h0
        14      R/W    BIT_SOC_ACTCK_DATA_MEM_EN               1'h0
        15      R/W    BIT_SOC_SLPCK_DATA_MEM_EN               1'h0
        16      R/W    BIT_SOC_ACTCK_SPI2W_EN                  1'h0
        17      R/W    BIT_SOC_SLPCK_SPI2W_EN                  1'h0
        18      R/W    BIT_SOC_ACTCK_LCD_EN                    1'h0
        19      R/W    BIT_SOC_SLPCK_LCD_EN                    1'h0
        20      R/W    BIT_SOC_ACTCKE_ASRC                     1'h0
        21      R/W    BIT_SOC_SLPCKE_ASRC                     1'h0
        22      R/W    BIT_SOC_ACTCKE_DSP_MEM                  1'h0
        23      R/W    BIT_SOC_SLPCKE_DSP_MEM                  1'h0
        24      R/W    BIT_SOC_ACTCK_ADC_EN                    1'h0
        25      R/W    BIT_SOC_SLPCK_ADC_EN                    1'h0
        26      R/W    BIT_SOC_ACTCKE_H2D_D2H                  1'h0
        27      R/W    BIT_SOC_SLPCKE_H2D_D2H                  1'h0
        28      R/W    BIT_SOC_ACTCKE_DSP                      1'h0
        29      R/W    BIT_SOC_SLPCKE_DSP                      1'h0
        30      R/W    BIT_SOC_CKE_DSP_WDT                     1'h0
        31      R/W    BIT_SOC_CLK_EFUSE                       1'h0
    */
    union
    {
        __IO uint32_t PESOC_PERI_CLK_CTRL1;
        struct
        {
            __IO uint32_t BIT_SOC_ACTCK_I2C0_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_I2C0_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_I2C1_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_I2C1_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_QDEC_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_QDEC_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_KEYSCAN_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_KEYSCAN_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_AES_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_AES_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_SIMC_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_SIMC_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_I2C2_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_I2C2_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_DATA_MEM_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_DATA_MEM_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_SPI2W_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_SPI2W_EN: 1;
            __IO uint32_t BIT_SOC_ACTCK_LCD_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_LCD_EN: 1;
            __IO uint32_t BIT_SOC_ACTCKE_ASRC: 1;
            __IO uint32_t BIT_SOC_SLPCKE_ASRC: 1;
            __IO uint32_t BIT_SOC_ACTCKE_DSP_MEM: 1;
            __IO uint32_t BIT_SOC_SLPCKE_DSP_MEM: 1;
            __IO uint32_t BIT_SOC_ACTCK_ADC_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_ADC_EN: 1;
            __IO uint32_t BIT_SOC_ACTCKE_H2D_D2H: 1;
            __IO uint32_t BIT_SOC_SLPCKE_H2D_D2H: 1;
            __IO uint32_t BIT_SOC_ACTCKE_DSP: 1;
            __IO uint32_t BIT_SOC_SLPCKE_DSP: 1;
            __IO uint32_t BIT_SOC_CKE_DSP_WDT: 1;
            __IO uint32_t BIT_SOC_CLK_EFUSE: 1;
        } BITS_238;
    } u_238;

    /* 0x023C       0x4000_023c
        0       R/W    r_epalna_od                             1'h0
        1       R/W    r_spic_dbg_dis                          1'h0
        3:2     R/W    r_SPIC_MON_SEL                          2'h0
        31:4    R/W    RESERVED023C_31_04                      28'h0
    */
    union
    {
        __IO uint32_t REG_TESTMODE_SEL_RF;
        struct
        {
            __IO uint32_t r_epalna_od: 1;
            __IO uint32_t r_spic_dbg_dis: 1;
            __IO uint32_t r_SPIC_MON_SEL: 2;
            __IO uint32_t RESERVED023C_31_04: 28;
        } BITS_23C;
    } u_23C;

    /* 0x0240       0x4000_0240
        31:0    R      RESERVED0240_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x0240;
        struct
        {
            __I uint32_t RESERVED0240_31_00: 32;
        } BITS_240;
    } u_240;

    /* 0x0244       0x4000_0244
        0       R/W    BIT_SOC_ACTCK_BTBUS_EN                  1'h1
        1       R/W    BIT_SOC_SLPCK_BTBUS_EN                  1'h1
        31:2    R/W    RESERVED0244_31_02                      30'h0
    */
    union
    {
        __IO uint32_t OFF_MEM_PWR_CTRL;
        struct
        {
            __IO uint32_t BIT_SOC_ACTCK_BTBUS_EN: 1;
            __IO uint32_t BIT_SOC_SLPCK_BTBUS_EN: 1;
            __IO uint32_t RESERVED0244_31_02: 30;
        } BITS_244;
    } u_244;

    /* 0x0248       0x4000_0248
        7:0     R/W    RESERVED0248_07_00                      8'h0
        10:8    R/W    r_swr_ss_div_sel                        3'h0
        31:11   R/W    RESERVED0248_31_11                      21'h0
    */
    union
    {
        __IO uint32_t REG_0x248;
        struct
        {
            __IO uint32_t RESERVED0248_07_00: 8;
            __IO uint32_t r_swr_ss_div_sel: 3;
            __IO uint32_t RESERVED0248_31_11: 21;
        } BITS_248;
    } u_248;

    /* 0x024C       0x4000_024c
        31:0    R      RESERVED024C_31_00                      32'hEAEAEAEA
    */
    union
    {
        __IO uint32_t REG_0x024C;
        struct
        {
            __I uint32_t RESERVED024C_31_00: 32;
        } BITS_24C;
    } u_24C;

    /* 0x0250       0x4000_0250
        31:0    R      RESERVED0250_31_00                      32'hEAEAEAEA
    */
    union
    {
        __IO uint32_t REG_0x0250;
        struct
        {
            __I uint32_t RESERVED0250_31_00: 32;
        } BITS_250;
    } u_250;

    /* 0x0254       0x4000_0254
        0       R/W    DSP_RUN_STALL                           1'h1
        1       R/W    DSP_STAT_VECTOR_SEL                     1'h0
        2       R/W    reg_bypass_pipe                         1'h0
        14:3    R/W    DUMMY                                   12'h0
        15      R/W    HW_ASRC_MCU_EN                          1'h0
        19:16   R/W    r_cpu_low_rate_valid_num1               4'h3
        23:20   R/W    r_dsp_low_rate_valid_num1               4'h3
        24      R/W    r_cpu_auto_slow_filter1_en              1'h0
        25      R/W    r_dsp_auto_slow_filter1_en              1'h0
        29:26   R/W    RESERVED0254_29_26                      4'h0
        30      R      km4_warm_rst_n_from_reg                 1'h0
        31      R/W    reg_trigger_reset_km4                   1'h0
    */
    union
    {
        __IO uint32_t REG_0x254;
        struct
        {
            __IO uint32_t DSP_RUN_STALL: 1;
            __IO uint32_t DSP_STAT_VECTOR_SEL: 1;
            __IO uint32_t reg_bypass_pipe: 1;
            __IO uint32_t RESERVED_0: 12;
            __IO uint32_t HW_ASRC_MCU_EN: 1;
            __IO uint32_t r_cpu_low_rate_valid_num1: 4;
            __IO uint32_t r_dsp_low_rate_valid_num1: 4;
            __IO uint32_t r_cpu_auto_slow_filter1_en: 1;
            __IO uint32_t r_dsp_auto_slow_filter1_en: 1;
            __IO uint32_t RESERVED0254_29_26: 4;
            __I uint32_t km4_warm_rst_n_from_reg: 1;
            __IO uint32_t reg_trigger_reset_km4: 1;
        } BITS_254;
    } u_254;

    /* 0x0258       0x4000_0258
        1:0     R/W    CORE0_TUNE_OCP_RES                      2'h0
        4:2     R/W    CORE0_TUNE_PWM_R3                       3'h0
        7:5     R/W    CORE0_TUNE_PWM_R2                       3'h0
        10:8    R/W    CORE0_TUNE_PWM_R1                       3'h0
        13:11   R/W    CORE0_TUNE_PWM_C3                       3'h0
        16:14   R/W    CORE0_TUNE_PWM_C2                       3'h0
        19:17   R/W    CORE0_TUNE_PWM_C1                       3'h0
        20      R/W    CORE0_BYPASS_PWM_BYPASS_RoughSS         1'h0
        22:21   R/W    CORE0_BYPASS_PWM_TUNE_RoughSS           2'h0
        25:23   R/W    CORE0_BYPASS_PWM_TUNE_VCL               3'h0
        28:26   R/W    CORE0_BYPASS_PWM_TUNE_VCH               3'h0
        29      R/W    CORE0_X4_PWM_COMP_IB                    1'h0
        30      R/W    CORE0_X4_POW_PWM_CLP                    1'h0
        31      R/W    CORE0_X4_TUNE_VDIV_Bit0                 1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR0_31_0;
        struct
        {
            __IO uint32_t CORE0_TUNE_OCP_RES: 2;
            __IO uint32_t CORE0_TUNE_PWM_R3: 3;
            __IO uint32_t CORE0_TUNE_PWM_R2: 3;
            __IO uint32_t CORE0_TUNE_PWM_R1: 3;
            __IO uint32_t CORE0_TUNE_PWM_C3: 3;
            __IO uint32_t CORE0_TUNE_PWM_C2: 3;
            __IO uint32_t CORE0_TUNE_PWM_C1: 3;
            __IO uint32_t CORE0_BYPASS_PWM_BYPASS_RoughSS: 1;
            __IO uint32_t CORE0_BYPASS_PWM_TUNE_RoughSS: 2;
            __IO uint32_t CORE0_BYPASS_PWM_TUNE_VCL: 3;
            __IO uint32_t CORE0_BYPASS_PWM_TUNE_VCH: 3;
            __IO uint32_t CORE0_X4_PWM_COMP_IB: 1;
            __IO uint32_t CORE0_X4_POW_PWM_CLP: 1;
            __IO uint32_t CORE0_X4_TUNE_VDIV_Bit0: 1;
        } BITS_258;
    } u_258;

    /* 0x025C       0x4000_025c
        6:0     R/W    CORE0_X4_TUNE_VDIV_Bit7_Bit1            7'h0
        14:7    R/W    CORE0_BYPASS_PWM_TUNE_POS_VREFPFM       8'h0
        17:15   R/W    CORE0_BYPASS_PWM_TUNE_POS_VREFOCP       3'h0
        18      R/W    CORE0_FPWM                              1'h0
        19      R/W    CORE0_POW_PFM                           1'h0
        20      R/W    CORE0_POW_PWM                           1'h0
        21      R/W    CORE0_POW_VDIV                          1'h0
        23:22   R/W    CORE0_XTAL_OV_RATIO                     2'h0
        26:24   R/W    CORE0_XTAL_OV_UNIT                      3'h0
        29:27   R/W    CORE0_XTAL_OV_MODE                      3'h0
        30      R/W    CORE0_EN_POWERMOS_DR8X                  1'h0
        31      R/W    CORE0_SEL_OCP_TABLE                     1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR0_63_32;
        struct
        {
            __IO uint32_t CORE0_X4_TUNE_VDIV_Bit7_Bit1: 7;
            __IO uint32_t CORE0_BYPASS_PWM_TUNE_POS_VREFPFM: 8;
            __IO uint32_t CORE0_BYPASS_PWM_TUNE_POS_VREFOCP: 3;
            __IO uint32_t CORE0_FPWM: 1;
            __IO uint32_t CORE0_POW_PFM: 1;
            __IO uint32_t CORE0_POW_PWM: 1;
            __IO uint32_t CORE0_POW_VDIV: 1;
            __IO uint32_t CORE0_XTAL_OV_RATIO: 2;
            __IO uint32_t CORE0_XTAL_OV_UNIT: 3;
            __IO uint32_t CORE0_XTAL_OV_MODE: 3;
            __IO uint32_t CORE0_EN_POWERMOS_DR8X: 1;
            __IO uint32_t CORE0_SEL_OCP_TABLE: 1;
        } BITS_25C;
    } u_25C;

    /* 0x0260       0x4000_0260
        1:0     R/W    CORE4_TUNE_OCP_RES                      2'h0
        4:2     R/W    CORE4_TUNE_PWM_R3                       3'h0
        7:5     R/W    CORE4_TUNE_PWM_R2                       3'h0
        10:8    R/W    CORE4_TUNE_PWM_R1                       3'h0
        13:11   R/W    CORE4_TUNE_PWM_C3                       3'h0
        16:14   R/W    CORE4_TUNE_PWM_C2                       3'h0
        19:17   R/W    CORE4_TUNE_PWM_C1                       3'h0
        20      R/W    CORE4_BYPASS_PWM_BYPASS_RoughSS         1'h0
        22:21   R/W    CORE4_BYPASS_PWM_TUNE_RoughSS           2'h0
        25:23   R/W    CORE4_BYPASS_PWM_TUNE_VCL               3'h0
        28:26   R/W    CORE4_BYPASS_PWM_TUNE_VCH               3'h0
        29      R/W    CORE4_X4_PWM_COMP_IB                    1'h0
        30      R/W    CORE4_X4_POW_PWM_CLP                    1'h0
        31      R/W    CORE4_X4_TUNE_VDIV_Bit0                 1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR4_31_0;
        struct
        {
            __IO uint32_t CORE4_TUNE_OCP_RES: 2;
            __IO uint32_t CORE4_TUNE_PWM_R3: 3;
            __IO uint32_t CORE4_TUNE_PWM_R2: 3;
            __IO uint32_t CORE4_TUNE_PWM_R1: 3;
            __IO uint32_t CORE4_TUNE_PWM_C3: 3;
            __IO uint32_t CORE4_TUNE_PWM_C2: 3;
            __IO uint32_t CORE4_TUNE_PWM_C1: 3;
            __IO uint32_t CORE4_BYPASS_PWM_BYPASS_RoughSS: 1;
            __IO uint32_t CORE4_BYPASS_PWM_TUNE_RoughSS: 2;
            __IO uint32_t CORE4_BYPASS_PWM_TUNE_VCL: 3;
            __IO uint32_t CORE4_BYPASS_PWM_TUNE_VCH: 3;
            __IO uint32_t CORE4_X4_PWM_COMP_IB: 1;
            __IO uint32_t CORE4_X4_POW_PWM_CLP: 1;
            __IO uint32_t CORE4_X4_TUNE_VDIV_Bit0: 1;
        } BITS_260;
    } u_260;

    /* 0x0264       0x4000_0264
        6:0     R/W    CORE4_X4_TUNE_VDIV_Bit7_Bit1            7'h0
        14:7    R/W    CORE4_BYPASS_PWM_TUNE_POS_VREFPFM       8'h0
        17:15   R/W    CORE4_BYPASS_PWM_TUNE_POS_VREFOCP       3'h0
        18      R/W    CORE4_FPWM                              1'h0
        19      R/W    CORE4_POW_PFM                           1'h0
        20      R/W    CORE4_POW_PWM                           1'h0
        21      R/W    CORE4_POW_VDIV                          1'h0
        23:22   R/W    CORE4_XTAL_OV_RATIO                     2'h0
        26:24   R/W    CORE4_XTAL_OV_UNIT                      3'h0
        29:27   R/W    CORE4_XTAL_OV_MODE                      3'h0
        30      R/W    CORE4_EN_POWERMOS_DR8X                  1'h0
        31      R/W    CORE4_SEL_OCP_TABLE                     1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR4_63_32;
        struct
        {
            __IO uint32_t CORE4_X4_TUNE_VDIV_Bit7_Bit1: 7;
            __IO uint32_t CORE4_BYPASS_PWM_TUNE_POS_VREFPFM: 8;
            __IO uint32_t CORE4_BYPASS_PWM_TUNE_POS_VREFOCP: 3;
            __IO uint32_t CORE4_FPWM: 1;
            __IO uint32_t CORE4_POW_PFM: 1;
            __IO uint32_t CORE4_POW_PWM: 1;
            __IO uint32_t CORE4_POW_VDIV: 1;
            __IO uint32_t CORE4_XTAL_OV_RATIO: 2;
            __IO uint32_t CORE4_XTAL_OV_UNIT: 3;
            __IO uint32_t CORE4_XTAL_OV_MODE: 3;
            __IO uint32_t CORE4_EN_POWERMOS_DR8X: 1;
            __IO uint32_t CORE4_SEL_OCP_TABLE: 1;
        } BITS_264;
    } u_264;

    /* 0x0268       0x4000_0268
        1:0     R/W    CORE5_TUNE_OCP_RES                      2'h0
        4:2     R/W    CORE5_TUNE_PWM_R3                       3'h0
        7:5     R/W    CORE5_TUNE_PWM_R2                       3'h0
        10:8    R/W    CORE5_TUNE_PWM_R1                       3'h0
        13:11   R/W    CORE5_TUNE_PWM_C3                       3'h0
        16:14   R/W    CORE5_TUNE_PWM_C2                       3'h0
        19:17   R/W    CORE5_TUNE_PWM_C1                       3'h0
        20      R/W    CORE5_BYPASS_PWM_BYPASS_RoughSS         1'h0
        22:21   R/W    CORE5_BYPASS_PWM_TUNE_RoughSS           2'h0
        25:23   R/W    CORE5_BYPASS_PWM_TUNE_VCL               3'h0
        28:26   R/W    CORE5_BYPASS_PWM_TUNE_VCH               3'h0
        29      R/W    CORE5_X4_PWM_COMP_IB                    1'h0
        30      R/W    CORE5_X4_POW_PWM_CLP                    1'h0
        31      R/W    CORE5_X4_TUNE_VDIV_Bit0                 1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR5_31_0;
        struct
        {
            __IO uint32_t CORE5_TUNE_OCP_RES: 2;
            __IO uint32_t CORE5_TUNE_PWM_R3: 3;
            __IO uint32_t CORE5_TUNE_PWM_R2: 3;
            __IO uint32_t CORE5_TUNE_PWM_R1: 3;
            __IO uint32_t CORE5_TUNE_PWM_C3: 3;
            __IO uint32_t CORE5_TUNE_PWM_C2: 3;
            __IO uint32_t CORE5_TUNE_PWM_C1: 3;
            __IO uint32_t CORE5_BYPASS_PWM_BYPASS_RoughSS: 1;
            __IO uint32_t CORE5_BYPASS_PWM_TUNE_RoughSS: 2;
            __IO uint32_t CORE5_BYPASS_PWM_TUNE_VCL: 3;
            __IO uint32_t CORE5_BYPASS_PWM_TUNE_VCH: 3;
            __IO uint32_t CORE5_X4_PWM_COMP_IB: 1;
            __IO uint32_t CORE5_X4_POW_PWM_CLP: 1;
            __IO uint32_t CORE5_X4_TUNE_VDIV_Bit0: 1;
        } BITS_268;
    } u_268;

    /* 0x026C       0x4000_026c
        6:0     R/W    CORE5_X4_TUNE_VDIV_Bit7_Bit1            7'h0
        14:7    R/W    CORE5_BYPASS_PWM_TUNE_POS_VREFPFM       8'h0
        17:15   R/W    CORE5_BYPASS_PWM_TUNE_POS_VREFOCP       3'h0
        18      R/W    CORE5_FPWM                              1'h0
        19      R/W    CORE5_POW_PFM                           1'h0
        20      R/W    CORE5_POW_PWM                           1'h0
        21      R/W    CORE5_POW_VDIV                          1'h0
        23:22   R/W    CORE5_XTAL_OV_RATIO                     2'h0
        26:24   R/W    CORE5_XTAL_OV_UNIT                      3'h0
        29:27   R/W    CORE5_XTAL_OV_MODE                      3'h0
        30      R/W    CORE5_EN_POWERMOS_DR8X                  1'h0
        31      R/W    CORE5_SEL_OCP_TABLE                     1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR5_63_32;
        struct
        {
            __IO uint32_t CORE5_X4_TUNE_VDIV_Bit7_Bit1: 7;
            __IO uint32_t CORE5_BYPASS_PWM_TUNE_POS_VREFPFM: 8;
            __IO uint32_t CORE5_BYPASS_PWM_TUNE_POS_VREFOCP: 3;
            __IO uint32_t CORE5_FPWM: 1;
            __IO uint32_t CORE5_POW_PFM: 1;
            __IO uint32_t CORE5_POW_PWM: 1;
            __IO uint32_t CORE5_POW_VDIV: 1;
            __IO uint32_t CORE5_XTAL_OV_RATIO: 2;
            __IO uint32_t CORE5_XTAL_OV_UNIT: 3;
            __IO uint32_t CORE5_XTAL_OV_MODE: 3;
            __IO uint32_t CORE5_EN_POWERMOS_DR8X: 1;
            __IO uint32_t CORE5_SEL_OCP_TABLE: 1;
        } BITS_26C;
    } u_26C;

    /* 0x0270       0x4000_0270
        0       R/W    CORE0_EN_HVD17_LOWIQ                    1'h0
        4:1     R/W    CORE0_TUNE_HVD17_IB                     4'h0
        5       R/W    CORE0_X4_PFM_COMP_IB                    1'h0
        8:6     R/W    CORE0_TUNE_PFM_VREFOCPPFM               3'h0
        14:9    R/W    CORE0_TUNE_SAW_ICLK                     6'h0
        15      R/W    RESERVED0270_15                         1'h0
        16      R/W    CORE4_EN_HVD17_LOWIQ                    1'h0
        20:17   R/W    CORE4_TUNE_HVD17_IB                     4'h0
        21      R/W    CORE4_X4_PFM_COMP_IB                    1'h0
        24:22   R/W    CORE4_TUNE_PFM_VREFOCPPFM               3'h0
        30:25   R/W    CORE4_TUNE_SAW_ICLK                     6'h0
        31      R/W    RESERVED0270_31                         1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR4_79_64_AUTO_SW_PAR0_79_64;
        struct
        {
            __IO uint32_t CORE0_EN_HVD17_LOWIQ: 1;
            __IO uint32_t CORE0_TUNE_HVD17_IB: 4;
            __IO uint32_t CORE0_X4_PFM_COMP_IB: 1;
            __IO uint32_t CORE0_TUNE_PFM_VREFOCPPFM: 3;
            __IO uint32_t CORE0_TUNE_SAW_ICLK: 6;
            __IO uint32_t RESERVED0270_15: 1;
            __IO uint32_t CORE4_EN_HVD17_LOWIQ: 1;
            __IO uint32_t CORE4_TUNE_HVD17_IB: 4;
            __IO uint32_t CORE4_X4_PFM_COMP_IB: 1;
            __IO uint32_t CORE4_TUNE_PFM_VREFOCPPFM: 3;
            __IO uint32_t CORE4_TUNE_SAW_ICLK: 6;
            __IO uint32_t RESERVED0270_31: 1;
        } BITS_270;
    } u_270;

    /* 0x0274       0x4000_0274
        19:0    R/W    r_dss_data_in                           20'h0
        22:20   R/W    r_dss_ro_sel                            3'h0
        23      R/W    r_dss_wire_sel                          1'h0
        24      R/W    r_dss_clk_en                            1'h0
        25      R/W    r_dss_speed_en                          1'h0
        26      R/W    r_FEN_DSS                               1'h0
        31:27   R/W    RESERVED0274_31_27                      5'h0
    */
    union
    {
        __IO uint32_t REG_DSS_CTRL;
        struct
        {
            __IO uint32_t r_dss_data_in: 20;
            __IO uint32_t r_dss_ro_sel: 3;
            __IO uint32_t r_dss_wire_sel: 1;
            __IO uint32_t r_dss_clk_en: 1;
            __IO uint32_t r_dss_speed_en: 1;
            __IO uint32_t r_FEN_DSS: 1;
            __IO uint32_t RESERVED0274_31_27: 5;
        } BITS_274;
    } u_274;

    /* 0x0278       0x4000_0278
        19:0    R      bset_dss_count_out                      20'h0
        20      R      bset_dss_wsort_go                       1'h0
        21      R      bset_dss_ready                          1'h0
        31:22   R      RESERVED0278_31_22                      10'h0
    */
    union
    {
        __IO uint32_t REG_BEST_DSS_RD;
        struct
        {
            __I uint32_t bset_dss_count_out: 20;
            __I uint32_t bset_dss_wsort_go: 1;
            __I uint32_t bset_dss_ready: 1;
            __I uint32_t RESERVED0278_31_22: 10;
        } BITS_278;
    } u_278;

    /* 0x027C       0x4000_027c
        19:0    R      dss_ir_count_out                        20'h0
        20      R      dss_ir_wsort_go                         1'h0
        21      R      dss_ir_ready                            1'h0
        31:22   R      RESERVED027C_31_22                      10'h0
    */
    union
    {
        __IO uint32_t REG_BEST_DSS_IR_RD;
        struct
        {
            __I uint32_t dss_ir_count_out: 20;
            __I uint32_t dss_ir_wsort_go: 1;
            __I uint32_t dss_ir_ready: 1;
            __I uint32_t RESERVED027C_31_22: 10;
        } BITS_27C;
    } u_27C;

    /* 0x0280       0x4000_0280
        7:0     R/W    PMUX_GPIO_ADC_0                         8'h0
        15:8    R/W    PMUX_GPIO_ADC_1                         8'h0
        23:16   R/W    PMUX_GPIO_ADC_2                         8'h0
        31:24   R/W    PMUX_GPIO_ADC_3                         8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO_A0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_ADC_0: 8;
            __IO uint32_t PMUX_GPIO_ADC_1: 8;
            __IO uint32_t PMUX_GPIO_ADC_2: 8;
            __IO uint32_t PMUX_GPIO_ADC_3: 8;
        } BITS_280;
    } u_280;

    /* 0x0284       0x4000_0284
        7:0     R/W    PMUX_GPIO_P3_2                          8'h0
        15:8    R/W    PMUX_GPIO_P3_3                          8'h0
        23:16   R/W    PMUX_GPIO_P3_4                          8'h0
        31:24   R/W    PMUX_GPIO_P3_5                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO_A4_7;
        struct
        {
            __IO uint32_t PMUX_GPIO_P3_2: 8;
            __IO uint32_t PMUX_GPIO_P3_3: 8;
            __IO uint32_t PMUX_GPIO_P3_4: 8;
            __IO uint32_t PMUX_GPIO_P3_5: 8;
        } BITS_284;
    } u_284;

    /* 0x0288       0x4000_0288
        7:0     R/W    PMUX_GPIO_P1_0                          8'h38
        15:8    R/W    PMUX_GPIO_P1_1                          8'h39
        23:16   R/W    PMUX_GPIO_P1_2                          8'h0
        31:24   R/W    PMUX_GPIO_P1_3                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO_B0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_P1_0: 8;
            __IO uint32_t PMUX_GPIO_P1_1: 8;
            __IO uint32_t PMUX_GPIO_P1_2: 8;
            __IO uint32_t PMUX_GPIO_P1_3: 8;
        } BITS_288;
    } u_288;

    /* 0x028C       0x4000_028c
        7:0     R/W    PMUX_GPIO_P1_4                          8'h0
        15:8    R/W    PMUX_GPIO_P1_5                          8'h0
        23:16   R/W    PMUX_GPIO_P1_6                          8'h0
        31:24   R/W    PMUX_GPIO_P1_7                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO_B4_7;
        struct
        {
            __IO uint32_t PMUX_GPIO_P1_4: 8;
            __IO uint32_t PMUX_GPIO_P1_5: 8;
            __IO uint32_t PMUX_GPIO_P1_6: 8;
            __IO uint32_t PMUX_GPIO_P1_7: 8;
        } BITS_28C;
    } u_28C;

    /* 0x0290       0x4000_0290
        7:0     R/W    PMUX_GPIO_P2_0                          8'h18
        15:8    R/W    PMUX_GPIO_P2_1                          8'h0
        23:16   R/W    PMUX_GPIO_P2_2                          8'h0
        31:24   R/W    PMUX_GPIO_P2_3                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO_C0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_P2_0: 8;
            __IO uint32_t PMUX_GPIO_P2_1: 8;
            __IO uint32_t PMUX_GPIO_P2_2: 8;
            __IO uint32_t PMUX_GPIO_P2_3: 8;
        } BITS_290;
    } u_290;

    /* 0x0294       0x4000_0294
        7:0     R/W    PMUX_GPIO_P2_4                          8'h0
        15:8    R/W    PMUX_GPIO_P2_5                          8'h0
        23:16   R/W    PMUX_GPIO_P2_6                          8'h0
        31:24   R/W    PMUX_GPIO_P2_7                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO_C4_7;
        struct
        {
            __IO uint32_t PMUX_GPIO_P2_4: 8;
            __IO uint32_t PMUX_GPIO_P2_5: 8;
            __IO uint32_t PMUX_GPIO_P2_6: 8;
            __IO uint32_t PMUX_GPIO_P2_7: 8;
        } BITS_294;
    } u_294;

    /* 0x0298       0x4000_0298
        7:0     R/W    PMUX_GPIO_P3_0                          8'h24
        15:8    R/W    PMUX_GPIO_P3_1                          8'h23
        23:16   R/W    PMUX_GPIO_H_0                           8'h0
        31:24   R/W    PMUX_GPIO_H_1                           8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO_D0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_P3_0: 8;
            __IO uint32_t PMUX_GPIO_P3_1: 8;
            __IO uint32_t PMUX_GPIO_H_0: 8;
            __IO uint32_t PMUX_GPIO_H_1: 8;
        } BITS_298;
    } u_298;

    /* 0x029C       0x4000_029c
        7:0     R/W    PMUX_GPIO_H_2                           8'h2e
        15:8    R/W    PMUX_GPIO_H_3                           8'h2f
        23:16   R/W    PMUX_GPIO_H_4                           8'h30
        31:24   R/W    PMUX_GPIO_H_5                           8'h2d
    */
    union
    {
        __IO uint32_t REG_GPIO_D4_7;
        struct
        {
            __IO uint32_t PMUX_GPIO_H_2: 8;
            __IO uint32_t PMUX_GPIO_H_3: 8;
            __IO uint32_t PMUX_GPIO_H_4: 8;
            __IO uint32_t PMUX_GPIO_H_5: 8;
        } BITS_29C;
    } u_29C;

    /* 0x02A0       0x4000_02a0
        7:0     R/W    PMUX_GPIO_H_6                           8'h0
        31:8    R/W    RESERVED02A0_31_08                      24'h0
    */
    union
    {
        __IO uint32_t REG_GPIO_E0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_H_6: 8;
            __IO uint32_t RESERVED02A0_31_08: 24;
        } BITS_2A0;
    } u_2A0;

    /* 0x02A4       0x4000_02a4
        31:0    R/W    RESERVED02A4_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_GPIOE_dummy;
        struct
        {
            __IO uint32_t RESERVED02A4_31_00: 32;
        } BITS_2A4;
    } u_2A4;

    /* 0x02A8       0x4000_02a8
        3:0     R/W    PMUX_TEST_MODE                          4'h1
        6:4     R/W    RESERVED02A8_06_04                      3'h0
        7       R/W    PMUX_TEST_MODE_EN                       1'h0
        8       R/W    PMUX_DBG_INF_EN                         1'h0
        9       R/W    PMUX_DBG_FLASH_INF_EN                   1'h0
        10      R/W    PMUX_SPI_DMA_REQ_EN                     1'h0
        11      R/W    PMUX_OPI_EN                             1'h0
        12      R/W    PMUX_LCD_EN                             1'h0
        13      R/W    PMUX_LCD_VSYNC_DIS                      1'h0
        14      R/W    PMUX_LCD_RD_DIS                         1'h0
        15      R/W    PMUX_LCD_VSYNC_IO_SEL                   1'h0
        19:16   R/W    PMUX_DBG_MODE_SEL                       4'h2
        20      R/W    PMUX_FLASH_EXTS_FT_EN                   1'h0
        22:21   R/W    RESERVED02A8_22_21                      2'h0
        23      R/W    r_dbg_cpu_dsp_clk_en                    1'h0
        24      R/W    SPIC_MASTER_EN                          1'h0
        25      R/W    SPIC1_MASTER_EN                         1'h0
        26      R/W    SPIC2_MASTER_EN                         1'h0
        27      R/W    PMUX_FLASH_EXTC_MP_EN                   1'h0
        28      R/W    PMUX_DIG_SMUX_EN                        1'h0
        29      R/W    SPIC3_MASTER_EN                         1'h0
        30      R/W    bypass_dcd_dbnc                         1'h0
        31      R/W    bypass_non_std_det                      1'h0
    */
    union
    {
        __IO uint32_t REG_TEST_MODE;
        struct
        {
            __IO uint32_t PMUX_TEST_MODE: 4;
            __IO uint32_t RESERVED02A8_06_04: 3;
            __IO uint32_t PMUX_TEST_MODE_EN: 1;
            __IO uint32_t PMUX_DBG_INF_EN: 1;
            __IO uint32_t PMUX_DBG_FLASH_INF_EN: 1;
            __IO uint32_t PMUX_SPI_DMA_REQ_EN: 1;
            __IO uint32_t PMUX_OPI_EN: 1;
            __IO uint32_t PMUX_LCD_EN: 1;
            __IO uint32_t PMUX_LCD_VSYNC_DIS: 1;
            __IO uint32_t PMUX_LCD_RD_DIS: 1;
            __IO uint32_t PMUX_LCD_VSYNC_IO_SEL: 1;
            __IO uint32_t PMUX_DBG_MODE_SEL: 4;
            __IO uint32_t PMUX_FLASH_EXTS_FT_EN: 1;
            __IO uint32_t RESERVED02A8_22_21: 2;
            __IO uint32_t r_dbg_cpu_dsp_clk_en: 1;
            __IO uint32_t SPIC_MASTER_EN: 1;
            __IO uint32_t SPIC1_MASTER_EN: 1;
            __IO uint32_t SPIC2_MASTER_EN: 1;
            __IO uint32_t PMUX_FLASH_EXTC_MP_EN: 1;
            __IO uint32_t PMUX_DIG_SMUX_EN: 1;
            __IO uint32_t SPIC3_MASTER_EN: 1;
            __IO uint32_t bypass_dcd_dbnc: 1;
            __IO uint32_t bypass_non_std_det: 1;
        } BITS_2A8;
    } u_2A8;

    /* 0x02AC       0x4000_02ac
        0       R/W    r_PMUX_UART0_1_W_CTRL                   1'h0
        1       R/W    r_PMUX_UARTLOG_1_W_CTRL                 1'h0
        2       R/W    r_PMUX_UARTLOG1_1_W_CTRL                1'h0
        3       R/W    r_PMUX_UART0_1_W_EN                     1'h0
        4       R/W    r_PMUX_UARTLOG_1_W_EN                   1'h0
        5       R/W    r_PMUX_UARTLOG1_1_W_EN                  1'h0
        7:6     R/W    RESERVED02AC_07_06                      2'h0
        8       R/W    r_SPIC0_PULL_SEL_SIO0_PULL_CTRL         1'h0
        9       R/W    r_SPIC0_PULL_SEL_SIO1_PULL_CTRL         1'h0
        10      R/W    r_SPIC0_PULL_SEL_SIO2_PULL_CTRL         1'h0
        11      R/W    r_SPIC0_PULL_SEL_SIO3_PULL_CTRL         1'h0
        12      R/W    r_SPIC1_PULL_SEL_SIO0_PULL_CTRL         1'h0
        13      R/W    r_SPIC1_PULL_SEL_SIO1_PULL_CTRL         1'h0
        14      R/W    r_SPIC1_PULL_SEL_SIO2_PULL_CTRL         1'h0
        15      R/W    r_SPIC1_PULL_SEL_SIO3_PULL_CTRL         1'h0
        16      R/W    r_SPIC2_PULL_SEL_SIO0_PULL_CTRL         1'h0
        17      R/W    r_SPIC2_PULL_SEL_SIO1_PULL_CTRL         1'h0
        18      R/W    r_SPIC2_PULL_SEL_SIO2_PULL_CTRL         1'h0
        19      R/W    r_SPIC2_PULL_SEL_SIO3_PULL_CTRL         1'h0
        20      R/W    r_SPIC3_PULL_SEL_SIO0_PULL_CTRL         1'h0
        21      R/W    r_SPIC3_PULL_SEL_SIO1_PULL_CTRL         1'h0
        22      R/W    r_SPIC3_PULL_SEL_SIO2_PULL_CTRL         1'h0
        23      R/W    r_SPIC3_PULL_SEL_SIO3_PULL_CTRL         1'h0
        31:24   R/W    RESERVED02AC_31_24                      8'h0
    */
    union
    {
        __IO uint32_t REG_SPIC_PULL_SEL;
        struct
        {
            __IO uint32_t r_PMUX_UART0_1_W_CTRL: 1;
            __IO uint32_t r_PMUX_UARTLOG_1_W_CTRL: 1;
            __IO uint32_t r_PMUX_UARTLOG1_1_W_CTRL: 1;
            __IO uint32_t r_PMUX_UART0_1_W_EN: 1;
            __IO uint32_t r_PMUX_UARTLOG_1_W_EN: 1;
            __IO uint32_t r_PMUX_UARTLOG1_1_W_EN: 1;
            __IO uint32_t RESERVED02AC_07_06: 2;
            __IO uint32_t r_SPIC0_PULL_SEL_SIO0_PULL_CTRL: 1;
            __IO uint32_t r_SPIC0_PULL_SEL_SIO1_PULL_CTRL: 1;
            __IO uint32_t r_SPIC0_PULL_SEL_SIO2_PULL_CTRL: 1;
            __IO uint32_t r_SPIC0_PULL_SEL_SIO3_PULL_CTRL: 1;
            __IO uint32_t r_SPIC1_PULL_SEL_SIO0_PULL_CTRL: 1;
            __IO uint32_t r_SPIC1_PULL_SEL_SIO1_PULL_CTRL: 1;
            __IO uint32_t r_SPIC1_PULL_SEL_SIO2_PULL_CTRL: 1;
            __IO uint32_t r_SPIC1_PULL_SEL_SIO3_PULL_CTRL: 1;
            __IO uint32_t r_SPIC2_PULL_SEL_SIO0_PULL_CTRL: 1;
            __IO uint32_t r_SPIC2_PULL_SEL_SIO1_PULL_CTRL: 1;
            __IO uint32_t r_SPIC2_PULL_SEL_SIO2_PULL_CTRL: 1;
            __IO uint32_t r_SPIC2_PULL_SEL_SIO3_PULL_CTRL: 1;
            __IO uint32_t r_SPIC3_PULL_SEL_SIO0_PULL_CTRL: 1;
            __IO uint32_t r_SPIC3_PULL_SEL_SIO1_PULL_CTRL: 1;
            __IO uint32_t r_SPIC3_PULL_SEL_SIO2_PULL_CTRL: 1;
            __IO uint32_t r_SPIC3_PULL_SEL_SIO3_PULL_CTRL: 1;
            __IO uint32_t RESERVED02AC_31_24: 8;
        } BITS_2AC;
    } u_2AC;

    /* 0x02B0       0x4000_02b0
        0       R      SPI_FLASH_SEL                           1'h0
        31:1    R/W    RESERVED02B0_31_01                      31'h0
    */
    union
    {
        __IO uint32_t REG_0x2B0;
        struct
        {
            __I uint32_t SPI_FLASH_SEL: 1;
            __IO uint32_t RESERVED02B0_31_01: 31;
        } BITS_2B0;
    } u_2B0;

    /* 0x02B4       0x4000_02b4
        0       R/W    PMUX_SDIO_EN                            1'h0
        1       R/W    PMUX_SDIO_SEL                           1'h0
        3:2     R/W    RESERVED02B0_03_02                      2'h0
        11:4    R/W    PMUX_SDIO_PIN_EN                        8'h0
        31:12   R/W    RESERVED02B4_31_12                      20'h0
    */
    union
    {
        __IO uint32_t REG_0x2B4;
        struct
        {
            __IO uint32_t PMUX_SDIO_EN: 1;
            __IO uint32_t PMUX_SDIO_SEL: 1;
            __IO uint32_t RESERVED02B0_03_02: 2;
            __IO uint32_t PMUX_SDIO_PIN_EN: 8;
            __IO uint32_t RESERVED02B4_31_12: 20;
        } BITS_2B4;
    } u_2B4;

    /* 0x02B8       0x4000_02b8
        0       R/W    SPIC1_QPI_EN                            1'h0
        1       R/W    SPIC0_DDR_MODE_EN                       1'h0
        2       R/W    SPIC2_DDR_MODE_EN                       1'h0
        3       R/W    SPIC0_SCLK_SHIFT_EN                     1'h0
        4       R/W    SPIC2_SCLK_SHIFT_EN                     1'h0
        11:5    R/W    spi_dqs_dly                             7'h0
        19:12   R/W    cko_dly_sel                             8'h0
        20      R/W    fetch_sclk_phase                        1'h0
        21      R/W    fetch_sclk_phase_2                      1'h0
        22      R/W    ds_sel                                  1'h0
        23      R/W    ds_dtr                                  1'h0
        24      R/W    data_phase_sel                          1'h0
        25      R/W    r_pos_data_order                        1'h0
        28:26   R/W    r_spi_clk_sel                           3'h0
        31:29   R/W    RESERVED02B8_31_29                      3'h0
    */
    union
    {
        __IO uint32_t REG_SPI_OPI_PHY_Ctrl;
        struct
        {
            __IO uint32_t SPIC1_QPI_EN: 1;
            __IO uint32_t SPIC0_DDR_MODE_EN: 1;
            __IO uint32_t SPIC2_DDR_MODE_EN: 1;
            __IO uint32_t SPIC0_SCLK_SHIFT_EN: 1;
            __IO uint32_t SPIC2_SCLK_SHIFT_EN: 1;
            __IO uint32_t spi_dqs_dly: 7;
            __IO uint32_t cko_dly_sel: 8;
            __IO uint32_t fetch_sclk_phase: 1;
            __IO uint32_t fetch_sclk_phase_2: 1;
            __IO uint32_t ds_sel: 1;
            __IO uint32_t ds_dtr: 1;
            __IO uint32_t data_phase_sel: 1;
            __IO uint32_t r_pos_data_order: 1;
            __IO uint32_t r_spi_clk_sel: 3;
            __IO uint32_t RESERVED02B8_31_29: 3;
        } BITS_2B8;
    } u_2B8;

    /* 0x02BC       0x4000_02bc
        31:0    R/W    RESERVED02BC_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x02BC;
        struct
        {
            __IO uint32_t RESERVED02BC_31_00: 32;
        } BITS_2BC;
    } u_2BC;

    /* 0x02C0       0x4000_02c0
        1:0     R/W    CORE6_TUNE_OCP_RES                      2'h0
        4:2     R/W    CORE6_TUNE_PWM_R3                       3'h0
        7:5     R/W    CORE6_TUNE_PWM_R2                       3'h0
        10:8    R/W    CORE6_TUNE_PWM_R1                       3'h0
        13:11   R/W    CORE6_TUNE_PWM_C3                       3'h0
        16:14   R/W    CORE6_TUNE_PWM_C2                       3'h0
        19:17   R/W    CORE6_TUNE_PWM_C1                       3'h0
        20      R/W    CORE6_BYPASS_PWM_BYPASS_RoughSS         1'h0
        22:21   R/W    CORE6_BYPASS_PWM_TUNE_RoughSS           2'h0
        25:23   R/W    CORE6_BYPASS_PWM_TUNE_VCL               3'h0
        28:26   R/W    CORE6_BYPASS_PWM_TUNE_VCH               3'h0
        29      R/W    CORE6_X4_PWM_COMP_IB                    1'h0
        30      R/W    CORE6_X4_POW_PWM_CLP                    1'h0
        31      R/W    CORE6_X4_TUNE_VDIV_Bit0                 1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR6_31_0;
        struct
        {
            __IO uint32_t CORE6_TUNE_OCP_RES: 2;
            __IO uint32_t CORE6_TUNE_PWM_R3: 3;
            __IO uint32_t CORE6_TUNE_PWM_R2: 3;
            __IO uint32_t CORE6_TUNE_PWM_R1: 3;
            __IO uint32_t CORE6_TUNE_PWM_C3: 3;
            __IO uint32_t CORE6_TUNE_PWM_C2: 3;
            __IO uint32_t CORE6_TUNE_PWM_C1: 3;
            __IO uint32_t CORE6_BYPASS_PWM_BYPASS_RoughSS: 1;
            __IO uint32_t CORE6_BYPASS_PWM_TUNE_RoughSS: 2;
            __IO uint32_t CORE6_BYPASS_PWM_TUNE_VCL: 3;
            __IO uint32_t CORE6_BYPASS_PWM_TUNE_VCH: 3;
            __IO uint32_t CORE6_X4_PWM_COMP_IB: 1;
            __IO uint32_t CORE6_X4_POW_PWM_CLP: 1;
            __IO uint32_t CORE6_X4_TUNE_VDIV_Bit0: 1;
        } BITS_2C0;
    } u_2C0;

    /* 0x02C4       0x4000_02c4
        6:0     R/W    CORE6_X4_TUNE_VDIV_Bit7_Bit1            7'h0
        14:7    R/W    CORE6_BYPASS_PWM_TUNE_POS_VREFPFM       8'h0
        17:15   R/W    CORE6_BYPASS_PWM_TUNE_POS_VREFOCP       3'h0
        18      R/W    CORE6_FPWM                              1'h0
        19      R/W    CORE6_POW_PFM                           1'h0
        20      R/W    CORE6_POW_PWM                           1'h0
        21      R/W    CORE6_POW_VDIV                          1'h0
        23:22   R/W    CORE6_XTAL_OV_RATIO                     2'h0
        26:24   R/W    CORE6_XTAL_OV_UNIT                      3'h0
        29:27   R/W    CORE6_XTAL_OV_MODE                      3'h0
        30      R/W    CORE6_EN_POWERMOS_DR8X                  1'h0
        31      R/W    CORE6_SEL_OCP_TABLE                     1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR6_63_32;
        struct
        {
            __IO uint32_t CORE6_X4_TUNE_VDIV_Bit7_Bit1: 7;
            __IO uint32_t CORE6_BYPASS_PWM_TUNE_POS_VREFPFM: 8;
            __IO uint32_t CORE6_BYPASS_PWM_TUNE_POS_VREFOCP: 3;
            __IO uint32_t CORE6_FPWM: 1;
            __IO uint32_t CORE6_POW_PFM: 1;
            __IO uint32_t CORE6_POW_PWM: 1;
            __IO uint32_t CORE6_POW_VDIV: 1;
            __IO uint32_t CORE6_XTAL_OV_RATIO: 2;
            __IO uint32_t CORE6_XTAL_OV_UNIT: 3;
            __IO uint32_t CORE6_XTAL_OV_MODE: 3;
            __IO uint32_t CORE6_EN_POWERMOS_DR8X: 1;
            __IO uint32_t CORE6_SEL_OCP_TABLE: 1;
        } BITS_2C4;
    } u_2C4;

    /* 0x02C8       0x4000_02c8
        1:0     R/W    CORE7_TUNE_OCP_RES                      2'h0
        4:2     R/W    CORE7_TUNE_PWM_R3                       3'h0
        7:5     R/W    CORE7_TUNE_PWM_R2                       3'h0
        10:8    R/W    CORE7_TUNE_PWM_R1                       3'h0
        13:11   R/W    CORE7_TUNE_PWM_C3                       3'h0
        16:14   R/W    CORE7_TUNE_PWM_C2                       3'h0
        19:17   R/W    CORE7_TUNE_PWM_C1                       3'h0
        20      R/W    CORE7_BYPASS_PWM_BYPASS_RoughSS         1'h0
        22:21   R/W    CORE7_BYPASS_PWM_TUNE_RoughSS           2'h0
        25:23   R/W    CORE7_BYPASS_PWM_TUNE_VCL               3'h0
        28:26   R/W    CORE7_BYPASS_PWM_TUNE_VCH               3'h0
        29      R/W    CORE7_X4_PWM_COMP_IB                    1'h0
        30      R/W    CORE7_X4_POW_PWM_CLP                    1'h0
        31      R/W    CORE7_X4_TUNE_VDIV_Bit0                 1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR7_31_0;
        struct
        {
            __IO uint32_t CORE7_TUNE_OCP_RES: 2;
            __IO uint32_t CORE7_TUNE_PWM_R3: 3;
            __IO uint32_t CORE7_TUNE_PWM_R2: 3;
            __IO uint32_t CORE7_TUNE_PWM_R1: 3;
            __IO uint32_t CORE7_TUNE_PWM_C3: 3;
            __IO uint32_t CORE7_TUNE_PWM_C2: 3;
            __IO uint32_t CORE7_TUNE_PWM_C1: 3;
            __IO uint32_t CORE7_BYPASS_PWM_BYPASS_RoughSS: 1;
            __IO uint32_t CORE7_BYPASS_PWM_TUNE_RoughSS: 2;
            __IO uint32_t CORE7_BYPASS_PWM_TUNE_VCL: 3;
            __IO uint32_t CORE7_BYPASS_PWM_TUNE_VCH: 3;
            __IO uint32_t CORE7_X4_PWM_COMP_IB: 1;
            __IO uint32_t CORE7_X4_POW_PWM_CLP: 1;
            __IO uint32_t CORE7_X4_TUNE_VDIV_Bit0: 1;
        } BITS_2C8;
    } u_2C8;

    /* 0x02CC       0x4000_02cc
        6:0     R/W    CORE7_X4_TUNE_VDIV_Bit7_Bit1            7'h0
        14:7    R/W    CORE7_BYPASS_PWM_TUNE_POS_VREFPFM       8'h0
        17:15   R/W    CORE7_BYPASS_PWM_TUNE_POS_VREFOCP       3'h0
        18      R/W    CORE7_FPWM                              1'h0
        19      R/W    CORE7_POW_PFM                           1'h0
        20      R/W    CORE7_POW_PWM                           1'h0
        21      R/W    CORE7_POW_VDIV                          1'h0
        23:22   R/W    CORE7_XTAL_OV_RATIO                     2'h0
        26:24   R/W    CORE7_XTAL_OV_UNIT                      3'h0
        29:27   R/W    CORE7_XTAL_OV_MODE                      3'h0
        30      R/W    CORE7_EN_POWERMOS_DR8X                  1'h0
        31      R/W    CORE7_SEL_OCP_TABLE                     1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR7_63_32;
        struct
        {
            __IO uint32_t CORE7_X4_TUNE_VDIV_Bit7_Bit1: 7;
            __IO uint32_t CORE7_BYPASS_PWM_TUNE_POS_VREFPFM: 8;
            __IO uint32_t CORE7_BYPASS_PWM_TUNE_POS_VREFOCP: 3;
            __IO uint32_t CORE7_FPWM: 1;
            __IO uint32_t CORE7_POW_PFM: 1;
            __IO uint32_t CORE7_POW_PWM: 1;
            __IO uint32_t CORE7_POW_VDIV: 1;
            __IO uint32_t CORE7_XTAL_OV_RATIO: 2;
            __IO uint32_t CORE7_XTAL_OV_UNIT: 3;
            __IO uint32_t CORE7_XTAL_OV_MODE: 3;
            __IO uint32_t CORE7_EN_POWERMOS_DR8X: 1;
            __IO uint32_t CORE7_SEL_OCP_TABLE: 1;
        } BITS_2CC;
    } u_2CC;

    /* 0x02D0       0x4000_02d0
        3:0     R/W    flash1_div_sel                          4'h0
        4       R/W    flash1_div_en                           1'h0
        5       R/W    FLASH1_CLK_SRC_EN                       1'h0
        6       R/W    flash1_clk_src_sel_1                    1'h0
        7       R/W    flash1_clk_src_sel_0                    1'h0
        8       R/W    flash1_mux_1_clk_cg_en                  1'h0
        12:9    R/W    flash2_div_sel                          4'h0
        13      R/W    flash2_div_en                           1'h0
        14      R/W    FLASH2_CLK_SRC_EN                       1'h0
        15      R/W    flash2_clk_src_sel_1                    1'h0
        16      R/W    flash2_clk_src_sel_0                    1'h0
        17      R/W    flash2_mux_1_clk_cg_en                  1'h0
        26:18   R/W    RESERVED02B4_26_18                      9'h0
        27      R/W    r_flash1_clk_src_pll_sel                1'h0
        28      R/W    r_flash2_clk_src_pll_sel                1'h0
        31:29   R/W    RESERVED02B4_31_29                      3'h0
    */
    union
    {
        __IO uint32_t REG_0x2D0;
        struct
        {
            __IO uint32_t flash1_div_sel: 4;
            __IO uint32_t flash1_div_en: 1;
            __IO uint32_t FLASH1_CLK_SRC_EN: 1;
            __IO uint32_t flash1_clk_src_sel_1: 1;
            __IO uint32_t flash1_clk_src_sel_0: 1;
            __IO uint32_t flash1_mux_1_clk_cg_en: 1;
            __IO uint32_t flash2_div_sel: 4;
            __IO uint32_t flash2_div_en: 1;
            __IO uint32_t FLASH2_CLK_SRC_EN: 1;
            __IO uint32_t flash2_clk_src_sel_1: 1;
            __IO uint32_t flash2_clk_src_sel_0: 1;
            __IO uint32_t flash2_mux_1_clk_cg_en: 1;
            __IO uint32_t RESERVED02B4_26_18: 9;
            __IO uint32_t r_flash1_clk_src_pll_sel: 1;
            __IO uint32_t r_flash2_clk_src_pll_sel: 1;
            __IO uint32_t RESERVED02B4_31_29: 3;
        } BITS_2D0;
    } u_2D0;

    /* 0x02D4       0x4000_02d4
        31:0    R/W    RESERVED02D4_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x2D4;
        struct
        {
            __IO uint32_t RESERVED02D4_31_00: 32;
        } BITS_2D4;
    } u_2D4;

    /* 0x02D8       0x4000_02d8
        31:0    R      RESERVED02D8_31_00                      32'hEAEAEAEA
    */
    union
    {
        __IO uint32_t REG_0x02D8;
        struct
        {
            __I uint32_t RESERVED02D8_31_00: 32;
        } BITS_2D8;
    } u_2D8;

    /* 0x02DC       0x4000_02dc
        0       R/W    CORE5_EN_HVD17_LOWIQ                    1'h0
        4:1     R/W    CORE5_TUNE_HVD17_IB                     4'h0
        5       R/W    CORE5_X4_PFM_COMP_IB                    1'h0
        8:6     R/W    CORE5_TUNE_PFM_VREFOCPPFM               3'h0
        14:9    R/W    CORE5_TUNE_SAW_ICLK                     6'h0
        15      R/W    RESERVED02DC_15                         1'h0
        16      R/W    CORE6_EN_HVD17_LOWIQ                    1'h0
        20:17   R/W    CORE6_TUNE_HVD17_IB                     4'h0
        21      R/W    CORE6_X4_PFM_COMP_IB                    1'h0
        24:22   R/W    CORE6_TUNE_PFM_VREFOCPPFM               3'h0
        30:25   R/W    CORE6_TUNE_SAW_ICLK                     6'h0
        31      R/W    RESERVED02DC_31                         1'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR6_79_64_AUTO_SW_PAR5_79_64;
        struct
        {
            __IO uint32_t CORE5_EN_HVD17_LOWIQ: 1;
            __IO uint32_t CORE5_TUNE_HVD17_IB: 4;
            __IO uint32_t CORE5_X4_PFM_COMP_IB: 1;
            __IO uint32_t CORE5_TUNE_PFM_VREFOCPPFM: 3;
            __IO uint32_t CORE5_TUNE_SAW_ICLK: 6;
            __IO uint32_t RESERVED02DC_15: 1;
            __IO uint32_t CORE6_EN_HVD17_LOWIQ: 1;
            __IO uint32_t CORE6_TUNE_HVD17_IB: 4;
            __IO uint32_t CORE6_X4_PFM_COMP_IB: 1;
            __IO uint32_t CORE6_TUNE_PFM_VREFOCPPFM: 3;
            __IO uint32_t CORE6_TUNE_SAW_ICLK: 6;
            __IO uint32_t RESERVED02DC_31: 1;
        } BITS_2DC;
    } u_2DC;

    /* 0x02E0       0x4000_02e0
        31:0    R/W    RESERVED02E0_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x02E0;
        struct
        {
            __IO uint32_t RESERVED02E0_31_00: 32;
        } BITS_2E0;
    } u_2E0;

    /* 0x02E4       0x4000_02e4
        31:0    R/W    RESERVED02E4_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x02E4;
        struct
        {
            __IO uint32_t RESERVED02E4_31_00: 32;
        } BITS_2E4;
    } u_2E4;

    /* 0x02E8       0x4000_02e8
        31:0    R/W    RESERVED02E8_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x02E8;
        struct
        {
            __IO uint32_t RESERVED02E8_31_00: 32;
        } BITS_2E8;
    } u_2E8;

    /* 0x02EC       0x4000_02ec
        31:0    R/W    RESERVED02EC_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x02EC;
        struct
        {
            __IO uint32_t RESERVED02EC_31_00: 32;
        } BITS_2EC;
    } u_2EC;

    /* 0x02F0       0x4000_02f0
        31:0    R/W    RESERVED02F0_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x02F0;
        struct
        {
            __IO uint32_t RESERVED02F0_31_00: 32;
        } BITS_2F0;
    } u_2F0;

    /* 0x02F4       0x4000_02f4
        31:0    R/W    RESERVED02F4_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x02F4;
        struct
        {
            __IO uint32_t RESERVED02F4_31_00: 32;
        } BITS_2F4;
    } u_2F4;

    /* 0x02F8       0x4000_02f8
        31:0    R/W    RESERVED02F8_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x02F8;
        struct
        {
            __IO uint32_t RESERVED02F8_31_00: 32;
        } BITS_2F8;
    } u_2F8;

    /* 0x02FC       0x4000_02fc
        31:0    R/W    RESERVED02FC_31_00                      32'h0
    */
    union
    {
        __IO uint32_t REG_0x02FC;
        struct
        {
            __IO uint32_t RESERVED02FC_31_00: 32;
        } BITS_2FC;
    } u_2FC;

    /* 0x0300       0x4000_0300
        7:0     R/W    PON_PERI_DLYSEL_SPIM                    8'h0
        15:8    R/W    PON_PERI_DLYSEL_SPIM1                   8'h0
        23:16   R/W    PON_PERI_DLYSEL_SPIM2                   8'h0
        31:24   R/W    RESERVED0300_31_24                      8'h0
    */
    union
    {
        __IO uint32_t PON_PERI_DLYSEL_CTRL;
        struct
        {
            __IO uint32_t PON_PERI_DLYSEL_SPIM: 8;
            __IO uint32_t PON_PERI_DLYSEL_SPIM1: 8;
            __IO uint32_t PON_PERI_DLYSEL_SPIM2: 8;
            __IO uint32_t RESERVED0300_31_24: 8;
        } BITS_300;
    } u_300;

    /* 0x0304       0x4000_0304
        31:0    R/W    RESERVED0304_31_00                      32'h1FC00001
    */
    union
    {
        __IO uint32_t REG_0x0304;
        struct
        {
            __IO uint32_t RESERVED0304_31_00: 32;
        } BITS_304;
    } u_304;

    /* 0x0308       0x4000_0308
        0       R/W    PON_SPI0_MST                            1'h0
        1       R/W    PON_SPI0_BRIDGE_EN                      1'h0
        2       R/W    PON_SPI1_BRIDGE_EN                      1'h0
        3       R/W    PON_SPI2_BRIDGE_EN                      1'h0
        4       R/W    PON_SPI0_H2S_BRG_EN                     1'h0
        31:5    R/W    RESERVED0308_31_05                      27'h0
    */
    union
    {
        __IO uint32_t REG_0x308;
        struct
        {
            __IO uint32_t PON_SPI0_MST: 1;
            __IO uint32_t PON_SPI0_BRIDGE_EN: 1;
            __IO uint32_t PON_SPI1_BRIDGE_EN: 1;
            __IO uint32_t PON_SPI2_BRIDGE_EN: 1;
            __IO uint32_t PON_SPI0_H2S_BRG_EN: 1;
            __IO uint32_t RESERVED0308_31_05: 27;
        } BITS_308;
    } u_308;

    /* 0x030C       0x4000_030c
        31:0    R      RESERVED030C_31_00                      32'hEAEAEAEA
    */
    union
    {
        __IO uint32_t REG_0x030C;
        struct
        {
            __I uint32_t RESERVED030C_31_00: 32;
        } BITS_30C;
    } u_30C;

    /* 0x0310       0x4000_0310
        31:0    R      RESERVED0310_31_00                      32'hEAEAEAEA
    */
    union
    {
        __IO uint32_t REG_0x0310;
        struct
        {
            __I uint32_t RESERVED0310_31_00: 32;
        } BITS_310;
    } u_310;

    /* 0x0314       0x4000_0314
        31:0    R      RESERVED0314_31_00                      32'hEAEAEAEA
    */
    union
    {
        __IO uint32_t REG_0x0314;
        struct
        {
            __I uint32_t RESERVED0314_31_00: 32;
        } BITS_314;
    } u_314;

    /* 0x0318       0x4000_0318
        23:0    R      RESERVED0318_23_00                      24'h0
        31:24   R/W    RESERVED0318_31_24                      8'h0
    */
    union
    {
        __IO uint32_t CPU_INTER_ENABLE;
        struct
        {
            __I uint32_t RESERVED0318_23_00: 24;
            __IO uint32_t RESERVED0318_31_24: 8;
        } BITS_318;
    } u_318;

    /* 0x031C       0x4000_031c
        31:0    R      RESERVED031C_31_00                      32'hEAEAEAEA
    */
    union
    {
        __IO uint32_t REG_0x031C;
        struct
        {
            __I uint32_t RESERVED031C_31_00: 32;
        } BITS_31C;
    } u_31C;

    /* 0x0320       0x4000_0320
        31:0    R/W    RESERVED0320_31_00                      32'he0342a43
    */
    union
    {
        __IO uint32_t REG_0x0320;
        struct
        {
            __IO uint32_t RESERVED0320_31_00: 32;
        } BITS_320;
    } u_320;

    /* 0x0324       0x4000_0324
        31:0    R/W    RESERVED0324_31_00                      32'h2419e6ab
    */
    union
    {
        __IO uint32_t REG_0x0324;
        struct
        {
            __IO uint32_t RESERVED0324_31_00: 32;
        } BITS_324;
    } u_324;

    /* 0x0328       0x4000_0328
        31:0    R/W    RESERVED0328_31_00                      32'he0000000
    */
    union
    {
        __IO uint32_t REG_0x0328;
        struct
        {
            __IO uint32_t RESERVED0328_31_00: 32;
        } BITS_328;
    } u_328;

    /* 0x032C       0x4000_032c
        3:0     R/W    OSC40_FSET                              4'h8
        31:4    R/W    RESERVED032C_31_04                      28'h0001090
    */
    union
    {
        __IO uint32_t REG_OSC40_FSET;
        struct
        {
            __IO uint32_t OSC40_FSET: 4;
            __IO uint32_t RESERVED032C_31_04: 28;
        } BITS_32C;
    } u_32C;

    /* 0x0330       0x4000_0330
        0       RW     rst_n_aac                               1'b0
        1       RW     offset_plus                             1'b0
        7:2     RW     XAAC_GM_offset                          6'h0
        8       RW     GM_STEP                                 1'b0
        14:9    RW     GM_INIT                                 6'h3F
        17:15   RW     XTAL_CLK_SET                            3'b101
        23:18   RW     GM_STUP                                 6'h3F
        29:24   RW     GM_MANUAL                               6'h1F
        30      RW     r_EN_XTAL_AAC_DIGI                      1'b0
        31      RW     r_EN_XTAL_AAC_TRIG                      1'b0
    */
    union
    {
        __IO uint32_t REG_0x330;
        struct
        {
            __IO uint32_t rst_n_aac: 1;
            __IO uint32_t offset_plus: 1;
            __IO uint32_t XAAC_GM_offset: 6;
            __IO uint32_t GM_STEP: 1;
            __IO uint32_t GM_INIT: 6;
            __IO uint32_t XTAL_CLK_SET: 3;
            __IO uint32_t GM_STUP: 6;
            __IO uint32_t GM_MANUAL: 6;
            __IO uint32_t r_EN_XTAL_AAC_DIGI: 1;
            __IO uint32_t r_EN_XTAL_AAC_TRIG: 1;
        } BITS_330;
    } u_330;

    /* 0x0334       0x4000_0334
        0       R      XAAC_BUSY                               1'b0
        1       R      XAAC_READY                              1'b0
        7:2     R      XTAL_GM_OUT                             6'h1F
        11:8    R      xaac_curr_state                         4'h0
        12      R/W    EN_XTAL_AAC_GM                          1'b0
        13      R/W    EN_XTAL_AAC_PKDET                       1'b0
        14      R      XTAL_PKDET_OUT                          1'b0
        15      R/W    RESERVED036C_15                         1'b0
        31:16   R/W    RESERVED036C_31_16                      16'h0129
    */
    union
    {
        __IO uint32_t AAC_CTRL_1;
        struct
        {
            __I uint32_t XAAC_BUSY: 1;
            __I uint32_t XAAC_READY: 1;
            __I uint32_t XTAL_GM_OUT: 6;
            __I uint32_t xaac_curr_state: 4;
            __IO uint32_t EN_XTAL_AAC_GM: 1;
            __IO uint32_t EN_XTAL_AAC_PKDET: 1;
            __I uint32_t XTAL_PKDET_OUT: 1;
            __IO uint32_t RESERVED036C_15: 1;
            __IO uint32_t RESERVED036C_31_16: 16;
        } BITS_334;
    } u_334;

    /* 0x0338       0x4000_0338
        0       R/W    disable_pll_pre_gating                  1'b0
        15:1    R/W    RESERVED0338_15_01                      15'h0
        16      R      XTAL32K_OK                              1'h0
        17      R      OSC32K_OK                               1'h0
        22:18   R      XTAL_CTRL_DEBUG_OUT[4:0]                5'h0
        23      R      PLL_CKO2_READY                          1'h0
        24      R      BT_PLL_READY                            1'h0
        25      R      XTAL_OK                                 1'h0
        28:26   R      XTAL_CTRL_DEBUG_OUT[7:5]                3'h0
        31:29   R      XTAL_MODE_O                             3'h0
    */
    union
    {
        __IO uint32_t REG_0x338;
        struct
        {
            __IO uint32_t disable_pll_pre_gating: 1;
            __IO uint32_t RESERVED0338_15_01: 15;
            __I uint32_t XTAL32K_OK: 1;
            __I uint32_t OSC32K_OK: 1;
            __I uint32_t XTAL_CTRL_DEBUG_OUT_4_0: 5;
            __I uint32_t PLL_CKO2_READY: 1;
            __I uint32_t BT_PLL_READY: 1;
            __I uint32_t XTAL_OK: 1;
            __I uint32_t XTAL_CTRL_DEBUG_OUT_7_5: 3;
            __I uint32_t XTAL_MODE_O: 3;
        } BITS_338;
    } u_338;

    /* 0x033C       0x4000_033c
        0       R/W    resetn                                  1'b0
        1       R/W    EN_XTAL_PDCK_DIGI                       1'b0
        2       R/W    PDCK_SEARCH_MODE                        1'b0
        4:3     R/W    PDCK_WAIT_CYC[1:0]                      2'b01
        9:5     R/W    VREF_MANUAL[4:0]                        5'h1F
        14:10   R/W    VREF_INIT[4:0]                          5'h1F
        16:15   R/W    XTAL_PDCK_UNIT[1:0]                     2'b01
        21:17   R/W    XPDCK_VREF_SEL[4:0]                     5'h2
        22      R/W    PDCK_LPOW                               1'b0
        27:23   R/W    RESERVED033C_27_23                      5'h0
        31:28   R      pdck_state[3:0]                         4'h0
    */
    union
    {
        __IO uint32_t XTAL_PDCK;
        struct
        {
            __IO uint32_t resetn: 1;
            __IO uint32_t EN_XTAL_PDCK_DIGI: 1;
            __IO uint32_t PDCK_SEARCH_MODE: 1;
            __IO uint32_t PDCK_WAIT_CYC_1_0: 2;
            __IO uint32_t VREF_MANUAL_4_0: 5;
            __IO uint32_t VREF_INIT_4_0: 5;
            __IO uint32_t XTAL_PDCK_UNIT_1_0: 2;
            __IO uint32_t XPDCK_VREF_SEL_4_0: 5;
            __IO uint32_t PDCK_LPOW: 1;
            __IO uint32_t RESERVED033C_27_23: 5;
            __I uint32_t pdck_state_3_0: 4;
        } BITS_33C;
    } u_33C;

    /* 0x0340       0x4000_0340
        31:0    R/W    RESERVED0340_31_00                      32'hAAAAAAAA
    */
    union
    {
        __IO uint32_t USB_ISO_INT_CTRL;
        struct
        {
            __IO uint32_t RESERVED0340_31_00: 32;
        } BITS_340;
    } u_340;

    /* 0x0344       0x4000_0344
        12:0    R/W    GPIO_DBNC_CTRL                          13'h101
        15:13   R/W    RESERVED0344_15_13                      3'h0
        28:16   R/W    GPIO1_DBNC_CTRL                         13'h101
        31:29   R/W    RESERVED0344_31_29                      3'h0
    */
    union
    {
        __IO uint32_t REG_0x344;
        struct
        {
            __IO uint32_t GPIO_DBNC_CTRL: 13;
            __IO uint32_t RESERVED0344_15_13: 3;
            __IO uint32_t GPIO1_DBNC_CTRL: 13;
            __IO uint32_t RESERVED0344_31_29: 3;
        } BITS_344;
    } u_344;

    /* 0x0348       0x4000_0348
        2:0     R/W    r_timer_div_sel                         3'h0
        3       R/W    r_timer_div_en                          1'b0
        4       R/W    r_timer_cg_en                           1'b0
        5       R/W    r_timer_clk_src_sel_0                   1'h1
        6       R/W    r_timer_clk_src_sel_1                   1'b0
        7       R/W    r_timer_mux_1_clk_cg_en                 1'b0
        8       R/W    timer_clk_src_pll_sel                   1'b0
        9       R/W    r_irrc_clk_sel                          1'b0
        10      R/W    r_irrc_clk_src_pll_sel                  1'b0
        31:11   R      RESERVED0348_31_11                      21'h0
    */
    union
    {
        __IO uint32_t REG_0x348;
        struct
        {
            __IO uint32_t r_timer_div_sel: 3;
            __IO uint32_t r_timer_div_en: 1;
            __IO uint32_t r_timer_cg_en: 1;
            __IO uint32_t r_timer_clk_src_sel_0: 1;
            __IO uint32_t r_timer_clk_src_sel_1: 1;
            __IO uint32_t r_timer_mux_1_clk_cg_en: 1;
            __IO uint32_t timer_clk_src_pll_sel: 1;
            __IO uint32_t r_irrc_clk_sel: 1;
            __IO uint32_t r_irrc_clk_src_pll_sel: 1;
            __I uint32_t RESERVED0348_31_11: 21;
        } BITS_348;
    } u_348;

    __IO uint32_t RSVD_0x34c[4];

    /* 0x035C       0x4000_035c
        2:0     R/W    BIT_PERI_GT5_CLK_DIV                    3'h0
        5:3     R/W    BIT_PERI_GT6_CLK_DIV                    3'h0
        8:6     R/W    BIT_PERI_GT7_CLK_DIV                    3'h0
        10:9    R/W    BIT_PERI_UART0_CLK_DIV                  2'h0
        12:11   R/W    BIT_PERI_UART1_CLK_DIV                  2'h0
        14:13   R/W    BIT_PERI_UART2_CLK_DIV                  2'h0
        16:15   R/W    BIT_PERI_I2C0_CLK_DIV                   2'h0
        18:17   R/W    BIT_PERI_I2C1_CLK_DIV                   2'h0
        21:19   R/W    BIT_PERI_SPI0_CLK_DIV                   3'h0
        24:22   R/W    BIT_PERI_SPI1_CLK_DIV                   3'h0
        27:25   R/W    BIT_PERI_SPI2_CLK_DIV                   3'h0
        29:28   R/W    BIT_PERI_I2C2_CLK_DIV                   2'h0
        30      R/W    r_spi0_clk_src_pll_sel                  1'b0
        31      R/W    RESERVED035C_31                         1'b0
    */
    union
    {
        __IO uint32_t REG_PERI_GTIMER_CLK_SRC1;
        struct
        {
            __IO uint32_t BIT_PERI_GT5_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_GT6_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_GT7_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_UART0_CLK_DIV: 2;
            __IO uint32_t BIT_PERI_UART1_CLK_DIV: 2;
            __IO uint32_t BIT_PERI_UART2_CLK_DIV: 2;
            __IO uint32_t BIT_PERI_I2C0_CLK_DIV: 2;
            __IO uint32_t BIT_PERI_I2C1_CLK_DIV: 2;
            __IO uint32_t BIT_PERI_SPI0_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_SPI1_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_SPI2_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_I2C2_CLK_DIV: 2;
            __IO uint32_t r_spi0_clk_src_pll_sel: 1;
            __IO uint32_t RESERVED035C_31: 1;
        } BITS_35C;
    } u_35C;

    /* 0x0360       0x4000_0360
        7:0     R/W    RESERVED0360_07_00                      8'h0
        8       R/W    BIT_TIMER_CLK_32K_EN                    1'h1
        9       R/W    BIT_TIMER_CLK_f40M_EN                   1'h1
        10      R/W    timer_apb_clk_disable                   1'h0
        11      R/W    r_timer_div1_en                         1'h0
        12      R/W    r_clk_timer_f1m_en                      1'h0
        13      R/W    r_timer38_div_en                        1'h0
        15:14   R/W    RESERVED0360_15_14                      2'h0
        18:16   R/W    BIT_PERI_GT0_CLK_DIV                    3'h0
        21:19   R/W    BIT_PERI_GT1_CLK_DIV                    3'h0
        24:22   R/W    BIT_PERI_GT2_CLK_DIV                    3'h0
        27:25   R/W    BIT_PERI_GT3_CLK_DIV                    3'h0
        30:28   R/W    BIT_PERI_GT4_CLK_DIV                    3'h0
        31      R/W    RESERVED0360_31                         1'h0
    */
    union
    {
        __IO uint32_t REG_PERI_GTIMER_CLK_SRC0;
        struct
        {
            __IO uint32_t RESERVED0360_07_00: 8;
            __IO uint32_t BIT_TIMER_CLK_32K_EN: 1;
            __IO uint32_t BIT_TIMER_CLK_f40M_EN: 1;
            __IO uint32_t timer_apb_clk_disable: 1;
            __IO uint32_t r_timer_div1_en: 1;
            __IO uint32_t r_clk_timer_f1m_en: 1;
            __IO uint32_t r_timer38_div_en: 1;
            __IO uint32_t RESERVED0360_15_14: 2;
            __IO uint32_t BIT_PERI_GT0_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_GT1_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_GT2_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_GT3_CLK_DIV: 3;
            __IO uint32_t BIT_PERI_GT4_CLK_DIV: 3;
            __IO uint32_t RESERVED0360_31: 1;
        } BITS_360;
    } u_360;

    /* 0x0364       0x4000_0364
        15:0    R/W    TIMER_PWM0_CTRL                         16'h0
        31:16   R/W    RESERVED0364_31_16                      16'h0
    */
    union
    {
        __IO uint32_t REG_PERI_PWM2_DZONE_CTRL;
        struct
        {
            __IO uint32_t TIMER_PWM0_CTRL: 16;
            __IO uint32_t RESERVED0364_31_16: 16;
        } BITS_364;
    } u_364;

    /* 0x0368       0x4000_0368
        15:0    R/W    TIMER_PWM1_CTRL                         16'h0
        31:16   R/W    RESERVED0368_31_16                      16'h0
    */
    union
    {
        __IO uint32_t REG_PERI_PWM3_DZONE_CTRL;
        struct
        {
            __IO uint32_t TIMER_PWM1_CTRL: 16;
            __IO uint32_t RESERVED0368_31_16: 16;
        } BITS_368;
    } u_368;

    /* 0x036C       0x4000_036c
        2:0     R/W    etimer_0_div_sel                        3'h0
        3       R/W    etimer_0_div_en                         1'h0
        6:4     R/W    etimer_1_div_sel                        3'h0
        7       R/W    etimer_1_div_en                         1'h0
        10:8    R/W    etimer_2_div_sel                        3'h0
        11      R/W    etimer_2_div_en                         1'h0
        14:12   R/W    etimer_3_div_sel                        3'h0
        15      R/W    etimer_3_div_en                         1'h0
        23:16   R/W    RESERVED036C_23_16                      8'h0
        25:24   R/W    r_can_lp_scan_div_sel                   2'h0
        26      R/W    r_can_scan_div_sel                      1'h0
        31:27   R/W    RESERVED036C_31_27                      5'h0
    */
    union
    {
        __IO uint32_t REG_PERI_ETIMER_CAN_DIV;
        struct
        {
            __IO uint32_t etimer_0_div_sel: 3;
            __IO uint32_t etimer_0_div_en: 1;
            __IO uint32_t etimer_1_div_sel: 3;
            __IO uint32_t etimer_1_div_en: 1;
            __IO uint32_t etimer_2_div_sel: 3;
            __IO uint32_t etimer_2_div_en: 1;
            __IO uint32_t etimer_3_div_sel: 3;
            __IO uint32_t etimer_3_div_en: 1;
            __IO uint32_t RESERVED036C_23_16: 8;
            __IO uint32_t r_can_lp_scan_div_sel: 2;
            __IO uint32_t r_can_scan_div_sel: 1;
            __IO uint32_t RESERVED036C_31_27: 5;
        } BITS_36C;
    } u_36C;

    __IO uint32_t RSVD_0x370[7];

    /* 0x038C       0x4000_038c
        31:0    R/W    SWR_SS_LUT_2                            32'h0
    */
    union
    {
        __IO uint32_t REG_0x38C;
        struct
        {
            __IO uint32_t SWR_SS_LUT_2: 32;
        } BITS_38C;
    } u_38C;

    /* 0x0390       0x4000_0390
        31:0    R/W    SWR_SS_LUT_3                            32'h0
    */
    union
    {
        __IO uint32_t REG_0x390;
        struct
        {
            __IO uint32_t SWR_SS_LUT_3: 32;
        } BITS_390;
    } u_390;

    /* 0x0394       0x4000_0394
        31:0    R/W    SWR_SS_LUT_4                            32'h0
    */
    union
    {
        __IO uint32_t REG_0x394;
        struct
        {
            __IO uint32_t SWR_SS_LUT_4: 32;
        } BITS_394;
    } u_394;

    /* 0x0398       0x4000_0398
        31:0    R/W    SWR_SS_LUT_5                            32'h0
    */
    union
    {
        __IO uint32_t REG_0x398;
        struct
        {
            __IO uint32_t SWR_SS_LUT_5: 32;
        } BITS_398;
    } u_398;

    /* 0x039C       0x4000_039c
        15:0    R/W    SWR_SS_CONFIG                           16'h0
        31:16   R/W    RESERVED039C_31_16                      16'h0
    */
    union
    {
        __IO uint32_t REG_0x39C;
        struct
        {
            __IO uint32_t SWR_SS_CONFIG: 16;
            __IO uint32_t RESERVED039C_31_16: 16;
        } BITS_39C;
    } u_39C;

    /* 0x03A0       0x4000_03a0
        31:0    R/W    SWR_SS_LUT_0                            32'h0
    */
    union
    {
        __IO uint32_t REG_0x3A0;
        struct
        {
            __IO uint32_t SWR_SS_LUT_0: 32;
        } BITS_3A0;
    } u_3A0;

    /* 0x03A4       0x4000_03a4
        31:0    R/W    SWR_SS_LUT_1                            32'h0
    */
    union
    {
        __IO uint32_t REG_0x3A4;
        struct
        {
            __IO uint32_t SWR_SS_LUT_1: 32;
        } BITS_3A4;
    } u_3A4;

    __IO uint32_t RSVD_0x3a8[2];

    /* 0x03B0       0x4000_03b0
        7:0     R/W    PMUX_GPIO_P4_0                          8'h0
        15:8    R/W    PMUX_GPIO_P4_1                          8'h0
        23:16   R/W    PMUX_GPIO_P4_2                          8'h0
        31:24   R/W    PMUX_GPIO_P4_3                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO4_2_4_7;
        struct
        {
            __IO uint32_t PMUX_GPIO_P4_0: 8;
            __IO uint32_t PMUX_GPIO_P4_1: 8;
            __IO uint32_t PMUX_GPIO_P4_2: 8;
            __IO uint32_t PMUX_GPIO_P4_3: 8;
        } BITS_3B0;
    } u_3B0;

    /* 0x03B4       0x4000_03b4
        7:0     R/W    PMUX_GPIO_P4_4                          8'h0
        15:8    R/W    PMUX_GPIO_P4_5                          8'h0
        23:16   R/W    PMUX_GPIO_P4_6                          8'h0
        31:24   R/W    PMUX_GPIO_P4_7                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIOC4_7;
        struct
        {
            __IO uint32_t PMUX_GPIO_P4_4: 8;
            __IO uint32_t PMUX_GPIO_P4_5: 8;
            __IO uint32_t PMUX_GPIO_P4_6: 8;
            __IO uint32_t PMUX_GPIO_P4_7: 8;
        } BITS_3B4;
    } u_3B4;

    /* 0x03B8       0x4000_03b8
        7:0     R/W    PMUX_GPIO_P5_0                          8'h0
        15:8    R/W    PMUX_GPIO_P5_1                          8'h0
        23:16   R/W    PMUX_GPIO_P5_2                          8'h0
        31:24   R/W    PMUX_GPIO_P5_3                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO5_0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_P5_0: 8;
            __IO uint32_t PMUX_GPIO_P5_1: 8;
            __IO uint32_t PMUX_GPIO_P5_2: 8;
            __IO uint32_t PMUX_GPIO_P5_3: 8;
        } BITS_3B8;
    } u_3B8;

    /* 0x03BC       0x4000_03bc
        7:0     R/W    PMUX_GPIO_P5_4                          8'h0
        15:8    R/W    PMUX_GPIO_P5_5                          8'h0
        23:16   R/W    PMUX_GPIO_P5_6                          8'h0
        31:24   R/W    RESERVED03BC_31_24                      8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO5_4_7;
        struct
        {
            __IO uint32_t PMUX_GPIO_P5_4: 8;
            __IO uint32_t PMUX_GPIO_P5_5: 8;
            __IO uint32_t PMUX_GPIO_P5_6: 8;
            __IO uint32_t RESERVED03BC_31_24: 8;
        } BITS_3BC;
    } u_3BC;

    /* 0x03C0       0x4000_03c0
        7:0     R/W    PMUX_GPIO_P6_0                          8'h0
        15:8    R/W    PMUX_GPIO_P6_1                          8'h0
        23:16   R/W    PMUX_GPIO_P6_2                          8'h0
        31:24   R/W    PMUX_GPIO_P6_3                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO6_0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_P6_0: 8;
            __IO uint32_t PMUX_GPIO_P6_1: 8;
            __IO uint32_t PMUX_GPIO_P6_2: 8;
            __IO uint32_t PMUX_GPIO_P6_3: 8;
        } BITS_3C0;
    } u_3C0;

    /* 0x03C4       0x4000_03c4
        7:0     R/W    PMUX_GPIO_P6_4                          8'h0
        15:8    R/W    PMUX_GPIO_P6_5                          8'h0
        23:16   R/W    PMUX_GPIO_P6_6                          8'h0
        31:24   R/W    RESERVED03C4_31_24                      8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO6_4_6;
        struct
        {
            __IO uint32_t PMUX_GPIO_P6_4: 8;
            __IO uint32_t PMUX_GPIO_P6_5: 8;
            __IO uint32_t PMUX_GPIO_P6_6: 8;
            __IO uint32_t RESERVED03C4_31_24: 8;
        } BITS_3C4;
    } u_3C4;

    /* 0x03C8       0x4000_03c8
        7:0     R/W    PMUX_GPIO_P7_0                          8'h0
        15:8    R/W    PMUX_GPIO_P7_1                          8'h0
        23:16   R/W    PMUX_GPIO_P7_2                          8'h0
        31:24   R/W    PMUX_GPIO_P7_3                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO7_0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_P7_0: 8;
            __IO uint32_t PMUX_GPIO_P7_1: 8;
            __IO uint32_t PMUX_GPIO_P7_2: 8;
            __IO uint32_t PMUX_GPIO_P7_3: 8;
        } BITS_3C8;
    } u_3C8;

    /* 0x03CC       0x4000_03cc
        7:0     R/W    PMUX_GPIO_P7_4                          8'h0
        15:8    R/W    PMUX_GPIO_P7_5                          8'h0
        23:16   R/W    PMUX_GPIO_P7_6                          8'h0
        31:24   R/W    RESERVED03CC_31_24                      8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO7_4_6;
        struct
        {
            __IO uint32_t PMUX_GPIO_P7_4: 8;
            __IO uint32_t PMUX_GPIO_P7_5: 8;
            __IO uint32_t PMUX_GPIO_P7_6: 8;
            __IO uint32_t RESERVED03CC_31_24: 8;
        } BITS_3CC;
    } u_3CC;

    /* 0x03D0       0x4000_03d0
        7:0     R/W    PMUX_GPIO_P8_0                          8'h0
        15:8    R/W    PMUX_GPIO_P8_1                          8'h0
        23:16   R/W    PMUX_GPIO_P8_2                          8'h0
        31:24   R/W    PMUX_GPIO_P8_3                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO8_0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_P8_0: 8;
            __IO uint32_t PMUX_GPIO_P8_1: 8;
            __IO uint32_t PMUX_GPIO_P8_2: 8;
            __IO uint32_t PMUX_GPIO_P8_3: 8;
        } BITS_3D0;
    } u_3D0;

    /* 0x03D4       0x4000_03d4
        7:0     R/W    PMUX_GPIO_P8_4                          8'h0
        15:8    R/W    PMUX_GPIO_P8_5                          8'h0
        23:16   R/W    PMUX_GPIO_P8_6                          8'h0
        31:24   R/W    PMUX_GPIO_P8_7                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO5_4_7;
        struct
        {
            __IO uint32_t PMUX_GPIO_P8_4: 8;
            __IO uint32_t PMUX_GPIO_P8_5: 8;
            __IO uint32_t PMUX_GPIO_P8_6: 8;
            __IO uint32_t PMUX_GPIO_P8_7: 8;
        } BITS_3D4;
    } u_3D4;

    /* 0x03D8       0x4000_03d8
        7:0     R/W    PMUX_GPIO_P9_0                          8'h0
        15:8    R/W    PMUX_GPIO_P9_1                          8'h0
        23:16   R/W    PMUX_GPIO_P9_2                          8'h0
        31:24   R/W    PMUX_GPIO_P9_3                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO9_0_3;
        struct
        {
            __IO uint32_t PMUX_GPIO_P9_0: 8;
            __IO uint32_t PMUX_GPIO_P9_1: 8;
            __IO uint32_t PMUX_GPIO_P9_2: 8;
            __IO uint32_t PMUX_GPIO_P9_3: 8;
        } BITS_3D8;
    } u_3D8;

    /* 0x03DC       0x4000_03dc
        7:0     R/W    PMUX_GPIO_P9_4                          8'h0
        15:8    R/W    PMUX_GPIO_P9_5                          8'h0
        23:16   R/W    RESERVED03DC_23_16                      8'h0
        31:24   R/W    PMUX_GPIO_P9_6                          8'h0
    */
    union
    {
        __IO uint32_t REG_GPIO9_4_5;
        struct
        {
            __IO uint32_t PMUX_GPIO_P9_4: 8;
            __IO uint32_t PMUX_GPIO_P9_5: 8;
            __IO uint32_t RESERVED03DC_23_16: 8;
            __IO uint32_t PMUX_GPIO_P9_6: 8;
        } BITS_3DC;
    } u_3DC;

    __IO uint32_t RSVD_0x3e0[6];

    /* 0x03F8       0x4000_03f8
        0       R/W    CORE7_EN_HVD17_LOWIQ                    1'h0
        4:1     R/W    CORE7_TUNE_HVD17_IB                     4'h0
        5       R/W    CORE7_X4_PFM_COMP_IB                    1'h0
        8:6     R/W    CORE7_TUNE_PFM_VREFOCPPFM               3'h0
        14:9    R/W    CORE7_TUNE_SAW_ICLK                     6'h0
        31:15   R/W    RESERVED03F8_31_15                      17'h0
    */
    union
    {
        __IO uint32_t AUTO_SW_PAR7_79_64;
        struct
        {
            __IO uint32_t CORE7_EN_HVD17_LOWIQ: 1;
            __IO uint32_t CORE7_TUNE_HVD17_IB: 4;
            __IO uint32_t CORE7_X4_PFM_COMP_IB: 1;
            __IO uint32_t CORE7_TUNE_PFM_VREFOCPPFM: 3;
            __IO uint32_t CORE7_TUNE_SAW_ICLK: 6;
            __IO uint32_t RESERVED03F8_31_15: 17;
        } BITS_3F8;
    } u_3F8;
} SYS_BLKCTRL_TypeDef;

/* ================================================================================ */
/* ================                     Pinmux                     ================ */
/* ================================================================================ */

/**
  * @brief Pinmux. (Pinmux)
  */

typedef struct                                      /*!< Pinmux Structure */
{
    __IO uint32_t
    CFG[12];                              /*!<                                            */
} PINMUX_TypeDef;

/* ================================================================================ */
/* ================                     Clock Debounce             ================ */
/* ================================================================================ */

/**
  * @brief Clock Debounce. (Clock Debounce)
  */
typedef struct
{
    union
    {
        __IO uint32_t WORD0;
        struct
        {
            __IO uint32_t dbnc_div_limit: 8;
            __IO uint32_t dbnc_div_sel: 4;
            __IO uint32_t dbnc_div_en: 1;
            __IO uint32_t reserved: 19;
        } BITS_0;
    } u_0;
} DEBOUNCE_CLK_SET_TypeDef;
/**
  * @brief Peripheral. (Peripheral)
  */
/* ================================================================================ */
/* ================                   PERI_ON     0x40000200              ================ */
/* ================================================================================ */
typedef struct                                      /*!< Peripheral Structure */
{
    __IO uint32_t    SYS_CLK_SEL;       /*!< 0x200 */
    __IO uint32_t    SYS_CLK_SEL_2;     /*!< 0x204 */
    __IO uint32_t    SYS_CLK_SEL_3;     /*!< 0x208 */
    __IO uint32_t    SOC_FUNC_EN;       /*!< 0x210 */
    __IO uint32_t    RSVD0;             /*!< 0x214 */
} PHERION_TypeDef;

/* ================================================================================ */
/* ================              Peripheral Interrupt              ================ */
/* ================================================================================ */
#define PERI_IRQ_BIT_MBIAS_MFB_DET_L     BIT0
#define PERI_IRQ_BIT_mailbox_int         BIT1
#define PERI_IRQ_BIT_utmi_suspend_n      BIT2
#define PERI_IRQ_BIT_dig_trda_int_r      BIT3
#define PERI_IRQ_BIT_rng_int             BIT4
#define PERI_IRQ_BIT_psram_intr          BIT5
#define PERI_IRQ_BIT_dig_lpcomp_int_r    BIT6
#define PERI_IRQ_BIT_timer_intr_5        BIT7
#define PERI_IRQ_BIT_timer_intr_6        BIT8
#define PERI_IRQ_BIT_timer_intr_7        BIT9
#define PERI_IRQ_BIT_dig_lpcomp_int      BIT11
#define PERI_IRQ_BIT_MBIAS_VBAT_DET_L    BIT12
#define PERI_IRQ_BIT_MBIAS_ADP_DET_L     BIT13
#define PERI_IRQ_BIT_HW_ASRC_ISR1        BIT14
#define PERI_IRQ_BIT_HW_ASRC_ISR2        BIT15
#define PERI_IRQ_BIT_gpio_intr_31_6      BIT16
#define PERI_IRQ_BIT_dsp_wdt_to_mcu_intr BIT18
#define PERI_IRQ_BIT_flash_pwr_intr      BIT19
#define PERI_IRQ_BIT_sp0_intr_tx         BIT25
#define PERI_IRQ_BIT_sp0_intr_rx         BIT26
#define PERI_IRQ_BIT_sp1_intr_tx         BIT27
#define PERI_IRQ_BIT_sp1_intr_rx         BIT28

typedef enum
{
    TRIGGER_MODE_HGIH_LEVEL,
    TRIGGER_MODE_EDGE,
} TRIGGER_MODE;

typedef enum
{
    EDGE_MODE_RISING,
    EDGE_MODE_BOTH,
} EDGE_MODE;

/* Auto gen based on BB2Plus_IO_module_20230920_v0.xlsx */

typedef struct
{
    /* 0x0000       0x4000_6000
        15:0    R/W    Wdt_divfactor                   1
        23:16   R/W    Wdt_en_byte                     0
        24      W      Wdt_clear                       1
        28:25   R/W    Cnt_limit                       0
        29      R/W    wdtaon_en                       1
        30      R/W    Wdt_mode                        0
        31      R/W1C  Wdt_to                          0
    */
    union
    {
        __IO uint32_t REGWATCH_DOG_TIMER;
        struct
        {
            __IO uint32_t Wdt_divfactor: 16;
            __IO uint32_t Wdt_en_byte: 8;
            __IO uint32_t Wdt_clear: 1;
            __IO uint32_t Cnt_limit: 4;
            __IO uint32_t wdtaon_en: 1;
            __IO uint32_t Wdt_mode: 1;
            __IO uint32_t Wdt_to: 1;
        } BITS_000;
    } u_000;

    /* 0x0004       0x4000_6004
        0       R/W1C  MBIAS_MFB_DET_L                 0
        1       R/W1C  mailbox_int                     0
        2       R/W1C  utmi_suspend_n                  0
        3       R/W1C  dig_trda_int_r                  0
        4       R/W1C  rng_int                         0
        5       R/W1C  psram_intr                      0
        6       R/W1C  dig_lpcomp_int_r                0
        7       R/W1C  timer_intr[5]                   0
        8       R/W1C  timer_intr[6]                   0
        9       R/W1C  timer_intr[7]                   0
        10      R      reserved                        0
        11      R/W1C  dig_lpcomp_int                  0
        12      R/W1C  MBIAS_VBAT_DET_L                0
        13      R/W1C  MBIAS_ADP_DET_L                 0
        14      R/W1C  HW_ASRC_ISR1                    0
        15      R/W1C  HW_ASRC_ISR2                    0
        16      R/W1C  gpio_intr[31:6]                 0
        17      R      reserved                        0
        18      R/W1C  dsp_wdt_to_mcu_intr             0
        19      R/W1C  flash_pwr_intr                  0
        20      R      reserved                        0
        24:21   R      reserved                        0
        25      R/W1C  sp0_intr_tx                     0
        26      R/W1C  sp0_intr_rx                     0
        27      R/W1C  sp1_intr_tx                     0
        28      R/W1C  sp1_intr_rx                     0
        29      R      reserved                        0
        30      R      reserved                        0
        31      R      reserved                        0
    */
    union
    {
        __IO uint32_t REG_LOW_PRI_INT_STATUS;
        struct
        {
            __IO uint32_t MBIAS_MFB_DET_L: 1;
            __IO uint32_t mailbox_int: 1;
            __IO uint32_t utmi_suspend_n: 1;
            __IO uint32_t dig_trda_int_r: 1;
            __IO uint32_t rng_int: 1;
            __IO uint32_t psram_intr: 1;
            __IO uint32_t dig_lpcomp_int_r: 1;
            __IO uint32_t timer_intr_5: 1;
            __IO uint32_t timer_intr_6: 1;
            __IO uint32_t timer_intr_7: 1;
            __I uint32_t RESERVED_6: 1;
            __IO uint32_t dig_lpcomp_int: 1;
            __IO uint32_t MBIAS_VBAT_DET_L: 1;
            __IO uint32_t MBIAS_ADP_DET_L: 1;
            __IO uint32_t HW_ASRC_ISR1: 1;
            __IO uint32_t HW_ASRC_ISR2: 1;
            __IO uint32_t gpio_intr_31_6: 1;
            __I uint32_t RESERVED_5: 1;
            __IO uint32_t dsp_wdt_to_mcu_intr: 1;
            __IO uint32_t flash_pwr_intr: 1;
            __I uint32_t RESERVED_4: 1;
            __I uint32_t RESERVED_3: 4;
            __IO uint32_t sp0_intr_tx: 1;
            __IO uint32_t sp0_intr_rx: 1;
            __IO uint32_t sp1_intr_tx: 1;
            __IO uint32_t sp1_intr_rx: 1;
            __I uint32_t RESERVED_2: 1;
            __I uint32_t RESERVED_1: 1;
            __I uint32_t RESERVED_0: 1;
        } BITS_004;
    } u_004;

    /* 0x0008       0x4000_6008
        31:0    R/W    int_mode                        32'hffffffff
    */
    union
    {
        __IO uint32_t REG_LOW_PRI_INT_MODE;
        struct
        {
            __IO uint32_t int_mode: 32;
        } BITS_008;
    } u_008;

    /* 0x000C       0x4000_600c
        31:0    R/W    int_en                          32'h0
    */
    union
    {
        __IO uint32_t REG_LOW_PRI_INT_EN;
        struct
        {
            __IO uint32_t int_en: 32;
        } BITS_00C;
    } u_00C;

    /* 0x0010       0x4000_6010
        0       R      timer_intr1andtimer_intr0       0
        1       R      bluewiz_intr_r                  0
        2       R      hci_dma_intr                    0
        3       R      btverdor_reg_intr               0
        4       R      RSVD                            0
        31:5    R      RSVD                            0
    */
    union
    {
        __IO uint32_t BT_MAC_interrupt;
        struct
        {
            __I uint32_t timer_intr1andtimer_intr0: 1;
            __I uint32_t bluewiz_intr_r: 1;
            __I uint32_t hci_dma_intr: 1;
            __I uint32_t btverdor_reg_intr: 1;
            __I uint32_t RESERVED_1: 1;
            __I uint32_t RESERVED_0: 27;
        } BITS_010;
    } u_010;

    /* 0x0014       0x4000_6014
        0       R      otg_intr                        0
        1       R      sdio_host_intr                  0
        15:2    R      RSVD                            0
        16      R      rxi300_intr                     0
        17      R      rdp_intr                        0
        23:18   R      RSVD                            0
        24      RW     rxi300_intr_en                  1
        25      RW     rdp_intr_en                     0
        31:26   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_PLATFORM_INT_CTRL;
        struct
        {
            __I uint32_t otg_intr: 1;
            __I uint32_t sdio_host_intr: 1;
            __I uint32_t RESERVED_2: 14;
            __I uint32_t rxi300_intr: 1;
            __I uint32_t rdp_intr: 1;
            __I uint32_t RESERVED_1: 6;
            __IO uint32_t rxi300_intr_en: 1;
            __IO uint32_t rdp_intr_en: 1;
            __I uint32_t RESERVED_0: 6;
        } BITS_014;
    } u_014;

    /* 0x0018       0x4000_6018
        31:0    RW     int_pol                         0
    */
    union
    {
        __IO uint32_t Interrupt_edge_option;
        struct
        {
            __IO uint32_t int_pol: 32;
        } BITS_018;
    } u_018;

    /* 0x001C       0x4000_601c
        11:0    RW     BIT_DEBUG_SEL                   0
        15:12   R      RSVD                            0
        16      RW     pke_clk_always_disable          1'h0
        17      RW     pke_clk_always_enable           1'h0
        18      RW     ppe_clk_always_disable          1'h0
        19      RW     ppe_clk_always_enable           1'h0
        28:20   R      RSVD                            0
        29      RW     spi_speed_up_sim                0
        30      R      RSVD                            0
        31      RW     uartlog_sir_inv                 0
    */
    union
    {
        __IO uint32_t REG_DEBUG_SEL;
        struct
        {
            __IO uint32_t BIT_DEBUG_SEL: 12;
            __I uint32_t RESERVED_2: 4;
            __IO uint32_t pke_clk_always_disable: 1;
            __IO uint32_t pke_clk_always_enable: 1;
            __IO uint32_t ppe_clk_always_disable: 1;
            __IO uint32_t ppe_clk_always_enable: 1;
            __I uint32_t RESERVED_1: 9;
            __IO uint32_t spi_speed_up_sim: 1;
            __I uint32_t RESERVED_0: 1;
            __IO uint32_t uartlog_sir_inv: 1;
        } BITS_01C;
    } u_01C;

    /* 0x0020       0x4000_6020
        0       R/W    BIT_I2C0_TX_BIT_SWAP_EN         0
        1       R/W    BIT_I2C0_RX_BIT_SWAP_EN         0
        2       R/W    BIT_I2C1_TX_BIT_SWAP_EN         0
        3       R/W    BIT_I2C1_RX_BIT_SWAP_EN         0
        4       R/W    BIT_I2C2_TX_BIT_SWAP_EN         0
        5       R/W    BIT_I2C2_RX_BIT_SWAP_EN         0
        7:6     R      RSVD                            0
        8       R/W    BYTE_SPI0_TX_BYTE_SWAP_EN       0
        9       R/W    BYTE_SPI0_RX_BYTE_SWAP_EN       0
        15:10   R      RSVD                            0
        16      R/W    BIT_SPI0_TX_BIT_SWAP_EN         0
        17      R/W    BIT_SPI0_RX_BIT_SWAP_EN         0
        28:18   R      RSVD                            0
        29      W1O    dummy                           0
        30      W1O    reg_efuse_wp                    0
        31      W1O    reg_efuse_rp                    0
    */
    union
    {
        __IO uint32_t REG_DATA_FIFO_SWAP_CTRL;
        struct
        {
            __IO uint32_t BIT_I2C0_TX_BIT_SWAP_EN: 1;
            __IO uint32_t BIT_I2C0_RX_BIT_SWAP_EN: 1;
            __IO uint32_t BIT_I2C1_TX_BIT_SWAP_EN: 1;
            __IO uint32_t BIT_I2C1_RX_BIT_SWAP_EN: 1;
            __IO uint32_t BIT_I2C2_TX_BIT_SWAP_EN: 1;
            __IO uint32_t BIT_I2C2_RX_BIT_SWAP_EN: 1;
            __I uint32_t RESERVED_3: 2;
            __IO uint32_t BYTE_SPI0_TX_BYTE_SWAP_EN: 1;
            __IO uint32_t BYTE_SPI0_RX_BYTE_SWAP_EN: 1;
            __I uint32_t RESERVED_2: 6;
            __IO uint32_t BIT_SPI0_TX_BIT_SWAP_EN: 1;
            __IO uint32_t BIT_SPI0_RX_BIT_SWAP_EN: 1;
            __I uint32_t RESERVED_1: 11;
            __IO uint32_t RESERVED_0: 1;
            __IO uint32_t reg_efuse_wp: 1;
            __IO uint32_t reg_efuse_rp: 1;
        } BITS_020;
    } u_020;

    /* 0x0024       0x4000_6024
        7:0     R      RSVD                            0
        8       R/W    rx2_dvfs_en                     0
        9       R      RSVD                            0
        10      R/W    reg_fwpi_enable                 1
        11      R/W    spic_icg_disable                0
        12      R/W    ic_icg_disable                  0
        13      R/W    dmac_clk_always_disable         0
        14      R/W    dmac_clk_always_enable          0
        15      R/W    rxi300_auto_icg_en              1
        23:16   R/W    timer_dma_en                    0
        24      R/W    spic0_mem_wr_en                 0
        25      R/W    spic1_mem_wr_en                 0
        26      R/W    spic2_mem_wr_en                 0
        27      R/W    spic3_mem_wr_en                 0
        28      R/W    psram_mem_wr_en                 0
        30:29   R      RSVD                            0
        31      R/W    dsp_dma_int_mask_n              1
    */
    union
    {
        __IO uint32_t REG_DMAC_CLK_CTRL;
        struct
        {
            __I uint32_t RESERVED_2: 8;
            __IO uint32_t rx2_dvfs_en: 1;
            __I uint32_t RESERVED_1: 1;
            __IO uint32_t reg_fwpi_enable: 1;
            __IO uint32_t spic_icg_disable: 1;
            __IO uint32_t ic_icg_disable: 1;
            __IO uint32_t dmac_clk_always_disable: 1;
            __IO uint32_t dmac_clk_always_enable: 1;
            __IO uint32_t rxi300_auto_icg_en: 1;
            __IO uint32_t timer_dma_en: 8;
            __IO uint32_t spic0_mem_wr_en: 1;
            __IO uint32_t spic1_mem_wr_en: 1;
            __IO uint32_t spic2_mem_wr_en: 1;
            __IO uint32_t spic3_mem_wr_en: 1;
            __IO uint32_t psram_mem_wr_en: 1;
            __I uint32_t RESERVED_0: 2;
            __IO uint32_t dsp_dma_int_mask_n: 1;
        } BITS_024;
    } u_024;

    /* 0x0028       0x4000_6028
        31:0    R      reserved                        0
    */
    __IO uint32_t REG_0x0028;

    /* 0x002C       0x4000_602c
        0       R/W    r_hw_alu_en                     0
        3:1     R/W    r_hw_alu_mode                   0
        4       R/W    r_hw_alu_trig                   0
        30:5    R      reserved                        0
        31      R      hw_alu_done                     0
    */
    union
    {
        __IO uint32_t REG_CORDIC_CTRL;
        struct
        {
            __IO uint32_t r_hw_alu_en: 1;
            __IO uint32_t r_hw_alu_mode: 3;
            __IO uint32_t r_hw_alu_trig: 1;
            __I uint32_t RESERVED_0: 26;
            __I uint32_t hw_alu_done: 1;
        } BITS_02C;
    } u_02C;

    /* 0x0030       0x4000_6030
        15:0    R/W    r_hw_alu_in_a                   0
        31:16   R/W    r_hw_alu_in_b                   0
    */
    union
    {
        __IO uint32_t REG_CORDIC_IN0;
        struct
        {
            __IO uint32_t r_hw_alu_in_a: 16;
            __IO uint32_t r_hw_alu_in_b: 16;
        } BITS_030;
    } u_030;

    /* 0x0034       0x4000_6034
        15:0    R/W    r_hw_alu_in_c                   0
        31:16   R/W    r_hw_alu_in_d                   0
    */
    union
    {
        __IO uint32_t REG_CORDIC_IN1;
        struct
        {
            __IO uint32_t r_hw_alu_in_c: 16;
            __IO uint32_t r_hw_alu_in_d: 16;
        } BITS_034;
    } u_034;

    /* 0x0038       0x4000_6038
        15:0    R      hw_alu_out_a                    0
        31:16   R      hw_alu_out_b                    0
    */
    union
    {
        __IO uint32_t REG_CORDIC_OUT0;
        struct
        {
            __I uint32_t hw_alu_out_a: 16;
            __I uint32_t hw_alu_out_b: 16;
        } BITS_038;
    } u_038;

    /* 0x003C       0x4000_603c
        15:0    R      hw_alu_out_c                    0
        31:16   R      hw_alu_out_d                    0
    */
    union
    {
        __IO uint32_t REG_CORDIC_OUT1;
        struct
        {
            __I uint32_t hw_alu_out_c: 16;
            __I uint32_t hw_alu_out_d: 16;
        } BITS_03C;
    } u_03C;

    /* 0x0040       0x4000_6040
        4:0     R/W    bist_rstn_rom                   5'h0
        8:5     R/W    bist_rstn_buffer_ram            4'h0
        13:9    R      RSVD                            5'h0
        21:14   R/W    bist_rstn_cache_ram             8'h0
        28:22   R/W    bist_rstn_dsp_rom               7'h0
        31:29   R      RSVD                            3'h0
    */
    union
    {
        __IO uint32_t REG_BIST_RSTN_0;
        struct
        {
            __IO uint32_t bist_rstn_rom: 5;
            __IO uint32_t bist_rstn_buffer_ram: 4;
            __I uint32_t RESERVED_1: 5;
            __IO uint32_t bist_rstn_cache_ram: 8;
            __IO uint32_t bist_rstn_dsp_rom: 7;
            __I uint32_t RESERVED_0: 3;
        } BITS_040;
    } u_040;

    /* 0x0044       0x4000_6044
        4:0     R/W    bist_mode_rom                   5'h0
        8:5     R/W    bist_mode_buffer_ram            4'h0
        13:9    R      RSVD                            5'h0
        21:14   R/W    bist_mode_cache_ram             8'h0
        28:22   R/W    bist_mode_dsp_rom               7'h0
        31:29   R      RSVD                            3'h0
    */
    union
    {
        __IO uint32_t REG_BIST_MODE_0;
        struct
        {
            __IO uint32_t bist_mode_rom: 5;
            __IO uint32_t bist_mode_buffer_ram: 4;
            __I uint32_t RESERVED_1: 5;
            __IO uint32_t bist_mode_cache_ram: 8;
            __IO uint32_t bist_mode_dsp_rom: 7;
            __I uint32_t RESERVED_0: 3;
        } BITS_044;
    } u_044;

    /* 0x0048       0x4000_6048
        4:0     R      RSVD                            5'h0
        8:5     R/W    bist_mode_drf_buffer_ram        4'h0
        13:9    R      RSVD                            5'h0
        21:14   R/W    bist_mode_drf_cache_ram         8'h0
        31:22   R      RSVD                            10'h0
    */
    union
    {
        __IO uint32_t REG_BIST_MODE_DRF_0;
        struct
        {
            __I uint32_t RESERVED_2: 5;
            __IO uint32_t bist_mode_drf_buffer_ram: 4;
            __I uint32_t RESERVED_1: 5;
            __IO uint32_t bist_mode_drf_cache_ram: 8;
            __I uint32_t RESERVED_0: 10;
        } BITS_048;
    } u_048;

    /* 0x004C       0x4000_604c
        4:0     R      RSVD                            5'h0
        8:5     R/W    bist_test_resume_buffer_ram     4'h0
        13:9    R      RSVD                            5'h0
        21:14   R/W    bist_test_resume_cache_ram      8'h0
        31:22   R      RSVD                            10'h0
    */
    union
    {
        __IO uint32_t REG_BIST_TEST_RESUME_0;
        struct
        {
            __I uint32_t RESERVED_2: 5;
            __IO uint32_t bist_test_resume_buffer_ram: 4;
            __I uint32_t RESERVED_1: 5;
            __IO uint32_t bist_test_resume_cache_ram: 8;
            __I uint32_t RESERVED_0: 10;
        } BITS_04C;
    } u_04C;

    /* 0x0050       0x4000_6050
        4:0     R      RSVD                            5'h0
        8:5     R      bist_start_pause_buffer_ram     4'h0
        13:9    R      RSVD                            5'h0
        21:14   R      bist_start_pause_cache_ram      8'h0
        31:22   R      RSVD                            10'h0
    */
    union
    {
        __IO uint32_t REG_BIST_START_PAUSE_0;
        struct
        {
            __I uint32_t RESERVED_2: 5;
            __I uint32_t bist_start_pause_buffer_ram: 4;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t bist_start_pause_cache_ram: 8;
            __I uint32_t RESERVED_0: 10;
        } BITS_050;
    } u_050;

    /* 0x0054       0x4000_6054
        4:0     R      bist_done_rom                   5'h0
        8:5     R      bist_done_buffer_ram            4'h0
        13:9    R      RSVD                            5'h0
        21:14   R      bist_done_cache_ram             8'h0
        28:22   R      bist_done_dsp_rom               7'h0
        31:29   R      RSVD                            3'h0
    */
    union
    {
        __IO uint32_t REG_BIST_DONE_0;
        struct
        {
            __I uint32_t bist_done_rom: 5;
            __I uint32_t bist_done_buffer_ram: 4;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t bist_done_cache_ram: 8;
            __I uint32_t bist_done_dsp_rom: 7;
            __I uint32_t RESERVED_0: 3;
        } BITS_054;
    } u_054;

    /* 0x0058       0x4000_6058
        4:0     R      RSVD                            5'h0
        8:5     R      bist_fail_buffer_ram            4'h0
        13:9    R      RSVD                            5'h0
        21:14   R      bist_fail_cache_ram             8'h0
        31:22   R      RSVD                            10'h0
    */
    union
    {
        __IO uint32_t REG_BIST_FAIL_0;
        struct
        {
            __I uint32_t RESERVED_2: 5;
            __I uint32_t bist_fail_buffer_ram: 4;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t bist_fail_cache_ram: 8;
            __I uint32_t RESERVED_0: 10;
        } BITS_058;
    } u_058;

    /* 0x005C       0x4000_605c
        4:0     R      RSVD                            5'h0
        8:5     R      bist_done_drf_buffer_ram        4'h0
        13:9    R      RSVD                            5'h0
        21:14   R      bist_done_drf_cache_ram         8'h0
        31:22   R      RSVD                            10'h0
    */
    union
    {
        __IO uint32_t REG_BIST_DONE_DRF_0;
        struct
        {
            __I uint32_t RESERVED_2: 5;
            __I uint32_t bist_done_drf_buffer_ram: 4;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t bist_done_drf_cache_ram: 8;
            __I uint32_t RESERVED_0: 10;
        } BITS_05C;
    } u_05C;

    /* 0x0060       0x4000_6060
        4:0     R      RSVD                            5'h0
        8:5     R      bist_fail_drf_buffer_ram        4'h0
        13:9    R      RSVD                            5'h0
        21:14   R      bist_fail_drf_cache_ram         8'h0
        31:22   R      RSVD                            10'h0
    */
    union
    {
        __IO uint32_t REG_BIST_FAIL_DRF_0;
        struct
        {
            __I uint32_t RESERVED_2: 5;
            __I uint32_t bist_fail_drf_buffer_ram: 4;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t bist_fail_drf_cache_ram: 8;
            __I uint32_t RESERVED_0: 10;
        } BITS_060;
    } u_060;

    /* 0x0064       0x4000_6064
        31:0    R      misr_data_out_rom_0_l           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM0_MISR0;
        struct
        {
            __I uint32_t misr_data_out_rom_0_l: 32;
        } BITS_064;
    } u_064;

    /* 0x0068       0x4000_6068
        31:0    R      misr_data_out_rom_0_h           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM0_MISR1;
        struct
        {
            __I uint32_t misr_data_out_rom_0_h: 32;
        } BITS_068;
    } u_068;

    /* 0x006C       0x4000_606c
        31:0    R      misr_data_out_rom_1_l           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM1_MISR0;
        struct
        {
            __I uint32_t misr_data_out_rom_1_l: 32;
        } BITS_06C;
    } u_06C;

    /* 0x0070       0x4000_6070
        31:0    R      misr_data_out_rom_1_h           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM1_MISR1;
        struct
        {
            __I uint32_t misr_data_out_rom_1_h: 32;
        } BITS_070;
    } u_070;

    /* 0x0074       0x4000_6074
        31:0    R      misr_data_out_rom_2_l           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM2_MISR0;
        struct
        {
            __I uint32_t misr_data_out_rom_2_l: 32;
        } BITS_074;
    } u_074;

    /* 0x0078       0x4000_6078
        31:0    R      misr_data_out_rom_2_h           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM2_MISR1;
        struct
        {
            __I uint32_t misr_data_out_rom_2_h: 32;
        } BITS_078;
    } u_078;

    /* 0x007C       0x4000_607c
        31:0    R      misr_data_out_rom_3_l           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM3_MISR0;
        struct
        {
            __I uint32_t misr_data_out_rom_3_l: 32;
        } BITS_07C;
    } u_07C;

    /* 0x0080       0x4000_6080
        31:0    R      misr_data_out_rom_3_h           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM3_MISR1;
        struct
        {
            __I uint32_t misr_data_out_rom_3_h: 32;
        } BITS_080;
    } u_080;

    /* 0x0084       0x4000_6084
        31:0    R      misr_data_out_rom_4_l           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM4_MISR0;
        struct
        {
            __I uint32_t misr_data_out_rom_4_l: 32;
        } BITS_084;
    } u_084;

    /* 0x0088       0x4000_6088
        31:0    R      misr_data_out_rom_4_h           0
    */
    union
    {
        __IO uint32_t REG_MCU_ROM4_MISR1;
        struct
        {
            __I uint32_t misr_data_out_rom_4_h: 32;
        } BITS_088;
    } u_088;

    /* 0x008C       0x4000_608c
        29:0    R      RSVD                            30'h33550353
        30      R/W    BIST_VDDR_EN                    1'h0
        31      R/W    BIST_LOOP_MODE                  1'h0
    */
    union
    {
        __IO uint32_t REG_BIST_GLOBAL_CTL;
        struct
        {
            __I uint32_t RESERVED_0: 30;
            __IO uint32_t BIST_VDDR_EN: 1;
            __IO uint32_t BIST_LOOP_MODE: 1;
        } BITS_08C;
    } u_08C;

    /* 0x0090       0x4000_6090
        31:0    R/W    OUT_DATA0                       0
    */
    union
    {
        __IO uint32_t REG_I2C_MAILBOX_OUT_DATA0;
        struct
        {
            __IO uint32_t OUT_DATA0: 32;
        } BITS_090;
    } u_090;

    /* 0x0094       0x4000_6094
        31:0    R/W    OUT_DATA1                       0
    */
    union
    {
        __IO uint32_t REG_I2C_MAILBOX_OUT_DATA1;
        struct
        {
            __IO uint32_t OUT_DATA1: 32;
        } BITS_094;
    } u_094;

    /* 0x0098       0x4000_6098
        31:0    R      IN_DATA0                        0
    */
    union
    {
        __IO uint32_t REG_I2C_MAILBOX_IN_DATA0;
        struct
        {
            __I uint32_t IN_DATA0: 32;
        } BITS_098;
    } u_098;

    /* 0x009C       0x4000_609c
        31:0    R      IN_DATA1                        0
    */
    union
    {
        __IO uint32_t REG_I2C_MAILBOX_IN_DATA1;
        struct
        {
            __I uint32_t IN_DATA1: 32;
        } BITS_09C;
    } u_09C;

    /* 0x00A0       0x4000_60a0
        0       R/W1C  outbox_rdy_r                    0
        1       R/W    out_empty_inten                 0
        2       R/W    out_int_mode                    0
        3       R/W1C  out_edge_sts                    0
        4       R      rx_data_crc_ok                  0
        15:5    R      RSVD                            0
        16      R/W1C  inbox_rdy_r                     0
        17      R/W    in_rdy_inten                    0
        30:18   R      RSVD                            0
        31      R/W    pta_i2c_en                      0
    */
    union
    {
        __IO uint32_t REG_I2C_MAILBOX_CTRL;
        struct
        {
            __IO uint32_t outbox_rdy_r: 1;
            __IO uint32_t out_empty_inten: 1;
            __IO uint32_t out_int_mode: 1;
            __IO uint32_t out_edge_sts: 1;
            __I uint32_t rx_data_crc_ok: 1;
            __I uint32_t RESERVED_1: 11;
            __IO uint32_t inbox_rdy_r: 1;
            __IO uint32_t in_rdy_inten: 1;
            __I uint32_t RESERVED_0: 13;
            __IO uint32_t pta_i2c_en: 1;
        } BITS_0A0;
    } u_0A0;

    /* 0x00A4       0x4000_60a4
        31:0    R      RSVD                            0
    */
    __IO uint32_t REG_MCU_BT_TIME_STAMP;

    /* 0x00A8       0x4000_60a8
        31:0    R      RSVD                            0
    */
    __IO uint32_t REG_MCU_BT_CLOCK_COMPARE;

    /* 0x00AC       0x4000_60ac
        7:0     R/W    BIT_EFUSE_BURN_GNT              8'h0
        23:8    R      RSVD                            16'h0
        28:24   R/W    EF_SCAN_END[8:4]                0
        31:29   R      RSVD                            3'h0
    */
    union
    {
        __IO uint32_t REG_EFUSE_PGPWD;
        struct
        {
            __IO uint32_t BIT_EFUSE_BURN_GNT: 8;
            __I uint32_t RESERVED_1: 16;
            __IO uint32_t EF_SCAN_END_8_4: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_0AC;
    } u_0AC;

    /* 0x00B0       0x4000_60b0
        7:0     R/W    Data7-0                         8'h0
        16:8    R/W    RegAddr8-0                      9'h0
        17      R      RSVD                            0
        18      R/W    Alden                           0
        19      R      RSVD                            0
        23:20   R/W    setup_time                      4'h6
        26:24   R/W    VDDQ_time                       3'h1
        27      R/W    RD_time                         0
        30:28   R/W    PG_time
                            3'h2
        31      R/W    Flag                            0
    */
    union
    {
        __IO uint32_t REG_EFUSE_CTRL;
        struct
        {
            __IO uint32_t Data7_0: 8;
            __IO uint32_t RegAddr8_0: 9;
            __I uint32_t RESERVED_1: 1;
            __IO uint32_t Alden: 1;
            __I uint32_t RESERVED_0: 1;
            __IO uint32_t setup_time: 4;
            __IO uint32_t VDDQ_time: 3;
            __IO uint32_t RD_time: 1;
            __IO uint32_t PG_time
            : 3;
            __IO uint32_t Flag: 1;
        } BITS_0B0;
    } u_0B0;

    /* 0x00B4       0x4000_60b4
        6:0     R/WAC  Thres6-0                        7'h0
        7       R/W    EFSCAN_FAIL                     0
        9:8     R/W    EFCELL_SEL                      2'b0
        10      R/W    r_PWC_EV2EF                     0
        11      R/W    EFPGMEN_FORCE                   0
        15:12   R/W    EF_SCAN_EADR                    0
        24:16   R/W    EF_SCAN_SADR                    0
        25      R/W    EFPD_SEL                        0
        26      R/W    EFCRES_SEL                      0
        31:27   R/W    DUMMY                           5'b00011
    */
    union
    {
        __IO uint32_t REG_EFUSE_TEST;
        struct
        {
            uint32_t Thres6_0: 7;
            __IO uint32_t EFSCAN_FAIL: 1;
            __IO uint32_t EFCELL_SEL: 2;
            __IO uint32_t r_PWC_EV2EF: 1;
            __IO uint32_t EFPGMEN_FORCE: 1;
            __IO uint32_t EF_SCAN_EADR: 4;
            __IO uint32_t EF_SCAN_SADR: 9;
            __IO uint32_t EFPD_SEL: 1;
            __IO uint32_t EFCRES_SEL: 1;
            __IO uint32_t RESERVED_0: 5;
        } BITS_0B4;
    } u_0B4;

    /* 0x00B8       0x4000_60b8
        31:0    R      RSVD                            0
    */
    __IO uint32_t REG_0x00B8;

    /* 0x00BC       0x4000_60bc
        4:0     R/W    bist_rstn_dsp_ram_g0            0
        9:5     R/W    bist_rstn_dsp_ram_g1            0
        14:10   R/W    bist_rstn_dsp_ram_g2            0
        19:15   R/W    bist_rstn_dsp_ram_g3            0
        24:20   R/W    RSVD                            0
        29:25   R/W    RSVD                            0
        31:30   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_BIST_RSTN;
        struct
        {
            __IO uint32_t bist_rstn_dsp_ram_g0: 5;
            __IO uint32_t bist_rstn_dsp_ram_g1: 5;
            __IO uint32_t bist_rstn_dsp_ram_g2: 5;
            __IO uint32_t bist_rstn_dsp_ram_g3: 5;
            __IO uint32_t RESERVED_2: 5;
            __IO uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 2;
        } BITS_0BC;
    } u_0BC;

    /* 0x00C0       0x4000_60c0
        4:0     R/W    bist_mode_dsp_ram_g0            0
        9:5     R/W    bist_mode_dsp_ram_g1            0
        14:10   R/W    bist_mode_dsp_ram_g2            0
        19:15   R/W    bist_mode_dsp_ram_g3            0
        24:20   R/W    RSVD                            0
        29:25   R/W    RSVD                            0
        31:30   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_BIST_MODE;
        struct
        {
            __IO uint32_t bist_mode_dsp_ram_g0: 5;
            __IO uint32_t bist_mode_dsp_ram_g1: 5;
            __IO uint32_t bist_mode_dsp_ram_g2: 5;
            __IO uint32_t bist_mode_dsp_ram_g3: 5;
            __IO uint32_t RESERVED_2: 5;
            __IO uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 2;
        } BITS_0C0;
    } u_0C0;

    /* 0x00C4       0x4000_60c4
        4:0     R/W    bist_mode_drf_dsp_ram_g0        0
        9:5     R/W    bist_mode_drf_dsp_ram_g1        0
        14:10   R/W    bist_mode_drf_dsp_ram_g2        0
        19:15   R/W    bist_mode_drf_dsp_ram_g3        0
        24:20   R/W    RSVD                            0
        29:25   R/W    RSVD                            0
        31:30   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_DRF_BIST_MODE;
        struct
        {
            __IO uint32_t bist_mode_drf_dsp_ram_g0: 5;
            __IO uint32_t bist_mode_drf_dsp_ram_g1: 5;
            __IO uint32_t bist_mode_drf_dsp_ram_g2: 5;
            __IO uint32_t bist_mode_drf_dsp_ram_g3: 5;
            __IO uint32_t RESERVED_2: 5;
            __IO uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 2;
        } BITS_0C4;
    } u_0C4;

    /* 0x00C8       0x4000_60c8
        4:0     R/W    bist_test_resume_dsp_ram_g0     0
        9:5     R/W    bist_test_resume_dsp_ram_g1     0
        14:10   R/W    bist_test_resume_dsp_ram_g2     0
        19:15   R/W    bist_test_resume_dsp_ram_g3     0
        24:20   R/W    RSVD                            0
        29:25   R/W    RSVD                            0
        31:30   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_BIST_TEST_RESUME;
        struct
        {
            __IO uint32_t bist_test_resume_dsp_ram_g0: 5;
            __IO uint32_t bist_test_resume_dsp_ram_g1: 5;
            __IO uint32_t bist_test_resume_dsp_ram_g2: 5;
            __IO uint32_t bist_test_resume_dsp_ram_g3: 5;
            __IO uint32_t RESERVED_2: 5;
            __IO uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 2;
        } BITS_0C8;
    } u_0C8;

    /* 0x00CC       0x4000_60cc
        4:0     R      bist_start_pause_dsp_ram_g0     0
        9:5     R      bist_start_pause_dsp_ram_g1     0
        14:10   R      bist_start_pause_dsp_ram_g2     0
        19:15   R      bist_start_pause_dsp_ram_g3     0
        24:20   R      RSVD                            0
        29:25   R      RSVD                            0
        31:30   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_BIST_START_PAUSE;
        struct
        {
            __I uint32_t bist_start_pause_dsp_ram_g0: 5;
            __I uint32_t bist_start_pause_dsp_ram_g1: 5;
            __I uint32_t bist_start_pause_dsp_ram_g2: 5;
            __I uint32_t bist_start_pause_dsp_ram_g3: 5;
            __I uint32_t RESERVED_2: 5;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 2;
        } BITS_0CC;
    } u_0CC;

    /* 0x00D0       0x4000_60d0
        4:0     R      bist_done_dsp_ram_g0            0
        9:5     R      bist_done_dsp_ram_g1            0
        14:10   R      bist_done_dsp_ram_g2            0
        19:15   R      bist_done_dsp_ram_g3            0
        24:20   R      RSVD                            0
        29:25   R      RSVD                            0
        31:30   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_BIST_DONE;
        struct
        {
            __I uint32_t bist_done_dsp_ram_g0: 5;
            __I uint32_t bist_done_dsp_ram_g1: 5;
            __I uint32_t bist_done_dsp_ram_g2: 5;
            __I uint32_t bist_done_dsp_ram_g3: 5;
            __I uint32_t RESERVED_2: 5;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 2;
        } BITS_0D0;
    } u_0D0;

    /* 0x00D4       0x4000_60d4
        4:0     R      bist_fail_dsp_ram_g0            0
        9:5     R      bist_fail_dsp_ram_g1            0
        14:10   R      bist_fail_dsp_ram_g2            0
        19:15   R      bist_fail_dsp_ram_g3            0
        24:20   R      RSVD                            0
        29:25   R      RSVD                            0
        31:30   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_BIST_FAIL;
        struct
        {
            __I uint32_t bist_fail_dsp_ram_g0: 5;
            __I uint32_t bist_fail_dsp_ram_g1: 5;
            __I uint32_t bist_fail_dsp_ram_g2: 5;
            __I uint32_t bist_fail_dsp_ram_g3: 5;
            __I uint32_t RESERVED_2: 5;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 2;
        } BITS_0D4;
    } u_0D4;

    /* 0x00D8       0x4000_60d8
        4:0     R      drf_bist_done_dsp_ram_g0        0
        9:5     R      drf_bist_done_dsp_ram_g1        0
        14:10   R      drf_bist_done_dsp_ram_g2        0
        19:15   R      drf_bist_done_dsp_ram_g3        0
        24:20   R      RSVD                            0
        29:25   R      RSVD                            0
        31:30   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_DRF_BIST_DONE;
        struct
        {
            __I uint32_t drf_bist_done_dsp_ram_g0: 5;
            __I uint32_t drf_bist_done_dsp_ram_g1: 5;
            __I uint32_t drf_bist_done_dsp_ram_g2: 5;
            __I uint32_t drf_bist_done_dsp_ram_g3: 5;
            __I uint32_t RESERVED_2: 5;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 2;
        } BITS_0D8;
    } u_0D8;

    /* 0x00DC       0x4000_60dc
        4:0     R      drf_bist_fail_dsp_ram_g0        0
        9:5     R      drf_bist_fail_dsp_ram_g1        0
        14:10   R      drf_bist_fail_dsp_ram_g2        0
        19:15   R      drf_bist_fail_dsp_ram_g3        0
        24:20   R      RSVD                            0
        29:25   R      RSVD                            0
        31:30   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_DRF_BIST_FAIL;
        struct
        {
            __I uint32_t drf_bist_fail_dsp_ram_g0: 5;
            __I uint32_t drf_bist_fail_dsp_ram_g1: 5;
            __I uint32_t drf_bist_fail_dsp_ram_g2: 5;
            __I uint32_t drf_bist_fail_dsp_ram_g3: 5;
            __I uint32_t RESERVED_2: 5;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 2;
        } BITS_0DC;
    } u_0DC;

    /* 0x00E0       0x4000_60e0
        12:0    R/W    bist_rstn_btmac                 0
        15:13   R      RSVD                            0
        19:16   R/W    bist_rstn_btphy                 0
        29:20   R      RSVD                            0
        30      R/W    bist_rstn_sdh                   0
        31      R/W    RSVD                            0
    */
    union
    {
        __IO uint32_t REG_OTHER_BIST_RSTN;
        struct
        {
            __IO uint32_t bist_rstn_btmac: 13;
            __I uint32_t RESERVED_2: 3;
            __IO uint32_t bist_rstn_btphy: 4;
            __I uint32_t RESERVED_1: 10;
            __IO uint32_t bist_rstn_sdh: 1;
            __IO uint32_t RESERVED_0: 1;
        } BITS_0E0;
    } u_0E0;

    /* 0x00E4       0x4000_60e4
        12:0    R/W    bist_mode_btmac                 0
        15:13   R      RSVD                            0
        19:16   R/W    bist_mode_btphy                 0
        29:20   R      RSVD                            0
        30      R/W    bist_mode_sdh                   0
        31      R/W    RSVD                            0
    */
    union
    {
        __IO uint32_t REG_OTHER_BIST_MODE;
        struct
        {
            __IO uint32_t bist_mode_btmac: 13;
            __I uint32_t RESERVED_2: 3;
            __IO uint32_t bist_mode_btphy: 4;
            __I uint32_t RESERVED_1: 10;
            __IO uint32_t bist_mode_sdh: 1;
            __IO uint32_t RESERVED_0: 1;
        } BITS_0E4;
    } u_0E4;

    /* 0x00E8       0x4000_60e8
        12:0    R/W    bist_drf_mode_btmac             0
        15:13   R      RSVD                            0
        19:16   R/W    bist_drf_mode_btphy             0
        29:20   R      RSVD                            0
        30      R/W    bist_drf_mode_sdh               0
        31      R/W    RSVD                            0
    */
    union
    {
        __IO uint32_t REG_OTHER_DRF_BIST_MODE;
        struct
        {
            __IO uint32_t bist_drf_mode_btmac: 13;
            __I uint32_t RESERVED_2: 3;
            __IO uint32_t bist_drf_mode_btphy: 4;
            __I uint32_t RESERVED_1: 10;
            __IO uint32_t bist_drf_mode_sdh: 1;
            __IO uint32_t RESERVED_0: 1;
        } BITS_0E8;
    } u_0E8;

    /* 0x00EC       0x4000_60ec
        12:0    R/W    bist_test_resume_btmac          0
        15:13   R      RSVD                            0
        19:16   R/W    bist_test_resume_btphy          0
        29:20   R      RSVD                            0
        30      R/W    bist_test_resume_sdh            0
        31      R/W    RSVD                            0
    */
    union
    {
        __IO uint32_t REG_OTHER_BIST_TEST_RESUME;
        struct
        {
            __IO uint32_t bist_test_resume_btmac: 13;
            __I uint32_t RESERVED_2: 3;
            __IO uint32_t bist_test_resume_btphy: 4;
            __I uint32_t RESERVED_1: 10;
            __IO uint32_t bist_test_resume_sdh: 1;
            __IO uint32_t RESERVED_0: 1;
        } BITS_0EC;
    } u_0EC;

    /* 0x00F0       0x4000_60f0
        12:0    R      bist_start_pause_btmac          0
        15:13   R      RSVD                            0
        19:16   R      bist_start_pause_btphy          0
        29:20   R      RSVD                            0
        30      R      bist_start_pause_sdh            0
        31      R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_OTHER_BIST_START_PAUSE;
        struct
        {
            __I uint32_t bist_start_pause_btmac: 13;
            __I uint32_t RESERVED_2: 3;
            __I uint32_t bist_start_pause_btphy: 4;
            __I uint32_t RESERVED_1: 10;
            __I uint32_t bist_start_pause_sdh: 1;
            __I uint32_t RESERVED_0: 1;
        } BITS_0F0;
    } u_0F0;

    /* 0x00F4       0x4000_60f4
        12:0    R      bist_done_btmac                 0
        15:13   R      RSVD                            0
        19:16   R      bist_done_btphy                 0
        29:20   R      RSVD                            0
        30      R      bist_done_sdh                   0
        31      R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_OTHER_BIST_DONE;
        struct
        {
            __I uint32_t bist_done_btmac: 13;
            __I uint32_t RESERVED_2: 3;
            __I uint32_t bist_done_btphy: 4;
            __I uint32_t RESERVED_1: 10;
            __I uint32_t bist_done_sdh: 1;
            __I uint32_t RESERVED_0: 1;
        } BITS_0F4;
    } u_0F4;

    /* 0x00F8       0x4000_60f8
        12:0    R      bist_fail_btmac                 0
        15:13   R      RSVD                            0
        19:16   R      bist_fail_btphy                 0
        29:20   R      RSVD                            0
        30      R      bist_fail_sdh                   0
        31      R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_OTHER_BIST_FAIL;
        struct
        {
            __I uint32_t bist_fail_btmac: 13;
            __I uint32_t RESERVED_2: 3;
            __I uint32_t bist_fail_btphy: 4;
            __I uint32_t RESERVED_1: 10;
            __I uint32_t bist_fail_sdh: 1;
            __I uint32_t RESERVED_0: 1;
        } BITS_0F8;
    } u_0F8;

    /* 0x00FC       0x4000_60fc
        12:0    R      bist_done_drf_btmac             0
        15:13   R      RSVD                            0
        19:16   R      bist_done_drf_btphy             0
        29:20   R      RSVD                            0
        30      R      bist_done_drf_sdh               0
        31      R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_OTHER_DRF_BIST_BIST_DONE;
        struct
        {
            __I uint32_t bist_done_drf_btmac: 13;
            __I uint32_t RESERVED_2: 3;
            __I uint32_t bist_done_drf_btphy: 4;
            __I uint32_t RESERVED_1: 10;
            __I uint32_t bist_done_drf_sdh: 1;
            __I uint32_t RESERVED_0: 1;
        } BITS_0FC;
    } u_0FC;

    /* 0x0100       0x4000_6100
        12:0    R      bist_fail_drf_btmac             0
        15:13   R      RSVD                            0
        19:16   R      bist_fail_drf_btphy             0
        29:20   R      RSVD                            0
        30      R      bist_fail_drf_sdh               0
        31      R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_OTHER_DRF_BIST_BIST_FAIL;
        struct
        {
            __I uint32_t bist_fail_drf_btmac: 13;
            __I uint32_t RESERVED_2: 3;
            __I uint32_t bist_fail_drf_btphy: 4;
            __I uint32_t RESERVED_1: 10;
            __I uint32_t bist_fail_drf_sdh: 1;
            __I uint32_t RESERVED_0: 1;
        } BITS_100;
    } u_100;

    /* 0x0104       0x4000_6104
        31:0    R      RSVD                            0
    */
    __IO uint32_t RSVD_2;

    /* 0x0108       0x4000_6108
        31:0    R      RSVD                            0
    */
    __IO uint32_t RSVD_3;

    __IO uint32_t RSVD_0x10c[16];

    /* 0x014C       0x4000_614c
        31:0    RW     km4_initzwfrange                32'h0
    */
    union
    {
        __IO uint32_t REG_KM4_INITZWFRANGE;
        struct
        {
            __IO uint32_t km4_initzwfrange: 32;
        } BITS_14C;
    } u_14C;

    /* 0x0150       0x4000_6150
        0       R/W    km4_retention_mode              1'h0
        29:1    R      RSVD                            29'h0
        30      W1O    km4_init_xo_lock                1'h0
        31      R/W    km4_init_xo_range_en            1'h1
    */
    union
    {
        __IO uint32_t REG_KM4_INIT_CONTROL;
        struct
        {
            __IO uint32_t km4_retention_mode: 1;
            __I uint32_t RESERVED_0: 29;
            __IO uint32_t km4_init_xo_lock: 1;
            __IO uint32_t km4_init_xo_range_en: 1;
        } BITS_150;
    } u_150;

    /* 0x0154       0x4000_6154
        4:0     R      RSVD                            5'h0
        31:5    R/W    km4_init_xo_base                27'h0
    */
    union
    {
        __IO uint32_t km4_control0;
        struct
        {
            __I uint32_t RESERVED_0: 5;
            __IO uint32_t km4_init_xo_base: 27;
        } BITS_154;
    } u_154;

    /* 0x0158       0x4000_6158
        4:0     R      RSVD                            5'h1f
        31:5    R/W    km4_init_xo_top                 27'h1f
    */
    union
    {
        __IO uint32_t km4_control1;
        struct
        {
            __I uint32_t RESERVED_0: 5;
            __IO uint32_t km4_init_xo_top: 27;
        } BITS_158;
    } u_158;

    /* 0x015C       0x4000_615c
        4:0     R/W    RSVD                            0
        8:5     R/W    RSVD                            0
        31:9    R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_BIST_RSTN1;
        struct
        {
            __IO uint32_t RESERVED_2: 5;
            __IO uint32_t RESERVED_1: 4;
            __I uint32_t RESERVED_0: 23;
        } BITS_15C;
    } u_15C;

    /* 0x0160       0x4000_6160
        4:0     R/W    RSVD                            0
        8:5     R/W    RSVD                            0
        31:9    R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_BIST_MODE1;
        struct
        {
            __IO uint32_t RESERVED_2: 5;
            __IO uint32_t RESERVED_1: 4;
            __I uint32_t RESERVED_0: 23;
        } BITS_160;
    } u_160;

    /* 0x0164       0x4000_6164
        4:0     R/W    RSVD                            0
        8:5     R/W    RSVD                            0
        31:9    R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_DRF_BIST_MODE1;
        struct
        {
            __IO uint32_t RESERVED_2: 5;
            __IO uint32_t RESERVED_1: 4;
            __I uint32_t RESERVED_0: 23;
        } BITS_164;
    } u_164;

    /* 0x0168       0x4000_6168
        4:0     R/W    RSVD                            0
        8:5     R/W    RSVD                            0
        31:9    R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DSP_BIST_TEST_RESUME1;
        struct
        {
            __IO uint32_t RESERVED_2: 5;
            __IO uint32_t RESERVED_1: 4;
            __I uint32_t RESERVED_0: 23;
        } BITS_168;
    } u_168;

    /* 0x016C       0x4000_616c
        31:0    R      RSVD                            0
    */
    __IO uint32_t REG_DSP_BIST_START_PAUSE1;

    /* 0x0170       0x4000_6170
        31:0    R      RSVD                            0
    */
    __IO uint32_t REG_DSP_BIST_DONE1;

    /* 0x0174       0x4000_6174
        31:0    R      RSVD                            0
    */
    __IO uint32_t REG_DSP_BIST_FAIL1;

    /* 0x0178       0x4000_6178
        31:0    R      RSVD                            0
    */
    __IO uint32_t REG_DSP_DRF_BIST_DONE1;

    /* 0x017C       0x4000_617c
        31:0    R      RSVD                            0
    */
    __IO uint32_t REG_DSP_DRF_BIST_FAIL1;

    /* 0x0180       0x4000_6180
        3:0     R/W    bist_rstn_rx2_itcm1_g0          0
        15:4    R/W    RSVD                            0
        23:16   R/W    RSVD                            0
        28:24   R/W    RSVD                            0
        31:29   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM1_BIST_RSTN;
        struct
        {
            __IO uint32_t bist_rstn_rx2_itcm1_g0: 4;
            __IO uint32_t RESERVED_3: 12;
            __IO uint32_t RESERVED_2: 8;
            __IO uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_180;
    } u_180;

    /* 0x0184       0x4000_6184
        3:0     R/W    bist_mode_rx2_itcm1_g0          0
        15:4    R/W    RSVD                            0
        23:16   R/W    RSVD                            0
        28:24   R/W    RSVD                            0
        31:29   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM1_BIST_MODE;
        struct
        {
            __IO uint32_t bist_mode_rx2_itcm1_g0: 4;
            __IO uint32_t RESERVED_3: 12;
            __IO uint32_t RESERVED_2: 8;
            __IO uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_184;
    } u_184;

    /* 0x0188       0x4000_6188
        3:0     R/W    bist_mode_drf_rx2_itcm1_g0      0
        15:4    R/W    RSVD                            0
        23:16   R/W    RSVD                            0
        28:24   R/W    RSVD                            0
        31:29   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM1_DRF_BIST_MODE;
        struct
        {
            __IO uint32_t bist_mode_drf_rx2_itcm1_g0: 4;
            __IO uint32_t RESERVED_3: 12;
            __IO uint32_t RESERVED_2: 8;
            __IO uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_188;
    } u_188;

    /* 0x018C       0x4000_618c
        3:0     R/W    bist_test_resume_rx2_itcm1_g0   0
        15:4    R/W    RSVD                            0
        23:16   R/W    RSVD                            0
        28:24   R/W    RSVD                            0
        31:29   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM1_BIST_TEST_RESUME;
        struct
        {
            __IO uint32_t bist_test_resume_rx2_itcm1_g0: 4;
            __IO uint32_t RESERVED_3: 12;
            __IO uint32_t RESERVED_2: 8;
            __IO uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_18C;
    } u_18C;

    /* 0x0190       0x4000_6190
        3:0     R      bist_start_pause_rx2_itcm1_g0   0
        15:4    R      RSVD                            0
        23:16   R      RSVD                            0
        28:24   R      RSVD                            0
        31:29   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM1_BIST_START_PAUSE;
        struct
        {
            __I uint32_t bist_start_pause_rx2_itcm1_g0: 4;
            __I uint32_t RESERVED_3: 12;
            __I uint32_t RESERVED_2: 8;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_190;
    } u_190;

    /* 0x0194       0x4000_6194
        3:0     R      bist_done_rx2_itcm1_g0          0
        15:4    R      RSVD                            0
        23:16   R      RSVD                            0
        28:24   R      RSVD                            0
        31:29   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM1_BIST_DONE;
        struct
        {
            __I uint32_t bist_done_rx2_itcm1_g0: 4;
            __I uint32_t RESERVED_3: 12;
            __I uint32_t RESERVED_2: 8;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_194;
    } u_194;

    /* 0x0198       0x4000_6198
        3:0     R      bist_fail_rx2_itcm1_g0          0
        15:4    R      RSVD                            0
        23:16   R      RSVD                            0
        28:24   R      RSVD                            0
        31:29   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM1_BIST_FAIL;
        struct
        {
            __I uint32_t bist_fail_rx2_itcm1_g0: 4;
            __I uint32_t RESERVED_3: 12;
            __I uint32_t RESERVED_2: 8;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_198;
    } u_198;

    /* 0x019C       0x4000_619c
        3:0     R      bist_done_drf_rx2_itcm1_g0      0
        15:4    R      RSVD                            0
        23:16   R      RSVD                            0
        28:24   R      RSVD                            0
        31:29   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM1_DRF_BIST_DONE;
        struct
        {
            __I uint32_t bist_done_drf_rx2_itcm1_g0: 4;
            __I uint32_t RESERVED_3: 12;
            __I uint32_t RESERVED_2: 8;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_19C;
    } u_19C;

    /* 0x01A0       0x4000_61a0
        3:0     R      bist_fail_drf_rx2_itcm1_g0      0
        15:4    R      RSVD                            0
        23:16   R      RSVD                            0
        28:24   R      RSVD                            0
        31:29   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM1_DRF_BIST_FAIL;
        struct
        {
            __I uint32_t bist_fail_drf_rx2_itcm1_g0: 4;
            __I uint32_t RESERVED_3: 12;
            __I uint32_t RESERVED_2: 8;
            __I uint32_t RESERVED_1: 5;
            __I uint32_t RESERVED_0: 3;
        } BITS_1A0;
    } u_1A0;

    /* 0x01A4       0x4000_61a4
        1:0     R/W    bist_rstn_rx2_dtcm0_g0          0
        11:2    R/W    RSVD                            0
        18:12   R/W    bist_rstn_rx2_dtcm1_g0          0
        26:19   R/W    RSVD                            0
        31:27   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DTCM0_1_BIST_RSTN;
        struct
        {
            __IO uint32_t bist_rstn_rx2_dtcm0_g0: 2;
            __IO uint32_t RESERVED_2: 10;
            __IO uint32_t bist_rstn_rx2_dtcm1_g0: 7;
            __IO uint32_t RESERVED_1: 8;
            __I uint32_t RESERVED_0: 5;
        } BITS_1A4;
    } u_1A4;

    /* 0x01A8       0x4000_61a8
        1:0     R/W    bist_mode_rx2_dtcm0_g0          0
        11:2    R/W    RSVD                            0
        18:12   R/W    bist_mode_rx2_dtcm1_g0          0
        26:19   R/W    RSVD                            0
        31:27   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DTCM0_1_BIST_MODE;
        struct
        {
            __IO uint32_t bist_mode_rx2_dtcm0_g0: 2;
            __IO uint32_t RESERVED_2: 10;
            __IO uint32_t bist_mode_rx2_dtcm1_g0: 7;
            __IO uint32_t RESERVED_1: 8;
            __I uint32_t RESERVED_0: 5;
        } BITS_1A8;
    } u_1A8;

    /* 0x01AC       0x4000_61ac
        1:0     R/W    bist_mode_drf_rx2_dtcm0_g0      0
        11:2    R/W    RSVD                            0
        18:12   R/W    bist_mode_drf_rx2_dtcm1_g0      0
        26:19   R/W    RSVD                            0
        31:27   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DTCM0_1_DRF_BIST_MODE;
        struct
        {
            __IO uint32_t bist_mode_drf_rx2_dtcm0_g0: 2;
            __IO uint32_t RESERVED_2: 10;
            __IO uint32_t bist_mode_drf_rx2_dtcm1_g0: 7;
            __IO uint32_t RESERVED_1: 8;
            __I uint32_t RESERVED_0: 5;
        } BITS_1AC;
    } u_1AC;

    /* 0x01B0       0x4000_61b0
        1:0     R/W    bist_test_resume_rx2_dtcm0_g0   0
        11:2    R/W    RSVD                            0
        18:12   R/W    bist_test_resume_rx2_dtcm1_g0   0
        26:19   R/W    RSVD                            0
        31:27   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_DTCM0_1_BIST_TEST_RESUME;
        struct
        {
            __IO uint32_t bist_test_resume_rx2_dtcm0_g0: 2;
            __IO uint32_t RESERVED_2: 10;
            __IO uint32_t bist_test_resume_rx2_dtcm1_g0: 7;
            __IO uint32_t RESERVED_1: 8;
            __I uint32_t RESERVED_0: 5;
        } BITS_1B0;
    } u_1B0;

    /* 0x01B4       0x4000_61b4
        1:0     R/W    bist_start_pause_rx2_dtcm0_g0   0
        11:2    R/W    RSVD                            0
        18:12   R/W    bist_start_pause_rx2_dtcm1_g0   0
        26:19   R/W    RSVD                            0
        31:27   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM0_1_BIST_START_PAUSE;
        struct
        {
            __IO uint32_t bist_start_pause_rx2_dtcm0_g0: 2;
            __IO uint32_t RESERVED_2: 10;
            __IO uint32_t bist_start_pause_rx2_dtcm1_g0: 7;
            __IO uint32_t RESERVED_1: 8;
            __I uint32_t RESERVED_0: 5;
        } BITS_1B4;
    } u_1B4;

    /* 0x01B8       0x4000_61b8
        1:0     R/W    bist_done_rx2_dtcm0_g0          0
        11:2    R/W    RSVD                            0
        18:12   R/W    bist_done_rx2_dtcm1_g0          0
        26:19   R/W    RSVD                            0
        31:27   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM0_1_BIST_DONE;
        struct
        {
            __IO uint32_t bist_done_rx2_dtcm0_g0: 2;
            __IO uint32_t RESERVED_2: 10;
            __IO uint32_t bist_done_rx2_dtcm1_g0: 7;
            __IO uint32_t RESERVED_1: 8;
            __I uint32_t RESERVED_0: 5;
        } BITS_1B8;
    } u_1B8;

    /* 0x01BC       0x4000_61bc
        1:0     R/W    bist_fail_rx2_dtcm0_g0          0
        11:2    R/W    RSVD                            0
        18:12   R/W    bist_fail_rx2_dtcm1_g0          0
        26:19   R/W    RSVD                            0
        31:27   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM0_1_BIST_FAIL;
        struct
        {
            __IO uint32_t bist_fail_rx2_dtcm0_g0: 2;
            __IO uint32_t RESERVED_2: 10;
            __IO uint32_t bist_fail_rx2_dtcm1_g0: 7;
            __IO uint32_t RESERVED_1: 8;
            __I uint32_t RESERVED_0: 5;
        } BITS_1BC;
    } u_1BC;

    /* 0x01C0       0x4000_61c0
        1:0     R/W    bist_done_drf_rx2_dtcm0_g0      0
        11:2    R/W    RSVD                            0
        18:12   R/W    bist_done_drf_rx2_dtcm1_g0      0
        26:19   R/W    RSVD                            0
        31:27   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM0_1_DRF_BIST_DONE;
        struct
        {
            __IO uint32_t bist_done_drf_rx2_dtcm0_g0: 2;
            __IO uint32_t RESERVED_2: 10;
            __IO uint32_t bist_done_drf_rx2_dtcm1_g0: 7;
            __IO uint32_t RESERVED_1: 8;
            __I uint32_t RESERVED_0: 5;
        } BITS_1C0;
    } u_1C0;

    /* 0x01C4       0x4000_61c4
        1:0     R/W    bist_fail_drf_rx2_dtcm0_g0      0
        11:2    R/W    RSVD                            0
        18:12   R/W    bist_fail_drf_rx2_dtcm1_g0      0
        26:19   R/W    RSVD                            0
        31:27   R      RSVD                            0
    */
    union
    {
        __IO uint32_t REG_ITCM0_1_DRF_BIST_FAIL;
        struct
        {
            __IO uint32_t bist_fail_drf_rx2_dtcm0_g0: 2;
            __IO uint32_t RESERVED_2: 10;
            __IO uint32_t bist_fail_drf_rx2_dtcm1_g0: 7;
            __IO uint32_t RESERVED_1: 8;
            __I uint32_t RESERVED_0: 5;
        } BITS_1C4;
    } u_1C4;

    /* 0x1C8        0x4000_61c8
        31:0    R      RSVD                            32'h0
    */
    __IO uint32_t REG_0x1C8;

    /* 0x1CC        0x4000_61cc
        0       R      can_ram_drf_start_pause         1'h0
        1       R      can_ram_drf_bist_fail           1'h0
        2       R      can_ram_drf_bist_done           1'h0
        3       R      can_ram_bist_fail               1'h0
        4       R      can_ram_bist_done               1'h0
        24:5    R      RSVD                            20'h0
        25      R/W    can_ram_bist_grp_en             1'h0
        26      R/W    can_ram_dyn_read_en             1'h0
        27      R/W    can_ram_bist_loop_mode          1'h0
        28      R/W    can_ram_drf_test_resume         1'h0
        29      R/W    can_ram_drf_bist_mode           1'h0
        30      R/W    can_ram_bist_mode               1'h0
        31      R/W    can_ram_bist_rstn               1'h0
    */
    union
    {
        __IO uint32_t REG_CAN_MBIST;
        struct
        {
            __I uint32_t can_ram_drf_start_pause: 1;
            __I uint32_t can_ram_drf_bist_fail: 1;
            __I uint32_t can_ram_drf_bist_done: 1;
            __I uint32_t can_ram_bist_fail: 1;
            __I uint32_t can_ram_bist_done: 1;
            __I uint32_t RESERVED_0: 20;
            __IO uint32_t can_ram_bist_grp_en: 1;
            __IO uint32_t can_ram_dyn_read_en: 1;
            __IO uint32_t can_ram_bist_loop_mode: 1;
            __IO uint32_t can_ram_drf_test_resume: 1;
            __IO uint32_t can_ram_drf_bist_mode: 1;
            __IO uint32_t can_ram_bist_mode: 1;
            __IO uint32_t can_ram_bist_rstn: 1;
        } BITS_1CC;
    } u_1CC;

    /* 0x1D0        0x4000_61d0
        0       R      display_128x32_drf_start_pause  1'h0
        1       R      display_128x32_drf_bist_fail    1'h0
        2       R      display_128x32_drf_bist_done    1'h0
        3       R      display_128x32_bist_fail        1'h0
        4       R      display_128x32_bist_done        1'h0
        24:5    R      RSVD                            20'h0
        25      R/W    display_128x32_bist_grp_en      1'h0
        26      R/W    display_128x32_dyn_read_en      1'h0
        27      R/W    display_128x32_bist_loop_mode   1'h0
        28      R/W    display_128x32_drf_test_resume  1'h0
        29      R/W    display_128x32_drf_bist_mode    1'h0
        30      R/W    display_128x32_bist_mode        1'h0
        31      R/W    display_128x32_bist_rstn        1'h0
    */
    union
    {
        __IO uint32_t REG_DISPLAY_128X32_MBIST;
        struct
        {
            __I uint32_t display_128x32_drf_start_pause: 1;
            __I uint32_t display_128x32_drf_bist_fail: 1;
            __I uint32_t display_128x32_drf_bist_done: 1;
            __I uint32_t display_128x32_bist_fail: 1;
            __I uint32_t display_128x32_bist_done: 1;
            __I uint32_t RESERVED_0: 20;
            __IO uint32_t display_128x32_bist_grp_en: 1;
            __IO uint32_t display_128x32_dyn_read_en: 1;
            __IO uint32_t display_128x32_bist_loop_mode: 1;
            __IO uint32_t display_128x32_drf_test_resume: 1;
            __IO uint32_t display_128x32_drf_bist_mode: 1;
            __IO uint32_t display_128x32_bist_mode: 1;
            __IO uint32_t display_128x32_bist_rstn: 1;
        } BITS_1D0;
    } u_1D0;

    /* 0x1D4        0x4000_61d4
        0       R      display_672x24_drf_start_pause  1'h0
        1       R      display_672x24_drf_bist_fail    1'h0
        2       R      display_672x24_drf_bist_done    1'h0
        3       R      display_672x24_bist_fail        1'h0
        4       R      display_672x24_bist_done        1'h0
        24:5    R      RSVD                            20'h0
        25      R/W    display_672x24_bist_grp_en      1'h0
        26      R/W    display_672x24_dyn_read_en      1'h0
        27      R/W    display_672x24_bist_loop_mode   1'h0
        28      R/W    display_672x24_drf_test_resume  1'h0
        29      R/W    display_672x24_drf_bist_mode    1'h0
        30      R/W    display_672x24_bist_mode        1'h0
        31      R/W    display_672x24_bist_rstn        1'h0
    */
    union
    {
        __IO uint32_t REG_DISPLAY_672X24_MBIST;
        struct
        {
            __I uint32_t display_672x24_drf_start_pause: 1;
            __I uint32_t display_672x24_drf_bist_fail: 1;
            __I uint32_t display_672x24_drf_bist_done: 1;
            __I uint32_t display_672x24_bist_fail: 1;
            __I uint32_t display_672x24_bist_done: 1;
            __I uint32_t RESERVED_0: 20;
            __IO uint32_t display_672x24_bist_grp_en: 1;
            __IO uint32_t display_672x24_dyn_read_en: 1;
            __IO uint32_t display_672x24_bist_loop_mode: 1;
            __IO uint32_t display_672x24_drf_test_resume: 1;
            __IO uint32_t display_672x24_drf_bist_mode: 1;
            __IO uint32_t display_672x24_bist_mode: 1;
            __IO uint32_t display_672x24_bist_rstn: 1;
        } BITS_1D4;
    } u_1D4;

    /* 0x1D8        0x4000_61d8
        0       R      pke_imem_bist_done              1'h0
        28:1    R      RSVD                            28'h0
        29      R/W    pke_imem_bist_grp_en            1'h0
        30      R/W    pke_imem_bist_mode              1'h0
        31      R/W    pke_imem_bist_rstn              1'h0
    */
    union
    {
        __IO uint32_t REG_PKE_IMEM_MBIST;
        struct
        {
            __I uint32_t pke_imem_bist_done: 1;
            __I uint32_t RESERVED_0: 28;
            __IO uint32_t pke_imem_bist_grp_en: 1;
            __IO uint32_t pke_imem_bist_mode: 1;
            __IO uint32_t pke_imem_bist_rstn: 1;
        } BITS_1D8;
    } u_1D8;

    /* 0x1DC        0x4000_61dc
        31:0    R      pke_imem_misr_dataout_0_31_00   32'h0
    */
    union
    {
        __IO uint32_t REG_PKE_IMEM_MBIST_DATAOUT_0_31_00;
        struct
        {
            __I uint32_t pke_imem_misr_dataout_0_31_00: 32;
        } BITS_1DC;
    } u_1DC;

    /* 0x1E0        0x4000_61e0
        31:0    R      pke_imem_misr_dataout_0_63_32   32'h0
    */
    union
    {
        __IO uint32_t REG_PKE_IMEM_MBIST_DATAOUT_0_63_32;
        struct
        {
            __I uint32_t pke_imem_misr_dataout_0_63_32: 32;
        } BITS_1E0;
    } u_1E0;

    /* 0x1E4        0x4000_61e4
        0       R      pke_mmem_drf_start_pause        1'h0
        2:1     R      pke_mmem_drf_bist_fail          2'h0
        3       R      pke_mmem_drf_bist_done          1'h0
        5:4     R      pke_mmem_bist_fail              2'h0
        6       R      pke_mmem_bist_done              1'h0
        23:7    R      RSVD                            17'h0
        25:24   R/W    pke_mmem_bist_grp_en            2'h0
        26      R/W    pke_mmem_dyn_read_en            1'h0
        27      R/W    pke_mmem_bist_loop_mode         1'h0
        28      R/W    pke_mmem_drf_test_resume        1'h0
        29      R/W    pke_mmem_drf_bist_mode          1'h0
        30      R/W    pke_mmem_bist_mode              1'h0
        31      R/W    pke_mmem_bist_rstn              1'h0
    */
    union
    {
        __IO uint32_t REG_PKE_MMEM_MBIST;
        struct
        {
            __I uint32_t pke_mmem_drf_start_pause: 1;
            __I uint32_t pke_mmem_drf_bist_fail: 2;
            __I uint32_t pke_mmem_drf_bist_done: 1;
            __I uint32_t pke_mmem_bist_fail: 2;
            __I uint32_t pke_mmem_bist_done: 1;
            __I uint32_t RESERVED_0: 17;
            __IO uint32_t pke_mmem_bist_grp_en: 2;
            __IO uint32_t pke_mmem_dyn_read_en: 1;
            __IO uint32_t pke_mmem_bist_loop_mode: 1;
            __IO uint32_t pke_mmem_drf_test_resume: 1;
            __IO uint32_t pke_mmem_drf_bist_mode: 1;
            __IO uint32_t pke_mmem_bist_mode: 1;
            __IO uint32_t pke_mmem_bist_rstn: 1;
        } BITS_1E4;
    } u_1E4;

    /* 0x1E8        0x4000_61e8
        0       R      pke_tmem_drf_start_pause        1'h0
        2:1     R      pke_tmem_drf_bist_fail          2'h0
        3       R      pke_tmem_drf_bist_done          1'h0
        5:4     R      pke_tmem_bist_fail              2'h0
        6       R      pke_tmem_bist_done              1'h0
        23:7    R      RSVD                            17'h0
        25:24   R/W    pke_tmem_bist_grp_en            2'h0
        26      R/W    pke_tmem_dyn_read_en            1'h0
        27      R/W    pke_tmem_bist_loop_mode         1'h0
        28      R/W    pke_tmem_drf_test_resume        1'h0
        29      R/W    pke_tmem_drf_bist_mode          1'h0
        30      R/W    pke_tmem_bist_mode              1'h0
        31      R/W    pke_tmem_bist_rstn              1'h0
    */
    union
    {
        __IO uint32_t REG_PKE_TMEM_MBIST;
        struct
        {
            __I uint32_t pke_tmem_drf_start_pause: 1;
            __I uint32_t pke_tmem_drf_bist_fail: 2;
            __I uint32_t pke_tmem_drf_bist_done: 1;
            __I uint32_t pke_tmem_bist_fail: 2;
            __I uint32_t pke_tmem_bist_done: 1;
            __I uint32_t RESERVED_0: 17;
            __IO uint32_t pke_tmem_bist_grp_en: 2;
            __IO uint32_t pke_tmem_dyn_read_en: 1;
            __IO uint32_t pke_tmem_bist_loop_mode: 1;
            __IO uint32_t pke_tmem_drf_test_resume: 1;
            __IO uint32_t pke_tmem_drf_bist_mode: 1;
            __IO uint32_t pke_tmem_bist_mode: 1;
            __IO uint32_t pke_tmem_bist_rstn: 1;
        } BITS_1E8;
    } u_1E8;

    __IO uint32_t RSVD_0x1ec[37];

    /* 0x0280       0x4000_6280
        9:0     R/W    EFUSE_WP_START                  10'h1a6
        11:10   R      RSVD                            2'h0
        21:12   R/W    EFUSE_WP_END                    10'h1ff
        29:22   R      RSVD                            8'h0
        30      W1O    EFUSE_WP_LOCK                   1'h0
        31      R/W    EFUSE_WP_EN                     1'h0
    */
    union
    {
        __IO uint32_t efuse_write_protect;
        struct
        {
            __IO uint32_t EFUSE_WP_START: 10;
            __I uint32_t RESERVED_1: 2;
            __IO uint32_t EFUSE_WP_END: 10;
            __I uint32_t RESERVED_0: 8;
            __IO uint32_t EFUSE_WP_LOCK: 1;
            __IO uint32_t EFUSE_WP_EN: 1;
        } BITS_280;
    } u_280;

    /* 0x0284       0x4000_6284
        9:0     R/W    EFUSE_RP_START                  10'h1a6
        11:10   R      RSVD                            2'h0
        21:12   R/W    EFUSE_RP_END                    10'h1ff
        29:22   R      RSVD                            8'h0
        30      W1O    EFUSE_RP_LOCK                   1'h0
        31      R/W    EFUSE_RP_EN                     1'h0
    */
    union
    {
        __IO uint32_t efuse_read_protect;
        struct
        {
            __IO uint32_t EFUSE_RP_START: 10;
            __I uint32_t RESERVED_1: 2;
            __IO uint32_t EFUSE_RP_END: 10;
            __I uint32_t RESERVED_0: 8;
            __IO uint32_t EFUSE_RP_LOCK: 1;
            __IO uint32_t EFUSE_RP_EN: 1;
        } BITS_284;
    } u_284;

    /* 0x0288       0x4000_6288
        0       R/W    data_ram_err_flag_en            1'h0
        1       R/W    data_ram_err_int_en             1'h0
        2       R/W    data_ram_rd_flag_clr            1'h0
        3       R/W    data_ram_err_int_clr            1'h0
        31:4    R      RSVD                            28'h0
    */
    union
    {
        __IO uint32_t REG_DATA_RAM_ERR_0;
        struct
        {
            __IO uint32_t data_ram_err_flag_en: 1;
            __IO uint32_t data_ram_err_int_en: 1;
            __IO uint32_t data_ram_rd_flag_clr: 1;
            __IO uint32_t data_ram_err_int_clr: 1;
            __I uint32_t RESERVED_0: 28;
        } BITS_288;
    } u_288;

    /* 0x028C       0x4000_628c
        17:0    R      data_ram_err_flag               18'h0
        18      R      data_ram_err_int                1'h0
        31:19   R      RSVD                            13'h0
    */
    union
    {
        __IO uint32_t REG_DATA_RAM_ERR_1;
        struct
        {
            __I uint32_t data_ram_err_flag: 18;
            __I uint32_t data_ram_err_int: 1;
            __I uint32_t RESERVED_0: 13;
        } BITS_28C;
    } u_28C;

    __IO uint32_t RSVD_0x290[28];

    /* 0x300        0x4000_6300
        0       R/W    SYS_TIMER_RST_SEL               1'h1
        1       R/W    SYS_TIMER_MANUAL_RSTB           1'h0
        31:2    R      RSVD                            30'h0
    */
    union
    {
        __IO uint32_t REG_SYS_TIMER_CTL;
        struct
        {
            __IO uint32_t SYS_TIMER_RST_SEL: 1;
            __IO uint32_t SYS_TIMER_MANUAL_RSTB: 1;
            __I uint32_t RESERVED_0: 30;
        } BITS_300;
    } u_300;

    /* 0x304        0x4000_6304
        31:0    R      SYS_TIMER_CNT                   32'h0
    */
    union
    {
        __IO uint32_t REG_SYS_TIMER_CNT;
        struct
        {
            __I uint32_t SYS_TIMER_CNT: 32;
        } BITS_304;
    } u_304;

    /* 0x308        0x4000_6308
        31:0    R      RSVD                            32'h0
    */
    __IO uint32_t REG_SYS_TIMER_RSVD;

    /* 0x30C        0x4000_630c
        31:0    R      SYS_TIMER_VERSION               32'h2306270A
    */
    union
    {
        __IO uint32_t REG_SYS_TIMER_VERSION;
        struct
        {
            __I uint32_t SYS_TIMER_VERSION: 32;
        } BITS_30C;
    } u_30C;

} SoC_VENDOR_REG_TypeDef;

/* ================================================================================ */
/* ================                    Key Scan                    ================ */
/* ================================================================================ */

/**
  * @brief Key Scan. (KeyScan)
  */

typedef struct                                      /*!< KeyScan Structure */
{

    __IO uint32_t CLKDIV;               /*!< 0x00 */
    __IO uint32_t TIMERCR;              /*!< 0x04 */
    __IO uint32_t CR;                   /*!< 0x08 */
    __IO uint32_t COLCR;                /*!< 0x0C */
    __IO uint32_t ROWCR;                /*!< 0x10 */
    __I  uint32_t FIFODATA;             /*!< 0x14 */
    __IO uint32_t INTMASK;              /*!< 0x18 */
    __IO uint32_t INTCLR;               /*!< 0x1C */
    __I  uint32_t STATUS;               /*!< 0x20 */

} KEYSCAN_TypeDef;

/* ======================================================== */
/* ================                      GPIO                       ================ */
/* ======================================================== */

/**
  * @brief General purpose input and output. (GPIO)
  */

typedef struct
{
    __IO uint32_t DATAOUT;                              /*!< 0x00 Data register: data output */
    __IO uint32_t DATADIR;                              /*!< 0x04 Data direction register */
    __IO uint32_t DATASRC;                              /*!< 0x08 Data source register  */
    uint32_t RSVD[9];                                   /*!< 0x0C ~ 0x2C */
    __IO uint32_t INTEN;                                /*!< 0x30 Interrupt enable register */
    __IO uint32_t INTMASK;                              /*!< 0x34 Interrupt mask register */
    __IO uint32_t INTTYPE;                              /*!< 0x38 Interrupt level register */
    __IO uint32_t INTPOLARITY;                          /*!< 0x3C Interrupt polarity register */
    __IO uint32_t INTSTATUS;                            /*!< 0x40 Interrupt status of Port A  */
    __IO uint32_t
    RAWINTSTATUS;                         /*!< 0x44 Raw interrupt status of Port A (premasking) */
    __IO uint32_t DEBOUNCE;                             /*!< 0x48 Debounce enable register */
    __O  uint32_t INTCLR;                               /*!< 0x4C clear interrupt register */
    __I  uint32_t DATAIN;                               /*!< 0x50 external port register */
    uint32_t RSVD1[3];                                  /*!< 0x54 ~ 0x5C */
    __IO uint32_t
    LSSYNC;                               /*!< 0x60 Level-sensitive synchronization enable register*/
    uint32_t RSVD2;                                     /*!< 0x64 */
    //__I  uint32_t IDCODE;                               /*!< ID code register */
    __IO uint32_t INTBOTHEDGE;                          /*!< 0x68 Both edge to trigger interrupt*/
} GPIO_TypeDef;

/* ======================================================= */
/* ================                      PWM                     ================ */
/* ======================================================= */

/**
  * @brief PWM
  */
typedef struct
{
    __IO uint32_t CR;                   /*!< 0x00*/
} PWM_TypeDef;


/* ======================================================= */
/* ================                 Peri clock reg               ================ */
/* ======================================================= */

/**
  * @brief PWM
  */
typedef struct
{
    __IO uint32_t CLKSELE;                    /*!< 0x348*/
    uint32_t RSVD[4];
    __IO uint32_t CLK_SRCL;
    __IO uint32_t CLK_SRCH;                   /*!< 0x360*/
    __IO uint32_t PWM0_CTRL_L;
    __IO uint32_t PWM0_CTRL_H;
    __IO uint32_t PWM1_CTRL_L;
    __IO uint32_t PWM1_CTRL_H;                /*!< 0x370*/
    __IO uint32_t PWM2_CTRL_L;
    __IO uint32_t PWM2_CTRL_H;
    __IO uint32_t PWM3_CTRL_L;
    __IO uint32_t PWM3_CTRL_H;                /*!< 0x380*/
    __IO uint32_t TIM_EVT_CTRL;

} Peri_ClockGate_TypeDef;

/* ======================================================= */
/* ================                      ICG                     ================ */
/* ======================================================= */

/**
  * @brief cache for flash
  */
typedef struct
{
    __IO uint32_t CTRL0;            /*!< ICG cells control address register0, Address Offset: 0x00*/
    __IO uint32_t CTRL1;            /*!< ICG cells control address register1, Address Offset: 0x04*/
    __IO uint32_t CACHE_RAM_CTRL;   /*!< icache twoway control, Address Offset: 0x08*/
} ICG_TypeDef;

/* ======================================================= */
/* ================                      CACHE                     ================ */
/* ======================================================= */

/**
  * @brief cache for flash
  */
typedef struct
{
    __IO uint32_t CACHE_ENABLE;          /*!< SPIC cache Enable Register, Address Offset: 0x00*/
    __IO uint32_t FLUSH;                 /*!< Cache Flush register, Address Offset: 0x04*/
    __IO uint32_t INTR;                  /*!< Cache Interrupt register, Address Offset: 0x08*/
    __IO uint32_t RST_CNT;           /*!< Cache Reset Counter register, Address Offset: 0x0C*/
    __IO uint32_t RD_EVT_CNT;        /*!< Cache Read Event Counter register, Address Offset: 0x10*/
    __IO uint32_t HIT_EVT_CNT;       /*!< Cache HIT Event Counter register, Address Offset: 0x14*/
    __IO uint32_t HIT_LSTW_EVT_CNT;  /*!< Cache Hit lastway event counter register, Offset: 0x18*/
    __IO uint32_t RD_PND_CNT;        /*!< Cache Read pending counter register, Offset: 0x1c*/
} CACHE_TypeDef;

/* ================================================================================ */
/* ================                      SPIC                      ================ */
/* ================================================================================ */
typedef struct
{
    __IO uint32_t CTRLR0;               /*!< Control reg 0,                         offset: 0x000 */
    __IO uint32_t RX_NDF;               /*!< User mode rx data data frame counter,  offset: 0x004 */
    __IO uint32_t SSIENR;               /*!< Enable reg,                            offset: 0x008 */
    __IO uint32_t MWCR;                 /*!< N/A,                                   offset: 0x00C */
    __IO uint32_t SER;                  /*!< Slave enable reg,                      offset: 0x010 */
    __IO uint32_t BAUDR;                /*!< Baudrate select reg,                   offset: 0x014 */
    __IO uint32_t TXFTLR;               /*!< Tx FIFO threshold level,               offset: 0x018 */
    __IO uint32_t RXFTLR;               /*!< Rx FIFO threshold level,               offset: 0x01C */
    __IO uint32_t TXFLR;                /*!< Tx FIFO level reg,                     offset: 0x020 */
    __IO uint32_t RXFLR;                /*!< Rx FIFO level reg,                     offset: 0x024 */
    __IO uint32_t SR;                   /*!< Status reg,                            offset: 0x028 */
    __IO uint32_t IMR;                  /*!< Interrupt mask reg,                    offset: 0x02C */
    __IO uint32_t ISR;                  /*!< Interrupt status reg,                  offset: 0x030 */
    __IO uint32_t RISR;                 /*!< Raw interrupt status reg,              offset: 0x034 */
    __IO uint32_t TXOICR;               /*!< Tx FIFO overflow interrupt clear reg,  offset: 0x038 */
    __IO uint32_t RXOICR;               /*!< Rx FIFO overflow interrupt clear reg,  offset: 0x03C */
    __IO uint32_t RXUICR;               /*!< Rx FIFO underflow interrupt clear reg, offset: 0x040 */
    __IO uint32_t MSTICR;               /*!< Master error interrupt clear reg,      offset: 0x044 */
    __IO uint32_t ICR;                  /*!< Interrupt clear reg,                   offset: 0x048 */
    __IO uint32_t DMACR;                /*!< DMA control reg,                       offset: 0x04C */
    __IO uint32_t DMATDLR;              /*!< DMA transimit data level reg,          offset: 0x050 */
    __IO uint32_t DMARDLR;              /*!< DMA revceive data level reg,           offset: 0x054 */
    __IO uint32_t IDR;                  /*!< Identiation reg,                       offset: 0x058 */
    __IO uint32_t SPIC_VERSION;         /*!< Version ID reg,                        offset: 0x05C */
    union
    {
        __IO uint8_t  BYTE;
        __IO uint16_t HALF;
        __IO uint32_t WORD;
    } DR[16];                           /*!< Data reg,                              offset: 0x060 */
    __IO uint32_t DM_DR[16];            /*!< Data mask data register,               offset: 0x0A0 */
    __IO uint32_t READ_FAST_SINGLE;     /*!< Fast read data cmd of flash,           offset: 0x0E0 */
    __IO uint32_t READ_DUAL_DATA;       /*!< Dual output read cmd of flash,         offset: 0x0E4 */
    __IO uint32_t READ_DUAL_ADDR_DATA;  /*!< Dual I/O read cmd of flash,            offset: 0x0E8 */
    __IO uint32_t READ_QUAD_DATA;       /*!< Quad output read cmd of flash,         offset: 0x0EC */
    __IO uint32_t READ_QUAD_ADDR_DATA;  /*!< Quad I/O read cmd of flash,            offset: 0x0F0 */
    __IO uint32_t WRITE_SINGLE;         /*!< Page program cmd of flash,             offset: 0x0F4 */
    __IO uint32_t WRITE_DUAL_DATA;      /*!< Dual data input program cmd of flash,  offset: 0x0F8 */
    __IO uint32_t WRITE_DUAL_ADDR_DATA; /*!< Dual addr & data program cmd of flash, offset: 0x0FC */
    __IO uint32_t WRITE_QUAD_DATA;      /*!< Quad data input program cmd of flash,  offset: 0x100 */
    __IO uint32_t WRITE_QUAD_ADDR_DATA; /*!< Quad addr & data program cmd of flash, offset: 0x104 */
    __IO uint32_t WRITE_ENABLE;         /*!< Write enabe cmd of flash,              offset: 0x108 */
    __IO uint32_t READ_STATUS;          /*!< Read status cmd of flash,              offset: 0x10C */
    __IO uint32_t CTRLR2;               /*!< Control reg 2,                         offset: 0x110 */
    __IO uint32_t FBAUDR;               /*!< Fast baudrate select,                  offset: 0x114 */
    __IO uint32_t USER_LENGTH;          /*!< Addr length reg,                       offset: 0x118 */
    __IO uint32_t AUTO_LENGTH;          /*!< Auto addr length reg,                  offset: 0x11C */
    __IO uint32_t VALID_CMD;            /*!< Valid cmd reg,                         offset: 0x120 */
    __IO uint32_t FLASH_SIZE_R;           /*!< Flash size reg,                        offset: 0x124 */
    __IO uint32_t FLUSH_FIFO;           /*!< Flush FIFO reg,                        offset: 0x128 */
    __IO uint32_t DUM_BYTE;             /*!< Dummy byte value,                      offset: 0x12C */
    __IO uint32_t TX_NDF;               /*!< Tx NDF,                                offset: 0x130 */
    __IO uint32_t DEVICE_INFO;          /*!< Device info,                           offset: 0x134 */
    __IO uint32_t TPR0;                 /*!< Timing parameters,                     offset: 0x138 */
    __IO uint32_t AUTO_LENGTH2;         /*!< Auto addr length reg 2,                offset: 0x13C */
} SPIC_TypeDef;

/* ================================================================================ */
/* ================                      PSRAM                     ================ */
/* ================================================================================ */
typedef struct
{
    __IO uint32_t CCR;           /*!< Configuration control register,          Address offset: 0x000 */
    __IO uint32_t DCR;           /*!< Device configuration control register,   Address offset: 0x004 */
    __IO uint32_t IOCR0;         /*!< I/O configuration control regsiter0,     Address offset: 0x008 */
    __IO uint32_t CSR;           /*!< Controller status register,              Address offset: 0x00c */
    __IO uint32_t DRR;           /*!< Device refresh/power-up register,        Address offset: 0x010 */
    __IO uint32_t RSVD0[4];      /*!< Reserved 0,                              Address offset: 0x014 */
    __IO uint32_t CMD_DPIN_NDGE; /*!< Device cmd/addr pin register (NEDGE),    Address offset: 0x024 */
    __IO uint32_t CMD_DPIN;      /*!< Device cmd/addr pin regsiter (PEDGE),    Address offset: 0x028 */
    __IO uint32_t CR_TDPIN;      /*!< Tie DPIN register (sw ctrl dfi_reset_n), Address offset: 0x02c */
    __IO uint32_t MR_INFO;       /*!< Mode latency information regster,        Address offset: 0x030 */
    __IO uint32_t MR0;           /*!< Device CR0 register,                     Address offset: 0x034 */
    __IO uint32_t MR1;           /*!< Device CR1 register,                     Address offset: 0x038 */
    __IO uint32_t RSVD1[9];      /*!< Reserved 1,                              Address offset: 0x03c */
    __IO uint32_t DPDRI;         /*!< DPIN data index register,                Address offset: 0x060 */
    __IO uint32_t DPDR;          /*!< DPIN data register,                      Address offset: 0x064 */
    __IO uint32_t RSVD2[35];     /*!< Reserved 2,                              Address offset: 0x068 */
    __IO uint32_t PCTL_SVN_ID;   /*!< PSRAM_LPC_CTRL version number,           Address offset: 0x0f4 */
    __IO uint32_t PCTL_IDR;      /*!< PSRAM_LPC_CTRL identification register,  Address offset: 0x0f8 */
    __IO uint32_t RSVD3[193];    /*!< Reserved 3,                              Address offset: 0x0fc */
    __IO uint32_t USER0_INDEX;   /*!< User extended index,                     Address offset: 0x400 */
    __IO uint32_t USER0_DATA;    /*!< User extended data,                      Address offset: 0x404 */
} PSRAMC_TypeDef;

/* =========================== RL6736 RXI300 section ===========================*/
typedef struct
{
    __I uint32_t NAME;
    __I uint32_t VER;
    __I uint32_t REV;
    __I uint32_t INST;
    __I uint32_t IMPL_Y;
    __I uint32_t IMPL_D;
    __I uint32_t DEV;
    __I uint32_t PRO_NUM;
    __I uint32_t rsvd0[56];
    __IO uint32_t CCR;
    __I uint32_t rsvd1[63];
    union
    {
        __I uint32_t ELR_0_PLD0;
        struct
        {
            __I uint32_t ERR_SRC: 8;
            __I uint32_t ERR_CMD: 3;
            __I uint32_t ERR_BSTTYPE: 3;
            __I uint32_t rsvd: 2;
            __I uint32_t ERR_BSTLEN: 8;
            __I uint32_t ERR_BSTINDEX: 8;
        } BITS_200;
    } u_200;
    union
    {
        __I uint32_t ELR_0_PLD1;
        struct
        {
            __I uint32_t ERR_BYTEEN: 16;
            __I uint32_t ERR_SIZE: 3;
            __I uint32_t rsvd1: 4;
            __I uint32_t ERR_PROT: 3;
            __I uint32_t ERR_Cache: 4;
            __I uint32_t ERR_Lock: 2;
        } BITS_204;
    } u_204;
    union
    {
        __I uint32_t ELR_0_ID;
        struct
        {
            __I uint32_t ERR_ID;
        } BITS_208;
    } u_208;
    union
    {
        __I uint32_t ELR_0_ADR0;
        struct
        {
            __I uint32_t ERR_ADR0;
        } BITS_20C;
    } u_20C;
    union
    {
        __I uint32_t ELR_0_ADR1;
        struct
        {
            __I uint32_t ERR_ADR1;
        } BITS_210;
    } u_210;
    __I uint32_t rsvd2[7];
    union
    {
        __I uint32_t ELR_0_CODE;
        struct
        {
            __I uint32_t ELR_CODE: 8;
            __I uint32_t rsvd: 24;
        } BITS_230;
    } u_230;
    __I uint32_t rsvd3[2];
    union
    {
        __IO uint32_t ELR_0_INTR_CLR;
        struct
        {
            __IO uint32_t ELR_INTR_CLR: 1;
            __IO uint32_t rsvd: 31;
        } BITS_23C;
    } u_23C;
    union
    {
        __I uint32_t ELR_1_PLD0;
        struct
        {
            __I uint32_t ERR_SRC: 8;
            __I uint32_t ERR_CMD: 3;
            __I uint32_t ERR_BSTTYPE: 3;
            __I uint32_t rsvd: 2;
            __I uint32_t ERR_BSTLEN: 8;
            __I uint32_t ERR_BSTINDEX: 8;
        } BITS_240;
    } u_240;
    union
    {
        __I uint32_t ELR_1_PLD1;
        struct
        {
            __I uint32_t ERR_BYTEEN: 16;
            __I uint32_t ERR_SIZE: 3;
            __I uint32_t rsvd1: 4;
            __I uint32_t ERR_PROT: 3;
            __I uint32_t ERR_Cache: 4;
            __I uint32_t ERR_Lock: 2;
        } BITS_244;
    } u_244;
    union
    {
        __I uint32_t ELR_1_ID;
        struct
        {
            __I uint32_t ERR_ID;
        } BITS_248;
    } u_248;
    union
    {
        __I uint32_t ELR_1_ADR0;
        struct
        {
            __I uint32_t ERR_ADR0;
        } BITS_24C;
    } u_24C;
    union
    {
        __I uint32_t ELR_1_ADR1;
        struct
        {
            __I uint32_t ERR_ADR1;
        } BITS_250;
    } u_250;
    __I uint32_t rsvd4[7];
    union
    {
        __I uint32_t ELR_1_CODE;
        struct
        {
            __I uint32_t ELR_CODE: 8;
            __I uint32_t rsvd: 24;
        } BITS_270;
    } u_270;
    __I uint32_t rsvd5[2];
    union
    {
        __IO uint32_t ELR_0_INTR_CLR;
        struct
        {
            __IO uint32_t ELR_INTR_CLR: 1;
            __IO uint32_t rsvd: 31;
        } BITS_27C;
    } u_27C;

} RXI300_Typedef;
/** @} */ /* End of group RTL876X_Peripheral_Registers_Structures */

/*============================================================================*
 *                              Macros
 *============================================================================*/

/** @defgroup RTL876X_Exported_Macros RTL876X  Exported Macros
    * @brief
  * @{
  */
/* ================================================================================ */
/* ================              Peripheral memory map             ================ */
/* ================================================================================ */
#define SYSTEM_REG_BASE             0x40000000UL
#define PERIPH_REG_BASE             0x40000000UL
#define VENDOR_REG_BASE             0x40006000UL
#define PERI_INT_REG_BASE           0x40006004UL
#define SYSBLKCTRL_REG_BASE         0x40000200UL
#define PINMUX_REG0_BASE            0x40000280UL
#define PINMUX_REG1_BASE            0x400003B0UL
#define PPE_CFG_BASE                0x40005000UL
#define RTK_TIMER_BASE              0x40007000UL
#define MODEMRFCPI_BASE             0x40008000UL
#define DISPLAY_CTRL_BASE           0x40017000UL
#define CAN_BASE                    0x40028000UL
#define IMDC_BASE                   0x40090000UL
#define PKE_BASE                    0x400C0000UL
#define SM3_BASE                    0x40018000UL


#define SPIC_DLY_CTRL_BASE          0x40000300UL
#define DEBOUNCE_CLK_SET_REG_BASE   0x40000344UL
#define PERICLKGAT_REG_BASE         0x40000348UL
#define GPIO0_REG_BASE              0x40001000UL
#define GPIO1_REG_BASE              0x40004800UL
#define RTC_REG_BASE                0x40000100UL//0x40000140UL
#define PF_RTC_REG_BASE             0x40000140UL
#define RTC_LP_REG_BASE             0x40000180UL
#define RTC_LED_REG_BASE            0x40000400UL
#define LPC_REG_BASE                0x40000180UL
#define AON_WDG_REG_BASE            0x40000190UL
#define TIM0_REG_BASE               0x40025000UL
#define TIM1_REG_BASE               0x40025014UL
#define TIM2_REG_BASE               0x40025028UL
#define TIM3_REG_BASE               0x4002503CUL
#define TIM4_REG_BASE               0x40025050UL
#define TIM5_REG_BASE               0x40025064UL
#define TIM6_REG_BASE               0x40025078UL
#define TIM7_REG_BASE               0x4002508CUL
#define PWM0_REG_BASE               0x40000364UL
#define PWM1_REG_BASE               0x40000368UL
#define PWM2_REG_BASE               0x40000374UL
#define PWM3_REG_BASE               0x4000037CUL
#define TIM_CHANNELS_REG_BASE       0x400250A0UL

#define ENH_TIMER0_BASE             0x40007000UL
#define ENH_TIMER1_BASE             (ENH_TIMER0_BASE+0x50 )
#define ENH_TIMER2_BASE             (ENH_TIMER0_BASE+0xa0 )
#define ENH_TIMER3_BASE             (ENH_TIMER0_BASE+0xf0 )
#define ENH_TIMER_CTRL_BASE         (ENH_TIMER0_BASE+0xA00)

#define GDMA_CHANNEL_REG_BASE       0x40027000UL // for platform_1_1_1_20160323 later
#define GDMA_REG_BASE               (GDMA_CHANNEL_REG_BASE + 0x02c0)
#define GDMA_Channel0_BASE          (GDMA_CHANNEL_REG_BASE + 0x0000)
#define GDMA_Channel1_BASE          (GDMA_CHANNEL_REG_BASE + 0x0058)
#define GDMA_Channel2_BASE          (GDMA_CHANNEL_REG_BASE + 0x00b0)
#define GDMA_Channel3_BASE          (GDMA_CHANNEL_REG_BASE + 0x0108)
#define GDMA_Channel4_BASE          (GDMA_CHANNEL_REG_BASE + 0x0160)
#define GDMA_Channel5_BASE          (GDMA_CHANNEL_REG_BASE + 0x01b8)
#define GDMA_Channel6_BASE          (GDMA_CHANNEL_REG_BASE + 0x0210)
#define GDMA_Channel7_BASE          (GDMA_CHANNEL_REG_BASE + 0x0268)
#define GDMA_Channel8_BASE          (GDMA_CHANNEL_REG_BASE + 0x0400)
#define GDMA_Channel9_BASE          (GDMA_CHANNEL_REG_BASE + 0x0458)
#define GDMA_Channel10_BASE         (GDMA_CHANNEL_REG_BASE + 0x04b0)
#define GDMA_Channel11_BASE         (GDMA_CHANNEL_REG_BASE + 0x0508)

#define I8080_REG_BASE              0x40028000UL
#define SPI_CODEC_REG_BASE          0x4002C000UL
#define QDEC_REG_BASE               0x40004000UL
#define SPI2WIRE_REG_BASE           0x40004000UL
#define CODEC_REG_BASE              0x4002C000UL
#define KEYSCAN_REG_BASE            0x40005000UL
#define WDG_REG_BASE                0x40006000UL
#define RANDOM_GEN_REG_BASE         0x40006150UL
#define RXI300_MCU_REG_BASE         0x40026000UL
#define ICG_REG_BASE                0x40026400UL
//#define CAP_TOUCH_REG_BASE          0x40007000UL
#define ADC_REG_BASE                0x40010000UL

#define UART1_REG_BASE              0x40011000UL
#define UART0_REG_BASE              0x40012000UL
#define UART2_REG_BASE              0x40024000UL

#define LOG_UART_REG_BASE           0x40011000UL
#define UART_REG_BASE               0x40012000UL

#define HW_AES_REG_BASE             0x40014000UL
#define IR_REG_BASE                 0x40016800UL
#define PSRAM_REG_BASE              0x40017000UL
#define SPI0_HS_REG_BASE            0x40040000UL
#define RTL_SPI0_BASE               0x40042000UL
#define RTL_SPI1_BASE               0x40042400UL
#define I2C0_REG_BASE               0x40015000UL
#define I2C1_REG_BASE               0x40015400UL
#define I2C2_REG_BASE               0x40015800UL
#define SPI0_REG_BASE               0x40013000UL
#define SPI1_REG_BASE               0x40015C00UL
#define SPI2_REG_BASE               0x40016000UL
#define ISO7816_REG_BASE            0x40016400UL

#define IF8080_REG_BASE             0x40017000UL
#define IF8080_LLI_REG1_BASE        0x40017050UL
#define IF8080_LLI_REG1_GDMA_BASE   0x400170A0UL
#define IF8080_LLI_REG2_BASE        0x40017080UL
#define IF8080_LLI_REG2_GDMA_BASE   0x400170C0UL
#define IF8080_LLI_REG1_OFT_BASE    0x40017070UL
#define IF8080_LLI_REG2_OFT_BASE    0x40017078UL
#define IF8080_LLI_CR_REG_BASE      0x40017094UL

#define BLUEWIZ_REG_BASE            0x40050000UL
#define BT_LE_REG_BASE              0x40051000UL
#define BT_VENDOR_REG_BASE          0x40058000UL
#define GDMA0_REG_BASE              0x40060000UL
#define HCI_DMA_REG_BASE            0x40064000UL
#define HCI_UART_REG_BASE           0x40068000UL
#define SPIC0_REG_BASE              0x40080000UL
#define SPIC1_REG_BASE              0x40084000UL
#define SPIC2_REG_BASE              0x40088000UL
#define SPIC3_REG_BASE              0x4008C000UL
#define H2D_D2H_REG_BASE            0x40023000UL
#define SPORT0_REG_BASE             0x40020000UL
#define SPORT1_REG_BASE             0x40021000UL
#define SPORT2_REG_BASE             0x40022000UL
#define SPDIF_REG_BASE              0x40021800UL
#define SDIO0_REG_BASE              0x4005C000UL

#define SPI0_MASTER_MODE_REG        *((volatile uint32_t *)0x40000308UL)
#define SPI0_MASTER_MODE_BIT        BIT(8)

/** @brief clock divider for peripheral */
#define SYSTEM_CLK_CTRL             *((volatile uint32_t *)0x4000020CUL)
#define CLK_SOURCE_REG_0            *((volatile uint32_t *)0x40000348UL)
#define CLK_SOURCE_REG_1            *((volatile uint32_t *)0x4000035CUL)
#define CLK_SOURCE_REG_2            *((volatile uint32_t *)0x40000360UL)

#define PERI_CLOCKGATE_REG_BASE     0x40000348UL

#define REG_PEON_SYS_CLK_SEL        0x0200
#define REG_PEON_SYS_CLK_SEL_2      0x0208
#define REG_PEON_SYS_CLK_SEL_3      0x020C
// #define REG_SOC_FUNC_EN             0x0210
#define REG_SOC_HCI_COM_FUNC_EN     0x0214
//#define REG_SOC_PERI_FUNC0_EN       0x0218
//#define REG_SOC_PERI_FUNC1_EN       0x021C
// #define REG_PESOC_CLK_CTRL          0x0230
//#define REG_PESOC_PERI_CLK_CTRL0    0x0234
//#define REG_PESOC_PERI_CLK_CTRL1    0x0238
#define REG_PESOC_DSP_SHARE_RAM     0x0250
#define REG_TEST_MODE               0x02a8
#define REG_ANAPAR_PLL1_0           0x0320
#define REG_ANAPAR_PLL3_2           0x0324
#define REG_ANAPAR_PLL5_4           0x0328
#define REG_XTAL_PLL_READY          0x0338

#define GPIO_OUTPUT_OFFSET          0x00
#define GPIO_DIRECTION_OFFSET       0x04

/** @brief AON PAD AREA */
#define REG_PAD_WKEN_ADDRESS        0x20
#define REG_PAD_WK_CTRL_ADDRESS     0x12d
#define REG_PAD_WKPOL_ADDRESS       0x25
#define REG_PAD_O_ADDRESS           0x2A
#define REG_AON_PAD_E_ADDRESS       0x2F
#define REG_AON_PAD_S_ADDRESS       0x34
#define REG_AON_PAD_PU_ADDRESS      0x39
#define REG_AON_PAD_PD_ADDRESS      0x3E
#define REG_AON_PAD_PWRON_ADDRESS   0x4C

/************************ For lowerstack-dev ************************/
#define LOW_STACK_BB_BASE_ADDR                BLUEWIZ_REG_BASE
#define LOW_STACK_BZDMA_REG_BASE              BT_VENDOR_REG_BASE
#define TIMA_CH0                              TIM0
#define TIMA_CH1                              TIM1
#define TIMER_A0_REG_BASE                     TIM0_REG_BASE
#define GDMA0                                 ((GDMA_TypeDef             *) GDMA_REG_BASE)


/* ================================================================================ */
/* ================             Peripheral declaration             ================ */
/* ================================================================================ */
/** @brief System */
#define SYSBLKCTRL                      ((SYS_BLKCTRL_TypeDef      *) SYSBLKCTRL_REG_BASE)
#define SoC_VENDOR                      ((SoC_VENDOR_REG_TypeDef   *) VENDOR_REG_BASE)

/** @brief IO */
#define PINMUX0                         ((PINMUX_TypeDef           *) PINMUX_REG0_BASE)
#define PINMUX1                         ((PINMUX_TypeDef           *) PINMUX_REG1_BASE)
#define DEBOUNCE_CLK_SET                ((DEBOUNCE_CLK_SET_TypeDef *) DEBOUNCE_CLK_SET_REG_BASE)
#define KEYSCAN                         ((KEYSCAN_TypeDef          *) KEYSCAN_REG_BASE)
#define GPIO                            ((GPIO_TypeDef             *) GPIO0_REG_BASE)
#define GPIOA                           ((GPIO_TypeDef             *) GPIO0_REG_BASE)
#define GPIOB                           ((GPIO_TypeDef             *) GPIO1_REG_BASE)
#define QDEC                            ((QDEC_TypeDef             *) QDEC_REG_BASE)
#define I2C0                            ((I2C_TypeDef              *) I2C0_REG_BASE)
#define I2C1                            ((I2C_TypeDef              *) I2C1_REG_BASE)
#define I2C2                            ((I2C_TypeDef              *) I2C2_REG_BASE)
#define SPI0                            ((SPI_TypeDef              *) SPI0_REG_BASE)
#define SPI1                            ((SPI_TypeDef              *) SPI1_REG_BASE)
#define SPI2                            ((SPI_TypeDef              *) SPI2_REG_BASE)
#define TIM0                            ((TIM_TypeDef              *) TIM0_REG_BASE)
#define TIM1                            ((TIM_TypeDef              *) TIM1_REG_BASE)
#define TIM2                            ((TIM_TypeDef              *) TIM2_REG_BASE)
#define TIM3                            ((TIM_TypeDef              *) TIM3_REG_BASE)
#define TIM4                            ((TIM_TypeDef              *) TIM4_REG_BASE)
#define TIM5                            ((TIM_TypeDef              *) TIM5_REG_BASE)
#define TIM6                            ((TIM_TypeDef              *) TIM6_REG_BASE)
#define TIM7                            ((TIM_TypeDef              *) TIM7_REG_BASE)

#define ENH_TIM0                        ((ENH_TIM_TypeDef          *) ENH_TIMER0_BASE)
#define ENH_TIM1                        ((ENH_TIM_TypeDef          *) ENH_TIMER1_BASE)
#define ENH_TIM2                        ((ENH_TIM_TypeDef          *) ENH_TIMER2_BASE)
#define ENH_TIM3                        ((ENH_TIM_TypeDef          *) ENH_TIMER3_BASE)
#define ENH_TIM_CTRL                    ((ENH_TIM_CTRL_TypeDef     *) ENH_TIMER_CTRL_BASE)

#define PWM0_PN                         ((PWM_TypeDef              *) PWM0_REG_BASE)
#define PWM1_PN                         ((PWM_TypeDef              *) PWM1_REG_BASE)
#define PWM2                            ((PWM_TypeDef              *) PWM2_REG_BASE)
#define PWM3                            ((PWM_TypeDef              *) PWM3_REG_BASE)
#define TIM_CHANNELS                    ((TIM_ChannelsTypeDef      *) TIM_CHANNELS_REG_BASE)
#define GDMA_BASE                       ((GDMA_TypeDef             *) GDMA_REG_BASE)
#define GDMA_Channel0                   ((GDMA_ChannelTypeDef      *) GDMA_Channel0_BASE)
#define GDMA_Channel1                   ((GDMA_ChannelTypeDef      *) GDMA_Channel1_BASE)
#define GDMA_Channel2                   ((GDMA_ChannelTypeDef      *) GDMA_Channel2_BASE)
#define GDMA_Channel3                   ((GDMA_ChannelTypeDef      *) GDMA_Channel3_BASE)
#define GDMA_Channel4                   ((GDMA_ChannelTypeDef      *) GDMA_Channel4_BASE)
#define GDMA_Channel5                   ((GDMA_ChannelTypeDef      *) GDMA_Channel5_BASE)
#define GDMA_Channel6                   ((GDMA_ChannelTypeDef      *) GDMA_Channel6_BASE)
#define GDMA_Channel7                   ((GDMA_ChannelTypeDef      *) GDMA_Channel7_BASE)
#define GDMA_Channel8                   ((GDMA_ChannelTypeDef      *) GDMA_Channel8_BASE)
#define GDMA_Channel9                   ((GDMA_ChannelTypeDef      *) GDMA_Channel9_BASE)
#define GDMA_Channel10                  ((GDMA_ChannelTypeDef      *) GDMA_Channel10_BASE)
#define GDMA_Channel11                  ((GDMA_ChannelTypeDef      *) GDMA_Channel11_BASE)
#define ADC                             ((ADC_TypeDef              *) ADC_REG_BASE)
#define UART0                           ((UART_TypeDef             *) UART0_REG_BASE)
#define UART1                           ((UART_TypeDef             *) UART1_REG_BASE)
#define UART2                           ((UART_TypeDef             *) UART2_REG_BASE)

#define SPI3WIRE                        ((SPI3WIRE_TypeDef         *) SPI2WIRE_REG_BASE)
#define IR                              ((IR_TypeDef               *) IR_REG_BASE)
#define RTC                             ((RTC_TypeDef              *) RTC_REG_BASE)
#define RTC_LP                          ((RTC_LP_TypeDef           *) RTC_LP_REG_BASE)
#define RTC_LED                         ((RTC_LED_TypeDef          *) RTC_LED_REG_BASE)
#define LPC                             ((LPC_TypeDef              *) LPC_REG_BASE)
#define HWAES                           ((HW_AES_TypeDef           *) HW_AES_REG_BASE)
#define WDG                             ((WDG_TypeDef              *) WDG_REG_BASE)
#define AON_WDG                         ((AON_WDG_TypeDef          *) AON_WDG_REG_BASE)
#define RAN_GEN                         ((RAN_GEN_TypeDef          *) RANDOM_GEN_REG_BASE)
#define ICG                             ((ICG_TypeDef              *) ICG_REG_BASE)
#define SPIC0                           ((SPIC_TypeDef             *) SPIC0_REG_BASE)
#define SPIC1                           ((SPIC_TypeDef             *) SPIC1_REG_BASE)
#define SPIC2                           ((SPIC_TypeDef             *) SPIC2_REG_BASE)
#define SPIC3                           ((SPIC_TypeDef             *) SPIC3_REG_BASE)
#define PSRAMC                          ((PSRAMC_TypeDef           *) PSRAM_REG_BASE)
#define SDIO                            ((SDIO_TypeDef             *) SDIO0_REG_BASE)
#define CLK_GATE                        ((Peri_ClockGate_TypeDef   *) PERI_CLOCKGATE_REG_BASE)
#define RXI300                          ((RXI300_Typedef           *) RXI300_MCU_REG_BASE)

#define ISO7816                         ((ISO7816_TypeDef*) ISO7816_REG_BASE)
#define IF8080                          ((IF8080_TypeDef*) IF8080_REG_BASE)
#define IF8080_LLPGROUP1                ((IF8080_GDMALLITypeDef*) IF8080_LLI_REG1_BASE)
#define IF8080_LLPGROUP2                ((IF8080_GDMALLITypeDef*) IF8080_LLI_REG2_BASE)
#define IF8080_LLPGROUP1_GDMA           ((IF8080_GDMALLITypeDef*) IF8080_LLI_REG1_GDMA_BASE)
#define IF8080_LLPGROUP2_GDMA           ((IF8080_GDMALLITypeDef*) IF8080_LLI_REG2_GDMA_BASE)
#define IF8080_LLPGROUP1_OFT            ((IF8080_GDMALLIOFTTypeDef*) IF8080_LLI_REG1_OFT_BASE)
#define IF8080_LLPGROUP2_OFT            ((IF8080_GDMALLIOFTTypeDef*) IF8080_LLI_REG2_OFT_BASE)
#define IF8080_GDMA                     ((IF8080_GDMATypeDef*)IF8080_LLI_CR_REG_BASE)

//Add by Vendor
#define LITTLE_ENDIAN                        0
#define BIG_ENDIAN                           1
#define SYSTEM_ENDIAN                        LITTLE_ENDIAN

#define SWAP32(x) ((uint32_t)(                         \
                                                       (((uint32_t)(x) & (uint32_t)0x000000ff) << 24) |            \
                                                       (((uint32_t)(x) & (uint32_t)0x0000ff00) <<  8) |            \
                                                       (((uint32_t)(x) & (uint32_t)0x00ff0000) >>  8) |            \
                                                       (((uint32_t)(x) & (uint32_t)0xff000000) >> 24)))

#define WAP16(x) ((uint16_t)(                         \
                                                      (((uint16_t)(x) & (uint16_t)0x00ff) <<  8) |            \
                                                      (((uint16_t)(x) & (uint16_t)0xff00) >>  8)))

#if SYSTEM_ENDIAN == LITTLE_ENDIAN
#ifndef rtk_le16_to_cpu
#define rtk_cpu_to_le32(x)      ((uint32_t)(x))
#define rtk_le32_to_cpu(x)      ((uint32_t)(x))
#define rtk_cpu_to_le16(x)      ((uint16_t)(x))
#define rtk_le16_to_cpu(x)      ((uint16_t)(x))
#define rtk_cpu_to_be32(x)      SWAP32((x))
#define rtk_be32_to_cpu(x)      SWAP32((x))
#define rtk_cpu_to_be16(x)      WAP16((x))
#define rtk_be16_to_cpu(x)      WAP16((x))
#endif

#elif SYSTEM_ENDIAN == BIG_ENDIAN
#ifndef rtk_le16_to_cpu
#define rtk_cpu_to_le32(x)      SWAP32((x))
#define rtk_le32_to_cpu(x)      SWAP32((x))
#define rtk_cpu_to_le16(x)      WAP16((x))
#define rtk_le16_to_cpu(x)      WAP16((x))
#define rtk_cpu_to_be32(x)      ((uint32_t)(x))
#define rtk_be32_to_cpu(x)      ((uint32_t)(x))
#define rtk_cpu_to_be16(x)      ((uint16_t)(x))
#define rtk_be16_to_cpu(x)      ((uint16_t)(x))
#endif
#endif

#define HAL_READ32(base, addr)            \
    rtk_le32_to_cpu(*((volatile uint32_t *)(base + addr)))

#define HAL_WRITE32(base, addr, value32)  \
    ((*((volatile uint32_t *)(base + addr))) = rtk_cpu_to_le32(value32))

#define HAL_UPDATE32(addr, mask, value32)  \
    HAL_WRITE32(0, addr, (HAL_READ32(0, addr) & ~(mask)) | ((value32) & (mask)))

#define HAL_READ16(base, addr)            \
    rtk_le16_to_cpu(*((volatile uint16_t *)(base + addr)))

#define HAL_WRITE16(base, addr, value)  \
    ((*((volatile uint16_t *)(base + addr))) = rtk_cpu_to_le16(value))

#define HAL_UPDATE16(addr, mask, value16)  \
    HAL_WRITE16(0, addr, (HAL_READ16(0, addr) & ~(mask)) | ((value16) & (mask)))

#define HAL_READ8(base, addr)            \
    (*((volatile uint8_t *)(base + addr)))

#define HAL_WRITE8(base, addr, value)  \
    ((*((volatile uint8_t *)(base + addr))) = value)

#define HAL_UPDATE8(addr, mask, value8)  \
    HAL_WRITE8(0, addr, (HAL_READ8(0, addr) & ~(mask)) | ((value8) & (mask)))

#define BIT0        0x00000001
#define BIT1        0x00000002
#define BIT2        0x00000004
#define BIT3        0x00000008
#define BIT4        0x00000010
#define BIT5        0x00000020
#define BIT6        0x00000040
#define BIT7        0x00000080
#define BIT8        0x00000100
#define BIT9        0x00000200
#define BIT10       0x00000400
#define BIT11       0x00000800
#define BIT12       0x00001000
#define BIT13       0x00002000
#define BIT14       0x00004000
#define BIT15       0x00008000
#define BIT16       0x00010000
#define BIT17       0x00020000
#define BIT18       0x00040000
#define BIT19       0x00080000
#define BIT20       0x00100000
#define BIT21       0x00200000
#define BIT22       0x00400000
#define BIT23       0x00800000
#define BIT24       0x01000000
#define BIT25       0x02000000
#define BIT26       0x04000000
#define BIT27       0x08000000
#define BIT28       0x10000000
#define BIT29       0x20000000
#define BIT30       0x40000000
#define BIT31       0x80000000

#define BIT(_n)     (uint32_t)(1U << (_n))
#define BIT64(_n)   (1ULL << (_n))

/* Uncomment the line below to expanse the "assert_param" macro in the
   Standard Peripheral Library drivers code */
//#define USE_FULL_ASSERT


/** @} */ /* End of group RTL876X_Exported_Macros */


/*============================================================================*
  *                                Functions
 *============================================================================*/
/** @defgroup RTL876X_Exported_Functions RTL876X Sets Exported Functions
    * @brief
    * @{
    */
#ifdef  USE_FULL_ASSERT
/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param  expr: If expr is false, it calls assert_failed function which reports
  *         the name of the source file and the source line number of the call
  *         that failed. If expr is true, it returns no value.
  * @retval None
  */
#define assert_param(expr) ((expr) ? (void)0 : io_assert_failed((uint8_t *)__FILE__, __LINE__))
void io_assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0)
#endif /* USE_FULL_ASSERT */

/**
    * @brief    Read data from aon register safely
    * @param    offset: register address
    * @return   data read from register
    */
extern uint16_t btaon_fast_read_safe(uint16_t offset);
extern uint8_t btaon_fast_read_safe_8b(uint16_t offset);

/**
    * @brief    Write data to aon egister safely
    * @param    offset:  register address
    * @param    data:  data to be writen to register
    * @return
    */
extern void btaon_fast_write_safe(uint16_t offset, uint16_t data);
extern void btaon_fast_write_safe_8b(uint16_t offset, uint8_t data);

/** @} */ /* End of RTL876X_Exported_Functions */


/** @} */ /* End of group RTL876X */

#ifdef __cplusplus
}
#endif
#endif  /* RTL876X_H */

