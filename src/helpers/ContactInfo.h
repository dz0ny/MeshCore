#pragma once

#include <Arduino.h>
#include <Mesh.h>

struct ContactInfo {
  mesh::Identity id;
  char name[32];
  uint8_t type;   // on of ADV_TYPE_*
  uint8_t flags;
  int8_t out_path_len;
  uint8_t out_path[MAX_PATH_SIZE];
  uint32_t last_advert_timestamp;   // by THEIR clock
  uint8_t shared_secret[PUB_KEY_SIZE];
  uint32_t lastmod;  // by OUR clock
  int32_t gps_lat, gps_lon;    // 5 dec places (meter accuracy)
  uint32_t sync_since;


  #if BLE_ADVERT
  // BLE discovery fields (added for BLE auto-discovery)
  // These fields are only used when BLE_ADVERT=1 (BLE discovery enabled)
  unsigned long last_seen_ble;      // millis() when last BLE advertisement received
  int16_t ble_rssi;                 // Last BLE RSSI value in dBm
  double last_known_lat;            // Latest location from BLE or LoRa
  double last_known_lon;            // Latest location from BLE or LoRa
  uint8_t location_source;          // 0=unknown, 1=BLE, 2=LoRa (LOCATION_SOURCE_*)
  uint32_t location_timestamp;      // When location was last updated
  #endif

  // Helper to access public key directly
  uint8_t* pub_key = id.pub_key;
  uint8_t device_type = type;
};
