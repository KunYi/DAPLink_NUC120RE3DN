
#include <stdint.h>
#include <stdio.h>
#include "tx_api.h"
#include "NUC100Series.h"
#include "board_config.h"
#include "get_serial.h"
#include "usbd.h"
#include "usb_vendor.h"

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

/*----------------------------------------------------------------------------*/
/*!<USB Device Descriptor */
__ALIGNED(8) const uint8_t gu8DeviceDescriptor[] =
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
#define TOTOAL_INF          ((uint8_t)0x1)
#define LEN_WINUSB_CLASS    (LEN_INTERFACE + (LEN_ENDPOINT * 2))
#define TOTOAL_LEN          ((uint16_t)LEN_CONFIG + LEN_WINUSB_CLASS)
/*!<USB Configure Descriptor */
__ALIGNED(8) const uint8_t gu8ConfigDescriptor[] =
{
  LEN_CONFIG,               /* bLength                */
  DESC_CONFIG,              /* bDescriptorType        */
  LE16_TO_BYTES(TOTOAL_LEN),/* wTotalLength           */
  TOTOAL_INF,               /* bNumInterfaces         */
  0x01,                     /* bConfigurationValue    */
  0x00,                     /* iConfiguration         */
  DEVICE_DEFAULT_ATTRIBUTE, /* bmAttributes           */
  DEVICE_MAX_POWER,         /* MaxPower               */

  /* Interface 1: Vendor-Specific: (DAP V2: WINUSB) */
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
  LE16_TO_BYTES(EP4_MAX_PKT_SIZE),          /* wMaxPacketSize   */
  0x00,                                     /* bInterval        */

  /* ENDPOINT descriptor (DAP IN) */
  LEN_ENDPOINT,                             /* bLength          */
  DESC_ENDPOINT,                            /* bDescriptorType  */
  (EP_INPUT | DAP_IN_EP_NUM),               /* bEndpointAddress */
  EP_BULK,                                  /* bmAttributes     */
  LE16_TO_BYTES(EP5_MAX_PKT_SIZE),          /* wMaxPacketSize   */
  0x00,                                     /* bInterval        */
};

/*!<USB Language String Descriptor */
__ALIGNED(8) const uint8_t gu8StringLang[] =
{
    4,              /* bLength */
    DESC_STRING,    /* bDescriptorType */
    0x09, 0x04
};

/*!<USB Vendor String Descriptor */
__ALIGNED(8) const uint8_t gu8VendorStringDesc[] =
{
    14,
    DESC_STRING,
    // String: "UWINGS"
    'U', 0, 'W', 0, 'I', 0, 'N', 0, 'G', 0, 'S', 0,
};

/*!<USB Product String Descriptor */
__ALIGNED(8) const uint8_t gu8ProductStringDesc[] =
{
    26,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    // String: "CMSIS DAP v2"
    'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, ' ', 0, 'D', 0, 'A', 0, 'P', 0, ' ', 0, 'v', 0, '2', 0
};


__ALIGNED(8) const uint8_t gu8StringSerial[] =
{
    26,             // bLength
    DESC_STRING,    // bDescriptorType
    // String: "A02025060404"
    'A', 0, '0', 0, '2', 0, '0', 0, '2', 0, '5', 0, '0', 0, '6', 0, '0', 0, '4', 0, '0', 0, '4', 0
};

__ALIGNED(8) const uint8_t gu8DAPStringsDesc[] =
{
    46,             // bLength
    DESC_STRING,    // bDescriptorType
    // String: "CMSIS-DAP v2 Interface"
    'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, '-', 0, 'D', 0, 'A', 0, 'P', 0, ' ', 0,
    'v', 0, '2', 0, ' ', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0,
};


__ALIGNED(8) const uint8_t *gpu8UsbString[] =
{
    gu8StringLang,
    gu8VendorStringDesc,
    gu8ProductStringDesc,
    gu8StringSerial,
    gu8DAPStringsDesc,
};

