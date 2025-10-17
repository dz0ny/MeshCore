#pragma once
#include <cstdint> // For uint8_t, uint32_t

#define TELEM_MODE_DENY            0
#define TELEM_MODE_ALLOW_FLAGS     1     // use contact.flags
#define TELEM_MODE_ALLOW_ALL       2

#define ADVERT_LOC_NONE       0
#define ADVERT_LOC_SHARE      1

struct NodePrefs {  // persisted to file
  float airtime_factor;
  char node_name[32];
  float freq;
  uint8_t sf;
  uint8_t cr;
  uint8_t multi_acks;
  uint8_t manual_add_contacts;
  float bw;
  uint8_t tx_power_dbm;
  uint8_t telemetry_mode_base;
  uint8_t telemetry_mode_loc;
  uint8_t telemetry_mode_env;
  float rx_delay_base;
  uint32_t ble_pin;
  uint8_t  advert_loc_policy;
  // GPS location advertising settings
  uint8_t gps_loc_advert_enabled;       // 0=off, 1=on
  uint8_t gps_loc_distance_threshold;   // meters (1-20)
  uint8_t gps_loc_frequency;            // seconds/10 (30s-300s stored as 3-30)
  uint8_t gps_loc_guaranteed_interval;  // 0=1min, 1=5min, 2=15min
  uint8_t gps_loc_accuracy_threshold;   // meters (3-20, GPS accuracy must be better than this)
  double last_advert_lat, last_advert_lon; // Track last advertised position
  // UI settings
  uint8_t buzzer_key_press;             // 0=off, 1=on (buzzer feedback for key presses)
};