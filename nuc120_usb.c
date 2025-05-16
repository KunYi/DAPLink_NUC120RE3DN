
#include <stdint.h>
#include "NUC100Series.h"

#ifndef U32
#define U32 uint32_t
#endif

/**
 * @defgroup USBD_Endpoint_Definitions USB Device Endpoint Definitions
 * @brief Definitions for USB Full-Speed endpoints and buffer settings.
 * @{
 */

/**
 * @brief Number of USB endpoints used.
 */
#define USBD_EP_NUM     (6)

/**
 * @brief Endpoint 0 - Default control endpoint.
 */
#define EP0     0

/**
 * @brief Endpoint 1.
 */
#define EP1     1

/**
 * @brief Endpoint 2.
 */
#define EP2     2

/**
 * @brief Endpoint 3.
 */
#define EP3     3

/**
 * @brief Endpoint 4.
 */
#define EP4     4

/**
 * @brief Endpoint 5.
 */
#define EP5     5

/**
 * @brief Base address of the USB buffer memory.
 *
 * The buffer area starts 0x100 bytes after the USBD base address.
 */
#define USBD_BUF_BASE   (USBD_BASE + 0x100UL)

/**
 * @brief Total length of USB buffer memory.
 */
#define USBD_BUF_LEN    (512)

/**
 * @brief Maximum packet size for Control, Interrupt, and Bulk endpoints.
 *
 * - Control, Interrupt, Bulk: 64 bytes
 * - Isochronous: up to 1023 bytes (not defined here)
 *
 * @note For Isochronous endpoints, use up to 1023 bytes as per USB FS spec.
 */
#define MAX_PACKET_SIZE (64)

/**
 * @brief Setup packet buffer base address.
 */
#define SETUP_BUF_BASE              (0)

/**
 * @brief Length of the setup packet buffer (always 8 bytes).
 */
#define SETUP_BUF_LEN               (8)

/**
 * @brief EP0 buffer base address.
 */
#define EP0_BUF_BASE                (SETUP_BUF_BASE + SETUP_BUF_LEN)

/**
 * @brief EP0 buffer length.
 */
#define EP0_BUF_LEN                 (MAX_PACKET_SIZE)

/**
 * @brief EP1 buffer base address.
 */
#define EP1_BUF_BASE                (EP0_BUF_BASE + EP0_BUF_LEN)

/**
 * @brief EP1 buffer length.
 */
#define EP1_BUF_LEN                 (MAX_PACKET_SIZE)

/**
 * @brief EP2 buffer base address.
 */
#define EP2_BUF_BASE                (EP1_BUF_BASE + EP1_BUF_LEN)

/**
 * @brief EP2 buffer length.
 */
#define EP2_BUF_LEN                 (MAX_PACKET_SIZE)

/**
 * @brief EP3 buffer base address.
 */
#define EP3_BUF_BASE                (EP2_BUF_BASE + EP2_BUF_LEN)

/**
 * @brief EP3 buffer length.
 */
#define EP3_BUF_LEN                 (MAX_PACKET_SIZE)

/**
 * @brief EP4 buffer base address.
 */
#define EP4_BUF_BASE                (EP3_BUF_BASE + EP3_BUF_LEN)

/**
 * @brief EP5 buffer length.
 */
#define EP4_BUF_LEN                 (MAX_PACKET_SIZE)

/**
 * @brief EP5 buffer base address.
 */
#define EP5_BUF_BASE                (EP4_BUF_BASE + EP4_BUF_LEN)

/**
 * @brief EP5 buffer length.
 */
#define EP5_BUF_LEN                 (MAX_PACKET_SIZE)

/**
 * @brief Macro to set the buffer address for a given endpoint.
 *
 * @param[in] ep       Endpoint number (0 to 5)
 * @param[in] offset   Offset from USBD_BUF_BASE (in bytes)
 *
 * @note This writes to the BUFSEG register for the selected endpoint.
 */
#define USBD_SET_EP_BUF_ADDR(ep, offset)    (*((__IO U32 *) ((U32)&USBD->EP[0].BUFSEG + (U32)((ep) << 4))) = (offset))

