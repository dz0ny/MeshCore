# BLE_ADVERT Flag Documentation

## Overview

The `BLE_ADVERT` flag controls BLE (Bluetooth Low Energy) advertisement and discovery features in MeshCore firmware. When set to `0`, it disables active BLE discovery while keeping basic BLE infrastructure intact.

## Flag Values

- **`BLE_ADVERT=1`** (default): Full BLE discovery enabled
  - Advertisement broadcasting enabled
  - Scanning for other devices enabled
  - Auto-connection to unknown devices enabled
  - Contact creation from BLE enabled

- **`BLE_ADVERT=0`**: BLE discovery disabled
  - Classes still exist (for compilation)
  - All discovery methods return early/false
  - Reduces memory and power usage
  - Still allows basic BLE connectivity if `BLE_PIN_CODE` is set

## Architecture

### Current Implementation (2025-10-20)

The BLE discovery code has been refactored to properly handle conditional compilation.

**Key Principle**: Classes always exist (for type safety), but implementations contain stubs when `BLE_ADVERT=0`.

1. **Header Files** - Classes ALWAYS defined (no `#if BLE_ADVERT` wrappers):
   - `src/helpers/BLEDiscoveryManager.h` - Base class always declared
   - `src/helpers/nrf52/nRF52BLEDiscoveryManager.h` - Derived class always declared
   - `src/helpers/nrf52/SerialBLEInterface.h` - Methods conditionally declared with `#if BLE_ADVERT`

2. **Implementation Files** - Methods have conditional or stub implementations:
   - `src/helpers/BLEDiscoveryManager.cpp` - All methods wrapped in `#if BLE_ADVERT`
   - `src/helpers/nrf52/nRF52BLEDiscoveryManager.cpp` - Methods always exist, contain `#if BLE_ADVERT` stubs
   - `src/helpers/nrf52/SerialBLEInterface.cpp` - Methods wrapped in `#if BLE_ADVERT`

3. **Usage Sites** - Conditional execution:
   - `examples/companion_radio/MyMesh.cpp` - Early returns in init/loop
   - `examples/companion_radio/ui-new/NearbyScreen.cpp` - Skip BLE neighbor queries
   - `examples/companion_radio/ui-new/UITask.cpp` - Skip nearby count widget

**CRITICAL**: Headers must NOT wrap class definitions in `#if BLE_ADVERT` because MyMesh has a member variable `_ble_discovery` of type `nRF52BLEDiscoveryManager`. If the class doesn't exist, compilation fails.

## File-by-File Usage

### Header Files

#### `src/helpers/BLEDiscoveryManager.h`
```cpp
#pragma once

#include "BLEServiceDefinitions.h"
#include "BaseChatMesh.h"
#include <Arduino.h>

// BLENeighbor struct - ALWAYS defined
// BLEDiscoveryManager class - ALWAYS defined
```
**Status**: ✅ Class ALWAYS defined (no `#if BLE_ADVERT` wrapper)

#### `src/helpers/nrf52/nRF52BLEDiscoveryManager.h`
```cpp
class nRF52BLEDiscoveryManager : public BLEDiscoveryManager {
  // Always defined (inherits from base class)
};
```
**Status**: ✅ Always defined (base class handles conditional compilation)

#### `src/helpers/nrf52/SerialBLEInterface.h`
```cpp
#if BLE_ADVERT
  BLEDiscoveryManager* discovery_manager;
  void startScanning(BLEDiscoveryManager* discovery_mgr);
  bool connectAndReadDevice(const uint8_t* mac_addr, uint8_t device_hash);
#endif
```
**Status**: ✅ Methods only declared when enabled

#### `src/helpers/ContactInfo.h`
```cpp
#if BLE_ADVERT
  unsigned long last_seen_ble;
  int16_t ble_rssi;
  uint8_t mac_address[6];
#endif
```
**Status**: ✅ BLE-specific contact fields conditional

### Implementation Files

#### `src/helpers/BLEDiscoveryManager.cpp`
```cpp
#if BLE_ADVERT
// All method implementations
#endif // BLE_ADVERT
```
**Status**: ✅ Entire file wrapped

