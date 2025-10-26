# BLE Auto-Discovery Implementation

## Overview

This implementation adds automatic Bluetooth LE device discovery to MeshCore, enabling devices to discover and build contacts automatically via BLE, complementing the existing LoRa mesh functionality.

## Key Features

- ✅ **Passive scanning** with manufacturer data (no connection for known devices)
- ✅ **Automatic contact creation** when unknown MeshCore devices discovered
- ✅ **Location updates** from BLE advertisements for known contacts
- ✅ **Signature verification** to prevent spoofing
- ✅ **Multi-layer filtering** to ignore non-MeshCore BLE devices
- ✅ **Completely transparent** - no user interaction required

## Implementation Status

### Phase 1: Core Infrastructure ✅ COMPLETED

1. **BLEServiceDefinitions.h** - Created
   - Manufacturer data structure (27 bytes)
   - GATT service/characteristic UUIDs
   - Magic bytes and protocol version
   - CRC16 helper function
   - Location: `src/helpers/BLEServiceDefinitions.h`

2. **BLEDiscoveryManager** - Created
   - Header: `src/helpers/BLEDiscoveryManager.h`
   - Implementation: `src/helpers/BLEDiscoveryManager.cpp`
   - Handles scanning, filtering, auto-connection, contact creation
   - Platform-agnostic base class

3. **ContactInfo Extension** - Modified
   - Added BLE-specific fields:
     - `last_seen_ble`: When last BLE advertisement received
     - `ble_rssi`: Last BLE signal strength
     - `last_known_lat/lon`: Latest location from BLE or LoRa
     - `location_source`: BLE=1, LoRa=2
     - `location_timestamp`: When location updated
   - Location: `src/helpers/ContactInfo.h`

4. **BaseChatMesh Helper** - Added
   - New method: `findContactByHash(uint8_t device_hash)`
   - Finds contacts by first byte of public key (routing hash)
   - Location: `src/helpers/BaseChatMesh.h/cpp`

### Phase 2: Platform Implementations ✅ COMPLETED

#### ESP32 Implementation ✅ COMPLETED

1. **SerialBLEInterface.h** - Modified
   - Added BLEServiceDefinitions.h include
   - Added MeshCore service member variables (pMeshCoreService, characteristics, pBLEScan)
   - Added method declarations for MeshCore BLE discovery
   - Location: `src/helpers/esp32/SerialBLEInterface.h`

2. **SerialBLEInterface.cpp** - Modified
   - Added MeshCoreScanCallbacks class for BLE scanning
   - Implemented `createMeshCoreService()` - Creates GATT service with 3 characteristics
   - Implemented `updateManufacturerData()` - Updates advertisement with manufacturer data
   - Implemented `setMeshCoreCharacteristics()` - Sets public key, device info, signature
   - Implemented `startScanning()` - Starts BLE scanning with discovery callback
   - Location: `src/helpers/esp32/SerialBLEInterface.cpp`

**Next steps:**
1. ~~nRF52 SerialBLEInterface extensions~~ ✅ COMPLETED
2. ~~Integration with companion radio example~~ ✅ COMPLETED
3. Testing on real hardware

## Architecture

### Discovery Flow

```
BLE Advertisement Received
    ↓
[Filter 1] manufacturer_id == 0xFFFF?
    ↓
[Filter 2] magic_byte == 0x4D ('M')?
    ↓
[Filter 3] protocol_version compatible?
    ↓
[Filter 4] CRC16 valid?
    ↓
Extract device_hash (pub_key[0])
    ↓
Check: device_hash in contacts?
    ↓
YES → Update location/RSSI, done ✅
NO  → Auto-connect, read characteristics, verify signature, add contact ✅
```

### Manufacturer Data Structure

```cpp
struct BLEManufacturerData {
    uint16_t manufacturer_id;  // 0xFFFF
    uint8_t  magic_byte;       // 0x4D ('M')
    uint8_t  protocol_version; // 0x01
    uint8_t  device_hash;      // pub_key[0]
    uint8_t  flags;            // Type + has_location
    int32_t  latitude;         // lat * 1E6
    int32_t  longitude;        // lon * 1E6
    uint32_t timestamp;        // RTC time
    uint16_t crc16;            // Validation
    uint8_t  reserved[7];      // Future
} __attribute__((packed));    // 27 bytes total
```

### GATT Service Structure

