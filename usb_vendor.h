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
#define EP4_MAX_PKT_SIZE    8

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
#define STRING_CDC_INTERFACE            4
#define STRING_WINUSB_INTERFACE         5

#define BCD_USB                         (0x0210) // USB 2.1
#define BCD_DEVICE                      (0x0100) // hardware device version

#define LE16_TO_BYTES(x)  ((uint8_t)((x) & 0xFF)), ((uint8_t)(((x) >> 8) & 0xFF))

/* Define the interrupt In EP number */
#define CDC_NOTIFICATION_EP_NUM 0x01
#define CDC_DATA_OUT_EP_NUM     0x02
#define CDC_DATA_IN_EP_NUM      0x03
#define DAP_OUT_EP_NUM          0x04
#define DAP_IN_EP_NUM           0x05

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

extern STR_VCOM_LINE_CODING gLineCoding;
extern uint16_t gCtrlSignal;

extern void Vendor_ClassRequest(void);
#endif
