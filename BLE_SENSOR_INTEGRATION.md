# BLE Advertising Integration Guide for Sensor Nodes

## Overview

This guide explains how to add BLE advertising to sensor nodes in MeshCore. Sensor nodes only **advertise** their presence via BLE - they do not scan for other devices.

### Why Sensors Only Advertise

1. **Power Efficiency**: BLE advertising consumes significantly less power than scanning
2. **Simplified Logic**: Sensors don't need to discover other devices - they only need to be discovered
3. **Complementary to LoRa**: Works alongside existing LoRa mesh functionality
4. **Faster Discovery**: Companion radios can discover sensor locations via BLE much faster than waiting for LoRa advertisements

### How It Works

```
┌─────────────┐          BLE Advertisement          ┌──────────────────┐
│             │  ──────────────────────────────>    │                  │
│  Sensor     │     (Device ID, Location, etc.)     │  Companion Radio │
│  Node       │                                      │  (with scanning) │
│             │  <──── NO BLE SCANNING ─────────    │                  │
└─────────────┘                                      └──────────────────┘
       │
       └──> LoRa mesh communication continues as normal
```

Sensors broadcast their identity and location via BLE manufacturer data. Companion radios scan for these advertisements and can:
- Display nearby sensors on a map
- Auto-add unknown sensors to contacts
- Track sensor location in real-time
- Communicate with sensors via LoRa mesh

---

## Files to Modify

To add BLE advertising to a sensor node, you need to modify three files in your sensor project:

1. **`examples/simple_sensor/SensorMesh.h`** - Add BLE manager member and method declarations
2. **`examples/simple_sensor/SensorMesh.cpp`** - Implement BLE initialization and updates
3. **`examples/simple_sensor/main.cpp`** - Initialize BLE service in setup()

---

## Implementation Steps

### Step 1: Add Required Includes and Member Variables

**File: `examples/simple_sensor/SensorMesh.h`**

Add the BLE discovery manager include at the top of the file:

```cpp
#pragma once

#include <Arduino.h>
#include <Mesh.h>
#include "TimeSeriesData.h"

// Add this include for BLE advertising
#include <helpers/BLEDiscoveryManager.h>

// ... rest of existing includes ...
```

Add BLE-related members to the `SensorMesh` class (in the private section):

```cpp
class SensorMesh : public mesh::Mesh, public CommonCLICallbacks {
public:
  // ... existing public methods ...

protected:
  // ... existing protected methods ...

private:
  FILESYSTEM* _fs;
  unsigned long next_local_advert, next_flood_advert;
  NodePrefs _prefs;
  CommonCLI _cli;

  // Add these BLE-related members
  #ifdef BLE_PIN_CODE
  BLEDiscoveryManager* ble_discovery_mgr;  // For BLE advertising only
  unsigned long next_ble_update;            // Timer for periodic BLE updates
  #endif

  // ... rest of existing private members ...
};
```

Add a method declaration for BLE manufacturer data updates:

```cpp
private:
  // ... existing private methods ...

  #ifdef BLE_PIN_CODE
  void updateBLEAdvertisement();  // Update BLE manufacturer data
  #endif
```

### Step 2: Initialize BLE Service in Constructor

**File: `examples/simple_sensor/SensorMesh.cpp`**

Initialize the BLE-related members in the constructor:

```cpp
SensorMesh::SensorMesh(mesh::MainBoard& board, mesh::Radio& radio,
                       mesh::MillisecondClock& ms, mesh::RNG& rng,
                       mesh::RTCClock& rtc, mesh::MeshTables& tables)
     : mesh::Mesh(radio, ms, rng, rtc, *new StaticPoolPacketManager(32), tables),
      _cli(board, rtc, sensors, &_prefs, this),
      telemetry(MAX_PACKET_PAYLOAD - 4)
{
  next_local_advert = next_flood_advert = 0;
  dirty_contacts_expiry = 0;
  last_read_time = 0;
  num_alert_tasks = 0;
  set_radio_at = revert_radio_at = 0;

  // Initialize BLE members
  #ifdef BLE_PIN_CODE
  ble_discovery_mgr = nullptr;
  next_ble_update = 0;
  #endif

  // ... rest of existing initialization ...
}
```

### Step 3: Set Up BLE Service in begin()