/**
 * @brief Total length of the BOS Descriptor in bytes.
 *
 * Defines the total length of the Binary Object Store (BOS) Descriptor,
 * including the BOS Header (5 bytes) and Microsoft OS 2.0 Platform Capability
 * Descriptor (28 bytes). Used for DAP-LINK/WINUSB support.
 *
 * @note Update this value if additional Device Capability Descriptors
 *       (e.g., USB 2.0 Extension) are enabled.
 */
#define TOTAL_BOS_LEN   (LEN_BOS + LEN_MSOS2)

/**
 * @brief Total length of the Microsoft OS 2.0 Descriptor in bytes.
 *
 * Defines the total length of the MSOS 2.0 Descriptor (178/0xB2 bytes), which includes
 * Set Header, Configuration Subset Header, Function Subset Header, Compatible ID,
 * and Extended Properties for WINUSB support on Windows 8.1 and later.
 */
#define MSOS20_DESC_LEN (0xB2)

/**
 * @brief Windows version for MSOS 2.0 compatibility.
 *
 * Specifies the Windows 8.1 version (0x06030000) for MSOS 2.0 and BOS Descriptors.
 * Used to ensure compatibility with Windows 8.1 and later
 */
#define WINDOWS_BLUE    (0x06030000) /*^! Windows 8.1 */

/**
 * @brief USB Binary Object Store (BOS) Descriptor for DAP-LINK.
 *
 * This array defines the BOS Descriptor, which includes the BOS Header and
 * Microsoft OS 2.0 Platform Capability Descriptor to enable WINUSB driver support
 * on Windows 8.1 and later.
 *
 * @note The USB 2.0 Extension Descriptor is currently disabled (#if 0). Enable it
 *       if Link Power Management (LPM) is required, and update TOTAL_BOS_LEN and
 *       bNumDeviceCaps accordingly.
 * @note The MSOS 2.0 Platform Capability Descriptor references gu8MSOS20_Desc
 *       for additional WINUSB configuration.
 */
__ALIGNED(8) const uint8_t gpu8BOSDescriptor[] =
{
    // BOS Descriptor Header
    LEN_BOS,                       // bLength: 5 bytes
    DESC_BOS,                      // bDescriptorType:
    LE16_TO_BYTES(TOTAL_BOS_LEN),  // wTotalLength
    1,                             // bNumDeviceCaps
#if 0
    // USB 2.0 Extension Device Capability Descriptor
    LEN_USB20_EXT,                 // bLength: 7 bytes
    DESC_DEVICE_CAPABILITY,        // bDescriptorType: Device Capability
    CAP_USB20_EXTENSION,           // bDevCapabilityType: USB 2.0 Extension
    LE32_TO_BYTES(0x00000002),     // bmAttributes: Bit 1 = 1 (LPM supported), others reserved
#endif
    // Microsoft OS 2.0 Platform Capability Descriptor (for WINUSB)
    LEN_MSOS2,                     // bLength: 28 bytes
    DESC_DEVICE_CAPABILITY,        // bDescriptorType: Device Capability
    CAP_MS_OS_20,                  // bDevCapabilityType: Platform
    0x00,                          // bReserved
    MSOS20_PLATFORM_CAPABILITY_UUID, // PlatformCapabilityUUID: MS OS 20 Platform Capability ID
    LE32_TO_BYTES(WINDOWS_BLUE),   // dwWindowsVersion: Windows 8.1 (0x06030000) or 0 for generic
    LE16_TO_BYTES(MSOS20_DESC_LEN),// wMSOSDescriptorSetTotalLength: Length of MS OS 2.0 set
    MSOS_VENDOR_CODE,              // bMS_VendorCode: Vendor code for MS OS descriptors
    0x00                           // bAltEnumCode: No alternate enumeration
};

