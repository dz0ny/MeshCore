#include "SerialBLEInterface.h"
#include "../BLEDiscoveryManager.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define ADVERT_RESTART_DELAY  1000   // millis

// BLE Scan callback for MeshCore discovery
class MeshCoreScanCallbacks : public BLEAdvertisedDeviceCallbacks {
private:
  BLEDiscoveryManager* discovery_mgr;

public:
  MeshCoreScanCallbacks(BLEDiscoveryManager* mgr) : discovery_mgr(mgr) {}

  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (!discovery_mgr) return;

    // Get manufacturer data
    std::string mfgData = advertisedDevice.getManufacturerData();
    if (mfgData.length() > 0) {
      // Extract MAC address
      uint8_t mac[6];
      memcpy(mac, advertisedDevice.getAddress().getNative(), 6);

      // Get RSSI
      int rssi = advertisedDevice.getRSSI();

      // Call discovery manager callback
      discovery_mgr->onAdvertisementReceived(
        mac,
        (const uint8_t*)mfgData.data(),
        mfgData.length(),
        rssi
      );
    }
  }
};

void SerialBLEInterface::begin(const char* device_name, uint32_t pin_code) {
  _pin_code = pin_code;

  // Safety check: ensure device name is not empty or null
  const char* ble_name = device_name;
  if (device_name == nullptr || device_name[0] == '\0') {
    BLE_DEBUG_PRINTLN("WARNING: Empty device name provided, using default 'MeshCore'");
    ble_name = "MeshCore";
  } else {
    BLE_DEBUG_PRINTLN("SerialBLEInterface::begin - BLE name set to: %s", device_name);
  }

  // Create the BLE Device
  BLEDevice::init(ble_name);
  BLEDevice::setSecurityCallbacks(this);
  BLEDevice::setMTU(MAX_FRAME_SIZE);

  BLESecurity  sec;
  sec.setStaticPIN(pin_code);
  sec.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

  //BLEDevice::setPower(ESP_PWR_LVL_N8);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  // Create the BLE Service
  pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
  pRxCharacteristic->setCallbacks(this);

  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
}

// -------- BLESecurityCallbacks methods

uint32_t SerialBLEInterface::onPassKeyRequest() {
  BLE_DEBUG_PRINTLN("onPassKeyRequest()");
  return _pin_code;
}

void SerialBLEInterface::onPassKeyNotify(uint32_t pass_key) {
  BLE_DEBUG_PRINTLN("onPassKeyNotify(%u)", pass_key);
}

bool SerialBLEInterface::onConfirmPIN(uint32_t pass_key) {
  BLE_DEBUG_PRINTLN("onConfirmPIN(%u)", pass_key);
  return true;
}

bool SerialBLEInterface::onSecurityRequest() {
  BLE_DEBUG_PRINTLN("onSecurityRequest()");
  return true;  // allow
}

void SerialBLEInterface::onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
  if (cmpl.success) {
    BLE_DEBUG_PRINTLN(" - SecurityCallback - Authentication Success");
    deviceConnected = true;
  } else {
    BLE_DEBUG_PRINTLN(" - SecurityCallback - Authentication Failure*");

    //pServer->removePeerDevice(pServer->getConnId(), true);
    pServer->disconnect(pServer->getConnId());
    adv_restart_time = millis() + ADVERT_RESTART_DELAY;
  }
}

// -------- BLEServerCallbacks methods

void SerialBLEInterface::onConnect(BLEServer* pServer) {
}

void SerialBLEInterface::onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
  BLE_DEBUG_PRINTLN("onConnect(), conn_id=%d, mtu=%d", param->connect.conn_id, pServer->getPeerMTU(param->connect.conn_id));
  last_conn_id = param->connect.conn_id;
}

void SerialBLEInterface::onMtuChanged(BLEServer* pServer, esp_ble_gatts_cb_param_t* param) {
  BLE_DEBUG_PRINTLN("onMtuChanged(), mtu=%d", pServer->getPeerMTU(param->mtu.conn_id));
}

void SerialBLEInterface::onDisconnect(BLEServer* pServer) {
  BLE_DEBUG_PRINTLN("onDisconnect()");
  if (_isEnabled) {
    adv_restart_time = millis() + ADVERT_RESTART_DELAY;

    // loop() will detect this on next loop, and set deviceConnected to false
  }
}

// -------- BLECharacteristicCallbacks methods

void SerialBLEInterface::onWrite(BLECharacteristic* pCharacteristic, esp_ble_gatts_cb_param_t* param) {
  uint8_t* rxValue = pCharacteristic->getData();
  int len = pCharacteristic->getLength();

  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("ERROR: onWrite(), frame too big, len=%d", len);
  } else if (recv_queue_len >= FRAME_QUEUE_SIZE) {
    BLE_DEBUG_PRINTLN("ERROR: onWrite(), recv_queue is full!");
  } else {
    recv_queue[recv_queue_len].len = len;
    memcpy(recv_queue[recv_queue_len].buf, rxValue, len);
    recv_queue_len++;
  }
}