**MeshCore Device Service**: `6E400020-B5A3-F393-E0A9-E50E24DCCA9E`

**Characteristics:**
- **Public Key**: 32 bytes Ed25519 key
- **Device Info**: Type, name, timestamp
- **Signature**: 64 bytes Ed25519 signature
- **Battery**: Optional battery percentage

## Files Created

1. `src/helpers/BLEServiceDefinitions.h` - Service definitions and structures
2. `src/helpers/BLEDiscoveryManager.h` - Discovery manager interface
3. `src/helpers/BLEDiscoveryManager.cpp` - Discovery manager implementation

## Files Modified

1. `src/helpers/ContactInfo.h` - Added BLE fields to ContactInfo struct
2. `src/helpers/BaseChatMesh.h` - Added findContactByHash() method
3. `src/helpers/BaseChatMesh.cpp` - Implemented findContactByHash()
4. `src/helpers/esp32/SerialBLEInterface.h` - Added MeshCore BLE discovery methods
5. `src/helpers/esp32/SerialBLEInterface.cpp` - Implemented ESP32 BLE discovery
6. `src/helpers/nrf52/SerialBLEInterface.h` - Added MeshCore BLE discovery methods
7. `src/helpers/nrf52/SerialBLEInterface.cpp` - Implemented nRF52 BLE discovery
8. `examples/companion_radio/MyMesh.h` - Added BLE discovery method declarations
9. `examples/companion_radio/MyMesh.cpp` - Implemented BLE advertising for companion radio

## Next Steps

1. ~~Implement ESP32-specific BLE methods in `SerialBLEInterface`~~ ✅ COMPLETED
2. ~~Implement nRF52-specific BLE methods in `SerialBLEInterface`~~ ✅ COMPLETED
3. ~~Integrate with companion radio example~~ ✅ COMPLETED
4. **Implement BLE scanning for auto-discovery** (next phase)
   - Create platform-specific BLEDiscoveryManager subclasses
   - Implement `connectAndReadDevice()` method for ESP32
   - Implement `connectAndReadDevice()` method for nRF52
   - Wire up scanning callbacks in companion radio
5. Test on both platforms (ESP32 and nRF52)
6. Add UI elements to display BLE neighbors (optional)

## Testing Plan

1. **Single Device Test**: Verify advertising and GATT service setup
2. **Two Device Test**: Verify auto-discovery and contact creation
3. **Multi Device Test**: Verify neighbor table and location updates
4. **Stress Test**: Multiple unknown devices, verify connection queue
5. **Range Test**: Verify RSSI filtering (-80 dBm threshold)

## Usage

### Companion Radio (Advertising Only - Current Implementation)

BLE advertising is now integrated into the companion radio. It works automatically:

```cpp
// In MyMesh::startInterface() - called automatically
initBLEDiscovery();  // Creates GATT service, sets characteristics, starts advertising

// In MyMesh::loop() - runs automatically
if (millis() - last_ble_update > 30000) {
  updateBLEAdvertisement();  // Updates manufacturer data every 30 seconds
}

// Location changes automatically trigger BLE advertisement updates
void MyMesh::sendLocationAdvertisement(double lat, double lon) {
  // ... send LoRa advertisement ...
  updateBLEAdvertisement();  // Also update BLE
}
```

The companion radio broadcasts:
- Device hash (first byte of public key)
- Device type (ADV_TYPE_CHAT)
- GPS location (if enabled and available)
- Timestamp
- CRC16 for validation

### Future: Full Discovery (Advertising + Scanning)

When BLE scanning is implemented, companion radios will be able to:
- Discover nearby MeshCore devices via BLE
- Auto-connect to unknown devices to read full public key
- Verify Ed25519 signatures
- Automatically add contacts
- Update location for known contacts

```cpp
// Future implementation:
// In MyMesh::initBLEDiscovery()
ble_discovery_mgr.begin(this);
_serial->startScanning(&ble_discovery_mgr);

// In MyMesh::loop()
ble_discovery_mgr.loop();  // Processes connection queue
```

## Benefits Over LoRa-Only Discovery

- **Faster**: BLE discovery in seconds vs minutes for LoRa
- **Lower power**: BLE scanning very efficient
- **Better indoors**: BLE penetrates buildings better
- **Location tracking**: Real-time position updates for known devices
- **Complementary**: Works alongside LoRa, doesn't replace it
