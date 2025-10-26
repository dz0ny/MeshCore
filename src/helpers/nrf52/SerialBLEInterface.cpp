#include "SerialBLEInterface.h"
#include "../BLEDiscoveryManager.h"
#include "nrf52_watchdog.h"

// BLE Scanning control - set to 0 to disable scanning (advertising only mode)
#ifndef BLE_SCANNING
#define BLE_SCANNING 0  // Default: scanning DISABLED (advertising only)
#endif

static SerialBLEInterface* instance;

// Peripheral callbacks (for incoming connections to our UART service)
void SerialBLEInterface::onConnect(uint16_t connection_handle) {
  BLE_DEBUG_PRINTLN("SerialBLEInterface: connected (peripheral)");
  // we now set _isDeviceConnected=true in onSecured callback instead
}

void SerialBLEInterface::onDisconnect(uint16_t connection_handle, uint8_t reason) {
  BLE_DEBUG_PRINTLN("SerialBLEInterface: disconnected (peripheral) reason=%d", reason);
  if(instance){
    instance->_isDeviceConnected = false;
    instance->startAdv();
  }
}

void SerialBLEInterface::onSecured(uint16_t connection_handle) {
  BLE_DEBUG_PRINTLN("SerialBLEInterface: onSecured (peripheral)");
  if(instance){
    instance->_isDeviceConnected = true;
    // no need to stop advertising on connect, as the ble stack does this automatically
  }
}

// Central callbacks (for outgoing connections to read MeshCore devices)
static void central_connect_callback(uint16_t conn_handle) {
  BLE_DEBUG_PRINTLN("SerialBLEInterface: connected as central, conn_handle=%d", conn_handle);
  if (instance) {
    instance->_client_connected = true;
    instance->_client_conn_handle = conn_handle;
  }
}

static void central_disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  BLE_DEBUG_PRINTLN("SerialBLEInterface: disconnected as central, reason=%d", reason);
  if (instance) {
    instance->_client_connected = false;
    instance->_client_conn_handle = BLE_CONN_HANDLE_INVALID;
  }
}

void SerialBLEInterface::begin(const char* device_name, uint32_t pin_code) {

  instance = this;

  char charpin[20];
  sprintf(charpin, "%d", pin_code);

  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.configPrphConn(250, BLE_GAP_EVENT_LENGTH_MIN, 16, 16);  // increase MTU
  Bluefruit.setTxPower(BLE_TX_POWER);
  Bluefruit.begin();

  // Safety check: ensure device name is not empty or null
  if (device_name == nullptr || device_name[0] == '\0') {
    BLE_DEBUG_PRINTLN("WARNING: Empty device name provided, using default 'MeshCore'");
    Bluefruit.setName("MeshCore");
  } else {
    Bluefruit.setName(device_name);
    BLE_DEBUG_PRINTLN("SerialBLEInterface::begin - BLE name set to: %s", device_name);
  }

  Bluefruit.Security.setMITM(true);
  Bluefruit.Security.setPIN(charpin);

  // Set peripheral callbacks (for incoming UART connections)
  Bluefruit.Periph.setConnectCallback(onConnect);
  Bluefruit.Periph.setDisconnectCallback(onDisconnect);
  Bluefruit.Security.setSecuredCallback(onSecured);

  // Set central callbacks (for outgoing discovery connections)
  Bluefruit.Central.setConnectCallback(central_connect_callback);
  Bluefruit.Central.setDisconnectCallback(central_disconnect_callback);

  // To be consistent OTA DFU should be added first if it exists
  //bledfu.begin();

  // Configure and start the BLE Uart service
  bleuart.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  bleuart.begin();

  // Initialize BLE client service for auto-discovery
  meshCoreClientService.begin();
  pubKeyClientChar.begin();
  deviceInfoClientChar.begin();
  signatureClientChar.begin();

}

