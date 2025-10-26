#include "BLEDiscoveryManager.h"
#include "AdvertDataHelpers.h"
#include <Ed25519.h>

#if BLE_ADVERT
// All BLE discovery functionality disabled when BLE_ADVERT=0

BLEDiscoveryManager::BLEDiscoveryManager()
    : mesh(nullptr), is_scanning(false), is_connecting(false),
      last_connection_attempt(0), neighbor_count(0), ble_client(nullptr) {
}

void BLEDiscoveryManager::begin(BaseChatMesh* mesh_instance) {
    mesh = mesh_instance;
    is_scanning = true;
    MESH_DEBUG_PRINTLN("BLE Discovery: Started (transparent mode)");
}

void BLEDiscoveryManager::loop() {
    // Cleanup stale neighbors every 30 seconds
    static unsigned long last_cleanup = 0;
    if (millis() - last_cleanup > 30000) {
        cleanupStaleNeighbors();
        last_cleanup = millis();

        #ifdef MESH_DEBUG
        // Log neighbor table status
        if (neighbor_count > 0) {
            MESH_DEBUG_PRINTLN("BLE Discovery: Neighbor table has %d entries:", neighbor_count);
            unsigned long now = millis();
            for (int i = 0; i < neighbor_count; i++) {
                unsigned long age_sec = (now - neighbors[i].last_seen) / 1000;
                MESH_DEBUG_PRINTLN("  [%d] hash=%02X, rssi=%d, age=%lu sec",
                                 i, neighbors[i].device_hash, neighbors[i].rssi, age_sec);
            }
        } else {
            MESH_DEBUG_PRINTLN("BLE Discovery: Neighbor table is empty (no MeshCore devices detected)");
        }
        #endif
    }

    // Platform-specific scanning happens in callbacks
    // This loop is for future connection queue processing
}

bool BLEDiscoveryManager::validateManufacturerData(const BLEManufacturerData* data) {
    // Check size (should be validated by caller, but double-check)
    if (sizeof(BLEManufacturerData) != 26) {
        #ifdef MESH_DEBUG
        MESH_DEBUG_PRINTLN("BLE Discovery: REJECT - struct size %d != 26", sizeof(BLEManufacturerData));
        #endif
        return false;
    }

    // Check manufacturer ID
    if (data->manufacturer_id != MESHCORE_MANUFACTURER_ID) {
        #ifdef MESH_DEBUG
        MESH_DEBUG_PRINTLN("BLE Discovery: REJECT - manufacturer ID 0x%04X != 0x%04X",
                         data->manufacturer_id, MESHCORE_MANUFACTURER_ID);
        #endif
        return false;
    }

    // Check magic byte
    if (data->magic_byte != MESHCORE_MAGIC_BYTE) {
        #ifdef MESH_DEBUG
        MESH_DEBUG_PRINTLN("BLE Discovery: REJECT - magic byte 0x%02X != 0x%02X",
                         data->magic_byte, MESHCORE_MAGIC_BYTE);
        #endif
        return false;
    }

    // Check protocol version
    if (data->protocol_version > MAX_SUPPORTED_PROTOCOL_VERSION) {
        MESH_DEBUG_PRINTLN("BLE Discovery: REJECT - unsupported protocol v%d > v%d",
                         data->protocol_version, MAX_SUPPORTED_PROTOCOL_VERSION);
        return false;
    }

    // Validate CRC16
    uint16_t calculated_crc = ble_crc16_calc((const uint8_t*)data, 18);
    if (data->crc16 != calculated_crc) {
        #ifdef MESH_DEBUG
        MESH_DEBUG_PRINTLN("BLE Discovery: REJECT - CRC mismatch (calculated=0x%04X, advertised=0x%04X)",
                         calculated_crc, data->crc16);
        #endif
        return false;
    }

    return true;
}

ContactInfo* BLEDiscoveryManager::findContactByHash(uint8_t device_hash) {
    if (!mesh) return nullptr;

    // Search contacts by first byte of public key
    return mesh->findContactByHash(device_hash);
}

void BLEDiscoveryManager::updateKnownContact(ContactInfo* contact, const BLEManufacturerData* data, int rssi) {
    #if BLE_ADVERT
    unsigned long now = millis();

    // Throttle contact updates to every 25 seconds
    if (now - contact->last_seen_ble < 25000) {
        #ifdef MESH_DEBUG
        MESH_DEBUG_PRINTLN("BLE Discovery: Skipping contact update for %s (throttled, last seen %lu ms ago)",
                         contact->name, now - contact->last_seen_ble);
        #endif
        return; // Skip this update
    }

    // Update last seen timestamp
    contact->last_seen_ble = now;
    contact->ble_rssi = rssi;

    // Update location if advertised
    if (BLE_FLAGS_HAS_LOCATION(data->flags)) {
        contact->last_known_lat = ((double)data->latitude) / 100000.0;  // 1E5 for meter accuracy
        contact->last_known_lon = ((double)data->longitude) / 100000.0;  // 1E5 for meter accuracy
        contact->location_source = LOCATION_SOURCE_BLE;
        contact->location_timestamp = data->timestamp;

        MESH_DEBUG_PRINTLN("BLE Discovery: Updated location for %s: %.5f, %.5f (RSSI: %d)",
                         contact->name,
                         contact->last_known_lat,
                         contact->last_known_lon,
                         rssi);
    }
    #endif
}

