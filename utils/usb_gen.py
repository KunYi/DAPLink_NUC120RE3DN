import yaml
import sys
from typing import Dict, List, Tuple
from collections import defaultdict

# 驗證端點和緩衝區配置
def validate_config(config: Dict) -> Tuple[int, List[Tuple], Dict]:
    device = config.get("device", {})
    if not device:
        raise ValueError("Missing 'device' section in YAML")

    max_endpoints = device.get("endpoints", {}).get("max_count", 0)
    if max_endpoints < 1:
        raise ValueError("Invalid 'max_count' in device.endpoints, must be > 0")

    speed = device.get("speed", "full")
    max_packet_size0 = device.get("max_packet_size0", 8)
    if speed == "full" and max_packet_size0 > 64:
        raise ValueError("Control endpoint max_packet_size0 must be <= 64 for Full Speed")

    # 收集所有端點需求並按優先級排序
    interfaces = config.get("configuration", {}).get("interfaces", [])
    if not interfaces:
        raise ValueError("Missing 'interfaces' section in configuration")

    endpoints_needed = []
    for iface in sorted(interfaces, key=lambda x: x.get("priority", 999)):
        if iface["class"] == "cdc":
            comm = iface.get("communication", {})
            data = iface.get("data", {})
            if not comm or not data:
                raise ValueError(f"CDC interface missing 'communication' or 'data' section: {iface}")
            if not comm.get("endpoints") or len(comm["endpoints"]) != 1:
                raise ValueError(f"CDC communication interface must have exactly 1 endpoint")
            if not data.get("endpoints") or len(data["endpoints"]) != 2:
                raise ValueError(f"CDC data interface must have exactly 2 endpoints")
            endpoints_needed.append((iface, "communication", comm["endpoints"][0]))
            endpoints_needed.append((iface, "data", data["endpoints"][0]))
            endpoints_needed.append((iface, "data", data["endpoints"][1]))
        elif iface["class"] == "custom":
            if not iface.get("endpoints") or len(iface["endpoints"]) != 2:
                raise ValueError(f"Custom interface must have exactly 2 endpoints: {iface}")
            for ep in iface["endpoints"]:
                endpoints_needed.append((iface, None, ep))

    # 檢查端點數量是否超過限制
    if len(endpoints_needed) > max_endpoints:
        raise ValueError(f"Too many endpoints ({len(endpoints_needed)}) for max_count ({max_endpoints})")

    # 分配端點編號
    used_endpoints = set()
    endpoint_assignments = []
    for iface, subiface, ep in endpoints_needed:
        for ep_num in range(1, max_endpoints + 1):
            if ep_num not in used_endpoints:
                used_endpoints.add(ep_num)
                ep["number"] = ep_num
                endpoint_assignments.append((iface, subiface, ep))
                break
        else:
            raise ValueError(f"No available endpoint for {iface['class']} interface")

    # 分配接口編號
    interface_numbers = {}
    current_if_num = 0
    for iface in interfaces:
        if iface["class"] == "cdc":
            comm = iface["communication"]
            data = iface["data"]
            if comm.get("interface_number") == "auto":
                comm["interface_number"] = current_if_num
                interface_numbers[("cdc", "communication")] = current_if_num
                current_if_num += 1
            else:
                interface_numbers[("cdc", "communication")] = comm["interface_number"]
            if data.get("interface_number") == "auto":
                data["interface_number"] = current_if_num
                interface_numbers[("cdc", "data")] = current_if_num
                current_if_num += 1
            else:
                interface_numbers[("cdc", "data")] = data["interface_number"]
            # 更新 CDC 功能描述符的接口參考
            for fd in comm.get("functional_descriptors", []):
                if fd["type"] in ["call_management", "union"]:
                    if fd.get("data_interface") == "auto":
                        fd["data_interface"] = data["interface_number"]
                    if fd["type"] == "union":
                        if fd.get("master_interface") == "auto":
                            fd["master_interface"] = comm["interface_number"]
                        if fd.get("slave_interface") == "auto":
                            fd["slave_interface"] = data["interface_number"]
        else:
            if iface.get("interface_number") == "auto":
                iface["interface_number"] = current_if_num
                interface_numbers[(iface["class"], None)] = current_if_num
                current_if_num += 1
            else:
                interface_numbers[(iface["class"], None)] = iface["interface_number"]

    return max_endpoints, endpoint_assignments, interface_numbers

