/**
*********************************************************************************************************
*               Copyright(c) 2024, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtl876x_pinmux.h
* @brief
* @details
* @author    Chuanguo Xue
* @date      2024-07-18
* @version   v1.0
* *********************************************************************************************************
*/


#ifndef _RTL876X_PINMUX_H_
#define _RTL876X_PINMUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl876x.h"


/** @addtogroup 87x3e_PINMUX PINMUX
  * @brief PINMUX driver module.
  * @{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/


/** @defgroup 87x3e_PINMUX_Exported_Constants PINMUX Exported Constants
  * @{
  */



/** @defgroup 87x3e_Pin_Function_Number Pin Function Number
  * @{
  */
#define IDLE_MODE           0   //!< Function for entering idle mode.
#define DIGI_DEBUG          1   //!< Function for digital debugging.

#define I2C0_CLK            5   //!< Function for I2C0 clock line.
#define I2C0_DAT            6   //!< Function for I2C0 data line.
#define I2C1_CLK            7   //!< Function for I2C1 clock line.
#define I2C1_DAT            8   //!< Function for I2C1 data line.
#define PWM2_P              9   //!< Positive terminal for PWM2 function.
#define PWM2_N              10  //!< Negative terminal for PWM2 function.
#define PWM3_P              11  //!< Positive terminal for PWM3 function.
#define PWM3_N              12  //!< Negative terminal for PWM3 function.
#define TIMER_PWM0          13  //!< Timer function for PWM0.
#define TIMER_PWM1          14  //!< Timer function for PWM1.
#define TIMER_PWM2          15  //!< Timer function for PWM2.
#define TIMER_PWM3          16  //!< Timer function for PWM3.
#define TIMER_PWM4          17  //!< Timer function for PWM4.
#define TIMER_PWM5          18  //!< Timer function for PWM5.
#define TIMER_PWM6          19  //!< Timer function for PWM6.
#define TIMER_PWM7          20  //!< Timer function for PWM7.
#define QDEC_PHASE_A_X      21  //!< Qdecoder phase A function for axis X.
#define QDEC_PHASE_B_X      22  //!< Qdecoder phase B function for axis X.
#define QDEC_PHASE_A_Y      23  //!< Qdecoder phase A function for axis Y.
#define QDEC_PHASE_B_Y      24  //!< Qdecoder phase B function for axis Y.
#define QDEC_PHASE_A_Z      25  //!< Qdecoder phase A function for axis Z.
#define QDEC_PHASE_B_Z      26  //!< Qdecoder phase B function for axis Z.
#define UART1_TX            27  //!< Transmit function for UART1 line.
#define UART1_RX            28  //!< Receive function for UART1 line.
#define UART2_TX            29  //!< Transmit function for UART2 line.
#define UART2_RX            30  //!< Receive function for UART2 line.
#define UART2_CTS           31  //!< Function for UART2 CTS.
#define UART2_RTS           32  //!< Function for UART2 RTS.
#define IRDA_TX             33  //!< Transmit function for IRDA.
#define IRDA_RX             34  //!< Receive function for IRDA.
#define UART0_TX            35  //!< Transmit function for UART0 line.
#define UART0_RX            36  //!< Receive function for UART0 line.
#define UART0_CTS           37  //!< Function for UART0 CTS.
#define UART0_RTS           38  //!< Function for UART0 RTS.
#define SPI1_SS_N_0_MASTER  39  //!< Slave select 0 function for SPI1 in master mode.
#define SPI1_SS_N_1_MASTER  40  //!< Slave select 1 function for SPI1 in master mode.
#define SPI1_SS_N_2_MASTER  41  //!< Slave select 2 function for SPI1 in master mode.
#define SPI1_CLK_MASTER     42  //!< Clock line function for SPI1 in master mode.
#define SPI1_MO_MASTER      43  //!< Master output function for SPI1 in master mode.
#define SPI1_MI_MASTER      44  //!< Master input function for SPI1 in master mode.
#define SPI0_SS_N_0_SLAVE   45  //!< Slave select 0 function for SPI0 in slave mode.
#define SPI0_CLK_SLAVE      46  //!< Clock line function for SPI0 in slave mode.
#define SPI0_SO_SLAVE       47  //!< Slave output function for SPI0 in slave mode.
#define SPI0_SI_SLAVE       48  //!< Slave input function for SPI0 in slave mode.
#define SPI0_SS_N_0_MASTER  49  //!< Slave select 0 function for SPI0 in master mode.
#define SPI0_CLK_MASTER     50  //!< Clock line function for SPI0 in master mode.
#define SPI0_MO_MASTER      51  //!< Master output function for SPI0 in master mode.
#define SPI0_MI_MASTER      52  //!< Master input function for SPI0 in master mode.

