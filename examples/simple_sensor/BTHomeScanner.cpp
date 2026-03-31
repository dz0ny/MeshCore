#include "BTHomeScanner.h"

#include <helpers/sensors/LPPDataHelpers.h>
#include <helpers/sensors/MeshLPPTypes.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cstring>

namespace {

static unsigned long measurementAgeMs(unsigned long seen_at) {
  return millis() - seen_at;
}

static bool isFresh(unsigned long seen_at, unsigned long freshness_ms) {
  return seen_at != 0 && measurementAgeMs(seen_at) <= freshness_ms;
}

static bool macEquals(const uint8_t lhs[6], const uint8_t rhs[6]) {
  return memcmp(lhs, rhs, 6) == 0;
}

static const BTHomeScanner::DeviceCache* findDeviceByIndex(
    const BTHomeScanner::DeviceCache (&devices)[BTHomeScanner::MAX_DEVICES],
    uint8_t index);

}  // namespace

#if defined(ESP32_PLATFORM)
#include <NimBLEDevice.h>

namespace {

static const uint16_t kBTHomeServiceUuid = 0xFCD2;
static const uint8_t kDeviceInfoUnencrypted = 0x40;
static const uint8_t kDeviceInfoEncrypted = 0x41;
static const uint8_t kDeviceInfoTriggerUnencrypted = 0x44;
static const uint8_t kDeviceInfoTriggerEncrypted = 0x45;
static const uint8_t kButtonObjectId = 0x3A;
static const uint8_t kDimmerObjectId = 0x3C;

struct BTHomeObjectDef {
  uint8_t object_id;
  uint8_t data_size;
  bool is_signed;
  float factor;
  const char* label;
};

static const BTHomeObjectDef kObjectDefs[] = {
  {0x00, 1, false, 1.0f, "pkt"},
  {0x01, 1, false, 1.0f, "batt"},
  {0x02, 2, true, 0.01f, "temp"},
  {0x03, 2, false, 0.01f, "hum"},
  {0x04, 3, false, 0.01f, "press"},
  {0x05, 3, false, 0.01f, "lux"},
  {0x06, 2, false, 0.01f, "kg"},
  {0x07, 2, false, 0.01f, "lb"},
  {0x08, 2, true, 0.01f, "dew"},
  {0x09, 1, false, 1.0f, "cnt8"},
  {0x0A, 3, false, 0.001f, "energy"},
  {0x0B, 3, false, 0.01f, "power"},
  {0x0C, 2, false, 0.001f, "volt"},
  {0x0D, 2, false, 1.0f, "pm25"},
  {0x0E, 2, false, 1.0f, "pm10"},
  {0x0F, 1, false, 1.0f, "bool"},
  {0x10, 1, false, 1.0f, "pwrsw"},
  {0x11, 1, false, 1.0f, "open"},
  {0x12, 2, false, 1.0f, "co2"},
  {0x13, 2, false, 1.0f, "tvoc"},
  {0x14, 2, false, 0.01f, "moist"},
  {0x15, 1, false, 1.0f, "batlow"},
  {0x16, 1, false, 1.0f, "charge"},
  {0x17, 1, false, 1.0f, "co"},
  {0x18, 1, false, 1.0f, "cold"},
  {0x19, 1, false, 1.0f, "conn"},
  {0x1A, 1, false, 1.0f, "door"},
  {0x1B, 1, false, 1.0f, "garage"},
  {0x1C, 1, false, 1.0f, "gas"},
  {0x1D, 1, false, 1.0f, "heat"},
  {0x1E, 1, false, 1.0f, "light"},
  {0x1F, 1, false, 1.0f, "lock"},
  {0x20, 1, false, 1.0f, "moistbin"},
  {0x21, 1, false, 1.0f, "motion"},
  {0x22, 1, false, 1.0f, "moving"},
  {0x23, 1, false, 1.0f, "occup"},
  {0x24, 1, false, 1.0f, "plug"},
  {0x25, 1, false, 1.0f, "presence"},
  {0x26, 1, false, 1.0f, "problem"},
  {0x27, 1, false, 1.0f, "running"},
  {0x28, 1, false, 1.0f, "safety"},
  {0x29, 1, false, 1.0f, "smoke"},
  {0x2A, 1, false, 1.0f, "sound"},
  {0x2B, 1, false, 1.0f, "tamper"},
  {0x2C, 1, false, 1.0f, "vibrate"},
  {0x2D, 1, false, 1.0f, "window"},
  {0x2E, 1, false, 1.0f, "hum"},
  {0x2F, 1, false, 1.0f, "moist"},
  {0x3A, 1, false, 1.0f, "button"},
  {0x3D, 2, false, 1.0f, "cnt16"},
  {0x3C, 1, true, 1.0f, "dimmer"},
  {0x3E, 4, false, 1.0f, "cnt32"},
  {0x3F, 2, true, 0.1f, "rot"},
  {0x40, 2, false, 1.0f, "dist"},
  {0x41, 2, false, 0.1f, "dist"},
  {0x42, 3, false, 0.001f, "dur"},
  {0x43, 2, false, 0.001f, "curr"},
  {0x44, 2, false, 0.01f, "speed"},
  {0x45, 2, true, 0.1f, "temp"},
  {0x46, 1, false, 0.1f, "uv"},
  {0x47, 2, false, 0.1f, "vol"},
  {0x48, 2, false, 1.0f, "ml"},
  {0x49, 2, false, 0.001f, "flow"},
  {0x4A, 2, false, 0.1f, "volt"},
  {0x4B, 3, false, 0.001f, "gas"},
  {0x4C, 4, false, 0.001f, "gas32"},
  {0x4D, 4, false, 0.001f, "energy"},
  {0x4E, 4, false, 0.001f, "vol32"},
  {0x4F, 4, false, 0.001f, "water"},
  {0x50, 4, false, 1.0f, "time"},
  {0x51, 2, false, 0.001f, "accel"},
  {0x52, 2, false, 0.001f, "gyro"},
  {0x55, 4, false, 0.001f, "store"},
  {0x56, 2, false, 1.0f, "cond"},
  {0x57, 1, true, 1.0f, "temp"},
  {0x58, 1, true, 0.35f, "temp"},
  {0x59, 1, true, 1.0f, "cnts8"},
  {0x5A, 2, true, 1.0f, "cnts16"},
  {0x5B, 4, true, 1.0f, "cnts32"},
  {0x5C, 4, true, 0.01f, "power"},
  {0x5D, 2, true, 0.001f, "curr"},
  {0x5E, 2, false, 0.01f, "dir"},
  {0x5F, 2, false, 0.01f, "rain"},
  {0x60, 1, false, 1.0f, "chan"},
  {0x61, 2, false, 1.0f, "rpm"},
  {0x62, 4, true, 0.000001f, "speeds"},
  {0x63, 4, true, 0.000001f, "accels"},
  {0x64, 1, false, 1.0f, "light"},
};

static const BTHomeObjectDef* findObjectDef(uint8_t object_id) {
  for (const auto& def : kObjectDefs) {
    if (def.object_id == object_id) {
      return &def;
    }
  }
  return nullptr;
}

static int findOccurrenceIndex(const uint8_t object_ids[], uint8_t counts[], uint8_t num_ids, uint8_t object_id) {
  for (uint8_t i = 0; i < num_ids; i++) {
    if (object_ids[i] == object_id) {
      return i;
    }
  }
  return -1;
}

static float decodeValue(const uint8_t* data, uint8_t size, bool is_signed, float factor) {
  uint32_t raw = 0;
  for (uint8_t i = 0; i < size; i++) {
    raw |= ((uint32_t) data[i]) << (i * 8);
  }

  int32_t signed_value = raw;
  if (is_signed) {
    const uint32_t sign_bit = 1UL << ((size * 8) - 1);
    if ((raw & sign_bit) != 0) {
      signed_value = raw - (1UL << (size * 8));
    }
  }

  return (is_signed ? signed_value : (int32_t) raw) * factor;
}

static void copyName(char dest[], size_t dest_len, const std::string& name) {
  if (dest_len == 0) {
    return;
  }
  if (name.empty()) {
    dest[0] = 0;
    return;
  }
  strncpy(dest, name.c_str(), dest_len - 1);
  dest[dest_len - 1] = 0;
}

static const char* getObjectLabel(uint8_t object_id) {
  const auto* def = findObjectDef(object_id);
  return def != nullptr ? def->label : "obj";
}

static bool extractBTHomeServiceData(const uint8_t* raw,
                                     size_t raw_len,
                                     const uint8_t*& payload,
                                     size_t& payload_len,
                                     char* name,
                                     size_t name_len) {
  payload = nullptr;
  payload_len = 0;
  if (name != nullptr && name_len > 0) {
    name[0] = 0;
  }

  size_t pos = 0;
  while (pos < raw_len) {
    const uint8_t field_len = raw[pos];
    if (field_len == 0 || pos + field_len >= raw_len) {
      break;
    }

    const uint8_t ad_type = raw[pos + 1];
    if (ad_type == 0x16 && field_len >= 3) {
      const uint16_t uuid = raw[pos + 2] | (raw[pos + 3] << 8);
      if (uuid == kBTHomeServiceUuid) {
        payload = &raw[pos + 4];
        payload_len = field_len - 3;
      }
    } else if ((ad_type == 0x08 || ad_type == 0x09) && field_len > 1 && name != nullptr && name_len > 0) {
      const size_t copy_len = min((size_t)(field_len - 1), name_len - 1);
      memcpy(name, &raw[pos + 2], copy_len);
      name[copy_len] = 0;
    }

    pos += field_len + 1;
  }

  return payload != nullptr && payload_len > 0;
}

static void copyDisplayMac(const uint8_t src[6], uint8_t dest[6]) {
  for (int i = 0; i < 6; i++) {
    dest[i] = src[5 - i];
  }
}

static bool g_ble_inited = false;
static bool g_scanning = false;
static BTHomeScanner* g_active_scanner = nullptr;

static uint8_t getOwnAddrType() {
  if (ble_hs_id_copy_addr(BLE_OWN_ADDR_PUBLIC, nullptr, nullptr) == 0) {
    return BLE_OWN_ADDR_PUBLIC;
  }
  return BLE_OWN_ADDR_RANDOM;
}

static bool startScan();

static bool stopScan() {
  int rc = ble_gap_disc_cancel();
  return rc == 0 || rc == BLE_HS_EALREADY || rc == BLE_HS_EINVAL;
}

static int handleGapEvent(ble_gap_event* event, void* arg) {
  (void) arg;

  switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
      BTHomeScanner* scanner = g_active_scanner;
      if (scanner == nullptr || !scanner->isEnabled()) {
        return 0;
      }

      const auto& disc = event->disc;
      const uint8_t* payload = nullptr;
      size_t payload_len = 0;
      char name[24];
      if (!extractBTHomeServiceData(disc.data, disc.length_data, payload, payload_len, name, sizeof(name))) {
        return 0;
      }

      uint8_t mac[6];
      copyDisplayMac(disc.addr.val, mac);
      scanner->handleScanResult(mac,
                                name[0] != 0 ? name : nullptr,
                                disc.rssi,
                                payload,
                                payload_len);
      return 0;
    }

