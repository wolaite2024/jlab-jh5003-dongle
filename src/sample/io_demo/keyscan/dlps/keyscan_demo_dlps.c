/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     keyscan_demo_dlps.c
* @brief    This file provides demo code of keyscan working in dlps mode.
* @details
* @author   renee
* @date     2017-02-22
* @version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "rtl876x_nvic.h"
#include "rtl876x_keyscan.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "vector_table.h"
#include "trace.h"
#include "pm.h"
#include "io_dlps.h"
#include "os_timer.h"
#include "os_task.h"
#include "os_msg.h"
#include "string.h"

/** @defgroup  KEYSCAN_DEMO_DLPS  KEYSCAN DEMO DLPS
    * @brief  keyscan work in system dlps mode implementation demo code
    * @{
    */

/** @defgroup Keyscan_Dlps_Exported_Functions Keyscan Dlps Exported Functions
  * @brief
  * @{
  */
/*============================================================================*
 *                         Types
 *============================================================================*/
typedef struct
{
    uint16_t Length;            /**< Keyscan state register */
    uint16_t key[26];
} KeyScanDataStruct, *pKeyScanDataStruct;

/* keyscan message type */
typedef struct
{
    uint16_t msgType;
    union
    {
        uint32_t parm;
        void *pBuf;
    };
} KeyscanMsg;
/** @} */ /* End of group Keyscan_Dlps_Exported_Types */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup Keyscan_Dlps_Exported_Macros Keyscan Dlps Exported Macros
  * @brief
  * @{
  */

/* keypad row and column */
#define KEYPAD_ROW_SIZE       2
#define KEYPAD_COLUMN_SIZE    2
#define COLUMN0               ADC_0
#define COLUMN1               ADC_1
#define ROW0                  ADC_2
#define ROW1                  ADC_3

#define KEYSCAN_FIFO_SIZE           26

/* queue size */
#define IO_DEMO_EVENT_QUEUE_SIZE        0x10

/* event */
#define IO_DEMO_EVENT_KEYSCAN_SCAN_END            0x01
#define IO_DEMO_EVENT_KEYSCAN_ALL_RELEASE         0x02

/** @} */ /* End of group Keyscan_Dlps_Exported_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup Keyscan_Dlps_Exported_Variables Keyscan Dlps Exported Variables
  * @brief
  * @{
  */

/* task handle & queue handle */
static void *iodemo_app_task_handle;
static void *io_queue_handle;

/* To record filter data */
static uint8_t RepeatReport = 0;

static KeyScanDataStruct  CurKeyData;
static KeyScanDataStruct  PreKeyData;

/* To keep System Active */
static uint8_t keyscanallowdlps = true;

/** @} */ /* End of group Keyscan_Dlps_Exported_Variables */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup Keyscan_Dlps_Exported_Functions Keyscan Dlps Exported Functions
  * @brief
  * @{
  */
static void io_demo_task(void *param);
static void keyscan_handler(void);

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
static void board_keyscan_init(void)
{
    Pad_Config(COLUMN0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(COLUMN1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(ROW0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(ROW1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(COLUMN0, KEY_COL_0);
    Pinmux_Config(COLUMN1, KEY_COL_1);
    Pinmux_Config(ROW0, KEY_ROW_0);
    Pinmux_Config(ROW1, KEY_ROW_1);
}

/**
  * @brief  Initialize Keyscan peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_keyscan_init(void)
{
    /* data struct init */
    memset(&CurKeyData, 0, sizeof(KeyScanDataStruct));
    memset(&PreKeyData, 0, sizeof(KeyScanDataStruct));

    RepeatReport = false;

    RCC_PeriphClockCmd(APBPeriph_KEYSCAN, APBPeriph_KEYSCAN_CLOCK, ENABLE);
    KEYSCAN_InitTypeDef  KeyScan_InitStruct;
    KeyScan_StructInit(&KeyScan_InitStruct);
    KeyScan_InitStruct.colSize         = KEYPAD_COLUMN_SIZE;
    KeyScan_InitStruct.rowSize         = KEYPAD_ROW_SIZE;
    KeyScan_InitStruct.scanInterval    = 0xFA;      //50 ms
    KeyScan_InitStruct.debounceEn      = KeyScan_Debounce_Disable;
    KeyScan_InitStruct.clockdiv        = 0x3E8;
    KeyScan_InitStruct.releasecnt      = 0x01;
    KeyScan_Init(KEYSCAN, &KeyScan_InitStruct);

    RamVectorTableUpdate(Keyscan_VECTORn, keyscan_handler);
    KeyScan_INTConfig(KEYSCAN, KEYSCAN_INT_SCAN_END | KEYSCAN_INT_ALL_RELEASE, ENABLE);

    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = KeyScan_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    KeyScan_Cmd(KEYSCAN, ENABLE);
}

/**
  * @brief  IO Enter dlps call back function.
  * @param   No parameter.
  * @return  void
  */
static void io_dlps_enter(void)
{
    /* switch pad to Software mode */
    Pad_ControlSelectValue(COLUMN0, PAD_SW_MODE);
    Pad_ControlSelectValue(COLUMN1, PAD_SW_MODE);
    Pad_ControlSelectValue(ROW0, PAD_SW_MODE);
    Pad_ControlSelectValue(ROW1, PAD_SW_MODE);

    /* Wake up pin config */
    System_WakeUpPinEnable(ROW0, PAD_WAKEUP_POL_LOW);
    System_WakeUpPinEnable(ROW1, PAD_WAKEUP_POL_LOW);

    /* To Debug */
    DBG_DIRECT("io_dlps_enter");
}

/**
  * @brief  IO Exit dlps call back function.
  * @param   No parameter.
  * @return  void
  */
static void io_dlps_exit(void)
{
    /* switch pad to Software mode */
    Pad_ControlSelectValue(COLUMN0, PAD_PINMUX_MODE);
    Pad_ControlSelectValue(COLUMN1, PAD_PINMUX_MODE);
    Pad_ControlSelectValue(ROW0, PAD_PINMUX_MODE);
    Pad_ControlSelectValue(ROW1, PAD_PINMUX_MODE);

    if ((System_WakeUpInterruptValue(ROW0) == SET) || (System_WakeUpInterruptValue(ROW1) == SET))
    {
        keyscanallowdlps = false;
    }

    /* To Debug */
    IO_PRINT_INFO0("io_dlps_exit");
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
static bool io_dlps_check(void)
{
    return keyscanallowdlps;
}

/**
  * @brief  IO Enter dlps check function.
  * @param   No parameter.
  * @return  void
  */
static void power_keyscan_init(void)
{
    power_check_cb_register(io_dlps_check);

    io_dlps_register();
    io_dlps_register_enter_cb(io_dlps_enter);
    io_dlps_register_exit_cb(io_dlps_exit);

    /*set btmac to dsm mode*/
    bt_power_mode_set(BTPOWER_DEEP_SLEEP);
    /*set platfrom as dlps mode*/
    power_mode_set(POWER_DLPS_MODE);
}

static void keyscan_release_handle(void)
{
    keyscanallowdlps = true;
    IO_PRINT_INFO0("keyscan_release_handle: All key release");
    memset(&CurKeyData, 0, sizeof(KeyScanDataStruct));
    memset(&PreKeyData, 0, sizeof(KeyScanDataStruct));
}

/**
  * @brief  Initialize peripheral Task and IO message queue.
  * @param   No parameter.
  * @return  void
  */
void keyscan_demo_dlps(void)
{
    /* create io test task */
    os_task_create(&iodemo_app_task_handle, "app", io_demo_task, NULL, 384 * 4, 2);

    /* create event queue and message queue */
    os_msg_queue_create(&io_queue_handle, "keyscanQ", IO_DEMO_EVENT_QUEUE_SIZE, sizeof(KeyscanMsg));
}

/**
  * @brief  IO_Demo Task Handle.
  * @param   No parameter.
  * @return  void
  */
static void io_demo_task(void *param)
{
    KeyscanMsg msg;

    /* Initialize pinmux & pad */
    board_keyscan_init();

    /* KeyScan Peripheral configuration */
    driver_keyscan_init();

    /* power setting */
    power_keyscan_init();

    while (1)
    {
        if (os_msg_recv(io_queue_handle, &msg, 0xFFFFFFFF) == true)
        {
            if (msg.msgType == IO_DEMO_EVENT_KEYSCAN_SCAN_END)
            {
                pKeyScanDataStruct pKeyData = (pKeyScanDataStruct)(msg.pBuf);
                for (uint16_t i = 0; i < pKeyData->Length; i++)
                {
                    IO_PRINT_INFO2("io_demo_task: pKeyData->key[%d] 0x%x", i, pKeyData->key[i]);
                }
            }
            else if (msg.msgType == IO_DEMO_EVENT_KEYSCAN_ALL_RELEASE)
            {
                keyscan_release_handle();
            }
        }
    }
}

/**
* @brief  Keyscan interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void keyscan_handler(void)
{
    pKeyScanDataStruct pKeyData = &CurKeyData;
    KeyscanMsg msg;

    if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_INT_FLAG_ALL_RELEASE) == SET)   //all release
    {
        /* Mask keyscan interrupt */
        KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_ALL_RELEASE, ENABLE);

        msg.msgType = IO_DEMO_EVENT_KEYSCAN_ALL_RELEASE;
        if (os_msg_send(io_queue_handle, &msg, 0) == false)
        {
            IO_PRINT_ERROR0("keyscan_handler: Send queue fail");
        }
        /* clear & Unmask keyscan interrupt */
        KeyScan_ClearINTPendingBit(KEYSCAN, KEYSCAN_INT_ALL_RELEASE);
        KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_ALL_RELEASE, DISABLE);
    }
    if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_INT_FLAG_SCAN_END) == SET)   // scan finish
    {
        /* Mask keyscan interrupt */
        KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_SCAN_END, ENABLE);

        /* KeyScan fifo not empty */
        if (KeyScan_GetFlagState(KEYSCAN, KEYSCAN_FLAG_EMPTY) != SET)
        {
            /* Read fifo data */
            uint16_t len = KeyScan_GetFifoDataNum(KEYSCAN);
            if (len > KEYSCAN_FIFO_SIZE)
            {
                len = KEYSCAN_FIFO_SIZE;
            }
            KeyScan_Read(KEYSCAN, (uint16_t *)(&pKeyData->key[0]), len);
            pKeyData->Length = len;

            if (!RepeatReport)
            {
                if (!memcmp(pKeyData, &PreKeyData, sizeof(KeyScanDataStruct)))
                {
                    goto UNMASK_INT;
                }
                else
                {
                    memcpy(&PreKeyData, pKeyData, sizeof(KeyScanDataStruct));
                }
            }

            msg.msgType = IO_DEMO_EVENT_KEYSCAN_SCAN_END;
            msg.pBuf = (void *)pKeyData;
            if (os_msg_send(io_queue_handle, &msg, 0) == false)
            {
                IO_PRINT_ERROR0("keyscan_handler: Send queue fail");
            }
        }

UNMASK_INT:
        /* clear & Unmask keyscan interrupt */
        KeyScan_ClearINTPendingBit(KEYSCAN, KEYSCAN_INT_SCAN_END);
        KeyScan_INTMask(KEYSCAN, KEYSCAN_INT_SCAN_END, DISABLE);
    }
}

/** @} */ /* End of group Keyscan_Dlps_Exported_Functions */
/** @} */ /* End of group KEYSCAN_DEMO_DLPS */

