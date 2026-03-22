#include "SensorMesh.h"
#include <helpers/sensors/LPPDataHelpers.h>
#include <ctype.h>
#include <math.h>

/* ------------------------------ Config -------------------------------- */

#ifndef LORA_FREQ
  #define LORA_FREQ   915.0
#endif
#ifndef LORA_BW
  #define LORA_BW     250
#endif
#ifndef LORA_SF
  #define LORA_SF     10
#endif
#ifndef LORA_CR
  #define LORA_CR      5
#endif
#ifndef LORA_TX_POWER
  #define LORA_TX_POWER  20
#endif

#ifndef ADVERT_NAME
  #define  ADVERT_NAME   "sensor"
#endif
#ifndef ADVERT_LAT
  #define  ADVERT_LAT  0.0
#endif
#ifndef ADVERT_LON
  #define  ADVERT_LON  0.0
#endif

#ifndef ADMIN_PASSWORD
  #define  ADMIN_PASSWORD  "password"
#endif

#ifndef SERVER_RESPONSE_DELAY
  #define SERVER_RESPONSE_DELAY   300
#endif

#ifndef TXT_ACK_DELAY
  #define TXT_ACK_DELAY     200
#endif

#ifndef SENSOR_READ_INTERVAL_SECS
  #define SENSOR_READ_INTERVAL_SECS  60
#endif

/* ------------------------------ Code -------------------------------- */

#define FIRMWARE_VER_LEVEL       1

#define REQ_TYPE_LOGIN               0x00
#define REQ_TYPE_GET_STATUS          0x01
#define REQ_TYPE_KEEP_ALIVE          0x02
#define REQ_TYPE_GET_TELEMETRY_DATA  0x03
#define REQ_TYPE_GET_AVG_MIN_MAX     0x04
#define REQ_TYPE_GET_ACCESS_LIST     0x05

#define RESP_SERVER_LOGIN_OK      0   // response to ANON_REQ

#define CLI_REPLY_DELAY_MILLIS  1000

#define LAZY_CONTACTS_WRITE_DELAY       5000

#define ALERT_ACK_EXPIRY_MILLIS         8000   // wait 8 secs for ACKs to alert messages

#define BTHOME_CONFIG_FILE "/bthome_cfg"
#define BTHOME_CONFIG_MAGIC 0x42544831UL
#define BTHOME_CONFIG_VERSION 4
#define BTHOME_MET_HISTORY_PAGE_SIZE 12

static File openAppend(FILESYSTEM* _fs, const char* fname) {
  #if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
    return _fs->open(fname, FILE_O_WRITE);
  #elif defined(RP2040_PLATFORM)
    return _fs->open(fname, "a");
  #else
    return _fs->open(fname, "a", true);
  #endif
}

static uint8_t getDataSize(uint8_t type) {
    if (isMeshCustom1ByteType(type)) {
      return 1;
    }
    if (isMeshCustom2ByteType(type)) {
      return 2;
    }
    if (isMeshCustom4ByteType(type)) {
      return 4;
    }

    switch (type) {
      case LPP_GPS:
        return 9;
      case LPP_POLYLINE:
        return 8;  // TODO: this is MINIMIUM
      case LPP_GYROMETER:
      case LPP_ACCELEROMETER:
        return 6;
      case LPP_GENERIC_SENSOR:
      case LPP_FREQUENCY:
      case LPP_DISTANCE:
      case LPP_ENERGY:
      case LPP_UNIXTIME:
        return 4;
      case LPP_COLOUR:
        return 3;
      case LPP_ANALOG_INPUT:
      case LPP_ANALOG_OUTPUT:
      case LPP_LUMINOSITY:
      case LPP_TEMPERATURE:
      case LPP_CONCENTRATION:
      case LPP_BAROMETRIC_PRESSURE:
      case LPP_RELATIVE_HUMIDITY:
      case LPP_ALTITUDE:
      case LPP_VOLTAGE:
      case LPP_CURRENT:
      case LPP_SPEED:
      case LPP_GUST:
      case LPP_DIRECTION:
      case LPP_POWER:
        return 2;
    }
    return 1;
}

static uint32_t getMultiplier(uint8_t type) {
    if (isMeshBinaryType(type) || type == LPP_BUTTON_EVENT || type == LPP_DIMMER || type == LPP_LIGHT_LEVEL) {
      return 1;
    }

    switch (type) {
      case LPP_CURRENT:
      case LPP_DISTANCE:
      case LPP_ENERGY:
        return 1000;
      case LPP_VOLTAGE:
      case LPP_ANALOG_INPUT:
      case LPP_ANALOG_OUTPUT:
      case LPP_SPEED:
      case LPP_GUST:
        return 100;
      case LPP_UV:
      case LPP_DEWPOINT:
      case LPP_RAIN:
      case LPP_ROTATION:
        return 10;
      case LPP_ACCELERATION:
      case LPP_SIGNED_SPEED:
        return 1000000;
      case LPP_GYRO_RATE:
      case LPP_VOLUME:
      case LPP_FLOW_RATE:
      case LPP_VOLUME_STORAGE:
      case LPP_WATER:
      case LPP_GAS_VOLUME:
      case LPP_MASS:
      case LPP_DURATION:
      case LPP_SIGNED_CURRENT:
        return 1000;
      case LPP_SIGNED_POWER:
        return 100;
      case LPP_PM25:
      case LPP_PM10:
      case LPP_CO2:
      case LPP_TVOC:
      case LPP_RPM:
      case LPP_CONDUCTIVITY:
        return 1;
      case LPP_TEMPERATURE:
      case LPP_BAROMETRIC_PRESSURE:
      case LPP_RELATIVE_HUMIDITY:
        return 10;
    }
    return 1;
}

static bool isSigned(uint8_t type) {
  return type == LPP_ALTITUDE || type == LPP_TEMPERATURE || type == LPP_DEWPOINT || type == LPP_GYROMETER ||
      type == LPP_ANALOG_INPUT || type == LPP_ANALOG_OUTPUT || type == LPP_GPS || type == LPP_ACCELEROMETER ||
      type == LPP_DIMMER || type == LPP_ROTATION || type == LPP_ACCELERATION || type == LPP_GYRO_RATE ||
      type == LPP_SIGNED_SPEED || type == LPP_SIGNED_POWER || type == LPP_SIGNED_CURRENT;
}

static float getFloat(const uint8_t * buffer, uint8_t size, uint32_t multiplier, bool is_signed) {
  uint32_t value = 0;
  for (uint8_t i = 0; i < size; i++) {
    value = (value << 8) + buffer[i];
  }

  int sign = 1;
  if (is_signed) {
    uint32_t bit = 1ul << ((size * 8) - 1);
    if ((value & bit) == bit) {
      value = (bit << 1) - value;
      sign = -1;
    }
  }
  return sign * ((float) value / multiplier);
}

static uint8_t putFloat(uint8_t * dest, float value, uint8_t size, uint32_t multiplier, bool is_signed) {
  // check sign
  bool sign = value < 0;
  if (sign) value = -value;

  // get value to store
  uint32_t v = value * multiplier;

  // format an uint32_t as if it was an int32_t
  if (is_signed & sign) {
    uint32_t mask = (1 << (size * 8)) - 1;
    v = v & mask;
    if (sign) v = mask - v + 1;
  }

  // add bytes (MSB first)
  for (uint8_t i=1; i<=size; i++) {
    dest[size - i] = (v & 0xFF);
    v >>= 8;
  }
  return size;
}

static const char* getBTHomeMetSlotName(uint8_t slot_mask) {
  switch (slot_mask) {
    case 1:
      return "morning";
    case 2:
      return "midday";
    case 4:
      return "evening";
    default:
      return "day";
  }
}

static uint8_t getBTHomeMetSlotForTime(uint32_t seconds_of_day) {
  if (seconds_of_day >= 18UL * 60UL * 60UL) {
    return 4;
  }
  if (seconds_of_day >= 12UL * 60UL * 60UL) {
    return 2;
  }
  if (seconds_of_day >= 6UL * 60UL * 60UL) {
    return 1;
  }
  return 0;
}

static uint8_t parseBTHomeMetPublishMask(const char* text) {
  if (strcmp(text, "off") == 0) {
    return 0;
  }
  if (strcmp(text, "morning") == 0) {
    return 1;
  }
  if (strcmp(text, "midday") == 0) {
    return 2;
  }
  if (strcmp(text, "evening") == 0) {
    return 4;
  }
  if (strcmp(text, "all") == 0) {
    return 1 | 2 | 4;
  }
  return 0xFF;
}

static void formatBTHomeMetPublishMask(uint8_t mask, char* dest, size_t len) {
  if (len == 0) {
    return;
  }
  if (mask == 0) {
    strncpy(dest, "off", len - 1);
    dest[len - 1] = 0;
    return;
  }
  if (mask == (1 | 2 | 4)) {
    strncpy(dest, "all", len - 1);
    dest[len - 1] = 0;
    return;
  }
  strncpy(dest, getBTHomeMetSlotName(mask), len - 1);
  dest[len - 1] = 0;
}

