# BLE Advertising Integration for Room Servers

## Overview

Room servers are fixed infrastructure nodes that provide message relay and storage services to mobile companion radios. Unlike mobile devices, room servers **only advertise** their presence via BLE—they do not scan for other devices.

### Why Advertising Only?

1. **Fixed Location**: Room servers are stationary infrastructure (home servers, public BBS nodes)
2. **Infrastructure Role**: They serve mobile clients, not discover peers
3. **Power Efficiency**: Advertising-only uses minimal power compared to continuous scanning
4. **Indoor Advantages**: BLE advertising penetrates buildings better than LoRa
5. **Instant Discovery**: Companion radios entering the building discover room servers immediately

This design mirrors WiFi access points: they broadcast their SSID, but don't scan for other APs.

## Architecture

```
┌─────────────────────┐
│   Room Server       │
│                     │
│  ┌──────────────┐   │
│  │ LoRa Mesh    │   │  Fixed position
│  │ Radio        │   │  Serves messages
│  └──────────────┘   │  Relays packets
│                     │
│  ┌──────────────┐   │
│  │ BLE Radio    │   │
│  │ (Advertise)  │◄──┼── Broadcasts presence
│  └──────────────┘   │   every ~1 second
│                     │
└─────────────────────┘
         │
         │ BLE Advertisement:
         │ - Device hash
         │ - Room location
         │ - Timestamp
         │ - Device type: ROOM
         │
         ▼
┌─────────────────────┐
│ Companion Radio     │
│ (Mobile Device)     │
│                     │
│  ┌──────────────┐   │
│  │ BLE Radio    │   │
│  │ (Scan)       │◄──┼── Discovers nearby
│  └──────────────┘   │   room servers
│                     │
└─────────────────────┘
```

## Implementation Steps

### Step 1: Understand the BLE Discovery Infrastructure

The BLE discovery system is already implemented in MeshCore and consists of:

- **BLEServiceDefinitions.h**: Defines manufacturer data structure (27 bytes)
- **BLEManufacturerData**: The data structure advertised via BLE
- **SerialBLEInterface**: Platform-specific BLE implementation (nRF52/ESP32)

Room servers will use **only** the advertising portion of this infrastructure.

### Step 2: Modify MyMesh.h

Add the BLE interface member to your MyMesh class.

**File**: `/Users/dz0ny/meshcore-sar/MeshCore/examples/simple_room_server/MyMesh.h`

**Add includes** (after existing includes, around line 23):

```cpp
#include <helpers/BLEServiceDefinitions.h>

// Platform-specific BLE interface
#if defined(NRF52_PLATFORM)
  #include <helpers/nrf52/SerialBLEInterface.h>
#elif defined(ESP32)
  #include <helpers/esp32/SerialBLEInterface.h>
#endif
```

**Add member variable** (in the private section of MyMesh class, around line 89):

```cpp
private:
  FILESYSTEM* _fs;
  unsigned long next_local_advert, next_flood_advert;
  bool _logging;
  NodePrefs _prefs;
  CommonCLI _cli;
  ClientACL acl;

  // BLE advertising (add this section)
#if defined(NRF52_PLATFORM) || defined(ESP32)
  SerialBLEInterface _ble_interface;
  unsigned long next_ble_update;
#endif

  // ... rest of members
```

### Step 3: Modify MyMesh.cpp - Initialization

Initialize BLE service and start advertising in the `begin()` method.

**File**: `/Users/dz0ny/meshcore-sar/MeshCore/examples/simple_room_server/MyMesh.cpp`

**In the constructor** (around line 622, add initialization):

```cpp
MyMesh::MyMesh(mesh::MainBoard &board, mesh::Radio &radio, mesh::MillisecondClock &ms, mesh::RNG &rng,
               mesh::RTCClock &rtc, mesh::MeshTables &tables)
    : mesh::Mesh(radio, ms, rng, rtc, *new StaticPoolPacketManager(32), tables),
      _cli(board, rtc, sensors, &_prefs, this), telemetry(MAX_PACKET_PAYLOAD - 4) {
  next_local_advert = next_flood_advert = 0;
  dirty_contacts_expiry = 0;
  _logging = false;
  set_radio_at = revert_radio_at = 0;

#if defined(NRF52_PLATFORM) || defined(ESP32)
  next_ble_update = 0;
#endif

  // ... rest of constructor
```

**In the begin() method** (after line 641, add BLE initialization):

