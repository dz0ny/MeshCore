# ESP32 BLE Client Implementation for Auto-Discovery

## Overview

This document describes the ESP32-specific implementation of BLE client functionality for MeshCore auto-discovery. The system automatically connects to unknown MeshCore devices to read their full public key and verify their signature.

## Implementation Details

### Files Created

1. **`src/helpers/esp32/ESP32BLEDiscoveryManager.h`**
   - ESP32-specific subclass of `BLEDiscoveryManager`
   - Declares `connectAndReadDevice()` method override
   - Manages BLE client lifecycle

2. **`src/helpers/esp32/ESP32BLEDiscoveryManager.cpp`**
   - Implements BLE client connection logic
   - Handles GATT service discovery
   - Reads characteristics (public key, device info, signature)
   - Verifies device hash and creates contacts

### Key Features

#### 1. Connection Management
- Creates and manages ESP32 BLEClient instance
- Handles connection/disconnection lifecycle
- Includes error handling with try-catch blocks
- Automatic cleanup on failure

#### 2. GATT Service Discovery
- Discovers MeshCore service (UUID: `MESHCORE_SERVICE_UUID`)
- Locates three required characteristics:
  - Public Key (32 bytes) - `MESHCORE_PUBKEY_UUID`
  - Device Info (struct) - `MESHCORE_DEVICE_INFO_UUID`
  - Signature (64 bytes) - `MESHCORE_SIGNATURE_UUID`

#### 3. Data Validation
- Verifies characteristic lengths
- Validates device hash matches advertisement
- Checks signature via Ed25519 (in base class)

#### 4. Contact Creation
- Calls `createContactFromBLE()` from base class
- Signature verification happens automatically
- Contact saved to mesh database if valid

### Usage Example

```cpp
#include "esp32/ESP32BLEDiscoveryManager.h"

// In your mesh class (e.g., MyMesh.cpp)
ESP32BLEDiscoveryManager ble_discovery;

void MyMesh::initBLEDiscovery() {
  MESH_DEBUG_PRINTLN("Initializing BLE discovery (advertising + scanning)");

  // Cast serial interface to SerialBLEInterface
  SerialBLEInterface* ble_serial = static_cast<SerialBLEInterface*>(_serial);

  // Create MeshCore GATT service with characteristics
  ble_serial->createMeshCoreService();

  // Build device info for GATT characteristics
  BLEDeviceInfo device_info;
  device_info.device_type = ADV_TYPE_CHAT;
  device_info.flags = 0;
  device_info.timestamp = getRTCClock()->getCurrentTime();
  strncpy(device_info.name, _prefs.node_name, sizeof(device_info.name) - 1);
  device_info.name[sizeof(device_info.name) - 1] = '\0';

  // Sign the data: pubkey || timestamp || device_info
  uint8_t signature[64];
  uint8_t msg_to_sign[32 + 4 + sizeof(BLEDeviceInfo)];
  memcpy(msg_to_sign, self_id.pub_key, 32);
  memcpy(msg_to_sign + 32, &device_info.timestamp, 4);
  memcpy(msg_to_sign + 36, &device_info, sizeof(BLEDeviceInfo));
  Ed25519::sign(signature, getPrivateKey(), self_id.pub_key, msg_to_sign, sizeof(msg_to_sign));

  // Set GATT characteristics
  ble_serial->setMeshCoreCharacteristics(self_id.pub_key, &device_info, signature);

  // Initialize discovery manager
  ble_discovery.begin(this);  // 'this' is the BaseChatMesh instance

  // Start BLE scanning (will call ble_discovery.onAdvertisementReceived)
  ble_serial->startScanning(&ble_discovery);

  // Update advertisement data
  updateBLEAdvertisement();

  MESH_DEBUG_PRINTLN("BLE discovery initialized ✓");
}

void MyMesh::loop() {
  BaseChatMesh::loop();

  // Process discovery events (connection queue cleanup, etc.)
  ble_discovery.loop();

  // Update BLE advertisement every 30 seconds
  if (millis() - last_ble_update > 30000) {
    updateBLEAdvertisement();
    last_ble_update = millis();
  }
}
```