**File: `examples/simple_sensor/SensorMesh.cpp`**

In the `begin()` method, after existing initialization, add BLE setup:

```cpp
void SensorMesh::begin(FILESYSTEM* fs) {
  mesh::Mesh::begin();
  _fs = fs;

  // Load persisted prefs
  _cli.loadPrefs(_fs);
  acl.load(_fs);

  radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
  radio_set_tx_power(_prefs.tx_power_dbm);

  updateAdvertTimer();
  updateFloodAdvertTimer();

  #if ENV_INCLUDE_GPS == 1
  applyGpsPrefs();

  // Initialize location advertiser with config pointers
  sensors.initLocationAdvertiser(
    &_prefs.gps_loc_distance_threshold,
    &_prefs.gps_loc_frequency,
    &_prefs.gps_loc_guaranteed_interval,
    &_prefs.gps_loc_accuracy_threshold,
    &_prefs.gps_loc_advert_enabled
  );

  // Set the callback for location advertisement triggers
  sensors.setLocationAdvertCallback(&SensorMesh::onLocationAdvertTrigger);
  #endif

  // ═══════════════════════════════════════════════════════════
  // BLE ADVERTISING SETUP (ADVERTISING ONLY, NO SCANNING)
  // ═══════════════════════════════════════════════════════════
  #ifdef BLE_PIN_CODE
  // Note: BLE interface must be initialized in main.cpp before calling begin()
  // This section only sets up the advertising data

  // Create BLE discovery manager (used for advertising only on sensors)
  ble_discovery_mgr = new BLEDiscoveryManager();
  // Note: We do NOT call begin() on the manager - sensors don't scan

  // Initial BLE advertisement update
  updateBLEAdvertisement();

  // Schedule periodic BLE updates (every 30 seconds)
  next_ble_update = futureMillis(30000);

  MESH_DEBUG_PRINTLN("BLE advertising initialized (advertising only, no scanning)");
  #endif
}
```

### Step 4: Implement BLE Advertisement Update Method

**File: `examples/simple_sensor/SensorMesh.cpp`**

Add this new method to update the BLE manufacturer data:

```cpp
#ifdef BLE_PIN_CODE
void SensorMesh::updateBLEAdvertisement() {
  // Build BLE manufacturer data structure
  BLEManufacturerData ble_data;
  memset(&ble_data, 0, sizeof(ble_data));

  // Set standard fields
  ble_data.manufacturer_id = MESHCORE_MANUFACTURER_ID;
  ble_data.magic_byte = MESHCORE_MAGIC_BYTE;
  ble_data.protocol_version = MESHCORE_PROTOCOL_VERSION;

  // Set device hash (first byte of public key for routing)
  ble_data.device_hash = self_id.pub_key[0];

  // Set device type and flags
  ble_data.flags = BLE_FLAGS_SET_TYPE(0, ADV_TYPE_SENSOR);

  // Add location if available
  bool has_location = false;
  #if ENV_INCLUDE_GPS == 1
  if (sensors.node_lat != 0.0 || sensors.node_lon != 0.0) {
    ble_data.latitude = (int32_t)(sensors.node_lat * 1000000.0);
    ble_data.longitude = (int32_t)(sensors.node_lon * 1000000.0);
    ble_data.flags |= BLE_FLAG_HAS_LOCATION;
    has_location = true;
  }
  #endif

  // Use configured static location if no GPS
  if (!has_location && (_prefs.node_lat != 0.0 || _prefs.node_lon != 0.0)) {
    ble_data.latitude = (int32_t)(_prefs.node_lat * 1000000.0);
    ble_data.longitude = (int32_t)(_prefs.node_lon * 1000000.0);
    ble_data.flags |= BLE_FLAG_HAS_LOCATION;
  }

  // Set timestamp from RTC
  ble_data.timestamp = getRTCClock()->getCurrentTime();

  // Calculate CRC16 over first 18 bytes
  ble_data.crc16 = ble_crc16_calc((const uint8_t*)&ble_data, 18);

  // Update BLE advertisement via serial interface
  // Note: serial_interface is global and must be SerialBLEInterface
  extern SerialBLEInterface serial_interface;
  serial_interface.updateManufacturerData(ble_data);

  MESH_DEBUG_PRINTLN("BLE advertisement updated - hash=%02X, has_loc=%d",
                     ble_data.device_hash,
                     (ble_data.flags & BLE_FLAG_HAS_LOCATION) != 0);
}
#endif
```