```cpp
void MyMesh::begin(FILESYSTEM *fs) {
  mesh::Mesh::begin();
  _fs = fs;

  // load persisted prefs
  _cli.loadPrefs(_fs);

  acl.load(_fs);

  radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
  radio_set_tx_power(_prefs.tx_power_dbm);

  updateAdvertTimer();
  updateFloodAdvertTimer();

#if ENV_INCLUDE_GPS == 1
  applyGpsPrefs();
#endif

  // ============================================================
  // BLE ADVERTISING SETUP (Room Server - Advertising Only)
  // ============================================================
#if defined(NRF52_PLATFORM) || defined(ESP32)

  // Initialize BLE interface
  char ble_name[32];
  snprintf(ble_name, sizeof(ble_name), "Room-%02X%02X",
           self_id.pub_key[0], self_id.pub_key[1]);

  // BLE PIN is derived from device hash (for security if someone connects)
  uint32_t ble_pin = ((uint32_t)self_id.pub_key[0] << 16) |
                     ((uint32_t)self_id.pub_key[1] << 8) |
                     self_id.pub_key[2];

  _ble_interface.begin(ble_name, ble_pin);

  // Create MeshCore GATT service (for devices that connect)
  _ble_interface.createMeshCoreService();

  // Prepare device info for GATT characteristics
  BLEDeviceInfo device_info;
  device_info.device_type = ADV_TYPE_ROOM;
  device_info.flags = 0;
  device_info.timestamp = getRTCClock()->getCurrentTime();
  strncpy(device_info.name, _prefs.node_name, sizeof(device_info.name) - 1);
  device_info.name[sizeof(device_info.name) - 1] = '\0';

  // Create signature: sign (pubkey || timestamp || device_info)
  uint8_t signature[64];
  uint8_t msg_to_sign[32 + 4 + sizeof(BLEDeviceInfo)];
  memcpy(msg_to_sign, self_id.pub_key, 32);
  memcpy(msg_to_sign + 32, &device_info.timestamp, 4);
  memcpy(msg_to_sign + 36, &device_info, sizeof(BLEDeviceInfo));
  Ed25519::sign(signature, self_id.priv_key, self_id.pub_key, msg_to_sign, sizeof(msg_to_sign));

  // Set GATT characteristics (readable by connecting devices)
  _ble_interface.setMeshCoreCharacteristics(self_id.pub_key, &device_info, signature);

  // Build initial manufacturer data
  BLEManufacturerData mfg_data;
  mfg_data.manufacturer_id = MESHCORE_MANUFACTURER_ID;
  mfg_data.magic_byte = MESHCORE_MAGIC_BYTE;
  mfg_data.protocol_version = MESHCORE_PROTOCOL_VERSION;
  mfg_data.device_hash = self_id.pub_key[0]; // First byte of public key
  mfg_data.flags = BLE_FLAGS_SET_TYPE(0, ADV_TYPE_ROOM);

  // Include room location if configured
  if (_prefs.node_lat != 0.0 || _prefs.node_lon != 0.0) {
    mfg_data.flags |= BLE_FLAG_HAS_LOCATION;
    mfg_data.latitude = (int32_t)(_prefs.node_lat * 1000000.0);
    mfg_data.longitude = (int32_t)(_prefs.node_lon * 1000000.0);
  } else {
    mfg_data.latitude = 0;
    mfg_data.longitude = 0;
  }

  mfg_data.timestamp = getRTCClock()->getCurrentTime();
  memset(mfg_data.reserved, 0, sizeof(mfg_data.reserved));

  // Calculate CRC16 over first 18 bytes
  mfg_data.crc16 = ble_crc16_calc((const uint8_t*)&mfg_data, 18);

  // Update advertisement with manufacturer data
  _ble_interface.updateManufacturerData(mfg_data);

  // Enable BLE advertising
  _ble_interface.enable();

  // Schedule next BLE update (refresh timestamp every 60 seconds)
  next_ble_update = futureMillis(60000);

  MESH_DEBUG_PRINTLN("BLE advertising started as ROOM server");

#endif
  // ============================================================
}
```

### Step 4: Modify MyMesh.cpp - Loop Method

Update BLE advertisement periodically to refresh timestamp.

**File**: `/Users/dz0ny/meshcore-sar/MeshCore/examples/simple_room_server/MyMesh.cpp`

**In the loop() method** (add at the end, around line 860):