void BLEDiscoveryManager::updateNeighborTable(uint8_t device_hash, const uint8_t* mac, const BLEManufacturerData* data, int rssi) {
    // Find existing neighbor or create new entry
    BLENeighbor* neighbor = nullptr;
    unsigned long now = millis();

    for (int i = 0; i < neighbor_count; i++) {
        if (neighbors[i].device_hash == device_hash) {
            neighbor = &neighbors[i];

            // Throttle updates to every 25 seconds for existing neighbors
            if (now - neighbor->last_seen < 25000) {
                #ifdef MESH_DEBUG
                MESH_DEBUG_PRINTLN("BLE Discovery: Skipping update for neighbor hash=%02X (throttled, last seen %lu ms ago)",
                                 device_hash, now - neighbor->last_seen);
                #endif
                return; // Skip this update
            }

            #ifdef MESH_DEBUG
            MESH_DEBUG_PRINTLN("BLE Discovery: Updating existing neighbor at index %d, hash=%02X", i, device_hash);
            #endif
            break;
        }
    }

    // If not found and we have space, create new
    if (!neighbor && neighbor_count < MAX_NEIGHBORS) {
        neighbor = &neighbors[neighbor_count];
        #ifdef MESH_DEBUG
        MESH_DEBUG_PRINTLN("BLE Discovery: Adding new neighbor at index %d, hash=%02X (total: %d)",
                         neighbor_count, device_hash, neighbor_count + 1);
        #endif
        neighbor_count++;
    }

    // If still no space, replace oldest entry
    if (!neighbor) {
        unsigned long oldest_time = 0xFFFFFFFF;
        int oldest_idx = 0;
        for (int i = 0; i < MAX_NEIGHBORS; i++) {
            if (neighbors[i].last_seen < oldest_time) {
                oldest_time = neighbors[i].last_seen;
                oldest_idx = i;
            }
        }
        neighbor = &neighbors[oldest_idx];
        #ifdef MESH_DEBUG
        MESH_DEBUG_PRINTLN("BLE Discovery: Neighbor table full, replacing oldest at index %d", oldest_idx);
        #endif
    }

    // Update neighbor data
    neighbor->device_hash = device_hash;
    memcpy(neighbor->mac_address, mac, 6);
    neighbor->rssi = rssi;
    neighbor->last_seen = millis();
    neighbor->device_type = BLE_FLAGS_GET_TYPE(data->flags);
    neighbor->has_location = BLE_FLAGS_HAS_LOCATION(data->flags);

    if (neighbor->has_location) {
        neighbor->latitude = ((double)data->latitude) / 100000.0;  // 1E5 for meter accuracy
        neighbor->longitude = ((double)data->longitude) / 100000.0;  // 1E5 for meter accuracy
        #ifdef MESH_DEBUG
        MESH_DEBUG_PRINTLN("BLE Discovery: Neighbor has location: %.5f, %.5f",
                         neighbor->latitude, neighbor->longitude);
        #endif
    }

    #ifdef MESH_DEBUG
    MESH_DEBUG_PRINTLN("BLE Discovery: Neighbor table now has %d entries", neighbor_count);
    #endif
}

void BLEDiscoveryManager::cleanupStaleNeighbors() {
    unsigned long now = millis();

    for (int i = 0; i < neighbor_count; i++) {
        if (now - neighbors[i].last_seen > BLE_NEIGHBOR_TIMEOUT_MS) {
            // Remove stale neighbor by shifting array
            MESH_DEBUG_PRINTLN("BLE Discovery: Removing stale neighbor hash=%02X", neighbors[i].device_hash);

            for (int j = i; j < neighbor_count - 1; j++) {
                neighbors[j] = neighbors[j + 1];
            }
            neighbor_count--;
            i--; // Check this index again
        }
    }
}

