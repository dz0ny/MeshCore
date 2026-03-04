#ifndef UI_STRINGS_H
#define UI_STRINGS_H

#include <Arduino.h>

// Language enum
enum UILanguage {
  LANG_EN = 0,  // English
  LANG_SL = 1,  // Slovenian
  LANG_HR = 2,  // Croatian
  LANG_COUNT    // Total number of languages
};

// String ID enumeration - all translatable UI strings
enum UIStringID {
  // Common
  STR_ON,
  STR_OFF,
  STR_NOT_AVAILABLE,
  STR_NOT_SUPPORTED,

  // SettingsScreen
  STR_SETTINGS,
  STR_HEADER_CONNECTIVITY,
  STR_HEADER_SOUND,
  STR_HEADER_GPS,
  STR_HEADER_LOCATION_ADVERT,
  STR_HEADER_PRIVACY,
  STR_HEADER_MAINTENANCE,
  STR_BLUETOOTH,
  STR_BUZZER,
  STR_KEY_PRESS_BUZZER,
  STR_GPS,
  STR_BROADCAST_LOCATION,
  STR_MOVEMENT_THRESHOLD,
  STR_UPDATE_FREQUENCY,
  STR_GUARANTEED_INTERVAL,
  STR_REQUIRED_ACCURACY,
  STR_TELEMETRY_SHARE,
  STR_ADVERTISE_LOCATION,
  STR_CLEAR_FILES,
  STR_LANGUAGE,
  STR_DENY,
  STR_FLAGS,
  STR_ALL,
  STR_INTERVAL_1M,
  STR_INTERVAL_2M,
  STR_INTERVAL_5M,
  STR_INTERVAL_10M,
  STR_INTERVAL_15M,
  STR_ALERT_BLE_OFF,
  STR_ALERT_BLE_ON,
  STR_ALERT_KEY_BUZZ_ON,
  STR_ALERT_KEY_BUZZ_OFF,
  STR_ALERT_LOCADV_ON,
  STR_ALERT_LOCADV_OFF,
  STR_ALERT_DISTANCE,        // "Distance: %dm"
  STR_ALERT_FREQUENCY,       // "Frequency: %ds"
  STR_ALERT_INTERVAL,        // "Interval: %s"
  STR_ALERT_ACCURACY,        // "Accuracy: %dm"
  STR_ALERT_TELEMETRY,       // "Telemetry: %s"
  STR_ALERT_ADVERT_LOC_ON,
  STR_ALERT_ADVERT_LOC_OFF,
  STR_ALERT_CLEARING_FILES,
  STR_ALERT_FILES_CLEARED_REBOOTING,
  STR_ALERT_CLEAR_FAILED,
  STR_ALERT_LANGUAGE_CHANGED, // "Language: %s"
  STR_LANG_ENGLISH,
  STR_LANG_SLOVENIAN,
  STR_LANG_CROATIAN,

  // SystemStatsScreen
  STR_SYSTEM_STATS,
  STR_SECTION_RUNTIME,
  STR_SECTION_HEAP,
  STR_SECTION_STACK,
  STR_UPTIME,
  STR_FREE,
  STR_MIN_FREE,
  STR_TOTAL,
  STR_USAGE,
  STR_FREE_WORDS,
  STR_FREE_BYTES,
  STR_UNIT_HOURS,            // "h"
  STR_UNIT_MINUTES,          // "m"
  STR_UNIT_SECONDS,          // "s"
  STR_UNIT_BYTES,            // "B"
  STR_UNIT_PERCENT,          // "%"

  // ContactsScreen
  STR_RECENT_CONTACTS,
  STR_NO_NEARBY_CONTACTS,
  STR_NO_GPS_FIX,
  STR_NEED_GPS_TO_SHOW_MAP,
  STR_UNIT_DAYS,             // "d"
  STR_UNIT_KM,               // "km"

  // MessagesScreen
  STR_MESSAGES,
  STR_MESSAGES_COUNT,        // "Messages %d/%d" or "Messages %d"
  STR_OPTIONS,
  STR_NO_SAVED_MESSAGES,
  STR_READ,
  STR_DELETE,
  STR_FROM,                  // "From: %s"
  STR_FROM_HOPS,             // "From: %s (h%d)"

  // RadioStatsScreen
  STR_RADIO_STATS,

  // ReportsScreen
  STR_REPORTS,
  STR_SHOW_GPS_INFO,
  STR_SHOW_RADIO_STATS,
  STR_SHOW_TELEMETRY,
  STR_SHOW_DEBUG_KEYS,
  STR_SHOW_SYSTEM_STATS,

  // GPSScreen
  STR_GPS_OFF,
  STR_ENABLE_IN_SETTINGS,
  STR_GPS_ERROR,
  STR_CANT_ACCESS_GPS,
  STR_GPS_FIX,
  STR_GPS_SEARCH,
  STR_LAT,
  STR_LON,
  STR_ALT,
  STR_ACC,

  // TelemetryScreen
  STR_TELEMETRY,
  STR_BATTERY,
  STR_TEMPERATURE,
  STR_HUMIDITY,
  STR_PRESSURE,
  STR_LATITUDE,
  STR_LONGITUDE,
  STR_NO_ADDITIONAL_SENSORS,
  STR_DETECTED,
  STR_UNIT_CELSIUS,          // "C"
  STR_UNIT_HPA,              // "hPa"

  // HomeScreen
  STR_CONNECTED,
  STR_PIN,                   // "Pin:%d"
  STR_NEARBY,                // "%d nearby"
  STR_NO_CONTACTS,
  STR_PRESS_TO_VIEW,
  STR_NO_MESSAGES,
  STR_UNREAD,                // "%d unread"
  STR_TOTAL_COUNT,           // "%d total"
  STR_ID,
  STR_SHARE,
  STR_FIX,
  STR_SATS,
  STR_SEND_LOCATION,
  STR_PRESS_ENTER,
  STR_LONG_PRESS,
  STR_TO_SEND,
  STR_HIBERNATING,
  STR_HIBERNATE,
  STR_UNREAD_COUNT,          // "Unread: %d"
  STR_ADVERT_SENT,
  STR_ADVERT_FAILED,
  STR_ENTER_WHEN_PAIRING,    // "Enter when pairing"

  // Keep as last
  STR_COUNT
};

// Forward declaration
struct NodePrefs;

// Get current UI language from NodePrefs
extern UILanguage uiGetCurrentLanguage(NodePrefs* prefs);

// Set UI language (updates NodePrefs)
extern void uiSetLanguage(NodePrefs* prefs, UILanguage lang);

// Translate string ID to current language
// Returns pointer to string in RAM buffer (copied from PROGMEM)
extern const char* uiGetString(NodePrefs* prefs, UIStringID id);

// Convenience macro for translation - requires _node_prefs in scope
#define TR(id) uiGetString(_node_prefs, id)

// Get language name for display
extern const char* uiGetLanguageName(NodePrefs* prefs, UILanguage lang);

#endif // UI_STRINGS_H