#define SWD_CLK             56  //!< Clock line function for serial wire debug (SWD).
#define SWD_DIO             57  //!< Data input/output function for serial wire debug (SWD).
#define KEY_COL_0           58  //!< Function for column 0 in KeyScan.
#define KEY_COL_1           59  //!< Function for column 1 in KeyScan.
#define KEY_COL_2           60  //!< Function for column 2 in KeyScan.
#define KEY_COL_3           61  //!< Function for column 3 in KeyScan.
#define KEY_COL_4           62  //!< Function for column 4 in KeyScan.
#define KEY_COL_5           63  //!< Function for column 5 in KeyScan.
#define KEY_COL_6           64  //!< Function for column 6 in KeyScan.
#define KEY_COL_7           65  //!< Function for column 7 in KeyScan.
#define KEY_COL_8           66  //!< Function for column 8 in KeyScan.
#define KEY_COL_9           67  //!< Function for column 9 in KeyScan.
#define KEY_COL_10          68  //!< Function for column 10 in KeyScan.
#define KEY_COL_11          69  //!< Function for column 11 in KeyScan.
#define KEY_COL_12          70  //!< Function for column 12 in KeyScan.
#define KEY_COL_13          71  //!< Function for column 13 in KeyScan.
#define KEY_COL_14          72  //!< Function for column 14 in KeyScan.
#define KEY_COL_15          73  //!< Function for column 15 in KeyScan.
#define KEY_COL_16          74  //!< Function for column 16 in KeyScan.
#define KEY_COL_17          75  //!< Function for column 17 in KeyScan.
#define KEY_COL_18          76  //!< Function for column 18 in KeyScan.
#define KEY_COL_19          77  //!< Function for column 19 in KeyScan.
#define KEY_ROW_0           78  //!< Function for row 0 in KeyScan.
#define KEY_ROW_1           79  //!< Function for row 1 in KeyScan.
#define KEY_ROW_2           80  //!< Function for row 2 in KeyScan.
#define KEY_ROW_3           81  //!< Function for row 3 in KeyScan.
#define KEY_ROW_4           82  //!< Function for row 4 in KeyScan.
#define KEY_ROW_5           83  //!< Function for row 5 in KeyScan.
#define KEY_ROW_6           84  //!< Function for row 6 in KeyScan.
#define KEY_ROW_7           85  //!< Function for row 7 in KeyScan.
#define KEY_ROW_8           86  //!< Function for row 8 in KeyScan.
#define KEY_ROW_9           87  //!< Function for row 9 in KeyScan.
#define KEY_ROW_10          88  //!< Function for row 10 in KeyScan.
#define KEY_ROW_11          89  //!< Function for row 11 in KeyScan.
#define DWGPIO              90  //!< Function for GPIO.
#define LRC_SPORT1          91  //!< Function for LRC SPORT1.
#define BCLK_SPORT1         92  //!< Function for BCLK SPORT1.
#define ADCDAT_SPORT1       93  //!< Function for ADCDAT SPORT1.
#define DACDAT_SPORT1       94  //!< Function for DACDAT SPORT1.
#define SPDIF_TX            95  //!< Function for transmit line in SPDIF.
#define DMIC1_CLK           96  //!< Function for clock line for DMIC1.
#define DMIC1_DAT           97  //!< Function for data line for DMIC1.
#define LRC_I_CODEC_SLAVE   98  //!< Function for LRC_I_CODEC in slave mode.
#define BCLK_I_CODEC_SLAVE  99  //!< Function for BCLK_I_CODEC in slave mode.
#define SDI_CODEC_SLAVE     100 //!< Function for SDI_CODEC in slave mode.
#define SDO_CODEC_SLAVE     101 //!< Function for SDO_CODEC in slave mode.
#define LRC_I_PCM           102 //!< Function for LRC_I_PCM.
#define BCLK_I_PCM          103 //!< Function for BCLK_I_PCM.
#define SDI_PCM             104 //!< Function for serial data in for PCM.
#define SDO_PCM             105 //!< Function for serial data out for PCM.
#define BT_COEX_I_0         106 //!< Function for BT_COEX input 0.
#define BT_COEX_I_1         107 //!< Function for BT_COEX input 1.
#define BT_COEX_I_2         108 //!< Function for BT_COEX input 2.
#define BT_COEX_I_3         109 //!< Function for BT_COEX input 3.
#define BT_COEX_O_0         110 //!< Function for BT_COEX output 0.
#define BT_COEX_O_1         111 //!< Function for BT_COEX output 1.
#define BT_COEX_O_2         112 //!< Function for BT_COEX output 2.
#define BT_COEX_O_3         113 //!< Function for BT_COEX output 3.
#define PTA_I2C_CLK_SLAVE   114 //!< Function for I2C clock line for PTA in slave mode.
#define PTA_I2C_DAT_SLAVE   115 //!< Function for I2C data line for PTA in slave mode.
#define PTA_I2C_INT_OUT     116 //!< Function for interrupt output for PTA I2C.
#define DSP_GPIO_OUT        117 //!< Function for GPIO output for DSP.
#define DSP_JTCK            118 //!< Function for JTAG clock for DSP.
#define DSP_JTDI            119 //!< Function for JTAG data in for DSP.
#define DSP_JTDO            120 //!< Function for JTAG data out for DSP.
#define DSP_JTMS            121 //!< Function for JTAG mode select for DSP.
#define DSP_JTRST           122 //!< Function for JTAG reset for DSP.
#define LRC_SPORT0          123 //!< Function for LRC SPORT0.
#define BCLK_SPORT0         124 //!< Function for BCLK SPORT0.
#define ADCDAT_SPORT0       125 //!< Function for ADCDAT SPORT0.
#define DACDAT_SPORT0       126 //!< Function for DACDAT SPORT0.
#define MCLK_M              127 //!< Function for MCLK_M.
#define SPI0_SS_N_1_MASTER  128 //!< Slave select 1 function for SPI0 in master mode.
#define SPI0_SS_N_2_MASTER  129 //!< Slave select 2 function for SPI0 in master mode.
#define SPI2_SS_N_0_MASTER  130 //!< Slave select 0 function for SPI2 in master mode.
#define SPI2_CLK_MASTER     131 //!< Clock line function for SPI2 in master mode.
#define SPI2_MO_MASTER      132 //!< Master output function for SPI2 in master mode.
#define SPI2_MI_MASTER      133 //!< Master input function for SPI2 in master mode.
#define I2C2_CLK            134 //!< Clock line function for I2C2.
#define I2C2_DAT            135 //!< Data line function for I2C2.
#define ISO7816_RST         136 //!< Reset line function for ISO7816.
#define ISO7816_CLK         137 //!< Clock line function for ISO7816.
#define ISO7816_IO          138 //!< Input/output line function for ISO7816.
#define ISO7816_VCC_EN      139 //!< VCC enable function for ISO7816.
#define DMIC2_CLK           144 //!< Clock line function for DMIC2.
#define DMIC2_DAT           145 //!< Data line function for DMIC2.
#define BCLK_SPORT2         149 //!< Function for BCLK SPORT2.
#define ADCDAT_SPORT2       150 //!< Function for ADCDAT SPORT2.
#define UART1_CTS           167 //!< Function for UART1 CTS.
#define UART1_RTS           168 //!< Function for UART1 RTS.
#define SPIC0_SCK           169 //!< SPI clock line function for SPIC0.
#define SPIC0_CSN           170 //!< Chip select line function for SPIC0.
#define SPIC0_SIO_0         171 //!< Serial input/output line 0 function for SPIC0.
#define SPIC0_SIO_1         172 //!< Serial input/output line 1 function for SPIC0.
#define SPIC0_SIO_2         173 //!< Serial input/output line 2 function for SPIC0.
#define SPIC0_SIO_3         174 //!< Serial input/output line 3 function for SPIC0.
#define LRC_RX_CODEC_SLAVE  175 //!< LRC receive function for codec in slave mode.
#define LRC_RX_SPORT0       176 //!< LRC receive function for SPORT0.
#define LRC_RX_SPORT1       177 //!< LRC receive function for SPORT1.
#define LRC_RX_SPORT2       178 //!< LRC receive function for SPORT2.

#define PDM_AMP_DATA        193 //!< Amplifier data function for PDM.
#define PDM_AMP_CLK         194 //!< Amplifier clock function for PDM.

/** End of group 87x3e_Pin_Function_Number
  * @}
  */

/** @cond private
  * @defgroup 87x3e_HCI_UART_PIN HCI UART Pin
  * @{
  */
extern uint8_t hci_uart_rx_pin; //!< HCI UART RX pin is P3_0.
extern uint8_t hci_uart_tx_pin; //!< HCI UART TX pin is P3_1.

extern const uint8_t digi_debug_pin[32];
/** End of Group 87x3e_HCI_UART_PIN
  * @}
  * @endcond
  */

/** End of group 87x3e_PINMUX_Exported_Constants
  * @}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/

/** @defgroup 87x3e_PINMUX_Exported_Types PINMUX Exported Types
  * @{
  */

/** @cond private
  * @defgroup 87x3e_AON_FAST_REG_PAD_TYPE AON Fast Register Pad Type
  * @{
  */
typedef union _AON_FAST_REG_PAD_TYPE
{
    uint16_t d16;
    struct
    {
        uint16_t REG_PAD_DUMMY0: 1;
        uint16_t REG_PAD_DUMMY1: 1;
        uint16_t PAD_WKUP_INT_EN: 1;
        uint16_t PAD_WK_STATUS: 1;
        uint16_t PAD_PINMUX_M_EN: 1;
        uint16_t PAD_SMT: 1;
        uint16_t PAD_E3: 1;
        uint16_t PAD_E2: 1;
        uint16_t PAD_SHDN_PW_ON: 1;
        uint16_t PAD_OUT_EN: 1;
        uint16_t PAD_WKPOL: 1;
        uint16_t PAD_WKEN: 1;
        uint16_t PAD_OUT_VALUE: 1;
        uint16_t PAD_PUPDC_WE_STR: 1;
        uint16_t PAD_PUPD_DIR: 1;
        uint16_t PAD_PU_EN: 1;
    };
} AON_FAST_REG_PAD_TYPE;
/** End of Group 87x3e_AON_FAST_REG_PAD_TYPE
  * @}
  * @endcond
  */

