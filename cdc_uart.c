
#include <stdint.h>
#include "NUC100Series.h"
#include "tusb.h"

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* line_coding)
{
  uint32_t lcr = 0;

  // Disabled Interrupt
  UART0->IER &= ~(UART_IER_TOUT_IEN_Msk | UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_ISR_BUF_ERR_INT_Msk);

  /* Reset IP */
  SYS->IPRSTC2 |=  SYS_IPRSTC2_UART0_RST_Msk;
  SYS->IPRSTC2 &= ~SYS_IPRSTC2_UART0_RST_Msk;
  tud_cdc_write_clear();
  tud_cdc_read_flush();

  // baudrate
  UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, line_coding->bit_rate);

  // parity
  switch (line_coding->parity) {
  case CDC_LINE_CODING_PARITY_ODD:
    lcr = UART_PARITY_ODD;
    break;
  case CDC_LINE_CODING_PARITY_EVEN:
    lcr = UART_PARITY_EVEN;
    break;
  case CDC_LINE_CODING_PARITY_MARK:
    lcr = UART_PARITY_MARK;
    break;
  case CDC_LINE_CODING_PARITY_SPACE:
    lcr = UART_PARITY_SPACE;
    break;
  default:
    /* fallthrough */
  case CDC_LINE_CODING_PARITY_NONE:
    lcr = UART_PARITY_NONE;
    break;
  }

  // data length bits
  switch (line_coding->data_bits) {
  case 5:
    lcr |= UART_WORD_LEN_5;
    break;
  case 6:
    lcr |= UART_WORD_LEN_6;
    break;
  case 7:
    lcr |= UART_WORD_LEN_7;
    break;
  default:
    /* fallthrough */
  case 8:
    lcr |= UART_WORD_LEN_8;
    break;
  }

  // stop bits
  switch (line_coding->stop_bits) {
  case CDC_LINE_CONDING_STOP_BITS_1_5:
    lcr |= UART_STOP_BIT_1_5;
    break;
  case CDC_LINE_CONDING_STOP_BITS_2:
    lcr |= UART_STOP_BIT_2;
    break;
  default:
    /* fallthrough */
  case CDC_LINE_CONDING_STOP_BITS_1:
    lcr |= UART_STOP_BIT_1;
    break;
  }
  UART0->LCR = lcr;
  UART0->IER |= UART_IER_TOUT_IEN_Msk | UART_IER_RDA_IEN_Msk;
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{

}
