#pragma once

#include "../BLEDiscoveryManager.h"

// Forward declaration
class SerialBLEInterface;

/**
 * nRF52-specific BLE Discovery Manager
 * Implements BLE client connection for auto-discovery
 * Delegates actual connection to SerialBLEInterface
 */
class nRF52BLEDiscoveryManager : public BLEDiscoveryManager {
private:
    SerialBLEInterface* ble_interface;

public:
    nRF52BLEDiscoveryManager();
    ~nRF52BLEDiscoveryManager();

    /**
     * Set the BLE interface that will handle connections
     * @param interface Pointer to SerialBLEInterface
     */
    void setBLEInterface(SerialBLEInterface* interface);

    /**
     * Connect to unknown device and read characteristics
     * @param mac_addr BLE MAC address (6 bytes)
     * @param device_hash Expected hash (for verification)
     * @return true if contact successfully created
     */
    bool connectAndReadDevice(const uint8_t* mac_addr, uint8_t device_hash) override;
};
