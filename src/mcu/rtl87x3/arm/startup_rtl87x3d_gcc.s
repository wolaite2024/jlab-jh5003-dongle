;/*****************************************************************************/
;/*     startup_rtl87x3d_gcc.s: Startup file for Realtek RTL87x3d device series     */
;/*****************************************************************************/
#include "mem_config.h"
/**
**===========================================================================
**  Definitions
**===========================================================================
*/
    .syntax unified
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb

  .global    g_pfnVectors
  .global    Default_Handler
  .type    Default_Handler, %function 


.equ _estack, APP_GLOBAL_ADDR
/**
**===========================================================================
**  Program - Reset_Handler
**  Abstract: This code gets called after a reset event.  
**===========================================================================
*/
  .section RESET
  .weak    Reset_Handler
  .type    Reset_Handler, %function
Reset_Handler:
    IF     :DEF:__USE_C_LIBRARY_INIT; // use c library init

    LDR    R0, =system_init
    BLX    R0
    LDR    R0, =__main
    BX     R0

    ELSE

    ; write our init flow: copy rw, encrypted and clear zi
    ; data on rw, data on zi, buffer on rw, buffer on zi

    ; copy data on rw
    IMPORT |Load$$RAM_GLOBAL$$RW$$Base|
    IMPORT |Image$$RAM_GLOBAL$$RW$$Base|
    IMPORT |Image$$RAM_GLOBAL$$RW$$Length|
    LDR R0, =|Load$$RAM_GLOBAL$$RW$$Base|
    LDR R1, =|Image$$RAM_GLOBAL$$RW$$Base|
    LDR R2, =|Image$$RAM_GLOBAL$$RW$$Length|
    CMP R2, #0
    BEQ end_of_copy_data_on_rw
/**
**===========================================================================
**  Program - Default_Handler
**  Abstract: This code gets called when the processor receives an
**    unexpected interrupt.
**===========================================================================
*/
  .section    .text,"ax",%progbits
  Default_Handler:
    b   .
    .size  Default_Handler, .-Default_Handler 