void SerialBLEInterface::startAdv() {
  BLE_DEBUG_PRINTLN("SerialBLEInterface: startAdv() - using updateManufacturerData() instead");

  // Create a default/empty manufacturer data packet
  // The actual data will be updated by periodic calls to updateManufacturerData()
  BLEManufacturerData data = {0};
  data.manufacturer_id = MESHCORE_MANUFACTURER_ID;
  data.magic_byte = MESHCORE_MAGIC_BYTE;
  data.protocol_version = MESHCORE_PROTOCOL_VERSION;
  // Other fields will be zero until first update

  // Use the unified advertising function
  updateManufacturerData(data);
}

void SerialBLEInterface::stopAdv() {

  BLE_DEBUG_PRINTLN("SerialBLEInterface: stopping advertising");
  
  // we only want to stop advertising if it's running, otherwise an invalid state error is logged by ble stack
  if(!Bluefruit.Advertising.isRunning()){
    return;
  }

  // stop advertising
  Bluefruit.Advertising.stop();

}

// ---------- public methods

void SerialBLEInterface::enable() { 
  if (_isEnabled) return;

  _isEnabled = true;
  clearBuffers();

  // Start advertising
  startAdv();
}

void SerialBLEInterface::disable() {
  _isEnabled = false;
  BLE_DEBUG_PRINTLN("SerialBLEInterface::disable");

#ifdef RAK_BOARD
  Bluefruit.disconnect(Bluefruit.connHandle());
#else
  uint16_t conn_id;
  if (Bluefruit.getConnectedHandles(&conn_id, 1) > 0) {
    Bluefruit.disconnect(conn_id);
  }
#endif

  Bluefruit.Advertising.restartOnDisconnect(false);
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();

  stopAdv();
}

size_t SerialBLEInterface::writeFrame(const uint8_t src[], size_t len) {
  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("writeFrame(), frame too big, len=%d", len);
    return 0;
  }

  if (_isDeviceConnected && len > 0) {
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
    bleuart.write(send_queue[0].buf, send_queue[0].len);
    BLE_DEBUG_PRINTLN("writeBytes: sz=%d, hdr=%d", (uint32_t)send_queue[0].len, (uint32_t) send_queue[0].buf[0]);

    send_queue_len--;
    for (int i = 0; i < send_queue_len; i++) {   // delete top item from queue
      send_queue[i] = send_queue[i + 1];
    }
  } else {
    int len = bleuart.available();
    if (len > 0) {
      bleuart.readBytes(dest, len);
      BLE_DEBUG_PRINTLN("readBytes: sz=%d, hdr=%d", len, (uint32_t) dest[0]);
      return len;
    }
  }
  return 0;
}

bool SerialBLEInterface::isConnected() const {
  return _isDeviceConnected;
}

// ---------- MeshCore BLE Discovery methods

void SerialBLEInterface::createMeshCoreService() {
  BLE_DEBUG_PRINTLN("SerialBLEInterface::createMeshCoreService");

  // Create the MeshCore service with UUID
  meshCoreService = BLEService(MESHCORE_SERVICE_UUID);

  // Create public key characteristic (32 bytes, read-only)
  pubKeyChar = BLECharacteristic(MESHCORE_PUBKEY_UUID);
  pubKeyChar.setProperties(CHR_PROPS_READ);
  pubKeyChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  pubKeyChar.setFixedLen(32);

  // Create device info characteristic (variable length, read-only)
  deviceInfoChar = BLECharacteristic(MESHCORE_DEVICE_INFO_UUID);
  deviceInfoChar.setProperties(CHR_PROPS_READ);
  deviceInfoChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  deviceInfoChar.setFixedLen(sizeof(BLEDeviceInfo));

  // Create signature characteristic (64 bytes, read-only)
  signatureChar = BLECharacteristic(MESHCORE_SIGNATURE_UUID);
  signatureChar.setProperties(CHR_PROPS_READ);
  signatureChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  signatureChar.setFixedLen(64);

  // Add characteristics to service
  meshCoreService.begin();
  pubKeyChar.begin();
  deviceInfoChar.begin();
  signatureChar.begin();

  BLE_DEBUG_PRINTLN("MeshCore service created");
}

