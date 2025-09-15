/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     external_flash.c
* @brief    This file provides driver for flash.
* @details
* @author
* @date     2021-07-29
* @version  v1.0
*********************************************************************************************************
*/


/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "platform_utils.h"
#include "external_flash.h"
#include "external_flash_test.h"
#include "rtl876x_spi.h"
#include "rtl876x_nvic.h"
#include "hal_gpio.h"
#include "rtl876x_gdma.h"
#include "dma_channel.h"
#include "vector_table.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"

/** @addtogroup SPI_EXTERNAL_FLASH External Flash
  * @brief External flash SPI module
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup External_Flash_SPI_Macros External flash data transfered by SPI Macros
    * @brief
    * @{
    */

/* if use user external flash, config here */
#define FLASH_SCK                   P5_0
#define FLASH_MOSI                  P5_2
#define FLASH_MISO                  P5_3
#define FLASH_CS                    P5_1
#define FLASH_HOLD                  P5_5    //Some flash has hold function in default, such as ZB25VQ64A 

#define FLASH_DBG                   0
#define TIME_OUT_MAX                12000000
#define TX_FIFO_SIZE                64

#define FLASH_SPI                   SPI0
#define Flash_IRQChannel            SPI0_IRQn
#define Flash_VECTOR                SPI0_VECTORn
#define FLASH_SPI_TX_GDMA_HANDSHAKE GDMA_Handshake_SPI0_TX
#define FLASH_SPI_RX_GDMA_HANDSHAKE GDMA_Handshake_SPI0_RX

#define FLASH_DP_RELEASE_DELAY_US   10      //delay after flash release deep sleep cmd, refer to flash spec 
#define FLASH_SW_RESET_DELAY_US     60      //delay after flash sw reset cmd, refer to flash spec
#define FLASH_PAGE_SIZE             256     //flash page size ,max write length

