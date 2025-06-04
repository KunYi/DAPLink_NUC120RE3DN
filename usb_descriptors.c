
#include <stdint.h>
#include <stdio.h>
#include "NUC100Series.h"
#include "get_serial.h"
#include "board_config.h"
#include "usbd.h"

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
#define CDC_DATA_OUT_EP_NUM 0x02
#define CDC_DATA_IN_EP_NUM 0x03
#define DAP_OUT_EP_NUM 0x04
#define DAP_IN_EP_NUM 0x05

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

/*----------------------------------------------------------------------------*/
/*!<USB Device Descriptor */
uint8_t gu8DeviceDescriptor[] =
{
  LEN_DEVICE,               /* bLength */
  DESC_DEVICE,              /* bDescriptorType */
  LE16_TO_BYTES(BCD_USB),   /* bcdUSB = 0x0210 (USB 2.1) */
  0xEF,                     /* bDeviceClass */
  0x02,                     /* bDeviceSubClass */
  0x01,                     /* bDeviceProtocol */
  EP0_MAX_PKT_SIZE,         /* bMaxPacketSize0 */
  LE16_TO_BYTES(USBD_VID),  /* idVendor */
  LE16_TO_BYTES(USBD_PID),  /* idProduct */
  LE16_TO_BYTES(BCD_DEVICE),/* bcdDevice */
  STRING_MANUFACTURER,      /* iManufacture */
  STRING_PRODUCT,           /* iProduct */
  STRING_SERIAL,            /* iSerialNumber - no serial */
  0x01                      /* bNumConfigurations */
};

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
#define TOTOAL_INF          ((uint8_t)0x3)
#define LEN_CDC_CLASS       (LEN_IAD + (5 * 3) + 4 + (LEN_INTERFACE * 2) + (LEN_ENDPOINT * 3))
#define LEN_WINUSB_CLASS    (LEN_INTERFACE + (LEN_ENDPOINT * 2))
#define TOTOAL_LEN          ((uint16_t)LEN_CONFIG + LEN_CDC_CLASS + LEN_WINUSB_CLASS)
/*!<USB Configure Descriptor */
uint8_t gu8ConfigDescriptor[] =
{
  LEN_CONFIG,               /* bLength                */
  DESC_CONFIG,              /* bDescriptorType        */
  LE16_TO_BYTES(TOTOAL_LEN),/* wTotalLength           */
  TOTOAL_INF,               /* bNumInterfaces         */
  0x01,                     /* bConfigurationValue    */
  0x00,                     /* iConfiguration         */
  DEVICE_DEFAULT_ATTRIBUTE, /* bmAttributes           */
  DEVICE_MAX_POWER,         /* MaxPower               */

  /* IAD of CDC */
  LEN_IAD,                  /* bLength                */
  DESC_IAD,                 /* bDescriptorType        */
  0x00,                     /* bFirstInterface        */
  0x02,                     /* bInterfaceCount        */
  0x02,                     /* bFunctionClass: CDC    */
  0x02,                     /* bFunctionSubClass: ACM */
  0x00,                     /* bFunctionProtocol:     */
  STRING_CDC_INTERFACE,     /* iFunction              */

  /* Interface 0: CDC Control */
  LEN_INTERFACE,            /* bLength                */
  DESC_INTERFACE,           /* bDescriptorType        */
  0x00,                     /* bInterfaceNumber       */
  0x00,                     /* bAlternateSetting      */
  0x01,                     /* bNumEndpoints          */
  0x02,                     /* bInterfaceClass        */
  0x02,                     /* bInterfaceSubClass     */
  0x01,                     /* bInterfaceProtocol     */
  STRING_CDC_INTERFACE,     /* iInterface             */

  /* Communication Class Specified INTERFACE descriptor (Header) */
  0x05,                     /* Size of the descriptor, in bytes */
  DESC_CS_INTERFACE,        /* CS_INTERFACE descriptor type */
  0x00,                     /* Header functional descriptor subtype */
  LE16_TO_BYTES(0x0110),    /* Communication device compliant to the communication spec. ver. 1.10 */

  /* Communication Class Specified INTERFACE descriptor (Call management) */
  0x05,                     /* Size of the descriptor, in bytes */
  DESC_CS_INTERFACE,        /* CS_INTERFACE descriptor type */
  0x01,                     /* Call management functional descriptor */
  0x00,                     /* BIT0: Whether device handle call management itself. */
                            /* BIT1: Whether device can send/receive call management information over a Data Class Interface 0 */
  0x01,                     /* Interface number of data class interface optionally used for call management */

  /* Communication Class Specified INTERFACE descriptor (ACM) */
  0x04,                     /* Size of the descriptor, in bytes */
  DESC_CS_INTERFACE,        /* CS_INTERFACE descriptor type */
  0x02,                     /* Abstract control management functional descriptor subtype */
  0x02,                     /* bmCapabilities: SetLineCoding */

  /* Communication Class Specified INTERFACE descriptor (Union) */
  0x05,                     /* bLength              */
  DESC_CS_INTERFACE,        /* bDescriptorType: CS_INTERFACE descriptor type */
  0x06,                     /* bDescriptorSubType   */
  0x00,                     /* bMasterInterface     */
  0x01,                     /* bSlaveInterface0     */

  /* ENDPOINT descriptor (CDC NOTIFICATION) */
  LEN_ENDPOINT,                             /* bLength          */
  DESC_ENDPOINT,                            /* bDescriptorType  */
  (EP_INPUT | CDC_NOTIFICATION_EP_NUM),     /* bEndpointAddress */
  EP_INT,                                   /* bmAttributes     */
  LE16_TO_BYTES(EP1_MAX_PKT_SIZE),          /* wMaxPacketSize   */
  0x01,                                     /* bInterval        */

  /* Interface 1: CDC Data */
  LEN_INTERFACE,            /* bLength              */
  DESC_INTERFACE,           /* bDescriptorType      */
  0x01,                     /* bInterfaceNumber     */
  0x00,                     /* bAlternateSetting    */
  0x02,                     /* bNumEndpoints        */
  0x0A,                     /* bInterfaceClass      */
  0x00,                     /* bInterfaceSubClass   */
  0x00,                     /* bInterfaceProtocol   */
  STRING_CDC_INTERFACE,     /* iInterface           */

  /* ENDPOINT descriptor (CDC DATA OUT) */
  LEN_ENDPOINT,                             /* bLength          */
  DESC_ENDPOINT,                            /* bDescriptorType  */
  (EP_OUTPUT | CDC_DATA_OUT_EP_NUM),        /* bEndpointAddress */
  EP_BULK,                                  /* bmAttributes     */
  LE16_TO_BYTES(EP2_MAX_PKT_SIZE),          /* wMaxPacketSize   */
  0x00,                                     /* bInterval        */

  /* ENDPOINT descriptor (CDC DATA IN) */
  LEN_ENDPOINT,                             /* bLength          */
  DESC_ENDPOINT,                            /* bDescriptorType  */
  (EP_INPUT | CDC_DATA_IN_EP_NUM),          /* bEndpointAddress */
  EP_BULK,                                  /* bmAttributes     */
  LE16_TO_BYTES(EP3_MAX_PKT_SIZE),          /* wMaxPacketSize   */
  0x00,                                     /* bInterval        */

  /* Interface 2: Vendor-Specific: (DAP V2: WINUSB) */
  LEN_INTERFACE,            /* bLength              */
  DESC_INTERFACE,           /* bDescriptorType      */
  0x02,                     /* bInterfaceNumber     */
  0x00,                     /* bAlternateSetting    */
  0x02,                     /* bNumEndpoints        */
  0xFF,                     /* bInterfaceClass: Vendor-Specific */
  0x00,                     /* bInterfaceSubClass   */
  0x00,                     /* bInterfaceProtocol   */
  STRING_WINUSB_INTERFACE,  /* iInterface           */

  /* ENDPOINT descriptor (DAP OUT) */
  LEN_ENDPOINT,                             /* bLength          */
  DESC_ENDPOINT,                            /* bDescriptorType  */
  (EP_OUTPUT | DAP_OUT_EP_NUM),             /* bEndpointAddress */
  EP_BULK,                                  /* bmAttributes     */
  LE16_TO_BYTES(EP3_MAX_PKT_SIZE),          /* wMaxPacketSize   */
  0x00,                                     /* bInterval        */

  /* ENDPOINT descriptor (DAP IN) */
  LEN_ENDPOINT,                             /* bLength          */
  DESC_ENDPOINT,                            /* bDescriptorType  */
  (EP_INPUT | DAP_IN_EP_NUM),               /* bEndpointAddress */
  EP_BULK,                                  /* bmAttributes     */
  LE16_TO_BYTES(EP3_MAX_PKT_SIZE),          /* wMaxPacketSize   */
  0x00,                                     /* bInterval        */
};

