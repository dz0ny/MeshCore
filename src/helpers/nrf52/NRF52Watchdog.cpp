#include "NRF52Watchdog.h"
#include <nrf_wdt.h>

namespace nrf52 {

static nrf_wdt_rr_register_t s_reload_register = NRF_WDT_RR0;

void initWatchdog(uint32_t timeout_ms) {
    // Configure watchdog
    // WDT runs at 32768 Hz
    // Reload value = (timeout_ms * 32768) / 1000
    uint32_t reload_value = (timeout_ms * 32768UL) / 1000UL;

    // Configure the watchdog
    nrf_wdt_behaviour_set(NRF_WDT, NRF_WDT_BEHAVIOUR_RUN_SLEEP);  // Run in sleep mode
    nrf_wdt_reload_value_set(NRF_WDT, reload_value);

    // Enable reload register 0
    nrf_wdt_reload_request_enable(NRF_WDT, s_reload_register);

    // Start the watchdog
    nrf_wdt_task_trigger(NRF_WDT, NRF_WDT_TASK_START);
}

void resetWatchdog() {
    // Feed the watchdog by reloading the counter
    nrf_wdt_reload_request_set(NRF_WDT, s_reload_register);
}

} // namespace nrf52
