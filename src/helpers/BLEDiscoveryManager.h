#pragma once

#include "BLEServiceDefinitions.h"
#include "BaseChatMesh.h"
#include <Arduino.h>

#ifndef MESH_DEBUG_PRINTLN
#define MESH_DEBUG_PRINTLN(...) {}
#endif

/**
 * BLE Neighbor entry - discovered device via BLE scanning
 * Separate from full ContactInfo, used for UI display and tracking
 */
struct BLENeighbor {
    uint8_t device_hash;           // First byte of public key
    uint8_t mac_address[6];        // BLE MAC address
    int16_t rssi;                  // Last RSSI value
    double latitude;               // Last advertised lat
    double longitude;              // Last advertised lon
    uint32_t last_seen;            // millis() when last seen
    uint8_t device_type;           // ADV_TYPE_*
    bool has_location;             // Whether location is valid

    BLENeighbor() : device_hash(0), rssi(-128), latitude(0), longitude(0),
                    last_seen(0), device_type(0), has_location(false) {
        memset(mac_address, 0, 6);
    }
};

/**
 * BLE Discovery Manager
 *
 * Handles:
 * - Passive BLE scanning for MeshCore devices
 * - Manufacturer data parsing and filtering
 * - Automatic connection to unknown devices
 * - Contact creation with signature verification
 * - Location updates for known devices
 *
 * Works completely transparently - no user interaction needed.
 */
class BLEDiscoveryManager {
private:
    BaseChatMesh* mesh;
    bool is_scanning;
    bool is_connecting;
    unsigned long last_connection_attempt;

    // Neighbor table for UI display
    static const int MAX_NEIGHBORS = 16;
    BLENeighbor neighbors[MAX_NEIGHBORS];
    int neighbor_count;

    // Platform-specific client pointer (will be cast appropriately)
    void* ble_client;

    /**
     * Validate manufacturer data structure
     */
    bool validateManufacturerData(const BLEManufacturerData* data);

    /**
     * Check if device hash exists in contacts
     */
    ContactInfo* findContactByHash(uint8_t device_hash);

    /**
     * Update existing contact with BLE advertisement data
     */
    void updateKnownContact(ContactInfo* contact, const BLEManufacturerData* data, int rssi);

    /**
     * Add or update neighbor in table
     */
    void updateNeighborTable(uint8_t device_hash, const uint8_t* mac, const BLEManufacturerData* data, int rssi);

    /**
     * Remove stale neighbors (not seen in BLE_NEIGHBOR_TIMEOUT_MS)
     */
    void cleanupStaleNeighbors();

public:
    BLEDiscoveryManager();

    /**
     * Initialize discovery manager
     * @param mesh_instance Pointer to BaseChatMesh for contact access
     */
    void begin(BaseChatMesh* mesh_instance);

    /**
     * Process discovery in main loop
     * Handles auto-connection queue and cleanup
     */
    void loop();

    /**
     * Called when BLE advertisement received (from platform-specific scanner)
     * @param mac_addr BLE MAC address (6 bytes)
     * @param mfg_data Manufacturer data payload
     * @param mfg_len Length of manufacturer data
     * @param rssi Signal strength in dBm
     * @param device_name Optional device name from advertisement (can be nullptr)
     */
    void onAdvertisementReceived(const uint8_t* mac_addr, const uint8_t* mfg_data, size_t mfg_len, int rssi, const char* device_name = nullptr);

    /**
     * Connect to unknown device and read characteristics
     * Platform-specific implementation will override this
     * @param mac_addr BLE MAC address (6 bytes)
     * @param device_hash Expected hash (for verification)
     * @return true if contact successfully created
     */
    virtual bool connectAndReadDevice(const uint8_t* mac_addr, uint8_t device_hash) = 0;

    /**
     * Create contact from BLE-read data
     * @param pubkey 32-byte Ed25519 public key
     * @param device_info Device information
     * @param signature 64-byte Ed25519 signature
     * @return true if contact created successfully
     */
    bool createContactFromBLE(const uint8_t* pubkey, const BLEDeviceInfo* device_info, const uint8_t* signature);

    /**
     * Get neighbor by index (for UI display)
     */
    const BLENeighbor* getNeighbor(int index) const {
        if (index >= 0 && index < neighbor_count) {
            return &neighbors[index];
        }
        return nullptr;
    }

    /**
     * Get count of active neighbors
     */
    int getNeighborCount() const { return neighbor_count; }

    /**
     * Get all active (non-stale) neighbors
     * @param out_neighbors Output array to populate
     * @param max_count Maximum number of neighbors to return
     * @return Number of neighbors copied
     */
    int getActiveNeighbors(BLENeighbor* out_neighbors, int max_count);

    /**
     * Check if currently scanning
     */
    bool isScanning() const { return is_scanning; }
};
