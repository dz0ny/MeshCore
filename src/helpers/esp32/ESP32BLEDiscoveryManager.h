#pragma once

#include "../BLEDiscoveryManager.h"
#include <BLEDevice.h>
#include <BLEClient.h>

/**
 * ESP32-specific BLE Discovery Manager
 * Implements BLE client connection for auto-discovery
 */
class ESP32BLEDiscoveryManager : public BLEDiscoveryManager {
private:
    BLEClient* pBLEClient;

public:
    ESP32BLEDiscoveryManager();
    ~ESP32BLEDiscoveryManager();

    /**
     * Connect to unknown device and read characteristics
     * @param mac_addr BLE MAC address (6 bytes)
     * @param device_hash Expected hash (for verification)
     * @return true if contact successfully created
     */
    bool connectAndReadDevice(const uint8_t* mac_addr, uint8_t device_hash) override;
};
