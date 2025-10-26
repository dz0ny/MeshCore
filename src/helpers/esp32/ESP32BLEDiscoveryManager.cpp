#include "ESP32BLEDiscoveryManager.h"
#include "../BLEServiceDefinitions.h"
#include <Arduino.h>

// Use existing BLE debug logging from SerialBLEInterface.h
#if BLE_DEBUG_LOGGING && ARDUINO
  #define BLE_DEBUG_PRINTLN(F, ...) Serial.printf("BLE: " F "\n", ##__VA_ARGS__)
#else
  #define BLE_DEBUG_PRINTLN(...) {}
#endif

#define BLE_CONNECTION_TIMEOUT_MS 10000  // 10 second timeout

ESP32BLEDiscoveryManager::ESP32BLEDiscoveryManager()
    : BLEDiscoveryManager(), pBLEClient(nullptr) {
}

ESP32BLEDiscoveryManager::~ESP32BLEDiscoveryManager() {
    if (pBLEClient) {
        delete pBLEClient;
        pBLEClient = nullptr;
    }
}

bool ESP32BLEDiscoveryManager::connectAndReadDevice(const uint8_t* mac_addr, uint8_t device_hash) {
    if (!mac_addr) {
        BLE_DEBUG_PRINTLN("ESP32BLE: connectAndReadDevice called with NULL mac_addr");
        return false;
    }

    // Convert MAC address to BLEAddress (need to cast away const for ESP32 BLE API)
    BLEAddress bleAddress(const_cast<uint8_t*>(mac_addr));

    BLE_DEBUG_PRINTLN("ESP32BLE: Attempting to connect to %s (hash: %02X)",
                     bleAddress.toString().c_str(), device_hash);

    // Create BLE client if not already created
    if (!pBLEClient) {
        pBLEClient = BLEDevice::createClient();
        if (!pBLEClient) {
            BLE_DEBUG_PRINTLN("ESP32BLE: Failed to create BLE client");
            return false;
        }
        BLE_DEBUG_PRINTLN("ESP32BLE: BLE client created");
    }

    // Try to connect to the device
    unsigned long start_time = millis();
    bool connected = false;

    try {
        // Disconnect if already connected
        if (pBLEClient->isConnected()) {
            pBLEClient->disconnect();
            delay(100);  // Brief delay after disconnect
        }

        // Attempt connection with timeout
        connected = pBLEClient->connect(bleAddress);

        if (!connected) {
            BLE_DEBUG_PRINTLN("ESP32BLE: Failed to connect to device");
            return false;
        }

        BLE_DEBUG_PRINTLN("ESP32BLE: Connected successfully");

        // Get MeshCore service
        BLERemoteService* pRemoteService = pBLEClient->getService(BLEUUID(MESHCORE_SERVICE_UUID));

        if (pRemoteService == nullptr) {
            BLE_DEBUG_PRINTLN("ESP32BLE: Failed to find MeshCore service");
            pBLEClient->disconnect();
            return false;
        }

        BLE_DEBUG_PRINTLN("ESP32BLE: Found MeshCore service");

        // Get all three characteristics
        BLERemoteCharacteristic* pPubKeyChar = pRemoteService->getCharacteristic(BLEUUID(MESHCORE_PUBKEY_UUID));
        BLERemoteCharacteristic* pDeviceInfoChar = pRemoteService->getCharacteristic(BLEUUID(MESHCORE_DEVICE_INFO_UUID));
        BLERemoteCharacteristic* pSignatureChar = pRemoteService->getCharacteristic(BLEUUID(MESHCORE_SIGNATURE_UUID));

        if (pPubKeyChar == nullptr || pDeviceInfoChar == nullptr || pSignatureChar == nullptr) {
            BLE_DEBUG_PRINTLN("ESP32BLE: Failed to find required characteristics");
            pBLEClient->disconnect();
            return false;
        }

        BLE_DEBUG_PRINTLN("ESP32BLE: Found all characteristics");

        // Read public key (32 bytes)
        std::string pubKeyValue = pPubKeyChar->readValue();

        if (pubKeyValue.length() != 32) {
            BLE_DEBUG_PRINTLN("ESP32BLE: Invalid public key length: %d", pubKeyValue.length());
            pBLEClient->disconnect();
            return false;
        }

        // Verify device hash matches
        uint8_t read_hash = (uint8_t)pubKeyValue[0];
        if (read_hash != device_hash) {
            BLE_DEBUG_PRINTLN("ESP32BLE: Device hash mismatch! Expected %02X, got %02X",
                            device_hash, read_hash);
            pBLEClient->disconnect();
            return false;
        }

        BLE_DEBUG_PRINTLN("ESP32BLE: Public key read (32 bytes), hash verified");

        // Read device info
        std::string deviceInfoValue = pDeviceInfoChar->readValue();

        if (deviceInfoValue.length() != sizeof(BLEDeviceInfo)) {
            BLE_DEBUG_PRINTLN("ESP32BLE: Invalid device info length: %d", deviceInfoValue.length());
            pBLEClient->disconnect();
            return false;
        }

        BLE_DEBUG_PRINTLN("ESP32BLE: Device info read (%d bytes)", deviceInfoValue.length());

        // Read signature (64 bytes)
        std::string signatureValue = pSignatureChar->readValue();

        if (signatureValue.length() != 64) {
            BLE_DEBUG_PRINTLN("ESP32BLE: Invalid signature length: %d", signatureValue.length());
            pBLEClient->disconnect();
            return false;
        }

        BLE_DEBUG_PRINTLN("ESP32BLE: Signature read (64 bytes)");

        // Disconnect from device (we have all the data we need)
        pBLEClient->disconnect();
        BLE_DEBUG_PRINTLN("ESP32BLE: Disconnected from device");

        // Create contact using base class method
        const uint8_t* pubkey = (const uint8_t*)pubKeyValue.data();
        const BLEDeviceInfo* device_info = (const BLEDeviceInfo*)deviceInfoValue.data();
        const uint8_t* signature = (const uint8_t*)signatureValue.data();

        bool success = createContactFromBLE(pubkey, device_info, signature);

        if (success) {
            BLE_DEBUG_PRINTLN("ESP32BLE: Contact created successfully: %s", device_info->name);
        } else {
            BLE_DEBUG_PRINTLN("ESP32BLE: Failed to create contact (signature invalid or database full)");
        }

        return success;

    } catch (const std::exception& e) {
        BLE_DEBUG_PRINTLN("ESP32BLE: Exception during connection: %s", e.what());
        if (pBLEClient && pBLEClient->isConnected()) {
            pBLEClient->disconnect();
        }
        return false;
    } catch (...) {
        BLE_DEBUG_PRINTLN("ESP32BLE: Unknown exception during connection");
        if (pBLEClient && pBLEClient->isConnected()) {
            pBLEClient->disconnect();
        }
        return false;
    }
}