# 將字符串轉為 Unicode 數組
def string_to_unicode(s: str) -> str:
    return ", ".join(f"0x{ord(c):04X}" for c in s)

# 生成端點描述符（縮排，帶詳細註釋）
def generate_endpoint_descriptor(ep: Dict) -> List[str]:
    ep_type = {"interrupt": 0x03, "bulk": 0x02}.get(ep["type"], 0x02)
    direction = 0x80 if ep["direction"] == "in" else 0x00
    ep_num = ep.get("number", 0)
    if ep_num == 0:
        raise ValueError(f"Endpoint number not assigned for {ep}")
    max_packet_size = ep.get("max_packet_size", 64)
    interval = ep.get("interval", 0)
    return [
        f"    // Endpoint {ep_num}: {'Interrupt' if ep_type == 0x03 else 'Bulk'} {'IN' if direction else 'OUT'}",
        f"    0x07,       // bLength: Size of descriptor (7 bytes)",
        f"    0x05,       // bDescriptorType: Endpoint",
        f"    0x{ep_num | direction:02X},       // bEndpointAddress: Endpoint {ep_num} {'IN' if direction else 'OUT'}",
        f"    0x{ep_type:02X},       // bmAttributes: {'Interrupt' if ep_type == 0x03 else 'Bulk'} transfer type",
        f"    0x{max_packet_size & 0xFF:02X}, 0x{(max_packet_size >> 8) & 0xFF:02X}, // wMaxPacketSize: {max_packet_size} bytes",
        f"    0x{interval:02X},       // bInterval: Polling interval ({interval} ms for Interrupt, 0 for Bulk)",
    ]

