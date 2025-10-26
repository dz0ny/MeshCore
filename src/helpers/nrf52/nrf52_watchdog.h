/**
 * NRF52 Watchdog Timer Utilities
 *
 * Prevents system freezes by automatically resetting the chip if the watchdog
 * is not fed within the configured timeout period.
 *
 * Usage:
 *   nrf52_wdt_init(10);  // Initialize with 10 second timeout
 *   nrf52_wdt_feed();    // Feed watchdog (call regularly in loops)
 */

#pragma once

#ifdef NRF52_PLATFORM

#include <nrf.h>

// Watchdog state
static bool _wdt_initialized = false;
static uint32_t _wdt_timeout_seconds = 0;

/**
 * Initialize the NRF52 watchdog timer
 *
 * @param timeout_seconds Watchdog timeout in seconds (1-131 seconds max)
 *                        If watchdog is not fed within this time, chip resets
 *
 * IMPORTANT: Once started, the watchdog CANNOT be stopped - only fed!
 *            The chip will reset if watchdog is not fed within timeout.
 */
inline void nrf52_wdt_init(uint32_t timeout_seconds = 10) {
  if (_wdt_initialized) {
    // Watchdog already running - cannot reinitialize
    return;
  }

  // Validate timeout (max ~131 seconds due to 32-bit counter at 32768 Hz)
  if (timeout_seconds < 1) timeout_seconds = 1;
  if (timeout_seconds > 131) timeout_seconds = 131;

  _wdt_timeout_seconds = timeout_seconds;

  // Configure watchdog
  // HALT: Pause watchdog during debug/sleep
  // SLEEP: Run watchdog during sleep (set to Run for maximum safety)
  NRF_WDT->CONFIG = (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) |
                    (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);

  // Set reload value (timeout in 32.768 kHz clock ticks)
  // CRV = timeout_seconds * 32768
  NRF_WDT->CRV = timeout_seconds * 32768;

  // Enable reload register 0 (RR0)
  NRF_WDT->RREN = WDT_RREN_RR0_Msk;

  // Start the watchdog
  NRF_WDT->TASKS_START = 1;

  _wdt_initialized = true;

  #if defined(ARDUINO) && defined(DEBUG)
  Serial.printf("NRF52 Watchdog: Initialized with %d second timeout\n", timeout_seconds);
  Serial.printf("NRF52 Watchdog: Feed regularly to prevent reset!\n");
  Serial.flush();
  #endif
}

/**
 * Feed (reload) the watchdog timer
 *
 * Call this regularly (well before timeout) to prevent system reset.
 * Safe to call even if watchdog is not initialized.
 */
inline void nrf52_wdt_feed() {
  if (!_wdt_initialized) {
    return; // Watchdog not running
  }

  // Reload watchdog timer by writing magic value to RR[0]
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

/**
 * Check if watchdog is initialized and running
 */
inline bool nrf52_wdt_is_running() {
  return _wdt_initialized;
}

/**
 * Get configured watchdog timeout in seconds
 */
inline uint32_t nrf52_wdt_get_timeout() {
  return _wdt_timeout_seconds;
}

/**
 * Watchdog panic - force immediate reset
 * Use this when you detect an unrecoverable error
 */
inline void nrf52_wdt_panic(const char* reason = nullptr) {
  #if defined(ARDUINO) && defined(DEBUG)
  Serial.printf("\n!!! WATCHDOG PANIC !!!\n");
  if (reason) {
    Serial.printf("Reason: %s\n", reason);
  }
  Serial.printf("Forcing system reset...\n");
  Serial.flush();
  delay(100); // Give serial time to flush
  #endif

  // Force immediate reset by triggering watchdog timeout
  // (Don't feed watchdog, wait for it to expire)
  while (1) {
    // Busy wait for watchdog reset
  }
}

/**
 * Safe delay with automatic watchdog feeding
 *
 * @param ms Delay time in milliseconds
 *
 * Unlike standard delay(), this function feeds the watchdog
 * automatically during long delays to prevent timeout.
 */
inline void nrf52_wdt_safe_delay(uint32_t ms) {
  if (!_wdt_initialized) {
    delay(ms);
    return;
  }

  unsigned long start = millis();
  while (millis() - start < ms) {
    // Feed watchdog every 500ms during delay
    static unsigned long last_feed = 0;
    if (millis() - last_feed > 500) {
      nrf52_wdt_feed();
      last_feed = millis();
    }
    yield(); // Allow background tasks
    delay(10);
  }
}

#endif // NRF52_PLATFORM