static int decodeBase64Secret(const char* src, uint8_t* dest, size_t dest_len) {
  uint32_t buffer = 0;
  int bits = 0;
  size_t out_len = 0;

  while (*src != 0) {
    char c = *src++;
    if (isspace((unsigned char) c)) {
      continue;
    }
    if (c == '=') {
      break;
    }

    int value;
    if (c >= 'A' && c <= 'Z') {
      value = c - 'A';
    } else if (c >= 'a' && c <= 'z') {
      value = c - 'a' + 26;
    } else if (c >= '0' && c <= '9') {
      value = c - '0' + 52;
    } else if (c == '+') {
      value = 62;
    } else if (c == '/') {
      value = 63;
    } else {
      return -1;
    }

    buffer = (buffer << 6) | (uint32_t) value;
    bits += 6;
    while (bits >= 8) {
      bits -= 8;
      if (out_len >= dest_len) {
        return -1;
      }
      dest[out_len++] = (uint8_t) ((buffer >> bits) & 0xFF);
    }
  }

  return (int) out_len;
}

void SensorMesh::buildTelemetry(uint8_t requester_permissions) {
  telemetry.reset();
  telemetry.addVoltage(TELEM_CHANNEL_SELF, (float) board.getBattMilliVolts() / 1000.0f);
  sensors.querySensors(requester_permissions, telemetry);

  bool has_rain_override = false;
  float rain_override = 0.0f;
  if (_met_report.hasTarget()) {
    uint32_t now = getRTCClock()->getCurrentTime();
    uint32_t seconds_of_day = now % 86400UL;
    if (seconds_of_day > 0) {
      float rain_first = 0.0f;
      float rain_last = 0.0f;
      if (_met_report.rain_history.calcFirstLast(getRTCClock(), seconds_of_day, 0, rain_first, rain_last) &&
          rain_last + 0.01f >= rain_first) {
        has_rain_override = true;
        rain_override = max(0.0f, rain_last - rain_first);
      }
    }
  }

  _bthome.appendTelemetry(telemetry,
                          getNextTelemetryChannel(),
                          BTHomeScanner::DEFAULT_FRESHNESS_MS,
                          has_rain_override ? _met_report.target_mac : nullptr,
                          has_rain_override,
                          rain_override);
}

uint8_t SensorMesh::getNextTelemetryChannel() {
  LPPReader reader(telemetry.getBuffer(), telemetry.getSize());
  uint8_t channel = 0;
  uint8_t type = 0;
  uint8_t max_channel = 0;
  while (reader.readHeader(channel, type)) {
    if (channel > max_channel) {
      max_channel = channel;
    }
    reader.skipData(type);
  }
  return max_channel + 1;
}

bool SensorMesh::configureBTHomeMetTarget(uint8_t device_index) {
  uint8_t mac[6];
  float temperature = 0.0f;
  float humidity = 0.0f;
  float wind_speed = 0.0f;
  float gust = 0.0f;

  if (!_bthome.getMetReportObservationByIndex(
          device_index,
          mac,
          nullptr,
          0,
          temperature,
          humidity,
          wind_speed,
          gust,
          BTHomeScanner::DEFAULT_FRESHNESS_MS)) {
    return false;
  }

  memcpy(_met_report.target_mac, mac, sizeof(mac));
  _met_report.target_configured = 1;
  _met_report.clearHistory();
  recordBTHomeMetHistory();
  return true;
}

bool SensorMesh::configureBTHomeMetChannel(const char* psk_base64) {
  mesh::GroupChannel channel;
  memset(&channel, 0, sizeof(channel));

  int secret_len = decodeBase64Secret(psk_base64, channel.secret, sizeof(channel.secret));
  if (secret_len != 16 && secret_len != 32) {
    return false;
  }

  mesh::Utils::sha256(channel.hash, sizeof(channel.hash), channel.secret, secret_len);
  _met_report.channel = channel;
  _met_report.channel_secret_len = (uint8_t) secret_len;
  return true;
}

void SensorMesh::recordBTHomeMetHistory() {
  if (!_met_report.hasTarget()) {
    return;
  }

  float temperature = 0.0f;
  float humidity = 0.0f;
  float wind_speed = 0.0f;
  float gust = 0.0f;
  if (!_bthome.getMetReportObservationByMac(
          _met_report.target_mac,
          nullptr,
          0,
          temperature,
          humidity,
          wind_speed,
          gust,
          BTHomeScanner::DEFAULT_FRESHNESS_MS)) {
    return;
  }

  _met_report.temperature_history.recordData(getRTCClock(), temperature);
  _met_report.humidity_history.recordData(getRTCClock(), humidity);
  _met_report.wind_speed_history.recordData(getRTCClock(), wind_speed);
  _met_report.gust_history.recordData(getRTCClock(), gust);

  float rain = 0.0f;
  if (_bthome.getRainMeasurementByMac(
          _met_report.target_mac,
          rain,
          BTHomeScanner::DEFAULT_FRESHNESS_MS)) {
    _met_report.rain_history.recordData(getRTCClock(), rain);
  }
}

bool SensorMesh::buildBTHomeMetReport(char* dest, size_t len, const char* slot_name) const {
  if (len == 0 || !_met_report.hasTarget()) {
    return false;
  }
  (void) slot_name;

  float temperature = 0.0f;
  float humidity = 0.0f;
  float wind_speed = 0.0f;
  float gust = 0.0f;
  if (!_bthome.getMetReportObservationByMac(
          _met_report.target_mac,
          nullptr,
          0,
          temperature,
          humidity,
          wind_speed,
          gust,
          BTHomeScanner::DEFAULT_FRESHNESS_MS)) {
    return false;
  }

  uint32_t now = getRTCClock()->getCurrentTime();
  uint32_t seconds_of_day = now % 86400UL;
  uint32_t start_secs_ago = max(seconds_of_day, BTHomeMetReportState::HISTORY_INTERVAL_SECS);

  MinMaxAvg temperature_stats;
  MinMaxAvg gust_stats;
  _met_report.temperature_history.calcMinMaxAvg(getRTCClock(), start_secs_ago, 0, &temperature_stats, 0, LPP_TEMPERATURE);
  _met_report.gust_history.calcMinMaxAvg(getRTCClock(), start_secs_ago, 0, &gust_stats, 0, LPP_GUST);

  if (isnan(temperature_stats._avg) || isnan(gust_stats._avg)) {
    return false;
  }

  char rain_sentence[16];
  rain_sentence[0] = 0;
  if (seconds_of_day > 0) {
    float rain_first = 0.0f;
    float rain_last = 0.0f;
    if (_met_report.rain_history.calcFirstLast(getRTCClock(), seconds_of_day, 0, rain_first, rain_last) &&
        rain_last + 0.01f >= rain_first) {
      snprintf(rain_sentence, sizeof(rain_sentence), " \xF0\x9F\x8C\xA7%.1fmm", max(0.0f, rain_last - rain_first));
    }
  }

  int written = snprintf(dest,
                         len,
                         "\xF0\x9F\x8C\xA1%.1fC \xF0\x9F\x92\xA7%.0f%% \xF0\x9F\x8C\x80%.1f\xE2\x86\x92%.1fm/s "
                         "\xE2\xAC\x87%.1fC \xE2\xAC\x86%.1fC%s",
                         temperature,
                         humidity,
                         wind_speed,
                         gust_stats._max,
                         temperature_stats._min,
                         temperature_stats._max,
                         rain_sentence);
  return written > 0 && (size_t) written < len;
}

bool SensorMesh::formatBTHomeMetHistory(char* dest, size_t len, uint8_t measurement_id, uint8_t page) const {
  if (len == 0 || !_met_report.hasTarget()) {
    return false;
  }

  uint8_t precision = 0;
  const TimeSeriesData* history = nullptr;
  switch (measurement_id) {
    case 1:
      precision = 1;
      history = &_met_report.temperature_history;
      break;
    case 2:
      precision = 0;
      history = &_met_report.humidity_history;
      break;
    case 3:
      precision = 1;
      history = &_met_report.wind_speed_history;
      break;
    case 4:
      precision = 1;
      history = &_met_report.gust_history;
      break;
    case 5:
      precision = 1;
      history = &_met_report.rain_history;
      break;
    default:
      return false;
  }

  float values[BTHomeMetReportState::HISTORY_SLOTS];
  int total = history->copyChronological(values, BTHomeMetReportState::HISTORY_SLOTS);
  if (total <= 0) {
    return false;
  }

  int newest_end = total - ((int) page * BTHOME_MET_HISTORY_PAGE_SIZE);
  if (newest_end <= 0) {
    return false;
  }

  int start = max(0, newest_end - BTHOME_MET_HISTORY_PAGE_SIZE);
  int count = newest_end - start;
  int written = snprintf(dest, len, "%u,%u,%d", measurement_id, page, count);
  if (written <= 0 || (size_t) written >= len) {
    return false;
  }

  for (int i = start; i < newest_end; i++) {
    int part = snprintf(dest + written,
                        len - (size_t) written,
                        precision == 0 ? ",%.0f" : ",%.1f",
                        values[i]);
    if (part <= 0 || (size_t) (written + part) >= len) {
      return false;
    }
    written += part;
  }

  return true;
}

