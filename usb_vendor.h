#pragma once
#ifndef __USB_VENDOR_H__
#define __USB_VENDOR_H__

/* Define the vendor id and product id */
#define USBD_VID        0x0416
#define USBD_PID        0xB002

/*!<Define CDC Class Specific Request */
#define SET_LINE_CODE           0x20
#define GET_LINE_CODE           0x21
#define SET_CONTROL_LINE_STATE  0x22

/*-------------------------------------------------------------*/
/* Define EP maximum packet size */
#define EP0_MAX_PKT_SIZE    64
#define EP1_MAX_PKT_SIZE    EP0_MAX_PKT_SIZE
#define EP2_MAX_PKT_SIZE    64
#define EP3_MAX_PKT_SIZE    64
#define EP4_MAX_PKT_SIZE    64
#define EP5_MAX_PKT_SIZE    64

#define SETUP_BUF_BASE      0
#define SETUP_BUF_LEN       8
#define EP0_BUF_BASE        (SETUP_BUF_BASE + SETUP_BUF_LEN)
#define EP0_BUF_LEN         EP0_MAX_PKT_SIZE
#define EP1_BUF_BASE        (SETUP_BUF_BASE + SETUP_BUF_LEN)
#define EP1_BUF_LEN         EP1_MAX_PKT_SIZE
#define EP2_BUF_BASE        (EP1_BUF_BASE + EP1_BUF_LEN)
#define EP2_BUF_LEN         EP2_MAX_PKT_SIZE
#define EP3_BUF_BASE        (EP2_BUF_BASE + EP2_BUF_LEN)
#define EP3_BUF_LEN         EP3_MAX_PKT_SIZE
#define EP4_BUF_BASE        (EP3_BUF_BASE + EP3_BUF_LEN)
#define EP4_BUF_LEN         EP4_MAX_PKT_SIZE
#define EP5_BUF_BASE        (EP4_BUF_BASE + EP4_BUF_LEN)
#define EP5_BUF_LEN         EP5_MAX_PKT_SIZE

#define LEN_IAD             0x08
#define DESC_IAD            0x0B
#define DESC_CS_INTERFACE   0x24

/* Define Descriptor information */
#define USBD_SELF_POWERED               0
#define USBD_REMOTE_WAKEUP              0

#define USB_CONFIG_BMATTR_RESERVED       ((uint8_t)1 << 7)
#define USB_CONFIG_SELF_POWERED          ((uint8_t)1 << 6)
#define USB_CONFIG_REMOTE_WAKEUP         ((uint8_t)1 << 5)
#define DEVICE_DEFAULT_ATTRIBUTE         (USB_CONFIG_BMATTR_RESERVED)

#define DEVICE_MAX_POWER                50  /* The unit is in 2mA. ex: 50 * 2mA = 100mA */

#define STRING_MANUFACTURER             1
#define STRING_PRODUCT                  2
#define STRING_SERIAL                   3
#define STRING_WINUSB_INTERFACE         4
#define MAX_STRINGS                     5

#define BCD_USB                         (0x0210) // USB 2.1
#define BCD_DEVICE                      (0x0100) // hardware device version

#define LE16_TO_BYTES(x)            ((uint8_t)((x) & 0xFF)), ((uint8_t)(((x) >> 8) & 0xFF))
#define LE32_TO_BYTES(x)            ((uint8_t)((x) & 0xFF)), ((uint8_t)(((x) >> 8) & 0xFF)), \
                                    ((uint8_t)(((x) >> 16) & 0xFF)), ((uint8_t)(((x) >> 24) & 0xFF))

/* Define the interrupt In EP number */
#define CDC_NOTIFICATION_EP_NUM 0x01
#define CDC_DATA_OUT_EP_NUM     0x02
#define CDC_DATA_IN_EP_NUM      0x03
#define DAP_OUT_EP_NUM          0x04
#define DAP_IN_EP_NUM           0x05

#define MSOS_VENDOR_CODE           1

typedef struct
{
    uint32_t  u32DTERate;     /* Baud rate    */
    uint8_t   u8CharFormat;   /* stop bit     */
    uint8_t   u8ParityType;   /* parity       */
    uint8_t   u8DataBits;     /* data bits    */
} STR_VCOM_LINE_CODING;

__STATIC_INLINE void USBD_CustomerStart(void)
{
    tx_thread_sleep(20);

    /* Disable software-disconnect function */
    USBD_CLR_SE0();

    /* Clear USB-related interrupts before enable interrupt */
    USBD_CLR_INT_FLAG(USBD_INT_BUS | USBD_INT_USB | USBD_INT_FLDET | USBD_INT_WAKEUP);

    /* Enable USB-related interrupts. */
    USBD_ENABLE_INT(USBD_INT_BUS | USBD_INT_USB | USBD_INT_FLDET | USBD_INT_WAKEUP);
}

__STATIC_INLINE void Vendor_Init(void)
{
    /* Init setup packet buffer */
    /* Buffer for setup packet -> [0 ~ 0x7] */
    USBD->STBUFSEG = SETUP_BUF_BASE;

    /*****************************************************/
    /* EP0 ==> control IN endpoint, address 0 */
    USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
    /* Buffer range for EP0 */
    USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);
    /* EP1 ==> control OUT endpoint, address 0 */
    USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
    /* Buffer range for EP1 */
    USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);

    /*****************************************************/
    /* Endpoint configuration for CDC */
    /* EP2 ==> Bulk Out endpoint, address 2 */
    USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_OUT | CDC_DATA_OUT_EP_NUM);
    /* Buffer offset for EP2 */
    USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);
    /* trigger receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP2, EP2_MAX_PKT_SIZE);
    /* EP3 ==> Bulk IN endpoint, address 3 */
    USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_IN | CDC_DATA_IN_EP_NUM);
    /* Buffer offset for EP3 */
    USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);

    /*****************************************************/
    /* Endpoint configuration for DAP/WINUSB */
    /* EP4 ==> Bulk Out endpoint, address 4 */
    USBD_CONFIG_EP(EP4, USBD_CFG_EPMODE_OUT | DAP_OUT_EP_NUM);
    /* Buffer offset for EP2 */
    USBD_SET_EP_BUF_ADDR(EP4, EP4_BUF_BASE);
    /* trigger receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP4, EP4_MAX_PKT_SIZE);
    /* EP5 ==> Bulk IN endpoint, address 5 */
    USBD_CONFIG_EP(EP5, USBD_CFG_EPMODE_IN | DAP_IN_EP_NUM);
    /* Buffer offset for EP5 */
    USBD_SET_EP_BUF_ADDR(EP5, EP5_BUF_BASE);
}

// export global variables
extern STR_VCOM_LINE_CODING gLineCoding;
extern uint16_t gCtrlSignal;

// export functions
extern void CDC_ClassRequest(void);
extern void Vendor_Request(void);

void VCOM_LineCoding(uint8_t port);
#endif