/**
 * @brief Microsoft OS 2.0 Descriptor for WINUSB support.
 *
 * This array defines the MSOS 2.0 descriptor for DAP-LINK,
 * including Set Header, Configuration Subset Header, Function Subset
 * Header, Compatible ID (WINUSB), and Extended Properties (DeviceInterfaceGUIDs)
 * to enable WINUSB driver loading on Windows 8.1 and later.
 *
 * @note Total length is defined by MSOS20_DESC_LEN (178/0xB2 bytes).
 */
__ALIGNED(8) const uint8_t gu8MSOS20_Desc[] =
{
  // Set header: length, type, windows version, total length
  LE16_TO_BYTES(0x000A), LE16_TO_BYTES(MSOS20_SET_HEADER_DESCRIPTOR), LE32_TO_BYTES(WINDOWS_BLUE), LE16_TO_BYTES(MSOS20_DESC_LEN),

  // Configuration subset header: length, type, configuration index, reserved, configuration total length
  LE16_TO_BYTES(0x0008), LE16_TO_BYTES(MSOS20_SUBSET_HEADER_CONFIGURATION), 0, 0, LE16_TO_BYTES(MSOS20_DESC_LEN - 0x0A),

  // Function Subset header: length, type, first interface, reserved, subset length
  LE16_TO_BYTES(0x0008), LE16_TO_BYTES(MSOS20_SUBSET_HEADER_FUNCTION), 2, 0, LE16_TO_BYTES(MSOS20_DESC_LEN - 0x0A - 0x08),

  // MS OS 2.0 Compatible ID descriptor:  Length: 20 bytes, Type: Compatible ID, sub compatible ID
  LE16_TO_BYTES(0x0014), LE16_TO_BYTES(MSOS20_FEATURE_COMPATIBLE_ID),
  'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,       // Compatible ID: "WINUSB\0\0"
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SubCompatibleID: all zeros

  // Extended Properties Feature Descriptor (for DeviceInterfaceGUID)
  LE16_TO_BYTES(MSOS20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14), LE16_TO_BYTES(MSOS20_FEATURE_REG_PROPERTY), // Length: 142 bytes, Type: Registry Property
  LE16_TO_BYTES(0x0007), // wPropertyDataType: Multiple NULL-terminated Unicode strings (REG_MULTI_SZ)
  LE16_TO_BYTES(0x002A), // wPropertyNameLength
  // PropertyName "DeviceInterfaceGUIDs\0" in UTF-16
  'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00,
  'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00,
  0x00, 0x00, // Multiple NULL-Terminated
  LE16_TO_BYTES(0x0050), // wPropertyDataLength
  // bPropertyData "{CDB3B5AD-293B-4663-AA36-1AAE46463776}" as a UTF-16 string, for CMSIS-DAP  v2 WINUSB
  '{', 0x00, 'C', 0x00, 'D', 0x00, 'B', 0x00, '3', 0x00, 'B', 0x00, '5', 0x00, 'A', 0x00, 'D', 0x00, '-', 0x00,
  '2', 0x00, '9', 0x00, '3', 0x00, 'B', 0x00, '-', 0x00, '4', 0x00, '6', 0x00, '6', 0x00, '3', 0x00, '-', 0x00,
  'A', 0x00, 'A', 0x00, '3', 0x00, '6', 0x00, '-', 0x00, '1', 0x00, 'A', 0x00, 'A', 0x00, 'E', 0x00, '4', 0x00,
  '6', 0x00, '4', 0x00, '6', 0x00, '3', 0x00, '7', 0x00, '7', 0x00, '6', 0x00, '}', 0x00,
  0x00, 0x00, 0x00, 0x00
};

__ALIGNED(8) const S_USBD_INFO_T gsInfo =
{
    gu8DeviceDescriptor,
    gu8ConfigDescriptor,
    gpu8UsbString,
    NULL,
    gpu8BOSDescriptor,
};