# 生成 C 程式碼
def generate_c_code(config: Dict, max_endpoints: int, endpoint_assignments: List[Tuple], interface_numbers: Dict) -> str:
    device = config["device"]
    vid = device.get("vid", 0x0000)
    pid = device.get("pid", 0x0000)
    bcd_device = int(device.get("bcd_device", "1.00").replace(".", ""), 16)
    max_power = device.get("max_power", 100) // 2
    bm_attributes = 0x80 if config.get("configuration", {}).get("attributes") == "bus_powered" else 0xC0

    # 計算配置描述符總長度
    total_length = 9  # Configuration Descriptor
    interfaces = config.get("configuration", {}).get("interfaces", [])
    interface_count = sum(2 if iface["class"] == "cdc" else 1 for iface in interfaces)
    string_indices = defaultdict(int)
    string_indices[device["manufacturer"]] = 1
    string_indices[device["product"]] = 2
    string_indices[device["serial"]] = 3
    next_string_index = 4

    # 計算描述符長度
    for iface in interfaces:
        if iface["class"] == "cdc":
            total_length += 9 + 5 + 5 + 4 + 5 + 7  # Comm Interface + Functional Descs + EP
            total_length += 9 + 7 + 7  # Data Interface + 2 EPs
        elif iface["class"] == "custom":
            total_length += 9 + 7 + 7  # Custom Interface + 2 EPs

    # 生成配置描述符
    config_desc = [
        f"""    // Configuration Descriptor
    0x09,       // bLength: Size of descriptor (9 bytes)
    0x02,       // bDescriptorType: Configuration
    0x{total_length & 0xFF:02X}, 0x{(total_length >> 8) & 0xFF:02X}, // wTotalLength: Total length of configuration ({total_length} bytes)
    0x{interface_count:02X},       // bNumInterfaces: Number of interfaces ({interface_count})
    0x01,       // bConfigurationValue: Configuration ID (1)
    0x00,       // iConfiguration: Index of string descriptor (0 = none)
    0x{bm_attributes:02X},       // bmAttributes: {'Bus-powered' if bm_attributes == 0x80 else 'Self-powered'} device
    0x{max_power:02X},       // bMaxPower: Maximum power consumption ({max_power * 2} mA)
"""
    ]

    # 生成接口描述符
    for iface in interfaces:
        if iface["class"] == "cdc":
            comm = iface["communication"]
            data = iface["data"]
            string_indices[comm["string"]] = next_string_index
            next_string_index += 1
            string_indices[data["string"]] = next_string_index
            next_string_index += 1
            config_desc.append(f"""
    // CDC Communication Interface (Interface {comm['interface_number']})
    0x09,       // bLength: Size of descriptor (9 bytes)
    0x04,       // bDescriptorType: Interface
    0x{comm['interface_number']:02X},       // bInterfaceNumber: Interface {comm['interface_number']}
    0x00,       // bAlternateSetting: Alternate setting (0)
    0x01,       // bNumEndpoints: Number of endpoints (1)
    0x{comm['subclass']:02X},       // bInterfaceClass: Communications Interface Class (CDC)
    0x{comm['protocol']:02X},       // bInterfaceSubClass: Abstract Control Model (ACM)
    0x01,       // bInterfaceProtocol: AT Commands (V.25ter)
    0x{string_indices[comm['string']]:02X},       // iInterface: String index for interface ({comm['string']})
""")
            config_desc.append(f"""    // CDC Functional Descriptors
    // Header Functional Descriptor
    0x05,       // bLength: Size of descriptor (5 bytes)
    0x24,       // bDescriptorType: CS_INTERFACE
    0x00,       // bDescriptorSubType: Header Functional Descriptor
    0x10, 0x01, // bcdCDC: CDC specification version (1.10)
    // Call Management Functional Descriptor
    0x05,       // bLength: Size of descriptor (5 bytes)
    0x24,       // bDescriptorType: CS_INTERFACE
    0x01,       // bDescriptorSubType: Call Management Functional Descriptor
    0x00,       // bmCapabilities: Device handles call management
    0x{data['interface_number']:02X},       // bDataInterface: Data interface ({data['interface_number']})
    // Abstract Control Management (ACM) Functional Descriptor
    0x04,       // bLength: Size of descriptor (4 bytes)
    0x24,       // bDescriptorType: CS_INTERFACE
    0x02,       // bDescriptorSubType: Abstract Control Management (ACM)
    0x{comm['functional_descriptors'][2]['capabilities']:02X},       // bmCapabilities: Supports Set/Get Line Coding, Serial State
    // Union Functional Descriptor
    0x05,       // bLength: Size of descriptor (5 bytes)
    0x24,       // bDescriptorType: CS_INTERFACE
    0x06,       // bDescriptorSubType: Union Functional Descriptor
    0x{comm['interface_number']:02X},       // bMasterInterface: Communication interface ({comm['interface_number']})
    0x{data['interface_number']:02X},       // bSlaveInterface: Data interface ({data['interface_number']})
""")
            config_desc.extend(generate_endpoint_descriptor(comm["endpoints"][0]))
            config_desc.append("")
            config_desc.append(f"""
    // CDC Data Interface (Interface {data['interface_number']})
    0x09,       // bLength: Size of descriptor (9 bytes)
    0x04,       // bDescriptorType: Interface
    0x{data['interface_number']:02X},       // bInterfaceNumber: Interface {data['interface_number']}
    0x00,       // bAlternateSetting: Alternate setting (0)
    0x02,       // bNumEndpoints: Number of endpoints (2)
    0x0A,       // bInterfaceClass: CDC Data
    0x00,       // bInterfaceSubClass: No subclass
    0x00,       // bInterfaceProtocol: No protocol
    0x{string_indices[data['string']]:02X},       // iInterface: String index for interface ({data['string']})
""")
            config_desc.extend(generate_endpoint_descriptor(data["endpoints"][0]))
            config_desc.append("")
            config_desc.extend(generate_endpoint_descriptor(data["endpoints"][1]))
            config_desc.append("")
        elif iface["class"] == "custom":
            string_indices[iface["string"]] = next_string_index
            next_string_index += 1
            config_desc.append(f"""
    // CMSIS-DAP v2 Interface (Interface {iface['interface_number']})
    0x09,       // bLength: Size of descriptor (9 bytes)
    0x04,       // bDescriptorType: Interface
    0x{iface['interface_number']:02X},       // bInterfaceNumber: Interface {iface['interface_number']}
    0x00,       // bAlternateSetting: Alternate setting (0)
    0x02,       // bNumEndpoints: Number of endpoints (2)
    0x{iface['class_code']:02X},       // bInterfaceClass: Vendor-specific
    0x{iface['subclass_code']:02X},       // bInterfaceSubClass: No subclass
    0x{iface['protocol_code']:02X},       // bInterfaceProtocol: No protocol
    0x{string_indices[iface['string']]:02X},       // iInterface: String index for interface ({iface['string']})
""")
            config_desc.extend(generate_endpoint_descriptor(iface["endpoints"][0]))
            config_desc.append("")
            config_desc.extend(generate_endpoint_descriptor(iface["endpoints"][1]))
            config_desc.append("")

    # 生成 C 程式碼
    output = f"""
#include "usb_desc.h"

// Device Descriptor
const uint8_t usbd_device_descriptor[] = {{
    0x12,       // bLength: Size of descriptor (18 bytes)
    0x01,       // bDescriptorType: Device
    0x00, 0x02, // bcdUSB: USB specification version (2.00)
    0x{device['class']:02X},       // bDeviceClass: Class code (0x00 = defined by interface)
    0x{device['subclass']:02X},       // bDeviceSubClass: Subclass code (0x00)
    0x{device['protocol']:02X},       // bDeviceProtocol: Protocol code (0x00)
    0x{device['max_packet_size0']:02X},       // bMaxPacketSize0: Maximum packet size for EP0 ({device['max_packet_size0']} bytes)
    0x{vid & 0xFF:02X}, 0x{(vid >> 8) & 0xFF:02X}, // idVendor: Vendor ID (0x{vid:04X})
    0x{pid & 0xFF:02X}, 0x{(pid >> 8) & 0xFF:02X}, // idProduct: Product ID (0x{pid:04X})
    0x{bcd_device & 0xFF:02X}, 0x{(bcd_device >> 8) & 0xFF:02X}, // bcdDevice: Device release number (0x{bcd_device:04X})
    0x01,       // iManufacturer: String index for manufacturer ({device['manufacturer']})
    0x02,       // iProduct: String index for product ({device['product']})
    0x03,       // iSerialNumber: String index for serial number ({device['serial']})
    0x01        // bNumConfigurations: Number of configurations (1)
}};

// Configuration Descriptor
const uint8_t usbd_config_descriptor[] = {{
{''.join(config_desc)}
}};

// Microsoft OS Descriptors
const uint8_t usbd_msos_descriptor[] = {{
    0x12,       // bLength: Size of descriptor (18 bytes)
    0x03,       // bDescriptorType: String
    'M', 0, 'S', 0, 'F', 0, 'T', 0, '1', 0, '0', 0, '0', 0, // qwSignature: MSFT100
    0x01,       // bMS_VendorCode: Vendor code for MS OS descriptors
    0x00        // bPad: Padding
}};

const uint8_t usbd_msos_feature_descriptor[] = {{
    // Extended Compat ID Descriptor
    0x30, 0x00, // wLength: Size of descriptor (48 bytes)
    0x00, 0x01, // bcdVersion: Version (1.0)
    0x04, 0x00, // wIndex: Extended Compat ID descriptor
    0x01,       // bCount: Number of function sections (1)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved
    // Function Section for CMSIS-DAP v2
    0x02,       // bFirstInterfaceNumber: CMSIS-DAP v2 interface (2)
    0x01,       // Reserved
    'W', 'I', 'N', 'U', 'S', 'B', 0, 0, // CompatibleID: WinUSB
    0, 0, 0, 0, 0, 0, 0, 0, // SubCompatibleID: None
"""

    # 處理 Microsoft OS GUID
    for iface in interfaces:
        if iface["class"] == "custom" and "ms_os" in iface:
            guid = iface["ms_os"]["guid"].strip("{}").replace("-", "")
            guid_bytes = []
            # Data1 (4 字節，CDB3B5AD，小端序)
            val = int(guid[0:8], 16)
            guid_bytes.extend([val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF])
            # Data2 (2 字節，293B，小端序)
            val = int(guid[8:12], 16)
            guid_bytes.extend([val & 0xFF, (val >> 8) & 0xFF])
            # Data3 (2 字節，4663，小端序)
            val = int(guid[12:16], 16)
            guid_bytes.extend([val & 0xFF, (val >> 8) & 0xFF])
            # Data4[0..1] (2 字節，AA36，大端序)
            val = int(guid[16:20], 16)
            guid_bytes.extend([(val >> 8) & 0xFF, val & 0xFF])
            # Data4[2..7] (6 字節，1AAE46463776，大端序)
            for i in range(20, 32, 2):
                val = int(guid[i:i+2], 16)
                guid_bytes.append(val)
            output += "    " + ", ".join(f"0x{b:02X}" for b in guid_bytes) + f", // GUID: {{{iface['ms_os']['guid']}}}\n"
            break
    output += "};\n\n"

    # 生成字符串描述符
    output += f"""// String Descriptors
const uint8_t usbd_string0[] = {{
    0x04,       // bLength: Size of descriptor (4 bytes)
    0x03,       // bDescriptorType: String
    0x09, 0x04  // wLangID: Language ID (0x0409 = US English)
}};
const uint16_t usbd_string1[] = {{ {string_to_unicode(device.get('manufacturer', ''))}, 0 // Manufacturer: {device.get('manufacturer', '')} }};
const uint16_t usbd_string2[] = {{ {string_to_unicode(device.get('product', ''))}, 0 // Product: {device.get('product', '')} }};
const uint16_t usbd_string3[] = {{ {string_to_unicode(device.get('serial', ''))}, 0 // Serial Number: {device.get('serial', '')} }};
"""
    for s, idx in sorted(string_indices.items(), key=lambda x: x[1]):
        if idx >= 4:
            output += f"const uint16_t usbd_string{idx}[] = {{ {string_to_unicode(s)}, 0 // {s} }};\n"

    # 生成端點緩衝區配置
    output += "\n// Endpoint Buffer Configuration\n"
    for iface, subiface, ep in endpoint_assignments:
        buffer_size = ep.get("buffer_size", device["endpoints"]["buffer_size"])
        if buffer_size < ep.get("max_packet_size", 64):
            raise ValueError(f"Buffer size ({buffer_size}) too small for endpoint {ep['number']} (max_packet_size: {ep['max_packet_size']})")
        output += f"// Endpoint {ep['number']} ({ep['type'].capitalize()} {ep['direction'].capitalize()}) Buffer Size: {buffer_size} bytes\n"
        output += f"uint8_t ep{ep['number']}_buffer[{buffer_size}] __attribute__((aligned(4)));\n"

    return output