### Step 5: Update BLE Advertisement in loop()

**File: `examples/simple_sensor/SensorMesh.cpp`**

In the `loop()` method, add periodic BLE updates:

```cpp
void SensorMesh::loop() {
  mesh::Mesh::loop();

  if (next_flood_advert && millisHasNowPassed(next_flood_advert)) {
    mesh::Packet* pkt = createSelfAdvert();
    if (pkt) sendFlood(pkt);

    updateFloodAdvertTimer();
    updateAdvertTimer();
  } else if (next_local_advert && millisHasNowPassed(next_local_advert)) {
    mesh::Packet* pkt = createSelfAdvert();
    if (pkt) sendZeroHop(pkt);

    updateAdvertTimer();
  }

  // ... existing radio parameter handling ...

  uint32_t curr = getRTCClock()->getCurrentTime();
  if (curr >= last_read_time + SENSOR_READ_INTERVAL_SECS) {
    telemetry.reset();
    telemetry.addVoltage(TELEM_CHANNEL_SELF, (float)board.getBattMilliVolts() / 1000.0f);
    sensors.querySensors(0xFF, telemetry);

    onSensorDataRead();

    last_read_time = curr;
  }

  // ═══════════════════════════════════════════════════════════
  // PERIODIC BLE ADVERTISEMENT UPDATE
  // ═══════════════════════════════════════════════════════════
  #ifdef BLE_PIN_CODE
  // Update BLE advertisement periodically (every 30 seconds)
  if (next_ble_update && millisHasNowPassed(next_ble_update)) {
    updateBLEAdvertisement();
    next_ble_update = futureMillis(30000);  // Next update in 30 seconds
  }
  #endif

  // ... rest of existing loop code (alert handling, contacts, etc.) ...
}
```

### Step 6: Hook Location Updates to BLE

**File: `examples/simple_sensor/SensorMesh.cpp`**

Update the `onLocationAdvertTrigger()` callback to also update BLE:

```cpp
// Static callback that forwards to instance method
void SensorMesh::onLocationAdvertTrigger(double lat, double lon) {
  if (instance) {
    instance->sendLocationAdvertisement(lat, lon);
  }
}

// Send location advertisement (flood only)
void SensorMesh::sendLocationAdvertisement(double lat, double lon) {
  MESH_DEBUG_PRINTLN("Location advert triggered: lat=%f, lon=%f", lat, lon);

  // Send LoRa advertisement
  mesh::Packet* pkt = createSelfAdvert();
  if (pkt) {
    sendFlood(pkt);
  } else {
    MESH_DEBUG_PRINTLN("ERROR: unable to create location advertisement packet!");
  }

  // Update stored last advertised position
  _prefs.last_advert_lat = lat;
  _prefs.last_advert_lon = lon;
  savePrefs();

  // ═══════════════════════════════════════════════════════════
  // ALSO UPDATE BLE ADVERTISEMENT WITH NEW LOCATION
  // ═══════════════════════════════════════════════════════════
  #ifdef BLE_PIN_CODE
  updateBLEAdvertisement();
  MESH_DEBUG_PRINTLN("BLE advertisement updated with new location");
  #endif
}
```

### Step 7: Initialize BLE in main.cpp

**File: `examples/simple_sensor/main.cpp`**

Add conditional BLE interface includes and initialization:

```cpp
#include "SensorMesh.h"

// ... existing includes ...

// Add BLE interface support
#ifdef BLE_PIN_CODE
  #ifdef ESP32
    #include <helpers/esp32/SerialBLEInterface.h>
  #elif defined(NRF52_PLATFORM)
    #include <helpers/nrf52/SerialBLEInterface.h>
  #endif

  SerialBLEInterface serial_interface;
#endif

// ... rest of global objects ...

void setup() {
  Serial.begin(115200);
  delay(1000);

  board.begin();

  #ifdef DISPLAY_CLASS
  if (display.begin()) {
    display.startFrame();
    display.print("Please wait...");
    display.endFrame();
  }
  #endif

  if (!radio_init()) { halt(); }

  fast_rng.begin(radio_get_rng_seed());

  // ... filesystem initialization ...

  sensors.begin();

  // ═══════════════════════════════════════════════════════════
  // INITIALIZE BLE INTERFACE (IF ENABLED)
  // ═══════════════════════════════════════════════════════════
  #ifdef BLE_PIN_CODE
  // Build BLE device name
  char ble_name[48];
  snprintf(ble_name, sizeof(ble_name), "Sensor-%02X%02X",
           the_mesh.self_id.pub_key[0],
           the_mesh.self_id.pub_key[1]);

  // Initialize BLE interface
  serial_interface.begin(ble_name, BLE_PIN_CODE);

  // Create MeshCore BLE service for discovery
  serial_interface.createMeshCoreService();

  // Set BLE characteristics (public key, device info, signature)
  BLEDeviceInfo device_info;
  device_info.device_type = ADV_TYPE_SENSOR;
  device_info.flags = 0;
  device_info.timestamp = rtc_clock.getCurrentTime();
  strncpy(device_info.name, the_mesh.getNodeName(), sizeof(device_info.name) - 1);
  device_info.name[sizeof(device_info.name) - 1] = '\0';

  // Create signature: sign(pubkey || timestamp || device_info)
  uint8_t signature[64];
  uint8_t msg_to_sign[32 + 4 + sizeof(BLEDeviceInfo)];
  memcpy(msg_to_sign, the_mesh.self_id.pub_key, 32);
  memcpy(msg_to_sign + 32, &device_info.timestamp, 4);
  memcpy(msg_to_sign + 36, &device_info, sizeof(BLEDeviceInfo));
  Ed25519::sign(signature, the_mesh.self_id.priv_key, the_mesh.self_id.pub_key,
                msg_to_sign, sizeof(msg_to_sign));

  serial_interface.setMeshCoreCharacteristics(
    the_mesh.self_id.pub_key,
    &device_info,
    signature
  );

  MESH_DEBUG_PRINTLN("BLE interface initialized: %s", ble_name);

  // DO NOT call startScanning() - sensors don't scan, they only advertise
  #endif

  the_mesh.begin(fs);

  #ifdef DISPLAY_CLASS
  ui_task.begin(the_mesh.getNodePrefs(), FIRMWARE_BUILD_DATE, FIRMWARE_VERSION);
  #endif

  // Send out initial Advertisement to the mesh
  the_mesh.sendSelfAdvertisement(16000);
}

void loop() {
  the_mesh.loop();
  sensors.loop();
  #ifdef DISPLAY_CLASS
  ui_task.loop();
  #endif
}
```

---

## BLE Manufacturer Data Structure

The BLE advertisement includes a 27-byte manufacturer data packet with the following structure:

```cpp
struct BLEManufacturerData {
    uint16_t manufacturer_id;  // [0-1]   0xFFFF (MESHCORE_MANUFACTURER_ID)
    uint8_t  magic_byte;       // [2]     0x4D ('M' for MeshCore)
    uint8_t  protocol_version; // [3]     0x01 (MESHCORE_PROTOCOL_VERSION)
    uint8_t  device_hash;      // [4]     First byte of pub_key (routing hash)
    uint8_t  flags;            // [5]     Device type (4 bits) + has_location (bit 4)
    int32_t  latitude;         // [6-9]   lat * 1E6 (if has_location set)
    int32_t  longitude;        // [10-13] lon * 1E6 (if has_location set)
    uint32_t timestamp;        // [14-17] Current RTC time
    uint16_t crc16;            // [18-19] CRC16 of bytes 0-17
    uint8_t  reserved[7];      // [20-26] Reserved for future use
} __attribute__((packed));
```

### Field Details

| Field | Size | Description |
|-------|------|-------------|
| `manufacturer_id` | 2 bytes | Always `0xFFFF` for MeshCore devices |
| `magic_byte` | 1 byte | Always `0x4D` ('M') to identify MeshCore protocol |
| `protocol_version` | 1 byte | Currently `0x01`, for future compatibility |
| `device_hash` | 1 byte | First byte of Ed25519 public key (used for routing) |
| `flags` | 1 byte | Bits 0-3: Device type (`ADV_TYPE_SENSOR`=4), Bit 4: Has location |
| `latitude` | 4 bytes | GPS latitude × 1,000,000 (signed int32) |
| `longitude` | 4 bytes | GPS longitude × 1,000,000 (signed int32) |
| `timestamp` | 4 bytes | Current RTC time in seconds since epoch |
| `crc16` | 2 bytes | CRC16 checksum of bytes 0-17 for validation |
| `reserved` | 7 bytes | Reserved for future protocol extensions |