// ---------- public methods

void SerialBLEInterface::enable() { 
  if (_isEnabled) return;

  _isEnabled = true;
  clearBuffers();

  // Start the service
  pService->start();

  // Start advertising

  //pServer->getAdvertising()->setMinInterval(500);
  //pServer->getAdvertising()->setMaxInterval(1000);

  pServer->getAdvertising()->start();
  adv_restart_time = 0;
}

void SerialBLEInterface::disable() {
  _isEnabled = false;

  BLE_DEBUG_PRINTLN("SerialBLEInterface::disable");

  pServer->getAdvertising()->stop();
  pServer->disconnect(last_conn_id);
  pService->stop();
  oldDeviceConnected = deviceConnected = false;
  adv_restart_time = 0;
}

size_t SerialBLEInterface::writeFrame(const uint8_t src[], size_t len) {
  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("writeFrame(), frame too big, len=%d", len);
    return 0;
  }

  if (deviceConnected && len > 0) {
    if (send_queue_len >= FRAME_QUEUE_SIZE) {
      BLE_DEBUG_PRINTLN("writeFrame(), send_queue is full!");
      return 0;
    }

    send_queue[send_queue_len].len = len;  // add to send queue
    memcpy(send_queue[send_queue_len].buf, src, len);
    send_queue_len++;

    return len;
  }
  return 0;
}

#define  BLE_WRITE_MIN_INTERVAL   60

bool SerialBLEInterface::isWriteBusy() const {
  return millis() < _last_write + BLE_WRITE_MIN_INTERVAL;   // still too soon to start another write?
}

size_t SerialBLEInterface::checkRecvFrame(uint8_t dest[]) {
  if (send_queue_len > 0   // first, check send queue
    && millis() >= _last_write + BLE_WRITE_MIN_INTERVAL    // space the writes apart
  ) {
    _last_write = millis();
    pTxCharacteristic->setValue(send_queue[0].buf, send_queue[0].len);
    pTxCharacteristic->notify();

    BLE_DEBUG_PRINTLN("writeBytes: sz=%d, hdr=%d", (uint32_t)send_queue[0].len, (uint32_t) send_queue[0].buf[0]);

    send_queue_len--;
    for (int i = 0; i < send_queue_len; i++) {   // delete top item from queue
      send_queue[i] = send_queue[i + 1];
    }
  }

  if (recv_queue_len > 0) {   // check recv queue
    size_t len = recv_queue[0].len;   // take from top of queue
    memcpy(dest, recv_queue[0].buf, len);

    BLE_DEBUG_PRINTLN("readBytes: sz=%d, hdr=%d", len, (uint32_t) dest[0]);

    recv_queue_len--;
    for (int i = 0; i < recv_queue_len; i++) {   // delete top item from queue
      recv_queue[i] = recv_queue[i + 1];
    }
    return len;
  }

  if (pServer->getConnectedCount() == 0)  deviceConnected = false;

  if (deviceConnected != oldDeviceConnected) {
    if (!deviceConnected) {    // disconnecting
      clearBuffers();

      BLE_DEBUG_PRINTLN("SerialBLEInterface -> disconnecting...");

      //pServer->getAdvertising()->setMinInterval(500);
      //pServer->getAdvertising()->setMaxInterval(1000);

      adv_restart_time = millis() + ADVERT_RESTART_DELAY;
    } else {
      BLE_DEBUG_PRINTLN("SerialBLEInterface -> stopping advertising");
      BLE_DEBUG_PRINTLN("SerialBLEInterface -> connecting...");
      // connecting
      // do stuff here on connecting
      pServer->getAdvertising()->stop();
      adv_restart_time = 0;
    }
    oldDeviceConnected = deviceConnected;
  }

  if (adv_restart_time && millis() >= adv_restart_time) {
    if (pServer->getConnectedCount() == 0) {
      BLE_DEBUG_PRINTLN("SerialBLEInterface -> re-starting advertising");
      pServer->getAdvertising()->start();  // re-Start advertising
    }
    adv_restart_time = 0;
  }
  return 0;
}

bool SerialBLEInterface::isConnected() const {
  return deviceConnected;  //pServer != NULL && pServer->getConnectedCount() > 0;
}

// ---------- MeshCore BLE discovery methods

void SerialBLEInterface::createMeshCoreService() {
  if (!pServer) {
    BLE_DEBUG_PRINTLN("ERROR: createMeshCoreService() called before begin()");
    return;
  }

  // Create MeshCore service
  pMeshCoreService = pServer->createService(MESHCORE_SERVICE_UUID);

  // Create public key characteristic (32 bytes, READ only)
  pPubKeyChar = pMeshCoreService->createCharacteristic(
    MESHCORE_PUBKEY_UUID,
    BLECharacteristic::PROPERTY_READ
  );

  // Create device info characteristic (variable length, READ only)
  pDeviceInfoChar = pMeshCoreService->createCharacteristic(
    MESHCORE_DEVICE_INFO_UUID,
    BLECharacteristic::PROPERTY_READ
  );

  // Create signature characteristic (64 bytes, READ only)
  pSignatureChar = pMeshCoreService->createCharacteristic(
    MESHCORE_SIGNATURE_UUID,
    BLECharacteristic::PROPERTY_READ
  );

  // Start the MeshCore service
  pMeshCoreService->start();

  // Add MeshCore service UUID to advertising
  pServer->getAdvertising()->addServiceUUID(MESHCORE_SERVICE_UUID);

  BLE_DEBUG_PRINTLN("MeshCore service created");
}

