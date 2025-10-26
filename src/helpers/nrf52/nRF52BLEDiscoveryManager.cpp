#include "nRF52BLEDiscoveryManager.h"
#include "SerialBLEInterface.h"

// Use existing BLE debug logging from SerialBLEInterface.h
#ifndef BLE_DEBUG_PRINTLN
  #if BLE_DEBUG_LOGGING && ARDUINO
    #include <Arduino.h>
    #define BLE_DEBUG_PRINTLN(F, ...) Serial.printf("BLE: " F "\n", ##__VA_ARGS__)
  #else
    #define BLE_DEBUG_PRINTLN(...) {}
  #endif
#endif

nRF52BLEDiscoveryManager::nRF52BLEDiscoveryManager()
    : BLEDiscoveryManager(), ble_interface(nullptr) {
}

nRF52BLEDiscoveryManager::~nRF52BLEDiscoveryManager() {
    // Don't delete ble_interface - we don't own it
}

void nRF52BLEDiscoveryManager::setBLEInterface(SerialBLEInterface* interface) {
    ble_interface = interface;

#if BLE_ADVERT
    // Set up bidirectional relationship - the interface needs to know about us
    // for calling createContactFromBLE() in connectAndReadDevice()
    if (ble_interface) {
        ble_interface->discovery_manager = this;
    }
#endif
}

bool nRF52BLEDiscoveryManager::connectAndReadDevice(const uint8_t* mac_addr, uint8_t device_hash) {
#if BLE_ADVERT
    if (!ble_interface) {
        BLE_DEBUG_PRINTLN("nRF52BLE: BLE interface not set!");
        return false;
    }

    if (!mac_addr) {
        BLE_DEBUG_PRINTLN("nRF52BLE: connectAndReadDevice called with NULL mac_addr");
        return false;
    }

    BLE_DEBUG_PRINTLN("nRF52BLE: Delegating connection to SerialBLEInterface");

    // Delegate to SerialBLEInterface which has all the nRF52-specific BLE client logic
    return ble_interface->connectAndReadDevice(mac_addr, device_hash);
#else
    // BLE_ADVERT disabled - return false immediately
    return false;
#endif
}