    case BLE_GAP_EVENT_DISC_COMPLETE:
      g_scanning = false;
      return 0;

    default:
      return 0;
  }
}

static bool startScan() {
  if (g_scanning) {
    return true;
  }

  ble_gap_disc_params disc_params{};
  disc_params.passive = 1;
  disc_params.filter_duplicates = 0;
  disc_params.itvl = 160;
  disc_params.window = 80;
  disc_params.limited = 0;

  int rc = ble_gap_disc(getOwnAddrType(), BLE_HS_FOREVER, &disc_params, handleGapEvent, nullptr);
  if (rc == 0) {
    g_scanning = true;
    return true;
  }

  return false;
}

}  // namespace
#endif

BTHomeScanner::BTHomeScanner() {
  _enabled = false;
  _target_count = 0;
  memset(_target_macs, 0, sizeof(_target_macs));
  clearCache();
}

void BTHomeScanner::begin() {
#if defined(ESP32_PLATFORM)
  g_active_scanner = this;
  if (!_enabled && !g_ble_inited) {
    return;
  }

  if (!g_ble_inited) {
    if (!NimBLEDevice::init("MeshCore-BTHome")) {
      return;
    }
    g_ble_inited = true;
  }
#endif
}

void BTHomeScanner::loop() {
#if defined(ESP32_PLATFORM)
  if (_enabled && g_ble_inited && !g_scanning) {
    startScan();
  }
#endif
}

void BTHomeScanner::setEnabled(bool enabled) {
  if (_enabled == enabled) {
    return;
  }
  _enabled = enabled;
#if defined(ESP32_PLATFORM)
  if (_enabled) {
    begin();
    startScan();
  } else {
    stopScan();
    g_scanning = false;
  }
#endif
}

bool BTHomeScanner::isEnabled() const {
  return _enabled;
}

void BTHomeScanner::clearCache() {
  memset(_devices, 0, sizeof(_devices));
}

uint8_t BTHomeScanner::getDeviceCount() const {
  uint8_t count = 0;
  for (const auto& device : _devices) {
    if (device.used) {
      count++;
    }
  }
  return count;
}

bool BTHomeScanner::getDeviceMacByIndex(uint8_t index, uint8_t mac[6]) const {
  const DeviceCache* device = findDeviceByIndex(_devices, index);
  if (device == nullptr) {
    return false;
  }

  memcpy(mac, device->mac, 6);
  return true;
}

bool BTHomeScanner::addTargetMac(const uint8_t mac[6]) {
  for (uint8_t i = 0; i < _target_count; i++) {
    if (macEquals(_target_macs[i], mac)) {
      return true;
    }
  }
  if (_target_count >= MAX_TARGETS) {
    return false;
  }
  memcpy(_target_macs[_target_count], mac, sizeof(_target_macs[0]));
  _target_count++;
  return true;
}

bool BTHomeScanner::removeTargetMac(const uint8_t mac[6]) {
  for (uint8_t i = 0; i < _target_count; i++) {
    if (!macEquals(_target_macs[i], mac)) {
      continue;
    }
    for (uint8_t j = i + 1; j < _target_count; j++) {
      memcpy(_target_macs[j - 1], _target_macs[j], sizeof(_target_macs[0]));
    }
    memset(_target_macs[_target_count - 1], 0, sizeof(_target_macs[0]));
    _target_count--;
    return true;
  }
  return false;
}

void BTHomeScanner::clearTargetMacs() {
  _target_count = 0;
  memset(_target_macs, 0, sizeof(_target_macs));
}

uint8_t BTHomeScanner::getTargetCount() const {
  return _target_count;
}

bool BTHomeScanner::getTargetMac(uint8_t index, uint8_t mac[6]) const {
  if (index >= _target_count) {
    return false;
  }
  memcpy(mac, _target_macs[index], sizeof(_target_macs[0]));
  return true;
}

bool BTHomeScanner::isTargetMac(const uint8_t mac[6]) const {
  for (uint8_t i = 0; i < _target_count; i++) {
    if (macEquals(_target_macs[i], mac)) {
      return true;
    }
  }
  return false;
}

bool BTHomeScanner::isKnownEncrypted(const uint8_t mac[6]) const {
  const DeviceCache* device = findDeviceByMac(mac);
  return device != nullptr && device->encrypted;
}