# 主函數
def main():
    try:
        with open("usb_desc.yaml", "r") as file:
            config = yaml.safe_load(file)

        # 驗證並分配端點與接口編號
        max_endpoints, endpoint_assignments, interface_numbers = validate_config(config)

        # 生成 C 程式碼
        c_code = generate_c_code(config, max_endpoints, endpoint_assignments, interface_numbers)

        # 寫入 usb_desc.c
        with open("usb_desc.c", "w") as f:
            f.write(c_code)

        # 生成 usb_desc.h
        header = """
#ifndef __USB_DESC_H__
#define __USB_DESC_H__

#include <stdint.h>

extern const uint8_t usbd_device_descriptor[];
extern const uint8_t usbd_config_descriptor[];
extern const uint8_t usbd_msos_descriptor[];
extern const uint8_t usbd_msos_feature_descriptor[];
extern const uint8_t usbd_string0[];
extern const uint16_t usbd_string1[];
extern const uint16_t usbd_string2[];
extern const uint16_t usbd_string3[];
"""
        string_indices = defaultdict(int)
        string_indices[config["device"]["manufacturer"]] = 1
        string_indices[config["device"]["product"]] = 2
        string_indices[config["device"]["serial"]] = 3
        next_string_index = 4
        for iface in config["configuration"]["interfaces"]:
            if iface["class"] == "cdc":
                string_indices[iface["communication"]["string"]] = next_string_index
                next_string_index += 1
                string_indices[iface["data"]["string"]] = next_string_index
                next_string_index += 1
            else:
                string_indices[iface["string"]] = next_string_index
                next_string_index += 1
        for i in range(4, next_string_index):
            header += f"extern const uint16_t usbd_string{i}[];\n"
        for i in range(1, max_endpoints + 1):
            header += f"extern uint8_t ep{i}_buffer[];\n"
        header += "\n#endif"

        with open("usb_desc.h", "w") as f:
            f.write(header)

        print("Generated usb_desc.c and usb_desc.h successfully!")

    except Exception as e:
        print(f"Error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