### Connection Flow

1. **Advertisement Received**
   - `onAdvertisementReceived()` called by BLE scan callback
   - Validates manufacturer data structure
   - Checks if device is already known

2. **Unknown Device Detected**
   - RSSI check (must be > -80 dBm)
   - Cooldown check (10 seconds between attempts)
   - Sets `is_connecting = true`

3. **BLE Client Connection**
   - `connectAndReadDevice()` called
   - Creates BLE client if needed
   - Connects to device by MAC address
   - Timeout: handled by ESP32 BLE stack

4. **Service Discovery**
   - Discovers MeshCore GATT service
   - Gets references to three characteristics

5. **Data Reading**
   - Reads public key (32 bytes)
   - Verifies device hash matches
   - Reads device info (struct)
   - Reads signature (64 bytes)

6. **Disconnection**
   - Disconnects from device (data collection complete)
   - Frees BLE resources

7. **Contact Creation**
   - Calls `createContactFromBLE()`
   - Verifies Ed25519 signature
   - Adds contact to database if valid
   - Saves to storage automatically

### Error Handling

The implementation handles various error conditions:

- **NULL pointer checks**: Validates all input parameters
- **Connection failures**: Returns false if connection fails
- **Service not found**: Handles missing MeshCore service
- **Missing characteristics**: Validates all three characteristics exist
- **Invalid data lengths**: Checks characteristic sizes
- **Hash mismatch**: Verifies device hash matches advertisement
- **Signature verification**: Handled in base class
- **Exceptions**: Try-catch blocks for ESP32 BLE exceptions

### Debug Logging

Enable debug logging by defining `BLE_DEBUG_LOGGING`:

```cpp
#define BLE_DEBUG_LOGGING 1
```

Debug output includes:
- Connection attempts and status
- Service/characteristic discovery
- Data read operations
- Validation results
- Contact creation status

### Platform-Specific Notes

#### ESP32
- Uses ESP32 Arduino BLE library
- BLEClient manages connections
- BLERemoteService for service discovery
- BLERemoteCharacteristic for reading data
- Exception handling for BLE errors

#### Memory Management
- Single BLE client instance (reused for all connections)
- Automatic cleanup on errors
- Disconnects immediately after reading data
- Minimal memory footprint

### Security Considerations

1. **Signature Verification**
   - All contacts verified via Ed25519 signature
   - Prevents spoofing/impersonation attacks
   - Device hash validated against public key

2. **RSSI Threshold**
   - Only connects to nearby devices (> -80 dBm)
   - Prevents wasting resources on distant devices

3. **Connection Cooldown**
   - 10 second cooldown between connection attempts
   - Prevents rapid reconnection loops
   - Reduces power consumption

4. **CRC Validation**
   - Manufacturer data validated via CRC16
   - Filters out corrupted/invalid advertisements

### Performance Characteristics

- **Connection Time**: ~2-5 seconds typical
- **Data Transfer**: < 100 bytes total
- **Power Impact**: Minimal (brief connections only)
- **Cooldown Period**: 10 seconds between attempts
- **Concurrent Connections**: 1 at a time

### Testing Recommendations

1. **Basic Connectivity**
   - Test connection to known MeshCore device
   - Verify service/characteristic discovery
   - Validate data reading

2. **Error Cases**
   - Test with non-MeshCore BLE device
   - Test with device out of range
   - Test with invalid signature

3. **Edge Cases**
   - Multiple devices advertising simultaneously
   - Rapid connection/disconnection
   - Low RSSI scenarios

4. **Integration**
   - Verify contacts appear in mesh database
   - Check signature verification
   - Validate storage persistence

### Future Enhancements

- Connection timeout configuration
- Retry logic for failed connections
- Connection queue for multiple unknown devices
- Battery level reading (optional characteristic)
- Bonding/pairing support (if needed)

## Conclusion

The ESP32 BLE client implementation provides robust auto-discovery functionality for MeshCore devices. It handles connection management, data validation, and contact creation seamlessly, with comprehensive error handling and security verification.