bool SensorMesh::publishBTHomeMetReport(const char* slot_name) {
  if (!_met_report.hasChannel()) {
    return false;
  }

  recordBTHomeMetHistory();

  char report[128];
  if (!buildBTHomeMetReport(report, sizeof(report), slot_name)) {
    return false;
  }

  char text[160];
  int written = snprintf(text, sizeof(text), "%s: %s", _prefs.node_name, report);
  if (written <= 0) {
    return false;
  }

  uint32_t timestamp = getRTCClock()->getCurrentTimeUnique();
  uint8_t payload[5 + sizeof(text)];
  memcpy(payload, &timestamp, 4);
  payload[4] = (TXT_TYPE_PLAIN << 2);

  size_t text_len = min((size_t) strlen(text), sizeof(text) - 1);
  memcpy(&payload[5], text, text_len);

  mesh::Packet* packet = createGroupDatagram(PAYLOAD_TYPE_GRP_TXT, _met_report.channel, payload, 5 + text_len);
  if (packet == nullptr) {
    return false;
  }

  sendFlood(packet);
  return true;
}

void SensorMesh::maybePublishBTHomeMetReport() {
  if (!_met_report.hasTarget() || !_met_report.hasChannel() || _met_report.publish_mask == 0) {
    return;
  }

  uint32_t now = getRTCClock()->getCurrentTime();
  uint32_t day = now / 86400UL;
  uint32_t seconds_of_day = now % 86400UL;
  if (_met_report.last_publish_day != day) {
    _met_report.last_publish_day = day;
    _met_report.last_publish_mask = 0;
  }

  uint8_t current_slot = getBTHomeMetSlotForTime(seconds_of_day);
  if ((current_slot & _met_report.publish_mask) == 0) {
    return;
  }
  if ((_met_report.last_publish_mask & current_slot) != 0) {
    return;
  }

  if (publishBTHomeMetReport(getBTHomeMetSlotName(current_slot))) {
    _met_report.last_publish_mask |= current_slot;
  }
}

size_t SensorMesh::formatBTHomeMetStatus(char* dest, size_t len) const {
  if (len == 0) {
    return 0;
  }

  char target[24];
  if (_met_report.hasTarget()) {
    _bthome.formatDeviceLabelByMac(_met_report.target_mac, target, sizeof(target));
  } else {
    strcpy(target, "off");
  }

  char schedule[16];
  formatBTHomeMetPublishMask(_met_report.publish_mask, schedule, sizeof(schedule));

  char channel_hash[PATH_HASH_SIZE * 2 + 1];
  if (_met_report.hasChannel()) {
    mesh::Utils::toHex(channel_hash, _met_report.channel.hash, sizeof(_met_report.channel.hash));
  } else {
    strcpy(channel_hash, "off");
  }

  float temperature = 0.0f;
  float humidity = 0.0f;
  float wind_speed = 0.0f;
  float gust = 0.0f;
  bool ready = _met_report.hasTarget() &&
      _bthome.getMetReportObservationByMac(
          _met_report.target_mac,
          nullptr,
          0,
          temperature,
          humidity,
          wind_speed,
          gust,
          BTHomeScanner::DEFAULT_FRESHNESS_MS);

  return snprintf(dest,
                  len,
                  "met target=%s ready=%s sched=%s chan=%s",
                  target,
                  ready ? "yes" : "no",
                  schedule,
                  channel_hash);
}

bool SensorMesh::handleBTHomeMetCommand(char* command, char* reply) {
  if (strcmp(command, "bthome met status") == 0) {
    formatBTHomeMetStatus(reply, 160);
  } else if (strcmp(command, "bthome met today") == 0 || strcmp(command, "bthome met now") == 0) {
    recordBTHomeMetHistory();
    if (!buildBTHomeMetReport(reply, 160, nullptr)) {
      strcpy(reply, "Err - no met history");
    }
  } else if (strcmp(command, "bthome met publish") == 0) {
    if (!_met_report.hasChannel()) {
      strcpy(reply, "Err - no channel");
    } else if (publishBTHomeMetReport(nullptr)) {
      strcpy(reply, "ok");
    } else {
      strcpy(reply, "Err - no met report");
    }
  } else if (strcmp(command, "bthome met clear") == 0) {
    _met_report.clearTarget();
    _met_report.publish_mask = 0;
    _met_report.clearChannel();
    saveBTHomeConfig();
    strcpy(reply, "ok");
  } else if (memcmp(command, "bthome met set ", 15) == 0) {
    char* end = nullptr;
    long index = strtol(&command[15], &end, 10);
    if (end == &command[15] || *end != 0 || index < 0 || index > 255) {
      strcpy(reply, "Err - bad index");
    } else if (!configureBTHomeMetTarget((uint8_t) index)) {
      strcpy(reply, "Err - not met");
    } else {
      saveBTHomeConfig();
      strcpy(reply, "ok");
    }
  } else if (memcmp(command, "bthome met channel ", 19) == 0) {
    const char* arg = &command[19];
    if (strcmp(arg, "off") == 0) {
      _met_report.clearChannel();
      saveBTHomeConfig();
      strcpy(reply, "ok");
    } else if (!configureBTHomeMetChannel(arg)) {
      strcpy(reply, "Err - bad psk");
    } else {
      saveBTHomeConfig();
      char hash_hex[PATH_HASH_SIZE * 2 + 1];
      mesh::Utils::toHex(hash_hex, _met_report.channel.hash, sizeof(_met_report.channel.hash));
      snprintf(reply, 160, "ok hash=%s", hash_hex);
    }
  } else if (memcmp(command, "bthome met history ", 19) == 0) {
    char* end = nullptr;
    long measurement_id = strtol(&command[19], &end, 10);
    long page = 0;
    if (end == &command[19] || measurement_id < 1 || measurement_id > 5) {
      strcpy(reply, "Err - bad measurement_id");
    } else {
      while (*end == ' ') {
        end++;
      }
      if (*end != 0) {
        char* page_end = nullptr;
        page = strtol(end, &page_end, 10);
        if (page_end == end || *page_end != 0 || page < 0 || page > 255) {
          strcpy(reply, "Err - bad page");
          return true;
        }
      }

      recordBTHomeMetHistory();
      if (!formatBTHomeMetHistory(reply, 160, (uint8_t) measurement_id, (uint8_t) page)) {
        strcpy(reply, "Err - no met history");
      }
    }
  } else if (memcmp(command, "bthome met schedule ", 20) == 0) {
    uint8_t mask = parseBTHomeMetPublishMask(&command[20]);
    if (mask == 0xFF) {
      strcpy(reply, "Err - bad schedule");
    } else {
      _met_report.publish_mask = mask;
      _met_report.last_publish_day = 0;
      _met_report.last_publish_mask = 0;
      saveBTHomeConfig();
      formatBTHomeMetStatus(reply, 160);
    }
  } else {
    strcpy(reply, "bthome met status|set <index>|today|history <measurement_id> [page]|publish|clear|channel <psk|off>|schedule <off|morning|midday|evening|all>");
  }
  return true;
}

uint8_t SensorMesh::handleRequest(uint8_t perms, uint32_t sender_timestamp, uint8_t req_type, uint8_t* payload, size_t payload_len) {
  memcpy(reply_data, &sender_timestamp, 4);   // reflect sender_timestamp back in response packet (kind of like a 'tag')

  if (req_type == REQ_TYPE_GET_TELEMETRY_DATA) {  // allow all
    uint8_t perm_mask = ~(payload[0]);    // NEW: first reserved byte (of 4), is now inverse mask to apply to permissions

    buildTelemetry(0xFF & perm_mask);  // allow all telemetry permissions for admin or guest
    // TODO: let requester know permissions they have:  telemetry.addPresence(TELEM_CHANNEL_SELF, perms);

    uint8_t tlen = telemetry.getSize();
    memcpy(&reply_data[4], telemetry.getBuffer(), tlen);
    return 4 + tlen;  // reply_len
  }
  if (req_type == REQ_TYPE_GET_AVG_MIN_MAX && (perms & PERM_ACL_ROLE_MASK) >= PERM_ACL_READ_ONLY) {
    uint32_t start_secs_ago, end_secs_ago;
    memcpy(&start_secs_ago, &payload[0], 4);
    memcpy(&end_secs_ago, &payload[4], 4);
    uint8_t res1 = payload[8];   // reserved for future  (extra query params)
    uint8_t res2 = payload[9];

    MinMaxAvg data[8];
    int n;
    if (res1 == 0 && res2 == 0) {
      n = querySeriesData(start_secs_ago, end_secs_ago, data, 8);
    } else {
      n = 0;
    }

    uint8_t ofs = 4;
    {
      uint32_t now = getRTCClock()->getCurrentTime();
      memcpy(&reply_data[ofs], &now, 4); ofs += 4;
    }

    for (int i = 0; i < n; i++) {
      auto d = &data[i];
      reply_data[ofs++] = d->_channel;
      reply_data[ofs++] = d->_lpp_type;
      uint8_t sz = getDataSize(d->_lpp_type);
      uint32_t mult = getMultiplier(d->_lpp_type);
      bool is_signed = isSigned(d->_lpp_type);
      ofs += putFloat(&reply_data[ofs], d->_min, sz, mult, is_signed);
      ofs += putFloat(&reply_data[ofs], d->_max, sz, mult, is_signed);
      ofs += putFloat(&reply_data[ofs], d->_avg, sz, mult, is_signed);
    }
    return ofs;
  }
  if (req_type == REQ_TYPE_GET_ACCESS_LIST && (perms & PERM_ACL_ROLE_MASK) == PERM_ACL_ADMIN) {
    uint8_t res1 = payload[0];   // reserved for future  (extra query params)
    uint8_t res2 = payload[1];
    if (res1 == 0 && res2 == 0) {
      uint8_t ofs = 4;
      for (int i = 0; i < acl.getNumClients() && ofs + 7 <= sizeof(reply_data) - 4; i++) {
        auto c = acl.getClientByIdx(i);
        if (c->permissions == 0) continue;  // skip deleted entries
        memcpy(&reply_data[ofs], c->id.pub_key, 6); ofs += 6;  // just 6-byte pub_key prefix
        reply_data[ofs++] = c->permissions;
      }
      return ofs;
    }
  }
  return 0;  // unknown command
}