/**
**===========================================================================
**  Reset, Exception, and Interrupt vectors
**===========================================================================
*/

  .section VECTOR,"a",%progbits
  .type    g_pfnVectors, %object
  .size    g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  /* Processor exception vectors */
  .word    _estack
  .word    Reset_Handler             ; Reset Handler
  .word    NMI_Handler               ; NMI Handler
  .word    HardFault_Handler         ; Hard Fault Handler
  .word    MemManage_Handler         ; MPU Fault Handler
  .word    BusFault_Handler          ; Bus Fault Handler
  .word    UsageFault_Handler        ; Usage Fault Handler
  .word    0                         ; Reserved
  .word    0                         ; Reserved
  .word    0                         ; Reserved
  .word    0                         ; Reserved
  .word    SVC_Handler               ; SVCall Handler
  .word    DebugMon_Handler          ; Debug Monitor Handler
  .word    0                         ; Reserved
  .word    PendSV_Handler            ; PendSV Handler
  .word    SysTick_Handler           ; SysTick Handler

  ; External Interrupts
  .word    System_Handler            ;[0]  System On interrupt
  .word    WDT_Handler               ;[1]  Watch dog global insterrupt
  .word    BTMAC_Handler             ;[2]  See Below Table ( an Extension of interrupt )
  .word    USB_IP_Handler  ;[3]  Master Peripheral IP interrupt (usb or sdio)
  .word    TIM2_Handler              ;[4]  Timer2 global interrupt
  .word    Platform_Handler          ;[5]  Platform interrupt (platfrom error interrupt)
  .word    I2S0_TX_Handler            ;[6]  I2S0 interrupt
  .word    I2S0_RX_Handler            ;[7]  I2S1 interrupt
  .word    UART2_Handler             ;[8]  Data_Uart1 interrupt (used for DSP)
  .word    GPIOA0_Handler            ;[9]  GPIOA0 interrupt
  .word    GPIOA1_Handler            ;[10] GPIOA1 interrupt
  .word    UART1_Handler             ;[11] Log_Uart interrupt (Log0)
  .word    UART0_Handler             ;[12] Data_Uart interrupt
  .word    RTC_Handler               ;[13] Realtime counter interrupt
  .word    SPI0_Handler              ;[14] SPI0 interrupt
  .word    SPI1_Handler              ;[15] SPI1 interrupt
  .word    I2C0_Handler              ;[16] I2C0 interrupt
  .word    I2C1_Handler              ;[17] I2C1 interrupt
  .word    ADC_Handler               ;[18] ADC global interrupt
  .word    Peripheral_Handler        ;[19] See Below Table ( an Extension of interrupt )
  .word    GDMA0_Channel0_Handler    ;[20] RTK-DMA0 channel 0 global interrupt
  .word    GDMA0_Channel1_Handler    ;[21] RTK-DMA0 channel 1 global interrupt
  .word    GDMA0_Channel2_Handler    ;[22] RTK-DMA0 channel 2 global interrupt
  .word    GDMA0_Channel3_Handler    ;[23] RTK-DMA0 channel 3 global interrupt
  .word    GDMA0_Channel4_Handler    ;[24] RTK-DMA0 channel 4 global interrupt
  .word    GDMA0_Channel5_Handler    ;[25] RTK-DMA0 channel 5 global interrupt
  .word    KeyScan_Handler           ;[26] keyscan global interrupt
  .word    QDEC_Handler              ;[27] qdecode global interrupt
  .word    IR_Handler                ;[28] IR module global interrupt
  .word    DSP_Handler               ;[29] DSP interrupt
  .word    GDMA0_Channel6_Handler    ;[30] RTK-DMA0 channel 6 global interrupt
  .word    GDMA0_Channel7_Handler    ;[31] RTK-DMA0 channel 7 global interrupt
  .word    GDMA0_Channel8_Handler    ;[32] RTK-DMA0 channel 8 global interrupt
  .word    GDMA0_Channel9_Handler    ;[33] RTK-DMA0 channel 9 global interrupt
  .word    GDMA0_Channel10_Handler   ;[34] RTK-DMA0 channel 6 global interrupt
  .word    GDMA0_Channel11_Handler   ;[35] RTK-DMA0 channel 7 global interrupt
  .word    GDMA0_Channel12_Handler   ;[36] RTK-DMA0 channel 8 global interrupt
  .word    GDMA0_Channel13_Handler   ;[37] RTK-DMA0 channel 9 global interrupt
  .word    GDMA0_Channel14_Handler   ;[38] RTK-DMA0 channel 6 global interrupt
  .word    GDMA0_Channel15_Handler   ;[39] RTK-DMA0 channel 7 global interrupt
  .word    TIM3_Handler              ;[40] Timer3 global interrupt
  .word    TIM4_Handler              ;[41] Timer4 global interrupt
  .word    TIM5_Handler              ;[42] Timer5 global interrupt
  .word    TIM6_Handler              ;[43] Timer6 global interrupt
  .word    TIM7_Handler              ;[44] Timer7 global interrupt
  .word    TIM8to11_Handler              ;[45] TRNG interrupt
  .word    GPIO_A_9_Handler          ;[46] GPIO_A[9] interrupt
  .word    GPIO_A_16_Handler         ;[47] GPIO_A[16] interrupt
  .word    GPIO_A_2to7_Handler       ;[48] GPIO_A[2:7] interrupt
  .word    GPIO_A_8_10to15_Handler   ;[49] GPIO_A[8,10:15] interrupt
  .word    GPIO_A_17to23_Handler     ;[50] GPIO_A[17:23] interrupt
  .word    GPIO_A_24to31_Handler     ;[51] GPIO_A[24:31] interrupt
  .word    GPIO_B_0to7_Handler       ;[52] GPIO_B[0:7] interrupt
  .word    GPIO_B_8to15_Handler      ;[53] GPIO_B[8:15] interrupt
  .word    GPIO_B_16to23_Handler     ;[54] GPIO_B[16:23] interrupt
  .word    GPIO_B_24to31_Handler     ;[55] GPIO_B[24:31] interrupt
  .word    UART3_Handler             ;[56] Uart3 interrupt(used for DSP)
  .word    SPI2_Handler              ;[57] SPI2 interrupt
  .word    I2C2_Handler              ;[58] I2C2 interrupt
  .word    DSP_Event_1_Handler       ;[59] DSP event interrupt 1
  .word    DSP_Event_2_Handler       ;[60] DSP event interrupt 2
  .word    SHA256_Handler            ;[61] SHA256 interrupt
  .word    USB_ISOC_Handler          ;[62] USB ISO interrupt
  .word    BT_CLK_compare_Handler    ;[63] BT clock compare interrupt
  .word    HW_RSA_Handler            ;[64] HW RSA interrupt
  .word    PSRAM_Handler             ;[65] PSRAM interrupt
  .word    Compare_clk_4_Handler     ;[66] Compare Clock interrupt 4
  .word    Compare_clk_5_Handler     ;[67] Compare Clock interrupt 5
  .word    dspdma_int_all_host_Handler    ;[68] RTK-DMA0 channel 0 global interrupt
  .word    NNA_Handler    ;[69] RTK-DMA0 channel 1 global interrupt
  .word    Compare_clk_9_Handler     ;[70] RTK-DMA0 channel 2 global interrupt
  .word    Compare_clk_10_Handler    ;[71] RTK-DMA0 channel 3 global interrupt
  .word    GPIO_C_0to7_Handler       ;[72] RTK-DMA0 channel 4 global interrupt
  .word    GPIO_C_8to15_Handler      ;[73] RTK-DMA0 channel 5 global interrupt
  .word    0                         ;[74] RTK-DMA0 channel 6 global interrupt
  .word    0                         ;[75] RTK-DMA0 channel 7 global interrupt
  .word    TIM8_Handler              ;[76] Timer8 global interrupt
  .word    TIM9_Handler              ;[77] Timer9 global interrupt
  .word    TIM10_Handler             ;[78] Timer10 global interrupt
  .word    TIM11_Handler             ;[79] Timer11 global interrupt
  .word    TIM12_Handler             ;[80] Timer12 global interrupt
  .word    TIM13_Handler             ;[81] Timer13 global interrupt
  .word    TIM14_Handler             ;[82] Timer14 global interrupt
  .word    TIM15_Handler             ;[83] Timer15 global interrupt
  .word    SPIC0_Handler             ;[84] SPIC0 interrupt
  .word    SPIC1_Handler             ;[85] SPIC1 interrupt
  .word    SPIC2_Handler             ;[86] SPIC2 interrupt
  .word    SPIC3_Handler             ;[87] SPIC3 interrupt
  .word    SDIO_HOST_Handler         ;[88] SDIO host interrupt
  .word    SPIC4_Handler
  .word    ERR_CORR_CODE_Handler
  .word    SLAVE_PORT_MONITOR_Handler
   ;       Name,                     IRQ_NUM,  StatusBit
  .word    RESERVED0_Handler         ;19,      0,
  .word    RESERVED1_Handler         ;19,      1,
  .word    RESERVED2_Handler         ;19,      2,
  .word    RESERVED3_Handler         ;19,      3,
  .word    RESERVED4_Handler         ;19,      4,
  .word    RESERVED5_Handler         ;19,      5,
  .word    RESERVED6_Handler         ;19,      6,
  .word    SPDIF_Rx_Handler          ;19,      7,
  .word    SPDIF_Tx_Handler          ;19,      8,
  .word    MBIAS_MFG_DET_L_Handler   ;19,      9,
  .word    SPI2W_Handler             ;19,      10,
  .word    LPCOMP_Handler            ;19,      11,
  .word    MBIAS_VBAT_DET_Handler    ;19,      12,
  .word    MBIAS_ADP_DET_Handler     ;19,      13,
  .word    HW_ASRC1_Handler          ;19,      14,
  .word    HW_ASRC2_Handler          ;19,      15,
  .word    VADBUF_Handler            ;19,      16,
  .word    PTA_Mailbox_Handler       ;19,      17,
  .word    DSP2MCU_Handler           ;19,      18,
  .word    SPORT2_TX_Handler         ;19,      19,
  .word    SPORT2_RX_Handler         ;19,      20,
  .word    SPORT3_TX_Handler         ;19,      21,
  .word    SPORT3_RX_Handler         ;19,      22,
  .word    VAD_Handler               ;19,      23,
  .word    ANC_Handler               ;19,      24,
  .word    SPORT0_TX_Handler         ;19,      25,
  .word    SPORT0_RX_Handler         ;19,      26,
  .word    SPORT1_TX_Handler         ;19,      27,
  .word    SPORT1_RX_Handler         ;19,      28,
  .word    USB_UTMI_SUSPEND_N_Handler;19,      29,
  .word    USB_DLPS_Resume_Handler   ;19,      30,
  .word    RESERVED7_Handler        ;19,      31,
  .word    ADP_IN_DET_Handler
  .word    ADP_OUT_DET_Handler
  .word    GPIOA2_Handler
  .word    GPIOA3_Handler
  .word    GPIOA4_Handler
  .word    GPIOA5_Handler
  .word    GPIOA6_Handler
  .word    GPIOA7_Handler
  .word    GPIOA8_Handler
  .word    GPIOA9_Handler
  .word    GPIOA10_Handler
  .word    GPIOA11_Handler
  .word    GPIOA12_Handler
  .word    GPIOA13_Handler
  .word    GPIOA14_Handler
  .word    GPIOA15_Handler
  .word    GPIOA16_Handler
  .word    GPIOA17_Handler
  .word    GPIOA18_Handler
  .word    GPIOA19_Handler
  .word    GPIOA20_Handler
  .word    GPIOA21_Handler
  .word    GPIOA22_Handler
  .word    GPIOA23_Handler
  .word    GPIOA24_Handler
  .word    GPIOA25_Handler
  .word    GPIOA26_Handler
  .word    GPIOA27_Handler
  .word    GPIOA28_Handler
  .word    GPIOA29_Handler
  .word    GPIOA30_Handler
  .word    GPIOA31_Handler
  .word    GPIOB0_Handler
  .word    GPIOB1_Handler
  .word    GPIOB2_Handler
  .word    GPIOB3_Handler
  .word    GPIOB4_Handler
  .word    GPIOB5_Handler
  .word    GPIOB6_Handler
  .word    GPIOB7_Handler
  .word    GPIOB8_Handler
  .word    GPIOB9_Handler
  .word    GPIOB10_Handler
  .word    GPIOB11_Handler
  .word    GPIOB12_Handler
  .word    GPIOB13_Handler
  .word    GPIOB14_Handler
  .word    GPIOB15_Handler
  .word    GPIOB16_Handler
  .word    GPIOB17_Handler
  .word    GPIOB18_Handler
  .word    GPIOB19_Handler
  .word    GPIOB20_Handler
  .word    GPIOB21_Handler
  .word    GPIOB22_Handler
  .word    GPIOB23_Handler
  .word    GPIOB24_Handler
  .word    GPIOB25_Handler
  .word    GPIOB26_Handler
  .word    GPIOB27_Handler
  .word    GPIOB28_Handler
  .word    GPIOB29_Handler
  .word    GPIOB30_Handler
  .word    GPIOB31_Handler
  .word    GPIOC0_Handler
  .word    GPIOC1_Handler
  .word    GPIOC2_Handler
  .word    GPIOC3_Handler
  .word    GPIOC4_Handler
  .word    GPIOC5_Handler
  .word    GPIOC6_Handler
  .word    GPIOC7_Handler
  .word    GPIOC8_Handler
  .word    GPIOC9_Handler
  .word    GPIOC10_Handler
  .word    GPIOC11_Handler
  .word    GPIOC12_Handler
  .word    GPIOC13_Handler
  .word    GPIOC14_Handler
  .word    GPIOC15_Handler
  