/** @cond private
  * @defgroup 87x3e_AON_FAST_PAD_BIT_POS_TYPE AON Fast Pad Pos Type
  * @{
  */
typedef enum _AON_FAST_PAD_POS_TYPE
{
    REG_PAD_DUMMY0,
    REG_PAD_DUMMY1,
    PAD_WKUP_INT_EN,
    PAD_WK_STATUS,

    PAD_PINMUX_M_EN,
    PAD_SMT,
    PAD_E3,
    PAD_E2,

    PAD_SHDN_PW_ON,
    PAD_OUT_EN,
    PAD_WKPOL,
    PAD_WKEN,

    PAD_OUT_VALUE,
    PAD_PUPDC_WE_STR,
    PAD_PUPD_DIR,
    PAD_PU_EN,
} AON_FAST_PAD_BIT_POS_TYPE;
/** End of Group 87x3e_AON_FAST_PAD_BIT_POS_TYPE
  * @}
  * @endcond
  */

/** @defgroup 87x3e_PAD_Pull_Mode PAD Pull Mode
  * @{
  */

/**
 * @brief Pad pull mode definition.
 */
typedef enum _PAD_Pull_Mode
{
    PAD_PULL_DOWN,     //!< Enable the pull-down resistor function for the pad.
    PAD_PULL_UP,       //!< Enable the pull-up resistor function for the pad.
    PAD_PULL_NONE,     //!< Pad is in a floating state.
} PAD_Pull_Mode;

/** End of group 87x3e_PAD_Pull_Mode
  * @}
  */

/** @defgroup 87x3e_PAD_Pull_Value PAD Pull Value
  * @{
  */
/**
 * @brief Pad pull value definition.
 */
typedef enum _PAD_Pull_Value
{
    PAD_PULL_HIGH, //!< PAD pull value is high level.
    PAD_PULL_LOW, //!< PAD pull value is low level.

} PAD_Pull_VALUE;

/** End of group 87x3e_PAD_Pull_Value
  * @}
  */

/** @defgroup 87x3e_PAD_Pull_EN PAD Pull Enable
  * @{
  */

/**
 * @brief Pad pull function definition.
 */
typedef enum _PAD_Pull_EN
{
    PAD_PULL_DISABLE, //!< Enable pad pull.
    PAD_PULL_ENABLE //!< Disable pad pull.
} PAD_Pull_EN;


/** End of group 87x3e_PAD_Pull_EN
  * @}
  */

/** @defgroup 87x3e_PAD_Mode PAD Mode
  * @{
  */

/**
 * @brief Pad mode definition.
 */
typedef enum _PAD_Mode
{
    PAD_SW_MODE, //!< PAD pin is configured in software mode.
    PAD_PINMUX_MODE //!< PAD pin is configured in pinmux mode.
} PAD_Mode;

/** End of group 87x3e_PAD_Mode
  * @}
  */

/** @defgroup 87x3e_PAD_Power_Mode PAD Power Mode
  * @{
  */

/**
 * @brief Pad power mode definition.
 */
typedef enum _PAD_PWR_Mode
{
    PAD_SHUTDOWN, //!< Shutdown power of pad.
    PAD_IS_PWRON = 1 //!< Enable power of pad.
} PAD_PWR_Mode;

/** End of group 87x3e_PAD_Power_Mode
  * @}
  */

/** @defgroup 87x3e_PAD_Output_Config PAD Output Config
  * @{
  */

/**
 * @brief Pad output function definition.
 */
typedef enum _PAD_OUTPUT_ENABLE_Mode
{
    PAD_OUT_DISABLE,   //!< Disable pad output.
    PAD_OUT_ENABLE     //!< Enable pad output.
} PAD_OUTPUT_ENABLE_Mode;

/** End of group 87x3e_PAD_Output_Config
  * @}
  */

/** @defgroup 87x3e_PAD_Output_Value PAD Output Value
  * @{
  */

/**
 * @brief Pad output value definition.
 */
typedef enum _PAD_OUTPUT_VAL
{
    PAD_OUT_LOW,     //!< The pad outputs a low level.
    PAD_OUT_HIGH     //!< The pad outputs a high level.
} PAD_OUTPUT_VAL;

/** End of group 87x3e_PAD_Output_Value
  * @}
  */
/** @defgroup 87x3e_PAD_WakeUp_EN PAD Wake Up Enable
  * @{
  */

/**
 * @brief Pad wake up function definition.
 */
typedef enum _PAD_WAKEUP_EN
{
    PAD_WAKEUP_DISABLE,    //!< Disable PAD wakeup function.
    PAD_WAKEUP_ENABLE      //!< Enable PAD wakeup function.
} PAD_WAKEUP_EN;

/** End of group 87x3e_PAD_WakeUp_EN
  * @}
  */

/** @defgroup 87x3e_PAD_SLEEP_LED_Pin PAD SLEEP LED Pin
  * @{
  */

/**
 * @brief SLEEP LED pins definition.
 */
typedef enum _SLEEP_LED_PIN
{
    SLEEP_LED_ADC_0, //!< PAD SLEEP LED pin is ADC_0.
    SLEEP_LED_ADC_1, //!< PAD SLEEP LED pin is ADC_1.
    SLEEP_LED_ADC_6, //!< PAD SLEEP LED pin is ADC_6.
    SLEEP_LED_ADC_7, //!< PAD SLEEP LED pin is ADC_7.
    SLEEP_LED_P1_0, //!< PAD SLEEP LED pin is P1_0.
    SLEEP_LED_P1_1, //!< PAD SLEEP LED pin is P1_1.
    SLEEP_LED_P1_4, //!< PAD SLEEP LED pin is P1_4.
    SLEEP_LED_P2_0, //!< PAD SLEEP LED pin is P2_0.
    SLEEP_LED_P2_1, //!< PAD SLEEP LED pin is P2_1.
    SLEEP_LED_P2_2, //!< PAD SLEEP LED pin is P2_2.
} SLEEP_LED_PIN;

/** End of group 87x3e_PAD_SLEEP_LED_Pin
  * @}
  */

/** @defgroup 87x3e_PAD_Function_Config PAD Function Config
  * @{
  */

/**
 * @brief Pad function config value definition.
 */
typedef enum _PAD_FUNCTION_CONFIG_VALUE
{
    AON_GPIO, //!< Default GPIO function.
    LED0, //!< SLEEP LED channel 0.
    LED1, //!< SLEEP LED channel 1.
    LED2, //!< SLEEP LED channel 2.
    CLK_REQ, //!< Clock request, internal debug function.
} PAD_FUNCTION_CONFIG_VAL;

/** End of group 87x3e_PAD_Function_Config
  * @}
  */

/** @defgroup 87x3e_PAD_WakeUp_Polarity_Value PAD Wake Up Polarity Value
  * @{
  */

/**
 * @brief Pad wake up polarity definition.
 */