const BTHomeScanner::DeviceCache* BTHomeScanner::findDeviceByMac(const uint8_t mac[6]) const {
  for (const auto& device : _devices) {
    if (device.used && macEquals(device.mac, mac)) {
      return &device;
    }
  }
  return nullptr;
}

#if defined(ESP32_PLATFORM)
namespace {

static BTHomeScanner::DeviceCache* allocDevice(BTHomeScanner::DeviceCache (&devices)[8],
                                                      const uint8_t mac[6]) {
  BTHomeScanner::DeviceCache* free_slot = nullptr;
  BTHomeScanner::DeviceCache* oldest_slot = nullptr;
  for (auto& device : devices) {
    if (device.used && macEquals(device.mac, mac)) {
      return &device;
    }
    if (!device.used && free_slot == nullptr) {
      free_slot = &device;
    }
    if (device.used && (oldest_slot == nullptr || device.last_seen < oldest_slot->last_seen)) {
      oldest_slot = &device;
    }
  }

  auto* slot = free_slot != nullptr ? free_slot : oldest_slot;
  if (slot != nullptr) {
    memset(slot, 0, sizeof(*slot));
    slot->used = true;
    memcpy(slot->mac, mac, 6);
  }
  return slot;
}

static BTHomeScanner::MeasurementSlot* findMeasurementSlot(BTHomeScanner::DeviceCache& device,
                                                                  uint8_t object_id,
                                                                  uint8_t occurrence) {
  BTHomeScanner::MeasurementSlot* free_slot = nullptr;
  BTHomeScanner::MeasurementSlot* oldest_slot = nullptr;
  for (auto& slot : device.measurements) {
    if (slot.used && slot.object_id == object_id && slot.occurrence == occurrence) {
      return &slot;
    }
    if (!slot.used && free_slot == nullptr) {
      free_slot = &slot;
    }
    if (slot.used && (oldest_slot == nullptr || slot.seen_at < oldest_slot->seen_at)) {
      oldest_slot = &slot;
    }
  }

  auto* slot = free_slot != nullptr ? free_slot : oldest_slot;
  if (slot != nullptr) {
    memset(slot, 0, sizeof(*slot));
    slot->used = true;
    slot->object_id = object_id;
    slot->occurrence = occurrence;
  }
  return slot;
}

static const BTHomeScanner::MeasurementSlot* findMeasurementSlot(
    const BTHomeScanner::DeviceCache& device,
    uint8_t object_id,
    uint8_t occurrence,
    unsigned long freshness_ms) {
  for (const auto& slot : device.measurements) {
    if (!slot.used) {
      continue;
    }
    if (slot.object_id == object_id && slot.occurrence == occurrence && isFresh(slot.seen_at, freshness_ms)) {
      return &slot;
    }
  }
  return nullptr;
}

static unsigned long getMeasurementRetentionMs(unsigned long freshness_ms) {
  return max(freshness_ms, BTHomeScanner::DEFAULT_MEASUREMENT_RETENTION_MS);
}

static bool isBinaryObjectId(uint8_t object_id) {
  switch (object_id) {
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x2D:
      return true;
    default:
      return false;
  }
}

static uint8_t getBinaryFieldType(uint8_t object_id) {
  switch (object_id) {
    case 0x0F: return LPP_BINARY_BOOL;
    case 0x10: return LPP_BINARY_POWER_SWITCH;
    case 0x11: return LPP_BINARY_OPEN;
    case 0x15: return LPP_BINARY_BATTERY_LOW;
    case 0x16: return LPP_BINARY_CHARGING;
    case 0x17: return LPP_BINARY_CARBON_MONOXIDE;
    case 0x18: return LPP_BINARY_COLD;
    case 0x19: return LPP_BINARY_CONNECTIVITY;
    case 0x1A: return LPP_BINARY_DOOR;
    case 0x1B: return LPP_BINARY_GARAGE_DOOR;
    case 0x1C: return LPP_BINARY_GAS;
    case 0x1D: return LPP_BINARY_HEAT;
    case 0x1E: return LPP_BINARY_LIGHT;
    case 0x1F: return LPP_BINARY_LOCK;
    case 0x20: return LPP_BINARY_MOISTURE;
    case 0x21: return LPP_BINARY_MOTION;
    case 0x22: return LPP_BINARY_MOVING;
    case 0x23: return LPP_BINARY_OCCUPANCY;
    case 0x24: return LPP_BINARY_PLUG;
    case 0x25: return LPP_BINARY_PRESENCE;
    case 0x26: return LPP_BINARY_PROBLEM;
    case 0x27: return LPP_BINARY_RUNNING;
    case 0x28: return LPP_BINARY_SAFETY;
    case 0x29: return LPP_BINARY_SMOKE;
    case 0x2A: return LPP_BINARY_SOUND;
    case 0x2B: return LPP_BINARY_TAMPER;
    case 0x2C: return LPP_BINARY_VIBRATION;
    case 0x2D: return LPP_BINARY_WINDOW;
    default:
      return 0;
  }
}

static const char* getButtonEventName(uint8_t event_type) {
  switch (event_type) {
    case 0x00:
      return "none";
    case 0x01:
      return "press";
    case 0x02:
      return "double";
    case 0x03:
      return "triple";
    case 0x04:
      return "long";
    case 0x05:
      return "longdbl";
    case 0x06:
      return "longtri";
    case 0x80:
      return "hold";
    default:
      return nullptr;
  }
}

static float normalizeDistance(const BTHomeScanner::MeasurementSlot& slot) {
  if (slot.object_id == 0x40) {
    return slot.value / 1000.0f;
  }
  return slot.value;
}

static float normalizeMass(const BTHomeScanner::MeasurementSlot& slot) {
  if (slot.object_id == 0x07) {
    return slot.value * 0.45359237f;
  }
  return slot.value;
}

static float normalizeVolume(const BTHomeScanner::MeasurementSlot& slot) {
  if (slot.object_id == 0x48) {
    return slot.value / 1000.0f;
  }
  return slot.value;
}

static bool appendAnalogInputField(MeshCayenneLPP& telemetry, uint8_t channel, float value) {
  if (value < -327.67f || value > 327.67f) {
    return false;
  }
  return telemetry.addAnalogInput(channel, value) != 0;
}

static bool isWholeNumber(float value) {
  return fabsf(value - roundf(value)) < 0.0005f;
}

static bool appendGenericSensorField(MeshCayenneLPP& telemetry, uint8_t channel, float value) {
  if (value < 0.0f || value > 4294967295.0f || !isWholeNumber(value)) {
    return false;
  }
  return telemetry.addGenericSensor(channel, roundf(value)) != 0;
}

static bool appendCountField(MeshCayenneLPP& telemetry, uint8_t channel, float value) {
  if (appendGenericSensorField(telemetry, channel, value)) {
    return true;
  }
  return appendAnalogInputField(telemetry, channel, value);
}

static uint8_t getCountFieldType(const BTHomeScanner::MeasurementSlot* slot) {
  if (slot != nullptr && slot->value >= 0.0f && isWholeNumber(slot->value)) {
    return LPP_GENERIC_SENSOR;
  }
  return LPP_ANALOG_INPUT;
}

static bool isOfficialClientSupportedType(uint8_t type) {
  switch (type) {
    case LPP_DIGITAL_INPUT:
    case LPP_DIGITAL_OUTPUT:
    case LPP_ANALOG_INPUT:
    case LPP_ANALOG_OUTPUT:
    case LPP_GENERIC_SENSOR:
    case LPP_LUMINOSITY:
    case LPP_PRESENCE:
    case LPP_TEMPERATURE:
    case LPP_RELATIVE_HUMIDITY:
    case LPP_BAROMETRIC_PRESSURE:
    case LPP_VOLTAGE:
    case LPP_CURRENT:
    case LPP_FREQUENCY:
    case LPP_PERCENTAGE:
    case LPP_ALTITUDE:
    case LPP_CONCENTRATION:
    case LPP_POWER:
    case LPP_DISTANCE:
    case LPP_ENERGY:
    case LPP_DIRECTION:
    case LPP_UNIXTIME:
    case LPP_GPS:
    case LPP_SWITCH:
      return true;
    default:
      return false;
  }
}

static bool appendBTHomeField(MeshCayenneLPP& telemetry,
                              uint8_t channel,
                              const BTHomeScanner::MeasurementSlot* slot,
                              bool override_rain,
                              float override_rain_value) {
  if (slot == nullptr) {
    return false;
  }
  if (isBinaryObjectId(slot->object_id)) {
    const uint8_t field_type = getBinaryFieldType(slot->object_id);
    return field_type != 0 && telemetry.addCustomU8(channel, field_type, slot->value >= 0.5f ? 1 : 0) != 0;
  }
  if (slot->object_id == kButtonObjectId) {
    return telemetry.addCustomU8(channel, LPP_BUTTON_EVENT, max(0, (int) roundf(slot->value))) != 0;
  }
  if (slot->object_id == kDimmerObjectId) {
    return telemetry.addCustomS8(channel, LPP_DIMMER, (int8_t) roundf(slot->value)) != 0;
  }

  switch (slot->object_id) {
    case 0x00:
      return appendCountField(telemetry, channel, slot->value);
    case 0x01:
      return telemetry.addPercentage(channel, constrain((int) roundf(slot->value), 0, 100)) != 0;
    case 0x02:
    case 0x45:
    case 0x57:
    case 0x58:
      return telemetry.addTemperature(channel, slot->value) != 0;
    case 0x08:
      return telemetry.addDewPoint(channel, slot->value) != 0;
    case 0x03:
    case 0x2E:
      return telemetry.addRelativeHumidity(channel, slot->value) != 0;
    case 0x14:
    case 0x2F:
      return telemetry.addPercentage(channel, constrain((int) roundf(slot->value), 0, 100)) != 0;
    case 0x04:
      return telemetry.addBarometricPressure(channel, slot->value) != 0;
    case 0x05:
      return telemetry.addLuminosity(channel, max(0, (int) roundf(slot->value))) != 0;
    case 0x06:
    case 0x07:
      return telemetry.addCustomScaledU32(channel, LPP_MASS, LPP_MASS_MULT, normalizeMass(*slot)) != 0;
    case 0x09:
    case 0x3D:
    case 0x3E:
      return appendCountField(telemetry, channel, slot->value);
    case 0x0A:
    case 0x4D:
      return telemetry.addEnergy(channel, slot->value) != 0;
    case 0x0B:
      return telemetry.addPower(channel, slot->value) != 0;  // standard LPP type 128, 2B unsigned
    case 0x5C:
      return telemetry.addCustomScaledS32(channel, LPP_SIGNED_POWER, LPP_SIGNED_POWER_MULT, slot->value) != 0;
    case 0x0C:
    case 0x4A:
      return appendAnalogInputField(telemetry, channel, slot->value);
    case 0x0D:
      return telemetry.addCustomScaledU16(channel, LPP_PM25, LPP_PM25_MULT, slot->value) != 0;
    case 0x0E:
      return telemetry.addCustomScaledU16(channel, LPP_PM10, LPP_PM10_MULT, slot->value) != 0;
    case 0x12:
      return telemetry.addCustomScaledU16(channel, LPP_CO2, LPP_CO2_MULT, slot->value) != 0;
    case 0x13:
      return telemetry.addCustomScaledU16(channel, LPP_TVOC, LPP_TVOC_MULT, slot->value) != 0;
    case 0x3F:
      return telemetry.addCustomScaledS16(channel, LPP_ROTATION, LPP_ROTATION_MULT, slot->value) != 0;
    case 0x40:
    case 0x41:
      return telemetry.addDistance(channel, normalizeDistance(*slot)) != 0;
    case 0x42:
      return telemetry.addCustomScaledU32(channel, LPP_DURATION, LPP_DURATION_MULT, slot->value) != 0;
    case 0x43:
      return telemetry.addCurrent(channel, slot->value) != 0;  // standard LPP type 117, 2B unsigned
    case 0x5D:
      return telemetry.addCustomScaledS32(channel, LPP_SIGNED_CURRENT, LPP_SIGNED_CURRENT_MULT, slot->value) != 0;
    case 0x44:
      if (slot->occurrence == 1) {
        return telemetry.addGust(channel, slot->value) != 0;
      }
      return telemetry.addSpeed(channel, slot->value) != 0;
    case 0x46:
      return telemetry.addCustomScaledU8(channel, LPP_UV, LPP_UV_MULT, slot->value) != 0;
    case 0x47:
    case 0x48:
    case 0x4E:
      return telemetry.addCustomScaledU32(channel, LPP_VOLUME, LPP_VOLUME_MULT, normalizeVolume(*slot)) != 0;
    case 0x49:
      return telemetry.addCustomScaledU32(channel, LPP_FLOW_RATE, LPP_FLOW_RATE_MULT, slot->value) != 0;
    case 0x4B:
    case 0x4C:
      return telemetry.addCustomScaledU32(channel, LPP_GAS_VOLUME, LPP_GAS_VOLUME_MULT, slot->value) != 0;
    case 0x4F:
      return telemetry.addCustomScaledU32(channel, LPP_WATER, LPP_WATER_MULT, slot->value) != 0;
    case 0x50:
      return telemetry.addUnixTime(channel, max(0, (int32_t) roundf(slot->value))) != 0;
    case 0x51:
    case 0x63:
      return telemetry.addCustomScaledS32(channel, LPP_ACCELERATION, LPP_ACCELERATION_MULT, slot->value) != 0;
    case 0x52:
      return telemetry.addCustomScaledS32(channel, LPP_GYRO_RATE, LPP_GYRO_RATE_MULT, slot->value) != 0;
    case 0x55:
      return telemetry.addCustomScaledU32(channel, LPP_VOLUME_STORAGE, LPP_VOLUME_STORAGE_MULT, slot->value) != 0;
    case 0x56:
      return telemetry.addCustomScaledU16(channel, LPP_CONDUCTIVITY, LPP_CONDUCTIVITY_MULT, slot->value) != 0;
    case 0x5F:
      return telemetry.addRain(channel, override_rain ? override_rain_value : slot->value) != 0;
    case 0x61:
      return telemetry.addCustomScaledU16(channel, LPP_RPM, LPP_RPM_MULT, slot->value) != 0;
    case 0x62:
      return telemetry.addCustomScaledS32(channel, LPP_SIGNED_SPEED, LPP_SIGNED_SPEED_MULT, slot->value) != 0;
    case 0x64:
      return telemetry.addCustomU8(channel, LPP_LIGHT_LEVEL, max(0, (int) roundf(slot->value))) != 0;
    case 0x59:
    case 0x5A:
    case 0x5B:
      return appendCountField(telemetry, channel, slot->value);
    case 0x5E:
      return telemetry.addDirection(channel, slot->value) != 0;
    default:
      return false;
  }
}

static uint8_t getBTHomeFieldType(const BTHomeScanner::MeasurementSlot* slot) {
  if (slot == nullptr) {
    return 0;
  }
  if (isBinaryObjectId(slot->object_id)) {
    return getBinaryFieldType(slot->object_id);
  }
  if (slot->object_id == kButtonObjectId) {
    return LPP_BUTTON_EVENT;
  }
  if (slot->object_id == kDimmerObjectId) {
    return LPP_DIMMER;
  }

  switch (slot->object_id) {
    case 0x00:
      // Keep packet metadata visible in the CLI, but do not publish it.
      return 0;
    case 0x01:
      return LPP_PERCENTAGE;
    case 0x02:
    case 0x45:
    case 0x57:
    case 0x58:
      return LPP_TEMPERATURE;
    case 0x08:
      return LPP_DEWPOINT;
    case 0x03:
    case 0x2E:
      return LPP_RELATIVE_HUMIDITY;
    case 0x14:
    case 0x2F:
      return LPP_PERCENTAGE;
    case 0x04:
      return LPP_BAROMETRIC_PRESSURE;
    case 0x05:
      return LPP_LUMINOSITY;
    case 0x06:
    case 0x07:
      return LPP_MASS;
    case 0x3F:
      return LPP_ROTATION;
    case 0x42:
      return LPP_DURATION;
    case 0x46:
      return LPP_UV;
    case 0x49:
      return LPP_FLOW_RATE;
    case 0x51:
    case 0x63:
      return LPP_ACCELERATION;
    case 0x52:
      return LPP_GYRO_RATE;
    case 0x09:
    case 0x3D:
    case 0x3E:
      return 0;
    case 0x0A:
    case 0x4D:
      return LPP_ENERGY;
    case 0x0B:
    case 0x5C:
      return LPP_SIGNED_POWER;
    case 0x0C:
    case 0x4A:
      return LPP_ANALOG_INPUT;
    case 0x0D:
      return LPP_PM25;
    case 0x0E:
      return LPP_PM10;
    case 0x12:
      return LPP_CO2;
    case 0x13:
      return LPP_TVOC;
    case 0x40:
    case 0x41:
      return LPP_DISTANCE;
    case 0x43:
    case 0x5D:
      return LPP_SIGNED_CURRENT;
    case 0x47:
    case 0x48:
    case 0x4E:
      return LPP_VOLUME;
    case 0x4F:
      return LPP_WATER;
    case 0x50:
      return LPP_UNIXTIME;
    case 0x55:
      return LPP_VOLUME_STORAGE;
    case 0x56:
      return LPP_CONDUCTIVITY;
    case 0x5F:
      return LPP_RAIN;
    case 0x61:
      return LPP_RPM;
    case 0x62:
      return LPP_SIGNED_SPEED;
    case 0x64:
      return LPP_LIGHT_LEVEL;
    case 0x4B:
    case 0x4C:
      return LPP_GAS_VOLUME;
    case 0x60:
      return 0;
    case 0x59:
    case 0x5A:
    case 0x5B:
      return 0;
    case 0x44:
      if (slot->occurrence == 1) {
        return LPP_GUST;
      }
      return LPP_SPEED;
    case 0x5E:
      return LPP_DIRECTION;
    default:
      return 0;
  }
}

static bool channelHasType(const uint8_t types[], uint8_t count, uint8_t type) {
  for (uint8_t i = 0; i < count; i++) {
    if (types[i] == type) {
      return true;
    }
  }
  return false;
}

static uint8_t seedExistingChannelTypes(MeshCayenneLPP& telemetry,
                                        uint8_t base_channel,
                                        uint8_t channel_types[][BTHomeScanner::TELEMETRY_FIELD_COUNT],
                                        uint8_t channel_type_counts[]) {
  uint8_t used_channels = 0;
  LPPReader reader(telemetry.getBuffer(), telemetry.getSize());
  uint8_t channel = 0;
  uint8_t type = 0;
  while (reader.readHeader(channel, type)) {
    if (channel >= base_channel) {
      const uint16_t channel_index = (uint16_t) channel - base_channel;
      if (channel_index < BTHomeScanner::TELEMETRY_FIELD_COUNT) {
        if (!channelHasType(channel_types[channel_index], channel_type_counts[channel_index], type) &&
            channel_type_counts[channel_index] < BTHomeScanner::TELEMETRY_FIELD_COUNT) {
          channel_types[channel_index][channel_type_counts[channel_index]++] = type;
        }
        if (used_channels < (channel_index + 1U)) {
          used_channels = (uint8_t) (channel_index + 1U);
        }
      }
    }
    reader.skipData(type);
  }
  return used_channels;
}

struct MeasurementRef {
  const BTHomeScanner::MeasurementSlot* slot;
  uint8_t order;
};

static uint8_t getObjectOrder(uint8_t object_id) {
  for (uint8_t i = 0; i < sizeof(kObjectDefs) / sizeof(kObjectDefs[0]); i++) {
    if (kObjectDefs[i].object_id == object_id) {
      return i;
    }
  }
  return 0xFF;
}

static void formatSlotLabel(const BTHomeScanner::MeasurementSlot& slot, char* dest, size_t len) {
  const char* label = getObjectLabel(slot.object_id);
  if (slot.object_id == kButtonObjectId) {
    snprintf(dest, len, "btn%u", slot.occurrence);
    return;
  }
  if (slot.object_id == 0x44 && slot.occurrence == 1) {
    strncpy(dest, "gust", len - 1);
    dest[len - 1] = 0;
    return;
  }

  if (slot.occurrence == 0) {
    strncpy(dest, label, len - 1);
    dest[len - 1] = 0;
    return;
  }

  snprintf(dest, len, "%s%u", label, slot.occurrence);
}

static void formatSlotValue(const BTHomeScanner::MeasurementSlot& slot,
                            char* dest,
                            size_t len,
                            bool normalize_distance = false) {
  if (len == 0) {
    return;
  }
  if (isBinaryObjectId(slot.object_id)) {
    strncpy(dest, slot.value >= 0.5f ? "on" : "off", len - 1);
    dest[len - 1] = 0;
    return;
  }
  if (slot.object_id == kButtonObjectId) {
    const char* event_name = getButtonEventName((uint8_t) roundf(slot.value));
    if (event_name != nullptr) {
      strncpy(dest, event_name, len - 1);
      dest[len - 1] = 0;
    } else {
      snprintf(dest, len, "%.0f", slot.value);
    }
    return;
  }

  float value = normalize_distance ? normalizeDistance(slot) : slot.value;
  snprintf(dest, len, "%.2f", value);
}

static void formatDeviceLabel(const BTHomeScanner::DeviceCache& device, char* dest, size_t len) {
  if (len == 0) {
    return;
  }
  if (device.name[0] != 0) {
    strncpy(dest, device.name, len - 1);
    dest[len - 1] = 0;
    return;
  }
  BTHomeScanner::formatMac(device.mac, dest, len);
}

static const BTHomeScanner::MeasurementSlot* findFirstFreshMeasurement(
    const BTHomeScanner::DeviceCache& device,
    const uint8_t object_ids[],
    uint8_t object_id_count,
    unsigned long freshness_ms) {
  for (uint8_t i = 0; i < object_id_count; i++) {
    const BTHomeScanner::MeasurementSlot* slot = findMeasurementSlot(device, object_ids[i], 0, freshness_ms);
    if (slot != nullptr) {
      return slot;
    }
  }
  return nullptr;
}

static bool getMetReportObservation(const BTHomeScanner::DeviceCache& device,
                                    char* label,
                                    size_t label_len,
                                    float& temperature,
                                    float& humidity,
                                    float& wind_speed,
                                    float& gust,
                                    unsigned long freshness_ms) {
  if (device.encrypted) {
    return false;
  }

  const unsigned long retention_ms = getMeasurementRetentionMs(freshness_ms);
  if (!isFresh(device.last_seen, retention_ms)) {
    return false;
  }

  static const uint8_t kTemperatureObjectIds[] = { 0x02, 0x45, 0x57, 0x58 };
  static const uint8_t kHumidityObjectIds[] = { 0x03, 0x2E };

  const BTHomeScanner::MeasurementSlot* temperature_slot = findFirstFreshMeasurement(
      device, kTemperatureObjectIds, sizeof(kTemperatureObjectIds), retention_ms);
  const BTHomeScanner::MeasurementSlot* humidity_slot = findFirstFreshMeasurement(
      device, kHumidityObjectIds, sizeof(kHumidityObjectIds), retention_ms);
  const BTHomeScanner::MeasurementSlot* wind_slot = findMeasurementSlot(device, 0x44, 0, retention_ms);
  const BTHomeScanner::MeasurementSlot* gust_slot = findMeasurementSlot(device, 0x44, 1, retention_ms);

  if (temperature_slot == nullptr || humidity_slot == nullptr || wind_slot == nullptr || gust_slot == nullptr) {
    return false;
  }

  if (label != nullptr && label_len > 0) {
    formatDeviceLabel(device, label, label_len);
  }

  temperature = temperature_slot->value;
  humidity = humidity_slot->value;
  wind_speed = wind_slot->value;
  gust = gust_slot->value;
  return true;
}

static bool getRainMeasurement(const BTHomeScanner::DeviceCache& device,
                               float& rain,
                               unsigned long freshness_ms) {
  if (device.encrypted) {
    return false;
  }

  const unsigned long retention_ms = getMeasurementRetentionMs(freshness_ms);
  if (!isFresh(device.last_seen, retention_ms)) {
    return false;
  }

  const BTHomeScanner::MeasurementSlot* rain_slot = findMeasurementSlot(device, 0x5F, 0, retention_ms);
  if (rain_slot == nullptr) {
    return false;
  }

  rain = rain_slot->value;
  return true;
}

static uint8_t collectOrderedMeasurements(const BTHomeScanner::DeviceCache& device,
                                         unsigned long freshness_ms,
                                         MeasurementRef refs[],
                                         uint8_t max_refs) {
  uint8_t count = 0;
  for (const auto& slot : device.measurements) {
    if (!slot.used || !isFresh(slot.seen_at, freshness_ms)) {
      continue;
    }
    if (count >= max_refs) {
      break;
    }
    refs[count].slot = &slot;
    refs[count].order = getObjectOrder(slot.object_id);
    count++;
  }

  for (uint8_t i = 1; i < count; i++) {
    MeasurementRef current = refs[i];
    int j = i - 1;
    while (j >= 0 && (refs[j].order > current.order ||
           (refs[j].order == current.order && refs[j].slot->occurrence > current.slot->occurrence))) {
      refs[j + 1] = refs[j];
      j--;
    }
    refs[j + 1] = current;
  }
  return count;
}

static void appendFieldSummary(Print& out,
                               const char* name,
                               const BTHomeScanner::MeasurementSlot* slot,
                               bool normalize_distance = false) {
  if (slot == nullptr) {
    return;
  }
  char buf[32];
  char value[16];
  formatSlotValue(*slot, value, sizeof(value), normalize_distance);
  snprintf(buf, sizeof(buf), " %s=%s", name, value);
  out.print(buf);
}

static bool appendToBuffer(char* dest, size_t len, size_t& used, const char* fmt, ...) {
  if (len == 0 || used >= len - 1) {
    return false;
  }

  va_list args;
  va_start(args, fmt);
  int written = vsnprintf(&dest[used], len - used, fmt, args);
  va_end(args);
  if (written <= 0) {
    return false;
  }

  size_t available = len - used;
  if ((size_t) written >= available) {
    used = len - 1;
    dest[used] = 0;
    return false;
  }

  used += written;
  return true;
}

static bool appendFieldSummary(char* dest,
                               size_t len,
                               size_t& used,
                               const char* name,
                               const BTHomeScanner::MeasurementSlot* slot,
                               bool normalize_distance = false) {
  if (slot == nullptr) {
    return true;
  }

  char value[16];
  formatSlotValue(*slot, value, sizeof(value), normalize_distance);
  return appendToBuffer(dest, len, used, " %s=%s", name, value);
}

static const BTHomeScanner::DeviceCache* findDeviceByIndex(
    const BTHomeScanner::DeviceCache (&devices)[BTHomeScanner::MAX_DEVICES],
    uint8_t index) {
  uint8_t current = 0;
  for (const auto& device : devices) {
    if (!device.used) {
      continue;
    }
    if (current == index) {
      return &device;
    }
    current++;
  }
  return nullptr;
}

}  // namespace

