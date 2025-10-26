#pragma once

#include "../BaseSerialInterface.h"
#include "../BLEServiceDefinitions.h"
#include <bluefruit.h>

#ifndef BLE_TX_POWER
#define BLE_TX_POWER 4
#endif

#ifndef BLE_ADVERT
#define BLE_ADVERT 0  // Default: BLE discovery disabled
#endif

#if BLE_ADVERT
// Forward declaration
class BLEDiscoveryManager;
#endif

class SerialBLEInterface : public BaseSerialInterface {
  BLEUart bleuart;
  bool _isEnabled;
  bool _isDeviceConnected;
  unsigned long _last_write;

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  #define FRAME_QUEUE_SIZE  4
  int send_queue_len;
  Frame send_queue[FRAME_QUEUE_SIZE];

  void clearBuffers() { send_queue_len = 0; }
  static void onConnect(uint16_t connection_handle);
  static void onDisconnect(uint16_t connection_handle, uint8_t reason);
  static void onSecured(uint16_t connection_handle);

  // MeshCore BLE Discovery (server-side members)
  BLEService meshCoreService;
  BLECharacteristic pubKeyChar;
  BLECharacteristic deviceInfoChar;
  BLECharacteristic signatureChar;

  // MeshCore BLE Client (for auto-discovery connections)
  BLEClientService meshCoreClientService;
  BLEClientCharacteristic pubKeyClientChar;
  BLEClientCharacteristic deviceInfoClientChar;
  BLEClientCharacteristic signatureClientChar;

public:
  // Connection state tracking (public so callbacks can access)
  bool _client_connected;
  uint16_t _client_conn_handle;

private:

public:
  #if BLE_ADVERT
  BLEDiscoveryManager* discovery_manager;
  #endif
  SerialBLEInterface() : meshCoreService(MESHCORE_SERVICE_UUID),
                          pubKeyChar(MESHCORE_PUBKEY_UUID),
                          deviceInfoChar(MESHCORE_DEVICE_INFO_UUID),
                          signatureChar(MESHCORE_SIGNATURE_UUID),
                          meshCoreClientService(MESHCORE_SERVICE_UUID),
                          pubKeyClientChar(MESHCORE_PUBKEY_UUID),
                          deviceInfoClientChar(MESHCORE_DEVICE_INFO_UUID),
                          signatureClientChar(MESHCORE_SIGNATURE_UUID) {
    _isEnabled = false;
    _isDeviceConnected = false;
    _last_write = 0;
    send_queue_len = 0;
    #if BLE_ADVERT
    discovery_manager = nullptr;
    #endif
    _client_connected = false;
    _client_conn_handle = BLE_CONN_HANDLE_INVALID;
  }

  void startAdv();
  void stopAdv();
  void begin(const char* device_name, uint32_t pin_code);

  // BaseSerialInterface methods
  void enable() override;
  void disable() override;
  bool isEnabled() const override { return _isEnabled; }

  bool isConnected() const override;

  bool isWriteBusy() const override;
  size_t writeFrame(const uint8_t src[], size_t len) override;
  size_t checkRecvFrame(uint8_t dest[]) override;

  // MeshCore BLE Discovery methods
  void createMeshCoreService();
  void updateManufacturerData(const BLEManufacturerData& data);
  void setMeshCoreCharacteristics(const uint8_t* pubkey, const BLEDeviceInfo* device_info, const uint8_t* signature);
  #if BLE_ADVERT
  void startScanning(BLEDiscoveryManager* discovery_mgr);
  bool connectAndReadDevice(const uint8_t* mac_addr, uint8_t device_hash);
  #endif
};

#if BLE_DEBUG_LOGGING && ARDUINO
  #include <Arduino.h>
  #define BLE_DEBUG_PRINT(F, ...) Serial.printf("BLE: " F, ##__VA_ARGS__)
  #define BLE_DEBUG_PRINTLN(F, ...) Serial.printf("BLE: " F "\n", ##__VA_ARGS__)
#else
  #define BLE_DEBUG_PRINT(...) {}
  #define BLE_DEBUG_PRINTLN(...) {}
#endif
