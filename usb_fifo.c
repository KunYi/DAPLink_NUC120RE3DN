#include <stdint.h>
#include <stdbool.h>
#include "tx_api.h"


#define SIZE_EPBUFF	   (64)
#define FIFO_DEPTH    	(8)

typedef struct _usb_epbuf_ {
    uint8_t buffer[FIFO_DEPTH][SIZE_EPBUFF];
    uint16_t size[FIFO_DEPTH];
    volatile uint8_t write_index;
    volatile uint8_t read_index;
    volatile uint8_t count;
} usb_ep_fifo_t;

static usb_ep_fifo_t usb_ep_fifo = { 0 };


#define USB_EVENT_BULK_OUT 0x01
#define USB_EVENT_BULK_IN  0x02
static TX_EVENT_FLAGS_GROUP usb_event_flags;


void usb_fifo_init(void) {
  usb_ep_fifo.write_index = 0;
  usb_ep_fifo.read_index = 0;
  usb_ep_fifo.count = 0;
  tx_event_flags_create(&usb_event_flags, "USB Event Flags");
}

static inline bool usb_fifo_is_full(const usb_ep_fifo_t *fifo) {
  return (fifo->count == FIFO_DEPTH);
}

static inline bool usb_fifo_is_empty(const usb_ep_fifo_t *fifo) {
  return (fifo->count == 0);
}

bool usb_fifo_write(const uint8_t *data, size_t len) {
  if (len > SIZE_EPBUFF) return false;

  if (usb_fifo_is_full(&usb_ep_fifo))
    return false;

  for (size_t i = 0; i < len; i++) {
    usb_ep_fifo.buffer[usb_ep_fifo.write_index][i] = data[i];
  }

  for (size_t i = len; i < SIZE_EPBUFF; i++) {
    usb_ep_fifo.buffer[usb_ep_fifo.write_index][i] = 0;
  }

  usb_ep_fifo.size[usb_ep_fifo.write_index] = len;
  usb_ep_fifo.write_index = (usb_ep_fifo.write_index + 1) % FIFO_DEPTH;
  usb_ep_fifo.count++;

  return true;
}

bool usb_fifo_read(uint8_t *data, size_t len) {
  if (len > SIZE_EPBUFF) return false;

  if (usb_fifo_is_empty(&usb_ep_fifo))
    return false;

  for (size_t i = 0; i < SIZE_EPBUFF; i++) {
    data[i] = usb_ep_fifo.buffer[usb_ep_fifo.read_index][i];
  }

  usb_ep_fifo.read_index = (usb_ep_fifo.read_index + 1) % FIFO_DEPTH;
  usb_ep_fifo.count--;

  return true;
}