void BTHomeScanner::handleScanResult(const uint8_t mac[6],
                                            const char* name,
                                            int rssi,
                                            const uint8_t* data,
                                            size_t len) {
  if (len == 0) {
    return;
  }

  auto* device = allocDevice(_devices, mac);
  if (device == nullptr) {
    return;
  }

  device->rssi = rssi;
  device->last_seen = millis();
  device->encrypted = false;
  if (name != nullptr && name[0] != 0) {
    copyName(device->name, sizeof(device->name), std::string(name));
  }

  const uint8_t info = data[0];
  if (info == kDeviceInfoEncrypted || info == kDeviceInfoTriggerEncrypted) {
    device->encrypted = true;
    return;
  }
  if (info != kDeviceInfoUnencrypted && info != kDeviceInfoTriggerUnencrypted) {
    return;
  }

  uint8_t seen_ids[BTHomeScanner::MAX_MEASUREMENT_SLOTS];
  uint8_t counts[BTHomeScanner::MAX_MEASUREMENT_SLOTS];
  uint8_t seen_count = 0;
  memset(seen_ids, 0, sizeof(seen_ids));
  memset(counts, 0, sizeof(counts));

  size_t pos = 1;
  while (pos < len) {
    const uint8_t object_id = data[pos++];
    if (object_id == kButtonObjectId) {
      if (pos >= len) {
        break;
      }

      const uint8_t event_data = data[pos++];
      const uint8_t button_index = (event_data >> 4) & 0x0F;
      const uint8_t event_type = event_data & 0x0F;
      MeasurementSlot* slot = findMeasurementSlot(*device, object_id, button_index);
      if (slot != nullptr) {
        slot->value = event_type;
        slot->seen_at = device->last_seen;
      }
      continue;
    }
    if (object_id == 0x53 || object_id == 0x54) {
      if (pos >= len) {
        break;
      }

      const uint8_t blob_len = data[pos++];
      if (pos + blob_len > len) {
        break;
      }
      pos += blob_len;
      continue;
    }

    const auto* def = findObjectDef(object_id);
    if (def == nullptr) {
      break;
    }
    if (pos + def->data_size > len) {
      break;
    }

    int idx = findOccurrenceIndex(seen_ids, counts, seen_count, object_id);
    if (idx < 0) {
      if (seen_count >= sizeof(seen_ids)) {
        break;
      }
      idx = seen_count;
      seen_ids[seen_count] = object_id;
      counts[seen_count] = 0;
      seen_count++;
    }
    const uint8_t occurrence = counts[idx]++;
    MeasurementSlot* slot = findMeasurementSlot(*device, object_id, occurrence);
    if (slot != nullptr) {
      slot->value = decodeValue(&data[pos], def->data_size, def->is_signed, def->factor);
      slot->seen_at = device->last_seen;
    }
    pos += def->data_size;
  }
}
#endif

