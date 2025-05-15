

#ifndef __IO_CONFIG_H__
#define __IO_CONFIG_H__

#include "NUC100Series.h"

// SWD
#define SWD_DAT_IO  PC10
#define SWD_DAT_GRP PC
#define SWD_DAT_BIT 10

#define SWD_CLK_IO  PC9
#define SWD_CLK_GRP PC
#define SWD_CLK_BIT 9

#define DBG_RST_IO  PC8
#define DBG_RST_GRP PC
#define DBG_RST_BIT 8

// LED
#define LED_ICE_IO  PB4
#define LED_ICE_GRP PB
#define LED_ICE_BIT 4

#define LED_ISP_IO  PB5
#define LED_ISP_GRP PB
#define LED_ISP_BIT 5

#define LED_RED_IO  PB6
#define LED_RED_GRP PB
#define LED_RED_BIT 6

#define LED_GRE_IO  PB7
#define LED_GRE_GRP PB
#define LED_GRE_BIT 7

// Other
#define OFF_BTN_IO  PB14
#define OFF_BTN_GRP PB
#define OFF_BTN_BIT 14

#endif