/** @} */ /* End of group External_Flash_SPI_Macros */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup External_Flash_SPI_Variables External flash data transfered by SPI Variables
    * @brief
    * @{
    */
static EXT_FLASH_SPI_DATA ext_flash_spi_data = {0};
static uint8_t *pdmabuf = NULL;

static uint8_t spi_tx_dma_ch_num = 0xa5;
static uint8_t spi_rx_dma_ch_num = 0xa5;
#define SPI_TX_DMA_CHANNEL_NUM     spi_tx_dma_ch_num
#define SPI_TX_DMA_CHANNEL         DMA_CH_BASE(spi_tx_dma_ch_num)
#define SPI_TX_DMA_IRQ             DMA_CH_IRQ(spi_tx_dma_ch_num)

#define SPI_RX_DMA_CHANNEL_NUM     spi_rx_dma_ch_num
#define SPI_RX_DMA_CHANNEL         DMA_CH_BASE(spi_rx_dma_ch_num)
#define SPI_RX_DMA_IRQ             DMA_CH_IRQ(spi_rx_dma_ch_num)

/** @} */ /* End of group External_Flash_SPI_Variables */

static void spi_tx_dma_handler(void);
static void spi_rx_dma_handler(void);
static void Flash_SPI_Handler(void);

/*============================================================================*
 *                              Functions
 *============================================================================*/

/** @defgroup External_Flash_SPI_Exported_Functions External flash data transfered by SPI Functions
    * @brief
    * @{
    */

/**
  * @brief  Clear external flash spi rx fifo.
  * @param   No parameter.
  * @return  void
  */
static void ext_flash_spi_clear_rx_fifo(void)
{
    uint16_t recv_len = 0;

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
    while (recv_len)
    {
        SPI_ReceiveData(FLASH_SPI);
        recv_len--;
    }
}

/**
  * @brief  Flash deep sleep control.
  * @param  enable              enable or disable flash deep sleep
  * @return none
  *
  */
static void ext_flash_spi_ds_ctrl(bool enable)
{
    uint8_t sendBuf[4] = {EXT_FLASH_RELEASE_DEEP_SLEEP, 0, 0, 0};
    uint8_t send_len = 4;

    /* Check SPI communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Release flash deep sleep */
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    ext_flash_spi_clear_rx_fifo();

    platform_delay_us(FLASH_DP_RELEASE_DELAY_US);

    if (enable)
    {
        /* Send deep sleep command */
        SPI_SendData(FLASH_SPI, EXT_FLASH_DEEP_SLEEP);
        ext_flash_spi_clear_rx_fifo();
    }

#if FLASH_DBG
    IO_PRINT_INFO1("ext_flash_spi_ds_ctrl: enable %d", enable);
#endif
    return;
}

/**
  * @brief  Flash software reset.
  * @param  none
  * @return none
  *
  */
static void ext_flash_spi_sw_reset(void)
{
    /* Check SPI communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Send sw reset enable command */
    SPI_SendData(FLASH_SPI, EXT_FLASH_SOFTWARE_RESET_ENABLE);

    /* SW reset enable cmd and sw reser cmd must be separated */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Send sw reset command */
    SPI_SendData(FLASH_SPI, EXT_FLASH_SOFTWARE_RESET);

    ext_flash_spi_clear_rx_fifo();

    platform_delay_us(FLASH_SW_RESET_DELAY_US);

#if FLASH_DBG
    IO_PRINT_INFO0("ext_flash_spi_sw_reset");
#endif
    return;
}

/**
  * @brief  Flash deep sleep control.
  * @param  enable              enable or disable flash deep sleep
  * @return none
  *
  */
static void ext_flash_spi_hold_ctrl(bool enable)
{
    if (enable)
    {
        Pad_Config(FLASH_HOLD, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(FLASH_HOLD, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }

#if FLASH_DBG
    IO_PRINT_INFO1("ext_flash_spi_hold_ctrl: enable %d", enable);
#endif
    return;
}

/**
  * @brief  Read ID of flash chip.
  * @param   No parameter.
  * @return  flash id
  *
  */
uint8_t ext_flash_spi_read_id(void)
{
    uint8_t sendBuf[5] = {EXT_FLASH_READ_ID_CMD, 0, 0, 0, 0};
    uint8_t send_len = 5;
    uint8_t recvBuf[5] = {0};
    uint8_t index = 0;
    uint8_t recv_len = 0;

    /* Check SPI communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Clear receive FIFO */
    recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
    while (recv_len)
    {
        SPI_ReceiveData(FLASH_SPI);
        recv_len--;
    }
    /* Read ID of Flash */
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            recvBuf[index++] = SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }

    }
#if FLASH_DBG
    IO_PRINT_INFO4("ext_flash_spi_read_id: recvBuf 0x%X, 0x%X, 0x%X, 0x%X",
                   recvBuf[1],
                   recvBuf[2],
                   recvBuf[3],
                   recvBuf[4]
                  );
#endif
    return recvBuf[1];
}

/**
  * @brief Read status register of flash.
  * @param  none.
  * @return value of status register.
  *
  */
uint8_t ext_flash_spi_read_status(void)
{
    uint8_t sendBuf[2] = {EXT_FLASH_READ_STATUS_CMD, 0};
    uint8_t send_len = 2;
    uint8_t recvBuf[2] = {0};
    uint8_t recv_len = 0;
    uint8_t index = 0;

    /* Check SPI communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Send command of write status register */
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            recvBuf[index++] = SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    return recvBuf[1];
}

/**
  * @brief Check flash is in progress or not.
  * @param  none.
  * @return Check write status
  * @retval FLASH_OPERATION_CHECK_STATUS_FAILED     Flash is always in progress
  * @retval FLASH_OPERATION_SUCCESS                 Flash is in standby mode
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_check_write_status(void)
{
    uint8_t reg_value = 0;
    uint32_t time_out = TIME_OUT_MAX;

    do
    {
        /* Time out control */
        time_out--;
        if (time_out == 0)
        {
            return FLASH_OPERATION_CHECK_STATUS_FAILED;
        }
        reg_value = ext_flash_spi_read_status();
    }
    while (reg_value & EXT_FLASH_STATUS_WIP); /* Check flash is communicating or not */

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief  Send write enable command before every page program(PP),
  * sector earse(SE), block earse(BE), chip earse(CE) and write status register(WRSR) command.
  * @param  NewState                             enable or disable flash write
  * @return Check write status
  * @retval FLASH_OPERATION_WRITE_CMD_FAILED     Write timeout
  * @retval FLASH_OPERATION_SUCCESS              Write success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_write_enable_cmd(FunctionalState NewState)
{
    uint8_t send_len = 1;
    uint8_t recv_len = 0;

#if FLASH_DBG
    IO_PRINT_INFO1("ext_flash_spi_write_enable_cmd: NewState 0x%X", NewState);
#endif

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_WRITE_CMD_FAILED;
    }

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Send write command */
    if (NewState == ENABLE)
    {
        /* Send write enable command */
        SPI_SendData(FLASH_SPI, EXT_FLASH_WRITE_ENABLE_CMD);
    }
    else
    {
        /* Send write disable command */
        SPI_SendData(FLASH_SPI, EXT_FLASH_WRITE_DISABLE_CMD);
    }

    /* Read data */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_WRITE_CMD_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief  Send write status register command.
  * @param  status: data whch want to write to status register.
  * @return Check write status
  * @retval FLASH_OPERATION_WRITE_CMD_FAILED     Write timeout
  * @retval FLASH_OPERATION_SUCCESS              Write success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_write_status_register(uint8_t status)
{
    uint8_t sendBuf[2] = {EXT_FLASH_WRITE_STATUS_CMD, 0};
    uint8_t send_len = 2;
    uint8_t recv_len = 0;

    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Write status register */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);
    sendBuf[1] = status;
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_WRITE_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Earse flash.
  * @param  address: address which begin to earse.
  * @param  mode: select earse mode.
  * @return Check erase status
  * @retval FLASH_OPERATION_ERASE_FAILED        Erase timeout
  * @retval FLASH_OPERATION_SUCCESS             Erase success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_erase(uint32_t address, EXT_FLASH_OPERATION_CMD mode)
{
    uint8_t sendBuf[4] = {0, 0, 0, 0};
    uint8_t send_len = 4;
    uint16_t recv_len = 0;

#if FLASH_DBG
    IO_PRINT_INFO1("ext_flash_spi_erase: mode 0x%X", mode);
#endif
    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Write data */
    sendBuf[0] = mode;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;

    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data no matter it is useful or not */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_ERASE_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Earse chip.
  * @param  none.
  * @return Check chip erase status
  * @retval FLASH_OPERATION_ERASE_FAILED        Erase chip timeout
  * @retval FLASH_OPERATION_SUCCESS             Erase chip success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_erase_chip(void)
{
    uint8_t sendBuf[4] = {0, 0, 0, 0};
    uint8_t send_len = 1;
    uint16_t recv_len = 0;

#if FLASH_DBG
    IO_PRINT_INFO0("ext_flash_spi_erase_chip");
#endif

    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Write data */
    sendBuf[0] = EXT_FLASH_CHIP_ERASE_CMD;
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);
    SPI_SendBuffer(FLASH_SPI, sendBuf, send_len);

    /* Read data no matter it is useful or not */
    while (send_len)
    {
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recv_len)
        {
            SPI_ReceiveData(FLASH_SPI);
            recv_len--;
            send_len--;
        }
    }

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_ERASE_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Page Program flash.
  * @param  address: address which begin to write.
  * @param  psendBuf: address of data buffer which want to write.
  * @param  len: length of data buffer which want to write, range is 1 to 256.
  * @return Check program status
  * @retval FLASH_OPERATION_WRITE_FAILED        Program timeout
  * @retval FLASH_OPERATION_SUCCESS             Program success
  * @retval FLASH_OPERATION_WRITE_LENGTH_ERROR  Program length is over 256
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_page_program(uint32_t address, uint8_t *psendBuf,
                                                          uint16_t len)
{
    uint8_t sendBuf[4];

#if FLASH_DBG
    IO_PRINT_INFO2("ext_flash_spi_page_program: address 0x%X, len %d", address, len);
#endif

    if (len > FLASH_PAGE_SIZE)
    {
        return FLASH_OPERATION_WRITE_LENGTH_ERROR;
    }

    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to Tx only mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_TxOnly);

    /* Send write command */
    sendBuf[0] = EXT_FLASH_PROGRAM_CMD;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;
    SPI_SendBuffer(FLASH_SPI, sendBuf, 4);

    /* Write data */
    SPI_SendBuffer(FLASH_SPI, psendBuf, len);

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to full duplex mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);

    /* Clear rx fifo */
    ext_flash_spi_clear_rx_fifo();

    /* Check flash status */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_WRITE_FAILED;
    }

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Write flash by interrupt.
  * @param  address: address which begin to write.
  * @param  psendBuf: address of data buffer which want to write.
  * @param  len: length of data buffer which want to write, range is 1 to 256.
  * @return Check interrupt program status
  * @retval FLASH_OPERATION_WRITE_FAILED        Last program or read not finish
  * @retval FLASH_OPERATION_SUCCESS             Program success
  * @retval FLASH_OPERATION_WRITE_LENGTH_ERROR  Program length is over 256
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_page_program_by_interrupt(uint32_t address,
                                                                       uint8_t *psendBuf,
                                                                       uint16_t len)
{
    uint8_t sendBuf[4];
    NVIC_InitTypeDef NVIC_InitStruct;

#if FLASH_DBG
    IO_PRINT_INFO2("ext_flash_spi_page_program_by_interrupt: address 0x%X, len %d",
                   address,
                   len);
#endif

    /* SPI will change to EEPROM mode when in reading process */
    if (ext_flash_spi_data.read_count || ext_flash_spi_data.write_count)
    {
        return FLASH_OPERATION_WRITE_FAILED;
    }

    if (len > FLASH_PAGE_SIZE)
    {
        return FLASH_OPERATION_WRITE_LENGTH_ERROR;
    }

    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to Tx only mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_TxOnly);

    /* Use GPIO as CS to prevent CS pull high when trigger TXE interrupt */
    hal_gpio_init();
    hal_gpio_init_pin(FLASH_CS, GPIO_TYPE_CORE, GPIO_DIR_OUTPUT, GPIO_PULL_UP);
    hal_gpio_set_level(FLASH_CS, GPIO_LEVEL_LOW);

    /* Send write command */
    sendBuf[0] = EXT_FLASH_PROGRAM_CMD;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;
    ext_flash_spi_data.write_count = len;
    ext_flash_spi_data.write_index = 0;
    ext_flash_spi_data.program_buffer = psendBuf;
    SPI_SendBuffer(FLASH_SPI, sendBuf, 4);

    /* Write data */
    if (len >= TX_FIFO_SIZE)
    {
        SPI_SendBuffer(FLASH_SPI, psendBuf, TX_FIFO_SIZE);
        ext_flash_spi_data.write_index += TX_FIFO_SIZE;
    }
    else
    {
        SPI_SendBuffer(FLASH_SPI, psendBuf, len);
        ext_flash_spi_data.write_index += len;
    }

    RamVectorTableUpdate(Flash_VECTOR, Flash_SPI_Handler);
    SPI_INTConfig(FLASH_SPI, SPI_INT_TXE, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel = Flash_IRQChannel;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Write flash by dma.
  * @param  address: address which begin to write.
  * @param  psendBuf: address of data buffer which want to write.
  * @param  len: length of data buffer which want to write, range is 1 to 256.
  * @return Check dma program status
  * @retval FLASH_OPERATION_WRITE_FAILED         Last program or read not finish
  * @retval FLASH_OPERATION_SUCCESS              Program success
  * @retval FLASH_OPERATION_WRITE_LENGTH_ERROR   Program length is over 256
  * @retval FLASH_OPERATION_NO_GDMA_FREE_CHANNEL No free dma channel to be used
  * @retval FLASH_OPERATION_NO_GDMA_FREE_CHANNEL No free buffer
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_page_program_by_dma(uint32_t address,
                                                                 uint8_t *psendBuf,
                                                                 uint16_t len)
{
    uint16_t total_len = len + 4;

#if FLASH_DBG
    IO_PRINT_INFO2("ext_flash_spi_page_program_by_dma: address 0x%X, len %d", address,
                   len);
#endif

    /* SPI will change to EEPROM mode when in reading process */
    if (ext_flash_spi_data.read_count || ext_flash_spi_data.write_count)
    {
        return FLASH_OPERATION_WRITE_FAILED;
    }

    if (len > FLASH_PAGE_SIZE)
    {
        return FLASH_OPERATION_WRITE_LENGTH_ERROR;
    }

    if (!GDMA_channel_request(&spi_tx_dma_ch_num, spi_tx_dma_handler, false))
    {
        IO_PRINT_INFO0("ext_flash_spi_page_program_by_dma: dma channel request fail");
        return FLASH_OPERATION_NO_GDMA_FREE_CHANNEL;
    }

    pdmabuf = (uint8_t *)malloc(total_len);

    if (pdmabuf == NULL)
    {
        IO_PRINT_INFO0("ext_flash_spi_page_program_by_dma: buf malloc fail");
        return FLASH_OPERATION_GDMA_MALLOC_FAILED;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    /*--------------GDMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_TX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize      = total_len;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)pdmabuf;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)FLASH_SPI->DR;
    GDMA_InitStruct.GDMA_DestHandshake = FLASH_SPI_TX_GDMA_HANDSHAKE;

    GDMA_Init(SPI_TX_DMA_CHANNEL, &GDMA_InitStruct);

    GDMA_INTConfig(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_TX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    /* Enable write */
    ext_flash_spi_write_enable_cmd(ENABLE);

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to Tx only mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_TxOnly);

    /* Send write command */
    pdmabuf[0] = EXT_FLASH_PROGRAM_CMD;
    pdmabuf[1] = (address >> 16) & 0xff;
    pdmabuf[2] = (address >> 8) & 0xff;
    pdmabuf[3] = address & 0xff;
    memcpy(&pdmabuf[4], psendBuf, len);
    ext_flash_spi_data.write_count = len;

    SPI_GDMACmd(FLASH_SPI, SPI_GDMAReq_Tx, DISABLE);
    SPI_GDMACmd(FLASH_SPI, SPI_GDMAReq_Tx, ENABLE);
    GDMA_Cmd(SPI_TX_DMA_CHANNEL_NUM, ENABLE);

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Read flash.
  * @param  address: address which begin to read.
  * @param  pStoreBuf: address of data buffer which want to read.
  * @param  len: length of data buffer which want to read.
  * @return Check read status
  * @retval FLASH_OPERATION_READ_FAILED         Read timeout
  * @retval FLASH_OPERATION_SUCCESS             Read success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_read(uint32_t address, uint8_t *pStoreBuf,
                                                  uint16_t len)
{
    uint32_t recvlen = 0;
    uint8_t sendBuf[4] = {EXT_FLASH_READ_CMD, 0, 0, 0};
    uint32_t time_out = TIME_OUT_MAX;

#if FLASH_DBG
    IO_PRINT_INFO2("ext_flash_spi_read: address 0x%X, len %d", address, len);
#endif

    /* Read flash status register */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_READ_FAILED;
    }

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to EEPROM mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_EEPROM);

    /* Clear RX FIFO */
    ext_flash_spi_clear_rx_fifo();

    /* Configure length of data which you want to read */
    SPI_SetReadLen(FLASH_SPI, len);

    /* Send read command and address */
    sendBuf[0] = 0x03;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;
    SPI_SendBuffer(FLASH_SPI, sendBuf, 4);

    /* Wait RX FIFO not empty flag */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_RFNE) == RESET);

    /* Read data */
    while (len)
    {
        recvlen = SPI_GetRxFIFOLen(FLASH_SPI);
        while (recvlen)
        {
            *pStoreBuf++ = (uint8_t)SPI_ReceiveData(FLASH_SPI);
            recvlen--;
            len--;
        }
        if (recvlen == 0)
        {
            if ((--time_out) == 0)
            {
                return FLASH_OPERATION_READ_FAILED;
            }
        }
    }
    /* Switch to full duplex mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Read flash by interrupt.
  * @param  address: address which begin to read.
  * @param  pStoreBuf: address of data buffer which want to read.
  * @param  len: length of data buffer which want to read.
  * @return Check read status
  * @retval FLASH_OPERATION_READ_FAILED         Last read or write not finish
  * @retval FLASH_OPERATION_SUCCESS             Read success
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_read_by_interrupt(uint32_t address, uint8_t *pStoreBuf,
                                                               uint16_t len)
{
    uint8_t sendBuf[4] = {EXT_FLASH_READ_CMD, 0, 0, 0};
    NVIC_InitTypeDef NVIC_InitStruct;

#if FLASH_DBG
    IO_PRINT_INFO2("ext_flash_spi_read_by_interrupt: address 0x%X, len %d", address,
                   len);
#endif

    /* Last read finish */
    if (ext_flash_spi_data.read_count)
    {
        return FLASH_OPERATION_READ_FAILED;
    }

    /* Read flash status register */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_READ_FAILED;
    }

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to EEPROM mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_EEPROM);

    /* Clear RX FIFO */
    ext_flash_spi_clear_rx_fifo();

    /* Configure length of data which you want to read */
    SPI_SetReadLen(FLASH_SPI, len);

    /* Send read command and address */
    sendBuf[0] = 0x03;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;
    ext_flash_spi_data.read_buffer = pStoreBuf;
    ext_flash_spi_data.read_count = len;
    ext_flash_spi_data.read_index = 0;

    RamVectorTableUpdate(Flash_VECTOR, Flash_SPI_Handler);
    SPI_INTConfig(FLASH_SPI, SPI_INT_RXF, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel = Flash_IRQChannel;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    SPI_SendBuffer(FLASH_SPI, sendBuf, 4);

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief Read flash by dma.
  * @param  address: address which begin to read.
  * @param  pStoreBuf: address of data buffer which want to read.
  * @param  len: length of data buffer which want to read.
  * @return Check read status
  * @retval FLASH_OPERATION_READ_FAILED         Last read or write not finish
  * @retval FLASH_OPERATION_SUCCESS             Read success
  * @retval FLASH_OPERATION_NO_GDMA_FREE_CHANNEL No free dma channel to be used
  *
  */
EXT_FLASH_SPI_OPERATION_STATUS ext_flash_spi_read_by_dma(uint32_t address, uint8_t *pStoreBuf,
                                                         uint16_t len)
{
    uint8_t sendBuf[4] = {EXT_FLASH_READ_CMD, 0, 0, 0};

#if FLASH_DBG
    IO_PRINT_INFO2("ext_flash_spi_read_by_dma: address 0x%X, len %d", address,
                   len);
#endif

    /* Last read finish */
    if (ext_flash_spi_data.read_count)
    {
        return FLASH_OPERATION_READ_FAILED;
    }

    /* Read flash status register */
    if (ext_flash_spi_check_write_status())
    {
        return FLASH_OPERATION_READ_FAILED;
    }

    if (!GDMA_channel_request(&spi_rx_dma_ch_num, spi_rx_dma_handler, false))
    {
        IO_PRINT_INFO0("ext_flash_spi_read_by_dma: dma channel request fail");
        return FLASH_OPERATION_NO_GDMA_FREE_CHANNEL;
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    /*--------------GDMA init-----------------------------*/
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum      = SPI_RX_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR             = GDMA_DIR_PeripheralToMemory;
    GDMA_InitStruct.GDMA_BufferSize      = len;
    GDMA_InitStruct.GDMA_SourceInc       = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc  = DMA_DestinationInc_Inc;
    GDMA_InitStruct.GDMA_SourceDataSize  = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)FLASH_SPI->DR;
    GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)(pStoreBuf);
    GDMA_InitStruct.GDMA_SourceHandshake = FLASH_SPI_RX_GDMA_HANDSHAKE;

    GDMA_Init(SPI_RX_DMA_CHANNEL, &GDMA_InitStruct);

    GDMA_INTConfig(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    /*-----------------GDMA IRQ init-------------------*/
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = SPI_RX_DMA_IRQ;
    nvic_init_struct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&nvic_init_struct);

    /* Check communication status */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);

    /* Switch to EEPROM mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_EEPROM);

    /* Clear RX FIFO */
    ext_flash_spi_clear_rx_fifo();

    SPI_GDMACmd(FLASH_SPI, SPI_GDMAReq_Rx, DISABLE);
    SPI_GDMACmd(FLASH_SPI, SPI_GDMAReq_Rx, ENABLE);

    GDMA_Cmd(SPI_RX_DMA_CHANNEL_NUM, ENABLE);

    /* Configure length of data which you want to read */
    SPI_SetReadLen(FLASH_SPI, len);

    /* Send read command and address */
    sendBuf[0] = 0x03;
    sendBuf[1] = (address >> 16) & 0xff;
    sendBuf[2] = (address >> 8) & 0xff;
    sendBuf[3] = address & 0xff;
    ext_flash_spi_data.read_count = len;

    SPI_SendBuffer(FLASH_SPI, sendBuf, 4);

    return FLASH_OPERATION_SUCCESS;
}