#if !defined(ESP32_PLATFORM)
namespace {

static const BTHomeScanner::DeviceCache* findDeviceByIndex(
    const BTHomeScanner::DeviceCache (&devices)[BTHomeScanner::MAX_DEVICES],
    uint8_t index) {
  uint8_t current = 0;
  for (const auto& device : devices) {
    if (!device.used) {
      continue;
    }
    if (current == index) {
      return &device;
    }
    current++;
  }
  return nullptr;
}

}  // namespace
#endif

uint8_t BTHomeScanner::appendTelemetry(MeshCayenneLPP& telemetry,
                                       uint8_t base_channel,
                                       unsigned long freshness_ms,
                                       const uint8_t override_rain_mac[6],
                                       bool has_override_rain,
                                       float override_rain_value) const {
  uint8_t emitted = 0;
#if defined(ESP32_PLATFORM)
  uint8_t next_channel = base_channel;
  const unsigned long measurement_retention_ms = getMeasurementRetentionMs(freshness_ms);
  // The official client stops parsing when it encounters an unsupported MeshLPP type.
  // Emit the standard types it understands first across all targets, then append richer custom types after.
  for (uint8_t support_pass = 0; support_pass < 2; support_pass++) {
    const bool emit_supported_types = support_pass == 0;
    for (uint8_t target_index = 0; target_index < _target_count; target_index++) {
      if (next_channel == 0) {
        return emitted;
      }

      const DeviceCache* device = findDeviceByMac(_target_macs[target_index]);
      if (device == nullptr || device->encrypted || !isFresh(device->last_seen, freshness_ms)) {
        continue;
      }

      const uint16_t remaining_channels = (uint16_t) UINT8_MAX - next_channel + 1U;
      if (remaining_channels == 0) {
        return emitted;
      }

      MeasurementRef ordered[TELEMETRY_FIELD_COUNT];
      const uint8_t ordered_count = collectOrderedMeasurements(*device, measurement_retention_ms, ordered, TELEMETRY_FIELD_COUNT);

      uint8_t channel_types[TELEMETRY_FIELD_COUNT][TELEMETRY_FIELD_COUNT];
      uint8_t channel_type_counts[TELEMETRY_FIELD_COUNT];
      memset(channel_types, 0, sizeof(channel_types));
      memset(channel_type_counts, 0, sizeof(channel_type_counts));

      uint8_t used_channels = seedExistingChannelTypes(telemetry, next_channel, channel_types, channel_type_counts);
      const bool device_has_rain_override =
          has_override_rain && override_rain_mac != nullptr && macEquals(device->mac, override_rain_mac);
      for (uint8_t i = 0; i < ordered_count; i++) {
        const MeasurementSlot* slot = ordered[i].slot;
        const uint8_t field_type = getBTHomeFieldType(slot);
        if (slot == nullptr || field_type == 0 ||
            isOfficialClientSupportedType(field_type) != emit_supported_types) {
          continue;
        }

        bool appended = false;
        for (uint8_t channel_index = 0; channel_index < used_channels; channel_index++) {
          if (channelHasType(channel_types[channel_index], channel_type_counts[channel_index], field_type)) {
            continue;
          }
          if (!appendBTHomeField(telemetry,
                                 next_channel + channel_index,
                                 slot,
                                 device_has_rain_override && slot->object_id == 0x5F,
                                 override_rain_value)) {
            return emitted;
          }
          channel_types[channel_index][channel_type_counts[channel_index]++] = field_type;
          emitted++;
          appended = true;
          break;
        }

        if (!appended) {
          if (used_channels >= remaining_channels) {
            return emitted;
          }
          if (!appendBTHomeField(telemetry,
                                 next_channel + used_channels,
                                 slot,
                                 device_has_rain_override && slot->object_id == 0x5F,
                                 override_rain_value)) {
            return emitted;
          }
          channel_types[used_channels][channel_type_counts[used_channels]++] = field_type;
          used_channels++;
          emitted++;
        }
      }

      const uint16_t advanced_channel = (uint16_t) next_channel + used_channels;
      if (advanced_channel > UINT8_MAX) {
        return emitted;
      }
      next_channel = (uint8_t) advanced_channel;
    }
  }
#else
  (void) telemetry;
  (void) base_channel;
  (void) freshness_ms;
#endif
  return emitted;
}