void SerialBLEInterface::updateManufacturerData(const BLEManufacturerData& data) {
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

  // Check advertising status
  bool was_advertising = Bluefruit.Advertising.isRunning();
  BLE_DEBUG_PRINTLN("Advertising was running: %d", was_advertising);

  if (!was_advertising) {
    BLE_DEBUG_PRINTLN("WARNING: Advertising stopped unexpectedly! (should always be running)");
  }

  // Stop advertising if running to reconfigure
  if (was_advertising) {
    Bluefruit.Advertising.stop();
  }

  // Clear previous advertising and scan response data
  Bluefruit.Advertising.clearData();
  Bluefruit.ScanResponse.clearData();

  // Set advertising packet - IMPORTANT: BLE advertisements limited to 31 bytes
  // Priority: Manufacturer data (26 bytes) for discovery - must be in main advertisement for hardware filtering
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE); // 3 bytes

  // Add manufacturer data with MeshCore discovery info - CRITICAL for discovery
  Bluefruit.Advertising.addManufacturerData(&data, sizeof(data)); // 26 bytes

  // Total so far: ~29 bytes (3 flags + 26 mfg data)
  // We have ~2 bytes left in the 31 byte limit - not enough for full device name
  // Device name will be in scan response instead (see below)

  // Debug: log manufacturer data hex
  #if BLE_DEBUG_LOGGING
  Serial.print("BLE: Advertising with manufacturer data (");
  Serial.print(sizeof(data));
  Serial.print(" bytes): ");
  const uint8_t* bytes = (const uint8_t*)&data;
  for (size_t i = 0; i < sizeof(data); i++) {
    Serial.printf("%02X ", bytes[i]);
  }
  Serial.println();
  #endif

  // Scan response contains services and device name
  Bluefruit.ScanResponse.addService(bleuart);       // BLE UART service for existing functionality
  Bluefruit.ScanResponse.addService(meshCoreService); // MeshCore service
  Bluefruit.ScanResponse.addTxPower();              // TX power
  Bluefruit.ScanResponse.addName();                 // Device name

  #ifdef BLE_DEBUG
  char current_name[32];
  Bluefruit.getName(current_name, sizeof(current_name));
  BLE_DEBUG_PRINTLN("updateManufacturerData: Advertising with name: %s", current_name);
  #endif

  // Configure advertising intervals and behavior
  Bluefruit.Advertising.restartOnDisconnect(true);  // Auto-restart advertising after disconnect
  Bluefruit.Advertising.setInterval(32, 244);       // Fast mode = 20ms, slow mode = 152.5ms
  Bluefruit.Advertising.setFastTimeout(30);         // 30 seconds in fast mode, then switch to slow mode

  // Always start advertising (not just if it was running)
  Bluefruit.Advertising.start(0);  // 0 = advertise forever (don't stop after n seconds)
  BLE_DEBUG_PRINTLN("Advertising started/restarted with manufacturer data");
}

void SerialBLEInterface::setMeshCoreCharacteristics(const uint8_t* pubkey, const BLEDeviceInfo* device_info, const uint8_t* signature) {
  BLE_DEBUG_PRINTLN("SerialBLEInterface::setMeshCoreCharacteristics");

  if (pubkey) {
    pubKeyChar.write(pubkey, 32);
    BLE_DEBUG_PRINTLN("Public key characteristic set");
  }

  if (device_info) {
    deviceInfoChar.write(device_info, sizeof(BLEDeviceInfo));
    BLE_DEBUG_PRINTLN("Device info characteristic set");
  }

  if (signature) {
    signatureChar.write(signature, 64);
    BLE_DEBUG_PRINTLN("Signature characteristic set");
  }
}