#### `src/helpers/nrf52/nRF52BLEDiscoveryManager.cpp`
```cpp
// Constructor/destructor always defined (no conditionals)
nRF52BLEDiscoveryManager::nRF52BLEDiscoveryManager()
    : BLEDiscoveryManager(), ble_interface(nullptr) { }

nRF52BLEDiscoveryManager::~nRF52BLEDiscoveryManager() { }

// Methods always exist, critical code wrapped
void setBLEInterface(SerialBLEInterface* interface) {
  ble_interface = interface;
  #if BLE_ADVERT
    if (ble_interface) {
      ble_interface->discovery_manager = this;
    }
  #endif
}

bool connectAndReadDevice(...) {
  #if BLE_ADVERT
    // Full implementation
    return ble_interface->connectAndReadDevice(mac_addr, device_hash);
  #else
    return false;  // Stub - always fail when disabled
  #endif
}
```
**Status**: ✅ Methods always exist but contain stubs when disabled

#### `src/helpers/nrf52/SerialBLEInterface.cpp`
```cpp
#if BLE_ADVERT
void SerialBLEInterface::startScanning(BLEDiscoveryManager* discovery_mgr) {
  // Implementation
}

bool SerialBLEInterface::connectAndReadDevice(...) {
  // Implementation
}
#endif  // BLE_ADVERT
```
**Status**: ✅ Methods only compiled when enabled

### Application Files

#### `examples/companion_radio/MyMesh.cpp`
```cpp
void MyMesh::initBLEDiscovery() {
  #if BLE_ADVERT == 0
  MESH_DEBUG_PRINTLN("BLE discovery DISABLED (BLE_ADVERT=0)");
  return;  // Early exit
  #endif

  // BLE initialization code
  #if BLE_ADVERT
  _ble_discovery.begin(this);
  // ...
  #endif
}

void MyMesh::loop() {
  // ...
  #if BLE_ADVERT
  _ble_discovery.loop();
  #endif
}

void MyMesh::updateBLEAdvertisement() {
  #if BLE_ADVERT == 0
  return;  // Skip when disabled
  #endif
  // Update advertisement data
}
```
**Status**: ✅ Early returns and conditional calls

#### `examples/companion_radio/ui-new/NearbyScreen.cpp`
```cpp
int buildUnifiedNearbyList(...) {
  // Step 1: Get LoRa neighbors (always)

  // Step 2: Get BLE neighbors (conditional)
  #if BLE_ADVERT
  auto* ble_mgr = the_mesh.getBLEDiscovery();
  BLENeighbor ble_neighbors[16];
  int ble_count = ble_mgr->getActiveNeighbors(ble_neighbors, 16);
  // Merge BLE neighbors into list
  #endif

  return count;
}

bool NearbyScreen::handleInput(char key) {
  // ...
  #if BLE_ADVERT
  BLENeighbor ble_neighbors[16];
  int ble_count = ble_mgr->getActiveNeighbors(ble_neighbors, 16);
  #endif
  // ...
}
```
**Status**: ✅ BLE neighbor code wrapped

#### `examples/companion_radio/ui-new/UITask.cpp`
```cpp
void renderNearbyCountWidget(DisplayDriver& display) {
  #if BLE_ADVERT == 0
  return;  // Skip rendering when disabled
  #endif

  // Render nearby device count
}
```
**Status**: ✅ Widget rendering skipped when disabled

## Build Configuration

### WioTrackerL1Eink Companion Radio

File: `variants/wio-tracker-l1-eink/platformio.ini`

```ini
[env:WioTrackerL1Eink_companion_radio_ble]
build_flags =
  -D BLE_PIN_CODE=123456      # BLE enabled for serial interface
  -D BLE_ADVERT=0             # BLE discovery DISABLED
```

**Rationale**:
- E-ink display updates are slow
- BLE scanning consumes power
- Nearby device count would cause frequent refreshes
- Serial BLE interface still works for data transfer

## Common Patterns

### Pattern 1: Early Return in Functions
```cpp
void someFunction() {
  #if BLE_ADVERT == 0
  return;  // Skip entire function
  #endif

  // BLE-specific code here
}
```

### Pattern 2: Conditional Code Blocks
```cpp
void someFunction() {
  // Common code always runs

  #if BLE_ADVERT
  // BLE-specific code only when enabled
  auto* ble_mgr = getBLEDiscovery();
  ble_mgr->doSomething();
  #endif

  // More common code
}
```

### Pattern 3: Stub Implementations
```cpp
bool SomeClass::bleMethod() {
  #if BLE_ADVERT
  // Full implementation
  return true;
  #else
  return false;  // Stub - always fail
  #endif
}
```

