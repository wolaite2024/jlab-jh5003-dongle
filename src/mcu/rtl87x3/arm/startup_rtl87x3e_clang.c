#include <string.h>
#include "mem_config.h"
#include "rtl876x.h"
#include "trace.h"
#include "vector_table.h"

#define IMG_HEADER_SECTION      __attribute__((section(".image_entry")))

const IRQ_Fun RamVectorTable_app[158] __attribute__((section(".vectors_table"))) =
{
    Default_Handler,           // Top of Stack
    Reset_Handler,             // Reset Handler
    NMI_Handler,               // NMI Handler
    HardFault_Handler,         // Hard Fault Handler
    MemManage_Handler,         // MPU Fault Handler
    BusFault_Handler,          // Bus Fault Handler
    UsageFault_Handler,        // Usage Fault Handler
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    SVC_Handler,               // SVCall Handler
    DebugMon_Handler,          // Debug Monitor Handler
    Default_Handler,
    PendSV_Handler,            // PendSV Handler
    SysTick_Handler,           // SysTick Handler
    System_Handler,
    WDT_Handler,
    BTMAC_Handler,
    DSP_Handler,
    RXI300_Handler,
    SPI0_Handler,
    I2C0_Handler,
    ADC_Handler,
    SPORT0_TX_Handler,
    SPORT0_RX_Handler,
    TIM2_Handler,
    TIM3_Handler,
    TIM4_Handler,
    RTC_Handler,
    UART0_Handler,
    UART1_Handler,
    UART2_Handler,
    Peri_Handler,
    GPIO_A0_Handler,
    GPIO_A1_Handler,
    GPIO_A_2_7_Handler,
    GPIO_A_8_15_Handler,
    GPIO_A_16_23_Handler,
    GPIO_A_24_31_Handler,
    SPORT1_RX_Handler,
    SPORT1_TX_Handler,
    ADP_DET_Handler,
    VBAT_DET_Handler,
    GDMA0_Channel0_Handler,
    GDMA0_Channel1_Handler,
    GDMA0_Channel2_Handler,
    GDMA0_Channel3_Handler,
    GDMA0_Channel4_Handler,
    GDMA0_Channel5_Handler,
    GDMA0_Channel6_Handler,
    GDMA0_Channel7_Handler,
    GDMA0_Channel8_Handler,
    GPIO_B_0_7_Handler,
    GPIO_B_8_15_Handler,
    GPIO_B_16_23_Handler,
    GPIO_B_24_31_Handler,
    SPI1_Handler,
    SPI2_Handler,
    I2C1_Handler,
    I2C2_Handler,
    KeyScan_Handler,
    QDEC_Handler,
    USB_Handler,
    USB_ISOC_Handler,
    SPIC0_Handler,
    SPIC1_Handler,
    SPIC2_Handler,
    SPIC3_Handler,
    TIM5_Handler,
    TIM6_Handler,
    TIM7_Handler,
    ASRC0_Handler,
    ASRC1_Handler,
    I8080_Handler,
    ISO7816_Handler,
    SDIO0_Handler,
    SPORT2_RX_Handler,
    ANC_Handler,
    TOUCH_Handler,
    MFB_DET_L_Handler,
    PTA_Mailbox_Handler,
    Utmi_Suspend_N_Handler,
    IR_Handler,
    TRNG_Handler,
    PSRAMC_Handler,
    LPCOMP_Handler,
    TIM5_Handler,
    TIM6_Handler,
    TIM7_Handler,
    Default_Handler,
    LPCOMP_Handler,
    VBAT_DET_Handler,
    ADP_DET_Handler,
    ADP_IN_DET_Handler,
    ADP_OUT_DET_Handler,
    GPIOA2_Handler,
    GPIOA3_Handler,
    GPIOA4_Handler,
    GPIOA5_Handler,
    GPIOA6_Handler,
    GPIOA7_Handler,
    GPIOA8_Handler,
    GPIOA9_Handler,
    GPIOA10_Handler,
    GPIOA11_Handler,
    GPIOA12_Handler,
    GPIOA13_Handler,
    GPIOA14_Handler,
    GPIOA15_Handler,
    GPIOA16_Handler,
    GPIOA17_Handler,
    GPIOA18_Handler,
    GPIOA19_Handler,
    GPIOA20_Handler,
    GPIOA21_Handler,
    GPIOA22_Handler,
    GPIOA23_Handler,
    GPIOA24_Handler,
    GPIOA25_Handler,
    GPIOA26_Handler,
    GPIOA27_Handler,
    GPIOA28_Handler,
    GPIOA29_Handler,
    GPIOA30_Handler,
    GPIOA31_Handler,
    GPIOB0_Handler,
    GPIOB1_Handler,
    GPIOB2_Handler,
    GPIOB3_Handler,
    GPIOB4_Handler,
    GPIOB5_Handler,
    GPIOB6_Handler,
    GPIOB7_Handler,
    GPIOB8_Handler,
    GPIOB9_Handler,
    GPIOB10_Handler,
    GPIOB11_Handler,
    GPIOB12_Handler,
    GPIOB13_Handler,
    GPIOB14_Handler,
    GPIOB15_Handler,
    GPIOB16_Handler,
    GPIOB17_Handler,
    GPIOB18_Handler,
    GPIOB19_Handler,
    GPIOB20_Handler,
    GPIOB21_Handler,
    GPIOB22_Handler,
    GPIOB23_Handler,
    GPIOB24_Handler,
    GPIOB25_Handler,
    GPIOB26_Handler,
    GPIOB27_Handler,
    GPIOB28_Handler,
    GPIOB29_Handler,
    GPIOB30_Handler,
    GPIOB31_Handler,
};