typedef enum _PAD_WAKEUP_POL_VAL
{
    PAD_WAKEUP_POL_HIGH,    //!< PAD wakeup polarity is high.
    PAD_WAKEUP_POL_LOW,     //!< PAD wakeup polarity is low.
    PAD_WAKEUP_NONE         //!< PAD wakeup polarity is none.
} PAD_WAKEUP_POL_VAL;

/** End of group 87x3e_PAD_WakeUp_Polarity_Value
  * @}
  */

/** @defgroup 87x3e_PAD_WakeUp_Debounce_En PAD Wake Up Debounce Enable
  * @{
  */
/**
 * @brief Pad pull wake-up debounce function definition.
 */
typedef enum _PAD_WAKEUP_DEBOUNCE_EN
{
    PAD_WK_DEBOUNCE_DISABLE, //!< Disable wake-up debounce function.
    PAD_WK_DEBOUNCE_ENABLE //!< Enable wake-up debounce function.
} PAD_WAKEUP_DEBOUNCE_EN;

/** End of group 87x3e_PAD_WakeUp_Debounce_En
  * @}
  */

/** @defgroup 87x3e_PAD_Pull_Value PAD Pull Value
  * @{
  */

/**
 * @brief Pad resistance pull value definition.
 */
typedef enum _PAD_PULL_CONFIG_VAL
{
    PAD_WEAKLY_PULL, //!< Resistance weak pull.
    PAD_STRONG_PULL //!< Resistance strong pull.
} PAD_PULL_VAL;
/** End of group 87x3e_PAD_Pull_Value
  * @}
  */

/** @defgroup 87x3e_PAD_DRIVING_CURRENT PAD Driving Current Value
  * @{
  */
/**
 * @brief Pad driving current level definition.
 */
typedef enum _DRIVER_LEVEL
{
    LEVEL0, //!< The PAD driving current is set to level 0.
    LEVEL1, //!< The PAD driving current is set to level 1.
    LEVEL2, //!< The PAD driving current is set to level 2.
    LEVEL3, //!< The PAD driving current is set to level 3.
} T_DRIVER_LEVEL_MODE;
/** End of group 87x3e_PAD_DRIVING_CURRENT
  * @}
  */

/** @defgroup 87x3e_PAD_POWER_GROUP Pad Power Supply Voltage
  * @{
  */
/**
 * @brief Pad pin power group definition.
 */
typedef enum _PIN_POWER_GROUP
{
    INVALID_PIN_GROUP  = 0,      //!< Invalid pad power group.
    VDDIO1             = 1,      //!< Pad power group of VDDIO1 pin.
    VDDIO2             = 2,      //!< Pad power group of VDDIO2 pin.
    VDDIO3             = 3,      //!< Pad power group of VDDIO3 pin.
    VDDIO4             = 4,      //!< Pad power group of VDDIO4 pin.
} T_PIN_POWER_GROUP;

/** End of group 87x3e_PAD_POWER_GROUP
  * @}
  */

/** @defgroup 87x3e_PAD_LDO_Type PAD LDO Type
  * @{
  */
/**
 * @brief Pad LDO type definition.
 */
typedef enum _PAD_LDO_TYPE
{
    PAD_LDOAUX1, //!< PAD LDO type is AUX1.
    PAD_LDOAUX2 //!< PAD LDO type is AUX2.
} PAD_LDO_TYPE;

/** End of group 87x3e_PAD_LDO_Type
  * @}
  */

/** @defgroup 87x3e_PAD_AON_STATUS PAD AON Status
  * @{
  */
/**
 * @brief Pad AON status definition.
 */
typedef enum _PAD_AON_Status
{
    PAD_AON_OUTPUT_LOW,        //!< Pad AON output low level.
    PAD_AON_OUTPUT_HIGH,       //!< Pad AON output high level.
    PAD_AON_OUTPUT_DISABLE,    //!< Pad AON output disable.
    PAD_AON_PINMUX_ON,         //!< Pad AON pinmux on.
    PAD_AON_PIN_ERR            //!< Pad AON pin error.
} PAD_AON_Status;
/** End of group 87x3e_PAD_AON_STATUS
  * @}
  */

/** @defgroup 87x3e_WAKEUP_POLARITY PAD Wake Up Polartity
  * @{
  */
/**
 * @brief Pad wake up polarity definition.
 */
typedef enum _WAKEUP_POL
{
    POL_HIGH,    //!< PAD high-level trigger wakeup.
    POL_LOW,     //!< PAD low-level trigger wakeup.
} WAKEUP_POL;
/** End of group 87x3e_WAKEUP_POLARITY
  * @}
  */

/** @defgroup 87x3e_WAKEUP_ENABLE PAD Wake Up Enable Mode
  * @{
  */
/**
 * @brief Pad wake up mode definition.
 */
typedef enum _WAKEUP_EN_MODE
{
    ADP_MODE,    //!< Wake up by adapter.
    BAT_MODE,    //!< Wake up by battery.
    MFB_MODE,    //!< Wake up by MFB.
    USB_MODE,    //!< Wake up by USB.
} WAKEUP_EN_MODE;
/** End of group 87x3e_WAKEUP_ENABLE
  * @}
  */

/** @defgroup 87x3e_ANA_MODE PAD Analog/Digital Mode
  * @{
  */
/**
 * @brief Pad analog/digital mode for CODEC hybrid IO.
 */
typedef enum _ANA_MODE
{
    PAD_ANALOG_MODE,      //!< Config hybrid pad analog function.
    PAD_DIGITAL_MODE,     //!< Config hybrid pad digital function.
} ANA_MODE;

/** End of group 87x3e_ANA_MODE
  * @}
  */

/** @defgroup 87x3e_WAKE_UP_MODE PAD Wake Up Mode
 * @{
 */
/**
 * @brief Pad wake up mode definition.
 */
typedef enum
{
    WAKE_UP_POWER_OFF,     //!< Config power off wake up.
    WAKE_UP_GENERAL,       //!< Config DLPS or power down wake up.
} T_WAKE_UP_MODE;

/** End of group 87x3e_WAKE_UP_MODE
  * @}
  */


/** End of group 87x3e_PINMUX_Exported_Types
  * @}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/

/** @defgroup 87x3e_PINMUX_Exported_Functions PINMUX Exported Functions
  * @{
  */

/** @cond private
  * @defgroup PAD_Table_Config PAD Table Config
  * @{
  */
void Pad_TableConfig(AON_FAST_PAD_BIT_POS_TYPE pad_bit_set, uint8_t Pin_Num,
                     uint8_t value); /* Note: PINMUX internal function. */

#define Pad_WKTableConfig       Pad_TableConfig //!< The macro is a wrapper for Pad_TableConfig.
/**
  * @}
  * @endcond
  */

/**
 *
 * \brief   Reset all pins to idle mode.
 *
 * \note   Two SWD pins will also be reset. Please use this function carefully.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void board_xxx_init(void)//XXX represents the name of the peripheral to be configured.
 * {
 *     Pinmux_Reset();
 * }
 * \endcode
 */
extern void Pinmux_Reset(void);

/**
 *
 * \brief     Configure the specified pin to idle mode.
 *
 * \param[in] Pin_Num: Pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void board_xxx_init(void)
 * {
 *     Pinmux_Deinit(P2_2);
 * }
 * \endcode
 */
extern void Pinmux_Deinit(uint8_t Pin_Num);