mesh::Packet* SensorMesh::createSelfAdvert() {
  uint8_t app_data[MAX_ADVERT_DATA_SIZE];
  uint8_t app_data_len = _cli.buildAdvertData(ADV_TYPE_SENSOR, app_data);

  return createAdvert(self_id, app_data, app_data_len);
}

void SensorMesh::sendAlert(const ClientInfo* c, Trigger* t) {
  int text_len = strlen(t->text);

  uint8_t data[MAX_PACKET_PAYLOAD];
  memcpy(data, &t->timestamp, 4);
  data[4] = (TXT_TYPE_PLAIN << 2) | t->attempt;  // attempt and flags
  memcpy(&data[5], t->text, text_len);

  // calc expected ACK reply
  mesh::Utils::sha256((uint8_t *)&t->expected_acks[t->attempt], 4, data, 5 + text_len, self_id.pub_key, PUB_KEY_SIZE);
  t->attempt++;

  auto pkt = createDatagram(PAYLOAD_TYPE_TXT_MSG, c->id, c->shared_secret, data, 5 + text_len);
  if (pkt) {
    if (c->out_path_len != OUT_PATH_UNKNOWN) {  // we have an out_path, so send DIRECT
      sendDirect(pkt, c->out_path, c->out_path_len);
    } else {
      unsigned long delay_millis = 0;
      sendFlood(pkt, delay_millis, _prefs.path_hash_mode + 1);
    }
  }
  t->send_expiry = futureMillis(ALERT_ACK_EXPIRY_MILLIS);
}

void SensorMesh::alertIf(bool condition, Trigger& t, AlertPriority pri, const char* text) {
  if (condition) {
    if (!t.isTriggered() && num_alert_tasks < MAX_CONCURRENT_ALERTS) {
      StrHelper::strncpy(t.text, text, sizeof(t.text));
      t.pri = pri;
      t.send_expiry = 0;  // signal that initial send is needed
      t.attempt = 4;
      t.curr_contact_idx = -1;  // start iterating thru contacts[]

      alert_tasks[num_alert_tasks++] = &t;  // add to queue
    }
  } else {
    if (t.isTriggered()) {
      t.text[0] = 0;
      // remove 't' from alert queue
      int i = 0;
      while (i < num_alert_tasks && alert_tasks[i] != &t) i++;

      if (i < num_alert_tasks) {  // found,  now delete from array
        num_alert_tasks--;
        while (i < num_alert_tasks) {
          alert_tasks[i] = alert_tasks[i + 1];
          i++;
        }
      }
    }
  }
}

float SensorMesh::getAirtimeBudgetFactor() const {
  return _prefs.airtime_factor;
}

bool SensorMesh::allowPacketForward(const mesh::Packet* packet) {
  if (_prefs.disable_fwd) return false;
  if (packet->isRouteFlood() && packet->getPathHashCount() >= _prefs.flood_max) return false;
  return true;
}

int SensorMesh::calcRxDelay(float score, uint32_t air_time) const {
  if (_prefs.rx_delay_base <= 0.0f) return 0;
  return (int) ((pow(_prefs.rx_delay_base, 0.85f - score) - 1.0) * air_time);
}

uint32_t SensorMesh::getRetransmitDelay(const mesh::Packet* packet) {
  uint32_t t = (_radio->getEstAirtimeFor(packet->getPathByteLen() + packet->payload_len + 2) * _prefs.tx_delay_factor);
  return getRNG()->nextInt(0, 6)*t;
}
uint32_t SensorMesh::getDirectRetransmitDelay(const mesh::Packet* packet) {
  uint32_t t = (_radio->getEstAirtimeFor(packet->getPathByteLen() + packet->payload_len + 2) * _prefs.direct_tx_delay_factor);
  return getRNG()->nextInt(0, 6)*t;
}
int SensorMesh::getInterferenceThreshold() const {
  return _prefs.interference_threshold;
}
int SensorMesh::getAGCResetInterval() const {
  return ((int)_prefs.agc_reset_interval) * 4000;   // milliseconds
}

uint8_t SensorMesh::handleLoginReq(const mesh::Identity& sender, const uint8_t* secret, uint32_t sender_timestamp, const uint8_t* data, bool is_flood) {
  ClientInfo* client = NULL;
  if (data[0] == 0) {   // blank password, just check if sender is in ACL
    client = acl.getClient(sender.pub_key, PUB_KEY_SIZE);
    if (client == NULL) {
    #if MESH_DEBUG
      MESH_DEBUG_PRINTLN("Login, sender not in ACL");
    #endif
    }
  }
  if (client == NULL) {
    uint8_t perms;
    uint8_t init_perms = 0;
    if (strcmp((char *)data, _prefs.password) == 0) {  // check for valid admin password
      perms = PERM_ACL_ADMIN;
      init_perms = PERM_RECV_ALERTS_HI | PERM_RECV_ALERTS_LO;
    } else if (strcmp((char *)data, _prefs.guest_password) == 0) {  // check guest password
      perms = PERM_ACL_READ_ONLY;  // sensor guests get read-only access (telemetry + history)
    } else {
    #if MESH_DEBUG
      MESH_DEBUG_PRINTLN("Invalid password: %s", data);
    #endif
      return 0;
    }

    client = acl.putClient(sender, init_perms);  // add to contacts (if not already known)
    if (sender_timestamp <= client->last_timestamp) {
      MESH_DEBUG_PRINTLN("Possible login replay attack!");
      return 0;  // FATAL: client table is full -OR- replay attack
    }

    MESH_DEBUG_PRINTLN("Login success!");
    client->last_timestamp = sender_timestamp;
    client->last_activity = getRTCClock()->getCurrentTime();
    client->permissions &= ~PERM_ACL_ROLE_MASK;
    client->permissions |= perms;
    memcpy(client->shared_secret, secret, PUB_KEY_SIZE);

    if (perms == PERM_ACL_ADMIN) {   // keep number of FS writes to a minimum (only persist admin contacts)
      dirty_contacts_expiry = futureMillis(LAZY_CONTACTS_WRITE_DELAY);
    }
  }

  if (is_flood) {
    client->out_path_len = OUT_PATH_UNKNOWN;  // need to rediscover out_path
  }

  uint32_t now = getRTCClock()->getCurrentTimeUnique();
  memcpy(reply_data, &now, 4);   // response packets always prefixed with timestamp
  reply_data[4] = RESP_SERVER_LOGIN_OK;
  reply_data[5] = 0;
  reply_data[6] = client->isAdmin() ? 1 : 0;
  reply_data[7] = client->permissions;
  getRNG()->random(&reply_data[8], 4);   // random blob to help packet-hash uniqueness
  reply_data[12] = FIRMWARE_VER_LEVEL;

  return 13;  // reply length
}