/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void board_ext_flash_spi_init(void)
{
    Pinmux_Config(FLASH_SCK, SPI0_CLK_MASTER);
    Pinmux_Config(FLASH_MOSI, SPI0_MO_MASTER);
    Pinmux_Config(FLASH_MISO, SPI0_MI_MASTER);
    Pinmux_Config(FLASH_CS, SPI0_SS_N_0_MASTER);

    Pad_Config(FLASH_SCK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(FLASH_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(FLASH_MISO, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(FLASH_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}

/**
  * @brief  Initialize SPI peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_ext_flash_spi_init(void)
{
    SPI_InitTypeDef  SPI_InitStructure;

    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 100;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_TxThresholdLevel  = 64;
    SPI_InitStructure.SPI_RxThresholdLevel  = 0;
    SPI_InitStructure.SPI_TxWaterlevel        = 7;
    SPI_InitStructure.SPI_RxWaterlevel        = 1;

    SPI_Init(FLASH_SPI, &SPI_InitStructure);
    SPI_Cmd(FLASH_SPI, ENABLE);
}

/**
  * @brief  Initialize Flash chip.
  * @param   No parameter.
  * @return  void
  */
void ext_flash_spi_init(void)
{
    uint8_t id = 0;

    board_ext_flash_spi_init();
    driver_ext_flash_spi_init();

    /* If flash has hold function,, must disable before communication */
    ext_flash_spi_hold_ctrl(DISABLE);

    /* Flash release deep sleep */
    ext_flash_spi_ds_ctrl(false);

    /* Flash sw reset */
    ext_flash_spi_sw_reset();

    id = ext_flash_spi_read_id();

    /* Check Flash ID */
    if (EXT_FLASH_ID != id)
    {
        //Add error handle code here
        IO_PRINT_INFO1("ext_flash_spi_init: id 0x%x mismatch", id);
    }
}

/**
* @brief  SPI tx GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void spi_tx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SPI_TX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);

    ext_flash_spi_data.write_count = 0;

    /* It is recommended to post the os msg to the task thread for data processing. */
    flash_test_send_msg(FLASH_TEST_GDMA_MODE, FLASH_TEST_WRITE_FINISH);

    /* Must wait for SPI free before change CS pin */
    while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);
    /* Clear rx fifo */
    ext_flash_spi_clear_rx_fifo();

    GDMA_channel_release(SPI_TX_DMA_CHANNEL_NUM);
}

/**
* @brief  SPI rx GDMA interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void spi_rx_dma_handler(void)
{
    GDMA_ClearINTPendingBit(SPI_RX_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    ext_flash_spi_data.read_count = 0;

    /* It is recommended to post the os msg to the task thread for data processing. */
    flash_test_send_msg(FLASH_TEST_GDMA_MODE, FLASH_TEST_READ_FINISH);
    /* Switch to full duplex mode */
    SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);
    GDMA_channel_release(SPI_RX_DMA_CHANNEL_NUM);
}