/**
 *
 * \brief     Config the selected pin to its corresponding IO function.
 *
 * \param[in] Pin_Num: Pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] Pin_Func: IO function of pin, can be a value of \ref x3e_Pin_Function_Number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_uart_init(void)
 * {
 *     Pad_Config(P2_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
 *     Pad_Config(P2_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);

 *     Pinmux_Config(P2_0, UART0_TX);
 *     Pinmux_Config(P2_1, UART0_RX);
 * }
 * \endcode
 */
extern void Pinmux_Config(uint8_t Pin_Num, uint8_t Pin_Func);

/**
 *
 * \brief     Configure the relevant operation mode,
 *            peripheral circuit and output level value in software mode of the specified pin.
 *
 * \param[in] Pin_Num: Pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] AON_PAD_Mode: Use software mode or PINMUX mode. Please refer to \ref x3e_PAD_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_SW_MODE: Use software mode.
 *            - PAD_PINMUX_MODE: Use PINMUX mode.
 * \param[in] AON_PAD_PwrOn: Config power of pad. Please refer to \ref x3e_PAD_Power_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_SHUTDOWN: Shutdown power of pad.
 *            - PAD_IS_PWRON: Enable power of pad.
 * \param[in] AON_PAD_Pull: Config pad pull mode. Please refer to \ref x3e_PAD_Pull_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_PULL_NONE: No pull.
 *            - PAD_PULL_UP: Pull this pin up.
 *            - PAD_PULL_DOWN: Pull this pin down.
 * \param[in] AON_PAD_E: Config pad output function, which only valid when PAD_SW_MODE. Please refer to \ref x3e_PAD_Output_Config.
 *            This parameter can be one of the following values:
 *            - PAD_OUT_DISABLE: Disable pin output.
 *            - PAD_OUT_ENABLE: Enable pad output.
 * \param[in] AON_PAD_O: Config pin output level, which only valid when PAD_SW_MODE and output mode. Please refer to \ref x3e_PAD_Output_Value.
 *            This parameter can be one of the following values:
 *            - PAD_OUT_LOW: Pad output low.
 *            - PAD_OUT_HIGH: Pad output high.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_adc_init(void)
 * {
 *     Pad_Config(P2_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
 *     Pad_Config(P2_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
 * }
 * \endcode
 */
extern void Pad_Config(uint8_t                Pin_Num,
                       PAD_Mode               AON_PAD_Mode,
                       PAD_PWR_Mode           AON_PAD_PwrOn,
                       PAD_Pull_Mode          AON_PAD_Pull,
                       PAD_OUTPUT_ENABLE_Mode AON_PAD_E,
                       PAD_OUTPUT_VAL         AON_PAD_O);

/**
 *
 * \brief     Configure the relevant operation mode, peripheral circuit, pull resistor value and
 *            output level value in software mode of the specified pin.
 *
 * \param[in] Pin_Num: Pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] AON_PAD_Mode: Use software mode or PINMUX mode. Please refer to \ref x3e_PAD_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_SW_MODE: Use software mode.
 *            - PAD_PINMUX_MODE: Use PINMUX mode.
 * \param[in] AON_PAD_PwrOn: Config power of pad. Please refer to \ref x3e_PAD_Power_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_SHUTDOWN: Shutdown power of pad.
 *            - PAD_IS_PWRON: Enable power of pad.
 * \param[in] AON_PAD_Pull: Config pad pull mode. Please refer to \ref x3e_PAD_Pull_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_PULL_NONE: No pull.
 *            - PAD_PULL_UP: Pull this pin up.
 *            - PAD_PULL_DOWN: Pull this pin down.
 * \param[in] AON_PAD_E: Config pad output function, which only valid when PAD_SW_MODE. Please refer to \ref x3e_PAD_Output_Config.
 *            This parameter can be one of the following values:
 *            - PAD_OUT_DISABLE: Disable pin output.
 *            - PAD_OUT_ENABLE: Enable pad output.
 * \param[in] AON_PAD_O: Config pin output level, which only valid when PAD_SW_MODE and output mode. Please refer to \ref x3e_PAD_Output_Value.
 *            This parameter can be one of the following values:
 *            - PAD_OUT_LOW: Pad output low.
 *            - PAD_OUT_HIGH: Pad output high.
 * \param[in] AON_PAD_P: Config resistor value. Please refer to \ref x3e_PAD_Pull_Value.
 *            This parameter can be one of the following values:
 *            - PAD_STRONG_PULL: Pad pull 150k resistance.
 *            - PAD_WEAKLY_PULL: Pad pull 15k resistance.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_adc_init(void)
 * {
 *     Pad_ConfigExt(P2_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW, PAD_WEAKLY_PULL);
 *     Pad_ConfigExt(P2_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW, PAD_STRONG_PULL);
 * }
 * \endcode
 */
extern void Pad_ConfigExt(uint8_t                Pin_Num,
                          PAD_Mode               AON_PAD_Mode,
                          PAD_PWR_Mode           AON_PAD_PwrOn,
                          PAD_Pull_Mode          AON_PAD_Pull,
                          PAD_OUTPUT_ENABLE_Mode AON_PAD_E,
                          PAD_OUTPUT_VAL         AON_PAD_O,
                          PAD_PULL_VAL           AON_PAD_P);

/**
 *
 * \brief   Set all pins to the default state.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void board_xxx_init(void)
 * {
 *     Pad_AllConfigDefault();
 * }
 * \endcode
 */
extern void (*Pad_AllConfigDefault)(void);

/**
 *
 * \brief   Enable the function of the wake-up system of the specified pin.
 *
 * \param[in] Pin_Num: Pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] Polarity: Polarity to wake up. Please refer to \ref x3e_PAD_WakeUp_Polarity_Value.
 *            This parameter can be the following:
 *            - PAD_WAKEUP_POL_HIGH: Use high level wakeup.
 *            - PAD_WAKEUP_POL_LOW: Use low level wakeup.
 *
 * <b>Example usage</b>
 * \code{.c}
 * //IO enter DLPS call back function.
 * void io_uart_dlps_enter(void)
 * {
 *     // Switch pad to software mode
 *     Pad_ControlSelectValue(P2_0, PAD_SW_MODE);//TX pin
 *     Pad_ControlSelectValue(P2_1, PAD_SW_MODE);//RX pin
 *
 *     System_WakeUpPinEnable(P2_1, PAD_WAKEUP_POL_LOW);
 * }
 * \endcode
 */
extern void (*System_WakeUpPinEnable)(uint8_t Pin_Num, uint8_t Polarity);

/**
 *
 * \brief   Disable the function of the wake-up system of the specified pin.
 *
 * \param[in] Pin_Num: Pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * #define UART_RX_PIN   P4_1
 *
 * //System interrupt handler function, for wakeup pin.
 * void System_Handler(void)
 * {
 *     if (System_WakeUpInterruptValue(UART_RX_PIN) == SET)
 *     {
 *         Pad_ClearWakeupINTPendingBit(UART_RX_PIN);
 *         System_WakeUpPinDisable(UART_RX_PIN);
 *         //Add user code here.
 *     }
 * }
 * \endcode
 */
extern void (*System_WakeUpPinDisable)(uint8_t Pin_Num);

