
#include <stdint.h>
#include "tx_api.h"
#include "NUC100Series.h"
#include "usbd.h"
#include "usb_vendor.h"
#include "get_serial.h"
#include "DAP_config.h"
#include "DAP.h"
#include "IO_Config.h"


// 48MHz for USB
#define PLLCON_SETTING  CLK_PLLCON_48MHz_HXT
#define PLL_CLOCK	(48000000U)

#define MEMORY_POOL_SIZE                 (8 * 1024)
static __ALIGNED(8) uint8_t gu8MemoryPool[MEMORY_POOL_SIZE];

#define THREAD_USB_PRI_NUM  (1)
#define THREAD_DAP_PRI_NUM  (THREAD_USB_PRI_NUM + 1)
#define THREAD_UART_PRI_NUM (THREAD_USB_PRI_NUM + 2)

#define THREAD_USB_STACK_SIZE   (1024)

TX_THREAD   threadUSB;

__STATIC_INLINE void init_io(void) {
    // LEDs and Button
    PB->PMD = (PB->PMD & (~(GPIO_PMD_PMD4_Msk | GPIO_PMD_PMD4_Msk |
                            GPIO_PMD_PMD6_Msk | GPIO_PMD_PMD7_Msk |
                            GPIO_PMD_PMD14_Msk))) |
                         ((GPIO_PMD_OUTPUT << GPIO_PMD_PMD4_Pos) |
                          (GPIO_PMD_OUTPUT << GPIO_PMD_PMD5_Pos) |
                          (GPIO_PMD_OUTPUT << GPIO_PMD_PMD6_Pos) |
                          (GPIO_PMD_OUTPUT << GPIO_PMD_PMD7_Pos) |
                          (GPIO_PMD_INPUT << GPIO_PMD_PMD14_Pos));
    PB4 = 1;
    PB5 = 1; // LED_CONNECTED_OUT
    PB6 = 1;
    PB7 = 1; // LED_RUNNING_OUT
}

__STATIC_INLINE void SYS_Init(void)
{
    uint32_t u32TimeOutCnt;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable Internal RC 22.1184 MHz clock */
    CLK->PWRCON |= CLK_PWRCON_OSC22M_EN_Msk;

    /* Waiting for Internal RC clock ready */
    u32TimeOutCnt = __HIRC; /* 1 second time-out */
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_OSC22M_STB_Msk))
        if(--u32TimeOutCnt == 0) return;

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLK_S_Msk)) | CLK_CLKSEL0_HCLK_S_HIRC;
    CLK->CLKDIV = (CLK->CLKDIV & (~CLK_CLKDIV_HCLK_N_Msk)) | CLK_CLKDIV_HCLK(1);

    /* Set PLL to power down mode and PLL_STB bit in CLKSTATUS register will be cleared by hardware */
    CLK->PLLCON |= CLK_PLLCON_PD_Msk;

    /* Enable external XTAL 12 MHz clock */
    CLK->PWRCON |= CLK_PWRCON_XTL12M_EN_Msk;

    /* Waiting for external XTAL clock ready */
    u32TimeOutCnt = __HIRC; /* 1 second time-out */
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_XTL12M_STB_Msk))
        if(--u32TimeOutCnt == 0) return;

    /* Set core clock as PLL_CLOCK from PLL */
    CLK->PLLCON = PLLCON_SETTING;

    /* Waiting for PLL ready */
    u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_PLL_STB_Msk))
        if(--u32TimeOutCnt == 0) return;

    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLK_S_Msk)) | CLK_CLKSEL0_HCLK_S_PLL;

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CycylesPerUs automatically. */
    //SystemCoreClockUpdate();
    PllClock        = PLL_CLOCK;            // PLL
    SystemCoreClock = PLL_CLOCK / 1;        // HCLK
    CyclesPerUs     = PLL_CLOCK / 1000000;  // For CLK_SysTickDelay()

    /* Enable UART0 and USBD module clock */
    CLK->APBCLK |= CLK_APBCLK_USBD_EN_Msk | CLK_APBCLK_UART0_EN_Msk;

    /* Select module clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_HXT, CLK_CLKDIV_UART(1));
    CLK_SetModuleClock(USBD_MODULE, 0, CLK_CLKDIV_USB(1));


   /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set GPB multi-function pins for UART0 RXD and TXD, and Clock Output */
    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk | SYS_GPB_MFP_PB12_Msk);
    SYS->GPB_MFP |= (SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD | SYS_GPB_MFP_PB12_CLKO);
    SYS->ALT_MFP &= ~SYS_ALT_MFP_PB12_Msk;
    SYS->ALT_MFP |= SYS_ALT_MFP_PB12_CLKO;

    /* Enable CLKO(PB.12) for monitor HCLK. CLKO = HCLK/8 Hz*/
    // EnableCLKO((2 << CLK_CLKSEL2_FRQDIV_S_Pos), 2);

    init_io();

    NVIC_SetPriority(USBD_IRQn, 0);
    NVIC_SetPriority(SysTick_IRQn, 1);
}