int BLEDiscoveryManager::getActiveNeighbors(BLENeighbor* out_neighbors, int max_count) {
    if (!out_neighbors || max_count <= 0) {
        return 0;
    }

    unsigned long now = millis();
    int copied = 0;

    #ifdef MESH_DEBUG
    MESH_DEBUG_PRINTLN("BLE Discovery: getActiveNeighbors() called, neighbor_count=%d", neighbor_count);
    #endif

    for (int i = 0; i < neighbor_count && copied < max_count; i++) {
        unsigned long age_ms = now - neighbors[i].last_seen;
        // Only include non-stale neighbors
        if (age_ms <= BLE_NEIGHBOR_TIMEOUT_MS) {
            out_neighbors[copied] = neighbors[i];
            #ifdef MESH_DEBUG
            MESH_DEBUG_PRINTLN("BLE Discovery: Active neighbor %d: hash=%02X, age=%lu ms",
                             copied, neighbors[i].device_hash, age_ms);
            #endif
            copied++;
        } else {
            #ifdef MESH_DEBUG
            MESH_DEBUG_PRINTLN("BLE Discovery: Neighbor %d: hash=%02X STALE (age=%lu ms > %lu ms)",
                             i, neighbors[i].device_hash, age_ms, (unsigned long)BLE_NEIGHBOR_TIMEOUT_MS);
            #endif
        }
    }

    #ifdef MESH_DEBUG
    MESH_DEBUG_PRINTLN("BLE Discovery: Returning %d active neighbors", copied);
    #endif

    return copied;
}