/**
 *
 * \brief   Configure the adpater wake-up system functions in power off(shipping) mode.
 *
 * \param[in] NewState: Enable or disable adpater wake up.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable adpater wake up system at specified polarity.
 *            - DISABLE: Disable adpater wake up system.
 * \param[in] pol: Polarity to wake up. Please refer to \ref x3e_WAKEUP_POLARITY.
 *            This parameter can be the following:
 *            - POL_HIGH: Use high level wakeup.
 *            - POL_LOW: Use low level wakeup.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void adapter_wake_up_enable(void)
 * {
 *     //adapter mode is wake_up_power_off
 *     System_SetAdpWakeUpFunction(ENABLE, POL_HIGH);
 * }
 * \endcode
 */
extern void System_SetAdpWakeUpFunction(FunctionalState NewState, WAKEUP_POL pol);

/**
 *
 * \brief   Configure the MFB wake-up system functions in power off(shipping) mode.
 *
 * \param[in] NewState: Enable or disable MFB wake up.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable MFB wake up system.
 *            - DISABLE: Disable MFB wake up system.
 *
 * <b>Example usage</b>
 * \code{.c}
 * //io_test_set_mfb_mode is POWER_OFF_WAKEUP_TEST
 * void mfb_wake_up_enable(void)
 * {
 *     System_SetMFBWakeUpFunction(ENABLE);
 * }
 * \endcode
 */
extern void System_SetMFBWakeUpFunction(FunctionalState NewState);

/**
 *
 * \brief   Disable the function of the wake-up system interrupt of the specified pin.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * //System interrupt handler function.
 * void System_Handler(void)
 * {
 *     if (System_WakeUpInterruptValue(P2_5) == SET)
 *     {
 *         Pad_ClearWakeupINTPendingBit(P2_5);
 *         System_WakeUpInterruptDisable(P2_5);
 *         //Add user code here.
 *     }
 * }
 * \endcode
 */
extern void (*System_WakeUpInterruptDisable)(uint8_t Pin_Num);

/**
 *
 * \brief   Enable the function of the wake-up system interrupt of the specified pin.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * //IO enter DLPS call back function.
 * void io_uart_dlps_enter(void)
 * {
 *     // Switch pad to software mode
 *     Pad_ControlSelectValue(P2_0, PAD_SW_MODE);//TX pin
 *     Pad_ControlSelectValue(P2_1, PAD_SW_MODE);//RX pin
 *
 *     System_WakeUpInterruptEnable(P2_1);
 * }
 * \endcode
 */
extern void (*System_WakeUpInterruptEnable)(uint8_t Pin_Num);

/**
 *
 * \brief   Config pad output function.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value: This parameter sets whether the pin outputs the level in software mode. Please refer to \ref x3e_PAD_Output_Config.
 *            This parameter can be one of the following values:
 *            - PAD_OUT_DISABLE: Disable pin output.
 *            - PAD_OUT_ENABLE: Enable pin output.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void board_xxx_init(void)
 * {
 *     Pad_OutputEnableValue(P2_0, PAD_OUT_ENABLE);
 * }
 * \endcode
 */
#define Pad_OutputEnableValue(Pin_Num, value) Pad_TableConfig(PAD_OUT_EN, Pin_Num, value)

/**
 *
 * \brief   Config pad pull enable or not.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value: Enable or disable pad pull-up / pull-down resistance function.
 *            This parameter can be one of the following values:
 *            - DISABLE: Disable pad pull-up / pull-down function.
 *            - ENABLE: Enable  pad pull-up / pull-down function.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void board_xxx_init(void)
 * {
 *     Pad_PullEnableValue(P2_0, ENABLE);
 * }
 * \endcode
 */
#define Pad_PullEnableValue(Pin_Num, value) Pad_TableConfig(PAD_PU_EN, Pin_Num, value)

/**
 *
 * \brief     Enable or disable the pull direction for the pad.
 *
 * \param[in] Pin_Num: Pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value: Enable or disable the pull direction for the pad.
 *            This parameter can be one of the following values:
 *            - 0: The pad pull direction is disabled.
 *            - 1: The pad pull direction is enabled.
 * \param[in] Pull_Direction_value: Config pad pull mode. Please refer to \ref x3e_PAD_Pull_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_PULL_NONE: No pull.
 *            - PAD_PULL_UP: Pull this pin up.
 *            - PAD_PULL_DOWN: Pull this pin down.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void driver_gpio_init(void)
 * {
 *     Pad_PullEnableValue_Dir(pin_index, 1, (PAD_Pull_Mode)pull_value);
 * }
 * \endcode
 */
extern void Pad_PullEnableValue_Dir(uint8_t Pin_Num, uint8_t value,
                                    PAD_Pull_Mode Pull_Direction_value);

/**
 *
 * \brief   Config pad pull up or down.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value : This parameter sets whether the pin pull-up or pull-down.
 *            This parameter can be one of the following values:
 *            - 0: Config pad pull-down function.
 *            - 1: Config pad pull-up function.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void board_xxx_init(void)
 * {
 *     Pad_PullUpOrDownValue(P2_0, 1);
 * }
 * \endcode
 */
#define Pad_PullUpOrDownValue(Pin_Num, value) Pad_TableConfig(PAD_PUPD_DIR, Pin_Num, value)

/**
 *
 * \brief   Config the pad control selected value.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value: Use software mode or PINMUX mode. Please refer to \ref x3e_PAD_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_SW_MODE: Use software mode, aon control.
 *            - PAD_PINMUX_MODE: Use PINMUX mode, core control.
 *
 * <b>Example usage</b>
 * \code{.c}
 * //IO enter DLPS call back function.
 * void io_uart_dlps_enter(void)
 * {
 *     // Switch pad to software mode
 *     Pad_ControlSelectValue(P2_0, PAD_SW_MODE);//TX pin
 *     Pad_ControlSelectValue(P2_1, PAD_SW_MODE);//RX pin
 *
 *     System_WakeUpPinEnable(P2_1, PAD_WAKEUP_POL_LOW);
 * }
 * \endcode
 */
#define Pad_ControlSelectValue(Pin_Num, value) Pad_TableConfig(PAD_PINMUX_M_EN, Pin_Num, value)

/**
 *
 * \brief     Configure the pad output level when pad set to SW mode.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value: Config pin output level. Please refer to \ref x3e_PAD_Output_Value.
 *            This parameter can be one of the following values:
 *            - PAD_OUT_LOW: Pad output low.
 *            - PAD_OUT_HIGH: Pad output high.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void board_xxx_init(void)
 * {
 *     Pad_OutputControlValue(P2_0, PAD_OUT_HIGH);
 * }
 * \endcode
 */
#define Pad_OutputControlValue(Pin_Num, value) Pad_TableConfig(PAD_OUT_VALUE, Pin_Num, value)

/**
 *
 * \brief     Enable the function of the wake-up system of the specified pin.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value:  Enable or disable wake-up system function.
 *            - ENABLE: Enable pad to wake up system from DLPS.
 *            - DISABLE: Disable pad to wake up system from DLPS.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void board_xxx_init(void)
 * {
 *     Pad_WakeupEnableValue(P2_0, ENABLE);
 * }
 * \endcode
 */
#define Pad_WakeupEnableValue(Pin_Num, value) Pad_WKTableConfig(PAD_WKEN, Pin_Num, value)

/**
 *
 * \brief     Config the pad wake up polarity.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] Polarity: Polarity of wake-up system. Please refer to \ref x3e_PAD_WakeUp_Polarity_Value.
 *            This parameter can be the following:
 *            - PAD_WAKEUP_POL_LOW: Use low level wakeup.
 *            - PAD_WAKEUP_POL_HIGH: Use high level wakeup.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void board_xxx_init(void)
 * {
 *     Pad_WakeupPolarityValue(P2_0, PAD_WAKEUP_POL_LOW);
 * }
 * \endcode
 */
