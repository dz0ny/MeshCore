# NRF52 Watchdog Timer Documentation

## Overview

The NRF52 watchdog timer is a hardware feature that automatically resets the chip if the software stops responding (freezes). This is critical for embedded systems that need to recover from bugs, BLE stack hangs, or infinite loops.

## How It Works

1. **Initialization**: The watchdog is started AFTER all setup completes (timeout: 5 seconds)
2. **Feeding**: Your code must "feed" the watchdog before the timeout expires
3. **Reset**: If the watchdog is not fed in time, the chip automatically resets

**IMPORTANT**: Once started, the watchdog CANNOT be stopped - only fed!

**Why start after setup?** Initialization (QSPI flash, radio, filesystem) can take up to 15 seconds. The watchdog starts at the END of `setup()` so initialization completes without timeout pressure.

## Usage

### Basic Setup

The watchdog is automatically initialized at the END of `setup()` in `main.cpp` for all NRF52 platforms:

```cpp
#ifdef NRF52_PLATFORM
  nrf52_wdt_init(5);  // 5 second timeout
  Serial.println("NRF52: Watchdog initialized (5s timeout) - initialization complete");
#endif
```

This happens AFTER all critical initialization (flash, radio, BLE, filesystem) completes.

### Feeding the Watchdog

In any long-running operation, feed the watchdog regularly:

```cpp
#include <helpers/nrf52/nrf52_watchdog.h>

while (waiting_for_something) {
  nrf52_wdt_feed();  // Feed every iteration
  delay(100);
}
```

### Safe Delays

Use `nrf52_wdt_safe_delay()` for long delays with automatic feeding:

```cpp
// Bad - will trigger watchdog!
delay(6000);  // 6 seconds > 5 second watchdog timeout!

// Good - automatically feeds watchdog every 500ms
nrf52_wdt_safe_delay(6000);
```

### Manual Panic/Reset

Force an immediate reset when you detect an unrecoverable error:

```cpp
if (critical_error_detected) {
  nrf52_wdt_panic("Critical error: Flash corruption");
  // Never returns - chip resets
}
```

## Where Watchdog is Fed

The watchdog is automatically fed in these critical locations:

### 1. BLE Connection Wait Loop
**Location**: `src/helpers/nrf52/SerialBLEInterface.cpp:513-528`

Prevents freeze when waiting for BLE central connection to establish.

```cpp
while (!_client_connected && (millis() - connect_start < CONNECT_TIMEOUT)) {
  if (millis() - last_feed > 500) {
    nrf52_wdt_feed();  // Every 500ms
  }
  yield();
  delay(10);
}
```

### 2. Service & Characteristic Discovery
**Location**: `src/helpers/nrf52/SerialBLEInterface.cpp:540-589`

Prevents freeze during BLE GATT operations:
- Service discovery
- Characteristic discovery (3 characteristics)
- Data reading operations

### 3. Error Handler
**Location**: `examples/companion_radio/main.cpp:10-23`

The NRF52 SoftDevice error handler feeds watchdog during panic:

```cpp
extern "C" void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
  Serial.printf("!!! NRF52 FATAL ERROR !!!\n");
  Serial.flush();
  delay(3000);
  nrf52_wdt_panic("SoftDevice error");  // Controlled reset
}
```

## Testing the Watchdog

### Test 1: Verify Watchdog is Running

Check serial output on boot:

```
NRF52 Watchdog: Initialized with 10 second timeout
NRF52 Watchdog: Feed regularly to prevent reset!
```

### Test 2: Intentional Freeze Test

Add this temporary code to test watchdog reset:

```cpp
void setup() {
  // ... normal setup ...

  #ifdef NRF52_PLATFORM
  // WARNING: This will freeze the device and trigger watchdog reset!
  // Remove this after testing!
  Serial.println("Testing watchdog in 5 seconds...");
  delay(5000);
  Serial.println("Freezing now - watchdog should reset in 10 seconds");
  while(1) {
    // Infinite loop without feeding watchdog
    // Chip should reset automatically after 10 seconds
  }
  #endif
}
```

Expected behavior:
1. Device prints "Freezing now..."
2. After ~5 seconds, chip resets automatically
3. Device reboots and prints "NRF52: Watchdog initialized (5s timeout)..."

### Test 3: BLE Freeze Protection

Monitor serial output during BLE operations:

```
BLE: Attempting BLE connection...
Waiting for connection... 2000 ms elapsed
Waiting for connection... 4000 ms elapsed
```

The watchdog is being fed every 500ms during this wait.

## Debugging Watchdog Resets

If your device keeps resetting unexpectedly:

### 1. Check for Long Operations

Look for code that takes >10 seconds without feeding:

```cpp
// BAD - will trigger watchdog
for (int i = 0; i < 1000000; i++) {
  expensive_operation();
}

// GOOD - feeds watchdog
for (int i = 0; i < 1000000; i++) {
  expensive_operation();
  if (i % 1000 == 0) {
    nrf52_wdt_feed();
  }
}
```

### 2. Check Busy Waits

Replace tight loops with yield() and watchdog feeds:

```cpp
// BAD
while (!condition) {
  delay(10);
}

// GOOD
while (!condition) {
  nrf52_wdt_feed();
  yield();
  delay(10);
}
```

### 3. Enable BLE Debug Logging

In `platformio.ini`:

```ini
build_flags =
  -D BLE_DEBUG_LOGGING=1
  -D DEBUG
```

Look for stuck operations in serial output.

## API Reference

### `nrf52_wdt_init(timeout_seconds)`
- **Parameters**: `timeout_seconds` (1-131 seconds, default: 5)
- **Returns**: void
- **Description**: Initializes and starts the watchdog timer
- **Note**: Can only be called once - watchdog cannot be reinitialized
- **Important**: Should be called at END of setup(), not beginning!

### `nrf52_wdt_feed()`
- **Parameters**: none
- **Returns**: void
- **Description**: Resets the watchdog timer countdown
- **Note**: Safe to call even if watchdog not initialized

### `nrf52_wdt_is_running()`
- **Parameters**: none
- **Returns**: bool (true if watchdog is running)
- **Description**: Check if watchdog is initialized and active

### `nrf52_wdt_get_timeout()`
- **Parameters**: none
- **Returns**: uint32_t (timeout in seconds)
- **Description**: Get configured watchdog timeout value

### `nrf52_wdt_panic(reason)`
- **Parameters**: `reason` (optional const char* error message)
- **Returns**: never returns - forces reset
- **Description**: Force immediate chip reset via watchdog

### `nrf52_wdt_safe_delay(ms)`
- **Parameters**: `ms` (milliseconds to delay)
- **Returns**: void
- **Description**: Delay with automatic watchdog feeding every 500ms

## Troubleshooting

### Device keeps resetting every 5 seconds

**Cause**: Main loop is not feeding the watchdog or long operation without feeding

**Solution**: Add `nrf52_wdt_feed()` to your main loop or long operations

### Device freezes without resetting

**Cause**: Watchdog not initialized or freeze occurs before initialization

**Solution**:
1. Check that `nrf52_wdt_init()` is called in setup()
2. Move watchdog init earlier in setup() if needed

### BLE operations still freeze

**Cause**: Additional BLE operations not covered by watchdog feeds

**Solution**: Add `nrf52_wdt_feed()` calls to your custom BLE code

### How to disable watchdog for debugging

**Not possible** - the NRF52 watchdog cannot be stopped once started.

**Workaround**: Comment out `nrf52_wdt_init()` in main.cpp and recompile:

```cpp
#ifdef NRF52_PLATFORM
  // nrf52_wdt_init(10);  // DISABLED FOR DEBUGGING
#endif
```

## Best Practices

1. **Feed regularly**: In main loop, feed every iteration or every second
2. **Feed before blocking operations**: BLE connects, flash writes, sensor reads
3. **Use safe_delay()**: For delays >1 second, use `nrf52_wdt_safe_delay()`
4. **Don't feed too often**: Feeding every millisecond wastes CPU - 500ms is ideal
5. **Test thoroughly**: Intentionally trigger watchdog during development
6. **Log resets**: Track unexpected resets to find bugs

## Implementation Details

### Hardware Registers

The watchdog uses these NRF52 registers:
- `NRF_WDT->CONFIG`: Watchdog configuration (halt/sleep behavior)
- `NRF_WDT->CRV`: Counter reload value (timeout in 32.768 kHz ticks)
- `NRF_WDT->RREN`: Reload register enable
- `NRF_WDT->RR[0]`: Reload register (write magic value to feed)
- `NRF_WDT->TASKS_START`: Start watchdog

### Clock Source

The watchdog runs on the 32.768 kHz low-frequency clock (LFCLK), which is always running even in sleep modes.

### Sleep Behavior

The watchdog continues running during sleep modes. The current configuration:
- **HALT**: Pauses during debug (allows debugging without resets)
- **SLEEP**: Runs during sleep (ensures recovery from sleep hangs)

---

**Last Updated**: 2025-10-20
**Applies To**: All NRF52 platforms (RAK4631, Heltec T114, etc.)