void SensorMesh::handleCommand(uint32_t sender_timestamp, char* command, char* reply) {
  while (*command == ' ') command++;   // skip leading spaces

  if (strlen(command) > 4 && command[2] == '|') {  // optional prefix (for companion radio CLI)
    memcpy(reply, command, 3);  // reflect the prefix back
    reply += 3;
    command += 3;
  }

  // first, see if this is a custom-handled CLI command (ie. in main.cpp)
  if (handleCustomCommand(sender_timestamp, command, reply)) {
    return;   // command has been handled
  }

  if (handleBTHomeCommand(sender_timestamp, command, reply)) {
    return;
  }

  // handle sensor-specific CLI commands
  if (memcmp(command, "setperm ", 8) == 0) {   // format:  setperm {pubkey-hex} {permissions-int8}
    char* hex = &command[8];
    char* sp = strchr(hex, ' ');   // look for separator char
    if (sp == NULL) {
      strcpy(reply, "Err - bad params");
    } else {
      *sp++ = 0;   // replace space with null terminator

      uint8_t pubkey[PUB_KEY_SIZE];
      int hex_len = min(sp - hex, PUB_KEY_SIZE*2);
      if (mesh::Utils::fromHex(pubkey, hex_len / 2, hex)) {
        uint8_t perms = atoi(sp);
        if (acl.applyPermissions(self_id, pubkey, hex_len / 2, perms)) {
          dirty_contacts_expiry = futureMillis(LAZY_CONTACTS_WRITE_DELAY);   // trigger acl.save()
          strcpy(reply, "OK");
        } else {
          strcpy(reply, "Err - invalid params");
        }
      } else {
        strcpy(reply, "Err - bad pubkey");
      }
    }
  } else if (sender_timestamp == 0 && strcmp(command, "get acl") == 0) {
    Serial.println("ACL:");
    for (int i = 0; i < acl.getNumClients(); i++) {
      auto c = acl.getClientByIdx(i);
      if (c->permissions == 0) continue;  // skip deleted entries

      Serial.printf("%02X ", c->permissions);
      mesh::Utils::printHex(Serial, c->id.pub_key, PUB_KEY_SIZE);
      Serial.printf("\n");
    }
    reply[0] = 0;
  } else if (memcmp(command, "io ", 2) == 0) { // io {value}: write, io: read 
    if (command[2] == ' ') { // it's a write
      uint32_t val;
      uint32_t g = board.getGpio();
      if (command[3] == 'r') { // reset bits
        sscanf(&command[4], "%x", &val);
        val = g & ~val;    
      } else if (command[3] == 's') { // set bits
        sscanf(&command[4], "%x", &val);
        val |= g;    
      } else if (command[3] == 't') { // toggle bits
        sscanf(&command[4], "%x", &val);
        val ^= g;    
      } else { // set value
        sscanf(&command[3], "%x", &val);
      }
      board.setGpio(val);
    }
    sprintf(reply, "%x", board.getGpio());
  } else{
    _cli.handleCommand(sender_timestamp, command, reply);  // common CLI commands
  }
}

bool SensorMesh::handleBTHomeCommand(uint32_t sender_timestamp, char* command, char* reply) {
  if (memcmp(command, "bthome", 6) != 0 || (command[6] != 0 && command[6] != ' ')) {
    return false;
  }

  if (strcmp(command, "bthome on") == 0) {
    _bthome.setEnabled(true);
    saveBTHomeConfig();
    strcpy(reply, "ok");
  } else if (strcmp(command, "bthome off") == 0) {
    _bthome.setEnabled(false);
    saveBTHomeConfig();
    strcpy(reply, "ok");
  } else if (strcmp(command, "bthome status") == 0) {
    _bthome.formatStatus(reply, 160, BTHomeScanner::DEFAULT_FRESHNESS_MS);
  } else if (strcmp(command, "bthome list") == 0) {
    _bthome.formatDeviceList(reply, 160, BTHomeScanner::DEFAULT_FRESHNESS_MS);
    if (sender_timestamp == 0) {
      _bthome.printDevices(Serial, BTHomeScanner::DEFAULT_FRESHNESS_MS);
    }
  } else if (memcmp(command, "bthome met", 10) == 0 && (command[10] == 0 || command[10] == ' ')) {
    return handleBTHomeMetCommand(command, reply);
  } else if (memcmp(command, "bthome get ", 11) == 0) {
    char* end = nullptr;
    long device_index = strtol(&command[11], &end, 10);
    if (end == &command[11] || *end != ' ') {
      strcpy(reply, "Err - bad index");
    } else {
      long field_index = strtol(end + 1, &end, 10);
      if (*end != 0 || device_index < 0 || device_index > 255 || field_index < 0 || field_index > 255) {
        strcpy(reply, "Err - bad index");
      } else {
        _bthome.formatDeviceFieldValue(reply,
                                       160,
                                       (uint8_t) device_index,
                                       (uint8_t) field_index,
                                       BTHomeScanner::DEFAULT_FRESHNESS_MS);
      }
    }
  } else if (memcmp(command, "bthome ", 7) == 0 && isdigit((unsigned char) command[7])) {
    char* end = nullptr;
    long device_index = strtol(&command[7], &end, 10);
    if (*end != 0 || device_index < 0 || device_index > 255) {
      strcpy(reply, "Err - bad index");
    } else {
      _bthome.formatDeviceFields(reply, 160, (uint8_t) device_index, BTHomeScanner::DEFAULT_FRESHNESS_MS);
    }
  } else if (memcmp(command, "bthome add ", 11) == 0) {
    const char* index_text = &command[11];
    char* end = nullptr;
    long index = strtol(index_text, &end, 10);
    uint8_t mac[6];
    if (end == index_text || *end != 0 || index < 0 || index > 255) {
      strcpy(reply, "Err - bad index");
    } else if (!_bthome.getDeviceMacByIndex((uint8_t) index, mac)) {
      strcpy(reply, "Err - unknown index");
    } else if (_bthome.isKnownEncrypted(mac)) {
      strcpy(reply, "Err - encrypted");
    } else if (!_bthome.addTargetMac(mac)) {
      strcpy(reply, "Err - target full");
    } else {
      saveBTHomeConfig();
      strcpy(reply, "ok");
    }
  } else if (memcmp(command, "bthome rm ", 10) == 0) {
    char* end = nullptr;
    long index = strtol(&command[10], &end, 10);
    uint8_t mac[6];
    if (end == &command[10] || *end != 0 || index < 0 || index > 255) {
      strcpy(reply, "Err - bad index");
    } else if (!_bthome.getDeviceMacByIndex((uint8_t) index, mac)) {
      strcpy(reply, "Err - unknown index");
    } else if (!_bthome.removeTargetMac(mac)) {
      strcpy(reply, "Err - not targeted");
    } else {
      saveBTHomeConfig();
      strcpy(reply, "ok");
    }
  } else {
    strcpy(reply, "bthome on|off|status|list|<index>|get <index> <field>|add <index>|rm <index>|met ...");
  }
  return true;
}

void SensorMesh::onAnonDataRecv(mesh::Packet* packet, const uint8_t* secret, const mesh::Identity& sender, uint8_t* data, size_t len) {
  if (packet->getPayloadType() == PAYLOAD_TYPE_ANON_REQ) {  // received an initial request by a possible admin client (unknown at this stage)
    uint32_t timestamp;
    memcpy(&timestamp, data, 4);

    data[len] = 0;  // ensure null terminator
    uint8_t reply_len;
    if (data[4] == 0 || data[4] >= ' ') {   // is password, ie. a login request
      reply_len = handleLoginReq(sender, secret, timestamp, &data[4], packet->isRouteFlood());
    } else if (data[4] == REQ_TYPE_GET_TELEMETRY_DATA && len >= 9) {  // anonymous telemetry request (guest access)
      reply_len = handleRequest(PERM_ACL_GUEST, timestamp, data[4], &data[5], len - 5);
    } else {
      reply_len = 0;  // unknown request type
    }

    if (reply_len == 0) return;   // invalid request

    if (packet->isRouteFlood()) {
      // let this sender know path TO here, so they can use sendDirect(), and ALSO encode the response
      mesh::Packet* path = createPathReturn(sender, secret, packet->path, packet->path_len,
                                            PAYLOAD_TYPE_RESPONSE, reply_data, reply_len);
      if (path) sendFlood(path, SERVER_RESPONSE_DELAY, packet->getPathHashSize());
    } else {
      mesh::Packet* reply = createDatagram(PAYLOAD_TYPE_RESPONSE, sender, secret, reply_data, reply_len);
      if (reply) sendFlood(reply, SERVER_RESPONSE_DELAY, packet->getPathHashSize());
    }
  }
}

int SensorMesh::searchPeersByHash(const uint8_t* hash) {
  int n = 0;
  for (int i = 0; i < acl.getNumClients() && n < MAX_SEARCH_RESULTS; i++) {
    if (acl.getClientByIdx(i)->id.isHashMatch(hash)) {
      matching_peer_indexes[n++] = i;  // store the INDEXES of matching contacts (for subsequent 'peer' methods)
    }
  }
  return n;
}

void SensorMesh::getPeerSharedSecret(uint8_t* dest_secret, int peer_idx) {
  int i = matching_peer_indexes[peer_idx];
  if (i >= 0 && i < acl.getNumClients()) {
    // lookup pre-calculated shared_secret
    memcpy(dest_secret, acl.getClientByIdx(i)->shared_secret, PUB_KEY_SIZE);
  } else {
    MESH_DEBUG_PRINTLN("getPeerSharedSecret: Invalid peer idx: %d", i);
  }
}