// Static callback for BLE scanning
static void scan_callback(ble_gap_evt_adv_report_t* report) {
  static unsigned long last_callback_log = 0;
  static int callback_count = 0;

  callback_count++;

  // Parse manufacturer data from advertisement
  uint8_t buffer[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
  memset(buffer, 0, sizeof(buffer));

  // parseReportByType returns the length of data copied to buffer
  uint8_t len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buffer, sizeof(buffer));

  // Filter out Apple devices (0x4C00) in software - they spam the logs
  // CRITICAL: Must call resume() before ANY early return!
  if (len >= 2 && buffer[0] == 0x4C && buffer[1] == 0x00) {
    Bluefruit.Scanner.resume();
    return;
  }

#if BLE_DEBUG_LOGGING
  // Detailed packet logging
  if (report->type.scan_response) {
    Serial.printf("[SR%10d] Packet received from ", millis());
  } else {
    Serial.printf("[ADV%9d] Packet received from ", millis());
  }
  Serial.printBuffer(report->peer_addr.addr, 6, ':');
  Serial.print("\n");

  // Raw buffer contents
  Serial.printf("%14s %d bytes\n", "PAYLOAD", report->data.len);
  if (report->data.len) {
    Serial.printf("%15s", " ");
    Serial.printBuffer(report->data.p_data, report->data.len, '-');
    Serial.println();
  }

  // RSSI value
  Serial.printf("%14s %d dBm\n", "RSSI", report->rssi);

  // Adv Type (using bitfields from ble_gap_adv_report_type_t)
  Serial.printf("%14s ", "ADV TYPE");
  if (report->type.connectable) Serial.print("Connectable ");
  if (report->type.scannable) Serial.print("Scannable ");
  if (report->type.directed) Serial.print("Directed ");
  if (report->type.scan_response) Serial.print("ScanResponse ");
  if (report->type.extended_pdu) Serial.print("Extended ");
  Serial.println();

  // Check for MeshCore service UUID
  if (Bluefruit.Scanner.checkReportForUuid(report, MESHCORE_SERVICE_UUID)) {
    Serial.printf("%14s %s\n", "MESHCORE", "UUID Found!");
  }

  // Show manufacturer data if present
  if (len >= 2) {
    uint16_t mfg_id = (buffer[1] << 8) | buffer[0];
    Serial.printf("%14s 0x%04X (%d bytes): ", "MFG DATA", mfg_id, len);
    for (int i = 0; i < len; i++) {
      Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();
  }

  Serial.println();
#endif

  #if BLE_ADVERT
  if (len > 0 && instance && instance->discovery_manager) {
    // Extract MAC address from peer_addr
    const uint8_t* mac = report->peer_addr.addr;
    int8_t rssi = report->rssi;

    // Try to get device name from advertisement or scan response
    char name_buffer[32] = {0};
    uint8_t name_len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, (uint8_t*)name_buffer, sizeof(name_buffer) - 1);
    if (name_len == 0) {
      // Try short name if complete name not found
      name_len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, (uint8_t*)name_buffer, sizeof(name_buffer) - 1);
    }

    // Forward to discovery manager (name will be passed separately)
    instance->discovery_manager->onAdvertisementReceived(mac, buffer, len, rssi, name_len > 0 ? name_buffer : nullptr);
  }
  #endif

  // Continue scanning (resume on the same sequence number)
  Bluefruit.Scanner.resume();
}

