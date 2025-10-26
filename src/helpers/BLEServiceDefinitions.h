#pragma once

#include <stdint.h>
#include <stddef.h>

// MeshCore manufacturer ID - encoded as 'MC' (MeshCore) in little-endian
// 'C' = 0x43, 'M' = 0x4D -> 0x4D43 in big-endian, 0x434D in little-endian
#define MESHCORE_MANUFACTURER_ID  0x434D  // 'CM' in little-endian (shows as 4D 43 'MC' in hex dumps)

// MeshCore protocol identification
#define MESHCORE_MAGIC_BYTE       0x01  // Protocol version/magic combined
#define MESHCORE_PROTOCOL_VERSION 0x01  // Current protocol version
#define MAX_SUPPORTED_PROTOCOL_VERSION 0x01

// MeshCore Device Service UUID (custom 128-bit)
#define MESHCORE_SERVICE_UUID        "6E400020-B5A3-F393-E0A9-E50E24DCCA9E"

// Characteristic UUIDs
#define MESHCORE_PUBKEY_UUID         "6E400021-B5A3-F393-E0A9-E50E24DCCA9E"  // 32 bytes: Ed25519 public key
#define MESHCORE_DEVICE_INFO_UUID    "6E400022-B5A3-F393-E0A9-E50E24DCCA9E"  // Variable: device info
#define MESHCORE_SIGNATURE_UUID      "6E400023-B5A3-F393-E0A9-E50E24DCCA9E"  // 64 bytes: Ed25519 signature
#define MESHCORE_BATTERY_UUID        "6E400024-B5A3-F393-E0A9-E50E24DCCA9E"  // 1 byte: battery %

// Location source indicators
#define LOCATION_SOURCE_UNKNOWN 0
#define LOCATION_SOURCE_BLE     1
#define LOCATION_SOURCE_LORA    2

// Minimum RSSI to attempt connection (dBm)
#define MIN_RSSI_FOR_CONNECTION -80

// BLE neighbor timeout (milliseconds)
#define BLE_NEIGHBOR_TIMEOUT_MS (5 * 60 * 1000)  // 5 minutes

/**
 * BLE Manufacturer Data structure (26 bytes)
 * Advertised continuously to enable passive device discovery
 */
struct BLEManufacturerData {
    uint16_t manufacturer_id;  // [0-1]  0x434D (shows as '4D 43' = 'MC' in hex dumps)
    uint8_t  magic_byte;       // [2]    0x01 (MESHCORE_MAGIC_BYTE - protocol version)
    uint8_t  protocol_version; // [3]    0x01 (MESHCORE_PROTOCOL_VERSION)
    uint8_t  device_hash;      // [4]    First byte of pub_key (routing hash)
    uint8_t  flags;            // [5]    Device type (4 bits) + has_location (bit 4)
    int32_t  latitude;         // [6-9]  lat * 1E5 (meter accuracy, if has_location)
    int32_t  longitude;        // [10-13] lon * 1E5 (meter accuracy, if has_location)
    uint32_t timestamp;        // [14-17] Current RTC time
    uint16_t crc16;            // [18-19] CRC16 of bytes 0-17
    uint8_t  reserved[6];      // [20-25] Future use
} __attribute__((packed));

// Static assert to ensure struct is exactly 26 bytes
static_assert(sizeof(BLEManufacturerData) == 26, "BLEManufacturerData must be 26 bytes");

/**
 * Device Info structure stored in GATT characteristic
 * Read when connecting to unknown device
 */
struct BLEDeviceInfo {
    uint8_t  device_type;      // ADV_TYPE_CHAT, ADV_TYPE_SENSOR, etc.
    uint8_t  flags;            // Extended flags
    uint32_t timestamp;        // Advertisement timestamp
    char     name[20];         // Device name (NULL-terminated)
} __attribute__((packed));

/**
 * Flags byte layout in manufacturer data
 * Bits 0-3: Device Type (ADV_TYPE_*)
 * Bit 4:    Has Location
 * Bit 5:    Battery Low Warning
 * Bits 6-7: Reserved
 */
#define BLE_FLAG_DEVICE_TYPE_MASK  0x0F
#define BLE_FLAG_HAS_LOCATION      0x10
#define BLE_FLAG_BATTERY_LOW       0x20

// Helper macros
#define BLE_FLAGS_SET_TYPE(flags, type)  ((flags & ~BLE_FLAG_DEVICE_TYPE_MASK) | (type & BLE_FLAG_DEVICE_TYPE_MASK))
#define BLE_FLAGS_GET_TYPE(flags)        (flags & BLE_FLAG_DEVICE_TYPE_MASK)
#define BLE_FLAGS_HAS_LOCATION(flags)    ((flags & BLE_FLAG_HAS_LOCATION) != 0)

/**
 * Simple CRC16 calculation for manufacturer data validation
 */
inline uint16_t ble_crc16_calc(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}
