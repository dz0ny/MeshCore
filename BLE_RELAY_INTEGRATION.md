# BLE Advertising Integration for Relay/Repeater Nodes

## Overview

This document provides step-by-step instructions for adding BLE advertising (advertising ONLY, no scanning) to relay/repeater nodes in MeshCore.

### Why Relays Only Advertise

Relay/repeater nodes serve a critical infrastructure role in the MeshCore mesh network. Their BLE behavior differs from companion radios in important ways:

**Infrastructure Role:**
- Relays provide mesh backbone connectivity
- Their primary purpose is LoRa packet forwarding
- They should be discoverable by companion radios via BLE
- They do NOT need to discover other devices via BLE (mesh handles that)

**Power Considerations:**
- BLE advertising consumes ~10-15mA continuous power
- BLE scanning consumes ~20-30mA continuous power
- Solar/battery-powered relays benefit from advertising-only mode
- Advertising provides visibility without scanning overhead

**Architecture Benefits:**
- Companion radios scan for and connect to relays
- Relays simply advertise their presence and location
- Clear separation: infrastructure advertises, clients scan
- Prevents redundant discovery (LoRa already handles mesh topology)

---

## What to Modify

The BLE advertising integration requires changes to three files in the `simple_repeater` example:

1. **`examples/simple_repeater/MyMesh.h`** - Add BLE manager member and platform detection
2. **`examples/simple_repeater/MyMesh.cpp`** - Initialize and update BLE advertisements
3. **`examples/simple_repeater/main.cpp`** - (Optional) Start advertising early in boot sequence

---

## Implementation Steps

### Step 1: Add Platform-Specific BLE Headers and Manager (MyMesh.h)

First, add conditional includes for BLE support based on platform. Only ESP32 and nRF52 platforms support BLE advertising natively.

**Location:** After the existing includes, around line 26

```cpp
// Add after existing includes
#include <helpers/AdvertDataHelpers.h>

// Platform-specific BLE advertising support
#if defined(ESP32)
  #include <BLE2902.h>
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
  #define BLE_ADVERTISING_SUPPORTED
#elif defined(NRF52_PLATFORM)
  #include <bluefruit.h>
  #define BLE_ADVERTISING_SUPPORTED
#endif

#ifdef BLE_ADVERTISING_SUPPORTED
  #include <helpers/BLEServiceDefinitions.h>
#endif
```

**Location:** Inside the `MyMesh` class private section, around line 102 (after the bridge declaration)

```cpp
#if defined(WITH_RS232_BRIDGE)
  RS232Bridge bridge;
#elif defined(WITH_ESPNOW_BRIDGE)
  ESPNowBridge bridge;
#endif

// Add BLE advertising support for relay discovery
#ifdef BLE_ADVERTISING_SUPPORTED
  BLEServer* ble_server;              // Platform-specific server
  BLEAdvertising* ble_advertising;    // Platform-specific advertising
  bool ble_initialized;
  unsigned long last_ble_update;

  void initBLEAdvertising();
  void updateBLEAdvertisement();
#endif
```

---

### Step 2: Initialize BLE Service in begin() (MyMesh.cpp)

Add BLE initialization at the end of the `begin()` method. This creates the MeshCore service, sets characteristics with relay information, and starts advertising.

**Location:** In `MyMesh::begin()`, after GPS initialization (around line 679)

```cpp
void MyMesh::begin(FILESYSTEM *fs) {
  mesh::Mesh::begin();
  _fs = fs;
  // ... existing code ...

#if ENV_INCLUDE_GPS == 1
  applyGpsPrefs();
#endif

  // Initialize BLE advertising for relay discovery
#ifdef BLE_ADVERTISING_SUPPORTED
  ble_initialized = false;
  last_ble_update = 0;
  initBLEAdvertising();
#endif
}
```

**Location:** Add new methods at the end of MyMesh.cpp (around line 912)