/**
* @brief  SPI interrupt handler function.
* @param   No parameter.
* @return  void
*/
static void Flash_SPI_Handler(void)
{
    uint8_t idx = 0;
    uint16_t recv_len = 0;

    if (SPI_GetINTStatus(FLASH_SPI, SPI_INT_RXF) == SET)
    {
        SPI_INTConfig(FLASH_SPI, SPI_INT_RXF, DISABLE);
        recv_len = SPI_GetRxFIFOLen(FLASH_SPI);
        for (idx = 0; idx < recv_len; idx++)
        {
            /* must read all data in receive FIFO , otherwise cause SPI_INT_RXF interrupt again */
            if (ext_flash_spi_data.read_index < ext_flash_spi_data.read_count)
            {
                ext_flash_spi_data.read_buffer[ext_flash_spi_data.read_index] = SPI_ReceiveData(
                                                                                    FLASH_SPI);
                ext_flash_spi_data.read_index++;
            }
            else
            {
                SPI_ReceiveData(FLASH_SPI);
            }
        }
        if (ext_flash_spi_data.read_index == ext_flash_spi_data.read_count)
        {
            ext_flash_spi_data.read_count = 0;

            /* It is recommended to post the os msg to the task thread for data processing. */
            flash_test_send_msg(FLASH_TEST_INTERRUPT_MODE, FLASH_TEST_READ_FINISH);
            /* Switch to full duplex mode */
            SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);
        }
        else
        {
            SPI_INTConfig(FLASH_SPI, SPI_INT_RXF, ENABLE);
        }
    }

    if (SPI_GetINTStatus(FLASH_SPI, SPI_INT_TXE) == SET)
    {
        SPI_INTConfig(FLASH_SPI, SPI_INT_TXE, DISABLE);
        if (ext_flash_spi_data.write_index >= ext_flash_spi_data.write_count)
        {
            ext_flash_spi_data.write_count = 0;

            /* It is recommended to post the os msg to the task thread for data processing. */
            flash_test_send_msg(FLASH_TEST_INTERRUPT_MODE, FLASH_TEST_WRITE_FINISH);
            /* Must wait for SPI free before change CS pin */
            while (SPI_GetFlagState(FLASH_SPI, SPI_FLAG_BUSY) == SET);
            Pinmux_Config(FLASH_CS, SPI0_SS_N_0_MASTER);
            SPI_ChangeDirection(FLASH_SPI, SPI_Direction_FullDuplex);
            /* Clear rx fifo */
            ext_flash_spi_clear_rx_fifo();
        }
        else
        {
            if ((ext_flash_spi_data.write_count - ext_flash_spi_data.write_index) >= TX_FIFO_SIZE)
            {
                SPI_SendBuffer(FLASH_SPI, &
                               (ext_flash_spi_data.program_buffer[ext_flash_spi_data.write_index]), TX_FIFO_SIZE);
                ext_flash_spi_data.write_index += TX_FIFO_SIZE;
            }
            else
            {
                SPI_SendBuffer(FLASH_SPI, &
                               (ext_flash_spi_data.program_buffer[ext_flash_spi_data.write_index]),
                               ext_flash_spi_data.write_count - ext_flash_spi_data.write_index);
                ext_flash_spi_data.write_index = ext_flash_spi_data.write_count;
            }
            SPI_INTConfig(FLASH_SPI, SPI_INT_TXE, ENABLE);
        }
    }
}
/** @} */ /* End of group External_Flash_SPI_Exported_Functions */

/** @} */ /* End of group SPI_EXTERNAL_FLASH */

/******************* (C) COPYRIGHT 2021 Realtek Semiconductor Corporation *****END OF FILE****/