### Device Type Values

```cpp
#define ADV_TYPE_NONE         0
#define ADV_TYPE_CHAT         1  // Companion radio / handheld
#define ADV_TYPE_REPEATER     2  // Mesh repeater / router
#define ADV_TYPE_ROOM         3  // Room monitor / base station
#define ADV_TYPE_SENSOR       4  // Sensor node (THIS TYPE)
```

---

## Configuration in platformio.ini

To enable BLE advertising on your sensor node, add the following build flags:

```ini
[env:sensor_with_ble]
extends = env:simple_sensor
build_flags =
    ${env:simple_sensor.build_flags}
    -D BLE_PIN_CODE=123456              ; Enable BLE with PIN code
    -D BLE_NAME_PREFIX="Sensor-"        ; BLE device name prefix
    -D BLE_TX_POWER=4                   ; BLE TX power in dBm
```

For GPS-enabled sensors, also ensure:

```ini
build_flags =
    ${env:simple_sensor.build_flags}
    -D ENV_INCLUDE_GPS=1                ; Enable GPS support
    -D BLE_PIN_CODE=123456
```

---

## Benefits of BLE Advertising on Sensors

### 1. Faster Discovery
Companion radios can discover nearby sensors in seconds via BLE scanning, versus waiting minutes for LoRa advertisements.

### 2. Real-Time Location Tracking
GPS-enabled sensors broadcast their location via BLE as soon as it changes, enabling real-time tracking on companion devices.

### 3. Lower Power Consumption
BLE advertising-only mode uses ~10x less power than BLE scanning, making it ideal for battery-powered sensors.

### 4. Transparent Operation
BLE advertising works completely in the background. All existing LoRa mesh functionality continues unchanged.

### 5. Extended Range via Mesh
While BLE discovery works at short range (~50m), sensors can still communicate over long distances via the LoRa mesh network.

---

## Update Triggers

The BLE manufacturer data is automatically updated when:

1. **Periodic Timer**: Every 30 seconds (configurable in `loop()`)
2. **Location Changes**: When GPS location advertising is triggered
3. **Sensor Readings**: When sensor data changes (optional - call `updateBLEAdvertisement()` in `onSensorDataRead()`)
4. **Manual Update**: Can be triggered via CLI or remote command

---

## Power Consumption Comparison

| Mode | Current Draw | Notes |
|------|-------------|-------|
| BLE Advertising Only | ~5-10 mA | Updates every 30 seconds |
| BLE Scanning | ~50-100 mA | Continuous listening |
| LoRa RX | ~10-15 mA | Duty cycled |
| LoRa TX | ~100-120 mA | Brief bursts |

Sensors using BLE advertising-only mode maintain excellent battery life while still being discoverable by nearby companion radios.

---

## Testing Your Implementation

### Step 1: Verify BLE Service Creation

After uploading to your sensor, check the serial output for:

```
BLE interface initialized: Sensor-AB12
MeshCore service created
BLE advertising initialized (advertising only, no scanning)
BLE advertisement updated - hash=AB, has_loc=0
```

### Step 2: Scan with nRF Connect Mobile App

1. Install "nRF Connect" app on your phone
2. Start scanning
3. Look for your sensor: `Sensor-XXXX`
4. Connect and verify you can read the MeshCore service characteristics:
   - Public Key (32 bytes)
   - Device Info (name, type, timestamp)
   - Signature (64 bytes)

### Step 3: Test with Companion Radio

1. Upload the companion radio firmware with BLE scanning enabled
2. Start the companion radio
3. Watch the serial output or UI for discovered sensors
4. Verify the sensor appears in the neighbor list
5. If GPS is enabled, verify location is displayed

### Step 4: Verify Location Updates

If your sensor has GPS:

1. Move the sensor to trigger a location update
2. Check that BLE advertisement is updated (serial log)
3. Verify companion radio receives the new location
4. Confirm the location is displayed correctly on the UI/map