copy_data_on_rw:
                LDR R3, [R0]
                ADDS R0, R0, #4
                STR R3, [R1]
                ADDS R1, R1, #4
                SUBS R2, R2, #4
                BNE copy_data_on_rw
end_of_copy_data_on_rw:

                ; clear data on zi
                IMPORT |Image$$RAM_GLOBAL$$ZI$$Base|
                IMPORT |Image$$RAM_GLOBAL$$ZI$$Length|
                LDR R0, =|Image$$RAM_GLOBAL$$ZI$$Base|
                LDR R1, =|Image$$RAM_GLOBAL$$ZI$$Length|
                CMP R1, #0
                BEQ end_of_clear_data_on_zi
clear_data_on_zi:
                STR R2, [R0]
                ADDS R0, R0, #4
                SUBS R1, R1, #4
                BNE clear_data_on_zi
end_of_clear_data_on_zi:

                ; copy buffer on rw
                IMPORT |Load$$RAM_TEXT$$RW$$Base|
                IMPORT |Image$$RAM_TEXT$$RW$$Base|
                IMPORT |Image$$RAM_TEXT$$RW$$Length|
                LDR R0, =|Load$$RAM_TEXT$$RW$$Base|
                LDR R1, =|Image$$RAM_TEXT$$RW$$Base|
                LDR R2, =|Image$$RAM_TEXT$$RW$$Length|
                CMP R2, #0
                BEQ end_of_copy_buffer_on_rw