void SerialBLEInterface::updateManufacturerData(const BLEManufacturerData& data) {
  if (!pServer) {
    BLE_DEBUG_PRINTLN("ERROR: updateManufacturerData() called before begin()");
    return;
  }

  // OPTIMIZATION: Compare with last advertised data to avoid unnecessary BLE stack updates
  // This reduces radio activity, power consumption, and BLE stack churn

  static BLEManufacturerData last_advertised_data = {0};
  static bool first_update = true;

  // Compare with last advertised data (skip timestamp field which changes frequently)
  // We only care about meaningful changes: location, flags, device type
  bool data_changed = first_update ||
    last_advertised_data.manufacturer_id != data.manufacturer_id ||
    last_advertised_data.magic_byte != data.magic_byte ||
    last_advertised_data.protocol_version != data.protocol_version ||
    last_advertised_data.device_hash != data.device_hash ||
    last_advertised_data.flags != data.flags ||
    last_advertised_data.latitude != data.latitude ||
    last_advertised_data.longitude != data.longitude;
    // NOTE: timestamp and CRC intentionally excluded from comparison

  if (!data_changed) {
    BLE_DEBUG_PRINTLN("SerialBLEInterface: Manufacturer data unchanged, skipping BLE update");
    return; // Skip unnecessary BLE stack update
  }

  BLE_DEBUG_PRINTLN("SerialBLEInterface::updateManufacturerData - data changed, updating");

  // Save current data for next comparison
  memcpy(&last_advertised_data, &data, sizeof(BLEManufacturerData));
  first_update = false;

  // Create advertisement data
  BLEAdvertisementData advData;

  // Set manufacturer data (27 bytes)
  std::string mfgData((char*)&data, sizeof(BLEManufacturerData));
  advData.setManufacturerData(mfgData);

  // Keep existing service UUIDs (both UART and MeshCore services)
  advData.setCompleteServices(BLEUUID(SERVICE_UUID));

  // Update advertising data
  pServer->getAdvertising()->setAdvertisementData(advData);

  // Restart advertising if currently enabled
  if (_isEnabled && !deviceConnected) {
    pServer->getAdvertising()->stop();
    pServer->getAdvertising()->start();
    BLE_DEBUG_PRINTLN("Manufacturer data updated, advertising restarted");
  } else {
    BLE_DEBUG_PRINTLN("Manufacturer data updated (will apply on next advertising start)");
  }
}

void SerialBLEInterface::setMeshCoreCharacteristics(const uint8_t* pubkey, const BLEDeviceInfo* device_info, const uint8_t* signature) {
  if (!pMeshCoreService || !pPubKeyChar || !pDeviceInfoChar || !pSignatureChar) {
    BLE_DEBUG_PRINTLN("ERROR: setMeshCoreCharacteristics() called before createMeshCoreService()");
    return;
  }

  // Set public key (32 bytes)
  if (pubkey) {
    pPubKeyChar->setValue((uint8_t*)pubkey, 32);
    BLE_DEBUG_PRINTLN("Public key characteristic set");
  }

  // Set device info (variable length)
  if (device_info) {
    pDeviceInfoChar->setValue((uint8_t*)device_info, sizeof(BLEDeviceInfo));
    BLE_DEBUG_PRINTLN("Device info characteristic set");
  }

  // Set signature (64 bytes)
  if (signature) {
    pSignatureChar->setValue((uint8_t*)signature, 64);
    BLE_DEBUG_PRINTLN("Signature characteristic set");
  }
}

void SerialBLEInterface::startScanning(BLEDiscoveryManager* discovery_mgr) {
  if (!discovery_mgr) {
    BLE_DEBUG_PRINTLN("ERROR: startScanning() called with NULL discovery_mgr");
    return;
  }

  // Create BLE scan object if not already created
  if (!pBLEScan) {
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MeshCoreScanCallbacks(discovery_mgr));
    pBLEScan->setActiveScan(true);  // Active scan uses more power but gets scan response data
    pBLEScan->setInterval(100);     // How often to scan (ms)
    pBLEScan->setWindow(99);        // How long to scan during the interval (ms)
  }

  // Start continuous scanning (0 = scan forever, true = continue scanning)
  pBLEScan->start(0, true);  // 0 = scan forever, true = continue (don't stop)

  BLE_DEBUG_PRINTLN("BLE scanning started for MeshCore discovery");
}
