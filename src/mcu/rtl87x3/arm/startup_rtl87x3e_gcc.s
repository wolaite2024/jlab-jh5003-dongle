;/*******************************************************************************/
;/*     startup_rtl87x3d_gcc.s: Startup file for Realtek RTL87x3d device series */
;/*******************************************************************************/
//#include "mem_config.h"
/**
**===========================================================================
**  Definitions
**===========================================================================
*/
    .syntax unified
    .cpu cortex-m3
    .thumb

  .global    g_pfnVectors
  .global    Default_Handler
  .type    Default_Handler, %function 


.equ _estack,   0x2C0000 /*APP_GLOBAL_ADDR*/

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
  .word    Reset_Handler              /*Reset Handler*/
  .word    NMI_Handler               /*NMI Handler*/
  .word    HardFault_Handler         /*Hard Fault Handler*/
  .word    MemManage_Handler         /*MPU Fault Handler*/
  .word    BusFault_Handler          /*Bus Fault Handler*/
  .word    UsageFault_Handler        /*Usage Fault Handler*/
  .word    Default_Handler
  .word    Default_Handler
  .word    Default_Handler
  .word    Default_Handler
  .word    SVC_Handler               /*SVCall Handler*/
  .word    DebugMon_Handler          /*Debug Monitor Handler*/
  .word    Default_Handler
  .word    PendSV_Handler            /*PendSV Handler*/
  .word    SysTick_Handler           /*SysTick Handler*/

   /*External Interrupts*/
  .word    System_Handler
  .word    WDT_Handler
  .word    BTMAC_Handler
  .word    DSP_Handler
  .word    RXI300_Handler
  .word    SPI0_Handler
  .word    I2C0_Handler
  .word    ADC_Handler
  .word    SPORT0_TX_Handler
  .word    SPORT0_RX_Handler
  .word    TIM2_Handler
  .word    TIM3_Handler
  .word    TIM4_Handler
  .word    RTC_Handler
  .word    UART0_Handler
  .word    UART1_Handler
  .word    UART2_Handler
  .word    Peri_Handler
  .word    GPIO_A0_Handler
  .word    GPIO_A1_Handler
  .word    GPIO_A_2_7_Handler
  .word    GPIO_A_8_15_Handler
  .word    GPIO_A_16_23_Handler
  .word    GPIO_A_24_31_Handler
  .word    SPORT1_RX_Handler

  .word    SPORT1_TX_Handler
  .word    ADP_DET_Handler
  .word    VBAT_DET_Handler
  .word    GDMA0_Channel0_Handler
  .word    GDMA0_Channel1_Handler
  .word    GDMA0_Channel2_Handler
  .word    GDMA0_Channel3_Handler
  .word    GDMA0_Channel4_Handler
  .word    GDMA0_Channel5_Handler
  .word    GDMA0_Channel6_Handler
  .word    GDMA0_Channel7_Handler
  .word    GDMA0_Channel8_Handler
  .word    GPIO_B_0_7_Handler
  .word    GPIO_B_8_15_Handler
  .word    GPIO_B_16_23_Handler
  .word    GPIO_B_24_31_Handler
  .word    SPI1_Handler
  .word    SPI2_Handler
  .word    I2C1_Handler
  .word    I2C2_Handler
  .word    KeyScan_Handler
  .word    QDEC_Handler
  .word    USB_Handler
  .word    USB_ISOC_Handler
  .word    SPIC0_Handler
  .word    SPIC1_Handler
  .word    SPIC2_Handler
  .word    SPIC3_Handler
  .word    TIM5_Handler
  .word    TIM6_Handler
  .word    TIM7_Handler
  .word    ASRC0_Handler
  .word    ASRC1_Handler
  .word    I8080_Handler
  .word    ISO7816_Handler
  .word    SDIO0_Handler
  .word    SPORT2_RX_Handler
  .word    ANC_Handler
  .word    TOUCH_Handler

  .word    MFB_DET_L_Handler
  .word    PTA_Mailbox_Handler
  .word    Utmi_Suspend_N_Handler
  .word    IR_Handler
  .word    TRNG_Handler
  .word    PSRAMC_Handler
  .word    LPCOMP_Handler
  .word    TIM5_Handler
  .word    TIM6_Handler
  .word    TIM7_Handler
  .word    Default_Handler
  .word    LPCOMP_Handler
  .word    VBAT_DET_Handler
  .word    ADP_DET_Handler

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
 
  .weak  DSP_Handler
  .thumb_set DSP_Handler,Default_Handler
 
  .weak  RXI300_Handler
  .thumb_set RXI300_Handler,Default_Handler
 
  .weak  SPI0_Handler
  .thumb_set SPI0_Handler,Default_Handler
 
  .weak  I2C0_Handler
  .thumb_set I2C0_Handler,Default_Handler
 
  .weak  ADC_Handler
  .thumb_set ADC_Handler,Default_Handler
 
  .weak  SPORT0_TX_Handler
  .thumb_set SPORT0_TX_Handler,Default_Handler
 
  .weak  SPORT0_RX_Handler
  .thumb_set SPORT0_RX_Handler,Default_Handler
 
  .weak  TIM2_Handler
  .thumb_set TIM2_Handler,Default_Handler
 
  .weak  TIM3_Handler
  .thumb_set TIM3_Handler,Default_Handler
 
  .weak  TIM4_Handler
  .thumb_set TIM4_Handler,Default_Handler
 
  .weak  RTC_Handler
  .thumb_set RTC_Handler,Default_Handler
 
  .weak  UART0_Handler
  .thumb_set UART0_Handler,Default_Handler
 
  .weak  UART1_Handler
  .thumb_set UART1_Handler,Default_Handler
 
  .weak  UART2_Handler
  .thumb_set UART2_Handler,Default_Handler
 
  .weak  Peri_Handler
  .thumb_set Peri_Handler,Default_Handler
 
  .weak  GPIO_A0_Handler
  .thumb_set GPIO_A0_Handler,Default_Handler

  .weak  GPIO_A1_Handler
  .thumb_set GPIO_A1_Handler,Default_Handler
 
  .weak  GPIO_A_2_7_Handler
  .thumb_set GPIO_A_2_7_Handler,Default_Handler
 
  .weak  GPIO_A_8_15_Handler
  .thumb_set GPIO_A_8_15_Handler,Default_Handler
 
  .weak  GPIO_A_16_23_Handler
  .thumb_set GPIO_A_16_23_Handler,Default_Handler
 
  .weak  GPIO_A_24_31_Handler
  .thumb_set GPIO_A_24_31_Handler,Default_Handler
 
  .weak  SPORT1_RX_Handler
  .thumb_set SPORT1_RX_Handler,Default_Handler
 
  .weak  SPORT1_TX_Handler
  .thumb_set SPORT1_TX_Handler,Default_Handler
 
  .weak  ADP_DET_Handler
  .thumb_set ADP_DET_Handler,Default_Handler
 
  .weak  VBAT_DET_Handler
  .thumb_set VBAT_DET_Handler,Default_Handler
 
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
 
  .weak  GDMA0_Channel6_Handler
  .thumb_set GDMA0_Channel6_Handler,Default_Handler
 
  .weak  GDMA0_Channel7_Handler
  .thumb_set GDMA0_Channel7_Handler,Default_Handler
 
  .weak  GDMA0_Channel8_Handler
  .thumb_set GDMA0_Channel8_Handler,Default_Handler
 
  .weak  GPIO_B_0_7_Handler
  .thumb_set GPIO_B_0_7_Handler,Default_Handler
 
  .weak  GPIO_B_8_15_Handler
  .thumb_set GPIO_B_8_15_Handler,Default_Handler
 
  .weak  GPIO_B_16_23_Handler
  .thumb_set GPIO_B_16_23_Handler,Default_Handler
 
  .weak  GPIO_B_24_31_Handler
  .thumb_set GPIO_B_24_31_Handler,Default_Handler
 
  .weak  SPI1_Handler
  .thumb_set SPI1_Handler,Default_Handler
 
  .weak  SPI2_Handler
  .thumb_set SPI2_Handler,Default_Handler
 
  .weak  I2C1_Handler
  .thumb_set I2C1_Handler,Default_Handler
 
  .weak  I2C2_Handler
  .thumb_set I2C2_Handler,Default_Handler
 
  .weak  KeyScan_Handler
  .thumb_set KeyScan_Handler,Default_Handler
 
  .weak  QDEC_Handler
  .thumb_set QDEC_Handler,Default_Handler
 
  .weak  USB_Handler
  .thumb_set USB_Handler,Default_Handler
 
  .weak  USB_ISOC_Handler
  .thumb_set USB_ISOC_Handler,Default_Handler
 
  .weak  SPIC0_Handler
  .thumb_set SPIC0_Handler,Default_Handler
 
  .weak  SPIC1_Handler
  .thumb_set SPIC1_Handler,Default_Handler
 
  .weak  SPIC2_Handler
  .thumb_set SPIC2_Handler,Default_Handler
 
  .weak  SPIC3_Handler
  .thumb_set SPIC3_Handler,Default_Handler
 
  .weak  TIM5_Handler
  .thumb_set TIM5_Handler,Default_Handler
 
  .weak  TIM6_Handler
  .thumb_set TIM6_Handler,Default_Handler
 
  .weak  TIM7_Handler
  .thumb_set TIM7_Handler,Default_Handler
 
  .weak  ASRC0_Handler
  .thumb_set ASRC0_Handler,Default_Handler
 
  .weak  ASRC1_Handler
  .thumb_set ASRC1_Handler,Default_Handler
 
  .weak  I8080_Handler
  .thumb_set I8080_Handler,Default_Handler
 
  .weak  ISO7816_Handler
  .thumb_set ISO7816_Handler,Default_Handler
 
  .weak  SDIO0_Handler
  .thumb_set SDIO0_Handler,Default_Handler
 
  .weak  SPORT2_RX_Handler
  .thumb_set SPORT2_RX_Handler,Default_Handler
 
  .weak  ANC_Handler
  .thumb_set ANC_Handler,Default_Handler
 
  .weak  TOUCH_Handler
  .thumb_set TOUCH_Handler,Default_Handler
 
  .weak  MFB_DET_L_Handler
  .thumb_set MFB_DET_L_Handler,Default_Handler
 
  .weak  PTA_Mailbox_Handler
  .thumb_set PTA_Mailbox_Handler,Default_Handler
 
  .weak  Utmi_Suspend_N_Handler
  .thumb_set Utmi_Suspend_N_Handler,Default_Handler
 
  .weak  IR_Handler
  .thumb_set IR_Handler,Default_Handler
 
  .weak  TRNG_Handler
  .thumb_set TRNG_Handler,Default_Handler
 
  .weak  PSRAMC_Handler
  .thumb_set PSRAMC_Handler,Default_Handler
 
  .weak  LPCOMP_Handler
  .thumb_set LPCOMP_Handler,Default_Handler
 
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
    .ifdef __USE_C_LIBRARY_INIT   /* use c library init*/
    LDR    R0, =system_init
    BLX    R0
    LDR    R0, =__main
    BX     R0  
    .else
    /* write our init flow: copy rw, encrypted and clear zi
      data on rw, data on zi, buffer on rw, buffer on zi
    */
	
    /* copy data on rw */
    .extern __ram_rw_load_addr__;
    .extern __ram_rw_dst_addr__;
    .extern __ram_dataon_rw_length__;
    LDR R0, = __ram_rw_load_addr__
    LDR R1, = __ram_rw_dst_addr__
    LDR R2, =__ram_dataon_rw_length__
    
    CMP R2, #0
    BEQ end_of_copy_data_on_rw