```cpp
#ifdef BLE_ADVERTISING_SUPPORTED

void MyMesh::initBLEAdvertising() {
  MESH_DEBUG_PRINTLN("BLE: Initializing advertising (relay mode)");

  // Generate BLE device name from repeater name
  char ble_name[32];
  snprintf(ble_name, sizeof(ble_name), "MC-%.8s", _prefs.node_name);

#if defined(ESP32)
  // ESP32 BLE initialization
  BLEDevice::init(ble_name);
  ble_server = BLEDevice::createServer();

  // Create MeshCore service
  BLEService* service = ble_server->createService(MESHCORE_SERVICE_UUID);

  // Public Key Characteristic (Read-only, 32 bytes)
  BLECharacteristic* pubkey_char = service->createCharacteristic(
    MESHCORE_PUBKEY_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  pubkey_char->setValue(self_id.pub_key, PUB_KEY_SIZE);

  // Device Info Characteristic (Read-only)
  BLEDeviceInfo device_info;
  device_info.device_type = ADV_TYPE_REPEATER;
  device_info.flags = 0;
  device_info.timestamp = getRTCClock()->getCurrentTime();
  strncpy(device_info.name, _prefs.node_name, sizeof(device_info.name) - 1);
  device_info.name[sizeof(device_info.name) - 1] = '\0';

  BLECharacteristic* info_char = service->createCharacteristic(
    MESHCORE_DEVICE_INFO_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  info_char->setValue((uint8_t*)&device_info, sizeof(BLEDeviceInfo));

  // Signature Characteristic (Read-only, 64 bytes)
  // Sign: pubkey || timestamp || device_info
  uint8_t msg_to_sign[32 + 4 + sizeof(BLEDeviceInfo)];
  memcpy(msg_to_sign, self_id.pub_key, 32);
  memcpy(msg_to_sign + 32, &device_info.timestamp, 4);
  memcpy(msg_to_sign + 36, &device_info, sizeof(BLEDeviceInfo));

  uint8_t signature[64];
  Ed25519::sign(signature, self_id.prv_key, self_id.pub_key, msg_to_sign, sizeof(msg_to_sign));

  BLECharacteristic* sig_char = service->createCharacteristic(
    MESHCORE_SIGNATURE_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  sig_char->setValue(signature, 64);

  // Battery Characteristic (Read-only, 1 byte percentage)
  BLECharacteristic* batt_char = service->createCharacteristic(
    MESHCORE_BATTERY_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  uint8_t batt_pct = (uint8_t)((board.getBattMilliVolts() - 3300) * 100 / (4200 - 3300));
  batt_char->setValue(&batt_pct, 1);

  service->start();

  // Configure advertising
  ble_advertising = BLEDevice::getAdvertising();
  ble_advertising->addServiceUUID(MESHCORE_SERVICE_UUID);
  ble_advertising->setScanResponse(true);

#elif defined(NRF52_PLATFORM)
  // nRF52 BLE initialization
  Bluefruit.begin();
  Bluefruit.setTxPower(4); // 4dBm for good range without excessive power
  Bluefruit.setName(ble_name);

  // Create MeshCore service (similar to ESP32 implementation)
  // Note: nRF52 implementation details omitted for brevity
  // Follow similar pattern as ESP32 above

#endif

  // Set manufacturer data with relay info
  updateBLEAdvertisement();

  // Start advertising
#if defined(ESP32)
  ble_advertising->start();
#elif defined(NRF52_PLATFORM)
  Bluefruit.Advertising.start();
#endif

  ble_initialized = true;
  MESH_DEBUG_PRINTLN("BLE: Advertising started (relay visible to companions)");
}

void MyMesh::updateBLEAdvertisement() {
  if (!ble_initialized) return;

  // Build manufacturer data structure
  BLEManufacturerData mfg_data;
  mfg_data.manufacturer_id = MESHCORE_MANUFACTURER_ID;
  mfg_data.magic_byte = MESHCORE_MAGIC_BYTE;
  mfg_data.protocol_version = MESHCORE_PROTOCOL_VERSION;
  mfg_data.device_hash = self_id.pub_key[0]; // First byte of public key

  // Set device type and location flag
  mfg_data.flags = BLE_FLAGS_SET_TYPE(0, ADV_TYPE_REPEATER);

  // Include location if GPS equipped and has valid fix
#if ENV_INCLUDE_GPS == 1
  if (_prefs.gps_enabled && sensors.node_lat != 0.0 && sensors.node_lon != 0.0) {
    mfg_data.flags |= BLE_FLAG_HAS_LOCATION;
    mfg_data.latitude = (int32_t)(sensors.node_lat * 1000000.0);
    mfg_data.longitude = (int32_t)(sensors.node_lon * 1000000.0);
  } else
#endif
  {
    // Use static prefs location (if configured)
    if (_prefs.node_lat != 0.0 || _prefs.node_lon != 0.0) {
      mfg_data.flags |= BLE_FLAG_HAS_LOCATION;
      mfg_data.latitude = (int32_t)(_prefs.node_lat * 1000000.0);
      mfg_data.longitude = (int32_t)(_prefs.node_lon * 1000000.0);
    } else {
      mfg_data.latitude = 0;
      mfg_data.longitude = 0;
    }
  }

  // Set current timestamp
  mfg_data.timestamp = getRTCClock()->getCurrentTime();

  // Check battery level for low warning
  uint16_t batt_mv = board.getBattMilliVolts();
  if (batt_mv < 3400) { // Below 3.4V
    mfg_data.flags |= BLE_FLAG_BATTERY_LOW;
  }

  // Calculate CRC16 over first 18 bytes
  mfg_data.crc16 = ble_crc16_calc((const uint8_t*)&mfg_data, 18);

  // Zero reserved bytes
  memset(mfg_data.reserved, 0, sizeof(mfg_data.reserved));

#if defined(ESP32)
  // Update ESP32 manufacturer data
  BLEAdvertisementData adv_data;
  adv_data.setManufacturerData(std::string((char*)&mfg_data, sizeof(mfg_data)));
  ble_advertising->setAdvertisementData(adv_data);

#elif defined(NRF52_PLATFORM)
  // Update nRF52 manufacturer data
  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addManufacturerData(&mfg_data, sizeof(mfg_data));

#endif

  last_ble_update = _ms->getMillis();

  MESH_DEBUG_PRINTLN("BLE: Advertisement updated (hash=%02X, loc=%s)",
                     mfg_data.device_hash,
                     (mfg_data.flags & BLE_FLAG_HAS_LOCATION) ? "yes" : "no");
}

#endif // BLE_ADVERTISING_SUPPORTED
```

