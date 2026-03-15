#pragma once

#include <Arduino.h>
#include <helpers/MeshCayenneLPP.h>

class BTHomeScanner {
public:
  static const unsigned long DEFAULT_FRESHNESS_MS = 180000UL;
  static const unsigned long DEFAULT_MEASUREMENT_RETENTION_MS = 1800000UL;
  static const uint8_t MAX_DEVICES = 8;
  static const uint8_t MAX_MEASUREMENT_SLOTS = 48;
  static const uint8_t MAX_TARGETS = MAX_DEVICES;
  static const uint8_t TELEMETRY_FIELD_COUNT = MAX_MEASUREMENT_SLOTS;

  struct MeasurementSlot {
    bool used;
    uint8_t object_id;
    uint8_t occurrence;
    float value;
    unsigned long seen_at;
  };

  struct DeviceCache {
    bool used;
    uint8_t mac[6];
    int rssi;
    bool encrypted;
    unsigned long last_seen;
    char name[24];
    MeasurementSlot measurements[MAX_MEASUREMENT_SLOTS];
  };

  BTHomeScanner();

  void begin();
  void loop();
  void setEnabled(bool enabled);
  bool isEnabled() const;
  void clearCache();

  uint8_t getDeviceCount() const;
  bool getDeviceMacByIndex(uint8_t index, uint8_t mac[6]) const;

  bool addTargetMac(const uint8_t mac[6]);
  bool removeTargetMac(const uint8_t mac[6]);
  void clearTargetMacs();
  uint8_t getTargetCount() const;
  bool getTargetMac(uint8_t index, uint8_t mac[6]) const;
  bool isTargetMac(const uint8_t mac[6]) const;

  bool isKnownEncrypted(const uint8_t mac[6]) const;

  uint8_t appendTelemetry(MeshCayenneLPP& telemetry, uint8_t base_channel, unsigned long freshness_ms) const;
  size_t formatStatus(char* dest, size_t len, unsigned long freshness_ms) const;
  size_t formatDeviceList(char* dest, size_t len, unsigned long freshness_ms) const;
  size_t formatDeviceFields(char* dest, size_t len, uint8_t device_index, unsigned long freshness_ms) const;
  size_t formatDeviceFieldValue(char* dest, size_t len, uint8_t device_index, uint8_t field_index, unsigned long freshness_ms) const;
  void printDevices(Print& out, unsigned long freshness_ms) const;
  void handleScanResult(const uint8_t mac[6], const char* name, int rssi, const uint8_t* data, size_t len);

  static bool parseMac(const char* text, uint8_t mac[6]);
  static void formatMac(const uint8_t mac[6], char* dest, size_t len);

private:
  bool _enabled;
  uint8_t _target_count;
  uint8_t _target_macs[MAX_TARGETS][6];
  DeviceCache _devices[MAX_DEVICES];

  const DeviceCache* findDeviceByMac(const uint8_t mac[6]) const;
};