### Pattern 4: Conditional Member Variables
```cpp
class MyClass {
  #if BLE_ADVERT
  BLEManager* ble_manager;
  unsigned long last_ble_update;
  #endif
};
```

## Why This Approach?

### Problem
The WioTrackerL1Eink build had:
- `BLE_PIN_CODE=123456` (BLE interface enabled)
- `BLE_ADVERT=0` (BLE discovery disabled)

This created a conflict:
- Code wanted to use `nRF52BLEDiscoveryManager` class
- But `#if BLE_ADVERT` wrapped the entire class definition
- Compilation failed with "class not defined" errors

### Solution
1. **Keep classes defined** - MyMesh needs `_ble_discovery` member variable
2. **Wrap implementations** - Methods become stubs that return early
3. **Conditional execution** - Application code checks flag before calling
4. **Zero overhead** - Unused code optimized out by compiler

### Benefits
- ✅ Compiles with any flag combination
- ✅ No runtime overhead when disabled
- ✅ Type safety maintained (class exists)
- ✅ Clean separation of concerns
- ✅ Easy to enable/disable per build

## Troubleshooting

### Compilation Error: "unterminated #if"
**Cause**: Missing `#endif` for `#if BLE_ADVERT`
**Fix**: Ensure every `#if BLE_ADVERT` has matching `#endif`

### Compilation Error: "class does not name a type"
**Cause**: Class wrapped in `#if BLE_ADVERT` but used unconditionally
**Fix**: Remove wrapper from class definition, wrap implementation instead

### Compilation Error: "no member named X"
**Cause**: Member variable/method wrapped but used unconditionally
**Fix**: Wrap usage site with `#if BLE_ADVERT` or remove wrapper from declaration

### Runtime: BLE not working when expected
**Cause**: `BLE_ADVERT=0` in build flags
**Fix**: Check `platformio.ini` build flags, change to `BLE_ADVERT=1`

### Runtime: Excessive power consumption
**Cause**: `BLE_ADVERT=1` enables continuous scanning
**Fix**: Set `BLE_ADVERT=0` to disable scanning but keep BLE interface

## Testing

### Test Matrix

| BLE_PIN_CODE | BLE_ADVERT | Expected Behavior |
|--------------|------------|-------------------|
| undefined    | 0          | No BLE at all |
| undefined    | 1          | No BLE at all |
| 123456       | 0          | BLE serial only, no discovery |
| 123456       | 1          | Full BLE with discovery |

### Verification Commands

```bash
# Check build flags
grep -r "BLE_ADVERT" variants/*/platformio.ini

# Find all usage sites
grep -r "BLE_ADVERT" --include="*.cpp" --include="*.h" src/ examples/

# Verify pattern consistency
grep -A 5 "#if BLE_ADVERT" src/helpers/BLEDiscoveryManager.cpp
```

## Future Improvements

### Potential Enhancements

1. **Add BLE_SCANNING flag** - Separate scanning from advertising
2. **Add BLE_AUTO_CONNECT flag** - Disable auto-connection but keep scanning
3. **Runtime enable/disable** - Allow toggling via settings menu
4. **Power profiles** - Preset configurations for different power budgets

### Code Health

- ✅ All conditionals reviewed (2025-10-20)
- ✅ Compilation tested with BLE_ADVERT=0
- ✅ Compilation tested with BLE_ADVERT=1 (default builds)
- ⚠️ Runtime testing needed for both modes
- ⚠️ Power consumption measurements needed

## Related Flags

### BLE_PIN_CODE
Enables basic BLE serial interface with PIN authentication.

**Independent of BLE_ADVERT** - you can have BLE serial without discovery.

### BLE_SCANNING
Used in `SerialBLEInterface.cpp`, controls whether scanner is active.

**Different from BLE_ADVERT** - BLE_ADVERT is higher level (discovery), BLE_SCANNING is lower level (radio scanning).

### BLE_DEBUG_LOGGING
Enables verbose BLE debug output.

**Independent** - can enable for debugging when BLE_ADVERT=0.

## References

- BLE Service Definitions: `src/helpers/BLEServiceDefinitions.h`
- BLE Discovery Manager: `src/helpers/BLEDiscoveryManager.h`
- Integration Guide: `BLE_RELAY_INTEGRATION.md`
- Contact Info: `src/helpers/ContactInfo.h`

---

*Last Updated: 2025-10-20*
*Author: Claude Code*
*Related: BLE_RELAY_INTEGRATION.md, CLAUDE.md*
