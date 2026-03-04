#ifndef NRF52_WATCHDOG_H
#define NRF52_WATCHDOG_H

#ifdef NRF52_PLATFORM

#include <Arduino.h>

namespace nrf52 {

/**
 * Initialize the NRF52 hardware watchdog timer
 * @param timeout_ms Watchdog timeout in milliseconds (default: 30000ms = 30s)
 */
void initWatchdog(uint32_t timeout_ms = 30000);

/**
 * Reset/feed the watchdog timer to prevent system reset
 * Call this function regularly to keep the system running
 */
void resetWatchdog();

} // namespace nrf52

#endif // NRF52_PLATFORM

#endif // NRF52_WATCHDOG_H