copy_buffer_on_rw:
                LDR R3, [R0]
                ADDS R0, R0, #4
                STR R3, [R1]
                ADDS R1, R1, #4
                SUBS R2, R2, #4
                BNE copy_buffer_on_rw
end_of_copy_buffer_on_rw:

                ; copy buffer on ro (for ram function)
                IMPORT |Load$$RAM_TEXT$$RO$$Base|
                IMPORT |Image$$RAM_TEXT$$RO$$Base|
                IMPORT |Image$$RAM_TEXT$$RO$$Length|
                LDR R0, =|Load$$RAM_TEXT$$RO$$Base|
                LDR R1, =|Image$$RAM_TEXT$$RO$$Base|
                LDR R2, =|Image$$RAM_TEXT$$RO$$Length|
                CMP R2, #0
                BEQ end_of_copy_buffer_on_ro
copy_buffer_on_ro:
                LDR R3, [R0]
                ADDS R0, R0, #4
                STR R3, [R1]
                ADDS R1, R1, #4
                SUBS R2, R2, #4
                BNE copy_buffer_on_ro
end_of_copy_buffer_on_ro:

                ; clear buffer on zi
                IMPORT |Image$$RAM_TEXT$$ZI$$Base|
                IMPORT |Image$$RAM_TEXT$$ZI$$Length|
                LDR R0, =|Image$$RAM_TEXT$$ZI$$Base|
                LDR R1, =|Image$$RAM_TEXT$$ZI$$Length|
                CMP R1, #0
                BEQ end_of_clear_buffer_on_zi
clear_buffer_on_zi:
                STR R2, [R0]
                ADDS R0, R0, #4
                SUBS R1, R1, #4
                BNE clear_buffer_on_zi
				NOP
end_of_clear_buffer_on_zi:


                IMPORT system_init
                IMPORT main
                LDR    R0, =system_init
                BLX    R0
                LDR    R0, =main
                BX     R0

                ENDIF ; end of macro __USE_C_LIBRARY_INIT

                ENDP ; end of Reset_Handler


                AREA    |.text|, CODE, READONLY
				
