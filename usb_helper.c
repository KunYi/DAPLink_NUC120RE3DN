
#include <stdint.h>
#include "NUC100Series.h"
#include "DAP.h"
#include "tusb.h"

extern uint8_t const desc_ms_os_20[]; // defined in usb_descriptors.c

#if (BOARD_DEBUG_PROTOCOL == PROTO_DAP_V2)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
  // nothing to with DATA & ACK stage
  if (stage != CONTROL_STAGE_SETUP) return true;

  switch (request->bmRequestType_bit.type)
  {
    case TUSB_REQ_TYPE_VENDOR:
      switch (request->bRequest)
      {
        case 1:
          if ( request->wIndex == 7 )
          {
            // Get Microsoft OS 2.0 compatible descriptor
            uint16_t total_len;
            memcpy(&total_len, desc_ms_os_20+8, 2);

            return tud_control_xfer(rhport, request, (void*) desc_ms_os_20, total_len);
          }else
          {
            return false;
          }

        default: break;
      }
    break;
    default: break;
  }

  // stall unknown request
  return false;
}
#endif

// USB interrupt handler to invoke tinyusb
void USBD_IRQHandler(void)
{
  tud_int_handler(0);
}