__attribute__((naked)) void __user_setup_stackheap(void)
{
    __asm volatile(
        "BX      lr                        \n"
    );

}

extern void system_init(void);
extern int main(void);

IMG_HEADER_SECTION void Reset_Handler(void)
{
    /* copy data on rw*/
    extern uint32_t Load$$RAM_GLOBAL$$RW$$Base;
    extern uint32_t Image$$RAM_GLOBAL$$RW$$Base;
    extern uint32_t Image$$RAM_GLOBAL$$RW$$Length;
    memcpy((void *) &Image$$RAM_GLOBAL$$RW$$Base, (void *) &Load$$RAM_GLOBAL$$RW$$Base,
           (uint32_t)&Image$$RAM_GLOBAL$$RW$$Length);

    /* clear data on zi */
    extern uint32_t Image$$RAM_GLOBAL$$ZI$$Base;
    extern uint32_t Image$$RAM_GLOBAL$$ZI$$Length;
    memset((uint8_t *)&Image$$RAM_GLOBAL$$ZI$$Base, 0, (uint32_t)&Image$$RAM_GLOBAL$$ZI$$Length);

    /* copy buffer on rw */
    extern uint32_t Load$$RAM_TEXT$$RW$$Base;
    extern uint32_t Image$$RAM_TEXT$$RW$$Base;
    extern uint32_t Image$$RAM_TEXT$$RW$$Length;
    memcpy((void *) &Image$$RAM_TEXT$$RW$$Base, (void *) &Load$$RAM_TEXT$$RW$$Base,
           (uint32_t)&Image$$RAM_TEXT$$RW$$Length);

    /* copy buffer on ro (for ram function) */
    extern uint32_t Load$$RAM_TEXT$$RO$$Base;
    extern uint32_t Image$$RAM_TEXT$$RO$$Base;
    extern uint32_t Image$$RAM_TEXT$$RO$$Length;
    memcpy((void *) &Image$$RAM_TEXT$$RO$$Base, (void *) &Load$$RAM_TEXT$$RO$$Base,
           (uint32_t)&Image$$RAM_TEXT$$RO$$Length);

    /* clear buffer on zi */
    extern uint32_t Image$$RAM_TEXT$$ZI$$Base;
    extern uint32_t Image$$RAM_TEXT$$ZI$$Length;
    memset((uint8_t *)&Image$$RAM_TEXT$$ZI$$Base, 0, (uint32_t)&Image$$RAM_TEXT$$ZI$$Length);

    system_init();
    main();
}

/*----------------------------------------------------------------------------
  Default Handler for Exceptions / Interrupts
 *----------------------------------------------------------------------------*/
void Default_Handler(void)
{
    DBG_DIRECT("Error! Please implement your ISR Handler for IRQ number %d!", __get_IPSR());
    while (1);
}