__STATIC_INLINE void UART0_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset IP */
    SYS->IPRSTC2 |=  SYS_IPRSTC2_UART0_RST_Msk;
    SYS->IPRSTC2 &= ~SYS_IPRSTC2_UART0_RST_Msk;

    /* Configure UART0 and set UART0 Baudrate */
    UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 115200);
    UART0->LCR = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;

    /* Enable UART Interrupt */
    UART0->IER = UART_IER_TOUT_IEN_Msk | UART_IER_RDA_IEN_Msk;
}





static void usb_thread(ULONG thread_input)
{
    (void)thread_input;

    USBD_CustomerStart();
    NVIC_EnableIRQ(USBD_IRQn);
    do {
    } while (1);
}

/* Define what the initial system looks like.  */
void    tx_application_define(void *first_unused_memory)
{
    TX_BYTE_POOL    byte_pool;
    CHAR    *pointer = TX_NULL;

    DAP_Setup();
    tx_byte_pool_create(&byte_pool, "pool", gu8MemoryPool, MEMORY_POOL_SIZE);
    tx_byte_allocate(&byte_pool, (VOID **) &pointer, THREAD_USB_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&threadUSB, "ThreadUSB",
        usb_thread,
        0,
        pointer,
        THREAD_USB_STACK_SIZE,
        THREAD_USB_PRI_NUM,
        THREAD_USB_PRI_NUM,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);
}

int main(void) {
    /* Unlock protected registers */
    SYS_UnlockReg();
    SYS_Init();
    UART0_Init();
    USBD_Open(&gsInfo, Vendor_ClassRequest, NULL);
    usb_serial_init(); // prepare USB Serial
    tx_kernel_enter();
    return 0;
}

__WEAK
uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp)
{
    uint32_t *sp;
    /* It is casued by hardfault. Just process the hard fault */
    /* TODO: Implement your hardfault handle code here */

#if defined (__ARM_FEATURE_CMSE) &&  (__ARM_FEATURE_CMSE == 3)
    /* Check the used stack */
    if(lr & 0x40UL)
    {
#endif
        /* Secure stack used */
        if(lr & 4UL)
        {
            sp = (uint32_t *)psp;
        }
        else
        {
            sp = (uint32_t *)msp;
        }

#if defined (__ARM_FEATURE_CMSE) &&  (__ARM_FEATURE_CMSE == 3)
    }
    else
    {
        /* Non-secure stack used */
        if(lr & 4)
            sp = (uint32_t *)__TZ_get_PSP_NS();
        else
            sp = (uint32_t *)__TZ_get_MSP_NS();

    }
#endif

    //printf("  HardFault!\n\n");

    /*
    printf("  HardFault!\n\n");
    printf("r0  = 0x%x\n", sp[0]);
    printf("r1  = 0x%x\n", sp[1]);
    printf("r2  = 0x%x\n", sp[2]);
    printf("r3  = 0x%x\n", sp[3]);
    printf("r12 = 0x%x\n", sp[4]);
    printf("lr  = 0x%x\n", sp[5]);
    printf("pc  = 0x%x\n", sp[6]);
    printf("psr = 0x%x\n", sp[7]);
    */

    /* Or *sp to remove compiler warning */
    while(1U|*sp){}

    return lr;
}