#if BLE_ADVERT
void SerialBLEInterface::startScanning(BLEDiscoveryManager* discovery_mgr) {
  #if BLE_SCANNING
  BLE_DEBUG_PRINTLN("SerialBLEInterface::startScanning");

  if (!discovery_mgr) {
    BLE_DEBUG_PRINTLN("Error: discovery_mgr is NULL");
    return;
  }

  // Store discovery manager pointer
  discovery_manager = discovery_mgr;

  // Set scan callback
  Bluefruit.Scanner.setRxCallback(scan_callback);
  BLE_DEBUG_PRINTLN("Scan callback registered");

  // Auto-restart scanning after disconnect
  Bluefruit.Scanner.restartOnDisconnect(true);

  // Set scan interval and window (in units of 0.625 ms)
  // interval=160 (100ms), window=80 (50ms) = 50% duty cycle
  Bluefruit.Scanner.setInterval(160, 80);

  // Use ACTIVE scanning to capture scan response data (manufacturer data might be in scan response)
  Bluefruit.Scanner.useActiveScan(true);

  // Try hardware MSD filter - may not work on all nRF52 platforms
  uint16_t meshcore_mfg_id = MESHCORE_MANUFACTURER_ID; // 0x434D
  Bluefruit.Scanner.filterMSD(meshcore_mfg_id);
  BLE_DEBUG_PRINTLN("Hardware MSD filter set for manufacturer ID: 0x%04X", meshcore_mfg_id);

  // Start scanning (0 = scan forever)
  bool started = Bluefruit.Scanner.start(0);

  BLE_DEBUG_PRINTLN("BLE scanning started: %d (interval=100ms, window=50ms, ACTIVE scan, HW MSD filter=0x%04X)", started, meshcore_mfg_id);

  // Verify scanner is running
  if (Bluefruit.Scanner.isRunning()) {
    BLE_DEBUG_PRINTLN("Scanner confirmed running ✓");
  } else {
    BLE_DEBUG_PRINTLN("WARNING: Scanner failed to start!");
  }
  #else
  BLE_DEBUG_PRINTLN("SerialBLEInterface::startScanning - DISABLED (BLE_SCANNING=0)");
  // Store discovery manager pointer even when scanning disabled
  discovery_manager = discovery_mgr;
  #endif
}