copy_data_on_rw:
                LDR R3, [R0]
                ADDS R0, R0, #4
                STR R3, [R1]
                ADDS R1, R1, #4
                SUBS R2, R2, #4
                BNE copy_data_on_rw

end_of_copy_data_on_rw:

                /*clear data on zi*/
                .extern __ram_dataon_zi_start__;
                .extern __ram_dataon_zi_length__;
                LDR R0, =__ram_dataon_zi_start__
                LDR R1, =__ram_dataon_zi_length__
                CMP R1, #0
                BEQ end_of_clear_data_on_zi
clear_data_on_zi:
                STR R2, [R0]
                ADDS R0, R0, #4
                SUBS R1, R1, #4
                BNE clear_data_on_zi

end_of_clear_data_on_zi:

                /*copy buffer on ro (for ram function)*/
                .extern __ram_code_load_addr__
                .extern __ram_code_exe_addr__
                .extern __ram_code_length__
                LDR R0, =__ram_code_load_addr__
                LDR R1, =__ram_code_exe_addr__
                LDR R2, =__ram_code_length__         
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
                .extern system_init
                .extern main                
                LDR    R0, =system_init
                BLX    R0
                LDR    R0, =main
                BX     R0

      .endif  /*end of macro __USE_C_LIBRARY_INIT*/

.end 