---

### Step 3: Update BLE Advertisement in loop() (MyMesh.cpp)

For mobile relays (GPS-equipped) or relays with changing battery status, periodically update the BLE advertisement to reflect current location and status.

**Location:** In `MyMesh::loop()`, before the final closing brace (around line 910)

```cpp
void MyMesh::loop() {
#ifdef WITH_BRIDGE
  bridge.loop();
#endif

  mesh::Mesh::loop();

  // ... existing timer code for adverts, radio params, etc. ...

  // is pending dirty contacts write needed?
  if (dirty_contacts_expiry && millisHasNowPassed(dirty_contacts_expiry)) {
    acl.save(_fs);
    dirty_contacts_expiry = 0;
  }

  // Update BLE advertisement periodically (for mobile relays or battery changes)
#ifdef BLE_ADVERTISING_SUPPORTED
  // Update every 60 seconds
  if (ble_initialized && _ms->getMillis() - last_ble_update > 60000) {
    updateBLEAdvertisement();
  }
#endif
}
```

---

### Step 4: (Optional) Early BLE Advertising in main.cpp

For faster discovery after boot, you can optionally start BLE advertising early in the setup sequence.

**Location:** In `main.cpp` `setup()`, after mesh initialization (around line 82)

```cpp
void setup() {
  // ... existing setup code ...

  the_mesh.begin(fs);

#ifdef DISPLAY_CLASS
  ui_task.begin(the_mesh.getNodePrefs(), FIRMWARE_BUILD_DATE, FIRMWARE_VERSION);
#endif

  // Note: BLE advertising is automatically initialized in the_mesh.begin()
  // No additional code needed here unless you want custom timing

  // send out initial Advertisement to the mesh
  the_mesh.sendSelfAdvertisement(16000);
}
```

---

## Manufacturer Data Content

The BLE manufacturer data advertised by relays includes the following information (27 bytes total):