void SensorMesh::sendAckTo(const ClientInfo& dest, uint32_t ack_hash, uint8_t path_hash_size) {
  if (dest.out_path_len == OUT_PATH_UNKNOWN) {
    mesh::Packet* ack = createAck(ack_hash);
    if (ack) sendFlood(ack, TXT_ACK_DELAY, path_hash_size);
  } else {
    uint32_t d = TXT_ACK_DELAY;
    if (getExtraAckTransmitCount() > 0) {
      mesh::Packet* a1 = createMultiAck(ack_hash, 1);
      if (a1) sendDirect(a1, dest.out_path, dest.out_path_len, d);
      d += 300;
    }

    mesh::Packet* a2 = createAck(ack_hash);
    if (a2) sendDirect(a2, dest.out_path, dest.out_path_len, d);
  }
}

void SensorMesh::onPeerDataRecv(mesh::Packet* packet, uint8_t type, int sender_idx, const uint8_t* secret, uint8_t* data, size_t len) {
  int i = matching_peer_indexes[sender_idx];
  if (i < 0 || i >= acl.getNumClients()) {
    MESH_DEBUG_PRINTLN("onPeerDataRecv: Invalid sender idx: %d", i);
    return;
  }

  ClientInfo* from = acl.getClientByIdx(i);

  if (type == PAYLOAD_TYPE_REQ) {  // request (from a known contact)
    uint32_t timestamp;
    memcpy(&timestamp, data, 4);

    if (timestamp > from->last_timestamp) {  // prevent replay attacks
      uint8_t reply_len = handleRequest(from->isAdmin() ? 0xFF : from->permissions, timestamp, data[4], &data[5], len - 5);
      if (reply_len == 0) return;  // invalid command

      from->last_timestamp = timestamp;
      from->last_activity = getRTCClock()->getCurrentTime();

      if (packet->isRouteFlood()) {
        // let this sender know path TO here, so they can use sendDirect(), and ALSO encode the response
        mesh::Packet* path = createPathReturn(from->id, secret, packet->path, packet->path_len,
                                              PAYLOAD_TYPE_RESPONSE, reply_data, reply_len);
        if (path) sendFlood(path, SERVER_RESPONSE_DELAY, packet->getPathHashSize());
      } else {
        mesh::Packet* reply = createDatagram(PAYLOAD_TYPE_RESPONSE, from->id, secret, reply_data, reply_len);
        if (reply) {
          if (from->out_path_len != OUT_PATH_UNKNOWN) {  // we have an out_path, so send DIRECT
            sendDirect(reply, from->out_path, from->out_path_len, SERVER_RESPONSE_DELAY);
          } else {
            sendFlood(reply, SERVER_RESPONSE_DELAY, packet->getPathHashSize());
          }
        }
      }
    } else {
      MESH_DEBUG_PRINTLN("onPeerDataRecv: possible replay attack detected");
    }
  } else if (type == PAYLOAD_TYPE_TXT_MSG && len > 5 && from->isAdmin()) {   // a CLI command
    uint32_t sender_timestamp;
    memcpy(&sender_timestamp, data, 4);  // timestamp (by sender's RTC clock - which could be wrong)
    uint8_t flags = (data[4] >> 2);   // message attempt number, and other flags

    if (sender_timestamp > from->last_timestamp) {  // prevent replay attacks
      if (flags == TXT_TYPE_PLAIN) {
        bool handled = handleIncomingMsg(*from, sender_timestamp, &data[5], flags, len - 5);
        if (handled) { // if msg was handled then send an ack
          uint32_t ack_hash;    // calc truncated hash of the message timestamp + text + sender pub_key, to prove to sender that we got it
          mesh::Utils::sha256((uint8_t *) &ack_hash, 4, data, 5 + strlen((char *)&data[5]), from->id.pub_key, PUB_KEY_SIZE);

          if (packet->isRouteFlood()) {
            // let this sender know path TO here, so they can use sendDirect(), and ALSO encode the ACK
            mesh::Packet* path = createPathReturn(from->id, secret, packet->path, packet->path_len,
                                                  PAYLOAD_TYPE_ACK, (uint8_t *) &ack_hash, 4);
            if (path) sendFlood(path, TXT_ACK_DELAY, packet->getPathHashSize());
          } else {
            sendAckTo(*from, ack_hash, packet->getPathHashSize());
          }
        }
      } else if (flags == TXT_TYPE_CLI_DATA) {  
        from->last_timestamp = sender_timestamp;
        from->last_activity = getRTCClock()->getCurrentTime();

        // len can be > original length, but 'text' will be padded with zeroes
        data[len] = 0; // need to make a C string again, with null terminator

        uint8_t temp[166];
        char *command = (char *) &data[5];
        char *reply = (char *) &temp[5];
        handleCommand(sender_timestamp, command, reply);

        int text_len = strlen(reply);
        if (text_len > 0) {
          uint32_t timestamp = getRTCClock()->getCurrentTimeUnique();
          if (timestamp == sender_timestamp) {
            // WORKAROUND: the two timestamps need to be different, in the CLI view
            timestamp++;
          }
          memcpy(temp, &timestamp, 4);   // mostly an extra blob to help make packet_hash unique
          temp[4] = (TXT_TYPE_CLI_DATA << 2);

          auto reply = createDatagram(PAYLOAD_TYPE_TXT_MSG, from->id, secret, temp, 5 + text_len);
          if (reply) {
            if (from->out_path_len == OUT_PATH_UNKNOWN) {
              sendFlood(reply, CLI_REPLY_DELAY_MILLIS, packet->getPathHashSize());
            } else {
              sendDirect(reply, from->out_path, from->out_path_len, CLI_REPLY_DELAY_MILLIS);
            }
          }
        }
      } else {
        MESH_DEBUG_PRINTLN("onPeerDataRecv: unsupported text type received: flags=%02x", (uint32_t)flags);
      }
    } else {
      MESH_DEBUG_PRINTLN("onPeerDataRecv: possible replay attack detected");
    }
  }
}

bool SensorMesh::handleIncomingMsg(ClientInfo& from, uint32_t timestamp, uint8_t* data, uint8_t flags, size_t len) {
  MESH_DEBUG_PRINT("handleIncomingMsg: unhandled msg from ");
  #ifdef MESH_DEBUG
  mesh::Utils::printHex(Serial, from.id.pub_key, PUB_KEY_SIZE);
  Serial.printf(": %s\n", data);
  #endif
  return false;
}

#define CTL_TYPE_NODE_DISCOVER_REQ   0x80
#define CTL_TYPE_NODE_DISCOVER_RESP  0x90

void SensorMesh::onControlDataRecv(mesh::Packet* packet) {
  uint8_t type = packet->payload[0] & 0xF0;    // just test upper 4 bits
  if (type == CTL_TYPE_NODE_DISCOVER_REQ && packet->payload_len >= 6) {
    // TODO: apply rate limiting to these!
    int i = 1;
    uint8_t  filter = packet->payload[i++];
    uint32_t tag;
    memcpy(&tag, &packet->payload[i], 4); i += 4;
    uint32_t since;
    if (packet->payload_len >= i+4) {   // optional since field
      memcpy(&since, &packet->payload[i], 4); i += 4;
    } else {
      since = 0;
    }

    if ((filter & (1 << ADV_TYPE_SENSOR)) != 0 && _prefs.discovery_mod_timestamp >= since) {
      bool prefix_only = packet->payload[0] & 1;
      uint8_t data[6 + PUB_KEY_SIZE];
      data[0] = CTL_TYPE_NODE_DISCOVER_RESP | ADV_TYPE_SENSOR;   // low 4-bits for node type
      data[1] = packet->_snr;   // let sender know the inbound SNR ( x 4)
      memcpy(&data[2], &tag, 4);     // include tag from request, for client to match to
      memcpy(&data[6], self_id.pub_key, PUB_KEY_SIZE);
      auto resp = createControlData(data,  prefix_only ? 6 + 8 : 6 + PUB_KEY_SIZE);
      if (resp) {
        sendZeroHop(resp, getRetransmitDelay(resp)*4);  // apply random delay (widened x4), as multiple nodes can respond to this
      }
    }
  }
}

bool SensorMesh::onPeerPathRecv(mesh::Packet* packet, int sender_idx, const uint8_t* secret, uint8_t* path, uint8_t path_len, uint8_t extra_type, uint8_t* extra, uint8_t extra_len) {
  int i = matching_peer_indexes[sender_idx];
  if (i < 0 || i >= acl.getNumClients()) {
    MESH_DEBUG_PRINTLN("onPeerPathRecv: Invalid sender idx: %d", i);
    return false;
  }

  ClientInfo* from = acl.getClientByIdx(i);

  MESH_DEBUG_PRINTLN("PATH to contact, path_len=%d", (uint32_t) path_len);
  // NOTE: for this impl, we just replace the current 'out_path' regardless, whenever sender sends us a new out_path.
  // FUTURE: could store multiple out_paths per contact, and try to find which is the 'best'(?)
  from->out_path_len = mesh::Packet::copyPath(from->out_path, path, path_len);  // store a copy of path, for sendDirect()
  from->last_activity = getRTCClock()->getCurrentTime();

  // REVISIT: maybe make ALL out_paths non-persisted to minimise flash writes??
  if (from->isAdmin()) {
    // only do saveContacts() (of this out_path change) if this is an admin
    dirty_contacts_expiry = futureMillis(LAZY_CONTACTS_WRITE_DELAY);
  }

  // NOTE: no reciprocal path send!!
  return false;
}