/** @} */  // end of USBD_Endpoint_Definitions

#define USBD_SET_ADDR(addr)         ((U32)USBD->FADDR = (addr))
#define USBD_GET_ADDR()             ((U32)(USBD->FADDR))
#define USBD_SET_SE0()              ((U32)(USBD->DRVSE0 |= USBD_DRVSE0_DRVSE0_Msk))
#define USBD_CLR_SE0()              ((U32)(USBD->DRVSE0 &= ~USBD_DRVSE0_DRVSE0_Msk))
#define USBD_SET_DP_PULLUP()        ((U32)(USBD->ATTR |= USBD_ATTR_DPPU_EN_Msk))
#define USBD_CLR_DP_PULLUP()        ((U32)(USBD->ATTR |= USBD_ATTR_DPPU_EN_Msk))


#define USBD_CLR_INT_FLAG(flag)     ((U32)USBD->INTSTS = (flag))
#define USBD_IS_ATTACHED()          ((U32)(USBD->FLDET & USBD_FLDET_FLDET_Msk))
#define USBD_ENABLE_INT(intr)       ((U32)USBD->INTEN |= (intr))

#define USBD_ENABLE_USB()           ((U32)(USBD->ATTR |= (USBD_ATTR_USB_EN_Msk|USBD_ATTR_PHY_EN_Msk)))
#define USBD_DISABLE_USB()          ((U32)(USBD->ATTR &= ~USBD_ATTR_USB_EN_Msk))

/**
 * @brief Initialize the USB device hardware.
 *
 * This function initializes the USB controller and PHY transceiver,
 * sets the default USB address to 0, and prepares the hardware for USB enumeration.
 *
 * @return None
 *
 * @note This function should be called once during system initialization
 *       before the USB device stack starts.
 *
 * @details
 * - Enables USB controller and PHY.
 * - Enables USB D+ pull-up to signal device presence to the host.
 * - Forces SE0 (Single-Ended Zero) to indicate bus reset.
 * - Sets the initial USB device address to 0.
 */
void USBD_Init(void)
{
    /* Initial USB engine */
    USBD->ATTR = USBD_ATTR_BYTEM_Msk |      // The size of the transfer from CPU to USB SRAM can be Byte only.
                 USBD_ATTR_PWRDN_Msk |      // Turn-on related circuit of PHY transceiver.
                 USBD_ATTR_DPPU_EN_Msk |    // Pull-up resistor in USB_DP bus Active.
                 USBD_ATTR_USB_EN_Msk |     // USB Controller Enabled.
                 (1ul << 6) |               // Reserved.
                 USBD_ATTR_PHY_EN_Msk;      // PHY transceiver function Enabled.

    /* Force SE0 */
    USBD_SET_SE0();

    /* Address 0 */
    USBD_SET_ADDR(0);

    USBD->STBUFSEG = SETUP_BUF_BASE;

    USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);
    USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);
    USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);
    USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);
    USBD_SET_EP_BUF_ADDR(EP4, EP4_BUF_BASE);
    USBD_SET_EP_BUF_ADDR(EP5, EP5_BUF_BASE);

    USBD_CLR_SE0();


    USBD_CLR_INT_FLAG(
        USBD_INTSTS_WAKEUP_STS_Msk |
        USBD_INTSTS_FLDET_STS_Msk |
        USBD_INTSTS_USB_STS_Msk |
        USBD_INTSTS_BUS_STS_Msk
    );

    USBD_ENABLE_INT(
        USBD_INTEN_WAKEUP_EN_Msk |
        USBD_INTEN_WAKEUP_IE_Msk |
        USBD_INTEN_FLDET_IE_Msk |
        USBD_INTEN_USB_IE_Msk |
        USBD_INTEN_BUS_IE_Msk |
    );
}

/**
 * @brief Connect or disconnect the USB device from the bus.
 *
 * This function is called by the USB device stack to simulate
 * a physical USB connect or disconnect by controlling the USB D+ pull-up
 * resistor or forcing SE0 (Single-Ended Zero).
 *
 * @param[in] con  Non-zero to connect, zero to disconnect.
 *
 * @return None
 *
 * @note Pulling D+ high signals device presence to the host (Full-Speed).
 *       Releasing pull-up or forcing SE0 signals disconnect.
 */