size_t BTHomeScanner::formatStatus(char* dest, size_t len, unsigned long freshness_ms) const {
  uint8_t fresh = 0;
  uint8_t stale = 0;
  uint8_t missing = 0;
  uint8_t encrypted = 0;
  for (uint8_t i = 0; i < _target_count; i++) {
    const DeviceCache* device = findDeviceByMac(_target_macs[i]);
    if (device == nullptr || device->last_seen == 0) {
      missing++;
    } else if (device->encrypted) {
      encrypted++;
    } else if (isFresh(device->last_seen, freshness_ms)) {
      fresh++;
    } else {
      stale++;
    }
  }

  return snprintf(dest, len, "bthome=%s targets=%u fresh=%u stale=%u missing=%u enc=%u cache=%u",
                  _enabled ? "on" : "off", _target_count, fresh, stale, missing, encrypted, getDeviceCount());
}

size_t BTHomeScanner::formatDeviceList(char* dest, size_t len, unsigned long freshness_ms) const {
  if (len == 0) {
    return 0;
  }

#if !defined(ESP32_PLATFORM)
  (void) freshness_ms;
  return snprintf(dest, len, "No BTHome devices cached");
#else
  dest[0] = 0;
  const uint8_t count = getDeviceCount();
  if (count == 0) {
    return snprintf(dest, len, "No BTHome devices cached");
  }

  size_t used = 0;
  uint8_t index = 0;
  const unsigned long measurement_retention_ms = getMeasurementRetentionMs(freshness_ms);
  for (const auto& device : _devices) {
    if (!device.used) {
      continue;
    }

    if (used > 0 && !appendToBuffer(dest, len, used, " | ")) {
      break;
    }

    bool ok = appendToBuffer(dest, len, used, "%u", index);
    if (isTargetMac(device.mac)) {
      ok = ok && appendToBuffer(dest, len, used, "*");
    }
    if (device.encrypted) {
      ok = ok && appendToBuffer(dest, len, used, " enc");
    } else {
      float temperature = 0.0f;
      float humidity = 0.0f;
      float wind_speed = 0.0f;
      float gust = 0.0f;
      const bool is_met = getMetReportObservation(device, nullptr, 0, temperature, humidity, wind_speed, gust, freshness_ms);
      if (is_met) {
        ok = ok && appendToBuffer(dest, len, used, " met");
        const MeasurementSlot* rain_slot = findMeasurementSlot(device, 0x5F, 0, measurement_retention_ms);
        ok = ok && appendFieldSummary(dest, len, used, "rain", rain_slot);
      }
      MeasurementRef ordered[TELEMETRY_FIELD_COUNT];
      const uint8_t ordered_count = collectOrderedMeasurements(device, measurement_retention_ms, ordered, TELEMETRY_FIELD_COUNT);
      for (uint8_t i = 0; i < ordered_count; i++) {
        if (ordered[i].slot->object_id == 0x00) {
          continue;
        }
        if (is_met && ordered[i].slot->object_id == 0x5F) {
          continue;
        }
        if (is_met && (ordered[i].slot->object_id == 0x01 || ordered[i].slot->object_id == 0x0C || ordered[i].slot->object_id == 0x4A)) {
          continue;
        }
        char label[16];
        formatSlotLabel(*ordered[i].slot, label, sizeof(label));
        ok = ok && appendFieldSummary(dest, len, used, label, ordered[i].slot, ordered[i].slot->object_id == 0x40);
        if (!ok) {
          break;
        }
      }
    }
    if (!ok) {
      break;
    }

    index++;
  }

  if (index < count && used + 8 < len) {
    appendToBuffer(dest, len, used, " +%u", count - index);
  }
  return used;
#endif
}