#define Pad_WakeupPolarityValue(Pin_Num, value) Pad_WKTableConfig(PAD_WKPOL, Pin_Num, value)

/**
 *
 * \brief   Check wake up pin interrupt status.
 *
 * \param[in] Pin_Num: Pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 *
 * \return   Pin interrupt status.
 * \retval 1: Pin wake up system.
 * \retval 0: The pin does not wake up the system.
 *
 * <b>Example usage</b>
 * \code{.c}
 * #define UART_RX_PIN                P4_1
 *
 * //System interrupt handler function.
 * void System_Handler(void)
 * {
 *     if (System_WakeUpInterruptValue(UART_RX_PIN) == SET)
 *     {
 *         Pad_ClearWakeupINTPendingBit(UART_RX_PIN);
 *         System_WakeUpPinDisable(UART_RX_PIN);
 *         //Add user code here.
 *     }
 * }
 * \endcode
 */
#define  System_WakeUpInterruptValue(Pin_Num) Pad_WakeupInterruptValue(Pin_Num)

/**
 *
 * \brief   Config pad wake up interrupt.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value: Enable or disable pad wake up interrupt.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable pad wake up to trigger system interrupt.
 *            - DISABLE: Disable pad wake up to trigger system interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *     Pad_WakeupInterruptEnable(P2_0, ENABLE);
 * }
 * \endcode
 */
extern void Pad_WakeupInterruptEnable(uint8_t Pin_Num, uint8_t value);

/**
 *
 * \brief   Check pad wake up pin interrupt status.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 *
 * \return   Pin interrupt status.
 * \retval 1: Pin wake up system.
 * \retval 0: The pin does not wake up the system.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void System_Handler(void)
 * {
 *     if (Pad_WakeupInterruptValue(P4_1) == SET)
 *     {
 *         Pad_ClearWakeupINTPendingBit(P4_1);
 *     }
 *     NVIC_DisableIRQ(System_IRQn);
 *     NVIC_ClearPendingIRQ(System_IRQn);
 * }
 * \endcode
 */
extern FlagStatus Pad_WakeupInterruptValue(uint8_t Pin_Num);

/**
 *
 * \brief   Clear pad wake up pin interrupt pending bit.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void System_Handler(void)
 * {
 *     if (Pad_WakeupInterruptValue(P4_1) == SET)
 *     {
 *         Pad_ClearWakeupINTPendingBit(P4_1);
 *     }
 *     NVIC_DisableIRQ(System_IRQn);
 *     NVIC_ClearPendingIRQ(System_IRQn);
 * }
 * \endcode
 */
extern void Pad_ClearWakeupINTPendingBit(uint8_t Pin_Num);

/**
 *
 * \brief   Clear all wake up pin interrupt pending bit.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void dlps_io_enter_cb(void)
 * {
 *     io_dlps_callback(&io_dlps_enter_q);
 *
 *     //clear aon fast 0x12E ~ 0x131, 0x133 (PAD wake up INT status), write one clear
 *     Pad_ClearAllWakeupINT();
 *
 *     if (power_mode_get() == POWER_DLPS_MODE)
 *     {
 *         dlps_io_store();
 *     }
 * }
 * \endcode
 */
extern void Pad_ClearAllWakeupINT(void);

/**
 *
 * \brief   Config pin power mode.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value: This parameter sets the power supply mode of the pin,
 *                   and the value is enumeration PAD_PWR_Mode One of the values. Please refer to \ref x3e_PAD_Power_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_SHUTDOWN: Power off.
 *            - PAD_IS_PWRON: Power on.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *     Pad_PowerOrShutDownValue(P2_0, PAD_IS_PWRON);
 * }
 * \endcode
 */
#define Pad_PowerOrShutDownValue(Pin_Num, value) Pad_TableConfig(PAD_SHDN_PW_ON, Pin_Num, value)

/**
 *
 * \brief     Configure the strength of pull-up/pull-down resistance.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value: This parameter sets the strength of pull-up/pull-down resistance. Please refer to \ref x3e_PAD_Pull_Value.
 *            This parameter can be one of the following values:
 *            - PAD_STRONG_PULL: Pad pull 150k resistance.
 *            - PAD_WEAKLY_PULL: Pad pull 15k resistance.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void board_xxx_init(void)
 * {
 *     Pad_PullConfigValue(P2_0, PAD_150K_PULL);
 * }
 * \endcode
 */
#define Pad_PullConfigValue(Pin_Num, value) Pad_TableConfig(PAD_PUPDC_WE_STR, Pin_Num, value)

/**
 *
 * \brief   Config driving current value.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] e2_value: Set driving current value.
 *            This parameter can be one of the following values:
 *            - 0: Set driving current value to low level.
 *            - 1: Set driving current value to high level.
 * \param[in] e3_value: Set driving current value.
 *            This parameter can be one of the following values:
 *            - 0: Set driving current value to low level.
 *            - 1: Set driving current value to high level.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *     Pad_DrivingCurrentControl(P2_0, 1, 1);
 * }
 * \endcode
 */
extern void Pad_DrivingCurrentControl(uint8_t Pin_Num, uint8_t e2_value, uint8_t e3_value);

/**
 *
 * \brief   Config Pad Function.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] value: Config value \ref x3e_PAD_Function_Config.
 *            This parameter can be one of the following values:
 *            - AON_GPIO: Default GPIO function.
 *            - LED0: SLEEP LED channel 0.
 *            - LED1: SLEEP LED channel 1.
 *            - LED2: SLEEP LED channel 2.
 *            - CLK_REQ: Clock request, internal debug function.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *     Pad_FunctionConfig(P2_0, AON_GPIO);
 * }
 * \endcode
 */
extern void Pad_FunctionConfig(uint8_t Pin_Num, PAD_FUNCTION_CONFIG_VAL value);

/**
 *
 * \brief   Get pad current output/input setting.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 *
 * \return The pad current output/input setting, which can refer to PAD_AON_Status. Please refer to \ref x3e_PAD_AON_STATUS.
 * \retval PAD_AON_OUTPUT_LOW: Pad AON output low level.
 * \retval PAD_AON_OUTPUT_HIGH: Pad AON output high level.
 * \retval PAD_AON_OUTPUT_DISABLE: Pad AON output disable.
 * \retval PAD_AON_PINMUX_ON: Pad AON PINMUX on.
 * \retval PAD_AON_PIN_ERR: Pad AON pin error.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *     if (Pad_GetOutputCtrl(P2_1) == PAD_AON_OUTPUT_LOW)
 *     {
 *         //Add user code here.
 *     }
 * }
 * \endcode
 */
uint8_t Pad_GetOutputCtrl(uint8_t Pin_Num);