/*!<USB Language String Descriptor */
uint8_t gu8StringLang[] =
{
    4,              /* bLength */
    DESC_STRING,    /* bDescriptorType */
    0x09, 0x04
};

/*!<USB Vendor String Descriptor */
uint8_t gu8VendorStringDesc[] =
{
    14,
    DESC_STRING,
    // String: "UWINGS"
    'U', 0, 'W', 0, 'I', 0, 'N', 0, 'G', 0, 'S', 0,
};

/*!<USB Product String Descriptor */
uint8_t gu8ProductStringDesc[] =
{
    26,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    // String: "CMSIS DAP v2"
    'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, ' ', 0, 'D', 0, 'A', 0, 'P', 0, ' ', 0, 'v', 0, '2', 0
};


const uint8_t gu8StringSerial[] =
{
    26,             // bLength
    DESC_STRING,    // bDescriptorType
    // String: "A02025060404"
    'A', 0, '0', 0, '2', 0, '0', 0, '2', 0, '5', 0, '0', 0, '6', 0, '0', 0, '4', 0, '0', 0, '4', 0
};

const uint8_t gu8CDCStringsDesc[] =
{
    46,             // bLength
    DESC_STRING,    // bDescriptorType
    // String: "CDC-ACM UART Interface"
    'C', 0, 'D', 0, 'C', 0, '-', 0, 'A', 0, 'C', 0, 'M', 0, ' ', 0,
    'U', 0, 'A', 0, 'R', 0, 'T', 0, ' ', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0,
};

const uint8_t gu8DAPStringsDesc[] =
{
    46,             // bLength
    DESC_STRING,    // bDescriptorType
    // String: "CMSIS-DAP v2 Interface"
    'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, '-', 0, 'D', 0, 'A', 0, 'P', 0, ' ', 0,
    'v', 0, '2', 0, ' ', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0,
};


const uint8_t *gpu8UsbString[] =
{
    gu8StringLang,
    gu8VendorStringDesc,
    gu8ProductStringDesc,
    gu8StringSerial,
    gu8CDCStringsDesc,
    gu8DAPStringsDesc,
};

const S_USBD_INFO_T gsInfo =
{
    gu8DeviceDescriptor,
    gu8ConfigDescriptor,
    gpu8UsbString,
    NULL
};