size_t BTHomeScanner::formatDeviceFields(char* dest, size_t len, uint8_t device_index, unsigned long freshness_ms) const {
  if (len == 0) {
    return 0;
  }

#if !defined(ESP32_PLATFORM)
  (void) device_index;
  (void) freshness_ms;
  return snprintf(dest, len, "Err - unsupported");
#else
  dest[0] = 0;
  const DeviceCache* device = findDeviceByIndex(_devices, device_index);
  if (device == nullptr) {
    return snprintf(dest, len, "Err - unknown index");
  }

  char mac[18];
  formatMac(device->mac, mac, sizeof(mac));
  size_t used = 0;
  if (!appendToBuffer(dest, len, used, "%u:%s", device_index, mac)) {
    return used;
  }
  if (device->encrypted) {
    appendToBuffer(dest, len, used, " enc");
    return used;
  }

  MeasurementRef ordered[TELEMETRY_FIELD_COUNT];
  const uint8_t ordered_count = collectOrderedMeasurements(*device, getMeasurementRetentionMs(freshness_ms), ordered, TELEMETRY_FIELD_COUNT);
  if (ordered_count == 0) {
    appendToBuffer(dest, len, used, " no-fields");
    return used;
  }

  for (uint8_t i = 0; i < ordered_count; i++) {
    char label[16];
    formatSlotLabel(*ordered[i].slot, label, sizeof(label));
    if (!appendToBuffer(dest, len, used, " %u=%s", i, label)) {
      break;
    }
  }
  return used;
#endif
}

