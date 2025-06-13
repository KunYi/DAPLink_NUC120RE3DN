#pragma once
#ifndef __USB_FIFO_H__
#define __USB_FIFO_H__
#include <stdbool.h>

void usb_fifo_init(void);
bool usb_fifo_write(const uint8_t *data, size_t len);

#endif