```cpp
struct BLEManufacturerData {
    uint16_t manufacturer_id;  // 0xFFFF (MESHCORE_MANUFACTURER_ID)
    uint8_t  magic_byte;       // 0x4D (MESHCORE_MAGIC_BYTE = 'M')
    uint8_t  protocol_version; // 0x01 (MESHCORE_PROTOCOL_VERSION)
    uint8_t  device_hash;      // First byte of pub_key (for routing)
    uint8_t  flags;            // Device type (4 bits) + has_location (bit 4) + battery_low (bit 5)
    int32_t  latitude;         // lat * 1E6 (if has_location flag set)
    int32_t  longitude;        // lon * 1E6 (if has_location flag set)
    uint32_t timestamp;        // Current RTC time (Unix timestamp)
    uint16_t crc16;            // CRC16 of bytes 0-17 (validation)
    uint8_t  reserved[7];      // Reserved for future use
} __attribute__((packed));
```

### Flags Byte Layout:
- **Bits 0-3:** Device Type (ADV_TYPE_REPEATER = 2)
- **Bit 4:** Has Location (1 if lat/lon are valid)
- **Bit 5:** Battery Low Warning (1 if < 3.4V)
- **Bits 6-7:** Reserved

### Device Types:
```cpp
#define ADV_TYPE_NONE         0
#define ADV_TYPE_CHAT         1
#define ADV_TYPE_REPEATER     2  // Use this for relays
#define ADV_TYPE_ROOM         3
#define ADV_TYPE_SENSOR       4
```

---

## Benefits

Adding BLE advertising to relay nodes provides several advantages:

### Mesh Topology Visualization
- Companion radios can see nearby relay infrastructure via BLE
- Map displays can show relay coverage areas
- Users understand mesh network structure visually

### Quick Discovery
- BLE discovery is near-instantaneous (vs. waiting for LoRa adverts)
- Companion radios can find nearby relays within 1-2 seconds
- No need to wait for mesh advertisement intervals

### Coverage Analysis
- Users can see relay signal strength (BLE RSSI)
- Helps identify coverage gaps or weak spots
- Assists in optimal relay placement for best coverage

### Mobile Relay Support
- GPS-equipped relays advertise current location via BLE
- Companion radios track mobile relay movements
- Useful for vehicle-mounted or portable relays

### Battery Monitoring
- Low battery warning flag alerts nearby users
- Solar relay status visible to maintenance personnel
- Predictive maintenance for relay infrastructure

---

## Special Considerations

### Fixed vs. Mobile Relays

**Fixed Relays** (no GPS):
- Use static location from `_prefs.node_lat` and `_prefs.node_lon`
- Set once during configuration, advertised continuously
- BLE advertisement updates every 60 seconds (only timestamp/battery changes)

**Mobile Relays** (GPS-equipped):
- Use dynamic location from `sensors.node_lat` and `sensors.node_lon`
- Advertisement updates when location changes (every 60 seconds in loop)
- Useful for vehicle-mounted repeaters or temporary deployments

Example configuration for fixed relay:
```cpp
// In CLI or via serial:
set name fixed-relay-1
set lat 37.774929
set lon -122.419418
```

### Solar-Powered Relays: Power Impact

BLE advertising adds continuous power consumption. Here's the breakdown:

**Power Consumption:**
- **BLE Advertising:** ~10-15mA continuous (ESP32/nRF52)
- **LoRa Idle:** ~1-2mA
- **LoRa TX (20dBm):** ~120mA for 1-2 seconds per packet

**Impact Analysis:**

For a solar relay with 3000mAh battery:
- **Without BLE:** ~50mAh/day typical (mostly LoRa TX)
- **With BLE:** ~300-400mAh/day (240-360mAh BLE + 50mAh LoRa)
- **Solar requirement:** ~20-30mA average charging (12 hours sun)

**Recommendations:**

1. **Well-Powered Relays** (AC, large solar, vehicle):
   - Enable BLE advertising without concern
   - Provides maximum visibility and utility

2. **Battery-Only Relays** (no solar):
   - Consider disabling BLE to extend runtime
   - Or use conditional advertising (only when battery > 50%)