void USBD_Connect(BOOL con)
{
    if (con)
    {
        USBD_SET_DP_PULLUP();
    }
    else
    {
        USBD_CLR_DP_PULLUP();
    }
}

void USBD_Reset(void)
{
    /* Address 0 */
    USBD_SET_ADDR(0);
}

void USBD_Suspend(void)
{

}

void USBD_Resume(void)
{

}

void USBD_WakeUp(void)
{

}

void USBD_WakeUpCfg(BOOL cfg)
{

}

/**
 * @brief Set the USB device address.
 *
 * This function sets the USB device address during the status stage of a
 * control transfer, as required by the USB specification. If called during
 * the setup stage, the function exits without setting the address.
 *
 * @param[in] adr     The USB device address to assign (0~127).
 * @param[in] setup   Non-zero if called during the setup stage; zero if during the status stage.
 *
 * @return None
 *
 * @note According to the USB 2.0 specification, the device must wait until the
 *       status stage of the Set Address control transfer before changing its address.
 */
void USBD_SetAddress(U32 adr, U32 setup)
{
    if (setup) {
        return;
    }

    USBD_SET_ADDR(adr);
}

void USBD_Configure(BOOL cfg)
{

}

void USBD_ConfigEP(USB_ENDPOINT_DESCRIPTOR *pEPD)
{

}

void USBD_DirCtrlEP(U32 dir)
{

}

void USBD_EnableEP(U32 EPNum)
{

}

void USBD_DisableEP(U32 EPNum)
{

}

void USBD_ResetEP(U32 EPNum)
{

}

void USBD_SetStallEP(U32 EPNum)
{

}

void USBD_ClrStallEP(U32 EPNum)
{

}

void USBD_ClearEPBuf(U32 EPNum)
{

}

U32 USBD_ReadEP(U32 EPNum, U8 *pData, U32 cnt)
{

}

U32 USBD_WriteEP(U32 EPNum, U8 *pData, U32 cnt)
{

}

U32 USBD_GetFrame(void)
{

}

U32 USBD_GetError(void)
{

}

#if 0
void USBD_SignalHandler(void)
{

}
#endif

__STATIC_FORCEINLINE USBD_ISR_Handler(void)
{
    U32 intSts = USBD->INTSTS;
    U32 busState = USBD->ATTR & 0xf;

    if (intSts & USBD_INTSTS_FLDET_STS_Msk) // There is attached/detached event in the USB bus
    {
        USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET_STS_Msk);
        if (USBD_IS_ATTACHED())
        {
            // USB Plug in
            USBD_ENABLE_USB();
        }
        else
        {
            // USB Plug out
            USBD_DISABLE_USB();
        }
    }

    if (intSts & USBD_INTSTS_BUS_STS_Msk) // The BUS event means that there is one of the suspense or the resume function
    {
        USBD_CLR_INT_FLAG(USBD_INTSTS_BUS_STS_Msk);
        if (busState & USBD_ATTR_USBRST_Msk)
        {

        }

        if (busState & USBD_ATTR_SUSPEND_Msk)
        {

        }

        if (busState & USBD_ATTR_RESUME_Msk)
        {

        }

        if (busState & USBD_ATTR_TIMEOUT_Msk)
        {

        }
    }

    if (intSts & USBD_INTSTS_USB_STS_Msk) // The USB event includes the Setup Token, IN Token, OUT ACK, ISO IN, or ISO OUT events in the bus
    {
        USBD_CLR_INT_FLAG(USBD_INTSTS_USB_STS_Msk);
    }

    if (intSts & USBD_INTSTS_WAKEUP_STS_Msk) // Wake-up event occurred
    {
        USBD_CLR_INT_FLAG(USBD_INTSTS_WAKEUP_STS_Msk);
    }
}

void USBD_Handler(void)
{
    USBD_ISR_Handler();
    NVIC_EnableIRQ(USBD_IRQn);
}

void USBD_IRQHandler(void)
{
    NVIC_DisableIRQ(USBD_IRQn);
    USBD_SignalHandler();
}