/**
 *
 * \brief   Config the system wakeup mode.
 *
 * \param[in] mode: The mode of set, this parameter can refer to \ref x3e_WAKEUP_ENABLE.
 *            This parameter can be one of the following values:
 *            - ADP_MODE: Wake up by adapter.
 *            - BAT_MODE: Wake up by battery.
 *            - MFB_MODE: Wake up by MFB.
 *            - USB_MODE: Wake up by USB.
 * \param[in] pol: The polarity to wake up. Please refer to \ref x3e_WAKEUP_POLARITY.
 *            This parameter can be the following:
 *            - POL_HIGH: Use high level wakeup.
 *            - POL_LOW: Use low level wakeup.
 * \param[in] NewState: Enable or disable wake up.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable the system wake up at specified polarity.
 *            - DISABLE: Disable the system wake up at specified polarity.
 *
 * \return     Config the system wakeup mode fail or success.
 * \retval 0   Config success.
 * \retval 1   Config fail due to wrong mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void adapter_wake_up_enable(void)
 * {
 *     //adapter mode is WAKE_UP_GENERAL
 *     Pad_WakeUpCmd(ADP_MODE, POL_HIGH, ENABLE);
 * }
 * \endcode
 */
extern uint8_t (*Pad_WakeUpCmd)(WAKEUP_EN_MODE mode, WAKEUP_POL pol, FunctionalState NewState);

/**
 *
 * \brief   Get the pin name in string.
 *
 * \param[in] pin_num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 *
 * \return   The pin name or null. When null is returned, it indicates that the pin index is invalid.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *     Pad_GetPinName(P2_1);
 * }
 * \endcode
 */
const char *Pad_GetPinName(uint8_t pin_num);

/**
 *
 * \brief   Configure the driving current of the pin.
 *
 * \param[in] Pin_Num: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] driver_level: Refer to the \ref x3e_PAD_DRIVING_CURRENT.
 *            This parameter can be: LEVEL0, LEVEL1, LEVEL2, LEVEL3.
 *            When VDDIO is set to 1.8V, the pad driving current is as followed:
 *            - LEVEL0: 1.25 mA.
 *            - LEVEL1: 2.50 mA.
 *            - LEVEL2: 3.75 mA.
 *            - LEVEL3: 5.00 mA.
 *            When VDDIO is set to 3.3V, the pad driving current is as followed:
 *            - LEVEL0: 4 mA.
 *            - LEVEL1: 8 mA.
 *            - LEVEL2: 12 mA.
 *            - LEVEL3: 16 mA.
 *
 * \return   The driving current of the pin set succeeded or failed.
 * \retval true    Driving current is set successfully.
 * \retval false   Driving current set fail. The failure reasons could be one of the following one:
 *                 invalid pin number or driving current setting is not supported for the setting pin.
 * \note  All pin support MODE_4MA MODE_8MA.
 *        Only follow pin support MODE_12MA, MODE_16MA driver current:
    {
    P1_0,    P1_1,    P1_2,    P1_3,    P1_4,    P1_5,    P1_6,    P1_7,
    P3_0,    P3_1,    P3_2,    P3_3,    P3_4,    P3_5,
    P5_0,    P5_1,    P5_2,    P5_3,    P5_4,    P5_5,    P5_6,    P5_7,
    P6_0,    P6_1,    P6_2,    P6_3,    P6_4,    P6_5,    P6_6,
    P7_0,    P7_1,    P7_2,    P7_3,    P7_4,    P7_5,    P7_6,
    AUX_R,    AUX_L,     MIC1_P,     MIC1_N,    MIC2_P, MIC2_N,   MICBIAS,    LOUT_P,
    LOUT_N,  ROUT_P, ROUT_N,    MIC3_P,    MIC3_N
    }
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *     Pad_SetPinDrivingCurrent(P2_1, LEVEL0);
 * }
 * \endcode
 */
bool Pad_SetPinDrivingCurrent(uint8_t pin, T_DRIVER_LEVEL_MODE driver_level);

/**
 *
 * \brief   Get the pin power group.
 *
 * \param[in] pin: Pin of set only VDDIO pin. Please refer to \ref x3e_Pin_Number.
 *
 * \return   The power group of the pin. Please refer to \ref x3e_PAD_POWER_GROUP.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *     Pad_GetPowerGroup(P2_1);
 * }
 * \endcode
 */
T_PIN_POWER_GROUP  Pad_GetPowerGroup(uint8_t pin);

/**
 *
 * \brief   Config hybrid pad analog/digital functions.
 *
 * \param[in] pin: The pin number to be configured.
 *            This parameter can be one of the following values:
 *            - AUX_R ~ MIC3_N, please refer to \ref x3e_Pin_Number.
 * \param[in] mode: Please refer to \ref x3e_ANA_MODE.
 *            - This parameter can be: PAD_ANALOG_MODE/PAD_DIGITAL_MODE.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *     Pad_AnalogMode(P2_1, PAD_ANALOG_MODE);
 * }
 * \endcode
 */
void Pad_AnalogMode(uint8_t pin, ANA_MODE mode);

/**
 *
 * \brief   Get the pad config.
 *
 * \xrefitem Added_API_2_12_0_0 "Added Since 2.12.0.0" "Added API"
 *
 * \param[in] pin_num: Pin number to be configured.
 *            This parameter can be one of the following values:
 *            - P0_0 ~ P10_0, please refer to \ref x3e_Pin_Number.
 * \param[in] mode: Use software mode or PINMUX mode. Please refer to \ref x3e_PAD_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_SW_MODE: Use software mode.
 *            - PAD_PINMUX_MODE: Use PINMUX mode.
 * \param[in] pwr_mode: Config power of pad. Please refer to \ref x3e_PAD_Power_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_SHUTDOWN: Shutdown power of pad.
 *            - PAD_IS_PWRON: Enable power of pad.
 * \param[in] pullup_config: Config pad pull mode. Please refer to \ref x3e_PAD_Pull_Mode.
 *            This parameter can be one of the following values:
 *            - PAD_PULL_NONE: No pull.
 *            - PAD_PULL_UP: Pull this pin up.
 *            - PAD_PULL_DOWN: Pull this pin down.
 * \param[in] output_en: Config pad output function, which only valid when PAD_SW_MODE. Please refer to \ref x3e_PAD_Output_Config.
 *            This parameter can be one of the following values:
 *            - PAD_OUT_DISABLE: Disable pin output.
 *            - PAD_OUT_ENABLE: Enable pad output.
 * \param[in] output_val: Config pin output level, which only valid when PAD_SW_MODE and output mode. Please refer to \ref x3e_PAD_Output_Value.
 *            This parameter can be one of the following values:
 *            - PAD_OUT_LOW: Pad output low.
 *            - PAD_OUT_HIGH: Pad output high.
 *
 * @return   The result of get pad config is success or fail.
 * @retval 0   The pad config is get success.
 * @retval -1  The pad config is get failure.
 *
 * <b>Example usage</b>
 * \code{.c}
 * void pad_demo(void)
 * {
 *    int32_t  get_result;
 *    get_result = Pad_GetConfig(P2_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
 * }
 */
int32_t Pad_GetConfig(uint8_t pin_num,
                      PAD_Mode *mode,
                      PAD_PWR_Mode *pwr_mode,
                      PAD_Pull_Mode *pullup_config,
                      PAD_OUTPUT_ENABLE_Mode *output_en,
                      PAD_OUTPUT_VAL *output_val);

#ifdef __cplusplus
}
#endif

#endif /* _RTL876X_PINMUX_H_ */

/** @} */ /* End of group 87x3e_PINMUX_Exported_Functions */
/** @} */ /* End of group 87x3e_PINMUX */


/******************* (C) COPYRIGHT 2024 Realtek Semiconductor *****END OF FILE****/