3. **Small Solar Relays** (< 5W panel):
   - Test power budget before deploying
   - May need to disable BLE or reduce LoRa TX power
   - Use `#ifdef BLE_ADVERTISING_SUPPORTED` to compile out if needed

4. **Large Solar Relays** (> 10W panel):
   - BLE power impact is negligible
   - Full BLE + LoRa operation is sustainable

### Advertisement Interval Tuning

The BLE advertisement interval affects both power consumption and discoverability:

**Current Implementation:**
- **Interval:** 100ms (default for most BLE stacks)
- **Update Rate:** 60 seconds (location/battery refresh)

**Power Optimization Options:**

```cpp
// In initBLEAdvertising(), after creating advertising:

#if defined(ESP32)
  // Option 1: Default (100ms) - Best discoverability, higher power
  ble_advertising->setMinInterval(0x20);  // 20ms
  ble_advertising->setMaxInterval(0xA0);  // 100ms

  // Option 2: Balanced (300ms) - Good discoverability, moderate power
  ble_advertising->setMinInterval(0x140); // 200ms
  ble_advertising->setMaxInterval(0x1E0); // 300ms

  // Option 3: Power Saver (1000ms) - Slower discovery, lowest power
  ble_advertising->setMinInterval(0x640); // 1000ms
  ble_advertising->setMaxInterval(0x640); // 1000ms

#elif defined(NRF52_PLATFORM)
  // nRF52 uses different units (0.625ms steps)
  Bluefruit.Advertising.setInterval(32, 160);  // 20-100ms (default)
  // or
  Bluefruit.Advertising.setInterval(320, 480); // 200-300ms (balanced)
  // or
  Bluefruit.Advertising.setInterval(1600, 1600); // 1000ms (power saver)
#endif
```

**Choosing Interval:**
- **High Traffic Areas:** 100ms (fast discovery, users walking by)
- **Remote Relays:** 300ms (balanced)
- **Battery Critical:** 1000ms (minimal power, slow discovery)

---

## Testing and Validation

After implementing BLE advertising, test with these steps:

### 1. Verify BLE Service Creation
```
// Serial console should show:
BLE: Initializing advertising (relay mode)
BLE: Advertising started (relay visible to companions)
```

### 2. Scan for Device with nRF Connect App
- Install nRF Connect (iOS/Android)
- Look for device named "MC-xxxxxxxx"
- Should see MeshCore service UUID
- Check manufacturer data (27 bytes)

### 3. Verify Manufacturer Data
- First 2 bytes: `FF FF` (manufacturer ID)
- Byte 3: `4D` (magic byte 'M')
- Byte 4: `01` (protocol version)
- Byte 5: First byte of your relay's public key
- Byte 6: `02` in bits 0-3 (ADV_TYPE_REPEATER)

### 4. Test with Companion Radio
- Companion radio should discover relay within 5 seconds
- Relay should appear in BLE neighbors list
- Location should match (if configured)
- RSSI should be reasonable (-40 to -80 dBm typical)

### 5. Verify Location Updates (GPS Relays)
- Move relay with GPS enabled
- Within 60 seconds, new location should appear in BLE advertisement
- Companion radio should see updated position

---

## Important Notes

### DO NOT Call startScanning()

**CRITICAL:** Relay nodes should NEVER call any BLE scanning functions. Scanning is power-intensive and unnecessary for infrastructure nodes.

```cpp
// WRONG - Do not do this in relay code:
ble_discovery.startScanning();  // NO!

// CORRECT - Only advertise:
initBLEAdvertising();           // YES
updateBLEAdvertisement();       // YES
```

### Platform Support

BLE advertising is only available on:
- **ESP32** (all variants: ESP32, ESP32-S3, ESP32-C3, etc.)
- **nRF52** (nRF52832, nRF52840)

Other platforms (RP2040, STM32) do not have native BLE and will skip BLE code automatically via `#ifdef BLE_ADVERTISING_SUPPORTED`.

### Security Considerations

The BLE service includes an Ed25519 signature to prevent spoofing:
- Signature covers: `pubkey || timestamp || device_info`
- Companion radios verify signature before adding contact
- Invalid signatures are rejected (prevents fake relays)