void SensorMesh::onAckRecv(mesh::Packet* packet, uint32_t ack_crc) {
  if (num_alert_tasks > 0) {
    auto t = alert_tasks[0];   // check current alert task
    for (int i = 0; i < t->attempt; i++) {
      if (ack_crc == t->expected_acks[i]) {   // matching ACK!
        t->attempt = 4;  // signal to move to next contact
        t->send_expiry = 0;
        packet->markDoNotRetransmit();   // ACK was for this node, so don't retransmit
        return;
      }
    }
  }
}

SensorMesh::SensorMesh(mesh::MainBoard& board, mesh::Radio& radio, mesh::MillisecondClock& ms, mesh::RNG& rng, mesh::RTCClock& rtc, mesh::MeshTables& tables)
     : mesh::Mesh(radio, ms, rng, rtc, *new StaticPoolPacketManager(32), tables),
      _cli(board, rtc, sensors, acl, &_prefs, this), telemetry(MAX_PACKET_PAYLOAD - 4)
{
  next_local_advert = next_flood_advert = 0;
  dirty_contacts_expiry = 0;
  last_read_time = 0;
  num_alert_tasks = 0;
  set_radio_at = revert_radio_at = 0;

  // defaults
  memset(&_prefs, 0, sizeof(_prefs));
  _prefs.airtime_factor = 1.0;
  _prefs.rx_delay_base =   0.0f;  // turn off by default, was 10.0;
  _prefs.tx_delay_factor = 0.5f;   // was 0.25f
  _prefs.direct_tx_delay_factor = 0.2f; // was zero
  StrHelper::strncpy(_prefs.node_name, ADVERT_NAME, sizeof(_prefs.node_name));
  _prefs.node_lat = ADVERT_LAT;
  _prefs.node_lon = ADVERT_LON;
  StrHelper::strncpy(_prefs.password, ADMIN_PASSWORD, sizeof(_prefs.password));
  _prefs.freq = LORA_FREQ;
  _prefs.sf = LORA_SF;
  _prefs.bw = LORA_BW;
  _prefs.cr = LORA_CR;
  _prefs.tx_power_dbm = LORA_TX_POWER;
  _prefs.advert_interval = 1;  // default to 2 minutes for NEW installs
  _prefs.flood_advert_interval = 0;   // disabled
  _prefs.disable_fwd = true;
  _prefs.flood_max = 64;
  _prefs.interference_threshold = 0;  // disabled

  // GPS defaults
  _prefs.gps_enabled = 0;
  _prefs.gps_interval = 0;
  _prefs.advert_loc_policy = ADVERT_LOC_PREFS;
}

void SensorMesh::loadBTHomeConfig() {
  struct LegacyBTHomeConfigV1File {
    uint32_t magic;
    uint8_t version;
    uint8_t enabled;
    uint8_t has_target;
    uint8_t reserved;
    uint8_t target_mac[6];
  };

  struct LegacyBTHomeConfigV2File {
    uint32_t magic;
    uint8_t version;
    uint8_t enabled;
    uint8_t target_count;
    uint8_t reserved;
    uint8_t target_macs[4][6];
  };

  struct LegacyBTHomeConfigV3File {
    uint32_t magic;
    uint8_t version;
    uint8_t enabled;
    uint8_t target_count;
    uint8_t reserved;
    uint8_t target_macs[BTHomeScanner::MAX_TARGETS][6];
  };

  BTHomeConfigFile cfg;
  memset(&cfg, 0, sizeof(cfg));
  LegacyBTHomeConfigV1File legacy_v1_cfg;
  memset(&legacy_v1_cfg, 0, sizeof(legacy_v1_cfg));
  LegacyBTHomeConfigV2File legacy_v2_cfg;
  memset(&legacy_v2_cfg, 0, sizeof(legacy_v2_cfg));
  LegacyBTHomeConfigV3File legacy_v3_cfg;
  memset(&legacy_v3_cfg, 0, sizeof(legacy_v3_cfg));

#if defined(RP2040_PLATFORM)
  File file = _fs->open(BTHOME_CONFIG_FILE, "r");
#else
  File file = _fs->open(BTHOME_CONFIG_FILE);
#endif
  if (!file) {
    _bthome.setEnabled(false);
    _bthome.clearTargetMacs();
    _met_report.clearTarget();
    _met_report.publish_mask = 0;
    _met_report.clearChannel();
    return;
  }

  size_t loaded = file.read((uint8_t*) &cfg, sizeof(cfg));
  file.close();

  _bthome.setEnabled(false);
  _bthome.clearTargetMacs();
  _met_report.clearTarget();
  _met_report.publish_mask = 0;
  _met_report.clearChannel();

  if (loaded >= sizeof(cfg) && cfg.magic == BTHOME_CONFIG_MAGIC && cfg.version == BTHOME_CONFIG_VERSION) {
    _bthome.setEnabled(cfg.enabled != 0);
    uint8_t count = min(cfg.target_count, BTHomeScanner::MAX_TARGETS);
    for (uint8_t i = 0; i < count; i++) {
      _bthome.addTargetMac(cfg.target_macs[i]);
    }
    if (cfg.met_target_configured != 0) {
      memcpy(_met_report.target_mac, cfg.met_target_mac, sizeof(_met_report.target_mac));
      _met_report.target_configured = 1;
    }
    _met_report.publish_mask = cfg.met_publish_mask &
        (BTHOME_MET_PUBLISH_MORNING | BTHOME_MET_PUBLISH_MIDDAY | BTHOME_MET_PUBLISH_EVENING);
    if (cfg.met_channel_secret_len == 16 || cfg.met_channel_secret_len == 32) {
      memcpy(_met_report.channel.secret, cfg.met_channel_secret, sizeof(_met_report.channel.secret));
      mesh::Utils::sha256(
          _met_report.channel.hash,
          sizeof(_met_report.channel.hash),
          _met_report.channel.secret,
          cfg.met_channel_secret_len);
      _met_report.channel_secret_len = cfg.met_channel_secret_len;
    }
    return;
  }

  memcpy(&legacy_v3_cfg, &cfg, min((size_t) loaded, sizeof(legacy_v3_cfg)));
  if (loaded >= sizeof(legacy_v3_cfg) && legacy_v3_cfg.magic == BTHOME_CONFIG_MAGIC && legacy_v3_cfg.version == 3) {
    _bthome.setEnabled(legacy_v3_cfg.enabled != 0);
    uint8_t count = min(legacy_v3_cfg.target_count, BTHomeScanner::MAX_TARGETS);
    for (uint8_t i = 0; i < count; i++) {
      _bthome.addTargetMac(legacy_v3_cfg.target_macs[i]);
    }
    return;
  }

  memcpy(&legacy_v2_cfg, &cfg, min((size_t) loaded, sizeof(legacy_v2_cfg)));
  if (loaded >= sizeof(legacy_v2_cfg) && legacy_v2_cfg.magic == BTHOME_CONFIG_MAGIC && legacy_v2_cfg.version == 2) {
    _bthome.setEnabled(legacy_v2_cfg.enabled != 0);
    uint8_t count = min(legacy_v2_cfg.target_count, (uint8_t) 4);
    for (uint8_t i = 0; i < count; i++) {
      _bthome.addTargetMac(legacy_v2_cfg.target_macs[i]);
    }
    return;
  }

  memcpy(&legacy_v1_cfg, &cfg, min((size_t) loaded, sizeof(legacy_v1_cfg)));
  if (loaded >= sizeof(legacy_v1_cfg) && legacy_v1_cfg.magic == BTHOME_CONFIG_MAGIC && legacy_v1_cfg.version == 1) {
    _bthome.setEnabled(legacy_v1_cfg.enabled != 0);
    if (legacy_v1_cfg.has_target) {
      _bthome.addTargetMac(legacy_v1_cfg.target_mac);
    }
  }
}