---

## Troubleshooting

### BLE Service Not Created

**Symptom**: No BLE advertisement visible in scanner apps

**Solutions**:
- Ensure `BLE_PIN_CODE` is defined in platformio.ini
- Check that `serial_interface` is of type `SerialBLEInterface`
- Verify `createMeshCoreService()` is called before `the_mesh.begin()`
- Check serial output for BLE initialization messages

### Manufacturer Data Not Updating

**Symptom**: Old location or timestamp in BLE advertisement

**Solutions**:
- Verify `updateBLEAdvertisement()` is being called
- Check `next_ble_update` timer is working
- Ensure `serial_interface.updateManufacturerData()` succeeds
- Look for error messages in serial output

### Location Not in Advertisement

**Symptom**: `has_location` flag is not set in manufacturer data

**Solutions**:
- Verify `ENV_INCLUDE_GPS=1` is defined if using GPS
- Check that `sensors.node_lat` and `sensors.node_lon` are non-zero
- Confirm static location is set in prefs if no GPS
- Check BLE flags calculation

### High Power Consumption

**Symptom**: Battery draining faster than expected

**Solutions**:
- Ensure you're NOT calling `startScanning()` on sensors
- Increase BLE update interval from 30s to 60s or longer
- Reduce BLE TX power if possible
- Verify BLE advertising interval is appropriate

### CRC Validation Failures

**Symptom**: Companion radio rejects sensor advertisements

**Solutions**:
- Ensure `ble_crc16_calc()` is called on exactly 18 bytes
- Verify structure packing is correct (should be 27 bytes total)
- Check for endianness issues on different platforms
- Validate manufacturer ID and magic byte are correct

---

## Advanced Topics

### Custom Update Intervals

You can adjust the BLE update frequency based on your needs:

```cpp
// Fast updates (every 10 seconds) - for rapidly changing sensors
next_ble_update = futureMillis(10000);

// Standard updates (every 30 seconds) - balanced power/responsiveness
next_ble_update = futureMillis(30000);

// Slow updates (every 2 minutes) - maximum battery life
next_ble_update = futureMillis(120000);
```

### Conditional Updates

Only update BLE when sensor readings change significantly:

```cpp
void onSensorDataRead() override {
  float batt_voltage = getVoltage(TELEM_CHANNEL_SELF);
  float temp = getTemperature(TELEM_CHANNEL_ENV);

  battery_data.recordData(getRTCClock(), batt_voltage);
  alertIf(batt_voltage < 3.4f, critical_batt, HIGH_PRI_ALERT, "Battery critical!");

  // Update BLE if temperature changed significantly
  #ifdef BLE_PIN_CODE
  static float last_temp = 0.0f;
  if (abs(temp - last_temp) > 0.5f) {  // 0.5°C threshold
    updateBLEAdvertisement();
    last_temp = temp;
  }
  #endif
}
```

### Adding Battery Level to Advertisement

Use the reserved bytes in manufacturer data for battery percentage:

```cpp
void SensorMesh::updateBLEAdvertisement() {
  // ... existing code ...

  // Add battery level to first reserved byte
  float batt_voltage = (float)board.getBattMilliVolts() / 1000.0f;
  uint8_t batt_percent = (uint8_t)((batt_voltage - 3.0f) / (4.2f - 3.0f) * 100.0f);
  batt_percent = constrain(batt_percent, 0, 100);
  ble_data.reserved[0] = batt_percent;

  // Update low battery flag
  if (batt_percent < 20) {
    ble_data.flags |= BLE_FLAG_BATTERY_LOW;
  }

  // ... rest of code ...
}
```

---

## Summary

By following this guide, you have:

1. Added BLE advertising capability to your sensor node
2. Implemented manufacturer data broadcasting with location
3. Integrated BLE updates with sensor readings and GPS
4. Maintained low power consumption through advertising-only mode
5. Enabled fast discovery by companion radios

Your sensor now broadcasts its identity and location via BLE while continuing to operate on the LoRa mesh network. Companion radios can discover your sensor quickly via BLE scanning and communicate with it via LoRa mesh for longer-range operation.

Remember: **Sensors ONLY advertise, they NEVER scan.** This is a key design principle for power efficiency.