/**
**===========================================================================
** Provide weak aliases for each Exception handler to the Default_Handler.
**===========================================================================
*/
  .weak  NMI_Handler
  .thumb_set NMI_Handler,Default_Handler
  
  .weak  HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler
  
  .weak  MemManage_Handler
  .thumb_set MemManage_Handler,Default_Handler
  
  .weak  BusFault_Handler
  .thumb_set BusFault_Handler,Default_Handler
  
  .weak  UsageFault_Handler
  .thumb_set UsageFault_Handler,Default_Handler
  
  .weak  SVC_Handler
  .thumb_set SVC_Handler,Default_Handler
  
  .weak  DebugMon_Handler
  .thumb_set DebugMon_Handler,Default_Handler
  
  .weak  PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler
  
  .weak  SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler
  
  .weak  System_Handler
  .thumb_set System_Handler,Default_Handler
  
  .weak  WDT_Handler
  .thumb_set WDT_Handler,Default_Handler
  
  .weak  BTMAC_Handler
  .thumb_set BTMAC_Handler,Default_Handler
  
  .weak  USB_IP_Handler
  .thumb_set USB_IP_Handler,Default_Handler
  
  .weak  TIM2_Handler
  .thumb_set TIM2_Handler,Default_Handler
  
  .weak  Platform_Handler
  .thumb_set Platform_Handler,Default_Handler
  
  .weak  I2S0_TX_Handler
  .thumb_set I2S0_TX_Handler,Default_Handler
  
  .weak  I2S0_RX_Handler
  .thumb_set I2S0_RX_Handler,Default_Handler
  
  .weak  UART2_Handler
  .thumb_set UART2_Handler,Default_Handler
  
  .weak  GPIOA0_Handler
  .thumb_set GPIOA0_Handler,Default_Handler
  
  .weak  GPIOA1_Handler
  .thumb_set GPIOA1_Handler,Default_Handler
  
  .weak  UART1_Handler
  .thumb_set UART1_Handler,Default_Handler
  
  .weak  UART0_Handler
  .thumb_set UART0_Handler,Default_Handler
  
  .weak  RTC_Handler
  .thumb_set RTC_Handler,Default_Handler
  
  .weak  SPI0_Handler
  .thumb_set SPI0_Handler,Default_Handler
  
  .weak  SPI1_Handler
  .thumb_set SPI1_Handler,Default_Handler
  
  .weak  I2C0_Handler
  .thumb_set I2C0_Handler,Default_Handler
  
  .weak  I2C1_Handler
  .thumb_set I2C1_Handler,Default_Handler
  
  .weak  ADC_Handler
  .thumb_set ADC_Handler,Default_Handler
  
  .weak  Peripheral_Handler
  .thumb_set Peripheral_Handler,Default_Handler
  
  .weak  GDMA0_Channel0_Handler
  .thumb_set GDMA0_Channel0_Handler,Default_Handler
  
  .weak  GDMA0_Channel1_Handler
  .thumb_set GDMA0_Channel1_Handler,Default_Handler
  
  .weak  GDMA0_Channel2_Handler
  .thumb_set GDMA0_Channel2_Handler,Default_Handler
  
  .weak  GDMA0_Channel3_Handler
  .thumb_set GDMA0_Channel3_Handler,Default_Handler
  
  .weak  GDMA0_Channel4_Handler
  .thumb_set GDMA0_Channel4_Handler,Default_Handler
  
  .weak  GDMA0_Channel5_Handler
  .thumb_set GDMA0_Channel5_Handler,Default_Handler
  
  .weak  KeyScan_Handler
  .thumb_set KeyScan_Handler,Default_Handler
  
  .weak  QDEC_Handler
  .thumb_set QDEC_Handler,Default_Handler
  
  .weak  IR_Handler
  .thumb_set IR_Handler,Default_Handler
  
  .weak  DSP_Handler
  .thumb_set DSP_Handler,Default_Handler
  
  .weak  GDMA0_Channel6_Handler
  .thumb_set GDMA0_Channel6_Handler,Default_Handler
  
  .weak  GDMA0_Channel7_Handler
  .thumb_set GDMA0_Channel7_Handler,Default_Handler
  
  .weak  GDMA0_Channel8_Handler
  .thumb_set GDMA0_Channel8_Handler,Default_Handler
  
  .weak  GDMA0_Channel9_Handler
  .thumb_set GDMA0_Channel9_Handler,Default_Handler
  
  .weak  GDMA0_Channel10_Handler
  .thumb_set GDMA0_Channel10_Handler,Default_Handler
  
  .weak  GDMA0_Channel11_Handler
  .thumb_set GDMA0_Channel11_Handler,Default_Handler
  
  .weak  GDMA0_Channel12_Handler
  .thumb_set GDMA0_Channel12_Handler,Default_Handler
  
  .weak  GDMA0_Channel13_Handler
  .thumb_set GDMA0_Channel13_Handler,Default_Handler
  
  .weak  GDMA0_Channel14_Handler
  .thumb_set GDMA0_Channel14_Handler,Default_Handler
  
  .weak  GDMA0_Channel15_Handler
  .thumb_set GDMA0_Channel15_Handler,Default_Handler
  
  .weak  TIM3_Handler
  .thumb_set TIM3_Handler,Default_Handler
  
  .weak  TIM4_Handler
  .thumb_set TIM4_Handler,Default_Handler
  
  .weak  TIM5_Handler
  .thumb_set TIM5_Handler,Default_Handler
  
  .weak  TIM6_Handler
  .thumb_set TIM6_Handler,Default_Handler
  
  .weak  TIM7_Handler
  .thumb_set TIM7_Handler,Default_Handler
  
  .weak  TIM8to11_Handler
  .thumb_set TIM8to11_Handler,Default_Handler
  
  .weak  GPIO_A_9_Handler
  .thumb_set GPIO_A_9_Handler,Default_Handler
  
  .weak  GPIO_A_16_Handler
  .thumb_set GPIO_A_16_Handler,Default_Handler
  
  .weak  GPIO_A_2to7_Handler
  .thumb_set GPIO_A_2to7_Handler,Default_Handler
  
  .weak  GPIO_A_8_10to15_Handler
  .thumb_set GPIO_A_8_10to15_Handler,Default_Handler
  
  .weak  GPIO_A_17to23_Handler
  .thumb_set GPIO_A_17to23_Handler,Default_Handler
  
  .weak  GPIO_A_24to31_Handler
  .thumb_set GPIO_A_24to31_Handler,Default_Handler
  
  .weak  GPIO_B_0to7_Handler
  .thumb_set GPIO_B_0to7_Handler,Default_Handler
  
  .weak  GPIO_B_8to15_Handler
  .thumb_set GPIO_B_8to15_Handler,Default_Handler
  
  .weak  GPIO_B_16to23_Handler
  .thumb_set GPIO_B_16to23_Handler,Default_Handler
  
  .weak  GPIO_B_24to31_Handler
  .thumb_set GPIO_B_24to31_Handler,Default_Handler
  
  .weak  UART3_Handler
  .thumb_set UART3_Handler,Default_Handler
  
  .weak  SPI2_Handler
  .thumb_set SPI2_Handler,Default_Handler
  
  .weak  I2C2_Handler
  .thumb_set I2C2_Handler,Default_Handler
  
  .weak  DSP_Event_1_Handler
  .thumb_set DSP_Event_1_Handler,Default_Handler
  
  .weak  DSP_Event_2_Handler
  .thumb_set DSP_Event_2_Handler,Default_Handler
  
  .weak  SHA256_Handler
  .thumb_set SHA256_Handler,Default_Handler
  
  .weak  USB_ISOC_Handler
  .thumb_set USB_ISOC_Handler,Default_Handler
  
  .weak  BT_CLK_compare_Handler
  .thumb_set BT_CLK_compare_Handler,Default_Handler
  
  .weak  HW_RSA_Handler
  .thumb_set HW_RSA_Handler,Default_Handler
  
  .weak  PSRAM_Handler
  .thumb_set PSRAM_Handler,Default_Handler
  
  .weak  Compare_clk_4_Handler
  .thumb_set Compare_clk_4_Handler,Default_Handler
  
  .weak  Compare_clk_5_Handler
  .thumb_set Compare_clk_5_Handler,Default_Handler
  
  .weak  dspdma_int_all_host_Handler
  .thumb_set dspdma_int_all_host_Handler,Default_Handler
  
  .weak  NNA_Handler
  .thumb_set NNA_Handler,Default_Handler
  
  .weak  Compare_clk_9_Handler
  .thumb_set Compare_clk_9_Handler,Default_Handler
  
  .weak  Compare_clk_10_Handler
  .thumb_set Compare_clk_10_Handler,Default_Handler
  
  .weak  GPIO_C_0to7_Handler
  .thumb_set GPIO_C_0to7_Handler,Default_Handler
  
  .weak  GPIO_C_8to15_Handler
  .thumb_set GPIO_C_8to15_Handler,Default_Handler
  
  .weak  TIM8_Handler
  .thumb_set TIM8_Handler,Default_Handler
  
  .weak  TIM9_Handler
  .thumb_set TIM9_Handler,Default_Handler
  
  .weak  TIM10_Handler
  .thumb_set TIM10_Handler,Default_Handler
  
  .weak  TIM11_Handler
  .thumb_set TIM11_Handler,Default_Handler
  
  .weak  TIM12_Handler
  .thumb_set TIM12_Handler,Default_Handler
  
  .weak  TIM13_Handler
  .thumb_set TIM13_Handler,Default_Handler
  
  .weak  TIM14_Handler
  .thumb_set TIM14_Handler,Default_Handler
  
  .weak  TIM15_Handler
  .thumb_set TIM15_Handler,Default_Handler
  
  .weak  SPIC0_Handler
  .thumb_set SPIC0_Handler,Default_Handler
  
  .weak  SPIC1_Handler
  .thumb_set SPIC1_Handler,Default_Handler
  
  .weak  SPIC2_Handler
  .thumb_set SPIC2_Handler,Default_Handler
  
  .weak  SPIC3_Handler
  .thumb_set SPIC3_Handler,Default_Handler
  
  .weak  SDIO_HOST_Handler
  .thumb_set SDIO_HOST_Handler,Default_Handler
  
  .weak  SPIC4_Handler
  .thumb_set SPIC4_Handler,Default_Handler
  
  .weak  ERR_CORR_CODE_Handler
  .thumb_set ERR_CORR_CODE_Handler,Default_Handler
  
  .weak  SLAVE_PORT_MONITOR_Handler
  .thumb_set SLAVE_PORT_MONITOR_Handler,Default_Handler
  
  .weak  RESERVED0_Handler
  .thumb_set RESERVED0_Handler,Default_Handler
  
  .weak  RESERVED1_Handler
  .thumb_set RESERVED1_Handler,Default_Handler
  
  .weak  RESERVED2_Handler
  .thumb_set RESERVED2_Handler,Default_Handler
  
  .weak  RESERVED3_Handler
  .thumb_set RESERVED3_Handler,Default_Handler
  
  .weak  RESERVED4_Handler
  .thumb_set RESERVED4_Handler,Default_Handler
  
  .weak  RESERVED5_Handler
  .thumb_set RESERVED5_Handler,Default_Handler
  
  .weak  RESERVED6_Handler
  .thumb_set RESERVED6_Handler,Default_Handler
  
  .weak  SPDIF_Rx_Handler
  .thumb_set SPDIF_Rx_Handler,Default_Handler
  
  .weak  SPDIF_Tx_Handler
  .thumb_set SPDIF_Tx_Handler,Default_Handler
  
  .weak  MBIAS_MFG_DET_L_Handler
  .thumb_set MBIAS_MFG_DET_L_Handler,Default_Handler
  
  .weak  SPI2W_Handler
  .thumb_set SPI2W_Handler,Default_Handler
  
  .weak  LPCOMP_Handler
  .thumb_set LPCOMP_Handler,Default_Handler
  
  .weak  MBIAS_VBAT_DET_Handler
  .thumb_set MBIAS_VBAT_DET_Handler,Default_Handler
  
  .weak  MBIAS_ADP_DET_Handler
  .thumb_set MBIAS_ADP_DET_Handler,Default_Handler
  
  .weak  HW_ASRC1_Handler
  .thumb_set HW_ASRC1_Handler,Default_Handler
  
  .weak  HW_ASRC2_Handler
  .thumb_set HW_ASRC2_Handler,Default_Handler
  
  .weak  VADBUF_Handler
  .thumb_set VADBUF_Handler,Default_Handler
  
  .weak  PTA_Mailbox_Handler
  .thumb_set PTA_Mailbox_Handler,Default_Handler
  
  .weak  DSP2MCU_Handler
  .thumb_set DSP2MCU_Handler,Default_Handler
  
  .weak  SPORT2_TX_Handler
  .thumb_set SPORT2_TX_Handler,Default_Handler
  
  .weak  SPORT2_RX_Handler
  .thumb_set SPORT2_RX_Handler,Default_Handler
  
  .weak  SPORT3_TX_Handler
  .thumb_set SPORT3_TX_Handler,Default_Handler
  
  .weak  SPORT3_RX_Handler
  .thumb_set SPORT3_RX_Handler,Default_Handler
  
  .weak  VAD_Handler
  .thumb_set VAD_Handler,Default_Handler
  
  .weak  ANC_Handler
  .thumb_set ANC_Handler,Default_Handler
  
  .weak  SPORT0_TX_Handler
  .thumb_set SPORT0_TX_Handler,Default_Handler
  
  .weak  SPORT0_RX_Handler
  .thumb_set SPORT0_RX_Handler,Default_Handler
  
  .weak  SPORT1_TX_Handler
  .thumb_set SPORT1_TX_Handler,Default_Handler
  
  .weak  SPORT1_RX_Handler
  .thumb_set SPORT1_RX_Handler,Default_Handler
  
  .weak  USB_UTMI_SUSPEND_N_Handler
  .thumb_set USB_UTMI_SUSPEND_N_Handler,Default_Handler
  
  .weak  USB_DLPS_Resume_Handler
  .thumb_set USB_DLPS_Resume_Handler,Default_Handler
  
  .weak  RESERVED7_Handler
  .thumb_set RESERVED7_Handler,Default_Handler
  
  .weak  ADP_IN_DET_Handler
  .thumb_set ADP_IN_DET_Handler,Default_Handler
  
  .weak  ADP_OUT_DET_Handler
  .thumb_set ADP_OUT_DET_Handler,Default_Handler
  
  .weak  GPIOA2_Handler
  .thumb_set GPIOA2_Handler,Default_Handler
  
  .weak  GPIOA3_Handler
  .thumb_set GPIOA3_Handler,Default_Handler
  
  .weak  GPIOA4_Handler
  .thumb_set GPIOA4_Handler,Default_Handler
  
  .weak  GPIOA5_Handler
  .thumb_set GPIOA5_Handler,Default_Handler
  
  .weak  GPIOA6_Handler
  .thumb_set GPIOA6_Handler,Default_Handler
  
  .weak  GPIOA7_Handler
  .thumb_set GPIOA7_Handler,Default_Handler
  
  .weak  GPIOA8_Handler
  .thumb_set GPIOA8_Handler,Default_Handler
  
  .weak  GPIOA9_Handler
  .thumb_set GPIOA9_Handler,Default_Handler
  
  .weak  GPIOA10_Handler
  .thumb_set GPIOA10_Handler,Default_Handler
  
  .weak  GPIOA11_Handler
  .thumb_set GPIOA11_Handler,Default_Handler
  
  .weak  GPIOA12_Handler
  .thumb_set GPIOA12_Handler,Default_Handler
  
  .weak  GPIOA13_Handler
  .thumb_set GPIOA13_Handler,Default_Handler
  
  .weak  GPIOA14_Handler
  .thumb_set GPIOA14_Handler,Default_Handler
  
  .weak  GPIOA15_Handler
  .thumb_set GPIOA15_Handler,Default_Handler
  
  .weak  GPIOA16_Handler
  .thumb_set GPIOA16_Handler,Default_Handler
  
  .weak  GPIOA17_Handler
  .thumb_set GPIOA17_Handler,Default_Handler
  
  .weak  GPIOA18_Handler
  .thumb_set GPIOA18_Handler,Default_Handler
  
  .weak  GPIOA19_Handler
  .thumb_set GPIOA19_Handler,Default_Handler
  
  .weak  GPIOA20_Handler
  .thumb_set GPIOA20_Handler,Default_Handler
  
  .weak  GPIOA21_Handler
  .thumb_set GPIOA21_Handler,Default_Handler
  
  .weak  GPIOA22_Handler
  .thumb_set GPIOA22_Handler,Default_Handler
  
  .weak  GPIOA23_Handler
  .thumb_set GPIOA23_Handler,Default_Handler
  
  .weak  GPIOA24_Handler
  .thumb_set GPIOA24_Handler,Default_Handler
  
  .weak  GPIOA25_Handler
  .thumb_set GPIOA25_Handler,Default_Handler
  
  .weak  GPIOA26_Handler
  .thumb_set GPIOA26_Handler,Default_Handler
  
  .weak  GPIOA27_Handler
  .thumb_set GPIOA27_Handler,Default_Handler
  
  .weak  GPIOA28_Handler
  .thumb_set GPIOA28_Handler,Default_Handler
  
  .weak  GPIOA29_Handler
  .thumb_set GPIOA29_Handler,Default_Handler
  
  .weak  GPIOA30_Handler
  .thumb_set GPIOA30_Handler,Default_Handler
  
  .weak  GPIOA31_Handler
  .thumb_set GPIOA31_Handler,Default_Handler
  
  .weak  GPIOB0_Handler
  .thumb_set GPIOB0_Handler,Default_Handler
  
  .weak  GPIOB1_Handler
  .thumb_set GPIOB1_Handler,Default_Handler
  
  .weak  GPIOB2_Handler
  .thumb_set GPIOB2_Handler,Default_Handler
  
  .weak  GPIOB3_Handler
  .thumb_set GPIOB3_Handler,Default_Handler
  
  .weak  GPIOB4_Handler
  .thumb_set GPIOB4_Handler,Default_Handler
  
  .weak  GPIOB5_Handler
  .thumb_set GPIOB5_Handler,Default_Handler
  
  .weak  GPIOB6_Handler
  .thumb_set GPIOB6_Handler,Default_Handler
  
  .weak  GPIOB7_Handler
  .thumb_set GPIOB7_Handler,Default_Handler
  
  .weak  GPIOB8_Handler
  .thumb_set GPIOB8_Handler,Default_Handler
  
  .weak  GPIOB9_Handler
  .thumb_set GPIOB9_Handler,Default_Handler
  
  .weak  GPIOB10_Handler
  .thumb_set GPIOB10_Handler,Default_Handler
  
  .weak  GPIOB11_Handler
  .thumb_set GPIOB11_Handler,Default_Handler
  
  .weak  GPIOB12_Handler
  .thumb_set GPIOB12_Handler,Default_Handler
  
  .weak  GPIOB13_Handler
  .thumb_set GPIOB13_Handler,Default_Handler
  
  .weak  GPIOB14_Handler
  .thumb_set GPIOB14_Handler,Default_Handler
  
  .weak  GPIOB15_Handler
  .thumb_set GPIOB15_Handler,Default_Handler
  
  .weak  GPIOB16_Handler
  .thumb_set GPIOB16_Handler,Default_Handler
  
  .weak  GPIOB17_Handler
  .thumb_set GPIOB17_Handler,Default_Handler
  
  .weak  GPIOB18_Handler
  .thumb_set GPIOB18_Handler,Default_Handler
  
  .weak  GPIOB19_Handler
  .thumb_set GPIOB19_Handler,Default_Handler
  
  .weak  GPIOB20_Handler
  .thumb_set GPIOB20_Handler,Default_Handler
  
  .weak  GPIOB21_Handler
  .thumb_set GPIOB21_Handler,Default_Handler
  
  .weak  GPIOB22_Handler
  .thumb_set GPIOB22_Handler,Default_Handler
  
  .weak  GPIOB23_Handler
  .thumb_set GPIOB23_Handler,Default_Handler
  
  .weak  GPIOB24_Handler
  .thumb_set GPIOB24_Handler,Default_Handler
  
  .weak  GPIOB25_Handler
  .thumb_set GPIOB25_Handler,Default_Handler
  
  .weak  GPIOB26_Handler
  .thumb_set GPIOB26_Handler,Default_Handler
  
  .weak  GPIOB27_Handler
  .thumb_set GPIOB27_Handler,Default_Handler
  
  .weak  GPIOB28_Handler
  .thumb_set GPIOB28_Handler,Default_Handler
  
  .weak  GPIOB29_Handler
  .thumb_set GPIOB29_Handler,Default_Handler
  
  .weak  GPIOB30_Handler
  .thumb_set GPIOB30_Handler,Default_Handler
  
  .weak  GPIOB31_Handler
  .thumb_set GPIOB31_Handler,Default_Handler
  
  .weak  GPIOC0_Handler
  .thumb_set GPIOC0_Handler,Default_Handler
  
  .weak  GPIOC1_Handler
  .thumb_set GPIOC1_Handler,Default_Handler
  
  .weak  GPIOC2_Handler
  .thumb_set GPIOC2_Handler,Default_Handler
  
  .weak  GPIOC3_Handler
  .thumb_set GPIOC3_Handler,Default_Handler
  
  .weak  GPIOC4_Handler
  .thumb_set GPIOC4_Handler,Default_Handler
  
  .weak  GPIOC5_Handler
  .thumb_set GPIOC5_Handler,Default_Handler
  
  .weak  GPIOC6_Handler
  .thumb_set GPIOC6_Handler,Default_Handler
  
  .weak  GPIOC7_Handler
  .thumb_set GPIOC7_Handler,Default_Handler
  
  .weak  GPIOC8_Handler
  .thumb_set GPIOC8_Handler,Default_Handler
  
  .weak  GPIOC9_Handler
  .thumb_set GPIOC9_Handler,Default_Handler
  
  .weak  GPIOC10_Handler
  .thumb_set GPIOC10_Handler,Default_Handler
  
  .weak  GPIOC11_Handler
  .thumb_set GPIOC11_Handler,Default_Handler
  
  .weak  GPIOC12_Handler
  .thumb_set GPIOC12_Handler,Default_Handler
  
  .weak  GPIOC13_Handler
  .thumb_set GPIOC13_Handler,Default_Handler
  
  .weak  GPIOC14_Handler
  .thumb_set GPIOC14_Handler,Default_Handler
  
  .weak  GPIOC15_Handler
  .thumb_set GPIOC15_Handler,Default_Handler

.end