size_t BTHomeScanner::formatDeviceFieldValue(char* dest,
                                             size_t len,
                                             uint8_t device_index,
                                             uint8_t field_index,
                                             unsigned long freshness_ms) const {
  if (len == 0) {
    return 0;
  }

#if !defined(ESP32_PLATFORM)
  (void) device_index;
  (void) field_index;
  (void) freshness_ms;
  return snprintf(dest, len, "Err - unsupported");
#else
  dest[0] = 0;
  const DeviceCache* device = findDeviceByIndex(_devices, device_index);
  if (device == nullptr) {
    return snprintf(dest, len, "Err - unknown index");
  }
  if (device->encrypted) {
    return snprintf(dest, len, "Err - encrypted");
  }

  MeasurementRef ordered[TELEMETRY_FIELD_COUNT];
  const uint8_t ordered_count = collectOrderedMeasurements(*device, getMeasurementRetentionMs(freshness_ms), ordered, TELEMETRY_FIELD_COUNT);
  if (field_index >= ordered_count) {
    return snprintf(dest, len, "Err - unknown field");
  }

  char label[16];
  char value[32];
  formatSlotLabel(*ordered[field_index].slot, label, sizeof(label));
  formatSlotValue(*ordered[field_index].slot,
                  value,
                  sizeof(value),
                  ordered[field_index].slot->object_id == 0x40);
  return snprintf(dest, len, "%u:%u %s=%s", device_index, field_index, label, value);
#endif
}

void BTHomeScanner::printDevices(Print& out, unsigned long freshness_ms) const {
#if !defined(ESP32_PLATFORM)
  (void) freshness_ms;
  out.println("No BTHome devices cached");
  return;
#else
  if (getDeviceCount() == 0) {
    out.println("No BTHome devices cached");
    return;
  }

  uint8_t index = 0;
  const unsigned long measurement_retention_ms = getMeasurementRetentionMs(freshness_ms);
  for (const auto& device : _devices) {
    if (!device.used) {
      continue;
    }

    char mac[18];
    formatMac(device.mac, mac, sizeof(mac));
    out.print(index);
    out.print(":");
    out.print(mac);
    out.print(" rssi=");
    out.print(device.rssi);
    out.print(" age=");
    out.print(measurementAgeMs(device.last_seen) / 1000);
    out.print("s");
    if (isTargetMac(device.mac)) {
      out.print(" target");
    }
    if (device.encrypted) {
      out.println(" encrypted");
      index++;
      continue;
    }
    float temperature = 0.0f;
    float humidity = 0.0f;
    float wind_speed = 0.0f;
    float gust = 0.0f;
    if (getMetReportObservation(device, nullptr, 0, temperature, humidity, wind_speed, gust, freshness_ms)) {
      out.print(" met");
    }
    if (device.name[0] != 0) {
      out.print(" name=");
      out.print(device.name);
    }

#if defined(ESP32_PLATFORM)
    MeasurementRef ordered[TELEMETRY_FIELD_COUNT];
    const uint8_t ordered_count = collectOrderedMeasurements(device, measurement_retention_ms, ordered, TELEMETRY_FIELD_COUNT);
    for (uint8_t i = 0; i < ordered_count; i++) {
      char label[16];
      formatSlotLabel(*ordered[i].slot, label, sizeof(label));
      appendFieldSummary(out, label, ordered[i].slot, ordered[i].slot->object_id == 0x40);
    }
#endif
    out.println();
    index++;
  }
#endif
}

bool BTHomeScanner::isMetReportCapable(uint8_t device_index, unsigned long freshness_ms) const {
  float temperature = 0.0f;
  float humidity = 0.0f;
  float wind_speed = 0.0f;
  float gust = 0.0f;
  uint8_t mac[6];
  return getMetReportObservationByIndex(
      device_index, mac, nullptr, 0, temperature, humidity, wind_speed, gust, freshness_ms);
}

bool BTHomeScanner::getMetReportObservationByIndex(uint8_t device_index,
                                                   uint8_t mac[6],
                                                   char* label,
                                                   size_t label_len,
                                                   float& temperature,
                                                   float& humidity,
                                                   float& wind_speed,
                                                   float& gust,
                                                   unsigned long freshness_ms) const {
  const DeviceCache* device = findDeviceByIndex(_devices, device_index);
  if (device == nullptr) {
    return false;
  }
  if (mac != nullptr) {
    memcpy(mac, device->mac, 6);
  }
#if defined(ESP32_PLATFORM)
  return getMetReportObservation(*device, label, label_len, temperature, humidity, wind_speed, gust, freshness_ms);
#else
  (void) label;
  (void) label_len;
  (void) temperature;
  (void) humidity;
  (void) wind_speed;
  (void) gust;
  (void) freshness_ms;
  return false;
#endif
}

bool BTHomeScanner::getMetReportObservationByMac(const uint8_t mac[6],
                                                 char* label,
                                                 size_t label_len,
                                                 float& temperature,
                                                 float& humidity,
                                                 float& wind_speed,
                                                 float& gust,
                                                 unsigned long freshness_ms) const {
  const DeviceCache* device = findDeviceByMac(mac);
  if (device == nullptr) {
    return false;
  }
#if defined(ESP32_PLATFORM)
  return getMetReportObservation(*device, label, label_len, temperature, humidity, wind_speed, gust, freshness_ms);
#else
  (void) label;
  (void) label_len;
  (void) temperature;
  (void) humidity;
  (void) wind_speed;
  (void) gust;
  (void) freshness_ms;
  return false;
#endif
}

bool BTHomeScanner::getRainMeasurementByMac(const uint8_t mac[6],
                                            float& rain,
                                            unsigned long freshness_ms) const {
  const DeviceCache* device = findDeviceByMac(mac);
  if (device == nullptr) {
    return false;
  }
#if defined(ESP32_PLATFORM)
  return getRainMeasurement(*device, rain, freshness_ms);
#else
  (void) rain;
  (void) freshness_ms;
  return false;
#endif
}

bool BTHomeScanner::formatDeviceLabelByMac(const uint8_t mac[6], char* dest, size_t len) const {
  if (len == 0) {
    return false;
  }

  const DeviceCache* device = findDeviceByMac(mac);
#if defined(ESP32_PLATFORM)
  if (device != nullptr) {
    formatDeviceLabel(*device, dest, len);
  } else {
    formatMac(mac, dest, len);
  }
#else
  (void) device;
  formatMac(mac, dest, len);
#endif
  return true;
}

bool BTHomeScanner::parseMac(const char* text, uint8_t mac[6]) {
  char hex[13];
  int count = 0;
  for (const char* sp = text; *sp != 0 && count < 12; sp++) {
    if (isxdigit(*sp)) {
      hex[count++] = *sp;
    }
  }
  if (count != 12) {
    return false;
  }
  hex[12] = 0;

  for (int i = 0; i < 6; i++) {
    char byte_hex[3] = {hex[i * 2], hex[i * 2 + 1], 0};
    char* end = nullptr;
    long value = strtol(byte_hex, &end, 16);
    if (end == nullptr || *end != 0 || value < 0 || value > 255) {
      return false;
    }
    mac[i] = value;
  }
  return true;
}

void BTHomeScanner::formatMac(const uint8_t mac[6], char* dest, size_t len) {
  snprintf(dest, len, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