void SensorMesh::saveBTHomeConfig() {
  if (_fs == nullptr) {
    return;
  }

  BTHomeConfigFile cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.magic = BTHOME_CONFIG_MAGIC;
  cfg.version = BTHOME_CONFIG_VERSION;
  cfg.enabled = _bthome.isEnabled() ? 1 : 0;
  cfg.target_count = _bthome.getTargetCount();
  for (uint8_t i = 0; i < cfg.target_count; i++) {
    uint8_t target_mac[6];
    if (_bthome.getTargetMac(i, target_mac)) {
      memcpy(cfg.target_macs[i], target_mac, sizeof(cfg.target_macs[i]));
    }
  }
  cfg.met_target_configured = _met_report.hasTarget() ? 1 : 0;
  cfg.met_publish_mask = _met_report.publish_mask;
  cfg.met_channel_secret_len = _met_report.hasChannel() ? _met_report.channel_secret_len : 0;
  if (cfg.met_target_configured != 0) {
    memcpy(cfg.met_target_mac, _met_report.target_mac, sizeof(cfg.met_target_mac));
  }
  if (cfg.met_channel_secret_len != 0) {
    memcpy(cfg.met_channel_secret, _met_report.channel.secret, sizeof(cfg.met_channel_secret));
  }

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  _fs->remove(BTHOME_CONFIG_FILE);
  File file = _fs->open(BTHOME_CONFIG_FILE, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File file = _fs->open(BTHOME_CONFIG_FILE, "w");
#else
  File file = _fs->open(BTHOME_CONFIG_FILE, "w", true);
#endif
  if (!file) {
    return;
  }
  file.write((uint8_t*) &cfg, sizeof(cfg));
  file.close();
}

void SensorMesh::begin(FILESYSTEM* fs) {
  mesh::Mesh::begin();
  _fs = fs;
  // load persisted prefs
  _cli.loadPrefs(_fs);
  _bthome.begin();
  loadBTHomeConfig();

  acl.load(_fs, self_id);

  radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
  radio_set_tx_power(_prefs.tx_power_dbm);

  updateAdvertTimer();
  updateFloodAdvertTimer();

   board.setAdcMultiplier(_prefs.adc_multiplier);

#if ENV_INCLUDE_GPS == 1
  applyGpsPrefs();
#endif
}

bool SensorMesh::formatFileSystem() {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
    return InternalFS.format();
#elif defined(RP2040_PLATFORM)
    return LittleFS.format();
#elif defined(ESP32)
    return SPIFFS.format();
#else
    #error "need to implement file system erase"
    return false;
#endif
}

void SensorMesh::saveIdentity(const mesh::LocalIdentity& new_id) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  IdentityStore store(*_fs, "");
#elif defined(ESP32)
  IdentityStore store(*_fs, "/identity");
#elif defined(RP2040_PLATFORM)
  IdentityStore store(*_fs, "/identity");
#else
  #error "need to define saveIdentity()"
#endif
  store.save("_main", new_id);
}

void SensorMesh::applyTempRadioParams(float freq, float bw, uint8_t sf, uint8_t cr, int timeout_mins) {
  set_radio_at = futureMillis(2000);   // give CLI reply some time to be sent back, before applying temp radio params
  pending_freq = freq;
  pending_bw = bw;
  pending_sf = sf;
  pending_cr = cr;

  revert_radio_at = futureMillis(2000 + timeout_mins*60*1000);   // schedule when to revert radio params
}

void SensorMesh::sendSelfAdvertisement(int delay_millis, bool flood) {
  mesh::Packet* pkt = createSelfAdvert();
  if (pkt) {
    if (flood) {
      sendFlood(pkt, delay_millis, _prefs.path_hash_mode + 1);
    } else {
      sendZeroHop(pkt, delay_millis);
    }
  } else {
    MESH_DEBUG_PRINTLN("ERROR: unable to create advertisement packet!");
  }
}

void SensorMesh::updateAdvertTimer() {
  if (_prefs.advert_interval > 0) {  // schedule local advert timer
    next_local_advert = futureMillis( ((uint32_t)_prefs.advert_interval) * 2 * 60 * 1000);
  } else {
    next_local_advert = 0;  // stop the timer
  }
}
void SensorMesh::updateFloodAdvertTimer() {
  if (_prefs.flood_advert_interval > 0) {  // schedule flood advert timer
    next_flood_advert = futureMillis( ((uint32_t)_prefs.flood_advert_interval) * 60 * 60 * 1000);
  } else {
    next_flood_advert = 0;  // stop the timer
  }
}

void SensorMesh::setTxPower(int8_t power_dbm) {
  radio_set_tx_power(power_dbm);
}

void SensorMesh::formatStatsReply(char *reply) {
  StatsFormatHelper::formatCoreStats(reply, board, *_ms, _err_flags, _mgr);
}

void SensorMesh::formatRadioStatsReply(char *reply) {
  StatsFormatHelper::formatRadioStats(reply, _radio, radio_driver, getTotalAirTime(), getReceiveAirTime());
}

void SensorMesh::formatPacketStatsReply(char *reply) {
  StatsFormatHelper::formatPacketStats(reply, radio_driver, getNumSentFlood(), getNumSentDirect(), 
                                       getNumRecvFlood(), getNumRecvDirect());
}

float SensorMesh::getTelemValue(uint8_t channel, uint8_t type) {
  auto buf = telemetry.getBuffer();
  uint8_t size = telemetry.getSize();
  uint8_t i = 0;

  while (i + 2 < size) {
    // Get channel #
    uint8_t ch = buf[i++];
    // Get data type
    uint8_t t = buf[i++];
    uint8_t sz = getDataSize(t);

    if (ch == channel && t == type) {
      return getFloat(&buf[i], sz, getMultiplier(t), isSigned(t));
    }
    i += sz;  // skip
  }
  return 0.0f;   // not found
}

bool  SensorMesh::getGPS(uint8_t channel, float& lat, float& lon, float& alt) {
  if (channel == TELEM_CHANNEL_SELF) {
    lat = sensors.node_lat;
    lon = sensors.node_lon;
    alt = sensors.node_altitude;
    return true;
  }
  // REVISIT: custom GPS channels??
  return false;
}

void SensorMesh::loop() {
  mesh::Mesh::loop();
  _bthome.loop();

  if (next_flood_advert && millisHasNowPassed(next_flood_advert)) {
    mesh::Packet* pkt = createSelfAdvert();
    unsigned long delay_millis = 0;
    if (pkt) sendFlood(pkt, delay_millis, _prefs.path_hash_mode + 1);

    updateFloodAdvertTimer();   // schedule next flood advert
    updateAdvertTimer();   // also schedule local advert (so they don't overlap)
  } else if (next_local_advert && millisHasNowPassed(next_local_advert)) {
    mesh::Packet* pkt = createSelfAdvert();
    if (pkt) sendZeroHop(pkt);

    updateAdvertTimer();   // schedule next local advert
  }

  if (set_radio_at && millisHasNowPassed(set_radio_at)) {   // apply pending (temporary) radio params
    set_radio_at = 0;  // clear timer
    radio_set_params(pending_freq, pending_bw, pending_sf, pending_cr);
    MESH_DEBUG_PRINTLN("Temp radio params");
  }

  if (revert_radio_at && millisHasNowPassed(revert_radio_at)) {   // revert radio params to orig
    revert_radio_at = 0;  // clear timer
    radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
    MESH_DEBUG_PRINTLN("Radio params restored");
  }

  uint32_t curr = getRTCClock()->getCurrentTime();
  if (curr >= last_read_time + SENSOR_READ_INTERVAL_SECS) {
    buildTelemetry(0xFF);

    onSensorDataRead();
    recordBTHomeMetHistory();

    last_read_time = curr;
  }

  maybePublishBTHomeMetReport();

  // check the alert send queue
  if (num_alert_tasks > 0) {
    auto t = alert_tasks[0];   // process head of queue

    if (millisHasNowPassed(t->send_expiry)) {  // next send needed?
      if (t->attempt >= 4) {  // max attempts reached, try next contact
        t->curr_contact_idx++;
        if (t->curr_contact_idx >= acl.getNumClients()) {  // no more contacts to try?
          num_alert_tasks--;   // remove t from queue
          for (int i = 0; i < num_alert_tasks; i++) {
            alert_tasks[i] = alert_tasks[i + 1];
          }
        } else {
          auto c = acl.getClientByIdx(t->curr_contact_idx);
          uint16_t pri_mask = (t->pri == HIGH_PRI_ALERT) ? PERM_RECV_ALERTS_HI : PERM_RECV_ALERTS_LO;

          if (c->permissions & pri_mask) {   // contact wants alert
            // reset attempts
            t->attempt = (t->pri == LOW_PRI_ALERT) ? 3 : 0;   // Low pri alerts, start at attempt #3 (ie. only make ONE attempt)
            t->timestamp = getRTCClock()->getCurrentTimeUnique();   // need unique timestamp per contact

            sendAlert(c, t);  // NOTE: modifies attempt, expected_acks[] and send_expiry
          } else {
            // next contact tested in next ::loop()
          }
        }
      } else if (t->curr_contact_idx < acl.getNumClients()) {
        auto c = acl.getClientByIdx(t->curr_contact_idx);   // send next attempt
        sendAlert(c, t);  // NOTE: modifies attempt, expected_acks[] and send_expiry
      } else {
        // contact list has likely been modified while waiting for alert ACK, cancel this task
        t->attempt = 4;   // next ::loop() will remove t from queue
      }
    }
  }

  // is there are pending dirty contacts write needed?
  if (dirty_contacts_expiry && millisHasNowPassed(dirty_contacts_expiry)) {
    acl.save(_fs);
    dirty_contacts_expiry = 0;
  }
}
