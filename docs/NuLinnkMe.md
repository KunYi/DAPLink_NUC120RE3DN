# Nu-Link-ME

## MCU - NUC120RE3DN

- ARCH: Cortex-M0
- System Clock: 50MHz
- Crystal: 12MHz
- FLASH: 128KB, address range[0x0 - 0x1FFFF]
- RAM: 16KB, address range[0x20000000 - 0x20003FFF]
- Unique ID(96bits): use FLASH command read

## Interface

### SWDs

- nRST, PC8, (SPI Chip Select)
- SWD_CLK, PC9 (SPI Clock)
- SWD_DIO
  - SWD_DI, PC10 (SPI MISO)
  - SWD_DO, PC11 (SPI MOSI)

### UART

- UART_RX, PB0
- UART_TX, PB1

### Boot Configure

- MSG_En, PA1 for Enable USB MSC
- VCOM_En, PE5 for Enabled USB CDC/ACM

### LEDs

- ICELED, PB4
- ICPLED, PB5
- RED,    PB6
- GREEN,  PB7

### BUTTONs

- DAP, PB14 for Enabled DAPLink Firmware Update