### Fallback Behavior

If BLE is not supported or fails to initialize:
- Relay continues normal LoRa operation
- Discovery still works via LoRa advertisements
- BLE is supplementary, not required

---

## Complete Example

Here's a complete minimal example for a solar-powered fixed relay with BLE:

```cpp
// MyMesh.h - Add at end of private section
#ifdef BLE_ADVERTISING_SUPPORTED
  BLEServer* ble_server;
  BLEAdvertising* ble_advertising;
  bool ble_initialized;
  unsigned long last_ble_update;

  void initBLEAdvertising();
  void updateBLEAdvertisement();
#endif

// MyMesh.cpp - begin() method
void MyMesh::begin(FILESYSTEM *fs) {
  mesh::Mesh::begin();
  _fs = fs;
  _cli.loadPrefs(_fs);
  acl.load(_fs);

  radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
  radio_set_tx_power(_prefs.tx_power_dbm);

  updateAdvertTimer();
  updateFloodAdvertTimer();

#ifdef BLE_ADVERTISING_SUPPORTED
  ble_initialized = false;
  last_ble_update = 0;
  initBLEAdvertising();
#endif
}

// MyMesh.cpp - loop() method
void MyMesh::loop() {
  mesh::Mesh::loop();

  // ... existing timers ...

#ifdef BLE_ADVERTISING_SUPPORTED
  if (ble_initialized && _ms->getMillis() - last_ble_update > 60000) {
    updateBLEAdvertisement();
  }
#endif
}
```

---

## Summary Checklist

- [ ] Added platform-specific BLE includes to MyMesh.h
- [ ] Added BLE manager member variables to MyMesh class
- [ ] Implemented `initBLEAdvertising()` method
- [ ] Implemented `updateBLEAdvertisement()` method
- [ ] Called `initBLEAdvertising()` in `begin()`
- [ ] Added BLE update timer in `loop()`
- [ ] Set device type to `ADV_TYPE_REPEATER`
- [ ] Verified NO scanning code is present
- [ ] Tested with nRF Connect app
- [ ] Tested with companion radio
- [ ] Measured power consumption (if battery/solar)
- [ ] Adjusted advertisement interval if needed

---

## Troubleshooting

### BLE Not Starting
- Check platform is ESP32 or nRF52
- Verify `BLE_ADVERTISING_SUPPORTED` is defined
- Check serial console for initialization errors
- Ensure BLE libraries are installed (platformio.ini)

### Companion Radio Not Discovering Relay
- Verify manufacturer data format (27 bytes)
- Check CRC16 calculation is correct
- Ensure device_hash matches first byte of pubkey
- Verify BLE advertising is actually running (use nRF Connect)

### High Power Consumption
- Increase advertisement interval (100ms → 300ms → 1000ms)
- Verify only advertising, not scanning
- Check for multiple BLE init calls (should only init once)
- Consider disabling BLE on battery-critical relays

### Location Not Updating
- Check GPS is enabled (`_prefs.gps_enabled`)
- Verify GPS has valid fix (`sensors.node_lat != 0`)
- Ensure 60-second update timer is running
- Check BLE_FLAG_HAS_LOCATION bit is set in flags

---

## Future Enhancements

Potential improvements for future versions:

1. **Conditional Advertising:** Enable/disable BLE based on battery level
2. **Dynamic Interval:** Adjust advertisement interval based on traffic/power
3. **Mesh Metrics:** Include hop count, airtime usage in manufacturer data
4. **OTA via BLE:** Allow firmware updates via BLE (for accessible relays)
5. **Remote Configuration:** Read/write relay settings via BLE characteristics

---

## References

- BLE Discovery Infrastructure: `src/helpers/BLEDiscoveryManager.h`
- BLE Service Definitions: `src/helpers/BLEServiceDefinitions.h`
- Advertisement Data Helpers: `src/helpers/AdvertDataHelpers.h`
- Companion Radio Example: `examples/companion_radio/MyMesh.cpp`
- Simple Repeater Example: `examples/simple_repeater/`

---

**Document Version:** 1.0
**Last Updated:** 2025-10-20
**Author:** MeshCore Development Team