```cpp
void MyMesh::loop() {
  mesh::Mesh::loop();

  // ... existing loop code ...

  // is pending dirty contacts write needed?
  if (dirty_contacts_expiry && millisHasNowPassed(dirty_contacts_expiry)) {
    acl.save(_fs, MyMesh::saveFilter);
    dirty_contacts_expiry = 0;
  }

  // TODO: periodically check for OLD/inactive entries in known_clients[], and evict

  // ============================================================
  // BLE ADVERTISEMENT UPDATE
  // ============================================================
#if defined(NRF52_PLATFORM) || defined(ESP32)

  // Update BLE advertisement periodically (refresh timestamp)
  if (next_ble_update && millisHasNowPassed(next_ble_update)) {

    // Rebuild manufacturer data with current timestamp
    BLEManufacturerData mfg_data;
    mfg_data.manufacturer_id = MESHCORE_MANUFACTURER_ID;
    mfg_data.magic_byte = MESHCORE_MAGIC_BYTE;
    mfg_data.protocol_version = MESHCORE_PROTOCOL_VERSION;
    mfg_data.device_hash = self_id.pub_key[0];
    mfg_data.flags = BLE_FLAGS_SET_TYPE(0, ADV_TYPE_ROOM);

    // Include room location (static, but refresh in case config changed)
    if (_prefs.node_lat != 0.0 || _prefs.node_lon != 0.0) {
      mfg_data.flags |= BLE_FLAG_HAS_LOCATION;
      mfg_data.latitude = (int32_t)(_prefs.node_lat * 1000000.0);
      mfg_data.longitude = (int32_t)(_prefs.node_lon * 1000000.0);
    } else {
      mfg_data.latitude = 0;
      mfg_data.longitude = 0;
    }

    mfg_data.timestamp = getRTCClock()->getCurrentTime();
    memset(mfg_data.reserved, 0, sizeof(mfg_data.reserved));

    // Calculate CRC16
    mfg_data.crc16 = ble_crc16_calc((const uint8_t*)&mfg_data, 18);

    // Update advertisement
    _ble_interface.updateManufacturerData(mfg_data);

    // Schedule next update (every 60 seconds)
    next_ble_update = futureMillis(60000);
  }

#endif
  // ============================================================
}
```

### Step 5: Verify Build Configuration

Ensure your `platformio.ini` includes the necessary BLE libraries:

**For nRF52 platforms**:
```ini
lib_deps =
    adafruit/Adafruit Bluefruit nRF52 Libraries@^0.21.0
```

**For ESP32 platforms**:
```ini
lib_deps =
    ESP32 BLE Arduino  # Built-in to ESP32 core
```

## Manufacturer Data Content

The room server advertises the following information in the 27-byte manufacturer data structure:

```cpp
struct BLEManufacturerData {
    uint16_t manufacturer_id;  // 0xFFFF (MESHCORE_MANUFACTURER_ID)
    uint8_t  magic_byte;       // 0x4D ('M' for MeshCore)
    uint8_t  protocol_version; // 0x01
    uint8_t  device_hash;      // self_id.pub_key[0] - routing hash
    uint8_t  flags;            // ADV_TYPE_ROOM (3) + has_location flag
    int32_t  latitude;         // _prefs.node_lat * 1E6
    int32_t  longitude;        // _prefs.node_lon * 1E6
    uint32_t timestamp;        // Current RTC time
    uint16_t crc16;            // CRC16 of bytes 0-17
    uint8_t  reserved[7];      // Future use
} __attribute__((packed));
```

### Device Type: ADV_TYPE_ROOM

The `flags` byte encodes the device type in the lower 4 bits:

```cpp
#define ADV_TYPE_NONE         0
#define ADV_TYPE_CHAT         1  // Companion radio
#define ADV_TYPE_REPEATER     2  // LoRa repeater
#define ADV_TYPE_ROOM         3  // Room server (THIS IS US!)
#define ADV_TYPE_SENSOR       4  // Sensor node
```

Room servers use `ADV_TYPE_ROOM` to identify themselves as fixed infrastructure.

### Location Data

If the room server has a static GPS location configured:

```cpp
// In MyMesh.h (defaults)
#ifndef ADVERT_LAT
  #define  ADVERT_LAT  0.0
#endif
#ifndef ADVERT_LON
  #define  ADVERT_LON  0.0
#endif
```

This location is advertised to companion radios, allowing them to:
- See the room server on a map
- Calculate distance to the room server
- Navigate to the physical location

### Timestamp Updates

The timestamp is refreshed every 60 seconds to:
- Prove the room server is alive
- Provide accurate time sync for companion radios
- Help detect stale advertisements

## GATT Service Characteristics

When a companion radio **connects** to the room server (for initial contact exchange), it can read:

1. **Public Key Characteristic** (32 bytes)
   - Full Ed25519 public key
   - Used for encryption and signature verification

2. **Device Info Characteristic** (sizeof(BLEDeviceInfo))
   - Device type: ADV_TYPE_ROOM
   - Device name: From `_prefs.node_name`
   - Timestamp: When service was created

3. **Signature Characteristic** (64 bytes)
   - Ed25519 signature over (pubkey || timestamp || device_info)
   - Prevents spoofing attacks
   - Companion radio verifies this before adding contact

## Benefits of BLE Advertising for Room Servers

### 1. Indoor Coverage
- BLE penetrates walls better than LoRa
- Companion radios discover room servers immediately upon entering building
- No need to wait for LoRa advertisement cycle

### 2. Battery Efficiency
- BLE advertising uses minimal power (few mW)
- No scanning overhead (room servers don't scan)
- Can run continuously without battery drain concerns

### 3. Instant Discovery
- Companion radios scan for BLE devices continuously
- Room server discovered in < 5 seconds
- Immediate connectivity when entering range

### 4. Location Awareness
- Room servers broadcast their fixed location
- Companion radios can display "Room Server: Main Office" on map
- Helps users find physical location of infrastructure

### 5. Complementary to LoRa
- BLE handles local discovery (< 50m indoors)
- LoRa handles mesh routing (> 1km outdoors)
- Both work together seamlessly

## Testing Your Implementation

### 1. Verify Advertising

On companion radio, you should see the room server appear in BLE scan results:

```
BLE Discovery: Found device hash=3F, type=ROOM, RSSI=-45 dBm
BLE Discovery: Location: 47.620506, -122.349277
```

### 2. Verify GATT Service

Connect to the room server and read characteristics:

```
Connected to Room-3F2A
Reading public key... [32 bytes]
Reading device info... name="Main BBS", type=ROOM
Verifying signature... OK ✓
Contact added: Main BBS (hash: 3F)
```

### 3. Verify Manufacturer Data

Check that CRC is valid and fields are correct:

```
Manufacturer ID: 0xFFFF ✓
Magic Byte: 0x4D ✓
Protocol Version: 0x01 ✓
Device Hash: 0x3F ✓
Device Type: ROOM (3) ✓
Has Location: Yes ✓
Latitude: 47.620506 ✓
Longitude: -122.349277 ✓
CRC16: Valid ✓
```

## Important Notes

### DO NOT Enable Scanning

Room servers should **NEVER** call:

```cpp
// DO NOT DO THIS IN ROOM SERVERS!
_ble_interface.startScanning(discovery_mgr);  // ❌ WRONG!
```

Scanning is for companion radios only. Room servers are stationary infrastructure—they advertise, not discover.

### Update Frequency

The implementation updates the BLE advertisement every 60 seconds. This is sufficient because:

- Room servers don't move (static location)
- Timestamp refresh shows the server is alive
- Companion radios cache manufacturer data

You can adjust this in the code:

```cpp
// Update every 30 seconds instead of 60
next_ble_update = futureMillis(30000);
```

### Power Consumption

BLE advertising uses approximately:
- **nRF52**: ~3-5 mW continuous
- **ESP32**: ~10-15 mW continuous

This is negligible for mains-powered room servers.

## Troubleshooting

### Room server not discovered

**Check:**
1. BLE is enabled in build config
2. Manufacturer data CRC is valid
3. Device type is set to ADV_TYPE_ROOM
4. Advertising is started in begin()

### Companion radio shows wrong location

**Check:**
1. `_prefs.node_lat` and `_prefs.node_lon` are set correctly
2. Values are in decimal degrees (not radians)
3. `BLE_FLAG_HAS_LOCATION` is set in flags byte
4. Location is multiplied by 1E6 before storing in int32_t

### High power consumption

**Check:**
1. Scanning is NOT enabled (room servers advertise only)
2. Update frequency is reasonable (60 seconds is good)
3. BLE stack is initialized correctly

## Summary

Adding BLE advertising to room servers is straightforward:

1. **Include** BLE headers in MyMesh.h
2. **Add** SerialBLEInterface member variable
3. **Initialize** BLE service in begin()
4. **Update** manufacturer data in loop() (every 60s)
5. **Never** enable scanning (advertising only!)

The result is a room server that:
- Broadcasts its presence via BLE
- Advertises its fixed location
- Provides GATT service for contact exchange
- Consumes minimal power
- Works seamlessly with companion radios

This transforms your room server from a hidden mesh node into visible local infrastructure that companion radios can instantly discover and connect to.