bool SerialBLEInterface::connectAndReadDevice(const uint8_t* mac_addr, uint8_t device_hash) {
  BLE_DEBUG_PRINTLN("SerialBLEInterface::connectAndReadDevice - Connecting to device hash=%02X", device_hash);

  // Prepare BLE address structure
  ble_gap_addr_t peer_addr;
  peer_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC; // Most common for advertising devices
  memcpy(peer_addr.addr, mac_addr, 6);

  // Stop scanning AND advertising before connecting
  // This prevents BLE stack conflicts when acting as central
  #if BLE_SCANNING
  if (Bluefruit.Scanner.isRunning()) {
    Bluefruit.Scanner.stop();
  }
  #endif

  if (Bluefruit.Advertising.isRunning()) {
    Bluefruit.Advertising.stop();
  }

  // Attempt connection with timeout
  BLE_DEBUG_PRINTLN("Attempting BLE connection...");
  if (!Bluefruit.Central.connect(&peer_addr)) {
    BLE_DEBUG_PRINTLN("Connection failed - could not initiate");
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  // Wait for connection to establish (with timeout + watchdog)
  unsigned long connect_start = millis();
  const unsigned long CONNECT_TIMEOUT = 10000; // 10 seconds
  unsigned long last_feed = millis();
  unsigned long last_log = millis();

  while (!_client_connected && (millis() - connect_start < CONNECT_TIMEOUT)) {
    // Feed watchdog every 500ms to prevent reset
    if (millis() - last_feed > 500) {
      nrf52_wdt_feed();
      last_feed = millis();
    }

    // Log progress every 2 seconds
    if (millis() - last_log > 2000) {
      BLE_DEBUG_PRINTLN("Waiting for connection... %d ms elapsed", millis() - connect_start);
      last_log = millis();
    }

    yield(); // CRITICAL: Allow BLE stack to process callbacks
    delay(10);
  }

  if (!_client_connected) {
    BLE_DEBUG_PRINTLN("Connection timeout");
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  BLE_DEBUG_PRINTLN("Connected! Discovering MeshCore service...");

  // Feed watchdog before service discovery
  nrf52_wdt_feed();

  // Discover MeshCore service
  if (!meshCoreClientService.discover(_client_conn_handle)) {
    BLE_DEBUG_PRINTLN("MeshCore service not found");
    Bluefruit.disconnect(_client_conn_handle);
    _client_connected = false;
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  BLE_DEBUG_PRINTLN("MeshCore service found! Discovering characteristics...");

  // Feed watchdog before characteristic discovery
  nrf52_wdt_feed();

  // Discover characteristics
  if (!pubKeyClientChar.discover()) {
    BLE_DEBUG_PRINTLN("Public key characteristic not found");
    Bluefruit.disconnect(_client_conn_handle);
    _client_connected = false;
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  nrf52_wdt_feed(); // Feed between each characteristic discovery

  if (!deviceInfoClientChar.discover()) {
    BLE_DEBUG_PRINTLN("Device info characteristic not found");
    Bluefruit.disconnect(_client_conn_handle);
    _client_connected = false;
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  nrf52_wdt_feed(); // Feed between each characteristic discovery

  if (!signatureClientChar.discover()) {
    BLE_DEBUG_PRINTLN("Signature characteristic not found");
    Bluefruit.disconnect(_client_conn_handle);
    _client_connected = false;
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  BLE_DEBUG_PRINTLN("All characteristics discovered! Reading data...");
  nrf52_wdt_feed(); // Feed before reading data

  // Read public key (32 bytes)
  uint8_t pubkey[32];
  uint16_t read_len = pubKeyClientChar.read(pubkey, 32);
  if (read_len != 32) {
    BLE_DEBUG_PRINTLN("Failed to read public key (got %d bytes)", read_len);
    Bluefruit.disconnect(_client_conn_handle);
    _client_connected = false;
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  // Verify hash matches
  if (pubkey[0] != device_hash) {
    BLE_DEBUG_PRINTLN("Hash mismatch! Expected %02X, got %02X", device_hash, pubkey[0]);
    Bluefruit.disconnect(_client_conn_handle);
    _client_connected = false;
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  // Read device info
  BLEDeviceInfo device_info;
  read_len = deviceInfoClientChar.read(&device_info, sizeof(BLEDeviceInfo));
  if (read_len != sizeof(BLEDeviceInfo)) {
    BLE_DEBUG_PRINTLN("Failed to read device info (got %d bytes)", read_len);
    Bluefruit.disconnect(_client_conn_handle);
    _client_connected = false;
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  // Read signature (64 bytes)
  uint8_t signature[64];
  read_len = signatureClientChar.read(signature, 64);
  if (read_len != 64) {
    BLE_DEBUG_PRINTLN("Failed to read signature (got %d bytes)", read_len);
    Bluefruit.disconnect(_client_conn_handle);
    _client_connected = false;
    #if BLE_SCANNING
    Bluefruit.Scanner.start(0); // Restart scanning
    #endif
    return false;
  }

  BLE_DEBUG_PRINTLN("All data read successfully! Verifying signature...");

  // Disconnect from device
  Bluefruit.disconnect(_client_conn_handle);
  _client_connected = false;

  #if BLE_SCANNING
  // Restart scanning after successful read
  Bluefruit.Scanner.start(0);
  #endif

  // Create contact using discovery manager (includes signature verification)
  // Note: discovery_manager is set by the nRF52BLEDiscoveryManager
  if (!discovery_manager) {
    BLE_DEBUG_PRINTLN("Error: discovery_manager is NULL");
    return false;
  }

  bool success = discovery_manager->createContactFromBLE(pubkey, &device_info, signature);

  if (success) {
    BLE_DEBUG_PRINTLN("Contact created successfully!");
  } else {
    BLE_DEBUG_PRINTLN("Failed to create contact (signature verification likely failed)");
  }

  return success;
}
#endif  // BLE_ADVERT