void BLEDiscoveryManager::onAdvertisementReceived(const uint8_t* mac_addr, const uint8_t* mfg_data, size_t mfg_len, int rssi, const char* device_name) {
    static unsigned long last_verbose_log = 0;
    static int total_received = 0;
    static int meshcore_prefix_count = 0;

    total_received++;

    // NOTE: parseReportByType() INCLUDES the manufacturer ID in the returned data
    // The mfg_data buffer layout is: [mfg_id_low][mfg_id_high][magic_byte][protocol_version]...
    // Expected total size: 26 bytes (2-byte manufacturer ID + 24 bytes of data)

    // Quick check for MeshCore prefix before logging (only log MeshCore devices)
    bool is_meshcore_prefix = false;
    if (mfg_len >= 4) {
        uint16_t mfg_id = (mfg_data[1] << 8) | mfg_data[0];  // Extract manufacturer ID
        uint8_t magic = mfg_data[2];           // Third byte is magic_byte
        uint8_t protocol = mfg_data[3];        // Fourth byte is protocol_version
        is_meshcore_prefix = (mfg_id == MESHCORE_MANUFACTURER_ID && magic == MESHCORE_MAGIC_BYTE && protocol <= MAX_SUPPORTED_PROTOCOL_VERSION);

        if (is_meshcore_prefix) {
            meshcore_prefix_count++;
        }
    }

    #ifdef MESH_DEBUG
    // Log statistics every 10 seconds
    if (millis() - last_verbose_log > 10000) {
        MESH_DEBUG_PRINTLN("BLE Discovery: Received %d advertisements, %d with MeshCore prefix",
                         total_received, meshcore_prefix_count);
        total_received = 0;
        meshcore_prefix_count = 0;
        last_verbose_log = millis();
    }

    if (is_meshcore_prefix) {
        // Log with device name if available
        if (device_name && device_name[0] != '\0') {
            MESH_DEBUG_PRINTLN("BLE Discovery: MeshCore advertisement from '%s', len=%d, rssi=%d dBm",
                             device_name, mfg_len, rssi);
        } else {
            MESH_DEBUG_PRINTLN("BLE Discovery: MeshCore advertisement received, len=%d, rssi=%d dBm",
                             mfg_len, rssi);
        }

        // Hex dump of advertisement data
        MESH_DEBUG_PRINT("BLE Discovery: Hex dump: ");
        for (size_t i = 0; i < mfg_len; i++) {
            MESH_DEBUG_PRINT("%02X ", mfg_data[i]);
        }
        MESH_DEBUG_PRINTLN("");

        // Also log MAC address for identification
        MESH_DEBUG_PRINTLN("BLE Discovery: MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                      mac_addr[5], mac_addr[4], mac_addr[3], mac_addr[2], mac_addr[1], mac_addr[0]);
    }
    #endif

    // Validate size (parseReportByType includes manufacturer ID, so we expect full 26 bytes)
    const size_t expected_size = sizeof(BLEManufacturerData); // 26 bytes total
    if (mfg_len != expected_size) {
        #ifdef MESH_DEBUG
        if (is_meshcore_prefix) {
            MESH_DEBUG_PRINTLN("BLE Discovery: REJECT - wrong size %d != %d (expected %d bytes)",
                             mfg_len, expected_size, expected_size);
        }
        #endif
        return; // Wrong size, not MeshCore
    }

    // Cast the manufacturer data directly - it already has the full structure
    // parseReportByType() includes the manufacturer ID, so no reconstruction needed
    const BLEManufacturerData* data = (const BLEManufacturerData*)mfg_data;

    // Validate manufacturer data (filters non-MeshCore devices)
    if (!validateManufacturerData(data)) {
        #ifdef MESH_DEBUG
        if (is_meshcore_prefix) {
            MESH_DEBUG_PRINTLN("BLE Discovery: REJECT - invalid manufacturer data (ID=0x%04X, magic=0x%02X)",
                             data->manufacturer_id, data->magic_byte);
        }
        #endif
        return; // Not a valid MeshCore device
    }

    uint8_t device_hash = data->device_hash;

    #ifdef MESH_DEBUG
    MESH_DEBUG_PRINTLN("BLE Discovery: Valid MeshCore device, hash=%02X", device_hash);
    #endif

    // Update neighbor table (for UI display)
    updateNeighborTable(device_hash, mac_addr, data, rssi);

    // Check if device is already in contacts
    ContactInfo* contact = findContactByHash(device_hash);

    if (contact != nullptr) {
        // ═══════════════════════════════════════════
        // KNOWN DEVICE: Update location from BLE
        // ═══════════════════════════════════════════
        #ifdef MESH_DEBUG
        MESH_DEBUG_PRINTLN("BLE Discovery: Device hash=%02X found in contacts: %s (rssi=%d dBm)",
                         device_hash, contact->name, rssi);
        #endif
        updateKnownContact(contact, data, rssi);
        return; // Done, no connection needed
    }

    #ifdef MESH_DEBUG
    MESH_DEBUG_PRINTLN("BLE Discovery: Device hash=%02X NOT in contacts, checking auto-connect", device_hash);
    #endif

    // ═══════════════════════════════════════════
    // UNKNOWN DEVICE: Auto-connect to get full key
    // ═══════════════════════════════════════════

    // Only connect if:
    // 1. Not currently connecting
    // 2. RSSI is strong enough
    // 3. Haven't attempted connection too recently
    if (!is_connecting &&
        rssi > MIN_RSSI_FOR_CONNECTION &&
        millis() - last_connection_attempt > 10000) { // 10 second cooldown

        MESH_DEBUG_PRINTLN("BLE Discovery: Unknown device hash=%02X, RSSI=%d, auto-connecting...", device_hash, rssi);

        is_connecting = true;
        last_connection_attempt = millis();

        // Platform-specific connection (will be implemented in ESP32/nRF52 subclasses)
        bool success = connectAndReadDevice(mac_addr, device_hash);

        is_connecting = false;

        if (success) {
            MESH_DEBUG_PRINTLN("BLE Discovery: Successfully added new contact");
        }
    }
}

bool BLEDiscoveryManager::createContactFromBLE(const uint8_t* pubkey, const BLEDeviceInfo* device_info, const uint8_t* signature) {
    if (!mesh) return false;

    // Build message that was signed: pubkey || timestamp || device_info
    uint8_t msg_to_verify[32 + 4 + sizeof(BLEDeviceInfo)];
    memcpy(msg_to_verify, pubkey, 32);
    memcpy(msg_to_verify + 32, &device_info->timestamp, 4);
    memcpy(msg_to_verify + 36, device_info, sizeof(BLEDeviceInfo));

    // Verify Ed25519 signature
    if (!Ed25519::verify(signature, pubkey, msg_to_verify, sizeof(msg_to_verify))) {
        MESH_DEBUG_PRINTLN("BLE Discovery: Signature verification FAILED - spoofed device!");
        return false;
    }

    MESH_DEBUG_PRINTLN("BLE Discovery: Signature verified ✓");

    // Create ContactInfo (same structure as LoRa contacts)
    ContactInfo contact;
    memcpy(contact.pub_key, pubkey, PUB_KEY_SIZE);
    strncpy(contact.name, device_info->name, sizeof(contact.name) - 1);
    contact.name[sizeof(contact.name) - 1] = '\0'; // Ensure null termination
    contact.last_advert_timestamp = device_info->timestamp;
    contact.device_type = device_info->device_type;
    contact.out_path_len = -1; // Unknown path initially

    #if BLE_ADVERT
    // BLE-specific fields
    contact.last_seen_ble = millis();
    contact.ble_rssi = -128; // Will be updated on next advertisement
    contact.last_known_lat = 0;
    contact.last_known_lon = 0;
    contact.location_source = LOCATION_SOURCE_UNKNOWN;
    contact.location_timestamp = 0;
    #endif

    // Add to contact database
    bool added = mesh->addContact(contact);

    if (added) {
        MESH_DEBUG_PRINTLN("BLE Discovery: Auto-added contact: %s (hash: %02X)", contact.name, pubkey[0]);
        // Note: Mesh should save contacts to storage
    } else {
        MESH_DEBUG_PRINTLN("BLE Discovery: Failed to add contact (database full?)");
    }

    return added;
}

#endif // BLE_ADVERT
