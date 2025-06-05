#include <stdint.h>
#include "tx_api.h"
#include "NUC100Series.h"
#include "usbd.h"
#include "usb_vendor.h"

/*--------------------------------------------------------------------------*/
STR_VCOM_LINE_CODING gLineCoding = {115200, 0, 0, 8};   /* Baud rate : 115200    */
/* Stop bit     */
/* parity       */
/* data bits    */
uint16_t gCtrlSignal = 0;     /* BIT0: DTR(Data Terminal Ready) , BIT1: RTS(Request To Send) */


void Vendor_ClassRequest(void)
{
    uint8_t buf[8];

    USBD_GetSetupPacket(buf);

    if(buf[0] & 0x80)    /* request data transfer direction */
    {
        // Device to host
        switch(buf[1])
        {
            case GET_LINE_CODE:
            {
                if(buf[4] == 0)    /* VCOM-1 */
                {
                    USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)), (uint8_t *)&gLineCoding, 7);
                }
                /* Data stage */
                USBD_SET_DATA1(EP0);
                USBD_SET_PAYLOAD_LEN(EP0, 7);
                /* Status stage */
                USBD_PrepareCtrlOut(0, 0);
                break;
            }
            default:
            {
                /* Setup error, stall the device */
                USBD_SetStall(EP0);
                USBD_SetStall(EP1);
                break;
            }
        }
    }
    else
    {
        // Host to device
        switch(buf[1])
        {
            case SET_CONTROL_LINE_STATE:
            {
                if(buf[4] == 0)    /* VCOM-1 */
                {
                    gCtrlSignal = buf[3];
                    gCtrlSignal = (gCtrlSignal << 8) | buf[2];
                    //printf("RTS=%d  DTR=%d\n", (gCtrlSignal0 >> 1) & 1, gCtrlSignal0 & 1);
                }

                /* Status stage */
                USBD_SET_DATA1(EP0);
                USBD_SET_PAYLOAD_LEN(EP0, 0);
                break;
            }
            case SET_LINE_CODE:
            {
                if(buf[4] == 0)  /* VCOM-1 */
                    USBD_PrepareCtrlOut((uint8_t *)&gLineCoding, 7);

                /* Status stage */
                USBD_SET_DATA1(EP0);
                USBD_SET_PAYLOAD_LEN(EP0, 0);

                break;
            }
            default:
            {
                // Stall
                /* Setup error, stall the device */
                USBD_SetStall(EP0);
                USBD_SetStall(EP1);
                break;
            }
        }
    }
}
