#pragma once
#ifndef __MAIN_H__
#define __MAIN_H__

// 48MHz for USB
#define PLLCON_SETTING  CLK_PLLCON_48MHz_HXT
#define PLL_CLOCK	(48000000U)

#define INTERRUPT_PRIORITY_SYSTICK (0)
#define INTERRUPT_PRIORITY_USB     (INTERRUPT_PRIORITY_SYSTICK + 2)

#define USB_EN_INTR()     NVIC_EnableIRQ(USBD_IRQn)
#define USB_DIS_INTR()    NVIC_DisableIRQ(USBD_IRQn)


#endif
